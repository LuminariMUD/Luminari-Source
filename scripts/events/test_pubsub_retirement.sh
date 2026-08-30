#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)

fail()
{
  echo "pubsub retirement test: $*" >&2
  exit 1
}

assert_absent()
{
  local pattern=$1
  shift
  if rg --fixed-strings --quiet -- "$pattern" "$@"; then
    fail "unexpected '$pattern' in retired runtime surface"
  fi
}

shopt -s nullglob
retired_sources=("$project_root"/src/pubsub/*.[ch])
(( ${#retired_sources[@]} == 0 )) || fail "src/pubsub still contains source files"

runtime_files=(
  "$project_root/src/comm.c"
  "$project_root/src/db.c"
  "$project_root/src/db_init.c"
  "$project_root/src/db_startup_init.c"
  "$project_root/src/interpreter.c"
  "$project_root/src/player_rename.c"
)

for symbol in pubsub_init pubsub_process_message_queue pubsub_db_create_tables \
  pubsub_db_create_v3_tables pubsub_invalidate_player_cache; do
  assert_absent "$symbol" "${runtime_files[@]}"
done

for command in '"pubsub"' '"pubsubtopic"' '"pubsubqueue"' '"subscribe"' '"topics"'; do
  assert_absent "$command" "$project_root/src/interpreter.c"
done

assert_absent "src/pubsub/" "$project_root/Makefile.am" "$project_root/CMakeLists.txt"
assert_absent "pubsub_topic" "$project_root/src/wilderness/spatial_core.h" \
  "$project_root/src/wilderness/spatial_visual.c" "$project_root/src/wilderness/spatial_audio.c"
assert_absent "pubsub_handler" "$project_root/src/wilderness/spatial_core.h" \
  "$project_root/src/wilderness/spatial_visual.c" "$project_root/src/wilderness/spatial_audio.c"
assert_absent "spatial_visual_meteor_" "$project_root/src/magic/magic.c" \
  "$project_root/src/wilderness/spatial_visual.h"
assert_absent "spatial_audio_test_" "$project_root/src/magic/magic.c" \
  "$project_root/src/wilderness/spatial_audio.h"

rg --fixed-strings --quiet -- "DEPRECATED: retired PubSub messaging system" \
  "$project_root/sql/master_schema.sql" || fail "master schema lacks deprecation marker"

for table in pubsub_topics pubsub_player_settings pubsub_subscriptions \
  pubsub_messages pubsub_message_metadata pubsub_message_fields \
  pubsub_messages_v3 pubsub_message_metadata_v3 pubsub_message_fields_v3 \
  pubsub_message_tags_v3; do
  rg --quiet -- "CREATE TABLE IF NOT EXISTS[[:space:]]+$table" \
    "$project_root/sql/master_schema.sql" || fail "preserved table $table is missing"
done

for table in pubsub_message_stats_v3 pubsub_filters_v3; do
  rg --quiet -- "CREATE TABLE IF NOT EXISTS[[:space:]]+$table" \
    "$project_root/sql/components/pubsub_v3_schema.sql" \
    "$project_root/lib/pubsub_v3_schema.sql" || fail "archival table $table is missing"
done

if rg --ignore-case --quiet -- 'DROP[[:space:]]+TABLE[^;]*pubsub_' \
  "$project_root/sql" "$project_root/scripts"; then
  fail "a PubSub table drop was introduced"
fi

echo "pubsub retirement test: PASS"
