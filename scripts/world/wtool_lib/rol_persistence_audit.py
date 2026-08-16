"""Sealed Phase 6.5 persistent-consumer migration and execution evidence."""

from __future__ import annotations

from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
from typing import Any

from .models import TOOL_VERSION
from .rol_baseline import _parse_mysql_config, _run_mysql
from .rol_persistence import persistent_consumer_ledger
from .rol_planner import verify_discovery_bundle
from .rol_rebase import (
    _ARTIFACT_REHOMES,
    _database_preflight_sql,
    _database_sql,
    _database_updates,
    _development_environment,
)


ROL_PERSISTENCE_AUDIT_SCHEMA_VERSION = 1
_SAFE_IDENTIFIER = re.compile(r"[A-Za-z0-9_$]+")


class RolPersistenceAuditError(ValueError):
  """Raised when persistent migration evidence is incomplete or inconsistent."""


def _canonical_json(data: Any) -> bytes:
  return (json.dumps(data, ensure_ascii=True, indent=2, sort_keys=True) + "\n").encode(
      "ascii"
  )


def _sha256_path(path: Path) -> str:
  return hashlib.sha256(path.read_bytes()).hexdigest()


def _created_at(value: str | None) -> str:
  if value is not None:
    return value
  return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def _write_jsonl(path: Path, rows: list[dict[str, Any]]) -> None:
  path.write_bytes(
      b"".join(
          (json.dumps(row, ensure_ascii=True, sort_keys=True) + "\n").encode("ascii")
          for row in rows
      )
  )


def _artifact(path: Path, root: Path) -> dict[str, Any]:
  return {
      "path": path.relative_to(root).as_posix(),
      "byte_size": path.stat().st_size,
      "sha256": _sha256_path(path),
  }


