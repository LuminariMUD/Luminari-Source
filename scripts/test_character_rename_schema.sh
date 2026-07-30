#!/usr/bin/env bash

set -euo pipefail

project_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
test_root=$(mktemp -d /tmp/luminari-character-rename.XXXXXX)
data_dir="$test_root/data"
socket_path="$test_root/mariadb.sock"
pid_path="$test_root/mariadb.pid"
log_path="$test_root/mariadb.log"
server_pid=

cleanup()
{
  if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
    kill "$server_pid"
    wait "$server_pid" 2>/dev/null || true
  fi
  find "$test_root" -depth -delete
}
trap cleanup EXIT

fail()
{
  echo "character rename schema test: $*" >&2
  exit 1
}

sql()
{
  mariadb --no-defaults --batch --skip-column-names \
    --socket="$socket_path" -u root rename_test "$@"
}

assert_sql()
{
  local query=$1
  local expected=$2
  local actual

  actual=$(sql -e "$query")
  [[ "$actual" == "$expected" ]] ||
    fail "expected '$expected' but got '$actual' for: $query"
}

command -v mariadb-install-db >/dev/null ||
  fail "mariadb-install-db is required"
command -v mariadbd >/dev/null ||
  fail "mariadbd is required"

mkdir "$data_dir"
mariadb-install-db --no-defaults --datadir="$data_dir" \
  --auth-root-authentication-method=normal --skip-test-db >/dev/null 2>&1

mariadbd --no-defaults --datadir="$data_dir" --socket="$socket_path" \
  --pid-file="$pid_path" --log-error="$log_path" --skip-networking &
server_pid=$!

for _ in {1..40}; do
  if mariadb-admin --no-defaults --socket="$socket_path" -u root ping >/dev/null 2>&1; then
    break
  fi
  sleep 0.25
done
mariadb-admin --no-defaults --socket="$socket_path" -u root ping >/dev/null 2>&1 ||
  fail "temporary MariaDB did not start; see $log_path"

mariadb --no-defaults --socket="$socket_path" -u root \
  -e "CREATE DATABASE rename_test CHARACTER SET utf8mb4"
sql < "$project_root/sql/master_schema.sql"

# Exercise optional deployed objects that are intentionally discovered at
# runtime rather than required in every schema.
sql -e "
  CREATE TABLE player_levelups (
    id INT AUTO_INCREMENT PRIMARY KEY,
    character_name VARCHAR(30) NOT NULL,
    event_payload TEXT NOT NULL
  ) ENGINE=InnoDB;
  CREATE TABLE player_levels (
    id INT AUTO_INCREMENT PRIMARY KEY,
    char_name VARCHAR(30) NOT NULL,
    event_payload TEXT NOT NULL
  ) ENGINE=InnoDB;
  CREATE VIEW level_30_characters AS
    SELECT name FROM player_data WHERE level >= 30;
"

# Exercise the production prerequisite migration from the audited legacy
# MyISAM shape, then verify that it preserves data.
sql -e "
  ALTER TABLE player_mail
    DROP INDEX idx_player_mail_sender,
    DROP INDEX idx_player_mail_receiver;
  ALTER TABLE player_mail_deleted
    DROP PRIMARY KEY,
    DROP INDEX idx_player_mail_deleted_mail;
  ALTER TABLE player_mail_read
    DROP PRIMARY KEY,
    DROP INDEX idx_player_mail_read_mail;
  ALTER TABLE player_mail ENGINE=MyISAM;
  ALTER TABLE player_mail_deleted ENGINE=MyISAM;
  ALTER TABLE player_mail_read ENGINE=MyISAM;
  INSERT INTO player_mail (sender, receiver, subject, message)
    VALUES ('LegacySender', 'All', 'legacy subject', 'legacy payload');
  SET @legacy_mail_id = LAST_INSERT_ID();
  INSERT INTO player_mail_deleted (player_name, mail_id)
    VALUES ('LegacyReader', @legacy_mail_id);
  INSERT INTO player_mail_read (player_name, mail_id)
    VALUES ('LegacyReader', @legacy_mail_id);
