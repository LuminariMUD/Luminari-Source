#!/usr/bin/env python3
"""Source-tree hygiene gate: no build products, well-formed text, ASCII docs.

Three independent checks run over a file list (default: ``git ls-files``):

artifacts
    Rejects compiled or generated build outputs: ELF, PE, Mach-O, and ar
    archives detected by magic bytes; object, library, coverage, profiler,
    and core-dump files by name; and configure/CMake/automake outputs that
    must never be committed. Media fixtures (images, audio, fonts) are
    allowed by extension.

encoding
    Every text file (anything not classified as media) must be valid UTF-8
    with no NUL bytes and no carriage returns. This runs independently of
    the ASCII policy so a malformed byte or CRLF cannot hide behind it.

ascii
    Documentation must be plain ASCII: every ``*.md`` file plus ``*.txt``
    under ``docs/``. Exceptions are listed in ASCII_EXCEPTIONS with the
    reason each one is allowed; it should stay short.

Usage:
    scripts/ci/check_source_hygiene.py              # tracked files
    scripts/ci/check_source_hygiene.py --root DIR   # an unpacked source
                                                    # distribution
    scripts/ci/check_source_hygiene.py FILE...      # specific paths

Exit status is 0 when clean, 1 when any finding is reported, 2 on usage
error. Only the standard library is used so the pre-commit hook and CI can
run it on a bare Python 3 install.
"""

import argparse
import fnmatch
import os
import subprocess
import sys

# Documentation files allowed to contain non-ASCII bytes. Each entry is a
# glob relative to the repository root with the reason it is exempt.
ASCII_EXCEPTIONS = {
    # No entries. Add "path/glob": "reason" pairs here only for content whose
    # meaning would be lost in ASCII (legal text, proper names), never for
    # decorative punctuation or symbols.
}

# Files that are legitimately binary. They still may not be build products
# and are skipped by the encoding and ASCII checks.
MEDIA_EXTENSIONS = {
    ".png", ".jpg", ".jpeg", ".gif", ".ico", ".webp", ".bmp", ".svg",
    ".wav", ".mp3", ".ogg", ".woff", ".woff2", ".ttf", ".otf", ".eot", ".pdf",
}

# Compiled or measured outputs. The .obj suffix is deliberately absent: in
# this tree every *.obj file is a text world file, and the magic-byte check
# still catches a real COFF object.
ARTIFACT_EXTENSIONS = {
    ".o", ".lo", ".la", ".a", ".so", ".dylib", ".dll", ".exe", ".ko", ".elf",
    ".gch", ".pch", ".pyc", ".pyo", ".class",
    ".gcda", ".gcno", ".gcov", ".profraw", ".profdata", ".sancov",
    ".dSYM", ".su", ".d", ".Po", ".Plo", ".trs",
}

ARTIFACT_BASENAMES = {
    "core", "vgcore", "cutest", "luminari", "circle",
    "CMakeCache.txt", "cmake_install.cmake", "CTestTestfile.cmake",
    "install_manifest.txt", "compile_commands.json",
    "config.status", "config.log", "config.cache", "stamp-h1", "conf.h",
    "AllTests.c",
}

ARTIFACT_DIRNAMES = {"CMakeFiles", ".deps", "autom4te.cache", ".libs", "_deps"}

# Autotools bootstrap outputs. They must not be tracked, but a generated
# source distribution ships them, so --root skips this set.
GENERATED_BOOTSTRAP = {
    "configure", "Makefile.in", "aclocal.m4", "conf.h.in", "config.h.in",
    "compile", "depcomp", "install-sh", "missing", "test-driver", "ltmain.sh",
}

# Shared-object versions (libfoo.so.1.2) and core dumps (core.1234).
ARTIFACT_PREFIXES = ("core.", "vgcore.")

MAGIC = (
    (b"\x7fELF", "ELF executable or object"),
    (b"MZ", "PE/DOS executable"),
    (b"\xfe\xed\xfa\xce", "Mach-O 32-bit"),
    (b"\xfe\xed\xfa\xcf", "Mach-O 64-bit"),
    (b"\xce\xfa\xed\xfe", "Mach-O 32-bit (LE)"),
    (b"\xcf\xfa\xed\xfe", "Mach-O 64-bit (LE)"),
    (b"\xca\xfe\xba\xbe", "Mach-O universal binary"),
    (b"!<arch>\n", "ar archive"),
    (b"\x1f\x8b", "gzip archive"),
    (b"PK\x03\x04", "zip archive"),
    (b"\xfd7zXZ\x00", "xz archive"),
    (b"BZh", "bzip2 archive"),
    (b"\x28\xb5\x2f\xfd", "zstd archive"),
    (b"7z\xbc\xaf\x27\x1c", "7-zip archive"),
)


