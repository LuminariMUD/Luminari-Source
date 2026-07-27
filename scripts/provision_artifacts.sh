#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${LUMINARI_PROJECT_ROOT:-$(cd "$SCRIPT_DIR/.." && pwd)}"
PACKAGE_DIR="$PROJECT_ROOT/lib/world/artifacts"

ensure_index_entry() {
    local index_file="$1"
    local entry="$2"
    local temp_file

    if [[ ! -f "$index_file" ]]; then
        printf '%s\n$\n' "$entry" > "$index_file"
        return
    fi

    if grep -Fqx "$entry" "$index_file"; then
        return
    fi

    temp_file="$(mktemp "${index_file}.artifact.XXXXXX")"
    awk -v entry="$entry" '
        $0 == "$" && !inserted {
            print entry
            inserted = 1
        }
        {
            print
        }
        END {
            if (!inserted) {
                print entry
                print "$"
            }
        }
    ' "$index_file" > "$temp_file"
    chmod --reference="$index_file" "$temp_file"
    mv "$temp_file" "$index_file"
}

provision_world_file() {
    local kind="$1"
    local filename="1699.$kind"
    local destination_dir="$PROJECT_ROOT/lib/world/$kind"

    mkdir -p "$destination_dir"
    if [[ ! -f "$destination_dir/$filename" ]]; then
        cp "$PACKAGE_DIR/$filename" "$destination_dir/$filename"
    fi
    ensure_index_entry "$destination_dir/index" "$filename"
}

for kind in zon wld mob obj; do
    provision_world_file "$kind"
done

mkdir -p "$PROJECT_ROOT/lib/text/help"
if [[ ! -f "$PROJECT_ROOT/lib/text/help/artifacts.hlp" ]]; then
    cp "$PACKAGE_DIR/artifacts.hlp" "$PROJECT_ROOT/lib/text/help/artifacts.hlp"
fi
ensure_index_entry "$PROJECT_ROOT/lib/text/help/index" "artifacts.hlp"

echo "Artifact world data provisioned."
