"""Deterministic full-corpus capability audit for the RoL conversion."""

from __future__ import annotations

from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
from typing import Any, Iterable

from .constants import default_repo_root
from .models import TOOL_VERSION
from .rol_pilot_build import (
    RolPilotBuildError,
    _identity_resolver,
    _load_jsonl,
    _source_records,
    _target_zone_by_basename,
    _verify_bundle,
)
from .rol_source import RolRecord
from .rol_transform import (
    APPLY_MAP,
    CLASS_MAP,
    MOB_ACTION_MAP,
    MOB_AFFECT_MAP,
    OBJECT_EXTRA_MAP,
    OBJECT_TYPE_MAP,
    OBJECT_WEAR_MAP,
    RACE_CODE_MAP,
    ROOM_FLAG_MAP,
    SECTOR_MAP,
    TransformResult,
    _source_mask_bits,
    emit_hlquest,
    emit_mobile,
    emit_object,
    emit_room,
    emit_shop,
    emit_zone,
)


ROL_CAPABILITY_AUDIT_SCHEMA_VERSION = 1
_SOURCE_ROOT_PREFIX = "EXAMPLE/RealmsOfLuminari"
_NUMBER = re.compile(r"(?<![A-Za-z_])-?\d+")


class RolCapabilityAuditError(ValueError):
  """Raised when the full-corpus audit cannot produce trustworthy evidence."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _canonical_line(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n").encode(
      "ascii"
  )


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolCapabilityAuditError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolCapabilityAuditError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _sha256_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  path.parent.mkdir(parents=True, exist_ok=True)
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


def _counter_rows(
    family: str,
    observed: Counter[int | str],
    mapped_values: set[int | str],
) -> list[dict[str, Any]]:
  return [
      {
          "family": family,
          "value": value,
          "observations": observations,
          "mapped": value in mapped_values,
      }
      for value, observations in sorted(observed.items(), key=lambda item: str(item[0]))
  ]


def build_symbolic_inventory(records: Iterable[RolRecord]) -> list[dict[str, Any]]:
  """Inventory every active symbolic value consumed by a record transform."""

  counters: dict[str, Counter[int | str]] = defaultdict(Counter)
  quest_shapes: Counter[str] = Counter()
  random_item_ranges = 0

  for record in records:
    if record.kind == "wld":
      base = list(record.values.get("base", []))
      first_mask = int(base[1]) if len(base) > 1 else 0
      second_mask = int(base[6]) if len(base) > 6 else 0
      counters["room_flag"].update(
          _source_mask_bits(first_mask, 1) | _source_mask_bits(second_mask, 33)
      )
      counters["sector"][int(base[2]) if len(base) > 2 else 0] += 1
    elif record.kind == "mob":
      flags = list(record.values.get("flags", []))
      counters["mobile_action_flag"].update(
          _source_mask_bits(int(flags[0]) if flags else 0, 1)
      )
      counters["mobile_affect_flag"].update(
          _source_mask_bits(int(flags[1]) if len(flags) > 1 else 0, 1)
          | _source_mask_bits(int(flags[2]) if len(flags) > 2 else 0, 33)
      )
      rows = list(record.values.get("base_rows", []))
      race_row = rows[0] if rows else ["N"]
      position_row = rows[3] if len(rows) > 3 else []
      counters["mobile_race_code"][str(race_row[0]).upper() if race_row else "N"] += 1
      counters["mobile_class"][int(position_row[3]) if len(position_row) > 3 else 0] += 1
    elif record.kind == "obj":
      counters["object_type"][int(record.values.get("item_type") or 0)] += 1
      flags = list(record.values.get("flags", []))
      counters["object_extra_flag"].update(
          _source_mask_bits(int(flags[1]) if len(flags) > 1 else 0, 0)
      )
      counters["object_wear_flag"].update(
          _source_mask_bits(int(flags[2]) if len(flags) > 2 else 0, 0)
      )
      for directive in record.directives:
        if directive["token"] == "AFFECT_FLAGS":
          for ordinal, mask in enumerate(directive.get("arguments", [])):
            counters["object_affect_flag"].update(
                _source_mask_bits(int(mask), ordinal * 32 + 1)
            )
        elif directive["token"] == "A" and directive.get("arguments"):
          counters["object_apply"][int(directive["arguments"][0])] += 1
    elif record.kind == "qst":
      for directive in record.directives:
        if directive["token"] not in {"G", "R"}:
          continue
        arguments = [int(value) for value in directive.get("arguments", [])]
        shape = f"{directive['token']}:{directive.get('subtype', '')}:{len(arguments)}"
        quest_shapes[shape] += 1
        if (
            directive["token"] == "R"
            and directive.get("subtype") == "I"
            and len(arguments) > 1
            and arguments[1] > arguments[0]
            and arguments[1] - arguments[0] < 100
        ):
          random_item_ranges += 1

  mapped: dict[str, set[int | str]] = {
      "room_flag": set(ROOM_FLAG_MAP),
      "sector": set(SECTOR_MAP),
      "mobile_action_flag": set(MOB_ACTION_MAP),
      "mobile_affect_flag": set(MOB_AFFECT_MAP),
      "mobile_race_code": set(RACE_CODE_MAP),
      "mobile_class": set(CLASS_MAP),
      "object_type": set(OBJECT_TYPE_MAP),
      "object_extra_flag": set(OBJECT_EXTRA_MAP),
      "object_wear_flag": set(OBJECT_WEAR_MAP),
      "object_affect_flag": set(MOB_AFFECT_MAP),
      "object_apply": set(APPLY_MAP),
  }
  rows = [
      row
      for family in sorted(counters)
      for row in _counter_rows(family, counters[family], mapped[family])
  ]
  rows.extend(
      {
          "family": "quest_direction_shape",
          "value": shape,
          "observations": observations,
          "mapped": True,
      }
      for shape, observations in sorted(quest_shapes.items())
  )
  rows.append(
      {
          "family": "quest_random_item_range",
          "value": "R:I:range",
          "observations": random_item_ranges,
          "mapped": random_item_ranges == 0,
      }
  )
  return rows


def classify_transform_diagnostic(message: str) -> str:
  """Classify one transform diagnostic by the owner needed to resolve it."""

  if "no identity for" in message or "unresolved" in message:
    return "reference-gap"
  if "source-inert" in message or "obsolete source" in message:
    return "inert-omission"
  if message.startswith(("mapped source", "converted source", "folded ")):
    return "bounded-adapter"
  if message.startswith(("capped ", "normalized ")):
    return "bounded-normalization"
  if "non-ASCII" in message or "embedded tilde" in message:
    return "text-normalization"
  if "incomplete" in message or "malformed" in message or "without a staged" in message:
    return "source-defect"
  if message.startswith("omitted source-only quest reward"):
    return "source-only-symbol"
  return "generic-capability-gap"


def _emit_record(
    action: dict[str, Any],
    record: RolRecord,
    resolve,
    zones: dict[str, int],
    rooms: dict[str, list[int]],
) -> TransformResult:
  kind = str(action["source_kind"])
  destination = int(action["destination_vnum"])
  basename = str(action["basename"])
  if kind == "mob":
    return emit_mobile(record, destination)
  if kind == "obj":
    return emit_object(record, destination, resolve)
  if kind == "wld":
    return emit_room(record, destination, zones[basename], resolve)
  if kind == "qst":
    return emit_hlquest(record, destination, resolve)
  if kind == "shp":
    return emit_shop(record, destination, resolve)
  if kind == "zon":
    header = list(record.values.get("header", []))
    source_top = int(header[0]) if header else 0

    def zone_resolve(target_kind: str, source_vnum: int) -> int:
      if target_kind == "wld" and source_vnum == source_top:
        try:
          return resolve(target_kind, source_vnum)
        except (KeyError, ValueError, RolPilotBuildError):
          return destination * 100 + (source_vnum - record.vnum * 100)
      return resolve(target_kind, source_vnum)

    return emit_zone(record, destination, min(rooms[basename]), zone_resolve)
  raise RolCapabilityAuditError(f"no full-corpus emitter for source kind {kind}")


def write_capability_audit_bundle(
    plan_dir: Path,
    source_root: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Emit every convertible active record and write a non-mutating audit bundle."""

  repo_root = default_repo_root()
  plan_dir = plan_dir.resolve()
  source_root = source_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolCapabilityAuditError(f"capability-audit output already exists: {output_dir}")
  if source_root != (repo_root / _SOURCE_ROOT_PREFIX).resolve():
    raise RolCapabilityAuditError("capability audit requires the repository RoL source root")
  plan_manifest = _verify_bundle(plan_dir, 2)
  actions = _load_jsonl(plan_dir / "reconciliation.jsonl")
  records, source_diagnostics = _source_records(repo_root, actions)
  resolve = _identity_resolver(plan_dir)
  zones = _target_zone_by_basename(actions)
  rooms: defaultdict[str, list[int]] = defaultdict(list)
  for action in actions:
    if action["source_kind"] == "wld" and action["destination_vnum"] is not None:
      rooms[str(action["basename"])].append(int(action["destination_vnum"]))

  emitted_counts: Counter[str] = Counter()
  disposition_counts: Counter[str] = Counter()
  diagnostic_counts: Counter[str] = Counter()
  diagnostics: list[dict[str, Any]] = []
  exceptions: list[dict[str, Any]] = []
  emitted_bytes = 0
  for action in actions:
    disposition_counts[str(action["action"])] += 1
    if action["source_kind"] == "soc" or action["action"] == "EXCLUDE":
      continue
    record = records[str(action["source_record_id"])]
    try:
      emitted = _emit_record(action, record, resolve, zones, rooms)
      payload = emitted.text.encode("ascii")
    except (KeyError, TypeError, UnicodeError, ValueError) as error:
      exceptions.append(
          {
              "source_record_id": action["source_record_id"],
              "source_kind": action["source_kind"],
              "error_type": type(error).__name__,
              "message": str(error),
          }
      )
      continue
    emitted_counts[str(action["source_kind"])] += 1
    emitted_bytes += len(payload)
    for message in emitted.diagnostics:
      classification = classify_transform_diagnostic(message)
      diagnostic_counts[classification] += 1
      diagnostics.append(
          {
              "source_record_id": action["source_record_id"],
              "source_kind": action["source_kind"],
              "planned_action": action["action"],
              "destination_vnum": action["destination_vnum"],
              "classification": classification,
              "pattern": _NUMBER.sub("N", message),
              "message": message,
          }
      )

  symbolic_rows = build_symbolic_inventory(records.values())
  random_ranges = next(
      row["observations"]
      for row in symbolic_rows
      if row["family"] == "quest_random_item_range"
  )
  unmapped_symbols = sum(
      int(row["observations"])
      for row in symbolic_rows
      if not row["mapped"] and row["family"] != "quest_random_item_range"
  )
  summary = {
      "schema_version": ROL_CAPABILITY_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "plan_run_id": plan_manifest["run_id"],
      "source_records": len(actions),
      "source_diagnostics": len(source_diagnostics),
      "record_dispositions": dict(sorted(disposition_counts.items())),
      "emitted_records": sum(emitted_counts.values()),
      "emitted_records_by_kind": dict(sorted(emitted_counts.items())),
      "emitted_bytes": emitted_bytes,
      "transform_exceptions": len(exceptions),
      "transform_diagnostics": len(diagnostics),
      "diagnostics_by_classification": dict(sorted(diagnostic_counts.items())),
      "symbolic_values": len(symbolic_rows),
      "unmapped_symbol_observations": unmapped_symbols,
      "quest_random_item_ranges": random_ranges,
      "live_target_writes": 0,
  }

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  summary_path = output_dir / "audit-summary.json"
  summary_path.write_bytes(_canonical_json(summary))
  artifacts.append(_artifact(summary_path, output_dir))
  diagnostics_path = output_dir / "transform-diagnostics.jsonl"
  diagnostics_count = _write_jsonl(diagnostics_path, diagnostics)
  artifacts.append(_artifact(diagnostics_path, output_dir, diagnostics_count))
  exceptions_path = output_dir / "transform-exceptions.jsonl"
  exception_count = _write_jsonl(exceptions_path, exceptions)
  artifacts.append(_artifact(exceptions_path, output_dir, exception_count))
  symbols_path = output_dir / "symbolic-values.jsonl"
  symbol_count = _write_jsonl(symbols_path, symbolic_rows)
  artifacts.append(_artifact(symbols_path, output_dir, symbol_count))
  source_path = output_dir / "source-diagnostics.jsonl"
  source_count = _write_jsonl(source_path, source_diagnostics)
  artifacts.append(_artifact(source_path, output_dir, source_count))

  fingerprint_seed = "\n".join(
      artifact["sha256"] for artifact in sorted(artifacts, key=lambda item: item["path"])
  ).encode("ascii")
  run_id = f"rol-phase5-audit-{hashlib.sha256(fingerprint_seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_CAPABILITY_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 5,
      "stage": "full-corpus-capability-audit",
      "plan_run_id": plan_manifest["run_id"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "all_records_disposed": sum(disposition_counts.values()) == len(actions),
          "all_convertible_records_emitted": (
              sum(emitted_counts.values())
              == len(actions) - disposition_counts["EXCLUDE"] - sum(
                  1 for action in actions if action["source_kind"] == "soc"
              )
          ),
          "transform_exceptions": len(exceptions),
          "quest_random_item_ranges": random_ranges,
          "live_target_writes": 0,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {"run_id": run_id, "output_dir": output_dir.as_posix(), **summary}


def render_rol_capability_audit_human(summary: dict[str, Any]) -> str:
  """Render a concise human summary for the audit CLI."""

  return "\n".join(
      (
          f"RoL Phase 5 capability audit: {summary['run_id']}",
          f"Output: {summary['output_dir']}",
          f"Source records: {summary['source_records']}",
          f"Emitted records: {summary['emitted_records']}",
          f"Emitted bytes: {summary['emitted_bytes']}",
          f"Transform exceptions: {summary['transform_exceptions']}",
          f"Transform diagnostics: {summary['transform_diagnostics']}",
          f"Unmapped symbol observations: {summary['unmapped_symbol_observations']}",
          f"Active random item-reward ranges: {summary['quest_random_item_ranges']}",
          "Live target writes: 0",
          "",
      )
  )
