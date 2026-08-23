from __future__ import annotations

from collections import Counter
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import unittest
from unittest.mock import patch

from wtool_lib.rol_persistence_check import (
    RolPersistenceCheckError,
    _development_database_config,
    _run_read_only_query,
    audit_development_persistence,
)


class _World:
  def __init__(self, definitions: dict[str, list[int]]) -> None:
    self.definitions = definitions

  def records(self, record_type: str) -> list[SimpleNamespace]:
    return [
        SimpleNamespace(vnum=vnum)
        for vnum in self.definitions.get(record_type, [])
    ]


class RolPersistenceCheckTests(unittest.TestCase):
  def test_database_configuration_is_fixed_to_local_development(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      lib_root = root / "lib"
      lib_root.mkdir()
      (lib_root / ".env").write_text("APP_ENV=testing\n", encoding="ascii")
      (lib_root / "mysql_config").write_text(
          "mysql_host=localhost\n"
          "mysql_database=luminari\n"
          "mysql_username=luminari\n"
          "mysql_password=secret\n",
          encoding="ascii",
      )

      with self.assertRaisesRegex(RolPersistenceCheckError, "development environment"):
        _development_database_config(root)

  def test_database_configuration_accepts_explicit_development_lib_root(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      root = Path(temporary)
      repository = root / "repository"
      lib_root = root / "development-lib"
      repository.mkdir()
      lib_root.mkdir()
      (lib_root / ".env").write_text("APP_ENV=development\n", encoding="ascii")
      (lib_root / "mysql_config").write_text(
          "mysql_host=localhost\n"
          "mysql_database=luminari\n"
          "mysql_username=luminari\n"
          "mysql_password=secret\n",
          encoding="ascii",
      )

      config_path, config = _development_database_config(repository, lib_root)

    self.assertEqual((lib_root / "mysql_config").resolve(), config_path)
    self.assertEqual("luminari", config["mysql_database"])

  def test_query_runner_rejects_every_write_shape(self) -> None:
    with self.assertRaisesRegex(RolPersistenceCheckError, "non-read-only"):
      _run_read_only_query({}, "UPDATE player_save_objs SET serialized_obj='' ")
    with self.assertRaisesRegex(RolPersistenceCheckError, "non-read-only"):
      _run_read_only_query({}, "SELECT 1; DELETE FROM player_save_objs")
    with self.assertRaisesRegex(RolPersistenceCheckError, "non-read-only"):
      _run_read_only_query({}, "SELECT serialized_obj INTO OUTFILE '/tmp/leak'")
    with self.assertRaisesRegex(RolPersistenceCheckError, "non-read-only"):
      _run_read_only_query({}, "SELECT GET_LOCK('rol', 10)")

  def test_query_runner_enforces_a_read_only_database_session(self) -> None:
    config = {
        "mysql_host": "localhost",
        "mysql_database": "luminari",
        "mysql_username": "luminari",
        "mysql_password": "secret",
    }
    completed = SimpleNamespace(returncode=0, stdout="1\n", stderr="")
    with patch(
        "wtool_lib.rol_persistence_check.shutil.which",
        return_value="/usr/bin/mariadb",
    ), patch(
        "wtool_lib.rol_persistence_check.subprocess.run",
        return_value=completed,
    ) as run:
      output = _run_read_only_query(config, "SELECT 1")

    self.assertEqual("1\n", output)
    arguments = run.call_args.args[0]
    self.assertIn(
        "--init-command=SET SESSION TRANSACTION READ ONLY",
        arguments,
    )

  def test_audit_requires_each_persisted_vnum_to_resolve_once(self) -> None:
    bindings = (
        {
            "table": "player_save_objs",
            "column": "serialized_obj",
            "record_type": "object",
            "encoding": "object_header_blob",
            "predicate": None,
        },
    )
    world = _World({"object": [2_019_912]})
    config_path = Path("/repo/lib/mysql_config")
    config = {
        "mysql_host": "localhost",
        "mysql_database": "luminari",
        "mysql_username": "luminari",
        "mysql_password": "secret",
    }
    with patch(
        "wtool_lib.rol_persistence_check.PERSISTENT_BINDINGS", bindings
    ), patch(
        "wtool_lib.rol_persistence_check._development_database_config",
        return_value=(config_path, config),
    ), patch(
        "wtool_lib.rol_persistence_check._database_schema",
        return_value={("player_save_objs", "serialized_obj")},
    ), patch(
        "wtool_lib.rol_persistence_check._binding_values",
        return_value=Counter({2_019_912: 1, 2_019_913: 2}),
    ):
      result = audit_development_persistence(world, Path("/repo"))

    self.assertFalse(result["summary"]["pass"])
    self.assertEqual(1, result["summary"]["missing_candidate_definitions"])
    self.assertEqual(3, result["summary"]["database_rows"])
    self.assertNotIn("mysql_password", result)

  def test_audit_redacts_explicit_development_lib_root(self) -> None:
    world = _World({})
    config_path = Path("/private/development/lib/mysql_config")
    config = {
        "mysql_host": "localhost",
        "mysql_database": "luminari",
        "mysql_username": "luminari",
        "mysql_password": "secret",
    }
    with patch(
        "wtool_lib.rol_persistence_check._development_database_config",
        return_value=(config_path, config),
    ), patch(
        "wtool_lib.rol_persistence_check._database_schema",
        return_value=set(),
    ):
      result = audit_development_persistence(
          world,
          Path("/repo"),
          Path("/private/development/lib"),
      )

    self.assertEqual(
        "explicit-development-lib-root", result["configuration_scope"]
    )
    self.assertEqual(
        "external-development-lib/mysql_config", result["configuration"]
    )
    self.assertNotIn("/private", json.dumps(result))


if __name__ == "__main__":
  unittest.main()
