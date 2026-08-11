"""Semantic transforms from Realms of Luminari records to Luminari data."""

from __future__ import annotations

from dataclasses import dataclass, field
import re
from typing import Callable

from .flags import encode_bits
from .rol_source import RolRecord


_TARGET_MAGIC_ITEM_TYPES = frozenset({2, 3, 4, 10})
_TARGET_MAX_LEVEL = 34
_TARGET_MAX_OBJECT_SPELL_LEVEL = _TARGET_MAX_LEVEL
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
}

MOB_ACTION_MAP = {
    2: 1,   # SENTINEL
    3: 2,   # SCAVENGER
    6: 5,   # AGGRESSIVE
    7: 6,   # STAY_ZONE
    8: 7,   # WIMPY
    9: 8,   # AGGRESSIVE_EVIL
    10: 9,  # AGGRESSIVE_GOOD
    11: 10, # AGGRESSIVE_NEUTRAL
    12: 11, # MEMORY
    15: 12, # HELPER
    18: 18, # NOKILL
    25: 32, # CITIZEN/WITNESS
    28: 21, # MOUNTABLE
    31: 33, # HUNTER
}

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

OBJECT_TYPE_MAP = {
    0: 12,
    1: 1,
    2: 2,
    3: 3,
    4: 4,
    5: 5,
    6: 7,
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
    32: 38,
    33: 28,
    34: 14,
    35: 44,
    36: 45,
    37: 25,
    38: 12,
    39: 42,
    40: 12,
}

OBJECT_EXTRA_MAP = {
    0: 0,
    3: 16,
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
    18: 40,
    19: 43,
    23: 2,
    25: 15,
    26: 13,
    27: 14,
    28: 12,
}

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
    17: 17,
    18: 18,
    19: 19,
    20: 20,
    21: 20,
    22: 21,
    23: 21,
    24: 22,
    25: 28,
    28: 6,
    51: 25,
    52: 13,
}

EQUIPMENT_POSITION_MAP = {
    **{position: position for position in range(18)},
    18: 26,  # eyes
    19: 22,  # face
    20: 24,  # right ear
    21: 25,  # left ear
    22: 23,  # quiver/ammo pouch
    23: 27,  # badge/insignia
}

CLASS_MAP = {
    0: 0,
    1: 3,
    2: 9,
    3: 6,
    4: 8,
    5: 24,
    6: 1,
    7: 4,
    8: 5,
    9: 1,
    10: 7,
    11: 29,
    12: 0,
    13: 2,
    14: 25,
    15: 3,
    16: 10,
    17: 21,
    18: 29,
    19: 0,
    20: 0,
    21: 0,
    22: 10,
    23: 2,
    24: 0,
    25: 3,
}

RACE_CODE_MAP = {
    "A": 3,
    "AA": 3,
    "AB": 3,
    "AC": 3,
    "AD": 3,
    "AE": 3,
    "AF": 3,
    "AH": 3,
    "AP": 15,
    "AS": 15,
    "AY": 10,
    "B": 3,
    "BR": 3,
    "D": 4,
    "DK": 4,
    "E1": 13,
    "E2": 13,
    "EA": 8,
    "EE": 8,
    "EF": 8,
    "EW": 8,
    "F": 3,
    "G": 5,
    "H": 1,
    "H2": 1,
    "HC": 11,
    "HF": 9,
    "HG": 1,
    "HH": 1,
    "HK": 1,
    "HO": 1,
    "HS": 11,
    "HY": 13,
    "I": 15,
    "IX": 6,
    "K": 10,
    "L": 16,
    "MH": 6,
    "MS": 11,
    "N": 0,
    "OB": 6,
    "OG": 7,
    "OH": 11,
    "OP": 13,
    "OS": 12,
    "OU": 14,
    "P2": 1,
    "PB": 1,
    "PD": 1,
    "PE": 1,
    "PF": 1,
    "PG": 1,
    "PH": 1,
    "PI": 6,
    "PL": 1,
    "PM": 1,
    "PO": 1,
    "PT": 1,
    "PR": 1,
    "PS": 14,
    "PY": 11,
    "PZ": 2,
    "R": 3,
    "RH": 11,
    "RS": 3,
    "RT": 11,
    "U": 2,
    "UG": 2,
    "UH": 2,
    "US": 2,
    "UV": 2,
    "VT": 14,
    "X": 13,
    "Y": 13,
    "Z": 13,
}


_SOURCE_COLOR = re.compile(r"&\+([A-Za-z])|&([Nn])")


