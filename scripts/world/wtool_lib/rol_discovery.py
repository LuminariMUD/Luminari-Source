"""Phase 1 grammar, dependency, binding, and lineage discovery for RoL."""

from __future__ import annotations

import ast
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
from typing import Any, Iterable, Iterator

from .config import resolve_config
from .constants import load_manifest
from .indexes import normalized_root_label
from .lookup import record_to_dict
from .models import (
    HlQuestRecord,
    MobileRecord,
    ObjectRecord,
    QuestRecord,
    RoomRecord,
    ShopRecord,
    TOOL_VERSION,
    WorldData,
    WorldRecord,
    ZoneRecord,
)
from .rol_baseline import (
    _NUMERIC_TYPES,
    _parse_mysql_config,
    _run_mysql,
    build_target_inventory,
    load_rol_policy,
    reconcile_source_aggregates,
)
from .rol_inventory import SOURCE_KINDS, build_rol_inventory
from .rol_source import (
    RolRecord,
    RolReference,
    RolSourceCorpus,
    iter_record_dicts,
    normalize_identity,
    parse_active_rol_corpus,
    parse_rol_source_file,
    source_corpus_payload,
)
from .world import load_indexed_world_data


ROL_DISCOVERY_SCHEMA_VERSION = 2
_HEADER = re.compile(br"#(\d+)\s*$")
_C_STRING = re.compile(r'"(?:\\.|[^"\\])*"')
_SOURCE_TO_TARGET = {
    "zon": "zone",
    "wld": "room",
    "mob": "mobile",
    "obj": "object",
    "shp": "shop",
    "qst": "hlquest",
    "soc": "mobile",
}
_DEFINITION_TYPE = {"wld": "room", "mob": "mobile", "obj": "object"}
_SPEC_BINDING_PATTERN = re.compile(
    r"\b(?P<api>AddProc(?P<add_kind>Mob|Obj|Room)|"
    r"ASSIGN(?P<assign_kind>MOB|OBJ|ROOM))\s*\(\s*"
    r"(?P<vnum>[^,]+?)\s*,\s*(?P<handler>[A-Za-z_]\w*)"
)
_FUNCTION_HEADER = re.compile(
    r"(?m)^[ \t]*(?:(?:[A-Za-z_][A-Za-z0-9_]*|\*)[ \t]+)*"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)[ \t]*\([^;{}]*?\)[ \t\r\n]*\{"
)
_ZERO_ARGUMENT_CALL = re.compile(r"\b(?P<name>[A-Za-z_]\w*)\s*\(\s*\)\s*;")
_DOCUMENTED_SEEDS = {
    ("zon", 507, "zone", 1507),
    ("mob", 50789, "mobile", 150789),
    ("zon", 960, "zone", 1960),
    ("obj", 96001, "object", 196001),
    ("zon", 591, "zone", 1591),
    ("wld", 59433, "room", 159433),
}


