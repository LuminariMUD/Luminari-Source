"""Deterministic RoL mobile identity, competence, size, class, and tier selection."""

from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import re
from typing import Any

from .constants import default_repo_root, load_manifest
from .rol_source import RolRecord, normalize_identity


class MobileConversionError(RuntimeError):
  """Raised when a mobile cannot be resolved without an unsafe fallback."""


@dataclass(frozen=True, slots=True)
class SourceRace:
  code: str
  macro: str
  source_index: int
  source_name: str
  average_height: int
  average_weight: int
  magic_resistance_cap: int


@dataclass(frozen=True, slots=True)
class MobileIdentityDecision:
  source_code: str
  source_race_name: str
  target_race: int
  target_race_symbol: str
  subraces: tuple[int, int, int]
  subrace_symbols: tuple[str, str, str]
  final_size: int
  final_size_symbol: str
  size_evidence: str
  rule_id: str
  rule_set_version: str
  status: str
  confidence: str
  evidence: tuple[str, ...]
  contradictions: tuple[str, ...]


@dataclass(frozen=True, slots=True)
class MobileSelection:
  source_record_id: str
  source_vnum: int
  mapped_level: int
  target_class: int
  target_class_symbol: str
  target_sex: int
  current_position: int
  default_position: int
  tier: int
  tier_symbol: str
  tier_rule_id: str
  tier_evidence: tuple[str, ...]
  identity: MobileIdentityDecision
  source_aggression_codes: tuple[str, ...]
  ignored_aggression_tokens: tuple[str, ...]
  custom_profile: str | None
  custom_hit_points: int | None
  effective_source_spell_resistance: int
  source_money: str
  source_experience: int
  source_prestige_bonus: int
  repairs: tuple[str, ...]

  def ledger(self) -> dict[str, Any]:
    identity = self.identity
    aggression_tokens = (*self.source_aggression_codes, *self.ignored_aggression_tokens)
    aggression = {
        "disposition": "EXCLUDED" if aggression_tokens else "EXACT",
        "source_codes": list(self.source_aggression_codes),
        "source_ignored_tokens": list(self.ignored_aggression_tokens),
        "reason": (
            "target has no race-list aggression primitive; broad aggression would attack "
            "unintended creatures"
            if aggression_tokens
            else "source record has no race-list aggression"
        ),
        "player_impact": (
            "the mobile does not automatically initiate combat solely from the source race list"
            if aggression_tokens
            else "none"
        ),
    }
    return {
        "source_record_id": self.source_record_id,
        "source_vnum": self.source_vnum,
        "level": {
            "disposition": "MAPPED",
            "target": self.mapped_level,
            "rule": "level-map-v1",
        },
        "class": {
            "disposition": "MAPPED",
            "target_symbol": self.target_class_symbol,
            "target": self.target_class,
            "rule": "class-map-v1",
        },
        "sex": {
            "disposition": "REPAIRED" if any("sex" in repair for repair in self.repairs) else "EXACT",
            "target": self.target_sex,
        },
        "positions": {
            "disposition": "MAPPED",
            "current": self.current_position,
            "default": self.default_position,
            "rule": "source-position-bitfield-v1",
        },
        "tier": {
            "disposition": "MAPPED",
            "target_symbol": self.tier_symbol,
            "target": self.tier,
            "rule": self.tier_rule_id,
            "evidence": list(self.tier_evidence),
        },
        "identity": {
            "disposition": "MAPPED",
            "source_code": identity.source_code,
            "source_race_name": identity.source_race_name,
            "target_race_symbol": identity.target_race_symbol,
            "target_race": identity.target_race,
            "subrace_symbols": list(identity.subrace_symbols),
            "subraces": list(identity.subraces),
            "size_symbol": identity.final_size_symbol,
            "size": identity.final_size,
            "size_evidence": identity.size_evidence,
            "rule": identity.rule_id,
            "rule_set_version": identity.rule_set_version,
            "status": identity.status,
            "confidence": identity.confidence,
            "evidence": list(identity.evidence),
            "contradictions": list(identity.contradictions),
        },
        "aggression_race_list": aggression,
        "spell_resistance": {
            "disposition": "MAPPED",
            "target": self.effective_source_spell_resistance,
            "rule": "source-effective-spell-resistance-v1",
            "reason": "persist the source loader's effective value as one target override",
        },
        "gold": {
            "disposition": "ADAPTED",
            "source": self.source_money,
            "rule": "target-autoroll-reward-v1",
            "reason": "target-native calculator reward with explicit MOB_CUSTOM_GOLD ownership",
            "player_impact": "replaces source per-spawn denomination randomization",
        },
        "experience": {
            "disposition": "ADAPTED",
            "source": self.source_experience,
            "rule": "target-autoroll-reward-v1",
            "reason": "target competence and selected tier own kill experience",
            "player_impact": "prevents raw source and target reward policies from stacking",
        },
        "prestige": {
            "disposition": "EXCLUDED",
            "source": self.source_prestige_bonus,
            "rule": "source-prestige-exclusion-v1",
            "reason": "Luminari has no equivalent mobile prestige kill-reward field",
            "player_impact": (
                "positive source prestige is not awarded"
                if self.source_prestige_bonus > 0
                else "none; non-positive source values granted no reward"
            ),
        },
        "custom_profile": self.custom_profile,
        "custom_hit_points": self.custom_hit_points,
        "repairs": list(self.repairs),
    }


