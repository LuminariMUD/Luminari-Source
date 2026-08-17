from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.mobiles import parse_mobile_file
from wtool_lib.spec_registry import extract_spec_names


def mobile_record(header: str = "0 0 0 0 0 0 0 0 0 E", enhanced: str = "") -> str:
  return (
      "#10000\nmobile test~\na test mobile~\nA test mobile is here.~\nA test mobile.~\n"
      f"{header}\n1 20 19 1d8+0 1d4+0\n0 0\n9 9 0\n{enhanced}"
      "E\n$\n"
  )


class MobileParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.spec_names = extract_spec_names(cls.repo_root)

  def parse(self, content: str):
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "100.mob"
      path.write_text(content, encoding="ascii")
      return parse_mobile_file(path, "100.mob", self.manifest, self.spec_names)

  def test_modern_enhanced_record_parses_keywords_path_and_attachment(self) -> None:
    result = self.parse(
        mobile_record(
            enhanced=(
                "Str: 20\nKnownSpell: 1\nRace: 1\nSubRace 1: 1\nClass: 1\n"
                "Size: 5\nTier: 3\nMFeat: 1 1\nFeat: 2 1\nAff2: 0 0 0 0\n"
                "SpecProc: bank\nPath: 5:10000 10001\n"
            )
        ).replace("E\n$\n", "E\nT 30000\n$\n")
    )
    self.assertEqual([], result.findings)
    self.assertEqual([10000, 10001], result.records[0].path_rooms)
    self.assertEqual(30000, result.records[0].attachments[0].trigger_vnum)

  def test_legacy_header_uses_shifted_affect_compatibility(self) -> None:
    content = mobile_record(header="0 a 0 S", enhanced="").replace("E\n$\n", "$\n")
    result = self.parse(content)
    self.assertEqual({"MOB007"}, {item.code for item in result.findings})

  def test_clamps_reserved_flags_and_malformed_path_are_all_diagnosed(self) -> None:
    result = self.parse(
        mobile_record(
            header="1048576 0 0 0 0 0 0 0 0 E",
            enhanced="Str: 99\nUnknownThing: 1\nMFeat: bad\nAff2: 1 2\nPath: nope\n",
        )
    )
    codes = {item.code for item in result.findings}
    self.assertTrue({"MOB011", "MOB019", "MOB021", "MOB022", "MOB024", "MOB026"} <= codes)

  def test_path_array_overflow_is_rejected(self) -> None:
    rooms = " ".join(str(10000 + index) for index in range(51))
    result = self.parse(mobile_record(enhanced=f"Path: 1:{rooms}\n"))
    self.assertIn("MOB025", {item.code for item in result.findings})

  def test_tier_rejects_out_of_range_and_non_integer_values(self) -> None:
    result = self.parse(mobile_record(enhanced="Tier: 6\nTier: 2junk\n"))
    self.assertEqual(["MOB028", "MOB028"], [item.code for item in result.findings])

  def test_spell_resistance_is_canonical_and_range_checked(self) -> None:
    valid = self.parse(mobile_record(enhanced="SpellRes: 47\n"))
    self.assertEqual([], valid.findings)
    self.assertEqual("47", valid.records[0].enhanced["SpellRes"][0])

    invalid = self.parse(mobile_record(enhanced="SpellRes: 101\n"))
    self.assertEqual(["MOB026"], [item.code for item in invalid.findings])


if __name__ == "__main__":
  unittest.main()
