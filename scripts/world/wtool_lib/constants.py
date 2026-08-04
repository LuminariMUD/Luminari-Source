"""Extract and consume bounded world-building constants from the C sources."""

from __future__ import annotations

import ast
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any


MANIFEST_SCHEMA_VERSION = 1
CHUNK_WIDTH = 32
SERIALIZED_CHUNKS = 4
WRITER_ALPHABET = "abcdefghijklmnopqrstuvwxyzABCDEF"
DECODER_ALPHABET = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"


class ExtractionError(RuntimeError):
  """Raised when a selected source block cannot be interpreted safely."""


@dataclass(frozen=True, slots=True)
class TableSpec:
  key: str
  table_name: str
  define_file: str
  start_symbol: str
  end_symbol: str
  prefix: str
  count_symbol: str


TABLE_SPECS = (
    TableSpec("directions", "dirs", "src/structs.h", "NORTH", "NUM_OF_DIRS", "", "NUM_OF_DIRS"),
    TableSpec(
        "room", "room_bits", "src/structs.h", "ROOM_DARK", "NUM_ROOM_FLAGS", "ROOM_", "NUM_ROOM_FLAGS"
    ),
    TableSpec(
        "zone", "zone_bits", "src/structs.h", "ZONE_CLOSED", "NUM_ZONE_FLAGS", "ZONE_", "NUM_ZONE_FLAGS"
    ),
    TableSpec(
        "sectors",
        "sector_types",
        "src/structs.h",
        "SECT_INSIDE",
        "NUM_ROOM_SECTORS",
        "SECT_",
        "NUM_ROOM_SECTORS",
    ),
    TableSpec(
        "positions", "position_types", "src/structs.h", "POS_DEAD", "NUM_POSITIONS", "POS_", "NUM_POSITIONS"
    ),
    TableSpec(
        "mob", "action_bits", "src/structs.h", "MOB_SPEC", "NUM_MOB_FLAGS", "MOB_", "NUM_MOB_FLAGS"
    ),
    TableSpec(
        "affect", "affected_bits", "src/structs.h", "AFF_DONTUSE", "NUM_AFF_FLAGS", "AFF_", "NUM_AFF_FLAGS"
    ),
    TableSpec(
        "affect2",
        "affected2_bits",
        "src/structs.h",
        "AFF2_DONTUSE",
        "NUM_AFF2_FLAGS",
        "AFF2_",
        "NUM_AFF2_FLAGS",
    ),
    TableSpec(
        "item-types", "item_types", "src/structs.h", "ITEM_LIGHT", "NUM_ITEM_TYPES", "ITEM_", "NUM_ITEM_TYPES"
    ),
    TableSpec(
        "obj-wear",
        "wear_bits",
        "src/structs.h",
        "ITEM_WEAR_TAKE",
        "NUM_ITEM_WEARS",
        "ITEM_WEAR_",
        "NUM_ITEM_WEARS",
    ),
    TableSpec(
        "obj-extra", "extra_bits", "src/structs.h", "ITEM_GLOW", "NUM_ITEM_FLAGS", "ITEM_", "NUM_ITEM_FLAGS"
    ),
)


LIMIT_SPECS = {
    "READ_SIZE": "src/utils.h",
    "MAX_STRING_LENGTH": "src/structs.h",
    "MAX_PATH": "src/structs.h",
    "MAX_FILEPATH": "src/structs.h",
    "MAX_OBJ_AFFECT": "src/structs.h",
    "MAX_WEAPON_SPELLS": "src/structs.h",
    "MAX_MOVING_ROOMS": "src/structs.h",
    "NUM_WEARS": "src/structs.h",
    "MAX_SHOP_OBJ": "src/obj/shop.h",
    "MAX_PROD": "src/obj/shop.h",
    "MAX_TRADE": "src/obj/shop.h",
    "NUM_ATTACK_TYPES": "src/structs.h",
    "NUM_CLASSES": "src/structs.h",
    "NUM_FEATS": "src/structs.h",
    "NUM_GENDERS": "src/structs.h",
    "NUM_PORTAL_TYPES": "src/structs.h",
    "NUM_RACE_TYPES": "src/structs.h",
    "NUM_SIZES": "src/structs.h",
    "NUM_SPELLS": "src/magic/spells.h",
    "NUM_SPECABS": "src/combat/spec_abilities.h",
    "NUM_SUB_RACES": "src/structs.h",
    "PORTAL_CHECKFLAGS": "src/structs.h",
    "PORTAL_CLANHALL": "src/structs.h",
    "PORTAL_NORMAL": "src/structs.h",
    "PORTAL_RANDOM": "src/structs.h",
    "RQ_MAXSIZE": "src/db.h",
    "SPELLBOOK_SIZE": "src/structs.h",
    "LVL_IMPL": "src/structs.h",
    "ITEM_SPECAB_HORN_OF_SUMMONING": "src/combat/spec_abilities.h",
    "ITEM_SPECAB_ITEM_SUMMON": "src/combat/spec_abilities.h",
}


