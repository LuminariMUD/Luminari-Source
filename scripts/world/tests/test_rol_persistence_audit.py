from __future__ import annotations

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from wtool_lib.rol_persistence_audit import (
    _indexed_object_definition_counts,
    _persistent_object_prototype_resolution,
)


class RolPersistenceAuditTests(unittest.TestCase):
  def test_indexed_object_definition_counts_follow_active_index(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      lib_root = Path(temporary)
      object_root = lib_root / "world/obj"
      object_root.mkdir(parents=True)
      (object_root / "index").write_text("20010.obj\n20053.obj\n$\n", encoding="ascii")
      (object_root / "20010.obj").write_text(
          "#2001007\nfirst~\n#2001008\nsecond~\n$~\n", encoding="ascii"
      )
      (object_root / "20053.obj").write_text(
          "#2001008\nduplicate~\n#2005343\nthird~\n$~\n", encoding="ascii"
      )

      counts = _indexed_object_definition_counts(lib_root)

      self.assertEqual(1, counts[2001007])
      self.assertEqual(2, counts[2001008])
      self.assertEqual(1, counts[2005343])

  def test_saved_object_resolution_reports_missing_and_duplicate_headers(self) -> None:
    with tempfile.TemporaryDirectory() as temporary:
      lib_root = Path(temporary)
      object_root = lib_root / "world/obj"
      object_root.mkdir(parents=True)
      (object_root / "index").write_text("20010.obj\n20053.obj\n$\n", encoding="ascii")
      (object_root / "20010.obj").write_text(
          "#2001007\nfirst~\n#2001008\nsecond~\n$~\n", encoding="ascii"
      )
      (object_root / "20053.obj").write_text(
          "#2001008\nduplicate~\n$~\n", encoding="ascii"
      )
      consumers = [
          {
              "table": "player_save_objs",
              "column": "serialized_obj",
              "record_type": "object",
              "encoding": "object_header_blob",
              "migration_required": True,
              "predicate": None,
          }
      ]
      schema = {
          ("player_save_objs", "serialized_obj"): {
              "data_type": "blob",
              "table_type": "BASE TABLE",
          }
      }

      with patch(
          "wtool_lib.rol_persistence_audit._run_mysql",
          return_value="2001007\n2001008\n2001043\n",
      ):
        result = _persistent_object_prototype_resolution(
            {"mysql_host": "isolated", "mysql_database": "test"},
            consumers,
            schema,
            lib_root,
        )

      self.assertEqual(3, result["referenced_canonical_object_vnums"])
      self.assertEqual(1, result["resolved_unique_object_vnums"])
      self.assertEqual(1, result["missing_object_prototypes"])
      self.assertEqual(1, result["nonunique_object_prototypes"])


if __name__ == "__main__":
  unittest.main()
