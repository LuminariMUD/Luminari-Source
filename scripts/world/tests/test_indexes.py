from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.indexes import DATA_EXTENSIONS, is_safe_path_component, validate_indexes


def make_world(root: Path, mini: bool = False) -> None:
  index_name = "index.mini" if mini else "index"
  for extension in DATA_EXTENSIONS:
    directory = root / extension
    directory.mkdir(parents=True, exist_ok=True)
    (directory / index_name).write_text(f"1.{extension}\n$\n", encoding="ascii")
    (directory / f"1.{extension}").write_text("#1\n$\n", encoding="ascii")


class IndexTests(unittest.TestCase):
  def test_safe_component_matches_c_contract(self) -> None:
    self.assertTrue(is_safe_path_component("123-zone.wld"))
    self.assertTrue(is_safe_path_component("zone_name.obj"))
    for unsafe in ("", ".", "..", "a..b", "../1.wld", "a/b.wld", "x y.wld"):
      self.assertFalse(is_safe_path_component(unsafe), unsafe)

  def test_valid_normal_indexes_are_clean(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      root = Path(directory) / "world"
      make_world(root)
      result = validate_indexes(root, Path(directory))
      self.assertEqual([], result.findings)
      self.assertTrue(result.complete)

  def test_index_defects_are_all_reported(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      repo = Path(directory)
      root = repo / "world"
      make_world(root)
      index = root / "wld/index"
      index.write_text("2.wld\n1.wld\n1.wld\n../bad.wld\nwrong.mob\n", encoding="ascii")
      (root / "wld/2.wld").write_text("#2\n$\n", encoding="ascii")
      (root / "wld/9.wld").write_text("#9\n$\n", encoding="ascii")
      result = validate_indexes(root, repo)
      codes = {finding.code for finding in result.findings}
      self.assertTrue({"IDX001", "IDX002", "IDX003", "IDX005", "IDX006", "IDX008"} <= codes)
      self.assertFalse(result.complete)

  def test_missing_listed_file_is_a_data_finding(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      repo = Path(directory)
      root = repo / "world"
      make_world(root)
      (root / "obj/1.obj").unlink()
      result = validate_indexes(root, repo)
      self.assertIn("IDX004", {finding.code for finding in result.findings})
      self.assertFalse(result.complete)

  def test_mini_is_intentional_subset_without_disk_omission_warnings(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      repo = Path(directory)
      root = repo / "world"
      make_world(root, mini=True)
      for extension in DATA_EXTENSIONS:
        (root / extension / f"999.{extension}").write_text("#999\n$\n", encoding="ascii")
      result = validate_indexes(root, repo, mini=True)
      self.assertNotIn("IDX008", {finding.code for finding in result.findings})


if __name__ == "__main__":
  unittest.main()
