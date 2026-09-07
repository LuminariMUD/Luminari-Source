"""RoL shops source grammar and target conversion."""

from __future__ import annotations

import re

from .rol_conversion_types import IdentityResolver, RolRecord, RolSourceCorpus, TransformResult
from .rol_objects import OBJECT_TYPE_MAP
from .rol_source_common import _INTEGER, _diagnostic, _new_record, _reference
from .rol_transform_common import _directive_rows, convert_text
from .source import SourceFile


_SHOP_HEADER = re.compile(br"SHOP\s*:\s*([+-]?\d+)", re.IGNORECASE)


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


def _shop_open_intervals(value: str) -> list[tuple[int, int]]:
  stripped = value.strip()
  open_hours: set[int] = set()
  if len(stripped) >= 24 and all(character.upper() in {"O", "C"} for character in stripped[:24]):
    open_hours = {
        hour for hour, character in enumerate(stripped[:24]) if character.upper() == "C"
    }
  else:
    for match in re.finditer(r"(\d+)\s*[-,]\s*(\d+)", stripped):
      first, last = (int(part) for part in match.groups())
      if first > last or first > 23:
        continue
      open_hours.update(range(first, min(last, 23) + 1))

  intervals: list[tuple[int, int]] = []
  for hour in sorted(open_hours):
    if intervals and hour == intervals[-1][1] + 1:
      intervals[-1] = (intervals[-1][0], hour)
    else:
      intervals.append((hour, hour))
  return intervals


def _shop_message(value: str, monetary: bool = False) -> tuple[str, list[str]]:
  text, diagnostics = convert_text(value)
  text = text.strip()
  wrapper = re.fullmatch(r"\$n\s+says\s+(['\"])(.*)\1", text, flags=re.DOTALL | re.IGNORECASE)
  if wrapper is not None:
    text = wrapper.group(2)
  if monetary:
    text = text.replace("%s", "%d coins")
  text = text.replace("%N", "%s").replace("$N", "%s")
  text = text.replace("$p", "that item").replace("$n", "I")
  if "%s" not in text:
    text = f"%s, {text}" if text else "%s."
  if monetary and "%d" in text and text.find("%d") < text.find("%s"):
    text = "%s, " + text.replace("%s", "you", 1)
    diagnostics.append("moved the shop customer placeholder before the monetary placeholder")
  first_name = text.find("%s")
  text = text[: first_name + 2] + text[first_name + 2 :].replace("%s", "you")
  text = re.sub(r"%(?![%sd])", "%%", text)
  return text, diagnostics


SHOP_CUSTOMER_TOKEN_MAP = {
    "GOODS": 1 << 0,
    "EVILS": 1 << 1,
    "CA": 1 << 6,
    "CB": 1 << 12,
    "CC": 1 << 8,
    "CD": 1 << 11,
    "CE": 1 << 28,
    "CF": 1 << 4,
    "CG": 1 << 7,
    "CH": 1 << 9,
    "CI": 1 << 4,
    "CJ": 1 << 10,
    "CK": 1 << 27,
    "CL": 1 << 3,
    "CM": 1 << 5,
    "CN": 1 << 5,
    "CO": 1 << 6,
    "CP": 1 << 13,
    "CQ": 1 << 29,
    "PH": 1 << 15,
    "PB": 1 << 15,
    "PL": 1 << 24,
    "PE": 1 << 16,
    "PM": 1 << 17,
    "PD": 1 << 25,
    "PF": 1 << 19,
    "PG": 1 << 22,
    "PO": 1 << 18,
    "PT": 1 << 18,
    "P2": 1 << 20,
    "PR": 1 << 21,
    "NPC": 1 << 26,
}


SHOP_CUSTOMER_BOUNDED_TOKENS = {
    "PB": "human",
    "PE": "elf",
    "PM": "dwarf",
    "PO": "half-troll",
    "PT": "half-troll",
    "CI": "cleric",
    "CN": "rogue",
    "CO": "warrior",
}


# Exact active tokens accepted by the source shop_bigot_table scan. Its ALL
# sentinel has value -1 and terminates the scan, so ALL is not a runtime token.
SHOP_SOURCE_CUSTOMER_TOKENS = {
    "PH", "PB", "PL", "PE", "PM", "PD", "PF", "PG", "PO", "PT", "P2", "PI", "PY",
    "CA", "CB", "CC", "CD", "CE", "CF", "CG", "CH", "CI", "CJ", "CK", "CL", "CM", "CN",
    "CO", "CP", "CQ", "CR", "CS", "CT", "CU", "CZ", "CV", "GOODS", "EVILS", "NPC", "OWN",
    "PR", "ALIEN",
}


def _shop_customer_restrictions(
    record: RolRecord,
    directive_token: str,
    diagnostics: list[str],
) -> int:
  restrictions = 0
  for directive in _directive_rows(record, directive_token):
    for token in str(directive.get("text", "")).upper().split():
      mapped = SHOP_CUSTOMER_TOKEN_MAP.get(token)
      if mapped is None:
        if token in SHOP_SOURCE_CUSTOMER_TOKENS:
          diagnostics.append(
              f"omitted source-only shop {directive_token} token {token!r} at source line "
              f"{directive['line']}"
          )
        else:
          diagnostics.append(
              f"omitted source-inert invalid shop {directive_token} token {token!r} at source "
              f"line {directive['line']}"
          )
        continue
      restrictions |= mapped
      if token in SHOP_CUSTOMER_BOUNDED_TOKENS:
        diagnostics.append(
            f"mapped source shop {directive_token} token {token} to target "
            f"{SHOP_CUSTOMER_BOUNDED_TOKENS[token]} customer identity at source line "
            f"{directive['line']}"
        )
  return restrictions


