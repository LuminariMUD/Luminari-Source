"""Shared JSONL loading helpers for RoL pipeline artifacts."""

from __future__ import annotations

import json
from pathlib import Path
import sys
from typing import Any


def intern_candidate_strings(row: dict[str, Any]) -> dict[str, Any]:
  """Share immutable candidate text across repeated lineage evidence rows."""

  if "record_sha256" in row and "source_file_sha256" in row and "evidence" in row:
    for key, value in row.items():
      if isinstance(value, str):
        row[key] = sys.intern(value)
    if isinstance(row["evidence"], list):
      row["evidence"] = [
          sys.intern(value) if isinstance(value, str) else value for value in row["evidence"]
      ]
  return row


def load_jsonl(
    path: Path,
    error_type: type[ValueError],
    input_description: str,
    *,
    intern_candidates: bool,
) -> list[dict[str, Any]]:
  """Load object-only JSONL while retaining each caller's error contract."""

  rows: list[dict[str, Any]] = []
  object_hook = intern_candidate_strings if intern_candidates else None
  try:
    with path.open(encoding="ascii") as source:
      for line_number, line in enumerate(source, start=1):
        try:
          row = json.loads(line, object_hook=object_hook)
        except json.JSONDecodeError as error:
          raise error_type(f"invalid JSONL at {path}:{line_number}: {error}") from error
        if not isinstance(row, dict):
          raise error_type(f"JSONL row at {path}:{line_number} is not an object")
        rows.append(row)
  except OSError as error:
    raise error_type(f"cannot read {input_description} input {path}: {error}") from error
  return rows
