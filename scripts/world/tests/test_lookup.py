from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from tests.test_cli import make_world, run_cli, tree_hash


class LookupTests(unittest.TestCase):
  def test_show_uses_typed_model_and_is_read_only(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      before = tree_hash(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "show", "room", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      self.assertTrue(payload["found"])
      self.assertEqual("room", payload["record_type"])
      self.assertEqual("Test Room", payload["matches"][0]["name"])
      self.assertEqual(before, tree_hash(root))

  def test_show_type_disambiguates_overlapping_vnums(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      status, stdout, _ = run_cli(
          ["--world-root", str(root), "--json", "show", "obj", "100"]
      )
      payload = json.loads(stdout)
      self.assertEqual(0, status)
      self.assertEqual("object", payload["record_type"])
      self.assertEqual("test key", payload["matches"][0]["aliases"])

  def test_refs_reports_typed_incoming_edges_and_is_read_only(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      before = tree_hash(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "refs", "obj", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      incoming = payload["incoming"]
      self.assertTrue(
          any(edge["source_type"] == "shop" and edge["role"] == "shop product" for edge in incoming)
      )
      self.assertEqual(before, tree_hash(root))

  def test_refs_reports_room_exit_outgoing_edge(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      room = root / "wld/1.wld"
      room.write_text(
          "#100\nTest Room~\nA test room.~\n1 0 0 0 0 0\n"
          "D0\n~\n~\n0 -1 100\nS\n$~\n",
          encoding="ascii",
      )
      status, stdout, _ = run_cli(
          ["--world-root", str(root), "--json", "refs", "room", "100"]
      )
      payload = json.loads(stdout)
      self.assertEqual(0, status)
      self.assertTrue(
          any(edge["target_type"] == "room" and edge["target_vnum"] == 100
              for edge in payload["outgoing"])
      )

  def test_missing_lookup_is_stable_status_one(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      first = run_cli(
          ["--world-root", str(root), "--json", "show", "trigger", "999"]
      )
      second = run_cli(
          ["--world-root", str(root), "--json", "show", "trigger", "999"]
      )
      self.assertEqual(1, first[0])
      self.assertEqual(first, second)
      self.assertFalse(json.loads(first[1])["found"])


if __name__ == "__main__":
  unittest.main()