def convert_text(value: str | None) -> tuple[str, list[str]]:
  """Convert legacy colors and return canonical ASCII/LF target text."""

  diagnostics: list[str] = []
  text = value or ""
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


def _source_mask_bits(mask: int, logical_offset: int) -> set[int]:
  return {
      logical_offset + bit
      for bit in range(32)
      if mask & (1 << bit)
  }


def _mapped_bits(source_bits: set[int], mapping: dict[int, int]) -> set[int]:
  return {mapping[bit] for bit in source_bits if bit in mapping}


def _unmapped(source_bits: set[int], mapping: dict[int, int]) -> list[int]:
  return sorted(source_bits - mapping.keys())


def _encoded(bits: set[int]) -> str:
  return " ".join(encode_bits(bits))


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
  first_name = text.find("%s")
  text = text[: first_name + 2] + text[first_name + 2 :].replace("%s", "you")
  text = re.sub(r"%(?![%sd])", "%%", text)
  return text, diagnostics


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
  profit_sell = 100.0 / max(1, 100 + source_profit)

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

  shop_flags = 0
  if _directive_rows(record, "KILLABLE"):
    shop_flags |= 1
  if _directive_rows(record, "ROAMING"):
    shop_flags |= 1 << 5

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

  unsupported = sorted(
      {
          directive["token"]
          for directive in record.directives
          if directive["token"] in {"CHEATS", "HATES", "DEADBEAT", "OFFENSE", "MOPEN", "MCLOSE", "MBIGOT"}
      }
  )
  if unsupported:
    diagnostics.append(
        "source-only shop behavior retained as conversion evidence: " + ", ".join(unsupported)
    )

  lines = [f"#{destination_vnum}~\n"]
  lines.extend(f"{value}\n" for value in products)
  lines.extend(["-1\n", f"{profit_buy:.4f}\n", f"{profit_sell:.4f}\n"])
  lines.extend(f"{value}\n" for value in buy_types)
  lines.append("-1\n")
  lines.extend(f"{message}~\n" for message in messages)
  lines.extend(["0\n", f"{shop_flags}\n", f"{keeper}\n", "0\n"])
  lines.extend(f"{value}\n" for value in rooms)
  lines.append("-1\n")
  for first, last in intervals:
    lines.extend([f"{first}\n", f"{last}\n"])
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
      keyword, text_diagnostics = _tilde(str(payload.get("keyword", "")))
      diagnostics.extend(text_diagnostics)
      message, text_diagnostics = _tilde(str(payload.get("message", "")))
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
    reply_text, text_diagnostics = _tilde(reply)
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
    attachments: tuple[int, ...] = (),
    source_zone_flags: int = 0,
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
  sector = SECTOR_MAP.get(base[2] if len(base) > 2 else 0, 0)
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
        diagnostics.append(
            f"legacy exit trap payload retained only as conversion evidence at source line {directive['line']}"
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
  lines.extend(["C\n", "0 0\n", "S\n"])
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)


def _source_position(value: int) -> int:
  if value & 4:
    return 0
  if value & 8:
    return 1
  if value & 16:
    return 2
  if value & 32:
    return 4
  if value & 64:
    return 6
  posture = value & 3
  return {0: 5, 1: 7, 2: 7, 3: 9}.get(posture, 9)


def _dice_row(tokens: list[str]) -> tuple[int, int, int, str, str]:
  padded = tokens + ["0"] * max(0, 5 - len(tokens))
  return (
      int(padded[0]),
      int(padded[1]),
      int(padded[2]),
      _normalize_dice(padded[3]),
      _normalize_dice(padded[4]),
  )


def _normalize_dice(value: str) -> str:
  match = re.fullmatch(r"([+-]?\d+)d([+-]?\d+)\+([+-]?\d+)", value)
  if match is None:
    return "1d1+0"
  count, size, bonus = (int(item) for item in match.groups())
  if count <= 0 or size <= 0:
    return f"0d0+{bonus}"
  return f"{count}d{size}+{bonus}"


def _money_row(tokens: list[str]) -> tuple[int, int]:
  if not tokens:
    return 0, 0
  if "." in tokens[0]:
    pieces = [int(value) for value in tokens[0].split(".")]
    pieces = (pieces + [0, 0, 0, 0])[:4]
    money = pieces[0] + pieces[1] * 10 + pieces[2] * 100 + pieces[3] * 1000
  else:
    money = int(tokens[0])
  return money, int(tokens[1]) if len(tokens) > 1 else 0


