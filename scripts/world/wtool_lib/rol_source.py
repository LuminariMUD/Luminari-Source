"""Grammar-aware source dispatch and compatibility exports."""

from __future__ import annotations

from collections import Counter
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from .models import TOOL_VERSION
from .rol_conversion_types import (
    RolDiagnostic,
    RolRecord,
    RolReference,
    RolSourceCorpus,
    _COLOR,
    normalize_identity,
)
from .rol_inventory import SOURCE_KINDS, build_rol_inventory
from .rol_mobiles import (
    _MOBILE_REPAIR_POLICY,
    _mobile_repair_owns,
    _mobile_repair_policy,
    _mobile_row_diagnostic,
    _parse_mob,
)
from .rol_objects import _parse_obj
from .rol_quests import _parse_qst
from .rol_rooms import _parse_wld
from .rol_shops import _parse_shp
from .rol_soc import _parse_soc
from .rol_source_common import (
    _HEADER,
    _INTEGER,
    _collect_numeric_lines,
    _diagnostic,
    _exclude_record,
    _integers,
    _line_bytes,
    _new_record,
    _next_content,
    _numeric_line,
    _read_tilde,
    _reference,
    _segment_hash,
    _segments,
)
from .rol_zones import _parse_zon
from .source import SourceFile


ROL_SOURCE_SCHEMA_VERSION = 1


_PARSERS = {
    "wld": _parse_wld,
    "mob": _parse_mob,
    "obj": _parse_obj,
    "zon": _parse_zon,
    "qst": _parse_qst,
    "shp": _parse_shp,
    "soc": _parse_soc,
}


def parse_rol_source_file(
    path: Path,
    display_path: str,
    kind: str,
    basename: str,
    corpus: RolSourceCorpus | None = None,
) -> tuple[list[RolRecord], RolSourceCorpus]:
  """Parse one physical or assembled source file with the selected grammar."""

  if kind not in _PARSERS:
    raise ValueError(f"unknown RoL source kind {kind!r}")
  selected_corpus = corpus if corpus is not None else RolSourceCorpus()
  source = SourceFile.from_path(path, display_path)
  return _PARSERS[kind](source, basename, selected_corpus), selected_corpus


def parse_active_rol_corpus(source_root: Path, repo_root: Path) -> RolSourceCorpus:
  """Parse every active physical source input into normalized typed records."""

  source_root = source_root.resolve()
  inventory = build_rol_inventory(source_root, repo_root)
  corpus = RolSourceCorpus()
  active_files = sorted(
      (record for record in inventory["files"] if record["included"]),
      key=lambda record: (SOURCE_KINDS.index(record["kind"]), record["path"]),
  )
  for file_record in active_files:
    kind = file_record["kind"]
    records, _ = parse_rol_source_file(
        source_root / file_record["path"],
        file_record["path"],
        kind,
        file_record["basename"],
        corpus,
    )
    corpus.records.extend(records)
  return corpus


def source_corpus_payload(corpus: RolSourceCorpus) -> dict[str, Any]:
  record_kinds = Counter(record.kind for record in corpus.records)
  directive_counts = Counter(
      (record.kind, directive["token"])
      for record in corpus.records
      for directive in record.directives
  )
  reference_counts = Counter(
      (reference.target_type, reference.role)
      for record in corpus.records
      for reference in record.references
  )
  diagnostic_counts = Counter(
      (diagnostic.severity, diagnostic.code) for diagnostic in corpus.diagnostics
  )
  return {
      "schema_version": ROL_SOURCE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "complete": corpus.complete,
      "summary": {
          "records": len(corpus.records),
          "records_by_kind": dict(sorted(record_kinds.items())),
          "references": sum(len(record.references) for record in corpus.records),
          "diagnostics": len(corpus.diagnostics),
          "incomplete_records": sum(not record.complete for record in corpus.records),
      },
      "file_versions": [
          {"kind": kind, "version": version, "files": count}
          for (kind, version), count in sorted(corpus.file_versions.items())
      ],
      "file_terminators": [
          {"kind": kind, "status": status, "files": count}
          for (kind, status), count in sorted(corpus.file_terminators.items())
      ],
      "directives": [
          {"kind": kind, "token": token, "occurrences": count}
          for (kind, token), count in sorted(directive_counts.items())
      ],
      "reference_roles": [
          {"target_type": target_type, "role": role, "occurrences": count}
          for (target_type, role), count in sorted(reference_counts.items())
      ],
      "diagnostic_counts": [
          {"severity": severity, "code": code, "occurrences": count}
          for (severity, code), count in sorted(diagnostic_counts.items())
      ],
      "diagnostics": [
          diagnostic.to_dict()
          for diagnostic in sorted(
              corpus.diagnostics,
              key=lambda item: (item.path, item.line, item.code, item.message),
          )
      ],
  }


def iter_record_dicts(records: Iterable[RolRecord]) -> Iterable[dict[str, Any]]:
  for record in sorted(records, key=lambda item: (item.kind, item.vnum, item.path, item.line)):
    yield record.to_dict()
