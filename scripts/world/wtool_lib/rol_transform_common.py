"""Shared target text and bit encoding."""

from __future__ import annotations

import re

from .flags import encode_bits
from .rol_conversion_types import RolRecord


TARGET_MAX_LEVEL = 34


_SOURCE_COLOR = re.compile(r"&&|&[Nn]|&[+-].?|&=.{0,2}|&.?", re.DOTALL)
_SOURCE_BACKGROUND_RGB = {
    "l": "000", "r": "200", "g": "020", "y": "220",
    "b": "002", "m": "202", "c": "022", "w": "222",
}


def convert_text(value: str | None) -> tuple[str, list[str]]:
  """Convert legacy colors and return canonical ASCII/LF target text."""

  diagnostics: list[str] = []
  text = value or ""

  def color(match: re.Match[str]) -> str:
    escape = match.group()
    if escape == "&&":
      return "&"
    if escape in {"&N", "&n"}:
      return "@n"
    mode = escape[1:2]
    expected_length = 4 if mode == "=" else 3 if mode in {"+", "-"} else 2
    if len(escape) < expected_length:
      diagnostics.append(f"dropped incomplete source color escape {escape!r}")
      return ""
    colors = escape[2:]
    if mode not in {"+", "-", "="} or any(
        value.lower() not in _SOURCE_BACKGROUND_RGB for value in colors
    ):
      if not escape[1].isspace():
        diagnostics.append(f"preserved unknown source color escape {escape!r} as literal text")
      return escape
    foreground = colors[-1] if mode in {"+", "="} else ""
    background = colors[0] if mode in {"-", "="} else ""
    # Source black is L/l; target L/l is lime and D/d is black/bright black.
    foreground = {"L": "D", "l": "d"}.get(foreground, foreground)
    result = f"@[b{_SOURCE_BACKGROUND_RGB[background.lower()]}]" if background else ""
    if background and background.isupper():
      result += "@-"
    return result + (f"@{foreground}" if foreground else "")

  # The target reader runs parse_at() over every tilde string, so a bare '@'
  # becomes a color introducer and swallows the next character. Escape source
  # at-signs to '@@' before introducing our own '@' color codes below.
  if "@" in text:
    text = text.replace("@", "@@")
    diagnostics.append("escaped literal '@' as '@@' for the target color parser")
  text = _SOURCE_COLOR.sub(color, text)
  text = text.replace("\r\n", "\n").replace("\r", "\n")
  if "~" in text:
    text = text.replace("~", "-")
    diagnostics.append("embedded tilde replaced with '-' to preserve target framing")
  encoded = text.encode("ascii", errors="replace")
  if encoded.decode("ascii") != text:
    diagnostics.append("non-ASCII source bytes replaced with '?' for target compatibility")
  return encoded.decode("ascii"), diagnostics


def _tilde(value: str | None) -> tuple[str, list[str]]:
  text, diagnostics = convert_text(value)
  return f"{text}~\n", diagnostics


def _source_mask_bits(mask: int, logical_offset: int) -> set[int]:
  return {
      logical_offset + bit
      for bit in range(32)
      if mask & (1 << bit)
  }


def _mapped_bits(source_bits: set[int], mapping: dict[int, int]) -> set[int]:
  return {mapping[bit] for bit in source_bits if bit in mapping}


def _encoded(bits: set[int]) -> str:
  return " ".join(encode_bits(bits))


def _directive_rows(record: RolRecord, token: str) -> list[dict[str, object]]:
  return [directive for directive in record.directives if directive["token"] == token]
