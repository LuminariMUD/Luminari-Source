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

# Assert that both aliases form the Phase A chain circle -> luminari -> release.
assert_alias_chain()
{
  local bin="$1"
  local build_id="$2"
  local context="$3"

  [[ -L "$bin/luminari" ]] || fail "$context: bin/luminari is not a symlink"
  [[ $(readlink "$bin/luminari") == "releases/$build_id/luminari" ]] ||
    fail "$context: bin/luminari has the wrong target"
  [[ -L "$bin/circle" ]] || fail "$context: bin/circle is not a symlink"
  [[ $(readlink "$bin/circle") == luminari ]] ||
    fail "$context: bin/circle is not the luminari compatibility link"
  [[ $(readlink -f "$bin/luminari") == $(readlink -f "$bin/circle") ]] ||
    fail "$context: the two aliases resolve differently"
  [[ -x "$bin/circle" ]] || fail "$context: bin/circle is not executable"
}

commit_one=1111111111111111111111111111111111111111
commit_two=2222222222222222222222222222222222222222
commit_three=3333333333333333333333333333333333333333
candidate_one="$test_root/candidate-one"
candidate_two="$test_root/candidate-two"
candidate_three="$test_root/candidate-three"
install_root="$test_root/project"
legacy_root="$test_root/legacy"
mixed_root="$test_root/mixed"
broken_root="$test_root/broken"
special_root="$test_root/special"
mkdir -p "$install_root/bin" "$legacy_root/bin" "$mixed_root/bin" \
  "$broken_root/bin" "$special_root/bin"

build_helper "$commit_one" one "$candidate_one"
build_helper "$commit_two" two "$candidate_two"
build_helper "$commit_three" three "$candidate_three"
build_one=$(readelf -nW "$candidate_one" |
  awk '/Build ID:/ {print tolower($NF); exit}')
build_two=$(readelf -nW "$candidate_two" |
  awk '/Build ID:/ {print tolower($NF); exit}')
build_three=$(readelf -nW "$candidate_three" |
  awk '/Build ID:/ {print tolower($NF); exit}')
[[ -n "$build_one" && -n "$build_two" && "$build_one" != "$build_two" ]] ||
  fail "helper build IDs are missing or identical"

# --- Fresh install produces the canonical layout and the compatibility link.
"$installer" "$candidate_one" "$install_root/bin" > "$test_root/install-one.log"
assert_alias_chain "$install_root/bin" "$build_one" "first install"
[[ -f "$install_root/bin/releases/$build_one/luminari.debug" ]] ||
  fail "first release has no debug symbols"
[[ ! -e "$install_root/bin/releases/$build_one/circle" ]] ||
  fail "first release still uses the old basename"
grep -Fxq "GIT_COMMIT=$commit_one" "$install_root/bin/releases/$build_one/manifest" ||
  fail "first manifest has the wrong commit"

# --- An old process launched through bin/circle keeps its immutable release.
"$install_root/bin/circle" &
helper_pid=$!
printf '%s\n' "$helper_pid" > "$install_root/.mud.pid"
active_executable=$(readlink -f "/proc/$helper_pid/exe")
[[ "$active_executable" == "$install_root/bin/releases/$build_one/luminari" ]] ||
  fail "first process did not launch from its immutable release"

# --- A repeated install switches both names together and retains the old one.
"$installer" "$candidate_two" "$install_root/bin" > "$test_root/install-two.log"
assert_alias_chain "$install_root/bin" "$build_two" "second install"
[[ $(readlink -f "/proc/$helper_pid/exe") == "$active_executable" ]] ||
  fail "activating a release changed the live process executable"
[[ -f "$install_root/bin/releases/$build_one/luminari" ]] ||
  fail "the active release was not retained"
[[ -f "$install_root/bin/releases/$build_two/luminari.debug" ]] ||
  fail "second release has no debug symbols"

# --- Installing an already-published release is idempotent.
"$installer" "$candidate_two" "$install_root/bin" > "$test_root/install-two-again.log"
assert_alias_chain "$install_root/bin" "$build_two" "repeated install"
kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$install_root/.mud.pid"

# --- A live regular bin/circle is never replaced.
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
[[ ! -e "$legacy_root/bin/luminari" ]] ||
  fail "failed legacy migration published a canonical alias"
[[ ! -d "$legacy_root/bin/releases/$build_two" ]] ||
  fail "failed legacy migration published a release"
grep -Fq "refusing to replace a live regular bin/circle" "$test_root/legacy-live.log" ||
  fail "legacy refusal was not explained"
kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$legacy_root/.mud.pid"

# --- A stopped regular bin/circle is archived by build ID, then migrated.
"$installer" "$candidate_two" "$legacy_root/bin" > "$test_root/legacy-stopped.log"
assert_alias_chain "$legacy_root/bin" "$build_two" "legacy migration"
[[ -f "$legacy_root/bin/releases/$build_one/luminari" ]] ||
  fail "legacy binary was not retained by build ID"

# --- A live regular bin/luminari gets the same refusal.
cp "$candidate_one" "$mixed_root/bin/luminari"
chmod 0755 "$mixed_root/bin/luminari"
"$mixed_root/bin/luminari" &
helper_pid=$!
printf '%s\n' "$helper_pid" > "$mixed_root/.mud.pid"
if "$installer" "$candidate_two" "$mixed_root/bin" > "$test_root/canonical-live.log" 2>&1; then
  fail "installer replaced a live regular bin/luminari"
fi
grep -Fq "refusing to replace a live regular bin/luminari" "$test_root/canonical-live.log" ||
  fail "canonical refusal was not explained"
[[ ! -L "$mixed_root/bin/luminari" ]] ||
  fail "failed canonical migration replaced the live binary"
kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$mixed_root/.mud.pid"
"$installer" "$candidate_two" "$mixed_root/bin" > "$test_root/canonical-stopped.log"
assert_alias_chain "$mixed_root/bin" "$build_two" "canonical migration"
[[ -f "$mixed_root/bin/releases/$build_one/luminari" ]] ||
  fail "stopped bin/luminari was not archived by build ID"

# --- An old-format release directory stays valid and usable for rollback.
old_release="$mixed_root/bin/releases/$build_three"
mkdir -p "$old_release"
install -m 0755 "$candidate_three" "$old_release/circle"
objcopy --only-keep-debug "$candidate_three" "$old_release/circle.debug"
{
  printf 'FORMAT=1\n'
  printf 'VERSION=test-three\n'
  printf 'GIT_COMMIT=%s\n' "$commit_three"
  printf 'GIT_DIRTY=0\n'
  printf 'ELF_BUILD_ID=%s\n' "$build_three"
  printf 'SHA256=%s\n' "$(sha256sum "$candidate_three" | awk '{print $1}')"
} > "$old_release/manifest"
"$installer" "$candidate_three" "$mixed_root/bin" > "$test_root/mixed-old.log"
[[ -f "$old_release/circle" ]] || fail "old-format release was rewritten"
[[ ! -e "$old_release/luminari" ]] ||
  fail "old-format release gained a canonical executable"
[[ $(readlink "$mixed_root/bin/luminari") == "releases/$build_three/circle" ]] ||
  fail "old-format release was not activated through bin/luminari"
[[ $(readlink "$mixed_root/bin/circle") == luminari ]] ||
  fail "old-format activation broke the compatibility link"

# --- An incomplete release directory is a hard failure that changes nothing.
rm -f -- "$old_release/circle.debug"
before_canonical=$(readlink "$mixed_root/bin/luminari")
if "$installer" "$candidate_three" "$mixed_root/bin" > "$test_root/mixed-broken.log" 2>&1; then
  fail "installer accepted a release with missing debug symbols"
fi
[[ $(readlink "$mixed_root/bin/luminari") == "$before_canonical" ]] ||
  fail "a failed install moved the canonical alias"
[[ $(readlink "$mixed_root/bin/circle") == luminari ]] ||
  fail "a failed install moved the compatibility alias"

# --- Broken alias symlinks are replaced rather than tripping the installer.
ln -s releases/deadbeef/luminari "$broken_root/bin/luminari"
ln -s luminari "$broken_root/bin/circle"
"$installer" "$candidate_one" "$broken_root/bin" > "$test_root/broken.log"
assert_alias_chain "$broken_root/bin" "$build_one" "broken alias recovery"

# --- A directory at either alias path is rejected outright.
mkdir -p "$special_root/bin/circle"
if "$installer" "$candidate_one" "$special_root/bin" > "$test_root/special.log" 2>&1; then
  fail "installer accepted a directory at bin/circle"
fi
grep -Fq "bin/circle is neither a regular file nor a symbolic link" \
  "$test_root/special.log" || fail "directory rejection was not explained"
[[ ! -e "$special_root/bin/luminari" ]] ||
  fail "rejected install still published a canonical alias"

printf 'versioned binary install test: PASS\n'
