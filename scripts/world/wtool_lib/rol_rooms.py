"""RoL rooms source grammar and target conversion."""

from __future__ import annotations

import re

from .rol_conversion_types import IdentityResolver, RolRecord, RolSourceCorpus, TransformResult
from .rol_source_common import (
    _collect_numeric_lines,
    _diagnostic,
    _exclude_record,
    _integers,
    _new_record,
    _next_content,
    _read_tilde,
    _reference,
    _segments,
)
from .rol_transform_common import (
    _TARGET_MAX_LEVEL,
    _encoded,
    _mapped_bits,
    _source_mask_bits,
    _tilde,
)
from .source import SourceFile


def _parse_wld(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  version_line = source.lines[0].text.strip() if source.lines else "missing"
  version_match = re.search(r"File Version\s+([0-9.]+)", version_line)
  version = version_match.group(1) if version_match else version_line
  corpus.file_versions[("wld", version)] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("wld", "sentinel")] += 1
      continue
    record = _new_record(source, basename, "wld", start, end, vnum)
    position = start + 1
    position, record.identity, ok = _read_tilde(source.lines, position, end)
    position, description, desc_ok = _read_tilde(source.lines, position, end)
    record.values["strings"] = {
        "name": record.identity,
        "description": description,
    }
    record.complete = ok and desc_ok
    position, base_line = _next_content(source.lines, position, end)
    if base_line is None or len(_integers(base_line)) < 3:
      record.complete = False
      _diagnostic(corpus, "ROLWLD001", "error", "room base row is missing or short", source.lines[start], "wld", vnum)
      records.append(record)
      continue
    base_values = _integers(base_line)
    position, base_values, _ = _collect_numeric_lines(
        source.lines, position, end, base_values, 7
    )
    record.values["base"] = base_values
    record.directives.append({"token": "BASE", "line": base_line.number, "field_count": len(base_values)})

    terminated = False
    while position < end:
      position, line = _next_content(source.lines, position, end)
      if line is None:
        break
      token = line.raw.strip().split(maxsplit=1)[0].decode("ascii", errors="replace")
      if token == "S":
        record.directives.append({"token": "S", "line": line.number})
        terminated = True
        break
      if re.fullmatch(r"D\d+", token):
        direction = int(token[1:])
        position, exit_description, first_ok = _read_tilde(
            source.lines, position, end
        )
        position, exit_keyword, second_ok = _read_tilde(
            source.lines, position, end
        )
        position, numeric = _next_content(source.lines, position, end)
        values = _integers(numeric) if numeric is not None else []
        if values and values[0] >= 16:
          position, values, _ = _collect_numeric_lines(
              source.lines, position, end, values, 10
          )
        record.directives.append(
            {
                "token": "D",
                "line": line.number,
                "direction": direction,
                "arguments": values,
                "description": exit_description,
                "keyword": exit_keyword,
            }
        )
        if numeric is None or len(values) < 3 or not first_ok or not second_ok:
          if numeric is not None and len(values) == 2 and first_ok and second_ok:
            record.directives[-1]["source_defaulted_destination"] = True
            _diagnostic(
                corpus,
                "ROLWLD002",
                "warning",
                "source exit omits its destination; exclude the smallest exit instruction",
                line,
                "wld",
                vnum,
            )
          else:
            record.complete = False
            _diagnostic(corpus, "ROLWLD002", "error", "room exit block is incomplete", line, "wld", vnum)
        else:
          _reference(record, "object", values[1], "exit_key", numeric)
          _reference(record, "room", values[2], "exit_destination", numeric)
        continue
      if token == "E":
        position, keyword, first_ok = _read_tilde(source.lines, position, end)
        position, description, second_ok = _read_tilde(
            source.lines, position, end
        )
        record.directives.append(
            {
                "token": "E",
                "line": line.number,
                "keyword": keyword,
                "description": description,
            }
        )
        if not first_ok or not second_ok:
          record.complete = False
          _diagnostic(corpus, "ROLWLD003", "error", "extra-description block is incomplete", line, "wld", vnum)
        continue
      if token in {"R", "F", "M"}:
        values = _integers(line)
        minimum = {"R": 2, "F": 1, "M": 2}[token]
        position, values, _ = _collect_numeric_lines(
            source.lines, position, end, values, minimum
        )
        record.directives.append({"token": token, "line": line.number, "arguments": values})
        continue
      if re.fullmatch(r"D\s+\d+", line.raw.strip().decode("ascii", errors="replace")):
        _exclude_record(
            corpus,
            record,
            "ROLWLD004",
            "source room uses a split direction opcode that its loader cannot parse",
            line,
        )
        terminated = True
        break
      record.complete = False
      _diagnostic(corpus, "ROLWLD004", "error", f"unknown room directive {token!r}", line, "wld", vnum)

    if not terminated:
      record.complete = False
      _diagnostic(corpus, "ROLWLD005", "error", "room record lacks S terminator", source.lines[start], "wld", vnum)
    records.append(record)
  corpus.file_terminators[("wld", "present" if any(line.raw.strip().startswith(b"$") for line in source.lines) else "absent")] += 1
  return records


