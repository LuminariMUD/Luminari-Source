"""Semantic transforms from Realms of Luminari records to Luminari data."""

from __future__ import annotations

from dataclasses import dataclass, field
from functools import lru_cache
import re
import textwrap
from typing import Any, Callable

from .flags import encode_bits
from .rol_mob_calculator import (
    MobileCalculatorClient,
    calculation_to_dict,
    default_mobile_calculator,
)
from .rol_mobile_identity import (
    load_mobile_conversion_policy,
    select_mobile_conversion,
)
from .rol_source import RolRecord, normalize_identity
from .rol_weapon_mapping import (
    SOURCE_ITEM_TYPE_FIREWEAPON,
    SOURCE_ITEM_TYPE_MISSILE,
    SOURCE_ITEM_TYPE_QUIVER,
    SOURCE_ITEM_TYPE_WEAPON,
    SOURCE_QUIVER_THROWING,
    TARGET_ITEM_AMMO_POUCH,
    TARGET_ITEM_CONTAINER,
    TARGET_ITEM_MISSILE,
    TARGET_ITEM_WEAPON,
    WeaponInference,
    infer_ammunition,
    infer_ranged_weapon_type,
    infer_weapon_type,
    missile_break_probability,
)
from .rol_weapon_table import weapon_table


_TARGET_MAGIC_ITEM_TYPES = frozenset({2, 3, 4, 10})
_TARGET_MAX_LEVEL = 34
_TARGET_MAX_OBJECT_SPELL_LEVEL = _TARGET_MAX_LEVEL
_TARGET_MAX_LIQUID = 22
SOURCE_ITEM_TYPE_INSTRUMENT = 32
TARGET_ITEM_INSTRUMENT = 38
_TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION = 30
_TARGET_INSTRUMENT_MAX_EFFECTIVENESS = 10
_TARGET_INSTRUMENT_DEFAULT_BREAKABILITY = 30
_SOURCE_INSTRUMENT_MAXIMUM_LEVEL = 45
SOURCE_INSTRUMENT_SUBTYPE_MAP = {
    184: 1, # FLUTE
    185: 0, # LYRE
    186: 5, # MANDOLIN
    187: 4, # HARP
    188: 3, # DRUMS -> DRUM
    189: 2, # HORN
}
_TARGET_INSTRUMENT_SUBTYPE_NAMES = {
    0: "Lyre",
    1: "Flute",
    2: "Horn",
    3: "Drum",
    4: "Harp",
    5: "Mandolin",
}
_TARGET_INSTRUMENT_NAME_MAP = {
    "lyre": 0,
    "flute": 1,
    "horn": 2,
    "drum": 3,
    "drums": 3,
    "harp": 4,
    "mandolin": 5,
}
_SOURCE_LIQUID_MAP = {
    23: 2,  # champagne -> wine
    24: 16, # Pepsi -> juice
    25: 13, # unholy water -> blood
    26: 2,  # sake -> wine
    27: 21, # curative liquid -> herbal remedy
    28: 10, # eggnog -> milk
}
_SOURCE_SPELL_MAP: dict[int, tuple[str, int | None]] = {
    9: ("full heal", 28),
    36: ("stone skin", 56),
    41: ("haste", 120),
    108: ("fly", 53),
    111: ("plane shift", 239),
    121: ("protection from fire", 433),
    194: ("globe of invulnerability", 172),
    236: ("barkskin", 263),
    453: ("mud to rock", None),
}

_SOURCE_QUEST_REWARD_MAP: dict[int, tuple[str, int | None]] = {
    72: ("meteor swarm", 74),
    73: ("creeping doom", 292),
    94: ("relocate", 2),
    111: ("plane shift", 239),
    112: ("gate", 205),
    113: ("resurrect", 319),
    172: ("vampiric curse", 113),
    194: ("globe of invulnerability", 172),
    237: ("moonwell", 443),
    239: ("group heal", 48),
    264: ("battle trance", 305),
    267: ("ultrablast", None),
    279: ("planar rift", 239),
    285: ("globe of darkness", 93),
    325: ("mind blank", 200),
    327: ("dragonscales", 56),
    329: ("sandstorm", None),
    330: ("inferno", 293),
    359: ("dimension shift", 239),
    363: ("shadow walk", 392),
    437: ("spirit walk", 392),
    438: ("ancestral shield", 89),
    465: ("scry remains", 294),
    477: ("nightmare", 154),
    483: ("elemental fire embodiment", 132),
    484: ("elemental earth embodiment", 56),
    504: ("time stop", 213),
    517: ("phantasmal tendrils", 174),
    521: ("song of recovery", 440),
}


IdentityResolver = Callable[[str, int], int]


@dataclass(slots=True)
class TransformResult:
  """One emitted target record plus explicit bounded-loss diagnostics."""

  text: str
  diagnostics: list[str] = field(default_factory=list)
  ledger: dict[str, Any] | None = None


@lru_cache(maxsize=1)
def _mobile_conversion_inputs():
  return load_mobile_conversion_policy()


def _manifest_bit(manifest: dict[str, Any], table: str, macro: str) -> int:
  for entry in manifest["tables"][table]["entries"]:
    if entry.get("macro") == macro:
      return int(entry["index"])
  raise ValueError(f"constants manifest table {table!r} has no {macro}")


ROOM_FLAG_MAP = {
    1: 0,   # DARK
    2: 1,   # DEATH
    3: 2,   # NO_MOB
    4: 3,   # INDOORS
    5: 5,   # ROOM_SILENT
    7: 19,  # NORECALL
    8: 7,   # NO_MAGIC
    9: 8,   # TUNNEL
    10: 9,  # PRIVATE
    11: 43,  # ARENA
    12: 4,  # SAFE_ZONE
    13: 42,  # NO_PRECIP
    14: 20, # SINGLE_FILE
    15: 44,  # JAIL (RoL compatibility marker)
    16: 21, # NO_TELEPORT
    17: 10,  # RESERVED_OLC -> STAFFROOM
    18: 17,  # HEAL
    19: 25, # NO_HEAL
    20: 33, # HAS_TRAP
    21: 41, # DOCKABLE
    22: 22, # MAGIC_DARK
    23: 23, # MAGIC_LIGHT
    24: 24, # NO_SUMMON
    30: 28, # AIRY_WATER
    31: 27,  # SOLID_FOG
    33: 11, # ROOM_HOUSE
    34: 13, # ROOM_ATRIUM
    36: 45,  # PSPREGEN
    48: 27,  # FIRE_FOG (also adds MAGIC_LIGHT below)
}

# Source zone behavior is persisted on every emitted room because the target
# zone format has no separate flags for silence, safety, magic, recall, or
# summon restrictions. Keys are source bit ordinals, not flag masks.
ZONE_ROOM_FLAG_MAP = {
    0: 5,   # ZONE_SILENT -> ROOM_SOUNDPROOF
    1: 4,   # ZONE_SAFE -> ROOM_PEACEFUL
    4: 21,  # ZONE_NO_TELE -> ROOM_NOTELEPORT
    5: 7,   # ZONE_NO_MAGIC -> ROOM_NOMAGIC
    6: 19,  # ZONE_NO_RECALL -> ROOM_NORECALL
    7: 24,  # ZONE_NO_SUMMON -> ROOM_NOSUMMON
}

# These source flags are handled by transform logic rather than a one-to-one
# persisted room flag.
ROOM_TRANSFORMED_FLAGS = frozenset({6, 32})
ZONE_SOURCE_ONLY_FLAGS = frozenset({2, 3, 8})

SOURCE_ASTRAL_SECTOR = 23
ROL_ASTRAL_ROOM_FLAG = 47

SECTOR_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 9,
    11: 25,
    12: 15,
    13: 19,
    14: 20,
    15: 21,
    16: 22,
    17: 23,
    18: 24,
    19: 18,
    20: 18,
    21: 18,
    22: 18,
    23: 18,
    24: 31,
    25: 23,
    26: 29,
    27: 5,
    28: 1,
    29: 35,
    30: 18,
    31: 25,
    100: 5,  # malformed room 50537 omitted its mountain-road sector value
}

MOB_ACTION_MAP = {
    2: 1,   # SENTINEL
    3: 2,   # SCAVENGER
    4: 3,   # ISNPC -> target's required mobile marker
    5: 105, # NICE_THIEF -> retain theft without automatic retaliation
    6: 5,   # AGGRESSIVE
    7: 6,   # STAY_ZONE
    8: 7,   # WIMPY
    9: 8,   # AGGRESSIVE_EVIL
    10: 9,  # AGGRESSIVE_GOOD
    11: 10, # AGGRESSIVE_NEUTRAL
    12: 11, # MEMORY
    13: 106,# STAY_SECTOR
    15: 12, # HELPER
    16: 107,# DELAY_HUNTER
    17: 108,# ARCHER
    18: 18, # NOKILL
    19: 109,# HAS_PS
    20: 110,# HAS_CL
    21: 111,# HAS_MU
    22: 112,# HAS_TH
    23: 113,# HAS_WA
    25: 32, # CITIZEN/WITNESS
    26: 13, # BREAK_CHARM -> target uncharmable behavior
    27: 12, # PROTECTOR -> helper plus adjacent-combat listener
    28: 21, # MOUNTABLE
    29: 114,# AGG_RACEEVIL
    30: 115,# AGG_RACEGOOD
    31: 33, # HUNTER
    32: 31, # AGG_OUTCAST -> bounded target guard behavior
}

# One source identity expands into more than one existing target behavior.
MOB_ACTION_EXPANSIONS = {
    27: frozenset({34}), # PROTECTOR also responds to adjacent combat
}

# The source attaches these procedures at boot from the authored race code,
# independently of ACT_SPEC and any direct assignment-table procedures. Target
# action flags route them through a composition-safe runtime hook.
MOB_AUTOMATIC_RACE_ACTION = {
    "X": 116,  # standardDemon
    "Y": 117,  # standardDevil
    "MH": 118, # standardUmberhulk
}

# Source initialization affects are prototype properties in the target. Its
# individual elemental protections collapse to the target's aggregate
# ELEMENT_PROT flag.
MOB_AUTOMATIC_RACE_AFFECTS = {
    "X": frozenset({11, 28}),
    "Y": frozenset({11, 28}),
    "MH": frozenset({28}),
}

# RoL's angel race is otherwise collapsed into the target outsider category.
# Retain its identity for source mechanics whose immunity lists distinguish
# angels from other outsiders.
MOB_SOURCE_RACE_IDENTITY_ACTIONS = {
    "Z": frozenset({122}), # RACE_ANGEL
}


def mobile_automatic_race_flags(
    record: RolRecord,
) -> tuple[frozenset[int], frozenset[int]]:
  """Return target prototype flags required by source boot-time race procedures."""

  rows = record.values.get("base_rows", [])
  race_row = rows[0] if len(rows) > 0 else ["N", "0", "0"]
  race_code = race_row[0].upper() if race_row else "N"
  action = MOB_AUTOMATIC_RACE_ACTION.get(race_code)
  actions = frozenset() if action is None else frozenset({action})
  return actions, MOB_AUTOMATIC_RACE_AFFECTS.get(race_code, frozenset())

# ACT_SPEC is implemented by the binding inventory in Phase 6. Emission adds
# MOB_SPEC only when a resolved native/adapted procedure is supplied.
MOB_DEFERRED_ACTIONS = frozenset({1})

# ACT_SAVE is a runtime owner/follower relationship, not a prototype property
# in the target. ACT_SPEC_DIE has no source runtime consumer.
MOB_SOURCE_ONLY_ACTIONS = frozenset({14, 24})

