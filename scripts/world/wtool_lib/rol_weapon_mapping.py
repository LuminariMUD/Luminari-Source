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

# A ranged WEAPON_TYPE_* may only be emitted when has_ammo_in_pouch()
# (src/combat/assign_wpn_armor.c:566) carries a case pairing it with an ammo
# type. Every other ranged type is an item that can neither fire -- no ammo
# pairs with it -- nor melee, because do_hit (src/combat/act.offensive.c:4667)
# refuses to start melee for a wielded WEAPON_FLAG_RANGED weapon.
# test_rol_weapon_mapping.py reparses that switch and fails if the set drifts.
AMMO_PAIRED_WEAPON_TYPES = frozenset({
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
    "WEAPON_TYPE_HEAVY_CROSSBOW",
    "WEAPON_TYPE_LIGHT_CROSSBOW",
    "WEAPON_TYPE_SLING",
    "WEAPON_TYPE_DART",
})

# Mirrors the AMMO_TYPE_* block in src/structs.h.
AMMO_TYPE_NAMES: dict[int, str] = {
    0: "AMMO_TYPE_UNDEFINED",
    1: "AMMO_TYPE_ARROW",
    2: "AMMO_TYPE_BOLT",
    3: "AMMO_TYPE_STONE",
    4: "AMMO_TYPE_DART",
}
AMMO_TYPE_IDS: dict[str, int] = {name: value for value, name in AMMO_TYPE_NAMES.items()}
NUM_AMMO_TYPES = 5

SOURCE_ITEM_TYPE_WEAPON = 5
SOURCE_ITEM_TYPE_FIREWEAPON = 6
SOURCE_ITEM_TYPE_MISSILE = 7
SOURCE_ITEM_TYPE_QUIVER = 30

TARGET_ITEM_WEAPON = 5
TARGET_ITEM_MISSILE = 14
TARGET_ITEM_CONTAINER = 15
TARGET_ITEM_AMMO_POUCH = 36

# RoL quivers declare their kind in value[3]
# (EXAMPLE/RealmsOfLuminari/src/missile.c:53). An archery quiver holds
# ITEM_MISSILE records and becomes an ammo pouch; a throwing quiver holds
# thrown ITEM_FIREWEAPON records, which convert to ITEM_WEAPON, and an ammo
# pouch may hold nothing but ITEM_MISSILE -- both has_ammo_in_pouch() and
# do_put (src/obj/act.item.c:2022) enforce that -- so it becomes a container.
SOURCE_QUIVER_ARCHERY = 1
SOURCE_QUIVER_THROWING = 2

# rangeweapons[] and missiles[], EXAMPLE/RealmsOfLuminari/src/missile.c:347 and
# :402. One shared one-based numbering, read from fireweapon value[7] and
# missile value[3].
SOURCE_RANGE_TYPE_NAMES: dict[int, str] = {
    1: "Short Bow",
    2: "Long Bow",
    3: "Elven Short Bow",
    4: "Elven Long Bow",
    5: "Hand Crossbow",
    6: "Light Crossbow",
    7: "Heavy Crossbow",
    8: "Special Missile Weapon",
    9: "Throwing Dagger",
    10: "Dart",
    11: "Throwing Hammer",
    12: "Throwing Handaxe",
    13: "Sling",
    14: "Javelin",
    15: "Spear",
    16: "Blowgun",
    17: "Special Thrown Weapon",
    18: "Common Object",
    19: "Scorpion Ballista",
    20: "Catapult",
    21: "Spell Missile",
}

# The "special" and "common object" buckets carry no weapon identity of their
# own, so they fall through to the name tier.
SOURCE_UNSPECIFIC_RANGE_TYPES = frozenset({8, 17, 18})

