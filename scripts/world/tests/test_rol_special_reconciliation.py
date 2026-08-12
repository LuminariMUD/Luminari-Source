from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root
from wtool_lib.rol_periodic_profiles import PROFILE_SOURCES
from wtool_lib.rol_state_periodic_profiles import (
    COMPOSED_STATE_PROFILE_SOURCES,
    STATE_PROFILE_SOURCES,
)
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
    bard_guilds = [
        handler_disposition(name) for name in ("guild_bard", "guild_battlechanter")
    ]
    bloodstone_guild_guards = [
        handler_disposition(f"bs_guildguard_{name}")
        for name in ("antiwar", "assassin", "clersham", "necro", "sorcconj", "thief")
    ]
    waterdeep_guilds = [
        handler_disposition(f"waterdeep_guild_{name}")
        for name in (
            "one",
            "two",
            "three",
            "four",
            "five",
            "six",
            "seven",
            "eight",
            "nine",
            "ten",
            "eleven",
            "twelve",
        )
    ]
    alert_callers = [
        handler_disposition(name)
        for name in (
            "av_drisinil_shout",
            "av_tukra_shout",
            "demogorgon_shout",
            "imix_pet_demon_shout",
        )
    ]
    composed_alerts = [
        handler_disposition(name) for name in ("imix_shout", "yancbin_shout")
    ]
    death_profiles = [
        handler_disposition(name)
        for name in (
            "tentacle_die",
            "fire_mephit_die",
            "water_mephit_die",
            "air_mephit_die",
            "earth_mephit_die",
            "fire_mental_die",
            "water_mental_die",
            "air_mental_die",
            "earth_mental_die",
            "treant_die",
            "phantom_steed_die",
            "dark_shade_die",
            "crystal_golem_die",
            "halruaa_fleshdoll",
            "halruaa_transmuter1",
            "halruaa_transmuter2",
            "halruaa_transmuter_itemdrop",
            "hippogriff_die",
            "ice_malice",
            "jotun_balor",
            "menden_figurine_die",
            "menden_inv_serv_die",
            "pure_blood_90812",
            "pure_blood_90819",
            "pure_blood_90837",
            "pure_blood_90866",
            "shadow_demon_of_torm",
            "spore_ball",
            "stone_crumble",
            "um2_blackPuddingSplit",
            "unseen_servant_die",
        )
    ]
    yggdrasil = handler_disposition("yggdrasil_branch")
    waterdeep_ambient = [
        handler_disposition(name)
        for name in (
            "artillery_one",
            "baker_one",
            "baker_two",
            "casino_one",
            "casino_two",
            "cat_one",
            "cleric_one",
            "drunk_one",
            "drunk_three",
            "drunk_two",
            "farmer_one",
            "homeless_one",
            "homeless_two",
            "mage_one",
            "mercenary_one",
            "mercenary_three",
            "mercenary_two",
            "merchant_one",
            "merchant_two",
            "wanderer",
            "warrior_one",
            "youth_one",
            "youth_two",
            "assassin_one",
            "brigand_one",
            "commoner_five",
            "commoner_four",
            "commoner_one",
            "commoner_six",
            "commoner_three",
            "fisherman_one",
            "fisherman_two",
            "naval_four",
            "naval_one",
            "naval_two",
            "sailor_one",
            "seabird_one",
            "seabird_two",
            "seaman_one",
            "shopper_one",
            "shopper_two",
            "tailor_one",
            "waterdeep_guard_one",
            "waterdeep_guard_two",
        )
    ]
    source_periodic = [handler_disposition(name) for name in PROFILE_SOURCES]
    state_periodic = [handler_disposition(name) for name in STATE_PROFILE_SOURCES]
    composed_state_periodic = [
        handler_disposition(name) for name in COMPOSED_STATE_PROFILE_SOURCES
    ]
    rogue = handler_disposition("rogue_one")
    major_beholder = handler_disposition("major_beholder")
    monster_combat = [
        handler_disposition(name)
        for name in (
            "plant_attacks_poison",
            "conj_lycan_tiger",
            "conj_lycan_fox",
            "spider_venom_medium",
            "ashentoris",
            "ryo_bansheeWail",
            "ttf_fourarms",
            "ttf_tentacles",
            "ttf_rot_bringer",
            "winged_deva",
            "halruaa_small_prismatic_elem",
            "halruaa_crit_prismatic_elem",
            "halruaa_uber_prismatic_elem",
            "et_fireBoss",
            "et_earthBoss",
            "et_airBoss",
            "et_waterBoss",
            "devil_pitFiendBite",
            "chicken",
            "kobold_priest",
            "piercer",
            "purple_worm",
            "phalanx",
            "skeleton",
            "xexos",
            "agthrodos",
            "tree_spirit",
            "dranum_lifesuck",
            "swallow_whole",
            "swallow_whole_spit",
            "movanic_deva",
            "ilshazone_canthus",
            "jotun_thrym",
            "jotun_utgard_loki",
        )
    ]
    pit_fiend_tail = handler_disposition("devil_pitFiendTail")
    lich_energy_drain = handler_disposition("lich_energy_drain")
    undead_drains = [
        handler_disposition(name)
        for name in (
            "undead_ghoul",
            "undead_shadow",
            "undead_wight",
            "undead_ghast",
            "undead_wraith",
            "undead_spectre",
            "undead_ghost",
        )
    ]
    bandit = handler_disposition("bandit")
    bloodstone_critter = handler_disposition("bs_critter")
    bloodstone_portal = handler_disposition("bs_portal")
    designated_follower = handler_disposition("follow_that_mob")
    elemental_tower_alert = handler_disposition("elemental_tower_shout")
    fixed_bodyguard = handler_disposition("ice_bodyguards")
    floating_pool = handler_disposition("floating_pool")
    item_blocker = handler_disposition("item_block")
    sister_knight = handler_disposition("sister_knight")
    shaman_totem = handler_disposition("shaman_totem")
    spirit_wolf = handler_disposition("spirit_wolf_die")
    ship = handler_disposition("ship")
    navigator = handler_disposition("navagator")
    portal_door = handler_disposition("portal_door")
    artifact_handlers = [
        handler_disposition(name)
        for name in (
            "OakenDefender",
            "Amaukekel",
            "Fade2",
            "HornOfHenekar",
            "Doombringer",
            "Kelrarin",
            "Kelrom",
            "Gesen",
            "tiamat_stinger",
            "New_Avernus",
        )
    ]
    unsafe_backdoor = handler_disposition("NeverLooseItem")
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
    self.assertEqual(
        "RoL Monster Combat plus RoL alert runtime profile",
        elemental_tower_alert["target"],
    )
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", elemental_tower_alert["strategy"])
    self.assertEqual("RoL Fixed Bodyguard", fixed_bodyguard["target"])
    self.assertEqual("NATIVE_ADAPTED", fixed_bodyguard["strategy"])
    self.assertEqual("RoL Portal Door", portal_door["target"])
    self.assertEqual("NATIVE_ADAPTED", portal_door["strategy"])
    self.assertEqual("RoL Mage Guild Room", mage_guild["target"])
    self.assertEqual("RoL Thief Guild Room", thief_guild["target"])
    self.assertEqual("RoL Warrior Guild Room", warrior_guild["target"])
    self.assertEqual("RoL Cleric Guild Room", cleric_guild["target"])
    self.assertTrue(
        all(
            row["target"] == "RoL Bard Guild Room"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in bard_guilds
        )
    )
    self.assertTrue(
        all(
            row["target"] == "RoL Guild Guard"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in bloodstone_guild_guards
        )
    )
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED"
            for row in (mage_guild, thief_guild, warrior_guild, cleric_guild)
        )
    )
    self.assertTrue(
        all(
            row["target"] == "RoL Alert Caller"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in alert_callers
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE" for row in composed_alerts)
    )
    self.assertTrue(
        all(
            row["target"] == "converted mobile death profile"
            and row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE"
            for row in death_profiles
        )
    )
    self.assertEqual("RoL Yggdrasil Branch", yggdrasil["target"])
    self.assertEqual("NATIVE_ADAPTED", yggdrasil["strategy"])
    self.assertTrue(
        all(
            row["target"] == "RoL Waterdeep Ambient"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in waterdeep_ambient
        )
    )
    self.assertTrue(
        all(
            row["target"] == "RoL Source Periodic"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in source_periodic
        )
    )
    self.assertTrue(
        all(
            row["target"] == "RoL Stateful Periodic"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in state_periodic
        )
    )
    self.assertTrue(
        all(
            row["target"] == "RoL Guild Guard"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in composed_state_periodic
        )
    )
    self.assertEqual("SOURCE_INERT_EXCLUDED", rogue["strategy"])
    self.assertIn("NPC_HIT", rogue["reason"])
    self.assertIn("victim", rogue["reason"])
    self.assertTrue(
        all(
            row["target"] == "RoL Waterdeep Guild Room"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in waterdeep_guilds
        )
    )
    self.assertEqual("RoL Major Beholder", major_beholder["target"])
    self.assertEqual("NATIVE_ADAPTED", major_beholder["strategy"])
    self.assertTrue(
        all(
            row["target"] == "RoL Monster Combat"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in monster_combat
        )
    )
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", pit_fiend_tail["strategy"])
    self.assertIn("pit-fiend tail", pit_fiend_tail["target"])
    self.assertEqual("RoL Lich Energy Drain", lich_energy_drain["target"])
    self.assertEqual("NATIVE_ADAPTED", lich_energy_drain["strategy"])
    self.assertTrue(
        all(
            row["target"] == "RoL Undead Drain"
            and row["strategy"] == "NATIVE_ADAPTED"
            for row in undead_drains
        )
    )
    self.assertEqual("RoL Trade Bandit", bandit["target"])
    self.assertEqual("NATIVE_ADAPTED", bandit["strategy"])
    self.assertEqual("RoL Bloodstone Critter", bloodstone_critter["target"])
    self.assertEqual("NATIVE_ADAPTED", bloodstone_critter["strategy"])
    self.assertEqual("RoL Bloodstone Portal", bloodstone_portal["target"])
    self.assertEqual("NATIVE_ADAPTED", bloodstone_portal["strategy"])
    self.assertEqual("RoL Designated Follower", designated_follower["target"])
    self.assertEqual("NATIVE_ADAPTED", designated_follower["strategy"])
    self.assertEqual("RoL Floating Pool", floating_pool["target"])
    self.assertEqual("NATIVE_ADAPTED", floating_pool["strategy"])
    self.assertEqual("RoL Item Blocker", item_blocker["target"])
    self.assertEqual("NATIVE_ADAPTED", item_blocker["strategy"])
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
    self.assertTrue(
        all(row["strategy"] == "NATIVE_RECONCILED" for row in artifact_handlers)
    )
    self.assertTrue(
        all("modern artifact subsystem" in row["target"] for row in artifact_handlers)
    )
    self.assertEqual("SOURCE_UNSAFE_EXCLUDED", unsafe_backdoor["strategy"])
    self.assertIn("permanent-stat", unsafe_backdoor["reason"])
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

  def test_named_guild_and_utility_object_batch_has_explicit_dispositions(self) -> None:
    guild_handlers = (
        "guild_guard_one",
        "guild_guard_four",
        "guild_guard_five",
        "guild_guard_six",
        "guild_guard_eight",
        "guild_guard_nine",
    )
    utility_handlers = (
        "bs_child_sacrifice",
        "fw_ruby_monocle",
        "goodberry_cure",
        "menden_figurine",
        "thp_necroChild",
        "basilisk_leggings",
        "basilisk_snakes",
        "gn_dragoncultrobes",
        "haste_sleeves",
        "magius_staff",
        "moonshae_earthmother_staff",
        "nh_blueplume",
        "nh_writhingash",
        "lathander_disc",
        "llyms_altar",
        "smoke_stun_shield",
        "tiamat_crescent_moon",
        "blackPlagueReservoir",
        "item_loot_block",
    )
    utility_room_handlers = ("newbieLoadRoom", "weight_trigger")

    for handler in guild_handlers:
      disposition = handler_disposition(handler)
      self.assertEqual("resolved", disposition["status"])
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])
      self.assertEqual("RoL Guild Guard", disposition["target"])

    for handler in utility_handlers:
      disposition = handler_disposition(handler)
      self.assertEqual("resolved", disposition["status"])
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])
      self.assertEqual("RoL Utility Object", disposition["target"])

    for handler in utility_room_handlers:
      disposition = handler_disposition(handler)
      self.assertEqual("resolved", disposition["status"])
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])
      self.assertEqual("RoL Utility Room", disposition["target"])

    plague = handler_disposition("blackPlagueCure")
    serpent = handler_disposition("craine_serpent")
    self.assertEqual("SOURCE_INERT_EXCLUDED", plague["strategy"])
    self.assertIn("never registers", plague["reason"])
    self.assertEqual("SOURCE_INERT_EXCLUDED", serpent["strategy"])
    self.assertIn("registers no", serpent["reason"])

    bow = handler_disposition("hellish_fury_bow")
    bomb = handler_disposition("nuclear_bomb")
    self.assertEqual("RoL Weapon Proc", bow["target"])
    self.assertEqual("NATIVE_ADAPTED", bow["strategy"])
    self.assertEqual("SOURCE_INERT_EXCLUDED", bomb["strategy"])
    self.assertIn("no event bits", bomb["reason"])

  def test_scheduled_mobile_batch_has_explicit_dispositions(self) -> None:
    handlers = (
        "gloomhaven_gate_guard",
        "lighthouse_one",
        "naval_three",
        "waterdeep_guard_three",
    )

    for handler in handlers:
      disposition = handler_disposition(handler)
      self.assertEqual("resolved", disposition["status"])
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])
      self.assertEqual("RoL Scheduled Mobile", disposition["target"])

  def test_residual_monster_combat_batch_has_explicit_dispositions(self) -> None:
    handlers = (
        "Tiamat_Crimson_Fury",
        "barbarian_spiritist",
        "dranum_jurtrem",
        "ilshazone_kamerynn",
        "jessica_summon_wisp",
        "jotun_mimer",
        "robyn_summon_servant",
        "robyn_summon_wisp",
        "tako_demon",
        "werewolf_lycan",
        "av_vanish",
        "beavis",
        "butthead",
        "faerie",
        "finn",
        "ilshazone_roll_with_it",
        "wr_ancientBrownie",
    )

    for handler in handlers:
      disposition = handler_disposition(handler)
      self.assertEqual("resolved", disposition["status"])
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])
      self.assertEqual("RoL Monster Combat", disposition["target"])

    clock_tower = handler_disposition("clock_tower")
    self.assertEqual("resolved", clock_tower["status"])
    self.assertEqual("SOURCE_INERT_EXCLUDED", clock_tower["strategy"])
    self.assertIn("no event bits", clock_tower["reason"])

  def test_source_definition_scanner_ignores_preprocessor_disabled_decoy(self) -> None:
    definitions = source_handler_definitions(
        self.root / "EXAMPLE/RealmsOfLuminari", {"tree_spirit"}
    )

    self.assertEqual("src/specs.realm.c", definitions["tree_spirit"]["path"])
    self.assertEqual(188, definitions["tree_spirit"]["line"])

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
      self.assertEqual(1_108, summary["direct_bindings_by_status"]["resolved"])
      self.assertEqual(39, summary["direct_bindings_by_status"]["pending"])
      self.assertEqual(535, summary["source_handlers_by_status"]["resolved"])
      self.assertEqual(27, summary["source_handlers_by_status"]["pending"])
      self.assertEqual(650, summary["direct_bindings_by_strategy"]["NATIVE_ADAPTED"])
      self.assertEqual(
          159, summary["direct_bindings_by_strategy"]["NATIVE_ADAPTED_COMPOSABLE"]
      )
      self.assertEqual(11, summary["direct_bindings_by_strategy"]["NATIVE_RECONCILED"])
      self.assertEqual(
          18, summary["direct_bindings_by_strategy"]["SOURCE_UNSAFE_EXCLUDED"]
      )
      self.assertEqual(848, summary["act_spec_records"])
      self.assertEqual(828, summary["act_spec_by_status"]["resolved"])
      self.assertEqual(20, summary["act_spec_by_status"]["pending"])
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
