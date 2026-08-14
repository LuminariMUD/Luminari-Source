from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from wtool_lib.models import SourceSpan, WorldData, ZoneRecord
from wtool_lib.rol_discovery import (
    build_capability_matrix,
    build_persistent_binding_inventory,
    build_target_catalog,
    extract_spec_bindings,
    extract_source_commands,
    lineage_candidates,
    resolve_reference,
)
from wtool_lib.rol_source import RolRecord, RolReference, RolSourceCorpus


class RolDiscoveryTests(unittest.TestCase):
  def setUp(self) -> None:
    self.policy = {
        "identity": {
            "canonical_formula": {"zone_offset": 20000, "entity_offset": 2000000},
            "new_zone_range": {"start": 20000, "end": 29999, "offset": 20000},
            "new_entity_range": {
                "start": 2000000,
                "end": 2999999,
                "offset": 2000000,
            },
            "normalizations": [],
        }
    }

  def test_source_binding_extraction_respects_preprocessor_configuration(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      (root / "src").mkdir()
      (root / "src/specs.assign.c").write_text(
          "#define ENABLED\n"
          "#ifdef ENABLED\n"
          'AddProcMob(100, active_handler, "active");\n'
          "#endif\n"
          "#if 0\n"
          'AddProcMob(101, disabled_handler, "disabled");\n'
          "#endif\n",
          encoding="ascii",
      )

      raw = extract_spec_bindings(root, "src/specs.assign.c", "source")
      active = extract_spec_bindings(
          root, "src/specs.assign.c", "source", preprocess=True
      )

    self.assertEqual(["active_handler", "disabled_handler"], [row["handler"] for row in raw])
    self.assertEqual(["active_handler"], [row["handler"] for row in active])

  def test_source_binding_extraction_follows_active_registration_wrappers(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      (root / "src").mkdir()
      (root / "src/specs.assign.c").write_text(
          "void assignWrapped(void);\n"
          "void assignDynamic(void);\n"
          "void assign_mobiles(void)\n"
          "{\n"
          '  AddProcMob(100, direct_handler, "direct");\n'
          "  assignWrapped();\n"
          "  assignDynamic();\n"
          "}\n"
          "void assign_objects(void) {}\n"
          "void assign_rooms(void) {}\n",
          encoding="ascii",
      )
      (root / "src/specs.wrapped.c").write_text(
          "#define WRAPPED_VNUM 200\n"
          "void assignWrapped(void)\n"
          "{\n"
          '  AddProcObj(WRAPPED_VNUM, wrapped_handler, "wrapped");\n'
          "#if 0\n"
          '  AddProcObj(201, disabled_handler, "disabled");\n'
          "#endif\n"
          "}\n",
          encoding="ascii",
      )
      (root / "src/shop.c").write_text(
          "void assignDynamic(void)\n"
          "{\n"
          '  AddProcMob(mob_index[keeper].virtual, shop_keeper, "shopkeep");\n'
          "}\n",
          encoding="ascii",
      )

      raw = extract_spec_bindings(
          root,
          "src/specs.assign.c",
          "source",
          follow_registration_wrappers=True,
      )
      active = extract_spec_bindings(
          root,
          "src/specs.assign.c",
          "source",
          preprocess=True,
          follow_registration_wrappers=True,
      )

    self.assertEqual(
        {"direct_handler", "shop_keeper", "wrapped_handler", "disabled_handler"},
        {row["handler"] for row in raw},
    )
    self.assertEqual(
        {"direct_handler", "shop_keeper", "wrapped_handler"},
        {row["handler"] for row in active},
    )
    wrapped = next(row for row in active if row["handler"] == "wrapped_handler")
    dynamic = next(row for row in active if row["handler"] == "shop_keeper")
    self.assertEqual(200, wrapped["vnum"])
    self.assertEqual("WRAPPED_VNUM", wrapped["vnum_token"])
    self.assertEqual("preprocessor", wrapped["vnum_resolution"])
    self.assertEqual(["assign_mobiles", "assignWrapped"], wrapped["registration_path"])
    self.assertIsNone(dynamic["vnum"])
    self.assertEqual("dynamic", dynamic["registration_kind"])

  def test_documented_formula_and_identity_seed_is_confirmed(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      world_root = Path(temporary)
      (world_root / "zon").mkdir()
      (world_root / "zon/1507.zon").write_text("#1507\n", encoding="ascii")
      target = ZoneRecord(
          1507,
          SourceSpan("zon/1507.zon", 1),
          "1507",
          name="Hulburg Trail",
      )
      catalog = build_target_catalog(WorldData(zones=[target]), world_root)
      source = RolRecord(
          "zon",
          507,
          "trail",
          "areas/zon/trail.zon",
          1,
          10,
          "a" * 64,
          identity="Hulburg Trail",
      )
      row = lineage_candidates(source, catalog, {}, self.policy)
      self.assertEqual("candidates", row["candidate_state"])
      self.assertTrue(row["candidates"][0]["confirmed_seed"])
      self.assertEqual(
          ["documented_traced_seed", "exact_normalized_identity", "legacy_lineage_formula"],
          row["candidates"][0]["evidence"],
      )

  def test_reference_resolution_distinguishes_active_excluded_and_target(self) -> None:
    reference = RolReference("mobile", 100, "test", "sample", 1)
    self.assertEqual(
        ("active_source", "map_active_definition"),
        resolve_reference(reference, {("mobile", 100)}, set(), set(), set(), set()),
    )
    self.assertEqual(
        ("excluded_source", "exclude_dependent_instruction"),
        resolve_reference(
            reference,
            {("mobile", 100)},
            {("mobile", 100)},
            set(),
            set(),
            set(),
        ),
    )
    self.assertEqual(
        ("target_canonical", "reconcile_canonical_target"),
        resolve_reference(reference, set(), set(), set(), {("mobile", 2000100)}, set()),
    )
    self.assertEqual(
        ("target_lineage_candidate", "resolve_lineage_before_emission"),
        resolve_reference(reference, set(), set(), set(), {("mobile", 100100)}, set()),
    )
    out_of_range = RolReference("object", 2147483647, "exit_key", "sample", 2)
    self.assertEqual(
        ("unresolved", "exclude_dependent_instruction"),
        resolve_reference(out_of_range, set(), set(), set(), set(), set()),
    )

  def test_capability_rows_are_counted_and_owned(self) -> None:
    record = RolRecord(
        "zon",
        1,
        "sample",
        "areas/zon/sample.zon",
        1,
        2,
        "b" * 64,
        directives=[{"token": "F", "line": 2}],
    )
    rows = build_capability_matrix(RolSourceCorpus(records=[record]))
    self.assertEqual("A", rows[0]["classification"])
    self.assertEqual("owned", rows[0]["status"])

  def test_source_command_inventory_includes_special_actions(self) -> None:
    repo_root = Path(__file__).resolve().parents[3]
    source_root = repo_root / "EXAMPLE/RealmsOfLuminari"
    result = extract_source_commands(source_root)
    self.assertGreater(result["command_count"], 100)
    self.assertEqual(
        list(range(1000, 1005)),
        [entry["action_code"] for entry in result["special_actions"]],
    )

  def test_persistent_inventory_uses_traced_semantics_and_flags_unknowns(self) -> None:
    schema = (
        "active_region_hints\tregion_vnum\tint\n"
        "house_data\tvnum\tint\n"
        "mystery\tstrange_vnum\tint\n"
        "player_save_objs\tserialized_obj\tblob\n"
        "ship_runtime_state\tlocation_vnum\tint\n"
    )

    def mysql_result(_config: dict[str, str], query: str) -> str:
      if "information_schema.COLUMNS" in query:
        return schema
      if "SELECT COUNT(*) FROM `player_save_objs`" in query:
        return "3\n"
      if "active_region_hints" in query:
        return "7\n"
      if "house_data" in query:
        return "159125\n"
      if "mystery" in query:
        return "12\n"
      if "ship_runtime_state" in query:
        return "196004\n"
      raise AssertionError(query)

    config = {
        "mysql_host": "localhost",
        "mysql_database": "test",
        "mysql_username": "test",
        "mysql_password": "test",
    }
    with patch("wtool_lib.rol_discovery._parse_mysql_config", return_value=config), patch(
        "wtool_lib.rol_discovery._run_mysql", side_effect=mysql_result
    ):
      inventory = build_persistent_binding_inventory(Path("unused"))

    rows = {(row["table"], row["column"]): row for row in inventory["columns"]}
    self.assertEqual("region", rows[("active_region_hints", "region_vnum")]["record_type"])
    self.assertFalse(rows[("active_region_hints", "region_vnum")]["migration_required"])
    self.assertEqual("room", rows[("house_data", "vnum")]["record_type"])
    self.assertEqual(
        "object_header_blob",
        rows[("player_save_objs", "serialized_obj")]["encoding"],
    )
    self.assertEqual("room", rows[("ship_runtime_state", "location_vnum")]["record_type"])
    self.assertEqual("unclassified", rows[("mystery", "strange_vnum")]["record_type"])
    self.assertEqual(1, inventory["unclassified_columns"])


if __name__ == "__main__":
  unittest.main()