# Source ITEM_FIREWEAPON value[7] to target weapon type. Elven bows are RoL's
# better tier (range 100/200 against 65/120), which is what the composite types
# express; the _2.._5 variants encode a strength rating no source record
# carries. Source thrown weapons retain their native target weapon identity and
# become melee-usable ITEM_WEAPON records until the target gains a throwing
# command.
SOURCE_RANGE_WEAPON_TYPES: dict[int, str] = {
    1: "WEAPON_TYPE_SHORT_BOW",
    2: "WEAPON_TYPE_LONG_BOW",
    3: "WEAPON_TYPE_COMPOSITE_SHORTBOW",
    4: "WEAPON_TYPE_COMPOSITE_LONGBOW",
    5: "WEAPON_TYPE_HAND_CROSSBOW",
    6: "WEAPON_TYPE_LIGHT_CROSSBOW",
    7: "WEAPON_TYPE_HEAVY_CROSSBOW",
    9: "WEAPON_TYPE_DAGGER",
    10: "WEAPON_TYPE_DART",
    11: "WEAPON_TYPE_LIGHT_HAMMER",
    12: "WEAPON_TYPE_THROWING_AXE",
    13: "WEAPON_TYPE_SLING",
    14: "WEAPON_TYPE_JAVELIN",
    15: "WEAPON_TYPE_SPEAR",
    16: "WEAPON_TYPE_DART",
}

# Source ITEM_MISSILE value[3] to target AMMO_TYPE_*.
SOURCE_MISSILE_AMMO_TYPES: dict[int, str] = {
    1: "AMMO_TYPE_ARROW",
    2: "AMMO_TYPE_ARROW",
    3: "AMMO_TYPE_ARROW",
    4: "AMMO_TYPE_ARROW",
    5: "AMMO_TYPE_BOLT",
    6: "AMMO_TYPE_BOLT",
    7: "AMMO_TYPE_BOLT",
    10: "AMMO_TYPE_DART",
    13: "AMMO_TYPE_STONE",
    16: "AMMO_TYPE_DART",
}

# Source ITEM_MISSILE types the target has no ammunition for. The target has no
# throwing command at all, so these records are retyped to ITEM_WEAPON and
# become the melee weapon they are.
SOURCE_MISSILE_WEAPON_TYPES: dict[int, str] = {
    9: "WEAPON_TYPE_DAGGER",
    11: "WEAPON_TYPE_LIGHT_HAMMER",
    12: "WEAPON_TYPE_THROWING_AXE",
    14: "WEAPON_TYPE_JAVELIN",
    15: "WEAPON_TYPE_SPEAR",
}

# Source missile durability runs 1..10 with 10 best; target missile value[2] is
# a break-probability percent read at src/combat/fight.c:12210, where higher is
# more fragile. The scale is inverted, not merely rescaled.
SOURCE_MISSILE_MIN_DURABILITY = 1
SOURCE_MISSILE_MAX_DURABILITY = 10
MISSILE_BREAK_PROBABILITY_BASE = SOURCE_MISSILE_MAX_DURABILITY + 1

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
     WEAPON_TYPE_IDS["WEAPON_TYPE_JAVELIN"]),
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

_catalog_cache: dict[str, dict[int, dict[str, Any]]] | None = None


def _melee_override(vnum: int, name: str) -> dict[str, Any]:
  if name not in WEAPON_TYPE_IDS:
    raise ValueError(f"override for source object {vnum} names unknown weapon type {name}")
  if WEAPON_TYPE_IDS[name] == 0:
    raise ValueError(f"override for source object {vnum} resolves to WEAPON_TYPE_UNDEFINED")
  if name in RANGED_WEAPON_TYPES:
    raise ValueError(
        f"override for source object {vnum} names ranged weapon type {name}; "
        "source melee weapons must not become ranged"
    )
  return {"weapon_type": WEAPON_TYPE_IDS[name], "name": name}


def _ranged_override(vnum: int, name: str) -> dict[str, Any]:
  if name not in WEAPON_TYPE_IDS:
    raise ValueError(f"override for source object {vnum} names unknown weapon type {name}")
  if WEAPON_TYPE_IDS[name] == 0:
    raise ValueError(f"override for source object {vnum} resolves to WEAPON_TYPE_UNDEFINED")
  if name in RANGED_WEAPON_TYPES and name not in AMMO_PAIRED_WEAPON_TYPES:
    raise ValueError(
        f"override for source object {vnum} names ranged weapon type {name}, which "
        "has no case in has_ammo_in_pouch() and could never fire"
    )
  return {"weapon_type": WEAPON_TYPE_IDS[name], "name": name}


