"""RoL zones source grammar and target conversion."""

from __future__ import annotations

import re

from .rol_conversion_types import IdentityResolver, RolRecord, RolSourceCorpus, TransformResult
from .rol_source_common import (
    _diagnostic,
    _integers,
    _new_record,
    _next_content,
    _read_tilde,
    _reference,
    _segments,
)
from .rol_transform_common import _encoded, convert_text
from .source import SourceFile, SourceLine


_RESET_COMMANDS = frozenset({"M", "O", "P", "G", "E", "D", "R", "F", "X", "T", "L", "Z"})


def _leading_integers(line: SourceLine) -> list[int]:
  """Return the scanf-style integer prefix, excluding trailing reset comments."""

  values: list[int] = []
  for token in line.raw.strip().split():
    if re.fullmatch(br"[+-]?\d+", token) is None:
      break
    values.append(int(token))
  return values


def _reset_references(record: RolRecord, command: str, values: list[int], line: SourceLine) -> None:
  if command == "M" and len(values) >= 4:
    _reference(record, "mobile", values[1], "reset_mobile", line)
    _reference(record, "room", values[3], "reset_room", line)
  elif command == "O" and len(values) >= 4:
    _reference(record, "object", values[1], "reset_object", line)
    _reference(record, "room", values[3], "reset_room", line)
  elif command == "P" and len(values) >= 4:
    _reference(record, "object", values[1], "reset_object", line)
    _reference(record, "object", values[3], "reset_container", line)
  elif command in {"G", "E"} and len(values) >= 2:
    _reference(record, "object", values[1], "reset_object", line)
  elif command == "D" and len(values) >= 2:
    _reference(record, "room", values[1], "door_room", line)
  elif command == "R" and len(values) >= 3:
    _reference(record, "room", values[1], "remove_room", line)
    _reference(record, "object", values[2], "remove_object", line)
  elif command == "F" and len(values) >= 4:
    _reference(record, "room", values[1], "follow_room", line)
    _reference(record, "mobile", values[2], "follow_leader", line)
    _reference(record, "mobile", values[3], "follow_mobile", line)
  elif command == "X" and len(values) >= 3:
    _reference(record, "room", values[1], "removal_room", line, allow_negative=True)
    _reference(record, "mobile", values[2], "removal_mobile", line)


def _parse_zon(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("zon", "legacy")] += 1
  for start, end, vnum in _segments(source):
    record = _new_record(source, basename, "zon", start, end, vnum)
    position = start + 1
    position, filename, first_ok = _read_tilde(source.lines, position, end)
    position, second, second_ok = _read_tilde(source.lines, position, end)
    if second is not None and second.lstrip().startswith("<"):
      position, record.identity, third_ok = _read_tilde(source.lines, position, end)
      record.values["strings"] = {
          "filename": filename,
          "coordinates": second,
          "name": record.identity,
      }
    else:
      record.identity = second
      third_ok = True
      record.values["strings"] = {
          "filename": filename,
          "name": record.identity,
      }
    if not first_ok or not second_ok or not third_ok:
      record.complete = False
      _diagnostic(corpus, "ROLZON001", "error", "zone string header is incomplete", source.lines[start], "zon", vnum)
    position, header = _next_content(source.lines, position, end)
    header_values = _integers(header) if header is not None else []
    if len(header_values) not in {4, 7}:
      record.complete = False
      _diagnostic(corpus, "ROLZON002", "error", "zone header must have four or seven integers", header or source.lines[start], "zon", vnum)
    record.values["header"] = header_values
    if header is not None:
      record.directives.append({"token": "HEADER", "line": header.number, "field_count": len(header_values)})
    climate_rows: list[list[int]] = []
    for _ in range(6):
      position, climate = _next_content(source.lines, position, end)
      if climate is None:
        record.complete = False
        break
      values = _integers(climate)
      climate_rows.append(values)
      record.directives.append({"token": "CLIMATE", "line": climate.number, "field_count": len(values)})
    record.values["climate"] = climate_rows

    terminated = False
    while position < end:
      line = source.lines[position]
      position += 1
      stripped = line.raw.lstrip()
      if not stripped or stripped.startswith(b"*"):
        continue
      command = chr(stripped[0])
      if command == "S":
        record.directives.append({"token": "S", "line": line.number})
        terminated = True
        break
      if command in _RESET_COMMANDS:
        values = _leading_integers(
            SourceLine(line.number, stripped[1:], line.newline, line.display_path)
        )
        if command == "G" and stripped.startswith((b"GROUPING", b"GATE QUEST STUFF")):
          record.directives.append({"token": "G_SOURCE_DEFECT", "line": line.number})
          _diagnostic(corpus, "ROLZON003", "warning", "source heading is misread as a malformed G reset", line, "zon", vnum)
          continue
        record.directives.append(
            {
                "token": command,
                "line": line.number,
                "arguments": values,
                "no_space_opcode": len(stripped) > 1 and stripped[1:2].isdigit(),
            }
        )
        _reset_references(record, command, values, line)
        continue
      record.complete = False
      _diagnostic(corpus, "ROLZON004", "error", f"unknown reset-stream token {command!r}", line, "zon", vnum)
    if not terminated:
      record.complete = False
      _diagnostic(corpus, "ROLZON005", "error", "zone record lacks S terminator", source.lines[start], "zon", vnum)
    records.append(record)
  return records


