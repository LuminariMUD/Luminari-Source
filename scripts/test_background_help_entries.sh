#!/usr/bin/env bash

set -euo pipefail

repo_root=${LUMINARI_TEST_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}
sql_file="${repo_root}/sql/components/help_character_backgrounds.sql"

if [[ ! -f "${sql_file}" ]]; then
  echo "Missing Background help migration: ${sql_file}" >&2
  exit 1
fi

background_tags=(
  ACOLYTE
  CHARLATAN
  CRIMINAL-SPY
  ENTERTAINER
  FOLK-HERO
  GLADIATOR
  TRADER
  HERMIT
  SQUIRE
  NOBLE
  OUTLANDER
  PIRATE
  SAGE
  SAILOR
  SOLDIER
  URCHIN
)

command_tags=(
  SWINDLE
  ENTERTAIN
  TRIBUTE
  EXTORT
  RELAY
  FORGEAS
  FORAGE
  RETAINER
  SHORTCUT
)

for tag in "${background_tags[@]}" "${command_tags[@]}"; do
  if [[ $(grep -c "^VALUES ('${tag}'," "${sql_file}") -ne 1 ]]; then
    echo "Expected one help entry for ${tag}" >&2
    exit 1
  fi
  if ! grep -F -q "VALUES ('${tag}', '${tag}')" "${sql_file}"; then
    echo "Expected a direct help keyword for ${tag}" >&2
    exit 1
  fi
done

for placeholder in "Unused Feat" "ask staff" "TODO" "TBD"; do
  if grep -F -q "${placeholder}" "${sql_file}"; then
    echo "Placeholder text found in Background help: ${placeholder}" >&2
    exit 1
  fi
done

if ! grep -F -q "VALUES ('FOLK-HERO', 'FOLK HERO')" "${sql_file}"; then
  echo "Missing direct HELP FOLK HERO keyword" >&2
  exit 1
fi

if ! grep -F -q "VALUES ('CRIMINAL-SPY', 'CRIMINAL/SPY')" "${sql_file}"; then
  echo "Missing direct HELP CRIMINAL/SPY keyword" >&2
  exit 1
fi

echo "Background help entry checks passed."
