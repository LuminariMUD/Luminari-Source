#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
installer="$project_root/scripts/deployment/install_versioned_binary.sh"
test_root=$(mktemp -d "${TMPDIR:-/tmp}/luminari-release-test.XXXXXX")
helper_pid=

fail()
{
  printf 'versioned binary install test: %s\n' "$*" >&2
  exit 1
}

cleanup()
{
  if [[ "$helper_pid" =~ ^[1-9][0-9]*$ ]]; then
    kill -TERM "$helper_pid" 2>/dev/null || true
    wait "$helper_pid" 2>/dev/null || true
  fi
  if [[ -d "$test_root" ]] && [[ $(basename "$test_root") == luminari-release-test.* ]]; then
    rm -rf -- "$test_root"
  fi
}
trap cleanup EXIT

build_helper()
{
  local commit=$1
  local marker=$2
  local output=$3
  local source_file="$test_root/helper-$marker.c"

  cat > "$source_file" <<EOF
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  if (argc == 2 && strcmp(argv[1], "--build-info") == 0)
  {
    puts("VERSION=test-$marker");
    puts("GIT_COMMIT=$commit");
    puts("GIT_DIRTY=0");
    return 0;
  }
  for (;;)
    pause();
}
EOF
  cc -g -Wl,--build-id -o "$output" "$source_file"
}

commit_one=1111111111111111111111111111111111111111
commit_two=2222222222222222222222222222222222222222
candidate_one="$test_root/candidate-one"
candidate_two="$test_root/candidate-two"
install_root="$test_root/project"
legacy_root="$test_root/legacy"
mkdir -p "$install_root/bin" "$legacy_root/bin"

build_helper "$commit_one" one "$candidate_one"
build_helper "$commit_two" two "$candidate_two"
build_one=$(readelf -nW "$candidate_one" |
  awk '/Build ID:/ {print tolower($NF); exit}')
build_two=$(readelf -nW "$candidate_two" |
  awk '/Build ID:/ {print tolower($NF); exit}')
[[ -n "$build_one" && -n "$build_two" && "$build_one" != "$build_two" ]] ||
  fail "helper build IDs are missing or identical"

"$installer" "$candidate_one" "$install_root/bin" > "$test_root/install-one.log"
[[ -L "$install_root/bin/circle" ]] || fail "first install did not create an alias"
[[ $(readlink "$install_root/bin/circle") == "releases/$build_one/circle" ]] ||
  fail "first alias has the wrong target"
[[ -f "$install_root/bin/releases/$build_one/circle.debug" ]] ||
  fail "first release has no debug symbols"
grep -Fxq "GIT_COMMIT=$commit_one" "$install_root/bin/releases/$build_one/manifest" ||
  fail "first manifest has the wrong commit"

"$install_root/bin/circle" &
helper_pid=$!
printf '%s\n' "$helper_pid" > "$install_root/.mud.pid"
active_executable=$(readlink -f "/proc/$helper_pid/exe")
[[ "$active_executable" == "$install_root/bin/releases/$build_one/circle" ]] ||
  fail "first process did not launch from its immutable release"

"$installer" "$candidate_two" "$install_root/bin" > "$test_root/install-two.log"
[[ $(readlink "$install_root/bin/circle") == "releases/$build_two/circle" ]] ||
  fail "second alias was not activated"
[[ $(readlink -f "/proc/$helper_pid/exe") == "$active_executable" ]] ||
  fail "activating a release changed the live process executable"
[[ -f "$install_root/bin/releases/$build_one/circle" ]] ||
  fail "the active release was not retained"
[[ -f "$install_root/bin/releases/$build_two/circle.debug" ]] ||
  fail "second release has no debug symbols"
kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$install_root/.mud.pid"

cp "$candidate_one" "$legacy_root/bin/circle"
chmod 0755 "$legacy_root/bin/circle"
"$legacy_root/bin/circle" &
helper_pid=$!
printf '%s\n' "$helper_pid" > "$legacy_root/.mud.pid"
legacy_sha=$(sha256sum "$legacy_root/bin/circle" | awk '{print $1}')
if "$installer" "$candidate_two" "$legacy_root/bin" > "$test_root/legacy-live.log" 2>&1; then
  fail "installer replaced a live legacy binary"
fi
[[ $(sha256sum "$legacy_root/bin/circle" | awk '{print $1}') == "$legacy_sha" ]] ||
  fail "failed legacy migration changed bin/circle"
grep -Fq "refusing to replace a live legacy bin/circle" "$test_root/legacy-live.log" ||
  fail "legacy refusal was not explained"
kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$legacy_root/.mud.pid"

"$installer" "$candidate_two" "$legacy_root/bin" > "$test_root/legacy-stopped.log"
[[ -L "$legacy_root/bin/circle" ]] || fail "stopped legacy binary was not migrated"
[[ -f "$legacy_root/bin/releases/$build_one/circle" ]] ||
  fail "legacy binary was not retained by build ID"

printf 'versioned binary install test: PASS\n'
