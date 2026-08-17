"""Final integration, application, and completion evidence for the RoL corpus."""

from __future__ import annotations

from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import shutil
import tempfile
from typing import Any, Iterable

from .config import resolve_config
from .constants import default_repo_root, load_manifest
from .flags import decode_tokens
from .models import TOOL_VERSION
from .reporting import result_payload
from .rol_phase7 import _runtime_contract, _validation_delta
from .rol_persistence_check import audit_development_persistence
from .rol_pilot_build import (
    _artifact,
    _canonical_json,
    _canonical_line,
    _created_at,
    _load_json,
    _load_jsonl,
    _sha256_path,
    _verify_bundle,
)
from .rol_skeleton import tree_manifest
from .world import load_indexed_world_data, validate_indexed_world


ROL_PHASE8_SCHEMA_VERSION = 1
_LOG_NAMES = ("world-tools", "cutest", "install", "syntax", "runtime")
_ACTION_RECORD_TYPES = {
    "zon": "zone",
    "wld": "room",
    "mob": "mobile",
    "obj": "object",
    "shp": "shop",
    "qst": "hlquest",
}
_CODE_EVIDENCE_PATHS = (
    "scripts/world/wtool_lib/cli.py",
    "scripts/world/wtool_lib/rol_phase7.py",
    "scripts/world/wtool_lib/rol_phase8.py",
    "scripts/world/wtool_lib/rol_persistence.py",
    "scripts/world/wtool_lib/rol_persistence_check.py",
    "scripts/world/wtool_lib/rol_transform.py",
    "src/db.c",
    "src/spec/spec_assign_mobiles.c",
    "src/spec/spec_assign_objects.c",
    "src/spec/spec_registry.c",
    "src/spec/spec_rol_conversion.c",
    "src/spec/spec_rol_conversion.h",
    "unittests/CuTest/test_spec_mechanics.c",
)
_DOCUMENTATION_PATHS = (
    "docs/utilities/WORLD_VALIDATOR_CLI.md",
    "docs/guides/TESTING_GUIDE.md",
    "docs/CHANGELOG.md",
    "docs/systems/ARTIFACT_SYSTEM.md",
    "docs/world_game-data/ZONE_FILE_FORMAT.md",
    "docs/world_game-data/SHOP_FILE_FORMAT.md",
    "docs/world_game-data/ROOM_FLAGS.md",
    "docs/world_game-data/MOB_FLAGS.md",
    "lib/text/help/realms_of_luminari.hlp",
    "lib/text/help/index",
)


class RolPhase8Error(ValueError):
  """Raised when Phase 8 cannot prove a safe final integration."""


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  path.parent.mkdir(parents=True, exist_ok=True)
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _development_environment(lib_root: Path) -> None:
  values: dict[str, str] = {}
  env_path = lib_root / ".env"
  for raw in env_path.read_text(encoding="utf-8").splitlines():
    line = raw.strip()
    if not line or line.startswith("#") or "=" not in line:
      continue
    key, value = line.split("=", 1)
    values[key.strip()] = value.strip().strip("'\"")
  if values.get("APP_ENV", "").casefold() not in {
      "dev",
      "development",
      "local",
      "test",
      "testing",
  }:
    raise RolPhase8Error("lib/.env does not explicitly identify a development environment")


def _phase7_paths(phase7_dir: Path) -> list[str]:
  change_plan = _load_json(phase7_dir / "change-plan.json")
  paths = [str(path) for path in change_plan.get("touched_paths", [])]
  if not paths or len(paths) != len(set(paths)):
    raise RolPhase8Error("Phase 7 touched-path plan is empty or duplicated")
  for relative in paths:
    source = (phase7_dir / "output/world" / relative).resolve()
    try:
      source.relative_to((phase7_dir / "output/world").resolve())
    except ValueError as error:
      raise RolPhase8Error("Phase 7 output path escapes its world root") from error
    if not source.is_file():
      raise RolPhase8Error(f"Phase 7 output is missing: {relative}")
  return sorted(paths)


def _assemble_candidate(phase7_dir: Path, baseline_world: Path, destination: Path) -> None:
  shutil.copytree(baseline_world, destination, copy_function=shutil.copy2)
  for relative in _phase7_paths(phase7_dir):
    source = phase7_dir / "output/world" / relative
    target = destination / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def _repeat_evidence(primary_dir: Path, repeat_dir: Path) -> dict[str, Any]:
  primary = _verify_bundle(primary_dir, 7, "cumulative-milestone")
  repeat = _verify_bundle(repeat_dir, 7, "cumulative-milestone")
  primary_stable = {key: value for key, value in primary.items() if key != "creation_time"}
  repeat_stable = {key: value for key, value in repeat.items() if key != "creation_time"}
  manifest_content_identical = primary_stable == repeat_stable
  primary_output = tree_manifest(primary_dir / "output/world")
  repeat_output = tree_manifest(repeat_dir / "output/world")
  return {
      "primary_run_id": primary["run_id"],
      "repeat_run_id": repeat["run_id"],
      "manifest_content_identical": manifest_content_identical,
      "output_tree_byte_identical": primary_output["tree_sha256"]
      == repeat_output["tree_sha256"],
      "primary_output_tree_sha256": primary_output["tree_sha256"],
      "repeat_output_tree_sha256": repeat_output["tree_sha256"],
      "pass": primary["run_id"] == repeat["run_id"]
      and manifest_content_identical
      and primary_output["tree_sha256"] == repeat_output["tree_sha256"],
  }


