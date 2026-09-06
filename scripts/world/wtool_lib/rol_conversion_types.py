"""Shared RoL conversion records, diagnostics, and results."""

from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from typing import Any, Callable



_COLOR = re.compile(r"&\+[A-Za-z]|&[Nn]|@[A-Za-z]")


@dataclass(frozen=True, slots=True)
class RolReference:
  target_type: str
  target_vnum: int
  role: str
  path: str
  line: int

  def to_dict(self) -> dict[str, Any]:
    return {
        "target_type": self.target_type,
        "target_vnum": self.target_vnum,
        "role": self.role,
        "path": self.path,
        "line": self.line,
    }


@dataclass(frozen=True, slots=True)
class RolDiagnostic:
  code: str
  severity: str
  message: str
  path: str
  line: int
  record_type: str | None = None
  record_vnum: int | None = None

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "code": self.code,
        "severity": self.severity,
        "message": self.message,
        "path": self.path,
        "line": self.line,
    }
    if self.record_type is not None:
      data["record_type"] = self.record_type
    if self.record_vnum is not None:
      data["record_vnum"] = self.record_vnum
    return data


@dataclass(slots=True)
class RolRecord:
  kind: str
  vnum: int
  basename: str
  path: str
  line: int
  end_line: int
  sha256: str
  identity: str | None = None
  format_version: str | int | None = None
  values: dict[str, Any] = field(default_factory=dict)
  directives: list[dict[str, Any]] = field(default_factory=list)
  references: list[RolReference] = field(default_factory=list)
  complete: bool = True

  @property
  def record_id(self) -> str:
    return f"{self.kind}:{self.vnum}:{self.path}:{self.line}"

  def to_dict(self) -> dict[str, Any]:
    data: dict[str, Any] = {
        "record_id": self.record_id,
        "kind": self.kind,
        "vnum": self.vnum,
        "basename": self.basename,
        "path": self.path,
        "line": self.line,
        "end_line": self.end_line,
        "sha256": self.sha256,
        "complete": self.complete,
        "directives": self.directives,
        "references": [reference.to_dict() for reference in self.references],
    }
    if self.identity is not None:
      data["identity"] = self.identity
      data["normalized_identity"] = normalize_identity(self.identity)
    if self.format_version is not None:
      data["format_version"] = self.format_version
    if self.values:
      data["values"] = self.values
    return data


@dataclass(slots=True)
class RolSourceCorpus:
  records: list[RolRecord] = field(default_factory=list)
  diagnostics: list[RolDiagnostic] = field(default_factory=list)
  file_versions: Counter[tuple[str, str]] = field(default_factory=Counter)
  file_terminators: Counter[tuple[str, str]] = field(default_factory=Counter)

  @property
  def complete(self) -> bool:
    return all(record.complete for record in self.records) and not any(
        diagnostic.severity == "error" for diagnostic in self.diagnostics
    )


def normalize_identity(value: str) -> str:
  value = _COLOR.sub(" ", value)
  value = value.replace("\r", " ").replace("\n", " ")
  value = re.sub(r"[^A-Za-z0-9]+", " ", value).strip().lower()
  return " ".join(value.split())


IdentityResolver = Callable[[str, int], int]


@dataclass(slots=True)
class TransformResult:
  """One emitted target record plus explicit bounded-loss diagnostics."""

  text: str
  diagnostics: list[str] = field(default_factory=list)
  ledger: dict[str, Any] | None = None
