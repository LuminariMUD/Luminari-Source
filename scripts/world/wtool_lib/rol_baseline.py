"""Phase 0 baseline evidence for Realms of Luminari reconciliation."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
from typing import Any

from .config import resolve_config
from .constants import load_manifest
from .indexes import DATA_EXTENSIONS, is_safe_path_component, normalized_root_label
from .models import TOOL_VERSION
from .reporting import result_payload
from .rol_inventory import KIND_MANIFEST, SOURCE_KINDS, build_rol_inventory
from .world import validate_indexed_world


ROL_BASELINE_SCHEMA_VERSION = 1
_POLICY_NAME = "rol_conversion_policy.json"
_INTEGER_TOKEN = re.compile(rb"(?<![A-Za-z0-9_])-?\d+(?![A-Za-z0-9_])")
_NUMERIC_TYPES = frozenset(
    {
        "bigint",
        "decimal",
        "double",
        "float",
        "int",
        "mediumint",
        "numeric",
        "real",
        "smallint",
        "tinyint",
    }
)


class RolBaselineError(ValueError):
  """Raised when a baseline cannot be reproduced safely."""


def _sha256_bytes(data: bytes) -> str:
  return hashlib.sha256(data).hexdigest()


def _sha256_path(path: Path) -> str:
  digest = hashlib.sha256()
  with path.open("rb") as source:
    while chunk := source.read(1024 * 1024):
      digest.update(chunk)
  return digest.hexdigest()


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode("ascii")


def _git_revision(root: Path) -> str | None:
  completed = subprocess.run(
      ["git", "-C", str(root), "rev-parse", "HEAD"],
      check=False,
      capture_output=True,
      text=True,
  )
  if completed.returncode != 0:
    return None
  revision = completed.stdout.strip()
  return revision if re.fullmatch(r"[0-9a-fA-F]{40}", revision) else None


def _index_tokens(path: Path) -> list[tuple[str, int]]:
  tokens: list[tuple[str, int]] = []
  for line_number, line in enumerate(path.read_bytes().splitlines(), start=1):
    for token in re.findall(br"\S+", line):
      tokens.append((token.decode("utf-8", errors="surrogateescape"), line_number))
  return tokens


def build_target_inventory(world_root: Path, repo_root: Path) -> dict[str, Any]:
  """Hash every target index and data path and classify index membership."""

  world_root = world_root.resolve()
  if not world_root.is_dir():
    raise RolBaselineError(f"target world root is inaccessible: {world_root}")

  indexes: list[dict[str, Any]] = []
  files: list[dict[str, Any]] = []
  kind_summaries: list[dict[str, Any]] = []
  for kind in DATA_EXTENSIONS:
    directory = world_root / kind
    indexed_names: dict[str, int] = {}
    missing: list[dict[str, Any]] = []
    for index_name in ("index", "index.mini"):
      index_path = directory / index_name
      record: dict[str, Any] = {
          "kind": kind,
          "path": f"{kind}/{index_name}",
          "present": index_path.is_file(),
      }
      if index_path.is_file():
        data = index_path.read_bytes()
        record.update({"byte_size": len(data), "sha256": _sha256_bytes(data)})
        entries: list[str] = []
        terminated = False
        for token, line_number in _index_tokens(index_path):
          if token.startswith("$"):
            terminated = token == "$"
            break
          entries.append(token)
          if index_name == "index" and is_safe_path_component(token):
            indexed_names.setdefault(token, line_number)
            if not (directory / token).is_file():
              missing.append({"name": token, "index_line": line_number})
        record.update({"entries": entries, "terminated": terminated})
      indexes.append(record)

    kind_files: list[dict[str, Any]] = []
    if directory.is_dir():
      for path in sorted(directory.iterdir(), key=lambda candidate: candidate.name):
        if not path.is_file() or path.suffix != f".{kind}":
          continue
        data_record = {
            "kind": kind,
            "path": f"{kind}/{path.name}",
            "byte_size": path.stat().st_size,
            "sha256": _sha256_path(path),
            "index_line": indexed_names.get(path.name),
            "status": "indexed" if path.name in indexed_names else "orphan",
        }
        files.append(data_record)
        kind_files.append(data_record)

    kind_summaries.append(
        {
            "kind": kind,
            "files": len(kind_files),
            "indexed": sum(record["status"] == "indexed" for record in kind_files),
            "missing": len(missing),
            "missing_entries": missing,
            "orphaned": sum(record["status"] == "orphan" for record in kind_files),
        }
    )

  return {
      "schema_version": ROL_BASELINE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "root": normalized_root_label(world_root, repo_root),
      "indexes": indexes,
      "files": files,
      "summary": {
          "files": len(files),
          "indexed": sum(record["status"] == "indexed" for record in files),
          "orphaned": sum(record["status"] == "orphan" for record in files),
          "missing": sum(summary["missing"] for summary in kind_summaries),
          "kinds": kind_summaries,
      },
  }


def _manifest_order(source_inventory: dict[str, Any], kind: str) -> list[str]:
  manifest_name = KIND_MANIFEST[kind]
  manifest = next(
      item for item in source_inventory["manifests"] if item["name"] == manifest_name
  )
  return [entry["basename"] for entry in manifest["active"]]


def _source_build_bytes(path: Path, kind: str) -> tuple[bytes, bytes | None]:
  """Mirror build_areas.c, including its unterminated-final-line drop."""

  lines = path.read_bytes().splitlines(keepends=True)
  dropped_tail = None
  if lines and not lines[-1].endswith(b"\n"):
    dropped_tail = lines.pop()
  if kind in {"wld", "mob"} and lines:
    lines = lines[1:]
  return b"".join(lines), dropped_tail


def reconcile_source_aggregates(
    source_root: Path,
    source_inventory: dict[str, Any],
) -> dict[str, Any]:
  """Rebuild each source aggregate in memory and compare its exact bytes."""

  areas_root = source_root.resolve() / "areas"
  results: list[dict[str, Any]] = []
  for kind in SOURCE_KINDS:
    chunks: list[bytes] = []
    selected_paths: list[str] = []
    missing: list[str] = []
    dropped: list[dict[str, Any]] = []
    for basename in _manifest_order(source_inventory, kind):
      path = areas_root / kind / f"{basename}.{kind}"
      if not path.is_file():
        missing.append(f"areas/{kind}/{basename}.{kind}")
        continue
      data, dropped_tail = _source_build_bytes(path, kind)
      chunks.append(data)
      selected_paths.append(f"areas/{kind}/{basename}.{kind}")
      if dropped_tail is not None:
        dropped.append(
            {
                "path": f"areas/{kind}/{basename}.{kind}",
                "byte_size": len(dropped_tail),
                "sha256": _sha256_bytes(dropped_tail),
            }
        )

    assembled = b"".join(chunks)
    aggregate_path = areas_root / f"world.{kind}"
    aggregate = aggregate_path.read_bytes() if aggregate_path.is_file() else None
    results.append(
        {
            "kind": kind,
            "manifest": KIND_MANIFEST[kind],
            "selected_paths": selected_paths,
            "missing_selected_paths": missing,
            "dropped_unterminated_tails": dropped,
            "assembled_byte_size": len(assembled),
            "assembled_sha256": _sha256_bytes(assembled),
            "aggregate_path": f"areas/world.{kind}",
            "aggregate_present": aggregate is not None,
            "aggregate_byte_size": len(aggregate) if aggregate is not None else None,
            "aggregate_sha256": _sha256_bytes(aggregate) if aggregate is not None else None,
            "byte_identical": aggregate == assembled if aggregate is not None else False,
        }
    )

  return {
      "schema_version": ROL_BASELINE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "source_runtime_evidence": "EXAMPLE/RealmsOfLuminari/src/build_areas.c",
      "kinds": results,
      "complete": all(result["aggregate_present"] for result in results),
      "all_byte_identical": all(result["byte_identical"] for result in results),
  }


def load_rol_policy(repo_root: Path) -> dict[str, Any]:
  path = repo_root / "scripts/world" / _POLICY_NAME
  try:
    policy = json.loads(path.read_text(encoding="ascii"))
  except (OSError, UnicodeError, json.JSONDecodeError) as error:
    raise RolBaselineError(f"cannot load versioned RoL conversion policy: {error}") from error
  if policy.get("policy_version") != "rol-conversion-policy-1":
    raise RolBaselineError("unsupported RoL conversion policy version")
  return policy


def _scan_hardcoded_range(
    repo_root: Path,
    start: int,
    end: int,
    namespace: str,
) -> list[dict[str, Any]]:
  matches: list[dict[str, Any]] = []
  roots = (repo_root / "src",)
  suffixes = frozenset({".c", ".h"})
  for root in roots:
    for path in sorted(candidate for candidate in root.rglob("*") if candidate.is_file()):
      if path.suffix not in suffixes:
        continue
      for line_number, line in enumerate(path.read_bytes().splitlines(), start=1):
        lowered = line.lower()
        if b"vnum" not in lowered and not re.search(
            br"(?:assign(?:room|mob|obj)|real_(?:room|mobile|object))\s*\(",
            lowered,
        ):
          continue
        if namespace == "zone" and b"zone" not in lowered:
          continue
        for token in _INTEGER_TOKEN.findall(line):
          value = int(token)
          if start <= value <= end:
            matches.append(
                {
                    "path": path.relative_to(repo_root).as_posix(),
                    "line": line_number,
                    "value": value,
                }
            )
  return matches


def _parse_mysql_config(path: Path) -> dict[str, str]:
  values: dict[str, str] = {}
  for raw_line in path.read_text(encoding="utf-8").splitlines():
    line = raw_line.strip()
    if not line or line.startswith("#") or "=" not in line:
      continue
    key, value = line.split("=", 1)
    values[key.strip()] = value.strip()
  required = ("mysql_host", "mysql_database", "mysql_username", "mysql_password")
  missing = [key for key in required if not values.get(key)]
  if missing:
    raise RolBaselineError(f"database configuration is missing {', '.join(missing)}")
  return values


def _run_mysql(config: dict[str, str], query: str) -> str:
  environment = dict(os.environ)
  environment["MYSQL_PWD"] = config["mysql_password"]
  completed = subprocess.run(
      [
          "mysql",
          "--batch",
          "--skip-column-names",
          "--host",
          config["mysql_host"],
          "--user",
          config["mysql_username"],
          config["mysql_database"],
          "--execute",
          query,
      ],
      check=False,
      capture_output=True,
      text=True,
      env=environment,
  )
  if completed.returncode != 0:
    message = completed.stderr.strip().splitlines()
    detail = message[-1] if message else "database client failed"
    raise RolBaselineError(f"database collision query failed: {detail}")
  return completed.stdout


def _database_collision_evidence(
    config_path: Path,
    ranges: dict[str, tuple[int, int]],
) -> dict[str, Any]:
  config = _parse_mysql_config(config_path)
  columns_query = (
      "SELECT TABLE_NAME, COLUMN_NAME, DATA_TYPE FROM information_schema.COLUMNS "
      "WHERE TABLE_SCHEMA = DATABASE() AND COLUMN_NAME LIKE '%vnum%' "
      "ORDER BY TABLE_NAME, ORDINAL_POSITION"
  )
  columns: list[tuple[str, str, str]] = []
  for line in _run_mysql(config, columns_query).splitlines():
    fields = line.split("\t")
    if len(fields) == 3 and fields[2].lower() in _NUMERIC_TYPES:
      if all(re.fullmatch(r"[A-Za-z0-9_$]+", field) for field in fields[:2]):
        columns.append((fields[0], fields[1], fields[2].lower()))

  range_results: list[dict[str, Any]] = []
  for range_name, (start, end) in ranges.items():
    collisions: list[dict[str, Any]] = []
    for table, column, data_type in columns:
      if range_name == "new_zone_range" and "zone" not in column.lower():
        continue
      query = (
          f"SELECT COUNT(*) FROM `{table}` WHERE `{column}` BETWEEN {start} AND {end}"
      )
      raw_count = _run_mysql(config, query).strip()
      count = int(raw_count or "0")
      if count:
        collisions.append(
            {
                "table": table,
                "column": column,
                "data_type": data_type,
                "rows": count,
            }
        )
    range_results.append(
        {
            "range": range_name,
            "start": start,
            "end": end,
            "collisions": collisions,
            "collision_rows": sum(item["rows"] for item in collisions),
        }
    )

  identity = f"{config['mysql_host']}/{config['mysql_database']}".encode("utf-8")
  return {
      "captured": True,
      "database_identity_sha256": _sha256_bytes(identity),
      "numeric_vnum_columns": len(columns),
      "ranges": range_results,
  }


def build_collision_evidence(
    repo_root: Path,
    world_root: Path,
    policy: dict[str, Any],
    database_config: Path | None = None,
) -> dict[str, Any]:
  """Check the reserved ranges in indexed world, source, config, and database stores."""

  identity = policy["identity"]
  ranges = {
      "new_entity_range": (
          identity["new_entity_range"]["start"],
          identity["new_entity_range"]["end"],
      ),
      "new_zone_range": (
          identity["new_zone_range"]["start"],
          identity["new_zone_range"]["end"],
      ),
  }
  world_matches: dict[str, list[dict[str, Any]]] = {name: [] for name in ranges}
  world_kinds = {
      "new_entity_range": frozenset({"wld", "mob", "obj", "shp", "hlq"}),
      "new_zone_range": frozenset({"zon"}),
  }
  for kind in DATA_EXTENSIONS:
    directory = world_root / kind
    if not directory.is_dir():
      continue
    for path in sorted(candidate for candidate in directory.iterdir() if candidate.is_file()):
      if path.suffix != f".{kind}":
        continue
      for line_number, line in enumerate(path.read_bytes().splitlines(), start=1):
        if not line.startswith(b"#"):
          continue
        match = re.match(br"#(-?\d+)", line)
        if match is None:
          continue
        value = int(match.group(1))
        for range_name, (start, end) in ranges.items():
          if kind in world_kinds[range_name] and start <= value <= end:
            world_matches[range_name].append(
                {"kind": kind, "path": f"{kind}/{path.name}", "line": line_number, "value": value}
            )

  hardcoded = {
      name: _scan_hardcoded_range(
          repo_root,
          start,
          end,
          "zone" if name == "new_zone_range" else "entity",
      )
      for name, (start, end) in ranges.items()
  }
  if database_config is None:
    database = {"captured": False, "reason": "database configuration not requested"}
  else:
    database = _database_collision_evidence(database_config, ranges)

  range_summary = []
  database_by_name = {
      result["range"]: result for result in database.get("ranges", [])
  }
  for name, (start, end) in ranges.items():
    database_rows = database_by_name.get(name, {}).get("collision_rows")
    range_summary.append(
        {
            "range": name,
            "start": start,
            "end": end,
            "world_definitions": len(world_matches[name]),
            "hardcoded_literals": len(hardcoded[name]),
            "database_rows": database_rows,
            "reserved": not world_matches[name]
            and not hardcoded[name]
            and database_rows == 0,
        }
    )

  return {
      "schema_version": ROL_BASELINE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "world_header_matches": world_matches,
      "hardcoded_literal_matches": hardcoded,
      "database": database,
      "ranges": range_summary,
      "complete": database.get("captured", False),
      "all_reserved": all(result["reserved"] for result in range_summary),
  }


def _created_at(value: str | None) -> str:
  if value is None:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
  try:
    parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
  except ValueError as error:
    raise RolBaselineError("--created-at must be an ISO-8601 timestamp") from error
  if parsed.tzinfo is None:
    raise RolBaselineError("--created-at must include a timezone")
  return parsed.astimezone(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def _artifact_record(path: str, data: bytes) -> dict[str, Any]:
  return {"path": path, "byte_size": len(data), "sha256": _sha256_bytes(data)}


def write_baseline_bundle(
    source_root: Path,
    world_root: Path,
    output_dir: Path,
    repo_root: Path,
    database_config: Path | None = None,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write a unique Phase 0 evidence bundle without modifying either input root."""

  source_root = source_root.resolve()
  world_root = world_root.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolBaselineError(f"baseline output directory already exists: {output_dir}")

  policy = load_rol_policy(repo_root)
  source_inventory = build_rol_inventory(source_root, repo_root)
  target_inventory = build_target_inventory(world_root, repo_root)
  aggregates = reconcile_source_aggregates(source_root, source_inventory)
  collision = build_collision_evidence(
      repo_root,
      world_root,
      policy,
      database_config=database_config,
  )
  validation = validate_indexed_world(
      world_root,
      repo_root,
      load_manifest(repo_root / "scripts/world/wtool_constants.json"),
      resolve_config(world_root, None),
  )
  baseline_validation = result_payload(validation)
  baseline_validation["command"] = (
      f"python3 scripts/world/wtool.py --world-root {target_inventory['root']} "
      "--json validate --all"
  )

  payloads: dict[str, Any] = {
      "source-inventory.json": source_inventory,
      "target-inventory.json": target_inventory,
      "source-aggregate-reconciliation.json": aggregates,
      "collision-evidence.json": collision,
      "policies.json": policy,
      "validation/baseline.json": baseline_validation,
  }
  output_dir.mkdir(parents=True)
  (output_dir / "validation").mkdir()
  artifacts: list[dict[str, Any]] = []
  for relative_path, payload in payloads.items():
    data = _canonical_json(payload)
    (output_dir / relative_path).write_bytes(data)
    artifacts.append(_artifact_record(relative_path, data))

  repo_revision = _git_revision(repo_root)
  source_revision = _git_revision(source_root)
  seed_fields = [record["sha256"] for record in artifacts]
  seed_fields.extend(
      (repo_revision or "unversioned-target", source_revision or "unversioned-source")
  )
  seed = "\n".join(seed_fields).encode("ascii")
  run_id = f"rol-phase0-{_sha256_bytes(seed)[:16]}"
  run_manifest = {
      "schema_version": ROL_BASELINE_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": 0,
      "repo_revision": repo_revision,
      "source_revision": source_revision,
      "source_root": normalized_root_label(source_root, repo_root),
      "target_root": normalized_root_label(world_root, repo_root),
      "policy_version": policy["policy_version"],
      "artifacts": artifacts,
      "acceptance": {
          "source_aggregates_match": aggregates["all_byte_identical"],
          "candidate_ranges_reserved": collision["all_reserved"],
          "collision_evidence_complete": collision["complete"],
          "target_parse_complete": baseline_validation["complete"],
          "baseline_findings": baseline_validation["summary"]["active"],
      },
  }
  manifest_data = _canonical_json(run_manifest)
  (output_dir / "run-manifest.json").write_bytes(manifest_data)

  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      "source_aggregates_match": aggregates["all_byte_identical"],
      "candidate_ranges_reserved": collision["all_reserved"],
      "collision_evidence_complete": collision["complete"],
      "target_parse_complete": baseline_validation["complete"],
      "findings": baseline_validation["summary"]["active"],
      "artifacts": len(artifacts) + 1,
  }


def render_rol_baseline_human(summary: dict[str, Any]) -> str:
  counts = summary["findings"]["by_severity"]
  lines = [
      f"RoL Phase 0 baseline: {summary['run_id']}",
      f"Output: {summary['output_dir']}",
      f"Source aggregates byte-identical: {str(summary['source_aggregates_match']).lower()}",
      f"Candidate ranges reserved: {str(summary['candidate_ranges_reserved']).lower()}",
      f"Database collision evidence captured: {str(summary['collision_evidence_complete']).lower()}",
      f"Target parse complete: {str(summary['target_parse_complete']).lower()}",
      f"Baseline findings: {counts['error']} error, {counts['warning']} warning, "
      f"{counts['info']} info",
      f"Artifacts written: {summary['artifacts']}",
  ]
  return "\n".join(lines) + "\n"
