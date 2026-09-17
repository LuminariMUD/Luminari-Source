#!/usr/bin/env bash
#
# Run the production-linked libFuzzer targets with their seed corpora,
# dictionaries, and saved regression inputs, then keep every reproducer.
#
# usage: run_fuzz_targets.sh [--seconds N] [--artifacts DIR] [--binary PATH]
#                            [--max-len BYTES] [target...]
#
# Each target runs for --seconds (default 15) in its own scratch corpus so the
# repository never gains generated inputs; --seconds 0 only replays the seeds
# and regression inputs once. A finding is written under
# --artifacts/<target>/ together with the fuzzer log and a minimized copy, and
# the script exits 1 after every requested target has run. Targets that end
# the process on a rejected file (see FUZZ_TARGET_MAY_EXIT in
# unittests/CuTest/fuzz_targets.h) run without leak detection: the records a
# rejected file leaves behind are abandoned by the production exit as well.

set -euo pipefail

repo_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
seconds=15
artifacts="$repo_root/fuzz-artifacts"
binary="$repo_root/luminari_fuzz"
max_len=16384
targets=()

usage() {
  sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
  exit 2
}

while [[ $# -gt 0 ]]; do
  case $1 in
    --seconds)
      [[ $# -ge 2 ]] || usage
      seconds=$2
      shift 2
      ;;
    --artifacts)
      [[ $# -ge 2 ]] || usage
      artifacts=$2
      shift 2
      ;;
    --binary)
      [[ $# -ge 2 ]] || usage
      binary=$2
      shift 2
      ;;
    --max-len)
      [[ $# -ge 2 ]] || usage
      max_len=$2
      shift 2
      ;;
    -h | --help) usage ;;
    -*) usage ;;
    *)
      targets+=("$1")
      shift
      ;;
  esac
done

[[ -x "$binary" ]] || {
  echo "fuzz: $binary is not built; see docs/guides/TESTING_GUIDE.md" >&2
  exit 2
}
binary=$(readlink -f "$binary")
artifacts=$(mkdir -p "$artifacts" && cd "$artifacts" && pwd)
corpus_root="$repo_root/unittests/CuTest/fuzz_corpus_game"
regression_root="$repo_root/unittests/CuTest/fuzz_regressions"
dictionary_root="$repo_root/unittests/CuTest/fuzz_dictionaries"

# Targets that run production code which may call exit(); keep in sync with
# the table in unittests/CuTest/test_fuzz_targets.c.
may_exit=" world config "

if [[ ${#targets[@]} -eq 0 ]]; then
  for directory in "$corpus_root"/*/; do
    targets+=("$(basename "$directory")")
  done
fi

symbolizer=$(command -v llvm-symbolizer 2>/dev/null || true)
if [[ -z "$symbolizer" ]]; then
  for candidate in /usr/lib/llvm-*/bin/llvm-symbolizer; do
    [[ -x "$candidate" ]] && symbolizer=$candidate
  done
fi
export ASAN_SYMBOLIZER_PATH="$symbolizer"
export ASAN_OPTIONS="halt_on_error=1:${ASAN_OPTIONS:-}"
export UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1:${UBSAN_OPTIONS:-}"

scratch=$(mktemp -d "${TMPDIR:-/tmp}/luminari-fuzz-run.XXXXXX")
trap 'rm -rf "$scratch"' EXIT

failed=()
for target in "${targets[@]}"; do
  seeds="$corpus_root/$target"
  [[ -d "$seeds" ]] || {
    echo "fuzz: unknown target $target (no $seeds)" >&2
    exit 2
  }
  work="$scratch/$target"
  mkdir -p "$work/corpus" "$artifacts/$target"
  cp "$seeds"/* "$work/corpus/"
  args=(-timeout=10 -max_len="$max_len" -print_final_stats=1
    -artifact_prefix="$artifacts/$target/")
  if [[ "$seconds" -gt 0 ]]; then
    args+=(-max_total_time="$seconds")
  else
    args+=(-runs=0)
  fi
  [[ -f "$dictionary_root/$target.dict" ]] && args+=(-dict="$dictionary_root/$target.dict")
  leak_options=""
  if [[ "$may_exit" == *" $target "* ]]; then
    args+=(-detect_leaks=0)
    leak_options="detect_leaks=0:"
  fi
  inputs=("$work/corpus")
  [[ -d "$regression_root/$target" ]] && inputs+=("$regression_root/$target")

  echo "::group::fuzz $target (${seconds}s)"
  if (cd "$work" && LUMINARI_FUZZ_TARGET="$target" ASAN_OPTIONS="$leak_options$ASAN_OPTIONS" \
    "$binary" "${args[@]}" "${inputs[@]}" 2>&1 | tee "$artifacts/$target/fuzzer.log"); then
    echo "::endgroup::"
    continue
  fi
  echo "::endgroup::"
  failed+=("$target")
  echo "::error::fuzz target $target found a defect; reproducers are under $artifacts/$target"
  # Minimize every reproducer the run wrote so the regression input is small.
  for reproducer in "$artifacts/$target"/crash-* "$artifacts/$target"/leak-* \
    "$artifacts/$target"/timeout-* "$artifacts/$target"/oom-*; do
    [[ -f "$reproducer" ]] || continue
    (cd "$work" && LUMINARI_FUZZ_TARGET="$target" ASAN_OPTIONS="$leak_options$ASAN_OPTIONS" \
      timeout 300 "$binary" -minimize_crash=1 \
      -max_total_time=120 -exact_artifact_path="$reproducer.min" "$reproducer" \
      >"$reproducer.minimize.log" 2>&1) || true
  done
done

# Empty artifact directories are removed so an upload step sees only findings.
find "$artifacts" -mindepth 1 -type d -empty -delete 2>/dev/null || true

if [[ ${#failed[@]} -gt 0 ]]; then
  echo "fuzz: findings in: ${failed[*]}" >&2
  echo "fuzz: reproduce with LUMINARI_FUZZ_TARGET=<target> $binary <reproducer>" >&2
  exit 1
fi
if [[ "$seconds" -gt 0 ]]; then
  echo "fuzz: ${#targets[@]} target(s) ran ${seconds}s each without findings"
else
  echo "fuzz: ${#targets[@]} target(s) replayed their seed and regression inputs without findings"
fi
