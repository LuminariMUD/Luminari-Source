#!/usr/bin/env bash

set -euo pipefail

candidate=${1:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
bin_dir=${2:?usage: install_versioned_binary.sh CANDIDATE BIN_DIRECTORY}
mkdir -p "$bin_dir"
release_root="$bin_dir/releases"
release_tmp=
link_tmp=

# The only executable basename this installer creates or activates.
exe_name=luminari

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

# Create the immutable release for this build ID, or validate the one that is
# already published.  A release always holds luminari, luminari.debug, and a
# manifest whose build ID and SHA-256 match the executable.
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
  local manifest_line_count
  local unexpected_entry

  if [[ -e "$release_dir" || -L "$release_dir" ]]; then
    [[ -d "$release_dir" && ! -L "$release_dir" ]] ||
      fail "release path is not a directory: $release_dir"
    unexpected_entry=$(find "$release_dir" -mindepth 1 -maxdepth 1 \
      ! -name "$exe_name" ! -name "$exe_name.debug" ! -name manifest \
      -print -quit)
    [[ -z "$unexpected_entry" ]] ||
      fail "release directory has unexpected content: $unexpected_entry"
    [[ -f "$release_dir/$exe_name" && ! -L "$release_dir/$exe_name" ]] ||
      fail "release executable is not a regular file: $release_dir/$exe_name"
    [[ -x "$release_dir/$exe_name" ]] ||
      fail "release executable is not executable: $release_dir/$exe_name"
    installed_sha256=$(sha256sum "$release_dir/$exe_name" | awk '{print $1}')
    [[ "$installed_sha256" == "$source_sha256" ]] ||
      fail "build ID collision at $release_dir"
    [[ -f "$release_dir/$exe_name.debug" && ! -L "$release_dir/$exe_name.debug" ]] ||
      fail "release debug symbols are not a regular file: $release_dir/$exe_name.debug"
    [[ -f "$release_dir/manifest" && ! -L "$release_dir/manifest" ]] ||
      fail "release manifest is not a regular file: $release_dir/manifest"
    manifest_line_count=$(wc -l < "$release_dir/manifest")
    [[ "$manifest_line_count" -eq 6 ]] ||
      fail "release manifest has the wrong shape: $release_dir/manifest"
    grep -Fxq "FORMAT=1" "$release_dir/manifest" ||
      fail "release manifest has the wrong format: $release_dir/manifest"
    grep -Fxq "VERSION=$source_version" "$release_dir/manifest" ||
      fail "release manifest has the wrong version: $release_dir/manifest"
    grep -Fxq "GIT_COMMIT=$source_commit" "$release_dir/manifest" ||
      fail "release manifest has the wrong Git commit: $release_dir/manifest"
    grep -Fxq "GIT_DIRTY=$source_dirty" "$release_dir/manifest" ||
      fail "release manifest has the wrong dirty flag: $release_dir/manifest"
    grep -Fxq "ELF_BUILD_ID=$source_build_id" "$release_dir/manifest" ||
      fail "release manifest has the wrong build ID: $release_dir/manifest"
    grep -Fxq "SHA256=$source_sha256" "$release_dir/manifest" ||
      fail "release manifest has the wrong SHA-256: $release_dir/manifest"
    debug_build_id=$(readelf -nW "$release_dir/$exe_name.debug" 2>/dev/null |
      awk '/Build ID:/ {print tolower($NF); exit}')
    [[ "$debug_build_id" == "$source_build_id" ]] ||
      fail "release debug symbols have the wrong build ID: $release_dir/$exe_name.debug"
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
}

# The canonical alias must be absent or a symbolic link.  Anything else is a
# hand-managed path this installer will not silently replace.  Checked before
# anything is mutated so a refusal leaves the installation untouched.
validate_alias_path()
{
  local alias_path=$1
  local label=$2

  [[ -L "$alias_path" ]] && return 0
  [[ -e "$alias_path" ]] || return 0
  fail "$label is not a symbolic link; remove it and reinstall"
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

validate_alias_path "$canonical_alias" "bin/$exe_name"

mkdir -p "$release_root"
create_release "$candidate" "$elf_build_id" "$git_commit" \
  "$git_dirty" "$candidate_sha256" "$version"

# A dangling link is replaced in place by the atomic publish below.
publish_symlink "$canonical_alias" "releases/$elf_build_id/$exe_name"

printf 'Installed release: %s\n' "$release_root/$elf_build_id/$exe_name"
printf 'Activated alias: %s -> releases/%s/%s\n' "$canonical_alias" \
  "$elf_build_id" "$exe_name"
printf 'Git commit: %s (dirty=%s)\n' "$git_commit" "$git_dirty"
printf 'ELF build ID: %s\n' "$elf_build_id"
printf 'SHA-256: %s\n' "$candidate_sha256"
