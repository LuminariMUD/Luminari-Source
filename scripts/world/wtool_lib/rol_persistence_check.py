"""Read-only persisted-VNUM validation for RoL release candidates."""

from __future__ import annotations

from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from typing import Any

from .models import WorldData
from .rol_persistence import PERSISTENT_BINDINGS


ROL_PERSISTENCE_CHECK_SCHEMA_VERSION = 1
_SAFE_IDENTIFIER = re.compile(r"[A-Za-z0-9_$]+")
_WRITE_SQL = re.compile(
    r"\b(?:ALTER|CALL|CREATE|DELETE|DO|DROP|GRANT|HANDLER|INSERT|INTO|LOAD|LOCK|"
    r"RENAME|REPLACE|REVOKE|SET|TRUNCATE|UNLOCK|UPDATE)\b",
    re.IGNORECASE,
)
_SIDE_EFFECT_FUNCTION = re.compile(
    r"\b(?:BENCHMARK|GET_LOCK|RELEASE_LOCK|SLEEP)\s*\(", re.IGNORECASE
)
_ROL_RANGES = {
    "zone": (20_000, 29_999),
    "room": (2_000_000, 2_999_999),
    "mobile": (2_000_000, 2_999_999),
    "object": (2_000_000, 2_999_999),
    "quest": (2_000_000, 2_999_999),
    "hlquest": (2_000_000, 2_999_999),
    "trigger": (2_000_000, 2_999_999),
    "shop": (2_000_000, 2_999_999),
}


class RolPersistenceCheckError(ValueError):
  """Raised when the local development persistence audit cannot be completed."""


def _parse_key_values(path: Path) -> dict[str, str]:
  values: dict[str, str] = {}
  for raw_line in path.read_text(encoding="utf-8").splitlines():
    line = raw_line.strip()
    if not line or line.startswith("#") or "=" not in line:
      continue
    key, value = line.split("=", 1)
    values[key.strip()] = value.strip().strip('"').strip("'")
  return values


def _development_database_config(
    repo_root: Path,
    development_lib_root: Path | None = None,
) -> tuple[Path, dict[str, str]]:
  lib_root = (
      (repo_root / "lib").resolve()
      if development_lib_root is None
      else development_lib_root.resolve()
  )
  environment_path = lib_root / ".env"
  config_path = lib_root / "mysql_config"
  if not environment_path.is_file():
    raise RolPersistenceCheckError("lib/.env is required for the persistence check")
  if _parse_key_values(environment_path).get("APP_ENV") != "development":
    raise RolPersistenceCheckError(
        "the RoL persistence check is restricted to the local development environment"
    )
  if not config_path.is_file():
    raise RolPersistenceCheckError("lib/mysql_config is required for the persistence check")
  config = _parse_key_values(config_path)
  required = ("mysql_host", "mysql_database", "mysql_username", "mysql_password")
  missing = [key for key in required if not config.get(key)]
  if missing:
    raise RolPersistenceCheckError(
        f"lib/mysql_config is missing {', '.join(missing)}"
    )
  return config_path, config


def _run_read_only_query(config: dict[str, str], query: str) -> str:
  normalized = query.strip()
  if (
      ";" in normalized
      or "--" in normalized
      or "/*" in normalized
      or "*/" in normalized
      or not normalized.upper().startswith(("SELECT ", "SHOW "))
      or _WRITE_SQL.search(normalized) is not None
      or _SIDE_EFFECT_FUNCTION.search(normalized) is not None
  ):
    raise RolPersistenceCheckError("persistence audit attempted a non-read-only query")
  executable = shutil.which("mariadb") or shutil.which("mysql")
  if executable is None:
    raise RolPersistenceCheckError("MariaDB client executable is required")
  environment = dict(os.environ)
  environment["MYSQL_PWD"] = config["mysql_password"]
  connection_arguments = ["--host", config["mysql_host"]]
  if config.get("mysql_socket"):
    connection_arguments = [
        "--protocol=socket",
        "--socket",
        config["mysql_socket"],
    ]
  elif config.get("mysql_port"):
    connection_arguments.extend(["--port", config["mysql_port"]])
  completed = subprocess.run(
      [
          executable,
          "--batch",
          "--skip-column-names",
          "--init-command=SET SESSION TRANSACTION READ ONLY",
          *connection_arguments,
          "--user",
          config["mysql_username"],
          config["mysql_database"],
          "--execute",
          normalized,
      ],
      check=False,
      capture_output=True,
      text=True,
      env=environment,
  )
  if completed.returncode != 0:
    message = completed.stderr.strip().splitlines()
    detail = message[-1] if message else "database query failed"
    raise RolPersistenceCheckError(f"persistence audit query failed: {detail}")
  return completed.stdout


def _database_schema(config: dict[str, str]) -> set[tuple[str, str]]:
  query = (
      "SELECT TABLE_NAME,COLUMN_NAME FROM information_schema.COLUMNS "
      "WHERE TABLE_SCHEMA=DATABASE() ORDER BY TABLE_NAME,ORDINAL_POSITION"
  )
  schema: set[tuple[str, str]] = set()
  for line in _run_read_only_query(config, query).splitlines():
    fields = line.split("\t")
    if len(fields) != 2 or any(_SAFE_IDENTIFIER.fullmatch(field) is None for field in fields):
      raise RolPersistenceCheckError("database schema returned an unsafe identifier")
    schema.add((fields[0], fields[1]))
  return schema


