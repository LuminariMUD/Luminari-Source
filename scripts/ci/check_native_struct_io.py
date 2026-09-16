#!/usr/bin/env python3
"""Block files read or written as native C structures (issue #95).

A structure copied to or from a file inherits the compiler's padding, integer
widths, byte order, and even pointer values, so the file means something only
to the build that wrote it. Durable binary files are encoded field by field
with src/core/binary_formats.c instead; docs/systems/BINARY_FILE_FORMATS.md
has the rules.

Every C source and header under src/ and util/ is scanned, and these calls are
reported:

* fread() or fwrite() whose element size is not one byte: anything other than
  1, sizeof(char), sizeof(unsigned char), sizeof(signed char), sizeof(uint8_t),
  or sizeof(int8_t);
* read(), write(), pread(), or pwrite() whose byte count uses sizeof(struct ...).

Comments and string literals are ignored, as are declarations such as
``size_t fread(void *ptr, size_t size, ...)`` and calls through members such
as ``ops->write(...)``. The tree has no such call, so there is no baseline:
any finding fails.

Usage:
  check_native_struct_io.py              # scan src/ and util/
  check_native_struct_io.py FILE...      # scan specific files
  check_native_struct_io.py --self-test  # exercise the scanner on fixtures
"""

import os
import re
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SCAN_ROOTS = ("src", "util")

CALL_PATTERN = re.compile(r"\b(fread|fwrite|read|write|pread|pwrite)\s*\(")
BYTE_SIZES = {
    "1",
    "1u",
    "1U",
    "sizeof(char)",
    "sizeof(unsignedchar)",
    "sizeof(signedchar)",
    "sizeof(uint8_t)",
    "sizeof(int8_t)",
}
NOT_DECLARATION_KEYWORDS = {"return", "else", "do", "case"}


def blank_comments_and_literals(text):
    """Blank comments and the contents of string and character literals.

    Newlines survive, so offsets still map to the original line numbers.
    """
    out = []
    index = 0
    length = len(text)
    while index < length:
        char = text[index]
        if char in "\"'":
            end = index + 1
            while end < length and text[end] != char and text[end] != "\n":
                end += 2 if text[end] == "\\" else 1
            end = min(end + 1, length)
            out.append(char + " " * max(end - index - 2, 0) + (char if end - index > 1 else ""))
            index = end
        elif text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            out.append(re.sub(r"[^\n]", " ", text[index:end]))
            index = end
        elif text.startswith("//", index):
            end = text.find("\n", index)
            end = length if end < 0 else end
            out.append(" " * (end - index))
            index = end
        else:
            out.append(char)
            index += 1
    return "".join(out)


def call_arguments(text, open_paren):
    """Return the top-level arguments of the call whose '(' is at open_paren."""
    arguments = []
    depth = 0
    start = open_paren + 1
    for index in range(open_paren, len(text)):
        char = text[index]
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
            if depth == 0:
                arguments.append(text[start:index])
                return [argument.strip() for argument in arguments]
        elif char == "," and depth == 1:
            arguments.append(text[start:index])
            start = index + 1
    return None


def is_call(text, name_start):
    """False for a declaration (type before the name) or a member call."""
    before = text[:name_start].rstrip()
    if before.endswith(".") or before.endswith("->"):
        return False
    previous = re.search(r"(\w+)$", before)
    return previous is None or previous.group(1) in NOT_DECLARATION_KEYWORDS


def scan_text(text):
    """Yield (line, message) for each native-structure file call."""
    clean = blank_comments_and_literals(text)
    for match in CALL_PATTERN.finditer(clean):
        if not is_call(clean, match.start()):
            continue
        arguments = call_arguments(clean, match.end() - 1)
        if arguments is None:
            continue
        name = match.group(1)
        line = clean.count("\n", 0, match.start()) + 1
        if name in ("fread", "fwrite"):
            if len(arguments) != 4:
                continue
            size = re.sub(r"\s+", "", arguments[1])
            if size not in BYTE_SIZES:
                yield (
                    line,
                    (
                        f"{name}() copies {arguments[1]}-sized elements; read and write bytes "
                        "and encode fields with core/binary_formats"
                    ),
                )
        elif len(arguments) >= 3 and re.search(r"sizeof\s*\(\s*struct\b", arguments[2]):
            yield (
                line,
                (
                    f"{name}() transfers a structure ({arguments[2]}); encode fields with "
                    "core/binary_formats"
                ),
            )


def default_files():
    for root in SCAN_ROOTS:
        for directory, _, names in os.walk(os.path.join(REPO_ROOT, root)):
            for name in sorted(names):
                if name.endswith((".c", ".h")):
                    yield os.path.join(directory, name)


def scan_files(paths):
    findings = []
    for path in paths:
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        relative = os.path.relpath(path, REPO_ROOT)
        findings.extend(f"{relative}:{line}: {message}" for line, message in scan_text(text))
    return findings


def self_test():
    flagged = {
        "struct record": "fwrite(&rec, sizeof(struct house_control_rec), 1, fp);",
        "int count": "if (fread(&count, sizeof(int), 1, fl) != 1)",
        "pointer element": "fwrite(\n  houses,\n  sizeof(*houses), count, fp);",
        "syscall structure": "write(fd, &entry, sizeof(struct last_entry));",
        "returned call": "return fread(&entry, sizeof entry, 1, fp);",
    }
    clean = {
        "byte buffer": "fread(buffer, 1, sizeof(buffer), input);",
        "char elements": "fwrite(text, sizeof(char), length, out);",
        "declaration": "size_t fread(void *ptr, size_t size, size_t nitems, FILE *stream);",
        "block comment": "/* fwrite(&rec, sizeof(struct x), 1, fp); */",
        "line comment": "// fread(&x, sizeof(int), 1, f);",
        "string literal": 'log("fwrite(&x, sizeof(int), 1, f)");',
        "signal byte": "write(fd, &signal_byte, sizeof(signal_byte));",
        "member call": "ops->write(fd, &entry, sizeof(struct last_entry));",
        "comparison": "written = size == 0 || fwrite(data, 1, size, stream) == size;",
    }
    failures = []
    for label, source in flagged.items():
        if not list(scan_text(source)):
            failures.append(f"not flagged: {label}")
    for label, source in clean.items():
        found = list(scan_text(source))
        if found:
            failures.append(f"falsely flagged: {label}: {found}")
    lines = [
        line for line, _ in scan_text("int a;\n/* two\nlines */\nfread(&a, sizeof a, 1, f);\n")
    ]
    if lines != [4]:
        failures.append(f"wrong line numbers: {lines}")
    for failure in failures:
        print(f"self-test: {failure}")
    print("self-test " + ("failed" if failures else "passed"))
    return 1 if failures else 0


def main(argv):
    if argv == ["--self-test"]:
        return self_test()
    if any(argument.startswith("-") for argument in argv):
        print(__doc__)
        return 2
    paths = [os.path.abspath(path) for path in argv] or list(default_files())
    findings = scan_files(paths)
    for finding in findings:
        print(finding)
    if findings:
        print(
            f"{len(findings)} native-structure file call(s); "
            "see docs/systems/BINARY_FILE_FORMATS.md"
        )
        return 1
    print(f"No native-structure file I/O in {len(paths)} files")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