def write_persistence_migration_bundle(
    discovery_dir: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Write a deterministic supplemental Phase 6.5 persistence bundle."""

  discovery_dir = discovery_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPersistenceAuditError(f"persistence output directory already exists: {output_dir}")
  discovery_manifest = verify_discovery_bundle(discovery_dir)
  bindings = json.loads((discovery_dir / "bindings.json").read_text(encoding="ascii"))
  columns = bindings["persistent_bindings"]["columns"]
  consumers = persistent_consumer_ledger(columns)
  unclassified = [
      row for row in consumers if row["classification_status"] == "unclassified"
  ]
  if unclassified:
    first = unclassified[0]
    raise RolPersistenceAuditError(
        f"persistent binding remains unclassified: {first['table']}.{first['column']}"
    )

  output_dir.mkdir(parents=True)
  ledger_path = output_dir / "persistent-consumer-ledger.jsonl"
  sql_path = output_dir / "rol_phase6_5_vnum_migration.sql"
  plan_path = output_dir / "migration-plan.json"
  _write_jsonl(ledger_path, consumers)
  sql = _database_sql(columns)
  sql_path.write_text(sql, encoding="ascii")
  plan = {
      "classified_consumers": len(consumers),
      "migration_consumers": sum(
          bool(row.get("migration_required")) for row in consumers
      ),
      "unclassified_consumers": 0,
      "statements": len(
          _database_updates(columns, include_guarded_missing=True)
      ),
      "transactional": sql.startswith("START TRANSACTION;\n")
      and sql.endswith("COMMIT;\n"),
      "preflight_uses_rollback": _database_preflight_sql(sql).endswith("ROLLBACK;\n"),
      "object_blob_consumers": sum(
          row["encoding"] == "object_header_blob" for row in consumers
      ),
      "repeat_application_contract": "zero retired rows and byte-identical database state",
  }
  plan_path.write_bytes(_canonical_json(plan))
  artifacts = [_artifact(path, output_dir) for path in (ledger_path, plan_path, sql_path)]
  seed = "\n".join(
      [discovery_manifest["run_id"]]
      + [row["sha256"] for row in sorted(artifacts, key=lambda item: item["path"])]
  ).encode("ascii")
  run_id = f"rol-phase6-5-persistence-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_PERSISTENCE_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5-persistence-completion",
      "discovery_run_id": discovery_manifest["run_id"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "complete": True,
          "all_consumers_classified": True,
          "unclassified_consumers": 0,
          "transactional": plan["transactional"],
          "preflight_uses_rollback": plan["preflight_uses_rollback"],
          "object_blob_consumers": plan["object_blob_consumers"],
      },
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(manifest))
  return {
      "run_id": run_id,
      "output_dir": output_dir.as_posix(),
      **plan,
  }


def write_persistence_recovery_bundle(
    migration_bundle_dir: Path,
    output_dir: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Seal the exact inverse of one accepted persistence migration."""

  migration_bundle_dir = migration_bundle_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPersistenceAuditError(
        f"persistence recovery output directory already exists: {output_dir}"
    )
  migration_manifest = _verify_persistence_bundle(migration_bundle_dir)
  consumers = [
      json.loads(line)
      for line in (migration_bundle_dir / "persistent-consumer-ledger.jsonl")
      .read_text(encoding="ascii")
      .splitlines()
  ]
  sql = _database_sql(consumers, reverse=True)
  statements = _database_updates(
      consumers, include_guarded_missing=True, reverse=True
  )

  output_dir.mkdir(parents=True)
  ledger_path = output_dir / "persistent-consumer-ledger.jsonl"
  sql_path = output_dir / "rol_phase6_5_vnum_recovery.sql"
  plan_path = output_dir / "recovery-plan.json"
  ledger_path.write_bytes(
      (migration_bundle_dir / "persistent-consumer-ledger.jsonl").read_bytes()
  )
  sql_path.write_text(sql, encoding="ascii")
  plan = {
      "direction": "canonical_to_retired",
      "exact_inverse_of_migration_run_id": migration_manifest["run_id"],
      "classified_consumers": len(consumers),
      "migration_consumers": sum(
          bool(row.get("migration_required")) for row in consumers
      ),
      "statements": len(statements),
      "transactional": sql.startswith("START TRANSACTION;\n")
      and sql.endswith("COMMIT;\n"),
      "preflight_uses_rollback": _database_preflight_sql(sql).endswith(
          "ROLLBACK;\n"
      ),
      "artifact_mapping_excludes_unmigrated_2001009": all(
          "2001009" not in statement for statement in statements
      ),
      "backup_required_before_apply": True,
      "repeat_application_contract": (
          "zero canonical migration rows and byte-identical database state"
      ),
  }
  plan_path.write_bytes(_canonical_json(plan))
  artifacts = [
      _artifact(path, output_dir) for path in (ledger_path, plan_path, sql_path)
  ]
  seed = "\n".join(
      [str(migration_manifest["run_id"])]
      + [row["sha256"] for row in sorted(artifacts, key=lambda item: item["path"])]
  ).encode("ascii")
  run_id = f"rol-phase6-5-persistence-recovery-{hashlib.sha256(seed).hexdigest()[:16]}"
  manifest = {
      "schema_version": ROL_PERSISTENCE_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": run_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5-persistence-recovery",
      "migration_run_id": migration_manifest["run_id"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": {
          "complete": True,
          "exact_inverse": True,
          "transactional": plan["transactional"],
          "preflight_uses_rollback": plan["preflight_uses_rollback"],
          "backup_required_before_apply": True,
          "artifact_mapping_excludes_unmigrated_2001009": plan[
              "artifact_mapping_excludes_unmigrated_2001009"
          ],
      },
  }
  (output_dir / "run-manifest.json").write_bytes(_canonical_json(manifest))
  return {"run_id": run_id, "output_dir": output_dir.as_posix(), **plan}


def _verify_persistence_bundle(bundle_dir: Path) -> dict[str, Any]:
  manifest_path = bundle_dir / "run-manifest.json"
  manifest = json.loads(manifest_path.read_text(encoding="ascii"))
  if manifest.get("phase") != "6.5-persistence-completion":
    raise RolPersistenceAuditError("not a Phase 6.5 persistence bundle")
  if not manifest.get("acceptance", {}).get("complete"):
    raise RolPersistenceAuditError("persistence bundle is not accepted")
  for artifact in manifest.get("artifacts", []):
    path = (bundle_dir / str(artifact["path"])).resolve()
    try:
      path.relative_to(bundle_dir)
    except ValueError as error:
      raise RolPersistenceAuditError("persistence artifact escapes bundle") from error
    if not path.is_file() or _sha256_path(path) != artifact["sha256"]:
      raise RolPersistenceAuditError(f"persistence artifact is missing or changed: {path}")
  return manifest


def _verify_persistence_recovery_bundle(bundle_dir: Path) -> dict[str, Any]:
  manifest_path = bundle_dir / "run-manifest.json"
  manifest = json.loads(manifest_path.read_text(encoding="ascii"))
  if manifest.get("phase") != "6.5-persistence-recovery":
    raise RolPersistenceAuditError("not a Phase 6.5 persistence recovery bundle")
  acceptance = manifest.get("acceptance", {})
  if not acceptance.get("complete") or not acceptance.get("exact_inverse"):
    raise RolPersistenceAuditError("persistence recovery bundle is not accepted")
  if not acceptance.get("backup_required_before_apply"):
    raise RolPersistenceAuditError("persistence recovery does not require a backup")
  for artifact in manifest.get("artifacts", []):
    path = (bundle_dir / str(artifact["path"])).resolve()
    try:
      path.relative_to(bundle_dir)
    except ValueError as error:
      raise RolPersistenceAuditError("persistence recovery artifact escapes bundle") from error
    if not path.is_file() or _sha256_path(path) != artifact["sha256"]:
      raise RolPersistenceAuditError(
          f"persistence recovery artifact is missing or changed: {path}"
      )
  return manifest


def verify_persistence_recovery_execution(output_dir: Path) -> dict[str, Any]:
  """Verify sealed recovery execution evidence and its restorable backup."""

  output_dir = output_dir.resolve()
  manifest = json.loads((output_dir / "run-manifest.json").read_text(encoding="ascii"))
  if manifest.get("phase") != "6.5-persistence-recovery-execution":
    raise RolPersistenceAuditError(
        "not a Phase 6.5 persistence recovery execution"
    )
  acceptance = manifest.get("acceptance", {})
  required = (
      "complete",
      "rollback_preflight_no_op",
      "repeat_apply_no_op",
      "row_counts_preserved",
      "serialized_suffixes_preserved",
      "recovered_object_prototypes_resolve",
  )
  if not all(acceptance.get(key) is True for key in required):
    raise RolPersistenceAuditError("persistence recovery execution is not accepted")
  if acceptance.get("canonical_relevant_rows_after") != 0:
    raise RolPersistenceAuditError("persistence recovery left canonical rows")
  for artifact in manifest.get("artifacts", []):
    path = (output_dir / str(artifact["path"])).resolve()
    try:
      path.relative_to(output_dir)
    except ValueError as error:
      raise RolPersistenceAuditError("persistence recovery evidence escapes output") from error
    if not path.is_file() or _sha256_path(path) != artifact["sha256"]:
      raise RolPersistenceAuditError(
          f"persistence recovery evidence is missing or changed: {path}"
      )
  backup = manifest.get("database_backup", {})
  backup_path = output_dir / str(backup.get("path", ""))
  if (
      backup.get("path") != "database-before-recovery.sql"
      or not backup_path.is_file()
      or backup_path.stat().st_size <= 0
      or _sha256_path(backup_path) != backup.get("sha256")
  ):
    raise RolPersistenceAuditError("persistence recovery backup is missing or changed")
  return manifest


def _source_target_predicates(record_type: str, expression: str) -> tuple[str, str]:
  if record_type == "zone":
    return (
        f"{expression} IN (1507,1591,1960)",
        f"{expression} IN (20507,20591,20960)",
    )
  source = (
      f"({expression} BETWEEN 150700 AND 150899 OR "
      f"{expression} BETWEEN 159100 AND 159599 OR "
      f"{expression} BETWEEN 196000 AND 196299)"
  )
  target = (
      f"({expression} BETWEEN 2050700 AND 2050899 OR "
      f"{expression} BETWEEN 2059100 AND 2059599 OR "
      f"{expression} BETWEEN 2096000 AND 2096299)"
  )
  if record_type == "object":
    source += " OR " + expression + " IN (" + ",".join(
        str(value) for value in sorted(_ARTIFACT_REHOMES)
    ) + ")"
    target += " OR " + expression + " IN (" + ",".join(
        str(value) for value in sorted(_ARTIFACT_REHOMES.values())
    ) + ")"
  return f"({source})", f"({target})"


def _token_patterns(record_type: str, delimiters: str) -> tuple[str, str]:
  source_values = "150[78][0-9]{2}|159[1-5][0-9]{2}|196[0-2][0-9]{2}"
  target_values = "2050[78][0-9]{2}|2059[1-5][0-9]{2}|2096[0-2][0-9]{2}"
  if record_type == "object":
    source_values += "|" + "|".join(str(value) for value in sorted(_ARTIFACT_REHOMES))
    target_values += "|" + "|".join(
        str(value) for value in sorted(_ARTIFACT_REHOMES.values())
    )
  return (
      f"(^|[{delimiters}])({source_values})([{delimiters}]|$)",
      f"(^|[{delimiters}])({target_values})([{delimiters}]|$)",
  )


def _database_schema(config: dict[str, str]) -> dict[tuple[str, str], dict[str, str]]:
  query = (
      "SELECT c.TABLE_NAME,c.COLUMN_NAME,c.DATA_TYPE,t.TABLE_TYPE "
      "FROM information_schema.COLUMNS c JOIN information_schema.TABLES t "
      "ON t.TABLE_SCHEMA=c.TABLE_SCHEMA AND t.TABLE_NAME=c.TABLE_NAME "
      "WHERE c.TABLE_SCHEMA=DATABASE() ORDER BY c.TABLE_NAME,c.ORDINAL_POSITION"
  )
  result: dict[tuple[str, str], dict[str, str]] = {}
  for line in _run_mysql(config, query).splitlines():
    fields = line.split("\t")
    if len(fields) == 4:
      result[(fields[0], fields[1])] = {
          "data_type": fields[2].lower(),
          "table_type": fields[3],
      }
  return result


def _count_expression(condition: str) -> str:
  return f"COALESCE(SUM(CASE WHEN {condition} THEN 1 ELSE 0 END),0)"


def _indexed_object_definition_counts(lib_root: Path) -> Counter[int]:
  """Return header counts for every object in the active object index."""

  object_root = lib_root / "world/obj"
  index_path = object_root / "index"
  counts: Counter[int] = Counter()
  for entry in index_path.read_text(encoding="ascii").splitlines():
    if not entry or entry == "$":
      continue
    path = object_root / entry
    if not path.is_file():
      raise RolPersistenceAuditError(f"indexed object file is missing: {path}")
    for line in path.read_bytes().splitlines():
      if re.fullmatch(rb"#[0-9]+", line) is not None:
        counts[int(line[1:])] += 1
  return counts


def _persistent_object_prototype_resolution(
    config: dict[str, str],
    consumers: list[dict[str, Any]],
    schema: dict[tuple[str, str], dict[str, str]],
    lib_root: Path,
    *,
    namespace: str = "canonical",
) -> dict[str, Any]:
  """Prove saved-object headers in one migration namespace resolve uniquely."""

  if namespace not in {"canonical", "retired"}:
    raise RolPersistenceAuditError(f"unsupported object namespace: {namespace}")

  values: set[int] = set()
  for consumer in consumers:
    if (
        consumer.get("record_type") != "object"
        or not consumer.get("migration_required")
        or (str(consumer["table"]), str(consumer["column"])) not in schema
    ):
      continue
    table = str(consumer["table"])
    column = str(consumer["column"])
    encoding = str(consumer["encoding"])
    scope = str(consumer["predicate"]) if consumer.get("predicate") else "TRUE"
    if encoding == "integer":
      expression = f"`{column}`"
      value_scope = scope
    elif encoding == "integer_text":
      expression = f"CAST(`{column}` AS SIGNED)"
      value_scope = f"({scope}) AND (`{column}` REGEXP '^[0-9]+$')"
    elif encoding == "object_header_blob":
      expression = (
          f"CAST(SUBSTRING(SUBSTRING_INDEX(CAST(`{column}` AS CHAR),CHAR(10),1),2) "
          "AS UNSIGNED)"
      )
      value_scope = (
          f"({scope}) AND LEFT(CAST(`{column}` AS CHAR),1)='#' "
          f"AND LOCATE(CHAR(10),`{column}`)>0"
      )
    else:
      continue
    source, target = _source_target_predicates("object", expression)
    selected = target if namespace == "canonical" else source
    query = (
        f"SELECT DISTINCT {expression} FROM `{table}` "
        f"WHERE ({value_scope}) AND ({selected}) ORDER BY 1"
    )
    for value in _run_mysql(config, query).splitlines():
      if value:
        values.add(int(value))

  definitions = _indexed_object_definition_counts(lib_root)
  missing = sum(definitions[value] == 0 for value in values)
  nonunique = sum(definitions[value] > 1 for value in values)
  resolved = sum(definitions[value] == 1 for value in values)
  fingerprint = hashlib.sha256(
      "\n".join(str(value) for value in sorted(values)).encode("ascii")
  ).hexdigest()
  result = {
      "namespace": namespace,
      "referenced_object_vnums": len(values),
      "resolved_unique_object_vnums": resolved,
      "missing_object_prototypes": missing,
      "nonunique_object_prototypes": nonunique,
      "referenced_vnum_set_sha256": fingerprint,
  }
  if namespace == "canonical":
    result["referenced_canonical_object_vnums"] = len(values)
  else:
    result["referenced_retired_object_vnums"] = len(values)
  return result


def audit_persistent_database(
    config: dict[str, str],
    consumers: list[dict[str, Any]],
    lib_root: Path | None = None,
) -> dict[str, Any]:
  """Capture typed counts and blob-suffix fingerprints without row contents."""

  schema = _database_schema(config)
  rows: list[dict[str, Any]] = []
  for consumer in consumers:
    table = str(consumer["table"])
    column = str(consumer["column"])
    if _SAFE_IDENTIFIER.fullmatch(table) is None or _SAFE_IDENTIFIER.fullmatch(column) is None:
      raise RolPersistenceAuditError("consumer ledger contains an unsafe identifier")
    metadata = schema.get((table, column))
    row = {
        "table": table,
        "column": column,
        "record_type": consumer["record_type"],
        "encoding": consumer["encoding"],
        "migration_required": bool(consumer.get("migration_required")),
        "disposition": consumer["disposition"],
        "present": metadata is not None,
    }
    if metadata is None:
      rows.append(row)
      continue
    row.update(metadata)
    encoding = str(consumer["encoding"])
    scope = str(consumer["predicate"]) if consumer.get("predicate") else "TRUE"
    if encoding == "integer":
      expression = f"`{column}`"
      source, target = _source_target_predicates(str(consumer["record_type"]), expression)
      query = (
          f"SELECT COUNT(*),{_count_expression(scope)},"
          f"{_count_expression(f'({scope}) AND ({source})')},"
          f"{_count_expression(f'({scope}) AND ({target})')},"
          f"COUNT(DISTINCT CASE WHEN ({scope}) AND ({source}) THEN {expression} END),"
          f"COUNT(DISTINCT CASE WHEN ({scope}) AND ({target}) THEN {expression} END) "
          f"FROM `{table}`"
      )
      values = [int(value) for value in _run_mysql(config, query).split("\t")]
      row.update(
          dict(
              zip(
                  (
                      "table_rows",
                      "scoped_rows",
                      "retired_rows",
                      "canonical_rows",
                      "retired_distinct",
                      "canonical_distinct",
                  ),
                  values,
                  strict=True,
              )
          )
      )
    elif encoding == "integer_text":
      expression = f"CAST(`{column}` AS SIGNED)"
      numeric_scope = f"({scope}) AND (`{column}` REGEXP '^[0-9]+$')"
      source, target = _source_target_predicates(str(consumer["record_type"]), expression)
      query = (
          f"SELECT COUNT(*),{_count_expression(numeric_scope)},"
          f"{_count_expression(f'({numeric_scope}) AND ({source})')},"
          f"{_count_expression(f'({numeric_scope}) AND ({target})')} FROM `{table}`"
      )
      values = [int(value) for value in _run_mysql(config, query).split("\t")]
      row.update(
          dict(
              zip(
                  ("table_rows", "scoped_rows", "retired_rows", "canonical_rows"),
                  values,
                  strict=True,
              )
          )
      )
    elif encoding == "object_header_blob":
      expression = (
          f"CAST(SUBSTRING(SUBSTRING_INDEX(CAST(`{column}` AS CHAR),CHAR(10),1),2) "
          "AS UNSIGNED)"
      )
      header_scope = (
          f"({scope}) AND LEFT(CAST(`{column}` AS CHAR),1)='#' "
          f"AND LOCATE(CHAR(10),`{column}`)>0"
      )
      source, target = _source_target_predicates(str(consumer["record_type"]), expression)
      suffix = f"SUBSTRING(`{column}`,LOCATE(CHAR(10),`{column}`))"
      source_condition = f"({header_scope}) AND ({source})"
      target_condition = f"({header_scope}) AND ({target})"
      query = (
          f"SELECT COUNT(*),{_count_expression(header_scope)},"
          f"{_count_expression(source_condition)},{_count_expression(target_condition)},"
          f"COALESCE(SUM(CASE WHEN {source_condition} THEN OCTET_LENGTH({suffix}) ELSE 0 END),0),"
          f"COALESCE(SUM(CASE WHEN {target_condition} THEN OCTET_LENGTH({suffix}) ELSE 0 END),0),"
          f"COALESCE(BIT_XOR(CASE WHEN {source_condition} THEN CRC32({suffix}) ELSE 0 END),0),"
          f"COALESCE(BIT_XOR(CASE WHEN {target_condition} THEN CRC32({suffix}) ELSE 0 END),0) "
          f"FROM `{table}`"
      )
      values = [int(value) for value in _run_mysql(config, query).split("\t")]
      row.update(
          dict(
              zip(
                  (
                      "table_rows",
                      "scoped_rows",
                      "retired_rows",
                      "canonical_rows",
                      "retired_suffix_bytes",
                      "canonical_suffix_bytes",
                      "retired_suffix_crc32_xor",
                      "canonical_suffix_crc32_xor",
                  ),
                  values,
                  strict=True,
              )
          )
      )
    elif encoding in {"csv_integer", "vessel_connections"}:
      delimiters = "," if encoding == "csv_integer" else "|:"
      source_pattern, target_pattern = _token_patterns(
          str(consumer["record_type"]), delimiters
      )
      query = (
          f"SELECT COUNT(*),{_count_expression(f'`{column}` REGEXP {json.dumps(source_pattern)}')},"
          f"{_count_expression(f'`{column}` REGEXP {json.dumps(target_pattern)}')} "
          f"FROM `{table}`"
      )
      values = [int(value) for value in _run_mysql(config, query).split("\t")]
      row.update(
          dict(
              zip(
                  ("table_rows", "retired_rows", "canonical_rows"),
                  values,
                  strict=True,
              )
          )
      )
    else:
      raise RolPersistenceAuditError(f"unsupported audit encoding: {encoding}")
    rows.append(row)

  identity = f"{config['mysql_host']}/{config['mysql_database']}".encode("utf-8")
  result = {
      "database_identity_sha256": hashlib.sha256(identity).hexdigest(),
      "bindings": rows,
      "summary": {
          "classified_bindings": len(rows),
          "present_bindings": sum(row["present"] for row in rows),
          "migration_bindings_present": sum(
              row["present"] and row["migration_required"] for row in rows
          ),
          "retired_relevant_rows": sum(
              int(row.get("retired_rows", 0))
              for row in rows
              if row["migration_required"]
          ),
          "canonical_relevant_rows": sum(
              int(row.get("canonical_rows", 0))
              for row in rows
              if row["migration_required"]
          ),
          "guarded_disjoint_retired_rows": sum(
              int(row.get("retired_rows", 0))
              for row in rows
              if row["disposition"]
              in {"generated_namespace_disjoint", "classified_empty_orphan"}
          ),
      },
  }
  if lib_root is not None:
    result["object_prototype_resolution"] = _persistent_object_prototype_resolution(
        config, consumers, schema, lib_root
    )
    result[
        "retired_object_prototype_resolution"
    ] = _persistent_object_prototype_resolution(
        config, consumers, schema, lib_root, namespace="retired"
    )
  return result


def _audit_by_key(audit: dict[str, Any]) -> dict[tuple[str, str], dict[str, Any]]:
  return {
      (str(row["table"]), str(row["column"])): row for row in audit["bindings"]
  }


def _validate_execution(
    before: dict[str, Any],
    after_preflight: dict[str, Any],
    after: dict[str, Any],
    repeat: dict[str, Any],
) -> dict[str, Any]:
  failures: list[str] = []
  if before != after_preflight:
    failures.append("rollback preflight changed database state")
  if after != repeat:
    failures.append("second migration changed database state")
  if after["summary"]["retired_relevant_rows"] != 0:
    failures.append("retired rows remain in migration-required consumers")
  if after["summary"]["guarded_disjoint_retired_rows"] != 0:
    failures.append("retired-range values remain in guarded disjoint consumers")
  prototype_resolution = after.get("object_prototype_resolution", {})
  if prototype_resolution.get("missing_object_prototypes", 0) != 0:
    failures.append("canonical saved objects reference missing prototypes")
  if prototype_resolution.get("nonunique_object_prototypes", 0) != 0:
    failures.append("canonical saved objects reference nonunique prototypes")

  before_rows = _audit_by_key(before)
  after_rows = _audit_by_key(after)
  for key, prior in before_rows.items():
    current = after_rows[key]
    if not prior["present"]:
      continue
    if prior.get("table_rows") != current.get("table_rows"):
      failures.append(f"row count changed for {key[0]}.{key[1]}")
    if not prior["migration_required"]:
      continue
    expected = int(prior.get("canonical_rows", 0)) + int(prior.get("retired_rows", 0))
    if int(current.get("canonical_rows", 0)) != expected:
      failures.append(f"canonical row count mismatch for {key[0]}.{key[1]}")
    if prior["encoding"] == "object_header_blob":
      expected_bytes = int(prior["canonical_suffix_bytes"]) + int(
          prior["retired_suffix_bytes"]
      )
      if int(current["canonical_suffix_bytes"]) != expected_bytes:
        failures.append(f"serialized suffix length changed for {key[0]}.{key[1]}")
      expected_xor = int(prior["canonical_suffix_crc32_xor"]) ^ int(
          prior["retired_suffix_crc32_xor"]
      )
      if int(current["canonical_suffix_crc32_xor"]) != expected_xor:
        failures.append(f"serialized suffix checksum changed for {key[0]}.{key[1]}")
  return {
      "complete": not failures,
      "failures": failures,
      "rollback_preflight_no_op": before == after_preflight,
      "repeat_apply_no_op": after == repeat,
      "retired_relevant_rows_after": after["summary"]["retired_relevant_rows"],
      "guarded_disjoint_retired_rows_after": after["summary"][
          "guarded_disjoint_retired_rows"
      ],
      "row_counts_preserved": not any("row count" in failure for failure in failures),
      "serialized_suffixes_preserved": not any(
          "serialized suffix" in failure for failure in failures
      ),
      "canonical_object_prototypes_resolve": not any(
          "saved objects" in failure for failure in failures
      ),
  }


def _write_database_backup(config: dict[str, str], path: Path) -> None:
  """Write a restorable full database dump without exposing credentials."""

  executable = shutil.which("mariadb-dump") or shutil.which("mysqldump")
  if executable is None:
    raise RolPersistenceAuditError("mariadb-dump or mysqldump is required for recovery")
  if path.exists():
    raise RolPersistenceAuditError(f"database backup already exists: {path}")
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
          "--no-defaults",
          *connection_arguments,
          "--user",
          config["mysql_username"],
          "--single-transaction",
          "--quick",
          "--skip-lock-tables",
          "--hex-blob",
          "--skip-comments",
          "--skip-dump-date",
          "--databases",
          config["mysql_database"],
          f"--result-file={path}",
      ],
      check=False,
      capture_output=True,
      text=True,
      env=environment,
  )
  if completed.returncode != 0:
    path.unlink(missing_ok=True)
    message = completed.stderr.strip().splitlines()
    detail = message[-1] if message else "database dump failed"
    raise RolPersistenceAuditError(f"database backup failed: {detail}")
  os.chmod(path, 0o600)


