#!/usr/bin/env python3
"""Environment adapters and guarded application for Luminari help sync."""

from __future__ import annotations

from dataclasses import dataclass, replace
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import re
import subprocess
import time
from typing import Any, Mapping
import uuid

from catalog import (
    Catalog,
    DEFAULT_CATEGORY,
    DEFAULT_MAX_LEVEL,
    HelpEntry,
    HelpSyncError,
    RelatedTopic,
    canonical_json_bytes,
    catalog_delta,
    normalize_aliases,
    normalize_tag,
    parse_help_hlp,
    render_help_hlp,
    sha256_bytes,
    validate_help_hlp,
    validate_plan,
)


try:
    import pymysql
    from pymysql.cursors import DictCursor
except ImportError:  # pragma: no cover - exercised by the dependency gate
    pymysql = None
    DictCursor = None


TOOL_VERSION = "1.0.0"
DB_LOCK_NAME = "luminari_help_sync_write"
LOCK_FILE_NAME = ".help_sync.lock"
RELOAD_REQUEST_NAME = ".help_sync.reload.request"
RELOAD_ACK_NAME = ".help_sync.reload.ack"
STATE_DIRECTORY_NAME = ".help-sync"
REQUIRED_ENTRY_COLUMNS = {
    "tag",
    "alternate_keywords",
    "entry",
    "min_level",
    "max_level",
    "category",
    "auto_generated",
    "last_updated",
}
REQUIRED_KEYWORD_COLUMNS = {"help_tag", "keyword"}
REQUIRED_RELATED_COLUMNS = {"source_tag", "related_tag", "relevance_score"}
REQUIRED_VERSION_COLUMNS = {
    "tag",
    "alternate_keywords",
    "entry",
    "min_level",
    "max_level",
    "category",
    "auto_generated",
    "changed_by",
    "change_date",
    "change_type",
    "sync_plan_id",
}
CONTENT_TABLES = ("help_entries", "help_keywords", "help_related_topics")
TRANSACTION_TABLES = (*CONTENT_TABLES, "help_versions")
_TOKEN_RE = re.compile(r"^[0-9a-f]{64}$")
_RUN_ID_RE = re.compile(r"^\d{8}T\d{6}Z-[0-9a-f]{12}-[0-9a-f]{8}$")


class EndpointError(HelpSyncError):
    """Raised when an endpoint cannot satisfy a synchronization invariant."""


class StalePlanError(EndpointError):
    """Raised when current content no longer matches a plan precondition."""


