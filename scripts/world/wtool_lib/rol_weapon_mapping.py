"""Infer Luminari weapon types for converted Realms of Luminari weapons.

Realms of Luminari stores no weapon identity. A source weapon carries a proc
hook in ``value[0]``, damage dice in ``value[1]``/``value[2]``, and a verb index
in ``value[3]``; nothing in the record names the weapon. Luminari drives every
weapon off ``value[0]``, an index into ``weapon_list[]`` (``WEAPON_TYPE_*`` 1..79
in ``src/structs.h``, populated by ``load_weapons()`` in
``src/combat/assign_wpn_armor.c``). Passing the source ``value[0]`` through lands
converted weapons on ``WEAPON_TYPE_UNDEFINED``, which disables criticals, leaves
the damage-type bitmask empty so damage reduction never bypasses, and matches no
weapon family so weapon feats do nothing.

This module reconstructs that identity from the signals the source record does
carry, in three tiers:

  1. A curated override catalog keyed by source vnum
     (``rol_weapon_overrides.json``), for records whose text is evocative but
     non-standard -- monster body parts, improvised objects, siege pieces.
  2. An ordered keyword rule engine over the record's aliases and short
     description, gated on handedness where a name is ambiguous.
  3. A mechanical fallback matrix over the source verb and handedness, which
     guarantees every record receives a defined weapon type.

Inference runs against the *source* record. The source verb in ``value[3]`` is
rewritten in place by ``rol_transform._object_values()``, which remaps it to the
target damage-message id; after that call crush 4/5/6 have collapsed to 6,
bludgeon 7 has become 5, and bite 10 has become 4, so the verb is no longer
recoverable. Classify from ``RolRecord`` before or independently of that call,
never from its return value.
"""

from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Any, Callable, Iterable

