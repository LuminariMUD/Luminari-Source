#!/usr/bin/env bash
# Configure, build, test, and install an untouched `git archive HEAD` through
# both the Autotools and CMake build systems. This is the repository's
# distribution check: it proves no untracked local file or generated artifact
# is required. Each half runs that system's full test entry point (`make test`
# and `ctest`), so the archive has to satisfy the same gates as a checkout.
#
# Usage: scripts/ci/check_clean_archive.sh [--keep] [--skip-autotools] [--skip-cmake]
#
# Environment:
#   CMAKE_PRESET   configure/build/test preset (default: dev)
#   JOBS           parallel build jobs (default: nproc)
#   LUMINARI_TEST_MYSQL_* / LUMINARI_TEST_DATA_DIR / LUMINARI_TEST_CONFIG_FILE
#                  pass through to the production-linked suite; prepare them
#                  with scripts/ci/prepare_test_runtime.sh (as the CI job does)
#                  or export LUMINARI_TEST_SKIP_SYNTAX_BOOT=1 without world data.

set -euo pipefail

script_path=$(readlink -f -- "$0")
repo_root=$(dirname "$(dirname "$(dirname "$script_path")")")
keep=0
run_autotools=1
run_cmake=1
preset=${CMAKE_PRESET:-dev}
jobs=${JOBS:-$(nproc)}

for argument in "$@"; do
  case "$argument" in
    --keep) keep=1 ;;
    --skip-autotools) run_autotools=0 ;;
    --skip-cmake) run_cmake=0 ;;
    *)
      printf 'Unknown argument: %s\n' "$argument" >&2
      exit 2
      ;;
  esac
done

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/luminari-archive.XXXXXX")
cleanup() {
  if [[ $keep -eq 0 ]]; then
    rm -rf "$work_dir"
  else
    printf 'Archive tree kept at %s\n' "$work_dir"
  fi
}
trap cleanup EXIT

printf '==> Exporting git archive HEAD from %s\n' "$repo_root"
git -C "$repo_root" archive --format=tar HEAD | tar -x -C "$work_dir"

cd "$work_dir"
cp src/campaign.example.h src/campaign.h
cp src/mud_options.example.h src/mud_options.h
cp src/vnums.example.h src/vnums.h
export LUMINARI_TEST_ROOT="$work_dir"
export LUMINARI_TEST_SPEC_WORLD_ROOT="$work_dir/unittests/CuTest/fixtures/spec_world_inventory"

if [[ $run_autotools -eq 1 ]]; then
  printf '==> Autotools: autoreconf, configure, build, make test, make install\n'
  autoreconf -fvi >/dev/null
  ./configure >/dev/null
  make -j"$jobs" >/dev/null
  make -j"$jobs" cutest bsd_snprintf_fallback_test >/dev/null
  make test
  make install >/dev/null
  test -x bin/luminari
  test ! -e luminari
fi

if [[ $run_cmake -eq 1 ]]; then
  printf '==> CMake: preset %s configure, build, ctest, install\n' "$preset"
  cmake --preset "$preset" >/dev/null
  cmake --build --preset "$preset" -j"$jobs" >/dev/null
  ctest --preset "$preset"
  cmake --install "build/$preset" >/dev/null
  test -x bin/luminari
fi

printf '==> Clean archive check passed\n'
