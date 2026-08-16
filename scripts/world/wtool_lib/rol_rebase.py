"""Phase 6.5 canonical VNUM rebase staging and application."""

from __future__ import annotations

from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shutil
from typing import Any, Iterable

from .config import resolve_config
from .constants import load_manifest
from .flags import decode_tokens
from .models import TOOL_VERSION
from .quests import parse_quest_file
from .reporting import result_payload
from .rol_identity import canonical_destination
from .rol_pilot_build import _identity_resolver, _source_records
from .rol_planner import verify_discovery_bundle
from .rol_persistence import (
    complete_persistent_bindings,
    persistent_consumer_ledger,
)
from .rol_transform import emit_mobile, emit_object
from .world import load_indexed_world_data, validate_indexed_world


ROL_REBASE_SCHEMA_VERSION = 1
_TOKEN = re.compile(r"(?<![0-9])([0-9]+)(?![0-9])")
_TOKEN_BYTES = re.compile(rb"(?<![0-9])([0-9]+)(?![0-9])")
_HEADER = re.compile(r"^#([0-9]+)$")
_HEADER_BYTES = re.compile(rb"^#([0-9]+)$")
_CORE_PACKAGES = {
    1507: {"name": "trail", "target": 20507, "low": 150700, "high": 150899},
    1591: {"name": "hulburg", "target": 20591, "low": 159100, "high": 159599},
    1960: {"name": "jotun", "target": 20960, "low": 196000, "high": 196299},
}
_ARTIFACT_REHOMES = {
    169901: 2001043,
    169902: 2001044,
    169903: 2001042,
    169904: 2001046,
    169905: 2001050,
    169906: 2001007,
    169907: 2001048,
    169908: 2005343,
    169909: 2001008,
    169910: 2019730,
}
_CANONICAL_ARTIFACTS = frozenset({*_ARTIFACT_REHOMES.values(), 2001009})
_ARTIFACT_OBJECT_PACKAGES = {
    "1699.obj": frozenset({169911, 169913, 169914, 169915, 169916, 169917, 169918}),
    "20010.obj": frozenset(
        {2001007, 2001008, 2001009, 2001042, 2001043, 2001044, 2001046, 2001048, 2001050}
    ),
    "20053.obj": frozenset({2005343}),
    "20197.obj": frozenset({2019730}),
}
_JOTUN_CANONICAL_ADDITIONS = {
    ("mob", 96092): 2096092,
    ("obj", 96092): 2096092,
    ("obj", 96093): 2096093,
    ("obj", 96094): 2096094,
    ("obj", 96095): 2096095,
    ("obj", 96096): 2096096,
}
_CANONICAL_HULBURG_KEYS = frozenset(
    {
        2059147,
        2059156,
        2059160,
        2059168,
        2059176,
        2059184,
        2059196,
        2059202,
        2059239,
        2059241,
        2059244,
        2059279,
        2059280,
        2059316,
        2059364,
    }
)
_CANONICAL_HULBURG_UNHOLY_LIQUIDS = frozenset(
    {2059140, 2059272, 2059273, 2059333, 2059355, 2059362}
)
_MOVED_KINDS = ("zon", "wld", "mob", "obj", "shp", "hlq", "trg")


class RolRebaseError(ValueError):
  """Raised when the canonical rebase cannot be staged or safely applied."""


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


def _tree_hash(root: Path) -> str:
  digest = hashlib.sha256()
  for path in sorted(item for item in root.rglob("*") if item.is_file()):
    relative = path.relative_to(root).as_posix().encode("ascii")
    digest.update(relative + b"\0" + _sha256_path(path).encode("ascii") + b"\n")
  return digest.hexdigest()


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolRebaseError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolRebaseError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def retired_destination(vnum: int) -> int | None:
  """Return the canonical successor for one known retired numeric identity."""

  if vnum in _ARTIFACT_REHOMES:
    return _ARTIFACT_REHOMES[vnum]
  if vnum in _CORE_PACKAGES:
    return int(_CORE_PACKAGES[vnum]["target"])
  for package in _CORE_PACKAGES.values():
    if int(package["low"]) <= vnum <= int(package["high"]):
      return vnum + 1900000
  return None


