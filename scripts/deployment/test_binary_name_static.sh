#!/usr/bin/env bash

# Static regression: no active file may refer to the pre-rename executable.
#
# This checks high-signal executable forms only. CircleMUD attribution,
# gameplay spell circles, and internal circle_* C identifiers are deliberately
# out of scope: the goal is a reviewed allowlist, not zero uses of the word.

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
cd "$project_root"

fail()
{
  printf 'binary name static test: %s\n' "$*" >&2
  exit 1
}

command -v git >/dev/null 2>&1 || fail "git is required"

# Obsolete executable, target, and release-artifact forms.
patterns=(
  'bin/circle'
  '\./circle'
  'circle\.debug'
  'circle\.exe'
  'bin_PROGRAMS *= *circle'
  'add_executable\(circle'
  'TARGET_FILE:circle'
  'circle_(SOURCES|LDADD)'
  'CIRCLE_(COMPILE_DEFINITIONS|COMPILE_OPTIONS|LINK_LIBRARIES)'
  'pgrep[^[:cntrl:]]*circle'
  'circle\.pid'
)

# Path-specific allowlist. Every entry is either the Phase A compatibility
# implementation, a test that exercises it, the rename plan itself, or dated
# historical evidence that must not be rewritten.
allowed_paths=(
  '.github/workflows/test.yml'
  'docs/CHANGELOG.md'
  'docs/deployment/DEPLOYMENT_FIX.md'
  'docs/deployment/DEPLOYMENT_STATUS.md'
  'docs/ongoing-projects/BINARY_RENAME_CIRCLE_TO_LUMINARI.md'
  'docs/ongoing-projects/CAMPAIGN_VARIANT_RETIREMENT_LIVE_TEST_REPORT.md'
  'docs/ongoing-projects/todo.md'
  'docs/testing/LOCAL_DEV_LOGIN_QUICK_GUIDE.md'
  'docs/testing/VESSEL_BENCHMARKS.md'
  'docs/testing/VESSEL_SYSTEM_TESTING.md'
  'docs/utilities/WORLD_VALIDATOR_CLI.md'
  'scripts/autorun/test_autorun_supervision.sh'
  'scripts/deployment/install_versioned_binary.sh'
  'scripts/deployment/test_binary_name_static.sh'
  'scripts/deployment/test_versioned_binary_install.sh'
)

is_allowed_path()
{
  local candidate=$1
  local allowed

  for allowed in "${allowed_paths[@]}"; do
    [[ "$candidate" == "$allowed" ]] && return 0
  done
  case "$candidate" in
    docs/previous_changelogs/*) return 0 ;;
    docs/testing/SPECIAL_PROCEDURE_PHASE_*) return 0 ;;
  esac
  return 1
}

pattern_expression=$(IFS='|'; printf '%s' "${patterns[*]}")
violations=()

while IFS= read -r line; do
  [[ -n "$line" ]] || continue
  path=${line%%:*}
  is_allowed_path "$path" && continue
  violations+=("$line")
done < <(git grep -n -I -E "$pattern_expression" -- . ':!EXAMPLE' || true)

if ((${#violations[@]} > 0)); then
  printf 'binary name static test: obsolete executable references found:\n' >&2
  printf '  %s\n' "${violations[@]}" >&2
  printf '\nRename these to luminari, or add a reviewed path to the allowlist\n' >&2
  printf 'in scripts/deployment/test_binary_name_static.sh if the reference is\n' >&2
  printf 'Phase A compatibility code or dated historical evidence.\n' >&2
  exit 1
fi

printf 'binary name static test: PASS\n'
