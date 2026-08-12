"""Version-aware parser for Luminari shop files."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any

from .models import ShopBuyTypeRecord, ShopRecord, SourceSpan, VnumReference
from .parsing import ParseResult, finding, source_issue_finding
from .source import READ_SIZE, SourceCursor, SourceFile, parse_c_integer_prefix, parse_c_integer_token


_FLOAT_PREFIX = re.compile(
    r"^[ \t\v\f]*([+-]?(?:(?:\d+(?:\.\d*)?)|(?:\.\d+))(?:[eE][+-]?\d+)?)"
)


def _line_limit(result: ParseResult[ShopRecord], line: Any, vnum: int | None = None) -> None:
  if line.byte_length <= READ_SIZE - 2:
    return
  result.findings.append(
      finding(
          "SHP001",
          "error",
          f"physical line is {line.byte_length} bytes; get_line() safely accepts at most "
          f"{READ_SIZE - 2}",
          line.span,
          "shop" if vnum is not None else None,
          vnum,
      )
  )
  result.complete = False


def _read_string(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    vnum: int | None = None,
) -> tuple[str, SourceSpan, bool]:
  value = cursor.read_tilde_string()
  for issue in value.issues:
    result.findings.append(
        source_issue_finding(issue, "SHP", "shop" if vnum is not None else None, vnum)
    )
    if issue.fatal:
      result.complete = False
  return value.text, value.span, value.terminated


def _read_integer(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
    label: str,
    bits: int = 32,
) -> tuple[int, SourceSpan] | None:
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding("SHP010", "error", f"shop ends before {label}", record.span, "shop", record.vnum)
    )
    record.complete = False
    result.complete = False
    return None
  _line_limit(result, line, record.vnum)
  parsed = parse_c_integer_prefix(line.text, bits=bits)
  if parsed.error is not None or parsed.value is None:
    result.findings.append(
        finding("SHP010", "error", f"{label} is not an integer", line.span, "shop", record.vnum)
    )
    return None
  return parsed.value, line.span


def _read_float(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
    label: str,
) -> float | None:
  line = cursor.read_significant()
  if line is None:
    result.findings.append(
        finding("SHP011", "error", f"shop ends before {label}", record.span, "shop", record.vnum)
    )
    record.complete = False
    result.complete = False
    return None
  _line_limit(result, line, record.vnum)
  match = _FLOAT_PREFIX.match(line.text)
  if match is None:
    result.findings.append(
        finding("SHP011", "error", f"{label} is not a floating-point number", line.span, "shop", record.vnum)
    )
    return None
  return float(match.group(1))


def _read_numeric_list(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
    modern: bool,
    legacy_count: int,
    maximum: int,
    label: str,
    target_type: str,
) -> list[int]:
  values: list[int] = []
  iterations = 0
  while modern or iterations < legacy_count:
    line = cursor.read_significant()
    if line is None:
      result.findings.append(
          finding(
              "SHP012",
              "error",
              f"{label} ends before its {'negative sentinel' if modern else 'fixed entries'}",
              record.span,
              "shop",
              record.vnum,
          )
      )
      record.complete = False
      result.complete = False
      return values
    _line_limit(result, line, record.vnum)
    parsed = parse_c_integer_token(line.text.strip())
    if parsed.error is not None or parsed.value is None:
      result.findings.append(
          finding(
              "SHP012",
              "error",
              f"{label} entry is not a complete integer",
              line.span,
              "shop",
              record.vnum,
          )
      )
      if modern:
        cursor.unread_raw()
      return values
    value = parsed.value
    iterations += 1
    if modern and value < 0:
      return values
    if value >= 0:
      if len(values) >= maximum:
        result.findings.append(
            finding(
                "SHP013",
                "error",
                f"{label} exceeds MAX_SHOP_OBJ ({maximum}); later entries are dropped",
                line.span,
                "shop",
                record.vnum,
            )
        )
      else:
        values.append(value)
        record.references.append(VnumReference(target_type, value, label, line.span))
  return values


def _resolve_item_type(text: str, manifest: dict[str, Any]) -> tuple[int | None, str | None]:
  stripped = text.strip()
  entries = manifest["tables"]["item-types"]["entries"]
  for entry in entries:
    name = entry["name"]
    if stripped.casefold().startswith(name.casefold()):
      remainder = stripped[len(name) :].strip()
      return entry["index"], remainder or None
  parsed = parse_c_integer_prefix(stripped)
  if parsed.error is not None or parsed.value is None:
    return None, None
  remainder = stripped[parsed.consumed :].strip()
  return parsed.value, remainder or None


def _read_buy_types(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
    manifest: dict[str, Any],
    modern: bool,
) -> None:
  maximum = manifest["limits"]["MAX_SHOP_OBJ"]["value"]
  legacy_count = manifest["limits"]["MAX_TRADE"]["value"]
  iterations = 0
  while modern or iterations < legacy_count:
    line = cursor.read_raw() if modern else cursor.read_significant()
    if line is None:
      result.findings.append(
          finding("SHP014", "error", "shop buy-type list ends unexpectedly", record.span, "shop", record.vnum)
      )
      result.complete = False
      record.complete = False
      return
    if not modern:
      _line_limit(result, line, record.vnum)
    text = line.text.split(";", 1)[0] if modern else line.text
    item_type, keywords = _resolve_item_type(text, manifest)
    iterations += 1
    if item_type is None:
      result.findings.append(
          finding("SHP015", "error", f"invalid shop buy type {text!r}", line.span, "shop", record.vnum)
      )
      if modern:
        return
      continue
    if item_type < 0:
      if modern:
        return
      continue
    item_count = len(manifest["tables"]["item-types"]["entries"])
    if not 0 <= item_type < item_count:
      result.findings.append(
          finding(
              "SHP016",
              "error",
              f"shop buy type {item_type} is outside 0..{item_count - 1}",
              line.span,
              "shop",
              record.vnum,
          )
      )
      continue
    if len(record.buy_types) >= maximum:
      result.findings.append(
          finding(
              "SHP013",
              "error",
              f"buy-type list exceeds MAX_SHOP_OBJ ({maximum})",
              line.span,
              "shop",
              record.vnum,
          )
      )
      continue
    record.buy_types.append(ShopBuyTypeRecord(item_type, keywords, line.span))


def _validate_message(
    message: str,
    index: int,
    span: SourceSpan,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
) -> None:
  strings = 0
  decimals = 0
  offset = 0
  while offset < len(message):
    if message[offset] != "%":
      offset += 1
      continue
    following = message[offset + 1] if offset + 1 < len(message) else "\0"
    if following == "s":
      strings += 1
    elif following == "d" and index in {5, 6}:
      if strings == 0:
        result.findings.append(
            finding(
                "SHP018",
                "error",
                f"message {index} has %d before %s",
                span,
                "shop",
                record.vnum,
            )
        )
      decimals += 1
    elif following != "%":
      result.findings.append(
          finding(
              "SHP018",
              "error",
              f"message {index} contains invalid format %{following}",
              span,
              "shop",
              record.vnum,
          )
      )
    offset += 2
  if strings > 1 or decimals > 1:
    result.findings.append(
        finding(
            "SHP018",
            "error",
            f"message {index} has too many format fields (%s={strings}, %d={decimals})",
            span,
            "shop",
            record.vnum,
        )
    )


def _parse_shop_record(
    cursor: SourceCursor,
    result: ParseResult[ShopRecord],
    record: ShopRecord,
    manifest: dict[str, Any],
) -> None:
  maximum = manifest["limits"]["MAX_SHOP_OBJ"]["value"]
  record.product_vnums = _read_numeric_list(
      cursor,
      result,
      record,
      record.modern,
      manifest["limits"]["MAX_PROD"]["value"],
      maximum,
      "shop product",
      "object",
  )
  record.profit_buy = _read_float(cursor, result, record, "buy profit")
  record.profit_sell = _read_float(cursor, result, record, "sell profit")
  _read_buy_types(cursor, result, record, manifest, record.modern)
  for index in range(7):
    message, span, complete = _read_string(cursor, result, record.vnum)
    if not complete:
      record.complete = False
      return
    if not message:
      result.findings.append(
          finding("SHP017", "error", f"shop message {index} is empty", span, "shop", record.vnum)
      )
    _validate_message(message, index, span, result, record)
    record.messages.append(message)

  scalar_fields: list[tuple[str, int]] = []
  for label, bits in (
      ("temper", 32),
      ("shop flags", 64),
      ("keeper", 32),
      ("customer restrictions", 32),
  ):
    value = _read_integer(cursor, result, record, label, bits)
    if value is None:
      return
    scalar_fields.append((label, value[0]))
  record.temper = scalar_fields[0][1]
  record.shop_flags = scalar_fields[1][1]
  record.keeper_vnum = scalar_fields[2][1]
  record.customer_restrictions = scalar_fields[3][1]
  if record.keeper_vnum >= 0:
    record.references.append(
        VnumReference("mobile", record.keeper_vnum, "shop keeper", record.span)
    )

  record.room_vnums = _read_numeric_list(
      cursor,
      result,
      record,
      record.modern,
      1,
      maximum,
      "shop room",
      "room",
  )
  for label in ("first open", "first close", "second open", "second close"):
    value = _read_integer(cursor, result, record, label)
    if value is None:
      return
    record.open_hours.append(value[0])
    if not 0 <= value[0] <= 28:
      result.findings.append(
          finding(
              "SHP019",
              "warning",
              f"{label} hour {value[0]} is outside the OLC range 0..28",
              value[1],
              "shop",
              record.vnum,
          )
      )


def parse_shop_file(
    path: Path,
    display_path: str,
    manifest: dict[str, Any],
) -> ParseResult[ShopRecord]:
  result: ParseResult[ShopRecord] = ParseResult()
  try:
    source = SourceFile.from_path(path, display_path)
  except OSError as error:
    result.findings.append(
        finding("SHP002", "error", f"cannot read shop file: {error}", SourceSpan(display_path, 1))
    )
    result.complete = False
    return result
  cursor = SourceCursor(source)
  modern = False
  found_end = False
  while not cursor.eof:
    header, span, complete = _read_string(cursor, result)
    if not complete:
      return result
    if not header:
      result.findings.append(finding("SHP003", "error", "empty shop header string", span))
      result.complete = False
      continue
    if header.startswith("$"):
      found_end = True
      break
    if not header.startswith("#"):
      if "v3.0" in header:
        modern = True
      elif header.startswith("R "):
        match = re.fullmatch(r"R\s+([0-9]+)", header)
        if not result.records or match is None:
          result.findings.append(
              finding("SHP021", "error", "invalid RoL shop extension", span)
          )
          result.complete = False
        else:
          parsed_restrictions = parse_c_integer_token(match.group(1), signed=False, bits=64)
          if parsed_restrictions.error is not None or parsed_restrictions.value is None:
            result.findings.append(
                finding(
                    "SHP021",
                    "error",
                    f"invalid RoL shop restrictions: {parsed_restrictions.error}",
                    span,
                    "shop",
                    result.records[-1].vnum,
                )
            )
            result.complete = False
          else:
            result.records[-1].rol_cheat_restrictions = parsed_restrictions.value
      continue
    match = re.match(r"^#([+-]?\d+)", header)
    if match is None:
      result.findings.append(finding("SHP004", "error", f"invalid shop header {header!r}", span))
      result.complete = False
      continue
    parsed_vnum = parse_c_integer_token(match.group(1))
    if parsed_vnum.error is not None or parsed_vnum.value is None:
      result.findings.append(finding("SHP004", "error", f"invalid shop vnum: {parsed_vnum.error}", span))
      result.complete = False
      continue
    record = ShopRecord(parsed_vnum.value, span, path.stem, modern=modern)
    result.records.append(record)
    if record.vnum < 0:
      result.findings.append(
          finding("SHP006", "error", "shop vnum must be non-negative", span, "shop", record.vnum)
      )
    if not modern:
      result.findings.append(
          finding(
              "SHP005",
              "warning",
              "legacy shop record has fixed five-product/five-buy-type lists; add a v3.0 marker",
              span,
              "shop",
              record.vnum,
          )
      )
    _parse_shop_record(cursor, result, record, manifest)

  if not found_end:
    result.findings.append(
        finding("SHP020", "error", "shop file is missing its '$~' terminator", SourceSpan(display_path, 1))
    )
    result.complete = False
  return result
