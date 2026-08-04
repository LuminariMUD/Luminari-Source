from __future__ import annotations

from pathlib import Path
import unittest

from wtool_lib.constants import default_repo_root


class QuestContractTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.fixtures = default_repo_root() / "scripts/world/tests/fixtures/phase2"

  def read_lines(self, relative_path: str) -> list[str]:
    path = self.fixtures / relative_path
    content = path.read_bytes()
    self.assertNotIn(b"\r", content)
    content.decode("ascii")
    return content.decode("ascii").splitlines()

  def test_canonical_writer_contract_sample_has_current_rows(self) -> None:
    lines = self.read_lines("contracts/qst/canonical.qst")
    self.assertEqual("#10100", lines[0])
    self.assertEqual(7, len(lines[6].split()))
    self.assertEqual(7, len(lines[7].split()))
    self.assertEqual(7, len(lines[8].split()))
    self.assertEqual("D", lines[9])
    self.assertEqual(4, len(lines[10].split()))
    self.assertEqual("S", lines[11])
    self.assertEqual("$~", lines[12])

  def test_legacy_contract_sample_preserves_loader_compatibility(self) -> None:
    lines = self.read_lines("contracts/qst/legacy.qst")
    self.assertEqual(3, len(lines[8].split()))
    self.assertNotIn("D", lines)
    self.assertEqual("S", lines[-2])
    self.assertEqual("$", lines[-1])

  def test_complete_fixture_has_normal_and_mini_qst_packages(self) -> None:
    complete = self.fixtures / "complete/qst"
    self.assertEqual("100.qst\n$\n", (complete / "index").read_text(encoding="ascii"))
    self.assertEqual(
        (complete / "index").read_bytes(),
        (complete / "index.mini").read_bytes(),
    )
    lines = self.read_lines("complete/qst/100.qst")
    self.assertEqual(["#10000", "#10001"], [line for line in lines if line.startswith("#")])


if __name__ == "__main__":
  unittest.main()
