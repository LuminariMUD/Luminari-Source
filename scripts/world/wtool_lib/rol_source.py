"""Grammar-aware source model for the active Realms of Luminari corpus."""

from __future__ import annotations

from collections import Counter
from dataclasses import dataclass, field
import hashlib
from pathlib import Path
import re
from typing import Any, Iterable

from .models import TOOL_VERSION
from .rol_inventory import SOURCE_KINDS, build_rol_inventory
from .source import SourceFile, SourceLine


ROL_SOURCE_SCHEMA_VERSION = 1
_HEADER = re.compile(br"#(\d+)\s*$")
_INTEGER = re.compile(rb"[+-]?\d+")
_COLOR = re.compile(r"&\+[A-Za-z]|&[Nn]|@[A-Za-z]")
_SHOP_HEADER = re.compile(br"SHOP\s*:\s*([+-]?\d+)", re.IGNORECASE)
_SOC_HEADER = re.compile(
    br"MOB\s*:\s*([+-]?\d+)\s+(PERIODIC|TRIGGER|TIMED|LIST|PATH)\b",
    re.IGNORECASE,
)
_SOC_LOOSE_HEADER = re.compile(
    br"MOB\s+([+-]?\d+)\s+(PERIODIC|TRIGGER|TIMED|LIST|PATH)\b",
    re.IGNORECASE,
)
_RESET_COMMANDS = frozenset({"M", "O", "P", "G", "E", "D", "R", "F", "X", "T", "L", "Z"})
_SHOP_KEYWORDS = frozenset(
    {
        "BT",
        "CASTING",
        "CHEATS",
        "DEADBEAT",
        "GREED",
        "HATES",
        "HOURS",
        "KILLABLE",
        "MBCASH",
        "MBHAVE",
        "MBIGOT",
        "MBUY",
        "MCLOSE",
        "MNBUY",
        "MOPEN",
        "MSCASH",
        "MSELL",
        "MSHAVE",
        "OFFENSE",
        "PO",
        "PROFIT",
        "ROAMING",
        "ROOM",
        "SHOP",
    }
)
_SOC_KEYS = frozenset(
    {
        "FLAG",
        "DELAY",
        "CHANCE",
        "ACTION",
        "TRIGGER",
        "HOUR",
        "ID",
        "TYPE",
        "ROOMS",
        "DIRS",
        "DONE",
        "LISTDONE",
    }
)


@dataclass(frozen=True, slots=True)
class RolReference:
  target_type: str
  target_vnum: int
  role: str
  path: str
  line: int

  def to_dict(self) -> dict[str, Any]:
    return {
        "target_type": self.target_type,
        "target_vnum": self.target_vnum,
        "role": self.role,
        "path": self.path,
        "line": self.line,
    }


@dataclass(frozen=True, slots=True)
class RolDiagnostic:
  code: str
  severity: str
  message: str
  path: str
  line: int
  record_type: str | None = None
  record_vnum: int | None = None

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "code": self.code,
        "severity": self.severity,
        "message": self.message,
        "path": self.path,
        "line": self.line,
    }
    if self.record_type is not None:
      data["record_type"] = self.record_type
    if self.record_vnum is not None:
      data["record_vnum"] = self.record_vnum
    return data


@dataclass(slots=True)
class RolRecord:
  kind: str
  vnum: int
  basename: str
  path: str
  line: int
  end_line: int
  sha256: str
  identity: str | None = None
  format_version: str | int | None = None
  values: dict[str, Any] = field(default_factory=dict)
  directives: list[dict[str, Any]] = field(default_factory=list)
  references: list[RolReference] = field(default_factory=list)
  complete: bool = True

  @property
  def record_id(self) -> str:
    return f"{self.kind}:{self.vnum}:{self.path}:{self.line}"

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "record_id": self.record_id,
        "kind": self.kind,
        "vnum": self.vnum,
        "basename": self.basename,
        "path": self.path,
        "line": self.line,
        "end_line": self.end_line,
        "sha256": self.sha256,
        "complete": self.complete,
        "directives": self.directives,
        "references": [reference.to_dict() for reference in self.references],
    }
    if self.identity is not None:
      data["identity"] = self.identity
      data["normalized_identity"] = normalize_identity(self.identity)
    if self.format_version is not None:
      data["format_version"] = self.format_version
    if self.values:
      data["values"] = self.values
    return data


