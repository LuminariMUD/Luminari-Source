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
    helper_pid=""
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

  cat > "$source_file" <<HELPER
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
HELPER
  cc -g -Wl,--build-id -o "$output" "$source_file"
}

# Assert the only supported layout: bin/luminari -> releases/<build-id>/luminari.
assert_canonical_alias()
{
  local bin="$1"
  local build_id="$2"
  local context="$3"

  [[ -L "$bin/luminari" ]] || fail "$context: bin/luminari is not a symlink"
  [[ $(readlink "$bin/luminari") == "releases/$build_id/luminari" ]] ||
    fail "$context: bin/luminari has the wrong target"
  [[ -x "$bin/luminari" ]] || fail "$context: bin/luminari is not executable"
  [[ $(find "$bin" -maxdepth 1 -mindepth 1 ! -name luminari ! -name releases \
     ! -name ".*" | wc -l) -eq 0 ]] ||
    fail "$context: bin holds an unexpected server alias"
}

commit_one=1111111111111111111111111111111111111111
commit_two=2222222222222222222222222222222222222222
commit_three=3333333333333333333333333333333333333333
candidate_one="$test_root/candidate-one"
candidate_two="$test_root/candidate-two"
candidate_three="$test_root/candidate-three"
install_root="$test_root/project"
broken_root="$test_root/broken"
special_root="$test_root/special"
regular_root="$test_root/regular"
fifo_root="$test_root/fifo"
mkdir -p "$install_root/bin" "$broken_root/bin" "$special_root/bin" \
  "$regular_root/bin" "$fifo_root/bin"

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

# --- A fresh install produces exactly the canonical release layout.
"$installer" "$candidate_one" "$install_root/bin" > "$test_root/install-one.log"
assert_canonical_alias "$install_root/bin" "$build_one" "first install"
[[ -f "$install_root/bin/releases/$build_one/luminari.debug" ]] ||
  fail "first release has no debug symbols"
[[ -f "$install_root/bin/releases/$build_one/manifest" ]] ||
  fail "first release has no manifest"
[[ $(find "$install_root/bin/releases/$build_one" -mindepth 1 -maxdepth 1 \
  -printf '%f\n' | sort | tr '\n' ' ') == "luminari luminari.debug manifest " ]] ||
  fail "first release does not contain exactly the canonical files"
grep -Fxq "GIT_COMMIT=$commit_one" "$install_root/bin/releases/$build_one/manifest" ||
  fail "first manifest has the wrong commit"

# --- A running server launched through the alias keeps its immutable release.
"$install_root/bin/luminari" &
helper_pid=$!
printf '%s\n' "$helper_pid" > "$install_root/.mud.pid"
# The background shell may not have exec'd the helper when $! becomes available.
# Wait for the executable identity being tested rather than racing the fork/exec.
active_executable=
for attempt in {1..200}; do
  active_executable=$(readlink -f "/proc/$helper_pid/exe" 2>/dev/null || true)
  [[ "$active_executable" == "$install_root/bin/releases/$build_one/luminari" ]] && break
  kill -0 "$helper_pid" 2>/dev/null || break
  sleep 0.01
done
[[ "$active_executable" == "$install_root/bin/releases/$build_one/luminari" ]] ||
  fail "the live process did not launch from its immutable release"

# --- A repeated install switches the alias and retains the live release.
"$installer" "$candidate_two" "$install_root/bin" > "$test_root/install-two.log"
assert_canonical_alias "$install_root/bin" "$build_two" "second install"
[[ $(readlink -f "/proc/$helper_pid/exe") == "$active_executable" ]] ||
  fail "activating a release changed the live process executable"
[[ -f "$install_root/bin/releases/$build_one/luminari" ]] ||
  fail "the live release was not retained"
[[ -f "$install_root/bin/releases/$build_two/luminari.debug" ]] ||
  fail "second release has no debug symbols"

# --- Installing an already-published release is idempotent.
"$installer" "$candidate_two" "$install_root/bin" > "$test_root/install-two-again.log"
assert_canonical_alias "$install_root/bin" "$build_two" "repeated install"

# --- Existing releases must retain the exact immutable three-file layout.
touch "$install_root/bin/releases/$build_two/unexpected"
if "$installer" "$candidate_two" "$install_root/bin" > "$test_root/unexpected.log" 2>&1; then
  fail "installer accepted unexpected release content"
fi
grep -Fq "release directory has unexpected content" "$test_root/unexpected.log" ||
  fail "the unexpected-content refusal was not explained"
rm -f -- "$install_root/bin/releases/$build_two/unexpected"

manifest="$install_root/bin/releases/$build_two/manifest"
cp "$manifest" "$test_root/manifest-backup"
printf 'EXTRA=invalid\n' >> "$manifest"
if "$installer" "$candidate_two" "$install_root/bin" > "$test_root/manifest.log" 2>&1; then
  fail "installer accepted a malformed release manifest"