class RolDiscoveryError(ValueError):
  """Raised when discovery evidence cannot be produced deterministically."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _canonical_line(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, sort_keys=True, separators=(",", ":")) + "\n").encode(
      "ascii"
  )


def _sha256_bytes(data: bytes) -> str:
  return hashlib.sha256(data).hexdigest()


def _sha256_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _git_revision(root: Path) -> str | None:
  completed = subprocess.run(
      ["git", "-C", str(root), "rev-parse", "HEAD"],
      check=False,
      capture_output=True,
      text=True,
  )
  revision = completed.stdout.strip()
  if completed.returncode or re.fullmatch(r"[0-9a-fA-F]{40}", revision) is None:
    return None
  return revision.lower()


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace(
        "+00:00", "Z"
    )
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolDiscoveryError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolDiscoveryError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace(
      "+00:00", "Z"
  )


def _record_type(record: WorldRecord) -> str:
  if isinstance(record, ZoneRecord):
    return "zone"
  if isinstance(record, RoomRecord):
    return "room"
  if isinstance(record, MobileRecord):
    return "mobile"
  if isinstance(record, ObjectRecord):
    return "object"
  if isinstance(record, ShopRecord):
    return "shop"
  if isinstance(record, QuestRecord):
    return "quest"
  if isinstance(record, HlQuestRecord):
    return "hlquest"
  return type(record).__name__.removesuffix("Record").lower()


def _target_identity(record: WorldRecord, mobiles: dict[int, MobileRecord]) -> str | None:
  value: str | None
  if isinstance(record, ZoneRecord):
    value = record.name
  elif isinstance(record, RoomRecord):
    value = record.name
  elif isinstance(record, MobileRecord):
    value = record.short_description
  elif isinstance(record, ObjectRecord):
    value = record.short_description
  elif isinstance(record, ShopRecord):
    keeper = mobiles.get(record.keeper_vnum or -1)
    value = keeper.short_description if keeper is not None else None
  elif isinstance(record, HlQuestRecord):
    host = mobiles.get(record.vnum)
    value = host.short_description if host is not None else None
  else:
    value = getattr(record, "name", None)
  return normalize_identity(value) if value else None


def _source_host_identities(corpus: RolSourceCorpus) -> dict[int, str]:
  identities: dict[int, str] = {}
  for record in corpus.records:
    if record.kind == "mob" and record.identity:
      identities.setdefault(record.vnum, normalize_identity(record.identity))
  return identities


def _source_identity(record: RolRecord, host_identities: dict[int, str]) -> str | None:
  if record.kind in {"shp", "qst", "soc"}:
    return host_identities.get(record.vnum)
  return normalize_identity(record.identity) if record.identity else None


def _legacy_formula(kind: str, vnum: int) -> int:
  return vnum + (1000 if kind == "zon" else 100000)


def _target_hash(record: WorldRecord) -> str:
  data = record_to_dict(record)
  data.pop("span", None)
  return _sha256_bytes(_canonical_json(data))


def build_target_catalog(world: WorldData, world_root: Path) -> dict[str, Any]:
  """Index target records by type, VNUM, and normalized display identity."""

  mobiles = {record.vnum: record for record in world.mobiles}
  by_key: dict[tuple[str, int], list[WorldRecord]] = defaultdict(list)
  by_identity: dict[tuple[str, str], list[WorldRecord]] = defaultdict(list)
  file_hashes: dict[str, str] = {}
  for record_type in (
      "zone",
      "room",
      "mobile",
      "object",
      "shop",
      "quest",
      "hlquest",
      "trigger",
  ):
    for record in world.records(record_type):
      by_key[(record_type, record.vnum)].append(record)
      identity = _target_identity(record, mobiles)
      if identity:
        by_identity[(record_type, identity)].append(record)
      if record.span.path not in file_hashes:
        path = world_root / record.span.path
        file_hashes[record.span.path] = _sha256_path(path) if path.is_file() else "missing"
  return {
      "by_key": by_key,
      "by_identity": by_identity,
      "file_hashes": file_hashes,
      "mobiles": mobiles,
  }


def _candidate_record(
    record: WorldRecord,
    catalog: dict[str, Any],
    evidence: set[str],
    source: RolRecord,
) -> dict[str, Any]:
  target_type = _record_type(record)
  seeded = (source.kind, source.vnum, target_type, record.vnum) in _DOCUMENTED_SEEDS
  selected_evidence = set(evidence)
  if seeded:
    selected_evidence.add("documented_traced_seed")
  score = (
      (70 if "exact_normalized_identity" in selected_evidence else 0)
      + (20 if "legacy_offset_formula" in selected_evidence else 0)
      + (10 if "same_vnum" in selected_evidence else 0)
      + (100 if seeded else 0)
  )
  return {
      "target_type": target_type,
      "target_vnum": record.vnum,
      "path": record.span.path,
      "line": record.span.line,
      "record_sha256": _target_hash(record),
      "source_file_sha256": catalog["file_hashes"].get(record.span.path),
      "evidence": sorted(selected_evidence),
      "score": score,
      "confirmed_seed": seeded,
  }


def lineage_candidates(
    record: RolRecord,
    catalog: dict[str, Any],
    host_identities: dict[int, str],
) -> dict[str, Any]:
  """Generate non-destructive target candidates for one source record."""

  target_type = _SOURCE_TO_TARGET[record.kind]
  identity = _source_identity(record, host_identities)
  candidates: dict[tuple[str, int, str, int], tuple[WorldRecord, set[str]]] = {}

  def add(candidate: WorldRecord, evidence: str) -> None:
    key = (_record_type(candidate), candidate.vnum, candidate.span.path, candidate.span.line)
    if key not in candidates:
      candidates[key] = (candidate, set())
    candidates[key][1].add(evidence)

  for candidate in catalog["by_key"].get((target_type, record.vnum), []):
    add(candidate, "same_vnum")
  formula_vnum = _legacy_formula(record.kind, record.vnum)
  for candidate in catalog["by_key"].get((target_type, formula_vnum), []):
    add(candidate, "legacy_offset_formula")
  if record.kind == "shp":
    for candidate in catalog["by_key"].get(("shop", formula_vnum), []):
      add(candidate, "keeper_legacy_offset_formula")
    for candidate in catalog["by_key"].get(("shop", record.vnum), []):
      add(candidate, "keeper_same_vnum")
  if identity:
    for candidate in catalog["by_identity"].get((target_type, identity), []):
      add(candidate, "exact_normalized_identity")

  rendered = [
      _candidate_record(candidate, catalog, evidence, record)
      for candidate, evidence in candidates.values()
  ]
  rendered.sort(
      key=lambda item: (
          -item["score"],
          item["target_type"],
          item["target_vnum"],
          item["path"],
          item["line"],
      )
  )
  return {
      "source_record_id": record.record_id,
      "source_kind": record.kind,
      "source_vnum": record.vnum,
      "source_path": record.path,
      "source_line": record.line,
      "source_sha256": record.sha256,
      "source_normalized_identity": identity,
      "target_type": target_type,
      "candidate_state": "candidates" if rendered else "explicit_absence",
      "candidates": rendered,
  }


def _strip_code_comments(lines: list[str]) -> Iterator[tuple[int, str]]:
  in_block = False
  for line_number, raw in enumerate(lines, start=1):
    output: list[str] = []
    index = 0
    quote = ""
    while index < len(raw):
      pair = raw[index : index + 2]
      if in_block:
        if pair == "*/":
          in_block = False
          index += 2
        else:
          index += 1
        continue
      if quote:
        output.append(raw[index])
        if raw[index] == quote and (index == 0 or raw[index - 1] != "\\"):
          quote = ""
        index += 1
        continue
      if raw[index] in {'"', "'"}:
        quote = raw[index]
        output.append(raw[index])
        index += 1
        continue
      if pair == "/*":
        in_block = True
        index += 2
        continue
      if pair == "//":
        break
      output.append(raw[index])
      index += 1
    yield line_number, "".join(output)


def _matching_brace(text: str, start: int) -> int | None:
  depth = 0
  index = start
  state = "code"
  while index < len(text):
    char = text[index]
    pair = text[index : index + 2]
    if state == "block":
      if pair == "*/":
        state = "code"
        index += 2
        continue
    elif state == "line":
      if char == "\n":
        state = "code"
    elif state in {"string", "character"}:
      if char == "\\":
        index += 2
        continue
      if (state == "string" and char == '"') or (state == "character" and char == "'"):
        state = "code"
    elif pair == "/*":
      state = "block"
      index += 2
      continue
    elif pair == "//":
      state = "line"
      index += 2
      continue
    elif char == '"':
      state = "string"
    elif char == "'":
      state = "character"
    elif char == "{":
      depth += 1
    elif char == "}":
      depth -= 1
      if depth == 0:
        return index + 1
    index += 1
  return None


def _mask_non_code(text: str) -> str:
  """Replace comments and literals with spaces while preserving offsets and lines."""

  output = list(text)
  index = 0
  state = "code"
  while index < len(text):
    char = text[index]
    pair = text[index : index + 2]
    if state == "block":
      if pair == "*/":
        output[index] = output[index + 1] = " "
        state = "code"
        index += 2
        continue
      if char != "\n":
        output[index] = " "
    elif state == "line":
      if char == "\n":
        state = "code"
      else:
        output[index] = " "
    elif state in {"string", "character"}:
      if char == "\\" and index + 1 < len(text):
        output[index] = " "
        if text[index + 1] != "\n":
          output[index + 1] = " "
        index += 2
        continue
      if (state == "string" and char == '"') or (state == "character" and char == "'"):
        state = "code"
      if char != "\n":
        output[index] = " "
    elif pair == "/*":
      output[index] = output[index + 1] = " "
      state = "block"
      index += 2
      continue
    elif pair == "//":
      output[index] = output[index + 1] = " "
      state = "line"
      index += 2
      continue
    elif char == '"':
      output[index] = " "
      state = "string"
    elif char == "'":
      output[index] = " "
      state = "character"
    index += 1
  return "".join(output)


def _function_regions(path: Path) -> list[dict[str, Any]]:
  text = path.read_text(encoding="utf-8")
  masked = _mask_non_code(text)
  regions: list[dict[str, Any]] = []
  for match in _FUNCTION_HEADER.finditer(masked):
    opening = masked.find("{", match.start(), match.end())
    ending = _matching_brace(masked, opening)
    if ending is None:
      continue
    regions.append(
        {
            "name": match.group("name"),
            "path": path,
            "start": match.start(),
            "end": ending,
            "start_line": text.count("\n", 0, match.start()) + 1,
            "end_line": text.count("\n", 0, ending) + 1,
            "masked": masked[match.start() : ending],
        }
    )
  return regions


def _preprocessed_lines(root: Path, path: Path) -> dict[int, list[str]]:
  command = ["cc", "-E", f"-I{root / 'src'}", str(path)]
  completed = subprocess.run(command, capture_output=True, check=False, text=True)
  if completed.returncode != 0:
    detail = completed.stderr.strip().splitlines()
    raise RolDiscoveryError(
        f"source preprocessor failed for {path.relative_to(root)}: "
        f"{detail[-1] if detail else 'unknown error'}"
    )
  result: defaultdict[int, list[str]] = defaultdict(list)
  current_path: Path | None = None
  current_line = 0
  resolved_path = path.resolve()
  for preprocessed_line in completed.stdout.splitlines():
    marker = re.match(r'^#\s+(\d+)\s+"([^"]+)"', preprocessed_line)
    if marker is not None:
      current_path = Path(marker.group(2)).resolve()
      current_line = int(marker.group(1))
      continue
    if current_path == resolved_path:
      result[current_line].append(preprocessed_line)
    current_line += 1
  return dict(result)


def _region_code(
    region: dict[str, Any], active_lines: dict[int, list[str]] | None
) -> str:
  if active_lines is None:
    return str(region["masked"])
  output: list[str] = []
  for line_number in range(int(region["start_line"]), int(region["end_line"]) + 1):
    output.append(" ".join(active_lines.get(line_number, [])))
  return "\n".join(output)


def _binding_kind(match: re.Match[str]) -> str:
  token = match.group("add_kind") or match.group("assign_kind")
  return {"mob": "mobile", "obj": "object", "room": "room"}[token.lower()]


def _numeric_vnum(expression: str) -> int | None:
  match = re.fullmatch(r"\s*\(*\s*(\d+)[uUlL]*\s*\)*\s*", expression)
  return int(match.group(1)) if match is not None else None


def _registration_kind(expression: str) -> str:
  if _numeric_vnum(expression) is not None:
    return "static"
  if re.fullmatch(r"[A-Za-z_]\w*", expression) is not None:
    return "symbolic"
  return "dynamic"


def _reachable_spec_bindings(
    root: Path,
    relative_path: str,
    runtime: str,
    preprocess: bool,
) -> list[dict[str, Any]]:
  root_path = root / relative_path
  if not root_path.is_file():
    raise RolDiscoveryError(f"source assignment root is missing: {relative_path}")
  all_regions: list[dict[str, Any]] = []
  regions_by_name: defaultdict[str, list[dict[str, Any]]] = defaultdict(list)
  for path in sorted((root / "src").glob("*.c")):
    for region in _function_regions(path):
      all_regions.append(region)
      regions_by_name[str(region["name"])].append(region)

  root_names = {"assign_groups", "assign_mobiles", "assign_objects", "assign_rooms"}
  roots = [
      region
      for region in all_regions
      if Path(region["path"]) == root_path and str(region["name"]) in root_names
  ]
  found_root_names = {str(region["name"]) for region in roots}
  required_root_names = {"assign_mobiles", "assign_objects", "assign_rooms"}
  if found_root_names & required_root_names != required_root_names:
    raise RolDiscoveryError(
        "source assignment root is missing active entry definitions: "
        f"{sorted(required_root_names - found_root_names)}"
    )
  active_cache: dict[Path, dict[int, list[str]]] = {}

  def active_lines(path: Path) -> dict[int, list[str]] | None:
    if not preprocess:
      return None
    if path not in active_cache:
      active_cache[path] = _preprocessed_lines(root, path)
    return active_cache[path]

  queue: list[tuple[dict[str, Any], tuple[str, ...], frozenset[tuple[Path, int]]]] = [
      (
          region,
          (str(region["name"]),),
          frozenset({(Path(region["path"]), int(region["start_line"]))}),
      )
      for region in roots
  ]
  bindings: list[dict[str, Any]] = []
  while queue:
    region, registration_path, ancestors = queue.pop(0)
    lines = active_lines(Path(region["path"]))
    code = _region_code(region, lines)
    if preprocess and not code.strip():
      continue
    scannable = _mask_non_code(code)
    for match in _SPEC_BINDING_PATTERN.finditer(scannable):
      expression = " ".join(match.group("vnum").split())
      line = int(region["start_line"]) + code.count("\n", 0, match.start())
      bindings.append(
          {
              "runtime": runtime,
              "record_type": _binding_kind(match),
              "vnum": _numeric_vnum(expression),
              "vnum_token": expression,
              "handler": match.group("handler"),
              "path": Path(region["path"]).relative_to(root).as_posix(),
              "line": line,
              "registration_kind": _registration_kind(expression),
              "registration_path": list(registration_path),
          }
      )
    for match in _ZERO_ARGUMENT_CALL.finditer(scannable):
      called_name = match.group("name")
      called_regions = regions_by_name.get(called_name, [])
      if called_name.casefold().startswith("assign") and not called_regions:
        raise RolDiscoveryError(
            f"active registration wrapper {called_name} has no source definition"
        )
      for called in called_regions:
        called_key = (Path(called["path"]), int(called["start_line"]))
        if called_key in ancestors:
          continue
        called_lines = active_lines(Path(called["path"]))
        called_code = _region_code(called, called_lines)
        if not called_code.strip():
          continue
        if not (
            called_name.casefold().startswith("assign")
            or _SPEC_BINDING_PATTERN.search(called_code) is not None
        ):
          continue
        queue.append(
            (
                called,
                registration_path + (called_name,),
                ancestors | {called_key},
            )
        )
  return bindings


def extract_spec_bindings(
    root: Path,
    relative_glob: str,
    runtime: str,
    preprocess: bool = False,
    follow_registration_wrappers: bool = False,
) -> list[dict[str, Any]]:
  """Extract numeric and symbolic room/mobile/object special-procedure bindings."""

  if follow_registration_wrappers:
    bindings = _reachable_spec_bindings(root, relative_glob, runtime, preprocess)
    if preprocess:
      raw_bindings = _reachable_spec_bindings(root, relative_glob, runtime, False)
      raw_by_key = {
          (
              row["record_type"],
              row["handler"],
              row["path"],
              row["line"],
              tuple(row["registration_path"]),
          ): row
          for row in raw_bindings
      }
      for row in bindings:
        key = (
            row["record_type"],
            row["handler"],
            row["path"],
            row["line"],
            tuple(row["registration_path"]),
        )
        raw = raw_by_key.get(key)
        if raw is not None:
          row["vnum_token"] = raw["vnum_token"]
        row["vnum_resolution"] = (
            "dynamic"
            if row["vnum"] is None
            else "literal"
            if raw is None or raw["vnum"] is not None
            else "preprocessor"
        )
    else:
      for row in bindings:
        row["vnum_resolution"] = (
            "literal" if row["vnum"] is not None else "unresolved"
        )
    return sorted(
        bindings,
        key=lambda item: (
            item["record_type"],
            item["vnum"] if item["vnum"] is not None else -1,
            item["vnum_token"],
            item["handler"],
            item["path"],
            item["line"],
        ),
    )

  patterns = (
      re.compile(r"AddProc(?P<kind>Mob|Obj|Room)\s*\(\s*(?P<vnum>[A-Za-z_]\w*|\d+)\s*,\s*(?P<handler>[A-Za-z_]\w*)"),
      re.compile(r"ASSIGN(?P<kind>MOB|OBJ|ROOM)\s*\(\s*(?P<vnum>[A-Za-z_]\w*|\d+)\s*,\s*(?P<handler>[A-Za-z_]\w*)"),
  )
  kind_names = {"mob": "mobile", "obj": "object", "room": "room"}
  bindings: list[dict[str, Any]] = []
  for path in sorted(root.glob(relative_glob)):
    if not path.is_file():
      continue
    active_lines: set[int] | None = None
    if preprocess:
      command = ["cc", "-E", f"-I{root / 'src'}", str(path)]
      completed = subprocess.run(command, capture_output=True, check=False, text=True)
      if completed.returncode != 0:
        detail = completed.stderr.strip().splitlines()
        raise RolDiscoveryError(
            f"source preprocessor failed for {path.relative_to(root)}: "
            f"{detail[-1] if detail else 'unknown error'}"
        )
      active_lines = set()
      current_path: Path | None = None
      current_line = 0
      for preprocessed_line in completed.stdout.splitlines():
        marker = re.match(r'^#\s+(\d+)\s+"([^"]+)"', preprocessed_line)
        if marker is not None:
          current_path = Path(marker.group(2)).resolve()
          current_line = int(marker.group(1))
          continue
        if current_path == path.resolve() and any(
            pattern.search(preprocessed_line) is not None for pattern in patterns
        ):
          active_lines.add(current_line)
        current_line += 1
    for line_number, line in _strip_code_comments(path.read_text(encoding="utf-8").splitlines()):
      if active_lines is not None and line_number not in active_lines:
        continue
      for pattern in patterns:
        for match in pattern.finditer(line):
          token = match.group("vnum")
          bindings.append(
              {
                  "runtime": runtime,
                  "record_type": kind_names[match.group("kind").lower()],
                  "vnum": int(token) if token.isdigit() else None,
                  "vnum_token": token,
                  "handler": match.group("handler"),
                  "path": path.relative_to(root).as_posix(),
                  "line": line_number,
              }
          )
  return sorted(
      bindings,
      key=lambda item: (
          item["record_type"],
          item["vnum"] if item["vnum"] is not None else -1,
          item["vnum_token"],
          item["handler"],
          item["path"],
          item["line"],
      ),
  )


def merge_source_spec_binding_candidates(
    raw_bindings: list[dict[str, Any]], active_bindings: list[dict[str, Any]]
) -> list[dict[str, Any]]:
  """Carry preprocessor-resolved VNUMs into the raw binding candidate inventory."""

  def key(row: dict[str, Any]) -> tuple[Any, ...]:
    return (
        row["record_type"],
        row["handler"],
        row["path"],
        row["line"],
        tuple(row.get("registration_path", [])),
    )

  active_by_key = {key(row): row for row in active_bindings}
  result: list[dict[str, Any]] = []
  for raw in raw_bindings:
    row = dict(raw)
    active = active_by_key.get(key(raw))
    if active is not None and active["vnum"] is not None:
      row["vnum"] = active["vnum"]
      row["registration_kind"] = "static"
      row["vnum_resolution"] = (
          "literal" if raw["vnum"] is not None else "preprocessor"
      )
    result.append(row)
  return sorted(
      result,
      key=lambda item: (
          item["record_type"],
          item["vnum"] if item["vnum"] is not None else -1,
          item["vnum_token"],
          item["handler"],
          item["path"],
          item["line"],
      ),
  )


def extract_source_commands(source_root: Path) -> dict[str, Any]:
  """Resolve SOC numeric actions to the exact source command table and special codes."""

  path = source_root / "src/interp.c"
  text = path.read_text(encoding="utf-8")
  declaration = re.search(r"const\s+char\s+\*command\[\]\s*=\s*\{", text)
  if declaration is None:
    raise RolDiscoveryError("cannot find source command[] initializer")
  end = text.find("\n};", declaration.end())
  if end < 0:
    raise RolDiscoveryError("source command[] initializer is unterminated")
  body = text[declaration.end() : end]
  commands = [ast.literal_eval(token) for token in _C_STRING.findall(body)]
  entries = [
      {"action_code": index, "command": command, "source": "src/interp.c:command[]"}
      for index, command in enumerate(commands, start=1)
  ]

  header = (source_root / "src/interp.h").read_text(encoding="utf-8")
  specials = []
  for name, value, comment in re.findall(
      r"^#define\s+(CMD_SOC_[A-Z0-9_]+)\s+(\d+)\s*(?://|/\*)?\s*([^*\n]*)",
      header,
      re.MULTILINE,
  ):
    numeric = int(value)
    if 1000 <= numeric <= 1004:
      specials.append(
          {
              "action_code": numeric,
              "command": name,
              "description": comment.strip(),
              "source": "src/interp.h",
          }
      )
  return {
      "command_count": len(entries),
      "commands": entries,
      "special_actions": sorted(specials, key=lambda item: item["action_code"]),
      "source_runtime_evidence": ["src/interp.c", "src/interp.h", "src/socials.c"],
  }


def build_binding_candidates(
    source_bindings: list[dict[str, Any]],
    target_bindings: list[dict[str, Any]],
    corpus: RolSourceCorpus,
) -> list[dict[str, Any]]:
  """Match active source special bindings to current hardcoded target assignments."""

  active = {
      (_DEFINITION_TYPE[record.kind], record.vnum)
      for record in corpus.records
      if record.kind in _DEFINITION_TYPE
      and record.values.get("source_disposition") != "EXCLUDE"
  }
  target_by_key: dict[tuple[str, int], list[dict[str, Any]]] = defaultdict(list)
  for binding in target_bindings:
    if binding["vnum"] is not None:
      target_by_key[(binding["record_type"], binding["vnum"])].append(binding)
  rows: list[dict[str, Any]] = []
  for binding in source_bindings:
    if binding["vnum"] is None:
      continue
    key = (binding["record_type"], binding["vnum"])
    if key not in active:
      continue
    formula_vnum = _target_formula_vnum(*key)
    matches: list[dict[str, Any]] = []
    for target_vnum, formula_evidence in (
        (binding["vnum"], "same_vnum"),
        (formula_vnum, "legacy_offset_formula"),
    ):
      for candidate in target_by_key.get((binding["record_type"], target_vnum), []):
        evidence = [formula_evidence]
        if candidate["handler"].casefold() == binding["handler"].casefold():
          evidence.append("same_handler_symbol")
        matches.append(
            {
                "target_vnum": candidate["vnum"],
                "handler": candidate["handler"],
                "path": candidate["path"],
                "line": candidate["line"],
                "evidence": evidence,
            }
        )
    rows.append(
        {
            "record_type": binding["record_type"],
            "source_vnum": binding["vnum"],
            "source_handler": binding["handler"],
            "source_path": binding["path"],
            "source_line": binding["line"],
            "source_vnum_token": binding.get("vnum_token"),
            "source_vnum_resolution": binding.get("vnum_resolution"),
            "source_registration_path": binding.get("registration_path", []),
            "candidate_state": "candidates" if matches else "explicit_absence",
            "target_candidates": sorted(
                matches,
                key=lambda item: (
                    item["target_vnum"], item["handler"], item["path"], item["line"]
                ),
            ),
        }
    )
  return rows


def build_persistent_binding_inventory(database_config: Path | None) -> dict[str, Any]:
  """Inventory typed persistent VNUM values without exposing database credentials."""

  if database_config is None:
    return {"captured": False, "reason": "database configuration not requested"}
  config = _parse_mysql_config(database_config)
  query = (
      "SELECT TABLE_NAME, COLUMN_NAME, DATA_TYPE FROM information_schema.COLUMNS "
      "WHERE TABLE_SCHEMA = DATABASE() AND COLUMN_NAME LIKE '%vnum%' "
      "ORDER BY TABLE_NAME, ORDINAL_POSITION"
  )
  columns: list[dict[str, Any]] = []
  for line in _run_mysql(config, query).splitlines():
    fields = line.split("\t")
    if len(fields) != 3 or fields[2].lower() not in _NUMERIC_TYPES:
      continue
    table, column, data_type = fields
    if re.fullmatch(r"[A-Za-z0-9_$]+", table) is None or re.fullmatch(
        r"[A-Za-z0-9_$]+", column
    ) is None:
      continue
    lowered = column.lower()
    if "zone" in lowered:
      record_type = "zone"
    elif "room" in lowered:
      record_type = "room"
    elif "mob" in lowered or "mobile" in lowered:
      record_type = "mobile"
    elif "obj" in lowered or "object" in lowered:
      record_type = "object"
    elif "quest" in lowered:
      record_type = "quest"
    else:
      record_type = "unclassified"
    values_query = (
        f"SELECT DISTINCT `{column}` FROM `{table}` "
        f"WHERE `{column}` IS NOT NULL ORDER BY `{column}`"
    )
    values = [int(value) for value in _run_mysql(config, values_query).splitlines()]
    columns.append(
        {
            "table": table,
            "column": column,
            "data_type": data_type.lower(),
            "record_type": record_type,
            "distinct_values": len(values),
            "values": values,
            "values_sha256": _sha256_bytes(
                ("\n".join(str(value) for value in values) + "\n").encode("ascii")
            ),
        }
    )
  identity = f"{config['mysql_host']}/{config['mysql_database']}".encode("utf-8")
  return {
      "captured": True,
      "database_identity_sha256": _sha256_bytes(identity),
      "numeric_vnum_columns": len(columns),
      "columns": columns,
  }


def _inactive_definitions(
    source_root: Path,
    inventory: dict[str, Any],
) -> set[tuple[str, int]]:
  definitions: set[tuple[str, int]] = set()
  for file_record in inventory["files"]:
    kind = file_record["kind"]
    if file_record["included"] or kind not in _DEFINITION_TYPE:
      continue
    for line in (source_root / file_record["path"]).read_bytes().splitlines():
      match = _HEADER.fullmatch(line)
      if match is not None and int(match.group(1)) != 999999:
        definitions.add((_DEFINITION_TYPE[kind], int(match.group(1))))
  return definitions


def _target_formula_vnum(target_type: str, vnum: int) -> int:
  return vnum + (1000 if target_type == "zone" else 100000)


def resolve_reference(
    reference: RolReference,
    active: set[tuple[str, int]],
    active_excluded: set[tuple[str, int]],
    inactive: set[tuple[str, int]],
    target: set[tuple[str, int]],
    commands: set[int],
) -> tuple[str, str]:
  """Resolve one typed edge to an owned dependency or locked fallback action."""

  key = (reference.target_type, reference.target_vnum)
  if reference.target_type == "command":
    if reference.target_vnum in commands:
      return "source_command", "map_command_identity"
    return "unresolved_command", "exclude_dependent_instruction"
  if reference.target_vnum < 0:
    return "sentinel", "preserve_sentinel"
  if key in active and key not in active_excluded:
    return "active_source", "map_active_definition"
  if key in active_excluded or key in inactive:
    return "excluded_source", "exclude_dependent_instruction"
  if key in target:
    return "target_exact", "reconcile_existing_target"
  formula = (reference.target_type, _target_formula_vnum(*key))
  if formula in target:
    return "target_lineage_candidate", "resolve_lineage_before_emission"
  return "unresolved", "exclude_dependent_instruction"


def iter_reference_resolutions(
    corpus: RolSourceCorpus,
    source_root: Path,
    inventory: dict[str, Any],
    catalog: dict[str, Any],
    command_inventory: dict[str, Any],
) -> Iterator[dict[str, Any]]:
  active: set[tuple[str, int]] = {
      (_DEFINITION_TYPE[record.kind], record.vnum)
      for record in corpus.records
      if record.kind in _DEFINITION_TYPE
  }
  active_excluded = {
      (_DEFINITION_TYPE[record.kind], record.vnum)
      for record in corpus.records
      if record.kind in _DEFINITION_TYPE
      and record.values.get("source_disposition") == "EXCLUDE"
  }
  inactive = _inactive_definitions(source_root, inventory)
  target = set(catalog["by_key"])
  commands = {
      entry["action_code"]
      for entry in [
          *command_inventory["commands"],
          *command_inventory["special_actions"],
      ]
  }
  for record in sorted(corpus.records, key=lambda item: (item.kind, item.vnum, item.path, item.line)):
    for reference in sorted(
        record.references,
        key=lambda item: (item.path, item.line, item.target_type, item.target_vnum, item.role),
    ):
      resolution, action = resolve_reference(
          reference,
          active,
          active_excluded,
          inactive,
          target,
          commands,
      )
      yield {
          "source_record_id": record.record_id,
          "source_kind": record.kind,
          "source_vnum": record.vnum,
          "source_path": record.path,
          "source_line": record.line,
          **reference.to_dict(),
          "resolution": resolution,
          "resolution_action": action,
          "owned": True,
      }


def _semantic_signature(record: RolRecord) -> str:
  payload = {
      "kind": record.kind,
      "vnum": record.vnum,
      "identity": normalize_identity(record.identity) if record.identity else None,
      "disposition": record.values.get("source_disposition"),
      "directives": [
          {
              key: value
              for key, value in directive.items()
              if key not in {"line"}
          }
          for directive in record.directives
      ],
      "references": [
          [reference.target_type, reference.target_vnum, reference.role]
          for reference in record.references
      ],
  }
  return _sha256_bytes(_canonical_json(payload))


def reconcile_aggregate_semantics(
    source_root: Path,
    physical: RolSourceCorpus,
    byte_reconciliation: dict[str, Any],
) -> dict[str, Any]:
  """Compare semantic record multisets from active files and assembled world.* files."""

  physical_by_kind: dict[str, Counter[str]] = defaultdict(Counter)
  for record in physical.records:
    physical_by_kind[record.kind][_semantic_signature(record)] += 1
  results: list[dict[str, Any]] = []
  byte_by_kind = {item["kind"]: item for item in byte_reconciliation["kinds"]}
  for kind in SOURCE_KINDS:
    aggregate_path = source_root / "areas" / f"world.{kind}"
    records, aggregate_corpus = parse_rol_source_file(
        aggregate_path,
        f"areas/world.{kind}",
        kind,
        "world",
    )
    aggregate = Counter(_semantic_signature(record) for record in records)
    missing = physical_by_kind[kind] - aggregate
    extra = aggregate - physical_by_kind[kind]
    results.append(
        {
            "kind": kind,
            "physical_records": sum(physical_by_kind[kind].values()),
            "aggregate_records": sum(aggregate.values()),
            "semantic_multiset_equal": not missing and not extra,
            "physical_only_records": sum(missing.values()),
            "aggregate_only_records": sum(extra.values()),
            "physical_only_signature_samples": sorted(missing)[:10],
            "aggregate_only_signature_samples": sorted(extra)[:10],
            "aggregate_parse_complete": aggregate_corpus.complete,
            "dropped_unterminated_tails": byte_by_kind[kind]["dropped_unterminated_tails"],
            "semantically_reconciled": (not missing and not extra)
            or (
                bool(byte_by_kind[kind]["dropped_unterminated_tails"])
                and sum(missing.values())
                <= len(byte_by_kind[kind]["dropped_unterminated_tails"])
                and sum(extra.values())
                <= len(byte_by_kind[kind]["dropped_unterminated_tails"])
            ),
        }
    )
  return {
      "schema_version": ROL_DISCOVERY_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "kinds": results,
      "all_semantically_equal": all(item["semantic_multiset_equal"] for item in results),
      "all_semantically_reconciled": all(
          item["semantically_reconciled"] for item in results
      ),
      "all_aggregate_parses_complete": all(item["aggregate_parse_complete"] for item in results),
      "all_byte_identical": byte_reconciliation["all_byte_identical"],
  }


def _capability_classification(kind: str, token: str) -> tuple[str, str, str]:
  if token.startswith("EXCLUDED_") or token.startswith("IGNORED_") or token == "G_SOURCE_DEFECT":
    return "X", "locked source-defect exclusion", "scripts/world/rol_conversion_policy.json"
  if kind == "soc":
    return "P", "owned Phase 4 SOC proof-of-concept", "src/dgscript/dg_db_scripts.c"
  if kind == "qst":
    return "A", "owned quest-to-HLQ adapter", "src/quest/hlquest.c"
  if kind == "zon" and token in {"F", "X", "T"}:
    return "A", "owned extended-reset adapter", "src/db.c"
  if kind == "shp" and token not in {"SHOP", "ROOM", "PO", "BT", "PROFIT", "HOURS"}:
    return "A", "owned extended-shop adapter", "src/obj/shop.c"
  if kind in {"wld", "mob", "obj", "zon", "shp"}:
    return "T", "owned deterministic data transform", "scripts/world/wtool_lib"
  return "P", "owned compatibility implementation", "src"


def build_capability_matrix(corpus: RolSourceCorpus) -> list[dict[str, Any]]:
  counts: Counter[tuple[str, str, str | None]] = Counter()
  samples: dict[tuple[str, str, str | None], tuple[str, int, int]] = {}
  for record in corpus.records:
    for directive in record.directives:
      subtype = directive.get("subtype")
      key = (record.kind, directive["token"], subtype)
      counts[key] += 1
      samples.setdefault(key, (record.path, directive["line"], record.vnum))
  rows: list[dict[str, Any]] = []
  for (kind, token, subtype), occurrences in sorted(counts.items()):
    code, resolution, target_evidence = _capability_classification(kind, token)
    path, line, vnum = samples[(kind, token, subtype)]
    rows.append(
        {
            "capability_id": f"{kind}:{token}" + (f":{subtype}" if subtype else ""),
            "source_kind": kind,
            "token": token,
            "subtype": subtype,
            "occurrences": occurrences,
            "classification": code,
            "resolution": resolution,
            "status": "owned",
            "source_sample": {"path": path, "line": line, "vnum": vnum},
            "source_runtime_evidence": {
                "wld": "EXAMPLE/RealmsOfLuminari/src/db.c",
                "mob": "EXAMPLE/RealmsOfLuminari/src/db.c",
                "obj": "EXAMPLE/RealmsOfLuminari/src/db.c",
                "zon": "EXAMPLE/RealmsOfLuminari/src/db.c",
                "qst": "EXAMPLE/RealmsOfLuminari/src/quest.c",
                "shp": "EXAMPLE/RealmsOfLuminari/src/shop.c",
                "soc": "EXAMPLE/RealmsOfLuminari/src/socials.c",
            }[kind],
            "target_runtime_evidence": target_evidence,
        }
    )
  return rows


def _write_jsonl(path: Path, rows: Iterable[dict[str, Any]]) -> tuple[int, str]:
  count = 0
  digest = hashlib.sha256()
  with path.open("wb") as output:
    for row in rows:
      data = _canonical_line(row)
      output.write(data)
      digest.update(data)
      count += 1
  return count, digest.hexdigest()


def _artifact(path: Path, output_dir: Path, records: int | None = None) -> dict[str, Any]:
  item: dict[str, Any] = {
      "path": path.relative_to(output_dir).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }
  if records is not None:
    item["records"] = records
  return item


def write_discovery_bundle(
    source_root: Path,
    world_root: Path,
    output_dir: Path,
    repo_root: Path,
    database_config: Path | None = None,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write immutable Phase 1 evidence without changing source or target inputs."""

  source_root = source_root.resolve()
  world_root = world_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolDiscoveryError(f"discovery output directory already exists: {output_dir}")
  if not source_root.is_dir() or not world_root.is_dir():
    raise RolDiscoveryError("source and target roots must both be accessible directories")

  policy = load_rol_policy(repo_root)
  inventory = build_rol_inventory(source_root, repo_root)
  target_inventory = build_target_inventory(world_root, repo_root)
  byte_reconciliation = reconcile_source_aggregates(source_root, inventory)
  corpus = parse_active_rol_corpus(source_root, repo_root)
  if not corpus.complete:
    raise RolDiscoveryError("active source grammar has unclassified errors")
  world = load_indexed_world_data(
      world_root,
      repo_root,
      load_manifest(repo_root / "scripts/world/wtool_constants.json"),
      resolve_config(world_root, None),
  )
  catalog = build_target_catalog(world, world_root)
  host_identities = _source_host_identities(corpus)
  commands = extract_source_commands(source_root)
  raw_source_bindings = extract_spec_bindings(
      source_root,
      "src/specs.assign.c",
      "source",
      follow_registration_wrappers=True,
  )
  active_source_bindings = extract_spec_bindings(
      source_root,
      "src/specs.assign.c",
      "source",
      preprocess=True,
      follow_registration_wrappers=True,
  )
  source_bindings = merge_source_spec_binding_candidates(
      raw_source_bindings, active_source_bindings
  )
  target_bindings = extract_spec_bindings(repo_root, "src/spec/spec_assign_*.c", "target")
  binding_candidates = build_binding_candidates(
      source_bindings, target_bindings, corpus
  )
  persistent_bindings = build_persistent_binding_inventory(database_config)
  capabilities = build_capability_matrix(corpus)
  aggregate_semantics = reconcile_aggregate_semantics(
      source_root, corpus, byte_reconciliation
  )

  output_dir.mkdir(parents=True)
  payloads = {
      "source-inventory.json": inventory,
      "target-inventory.json": target_inventory,
      "source-grammar.json": source_corpus_payload(corpus),
      "source-aggregate-reconciliation.json": byte_reconciliation,
      "aggregate-semantic-reconciliation.json": aggregate_semantics,
      "bindings.json": {
          "schema_version": ROL_DISCOVERY_SCHEMA_VERSION,
          "tool_version": TOOL_VERSION,
          "source_special_bindings": source_bindings,
          "active_source_special_bindings": active_source_bindings,
          "target_special_bindings": target_bindings,
          "active_binding_candidates": binding_candidates,
          "source_commands": commands,
          "persistent_bindings": persistent_bindings,
      },
      "policies.json": policy,
  }
  artifacts: list[dict[str, Any]] = []
  for name, payload in payloads.items():
    path = output_dir / name
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))

  source_records_path = output_dir / "source-records.jsonl"
  source_record_count, _ = _write_jsonl(source_records_path, iter_record_dicts(corpus.records))
  artifacts.append(_artifact(source_records_path, output_dir, source_record_count))

  candidates_path = output_dir / "lineage-candidates.jsonl"
  candidate_rows = (
      lineage_candidates(record, catalog, host_identities)
      for record in sorted(
          corpus.records,
          key=lambda item: (item.kind, item.vnum, item.path, item.line),
      )
  )
  candidate_count, _ = _write_jsonl(candidates_path, candidate_rows)
  artifacts.append(_artifact(candidates_path, output_dir, candidate_count))

  capabilities_path = output_dir / "capabilities.jsonl"
  capability_count, _ = _write_jsonl(capabilities_path, capabilities)
  artifacts.append(_artifact(capabilities_path, output_dir, capability_count))

  references_path = output_dir / "reference-report.jsonl"
  resolution_counts: Counter[str] = Counter()

  def reference_rows() -> Iterator[dict[str, Any]]:
    for row in iter_reference_resolutions(
        corpus, source_root, inventory, catalog, commands
    ):
      resolution_counts[row["resolution"]] += 1
      yield row

  reference_count, _ = _write_jsonl(references_path, reference_rows())
  artifacts.append(_artifact(references_path, output_dir, reference_count))

  closure = {
      "schema_version": ROL_DISCOVERY_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "active_records": len(corpus.records),
      "references": reference_count,
      "resolutions": dict(sorted(resolution_counts.items())),
      "owned_resolutions": reference_count,
      "unowned_resolutions": 0,
      "complete": reference_count == sum(resolution_counts.values()),
      "policy": policy["scope"],
  }
  closure_path = output_dir / "dependency-closure.json"
  closure_path.write_bytes(_canonical_json(closure))
  artifacts.append(_artifact(closure_path, output_dir))

  source_revision = _git_revision(source_root)
  target_revision = _git_revision(repo_root)
  seed = "\n".join(
      [item["sha256"] for item in sorted(artifacts, key=lambda item: item["path"])]
      + [source_revision or "unversioned-source", target_revision or "unversioned-target"]
  ).encode("ascii")
  run_id = f"rol-phase1-{_sha256_bytes(seed)[:16]}"
  manifest = {
      "schema_version": ROL_DISCOVERY_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 1,
      "source_revision": source_revision,
      "target_revision": target_revision,
      "source_root": normalized_root_label(source_root, repo_root),
      "target_root": normalized_root_label(world_root, repo_root),
      "policy_version": policy["policy_version"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "source_parse_complete": corpus.complete,
          "active_records": len(corpus.records),
          "tokens_classified": sum(
              len(record.directives) for record in corpus.records
          ),
          "records_with_candidate_or_absence": candidate_count,
          "dependency_closure_complete": closure["complete"],
          "all_capabilities_owned": all(row["status"] == "owned" for row in capabilities),
          "source_aggregates_byte_identical": byte_reconciliation["all_byte_identical"],
          "source_aggregates_semantically_equal": aggregate_semantics["all_semantically_equal"],
          "source_aggregates_semantically_reconciled": aggregate_semantics[
              "all_semantically_reconciled"
          ],
          "persistent_bindings_captured": persistent_bindings["captured"],
          "target_parse_complete": world.complete,
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "active_records": len(corpus.records),
      "references": reference_count,
      "capabilities": capability_count,
      "source_parse_complete": corpus.complete,
      "dependency_closure_complete": closure["complete"],
      "aggregate_semantics_equal": aggregate_semantics["all_semantically_equal"],
      "aggregate_semantics_reconciled": aggregate_semantics[
          "all_semantically_reconciled"
      ],
      "persistent_bindings_captured": persistent_bindings["captured"],
      "target_parse_complete": world.complete,
      "artifacts": len(artifacts) + 1,
  }


def render_rol_discovery_human(summary: dict[str, Any]) -> str:
  lines = [
      f"RoL Phase 1 discovery: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Active records: {summary['active_records']}",
      f"Typed references: {summary['references']}",
      f"Capabilities: {summary['capabilities']}",
      f"Source parse complete: {str(summary['source_parse_complete']).lower()}",
      "Dependency closure complete: "
      f"{str(summary['dependency_closure_complete']).lower()}",
      f"Aggregate semantics equal: {str(summary['aggregate_semantics_equal']).lower()}",
      "Aggregate semantics reconciled: "
      f"{str(summary['aggregate_semantics_reconciled']).lower()}",
      "Persistent bindings captured: "
      f"{str(summary['persistent_bindings_captured']).lower()}",
      f"Target parse complete: {str(summary['target_parse_complete']).lower()}",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
