"""World index selection and validation."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
import re

from .models import Finding, IndexRecord, SourceSpan, ValidationResult


DATA_EXTENSIONS = ("zon", "wld", "mob", "obj", "shp", "trg")
REQUIRED_FULL_DATASETS = frozenset({"zon", "wld", "mob", "obj"})
_SAFE_COMPONENT = re.compile(r"^[A-Za-z0-9._-]+$")
_NUMERIC_PACKAGE = re.compile(r"^(\d+)\.([A-Za-z0-9]+)$")
_RECORD_HEADER = re.compile(br"(?m)^#-?\d+")


@dataclass(slots=True)
class ParsedIndex:
  extension: str
  path: Path
  display_path: str
  records: list[IndexRecord] = field(default_factory=list)
  terminated: bool = False
  complete: bool = True


def is_safe_path_component(name: str) -> bool:
  """Mirror src/utils.c:is_safe_path_component() for ASCII world filenames."""

  return bool(
      name
      and name not in {".", ".."}
      and ".." not in name
      and _SAFE_COMPONENT.fullmatch(name)
  )


def normalized_root_label(root: Path, repo_root: Path) -> str:
  resolved = root.resolve()
  try:
    return resolved.relative_to(repo_root.resolve()).as_posix()
  except ValueError:
    return f"external:{resolved.name}"


def _finding(
    code: str,
    severity: str,
    message: str,
    path: str,
    line: int = 1,
) -> Finding:
  return Finding(code, severity, message, SourceSpan(path, line))


def _index_tokens(data: bytes) -> list[tuple[str, int]]:
  tokens: list[tuple[str, int]] = []
  for line_number, line in enumerate(data.splitlines(), start=1):
    for raw_token in re.findall(br"\S+", line):
      tokens.append((raw_token.decode("utf-8", errors="surrogateescape"), line_number))
  return tokens


def _parse_index(
    index_path: Path,
    display_path: str,
    extension: str,
    findings: list[Finding],
) -> ParsedIndex:
  parsed = ParsedIndex(extension, index_path, display_path)
  if not index_path.is_file():
    findings.append(
        _finding("IDX009", "error", f"required index file is missing for .{extension} data", display_path)
    )
    parsed.complete = False
    return parsed

  try:
    tokens = _index_tokens(index_path.read_bytes())
  except OSError as error:
    findings.append(_finding("IDX010", "error", f"cannot read index: {error}", display_path))
    parsed.complete = False
    return parsed

  seen: dict[str, int] = {}
  previous_numeric: int | None = None
  for token, line_number in tokens:
    if token.startswith("$"):
      parsed.terminated = token == "$"
      if token != "$":
        findings.append(
            _finding(
                "IDX011",
                "warning",
                f"non-conventional index terminator {token!r}; use '$'",
                display_path,
                line_number,
            )
        )
      break

    safe = is_safe_path_component(token)
    if not safe:
      findings.append(
          _finding("IDX001", "error", f"unsafe data filename {token!r}", display_path, line_number)
      )

    suffix = Path(token).suffix
    if suffix != f".{extension}":
      findings.append(
          _finding(
              "IDX002",
              "error",
              f"index entry {token!r} must use the .{extension} extension",
              display_path,
              line_number,
          )
      )

    if token in seen:
      findings.append(
          _finding(
              "IDX003",
              "error",
              f"duplicate index entry {token!r}; first listed on line {seen[token]}",
              display_path,
              line_number,
          )
      )
    else:
      seen[token] = line_number

    numeric_match = _NUMERIC_PACKAGE.fullmatch(token)
    if numeric_match is not None and numeric_match.group(2) == extension:
      number = int(numeric_match.group(1))
      if previous_numeric is not None and number < previous_numeric:
        findings.append(
            _finding(
                "IDX006",
                "warning",
                f"numeric package {number} moves backward after {previous_numeric}",
                display_path,
                line_number,
            )
        )
      previous_numeric = number

    data_path = index_path.parent / token if safe else index_path.parent / "__unsafe__"
    exists = safe and data_path.is_file()
    if safe and not exists:
      findings.append(
          _finding(
              "IDX004",
              "error",
              f"listed data file {token!r} is missing",
              display_path,
              line_number,
          )
      )
      parsed.complete = False
    parsed.records.append(
        IndexRecord(token, extension, SourceSpan(display_path, line_number), exists)
    )

  if not parsed.terminated:
    findings.append(
        _finding("IDX005", "error", "index is missing the conventional '$' terminator", display_path)
    )
    parsed.complete = False
  return parsed


def _dataset_paths(world_root: Path, extension: str, mini: bool) -> tuple[Path, str]:
  directory = world_root / extension
  name = "index.mini" if mini else "index"
  return directory / name, f"{extension}/{name}"


def validate_indexes(world_root: Path, repo_root: Path, mini: bool = False) -> ValidationResult:
  mode = "mini" if mini else "all"
  result = ValidationResult(normalized_root_label(world_root, repo_root), mode)
  parsed_indexes: list[ParsedIndex] = []

  for extension in DATA_EXTENSIONS:
    index_path, display_path = _dataset_paths(world_root, extension, mini)
    parsed = _parse_index(index_path, display_path, extension, result.findings)
    parsed_indexes.append(parsed)
    if not parsed.complete:
      result.complete = False

    record_count = 0
    for record in parsed.records:
      if not record.exists or not is_safe_path_component(record.name):
        continue
      try:
        data = (index_path.parent / record.name).read_bytes()
      except OSError:
        continue
      record_count += len(_RECORD_HEADER.findall(data))
    if not mini and extension in REQUIRED_FULL_DATASETS and record_count == 0:
      result.findings.append(
          _finding(
              "IDX007",
              "error",
              f"full boot requires a non-empty .{extension} dataset",
              display_path,
          )
      )

    if mini or not index_path.parent.is_dir():
      continue
    indexed = {record.name for record in parsed.records}
    conventional = sorted(
        path.name
        for path in index_path.parent.iterdir()
        if path.is_file() and re.fullmatch(rf"\d+\.{re.escape(extension)}", path.name)
    )
    for name in conventional:
      if name not in indexed:
        result.findings.append(
            _finding(
                "IDX008",
                "warning",
                f"conventional data file {name!r} is not listed in the normal index",
                f"{extension}/{name}",
            )
        )

  result.config = {"index_mode": mode}
  return result