TRIGGER_SPECS = (
    ("trigger-types-mob", "trig_types", "MTRIG_GLOBAL", "/* obj trigger types */", "MTRIG_"),
    ("trigger-types-object", "otrig_types", "OTRIG_GLOBAL", "/* wld trigger types */", "OTRIG_"),
    ("trigger-types-world", "wtrig_types", "WTRIG_GLOBAL", "/* obj command trigger types */", "WTRIG_"),
)


_DIRECTION_SYMBOLS = {
    "NORTH",
    "EAST",
    "SOUTH",
    "WEST",
    "UP",
    "DOWN",
    "NORTHWEST",
    "NORTHEAST",
    "SOUTHEAST",
    "SOUTHWEST",
    "IN",
    "OUT",
}


def default_repo_root() -> Path:
  return Path(__file__).resolve().parents[3]


def default_manifest_path(repo_root: Path | None = None) -> Path:
  root = repo_root or default_repo_root()
  return root / "scripts/world/wtool_constants.json"


def _conditional_value(expression: str) -> bool:
  compact = re.sub(r"\s+", "", expression)
  if compact in {"defined(CAMPAIGN_FR)", "definedCAMPAIGN_FR", "CAMPAIGN_FR"}:
    return False
  if compact in {"defined(CAMPAIGN_DL)", "definedCAMPAIGN_DL", "CAMPAIGN_DL"}:
    return False
  if compact in {"!defined(CAMPAIGN_FR)", "!definedCAMPAIGN_FR", "!CAMPAIGN_FR"}:
    return True
  if compact in {"!defined(CAMPAIGN_DL)", "!definedCAMPAIGN_DL", "!CAMPAIGN_DL"}:
    return True
  raise ExtractionError(f"unsupported conditional directive in selected constants block: {expression!r}")


def _filter_luminari_branch(text: str) -> str:
  output: list[str] = []
  stack: list[tuple[bool, bool, bool]] = []
  active = True

  for line in text.splitlines(keepends=True):
    stripped = line.lstrip()
    directive = re.match(r"#\s*(ifdef|ifndef|if|elif|else|endif)\b(.*)", stripped)
    if directive is None:
      if active:
        output.append(line)
      continue

    kind = directive.group(1)
    argument = directive.group(2).strip()
    if kind in {"ifdef", "ifndef", "if"}:
      if kind == "ifdef":
        condition = _conditional_value(argument)
      elif kind == "ifndef":
        condition = not _conditional_value(argument)
      else:
        condition = _conditional_value(argument)
      stack.append((active, condition, False))
      active = active and condition
      continue

    if not stack:
      raise ExtractionError(f"unmatched #{kind} in selected constants block")
    parent_active, branch_taken, saw_else = stack[-1]
    if kind == "elif":
      if saw_else:
        raise ExtractionError("#elif after #else in selected constants block")
      condition = _conditional_value(argument)
      active = parent_active and not branch_taken and condition
      stack[-1] = (parent_active, branch_taken or condition, saw_else)
    elif kind == "else":
      if saw_else:
        raise ExtractionError("duplicate #else in selected constants block")
      active = parent_active and not branch_taken
      stack[-1] = (parent_active, True, True)
    else:
      parent_active, _, _ = stack.pop()
      active = parent_active

  if stack:
    raise ExtractionError("unterminated conditional directive in selected constants block")
  return "".join(output)