fi
grep -Fq "release manifest has the wrong shape" "$test_root/manifest.log" ||
  fail "the malformed-manifest refusal was not explained"
cp "$test_root/manifest-backup" "$manifest"

cp "$manifest" "$test_root/external-manifest"
rm -f -- "$manifest"
ln -s "$test_root/external-manifest" "$manifest"
if "$installer" "$candidate_two" "$install_root/bin" > "$test_root/symlink.log" 2>&1; then
  fail "installer accepted a symlinked release manifest"
fi
grep -Fq "release manifest is not a regular file" "$test_root/symlink.log" ||
  fail "the symlinked-manifest refusal was not explained"
rm -f -- "$manifest"
cp "$test_root/manifest-backup" "$manifest"

kill -TERM "$helper_pid"
wait "$helper_pid" 2>/dev/null || true
helper_pid=
rm -f -- "$install_root/.mud.pid"

# --- An incomplete release directory is a hard failure that changes nothing.
"$installer" "$candidate_three" "$install_root/bin" > "$test_root/install-three.log"
assert_canonical_alias "$install_root/bin" "$build_three" "third install"
rm -f -- "$install_root/bin/releases/$build_three/luminari.debug"
before_canonical=$(readlink "$install_root/bin/luminari")
if "$installer" "$candidate_three" "$install_root/bin" > "$test_root/incomplete.log" 2>&1; then
  fail "installer accepted a release with missing debug symbols"
fi
[[ $(readlink "$install_root/bin/luminari") == "$before_canonical" ]] ||
  fail "a failed install moved the canonical alias"
grep -Fq "release debug symbols are not a regular file" "$test_root/incomplete.log" ||
  fail "the incomplete-release refusal was not explained"

# --- A build-ID collision with different content is refused.
objcopy --only-keep-debug "$candidate_three" \
  "$install_root/bin/releases/$build_three/luminari.debug"
install -m 0755 "$candidate_one" "$install_root/bin/releases/$build_three/luminari"
if "$installer" "$candidate_three" "$install_root/bin" > "$test_root/collision.log" 2>&1; then
  fail "installer accepted a build ID collision"
fi
grep -Fq "build ID collision" "$test_root/collision.log" ||
  fail "the collision refusal was not explained"
[[ $(readlink "$install_root/bin/luminari") == "$before_canonical" ]] ||
  fail "a refused collision moved the canonical alias"

# --- A broken canonical symlink is replaced rather than tripping the installer.
ln -s releases/deadbeef/luminari "$broken_root/bin/luminari"
"$installer" "$candidate_one" "$broken_root/bin" > "$test_root/broken.log"
assert_canonical_alias "$broken_root/bin" "$build_one" "broken alias recovery"

# --- A directory at the canonical path is rejected outright.
mkdir -p "$special_root/bin/luminari"
if "$installer" "$candidate_one" "$special_root/bin" > "$test_root/special.log" 2>&1; then
  fail "installer accepted a directory at bin/luminari"
fi
grep -Fq "bin/luminari is not a symbolic link" "$test_root/special.log" ||
  fail "the directory rejection was not explained"
[[ ! -d "$special_root/bin/releases" ]] ||
  fail "a rejected install published a release"

# --- A regular file at the canonical path is rejected, never migrated.
cp "$candidate_one" "$regular_root/bin/luminari"
chmod 0755 "$regular_root/bin/luminari"
regular_sha=$(sha256sum "$regular_root/bin/luminari" | awk '{print $1}')
if "$installer" "$candidate_two" "$regular_root/bin" > "$test_root/regular.log" 2>&1; then
  fail "installer replaced a regular bin/luminari"
fi
[[ $(sha256sum "$regular_root/bin/luminari" | awk '{print $1}') == "$regular_sha" ]] ||
  fail "a rejected install changed the regular bin/luminari"
[[ ! -d "$regular_root/bin/releases" ]] ||
  fail "a rejected install published a release"
grep -Fq "bin/luminari is not a symbolic link" "$test_root/regular.log" ||
  fail "the regular-file rejection was not explained"

# --- A special file at the canonical path is rejected.
mkfifo "$fifo_root/bin/luminari"
if "$installer" "$candidate_one" "$fifo_root/bin" > "$test_root/fifo.log" 2>&1; then
  fail "installer accepted a FIFO at bin/luminari"
fi
grep -Fq "bin/luminari is not a symbolic link" "$test_root/fifo.log" ||
  fail "the FIFO rejection was not explained"
[[ ! -d "$fifo_root/bin/releases" ]] ||
  fail "a rejected special-file install published a release"

printf 'versioned binary install test: PASS\n'
