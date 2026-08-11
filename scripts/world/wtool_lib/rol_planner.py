"""Phase 2 deterministic identity and record-action planning for RoL."""

from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
from typing import Any, Iterable

from .models import TOOL_VERSION


ROL_PLAN_SCHEMA_VERSION = 1
_ENTITY_KINDS = frozenset({"wld", "mob", "obj"})
_TARGET_TYPES = {
    "zon": "zone",
    "wld": "room",
    "mob": "mobile",
    "obj": "object",
    "shp": "shop",
    "qst": "hlquest",
    "soc": "soc_behavior",
}


class RolPlanError(ValueError):
  """Raised when Phase 2 cannot make a complete deterministic plan."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _canonical_line(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n").encode(
      "ascii"
  )


def _sha256_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _load_json(path: Path) -> Any:
  try:
    return json.loads(path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolPlanError(f"cannot read planning input {path}: {error}") from error


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolPlanError(f"invalid JSONL at {path}:{line_number}: {error}") from error
        if not isinstance(row, dict):
          raise RolPlanError(f"JSONL row at {path}:{line_number} is not an object")
        rows.append(row)
  except OSError as error:
    raise RolPlanError(f"cannot read planning input {path}: {error}") from error
  return rows


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolPlanError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolPlanError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def verify_discovery_bundle(discovery_dir: Path) -> dict[str, Any]:
  """Verify every Phase 1 artifact before consuming it."""

  manifest_path = discovery_dir / "run-manifest.json"
  manifest = _load_json(manifest_path)
  if manifest.get("phase") != 1:
    raise RolPlanError("planning input is not a Phase 1 discovery bundle")
  for artifact in manifest.get("artifacts", []):
    path = discovery_dir / artifact["path"]
    if not path.is_file():
      raise RolPlanError(f"discovery artifact is missing: {artifact['path']}")
    actual = _sha256_path(path)
    if actual != artifact["sha256"]:
      raise RolPlanError(f"discovery artifact hash mismatch: {artifact['path']}")
  acceptance = manifest.get("acceptance", {})
  required = (
      "source_parse_complete",
      "dependency_closure_complete",
      "all_capabilities_owned",
      "source_aggregates_byte_identical",
      "source_aggregates_semantically_reconciled",
  )
  failed = [name for name in required if not acceptance.get(name)]
  if failed:
    raise RolPlanError(f"discovery acceptance failed: {', '.join(failed)}")
  return manifest


def _strong_formula(candidate: dict[str, Any]) -> bool:
  evidence = set(candidate["evidence"])
  return "legacy_offset_formula" in evidence and "exact_normalized_identity" in evidence


def confirmed_lineage_packages(
    records: list[dict[str, Any]],
    candidates: dict[str, dict[str, Any]],
) -> dict[str, dict[str, Any]]:
  """Confirm package lineage only from a traced zone seed plus broad record evidence."""

  totals: Counter[str] = Counter()
  strong: Counter[str] = Counter()
  formula: Counter[str] = Counter()
  seed_zone: set[str] = set()
  for record in records:
    if record["kind"] not in {"zon", "wld", "mob", "obj"}:
      continue
    basename = record["basename"]
    totals[basename] += 1
    row = candidates[record["record_id"]]
    if any("legacy_offset_formula" in item["evidence"] for item in row["candidates"]):
      formula[basename] += 1
    if any(_strong_formula(item) for item in row["candidates"]):
      strong[basename] += 1
    if record["kind"] == "zon" and any(
        item["confirmed_seed"] for item in row["candidates"]
    ):
      seed_zone.add(basename)

  confirmed: dict[str, dict[str, Any]] = {}
  for basename in sorted(seed_zone):
    total = totals[basename]
    formula_ratio = formula[basename] / total if total else 0.0
    identity_ratio = strong[basename] / total if total else 0.0
    if total >= 20 and formula_ratio >= 0.75 and identity_ratio >= 0.50:
      confirmed[basename] = {
          "basename": basename,
          "core_records": total,
          "legacy_formula_records": formula[basename],
          "exact_identity_formula_records": strong[basename],
          "legacy_formula_ratio": round(formula_ratio, 6),
          "exact_identity_formula_ratio": round(identity_ratio, 6),
          "evidence": [
              "documented_traced_zone_seed",
              "legacy_formula_on_at_least_75_percent_of_core_records",
              "exact_identity_and_formula_on_at_least_50_percent_of_core_records",
          ],
      }
  return confirmed


def _formula_candidate(row: dict[str, Any]) -> dict[str, Any] | None:
  candidates = [
      candidate
      for candidate in row["candidates"]
      if "legacy_offset_formula" in candidate["evidence"]
  ]
  return candidates[0] if candidates else None


def _seed_candidate(row: dict[str, Any]) -> dict[str, Any] | None:
  return next(
      (candidate for candidate in row["candidates"] if candidate["confirmed_seed"]),
      None,
  )


def _zone_allocations(
    records: list[dict[str, Any]], policy: dict[str, Any]
) -> dict[int, int]:
  selected_range = policy["identity"]["new_zone_range"]
  start = selected_range["start"]
  end = selected_range["end"]
  offset = selected_range["offset"]
  source_vnums = sorted({record["vnum"] for record in records if record["kind"] == "zon"})
  allocations = {
      vnum: vnum + offset
      for vnum in source_vnums
      if start <= vnum + offset <= end
  }
  available = iter(value for value in range(start, end + 1) if value not in allocations.values())
  for vnum in source_vnums:
    if vnum not in allocations:
      try:
        allocations[vnum] = next(available)
      except StopIteration as error:
        raise RolPlanError("reserved zone range cannot hold all source zone identities") from error
  return allocations


def _new_destination(
    record: dict[str, Any],
    policy: dict[str, Any],
    zone_allocations: dict[int, int],
) -> int:
  identity = policy["identity"]
  if record["kind"] == "zon":
    return zone_allocations[record["vnum"]]
  return record["vnum"] + identity["new_entity_range"]["offset"]


def _directive_ids(record: dict[str, Any]) -> list[str]:
  identifiers = []
  for directive in record["directives"]:
    identifier = f"{record['kind']}:{directive['token']}"
    if directive.get("subtype"):
      identifier += f":{directive['subtype']}"
    identifiers.append(identifier)
  return sorted(set(identifiers))


def build_record_actions(
    records: list[dict[str, Any]],
    candidate_rows: list[dict[str, Any]],
    policy: dict[str, Any],
    reference_counts: dict[str, Counter[str]],
) -> tuple[list[dict[str, Any]], list[dict[str, Any]], dict[str, dict[str, Any]]]:
  """Resolve every active record without selecting a destructive ambiguous candidate."""

  candidates = {row["source_record_id"]: row for row in candidate_rows}
  if len(candidates) != len(records):
    raise RolPlanError("source record and candidate counts differ")
  confirmed_packages = confirmed_lineage_packages(records, candidates)
  zone_allocations = _zone_allocations(records, policy)
  duplicate_qst: Counter[int] = Counter(
      record["vnum"] for record in records if record["kind"] == "qst"
  )
  duplicate_source: Counter[tuple[str, int]] = Counter(
      (record["kind"], record["vnum"]) for record in records
  )

  preliminary: list[dict[str, Any]] = []
  core_destinations: dict[tuple[str, int], int] = {}
  for record in records:
    row = candidates[record["record_id"]]
    selected = _seed_candidate(row)
    formula = _formula_candidate(row)
    source_excluded = record.get("values", {}).get("source_disposition") == "EXCLUDE"
    if source_excluded:
      action = "EXCLUDE"
      destination = None
      rationale = record["values"]["source_exclusion_reason"]
      selected = None
    elif duplicate_source[(record["kind"], record["vnum"])] > 1:
      action = "MERGE"
      selected = selected or formula
      destination = (
          selected["target_vnum"]
          if selected is not None
          else _new_destination(record, policy, zone_allocations)
      )
      rationale = "duplicate active source definitions require deterministic merge"
    elif selected is not None:
      action = "KEEP"
      destination = selected["target_vnum"]
      rationale = "documented traced lineage seed; preserve authoritative target"
    elif record["basename"] in confirmed_packages and formula is not None:
      action = "KEEP"
      destination = formula["target_vnum"]
      selected = formula
      rationale = "confirmed package lineage; preserve existing target and local edits"
    elif record["kind"] == "qst" and duplicate_qst[record["vnum"]] > 1:
      action = "MERGE"
      destination = None
      rationale = "duplicate active quest-host blocks require deterministic HLQ merge"
      selected = None
    else:
      action = "ADD"
      destination = _new_destination(record, policy, zone_allocations)
      rationale = (
          "no acceptable target candidate exists"
          if not row["candidates"]
          else "candidate lineage is ambiguous; preserve every target candidate untouched"
      )
      selected = None

    item = {
        "source_record_id": record["record_id"],
        "source_kind": record["kind"],
        "source_vnum": record["vnum"],
        "source_path": record["path"],
        "source_line": record["line"],
        "source_sha256": record["sha256"],
        "basename": record["basename"],
        "action": action,
        "target_type": _TARGET_TYPES[record["kind"]],
        "destination_vnum": destination,
        "selected_target": selected,
        "all_target_candidates": row["candidates"],
        "candidate_state": row["candidate_state"],
        "capabilities": _directive_ids(record),
        "dependency_report": "reference-report.jsonl#source_record_id",
        "dependency_count": sum(reference_counts[record["record_id"]].values()),
        "dependency_resolutions": dict(
            sorted(reference_counts[record["record_id"]].items())
        ),
        "rationale": rationale,
        "emission_ready": action in {"KEEP", "EXCLUDE"},
    }
    preliminary.append(item)
    if record["kind"] in _ENTITY_KINDS | {"zon"} and destination is not None:
      core_destinations[(record["kind"], record["vnum"])] = destination

  actions: list[dict[str, Any]] = []
  identities: list[dict[str, Any]] = []
  for item in preliminary:
    kind = item["source_kind"]
    if kind in {"shp", "qst", "soc"} and item["action"] != "EXCLUDE":
      host_destination = core_destinations.get(("mob", item["source_vnum"]))
      if host_destination is None:
        host_destination = item["source_vnum"] + policy["identity"]["new_entity_range"][
            "offset"
        ]
      item["destination_vnum"] = host_destination
      if kind == "qst" and duplicate_qst[item["source_vnum"]] > 1:
        item["action"] = "MERGE"
        item["emission_ready"] = False

    actions.append(item)
    if kind in _ENTITY_KINDS | {"zon"}:
      identities.append(
          {
              "source_kind": kind,
              "source_vnum": item["source_vnum"],
              "source_record_id": item["source_record_id"],
              "target_type": item["target_type"],
              "destination_vnum": item["destination_vnum"],
              "resolution": item["action"],
              "evidence": (
                  item["selected_target"]["evidence"]
                  if item["selected_target"] is not None
                  else [item["rationale"]]
              ),
          }
      )

  _validate_actions(actions, policy)
  return actions, identities, confirmed_packages


def _validate_actions(actions: list[dict[str, Any]], policy: dict[str, Any]) -> None:
  permitted = set(policy["apply"]["permitted_actions"])
  invalid = sorted({item["action"] for item in actions} - permitted)
  if invalid:
    raise RolPlanError(f"planner produced invalid action(s): {', '.join(invalid)}")
  if any(item["action"] != "EXCLUDE" and item["destination_vnum"] is None for item in actions):
    raise RolPlanError("planner produced a non-excluded action without a destination")
  entity_range = policy["identity"]["new_entity_range"]
  zone_range = policy["identity"]["new_zone_range"]
  for item in actions:
    if item["action"] != "ADD":
      continue
    if item["source_kind"] not in _ENTITY_KINDS | {"zon"}:
      continue
    selected_range = zone_range if item["source_kind"] == "zon" else entity_range
    destination = item["destination_vnum"]
    if not selected_range["start"] <= destination <= selected_range["end"]:
      raise RolPlanError(
          f"ADD destination {destination} is outside reserved range for {item['source_record_id']}"
      )
    if any(
        candidate["target_type"] == item["target_type"]
        and candidate["target_vnum"] == destination
        for candidate in item["all_target_candidates"]
    ):
      raise RolPlanError(
          f"ADD destination already exists for {item['source_record_id']}"
      )
  by_destination: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  for item in actions:
    if item["destination_vnum"] is not None:
      by_destination[(item["target_type"], item["destination_vnum"])].append(item)
  for key, colliding in by_destination.items():
    if len(colliding) <= 1:
      continue
    if all(item["action"] == "MERGE" for item in colliding):
      continue
    source_keys = {(item["source_kind"], item["source_vnum"]) for item in colliding}
    if len(source_keys) == 1 and all(item["action"] in {"KEEP", "MERGE"} for item in colliding):
      continue
    raise RolPlanError(
        f"identity collision at {key[0]} {key[1]} across {len(colliding)} source records"
    )


def _reference_counts(path: Path) -> dict[str, Counter[str]]:
  counts: dict[str, Counter[str]] = defaultdict(Counter)
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolPlanError(
              f"invalid reference JSONL at {path}:{line_number}: {error}"
          ) from error
        counts[row["source_record_id"]][row["resolution"]] += 1
  except OSError as error:
    raise RolPlanError(f"cannot read reference report {path}: {error}") from error
  return counts


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _artifact(path: Path, output_dir: Path, records: int | None = None) -> dict[str, Any]:
  item: dict[str, Any] = {
      "path": path.relative_to(output_dir).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }
  if records is not None:
    item["records"] = records
  return item


def write_plan_bundle(
    discovery_dir: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write the complete non-mutating Phase 2 plan from verified Phase 1 inputs."""

  discovery_dir = discovery_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPlanError(f"plan output directory already exists: {output_dir}")
  discovery_manifest = verify_discovery_bundle(discovery_dir)
  records = _load_jsonl(discovery_dir / "source-records.jsonl")
  candidates = _load_jsonl(discovery_dir / "lineage-candidates.jsonl")
  capabilities = _load_jsonl(discovery_dir / "capabilities.jsonl")
  policy = _load_json(discovery_dir / "policies.json")
  reference_counts = _reference_counts(discovery_dir / "reference-report.jsonl")
  actions, identities, confirmed_packages = build_record_actions(
      records, candidates, policy, reference_counts
  )

  action_counts = Counter(item["action"] for item in actions)
  target_type_counts = Counter(item["target_type"] for item in actions)
  blockers = Counter(
      capability["classification"]
      for capability in capabilities
      if capability["classification"] in {"A", "P"}
  )
  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []

  reconciliation_path = output_dir / "reconciliation.jsonl"
  reconciliation_count = _write_jsonl(reconciliation_path, actions)
  artifacts.append(_artifact(reconciliation_path, output_dir, reconciliation_count))

  identity_path = output_dir / "identity-map.jsonl"
  identity_count = _write_jsonl(identity_path, identities)
  artifacts.append(_artifact(identity_path, output_dir, identity_count))

  capabilities_path = output_dir / "capabilities.jsonl"
  capability_count = _write_jsonl(capabilities_path, capabilities)
  artifacts.append(_artifact(capabilities_path, output_dir, capability_count))

  change_plan_path = output_dir / "change-plan.jsonl"
  change_plan_count = _write_jsonl(
      change_plan_path,
      (
          {
              "source_record_id": item["source_record_id"],
              "action": item["action"],
              "target_type": item["target_type"],
              "destination_vnum": item["destination_vnum"],
              "emission_ready": item["emission_ready"],
              "rationale": item["rationale"],
          }
          for item in actions
      ),
  )
  artifacts.append(_artifact(change_plan_path, output_dir, change_plan_count))

  schemas = {
      "schema_version": ROL_PLAN_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "jsonl": {
          "reconciliation.jsonl": "one final action per active physical source record",
          "identity-map.jsonl": "canonical zone/room/mobile/object identity resolution",
          "capabilities.jsonl": "one owned row per observed source construct",
          "change-plan.jsonl": "minimal apply-oriented action projection",
      },
      "determinism": "canonical ASCII JSON; sorted records; LF line endings",
  }
  schemas_path = output_dir / "schemas.json"
  schemas_path.write_bytes(_canonical_json(schemas))
  artifacts.append(_artifact(schemas_path, output_dir))

  summary = {
      "schema_version": ROL_PLAN_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "source_records": len(records),
      "planned_records": len(actions),
      "actions": dict(sorted(action_counts.items())),
      "target_types": dict(sorted(target_type_counts.items())),
      "confirmed_lineage_packages": list(confirmed_packages.values()),
      "capability_blocker_classes": dict(sorted(blockers.items())),
      "emission_ready_records": sum(item["emission_ready"] for item in actions),
      "live_target_writes": 0,
      "complete": len(records) == len(actions),
  }
  summary_path = output_dir / "plan-summary.json"
  summary_path.write_bytes(_canonical_json(summary))
  artifacts.append(_artifact(summary_path, output_dir))

  seed = "\n".join(
      [item["sha256"] for item in sorted(artifacts, key=lambda item: item["path"])]
      + [discovery_manifest["run_id"], policy["policy_version"]]
  ).encode("ascii")
  run_id = f"rol-phase2-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_PLAN_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 2,
      "discovery_run_id": discovery_manifest["run_id"],
      "policy_version": policy["policy_version"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "source_records": len(records),
          "records_planned": len(actions),
          "all_records_planned": len(records) == len(actions),
          "all_actions_final": all(
              item["action"] in policy["apply"]["permitted_actions"] for item in actions
          ),
          "ambiguous_candidates_preserved": all(
              item["action"] != "PATCH"
              for item in actions
              if item["candidate_state"] == "candidates"
              and item["selected_target"] is None
          ),
          "live_target_writes": 0,
          "schemas_present": True,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "records": len(actions),
      "actions": dict(sorted(action_counts.items())),
      "confirmed_lineage_packages": len(confirmed_packages),
      "emission_ready_records": summary["emission_ready_records"],
      "live_target_writes": 0,
      "artifacts": len(artifacts) + 1,
  }


def render_rol_plan_human(summary: dict[str, Any]) -> str:
  actions = ", ".join(
      f"{name}={count}" for name, count in summary["actions"].items()
  )
  lines = [
      f"RoL Phase 2 plan: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Records planned: {summary['records']}",
      f"Actions: {actions}",
      f"Confirmed lineage packages: {summary['confirmed_lineage_packages']}",
      f"Emission-ready records: {summary['emission_ready_records']}",
      f"Live target writes: {summary['live_target_writes']}",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