@dataclass(slots=True)
class RolSourceCorpus:
  records: list[RolRecord] = field(default_factory=list)
  diagnostics: list[RolDiagnostic] = field(default_factory=list)
  file_versions: Counter[tuple[str, str]] = field(default_factory=Counter)
  file_terminators: Counter[tuple[str, str]] = field(default_factory=Counter)

  @property
  def complete(self) -> bool:
    return all(record.complete for record in self.records) and not any(
        diagnostic.severity == "error" for diagnostic in self.diagnostics
    )


def normalize_identity(value: str) -> str:
  value = _COLOR.sub(" ", value)
  value = value.replace("\r", " ").replace("\n", " ")
  value = re.sub(r"[^A-Za-z0-9]+", " ", value).strip().lower()
  return " ".join(value.split())


def _line_bytes(lines: list[SourceLine], start: int, end: int) -> bytes:
  return b"".join(line.raw + line.newline for line in lines[start:end])


def _segment_hash(lines: list[SourceLine], start: int, end: int) -> str:
  return hashlib.sha256(_line_bytes(lines, start, end)).hexdigest()


def _segments(source: SourceFile) -> list[tuple[int, int, int]]:
  headers: list[tuple[int, int]] = []
  for index, line in enumerate(source.lines):
    match = _HEADER.fullmatch(line.raw)
    if match is not None:
      headers.append((index, int(match.group(1))))
  return [
      (start, headers[index + 1][0] if index + 1 < len(headers) else len(source.lines), vnum)
      for index, (start, vnum) in enumerate(headers)
  ]


def _next_content(lines: list[SourceLine], position: int, end: int) -> tuple[int, SourceLine | None]:
  while position < end:
    line = lines[position]
    position += 1
    stripped = line.raw.strip()
    if not stripped or stripped.startswith(b"*") or stripped.startswith(b";;"):
      continue
    return position, line
  return position, None


def _read_tilde(
    lines: list[SourceLine],
    position: int,
    end: int,
) -> tuple[int, str | None, bool]:
  pieces: list[bytes] = []
  while position < end:
    line = lines[position]
    position += 1
    stripped = line.raw.rstrip()
    if stripped.endswith(b"~"):
      pieces.append(stripped[:-1])
      return position, b"\r\n".join(pieces).decode("utf-8", errors="surrogateescape"), True
    pieces.append(line.raw)
  value = b"\r\n".join(pieces).decode("utf-8", errors="surrogateescape")
  return position, value or None, False


def _integers(line: SourceLine) -> list[int]:
  return [int(token) for token in _INTEGER.findall(line.raw)]


def _leading_integers(line: SourceLine) -> list[int]:
  """Return the scanf-style integer prefix, excluding trailing reset comments."""

  values: list[int] = []
  for token in line.raw.strip().split():
    if re.fullmatch(br"[+-]?\d+", token) is None:
      break
    values.append(int(token))
  return values


def _numeric_line(line: SourceLine) -> bool:
  return re.fullmatch(br"[+-]?\d+(?:\s+[+-]?\d+)*", line.raw.strip()) is not None


def _collect_numeric_lines(
    lines: list[SourceLine],
    position: int,
    end: int,
    values: list[int],
    minimum: int,
) -> tuple[int, list[int], SourceLine | None]:
  """Collect scanf-style integer arguments without consuming the next directive."""

  last_line: SourceLine | None = None
  while len(values) < minimum and position < end:
    probe = position
    next_position, line = _next_content(lines, probe, end)
    if line is None or not _numeric_line(line):
      break
    position = next_position
    values.extend(_integers(line))
    last_line = line
  return position, values, last_line


