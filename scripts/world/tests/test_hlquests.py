from __future__ import annotations

import unittest

from wtool_lib.constants import default_repo_root, extract_manifest


class HlQuestContractTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.fixtures = cls.repo_root / "scripts/world/tests/fixtures/phase2"
    cls.manifest = extract_manifest(cls.repo_root)

  def read_lines(self, relative_path: str) -> list[str]:
    path = self.fixtures / relative_path
    content = path.read_bytes()
    self.assertNotIn(b"\r", content)
    content.decode("ascii")
    return content.decode("ascii").splitlines()

  def test_canonical_writer_contract_covers_all_command_codes(self) -> None:
    lines = self.read_lines("contracts/hlq/canonical.hlq")
    expected_codes = "".join(
        entry["code"] for entry in self.manifest["tables"]["hlquest-commands"]["entries"]
    )
    input_codes = "".join(line.split()[1] for line in lines if line.startswith("I "))
    output_codes = "".join(line.split()[1] for line in lines if line.startswith("O "))
    self.assertEqual("CI", input_codes)
    self.assertEqual(expected_codes, output_codes)
    self.assertEqual(["A!", "Q!", "R!"], [line for line in lines if line in {"A!", "Q!", "R!"}])
    self.assertEqual(2, lines.count("S"))
    self.assertEqual("$~", lines[-1])

  def test_complete_fixture_has_normal_and_mini_hlq_packages(self) -> None:
    complete = self.fixtures / "complete/hlq"
    self.assertEqual("100.hlq\n$\n", (complete / "index").read_text(encoding="ascii"))
    self.assertEqual(
        (complete / "index").read_bytes(),
        (complete / "index.mini").read_bytes(),
    )
    lines = self.read_lines("complete/hlq/100.hlq")
    self.assertEqual("#10000", lines[0])
    self.assertEqual(["A!", "Q!", "R!"], [line for line in lines if line in {"A!", "Q!", "R!"}])


if __name__ == "__main__":
  unittest.main()
