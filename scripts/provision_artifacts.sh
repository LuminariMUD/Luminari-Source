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

# Add object prototypes from the package that the live file does not have
# yet.  Records that already exist are never touched: a builder may have
# edited them through OLC, and that edit is authoritative.
merge_missing_objects() {
    local package_file="$1"
    local live_file="$2"
    local temp_file

    temp_file="$(mktemp "${live_file}.artifact.XXXXXX")"

    awk -v live="$live_file" '
        # Collect the vnums the live file already defines.
        BEGIN {
            while ((getline line < live) > 0)
                if (line ~ /^#[0-9]+$/) {
                    v = substr(line, 2) + 0
                    have[v] = 1
                }
            close(live)
        }
        /^#[0-9]+$/ {
            vnum = substr($0, 2) + 0
            emit = (vnum in have) ? 0 : 1
        }
        /^\$~/ { emit = 0 }
        emit { print }
    ' "$package_file" > "$temp_file"

    if [[ -s "$temp_file" ]]; then
        # Splice the new records in ahead of the terminator.
        awk -v add="$temp_file" '
            /^\$~/ && !done {
                while ((getline line < add) > 0)
                    print line
                close(add)
                done = 1
            }
            { print }
        ' "$live_file" > "${temp_file}.merged"
        chmod --reference="$live_file" "${temp_file}.merged"
        mv "${temp_file}.merged" "$live_file"
    fi

    rm -f "$temp_file"
}

# Retired first-wave identities must never survive alongside their canonical
# successors.  This removes only complete object records whose headers are in
# the closed 169901-169910 migration range.
remove_retired_objects() {
    local live_file="$1"
    local temp_file

    [[ -f "$live_file" ]] || return
    temp_file="$(mktemp "${live_file}.artifact.XXXXXX")"
    awk '
        /^#[0-9]+$/ {
            vnum = substr($0, 2) + 0
            retired = (vnum >= 169901 && vnum <= 169910)
        }
        /^\$~/ { retired = 0 }
        !retired { print }
    ' "$live_file" > "$temp_file"
    chmod --reference="$live_file" "$temp_file"
    mv "$temp_file" "$live_file"
}

# Add reset commands from the package that the live zone does not have yet.
# Existing resets are never rewritten or reordered.
merge_missing_resets() {
    local package_file="$1"
    local live_file="$2"
    local temp_file

    temp_file="$(mktemp "${live_file}.artifact.XXXXXX")"

    awk -v live="$live_file" '
        BEGIN {
            while ((getline line < live) > 0)
                if (line ~ /^O /) {
                    split(line, f, /[ \t]+/)
                    have[f[3] + 0] = 1
                }
            close(live)
        }
        /^O / {
            split($0, f, /[ \t]+/)
            if (!((f[3] + 0) in have))
                print
        }
    ' "$package_file" > "$temp_file"

    if [[ -s "$temp_file" ]]; then
        awk -v add="$temp_file" '
            /^S$/ && !done {
                while ((getline line < add) > 0)
                    print line
                close(add)
                done = 1
            }
            { print }
        ' "$live_file" > "${temp_file}.merged"
        chmod --reference="$live_file" "${temp_file}.merged"
        mv "${temp_file}.merged" "$live_file"
    fi

    rm -f "$temp_file"
}

remove_retired_resets() {
    local live_file="$1"
    local temp_file

    [[ -f "$live_file" ]] || return
    temp_file="$(mktemp "${live_file}.artifact.XXXXXX")"
    awk '
        /^O / {
            split($0, fields, /[ \t]+/)
            vnum = fields[3] + 0
            if (vnum >= 169901 && vnum <= 169910)
                next
        }
        { print }
    ' "$live_file" > "$temp_file"
    chmod --reference="$live_file" "$temp_file"
    mv "$temp_file" "$live_file"
}

provision_world_file() {
    local kind="$1"
    local filename="$2"
    local destination_dir="$PROJECT_ROOT/lib/world/$kind"

    mkdir -p "$destination_dir"
    if [[ ! -f "$destination_dir/$filename" ]]; then
        cp "$PACKAGE_DIR/$filename" "$destination_dir/$filename"
    else
        # The file exists from an earlier provision, so it must not be
        # replaced wholesale - but artifacts added to the package since then
        # still have to reach the world.  Add what is missing, change
        # nothing that is already there.
        case "$kind" in
            obj) merge_missing_objects "$PACKAGE_DIR/$filename" "$destination_dir/$filename" ;;
            zon) merge_missing_resets "$PACKAGE_DIR/$filename" "$destination_dir/$filename" ;;
        esac
    fi
    ensure_index_entry "$destination_dir/index" "$filename"
}

for kind in zon wld mob; do
    provision_world_file "$kind" "1699.$kind"
done

mkdir -p "$PROJECT_ROOT/lib/world/obj" "$PROJECT_ROOT/lib/world/zon"
remove_retired_objects "$PROJECT_ROOT/lib/world/obj/1699.obj"
remove_retired_resets "$PROJECT_ROOT/lib/world/zon/1699.zon"
for artifact_object_file in 1699.obj 20010.obj 20053.obj 20197.obj; do
    provision_world_file obj "$artifact_object_file"
done
merge_missing_resets "$PACKAGE_DIR/1699.zon" "$PROJECT_ROOT/lib/world/zon/1699.zon"

mkdir -p "$PROJECT_ROOT/lib/text/help"
if [[ ! -f "$PROJECT_ROOT/lib/text/help/artifacts.hlp" ]] ||
   ! cmp -s "$PACKAGE_DIR/artifacts.hlp" "$PROJECT_ROOT/lib/text/help/artifacts.hlp"; then
    cp "$PACKAGE_DIR/artifacts.hlp" "$PROJECT_ROOT/lib/text/help/artifacts.hlp"
fi
ensure_index_entry "$PROJECT_ROOT/lib/text/help/index" "artifacts.hlp"

echo "Artifact world data provisioned."
