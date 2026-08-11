"""Phase 4 representative-pilot selection and evidence for RoL."""

from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
from typing import Any, Iterable

from .models import TOOL_VERSION
from .rol_planner import verify_discovery_bundle
from .rol_skeleton import verify_plan_bundle


ROL_PILOT_SCHEMA_VERSION = 1
PILOT_BASENAMES = (
    "swamp_two",
    "hulburg",
    "muspel",
    "theswamp",
    "cemetery",
)
PILOT_ROLES = {
    "swamp_two": "compact conventional reset and container/equipment oracle",
    "hulburg": "confirmed-lineage settlement with shops and quests",
    "muspel": "all five SOC modes, path behavior, custom resets, and special bindings",
    "theswamp": "calendar/removal resets and indoor-zone SOC echo",
    "cemetery": "all-zone SOC echo, uncommon extensions, and special bindings",
}
SOC_MODES = frozenset({"LIST", "PATH", "PERIODIC", "TIMED", "TRIGGER"})
SOC_SPECIAL_CODES = frozenset(range(1000, 1005))
CUSTOM_RESETS = frozenset({"F", "T", "X"})
CONVENTIONAL_RESETS = frozenset({"M", "O", "P", "G", "E", "D"})
UNCOMMON_EXTENSIONS = frozenset({"obj:AFFECT_FLAGS", "wld:R"})
_BINDING_KIND = {"mobile": "mob", "object": "obj", "room": "wld"}


class RolPilotError(ValueError):
  """Raised when Phase 4 pilot evidence does not satisfy its locked coverage."""


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
    raise RolPilotError(f"cannot read pilot input {path}: {error}") from error


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolPilotError(f"invalid JSONL at {path}:{line_number}: {error}") from error
        if not isinstance(row, dict):
          raise RolPilotError(f"JSONL row at {path}:{line_number} is not an object")
        rows.append(row)
  except OSError as error:
    raise RolPilotError(f"cannot read pilot input {path}: {error}") from error
  return rows


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolPilotError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolPilotError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _directive_ids(record: dict[str, Any]) -> set[str]:
  identifiers: set[str] = set()
  for directive in record["directives"]:
    identifier = f"{record['kind']}:{directive['token']}"
    if directive.get("subtype"):
      identifier += f":{directive['subtype']}"
    identifiers.add(identifier)
  return identifiers


def _soc_action_codes(record: dict[str, Any]) -> list[int]:
  return [
      directive["arguments"][0]
      for directive in record["directives"]
      if directive["token"] == "ACTION" and directive.get("arguments")
  ]


def package_metrics(
    basename: str,
    records: Iterable[dict[str, Any]],
    actions: Iterable[dict[str, Any]],
    special_binding_count: int,
) -> dict[str, Any]:
  """Summarize measured pilot coverage for one manually selected package."""

  selected_records = [record for record in records if record["basename"] == basename]
  selected_actions = [action for action in actions if action["basename"] == basename]
  if not selected_records:
    raise RolPilotError(f"selected pilot package is absent: {basename}")
  if len(selected_records) != len(selected_actions):
    raise RolPilotError(f"pilot package action count differs from source: {basename}")

  kinds = Counter(record["kind"] for record in selected_records)
  action_counts = Counter(action["action"] for action in selected_actions)
  modes = Counter(
      record["format_version"]
      for record in selected_records
      if record["kind"] == "soc"
  )
  action_codes = Counter(
      code
      for record in selected_records
      if record["kind"] == "soc"
      for code in _soc_action_codes(record)
  )
  capabilities = set().union(
      *(_directive_ids(record) for record in selected_records)
  )
  reset_tokens = {
      directive["token"]
      for record in selected_records
      if record["kind"] == "zon"
      for directive in record["directives"]
      if directive["token"] not in {"HEADER", "CLIMATE", "S"}
  }
  return {
      "basename": basename,
      "selection_role": PILOT_ROLES[basename],
      "records": len(selected_records),
      "records_by_kind": dict(sorted(kinds.items())),
      "actions": dict(sorted(action_counts.items())),
      "capabilities": sorted(capabilities),
      "zone_reset_tokens": sorted(reset_tokens),
      "soc_modes": dict(sorted(modes.items())),
      "soc_action_codes": dict(sorted(action_codes.items())),
      "soc_special_codes": sorted(set(action_codes) & SOC_SPECIAL_CODES),
      "uncommon_extensions": sorted(capabilities & UNCOMMON_EXTENSIONS),
      "source_special_bindings": special_binding_count,
  }