# Mirrors the WEAPON_TYPE_* block in src/structs.h. test_rol_weapon_mapping.py
# reparses that header and fails if the two drift apart.
WEAPON_TYPE_NAMES: dict[int, str] = {
    0: "WEAPON_TYPE_UNDEFINED",
    1: "WEAPON_TYPE_UNARMED",
    2: "WEAPON_TYPE_DAGGER",
    3: "WEAPON_TYPE_LIGHT_MACE",
    4: "WEAPON_TYPE_SICKLE",
    5: "WEAPON_TYPE_CLUB",
    6: "WEAPON_TYPE_HEAVY_MACE",
    7: "WEAPON_TYPE_MORNINGSTAR",
    8: "WEAPON_TYPE_SHORTSPEAR",
    9: "WEAPON_TYPE_LONGSPEAR",
    10: "WEAPON_TYPE_QUARTERSTAFF",
    11: "WEAPON_TYPE_SPEAR",
    12: "WEAPON_TYPE_HEAVY_CROSSBOW",
    13: "WEAPON_TYPE_LIGHT_CROSSBOW",
    14: "WEAPON_TYPE_DART",
    15: "WEAPON_TYPE_JAVELIN",
    16: "WEAPON_TYPE_SLING",
    17: "WEAPON_TYPE_THROWING_AXE",
    18: "WEAPON_TYPE_LIGHT_HAMMER",
    19: "WEAPON_TYPE_HAND_AXE",
    20: "WEAPON_TYPE_KUKRI",
    21: "WEAPON_TYPE_LIGHT_PICK",
    22: "WEAPON_TYPE_SAP",
    23: "WEAPON_TYPE_SHORT_SWORD",
    24: "WEAPON_TYPE_BATTLE_AXE",
    25: "WEAPON_TYPE_FLAIL",
    26: "WEAPON_TYPE_LONG_SWORD",
    27: "WEAPON_TYPE_HEAVY_PICK",
    28: "WEAPON_TYPE_RAPIER",
    29: "WEAPON_TYPE_SCIMITAR",
    30: "WEAPON_TYPE_TRIDENT",
    31: "WEAPON_TYPE_WARHAMMER",
    32: "WEAPON_TYPE_FALCHION",
    33: "WEAPON_TYPE_GLAIVE",
    34: "WEAPON_TYPE_GREAT_AXE",
    35: "WEAPON_TYPE_GREAT_CLUB",
    36: "WEAPON_TYPE_HEAVY_FLAIL",
    37: "WEAPON_TYPE_GREAT_SWORD",
    38: "WEAPON_TYPE_GUISARME",
    39: "WEAPON_TYPE_HALBERD",
    40: "WEAPON_TYPE_LANCE",
    41: "WEAPON_TYPE_RANSEUR",
    42: "WEAPON_TYPE_SCYTHE",
    43: "WEAPON_TYPE_LONG_BOW",
    44: "WEAPON_TYPE_SHORT_BOW",
    45: "WEAPON_TYPE_COMPOSITE_LONGBOW",
    46: "WEAPON_TYPE_COMPOSITE_SHORTBOW",
    47: "WEAPON_TYPE_KAMA",
    48: "WEAPON_TYPE_NUNCHAKU",
    49: "WEAPON_TYPE_SAI",
    50: "WEAPON_TYPE_SIANGHAM",
    51: "WEAPON_TYPE_BASTARD_SWORD",
    52: "WEAPON_TYPE_DWARVEN_WAR_AXE",
    53: "WEAPON_TYPE_WHIP",
    54: "WEAPON_TYPE_SPIKED_CHAIN",
    55: "WEAPON_TYPE_DOUBLE_AXE",
    56: "WEAPON_TYPE_DIRE_FLAIL",
    57: "WEAPON_TYPE_HOOKED_HAMMER",
    58: "WEAPON_TYPE_2_BLADED_SWORD",
    59: "WEAPON_TYPE_DWARVEN_URGOSH",
    60: "WEAPON_TYPE_HAND_CROSSBOW",
    61: "WEAPON_TYPE_HEAVY_REP_XBOW",
    62: "WEAPON_TYPE_LIGHT_REP_XBOW",
    63: "WEAPON_TYPE_BOLA",
    64: "WEAPON_TYPE_NET",
    65: "WEAPON_TYPE_SHURIKEN",
    66: "WEAPON_TYPE_COMPOSITE_LONGBOW_2",
    67: "WEAPON_TYPE_COMPOSITE_LONGBOW_3",
    68: "WEAPON_TYPE_COMPOSITE_LONGBOW_4",
    69: "WEAPON_TYPE_COMPOSITE_LONGBOW_5",
    70: "WEAPON_TYPE_COMPOSITE_SHORTBOW_2",
    71: "WEAPON_TYPE_COMPOSITE_SHORTBOW_3",
    72: "WEAPON_TYPE_COMPOSITE_SHORTBOW_4",
    73: "WEAPON_TYPE_COMPOSITE_SHORTBOW_5",
    74: "WEAPON_TYPE_WARMAUL",
    75: "WEAPON_TYPE_KHOPESH",
    76: "WEAPON_TYPE_KNIFE",
    77: "WEAPON_TYPE_HOOPAK",
    78: "WEAPON_TYPE_FOOTMANS_LANCE",
    79: "WEAPON_TYPE_ATHAME",
}

WEAPON_TYPE_IDS: dict[str, int] = {name: value for value, name in WEAPON_TYPE_NAMES.items()}
NUM_WEAPON_TYPES = 80

# Weapon types load_weapons() marks WEAPON_FLAG_RANGED. A source ITEM_WEAPON is
# a melee record -- it carries a melee verb and the source runtime resolves it
# through no ammunition system -- so inference never emits one of these. A
# converted bow or ballista becomes the melee weapon it is actually swung as.
# test_rol_weapon_mapping.py reparses the setweapon() calls and fails if the set
# drifts.
RANGED_WEAPON_TYPES = frozenset({
    "WEAPON_TYPE_HEAVY_CROSSBOW",
    "WEAPON_TYPE_LIGHT_CROSSBOW",
    "WEAPON_TYPE_DART",
    "WEAPON_TYPE_JAVELIN",
    "WEAPON_TYPE_SLING",
    "WEAPON_TYPE_LONG_BOW",
    "WEAPON_TYPE_SHORT_BOW",
    "WEAPON_TYPE_COMPOSITE_LONGBOW",
    "WEAPON_TYPE_COMPOSITE_LONGBOW_2",
    "WEAPON_TYPE_COMPOSITE_LONGBOW_3",
    "WEAPON_TYPE_COMPOSITE_LONGBOW_4",
    "WEAPON_TYPE_COMPOSITE_LONGBOW_5",
    "WEAPON_TYPE_COMPOSITE_SHORTBOW",
    "WEAPON_TYPE_COMPOSITE_SHORTBOW_2",
    "WEAPON_TYPE_COMPOSITE_SHORTBOW_3",
    "WEAPON_TYPE_COMPOSITE_SHORTBOW_4",
    "WEAPON_TYPE_COMPOSITE_SHORTBOW_5",
    "WEAPON_TYPE_HAND_CROSSBOW",
    "WEAPON_TYPE_HEAVY_REP_XBOW",
    "WEAPON_TYPE_LIGHT_REP_XBOW",
})

