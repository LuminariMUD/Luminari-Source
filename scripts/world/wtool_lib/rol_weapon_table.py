"""Read the target ``weapon_list[]`` table out of the live C sources.

``set_weapon_object()`` (``src/obj/treasure.c``) is what an immortal's OLC
session runs when it is handed a weapon type: it derives dice, cost, weight,
material, and size from ``weapon_list[type]`` and writes them onto the object.
A converted RoL weapon has to come out of the converter mechanically identical
to that, so the converter needs the same table.

There is no runtime bridge between the C server and these scripts, so the table
is parsed statically from the ``setweapon()`` calls in
``src/combat/assign_wpn_armor.c``. The argument order is fixed by the function
signature in that same file, and every symbolic argument resolves against the
``#define`` block in ``src/structs.h``. ``rol_mob_calculator.py`` is the
precedent for treating the C side as the authority this way.

``test_rol_weapon_mapping.py`` fails when the parse stops covering every
``WEAPON_TYPE_*`` the header declares, which is the drift guard for this file.
"""

from __future__ import annotations

from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
import re
from typing import Iterable

from .constants import default_repo_root

# The setweapon() parameter list, in order, after the leading weapon type. Kept
# as a table so a signature change in the C side is a one-line edit here.
SETWEAPON_FIELDS = (
    "name",
    "num_dice",
    "dice_size",
    "crit_range",
    "crit_mult",
    "weapon_flags",
    "cost",
    "damage_types",
    "weight",
    "range",
    "weapon_family",
    "size",
    "material",
    "handle_type",
    "head_type",
    "description",
)

_DEFINE = re.compile(r"^#define\s+([A-Z][A-Z0-9_]*)\s+(.+?)\s*(?:/\*.*)?$", re.M)
_INT_LITERAL = re.compile(r"^-?\d+$")


@dataclass(frozen=True)
class WeaponEntry:
  """One row of the target ``weapon_list[]`` table."""

  weapon_type: int
  name: str
  num_dice: int
  dice_size: int
  crit_range: int
  crit_mult: int
  weapon_flags: int
  cost: int
  damage_types: int
  weight: int
  range: int
  weapon_family: int
  size: int
  material: int
  handle_type: int
  head_type: int


def _strip_comments(text: str) -> str:
  text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
  return re.sub(r"//[^\n]*", " ", text)


@lru_cache(maxsize=4)
def target_defines(root: Path | None = None) -> dict[str, int]:
  """Harvest every integer ``#define`` the weapon table can reference."""

  base = root or default_repo_root()
  header = (base / "src/structs.h").read_text(encoding="utf-8", errors="ignore")
  defines: dict[str, int] = {}
  for name, body in _DEFINE.findall(header):
    value = _evaluate(body.strip(), defines)
    if value is not None:
      defines[name] = value
  return defines


def _evaluate(expression: str, defines: dict[str, int]) -> int | None:
  """Resolve a C integer constant expression over already-known defines.

  Only the forms the weapon table actually uses are supported: integer
  literals, defined names, parenthesized shifts, and ``|`` unions of those.
  Anything else resolves to None and is skipped rather than guessed at.
  """

  text = expression.strip()
  if not text:
    return None
  total = 0
  for term in text.split("|"):
    value = _evaluate_term(term.strip(), defines)
    if value is None:
      return None
    total |= value
  return total


def _evaluate_term(term: str, defines: dict[str, int]) -> int | None:
  while term.startswith("(") and term.endswith(")"):
    term = term[1:-1].strip()
  if _INT_LITERAL.match(term):
    return int(term)
  shift = re.match(r"^(\d+)\s*<<\s*(\d+)$", term)
  if shift:
    return int(shift.group(1)) << int(shift.group(2))
  if re.match(r"^[A-Za-z_]\w*$", term):
    return defines.get(term)
  return None


def _split_arguments(text: str) -> list[str]:
  """Split one call's argument list on top-level commas."""

  arguments: list[str] = []
  depth = 0
  in_string = False
  escaped = False
  current: list[str] = []
  for character in text:
    if in_string:
      current.append(character)
      if escaped:
        escaped = False
      elif character == "\\":
        escaped = True
      elif character == '"':
        in_string = False
      continue
    if character == '"':
      in_string = True
      current.append(character)
    elif character in "([":
      depth += 1
      current.append(character)
    elif character in ")]":
      depth -= 1
      current.append(character)
    elif character == "," and depth == 0:
      arguments.append("".join(current).strip())
      current = []
    else:
      current.append(character)
  arguments.append("".join(current).strip())
  return arguments


def _call_bodies(text: str, function: str = "setweapon") -> Iterable[str]:
  for match in re.finditer(r"\b" + re.escape(function) + r"\s*\(", text):
    depth = 1
    index = match.end()
    in_string = False
    escaped = False
    while index < len(text) and depth:
      character = text[index]
      if in_string:
        if escaped:
          escaped = False
        elif character == "\\":
          escaped = True
        elif character == '"':
          in_string = False
      elif character == '"':
        in_string = True
      elif character == "(":
        depth += 1
      elif character == ")":
        depth -= 1
      index += 1
    yield text[match.end():index - 1]


def _string_literal(term: str) -> str:
  pieces = re.findall(r'"((?:[^"\\]|\\.)*)"', term)
  return "".join(pieces).encode("ascii", "replace").decode("ascii")


@lru_cache(maxsize=4)
def weapon_table(root: Path | None = None) -> dict[int, WeaponEntry]:
  """Parse ``load_weapons()`` into ``{weapon type: WeaponEntry}``."""

  base = root or default_repo_root()
  defines = target_defines(base)
  source = _strip_comments(
      (base / "src/combat/assign_wpn_armor.c").read_text(encoding="utf-8", errors="ignore")
  )
  table: dict[int, WeaponEntry] = {}
  for body in _call_bodies(source):
    arguments = _split_arguments(body)
    if len(arguments) != len(SETWEAPON_FIELDS) + 1:
      continue
    weapon_type = _evaluate(arguments[0], defines)
    if weapon_type is None:
      continue
    fields: dict[str, object] = {}
    incomplete = False
    for field, term in zip(SETWEAPON_FIELDS, arguments[1:]):
      if field == "name":
        fields[field] = _string_literal(term)
        continue
      if field == "description":
        continue
      value = _evaluate(term, defines)
      if value is None:
        incomplete = True
        break
      fields[field] = value
    if incomplete:
      continue
    table[weapon_type] = WeaponEntry(weapon_type=weapon_type, **fields)  # type: ignore[arg-type]
  return table
