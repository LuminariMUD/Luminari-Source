"""Deterministic Phase 6 RoL special-procedure reconciliation evidence."""

from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any, Iterable

from .constants import default_repo_root
from .models import TOOL_VERSION
from .rol_planner import verify_discovery_bundle
from .rol_skeleton import verify_plan_bundle
from .rol_discovery import extract_spec_bindings
from .rol_special import (
    ADAPTED_HANDLER_NAMES,
    COMPOSABLE_MOBILE_HANDLER_AFFECTS,
    COMPOSABLE_MOBILE_HANDLER_FLAGS,
    COMPOSABLE_MOBILE_RUNTIME_HANDLERS,
    COMPOSABLE_ROOM_HANDLER_FLAGS,
    INERT_HANDLERS,
    NATIVE_HANDLER_NAMES,
    RECONCILED_OBJECT_RUNTIME_HANDLERS,
    UNSAFE_HANDLERS,
)


ROL_SPECIAL_RECONCILIATION_SCHEMA_VERSION = 4
_SOURCE_ROOT_PREFIX = "EXAMPLE/RealmsOfLuminari"
_RECORD_KIND = {"mobile": "mob", "object": "obj", "room": "wld"}
_AUTO_RACE_HANDLERS = {
    "X": "standardDemon",
    "Y": "standardDevil",
    "MH": "standardUmberhulk",
}
_AUTO_RACE_TARGETS = {
    "X": "MOB_ROL_DEMON composition-safe runtime hook",
    "Y": "MOB_ROL_DEVIL composition-safe runtime hook",
    "MH": "MOB_ROL_UMBERHULK composition-safe runtime hook",
}
_DYNAMIC_REGISTRATION_DISPOSITIONS = {
    "quester": {
        "record_kind": "qst",
        "strategy": "TARGET_DATA_DRIVEN_SERVICE",
        "target": "target HLQuest loader and quester service",
        "reason": (
            "source boot attaches one quester callback per active quest block; the target "
            "HLQuest loader owns the same host-driven behavior without a persisted named binding"
        ),
    },
    "shop_keeper": {
        "record_kind": "shp",
        "strategy": "TARGET_DATA_DRIVEN_SERVICE",
        "target": "target shop loader and shopkeeper service",
        "reason": (
            "source boot attaches one shop callback per active shop block; the target shop "
            "loader owns the same keeper-driven behavior and composes authored named bindings"
        ),
    },
}
_DG_HANDLERS = frozenset(
    {
        "cemetary_instrument_rub",
        "fw_warning_room",
        "muspel_chieftain_open",
        "muspel_chimney_pour",
        "muspel_giant_shout_m58708_m58709",
        "muspel_giant_shout_m58806",
        "muspel_giant_shout_m58833",
        "muspel_ice_river",
        "muspel_lookout_shout_m58708_m58709",
        "muspel_lookout_shout_m58806",
        "muspel_lookout_shout_m58833",
    }
)
_FUNCTION_HEADER = re.compile(
    r"(?m)^[ \t]*(?:(?:[A-Za-z_][A-Za-z0-9_]*|\*)[ \t]+)*"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\([^;{}]*?\)[ \t\r\n]*\{"
)


class RolSpecialReconciliationError(ValueError):
  """Raised when Phase 6 evidence cannot be produced deterministically."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _canonical_line(data: Any) -> bytes:
  return (
      json.dumps(data, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n"
  ).encode(
      "ascii",
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
    raise RolSpecialReconciliationError(
        "--created-at must be an ISO-8601 timestamp"
    ) from error
  if parsed.tzinfo is None:
    raise RolSpecialReconciliationError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _load_json(path: Path) -> Any:
  try:
    return json.loads(path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolSpecialReconciliationError(f"cannot read {path}: {error}") from error


def _read_jsonl(path: Path) -> Iterable[dict[str, Any]]:
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolSpecialReconciliationError(
              f"invalid JSONL at {path}:{line_number}: {error}"
          ) from error
        if not isinstance(row, dict):
          raise RolSpecialReconciliationError(
              f"JSONL row at {path}:{line_number} is not an object"
          )
        yield row
  except OSError as error:
    raise RolSpecialReconciliationError(f"cannot read {path}: {error}") from error


def _verify_capability_audit(audit_dir: Path) -> dict[str, Any]:
  manifest = _load_json(audit_dir / "run-manifest.json")
  if manifest.get("phase") != 5 or manifest.get("stage") != "full-corpus-capability-audit":
    raise RolSpecialReconciliationError("ACT_SPEC input is not a Phase 5 capability audit")
  for artifact in manifest.get("artifacts", []):
    path = audit_dir / str(artifact["path"])
    if not path.is_file():
      raise RolSpecialReconciliationError(f"capability-audit artifact is missing: {path.name}")
    if _sha256_path(path) != artifact["sha256"]:
      raise RolSpecialReconciliationError(
          f"capability-audit artifact hash mismatch: {path.name}"
      )
  return manifest


def _matching_brace(text: str, start: int) -> int | None:
  depth = 0
  index = start
  state = "code"
  while index < len(text):
    char = text[index]
    pair = text[index : index + 2]
    if state == "block":
      if pair == "*/":
        state = "code"
        index += 2
        continue
    elif state == "line":
      if char == "\n":
        state = "code"
    elif state in {"string", "character"}:
      if char == "\\":
        index += 2
        continue
      if (state == "string" and char == '"') or (state == "character" and char == "'"):
        state = "code"
    elif pair == "/*":
      state = "block"
      index += 2
      continue
    elif pair == "//":
      state = "line"
      index += 2
      continue
    elif char == '"':
      state = "string"
    elif char == "'":
      state = "character"
    elif char == "{":
      depth += 1
    elif char == "}":
      depth -= 1
      if depth == 0:
        return index + 1
    index += 1
  return None


def _mask_non_code(text: str) -> str:
  """Replace comments and literals with spaces while preserving offsets and lines."""

  output = list(text)
  index = 0
  state = "code"
  while index < len(text):
    char = text[index]
    pair = text[index : index + 2]
    if state == "block":
      if pair == "*/":
        output[index] = output[index + 1] = " "
        state = "code"
        index += 2
        continue
      if char != "\n":
        output[index] = " "
    elif state == "line":
      if char == "\n":
        state = "code"
      else:
        output[index] = " "
    elif state in {"string", "character"}:
      if char == "\\" and index + 1 < len(text):
        output[index] = " "
        if text[index + 1] != "\n":
          output[index + 1] = " "
        index += 2
        continue
      if (state == "string" and char == '"') or (state == "character" and char == "'"):
        output[index] = " "
        state = "code"
      elif char != "\n":
        output[index] = " "
    elif pair == "/*":
      output[index] = output[index + 1] = " "
      state = "block"
      index += 2
      continue
    elif pair == "//":
      output[index] = output[index + 1] = " "
      state = "line"
      index += 2
      continue
    elif char == '"':
      output[index] = " "
      state = "string"
    elif char == "'":
      output[index] = " "
      state = "character"
    index += 1
  return "".join(output)


def source_handler_definitions(
    source_root: Path, handlers: set[str]
) -> dict[str, dict[str, Any]]:
  """Locate assigned source function bodies without relying on external ctags."""

  definitions: dict[str, dict[str, Any]] = {}
  for path in sorted((source_root / "src").glob("*.c")):
    text = path.read_text(encoding="utf-8")
    masked = _mask_non_code(text)
    matches = [
        match
        for match in _FUNCTION_HEADER.finditer(masked)
        if match.group("name") in handlers
    ]
    if not matches:
      continue
    completed = subprocess.run(
        ["cc", "-E", f"-I{source_root / 'src'}", str(path)],
        capture_output=True,
        check=False,
        text=True,
    )
    if completed.returncode != 0:
      detail = completed.stderr.strip().splitlines()
      raise RolSpecialReconciliationError(
          f"source preprocessor failed for {path.relative_to(source_root)}: "
          f"{detail[-1] if detail else 'unknown error'}"
      )
    active_lines: set[int] = set()
    current_path: Path | None = None
    current_line = 0
    for preprocessed_line in completed.stdout.splitlines():
      marker = re.match(r'^#\s+(\d+)\s+"([^"]+)"', preprocessed_line)
      if marker is not None:
        current_path = Path(marker.group(2)).resolve()
        current_line = int(marker.group(1))
        continue
      if current_path == path.resolve() and preprocessed_line.strip():
        active_lines.add(current_line)
      current_line += 1
    for match in matches:
      name = match.group("name")
      line = text.count("\n", 0, match.start()) + 1
      if line not in active_lines or name in definitions:
        continue
      brace = masked.find("{", match.start(), match.end())
      end = _matching_brace(text, brace)
      if end is None:
        continue
      body = text[match.start() : end]
      end_line = line + body.count("\n")
      definitions[name] = {
          "path": path.relative_to(source_root).as_posix(),
          "line": line,
          "end_line": end_line,
          "lines": end_line - line + 1,
          "sha256": hashlib.sha256(body.encode("utf-8")).hexdigest(),
          "event_tokens": sorted(set(re.findall(r"\bPROC_[A-Z0-9_]+\b", body))),
      }
  return definitions


def handler_disposition(handler: str) -> dict[str, str]:
  """Return the current reviewed Phase 6 disposition for one source handler."""

  if handler in NATIVE_HANDLER_NAMES:
    return {
        "status": "resolved",
        "strategy": "NATIVE_PERSISTED",
        "target": NATIVE_HANDLER_NAMES[handler],
    }
  if handler in _DG_HANDLERS:
    return {"status": "resolved", "strategy": "DG_COMPILED", "target": "DG trigger"}
  if handler in ADAPTED_HANDLER_NAMES:
    return {
        "status": "resolved",
        "strategy": "NATIVE_ADAPTED",
        "target": ADAPTED_HANDLER_NAMES[handler],
    }
  if handler in COMPOSABLE_MOBILE_HANDLER_FLAGS or handler in COMPOSABLE_MOBILE_HANDLER_AFFECTS:
    action = COMPOSABLE_MOBILE_HANDLER_FLAGS.get(handler)
    affects = COMPOSABLE_MOBILE_HANDLER_AFFECTS.get(handler, ())
    target_parts = []
    if action is not None:
      target_parts.append(f"mobile action flag {action}")
    target_parts.extend(f"mobile affect flag {affect}" for affect in affects)
    return {
        "status": "resolved",
        "strategy": "NATIVE_ADAPTED_COMPOSABLE",
        "target": " plus ".join(target_parts),
    }
  if handler in COMPOSABLE_MOBILE_RUNTIME_HANDLERS:
    return {
        "status": "resolved",
        "strategy": "NATIVE_ADAPTED_COMPOSABLE",
        "target": COMPOSABLE_MOBILE_RUNTIME_HANDLERS[handler],
    }
  if handler in COMPOSABLE_ROOM_HANDLER_FLAGS:
    return {
        "status": "resolved",
        "strategy": "NATIVE_ADAPTED_COMPOSABLE",
        "target": f"room flag {COMPOSABLE_ROOM_HANDLER_FLAGS[handler]}",
    }
  if handler in RECONCILED_OBJECT_RUNTIME_HANDLERS:
    _, target = RECONCILED_OBJECT_RUNTIME_HANDLERS[handler]
    return {
        "status": "resolved",
        "strategy": "NATIVE_RECONCILED",
        "target": target,
    }
  if handler in INERT_HANDLERS:
    return {
        "status": "resolved",
        "strategy": "SOURCE_INERT_EXCLUDED",
        "target": "none",
        "reason": INERT_HANDLERS[handler],
    }
  if handler in UNSAFE_HANDLERS:
    return {
        "status": "resolved",
        "strategy": "SOURCE_UNSAFE_EXCLUDED",
        "target": "none",
        "reason": UNSAFE_HANDLERS[handler],
    }
  return {"status": "pending", "strategy": "PENDING_TRACE", "target": "unresolved"}


def _mobile_race_code(record: dict[str, Any]) -> str:
  race_rows = list(record.get("values", {}).get("base_rows", []))
  return str(race_rows[0][0]).upper() if race_rows and race_rows[0] else "N"


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _artifact(path: Path, output_dir: Path, records: int | None = None) -> dict[str, Any]:
  result: dict[str, Any] = {
      "path": path.relative_to(output_dir).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }
  if records is not None:
    result["records"] = records
  return result


def write_special_reconciliation_bundle(
    discovery_dir: Path,
    plan_dir: Path,
    capability_audit_dir: Path,
    source_root: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write the full direct-binding and ACT_SPEC Phase 6 reconciliation ledger."""

  repo_root = default_repo_root()
  discovery_dir = discovery_dir.resolve()
  plan_dir = plan_dir.resolve()
  capability_audit_dir = capability_audit_dir.resolve()
  source_root = source_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolSpecialReconciliationError(
        f"special-reconciliation output directory already exists: {output_dir}"
    )
  if source_root != (repo_root / _SOURCE_ROOT_PREFIX).resolve():
    raise RolSpecialReconciliationError(
        "Phase 6 reconciliation requires the inventoried repository RoL source root"
    )

  discovery_manifest = verify_discovery_bundle(discovery_dir)
  plan_manifest = verify_plan_bundle(plan_dir)
  audit_manifest = _verify_capability_audit(capability_audit_dir)
  if plan_manifest.get("discovery_run_id") != discovery_manifest.get("run_id"):
    raise RolSpecialReconciliationError("Phase 1 and Phase 2 inputs do not belong together")
  if audit_manifest.get("plan_run_id") != plan_manifest.get("run_id"):
    raise RolSpecialReconciliationError("Phase 5 audit and Phase 2 plan do not belong together")

  binding_input = _load_json(discovery_dir / "bindings.json")
  discovered_bindings = list(binding_input["active_binding_candidates"])
  live_source_bindings = extract_spec_bindings(
      source_root,
      "src/specs.assign.c",
      "source",
      preprocess=True,
      follow_registration_wrappers=True,
  )
  active_source_bindings = {
      (
          str(row["record_type"]),
          int(row["vnum"]),
          str(row["handler"]),
          str(row["path"]),
          int(row["line"]),
      )
      for row in live_source_bindings
      if row["vnum"] is not None
  }
  dynamic_registrations = [row for row in live_source_bindings if row["vnum"] is None]
  dynamic_handlers = {str(row["handler"]) for row in dynamic_registrations}
  unexpected_dynamic_handlers = dynamic_handlers - set(_DYNAMIC_REGISTRATION_DISPOSITIONS)
  missing_dynamic_handlers = set(_DYNAMIC_REGISTRATION_DISPOSITIONS) - dynamic_handlers
  if unexpected_dynamic_handlers or missing_dynamic_handlers:
    raise RolSpecialReconciliationError(
        "dynamic source registration inventory changed: "
        f"unexpected={sorted(unexpected_dynamic_handlers)}, "
        f"missing={sorted(missing_dynamic_handlers)}"
    )
  bindings = [
      row
      for row in discovered_bindings
      if (
          str(row["record_type"]),
          int(row["source_vnum"]),
          str(row["source_handler"]),
          str(row["source_path"]),
          int(row["source_line"]),
      )
      in active_source_bindings
  ]
  excluded_bindings = [row for row in discovered_bindings if row not in bindings]
  handlers = {str(row["source_handler"]) for row in bindings}
  definitions = source_handler_definitions(
      source_root,
      handlers
      | {str(row["source_handler"]) for row in excluded_bindings}
      | dynamic_handlers
      | set(_AUTO_RACE_HANDLERS.values()),
  )

  consumers: defaultdict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  record_by_id: dict[str, dict[str, Any]] = {}
  dynamic_instance_counts: Counter[str] = Counter()
  dynamic_hosts: defaultdict[str, set[int]] = defaultdict(set)
  relevant_keys = {
      (_RECORD_KIND[str(row["record_type"])], int(row["source_vnum"]))
      for row in discovered_bindings
  }
  for record in _read_jsonl(discovery_dir / "source-records.jsonl"):
    key = (str(record["kind"]), int(record["vnum"]))
    if str(record["kind"]) in {
        disposition["record_kind"]
        for disposition in _DYNAMIC_REGISTRATION_DISPOSITIONS.values()
    }:
      dynamic_instance_counts[str(record["kind"])] += 1
      dynamic_hosts[str(record["kind"])].add(int(record["vnum"]))
    if key in relevant_keys or record["kind"] == "mob":
      record_by_id[str(record["record_id"])] = record
    if key in relevant_keys:
      consumers[key].append(record)

  actions = {
      str(row["source_record_id"]): row
      for row in _read_jsonl(plan_dir / "reconciliation.jsonl")
      if str(row["source_record_id"]) in record_by_id
  }

  binding_rows: list[dict[str, Any]] = []
  excluded_binding_rows: list[dict[str, Any]] = []
  bindings_by_mobile: defaultdict[int, list[int]] = defaultdict(list)
  for ordinal, binding in enumerate(bindings, start=1):
    record_type = str(binding["record_type"])
    source_vnum = int(binding["source_vnum"])
    disposition = handler_disposition(str(binding["source_handler"]))
    consumer_rows = []
    for record in sorted(
        consumers.get((_RECORD_KIND[record_type], source_vnum), []),
        key=lambda item: str(item["record_id"]),
    ):
      action = actions.get(str(record["record_id"]), {})
      consumer_rows.append(
          {
              "source_record_id": record["record_id"],
              "basename": record["basename"],
              "planned_action": action.get("action"),
              "destination_vnum": action.get("destination_vnum"),
          }
      )
    row = {
        "binding_id": (
            f"{record_type}:{source_vnum}:{binding['source_handler']}:"
            f"{binding['source_line']}"
        ),
        "record_type": record_type,
        "source_vnum": source_vnum,
        "source_handler": binding["source_handler"],
        "source_path": binding["source_path"],
        "source_line": binding["source_line"],
        "source_vnum_token": binding.get("source_vnum_token"),
        "source_vnum_resolution": binding.get("source_vnum_resolution"),
        "source_registration_path": binding.get("source_registration_path", []),
        "source_definition": definitions.get(str(binding["source_handler"])),
        "consumers": consumer_rows,
        **disposition,
    }
    binding_rows.append(row)
    if record_type == "mobile":
      bindings_by_mobile[source_vnum].append(ordinal - 1)

  for binding in excluded_bindings:
    record_type = str(binding["record_type"])
    source_vnum = int(binding["source_vnum"])
    consumer_rows = []
    for record in sorted(
        consumers.get((_RECORD_KIND[record_type], source_vnum), []),
        key=lambda item: str(item["record_id"]),
    ):
      action = actions.get(str(record["record_id"]), {})
      consumer_rows.append(
          {
              "source_record_id": record["record_id"],
              "basename": record["basename"],
              "planned_action": action.get("action"),
              "destination_vnum": action.get("destination_vnum"),
          }
      )
    excluded_binding_rows.append(
        {
            "binding_id": (
                f"{record_type}:{source_vnum}:{binding['source_handler']}:"
                f"{binding['source_line']}"
            ),
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": binding["source_handler"],
            "source_path": binding["source_path"],
            "source_line": binding["source_line"],
            "source_vnum_token": binding.get("source_vnum_token"),
            "source_vnum_resolution": binding.get("source_vnum_resolution"),
            "source_registration_path": binding.get("source_registration_path", []),
            "source_definition": definitions.get(str(binding["source_handler"])),
            "consumers": consumer_rows,
            "status": "excluded",
            "strategy": "SOURCE_PREPROCESSOR_EXCLUDED",
            "target": "none",
            "reason": (
                "source C preprocessor removes this assignment under the checked-in "
                "src/config.h build configuration"
            ),
        }
    )

  handler_counts = Counter(str(row["source_handler"]) for row in bindings)
  handler_owners: defaultdict[str, set[str]] = defaultdict(set)
  for row in bindings:
    handler_owners[str(row["source_handler"])].add(str(row["record_type"]))
  handler_rows = [
      {
          "source_handler": handler,
          "binding_count": handler_counts[handler],
          "owner_types": sorted(handler_owners[handler]),
          "source_definition": definitions.get(handler),
          **handler_disposition(handler),
      }
      for handler in sorted(handlers)
  ]

  dynamic_rows: list[dict[str, Any]] = []
  for registration in sorted(
      dynamic_registrations,
      key=lambda item: (str(item["handler"]), str(item["path"]), int(item["line"])),
  ):
    handler = str(registration["handler"])
    disposition = _DYNAMIC_REGISTRATION_DISPOSITIONS[handler]
    record_kind = str(disposition["record_kind"])
    dynamic_rows.append(
        {
            "registration_id": f"dynamic:{handler}:{registration['path']}:{registration['line']}",
            "source_handler": handler,
            "source_path": registration["path"],
            "source_line": registration["line"],
            "source_vnum_expression": registration["vnum_token"],
            "registration_path": registration["registration_path"],
            "source_definition": definitions.get(handler),
            "source_record_kind": record_kind,
            "active_binding_instances": dynamic_instance_counts[record_kind],
            "unique_source_hosts": len(dynamic_hosts[record_kind]),
            "status": "resolved",
            "strategy": disposition["strategy"],
            "target": disposition["target"],
            "reason": disposition["reason"],
        }
    )

  automatic_race_rows: list[dict[str, Any]] = []
  for record in sorted(record_by_id.values(), key=lambda item: str(item["record_id"])):
    if record["kind"] != "mob":
      continue
    race_code = _mobile_race_code(record)
    handler = _AUTO_RACE_HANDLERS.get(race_code)
    if handler is None:
      continue
    source_vnum = int(record["vnum"])
    linked = [binding_rows[index] for index in bindings_by_mobile.get(source_vnum, [])]
    action = actions.get(str(record["record_id"]), {})
    automatic_race_rows.append(
        {
            "binding_id": f"mobile:{source_vnum}:{handler}:source-boot",
            "source_record_id": record["record_id"],
            "source_vnum": source_vnum,
            "basename": record["basename"],
            "race_code": race_code,
            "source_handler": handler,
            "source_definition": definitions.get(handler),
            "planned_action": action.get("action"),
            "destination_vnum": action.get("destination_vnum"),
            "direct_binding_ids": [row["binding_id"] for row in linked],
            "composition": "alongside-direct" if linked else "implicit-only",
            "status": "resolved",
            "strategy": "NATIVE_ADAPTED_COMPOSABLE",
            "target": _AUTO_RACE_TARGETS[race_code],
            "reason": (
                "source db.c attaches this race procedure during every mobile load, "
                "including prototypes with direct assignments; a dedicated target flag "
                "runs beside the ordinary persistent special-procedure slot"
            ),
        }
    )

  automatic_by_record = {
      str(row["source_record_id"]): row for row in automatic_race_rows
  }

  act_spec_input = [
      row
      for row in _read_jsonl(capability_audit_dir / "transform-diagnostics.jsonl")
      if row.get("classification") == "special-binding-gap"
  ]
  act_spec_rows: list[dict[str, Any]] = []
  for diagnostic in act_spec_input:
    record_id = str(diagnostic["source_record_id"])
    record = record_by_id.get(record_id)
    if record is None:
      raise RolSpecialReconciliationError(f"ACT_SPEC record is absent from discovery: {record_id}")
    source_vnum = int(record["vnum"])
    binding_indexes = bindings_by_mobile.get(source_vnum, [])
    linked = [binding_rows[index] for index in binding_indexes]
    automatic = automatic_by_record.get(record_id)
    race_code = _mobile_race_code(record)
    if linked:
      resolved = all(row["status"] == "resolved" for row in linked) and (
          automatic is None or automatic["status"] == "resolved"
      )
      status = "resolved" if resolved else "pending"
      if automatic is not None:
        strategy = (
            "DIRECT_AND_AUTOMATIC_RACE_RESOLVED"
            if resolved
            else "DIRECT_AND_AUTOMATIC_RACE_PENDING"
        )
        reason = "source VNUM has direct assignments plus an implicit race procedure"
      else:
        strategy = "DIRECT_ASSIGNMENTS_RESOLVED" if resolved else "DIRECT_ASSIGNMENTS_PENDING"
        reason = "source VNUM has direct assignment-table bindings"
    elif automatic is not None:
      status = str(automatic["status"])
      strategy = "AUTOMATIC_RACE_BINDING_RESOLVED"
      reason = f"source boot attaches {automatic['source_handler']} before ACT_SPEC validation"
    else:
      status = "resolved"
      strategy = "SOURCE_BOOT_CLEARS_UNBOUND_ACT_SPEC"
      reason = "source db.c clears ACT_SPEC when no function is attached"
    act_spec_rows.append(
        {
            "source_record_id": record_id,
            "source_vnum": source_vnum,
            "basename": record["basename"],
            "race_code": race_code,
            "binding_ids": [row["binding_id"] for row in linked],
            "automatic_race_binding_id": (
                automatic["binding_id"] if automatic is not None else None
            ),
            "status": status,
            "strategy": strategy,
            "reason": reason,
        }
    )

  binding_status = Counter(str(row["status"]) for row in binding_rows)
  handler_status = Counter(str(row["status"]) for row in handler_rows)
  automatic_status = Counter(str(row["status"]) for row in automatic_race_rows)
  automatic_handlers = Counter(str(row["source_handler"]) for row in automatic_race_rows)
  automatic_composition = Counter(str(row["composition"]) for row in automatic_race_rows)
  act_spec_status = Counter(str(row["status"]) for row in act_spec_rows)
  strategy_counts = Counter(str(row["strategy"]) for row in binding_rows)
  dynamic_status = Counter(str(row["status"]) for row in dynamic_rows)
  dynamic_strategy = Counter(str(row["strategy"]) for row in dynamic_rows)
  act_spec_strategies = Counter(str(row["strategy"]) for row in act_spec_rows)
  summary = {
      "schema_version": ROL_SPECIAL_RECONCILIATION_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "discovery_run_id": discovery_manifest["run_id"],
      "plan_run_id": plan_manifest["run_id"],
      "capability_audit_run_id": audit_manifest["run_id"],
      "discovered_direct_binding_candidates": len(discovered_bindings),
      "source_preprocessor_excluded_bindings": len(excluded_binding_rows),
      "source_preprocessor_excluded_bindings_by_handler": dict(
          sorted(Counter(str(row["source_handler"]) for row in excluded_binding_rows).items())
      ),
      "active_direct_bindings": len(binding_rows),
      "direct_bindings_by_owner": dict(
          sorted(Counter(row["record_type"] for row in binding_rows).items())
      ),
      "direct_bindings_by_status": dict(sorted(binding_status.items())),
      "direct_bindings_by_strategy": dict(sorted(strategy_counts.items())),
      "source_handlers": len(handler_rows),
      "source_handlers_by_status": dict(sorted(handler_status.items())),
      "source_handler_definitions_located": sum(
          1 for handler in handlers if handler in definitions
      ),
      "active_dynamic_bindings": sum(
          int(row["active_binding_instances"]) for row in dynamic_rows
      ),
      "dynamic_registration_paths": len(dynamic_rows),
      "dynamic_bindings_by_handler": {
          str(row["source_handler"]): int(row["active_binding_instances"])
          for row in dynamic_rows
      },
      "dynamic_registrations_by_status": dict(sorted(dynamic_status.items())),
      "dynamic_registrations_by_strategy": dict(sorted(dynamic_strategy.items())),
      "dynamic_handler_definitions_located": sum(
          1 for handler in dynamic_handlers if handler in definitions
      ),
      "total_active_bindings": len(binding_rows)
      + sum(int(row["active_binding_instances"]) for row in dynamic_rows),
      "total_source_handlers": len(handlers | dynamic_handlers),
      "active_implicit_race_bindings": len(automatic_race_rows),
      "implicit_race_bindings_by_status": dict(sorted(automatic_status.items())),
      "implicit_race_bindings_by_handler": dict(sorted(automatic_handlers.items())),
      "implicit_race_bindings_by_composition": dict(
          sorted(automatic_composition.items())
      ),
      "implicit_race_handler_definitions_located": sum(
          1 for handler in set(_AUTO_RACE_HANDLERS.values()) if handler in definitions
      ),
      "act_spec_records": len(act_spec_rows),
      "act_spec_by_status": dict(sorted(act_spec_status.items())),
      "act_spec_by_strategy": dict(sorted(act_spec_strategies.items())),
      "live_target_writes": 0,
  }

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  summary_path = output_dir / "reconciliation-summary.json"
  summary_path.write_bytes(_canonical_json(summary))
  artifacts.append(_artifact(summary_path, output_dir))
  binding_path = output_dir / "binding-ledger.jsonl"
  artifacts.append(_artifact(binding_path, output_dir, _write_jsonl(binding_path, binding_rows)))
  excluded_binding_path = output_dir / "preprocessor-excluded-binding-ledger.jsonl"
  artifacts.append(
      _artifact(
          excluded_binding_path,
          output_dir,
          _write_jsonl(excluded_binding_path, excluded_binding_rows),
      )
  )
  handler_path = output_dir / "handler-inventory.jsonl"
  artifacts.append(_artifact(handler_path, output_dir, _write_jsonl(handler_path, handler_rows)))
  dynamic_path = output_dir / "dynamic-registration-ledger.jsonl"
  artifacts.append(
      _artifact(dynamic_path, output_dir, _write_jsonl(dynamic_path, dynamic_rows))
  )
  automatic_path = output_dir / "automatic-race-ledger.jsonl"
  artifacts.append(
      _artifact(
          automatic_path,
          output_dir,
          _write_jsonl(automatic_path, automatic_race_rows),
      )
  )
  act_spec_path = output_dir / "act-spec-ledger.jsonl"
  artifacts.append(_artifact(act_spec_path, output_dir, _write_jsonl(act_spec_path, act_spec_rows)))

  seed = "\n".join(
      artifact["sha256"] for artifact in sorted(artifacts, key=lambda item: item["path"])
  ).encode("ascii")
  run_id = f"rol-phase6-special-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_SPECIAL_RECONCILIATION_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 6,
      "stage": "special-procedure-reconciliation",
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "all_direct_bindings_accounted": (
              len(binding_rows) + len(excluded_binding_rows) == len(discovered_bindings)
          ),
          "all_handlers_accounted": len(handler_rows) == len(handlers),
          "all_dynamic_registrations_accounted": (
              len(dynamic_rows) == len(_DYNAMIC_REGISTRATION_DISPOSITIONS)
              and all(row["status"] == "resolved" for row in dynamic_rows)
              and all(int(row["active_binding_instances"]) > 0 for row in dynamic_rows)
          ),
          "all_automatic_race_bindings_accounted": len(automatic_race_rows)
          == sum(
              1
              for record in record_by_id.values()
              if record["kind"] == "mob"
              and _mobile_race_code(record) in _AUTO_RACE_HANDLERS
          ),
          "all_act_spec_records_accounted": len(act_spec_rows) == len(act_spec_input),
          "direct_bindings_pending": binding_status["pending"],
          "automatic_race_bindings_pending": automatic_status["pending"],
          "act_spec_records_pending": act_spec_status["pending"],
          "live_target_writes": 0,
      },
  }
  (output_dir / "run-manifest.json").write_bytes(_canonical_json(manifest))
  return {"run_id": run_id, "output_dir": output_dir.as_posix(), **summary}


