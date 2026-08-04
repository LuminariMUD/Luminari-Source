"""Parser for Luminari mobile prototype files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .constants import DECODER_ALPHABET
from .flags import decode_tokens
from .models import AttachmentRecord, MobileRecord, SourceSpan, VnumReference
from .parsing import ParseResult, finding, scan_integers, source_issue_finding
from .source import READ_SIZE, SourceCursor, SourceFile, parse_c_integer_prefix, parse_c_integer_token


_DICE_LINE = re.compile(
    r"^[ \t\v\f]*([+-]?\d+)[ \t\v\f]+([+-]?\d+)[ \t\v\f]+([+-]?\d+)"
    r"[ \t\v\f]+([+-]?\d+)d([+-]?\d+)\+([+-]?\d+)"
    r"[ \t\v\f]+([+-]?\d+)d([+-]?\d+)\+([+-]?\d+)"
)

_CLAMP_RANGES = {
    "Str": (3, 50),
    "StrAdd": (0, 100),
    "Int": (3, 50),
    "Wis": (3, 50),
    "Dex": (3, 50),
    "Con": (3, 50),
    "Cha": (3, 50),
    "SavingPara": (0, 100),
    "SavingFort": (0, 100),
    "SavingRod": (0, 100),
    "SavingRefl": (0, 100),
    "SavingPetri": (0, 100),
    "SavingWill": (0, 100),
    "SavingBreath": (0, 100),
    "SavingPoison": (0, 100),
    "SavingSpell": (0, 100),
    "SavingDeath": (0, 100),
    "DR_MOD": (0, 100),
    "ResFire": (-100, 100),
    "ResCold": (-100, 100),
    "ResAir": (-100, 100),
    "ResEarth": (-100, 100),
    "ResAcid": (-100, 100),
    "ResHoly": (-100, 100),
    "ResElectric": (-100, 100),
    "ResUnholy": (-100, 100),
    "ResSlice": (-100, 100),
    "ResPuncture": (-100, 100),
    "ResForce": (-100, 100),
    "ResSound": (-100, 100),
    "ResPoison": (-100, 100),
    "ResDisease": (-100, 100),
    "ResNegative": (-100, 100),
    "ResIllusion": (-100, 100),
    "ResMental": (-100, 100),
    "ResLight": (-100, 100),
    "ResEnergy": (-100, 100),
    "ResWater": (-100, 100),
    "EchoZone": (0, 1),
    "EchoFreq": (0, 100),
    "EchoCount": (0, 20),
    "EchoSequential": (0, 1),
}


def _line_limit(result: ParseResult[MobileRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length <= READ_SIZE - 2:
    return
  result.findings.append(
      finding(
          "MOB001",
          "error",
          f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
          f"{READ_SIZE - 2}",
          line.span,
          "mobile" if vnum is not None else None,
          vnum,
      )
  )
  result.complete = False


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[MobileRecord],
    vnum: int,
) -> tuple[str, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(source_issue_finding(issue, "MOB", "mobile", vnum))
    if issue.fatal:
      result.complete = False
  return value.text, value.terminated


def _legacy_affect_token(token: str) -> str:
  if re.fullmatch(r"[+-]?\d+", token):
    return token
  mask = 0
  for character in token:
    index = DECODER_ALPHABET.find(character)
    if index >= 0:
      mask |= 1 << (index + 1)
  return str(mask)


def _decode_flags(
    result: ParseResult[MobileRecord],
    record: MobileRecord,
    tokens: list[str],
    table: dict[str, Any],
    label: str,
    span: SourceSpan,
    legacy_affect: bool = False,
) -> set[int]:
  decoded_tokens = [_legacy_affect_token(token) for token in tokens] if legacy_affect else tokens
  decoded = decode_tokens(decoded_tokens, len(table["entries"]))
  for issue in decoded.issues:
    result.findings.append(
        finding(
            "MOB010",
            "error",
            f"invalid {label} flags: {issue.message}",
            span,
            "mobile",
            record.vnum,
        )
    )
  return set(decoded.bits)


def _read_integer_line(
    cursor: SourceCursor,
    result: ParseResult[MobileRecord],
    record: MobileRecord,
    count: int,
    label: str,
) -> list[int] | None:
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding("MOB012", "error", f"mobile ends before its {label}", record.span, "mobile", record.vnum)
    )
    record.complete = False
    result.complete = False
    return None
  _line_limit(result, line, record.vnum)
  values, _, error = scan_integers(line.text, count)
  if error is not None or len(values) != count:
    result.findings.append(
        finding(
            "MOB012",
            "error",
            f"{label} requires {count} integer conversions",
            line.span,
            "mobile",
            record.vnum,
        )
    )
    return []
  return values


def _parse_simple(
    cursor: SourceCursor,
    result: ParseResult[MobileRecord],
    record: MobileRecord,
    manifest: dict[str, Any],
) -> bool:
  dice_line = cursor.read_significant()
  if dice_line is None:
    result.findings.append(
        finding("MOB012", "error", "mobile ends before its dice line", record.span, "mobile", record.vnum)
    )
    record.complete = False
    result.complete = False
    return False
  _line_limit(result, dice_line, record.vnum)
  match = _DICE_LINE.match(dice_line.text)
  if match is None:
    result.findings.append(
        finding(
            "MOB013",
            "error",
            "dice line requires '# # # #d#+# #d#+#' with nine conversions",
            dice_line.span,
            "mobile",
            record.vnum,
        )
    )
  else:
    values = [int(value) for value in match.groups()]
    record.level, record.hit_roll, record.armor_class = values[:3]
    record.hit_dice = tuple(values[3:6])  # type: ignore[assignment]
    record.damage_dice = tuple(values[6:9])  # type: ignore[assignment]
    level_limit = manifest["limits"]["LVL_IMPL"]["value"]
    if not 1 <= values[0] <= level_limit:
      result.findings.append(
          finding(
              "MOB014",
              "error",
              f"mobile level {values[0]} is outside 1..{level_limit}",
              dice_line.span,
              "mobile",
              record.vnum,
          )
      )
    for label, count, size in (
        ("hit", values[3], values[4]),
        ("damage", values[6], values[7]),
    ):
      if count < 0 or size <= 0:
        result.findings.append(
            finding(
                "MOB015",
                "error",
                f"{label} dice {count}d{size} require a non-negative count and positive size",
                dice_line.span,
                "mobile",
                record.vnum,
            )
        )

  gold_exp = _read_integer_line(cursor, result, record, 2, "gold/experience line")
  if gold_exp is None:
    return False
  if gold_exp:
    record.gold, record.experience = gold_exp
  position = _read_integer_line(cursor, result, record, 3, "position/sex line")
  if position is None:
    return False
  if position:
    record.position, record.default_position, record.sex = position
    position_count = len(manifest["tables"]["positions"]["entries"])
    for label, value in (("position", position[0]), ("default position", position[1])):
      if not 0 <= value < position_count:
        result.findings.append(
            finding(
                "MOB016",
                "error",
                f"{label} {value} is outside 0..{position_count - 1}",
                dice_line.span,
                "mobile",
                record.vnum,
            )
        )
      elif manifest["tables"]["positions"]["entries"][value]["macro"] == "POS_FIGHTING":
        result.findings.append(
            finding(
                "MOB017",
                "warning",
                f"{label} POS_FIGHTING is silently stored as POS_STANDING",
                dice_line.span,
                "mobile",
                record.vnum,
            )
        )
    gender_count = manifest["limits"]["NUM_GENDERS"]["value"]
    if not 0 <= position[2] < gender_count:
      result.findings.append(
          finding(
              "MOB018",
              "error",
              f"sex {position[2]} is outside 0..{gender_count - 1}",
              dice_line.span,
              "mobile",
              record.vnum,
          )
      )
  return True


def _clamp_keyword_ranges(manifest: dict[str, Any]) -> dict[str, tuple[int, int]]:
  ranges = dict(_CLAMP_RANGES)
  ranges.update(
      {
          "BareHandAttack": (0, manifest["limits"]["NUM_ATTACK_TYPES"]["value"] - 1),
          "KnownSpell": (1, manifest["limits"]["NUM_SPELLS"]["value"]),
          "Race": (0, manifest["limits"]["NUM_RACE_TYPES"]["value"]),
          "SubRace 1": (0, manifest["limits"]["NUM_SUB_RACES"]["value"]),
          "SubRace 2": (0, manifest["limits"]["NUM_SUB_RACES"]["value"]),
          "SubRace 3": (0, manifest["limits"]["NUM_SUB_RACES"]["value"]),
          "Class": (0, manifest["limits"]["NUM_CLASSES"]["value"]),
          "Size": (0, manifest["limits"]["NUM_SIZES"]["value"] - 1),
      }
  )
  return ranges


def _parse_path(
    value: str,
    line: Any,
    result: ParseResult[MobileRecord],
    record: MobileRecord,
    maximum: int,
) -> None:
  match = re.match(r"^[ \t]*([+-]?\d+)[ \t]*:(.*)$", value)
  if match is None:
    result.findings.append(
        finding(
            "MOB024",
            "error",
            "Path requires a numeric delay, ':', and a whitespace-separated room list",
            line.span,
            "mobile",
            record.vnum,
        )
    )
    return
  room_text = match.group(2).strip()
  rooms: list[int] = []
  if room_text:
    for token in room_text.split():
      parsed = parse_c_integer_token(token)
      if parsed.error is not None or parsed.value is None or parsed.value <= 0:
        result.findings.append(
            finding(
                "MOB024",
                "error",
                f"invalid Path room {token!r}",
                line.span,
                "mobile",
                record.vnum,
            )
        )
        continue
      rooms.append(parsed.value)
  if len(rooms) > maximum:
    result.findings.append(
        finding(
            "MOB025",
            "error",
            f"Path has {len(rooms)} rooms; the fixed source array holds {maximum}",
            line.span,
            "mobile",
            record.vnum,
        )
    )
  record.path_rooms = rooms
  record.references.extend(VnumReference("room", room, "mobile path", line.span) for room in rooms)


def _parse_enhanced_line(
    line: Any,
    result: ParseResult[MobileRecord],
    record: MobileRecord,
    manifest: dict[str, Any],
    spec_names: set[str],
) -> None:
  keyword, separator, value = line.text.partition(":")
  value = value.lstrip() if separator else ""
  record.enhanced.setdefault(keyword, []).append(value)
  if keyword in {"MFeat", "Feat"}:
    values, _, error = scan_integers(value, 2)
    if error is not None or len(values) != 2:
      result.findings.append(
          finding("MOB021", "error", f"{keyword} requires two integers", line.span, "mobile", record.vnum)
      )
    elif not 0 <= values[0] < manifest["limits"]["NUM_FEATS"]["value"]:
      result.findings.append(
          finding(
              "MOB021",
              "error",
              f"{keyword} feat {values[0]} is outside the feat table",
              line.span,
              "mobile",
              record.vnum,
          )
      )
    return
  if keyword == "Aff2":
    values, _, error = scan_integers(value, 4)
    if error is not None or len(values) != 4:
      result.findings.append(
          finding("MOB022", "error", "Aff2 requires four integers", line.span, "mobile", record.vnum)
      )
      return
    record.affect2_flags = [str(item) for item in values]
    _decode_flags(
        result,
        record,
        record.affect2_flags,
        manifest["tables"]["affect2"],
        "secondary affect",
        line.span,
    )
    return
  if keyword == "Path":
    _parse_path(value, line, result, record, manifest["limits"]["MAX_PATH"]["value"])
    return
  if keyword == "SpecProc":
    record.spec_proc = value
    if not value or value.casefold() not in spec_names:
      result.findings.append(
          finding(
              "MOB023",
              "error",
              f"unknown persisted spec-proc name {value!r}",
              line.span,
              "mobile",
              record.vnum,
          )
      )
    return
  if keyword in {"Walkin", "Walkout", "Echo"}:
    if not separator:
      result.findings.append(
          finding("MOB020", "error", f"{keyword} requires a value", line.span, "mobile", record.vnum)
      )
    return

  clamp_ranges = _clamp_keyword_ranges(manifest)
  limits = clamp_ranges.get(keyword)
  if limits is None:
    result.findings.append(
        finding(
            "MOB019",
            "warning",
            f"unrecognized enhanced mobile keyword {keyword!r}",
            line.span,
            "mobile",
            record.vnum,
        )
    )
    return
  parsed = parse_c_integer_prefix(value)
  if not separator or parsed.error is not None or parsed.value is None:
    result.findings.append(
        finding(
            "MOB020",
            "error",
            f"{keyword} requires an integer value",
            line.span,
            "mobile",
            record.vnum,
        )
    )
    return
  low, high = limits
  effective = min(high, max(low, parsed.value))
  if effective != parsed.value:
    result.findings.append(
        finding(
            "MOB026",
            "warning",
            f"{keyword} value {parsed.value} is silently clamped to {effective}",
            line.span,
            "mobile",
            record.vnum,
        )
    )


def _read_attachments(
    cursor: SourceCursor,
    result: ParseResult[MobileRecord],
    record: MobileRecord,
) -> None:
  while True:
    line = cursor.peek_raw()
    if line is None:
      return
    stripped = line.text.lstrip()
    if not stripped:
      cursor.read_raw()
      continue
    if not stripped.startswith("T"):
      return
    cursor.read_raw()
    _line_limit(result, line, record.vnum)
    tokens = stripped.split()
    parsed = parse_c_integer_token(tokens[1]) if len(tokens) >= 2 else None
    if parsed is None or parsed.error is not None or parsed.value is None:
      detail = "missing value" if parsed is None else parsed.error
      result.findings.append(
          finding(
              "MOB027",
              "error",
              f"inline trigger attachment has an invalid vnum: {detail}",
              line.span,
              "mobile",
              record.vnum,
          )
      )
      continue
    record.attachments.append(AttachmentRecord(parsed.value, "mobile", record.vnum, line.span))


def _recover_record(cursor: SourceCursor) -> None:
  while not cursor.eof:
    line = cursor.peek_raw()
    if line is not None and (line.text.startswith("#") or line.text.startswith("$")):
      return
    cursor.read_raw()


def parse_mobile_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
    spec_names: set[str],
) -> ParseResult[MobileRecord]:
  result: ParseResult[MobileRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding("MOB002", "error", f"cannot read mobile file: {error}", SourceSpan(display_path, 1))
    )
    result.complete = False
    return result
  cursor = SourceCursor(source)
  found_end = False
  while True:
    header = cursor.read_significant()
    if header is None:
      break
    _line_limit(result, header)
    if header.text.startswith("$"):
      found_end = True
      break
    match = re.match(r"^#([+-]?\d+)", header.text)
    if match is None:
      result.findings.append(finding("MOB003", "error", "expected a #vnum mobile header", header.span))
      _recover_record(cursor)
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(finding("MOB003", "error", f"invalid mobile vnum: {parsed_vnum.error}", header.span))
      _recover_record(cursor)
      continue
    record = MobileRecord(parsed_vnum.value, header.span, path.stem)
    result.records.append(record)
    if record.vnum < 0:
      result.findings.append(
          finding("MOB004", "error", "mobile vnum must be non-negative", header.span, "mobile", record.vnum)
      )

    strings: list[str] = []
    for _ in range(4):
      value, complete = _read_string(cursor, result, record.vnum)
      strings.append(value)
      if not complete:
        record.complete = False
        return result
    record.aliases, record.short_description, record.long_description, record.description = strings

    flags_line = cursor.read_significant()
    if flags_line is None:
      result.findings.append(
          finding("MOB005", "error", "mobile ends before its flag header", header.span, "mobile", record.vnum)
      )
      record.complete = False
      result.complete = False
      return result
    _line_limit(result, flags_line, record.vnum)
    tokens = flags_line.text.split()
    if len(tokens) == 4:
      record.action_flags = [tokens[0]]
      record.affect_flags = [tokens[1]]
      alignment = parse_c_integer_token(tokens[2])
      if alignment.error is not None or alignment.value is None:
        result.findings.append(
            finding("MOB006", "error", "legacy alignment is not an integer", flags_line.span, "mobile", record.vnum)
        )
      else:
        record.alignment = alignment.value
      record.record_kind = tokens[3][0] if tokens[3] else None
      result.findings.append(
          finding(
              "MOB007",
              "warning",
              "legacy four-field mobile flag header is accepted but should be converted to ten fields",
              flags_line.span,
              "mobile",
              record.vnum,
          )
      )
      action_bits = _decode_flags(
          result,
          record,
          record.action_flags,
          manifest["tables"]["mob"],
          "action",
          flags_line.span,
      )
      _decode_flags(
          result,
          record,
          record.affect_flags,
          manifest["tables"]["affect"],
          "affect",
          flags_line.span,
          legacy_affect=True,
      )
    elif len(tokens) >= 10:
      record.action_flags = tokens[:4]
      record.affect_flags = tokens[4:8]
      alignment = parse_c_integer_token(tokens[8])
      if alignment.error is not None or alignment.value is None:
        result.findings.append(
            finding("MOB006", "error", "alignment is not an integer", flags_line.span, "mobile", record.vnum)
        )
      else:
        record.alignment = alignment.value
      record.record_kind = tokens[9][0] if tokens[9] else None
      action_bits = _decode_flags(
          result,
          record,
          record.action_flags,
          manifest["tables"]["mob"],
          "action",
          flags_line.span,
      )
      _decode_flags(
          result,
          record,
          record.affect_flags,
          manifest["tables"]["affect"],
          "affect",
          flags_line.span,
      )
    else:
      result.findings.append(
          finding(
              "MOB006",
              "error",
              f"mobile flag header has {len(tokens)} conversions; expected 4 or at least 10",
              flags_line.span,
              "mobile",
              record.vnum,
          )
      )
      _recover_record(cursor)
      continue

    not_dead = next(
        entry["index"]
        for entry in manifest["tables"]["mob"]["entries"]
        if entry["macro"] == "MOB_NOTDEADYET"
    )
    if not_dead in action_bits:
      result.findings.append(
          finding(
              "MOB011",
              "error",
              "reserved MOB_NOTDEADYET is silently removed while loading",
              flags_line.span,
              "mobile",
              record.vnum,
          )
      )
    if record.record_kind is None or record.record_kind.upper() not in {"S", "E"}:
      result.findings.append(
          finding(
              "MOB008",
              "error",
              f"unsupported mobile record kind {record.record_kind!r}; expected S or E",
              flags_line.span,
              "mobile",
              record.vnum,
          )
      )
      _recover_record(cursor)
      continue
    if not _parse_simple(cursor, result, record, manifest):
      return result
    if record.record_kind.upper() == "E":
      enhanced_end = False
      while True:
        line = cursor.read_significant()
        if line is None:
          break
        _line_limit(result, line, record.vnum)
        if line.text == "E":
          enhanced_end = True
          break
        if line.text.startswith("#") or line.text.startswith("$"):
          cursor.unread_raw()
          break
        _parse_enhanced_line(line, result, record, manifest, spec_names)
      if not enhanced_end:
        result.findings.append(
            finding(
                "MOB009",
                "error",
                "enhanced mobile section is missing its exact 'E' terminator",
                flags_line.span,
                "mobile",
                record.vnum,
            )
        )
        record.complete = False
        result.complete = False
    _read_attachments(cursor, result, record)

  if not found_end:
    result.findings.append(
        finding("MOB028", "error", "mobile file is missing its '$' terminator", SourceSpan(display_path, 1))
    )
    result.complete = False
  return result
