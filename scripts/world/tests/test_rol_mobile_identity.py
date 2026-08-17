from __future__ import annotations

from collections import Counter
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_mobile_identity import (
    MobileConversionError,
    load_mobile_conversion_policy,
    map_mobile_level,
    select_mobile_conversion,
)
from wtool_lib.rol_mob_calculator import MobileCalculatorClient
from wtool_lib.rol_source import parse_active_rol_corpus, parse_rol_source_file
from wtool_lib.rol_transform import emit_mobile


class RolMobileIdentityTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()
    cls.policy, cls.manifest, cls.registry = load_mobile_conversion_policy(cls.root)
    cls.corpus = parse_active_rol_corpus(
        cls.root / "EXAMPLE/RealmsOfLuminari", cls.root
    )
    cls.mobiles = [record for record in cls.corpus.records if record.kind == "mob"]

  def _record(self, basename: str, vnum: int):
    return next(
        record
        for record in self.mobiles
        if record.basename == basename and record.vnum == vnum
    )

  def _fixture(self, race_row: str, aliases: str, short_description: str):
    data = (
        "<*> File Version 1 <*>\n#100\n"
        f"{aliases}~\n{short_description}~\nA mobile waits.~\nA mobile.~\n"
        f"0 0 0 0 S\n{race_row}\n51 10 0 51d100+1000 4d8+10\n0 100\n"
        "131 131 0 0\n"
    ).encode("ascii")
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "fixture.mob"
    path.write_bytes(data)
    records, corpus = parse_rol_source_file(
        path, "areas/mob/fixture.mob", "mob", "fixture"
    )
    self.assertTrue(corpus.complete)
    return records[0]

  def test_source_registry_and_base_matrix_are_complete(self) -> None:
    self.assertEqual(75, len(self.registry))
    self.assertEqual(
        set(self.registry),
        {
            code
            for profile in self.policy["base_profiles"]
            for code in profile["codes"]
        },
    )
    self.assertEqual("Kirin", self.registry["K"].source_name)

  def test_level_mapping_preserves_high_end_competence(self) -> None:
    expected = {
        1: 1,
        49: 30,
        50: 30,
        51: 31,
        52: 32,
        53: 32,
        54: 33,
        55: 33,
        56: 34,
        59: 34,
        60: 34,
    }
    self.assertEqual(expected, {level: map_mobile_level(level) for level in expected})
    with self.assertRaisesRegex(MobileConversionError, "non-positive"):
      map_mobile_level(0)

  def test_compound_phrase_wins_as_one_atomic_profile(self) -> None:
    record = self._fixture("D 0 0", "ancient undead dragon", "an undead dragon")
    selection = select_mobile_conversion(
        record, self.policy, self.manifest, self.registry
    )
    self.assertEqual("phrase-undead-dragon", selection.identity.rule_id)
    self.assertEqual("RACE_TYPE_UNDEAD", selection.identity.target_race_symbol)
    self.assertEqual(
        ("SUBRACE_UNKNOWN", "SUBRACE_UNKNOWN", "SUBRACE_UNKNOWN"),
        selection.identity.subrace_symbols,
    )
    self.assertEqual("AUTO_PHRASE", selection.identity.status)

  def test_unknown_source_code_fails_closed(self) -> None:
    record = self._fixture("QQ 0 0", "unknown", "an unknown mobile")
    with self.assertRaisesRegex(MobileConversionError, "unknown source race code"):
      select_mobile_conversion(record, self.policy, self.manifest, self.registry)

  def test_known_source_repairs_are_exact_and_automatic(self) -> None:
    griffon = select_mobile_conversion(
        self._record("griffon", 10632), self.policy, self.manifest, self.registry
    )
    missing_position = select_mobile_conversion(
        self._record("llyrath", 51348), self.policy, self.manifest, self.registry
    )
    echo = select_mobile_conversion(
        self._record("av", 59800), self.policy, self.manifest, self.registry
    )

    self.assertEqual(("US", "U", "UV"), griffon.source_aggression_codes)
    self.assertTrue(any("repair_race_row" in value for value in griffon.repairs))
    self.assertEqual(1, missing_position.target_sex)
    self.assertTrue(any("repair_position_row" in value for value in missing_position.repairs))
    self.assertEqual(1, echo.mapped_level)
    self.assertEqual("MOB_TIER_STANDARD", echo.tier_symbol)
    self.assertEqual("RACE_TYPE_CONSTRUCT", echo.identity.target_race_symbol)

  def test_tiamat_forms_have_individual_world_boss_profiles(self) -> None:
    living = select_mobile_conversion(
        self._record("astral_main", 19700), self.policy, self.manifest, self.registry
    )
    dracolich = select_mobile_conversion(
        self._record("astral_main", 19701), self.policy, self.manifest, self.registry
    )

    self.assertEqual("tiamat-living-v1", living.custom_profile)
    self.assertEqual("RACE_TYPE_DRAGON", living.identity.target_race_symbol)
    self.assertEqual("tiamat-dracolich-v1", dracolich.custom_profile)
    self.assertEqual("RACE_TYPE_UNDEAD", dracolich.identity.target_race_symbol)
    self.assertEqual(29999, living.custom_hit_points)
    self.assertEqual(30000, dracolich.custom_hit_points)
    for selection in (living, dracolich):
      self.assertEqual("MOB_TIER_WORLD_BOSS", selection.tier_symbol)
      self.assertEqual("SIZE_COLOSSAL", selection.identity.final_size_symbol)

  def test_source_spell_resistance_uses_explicit_then_race_derived_precedence(self) -> None:
    explicit_record = next(
        record
        for record in self.mobiles
        if len(record.values.get("base_rows", [])) == 4
        and len(record.values["base_rows"][3]) >= 5
        and 0 < int(record.values["base_rows"][3][4]) < 100
    )
    explicit = select_mobile_conversion(
        explicit_record, self.policy, self.manifest, self.registry
    )
    self.assertEqual(
        int(explicit_record.values["base_rows"][3][4]),
        explicit.effective_source_spell_resistance,
    )

    derived_record = next(
        record
        for record in self.mobiles
        if record.values["base_rows"][0][0].upper() == "D"
        and len(record.values["base_rows"][3]) < 5
    )
    derived = select_mobile_conversion(
        derived_record, self.policy, self.manifest, self.registry
    )
    source_level = min(int(derived_record.values["base_rows"][1][0]), 59)
    self.assertEqual(source_level * 60 // 59, derived.effective_source_spell_resistance)

    living = select_mobile_conversion(
        self._record("astral_main", 19700), self.policy, self.manifest, self.registry
    )
    self.assertEqual(80, living.effective_source_spell_resistance)

  def test_tiamat_serialization_preserves_each_runtime_phase_budget(self) -> None:
    with MobileCalculatorClient(self.root) as calculator:
      living = emit_mobile(
          self._record("astral_main", 19700), 2_019_700, calculator=calculator
      )
      dracolich = emit_mobile(
          self._record("astral_main", 19701), 2_019_701, calculator=calculator
      )

    self.assertIn("34 ", living.text)
    self.assertIn("1d1+29998", living.text)
    self.assertIn("SpellRes: 80\n", living.text)
    self.assertIn("Tier: 5\n", living.text)
    self.assertIn("1d1+29999", dracolich.text)
    self.assertIn("SpellRes: 60\n", dracolich.text)
    self.assertEqual("tiamat-living-v1", living.ledger["calculator"]["custom_profile"])
    self.assertTrue(living.ledger["serialization"]["mob_custom_stats"])
    self.assertEqual(
        "tiamat-dracolich-v1", dracolich.ledger["calculator"]["custom_profile"]
    )

  def test_full_active_corpus_resolves_without_manual_review(self) -> None:
    statuses = Counter()
    mixed_count = 0
    high_level_count = 0
    aggression_count = 0
    for record in self.mobiles:
      selection = select_mobile_conversion(
          record, self.policy, self.manifest, self.registry
      )
      statuses[selection.identity.status] += 1
      self.assertEqual(3, len(selection.identity.subraces))
      self.assertGreaterEqual(selection.identity.final_size, 1)
      self.assertGreaterEqual(selection.tier, 0)
      source_code = record.values["base_rows"][0][0].upper()
      if source_code in {"DK", "N", "OH", "OP", "OU", "K"}:
        mixed_count += 1
      if int(record.values["base_rows"][1][0]) >= 51:
        high_level_count += 1
        self.assertNotEqual("MOB_TIER_UNSPECIFIED", selection.tier_symbol)
      if len(record.values["base_rows"][0]) > 3:
        aggression_count += 1

    self.assertEqual(12407, len(self.mobiles))
    self.assertEqual(258, mixed_count)
    self.assertEqual(2542, high_level_count)
    self.assertEqual(356, aggression_count)
    self.assertEqual(
        {"AUTO_EXACT", "AUTO_PHRASE", "AUTO_FALLBACK"}, set(statuses)
    )

  def test_full_corpus_serialization_uses_one_native_calculator_process(self) -> None:
    explicit_spell_resistance = 0
    positive_prestige = 0
    negative_prestige = 0
    with MobileCalculatorClient(self.root) as calculator:
      for index, record in enumerate(self.mobiles):
        emitted = emit_mobile(record, 2_500_000 + index, calculator=calculator)
        self.assertIsNotNone(emitted.ledger)
        self.assertIn("SubRace 1:", emitted.text)
        self.assertIn("SubRace 2:", emitted.text)
        self.assertIn("SubRace 3:", emitted.text)
        self.assertIn("Size:", emitted.text)
        self.assertIn("Tier:", emitted.text)
        self.assertIn("SpellRes:", emitted.text)
        ledger = emitted.ledger or {}
        spell_resistance = ledger["spell_resistance"]["target"]
        if spell_resistance > 0:
          explicit_spell_resistance += 1
        if ledger["prestige"]["source"] > 0:
          positive_prestige += 1
        elif ledger["prestige"]["source"] < 0:
          negative_prestige += 1
      self.assertEqual(len(self.mobiles), calculator.requests)
      self.assertGreater(explicit_spell_resistance, 0)
      self.assertEqual(108, positive_prestige)
      self.assertEqual(13, negative_prestige)

  def test_aggression_list_never_changes_owner_identity(self) -> None:
    plain = self._fixture("H 0 0", "guard", "a guard")
    aggressive = self._fixture("H 0 0 U.D.X", "guard", "a guard")
    plain_selection = select_mobile_conversion(
        plain, self.policy, self.manifest, self.registry
    )
    aggressive_selection = select_mobile_conversion(
        aggressive, self.policy, self.manifest, self.registry
    )
    self.assertEqual(plain_selection.identity, aggressive_selection.identity)
    self.assertEqual(("U", "D", "X"), aggressive_selection.source_aggression_codes)
