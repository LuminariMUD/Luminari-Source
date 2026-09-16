#!/usr/bin/env python3
"""Fail when a CodeQL database did not extract every production source.

The Security workflow builds the server under CodeQL's tracer. A production
source the traced build never compiled would be missing from every CodeQL
result without any error. The database's source archive (src.zip) holds each
file the extractor saw, keyed by its absolute path without the leading slash;
this check requires every C source in Makefile.am's luminari_SOURCES there.

Usage:
  check_codeql_coverage.py --database DIR [--source-root DIR]
  check_codeql_coverage.py --db-locations JSON [--source-root DIR]

--db-locations takes the analyze action's db-locations output, a JSON object
mapping each language to its database directory.
"""

import argparse
import json
import os
import sys
import zipfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

from check_build_parity import MAKE_REFERENCE, expand, parse_makefile_am  # noqa: E402


def production_sources(root):
    variables = parse_makefile_am((root / "Makefile.am").read_text())
    return sorted(
        name
        for name in expand(variables, variables.get("luminari_SOURCES", []), MAKE_REFERENCE)
        if name.endswith(".c")
    )


def missing_sources(archive_names, root, sources):
    prefix = os.path.realpath(root).lstrip("/")
    names = set(archive_names)
    return [source for source in sources if f"{prefix}/{source}" not in names]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    where = parser.add_mutually_exclusive_group(required=True)
    where.add_argument("--database", type=Path, help="a CodeQL database directory")
    where.add_argument("--db-locations", help="the analyze action's db-locations JSON")
    parser.add_argument(
        "--source-root",
        type=Path,
        default=SCRIPT_DIR.parents[1],
        help="the checkout the database was built from (default: this repository)",
    )
    args = parser.parse_args()

    databases = [args.database] if args.database else list(json.loads(args.db_locations).values())
    if not databases:
        print("no CodeQL database was reported", file=sys.stderr)
        return 1
    sources = production_sources(args.source_root)
    if not sources:
        print("Makefile.am luminari_SOURCES names no C sources", file=sys.stderr)
        return 1

    status = 0
    for database in databases:
        archive = Path(database) / "src.zip"
        if not archive.is_file():
            print(f"{archive} does not exist", file=sys.stderr)
            status = 1
            continue
        with zipfile.ZipFile(archive) as handle:
            missing = missing_sources(handle.namelist(), args.source_root, sources)
        if missing:
            print(f"{archive} lacks {len(missing)} production sources:", file=sys.stderr)
            for source in missing:
                print(f"  {source}", file=sys.stderr)
            status = 1
        else:
            print(f"{archive}: all {len(sources)} production sources were extracted")
    return status


if __name__ == "__main__":
    sys.exit(main())
