#!/usr/bin/env bash

set -euo pipefail

candidate=${1:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
bin_dir=${2:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
mkdir -p "$bin_dir"
release_root="$bin_dir/releases"
project_root=$(cd "$bin_dir/.." && pwd)
release_tmp=
link_tmp=

fail()
{
  printf 'versioned binary install: %s\n' "$*" >&2
  exit 1
}

cleanup()
{
  if [[ -n "$link_tmp" ]] && [[ -L "$link_tmp" ]]; then
    rm -f -- "$link_tmp"
  fi
  if [[ -n "$release_tmp" ]] && [[ -d "$release_tmp" ]]; then
    rm -rf -- "$release_tmp"
  fi
}
trap cleanup EXIT

command -v readelf >/dev/null 2>&1 || fail "readelf is required"
command -v objcopy >/dev/null 2>&1 || fail "objcopy is required"
command -v sha256sum >/dev/null 2>&1 || fail "sha256sum is required"

[[ -f "$candidate" ]] || fail "candidate is not a regular file: $candidate"
[[ -x "$candidate" ]] || fail "candidate is not executable: $candidate"

elf_build_id=$(readelf -nW "$candidate" 2>/dev/null |
  awk '/Build ID:/ {print $NF; exit}')
[[ "$elf_build_id" =~ ^[0-9a-fA-F]{16,}$ ]] ||
  fail "candidate has no usable ELF build ID"
elf_build_id=${elf_build_id,,}
candidate_sha256=$(sha256sum "$candidate" | awk '{print $1}')

if ! build_info=$("$candidate" --build-info 2>/dev/null); then
  fail "candidate does not provide --build-info"
fi
version=$(awk -F= '$1 == "VERSION" {print substr($0, index($0, "=") + 1); exit}' <<< "$build_info")
git_commit=$(awk -F= '$1 == "GIT_COMMIT" {print $2; exit}' <<< "$build_info")
git_dirty=$(awk -F= '$1 == "GIT_DIRTY" {print $2; exit}' <<< "$build_info")
[[ -n "$version" ]] || fail "candidate build information has no version"
[[ "$git_commit" == unknown || "$git_commit" =~ ^[0-9a-f]{40}$ ]] ||
  fail "candidate build information has an invalid Git commit"
[[ "$git_dirty" == 0 || "$git_dirty" == 1 ]] ||
  fail "candidate build information has an invalid dirty flag"

write_manifest()
{
  local manifest=$1
  local manifest_build_id=$2
  local manifest_commit=$3
  local manifest_dirty=$4
  local manifest_sha256=$5
  local manifest_version=$6

  {
    printf 'FORMAT=1\n'
    printf 'VERSION=%s\n' "$manifest_version"
    printf 'GIT_COMMIT=%s\n' "$manifest_commit"
    printf 'GIT_DIRTY=%s\n' "$manifest_dirty"
    printf 'ELF_BUILD_ID=%s\n' "$manifest_build_id"
    printf 'SHA256=%s\n' "$manifest_sha256"
  } > "$manifest"
}

create_release()
{
  local source_binary=$1
  local source_build_id=$2
  local source_commit=$3
  local source_dirty=$4
  local source_sha256=$5
  local source_version=$6
  local release_dir="$release_root/$source_build_id"
  local debug_build_id
  local installed_sha256

  if [[ -d "$release_dir" ]]; then
    [[ -f "$release_dir/circle" ]] ||
      fail "release directory is incomplete: $release_dir"
    installed_sha256=$(sha256sum "$release_dir/circle" | awk '{print $1}')
    [[ "$installed_sha256" == "$source_sha256" ]] ||
      fail "build ID collision at $release_dir"
    [[ -f "$release_dir/circle.debug" ]] ||
      fail "release debug symbols are missing: $release_dir/circle.debug"
    [[ -f "$release_dir/manifest" ]] ||
      fail "release manifest is missing: $release_dir/manifest"
    grep -Fxq "ELF_BUILD_ID=$source_build_id" "$release_dir/manifest" ||
      fail "release manifest has the wrong build ID: $release_dir/manifest"
    grep -Fxq "SHA256=$source_sha256" "$release_dir/manifest" ||
      fail "release manifest has the wrong SHA-256: $release_dir/manifest"
    debug_build_id=$(readelf -nW "$release_dir/circle.debug" 2>/dev/null |
      awk '/Build ID:/ {print tolower($NF); exit}')
    [[ "$debug_build_id" == "$source_build_id" ]] ||
      fail "release debug symbols have the wrong build ID: $release_dir/circle.debug"
    return 0
  fi

  release_tmp=$(mktemp -d "$release_root/.${source_build_id}.tmp.XXXXXX")
  install -m 0755 "$source_binary" "$release_tmp/circle"
  objcopy --only-keep-debug "$source_binary" "$release_tmp/circle.debug"
  chmod 0644 "$release_tmp/circle.debug"
  write_manifest "$release_tmp/manifest" "$source_build_id" "$source_commit" \
    "$source_dirty" "$source_sha256" "$source_version"
  chmod 0644 "$release_tmp/manifest"
  mv -- "$release_tmp" "$release_dir"
  release_tmp=
}

archive_legacy_alias()
{
  local alias_path=$1
  local legacy_build_id
  local legacy_sha256

  legacy_build_id=$(readelf -nW "$alias_path" 2>/dev/null |
    awk '/Build ID:/ {print $NF; exit}')
  [[ "$legacy_build_id" =~ ^[0-9a-fA-F]{16,}$ ]] ||
    fail "existing bin/circle has no usable ELF build ID"
  legacy_build_id=${legacy_build_id,,}
  legacy_sha256=$(sha256sum "$alias_path" | awk '{print $1}')
  create_release "$alias_path" "$legacy_build_id" unknown 1 "$legacy_sha256" legacy
}

mkdir -p "$release_root"
create_release "$candidate" "$elf_build_id" "$git_commit" "$git_dirty" \
  "$candidate_sha256" "$version"

circle_alias="$bin_dir/circle"
if [[ -e "$circle_alias" ]] && [[ ! -L "$circle_alias" ]]; then
  active_pid=
  if [[ -r "$project_root/.mud.pid" ]]; then
    IFS= read -r active_pid < "$project_root/.mud.pid" || true
  fi
  if [[ "$active_pid" =~ ^[1-9][0-9]*$ ]] &&
     [[ -e "/proc/$active_pid/exe" ]] &&
     [[ $(stat -Lc '%d:%i' "$circle_alias" 2>/dev/null || true) == \
        $(stat -Lc '%d:%i' "/proc/$active_pid/exe" 2>/dev/null || true) ]]; then
    fail "refusing to replace a live legacy bin/circle; stop it once to migrate"
  fi
  archive_legacy_alias "$circle_alias"
  rm -f -- "$circle_alias"
elif [[ -L "$circle_alias" ]] && [[ ! -e "$circle_alias" ]]; then
  rm -f -- "$circle_alias"
elif [[ -e "$circle_alias" ]] && [[ ! -f "$circle_alias" ]]; then
  fail "bin/circle is neither a regular file nor a symbolic link"
fi

link_tmp="$bin_dir/.circle-link.$$"
ln -s "releases/$elf_build_id/circle" "$link_tmp"
mv -Tf -- "$link_tmp" "$circle_alias"
link_tmp=

printf 'Installed release: %s\n' "$bin_dir/releases/$elf_build_id/circle"
printf 'Activated alias: %s -> releases/%s/circle\n' "$circle_alias" "$elf_build_id"
printf 'Git commit: %s (dirty=%s)\n' "$git_commit" "$git_dirty"
printf 'ELF build ID: %s\n' "$elf_build_id"
printf 'SHA-256: %s\n' "$candidate_sha256"