def emit_shop(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
) -> TransformResult:
  """Emit one modern target shop record without the file header or terminator."""

  diagnostics: list[str] = []
  products: list[int] = []
  for directive in _directive_rows(record, "PO"):
    for value in directive.get("arguments", []):
      source_product = int(value)
      if source_product <= 0:
        continue
      try:
        products.append(resolve("obj", source_product))
      except (KeyError, ValueError) as error:
        diagnostics.append(
            f"excluded unresolved shop product {source_product} at source line "
            f"{directive['line']}: {error}"
        )
  buy_types: list[int] = []
  for directive in _directive_rows(record, "BT"):
    for value in directive.get("arguments", []):
      source_type = int(value)
      target_type = OBJECT_TYPE_MAP.get(source_type)
      if target_type is None:
        diagnostics.append(
            f"excluded unsupported shop buy type {source_type} at source line {directive['line']}"
        )
      elif target_type not in buy_types:
        buy_types.append(target_type)

  greed_rows = _directive_rows(record, "GREED")
  profit_rows = _directive_rows(record, "PROFIT")
  greed = int(greed_rows[-1].get("arguments", [100])[0]) if greed_rows else 100
  source_profit = int(profit_rows[-1].get("arguments", [100])[0]) if profit_rows else 100
  profit_buy = max(0.01, greed / 100.0)
  profit_sell = greed / max(1, 100 + source_profit)

  source_messages = {
      token: str(rows[-1].get("text", ""))
      for token in ("MSHAVE", "MBHAVE", "MNBUY", "MSCASH", "MBCASH", "MSELL", "MBUY")
      if (rows := _directive_rows(record, token))
  }
  message_defaults = {
      "MSHAVE": "I do not have that item.",
      "MBHAVE": "You do not have that item.",
      "MNBUY": "I do not buy that kind of item.",
      "MSCASH": "I cannot afford that item.",
      "MBCASH": "You cannot afford that item.",
      "MSELL": "Your purchase costs %s.",
      "MBUY": "I will pay you %s.",
  }
  messages: list[str] = []
  for token in ("MSHAVE", "MBHAVE", "MNBUY", "MSCASH", "MBCASH", "MSELL", "MBUY"):
    message, message_diagnostics = _shop_message(
        source_messages.get(token, message_defaults[token]),
        monetary=token in {"MSELL", "MBUY"},
    )
    diagnostics.extend(message_diagnostics)
    messages.append(message)

  shop_flags = 1 << 6
  if _directive_rows(record, "KILLABLE"):
    shop_flags |= 1
  if _directive_rows(record, "ROAMING"):
    shop_flags |= 1 << 5
  if _directive_rows(record, "CASTING"):
    shop_flags |= 1 << 7

  customer_restrictions = _shop_customer_restrictions(record, "HATES", diagnostics)
  cheat_restrictions = _shop_customer_restrictions(record, "CHEATS", diagnostics)

  keeper = resolve("mob", record.vnum)
  rooms: list[int] = []
  for directive in _directive_rows(record, "ROOM"):
    for value in directive.get("arguments", []):
      source_room = int(value)
      if source_room <= 0:
        continue
      try:
        rooms.append(resolve("wld", source_room))
      except (KeyError, ValueError) as error:
        diagnostics.append(
            f"excluded unresolved shop room {source_room} at source line "
            f"{directive['line']}: {error}"
        )
  hour_rows = _directive_rows(record, "HOURS")
  intervals = _shop_open_intervals(str(hour_rows[-1].get("text", ""))) if hour_rows else []
  if not intervals:
    intervals = [(0, 28)]
    diagnostics.append("source shop has no effective open-hour interval; used always-open target hours")
  if len(intervals) > 2:
    diagnostics.append(
        f"source shop has {len(intervals)} disjoint open intervals; merged intervals after the first"
    )
    intervals = [intervals[0], (intervals[1][0], intervals[-1][1])]
  intervals.extend([(0, 0)] * (2 - len(intervals)))

  if _directive_rows(record, "DEADBEAT"):
    diagnostics.append("omitted source-inert shop DEADBEAT value")
  if _directive_rows(record, "OFFENSE"):
    diagnostics.append(
        "mapped source shop OFFENSE response to the target shopkeeper attack policy"
    )
  source_only_messages = sorted(
      token for token in ("MOPEN", "MCLOSE", "MBIGOT") if _directive_rows(record, token)
  )
  if source_only_messages:
    diagnostics.append(
        "source-only shop behavior messages retained as conversion evidence: "
        + ", ".join(source_only_messages)
    )

  lines = [f"#{destination_vnum}~\n"]
  lines.extend(f"{value}\n" for value in products)
  lines.extend(["-1\n", f"{profit_buy:.4f}\n", f"{profit_sell:.4f}\n"])
  lines.extend(f"{value}\n" for value in buy_types)
  lines.append("-1\n")
  lines.extend(f"{message}~\n" for message in messages)
  lines.extend(
      ["0\n", f"{shop_flags}\n", f"{keeper}\n", f"{customer_restrictions}\n"]
  )
  lines.extend(f"{value}\n" for value in rooms)
  lines.append("-1\n")
  for first, last in intervals:
    lines.extend([f"{first}\n", f"{last}\n"])
  if cheat_restrictions:
    lines.append(f"R {cheat_restrictions}~\n")
  return TransformResult("".join(lines), diagnostics)