def _strip_c_comments(text: str) -> str:
  output: list[str] = []
  index = 0
  state = "code"
  while index < len(text):
    char = text[index]
    next_char = text[index + 1] if index + 1 < len(text) else ""
    if state == "code":
      if char == '"':
        state = "string"
        output.append(char)
      elif char == "'":
        state = "char"
        output.append(char)
      elif char == "/" and next_char == "*":
        state = "block"
        output.extend("  ")
        index += 1
      elif char == "/" and next_char == "/":
        state = "line"
        output.extend("  ")
        index += 1
      else:
        output.append(char)
    elif state in {"string", "char"}:
      output.append(char)
      if char == "\\" and next_char:
        output.append(next_char)
        index += 1
      elif (state == "string" and char == '"') or (state == "char" and char == "'"):
        state = "code"
    elif state == "block":
      if char == "*" and next_char == "/":
        output.extend("  ")
        index += 1
        state = "code"
      else:
        output.append("\n" if char == "\n" else " ")
    else:
      if char == "\n":
        output.append("\n")
        state = "code"
      else:
        output.append(" ")
    index += 1
  return "".join(output)


def _logical_lines(text: str) -> list[str]:
  lines: list[str] = []
  current = ""
  for line in text.splitlines():
    stripped = line.rstrip()
    if stripped.endswith("\\"):
      current += stripped[:-1] + " "
      continue
    lines.append(current + line)
    current = ""
  if current:
    lines.append(current)
  return lines


def _extract_define_block(text: str, start_symbol: str, end_symbol: str) -> str:
  start_match = re.search(rf"(?m)^\s*#\s*define\s+{re.escape(start_symbol)}\b", text)
  if start_match is None:
    raise ExtractionError(f"cannot find start symbol {start_symbol}")
  end_match = re.search(
      rf"(?m)^\s*#\s*define\s+{re.escape(end_symbol)}\b[^\n]*", text[start_match.start() :]
  )
  if end_match is None:
    raise ExtractionError(f"cannot find end symbol {end_symbol}")
  end = start_match.start() + end_match.end()
  return text[start_match.start() : end]


def _extract_marker_block(text: str, start_symbol: str, end_marker: str) -> str:
  start_match = re.search(rf"(?m)^\s*#\s*define\s+{re.escape(start_symbol)}\b", text)
  if start_match is None:
    raise ExtractionError(f"cannot find start symbol {start_symbol}")
  end = text.find(end_marker, start_match.start())
  if end < 0:
    raise ExtractionError(f"cannot find end marker {end_marker!r}")
  return text[start_match.start() : end]


def _eval_expression(expression: str, values: dict[str, int]) -> int:
  try:
    tree = ast.parse(expression, mode="eval")
  except SyntaxError as error:
    raise ExtractionError(f"unsupported constant expression {expression!r}") from error

  def evaluate(node: ast.AST) -> int:
    if isinstance(node, ast.Expression):
      return evaluate(node.body)
    if isinstance(node, ast.Constant) and isinstance(node.value, int):
      return node.value
    if isinstance(node, ast.Name) and node.id in values:
      return values[node.id]
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub, ast.Invert)):
      operand = evaluate(node.operand)
      if isinstance(node.op, ast.UAdd):
        return operand
      if isinstance(node.op, ast.USub):
        return -operand
      return ~operand
    if isinstance(node, ast.BinOp):
      left = evaluate(node.left)
      right = evaluate(node.right)
      operations = {
          ast.Add: lambda: left + right,
          ast.Sub: lambda: left - right,
          ast.Mult: lambda: left * right,
          ast.FloorDiv: lambda: left // right,
          ast.Div: lambda: left // right,
          ast.LShift: lambda: left << right,
          ast.RShift: lambda: left >> right,
          ast.BitOr: lambda: left | right,
          ast.BitAnd: lambda: left & right,
          ast.BitXor: lambda: left ^ right,
      }
      operation = operations.get(type(node.op))
      if operation is not None:
        return operation()
    raise ExtractionError(f"unsupported node in constant expression {expression!r}")

  return evaluate(tree)