def render_rol_special_reconciliation_human(summary: dict[str, Any]) -> str:
  """Render concise Phase 6 progress without implying that pending work is complete."""

  return "\n".join(
      (
          f"RoL Phase 6 special reconciliation: {summary['run_id']}",
          f"Output: {summary['output_dir']}",
          f"Discovered direct-binding candidates: "
          f"{summary['discovered_direct_binding_candidates']}",
          f"Source-preprocessor exclusions: "
          f"{summary['source_preprocessor_excluded_bindings']}",
          f"Direct bindings: {summary['active_direct_bindings']}",
          f"Resolved direct bindings: {summary['direct_bindings_by_status'].get('resolved', 0)}",
          f"Pending direct bindings: {summary['direct_bindings_by_status'].get('pending', 0)}",
          f"Dynamic registration paths: {summary['dynamic_registration_paths']}",
          f"Active dynamic bindings: {summary['active_dynamic_bindings']}",
          f"Total active bindings: {summary['total_active_bindings']}",
          f"Implicit race bindings: {summary['active_implicit_race_bindings']}",
          f"Pending implicit race bindings: "
          f"{summary['implicit_race_bindings_by_status'].get('pending', 0)}",
          f"Distinct source handlers: {summary['source_handlers']}",
          f"Total source handlers: {summary['total_source_handlers']}",
          f"ACT_SPEC records: {summary['act_spec_records']}",
          f"Resolved ACT_SPEC records: {summary['act_spec_by_status'].get('resolved', 0)}",
          f"Pending ACT_SPEC records: {summary['act_spec_by_status'].get('pending', 0)}",
          "Live target writes: 0",
          "",
      )
  )