# The zone E command's position argument is a source WEAR_* constant
# (EXAMPLE/RealmsOfLuminari/src/structs.h:1120-1146), resolved at reset through
# restore_wear[] (EXAMPLE/RealmsOfLuminari/src/files.c:547). It is not an index
# into the source equipment_types[] display table, which omits SECONDARY_WEAPON
# and therefore runs one short from 17 up. Source 25 is the real WEAR_TAIL
# position; source position 24 is also normalized contextually for tail objects
# because affected RoL resets rely on try_wear() to recover from that bad slot.
EQUIPMENT_POSITION_MAP = {
    **{position: position for position in range(17)},
    17: 18,  # SECONDARY_WEAPON -> WEAR_WIELD_OFFHAND
    18: 17,  # HOLD -> WEAR_HOLD_1
    19: 26,  # WEAR_EYES -> WEAR_EYES
    20: 22,  # WEAR_FACE -> WEAR_FACE
    21: 24,  # WEAR_EARRING_R -> WEAR_EAR_R
    22: 25,  # WEAR_EARRING_L -> WEAR_EAR_L
    23: 23,  # WEAR_QUIVER -> WEAR_AMMO_POUCH
    24: 27,  # GUILD_INSIGNIA -> WEAR_BADGE
    25: 43,  # WEAR_TAIL -> WEAR_TAIL
}


def _reset_probability(command: str, arguments: list[int]) -> int:
  if command in {"G", "R"}:
    value = arguments[3] if len(arguments) >= 4 else 100
  else:
    value = arguments[4] if len(arguments) >= 5 else 100
  return min(100, max(0, value))