def _parse_defines(text: str) -> tuple[dict[str, int], dict[str, str]]:
  filtered = _filter_luminari_branch(text)
  raw_lines = _logical_lines(filtered)
  clean_lines = _logical_lines(_strip_c_comments(filtered))
  values: dict[str, int] = {}
  raw_by_symbol: dict[str, str] = {}
  for raw_line in raw_lines:
    raw_match = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)\b", raw_line)
    if raw_match is not None:
      raw_by_symbol[raw_match.group(1)] = raw_line
  for clean_line in clean_lines:
    match = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$", clean_line)
    if match is None:
      continue
    symbol, expression = match.groups()
    if "(" in symbol:
      continue
    try:
      values[symbol] = _eval_expression(expression.strip(), values)
    except ExtractionError as error:
      raise ExtractionError(f"{symbol}: {error}") from error
  return values, raw_by_symbol


def _extract_array(text: str, table_name: str) -> list[str]:
  declaration = re.search(
      rf"const\s+char\s*\*\s*{re.escape(table_name)}\s*\[\s*\]\s*=\s*\{{",
      text,
  )
  if declaration is None:
    raise ExtractionError(f"cannot find table {table_name}")
  end = text.find("};", declaration.end())
  if end < 0:
    raise ExtractionError(f"unterminated table {table_name}")
  body = _filter_luminari_branch(text[declaration.end() : end])
  clean = _strip_c_comments(body)
  tokens = re.findall(r'"(?:\\.|[^"\\])*"', clean)
  values: list[str] = []
  for token in tokens:
    try:
      value = ast.literal_eval(token)
    except (SyntaxError, ValueError) as error:
      raise ExtractionError(f"invalid string literal in {table_name}: {token}") from error
    if not isinstance(value, str):
      raise ExtractionError(f"non-string entry in {table_name}")
    values.append(value)
  if not values or values[-1] != "\n":
    raise ExtractionError(f"table {table_name} lacks its newline sentinel")
  return values[:-1]


def _reserved(display_name: str, macro: str | None, raw: str | None) -> bool:
  combined = " ".join(part for part in (display_name, macro, raw) if part)
  return bool(
      re.search(r"\(R\)|DONTUSE|UNUSED|NOTDEADYET|BFS_MARK", combined, re.IGNORECASE)
      or display_name in {"*", "<spec>", "\0"}
  )


def _entry_table(
    display_names: list[str],
    values: dict[str, int],
    raw_by_symbol: dict[str, str],
    prefix: str,
    table_key: str,
) -> list[dict[str, Any]]:
  symbols_by_value: dict[int, list[str]] = {}
  for symbol, value in values.items():
    if symbol.startswith("NUM_"):
      continue
    if table_key == "directions":
      if symbol not in _DIRECTION_SYMBOLS:
        continue
    elif not symbol.startswith(prefix):
      continue
    if 0 <= value < len(display_names):
      symbols_by_value.setdefault(value, []).append(symbol)

  entries: list[dict[str, Any]] = []
  for index, display_name in enumerate(display_names):
    symbols = symbols_by_value.get(index, [])
    macro = symbols[0] if symbols else None
    raw = raw_by_symbol.get(macro) if macro is not None else None
    entries.append(
        {
            "index": index,
            "name": display_name,
            "macro": macro,
            "aliases": symbols[1:],
            "reserved": _reserved(display_name, macro, raw),
        }
    )
  return entries


