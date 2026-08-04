"""Parser for Luminari object prototype files and extension records."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .constants import DECODER_ALPHABET
from .flags import decode_tokens
from .models import (
    AttachmentRecord,
    ExtraDescriptionRecord,
    ObjectAffectRecord,
    ObjectRecord,
    ObjectSpecialAbilityRecord,
    SourceSpan,
    VnumReference,
)
from .parsing import ParseResult, finding, scan_integers, source_issue_finding
from .source import READ_SIZE, SourceCursor, SourceFile, parse_c_integer_token


def _line_limit(result: ParseResult[ObjectRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length <= READ_SIZE - 2:
    return
  result.findings.append(
      finding(
          "OBJ001",
          "error",
          f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
          f"{READ_SIZE - 2}",
          line.span,
          "object" if vnum is not None else None,
          vnum,
      )
  )
  result.complete = False


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[ObjectRecord],
    vnum: int,
) -> tuple[str, SourceSpan, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(source_issue_finding(issue, "OBJ", "object", vnum))
    if issue.fatal:
      result.complete = False
  return value.text, value.span, value.terminated


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
    result: ParseResult[ObjectRecord],
    record: ObjectRecord,
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
            "OBJ010",
            "error",
            f"invalid {label} flags: {issue.message}",
            span,
            "object",
            record.vnum,
        )
    )
  return set(decoded.bits)


def _item_type_ids(manifest: dict[str, Any]) -> dict[str, int]:
  return {
      entry["macro"]: entry["index"]
      for entry in manifest["tables"]["item-types"]["entries"]
      if entry["macro"] is not None
  }


def _read_payload(
    cursor: SourceCursor,
    result: ParseResult[ObjectRecord],
    record: ObjectRecord,
    kind: str,
    count: int,
) -> tuple[list[int], SourceSpan] | None:
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding(
            "OBJ019",
            "error",
            f"{kind} extension is missing its payload",
            record.span,
            "object",
            record.vnum,
        )
    )
    record.complete = False
    result.complete = False
    return None
  _line_limit(result, line, record.vnum)
  values, _, error = scan_integers(line.text, count)
  if error is not None or len(values) != count:
    result.findings.append(
        finding(
            "OBJ019",
            "error",
            f"{kind} extension requires {count} integer conversions",
            line.span,
            "object",
            record.vnum,
        )
    )
    return ([], line.span)
  return values, line.span


def _parse_affect(
    cursor: SourceCursor,
    result: ParseResult[ObjectRecord],
    record: ObjectRecord,
    slot: int,
    maximum: int,
) -> None:
  if slot >= maximum:
    result.findings.append(
        finding(
            "OBJ020",
            "error",
            f"A extension uses shared slot {slot}; only {maximum} object affects are safe",
            record.span,
            "object",
            record.vnum,
        )
    )
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding("OBJ019", "error", "A extension is missing its payload", record.span, "object", record.vnum)
    )
    result.complete = False
    record.complete = False
    return
  _line_limit(result, line, record.vnum)
  values, _, error = scan_integers(line.text, 4)
  if error is not None or len(values) not in {2, 3, 4}:
    result.findings.append(
        finding(
            "OBJ019",
            "error",
            "A extension requires two, three, or four integers",
            line.span,
            "object",
            record.vnum,
        )
    )
    return
  if len(values) < 4:
    result.findings.append(
        finding(
            "OBJ021",
            "error",
            f"{len(values)}-integer A payload leaves the stored specific field dependent on stale parser data",
            line.span,
            "object",
            record.vnum,
        )
    )
  padded: list[int | None] = values + [None] * (4 - len(values))
  record.affects.append(
      ObjectAffectRecord(
          int(padded[0]),
          int(padded[1]),
          padded[2],
          padded[3],
          line.span,
      )
  )


def _parse_special_ability(
    cursor: SourceCursor,
    result: ParseResult[ObjectRecord],
    record: ObjectRecord,
    manifest: dict[str, Any],
) -> None:
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding("OBJ019", "error", "C extension is missing its payload", record.span, "object", record.vnum)
    )
    result.complete = False
    record.complete = False
    return
  _line_limit(result, line, record.vnum)
  values, offset, error = scan_integers(line.text, 7)
  if error is not None or len(values) != 7:
    result.findings.append(
        finding(
            "OBJ019",
            "error",
            "C extension requires seven integers and an optional command word",
            line.span,
            "object",
            record.vnum,
        )
    )
    return
  command_parts = line.text[offset:].split()
  command_word = command_parts[0] if command_parts else None
  maximum = manifest["limits"]["NUM_SPECABS"]["value"]
  if not 0 <= values[0] < maximum:
    result.findings.append(
        finding(
            "OBJ022",
            "error",
            f"special ability {values[0]} is outside 0..{maximum - 1}",
            line.span,
            "object",
            record.vnum,
        )
    )
  ability = ObjectSpecialAbilityRecord(
      values[0], values[1], values[2], values[3:7], command_word, line.span
  )
  record.special_abilities.append(ability)
  mob_abilities = {
      manifest["limits"]["ITEM_SPECAB_HORN_OF_SUMMONING"]["value"],
      manifest["limits"]["ITEM_SPECAB_ITEM_SUMMON"]["value"],
  }
  if ability.ability in mob_abilities and ability.values[0] > 0:
    record.references.append(
        VnumReference("mobile", ability.values[0], "object special ability summon", line.span)
    )


def _add_typed_references(record: ObjectRecord, manifest: dict[str, Any]) -> None:
  if record.item_type is None or len(record.values) < 4:
    return
  item_types = _item_type_ids(manifest)
  values = record.values

  if record.item_type in {item_types["ITEM_CONTAINER"], item_types["ITEM_AMMO_POUCH"]}:
    if values[2] > 0:
      record.references.append(VnumReference("object", values[2], "container key", record.span))
  elif record.item_type == item_types["ITEM_PORTAL"]:
    portal_type = values[0]
    exact_types = {
        manifest["limits"]["PORTAL_NORMAL"]["value"],
        manifest["limits"]["PORTAL_CHECKFLAGS"]["value"],
    }
    if portal_type in exact_types and values[1] > 0:
      record.references.append(VnumReference("room", values[1], "portal destination", record.span))
    elif portal_type == manifest["limits"]["PORTAL_RANDOM"]["value"]:
      if values[1] > 0:
        record.references.append(VnumReference("room", values[1], "portal range start", record.span))
      if values[2] > 0:
        record.references.append(VnumReference("room", values[2], "portal range end", record.span))
  elif record.item_type == item_types["ITEM_TRAP"] and values[0] in {3, 4, 5}:
    if values[1] > 0:
      record.references.append(VnumReference("object", values[1], "trap target", record.span))
  elif record.item_type == item_types["ITEM_SWITCH"]:
    if values[1] > 0:
      record.references.append(VnumReference("room", values[1], "switch room", record.span))
  elif record.item_type == item_types["ITEM_GREYHAWK_SHIP"]:
    if values[0] > 0:
      record.references.append(VnumReference("room", values[0], "ship interior", record.span))
  elif record.item_type == item_types["ITEM_PET"]:
    record.references.append(VnumReference("mobile", record.vnum, "pet prototype", record.span))


def _parse_extensions(
    cursor: SourceCursor,
    result: ParseResult[ObjectRecord],
    record: ObjectRecord,
    manifest: dict[str, Any],
    spec_names: set[str],
) -> None:
  shared_slot = 0
  weapon_spell_count = 0
  max_affects = manifest["limits"]["MAX_OBJ_AFFECT"]["value"]
  spellbook_size = manifest["limits"]["SPELLBOOK_SIZE"]["value"]
  while True:
    line = cursor.read_significant()
    if line is None:
      record.complete = False
      result.complete = False
      result.findings.append(
          finding(
              "OBJ018",
              "error",
              "object file ended before the next # record or $ terminator",
              record.span,
              "object",
              record.vnum,
          )
      )
      return
    _line_limit(result, line, record.vnum)
    if line.text.startswith("#") or line.text.startswith("$"):
      cursor.unread_raw()
      return
    kind = line.text[0] if line.text else ""
    if kind == "A":
      _parse_affect(cursor, result, record, shared_slot, max_affects)
      shared_slot += 1
    elif kind == "B":
      if shared_slot >= spellbook_size:
        result.findings.append(
            finding(
                "OBJ023",
                "error",
                f"B extension uses shared slot {shared_slot}; spellbooks hold {spellbook_size}",
                line.span,
                "object",
                record.vnum,
            )
        )
      payload = _read_payload(cursor, result, record, "B", 2)
      if payload is not None and payload[0]:
        values, span = payload
        record.spellbook.append((values[0], values[1], span))
        if not 1 <= values[0] <= manifest["limits"]["NUM_SPELLS"]["value"]:
          result.findings.append(
              finding("OBJ024", "error", f"invalid spellbook spell {values[0]}", span, "object", record.vnum)
          )
      shared_slot += 1
    elif kind == "C":
      _parse_special_ability(cursor, result, record, manifest)
    elif kind == "E":
      keywords, keyword_span, keyword_complete = _read_string(cursor, result, record.vnum)
      if not keyword_complete:
        record.complete = False
        return
      description, _, description_complete = _read_string(cursor, result, record.vnum)
      if not description_complete:
        record.complete = False
        return
      record.extra_descriptions.append(
          ExtraDescriptionRecord(keywords or None, description or None, keyword_span)
      )
    elif kind in {"G", "H", "I", "J"}:
      payload = _read_payload(cursor, result, record, kind, 1)
      if payload is None or not payload[0]:
        continue
      value, span = payload[0][0], payload[1]
      if kind == "I":
        size_count = manifest["limits"]["NUM_SIZES"]["value"]
        if not 0 <= value < size_count:
          result.findings.append(
              finding(
                  "OBJ025",
                  "error",
                  f"object size {value} is outside 0..{size_count - 1}",
                  span,
                  "object",
                  record.vnum,
              )
          )
        elif value == 0:
          result.findings.append(
              finding(
                  "OBJ026",
                  "warning",
                  "object size 0 is silently converted to SIZE_MEDIUM",
                  span,
                  "object",
                  record.vnum,
              )
          )
      elif kind == "J":
        record.recipient_vnum = value
        if value > 0:
          record.references.append(VnumReference("mobile", value, "object recipient", span))
    elif kind == "R":
      payload_line = cursor.read_significant()
      if payload_line is None:
        result.findings.append(
            finding("OBJ019", "error", "R extension is missing its identifier", line.span, "object", record.vnum)
        )
        record.complete = False
        result.complete = False
      else:
        _line_limit(result, payload_line, record.vnum)
        record.restring_identifier = payload_line.text
    elif kind == "K":
      payload = _read_payload(cursor, result, record, "K", 5)
      if payload is not None and payload[0]:
        record.activated_spells.append(payload)
        spell = payload[0][1]
        if not 1 <= spell <= manifest["limits"]["NUM_SPELLS"]["value"]:
          result.findings.append(
              finding("OBJ024", "error", f"invalid activated spell {spell}", payload[1], "object", record.vnum)
          )
    elif kind == "P":
      result.findings.append(
          finding(
              "OBJ027",
              "warning",
              "P extension is a loader no-op and consumes no payload",
              line.span,
              "object",
              record.vnum,
          )
      )
    elif kind == "S":
      payload = _read_payload(cursor, result, record, "S", 4)
      weapon_spell_count += 1
      if weapon_spell_count > manifest["limits"]["MAX_WEAPON_SPELLS"]["value"]:
        result.findings.append(
            finding(
                "OBJ028",
                "error",
                f"weapon spell {weapon_spell_count} writes beyond MAX_WEAPON_SPELLS",
                line.span,
                "object",
                record.vnum,
            )
        )
      if payload is not None and payload[0]:
        record.weapon_spells.append(payload)
        spell = payload[0][0]
        if not 1 <= spell <= manifest["limits"]["NUM_SPELLS"]["value"]:
          result.findings.append(
              finding("OBJ024", "error", f"invalid weapon spell {spell}", payload[1], "object", record.vnum)
          )
    elif kind == "T":
      tokens = line.text.split()
      parsed = parse_c_integer_token(tokens[1]) if len(tokens) >= 2 else None
      if parsed is None or parsed.error is not None or parsed.value is None:
        detail = "missing value" if parsed is None else parsed.error
        result.findings.append(
            finding(
                "OBJ029",
                "error",
                f"inline trigger attachment has an invalid vnum: {detail}",
                line.span,
                "object",
                record.vnum,
            )
        )
      else:
        record.attachments.append(AttachmentRecord(parsed.value, "object", record.vnum, line.span))
    elif kind == "Z":
      spec_line = cursor.read_significant()
      if spec_line is None:
        result.findings.append(
            finding("OBJ030", "error", "Z extension is missing its spec-proc name", line.span, "object", record.vnum)
        )
        record.complete = False
        result.complete = False
      else:
        _line_limit(result, spec_line, record.vnum)
        record.spec_proc = spec_line.text
        if spec_line.text.casefold() not in spec_names:
          result.findings.append(
              finding(
                  "OBJ030",
                  "error",
                  f"unknown persisted spec-proc name {spec_line.text!r}",
                  spec_line.span,
                  "object",
                  record.vnum,
              )
          )
    else:
      result.findings.append(
          finding(
              "OBJ031",
              "error",
              f"unknown object extension record {kind!r}",
              line.span,
              "object",
              record.vnum,
          )
      )


def _recover_record(cursor: SourceCursor) -> None:
  while not cursor.eof:
    line = cursor.peek_raw()
    if line is not None and (line.text.startswith("#") or line.text.startswith("$")):
      return
    cursor.read_raw()


def parse_object_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
    spec_names: set[str],
) -> ParseResult[ObjectRecord]:
  result: ParseResult[ObjectRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding("OBJ002", "error", f"cannot read object file: {error}", SourceSpan(display_path, 1))
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
      result.findings.append(finding("OBJ003", "error", "expected a #vnum object header", header.span))
      _recover_record(cursor)
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(finding("OBJ003", "error", f"invalid object vnum: {parsed_vnum.error}", header.span))
      _recover_record(cursor)
      continue
    record = ObjectRecord(parsed_vnum.value, header.span, path.stem)
    result.records.append(record)
    if record.vnum < 0:
      result.findings.append(
          finding("OBJ004", "error", "object vnum must be non-negative", header.span, "object", record.vnum)
      )

    strings: list[str] = []
    for _ in range(4):
      value, _, complete = _read_string(cursor, result, record.vnum)
      strings.append(value)
      if not complete:
        record.complete = False
        return result
    record.aliases, record.short_description, record.description, record.action_description = strings

    first_line = cursor.read_significant()
    if first_line is None:
      result.findings.append(
          finding("OBJ005", "error", "object ends before its first numeric line", header.span, "object", record.vnum)
      )
      result.complete = False
      record.complete = False
      return result
    _line_limit(result, first_line, record.vnum)
    tokens = first_line.text.split()
    parsed_type = parse_c_integer_token(tokens[0]) if tokens else None
    if parsed_type is None or parsed_type.error is not None or parsed_type.value is None:
      result.findings.append(
          finding("OBJ006", "error", "object type is not an integer", first_line.span, "object", record.vnum)
      )
      _recover_record(cursor)
      continue
    record.item_type = parsed_type.value
    item_type_count = len(manifest["tables"]["item-types"]["entries"])
    if not 0 <= record.item_type < item_type_count:
      result.findings.append(
          finding(
              "OBJ007",
              "error",
              f"item type {record.item_type} is outside 0..{item_type_count - 1}",
              first_line.span,
              "object",
              record.vnum,
          )
      )
    if len(tokens) == 3:
      record.extra_flags = [tokens[1]]
      record.wear_flags = [tokens[2]]
      record.affect_flags = ["0"]
      result.findings.append(
          finding(
              "OBJ008",
              "error",
              "three-field legacy object header enters an undefined uninitialized-affect path",
              first_line.span,
              "object",
              record.vnum,
          )
      )
    elif len(tokens) == 4:
      record.extra_flags = [tokens[1]]
      record.wear_flags = [tokens[2]]
      record.affect_flags = [tokens[3]]
      result.findings.append(
          finding(
              "OBJ009",
              "warning",
              "legacy four-field object flag header is accepted but should be converted",
              first_line.span,
              "object",
              record.vnum,
          )
      )
    elif len(tokens) == 13:
      record.extra_flags = tokens[1:5]
      record.wear_flags = tokens[5:9]
      record.affect_flags = tokens[9:13]
    elif len(tokens) >= 17:
      record.extra_flags = tokens[1:5]
      record.wear_flags = tokens[5:9]
      record.affect_flags = tokens[9:13]
      record.affect2_flags = tokens[13:17]
    else:
      result.findings.append(
          finding(
              "OBJ006",
              "error",
              f"first numeric line has {len(tokens)} conversions; expected 3, 4, 13, or at least 17",
              first_line.span,
              "object",
              record.vnum,
          )
      )
      _recover_record(cursor)
      continue
    _decode_flags(
        result, record, record.extra_flags, manifest["tables"]["obj-extra"], "extra", first_line.span
    )
    wear_bits = _decode_flags(
        result, record, record.wear_flags, manifest["tables"]["obj-wear"], "wear", first_line.span
    )
    _decode_flags(
        result,
        record,
        record.affect_flags,
        manifest["tables"]["affect"],
        "affect",
        first_line.span,
        legacy_affect=len(tokens) in {3, 4},
    )
    if record.affect2_flags:
      _decode_flags(
          result,
          record,
          record.affect2_flags,
          manifest["tables"]["affect2"],
          "secondary affect",
          first_line.span,
      )

    value_line = cursor.read_significant()
    if value_line is None:
      result.findings.append(
          finding("OBJ011", "error", "object ends before its value vector", header.span, "object", record.vnum)
      )
      result.complete = False
      record.complete = False
      return result
    _line_limit(result, value_line, record.vnum)
    values, _, value_error = scan_integers(value_line.text, 16)
    if value_error is not None or len(values) not in {4, 16}:
      result.findings.append(
          finding(
              "OBJ011",
              "error",
              f"object value vector has {len(values)} conversions; expected 4 or 16",
              value_line.span,
              "object",
              record.vnum,
          )
      )
      values = (values + [0] * 16)[:16]
    elif len(values) == 4:
      values.extend([0] * 12)
    record.values = values

    cost_line = cursor.read_significant()
    if cost_line is None:
      result.findings.append(
          finding("OBJ012", "error", "object ends before weight/cost fields", header.span, "object", record.vnum)
      )
      result.complete = False
      record.complete = False
      return result
    _line_limit(result, cost_line, record.vnum)
    costs, _, cost_error = scan_integers(cost_line.text, 5)
    if cost_error is not None or len(costs) not in {3, 4, 5}:
      result.findings.append(
          finding(
              "OBJ012",
              "error",
              f"weight/cost line has {len(costs)} conversions; expected 3, 4, or 5",
              cost_line.span,
              "object",
              record.vnum,
          )
      )
      costs = (costs + [0] * 5)[:5]
    elif len(costs) < 5:
      costs.extend([0] * (5 - len(costs)))
    record.weight, record.cost, record.rent, record.level, record.timer = costs
    if record.weight < 0:
      result.findings.append(
          finding("OBJ013", "error", "object weight must not be negative", cost_line.span, "object", record.vnum)
      )
    if record.rent < 0:
      result.findings.append(
          finding("OBJ014", "error", "object rent must not be negative", cost_line.span, "object", record.vnum)
      )
    effective_level = min(30, max(1, record.level))
    if effective_level != record.level:
      result.findings.append(
          finding(
              "OBJ015",
              "warning",
              f"object level {record.level} is silently clamped to {effective_level}",
              cost_line.span,
              "object",
              record.vnum,
          )
      )
      record.level = effective_level
    item_types = _item_type_ids(manifest)
    if (
        record.item_type in {item_types["ITEM_DRINKCON"], item_types["ITEM_FOUNTAIN"]}
        and record.weight < record.values[1]
        and 0 in wear_bits
    ):
      result.findings.append(
          finding(
              "OBJ016",
              "warning",
              f"takeable liquid container weight {record.weight} is silently changed to "
              f"{record.values[1] + 5}",
              cost_line.span,
              "object",
              record.vnum,
          )
      )
      record.weight = record.values[1] + 5

    _parse_extensions(cursor, result, record, manifest, spec_names)
    _add_typed_references(record, manifest)

  if not found_end:
    result.findings.append(
        finding("OBJ032", "error", "object file is missing its '$' terminator", SourceSpan(display_path, 1))
    )
    result.complete = False
  return result
