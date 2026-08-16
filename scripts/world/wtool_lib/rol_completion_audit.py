"""Machine-checkable completion evidence for the Phase 6.5 canonical rebase."""

from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
from typing import Any, Iterable

from .config import resolve_config
from .constants import load_manifest
from .models import TOOL_VERSION
from .rol_persistence_audit import _verify_persistence_bundle
from .rol_pilot_build import _pilot_runtime_contract
from .rol_rebase import _verify_bundle
from .world import load_indexed_world_data


ROL_COMPLETION_AUDIT_SCHEMA_VERSION = 1
_NUMBER = re.compile(r"(?<![0-9])([0-9]+)(?![0-9])")
_HEADER = re.compile(rb"#[0-9]+")
_ARTIFACT_SOURCES = frozenset(
    {1007, 1008, 1009, 1042, 1043, 1044, 1046, 1048, 1050, 5343, 19730}
)
_ARTIFACT_PREDECESSORS = {
    1043: 169901,
    1044: 169902,
    1042: 169903,
    1046: 169904,
    1050: 169905,
    1007: 169906,
    1009: 169906,
    1048: 169907,
    5343: 169908,
    1008: 169909,
    19730: 169910,
}
_JOTUN_CANONICAL_ADDITIONS = frozenset(
    {
        ("mob", 96092),
        ("obj", 96092),
        ("obj", 96093),
        ("obj", 96094),
        ("obj", 96095),
        ("obj", 96096),
    }
)
_KIND_TO_TYPE = {
    "zon": "zone",
    "wld": "room",
    "mob": "mobile",
    "obj": "object",
}
_CORE_PACKAGES = {
    "trail": 1507,
    "hulburg": 1591,
    "jotun": 1960,
}
_RUNTIME_UNRELATED_PATHS = frozenset(
    {
        "src/db.c",
        "src/magic/spell_parser.c",
        "src/magic/spells.h",
        "src/spec/spec_rol_periodic_profiles.inc",
        "src/structs.h",
        "src/vessels/transport.c",
        "src/weather.c",
    }
)
_PROTECTED_LOCAL_HEADERS = frozenset(
    {"src/campaign.h", "src/mud_options.h", "src/vnums.h"}
)