def emit_mobile(
    record: RolRecord,
    destination_vnum: int,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
) -> TransformResult:
  """Emit one enhanced target mobile record."""

  diagnostics: list[str] = []
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
  target_actions = _mapped_bits(source_actions, MOB_ACTION_MAP) | {3}
  if special_proc is not None:
    target_actions.add(0)
  target_affects = _mapped_bits(source_affects, MOB_AFFECT_MAP)
  missing_actions = _unmapped(source_actions, MOB_ACTION_MAP)
  missing_affects = _unmapped(source_affects, MOB_AFFECT_MAP)
  if missing_actions:
    diagnostics.append(f"mobile action flags requiring behavior reconciliation: {missing_actions}")
  if missing_affects:
    diagnostics.append(f"mobile affect flags without persistent equivalents: {missing_affects}")
  lines.append(f"{_encoded(target_actions)} {_encoded(target_affects)} {alignment} E\n")

  rows = record.values.get("base_rows", [])
  race_row = rows[0] if len(rows) > 0 else ["N", "0", "0"]
  combat_row = rows[1] if len(rows) > 1 else ["1", "0", "0", "1d1+0", "1d1+0"]
  money_row = rows[2] if len(rows) > 2 else ["0", "0"]
  position_row = rows[3] if len(rows) > 3 else ["131", "131", "0", "0"]
  level, hitroll, armor, hit_dice, damage_dice = _dice_row(combat_row)
  level = min(34, max(1, level))
  lines.append(f"{level} {hitroll} {armor} {hit_dice} {damage_dice}\n")
  money, experience = _money_row(money_row)
  lines.append(f"{money} {experience}\n")
  position = _source_position(int(position_row[0]))
  default_position = _source_position(int(position_row[1]))
  sex = int(position_row[2]) if len(position_row) > 2 else 0
  lines.append(f"{position} {default_position} {sex}\n")

  source_class = int(position_row[3]) if len(position_row) > 3 else 0
  target_class = CLASS_MAP.get(source_class, 0)
  race_code = race_row[0].upper() if race_row else "N"
  target_race = RACE_CODE_MAP.get(race_code, 0)
  if race_code not in RACE_CODE_MAP:
    diagnostics.append(f"unknown source race code {race_code!r}; used target human")
  lines.extend([f"Class: {target_class}\n", f"Race: {target_race}\n"])
  if special_proc is not None:
    lines.append(f"SpecProc: {special_proc}\n")
  lines.append("E\n")
  lines.extend(f"T {trigger_vnum}\n" for trigger_vnum in attachments)
  return TransformResult("".join(lines), diagnostics)


def _object_values(
    record: RolRecord,
    source_type: int,
    target_type: int,
    resolve: IdentityResolver,
    diagnostics: list[str],
) -> list[int]:
  values = list(record.values.get("values", []))
  values = (values + [0] * 16)[:16]
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
  if source_type == 15 and values[2] > 0:
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
  if target_type == 15 and values[2] == 65535:
    values[2] = -1
  return values