def _sha256(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as handle:
    for block in iter(lambda: handle.read(1024 * 1024), b""):
      digest.update(block)
  return digest.hexdigest()


def _bounded_c_block(text: str, declaration: str) -> str:
  start = text.find(declaration)
  if start < 0:
    raise MobileConversionError(f"cannot find source registry declaration {declaration!r}")
  opening = text.find("{", start)
  closing = text.find("};", opening)
  if opening < 0 or closing < 0:
    raise MobileConversionError(f"unterminated source registry declaration {declaration!r}")
  return text[opening + 1 : closing]


def load_source_race_registry(repo_root: Path, mobile_policy: dict[str, Any]) -> dict[str, SourceRace]:
  registry_policy = mobile_policy["source_registry"]
  source_path = repo_root / registry_policy["path"]
  actual_hash = _sha256(source_path)
  if actual_hash != registry_policy["sha256"]:
    raise MobileConversionError(
        f"source race registry hash {actual_hash} does not match policy "
        f"{registry_policy['sha256']}"
    )
  text = source_path.read_text(encoding="ascii")
  header = source_path.with_name("race_class.h").read_text(encoding="ascii")
  race_values = {
      macro: int(value)
      for macro, value in re.findall(r"^#define\s+(RACE_[A-Z0-9_]+)\s+(\d+)\b", header, re.MULTILINE)
  }
  resistance_caps_by_index: dict[int, int] = {}
  for macro, cap in re.findall(
      r"\{\s*(RACE_[A-Z0-9_]+)\s*,\s*(\d+)\s*\}",
      _bounded_c_block(text, "racial_mods[][2]"),
  ):
    if macro not in race_values:
      raise MobileConversionError(f"source racial_mods uses unknown macro {macro}")
    resistance_caps_by_index[race_values[macro]] = int(cap)
  names = re.findall(r'"((?:\\.|[^"\\])*)"', _bounded_c_block(text, "race_types[]"))
  if names and names[-1] == r"\n":
    names.pop()
  size_rows = [
      tuple(int(value) for value in re.findall(r"-?\d+", row))
      for row in re.findall(r"\{([^{}]+)\}", _bounded_c_block(text, "race_size[][12]"))
  ]
  lookup_rows = re.findall(
      r'\{\s*(RACE_[A-Z0-9_]+)\s*,\s*"([A-Z0-9]+)"\s*\}',
      _bounded_c_block(text, "race_lookup_table[DEFINED_RACES]"),
  )
  registry: dict[str, SourceRace] = {}
  for macro, code in lookup_rows:
    if macro not in race_values:
      raise MobileConversionError(f"source race lookup uses unknown macro {macro}")
    index = race_values[macro]
    if index >= len(names) or index >= len(size_rows) or len(size_rows[index]) != 12:
      raise MobileConversionError(f"source race {macro} index {index} has no complete metadata")
    if code in registry:
      raise MobileConversionError(f"duplicate source race code {code}")
    dimensions = size_rows[index]
    registry[code] = SourceRace(
        code=code,
        macro=macro,
        source_index=index,
        source_name=names[index],
        average_height=max(dimensions[1], dimensions[7]),
        average_weight=max(dimensions[4], dimensions[10]),
        magic_resistance_cap=resistance_caps_by_index.get(index, 0),
    )
  expected = int(registry_policy["expected_code_count"])
  if len(registry) != expected:
    raise MobileConversionError(
        f"source race registry contains {len(registry)} codes; policy requires {expected}"
    )
  return registry


def load_mobile_conversion_policy(
    repo_root: Path | None = None,
) -> tuple[dict[str, Any], dict[str, Any], dict[str, SourceRace]]:
  root = (repo_root or default_repo_root()).resolve()
  policy = json.loads((root / "scripts/world/rol_conversion_policy.json").read_text(encoding="ascii"))
  mobile_policy = policy.get("mobile")
  if not isinstance(mobile_policy, dict) or mobile_policy.get("schema_version") != 1:
    raise MobileConversionError("conversion policy has no supported mobile section")
  manifest = load_manifest(root / "scripts/world/wtool_constants.json")
  registry = load_source_race_registry(root, mobile_policy)
  _validate_policy(mobile_policy, manifest, registry)
  return mobile_policy, manifest, registry


def _symbols(manifest: dict[str, Any], group: str) -> dict[str, int]:
  try:
    return {key: int(value) for key, value in manifest["symbols"][group]["values"].items()}
  except (KeyError, TypeError, ValueError) as error:
    raise MobileConversionError(f"constants manifest has no valid {group!r} symbols") from error


def _validate_profile(profile: dict[str, Any], manifest: dict[str, Any]) -> None:
  race_symbols = _symbols(manifest, "race-types")
  subrace_symbols = _symbols(manifest, "subraces")
  size_symbols = _symbols(manifest, "sizes")
  if profile.get("race") not in race_symbols:
    raise MobileConversionError(f"mobile rule uses unknown race symbol {profile.get('race')!r}")
  subraces = profile.get("subraces", [])
  if not isinstance(subraces, list) or len(subraces) > 3 or len(set(subraces)) != len(subraces):
    raise MobileConversionError("mobile rule subraces must be a unique list of at most three symbols")
  for symbol in subraces:
    if symbol not in subrace_symbols or symbol == "SUBRACE_UNKNOWN":
      raise MobileConversionError(f"mobile rule uses invalid meaningful subrace {symbol!r}")
  for key in ("size", "minimum_size", "fallback_size"):
    if key in profile and profile[key] not in size_symbols:
      raise MobileConversionError(f"mobile rule uses unknown size symbol {profile[key]!r}")


def _validate_policy(
    policy: dict[str, Any], manifest: dict[str, Any], registry: dict[str, SourceRace]
) -> None:
  assigned: dict[str, str] = {}
  for index, profile in enumerate(policy.get("base_profiles", [])):
    _validate_profile(profile, manifest)
    for code in profile.get("codes", []):
      if code in assigned:
        raise MobileConversionError(
            f"source race code {code} occurs in base profiles {assigned[code]} and {index}"
        )
      assigned[code] = str(index)
  if set(assigned) != set(registry):
    missing = sorted(set(registry) - set(assigned))
    extra = sorted(set(assigned) - set(registry))
    raise MobileConversionError(f"base profile registry mismatch; missing={missing}, extra={extra}")
  rule_ids: set[str] = set()
  for rule in [*policy.get("phrase_rules", []), *policy.get("exact_records", [])]:
    _validate_profile(rule, manifest)
    rule_id = rule.get("rule_id")
    if not isinstance(rule_id, str) or not rule_id or rule_id in rule_ids:
      raise MobileConversionError(f"invalid or duplicate mobile rule ID {rule_id!r}")
    rule_ids.add(rule_id)
  class_symbols = _symbols(manifest, "classes")
  for source_class, target_symbol in policy.get("class_map", {}).items():
    if not source_class.isdigit() or target_symbol not in class_symbols:
      raise MobileConversionError(f"invalid class mapping {source_class!r}: {target_symbol!r}")
  custom_symbols = _symbols(manifest, "mob-autoroll-custom-profiles")
  custom_profiles = policy.get("calculator", {}).get("custom_profiles", {})
  if not isinstance(custom_profiles, dict) or custom_profiles.get("none") != (
      "MOB_AUTOROLL_CUSTOM_NONE"
  ):
    raise MobileConversionError("mobile calculator policy has no canonical none profile")
  for name, symbol in custom_profiles.items():
    if not isinstance(name, str) or not name or symbol not in custom_symbols:
      raise MobileConversionError(
          f"mobile calculator custom profile {name!r} uses unknown symbol {symbol!r}"
      )
  for rule in policy.get("exact_records", []):
    custom_profile = rule.get("custom_profile")
    custom_hit_points = rule.get("custom_hit_points")
    if (custom_profile is None) != (custom_hit_points is None):
      raise MobileConversionError(
          f"mobile rule {rule['rule_id']} must pair custom profile and hit points"
      )
    if custom_profile is not None:
      if custom_profile not in custom_profiles or not isinstance(custom_hit_points, int) or (
          custom_hit_points < 1
      ):
        raise MobileConversionError(
            f"mobile rule {rule['rule_id']} has an invalid custom calculator profile"
        )
    if rule.get("tier") == "MOB_TIER_WORLD_BOSS" and custom_profile is None:
      raise MobileConversionError(
          f"world-boss mobile rule {rule['rule_id']} requires a named custom profile"
      )


def _base_profile(policy: dict[str, Any], source_code: str) -> dict[str, Any]:
  for profile in policy["base_profiles"]:
    if source_code in profile["codes"]:
      return profile
  raise MobileConversionError(f"known source race code {source_code} has no base profile")


def _exact_rule(policy: dict[str, Any], record: RolRecord) -> dict[str, Any] | None:
  for rule in policy.get("exact_records", []):
    if rule["basename"] == record.basename and int(rule["source_vnum"]) == record.vnum:
      if rule["source_sha256"] != record.sha256:
        raise MobileConversionError(
            f"exact mobile rule {rule['rule_id']} hash is stale for {record.record_id}"
        )
      return rule
  return None


def _effective_rows(record: RolRecord, exact: dict[str, Any] | None) -> list[list[str]]:
  rows = [list(row) for row in record.values.get("base_rows", [])]
  if exact is None:
    return rows
  if "repair_race_row" in exact:
    if not rows:
      raise MobileConversionError(f"mobile {record.record_id} cannot apply a race-row repair")
    rows[0] = [str(value) for value in exact["repair_race_row"]]
  if "repair_position_row" in exact:
    repaired = [str(value) for value in exact["repair_position_row"]]
    if len(rows) == 3:
      rows.append(repaired)
    elif len(rows) >= 4:
      rows[3] = repaired
    else:
      raise MobileConversionError(f"mobile {record.record_id} cannot apply a position-row repair")
  return rows


def _phrase_rule(policy: dict[str, Any], record: RolRecord) -> tuple[dict[str, Any], str] | None:
  strings = record.values.get("strings", {})
  evidence_text = " ".join(
      normalize_identity(str(strings.get(key) or ""))
      for key in ("aliases", "short_description")
  )
  padded = f" {evidence_text} "
  matches: list[tuple[int, int, str, dict[str, Any], str]] = []
  for rule in policy.get("phrase_rules", []):
    for phrase in rule.get("phrases", []):
      normalized = normalize_identity(str(phrase))
      if normalized and f" {normalized} " in padded:
        matches.append(
            (int(rule["priority"]), len(normalized.split()), str(rule["rule_id"]), rule, normalized)
        )
  if not matches:
    return None
  matches.sort(key=lambda item: (-item[0], -item[1], item[2], item[4]))
  return matches[0][3], matches[0][4]


def _size_from_dimensions(height: int, weight: int, size_symbols: dict[str, int]) -> tuple[int, str]:
  height_thresholds = (
      (6, "SIZE_FINE"),
      (12, "SIZE_DIMINUTIVE"),
      (24, "SIZE_TINY"),
      (48, "SIZE_SMALL"),
      (96, "SIZE_MEDIUM"),
      (192, "SIZE_LARGE"),
      (384, "SIZE_HUGE"),
      (768, "SIZE_GARGANTUAN"),
  )
  weight_thresholds = (
      (1, "SIZE_DIMINUTIVE"),
      (8, "SIZE_TINY"),
      (60, "SIZE_SMALL"),
      (500, "SIZE_MEDIUM"),
      (4000, "SIZE_LARGE"),
      (32000, "SIZE_HUGE"),
      (250000, "SIZE_GARGANTUAN"),
  )
  height_symbol = next(
      (symbol for threshold, symbol in height_thresholds if height <= threshold),
      "SIZE_COLOSSAL",
  )
  weight_symbol = next(
      (symbol for threshold, symbol in weight_thresholds if weight <= threshold),
      "SIZE_COLOSSAL",
  )
  symbol = max((height_symbol, weight_symbol), key=lambda value: size_symbols[value])
  return size_symbols[symbol], symbol


def resolve_mobile_identity(
    record: RolRecord,
    policy: dict[str, Any],
    manifest: dict[str, Any],
    registry: dict[str, SourceRace],
) -> MobileIdentityDecision:
  exact = _exact_rule(policy, record)
  rows = _effective_rows(record, exact)
  if len(rows) != 4 or not rows[0]:
    raise MobileConversionError(f"mobile {record.record_id} has no complete source race row")
  source_code = str(rows[0][0]).upper()
  if source_code not in registry:
    raise MobileConversionError(f"mobile {record.record_id} uses unknown source race code {source_code!r}")
  source_race = registry[source_code]
  base = _base_profile(policy, source_code)
  phrase_match = _phrase_rule(policy, record) if exact is None else None
  matched_phrase: str | None = None
  if exact is not None:
    winner = exact
    status = "AUTO_EXACT"
    confidence = "high"
    evidence = [
        f"exact source identity {record.basename}:{record.vnum}",
        f"source record sha256 {record.sha256}",
        str(exact["reason"]),
    ]
  elif phrase_match is not None:
    winner, matched_phrase = phrase_match
    status = "AUTO_PHRASE"
    confidence = "high"
    evidence = [
        f"whole phrase {matched_phrase!r} in normalized aliases/short description",
        f"source race code {source_code} ({source_race.source_name})",
    ]
  else:
    winner = base
    status = "AUTO_FALLBACK"
    confidence = "low" if source_code in {"DK", "N", "OH", "OP", "OU"} else "medium"
    evidence = [f"mandatory source-code fallback {source_code} ({source_race.source_name})"]

  race_symbols = _symbols(manifest, "race-types")
  subrace_symbols = _symbols(manifest, "subraces")
  size_symbols = _symbols(manifest, "sizes")
  meaningful_subraces = [str(value) for value in winner.get("subraces", [])]
  padded_subraces = meaningful_subraces + ["SUBRACE_UNKNOWN"] * (3 - len(meaningful_subraces))

  if "size" in winner:
    size_symbol = str(winner["size"])
    final_size = size_symbols[size_symbol]
    size_evidence = f"exact identity rule {winner['rule_id']}"
  else:
    try:
      authored_height = int(rows[0][1])
      authored_weight = int(rows[0][2])
    except (IndexError, ValueError) as error:
      raise MobileConversionError(f"mobile {record.record_id} has a malformed source race row") from error
    if authored_height > 0 and authored_weight > 0:
      final_size, size_symbol = _size_from_dimensions(
          authored_height, authored_weight, size_symbols
      )
      size_evidence = f"authored dimensions {authored_height}in/{authored_weight}lb"
    elif source_race.average_height > 0 and source_race.average_weight > 0:
      final_size, size_symbol = _size_from_dimensions(
          source_race.average_height, source_race.average_weight, size_symbols
      )
      size_evidence = (
          f"source race average dimensions {source_race.average_height}in/"
          f"{source_race.average_weight}lb"
      )
    else:
      size_symbol = str(winner.get("fallback_size", base.get("fallback_size", "SIZE_MEDIUM")))
      final_size = size_symbols[size_symbol]
      size_evidence = f"source-code fallback {source_code}"
    minimum_symbol = winner.get("minimum_size", base.get("minimum_size"))
    if minimum_symbol is not None and final_size < size_symbols[str(minimum_symbol)]:
      size_symbol = str(minimum_symbol)
      final_size = size_symbols[size_symbol]
      size_evidence += f"; identity minimum {minimum_symbol}"

  contradictions: list[str] = []
  if winner is not base and (
      winner["race"] != base["race"] or winner.get("subraces", []) != base.get("subraces", [])
  ):
    contradictions.append(
        f"source-code base {base['race']}/{base.get('subraces', [])} replaced by {winner['race']}/"
        f"{winner.get('subraces', [])}"
    )
  return MobileIdentityDecision(
      source_code=source_code,
      source_race_name=source_race.source_name,
      target_race=race_symbols[str(winner["race"])],
      target_race_symbol=str(winner["race"]),
      subraces=tuple(subrace_symbols[symbol] for symbol in padded_subraces),  # type: ignore[arg-type]
      subrace_symbols=tuple(padded_subraces),  # type: ignore[arg-type]
      final_size=final_size,
      final_size_symbol=size_symbol,
      size_evidence=size_evidence,
      rule_id=str(winner.get("rule_id", f"base-{source_code.lower()}")),
      rule_set_version=str(policy["rule_set_version"]),
      status=status,
      confidence=confidence,
      evidence=tuple(evidence),
      contradictions=tuple(contradictions),
  )


def map_mobile_level(authored_level: int) -> int:
  if authored_level <= 0:
    raise MobileConversionError(
        f"generic combat level mapping rejects non-positive source level {authored_level}"
    )
  source_level = min(authored_level, 59)
  return min(34, (3 * source_level + 4) // 5)


def _source_actions(record: RolRecord) -> set[int]:
  flags = record.values.get("flags", [])
  try:
    mask = int(flags[0])
  except (IndexError, ValueError) as error:
    raise MobileConversionError(f"mobile {record.record_id} has no valid source action mask") from error
  return {bit + 1 for bit in range(32) if mask & (1 << bit)}


def select_mobile_class(
    record: RolRecord,
    policy: dict[str, Any],
    manifest: dict[str, Any],
    exact: dict[str, Any] | None = None,
) -> tuple[int, str]:
  rows = _effective_rows(record, exact)
  if len(rows) != 4:
    raise MobileConversionError(f"mobile {record.record_id} has no complete position/class row")
  position_row = rows[3]
  try:
    source_class = int(position_row[3]) if len(position_row) > 3 else 0
  except ValueError as error:
    raise MobileConversionError(f"mobile {record.record_id} has invalid source class syntax") from error
  class_map = policy["class_map"]
  if str(source_class) not in class_map:
    raise MobileConversionError(
        f"mobile {record.record_id} uses unknown source class {source_class}"
    )
  symbol = str(class_map[str(source_class)])
  if source_class == 0:
    role_classes = (
        (19, "CLASS_PSIONICIST"),
        (21, "CLASS_WIZARD"),
        (20, "CLASS_CLERIC"),
        (22, "CLASS_ROGUE"),
        (23, "CLASS_WARRIOR"),
    )
    actions = _source_actions(record)
    symbol = next((value for action, value in role_classes if action in actions), symbol)
  return _symbols(manifest, "classes")[symbol], symbol


def _dice_average(value: str) -> int:
  match = re.fullmatch(r"(\d+)d(\d+)\+([+-]?\d+)", value)
  if match is None:
    raise MobileConversionError(f"invalid source dice expression {value!r}")
  count, size, bonus = (int(part) for part in match.groups())
  if count == 0 and size == 0:
    return bonus
  if count < 1 or size < 1:
    raise MobileConversionError(f"invalid source dice expression {value!r}")
  return count * (size + 1) // 2 + bonus


def _source_position(value: int) -> int:
  """Map the source posture/status bitfield using the target's exact enum values."""

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
  return {0: 5, 1: 7, 2: 7, 3: 9}.get(value & 3, 9)


def _source_money_and_experience(tokens: list[str]) -> tuple[str, int]:
  if not tokens:
    raise MobileConversionError("source mobile has no money/experience row")
  try:
    if "." in tokens[0]:
      denominations = tokens[0].split(".")
      if len(denominations) != 4:
        raise ValueError
      source_money = ".".join(str(int(value)) for value in denominations)
    else:
      source_money = str(int(tokens[0]))
    experience = int(tokens[1])
  except (IndexError, ValueError) as error:
    raise MobileConversionError("source mobile has invalid money/experience syntax") from error
  return source_money, experience


def select_mobile_tier(
    record: RolRecord,
    mapped_level: int,
    exact: dict[str, Any] | None,
    manifest: dict[str, Any],
) -> tuple[int, str, str, tuple[str, ...]]:
  tiers = _symbols(manifest, "mob-tiers")
  if exact is not None and "tier" in exact:
    symbol = str(exact["tier"])
    return tiers[symbol], symbol, str(exact["rule_id"]), (str(exact["reason"]),)
  rows = _effective_rows(record, exact)
  try:
    source_level = int(rows[1][0])
  except (IndexError, ValueError) as error:
    raise MobileConversionError(f"mobile {record.record_id} has no valid source level") from error
  if source_level <= 50:
    symbol = "MOB_TIER_STANDARD"
    return tiers[symbol], symbol, "tier-standard-v1", (
        "source level is outside the mandatory 51+ classification population",
    )

  try:
    source_hit_points = _dice_average(str(rows[1][3]))
    source_damage = _dice_average(str(rows[1][4]))
    source_experience = int(str(rows[2][1]))
  except (IndexError, ValueError) as error:
    raise MobileConversionError(f"mobile {record.record_id} lacks tier-classification evidence") from error
  competence_hit_points = mapped_level * mapped_level + mapped_level * 10
  pressure = source_hit_points * 100 // max(1, competence_hit_points)
  identity = normalize_identity(str(record.identity or ""))
  named_pressure = any(
      f" {phrase} " in f" {identity} "
      for phrase in ("boss", "emperor", "empress", "king", "queen", "overlord")
  )
  if pressure >= 1200 or (pressure >= 800 and source_damage >= 100):
    symbol = "MOB_TIER_RAID"
  elif pressure >= 700 or (pressure >= 450 and source_damage >= 75):
    symbol = "MOB_TIER_BIG_GROUP"
  elif pressure >= 350 or named_pressure:
    symbol = "MOB_TIER_SMALL_GROUP"
  elif pressure >= 180:
    symbol = "MOB_TIER_ELITE"
  else:
    symbol = "MOB_TIER_STANDARD"
  return tiers[symbol], symbol, "tier-source-pressure-v1", (
      f"source average hit points {source_hit_points}",
      f"source average damage {source_damage}",
      f"source experience {source_experience}",
      f"hit-point pressure {pressure} percent of mapped competence baseline",
      f"named encounter pressure {'present' if named_pressure else 'absent'}",
  )


def select_mobile_conversion(
    record: RolRecord,
    policy: dict[str, Any],
    manifest: dict[str, Any],
    registry: dict[str, SourceRace],
) -> MobileSelection:
  identity = resolve_mobile_identity(record, policy, manifest, registry)
  exact = _exact_rule(policy, record)
  rows = _effective_rows(record, exact)
  try:
    authored_level = int(rows[1][0])
  except (IndexError, ValueError) as error:
    raise MobileConversionError(f"mobile {record.record_id} has no valid authored level") from error
  effective_level = int(exact["repair_level"]) if exact and "repair_level" in exact else authored_level
  mapped_level = map_mobile_level(effective_level)
  target_class, class_symbol = select_mobile_class(record, policy, manifest, exact)
  tier, tier_symbol, tier_rule_id, tier_evidence = select_mobile_tier(
      record, mapped_level, exact, manifest
  )
  race_row = rows[0]
  aggression_codes: tuple[str, ...] = ()
  ignored_aggression_tokens: tuple[str, ...] = ()
  if len(race_row) > 3:
    authored_aggression = tuple(
        code for code in str(race_row[3]).upper().split(".") if code
    )
    ignored = set(str(value) for value in policy.get("source_ignored_aggression_tokens", []))
    aggression_codes = tuple(code for code in authored_aggression if code in registry)
    ignored_aggression_tokens = tuple(code for code in authored_aggression if code in ignored)
    unknown = sorted(set(authored_aggression) - set(registry) - ignored)
    if unknown:
      raise MobileConversionError(
          f"mobile {record.record_id} uses unknown aggression race codes {unknown}"
      )
  try:
    source_sex = int(rows[3][2])
  except (IndexError, ValueError) as error:
    raise MobileConversionError(f"mobile {record.record_id} has no valid sex field") from error
  if source_sex in {0, 1, 2}:
    target_sex = source_sex
  elif exact is not None and "repair_sex" in exact:
    target_sex = int(exact["repair_sex"])
  else:
    raise MobileConversionError(
        f"mobile {record.record_id} has invalid sex {source_sex} without an exact repair"
    )
  try:
    current_position = _source_position(int(rows[3][0]))
    default_position = _source_position(int(rows[3][1]))
    explicit_spell_resistance = int(rows[3][4]) if len(rows[3]) > 4 else 0
    source_prestige_bonus = int(rows[3][5]) if len(rows[3]) > 5 else 0
  except (IndexError, ValueError) as error:
    raise MobileConversionError(
        f"mobile {record.record_id} has an invalid position/class extension"
    ) from error
  source_money, source_experience = _source_money_and_experience(rows[2])
  source_level = min(max(effective_level, 0), 59)
  if 0 < explicit_spell_resistance <= 100 and not (
      source_level < 54 and explicit_spell_resistance > 99
  ):
    effective_spell_resistance = explicit_spell_resistance
  else:
    effective_spell_resistance = (
        source_level * registry[identity.source_code].magic_resistance_cap // 59
    )
  repairs: list[str] = []
  if exact is not None:
    for key in (
        "repair_race_row",
        "repair_position_row",
        "repair_level",
        "repair_sex",
        "ignored_money_tokens",
    ):
      if key in exact:
        repairs.append(f"{key}: {exact['reason']}")
  return MobileSelection(
      source_record_id=record.record_id,
      source_vnum=record.vnum,
      mapped_level=mapped_level,
      target_class=target_class,
      target_class_symbol=class_symbol,
      target_sex=target_sex,
      current_position=current_position,
      default_position=default_position,
      tier=tier,
      tier_symbol=tier_symbol,
      tier_rule_id=tier_rule_id,
      tier_evidence=tier_evidence,
      identity=identity,
      source_aggression_codes=aggression_codes,
      ignored_aggression_tokens=ignored_aggression_tokens,
      custom_profile=str(exact["custom_profile"]) if exact and exact.get("custom_profile") else None,
      custom_hit_points=int(exact["custom_hit_points"]) if exact and exact.get("custom_hit_points") else None,
      effective_source_spell_resistance=effective_spell_resistance,
      source_money=source_money,
      source_experience=source_experience,
      source_prestige_bonus=source_prestige_bonus,
      repairs=tuple(repairs),
  )