def _quest_arguments(
    lines: list[SourceLine],
    position: int,
    end: int,
    command_line: SourceLine,
    expected_values: int,
) -> tuple[int, str, list[int], SourceLine]:
  """Read the whitespace-insensitive G/R payload used by the source fscanf loader."""

  payload = command_line.raw.strip()[1:].strip()
  argument_line = command_line
  while position < end:
    subtype_match = re.match(br"\s*([A-Za-z])", payload)
    values = [int(value) for value in _INTEGER.findall(payload[subtype_match.end():])] if subtype_match else []
    minimum = expected_values
    if subtype_match is not None and expected_values == 0:
      minimum = 0 if subtype_match.group(1).upper() == b"A" else 1
    if subtype_match is not None and len(values) >= minimum:
      return position, subtype_match.group(1).decode("ascii").upper(), values, argument_line
    next_position, line = _next_content(lines, position, end)
    if line is None:
      break
    payload += b" " + line.raw.strip()
    position = next_position
    argument_line = line
  subtype_match = re.match(br"\s*([A-Za-z])", payload)
  subtype = subtype_match.group(1).decode("ascii").upper() if subtype_match else ""
  values = [int(value) for value in _INTEGER.findall(payload[subtype_match.end():])] if subtype_match else []
  return position, subtype, values, argument_line


def _diagnostic(
    corpus: RolSourceCorpus,
    code: str,
    severity: str,
    message: str,
    line: SourceLine,
    kind: str | None = None,
    vnum: int | None = None,
) -> None:
  corpus.diagnostics.append(
      RolDiagnostic(code, severity, message, line.display_path, line.number, kind, vnum)
  )


def _exclude_record(
    corpus: RolSourceCorpus,
    record: RolRecord,
    code: str,
    message: str,
    line: SourceLine,
) -> None:
  """Account for a source record that cannot be loaded without inventing intent."""

  record.complete = True
  record.values["source_disposition"] = "EXCLUDE"
  record.values["source_exclusion_reason"] = message
  record.directives.append({"token": "EXCLUDED_SOURCE_RECORD", "line": line.number})
  _diagnostic(corpus, code, "warning", message, line, record.kind, record.vnum)


def _reference(
    record: RolRecord,
    target_type: str,
    target_vnum: int,
    role: str,
    line: SourceLine,
    allow_negative: bool = False,
) -> None:
  if target_vnum < 0 and not allow_negative:
    return
  record.references.append(
      RolReference(target_type, target_vnum, role, line.display_path, line.number)
  )


def _new_record(
    source: SourceFile,
    basename: str,
    kind: str,
    start: int,
    end: int,
    vnum: int,
) -> RolRecord:
  return RolRecord(
      kind=kind,
      vnum=vnum,
      basename=basename,
      path=source.display_path,
      line=source.lines[start].number,
      end_line=source.lines[end - 1].number if end > start else source.lines[start].number,
      sha256=_segment_hash(source.lines, start, end),
  )


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


def _parse_mob(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  version_line = source.lines[0].text.strip() if source.lines else "missing"
  version_match = re.search(r"File Version\s+(\d+)", version_line)
  version: str | int = int(version_match.group(1)) if version_match else version_line
  corpus.file_versions[("mob", str(version))] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("mob", "sentinel")] += 1
      continue
    position = start + 1
    record = _new_record(source, basename, "mob", start, end, vnum)
    strings: list[str | None] = []
    strings_ok = True
    for _ in range(4):
      position, value, ok = _read_tilde(source.lines, position, end)
      strings.append(value)
      strings_ok = strings_ok and ok
    record.identity = strings[1]
    record.format_version = version
    record.values["strings"] = {
        "aliases": strings[0],
        "short_description": strings[1],
        "long_description": strings[2],
        "description": strings[3],
    }
    position, flags_line = _next_content(source.lines, position, end)
    flag_tokens = flags_line.text.split() if flags_line is not None else []
    if not strings_ok or flags_line is None or len(flag_tokens) < 5:
      record.complete = False
      _diagnostic(corpus, "ROLMOB001", "error", "mobile strings or flag row are incomplete", source.lines[start], "mob", vnum)
      records.append(record)
      continue
    letter = flag_tokens[4]
    record.values["format_letter"] = letter
    record.values["flags"] = flag_tokens
    record.directives.append({"token": "FLAGS", "line": flags_line.number, "field_count": len(flag_tokens)})
    if letter != "S":
      record.complete = False
      _diagnostic(corpus, "ROLMOB002", "error", f"unsupported mobile format letter {letter!r}", flags_line, "mob", vnum)
    base_rows: list[list[str]] = []
    for token in ("RACE", "COMBAT", "MONEY", "POSITION"):
      position, line = _next_content(source.lines, position, end)
      if line is None:
        _exclude_record(
            corpus,
            record,
            "ROLMOB003",
            f"source mobile lacks its {token.lower()} row",
            source.lines[start],
        )
        break
      values = line.text.split()
      base_rows.append(values)
      record.directives.append({"token": token, "line": line.number, "field_count": len(values)})
    record.values["base_rows"] = base_rows
    position, extra = _next_content(source.lines, position, end)
    if extra is not None and not extra.raw.strip().startswith(b"$"):
      record.complete = False
      _diagnostic(corpus, "ROLMOB004", "error", "unconsumed mobile record content", extra, "mob", vnum)
    records.append(record)
  return records