MOB_AFFECT_MAP = {
    1: 1,   # BLIND
    2: 2,   # INVISIBLE
    3: 82,  # FARSEE
    4: 4,   # DETECT_INVISIBLE
    5: 26,  # HASTE
    6: 6,   # SENSE_LIFE
    7: 42,  # MINOR_GLOBE
    8: 97,  # STONE_SKIN -> WARDED
    9: 92,  # CHARGING
    11: 77,  # WRAITHFORM -> IMMATERIAL
    12: 18, # WATERBREATH
    13: 31, # KNOCKED_OUT -> STUN
    14: 13, # PROTECT_EVIL
    15: 32,  # BOUND -> PARALYZED
    17: 14, # PROTECT_GOOD
    18: 15, # SLEEP
    19: 101, # SKILL_AWARE -> AWARE
    20: 19, # SNEAK
    21: 20, # HIDE
    22: 30, # FEAR
    23: 22, # CHARM
    24: 121, # MEDITATE -> RAPID_BUFF (bounded spell-preparation equivalent)
    25: 97,  # BARKSKIN -> WARDED
    26: 11, # INFRAVISION
    27: 103,# LEVITATE
    28: 17, # FLY
    29: 101, # AWARE
    30: 28, # PROTECT_FIRE -> ELEMENT_PROT
    33: 40, # FIRE_SHIELD
    34: 33, # ULTRAVISION
    35: 3,  # DETECT_EVIL -> DETECT_ALIGN
    36: 3,  # DETECT_GOOD -> DETECT_ALIGN
    37: 5,  # DETECT_MAGIC
    38: 97,  # MAJOR_PHYSICAL -> WARDED
    39: 28,  # PROTECT_COLD -> ELEMENT_PROT
    40: 28,  # PROTECT_LIGHTNING -> ELEMENT_PROT
    41: 32,  # MINOR_PARALYSIS
    42: 32,  # MAJOR_PARALYSIS
    43: 39, # SLOW
    44: 51, # GLOBE
    45: 28,  # PROTECT_GAS -> ELEMENT_PROT
    46: 28, # PROTECT_ACID -> ELEMENT_PROT
    49: 101, # MISSILE_AWARE -> AWARE
    50: 98, # MISSILE_SNARE -> ENTANGLED
    51: 112, # MISSILE_SHIELD -> WIND_WALL
    52: 31, # STUNNED
    60: 16, # PASS_WITHOUT_TRACE -> NOTRACK
    64: 69, # VAMPIRIC_TOUCH
    65: 72, # CATFALL -> SAFEFALL
    69: 51, # METAGLOBE -> GLOBE_OF_INVULN
    70: 123, # ICE_TOMB -> ENCASED_IN_ICE
    71: 23, # BLUR
    72: 118, # BURNING -> ON_FIRE
    73: 117, # REPULSION
    74: 58, # MIND_BLANK
    75: 97, # DRAGONSCALES -> WARDED
    76: 96, # MIRROR_IMAGE
    77: 56, # SEQUESTER -> REFUGE
    78: 38, # NONDETECTION
    79: 53, # DISPLACEMENT
    81: 93, # MORPH -> WILD_SHAPE
    82: 79, # MAGE_FLAME
    83: 73, # TOWER_OF_IRON_WILL
    93: 70, # BLACKMANTLE
    95: 10, # HEX -> CURSE
    96: 97, # ANCESTRAL_SHIELD -> WARDED
    98: 110, # SILENCE_PERSON
    100: 98, # ENTANGLE
    101: 45, # TRUE_SIGHT
    102: 93, # DOPPELGANGER -> WILD_SHAPE
    103: 75, # PLANT_ANCHOR -> NOTELEPORT
    110: 97, # ELEMENTAL_WARD -> WARDED
    112: 97, # CASTER_STONE_SKIN -> WARDED
    118: 97, # CASTER_DRAGONSCALES -> WARDED
    119: 60, # TIME_STOP
    122: 45, # REVELATION -> TRUE_SIGHT
    123: 97, # PROTECTION -> WARDED
    127: 20, # CAMO -> HIDE
    128: 61, # NOFEAR -> BRAVERY
    129: 8, # SANCTUARY
}

# The primary target affect bitset is full. These source behaviors use the
# extensible secondary bitset and small runtime adapters instead.
MOB_AFFECT2_MAP = {
    16: 3, # SLOW_POISON
    61: 4, # DOCILE
}

# These bits are transient engine state in RoL, or are never consumed by the
# source runtime. Persisting them on prototypes would invent behavior.
MOB_SOURCE_ONLY_AFFECTS = frozenset(
    {10, 47, 48, 53, 54, 55, 56, 57, 58, 59, 62, 63, 67, 97}
)

OBJECT_TYPE_MAP = {
    0: 12,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 5,  # ITEM_FIREWEAPON -> ITEM_WEAPON; the target's own type is deprecated
    7: 14,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 31,
    15: 15,
    16: 16,
    17: 17,
    18: 18,
    19: 19,
    20: 20,
    21: 21,
    22: 22,
    23: 28,
    24: 12,
    25: 29,
    26: 33,
    27: 34,
    28: 22,
    29: 35,
    30: 36,
    31: 37,
    SOURCE_ITEM_TYPE_INSTRUMENT: TARGET_ITEM_INSTRUMENT,
    33: 28,
    34: 14,
    35: 44,
    36: 45,
    37: 25,
    38: 12,
    39: 42,
    40: 12,
    8388672: 12, # malformed object 34864 shifted its extra flags into item type
}

OBJECT_EXTRA_MAP = {
    0: 0,
    1: 39,
    3: 16,
    4: 116,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 39,
    13: 38,
    14: 42,
    15: 41,
    16: 117,
    17: 118,
    18: 40,
    19: 43,
    20: 119,
    21: 120,
    22: 121,
    23: 2,
    24: 122,
    25: 15,
    26: 13,
    27: 14,
    28: 12,
    29: 123,
    30: 124,
}

# ITEM_DARK participates in RoL light recalculation, but the source light
# counters never consume it. Persisting target darkness would invent behavior.
OBJECT_SOURCE_ONLY_FLAGS = frozenset({2})

# read_object() in EXAMPLE/RealmsOfLuminari/src/db.c strips AFF_HIDE from every
# object as it loads ("No hide items."), so a source object carrying the bit
# confers nothing at runtime. Persisting it would invent behavior. This is
# object-specific: source mobiles keep AFF_HIDE, so it must not join
# MOB_SOURCE_ONLY_AFFECTS.
OBJECT_SOURCE_ONLY_AFFECTS = frozenset({21})

ROL_OBJECT_TRAP_EXTRA_BIT = 113
ROL_OBJECT_TRAP_EFFECT_MASK = 0xFFF
ROL_OBJECT_TRAP_DAMAGE_TYPES = frozenset((*range(1, 8), *range(11, 17), 30, 31))
ROL_OBJECT_TRAP_VALUE_OFFSET = 10

OBJECT_WEAR_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 6,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 14,
    15: 13,
    16: 14,
    17: 18,
    18: 15,
    19: 17,
    20: 16,
    21: 19,
    22: 10,
}

# Object 10455 is one playing card among an otherwise identical deck. Its two
# high wear bits are source corruption, not axe/pickaxe crafting slots.
OBJECT_SOURCE_ONLY_WEAR_FLAGS = frozenset({25, 27})

# Source apply 17 is RoL's "ARMOR", stated on the descending-AC scale where a
# negative modifier is protection. The target's live armour-class apply is
# APPLY_AC_NEW: ascending, and multiplied by ten by affect_modify
# (src/handler.c). Target APPLY_AC (17) is deprecated -- is_valid_apply()
# rejects it (src/utils.c) and it reaches armour class only as a tenth-scale
# remainder -- so armour applies are retargeted rather than passed through.
TARGET_APPLY_AC_NEW = 27
SOURCE_ARMOR_APPLY = 17

# RoL saving throws are descending targets: NewSaves() explicitly says "less
# is more" and multiplies item modifiers by five percentage points. Luminari
# adds these applies to a d20 save bonus, where higher is better. One point is
# still five percentage points, so preserve the magnitude and invert the sign.
SOURCE_SAVING_THROW_APPLIES = frozenset({20, 21, 22, 23, 24})

APPLY_MAP = {
    0: 0,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 0,
    7: 7,
    8: 8,
    9: 9,
    10: 10,
    11: 11,
    12: 12,
    13: 13,
    14: 14,
    15: 15,
    16: 16,
    17: TARGET_APPLY_AC_NEW,  # ARMOR, rescaled by _convert_armor_apply_modifier
    18: 18,
    19: 19,
    20: 20,
    21: 20,
    22: 21,
    23: 21,
    24: 22,
    25: 28,
    26: 2,  # AGI -> DEX
    27: 4,  # POW -> WIS
    28: 6,
    31: 1,  # STR_MAX -> STR
    32: 2,  # DEX_MAX -> DEX
    33: 3,  # INT_MAX -> INT
    34: 4,  # WIS_MAX -> WIS
    35: 5,  # CON_MAX -> CON
    36: 2,  # AGI_MAX -> DEX
    37: 4,  # POW_MAX -> WIS
    38: 6,  # CHA_MAX -> CHA
    51: 25,
    52: 13,
}

# Karma, Luck, and race-factor applies are source-only attributes with no
# target equivalent; racial modifiers in Luminari are flat character attributes
# rather than equipment multipliers.
OBJECT_SOURCE_ONLY_APPLIES = frozenset({29, 30, 39, 40, 41, 43, 45, 48, 49, 50})

# Converted item applies default to BONUS_TYPE_UNIVERSAL (23, stacks with everything)
OBJECT_APPLY_DEFAULT_BONUS_TYPE = 23

# The zone E command's position argument is a source WEAR_* constant
# (EXAMPLE/RealmsOfLuminari/src/structs.h:1120-1146), resolved at reset through
# restore_wear[] (EXAMPLE/RealmsOfLuminari/src/files.c:547). It is not an index
# into the source equipment_types[] display table, which omits SECONDARY_WEAPON
# and therefore runs one short from 17 up. Source 25 (WEAR_TAIL) has no target
# slot and its resets are dropped with a diagnostic.
EQUIPMENT_POSITION_MAP = {
    **{position: position for position in range(17)},
    17: 18,  # SECONDARY_WEAPON -> WEAR_WIELD_OFFHAND
    18: 17,  # HOLD -> WEAR_HOLD_1
    19: 26,  # WEAR_EYES -> WEAR_EYES
    20: 22,  # WEAR_FACE -> WEAR_FACE
    21: 24,  # WEAR_EARRING_R -> WEAR_EAR_R
    22: 25,  # WEAR_EARRING_L -> WEAR_EAR_L
    23: 23,  # WEAR_QUIVER -> WEAR_AMMO_POUCH
    24: 27,  # GUILD_INSIGNIA -> WEAR_BADGE
}

# Source weapons store a one-based index into RoL's weapons[] verb table
# (constant.c), while the target stores a zero-based index into
# attack_hit_text[] (src/combat/fight.c). The two tables share several verbs at
# different offsets, so the values must be translated rather than passed
# through. Source 0 and anything above 11 is rejected by the source runtime as
# well; those fall back to the target's "hit" verb.
SOURCE_WEAPON_MESSAGE_MAP = {
    1: 2,   # Whip -> whip
    2: 2,   # Whip -> whip
    3: 3,   # Slash -> slash
    4: 6,   # Crush -> crush
    5: 6,   # Crush -> crush
    6: 6,   # Crush -> crush
    7: 5,   # Bludgeon -> bludgeon
    8: 8,   # Claw -> claw
    9: 8,   # Claw -> claw
    10: 4,  # Bite -> bite
    11: 11, # Pierce -> pierce
}

