"""RoL mobiles source grammar and target conversion."""

from __future__ import annotations

import json
import re
from functools import lru_cache
from pathlib import Path
from typing import Any

from .rol_conversion_types import RolRecord, RolSourceCorpus, TransformResult
from .rol_mob_calculator import (
    MobileCalculatorClient,
    calculation_to_dict,
    default_mobile_calculator,
)
from .rol_mobile_identity import load_mobile_conversion_policy, select_mobile_conversion
from .rol_source_common import _diagnostic, _new_record, _next_content, _read_tilde, _segments
from .rol_transform_common import _encoded, _mapped_bits, _source_mask_bits, _tilde
from .source import SourceFile, SourceLine


_MOBILE_REPAIR_POLICY: dict[tuple[str, int, str], frozenset[str]] | None = None


def _mobile_repair_policy() -> dict[tuple[str, int, str], frozenset[str]]:
  """Read exact syntax-repair ownership without mutating the parsed source record."""

  global _MOBILE_REPAIR_POLICY
  if _MOBILE_REPAIR_POLICY is None:
    policy_path = Path(__file__).resolve().parents[1] / "rol_conversion_policy.json"
    try:
      policy = json.loads(policy_path.read_text(encoding="ascii"))["mobile"]
      _MOBILE_REPAIR_POLICY = {
          (str(rule["basename"]), int(rule["source_vnum"]), str(rule["source_sha256"])):
          frozenset(
              key
              for key in (
                  "repair_race_row",
                  "repair_position_row",
                  "repair_level",
                  "repair_sex",
                  "ignored_money_tokens",
              )
              if key in rule
          )
          for rule in policy.get("exact_records", [])
      }
    except (OSError, KeyError, TypeError, ValueError, json.JSONDecodeError) as error:
      raise RuntimeError(f"cannot load versioned mobile repair policy: {error}") from error
  return _MOBILE_REPAIR_POLICY


def _mobile_repair_owns(record: RolRecord, key: str) -> bool:
  return key in _mobile_repair_policy().get(
      (record.basename, record.vnum, record.sha256), frozenset()
  )


def _mobile_row_diagnostic(
    corpus: RolSourceCorpus,
    record: RolRecord,
    line: SourceLine,
    repair_key: str,
    message: str,
) -> None:
  repaired = _mobile_repair_owns(record, repair_key)
  if not repaired:
    record.complete = False
  _diagnostic(
      corpus,
      "ROLMOB005",
      "warning" if repaired else "error",
      f"{message}; {'owned by exact automatic repair' if repaired else 'no exact repair owns it'}",
      line,
      "mob",
      record.vnum,
  )