def _parse_obj(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("obj", "legacy")] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("obj", "sentinel")] += 1
      continue
    record = _new_record(source, basename, "obj", start, end, vnum)
    position = start + 1
    strings: list[str | None] = []
    strings_ok = True
    for _ in range(4):
      position, value, ok = _read_tilde(source.lines, position, end)
      strings.append(value)
      strings_ok = strings_ok and ok
    record.identity = strings[1]
    record.values["strings"] = {
        "aliases": strings[0],
        "short_description": strings[1],
        "description": strings[2],
        "action_description": strings[3],
    }
    rows: list[SourceLine] = []
    for token in ("FLAGS", "VALUES", "ECONOMY"):
      position, line = _next_content(source.lines, position, end)
      if line is None:
        _exclude_record(
            corpus,
            record,
            "ROLOBJ001",
            f"source object lacks its {token.lower()} row",
            source.lines[start],
        )
        break
      rows.append(line)
      record.directives.append({"token": token, "line": line.number, "field_count": len(_integers(line))})
    if not strings_ok:
      _exclude_record(
          corpus,
          record,
          "ROLOBJ002",
          "source object string block is incomplete",
          source.lines[start],
      )
    if len(rows) == 3:
      flags = _integers(rows[0])
      values = _integers(rows[1])
      economy = _integers(rows[2])
      record.values.update(
          {
              "item_type": flags[0] if flags else None,
              "flags": flags,
              "values": values,
              "economy": economy,
          }
      )
      item_type = flags[0] if flags else None
      if item_type == 15 and len(values) >= 3:
        _reference(record, "object", values[2], "container_key", rows[1])
      elif item_type == 25 and values:
        _reference(record, "room", values[0], "teleport_destination", rows[1])
      elif item_type == 27 and len(values) >= 2:
        _reference(record, "mobile", values[1], "summoned_mobile", rows[1])
      elif item_type == 29 and len(values) >= 2:
        _reference(record, "room", values[1], "switch_room", rows[1])

    affect_flag_rows = 0
    saw_extension = False
    while position < end:
      position, line = _next_content(source.lines, position, end)
      if line is None:
        break
      stripped = line.raw.strip()
      token = stripped[:1].decode("ascii", errors="replace")
      if stripped.startswith(b"$"):
        corpus.file_terminators[("obj", "present")] += 1
        break
      if token == "E":
        saw_extension = True
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
          record.directives[-1]["source_disposition"] = "EXCLUDE"
          _diagnostic(
              corpus,
              "ROLOBJ003",
              "warning",
              "source object extra-description is incomplete; exclude the extension",
              line,
              "obj",
              vnum,
          )
      elif token == "A":
        saw_extension = True
        values = _integers(line)
        position, values, _ = _collect_numeric_lines(
            source.lines, position, end, values, 2
        )
        record.directives.append({"token": "A", "line": line.number, "arguments": values})
      elif token == "T":
        saw_extension = True
        values = _integers(line)
        position, values, _ = _collect_numeric_lines(
            source.lines, position, end, values, 6
        )
        record.directives.append({"token": "T", "line": line.number, "arguments": values})
      elif re.fullmatch(br"[+-]?\d+(?:\s+[+-]?\d+)*", stripped):
        values = _integers(line)
        if not saw_extension and affect_flag_rows < 2:
          affect_flag_rows += 1
          record.directives.append(
              {
                  "token": "AFFECT_FLAGS",
                  "line": line.number,
                  "field_count": len(values),
                  "arguments": values,
              }
          )
        else:
          saw_extension = True
          record.directives.append(
              {"token": "IGNORED_SOURCE_CONTENT", "line": line.number}
          )
          _diagnostic(
              corpus,
              "ROLOBJ004",
              "warning",
              "source object loader ignores numeric content after extensions",
              line,
              "obj",
              vnum,
          )
      else:
        saw_extension = True
        record.directives.append(
            {"token": "IGNORED_SOURCE_CONTENT", "line": line.number}
        )
        _diagnostic(
            corpus,
            "ROLOBJ004",
            "warning",
            "source object loader ignores unrecognized trailing content",
            line,
            "obj",
            vnum,
        )
    records.append(record)
  return records


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