# Target wear bits a weapon carries after set_weapon_object()
# (src/obj/treasure.c:2562) clears the word and sets these two.
TARGET_WEAR_TAKE = 0
TARGET_WEAR_WIELD = 13

# Target object proficiency, the 'G' block. set_weapon_object() does not touch
# it, but leaving every converted weapon on ITEM_PROF_NONE throws away the one
# piece of weapon-training data weapon_list[] carries. The target's ladder is
# ITEM_PROF_MINIMAL/BASIC/ADVANCED/MASTER/EXOTIC (src/structs.h:4460), so the
# D20 simple/martial/exotic tiers land on its first, second, and last rungs.
# invalid_prof() is commented out at every call site (src/handler.c:2490,
# src/obj/objsave.c:801), so this is descriptive today rather than restrictive.
TARGET_ITEM_PROF_NONE = 0
TARGET_ITEM_PROF_MINIMAL = 1
TARGET_ITEM_PROF_BASIC = 2
TARGET_ITEM_PROF_EXOTIC = 5
WEAPON_FLAG_SIMPLE = 1 << 0
WEAPON_FLAG_MARTIAL = 1 << 1
WEAPON_FLAG_EXOTIC = 1 << 2

SOURCE_APPLY_HITROLL = 18
SOURCE_APPLY_DAMROLL = 19
# OLC accepts 0..10 for ITEM_WEAPON and ITEM_MISSILE value 5 (oedit.c:2978).
TARGET_MIN_ENHANCEMENT_BONUS = 0
TARGET_MAX_ENHANCEMENT_BONUS = 10

_SOURCE_COLOR = re.compile(r"&\+([A-Za-z])|&([Nn])")


def convert_text(value: str | None) -> tuple[str, list[str]]:
  """Convert legacy colors and return canonical ASCII/LF target text."""

  diagnostics: list[str] = []
  text = value or ""
  # The target reader runs parse_at() over every tilde string, so a bare '@'
  # becomes a color introducer and swallows the next character. Escape source
  # at-signs to '@@' before introducing our own '@' color codes below.
  if "@" in text:
    text = text.replace("@", "@@")
    diagnostics.append("escaped literal '@' as '@@' for the target color parser")
  text = _SOURCE_COLOR.sub(
      lambda match: "@n" if match.group(2) else f"@{match.group(1)}",
      text,
  )
  text = text.replace("\r\n", "\n").replace("\r", "\n")
  if "~" in text:
    text = text.replace("~", "-")
    diagnostics.append("embedded tilde replaced with '-' to preserve target framing")
  encoded = text.encode("ascii", errors="replace")
  if encoded.decode("ascii") != text:
    diagnostics.append("non-ASCII source bytes replaced with '?' for target compatibility")
  return encoded.decode("ascii"), diagnostics


def _tilde(value: str | None) -> tuple[str, list[str]]:
  text, diagnostics = convert_text(value)
  return f"{text}~\n", diagnostics


def _bounded_tilde(value: str | None, context: str) -> tuple[str, list[str]]:
  """Emit a target string without exceeding the runtime's physical-line buffer."""

  text, diagnostics = convert_text(value)
  output: list[str] = []
  wrapped = 0
  for line in text.split("\n"):
    if len(line.encode("ascii")) <= 480:
      output.append(line)
      continue
    chunks = textwrap.wrap(
        line,
        width=480,
        break_long_words=True,
        break_on_hyphens=False,
        replace_whitespace=False,
        drop_whitespace=True,
    )
    output.extend(chunks or [""])
    wrapped += 1
  if wrapped:
    diagnostics.append(
        f"wrapped {wrapped} overlong {context} physical line(s) for the target reader"
    )
  return "\n".join(output) + "~\n", diagnostics


def _source_mask_bits(mask: int, logical_offset: int) -> set[int]:
  return {
      logical_offset + bit
      for bit in range(32)
      if mask & (1 << bit)
  }


def _mapped_bits(source_bits: set[int], mapping: dict[int, int]) -> set[int]:
  return {mapping[bit] for bit in source_bits if bit in mapping}


def _convert_armor_apply_modifier(modifier: int) -> int:
  """Restate a source ARMOR apply as a target APPLY_AC_NEW modifier.

  The source scale is descending and ten times the target scale, so the sign is
  inverted and the magnitude is divided by ten. Any non-zero source modifier
  keeps at least one point of effect in its converted direction, because the
  source author expressed a deliberate armour-class change.
  """
  if not modifier:
    return 0
  magnitude = max(1, abs(modifier) // 10)
  return -magnitude if modifier > 0 else magnitude


def _unmapped(source_bits: set[int], mapping: dict[int, int]) -> list[int]:
  return sorted(source_bits - mapping.keys())


def _encoded(bits: set[int]) -> str:
  return " ".join(encode_bits(bits))


def _numeric_bitarray(bits: set[int], width: int = 4) -> str:
  values = [0] * width
  for bit in bits:
    if 0 <= bit < width * 32:
      values[bit // 32] |= 1 << (bit % 32)
  return " ".join(str(value) for value in values)


def _room_size_bits(base: list[int]) -> set[int]:
  if len(base) < 6:
    return set()
  length, width, height = (max(0, value) for value in base[3:6])
  effective_height = (length + width) // 2 if height == 500 else height
  volume = length * width * effective_height
  if volume <= 27:
    return {31}
  if volume <= 125:
    return {30}
  if length < 6 or width < 6:
    return {20}
  return set()


def _directive_rows(record: RolRecord, token: str) -> list[dict[str, object]]:
  return [directive for directive in record.directives if directive["token"] == token]


def _object_trap_values(
    record: RolRecord,
    values: list[int],
    diagnostics: list[str],
) -> tuple[int, int, int, int, int, int] | None:
  """Validate and normalize the source object's optional six-field trap payload."""

  valid_rows: list[tuple[dict[str, object], list[int]]] = []
  for directive in _directive_rows(record, "T"):
    arguments = [int(value) for value in directive.get("arguments", [])]
    if len(arguments) != 6:
      diagnostics.append(
          "excluded inactive/malformed source object trap at source line "
          f"{directive['line']} ({len(arguments)} of 6 fields)"
      )
      continue
    valid_rows.append((directive, arguments))

  if not valid_rows:
    return None
  if len(valid_rows) > 1:
    lines = [int(directive["line"]) for directive, _ in valid_rows]
    raise ValueError(f"source object has multiple active trap rows at lines {lines}")
  if any(values[ROL_OBJECT_TRAP_VALUE_OFFSET:ROL_OBJECT_TRAP_VALUE_OFFSET + 6]):
    raise ValueError("source object trap conflicts with occupied target values 10..15")

  directive, arguments = valid_rows[0]
  effect, damage_type, charges, level, dice_count, dice_size = arguments
  if effect <= 0 or effect & ~ROL_OBJECT_TRAP_EFFECT_MASK:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid effect mask {effect}"
    )
  if damage_type not in ROL_OBJECT_TRAP_DAMAGE_TYPES:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid damage type {damage_type}"
    )
  if charges < -1:
    diagnostics.append(
        f"normalized source object trap charges {charges} to unlimited (-1) at source line "
        f"{directive['line']}"
    )
    charges = -1
  if charges > 32767:
    raise ValueError(
        f"source object trap at line {directive['line']} has out-of-range charges {charges}"
    )
  if level < 0:
    raise ValueError(
        f"source object trap at line {directive['line']} has negative level {level}"
    )
  if level > 100:
    diagnostics.append(
        f"capped source object trap level {level} at 100 at source line {directive['line']}"
    )
    level = 100
  if dice_count < 0 or dice_size < 0 or dice_count > 32767 or dice_size > 32767:
    raise ValueError(
        f"source object trap at line {directive['line']} has invalid dice "
        f"{dice_count}d{dice_size}"
    )
  if bool(dice_count) != bool(dice_size):
    diagnostics.append(
        f"normalized incomplete source object trap dice {dice_count}d{dice_size} to the "
        f"level-derived default at source line {directive['line']}"
    )
    dice_count = 0
    dice_size = 0

  diagnostics.append(
      f"converted source object trap at line {directive['line']} into ITEM_TRAPPED values 10..15"
  )
  return effect, damage_type, charges, level, dice_count, dice_size


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


def _quest_command(
    directive: dict[str, object],
    resolve: IdentityResolver,
) -> tuple[str | None, str | None]:
  token = str(directive["token"])
  subtype = str(directive.get("subtype", ""))
  arguments = [int(value) for value in directive.get("arguments", [])]
  line = int(directive["line"])
  direction = "I" if token == "G" else "O"

  if token == "R" and subtype == "A":
    return f"{direction} A 0 0\n", None
  if not arguments:
    return None, f"excluded incomplete {token}:{subtype} quest direction at source line {line}"
  value = arguments[0]
  location = arguments[1] if len(arguments) > 1 else 0

  if token == "G" and subtype == "I":
    try:
      target = resolve("obj", value)
    except (KeyError, ValueError) as error:
      return None, f"excluded unresolved required item {value} at source line {line}: {error}"
    return f"{direction} I {target} 0\n", None
  if token == "G" and subtype == "C":
    return f"{direction} C {value} 0\n", None
  if token == "R" and subtype == "I":
    if location > value and location - value < 100:
      return (
          None,
          f"excluded random item reward {value}-{location} at source line {line}; "
          "the target HLQ item command has no range contract",
      )
    try:
      target = resolve("obj", value)
    except (KeyError, ValueError) as error:
      return None, f"excluded unresolved item reward {value} at source line {line}: {error}"
    return f"{direction} I {target} 0\n", None
  if token == "R" and subtype == "C":
    return f"{direction} C {value} 0\n", None
  if token == "R" and subtype == "E":
    return f"{direction} E {value} 0\n", None
  if token == "R" and subtype == "P":
    return f"{direction} P {value} 0\n", None
  if token == "R" and subtype == "S":
    mapped = _SOURCE_QUEST_REWARD_MAP.get(value)
    if mapped is None:
      return (
          "",
          f"omitted unmapped source spell or skill reward {value} at source line {line}",
      )
    source_name, target_spell = mapped
    if target_spell is None:
      return (
          "",
          f"omitted source-only quest reward {value} ({source_name}) at source line {line}; "
          "the target has no equivalent teachable spell contract",
      )
    diagnostic = None
    if value != target_spell:
      diagnostic = (
          f"mapped source quest reward {value} ({source_name}) to target spell "
          f"{target_spell} at source line {line}"
      )
    return f"{direction} T {target_spell} 0\n", diagnostic
  if token == "R" and subtype in {"M", "O"}:
    target_kind = "mob" if subtype == "M" else "obj"
    try:
      target_value = resolve(target_kind, value)
      target_location = resolve("wld", location) if location > 0 else 0
    except (KeyError, ValueError) as error:
      return (
          None,
          f"excluded unresolved {subtype} reward at source line {line}: {error}",
      )
    return f"{direction} {subtype} {target_value} {target_location}\n", None
  return (
      None,
      f"excluded unsupported {token}:{subtype} quest direction at source line {line}",
  )


