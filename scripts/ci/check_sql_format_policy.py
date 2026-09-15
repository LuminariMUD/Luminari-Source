#!/usr/bin/env python3
"""Keep every future SQL file under the sqlfluff formatter.

The sqlfluff pre-commit hook formats tracked SQL with the layout-only settings
in `.sqlfluff`. Legacy files that the MariaDB dialect cannot parse are exempt
through `.sqlfluffignore`; that list may shrink but never grow. This check fails
on anything that would let another SQL file skip formatting:

- a `.sqlfluffignore` entry outside the frozen legacy list, patterns included;
- sqlfluff configuration or ignore files anywhere but the repository root, or
  sqlfluff settings in setup.cfg, tox.ini, or pyproject.toml;
- a root `.sqlfluff` whose settings differ from the reviewed configuration;
- an inline `sqlfluff:` configuration comment in a tracked SQL file;
- a sqlfluff pre-commit hook that is missing, duplicated, or narrowed;
- a top-level `files` or `exclude` pattern in `.pre-commit-config.yaml` that
  keeps a tracked SQL file outside the frozen list away from every hook.

Inline `noqa` comments need no check: `.sqlfluff` sets `disable_noqa = True`.

Usage:
  check_sql_format_policy.py              # check the tracked tree
  check_sql_format_policy.py --self-test  # prove each bypass is rejected
"""

from __future__ import annotations

import argparse
import configparser
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]

FROZEN_EXEMPTIONS = frozenset(
    {
        "lib/pubsub_v3_schema.sql",
        "sql/components/ai_region_hints_schema.sql",
        "sql/components/ai_service_migration.sql",
        "sql/components/dynamic_descriptions_deployment.sql",
        "sql/components/help_gdb_binary_path.sql",
        "sql/components/help_race_yuan_ti_entries.sql",
        "sql/components/help_rol_feat_entries.sql",
        "sql/components/help_rol_player_kits.sql",
        "sql/components/help_system_indexes.sql",
        "sql/components/help_vessel_entries.sql",
        "sql/components/narrative_weaver_installation.sql",
        "sql/components/pubsub_v3_schema.sql",
        "sql/components/verify_help_rol_feat_entries.sql",
        "sql/components/verify_help_rol_player_kits.sql",
        "sql/components/verify_help_specproc_entries.sql",
        "sql/components/verify_help_vessel_entries.sql",
        "sql/components/vessels_phase2_schema.sql",
        "sql/master_schema.sql",
    }
)

EXPECTED_SETTINGS = {
    "sqlfluff": {
        "dialect": "mariadb",
        "templater": "raw",
        "disable_noqa": "True",
        "large_file_skip_byte_limit": "0",
        "max_line_length": "100",
        "exclude_rules": "LT05, LT12, CP01, CP02, CP03, CP04, CP05",
    },
    "sqlfluff:indentation": {"tab_space_size": "2"},
}

SQLFLUFF_REPO = "https://github.com/sqlfluff/sqlfluff"
EXPECTED_HOOK = {
    "id": "sqlfluff-fix",
    "name": "sqlfluff format",
    "entry": "sqlfluff format --processes 0 --disable-progress-bar",
}

CONFIG_NAMES = frozenset({".sqlfluff", ".sqlfluffignore"})
SHARED_CONFIG_NAMES = frozenset({"setup.cfg", "tox.ini", "pyproject.toml"})
INLINE_DIRECTIVE = re.compile(r"(--|#|/\*)\s*sqlfluff\s*:", re.IGNORECASE)


def tracked_files(root: Path) -> list[str]:
    """Return tracked and staged paths relative to root."""
    output = subprocess.run(
        ["git", "ls-files", "-z"], cwd=root, check=True, capture_output=True
    ).stdout
    return [path for path in output.decode("utf-8").split("\0") if path]


def check(root: Path, files: list[str]) -> list[str]:
    """Return one message per policy violation."""
    problems = []

    ignore = root / ".sqlfluffignore"
    if ignore.is_file():
        lines = ignore.read_text(encoding="utf-8").splitlines()
        for number, line in enumerate(lines, 1):
            entry = line.strip()
            if entry and not entry.startswith("#") and entry not in FROZEN_EXEMPTIONS:
                problems.append(
                    f".sqlfluffignore:{number}: {entry} is not a frozen legacy exemption"
                )

    for path in files:
        name = Path(path).name
        if not (root / path).is_file():
            continue
        text = (root / path).read_text(encoding="utf-8", errors="replace")
        if name in CONFIG_NAMES and path != name:
            problems.append(f"{path}: sqlfluff configuration belongs only at the root")
        elif name in SHARED_CONFIG_NAMES and "sqlfluff" in text:
            problems.append(f"{path}: sqlfluff settings belong only in .sqlfluff")
        elif path.endswith(".sql") and INLINE_DIRECTIVE.search(text):
            problems.append(f"{path}: inline sqlfluff configuration is not allowed")

    parser = configparser.ConfigParser(interpolation=None)
    parser.read(root / ".sqlfluff", encoding="utf-8")
    settings = {section: dict(parser[section]) for section in parser.sections()}
    if settings != EXPECTED_SETTINGS:
        problems.append(".sqlfluff: settings differ from the reviewed configuration")

    config_path = root / ".pre-commit-config.yaml"
    config = yaml.safe_load(config_path.read_text(encoding="utf-8")) or {}
    hooks = [
        hook
        for repo in config.get("repos", [])
        if repo.get("repo") == SQLFLUFF_REPO
        for hook in repo.get("hooks", [])
    ]
    if hooks != [EXPECTED_HOOK]:
        problems.append(
            f".pre-commit-config.yaml: the sqlfluff hook must be exactly {EXPECTED_HOOK}"
        )

    # pre-commit applies these top-level patterns to every hook.
    include = re.compile(config.get("files", ""))
    exclude = re.compile(config.get("exclude", "^$"))
    for path in files:
        if path.endswith(".sql") and path not in FROZEN_EXEMPTIONS:
            if not include.search(path) or exclude.search(path):
                problems.append(
                    f"{path}: a top-level files or exclude pattern in "
                    ".pre-commit-config.yaml keeps this file from the sqlfluff hook"
                )
    return problems