def _line_format_audit(root: Path, paths: Iterable[str]) -> dict[str, Any]:
  rows: list[dict[str, Any]] = []
  for relative in sorted(paths):
    data = (root / relative).read_bytes()
    rows.append(
        {
            "path": relative,
            "ascii": all(byte < 128 for byte in data),
            "lf_only": b"\r" not in data,
            "sha256": hashlib.sha256(data).hexdigest(),
        }
    )
  return {
      "files": rows,
      "all_ascii": all(row["ascii"] for row in rows),
      "all_lf_only": all(row["lf_only"] for row in rows),
      "pass": all(row["ascii"] and row["lf_only"] for row in rows),
  }


def _mechanics_markers(world) -> list[dict[str, Any]]:
  markers: list[dict[str, Any]] = []

  def add(owner_type: str, vnum: int, mechanic: str) -> None:
    markers.append({"owner_type": owner_type, "vnum": vnum, "mechanic": mechanic})

  for zone in world.zones:
    if 18 in decode_tokens(zone.flags).bits:
      add("zone", zone.vnum, "rol_reset_compat")
  for room in world.rooms:
    room_bits = decode_tokens(room.flags).bits
    for bit, mechanic in (
        (44, "rol_jail"),
        (46, "rol_home_reset"),
        (47, "rol_astral"),
    ):
      if bit in room_bits:
        add("room", room.vnum, mechanic)
    if room.rol_exit_traps:
      add("room", room.vnum, "rol_exit_trap")
    if room.spec_proc is not None and room.spec_proc.startswith("rol_"):
      add("room", room.vnum, room.spec_proc)
  for mobile in world.mobiles:
    action_bits = decode_tokens(mobile.action_flags).bits
    for bit in sorted(action_bits & set(range(105, 126))):
      add("mobile", mobile.vnum, f"mob_rol_flag_{bit}")
    if 4 in decode_tokens(mobile.affect2_flags).bits:
      add("mobile", mobile.vnum, "rol_docile")
    if mobile.spec_proc is not None and mobile.spec_proc.startswith("rol_"):
      add("mobile", mobile.vnum, mobile.spec_proc)
  for obj in world.objects:
    extra_bits = decode_tokens(obj.extra_flags).bits
    if 113 in extra_bits:
      add("object", obj.vnum, "rol_object_trap_compatibility")
    for bit in sorted(extra_bits & set(range(116, 125))):
      add("object", obj.vnum, f"item_rol_flag_{bit}")
    if obj.spec_proc is not None and obj.spec_proc.startswith("rol_"):
      add("object", obj.vnum, obj.spec_proc)
  rol_trade_mask = sum(1 << bit for bit in range(26, 30))
  for shop in world.shops:
    if shop.shop_flags & ((1 << 6) | (1 << 7)):
      add("shop", shop.vnum, "rol_magic_policy")
    if shop.customer_restrictions & rol_trade_mask:
      add("shop", shop.vnum, "rol_customer_restriction")
    if shop.rol_cheat_restrictions:
      add("shop", shop.vnum, "rol_cheat_policy")
  return markers