ROOM_FLAG_MAP = {
    1: 0,   # DARK
    2: 1,   # DEATH
    3: 2,   # NO_MOB
    4: 3,   # INDOORS
    5: 5,   # ROOM_SILENT
    7: 19,  # NORECALL
    8: 7,   # NO_MAGIC
    9: 8,   # TUNNEL
    10: 9,  # PRIVATE
    11: 43,  # ARENA
    12: 4,  # SAFE_ZONE
    13: 42,  # NO_PRECIP
    14: 20, # SINGLE_FILE
    15: 44,  # JAIL (RoL compatibility marker)
    16: 21, # NO_TELEPORT
    17: 10,  # RESERVED_OLC -> STAFFROOM
    18: 17,  # HEAL
    19: 25, # NO_HEAL
    20: 33, # HAS_TRAP
    21: 41, # DOCKABLE
    22: 22, # MAGIC_DARK
    23: 23, # MAGIC_LIGHT
    24: 24, # NO_SUMMON
    30: 28, # AIRY_WATER
    31: 27,  # SOLID_FOG
    33: 11, # ROOM_HOUSE
    34: 13, # ROOM_ATRIUM
    36: 45,  # PSPREGEN
    48: 27,  # FIRE_FOG (also adds MAGIC_LIGHT below)
}


# Source zone behavior is persisted on every emitted room because the target
# zone format has no separate flags for silence, safety, magic, recall, or
# summon restrictions. Keys are source bit ordinals, not flag masks.
ZONE_ROOM_FLAG_MAP = {
    0: 5,   # ZONE_SILENT -> ROOM_SOUNDPROOF
    1: 4,   # ZONE_SAFE -> ROOM_PEACEFUL
    4: 21,  # ZONE_NO_TELE -> ROOM_NOTELEPORT
    5: 7,   # ZONE_NO_MAGIC -> ROOM_NOMAGIC
    6: 19,  # ZONE_NO_RECALL -> ROOM_NORECALL
    7: 24,  # ZONE_NO_SUMMON -> ROOM_NOSUMMON
}


# These source flags are handled by transform logic rather than a one-to-one
# persisted room flag.
ROOM_TRANSFORMED_FLAGS = frozenset({6, 32})


ZONE_SOURCE_ONLY_FLAGS = frozenset({2, 3, 8})


SOURCE_ASTRAL_SECTOR = 23


ROL_ASTRAL_ROOM_FLAG = 47


SECTOR_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 9,
    11: 25,
    12: 15,
    13: 19,
    14: 20,
    15: 21,
    16: 22,
    17: 23,
    18: 24,
    19: 18,
    20: 18,
    21: 18,
    22: 18,
    23: 18,
    24: 31,
    25: 23,
    26: 29,
    27: 5,
    28: 1,
    29: 35,
    30: 18,
    31: 25,
    100: 5,  # malformed room 50537 omitted its mountain-road sector value
}


def _room_size_bits(base: list[int]) -> set[int]:
  if len(base) < 6:
    return set()
  length, width, height = (max(0, value) for value in base[3:6])
  effective_height = (length + width) // 2 if height == 500 else height
  volume = length * width * effective_height
  if volume <= 27:
    return {31}
  if volume <= 125:
    return {30}
  if length < 6 or width < 6:
    return {20}
  return set()


def _exit_flags(source_flags: int) -> int:
  is_door = bool(source_flags & 0x1FF)
  pickproof = bool(source_flags & (1 << 8))
  hidden = bool(source_flags & (1 << 6))
  if not is_door:
    return 0
  blocked = bool(source_flags & (1 << 7))
  return 1 + int(pickproof) + (2 if hidden else 0) + (4 if blocked else 0)


