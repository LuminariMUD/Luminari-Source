"""Byte-preserving source input with CircleMUD-compatible cursor operations."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import re

from .models import SourceSpan


READ_SIZE = 512
MAX_STRING_LENGTH = 49152


@dataclass(frozen=True, slots=True)
class SourceLine:
  number: int
  text: str
  raw: bytes
  newline: bytes
  span: SourceSpan

  @property
  def byte_length(self) -> int:
    return len(self.raw)


@dataclass(frozen=True, slots=True)
class SourceIssue:
  code: str
  message: str
  span: SourceSpan
  fatal: bool = False


@dataclass(frozen=True, slots=True)
class TildeString:
  text: str
  span: SourceSpan
  terminated: bool
  issues: tuple[SourceIssue, ...] = ()


@dataclass(frozen=True, slots=True)
class ParsedInteger:
  value: int | None
  consumed: int
  error: str | None = None


@dataclass(slots=True)
class SourceFile:
  path: Path
  display_path: str
  data: bytes
  lines: list[SourceLine] = field(init=False)

  def __post_init__(self) -> None:
    self.lines = []
    for number, raw_line in enumerate(self.data.splitlines(keepends=True), start=1):
      content, newline = _split_newline(raw_line)
      text = content.decode("utf-8", errors="surrogateescape")
      self.lines.append(
          SourceLine(
              number=number,
              text=text,
              raw=content,
              newline=newline,
              span=SourceSpan(self.display_path, number),
          )
      )

  @classmethod
  def from_path(cls, path: Path, display_path: str | None = None) -> "SourceFile":
    return cls(path=path, display_path=display_path or path.as_posix(), data=path.read_bytes())


def _split_newline(raw_line: bytes) -> tuple[bytes, bytes]:
  if raw_line.endswith(b"\r\n"):
    return raw_line[:-2], b"\r\n"
  if raw_line.endswith(b"\n") or raw_line.endswith(b"\r"):
    return raw_line[:-1], raw_line[-1:]
  return raw_line, b""


class SourceCursor:
  """Cursor exposing raw lines and the source server's get_line behavior."""

  def __init__(self, source: SourceFile):
    self.source = source
    self.position = 0

  @property
  def eof(self) -> bool:
    return self.position >= len(self.source.lines)

  def peek_raw(self) -> SourceLine | None:
    if self.eof:
      return None
    return self.source.lines[self.position]

  def read_raw(self) -> SourceLine | None:
    line = self.peek_raw()
    if line is not None:
      self.position += 1
    return line

  def read_significant(self) -> SourceLine | None:
    """Match get_line(): skip only empty physical lines and column-zero '*'."""

    while not self.eof:
      line = self.read_raw()
      if line is None:
        return None
      if line.raw == b"" or line.raw.startswith(b"*"):
        continue
      return line
    return None

  def read_tilde_string(self, max_length: int = MAX_STRING_LENGTH) -> TildeString:
    start = self.peek_raw()
    if start is None:
      span = SourceSpan(self.source.display_path, max(1, len(self.source.lines)))
      issue = SourceIssue("SRC002", "expected a tilde-terminated string at end of file", span, True)
      return TildeString("", span, False, (issue,))

    pieces: list[bytes] = []
    issues: list[SourceIssue] = []
    total = 0
    end_line = start.number
    terminated = False

    while not self.eof:
      line = self.read_raw()
      if line is None:
        break
      end_line = line.number
      if len(line.raw) > READ_SIZE - 2:
        issues.append(
            SourceIssue(
                "SRC001",
                f"physical line is {len(line.raw)} bytes; the source reader safely accepts at most "
                f"{READ_SIZE - 2} before a newline",
                line.span,
                True,
            )
        )

      if line.raw.endswith(b"~"):
        piece = line.raw[:-1]
        terminated = True
      else:
        piece = line.raw + b"\r\n"
      pieces.append(piece)
      total += len(piece)

      if total >= max_length:
        issues.append(
            SourceIssue(
                "SRC003",
                f"tilde string reaches {total} bytes; maximum stored length is {max_length - 1}",
                SourceSpan(self.source.display_path, start.number, end_line=end_line),
                True,
            )
        )
      if terminated:
        break

    span = SourceSpan(self.source.display_path, start.number, end_line=end_line)
    if not terminated:
      issues.append(SourceIssue("SRC002", "unterminated tilde string", span, True))

    value = b"".join(pieces).decode("utf-8", errors="surrogateescape")
    return TildeString(value, span, terminated, tuple(issues))


_INTEGER_PREFIX = re.compile(r"^[ \t\v\f]*([+-]?\d+)")


def parse_c_integer_prefix(text: str, bits: int = 32, signed: bool = True) -> ParsedInteger:
  """Parse the decimal prefix accepted by scanf and enforce the C target range."""

  match = _INTEGER_PREFIX.match(text)
  if match is None:
    return ParsedInteger(None, 0, "expected a decimal integer")

  token = match.group(1)
  value = int(token, 10)
  if signed:
    minimum = -(1 << (bits - 1))
    maximum = (1 << (bits - 1)) - 1
  else:
    minimum = 0
    maximum = (1 << bits) - 1
  if value < minimum or value > maximum:
    kind = "signed" if signed else "unsigned"
    return ParsedInteger(
        None,
        match.end(),
        f"integer {value} is outside the {kind} {bits}-bit range {minimum}..{maximum}",
    )
  return ParsedInteger(value, match.end())


def parse_c_integer_token(token: str, bits: int = 32, signed: bool = True) -> ParsedInteger:
  parsed = parse_c_integer_prefix(token, bits=bits, signed=signed)
  if parsed.error is None and token[parsed.consumed :].strip():
    return ParsedInteger(None, parsed.consumed, "unexpected characters after integer")
  return parsed