def _mechanics_isolation_audit(repo_root: Path, baseline_world, candidate_world) -> dict[str, Any]:
  baseline_markers = _mechanics_markers(baseline_world)
  candidate_markers = _mechanics_markers(candidate_world)
  low_baseline_markers = [
      row
      for row in baseline_markers
      if int(row["vnum"])
      < (20_000 if row["owner_type"] == "zone" else 2_000_000)
  ]
  low_candidate_markers = [
      row
      for row in candidate_markers
      if int(row["vnum"])
      < (20_000 if row["owner_type"] == "zone" else 2_000_000)
  ]
  mechanics_paths = sorted((repo_root / "src/spec").glob("spec_rol_*.[ch]"))
  mechanics_paths.extend(sorted((repo_root / "src/vessels").glob("vessels_rol.[ch]")))
  static_literals = 0
  noncanonical_literals: list[dict[str, Any]] = []
  literal_pattern = re.compile(r"(?<![A-Za-z0-9_])([1-9][0-9]{6})(?![A-Za-z0-9_])")
  for path in mechanics_paths:
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8").splitlines(), start=1
    ):
      for token in literal_pattern.findall(line):
        value = int(token)
        static_literals += 1
        if not 2_000_000 <= value <= 2_999_999:
          noncanonical_literals.append(
              {
                  "path": path.relative_to(repo_root).as_posix(),
                  "line": line_number,
                  "value": value,
              }
          )
  by_owner = Counter(str(row["owner_type"]) for row in candidate_markers)
  by_mechanic = Counter(str(row["mechanic"]) for row in candidate_markers)
  return {
      "baseline_rol_markers": len(baseline_markers),
      "baseline_marker_samples": baseline_markers[:40],
      "low_namespace_baseline_markers": len(low_baseline_markers),
      "low_namespace_baseline_marker_samples": low_baseline_markers[:40],
      "candidate_rol_markers": len(candidate_markers),
      "candidate_markers_by_owner": dict(sorted(by_owner.items())),
      "candidate_markers_by_mechanic": dict(sorted(by_mechanic.items())),
      "low_namespace_candidate_markers": len(low_candidate_markers),
      "low_namespace_marker_samples": low_candidate_markers[:40],
      "mechanics_source_files": len(mechanics_paths),
      "mechanics_source_file_sha256": {
          path.relative_to(repo_root).as_posix(): _sha256_path(path)
          for path in mechanics_paths
      },
      "seven_digit_identity_literals": static_literals,
      "noncanonical_seven_digit_identity_literals": noncanonical_literals,
      "pass": not low_baseline_markers
      and bool(candidate_markers)
      and not low_candidate_markers
      and bool(static_literals)
      and not noncanonical_literals,
  }


def _action_audit(
    action_rows: list[dict[str, Any]], validation: dict[str, Any], baseline_world=None
) -> dict[str, Any]:
  targets = {
      (_ACTION_RECORD_TYPES[kind], int(row["destination_vnum"]))
      for row in action_rows
      if (kind := str(row["source_kind"])) in _ACTION_RECORD_TYPES
      and row.get("destination_vnum") is not None
      and row.get("final_action") != "EXCLUDE"
  }
  blocking = [
      row
      for row in validation["findings"]
      if row["severity"] == "error"
      and not row.get("suppressed", False)
      and (row.get("record_type"), row.get("vnum")) in targets
  ]
  actions = Counter(str(row["final_action"]) for row in action_rows)
  kinds = Counter(str(row["source_kind"]) for row in action_rows)
  missing_destinations = [
      row["source_record_id"]
      for row in action_rows
      if row["final_action"] != "EXCLUDE" and row.get("destination_vnum") is None
  ]
  noncanonical = []
  for row in action_rows:
    destination = row.get("destination_vnum")
    if row["final_action"] == "EXCLUDE" or destination is None:
      continue
    minimum = 20_000 if row["source_kind"] == "zon" else 2_000_000
    if int(destination) < minimum:
      noncanonical.append(str(row["source_record_id"]))
  merge_rows = [row for row in action_rows if row["final_action"] == "MERGE"]
  existing_target_merge_destinations: list[str] = []
  if baseline_world is not None:
    baseline_by_kind = {
        "zon": {record.vnum for record in baseline_world.zones},
        "wld": {record.vnum for record in baseline_world.rooms},
        "mob": {record.vnum for record in baseline_world.mobiles},
        "obj": {record.vnum for record in baseline_world.objects},
        "shp": {record.vnum for record in baseline_world.shops},
        "qst": {record.vnum for record in baseline_world.hlquests},
        "soc": {
            record.vnum
            for records in (
                baseline_world.rooms,
                baseline_world.mobiles,
                baseline_world.objects,
            )
            for record in records
        },
    }
    existing_target_merge_destinations = [
        str(row["source_record_id"])
        for row in merge_rows
        if row.get("destination_vnum")
        in baseline_by_kind.get(str(row["source_kind"]), set())
    ]
  return {
      "rows": len(action_rows),
      "actions": dict(sorted(actions.items())),
      "source_kinds": dict(sorted(kinds.items())),
      "selected_target_records": len(targets),
      "missing_destinations": missing_destinations,
      "noncanonical_destinations": noncanonical,
      "source_internal_merge_actions": len(merge_rows),
      "source_internal_merges_with_existing_target_destination": (
          existing_target_merge_destinations
      ),
      "blocking_selected_record_findings": blocking,
      "pass": len(action_rows) == 71_680
      and not missing_destinations
      and not noncanonical
      and not blocking,
  }


