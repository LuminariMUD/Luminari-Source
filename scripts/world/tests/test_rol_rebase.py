from __future__ import annotations

from pathlib import Path
import hashlib
import json
import sqlite3
import tempfile
import unittest
from unittest.mock import patch

from wtool_lib.rol_rebase import (
    _ARTIFACT_OBJECT_PACKAGES,
    _database_preflight_sql,
    _database_sql,
    _database_updates,
    _migrate_artifact_state,
    _migrate_object_store,
    _rewrite_world,
    _split_artifact_objects,
    _world_role,
    apply_rebase_bundle,
    retired_destination,
)


class RolRebaseTests(unittest.TestCase):
  def test_known_core_and_artifact_rehomes_are_exact(self) -> None:
    self.assertEqual(20507, retired_destination(1507))
    self.assertEqual(2050700, retired_destination(150700))
    self.assertEqual(2059599, retired_destination(159599))
    self.assertEqual(2096299, retired_destination(196299))
    self.assertEqual(2001007, retired_destination(169906))
    self.assertEqual(2019730, retired_destination(169910))
    self.assertIsNone(retired_destination(1506))
    self.assertIsNone(retired_destination(196300))

  def test_zone_numbers_do_not_rewrite_target_owned_entity_namespaces(self) -> None:
    self.assertEqual(
        ("retain", "target-owned mob identity or untyped numeric match"),
        _world_role("mob/15.mob", 10, "#1507", 1507, {}),
    )
    self.assertEqual(
        ("retain", "target-owned trg identity or untyped numeric match"),
        _world_role("trg/19.trg", 10, "#1960", 1960, {}),
    )
    self.assertEqual(
        ("rewrite", "owned legacy zone header"),
        _world_role("zon/1591.zon", 1, "#1591", 1591, {}),
    )
    self.assertEqual(
        ("rewrite", "owned legacy room zone field"),
        _world_role("wld/1507.wld", 4, "1507 0 0 0 0 2", 1507, {}),
    )

  def test_artifact_state_moves_without_cloning_kelrarin(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      source = root / "world.artifact"
      destination = root / "migrated/world.artifact"
      tail = " ".join("0" for _ in range(16))
      legacy_vnums = [*range(169901, 169912), *range(169913, 169919)]
      rows = []
      for vnum in legacy_vnums:
        if vnum == 169906:
          rows.append(
              f"{vnum} Owner Account 3 50 123 1 First FirstAccount {tail}"
          )
        else:
          rows.append(f"{vnum} noone noone 1 0 0 0 noone noone {tail}")
      source.write_text(
          "# Artifact Ownership File v2.4\n\n" + "\n".join(rows) + "\n",
          encoding="ascii",
      )

      report = _migrate_artifact_state(source, destination)
      migrated_rows = [
          line.split()
          for line in destination.read_text(encoding="ascii").splitlines()
          if line and not line.startswith("#")
      ]

    by_vnum = {int(row[0]): row for row in migrated_rows}
    self.assertEqual(10, report["migrated_rows"])
    self.assertEqual(0, report["state_clones"])
    self.assertEqual("Owner", by_vnum[2001007][1])
    self.assertEqual("noone", by_vnum[2001009][1])
    self.assertNotIn(169906, by_vnum)

  def test_artifact_package_restores_named_second_kelrarin_identity(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      world_root = Path(temporary) / "world"
      object_root = world_root / "obj"
      zone_root = world_root / "zon"
      object_root.mkdir(parents=True)
      zone_root.mkdir()
      for name, vnums in _ARTIFACT_OBJECT_PACKAGES.items():
        blocks = []
        for vnum in sorted(vnums - {2001009}):
          if vnum == 2001007:
            blocks.extend(
                [
                    "#2001007",
                    "hammer kelrarin artifact~",
                    "@WKelrarin's Hammer@n~",
                    "@WA warhammer rests here.@n~",
                    "~",
                    "E",
                    "hammer kelrarin artifact~",
                    "A storm-forged hammer.~",
                ]
            )
          else:
            blocks.extend(
                [
                    f"#{vnum}",
                    f"artifact {vnum}~",
                    f"Artifact {vnum}~",
                    "An artifact rests here.~",
                    "~",
                ]
            )
        (object_root / name).write_text(
            "\n".join([*blocks, "$~", ""]), encoding="ascii"
        )
      (zone_root / "1699.zon").write_text(
          "#1699\nVault~\n169999 30 2 0\nO 0 2001007 1 169900 100\nS\n$\n",
          encoding="ascii",
      )

      rows = _split_artifact_objects(world_root)
      package = (object_root / "20010.obj").read_text(encoding="ascii")
      zone = (zone_root / "1699.zon").read_text(encoding="ascii")

    self.assertIn("#2001009\n", package)
    self.assertIn("Kelrarin's Second Hammer", package)
    self.assertIn("hammer kelrarin second artifact~", package)
    self.assertIn("O 0 2001009 1 169900 100", zone)
    self.assertTrue(
        any(row["operation"] == "RESTORE_DISTINCT_IDENTITY" for row in rows)
    )

  def test_player_object_headers_are_migrated_idempotently(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      source = root / "source"
      first = root / "first"
      second = root / "second"
      source.mkdir()
      (source / "player.objs").write_text(
          "2 0 0 0 0 0\n#159228\nLoc : 1\n#169906\nLoc : 2\n",
          encoding="ascii",
      )
      report = _migrate_object_store(source, first)
      second_report = _migrate_object_store(first, second)

      migrated = (first / "player.objs").read_text(encoding="ascii")

    self.assertEqual(1, len(report))
    self.assertEqual([], second_report)
    self.assertIn("#2059228\n", migrated)
    self.assertIn("#2001007\n", migrated)

  def test_world_rewrite_preserves_legacy_non_ascii_bytes_and_line_endings(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      world_root = Path(temporary) / "world"
      (world_root / "wld").mkdir(parents=True)
      path = world_root / "wld/42.wld"
      path.write_bytes(b"#42\r\nlegacy \xff text\r\n0 0 150700\r\n")

      rewrites, retained = _rewrite_world(
          world_root, Path(__file__).resolve().parents[3]
      )

      self.assertEqual([], retained)
      self.assertEqual(1, len(rewrites))
      self.assertEqual(
          b"#42\r\nlegacy \xff text\r\n0 0 2050700\r\n", path.read_bytes()
      )

  def test_database_migration_is_transactional_and_idempotent(self) -> None:
    columns = [
        {"table": "zones", "column": "zone_vnum", "record_type": "zone"},
        {"table": "rooms", "column": "room_vnum", "record_type": "room"},
        {"table": "objects", "column": "object_vnum", "record_type": "object"},
        {"table": "ignored", "column": "value_vnum", "record_type": "unclassified"},
    ]
    sql = _database_sql(columns)
    sqlite_sql = "BEGIN TRANSACTION;\n" + "\n".join(
        _database_updates(columns)
    ) + "\nCOMMIT;\n"
    connection = sqlite3.connect(":memory:")
    connection.executescript(
        "CREATE TABLE zones(zone_vnum INTEGER);"
        "CREATE TABLE rooms(room_vnum INTEGER);"
        "CREATE TABLE objects(object_vnum INTEGER);"
        "CREATE TABLE ignored(value_vnum INTEGER);"
        "INSERT INTO zones VALUES(1591);"
        "INSERT INTO rooms VALUES(196004);"
        "INSERT INTO objects VALUES(169906);"
        "INSERT INTO ignored VALUES(1591);"
    )
    connection.executescript(sqlite_sql)
    first = (
        connection.execute("SELECT zone_vnum FROM zones").fetchone()[0],
        connection.execute("SELECT room_vnum FROM rooms").fetchone()[0],
        connection.execute("SELECT object_vnum FROM objects").fetchone()[0],
        connection.execute("SELECT value_vnum FROM ignored").fetchone()[0],
    )
    connection.executescript(sqlite_sql)
    second = (
        connection.execute("SELECT zone_vnum FROM zones").fetchone()[0],
        connection.execute("SELECT room_vnum FROM rooms").fetchone()[0],
        connection.execute("SELECT object_vnum FROM objects").fetchone()[0],
        connection.execute("SELECT value_vnum FROM ignored").fetchone()[0],
    )
    connection.close()

    self.assertEqual((20591, 2096004, 2001007, 1591), first)
    self.assertEqual(first, second)
    self.assertTrue(sql.startswith("START TRANSACTION;\n"))
    self.assertTrue(sql.endswith("COMMIT;\n"))
    self.assertIn("TABLE_TYPE = 'BASE TABLE'", sql)
    self.assertIn("information_schema.COLUMNS", sql)
    self.assertIn("PREPARE rol_rebase_update_1", sql)
    self.assertTrue(_database_preflight_sql(sql).endswith("ROLLBACK;\n"))

  def test_database_migration_uses_traced_generic_vnum_semantics(self) -> None:
    columns = [
        {"table": "house_data", "column": "vnum", "record_type": "unclassified"},
        {"table": "pet_data", "column": "vnum", "record_type": "unclassified"},
        {
            "table": "ship_cargo_manifest",
            "column": "item_vnum",
            "record_type": "unclassified",
        },
        {
            "table": "active_region_hints",
            "column": "region_vnum",
            "record_type": "unclassified",
        },
    ]

    updates = "\n".join(_database_updates(columns))

    self.assertIn("UPDATE `house_data`", updates)
    self.assertIn("UPDATE `pet_data`", updates)
    self.assertIn("UPDATE `ship_cargo_manifest`", updates)
    self.assertIn("(`cargo_room` <> 0)", updates)
    self.assertNotIn("UPDATE `active_region_hints`", updates)

  def test_database_migration_covers_database_object_blobs(self) -> None:
    sql = _database_sql([])

    for table in (
        "player_save_objs",
        "player_save_objs_sheathed",
        "pet_save_objs",
        "house_data",
    ):
      self.assertIn(f"UPDATE `{table}` SET `serialized_obj`", sql)
    self.assertIn("SUBSTRING_INDEX", sql)
    self.assertIn("LOCATE(CHAR(10), `serialized_obj`)", sql)
    self.assertIn("WHEN", sql)
    self.assertIn("169906", sql)
    self.assertIn("2001007", sql)

  def test_apply_stops_before_files_if_database_commit_fails(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      lib_root = root / "lib"
      bundle = root / "bundle"
      destination = lib_root / "world/example.wld"
      staged = bundle / "output/world/example.wld"
      destination.parent.mkdir(parents=True)
      staged.parent.mkdir(parents=True)
      (lib_root / ".env").write_text("APP_ENV=development\n", encoding="ascii")
      destination.write_bytes(b"before\n")
      staged.write_bytes(b"after\n")
      persistence = bundle / "output/persistence"
      persistence.mkdir(parents=True)
      sql = "START TRANSACTION;\nDO 0;\nCOMMIT;\n"
      (persistence / "rol_phase6_5_vnum_migration.sql").write_text(
          sql, encoding="ascii"
      )
      config = root / "mysql_config"
      config.write_text(
          "mysql_host=localhost\nmysql_database=test\n"
          "mysql_username=test\nmysql_password=test\n",
          encoding="ascii",
      )
      before_hash = hashlib.sha256(b"before\n").hexdigest()
      after_hash = hashlib.sha256(b"after\n").hexdigest()
      (bundle / "change-plan.jsonl").write_text(
          json.dumps(
              {
                  "scope": "world",
                  "source_path": "example.wld",
                  "destination_path": "world/example.wld",
                  "before_sha256": before_hash,
                  "after_sha256": after_hash,
                  "action": "REPLACE",
              },
              sort_keys=True,
          )
          + "\n",
          encoding="ascii",
      )
      tree_hash = hashlib.sha256(
          b"example.wld\0" + after_hash.encode("ascii") + b"\n"
      ).hexdigest()
      (bundle / "run-manifest.json").write_text(
          json.dumps(
              {
                  "phase": "6.5",
                  "run_id": "test-rebase",
                  "world_tree_sha256": tree_hash,
                  "artifacts": [],
                  "acceptance": {"complete": True},
              }
          ),
          encoding="ascii",
      )

      with patch(
          "wtool_lib.rol_rebase._run_mysql",
          side_effect=["", RuntimeError("database commit failed")],
      ):
        with self.assertRaisesRegex(RuntimeError, "database commit failed"):
          apply_rebase_bundle(bundle, lib_root, config)

      self.assertEqual(b"before\n", destination.read_bytes())


if __name__ == "__main__":
  unittest.main()
