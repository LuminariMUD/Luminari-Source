#!/usr/bin/env bash

set -euo pipefail

output_file=${1:?usage: generate_build_identity.sh OUTPUT_FILE [REPOSITORY_ROOT]}
repository_root=${2:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}
output_dir=$(dirname "$output_file")
output_tmp=

cleanup()
{
  if [[ -n "$output_tmp" ]] && [[ -f "$output_tmp" ]]; then
    rm -f -- "$output_tmp"
  fi
}
trap cleanup EXIT

git_commit=$(git -C "$repository_root" rev-parse --verify HEAD 2>/dev/null || true)
if [[ ! "$git_commit" =~ ^[0-9a-f]{40}$ ]]; then
  git_commit=unknown
fi

git_dirty=0
if [[ "$git_commit" != unknown ]] &&
   [[ -n $(git -C "$repository_root" status --porcelain --untracked-files=normal 2>/dev/null) ]]; then
  git_dirty=1
fi

mkdir -p "$output_dir"
output_tmp=$(mktemp "${output_file}.tmp.XXXXXX")
{
  printf '%s\n' '#ifndef LUMINARI_BUILD_IDENTITY_H'
  printf '%s\n' '#define LUMINARI_BUILD_IDENTITY_H'
  printf '\n'
  printf '#define LUMINARI_BUILD_GIT_COMMIT "%s"\n' "$git_commit"
  printf '#define LUMINARI_BUILD_GIT_DIRTY %s\n' "$git_dirty"
  printf '\n'
  printf '%s\n' '#endif /* LUMINARI_BUILD_IDENTITY_H */'
} > "$output_tmp"

if [[ -f "$output_file" ]] && cmp -s "$output_tmp" "$output_file"; then
  rm -f -- "$output_tmp"
  output_tmp=
  exit 0
fi

mv -f -- "$output_tmp" "$output_file"
output_tmp=
