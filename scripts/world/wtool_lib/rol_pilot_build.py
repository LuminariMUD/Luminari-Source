"""Build and stage the complete Realms of Luminari Phase 4 pilot."""

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
from .constants import default_repo_root, load_manifest
from .flags import decode_tokens, encode_bits
from .models import TOOL_VERSION
from .reporting import result_payload
from .rol_discovery import extract_source_commands
from .rol_pilot import PILOT_BASENAMES
from .rol_skeleton import tree_manifest
from .rol_soc import SocCompilation, compile_soc_records
from .rol_source import RolRecord, parse_rol_source_file
from .rol_special import NativeSpecialBinding, SpecialCompilation, compile_special_bindings
from .rol_transform import (
    TransformResult,
    emit_hlquest,
    emit_mobile,
    emit_object,
    emit_room,
    emit_shop,
    emit_zone,
)
from .world import load_indexed_world_data, validate_explicit_paths, validate_indexed_world


ROL_PILOT_BUILD_SCHEMA_VERSION = 1
_SOURCE_ROOT_PREFIX = "EXAMPLE/RealmsOfLuminari"
_TARGET_KIND = {"mob": "mob", "obj": "obj", "wld": "wld"}
_FILE_KIND = {
    "mob": ("mob", "mob"),
    "obj": ("obj", "obj"),
    "qst": ("hlq", "hlq"),
    "shp": ("shp", "shp"),
    "wld": ("wld", "wld"),
    "zon": ("zon", "zon"),
}
_NEW_TRIGGER_ZONES = frozenset({20261, 20409, 20553, 20586})


class RolPilotBuildError(ValueError):
  """Raised when the Phase 4 pilot cannot be built without clobbering data."""


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
    raise RolPilotBuildError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolPilotBuildError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _load_json(path: Path) -> Any:
  try:
    return json.loads(path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolPilotBuildError(f"cannot read pilot input {path}: {error}") from error


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line)
        except json.JSONDecodeError as error:
          raise RolPilotBuildError(
              f"invalid JSONL at {path}:{line_number}: {error}"
          ) from error
        if not isinstance(row, dict):
          raise RolPilotBuildError(f"JSONL row at {path}:{line_number} is not an object")
        rows.append(row)
  except OSError as error:
    raise RolPilotBuildError(f"cannot read pilot input {path}: {error}") from error
  return rows


def _verify_bundle(path: Path, phase: int, stage: str | None = None) -> dict[str, Any]:
  manifest = _load_json(path / "run-manifest.json")
  if manifest.get("phase") != phase:
    raise RolPilotBuildError(f"input {path} is not a Phase {phase} bundle")
  if stage is not None and manifest.get("stage") != stage:
    raise RolPilotBuildError(f"input {path} is not the Phase {phase} {stage} bundle")
  for artifact in manifest.get("artifacts", []):
    artifact_path = path / artifact["path"]
    if not artifact_path.is_file():
      raise RolPilotBuildError(f"input artifact is missing: {artifact['path']}")
    if _sha256_path(artifact_path) != artifact["sha256"]:
      raise RolPilotBuildError(f"input artifact hash mismatch: {artifact['path']}")
  return manifest


def _source_records(
    repo_root: Path,
    actions: Iterable[dict[str, Any]],
) -> tuple[dict[str, RolRecord], list[dict[str, Any]]]:
  requested = sorted(
      {
          (str(action["source_path"]), str(action["source_kind"]), str(action["basename"]))
          for action in actions
      }
  )
  records: dict[str, RolRecord] = {}
  diagnostics: list[dict[str, Any]] = []
  for relative, kind, basename in requested:
    source_path = repo_root / _SOURCE_ROOT_PREFIX / relative
    parsed, corpus = parse_rol_source_file(source_path, relative, kind, basename)
    for record in parsed:
      records[record.record_id] = record
    diagnostics.extend(item.to_dict() for item in corpus.diagnostics)
  missing = sorted(
      action["source_record_id"]
      for action in actions
      if action["source_record_id"] not in records
  )
  if missing:
    raise RolPilotBuildError(f"selected source record could not be reparsed: {missing[0]}")
  return records, diagnostics


