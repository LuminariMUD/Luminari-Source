from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import default_repo_root, load_manifest
from wtool_lib.objects import parse_object_file
from wtool_lib.spec_registry import extract_spec_names


_HEADER = "5 0 0 0 0 8193 0 0 0 0 0 0 0 0 0 0 0"
_VALUES = "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0"


def object_record(vnum: int, extensions: str = "", header: str = _HEADER) -> str:
  return (
      f"#{vnum}\nobject test~\na test object~\nA test object is here.~\n~\n"
      f"{header}\n{_VALUES}\n1 1 0 1 0\n{extensions}"
  )


class ObjectParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.repo_root = default_repo_root()
    cls.manifest = load_manifest()
    cls.spec_names = extract_spec_names(cls.repo_root)

  def parse(self, content: str):
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "100.obj"
      path.write_text(content, encoding="ascii")
      return parse_object_file(path, "100.obj", self.manifest, self.spec_names)

  def test_all_object_extensions_and_discrete_record_boundary(self) -> None:
    extensions = (
        "A\n1 2 3 4\nB\n1 5\nC\n53 10 4 10000 0 0 0 summon\n"
        "E\nkeywords~\nAn extra description.~\nG\n1\nH\n1\nI\n5\nJ\n10000\n"
        "R\nfixture-id\nK\n10 1 1 2 3\nP\nS\n1 10 25 1\nT 30001\nZ\nbank\n"
    )
    result = self.parse(object_record(20000, extensions) + object_record(20001) + "$\n")
    self.assertEqual([20000, 20001], [record.vnum for record in result.records])
    self.assertEqual({"OBJ027"}, {item.code for item in result.findings})
    self.assertEqual(14, len(extensions.splitlines()) // 2 + 1)
    self.assertEqual(30001, result.records[0].attachments[0].trigger_vnum)

  def test_unsafe_three_field_header_is_an_error(self) -> None:
    result = self.parse(object_record(20000, header="5 0 1") + "$\n")
    self.assertIn("OBJ008", {item.code for item in result.findings})

  def test_legacy_affects_use_loader_defaults_and_weapon_spell_overflow_is_diagnosed(self) -> None:
    extensions = "A\n1 2 3 4\nA\n1 2\nA\n1 2 3\n" + "".join(
        "S\n1 1 1 1\n" for _ in range(4)
    )
    result = self.parse(object_record(20000, extensions) + "$\n")
    codes = {item.code for item in result.findings}
    self.assertEqual({"OBJ028"}, codes)
    self.assertEqual(
        [(3, 4), (0, 0), (3, 0)],
        [(affect.bonus_type, affect.specific) for affect in result.records[0].affects],
    )

  def test_four_value_vector_gets_source_defaults(self) -> None:
    content = object_record(20000).replace(_VALUES, "1 2 3 4") + "$\n"
    result = self.parse(content)
    self.assertEqual([1, 2, 3, 4] + [0] * 12, result.records[0].values)

  def test_affects_and_spellbook_entries_have_independent_capacity(self) -> None:
    affect = "A\n1 2 3 0\n"
    spell = "B\n1 1\n"
    affects = affect * self.manifest["limits"]["MAX_OBJ_AFFECT"]["value"]
    spells = spell * self.manifest["limits"]["SPELLBOOK_SIZE"]["value"]
    for extensions in (affects + spells, spells + affects):
      with self.subTest(order=extensions[0]):
        result = self.parse(object_record(20000, extensions) + "$\n")
        self.assertFalse([item for item in result.findings if item.severity == "error"])
        for extra, expected_code in ((affect, "OBJ020"), (spell, "OBJ023")):
          overflow = self.parse(object_record(20000, extensions + extra) + "$\n")
          self.assertEqual(
              {expected_code}, {item.code for item in overflow.findings if item.severity == "error"}
          )


if __name__ == "__main__":
  unittest.main()
