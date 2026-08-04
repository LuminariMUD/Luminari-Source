"""Bounded drift checks for the world-building documentation set."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import subprocess
from typing import Any

from .models import Finding, SourceSpan, ValidationResult


WORLD_DOCUMENTS = (
    "docs/world_game-data/ROOM_FLAGS.md",
    "docs/world_game-data/MOB_FLAGS.md",
    "docs/world_game-data/OEDIT_GUIDE.md",
    "docs/world_game-data/QUEST_FILE_FORMAT.md",
    "docs/world_game-data/HLQUEST_FILE_FORMAT.md",
    "docs/world_game-data/builder_manual.md",
    "docs/world_game-data/gear_guide.md",
    "docs/world_game-data/wilderness_system.md",
    "docs/world_game-data/CRAFTING_SYSTEM_NOTES.md",
    "docs/world_game-data/ZONE_FILE_FORMAT.md",
    "docs/world_game-data/SHOP_FILE_FORMAT.md",
    "docs/world_game-data/BUILDER_QUICKSTART.md",
    "docs/world/STARTER_AREA.md",
    "docs/systems/OLC_ONLINE_CREATION_SYSTEM.md",
    "docs/guides/OLC_SpecProcs.md",
    "docs/utilities/WORLD_VALIDATOR_CLI.md",
)

GENERATED_GUIDES = (
    "docs/web/guides/oedit.html",
    "docs/web/guides/mob_flags.html",
    "docs/web/guides/room_flags.html",
)


class DocumentationError(RuntimeError):
  """Raised when the documentation gate cannot run to completion."""


@dataclass(frozen=True, slots=True)
class DocumentTableSpec:
  path: str
  start_heading: str
  end_heading: str
  table_key: str
  macro_column: int
  label: str


@dataclass(frozen=True, slots=True)
class CommandSectionSpec:
  path: str
  start_heading: str
  end_heading: str
  layout: str


TABLE_SPECS = (
    DocumentTableSpec(
        "docs/world_game-data/ROOM_FLAGS.md",
        "## Complete Flag Reference",
        "## Usage Guidelines",
        "room",
        1,
        "room flags",
    ),
    DocumentTableSpec(
        "docs/world_game-data/MOB_FLAGS.md",
        "## Complete Flag Reference",
        "## Usage Guidelines",
        "mob",
        1,
        "mobile flags",
    ),
    DocumentTableSpec(
        "docs/world_game-data/OEDIT_GUIDE.md",
        "## Extra Flags Reference",
        "## Item Types Reference",
        "obj-extra",
        2,
        "object extra flags",
    ),
    DocumentTableSpec(
        "docs/world_game-data/OEDIT_GUIDE.md",
        "## Item Types Reference",
        "## Wear Flags Reference",
        "item-types",
        2,
        "item types",
    ),
    DocumentTableSpec(
        "docs/world_game-data/OEDIT_GUIDE.md",
        "## Wear Flags Reference",
        "## Object Value Reference",
        "obj-wear",
        3,
        "object wear flags",
    ),
    DocumentTableSpec(
        "docs/world_game-data/builder_manual.md",
        "#### Complete Sector Reference",
        "### Mobile Editor (MEDIT)",
        "sectors",
        2,
        "room sectors",
    ),
    DocumentTableSpec(
        "docs/world_game-data/QUEST_FILE_FORMAT.md",
        "## Quest Types Reference",
        "## Quest Flags Reference",
        "quest-types",
        1,
        "quest types",
    ),
    DocumentTableSpec(
        "docs/world_game-data/QUEST_FILE_FORMAT.md",
        "## Quest Flags Reference",
        "## Scalar and String Bounds",
        "quest",
        1,
        "quest flags",
    ),
    DocumentTableSpec(
        "docs/world_game-data/HLQUEST_FILE_FORMAT.md",
        "## Entry Types Reference",
        "## Command Types Reference",
        "hlquest-entry-types",
        1,
        "HLQ entry types",
    ),
    DocumentTableSpec(
        "docs/world_game-data/HLQUEST_FILE_FORMAT.md",
        "## Command Types Reference",
        "## Physical and Runtime Ordering",
        "hlquest-commands",
        1,
        "HLQ command types",
    ),
)

COMMAND_SECTIONS = (
    CommandSectionSpec(
        "docs/systems/OLC_ONLINE_CREATION_SYSTEM.md",
        "## Command Reference",
        "## OLC Architecture",
        "table",
    ),
    CommandSectionSpec(
        "docs/world_game-data/builder_manual.md",
        "### Basic OLC Commands",
        "### Room Editor (REDIT)",
        "bullets",
    ),
    CommandSectionSpec(
        "docs/world_game-data/OEDIT_GUIDE.md",
        "## Getting Started",
        "## Main Menu Snapshot",
        "bullets",
    ),
)

_SOURCE_PATH = re.compile(r"`(src/[^`\n]+)`")
_EXPLICIT_FUNCTION = re.compile(r"`([A-Za-z_]\w*)\(\)`")
_COMMAND_CELL = re.compile(r"^\|\s*`([a-z][a-z0-9_-]*)`\s*\|")
_COMMAND_TOKEN = re.compile(r"[a-z][a-z0-9_-]{2,}")


def _span(path: str, line: int = 1, column: int = 1) -> SourceSpan:
  return SourceSpan(path=path, line=line, column=column)


def _line_at_offset(text: str, offset: int) -> int:
  return text.count("\n", 0, offset) + 1


def _section(
    text: str,
    path: str,
    start_heading: str,
    end_heading: str,
) -> tuple[list[tuple[int, str]], list[Finding]]:
  lines = text.splitlines()
  start = next((index for index, line in enumerate(lines) if line == start_heading), None)
  if start is None:
    return [], [
        Finding(
            "DOC005",
            "error",
            f"missing required documentation section {start_heading!r}",
            _span(path),
        )
    ]
  end = next(
      (index for index in range(start + 1, len(lines)) if lines[index] == end_heading),
      None,
  )
  if end is None:
    return [], [
        Finding(
            "DOC005",
            "error",
            f"section {start_heading!r} is missing end heading {end_heading!r}",
            _span(path, start + 1),
        )
    ]
  return [(index + 1, lines[index]) for index in range(start + 1, end)], []


def _clean_cell(cell: str) -> str:
  return cell.strip().replace("`", "").replace("**", "")


def _table_findings(
    text: str,
    spec: DocumentTableSpec,
    entries: list[dict[str, Any]],
) -> list[Finding]:
  section, findings = _section(text, spec.path, spec.start_heading, spec.end_heading)
  if findings:
    return findings

  actual: dict[int, tuple[str, int]] = {}
  for line_number, line in section:
    if not line.lstrip().startswith("|"):
      continue
    cells = [_clean_cell(cell) for cell in line.strip().strip("|").split("|")]
    if not cells or not cells[0].isdigit() or len(cells) <= spec.macro_column:
      continue
    index = int(cells[0], 10)
    macro = cells[spec.macro_column]
    if index in actual:
      findings.append(
          Finding(
              "DOC005",
              "error",
              f"duplicate {spec.label} index {index}",
              _span(spec.path, line_number),
          )
      )
      continue
    actual[index] = (macro, line_number)

  expected = {int(entry["index"]): entry.get("macro") or "-" for entry in entries}
  heading_line = next(
      index
      for index, line in enumerate(text.splitlines(), start=1)
      if line == spec.start_heading
  )
  for index, macro in expected.items():
    if index not in actual:
      findings.append(
          Finding(
              "DOC005",
              "error",
              f"{spec.label} table is missing index {index} ({macro})",
              _span(spec.path, heading_line),
          )
      )
      continue
    documented_macro, line_number = actual[index]
    if documented_macro != macro:
      findings.append(
          Finding(
              "DOC005",
              "error",
              f"{spec.label} index {index} names {documented_macro!r}; source defines {macro!r}",
              _span(spec.path, line_number),
          )
      )

  for index, (_, line_number) in actual.items():
    if index not in expected:
      findings.append(
          Finding(
              "DOC005",
              "error",
              f"{spec.label} table publishes unknown index {index}",
              _span(spec.path, line_number),
          )
      )
  return findings


def _encoding_findings(path: str, data: bytes) -> tuple[str | None, list[Finding]]:
  findings: list[Finding] = []
  try:
    text = data.decode("utf-8")
  except UnicodeDecodeError as error:
    line = data[: error.start].count(b"\n") + 1
    findings.append(
        Finding("DOC002", "error", "document is not valid UTF-8", _span(path, line))
    )
    return None, findings

  for offset, character in enumerate(text):
    if ord(character) > 127:
      findings.append(
          Finding(
              "DOC003",
              "error",
              f"document contains non-ASCII character U+{ord(character):04X}",
              _span(path, _line_at_offset(text, offset)),
          )
      )
      break
  carriage = data.find(b"\r")
  if carriage >= 0:
    findings.append(
        Finding(
            "DOC004",
            "error",
            "document contains a CR or CRLF line ending; LF is required",
            _span(path, data[:carriage].count(b"\n") + 1),
        )
    )
  return text, findings


def _expand_source_path(repo_root: Path, token: str) -> list[Path]:
  candidate = Path(token)
  if candidate.is_absolute() or ".." in candidate.parts:
    return []
  if any(character in token for character in "*?["):
    return sorted(path for path in repo_root.glob(token) if path.exists())
  path = repo_root / candidate
  return [path] if path.exists() else []


def _source_reference_findings(
    repo_root: Path,
    texts: dict[str, str],
) -> list[Finding]:
  findings: list[Finding] = []
  explicit_symbol_counts = {
      "docs/world_game-data/ROOM_FLAGS.md": 0,
      "docs/world_game-data/MOB_FLAGS.md": 0,
  }
  source_cache: dict[Path, str] = {}

  for path, text in texts.items():
    for match in _SOURCE_PATH.finditer(text):
      token = match.group(1)
      if not _expand_source_path(repo_root, token):
        findings.append(
            Finding(
                "DOC006",
                "error",
                f"cited source path does not exist: {token}",
                _span(path, _line_at_offset(text, match.start(1))),
            )
        )

    for line_number, line in enumerate(text.splitlines(), start=1):
      if re.match(r"^\s*-\s+`src/", line) is None:
        continue
      path_tokens = _SOURCE_PATH.findall(line)
      symbols = _EXPLICIT_FUNCTION.findall(line)
      if not path_tokens or not symbols:
        continue
      if path in explicit_symbol_counts:
        explicit_symbol_counts[path] += len(symbols)
      source_paths = [
          source_path
          for token in path_tokens
          for source_path in _expand_source_path(repo_root, token)
          if source_path.is_file()
      ]
      bodies: list[str] = []
      for source_path in source_paths:
        if source_path not in source_cache:
          source_cache[source_path] = source_path.read_text(
              encoding="utf-8", errors="surrogateescape"
          )
        bodies.append(source_cache[source_path])
      for symbol in symbols:
        if not any(re.search(rf"\b{re.escape(symbol)}\b", body) for body in bodies):
          findings.append(
              Finding(
                  "DOC007",
                  "error",
                  f"explicit source symbol {symbol} is absent from the cited source path",
                  _span(path, line_number),
              )
          )

  for path, count in explicit_symbol_counts.items():
    if count == 0:
      findings.append(
          Finding(
              "DOC007",
              "error",
              "flag reference has no explicit source-function citations",
              _span(path),
          )
      )
  return findings


def _documented_commands(text: str, spec: CommandSectionSpec) -> tuple[list[tuple[str, int]], list[Finding]]:
  section, findings = _section(text, spec.path, spec.start_heading, spec.end_heading)
  commands: list[tuple[str, int]] = []
  for line_number, line in section:
    if spec.layout == "table":
      match = _COMMAND_CELL.match(line)
      if match is not None:
        commands.append((match.group(1), line_number))
      continue
    if not line.lstrip().startswith("-"):
      continue
    for code_span in re.findall(r"`([^`]+)`", line):
      command = code_span.split(maxsplit=1)[0]
      if _COMMAND_TOKEN.fullmatch(command) is not None:
        commands.append((command, line_number))
  if not commands and not findings:
    findings.append(
        Finding(
            "DOC008",
            "error",
            f"command reference section {spec.start_heading!r} contains no commands",
            _span(spec.path),
        )
    )
  return commands, findings


def _command_findings(repo_root: Path, texts: dict[str, str]) -> list[Finding]:
  source = repo_root / "src/interpreter.c"
  if not source.is_file():
    raise DocumentationError(f"command registry is inaccessible: {source}")
  command_source = source.read_text(encoding="utf-8", errors="surrogateescape")
  start_marker = "cpp_extern const struct command_info cmd_info[] = {"
  end_marker = "const struct mob_script_command_t mob_script_commands[] = {"
  start = command_source.find(start_marker)
  end = command_source.find(end_marker, start + len(start_marker))
  if start < 0 or end < 0:
    raise DocumentationError("cannot locate the cmd_info[] source block")
  command_table = command_source[start:end]
  registered = set(re.findall(r'\{\s*"([^"]+)"', command_table))
  findings: list[Finding] = []
  for spec in COMMAND_SECTIONS:
    text = texts.get(spec.path)
    if text is None:
      continue
    commands, section_findings = _documented_commands(text, spec)
    findings.extend(section_findings)
    for command, line_number in commands:
      if command not in registered:
        findings.append(
            Finding(
                "DOC008",
                "error",
                f"documented OLC command {command!r} is not registered in cmd_info[]",
                _span(spec.path, line_number),
            )
        )
  return findings


def _generated_guide_findings(repo_root: Path) -> list[Finding]:
  script_path = repo_root / "scripts/development/generate-web-guides.sh"
  if not script_path.is_file():
    raise DocumentationError(f"generated guide checker is inaccessible: {script_path}")
  completed = subprocess.run(
      [str(script_path), "--check"],
      cwd=repo_root,
      check=False,
      capture_output=True,
      text=True,
  )
  if completed.returncode == 0:
    return []
  output = " ".join((completed.stdout + completed.stderr).split())
  if completed.returncode == 1:
    return [
        Finding(
            "DOC009",
            "error",
            f"generated builder guides are stale: {output}",
            _span("scripts/development/generate-web-guides.sh"),
        )
    ]
  raise DocumentationError(
      f"generated guide checker failed with status {completed.returncode}: {output}"
  )


def validate_docs(
    repo_root: Path,
    manifest: dict[str, Any],
    *,
    check_generated: bool = True,
) -> ValidationResult:
  """Validate the explicitly selected world-documentation contract."""

  findings: list[Finding] = []
  texts: dict[str, str] = {}
  complete = True
  for path in WORLD_DOCUMENTS:
    full_path = repo_root / path
    if not full_path.is_file():
      findings.append(
          Finding("DOC001", "error", "required documentation file is missing", _span(path))
      )
      complete = False
      continue
    text, encoding_findings = _encoding_findings(path, full_path.read_bytes())
    findings.extend(encoding_findings)
    if text is None:
      complete = False
      continue
    texts[path] = text

  tables = manifest.get("tables", {})
  for spec in TABLE_SPECS:
    text = texts.get(spec.path)
    table = tables.get(spec.table_key)
    if text is None:
      continue
    if not isinstance(table, dict) or not isinstance(table.get("entries"), list):
      raise DocumentationError(f"constants manifest has no table {spec.table_key!r}")
    findings.extend(_table_findings(text, spec, table["entries"]))

  findings.extend(_source_reference_findings(repo_root, texts))
  findings.extend(_command_findings(repo_root, texts))
  if check_generated:
    findings.extend(_generated_guide_findings(repo_root))

  return ValidationResult(
      root_label=".",
      mode="docs",
      findings=findings,
      complete=complete,
      config={
          "documents": len(WORLD_DOCUMENTS),
          "generated_guides": len(GENERATED_GUIDES),
          "generated_check": check_generated,
      },
  )
