from __future__ import annotations

from pathlib import Path
import json
import re
import subprocess
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.flags import decode_tokens
from wtool_lib.hlquests import parse_hlquest_file
from wtool_lib.mobiles import parse_mobile_file
from wtool_lib.objects import parse_object_file
from wtool_lib.rol_source import parse_active_rol_corpus, parse_rol_source_file
from wtool_lib.rol_discovery import extract_source_commands
from wtool_lib.rol_pilot import PILOT_BASENAMES
from wtool_lib.rol_periodic_profiles import COMPOSED_PROFILE_TARGETS, PROFILE_SOURCES
from wtool_lib.rol_state_periodic_profiles import (
    COMPOSED_STATE_PROFILE_SOURCES,
    STATE_PROFILE_SOURCES,
)
from wtool_lib.rol_soc import build_soc_prototype_comparison, compile_soc_records
from wtool_lib.rol_special import compile_special_bindings
from wtool_lib.rol_transform import (
    APPLY_MAP,
    EQUIPMENT_POSITION_MAP,
    SOURCE_INSTRUMENT_SUBTYPE_MAP,
    SOURCE_SAVING_THROW_APPLIES,
    TARGET_APPLY_AC_NEW,
    _NON_CASTABLE_SOURCE_SPELLS,
    _SOURCE_SPELL_MAP,
    classify_source_tail_objects,
    convert_text,
    emit_mobile,
    emit_hlquest,
    emit_object,
    emit_room,
    emit_shop,
    emit_zone,
)
from wtool_lib.rooms import parse_room_file
from wtool_lib.shops import parse_shop_file
from wtool_lib.triggers import parse_trigger_file
from wtool_lib.zones import parse_zone_file


def _resolver(kind: str, vnum: int) -> int:
  offsets = {"wld": 2_000_000, "mob": 2_000_000, "obj": 2_000_000}
  return vnum + offsets[kind]


class RolTransformTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    root = default_repo_root()
    cls.root = root
    cls.manifest = load_manifest(root / "scripts/world/wtool_constants.json")

  def _source_record(self, kind: str, data: bytes):
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / f"sample.{kind}"
    path.write_bytes(data)
    records, corpus = parse_rol_source_file(
        path, f"areas/{kind}/sample.{kind}", kind, "sample"
    )
    self.assertTrue(corpus.complete)
    return records[0]

  def _target_path(self, suffix: str, text: str) -> Path:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / f"20000.{suffix}"
    path.write_text(text + "$~\n", encoding="ascii", newline="\n")
    return path

  def _require_reference_paths(self, *relative_paths: str) -> None:
    missing = [relative for relative in relative_paths if not (self.root / relative).exists()]
    if missing:
      self.skipTest("ignored RoL reference inputs are not installed")

  def test_tarrasque_encounter_bindings_share_typed_contract(self) -> None:
    bindings = [
        {
            "basename": "tarrasque",
            "record_type": "mobile",
            "source_vnum": 2601,
            "source_handler": "tarrasque_swallow_smack",
        },
        {
            "basename": "tarrasque",
            "record_type": "mobile",
            "source_vnum": 2601,
            "source_handler": "tarrasque_die",
        },
        {
            "basename": "tarrasque",
            "record_type": "object",
            "source_vnum": 2610,
            "source_handler": "tarrasque_stomache",
        },
        {
            "basename": "tarrasque",
            "record_type": "object",
            "source_vnum": 2604,
            "source_handler": "tarrasque_corpse_enter",
        },
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    mobile_bindings = [
        binding
        for binding in compiled.native_bindings
        if binding.source_record_type == "mobile"
    ]
    object_bindings = {
        binding.source_vnum: binding
        for binding in compiled.native_bindings
        if binding.source_record_type == "object"
    }

    self.assertEqual(4, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Tarrasque Encounter"
            for binding in compiled.native_bindings
        )
    )
    self.assertEqual(2, len(mobile_bindings))
    self.assertTrue(all(binding.required_flag_bits == (0,) for binding in mobile_bindings))
    self.assertEqual((44,), object_bindings[2610].required_flag_bits)
    self.assertEqual((), object_bindings[2604].required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_exact_class_guild_bindings_reuse_target_family_adapters(self) -> None:
    expected_names = {
        "guild_antipaladin": "RoL Warrior Guild Room",
        "guild_assassin": "RoL Thief Guild Room",
        "guild_cleric": "RoL Cleric Guild Room",
        "guild_conjurer": "RoL Mage Guild Room",
        "guild_druid": "RoL Cleric Guild Room",
        "guild_elementalist": "RoL Mage Guild Room",
        "guild_mercenary": "RoL Warrior Guild Room",
        "guild_monk": "RoL Warrior Guild Room",
        "guild_necromancer": "RoL Mage Guild Room",
        "guild_paladin": "RoL Warrior Guild Room",
        "guild_ranger": "RoL Warrior Guild Room",
        "guild_shaman": "RoL Cleric Guild Room",
        "guild_thief": "RoL Thief Guild Room",
        "guild_warrior": "RoL Warrior Guild Room",
    }
    bindings = [
        {
            "basename": "guild-family",
            "record_type": "room",
            "source_vnum": 46_000 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(expected_names)
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    handlers_by_vnum = {
        46_000 + index: handler for index, handler in enumerate(expected_names)
    }
    compiled_by_handler = {
        handlers_by_vnum[binding.source_vnum]: binding for binding in compiled.native_bindings
    }

    self.assertEqual(14, len(compiled.native_bindings))
    self.assertEqual(expected_names.keys(), compiled_by_handler.keys())
    for handler, persisted_name in expected_names.items():
      binding = compiled_by_handler[handler]
      self.assertEqual(persisted_name, binding.persisted_name)
      self.assertEqual((), binding.required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_planar_base_bindings_use_composable_runtime_contracts(self) -> None:
    bindings = [
        {
            "basename": "planar",
            "record_type": "mobile",
            "source_vnum": 205,
            "source_handler": "abyssForgedWeapons",
        },
        {
            "basename": "undermountain",
            "record_type": "mobile",
            "source_vnum": 92079,
            "source_handler": "standardDemon",
        },
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    native = {binding.source_vnum: binding for binding in compiled.native_bindings}
    dispositions = {row["source_handler"]: row for row in compiled.dispositions}

    self.assertEqual(2, len(native))
    self.assertIsNone(native[205].persisted_name)
    self.assertEqual((125,), native[205].required_flag_bits)
    self.assertIsNone(native[92079].persisted_name)
    self.assertEqual((), native[92079].required_flag_bits)
    self.assertEqual(
        "MOB_ROL_DEMON composition-safe runtime hook",
        dispositions["standardDemon"]["target"],
    )
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE"
            for row in compiled.dispositions
        )
    )

  def test_planar_static_initializers_use_prototype_state_or_inert_dispositions(self) -> None:
    bindings = [
        {
            "basename": "planar",
            "record_type": "mobile",
            "source_vnum": vnum,
            "source_handler": handler,
        }
        for vnum, handler in (
            (205, "demon_aluFiendRegen"),
            (208, "demon_bar_lgura"),
            (209, "demon_cambion"),
            (211, "demon_dretch"),
            (219, "demon_rutterkin"),
            (229, "devilLemure"),
            (230, "devilLemure"),
            (92079, "demon_cambion"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    native = {binding.source_vnum: binding for binding in compiled.native_bindings}
    dispositions = {row["source_handler"]: row for row in compiled.dispositions}

    self.assertEqual({208, 209, 229, 230, 92079}, native.keys())
    self.assertEqual((112,), native[208].required_flag_bits)
    self.assertEqual((20,), native[208].required_affect_bits)
    for vnum in (209, 92079):
      self.assertEqual((112,), native[vnum].required_flag_bits)
      self.assertEqual((19,), native[vnum].required_affect_bits)
    for vnum in (229, 230):
      self.assertEqual((13,), native[vnum].required_flag_bits)
      self.assertEqual((), native[vnum].required_affect_bits)
    self.assertTrue(
        all(
            dispositions[handler]["strategy"] == "SOURCE_INERT_EXCLUDED"
            for handler in ("demon_aluFiendRegen", "demon_dretch", "demon_rutterkin")
        )
    )

    source = self._source_record(
        "mob",
        b"#208\nbar-lgura~\na bar-lgura~\nA bar-lgura waits here.~\n~\n"
        b"0 0 0 0 S\nX 0 0\n10 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_000_208,
        special_resolved=True,
        required_action_bits=native[208].required_flag_bits,
        required_affect_bits=native[208].required_affect_bits,
    )
    result = parse_mobile_file(
        self._target_path("mob", emitted.text), "mob/20000.mob", self.manifest, set()
    )

    self.assertTrue(result.complete)
    self.assertIn(112, decode_tokens(result.records[0].action_flags).bits)
    self.assertIn(20, decode_tokens(result.records[0].affect_flags).bits)

  def test_darkhold_elemental_deaths_use_composable_runtime_profiles(self) -> None:
    bindings = [
        {
            "basename": "darkhold",
            "record_type": "mobile",
            "source_vnum": vnum,
            "source_handler": handler,
        }
        for vnum, handler in (
            (94501, "fire_die"),
            (94502, "air_die"),
            (94503, "water_die"),
            (94504, "earth_die"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(4, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name is None
            and binding.required_flag_bits == ()
            and binding.required_affect_bits == ()
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE"
            and row["target"] == "converted mobile death profile"
            for row in compiled.dispositions
        )
    )

  def test_darkhold_specials_use_exact_composable_runtime_profiles(self) -> None:
    bindings = [
        {
            "basename": "darkhold",
            "record_type": record_type,
            "source_vnum": vnum,
            "source_handler": handler,
        }
        for record_type, vnum, handler in (
            ("object", 94501, "musical_skull_1"),
            ("object", 94502, "musical_skull_1"),
            ("object", 94503, "musical_skull_1"),
            ("object", 94505, "musical_skull_1"),
            ("object", 94506, "musical_skull_1"),
            ("object", 94507, "musical_skull_1"),
            ("object", 94504, "musical_skull_2"),
            ("object", 94508, "ruby_aquamarine"),
            ("object", 94509, "gold_diamond"),
            ("object", 94510, "ruby_aquamarine"),
            ("object", 94511, "gold_diamond"),
            ("object", 94571, "proc_darkhold_warhammer"),
            ("object", 94566, "proc_darkhold_bastard"),
            ("mobile", 94505, "shadow_fiendDarkness"),
            ("mobile", 94505, "shadow_fiendSteal"),
            ("mobile", 94506, "shadow_dragon_die"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    self.assertEqual(16, len(compiled.native_bindings))
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )
    persisted_names = [binding.persisted_name for binding in compiled.native_bindings]
    self.assertEqual(11, persisted_names.count("RoL Darkhold Object"))
    self.assertEqual(2, persisted_names.count("RoL Weapon Proc"))
    self.assertEqual(3, persisted_names.count("RoL Monster Combat"))

    for binding in compiled.native_bindings:
      if binding.persisted_name == "RoL Weapon Proc":
        self.assertEqual((44,), binding.required_flag_bits)
      elif binding.persisted_name == "RoL Monster Combat":
        self.assertEqual((0,), binding.required_flag_bits)
      else:
        self.assertEqual((), binding.required_flag_bits)

  def test_drow_equipment_bindings_share_exact_decay_runtime(self) -> None:
    source_vnums = (
        92080, 92081, 92082, 92096, 93081, 93082, 93083, 93084,
        93085, 93087, 93150, 93151, 93152, 93153, 93154, 93155,
    )
    bindings = [
        {
            "basename": "undermountain",
            "record_type": "object",
            "source_vnum": vnum,
            "source_handler": "genericDrowEq",
        }
        for vnum in source_vnums
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(16, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Drow Equipment"
            and binding.required_flag_bits == (44,)
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

    source = self._source_record(
        "obj",
        b"#92080\ndrow sword~\na drow sword~\nA drow sword lies here.~\n~\n"
        b"5 0 8193\n0 2 6 0\n1000 5 0\n",
    )
    native = compiled.native_bindings[0]
    emitted = emit_object(
        source,
        native.target_vnum,
        _resolver,
        special_proc=native.persisted_name,
        required_extra_bits=native.required_flag_bits,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20920.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Drow Equipment", result.records[0].spec_proc)
    self.assertIn(44, decode_tokens(result.records[0].extra_flags).bits)

  def test_deaths_head_bindings_share_mobile_and_object_runtime(self) -> None:
    bindings = [
        {
            "basename": "undermountain2",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in (
            ("mobile", 93013, "um2_deathsHeadTree"),
            ("mobile", 93014, "um2_deathsHead"),
            ("mobile", 93015, "um2_deathsHeadTree"),
            ("mobile", 93016, "um2_deathsHeadTree"),
            ("object", 93044, "um2_deathsHeadSeed"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(5, len(compiled.native_bindings))
    self.assertTrue(
        all(binding.persisted_name == "RoL Death's Head" for binding in compiled.native_bindings)
    )
    for binding in compiled.native_bindings:
      self.assertEqual(
          (44,) if binding.source_record_type == "object" else (0,),
          binding.required_flag_bits,
      )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_remaining_hit_weapon_bindings_share_typed_weapon_runtime(self) -> None:
    handlers = (
        (96000, "proc_frostbite_cold"),
        (20208, "crystalSword"),
        (20271, "obsidianSword"),
        (21759, "broadsword_dancing_shadows"),
        (93035, "um2_searingRod"),
        (93086, "um2_drowSnakeWhip"),
        (93156, "um2_drowSnakeWhip"),
    )
    bindings = [
        {
            "basename": "hit-weapons",
            "record_type": "object",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(7, len(compiled.native_bindings))
    self.assertTrue(
        all(binding.persisted_name == "RoL Weapon Proc" for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(binding.required_flag_bits == (44,) for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

    source = self._source_record(
        "obj",
        b"#93086\nsnake whip~\na snake whip~\nA snake whip lies here.~\n~\n"
        b"5 0 0\n0 1 5 2\n2 10000 0\n",
    )
    native = next(
        binding for binding in compiled.native_bindings if binding.source_vnum == 93086
    )
    emitted = emit_object(
        source,
        2_093_086,
        _resolver,
        special_proc=native.persisted_name,
        required_extra_bits=native.required_flag_bits,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20930.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Weapon Proc", result.records[0].spec_proc)
    self.assertIn(44, decode_tokens(result.records[0].extra_flags).bits)

  def test_undermountain_forged_bindings_share_typed_weapon_runtime(self) -> None:
    handlers = (
        (93191, "um2_astralForged"),
        (93195, "um2_astralForged"),
        (93446, "um2_torinGeneral"),
        (93447, "um2_torinGeneral"),
        (93447, "um2_torinChainLightning"),
    )
    bindings = [
        {
            "basename": "undermountain-forged",
            "record_type": "object",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(5, len(compiled.native_bindings))
    self.assertTrue(
        all(binding.persisted_name == "RoL Weapon Proc" for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(binding.required_flag_bits == (44,) for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_spiderhaunt_bindings_use_typed_runtime_families(self) -> None:
    bindings = [
        {
            "basename": "spiderhaunt",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in (
            ("mobile", 80220, "shw_hugeWhiteSpider"),
            ("mobile", 80240, "shw_frailDruid"),
            ("object", 80205, "shw_maggots"),
            ("object", 80213, "shw_cyricsAltar"),
            ("object", 80212, "shw_spiderVenomPouch"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    by_target = {
        binding.target_vnum: binding for binding in compiled.native_bindings
    }
    by_handler = {
        source["source_handler"]: by_target[
            _resolver(
                "mob" if source["record_type"] == "mobile" else "obj",
                source["source_vnum"],
            )
        ]
        for source in bindings
    }

    self.assertEqual(5, len(by_handler))
    self.assertEqual("RoL Monster Combat", by_handler["shw_hugeWhiteSpider"].persisted_name)
    self.assertEqual((0,), by_handler["shw_hugeWhiteSpider"].required_flag_bits)
    self.assertEqual("RoL Monster Combat", by_handler["shw_frailDruid"].persisted_name)
    self.assertEqual((0,), by_handler["shw_frailDruid"].required_flag_bits)
    self.assertEqual("RoL Utility Object", by_handler["shw_maggots"].persisted_name)
    self.assertEqual((), by_handler["shw_maggots"].required_flag_bits)
    self.assertEqual("RoL Utility Object", by_handler["shw_cyricsAltar"].persisted_name)
    self.assertEqual((), by_handler["shw_cyricsAltar"].required_flag_bits)
    self.assertEqual("RoL Weapon Proc", by_handler["shw_spiderVenomPouch"].persisted_name)
    self.assertEqual((44,), by_handler["shw_spiderVenomPouch"].required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_undermountain_vortex_knights_use_composable_death_profiles(self) -> None:
    bindings = [
        {
            "basename": "undermountain-vortex-knights",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in (
            (93003, "um2_silverKnight"),
            (93004, "um2_goldenKnight"),
            (93005, "um2_platinumKnight"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(3, len(compiled.native_bindings))
    self.assertTrue(all(binding.persisted_name is None for binding in compiled.native_bindings))
    self.assertTrue(all(binding.required_flag_bits == () for binding in compiled.native_bindings))
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE"
            and row["target"] == "converted mobile death profile"
            for row in compiled.dispositions
        )
    )

  def test_trahern_combat_bindings_share_typed_monster_runtime(self) -> None:
    bindings = [
        {
            "basename": "trahern-combat",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in (
            (20217, "gakarakQuake"),
            (20234, "kazgorothToss"),
            (20246, "erinyesCharm"),
            (20246, "erinyesCharmed"),
            (20248, "slothenEngorge"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])

    self.assertEqual(5, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Monster Combat"
            and binding.required_flag_bits == (0,)
            and binding.required_affect_bits == ()
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_planar_death_burst_and_balor_weapon_bindings_are_explicit(self) -> None:
    bindings = [
        {
            "basename": "planar",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in (
            ("mobile", 207, "demon_balorDeath"),
            ("mobile", 210, "demon_chasmeBuzz"),
            ("mobile", 212, "demon_glabrezuGrab"),
            ("mobile", 214, "demon_manesDeath"),
            ("mobile", 215, "demon_marilithTail"),
            ("mobile", 220, "demon_succubusCharm"),
            ("mobile", 220, "demon_succubusCharmed"),
            ("mobile", 221, "demon_vrockDanceOfRuin"),
            ("mobile", 221, "demon_vrockScreech"),
            ("mobile", 221, "demon_vrockSpores"),
            ("mobile", 233, "devil_spinagonFlameSpike"),
            ("object", 93227, "demon_balorWhip"),
            ("object", 93228, "demon_balorLightningSword"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    native = {
        (binding.source_record_type, binding.source_vnum, binding.persisted_name): binding
        for binding in compiled.native_bindings
    }
    dispositions = {row["source_handler"]: row for row in compiled.dispositions}

    self.assertEqual(12, len(compiled.native_bindings))
    self.assertEqual("SOURCE_INERT_EXCLUDED", dispositions["demon_chasmeBuzz"]["strategy"])
    self.assertIn("cannot run", dispositions["demon_chasmeBuzz"]["reason"])
    balor = native[("mobile", 207, "RoL Monster Combat")]
    self.assertEqual((0,), balor.required_flag_bits)
    self.assertEqual((28,), balor.required_affect_bits)
    for source_vnum in (212, 214, 215, 220, 221, 233):
      binding = native[("mobile", source_vnum, "RoL Monster Combat")]
      self.assertEqual((0,), binding.required_flag_bits)
    for source_vnum in (93227, 93228):
      binding = native[("object", source_vnum, "RoL Weapon Proc")]
      self.assertEqual((2, 7, 16, 44), binding.required_flag_bits)
      self.assertEqual((), binding.required_affect_bits)
    for handler in (
        "demon_balorDeath",
        "demon_glabrezuGrab",
        "demon_manesDeath",
        "demon_marilithTail",
        "demon_succubusCharm",
        "demon_succubusCharmed",
        "demon_vrockDanceOfRuin",
        "demon_vrockScreech",
        "demon_vrockSpores",
        "devil_spinagonFlameSpike",
        "demon_balorWhip",
        "demon_balorLightningSword",
    ):
      self.assertEqual("NATIVE_ADAPTED", dispositions[handler]["strategy"])

  def test_avernus_devil_combat_bindings_compose_by_mobile_identity(self) -> None:
    bindings = [
        {
            "basename": "avernus",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in (
            ("mobile", 32622, "dragon_shout"),
            ("mobile", 32622, "guild_guard"),
            ("mobile", 32629, "barbazu_berserk"),
            ("mobile", 33015, "gelugon_tail_freeze"),
            ("mobile", 33015, "avernus_gelugon_meritos"),
            ("mobile", 33016, "gelugon_tail_freeze"),
            ("mobile", 33016, "avernus_gelugon_hanariel"),
            ("object", 32602, "barbazu_glaive"),
            ("object", 33012, "gelugon_freeze_spear"),
        )
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    dispositions = {row["source_handler"]: row for row in compiled.dispositions}

    self.assertEqual(9, len(compiled.native_bindings))
    self.assertIsNone(compiled.native_bindings[0].persisted_name)
    self.assertEqual("RoL Guild Guard", compiled.native_bindings[1].persisted_name)
    for binding in compiled.native_bindings[2:7]:
      self.assertEqual("RoL Monster Combat", binding.persisted_name)
      self.assertEqual((0,), binding.required_flag_bits)
    for binding in compiled.native_bindings[7:]:
      self.assertEqual("RoL Weapon Proc", binding.persisted_name)
      self.assertEqual((44,), binding.required_flag_bits)

    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", dispositions["dragon_shout"]["strategy"])
    self.assertEqual(
        "RoL Guild Guard plus RoL alert runtime profile",
        dispositions["dragon_shout"]["target"],
    )
    for handler in (
        "barbazu_berserk",
        "gelugon_tail_freeze",
        "avernus_gelugon_meritos",
        "avernus_gelugon_hanariel",
        "barbazu_glaive",
        "gelugon_freeze_spear",
    ):
      self.assertEqual("NATIVE_ADAPTED", dispositions[handler]["strategy"])

  def test_remaining_avernus_bindings_use_composed_runtime_profiles(self) -> None:
    rows = [
        ("mobile", 32623, "avernus_man"),
        ("mobile", 32641, "mob_patrol"),
        ("mobile", 32643, "mob_patrol"),
        ("mobile", 32654, "rehide"),
        ("mobile", 32659, "rehide"),
        ("mobile", 32660, "avernus_prisoner_return"),
        ("mobile", 33000, "mob_patrol"),
        ("mobile", 33003, "erinyes_death"),
        ("mobile", 33005, "avernus_deva_echos"),
        ("mobile", 33008, "mob_patrol"),
        ("mobile", 33014, "bel"),
        ("mobile", 33020, "rehide"),
        ("mobile", 33021, "mob_patrol"),
        ("mobile", 33026, "avernus_black_altar"),
        ("mobile", 33027, "dancing_dagger_mob"),
        ("object", 32631, "avernus_Rod"),
        ("object", 33006, "avernus_seal_unload"),
        ("object", 33011, "bel_flaming_sword"),
        ("object", 33021, "dancing_dagger_obj"),
        ("object", 33025, "dancing_dagger_obj"),
        ("room", 32672, "garden_room"),
    ]
    bindings = [
        {
            "basename": "avernus",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in rows
    ]

    compiled = compile_special_bindings(bindings, 2_100_000, _resolver, [])
    dispositions = compiled.dispositions

    self.assertEqual(20, len(compiled.native_bindings))
    self.assertEqual(0, len(compiled.triggers))
    for binding in compiled.native_bindings:
      if binding.source_record_type == "mobile":
        self.assertEqual("RoL Monster Combat", binding.persisted_name)
        self.assertEqual((0,), binding.required_flag_bits)
      elif binding.source_record_type == "object":
        self.assertEqual("RoL Avernus Object", binding.persisted_name)
        self.assertEqual((44,), binding.required_flag_bits)
      else:
        self.assertEqual("RoL Avernus Garden", binding.persisted_name)
        self.assertEqual((), binding.required_flag_bits)

    inert = [
        row for row in dispositions if row["source_handler"] == "avernus_seal_unload"
    ]
    self.assertEqual(1, len(inert))
    self.assertEqual("SOURCE_INERT_EXCLUDED", inert[0]["strategy"])
    self.assertIn("never parses", inert[0]["reason"])
    for row in dispositions:
      if row["source_handler"] != "avernus_seal_unload":
        self.assertEqual("NATIVE_ADAPTED", row["strategy"])

  def test_color_and_line_endings_are_canonicalized(self) -> None:
    text, diagnostics = convert_text("&+RRed&N\r\nplain")
    self.assertEqual("@RRed@n\nplain", text)
    self.assertEqual([], diagnostics)

  def test_emitted_room_parses_as_modern_target_data(self) -> None:
    source = self._source_record(
        "wld",
        b"<*> File Version 1 <*>\n#100\n&+RRoom&N~\nDescription~\n"
        b"1 26 2 5 5 5 0\nD0\nDoor~\nkey~\n449 200 101\n"
        b"E\nsign~\nA sign.~\nR 15 35\nF 30\nM 0 20\nS\n",
    )
    emitted = emit_room(source, 2_000_100, 20_001, _resolver)
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())
    self.assertTrue(result.complete)
    self.assertEqual(1, len(result.records))
    room = result.records[0]
    self.assertEqual(20_001, room.file_zone)
    self.assertEqual(2_000_101, room.exits[0].destination_vnum)
    self.assertEqual(2_000_200, room.exits[0].key_vnum)
    self.assertEqual(8, room.exits[0].door_flags)
    self.assertEqual(15, room.minimum_level)
    self.assertEqual(-1, room.maximum_level)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("fall chance", diagnostics)
    self.assertIn("obsolete source room mana", diagnostics)
    self.assertIn("target maximum level is 34", diagnostics)

  def test_emitted_room_persists_resolved_special_name(self) -> None:
    source = self._source_record(
        "wld",
        b"#100\nGuild room~\nA trainer works here.~\n1 0 0\nS\n",
    )

    emitted = emit_room(
        source,
        2_000_100,
        20_001,
        _resolver,
        special_proc="Guild",
    )
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertEqual("Guild", result.records[0].spec_proc)

  def test_emitted_astral_room_persists_source_plane_identity(self) -> None:
    source = self._source_record(
        "wld",
        b"#19701\nAstral room~\nSilver emptiness stretches away.~\n0 0 23\nS\n",
    )

    emitted = emit_room(source, 2_019_701, 20_197, _resolver)
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20197.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertEqual(18, result.records[0].sector)
    self.assertIn(47, decode_tokens(result.records[0].flags).bits)

  def test_inert_mobile_special_does_not_emit_flag_or_pending_diagnostic(self) -> None:
    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#100\ncity guard~\na city guard~\n"
        b"A city guard stands here.\n~\nA watchful city guard.\n~\n"
        b"1 0 0 0 S\nH 0 0\n1 0 10 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )

    emitted = emit_mobile(source, 2_000_100, special_resolved=True)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

    self.assertNotIn("source ACT_SPEC deferred", "\n".join(emitted.diagnostics))
    self.assertTrue(result.complete)
    self.assertNotIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_emitted_room_adapts_valid_exit_trap_payload(self) -> None:
    source = self._source_record(
        "wld",
        b"#100\nTrapped door~\nA trapped doorway.~\n1 0 0\n"
        b"D0\nA dangerous door.~\ndoor~\n18 0 101 1 10 1 50 1 -40 100\nS\n",
    )

    emitted = emit_room(source, 2_000_100, 20_001, _resolver)
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertEqual(1, len(result.records[0].rol_exit_traps))
    trap = result.records[0].rol_exit_traps[0]
    self.assertEqual((0, 1, 10), (trap.direction, trap.state, trap.trap_type))
    self.assertEqual((1, 50, 1, -40, 100),
                     (trap.minimum_damage, trap.maximum_damage, trap.area_effect,
                      trap.hardness, trap.load_percent))
    self.assertIn("adapted legacy exit trap payload", " ".join(emitted.diagnostics))

  def test_emitted_room_excludes_malformed_exit_trap_payload(self) -> None:
    source = self._source_record(
        "wld",
        b"#100\nBroken trap~\nA malformed doorway.~\n1 0 0\n"
        b"D3\n~\n~\n0 0 101 20 20 20\nS\n",
    )

    emitted = emit_room(source, 2_000_100, 20_001, _resolver)

    self.assertNotIn("\nY ", emitted.text)
    self.assertIn("excluded malformed legacy exit trap payload", " ".join(emitted.diagnostics))

  def test_emitted_room_preserves_room_and_zone_compatibility(self) -> None:
    first_mask = sum(1 << (flag - 1) for flag in (6, 11, 13, 15, 18, 31, 32))
    second_mask = sum(1 << (flag - 33) for flag in (36, 48))
    source = self._source_record(
        "wld",
        (
            "<*> File Version 1 <*>\n#100\nCompatibility room~\nDescription~\n"
            f"1 {first_mask} 2 5 5 5 {second_mask}\nS\n"
        ).encode("ascii"),
    )
    zone_flags = sum(1 << bit for bit in (0, 1, 4, 5, 6, 7))
    emitted = emit_room(
        source,
        2_000_100,
        20_001,
        _resolver,
        source_zone_flags=zone_flags,
    )
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20001.wld", self.manifest, False, set())
    self.assertTrue(result.complete)
    room = result.records[0]
    flags = decode_tokens(room.flags).bits

    self.assertEqual(9, room.sector)
    self.assertTrue({4, 5, 17, 19, 21, 23, 24, 27, 42, 43, 44, 45} <= flags)
    self.assertNotIn(7, flags)
    self.assertNotIn("room flags without target persistence", " ".join(emitted.diagnostics))

  def test_emitted_mobile_maps_flags_position_class_and_race(self) -> None:
    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#300\nspider~\na spider~\nA spider waits.\n~\n"
        b"A spider.\n~\n38 8 0 -100 S\nAS 0 0\n10 2 50 2d8+5 1d4+1\n"
        b"1.2.3.4 500\n131 131 2 6\n",
    )
    emitted = emit_mobile(source, 2_000_300)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())
    self.assertTrue(result.complete)
    mobile = result.records[0]
    self.assertEqual(9, mobile.position)
    self.assertEqual(1, int(mobile.enhanced["Class"][0]))
    self.assertEqual(15, int(mobile.enhanced["Race"][0]))
    self.assertEqual(0, mobile.gold)
    self.assertIn(39, decode_tokens(mobile.action_flags).bits)
    self.assertEqual(6, mobile.level)
    self.assertEqual(0, int(mobile.enhanced["SubRace 1"][0]))
    self.assertIn("Size", mobile.enhanced)
    self.assertEqual(0, int(mobile.enhanced["Tier"][0]))

  def test_emitted_mobile_maps_rol_secondary_and_source_only_affects(self) -> None:
    first_affects = (1 << (16 - 1)) | (1 << (24 - 1))
    second_affects = (1 << (48 - 33)) | (1 << (61 - 33))
    source = self._source_record(
        "mob",
        (
            "<*> File Version 1 <*>\n#300\nmeditator~\na meditator~\n"
            "A meditator waits.\n~\nA quiet meditator.\n~\n"
            f"0 {first_affects} {second_affects} 0 S\n"
            "PH 0 0\n10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n"
        ).encode("ascii"),
    )

    emitted = emit_mobile(source, 2_000_300)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

    self.assertTrue(result.complete)
    mobile = result.records[0]
    self.assertEqual({121}, decode_tokens(mobile.affect_flags).bits)
    self.assertEqual({3, 4}, decode_tokens(mobile.affect2_flags).bits)
    self.assertIn("omitted source transient/inert mobile affects: [48]", emitted.diagnostics)

  def test_emitted_mobile_persists_composable_race_behavior_and_identity(self) -> None:
    cases = {
        "X": ({116}, {11, 28}),
        "Y": ({117}, {11, 28}),
        "MH": ({118}, {28}),
        "Z": ({122}, set()),
        "OB": ({126}, set()),
    }
    for race_code, (expected_actions, expected_affects) in cases.items():
      with self.subTest(race_code=race_code):
        source = self._source_record(
            "mob",
            (
                "<*> File Version 1 <*>\n#300\nplanar mobile~\na planar mobile~\n"
                "A planar mobile waits.\n~\nA planar mobile.\n~\n"
                f"0 0 0 0 S\n{race_code} 0 0\n"
                "10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n"
            ).encode("ascii"),
        )

        emitted = emit_mobile(source, 2_000_300, special_proc="RoL Thief")
        path = self._target_path("mob", emitted.text)
        result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

        self.assertTrue(result.complete)
        mobile = result.records[0]
        self.assertTrue(expected_actions <= decode_tokens(mobile.action_flags).bits)
        self.assertTrue(expected_affects <= decode_tokens(mobile.affect_flags).bits)
        self.assertEqual("RoL Thief", mobile.spec_proc)

  def test_call_lycanthrope_prototypes_preserve_their_summon_role(self) -> None:
    for source_vnum in (525, 526):
      with self.subTest(source_vnum=source_vnum):
        source = self._source_record(
            "mob",
            (
                f"#{source_vnum}\nlycanthrope~\na lycanthrope~\n"
                "A lycanthrope waits here.~\n~\n"
                "0 0 0 0 S\nL 0 0\n"
                "20 0 0 1d1+0 1d1+0\n0 0\n131 131 0\n"
            ).encode("ascii"),
        )
        emitted = emit_mobile(source, 2_000_000 + source_vnum)
        path = self._target_path("mob", emitted.text)
        result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

        self.assertTrue(result.complete)
        self.assertIn(127, decode_tokens(result.records[0].action_flags).bits)

  def test_emitted_mobile_maps_all_action_dispositions_and_infers_primary_class(self) -> None:
    source_actions = (1, 4, 5, 13, 14, 16, 17, 19, 20, 21, 22, 23, 24, 26, 27, 29, 30, 32)
    action_mask = sum(1 << (flag - 1) for flag in source_actions)
    source = self._source_record(
        "mob",
        (
            "<*> File Version 1 <*>\n#300\ncompatibility mobile~\n"
            "a compatibility mobile~\nA compatibility mobile waits.\n~\n"
            "A compatibility mobile.\n~\n"
            f"{action_mask} 0 0 0 S\n"
            "H 0 0\n10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n"
        ).encode("ascii"),
    )

    emitted = emit_mobile(source, 2_000_300)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

    self.assertTrue(result.complete)
    mobile = result.records[0]
    self.assertEqual(21, int(mobile.enhanced["Class"][0]))
    self.assertTrue(
        {3, 12, 13, 31, 34, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115}
        <= decode_tokens(mobile.action_flags).bits
    )
    diagnostics = " ".join(emitted.diagnostics)
    self.assertNotIn("requiring behavior reconciliation", diagnostics)
    self.assertIn("deferred to Phase 6", diagnostics)
    self.assertIn("relationship/inert mobile actions: [14, 24]", diagnostics)

  def test_emitted_object_maps_extended_stats_and_repairs_source_defects(self) -> None:
    wear_mask = sum(1 << bit for bit in (0, 14, 25, 27))
    source = self._source_record(
        "obj",
        (
            "#200\ncompatibility relic~\na compatibility relic~\n"
            "A compatibility relic is here.~\n~\n"
            f"8388672 64 {wear_mask}\n0 0 0 0\n1 1 0\n"
            "A\n26 2\nA\n27 1\nA\n31 2\nA\n41 4\nA\n29 5\n"
        ).encode("ascii"),
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual(12, obj.item_type)
    self.assertEqual({0, 14}, decode_tokens(obj.wear_flags).bits)
    self.assertEqual(
        [(2, 2), (4, 1), (1, 2)],
        [(affect.location, affect.modifier) for affect in obj.affects],
    )
    self.assertEqual([23, 23, 23], [affect.bonus_type for affect in obj.affects])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("omitted malformed source object wear flags: [25, 27]", diagnostics)
    self.assertIn("omitted source-only object apply 29", diagnostics)
    self.assertIn("omitted source-only object apply 41", diagnostics)
    self.assertNotIn("unknown source item type", diagnostics)

  def test_source_tail_ring_uses_runtime_tail_eligibility(self) -> None:
    wear_mask = sum(1 << bit for bit in (0, 1, 22))
    source = self._source_record(
        "obj",
        (
            "#200\nring tail~\na tail-capable ring~\n"
            "A tail-capable ring is here.~\n~\n"
            f"9 0 {wear_mask}\n0 0 0 0\n1 1 0\n"
        ).encode("ascii"),
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual({0, 1}, decode_tokens(result.records[0].wear_flags).bits)
    self.assertIn(
        "runtime ring handling provides tail eligibility",
        " ".join(emitted.diagnostics),
    )

  def test_source_non_ring_tail_item_becomes_dedicated_tail_gear(self) -> None:
    wear_mask = sum(1 << bit for bit in (0, 5, 22))
    source = self._source_record(
        "obj",
        (
            "#201\ntail plates~\na set of tail plates~\n"
            "A set of tail plates is here.~\n~\n"
            f"9 0 {wear_mask}\n6 0 0 0\n1 1 0\n"
        ).encode("ascii"),
    )

    emitted = emit_object(source, 2_000_201, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual({0, 34}, decode_tokens(result.records[0].wear_flags).bits)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("normalized source non-ring tail item", diagnostics)
    self.assertIn("normalized conflicting target wear flags", diagnostics)

  def test_converted_text_escapes_literal_at_for_target_color_parser(self) -> None:
    text, diagnostics = convert_text("mail me @ &+rdawn&N")

    self.assertEqual("mail me @@ @rdawn@n", text)
    self.assertIn("escaped literal '@'", " ".join(diagnostics))

  def test_converted_text_leaves_color_free_source_untouched(self) -> None:
    text, diagnostics = convert_text("a plain iron axe")

    self.assertEqual("a plain iron axe", text)
    self.assertEqual([], diagnostics)

  def test_emitted_weapon_translates_source_damage_message_index(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nclub bludgeon~\na heavy club~\nA heavy club is here.~\n~\n"
        b"5 0 8193\n0 3 8 7\n15 100 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual(5, obj.item_type)
    # Source 7 is RoL weapons[6] "Bludgeon"; the target's "bludgeon" is 5.
    self.assertEqual(5, obj.values[3])
    self.assertIn("mapped source weapon damage message 7", " ".join(emitted.diagnostics))

  def test_emitted_weapon_keeps_already_aligned_damage_message(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nsword slash~\na long sword~\nA long sword is here.~\n~\n"
        b"5 0 8193\n0 1 12 3\n15 100 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(3, result.records[0].values[3])
    self.assertNotIn("weapon damage message", " ".join(emitted.diagnostics))

  def test_emitted_weapon_replaces_out_of_range_damage_message(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nodd weapon~\nan odd weapon~\nAn odd weapon is here.~\n~\n"
        b"5 0 8193\n0 1 6 12\n15 100 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(0, result.records[0].values[3])
    self.assertIn(
        "replaced out-of-range source weapon damage message 12",
        " ".join(emitted.diagnostics),
    )

  def test_emitted_drink_container_converts_quarter_pound_weight(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nflask water~\na leather flask~\nA leather flask is here.~\n~\n"
        b"17 0 1\n10 3 0 0\n20 25 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(17, result.records[0].item_type)
    self.assertEqual(5, result.records[0].weight)
    self.assertIn(
        "converted source drink-container weight 20", " ".join(emitted.diagnostics)
    )

  def test_emitted_object_weight_is_untouched_for_non_drink_containers(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nstatue~\na stone statue~\nA stone statue is here.~\n~\n"
        b"12 0 1\n0 0 0 0\n20 25 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(20, result.records[0].weight)

  def test_emitted_object_maps_charisma_and_primary_stats_1_to_1(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ncharm ring~\na charm ring~\nA charm ring is here.~\n~\n"
        b"11 0 3\n0 0 0 0\n1 100 0\n"
        b"A\n28 2\nA\n1 2\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    affects = {
        affect.location: affect.modifier for affect in result.records[0].affects
    }
    self.assertEqual(2, affects[6])
    self.assertEqual(2, affects[1])
    self.assertTrue(
        all(affect.bonus_type == 23 for affect in result.records[0].affects)
    )

  def test_emitted_object_preserves_negative_stat_modifier_1_to_1(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ncursed band~\na cursed band~\nA cursed band is here.~\n~\n"
        b"11 0 3\n0 0 0 0\n1 100 0\n"
        b"A\n1 -1\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(-1, result.records[0].affects[0].modifier)
    self.assertEqual(23, result.records[0].affects[0].bonus_type)

  def test_emitted_object_drops_source_inert_hide_affect(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ncloak shadow~\na shadowed cloak~\nA shadowed cloak is here.~\n~\n"
        b"11 0 1025\n0 0 0 0\n1 100 0\n"
        + f"{1 << 20}\n0\n".encode("ascii"),
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    # Target AFF_HIDE is 20; the source loader clears the source bit at load,
    # so the converted object must not confer it.
    self.assertNotIn(20, decode_tokens(result.records[0].affect_flags).bits)
    self.assertIn(
        "omitted source-inert object affects", " ".join(emitted.diagnostics)
    )

  def test_emitted_object_synthesizes_runtime_safe_identity_strings(self) -> None:
    source = self._source_record(
        "obj",
        b"#2\ncorpse~\n~\n~\n~\n24 0 0\n0 0 0 0\n200 0 0\n",
    )

    emitted = emit_object(source, 2_000_002, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20000.obj", self.manifest, set())

    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual("corpse", obj.aliases)
    self.assertEqual("corpse", obj.short_description)
    self.assertEqual("corpse is here.", obj.description)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("synthesized missing object short description", diagnostics)
    self.assertIn("synthesized missing object room description", diagnostics)

  def test_source_object_recovers_missing_action_description(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    source_path = Path(temporary.name) / "sample.obj"
    source_path.write_bytes(
        b"#7\nmanual~\na manual~\nA manual lies here.~\n"
        b"23 0 1\n0 0 0 0\n1 1000 0\n"
    )

    records, corpus = parse_rol_source_file(
        source_path, "areas/obj/sample.obj", "obj", "sample"
    )

    self.assertTrue(corpus.complete)
    self.assertEqual(23, records[0].values["item_type"])
    self.assertEqual("", records[0].values["strings"]["action_description"])
    self.assertTrue(any(item.code == "ROLOBJ005" for item in corpus.diagnostics))

  def test_source_object_recovers_pre_header_extra_description(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    source_path = Path(temporary.name) / "sample.obj"
    source_path.write_bytes(
        b"#1034\nkey~\na translucent key~\nA translucent key lies here.~\n~\n"
        b"E\nkey~\nA nearly transparent skeleton key.~\n"
        b"18 0 1\n0 0 0 0\n1 1 1\n"
    )

    records, corpus = parse_rol_source_file(
        source_path, "areas/obj/sample.obj", "obj", "sample"
    )

    self.assertTrue(corpus.complete)
    self.assertEqual(18, records[0].values["item_type"])
    extras = [row for row in records[0].directives if row["token"] == "E"]
    self.assertEqual(1, len(extras))
    self.assertEqual("key", extras[0]["keyword"])
    self.assertTrue(any(item.code == "ROLOBJ006" for item in corpus.diagnostics))

  def test_emitted_room_repairs_missing_mountain_sector(self) -> None:
    source = self._source_record(
        "wld",
        b"<*> File Version 1 <*>\n#50537\nMountain road~\nDescription~\n"
        b"505 4194304 100 150 500\nS\n",
    )

    emitted = emit_room(source, 2_050_537, 20_505, _resolver)
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20505.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertEqual(5, result.records[0].sector)

  def test_emitted_object_retargets_source_armor_apply_to_apply_ac_new(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nplated hauberk~\na plated hauberk~\nA plated hauberk is here.~\n~\n"
        b"9 0 1\n0 0 0 0\n1 1 0\n0\n0\n"
        b"A\n17 -50\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    affect = result.records[0].affects[0]
    self.assertEqual(TARGET_APPLY_AC_NEW, affect.location)
    self.assertEqual(5, affect.modifier)
    self.assertEqual(23, affect.bonus_type)
    self.assertIn(
        "restated source armor apply -50 as APPLY_AC_NEW 5", " ".join(emitted.diagnostics)
    )

  def test_emitted_object_keeps_one_point_of_small_source_armor_apply(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nfrayed cloak~\na frayed cloak~\nA frayed cloak is here.~\n~\n"
        b"9 0 1\n0 0 0 0\n1 1 0\n0\n0\n"
        b"A\n17 -5\nA\n17 3\nA\n17 0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(
        [(TARGET_APPLY_AC_NEW, 1), (TARGET_APPLY_AC_NEW, -1), (TARGET_APPLY_AC_NEW, 0)],
        [(affect.location, affect.modifier) for affect in result.records[0].affects],
    )
    self.assertEqual(
        [23, 23, 23], [affect.bonus_type for affect in result.records[0].affects]
    )

  def test_emitted_object_inverts_all_source_saving_throw_applies(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwarded cloak~\na warded cloak~\nA warded cloak is here.~\n~\n"
        b"11 0 1\n0 0 0 0\n1 1 0\n0\n0\n"
        b"A\n20 -1\nA\n21 2\nA\n22 -3\nA\n23 4\nA\n24 -5\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(
        [(20, 1), (20, -2), (21, 3), (21, -4), (22, 5)],
        [(affect.location, affect.modifier) for affect in result.records[0].affects],
    )
    self.assertEqual(
        [23, 23, 23, 23, 23],
        [affect.bonus_type for affect in result.records[0].affects],
    )
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("saving-throw apply 20 modifier -1 to 1", diagnostics)
    self.assertIn("saving-throw apply 23 modifier 4 to -4", diagnostics)

  def test_emitted_object_translates_rol_instrument_value_contract(self) -> None:
    source = self._source_record(
        "obj",
        b"#19959\ndrum oaken dragonhide~\na dragonhide drum~\n"
        b"A dragonhide drum is here.~\n~\n32 0 16385\n188 12 12 45\n1 1 0\n",
    )

    emitted = emit_object(source, 2_019_959, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20019.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(38, result.records[0].item_type)
    self.assertEqual([3, 12, 10, 0], result.records[0].values[:4])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("source instrument subtype 188 to target subtype 3 (Drum)", diagnostics)
    self.assertIn("source instrument effectiveness 12 to target maximum 10", diagnostics)
    self.assertIn(
        "source instrument minimum-use level 45 to target breakability 0", diagnostics
    )

  def test_active_object_saving_throw_applies_invert_for_target_sign(self) -> None:
    self._require_reference_paths("EXAMPLE/RealmsOfLuminari/areas")
    corpus = parse_active_rol_corpus(
        self.root / "EXAMPLE/RealmsOfLuminari", self.root
    )
    expected: dict[int, list[tuple[int, int]]] = {}
    emitted_records: list[str] = []

    for record in corpus.records:
      if record.kind != "obj":
        continue
      applies = [
          (APPLY_MAP[int(directive["arguments"][0])], -int(directive["arguments"][1]))
          for directive in record.directives
          if directive["token"] == "A"
          and len(directive.get("arguments", [])) >= 2
          and int(directive["arguments"][0]) in SOURCE_SAVING_THROW_APPLIES
      ]
      if not applies:
        continue
      destination_vnum = 2_000_000 + record.vnum
      expected[destination_vnum] = applies
      emitted_records.append(emit_object(record, destination_vnum, _resolver).text)

    path = self._target_path("obj", "".join(emitted_records))
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(458, len(expected))
    self.assertEqual(490, sum(len(applies) for applies in expected.values()))
    actual = {
        record.vnum: [
            (affect.location, affect.modifier)
            for affect in record.affects
            if affect.location in {20, 21, 22}
        ]
        for record in result.records
    }
    self.assertEqual(expected, actual)

  def test_active_object_instruments_emit_valid_target_values(self) -> None:
    self._require_reference_paths("EXAMPLE/RealmsOfLuminari/areas")
    corpus = parse_active_rol_corpus(
        self.root / "EXAMPLE/RealmsOfLuminari", self.root
    )
    expected_subtypes = {
        184: 1, # Flute
        185: 0, # Lyre
        186: 5, # Mandolin
        187: 4, # Harp
        188: 3, # Drums
        189: 2, # Horn
    }
    inferred_source_defects = {
        1046: 2,  # War Horn of Henekar has spell 299 in its subtype slot.
        22554: 1, # Flute has zero in its subtype slot.
        22575: 3, # Drums have zero in their subtype slot.
        57214: 4, # Harp has zero in its subtype slot.
    }
    expected: dict[int, list[int]] = {}
    emitted_records: list[str] = []
    diagnostics: list[str] = []

    self.assertEqual(expected_subtypes, SOURCE_INSTRUMENT_SUBTYPE_MAP)
    for record in corpus.records:
      if record.kind != "obj" or record.values.get("item_type") != 32:
        continue
      source_values = list(record.values.get("values", []))
      source_values = (source_values + [0] * 4)[:4]
      source_subtype, source_quality, source_effectiveness, source_level = source_values
      target_subtype = expected_subtypes.get(source_subtype)
      if target_subtype is None:
        target_subtype = inferred_source_defects[record.vnum]
      destination_vnum = 2_000_000 + record.vnum
      expected[destination_vnum] = [
          target_subtype,
          max(0, min(source_quality, 30)),
          max(0, min(source_effectiveness, 10)),
          30 - (max(1, min(source_level, 45)) * 30 // 45),
      ]
      emitted = emit_object(record, destination_vnum, _resolver)
      emitted_records.append(emitted.text)
      diagnostics.extend(emitted.diagnostics)

    path = self._target_path("obj", "".join(emitted_records))
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(83, len(expected))
    actual = {record.vnum: record.values[:4] for record in result.records}
    self.assertEqual(expected, actual)
    self.assertEqual([3, 12, 10, 0], actual[2_019_959])
    self.assertEqual(
        {0, 30},
        {
            values[3]
            for destination_vnum, values in actual.items()
            if destination_vnum in {2_019_959, 2_022_554}
        },
    )
    self.assertFalse(
        any("defaulted unsupported source instrument subtype" in item for item in diagnostics)
    )

  def test_emitted_object_maps_container_key_affects_and_apply(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ncontainer box~\na box~\nA box is here.~\n~\n"
        b"15 73 3\n0 0 201 0\n2 10 0\n2\n0\n"
        b"A\n1 2\nE\nbox~\nIt is sturdy.~\n",
    )
    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())
    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual(15, obj.item_type)
    self.assertEqual(2_000_201, obj.values[2])
    self.assertEqual(2, obj.affects[0].modifier)
    self.assertEqual(23, obj.affects[0].bonus_type)
    self.assertEqual(1, len(obj.extra_descriptions))

  def test_emitted_object_preserves_rol_compatibility_flags(self) -> None:
    source_bits = (1, 2, 4, 16, 17, 20, 21, 22, 24, 29, 30)
    source_mask = sum(1 << bit for bit in source_bits)
    source = self._source_record(
        "obj",
        (
            "#200\ncompatibility item~\na compatibility item~\n"
            "A compatibility item is here.~\n~\n"
            f"12 {source_mask} 1\n0 0 0 0\n1 1 0\n0\n0\n"
        ).encode("ascii"),
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    flags = decode_tokens(result.records[0].extra_flags).bits
    self.assertEqual({39, *range(116, 125)}, flags)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("omitted source-inert object DARK flag", diagnostics)
    self.assertNotIn("object extra flags without direct equivalents", diagnostics)

  def test_emitted_object_preserves_source_trap_without_colliding_with_dg_trigger(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ntrapped chest~\na trapped chest~\nA trapped chest is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\n"
        b"T\n2562 11 3 25 2 6\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver, attachments=(2_100_001,))
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    obj = result.records[0]
    self.assertEqual([2562, 11, 3, 25, 2, 6], obj.values[10:16])
    self.assertEqual([2_100_001], [attachment.trigger_vnum for attachment in obj.attachments])
    self.assertEqual(["0", "0", "0", "r"], obj.extra_flags)
    self.assertIn("converted source object trap", " ".join(emitted.diagnostics))

  def test_emitted_object_excludes_empty_source_trap(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nplain box~\na plain box~\nA plain box is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\nT\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual([0] * 6, result.records[0].values[10:16])
    self.assertNotEqual(["0", "0", "0", "r"], result.records[0].extra_flags)
    self.assertIn("inactive/malformed source object trap", " ".join(emitted.diagnostics))

  def test_emitted_object_rejects_invalid_source_trap_damage_type(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\ntrapped box~\na trapped box~\nA trapped box is here.~\n~\n"
        b"15 0 1\n0 0 -1 0\n10 100 0\n0\n0\nT 2 99 1 10 1 6\n",
    )

    with self.assertRaisesRegex(ValueError, "invalid damage type 99"):
      emit_object(source, 2_000_200, _resolver)

  def test_all_active_source_object_traps_have_explicit_dispositions(self) -> None:
    self._require_reference_paths("EXAMPLE/RealmsOfLuminari/areas")
    corpus = parse_active_rol_corpus(self.root / "EXAMPLE/RealmsOfLuminari", self.root)
    trapped = [
        record
        for record in corpus.records
        if record.kind == "obj" and any(row["token"] == "T" for row in record.directives)
    ]
    converted = 0
    excluded = 0

    for ordinal, record in enumerate(trapped):
      emitted = emit_object(record, 2_500_000 + ordinal, _resolver)
      path = self._target_path("obj", emitted.text)
      parsed = parse_object_file(path, "obj/25000.obj", self.manifest, set())
      self.assertTrue(parsed.complete, record.record_id)
      diagnostics = " ".join(emitted.diagnostics)
      if "converted source object trap" in diagnostics:
        converted += 1
        self.assertNotEqual([0] * 6, parsed.records[0].values[10:16])
      else:
        excluded += 1
        self.assertIn("inactive/malformed", diagnostics)

    self.assertEqual(33, len(trapped))
    self.assertEqual(29, converted)
    self.assertEqual(4, excluded)

  def test_emitted_magic_item_caps_source_level_at_target_maximum(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\npotion~\na potion~\nA potion is here.~\n~\n"
        b"10 0 1\n50 9 0 0\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(34, result.records[0].values[0])
    self.assertIn("capped source magic-item spell level 50", " ".join(emitted.diagnostics))

  def test_emitted_magic_item_maps_spells_by_source_name(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\npotion~\na potion~\nA potion is here.~\n~\n"
        b"10 0 1\n20 41 0 0\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(120, result.records[0].values[1])
    self.assertIn("source spell 41 (haste)", " ".join(emitted.diagnostics))

  def test_emitted_wand_maps_bigbys_clenched_fist(self) -> None:
    source = self._source_record(
        "obj",
        b"#19750\nrod dark blue~\na dark-blue rod~\nA dark-blue rod is here.~\n~\n"
        b"3 0 16385\n50 10 10 91\n10 100000 1\n0\n0\n",
    )

    emitted = emit_object(source, 2_019_750, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20197.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(34, result.records[0].values[0])
    self.assertEqual(188, result.records[0].values[3])
    self.assertIn(
        "source spell 91 (bigbys clenched fist) to target spell 188",
        " ".join(emitted.diagnostics),
    )

  def test_emitted_staff_repairs_current_charges_above_maximum(self) -> None:
    source = self._source_record(
        "obj",
        b"#8013\nstaff magi~\nthe staff of the magi~\n"
        b"The staff of the magi lies here.~\n~\n"
        b"4 0 1\n50 1 6 75\n4 10000 6000\n",
    )

    emitted = emit_object(source, 2_008_013, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20080.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(6, result.records[0].values[1])
    self.assertEqual(6, result.records[0].values[2])
    self.assertIn("raised source wand/staff maximum charges", " ".join(emitted.diagnostics))

  def test_emitted_magic_item_maps_mud_to_rock_without_zeroing_slot(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 453\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(581, result.records[0].values[3])
    self.assertIn("mud to rock", " ".join(emitted.diagnostics))

  def test_emitted_magic_item_maps_dragonscales_to_iron_skin(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 327\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(201, result.records[0].values[3])
    self.assertIn(
        "source spell 327 (dragonscales) to target spell 201",
        " ".join(emitted.diagnostics),
    )

  def test_emitted_magic_item_maps_dimensional_fold_to_portal(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 493\n1 1 0\n0\n0\n",
    )

    emitted = emit_object(source, 2_000_200, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20001.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual(216, result.records[0].values[3])
    self.assertIn(
        "source spell 493 (dimensional fold) to target spell 216",
        " ".join(emitted.diagnostics),
    )

  def test_emitted_magic_item_rejects_unmapped_positive_spell(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 999\n1 1 0\n0\n0\n",
    )

    with self.assertRaisesRegex(ValueError, "unmapped positive source spell 999"):
      emit_object(source, 2_000_200, _resolver)

  def test_emitted_magic_item_rejects_non_castable_source_spell_id(self) -> None:
    source = self._source_record(
        "obj",
        b"#200\nwand~\na wand~\nA wand is here.~\n~\n"
        b"3 0 1\n20 2 2 291\n1 1 0\n0\n0\n",
    )

    with self.assertRaisesRegex(
        ValueError,
        r"non-castable source spell ID 291 \(elemental embodiment maintain\)",
    ):
      emit_object(source, 2_000_200, _resolver)

  def test_spell_map_targets_registered_luminari_spells(self) -> None:
    spell_header = (self.root / "src/magic/spells.h").read_text(encoding="ascii")
    num_spells_match = re.search(
        r"^#define\s+NUM_SPELLS\s+(\d+)\b", spell_header, re.MULTILINE
    )
    self.assertIsNotNone(num_spells_match)
    num_spells = int(num_spells_match.group(1))
    preprocessed = subprocess.run(
        [
            "cc",
            "-E",
            "-P",
            f"-I{self.root / 'src'}",
            str(self.root / "src/magic/spell_parser.c"),
        ],
        cwd=self.root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    target_registry: dict[int, str] = {}
    for number, name in re.findall(
        r'spello\((\d+),\s*"([^"]+)"', preprocessed
    ):
      spell_number = int(number)
      if 0 < spell_number < num_spells:
        target_registry[spell_number] = name

    self.assertEqual(331, len(_SOURCE_SPELL_MAP))
    self.assertEqual(
        {64, 93, 291, 292, 293, 294, 361, 374, 498},
        set(_NON_CASTABLE_SOURCE_SPELLS),
    )
    self.assertFalse(set(_SOURCE_SPELL_MAP) & set(_NON_CASTABLE_SOURCE_SPELLS))
    self.assertEqual(599, _SOURCE_SPELL_MAP[233][1])
    self.assertEqual(599, _SOURCE_SPELL_MAP[314][1])
    self.assertEqual(600, _SOURCE_SPELL_MAP[377][1])
    self.assertEqual(601, _SOURCE_SPELL_MAP[378][1])
    self.assertEqual(602, _SOURCE_SPELL_MAP[473][1])
    self.assertEqual(603, _SOURCE_SPELL_MAP[90][1])
    self.assertEqual(604, _SOURCE_SPELL_MAP[303][1])
    self.assertEqual(605, _SOURCE_SPELL_MAP[487][1])
    self.assertEqual(606, _SOURCE_SPELL_MAP[488][1])
    self.assertEqual(607, _SOURCE_SPELL_MAP[343][1])
    self.assertEqual(608, _SOURCE_SPELL_MAP[376][1])
    self.assertEqual(609, _SOURCE_SPELL_MAP[482][1])
    self.assertEqual(610, _SOURCE_SPELL_MAP[483][1])
    self.assertEqual(611, _SOURCE_SPELL_MAP[484][1])
    self.assertEqual(612, _SOURCE_SPELL_MAP[485][1])
    for source_spell, (source_name, target_spell) in _SOURCE_SPELL_MAP.items():
      with self.subTest(source_spell=source_spell, source_name=source_name):
        self.assertGreater(target_spell, 0)
        self.assertLess(target_spell, num_spells)
        self.assertIn(target_spell, target_registry)
        self.assertNotEqual("!UNUSED!", target_registry[target_spell])

  def test_spell_map_covers_all_rol_spells_and_active_magic_items(self) -> None:
    self._require_reference_paths(
        "EXAMPLE/RealmsOfLuminari/areas",
        "EXAMPLE/RealmsOfLuminari/src/sparser.c",
        "EXAMPLE/RealmsOfLuminari/src/spells.h",
    )
    source_root = self.root / "EXAMPLE/RealmsOfLuminari"
    preprocessed = subprocess.run(
        [
            "cc",
            "-E",
            "-P",
            f"-I{source_root / 'src'}",
            str(source_root / "src/sparser.c"),
        ],
        cwd=self.root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    source_registry = {
        int(number): name
        for name, number in re.findall(
            r'SPELL_CREATE\("([^"]+)",\s*(\d+)\s*,', preprocessed
        )
    }
    source_header = (source_root / "src/spells.h").read_text(encoding="ascii")
    source_spell_constants = {
        int(number)
        for number in re.findall(
            r"^#define\s+SPELL_[A-Z0-9_]+\s+(\d+)\b",
            source_header,
            re.MULTILINE,
        )
        if int(number) > 0
    }
    corpus = parse_active_rol_corpus(source_root, self.root)
    source_type_slots = {2: (1, 2, 3), 3: (3,), 4: (3,), 10: (1, 2, 3)}
    source_type_counts = {source_type: 0 for source_type in source_type_slots}
    referenced_source_spells: set[int] = set()
    expected_spell_slots: dict[tuple[int, int], int] = {}
    emitted_records: list[str] = []

    self.assertTrue(corpus.complete)
    for record in corpus.records:
      if record.kind != "obj":
        continue
      source_type = int(record.values.get("item_type") or 0)
      if source_type not in source_type_slots:
        continue
      source_type_counts[source_type] += 1
      source_values = (list(record.values.get("values", [])) + [0] * 4)[:4]
      destination_vnum = 2_600_000 + len(emitted_records)
      for slot in source_type_slots[source_type]:
        source_spell = source_values[slot]
        if source_spell <= 0:
          continue
        referenced_source_spells.add(source_spell)
        expected_spell_slots[(destination_vnum, slot)] = _SOURCE_SPELL_MAP[
            source_spell
        ][1]
      emitted_records.append(emit_object(record, destination_vnum, _resolver).text)

    required_source_spells = (
        set(source_registry) | source_spell_constants | referenced_source_spells
    )
    self.assertEqual(327, len(source_registry))
    self.assertEqual(335, len(source_spell_constants))
    self.assertEqual(117, len(referenced_source_spells))
    self.assertEqual(340, len(required_source_spells))
    self.assertFalse(referenced_source_spells & set(_NON_CASTABLE_SOURCE_SPELLS))
    self.assertEqual(
        required_source_spells,
        set(_SOURCE_SPELL_MAP) | set(_NON_CASTABLE_SOURCE_SPELLS),
    )
    self.assertEqual({2: 71, 3: 80, 4: 109, 10: 251}, source_type_counts)
    self.assertEqual(511, len(emitted_records))
    self.assertEqual(734, len(expected_spell_slots))
    for source_spell, source_name in source_registry.items():
      if source_spell in _NON_CASTABLE_SOURCE_SPELLS:
        self.assertEqual(source_name, _NON_CASTABLE_SOURCE_SPELLS[source_spell])
      else:
        self.assertEqual(source_name, _SOURCE_SPELL_MAP[source_spell][0])

    path = self._target_path("obj", "".join(emitted_records))
    result = parse_object_file(path, "obj/26000.obj", self.manifest, set())
    actual_records = {record.vnum: record for record in result.records}

    self.assertTrue(result.complete)
    self.assertEqual(511, len(actual_records))
    for (destination_vnum, slot), expected_spell in expected_spell_slots.items():
      with self.subTest(destination_vnum=destination_vnum, slot=slot):
        actual_spell = actual_records[destination_vnum].values[slot]
        self.assertGreater(actual_spell, 0)
        self.assertEqual(expected_spell, actual_spell)

  def test_second_affect_word_converts_on_its_own_bit_range(self) -> None:
    # Source affect word 2 carries bits 33..64. Bit 34 is ULTRAVISION, which
    # maps to target affect 33; reading it as word 1 would land on bit 2.
    source = self._source_record(
        "obj",
        b"#203\nmace~\na mace~\nA mace lies here.~\n~\n"
        b"5 0 8193\n0 1 6 7\n10 100 1\n0\n2\n",
    )
    emitted = emit_object(source, 2_000_203, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20003.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertIn(33, decode_tokens(result.records[0].affect_flags).bits)

  def test_affect_words_on_the_economy_row_still_convert(self) -> None:
    source = self._source_record(
        "obj",
        b"#204\nmace~\na mace~\nA mace lies here.~\n~\n"
        b"5 0 8193\n0 1 6 7\n10 100 1 0 2\n",
    )
    emitted = emit_object(source, 2_000_204, _resolver)
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20004.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertIn(33, decode_tokens(result.records[0].affect_flags).bits)
    # The fourth number is an affect word, not an object level.
    self.assertEqual(1, result.records[0].level)

  def test_quiver_equips_land_on_the_ammo_pouch_slot(self) -> None:
    # Source position 23 is WEAR_QUIVER and target position 23 is
    # WEAR_AMMO_POUCH, the slot has_ammo_in_pouch() reads. The round trip is
    # the one the whole ranged chain depends on.
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 0\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"M 0 300 1 100\nE 1 200 1 23\nE 1 201 1 17\nE 1 202 1 18\nS\n",
    )
    emitted = emit_zone(source, 20_100, 2_000_100, _resolver)
    positions = [
        int(line.split()[4])
        for line in emitted.text.splitlines()
        if line.startswith("E ")
    ]
    self.assertEqual([23, 18, 17], positions)

  def test_equipment_positions_map_to_the_target_wear_constants(self) -> None:
    # The source zone E command carries a source WEAR_* constant, not an index
    # into the source equipment_types[] display table. Both headers are read so
    # the table fails when either side renumbers a slot.
    import re

    self._require_reference_paths("EXAMPLE/RealmsOfLuminari/src/structs.h")
    target = (self.root / "src/structs.h").read_text(encoding="utf-8", errors="ignore")
    target_wear = {
        name: int(value)
        for name, value in re.findall(r"#define (WEAR_\w+) (\d+)", target)
    }
    source = (self.root / "EXAMPLE/RealmsOfLuminari/src/structs.h").read_text(
        encoding="utf-8", errors="ignore"
    )
    source_wear = {
        name: int(value)
        for name, value in re.findall(
            r"#define (WEAR_\w+|PRIMARY_WEAPON|SECONDARY_WEAPON|HOLD|GUILD_INSIGNIA) +(\d+)",
            source,
        )
    }
    equivalents = {
        "WEAR_LIGHT": "WEAR_LIGHT",
        "WEAR_FINGER_R": "WEAR_FINGER_R",
        "WEAR_FINGER_L": "WEAR_FINGER_L",
        "WEAR_NECK_1": "WEAR_NECK_1",
        "WEAR_NECK_2": "WEAR_NECK_2",
        "WEAR_BODY": "WEAR_BODY",
        "WEAR_HEAD": "WEAR_HEAD",
        "WEAR_LEGS": "WEAR_LEGS",
        "WEAR_FEET": "WEAR_FEET",
        "WEAR_HANDS": "WEAR_HANDS",
        "WEAR_ARMS": "WEAR_ARMS",
        "WEAR_SHIELD": "WEAR_SHIELD",
        "WEAR_ABOUT": "WEAR_ABOUT",
        "WEAR_WAIST": "WEAR_WAIST",
        "WEAR_WRIST_R": "WEAR_WRIST_R",
        "WEAR_WRIST_L": "WEAR_WRIST_L",
        "PRIMARY_WEAPON": "WEAR_WIELD_1",
        "SECONDARY_WEAPON": "WEAR_WIELD_OFFHAND",
        "HOLD": "WEAR_HOLD_1",
        "WEAR_EYES": "WEAR_EYES",
        "WEAR_FACE": "WEAR_FACE",
        "WEAR_EARRING_R": "WEAR_EAR_R",
        "WEAR_EARRING_L": "WEAR_EAR_L",
        "WEAR_QUIVER": "WEAR_AMMO_POUCH",
        "GUILD_INSIGNIA": "WEAR_BADGE",
        "WEAR_TAIL": "WEAR_TAIL",
    }
    expected = {
        source_wear[source_name]: target_wear[target_name]
        for source_name, target_name in equivalents.items()
    }
    self.assertEqual(expected, EQUIPMENT_POSITION_MAP)

  def test_emitted_zone_normalizes_extended_resets(self) -> None:
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 64\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"D 0 100 1 8\nD 0 100 2 3\nR 0 100 200 35\nF 2 100 300 301\n"
        b"M 0 300 1 100\nE 1 200 1 25\nT 0 2 0 0\nX 1 -1 300 1 25\nS\n",
    )
    emitted = emit_zone(source, 20_100, 2_000_100, _resolver)
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "20100.zon"
    path.write_text(emitted.text, encoding="ascii", newline="\n")
    result = parse_zone_file(path, "zon/20100.zon", self.manifest, 6)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(
        ["K", "K", "R", "F", "M", "E", "C", "X"],
        [command.command for command in result.records[0].commands],
    )
    self.assertEqual(35, result.records[0].commands[2].probability)
    self.assertEqual(2, result.records[0].commands[3].dependency)
    self.assertEqual(43, result.records[0].commands[5].arguments[2])
    self.assertEqual(25, result.records[0].commands[7].probability)
    self.assertIn(18, decode_tokens(result.records[0].flags).bits)

  def test_emitted_zone_recovers_tail_objects_from_position_24(self) -> None:
    dedicated = self._source_record(
        "obj",
        b"#200\ntail plates~\ntail plates~\nTail plates are here.~\n~\n"
        b"9 0 4194305\n6 0 0 0\n1 1 0\n",
    )
    ring = self._source_record(
        "obj",
        b"#201\nring tail~\na tail ring~\nA tail ring is here.~\n~\n"
        b"9 0 4194307\n0 0 0 0\n1 1 0\n",
    )
    tail_only_objects, tail_ring_objects = classify_source_tail_objects(
        [dedicated, ring]
    )
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 0\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"M 0 300 1 100\nE 1 200 1 24\nE 1 201 1 24\nE 1 201 1 1\nS\n",
    )

    emitted = emit_zone(
        source,
        20_100,
        2_000_100,
        _resolver,
        tail_only_objects,
        tail_ring_objects,
    )
    positions = [
        int(line.split()[4])
        for line in emitted.text.splitlines()
        if line.startswith("E ")
    ]

    self.assertEqual({200}, tail_only_objects)
    self.assertEqual({201}, tail_ring_objects)
    self.assertEqual([43, 43, 1], positions)
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("normalized dedicated tail object 200", diagnostics)
    self.assertIn("position-24 compatibility defect", diagnostics)

  def test_emitted_zone_normalizes_source_boolean_dependencies(self) -> None:
    source = self._source_record(
        "zon",
        b"#100\nfile~\nPilot~\n199 30 2 0\n"
        b"0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n0 0 0 0\n"
        b"M 0 300 1 100\nG 2 200 1\nE -4 200 1 1\nS\n",
    )
    emitted = emit_zone(source, 20_100, 2_000_100, _resolver)
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "20100.zon"
    path.write_text(emitted.text, encoding="ascii", newline="\n")
    result = parse_zone_file(path, "zon/20100.zon", self.manifest, 6)

    self.assertTrue(result.complete)
    self.assertEqual([0, 1, 1], [command.dependency for command in result.records[0].commands])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("normalized source boolean dependency 2 to 1", diagnostics)
    self.assertIn("normalized source boolean dependency -4 to 1", diagnostics)

  def test_emitted_shop_maps_products_prices_hours_and_roaming(self) -> None:
    source = self._source_record(
        "shp",
        b"SHOP: 300\nHOURS: 8-12 13-17 18-20\nROOM: 100\nGREED: 140\n"
        b"PROFIT: 90\nCASTING:\nDEADBEAT: 1\nOFFENSE: 2\nCHEATS: GOODS PI PZ\n"
        b"HATES: NPC\nPO: 200\nBT: 5 11\nMBCASH: $n says 'No cash, $N.'\n"
        b"MBHAVE: $n says '$N does not have that.'\nMBIGOT: Go away.\n"
        b"MBUY: $n says 'Here is your %s.'\nMCLOSE: Closed.\n"
        b"MNBUY: $n says 'I do not buy $p.'\nMOPEN: Open.\n"
        b"MSCASH: $n says 'I cannot afford $p, $N.'\n"
        b"MSELL: $n says 'Sold to $N for %s.'\nMSHAVE: $n says 'I do not have $p.'\n",
    )
    emitted = emit_shop(source, 2_000_300, _resolver)
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / "20003.shp"
    path.write_text(
        "CircleMUD v3.0 Shop File~\n" + emitted.text + "$~\n",
        encoding="ascii",
        newline="\n",
    )
    result = parse_shop_file(path, "shp/20003.shp", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    shop = result.records[0]
    self.assertEqual([2_000_200], shop.product_vnums)
    self.assertEqual([5, 11], [entry.item_type for entry in shop.buy_types])
    self.assertEqual([2_000_100], shop.room_vnums)
    self.assertEqual([8, 20, 0, 0], shop.open_hours)
    self.assertEqual(1.4, shop.profit_buy)
    self.assertAlmostEqual(0.7368, shop.profit_sell or 0.0, places=4)
    self.assertEqual(192, shop.shop_flags)
    self.assertEqual(1 << 26, shop.customer_restrictions)
    self.assertEqual(1, shop.rol_cheat_restrictions)
    self.assertIn("%d coins", shop.messages[5])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("source-only shop CHEATS token 'PI'", diagnostics)
    self.assertIn("source-inert invalid shop CHEATS token 'PZ'", diagnostics)
    self.assertIn("source-only shop behavior", diagnostics)

  def test_emitted_roaming_shop_sets_native_compatibility_flag(self) -> None:
    source = self._source_record(
        "shp",
        b"SHOP: 300\nROAMING:\nHOURS: 1-23\nGREED: 100\nPROFIT: 100\n",
    )
    emitted = emit_shop(source, 2_000_300, _resolver)
    lines = emitted.text.splitlines()
    self.assertEqual("96", lines[-8])
    self.assertEqual("0", lines[-6])

  def test_emitted_hlquest_preserves_runtime_direction_order(self) -> None:
    source = self._source_record(
        "qst",
        b"#300\nM\nquestion~\nanswer~\nQ\ncomplete\n~\n"
        b"G\nI 201\nG\nI 201\nG\nC 5000000\n"
        b"R\nI 202\nR\nC 25\nD\nvanishes\n~\nS\n",
    )
    emitted = emit_hlquest(source, 2_000_300, _resolver)
    path = self._target_path("hlq", emitted.text)
    result = parse_hlquest_file(path, "hlq/20000.hlq", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    record = result.records[0]
    self.assertEqual(2_000_300, record.host_mobile_vnum)
    self.assertTrue(all(entry.approved for entry in record.entries))
    give = record.entries[1]
    self.assertEqual([2_000_201, 2_000_201, 5000000], [item.value for item in give.commands[:3]])
    self.assertEqual(["C", "I", "D"], [item.code for item in give.output_commands])
    self.assertIn("vanishes", give.reply_message)

  def test_emitted_hlquest_adapts_extended_reward_contracts(self) -> None:
    source = self._source_record(
        "qst",
        b"#300\nQ\ncomplete\n~\n"
        b"R\nA\nR\nE 1000\nR\nP -100\nR\nS 72\nS\n",
    )
    emitted = emit_hlquest(source, 2_000_300, _resolver)
    path = self._target_path("hlq", emitted.text)
    result = parse_hlquest_file(path, "hlq/20000.hlq", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    give = result.records[0].entries[0]
    self.assertEqual(["T", "P", "E", "A"], [item.code for item in give.output_commands])
    self.assertEqual([74, -100, 1000, 0], [item.value for item in give.output_commands])
    diagnostics = " ".join(emitted.diagnostics)
    self.assertIn("meteor swarm", diagnostics)

  def test_emitted_hlquest_maps_dragonscales_to_iron_skin(self) -> None:
    source = self._source_record(
        "qst",
        b"#300\nQ\ncomplete\n~\nR\nS 327\nS\n",
    )
    emitted = emit_hlquest(source, 2_000_300, _resolver)
    path = self._target_path("hlq", emitted.text)
    result = parse_hlquest_file(path, "hlq/20000.hlq", self.manifest)

    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    rewards = result.records[0].entries[0].output_commands
    self.assertEqual(["T"], [reward.code for reward in rewards])
    self.assertEqual([201], [reward.value for reward in rewards])
    self.assertIn("dragonscales", " ".join(emitted.diagnostics))

  def test_selected_pilot_quests_all_emit_valid_target_records(self) -> None:
    self._require_reference_paths(
        "lib/rol-conversion/runs/phase4-select-e6ea7982/pilot-actions.jsonl",
        "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl",
        "EXAMPLE/RealmsOfLuminari/areas/qst",
    )
    selection = self.root / "lib/rol-conversion/runs/phase4-select-e6ea7982"
    actions = [
        json.loads(line)
        for line in (selection / "pilot-actions.jsonl").read_text(encoding="ascii").splitlines()
    ]
    identities = [
        json.loads(line)
        for line in (
            self.root / "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl"
        ).read_text(encoding="ascii").splitlines()
    ]
    identity_map = {
        (row["source_kind"], row["source_vnum"]): row["destination_vnum"]
        for row in identities
    }
    selected = {
        (row["basename"], row["source_vnum"]): row["destination_vnum"]
        for row in actions
        if row["source_kind"] == "qst" and row["action"] == "ADD"
    }

    def resolver(kind: str, vnum: int) -> int:
      source_kind = {"wld": "wld", "mob": "mob", "obj": "obj"}[kind]
      return identity_map[(source_kind, vnum)]

    emitted_count = 0
    for basename in sorted({basename for basename, _ in selected}):
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/qst/{basename}.qst"
      records, corpus = parse_rol_source_file(
          source_path, f"areas/qst/{basename}.qst", "qst", basename
      )
      self.assertTrue(corpus.complete)
      text = ""
      for record in records:
        destination = selected.get((basename, record.vnum))
        if destination is None:
          continue
        converted = emit_hlquest(record, destination, resolver)
        self.assertFalse(any("excluded" in item for item in converted.diagnostics))
        text += converted.text
        emitted_count += 1
      temporary = tempfile.TemporaryDirectory()
      self.addCleanup(temporary.cleanup)
      target_path = Path(temporary.name) / f"{basename}.hlq"
      target_path.write_text(text + "$~\n", encoding="ascii", newline="\n")
      result = parse_hlquest_file(target_path, f"hlq/{basename}.hlq", self.manifest)
      self.assertTrue(result.complete)
      self.assertEqual([], result.findings)

    self.assertEqual(57, emitted_count)

  def test_selected_pilot_shops_all_emit_valid_target_records(self) -> None:
    self._require_reference_paths("EXAMPLE/RealmsOfLuminari/areas/shp")
    cases = (("hulburg", 100_000), ("muspel", 2_000_000))
    for basename, offset in cases:
      with self.subTest(basename=basename):
        source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/shp/{basename}.shp"
        records, corpus = parse_rol_source_file(
            source_path,
            f"areas/shp/{basename}.shp",
            "shp",
            basename,
        )
        self.assertTrue(corpus.complete)

        def resolver(kind: str, vnum: int) -> int:
          del kind
          return vnum + offset

        text = "CircleMUD v3.0 Shop File~\n"
        text += "".join(emit_shop(record, record.vnum + offset, resolver).text for record in records)
        text += "$~\n"
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        target_path = Path(temporary.name) / f"{basename}.shp"
        target_path.write_text(text, encoding="ascii", newline="\n")
        result = parse_shop_file(target_path, f"shp/{basename}.shp", self.manifest)
        self.assertTrue(result.complete)
        self.assertEqual([], result.findings)
        self.assertEqual(len(records), len(result.records))

  def test_soc_compiler_emits_exact_chance_flags_paths_and_filtered_echoes(self) -> None:
    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    source_path = Path(temporary.name) / "sample.soc"
    source_path.write_bytes(
        b"MOB: 300 PATH\nID: 1\nTYPE: 1\nDELAY: 2\n"
        b"ROOMS: 100 101\nDIRS: 1\nDONE\n"
        b"MOB: 300 TRIGGER\nTRIGGER: 23\nFLAG: 165\nCHANCE: 3\nDELAY: 0\n"
        b"ACTION: 1000\nAn indoor echo.\n~\nFLAG: 0\nCHANCE: 0\nDELAY: 4\n"
        b"ACTION: 1004\n1\n~\nDONE\n"
    )
    records, corpus = parse_rol_source_file(
        source_path, "areas/soc/sample.soc", "soc", "sample"
    )
    self.assertTrue(corpus.complete)
    compiled = compile_soc_records(
        records,
        2_030_000,
        _resolver,
        {23: "smile"},
    )

    self.assertEqual(2, compiled.source_records)
    self.assertEqual(2, compiled.source_actions)
    self.assertEqual(1, len(compiled.triggers))
    text = compiled.trigger_text
    self.assertIn("if %random.4% == 1", text)
    self.assertIn("if !%arg% || !(%self.name% /= %arg.car%)", text)
    self.assertIn("if %actor.is_pc%", text)
    self.assertIn("mrolzoneecho indoors %self.room.vnum% An indoor echo.", text)
    self.assertIn("wait 4", text)
    self.assertIn("wait 5 s\n        mrolwalkto 2000101", text)

  def test_all_pilot_soc_compiles_to_valid_target_triggers(self) -> None:
    self._require_reference_paths(
        "EXAMPLE/RealmsOfLuminari/areas/soc",
        "EXAMPLE/RealmsOfLuminari/src/interp.c",
    )
    records = []
    for basename in PILOT_BASENAMES:
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/soc/{basename}.soc"
      if not source_path.is_file():
        continue
      parsed, corpus = parse_rol_source_file(
          source_path, f"areas/soc/{basename}.soc", "soc", basename
      )
      self.assertTrue(corpus.complete)
      records.extend(parsed)

    command_evidence = extract_source_commands(
        self.root / "EXAMPLE/RealmsOfLuminari"
    )
    commands = {
        row["action_code"]: row["command"]
        for row in command_evidence["commands"]
    }
    compiled = compile_soc_records(records, 2_055_300, _resolver, commands)
    self.assertEqual(245, compiled.source_records)
    self.assertEqual(553, compiled.source_actions)
    self.assertEqual(181, len(compiled.triggers))
    self.assertEqual(163, len(compiled.attachments))
    self.assertFalse(
        any(
            word in diagnostic
            for diagnostic in compiled.diagnostics
            for word in ("lacks", "invalid", "missing", "unmapped")
        )
    )

    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    target_path = Path(temporary.name) / "20553.trg"
    target_path.write_text(compiled.trigger_text, encoding="ascii", newline="\n")
    result = parse_trigger_file(
        target_path, "trg/20553.trg", self.manifest
    )
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(181, len(result.records))

    comparison = build_soc_prototype_comparison(records, compiled)
    self.assertEqual("dg_compilation", comparison["selection"])
    self.assertEqual(
        245,
        comparison["native_compatibility_projection"]["persisted_behavior_records"],
    )
    self.assertEqual(
        26.122449,
        comparison["dg_compilation_pilot"]["record_reduction_percent"],
    )
    self.assertEqual(
        {1000: 7, 1001: 31, 1002: 2, 1003: 394, 1004: 14},
        comparison["measured_source"]["special_actions"],
    )

  def test_all_pilot_special_bindings_have_valid_native_or_dg_output(self) -> None:
    self._require_reference_paths(
        "lib/rol-conversion/runs/phase4-select-e6ea7982/pilot-special-bindings.jsonl",
        "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl",
        "EXAMPLE/RealmsOfLuminari/areas/wld",
    )
    selection = self.root / "lib/rol-conversion/runs/phase4-select-e6ea7982"
    bindings = [
        json.loads(line)
        for line in (selection / "pilot-special-bindings.jsonl")
        .read_text(encoding="ascii")
        .splitlines()
    ]
    identities = [
        json.loads(line)
        for line in (
            self.root / "lib/rol-conversion/runs/phase2-e6ea7982/identity-map.jsonl"
        )
        .read_text(encoding="ascii")
        .splitlines()
    ]
    identity_map = {
        (row["source_kind"], row["source_vnum"]): row["destination_vnum"]
        for row in identities
    }

    def resolver(kind: str, vnum: int) -> int:
      return identity_map[(kind, vnum)]

    rooms = []
    for basename in PILOT_BASENAMES:
      source_path = self.root / f"EXAMPLE/RealmsOfLuminari/areas/wld/{basename}.wld"
      parsed, corpus = parse_rol_source_file(
          source_path, f"areas/wld/{basename}.wld", "wld", basename
      )
      self.assertTrue(corpus.complete)
      rooms.extend(parsed)

    compiled = compile_special_bindings(bindings, 2_055_481, resolver, rooms)
    self.assertEqual(91, compiled.source_bindings)
    self.assertEqual(46, len(compiled.native_bindings))
    self.assertEqual(13, len(compiled.triggers))
    self.assertEqual(45, len(compiled.attachments))
    self.assertEqual(91, len(compiled.dispositions))
    self.assertEqual(
        46,
        sum(row["strategy"] == "NATIVE_PERSISTED" for row in compiled.dispositions),
    )
    self.assertIn("wrolroomflag", compiled.trigger_text)
    self.assertIn("wroldamage all-pcs 50 10", compiled.trigger_text)
    self.assertIn("mrolalert %actor%", compiled.trigger_text)
    self.assertIn("flags 2049", compiled.trigger_text)

    temporary = tempfile.TemporaryDirectory()
    self.addCleanup(temporary.cleanup)
    target_path = Path(temporary.name) / "20554.trg"
    target_path.write_text(compiled.trigger_text, encoding="ascii", newline="\n")
    result = parse_trigger_file(target_path, "trg/20554.trg", self.manifest)
    self.assertTrue(result.complete)
    self.assertEqual([], result.findings)
    self.assertEqual(13, len(result.records))

    native = next(
        item for item in compiled.native_bindings if item.persisted_name == "obj_drain"
    )
    self.assertEqual((44,), native.required_flag_bits)

  def test_shared_native_and_inert_special_dispositions_are_explicit(self) -> None:
    bindings = [
        {
            "basename": "sample",
            "record_type": "room",
            "source_vnum": 10,
            "source_handler": "guild",
        },
        {
            "basename": "sample",
            "record_type": "mobile",
            "source_vnum": 20,
            "source_handler": "janitor",
        },
        {
            "basename": "sample",
            "record_type": "room",
            "source_vnum": 30,
            "source_handler": "dump",
        },
        {
            "basename": "sample",
            "record_type": "mobile",
            "source_vnum": 40,
            "source_handler": "poison",
        },
        {
            "basename": "sample",
            "record_type": "mobile",
            "source_vnum": 50,
            "source_handler": "rogue_one",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(
        ["Janitor", "RoL Guild Room", "RoL Poison Bite"],
        sorted(binding.persisted_name for binding in compiled.native_bindings),
    )
    self.assertEqual([], compiled.triggers)
    self.assertEqual(5, len(compiled.dispositions))
    inert = next(row for row in compiled.dispositions if row["source_handler"] == "dump")
    self.assertEqual("SOURCE_INERT_EXCLUDED", inert["strategy"])
    self.assertIn("returns before", inert["reason"])
    adapted = next(row for row in compiled.dispositions if row["source_handler"] == "poison")
    self.assertEqual("NATIVE_ADAPTED", adapted["strategy"])
    rogue = next(row for row in compiled.dispositions if row["source_handler"] == "rogue_one")
    self.assertEqual("SOURCE_INERT_EXCLUDED", rogue["strategy"])
    self.assertIn("NPC_HIT", rogue["reason"])

  def test_artifact_handlers_reuse_modern_targets_and_unsafe_backdoor_is_excluded(self) -> None:
    artifact_targets = {
        1043: 2001043,
        1044: 2001044,
        1042: 2001042,
        1046: 2001046,
        1050: 2001050,
        1007: 2001007,
        1009: 2001009,
        1048: 2001048,
        5343: 2005343,
        1008: 2001008,
        19730: 2019730,
    }
    handlers = {
        1043: "OakenDefender",
        1044: "Amaukekel",
        1042: "Fade2",
        1046: "HornOfHenekar",
        1050: "Doombringer",
        1007: "Kelrarin",
        1009: "Kelrarin",
        1048: "Kelrom",
        5343: "Gesen",
        1008: "tiamat_stinger",
        19730: "New_Avernus",
        1045: "NeverLooseItem",
    }
    bindings = [
        {
            "basename": "artifacts",
            "record_type": "object",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers.items()
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: artifact_targets.get(vnum, 2_000_000 + vnum),
        [],
    )

    self.assertEqual([], compiled.native_bindings)
    self.assertEqual([], compiled.triggers)
    reconciled = [
        row for row in compiled.dispositions if row["strategy"] == "NATIVE_RECONCILED"
    ]
    self.assertEqual(11, len(reconciled))
    self.assertEqual(
        sorted(artifact_targets.values()),
        sorted(row["target_vnum"] for row in reconciled),
    )
    unsafe = next(
        row for row in compiled.dispositions if row["source_handler"] == "NeverLooseItem"
    )
    self.assertEqual("SOURCE_UNSAFE_EXCLUDED", unsafe["strategy"])
    self.assertIn("currency", unsafe["reason"])

  def test_banana_is_adapted_and_destructive_god_toys_are_excluded(self) -> None:
    unsafe_handlers = {
        "mystra",
        "lloth_avatar",
        "lloth",
        "varon",
        "zusukthing",
        "mask",
        "kor_avatar",
        "velshorn",
        "caytra",
        "cinandriel",
        "altherogs_blackSunSword",
        "kelly_mirror",
        "burunga",
        "diinkarazan",
        "erevan",
        "shar",
        "azuth",
    }
    bindings = [
        {
            "basename": "misc_code2",
            "record_type": "object",
            "source_vnum": 1234,
            "source_handler": "banana",
        },
        {
            "basename": "misc_code2",
            "record_type": "object",
            "source_vnum": 1235,
            "source_handler": "banana",
        },
    ]
    bindings.extend(
        {
            "basename": "god_toys",
            "record_type": "object",
            "source_vnum": index,
            "source_handler": handler,
        }
        for index, handler in enumerate(sorted(unsafe_handlers), start=1)
    )

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(
        ["RoL Banana", "RoL Banana"],
        sorted(binding.persisted_name for binding in compiled.native_bindings),
    )
    unsafe = [
        row for row in compiled.dispositions if row["strategy"] == "SOURCE_UNSAFE_EXCLUDED"
    ]
    self.assertEqual(17, len(unsafe))
    self.assertEqual(unsafe_handlers, {row["source_handler"] for row in unsafe})

  def test_undead_drain_family_uses_one_profiled_mobile_adapter(self) -> None:
    handlers = (
        "undead_ghoul",
        "undead_shadow",
        "undead_wight",
        "undead_ghast",
        "undead_wraith",
        "undead_spectre",
        "undead_ghost",
    )
    bindings = [
        {
            "basename": "mobile",
            "record_type": "mobile",
            "source_vnum": 1256 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(7, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Undead Drain"
            and binding.required_flag_bits == (0,)
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_conjured_death_binding_uses_composable_mobile_flag(self) -> None:
    binding = {
        "basename": "misc_code",
        "record_type": "mobile",
        "source_vnum": 300,
        "source_handler": "conj_familiar_die",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(1, len(compiled.native_bindings))
    native = compiled.native_bindings[0]
    self.assertIsNone(native.persisted_name)
    self.assertEqual((119,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#300\nfamiliar~\na familiar~\n"
        b"A familiar waits.\n~\nA familiar.\n~\n1 0 0 0 S\n"
        b"H 0 0\n10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_000_300,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20001.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertIn(119, decode_tokens(result.records[0].action_flags).bits)
    self.assertNotIn(0, decode_tokens(result.records[0].action_flags).bits)
    self.assertNotIn("source ACT_SPEC deferred", " ".join(emitted.diagnostics))

  def test_death_profile_binding_resolves_without_consuming_mobile_slot(self) -> None:
    binding = {
        "basename": "misc_code",
        "record_type": "mobile",
        "source_vnum": 907,
        "source_handler": "fire_mephit_die",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(1, len(compiled.native_bindings))
    native = compiled.native_bindings[0]
    self.assertIsNone(native.persisted_name)
    self.assertEqual((), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[0]["strategy"])
    self.assertEqual("converted mobile death profile", compiled.dispositions[0]["target"])

    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#907\nmephit~\na fire mephit~\n"
        b"A fire mephit waits.\n~\nA fire mephit.\n~\n1 0 0 0 S\n"
        b"N 0 0\n10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(source, 2_000_907, special_resolved=True)
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20000.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertNotIn(0, decode_tokens(result.records[0].action_flags).bits)
    self.assertNotIn("source ACT_SPEC deferred", " ".join(emitted.diagnostics))

  def test_source_death_effect_batch_resolves_as_composable_profiles(self) -> None:
    bindings = [
        {
            "basename": "source_death_effects",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in (
            (20221, "weevelDeath"),
            (20267, "weevelDeath"),
            (21783, "dk_aleanrahel"),
            (92062, "um_helmedHorror"),
            (93017, "um2_butcherKnife"),
            (93018, "um2_gargoyleDie"),
            (93020, "um2_crystalGolemDie"),
            (93301, "um2_whitePuddingSplit"),
        )
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(8, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name is None and binding.required_flag_bits == ()
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(
            row["strategy"] == "NATIVE_ADAPTED_COMPOSABLE"
            and row["target"] == "converted mobile death profile"
            for row in compiled.dispositions
        )
    )

  def test_waterdeep_ambient_handlers_share_one_persistent_adapter(self) -> None:
    handlers = (
        "wanderer",
        "drunk_one",
        "casino_two",
        "youth_two",
        "tailor_one",
        "waterdeep_guard_one",
    )
    bindings = [
        {
            "basename": "waterdeep",
            "record_type": "mobile",
            "source_vnum": 3000 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Waterdeep Ambient"
            and binding.required_flag_bits == ()
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_regular_periodic_handlers_share_generated_persistent_adapter(self) -> None:
    handlers = tuple(PROFILE_SOURCES)[:6]
    bindings = [
        {
            "basename": "regular-periodic",
            "record_type": "mobile",
            "source_vnum": 7100 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Source Periodic"
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_scornubel_periodic_and_fiery_mace_bindings_preserve_composition(self) -> None:
    mobile_rows = (
        (6001, "sc_guardsman"),
        (6002, "sc_merchant"),
        (6006, "sc_ladyRhessajan"),
        (6029, "sc_clerk"),
        (6051, "sc_commoner"),
        (6058, "sc_merchant"),
        (6061, "sc_parchimil"),
        (6064, "sc_loudPeddler"),
        (6067, "sc_mercenary"),
        (6072, "sc_angryMan"),
        (6106, "sc_butler"),
        (6109, "sc_commoner"),
        (6111, "sc_chansrin"),
        (6113, "sc_merchant"),
        (6132, "sc_merchant"),
        (6140, "sc_karlyn"),
        (6141, "sc_maid"),
    )
    bindings = [
        {
            "basename": "scorn",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in mobile_rows
    ]
    bindings.append(
        {
            "basename": "scorn",
            "record_type": "object",
            "source_vnum": 6084,
            "source_handler": "sc_fieryMace",
        }
    )

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(18, len(compiled.native_bindings))
    for binding in compiled.native_bindings:
      if binding.source_record_type == "object":
        self.assertEqual("RoL Weapon Proc", binding.persisted_name)
        self.assertEqual((44,), binding.required_flag_bits)
      elif binding.source_vnum == 6061:
        self.assertEqual(COMPOSED_PROFILE_TARGETS["sc_parchimil"], binding.persisted_name)
        self.assertEqual((0,), binding.required_flag_bits)
      else:
        self.assertEqual("RoL Source Periodic", binding.persisted_name)
        self.assertEqual((0,), binding.required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

    composed = compile_special_bindings(
        [
            {
                "basename": "scorn",
                "record_type": "mobile",
                "source_vnum": 6061,
                "source_handler": handler,
            }
            for handler in ("guild_guard", "sc_parchimil")
        ],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )
    self.assertEqual(
        ["RoL Guild Guard", "RoL Guild Guard"],
        sorted(binding.persisted_name for binding in composed.native_bindings),
    )

  def test_zhentil_periodic_bindings_share_generated_persistent_adapter(self) -> None:
    mobile_rows = (
        (81021, "zk_minstrel"),
        (81054, "zk_little_girl"),
        (81059, "zk_terrified_merchant"),
        (81066, "zk_visiting_dignitary"),
        (81067, "zk_scornubian_trader"),
        (81068, "zk_ugly_prostitute"),
    )
    bindings = [
        {
            "basename": "zhentilkeep",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in mobile_rows
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Source Periodic"
            and binding.required_flag_bits == (0,)
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_state_periodic_handlers_share_generated_persistent_adapter(self) -> None:
    handlers = tuple(STATE_PROFILE_SOURCES)[:6]
    bindings = [
        {
            "basename": "state-periodic",
            "record_type": "mobile",
            "source_vnum": 5500 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Stateful Periodic"
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_waterdeep_peacekeepers_share_one_persistent_adapter(self) -> None:
    handlers = (
        "bouncer_one",
        "bouncer_two",
        "bouncer_three",
        "bouncer_four",
        "casino_three",
        "guard_one",
    )
    bindings = [
        {
            "basename": "waterdeep-peacekeepers",
            "record_type": "mobile",
            "source_vnum": 5520 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Waterdeep Peacekeeper"
            and binding.required_flag_bits == (0,)
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
      all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_weapon_handlers_share_one_payload_aware_adapter(self) -> None:
    handlers = (
        "hammer",
        "proc_icydagger",
        "sf_glimmering_burst",
        "githyanki2",
        "githyanki",
        "valhalla_scepter",
        "longsword_slenderelven",
        "nightbringer",
        "kirinHorn",
        "windsong",
        "shadow_dagger",
        "swordOfFireGiants",
        "longsword_acid",
        "sword_wickedly_barbed",
        "longsword_rippling_flames",
        "jeweled_fang",
        "longsword_black_flames",
        "moonblade_starsong",
        "glowing_crimson_dagger",
        "mielikki_scimitar",
        "flamberge",
        "orb",
        "doombringer",
        "tahlshara",
        "rockcrusher",
        "cymric_hugh",
        "torment",
        "pahlurukroot",
        "proc_dirk_reversehit",
        "frulghiem",
        "sphere_lightning_weapon",
        "halruaa_enchanterstaff",
        "halruaa_illusionstaff",
        "halruaa_invokerstaff",
        "halruaa_magebane",
        "halruaa_dwarven_hammer",
        "halruaa_elemstaff",
        "halruaa_necrostaff",
        "hive_gythka",
        "holy_weapon",
        "kor_only_sword",
        "md_darken_aura",
        "md_gleaming_burst",
        "hellish_fury_bow",
    )
    bindings = [
        {
            "basename": "weapon-procs",
            "record_type": "object",
            "source_vnum": 4500 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(len(handlers), len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Weapon Proc"
            and binding.required_flag_bits == (44,)
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_composed_alert_keeps_existing_breath_binding(self) -> None:
    bindings = [
        {
            "basename": "plane_fire",
            "record_type": "mobile",
            "source_vnum": 25406,
            "source_handler": "breath_weapon_fire",
        },
        {
            "basename": "plane_fire",
            "record_type": "mobile",
            "source_vnum": 25406,
            "source_handler": "imix_shout",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(2, len(compiled.native_bindings))
    self.assertEqual(
        [None, "breath_weapon_fire"],
        [binding.persisted_name for binding in compiled.native_bindings],
    )
    shout = next(
        row for row in compiled.dispositions if row["source_handler"] == "imix_shout"
    )
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", shout["strategy"])

  def test_bloodstone_undead_death_binding_uses_composable_mobile_flag(self) -> None:
    binding = {
        "basename": "bs1",
        "record_type": "mobile",
        "source_vnum": 7119,
        "source_handler": "bs_undead_die",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(1, len(compiled.native_bindings))
    native = compiled.native_bindings[0]
    self.assertIsNone(native.persisted_name)
    self.assertEqual((124,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"<*> File Version 1 <*>\n#7119\nundead~\nan undead~\n"
        b"An undead waits.\n~\nAn undead.\n~\n1 0 0 0 S\n"
        b"US 0 0\n10 0 50 2d8+5 1d4+1\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_007_119,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20007.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertIn(124, decode_tokens(result.records[0].action_flags).bits)
    self.assertNotIn(0, decode_tokens(result.records[0].action_flags).bits)
    self.assertNotIn("source ACT_SPEC deferred", " ".join(emitted.diagnostics))

  def test_home_reset_binding_uses_composable_room_flag(self) -> None:
    binding = {
        "basename": "gen-obj",
        "record_type": "room",
        "source_vnum": 7909,
        "source_handler": "home_reset",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(1, len(compiled.native_bindings))
    native = compiled.native_bindings[0]
    self.assertIsNone(native.persisted_name)
    self.assertEqual((46,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "wld",
        b"#7909\nA patrol route~\nA patrol route continues here.~\n1 0 0\nS\n",
    )
    emitted = emit_room(
        source,
        2_007_909,
        20_079,
        _resolver,
        required_flag_bits=native.required_flag_bits,
    )
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/20079.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertIn(46, decode_tokens(result.records[0].flags).bits)
    self.assertIsNone(result.records[0].spec_proc)

  def test_magic_pool_binding_remaps_destination_value(self) -> None:
    binding = {
        "basename": "astral_main",
        "record_type": "object",
        "source_vnum": 19710,
        "source_handler": "magic_pool",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Magic Pool", native.persisted_name)
    self.assertEqual(((0, "wld"),), native.value_reference_slots)

    source = self._source_record(
        "obj",
        b"#19710\nruby pool~\na ruby pool~\nA ruby pool is here.~\n~\n"
        b"12 0 0\n19946 200 0 0\n1 1 1\n",
    )
    emitted = emit_object(
        source,
        2_019_710,
        _resolver,
        special_proc=native.persisted_name,
        required_value_references=native.value_reference_slots,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20197.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Magic Pool", result.records[0].spec_proc)
    self.assertEqual(2_019_946, result.records[0].values[0])
    self.assertEqual(200, result.records[0].values[1])

  def test_bloodstone_portal_binding_remaps_destination_value(self) -> None:
    binding = {
        "basename": "bs1",
        "record_type": "object",
        "source_vnum": 7147,
        "source_handler": "bs_portal",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Bloodstone Portal", native.persisted_name)
    self.assertEqual(((0, "wld"),), native.value_reference_slots)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "obj",
        b"#7147\nportal~\na shimmering portal~\nA portal is here.~\n~\n"
        b"12 0 0\n7250 0 0 0\n0 0 0\n",
    )
    emitted = emit_object(
        source,
        2_007_147,
        _resolver,
        special_proc=native.persisted_name,
        required_value_references=native.value_reference_slots,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20071.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Bloodstone Portal", result.records[0].spec_proc)
    self.assertEqual(2_007_250, result.records[0].values[0])

  def test_portal_door_binding_remaps_destination_value(self) -> None:
    binding = {
        "basename": "misc_code",
        "record_type": "object",
        "source_vnum": 751,
        "source_handler": "portal_door",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Portal Door", native.persisted_name)
    self.assertEqual(((0, "wld"),), native.value_reference_slots)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "obj",
        b"#751\nportal~\na rainbow portal~\nA portal is here.~\n~\n"
        b"13 0 0\n3001 0 0 0\n0 0 0\n",
    )
    emitted = emit_object(
        source,
        2_000_751,
        _resolver,
        special_proc=native.persisted_name,
        required_value_references=native.value_reference_slots,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20007.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Portal Door", result.records[0].spec_proc)
    self.assertEqual(2_003_001, result.records[0].values[0])

  def test_auto_distributor_binding_persists_room_procedure(self) -> None:
    binding = {
        "basename": "wilderness-08",
        "record_type": "room",
        "source_vnum": 820196,
        "source_handler": "autoDistributor",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Auto Distributor", native.persisted_name)
    self.assertEqual((), native.required_flag_bits)
    self.assertEqual("NATIVE_PERSISTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "wld",
        b"#820196\nA planar boundary~\nThe boundary flickers here.~\n1 0 0\nS\n",
    )
    emitted = emit_room(
        source,
        2_820_196,
        28_201,
        _resolver,
        special_proc=native.persisted_name,
    )
    path = self._target_path("wld", emitted.text)
    result = parse_room_file(path, "wld/28201.wld", self.manifest, False, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Auto Distributor", result.records[0].spec_proc)

  def test_shadow_giant_binding_persists_adapted_mobile_procedure(self) -> None:
    binding = {
        "basename": "abandon",
        "record_type": "mobile",
        "source_vnum": 90855,
        "source_handler": "shadow_giant",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Shadow Giant", native.persisted_name)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#90855\nshadow giant~\na shadow giant~\nA shadow giant waits.~\n~\n"
        b"1 0 0 0 S\nN 0 0\n30 0 0 25d8+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_090_855,
        special_proc=native.persisted_name,
        special_resolved=True,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20908.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Shadow Giant", result.records[0].spec_proc)

  def test_command_sentinel_family_shares_owner_aware_adapter(self) -> None:
    handlers = [
        ("mobile", 1438, "stone_golem"),
        ("mobile", 10301, "gate_guard"),
        ("mobile", 10302, "shady_man"),
        ("mobile", 81508, "ancient_man"),
        ("room", 1, "cage_command_block"),
        ("room", 46990, "necro_passing_glyph"),
    ]
    bindings = [
        {
            "basename": "sentinel",
            "record_type": record_type,
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for record_type, source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual([], compiled.triggers)
    self.assertEqual(6, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Command Sentinel"
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_foggy_woods_warning_rooms_share_one_entry_trigger(self) -> None:
    bindings = [
        {
            "basename": "foggy_woods",
            "record_type": "room",
            "source_vnum": source_vnum,
            "source_handler": "fw_warning_room",
        }
        for source_vnum in (90107, 90112, 90114)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual([], compiled.native_bindings)
    self.assertEqual(1, len(compiled.triggers))
    trigger = compiled.triggers[0]
    self.assertEqual("wld", trigger.owner_kind)
    self.assertEqual((2_090_107, 2_090_112, 2_090_114), trigger.owner_vnums)
    self.assertEqual(("fw_warning_room",), trigger.source_handlers)
    self.assertIn("RoL Foggy Woods entry warning", trigger.text)
    self.assertIn("wsend %actor%", trigger.text)
    self.assertEqual(
        {
            ("wld", 2_090_107): [2_100_000],
            ("wld", 2_090_112): [2_100_000],
            ("wld", 2_090_114): [2_100_000],
        },
        compiled.attachments,
    )
    self.assertTrue(
        all(row["strategy"] == "DG_ENTRY_WARNING" for row in compiled.dispositions)
    )

  def test_ship_family_bindings_persist_adapted_procedures(self) -> None:
    bindings = [
        {
            "basename": "ships",
            "record_type": "object",
            "source_vnum": 5731,
            "source_handler": "ship",
        },
        {
            "basename": "ships",
            "record_type": "object",
            "source_vnum": 5732,
            "source_handler": "control_panel",
        },
        {
            "basename": "ships",
            "record_type": "room",
            "source_vnum": 5999,
            "source_handler": "ship_exit_room",
        },
        {
            "basename": "ships",
            "record_type": "room",
            "source_vnum": 5998,
            "source_handler": "ship_look_out_room",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(
        ["RoL Ship", "RoL Ship Control", "RoL Ship Lookout", "RoL Ship Exit"],
        [binding.persisted_name for binding in compiled.native_bindings],
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_ship_navigator_binding_requires_mobile_spec_flag(self) -> None:
    binding = {
        "basename": "ships",
        "record_type": "mobile",
        "source_vnum": 5739,
        "source_handler": "navagator",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Ship Navigator", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    source = self._source_record(
        "mob",
        b"#5739\nnavigator~\na navigator~\nA navigator stands here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n10 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_005_739,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20057.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Ship Navigator", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_guild_guard_binding_requires_mobile_spec_flag(self) -> None:
    handlers = [
        "guild_guard",
        "bs_guildguard_antiwar",
        "bs_guildguard_assassin",
        "bs_guildguard_clersham",
        "bs_guildguard_necro",
        "bs_guildguard_sorcconj",
        "bs_guildguard_thief",
        "guild_guard_one",
        "guild_guard_four",
        "guild_guard_five",
        "guild_guard_six",
        "guild_guard_eight",
        "guild_guard_nine",
        *COMPOSED_STATE_PROFILE_SOURCES,
    ]
    bindings = [
        {
            "basename": "gloom",
            "record_type": "mobile",
            "source_vnum": 34261 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual(20, len(compiled.native_bindings))
    self.assertTrue(
        all(binding.persisted_name == "RoL Guild Guard" for binding in compiled.native_bindings)
    )
    self.assertTrue(all(binding.required_flag_bits == (0,) for binding in compiled.native_bindings))
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

    source = self._source_record(
        "mob",
        b"#34261\nguild guardian~\na guild guardian~\nA guardian stands here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n30 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_034_261,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20342.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Guild Guard", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_utility_object_batch_preserves_pulse_flags_and_figurine_reference(self) -> None:
    handlers = [
        (876, "goodberry_cure", (), ()),
        (7151, "bs_child_sacrifice", (), ()),
        (46991, "thp_necroChild", (44,), ()),
        (88825, "menden_figurine", (), ((0, "mob"),)),
        (90004, "fw_ruby_monocle", (44,), ()),
        (47, "magius_staff", (), ()),
        (10672, "gn_dragoncultrobes", (), ()),
        (26260, "moonshae_earthmother_staff", (), ()),
        (43723, "basilisk_leggings", (), ()),
        (44019, "basilisk_snakes", (), ()),
        (51110, "nh_blueplume", (), ()),
        (51207, "nh_writhingash", (), ()),
        (57236, "haste_sleeves", (), ()),
        (19932, "lathander_disc", (), ()),
        (19988, "tiamat_crescent_moon", (), ()),
        (57003, "smoke_stun_shield", (), ()),
        (88830, "llyms_altar", (), ()),
        (897, "item_loot_block", (44,), ()),
        (3088, "blackPlagueReservoir", (), ()),
    ]
    bindings = [
        {
            "basename": "utility-objects",
            "record_type": "object",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler, _, _ in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(len(handlers), len(compiled.native_bindings))
    by_vnum = {binding.source_vnum: binding for binding in compiled.native_bindings}
    for source_vnum, _, expected_flags, expected_slots in handlers:
      binding = by_vnum[source_vnum]
      self.assertEqual("RoL Utility Object", binding.persisted_name)
      self.assertEqual(expected_flags, binding.required_flag_bits)
      self.assertEqual(expected_slots, binding.value_reference_slots)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_utility_room_batch_persists_shared_typed_adapter(self) -> None:
    handlers = [(101, "newbieLoadRoom"), (51400, "weight_trigger")]
    bindings = [
        {
            "basename": "utility-rooms",
            "record_type": "room",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(2, len(compiled.native_bindings))
    for binding in compiled.native_bindings:
      self.assertEqual("RoL Utility Room", binding.persisted_name)
      self.assertEqual((), binding.required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_scheduled_mobile_batch_persists_shared_adapter(self) -> None:
    handlers = (
        (3008, "crier_one"),
        (3082, "waterdeep_guard_three"),
        (5311, "naval_three"),
        (5313, "lighthouse_one"),
        (34274, "gloomhaven_gate_guard"),
    )
    bindings = [
        {
            "basename": "scheduled-mobiles",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(5, len(compiled.native_bindings))
    for binding in compiled.native_bindings:
      self.assertEqual("RoL Scheduled Mobile", binding.persisted_name)
      self.assertEqual((0,), binding.required_flag_bits)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_inert_object_callbacks_emit_no_target_binding(self) -> None:
    bindings = [
        {
            "basename": "inert-objects",
            "record_type": "object",
            "source_vnum": 91248,
            "source_handler": "blackPlagueCure",
        },
        {
            "basename": "inert-objects",
            "record_type": "object",
            "source_vnum": 19985,
            "source_handler": "craine_serpent",
        },
        {
            "basename": "inert-objects",
            "record_type": "object",
            "source_vnum": 1019,
            "source_handler": "nuclear_bomb",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual([], compiled.native_bindings)
    self.assertEqual([], compiled.triggers)
    self.assertTrue(
        all(row["strategy"] == "SOURCE_INERT_EXCLUDED" for row in compiled.dispositions)
    )

  def test_toll_keeper_handlers_share_mobile_adapter(self) -> None:
    handlers = [
        (1919, "bridge_troll"),
        (7210, "bs_tax"),
        (7335, "bs_bouncer"),
        (7335, "bs_bouncer"),
        (11106, "ticket_taker"),
        (11306, "ticket_taker"),
        (11542, "ghore_paradise"),
        (14202, "bridge_troll"),
        (98357, "ticket_taker"),
        (98358, "ticket_taker"),
    ]
    bindings = [
        {
            "basename": "tolls",
            "record_type": "mobile",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(10, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Toll Keeper"
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(binding.required_flag_bits == (0,) for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_travel_portal_handlers_share_object_adapter_and_remap_destinations(self) -> None:
    handlers = [
        (882, "dim_fold", ((0, "wld"),)),
        (3088, "waterdeep_fountain_teleport", ()),
        (5515, "waterdeep_portal", ((0, "wld"),)),
        (5516, "waterdeep_portal", ((0, "wld"),)),
        (8112, "elfgate", ((0, "wld"), (1, "wld"), (2, "wld"), (3, "wld"))),
        (8113, "elfgate", ((0, "wld"), (1, "wld"), (2, "wld"), (3, "wld"))),
        (21500, "shaman_quest_teleport", ((0, "wld"),)),
        (21501, "shaman_quest_teleport", ((0, "wld"),)),
        (41941, "blip_portal", ((0, "wld"),)),
    ]
    bindings = [
        {
            "basename": "travel-portals",
            "record_type": "object",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler, _ in handlers
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(9, len(compiled.native_bindings))
    by_vnum = {binding.source_vnum: binding for binding in compiled.native_bindings}
    for source_vnum, _, expected_slots in handlers:
      self.assertEqual("RoL Travel Portal", by_vnum[source_vnum].persisted_name)
      self.assertEqual(expected_slots, by_vnum[source_vnum].value_reference_slots)
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_class_type_guild_bindings_use_distinct_room_adapters(self) -> None:
    expected = {
        "guild_classtype_mage": "RoL Mage Guild Room",
        "guild_classtype_thief": "RoL Thief Guild Room",
        "guild_classtype_warrior": "RoL Warrior Guild Room",
        "guild_classtype_cleric": "RoL Cleric Guild Room",
        "guild_bard": "RoL Bard Guild Room",
        "guild_battlechanter": "RoL Bard Guild Room",
    }
    bindings = [
        {
            "basename": "guilds",
            "record_type": "room",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in enumerate(expected, start=100)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual([], compiled.triggers)
    self.assertEqual(6, len(compiled.native_bindings))
    self.assertEqual(
        sorted(expected.values()),
        sorted(binding.persisted_name for binding in compiled.native_bindings),
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_waterdeep_guild_wrappers_share_room_adapter(self) -> None:
    handlers = [
        f"waterdeep_guild_{name}"
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
    bindings = [
        {
            "basename": "waterdeep",
            "record_type": "room",
            "source_vnum": source_vnum,
            "source_handler": handler,
        }
        for source_vnum, handler in enumerate(handlers, start=2956)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(12, len(compiled.native_bindings))
    self.assertTrue(
        all(
            binding.persisted_name == "RoL Waterdeep Guild Room"
            for binding in compiled.native_bindings
        )
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_lich_rite_bindings_require_mobile_spec_and_share_adapter(self) -> None:
    bindings = [
        {
            "basename": "misc_code",
            "record_type": "mobile",
            "source_vnum": 9,
            "source_handler": "lichConverter",
        },
        {
            "basename": "necro",
            "record_type": "mobile",
            "source_vnum": 46990,
            "source_handler": "lichConverter",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(2, len(compiled.native_bindings))
    self.assertTrue(
        all(binding.persisted_name == "RoL Lich Rite" for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(binding.required_flag_bits == (0,) for binding in compiled.native_bindings)
    )
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_shaman_totem_and_spirit_death_bindings_share_converted_identity(self) -> None:
    bindings = [
        {
            "basename": "misc_code",
            "record_type": "object",
            "source_vnum": 716,
            "source_handler": "shaman_totem",
        },
        {
            "basename": "misc_code",
            "record_type": "mobile",
            "source_vnum": 716,
            "source_handler": "spirit_wolf_die",
        },
        {
            "basename": "outpost",
            "record_type": "mobile",
            "source_vnum": 20971,
            "source_handler": "lostTotemRestorer",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )
    object_binding = next(
        binding for binding in compiled.native_bindings
        if binding.source_record_type == "object"
    )
    spirit_binding = next(
        binding for binding in compiled.native_bindings
        if binding.source_vnum == 716 and binding.source_record_type == "mobile"
    )
    restorer_binding = next(
        binding for binding in compiled.native_bindings
        if binding.source_vnum == 20971
    )
    self.assertEqual("RoL Shaman Totem", object_binding.persisted_name)
    self.assertEqual((), object_binding.required_flag_bits)
    self.assertIsNone(spirit_binding.persisted_name)
    self.assertEqual((123,), spirit_binding.required_flag_bits)
    self.assertEqual("RoL Totem Restorer", restorer_binding.persisted_name)
    self.assertEqual((0,), restorer_binding.required_flag_bits)

    source_object = self._source_record(
        "obj",
        b"#716\nwooden wolf totem~\na wooden wolf totem~\n"
        b"A wooden wolf totem lies here.~\n~\n5 0 0\n0 0 0 0\n1 1 1\n",
    )
    emitted_object = emit_object(
        source_object,
        2_000_716,
        _resolver,
        special_proc=object_binding.persisted_name,
    )
    object_path = self._target_path("obj", emitted_object.text)
    object_result = parse_object_file(object_path, "obj/20000.obj", self.manifest, set())
    self.assertTrue(object_result.complete)
    self.assertEqual("RoL Shaman Totem", object_result.records[0].spec_proc)

    source_mobile = self._source_record(
        "mob",
        b"#716\nspirit wolf~\na spirit wolf~\nA spirit wolf waits here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n10 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted_mobile = emit_mobile(
        source_mobile,
        2_000_716,
        special_resolved=True,
        required_action_bits=spirit_binding.required_flag_bits,
    )
    mobile_path = self._target_path("mob", emitted_mobile.text)
    mobile_result = parse_mobile_file(mobile_path, "mob/20000.mob", self.manifest, set())
    self.assertTrue(mobile_result.complete)
    self.assertIn(123, decode_tokens(mobile_result.records[0].action_flags).bits)
    self.assertNotIn(0, decode_tokens(mobile_result.records[0].action_flags).bits)

  def test_major_beholder_binding_requires_mobile_combat_gateway(self) -> None:
    binding = {
        "basename": "caves",
        "record_type": "mobile",
        "source_vnum": 80013,
        "source_handler": "major_beholder",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Major Beholder", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#80013\nmajor beholder~\na major beholder~\n"
        b"A major beholder floats here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n40 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_080_013,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20800.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Major Beholder", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_monster_combat_bindings_require_combat_gateway(self) -> None:
    handlers = (
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
        "standard_faerie_ff",
        "standard_faerie_prism",
        "faerie_search",
        "manscorpion_venom_light",
        "manscorpion_venom_medium",
        "manscorpion_venom_heavy",
        "manscorpion_king",
        "dk_bansheeWail",
        "dk_bladestorm",
        "ms_sandstorm_beast",
        "skriaxit_sandstorm",
        "gc_araleshTandar",
        "gc_bansheWail",
        "gc_urguthaForka",
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
    bindings = [
        {
            "basename": "combat",
            "record_type": "mobile",
            "source_vnum": 10_000 + index,
            "source_handler": handler,
        }
        for index, handler in enumerate(handlers)
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(len(handlers), len(compiled.native_bindings))
    self.assertEqual(len(handlers), len(compiled.dispositions))
    for native, disposition in zip(
        compiled.native_bindings, compiled.dispositions, strict=True
    ):
      self.assertEqual("RoL Monster Combat", native.persisted_name)
      self.assertEqual((0,), native.required_flag_bits)
      self.assertEqual("NATIVE_ADAPTED", disposition["strategy"])

  def test_elemental_tower_composes_alert_with_monster_combat(self) -> None:
    bindings = [
        {
            "basename": "elemental_tower",
            "record_type": "mobile",
            "source_vnum": 62401,
            "source_handler": "elemental_tower_shout",
        },
        {
            "basename": "elemental_tower",
            "record_type": "mobile",
            "source_vnum": 62401,
            "source_handler": "et_fireBoss",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(2, len(compiled.native_bindings))
    self.assertIsNone(compiled.native_bindings[0].persisted_name)
    self.assertEqual(
        "RoL Monster Combat", compiled.native_bindings[1].persisted_name
    )
    self.assertEqual(
        "NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[0]["strategy"]
    )
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[1]["strategy"])

  def test_seelie_faerie_bindings_share_one_mobile_procedure(self) -> None:
    bindings = [
        {
            "basename": "seeliecourt",
            "record_type": "mobile",
            "source_vnum": 62701,
            "source_handler": handler,
        }
        for handler in ("standard_faerie_ff", "standard_faerie_prism", "faerie_search")
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(3, len(compiled.native_bindings))
    self.assertEqual({2_062_701}, {row.target_vnum for row in compiled.native_bindings})
    self.assertTrue(
        all(row.persisted_name == "RoL Monster Combat" for row in compiled.native_bindings)
    )
    self.assertTrue(all(row.required_flag_bits == (0,) for row in compiled.native_bindings))
    self.assertTrue(
        all(row["strategy"] == "NATIVE_ADAPTED" for row in compiled.dispositions)
    )

  def test_pit_fiend_composes_bite_and_tail_with_monster_combat(self) -> None:
    bindings = [
        {
            "basename": "devil",
            "record_type": "mobile",
            "source_vnum": 81706,
            "source_handler": "devil_pitFiendTail",
        },
        {
            "basename": "devil",
            "record_type": "mobile",
            "source_vnum": 81706,
            "source_handler": "devil_pitFiendBite",
        },
    ]

    compiled = compile_special_bindings(
        bindings,
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    self.assertEqual(2, len(compiled.native_bindings))
    self.assertIsNone(compiled.native_bindings[0].persisted_name)
    self.assertEqual("RoL Monster Combat", compiled.native_bindings[1].persisted_name)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])
    self.assertEqual(
        "NATIVE_ADAPTED_COMPOSABLE", compiled.dispositions[1]["strategy"]
    )

  def test_trade_bandit_binding_requires_mobile_activity_gateway(self) -> None:
    binding = {
        "basename": "trade",
        "record_type": "mobile",
        "source_vnum": 99501,
        "source_handler": "bandit",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Trade Bandit", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#99501\nbandit~\na trade bandit~\nA trade bandit waits here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n20 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_099_501,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20995.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Trade Bandit", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_sister_knight_binding_requires_mobile_combat_gateway(self) -> None:
    binding = {
        "basename": "moonshae",
        "record_type": "mobile",
        "source_vnum": 26218,
        "source_handler": "sister_knight",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Sister Knight", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#26218\nsister knight~\na sister knight~\nA sister knight waits here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n20 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_026_218,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20262.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Sister Knight", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_bloodstone_critter_binding_requires_mobile_activity_gateway(self) -> None:
    binding = {
        "basename": "bs1",
        "record_type": "mobile",
        "source_vnum": 7110,
        "source_handler": "bs_critter",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Bloodstone Critter", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#7110\ndemon vrock~\na vrock demon~\nA vrock demon waits here.~\n~\n"
        b"0 0 0 0 S\nU 0 0\n20 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_007_110,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20007.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Bloodstone Critter", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_item_block_binding_preserves_authored_direction_value(self) -> None:
    binding = {
        "basename": "misc_code",
        "record_type": "object",
        "source_vnum": 891,
        "source_handler": "item_block",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Item Blocker", native.persisted_name)
    self.assertEqual((), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "obj",
        b"#891\nATD device~\nan ATD north blocker~\nAn ATD blocks north.~\n~\n"
        b"13 0 0\n0 0 0 0\n100 0 0\n",
    )
    emitted = emit_object(
        source,
        2_000_891,
        _resolver,
        special_proc=native.persisted_name,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20000.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Item Blocker", result.records[0].spec_proc)
    self.assertEqual(0, result.records[0].values[0])

  def test_floating_pool_binding_requires_room_object_pulse_gateway(self) -> None:
    binding = {
        "basename": "ethereal",
        "record_type": "object",
        "source_vnum": 22706,
        "source_handler": "floating_pool",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Floating Pool", native.persisted_name)
    self.assertEqual((44,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "obj",
        b"#22706\nfloating pool~\na floating pool~\nA floating pool is here.~\n~\n"
        b"12 0 0\n0 0 0 0\n100 0 0\n",
    )
    emitted = emit_object(
        source,
        2_022_706,
        _resolver,
        special_proc=native.persisted_name,
        required_extra_bits=native.required_flag_bits,
    )
    path = self._target_path("obj", emitted.text)
    result = parse_object_file(path, "obj/20227.obj", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Floating Pool", result.records[0].spec_proc)
    self.assertIn(44, decode_tokens(result.records[0].extra_flags).bits)

  def test_designated_follower_binding_requires_mobile_activity_gateway(self) -> None:
    binding = {
        "basename": "icecrag",
        "record_type": "mobile",
        "source_vnum": 97009,
        "source_handler": "follow_that_mob",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Designated Follower", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#97009\nvault sentinel~\na vault sentinel~\nA sentinel waits here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n20 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_097_009,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20970.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Designated Follower", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)

  def test_lich_energy_drain_binding_requires_mobile_activity_gateway(self) -> None:
    binding = {
        "basename": "rib",
        "record_type": "mobile",
        "source_vnum": 9040,
        "source_handler": "lich_energy_drain",
    }

    compiled = compile_special_bindings(
        [binding],
        2_100_000,
        lambda kind, vnum: 2_000_000 + vnum,
        [],
    )

    native = compiled.native_bindings[0]
    self.assertEqual("RoL Lich Energy Drain", native.persisted_name)
    self.assertEqual((0,), native.required_flag_bits)
    self.assertEqual("NATIVE_ADAPTED", compiled.dispositions[0]["strategy"])

    source = self._source_record(
        "mob",
        b"#9040\nlich~\na skeletal lich~\nA skeletal lich waits here.~\n~\n"
        b"0 0 0 0 S\nN 0 0\n35 0 0 1d1+0 1d1+0\n0 0\n131 131 0 0\n",
    )
    emitted = emit_mobile(
        source,
        2_009_040,
        special_proc=native.persisted_name,
        special_resolved=True,
        required_action_bits=native.required_flag_bits,
    )
    path = self._target_path("mob", emitted.text)
    result = parse_mobile_file(path, "mob/20090.mob", self.manifest, set())

    self.assertTrue(result.complete)
    self.assertEqual("RoL Lich Energy Drain", result.records[0].spec_proc)
    self.assertIn(0, decode_tokens(result.records[0].action_flags).bits)


if __name__ == "__main__":
  unittest.main()