def _emit_reset(
    directive: dict[str, object],
    resolve: IdentityResolver,
    source_tail_only_objects: frozenset[int],
    source_tail_ring_objects: frozenset[int],
) -> tuple[str | None, list[str]]:
  command = str(directive["token"])
  arguments = [int(value) for value in directive.get("arguments", [])]
  line = int(directive["line"])
  diagnostics: list[str] = []

  if command != "F" and arguments:
    source_dependency = arguments[0]
    arguments[0] = 1 if source_dependency else 0
    if source_dependency not in {0, 1}:
      diagnostics.append(
          f"normalized source boolean dependency {source_dependency} to "
          f"{arguments[0]} at source line {line}"
      )

  try:
    if command in {"M", "O", "P", "E"}:
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, prototype, maximum, destination = arguments[:4]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      source_prototype = prototype
      target_kind = "mob" if command == "M" else "obj"
      prototype = resolve(target_kind, prototype)
      if command in {"M", "O"}:
        destination = resolve("wld", destination) if destination >= 0 else destination
      elif command == "P":
        destination = resolve("obj", destination)
      else:
        source_position = destination
        if source_prototype in source_tail_only_objects:
          destination = EQUIPMENT_POSITION_MAP[25]
          if source_position != 25:
            diagnostics.append(
                f"normalized dedicated tail object {source_prototype} from source "
                f"equipment position {source_position} to tail at source line {line}"
            )
        elif (
            source_prototype in source_tail_ring_objects
            and source_position in {24, 25}
        ):
          destination = EQUIPMENT_POSITION_MAP[25]
          if source_position == 24:
            diagnostics.append(
                f"normalized source tail-ring reset {source_prototype} from the "
                f"position-24 compatibility defect at source line {line}"
            )
        else:
          mapped_position = EQUIPMENT_POSITION_MAP.get(source_position)
          if mapped_position is None:
            raise ValueError(f"has unsupported equipment position {source_position}")
          destination = mapped_position
      probability = _reset_probability(command, arguments)
      return (
          f"{command} {dependency} {prototype} {maximum} {destination} {probability}\n",
          diagnostics,
      )

    if command == "G":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, prototype, maximum = arguments[:3]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      return (
          f"G {dependency} {resolve('obj', prototype)} {maximum} "
          f"{_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "D":
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, room, direction, state = arguments[:4]
      if room <= 0:
        raise ValueError(f"has non-positive room {room}")
      probability = _reset_probability(command, arguments)
      if 0 <= state <= 2 and probability == 100:
        return f"D {dependency} {resolve('wld', room)} {direction} {state}\n", diagnostics
      if state & 0x10:
        diagnostics.append(
            f"mapped legacy door-trap rearm bit at source line {line} to the RoL exit-trap runtime"
        )
      return (
          f"K {dependency} {resolve('wld', room)} {direction} {state} {probability}\n",
          diagnostics,
      )

    if command == "R":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
      if room <= 0 or prototype <= 0:
        raise ValueError(f"has non-positive room/prototype {room}/{prototype}")
      return (
          f"R {dependency} {resolve('wld', room)} {resolve('obj', prototype)} "
          f"{_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "F":
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      mode, room, leader, follower = arguments[:4]
      if mode not in {0, 1, 2, 3}:
        raise ValueError(f"has unsupported follow mode {mode}")
      if room <= 0 or leader <= 0 or follower <= 0:
        raise ValueError(
            f"has non-positive room/leader/follower {room}/{leader}/{follower}"
        )
      return (
          f"F {mode} {resolve('wld', room)} {resolve('mob', leader)} "
          f"{resolve('mob', follower)} 100\n",
          diagnostics,
      )

    if command == "X":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      combat_guard = arguments[3] if len(arguments) >= 4 else 0
      target_room = resolve("wld", room) if room >= 0 else -1
      return (
          f"X {dependency} {target_room} {resolve('mob', prototype)} "
          f"{combat_guard} {_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "T":
      if len(arguments) < 4:
        raise ValueError("requires dependency, hour, day, and weekday")
      dependency, hour, day, weekday = arguments[:4]
      month = arguments[4] if len(arguments) >= 5 else 0
      if day < 0:
        day = 0
      if weekday < 0:
        weekday = 0
      if month < 0:
        month = 0
      return f"C {dependency} {hour} {day} {weekday} {month}\n", diagnostics
  except (KeyError, ValueError) as error:
    diagnostics.append(f"excluded malformed {command} reset at source line {line}: {error}")
    return None, diagnostics

  diagnostics.append(f"excluded unsupported {command} reset at source line {line}")
  return None, diagnostics


def emit_zone(
    record: RolRecord,
    destination_vnum: int,
    destination_bottom: int,
    resolve: IdentityResolver,
    source_tail_only_objects: frozenset[int] = frozenset(),
    source_tail_ring_objects: frozenset[int] = frozenset(),
) -> TransformResult:
  """Emit a target zone and its normalized reset stream."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  name, text_diagnostics = convert_text(strings.get("name") or record.identity or "RoL zone")
  diagnostics.extend(text_diagnostics)
  name = name.replace("~", "-")
  header = list(record.values.get("header", []))
  if len(header) < 4:
    return TransformResult("", ["zone header has fewer than four numeric fields"])
  destination_top = resolve("wld", int(header[0]))
  lifespan = min(240, max(0, int(header[1])))
  source_reset_mode = int(header[2])
  reset_mode = source_reset_mode if source_reset_mode in {0, 1, 2} else 1
  if source_reset_mode not in {0, 1, 2}:
    diagnostics.append(
        f"source reset mode {source_reset_mode} mapped to target occupied-zone mode 1"
    )
  source_flags = int(header[3])
  target_flags = {18}
  if source_flags & (16 | 64 | 128):
    target_flags.add(5)
  if source_flags & ~(16 | 64 | 128):
    diagnostics.append(
        f"source zone flags without target zone equivalents: {source_flags & ~(16 | 64 | 128)}"
    )

  lines = [f"#{destination_vnum}\n", "RoL conversion~\n", f"{name}~\n"]
  lines.append(
      f"{destination_bottom} {destination_top} {lifespan} {reset_mode} "
      f"{_encoded(target_flags)} -1 -1 1 0 0 0\n"
  )

  current_mobile = False
  emitted_count = 0
  for directive in record.directives:
    if directive["token"] not in {"M", "O", "P", "G", "E", "D", "R", "F", "X", "T"}:
      continue
    if directive["token"] in {"G", "E"} and not current_mobile:
      diagnostics.append(
          f"excluded {directive['token']} reset without a staged mobile host at "
          f"source line {directive['line']}"
      )
      continue
    emitted, reset_diagnostics = _emit_reset(
        directive,
        resolve,
        source_tail_only_objects,
        source_tail_ring_objects,
    )
    diagnostics.extend(reset_diagnostics)
    if emitted is not None:
      if emitted_count == 0:
        parts = emitted.split(maxsplit=2)
        if len(parts) >= 2 and parts[0] != "F" and parts[1] != "0":
          diagnostics.append(
              f"normalized the first reset dependency {parts[1]} to 0 at source line "
              f"{directive['line']}"
          )
          parts[1] = "0"
          emitted = " ".join(parts)
      lines.append(emitted)
      emitted_count += 1
    if directive["token"] == "M":
      current_mobile = emitted is not None
  lines.extend(["S\n", "$\n"])
  return TransformResult("".join(lines), diagnostics)
