from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import load_manifest
from wtool_lib.triggers import parse_trigger_file


class TriggerParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.manifest = load_manifest()

  def parse(self, content: str):
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "100.trg"
      path.write_text(content, encoding="ascii")
      return parse_trigger_file(path, "100.trg", self.manifest)

  def test_all_attach_types_and_optional_numeric_argument(self) -> None:
    content = ""
    for vnum, attach in ((30000, 0), (30001, 1), (30002, 2)):
      content += f"#{vnum}\nTrigger {attach}~\n{attach} a 7\nargument~\nsay test~\n"
    result = self.parse(content + "$~\n")
    self.assertEqual([], result.findings)
    self.assertEqual([0, 1, 2], [record.attach_type for record in result.records])
    self.assertEqual([7, 7, 7], [record.numeric_argument for record in result.records])

  def test_empty_body_and_wrong_attach_flags_are_rejected(self) -> None:
    result = self.parse("#30000\nBroken~\n9 Z\n~\n~\n$~\n")
    self.assertTrue({"TRG007", "TRG009"} <= {item.code for item in result.findings})


if __name__ == "__main__":
  unittest.main()