def _identity_resolver(plan_dir: Path):
  rows = _load_jsonl(plan_dir / "identity-map.jsonl")
  identities = {
      (str(row["source_kind"]), int(row["source_vnum"])): row["destination_vnum"]
      for row in rows
  }
  # The source room is reference-only in the active corpus; target 196003 is the
  # confirmed zone-960 lineage room already present in the development world.
  identities[("wld", 96003)] = 196003

  def resolve(kind: str, vnum: int) -> int:
    try:
      destination = identities[(kind, vnum)]
    except KeyError as error:
      raise RolPilotBuildError(f"no typed identity for {kind} {vnum}") from error
    if destination is None:
      raise RolPilotBuildError(f"required identity {kind} {vnum} is excluded")
    return int(destination)

  return resolve


def _combine_soc(
    records: Iterable[RolRecord],
    resolve,
    source_commands: dict[int, str],
) -> SocCompilation:
  by_basename: defaultdict[str, list[RolRecord]] = defaultdict(list)
  for record in records:
    by_basename[record.basename].append(record)

  allocations: dict[str, Iterable[int]] = {
      "hulburg": range(2_026_100, 2_026_150),
      "theswamp": range(2_040_900, 2_041_000),
      "cemetery": range(2_055_300, 2_055_400),
      "muspel": (
          *range(2_058_600, 2_058_700),
          *range(2_026_150, 2_026_250),
      ),
  }
  compilations: list[SocCompilation] = []
  for basename in sorted(by_basename):
    compilation = compile_soc_records(
        by_basename[basename],
        0,
        resolve,
        source_commands,
        trigger_vnums=allocations[basename],
    )
    compilations.append(compilation)

  triggers = [trigger for compilation in compilations for trigger in compilation.triggers]
  if len({trigger.vnum for trigger in triggers}) != len(triggers):
    raise RolPilotBuildError("SOC trigger VNUM allocation contains a collision")
  attachments: defaultdict[int, list[int]] = defaultdict(list)
  for compilation in compilations:
    for owner, trigger_vnums in compilation.attachments.items():
      attachments[owner].extend(trigger_vnums)
  return SocCompilation(
      triggers=sorted(triggers, key=lambda item: item.vnum),
      attachments={key: sorted(value) for key, value in sorted(attachments.items())},
      diagnostics=[item for compilation in compilations for item in compilation.diagnostics],
      source_records=sum(item.source_records for item in compilations),
      source_actions=sum(item.source_actions for item in compilations),
  )


def _artifact(path: Path, output_dir: Path, records: int | None = None) -> dict[str, Any]:
  item: dict[str, Any] = {
      "path": path.relative_to(output_dir).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }
  if records is not None:
    item["records"] = records
  return item


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  path.parent.mkdir(parents=True, exist_ok=True)
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _file_payload(kind: str, records: list[tuple[int, str]]) -> str:
  ordered = "".join(text for _, text in sorted(records))
  if kind == "shp":
    return "CircleMUD v3.0 Shop File~\n" + ordered + "$~\n"
  if kind == "zon":
    return ordered
  return ordered + "$~\n"


def _decode_first_flag(token: str) -> set[int]:
  decoded = decode_tokens([token], serialized_chunks=1)
  if decoded.issues:
    raise RolPilotBuildError(f"cannot decode existing mobile action flag {token!r}")
  return set(decoded.bits)