def emit_object(
    record: RolRecord,
    destination_vnum: int,
    resolve: IdentityResolver,
    special_proc: str | None = None,
    attachments: tuple[int, ...] = (),
    required_extra_bits: tuple[int, ...] = (),
) -> TransformResult:
  """Emit one modern target object record."""

  diagnostics: list[str] = []
  strings = record.values.get("strings", {})
  lines = [f"#{destination_vnum}\n"]
  for key in ("aliases", "short_description", "description", "action_description"):
    value, text_diagnostics = _tilde(strings.get(key))
    diagnostics.extend(text_diagnostics)
    lines.append(value)

  source_type = int(record.values.get("item_type") or 0)
  target_type = OBJECT_TYPE_MAP.get(source_type, 12)
  if source_type not in OBJECT_TYPE_MAP:
    diagnostics.append(f"unknown source item type {source_type}; used ITEM_OTHER")
  source_flags = record.values.get("flags", [])
  extra_mask = source_flags[1] if len(source_flags) > 1 else 0
  wear_mask = source_flags[2] if len(source_flags) > 2 else 0
  source_extra = _source_mask_bits(extra_mask, 0)
  source_wear = _source_mask_bits(wear_mask, 0)
  target_extra = _mapped_bits(source_extra, OBJECT_EXTRA_MAP) | set(required_extra_bits)
  target_wear = _mapped_bits(source_wear, OBJECT_WEAR_MAP)
  missing_extra = _unmapped(source_extra, OBJECT_EXTRA_MAP)
  missing_wear = _unmapped(source_wear, OBJECT_WEAR_MAP)
  if missing_extra:
    diagnostics.append(f"object extra flags without direct equivalents: {missing_extra}")
  if missing_wear:
    diagnostics.append(f"object wear flags without direct equivalents: {missing_wear}")

  source_affects: set[int] = set()
  for directive in record.directives:
    if directive["token"] != "AFFECT_FLAGS":
      continue
    for ordinal, mask in enumerate(directive.get("arguments", [])):
      source_affects.update(_source_mask_bits(mask, ordinal * 32 + 1))
  target_affects = _mapped_bits(source_affects, MOB_AFFECT_MAP)
  missing_affects = _unmapped(source_affects, MOB_AFFECT_MAP)
  if missing_affects:
    diagnostics.append(f"object affect flags without persistent equivalents: {missing_affects}")
  values = _object_values(record, source_type, target_type, resolve, diagnostics)
  trap_values = _object_trap_values(record, values, diagnostics)
  if trap_values is not None:
    target_extra.add(ROL_OBJECT_TRAP_EXTRA_BIT)
    values[ROL_OBJECT_TRAP_VALUE_OFFSET:ROL_OBJECT_TRAP_VALUE_OFFSET + 6] = trap_values
  lines.append(
      f"{target_type} {_encoded(target_extra)} {_encoded(target_wear)} "
      f"{_encoded(target_affects)} {_encoded(set())}\n"
  )
  lines.append(" ".join(str(value) for value in values) + "\n")
  economy = list(record.values.get("economy", []))
  economy_defaults = [0, 1, 0, 1, 1]
  economy = economy[:5] + economy_defaults[len(economy[:5]):]
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
      location = APPLY_MAP.get(arguments[0])
      if location is None or location == 0 and arguments[0] != 0:
        diagnostics.append(
            f"excluded unsupported object apply {arguments[0]} at source line {directive['line']}"
        )
        continue
      modifier = arguments[1]
      if 1 <= arguments[0] <= 5:
        modifier = (modifier * 45) // 10
      lines.extend(["A\n", f"{location} {modifier} 0 0\n"])
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

  try:
    if command in {"M", "O", "P", "E"}:
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, prototype, maximum, destination = arguments[:4]
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
      return (
          f"G {dependency} {resolve('obj', prototype)} {maximum} "
          f"{_reset_probability(command, arguments)}\n",
          diagnostics,
      )

    if command == "D":
      if len(arguments) < 4:
        raise ValueError("requires four leading arguments")
      dependency, room, direction, state = arguments[:4]
      probability = _reset_probability(command, arguments)
      if 0 <= state <= 2 and probability == 100:
        return f"D {dependency} {resolve('wld', room)} {direction} {state}\n", diagnostics
      if state & 0x10:
        diagnostics.append(
            f"excluded legacy door-trap activation bit at source line {line}; "
            "the target exit trap runtime has no equivalent payload"
        )
        state &= ~0x10
      return (
          f"K {dependency} {resolve('wld', room)} {direction} {state} {probability}\n",
          diagnostics,
      )

    if command == "R":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
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
      return (
          f"F {mode} {resolve('wld', room)} {resolve('mob', leader)} "
          f"{resolve('mob', follower)} 100\n",
          diagnostics,
      )

    if command == "X":
      if len(arguments) < 3:
        raise ValueError("requires three leading arguments")
      dependency, room, prototype = arguments[:3]
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
  target_flags = {5} if source_flags & (16 | 64 | 128) else set()
  if source_flags & ~(16 | 64 | 128):
    diagnostics.append(
        f"source zone flags without target zone equivalents: {source_flags & ~(16 | 64 | 128)}"
    )

  lines = [f"#{destination_vnum}\n", "RoL conversion~\n", f"{name}~\n"]
  if target_flags:
    lines.append(
        f"{destination_bottom} {destination_top} {lifespan} {reset_mode} "
        f"{_encoded(target_flags)} -1 -1 1 0 0 0\n"
    )
  else:
    lines.append(f"{destination_bottom} {destination_top} {lifespan} {reset_mode}\n")

  current_mobile = False
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
      lines.append(emitted)
    if directive["token"] == "M":
      current_mobile = emitted is not None
  lines.extend(["S\n", "$\n"])
  return TransformResult("".join(lines), diagnostics)