def emit_hlquest(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
) -> TransformResult:
  """Compile one source quest-host block into canonical target HLQ entries."""

  diagnostics: list[str] = []
  entries: list[tuple[int, str, dict[str, object] | list[dict[str, object]]]] = []
  current_completion: list[dict[str, object]] | None = None
  resolved_host = resolve("mob", record.vnum)
  if resolved_host != destination_vnum:
    diagnostics.append(
        f"quest action destination {destination_vnum} differs from resolved host "
        f"{resolved_host}; used the resolved host identity"
    )

  for directive in record.directives:
    token = str(directive["token"])
    if token == "M":
      entries.append((int(directive["line"]), "M", directive))
    elif token == "Q":
      current_completion = [directive]
      entries.append((int(directive["line"]), "Q", current_completion))
    elif token in {"G", "R", "D"}:
      if current_completion is None:
        diagnostics.append(
            f"excluded {token} quest direction before any completion at source line "
            f"{directive['line']}"
        )
      else:
        current_completion.append(directive)

  lines = [f"#{resolved_host}\n"]
  for _, entry_type, payload in sorted(entries, key=lambda item: item[0]):
    if entry_type == "M":
      assert isinstance(payload, dict)
      keyword_value = str(payload.get("keyword", ""))
      message_value = str(payload.get("message", ""))
      if not keyword_value.strip():
        keyword_value = "unknown"
        diagnostics.append(
            f"replaced an empty quest keyword at source line {payload['line']}"
        )
      if not message_value.strip():
        message_value = "."
        diagnostics.append(
            f"replaced an empty ASK reply at source line {payload['line']}"
        )
      keyword, text_diagnostics = _bounded_tilde(keyword_value, "quest keyword")
      diagnostics.extend(text_diagnostics)
      message, text_diagnostics = _bounded_tilde(message_value, "quest reply")
      diagnostics.extend(text_diagnostics)
      lines.extend(["A!\n", keyword, message])
      continue

    assert isinstance(payload, list)
    completion = payload[0]
    inputs = [directive for directive in payload[1:] if directive["token"] == "G"]
    compiled_inputs = [_quest_command(directive, resolve) for directive in inputs]
    input_diagnostics = [item[1] for item in compiled_inputs if item[1] is not None]
    if any(command is None for command, _ in compiled_inputs):
      diagnostics.extend(str(item) for item in input_diagnostics)
      diagnostics.append(
          f"excluded completion at source line {completion['line']} because a required "
          "input cannot be staged"
      )
      continue
    reply = str(completion.get("message", ""))
    disappear_messages = [
        str(directive.get("message", ""))
        for directive in payload[1:]
        if directive["token"] == "D"
    ]
    if disappear_messages:
      reply += "".join(disappear_messages)
      diagnostics.append(
          f"folded {len(disappear_messages)} disappear message(s) into the completion "
          f"reply at source line {completion['line']}"
      )
    if not reply.strip():
      reply = "."
      diagnostics.append(
          f"replaced an empty completion reply at source line {completion['line']}"
      )
    reply_text, text_diagnostics = _bounded_tilde(reply, "quest reply")
    diagnostics.extend(text_diagnostics)
    lines.extend(["Q!\n", reply_text])

    rewards = [directive for directive in payload[1:] if directive["token"] == "R"]
    for command, command_diagnostic in compiled_inputs:
      assert command is not None
      lines.append(command)
      if command_diagnostic is not None:
        diagnostics.append(command_diagnostic)
    for directive in reversed(rewards):
      command, command_diagnostic = _quest_command(directive, resolve)
      if command is not None:
        lines.append(command)
      if command_diagnostic is not None:
        diagnostics.append(command_diagnostic)
    if disappear_messages:
      lines.append("O D 0 0\n")
    lines.append("S\n")

  return TransformResult("".join(lines), diagnostics)


def _exit_flags(source_flags: int) -> int:
  is_door = bool(source_flags & 0x1FF)
  pickproof = bool(source_flags & (1 << 8))
  hidden = bool(source_flags & (1 << 6))
  if not is_door:
    return 0
  blocked = bool(source_flags & (1 << 7))
  return 1 + int(pickproof) + (2 if hidden else 0) + (4 if blocked else 0)