SOURCE_ITEM_TYPE_WEAPON = 5
# ITEM_TWOHANDS in the source extra-flag word.
SOURCE_TWO_HANDED_BIT = 1 << 22

# Source verb groups, decoded from the switch on value[3] at
# EXAMPLE/RealmsOfLuminari/src/combat.c:854. Verb 0 falls in the whip group
# there even though the source loader never assigns it deliberately.
SOURCE_VERB_WHIP = frozenset({0, 1, 2})
SOURCE_VERB_SLASH = frozenset({3})
SOURCE_VERB_CRUSH = frozenset({4, 5, 6})
SOURCE_VERB_BLUDGEON = frozenset({7})
SOURCE_VERB_CLAW = frozenset({8, 9})
SOURCE_VERB_BITE = frozenset({10})
SOURCE_VERB_PIERCE = frozenset({11})

OVERRIDES_PATH = Path(__file__).with_name("rol_weapon_overrides.json")

_SOURCE_COLOR = re.compile(r"&\+[A-Za-z]|&[Nn]")
_NON_WORD = re.compile(r"[^a-z0-9]+")


@dataclass(frozen=True)
class WeaponContext:
  """The source signals weapon inference is allowed to read."""

  vnum: int
  text: str
  two_handed: bool
  verb: int
  weight: int
  ndice: int
  sdice: int

  @property
  def average_damage(self) -> float:
    return self.ndice * (self.sdice + 1) / 2.0


@dataclass(frozen=True)
class WeaponInference:
  """A resolved weapon type plus the trail that produced it."""

  weapon_type: int
  name: str
  tier: str
  rule: str

  @property
  def diagnostic(self) -> str:
    return (
        f"inferred weapon type {self.weapon_type} ({self.name}) "
        f"from {self.tier} rule '{self.rule}'"
    )


def normalize_text(value: str | None) -> str:
  """Fold source color codes and punctuation into space-delimited lowercase."""

  text = _SOURCE_COLOR.sub(" ", value or "").lower()
  return f" {_NON_WORD.sub(' ', text).strip()} "


def _phrase(*phrases: str) -> Callable[[str], bool]:
  needles = tuple(f" {phrase} " for phrase in phrases)
  return lambda text: any(needle in text for needle in needles)