def _extract_limit(repo_root: Path, symbol: str, source_path: str) -> int:
  text = (repo_root / source_path).read_text(encoding="utf-8")
  clean = _strip_c_comments(text)
  values: dict[str, int] = {}
  for line in _logical_lines(clean):
    match = re.match(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s+(.+?)\s*$", line)
    if match is None:
      continue
    name, expression = match.groups()
    try:
      values[name] = _eval_expression(expression.strip(), values)
    except ExtractionError:
      continue
    if name == symbol:
      return values[name]
  raise ExtractionError(f"cannot extract {symbol} from {source_path}")


def _extract_branch_limit(
    repo_root: Path,
    symbol: str,
    start_marker: str,
    end_marker: str,
) -> int:
  text = (repo_root / "src/structs.h").read_text(encoding="utf-8")
  start = text.find(start_marker)
  if start < 0:
    raise ExtractionError(f"cannot find branch marker {start_marker!r}")
  conditional = text.find("#if", start)
  end = text.find(end_marker, conditional)
  if conditional < 0 or end < 0:
    raise ExtractionError(f"cannot bound branch constant {symbol}")
  values, _ = _parse_defines(text[conditional:end])
  if symbol not in values:
    raise ExtractionError(f"cannot extract {symbol} from Luminari branch")
  return values[symbol]


def extract_manifest(repo_root: Path | None = None) -> dict[str, Any]:
  root = (repo_root or default_repo_root()).resolve()
  constants_text = (root / "src/constants.c").read_text(encoding="utf-8")
  tables: dict[str, Any] = {}

  for spec in TABLE_SPECS:
    source_text = (root / spec.define_file).read_text(encoding="utf-8")
    if spec.key == "directions":
      block = _extract_marker_block(
          source_text, spec.start_symbol, "/* ============================================================================ */"
      )
    else:
      block = _extract_define_block(source_text, spec.start_symbol, spec.end_symbol)
    values, raw_by_symbol = _parse_defines(block)
    display_names = _extract_array(constants_text, spec.table_name)
    count = values.get(spec.count_symbol)
    if count is None:
      raise ExtractionError(f"{spec.count_symbol} was not resolved")
    if count != len(display_names):
      raise ExtractionError(
          f"{spec.table_name} has {len(display_names)} entries but {spec.count_symbol} is {count}"
      )
    entries = _entry_table(display_names, values, raw_by_symbol, spec.prefix, spec.key)
    missing = [entry["index"] for entry in entries if entry["macro"] is None]
    allowed_missing = [0] if spec.key == "item-types" else []
    if missing != allowed_missing:
      raise ExtractionError(f"{spec.table_name} has unmatched source indices: {missing}")
    tables[spec.key] = {
        "source_table": f"src/constants.c:{spec.table_name}",
        "source_defines": f"{spec.define_file}:{spec.start_symbol}..{spec.end_symbol}",
        "count_symbol": spec.count_symbol,
        "entries": entries,
    }

  wear_source = (root / "src/obj/act.item.c").read_text(encoding="utf-8")
  wear_match = re.search(r"int\s+wear_bitvectors\[\]\s*=\s*\{(.*?)\};", wear_source, re.DOTALL)
  if wear_match is None:
    raise ExtractionError("cannot find wear_bitvectors[] in src/obj/act.item.c")
  wear_macros = re.findall(r"\bITEM_WEAR_[A-Z0-9_]+\b", _strip_c_comments(wear_match.group(1)))
  equipment_names = _extract_array(constants_text, "equipment_types")
  if equipment_names and equipment_names[-1] == "\n":
    equipment_names.pop()
  num_wears = _extract_limit(root, "NUM_WEARS", "src/structs.h")
  if len(wear_macros) != num_wears or len(equipment_names) != num_wears:
    raise ExtractionError(
        "wear_bitvectors/equipment_types do not match NUM_WEARS: "
        f"{len(wear_macros)}/{len(equipment_names)}/{num_wears}"
    )
  wear_indices = {
      entry["macro"]: entry["index"] for entry in tables["obj-wear"]["entries"]
  }
  tables["wear-slots"] = {
      "source_table": "src/obj/act.item.c:perform_wear.wear_bitvectors",
      "source_names": "src/constants.c:equipment_types",
      "entries": [
          {
              "index": index,
              "name": equipment_names[index],
              "required_wear_macro": macro,
              "required_wear_index": wear_indices[macro],
          }
          for index, macro in enumerate(wear_macros)
      ],
  }

  trigger_header = (root / "src/dgscript/dg_scripts.h").read_text(encoding="utf-8")
  for key, table_name, start_symbol, end_marker, prefix in TRIGGER_SPECS:
    block = _extract_marker_block(trigger_header, start_symbol, end_marker)
    values, raw_by_symbol = _parse_defines(block)
    display_names = _extract_array(constants_text, table_name)
    symbols_by_index: dict[int, list[str]] = {}
    for symbol, mask in values.items():
      if not symbol.startswith(prefix) or mask <= 0 or mask & (mask - 1):
        continue
      index = mask.bit_length() - 1
      if index < len(display_names):
        symbols_by_index.setdefault(index, []).append(symbol)
    entries: list[dict[str, Any]] = []
    for index, display_name in enumerate(display_names):
      symbols = symbols_by_index.get(index, [])
      macro = symbols[0] if symbols else None
      entries.append(
          {
              "index": index,
              "name": display_name,
              "macro": macro,
              "aliases": symbols[1:],
              "reserved": _reserved(display_name, macro, raw_by_symbol.get(macro or "")),
          }
      )
    tables[key] = {
        "source_table": f"src/constants.c:{table_name}",
        "source_defines": f"src/dgscript/dg_scripts.h:{start_symbol}",
        "entries": entries,
    }

  limits = {
      symbol: {
          "value": _extract_limit(root, symbol, source_path),
          "source": f"{source_path}:{symbol}",
      }
      for symbol, source_path in sorted(LIMIT_SPECS.items())
  }
  limits["NUM_CITIES"] = {
      "value": _extract_branch_limit(root, "NUM_CITIES", "// cities", "/* Positions */"),
      "source": "src/structs.h:NUM_CITIES (LUMINARI branch)",
  }
  limits["NUM_FACTIONS"] = {
      "value": _extract_branch_limit(root, "NUM_FACTIONS", "/* factions */", "// cities"),
      "source": "src/structs.h:NUM_FACTIONS (LUMINARI branch)",
  }
  limits["NUM_REGIONS"] = {
      "value": _extract_branch_limit(root, "NUM_REGIONS", "#define NUM_SEX", "/* factions */"),
      "source": "src/structs.h:NUM_REGIONS (LUMINARI branch)",
  }
  return {
      "schema_version": MANIFEST_SCHEMA_VERSION,
      "source_branch": "LUMINARI",
      "flag_encoding": {
          "chunk_width": CHUNK_WIDTH,
          "serialized_chunks": SERIALIZED_CHUNKS,
          "writer_alphabet": WRITER_ALPHABET,
          "decoder_alphabet": DECODER_ALPHABET,
      },
      "flag_set_aliases": {
          "obj-affect": "affect",
          "obj-affect2": "affect2",
      },
      "limits": limits,
      "tables": tables,
  }


def manifest_text(manifest: dict[str, Any]) -> str:
  return json.dumps(manifest, ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def load_manifest(path: Path | None = None) -> dict[str, Any]:
  manifest_path = path or default_manifest_path()
  try:
    data = json.loads(manifest_path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise ExtractionError(f"cannot load constants manifest {manifest_path}: {error}") from error
  if data.get("schema_version") != MANIFEST_SCHEMA_VERSION:
    raise ExtractionError(
        f"unsupported constants manifest schema {data.get('schema_version')!r}; "
        f"expected {MANIFEST_SCHEMA_VERSION}"
    )
  return data


def check_manifest(repo_root: Path | None = None, path: Path | None = None) -> tuple[bool, str]:
  root = repo_root or default_repo_root()
  manifest_path = path or default_manifest_path(root)
  fresh = manifest_text(extract_manifest(root))
  try:
    current = manifest_path.read_text(encoding="ascii")
  except (OSError, UnicodeError) as error:
    return False, f"cannot read constants manifest {manifest_path}: {error}"
  if current == fresh:
    return True, "constants manifest is current"
  return False, "constants manifest is stale; run 'wtool constants sync --write'"


def write_manifest(repo_root: Path | None = None, path: Path | None = None) -> Path:
  root = repo_root or default_repo_root()
  manifest_path = path or default_manifest_path(root)
  manifest_path.parent.mkdir(parents=True, exist_ok=True)
  content = manifest_text(extract_manifest(root))
  descriptor, temporary_name = tempfile.mkstemp(
      prefix=f".{manifest_path.name}.", suffix=".tmp", dir=manifest_path.parent
  )
  temporary = Path(temporary_name)
  try:
    with os.fdopen(descriptor, "w", encoding="ascii", newline="\n") as handle:
      handle.write(content)
      handle.flush()
      os.fsync(handle.fileno())
    os.replace(temporary, manifest_path)
  except BaseException:
    temporary.unlink(missing_ok=True)
    raise
  return manifest_path