def write_compliant_tree(root: Path) -> list[str]:
    """Create a minimal tree that satisfies the policy; return its files."""
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_dict(EXPECTED_SETTINGS)
    with (root / ".sqlfluff").open("w", encoding="utf-8") as handle:
        parser.write(handle)
    (root / ".sqlfluffignore").write_text("# legacy\nsql/master_schema.sql\n", encoding="utf-8")
    config = {"repos": [{"repo": SQLFLUFF_REPO, "rev": "4.3.0", "hooks": [dict(EXPECTED_HOOK)]}]}
    (root / ".pre-commit-config.yaml").write_text(yaml.safe_dump(config), encoding="utf-8")
    (root / "sql").mkdir()
    (root / "sql/master_schema.sql").write_text("SELECT 1;\n", encoding="utf-8")
    (root / "sql/new.sql").write_text("SELECT 1;\n", encoding="utf-8")
    return [
        ".sqlfluff",
        ".sqlfluffignore",
        ".pre-commit-config.yaml",
        "sql/master_schema.sql",
        "sql/new.sql",
    ]


def append(path: str, text: str):
    """Return a mutation that appends text to path and tracks it."""

    def mutate(root: Path, files: list[str]) -> None:
        target = root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("a", encoding="utf-8") as handle:
            handle.write(text)
        if path not in files:
            files.append(path)

    return mutate


def narrow_hook(root: Path, files: list[str]) -> None:
    """Add an exclude pattern to the sqlfluff hook."""
    path = root / ".pre-commit-config.yaml"
    config = yaml.safe_load(path.read_text(encoding="utf-8"))
    config["repos"][0]["hooks"][0]["exclude"] = "^sql/new\\.sql$"
    path.write_text(yaml.safe_dump(config), encoding="utf-8")


def set_top_level(key: str, pattern: str):
    """Return a mutation that sets a top-level pre-commit file pattern."""

    def mutate(root: Path, files: list[str]) -> None:
        path = root / ".pre-commit-config.yaml"
        config = yaml.safe_load(path.read_text(encoding="utf-8"))
        config[key] = pattern
        path.write_text(yaml.safe_dump(config), encoding="utf-8")

    return mutate


def self_test() -> int:
    """Check a compliant tree passes and every known bypass fails."""
    cases = {
        "new exemption": append(".sqlfluffignore", "sql/new.sql\n"),
        "exemption pattern": append(".sqlfluffignore", "sql/*.sql\n"),
        "nested config": append("sql/.sqlfluff", "[sqlfluff]\nexclude_rules = LT01\n"),
        "nested ignore": append("sql/.sqlfluffignore", "new.sql\n"),
        "shared config": append("pyproject.toml", "[tool.sqlfluff.core]\nignore = 'parsing'\n"),
        "changed settings": append(".sqlfluff", "ignore = parsing\n"),
        "inline directive": append("sql/new.sql", "-- sqlfluff:exclude_rules:LT01\n"),
        "narrowed hook": narrow_hook,
        "global exclude": set_top_level("exclude", "^sql/new\\.sql$"),
        "global files": set_top_level("files", "^src/"),
    }
    failures = []
    with tempfile.TemporaryDirectory() as temporary:
        base = Path(temporary) / "base"
        base.mkdir()
        files = write_compliant_tree(base)
        if check(base, files):
            failures.append(f"compliant tree rejected: {check(base, files)}")
        for name, mutate in cases.items():
            root = Path(temporary) / name.replace(" ", "-")
            shutil.copytree(base, root)
            case_files = list(files)
            mutate(root, case_files)
            if not check(root, case_files):
                failures.append(f"{name}: not rejected")
    for failure in failures:
        print(f"sql format policy self-test: {failure}", file=sys.stderr)
    if not failures:
        print(f"sql format policy self-test: {len(cases)} bypasses rejected")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--self-test", action="store_true", help="prove each bypass is rejected")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    problems = check(ROOT, tracked_files(ROOT))
    for problem in problems:
        print(f"sql format policy: {problem}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