def _patch_mobile_block(
    block: list[str],
    vnum: int,
    special_proc: str | None,
    attachments: Iterable[int],
) -> list[str]:
  result = list(block)
  if special_proc is not None:
    flag_index = next(
        (
            index
            for index, line in enumerate(result)
            if len(line.strip().split()) >= 10 and line.strip().split()[-1] == "E"
        ),
        None,
    )
    if flag_index is None:
      raise RolPilotBuildError(f"mobile {vnum} has no enhanced action-flag row")
    tokens = result[flag_index].strip().split()
    first_flags = _decode_first_flag(tokens[0])
    first_flags.add(0)
    tokens[0] = encode_bits(first_flags, serialized_chunks=1)[0]
    result[flag_index] = " ".join(tokens) + "\n"

    existing = [
        line.partition(":")[2].strip()
        for line in result
        if line.startswith("SpecProc:")
    ]
    if existing and existing != [special_proc]:
      raise RolPilotBuildError(
          f"mobile {vnum} already has incompatible SpecProc {existing[0]!r}"
      )
    if not existing:
      end_index = max(index for index, line in enumerate(result) if line.strip() == "E")
      result.insert(end_index, f"SpecProc: {special_proc}\n")

  existing_attachments = {
      int(match.group(1))
      for line in result
      if (match := re.fullmatch(r"T\s+(\d+)\s*", line)) is not None
  }
  new_attachments = sorted(set(attachments) - existing_attachments)
  if new_attachments:
    insert_at = len(result)
    result[insert_at:insert_at] = [f"T {trigger_vnum}\n" for trigger_vnum in new_attachments]
  return result


def _patch_mobile_file(
    path: Path,
    patches: dict[int, tuple[str | None, tuple[int, ...]]],
) -> None:
  lines = path.read_text(encoding="ascii").splitlines(keepends=True)
  headers = [
      (index, int(match.group(1)))
      for index, line in enumerate(lines)
      if (match := re.fullmatch(r"#(\d+)\s*\n?", line)) is not None
  ]
  bounds: dict[int, tuple[int, int]] = {}
  for offset, (start, vnum) in enumerate(headers):
    end = headers[offset + 1][0] if offset + 1 < len(headers) else len(lines)
    while end > start and lines[end - 1].lstrip().startswith("$"):
      end -= 1
    bounds[vnum] = (start, end)
  missing = sorted(set(patches) - bounds.keys())
  if missing:
    raise RolPilotBuildError(f"mobile patch target {missing[0]} is absent from {path}")
  for vnum in sorted(patches, key=lambda item: bounds[item][0], reverse=True):
    start, end = bounds[vnum]
    special_proc, attachments = patches[vnum]
    lines[start:end] = _patch_mobile_block(
        lines[start:end], vnum, special_proc, attachments
    )
  path.write_text("".join(lines), encoding="ascii", newline="\n")


def _append_shop_records(path: Path, records: str) -> None:
  original = path.read_text(encoding="ascii")
  stripped = original.strip()
  if stripped == "$~":
    merged = "CircleMUD v3.0 Shop File~\n" + records + "$~\n"
  else:
    marker = original.rfind("$~")
    if marker < 0:
      raise RolPilotBuildError(f"shop file lacks final marker: {path}")
    merged = original[:marker].rstrip() + "\n" + records + "$~\n"
  path.write_text(merged, encoding="ascii", newline="\n")


def _update_index(path: Path, filenames: Iterable[str]) -> None:
  entries = [line.strip() for line in path.read_text(encoding="ascii").splitlines()]
  entries = [entry for entry in entries if entry and entry != "$"]
  entries.extend(name for name in filenames if name not in entries)

  def sort_key(name: str) -> tuple[int, str]:
    stem = name.partition(".")[0]
    return (int(stem) if stem.isdigit() else 2**63 - 1, name)

  path.write_text(
      "".join(f"{entry}\n" for entry in sorted(set(entries), key=sort_key)) + "$\n",
      encoding="ascii",
      newline="\n",
  )


