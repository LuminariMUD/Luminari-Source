from __future__ import annotations

from contextlib import redirect_stderr, redirect_stdout
import hashlib
from io import StringIO
import json
from pathlib import Path
import tempfile
import unittest

from wtool_lib.cli import main
from wtool_lib.indexes import DATA_EXTENSIONS


def run_cli(arguments: list[str]) -> tuple[int, str, str]:
  stdout = StringIO()
  stderr = StringIO()
  with redirect_stdout(stdout), redirect_stderr(stderr):
    status = main(arguments)
  return status, stdout.getvalue(), stderr.getvalue()


def make_world(root: Path) -> None:
  for extension in DATA_EXTENSIONS:
    directory = root / extension
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "index").write_text(f"1.{extension}\n$\n", encoding="ascii")
    if extension == "zon":
      content = "#1\nBuilder~\nTest Zone~\n100 199 30 2\nS\n$\n"
    elif extension == "wld":
      content = "#100\nTest Room~\nA test room.~\n1 0 0 0 0 0\nS\n$~\n"
    elif extension == "mob":
      content = (
          "#100\ntest mobile~\na test mobile~\nA test mobile is here.~\nA test mobile.~\n"
          "0 0 0 0 0 0 0 0 0 S\n1 20 19 1d1+0 1d1+0\n0 0\n9 9 0\n$\n"
      )
    elif extension == "obj":
      content = (
          "#100\ntest key~\na test key~\nA test key is here.~\n~\n"
          "18 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0\n"
          "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n1 1 0 1 0\n$\n"
      )
    elif extension == "trg":
      content = "#100\nTest trigger~\n2 a 0\n~\n* Empty script~\n$~\n"
    elif extension == "shp":
      content = (
          "LuminariMUD v3.0 Shop File~\n#100~\n100\n-1\n1.00\n1.00\n"
          "18\n-1\n%s no item.~\n%s no item.~\n%s no buy.~\n%s no cash.~\n"
          "%s no cash.~\n%s buys for %d.~\n%s sells for %d.~\n0\n0\n100\n0\n"
          "100\n-1\n0\n28\n0\n28\n$~\n"
      )
    else:
      raise AssertionError(extension)
    (directory / f"1.{extension}").write_text(content, encoding="ascii")


def tree_hash(root: Path) -> str:
  digest = hashlib.sha256()
  for path in sorted(item for item in root.rglob("*") if item.is_file()):
    digest.update(path.relative_to(root).as_posix().encode("ascii"))
    digest.update(path.read_bytes())
  return digest.hexdigest()


class CliTests(unittest.TestCase):
  def test_flags_list_has_source_derived_room_count(self) -> None:
    status, stdout, stderr = run_cli(["flags", "list", "room"])
    self.assertEqual(0, status)
    self.assertEqual(42, len(stdout.splitlines()))
    self.assertIn("ROOM_DOCKABLE", stdout)
    self.assertEqual("", stderr)

  def test_json_flag_output_is_clean_stdout(self) -> None:
    status, stdout, stderr = run_cli(["--json", "flags", "encode", "room", "ROOM_DOCKABLE"])
    self.assertEqual(0, status)
    self.assertEqual("", stderr)
    payload = json.loads(stdout)
    self.assertEqual([41], payload["bits"])
    self.assertEqual(["0", "j", "0", "0"], payload["tokens"])

  def test_validate_is_read_only_and_json_is_repeatable(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      before = tree_hash(root)
      arguments = ["--world-root", str(root), "--json", "validate", "--all"]
      first = run_cli(arguments)
      second = run_cli(arguments)
      self.assertEqual(0, first[0])
      self.assertEqual(first, second)
      self.assertEqual("", first[2])
      self.assertEqual(before, tree_hash(root))
      self.assertTrue(json.loads(first[1])["complete"])

  def test_inaccessible_world_root_is_operational_error(self) -> None:
    status, stdout, stderr = run_cli(
        ["--world-root", "/definitely/not/a/world/root", "validate", "--all"]
    )
    self.assertEqual(2, status)
    self.assertEqual("", stdout)
    self.assertIn("inaccessible", stderr)

  def test_zone_selector_requires_a_readable_zone_package(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "validate", "--zone", "999"]
      )
      self.assertEqual(1, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      self.assertIn("IDX013", {finding["code"] for finding in payload["findings"]})

  def test_zone_selector_merges_an_unindexed_canonical_package(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      (root / "zon/2.zon").write_text(
          "#2\nBuilder~\nUnindexed~\n200 299 30 2\nS\n$\n",
          encoding="ascii",
      )
      (root / "wld/2.wld").write_text(
          "#200\nUnindexed Room~\nAn unindexed room.~\n2 0 0 0 0 0\nS\n$~\n",
          encoding="ascii",
      )
      before = tree_hash(root)
      status, stdout, stderr = run_cli(
          ["--world-root", str(root), "--json", "validate", "--zone", "2"]
      )
      self.assertEqual(0, status)
      self.assertEqual("", stderr)
      payload = json.loads(stdout)
      self.assertEqual("zone", payload["mode"])
      self.assertEqual([2], payload["config"]["selected_zones"])
      self.assertIn("IDX008", {finding["code"] for finding in payload["findings"]})
      self.assertEqual(before, tree_hash(root))


if __name__ == "__main__":
  unittest.main()