def _parse_qst(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("qst", "legacy")] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("qst", "sentinel")] += 1
      continue
    record = _new_record(source, basename, "qst", start, end, vnum)
    record.identity = f"host {vnum}"
    _reference(record, "mobile", vnum, "quest_host", source.lines[start])
    position = start + 1
    terminated = False
    while position < end:
      position, line = _next_content(source.lines, position, end)
      if line is None:
        break
      token = line.raw.strip()[:1].decode("ascii", errors="replace")
      if token == "S":
        record.directives.append({"token": "S", "line": line.number})
        terminated = True
        break
      if token == "M":
        position, keyword, first_ok = _read_tilde(source.lines, position, end)
        position, message, second_ok = _read_tilde(source.lines, position, end)
        record.directives.append(
            {
                "token": "M",
                "line": line.number,
                "keyword": keyword,
                "message": message,
            }
        )
        if not first_ok or not second_ok:
          record.complete = False
        continue
      if token in {"Q", "D"}:
        position, message, ok = _read_tilde(source.lines, position, end)
        record.directives.append(
            {"token": token, "line": line.number, "message": message}
        )
        if not ok:
          record.complete = False
        continue
      if token in {"G", "R"}:
        expected_values = 1 if token == "G" else 0
        position, subtype, values, argument_line = _quest_arguments(
            source.lines, position, end, line, expected_values
        )
        record.directives.append(
            {"token": token, "subtype": subtype, "line": line.number, "arguments": values}
        )
        minimum = 0 if token == "R" and subtype == "A" else 1
        if not subtype or len(values) < minimum:
          record.complete = False
          _diagnostic(corpus, "ROLQST001", "error", "quest goal row is incomplete", line, "qst", vnum)
          continue
        number = values[0] if values else 0
        destination = values[1] if len(values) > 1 else 0
        if token == "G" and subtype == "I":
          _reference(record, "object", number, "quest_required_item", argument_line)
        elif token == "R" and subtype == "I":
          _reference(record, "object", number, "quest_reward_item", argument_line)
          if destination > number and destination - number < 100:
            _reference(record, "object", destination, "quest_random_reward_end", argument_line)
        elif token == "R" and subtype == "M":
          _reference(record, "mobile", number, "quest_load_mobile", argument_line)
          _reference(record, "room", destination, "quest_load_destination", argument_line)
        elif token == "R" and subtype == "O":
          _reference(record, "object", number, "quest_load_object", argument_line)
          _reference(record, "room", destination, "quest_load_destination", argument_line)
        continue
      record.complete = False
      _diagnostic(corpus, "ROLQST002", "error", f"unknown quest directive {token!r}", line, "qst", vnum)
    if not terminated:
      _exclude_record(
          corpus,
          record,
          "ROLQST003",
          "source quest block is incomplete or lacks its S terminator",
          source.lines[start],
      )
    records.append(record)
  corpus.file_terminators[("qst", "present" if any(line.raw.strip().startswith(b"$") for line in source.lines) else "absent")] += 1
  return records