def _query_expression(binding: dict[str, Any]) -> tuple[str, str] | None:
  column = str(binding["column"])
  encoding = str(binding["encoding"])
  scope = str(binding["predicate"]) if binding.get("predicate") else "TRUE"
  if encoding == "integer":
    return f"`{column}`", scope
  if encoding == "integer_text":
    return f"CAST(`{column}` AS SIGNED)", f"({scope}) AND `{column}` REGEXP '^[0-9]+$'"
  if encoding == "object_header_blob":
    expression = (
        f"CAST(SUBSTRING(SUBSTRING_INDEX(CAST(`{column}` AS CHAR),CHAR(10),1),2) "
        "AS UNSIGNED)"
    )
    header_scope = (
        f"({scope}) AND LEFT(CAST(`{column}` AS CHAR),1)='#' "
        f"AND LOCATE(CHAR(10),`{column}`)>0"
    )
    return expression, header_scope
  return None


def _binding_values(
    config: dict[str, str], binding: dict[str, Any]
) -> Counter[int]:
  table = str(binding["table"])
  column = str(binding["column"])
  record_type = str(binding["record_type"])
  low, high = _ROL_RANGES[record_type]
  expression_and_scope = _query_expression(binding)
  if expression_and_scope is not None:
    expression, scope = expression_and_scope
    query = (
        f"SELECT {expression},COUNT(*) FROM `{table}` WHERE ({scope}) "
        f"AND {expression} BETWEEN {low} AND {high} GROUP BY {expression} ORDER BY 1"
    )
    counts: Counter[int] = Counter()
    for line in _run_read_only_query(config, query).splitlines():
      fields = line.split("\t")
      if len(fields) != 2:
        raise RolPersistenceCheckError("persistence query returned a malformed count row")
      counts[int(fields[0])] += int(fields[1])
    return counts

  if str(binding["encoding"]) not in {"csv_integer", "vessel_connections"}:
    raise RolPersistenceCheckError(
        f"unsupported persistence encoding {binding['encoding']}"
    )
  scope = str(binding["predicate"]) if binding.get("predicate") else "TRUE"
  query = f"SELECT `{column}` FROM `{table}` WHERE ({scope}) AND `{column}` IS NOT NULL"
  counts = Counter()
  for line in _run_read_only_query(config, query).splitlines():
    for token in re.findall(r"(?<![0-9])[0-9]+(?![0-9])", line):
      value = int(token)
      if low <= value <= high:
        counts[value] += 1
  return counts


def _definition_counts(world: WorldData) -> dict[str, Counter[int]]:
  return {
      record_type: Counter(record.vnum for record in world.records(record_type))
      for record_type in _ROL_RANGES
  }


def audit_development_persistence(
    world: WorldData,
    repo_root: Path,
    development_lib_root: Path | None = None,
) -> dict[str, Any]:
  """Check persisted RoL VNUMs against a candidate using the local development DB."""

  repo_root = repo_root.resolve()
  config_path, config = _development_database_config(
      repo_root, development_lib_root
  )
  schema = _database_schema(config)
  definitions = _definition_counts(world)
  references: list[dict[str, Any]] = []
  present_bindings = 0
  for binding in PERSISTENT_BINDINGS:
    record_type = str(binding["record_type"])
    if record_type not in _ROL_RANGES:
      continue
    table = str(binding["table"])
    column = str(binding["column"])
    if _SAFE_IDENTIFIER.fullmatch(table) is None or _SAFE_IDENTIFIER.fullmatch(column) is None:
      raise RolPersistenceCheckError("persistent binding contains an unsafe identifier")
    if (table, column) not in schema:
      continue
    present_bindings += 1
    for vnum, rows in sorted(_binding_values(config, binding).items()):
      definition_count = definitions[record_type][vnum]
      references.append(
          {
              "table": table,
              "column": column,
              "record_type": record_type,
              "vnum": vnum,
              "database_rows": rows,
              "candidate_definitions": definition_count,
              "status": "unique" if definition_count == 1 else (
                  "missing" if definition_count == 0 else "duplicate"
              ),
          }
      )
  missing = sum(row["status"] == "missing" for row in references)
  duplicate = sum(row["status"] == "duplicate" for row in references)
  identity = f"{config['mysql_host']}/{config['mysql_database']}".encode("utf-8")
  try:
    configuration = config_path.relative_to(repo_root).as_posix()
    configuration_scope = "repository"
  except ValueError:
    configuration = "external-development-lib/mysql_config"
    configuration_scope = "explicit-development-lib-root"
  result = {
      "schema_version": ROL_PERSISTENCE_CHECK_SCHEMA_VERSION,
      "mode": "read-only-local-development",
      "configuration": configuration,
      "configuration_scope": configuration_scope,
      "database_identity_sha256": hashlib.sha256(identity).hexdigest(),
      "tracked_bindings": sum(
          str(binding["record_type"]) in _ROL_RANGES
          for binding in PERSISTENT_BINDINGS
      ),
      "present_bindings": present_bindings,
      "references": references,
      "summary": {
          "distinct_persisted_rol_vnums": len(references),
          "database_rows": sum(int(row["database_rows"]) for row in references),
          "unique_candidate_definitions": sum(
              row["status"] == "unique" for row in references
          ),
          "missing_candidate_definitions": missing,
          "duplicate_candidate_definitions": duplicate,
          "pass": missing == 0 and duplicate == 0,
      },
  }
  return result


def render_persistence_check_human(result: dict[str, Any]) -> str:
  summary = result["summary"]
  return (
      "RoL persisted-VNUM check\n"
      f"Mode: {result['mode']}\n"
      f"Configuration: {result['configuration']}\n"
      f"Persisted RoL VNUMs: {summary['distinct_persisted_rol_vnums']}\n"
      f"Database rows: {summary['database_rows']}\n"
      f"Missing candidate definitions: {summary['missing_candidate_definitions']}\n"
      f"Duplicate candidate definitions: {summary['duplicate_candidate_definitions']}\n"
      f"Pass: {str(summary['pass']).lower()}\n"
  )
