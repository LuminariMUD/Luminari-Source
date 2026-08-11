"""Phase 3 no-clobber walking skeleton for a confirmed RoL lineage record."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import shutil
from typing import Any, Iterable

from .config import resolve_config
from .constants import default_repo_root, load_manifest
from .models import TOOL_VERSION
from .reporting import result_payload
from .world import validate_indexed_world


ROL_SKELETON_SCHEMA_VERSION = 1


class RolSkeletonError(ValueError):
  """Raised when the walking skeleton cannot preserve its no-clobber contract."""


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


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolSkeletonError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolSkeletonError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _load_json(path: Path) -> Any:
  try:
    return json.loads(path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolSkeletonError(f"cannot read skeleton input {path}: {error}") from error


def verify_plan_bundle(plan_dir: Path) -> dict[str, Any]:
  manifest = _load_json(plan_dir / "run-manifest.json")
  if manifest.get("phase") != 2:
    raise RolSkeletonError("walking-skeleton input is not a Phase 2 plan")
  for artifact in manifest.get("artifacts", []):
    path = plan_dir / artifact["path"]
    if not path.is_file():
      raise RolSkeletonError(f"plan artifact is missing: {artifact['path']}")
    if _sha256_path(path) != artifact["sha256"]:
      raise RolSkeletonError(f"plan artifact hash mismatch: {artifact['path']}")
  if not manifest.get("acceptance", {}).get("all_records_planned"):
    raise RolSkeletonError("Phase 2 did not plan every active record")
  return manifest


def _read_jsonl(path: Path) -> Iterable[dict[str, Any]]:
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolSkeletonError(f"invalid JSONL at {path}:{line_number}: {error}") from error
        if not isinstance(row, dict):
          raise RolSkeletonError(f"JSONL row at {path}:{line_number} is not an object")
        yield row
  except OSError as error:
    raise RolSkeletonError(f"cannot read skeleton input {path}: {error}") from error


def select_keep_action(plan_dir: Path, basename: str) -> dict[str, Any]:
  """Select one confirmed zone KEEP from the requested prior-lineage package."""

  matches = [
      row
      for row in _read_jsonl(plan_dir / "reconciliation.jsonl")
      if row["basename"] == basename
      and row["source_kind"] == "zon"
      and row["action"] == "KEEP"
      and row.get("selected_target", {}).get("confirmed_seed")
  ]
  if len(matches) != 1:
    raise RolSkeletonError(
        f"expected one confirmed zone KEEP for basename {basename!r}; found {len(matches)}"
    )
  return matches[0]


def tree_manifest(root: Path) -> dict[str, Any]:
  """Hash every regular file in a tree using paths and content only."""

  files: list[dict[str, Any]] = []
  digest = hashlib.sha256()
  for path in sorted(candidate for candidate in root.rglob("*") if candidate.is_file()):
    relative = path.relative_to(root).as_posix()
    sha256 = _sha256_path(path)
    size = path.stat().st_size
    row = {"path": relative, "byte_size": size, "sha256": sha256}
    files.append(row)
    digest.update(relative.encode("utf-8"))
    digest.update(b"\0")
    digest.update(sha256.encode("ascii"))
    digest.update(b"\n")
  return {"files": files, "file_count": len(files), "tree_sha256": digest.hexdigest()}


def apply_keep_action(action: dict[str, Any], world_root: Path) -> dict[str, Any]:
  """Apply a KEEP action by verifying its precondition and performing zero writes."""

  if action.get("action") != "KEEP":
    raise RolSkeletonError("the Phase 3 apply proof accepts only KEEP")
  selected = action.get("selected_target")
  if not isinstance(selected, dict):
    raise RolSkeletonError("KEEP action lacks selected target evidence")
  relative = selected.get("path")
  if not isinstance(relative, str) or Path(relative).is_absolute() or ".." in Path(relative).parts:
    raise RolSkeletonError("KEEP target path is unsafe")
  path = world_root / relative
  if not path.is_file():
    raise RolSkeletonError(f"KEEP target is missing: {relative}")
  before = _sha256_path(path)
  expected = selected.get("source_file_sha256")
  if before != expected:
    raise RolSkeletonError(f"KEEP precondition hash changed for {relative}")
  after = _sha256_path(path)
  return {
      "action": "KEEP",
      "path": relative,
      "expected_sha256": expected,
      "before_sha256": before,
      "after_sha256": after,
      "writes": 0,
      "no_op": before == after == expected,
  }


def _validation_payload(
    world_root: Path,
    zone_vnum: int,
    logical_root: str,
    config: dict[str, Any],
) -> dict[str, Any]:
  repo_root = default_repo_root()
  result = validate_indexed_world(
      world_root,
      repo_root,
      load_manifest(repo_root / "scripts/world/wtool_constants.json"),
      dict(config),
      selected_packages={zone_vnum},
  )
  payload = result_payload(result)
  payload["root"] = logical_root
  payload["command"] = (
      f"python3 scripts/world/wtool.py --world-root <{logical_root}> "
      f"--json validate --zone {zone_vnum}"
  )
  return payload


def _comparable_validation(payload: dict[str, Any]) -> dict[str, Any]:
  comparable = dict(payload)
  comparable.pop("root", None)
  comparable.pop("command", None)
  return comparable


def _serialized_payload(relative: str, payload: Any) -> bytes:
  return _canonical_line(payload) if relative.endswith(".jsonl") else _canonical_json(payload)


def _artifact(path: Path, output_dir: Path) -> dict[str, Any]:
  return {
      "path": path.relative_to(output_dir).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }


def write_skeleton_bundle(
    plan_dir: Path,
    world_root: Path,
    output_dir: Path,
    basename: str = "jotun",
    created_at: str | None = None,
) -> dict[str, Any]:
  """Exercise inventory-to-validate delivery for one confirmed KEEP record."""

  plan_dir = plan_dir.resolve()
  world_root = world_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolSkeletonError(f"skeleton output directory already exists: {output_dir}")
  if not world_root.is_dir():
    raise RolSkeletonError(f"target world root is inaccessible: {world_root}")
  plan_manifest = verify_plan_bundle(plan_dir)
  action = select_keep_action(plan_dir, basename)
  selected = action["selected_target"]
  zone_vnum = action["destination_vnum"]

  target_before = tree_manifest(world_root)
  expected_path = world_root / selected["path"]
  if _sha256_path(expected_path) != selected["source_file_sha256"]:
    raise RolSkeletonError("selected target changed after Phase 1 inventory")

  output_dir.mkdir(parents=True)
  (output_dir / "output/world").mkdir(parents=True)
  staging_root = output_dir / "staging/world"
  shutil.copytree(world_root, staging_root, copy_function=shutil.copy2)
  staged_tree = tree_manifest(staging_root)
  if staged_tree["tree_sha256"] != target_before["tree_sha256"]:
    raise RolSkeletonError("staged world differs from the authoritative target")

  validation_config = resolve_config(world_root, None)
  before_validation = _validation_payload(
      world_root,
      zone_vnum,
      "authoritative-target",
      validation_config,
  )
  staged_validation = _validation_payload(
      staging_root,
      zone_vnum,
      "staged-target",
      validation_config,
  )
  validation_equal = _comparable_validation(before_validation) == _comparable_validation(
      staged_validation
  )
  if not validation_equal:
    raise RolSkeletonError("staged validation differs from the target baseline")

  first_apply = apply_keep_action(action, world_root)
  second_apply = apply_keep_action(action, world_root)
  target_after = tree_manifest(world_root)
  no_clobber = target_before["tree_sha256"] == target_after["tree_sha256"]
  if not no_clobber:
    raise RolSkeletonError("KEEP proof changed builder-owned target data")

  payloads: dict[str, Any] = {
      "reconciliation.jsonl": action,
      "identity-map.jsonl": {
          "source_kind": action["source_kind"],
          "source_vnum": action["source_vnum"],
          "target_type": action["target_type"],
          "destination_vnum": action["destination_vnum"],
          "resolution": "KEEP",
          "evidence": selected["evidence"],
      },
      "change-plan.jsonl": {
          "action": "KEEP",
          "path": selected["path"],
          "writes": 0,
          "precondition_sha256": selected["source_file_sha256"],
      },
      "target-tree-before.json": target_before,
      "staged-tree.json": staged_tree,
      "target-tree-after.json": target_after,
      "validation/target-before.json": before_validation,
      "validation/staged.json": staged_validation,
      "validation/equivalence.json": {
          "equal": validation_equal,
          "target_complete": before_validation["complete"],
          "staged_complete": staged_validation["complete"],
          "new_findings": 0,
          "note": "The target baseline is pre-existing and incomplete; KEEP adds no finding.",
      },
      "validation/apply-first.json": first_apply,
      "validation/apply-second.json": second_apply,
  }
  serialized_once = {
      relative: _serialized_payload(relative, payload)
      for relative, payload in payloads.items()
  }
  serialized_twice = {
      relative: _serialized_payload(relative, payload)
      for relative, payload in payloads.items()
  }
  canonical_repeat_equal = serialized_once == serialized_twice
  if not canonical_repeat_equal:
    raise RolSkeletonError("canonical Phase 3 serialization is not repeatable")

  artifacts: list[dict[str, Any]] = []
  for relative, data in serialized_once.items():
    path = output_dir / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    artifacts.append(_artifact(path, output_dir))

  deterministic_seed = "\n".join(
      item["sha256"] for item in sorted(artifacts, key=lambda item: item["path"])
  ).encode("ascii")
  deterministic_fingerprint = hashlib.sha256(deterministic_seed).hexdigest()
  determinism = {
      "canonical_serialization_repeat_equal": canonical_repeat_equal,
      "staged_tree_equals_target": staged_tree["tree_sha256"]
      == target_before["tree_sha256"],
      "artifact_fingerprint": deterministic_fingerprint,
  }
  determinism_path = output_dir / "validation/determinism.json"
  determinism_path.write_bytes(_canonical_json(determinism))
  artifacts.append(_artifact(determinism_path, output_dir))

  run_id = f"rol-phase3-{deterministic_fingerprint[:16]}"
  manifest = {
      "schema_version": ROL_SKELETON_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 3,
      "plan_run_id": plan_manifest["run_id"],
      "selected_basename": basename,
      "selected_source_record_id": action["source_record_id"],
      "selected_target": selected,
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "inventory_parse_reconcile_map_bundle_stage_validate": True,
          "existing_identity_preserved": True,
          "real_keep_action": True,
          "adds": 0,
          "add_only_if_absent": True,
          "deterministic_output": determinism["canonical_serialization_repeat_equal"],
          "first_apply_no_op": first_apply["no_op"],
          "second_apply_no_op": second_apply["no_op"],
          "target_tree_unchanged": no_clobber,
          "staged_validation_equal": validation_equal,
          "new_findings": 0,
          "builder_owned_files_written": 0,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "selected_basename": basename,
      "source_record_id": action["source_record_id"],
      "target_path": selected["path"],
      "action": "KEEP",
      "writes": 0,
      "first_apply_no_op": first_apply["no_op"],
      "second_apply_no_op": second_apply["no_op"],
      "target_tree_unchanged": no_clobber,
      "staged_validation_equal": validation_equal,
      "target_validation_complete": before_validation["complete"],
      "artifacts": len(artifacts) + 1,
  }


def render_rol_skeleton_human(summary: dict[str, Any]) -> str:
  lines = [
      f"RoL Phase 3 walking skeleton: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Selected package: {summary['selected_basename']}",
      f"Record: {summary['source_record_id']}",
      f"Action: {summary['action']} {summary['target_path']}",
      f"Writes: {summary['writes']}",
      f"First apply no-op: {str(summary['first_apply_no_op']).lower()}",
      f"Second apply no-op: {str(summary['second_apply_no_op']).lower()}",
      f"Target tree unchanged: {str(summary['target_tree_unchanged']).lower()}",
      "Staged validation equals target: "
      f"{str(summary['staged_validation_equal']).lower()}",
      "Target validation complete: "
      f"{str(summary['target_validation_complete']).lower()} (pre-existing baseline)",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