def pilot_coverage(packages: list[dict[str, Any]]) -> dict[str, Any]:
  """Evaluate the explicit Phase 4 selection against every representative gate."""

  by_name = {package["basename"]: package for package in packages}
  modes = set().union(*(set(package["soc_modes"]) for package in packages))
  codes = set().union(*(set(package["soc_special_codes"]) for package in packages))
  resets = set().union(*(set(package["zone_reset_tokens"]) for package in packages))
  uncommon = set().union(*(set(package["uncommon_extensions"]) for package in packages))
  settlement = by_name.get("hulburg", {})
  conventional = by_name.get("swamp_two", {})
  maximum_bindings = max(
      (package["source_special_bindings"] for package in packages),
      default=0,
  )
  checks = {
      "package_count_between_three_and_five": 3 <= len(packages) <= 5,
      "compact_conventional_reset_package": (
          CONVENTIONAL_RESETS <= set(conventional.get("zone_reset_tokens", []))
          and not CUSTOM_RESETS & set(conventional.get("zone_reset_tokens", []))
      ),
      "settlement_has_shops_and_quests": (
          settlement.get("records_by_kind", {}).get("shp", 0) > 0
          and settlement.get("records_by_kind", {}).get("qst", 0) > 0
      ),
      "all_soc_modes": modes == SOC_MODES,
      "all_soc_special_codes": codes == SOC_SPECIAL_CODES,
      "custom_follow_calendar_removal_resets": CUSTOM_RESETS <= resets,
      "uncommon_extensions": UNCOMMON_EXTENSIONS <= uncommon,
      "significant_special_procedure_dependencies": maximum_bindings >= 25,
      "confirmed_lineage_reuse": any(
          package["actions"].get("KEEP", 0) > 0 for package in packages
      ),
  }
  return {
      "checks": checks,
      "complete": all(checks.values()),
      "covered_soc_modes": sorted(modes),
      "covered_soc_special_codes": sorted(codes),
      "covered_custom_resets": sorted(resets & CUSTOM_RESETS),
      "covered_uncommon_extensions": sorted(uncommon),
      "maximum_package_special_bindings": maximum_bindings,
  }


def _pilot_binding_rows(
    records: list[dict[str, Any]],
    bindings: dict[str, Any],
) -> list[dict[str, Any]]:
  by_identity: dict[tuple[str, int], set[str]] = defaultdict(set)
  for record in records:
    if record["basename"] in PILOT_BASENAMES:
      by_identity[(record["kind"], record["vnum"])].add(record["basename"])

  rows: list[dict[str, Any]] = []
  for binding in bindings["active_binding_candidates"]:
    kind = _BINDING_KIND.get(binding["record_type"], binding["record_type"])
    for basename in sorted(by_identity.get((kind, binding["source_vnum"]), set())):
      row = dict(binding)
      row["basename"] = basename
      rows.append(row)
  return sorted(
      rows,
      key=lambda row: (
          row["basename"],
          row["record_type"],
          row["source_vnum"],
          row["source_path"],
          row["source_line"],
      ),
  )


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


