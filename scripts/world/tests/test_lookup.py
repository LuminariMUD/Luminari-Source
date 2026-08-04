from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from tests.test_cli import make_world, run_cli, tree_hash


class LookupTests(unittest.TestCase):
  def test_quest_show_supports_json_human_and_qst_alias(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      before = tree_hash(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "show", "qst", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      self.assertEqual(1, payload["schema_version"])
      self.assertEqual("0.2.0", payload["tool_version"])
      self.assertEqual("quest", payload["record_type"])
      self.assertEqual("Test Quest", payload["matches"][0]["name"])
      self.assertEqual(100, payload["matches"][0]["questmaster_vnum"])

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "show", "quest", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      self.assertIn("quest 100 (qst/1.qst:1)", stdout)
      self.assertIn("questmaster_vnum: 100", stdout)
      self.assertEqual(before, tree_hash(root))

  def test_hlquest_show_exposes_physical_and_runtime_order(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "hlq/1.hlq").write_text(
          "#100\n"
          "A!\nfirst~\nFirst reply.~\n"
          "A!\nsecond~\nSecond reply.~\n"
          "A!\nthird~\nThird reply.~\n"
          "$~\n",
          encoding="ascii",
      )
      before = tree_hash(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "show", "hlq", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      entries = json.loads(stdout)["matches"][0]["entries"]
      self.assertEqual([1, 2, 3], [entry["physical_ordinal"] for entry in entries])
      self.assertEqual([3, 2, 1], [entry["effective_runtime_ordinal"] for entry in entries])

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "show", "hlquest", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      self.assertIn("entries (physical order):", stdout)
      self.assertIn("entry physical=1 runtime=3", stdout)
      self.assertIn("entry physical=3 runtime=1", stdout)
      self.assertEqual(before, tree_hash(root))

  def test_hlquest_show_keeps_duplicate_host_blocks_separate(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "hlq/1.hlq").write_text(
          "#100\nA!\nfirst~\nFirst reply.~\n"
          "#100\nA!\nsecond~\nSecond reply.~\n$~\n",
          encoding="ascii",
      )
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "show", "hlquest", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      self.assertEqual(2, len(payload["matches"]))
      self.assertGreater(payload["lookup_parse_errors"], 0)

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

  def test_quest_system_refs_are_bidirectional_in_json_and_human_output(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      before = tree_hash(root)

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "refs", "qst", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      outgoing = json.loads(stdout)["outgoing"]
      self.assertTrue(
          any(edge["target_type"] == "mobile" for edge in outgoing)
      )
      self.assertTrue(
          any(edge["target_type"] == "object" for edge in outgoing)
      )

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "refs", "hlq", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      self.assertTrue(
          any(
              edge["target_type"] == "mobile"
              and edge["role"] == "attached host mobile"
              for edge in json.loads(stdout)["outgoing"]
          )
      )

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "refs", "object", "100"]
      )
      self.assertEqual(0, status)
      incoming = json.loads(stdout)["incoming"]
      self.assertTrue(any(edge["source_type"] == "quest" for edge in incoming))

      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "refs", "mobile", "100"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      self.assertIn("quest 100: questmaster mobile", stdout)
      self.assertIn("hlquest 100: attached host mobile", stdout)
      self.assertEqual(before, tree_hash(root))

  def test_existing_six_record_payloads_match_compatibility_golden(self) -> None:
    golden_path = (
        Path(__file__).resolve().parent
        / "fixtures/phase2/contracts/lookup/existing-six.json"
    )
    expected = json.loads(golden_path.read_text(encoding="ascii"))
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      actual = {}
      for record_type, vnum in (
          ("zone", 1),
          ("room", 100),
          ("mob", 100),
          ("obj", 100),
          ("shop", 100),
          ("trigger", 100),
      ):
        status, stdout, stderr = run_cli(
            [
                "--world-root",
                str(root),
                "--json",
                "show",
                record_type,
                str(vnum),
            ]
        )
        self.assertEqual(0, status)
        self.assertEqual("", stderr)
        payload = json.loads(stdout)
        self.assertEqual(1, payload["schema_version"])
        self.assertEqual("0.2.0", payload["tool_version"])
        actual[payload["record_type"]] = payload["matches"][0]
      self.assertEqual(expected, actual)

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