def _parse_mob(
    source: SourceFile,
    basename: str,
    corpus: RolSourceCorpus,
) -> list[RolRecord]:
  records: list[RolRecord] = []
  version_line = source.lines[0].text.strip() if source.lines else "missing"
  version_match = re.search(r"File Version\s+(\d+)", version_line)
  version: str | int = int(version_match.group(1)) if version_match else version_line
  corpus.file_versions[("mob", str(version))] += 1
  for start, end, vnum in _segments(source):
    position = start + 1
    position, peek = _next_content(source.lines, position, end)
    if peek is not None and peek.raw.strip().startswith(b"$"):
      corpus.file_terminators[("mob", "sentinel")] += 1
      continue
    position = start + 1
    record = _new_record(source, basename, "mob", start, end, vnum)
    strings: list[str | None] = []
    strings_ok = True
    for _ in range(4):
      position, value, ok = _read_tilde(source.lines, position, end)
      strings.append(value)
      strings_ok = strings_ok and ok
    record.identity = strings[1]
    record.format_version = version
    record.values["strings"] = {
        "aliases": strings[0],
        "short_description": strings[1],
        "long_description": strings[2],
        "description": strings[3],
    }
    position, flags_line = _next_content(source.lines, position, end)
    flag_tokens = flags_line.text.split() if flags_line is not None else []
    if not strings_ok or flags_line is None or len(flag_tokens) < 5:
      record.complete = False
      _diagnostic(corpus, "ROLMOB001", "error", "mobile strings or flag row are incomplete", source.lines[start], "mob", vnum)
      records.append(record)
      continue
    letter = flag_tokens[4]
    record.values["format_letter"] = letter
    record.values["flags"] = flag_tokens
    record.directives.append({"token": "FLAGS", "line": flags_line.number, "field_count": len(flag_tokens)})
    if letter != "S":
      record.complete = False
      _diagnostic(corpus, "ROLMOB002", "error", f"unsupported mobile format letter {letter!r}", flags_line, "mob", vnum)
    base_rows: list[list[str]] = []
    base_lines: list[SourceLine] = []
    for token in ("RACE", "COMBAT", "MONEY", "POSITION"):
      position, line = _next_content(source.lines, position, end)
      if line is None:
        repair_key = "repair_position_row" if token == "POSITION" else ""
        if repair_key and _mobile_repair_owns(record, repair_key):
          _diagnostic(
              corpus,
              "ROLMOB003",
              "warning",
              f"source mobile lacks its {token.lower()} row; owned by exact automatic repair",
              source.lines[start],
              "mob",
              vnum,
          )
        else:
          record.complete = False
          _diagnostic(
              corpus,
              "ROLMOB003",
              "error",
              f"source mobile lacks its {token.lower()} row and has no exact repair",
              source.lines[start],
              "mob",
              vnum,
          )
        break
      values = line.text.split()
      base_rows.append(values)
      base_lines.append(line)
      record.directives.append({"token": token, "line": line.number, "field_count": len(values)})
    record.values["base_rows"] = base_rows
    if len(base_rows) >= 1:
      race = base_rows[0]
      valid_race = (
          len(race) in {3, 4}
          and re.fullmatch(r"[A-Za-z0-9]+", race[0]) is not None
          and all(re.fullmatch(r"[+-]?\d+", value) is not None for value in race[1:3])
          and (len(race) == 3 or re.fullmatch(r"[A-Za-z0-9.-]+", race[3]) is not None)
      )
      if not valid_race:
        _mobile_row_diagnostic(
            corpus, record, base_lines[0], "repair_race_row",
            "race row requires code, integer height/weight, and optional period-list aggression",
        )
    if len(base_rows) >= 2:
      combat = base_rows[1]
      valid_combat = (
          len(combat) == 5
          and all(re.fullmatch(r"[+-]?\d+", value) is not None for value in combat[:3])
          and all(
              re.fullmatch(r"[+-]?\d+d[+-]?\d+\+[+-]?\d+", value) is not None
              for value in combat[3:]
          )
      )
      if not valid_combat:
        _mobile_row_diagnostic(
            corpus, record, base_lines[1], "repair_combat_row",
            "combat row requires level, hitroll, armor, HP dice, and damage dice",
        )
      elif int(combat[0]) <= 0:
        _mobile_row_diagnostic(
            corpus, record, base_lines[1], "repair_level",
            "generic mobile grammar requires a positive combat level",
        )
    if len(base_rows) >= 3:
      money = base_rows[2]
      money_token = r"(?:[+-]?\d+|\d+\.\d+\.\d+\.\d+)"
      valid_money = (
          len(money) == 2
          and re.fullmatch(money_token, money[0]) is not None
          and re.fullmatch(r"[+-]?\d+", money[1]) is not None
      )
      if not valid_money:
        _mobile_row_diagnostic(
            corpus, record, base_lines[2], "ignored_money_tokens",
            "money row requires exactly money and experience",
        )
    if len(base_rows) >= 4:
      position_values = base_rows[3]
      valid_position = len(position_values) in {3, 4, 5, 6} and all(
          re.fullmatch(r"[+-]?\d+", value) is not None for value in position_values
      )
      if not valid_position:
        _mobile_row_diagnostic(
            corpus, record, base_lines[3], "repair_position_row",
            "position row requires three through six integers",
        )
      elif not 0 <= int(position_values[2]) <= 2:
        _mobile_row_diagnostic(
            corpus, record, base_lines[3], "repair_sex",
            "mobile sex must be in the source range 0 through 2",
        )
    position, extra = _next_content(source.lines, position, end)
    if extra is not None and not extra.raw.strip().startswith(b"$"):
      record.complete = False
      _diagnostic(corpus, "ROLMOB004", "error", "unconsumed mobile record content", extra, "mob", vnum)
    records.append(record)
  return records


@lru_cache(maxsize=1)
def _mobile_conversion_inputs():
  return load_mobile_conversion_policy()


def _manifest_bit(manifest: dict[str, Any], table: str, macro: str) -> int:
  for entry in manifest["tables"][table]["entries"]:
    if entry.get("macro") == macro:
      return int(entry["index"])
  raise ValueError(f"constants manifest table {table!r} has no {macro}")


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
    "Z": frozenset({122}),  # RACE_ANGEL
    "OB": frozenset({126}),  # RACE_BEHOLDER
}


# Call lycanthrope chooses only the two source prototypes named by its live
# handler. Preserve that role independently of their broader lycanthrope race.
MOB_SOURCE_VNUM_IDENTITY_ACTIONS = {
    525: frozenset({127}),
    526: frozenset({127}),
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


def _numeric_bitarray(bits: set[int], width: int = 4) -> str:
  values = [0] * width
  for bit in bits:
    if 0 <= bit < width * 32:
      values[bit // 32] |= 1 << (bit % 32)
  return " ".join(str(value) for value in values)


def emit_mobile(
    record: RolRecord,
    destination_vnum: int,
    special_proc: str | None = None,
    special_resolved: bool = False,
    attachments: tuple[int, ...] = (),
    required_action_bits: tuple[int, ...] = (),
    required_affect_bits: tuple[int, ...] = (),
    calculator: MobileCalculatorClient | None = None,
    scripted_path: bool = False,
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
  if scripted_path:
    # Source AFF_PATH excludes ordinary wandering while the autonomous route owns movement.
    target_actions.add(_manifest_bit(manifest, "mob", "MOB_SENTINEL"))
  automatic_actions, automatic_affects = mobile_automatic_race_flags(record)
  target_actions.update(automatic_actions)
  target_actions.update(MOB_SOURCE_RACE_IDENTITY_ACTIONS.get(race_code, frozenset()))
  target_actions.update(MOB_SOURCE_VNUM_IDENTITY_ACTIONS.get(record.vnum, frozenset()))
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
