from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.constants import load_manifest
from wtool_lib.shops import parse_shop_file


_MESSAGES = (
    "%s no item.~\n%s no item.~\n%s no buy.~\n%s no cash.~\n%s no cash.~\n"
    "%s buys for %d.~\n%s sells for %d.~\n"
)


def modern_shop() -> str:
  return (
      "LuminariMUD v3.0 Shop File~\n#10000~\n20000\n-1\n1.25\n0.80\n"
      "Weapon sword\n-1\n"
      + _MESSAGES
      + "0\n0\n10000\n0\n10000\n-1\n0\n28\n0\n28\n$~\n"
  )


class ShopParserTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.manifest = load_manifest()

  def parse(self, content: str):
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "100.shp"
      path.write_text(content, encoding="ascii")
      return parse_shop_file(path, "100.shp", self.manifest)

  def test_modern_shop_parses_variable_lists_and_item_name(self) -> None:
    result = self.parse(modern_shop())
    self.assertEqual([], result.findings)
    self.assertEqual([20000], result.records[0].product_vnums)
    self.assertEqual("sword", result.records[0].buy_types[0].keywords)

  def test_legacy_fixed_lists_are_accepted_with_warning(self) -> None:
    content = (
        "Legacy Shop File~\n#10100~\n20000\n-1\n-1\n-1\n-1\n1.0\n1.0\n"
        "5\n-1\n-1\n-1\n-1\n"
        + _MESSAGES
        + "0\n0\n10000\n0\n10000\n0\n28\n0\n28\n$~\n"
    )
    result = self.parse(content)
    self.assertEqual({"SHP005"}, {item.code for item in result.findings})

  def test_message_format_order_and_hours_are_diagnosed(self) -> None:
    broken = modern_shop().replace("%s buys for %d.~", "%d before %s.~").replace(
        "0\n28\n0\n28\n$~", "-1\n29\n0\n28\n$~"
    )
    result = self.parse(broken)
    self.assertTrue({"SHP018", "SHP019"} <= {item.code for item in result.findings})


if __name__ == "__main__":
  unittest.main()
