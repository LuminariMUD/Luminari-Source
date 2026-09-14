#!/usr/bin/env bash
#
# Probe the C compiler for the LuminariMUD production build profile and print
# the flags both build systems apply.  Autotools (configure.ac) and CMake
# (CMakeLists.txt) call this script so the two builds share one contract.
#
# Output (one KEY=VALUE per line, every key always present):
#   PRODUCTION_CFLAGS       compiler flags to append
#   PRODUCTION_LDFLAGS      linker flags to append
#   PRODUCTION_SUPPORTED    space-separated hardening features enabled
#   PRODUCTION_UNSUPPORTED  space-separated hardening features the toolchain
#                           rejected; the caller must report these
#   WARNING_TIER            the warning tier that was requested (or "none")
#   WARNING_CFLAGS          warning flags for that tier, each one probed
#   WARNING_UNSUPPORTED     tier flags this compiler rejected; the caller must
#                           report these
#
# Warning tiers (--warnings TIER), cumulative:
#   baseline   clean on every supported compiler; the strict CI builds turn
#              these into errors with -Werror
#   migration  known-noisy families tracked by scripts/ci/check_warning_budget.py;
#              the budget may only shrink, so new code cannot add to them
#   analysis   compiler-specific diagnostics for the scheduled analysis job,
#              informational only
#
# Exit status is non-zero when the compiler cannot build a trivial program, an
# option is unknown, or an explicitly requested LTO/PGO profile is unsupported.
# Missing hardening features are reported through PRODUCTION_UNSUPPORTED, never
# silently dropped.

set -euo pipefail

cc=
base_cflags=
production=0
lto=0
pgo_generate=
pgo_use=
warning_tier=none

usage()
{
  cat >&2 <<'USAGE'
usage: production_profile.sh --cc COMPILER [--cflags FLAGS] [--production]
                             [--lto] [--pgo-generate DIR] [--pgo-use PATH]
                             [--warnings baseline|migration|analysis]
USAGE
  exit 2
}

fail()
{
  printf 'production profile: %s\n' "$*" >&2
  exit 1
}