def _behavior_evidence(world, selected_zones: set[int]) -> dict[str, Any]:
  reset_counts = Counter(
      command.command for zone in world.zones for command in zone.commands
  )
  exits = [exit_record for room in world.rooms for exit_record in room.exits]
  object_traps = 0
  for obj in world.objects:
    decoded = decode_tokens(obj.extra_flags)
    if 116 in decoded.bits:
      object_traps += 1
  hlquest_entries = sum(len(record.entries) for record in world.hlquests)
  hlquest_commands = sum(
      len(entry.commands) for record in world.hlquests for entry in record.entries
  )
  samples = {
      "zones": sorted(selected_zones)[:20],
      "shops": sorted(record.vnum for record in world.shops if record.vnum >= 2_000_000)[:20],
      "hlquests": sorted(record.vnum for record in world.hlquests if record.vnum >= 2_000_000)[:20],
      "special_mobiles": sorted(
          record.vnum
          for record in world.mobiles
          if record.vnum >= 2_000_000 and record.spec_proc is not None
      )[:20],
      "triggered_rooms": sorted(
          record.vnum
          for record in world.rooms
          if record.vnum >= 2_000_000 and record.attachments
      )[:20],
  }
  counts = {
      "zones": len(world.zones),
      "rooms": len(world.rooms),
      "mobiles": len(world.mobiles),
      "objects": len(world.objects),
      "shops": len(world.shops),
      "triggers": len(world.triggers),
      "quests": len(world.quests),
      "hlquests": len(world.hlquests),
      "hlquest_entries": hlquest_entries,
      "hlquest_commands": hlquest_commands,
      "reset_commands": sum(reset_counts.values()),
      "room_exits": len(exits),
      "keyed_exits": sum((exit_record.key_vnum or -1) > 0 for exit_record in exits),
      "containers": sum(obj.item_type == 15 for obj in world.objects),
      "object_traps": object_traps,
      "exit_traps": sum(len(room.rol_exit_traps) for room in world.rooms),
      "mobile_paths": sum(bool(mobile.path_rooms) for mobile in world.mobiles),
      "mobile_specials": sum(mobile.spec_proc is not None for mobile in world.mobiles),
      "object_specials": sum(obj.spec_proc is not None for obj in world.objects),
      "room_specials": sum(room.spec_proc is not None for room in world.rooms),
      "attachments": sum(len(room.attachments) for room in world.rooms)
      + sum(len(mobile.attachments) for mobile in world.mobiles)
      + sum(len(obj.attachments) for obj in world.objects),
      "shop_products": sum(len(shop.product_vnums) for shop in world.shops),
  }
  required_positive = (
      "rooms",
      "mobiles",
      "objects",
      "shops",
      "triggers",
      "hlquests",
      "reset_commands",
      "room_exits",
      "keyed_exits",
      "containers",
      "object_traps",
      "exit_traps",
      "mobile_specials",
      "attachments",
      "shop_products",
  )
  return {
      "counts": counts,
      "reset_commands_by_opcode": dict(sorted(reset_counts.items())),
      "deterministic_walkthrough_samples": samples,
      "required_positive": list(required_positive),
      "pass": all(counts[name] > 0 for name in required_positive),
  }


def _read_log(path: Path, label: str) -> str:
  if not path.is_file():
    raise RolPhase8Error(f"{label} log is required: {path}")
  return path.read_text(encoding="utf-8", errors="replace")


def _code_gates(repo_root: Path, logs: dict[str, Path]) -> dict[str, Any]:
  if set(logs) != set(_LOG_NAMES):
    raise RolPhase8Error("Phase 8 requires world-tools, cutest, install, syntax, and runtime logs")
  texts = {name: _read_log(logs[name], name) for name in _LOG_NAMES}
  world_matches = re.findall(r"Ran ([0-9]+) tests?", texts["world-tools"])
  cutest_matches = re.findall(r"OK \(([0-9]+) tests\)", texts["cutest"])
  converted_diagnostics = [
      line
      for line in (texts["syntax"] + "\n" + texts["runtime"]).splitlines()
      if any(marker in line for marker in ("SYSERR", "ZONE ERROR", "invalid", "missing"))
      and re.search(r"(?:#|zone\s+)20(?:[0-9]{3}|[0-9]{5,6})\b", line, re.IGNORECASE)
  ]
  gates = {
      "world_tools": {
          "passed": bool(world_matches) and "OK" in texts["world-tools"],
          "tests": int(world_matches[-1]) if world_matches else 0,
      },
      "production_cutests": {
          "passed": bool(cutest_matches),
          "tests": int(cutest_matches[-1]) if cutest_matches else 0,
      },
      "install": {
          "passed": "Installed release:" in texts["install"]
          and (repo_root / "bin/circle").is_file()
          and not (repo_root / "circle").exists(),
          "root_circle_absent": not (repo_root / "circle").exists(),
          "installed_binary_sha256": _sha256_path(repo_root / "bin/circle"),
      },
      "syntax_boot": {
          "passed": "Syntax check mode enabled." in texts["syntax"]
          and "Done." in texts["syntax"],
      },
      "bounded_runtime_boot": {
          "passed": all(
              marker in texts["runtime"]
              for marker in (
                  "Entering game loop.",
                  "Resetting #20000:",
                  "Normal termination of game.",
              )
          ),
      },
      "converted_boot_diagnostics": {
          "passed": not converted_diagnostics,
          "count": len(converted_diagnostics),
          "samples": converted_diagnostics[:40],
      },
  }
  gates["world_tools"]["log_sha256"] = _sha256_path(logs["world-tools"])
  gates["production_cutests"]["log_sha256"] = _sha256_path(logs["cutest"])
  gates["install"]["log_sha256"] = _sha256_path(logs["install"])
  gates["syntax_boot"]["log_sha256"] = _sha256_path(logs["syntax"])
  gates["bounded_runtime_boot"]["log_sha256"] = _sha256_path(logs["runtime"])
  gates["converted_boot_diagnostics"]["log_sha256"] = _sha256_path(logs["runtime"])
  return {"gates": gates, "all_pass": all(bool(row["passed"]) for row in gates.values())}


