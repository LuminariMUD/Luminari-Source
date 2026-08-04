from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from wtool_lib.source import (
    SourceCursor,
    SourceFile,
    parse_c_integer_prefix,
    parse_c_integer_token,
)


class SourceCursorTests(unittest.TestCase):
  def source(self, data: bytes) -> SourceFile:
    return SourceFile(Path("fixture.wld"), "fixture.wld", data)

  def test_significant_lines_match_get_line_column_rules(self) -> None:
    cursor = SourceCursor(self.source(b"\r\n* comment\r\n  * data\r\nvalue\r\n"))
    first = cursor.read_significant()
    second = cursor.read_significant()
    self.assertIsNotNone(first)
    self.assertIsNotNone(second)
    self.assertEqual("  * data", first.text)
    self.assertEqual(3, first.number)
    self.assertEqual("value", second.text)
    self.assertEqual(4, second.number)

  def test_tilde_string_tracks_lines_and_normalizes_crlf(self) -> None:
    cursor = SourceCursor(self.source(b"first\nsecond~\r\nnext\n"))
    value = cursor.read_tilde_string()
    self.assertTrue(value.terminated)
    self.assertEqual("first\r\nsecond", value.text)
    self.assertEqual(1, value.span.line)
    self.assertEqual(2, value.span.end_line)
    self.assertEqual("next", cursor.read_raw().text)

  def test_unterminated_tilde_string_is_incomplete_without_decode_loss(self) -> None:
    cursor = SourceCursor(self.source(b"legacy \xff bytes\n"))
    value = cursor.read_tilde_string()
    self.assertFalse(value.terminated)
    self.assertIn("\udcff", value.text)
    self.assertEqual("SRC002", value.issues[-1].code)
    self.assertTrue(value.issues[-1].fatal)

  def test_long_physical_line_is_reported(self) -> None:
    cursor = SourceCursor(self.source(b"x" * 511 + b"~\n"))
    value = cursor.read_tilde_string()
    self.assertIn("SRC001", {issue.code for issue in value.issues})

  def test_from_path_preserves_bytes(self) -> None:
    with tempfile.TemporaryDirectory() as directory:
      path = Path(directory) / "data.mob"
      path.write_bytes(b"name \xff~\r\n")
      source = SourceFile.from_path(path, "mob/data.mob")
      self.assertEqual(b"name \xff", source.lines[0].raw[:-1])


class IntegerParsingTests(unittest.TestCase):
  def test_scanf_prefix_is_retained(self) -> None:
    parsed = parse_c_integer_prefix("  -12 annotation")
    self.assertEqual(-12, parsed.value)
    self.assertEqual(" annotation", "  -12 annotation"[parsed.consumed :])

  def test_signed_overflow_is_rejected(self) -> None:
    parsed = parse_c_integer_token("2147483648")
    self.assertIsNone(parsed.value)
    self.assertIn("signed 32-bit", parsed.error)

  def test_unsigned_range_is_enforced(self) -> None:
    self.assertIsNotNone(parse_c_integer_token("4294967295", signed=False).value)
    self.assertIsNone(parse_c_integer_token("-1", signed=False).value)

  def test_token_rejects_unconsumed_suffix(self) -> None:
    self.assertEqual(
        "unexpected characters after integer",
        parse_c_integer_token("12x").error,
    )


if __name__ == "__main__":
  unittest.main()