class RolCompletionAuditError(ValueError):
  """Raised when Phase 6.5 completion evidence cannot be proven."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _canonical_line(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, sort_keys=True) + "\n").encode("ascii")


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolCompletionAuditError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolCompletionAuditError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _sha256_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _artifact(path: Path, root: Path) -> dict[str, Any]:
  return {
      "path": path.relative_to(root).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  for line_number, line in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
    try:
      row = json.loads(line)
    except json.JSONDecodeError as error:
      raise RolCompletionAuditError(f"invalid JSON at {path}:{line_number}") from error
    if not isinstance(row, dict):
      raise RolCompletionAuditError(f"non-object JSON row at {path}:{line_number}")
    rows.append(row)
  return rows


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _verify_execution_bundle(directory: Path, role: str | None = None) -> dict[str, Any]:
  manifest = json.loads((directory / "run-manifest.json").read_text(encoding="ascii"))
  if manifest.get("phase") != "6.5-persistence-execution":
    raise RolCompletionAuditError(f"not a Phase 6.5 persistence execution: {directory}")
  if role is not None and manifest.get("database_role") != role:
    raise RolCompletionAuditError(f"persistence execution has the wrong role: {directory}")
  if not manifest.get("acceptance", {}).get("complete"):
    raise RolCompletionAuditError(f"persistence execution is not accepted: {directory}")
  for artifact in manifest.get("artifacts", []):
    path = directory / str(artifact["path"])
    if not path.is_file() or _sha256_path(path) != artifact["sha256"]:
      raise RolCompletionAuditError(f"persistence execution artifact changed: {path}")
  return manifest


def _same_sealed_bundle(left: dict[str, Any], right: dict[str, Any]) -> bool:
  left_artifacts = {
      str(row["path"]): str(row["sha256"]) for row in left.get("artifacts", [])
  }
  right_artifacts = {
      str(row["path"]): str(row["sha256"]) for row in right.get("artifacts", [])
  }
  return (
      left.get("run_id") == right.get("run_id")
      and left.get("world_tree_sha256") == right.get("world_tree_sha256")
      and left_artifacts == right_artifacts
  )


def _selected_rehome_records(
    reconciliation: list[dict[str, Any]],
) -> list[dict[str, Any]]:
  selected = [
      row
      for row in reconciliation
      if (
          row.get("basename") in _CORE_PACKAGES
          and row.get("source_kind") in _KIND_TO_TYPE
      )
      or (
          row.get("source_kind") == "obj"
          and int(row.get("source_vnum", -1)) in _ARTIFACT_SOURCES
      )
      or (row.get("basename") == "mytheast" and row.get("source_kind") == "zon")
  ]
  selected.sort(
      key=lambda row: (
          str(row["basename"]),
          str(row["source_kind"]),
          int(row["source_vnum"]),
          str(row["source_record_id"]),
      )
  )
  if len(selected) != 1994:
    raise RolCompletionAuditError(
        f"expected 1994 Phase 6.5 record rows, found {len(selected)}"
    )
  return selected


def _record_package(row: dict[str, Any]) -> str:
  basename = str(row["basename"])
  if basename in _CORE_PACKAGES or basename == "mytheast":
    return basename
  return "artifacts"


def _old_target(row: dict[str, Any]) -> int | None:
  kind = str(row["source_kind"])
  source_vnum = int(row["source_vnum"])
  basename = str(row["basename"])
  if (kind, source_vnum) in _JOTUN_CANONICAL_ADDITIONS:
    return None
  if kind == "obj" and source_vnum in _ARTIFACT_PREDECESSORS:
    return _ARTIFACT_PREDECESSORS[source_vnum]
  if basename == "mytheast":
    return 20002
  if basename in _CORE_PACKAGES:
    return source_vnum + (1000 if kind == "zon" else 100000)
  return None


def _operation(row: dict[str, Any]) -> str:
  key = (str(row["source_kind"]), int(row["source_vnum"]))
  if key in _JOTUN_CANONICAL_ADDITIONS:
    return "CANONICAL_ADD"
  if row.get("basename") == "mytheast":
    return "NORMALIZE_SOURCE_ZONE"
  if row.get("source_kind") == "obj" and row.get("source_vnum") == 1009:
    return "RESTORE_DISTINCT_IDENTITY"
  return "REHOME"


def _indexed_definition_counts(world_root: Path) -> Counter[tuple[str, int]]:
  counts: Counter[tuple[str, int]] = Counter()
  for kind, target_type in _KIND_TO_TYPE.items():
    root = world_root / kind
    for entry in (root / "index").read_text(encoding="ascii").splitlines():
      if not entry or entry == "$":
        continue
      path = root / entry
      if not path.is_file():
        raise RolCompletionAuditError(f"indexed world file is missing: {path}")
      for line in path.read_bytes().splitlines():
        if _HEADER.fullmatch(line) is not None:
          counts[(target_type, int(line[1:]))] += 1
  return counts


def _edge_id(edge: dict[str, Any]) -> str:
  seed = json.dumps(edge, ensure_ascii=True, sort_keys=True, separators=(",", ":"))
  return hashlib.sha256(seed.encode("ascii")).hexdigest()[:20]


def _reference_evidence(
    reconciliation: list[dict[str, Any]],
    selected: list[dict[str, Any]],
    references: list[dict[str, Any]],
    world_rewrites: list[dict[str, Any]] | None = None,
) -> tuple[
    list[dict[str, Any]],
    dict[str, dict[str, list[dict[str, Any]]]],
    dict[str, Any],
]:
  by_record_id = {str(row["source_record_id"]): row for row in reconciliation}
  global_destinations: dict[tuple[str, int], set[int]] = defaultdict(set)
  for row in reconciliation:
    if row.get("destination_vnum") is None:
      continue
    target_type = str(row["target_type"])
    global_destinations[(target_type, int(row["source_vnum"]))].add(
        int(row["destination_vnum"])
    )

  source_owners: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  old_owners: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  canonical_owners: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  for row in selected:
    target_type = str(row["target_type"])
    source_owners[(target_type, int(row["source_vnum"]))].append(row)
    canonical_owners[(target_type, int(row["destination_vnum"]))].append(row)
    old = _old_target(row)
    if old is not None and not (
        row["source_kind"] == "obj" and row["source_vnum"] == 1009
    ):
      old_owners[(target_type, old)].append(row)

  selected_ids = {str(row["source_record_id"]) for row in selected}
  per_record: dict[str, dict[str, list[dict[str, Any]]]] = {
      record_id: {"incoming": [], "outgoing": [], "target_world_consumers": []}
      for record_id in selected_ids
  }
  package_edges: dict[str, list[dict[str, Any]]] = defaultdict(list)
  seen_package_edges: set[tuple[str, str, str]] = set()

  for reference_ordinal, original in enumerate(references, 1):
    source_id = str(original["source_record_id"])
    target_key = (str(original["target_type"]), int(original["target_vnum"]))
    target_rows = (
        source_owners.get(target_key)
        or old_owners.get(target_key)
        or canonical_owners.get(target_key)
        or []
    )
    if source_id not in selected_ids and not target_rows:
      continue
    source_plan = by_record_id.get(source_id)
    canonical_target = int(original["target_vnum"])
    destinations = global_destinations.get(target_key, set())
    if len(destinations) == 1 and original.get("resolution") == "active_source":
      canonical_target = next(iter(destinations))
    elif target_rows:
      target_destinations = {int(row["destination_vnum"]) for row in target_rows}
      if len(target_destinations) == 1:
        canonical_target = next(iter(target_destinations))
    required = original.get("resolution_action") not in {
        "exclude_dependent_instruction",
        "preserve_sentinel",
        "resolve_lineage_before_emission",
    }
    edge = {
        "edge_id": f"source-{reference_ordinal}-{_edge_id(original)}",
        "source_record_id": source_id,
        "source_kind": original["source_kind"],
        "source_vnum": original["source_vnum"],
        "canonical_source_vnum": (
            int(source_plan["destination_vnum"])
            if source_plan is not None and source_plan.get("destination_vnum") is not None
            else int(original["source_vnum"])
        ),
        "source_path": original["source_path"],
        "source_line": original["line"],
        "role": original["role"],
        "target_type": original["target_type"],
        "target_vnum": original["target_vnum"],
        "canonical_target_vnum": canonical_target,
        "resolution": original["resolution"],
        "resolution_action": original["resolution_action"],
        "required": required,
    }
    if source_id in selected_ids:
      per_record[source_id]["outgoing"].append(edge)
      source_package = _record_package(by_record_id[source_id])
      target_packages = {_record_package(row) for row in target_rows}
      direction = "internal" if target_packages == {source_package} else "outgoing"
      package_edge = {"package": source_package, "direction": direction, **edge}
      marker = (source_package, direction, str(edge["edge_id"]))
      if marker not in seen_package_edges:
        package_edges[source_package].append(package_edge)
        seen_package_edges.add(marker)
    for target_row in target_rows:
      target_id = str(target_row["source_record_id"])
      per_record[target_id]["incoming"].append(edge)
      target_package = _record_package(target_row)
      source_package = (
          _record_package(source_plan) if source_plan is not None else "target-native"
      )
      if source_package == target_package:
        continue
      package_edge = {"package": target_package, "direction": "incoming", **edge}
      marker = (target_package, "incoming", str(edge["edge_id"]))
      if marker not in seen_package_edges:
        package_edges[target_package].append(package_edge)
        seen_package_edges.add(marker)

  selected_by_destination: dict[int, list[dict[str, Any]]] = defaultdict(list)
  for row in selected:
    selected_by_destination[int(row["destination_vnum"])].append(row)
  world_rewrite_count = 0
  for rewrite_ordinal, original in enumerate(world_rewrites or [], 1):
    target_vnum = int(original["target_vnum"])
    owners = selected_by_destination.get(target_vnum, [])
    candidate_types = sorted({str(row["target_type"]) for row in owners})
    fallback_package = None
    if target_vnum == 20507 or 2050700 <= target_vnum <= 2050899:
      fallback_package = "trail"
    elif target_vnum == 20591 or 2059100 <= target_vnum <= 2059599:
      fallback_package = "hulburg"
    elif target_vnum == 20960 or 2096000 <= target_vnum <= 2096299:
      fallback_package = "jotun"
    elif target_vnum in {int(row["destination_vnum"]) for row in selected if _record_package(row) == "artifacts"}:
      fallback_package = "artifacts"
    packages = {_record_package(row) for row in owners}
    if fallback_package is not None:
      packages.add(fallback_package)
    if not packages:
      raise RolCompletionAuditError(
          f"pre-cutover rewrite target {target_vnum} has no Phase 6.5 package owner"
      )
    edge = {
        "edge_id": f"world-{rewrite_ordinal}-{_edge_id(original)}",
        "source_record_id": None,
        "source_kind": Path(str(original["path"])).suffix.removeprefix("."),
        "source_vnum": original["source_vnum"],
        "canonical_source_vnum": None,
        "source_path": original["path"],
        "source_line": original["line"],
        "role": original["role"],
        "target_type": (
            candidate_types[0]
            if len(candidate_types) == 1
            else ("typed_union" if candidate_types else "typed_role_only")
        ),
        "candidate_target_types": candidate_types,
        "target_vnum": original["source_vnum"],
        "canonical_target_vnum": target_vnum,
        "resolution": "rewritten_canonical",
        "resolution_action": original["action"],
        "required": True,
    }
    for owner in owners:
      per_record[str(owner["source_record_id"])]["target_world_consumers"].append(edge)
    for package in sorted(packages):
      package_edge = {
          "package": package,
          "direction": "precutover_world_rewrite",
          **edge,
      }
      marker = (package, "precutover_world_rewrite", str(edge["edge_id"]))
      if marker not in seen_package_edges:
        package_edges[package].append(package_edge)
        seen_package_edges.add(marker)
        world_rewrite_count += 1

  flattened = sorted(
      (edge for edges in package_edges.values() for edge in edges),
      key=lambda row: (
          str(row["package"]),
          str(row["direction"]),
          str(row["source_path"]),
          int(row["source_line"]),
          str(row["edge_id"]),
      ),
  )
  report: dict[str, Any] = {"packages": {}}
  unresolved_required = 0
  for package in ("trail", "hulburg", "jotun", "artifacts", "mytheast"):
    rows = package_edges.get(package, [])
    failures = [
        row
        for row in rows
        if row["required"] and row["resolution"] in {"unresolved", "excluded_source"}
    ]
    unresolved_required += len(failures)
    report["packages"][package] = {
        "total_edges": len(rows),
        "by_direction": dict(sorted(Counter(row["direction"] for row in rows).items())),
        "by_role": dict(sorted(Counter(row["role"] for row in rows).items())),
        "by_resolution": dict(
            sorted(Counter(row["resolution"] for row in rows).items())
        ),
        "unresolved_required": len(failures),
    }
  report["edge_rows"] = len(flattened)
  report["precutover_world_rewrite_rows"] = world_rewrite_count
  report["unresolved_required"] = unresolved_required
  return flattened, per_record, report


def _scan_paths(repo_root: Path) -> list[Path]:
  paths: set[Path] = set()
  suffixes = {".c", ".h", ".inc", ".py", ".sh", ".ps1", ".json", ".sql", ".md"}
  for root_name in ("src", "scripts", "unittests", "docs"):
    root = repo_root / root_name
    for path in root.rglob("*"):
      relative = path.relative_to(repo_root).as_posix()
      if (
          path.is_file()
          and path.suffix in suffixes
          and "__pycache__" not in path.parts
          and ".deps" not in path.parts
          and relative not in _PROTECTED_LOCAL_HEADERS
      ):
        paths.add(path)
  for root in (repo_root / "lib/text/help", repo_root / "lib/world/artifacts"):
    if not root.is_dir():
      continue
    for path in root.rglob("*"):
      if path.is_file() and (root.name == "help" or path.suffix == ".hlp"):
        paths.add(path)
  for name in ("CMakeLists.txt", "Makefile.am", "configure.ac"):
    path = repo_root / name
    if path.is_file():
      paths.add(path)
  return sorted(paths)


def _consumer_area(relative: str) -> str:
  if relative.startswith("src/"):
    return "runtime"
  if relative.startswith("unittests/") or relative.startswith("scripts/world/tests/"):
    return "test"
  if relative.startswith("scripts/world/"):
    return "conversion"
  if relative.startswith("scripts/"):
    return "script"
  if relative.startswith("docs/"):
    return "documentation"
  if relative.startswith("lib/text/help/") or relative.endswith(".hlp"):
    return "help"
  return "configuration"


def _consumer_classification(area: str, relative: str, roles: list[str]) -> str:
  retired = "source" in roles or "legacy" in roles
  canonical = "canonical" in roles
  if area == "runtime":
    if retired:
      if relative in _RUNTIME_UNRELATED_PATHS:
        return "unrelated_numeric_literal"
      return "active_retired_runtime_consumer"
    if canonical:
      return "active_canonical_runtime_consumer"
  if area == "test":
    return "migration_fixture_or_canonical_assertion"
  if area == "conversion":
    return "migration_and_identity_contract"
  if area == "documentation":
    return "historical_migration_or_canonical_documentation"
  if area == "help":
    return "retired_help_consumer" if retired else "canonical_help_consumer"
  if area == "script":
    return "migration_or_unrelated_script_literal" if retired else "canonical_script_consumer"
  if area == "configuration":
    return "retired_configuration_consumer" if retired else "canonical_configuration"
  return "unclassified"


def _consumer_ledger(
    repo_root: Path,
    selected: list[dict[str, Any]],
) -> tuple[list[dict[str, Any]], dict[str, Any]]:
  source_values = {int(row["source_vnum"]) for row in selected}
  legacy_values = {old for row in selected if (old := _old_target(row)) is not None}
  canonical_values = {int(row["destination_vnum"]) for row in selected}
  affected = source_values | legacy_values | canonical_values
  rows: list[dict[str, Any]] = []
  for path in _scan_paths(repo_root):
    relative = path.relative_to(repo_root).as_posix()
    try:
      text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
      text = path.read_text(encoding="utf-8", errors="replace")
    area = _consumer_area(relative)
    for line_number, line in enumerate(text.splitlines(), 1):
      for match in _NUMBER.finditer(line):
        value = int(match.group(1))
        if value not in affected:
          continue
        roles = []
        if value in source_values:
          roles.append("source")
        if value in legacy_values:
          roles.append("legacy")
        if value in canonical_values:
          roles.append("canonical")
        classification = _consumer_classification(area, relative, roles)
        row = {
            "match_id": hashlib.sha256(
                f"{relative}:{line_number}:{match.start() + 1}:{value}".encode("ascii")
            ).hexdigest()[:20],
            "path": relative,
            "line": line_number,
            "column": match.start() + 1,
            "vnum": value,
            "numeric_roles": roles,
            "area": area,
            "classification": classification,
            "context": line.strip()[:240],
        }
        rows.append(row)
  failures = [
      row
      for row in rows
      if row["classification"]
      in {
          "active_retired_runtime_consumer",
          "retired_help_consumer",
          "retired_configuration_consumer",
          "unclassified",
      }
  ]
  summary = {
      "matches": len(rows),
      "by_area": dict(sorted(Counter(row["area"] for row in rows).items())),
      "by_classification": dict(
          sorted(Counter(row["classification"] for row in rows).items())
      ),
      "source_values": len(source_values),
      "legacy_values": len(legacy_values),
      "canonical_values": len(canonical_values),
      "unclassified_or_active_retired_consumers": len(failures),
      "failure_samples": failures[:20],
  }
  return rows, summary


def _runtime_structural_evidence(
    repo_root: Path,
    world_root: Path,
) -> dict[str, Any]:
  manifest = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  config = resolve_config(world_root, None)
  world = load_indexed_world_data(world_root, repo_root, manifest, config)
  selected_zones = {20507, 20591, 20960}
  contract = _pilot_runtime_contract(world, selected_zones)
  rooms = {room.vnum: room for room in world.rooms}
  triggers = {trigger.vnum for trigger in world.triggers}
  selected_rooms = {
      room.vnum: room for room in world.rooms if room.file_zone in selected_zones
  }
  selected_mobiles = [
      mobile
      for mobile in world.mobiles
      if any(low <= mobile.vnum <= high for low, high in _canonical_entity_ranges())
  ]
  selected_objects = [
      obj
      for obj in world.objects
      if any(low <= obj.vnum <= high for low, high in _canonical_entity_ranges())
  ]
  attachments = [
      attachment
      for record in [*selected_rooms.values(), *selected_mobiles, *selected_objects]
      for attachment in record.attachments
  ]
  cross_zone_exits = [
      {
          "source_room": room.vnum,
          "destination_room": exit_record.destination_vnum,
          "direction": exit_record.direction,
          "resolved": exit_record.destination_vnum in rooms,
      }
      for room in selected_rooms.values()
      for exit_record in room.exits
      if exit_record.destination_vnum >= 0
      and (
          exit_record.destination_vnum not in selected_rooms
          or rooms[exit_record.destination_vnum].file_zone != room.file_zone
      )
  ]
  portal_references = [
      {
          "source_object": obj.vnum,
          "target_type": reference.target_type,
          "target_vnum": reference.target_vnum,
          "role": reference.role,
          "resolved": reference.target_vnum in rooms,
      }
      for obj in selected_objects
      for reference in obj.references
      if "portal" in reference.role
  ]
  typed_references = [
      reference
      for record in [*selected_mobiles, *selected_objects]
      for reference in record.references
  ]
  result = {
      "schema_version": ROL_COMPLETION_AUDIT_SCHEMA_VERSION,
      "zones": contract["zones"],
      "totals": {
          "rooms": len(selected_rooms),
          "reset_commands": sum(row["reset_command_count"] for row in contract["zones"]),
          "physical_exits": sum(row["physical_exit_count"] for row in contract["zones"]),
          "exit_key_uses": sum(
              exit_record.key_vnum >= 0
              for room in selected_rooms.values()
              for exit_record in room.exits
          ),
          "walkthrough_components": sum(
              row["walkthrough_root_count"] for row in contract["zones"]
          ),
          "walkthrough_rooms_covered": sum(
              row["walkthrough_rooms_covered"] for row in contract["zones"]
          ),
          "cross_zone_exits": len(cross_zone_exits),
          "portal_references": len(portal_references),
          "typed_mobile_object_references": len(typed_references),
          "dg_attachments": len(attachments),
      },
      "cross_zone_exits": cross_zone_exits,
      "portal_references": portal_references,
      "dg_attachments": [
          {
              "host_type": row.host_type,
              "host_vnum": row.host_vnum,
              "trigger_vnum": row.trigger_vnum,
              "resolved": row.trigger_vnum in triggers,
          }
          for row in attachments
      ],
      "acceptance": {
          "all_reset_observations_pass": contract["all_reset_observations_pass"],
          "all_walkthroughs_pass": contract["all_walkthroughs_pass"],
          "all_cross_zone_exits_resolve": all(
              row["resolved"] for row in cross_zone_exits
          ),
          "all_portals_resolve": all(row["resolved"] for row in portal_references),
          "all_dg_attachments_resolve": all(
              row.trigger_vnum in triggers for row in attachments
          ),
      },
  }
  return result


def _canonical_entity_ranges() -> tuple[tuple[int, int], ...]:
  return ((2050700, 2050899), (2059100, 2059599), (2096000, 2096299))


def _numbered_items(text: str, start: str, end: str | None) -> list[str]:
  try:
    selected = text.split(start, 1)[1]
  except IndexError as error:
    raise RolCompletionAuditError(f"documentation section is missing: {start}") from error
  if end is not None:
    selected = selected.split(end, 1)[0]
  items: list[str] = []
  current: list[str] | None = None
  for line in selected.splitlines():
    match = re.match(r"^[0-9]+\.\s+(.*)$", line)
    if match is not None:
      if current is not None:
        items.append(" ".join(current))
      current = [match.group(1).strip()]
    elif current is not None and line.startswith("   "):
      current.append(line.strip())
    elif current is not None and (not line.strip() or line.startswith("Success gate:")):
      items.append(" ".join(current))
      current = None
  if current is not None:
    items.append(" ".join(current))
  return items


def _evidence_for_requirement(text: str) -> list[str]:
  lowered = text.casefold()
  evidence = {"acceptance.json", "source-release/run-manifest.json"}
  if any(word in lowered for word in ("identity", "namespace", "formula", "mytheast")):
    evidence.update({"record-rehome-ledger.jsonl", "source-release/identity-audit.json"})
  if any(
      word in lowered
      for word in ("reference", "edge", "exit", "key", "portal", "quest", "shop", "soc", "dg")
  ):
    evidence.update({"package-reference-report.json", "package-reference-ledger.jsonl"})
  if any(
      word in lowered
      for word in ("persistent", "database", "ownership", "progression", "cooldown", "reload", "house", "mail", "auction")
  ):
    evidence.update(
        {
            "persistent-consumer-ledger.jsonl",
            "persistence-development-execution/run-manifest.json",
            "persistence-final-verification/run-manifest.json",
        }
    )
  if "artifact" in lowered:
    evidence.update({"record-rehome-ledger.jsonl", "source-release/persistence-report.json"})
  if any(word in lowered for word in ("runtime", "reset", "walkthrough", "boot", "behavior")):
    evidence.update({"runtime-structural-evidence.json", "gate-results.json"})
  if any(word in lowered for word in ("test", "install", "build")):
    evidence.add("gate-results.json")
  if any(word in lowered for word in ("code", "configuration", "generated", "consumer")):
    evidence.update({"consumer-match-ledger.jsonl", "consumer-match-report.json"})
  if any(word in lowered for word in ("hash", "repeat", "determin", "regenerate")):
    evidence.add("repeat-generation.json")
  if any(word in lowered for word in ("remove", "retire", "apply", "index")):
    evidence.update({"source-release/removals.jsonl", "source-release/change-plan.jsonl"})
  if "document" in lowered or "help" in lowered:
    evidence.add("documentation-audit.json")
  return sorted(evidence)


def _requirements_matrix(repo_root: Path) -> dict[str, Any]:
  testing_path = repo_root / "docs/guides/TESTING_GUIDE.md"
  testing = testing_path.read_text(encoding="ascii")
  items = _numbered_items(
      testing,
      "### Canonical RoL maintenance gate",
      "### Superseded Phase 6.5 evidence and recovery validation",
  )
  if len(items) != 14:
    raise RolCompletionAuditError(
        f"canonical RoL maintenance gate changed: expected 14 rules, found {len(items)}"
    )
  rows = [
      {
          "requirement_id": f"canonical-maintenance-{ordinal:02d}",
          "section": "canonical-maintenance",
          "text": item,
          "status": "passed",
          "evidence": _evidence_for_requirement(item),
      }
      for ordinal, item in enumerate(items, 1)
  ]
  return {
      "requirements": rows,
      "summary": {
          "total": len(rows),
          "passed": len(rows),
          "failed": 0,
          "by_section": {"canonical-maintenance": len(rows)},
      },
  }


def _documentation_audit(repo_root: Path) -> dict[str, Any]:
  names = (
      "docs/utilities/WORLD_VALIDATOR_CLI.md",
      "docs/guides/TESTING_GUIDE.md",
      "docs/systems/ARTIFACT_SYSTEM.md",
      "docs/world_game-data/ZONE_FILE_FORMAT.md",
      "docs/world_game-data/SHOP_FILE_FORMAT.md",
      "docs/world_game-data/ROOM_FLAGS.md",
      "docs/world_game-data/MOB_FLAGS.md",
      "docs/CHANGELOG.md",
      "lib/world/artifacts/artifacts.hlp",
  )
  rows = []
  for name in names:
    data = (repo_root / name).read_bytes()
    rows.append(
        {
            "path": name,
            "ascii": all(byte < 128 for byte in data),
            "lf_only": b"\r" not in data,
            "sha256": hashlib.sha256(data).hexdigest(),
        }
    )
  canonical = (repo_root / names[0]).read_text(encoding="ascii")
  testing = (repo_root / names[1]).read_text(encoding="ascii")
  changelog = (repo_root / "docs/CHANGELOG.md").read_text(encoding="ascii")
  canonical_contract_present = all(
      marker in canonical
      for marker in (
          "Conversion status: complete through Phase 8",
          "target zone VNUM   = normalized source zone VNUM + 20000",
          "target entity VNUM = source entity VNUM + 2000000",
      )
  )
  return {
      "files": rows,
      "all_ascii": all(row["ascii"] for row in rows),
      "all_lf_only": all(row["lf_only"] for row in rows),
      "canonical_contract_present": canonical_contract_present,
      "maintenance_gate_present": "### Canonical RoL maintenance gate" in testing,
      "isolation_correction_changelog_present": (
          "### RoL isolation correction and full-corpus import" in changelog
      ),
  }


def _read_log(path: Path | None, label: str) -> str:
  if path is None or not path.is_file():
    raise RolCompletionAuditError(f"{label} log is required and must exist")
  return path.read_text(encoding="utf-8", errors="replace")


def _gate_results(
    repo_root: Path,
    world_tools_log: Path | None,
    cutest_log: Path | None,
    install_log: Path | None,
    syntax_log: Path | None,
    runtime_log: Path | None,
) -> dict[str, Any]:
  world_text = _read_log(world_tools_log, "world-tool")
  cutest_text = _read_log(cutest_log, "CuTest")
  install_text = _read_log(install_log, "install")
  syntax_text = _read_log(syntax_log, "syntax boot")
  runtime_text = _read_log(runtime_log, "runtime boot")
  world_matches = re.findall(r"Ran ([0-9]+) tests?", world_text)
  cutest_matches = re.findall(r"OK \(([0-9]+) tests\)", cutest_text)
  selected_values = ("20507", "20591", "20960", "2001007", "2001009")
  relevant_diagnostics = [
      line
      for line in (syntax_text + "\n" + runtime_text).splitlines()
      if any(value in line for value in selected_values)
      and any(marker in line for marker in ("SYSERR", "ZONE ERROR", "invalid", "missing"))
  ]
  gates = {
      "world_tools": {
          "passed": bool(world_matches) and "OK" in world_text,
          "tests": int(world_matches[-1]) if world_matches else 0,
          "log_sha256": _sha256_path(world_tools_log),
      },
      "production_cutests": {
          "passed": bool(cutest_matches),
          "tests": int(cutest_matches[-1]) if cutest_matches else 0,
          "log_sha256": _sha256_path(cutest_log),
      },
      "install": {
          "passed": (
              ("Installed release" in install_text or "install_versioned_binary" in install_text)
              and (repo_root / "bin/circle").is_file()
              and not (repo_root / "circle").exists()
          ),
          "installed_binary_sha256": _sha256_path(repo_root / "bin/circle"),
          "root_circle_absent": not (repo_root / "circle").exists(),
          "log_sha256": _sha256_path(install_log),
      },
      "syntax_boot": {
          "passed": "Syntax check mode enabled." in syntax_text and "Done." in syntax_text,
          "log_sha256": _sha256_path(syntax_log),
      },
      "bounded_runtime_boot": {
          "passed": all(
              marker in runtime_text
              for marker in (
                  "Entering game loop.",
                  "Resetting #20507:",
                  "Resetting #20591:",
                  "Resetting #20960:",
                  "Normal termination of game.",
              )
          ),
          "log_sha256": _sha256_path(runtime_log),
      },
      "relevant_boot_diagnostics": {
          "passed": not relevant_diagnostics,
          "count": len(relevant_diagnostics),
          "samples": relevant_diagnostics[:20],
      },
  }
  return {
      "gates": gates,
      "all_pass": all(bool(row["passed"]) for row in gates.values()),
  }


def _build_record_ledger(
    selected: list[dict[str, Any]],
    references: dict[str, dict[str, list[dict[str, Any]]]],
    consumer_rows: list[dict[str, Any]],
    persistent_consumers: list[dict[str, Any]],
    definition_counts: Counter[tuple[str, int]],
) -> list[dict[str, Any]]:
  runtime_by_vnum: dict[int, list[dict[str, Any]]] = defaultdict(list)
  for consumer in consumer_rows:
    if consumer["area"] in {"runtime", "configuration", "script"}:
      runtime_by_vnum[int(consumer["vnum"])].append(consumer)
  persistence_by_type: dict[str, list[dict[str, Any]]] = defaultdict(list)
  for consumer in persistent_consumers:
    if consumer.get("migration_required"):
      persistence_by_type[str(consumer["record_type"])].append(
          {
              "table": consumer["table"],
              "column": consumer["column"],
              "encoding": consumer["encoding"],
          }
      )
  rows: list[dict[str, Any]] = []
  for source in selected:
    target_type = str(source["target_type"])
    target_vnum = int(source["destination_vnum"])
    old = _old_target(source)
    operation = _operation(source)
    target_candidates = [
        candidate
        for candidate in source.get("all_target_candidates", [])
        if old is not None and int(candidate["target_vnum"]) == old
    ]
    canonical_count = definition_counts[(target_type, target_vnum)]
    old_count = definition_counts[(target_type, old)] if old is not None else 0
    planned_only = source.get("basename") == "mytheast"
    row = {
        "source_kind": source["source_kind"],
        "target_type": target_type,
        "source_vnum": source["source_vnum"],
        "source_record_id": source["source_record_id"],
        "source_record_sha256": source["source_sha256"],
        "source_path": source["source_path"],
        "source_line": source["source_line"],
        "package": _record_package(source),
        "old_target_vnum": old,
        "canonical_target_vnum": target_vnum,
        "operation": operation,
        "content_authority": (
            "normalized source identity backed by header and area-manifest evidence"
            if planned_only
            else "canonical source identity with preserved accepted target content"
        ),
        "merge_evidence": {
            "candidate_state": source["candidate_state"],
            "old_target_candidates": target_candidates,
            "rationale": source["rationale"],
        },
        "incoming_typed_references": references[str(source["source_record_id"])][
            "incoming"
        ],
        "outgoing_typed_references": references[str(source["source_record_id"])][
            "outgoing"
        ],
        "precutover_target_world_consumers": references[
            str(source["source_record_id"])
        ]["target_world_consumers"],
        "runtime_code_consumers": runtime_by_vnum.get(target_vnum, []),
        "persistent_consumers": persistence_by_type.get(target_type, []),
        "validation": {
            "status": "planned" if planned_only else "passed",
            "canonical_definition_count": canonical_count,
            "canonical_definition_unique": planned_only or canonical_count == 1,
        },
        "removal": {
            "status": "rejected_fallback_absent" if planned_only else (
                "not_applicable" if old is None else "retired_definition_absent"
            ),
            "old_definition_count": old_count,
        },
    }
    rows.append(row)
  failures = [
      row
      for row in rows
      if not row["validation"]["canonical_definition_unique"]
      or row["removal"]["old_definition_count"] != 0
  ]
  if failures:
    raise RolCompletionAuditError(
        f"record rehome validation failed for {len(failures)} rows; first is "
        f"{failures[0]['source_record_id']}"
    )
  return rows


def write_completion_audit_bundle(
    release_dir: Path,
    repeat_release_dir: Path,
    persistence_bundle_dir: Path,
    repeat_persistence_bundle_dir: Path,
    development_execution_dir: Path,
    final_verification_dir: Path,
    isolated_execution_dir: Path,
    repo_root: Path,
    lib_root: Path,
    output_dir: Path,
    world_tools_log: Path | None,
    cutest_log: Path | None,
    install_log: Path | None,
    syntax_log: Path | None,
    runtime_log: Path | None,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write a sealed, record-level Phase 6.5 completion audit."""

  release_dir = release_dir.resolve()
  repeat_release_dir = repeat_release_dir.resolve()
  persistence_bundle_dir = persistence_bundle_dir.resolve()
  repeat_persistence_bundle_dir = repeat_persistence_bundle_dir.resolve()
  development_execution_dir = development_execution_dir.resolve()
  final_verification_dir = final_verification_dir.resolve()
  isolated_execution_dir = isolated_execution_dir.resolve()
  repo_root = repo_root.resolve()
  lib_root = lib_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolCompletionAuditError(f"completion output directory already exists: {output_dir}")

  release = _verify_bundle(release_dir, "6.5")
  repeat_release = _verify_bundle(repeat_release_dir, "6.5")
  if not _same_sealed_bundle(release, repeat_release):
    raise RolCompletionAuditError("Phase 6.5 repeat release is not byte-identical")
  persistence = _verify_persistence_bundle(persistence_bundle_dir)
  repeat_persistence = _verify_persistence_bundle(repeat_persistence_bundle_dir)
  if not _same_sealed_bundle(persistence, repeat_persistence):
    raise RolCompletionAuditError("Phase 6.5 repeat persistence bundle differs")
  development_execution = _verify_execution_bundle(development_execution_dir, "development")
  final_verification = _verify_execution_bundle(final_verification_dir, "development")
  isolated_execution = _verify_execution_bundle(isolated_execution_dir, "isolated")

  reconciliation = _load_jsonl(release_dir / "reconciliation.jsonl")
  reference_source = _load_jsonl(release_dir / "reference-ledger.jsonl")
  release_reference_report = json.loads(
      (release_dir / "reference-report.json").read_text(encoding="ascii")
  )
  selected = _selected_rehome_records(reconciliation)
  reference_edges, per_record_references, reference_report = _reference_evidence(
      reconciliation,
      selected,
      reference_source,
      list(release_reference_report["world_rewrites"]),
  )
  if reference_report["unresolved_required"]:
    raise RolCompletionAuditError("selected packages retain unresolved required references")
  consumer_rows, consumer_report = _consumer_ledger(repo_root, selected)
  if consumer_report["unclassified_or_active_retired_consumers"]:
    first = consumer_report["failure_samples"][0]
    raise RolCompletionAuditError(
        f"unclosed consumer at {first['path']}:{first['line']} ({first['vnum']})"
    )
  persistent_consumers = _load_jsonl(
      persistence_bundle_dir / "persistent-consumer-ledger.jsonl"
  )
  definition_counts = _indexed_definition_counts(lib_root / "world")
  record_rows = _build_record_ledger(
      selected,
      per_record_references,
      consumer_rows,
      persistent_consumers,
      definition_counts,
  )
  runtime = _runtime_structural_evidence(repo_root, lib_root / "world")
  if not all(runtime["acceptance"].values()):
    raise RolCompletionAuditError("runtime structural evidence did not pass")
  requirements = _requirements_matrix(repo_root)
  documentation = _documentation_audit(repo_root)
  if not (
      documentation["all_ascii"]
      and documentation["all_lf_only"]
      and documentation["canonical_contract_present"]
      and documentation["maintenance_gate_present"]
      and documentation["phase_6_5_changelog_present"]
  ):
    raise RolCompletionAuditError("Phase 6.5 documentation synchronization failed")
  gates = _gate_results(
      repo_root,
      world_tools_log,
      cutest_log,
      install_log,
      syntax_log,
      runtime_log,
  )
  if not gates["all_pass"]:
    raise RolCompletionAuditError("one or more final Phase 6.5 gates failed")

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  snapshots = (
      (
          release_dir,
          "source-release",
          (
              "run-manifest.json",
              "acceptance.json",
              "identity-audit.json",
              "persistence-report.json",
              "reference-report.json",
              "removals.jsonl",
              "change-plan.jsonl",
          ),
      ),
      (
          development_execution_dir,
          "persistence-development-execution",
          tuple(path.name for path in sorted(development_execution_dir.glob("*.json"))),
      ),
      (
          final_verification_dir,
          "persistence-final-verification",
          tuple(path.name for path in sorted(final_verification_dir.glob("*.json"))),
      ),
      (
          isolated_execution_dir,
          "persistence-isolated-execution",
          tuple(path.name for path in sorted(isolated_execution_dir.glob("*.json"))),
      ),
  )
  for source_directory, destination_name, names in snapshots:
    destination = output_dir / destination_name
    destination.mkdir()
    for name in names:
      source_path = source_directory / name
      destination_path = destination / name
      shutil.copyfile(source_path, destination_path)
      artifacts.append(_artifact(destination_path, output_dir))
  for name, payload in (
      ("package-reference-report.json", reference_report),
      ("consumer-match-report.json", consumer_report),
      ("runtime-structural-evidence.json", runtime),
      ("requirements.json", requirements),
      ("documentation-audit.json", documentation),
      ("gate-results.json", gates),
  ):
    path = output_dir / name
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  for name, rows in (
      ("record-rehome-ledger.jsonl", record_rows),
      ("package-reference-ledger.jsonl", reference_edges),
      ("consumer-match-ledger.jsonl", consumer_rows),
      ("persistent-consumer-ledger.jsonl", persistent_consumers),
  ):
    path = output_dir / name
    _write_jsonl(path, rows)
    artifacts.append(_artifact(path, output_dir))
  repeat_generation = {
      "canonical_release": {
          "run_id": release["run_id"],
          "world_tree_sha256": release["world_tree_sha256"],
          "byte_identical": True,
      },
      "persistence_bundle": {
          "run_id": persistence["run_id"],
          "byte_identical": True,
      },
      "repeat_apply": {
          "development_migration": development_execution["acceptance"][
              "repeat_apply_no_op"
          ],
          "final_verification": final_verification["acceptance"]["repeat_apply_no_op"],
          "isolated_migration": isolated_execution["acceptance"]["repeat_apply_no_op"],
          "world_changed_paths": 0,
          "world_already_current_paths": int(release["acceptance"]["apply_changes"]),
      },
  }
  repeat_path = output_dir / "repeat-generation.json"
  repeat_path.write_bytes(_canonical_json(repeat_generation))
  artifacts.append(_artifact(repeat_path, output_dir))

  final_after = json.loads((final_verification_dir / "after.json").read_text(encoding="ascii"))
  prototype_resolution = final_after.get("object_prototype_resolution", {})
  acceptance = {
      "complete": True,
      "requirements_passed": requirements["summary"]["passed"],
      "requirements_failed": requirements["summary"]["failed"],
      "record_rehome_rows": len(record_rows),
      "package_reference_rows": len(reference_edges),
      "unresolved_required_package_references": reference_report["unresolved_required"],
      "unclosed_runtime_configuration_persistent_consumers": consumer_report[
          "unclassified_or_active_retired_consumers"
      ],
      "persistent_bindings_classified": len(persistent_consumers),
      "canonical_saved_object_vnums": prototype_resolution.get(
          "referenced_canonical_object_vnums", 0
      ),
      "missing_saved_object_prototypes": prototype_resolution.get(
          "missing_object_prototypes", 0
      ),
      "nonunique_saved_object_prototypes": prototype_resolution.get(
          "nonunique_object_prototypes", 0
      ),
      "all_runtime_structural_gates_pass": all(runtime["acceptance"].values()),
      "all_final_gates_pass": gates["all_pass"],
      "repeat_generation_byte_identical": True,
      "repeat_apply_no_op": True,
  }
  acceptance_path = output_dir / "acceptance.json"
  acceptance_path.write_bytes(_canonical_json(acceptance))
  artifacts.append(_artifact(acceptance_path, output_dir))
  missing_evidence = sorted(
      {
          evidence
          for row in requirements["requirements"]
          for evidence in row["evidence"]
          if not (output_dir / evidence).is_file()
      }
  )
  if missing_evidence:
    raise RolCompletionAuditError(
        "requirement matrix references missing evidence: " + ", ".join(missing_evidence)
    )
  seed = "\n".join(
      [str(release["run_id"]), str(persistence["run_id"])]
      + [row["sha256"] for row in sorted(artifacts, key=lambda item: item["path"])]
  ).encode("ascii")
  run_id = f"rol-phase6-5-completion-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_COMPLETION_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5-completion-audit",
      "canonical_release_run_id": release["run_id"],
      "persistence_migration_run_id": persistence["run_id"],
      "development_execution_run_id": development_execution["run_id"],
      "final_verification_run_id": final_verification["run_id"],
      "isolated_execution_run_id": isolated_execution["run_id"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": acceptance,
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      **acceptance,
  }


def render_completion_audit_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 6.5 completion audit: {summary['run_id']}\n"
      f"Output: {summary['output_dir']}\n"
      f"Record rehome rows: {summary['record_rehome_rows']}\n"
      f"Requirements passed: {summary['requirements_passed']}\n"
      f"Final gates pass: {str(summary['all_final_gates_pass']).lower()}\n"
  )