def _stage_overlay(
    target_world: Path,
    staging_world: Path,
    generated_files: dict[str, bytes],
    mobile_patches: dict[str, dict[int, tuple[str | None, tuple[int, ...]]]],
    shop_appends: dict[str, str],
) -> dict[str, Any]:
  shutil.copytree(target_world, staging_world, copy_function=shutil.copy2)
  indexed: defaultdict[str, list[str]] = defaultdict(list)
  for relative, payload in sorted(generated_files.items()):
    target = staging_world / relative
    if target.exists():
      raise RolPilotBuildError(f"generated ADD would overwrite existing target {relative}")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(payload)
    kind, filename = relative.split("/", 1)
    indexed[kind].append(filename)
  for relative, patches in sorted(mobile_patches.items()):
    _patch_mobile_file(staging_world / relative, patches)
  for relative, records in sorted(shop_appends.items()):
    _append_shop_records(staging_world / relative, records)
  for kind, filenames in sorted(indexed.items()):
    _update_index(staging_world / kind / "index", filenames)
  return {
      "new_files": len(generated_files),
      "patched_mobile_files": len(mobile_patches),
      "appended_shop_files": len(shop_appends),
      "indexed_files": sum(len(items) for items in indexed.values()),
  }


def _native_maps(
    compilation: SpecialCompilation,
) -> tuple[
    dict[tuple[str, int], NativeSpecialBinding],
    dict[tuple[str, int], list[int]],
]:
  native = {
      (binding.target_kind, binding.target_vnum): binding
      for binding in compilation.native_bindings
  }
  return native, compilation.attachments


def _target_zone_by_basename(actions: Iterable[dict[str, Any]]) -> dict[str, int]:
  result: dict[str, int] = {}
  for action in actions:
    if action["source_kind"] != "zon":
      continue
    result[str(action["basename"])] = int(action["destination_vnum"])
  return result


