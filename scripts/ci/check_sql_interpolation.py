#!/usr/bin/env python3
"""Block new formatted SQL that interpolates data values.

Issue #98 migrates dynamic SQL to bound prepared statements. Until every call
site is migrated, this check ratchets: each source file may contain at most the
number of formatted-SQL sites recorded in the baseline. Adding a site fails the
check; removing sites is allowed and should be followed by ``--update`` so the
baseline only ever shrinks.

A "formatted SQL site" is a printf-style call (snprintf, sprintf, asprintf,
snprintf_append, strcat, strncat, strlcat) whose format string contains an SQL
statement or clause opener (SELECT, INSERT INTO, UPDATE ... SET, DELETE FROM,
REPLACE INTO, WHERE/AND/OR comparisons, ORDER BY, GROUP BY, LIMIT) and at least
one ``%`` conversion. The match is lexical; a rare non-SQL format that reads
like a clause is acceptable noise because only growth fails the check. Placeholder-only construction
(``"%s(?,?)"``) is not a data interpolation, but it is counted conservatively
when it mentions a keyword; keep placeholder building free of SQL keywords.

Usage:
  check_sql_interpolation.py            # compare against the baseline
  check_sql_interpolation.py --list     # print every counted site
  check_sql_interpolation.py --update   # rewrite the baseline from the tree
"""

import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE_ROOT = os.path.join(REPO_ROOT, "src")
BASELINE_PATH = os.path.join(REPO_ROOT, "scripts", "ci", "sql_interpolation_baseline.txt")

CALL_PATTERN = re.compile(
    r"\b(snprintf|sprintf|asprintf|snprintf_append|strcat|strncat|strlcat)\s*\("
)
KEYWORD_PATTERN = re.compile(
    r"\b(SELECT\s|INSERT\s+(?:IGNORE\s+)?INTO\s|DELETE\s+FROM\s|REPLACE\s+INTO\s"
    r"|UPDATE\s+\w+\s+SET\s|(?:WHERE|AND|OR)\s+[\w.]+\s*(?:=|<|>|IN\b|LIKE\b)"
    r"|ORDER\s+BY\s|GROUP\s+BY\s|LIMIT\s+%)",
    re.IGNORECASE,
)
CONVERSION_PATTERN = re.compile(r"%[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z|j|t|L)?[diouxXeEfFgGscp]")
STRING_LITERAL_PATTERN = re.compile(r'"((?:[^"\\]|\\.)*)"')


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", lambda match: " " * len(match.group(0)), text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def iter_calls(text):
    """Yield (offset, call_text) for each printf-style call in the file."""
    for match in CALL_PATTERN.finditer(text):
        depth = 1
        index = match.end()
        while index < len(text) and depth:
            char = text[index]
            if char == '"':
                literal = STRING_LITERAL_PATTERN.match(text, index)
                index = literal.end() if literal else index + 1
                continue
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
            index += 1
        yield match.start(), text[match.start():index]


def call_format_string(call_text):
    return "".join(STRING_LITERAL_PATTERN.findall(call_text))


def collect_sites():
    sites = {}
    for directory, _, files in os.walk(SOURCE_ROOT):
        for name in sorted(files):
            if not name.endswith(".c"):
                continue
            path = os.path.join(directory, name)
            relative = os.path.relpath(path, REPO_ROOT)
            with open(path, encoding="utf-8", errors="replace") as handle:
                text = strip_comments(handle.read())
            for offset, call_text in iter_calls(text):
                fmt = call_format_string(call_text)
                if KEYWORD_PATTERN.search(fmt) and CONVERSION_PATTERN.search(fmt):
                    line = text.count("\n", 0, offset) + 1
                    sites.setdefault(relative, []).append(line)
    return sites


def read_baseline():
    baseline = {}
    if not os.path.exists(BASELINE_PATH):
        return baseline
    with open(BASELINE_PATH, encoding="utf-8") as handle:
        for raw in handle:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            path, count = line.rsplit(" ", 1)
            baseline[path] = int(count)
    return baseline


def write_baseline(sites):
    with open(BASELINE_PATH, "w", encoding="utf-8") as handle:
        handle.write("# Formatted SQL sites per file (issue #98 ratchet).\n")
        handle.write("# Regenerate only after removing sites:\n")
        handle.write("#   python3 scripts/ci/check_sql_interpolation.py --update\n")
        for path in sorted(sites):
            handle.write("%s %d\n" % (path, len(sites[path])))


def main(argv):
    sites = collect_sites()
    if "--list" in argv:
        for path in sorted(sites):
            for line in sites[path]:
                print("%s:%d" % (path, line))
        print("total: %d" % sum(len(lines) for lines in sites.values()))
        return 0
    if "--update" in argv:
        write_baseline(sites)
        print("baseline written: %d files, %d sites" % (len(sites), sum(map(len, sites.values()))))
        return 0

    baseline = read_baseline()
    failures = []
    for path, lines in sorted(sites.items()):
        allowed = baseline.get(path, 0)
        if len(lines) > allowed:
            failures.append((path, allowed, lines))
    if failures:
        print("New formatted SQL with data values detected. Bind values with prepared", file=sys.stderr)
        print("statements (see docs/systems/DATABASE_INTEGRATION.md) instead.", file=sys.stderr)
        for path, allowed, lines in failures:
            print("  %s: %d site(s), baseline allows %d; lines %s"
                  % (path, len(lines), allowed, ",".join(map(str, lines))), file=sys.stderr)
        return 1

    shrunk = [(path, count, len(sites.get(path, []))) for path, count in sorted(baseline.items())
              if len(sites.get(path, [])) < count]
    total = sum(len(lines) for lines in sites.values())
    print("sql interpolation check: %d formatted SQL sites, within baseline" % total)
    for path, count, now in shrunk:
        print("  %s dropped from %d to %d; run --update to lower the baseline" % (path, count, now))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