def _validate_recovery_execution(
    before: dict[str, Any],
    after_preflight: dict[str, Any],
    after: dict[str, Any],
    repeat: dict[str, Any],
) -> dict[str, Any]:
  failures: list[str] = []
  if before != after_preflight:
    failures.append("rollback preflight changed database state")
  if after != repeat:
    failures.append("second recovery changed database state")
  if after["summary"]["canonical_relevant_rows"] != 0:
    failures.append("canonical migration rows remain after recovery")

  prototype_resolution = after.get("retired_object_prototype_resolution", {})
  if prototype_resolution.get("missing_object_prototypes", 0) != 0:
    failures.append("recovered saved objects reference missing Luminari prototypes")
  if prototype_resolution.get("nonunique_object_prototypes", 0) != 0:
    failures.append("recovered saved objects reference nonunique Luminari prototypes")

  before_rows = _audit_by_key(before)
  after_rows = _audit_by_key(after)
  for key, prior in before_rows.items():
    current = after_rows[key]
    if not prior["present"]:
      continue
    if prior.get("table_rows") != current.get("table_rows"):
      failures.append(f"row count changed for {key[0]}.{key[1]}")
    if not prior["migration_required"]:
      continue
    expected = int(prior.get("retired_rows", 0)) + int(
        prior.get("canonical_rows", 0)
    )
    if int(current.get("retired_rows", 0)) != expected:
      failures.append(f"recovered row count mismatch for {key[0]}.{key[1]}")
    if prior["encoding"] == "object_header_blob":
      expected_bytes = int(prior["retired_suffix_bytes"]) + int(
          prior["canonical_suffix_bytes"]
      )
      if int(current["retired_suffix_bytes"]) != expected_bytes:
        failures.append(f"serialized suffix length changed for {key[0]}.{key[1]}")
      expected_xor = int(prior["retired_suffix_crc32_xor"]) ^ int(
          prior["canonical_suffix_crc32_xor"]
      )
      if int(current["retired_suffix_crc32_xor"]) != expected_xor:
        failures.append(f"serialized suffix checksum changed for {key[0]}.{key[1]}")
  return {
      "complete": not failures,
      "failures": failures,
      "rollback_preflight_no_op": before == after_preflight,
      "repeat_apply_no_op": after == repeat,
      "canonical_relevant_rows_after": after["summary"][
          "canonical_relevant_rows"
      ],
      "retired_relevant_rows_after": after["summary"]["retired_relevant_rows"],
      "row_counts_preserved": not any("row count" in failure for failure in failures),
      "serialized_suffixes_preserved": not any(
          "serialized suffix" in failure for failure in failures
      ),
      "recovered_object_prototypes_resolve": not any(
          "recovered saved objects" in failure for failure in failures
      ),
  }


