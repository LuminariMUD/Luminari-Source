#!/usr/bin/env bash

set -euo pipefail

candidate=${1:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
bin_dir=${2:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
mkdir -p "$bin_dir"
release_root="$bin_dir/releases"
project_root=$(cd "$bin_dir/.." && pwd)
release_tmp=
link_tmp=

# Canonical basename for newly created releases and for the mutable alias.
# Old releases keep the pre-rename basename; see resolve_release_basename.
exe_name=luminari
compat_name=circle

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

# Report the executable basename an existing release directory uses.  New
# releases are always canonical, but pre-rename releases must stay usable for
# rollback and core analysis, so both layouts are recognized here.
resolve_release_basename()
{
  local release_dir=$1

  if [[ -f "$release_dir/$exe_name" ]]; then
    printf '%s\n' "$exe_name"
    return 0
  fi
  if [[ -f "$release_dir/$compat_name" ]]; then
    printf '%s\n' "$compat_name"
    return 0
  fi
  return 1
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
  local release_exe
  local debug_build_id
  local installed_sha256

  if [[ -d "$release_dir" ]]; then
    release_exe=$(resolve_release_basename "$release_dir") ||
      fail "release directory is incomplete: $release_dir"
    installed_sha256=$(sha256sum "$release_dir/$release_exe" | awk '{print $1}')
    [[ "$installed_sha256" == "$source_sha256" ]] ||
      fail "build ID collision at $release_dir"
    [[ -f "$release_dir/$release_exe.debug" ]] ||
      fail "release debug symbols are missing: $release_dir/$release_exe.debug"
    [[ -f "$release_dir/manifest" ]] ||
      fail "release manifest is missing: $release_dir/manifest"
    grep -Fxq "ELF_BUILD_ID=$source_build_id" "$release_dir/manifest" ||
      fail "release manifest has the wrong build ID: $release_dir/manifest"
    grep -Fxq "SHA256=$source_sha256" "$release_dir/manifest" ||
      fail "release manifest has the wrong SHA-256: $release_dir/manifest"
    debug_build_id=$(readelf -nW "$release_dir/$release_exe.debug" 2>/dev/null |
      awk '/Build ID:/ {print tolower($NF); exit}')
    [[ "$debug_build_id" == "$source_build_id" ]] ||
      fail "release debug symbols have the wrong build ID: $release_dir/$release_exe.debug"
    created_release_exe=$release_exe
    return 0
  fi

  release_tmp=$(mktemp -d "$release_root/.${source_build_id}.tmp.XXXXXX")
  install -m 0755 "$source_binary" "$release_tmp/$exe_name"
  objcopy --only-keep-debug "$source_binary" "$release_tmp/$exe_name.debug"
  chmod 0644 "$release_tmp/$exe_name.debug"
  write_manifest "$release_tmp/manifest" "$source_build_id" "$source_commit" \
    "$source_dirty" "$source_sha256" "$source_version"
  chmod 0644 "$release_tmp/manifest"
  mv -- "$release_tmp" "$release_dir"
  release_tmp=
  created_release_exe=$exe_name
}

archive_regular_alias()
{
  local alias_path=$1
  local legacy_build_id
  local legacy_sha256

  legacy_build_id=$(readelf -nW "$alias_path" 2>/dev/null |
    awk '/Build ID:/ {print $NF; exit}')
  [[ "$legacy_build_id" =~ ^[0-9a-fA-F]{16,}$ ]] ||
    fail "existing $alias_path has no usable ELF build ID"
  legacy_build_id=${legacy_build_id,,}
  legacy_sha256=$(sha256sum "$alias_path" | awk '{print $1}')
  create_release "$alias_path" "$legacy_build_id" unknown 1 "$legacy_sha256" legacy
}

# Return 0 when alias_path is the executable image of the recorded live MUD.
alias_is_live()
{
  local alias_path=$1
  local active_pid=

  if [[ -r "$project_root/.mud.pid" ]]; then
    IFS= read -r active_pid < "$project_root/.mud.pid" || true
  fi
  [[ "$active_pid" =~ ^[1-9][0-9]*$ ]] || return 1
  [[ -e "/proc/$active_pid/exe" ]] || return 1
  [[ $(stat -Lc '%d:%i' "$alias_path" 2>/dev/null || true) == \
     $(stat -Lc '%d:%i' "/proc/$active_pid/exe" 2>/dev/null || true) ]]
}

# Reject an alias path we must never replace.  Runs for both aliases before
# anything is mutated so a refusal leaves the whole installation untouched.
validate_alias_path()
{
  local alias_path=$1
  local label=$2

  if [[ -L "$alias_path" ]]; then
    return 0
  fi
  [[ -e "$alias_path" ]] || return 0
  [[ -f "$alias_path" ]] ||
    fail "$label is neither a regular file nor a symbolic link"
  if alias_is_live "$alias_path"; then
    fail "refusing to replace a live regular $label; stop it once to migrate"
  fi
  return 0
}

# Leave alias_path either absent or a symbolic link we may replace.  A stopped
# regular executable is archived by build ID before it is removed.
prepare_alias_path()
{
  local alias_path=$1

  if [[ -L "$alias_path" ]]; then
    if [[ ! -e "$alias_path" ]]; then
      rm -f -- "$alias_path"
    fi
    return 0
  fi
  [[ -f "$alias_path" ]] || return 0
  archive_regular_alias "$alias_path"
  rm -f -- "$alias_path"
}

publish_symlink()
{
  local alias_path=$1
  local target=$2

  link_tmp="$bin_dir/.$(basename "$alias_path")-link.$$"
  rm -f -- "$link_tmp"
  ln -s "$target" "$link_tmp"
  mv -Tf -- "$link_tmp" "$alias_path"
  link_tmp=
}

canonical_alias="$bin_dir/$exe_name"
compat_alias="$bin_dir/$compat_name"

validate_alias_path "$canonical_alias" "bin/$exe_name"
validate_alias_path "$compat_alias" "bin/$compat_name"

mkdir -p "$release_root"
created_release_exe=
create_release "$candidate" "$elf_build_id" "$git_commit" \
  "$git_dirty" "$candidate_sha256" "$version"
release_exe=$created_release_exe

prepare_alias_path "$canonical_alias"
prepare_alias_path "$compat_alias"

# Publish the canonical alias first so the compatibility link never resolves
# to a target that does not exist yet.
publish_symlink "$canonical_alias" "releases/$elf_build_id/$release_exe"

# Phase A keeps bin/circle as a stable link to bin/luminari.  Both names then
# change together whenever the canonical alias is switched.
if [[ ! -L "$compat_alias" ]] ||
   [[ $(readlink "$compat_alias") != "$exe_name" ]]; then
  publish_symlink "$compat_alias" "$exe_name"
fi

printf 'Installed release: %s\n' "$release_root/$elf_build_id/$release_exe"
printf 'Activated alias: %s -> releases/%s/%s\n' "$canonical_alias" \
  "$elf_build_id" "$release_exe"
printf 'Compatibility alias: %s -> %s\n' "$compat_alias" "$exe_name"
printf 'Git commit: %s (dirty=%s)\n' "$git_commit" "$git_dirty"
printf 'ELF build ID: %s\n' "$elf_build_id"
printf 'SHA-256: %s\n' "$candidate_sha256"