def _code_evidence(repo_root: Path) -> dict[str, Any]:
  rows = []
  for relative in _CODE_EVIDENCE_PATHS:
    path = repo_root / relative
    if not path.is_file():
      raise RolPhase8Error(f"Phase 8 code evidence path is missing: {relative}")
    rows.append({"path": relative, "sha256": _sha256_path(path)})
  return {"files": rows, "installed_binary_sha256": _sha256_path(repo_root / "bin/circle")}


def _apply_plan(
    baseline_world: Path, phase7_dir: Path, paths: Iterable[str]
) -> list[dict[str, Any]]:
  rows = []
  for relative in sorted(paths):
    before = baseline_world / relative
    after = phase7_dir / "output/world" / relative
    rows.append(
        {
            "path": relative,
            "action": "REPLACE" if before.is_file() else "ADD",
            "before_sha256": _sha256_path(before) if before.is_file() else None,
            "after_sha256": _sha256_path(after),
        }
    )
  return rows


def write_phase8_bundle(
    phase7_dir: Path,
    repeat_phase7_dir: Path,
    target_world: Path,
    output_dir: Path,
    logs: dict[str, Path],
    created_at: str | None = None,
) -> dict[str, Any]:
  """Assemble and seal the complete, non-mutating Phase 8 release candidate."""

  repo_root = default_repo_root()
  phase7_dir = phase7_dir.resolve()
  repeat_phase7_dir = repeat_phase7_dir.resolve()
  target_world = target_world.resolve()
  output_dir = output_dir.resolve()
  logs = {name: path.resolve() for name, path in logs.items()}
  if output_dir.exists():
    raise RolPhase8Error(f"Phase 8 output directory already exists: {output_dir}")
  phase7 = _verify_bundle(phase7_dir, 7, "cumulative-milestone")
  if not phase7.get("acceptance", {}).get("phase7_complete"):
    raise RolPhase8Error("Phase 8 requires the accepted final Phase 7 milestone")
  if not phase7.get("acceptance", {}).get("connection_graph_pass"):
    raise RolPhase8Error("Phase 8 requires exact isolated RoL connection-graph parity")
  baseline_tree = tree_manifest(target_world)
  if baseline_tree["tree_sha256"] != phase7.get("frozen_target_tree_sha256"):
    raise RolPhase8Error("development world changed after the Phase 7 baseline freeze")
  repeat = _repeat_evidence(phase7_dir, repeat_phase7_dir)
  if not repeat["pass"]:
    raise RolPhase8Error("final Phase 7 repeat generation is not byte-identical")

  paths = _phase7_paths(phase7_dir)
  with tempfile.TemporaryDirectory(prefix="rol-phase8-") as temporary:
    candidate_world = Path(temporary) / "world"
    _assemble_candidate(phase7_dir, target_world, candidate_world)
    candidate_tree = tree_manifest(candidate_world)
    if candidate_tree["tree_sha256"] != phase7.get("staged_tree_sha256"):
      raise RolPhase8Error("Phase 8 candidate does not reproduce the accepted Phase 7 tree")

    constants = load_manifest(repo_root / "scripts/world/wtool_constants.json")
    config = resolve_config(target_world, None)
    baseline_validation = result_payload(
        validate_indexed_world(target_world, repo_root, constants, config)
    )
    candidate_validation = result_payload(
        validate_indexed_world(candidate_world, repo_root, constants, config)
    )
    baseline_validation["root"] = "development/world"
    candidate_validation["root"] = "phase8-candidate/world"
    delta = _validation_delta(baseline_validation, candidate_validation)
    baseline_world = load_indexed_world_data(target_world, repo_root, constants, config)
    world = load_indexed_world_data(candidate_world, repo_root, constants, config)
    action_rows = _load_jsonl(phase7_dir / "action-ledger.jsonl")
    action_audit = _action_audit(
        action_rows, candidate_validation, baseline_world
    )
    selected_zones = {
        int(row["destination_vnum"])
        for row in action_rows
        if row["source_kind"] == "zon" and row.get("destination_vnum") is not None
    }
    runtime = _runtime_contract(world, selected_zones)
    behavior = _behavior_evidence(world, selected_zones)
    mechanics_isolation = _mechanics_isolation_audit(
        repo_root, baseline_world, world
    )
    persistence = audit_development_persistence(world, repo_root)

  line_format = _line_format_audit(phase7_dir / "output/world", paths)
  code_gates = _code_gates(repo_root, logs)
  code_evidence = _code_evidence(repo_root)
  persistence_summary = persistence["summary"]
  namespace = {
      "persistence_mode": persistence["mode"],
      "persisted_rol_vnums": persistence_summary["distinct_persisted_rol_vnums"],
      "persisted_database_rows": persistence_summary["database_rows"],
      "missing_persisted_targets": persistence_summary[
          "missing_candidate_definitions"
      ],
      "duplicate_persisted_targets": persistence_summary[
          "duplicate_candidate_definitions"
      ],
  }
  namespace["pass"] = persistence_summary["pass"]
  apply_rows = _apply_plan(target_world, phase7_dir, paths)
  compiler = _load_json(phase7_dir / "compiler-summary.json")
  connection_graph = _load_json(phase7_dir / "validation/connection-graph.json")
  graph_summary = connection_graph["summary"]
  graph_isolated = (
      graph_summary["pass"]
      and graph_summary.get("cross_world_typed_references") == 0
      and graph_summary.get("missing_namespace_targets") == 0
  )
  reconciliation = {
      "phase7_run_id": phase7["run_id"],
      "packages": phase7["acceptance"]["final_packages"],
      "records": phase7["acceptance"]["final_records"],
      "apply_paths": len(apply_rows),
      "compiler": compiler,
      "reference_exceptions": len(_load_jsonl(phase7_dir / "reference-exceptions.jsonl")),
      "baseline_tree_sha256": baseline_tree["tree_sha256"],
      "candidate_tree_sha256": candidate_tree["tree_sha256"],
  }

  output_dir.mkdir(parents=True)
  output_world = output_dir / "output/world"
  for relative in paths:
    source = phase7_dir / "output/world" / relative
    destination = output_world / relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
  artifacts: list[dict[str, Any]] = []
  evidence = {
      "freeze.json": {
          "phase7_manifest_sha256": _sha256_path(phase7_dir / "run-manifest.json"),
          "repeat_phase7_manifest_sha256": _sha256_path(
              repeat_phase7_dir / "run-manifest.json"
          ),
          "baseline_tree": baseline_tree,
          "candidate_tree": candidate_tree,
      },
      "repeat-generation.json": repeat,
      "reconciliation.json": reconciliation,
      "namespace-audit.json": namespace,
      "persistence-check.json": persistence,
      "action-audit.json": action_audit,
      "behavior-evidence.json": behavior,
      "runtime-contract.json": runtime,
      "connection-graph.json": connection_graph,
      "mechanics-isolation.json": mechanics_isolation,
      "line-format-audit.json": line_format,
      "code-gates.json": code_gates,
      "code-evidence.json": code_evidence,
      "validation/baseline.json": baseline_validation,
      "validation/candidate.json": candidate_validation,
      "validation/delta.json": delta,
  }
  for relative, payload in sorted(evidence.items()):
    path = output_dir / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  plan_path = output_dir / "apply-plan.jsonl"
  _write_jsonl(plan_path, apply_rows)
  artifacts.append(_artifact(plan_path, output_dir, len(apply_rows)))
  for name in _LOG_NAMES:
    destination = output_dir / f"logs/{name}.log"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(logs[name], destination)
    artifacts.append(_artifact(destination, output_dir))
  for relative in paths:
    artifacts.append(_artifact(output_world / relative, output_dir))

  ready = all(
      (
          repeat["pass"],
          delta["new_active_errors"] == 0,
          action_audit["pass"],
          runtime["all_pass"],
          graph_isolated,
          mechanics_isolation["pass"],
          behavior["pass"],
          line_format["pass"],
          code_gates["all_pass"],
          namespace["pass"],
          phase7["acceptance"]["final_records"] == 71_680,
          phase7["acceptance"]["final_packages"] == 258,
      )
  )
  seed = "\n".join(
      row["sha256"] for row in sorted(artifacts, key=lambda row: row["path"])
  ).encode("ascii")
  run_id = f"rol-phase8-release-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_PHASE8_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 8,
      "stage": "release-candidate",
      "phase7_run_id": phase7["run_id"],
      "baseline_tree_sha256": baseline_tree["tree_sha256"],
      "candidate_tree_sha256": candidate_tree["tree_sha256"],
      "installed_binary_sha256": code_evidence["installed_binary_sha256"],
      "artifacts": sorted(artifacts, key=lambda row: row["path"]),
      "acceptance": {
          "packages": phase7["acceptance"]["final_packages"],
          "records": phase7["acceptance"]["final_records"],
          "apply_paths": len(apply_rows),
          "new_active_errors": delta["new_active_errors"],
          "selected_records_clean": action_audit["pass"],
          "runtime_contract_pass": runtime["all_pass"],
          "connection_graph_pass": graph_isolated,
          "cross_world_typed_references": graph_summary.get(
              "cross_world_typed_references"
          ),
          "missing_namespace_targets": graph_summary.get(
              "missing_namespace_targets"
          ),
          "mechanics_isolation_pass": mechanics_isolation["pass"],
          "behavior_evidence_pass": behavior["pass"],
          "code_gates_pass": code_gates["all_pass"],
          "namespace_audit_pass": namespace["pass"],
          "persistence_check_pass": persistence_summary["pass"],
          "repeat_generation_byte_identical": repeat["pass"],
          "ready_to_apply": ready,
      },
  }
  (output_dir / "run-manifest.json").write_bytes(_canonical_json(manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "packages": phase7["acceptance"]["final_packages"],
      "records": phase7["acceptance"]["final_records"],
      "apply_paths": len(apply_rows),
      "new_active_errors": delta["new_active_errors"],
      "connection_graph_pass": graph_isolated,
      "mechanics_isolation_pass": mechanics_isolation["pass"],
      "ready_to_apply": ready,
      "candidate_tree_sha256": candidate_tree["tree_sha256"],
  }


def apply_phase8_bundle(bundle_dir: Path, lib_root: Path) -> dict[str, Any]:
  """Apply one hash-preconditioned Phase 8 world overlay to development."""

  bundle_dir = bundle_dir.resolve()
  lib_root = lib_root.resolve()
  world_root = lib_root / "world"
  _development_environment(lib_root)
  manifest = _verify_bundle(bundle_dir, 8, "release-candidate")
  if not manifest.get("acceptance", {}).get("ready_to_apply"):
    raise RolPhase8Error("Phase 8 release candidate is not accepted")
  if _sha256_path(default_repo_root() / "bin/circle") != manifest["installed_binary_sha256"]:
    raise RolPhase8Error("installed runtime changed after Phase 8 validation")
  current_tree = tree_manifest(world_root)["tree_sha256"]
  if current_tree not in {
      manifest["baseline_tree_sha256"],
      manifest["candidate_tree_sha256"],
  }:
    raise RolPhase8Error("development world changed after Phase 8 staging")
  operations: list[tuple[Path, Path]] = []
  unchanged = 0
  for row in _load_jsonl(bundle_dir / "apply-plan.jsonl"):
    relative = str(row["path"])
    source = (bundle_dir / "output/world" / relative).resolve()
    destination = (world_root / relative).resolve()
    try:
      source.relative_to((bundle_dir / "output/world").resolve())
      destination.relative_to(world_root.resolve())
    except ValueError as error:
      raise RolPhase8Error("Phase 8 apply path escapes its declared root") from error
    current = _sha256_path(destination) if destination.is_file() else None
    if current == row["after_sha256"]:
      unchanged += 1
      continue
    if current != row["before_sha256"]:
      raise RolPhase8Error(f"apply destination changed since staging: {relative}")
    if not source.is_file() or _sha256_path(source) != row["after_sha256"]:
      raise RolPhase8Error(f"apply source is missing or changed: {relative}")
    operations.append((source, destination))
  for source, destination in operations:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
  after_tree = tree_manifest(world_root)["tree_sha256"]
  if after_tree != manifest["candidate_tree_sha256"]:
    raise RolPhase8Error("post-apply development tree differs from the accepted candidate")
  return {
      "run_id": manifest["run_id"],
      "changed_paths": len(operations),
      "already_current_paths": unchanged,
      "candidate_tree_sha256": after_tree,
      "idempotent_no_op": not operations,
  }


def _documentation_audit(repo_root: Path) -> dict[str, Any]:
  rows = []
  for relative in _DOCUMENTATION_PATHS:
    path = repo_root / relative
    if not path.is_file():
      rows.append({"path": relative, "present": False, "ascii": False, "lf_only": False})
      continue
    data = path.read_bytes()
    rows.append(
        {
            "path": relative,
            "present": True,
            "ascii": all(byte < 128 for byte in data),
            "lf_only": b"\r" not in data,
            "sha256": hashlib.sha256(data).hexdigest(),
        }
    )
  canonical_path = repo_root / _DOCUMENTATION_PATHS[0]
  changelog_path = repo_root / "docs/CHANGELOG.md"
  canonical = canonical_path.read_text(encoding="ascii") if canonical_path.is_file() else ""
  changelog = changelog_path.read_text(encoding="ascii") if changelog_path.is_file() else ""
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
      "canonical_contract_present": canonical_contract_present,
      "isolation_correction_changelog_present": (
          "### RoL isolation correction and full-corpus import" in changelog
      ),
      "pass": all(row["present"] and row["ascii"] and row["lf_only"] for row in rows)
      and canonical_contract_present
      and "### RoL isolation correction and full-corpus import" in changelog,
  }