def _ammunition_override(vnum: int, name: str) -> dict[str, Any]:
  if name in AMMO_TYPE_IDS:
    if AMMO_TYPE_IDS[name] == 0:
      raise ValueError(f"override for source object {vnum} resolves to AMMO_TYPE_UNDEFINED")
    return {
        "item_type": TARGET_ITEM_MISSILE,
        "ammo_type": AMMO_TYPE_IDS[name],
        "weapon_type": 0,
        "name": name,
    }
  resolved = _melee_override(vnum, name)
  return {
      "item_type": TARGET_ITEM_WEAPON,
      "ammo_type": 0,
      "weapon_type": resolved["weapon_type"],
      "name": resolved["name"],
  }


_CATALOG_SECTIONS: dict[str, Callable[[int, str], dict[str, Any]]] = {
    "overrides": _melee_override,
    "ranged_overrides": _ranged_override,
    "ammunition_overrides": _ammunition_override,
}

# The section key the entries in each catalog section name their target with.
_CATALOG_VALUE_KEYS: dict[str, str] = {
    "overrides": "weapon_type",
    "ranged_overrides": "weapon_type",
    "ammunition_overrides": "target",
}


def load_catalog(path: Path | None = None) -> dict[str, dict[int, dict[str, Any]]]:
  """Load and validate every curated per-vnum override section."""

  global _catalog_cache
  if path is None and _catalog_cache is not None:
    return _catalog_cache
  target = path or OVERRIDES_PATH
  payload = json.loads(target.read_text(encoding="ascii")) if target.exists() else {}
  catalog: dict[str, dict[int, dict[str, Any]]] = {}
  for section, validate in _CATALOG_SECTIONS.items():
    entries = payload.get(section, {})
    resolved: dict[int, dict[str, Any]] = {}
    for key, entry in entries.items():
      vnum = int(key)
      record = validate(vnum, entry[_CATALOG_VALUE_KEYS[section]])
      record["rationale"] = entry.get("rationale", "")
      resolved[vnum] = record
    catalog[section] = resolved
  if path is None:
    _catalog_cache = catalog
  return catalog


def load_overrides(path: Path | None = None) -> dict[int, dict[str, Any]]:
  """Load the curated per-vnum override catalog for source melee weapons."""

  return load_catalog(path)["overrides"]


def load_ranged_overrides(path: Path | None = None) -> dict[int, dict[str, Any]]:
  """Load the curated per-vnum override catalog for source ranged weapons."""

  return load_catalog(path)["ranged_overrides"]


def load_ammunition_overrides(path: Path | None = None) -> dict[int, dict[str, Any]]:
  """Load the curated per-vnum override catalog for source ammunition."""

  return load_catalog(path)["ammunition_overrides"]


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


# Name rules for the source "special"/"common object" buckets, which declare no
# usable type. Ordered; the first match wins. A launcher rule may only name an
# ammo-paired type, and an ammunition rule may only name an AMMO_TYPE_*.
_LAUNCHER_NAME_RULES: list[tuple[str, Callable[[str], bool], str]] = [
    ("crossbow", _phrase(
        "crossbow", "cross bow", "bolt thrower", "arbalest"), "WEAPON_TYPE_LIGHT_CROSSBOW"),
    ("short bow", _phrase("short bow", "shortbow"), "WEAPON_TYPE_SHORT_BOW"),
    ("composite bow", _phrase(
        "composite bow", "composite longbow", "elven longbow", "elven long bow"),
     "WEAPON_TYPE_COMPOSITE_LONGBOW"),
    ("bow", _phrase("bow", "longbow", "long bow", "warbow"), "WEAPON_TYPE_LONG_BOW"),
    ("sling", _phrase("sling", "slingshot"), "WEAPON_TYPE_SLING"),
    ("dart launcher", _phrase("blowgun", "blow gun", "dart gun", "dartgun"),
     "WEAPON_TYPE_DART"),
]

_AMMUNITION_NAME_RULES: list[tuple[str, Callable[[str], bool], str]] = [
    ("arrow", _phrase("arrow", "arrows", "shaft", "shafts", "flight"), "AMMO_TYPE_ARROW"),
    ("bolt", _phrase("bolt", "bolts", "quarrel", "quarrels"), "AMMO_TYPE_BOLT"),
    ("sling stone", _phrase(
        "sling stone", "slingstone", "sling bullet", "pebble", "pebbles"), "AMMO_TYPE_STONE"),
    ("dart", _phrase("dart", "darts", "needle", "needles"), "AMMO_TYPE_DART"),
]


