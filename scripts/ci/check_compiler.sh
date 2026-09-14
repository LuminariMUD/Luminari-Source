#!/usr/bin/env bash
#
# Prove which compiler a CI job is really using.  Installing Clang on a runner
# can silently leave the build on GCC, so every job that compiles records the
# compiler family and version here and fails when they are not the expected
# ones.  The family and version come from the preprocessor's predefined macros
# rather than from the --version banner, which distributions reword freely.
#
# usage: check_compiler.sh --cc COMPILER [--family gcc|clang] [--min-version X[.Y[.Z]]]
#
# Without --family the expected family follows from the compiler's name: a
# name starting with "clang" must be Clang and anything else must be GCC.

set -euo pipefail

cc=
family=
min_version=

usage()
{
  echo 'usage: check_compiler.sh --cc COMPILER [--family gcc|clang] [--min-version X[.Y[.Z]]]' >&2
  exit 2
}

fail()
{
  printf 'compiler check: %s\n' "$*" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case $1 in
    --cc) [[ $# -ge 2 ]] || usage; cc=$2; shift 2 ;;
    --family) [[ $# -ge 2 ]] || usage; family=$2; shift 2 ;;
    --min-version) [[ $# -ge 2 ]] || usage; min_version=$2; shift 2 ;;
    *) usage ;;
  esac
done
[[ -n "$cc" ]] || usage
if [[ -z "$family" ]]; then
  case $(basename "$cc") in clang*) family=clang ;; *) family=gcc ;; esac
fi
case $family in gcc | clang) ;; *) usage ;; esac

macros=$("$cc" -dM -E - </dev/null 2>/dev/null) || fail "$cc cannot run the preprocessor"

macro_value()
{
  sed -n "s/^#define $1 //p" <<< "$macros" | head -n 1
}

if grep -q '^#define __clang__ ' <<< "$macros"; then
  actual_family=clang
  version="$(macro_value __clang_major__).$(macro_value __clang_minor__).$(macro_value __clang_patchlevel__)"
elif grep -q '^#define __GNUC__ ' <<< "$macros"; then
  actual_family=gcc
  version="$(macro_value __GNUC__).$(macro_value __GNUC_MINOR__).$(macro_value __GNUC_PATCHLEVEL__)"
else
  fail "$cc is neither GCC nor Clang"
fi

printf 'compiler: %s\n' "$cc"
printf 'family: %s\n' "$actual_family"
printf 'version: %s\n' "$version"
printf 'banner: %s\n' "$("$cc" --version | head -n 1)"
printf 'path: %s\n' "$(command -v "$cc")"

[[ "$actual_family" == "$family" ]] ||
  fail "expected $family but $cc is $actual_family $version"

if [[ -n "$min_version" ]]; then
  lowest=$(printf '%s\n%s\n' "$min_version" "$version" | sort -V | head -n 1)
  [[ "$lowest" == "$min_version" ]] ||
    fail "expected $family $min_version or newer but $cc is $version"
fi
printf 'result: PASS\n'