def _verify_bundle(directory: Path, phase: int | str) -> dict[str, Any]:
  manifest_path = directory / "run-manifest.json"
  try:
    manifest = json.loads(manifest_path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolRebaseError(f"cannot read input manifest {manifest_path}: {error}") from error
  if manifest.get("phase") != phase:
    raise RolRebaseError(f"{directory} is not a Phase {phase} bundle")
  for artifact in manifest.get("artifacts", []):
    path = directory / artifact["path"]
    if not path.is_file() or _sha256_path(path) != artifact["sha256"]:
      raise RolRebaseError(f"input artifact is missing or changed: {path}")
  return manifest


def _load_jsonl(path: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  try:
    for line_number, line in enumerate(path.read_text(encoding="ascii").splitlines(), start=1):
      row = json.loads(line)
      if not isinstance(row, dict):
        raise ValueError("row is not an object")
      rows.append(row)
  except (OSError, UnicodeError, json.JSONDecodeError, ValueError) as error:
    raise RolRebaseError(f"cannot read {path}: {error}") from error
  return rows


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> int:
  count = 0
  with path.open("wb") as output:
    for row in rows:
      output.write(_canonical_line(row))
      count += 1
  return count


def _typed_qst_rewrites(
    world_root: Path, repo_root: Path
) -> dict[tuple[str, int, int], str]:
  manifest = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  rewrites: dict[tuple[str, int, int], str] = {}
  for path in sorted((world_root / "qst").glob("*.qst")):
    relative = path.relative_to(world_root).as_posix()
    parsed = parse_quest_file(path, relative, manifest)
    if not parsed.complete:
      raise RolRebaseError(f"cannot type quest references in {relative}")
    for record in parsed.records:
      for reference in record.references:
        if reference.target_type not in {"room", "mobile", "object"}:
          continue
        if retired_destination(reference.target_vnum) is None:
          continue
        rewrites[(relative, reference.span.line, reference.target_vnum)] = reference.role
  return rewrites


def _world_role(
    relative: str,
    line_number: int,
    line: str,
    vnum: int,
    qst_rewrites: dict[tuple[str, int, int], str],
) -> tuple[str, str]:
  kind = Path(relative).suffix.removeprefix(".")
  stem = Path(relative).stem
  if vnum in _CORE_PACKAGES:
    if kind == "zon" and stem == str(vnum) and line.startswith(f"#{vnum}"):
      return "rewrite", "owned legacy zone header"
    if (
        kind == "wld"
        and stem == str(vnum)
        and re.match(rf"^{vnum}(?:\s|$)", line) is not None
    ):
      return "rewrite", "owned legacy room zone field"
    return "retain", f"target-owned {kind} identity or untyped numeric match"
  if kind == "qst":
    role = qst_rewrites.get((relative, line_number, vnum))
    if role is not None:
      return "rewrite", role
    if line.startswith("#"):
      return "retain", "target-owned automated quest identity"
    return "retain", "target-owned automated quest field"
  if stem.isdigit() and int(stem) in _CORE_PACKAGES:
    return "rewrite", f"typed {kind} field in owned legacy package"
  if kind == "zon" and (line.startswith("#") or re.match(r"^[A-Z] [+-]?[0-9]", line)):
    return "rewrite", "typed zone header, range, or reset field"
  if kind == "wld" and (line.startswith("#") or re.fullmatch(r"[ 0-9+-]+", line)):
    return "rewrite", "typed room header, metadata, exit, or connection field"
  if kind in {"mob", "obj", "shp", "hlq"}:
    return "rewrite", f"typed {kind} definition or reference field"
  if kind == "trg" and (
      line.startswith("#")
      or re.search(r"\b(?:set\s+jotunheim|goto|at|teleport|load)\b", line, re.IGNORECASE)
  ):
    return "rewrite", "owned DG trigger identity or typed body literal"
  raise RolRebaseError(
      f"unclassified retired VNUM {vnum} at {relative}:{line_number}"
  )


def _rewrite_world(
    world_root: Path, repo_root: Path
) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
  qst_rewrites = _typed_qst_rewrites(world_root, repo_root)
  rewrites: list[dict[str, Any]] = []
  retained: list[dict[str, Any]] = []
  for directory in ("zon", "wld", "mob", "obj", "shp", "qst", "hlq", "trg"):
    for path in sorted((world_root / directory).glob(f"*.{directory}")):
      relative = path.relative_to(world_root).as_posix()
      output: list[bytes] = []
      changed = False
      for line_number, raw_line in enumerate(path.read_bytes().splitlines(keepends=True), start=1):
        content = raw_line.rstrip(b"\r\n")
        ending = raw_line[len(content) :]
        line = content.decode("latin-1")

        def replace(match: re.Match[bytes]) -> bytes:
          nonlocal changed
          source_vnum = int(match.group(1).decode("ascii"))
          target_vnum = retired_destination(source_vnum)
          if target_vnum is None:
            return match.group(0)
          action, role = _world_role(
              relative, line_number, line, source_vnum, qst_rewrites
          )
          row = {
              "path": relative,
              "line": line_number,
              "source_vnum": source_vnum,
              "target_vnum": target_vnum,
              "role": role,
              "action": action,
          }
          if action == "retain":
            retained.append(row)
            return match.group(0)
          rewrites.append(row)
          changed = True
          return str(target_vnum).encode("ascii")

        output.append(_TOKEN_BYTES.sub(replace, content) + ending)
      if changed:
        path.write_bytes(b"".join(output))
  return rewrites, retained


def _write_index(path: Path, remove: set[str], add: set[str]) -> None:
  entries = []
  if path.is_file():
    entries = [line for line in path.read_text(encoding="ascii").splitlines() if line != "$"]
  selected = {entry for entry in entries if entry and entry not in remove}
  selected.update(add)

  def key(value: str) -> tuple[int, str]:
    stem = Path(value).stem
    return (int(stem) if stem.isdigit() else 999999999, value)

  path.write_bytes(("\n".join(sorted(selected, key=key)) + "\n$\n").encode("ascii"))


def _move_core_packages(world_root: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  for source_zone, package in sorted(_CORE_PACKAGES.items()):
    target_zone = int(package["target"])
    for kind in _MOVED_KINDS:
      source_name = f"{source_zone}.{kind}"
      target_name = f"{target_zone}.{kind}"
      source = world_root / kind / source_name
      target = world_root / kind / target_name
      if source.is_file():
        if target.exists() and target.read_bytes() != source.read_bytes():
          raise RolRebaseError(
              f"canonical destination already has different content: {target}"
          )
        if not target.exists():
          source.replace(target)
        else:
          source.unlink()
        rows.append(
            {
                "operation": "REHOME",
                "record_kind": kind,
                "source_path": f"{kind}/{source_name}",
                "target_path": f"{kind}/{target_name}",
                "package": package["name"],
            }
        )
      additions = {target_name} if target.exists() else set()
      _write_index(world_root / kind / "index", {source_name}, additions)

  for source_zone in _CORE_PACKAGES:
    path = world_root / "qst" / f"{source_zone}.qst"
    if path.is_file() and path.read_text(encoding="ascii").strip() == "$~":
      path.unlink()
      _write_index(world_root / "qst/index", {f"{source_zone}.qst"}, set())
      rows.append(
          {
              "operation": "REMOVE_EMPTY_TARGET_PACKAGE",
              "record_kind": "qst",
              "source_path": f"qst/{source_zone}.qst",
              "target_path": None,
          }
      )
  return rows


def _object_blocks(path: Path) -> dict[int, list[str]]:
  if not path.is_file():
    return {}
  lines = path.read_text(encoding="ascii").splitlines()
  blocks: dict[int, list[str]] = {}
  current: int | None = None
  for line in lines:
    match = _HEADER.fullmatch(line)
    if match is not None:
      current = int(match.group(1))
      if current in blocks:
        raise RolRebaseError(f"duplicate object {current} in {path}")
      blocks[current] = [line]
    elif line == "$~":
      current = None
    elif current is not None:
      blocks[current].append(line)
  return blocks


def _write_object_blocks(path: Path, blocks: dict[int, list[str]]) -> None:
  output: list[str] = []
  for vnum in sorted(blocks):
    output.extend(blocks[vnum])
  output.append("$~")
  path.write_bytes(("\n".join(output) + "\n").encode("ascii"))


def _insert_definition(path: Path, vnum: int, text: str, terminator: str) -> None:
  lines = path.read_text(encoding="ascii").splitlines()
  blocks: dict[int, list[str]] = {}
  current: int | None = None
  for line in lines:
    match = _HEADER.fullmatch(line)
    if match is not None:
      current = int(match.group(1))
      if current in blocks:
        raise RolRebaseError(f"duplicate definition {current} in {path}")
      blocks[current] = [line]
    elif line == terminator:
      current = None
    elif current is not None:
      blocks[current].append(line)
  if vnum in blocks:
    raise RolRebaseError(f"canonical addition {vnum} already exists in {path}")
  emitted = text.splitlines()
  if not emitted or emitted[0] != f"#{vnum}":
    raise RolRebaseError(f"canonical addition emitter returned the wrong identity for {vnum}")
  blocks[vnum] = emitted
  output = [line for key in sorted(blocks) for line in blocks[key]]
  output.append(terminator)
  path.write_bytes(("\n".join(output) + "\n").encode("ascii"))


def _physical_lines(path: Path) -> tuple[list[str], list[bytes]]:
  contents: list[str] = []
  endings: list[bytes] = []
  for raw_line in path.read_bytes().splitlines(keepends=True):
    content = raw_line.rstrip(b"\r\n")
    contents.append(content.decode("latin-1"))
    endings.append(raw_line[len(content) :])
  return contents, endings


def _write_physical_lines(path: Path, contents: list[str], endings: list[bytes]) -> None:
  if len(contents) != len(endings):
    raise RolRebaseError(f"line content/endings diverged while editing {path}")
  path.write_bytes(
      b"".join(
          content.encode("latin-1") + ending
          for content, ending in zip(contents, endings, strict=True)
      )
  )


def _definition_indexes(contents: list[str], vnum: int) -> tuple[int, int, int, int]:
  try:
    start = contents.index(f"#{vnum}")
  except ValueError as error:
    raise RolRebaseError(f"definition {vnum} is missing") from error
  end = next(
      (
          index
          for index in range(start + 1, len(contents))
          if _HEADER.fullmatch(contents[index]) is not None
          or contents[index] in {"$", "$~"}
      ),
      len(contents),
  )
  terminators = 0
  header_index = -1
  for index in range(start + 1, end):
    if contents[index].rstrip().endswith("~"):
      terminators += 1
    elif terminators >= 4 and contents[index].strip():
      header_index = index
      break
  if header_index < 0 or header_index + 1 >= end:
    raise RolRebaseError(f"definition {vnum} has no complete numeric header")
  return start, end, header_index, header_index + 1


def _repair_static_package_defects(world_root: Path) -> list[dict[str, Any]]:
  repairs: list[dict[str, Any]] = []
  object_path = world_root / "obj/20591.obj"
  contents, endings = _physical_lines(object_path)
  for vnum in sorted(_CANONICAL_HULBURG_KEYS):
    _, _, header_index, _ = _definition_indexes(contents, vnum)
    tokens = contents[header_index].split()
    if int(tokens[0]) != 18:
      old_type = int(tokens[0])
      tokens[0] = "18"
      contents[header_index] = " ".join(tokens)
      repairs.append(
          {
              "path": "obj/20591.obj",
              "vnum": vnum,
              "finding": "REF025",
              "action": "NORMALIZE_KEY_ITEM_TYPE",
              "before": old_type,
              "after": 18,
              "evidence": "incoming exit/container key role and source key identity",
          }
      )
  for vnum in sorted(_CANONICAL_HULBURG_UNHOLY_LIQUIDS):
    _, _, _, values_index = _definition_indexes(contents, vnum)
    values = contents[values_index].split()
    if int(values[2]) == 25:
      values[2] = "13"
      contents[values_index] = " ".join(values)
      repairs.append(
          {
              "path": "obj/20591.obj",
              "vnum": vnum,
              "finding": "SEM018",
              "action": "MAP_UNHOLY_LIQUID_TO_BLOOD",
              "before": 25,
              "after": 13,
              "evidence": (
                  "source LIQ_UNHOLY has no target equivalent; blood preserves hostile theme"
              ),
          }
      )
  for vnum in (2059155, 2059250, 2059307):
    _, _, _, values_index = _definition_indexes(contents, vnum)
    values = contents[values_index].split()
    source_mask = int(values[1])
    target_mask = source_mask & 0xF
    if source_mask != target_mask:
      values[1] = str(target_mask)
      contents[values_index] = " ".join(values)
      repairs.append(
          {
              "path": "obj/20591.obj",
              "vnum": vnum,
              "finding": "SEM018",
              "action": "MASK_CONTAINER_FLAGS",
              "before": source_mask,
              "after": target_mask,
          }
      )
  _, _, _, portal_values_index = _definition_indexes(contents, 2059151)
  portal_values = contents[portal_values_index].split()
  if int(portal_values[1]) == 1159151:
    portal_values[1] = "2059104"
    portal_values[2] = "2059104"
    contents[portal_values_index] = " ".join(portal_values)
    repairs.append(
        {
            "path": "obj/20591.obj",
            "vnum": 2059151,
            "finding": "REF022",
            "action": "REPAIR_PORTAL_DESTINATION",
            "before": 1159151,
            "after": 2059104,
            "evidence": "source portal 59151 targets source room 59104",
        }
    )
  _, _, _, container_values_index = _definition_indexes(contents, 2059307)
  container_values = contents[container_values_index].split()
  if int(container_values[2]) == 100000:
    container_values[2] = "-1"
    contents[container_values_index] = " ".join(container_values)
    repairs.append(
        {
            "path": "obj/20591.obj",
            "vnum": 2059307,
            "finding": "REF022",
            "action": "REMOVE_SYNTHETIC_CONTAINER_KEY",
            "before": 100000,
            "after": -1,
            "evidence": "source container 59307 has no key",
        }
    )
  _write_physical_lines(object_path, contents, endings)

  trail_path = world_root / "obj/20507.obj"
  contents, endings = _physical_lines(trail_path)
  start, end, _, _ = _definition_indexes(contents, 2050700)
  for index in range(start, end - 1):
    if contents[index] == "A" and len(contents[index + 1].split()) == 3:
      contents[index + 1] += " 0"
      repairs.append(
          {
              "path": "obj/20507.obj",
              "vnum": 2050700,
              "finding": "OBJ021",
              "action": "COMPLETE_OBJECT_APPLY_PAYLOAD",
              "after": contents[index + 1],
          }
      )
  _write_physical_lines(trail_path, contents, endings)

  room_path = world_root / "wld/20591.wld"
  contents, endings = _physical_lines(room_path)
  start = contents.index("#2059110")
  end = next(
      index
      for index in range(start + 1, len(contents))
      if _HEADER.fullmatch(contents[index]) is not None
  )
  for index in range(start, end):
    if re.fullmatch(r"\s*0\s+-2\s+2059598\s*", contents[index]) is not None:
      contents[index] = "0 -1 2059598"
      repairs.append(
          {
              "path": "wld/20591.wld",
              "vnum": 2059110,
              "finding": "WLD014",
              "action": "NORMALIZE_NO_KEY_SENTINEL",
              "before": -2,
              "after": -1,
          }
      )
  reverse_start = contents.index("#2059598")
  reverse_end = next(
      index
      for index in range(reverse_start + 1, len(contents))
      if _HEADER.fullmatch(contents[index]) is not None
  )
  for index in range(reverse_start, reverse_end):
    if re.fullmatch(r"\s*0\s+0\s+2059110\s*", contents[index]) is not None:
      contents[index] = "0 -1 2059110"
      repairs.append(
          {
              "path": "wld/20591.wld",
              "vnum": 2059598,
              "finding": "SEM007",
              "action": "MATCH_REVERSE_NO_KEY_SENTINEL",
              "before": 0,
              "after": -1,
          }
      )
  _write_physical_lines(room_path, contents, endings)
  return repairs


def _repair_validation_findings(
    world_root: Path,
    repo_root: Path,
    manifest: dict[str, Any],
    config: Any,
    payload: dict[str, Any],
) -> list[dict[str, Any]]:
  repairs: list[dict[str, Any]] = []
  findings = [item for item in payload.get("findings", []) if not item.get("suppressed")]
  line_actions: dict[str, dict[int, tuple[str, str | None, dict[str, Any]]]] = {}

  for finding in findings:
    path = str(finding["path"])
    code = str(finding["code"])
    line = int(finding["line"])
    if path in {"hlq/20507.hlq", "hlq/20591.hlq", "hlq/20960.hlq"}:
      if code in {"REF034", "REF035"}:
        row = {
            "path": path,
            "vnum": finding["vnum"],
            "line": line,
            "finding": code,
            "action": "EXCLUDE_INVALID_HLQUEST_ITEM_INSTRUCTION",
            "message": finding["message"],
        }
        line_actions.setdefault(path, {})[line] = ("delete", None, row)
      elif code == "SEM033":
        source_line = (world_root / path).read_bytes().splitlines()[line - 1].decode(
            "latin-1"
        )
        tokens = source_line.split()
        if tokens[:2] == ["O", "A"]:
          replacement = "O A 0 0"
        else:
          tokens[-1] = "0"
          replacement = " ".join(tokens)
        row = {
            "path": path,
            "vnum": finding["vnum"],
            "line": line,
            "finding": code,
            "action": "ZERO_UNUSED_HLQUEST_PARAMETER",
            "before": source_line,
            "after": replacement,
        }
        line_actions.setdefault(path, {})[line] = ("replace", replacement, row)
    elif path in {"zon/20507.zon", "zon/20591.zon", "zon/20960.zon"} and code == "ZON027":
      source_line = (world_root / path).read_bytes().splitlines()[line - 1].decode(
          "latin-1"
      )
      tokens = source_line.split()
      if not tokens or tokens[0] != "R" or len(tokens) < 5:
        raise RolRebaseError(f"cannot normalize reset probability at {path}:{line}")
      tokens[4] = "0"
      replacement = " ".join(tokens)
      row = {
          "path": path,
          "vnum": finding["vnum"],
          "line": line,
          "finding": code,
          "action": "CLAMP_RESET_PROBABILITY",
          "before": source_line,
          "after": replacement,
      }
      line_actions.setdefault(path, {})[line] = ("replace", replacement, row)

  world = load_indexed_world_data(world_root, repo_root, manifest, config)
  objects = {record.vnum: record for record in world.objects}
  wear_entries = manifest["tables"]["obj-wear"]["entries"]
  wear_slots = manifest["tables"]["wear-slots"]["entries"]
  preferred_slots = {
      0: 17,
      1: 1,
      2: 3,
      3: 5,
      4: 6,
      5: 7,
      6: 8,
      7: 9,
      8: 10,
      9: 11,
      10: 12,
      11: 13,
      12: 14,
      13: 16,
      15: 22,
      16: 23,
      17: 24,
      18: 26,
      19: 27,
      20: 32,
      21: 28,
      22: 29,
      23: 31,
      33: 42,
  }
  for finding in findings:
    path = str(finding["path"])
    if (
        path not in {"zon/20507.zon", "zon/20591.zon", "zon/20960.zon"}
        or finding["code"] != "REF030"
    ):
      continue
    match = re.search(r"equips object ([0-9]+) in slot ([0-9]+)", finding["message"])
    if match is None:
      raise RolRebaseError(f"cannot parse equipment finding at {path}:{finding['line']}")
    object_vnum = int(match.group(1))
    old_slot = int(match.group(2))
    obj = objects.get(object_vnum)
    if obj is None:
      raise RolRebaseError(f"equipment repair object {object_vnum} is missing")
    wear_bits = decode_tokens(obj.wear_flags, len(wear_entries)).bits
    non_take = sorted(bit for bit in wear_bits if bit in preferred_slots and bit != 0)
    selected_bit = non_take[0] if non_take else 0 if 0 in wear_bits else None
    line = int(finding["line"])
    source_line = (world_root / path).read_bytes().splitlines()[line - 1].decode(
        "latin-1"
    )
    tokens = source_line.split()
    if len(tokens) < 5 or tokens[0] != "E" or int(tokens[4]) != old_slot:
      raise RolRebaseError(f"equipment reset changed before repair at {path}:{line}")
    if selected_bit is None:
      probability = tokens[5] if len(tokens) > 5 else "100"
      replacement = " ".join(["G", tokens[1], tokens[2], tokens[3], probability])
      row = {
          "path": path,
          "vnum": finding["vnum"],
          "line": line,
          "finding": "REF030",
          "action": "CONVERT_UNWEARABLE_EQUIP_TO_GIVE",
          "object_vnum": object_vnum,
          "before": old_slot,
          "after": "inventory",
      }
    else:
      new_slot = preferred_slots[selected_bit]
      if wear_slots[new_slot]["required_wear_index"] != selected_bit:
        raise RolRebaseError(
            f"equipment slot mapping is inconsistent for wear bit {selected_bit}"
        )
      tokens[4] = str(new_slot)
      replacement = " ".join(tokens)
      row = {
          "path": path,
          "vnum": finding["vnum"],
          "line": line,
          "finding": "REF030",
          "action": "MAP_EQUIPMENT_SLOT_TO_OBJECT_WEAR",
          "object_vnum": object_vnum,
          "before": old_slot,
          "after": new_slot,
          "wear_bit": selected_bit,
      }
    line_actions.setdefault(path, {})[line] = ("replace", replacement, row)

  for relative, actions in sorted(line_actions.items()):
    path = world_root / relative
    contents, endings = _physical_lines(path)
    for line, (action, replacement, row) in sorted(
        actions.items(), reverse=True
    ):
      index = line - 1
      if action == "delete":
        del contents[index]
        del endings[index]
      else:
        assert replacement is not None
        contents[index] = replacement
      repairs.append(row)
    _write_physical_lines(path, contents, endings)
  return repairs


def _add_jotun_canonical_records(
    world_root: Path, plan_dir: Path, repo_root: Path
) -> list[dict[str, Any]]:
  actions = [
      row
      for row in _load_jsonl(plan_dir / "reconciliation.jsonl")
      if (str(row["source_kind"]), int(row["source_vnum"]))
      in _JOTUN_CANONICAL_ADDITIONS
  ]
  if len(actions) != len(_JOTUN_CANONICAL_ADDITIONS):
    raise RolRebaseError(
        "the identity plan does not contain all six canonical Jotunheim additions"
    )
  records, diagnostics = _source_records(repo_root, actions)
  if diagnostics:
    raise RolRebaseError("canonical Jotunheim additions have source parse diagnostics")
  resolve = _identity_resolver(plan_dir)
  rows: list[dict[str, Any]] = []
  for action in sorted(
      actions, key=lambda row: (str(row["source_kind"]), int(row["source_vnum"]))
  ):
    kind = str(action["source_kind"])
    source_vnum = int(action["source_vnum"])
    destination = _JOTUN_CANONICAL_ADDITIONS[(kind, source_vnum)]
    if int(action["destination_vnum"]) != destination:
      raise RolRebaseError(
          f"Jotunheim addition {kind} {source_vnum} has a noncanonical destination"
      )
    record = records[str(action["source_record_id"])]
    if kind == "mob":
      emitted = emit_mobile(record, destination)
      path = world_root / "mob/20960.mob"
      terminator = "$"
    else:
      emitted = emit_object(record, destination, resolve)
      path = world_root / "obj/20960.obj"
      terminator = "$~"
    _insert_definition(path, destination, emitted.text, terminator)
    rows.append(
        {
            "operation": "CANONICAL_ADD",
            "record_kind": kind,
            "source_vnum": source_vnum,
            "target_vnum": destination,
            "source_record_id": action["source_record_id"],
            "source_sha256": action["source_sha256"],
        }
    )
  return rows


def _split_artifact_objects(world_root: Path) -> list[dict[str, Any]]:
  object_root = world_root / "obj"
  all_blocks: dict[int, list[str]] = {}
  for name in _ARTIFACT_OBJECT_PACKAGES:
    for vnum, block in _object_blocks(object_root / name).items():
      if vnum in all_blocks:
        raise RolRebaseError(f"artifact object {vnum} is defined more than once")
      all_blocks[vnum] = block
  missing = sorted((_CANONICAL_ARTIFACTS - {2001009}) - set(all_blocks))
  if missing:
    raise RolRebaseError(f"canonical artifact prototype is missing: {missing[0]}")
  if 2001009 not in all_blocks:
    variant = list(all_blocks[2001007])
    variant[0] = "#2001009"
    variant[1] = "hammer kelrarin second artifact~"
    variant[2] = "@WKelrarin's Second Hammer@n~"
    variant[3] = (
        "@WA second warhammer rests here, its head crackling with contained lightning.@n~"
    )
    for index, line in enumerate(variant):
      if index > 3 and line == "hammer kelrarin artifact~":
        variant[index] = "hammer kelrarin second artifact~"
        break
    all_blocks[2001009] = variant

  expected = set().union(*_ARTIFACT_OBJECT_PACKAGES.values())
  unexpected = sorted(set(all_blocks) - expected)
  if unexpected:
    raise RolRebaseError(f"unexpected artifact package object {unexpected[0]}")
  for name, vnums in _ARTIFACT_OBJECT_PACKAGES.items():
    _write_object_blocks(object_root / name, {vnum: all_blocks[vnum] for vnum in vnums})
  _write_index(object_root / "index", set(), set(_ARTIFACT_OBJECT_PACKAGES))

  zone_path = world_root / "zon/1699.zon"
  lines = zone_path.read_text(encoding="ascii").splitlines()
  if not any(re.match(r"^O\s+\S+\s+2001009\b", line) for line in lines):
    for index, line in enumerate(lines):
      if re.match(r"^O\s+\S+\s+2001007\b", line):
        lines.insert(
            index + 1,
            "O 0 2001009 1 169900 100 \t(\tKelrarin's second Hammer\tn)",
        )
        break
    else:
      raise RolRebaseError("artifact vault reset for canonical Kelrarin is missing")
    zone_path.write_bytes(("\n".join(lines) + "\n").encode("ascii"))

  return [
      {
          "operation": "REHOME",
          "record_kind": "obj",
          "source_vnum": source,
          "target_vnum": target,
          "persistent_successor": source != 169906 or target == 2001007,
      }
      for source, target in sorted(_ARTIFACT_REHOMES.items())
  ] + [
      {
          "operation": "RESTORE_DISTINCT_IDENTITY",
          "record_kind": "obj",
          "source_vnum": 1009,
          "target_vnum": 2001009,
          "persistent_successor": False,
          "runtime_contract": "shared Kelrarin behavior with independent registry state",
          "identity_contract": "distinct name, prototype, ownership, progression, and cooldowns",
      }
  ]


def _migrate_artifact_state(source: Path, destination: Path) -> dict[str, Any]:
  lines = source.read_text(encoding="ascii").splitlines()
  output: list[str] = []
  seen: set[int] = set()
  migrated = 0
  for line in lines:
    if not line or line.startswith("#"):
      output.append(line)
      continue
    fields = line.split()
    try:
      old_vnum = int(fields[0])
    except (IndexError, ValueError) as error:
      raise RolRebaseError(f"malformed artifact state row: {line!r}") from error
    vnum = _ARTIFACT_REHOMES.get(old_vnum, old_vnum)
    if vnum in seen:
      raise RolRebaseError(f"artifact state would duplicate VNUM {vnum}")
    seen.add(vnum)
    fields[0] = str(vnum)
    output.append(" ".join(fields))
    migrated += vnum != old_vnum
  added = False
  if 2001009 not in seen:
    output.append(
        "2001009 noone noone 1 0 0 0 noone noone " + " ".join("0" for _ in range(16))
    )
    seen.add(2001009)
    added = True
  expected = set().union(*_ARTIFACT_OBJECT_PACKAGES.values())
  if seen != expected:
    missing = sorted(expected - seen)
    unexpected = sorted(seen - expected)
    raise RolRebaseError(
        "artifact state does not match the canonical registry: "
        f"missing={missing}, unexpected={unexpected}"
    )
  data_rows = sorted(
      (line for line in output if line and not line.startswith("#")),
      key=lambda line: int(line.split()[0]),
  )
  comments = [line for line in output if not line or line.startswith("#")]
  destination.parent.mkdir(parents=True, exist_ok=True)
  destination.write_bytes(("\n".join([*comments, *data_rows]) + "\n").encode("ascii"))
  return {
      "path": "world/world.artifact",
      "migrated_rows": migrated,
      "added_unowned_rows": int(added),
      "rows": len(data_rows),
      "kelrarin_state_successor": 2001007,
      "state_clones": 0,
  }


def _migrate_object_store(source_root: Path, destination_root: Path) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  if not source_root.is_dir():
    return rows
  for path in sorted(item for item in source_root.rglob("*") if item.is_file()):
    lines = path.read_bytes().decode("ascii").splitlines()
    changed = False
    count = 0
    for index, line in enumerate(lines):
      match = _HEADER.fullmatch(line)
      if match is None:
        continue
      source_vnum = int(match.group(1))
      target_vnum = retired_destination(source_vnum)
      if target_vnum is None or source_vnum in _CORE_PACKAGES:
        continue
      lines[index] = f"#{target_vnum}"
      changed = True
      count += 1
    if changed:
      destination = destination_root / path.relative_to(source_root)
      destination.parent.mkdir(parents=True, exist_ok=True)
      destination.write_bytes(("\n".join(lines) + "\n").encode("ascii"))
      rows.append(
          {
              "path": path.relative_to(source_root).as_posix(),
              "object_headers_migrated": count,
          }
      )
  return rows


def _database_value_mapping(
    record_type: str, value_expression: str, *, reverse: bool = False
) -> tuple[list[str], list[str]]:
  cases: list[str] = []
  predicates: list[str] = []
  if record_type == "zone":
    for source, target in ((1507, 20507), (1591, 20591), (1960, 20960)):
      if reverse:
        source, target = target, source
      cases.append(f"WHEN {value_expression} = {source} THEN {target}")
      predicates.append(f"{value_expression} = {source}")
    return cases, predicates

  if record_type not in {"room", "mobile", "object", "quest", "trigger"}:
    return cases, predicates
  for low, high in ((150700, 150899), (159100, 159599), (196000, 196299)):
    if reverse:
      cases.append(
          f"WHEN {value_expression} BETWEEN {low + 1900000} AND {high + 1900000} "
          f"THEN {value_expression} - 1900000"
      )
      predicates.append(
          f"{value_expression} BETWEEN {low + 1900000} AND {high + 1900000}"
      )
    else:
      cases.append(
          f"WHEN {value_expression} BETWEEN {low} AND {high} "
          f"THEN {value_expression} + 1900000"
      )
      predicates.append(f"{value_expression} BETWEEN {low} AND {high}")
  if record_type == "object":
    for source, target in sorted(_ARTIFACT_REHOMES.items()):
      if reverse:
        source, target = target, source
      cases.append(f"WHEN {value_expression} = {source} THEN {target}")
      predicates.append(f"{value_expression} = {source}")
  return cases, predicates


def _database_updates(
    columns: list[dict[str, Any]],
    include_guarded_missing: bool = False,
    *,
    reverse: bool = False,
) -> list[str]:
  statements: list[str] = []
  for column in complete_persistent_bindings(columns, include_guarded_missing):
    if not column.get("migration_required"):
      continue
    table = str(column["table"])
    name = str(column["column"])
    if re.fullmatch(r"[A-Za-z0-9_$]+", table) is None or re.fullmatch(
        r"[A-Za-z0-9_$]+", name
    ) is None:
      raise RolRebaseError("database binding contains an unsafe identifier")

    record_type = str(column["record_type"])
    encoding = str(column.get("encoding", "integer"))
    predicate = str(column["predicate"]) if column.get("predicate") else None
    if encoding == "integer":
      value_expression = f"`{name}`"
      cases, predicates = _database_value_mapping(
          record_type, value_expression, reverse=reverse
      )
      if not cases:
        continue
      where = " OR ".join(predicates)
      if predicate:
        where = f"({predicate}) AND ({where})"
      statements.append(
          f"UPDATE `{table}` SET `{name}` = CASE "
          + " ".join(cases)
          + f" ELSE `{name}` END WHERE {where};"
      )
      continue

    if encoding == "integer_text":
      value_expression = f"CAST(`{name}` AS SIGNED)"
      cases, predicates = _database_value_mapping(
          record_type, value_expression, reverse=reverse
      )
      if not cases:
        continue
      where = f"`{name}` REGEXP '^[0-9]+$' AND (" + " OR ".join(predicates) + ")"
      if predicate:
        where = f"({predicate}) AND ({where})"
      statements.append(
          f"UPDATE `{table}` SET `{name}` = CAST(CASE "
          + " ".join(cases)
          + f" ELSE {value_expression} END AS CHAR) WHERE {where};"
      )
      continue

    if encoding == "object_header_blob":
      value_expression = (
          f"CAST(SUBSTRING(SUBSTRING_INDEX(CAST(`{name}` AS CHAR), CHAR(10), 1), 2) "
          "AS UNSIGNED)"
      )
      cases, predicates = _database_value_mapping(
          record_type, value_expression, reverse=reverse
      )
      where = (
          f"LEFT(CAST(`{name}` AS CHAR), 1) = '#' "
          f"AND LOCATE(CHAR(10), `{name}`) > 0 AND ("
          + " OR ".join(predicates)
          + ")"
      )
      statements.append(
          f"UPDATE `{table}` SET `{name}` = CONCAT('#', CASE "
          + " ".join(cases)
          + f" ELSE {value_expression} END, "
          f"SUBSTRING(`{name}`, LOCATE(CHAR(10), `{name}`))) WHERE {where};"
      )
      continue

    raise RolRebaseError(
        f"database binding {table}.{name} has unsupported migration encoding {encoding}"
    )
  return statements


def _database_sql(
    columns: list[dict[str, Any]], *, reverse: bool = False
) -> str:
  statements = ["START TRANSACTION;"]
  for index, update in enumerate(
      _database_updates(
          columns, include_guarded_missing=True, reverse=reverse
      ),
      start=1,
  ):
    table_match = re.match(r"UPDATE `([A-Za-z0-9_$]+)` ", update)
    if table_match is None:
      raise RolRebaseError("database migration contains an unrecognized update")
    table = table_match.group(1)
    required_columns = sorted(
        set(re.findall(r"`([A-Za-z0-9_$]+)`", update)) - {table}
    )
    escaped_update = update.replace("'", "''")
    statement_name = f"rol_rebase_update_{index}"
    column_guards = "".join(
        " AND EXISTS(SELECT 1 FROM information_schema.COLUMNS "
        "WHERE TABLE_SCHEMA = DATABASE() "
        f"AND TABLE_NAME = '{table}' AND COLUMN_NAME = '{column}')"
        for column in required_columns
    )
    statements.extend(
        [
            "SET @rol_rebase_sql = IF(",
            "  EXISTS(SELECT 1 FROM information_schema.TABLES "
            "WHERE TABLE_SCHEMA = DATABASE() "
            f"AND TABLE_NAME = '{table}' AND TABLE_TYPE = 'BASE TABLE')"
            f"{column_guards},",
            f"  '{escaped_update}',",
            "  'DO 0');",
            f"PREPARE {statement_name} FROM @rol_rebase_sql;",
            f"EXECUTE {statement_name};",
            f"DEALLOCATE PREPARE {statement_name};",
        ]
    )
  statements.append("COMMIT;")
  return "\n".join(statements) + "\n"


def _database_preflight_sql(sql: str) -> str:
  if not sql.endswith("COMMIT;\n"):
    raise RolRebaseError("database migration does not end with COMMIT")
  return sql[: -len("COMMIT;\n")] + "ROLLBACK;\n"


def _validate_identity_plan(
    plan_dir: Path, discovery_dir: Path
) -> dict[str, Any]:
  policy = json.loads((discovery_dir / "policies.json").read_text(encoding="ascii"))
  records = {row["record_id"]: row for row in _load_jsonl(discovery_dir / "source-records.jsonl")}
  identities = _load_jsonl(plan_dir / "identity-map.jsonl")
  failures: list[dict[str, Any]] = []
  mytheast: list[int] = []
  for identity in identities:
    if identity["resolution"] == "EXCLUDE":
      continue
    record = records[identity["source_record_id"]]
    expected = canonical_destination(
        identity["source_kind"], identity["source_vnum"], record["basename"], policy
    )
    if identity["destination_vnum"] != expected:
      failures.append(
          {
              "source_record_id": identity["source_record_id"],
              "actual": identity["destination_vnum"],
              "expected": expected,
          }
      )
    if record["basename"] == "mytheast" and record["kind"] == "zon":
      mytheast.append(identity["destination_vnum"])
  if failures:
    raise RolRebaseError(
        f"identity plan contains {len(failures)} noncanonical destination(s)"
    )
  if mytheast != [20817]:
    raise RolRebaseError(f"mytheast normalization resolved to {mytheast}, not [20817]")
  return {
      "active_core_identities": len(identities),
      "noncanonical_destinations": 0,
      "mytheast_target_zones": mytheast,
  }


def _world_retired_audit(world_root: Path, repo_root: Path) -> dict[str, Any]:
  qst_rewrites = _typed_qst_rewrites(world_root, repo_root)
  active: list[dict[str, Any]] = []
  retained: list[dict[str, Any]] = []
  for directory in ("zon", "wld", "mob", "obj", "shp", "qst", "hlq", "trg"):
    for path in sorted((world_root / directory).glob(f"*.{directory}")):
      relative = path.relative_to(world_root).as_posix()
      for line_number, raw_line in enumerate(path.read_bytes().splitlines(), start=1):
        line = raw_line.decode("latin-1")
        for match in _TOKEN_BYTES.finditer(raw_line):
          vnum = int(match.group(1).decode("ascii"))
          if retired_destination(vnum) is None:
            continue
          action, role = _world_role(relative, line_number, line, vnum, qst_rewrites)
          row = {"path": relative, "line": line_number, "vnum": vnum, "role": role}
          (retained if action == "retain" else active).append(row)
  if active:
    first = active[0]
    raise RolRebaseError(
        f"staged world retains active VNUM {first['vnum']} at {first['path']}:{first['line']}"
    )
  definitions: Counter[int] = Counter()
  for path in sorted((world_root / "obj").glob("*.obj")):
    for line in path.read_bytes().splitlines():
      match = _HEADER_BYTES.fullmatch(line)
      if match is not None and int(match.group(1).decode("ascii")) in _CANONICAL_ARTIFACTS:
        definitions[int(match.group(1).decode("ascii"))] += 1
  bad_artifacts = sorted(vnum for vnum in _CANONICAL_ARTIFACTS if definitions[vnum] != 1)
  if bad_artifacts:
    raise RolRebaseError(
        f"canonical artifact {bad_artifacts[0]} has {definitions[bad_artifacts[0]]} definitions"
    )
  return {
      "active_references_to_retired_vnums": 0,
      "target_owned_numeric_matches": retained,
      "canonical_artifact_definitions": len(definitions),
      "artifact_definition_collisions": 0,
  }


def _validation_summary(payload: dict[str, Any]) -> dict[str, Any]:
  findings = [item for item in payload.get("findings", []) if not item.get("suppressed")]
  return {
      "complete": payload.get("complete"),
      "errors": sum(item["severity"] == "error" for item in findings),
      "warnings": sum(item["severity"] == "warning" for item in findings),
      "by_code": dict(sorted(Counter(item["code"] for item in findings).items())),
  }


def _legacy_predecessor(vnum: int) -> int:
  artifact = {target: source for source, target in _ARTIFACT_REHOMES.items()}
  artifact[2001009] = 1009
  if vnum in artifact:
    return artifact[vnum]
  for source_zone, package in _CORE_PACKAGES.items():
    if vnum == int(package["target"]):
      return source_zone
    low = int(package["low"]) + 1900000
    high = int(package["high"]) + 1900000
    if low <= vnum <= high:
      return vnum - 1900000
  return vnum


def _normalize_message(message: str) -> str:
  message = re.sub(r"\([0-9]+ key uses\)", "(key uses)", message)
  return _TOKEN.sub(
      lambda match: str(_legacy_predecessor(int(match.group(1)))), message
  )


def _normalize_finding(finding: dict[str, Any]) -> tuple[tuple[str, str], ...]:
  normalized = dict(finding)
  normalized.pop("suppressed", None)
  path = str(normalized.get("path", ""))
  moved = Path(path).stem in {
      *(str(zone) for zone in _CORE_PACKAGES),
      *(str(package["target"]) for package in _CORE_PACKAGES.values()),
      "1699",
      "20010",
      "20053",
      "20197",
  }
  for source_zone, package in _CORE_PACKAGES.items():
    target_zone = str(package["target"])
    if Path(path).stem == target_zone:
      path = str(Path(path).with_name(f"{source_zone}{Path(path).suffix}"))
      moved = True
      break
  if path.startswith("obj/") and Path(path).name in _ARTIFACT_OBJECT_PACKAGES:
    path = "obj/1699.obj"
    moved = True
  normalized["path"] = path
  if isinstance(normalized.get("vnum"), int):
    normalized["vnum"] = _legacy_predecessor(int(normalized["vnum"]))
  if isinstance(normalized.get("message"), str):
    normalized["message"] = _normalize_message(str(normalized["message"]))
  related = normalized.get("related")
  if isinstance(related, dict):
    related = dict(related)
    related_path = str(related.get("path", ""))
    if Path(related_path).stem in {
        *(str(zone) for zone in _CORE_PACKAGES),
        *(str(package["target"]) for package in _CORE_PACKAGES.values()),
    }:
      moved = True
    for source_zone, package in _CORE_PACKAGES.items():
      if Path(related_path).stem == str(package["target"]):
        related_path = str(
            Path(related_path).with_name(f"{source_zone}{Path(related_path).suffix}")
        )
        moved = True
        break
    related["path"] = related_path
    if isinstance(related.get("vnum"), int):
      related["vnum"] = _legacy_predecessor(int(related["vnum"]))
    normalized["related"] = related
  if moved:
    for key in ("line", "end_line", "column", "end_column"):
      normalized.pop(key, None)
    if isinstance(normalized.get("related"), dict):
      for key in ("line", "column"):
        normalized["related"].pop(key, None)
  return tuple(
      (key, json.dumps(value, ensure_ascii=True, sort_keys=True))
      for key, value in sorted(normalized.items())
  )


def _validation_delta(
    baseline: dict[str, Any], staged: dict[str, Any]
) -> dict[str, Any]:
  baseline_findings = [
      item for item in baseline.get("findings", []) if not item.get("suppressed")
  ]
  staged_findings = [
      item for item in staged.get("findings", []) if not item.get("suppressed")
  ]
  before = Counter(_normalize_finding(item) for item in baseline_findings)
  after = Counter(_normalize_finding(item) for item in staged_findings)
  additions = list((after - before).elements())
  removals = list((before - after).elements())
  return {
      "normalized_baseline_findings": sum(before.values()),
      "normalized_staged_findings": sum(after.values()),
      "normalized_added_findings": len(additions),
      "normalized_repaired_findings": len(removals),
      "adds_no_baseline_finding": not additions,
      "addition_samples": [dict(item) for item in additions[:10]],
  }


def _source_reference_closure(discovery_dir: Path) -> dict[str, Any]:
  rows = _load_jsonl(discovery_dir / "reference-report.jsonl")
  unresolved_required = [
      row
      for row in rows
      if str(row["resolution"]).startswith("unresolved")
      and row["resolution_action"] != "exclude_dependent_instruction"
  ]
  return {
      "typed_edges": len(rows),
      "by_resolution": dict(sorted(Counter(row["resolution"] for row in rows).items())),
      "by_action": dict(
          sorted(Counter(row["resolution_action"] for row in rows).items())
      ),
      "owned_exclusions": sum(
          row["resolution_action"] == "exclude_dependent_instruction" for row in rows
      ),
      "unresolved_required_typed_references": len(unresolved_required),
      "unresolved_required_samples": unresolved_required[:10],
  }


def _build_apply_plan(
    source_root: Path,
    staged_root: Path,
    scope: str,
    destination_prefix: str,
) -> list[dict[str, Any]]:
  paths = {
      path.relative_to(source_root).as_posix()
      for path in source_root.rglob("*")
      if path.is_file()
  } | {
      path.relative_to(staged_root).as_posix()
      for path in staged_root.rglob("*")
      if path.is_file()
  }
  rows: list[dict[str, Any]] = []
  for relative in sorted(paths):
    source = source_root / relative
    staged = staged_root / relative
    before = _sha256_path(source) if source.is_file() else None
    after = _sha256_path(staged) if staged.is_file() else None
    if before == after:
      continue
    rows.append(
        {
            "scope": scope,
            "source_path": relative,
            "destination_path": f"{destination_prefix}/{relative}",
            "before_sha256": before,
            "after_sha256": after,
            "action": "REMOVE" if after is None else "ADD" if before is None else "REPLACE",
        }
    )
  return rows


def _build_overlay_plan(
    source_root: Path,
    staged_root: Path,
    scope: str,
    destination_prefix: str,
) -> list[dict[str, Any]]:
  rows: list[dict[str, Any]] = []
  for staged in sorted(item for item in staged_root.rglob("*") if item.is_file()):
    relative = staged.relative_to(staged_root).as_posix()
    source = source_root / relative
    before = _sha256_path(source) if source.is_file() else None
    after = _sha256_path(staged)
    if before == after:
      continue
    rows.append(
        {
            "scope": scope,
            "source_path": relative,
            "destination_path": f"{destination_prefix}/{relative}",
            "before_sha256": before,
            "after_sha256": after,
            "action": "ADD" if before is None else "REPLACE",
        }
    )
  return rows


def write_rebase_bundle(
    discovery_dir: Path,
    plan_dir: Path,
    phase6_dir: Path,
    world_root: Path,
    lib_root: Path,
    repo_root: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Build the complete non-mutating Phase 6.5 cutover bundle."""

  discovery_dir = discovery_dir.resolve()
  plan_dir = plan_dir.resolve()
  phase6_dir = phase6_dir.resolve()
  world_root = world_root.resolve()
  lib_root = lib_root.resolve()
  repo_root = repo_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolRebaseError(f"rebase output directory already exists: {output_dir}")
  discovery_manifest = verify_discovery_bundle(discovery_dir)
  plan_manifest = _verify_bundle(plan_dir, 2)
  phase6_manifest = _verify_bundle(phase6_dir, 6)
  phase6_summary = json.loads(
      (phase6_dir / "reconciliation-summary.json").read_text(encoding="ascii")
  )
  if plan_manifest.get("discovery_run_id") != discovery_manifest.get("run_id"):
    raise RolRebaseError("Phase 2 plan does not belong to the discovery bundle")
  if phase6_manifest.get(
      "discovery_run_id", phase6_summary.get("discovery_run_id")
  ) != discovery_manifest.get("run_id"):
    raise RolRebaseError("Phase 6 bundle does not belong to the discovery bundle")
  if phase6_manifest.get("plan_run_id", phase6_summary.get("plan_run_id")) != plan_manifest.get(
      "run_id"
  ):
    raise RolRebaseError("Phase 6 bundle does not belong to the identity plan")
  phase6_acceptance = phase6_manifest.get("acceptance", {})
  required_phase6 = (
      "all_act_spec_records_accounted",
      "all_automatic_race_bindings_accounted",
      "all_direct_bindings_accounted",
      "all_dynamic_registrations_accounted",
      "all_handlers_accounted",
  )
  pending_phase6 = (
      "act_spec_records_pending",
      "automatic_race_bindings_pending",
      "direct_bindings_pending",
  )
  if not all(phase6_acceptance.get(key) is True for key in required_phase6) or not all(
      phase6_acceptance.get(key) == 0 for key in pending_phase6
  ):
    raise RolRebaseError("Phase 6 acceptance is incomplete")

  identity_audit = _validate_identity_plan(plan_dir, discovery_dir)
  output_dir.mkdir(parents=True)
  staged_world = output_dir / "output/world"
  shutil.copytree(world_root, staged_world)
  rewrites, retained = _rewrite_world(staged_world, repo_root)
  rehomes = _move_core_packages(staged_world)
  rehomes.extend(_add_jotun_canonical_records(staged_world, plan_dir, repo_root))
  rehomes.extend(_split_artifact_objects(staged_world))
  repairs = _repair_static_package_defects(staged_world)

  manifest = load_manifest(repo_root / "scripts/world/wtool_constants.json")
  config = resolve_config(world_root, None)
  preliminary_payload = result_payload(
      validate_indexed_world(staged_world, repo_root, manifest, config)
  )
  repairs.extend(
      _repair_validation_findings(
          staged_world, repo_root, manifest, config, preliminary_payload
      )
  )

  persistence_root = output_dir / "output/persistence"
  artifact_state = _migrate_artifact_state(
      lib_root / "world/world.artifact", persistence_root / "world/world.artifact"
  )
  persisted_objects = {
      "plrobjs": _migrate_object_store(lib_root / "plrobjs", persistence_root / "plrobjs"),
      "house": _migrate_object_store(lib_root / "house", persistence_root / "house"),
  }

  artifact_package = output_dir / "output/artifact-package"
  artifact_package.mkdir(parents=True)
  for name in ("1699.zon", "1699.wld", "1699.mob", *_ARTIFACT_OBJECT_PACKAGES):
    source = staged_world / Path(name).suffix.removeprefix(".") / name
    shutil.copy2(source, artifact_package / name)
  shutil.copy2(lib_root / "world/artifacts/artifacts.hlp", artifact_package / "artifacts.hlp")
  help_package = output_dir / "output/help"
  help_package.mkdir()
  shutil.copy2(artifact_package / "artifacts.hlp", help_package / "artifacts.hlp")

  bindings = json.loads((discovery_dir / "bindings.json").read_text(encoding="ascii"))
  persistent_columns = bindings["persistent_bindings"]["columns"]
  consumer_rows = persistent_consumer_ledger(persistent_columns)
  unclassified_consumers = [
      row for row in consumer_rows if row["classification_status"] == "unclassified"
  ]
  if unclassified_consumers:
    first = unclassified_consumers[0]
    raise RolRebaseError(
        "persistent database binding remains unclassified: "
        f"{first['table']}.{first['column']}"
    )
  _write_jsonl(output_dir / "persistent-consumer-ledger.jsonl", consumer_rows)
  sql_path = output_dir / "output/persistence/rol_phase6_5_vnum_migration.sql"
  sql_path.write_text(_database_sql(persistent_columns), encoding="ascii")

  retired_audit = _world_retired_audit(staged_world, repo_root)
  baseline_payload = result_payload(
      validate_indexed_world(world_root, repo_root, manifest, config)
  )
  staged_payload = result_payload(
      validate_indexed_world(staged_world, repo_root, manifest, config)
  )
  baseline_payload["root"] = "authoritative-development-baseline"
  preliminary_payload["root"] = "canonical-rebase-preliminary"
  staged_payload["root"] = "canonical-rebase-staged"
  validation_dir = output_dir / "validation"
  validation_dir.mkdir()
  (validation_dir / "baseline.json").write_bytes(_canonical_json(baseline_payload))
  (validation_dir / "preliminary.json").write_bytes(
      _canonical_json(preliminary_payload)
  )
  (validation_dir / "staged.json").write_bytes(_canonical_json(staged_payload))
  baseline_summary = _validation_summary(baseline_payload)
  staged_summary = _validation_summary(staged_payload)
  validation_delta = _validation_delta(baseline_payload, staged_payload)
  if not validation_delta["adds_no_baseline_finding"]:
    raise RolRebaseError(
        "staged rebase adds normalized validation findings: "
        f"{validation_delta['normalized_added_findings']}"
    )

  touched_names = {
      f"{package['target']}.{kind}"
      for package in _CORE_PACKAGES.values()
      for kind in _MOVED_KINDS
  } | set(_ARTIFACT_OBJECT_PACKAGES) | {"1699.zon", "1699.wld", "1699.mob"}
  touched_findings = [
      item
      for item in staged_payload.get("findings", [])
      if not item.get("suppressed") and Path(str(item["path"])).name in touched_names
  ]
  touched_blockers = [
      item
      for item in touched_findings
      if item["severity"] == "error" or str(item["code"]).startswith("REF")
  ]
  if touched_blockers:
    first = touched_blockers[0]
    raise RolRebaseError(
        "touched record retains an unresolved finding: "
        f"{first['code']} at {first['path']}:{first['line']}"
    )

  source_reference_closure = _source_reference_closure(discovery_dir)
  if source_reference_closure["unresolved_required_typed_references"]:
    raise RolRebaseError("source reference ledger retains unresolved required edges")

  for name in ("source-inventory.json", "target-inventory.json"):
    shutil.copy2(discovery_dir / name, output_dir / name)
  shutil.copy2(plan_dir / "reconciliation.jsonl", output_dir / "reconciliation.jsonl")
  shutil.copy2(plan_dir / "identity-map.jsonl", output_dir / "identity-map.jsonl")
  shutil.copy2(plan_dir / "capabilities.jsonl", output_dir / "capabilities.jsonl")
  shutil.copy2(
      discovery_dir / "reference-report.jsonl", output_dir / "reference-ledger.jsonl"
  )

  _write_jsonl(output_dir / "rehome.jsonl", rehomes)
  _write_jsonl(output_dir / "repair-ledger.jsonl", repairs)
  reference_report = {
      "source_reference_closure": source_reference_closure,
      "world_rewrites": rewrites,
      "target_owned_matches": retained,
      "post_stage": retired_audit,
  }
  (output_dir / "reference-report.json").write_bytes(_canonical_json(reference_report))
  persistence_report = {
      "artifact_state": artifact_state,
      "object_stores": persisted_objects,
      "database_migration": sql_path.relative_to(output_dir).as_posix(),
      "database_consumers": {
          "classified": len(consumer_rows),
          "migration_required": sum(
              bool(row.get("migration_required")) for row in consumer_rows
          ),
          "unclassified": 0,
          "ledger": "persistent-consumer-ledger.jsonl",
      },
  }
  (output_dir / "persistence-report.json").write_bytes(_canonical_json(persistence_report))
  (output_dir / "identity-audit.json").write_bytes(_canonical_json(identity_audit))
  validation_summary = {
      "baseline": baseline_summary,
      "staged": staged_summary,
      "delta": validation_delta,
      "touched_findings": {
          "total": len(touched_findings),
          "blocking": 0,
          "by_code": dict(
              sorted(Counter(item["code"] for item in touched_findings).items())
          ),
          "disposition": "baseline-preserved nonblocking finding",
      },
  }
  (validation_dir / "summary.json").write_bytes(_canonical_json(validation_summary))

  apply_rows = _build_apply_plan(world_root, staged_world, "world", "world")
  apply_rows.extend(
      _build_apply_plan(
          lib_root / "world/artifacts",
          artifact_package,
          "artifact-package",
          "world/artifacts",
      )
  )
  apply_rows.extend(
      _build_overlay_plan(lib_root / "text/help", help_package, "help", "text/help")
  )
  for store in ("plrobjs", "house"):
    staged_store = persistence_root / store
    if staged_store.exists():
      apply_rows.extend(
          _build_overlay_plan(lib_root / store, staged_store, store, store)
      )
  state_row = {
      "scope": "artifact-state",
      "source_path": "world/world.artifact",
      "destination_path": "world/world.artifact",
      "before_sha256": _sha256_path(lib_root / "world/world.artifact"),
      "after_sha256": _sha256_path(persistence_root / "world/world.artifact"),
      "action": "REPLACE",
  }
  if state_row["before_sha256"] != state_row["after_sha256"]:
    apply_rows.append(state_row)
  _write_jsonl(output_dir / "change-plan.jsonl", apply_rows)
  removals = [row for row in apply_rows if row["action"] == "REMOVE"]
  _write_jsonl(output_dir / "removals.jsonl", removals)

  invariants = {
      "noncanonical_active_rol_zone_identities": 0,
      "noncanonical_active_rol_entity_identities": 0,
      "active_references_to_retired_rol_vnums": retired_audit[
          "active_references_to_retired_vnums"
      ],
      "unresolved_required_typed_references": source_reference_closure[
          "unresolved_required_typed_references"
      ],
  }
  (output_dir / "acceptance.json").write_bytes(
      _canonical_json(
          {
              "invariants": invariants,
              "identity": identity_audit,
              "world": retired_audit,
              "persistence": persistence_report,
              "validation": validation_summary,
              "repeat_generation_contract": "byte-identical for identical inputs",
              "repeat_apply_contract": "no-op when destination hashes equal bundle outputs",
          }
      )
  )

  input_ids = [
      discovery_manifest["run_id"],
      plan_manifest["run_id"],
      phase6_manifest["run_id"],
      _tree_hash(staged_world),
      _tree_hash(persistence_root),
      _tree_hash(artifact_package),
      _tree_hash(help_package),
  ]
  run_hash = hashlib.sha256(chr(10).join(input_ids).encode("ascii")).hexdigest()
  run_id = f"rol-phase6-5-{run_hash[:16]}"
  artifacts = []
  for path in sorted(
      item
      for item in output_dir.rglob("*")
      if item.is_file()
      and item.name != "run-manifest.json"
      and not item.relative_to(output_dir).as_posix().startswith("output/world/")
  ):
    artifacts.append(
        {
            "path": path.relative_to(output_dir).as_posix(),
            "byte_size": path.stat().st_size,
            "sha256": _sha256_path(path),
        }
    )
  run_manifest = {
      "schema_version": ROL_REBASE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5",
      "discovery_run_id": discovery_manifest["run_id"],
      "plan_run_id": plan_manifest["run_id"],
      "phase6_run_id": phase6_manifest["run_id"],
      "world_tree_sha256": _tree_hash(staged_world),
      "artifacts": artifacts,
      "acceptance": {
          "complete": True,
          "canonical_identity_failures": 0,
          "retired_world_references": 0,
          "artifact_state_clones": 0,
          "normalized_added_findings": 0,
          "touched_blocking_findings": 0,
          "repairs": len(repairs),
          "apply_changes": len(apply_rows),
          "invariants": invariants,
      },
  }
  (output_dir / "run-manifest.json").write_bytes(_canonical_json(run_manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "world_rewrites": len(rewrites),
      "rehomes": len(rehomes),
      "persistent_object_files": sum(len(rows) for rows in persisted_objects.values()),
      "apply_changes": len(apply_rows),
      "invariants": invariants,
  }


def _development_environment(lib_root: Path) -> None:
  env_path = lib_root / ".env"
  values: dict[str, str] = {}
  for raw in env_path.read_text(encoding="utf-8").splitlines():
    line = raw.strip()
    if not line or line.startswith("#") or "=" not in line:
      continue
    key, value = line.split("=", 1)
    values[key.strip()] = value.strip().strip("'\"")
  environment = values.get("APP_ENV", "").casefold()
  if environment not in {"dev", "development", "local", "test", "testing"}:
    raise RolRebaseError("lib/.env does not explicitly identify a development environment")


def apply_rebase_bundle(
    bundle_dir: Path,
    lib_root: Path,
    database_config: Path | None = None,
) -> dict[str, Any]:
  """Reject the superseded destructive Phase 6.5 target-rehome workflow."""

  raise RolRebaseError(
      "Phase 6.5 target-rehome apply is disabled because it overwrites existing "
      "Luminari identities; use the isolated Phase 7/8 overlay workflow"
  )


def render_rol_rebase_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 6.5 rebase: {summary['run_id']}\n"
      f"Output: {summary['output_dir']}\n"
      f"World rewrites: {summary['world_rewrites']}\n"
      f"Rehome rows: {summary['rehomes']}\n"
      f"Persistent object files: {summary['persistent_object_files']}\n"
      f"Apply changes: {summary['apply_changes']}\n"
  )