@dataclass(frozen=True)
class RangedContext:
  """The source signals ranged inference is allowed to read."""

  vnum: int
  text: str
  source_item_type: int
  declared_type: int
  values: tuple[int, ...]

  @property
  def declared_name(self) -> str:
    return SOURCE_RANGE_TYPE_NAMES.get(self.declared_type, "unknown")


@dataclass(frozen=True)
class AmmoInference:
  """A resolved ammunition identity plus the trail that produced it.

  ``item_type`` is the target item type the record must be emitted as. Source
  ammunition the target has no ammo type for is retyped to ``ITEM_WEAPON``,
  because the target has no throwing command and a thrown record is only ever
  swung there.
  """

  item_type: int
  ammo_type: int
  weapon_type: int
  name: str
  tier: str
  rule: str

  @property
  def diagnostic(self) -> str:
    if self.item_type == TARGET_ITEM_WEAPON:
      return (
          f"retyped source ammunition to ITEM_WEAPON on weapon type "
          f"{self.weapon_type} ({self.name}) from {self.tier} rule '{self.rule}'"
      )
    return (
        f"inferred ammo type {self.ammo_type} ({self.name}) "
        f"from {self.tier} rule '{self.rule}'"
    )


def ranged_context(record: Any) -> RangedContext:
  """Build the ranged inference context from an unmodified source record."""

  values = list(record.values.get("values", []))
  values = (values + [0] * 8)[:8]
  source_item_type = int(record.values.get("item_type") or 0)
  declared = values[7] if source_item_type == SOURCE_ITEM_TYPE_FIREWEAPON else values[3]
  strings = record.values.get("strings", {})
  text = normalize_text(
      f"{strings.get('aliases', '')} {strings.get('short_description', '')}"
  )
  return RangedContext(
      vnum=record.vnum,
      text=text,
      source_item_type=source_item_type,
      declared_type=declared,
      values=tuple(values),
  )


class _MeleeFallbackRecord:
  """Adapt a RangedContext back into the shape ``weapon_context()`` reads.

  The melee classifier is the last tier for the source "special" buckets, and
  it reads its own signals off a source record. A ranged record carries no
  melee verb and no handedness that means anything, so only the text and the
  vnum are carried across; the melee fallback matrix then resolves on the
  unmapped-verb default rather than on an invented verb.
  """

  def __init__(self, context: RangedContext) -> None:
    self.vnum = context.vnum
    self.kind = "obj"
    self.values = {
        "values": [0, 0, 0, -1],
        "flags": [0, 0, 0],
        "economy": [0],
        "strings": {"aliases": context.text, "short_description": ""},
    }


def classify_ranged_weapon(
    context: RangedContext,
    overrides: dict[int, dict[str, Any]] | None = None,
) -> WeaponInference:
  """Resolve one source ITEM_FIREWEAPON to a defined target WEAPON_TYPE_*.

  The tiers invert relative to melee inference. The source record declares its
  own range-weapon type in ``value[7]``, and that is what the RoL engine used
  for range, rate of fire, and quiver compatibility, so it outranks the name.
  A curated override still wins over both, as it does for melee weapons: it is
  an explicit builder-review decision rather than an inference.
  """

  catalog = load_ranged_overrides() if overrides is None else overrides
  override = catalog.get(context.vnum)
  if override is not None:
    return WeaponInference(override["weapon_type"], override["name"], "override", "catalog")

  declared = SOURCE_RANGE_WEAPON_TYPES.get(context.declared_type)
  if declared is not None:
    return WeaponInference(
        WEAPON_TYPE_IDS[declared],
        declared,
        "declared",
        f"range type {context.declared_type} ({context.declared_name})",
    )

  for rule, matches, name in _LAUNCHER_NAME_RULES:
    if matches(context.text):
      return WeaponInference(WEAPON_TYPE_IDS[name], name, "keyword", rule)

  # Nothing declared and nothing named: RoL's "special"/"common object" buckets
  # are improvised objects as often as they are weapons, so fall through to the
  # melee classifier, which always resolves and never emits a ranged type.
  melee = classify(weapon_context(_MeleeFallbackRecord(context)))
  return WeaponInference(melee.weapon_type, melee.name, "fallback", f"melee {melee.rule}")