"
sql < "$project_root/sql/components/character_rename_transactional_schema.sql" >/dev/null
sql < "$project_root/sql/components/vessels_phase15_schema.sql" >/dev/null
assert_sql "
  SELECT COUNT(*) FROM information_schema.TABLES
  WHERE TABLE_SCHEMA=DATABASE()
    AND TABLE_NAME IN ('player_mail','player_mail_deleted','player_mail_read')
    AND ENGINE='InnoDB';
" "3"
assert_sql "
  SELECT COUNT(*) FROM information_schema.TABLES
  WHERE TABLE_SCHEMA=DATABASE()
    AND TABLE_NAME IN (
      'player_data',
      'player_save_objs',
      'player_save_objs_sheathed',
      'pet_data',
      'pet_save_objs',
      'player_eidolons',
      'loot_chests',
      'player_mail',
      'player_mail_deleted',
      'player_mail_read',
      'player_quest_info',
      'player_quest_progress',
      'player_supply_orders',
      'supply_orders_available',
      'pubsub_player_settings',
      'pubsub_subscriptions',
      'player_levelups',
      'player_levels',
      'vessel_hunter_encounters',
      'vessel_bounty_hunts'
    )
    AND TABLE_TYPE='BASE TABLE'
    AND ENGINE='InnoDB';
" "20"
assert_sql "
  SELECT CONCAT(sender, '|', receiver, '|', subject, '|', message)
  FROM player_mail WHERE subject='legacy subject';
" "LegacySender|All|legacy subject|legacy payload"
assert_sql "
  SELECT CONCAT(
    (SELECT COUNT(*) FROM player_mail_deleted d
       JOIN player_mail m ON m.mail_id=d.mail_id
       WHERE d.player_name='LegacyReader' AND m.subject='legacy subject'), '|',
    (SELECT COUNT(*) FROM player_mail_read r
       JOIN player_mail m ON m.mail_id=r.mail_id
       WHERE r.player_name='LegacyReader' AND m.subject='legacy subject')
  );
" "1|1"

# Every guaranteed active-name key in the fresh schema must have an index with
# that key as its leading column. Optional level-history tables are discovered
# at runtime and are outside this guaranteed set.
assert_sql "
  SELECT COUNT(*)
  FROM (
    SELECT 'player_data' AS table_name, 'name' AS column_name
    UNION ALL SELECT 'player_save_objs', 'name'
    UNION ALL SELECT 'player_save_objs_sheathed', 'owner_name'
    UNION ALL SELECT 'pet_data', 'owner_name'
    UNION ALL SELECT 'pet_save_objs', 'owner_name'
    UNION ALL SELECT 'player_eidolons', 'owner'
    UNION ALL SELECT 'loot_chests', 'character_name'
    UNION ALL SELECT 'player_mail', 'sender'
    UNION ALL SELECT 'player_mail', 'receiver'
    UNION ALL SELECT 'player_mail_deleted', 'player_name'
    UNION ALL SELECT 'player_mail_read', 'player_name'
    UNION ALL SELECT 'player_quest_info', 'character_name'
    UNION ALL SELECT 'player_quest_progress', 'character_name'
    UNION ALL SELECT 'player_supply_orders', 'player_name'
    UNION ALL SELECT 'supply_orders_available', 'player_name'
    UNION ALL SELECT 'pubsub_player_settings', 'player_name'
    UNION ALL SELECT 'pubsub_subscriptions', 'player_name'
    UNION ALL SELECT 'vessel_bounty_hunts', 'target_player'
  ) AS expected
  WHERE EXISTS (
    SELECT 1
    FROM information_schema.STATISTICS AS indexes
    WHERE indexes.TABLE_SCHEMA=DATABASE()
      AND indexes.TABLE_NAME=expected.table_name
      AND indexes.COLUMN_NAME=expected.column_name
      AND indexes.SEQ_IN_INDEX=1
  );
" "18"