def _parse_shp(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("shp", "keyword")] += 1
  headers: list[tuple[int, int]] = []
  for index, line in enumerate(source.lines):
    cleaned = line.raw.split(b";;", 1)[0].strip()
    match = _SHOP_HEADER.match(cleaned)
    if match is not None:
      headers.append((index, int(match.group(1))))
  for ordinal, (start, vnum) in enumerate(headers):
    end = headers[ordinal + 1][0] if ordinal + 1 < len(headers) else len(source.lines)
    record = _new_record(source, basename, "shp", start, end, vnum)
    record.identity = f"keeper {vnum}"
    _reference(record, "mobile", vnum, "shop_keeper", source.lines[start])
    for line in source.lines[start:end]:
      cleaned = line.raw.split(b";;", 1)[0].strip()
      if not cleaned:
        continue
      if b":" not in cleaned:
        _diagnostic(
            corpus,
            "ROLSHP001",
            "warning",
            "source shop loader ignores content without a keyword colon",
            line,
            "shp",
            vnum,
        )
        record.directives.append({"token": "IGNORED_SOURCE_CONTENT", "line": line.number})
        continue
      raw_key, raw_value = cleaned.split(b":", 1)
      key = raw_key.decode("ascii", errors="replace").upper()
      values = [int(value) for value in _INTEGER.findall(raw_value)]
      if key not in _SHOP_KEYWORDS:
        _diagnostic(
            corpus,
            "ROLSHP002",
            "warning",
            f"source shop loader ignores unknown keyword {key!r}",
            line,
            "shp",
            vnum,
        )
        record.directives.append({"token": "IGNORED_SOURCE_KEYWORD", "line": line.number})
        continue
      record.directives.append(
          {
              "token": key,
              "line": line.number,
              "arguments": values,
              "text": raw_value.strip().decode(
                  "utf-8", errors="surrogateescape"
              ),
          }
      )
      if key == "ROOM":
        for value in values:
          _reference(record, "room", value, "shop_room", line)
      elif key == "PO":
        for value in values:
          _reference(record, "object", value, "shop_product", line)
    records.append(record)
  return records


