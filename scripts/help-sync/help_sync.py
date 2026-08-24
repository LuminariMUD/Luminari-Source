#!/usr/bin/env python3
"""Guarded two-way help synchronization for LuminariMUD."""

from __future__ import annotations

import argparse
from dataclasses import replace
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
from typing import Any, Mapping, Sequence

from catalog import (
    Catalog,
    HelpSyncError,
    MergeResult,
    Rename,
    build_plan_core,
    canonical_json_bytes,
    merge_catalogs,
    resolve_merge,
    seal_plan,
    sha256_bytes,
    validate_plan,
)
from endpoint import (
    ApplyFailure,
    EndpointError,
    EndpointSnapshot,
    HelpWriteBarrier,
    STATE_DIRECTORY_NAME,
    StalePlanError,
    apply_plan_to_endpoint,
    atomic_write_json,
    environment_config,
    environment_name,
    load_common_baseline,
    normalize_integrity_repairs,
    repair_missing_keywords,
    rollback_endpoint,
    take_snapshot,
    utc_now,
    verify_endpoint,
    write_common_baseline,
)


JSON_BEGIN = "HELP_SYNC_JSON_BEGIN"
JSON_END = "HELP_SYNC_JSON_END"
PLAN_SUFFIX = ".json"
REMOTE_USER_RE = re.compile(r"^[a-z_][a-z0-9_-]*[$]?$", re.IGNORECASE)
DEFAULT_SYNC_PASSES = 5
MAX_SYNC_PASSES = 10
RETRYABLE_SYNC_DRIFT = (
    "authorization token from a fresh preview",
    "changed after planning",
    "changed during apply",
    "changed after the plan snapshot",
    "database catalog hash does not match candidate",
    "help.hlp is not the deterministic candidate projection",
    "old-hash precondition failed",
    "planned addition",
    "production catalog changed after planning",
    "stale plan:",
)


def repository_root() -> Path:
    root = Path(__file__).resolve().parents[2]
    if not (root / "AGENTS.md").is_file() or not (root / "lib" / "text" / "help").is_dir():
        raise EndpointError("help-sync script is not inside a LuminariMUD repository")
    return root


def state_root(root: Path) -> Path:
    return root / "lib" / "text" / "help" / STATE_DIRECTORY_NAME


def plan_path(root: Path, plan_id: str) -> Path:
    return state_root(root) / "plans" / f"{plan_id}{PLAN_SUFFIX}"


def proof_path(root: Path, plan_id: str) -> Path:
    return state_root(root) / "proofs" / plan_id / "development.json"


def read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise EndpointError(f"file does not exist: {path}") from exc
    except json.JSONDecodeError as exc:
        raise EndpointError(f"invalid JSON in {path}: {exc}") from exc


def load_plan(root: Path, reference: str) -> dict[str, Any]:
    candidate = Path(reference)
    if candidate.is_file():
        value = read_json(candidate)
    elif len(reference) == 64 and all(character in "0123456789abcdef" for character in reference):
        value = read_json(plan_path(root, reference))
    else:
        raise EndpointError(
            "plan must be a readable JSON path or a stored 64-character plan ID"
        )
    if not isinstance(value, dict):
        raise EndpointError("plan JSON must be an object")
    validate_plan(value)
    return value


def save_plan(root: Path, plan: Mapping[str, Any]) -> Path:
    validate_plan(plan)
    destination = plan_path(root, str(plan["plan_id"]))
    if destination.exists():
        existing = read_json(destination)
        validate_plan(existing)
        if existing != plan:
            raise EndpointError("stored plan ID collision or noncanonical existing artifact")
        return destination
    atomic_write_json(destination, plan)
    return destination