sql -e "
  INSERT INTO account_data (name, password)
    VALUES ('FixtureAccount', 'fixture-password');
  SET @account_id = LAST_INSERT_ID();

  INSERT INTO player_data
    (name, race, classes, level, account_id, obj_save_header, character_info)
    VALUES ('Sourcechar', 4, 8, 31, @account_id, 'header bytes', 'level snapshot');

  INSERT INTO player_save_objs
    (name, serialized_obj, creation_date, idnum)
    VALUES
      ('Sourcechar', 'object payload one', '2026-01-01 01:02:03.000001', 70001),
      ('Sourcechar', 'object payload two', '2026-01-01 01:02:04.000002', 70002);
  INSERT INTO player_save_objs_sheathed
    (sheath_obj_id, sheathed_position, owner_name, serialized_obj, creation_date)
    VALUES (70001, 2, 'Sourcechar', 'sheathed payload', '2026-01-01 01:02:05');
  INSERT INTO pet_data
    (owner_name, vnum, hp, max_hp, str, con, dex, level, ac, intel, wis, cha,
     pet_name, pet_sdesc, pet_ldesc, pet_ddesc)
    VALUES
      ('Sourcechar', 10, 20, 30, 11, 12, 13, 14, 15, 16, 17, 18,
       'authored pet name', 'authored short', 'authored long', 'authored description');
  SET @pet_id = LAST_INSERT_ID();
  INSERT INTO pet_save_objs
    (pet_idnum, owner_name, serialized_obj, creation_date)
    VALUES (@pet_id, 'Sourcechar', 'pet object payload', '2026-01-01 01:02:06');
  INSERT INTO player_eidolons (owner, short_desc, long_desc)
    VALUES ('Sourcechar', 'eidolon short', 'eidolon long');
  INSERT INTO loot_chests (chest_vnum, character_name, last_loot)
    VALUES (12345, 'Sourcechar', '2026-01-01 01:02:07');

  INSERT INTO player_mail (sender, receiver, subject, message)
    VALUES
      ('Sourcechar', 'Otherchar', 'sent', 'sent payload'),
      ('Otherchar', 'Sourcechar', 'received', 'received payload'),
      ('Otherchar', 'All', 'broadcast', 'broadcast payload');
  SET @received_mail = LAST_INSERT_ID() + 1;
  INSERT INTO player_mail_deleted (player_name, mail_id)
    VALUES ('Sourcechar', @received_mail);
  INSERT INTO player_mail_read (player_name, mail_id)
    VALUES ('Sourcechar', @received_mail);

  INSERT INTO player_quest_info
    (character_name, quest_done, quest_id, in_progress)
    VALUES ('Sourcechar', '2026-01-01 01:02:08', 45, 1);
  INSERT INTO player_quest_progress (quest_id, character_name, num_obtained)
    VALUES (45, 'Sourcechar', 3);
  INSERT INTO player_supply_orders (player_name, supply_orders_available)
    VALUES ('Sourcechar', 6);
  INSERT INTO supply_orders_available
    (player_name, supply_orders_available, last_updated)
    VALUES ('Sourcechar', 7, '2026-01-01 01:02:09');

  INSERT INTO pubsub_topics (topic_name) VALUES ('fixture.topic');
  SET @topic_id = LAST_INSERT_ID();
  INSERT INTO pubsub_player_settings (player_name, max_subscriptions)
    VALUES ('Sourcechar', 19);
  INSERT INTO pubsub_subscriptions (topic_id, player_name, status)
    VALUES (@topic_id, 'Sourcechar', 1);
  INSERT INTO player_levelups (character_name, event_payload)
    VALUES ('Sourcechar', 'new level history payload');
  INSERT INTO player_levels (char_name, event_payload)
    VALUES ('Sourcechar', 'legacy level history payload');
  INSERT INTO vessel_bounty_hunts
    (target_player, encounter_id, target_ship_id, hunter_ship_id,
     hunter_name, generation, status, started_at, expires_at,
     next_eligible_at, ended_at, end_reason)
    VALUES
      ('Sourcechar', 77, 41, 42, 'Admiralty hunter #9', 9, 'active',
       1000, 1300, 0, 0, '');
"

