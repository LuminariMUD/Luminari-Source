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
    (directory / f"1.{extension}").write_text("#1\n$\n", encoding="ascii")


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


if __name__ == "__main__":
  unittest.main()