def emit_room(
    record: RolRecord,
    destination_vnum: int,
    destination_zone: int,
    resolve: IdentityResolver,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
    source_zone_flags: int = 0,
    required_flag_bits: tuple[int, ...] = (),
) -> TransformResult:
  """Emit one modern target room record."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  name, text_diagnostics = _tilde(strings.get("name"))
  diagnostics.extend(text_diagnostics)
  description, text_diagnostics = _tilde(strings.get("description"))
  diagnostics.extend(text_diagnostics)
  base = record.values.get("base", [])
  first_mask = base[1] if len(base) > 1 else 0
  second_mask = base[6] if len(base) > 6 else 0
  source_flags = _source_mask_bits(first_mask, 1) | _source_mask_bits(second_mask, 33)
  target_flags = _mapped_bits(source_flags, ROOM_FLAG_MAP) | _room_size_bits(base)
  source_sector = base[2] if len(base) > 2 else 0
  if source_sector == SOURCE_ASTRAL_SECTOR:
    target_flags.add(ROL_ASTRAL_ROOM_FLAG)
  target_flags.update(required_flag_bits)
  source_zone_bits = _source_mask_bits(source_zone_flags, 0)
  target_flags.update(_mapped_bits(source_zone_bits, ZONE_ROOM_FLAG_MAP))
  if 32 in source_flags:
    target_flags.discard(7)
  if 48 in source_flags:
    target_flags.add(23)
  missing = sorted(source_flags - ROOM_FLAG_MAP.keys() - ROOM_TRANSFORMED_FLAGS)
  if missing:
    diagnostics.append(f"room flags without target persistence: {missing}")
  missing_zone = sorted(source_zone_bits - ZONE_ROOM_FLAG_MAP.keys() - ZONE_SOURCE_ONLY_FLAGS)
  if missing_zone:
    diagnostics.append(f"zone flags without room compatibility: {missing_zone}")
  if source_zone_bits & ZONE_SOURCE_ONLY_FLAGS:
    diagnostics.append(
        "preserved source-only zone metadata outside room flags: "
        f"{sorted(source_zone_bits & ZONE_SOURCE_ONLY_FLAGS)}"
    )
  sector = SECTOR_MAP.get(source_sector, 0)
  if 6 in source_flags:
    sector = 9
  lines = [
      f"#{destination_vnum}\n",
      name,
      description,
      f"{destination_zone} {_encoded(target_flags)} {sector}\n",
  ]
  for directive in record.directives:
    token = directive["token"]
    if token == "D":
      arguments = directive.get("arguments", [])
      if len(arguments) < 3 or directive.get("source_defaulted_destination"):
        diagnostics.append(f"excluded incomplete exit at source line {directive['line']}")
        continue
      exit_description, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      try:
        key = resolve("obj", arguments[1]) if arguments[1] > 0 else -1
      except (KeyError, ValueError) as error:
        key = -1
        diagnostics.append(
            f"removed unresolved exit key {arguments[1]} at source line "
            f"{directive['line']}: {error}"
        )
      try:
        destination = resolve("wld", arguments[2]) if arguments[2] > 0 else -1
      except (KeyError, ValueError) as error:
        diagnostics.append(
            f"excluded exit with unresolved destination {arguments[2]} at source line "
            f"{directive['line']}: {error}"
        )
        continue
      lines.extend(
          [
              f"D{directive['direction']}\n",
              exit_description,
              keyword,
              f"{_exit_flags(arguments[0])} {key} {destination}\n",
          ]
      )
      if len(arguments) > 3:
        valid_trap = (
            len(arguments) == 10
            and arguments[0] >= 16
            and arguments[3] in {0, 1}
            and arguments[4] in {1, 2, 3, 4, 5, 10, 11}
            and 0 <= arguments[5] <= arguments[6] <= 32766
            and arguments[7] in {0, 1}
            and -100 <= arguments[8] <= 100
            and 0 <= arguments[9] <= 100
        )
        if valid_trap:
          lines.append(
              f"Y {directive['direction']} {arguments[3]} {arguments[4]} "
              f"{arguments[5]} {arguments[6]} {arguments[7]} {arguments[8]} "
              f"{arguments[9]}\n"
          )
          diagnostics.append(
              f"adapted legacy exit trap payload at source line {directive['line']}"
          )
        else:
          diagnostics.append(
              f"excluded malformed legacy exit trap payload at source line {directive['line']}"
          )
    elif token == "E":
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      extra, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      lines.extend(["E\n", keyword, extra])
    elif token == "R":
      arguments = directive.get("arguments", [])
      if len(arguments) < 2:
        diagnostics.append(
            f"excluded incomplete room level range at source line {directive['line']}"
        )
        continue
      minimum_level, maximum_level = arguments[:2]
      if minimum_level < 1:
        if minimum_level != -1:
          diagnostics.append(
              f"normalized source room minimum level {minimum_level} to unrestricted at "
              f"source line {directive['line']}"
          )
        minimum_level = -1
      elif minimum_level > _TARGET_MAX_LEVEL:
        raise ValueError(
            f"source room minimum level {minimum_level} exceeds target maximum "
            f"{_TARGET_MAX_LEVEL} at "
            f"source line {directive['line']}"
        )
      if maximum_level < 1 or maximum_level > _TARGET_MAX_LEVEL:
        if maximum_level != -1:
          diagnostics.append(
              f"normalized source room maximum level {maximum_level} to unrestricted at "
              f"source line {directive['line']} because the target maximum level is "
              f"{_TARGET_MAX_LEVEL}"
          )
        maximum_level = -1
      if minimum_level > 0 and maximum_level > 0 and minimum_level > maximum_level:
        raise ValueError(
            f"source room level range {minimum_level}..{maximum_level} is reversed at "
            f"source line {directive['line']}"
        )
      lines.append(f"R {minimum_level} {maximum_level}\n")
    elif token == "F":
      diagnostics.append(
          f"omitted source-inert room fall chance at source line {directive['line']}; "
          "the source loader stores and validates it but no runtime path consumes it"
      )
    elif token == "M":
      diagnostics.append(
          f"omitted obsolete source room mana at source line {directive['line']}; the "
          "source runtime never consumes it and target M is reserved for moving rooms"
      )
  if special_proc is not None:
    lines.extend(["Z\n", f"{special_proc}\n"])
  lines.extend(["C\n", "0 0\n", "S\n"])
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)


def emit_mobile(
    record: RolRecord,
    destination_vnum: int,
    special_proc: str | None = None,
    special_resolved: bool = False,
    attachments: tuple[int, ...] = (),
    required_action_bits: tuple[int, ...] = (),
    required_affect_bits: tuple[int, ...] = (),
    calculator: MobileCalculatorClient | None = None,
) -> TransformResult:
  """Emit one enhanced target mobile record."""

  diagnostics: list[str] = []
  mobile_policy, manifest, registry = _mobile_conversion_inputs()
  selection = select_mobile_conversion(record, mobile_policy, manifest, registry)
  calculator_client = calculator or default_mobile_calculator()
  custom_profile_name = selection.custom_profile or "none"
  try:
    custom_profile_symbol = str(
        mobile_policy["calculator"]["custom_profiles"][custom_profile_name]
    )
    custom_profile = int(
        manifest["symbols"]["mob-autoroll-custom-profiles"]["values"][
            custom_profile_symbol
        ]
    )
  except (KeyError, TypeError, ValueError) as error:
    raise ValueError(
        f"unsupported mobile calculator custom profile {custom_profile_name!r}"
    ) from error
  calculation = calculator_client.calculate(
      destination_vnum,
      selection.mapped_level,
      selection.identity.target_race,
      selection.target_class,
      selection.tier,
      custom_profile,
  )
  stats = calculation.persisted
  hit_points = stats.hit_points
  if selection.custom_hit_points is not None and hit_points != selection.custom_hit_points:
    raise ValueError(
        f"calculator custom profile {custom_profile_name!r} returned {hit_points} hit points, "
        f"expected {selection.custom_hit_points} for mobile {record.record_id}"
    )
  if stats.armor_class % 10:
    raise ValueError(
        f"calculator returned non-serializable armor class {stats.armor_class} "
        f"for mobile {record.record_id}"
    )
  if hit_points < 1:
    raise ValueError(
        f"calculator returned non-positive hit points {hit_points} for mobile {record.record_id}"
    )
  strings = record.values.get("strings", {})
  lines = [f"#{destination_vnum}\n"]
  for key in ("aliases", "short_description", "long_description", "description"):
    value, text_diagnostics = _tilde(strings.get(key))
    diagnostics.extend(text_diagnostics)
    lines.append(value)

  flags = record.values.get("flags", [])
  action_mask = int(flags[0]) if len(flags) > 0 else 0
  affect_mask1 = int(flags[1]) if len(flags) > 1 else 0
  affect_mask2 = int(flags[2]) if len(flags) > 2 else 0
  alignment = int(flags[3]) if len(flags) > 3 else 0
  source_actions = _source_mask_bits(action_mask, 1)
  source_affects = _source_mask_bits(affect_mask1, 1) | _source_mask_bits(
      affect_mask2, 33
  )
  rows = record.values.get("base_rows", [])
  race_row = rows[0] if len(rows) > 0 else ["N", "0", "0"]
  race_code = race_row[0].upper() if race_row else "N"
  custom_gold_bit = _manifest_bit(manifest, "mob", "MOB_CUSTOM_GOLD")
  target_actions = _mapped_bits(source_actions, MOB_ACTION_MAP) | {3, custom_gold_bit}
  if selection.custom_profile is not None:
    target_actions.add(_manifest_bit(manifest, "mob", "MOB_CUSTOM_MOB_STATS"))
  for source_action, expanded_actions in MOB_ACTION_EXPANSIONS.items():
    if source_action in source_actions:
      target_actions.update(expanded_actions)
  if special_proc is not None:
    target_actions.add(0)
  target_actions.update(required_action_bits)
  automatic_actions, automatic_affects = mobile_automatic_race_flags(record)
  target_actions.update(automatic_actions)
  target_actions.update(MOB_SOURCE_RACE_IDENTITY_ACTIONS.get(race_code, frozenset()))
  target_affects = _mapped_bits(source_affects, MOB_AFFECT_MAP)
  target_affects.update(automatic_affects)
  target_affects.update(required_affect_bits)
  target_affects2 = _mapped_bits(source_affects, MOB_AFFECT2_MAP)
  missing_actions = sorted(
      source_actions
      - MOB_ACTION_MAP.keys()
      - MOB_DEFERRED_ACTIONS
      - MOB_SOURCE_ONLY_ACTIONS
  )
  missing_affects = sorted(
      source_affects
      - MOB_AFFECT_MAP.keys()
      - MOB_AFFECT2_MAP.keys()
      - MOB_SOURCE_ONLY_AFFECTS
  )
  if missing_actions:
    diagnostics.append(f"mobile action flags requiring behavior reconciliation: {missing_actions}")
  if 1 in source_actions and special_proc is None and not special_resolved:
    diagnostics.append("source ACT_SPEC deferred to Phase 6 binding reconciliation")
  if source_actions & MOB_SOURCE_ONLY_ACTIONS:
    diagnostics.append(
        "omitted source relationship/inert mobile actions: "
        f"{sorted(source_actions & MOB_SOURCE_ONLY_ACTIONS)}"
    )
  if 26 in source_actions:
    diagnostics.append("source BREAK_CHARM uses bounded target uncharmable behavior")
  if 32 in source_actions:
    diagnostics.append("source outcast aggression uses bounded target guard behavior")
  if missing_affects:
    diagnostics.append(f"mobile affect flags without persistent equivalents: {missing_affects}")
  if source_affects & MOB_SOURCE_ONLY_AFFECTS:
    diagnostics.append(
        "omitted source transient/inert mobile affects: "
        f"{sorted(source_affects & MOB_SOURCE_ONLY_AFFECTS)}"
    )
  lines.append(f"{_encoded(target_actions)} {_encoded(target_affects)} {alignment} E\n")

  if selection.custom_profile is not None:
    hit_die_size = 1
    hit_point_bonus = hit_points - 1
  else:
    # Production autoroll stores one hit die whose size is rolled from 1..level,
    # plus the completed base HP in the file bonus field. Use the destination
    # identity to choose that die size reproducibly for generated world data.
    hit_die_size = destination_vnum % selection.mapped_level + 1
    hit_point_bonus = hit_points
  hit_dice = f"1d{hit_die_size}+{hit_point_bonus}"
  damage_dice = f"{stats.damage_dice_count}d{stats.damage_dice_size}+{stats.damage_bonus}"
  lines.append(
      f"{selection.mapped_level} {20 - stats.hitroll} "
      f"{20 - stats.armor_class // 10} {hit_dice} {damage_dice}\n"
  )
  lines.append(f"{stats.gold} {stats.experience}\n")
  lines.append(
      f"{selection.current_position} {selection.default_position} {selection.target_sex}\n"
  )

  identity = selection.identity
  lines.extend(
      [
          f"Class: {selection.target_class}\n",
          f"Race: {identity.target_race}\n",
          f"SubRace 1: {identity.subraces[0]}\n",
          f"SubRace 2: {identity.subraces[1]}\n",
          f"SubRace 3: {identity.subraces[2]}\n",
          f"Size: {identity.final_size}\n",
          f"Tier: {selection.tier}\n",
          f"Str: {stats.strength}\n",
          f"StrAdd: {stats.strength_add}\n",
          f"Int: {stats.intelligence}\n",
          f"Wis: {stats.wisdom}\n",
          f"Dex: {stats.dexterity}\n",
          f"Con: {stats.constitution}\n",
          f"Cha: {stats.charisma}\n",
          f"SavingFort: {stats.saving_fortitude}\n",
          f"SavingRefl: {stats.saving_reflex}\n",
          f"SavingWill: {stats.saving_will}\n",
          f"SavingPoison: {stats.saving_poison}\n",
          f"SavingDeath: {stats.saving_death}\n",
          f"SpellRes: {selection.effective_source_spell_resistance}\n",
      ]
  )
  if target_affects2:
    lines.append(f"Aff2: {_numeric_bitarray(target_affects2)}\n")
  if special_proc is not None:
    lines.append(f"SpecProc: {special_proc}\n")
  lines.append("E\n")
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  if selection.repairs:
    diagnostics.extend(f"automatic source repair: {repair}" for repair in selection.repairs)
  if selection.source_aggression_codes or selection.ignored_aggression_tokens:
    diagnostics.append(
        "excluded source race-list aggression under bounded adaptation: "
        f"{[*selection.source_aggression_codes, *selection.ignored_aggression_tokens]}"
    )
  if selection.source_prestige_bonus > 0:
    diagnostics.append(
        f"excluded source prestige bonus {selection.source_prestige_bonus}; target has no equivalent"
    )
  ledger = selection.ledger()
  ledger["calculator"] = {
      **calculator_client.identity,
      "result": calculation_to_dict(calculation),
      "custom_profile": selection.custom_profile,
      "custom_profile_symbol": custom_profile_symbol,
      "custom_profile_id": custom_profile,
      "custom_hit_points": selection.custom_hit_points,
  }
  ledger["serialization"] = {
      "disposition": "MAPPED",
      "hitroll_file": 20 - stats.hitroll,
      "armor_file": 20 - stats.armor_class // 10,
      "hit_dice": hit_dice,
      "hit_die_size": hit_die_size,
      "hit_point_bonus": hit_point_bonus,
      "damage_dice": damage_dice,
      "final_size_assigned_after_calculation": identity.final_size,
      "mob_custom_gold": True,
      "mob_custom_stats": selection.custom_profile is not None,
  }
  source_rows = record.values.get("base_rows", [])
  source_combat = source_rows[1] if len(source_rows) > 1 else []
  ledger["field_dispositions"] = {
      "file_framing": {
          "disposition": "MAPPED",
          "source_format": record.format_version,
          "target_format": "enhanced-mobile-E",
          "reason": "source and target use different native record framing",
          "player_impact": "none",
      },
      "vnum": {
          "disposition": "MAPPED",
          "source": record.vnum,
          "target": destination_vnum,
          "rule": "canonical-rol-vnum-map",
          "player_impact": "references resolve through the canonical typed identity map",
      },
      "text": {
          "disposition": "MAPPED",
          "fields": ["aliases", "short_description", "long_description", "description"],
          "rule": "legacy-color-and-ascii-v1",
          "player_impact": "visible text is retained with target color syntax",
      },
      "action_mask": {
          "disposition": "MAPPED",
          "source_bits": sorted(source_actions),
          "target_bits": sorted(target_actions),
          "rule": "mobile-action-map-v1",
          "player_impact": "mapped, adapted, deferred, and source-only bits are diagnosed",
      },
      "affect_masks": {
          "disposition": "MAPPED",
          "source_bits": sorted(source_affects),
          "target_affect_bits": sorted(target_affects),
          "target_affect2_bits": sorted(target_affects2),
          "rule": "mobile-affect-map-v1",
          "player_impact": "persistent equivalents are retained; transient exclusions are diagnosed",
      },
      "alignment": {
          "disposition": "EXACT",
          "source": alignment,
          "target": alignment,
          "player_impact": "none",
      },
      "format_letter": {
          "disposition": "MAPPED",
          "source": record.values.get("format_letter"),
          "target": "E",
          "reason": "target-only generated stats require enhanced fields",
          "player_impact": "none",
      },
      "source_hitroll": {
          "disposition": "EXCLUDED",
          "source": source_combat[1] if len(source_combat) > 1 else None,
          "reason": "target calculator owns attack bonus and target file encoding is inverse",
          "player_impact": "target-native attack progression replaces the incompatible raw value",
      },
      "source_armor": {
          "disposition": "EXCLUDED",
          "source": source_combat[2] if len(source_combat) > 2 else None,
          "reason": "target calculator owns armor and target file encoding is inverse-times-ten",
          "player_impact": "target-native defense progression replaces the incompatible raw value",
      },
      "source_hit_dice": {
          "disposition": "EXCLUDED",
          "source": source_combat[3] if len(source_combat) > 3 else None,
          "reason": "calculator or exact named profile owns target-native hit points",
          "player_impact": "ordinary autorolled mobs retain the target's saved hit-die variation",
      },
      "source_damage_dice": {
          "disposition": "EXCLUDED",
          "source": source_combat[4] if len(source_combat) > 4 else None,
          "reason": "calculator and selected tier own target damage",
          "player_impact": "target-native damage progression replaces the raw source roll",
      },
      "target_only_stats": {
          "disposition": "MAPPED",
          "owner": "mob-autoroll-profile-v1",
          "fields": [
              "Str", "StrAdd", "Int", "Wis", "Dex", "Con", "Cha",
              "SavingFort", "SavingRefl", "SavingWill", "SavingPoison", "SavingDeath",
          ],
          "player_impact": "class, race, configuration, and tier are applied exactly once",
      },
  }
  ledger["loader_consequences"] = {
      "source_ability_rolls": "EXCLUDED: target calculator owns deterministic abilities",
      "source_spell_circle_budgets": "ADAPTED: target class spell-slot initialization runs at spawn",
      "source_racial_infravision": "ADAPTED: target race/subrace and affect mappings own vision",
      "source_coin_randomization": "ADAPTED: fixed calculator gold uses MOB_CUSTOM_GOLD",
      "source_dimension_generation": "MAPPED: identity resolver persists one final target size",
      "source_memory_default": "EXCLUDED: target has no safe equivalent to universal memory",
      "source_classless_experience_reduction": "ADAPTED: one target-native experience policy",
      "source_elite_alias_detection": "EXCLUDED: exact words do not silently add target affects",
      "source_scavenger_suppression": "ADAPTED: explicit mapped target flags own behavior",
      "source_psionic_mana": "ADAPTED: target spell-slot initialization owns class resources",
      "source_race_procedures": "ADAPTED: composition-safe target action hooks",
      "source_periodic_and_path_behavior": "ADAPTED: separate special/SOC reconciliation",
      "target_hitroll_inverse": "MAPPED: loader returns the calculator hitroll",
      "target_armor_inverse": "MAPPED: loader returns the calculator armor class",
      "target_hp_roll": (
          "MAPPED: ordinary mobs preserve one saved variable hit die above completed autoroll HP; "
          "exact named profiles retain fixed pre-loader HP"
      ),
      "target_explicit_tier": "EXACT: loader preserves explicit selected tier",
      "target_class_category": "MAPPED: calculator records expected post-load values",
      "target_spell_slots": "ADAPTED: initialized once by target runtime",
  }
  return TransformResult("".join(lines), diagnostics, ledger)


def _instrument_subtype(
    record: RolRecord, source_subtype: int, diagnostics: list[str]
) -> int:
  target_subtype = SOURCE_INSTRUMENT_SUBTYPE_MAP.get(source_subtype)
  if target_subtype is not None:
    diagnostics.append(
        f"mapped source instrument subtype {source_subtype} to target subtype "
        f"{target_subtype} ({_TARGET_INSTRUMENT_SUBTYPE_NAMES[target_subtype]})"
    )
    return target_subtype

  strings = record.values.get("strings", {})
  identity = normalize_identity(
      " ".join(
          str(strings.get(key) or "")
          for key in ("aliases", "short_description", "description")
      )
  )
  words = set(identity.split())
  for name, inferred_subtype in _TARGET_INSTRUMENT_NAME_MAP.items():
    if name not in words:
      continue
    diagnostics.append(
        f"inferred target instrument subtype {inferred_subtype} "
        f"({_TARGET_INSTRUMENT_SUBTYPE_NAMES[inferred_subtype]}) from source object "
        f"identity for unsupported source subtype {source_subtype}"
    )
    return inferred_subtype

  diagnostics.append(
      f"defaulted unsupported source instrument subtype {source_subtype} to target "
      "subtype 0 (Lyre); source object identity has no recognized instrument name"
  )
  return 0


def _instrument_values(
    record: RolRecord, values: list[int], diagnostics: list[str]
) -> list[int]:
  """Translate the active RoL NEW_BARD value contract to target instruments."""

  source_subtype, source_quality, source_effectiveness, source_minimum_level = values[:4]
  target_subtype = _instrument_subtype(record, source_subtype, diagnostics)
  target_quality = max(
      0, min(source_quality, _TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION)
  )
  target_effectiveness = max(
      0, min(source_effectiveness, _TARGET_INSTRUMENT_MAX_EFFECTIVENESS)
  )
  bounded_source_level = max(
      1, min(source_minimum_level, _SOURCE_INSTRUMENT_MAXIMUM_LEVEL)
  )
  target_breakability = _TARGET_INSTRUMENT_DEFAULT_BREAKABILITY - (
      bounded_source_level
      * _TARGET_INSTRUMENT_DEFAULT_BREAKABILITY
      // _SOURCE_INSTRUMENT_MAXIMUM_LEVEL
  )

  if target_quality != source_quality:
    diagnostics.append(
        f"bounded source instrument quality {source_quality} to target difficulty "
        f"maximum {_TARGET_INSTRUMENT_MAX_DIFFICULTY_REDUCTION}"
    )
  if target_effectiveness != source_effectiveness:
    diagnostics.append(
        f"bounded source instrument effectiveness {source_effectiveness} to "
        f"target maximum {_TARGET_INSTRUMENT_MAX_EFFECTIVENESS}"
    )
  if bounded_source_level != source_minimum_level:
    diagnostics.append(
        f"bounded source instrument minimum-use level {source_minimum_level} to "
        f"{bounded_source_level}"
    )
  diagnostics.append(
      f"mapped source instrument minimum-use level {bounded_source_level} to target "
      f"breakability {target_breakability}"
  )

  return [
      target_subtype,
      target_quality,
      target_effectiveness,
      target_breakability,
  ] + values[4:]


def _object_values(
    record: RolRecord,
    source_type: int,
    target_type: int,
    resolve: IdentityResolver,
    diagnostics: list[str],
) -> list[int]:
  values = list(record.values.get("values", []))
  values = (values + [0] * 16)[:16]
  if source_type == SOURCE_ITEM_TYPE_INSTRUMENT:
    values = _instrument_values(record, values, diagnostics)
  if target_type in {17, 23} and not 0 <= values[2] <= _TARGET_MAX_LIQUID:
    source_liquid = values[2]
    values[2] = _SOURCE_LIQUID_MAP.get(source_liquid, 0)
    diagnostics.append(
        f"mapped unsupported source liquid {source_liquid} to target liquid {values[2]}"
    )
  if target_type in _TARGET_MAGIC_ITEM_TYPES and values[0] > _TARGET_MAX_OBJECT_SPELL_LEVEL:
    diagnostics.append(
        f"capped source magic-item spell level {values[0]} at target maximum "
        f"{_TARGET_MAX_OBJECT_SPELL_LEVEL}"
    )
    values[0] = _TARGET_MAX_OBJECT_SPELL_LEVEL
  if source_type in {2, 10}:
    spell_slots = (1, 2, 3)
  elif source_type in {3, 4}:
    spell_slots = (3,)
  else:
    spell_slots = ()
  for slot in spell_slots:
    source_spell = values[slot]
    if source_spell <= 0:
      continue
    mapped = _SOURCE_SPELL_MAP.get(source_spell)
    if mapped is None:
      values[slot] = 0
      diagnostics.append(
          f"disabled unresolved source spell {source_spell} in magic-item slot {slot}"
      )
      continue
    spell_name, target_spell = mapped
    if target_spell is None:
      values[slot] = 0
      diagnostics.append(
          f"disabled source spell {source_spell} ({spell_name}) in magic-item slot "
          f"{slot}; target has no equivalent"
      )
      continue
    values[slot] = target_spell
    diagnostics.append(
        f"mapped source spell {source_spell} ({spell_name}) to target spell "
        f"{target_spell} in magic-item slot {slot}"
    )
  if source_type in {5, SOURCE_ITEM_TYPE_FIREWEAPON}:
    source_message = values[3]
    target_message = SOURCE_WEAPON_MESSAGE_MAP.get(source_message)
    if target_message is None:
      values[3] = 0
      diagnostics.append(
          f"replaced out-of-range source weapon damage message {source_message} "
          "with the target default"
      )
    elif target_message != source_message:
      values[3] = target_message
      diagnostics.append(
          f"mapped source weapon damage message {source_message} to target "
          f"message {target_message}"
      )
  if target_type in {3, 4} and values[2] > values[1]:
    source_maximum = values[1]
    values[1] = values[2]
    diagnostics.append(
        f"raised source wand/staff maximum charges {source_maximum} to current "
        f"charges {values[2]} for the target runtime"
    )
  if source_type in {15, SOURCE_ITEM_TYPE_QUIVER} and values[2] > 0:
    # Source quivers carry the container value layout, key vnum included, and
    # convert to an ammo pouch or a container -- both of which read value[2] as
    # a key vnum in the target.
    source_key = values[2]
    try:
      values[2] = resolve("obj", source_key)
    except (KeyError, ValueError) as error:
      values[2] = -1
      diagnostics.append(f"removed unresolved container key {source_key}: {error}")
  elif source_type == 25:
    source_destination = values[0]
    try:
      destination = resolve("wld", source_destination) if source_destination > 0 else 0
    except (KeyError, ValueError) as error:
      destination = 0
      diagnostics.append(
          f"disabled portal with unresolved room {source_destination}: {error}"
      )
    values = [0, destination, destination, 0] + [0] * 12
  elif source_type == 27 and values[1] > 0:
    source_mobile = values[1]
    try:
      values[1] = resolve("mob", source_mobile)
    except (KeyError, ValueError) as error:
      values[1] = 0
      diagnostics.append(
          f"disabled summon reference to unresolved mobile {source_mobile}: {error}"
      )
  elif source_type == 29 and values[1] > 0:
    source_destination = values[1]
    try:
      values[1] = resolve("wld", source_destination)
    except (KeyError, ValueError) as error:
      values[1] = 0
      diagnostics.append(
          f"disabled vehicle destination to unresolved room {source_destination}: {error}"
      )
  if target_type in {TARGET_ITEM_CONTAINER, TARGET_ITEM_AMMO_POUCH} and values[2] == 65535:
    values[2] = -1
  if source_type == SOURCE_ITEM_TYPE_QUIVER and values[3]:
    # The source quiver kind has been consumed by the item-type decision. The
    # target slot is the corpse flag (IS_CORPSE, src/utils.h:1983).
    diagnostics.append(
        f"zeroed source quiver kind {values[3]}; the target slot is the corpse flag"
    )
    values[3] = 0
  return values


def _object_target_type(
    record: RolRecord,
    source_type: int,
    diagnostics: list[str],
) -> tuple[int, WeaponInference | None, Any]:
  """Resolve the target item type, and any weapon identity it depends on.

  Most source types resolve straight through ``OBJECT_TYPE_MAP``. Three do not,
  because the target type depends on the record rather than only on its source
  type: source weapons and ranged weapons both become ``ITEM_WEAPON``, source
  ammunition becomes either ``ITEM_MISSILE`` or, when it is physically thrown,
  ``ITEM_WEAPON``. Both source quiver kinds use the target ammo-pouch contract.
  """

  values = (list(record.values.get("values", [])) + [0] * 8)[:8]
  if source_type == SOURCE_ITEM_TYPE_WEAPON:
    # Source value[0] is a proc hook, target value[0] is an index into
    # weapon_list[]. Passing it through lands every converted weapon on
    # WEAPON_TYPE_UNDEFINED, which disables criticals, empties the damage-type
    # bitmask so damage reduction never bypasses, and matches no weapon family.
    return TARGET_ITEM_WEAPON, infer_weapon_type(record), None
  if source_type == SOURCE_ITEM_TYPE_FIREWEAPON:
    # The target's own ITEM_FIREWEAPON is deprecated (src/structs.h:4348) and
    # cannot fire: is_using_ranged_weapon() tests the wielded object's
    # weapon_list[] flags and never looks at item type.
    diagnostics.append(
        "retyped source ITEM_FIREWEAPON to ITEM_WEAPON; the target's own "
        "ITEM_FIREWEAPON is deprecated and never fires"
    )
    return TARGET_ITEM_WEAPON, infer_ranged_weapon_type(record), None
  if source_type == SOURCE_ITEM_TYPE_MISSILE:
    inference = infer_ammunition(record)
    if inference.item_type == TARGET_ITEM_WEAPON:
      return (
          TARGET_ITEM_WEAPON,
          WeaponInference(
              inference.weapon_type, inference.name, inference.tier, inference.rule
          ),
          inference,
      )
    return TARGET_ITEM_MISSILE, None, inference
  if source_type == SOURCE_ITEM_TYPE_QUIVER and values[3] == SOURCE_QUIVER_THROWING:
    # A throwing quiver now shares the ammo-pouch contract with missiles.
    diagnostics.append(
        "retained source throwing quiver as ITEM_AMMO_POUCH for throwable weapons"
    )
    return TARGET_ITEM_AMMO_POUCH, None, None
  target_type = OBJECT_TYPE_MAP.get(source_type, 12)
  if source_type not in OBJECT_TYPE_MAP:
    diagnostics.append(f"unknown source item type {source_type}; used ITEM_OTHER")
  return target_type, None, None


def _object_enhancement_bonus(
    record: RolRecord,
    diagnostics: list[str],
) -> int:
  """Restate the source hitroll/damroll applies as the native enhancement bonus.

  RoL has no enhancement-bonus concept and expresses a ``+N`` weapon as
  ``APPLY_HITROLL`` and ``APPLY_DAMROLL`` affects. The target reads
  ``GET_ENHANCEMENT_BONUS()`` into both to-hit and damage already
  (``src/combat/fight.c:7135`` and ``:10500``), so the caller drops the source
  applies after this restatement; emitting both would grant the bonus twice.
  """

  hitroll = 0
  damroll = 0
  for directive in record.directives:
    if directive["token"] != "A":
      continue
    arguments = directive.get("arguments", [])
    if len(arguments) < 2:
      continue
    if arguments[0] == SOURCE_APPLY_HITROLL:
      hitroll += arguments[1]
    elif arguments[0] == SOURCE_APPLY_DAMROLL:
      damroll += arguments[1]
  if not hitroll and not damroll:
    return 0
  # A record stating only one of the two averages against zero, which is the
  # intended reading of a half-stated bonus.
  average = (hitroll + damroll) // 2
  bonus = min(TARGET_MAX_ENHANCEMENT_BONUS, max(TARGET_MIN_ENHANCEMENT_BONUS, average))
  diagnostics.append(
      f"restated source hitroll {hitroll} and damroll {damroll} as enhancement "
      f"bonus {bonus} and dropped the source applies"
  )
  if bonus != average:
    diagnostics.append(
        f"clamped enhancement bonus {average} to {bonus} for the target range "
        f"{TARGET_MIN_ENHANCEMENT_BONUS}..{TARGET_MAX_ENHANCEMENT_BONUS}"
    )
  return bonus


def _apply_weapon_object(
    values: list[int],
    economy: list[int],
    target_wear: set[int],
    inference: WeaponInference,
    enhancement: int,
    diagnostics: list[str],
    carries_attack_message: bool = True,
) -> tuple[int, int, int]:
  """Replicate set_weapon_object() (src/obj/treasure.c:2562) at emit time.

  A converted weapon has to come out mechanically identical to one an immortal
  builds in OLC by picking a weapon type, so dice, cost, weight, material,
  size, and the wear word are all derived from ``weapon_list[]`` rather than
  carried over. Returns the proficiency, material, and size for the ``G``,
  ``H``, and ``I`` blocks.
  """

  entry = weapon_table()[inference.weapon_type]
  values[0] = inference.weapon_type
  if [values[1], values[2]] != [entry.num_dice, entry.dice_size]:
    diagnostics.append(
        f"replaced source damage dice {values[1]}d{values[2]} with the "
        f"{entry.name} table dice {entry.num_dice}d{entry.dice_size}"
    )
  values[1] = entry.num_dice
  values[2] = entry.dice_size
  if not carries_attack_message:
    # A record retyped out of ITEM_MISSILE has a source missile type in this
    # slot, not a damage message. The target reads value[3] as an index into
    # attack_hit_text[] (src/combat/fight.c:11922), so the source value would
    # name an unrelated verb.
    if values[3]:
      diagnostics.append(
          f"zeroed source missile type {values[3]}; the target slot is the "
          "weapon attack message"
      )
    values[3] = 0
  values[4] = enhancement
  # value[5] is the target's loaded-ammo counter, read by weapon_is_loaded()
  # (src/combat/assign_wpn_armor.c:549). The source slot in that position is a
  # rate of fire, which would silently pre-load a converted crossbow.
  if values[5]:
    diagnostics.append(
        f"zeroed source rate of fire {values[5]}; the target slot is the "
        "loaded-ammo counter"
    )
  for slot in range(5, len(values)):
    values[slot] = 0
  source_weight, source_cost = economy[0], economy[1]
  economy[0] = entry.weight
  economy[1] = entry.cost + 1
  if [source_weight, source_cost] != [economy[0], economy[1]]:
    diagnostics.append(
        f"replaced source weight {source_weight} and cost {source_cost} with the "
        f"{entry.name} table weight {economy[0]} and cost {economy[1]}"
    )
  dropped = sorted(target_wear - {TARGET_WEAR_TAKE, TARGET_WEAR_WIELD})
  if dropped:
    diagnostics.append(
        f"cleared object wear flags {dropped}; a weapon carries only TAKE and WIELD"
    )
  target_wear.clear()
  target_wear.update({TARGET_WEAR_TAKE, TARGET_WEAR_WIELD})
  if entry.weapon_flags & WEAPON_FLAG_EXOTIC:
    proficiency = TARGET_ITEM_PROF_EXOTIC
  elif entry.weapon_flags & WEAPON_FLAG_MARTIAL:
    proficiency = TARGET_ITEM_PROF_BASIC
  elif entry.weapon_flags & WEAPON_FLAG_SIMPLE:
    proficiency = TARGET_ITEM_PROF_MINIMAL
  else:
    proficiency = TARGET_ITEM_PROF_NONE
  return proficiency, entry.material, entry.size


def _apply_missile_object(
    record: RolRecord,
    values: list[int],
    inference: Any,
    enhancement: int,
    diagnostics: list[str],
) -> None:
  """Apply the target ITEM_MISSILE value layout to a converted source missile.

  Two of these slots mean something entirely different from the source slot
  sitting in them, so passing them through is a live defect rather than
  lossiness.
  """

  source_values = (list(record.values.get("values", [])) + [0] * 4)[:4]
  values[0] = inference.ammo_type
  # value[1] is the target's imbued spell number: imbued_arrow()
  # (src/combat/fight.c:12057) casts it through call_magic() on every shot. The
  # source slot in that position is a damage die.
  if values[1]:
    diagnostics.append(
        f"zeroed source dice size {values[1]}; the target slot is the imbued "
        "spell number"
    )
  values[1] = 0
  values[2] = missile_break_probability(source_values[2])
  diagnostics.append(
      f"restated source missile durability {source_values[2]} as target break "
      f"probability {values[2]} percent"
  )
  values[3] = 0
  values[4] = enhancement
  for slot in range(5, len(values)):
    values[slot] = 0


def emit_object(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
    required_extra_bits: tuple[int, ...] = (),
    required_value_references: tuple[tuple[int, str], ...] = (),
) -> TransformResult:
  """Emit one modern target object record."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  aliases = str(strings.get("aliases") or "").strip()
  if not aliases:
    aliases = f"converted object {destination_vnum}"
    diagnostics.append("synthesized missing object aliases for target runtime safety")
  short_description = str(strings.get("short_description") or "").strip()
  if not short_description:
    short_description = aliases
    diagnostics.append("synthesized missing object short description for target runtime safety")
  description = str(strings.get("description") or "").strip()
  if not description:
    description = f"{short_description} is here."
    diagnostics.append("synthesized missing object room description for target runtime safety")
  string_values = {
      "aliases": aliases,
      "short_description": short_description,
      "description": description,
      "action_description": strings.get("action_description"),
  }
  lines = [f"#{destination_vnum}\n"]
  for key in ("aliases", "short_description", "description", "action_description"):
    value, text_diagnostics = _tilde(string_values.get(key))
    diagnostics.extend(text_diagnostics)
    lines.append(value)

  source_type = int(record.values.get("item_type") or 0)
  target_type, weapon_inference, ammo_inference = _object_target_type(
      record, source_type, diagnostics
  )
  source_flags = record.values.get("flags", [])
  extra_mask = source_flags[1] if len(source_flags) > 1 else 0
  wear_mask = source_flags[2] if len(source_flags) > 2 else 0
  source_extra = _source_mask_bits(extra_mask, 0)
  source_wear = _source_mask_bits(wear_mask, 0)
  target_extra = _mapped_bits(source_extra, OBJECT_EXTRA_MAP) | set(required_extra_bits)
  target_wear = _mapped_bits(source_wear, OBJECT_WEAR_MAP)
  missing_extra = [
      flag
      for flag in _unmapped(source_extra, OBJECT_EXTRA_MAP)
      if flag not in OBJECT_SOURCE_ONLY_FLAGS
  ]
  missing_wear = sorted(
      source_wear - OBJECT_WEAR_MAP.keys() - OBJECT_SOURCE_ONLY_WEAR_FLAGS
  )
  if missing_extra:
    diagnostics.append(f"object extra flags without direct equivalents: {missing_extra}")
  if source_extra & OBJECT_SOURCE_ONLY_FLAGS:
    diagnostics.append("omitted source-inert object DARK flag")
  if missing_wear:
    diagnostics.append(f"object wear flags without direct equivalents: {missing_wear}")
  if source_wear & OBJECT_SOURCE_ONLY_WEAR_FLAGS:
    diagnostics.append(
        "omitted malformed source object wear flags: "
        f"{sorted(source_wear & OBJECT_SOURCE_ONLY_WEAR_FLAGS)}"
    )

  source_affects: set[int] = set()
  for directive in record.directives:
    if directive["token"] != "AFFECT_FLAGS":
      continue
    offset = int(directive.get("word_offset", 0))
    for ordinal, mask in enumerate(directive.get("arguments", [])):
      source_affects.update(_source_mask_bits(mask, (offset + ordinal) * 32 + 1))
  if source_affects & OBJECT_SOURCE_ONLY_AFFECTS:
    diagnostics.append(
        "omitted source-inert object affects the source loader clears at load: "
        f"{sorted(source_affects & OBJECT_SOURCE_ONLY_AFFECTS)}"
    )
    source_affects -= OBJECT_SOURCE_ONLY_AFFECTS
  target_affects = _mapped_bits(source_affects, MOB_AFFECT_MAP)
  target_affects2 = _mapped_bits(source_affects, MOB_AFFECT2_MAP)
  missing_affects = sorted(
      source_affects
      - MOB_AFFECT_MAP.keys()
      - MOB_AFFECT2_MAP.keys()
      - MOB_SOURCE_ONLY_AFFECTS
  )
  if missing_affects:
    diagnostics.append(f"object affect flags without persistent equivalents: {missing_affects}")
  if source_affects & MOB_SOURCE_ONLY_AFFECTS:
    diagnostics.append(
        "omitted source transient/inert object affects: "
        f"{sorted(source_affects & MOB_SOURCE_ONLY_AFFECTS)}"
    )
  values = _object_values(record, source_type, target_type, resolve, diagnostics)
  for slot, target_kind in required_value_references:
    source_value = values[slot]
    if source_value <= 0:
      continue
    try:
      values[slot] = resolve(target_kind, source_value)
    except (KeyError, ValueError) as error:
      values[slot] = 0
      diagnostics.append(
          f"disabled special-procedure reference {target_kind} {source_value} "
          f"in object value slot {slot}: {error}"
      )
  economy = list(record.values.get("economy", []))
  economy_defaults = [0, 1, 0, 1, 1]
  economy = economy[:5] + economy_defaults[len(economy[:5]):]
  proficiency = material = size = None
  enhancement = 0
  if target_type in {TARGET_ITEM_WEAPON, TARGET_ITEM_MISSILE}:
    enhancement = _object_enhancement_bonus(record, diagnostics)
  if weapon_inference is not None:
    if any(slot == 0 for slot, _ in required_value_references):
      diagnostics.append(
          "replaced a special-procedure reference in object value slot 0 with "
          "the inferred weapon type"
      )
    diagnostics.append(weapon_inference.diagnostic)
    proficiency, material, size = _apply_weapon_object(
        values,
        economy,
        target_wear,
        weapon_inference,
        enhancement,
        diagnostics,
        carries_attack_message=source_type != SOURCE_ITEM_TYPE_MISSILE,
    )
  elif target_type == TARGET_ITEM_MISSILE and ammo_inference is not None:
    diagnostics.append(ammo_inference.diagnostic)
    _apply_missile_object(record, values, ammo_inference, enhancement, diagnostics)
  trap_values = _object_trap_values(record, values, diagnostics)
  if trap_values is not None:
    target_extra.add(ROL_OBJECT_TRAP_EXTRA_BIT)
    values[ROL_OBJECT_TRAP_VALUE_OFFSET:ROL_OBJECT_TRAP_VALUE_OFFSET + 6] = trap_values
  lines.append(
      f"{target_type} {_encoded(target_extra)} {_encoded(target_wear)} "
      f"{_encoded(target_affects)} {_encoded(target_affects2)}\n"
  )
  lines.append(" ".join(str(value) for value in values) + "\n")
  if source_type == 17 and economy[0] > 0:
    # Source drink containers store weight in quarter pounds and the source
    # loader divides by four at load. The target reader applies no such
    # division, so scale the stored weight here instead.
    source_weight = economy[0]
    economy[0] = source_weight // 4
    diagnostics.append(
        f"converted source drink-container weight {source_weight} from quarter "
        f"pounds to {economy[0]}"
    )
  economy[0] = max(0, economy[0])
  economy[2] = max(0, economy[2])
  if economy[3] <= 0:
    economy[3] = 1
  if target_type in {17, 23} and 0 in target_wear and economy[0] < values[1]:
    economy[0] = values[1] + 5
  lines.append(" ".join(str(value) for value in economy) + "\n")

  for directive in record.directives:
    token = directive["token"]
    if token == "E" and directive.get("source_disposition") != "EXCLUDE":
      keyword, text_diagnostics = _tilde(directive.get("keyword"))
      diagnostics.extend(text_diagnostics)
      extra, text_diagnostics = _tilde(directive.get("description"))
      diagnostics.extend(text_diagnostics)
      lines.extend(["E\n", keyword, extra])
    elif token == "A":
      arguments = directive.get("arguments", [])
      if len(arguments) < 2:
        diagnostics.append(f"excluded incomplete object affect at source line {directive['line']}")
        continue
      source_location = arguments[0]
      if (
          target_type in {TARGET_ITEM_WEAPON, TARGET_ITEM_MISSILE}
          and source_location in {SOURCE_APPLY_HITROLL, SOURCE_APPLY_DAMROLL}
      ):
        # Restated as the native enhancement bonus in value[4]; emitting both
        # would grant the bonus twice.
        continue
      if source_location in OBJECT_SOURCE_ONLY_APPLIES:
        diagnostics.append(
            f"omitted source-only object apply {source_location} at source line "
            f"{directive['line']}"
        )
        continue
      location = APPLY_MAP.get(source_location)
      if location is None or location == 0 and arguments[0] != 0:
        diagnostics.append(
            f"excluded unsupported object apply {source_location} at source line {directive['line']}"
        )
        continue
      modifier = arguments[1]
      if source_location == SOURCE_ARMOR_APPLY:
        converted = _convert_armor_apply_modifier(modifier)
        if converted != modifier:
          diagnostics.append(
              f"restated source armor apply {modifier} as APPLY_AC_NEW {converted} at "
              f"source line {directive['line']}"
          )
        modifier = converted
      elif source_location in SOURCE_SAVING_THROW_APPLIES:
        converted = -modifier
        if converted != modifier:
          diagnostics.append(
              f"inverted source saving-throw apply {source_location} modifier "
              f"{modifier} to {converted} at source line {directive['line']}"
          )
        modifier = converted
      lines.extend(
          ["A\n", f"{location} {modifier} {OBJECT_APPLY_DEFAULT_BONUS_TYPE} 0\n"]
      )
  if proficiency is not None:
    # G/H/I, in the order oedit_save_to_disk() writes them (src/olc/genobj.c).
    # The 'I' block is required even when the table size is SIZE_MEDIUM: the
    # loader rewrites a missing or zero size to SIZE_MEDIUM (src/db.c:4112),
    # which would silently resize every converted weapon that is not medium.
    lines.extend(["G\n", f"{proficiency}\n", "H\n", f"{material}\n", "I\n", f"{size}\n"])
  if special_proc is not None:
    lines.extend(["Z\n", f"{special_proc}\n"])
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)


