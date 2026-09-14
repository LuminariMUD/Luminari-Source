#!/usr/bin/env bash
#
# Feature detection must not depend on the warning policy.  A probe that emits
# a warning under -Werror would flip its result and activate a fallback
# definition (the struct in_addr probe once did exactly that with Clang).  This
# script configures both build systems twice, once with strict flags supplied
# the way a caller would supply them and once plainly, and fails when the
# generated src/conf.h differs.  It leaves the Autotools tree plainly
# configured.
#
# usage: check_configure_probes.sh [--cc COMPILER]

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cc=${CC:-gcc}
strict='-Wall -Wextra -Werror'

while [[ $# -gt 0 ]]; do
  case $1 in
    --cc) [[ $# -ge 2 ]] || { echo 'usage: check_configure_probes.sh [--cc COMPILER]' >&2; exit 2; }
          cc=$2; shift 2 ;;
    *) echo 'usage: check_configure_probes.sh [--cc COMPILER]' >&2; exit 2 ;;
  esac
done

work=$(mktemp -d "${TMPDIR:-/tmp}/luminari-probe-check.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
cd "$project_root"

# Only the feature results matter; the header also records the flags used.
defines()
{
  grep -E '^#(define|undef) (HAVE_|socklen_t|CIRCLE_)' src/conf.h | sort
}

autoreconf -fvi >"$work/autoreconf.log" 2>&1 || { cat "$work/autoreconf.log"; exit 1; }

./configure CC="$cc" CFLAGS="$strict" >"$work/configure-strict.log" 2>&1 ||
  { cat "$work/configure-strict.log"; echo 'configure failed with strict CFLAGS' >&2; exit 1; }
defines >"$work/autotools-strict.txt"
./configure CC="$cc" >"$work/configure-plain.log" 2>&1 ||
  { cat "$work/configure-plain.log"; echo 'plain configure failed' >&2; exit 1; }
defines >"$work/autotools-plain.txt"
if ! diff -u "$work/autotools-plain.txt" "$work/autotools-strict.txt"; then
  echo 'Autotools feature probes changed under strict CFLAGS' >&2
  exit 1
fi
grep -q '^#define HAVE_STRUCT_IN_ADDR 1' src/conf.h ||
  { echo 'Autotools did not detect struct in_addr' >&2; exit 1; }

cmake -S . -B "$work/cmake-strict" -DCMAKE_C_COMPILER="$cc" -DCMAKE_C_FLAGS="$strict" \
  >"$work/cmake-strict.log" 2>&1 ||
  { cat "$work/cmake-strict.log"; echo 'cmake failed with strict C flags' >&2; exit 1; }
defines >"$work/cmake-strict.txt"
cmake -S . -B "$work/cmake-plain" -DCMAKE_C_COMPILER="$cc" >"$work/cmake-plain.log" 2>&1 ||
  { cat "$work/cmake-plain.log"; echo 'plain cmake failed' >&2; exit 1; }
defines >"$work/cmake-plain.txt"
if ! diff -u "$work/cmake-plain.txt" "$work/cmake-strict.txt"; then
  echo 'CMake feature probes changed under strict C flags' >&2
  exit 1
fi
grep -q '^#define HAVE_STRUCT_IN_ADDR 1' src/conf.h ||
  { echo 'CMake did not detect struct in_addr' >&2; exit 1; }

# Leave the tree the way the plain Autotools configure left it.
./configure CC="$cc" >"$work/configure-final.log" 2>&1
echo "Feature probes are independent of strict flags for $cc"