def _parse_soc(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  corpus.file_versions[("soc", "keyword")] += 1
  headers: list[tuple[int, int, str]] = []
  for index, line in enumerate(source.lines):
    cleaned = line.raw.split(b";;", 1)[0].strip()
    match = _SOC_HEADER.match(cleaned)
    if match is not None:
      headers.append((index, int(match.group(1)), match.group(2).decode("ascii").upper()))
    elif _SOC_LOOSE_HEADER.match(cleaned) is not None:
      _diagnostic(
          corpus,
          "ROLSOC001",
          "warning",
          "source SOC loader ignores MOB header without a colon",
          line,
          "soc",
          int(_SOC_LOOSE_HEADER.match(cleaned).group(1)),
      )
  for ordinal, (start, vnum, mode) in enumerate(headers):
    end = headers[ordinal + 1][0] if ordinal + 1 < len(headers) else len(source.lines)
    record = _new_record(source, basename, "soc", start, end, vnum)
    record.identity = f"mobile {vnum} {mode.lower()}"
    record.format_version = mode
    _reference(record, "mobile", vnum, "soc_mobile", source.lines[start])
    position = start + 1
    while position < end:
      line = source.lines[position]
      position += 1
      cleaned = line.raw.split(b";;", 1)[0].strip()
      if not cleaned:
        continue
      if b":" in cleaned:
        raw_key, raw_value = cleaned.split(b":", 1)
        key = raw_key.decode("ascii", errors="replace").upper()
      else:
        key = cleaned.decode("ascii", errors="replace").upper()
        raw_value = b""
      if key not in _SOC_KEYS:
        record.complete = False
        _diagnostic(corpus, "ROLSOC001", "error", f"unknown SOC keyword {key!r}", line, "soc", vnum)
        continue
      values = [int(value) for value in _INTEGER.findall(raw_value)]
      record.directives.append({"token": key, "line": line.number, "arguments": values})
      if key == "LISTDONE" or key == "DONE" and mode != "LIST":
        break
      if key == "ROOMS":
        for value in values:
          _reference(record, "room", value, "soc_path_room", line)
      if key == "ACTION":
        if values:
          _reference(record, "command", values[0], "soc_action", line)
        position, argument, ok = _read_tilde(source.lines, position, end)
        record.directives[-1]["argument"] = argument
        if not ok:
          record.complete = False
          _diagnostic(corpus, "ROLSOC002", "error", "SOC action message is unterminated", line, "soc", vnum)
    records.append(record)
  return records


_PARSERS = {
    "wld": _parse_wld,
    "mob": _parse_mob,
    "obj": _parse_obj,
    "zon": _parse_zon,
    "qst": _parse_qst,
    "shp": _parse_shp,
    "soc": _parse_soc,
}


def parse_rol_source_file(
    path: Path,
    display_path: str,
    kind: str,
    basename: str,
    corpus: RolSourceCorpus | None = None,
) -> tuple[list[RolRecord], RolSourceCorpus]:
  """Parse one physical or assembled source file with the selected grammar."""

  if kind not in _PARSERS:
    raise ValueError(f"unknown RoL source kind {kind!r}")
  selected_corpus = corpus if corpus is not None else RolSourceCorpus()
  source = SourceFile.from_path(path, display_path)
  return _PARSERS[kind](source, basename, selected_corpus), selected_corpus


def parse_active_rol_corpus(source_root: Path, repo_root: Path) -> RolSourceCorpus:
  """Parse every active physical source input into normalized typed records."""

  source_root = source_root.resolve()
  inventory = build_rol_inventory(source_root, repo_root)
  corpus = RolSourceCorpus()
  active_files = sorted(
      (record for record in inventory["files"] if record["included"]),
      key=lambda record: (SOURCE_KINDS.index(record["kind"]), record["path"]),
  )
  for file_record in active_files:
    kind = file_record["kind"]
    records, _ = parse_rol_source_file(
        source_root / file_record["path"],
        file_record["path"],
        kind,
        file_record["basename"],
        corpus,
    )
    corpus.records.extend(records)
  return corpus


def source_corpus_payload(corpus: RolSourceCorpus) -> dict[str, Any]:
  record_kinds = Counter(record.kind for record in corpus.records)
  directive_counts = Counter(
      (record.kind, directive["token"])
      for record in corpus.records
      for directive in record.directives
  )
  reference_counts = Counter(
      (reference.target_type, reference.role)
      for record in corpus.records
      for reference in record.references
  )
  diagnostic_counts = Counter(
      (diagnostic.severity, diagnostic.code) for diagnostic in corpus.diagnostics
  )
  return {
      "schema_version": ROL_SOURCE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "complete": corpus.complete,
      "summary": {
          "records": len(corpus.records),
          "records_by_kind": dict(sorted(record_kinds.items())),
          "references": sum(len(record.references) for record in corpus.records),
          "diagnostics": len(corpus.diagnostics),
          "incomplete_records": sum(not record.complete for record in corpus.records),
      },
      "file_versions": [
          {"kind": kind, "version": version, "files": count}
          for (kind, version), count in sorted(corpus.file_versions.items())
      ],
      "file_terminators": [
          {"kind": kind, "status": status, "files": count}
          for (kind, status), count in sorted(corpus.file_terminators.items())
      ],
      "directives": [
          {"kind": kind, "token": token, "occurrences": count}
          for (kind, token), count in sorted(directive_counts.items())
      ],
      "reference_roles": [
          {"target_type": target_type, "role": role, "occurrences": count}
          for (target_type, role), count in sorted(reference_counts.items())
      ],
      "diagnostic_counts": [
          {"severity": severity, "code": code, "occurrences": count}
          for (severity, code), count in sorted(diagnostic_counts.items())
      ],
      "diagnostics": [
          diagnostic.to_dict()
          for diagnostic in sorted(
              corpus.diagnostics,
              key=lambda item: (item.path, item.line, item.code, item.message),
          )
      ],
  }


def iter_record_dicts(records: Iterable[RolRecord]) -> Iterable[dict[str, Any]]:
  for record in sorted(records, key=lambda item: (item.kind, item.vnum, item.path, item.line)):
    yield record.to_dict()