def write_phase8_completion(
    bundle_dir: Path,
    lib_root: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Seal post-apply Phase 8 validation, documentation, and idempotency evidence."""

  repo_root = default_repo_root()
  bundle_dir = bundle_dir.resolve()
  lib_root = lib_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPhase8Error(f"Phase 8 completion directory already exists: {output_dir}")
  manifest = _verify_bundle(bundle_dir, 8, "release-candidate")
  if not manifest.get("acceptance", {}).get("ready_to_apply"):
    raise RolPhase8Error("Phase 8 release candidate is not accepted")
  _development_environment(lib_root)
  world_root = lib_root / "world"
  current_tree = tree_manifest(world_root)
  if current_tree["tree_sha256"] != manifest["candidate_tree_sha256"]:
    raise RolPhase8Error("Phase 8 candidate has not been applied to development")
  constants = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  config = resolve_config(world_root, None)
  validation = result_payload(validate_indexed_world(world_root, repo_root, constants, config))
  validation["root"] = "development/world"
  candidate_validation = _load_json(bundle_dir / "validation/candidate.json")
  postapply_matches = (
      validation["complete"] == candidate_validation["complete"]
      and validation["summary"] == candidate_validation["summary"]
      and validation["findings"] == candidate_validation["findings"]
  )
  repeat_apply = apply_phase8_bundle(bundle_dir, lib_root)
  documentation = _documentation_audit(repo_root)
  code_evidence = _load_json(bundle_dir / "code-evidence.json")
  code_unchanged = all(
      (repo_root / row["path"]).is_file()
      and _sha256_path(repo_root / row["path"]) == row["sha256"]
      for row in code_evidence["files"]
  )
  acceptance = {
      "development_tree_matches_candidate": True,
      "postapply_validation_matches_candidate": postapply_matches,
      "repeat_apply_no_op": repeat_apply["idempotent_no_op"],
      "repeat_apply_changed_paths": repeat_apply["changed_paths"],
      "documentation_pass": documentation["pass"],
      "runtime_code_unchanged_since_gates": code_unchanged,
  }
  acceptance["complete"] = all(
      (
          acceptance["development_tree_matches_candidate"],
          acceptance["postapply_validation_matches_candidate"],
          acceptance["repeat_apply_no_op"],
          acceptance["documentation_pass"],
          acceptance["runtime_code_unchanged_since_gates"],
      )
  )
  output_dir.mkdir(parents=True)
  evidence = {
      "acceptance.json": acceptance,
      "postapply-validation.json": validation,
      "postapply-tree.json": current_tree,
      "repeat-apply.json": repeat_apply,
      "documentation-audit.json": documentation,
  }
  artifacts = []
  for relative, payload in sorted(evidence.items()):
    path = output_dir / relative
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  seed = "\n".join(row["sha256"] for row in artifacts).encode("ascii")
  run_id = f"rol-phase8-complete-{hashlib.sha256(seed).hexdigest()[:16]}"
  completion_manifest = {
      "schema_version": ROL_PHASE8_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 8,
      "stage": "completion-audit",
      "release_run_id": manifest["run_id"],
      "candidate_tree_sha256": manifest["candidate_tree_sha256"],
      "artifacts": sorted(artifacts, key=lambda row: row["path"]),
      "acceptance": acceptance,
  }
  (output_dir / "run-manifest.json").write_bytes(_canonical_json(completion_manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "release_run_id": manifest["run_id"],
      "repeat_apply_no_op": repeat_apply["idempotent_no_op"],
      "documentation_pass": documentation["pass"],
      "complete": acceptance["complete"],
  }


def render_rol_phase8_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 8 release: {summary['run_id']}\n"
      f"Output: {summary['output_dir']}\n"
      f"Packages: {summary['packages']}\n"
      f"Records: {summary['records']}\n"
      f"Apply paths: {summary['apply_paths']}\n"
      f"Ready to apply: {str(summary['ready_to_apply']).lower()}\n"
  )
