from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_special_reconciliation import (
    handler_disposition,
    source_handler_definitions,
    write_special_reconciliation_bundle,
)


class RolSpecialReconciliationTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.root = default_repo_root()

  def test_reviewed_handler_dispositions_do_not_rely_on_matching_names(self) -> None:
    guild = handler_disposition("guild")
    dump = handler_disposition("dump")
    poison = handler_disposition("poison")
    breath = handler_disposition("breath_weapon_fire")
    conjured = handler_disposition("conj_familiar_die")
    bloodstone_death = handler_disposition("bs_undead_die")
    home_reset = handler_disposition("home_reset")
    magic_pool = handler_disposition("magic_pool")
    auto_distributor = handler_disposition("autoDistributor")
    shadow_giant = handler_disposition("shadow_giant")
    guild_guard = handler_disposition("guild_guard")
    mage_guild = handler_disposition("guild_classtype_mage")
    thief_guild = handler_disposition("guild_classtype_thief")
    warrior_guild = handler_disposition("guild_classtype_warrior")
    cleric_guild = handler_disposition("guild_classtype_cleric")
    major_beholder = handler_disposition("major_beholder")
    lich_energy_drain = handler_disposition("lich_energy_drain")
    bandit = handler_disposition("bandit")
    sister_knight = handler_disposition("sister_knight")
    shaman_totem = handler_disposition("shaman_totem")
    spirit_wolf = handler_disposition("spirit_wolf_die")
    ship = handler_disposition("ship")
    navigator = handler_disposition("navagator")
    unknown = handler_disposition("not_reviewed")

    self.assertEqual("RoL Guild Room", guild["target"])
    self.assertEqual("NATIVE_PERSISTED", guild["strategy"])
    self.assertEqual("SOURCE_INERT_EXCLUDED", dump["strategy"])
    self.assertIn("returns before", dump["reason"])
    self.assertEqual("RoL Poison Bite", poison["target"])
    self.assertEqual("NATIVE_ADAPTED", poison["strategy"])
    self.assertEqual("breath_weapon_fire", breath["target"])
    self.assertEqual("NATIVE_PERSISTED", breath["strategy"])
    self.assertEqual("mobile action flag 119", conjured["target"])
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", conjured["strategy"])
    self.assertEqual("mobile action flag 124", bloodstone_death["target"])
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", bloodstone_death["strategy"])
    self.assertEqual("room flag 46", home_reset["target"])
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", home_reset["strategy"])
    self.assertEqual("RoL Magic Pool", magic_pool["target"])
    self.assertEqual("NATIVE_PERSISTED", magic_pool["strategy"])
    self.assertEqual("RoL Auto Distributor", auto_distributor["target"])
    self.assertEqual("NATIVE_PERSISTED", auto_distributor["strategy"])
    self.assertEqual("RoL Shadow Giant", shadow_giant["target"])
    self.assertEqual("NATIVE_ADAPTED", shadow_giant["strategy"])
    self.assertEqual("RoL Guild Guard", guild_guard["target"])
    self.assertEqual("NATIVE_ADAPTED", guild_guard["strategy"])
    self.assertEqual("RoL Mage Guild Room", mage_guild["target"])
    self.assertEqual("RoL Thief Guild Room", thief_guild["target"])
    self.assertEqual("RoL Warrior Guild Room", warrior_guild["target"])
    self.assertEqual("RoL Cleric Guild Room", cleric_guild["target"])
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED"
            for row in (mage_guild, thief_guild, warrior_guild, cleric_guild)
        )
    )
    self.assertEqual("RoL Major Beholder", major_beholder["target"])
    self.assertEqual("NATIVE_ADAPTED", major_beholder["strategy"])
    self.assertEqual("RoL Lich Energy Drain", lich_energy_drain["target"])
    self.assertEqual("NATIVE_ADAPTED", lich_energy_drain["strategy"])
    self.assertEqual("RoL Trade Bandit", bandit["target"])
    self.assertEqual("NATIVE_ADAPTED", bandit["strategy"])
    self.assertEqual("RoL Sister Knight", sister_knight["target"])
    self.assertEqual("NATIVE_ADAPTED", sister_knight["strategy"])
    self.assertEqual("RoL Shaman Totem", shaman_totem["target"])
    self.assertEqual("NATIVE_ADAPTED", shaman_totem["strategy"])
    self.assertEqual("mobile action flag 123", spirit_wolf["target"])
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", spirit_wolf["strategy"])
    self.assertEqual("RoL Ship", ship["target"])
    self.assertEqual("NATIVE_ADAPTED", ship["strategy"])
    self.assertEqual("RoL Ship Navigator", navigator["target"])
    self.assertEqual("NATIVE_ADAPTED", navigator["strategy"])
    self.assertEqual("pending", unknown["status"])

  def test_source_definition_scanner_ignores_comment_and_string_decoys(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      source_root = Path(temporary)
      (source_root / "src").mkdir()
      (source_root / "src/sample.c").write_text(
          "/* int decoy(void) { } */\n"
          "const char *text = \"int string_decoy(void) { }\";\n"
          "int actual(void)\n"
          "{\n"
          "  return 1;\n"
          "}\n",
          encoding="ascii",
      )

      definitions = source_handler_definitions(
          source_root, {"decoy", "string_decoy", "actual"}
      )

    self.assertEqual(["actual"], sorted(definitions))
    self.assertEqual(3, definitions["actual"]["line"])
    self.assertEqual(4, definitions["actual"]["lines"])

  def test_production_inputs_generate_complete_progress_ledgers(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      summary = write_special_reconciliation_bundle(
          self.root / "lib/rol-conversion/runs/phase1-e6ea7982",
          self.root / "lib/rol-conversion/runs/phase2-e6ea7982",
          self.root / "lib/rol-conversion/runs/phase5-shop-20260812-audit",
          self.root / "EXAMPLE/RealmsOfLuminari",
          Path(temporary) / "phase6",
          created_at="2026-08-12T02:05:00Z",
      )

      self.assertEqual(1_234, summary["discovered_direct_binding_candidates"])
      self.assertEqual(87, summary["source_preprocessor_excluded_bindings"])
      self.assertEqual(1_147, summary["active_direct_bindings"])
      self.assertEqual(562, summary["source_handlers"])
      self.assertEqual(562, summary["source_handler_definitions_located"])
      self.assertEqual(247, summary["active_implicit_race_bindings"])
      self.assertEqual(
          {"standardDemon": 134, "standardDevil": 101, "standardUmberhulk": 12},
          summary["implicit_race_bindings_by_handler"],
      )
      self.assertEqual(
          {"alongside-direct": 22, "implicit-only": 225},
          summary["implicit_race_bindings_by_composition"],
      )
      self.assertEqual(3, summary["implicit_race_handler_definitions_located"])
      self.assertEqual(571, summary["direct_bindings_by_status"]["resolved"])
      self.assertEqual(576, summary["direct_bindings_by_status"]["pending"])
      self.assertEqual(94, summary["source_handlers_by_status"]["resolved"])
      self.assertEqual(468, summary["source_handlers_by_status"]["pending"])
      self.assertEqual(196, summary["direct_bindings_by_strategy"]["NATIVE_ADAPTED"])
      self.assertEqual(
          113, summary["direct_bindings_by_strategy"]["NATIVE_ADAPTED_COMPOSABLE"]
      )
      self.assertEqual(848, summary["act_spec_records"])
      self.assertEqual(559, summary["act_spec_by_status"]["resolved"])
      self.assertEqual(289, summary["act_spec_by_status"]["pending"])
      self.assertEqual(
          {"resolved": 247}, summary["implicit_race_bindings_by_status"]
      )

      output_dir = Path(temporary) / "phase6"
      manifest = json.loads((output_dir / "run-manifest.json").read_text(encoding="ascii"))
      expected_records = {
          "act-spec-ledger.jsonl": 848,
          "automatic-race-ledger.jsonl": 247,
          "binding-ledger.jsonl": 1_147,
          "handler-inventory.jsonl": 562,
          "preprocessor-excluded-binding-ledger.jsonl": 87,
      }
      for artifact in manifest["artifacts"]:
        if artifact["path"] in expected_records:
          lines = (output_dir / artifact["path"]).read_text(encoding="ascii").splitlines()
          self.assertEqual(expected_records[artifact["path"]], artifact["records"])
          self.assertEqual(expected_records[artifact["path"]], len(lines))


if __name__ == "__main__":
  unittest.main()
