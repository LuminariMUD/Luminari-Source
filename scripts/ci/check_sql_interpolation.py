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
  check_sql_interpolation.py              # compare against the baseline
  check_sql_interpolation.py --list       # print every counted site
  check_sql_interpolation.py --update     # lower the baseline; refuses growth
  check_sql_interpolation.py --self-test  # exercise the scanner on fixtures
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
    """Blank out C comments while leaving string and character literals intact.

    Comment-like sequences inside literals ("-- //", "/* hint */") are SQL
    text, not C comments, so a literal-aware scan is required to keep those
    sites visible to the ratchet.
    """
    out = []
    index = 0
    length = len(text)
    while index < length:
        char = text[index]
        if char == '"' or char == "'":
            quote = char
            end = index + 1
            while end < length and text[end] != quote:
                end += 2 if text[end] == "\\" else 1
            end = min(end + 1, length)
            out.append(text[index:end])
            index = end
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append(re.sub(r"[^\n]", " ", text[index:end]))
            index = end
        elif text.startswith("//", index):
            end = text.find("\n", index)
            end = length if end < 0 else end
            index = end
        else:
            out.append(char)
            index += 1
    return "".join(out)


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


def count_sites_in_text(text):
    """Count formatted SQL sites in one C source text (shared by the self-test)."""
    stripped = strip_comments(text)
    count = 0
    for _offset, call_text in iter_calls(stripped):
        fmt = call_format_string(call_text)
        if KEYWORD_PATTERN.search(fmt) and CONVERSION_PATTERN.search(fmt):
            count += 1
    return count


SELF_TEST_CASES = [
    # (description, source, expected count)
    ("plain formatted select",
     'snprintf(q, sizeof(q), "SELECT a FROM t WHERE b = %d", b);', 1),
    ("placeholders only are not data",
     'snprintf(q, sizeof(q), "SELECT a FROM t WHERE b = ?");', 0),
    ("block comment does not hide a site",
     '/* SELECT %s */ snprintf(q, sizeof(q), "DELETE FROM t WHERE k = \'%s\'", k);', 1),
    ("line comment site is not counted",
     '// snprintf(q, sizeof(q), "SELECT a FROM t WHERE b = %d", b);\nint x;', 0),
    ("line comment sequence inside a literal is kept",
     'snprintf(q, sizeof(q), "SELECT a FROM t WHERE b = %d -- // note", b);', 1),
    ("block comment sequence inside a literal is kept",
     'snprintf(q, sizeof(q), "/* hint */ SELECT a FROM t WHERE b = %d", b);', 1),
    ("comment opener inside a character literal",
     "char c = '/'; char d = '*'; snprintf(q, sizeof(q), \"UPDATE t SET a = %d\", a);", 1),
    ("escaped quote inside a literal",
     'snprintf(q, sizeof(q), "INSERT INTO t (a) VALUES (\'%s\') /* \\" */", a);', 1),
    ("non-sql format text",
     'snprintf(buf, sizeof(buf), "You set %s down from where it came.", name);', 0),
]


def self_test():
    failures = 0
    for description, source, expected in SELF_TEST_CASES:
        actual = count_sites_in_text(source)
        if actual != expected:
            failures += 1
            print("self-test FAILED: %s (expected %d, got %d)" % (description, expected, actual),
                  file=sys.stderr)
    if failures:
        return 1
    print("sql interpolation self-test: %d cases passed" % len(SELF_TEST_CASES))
    return 0


def main(argv):
    if "--self-test" in argv:
        return self_test()
    sites = collect_sites()
    if "--list" in argv:
        for path in sorted(sites):
            for line in sites[path]:
                print("%s:%d" % (path, line))
        print("total: %d" % sum(len(lines) for lines in sites.values()))
        return 0
    if "--update" in argv:
        baseline = read_baseline()
        grown = [(path, baseline.get(path, 0), len(lines)) for path, lines in sorted(sites.items())
                 if len(lines) > baseline.get(path, 0)]
        if baseline and grown:
            print("Refusing to record baseline growth; migrate or justify these sites first:",
                  file=sys.stderr)
            for path, allowed, now in grown:
                print("  %s: %d site(s), baseline allows %d" % (path, now, allowed), file=sys.stderr)
            return 1
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