while [[ $# -gt 0 ]]; do
  case $1 in
    --cc)
      [[ $# -ge 2 ]] || usage
      cc=$2
      shift 2
      ;;
    --cflags)
      [[ $# -ge 2 ]] || usage
      base_cflags=$2
      shift 2
      ;;
    --production)
      production=1
      shift
      ;;
    --lto)
      lto=1
      shift
      ;;
    --pgo-generate)
      [[ $# -ge 2 ]] || usage
      pgo_generate=$2
      shift 2
      ;;
    --pgo-use)
      [[ $# -ge 2 ]] || usage
      pgo_use=$2
      shift 2
      ;;
    --warnings)
      [[ $# -ge 2 ]] || usage
      warning_tier=$2
      shift 2
      ;;
    *)
      usage
      ;;
  esac
done

[[ -n "$cc" ]] || usage
if [[ -n "$pgo_generate" && -n "$pgo_use" ]]; then
  fail "--pgo-generate and --pgo-use are mutually exclusive"
fi
case $warning_tier in
  none | baseline | migration | analysis) ;;
  *) fail "unknown warning tier: $warning_tier" ;;
esac

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/luminari-production-profile.XXXXXX")
trap 'rm -rf -- "$work_dir"' EXIT

trivial_source="$work_dir/trivial.c"
cat > "$trivial_source" <<'SOURCE'
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  char buffer[32];

  snprintf(buffer, sizeof(buffer), "%d", argc);
  return argv[0] != NULL && strlen(buffer) > 0 ? 0 : 1;
}
SOURCE

fortify_source="$work_dir/fortify.c"
cat > "$fortify_source" <<'SOURCE'
#include <string.h>

#if !defined(__USE_FORTIFY_LEVEL) || __USE_FORTIFY_LEVEL < 3
#error "_FORTIFY_SOURCE=3 is not available with this compiler and C library"
#endif

int main(int argc, char **argv)
{
  char buffer[8];

  (void)argc;
  strcpy(buffer, "x");
  return argv[0] != NULL && buffer[0] == 'x' ? 0 : 1;
}
SOURCE

# try_build SOURCE STRICT FLAGS...: compile and link SOURCE with FLAGS.  With
# STRICT=1 every warning is fatal so a flag that is merely tolerated (for
# example "unused command-line argument") counts as unsupported.
try_build()
{
  local source=$1
  local strict=$2
  shift 2
  local -a strict_flags=()

  if [[ "$strict" == 1 ]]; then
    # shellcheck disable=SC2054
    strict_flags=(-Werror -Wl,--fatal-warnings)
  fi
  # shellcheck disable=SC2086
  "$cc" $base_cflags "${strict_flags[@]}" "$@" -o "$work_dir/probe" "$source" \
    >"$work_dir/probe.log" 2>&1
}

# shellcheck disable=SC2086
"$cc" $base_cflags -o "$work_dir/probe" "$trivial_source" >"$work_dir/probe.log" 2>&1 ||
  fail "$cc cannot build a trivial program: $(tr '\n' ' ' < "$work_dir/probe.log")"

is_clang=0
if "$cc" -dM -E - </dev/null 2>/dev/null | grep -q '__clang__'; then
  is_clang=1
fi

# Local suppression, recorded here with its justification rather than in the
# repository-wide profile: a Clang 22 host with several GCC installations
# emits a driver note (-Wgcc-install-dir-libstdcxx) on every invocation,
# which -Werror turns into a failure of every compile and every strict probe
# below.  It concerns the C++ standard library search path and is irrelevant
# to this C code base, so it is silenced only where it actually fires.
warning_cflags=()
if grep -q 'Wgcc-install-dir-libstdcxx' "$work_dir/probe.log"; then
  base_cflags="$base_cflags -Wno-gcc-install-dir-libstdcxx"
  if [[ "$warning_tier" != none ]]; then
    warning_cflags+=(-Wno-gcc-install-dir-libstdcxx)
  fi
fi

cflags=()
ldflags=()
supported=()
unsupported=()

# probe_feature NAME CFLAGS LDFLAGS [SOURCE]: enable one hardening feature when
# the toolchain accepts it, otherwise record it as unsupported.
probe_feature()
{
  local name=$1
  local feature_cflags=$2
  local feature_ldflags=$3
  local source=${4:-$trivial_source}

  # shellcheck disable=SC2086
  if try_build "$source" 1 -O2 "${cflags[@]}" $feature_cflags "${ldflags[@]}" \
    $feature_ldflags; then
    # shellcheck disable=SC2206
    cflags+=($feature_cflags)
    # shellcheck disable=SC2206
    ldflags+=($feature_ldflags)
    supported+=("$name")
  else
    unsupported+=("$name")
  fi
}

if [[ "$production" == 1 ]]; then
  # Optimization and symbol policy: -O2 with full DWARF.  The versioned
  # installer splits the debug information into a .debug companion keyed by
  # ELF build ID, so production keeps symbolizable crashes without shipping a
  # bloated executable.  Assertions stay enabled (no NDEBUG): a MUD that stops
  # with a core file is preferable to one that keeps running on corrupt state.
  #
  # LUMINARI_PRODUCTION_PROFILE makes src/constants.c embed a marker section
  # that verify_hardened_binary.sh requires, so a binary built from default
  # toolchain flags fails verification even on a distribution whose defaults
  # already happen to include every hardening property below.
  cflags+=(-O2 -g -DLUMINARI_PRODUCTION_PROFILE=1)

  # Every hardening item is requested explicitly and probed individually.
  # GCC 14's -fhardened bundle is deliberately not used: distributions that
  # already define _FORTIFY_SOURCE in the compiler spec (Ubuntu) make
  # -Whardened reject it, and the explicit set is identical on every GCC and
  # Clang the project supports.
  probe_feature fortify-source "-U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3" "" \
    "$fortify_source"
  probe_feature pie "-fPIE" "-pie"
  probe_feature relro "" "-Wl,-z,relro"
  probe_feature bind-now "" "-Wl,-z,now"
  probe_feature stack-protector "-fstack-protector-strong" ""
  probe_feature stack-clash-protection "-fstack-clash-protection" ""
  probe_feature cf-protection "-fcf-protection=full" ""
  # Accepting the flag is not enough: the linked image only carries the CET
  # property note when every input object has it, and a toolchain built from
  # source without --enable-cet (the official gcc container images) drops it
  # even though it compiles -fcf-protection.  Verify the note on the probe
  # binary that was just linked with the flag, and demote on failure.
  if [[ " ${supported[*]-} " == *" cf-protection "* ]] &&
    ! { readelf -nW "$work_dir/probe" 2>/dev/null | grep -q 'IBT' &&
        readelf -nW "$work_dir/probe" 2>/dev/null | grep -q 'SHSTK'; }; then
    filtered=()
    for flag in "${cflags[@]}"; do
      [[ "$flag" == -fcf-protection=full ]] || filtered+=("$flag")
    done
    cflags=("${filtered[@]}")
    filtered=()
    for name in "${supported[@]}"; do
      [[ "$name" == cf-protection ]] || filtered+=("$name")
    done
    supported=("${filtered[@]}")
    unsupported+=(cf-protection)
  fi
  probe_feature noexecstack "" "-Wl,-z,noexecstack"
  # The versioned installer keys on the ELF build ID; distribution compilers
  # add one by default, a source-built toolchain does not.
  probe_feature build-id "" "-Wl,--build-id"
fi

if [[ "$lto" == 1 ]]; then
  if [[ "$is_clang" == 1 ]]; then
    lto_flag=-flto=thin
  else
    lto_flag=-flto=auto
  fi
  try_build "$trivial_source" 0 -O2 "${cflags[@]}" "$lto_flag" "${ldflags[@]}" "$lto_flag" ||
    fail "$cc does not support link-time optimization ($lto_flag): $(tr '\n' ' ' < "$work_dir/probe.log")"
  cflags+=("$lto_flag")
  ldflags+=("$lto_flag")
fi

if [[ -n "$pgo_generate" ]]; then
  pgo_flags=("-fprofile-generate=$pgo_generate")
  try_build "$trivial_source" 0 -O2 "${cflags[@]}" "${pgo_flags[@]}" "${ldflags[@]}" \
    "${pgo_flags[@]}" ||
    fail "$cc does not support profile generation: $(tr '\n' ' ' < "$work_dir/probe.log")"
  cflags+=("${pgo_flags[@]}")
  ldflags+=("${pgo_flags[@]}")
fi

if [[ -n "$pgo_use" ]]; then
  [[ -e "$pgo_use" ]] || fail "profile data path does not exist: $pgo_use"
  if [[ "$is_clang" == 1 ]]; then
    pgo_flags=("-fprofile-use=$pgo_use")
  else
    pgo_flags=("-fprofile-use=$pgo_use" -fprofile-correction -Wno-missing-profile)
  fi
  try_build "$trivial_source" 0 -O2 "${cflags[@]}" "${pgo_flags[@]}" "${ldflags[@]}" \
    "${pgo_flags[@]}" ||
    fail "$cc rejected the profile data at $pgo_use: $(tr '\n' ' ' < "$work_dir/probe.log")"
  cflags+=("${pgo_flags[@]}")
  ldflags+=("${pgo_flags[@]}")
fi

# Warning tiers.  Every flag is probed so an older supported compiler simply
# reports the ones it lacks instead of failing; the caller prints the list.
warning_unsupported=()

# probe_warning FLAG: keep FLAG when the compiler accepts it without complaint.
probe_warning()
{
  if try_build "$trivial_source" 1 "${warning_cflags[@]}" "$1"; then
    warning_cflags+=("$1")
  else
    warning_unsupported+=("$1")
  fi
}

# Baseline: zero occurrences on GCC 13-16 and Clang 18-22 at the time the
# tiers were introduced, so -Werror is safe.  A new baseline flag must first
# be proven clean by the migration budget reaching zero for that class.
baseline_common=(-Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wpointer-arith
  -Wformat-security -Wvla)
baseline_gcc=(-Wtrampolines -Walloc-size -Wbidi-chars=any -Wcalloc-transposed-args
  -Wflex-array-member-not-at-end -Wunterminated-string-initialization)
baseline_clang=()

# Migration: the families the strict-C23 audit found in bulk (sign and value
# conversion, switch coverage, prototype hygiene, format types, shadowing,
# allocation size, duplicated logic, fallthrough).  Never combined with
# -Werror; scripts/ci/check_warning_budget.py ratchets them down.
migration_common=(-Wconversion -Wsign-conversion -Wdouble-promotion -Wshadow
  -Wmissing-prototypes -Wswitch-enum -Wswitch-default -Wcast-qual -Wundef
  -Wjump-misses-init -Wnull-dereference -Wnested-externs -Wredundant-decls -Wformat=2
  -Wfloat-equal -Walloca -Wimplicit-fallthrough -Wwrite-strings)
migration_gcc=(-Wformat-signedness -Wcast-align=strict -Walloc-zero -Wduplicated-cond
  -Wduplicated-branches -Wlogical-op)
migration_clang=(-Wcast-align)

# Analysis: expensive or opinionated, compiler-specific, scheduled only.
analysis_gcc=(-fanalyzer)
analysis_clang=(-Wextra-semi-stmt -Wcomma -Wunreachable-code-aggressive -Wbad-function-cast
  -Wconditional-uninitialized -Wcovered-switch-default -Wmissing-format-attribute
  -Wformat-pedantic -Wassign-enum -Wenum-enum-conversion)

tier_flags=("${baseline_common[@]}")
if [[ "$is_clang" == 1 ]]; then tier_flags+=("${baseline_clang[@]-}"); else tier_flags+=("${baseline_gcc[@]}"); fi
if [[ "$warning_tier" == migration || "$warning_tier" == analysis ]]; then
  tier_flags+=("${migration_common[@]}")
  if [[ "$is_clang" == 1 ]]; then tier_flags+=("${migration_clang[@]}"); else tier_flags+=("${migration_gcc[@]}"); fi
fi
if [[ "$warning_tier" == analysis ]]; then
  if [[ "$is_clang" == 1 ]]; then tier_flags+=("${analysis_clang[@]}"); else tier_flags+=("${analysis_gcc[@]}"); fi
fi
if [[ "$warning_tier" == none ]]; then
  tier_flags=()
fi
for flag in "${tier_flags[@]-}"; do
  [[ -n "$flag" ]] && probe_warning "$flag"
done

printf 'PRODUCTION_CFLAGS=%s\n' "${cflags[*]-}"
printf 'PRODUCTION_LDFLAGS=%s\n' "${ldflags[*]-}"
printf 'PRODUCTION_SUPPORTED=%s\n' "${supported[*]-}"
printf 'PRODUCTION_UNSUPPORTED=%s\n' "${unsupported[*]-}"
printf 'WARNING_TIER=%s\n' "$warning_tier"
printf 'WARNING_CFLAGS=%s\n' "${warning_cflags[*]-}"
printf 'WARNING_UNSUPPORTED=%s\n' "${warning_unsupported[*]-}"
