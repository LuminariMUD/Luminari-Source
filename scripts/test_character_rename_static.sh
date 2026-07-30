#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
rename_source="$project_root/src/player_rename.c"
wizard_source="$project_root/src/act.wizard.c"

fail()
{
  echo "character rename static test: $*" >&2
  exit 1
}

assert_contains()
{
  local file=$1
  local pattern=$2

  rg --fixed-strings --quiet -- "$pattern" "$file" ||
    fail "missing '$pattern' in ${file#"$project_root"/}"
}

assert_not_contains()
{
  local file=$1
  local pattern=$2

  if rg --fixed-strings --quiet -- "$pattern" "$file"; then
    fail "unexpected '$pattern' in ${file#"$project_root"/}"
  fi
}

rename_keys=(
  "player_data|name"
  "player_save_objs|name"
  "player_save_objs_sheathed|owner_name"
  "pet_data|owner_name"
  "pet_save_objs|owner_name"
  "player_eidolons|owner"
  "loot_chests|character_name"
  "player_mail|sender"
  "player_mail|receiver"
  "player_mail_deleted|player_name"
  "player_mail_read|player_name"
  "player_quest_info|character_name"
  "player_quest_progress|character_name"
  "player_supply_orders|player_name"
  "supply_orders_available|player_name"
  "pubsub_player_settings|player_name"
  "pubsub_subscriptions|player_name"
  "player_levelups|character_name"
  "player_levels|char_name"
  "vessel_bounties|player_name"
  "vessel_merchant_consequences|player_name"
  "vessel_npc_merchants|last_attacker_name"
  "vessel_bounty_hunts|target_player"
)

for key in "${rename_keys[@]}"; do
  table=${key%%|*}
  column=${key#*|}
  assert_contains "$rename_source" "{\"$table\", \"$column\""
done

assert_contains "$wizard_source" "rename_player_everywhere(ch, vict, new_name, &report)"
assert_not_contains "$wizard_source" "UPDATE player_data SET name"
assert_not_contains "$wizard_source" "system(buf)"
assert_not_contains "$rename_source" "system("

assert_contains "$rename_source" "START TRANSACTION"
assert_contains "$rename_source" "\"COMMIT\""
assert_contains "$rename_source" "\"ROLLBACK\""
assert_contains "$rename_source" "parse_player_name(parse_input, parsed_name)"
assert_contains "$rename_source" "mysql_real_escape_string(conn, escaped, name, length)"
assert_contains "$rename_source" "save_player_index_checked()"
assert_contains "$rename_source" "rename_capture_payloads(ctx, ctx->escaped_old_name, FALSE)"
assert_contains "$rename_source" "rename_capture_payloads(ctx, ctx->escaped_new_name, TRUE)"
assert_contains "$rename_source" "rename_load_preserve_assignments"
assert_contains "$rename_source" "ctx->keys[i].preserve_assignments"
assert_contains "$rename_source" "rename_require_innodb_table(&ctx, \"account_data\")"
assert_contains "$rename_source" "rename_digest_regular_file"
assert_contains "$rename_source" "ctx->files[i].source_digest"
assert_contains "$rename_source" "rename_scan_pfile_line"
assert_contains "$project_root/unittests/CuTest/test_player_rename.c" \
  "Test_player_rename_ignores_identity_tags_in_nested_records"
assert_contains "$rename_source" "pubsub_invalidate_player_cache(ctx->old_display_name)"
assert_contains "$rename_source" "pubsub_invalidate_player_cache(ctx->new_display_name)"
assert_contains "$rename_source" "load_account_characters(refresh->account)"
failure_points=(
  PLAYER_RENAME_TEST_FAIL_AUXILIARY_MOVE
  PLAYER_RENAME_TEST_FAIL_DATABASE_UPDATE
  PLAYER_RENAME_TEST_FAIL_PLAYER_FILE_WRITE
  PLAYER_RENAME_TEST_FAIL_INTRODUCTION_WRITE
  PLAYER_RENAME_TEST_FAIL_INDEX_WRITE
  PLAYER_RENAME_TEST_FAIL_POSTCONDITION
  PLAYER_RENAME_TEST_FAIL_COMMIT
)
for failure_point in "${failure_points[@]}"; do
  assert_contains "$rename_source" "$failure_point"
done

assert_contains "$project_root/src/players.c" "player_file_account_name(ch)"
assert_contains "$project_root/src/players.c" "if (GET_MOB_VNUM(mob) == MOB_CLONE)"
assert_contains "$project_root/Makefile.am" "src/player_rename.c"
assert_contains "$project_root/CMakeLists.txt" "src/player_rename.c"

for table in player_mail player_mail_deleted player_mail_read; do
  assert_contains "$project_root/sql/components/character_rename_transactional_schema.sql" \
    "ALTER TABLE $table ENGINE=InnoDB"
done

implementation_files=(
  "$rename_source"
  "$project_root/src/player_rename.h"
  "$project_root/unittests/CuTest/test_player_rename.c"
  "$project_root/scripts/test_character_rename_schema.sh"
  "$project_root/sql/components/character_rename_transactional_schema.sql"
)
for incident_value in Bartof Hartof DollhouseTestChar Ralmont 5063 475 118 641; do
  for file in "${implementation_files[@]}"; do
    assert_not_contains "$file" "$incident_value"
  done
done

echo "character rename static test: PASS"