def write_pilot_build_bundle(
    selection_dir: Path,
    plan_dir: Path,
    source_root: Path,
    target_world: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Build every Phase 4 pilot action and validate it in a no-clobber stage."""

  repo_root = default_repo_root()
  selection_dir = selection_dir.resolve()
  plan_dir = plan_dir.resolve()
  source_root = source_root.resolve()
  target_world = target_world.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPilotBuildError(f"pilot build output directory already exists: {output_dir}")
  if source_root != (repo_root / _SOURCE_ROOT_PREFIX).resolve():
    raise RolPilotBuildError("Phase 4 build requires the inventoried repository RoL source root")
  if not target_world.is_dir():
    raise RolPilotBuildError(f"target world root is inaccessible: {target_world}")

  selection_manifest = _verify_bundle(selection_dir, 4, "selection")
  plan_manifest = _verify_bundle(plan_dir, 2)
  if selection_manifest.get("plan_run_id") != plan_manifest.get("run_id"):
    raise RolPilotBuildError("selection and identity plan do not belong to the same run")

  actions = _load_jsonl(selection_dir / "pilot-actions.jsonl")
  binding_rows = _load_jsonl(selection_dir / "pilot-special-bindings.jsonl")
  if len(actions) != 3001:
    raise RolPilotBuildError(f"expected 3001 selected actions, found {len(actions)}")
  source_records, source_diagnostics = _source_records(repo_root, actions)
  resolve = _identity_resolver(plan_dir)
  manifest = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  validation_config = resolve_config(target_world, None)
  target_model = load_indexed_world_data(
      target_world,
      repo_root,
      manifest,
      validation_config,
  )
  available = {
      "mob": {record.vnum for record in target_model.mobiles},
      "obj": {record.vnum for record in target_model.objects},
      "wld": {record.vnum for record in target_model.rooms},
  }
  for action in actions:
    kind = str(action["source_kind"])
    destination = action["destination_vnum"]
    if kind in available and destination is not None and action["action"] != "EXCLUDE":
      available[kind].add(int(destination))

  def stage_resolve(kind: str, vnum: int) -> int:
    destination = resolve(kind, vnum)
    if destination not in available[kind]:
      raise RolPilotBuildError(
          f"{kind} {vnum} maps to {destination}, which is outside the pilot stage"
      )
    return destination

  selected_records = [source_records[action["source_record_id"]] for action in actions]
  room_records = [record for record in selected_records if record.kind == "wld"]
  soc_records = [record for record in selected_records if record.kind == "soc"]

  command_evidence = extract_source_commands(source_root)
  source_commands = {
      int(row["action_code"]): str(row["command"])
      for row in command_evidence["commands"]
  }
  soc = _combine_soc(soc_records, stage_resolve, source_commands)
  muspel_overflow = sum(trigger.vnum // 100 == 20261 for trigger in soc.triggers) - 50
  if muspel_overflow != 17:
    raise RolPilotBuildError(
        f"measured Muspel SOC overflow changed from 17 to {muspel_overflow}"
    )
  specials = compile_special_bindings(
      binding_rows,
      2_026_167,
      stage_resolve,
      room_records,
  )
  all_triggers = sorted([*soc.triggers, *specials.triggers], key=lambda item: item.vnum)
  if len(all_triggers) != 194 or len({item.vnum for item in all_triggers}) != 194:
    raise RolPilotBuildError("pilot must emit 194 unique DG triggers")
  if {item.vnum // 100 for item in all_triggers} - _NEW_TRIGGER_ZONES:
    raise RolPilotBuildError("pilot trigger allocation escaped active ADD zones")

  attachments: defaultdict[tuple[str, int], list[int]] = defaultdict(list)
  for owner, trigger_vnums in soc.attachments.items():
    attachments[("mob", owner)].extend(trigger_vnums)
  for owner, trigger_vnums in specials.attachments.items():
    attachments[owner].extend(trigger_vnums)
  native, _ = _native_maps(specials)
  zone_by_basename = _target_zone_by_basename(actions)
  generated: defaultdict[tuple[str, int], list[tuple[int, str]]] = defaultdict(list)
  shop_appends: defaultdict[str, list[tuple[int, str]]] = defaultdict(list)
  diagnostics: list[dict[str, Any]] = []
  generated_record_ids: dict[str, str] = {}

  for action in actions:
    if action["action"] != "ADD" or action["source_kind"] == "soc":
      continue
    kind = str(action["source_kind"])
    if kind == "obj" and action["destination_vnum"] is None:
      continue
    destination = int(action["destination_vnum"])
    record = source_records[action["source_record_id"]]
    binding = native.get((_TARGET_KIND.get(kind, kind), destination))
    owner_attachments = tuple(sorted(attachments.get((_TARGET_KIND.get(kind, kind), destination), [])))
    emitted: TransformResult
    if kind == "mob":
      emitted = emit_mobile(
          record,
          destination,
          special_proc=binding.persisted_name if binding is not None else None,
          attachments=owner_attachments,
      )
    elif kind == "obj":
      emitted = emit_object(
          record,
          destination,
          stage_resolve,
          special_proc=binding.persisted_name if binding is not None else None,
          attachments=owner_attachments,
          required_extra_bits=binding.required_flag_bits if binding is not None else (),
      )
    elif kind == "wld":
      emitted = emit_room(
          record,
          destination,
          zone_by_basename[str(action["basename"])],
          stage_resolve,
          attachments=owner_attachments,
      )
    elif kind == "zon":
      room_destinations = [
          int(row["destination_vnum"])
          for row in actions
          if row["basename"] == action["basename"]
          and row["source_kind"] == "wld"
          and row["destination_vnum"] is not None
      ]
      source_header = list(record.values.get("header", []))
      source_top = int(source_header[0])

      def zone_resolve(target_kind: str, source_vnum: int) -> int:
        if target_kind == "wld" and source_vnum == source_top:
          try:
            return stage_resolve(target_kind, source_vnum)
          except RolPilotBuildError:
            return destination * 100 + (source_vnum - record.vnum * 100)
        return stage_resolve(target_kind, source_vnum)

      emitted = emit_zone(
          record, destination, min(room_destinations), zone_resolve
      )
    elif kind == "qst":
      emitted = emit_hlquest(record, destination, stage_resolve)
    elif kind == "shp":
      emitted = emit_shop(record, destination, stage_resolve)
    else:
      raise RolPilotBuildError(f"no pilot emitter for source kind {kind}")

    target_kind, extension = _FILE_KIND[kind]
    zone = zone_by_basename[str(action["basename"])]
    relative = f"{target_kind}/{zone}.{extension}"
    if kind == "shp" and (target_world / relative).exists():
      shop_appends[relative].append((destination, emitted.text))
    else:
      generated[(target_kind, zone)].append((destination, emitted.text))
    generated_record_ids[str(action["source_record_id"])] = relative
    diagnostics.extend(
        {
            "source_record_id": action["source_record_id"],
            "target": relative,
            "message": item,
        }
        for item in emitted.diagnostics
    )

  for trigger in all_triggers:
    generated[("trg", trigger.vnum // 100)].append((trigger.vnum, trigger.text))

  generated_files: dict[str, bytes] = {}
  for (kind, zone), records in sorted(generated.items()):
    extension = kind
    relative = f"{kind}/{zone}.{extension}"
    generated_files[relative] = _file_payload(kind, records).encode("ascii")

  mobile_patches_by_file: defaultdict[
      str, dict[int, tuple[str | None, tuple[int, ...]]]
  ] = defaultdict(dict)
  action_by_target = {
      (str(action["source_kind"]), action["destination_vnum"]): action
      for action in actions
      if action["destination_vnum"] is not None
  }
  patched_mobile_targets = {
      target_vnum
      for (kind, target_vnum) in attachments
      if kind == "mob"
  } | {
      target_vnum for kind, target_vnum in native if kind == "mob"
  }
  added_mobile_targets = {
      int(action["destination_vnum"])
      for action in actions
      if action["source_kind"] == "mob" and action["action"] == "ADD"
  }
  for target_vnum in sorted(patched_mobile_targets - added_mobile_targets):
    action = action_by_target.get(("mob", target_vnum))
    if action is None or action.get("selected_target") is None:
      raise RolPilotBuildError(f"no preserved target evidence for mobile patch {target_vnum}")
    relative = str(action["selected_target"]["path"])
    binding = native.get(("mob", target_vnum))
    mobile_patches_by_file[relative][target_vnum] = (
        binding.persisted_name if binding is not None else None,
        tuple(sorted(attachments.get(("mob", target_vnum), []))),
    )

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  for relative, payload in sorted(generated_files.items()):
    path = output_dir / "output/world" / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(payload)
    key = (relative.split("/")[0], int(Path(relative).stem))
    artifacts.append(_artifact(path, output_dir, len(generated[key])))

  shop_append_text = {
      relative: "".join(text for _, text in sorted(records))
      for relative, records in shop_appends.items()
  }
  for relative, text in sorted(shop_append_text.items()):
    path = output_dir / "output/appends" / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="ascii", newline="\n")
    artifacts.append(_artifact(path, output_dir, len(shop_appends[relative])))

  generated_validation = validate_explicit_paths(
      [output_dir / "output/world" / relative for relative in sorted(generated_files)],
      repo_root,
      manifest,
      validation_config,
  )
  generated_validation_data = result_payload(generated_validation)
  generated_validation_data["root"] = "phase4-generated-overlay"

  staging_world = output_dir / "staging/world"
  stage_apply = _stage_overlay(
      target_world,
      staging_world,
      generated_files,
      dict(mobile_patches_by_file),
      shop_append_text,
  )
  selected_zones = {1591, 20261, 20409, 20553, 20586}
  target_validation = validate_indexed_world(
      target_world,
      repo_root,
      manifest,
      validation_config,
      selected_packages=selected_zones,
  )
  target_validation_data = result_payload(target_validation)
  target_validation_data["root"] = "authoritative-target-before-pilot"
  validation = validate_indexed_world(
      staging_world,
      repo_root,
      manifest,
      validation_config,
      selected_packages=selected_zones,
  )
  validation_data = result_payload(validation)
  validation_data["root"] = "phase4-pilot-stage"
  validation_data["command"] = (
      "python3 scripts/world/wtool.py --world-root <phase4-pilot-stage> "
      "--json validate --zone 1591 20261 20409 20553 20586"
  )
  before_findings = Counter(
      json.dumps(item, ensure_ascii=True, sort_keys=True)
      for item in target_validation_data["findings"]
  )
  staged_findings = Counter(
      json.dumps(item, ensure_ascii=True, sort_keys=True)
      for item in validation_data["findings"]
  )
  new_findings = [json.loads(item) for item in (staged_findings - before_findings).elements()]
  resolved_findings = [
      json.loads(item) for item in (before_findings - staged_findings).elements()
  ]
  new_active_errors = sum(
      item["severity"] == "error" and not item.get("suppressed", False)
      for item in new_findings
  )
  validation_delta = {
      "new_findings": sorted(
          new_findings,
          key=lambda item: (
              item["severity"],
              item["code"],
              item.get("span", {}).get("path", ""),
              item.get("span", {}).get("line", 0),
              item["message"],
          ),
      ),
      "resolved_findings": sorted(
          resolved_findings,
          key=lambda item: (
              item["severity"],
              item["code"],
              item.get("span", {}).get("path", ""),
              item.get("span", {}).get("line", 0),
              item["message"],
          ),
      ),
      "new_active_errors": new_active_errors,
      "baseline_complete": target_validation_data["complete"],
      "staged_complete": validation_data["complete"],
      "note": "Completeness includes pre-existing parse failures in unselected indexed files.",
  }
  staged_tree = tree_manifest(staging_world)

  patched_targets = {
      target
      for patches in mobile_patches_by_file.values()
      for target in patches
  }
  dispositions: list[dict[str, Any]] = []
  special_disposition = {
      (row["source_record_type"], row["source_vnum"]): row
      for row in specials.dispositions
  }
  record_type_for_kind = {"mob": "mobile", "obj": "object", "wld": "room"}
  for action in actions:
    destination = action["destination_vnum"]
    kind = str(action["source_kind"])
    special_row = special_disposition.get(
        (record_type_for_kind.get(kind, ""), int(action["source_vnum"]))
    )
    if action["action"] == "EXCLUDE":
      strategy = "EXCLUDE_LOCKED_MALFORMED"
    elif kind == "soc":
      strategy = "DG_COMPILED"
    elif destination in patched_targets:
      strategy = "PATCH_PRESERVED_TARGET"
    elif action["action"] == "ADD":
      strategy = "GENERATED_ADD"
    else:
      strategy = "KEEP_UNCHANGED"
    row = {
        "source_record_id": action["source_record_id"],
        "source_kind": kind,
        "source_vnum": action["source_vnum"],
        "destination_vnum": destination,
        "planned_action": action["action"],
        "strategy": strategy,
    }
    if action["source_record_id"] in generated_record_ids:
      row["output"] = generated_record_ids[action["source_record_id"]]
    if special_row is not None:
      row["special_strategy"] = special_row["strategy"]
    dispositions.append(row)

  evidence_payloads = {
      "change-plan.json": {
          "selected_actions": len(actions),
          "generated_files": sorted(generated_files),
          "mobile_patches": {
              relative: sorted(patches)
              for relative, patches in sorted(mobile_patches_by_file.items())
          },
          "shop_appends": {
              relative: len(records) for relative, records in sorted(shop_appends.items())
          },
          "stage_apply": stage_apply,
          "live_target_writes": 0,
      },
      "compiler-summary.json": {
          "source_records": len(actions),
          "source_diagnostics": len(source_diagnostics),
          "transform_diagnostics": len(diagnostics),
          "soc": {
              "source_records": soc.source_records,
              "source_actions": soc.source_actions,
              "triggers": len(soc.triggers),
              "attached_mobiles": len(soc.attachments),
          },
          "specials": {
              "source_bindings": specials.source_bindings,
              "native_bindings": len(specials.native_bindings),
              "dg_bindings": specials.source_bindings - len(specials.native_bindings),
              "triggers": len(specials.triggers),
          },
      },
      "diagnostics/source.json": source_diagnostics,
      "diagnostics/transforms.json": diagnostics,
      "diagnostics/soc.json": soc.diagnostics,
      "diagnostics/specials.json": specials.diagnostics,
      "validation/generated-structure.json": generated_validation_data,
      "validation/target-before.json": target_validation_data,
      "validation/staged.json": validation_data,
      "validation/delta.json": validation_delta,
      "validation/staged-tree.json": staged_tree,
  }
  for relative, payload in evidence_payloads.items():
    path = output_dir / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  disposition_path = output_dir / "dispositions.jsonl"
  disposition_count = _write_jsonl(disposition_path, dispositions)
  artifacts.append(_artifact(disposition_path, output_dir, disposition_count))
  special_path = output_dir / "special-dispositions.jsonl"
  special_count = _write_jsonl(special_path, specials.dispositions)
  artifacts.append(_artifact(special_path, output_dir, special_count))

  seed = "\n".join(
      artifact["sha256"]
      for artifact in sorted(artifacts, key=lambda item: item["path"])
      if not artifact["path"].startswith("validation/")
  ).encode("ascii")
  fingerprint = hashlib.sha256(seed).hexdigest()
  run_id = f"rol-phase4-build-{fingerprint[:16]}"
  active_errors = validation_data["summary"]["active"]["by_severity"]["error"]
  manifest = {
      "schema_version": ROL_PILOT_BUILD_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 4,
      "stage": "build-and-stage",
      "selection_run_id": selection_manifest["run_id"],
      "plan_run_id": plan_manifest["run_id"],
      "selected_basenames": list(PILOT_BASENAMES),
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "selected_actions": len(actions),
          "all_selected_actions_disposed": len(dispositions) == len(actions),
          "soc_source_records": soc.source_records,
          "soc_triggers": len(soc.triggers),
          "special_source_bindings": specials.source_bindings,
          "native_special_bindings": len(specials.native_bindings),
          "dg_special_bindings": specials.source_bindings - len(specials.native_bindings),
          "special_triggers": len(specials.triggers),
          "generated_dg_triggers": len(all_triggers),
          "generated_parse_complete": generated_validation_data["complete"],
          "staged_parse_complete": validation_data["complete"],
          "staged_active_errors": active_errors,
          "staged_new_active_errors": new_active_errors,
          "staged_without_new_errors": new_active_errors == 0,
          "no_implicit_overwrite": True,
          "live_target_writes": 0,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "selected_actions": len(actions),
      "generated_files": len(generated_files),
      "patched_mobiles": len(patched_targets),
      "appended_shops": sum(len(items) for items in shop_appends.values()),
      "soc_triggers": len(soc.triggers),
      "special_triggers": len(specials.triggers),
      "native_special_bindings": len(specials.native_bindings),
      "staged_parse_complete": validation_data["complete"],
      "generated_parse_complete": generated_validation_data["complete"],
      "staged_errors": active_errors,
      "staged_new_errors": new_active_errors,
      "artifacts": len(artifacts) + 1,
  }


def render_rol_pilot_build_human(summary: dict[str, Any]) -> str:
  lines = [
      f"RoL Phase 4 pilot build: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Selected actions: {summary['selected_actions']}",
      f"Generated files: {summary['generated_files']}",
      f"Patched preserved mobiles in stage: {summary['patched_mobiles']}",
      f"Appended staged shops: {summary['appended_shops']}",
      f"SOC DG triggers: {summary['soc_triggers']}",
      f"Special DG triggers: {summary['special_triggers']}",
      f"Native special bindings: {summary['native_special_bindings']}",
      f"Generated parse complete: {str(summary['generated_parse_complete']).lower()}",
      f"Staged parse complete: {str(summary['staged_parse_complete']).lower()}",
      f"Staged active errors: {summary['staged_errors']}",
      f"Staged new errors: {summary['staged_new_errors']}",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
