"""Shared target text and bit encoding."""

from __future__ import annotations

import re

from .flags import encode_bits
from .rol_conversion_types import RolRecord


_TARGET_MAX_LEVEL = 34


_SOURCE_COLOR = re.compile(r"&\+([A-Za-z])|&([Nn])")


def convert_text(value: str | None) -> tuple[str, list[str]]:
  """Convert legacy colors and return canonical ASCII/LF target text."""

  diagnostics: list[str] = []
  text = value or ""
  # The target reader runs parse_at() over every tilde string, so a bare '@'
  # becomes a color introducer and swallows the next character. Escape source
  # at-signs to '@@' before introducing our own '@' color codes below.
  if "@" in text:
    text = text.replace("@", "@@")
    diagnostics.append("escaped literal '@' as '@@' for the target color parser")
  text = _SOURCE_COLOR.sub(
      lambda match: "@n" if match.group(2) else f"@{match.group(1)}",
      text,
  )
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