# Ordered keyword rules. The first match wins, so compound and specific names
# must precede the general nouns they contain. Each entry is
# (rule name, text predicate, weapon type or resolver).
_Resolver = Callable[[WeaponContext], int]
_RULES: list[tuple[str, Callable[[str], bool], int | _Resolver]] = [
    # Blades -- specific patterns first.
    ("two-bladed sword", _phrase(
        "two bladed sword", "2 bladed sword", "double bladed sword",
        "twin bladed sword", "doublesword", "double sword"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_2_BLADED_SWORD"]),
    ("bastard sword", _phrase(
        "bastard sword", "bastardsword", "hand and a half", "hand and a half sword"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_BASTARD_SWORD"]),
    ("great sword", _phrase(
        "greatsword", "great sword", "claymore", "zweihander", "flamberge",
        "espadon", "great blade", "greatblade"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_SWORD"]),
    ("short sword", _phrase(
        "short sword", "shortsword", "gladius", "wakizashi", "drusus", "xiphos",
        "smallsword", "small sword"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_SHORT_SWORD"]),
    ("rapier", _phrase("rapier", "foil", "epee", "estoc"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_RAPIER"]),
    ("falchion", _phrase("falchion"), WEAPON_TYPE_IDS["WEAPON_TYPE_FALCHION"]),
    ("scimitar", _phrase(
        "scimitar", "cutlass", "saber", "sabre", "tulwar", "talwar", "shamshir",
        "falcata"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_FALCHION"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_SCIMITAR"]),
    ("khopesh", _phrase("khopesh"), WEAPON_TYPE_IDS["WEAPON_TYPE_KHOPESH"]),
    ("kukri", _phrase("kukri", "khukuri"), WEAPON_TYPE_IDS["WEAPON_TYPE_KUKRI"]),
    ("athame", _phrase("athame"), WEAPON_TYPE_IDS["WEAPON_TYPE_ATHAME"]),
    ("sickle", _phrase("sickle"), WEAPON_TYPE_IDS["WEAPON_TYPE_SICKLE"]),
    ("scythe", _phrase("scythe"), WEAPON_TYPE_IDS["WEAPON_TYPE_SCYTHE"]),
    ("kama", _phrase("kama"), WEAPON_TYPE_IDS["WEAPON_TYPE_KAMA"]),
    ("dagger", _phrase(
        "dagger", "dirk", "stiletto", "poniard", "main gauche", "bodkin", "tanto",
        "kris", "misericorde", "kindjal"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DAGGER"]),
    ("knife", _phrase("knife", "knives", "scalpel", "shiv"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_KNIFE"]),
    ("machete", _phrase("machete", "cleaver"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_SHORT_SWORD"]
     if ctx.verb in SOURCE_VERB_SLASH else WEAPON_TYPE_IDS["WEAPON_TYPE_KNIFE"]),
    ("katana", _phrase("katana", "nodachi", "no dachi", "daito"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_SWORD"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_BASTARD_SWORD"]),
    ("long sword", _phrase(
        "longsword", "long sword", "broadsword", "broad sword", "bladesword",
        "spatha", "arming sword"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_SWORD"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_LONG_SWORD"]),
    ("generic sword", _phrase(
        "sword", "swords", "blade", "blades", "mageblade", "moonblade"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_SWORD"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_LONG_SWORD"]),

    # Axes and polearms.
    ("dwarven urgosh", _phrase("urgosh", "dwarven urgosh"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DWARVEN_URGOSH"]),
    ("dwarven war axe", _phrase(
        "dwarven war axe", "dwarven waraxe", "dwarven axe", "dwarven battle axe"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DWARVEN_WAR_AXE"]),
    ("double axe", _phrase(
        "double axe", "doubleaxe", "double bladed axe", "twin axe", "twin bladed axe"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DOUBLE_AXE"]),
    ("throwing axe", _phrase("throwing axe", "throwing axes"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_THROWING_AXE"]),
    ("hand axe", _phrase(
        "hand axe", "handaxe", "hatchet", "tomahawk", "francisca"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_HAND_AXE"]),
    ("great axe", _phrase(
        "great axe", "greataxe", "greater axe", "huge axe", "massive axe"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_AXE"]),
    ("pick", _phrase("pick", "pickaxe", "pick axe", "mattock"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_HEAVY_PICK"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_LIGHT_PICK"]),
    ("halberd", _phrase(
        "halberd", "poleaxe", "pole axe", "bardiche", "voulge", "lucerne",
        "lochaber", "gythka", "bec de corbin"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_HALBERD"]),
    ("glaive", _phrase("glaive", "naginata", "fauchard"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_GLAIVE"]),
    ("guisarme", _phrase("guisarme", "bill hook", "billhook"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_GUISARME"]),
    ("ranseur", _phrase("ranseur", "partisan", "spetum"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_RANSEUR"]),
    ("generic polearm", _phrase("polearm", "pole arm", "pole weapon"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_HALBERD"]),
    ("footmans lance", _phrase("footman s lance", "footmans lance"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_FOOTMANS_LANCE"]),
    ("lance", _phrase("lance"), WEAPON_TYPE_IDS["WEAPON_TYPE_LANCE"]),
    ("trident", _phrase("trident", "fork", "pitchfork"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_TRIDENT"]),
    ("javelin", _phrase("javelin", "harpoon"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_SHORTSPEAR"]),
    ("longspear", _phrase("longspear", "long spear", "pike", "lugged spear"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_LONGSPEAR"]),
    ("shortspear", _phrase("shortspear", "short spear", "half spear"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_SHORTSPEAR"]),
    ("spear", _phrase("spear", "spears", "lance head", "assegai"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_LONGSPEAR"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_SPEAR"]),
    ("battle axe", _phrase("battleaxe", "battle axe", "war axe", "waraxe"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_AXE"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_BATTLE_AXE"]),
    ("generic axe", _phrase("axe", "axes", "adze"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_AXE"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_BATTLE_AXE"]),

    # Bludgeons, flails, hammers, staves.
    ("quarterstaff", _phrase(
        "quarterstaff", "quarter staff", "staff", "stave", "staves", "bo staff",
        "crozier", "cane", "walking stick"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_QUARTERSTAFF"]),
    ("morningstar", _phrase("morningstar", "morning star"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_MORNINGSTAR"]),
    ("dire flail", _phrase("dire flail", "double flail"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DIRE_FLAIL"]),
    ("flail", _phrase("flail", "flindbar", "nine tails", "cat o nine"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_HEAVY_FLAIL"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_FLAIL"]),
    ("hooked hammer", _phrase("hooked hammer", "gnome hooked hammer"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_HOOKED_HAMMER"]),
    ("light hammer", _phrase("light hammer", "throwing hammer", "tack hammer"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_LIGHT_HAMMER"]),
    ("warmaul", _phrase(
        "warmaul", "war maul", "maul", "sledgehammer", "sledge hammer", "sledge",
        "great hammer", "greathammer"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_WARMAUL"]),
    ("warhammer", _phrase("warhammer", "war hammer", "hammer", "mallet"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_WARMAUL"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_WARHAMMER"]),
    ("mace", _phrase("mace", "scepter", "sceptre", "flanged mace"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_HEAVY_MACE"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_LIGHT_MACE"]),
    ("sap", _phrase("sap", "blackjack", "cosh"), WEAPON_TYPE_IDS["WEAPON_TYPE_SAP"]),
    ("club", _phrase(
        "club", "cudgel", "crudgel", "baton", "nightstick", "truncheon", "shillelagh",
        "bludgeon", "branch", "stick", "rolling pin"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_CLUB"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_CLUB"]),

    # Exotic, chains, picks, monk weapons.
    ("whip", _phrase(
        "whip", "bullwhip", "scourge", "lash", "flog", "quirt", "garrote",
        "garrotte", "garroting", "ligature"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_WHIP"]),
    ("spiked chain", _phrase(
        "spiked chain", "chain", "chains", "kusari", "manriki", "meteor hammer"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_SPIKED_CHAIN"]),
    ("nunchaku", _phrase("nunchaku", "nunchucks", "nunchuck", "nunchakus"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_NUNCHAKU"]),
    ("siangham", _phrase("siangham"), WEAPON_TYPE_IDS["WEAPON_TYPE_SIANGHAM"]),
    ("sai", _phrase("sai", "jitte"), WEAPON_TYPE_IDS["WEAPON_TYPE_SAI"]),
    ("shuriken", _phrase("shuriken", "throwing star", "chakram"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_SHURIKEN"]),
    ("net", _phrase("net", "mesh net"), WEAPON_TYPE_IDS["WEAPON_TYPE_NET"]),
    ("bola", _phrase("bola", "bolas"), WEAPON_TYPE_IDS["WEAPON_TYPE_BOLA"]),
    ("hoopak", _phrase("hoopak"), WEAPON_TYPE_IDS["WEAPON_TYPE_HOOPAK"]),

    # Natural and unarmed.
    ("natural weapon", _phrase(
        "claw", "claws", "talon", "talons", "bite", "fang", "fangs", "fist",
        "fists", "gauntlet", "gauntlets", "knuckle", "knuckles", "cestus",
        "tentacle", "tentacles", "mandible", "mandibles", "horn", "horns",
        "stinger", "tail", "pincer", "pincers", "pincher", "pinchers", "hoof", "hooves", "tusk", "tusks"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_UNARMED"]),

    # Weak modifiers. These tokens routinely decorate some other weapon ("a
    # razor edge battleaxe", "a razor-sharp hunting spear"), so they may only be
    # consulted once every strong noun rule above has declined the record.
    ("weak razor", _phrase(
        "razor", "razorblade", "razors", "carver", "skinner"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_KNIFE"]),
    ("weak hook", _phrase("hook", "hooks", "gaff"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_KNIFE"]),
    ("weak rod", _phrase("rod", "rods", "wand", "baton"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_CLUB"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_CLUB"]),
    ("weak shard", _phrase(
        "shard", "shards", "splinter", "spike", "spikes", "sliver", "thorn"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DAGGER"]),
    ("weak bone", _phrase(
        "bone", "bones", "shinbone", "thighbone", "femur", "rib", "ribs",
        "spine", "skull", "jawbone", "antler", "antlers"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_CLUB"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_CLUB"]),
    ("weak tooth", _phrase("tooth", "teeth", "sting", "quill", "spur"),
     WEAPON_TYPE_IDS["WEAPON_TYPE_DAGGER"]),
    ("weak stone", _phrase(
        "rock", "rocks", "stone", "stones", "boulder", "lump", "brick"),
     lambda ctx: WEAPON_TYPE_IDS["WEAPON_TYPE_GREAT_CLUB"] if ctx.two_handed
     else WEAPON_TYPE_IDS["WEAPON_TYPE_CLUB"]),
]

# Tier 3. Weight and damage are deliberately absent: the source weights in this
# corpus are frequently non-physical, and read_object() clamps ndice at 2 and
# sdice at 12 (src/olc/oasis.h, src/db.c) so pre-clamp damage does not survive
# into the target anyway. Handedness is the only mechanical signal worth
# branching on.
_FALLBACK: list[tuple[str, frozenset[int], str, str]] = [
    ("slash", SOURCE_VERB_SLASH, "WEAPON_TYPE_GREAT_SWORD", "WEAPON_TYPE_LONG_SWORD"),
    ("pierce", SOURCE_VERB_PIERCE, "WEAPON_TYPE_LONGSPEAR", "WEAPON_TYPE_SHORT_SWORD"),
    ("crush", SOURCE_VERB_CRUSH, "WEAPON_TYPE_GREAT_CLUB", "WEAPON_TYPE_HEAVY_MACE"),
    ("bludgeon", SOURCE_VERB_BLUDGEON, "WEAPON_TYPE_GREAT_CLUB", "WEAPON_TYPE_CLUB"),
    ("whip", SOURCE_VERB_WHIP, "WEAPON_TYPE_WHIP", "WEAPON_TYPE_WHIP"),
    ("claw", SOURCE_VERB_CLAW, "WEAPON_TYPE_UNARMED", "WEAPON_TYPE_UNARMED"),
    ("bite", SOURCE_VERB_BITE, "WEAPON_TYPE_UNARMED", "WEAPON_TYPE_UNARMED"),
]

# Applied when value[3] holds a verb the source runtime itself would reject.
_FALLBACK_DEFAULT = "WEAPON_TYPE_CLUB"

_overrides_cache: dict[int, dict[str, Any]] | None = None


def load_overrides(path: Path | None = None) -> dict[int, dict[str, Any]]:
  """Load and validate the curated per-vnum override catalog."""

  global _overrides_cache
  if path is None and _overrides_cache is not None:
    return _overrides_cache
  target = path or OVERRIDES_PATH
  if not target.exists():
    overrides: dict[int, dict[str, Any]] = {}
  else:
    payload = json.loads(target.read_text(encoding="ascii"))
    entries = payload.get("overrides", {})
    overrides = {}
    for key, entry in entries.items():
      vnum = int(key)
      name = entry["weapon_type"]
      if name not in WEAPON_TYPE_IDS:
        raise ValueError(f"override for source object {vnum} names unknown weapon type {name}")
      if WEAPON_TYPE_IDS[name] == 0:
        raise ValueError(f"override for source object {vnum} resolves to WEAPON_TYPE_UNDEFINED")
      if name in RANGED_WEAPON_TYPES:
        raise ValueError(
            f"override for source object {vnum} names ranged weapon type {name}; "
            "source melee weapons must not become ranged"
        )
      overrides[vnum] = {
          "weapon_type": WEAPON_TYPE_IDS[name],
          "name": name,
          "rationale": entry.get("rationale", ""),
      }
  if path is None:
    _overrides_cache = overrides
  return overrides


def weapon_context(record: Any) -> WeaponContext:
  """Build the inference context from an unmodified source object record."""

  values = list(record.values.get("values", []))
  values = (values + [0] * 4)[:4]
  flags = list(record.values.get("flags", []))
  extra = flags[1] if len(flags) > 1 else 0
  economy = list(record.values.get("economy", []))
  strings = record.values.get("strings", {})
  text = normalize_text(
      f"{strings.get('aliases', '')} {strings.get('short_description', '')}"
  )
  return WeaponContext(
      vnum=record.vnum,
      text=text,
      two_handed=bool(extra & SOURCE_TWO_HANDED_BIT),
      verb=values[3],
      weight=economy[0] if economy else 0,
      ndice=values[1],
      sdice=values[2],
  )


def classify(context: WeaponContext, overrides: dict[int, dict[str, Any]] | None = None) -> WeaponInference:
  """Resolve one source weapon to a defined target WEAPON_TYPE_*."""

  catalog = load_overrides() if overrides is None else overrides
  override = catalog.get(context.vnum)
  if override is not None:
    return WeaponInference(override["weapon_type"], override["name"], "override", "catalog")

  for rule, matches, resolver in _RULES:
    if not matches(context.text):
      continue
    weapon_type = resolver(context) if callable(resolver) else resolver
    return WeaponInference(weapon_type, WEAPON_TYPE_NAMES[weapon_type], "keyword", rule)

  for rule, verbs, two_handed_name, one_handed_name in _FALLBACK:
    if context.verb not in verbs:
      continue
    name = two_handed_name if context.two_handed else one_handed_name
    return WeaponInference(WEAPON_TYPE_IDS[name], name, "fallback", rule)

  return WeaponInference(
      WEAPON_TYPE_IDS[_FALLBACK_DEFAULT], _FALLBACK_DEFAULT, "fallback", "unmapped verb"
  )


def infer_weapon_type(record: Any) -> WeaponInference:
  """Infer the target weapon type for one source ITEM_WEAPON record."""

  inference = classify(weapon_context(record))
  if not 1 <= inference.weapon_type < NUM_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} resolved to out-of-range weapon type "
        f"{inference.weapon_type}"
    )
  if inference.name in RANGED_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} resolved to ranged weapon type {inference.name}"
    )
  return inference


def audit(records: Iterable[Any]) -> dict[str, Any]:
  """Summarize inference over a corpus for conversion reporting."""

  rows: list[dict[str, Any]] = []
  for record in records:
    if record.kind != "obj" or record.values.get("item_type") != SOURCE_ITEM_TYPE_WEAPON:
      continue
    context = weapon_context(record)
    inference = classify(context)
    rows.append(
        {
            "vnum": record.vnum,
            "path": record.path,
            "identity": record.identity,
            "weapon_type": inference.weapon_type,
            "weapon_type_name": inference.name,
            "tier": inference.tier,
            "rule": inference.rule,
            "two_handed": context.two_handed,
            "verb": context.verb,
        }
    )
  tiers: dict[str, int] = {}
  rules: dict[str, int] = {}
  types: dict[str, int] = {}
  for row in rows:
    tiers[row["tier"]] = tiers.get(row["tier"], 0) + 1
    rules[f"{row['tier']}:{row['rule']}"] = rules.get(f"{row['tier']}:{row['rule']}", 0) + 1
    types[row["weapon_type_name"]] = types.get(row["weapon_type_name"], 0) + 1
  return {
      "weapons": len(rows),
      "undefined": sum(1 for row in rows if row["weapon_type"] == 0),
      "tiers": dict(sorted(tiers.items())),
      "rules": dict(sorted(rules.items(), key=lambda item: (-item[1], item[0]))),
      "weapon_types": dict(sorted(types.items(), key=lambda item: (-item[1], item[0]))),
      "records": rows,
  }
