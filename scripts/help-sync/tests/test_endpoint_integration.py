import ast
import os
from pathlib import Path
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock
import uuid

INTEGRATION_ENABLED = os.environ.get("LUMINARI_HELP_SYNC_INTEGRATION") == "1"
if INTEGRATION_ENABLED:
    import pymysql
else:
    pymysql = None

HELP_SYNC_DIRECTORY = Path(__file__).resolve().parents[1]
REPOSITORY_ROOT = HELP_SYNC_DIRECTORY.parents[1]
sys.path.insert(0, str(HELP_SYNC_DIRECTORY))

from catalog import Catalog, HelpEntry, MergeResult, build_plan_core, seal_plan  # noqa: E402
import endpoint  # noqa: E402
from endpoint import (  # noqa: E402
    ApplyFailure,
    DatabaseConfig,
    EndpointError,
    StalePlanError,
    apply_plan_to_endpoint,
    normalize_integrity_repairs,
    inspect_schema,
    repair_missing_keywords,
    rollback_endpoint,
    take_snapshot,
    verify_endpoint,
)


SCHEMA_STATEMENTS = (
    """
    CREATE TABLE help_entries (
      id INT AUTO_INCREMENT PRIMARY KEY,
      tag VARCHAR(50) NOT NULL UNIQUE,
      alternate_keywords TEXT DEFAULT NULL,
      category VARCHAR(50) NOT NULL DEFAULT 'general',
      entry LONGTEXT NOT NULL,
      min_level INT NOT NULL DEFAULT 0,
      max_level INT NOT NULL DEFAULT 1000,
      auto_generated BOOLEAN NOT NULL DEFAULT FALSE,
      last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE help_keywords (
      id INT AUTO_INCREMENT PRIMARY KEY,
      help_tag VARCHAR(50) NOT NULL,
      keyword VARCHAR(100) NOT NULL,
      INDEX idx_keyword (keyword),
      INDEX idx_help_tag (help_tag),
      UNIQUE KEY unique_tag_keyword (help_tag, keyword)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE help_related_topics (
      source_tag VARCHAR(50) NOT NULL,
      related_tag VARCHAR(50) NOT NULL,
      relevance_score DECIMAL(12,6) NOT NULL DEFAULT 1.0,
      PRIMARY KEY (source_tag, related_tag)
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE help_versions (
      id BIGINT AUTO_INCREMENT PRIMARY KEY,
      tag VARCHAR(50) NOT NULL,
      alternate_keywords TEXT DEFAULT NULL,
      entry LONGTEXT,
      min_level INT DEFAULT 0,
      max_level INT DEFAULT 1000,
      category VARCHAR(50) DEFAULT 'general',
      auto_generated BOOLEAN DEFAULT FALSE,
      changed_by VARCHAR(50),
      change_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
      change_type ENUM('CREATE', 'UPDATE', 'DELETE') DEFAULT 'UPDATE',
      sync_plan_id VARCHAR(80) DEFAULT NULL
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE help_search_history (
      id BIGINT AUTO_INCREMENT PRIMARY KEY,
      search_term VARCHAR(200) NOT NULL
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
    """
    CREATE TABLE unrelated_sentinel (
      id INT PRIMARY KEY,
      value_text VARCHAR(100) NOT NULL
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
    """,
)


def embedded_help_migrations():
    source = (REPOSITORY_ROOT / "src" / "db_init.c").read_text(encoding="utf-8")
    pattern = re.compile(
        r"apply_migration\((20260824\d+),\s*\"(?:\\.|[^\"])*\",\s*"
        r"((?:\"(?:\\.|[^\"])*\"\s*)+)\)"
    )
    migrations = []
    for version, literals in pattern.findall(source):
        sql = "".join(
            ast.literal_eval(literal)
            for literal in re.findall(r'\"(?:\\.|[^\"])*\"', literals)
        )
        migrations.append((int(version), sql))
    return sorted(migrations)


@unittest.skipUnless(
    INTEGRATION_ENABLED,
    "set LUMINARI_HELP_SYNC_INTEGRATION=1 for isolated MariaDB tests",
)
class EndpointDatabaseIntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary_directory = tempfile.TemporaryDirectory()
        temporary_root = Path(cls.temporary_directory.name)
        data_directory = temporary_root / "mariadb-data"
        socket_path = temporary_root / "mariadb.sock"
        log_path = temporary_root / "mariadb.log"
        install_program = shutil.which("mariadb-install-db")
        server_program = shutil.which("mariadbd") or "/usr/sbin/mariadbd"
        if not install_program or not Path(server_program).is_file():
            raise unittest.SkipTest("local MariaDB server tools are unavailable")
        subprocess.run(
            [
                install_program,
                "--no-defaults",
                f"--datadir={data_directory}",
                "--auth-root-authentication-method=normal",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=True,
        )
        listener = socket.socket()
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
        listener.close()
        cls.server_process = subprocess.Popen(
            [
                server_program,
                "--no-defaults",
                f"--datadir={data_directory}",
                f"--socket={socket_path}",
                f"--port={port}",
                "--bind-address=127.0.0.1",
                "--skip-grant-tables",
                f"--pid-file={temporary_root / 'mariadb.pid'}",
                f"--log-error={log_path}",
            ],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        deadline = time.monotonic() + 20
        cls.server_connection = None
        while time.monotonic() < deadline:
            if cls.server_process.poll() is not None:
                break
            try:
                cls.server_connection = pymysql.connect(
                    host="127.0.0.1",
                    port=port,
                    user="root",
                    password="unused",
                    autocommit=True,
                )
                break
            except pymysql.MySQLError:
                time.sleep(0.1)
        if cls.server_connection is None:
            cls.server_process.terminate()
            cls.server_process.wait(timeout=5)
            raise RuntimeError("isolated MariaDB server did not start")

        cls.database_name = "luminari_help_sync_test_" + uuid.uuid4().hex[:12]
        with cls.server_connection.cursor() as cursor:
            cursor.execute(f"CREATE DATABASE `{cls.database_name}` CHARACTER SET utf8mb4")
        cls.server_config = DatabaseConfig(
            host="127.0.0.1",
            database=cls.database_name,
            username="root",
            password="unused",
            port=port,
        )
        cls.root = temporary_root / "endpoint"
        help_directory = cls.root / "lib" / "text" / "help"
        help_directory.mkdir(parents=True)
        (cls.root / "lib" / ".env").write_text(
            "APP_ENV=development\n", encoding="ascii"
        )
        mysql_config = (
            f"mysql_host={cls.server_config.host}\n"
            f"mysql_port={cls.server_config.port}\n"
            f"mysql_database={cls.database_name}\n"
            f"mysql_username={cls.server_config.username}\n"
            f"mysql_password={cls.server_config.password}\n"
            f"mysql_charset={cls.server_config.charset}\n"
        )
        config_path = cls.root / "lib" / "mysql_config"
        config_path.write_text(mysql_config, encoding="utf-8")
        config_path.chmod(0o600)
        cls.config = DatabaseConfig.from_root(cls.root)

    @classmethod
    def tearDownClass(cls):
        try:
            with cls.server_connection.cursor() as cursor:
                cursor.execute(f"DROP DATABASE `{cls.database_name}`")
        finally:
            cls.server_connection.close()
            cls.server_process.terminate()
            cls.server_process.wait(timeout=10)
            cls.temporary_directory.cleanup()

    def setUp(self):
        connection = self.config.connect(autocommit=True)
        with connection.cursor() as cursor:
            cursor.execute("SET FOREIGN_KEY_CHECKS=0")
            for table in (
                "help_related_topics",
                "help_keywords",
                "help_entries",
                "help_versions",
                "help_search_history",
                "unrelated_sentinel",
                "schema_migrations",
            ):
                cursor.execute(f"DROP TABLE IF EXISTS `{table}`")
            for statement in SCHEMA_STATEMENTS:
                cursor.execute(statement)
            cursor.execute(
                "INSERT INTO help_entries "
                "(tag, alternate_keywords, entry, min_level, max_level, category, auto_generated) "
                "VALUES "
                "('alpha', 'FIRST', 'Alpha old.\\n', 0, 1000, 'general', FALSE), "
                "('beta', NULL, 'Beta old.\\n', 1, 1000, 'general', FALSE)"
            )
            cursor.execute(
                "INSERT INTO help_keywords (help_tag, keyword) VALUES "
                "('alpha', 'ALPHA'), ('ghost', 'ORPHAN')"
            )
            cursor.execute(
                "INSERT INTO help_search_history (search_term) VALUES ('keep-me')"
            )
            cursor.execute(
                "INSERT INTO unrelated_sentinel (id, value_text) VALUES (1, 'untouched')"
            )
        connection.close()
        (self.root / "lib" / "text" / "help" / "help.hlp").write_bytes(b"$~\n")

    def build_plan(self, candidate_transform=None):
        snapshot = take_snapshot(self.root, "development")
        candidate = repair_missing_keywords(snapshot.catalog)
        if candidate_transform is not None:
            candidate = candidate_transform(candidate)
        result = MergeResult(candidate, (), (), ())
        core = build_plan_core(
            snapshot.catalog,
            snapshot.catalog,
            snapshot.catalog,
            result,
        )
        core["sources"] = {
            "base": snapshot.catalog.to_dict(),
            "development": snapshot.catalog.to_dict(),
            "production": snapshot.catalog.to_dict(),
        }
        core["development_integrity_repairs"] = normalize_integrity_repairs(
            snapshot.integrity_repairs
        )
        core["production_integrity_repairs"] = normalize_integrity_repairs(
            snapshot.integrity_repairs
        )
        core["development_state"] = {"file_hash": snapshot.file_hash}
        core["production_state"] = {"file_hash": snapshot.file_hash}
        core["repair_layers"] = True
        return seal_plan(core), snapshot

    @staticmethod
    def changed_candidate(catalog):
        entries = list(catalog.entries)
        alpha = entries[0]
        entries[0] = HelpEntry(
            tag=alpha.tag,
            body="Alpha new.\n",
            keywords=alpha.keywords,
            aliases=alpha.aliases,
            min_level=alpha.min_level,
            max_level=alpha.max_level,
            category=alpha.category,
            auto_generated=alpha.auto_generated,
        )
        entries.append(HelpEntry(tag="gamma", body="Gamma.\n", keywords=("GAMMA",)))
        return Catalog(tuple(entries))

    def scalar(self, query):
        connection = self.config.connect()
        try:
            with connection.cursor() as cursor:
                cursor.execute(query)
                row = cursor.fetchone()
                return next(iter(row.values()))
        finally:
            connection.close()

    def test_apply_verify_idempotence_unrelated_tables_and_rollback(self):
        plan, before = self.build_plan(self.changed_candidate)
        first = apply_plan_to_endpoint(self.root, "development", plan)
        candidate = Catalog.from_dict(plan["candidate"])
        verification = verify_endpoint(self.root, "development", candidate)
        self.assertEqual(verification["catalog_hash"], candidate.content_hash)
        self.assertEqual(self.scalar("SELECT COUNT(*) FROM help_search_history"), 1)
        self.assertEqual(self.scalar("SELECT value_text FROM unrelated_sentinel WHERE id=1"), "untouched")
        second = apply_plan_to_endpoint(self.root, "development", plan)
        self.assertEqual(second["status"], "verified-no-op")
        rolled_back = rollback_endpoint(
            self.root, "development", first["run_id"], candidate.content_hash
        )
        self.assertEqual(rolled_back["catalog_hash"], before.catalog.content_hash)
        restored = take_snapshot(self.root, "development")
        self.assertEqual(restored.catalog.content_hash, before.catalog.content_hash)
        self.assertEqual(
            normalize_integrity_repairs(restored.integrity_repairs),
            normalize_integrity_repairs(before.integrity_repairs),
        )
        self.assertEqual(
            (self.root / "lib" / "text" / "help" / "help.hlp").read_bytes(), b"$~\n"
        )

    def test_stale_plan_is_refused_after_database_edit(self):
        plan, _ = self.build_plan(self.changed_candidate)
        connection = self.config.connect()
        with connection.cursor() as cursor:
            cursor.execute("UPDATE help_entries SET entry='Staff changed.\\n' WHERE tag='alpha'")
        connection.commit()
        connection.close()
        with self.assertRaises(StalePlanError):
            apply_plan_to_endpoint(self.root, "development", plan)

    def test_stale_plan_is_refused_after_help_file_edit(self):
        plan, _ = self.build_plan(self.changed_candidate)
        (self.root / "lib" / "text" / "help" / "help.hlp").write_bytes(
            b"CHANGED\n\nChanged after planning.\n#0\n$~\n"
        )
        with self.assertRaises(StalePlanError):
            apply_plan_to_endpoint(self.root, "development", plan)

    def test_database_partial_failure_rolls_back_transaction(self):
        plan, before = self.build_plan(self.changed_candidate)

        def fail_after_write(connection, schema, delta, candidate, plan_id):
            with connection.cursor() as cursor:
                cursor.execute("UPDATE help_entries SET entry='partial' WHERE tag='alpha'")
            raise EndpointError("injected database failure")

        with mock.patch("endpoint.apply_delta_to_database", side_effect=fail_after_write):
            with self.assertRaises(ApplyFailure):
                apply_plan_to_endpoint(self.root, "development", plan)
        after = take_snapshot(self.root, "development")
        self.assertEqual(after.catalog.content_hash, before.catalog.content_hash)

    def test_atomic_file_failure_triggers_compensating_rollback(self):
        plan, before = self.build_plan(self.changed_candidate)
        original_atomic_write = endpoint.atomic_write
        live_help_file = self.root / "lib" / "text" / "help" / "help.hlp"
        failed = False

        def fail_first_help_file(path, data, mode=0o640):
            nonlocal failed
            if path == live_help_file and not failed:
                failed = True
                raise OSError("injected atomic install failure")
            return original_atomic_write(path, data, mode)

        with mock.patch("endpoint.atomic_write", side_effect=fail_first_help_file):
            with self.assertRaises(ApplyFailure) as context:
                apply_plan_to_endpoint(self.root, "development", plan)
        self.assertEqual(
            context.exception.details["automatic_rollback"]["status"], "rolled-back"
        )
        restored = take_snapshot(self.root, "development")
        self.assertEqual(restored.catalog.content_hash, before.catalog.content_hash)

    def test_environment_refusal_occurs_before_database_access(self):
        plan, _ = self.build_plan(self.changed_candidate)
        (self.root / "lib" / ".env").write_text("APP_ENV=testing\n", encoding="ascii")
        try:
            with self.assertRaises(EndpointError):
                apply_plan_to_endpoint(self.root, "development", plan)
        finally:
            (self.root / "lib" / ".env").write_text(
                "APP_ENV=development\n", encoding="ascii"
            )

    def test_embedded_migrations_upgrade_legacy_schema_despite_low_id_collision(self):
        migrations = embedded_help_migrations()
        self.assertEqual([version for version, _ in migrations], list(range(2026082401, 2026082409)))
        connection = self.config.connect(autocommit=True)
        with connection.cursor() as cursor:
            for table in (
                "help_related_topics",
                "help_keywords",
                "help_entries",
                "help_versions",
                "help_search_history",
            ):
                cursor.execute(f"DROP TABLE IF EXISTS `{table}`")
            cursor.execute(
                "CREATE TABLE help_entries ("
                "tag VARCHAR(50) PRIMARY KEY, alternate_keywords TEXT, entry TEXT NOT NULL, "
                "min_level INT DEFAULT 0, auto_generated BOOLEAN DEFAULT FALSE, "
                "last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
            )
            cursor.execute(
                "CREATE TABLE help_keywords ("
                "id INT AUTO_INCREMENT PRIMARY KEY, help_tag VARCHAR(50) NOT NULL, "
                "keyword VARCHAR(50) NOT NULL"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
            )
            cursor.execute(
                "CREATE TABLE schema_migrations ("
                "version INT PRIMARY KEY, description VARCHAR(255) NOT NULL, "
                "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"
            )
            cursor.executemany(
                "INSERT INTO schema_migrations (version, description) VALUES (%s, %s)",
                [(version, "unrelated legacy migration") for version in range(1, 5)],
            )
            for _, sql in migrations:
                cursor.execute(sql)
        schema = inspect_schema(connection, self.database_name)
        connection.close()
        self.assertTrue(schema.write_ready, schema.missing_requirements)
        self.assertTrue(
            {
                "alternate_keywords",
                "max_level",
                "category",
                "auto_generated",
                "changed_by",
                "change_date",
                "change_type",
                "sync_plan_id",
            }.issubset(schema.tables["help_versions"])
        )


if __name__ == "__main__":
    unittest.main()
