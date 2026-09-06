"""Infer RoL armor families against the native setarmor() table.

Source protection is not an inference signal. The emitter and review audit use
the same disposition: a standard family/slot, dedicated tail, or general worn.
"""

from __future__ import annotations

from collections import Counter
from dataclasses import asdict, dataclass
from functools import lru_cache
import json
from pathlib import Path
import re
from typing import Iterable

from .constants import default_repo_root
from .rol_conversion_types import RolRecord
from .rol_weapon_table import (
    _call_bodies, _evaluate, _split_arguments, _string_literal, _strip_comments,
    target_defines,
)


FAMILIES = (
    "CLOTHING", "PADDED", "LEATHER", "STUDDED_LEATHER", "LIGHT_CHAIN", "HIDE",
    "SCALE", "CHAINMAIL", "PIECEMEAL", "SPLINT", "BANDED", "HALF_PLATE", "FULL_PLATE",
)
SHIELDS = ("BUCKLER", "SMALL_SHIELD", "LARGE_SHIELD", "TOWER_SHIELD")
SLOT_NAMES = {3: "BODY", 4: "HEAD", 5: "LEGS", 8: "ARMS", 9: "SHIELD"}
_TABLE_FIELDS = (
    "name", "armor_type", "cost", "armor_bonus", "dex_cap", "armor_check",
    "spell_failure", "move_30", "move_20", "weight", "material", "wear", "description",
)
_SOURCE_COLOR = re.compile(r"&[+-].|&=..|&[Nn]")
_KEYWORDS = (
    ("HALF_PLATE", ("half plate", "breastplate", "cuirass")),
    ("FULL_PLATE", ("full plate", "plate mail", "platemail", "field plate", "plate",
                    "plates", "plated")),
    ("BANDED", ("banded",)),
    ("SPLINT", ("splint", "splinted")),
    ("PIECEMEAL", ("piecemeal", "patchwork", "mismatched", "cobbled together")),
    ("LIGHT_CHAIN", ("elven chain", "light chain", "mithril chain", "chain shirt",
                     "fine mesh")),
    ("SCALE", ("scale", "scales", "scalemail", "lamellar", "dragonscale")),
    ("CHAINMAIL", ("chainmail", "chain mail", "ringmail", "ring mail", "hauberk",
                   "maille", "chain", "mail")),
    ("STUDDED_LEATHER", ("studded",)),
    ("LEATHER", ("leather", "suede")),
    ("HIDE", ("hide", "hides", "demonhide", "dragonhide", "pelts", "pelt", "fur", "skin",
              "batskin", "snakeskin", "lizardskin", "shell", "turtleshell", "carapace",
              "chitin", "bone", "bones", "tusk")),
    ("PADDED", ("padded", "quilted", "gambeson", "aketon", "felt", "knit", "wool",
                "woolen", "velvet")),
    ("CLOTHING", ("robe", "robes", "cloak", "tunic", "shirt", "silk", "cloth", "vest",
                  "gown", "dress", "linen", "sash", "toga", "headband", "tiara",
                  "circlet", "crown", "bandana", "hood", "cowl", "mitre", "hat", "cap",
                  "veil", "pants", "skirt", "trousers", "overalls", "overall s",
                  "head band", "headdress", "lingerie", "apron", "wreath", "rags")),
)
_MATERIALS = (
    ("CHAINMAIL", ("iron", "steel", "alloy", "bronze", "brass", "metal", "adamantine",
                   "mithril", "cyanite")),
    ("PADDED", ("oaken", "ironwood", "wooden", "wood", "vine", "root", "reed", "straw")),
    ("CLOTHING", ("gold", "silver", "jewel", "gem", "crystal", "pearl", "ivory", "feather")),
)


@lru_cache(maxsize=4)
def armor_table(root: Path | None = None) -> dict[int, dict[str, int | str]]:
  """Read actual initialized entries, with the same C reader as weapons."""
  base = root or default_repo_root()
  defines = target_defines(base)
  source = _strip_comments((base / "src/combat/assign_wpn_armor.c").read_text())
  table = {}
  for body in _call_bodies(source, "setarmor"):
    args = _split_arguments(body)
    index = _evaluate(args[0], defines)
    if index is None:
      continue  # Function declaration, not a table initializer.
    if len(args) != len(_TABLE_FIELDS) + 1 or index in table:
      raise ValueError(f"invalid or duplicate native armor initializer {args[0]}")
    entry = {"constant": args[0]}
    for field, term in zip(_TABLE_FIELDS, args[1:]):
      value = (
          _string_literal(term) if field in {"name", "description"} else _evaluate(term, defines)
      )
      if value is None:
        raise ValueError(f"unresolved native armor field {args[0]} {field}: {term}")
      # assign_wpn_armor.h stores these as ubyte; shields author 999 for the
      # unused movement fields, which the native assignment stores as 231.
      if field in {"armor_type", "armor_bonus", "dex_cap", "spell_failure",
                   "move_30", "move_20", "material"}:
        value %= 256
      elif field in {"cost", "weight"}:
        value %= 65536
      elif field == "armor_check":
        value = (value + 128) % 256 - 128
      entry[field] = value
    table[index] = entry
  if set(table) != set(range(1, defines["NUM_SPEC_ARMOR_TYPES"])):
    raise ValueError("native armor table has missing or undefined entries")
  return table