def _reset_probability(command: str, arguments: list[int]) -> int:
  if command in {"G", "R"}:
    value = arguments[3] if len(arguments) >= 4 else 100
  else:
    value = arguments[4] if len(arguments) >= 5 else 100
  return min(100, max(0, value))


def _emit_reset(
    directive: dict[str, object],
    resolve: IdentityResolver,
) -> tuple[str | None, list[str]]:
  command = str(directive["token"])
  arguments = [int(value) for value in directive.get("arguments", [])]
  line = int(directive["line"])
  diagnostics: list[str] = []

  if command != "F" and arguments:
    source_dependency = arguments[0]
    arguments[0] = 1 if source_dependency else 0
    if source_dependency not in {0, 1}:
      diagnostics.append(
          f"normalized source boolean dependency {source_dependency} to "
          f"{arguments[0]} at source line {line}"
      )

  try:
    if command in {"M", "O", "P", "E"}:
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, prototype, maximum, destination = arguments[:4]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      target_kind = "mob" if command == "M" else "obj"
      prototype = resolve(target_kind, prototype)
      if command in {"M", "O"}:
        destination = resolve("wld", destination) if destination >= 0 else destination
      elif command == "P":
        destination = resolve("obj", destination)
      else:
        mapped_position = EQUIPMENT_POSITION_MAP.get(destination)
        if mapped_position is None:
          raise ValueError(f"has unsupported equipment position {destination}")
        destination = mapped_position
      probability = _reset_probability(command, arguments)
      return (
          f"{command} {dependency} {prototype} {maximum} {destination} {probability}\n",
          diagnostics,
      )

    if command == "G":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, prototype, maximum = arguments[:3]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      return (
          f"G {dependency} {resolve('obj', prototype)} {maximum} "
          f"{_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "D":
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, room, direction, state = arguments[:4]
      if room <= 0:
        raise ValueError(f"has non-positive room {room}")
      probability = _reset_probability(command, arguments)
      if 0 <= state <= 2 and probability == 100:
        return f"D {dependency} {resolve('wld', room)} {direction} {state}\n", diagnostics
      if state & 0x10:
        diagnostics.append(
            f"mapped legacy door-trap rearm bit at source line {line} to the RoL exit-trap runtime"
        )
      return (
          f"K {dependency} {resolve('wld', room)} {direction} {state} {probability}\n",
          diagnostics,
      )

    if command == "R":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
      if room <= 0 or prototype <= 0:
        raise ValueError(f"has non-positive room/prototype {room}/{prototype}")
      return (
          f"R {dependency} {resolve('wld', room)} {resolve('obj', prototype)} "
          f"{_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "F":
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      mode, room, leader, follower = arguments[:4]
      if mode not in {0, 1, 2, 3}:
        raise ValueError(f"has unsupported follow mode {mode}")
      if room <= 0 or leader <= 0 or follower <= 0:
        raise ValueError(
            f"has non-positive room/leader/follower {room}/{leader}/{follower}"
        )
      return (
          f"F {mode} {resolve('wld', room)} {resolve('mob', leader)} "
          f"{resolve('mob', follower)} 100\n",
          diagnostics,
      )

    if command == "X":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
      if prototype <= 0:
        raise ValueError(f"has non-positive prototype {prototype}")
      combat_guard = arguments[3] if len(arguments) >= 4 else 0
      target_room = resolve("wld", room) if room >= 0 else -1
      return (
          f"X {dependency} {target_room} {resolve('mob', prototype)} "
          f"{combat_guard} {_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "T":
      if len(arguments) < 4:
        raise ValueError("requires dependency, hour, day, and weekday")
      dependency, hour, day, weekday = arguments[:4]
      month = arguments[4] if len(arguments) >= 5 else 0
      if day < 0:
        day = 0
      if weekday < 0:
        weekday = 0
      if month < 0:
        month = 0
      return f"C {dependency} {hour} {day} {weekday} {month}\n", diagnostics
  except (KeyError, ValueError) as error:
    diagnostics.append(f"excluded malformed {command} reset at source line {line}: {error}")
    return None, diagnostics

  diagnostics.append(f"excluded unsupported {command} reset at source line {line}")
  return None, diagnostics


def emit_zone(
    record: RolRecord,
    destination_vnum: int,
    destination_bottom: int,
    resolve: IdentityResolver,
) -> TransformResult:
  """Emit a target zone and its normalized reset stream."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  name, text_diagnostics = convert_text(strings.get("name") or record.identity or "RoL zone")
  diagnostics.extend(text_diagnostics)
  name = name.replace("~", "-")
  header = list(record.values.get("header", []))
  if len(header) < 4:
    return TransformResult("", ["zone header has fewer than four numeric fields"])
  destination_top = resolve("wld", int(header[0]))
  lifespan = min(240, max(0, int(header[1])))
  source_reset_mode = int(header[2])
  reset_mode = source_reset_mode if source_reset_mode in {0, 1, 2} else 1
  if source_reset_mode not in {0, 1, 2}:
    diagnostics.append(
        f"source reset mode {source_reset_mode} mapped to target occupied-zone mode 1"
    )
  source_flags = int(header[3])
  target_flags = {18}
  if source_flags & (16 | 64 | 128):
    target_flags.add(5)
  if source_flags & ~(16 | 64 | 128):
    diagnostics.append(
        f"source zone flags without target zone equivalents: {source_flags & ~(16 | 64 | 128)}"
    )

  lines = [f"#{destination_vnum}\n", "RoL conversion~\n", f"{name}~\n"]
  lines.append(
      f"{destination_bottom} {destination_top} {lifespan} {reset_mode} "
      f"{_encoded(target_flags)} -1 -1 1 0 0 0\n"
  )

  current_mobile = False
  emitted_count = 0
  for directive in record.directives:
    if directive["token"] not in {"M", "O", "P", "G", "E", "D", "R", "F", "X", "T"}:
      continue
    if directive["token"] in {"G", "E"} and not current_mobile:
      diagnostics.append(
          f"excluded {directive['token']} reset without a staged mobile host at "
          f"source line {directive['line']}"
      )
      continue
    emitted, reset_diagnostics = _emit_reset(directive, resolve)
    diagnostics.extend(reset_diagnostics)
    if emitted is not None:
      if emitted_count == 0:
        parts = emitted.split(maxsplit=2)
        if len(parts) >= 2 and parts[0] != "F" and parts[1] != "0":
          diagnostics.append(
              f"normalized the first reset dependency {parts[1]} to 0 at source line "
              f"{directive['line']}"
          )
          parts[1] = "0"
          emitted = " ".join(parts)
      lines.append(emitted)
      emitted_count += 1
    if directive["token"] == "M":
      current_mobile = emitted is not None
  lines.extend(["S\n", "$\n"])
  return TransformResult("".join(lines), diagnostics)