class RemoteEndpoint:
    """Invoke the exact same engine in the configured production checkout."""

    def __init__(self, root: Path):
        values = environment_config(root)
        login = values.get("REMOTE_LOGIN_COMMAND", "").strip()
        project = values.get("REMOTE_PROJECT_PATH", "").strip()
        if not login or not project or not project.startswith("/"):
            raise EndpointError(
                "REMOTE_LOGIN_COMMAND and an absolute REMOTE_PROJECT_PATH are required"
            )
        self.login = shlex.split(login)
        self.project = project
        self.sudo_password = values.get("REMOTE_LOGIN_SUDO_PASSWORD", "")
        self.endpoint_user = values.get("REMOTE_HELP_SYNC_USER", "luminari").strip()
        if not self.login:
            raise EndpointError("REMOTE_LOGIN_COMMAND is empty")
        if "\n" in self.sudo_password or "\r" in self.sudo_password:
            raise EndpointError("REMOTE_LOGIN_SUDO_PASSWORD must be a single line")
        if self.sudo_password and not REMOTE_USER_RE.fullmatch(self.endpoint_user):
            raise EndpointError("REMOTE_HELP_SYNC_USER is not a valid local user name")

    def call(
        self,
        action: str,
        arguments: Sequence[str] = (),
        payload: Mapping[str, Any] | None = None,
        timeout: int = 180,
    ) -> Any:
        remote_arguments = [
            "python3",
            "scripts/help-sync/help_sync.py",
            "_endpoint",
            action,
            *arguments,
        ]
        if self.sudo_password:
            remote_arguments = [
                "sudo",
                "-k",
                "-S",
                "-p",
                "",
                "-u",
                self.endpoint_user,
                "--",
                *remote_arguments,
            ]
        remote_command = (
            f"cd {shlex.quote(self.project)} && " + shlex.join(remote_arguments)
        )
        input_bytes = canonical_json_bytes(payload) + b"\n" if payload is not None else None
        if self.sudo_password:
            input_bytes = self.sudo_password.encode("utf-8") + b"\n" + (input_bytes or b"")
        try:
            completed = subprocess.run(
                [*self.login, remote_command],
                input=input_bytes,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
                timeout=timeout,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            raise EndpointError(f"production endpoint transport failed: {exc}") from exc
        output = completed.stdout.decode("utf-8", errors="replace")
        if completed.returncode != 0:
            error = completed.stderr.decode("utf-8", errors="replace").strip()
            raise EndpointError(
                "production endpoint command failed"
                + (f": {error[-1200:]}" if error else "")
            )
        start = output.rfind(JSON_BEGIN + "\n")
        end = output.rfind("\n" + JSON_END)
        if start < 0 or end < start:
            raise EndpointError("production endpoint returned no framed JSON result")
        raw = output[start + len(JSON_BEGIN) + 1 : end]
        try:
            return json.loads(raw)
        except json.JSONDecodeError as exc:
            raise EndpointError("production endpoint returned invalid JSON") from exc

    def snapshot(self) -> EndpointSnapshot:
        return EndpointSnapshot.from_dict(self.call("snapshot"))

    def baseline(self) -> Catalog:
        return Catalog.from_dict(self.call("baseline-read"))

    def apply(self, plan: Mapping[str, Any]) -> dict[str, Any]:
        return dict(self.call("apply", payload=plan, timeout=300))

    def verify(self, candidate: Catalog) -> dict[str, Any]:
        return dict(self.call("verify", payload=candidate.to_dict()))

    def write_baseline(self, catalog: Catalog, plan_id: str) -> dict[str, Any]:
        return dict(
            self.call("baseline-write", ["--plan-id", plan_id], payload=catalog.to_dict())
        )

    def rollback(self, run_id: str, expected_hash: str) -> dict[str, Any]:
        return dict(
            self.call(
                "rollback",
                ["--run-id", run_id, "--expected-current-hash", expected_hash],
                timeout=300,
            )
        )


def require_development(root: Path) -> None:
    observed = environment_name(root)
    if observed != "development":
        raise EndpointError(
            f"public help-sync orchestration requires development, observed {observed!r}"
        )


def snapshot_state(snapshot: EndpointSnapshot) -> dict[str, Any]:
    return {
        "catalog_hash": snapshot.catalog.content_hash,
        "file_hash": snapshot.file_hash,
        "expected_file_hash": snapshot.expected_file_hash,
        "file_matches": snapshot.file_matches,
        "file_issues": list(snapshot.file_issues),
        "integrity_issues": list(snapshot.integrity_issues),
        "integrity_repairs": normalize_integrity_repairs(snapshot.integrity_repairs),
        "schema_write_ready": snapshot.schema.write_ready,
        "schema_missing_requirements": snapshot.schema.missing_requirements,
        "entry_count": len(snapshot.catalog.entries),
        "relationship_count": snapshot.catalog.relationship_count,
        "git_identity": snapshot.manifest.get("git_identity"),
    }


def parse_tombstones(path: str | None) -> dict[str, list[str]]:
    if path is None:
        return {"development": [], "production": []}
    value = read_json(Path(path))
    if not isinstance(value, dict):
        raise EndpointError("tombstone file must contain an object")
    return {
        "development": [str(tag) for tag in value.get("development", ())],
        "production": [str(tag) for tag in value.get("production", ())],
    }


def parse_renames(path: str | None) -> list[Rename]:
    if path is None:
        return []
    value = read_json(Path(path))
    if not isinstance(value, list):
        raise EndpointError("rename file must contain a list")
    return [
        Rename(side=str(item["side"]), old_tag=str(item["from"]), new_tag=str(item["to"]))
        for item in value
    ]


def prepare_merge_catalogs(
    base: Catalog,
    development: Catalog,
    production: Catalog,
    repair_integrity: bool,
) -> tuple[Catalog, Catalog, Catalog]:
    if not repair_integrity:
        return base, development, production
    return (
        base,
        repair_missing_keywords(development, base, production),
        repair_missing_keywords(production, base, development),
    )


def make_plan(
    base: Catalog,
    development: Catalog,
    production: Catalog,
    result: MergeResult,
    tombstones: Mapping[str, Sequence[str]],
    renames: Sequence[Rename],
    development_state: Mapping[str, Any],
    production_state: Mapping[str, Any],
    *,
    repair_integrity: bool,
    repair_layers: bool = False,
    parent_plan_id: str | None = None,
) -> dict[str, Any]:
    if repair_integrity:
        result = replace(
            result,
            candidate=repair_missing_keywords(
                result.candidate, base, development, production
            ),
        )
    core = build_plan_core(
        base,
        development,
        production,
        result,
        tombstones=tombstones,
        renames=renames,
        parent_plan_id=parent_plan_id,
    )
    core["sources"] = {
        "base": base.to_dict(),
        "development": development.to_dict(),
        "production": production.to_dict(),
    }
    core["development_state"] = dict(development_state)
    core["production_state"] = dict(production_state)
    core["development_integrity_repairs"] = normalize_integrity_repairs(
        development_state.get("integrity_repairs", {})
    )
    core["production_integrity_repairs"] = normalize_integrity_repairs(
        production_state.get("integrity_repairs", {})
    )
    core["layer_repairs"] = {
        "development_help_hlp": not bool(development_state.get("file_matches")),
        "production_help_hlp": not bool(production_state.get("file_matches")),
    }
    core["excluded_operational_data"] = [
        "database IDs",
        "created/update timestamps",
        "help_search_history",
        "existing help_versions rows",
        "player and unrelated tables",
    ]
    core["repair_integrity"] = repair_integrity
    core["repair_layers"] = repair_layers

    blockers: list[str] = []
    for side, state in (
        ("development", development_state),
        ("production", production_state),
    ):
        if not state.get("schema_write_ready"):
            blockers.append(f"{side} help schema is not write-ready")
        if not state.get("file_matches") and not repair_layers:
            blockers.append(
                f"{side} has database/help.hlp layer drift; rerun with "
                "--repair-layers after reviewing file-only work"
            )
        issues = list(state.get("integrity_issues", ()))
        if issues and not repair_integrity:
            blockers.append(
                f"{side} has {len(issues)} database integrity issues; rerun with "
                "--repair-integrity after review"
            )
        elif issues:
            repairs = normalize_integrity_repairs(state.get("integrity_repairs", {}))
            covered_issues = len(repairs["missing_keyword_tags"]) + sum(
                repair["count"] for repair in repairs["orphan_keywords"]
            )
            if covered_issues != len(issues):
                blockers.append(
                    f"{side} has integrity issues without a supported explicit repair"
                )
    if any(not entry.keywords for entry in result.candidate.entries):
        blockers.append("candidate contains entries without database lookup keywords")
    if blockers:
        core["sealed"] = False
        core["integrity_blockers"] = blockers
        core.pop("development_delta", None)
        core.pop("production_delta", None)

    plan = seal_plan(core)
    plan["created_at"] = utc_now()
    validate_plan(plan)
    return plan


def create_plan(
    root: Path,
    tombstones: Mapping[str, Sequence[str]],
    renames: Sequence[Rename],
    repair_integrity: bool,
    repair_layers: bool,
) -> dict[str, Any]:
    require_development(root)
    development_snapshot = take_snapshot(root, "development")
    remote = RemoteEndpoint(root)
    production_snapshot = remote.snapshot()
    if production_snapshot.environment != "production":
        raise EndpointError("configured remote did not identify itself as production")
    base = load_common_baseline(root)
    remote_base = remote.baseline()
    if remote_base.content_hash != base.content_hash:
        raise EndpointError("development and production common-baseline artifacts disagree")
    merge_base, merge_development, merge_production = prepare_merge_catalogs(
        base,
        development_snapshot.catalog,
        production_snapshot.catalog,
        repair_integrity,
    )
    result = merge_catalogs(
        merge_base,
        merge_development,
        merge_production,
        tombstones=tombstones,
        renames=renames,
    )
    return make_plan(
        base,
        development_snapshot.catalog,
        production_snapshot.catalog,
        result,
        tombstones,
        renames,
        snapshot_state(development_snapshot),
        snapshot_state(production_snapshot),
        repair_integrity=repair_integrity,
        repair_layers=repair_layers,
    )


def resolve_plan(plan: Mapping[str, Any], resolutions: Mapping[str, Any]) -> dict[str, Any]:
    validate_plan(plan)
    sources = plan.get("sources", {})
    base = Catalog.from_dict(sources["base"])
    development = Catalog.from_dict(sources["development"])
    production = Catalog.from_dict(sources["production"])
    tombstones = dict(plan.get("tombstones", {}))
    renames = [
        Rename(side=item["side"], old_tag=item["from"], new_tag=item["to"])
        for item in plan.get("renames", ())
    ]
    merge_base, merge_development, merge_production = prepare_merge_catalogs(
        base,
        development,
        production,
        bool(plan.get("repair_integrity")),
    )
    initial = merge_catalogs(
        merge_base,
        merge_development,
        merge_production,
        tombstones=tombstones,
        renames=renames,
    )
    known_conflicts = {conflict["id"] for conflict in initial.conflicts}
    unknown_resolutions = sorted(set(resolutions) - known_conflicts)
    if unknown_resolutions:
        raise EndpointError(
            "resolutions reference unknown conflict IDs: " + ", ".join(unknown_resolutions)
        )
    resolved = resolve_merge(initial, resolutions)
    return make_plan(
        base,
        development,
        production,
        resolved,
        tombstones,
        renames,
        plan["development_state"],
        plan["production_state"],
        repair_integrity=bool(plan.get("repair_integrity")),
        repair_layers=bool(plan.get("repair_layers")),
        parent_plan_id=str(plan["plan_id"]),
    )


def plan_summary(plan: Mapping[str, Any]) -> dict[str, Any]:
    summary: dict[str, Any] = {
        "plan_id": plan["plan_id"],
        "sealed": bool(plan.get("sealed")),
        "base_hash": plan["base_hash"],
        "development_hash": plan["development_hash"],
        "production_hash": plan["production_hash"],
        "candidate_hash": plan["candidate_hash"],
        "entry_count": len(plan["candidate"].get("entries", ())),
        "conflict_count": len(plan.get("conflicts", ())),
        "conflicts": [
            {"id": item.get("id"), "tag": item.get("tag"), "reason": item.get("reason")}
            for item in plan.get("conflicts", ())
        ],
        "authorized_deletions": list(plan.get("authorized_deletions", ())),
        "layer_repairs": dict(plan.get("layer_repairs", {})),
        "repair_integrity": bool(plan.get("repair_integrity")),
        "repair_layers": bool(plan.get("repair_layers")),
        "integrity_blockers": list(plan.get("integrity_blockers", ())),
    }
    if plan.get("sealed"):
        summary["development_counts"] = dict(plan["development_delta"]["counts"])
        summary["production_counts"] = dict(plan["production_delta"]["counts"])
    return summary


def proof_core(plan: Mapping[str, Any], apply_result: Mapping[str, Any], verification: Mapping[str, Any]) -> dict[str, Any]:
    return {
        "format": "luminari-help-sync-development-proof",
        "version": 1,
        "plan_id": plan["plan_id"],
        "candidate_hash": plan["candidate_hash"],
        "development_source_hash": plan["development_hash"],
        "run_id": apply_result["run_id"],
        "apply_status": apply_result["status"],
        "verified_catalog_hash": verification["catalog_hash"],
        "verified_file_hash": verification["file_hash"],
        "lookup_checks": list(verification["lookup_checks"]),
    }


def write_development_proof(
    root: Path,
    plan: Mapping[str, Any],
    apply_result: Mapping[str, Any],
    verification: Mapping[str, Any],
) -> dict[str, Any]:
    core = proof_core(plan, apply_result, verification)
    proof = dict(core)
    proof["proof_id"] = sha256_bytes(canonical_json_bytes(core))
    proof["recorded_at"] = utc_now()
    atomic_write_json(proof_path(root, str(plan["plan_id"])), proof)
    return proof


def load_development_proof(root: Path, plan: Mapping[str, Any]) -> dict[str, Any]:
    proof = read_json(proof_path(root, str(plan["plan_id"])))
    if not isinstance(proof, dict):
        raise EndpointError("development proof is not a JSON object")
    core = {key: value for key, value in proof.items() if key not in {"proof_id", "recorded_at"}}
    if proof.get("proof_id") != sha256_bytes(canonical_json_bytes(core)):
        raise EndpointError("development proof checksum mismatch")
    if proof.get("plan_id") != plan["plan_id"] or proof.get("candidate_hash") != plan["candidate_hash"]:
        raise EndpointError("development proof belongs to a different plan or candidate")
    if proof.get("verified_catalog_hash") != plan["candidate_hash"]:
        raise EndpointError("development proof did not verify the candidate hash")
    if not all(check.get("passed") for check in proof.get("lookup_checks", ())):
        raise EndpointError("development proof contains a failed representative lookup")
    return proof


def preview_production(root: Path, plan: Mapping[str, Any]) -> tuple[dict[str, Any], EndpointSnapshot]:
    if not plan.get("sealed"):
        raise EndpointError("production preview requires a sealed, conflict-free plan")
    load_development_proof(root, plan)
    candidate = Catalog.from_dict(plan["candidate"])
    development_verification = verify_endpoint(root, "development", candidate)
    production_snapshot = RemoteEndpoint(root).snapshot()
    clean_repairs = normalize_integrity_repairs({})
    observed_repairs = normalize_integrity_repairs(production_snapshot.integrity_repairs)
    expected_repairs = normalize_integrity_repairs(plan["production_integrity_repairs"])
    if production_snapshot.catalog.content_hash == candidate.content_hash:
        if observed_repairs != clean_repairs:
            raise EndpointError("production has candidate content but a non-clean integrity state")
    elif production_snapshot.catalog.content_hash == plan["production_hash"]:
        if observed_repairs != expected_repairs:
            raise EndpointError("production integrity state drifted after planning")
    else:
        raise EndpointError("production catalog changed after planning; create a new plan")
    token_core = {
        "plan_id": plan["plan_id"],
        "candidate_hash": plan["candidate_hash"],
        "production_catalog_hash": production_snapshot.catalog.content_hash,
        "production_integrity_state_hash": production_snapshot.manifest.get(
            "integrity_state_hash"
        ),
        "production_file_hash": production_snapshot.file_hash,
    }
    preview = {
        **plan_summary(plan),
        "development_proof": "valid",
        "development_verification": development_verification,
        "fresh_production_hash": production_snapshot.catalog.content_hash,
        "fresh_production_file_hash": production_snapshot.file_hash,
        "production_layer_drift": not production_snapshot.file_matches,
        "backup_destination": (
            "REMOTE_PROJECT_PATH/lib/text/help/.help-sync/runs/"
            "<run-id>/production"
        ),
        "authorization_token": sha256_bytes(canonical_json_bytes(token_core)),
    }
    return preview, production_snapshot


def audit_command(root: Path, as_json: bool) -> int:
    require_development(root)
    development = take_snapshot(root, "development")
    production = RemoteEndpoint(root).snapshot()
    result = {
        "mode": "audit",
        "development": development.to_dict(),
        "production": production.to_dict(),
        "excluded_operational_data": [
            "help_search_history",
            "existing help_versions rows",
            "database IDs and timestamps",
            "unrelated tables",
        ],
    }
    if as_json:
        print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    else:
        print(
            json.dumps(
                {
                    "mode": "audit",
                    "development": snapshot_state(development),
                    "production": snapshot_state(production),
                },
                ensure_ascii=False,
                sort_keys=True,
                indent=2,
            )
        )
    return 0


def baseline_init_command(root: Path, args: argparse.Namespace) -> int:
    require_development(root)
    remote = RemoteEndpoint(root)
    if args.source == "development":
        catalog = take_snapshot(root, "development").catalog
    elif args.source == "production":
        catalog = remote.snapshot().catalog
    else:
        catalog = Catalog.from_dict(read_json(Path(args.catalog_file)))
    if args.authorize_hash != catalog.content_hash:
        raise EndpointError(
            f"baseline authorization must equal selected catalog hash {catalog.content_hash}"
        )
    remote_result = remote.write_baseline(catalog, catalog.content_hash)
    local_result = write_common_baseline(root, catalog, catalog.content_hash)
    print(
        json.dumps(
            {
                "status": "initialized",
                "catalog_hash": catalog.content_hash,
                "development": local_result,
                "production": remote_result,
            },
            sort_keys=True,
            indent=2,
        )
    )
    return 0


def plan_command(root: Path, args: argparse.Namespace) -> int:
    plan = create_plan(
        root,
        parse_tombstones(args.tombstones),
        parse_renames(args.renames),
        args.repair_integrity,
        args.repair_layers,
    )
    if args.save:
        saved = save_plan(root, plan)
        result = plan_summary(plan)
        result["stored_at"] = str(saved)
        print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    else:
        print(json.dumps(plan, ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if plan.get("sealed") else 3


def resolve_command(root: Path, args: argparse.Namespace) -> int:
    plan = load_plan(root, args.plan)
    resolutions = read_json(Path(args.resolutions))
    if not isinstance(resolutions, dict):
        raise EndpointError("resolutions file must contain an object keyed by conflict ID")
    resolved = resolve_plan(plan, resolutions)
    if args.save:
        saved = save_plan(root, resolved)
        summary = plan_summary(resolved)
        summary["stored_at"] = str(saved)
        print(json.dumps(summary, ensure_ascii=False, sort_keys=True, indent=2))
    else:
        print(json.dumps(resolved, ensure_ascii=False, sort_keys=True, indent=2))
    return 0 if resolved.get("sealed") else 3


def apply_development_plan(root: Path, plan: Mapping[str, Any]) -> dict[str, Any]:
    """Apply and prove one already-loaded plan on development."""

    result = apply_plan_to_endpoint(root, "development", plan)
    candidate = Catalog.from_dict(plan["candidate"])
    verification = verify_endpoint(root, "development", candidate)
    proof = write_development_proof(root, plan, result, verification)
    return {
        "mode": "apply-dev",
        "plan_id": plan["plan_id"],
        "apply": result,
        "verification": verification,
        "proof": proof,
        "common_baseline_advanced": False,
    }


def apply_development_command(root: Path, args: argparse.Namespace) -> int:
    require_development(root)
    report = apply_development_plan(root, load_plan(root, args.plan))
    print(
        json.dumps(
            report,
            ensure_ascii=False,
            sort_keys=True,
            indent=2,
        )
    )
    return 0


def publish_production_plan(
    root: Path,
    plan: Mapping[str, Any],
    authorization_token: str,
    *,
    tolerate_development_drift: bool = False,
) -> dict[str, Any]:
    """Publish one proven plan and checkpoint it before following new dev work."""

    preview, _ = preview_production(root, plan)
    if authorization_token != preview["authorization_token"]:
        raise EndpointError(
            "authorization token must equal the token from a fresh preview-prod result"
        )
    remote = RemoteEndpoint(root)
    apply_result = remote.apply(plan)
    candidate = Catalog.from_dict(plan["candidate"])
    production_verification = remote.verify(candidate)
    development_verification: dict[str, Any] | None = None
    development_drift: str | None = None
    try:
        development_verification = verify_endpoint(root, "development", candidate)
    except EndpointError as exc:
        development_drift = str(exc)

    # The candidate was already applied and proven on development before the
    # production mutation. Once production independently verifies it, that
    # candidate is the last common lineage point even if development acquired
    # a newer edit during publication. Checkpoint it, then let autonomous sync
    # create a follow-up plan for the newer development state.
    remote_baseline = remote.write_baseline(candidate, str(plan["plan_id"]))
    local_baseline = write_common_baseline(root, candidate, str(plan["plan_id"]))
    report = {
        "mode": "apply-prod",
        "plan_id": plan["plan_id"],
        "apply": apply_result,
        "development_verification": development_verification,
        "development_drift": development_drift,
        "production_verification": production_verification,
        "development_baseline": local_baseline,
        "production_baseline": remote_baseline,
        "common_baseline_advanced": True,
        "converged": development_drift is None,
    }
    if development_drift is not None and not tolerate_development_drift:
        raise ApplyFailure(
            "production applied and verified, but development changed concurrently; "
            "the common checkpoint advanced and a follow-up plan is required",
            report,
        )
    return report


def apply_production_command(root: Path, args: argparse.Namespace) -> int:
    require_development(root)
    plan = load_plan(root, args.plan)
    if args.authorize_plan != plan["plan_id"]:
        raise EndpointError("--authorize-plan must exactly equal the sealed plan ID")
    report = publish_production_plan(root, plan, args.authorize_preview)
    print(
        json.dumps(
            report,
            ensure_ascii=False,
            sort_keys=True,
            indent=2,
        )
    )
    return 0


def validate_autonomous_plan(plan: Mapping[str, Any], stored_at: Path) -> None:
    """Refuse policy decisions that an end-to-end run must not make itself."""

    summary = plan_summary(plan)
    if not plan.get("sealed"):
        raise EndpointError(
            "autonomous sync requires explicit conflict resolution; inspect "
            f"{stored_at}: {json.dumps(summary, sort_keys=True)}"
        )

    deletion_count = sum(
        int(plan[f"{side}_delta"]["counts"]["deletions"])
        for side in ("development", "production")
    )
    if (
        deletion_count
        or plan.get("authorized_deletions")
        or plan.get("renames")
    ):
        raise EndpointError(
            "autonomous sync refuses deletions and renames; inspect the sealed plan "
            f"{stored_at} and use the explicit reviewed workflow"
        )


def retryable_sync_drift(error: Exception) -> bool:
    message = str(error).lower()
    return isinstance(error, StalePlanError) or any(
        fragment in message for fragment in RETRYABLE_SYNC_DRIFT
    )


def synchronize_command(root: Path, args: argparse.Namespace) -> int:
    """Run one explicitly authorized, non-destructive sync through both endpoints."""

    require_development(root)
    if not args.authorize_production:
        raise EndpointError(
            "sync requires --authorize-production for this bounded end-to-end run"
        )
    if args.max_passes < 1 or args.max_passes > MAX_SYNC_PASSES:
        raise EndpointError(
            f"--max-passes must be between 1 and {MAX_SYNC_PASSES}"
        )

    attempts: list[dict[str, Any]] = []
    for pass_number in range(1, args.max_passes + 1):
        plan = create_plan(
            root,
            {"development": [], "production": []},
            (),
            repair_integrity=True,
            repair_layers=args.repair_layers,
        )
        stored_at = save_plan(root, plan)
        validate_autonomous_plan(plan, stored_at)
        candidate = Catalog.from_dict(plan["candidate"])
        barrier = HelpWriteBarrier(root, str(plan["plan_id"]))
        development_report: dict[str, Any] | None = None
        preview: dict[str, Any] | None = None
        publication: dict[str, Any] | None = None

        try:
            development_report = apply_development_plan(root, plan)
            barrier.acquire()
            preview, _ = preview_production(root, plan)
            print(
                json.dumps(
                    {
                        "mode": "sync-preview",
                        "pass": pass_number,
                        "preview": preview,
                    },
                    ensure_ascii=False,
                    sort_keys=True,
                    indent=2,
                ),
                flush=True,
            )
            publication = publish_production_plan(
                root,
                plan,
                str(preview["authorization_token"]),
                tolerate_development_drift=True,
            )
            if not publication["converged"]:
                attempts.append(
                    {
                        "pass": pass_number,
                        "plan": plan_summary(plan),
                        "development": development_report,
                        "production": publication,
                        "status": "follow-up-required",
                    }
                )
                continue

            development_verification = verify_endpoint(
                root, "development", candidate
            )
            remote = RemoteEndpoint(root)
            production_verification = remote.verify(candidate)
            development_baseline = load_common_baseline(root)
            production_baseline = remote.baseline()
            if (
                development_baseline.content_hash != candidate.content_hash
                or production_baseline.content_hash != candidate.content_hash
            ):
                raise EndpointError(
                    "common baseline did not advance to the verified candidate"
                )

            attempts.append(
                {
                    "pass": pass_number,
                    "plan": plan_summary(plan),
                    "development": development_report,
                    "production": publication,
                    "status": "verified",
                }
            )
            print(
                json.dumps(
                    {
                        "mode": "sync",
                        "status": "verified",
                        "passes": attempts,
                        "final_plan_id": plan["plan_id"],
                        "catalog_hash": candidate.content_hash,
                        "entry_count": len(candidate.entries),
                        "relationship_count": candidate.relationship_count,
                        "development_verification": development_verification,
                        "production_verification": production_verification,
                        "common_baseline_advanced": True,
                    },
                    ensure_ascii=False,
                    sort_keys=True,
                    indent=2,
                )
            )
            return 0
        except ApplyFailure:
            raise
        except EndpointError as exc:
            attempts.append(
                {
                    "pass": pass_number,
                    "plan": plan_summary(plan),
                    "status": "retrying-drift",
                    "reason": str(exc),
                }
            )
            if not retryable_sync_drift(exc) or pass_number == args.max_passes:
                raise
        finally:
            if barrier.acquired:
                barrier.release()

    raise EndpointError(
        f"help catalogs did not quiesce after {args.max_passes} sync passes"
    )


def verify_command(root: Path, args: argparse.Namespace) -> int:
    require_development(root)
    plan = load_plan(root, args.plan)
    candidate = Catalog.from_dict(plan["candidate"])
    result: dict[str, Any] = {"mode": "verify", "plan_id": plan["plan_id"]}
    if args.environment in {"development", "both"}:
        result["development"] = verify_endpoint(root, "development", candidate)
    if args.environment in {"production", "both"}:
        result["production"] = RemoteEndpoint(root).verify(candidate)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))
    return 0


def rollback_command(root: Path, args: argparse.Namespace) -> int:
    require_development(root)
    if args.environment == "production":
        if args.authorize_run != args.run_id:
            raise EndpointError("production rollback requires --authorize-run equal to the run ID")
        result = RemoteEndpoint(root).rollback(args.run_id, args.expected_current_hash)
    else:
        result = rollback_endpoint(
            root, "development", args.run_id, args.expected_current_hash
        )
    print(
        json.dumps(
            {
                "mode": "rollback",
                "result": result,
                "common_baseline_advanced": False,
            },
            ensure_ascii=False,
            sort_keys=True,
            indent=2,
        )
    )
    return 0


def read_stdin_json() -> Mapping[str, Any]:
    try:
        value = json.load(sys.stdin)
    except json.JSONDecodeError as exc:
        raise EndpointError(f"endpoint stdin contains invalid JSON: {exc}") from exc
    if not isinstance(value, dict):
        raise EndpointError("endpoint stdin JSON must be an object")
    return value


def endpoint_command(root: Path, args: argparse.Namespace) -> int:
    action = args.endpoint_action
    if action == "snapshot":
        result: Any = take_snapshot(root, "production").to_dict()
    elif action == "baseline-read":
        if environment_name(root) != "production":
            raise EndpointError("remote endpoint does not identify itself as production")
        result = load_common_baseline(root).to_dict()
    elif action == "baseline-write":
        if environment_name(root) != "production":
            raise EndpointError("remote endpoint does not identify itself as production")
        catalog = Catalog.from_dict(read_stdin_json())
        result = write_common_baseline(root, catalog, args.plan_id)
    elif action == "apply":
        result = apply_plan_to_endpoint(root, "production", read_stdin_json())
    elif action == "verify":
        result = verify_endpoint(root, "production", Catalog.from_dict(read_stdin_json()))
    elif action == "rollback":
        result = rollback_endpoint(
            root,
            "production",
            args.run_id,
            args.expected_current_hash,
        )
    else:  # pragma: no cover - argparse prevents this
        raise EndpointError(f"unknown endpoint action {action!r}")
    print(JSON_BEGIN)
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, separators=(",", ":")))
    print(JSON_END)
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Audit, reconcile, prove, publish, verify, or roll back Luminari help content."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    audit = subparsers.add_parser("audit", help="read-only development/production drift audit")
    audit.add_argument("--json", action="store_true", help="include full canonical catalogs")

    baseline = subparsers.add_parser(
        "baseline-init", help="explicitly initialize the common baseline on both endpoints"
    )
    baseline.add_argument(
        "--source", choices=("development", "production", "file"), required=True
    )
    baseline.add_argument("--catalog-file")
    baseline.add_argument("--authorize-hash", required=True)

    plan = subparsers.add_parser("plan", help="read-only three-way plan by default")
    plan.add_argument("--tombstones", help="JSON object naming explicit deletions by side")
    plan.add_argument("--renames", help="JSON list naming explicit tag renames")
    plan.add_argument(
        "--repair-integrity",
        action="store_true",
        help="explicitly include observed orphan/missing-keyword repairs",
    )
    plan.add_argument(
        "--repair-layers",
        action="store_true",
        help="explicitly regenerate drifted help.hlp files from reviewed databases",
    )
    plan.add_argument(
        "--save", action="store_true", help="persist the validated plan under ignored state"
    )

    resolve = subparsers.add_parser("resolve", help="resolve conflicts in an existing plan")
    resolve.add_argument("plan")
    resolve.add_argument("--resolutions", required=True)
    resolve.add_argument("--save", action="store_true")

    show = subparsers.add_parser("show", help="show a compact validated plan preview")
    show.add_argument("plan")

    apply_dev = subparsers.add_parser("apply-dev", help="apply and prove a sealed plan on dev")
    apply_dev.add_argument("plan")

    preview_prod = subparsers.add_parser(
        "preview-prod", help="fresh read-only production preview and authorization token"
    )
    preview_prod.add_argument("plan")

    apply_prod = subparsers.add_parser(
        "apply-prod", help="publish an already-proven plan with exact authorization"
    )
    apply_prod.add_argument("plan")
    apply_prod.add_argument("--authorize-plan", required=True)
    apply_prod.add_argument("--authorize-preview", required=True)

    sync = subparsers.add_parser(
        "sync",
        help="run an explicitly authorized zero-deletion sync through dev and production",
    )
    sync.add_argument(
        "--authorize-production",
        action="store_true",
        help="authorize this bounded run to publish its exact fresh preview",
    )
    sync.add_argument(
        "--max-passes",
        type=int,
        default=DEFAULT_SYNC_PASSES,
        help=(
            "maximum reconciliation passes when supported concurrent edits arrive "
            f"(default: {DEFAULT_SYNC_PASSES}, maximum: {MAX_SYNC_PASSES})"
        ),
    )
    sync.add_argument(
        "--repair-layers",
        action="store_true",
        help=(
            "regenerate drifted help.hlp projections after file-only work has "
            "been reviewed and normalized"
        ),
    )

    verify = subparsers.add_parser("verify", help="verify exact database/file candidate equality")
    verify.add_argument("plan")
    verify.add_argument(
        "--environment",
        choices=("development", "production", "both"),
        default="both",
    )

    rollback = subparsers.add_parser("rollback", help="restore and verify a named run backup")
    rollback.add_argument("run_id")
    rollback.add_argument("--environment", choices=("development", "production"), required=True)
    rollback.add_argument("--expected-current-hash", required=True)
    rollback.add_argument("--authorize-run")

    endpoint = subparsers.add_parser("_endpoint", help=argparse.SUPPRESS)
    endpoint_subparsers = endpoint.add_subparsers(dest="endpoint_action", required=True)
    endpoint_subparsers.add_parser("snapshot")
    endpoint_subparsers.add_parser("baseline-read")
    endpoint_write = endpoint_subparsers.add_parser("baseline-write")
    endpoint_write.add_argument("--plan-id", required=True)
    endpoint_subparsers.add_parser("apply")
    endpoint_subparsers.add_parser("verify")
    endpoint_rollback = endpoint_subparsers.add_parser("rollback")
    endpoint_rollback.add_argument("--run-id", required=True)
    endpoint_rollback.add_argument("--expected-current-hash", required=True)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    root = repository_root()
    if args.command == "audit":
        return audit_command(root, args.json)
    if args.command == "baseline-init":
        if args.source == "file" and not args.catalog_file:
            parser.error("baseline-init --source file requires --catalog-file")
        return baseline_init_command(root, args)
    if args.command == "plan":
        return plan_command(root, args)
    if args.command == "resolve":
        return resolve_command(root, args)
    if args.command == "show":
        print(json.dumps(plan_summary(load_plan(root, args.plan)), sort_keys=True, indent=2))
        return 0
    if args.command == "apply-dev":
        return apply_development_command(root, args)
    if args.command == "preview-prod":
        preview, _ = preview_production(root, load_plan(root, args.plan))
        print(json.dumps(preview, ensure_ascii=False, sort_keys=True, indent=2))
        return 0
    if args.command == "apply-prod":
        return apply_production_command(root, args)
    if args.command == "sync":
        return synchronize_command(root, args)
    if args.command == "verify":
        return verify_command(root, args)
    if args.command == "rollback":
        return rollback_command(root, args)
    if args.command == "_endpoint":
        return endpoint_command(root, args)
    parser.error("unknown command")
    return 2


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except ApplyFailure as exc:
        print(str(exc), file=sys.stderr)
        if exc.details:
            print(json.dumps(exc.details, sort_keys=True, indent=2), file=sys.stderr)
        raise SystemExit(2)
    except (HelpSyncError, OSError, KeyError, ValueError) as exc:
        print(f"help-sync: {exc}", file=sys.stderr)
        raise SystemExit(2)
