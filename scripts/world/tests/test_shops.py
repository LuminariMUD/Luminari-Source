from __future__ import annotations

from pathlib import Path
import os
import shlex
import subprocess
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

  def test_rol_cheat_extension_attaches_to_preceding_shop(self) -> None:
    result = self.parse(modern_shop().replace("0\n28\n$~\n", "0\n28\nR 67108865~\n$~\n"))
    self.assertEqual([], result.findings)
    self.assertEqual(67108865, result.records[0].rol_cheat_restrictions)

  def test_malformed_rol_cheat_extension_is_rejected(self) -> None:
    result = self.parse(modern_shop().replace("0\n28\n$~\n", "0\n28\nR nope~\n$~\n"))
    self.assertIn("SHP021", {item.code for item in result.findings})

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


class ShopConverterTests(unittest.TestCase):
  @classmethod
  def setUpClass(cls) -> None:
    cls.build = tempfile.TemporaryDirectory(prefix="luminari-shopconv-test-")
    cls.addClassCleanup(cls.build.cleanup)
    cls.binary = Path(cls.build.name) / "shopconv"
    root = Path(__file__).resolve().parents[3]
    subprocess.run(
        shlex.split(os.environ.get("CC", "gcc"))
        + ["-std=gnu2x", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
           "-D_FORTIFY_SOURCE=3", "-I", str(root / "src")]
        + shlex.split(os.environ.get("SHOPCONV_TEST_CFLAGS", ""))
        + [str(root / "util/shopconv.c"), "-o", str(cls.binary)],
        check=True, capture_output=True, text=True,
    )

  def convert(self, text: str, name: str = "shop.shp") -> tuple[Path, subprocess.CompletedProcess]:
    temporary = tempfile.TemporaryDirectory(prefix="luminari-shopconv-input-")
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / name
    path.write_bytes(text.encode("ascii"))
    result = subprocess.run([str(self.binary), str(path)], capture_output=True, text=True)
    self.assertEqual(0, result.returncode, result.stderr)
    return path, result

  def test_empty_messages_and_unterminated_final_newline(self) -> None:
    original = "#1~\n" + "0\n" * 5 + "1.0\n1.0\n" + "0\n" * 5
    original += "~\n" * 7 + "0\n" * 9 + "$~"
    path, _ = self.convert(original)
    converted = path.read_text(encoding="ascii")
    self.assertIn("~\n" * 7, converted)
    self.assertNotIn("(null)", converted)
    self.assertTrue(converted.endswith("$~\n"))
    self.assertEqual(original, Path(str(path) + ".bak").read_text(encoding="ascii"))

  def test_crlf_terminators_preserve_multiline_messages(self) -> None:
    original = "#1~\r\n" + "0\r\n" * 5 + "1.0\r\n1.0\r\n" + "0\r\n" * 5
    original += "first\r\nsecond~ \r\n" * 7 + "0\r\n" * 9 + "$~\r\n"
    path, _ = self.convert(original)
    self.assertEqual(7, path.read_bytes().count(b"first\r\nsecond~\n"))

  def test_invalid_shop_header_restores_original(self) -> None:
    original = "#invalid~\n$~\n"
    path, result = self.convert(original)
    self.assertIn("Invalid shop header", result.stderr)
    self.assertEqual(original, path.read_text(encoding="ascii"))
    self.assertFalse(Path(str(path) + ".bak").exists())

  def test_long_filename_is_preserved_in_diagnostics(self) -> None:
    temporary = tempfile.TemporaryDirectory(prefix="luminari-shopconv-input-")
    self.addCleanup(temporary.cleanup)
    path = Path(temporary.name) / ("s" * 160 + ".shp")
    path.write_text("", encoding="ascii")
    result = subprocess.run([str(self.binary), str(path)], capture_output=True, text=True)
    self.assertEqual(1, result.returncode)
    self.assertIn("beginning of shop file " + str(path), result.stdout)


if __name__ == "__main__":
  unittest.main()