def classify_ammunition(
    context: RangedContext,
    overrides: dict[int, dict[str, Any]] | None = None,
) -> AmmoInference:
  """Resolve one source ITEM_MISSILE to an ammo type or a retyped weapon."""

  catalog = load_ammunition_overrides() if overrides is None else overrides
  override = catalog.get(context.vnum)
  if override is not None:
    return AmmoInference(
        override["item_type"],
        override["ammo_type"],
        override["weapon_type"],
        override["name"],
        "override",
        "catalog",
    )

  declared = SOURCE_MISSILE_AMMO_TYPES.get(context.declared_type)
  if declared is not None:
    return AmmoInference(
        TARGET_ITEM_MISSILE,
        AMMO_TYPE_IDS[declared],
        0,
        declared,
        "declared",
        f"missile type {context.declared_type} ({context.declared_name})",
    )

  thrown = SOURCE_MISSILE_WEAPON_TYPES.get(context.declared_type)
  if thrown is not None:
    return AmmoInference(
        TARGET_ITEM_WEAPON,
        0,
        WEAPON_TYPE_IDS[thrown],
        thrown,
        "declared",
        f"missile type {context.declared_type} ({context.declared_name})",
    )

  for rule, matches, name in _AMMUNITION_NAME_RULES:
    if matches(context.text):
      return AmmoInference(
          TARGET_ITEM_MISSILE, AMMO_TYPE_IDS[name], 0, name, "keyword", rule
      )

  melee = classify(weapon_context(_MeleeFallbackRecord(context)))
  return AmmoInference(
      TARGET_ITEM_WEAPON, 0, melee.weapon_type, melee.name, "fallback", f"melee {melee.rule}"
  )


def infer_ranged_weapon_type(record: Any) -> WeaponInference:
  """Infer the target weapon type for one source ITEM_FIREWEAPON record."""

  inference = classify_ranged_weapon(ranged_context(record))
  if not 1 <= inference.weapon_type < NUM_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} resolved to out-of-range weapon type "
        f"{inference.weapon_type}"
    )
  if inference.name in RANGED_WEAPON_TYPES and inference.name not in AMMO_PAIRED_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} resolved to ranged weapon type {inference.name}, "
        "which has no case in has_ammo_in_pouch() and could never fire"
    )
  return inference


def infer_ammunition(record: Any) -> AmmoInference:
  """Infer the target ammunition identity for one source ITEM_MISSILE record."""

  inference = classify_ammunition(ranged_context(record))
  if inference.item_type == TARGET_ITEM_MISSILE:
    if not 1 <= inference.ammo_type < NUM_AMMO_TYPES:
      raise ValueError(
          f"source object {record.vnum} resolved to out-of-range ammo type "
          f"{inference.ammo_type}"
      )
  elif not 1 <= inference.weapon_type < NUM_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} resolved to out-of-range weapon type "
        f"{inference.weapon_type}"
    )
  elif inference.name in RANGED_WEAPON_TYPES:
    raise ValueError(
        f"source object {record.vnum} retyped to ranged weapon type {inference.name}"
    )
  return inference


def missile_break_probability(durability: int) -> int:
  """Restate source missile durability as the target's break-probability percent.

  Source durability runs 1..10 with 10 best; the target breaks the missile when
  ``value[2] >= dice(1, 100)``, so the scale is inverted rather than rescaled.
  """

  clamped = min(
      SOURCE_MISSILE_MAX_DURABILITY, max(SOURCE_MISSILE_MIN_DURABILITY, durability)
  )
  return MISSILE_BREAK_PROBABILITY_BASE - clamped