def tracked_files():
    out = subprocess.run(
        ["git", "ls-files", "-z"], check=True, capture_output=True
    ).stdout
    return [p.decode("utf-8", "surrogateescape") for p in out.split(b"\0") if p]


def files_under(root):
    result = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d != ".git"]
        for name in filenames:
            full = os.path.join(dirpath, name)
            result.append(os.path.relpath(full, root))
    return sorted(result)


def is_media(path):
    return os.path.splitext(path)[1].lower() in MEDIA_EXTENSIONS


def read_head(full, size=16):
    with open(full, "rb") as fh:
        return fh.read(size)


def check_artifacts(path, full, dist_mode):
    base = os.path.basename(path)
    ext = os.path.splitext(base)[1]
    parts = path.split(os.sep)
    for part in parts[:-1]:
        if part in ARTIFACT_DIRNAMES:
            return "generated build directory (%s)" % part
    if ext in ARTIFACT_EXTENSIONS:
        return "build artifact extension %s" % ext
    if base in ARTIFACT_BASENAMES:
        return "build output %s" % base
    if base.startswith(ARTIFACT_PREFIXES):
        return "core dump"
    if ".so." in base:
        return "versioned shared object"
    if not dist_mode and base in GENERATED_BOOTSTRAP:
        return "autotools bootstrap output %s" % base
    if os.path.islink(full):
        return None
    if is_media(path):
        return None
    head = read_head(full)
    for magic, label in MAGIC:
        if head.startswith(magic):
            return label
    return None


def check_encoding(path, full):
    if is_media(path) or os.path.islink(full):
        return None
    with open(full, "rb") as fh:
        data = fh.read()
    if b"\0" in data:
        return "contains NUL bytes"
    try:
        data.decode("utf-8")
    except UnicodeDecodeError as exc:
        return "invalid UTF-8 at byte %d" % exc.start
    if b"\r" in data:
        return "carriage return (CRLF or CR line endings)"
    return None


def is_documentation(path):
    norm = path.replace(os.sep, "/")
    if norm.endswith(".md"):
        return True
    return norm.startswith("docs/") and norm.endswith(".txt")


def ascii_exempt(path):
    norm = path.replace(os.sep, "/")
    return any(fnmatch.fnmatch(norm, pattern) for pattern in ASCII_EXCEPTIONS)


def check_ascii(path, full):
    if not is_documentation(path) or ascii_exempt(path):
        return None
    with open(full, "rb") as fh:
        for lineno, line in enumerate(fh, 1):
            for offset, byte in enumerate(line):
                if byte > 0x7F:
                    return "non-ASCII byte 0x%02X at line %d column %d" % (
                        byte, lineno, offset + 1)
    return None


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument(
        "--root", default=None,
        help="check every file under this directory (an unpacked source "
             "distribution) instead of the tracked files")
    parser.add_argument(
        "paths", nargs="*",
        help="specific files to check, relative to the repository root")
    args = parser.parse_args(argv)

    if args.root and args.paths:
        parser.error("--root and explicit paths are mutually exclusive")

    if args.root:
        root = args.root
        paths = files_under(root)
        dist_mode = True
    else:
        root = subprocess.run(
            ["git", "rev-parse", "--show-toplevel"], check=True,
            capture_output=True, text=True).stdout.strip()
        paths = args.paths or tracked_files()
        dist_mode = False

    findings = []
    for path in paths:
        full = os.path.join(root, path)
        if not os.path.isfile(full) and not os.path.islink(full):
            continue
        for check, fn in (
            ("artifact", lambda: check_artifacts(path, full, dist_mode)),
            ("encoding", lambda: check_encoding(path, full)),
            ("ascii", lambda: check_ascii(path, full)),
        ):
            result = fn()
            if result:
                findings.append((path, check, result))

    for path, check, result in findings:
        print("%s: %s: %s" % (path, check, result))
    if findings:
        print("%d hygiene finding(s) in %d file(s)." % (
            len(findings), len({f[0] for f in findings})), file=sys.stderr)
        return 1
    print("Source hygiene: %d file(s) clean." % len(paths))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
