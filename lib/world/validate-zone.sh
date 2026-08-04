#!/usr/bin/env bash
# Compatibility entry point for validating one world-data zone package.

set -euo pipefail

usage()
{
  echo "Usage: $0 <zone_number> [wtool options]" >&2
}

if [ "$#" -lt 1 ]; then
  usage
  exit 1
fi

zone_number=$1
shift

case "$zone_number" in
  ''|*[!0-9]*)
    echo "validate-zone.sh: zone_number must be a non-negative integer" >&2
    exit 2
    ;;
esac

script_dir=$(CDPATH='' cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
repo_root=$(CDPATH='' cd -- "$script_dir/../.." && pwd -P)
wtool_path="$repo_root/scripts/world/wtool.py"

if [ ! -f "$wtool_path" ]; then
  echo "validate-zone.sh: wtool entry point not found at $wtool_path" >&2
  exit 2
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "validate-zone.sh: python3 is required" >&2
  exit 2
fi

global_args=()
validate_args=()
while [ "$#" -gt 0 ]; do
  case "$1" in
    --json)
      global_args+=("$1")
      shift
      ;;
    --world-root|--config|--ignore-code)
      if [ "$#" -lt 2 ]; then
        echo "validate-zone.sh: $1 requires a value" >&2
        exit 2
      fi
      global_args+=("$1" "$2")
      shift 2
      ;;
    --world-root=*|--config=*|--ignore-code=*)
      global_args+=("$1")
      shift
      ;;
    *)
      validate_args+=("$1")
      shift
      ;;
  esac
done

exec python3 "$wtool_path" "${global_args[@]}" validate --zone "$zone_number" "${validate_args[@]}"