def audit(records: Iterable[Any]) -> dict[str, Any]:
  """Summarize inference over a corpus for conversion reporting.

  One report covers all three source families -- melee weapons, ranged
  weapons, and ammunition -- so the builder review packet is one document
  rather than three. ``name_disagreements`` collects the records whose text
  argues with the type their builder declared; the declared type wins
  mechanically, but each one is a builder judgement the review should see.
  """

  rows: list[dict[str, Any]] = []
  disagreements: list[dict[str, Any]] = []
  for record in records:
    if record.kind != "obj":
      continue
    source_type = record.values.get("item_type")
    if source_type == SOURCE_ITEM_TYPE_WEAPON:
      context = weapon_context(record)
      inference = classify(context)
      rows.append(
          {
              "vnum": record.vnum,
              "path": record.path,
              "identity": record.identity,
              "source_item_type": source_type,
              "target_item_type": TARGET_ITEM_WEAPON,
              "weapon_type": inference.weapon_type,
              "weapon_type_name": inference.name,
              "ammo_type": 0,
              "tier": inference.tier,
              "rule": inference.rule,
              "two_handed": context.two_handed,
              "verb": context.verb,
          }
      )
      continue
    if source_type not in {SOURCE_ITEM_TYPE_FIREWEAPON, SOURCE_ITEM_TYPE_MISSILE}:
      continue
    context = ranged_context(record)
    if source_type == SOURCE_ITEM_TYPE_FIREWEAPON:
      inference = classify_ranged_weapon(context)
      row = {
          "target_item_type": TARGET_ITEM_WEAPON,
          "weapon_type": inference.weapon_type,
          "weapon_type_name": inference.name,
          "ammo_type": 0,
          "tier": inference.tier,
          "rule": inference.rule,
      }
      named = _launcher_name_opinion(context)
    else:
      ammo = classify_ammunition(context)
      row = {
          "target_item_type": ammo.item_type,
          "weapon_type": ammo.weapon_type,
          "weapon_type_name": ammo.name,
          "ammo_type": ammo.ammo_type,
          "tier": ammo.tier,
          "rule": ammo.rule,
      }
      named = _ammunition_name_opinion(context)
    row.update(
        {
            "vnum": record.vnum,
            "path": record.path,
            "identity": record.identity,
            "source_item_type": source_type,
            "declared_type": context.declared_type,
            "declared_name": context.declared_name,
            "two_handed": False,
            "verb": 0,
        }
    )
    rows.append(row)
    if named is not None and named != row["weapon_type_name"]:
      disagreements.append(
          {
              "vnum": record.vnum,
              "identity": record.identity,
              "declared_name": context.declared_name,
              "resolved": row["weapon_type_name"],
              "named": named,
          }
      )
  tiers: dict[str, int] = {}
  rules: dict[str, int] = {}
  types: dict[str, int] = {}
  for row in rows:
    tiers[row["tier"]] = tiers.get(row["tier"], 0) + 1
    rules[f"{row['tier']}:{row['rule']}"] = rules.get(f"{row['tier']}:{row['rule']}", 0) + 1
    name = row["weapon_type_name"]
    types[name] = types.get(name, 0) + 1
  return {
      "weapons": sum(
          1 for row in rows if row["source_item_type"] == SOURCE_ITEM_TYPE_WEAPON
      ),
      "ranged_weapons": sum(
          1 for row in rows if row["source_item_type"] == SOURCE_ITEM_TYPE_FIREWEAPON
      ),
      "ammunition": sum(
          1 for row in rows if row["source_item_type"] == SOURCE_ITEM_TYPE_MISSILE
      ),
      "retyped_ammunition": sum(
          1
          for row in rows
          if row["source_item_type"] == SOURCE_ITEM_TYPE_MISSILE
          and row["target_item_type"] == TARGET_ITEM_WEAPON
      ),
      "undefined": sum(
          1
          for row in rows
          if row["target_item_type"] == TARGET_ITEM_WEAPON and row["weapon_type"] == 0
      ),
      "undefined_ammo": sum(
          1
          for row in rows
          if row["target_item_type"] == TARGET_ITEM_MISSILE and row["ammo_type"] == 0
      ),
      "tiers": dict(sorted(tiers.items())),
      "rules": dict(sorted(rules.items(), key=lambda item: (-item[1], item[0]))),
      "weapon_types": dict(sorted(types.items(), key=lambda item: (-item[1], item[0]))),
      "name_disagreements": disagreements,
      "records": rows,
  }


def _launcher_name_opinion(context: RangedContext) -> str | None:
  """What the launcher name rules would say, ignoring the declared type."""

  for _rule, matches, name in _LAUNCHER_NAME_RULES:
    if matches(context.text):
      return name
  return None


def _ammunition_name_opinion(context: RangedContext) -> str | None:
  """What the ammunition name rules would say, ignoring the declared type."""

  for _rule, matches, name in _AMMUNITION_NAME_RULES:
    if matches(context.text):
      return name
  return None