def write_pilot_selection_bundle(
    discovery_dir: Path,
    plan_dir: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write deterministic, non-mutating evidence for the Phase 4 pilot selection."""

  discovery_dir = discovery_dir.resolve()
  plan_dir = plan_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPilotError(f"pilot output directory already exists: {output_dir}")
  discovery_manifest = verify_discovery_bundle(discovery_dir)
  plan_manifest = verify_plan_bundle(plan_dir)
  if plan_manifest.get("discovery_run_id") != discovery_manifest.get("run_id"):
    raise RolPilotError("Phase 1 and Phase 2 pilot inputs do not belong to the same run")

  records = _load_jsonl(discovery_dir / "source-records.jsonl")
  actions = _load_jsonl(plan_dir / "reconciliation.jsonl")
  identities = _load_jsonl(plan_dir / "identity-map.jsonl")
  references = _load_jsonl(discovery_dir / "reference-report.jsonl")
  source_inventory = _load_json(discovery_dir / "source-inventory.json")
  bindings = _load_json(discovery_dir / "bindings.json")
  binding_rows = _pilot_binding_rows(records, bindings)
  binding_counts = Counter(row["basename"] for row in binding_rows)

  selected_records = [
      record for record in records if record["basename"] in PILOT_BASENAMES
  ]
  selected_ids = {record["record_id"] for record in selected_records}
  selected_actions = [
      action for action in actions if action["source_record_id"] in selected_ids
  ]
  selected_identities = [
      identity for identity in identities if identity["source_record_id"] in selected_ids
  ]
  selected_references = [
      reference
      for reference in references
      if reference["source_record_id"] in selected_ids
  ]
  packages = [
      package_metrics(basename, selected_records, selected_actions, binding_counts[basename])
      for basename in PILOT_BASENAMES
  ]
  coverage = pilot_coverage(packages)
  if not coverage["complete"]:
    failed = [name for name, passed in coverage["checks"].items() if not passed]
    raise RolPilotError(f"pilot selection coverage failed: {', '.join(failed)}")

  source_files = [
      file_row
      for file_row in source_inventory["files"]
      if file_row["basename"] in PILOT_BASENAMES and file_row["included"]
  ]
  source_oracle = {
      "selected_files": sorted(
          source_files,
          key=lambda row: (row["basename"], row["kind"], row["path"]),
      ),
      "selected_records": len(selected_records),
      "record_hashes": [
          {
              "record_id": record["record_id"],
              "sha256": record["sha256"],
          }
          for record in sorted(
              selected_records,
              key=lambda row: (row["basename"], row["kind"], row["vnum"], row["line"]),
          )
      ],
  }
  selection = {
      "schema_version": ROL_PILOT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "selection_method": (
          "manual engineering selection from measured Phase 1/2 coverage; no automated ranking"
      ),
      "packages": packages,
      "totals": {
          "packages": len(packages),
          "records": len(selected_records),
          "references": len(selected_references),
          "source_special_bindings": len(binding_rows),
          "actions": dict(
              sorted(Counter(action["action"] for action in selected_actions).items())
          ),
      },
  }

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  json_payloads = {
      "pilot-selection.json": selection,
      "coverage.json": coverage,
      "source-oracle.json": source_oracle,
  }
  for relative, payload in json_payloads.items():
    path = output_dir / relative
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))

  jsonl_payloads = {
      "pilot-records.jsonl": sorted(
          selected_records,
          key=lambda row: (row["basename"], row["kind"], row["vnum"], row["line"]),
      ),
      "pilot-actions.jsonl": sorted(
          selected_actions,
          key=lambda row: (row["basename"], row["source_kind"], row["source_vnum"], row["source_line"]),
      ),
      "pilot-identities.jsonl": sorted(
          selected_identities,
          key=lambda row: (row["source_kind"], row["source_vnum"], row["source_record_id"]),
      ),
      "pilot-references.jsonl": sorted(
          selected_references,
          key=lambda row: (
              row["source_path"],
              row["source_line"],
              row["line"],
              row["role"],
              row["target_type"],
              row["target_vnum"],
          ),
      ),
      "pilot-special-bindings.jsonl": binding_rows,
  }
  for relative, rows in jsonl_payloads.items():
    path = output_dir / relative
    count = _write_jsonl(path, rows)
    artifacts.append(_artifact(path, output_dir, count))

  seed = "\n".join(
      [artifact["sha256"] for artifact in sorted(artifacts, key=lambda row: row["path"])]
      + [discovery_manifest["run_id"], plan_manifest["run_id"]]
  ).encode("ascii")
  run_id = f"rol-phase4-select-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_PILOT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 4,
      "stage": "selection",
      "discovery_run_id": discovery_manifest["run_id"],
      "plan_run_id": plan_manifest["run_id"],
      "selected_basenames": list(PILOT_BASENAMES),
      "artifacts": sorted(artifacts, key=lambda row: row["path"]),
      "acceptance": {
          **coverage["checks"],
          "all_selection_checks": coverage["complete"],
          "selected_records": len(selected_records),
          "selected_records_planned": len(selected_actions),
          "all_selected_records_planned": len(selected_records) == len(selected_actions),
          "live_target_writes": 0,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "packages": len(packages),
      "records": len(selected_records),
      "references": len(selected_references),
      "source_special_bindings": len(binding_rows),
      "actions": selection["totals"]["actions"],
      "coverage_complete": coverage["complete"],
      "live_target_writes": 0,
      "artifacts": len(artifacts) + 1,
  }


def render_rol_pilot_selection_human(summary: dict[str, Any]) -> str:
  actions = ", ".join(
      f"{name}={count}" for name, count in summary["actions"].items()
  )
  lines = [
      f"RoL Phase 4 pilot selection: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Packages: {summary['packages']}",
      f"Records: {summary['records']}",
      f"References: {summary['references']}",
      f"Source special bindings: {summary['source_special_bindings']}",
      f"Actions: {actions}",
      f"Coverage complete: {str(summary['coverage_complete']).lower()}",
      f"Live target writes: {summary['live_target_writes']}",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