def emit_room(
    record: RolRecord,
    destination_vnum: int,
    destination_zone: int,
    resolve: IdentityResolver,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
    source_zone_flags: int = 0,
    required_flag_bits: tuple[int, ...] = (),
) -> TransformResult:
  """Emit one modern target room record."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  name, text_diagnostics = _tilde(strings.get("name"))
  diagnostics.extend(text_diagnostics)
  description, text_diagnostics = _tilde(strings.get("description"))
  diagnostics.extend(text_diagnostics)
  base = record.values.get("base", [])
  first_mask = base[1] if len(base) > 1 else 0
  second_mask = base[6] if len(base) > 6 else 0
  source_flags = _source_mask_bits(first_mask, 1) | _source_mask_bits(second_mask, 33)
  target_flags = _mapped_bits(source_flags, ROOM_FLAG_MAP) | _room_size_bits(base)
  source_sector = base[2] if len(base) > 2 else 0
  if source_sector == SOURCE_ASTRAL_SECTOR:
    target_flags.add(ROL_ASTRAL_ROOM_FLAG)
  target_flags.update(required_flag_bits)
  source_zone_bits = _source_mask_bits(source_zone_flags, 0)
  target_flags.update(_mapped_bits(source_zone_bits, ZONE_ROOM_FLAG_MAP))
  if 32 in source_flags:
    target_flags.discard(7)
  if 48 in source_flags:
    target_flags.add(23)
  missing = sorted(source_flags - ROOM_FLAG_MAP.keys() - ROOM_TRANSFORMED_FLAGS)
  if missing:
    diagnostics.append(f"room flags without target persistence: {missing}")
  missing_zone = sorted(source_zone_bits - ZONE_ROOM_FLAG_MAP.keys() - ZONE_SOURCE_ONLY_FLAGS)
  if missing_zone:
    diagnostics.append(f"zone flags without room compatibility: {missing_zone}")
  if source_zone_bits & ZONE_SOURCE_ONLY_FLAGS:
    diagnostics.append(
        "preserved source-only zone metadata outside room flags: "
        f"{sorted(source_zone_bits & ZONE_SOURCE_ONLY_FLAGS)}"
    )
  sector = SECTOR_MAP.get(source_sector, 0)
  if 6 in source_flags:
    sector = 9
  lines = [
      f"#{destination_vnum}\n",
      name,
      description,
      f"{destination_zone} {_encoded(target_flags)} {sector}\n",
  ]
  for directive in record.directives:
    token = directive["token"]
    if token == "D":
      arguments = directive.get("arguments", [])
      if len(arguments) < 3 or directive.get("source_defaulted_destination"):
        diagnostics.append(f"excluded incomplete exit at source line {directive['line']}")
        continue
      exit_description, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      try:
        key = resolve("obj", arguments[1]) if arguments[1] > 0 else -1
      except (KeyError, ValueError) as error:
        key = -1
        diagnostics.append(
            f"removed unresolved exit key {arguments[1]} at source line "
            f"{directive['line']}: {error}"
        )
      try:
        destination = resolve("wld", arguments[2]) if arguments[2] > 0 else -1
      except (KeyError, ValueError) as error:
        diagnostics.append(
            f"excluded exit with unresolved destination {arguments[2]} at source line "
            f"{directive['line']}: {error}"
        )
        continue
      lines.extend(
          [
              f"D{directive['direction']}\n",
              exit_description,
              keyword,
              f"{_exit_flags(arguments[0])} {key} {destination}\n",
          ]
      )
      if len(arguments) > 3:
        valid_trap = (
            len(arguments) == 10
            and arguments[0] >= 16
            and arguments[3] in {0, 1}
            and arguments[4] in {1, 2, 3, 4, 5, 10, 11}
            and 0 <= arguments[5] <= arguments[6] <= 32766
            and arguments[7] in {0, 1}
            and -100 <= arguments[8] <= 100
            and 0 <= arguments[9] <= 100
        )
        if valid_trap:
          lines.append(
              f"Y {directive['direction']} {arguments[3]} {arguments[4]} "
              f"{arguments[5]} {arguments[6]} {arguments[7]} {arguments[8]} "
              f"{arguments[9]}\n"
          )
          diagnostics.append(
              f"adapted legacy exit trap payload at source line {directive['line']}"
          )
        else:
          diagnostics.append(
              f"excluded malformed legacy exit trap payload at source line {directive['line']}"
          )
    elif token == "E":
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      extra, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      lines.extend(["E\n", keyword, extra])
    elif token == "R":
      arguments = directive.get("arguments", [])
      if len(arguments) < 2:
        diagnostics.append(
            f"excluded incomplete room level range at source line {directive['line']}"
        )
        continue
      minimum_level, maximum_level = arguments[:2]
      if minimum_level < 1:
        if minimum_level != -1:
          diagnostics.append(
              f"normalized source room minimum level {minimum_level} to unrestricted at "
              f"source line {directive['line']}"
          )
        minimum_level = -1
      elif minimum_level > _TARGET_MAX_LEVEL:
        raise ValueError(
            f"source room minimum level {minimum_level} exceeds target maximum "
            f"{_TARGET_MAX_LEVEL} at "
            f"source line {directive['line']}"
        )
      if maximum_level < 1 or maximum_level > _TARGET_MAX_LEVEL:
        if maximum_level != -1:
          diagnostics.append(
              f"normalized source room maximum level {maximum_level} to unrestricted at "
              f"source line {directive['line']} because the target maximum level is "
              f"{_TARGET_MAX_LEVEL}"
          )
        maximum_level = -1
      if minimum_level > 0 and maximum_level > 0 and minimum_level > maximum_level:
        raise ValueError(
            f"source room level range {minimum_level}..{maximum_level} is reversed at "
            f"source line {directive['line']}"
        )
      lines.append(f"R {minimum_level} {maximum_level}\n")
    elif token == "F":
      diagnostics.append(
          f"omitted source-inert room fall chance at source line {directive['line']}; "
          "the source loader stores and validates it but no runtime path consumes it"
      )
    elif token == "M":
      diagnostics.append(
          f"omitted obsolete source room mana at source line {directive['line']}; the "
          "source runtime never consumes it and target M is reserved for moving rooms"
      )
  if special_proc is not None:
    lines.extend(["Z\n", f"{special_proc}\n"])
  lines.extend(["C\n", "0 0\n", "S\n"])
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)
