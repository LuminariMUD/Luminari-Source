"""Four-chunk flat-file flag encoding and lookup."""

from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Any

from .constants import CHUNK_WIDTH, DECODER_ALPHABET, SERIALIZED_CHUNKS, WRITER_ALPHABET


FLAG_SETS = (
    "room",
    "zone",
    "mob",
    "affect",
    "affect2",
    "obj-extra",
    "obj-wear",
    "obj-affect",
    "obj-affect2",
)


@dataclass(frozen=True, slots=True)
class FlagIssue:
  code: str
  message: str
  token_index: int | None = None


@dataclass(frozen=True, slots=True)
class DecodedFlags:
  bits: frozenset[int]
  chunks: tuple[int, int, int, int]
  issues: tuple[FlagIssue, ...]


def resolve_set(manifest: dict[str, Any], set_name: str) -> tuple[str, dict[str, Any]]:
  aliases = manifest.get("flag_set_aliases", {})
  canonical = aliases.get(set_name, set_name)
  table = manifest.get("tables", {}).get(canonical)
  if table is None or set_name not in FLAG_SETS:
    raise ValueError(f"unknown flag set {set_name!r}")
  return canonical, table


def _decode_alpha(token: str, token_index: int) -> tuple[int, list[FlagIssue]]:
  mask = 0
  issues: list[FlagIssue] = []
  for character in token:
    position = DECODER_ALPHABET.find(character)
    if position < 0:
      issues.append(
          FlagIssue("FLG001", f"invalid flag character {character!r}", token_index)
      )
      continue
    if position >= CHUNK_WIDTH:
      issues.append(
          FlagIssue(
              "FLG002",
              f"flag character {character!r} addresses local bit {position}, above the "
              f"logical chunk maximum {CHUNK_WIDTH - 1}",
              token_index,
          )
      )
      continue
    mask |= 1 << position
  return mask, issues


def decode_tokens(tokens: list[str], entry_count: int | None = None) -> DecodedFlags:
  issues: list[FlagIssue] = []
  if len(tokens) > SERIALIZED_CHUNKS:
    issues.append(
        FlagIssue(
            "FLG003",
            f"received {len(tokens)} flag chunks; only {SERIALIZED_CHUNKS} are serialized",
        )
    )
  chunks = [0, 0, 0, 0]
  for token_index, token in enumerate(tokens[:SERIALIZED_CHUNKS]):
    if re.fullmatch(r"[+-]?\d+", token):
      value = int(token, 10)
      if value < 0:
        issues.append(FlagIssue("FLG004", "negative numeric flag masks are invalid", token_index))
        continue
      if value >= 1 << CHUNK_WIDTH:
        issues.append(
            FlagIssue(
                "FLG005",
                f"numeric flag mask {value} exceeds {CHUNK_WIDTH} logical bits",
                token_index,
            )
        )
        value &= (1 << CHUNK_WIDTH) - 1
      chunks[token_index] = value
    else:
      chunks[token_index], token_issues = _decode_alpha(token, token_index)
      issues.extend(token_issues)

  bits = {
      chunk_index * CHUNK_WIDTH + local_bit
      for chunk_index, mask in enumerate(chunks)
      for local_bit in range(CHUNK_WIDTH)
      if mask & (1 << local_bit)
  }
  if entry_count is not None:
    for bit in sorted(bits):
      if bit >= entry_count:
        issues.append(
            FlagIssue(
                "FLG006",
                f"global bit {bit} is outside the source table range 0..{entry_count - 1}",
                bit // CHUNK_WIDTH,
            )
        )
  return DecodedFlags(frozenset(bits), tuple(chunks), tuple(issues))


def encode_bits(bits: set[int] | frozenset[int]) -> tuple[str, str, str, str]:
  chunks = [0, 0, 0, 0]
  for bit in bits:
    if bit < 0 or bit >= CHUNK_WIDTH * SERIALIZED_CHUNKS:
      raise ValueError(f"flag bit {bit} is outside the four serialized chunks")
    chunks[bit // CHUNK_WIDTH] |= 1 << (bit % CHUNK_WIDTH)
  encoded: list[str] = []
  for mask in chunks:
    if mask == 0:
      encoded.append("0")
      continue
    encoded.append(
        "".join(WRITER_ALPHABET[bit] for bit in range(CHUNK_WIDTH) if mask & (1 << bit))
    )
  return tuple(encoded)  # type: ignore[return-value]


def _normalized_name(name: str) -> str:
  return re.sub(r"[^a-z0-9]", "", name.casefold())


def resolve_names(table: dict[str, Any], names: list[str]) -> set[int]:
  candidates: dict[str, set[int]] = {}
  for entry in table["entries"]:
    forms = [entry["name"]]
    if entry.get("macro"):
      forms.append(entry["macro"])
    forms.extend(entry.get("aliases", []))
    for form in forms:
      candidates.setdefault(_normalized_name(form), set()).add(entry["index"])

  resolved: set[int] = set()
  for name in names:
    matches = candidates.get(_normalized_name(name), set())
    if not matches:
      raise ValueError(f"unknown flag name {name!r}")
    if len(matches) > 1:
      choices = ", ".join(str(index) for index in sorted(matches))
      raise ValueError(f"ambiguous flag name {name!r}; matches indices {choices}")
    resolved.update(matches)
  return resolved


def decoded_entries(table: dict[str, Any], decoded: DecodedFlags) -> list[dict[str, Any]]:
  by_index = {entry["index"]: entry for entry in table["entries"]}
  return [by_index[index] for index in sorted(decoded.bits) if index in by_index]
