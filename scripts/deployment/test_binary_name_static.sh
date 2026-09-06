#!/usr/bin/env bash

# Static regression: no tracked file may refer to the pre-rename executable.
#
# This checks executable, build-target, release-artifact, and process-probe
# forms only. CircleMUD attribution, gameplay spell circles, internal circle_*
# C identifiers, and CIRCLE_* platform macros are deliberately out of scope:
# they are unrelated to the executable name.
#
# There is no compatibility allowlist. The rename plan and this detector must
# name rejected forms to do their jobs. Repository policy separately freezes
# paths in the current and archived changelogs as historical records.

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
  'MUD_BINARY *= *circle'
  'pgrep[^[:cntrl:]]*circle'
  '(killall|pkill|pidof|taskkill)[^[:cntrl:]]*circle'
  '(Get-Process|get-process)[^[:cntrl:]]*circle'
  'grep +circle'
  '--target +circle'
  'circle\.pid'
  'root-level +circle +(artifact|binary|executable)'
)

# Exempt only necessary detector/spec text and repository-mandated frozen
# history. These are not runtime compatibility paths.
is_allowed_path()
{
  local candidate=$1

  case "$candidate" in
    docs/ongoing-projects/BINARY_RENAME_CIRCLE_TO_LUMINARI.md) return 0 ;;
    docs/ongoing-projects/CHANGELOG.md) return 0 ;;
    docs/previous_changelogs/*) return 0 ;;
    scripts/deployment/test_binary_name_static.sh) return 0 ;;
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
done < <(
  git grep -n -I -E "$pattern_expression" -- . ':!EXAMPLE' || true

  # A bare root artifact name is otherwise indistinguishable from gameplay
  # prose. In maintained automation and tool templates, however, a line that
  # consists only of the old name is necessarily an executable artifact.
  git grep -n -I -E '^[[:space:]]*circle[[:space:]]*$' -- \
    .github scripts util || true
)

if ((${#violations[@]} > 0)); then
  printf 'binary name static test: obsolete executable references found:\n' >&2
  printf '  %s\n' "${violations[@]}" >&2
  printf '\nRename these to luminari. The rename is a clean cutover, so there\n' >&2
  printf 'is no compatibility allowlist to add them to.\n' >&2
  exit 1
fi

printf 'binary name static test: PASS\n'