def apply_persistence_migration_bundle(
    bundle_dir: Path,
    database_config: Path,
    database_role: str,
    output_dir: Path,
    lib_root: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Apply, reapply, and record a persistent migration without exposing data."""

  bundle_dir = bundle_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPersistenceAuditError(f"execution output directory already exists: {output_dir}")
  if database_role not in {"isolated", "development"}:
    raise RolPersistenceAuditError("database role must be isolated or development")
  if database_role == "development":
    raise RolPersistenceAuditError(
        "forward persistence migration is disabled for development because it "
        "rehomes existing Luminari identities"
    )
  manifest = _verify_persistence_bundle(bundle_dir)
  consumers = [
      json.loads(line)
      for line in (bundle_dir / "persistent-consumer-ledger.jsonl")
      .read_text(encoding="ascii")
      .splitlines()
  ]
  config = _parse_mysql_config(database_config)
  sql = (bundle_dir / "rol_phase6_5_vnum_migration.sql").read_text(encoding="ascii")

  before = audit_persistent_database(config, consumers, lib_root)
  _run_mysql(config, _database_preflight_sql(sql))
  after_preflight = audit_persistent_database(config, consumers, lib_root)
  _run_mysql(config, sql)
  after = audit_persistent_database(config, consumers, lib_root)
  _run_mysql(config, sql)
  repeat = audit_persistent_database(config, consumers, lib_root)
  acceptance = _validate_execution(before, after_preflight, after, repeat)
  if not acceptance["complete"]:
    raise RolPersistenceAuditError("; ".join(acceptance["failures"]))

  output_dir.mkdir(parents=True)
  artifacts: list[dict[str, Any]] = []
  for name, payload in (
      ("before.json", before),
      ("after-preflight.json", after_preflight),
      ("after.json", after),
      ("repeat.json", repeat),
      ("acceptance.json", acceptance),
  ):
    path = output_dir / name
    path.write_bytes(_canonical_json(payload))
    artifacts.append(_artifact(path, output_dir))
  execution_seed = "\n".join(
      [manifest["run_id"], database_role]
      + [row["sha256"] for row in sorted(artifacts, key=lambda item: item["path"])]
  ).encode("ascii")
  execution_id = f"rol-phase6-5-persistence-exec-{hashlib.sha256(execution_seed).hexdigest()[:16]}"
  execution_manifest = {
      "schema_version": ROL_PERSISTENCE_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": execution_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5-persistence-execution",
      "database_role": database_role,
      "migration_run_id": manifest["run_id"],
      "database_identity_sha256": before["database_identity_sha256"],
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": acceptance,
  }
  manifest_path = output_dir / "run-manifest.json"
  manifest_path.write_bytes(_canonical_json(execution_manifest))
  return {
      "run_id": execution_id,
      "output_dir": output_dir.as_posix(),
      "database_role": database_role,
      "retired_rows_before": before["summary"]["retired_relevant_rows"],
      "retired_rows_after": after["summary"]["retired_relevant_rows"],
      **acceptance,
  }


def apply_persistence_recovery_bundle(
    bundle_dir: Path,
    database_config: Path,
    database_role: str,
    output_dir: Path,
    lib_root: Path,
    created_at: str | None = None,
) -> dict[str, Any]:
  """Back up, apply, reapply, and audit one sealed persistence recovery."""

  bundle_dir = bundle_dir.resolve()
  output_dir = output_dir.resolve()
  if output_dir.exists():
    raise RolPersistenceAuditError(
        f"recovery execution output directory already exists: {output_dir}"
    )
  if database_role not in {"isolated", "development"}:
    raise RolPersistenceAuditError("database role must be isolated or development")
  if database_role == "development":
    _development_environment(lib_root.resolve())
  manifest = _verify_persistence_recovery_bundle(bundle_dir)
  consumers = [
      json.loads(line)
      for line in (bundle_dir / "persistent-consumer-ledger.jsonl")
      .read_text(encoding="ascii")
      .splitlines()
  ]
  config = _parse_mysql_config(database_config)
  sql = (bundle_dir / "rol_phase6_5_vnum_recovery.sql").read_text(
      encoding="ascii"
  )

  before = audit_persistent_database(config, consumers, lib_root)
  output_dir.mkdir(parents=True)
  backup_path = output_dir / "database-before-recovery.sql"
  _write_database_backup(config, backup_path)
  (output_dir / "before.json").write_bytes(_canonical_json(before))

  _run_mysql(config, _database_preflight_sql(sql))
  after_preflight = audit_persistent_database(config, consumers, lib_root)
  (output_dir / "after-preflight.json").write_bytes(
      _canonical_json(after_preflight)
  )
  if before != after_preflight:
    raise RolPersistenceAuditError("rollback preflight changed database state")

  _run_mysql(config, sql)
  after = audit_persistent_database(config, consumers, lib_root)
  (output_dir / "after.json").write_bytes(_canonical_json(after))
  _run_mysql(config, sql)
  repeat = audit_persistent_database(config, consumers, lib_root)
  (output_dir / "repeat.json").write_bytes(_canonical_json(repeat))
  acceptance = _validate_recovery_execution(
      before, after_preflight, after, repeat
  )
  (output_dir / "acceptance.json").write_bytes(_canonical_json(acceptance))
  if not acceptance["complete"]:
    raise RolPersistenceAuditError("; ".join(acceptance["failures"]))

  artifact_paths = [
      backup_path,
      *(output_dir / name for name in (
          "before.json",
          "after-preflight.json",
          "after.json",
          "repeat.json",
          "acceptance.json",
      )),
  ]
  artifacts = [_artifact(path, output_dir) for path in artifact_paths]
  execution_seed = "\n".join(
      [str(manifest["run_id"]), database_role]
      + [row["sha256"] for row in sorted(artifacts, key=lambda item: item["path"])]
  ).encode("ascii")
  execution_id = (
      "rol-phase6-5-persistence-recovery-exec-"
      f"{hashlib.sha256(execution_seed).hexdigest()[:16]}"
  )
  execution_manifest = {
      "schema_version": ROL_PERSISTENCE_AUDIT_SCHEMA_VERSION,
      "tool_version": TOOL_VERSION,
      "run_id": execution_id,
      "creation_time": _created_at(created_at),
      "phase": "6.5-persistence-recovery-execution",
      "database_role": database_role,
      "recovery_run_id": manifest["run_id"],
      "database_identity_sha256": before["database_identity_sha256"],
      "database_backup": _artifact(backup_path, output_dir),
      "artifacts": sorted(artifacts, key=lambda item: item["path"]),
      "acceptance": acceptance,
  }
  (output_dir / "run-manifest.json").write_bytes(
      _canonical_json(execution_manifest)
  )
  return {
      "run_id": execution_id,
      "output_dir": output_dir.as_posix(),
      "database_role": database_role,
      "database_backup": backup_path.as_posix(),
      "canonical_rows_before": before["summary"]["canonical_relevant_rows"],
      "canonical_rows_after": after["summary"]["canonical_relevant_rows"],
      **acceptance,
  }


def render_persistence_bundle_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 6.5 persistence bundle: {summary['run_id']}\n"
      f"Output: {summary['output_dir']}\n"
      f"Classified consumers: {summary['classified_consumers']}\n"
      f"Migration consumers: {summary['migration_consumers']}\n"
      f"SQL statements: {summary['statements']}\n"
  )


def render_persistence_apply_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 6.5 persistence execution: {summary['run_id']}\n"
      f"Database role: {summary['database_role']}\n"
      f"Retired rows before: {summary['retired_rows_before']}\n"
      f"Retired rows after: {summary['retired_rows_after']}\n"
      f"Repeat apply no-op: {str(summary['repeat_apply_no_op']).lower()}\n"
  )


def render_persistence_recovery_human(summary: dict[str, Any]) -> str:
  return (
      f"RoL Phase 6.5 persistence recovery: {summary['run_id']}\n"
      f"Database role: {summary['database_role']}\n"
      f"Backup: {summary['database_backup']}\n"
      f"Canonical rows before: {summary['canonical_rows_before']}\n"
      f"Canonical rows after: {summary['canonical_rows_after']}\n"
      f"Repeat apply no-op: {str(summary['repeat_apply_no_op']).lower()}\n"
  )