def family_entry(family: str, slot: int) -> tuple[int, dict[str, int | str]]:
  """Resolve named constants, never numeric offsets across the irregular grid."""
  if slot == 9 and family not in SHIELDS or slot != 9 and family not in FAMILIES:
    raise ValueError(f"armor family {family} does not support slot {slot}")
  if slot not in SLOT_NAMES:
    raise ValueError(f"no native armor family for slot {slot}")
  suffix = "" if slot in {3, 9} else "_" + SLOT_NAMES[slot]
  constant = "SPEC_ARMOR_TYPE_" + family + suffix
  defines = target_defines()
  index = defines[constant]
  entry = armor_table().get(index)
  if index in {0, defines["SPEC_ARMOR_TYPE_CHAIN_HEAD"]} or entry is None or entry["wear"] != slot:
    raise ValueError(f"undefined, duplicate-only or slot-mismatched armor entry {constant}")
  return index, entry


@lru_cache(maxsize=1)
def load_overrides() -> dict[str, dict]:
  path = Path(__file__).with_name("rol_armor_overrides.json")
  return json.loads(path.read_text(encoding="ascii"))["overrides"]


@dataclass(frozen=True)
class ArmorInference:
  disposition: str
  slot: int | None
  family: str | None
  armor_index: int
  tier: str
  rule: str

  @property
  def diagnostic(self) -> str:
    return (f"converted source armor to {self.disposition}: family {self.family or 'none'}, "
            f"slot {self.slot}, index {self.armor_index}; {self.tier}: {self.rule}")


def _text(value: str) -> str:
  # Removing an embedded color escape must not split a word such as 'ch&+rain'.
  return " " + re.sub(r"[^a-z0-9]+", " ", _SOURCE_COLOR.sub("", value).lower()).strip() + " "


def _match(text: str, rules: tuple) -> tuple[str, str] | None:
  for family, phrases in rules:
    for phrase in phrases:
      if f" {phrase} " in text:
        return family, phrase
  return None


def infer_armor(record: RolRecord) -> ArmorInference:
  """Classify source armor without mutating source data or examining its AC."""
  if record.values.get("item_type") != 9:
    raise ValueError("armor inference requires source ITEM_ARMOR")
  flags = record.values.get("flags", [])
  mask = flags[2] if len(flags) > 2 else 0
  override = load_overrides().get(f"{record.path}:{record.vnum}")
  if override is not None and override["source_sha256"] != record.sha256:
    raise ValueError(f"stale armor override for {record.record_id}")
  if mask & (1 << 22) and not mask & (1 << 1):
    return ArmorInference("dedicated-tail", 34, None, 0, "slot", "native tail AC exception")
  slots = {slot for slot in SLOT_NAMES if mask & (1 << slot)}
  if not slots:
    return ArmorInference("worn", None, None, 0, "slot", "no native armor slot")
  if len(slots) != 1:
    raise ValueError(f"multiple armor slots require a reviewed disposition: {record.record_id}")
  slot = next(iter(slots))
  if mask & ~((1 << slot) | 1) and (override is None or override.get("slot") != slot):
    raise ValueError(f"mixed armor wear mask requires a reviewed disposition: {record.record_id}")
  if override is not None:
    family = override["family"]
    tier, rule = "override", override["rationale"]
  else:
    strings = record.values.get("strings", {})
    identity = _text(strings.get("aliases", "") + " " + strings.get("short_description", ""))
    details = _text(strings.get("description", "") + " " + " ".join(
        d.get("description", "") for d in record.directives
        if d["token"] == "E" and d.get("source_disposition") != "EXCLUDE"
    ))
    if " string me " in identity or " string this " in identity:
      family = "SMALL_SHIELD" if slot == 9 else "CLOTHING"
      tier, rule = "placeholder", "unstrung prototype"
    else:
      rules = (
          ("TOWER_SHIELD", ("tower shield", "wall shield", "pavise")),
          ("LARGE_SHIELD", ("large shield", "heavy shield", "kite shield", "great shield")),
          ("BUCKLER", ("buckler", "targe")),
          ("SMALL_SHIELD", ("shield", "aegis")),
      ) if slot == 9 else _KEYWORDS
      match = _match(identity, rules)
      tier = "identity"
      if match is None:
        match, tier = _match(details, rules), "description"
      if match is None and slot != 9:
        match, tier = _match(identity + details, _MATERIALS), "material"
      if match is None:
        family = "SMALL_SHIELD" if slot == 9 else "CLOTHING"
        tier, rule = "fallback", "no construction evidence"
      else:
        family, rule = match
  index, _ = family_entry(family, slot)
  return ArmorInference("standard", slot, family, index, tier, rule)


def audit(records: Iterable[RolRecord]) -> dict:
  """Produce stable source-identified dispositions and actual family penalties."""
  rows = []
  for record in records:
    if record.kind != "obj" or record.values.get("item_type") != 9:
      continue
    inference = infer_armor(record)
    rows.append({
        "record_id": record.record_id, "source_sha256": record.sha256,
        "path": record.path, "vnum": record.vnum, "identity": record.identity,
        "source_values": record.values.get("values", []),
        "source_flags": record.values.get("flags", []),
        **asdict(inference),
        "native_family": armor_table().get(inference.armor_index),
    })
  rows.sort(key=lambda row: (row["path"], row["vnum"], row["record_id"]))
  counts = Counter(row["disposition"] for row in rows)
  return {"records": rows, "counts": dict(sorted(counts.items()))}