payload_before=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT player_idnum FROM player_data WHERE name='Sourcechar'),
    (SELECT obj_save_header FROM player_data WHERE name='Sourcechar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', HEX(serialized_obj), ':',
             DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s.%f'))
            ORDER BY creation_date SEPARATOR ';')
       FROM player_save_objs WHERE name='Sourcechar'),
    (SELECT GROUP_CONCAT(CONCAT(id, ':', sheath_obj_id, ':', sheathed_position,
             ':', HEX(serialized_obj), ':',
             DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s'))
             ORDER BY id SEPARATOR ';')
       FROM player_save_objs_sheathed WHERE owner_name='Sourcechar'),
    (SELECT GROUP_CONCAT(CONCAT(pet_data_id, ':', vnum, ':', hp, ':', max_hp,
             ':', str, ':', con, ':', dex, ':', level, ':', ac, ':', intel,
             ':', wis, ':', cha, ':', pet_name, ':', pet_sdesc, ':',
             pet_ldesc, ':', pet_ddesc)
             ORDER BY pet_data_id SEPARATOR ';')
       FROM pet_data WHERE owner_name='Sourcechar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', pet_idnum, ':', HEX(serialized_obj),
             ':', DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s'))
             ORDER BY idnum SEPARATOR ';')
       FROM pet_save_objs WHERE owner_name='Sourcechar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', short_desc, ':', long_desc)
             ORDER BY idnum SEPARATOR ';')
       FROM player_eidolons WHERE owner='Sourcechar')
  ), 256);
")
mail_payload_before=$(sql -e "
  SELECT SHA2(GROUP_CONCAT(CONCAT(mail_id, ':', subject, ':', HEX(message), ':',
    COALESCE(DATE_FORMAT(date_sent, '%Y-%m-%d'), 'NULL'))
    ORDER BY mail_id SEPARATOR ';'), 256)
  FROM player_mail
  WHERE subject IN ('sent', 'received', 'broadcast');
")
mail_state_before=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT GROUP_CONCAT(mail_id ORDER BY mail_id)
       FROM player_mail_deleted WHERE player_name='Sourcechar'),
    (SELECT GROUP_CONCAT(mail_id ORDER BY mail_id)
       FROM player_mail_read WHERE player_name='Sourcechar')
  ), 256);
")
state_payload_before=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT CONCAT_WS(':', loot_id, chest_vnum,
             DATE_FORMAT(last_loot, '%Y-%m-%d %H:%i:%s'))
       FROM loot_chests WHERE character_name='Sourcechar'),
    (SELECT CONCAT_WS(':', id_num, DATE_FORMAT(quest_done, '%Y-%m-%d %H:%i:%s'),
             quest_id, in_progress)
       FROM player_quest_info WHERE character_name='Sourcechar'),
    (SELECT CONCAT_WS(':', prog_id, quest_id, num_obtained)
       FROM player_quest_progress WHERE character_name='Sourcechar'),
    (SELECT CONCAT_WS(':', idnum, supply_orders_available)
       FROM player_supply_orders WHERE player_name='Sourcechar'),
    (SELECT CONCAT_WS(':', idnum, supply_orders_available,
             DATE_FORMAT(last_updated, '%Y-%m-%d %H:%i:%s'))
       FROM supply_orders_available WHERE player_name='Sourcechar'),
    (SELECT CONCAT_WS(':', setting_id, max_subscriptions)
       FROM pubsub_player_settings WHERE player_name='Sourcechar'),
    (SELECT CONCAT_WS(':', subscription_id, topic_id, status,
             DATE_FORMAT(subscribed_at, '%Y-%m-%d %H:%i:%s'))
       FROM pubsub_subscriptions WHERE player_name='Sourcechar'),
    (SELECT CONCAT_WS(':', id, event_payload)
       FROM player_levelups WHERE character_name='Sourcechar'),
    (SELECT CONCAT_WS(':', id, event_payload)
       FROM player_levels WHERE char_name='Sourcechar')
  ), 256);