class ApplyFailure(EndpointError):
    """Raised after an apply fails, with rollback evidence when available."""

    def __init__(self, message: str, details: Mapping[str, Any] | None = None):
        super().__init__(message)
        self.details = dict(details or {})


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def read_key_value_file(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    if not path.is_file():
        return values
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in {"'", '"'}:
            value = value[1:-1]
        values[key] = value
    return values


def environment_config(root: Path) -> dict[str, str]:
    return read_key_value_file(root / "lib" / ".env")


def environment_name(root: Path) -> str:
    value = environment_config(root).get("APP_ENV", "").strip().lower()
    if value not in {"development", "testing", "production"}:
        raise EndpointError(f"invalid or missing APP_ENV for endpoint {root}")
    return value


@dataclass(frozen=True)
class DatabaseConfig:
    host: str
    database: str
    username: str
    password: str
    port: int = 3306
    charset: str = "utf8mb4"

    @classmethod
    def from_root(cls, root: Path) -> "DatabaseConfig":
        mysql_values = read_key_value_file(root / "lib" / "mysql_config")
        env_values = environment_config(root)
        host = mysql_values.get("mysql_host") or env_values.get("DB_HOST")
        database = mysql_values.get("mysql_database") or env_values.get("DB_NAME")
        username = mysql_values.get("mysql_username") or env_values.get("DB_USER")
        password = mysql_values.get("mysql_password") or env_values.get("DB_PASS")
        port_value = mysql_values.get("mysql_port") or env_values.get("DB_PORT") or "3306"
        if not all((host, database, username, password)):
            raise EndpointError("database configuration is incomplete")
        try:
            port = int(port_value)
        except ValueError as exc:
            raise EndpointError("database port is not an integer") from exc
        return cls(
            host=host,
            database=database,
            username=username,
            password=password,
            port=port,
            charset=mysql_values.get("mysql_charset", "utf8mb4"),
        )

    def connect(self, *, autocommit: bool = False):
        if pymysql is None:
            raise EndpointError(
                "PyMySQL is required; install scripts/help-sync/requirements.txt"
            )
        return pymysql.connect(
            host=self.host,
            user=self.username,
            password=self.password,
            database=self.database,
            port=self.port,
            charset=self.charset,
            autocommit=autocommit,
            cursorclass=DictCursor,
        )


@dataclass(frozen=True)
class SchemaInfo:
    tables: Mapping[str, frozenset[str]]
    engines: Mapping[str, str]

    @property
    def write_ready(self) -> bool:
        return (
            REQUIRED_ENTRY_COLUMNS.issubset(self.tables.get("help_entries", frozenset()))
            and REQUIRED_KEYWORD_COLUMNS.issubset(
                self.tables.get("help_keywords", frozenset())
            )
            and REQUIRED_RELATED_COLUMNS.issubset(
                self.tables.get("help_related_topics", frozenset())
            )
            and REQUIRED_VERSION_COLUMNS.issubset(
                self.tables.get("help_versions", frozenset())
            )
            and all(
                self.engines.get(table, "").upper() == "INNODB"
                for table in TRANSACTION_TABLES
            )
        )

    @property
    def missing_requirements(self) -> list[str]:
        missing: list[str] = []
        required = {
            "help_entries": REQUIRED_ENTRY_COLUMNS,
            "help_keywords": REQUIRED_KEYWORD_COLUMNS,
            "help_related_topics": REQUIRED_RELATED_COLUMNS,
            "help_versions": REQUIRED_VERSION_COLUMNS,
        }
        for table, columns in required.items():
            observed = self.tables.get(table)
            if observed is None:
                missing.append(f"missing table {table}")
                continue
            for column in sorted(columns - observed):
                missing.append(f"missing column {table}.{column}")
            engine = self.engines.get(table, "")
            if engine and engine.upper() != "INNODB":
                missing.append(f"table {table} uses non-transactional engine {engine}")
        return missing


@dataclass(frozen=True)
class DatabaseCatalog:
    catalog: Catalog
    schema: SchemaInfo
    integrity_issues: tuple[str, ...]
    integrity_repairs: Mapping[str, Any]


@dataclass(frozen=True)
class EndpointSnapshot:
    environment: str
    catalog: Catalog
    manifest: Mapping[str, Any]
    schema: SchemaInfo
    integrity_issues: tuple[str, ...]
    integrity_repairs: Mapping[str, Any]
    file_issues: tuple[str, ...]
    file_hash: str | None
    expected_file_hash: str | None
    file_matches: bool
    file_entry_count: int | None

    def to_dict(self) -> dict[str, Any]:
        return {
            "tool_version": TOOL_VERSION,
            "environment": self.environment,
            "catalog": self.catalog.to_dict(),
            "manifest": dict(self.manifest),
            "schema": {
                "tables": {
                    table: sorted(columns) for table, columns in self.schema.tables.items()
                },
                "engines": dict(self.schema.engines),
                "write_ready": self.schema.write_ready,
                "missing_requirements": self.schema.missing_requirements,
            },
            "integrity_issues": list(self.integrity_issues),
            "integrity_repairs": dict(self.integrity_repairs),
            "file_issues": list(self.file_issues),
            "file_hash": self.file_hash,
            "expected_file_hash": self.expected_file_hash,
            "file_matches": self.file_matches,
            "file_entry_count": self.file_entry_count,
        }

    @classmethod
    def from_dict(cls, value: Mapping[str, Any]) -> "EndpointSnapshot":
        schema_value = value["schema"]
        schema = SchemaInfo(
            tables={
                table: frozenset(columns)
                for table, columns in schema_value.get("tables", {}).items()
            },
            engines=dict(schema_value.get("engines", {})),
        )
        return cls(
            environment=str(value["environment"]),
            catalog=Catalog.from_dict(value["catalog"]),
            manifest=dict(value["manifest"]),
            schema=schema,
            integrity_issues=tuple(value.get("integrity_issues", ())),
            integrity_repairs=dict(value.get("integrity_repairs", {})),
            file_issues=tuple(value.get("file_issues", ())),
            file_hash=value.get("file_hash"),
            expected_file_hash=value.get("expected_file_hash"),
            file_matches=bool(value.get("file_matches", False)),
            file_entry_count=value.get("file_entry_count"),
        )


def inspect_schema(connection, database_name: str) -> SchemaInfo:
    tables: dict[str, frozenset[str]] = {}
    engines: dict[str, str] = {}
    with connection.cursor() as cursor:
        for table in (
            "help_entries",
            "help_keywords",
            "help_related_topics",
            "help_versions",
            "help_search_history",
        ):
            cursor.execute(
                "SELECT ENGINE FROM information_schema.TABLES "
                "WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s",
                (database_name, table),
            )
            engine_row = cursor.fetchone()
            if not engine_row:
                continue
            engines[table] = str(engine_row["ENGINE"])
            cursor.execute(
                "SELECT COLUMN_NAME FROM information_schema.COLUMNS "
                "WHERE TABLE_SCHEMA=%s AND TABLE_NAME=%s ORDER BY ORDINAL_POSITION",
                (database_name, table),
            )
            tables[table] = frozenset(str(row["COLUMN_NAME"]) for row in cursor.fetchall())
    return SchemaInfo(tables=tables, engines=engines)


def _entry_select(schema: SchemaInfo, *, for_update: bool) -> str:
    columns = schema.tables.get("help_entries", frozenset())
    if not {"tag", "entry", "min_level"}.issubset(columns):
        raise EndpointError("help_entries lacks the minimum readable columns")
    selections = ["tag", "entry", "min_level"]
    selections.append("max_level" if "max_level" in columns else f"{DEFAULT_MAX_LEVEL} max_level")
    selections.append(
        "category" if "category" in columns else f"'{DEFAULT_CATEGORY}' category"
    )
    selections.append(
        "auto_generated" if "auto_generated" in columns else "FALSE auto_generated"
    )
    selections.append(
        "alternate_keywords" if "alternate_keywords" in columns else "NULL alternate_keywords"
    )
    suffix = " FOR UPDATE" if for_update else ""
    return f"SELECT {', '.join(selections)} FROM help_entries ORDER BY tag{suffix}"


def read_database_catalog(
    connection,
    database_name: str,
    *,
    for_update: bool = False,
    begin_transaction: bool = True,
) -> DatabaseCatalog:
    schema = inspect_schema(connection, database_name)
    integrity: list[str] = []
    orphan_keyword_counts: dict[tuple[str, str], int] = {}
    missing_keyword_tags: list[str] = []
    if begin_transaction:
        connection.commit()
        with connection.cursor() as cursor:
            cursor.execute("SET TRANSACTION ISOLATION LEVEL REPEATABLE READ")
            if for_update:
                cursor.execute("START TRANSACTION")
            else:
                cursor.execute("START TRANSACTION WITH CONSISTENT SNAPSHOT")

    with connection.cursor() as cursor:
        cursor.execute(_entry_select(schema, for_update=for_update))
        rows = cursor.fetchall()
        keyword_rows: list[Mapping[str, Any]] = []
        if "help_keywords" in schema.tables:
            query = "SELECT help_tag, keyword FROM help_keywords ORDER BY help_tag, keyword"
            if for_update:
                query += " FOR UPDATE"
            cursor.execute(query)
            keyword_rows = list(cursor.fetchall())
        related_rows: list[Mapping[str, Any]] = []
        if "help_related_topics" in schema.tables:
            query = (
                "SELECT source_tag, related_tag, relevance_score FROM help_related_topics "
                "ORDER BY source_tag, related_tag"
            )
            if for_update:
                query += " FOR UPDATE"
            cursor.execute(query)
            related_rows = list(cursor.fetchall())

    actual_tags: dict[str, str] = {}
    raw_rows: dict[str, Mapping[str, Any]] = {}
    for row in rows:
        normalized = normalize_tag(str(row["tag"]))
        if normalized in raw_rows:
            raise EndpointError(f"database contains duplicate normalized tag {normalized!r}")
        raw_rows[normalized] = row
        actual_tags[str(row["tag"])] = normalized

    keywords: dict[str, set[str]] = {tag: set() for tag in raw_rows}
    for row in keyword_rows:
        actual_help_tag = str(row["help_tag"])
        tag = actual_tags.get(actual_help_tag)
        if tag is None:
            try:
                tag = normalize_tag(actual_help_tag)
            except HelpSyncError:
                tag = actual_help_tag
        if tag not in raw_rows:
            integrity.append(
                f"orphan keyword {row['keyword']!r} references missing tag {actual_help_tag!r}"
            )
            key = (actual_help_tag, str(row["keyword"]))
            orphan_keyword_counts[key] = orphan_keyword_counts.get(key, 0) + 1
            continue
        keywords[tag].add(str(row["keyword"]))

    related: dict[str, list[RelatedTopic]] = {tag: [] for tag in raw_rows}
    for row in related_rows:
        source = normalize_tag(str(row["source_tag"]))
        target = normalize_tag(str(row["related_tag"]))
        if source not in raw_rows or target not in raw_rows:
            integrity.append(
                f"orphan related topic {source!r} -> {target!r} references a missing entry"
            )
            continue
        related[source].append(RelatedTopic(target, row["relevance_score"]))

    entries: list[HelpEntry] = []
    for tag, row in raw_rows.items():
        if not keywords[tag]:
            integrity.append(f"help entry {tag!r} has no help_keywords rows")
            missing_keyword_tags.append(tag)
        entries.append(
            HelpEntry(
                tag=tag,
                body=row.get("entry"),
                keywords=tuple(keywords[tag]),
                aliases=normalize_aliases(row.get("alternate_keywords")),
                min_level=int(row.get("min_level") or 0),
                max_level=int(
                    row.get("max_level")
                    if row.get("max_level") is not None
                    else DEFAULT_MAX_LEVEL
                ),
                category=str(row.get("category") or DEFAULT_CATEGORY),
                auto_generated=bool(row.get("auto_generated") or False),
                related_topics=tuple(related[tag]),
            )
        )

    return DatabaseCatalog(
        catalog=Catalog(tuple(entries)),
        schema=schema,
        integrity_issues=tuple(sorted(integrity)),
        integrity_repairs={
            "orphan_keywords": [
                {"help_tag": help_tag, "keyword": keyword, "count": count}
                for (help_tag, keyword), count in sorted(orphan_keyword_counts.items())
            ],
            "missing_keyword_tags": sorted(missing_keyword_tags),
        },
    )


def repair_missing_keywords(catalog: Catalog, *fallback_catalogs: Catalog) -> Catalog:
    """Return the explicit repair for entries with no keyword rows.

    Preserve reviewed keywords from the baseline or peer endpoint when they exist;
    use the tag only when no catalog has a keyword for the entry.
    """
    fallback_maps = [fallback.by_tag for fallback in fallback_catalogs]
    entries = []
    for entry in catalog.entries:
        if entry.keywords:
            entries.append(entry)
        else:
            keywords = {
                keyword
                for fallback in fallback_maps
                if entry.tag in fallback
                for keyword in fallback[entry.tag].keywords
            }
            entries.append(replace(entry, keywords=tuple(keywords or {entry.tag})))
    return Catalog(tuple(entries))


def normalize_integrity_repairs(value: Mapping[str, Any] | None) -> dict[str, Any]:
    value = value or {}
    orphan_counts: dict[tuple[str, str], int] = {}
    for repair in value.get("orphan_keywords", ()):
        help_tag = str(repair["help_tag"])
        keyword = str(repair["keyword"])
        count = int(repair.get("count", 1))
        if not help_tag or not keyword or "\x00" in help_tag or "\x00" in keyword or count < 1:
            raise EndpointError("invalid orphan-keyword repair record")
        key = (help_tag, keyword)
        orphan_counts[key] = orphan_counts.get(key, 0) + count
    missing = sorted({normalize_tag(tag) for tag in value.get("missing_keyword_tags", ())})
    return {
        "orphan_keywords": [
            {"help_tag": help_tag, "keyword": keyword, "count": count}
            for (help_tag, keyword), count in sorted(orphan_counts.items())
        ],
        "missing_keyword_tags": missing,
    }


def apply_integrity_repairs(connection, repairs: Mapping[str, Any]) -> None:
    normalized = normalize_integrity_repairs(repairs)
    with connection.cursor() as cursor:
        for repair in normalized["orphan_keywords"]:
            cursor.execute(
                "DELETE FROM help_keywords WHERE BINARY help_tag=BINARY %s "
                "AND BINARY keyword=BINARY %s",
                (repair["help_tag"], repair["keyword"]),
            )
            if cursor.rowcount != repair["count"]:
                raise StalePlanError(
                    "orphan-keyword repair expected "
                    f"{repair['count']} rows for {repair['help_tag']!r}/{repair['keyword']!r}, "
                    f"affected {cursor.rowcount}"
                )


def restore_integrity_rows(connection, repairs: Mapping[str, Any]) -> None:
    normalized = normalize_integrity_repairs(repairs)
    with connection.cursor() as cursor:
        for repair in normalized["orphan_keywords"]:
            for _ in range(repair["count"]):
                cursor.execute(
                    "INSERT INTO help_keywords (help_tag, keyword) VALUES (%s, %s)",
                    (repair["help_tag"], repair["keyword"]),
                )


def _git_identity(root: Path) -> str | None:
    result = subprocess.run(
        ["git", "rev-parse", "HEAD"],
        cwd=root,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return result.stdout.strip() if result.returncode == 0 else None


def take_snapshot(root: Path, expected_environment: str | None = None) -> EndpointSnapshot:
    root = root.resolve()
    observed_environment = environment_name(root)
    if expected_environment and observed_environment != expected_environment:
        raise EndpointError(
            f"endpoint environment is {observed_environment!r}, expected {expected_environment!r}"
        )
    config = DatabaseConfig.from_root(root)
    connection = config.connect()
    try:
        database = read_database_catalog(connection, config.database)
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()

    help_file = root / "lib" / "text" / "help" / "help.hlp"
    file_issues: list[str] = []
    file_hash: str | None = None
    file_entry_count: int | None = None
    actual = b""
    if not help_file.is_file():
        file_issues.append("help.hlp is missing")
    elif help_file.is_symlink():
        file_issues.append("help.hlp must not be a symbolic link")
    else:
        actual = help_file.read_bytes()
        file_hash = sha256_bytes(actual)
        try:
            file_entry_count = len(parse_help_hlp(actual))
        except HelpSyncError as exc:
            file_issues.append(str(exc))

    expected_file_hash: str | None = None
    try:
        expected = render_help_hlp(database.catalog)
        expected_file_hash = sha256_bytes(expected)
    except HelpSyncError as exc:
        expected = b""
        file_issues.append(f"database catalog cannot be projected to help.hlp: {exc}")
    file_matches = bool(actual) and not file_issues and actual == expected
    manifest = {
        "format": "luminari-help-snapshot-manifest",
        "version": 1,
        "tool_version": TOOL_VERSION,
        "environment": observed_environment,
        "catalog_hash": database.catalog.content_hash,
        "file_hash": file_hash,
        "expected_file_hash": expected_file_hash,
        "entry_count": len(database.catalog.entries),
        "relationship_count": database.catalog.relationship_count,
        "integrity_state_hash": sha256_bytes(
            canonical_json_bytes(normalize_integrity_repairs(database.integrity_repairs))
        ),
        "snapshot_time": utc_now(),
        "git_identity": _git_identity(root),
    }
    return EndpointSnapshot(
        environment=observed_environment,
        catalog=database.catalog,
        manifest=manifest,
        schema=database.schema,
        integrity_issues=database.integrity_issues,
        integrity_repairs=database.integrity_repairs,
        file_issues=tuple(sorted(file_issues)),
        file_hash=file_hash,
        expected_file_hash=expected_file_hash,
        file_matches=file_matches,
        file_entry_count=file_entry_count,
    )


def verify_endpoint(
    root: Path, expected_environment: str, candidate: Catalog
) -> dict[str, Any]:
    snapshot = take_snapshot(root, expected_environment)
    failures: list[str] = []
    if not snapshot.schema.write_ready:
        failures.extend(snapshot.schema.missing_requirements)
    if snapshot.integrity_issues:
        failures.extend(snapshot.integrity_issues)
    if snapshot.file_issues:
        failures.extend(snapshot.file_issues)
    if snapshot.catalog.content_hash != candidate.content_hash:
        failures.append(
            "database catalog hash does not match candidate "
            f"({snapshot.catalog.content_hash} != {candidate.content_hash})"
        )
    if not snapshot.file_matches:
        failures.append("help.hlp is not the deterministic candidate projection")
    mud_running = _process_is_running(root)
    if expected_environment == "production" and not mud_running:
        failures.append("production MUD process is not healthy/running")

    lookup_checks: list[dict[str, Any]] = []
    if not failures and candidate.entries:
        indexes = sorted({0, len(candidate.entries) // 2, len(candidate.entries) - 1})
        config = DatabaseConfig.from_root(root)
        connection = config.connect()
        try:
            with connection.cursor() as cursor:
                for index in indexes:
                    entry = candidate.entries[index]
                    keyword = entry.keywords[0]
                    cursor.execute(
                        "SELECT he.tag, he.entry, he.min_level FROM help_entries he "
                        "INNER JOIN help_keywords hk ON he.tag=hk.help_tag "
                        "WHERE LOWER(he.tag)=LOWER(%s) AND BINARY hk.keyword=BINARY %s",
                        (entry.tag, keyword),
                    )
                    row = cursor.fetchone()
                    passed = bool(
                        row
                        and normalize_tag(str(row["tag"])) == entry.tag
                        and HelpEntry(
                            tag=entry.tag,
                            body=str(row["entry"]),
                            keywords=entry.keywords,
                            aliases=entry.aliases,
                            min_level=int(row["min_level"]),
                            max_level=entry.max_level,
                            category=entry.category,
                            auto_generated=entry.auto_generated,
                            related_topics=entry.related_topics,
                        ).body
                        == entry.body
                    )
                    lookup_checks.append(
                        {"tag": entry.tag, "keyword": keyword, "passed": passed}
                    )
                    if not passed:
                        failures.append(f"representative database help lookup failed for {entry.tag}")
        finally:
            connection.close()

    if failures:
        raise EndpointError("help endpoint verification failed: " + "; ".join(failures[:20]))
    return {
        "status": "verified",
        "environment": expected_environment,
        "catalog_hash": candidate.content_hash,
        "file_hash": snapshot.file_hash,
        "entry_count": len(candidate.entries),
        "relationship_count": candidate.relationship_count,
        "lookup_checks": lookup_checks,
        "mud_running": mud_running,
    }


def atomic_write(path: Path, data: bytes, mode: int = 0o640) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.is_symlink():
        raise EndpointError(f"refusing to replace symbolic link {path}")
    temp_path = path.with_name(f".{path.name}.tmp.{os.getpid()}.{uuid.uuid4().hex}")
    descriptor = os.open(temp_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, mode)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        if path.exists():
            os.chmod(temp_path, path.stat().st_mode & 0o777)
        os.replace(temp_path, path)
        directory_fd = os.open(path.parent, os.O_RDONLY)
        try:
            os.fsync(directory_fd)
        finally:
            os.close(directory_fd)
    finally:
        if temp_path.exists():
            temp_path.unlink()


def atomic_write_json(path: Path, value: Mapping[str, Any]) -> None:
    atomic_write(path, canonical_json_bytes(value) + b"\n")


class HelpWriteBarrier:
    def __init__(self, root: Path, owner: str):
        if not _TOKEN_RE.fullmatch(owner):
            raise EndpointError("barrier owner must be a 64-character lowercase hex plan ID")
        self.path = root / "lib" / "text" / "help" / LOCK_FILE_NAME
        self.owner = owner
        self.acquired = False

    def acquire(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        try:
            descriptor = os.open(self.path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o640)
        except FileExistsError as exc:
            observed = "unknown"
            try:
                observed = self.path.read_text(encoding="ascii", errors="replace").strip()[:64]
            except OSError:
                pass
            raise EndpointError(f"HEDIT barrier is already held by {observed}") from exc
        with os.fdopen(descriptor, "w", encoding="ascii", newline="\n") as stream:
            stream.write(self.owner + "\n")
            stream.flush()
            os.fsync(stream.fileno())
        self.acquired = True

    def release(self) -> None:
        if not self.acquired:
            return
        try:
            observed = self.path.read_text(encoding="ascii").strip()
        except FileNotFoundError:
            observed = ""
        if observed != self.owner:
            raise EndpointError("HEDIT barrier ownership changed; refusing to remove it")
        self.path.unlink()
        self.acquired = False

    def __enter__(self) -> "HelpWriteBarrier":
        self.acquire()
        return self

    def __exit__(self, exc_type, exc, traceback) -> None:
        self.release()


def acquire_database_lock(connection, timeout: int) -> None:
    with connection.cursor() as cursor:
        cursor.execute("SELECT GET_LOCK(%s, %s) AS acquired", (DB_LOCK_NAME, timeout))
        row = cursor.fetchone()
    if not row or int(row["acquired"] or 0) != 1:
        raise EndpointError("could not acquire the help synchronization database lock")


def release_database_lock(connection) -> None:
    try:
        with connection.cursor() as cursor:
            cursor.execute("SELECT RELEASE_LOCK(%s) AS released", (DB_LOCK_NAME,))
    except Exception:
        pass


def _process_is_running(root: Path) -> bool:
    pid_file = root / ".mud.pid"
    try:
        raw_pid = pid_file.read_text(encoding="ascii").strip()
        if not raw_pid.isdigit():
            return False
        pid = int(raw_pid)
        os.kill(pid, 0)
        command_line = Path(f"/proc/{pid}/cmdline")
        if command_line.is_file():
            command = command_line.read_bytes().replace(b"\x00", b" ").lower()
            return b"luminari" in command
        return True
    except (FileNotFoundError, PermissionError, ProcessLookupError, ValueError):
        return False


def help_file_hash(path: Path) -> str | None:
    if path.is_symlink():
        raise EndpointError("help.hlp must not be a symbolic link")
    if path.exists() and not path.is_file():
        raise EndpointError("help.hlp exists but is not a regular file")
    return sha256_bytes(path.read_bytes()) if path.is_file() else None


def request_runtime_reload(
    root: Path, token: str, timeout: float, *, require_running: bool = False
) -> str:
    if not _TOKEN_RE.fullmatch(token):
        raise EndpointError("runtime reload token must be a catalog SHA-256")
    if not _process_is_running(root):
        if require_running:
            raise EndpointError("the production MUD is not running; runtime reload cannot be proven")
        return "not-running; next boot will load the new projection"

    help_directory = root / "lib" / "text" / "help"
    request_path = help_directory / RELOAD_REQUEST_NAME
    ack_path = help_directory / RELOAD_ACK_NAME
    if request_path.exists():
        raise EndpointError("a help runtime reload request is already pending")
    if ack_path.exists():
        ack_path.unlink()
    atomic_write(request_path, (token + "\n").encode("ascii"))
    deadline = time.monotonic() + timeout
    expected_ack = f"{token} ok"
    while time.monotonic() < deadline:
        try:
            acknowledgment = ack_path.read_text(encoding="ascii").strip()
        except FileNotFoundError:
            acknowledgment = ""
        if acknowledgment == expected_ack:
            ack_path.unlink()
            request_path.unlink(missing_ok=True)
            return "runtime cache and fallback table reloaded"
        if acknowledgment and acknowledgment != expected_ack:
            request_path.unlink(missing_ok=True)
            raise EndpointError(f"runtime returned an invalid reload acknowledgment: {acknowledgment}")
        time.sleep(0.1)
    request_path.unlink(missing_ok=True)
    raise EndpointError("timed out waiting for the MUD to acknowledge help reload")


def _validate_delta_preconditions(current: Catalog, delta: Mapping[str, Any]) -> None:
    current_map = current.by_tag
    for change in delta.get("additions", ()):
        if normalize_tag(change["tag"]) in current_map:
            raise StalePlanError(f"planned addition {change['tag']!r} already exists")
    for kind in ("updates", "deletions"):
        for change in delta.get(kind, ()):
            tag = normalize_tag(change["tag"])
            observed = current_map.get(tag)
            if observed is None or observed.content_hash != change["before_hash"]:
                raise StalePlanError(f"old-hash precondition failed for {tag!r}")


def _history_columns(schema: SchemaInfo) -> frozenset[str]:
    return schema.tables.get("help_versions", frozenset())


def _write_history(
    connection,
    schema: SchemaInfo,
    plan_id: str,
    delta: Mapping[str, Any],
) -> None:
    columns = _history_columns(schema)
    if not {"tag", "entry", "min_level"}.issubset(columns):
        return
    records: list[tuple[HelpEntry, str]] = []
    for change in delta.get("additions", ()):
        records.append((HelpEntry.from_dict(change["after"]), "CREATE"))
    for change in delta.get("updates", ()):
        records.append((HelpEntry.from_dict(change["before"]), "UPDATE"))
    for change in delta.get("deletions", ()):
        records.append((HelpEntry.from_dict(change["before"]), "DELETE"))

    for entry, change_type in records:
        names = ["tag", "entry", "min_level"]
        values: list[Any] = [entry.tag, entry.body, entry.min_level]
        optional = {
            "alternate_keywords": " ".join(entry.aliases) or None,
            "max_level": entry.max_level,
            "category": entry.category,
            "auto_generated": entry.auto_generated,
            "changed_by": "help-sync",
            "change_type": change_type,
            "sync_plan_id": plan_id,
            "saved_by": "help-sync",
        }
        for name, value in optional.items():
            if name in columns:
                names.append(name)
                values.append(value)
        placeholders = ", ".join("%s" for _ in names)
        quoted_names = ", ".join(f"`{name}`" for name in names)
        with connection.cursor() as cursor:
            cursor.execute(
                f"INSERT INTO help_versions ({quoted_names}) VALUES ({placeholders})", values
            )


def _entry_write_columns(schema: SchemaInfo, entry: HelpEntry) -> tuple[list[str], list[Any]]:
    available = schema.tables["help_entries"]
    names = ["tag", "entry", "min_level"]
    values: list[Any] = [entry.tag, entry.body, entry.min_level]
    optional = {
        "alternate_keywords": " ".join(entry.aliases) or None,
        "max_level": entry.max_level,
        "category": entry.category,
        "auto_generated": entry.auto_generated,
    }
    for name, value in optional.items():
        if name in available:
            names.append(name)
            values.append(value)
    return names, values


def apply_delta_to_database(
    connection,
    schema: SchemaInfo,
    delta: Mapping[str, Any],
    candidate: Catalog,
    plan_id: str,
) -> None:
    if not schema.write_ready:
        raise EndpointError(
            "help schema is not write-ready: " + "; ".join(schema.missing_requirements)
        )
    _write_history(connection, schema, plan_id, delta)
    changed_tags = {
        normalize_tag(change["tag"])
        for kind in ("additions", "updates", "deletions")
        for change in delta.get(kind, ())
    }

    with connection.cursor() as cursor:
        for tag in sorted(changed_tags):
            cursor.execute(
                "DELETE FROM help_related_topics "
                "WHERE LOWER(source_tag)=LOWER(%s) OR LOWER(related_tag)=LOWER(%s)",
                (tag, tag),
            )
            cursor.execute("DELETE FROM help_keywords WHERE LOWER(help_tag)=LOWER(%s)", (tag,))

        for change in delta.get("deletions", ()):
            cursor.execute(
                "DELETE FROM help_entries WHERE LOWER(tag)=LOWER(%s)", (change["tag"],)
            )
            if cursor.rowcount != 1:
                raise StalePlanError(
                    f"delete precondition affected {cursor.rowcount} rows for {change['tag']!r}"
                )

        for change in delta.get("updates", ()):
            entry = HelpEntry.from_dict(change["after"])
            names, values = _entry_write_columns(schema, entry)
            update_names = names[1:]
            assignments = ", ".join(f"`{name}`=%s" for name in update_names)
            cursor.execute(
                f"UPDATE help_entries SET {assignments} WHERE LOWER(tag)=LOWER(%s)",
                values[1:] + [change["tag"]],
            )
            if cursor.rowcount not in {0, 1}:
                raise EndpointError(f"update touched multiple rows for {change['tag']!r}")

        for change in delta.get("additions", ()):
            entry = HelpEntry.from_dict(change["after"])
            names, values = _entry_write_columns(schema, entry)
            placeholders = ", ".join("%s" for _ in names)
            quoted_names = ", ".join(f"`{name}`" for name in names)
            cursor.execute(
                f"INSERT INTO help_entries ({quoted_names}) VALUES ({placeholders})", values
            )

        target_map = candidate.by_tag
        for tag in sorted(changed_tags & set(target_map)):
            entry = target_map[tag]
            for keyword in entry.keywords:
                cursor.execute(
                    "INSERT INTO help_keywords (help_tag, keyword) VALUES (%s, %s)",
                    (entry.tag, keyword),
                )

        for entry in candidate.entries:
            if entry.tag not in changed_tags and not any(
                topic.tag in changed_tags for topic in entry.related_topics
            ):
                continue
            cursor.execute(
                "DELETE FROM help_related_topics WHERE LOWER(source_tag)=LOWER(%s)",
                (entry.tag,),
            )
            for topic in entry.related_topics:
                cursor.execute(
                    "INSERT INTO help_related_topics "
                    "(source_tag, related_tag, relevance_score) VALUES (%s, %s, %s)",
                    (entry.tag, topic.tag, topic.relevance),
                )


def _run_directory(root: Path, environment: str, plan_id: str) -> tuple[str, Path]:
    run_id = (
        datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        + f"-{plan_id[:12]}-{uuid.uuid4().hex[:8]}"
    )
    directory = (
        root
        / "lib"
        / "text"
        / "help"
        / STATE_DIRECTORY_NAME
        / "runs"
        / run_id
        / environment
    )
    directory.mkdir(parents=True, exist_ok=False)
    return run_id, directory


def write_backup(
    directory: Path,
    environment: str,
    plan_id: str,
    catalog: Catalog,
    integrity_repairs: Mapping[str, Any],
    help_file: Path,
) -> dict[str, Any]:
    catalog_path = directory / "catalog.json"
    integrity_path = directory / "integrity.json"
    file_path = directory / "help.hlp"
    file_present = help_file.is_file()
    file_bytes = help_file.read_bytes() if file_present else b""
    atomic_write_json(catalog_path, catalog.to_dict())
    atomic_write_json(integrity_path, normalize_integrity_repairs(integrity_repairs))
    if file_present:
        atomic_write(file_path, file_bytes)
    manifest = {
        "format": "luminari-help-sync-backup",
        "version": 2,
        "tool_version": TOOL_VERSION,
        "environment": environment,
        "plan_id": plan_id,
        "created_at": utc_now(),
        "catalog_hash": catalog.content_hash,
        "catalog_file_hash": sha256_bytes(catalog_path.read_bytes()),
        "integrity_file_hash": sha256_bytes(integrity_path.read_bytes()),
        "help_file_present": file_present,
        "help_file_hash": sha256_bytes(file_bytes) if file_present else None,
        "state": "prepared",
    }
    atomic_write_json(directory / "manifest.json", manifest)
    validate_backup(directory)
    return manifest


def validate_backup(
    directory: Path,
) -> tuple[dict[str, Any], Catalog, dict[str, Any], bytes | None]:
    manifest = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
    if manifest.get("format") != "luminari-help-sync-backup" or manifest.get("version") != 2:
        raise EndpointError("unsupported backup format or version")
    catalog_bytes = (directory / "catalog.json").read_bytes()
    if sha256_bytes(catalog_bytes) != manifest["catalog_file_hash"]:
        raise EndpointError("backup catalog file checksum mismatch")
    catalog = Catalog.from_dict(json.loads(catalog_bytes.decode("utf-8")))
    if catalog.content_hash != manifest["catalog_hash"]:
        raise EndpointError("backup semantic catalog checksum mismatch")
    integrity_bytes = (directory / "integrity.json").read_bytes()
    if sha256_bytes(integrity_bytes) != manifest["integrity_file_hash"]:
        raise EndpointError("backup integrity-state checksum mismatch")
    integrity_repairs = normalize_integrity_repairs(
        json.loads(integrity_bytes.decode("utf-8"))
    )
    file_bytes: bytes | None = None
    if manifest["help_file_present"]:
        file_bytes = (directory / "help.hlp").read_bytes()
        if sha256_bytes(file_bytes) != manifest["help_file_hash"]:
            raise EndpointError("backup help.hlp checksum mismatch")
    return manifest, catalog, integrity_repairs, file_bytes


def _restore_after_failure(
    root: Path,
    connection,
    config: DatabaseConfig,
    schema: SchemaInfo,
    directory: Path,
    failed_current: Catalog,
    plan_id: str,
    reload_timeout: float,
) -> dict[str, Any]:
    manifest, backup_catalog, backup_repairs, backup_file = validate_backup(directory)
    restore_delta = catalog_delta(
        failed_current,
        backup_catalog,
        authorized_deletions=set(failed_current.by_tag) - set(backup_catalog.by_tag),
    )
    try:
        with connection.cursor() as cursor:
            cursor.execute("START TRANSACTION")
        apply_delta_to_database(
            connection, schema, restore_delta, backup_catalog, f"rollback-{plan_id}"
        )
        restore_integrity_rows(connection, backup_repairs)
        connection.commit()
        help_file = root / "lib" / "text" / "help" / "help.hlp"
        if backup_file is None:
            if help_file.exists():
                help_file.unlink()
        else:
            atomic_write(help_file, backup_file)
        runtime = request_runtime_reload(
            root,
            backup_catalog.content_hash,
            reload_timeout,
            require_running=environment_name(root) == "production",
        )
        observed = read_database_catalog(
            connection, config.database, begin_transaction=True
        )
        connection.commit()
        if observed.catalog.content_hash != backup_catalog.content_hash:
            raise EndpointError("automatic rollback database verification failed")
        if normalize_integrity_repairs(observed.integrity_repairs) != backup_repairs:
            raise EndpointError("automatic rollback integrity-state verification failed")
        if backup_file is not None and help_file.read_bytes() != backup_file:
            raise EndpointError("automatic rollback help.hlp verification failed")
        manifest["state"] = "rolled-back"
        manifest["rolled_back_at"] = utc_now()
        atomic_write_json(directory / "manifest.json", manifest)
        return {
            "status": "rolled-back",
            "catalog_hash": backup_catalog.content_hash,
            "runtime": runtime,
        }
    except Exception as exc:
        connection.rollback()
        manifest["state"] = "rollback-failed"
        manifest["rollback_error"] = str(exc)
        atomic_write_json(directory / "manifest.json", manifest)
        return {"status": "rollback-failed", "error": str(exc)}


def apply_plan_to_endpoint(
    root: Path,
    expected_environment: str,
    plan: Mapping[str, Any],
    *,
    lock_timeout: int = 30,
    reload_timeout: float = 30.0,
) -> dict[str, Any]:
    validate_plan(plan)
    if not plan.get("sealed"):
        raise EndpointError("unresolved plan cannot be applied")
    root = root.resolve()
    observed_environment = environment_name(root)
    if observed_environment != expected_environment:
        raise EndpointError(
            f"refusing {expected_environment} apply against {observed_environment} endpoint"
        )
    delta_key = (
        "development_delta" if expected_environment == "development" else "production_delta"
    )
    expected_hash_key = (
        "development_hash" if expected_environment == "development" else "production_hash"
    )
    delta = plan[delta_key]
    candidate = Catalog.from_dict(plan["candidate"])
    plan_id = str(plan["plan_id"])
    if candidate.content_hash != plan["candidate_hash"]:
        raise EndpointError("candidate hash mismatch")
    if any(not entry.keywords for entry in candidate.entries):
        raise EndpointError("candidate contains an entry without a database lookup keyword")
    repair_key = f"{expected_environment}_integrity_repairs"
    expected_repairs = normalize_integrity_repairs(plan.get(repair_key, {}))

    config = DatabaseConfig.from_root(root)
    connection = config.connect()
    barrier = HelpWriteBarrier(root, plan_id)
    database_locked = False
    run_id: str | None = None
    run_directory: Path | None = None
    backup_manifest: dict[str, Any] | None = None
    committed = False
    file_replaced = False
    current_after_commit: Catalog | None = None
    help_file = root / "lib" / "text" / "help" / "help.hlp"

    try:
        barrier.acquire()
        acquire_database_lock(connection, lock_timeout)
        database_locked = True
        current_data = read_database_catalog(
            connection, config.database, for_update=True, begin_transaction=True
        )
        if not current_data.schema.write_ready:
            raise EndpointError(
                "help schema is not write-ready: "
                + "; ".join(current_data.schema.missing_requirements)
            )
        observed_repairs = normalize_integrity_repairs(current_data.integrity_repairs)
        current = current_data.catalog
        rendered = render_help_hlp(candidate)
        candidate_file_hash = sha256_bytes(rendered)
        observed_file_hash = help_file_hash(help_file)
        state_key = (
            "development_state"
            if observed_environment == "development"
            else "production_state"
        )
        planned_file_hash = dict(plan.get(state_key, {})).get("file_hash")
        if observed_file_hash not in {planned_file_hash, candidate_file_hash}:
            raise StalePlanError("help.hlp changed after planning; create a new plan")
        current_projection = render_help_hlp(current)
        current_layer_matches = (
            observed_file_hash is not None
            and help_file.read_bytes() == current_projection
        )
        if not current_layer_matches and not bool(plan.get("repair_layers")):
            raise StalePlanError(
                "database/help.hlp drift requires an explicitly sealed layer repair"
            )
        expected_hash = str(plan[expected_hash_key])
        if current.content_hash not in {expected_hash, candidate.content_hash}:
            raise StalePlanError(
                f"stale plan: expected {expected_hash}, observed {current.content_hash}"
            )
        clean_repairs = normalize_integrity_repairs({})
        if current.content_hash == candidate.content_hash:
            if observed_repairs != clean_repairs:
                raise StalePlanError(
                    "candidate content is present but its database integrity state is not clean"
                )
            repairs_to_apply = clean_repairs
        else:
            if observed_repairs != expected_repairs:
                raise StalePlanError("database integrity state changed after the plan snapshot")
            repairs_to_apply = expected_repairs

        run_id, run_directory = _run_directory(root, observed_environment, plan_id)
        backup_manifest = write_backup(
            run_directory,
            observed_environment,
            plan_id,
            current,
            observed_repairs,
            help_file,
        )
        validate_help_hlp(rendered, candidate)
        apply_integrity_repairs(connection, repairs_to_apply)

        if current.content_hash == candidate.content_hash:
            if repairs_to_apply["orphan_keywords"]:
                if help_file_hash(help_file) != observed_file_hash:
                    raise StalePlanError("help.hlp changed during apply; refusing commit")
                connection.commit()
                committed = True
            else:
                connection.rollback()
            current_after_commit = candidate
            layer_repaired = not help_file.is_file() or help_file.read_bytes() != rendered
            if layer_repaired:
                if help_file_hash(help_file) != observed_file_hash:
                    raise StalePlanError("help.hlp changed during apply; refusing replacement")
                atomic_write(help_file, rendered)
                file_replaced = True
                runtime = request_runtime_reload(
                    root,
                    candidate.content_hash,
                    reload_timeout,
                    require_running=observed_environment == "production",
                )
            else:
                runtime = "no-op; runtime content already current"
            verified_data = read_database_catalog(
                connection, config.database, begin_transaction=True
            )
            connection.commit()
            if verified_data.catalog.content_hash != candidate.content_hash:
                raise EndpointError("no-op database verification failed")
            if verified_data.integrity_issues:
                raise EndpointError(
                    "no-op integrity verification failed: "
                    + "; ".join(verified_data.integrity_issues[:10])
                )
            validate_help_hlp(help_file.read_bytes(), candidate)
            backup_manifest["state"] = "verified-no-op"
            backup_manifest["post_hash"] = candidate.content_hash
            atomic_write_json(run_directory / "manifest.json", backup_manifest)
            return {
                "status": "verified-no-op",
                "environment": observed_environment,
                "run_id": run_id,
                "backup_directory": str(run_directory),
                "catalog_hash": candidate.content_hash,
                "file_hash": sha256_bytes(rendered),
                "layer_repaired": layer_repaired,
                "runtime": runtime,
            }

        _validate_delta_preconditions(current, delta)
        apply_delta_to_database(connection, current_data.schema, delta, candidate, plan_id)
        if help_file_hash(help_file) != observed_file_hash:
            raise StalePlanError("help.hlp changed during apply; refusing commit")
        connection.commit()
        committed = True
        current_after_commit = candidate
        if help_file_hash(help_file) != observed_file_hash:
            raise StalePlanError("help.hlp changed during apply; refusing replacement")
        atomic_write(help_file, rendered)
        file_replaced = True
        runtime = request_runtime_reload(
            root,
            candidate.content_hash,
            reload_timeout,
            require_running=observed_environment == "production",
        )

        verified_data = read_database_catalog(
            connection, config.database, begin_transaction=True
        )
        connection.commit()
        if verified_data.integrity_issues:
            raise EndpointError(
                "post-apply database integrity check failed: "
                + "; ".join(verified_data.integrity_issues[:10])
            )
        if verified_data.catalog.content_hash != candidate.content_hash:
            raise EndpointError("post-apply database hash does not match candidate")
        installed = help_file.read_bytes()
        validate_help_hlp(installed, candidate)

        backup_manifest["state"] = "applied"
        backup_manifest["post_hash"] = candidate.content_hash
        backup_manifest["applied_at"] = utc_now()
        atomic_write_json(run_directory / "manifest.json", backup_manifest)
        return {
            "status": "applied",
            "environment": observed_environment,
            "run_id": run_id,
            "backup_directory": str(run_directory),
            "before_hash": current.content_hash,
            "catalog_hash": candidate.content_hash,
            "file_hash": sha256_bytes(installed),
            "counts": dict(delta["counts"]),
            "runtime": runtime,
        }
    except Exception as exc:
        try:
            connection.rollback()
        except Exception:
            pass
        if isinstance(exc, StalePlanError) and not committed and not file_replaced:
            raise
        details: dict[str, Any] = {
            "environment": observed_environment,
            "run_id": run_id,
            "backup_directory": str(run_directory) if run_directory else None,
            "error": str(exc),
        }
        if (committed or file_replaced) and run_directory and current_after_commit:
            details["automatic_rollback"] = _restore_after_failure(
                root,
                connection,
                config,
                inspect_schema(connection, config.database),
                run_directory,
                current_after_commit,
                plan_id,
                reload_timeout,
            )
        raise ApplyFailure(f"help apply failed: {exc}", details) from exc
    finally:
        if database_locked:
            release_database_lock(connection)
        connection.close()
        if barrier.acquired:
            barrier.release()


def rollback_endpoint(
    root: Path,
    expected_environment: str,
    run_id: str,
    expected_current_hash: str,
    *,
    lock_timeout: int = 30,
    reload_timeout: float = 30.0,
) -> dict[str, Any]:
    root = root.resolve()
    if not _RUN_ID_RE.fullmatch(run_id):
        raise EndpointError("rollback run ID has an invalid format")
    if not _TOKEN_RE.fullmatch(expected_current_hash):
        raise EndpointError("rollback expected-current hash must be a SHA-256")
    observed_environment = environment_name(root)
    if observed_environment != expected_environment:
        raise EndpointError(
            f"refusing {expected_environment} rollback against {observed_environment} endpoint"
        )
    directory = (
        root
        / "lib"
        / "text"
        / "help"
        / STATE_DIRECTORY_NAME
        / "runs"
        / run_id
        / observed_environment
    )
    manifest, backup_catalog, backup_repairs, backup_file = validate_backup(directory)
    if manifest.get("state") not in {"applied", "verified-no-op"}:
        raise EndpointError(f"run {run_id!r} is not an applied rollback source")
    plan_id = str(manifest["plan_id"])
    config = DatabaseConfig.from_root(root)
    connection = config.connect()
    barrier = HelpWriteBarrier(root, plan_id)
    database_locked = False
    help_file = root / "lib" / "text" / "help" / "help.hlp"
    try:
        barrier.acquire()
        acquire_database_lock(connection, lock_timeout)
        database_locked = True
        current_data = read_database_catalog(
            connection, config.database, for_update=True, begin_transaction=True
        )
        current = current_data.catalog
        if current_data.integrity_issues:
            raise EndpointError(
                "rollback requires the exact clean applied state; integrity issues are present"
            )
        if current.content_hash != expected_current_hash:
            raise StalePlanError(
                f"rollback expected {expected_current_hash}, observed {current.content_hash}"
            )
        restore_delta = catalog_delta(
            current,
            backup_catalog,
            authorized_deletions=set(current.by_tag) - set(backup_catalog.by_tag),
        )
        apply_delta_to_database(
            connection,
            current_data.schema,
            restore_delta,
            backup_catalog,
            f"rollback-{plan_id}",
        )
        restore_integrity_rows(connection, backup_repairs)
        connection.commit()
        if backup_file is None:
            if help_file.exists():
                help_file.unlink()
        else:
            atomic_write(help_file, backup_file)
        runtime = request_runtime_reload(
            root,
            backup_catalog.content_hash,
            reload_timeout,
            require_running=observed_environment == "production",
        )
        verified = read_database_catalog(
            connection, config.database, begin_transaction=True
        )
        connection.commit()
        if verified.catalog.content_hash != backup_catalog.content_hash:
            raise EndpointError("rollback database verification failed")
        if normalize_integrity_repairs(verified.integrity_repairs) != backup_repairs:
            raise EndpointError("rollback integrity-state verification failed")
        if backup_file is not None and help_file.read_bytes() != backup_file:
            raise EndpointError("rollback file verification failed")
        manifest["state"] = "rolled-back"
        manifest["rolled_back_at"] = utc_now()
        atomic_write_json(directory / "manifest.json", manifest)
        return {
            "status": "rolled-back",
            "environment": observed_environment,
            "run_id": run_id,
            "catalog_hash": backup_catalog.content_hash,
            "runtime": runtime,
        }
    except Exception:
        connection.rollback()
        raise
    finally:
        if database_locked:
            release_database_lock(connection)
        connection.close()
        if barrier.acquired:
            barrier.release()


def write_common_baseline(root: Path, catalog: Catalog, plan_id: str) -> dict[str, Any]:
    if not _TOKEN_RE.fullmatch(plan_id):
        raise EndpointError("baseline plan ID must be a SHA-256")
    directory = root / "lib" / "text" / "help" / STATE_DIRECTORY_NAME / "baseline"
    manifest = {
        "format": "luminari-help-sync-baseline",
        "version": 1,
        "tool_version": TOOL_VERSION,
        "plan_id": plan_id,
        "catalog_hash": catalog.content_hash,
        "entry_count": len(catalog.entries),
        "relationship_count": catalog.relationship_count,
        "recorded_at": utc_now(),
    }
    atomic_write_json(directory / "catalog.json", catalog.to_dict())
    atomic_write_json(directory / "manifest.json", manifest)
    loaded = load_common_baseline(root)
    if loaded.content_hash != catalog.content_hash:
        raise EndpointError("baseline write verification failed")
    return manifest


def load_common_baseline(root: Path) -> Catalog:
    directory = root / "lib" / "text" / "help" / STATE_DIRECTORY_NAME / "baseline"
    catalog_path = directory / "catalog.json"
    manifest_path = directory / "manifest.json"
    if not catalog_path.is_file() or not manifest_path.is_file():
        raise EndpointError("no common help baseline has been initialized")
    catalog = Catalog.from_dict(json.loads(catalog_path.read_text(encoding="utf-8")))
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if catalog.content_hash != manifest.get("catalog_hash"):
        raise EndpointError("common baseline catalog hash does not match its manifest")
    return catalog