")
hunt_payload_before=$(sql -e "
  SELECT SHA2(CONCAT_WS('|', encounter_id, target_ship_id, hunter_ship_id,
    hunter_name, generation, status, started_at, expires_at,
    next_eligible_at, ended_at, end_reason), 256)
  FROM vessel_bounty_hunts
  WHERE target_player='Sourcechar';
")

# The runtime preflight uses locking aggregate reads to protect old/new name
# ranges against concurrent external writes until the transaction commits.
sql -e "
  START TRANSACTION;
  SELECT COUNT(*) FROM player_data
    WHERE LOWER(name)=LOWER('Sourcechar') FOR UPDATE;
  SELECT COUNT(*) FROM player_mail
    WHERE LOWER(sender)=LOWER('Sourcechar') FOR UPDATE;
  SELECT name FROM account_data WHERE id=1 FOR UPDATE;
  SELECT name FROM player_data
    WHERE account_id=1 ORDER BY name FOR UPDATE;
  ROLLBACK;
" >/dev/null

sql -e "
  START TRANSACTION;
  UPDATE player_data SET name='Targetchar'
    WHERE LOWER(name)=LOWER('Sourcechar');
  UPDATE player_save_objs SET name='Targetchar'
    WHERE LOWER(name)=LOWER('Sourcechar');
  UPDATE player_save_objs_sheathed SET owner_name='Targetchar'
    WHERE LOWER(owner_name)=LOWER('Sourcechar');
  UPDATE pet_data SET owner_name='Targetchar'
    WHERE LOWER(owner_name)=LOWER('Sourcechar');
  UPDATE pet_save_objs SET owner_name='Targetchar'
    WHERE LOWER(owner_name)=LOWER('Sourcechar');
  UPDATE player_eidolons SET owner='Targetchar'
    WHERE LOWER(owner)=LOWER('Sourcechar');
  UPDATE loot_chests SET character_name='Targetchar'
    WHERE LOWER(character_name)=LOWER('Sourcechar');
  UPDATE player_mail SET sender='Targetchar'
    WHERE LOWER(sender)=LOWER('Sourcechar');
  UPDATE player_mail SET receiver='Targetchar'
    WHERE LOWER(receiver)=LOWER('Sourcechar')
      AND LOWER(receiver) <> LOWER('All');
  UPDATE player_mail_deleted SET player_name='Targetchar'
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE player_mail_read SET player_name='Targetchar'
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE player_quest_info SET character_name='Targetchar'
    WHERE LOWER(character_name)=LOWER('Sourcechar');
  UPDATE player_quest_progress SET character_name='Targetchar'
    WHERE LOWER(character_name)=LOWER('Sourcechar');
  UPDATE player_supply_orders SET player_name='Targetchar'
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE supply_orders_available
    SET player_name='Targetchar', last_updated=last_updated
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE pubsub_player_settings SET player_name='Targetchar'
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE pubsub_subscriptions SET player_name='Targetchar'
    WHERE LOWER(player_name)=LOWER('Sourcechar');
  UPDATE player_levelups SET character_name='Targetchar'
    WHERE LOWER(character_name)=LOWER('Sourcechar');
  UPDATE player_levels SET char_name='Targetchar'
    WHERE LOWER(char_name)=LOWER('Sourcechar');
  UPDATE vessel_bounty_hunts SET target_player='Targetchar'
    WHERE LOWER(target_player)=LOWER('Sourcechar');
  COMMIT;
"

assert_sql "
  SELECT
    (SELECT COUNT(*) FROM player_data WHERE LOWER(name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_save_objs WHERE LOWER(name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_save_objs_sheathed
       WHERE LOWER(owner_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM pet_data WHERE LOWER(owner_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM pet_save_objs WHERE LOWER(owner_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_eidolons WHERE LOWER(owner)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM loot_chests WHERE LOWER(character_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_mail
       WHERE LOWER(sender)=LOWER('Sourcechar') OR LOWER(receiver)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_mail_deleted
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_mail_read
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_quest_info
       WHERE LOWER(character_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_quest_progress
       WHERE LOWER(character_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_supply_orders
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM supply_orders_available
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM pubsub_player_settings
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM pubsub_subscriptions
       WHERE LOWER(player_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_levelups
       WHERE LOWER(character_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM player_levels
       WHERE LOWER(char_name)=LOWER('Sourcechar')) +
    (SELECT COUNT(*) FROM vessel_bounty_hunts
       WHERE LOWER(target_player)=LOWER('Sourcechar'));
" "0"

assert_sql "
  SELECT CONCAT(account_id, '|', obj_save_header)
  FROM player_data WHERE name='Targetchar';
" "1|header bytes"
assert_sql "
  SELECT COUNT(*) FROM player_mail
  WHERE receiver='All' AND subject IN ('legacy subject', 'broadcast');
" "2"
assert_sql "
  SELECT GROUP_CONCAT(name ORDER BY name)
  FROM level_30_characters;
" "Targetchar"

payload_after=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT player_idnum FROM player_data WHERE name='Targetchar'),
    (SELECT obj_save_header FROM player_data WHERE name='Targetchar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', HEX(serialized_obj), ':',
             DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s.%f'))
            ORDER BY creation_date SEPARATOR ';')
       FROM player_save_objs WHERE name='Targetchar'),
    (SELECT GROUP_CONCAT(CONCAT(id, ':', sheath_obj_id, ':', sheathed_position,
             ':', HEX(serialized_obj), ':',
             DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s'))
             ORDER BY id SEPARATOR ';')
       FROM player_save_objs_sheathed WHERE owner_name='Targetchar'),
    (SELECT GROUP_CONCAT(CONCAT(pet_data_id, ':', vnum, ':', hp, ':', max_hp,
             ':', str, ':', con, ':', dex, ':', level, ':', ac, ':', intel,
             ':', wis, ':', cha, ':', pet_name, ':', pet_sdesc, ':',
             pet_ldesc, ':', pet_ddesc)
             ORDER BY pet_data_id SEPARATOR ';')
       FROM pet_data WHERE owner_name='Targetchar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', pet_idnum, ':', HEX(serialized_obj),
             ':', DATE_FORMAT(creation_date, '%Y-%m-%d %H:%i:%s'))
             ORDER BY idnum SEPARATOR ';')
       FROM pet_save_objs WHERE owner_name='Targetchar'),
    (SELECT GROUP_CONCAT(CONCAT(idnum, ':', short_desc, ':', long_desc)
             ORDER BY idnum SEPARATOR ';')
       FROM player_eidolons WHERE owner='Targetchar')
  ), 256);
")
[[ "$payload_before" == "$payload_after" ]] ||
  fail "object, pet, sheath, or eidolon payload changed during key migration"
mail_payload_after=$(sql -e "
  SELECT SHA2(GROUP_CONCAT(CONCAT(mail_id, ':', subject, ':', HEX(message), ':',
    COALESCE(DATE_FORMAT(date_sent, '%Y-%m-%d'), 'NULL'))
    ORDER BY mail_id SEPARATOR ';'), 256)
  FROM player_mail
  WHERE subject IN ('sent', 'received', 'broadcast');
")
mail_state_after=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT GROUP_CONCAT(mail_id ORDER BY mail_id)
       FROM player_mail_deleted WHERE player_name='Targetchar'),
    (SELECT GROUP_CONCAT(mail_id ORDER BY mail_id)
       FROM player_mail_read WHERE player_name='Targetchar')
  ), 256);
")
state_payload_after=$(sql -e "
  SELECT SHA2(CONCAT_WS('|',
    (SELECT CONCAT_WS(':', loot_id, chest_vnum,
             DATE_FORMAT(last_loot, '%Y-%m-%d %H:%i:%s'))
       FROM loot_chests WHERE character_name='Targetchar'),
    (SELECT CONCAT_WS(':', id_num, DATE_FORMAT(quest_done, '%Y-%m-%d %H:%i:%s'),
             quest_id, in_progress)
       FROM player_quest_info WHERE character_name='Targetchar'),
    (SELECT CONCAT_WS(':', prog_id, quest_id, num_obtained)
       FROM player_quest_progress WHERE character_name='Targetchar'),
    (SELECT CONCAT_WS(':', idnum, supply_orders_available)
       FROM player_supply_orders WHERE player_name='Targetchar'),
    (SELECT CONCAT_WS(':', idnum, supply_orders_available,
             DATE_FORMAT(last_updated, '%Y-%m-%d %H:%i:%s'))
       FROM supply_orders_available WHERE player_name='Targetchar'),
    (SELECT CONCAT_WS(':', setting_id, max_subscriptions)
       FROM pubsub_player_settings WHERE player_name='Targetchar'),
    (SELECT CONCAT_WS(':', subscription_id, topic_id, status,
             DATE_FORMAT(subscribed_at, '%Y-%m-%d %H:%i:%s'))
       FROM pubsub_subscriptions WHERE player_name='Targetchar'),
    (SELECT CONCAT_WS(':', id, event_payload)
       FROM player_levelups WHERE character_name='Targetchar'),
    (SELECT CONCAT_WS(':', id, event_payload)
       FROM player_levels WHERE char_name='Targetchar')
  ), 256);
")
hunt_payload_after=$(sql -e "
  SELECT SHA2(CONCAT_WS('|', encounter_id, target_ship_id, hunter_ship_id,
    hunter_name, generation, status, started_at, expires_at,
    next_eligible_at, ended_at, end_reason), 256)
  FROM vessel_bounty_hunts
  WHERE target_player='Targetchar';
")
[[ "$mail_payload_before" == "$mail_payload_after" ]] ||
  fail "mail identity, subject, message, or date changed during key migration"
[[ "$mail_state_before" == "$mail_state_after" ]] ||
  fail "mail read/deleted relationships changed during key migration"
[[ "$state_payload_before" == "$state_payload_after" ]] ||
  fail "loot, quest, crafting, pubsub, or level payload changed during key migration"
[[ "$hunt_payload_before" == "$hunt_payload_after" ]] ||
  fail "bounty-hunter lifecycle payload changed during key migration"
assert_sql "
  SELECT CONCAT(
    (SELECT COUNT(*) FROM player_mail WHERE sender='Targetchar' AND subject='sent'), '|',
    (SELECT COUNT(*) FROM player_mail WHERE receiver='Targetchar' AND subject='received'), '|',
    (SELECT COUNT(*) FROM player_mail_deleted d
       JOIN player_mail m ON m.mail_id=d.mail_id
       WHERE d.player_name='Targetchar' AND m.subject='received'), '|',
    (SELECT COUNT(*) FROM player_mail_read r
       JOIN player_mail m ON m.mail_id=r.mail_id
       WHERE r.player_name='Targetchar' AND m.subject='received')
  );
" "1|1|1|1"

sql -e "
  INSERT INTO player_data (name) VALUES ('Rollbackchar');
  INSERT INTO player_save_objs (name, serialized_obj)
    VALUES ('Rollbackchar', 'rollback payload');
  START TRANSACTION;
  UPDATE player_data SET name='Shouldnotpersist' WHERE name='Rollbackchar';
  UPDATE player_save_objs SET name='Shouldnotpersist' WHERE name='Rollbackchar';
  ROLLBACK;
"
assert_sql "
  SELECT CONCAT(
    (SELECT COUNT(*) FROM player_data WHERE name='Rollbackchar'), '|',
    (SELECT COUNT(*) FROM player_save_objs WHERE name='Rollbackchar'), '|',
    (SELECT COUNT(*) FROM player_data WHERE name='Shouldnotpersist'), '|',
    (SELECT COUNT(*) FROM player_save_objs WHERE name='Shouldnotpersist')
  );
" "1|1|0|0"

# A forced error after updates at several points must leave every transactional
# table on the old identity when the failed connection closes.
sql -e "
  INSERT INTO player_data (name) VALUES ('Injectedchar');
  INSERT INTO player_save_objs (name, serialized_obj)
    VALUES ('Injectedchar', 'injected object payload');
  INSERT INTO player_mail (sender, receiver, subject, message)
    VALUES ('Injectedchar', 'Otherchar', 'injected mail', 'injected mail payload');
  INSERT INTO player_levels (char_name, event_payload)
    VALUES ('Injectedchar', 'injected final-table payload');
"
if sql -e "
  START TRANSACTION;
  UPDATE player_data SET name='Leakedchar' WHERE name='Injectedchar';
  UPDATE player_save_objs SET name='Leakedchar' WHERE name='Injectedchar';
  UPDATE player_mail SET sender='Leakedchar' WHERE sender='Injectedchar';
  UPDATE player_levels SET char_name='Leakedchar' WHERE char_name='Injectedchar';
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='forced rename failure';
  COMMIT;
" >/dev/null 2>&1; then
  fail "forced SQL failure unexpectedly succeeded"
fi
assert_sql "
  SELECT CONCAT(
    (SELECT COUNT(*) FROM player_data WHERE name='Injectedchar'), '|',
    (SELECT COUNT(*) FROM player_save_objs WHERE name='Injectedchar'), '|',
    (SELECT COUNT(*) FROM player_mail WHERE sender='Injectedchar'), '|',
    (SELECT COUNT(*) FROM player_levels WHERE char_name='Injectedchar'), '|',
    (SELECT COUNT(*) FROM player_data WHERE name='Leakedchar') +
      (SELECT COUNT(*) FROM player_save_objs WHERE name='Leakedchar') +
      (SELECT COUNT(*) FROM player_mail WHERE sender='Leakedchar') +
      (SELECT COUNT(*) FROM player_levels WHERE char_name='Leakedchar')
  );
" "1|1|1|1|0"

echo "character rename schema test: PASS"
