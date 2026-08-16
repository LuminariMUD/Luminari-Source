#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/mysql.h"
#include "../../src/db_init.h"
#include "../../src/mudlim.h"
#include "../../src/magic/spells.h"

#include <stdlib.h>
#include <string.h>

extern MYSQL *conn;
extern bool mysql_available;
extern char *serialize_pet_runtime_state_for_test(struct char_data *pet);
extern bool restore_pet_runtime_state_for_test(struct char_data *pet, const char *serialized);
extern char *build_pet_keyword_list_for_test(const char *saved_keywords,
                                             const char *prototype_keywords);
extern bool save_char_pets(struct char_data *ch);

static int query_single_int(MYSQL *connection, const char *query, int fallback);

void Test_restored_pet_keeps_prototype_target_keywords(CuTest *tc)
{
  char *keywords;

  keywords = build_pet_keyword_list_for_test("Bones", "mummy undead monster summoned figure");
  CuAssertPtrNotNull(tc, keywords);
  CuAssertTrue(tc, isname("Bones", keywords));
  CuAssertTrue(tc, isname("mummy", keywords));
  CuAssertTrue(tc, isname("undead", keywords));
  free(keywords);
}

struct pet_save_fixture
{
  struct char_data owner;
  struct player_special_data owner_specials;
  struct char_data first_pet;
  struct char_data second_pet;
  struct descriptor_data descriptor;
  struct follow_type first_follower;
  struct follow_type second_follower;
  struct obj_data equipped_object;
  struct obj_data inventory_object;
  struct obj_data contained_object;
  struct affected_type timed_affect;
};

static void free_test_affects(struct char_data *ch)
{
  struct affected_type *affect;

  while (ch->affected)
  {
    affect = ch->affected;
    ch->affected = affect->next;
    free(affect);
  }
}

static MYSQL *open_test_database(void)
{
  const char *host;
  const char *user;
  const char *password;
  const char *database;
  const char *port_text;
  MYSQL *connection;
  unsigned int port;

  host = getenv("LUMINARI_TEST_MYSQL_HOST");
  user = getenv("LUMINARI_TEST_MYSQL_USER");
  password = getenv("LUMINARI_TEST_MYSQL_PASSWORD");
  database = getenv("LUMINARI_TEST_MYSQL_DATABASE");
  port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  port = port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306;

  if (host == NULL || user == NULL || password == NULL || database == NULL)
    return NULL;

  connection = mysql_init(NULL);
  if (connection == NULL)
    return NULL;

  if (mysql_real_connect(connection, host, user, password, database, port, NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }

  return connection;
}

static bool create_legacy_pet_temporary_schema(MYSQL *connection)
{
  const char *queries[] = {"CREATE TEMPORARY TABLE schema_migrations ("
                           "version INT NOT NULL PRIMARY KEY, description VARCHAR(255) NOT NULL, "
                           "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB",
                           "CREATE TEMPORARY TABLE pet_data ("
                           "pet_data_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
                           "owner_name VARCHAR(50) NOT NULL, cha INT NOT NULL) ENGINE=InnoDB",
                           "CREATE TEMPORARY TABLE pet_save_objs ("
                           "idnum INT UNSIGNED AUTO_INCREMENT, owner_name VARCHAR(50) NOT NULL, "
                           "pet_idnum INT NOT NULL, serialized_obj TEXT NOT NULL, "
                           "UNIQUE KEY IDNUM (idnum)) ENGINE=InnoDB",
                           "INSERT INTO pet_data (owner_name, cha) VALUES ('LegacyOwner', 17)",
                           "INSERT INTO pet_save_objs (owner_name, pet_idnum, serialized_obj) "
                           "VALUES ('LegacyOwner', 1, '#1234')",
                           NULL};
  int index;

  for (index = 0; queries[index] != NULL; index++)
    if (mysql_query(connection, queries[index]))
      return false;

  return true;
}

static bool create_invalid_pet_temporary_schema(MYSQL *connection)
{
  const char *queries[] = {
      "CREATE TEMPORARY TABLE schema_migrations ("
      "version INT NOT NULL PRIMARY KEY, description VARCHAR(255) NOT NULL, "
      "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE pet_data ("
      "pet_data_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
      "owner_name VARCHAR(50) NOT NULL, runtime_state VARCHAR(32) NOT NULL) ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE pet_save_objs ("
      "idnum INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, owner_name VARCHAR(50) NOT NULL, "
      "pet_idnum INT NOT NULL, serialized_obj TEXT NOT NULL) ENGINE=InnoDB",
      "INSERT INTO schema_migrations (version, description) "
      "VALUES (2026080504, 'invalid contract test fixture')",
      NULL};
  int index;

  for (index = 0; queries[index] != NULL; index++)
    if (mysql_query(connection, queries[index]))
      return false;

  return true;
}

static bool create_pet_snapshot_temporary_schema(MYSQL *connection)
{
  const char *queries[] = {
      "CREATE TEMPORARY TABLE pet_data ("
      "pet_data_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
      "owner_name VARCHAR(50) NOT NULL, pet_name VARCHAR(50), pet_sdesc VARCHAR(255), "
      "pet_ldesc TEXT, pet_ddesc TEXT, vnum INT NOT NULL, level INT NOT NULL, "
      "hp INT NOT NULL, max_hp INT NOT NULL, str INT NOT NULL, con INT NOT NULL, "
      "dex INT NOT NULL, ac INT NOT NULL, intel INT NOT NULL, wis INT NOT NULL, "
      "cha INT NOT NULL, runtime_state LONGTEXT) ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE pet_save_objs ("
      "idnum INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, pet_idnum BIGINT NOT NULL, "
      "owner_name VARCHAR(50) NOT NULL, serialized_obj TEXT NOT NULL) ENGINE=InnoDB",
      NULL};
  int index;

  for (index = 0; queries[index] != NULL; index++)
    if (mysql_query(connection, queries[index]))
      return false;

  return true;
}

static bool reset_old_pet_snapshot(MYSQL *connection)
{
  const char *queries[] = {
      "DELETE FROM pet_save_objs", "DELETE FROM pet_data",
      "INSERT INTO pet_data "
      "(pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, "
      "hp, max_hp, str, con, dex, ac, intel, wis, cha, runtime_state) VALUES "
      "(700, 'SnapshotOwner', 'OldPet', 'old pet', 'old pet is here', 'old description', "
      "1, 1, 10, 10, 10, 10, 10, 10, 10, 10, 10, 'old-runtime')",
      "INSERT INTO pet_save_objs (pet_idnum, owner_name, serialized_obj) "
      "VALUES (700, 'SnapshotOwner', '#old-object')",
      NULL};
  int index;

  for (index = 0; queries[index] != NULL; index++)
    if (mysql_query(connection, queries[index]))
      return false;

  return true;
}

static void initialize_pet_save_fixture(struct pet_save_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));
  clear_char(&fixture->owner);
  clear_char(&fixture->first_pet);
  clear_char(&fixture->second_pet);
  clear_object(&fixture->equipped_object);
  clear_object(&fixture->inventory_object);
  clear_object(&fixture->contained_object);

  fixture->owner.player.name = (char *)"SnapshotOwner";
  fixture->owner.player_specials = &fixture->owner_specials;
  fixture->owner.desc = &fixture->descriptor;
  fixture->descriptor.character = &fixture->owner;
  STATE(&fixture->descriptor) = CON_PLAYING;
  fixture->owner.followers = &fixture->first_follower;
  fixture->first_follower.follower = &fixture->first_pet;
  fixture->first_follower.next = &fixture->second_follower;
  fixture->second_follower.follower = &fixture->second_pet;

  SET_BIT_AR(MOB_FLAGS(&fixture->first_pet), MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(&fixture->first_pet), AFF_CHARM);
  fixture->first_pet.player.name = (char *)"FirstPet's marker";
  fixture->first_pet.player.short_descr = (char *)"the first pet's saved form";
  fixture->first_pet.player.long_descr = (char *)"The first pet's saved form is here.";
  fixture->first_pet.player.description = (char *)"A transaction pet's description.";
  GET_LEVEL(&fixture->first_pet) = 8;
  GET_HIT(&fixture->first_pet) = 71;
  GET_REAL_MAX_HIT(&fixture->first_pet) = 90;
  GET_REAL_STR(&fixture->first_pet) = 15;
  GET_REAL_CON(&fixture->first_pet) = 14;
  GET_REAL_DEX(&fixture->first_pet) = 13;
  GET_REAL_INT(&fixture->first_pet) = 5;
  GET_REAL_WIS(&fixture->first_pet) = 12;
  GET_REAL_CHA(&fixture->first_pet) = 7;
  new_affect(&fixture->timed_affect);
  fixture->timed_affect.spell = SPELL_CHARM_MONSTER;
  fixture->timed_affect.duration = 12;
  SET_BIT_AR(fixture->timed_affect.bitvector, AFF_CHARM);
  fixture->first_pet.affected = &fixture->timed_affect;

  SET_BIT_AR(MOB_FLAGS(&fixture->second_pet), MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(&fixture->second_pet), AFF_CHARM);
  fixture->second_pet.player.name = (char *)"SecondPet's marker";
  fixture->second_pet.player.short_descr = (char *)"the second pet's saved form";
  fixture->second_pet.player.long_descr = (char *)"The second pet's saved form is here.";
  fixture->second_pet.player.description = (char *)"Another transaction pet's description.";
  GET_LEVEL(&fixture->second_pet) = 9;
  GET_HIT(&fixture->second_pet) = 81;
  GET_REAL_MAX_HIT(&fixture->second_pet) = 100;
  GET_REAL_STR(&fixture->second_pet) = 16;
  GET_REAL_CON(&fixture->second_pet) = 15;
  GET_REAL_DEX(&fixture->second_pet) = 14;
  GET_REAL_INT(&fixture->second_pet) = 6;
  GET_REAL_WIS(&fixture->second_pet) = 13;
  GET_REAL_CHA(&fixture->second_pet) = 8;

  fixture->equipped_object.name = (char *)"pet's test collar";
  fixture->equipped_object.short_description = (char *)"a pet's test collar";
  fixture->equipped_object.description = (char *)"A pet's test collar lies here.";
  fixture->first_pet.equipment[0] = &fixture->equipped_object;

  fixture->inventory_object.name = (char *)"pet's carried token";
  fixture->inventory_object.short_description = (char *)"a pet's carried token";
  fixture->inventory_object.description = (char *)"A pet's carried token lies here.";
  fixture->inventory_object.carried_by = &fixture->second_pet;
  fixture->second_pet.carrying = &fixture->inventory_object;

  fixture->contained_object.name = (char *)"pet's nested token";
  fixture->contained_object.short_description = (char *)"a pet's nested token";
  fixture->contained_object.description = (char *)"A pet's nested token lies here.";
  fixture->contained_object.in_obj = &fixture->inventory_object;
  fixture->inventory_object.contains = &fixture->contained_object;
}

static bool old_pet_snapshot_is_intact(MYSQL *connection)
{
  return query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 1 &&
         query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 1 &&
         query_single_int(connection,
                          "SELECT COUNT(*) FROM pet_data AS pet JOIN pet_save_objs AS object "
                          "ON object.pet_idnum = pet.pet_data_id "
                          "WHERE pet.pet_data_id = 700 AND pet.owner_name = 'SnapshotOwner' "
                          "AND pet.pet_name = 'OldPet' AND pet.runtime_state = 'old-runtime' "
                          "AND object.serialized_obj = '#old-object'",
                          -1) == 1;
}

static int query_single_int(MYSQL *connection, const char *query, int fallback)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int value;

  if (mysql_query(connection, query))
    return fallback;
  result = mysql_store_result(connection);
  if (!result)
    return fallback;
  row = mysql_fetch_row(result);
  value = row && row[0] ? atoi(row[0]) : fallback;
  mysql_free_result(result);
  return value;
}

void Test_database_production_invalid_prepared_statement_inputs(CuTest *tc)
{
  CuAssertPtrEquals(tc, NULL, mysql_stmt_create(NULL));
  CuAssertTrue(tc, !mysql_stmt_prepare_query(NULL, "SELECT 1"));
  CuAssertTrue(tc, !mysql_stmt_bind_param_string(NULL, 0, "value"));
  CuAssertTrue(tc, !mysql_stmt_bind_param_int(NULL, 0, 1));
  CuAssertTrue(tc, !mysql_stmt_execute_prepared(NULL));
  CuAssertTrue(tc, !mysql_stmt_fetch_row(NULL));
}

void Test_database_production_prepared_statement_round_trip(CuTest *tc)
{
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  PREPARED_STMT *insert_stmt;
  PREPARED_STMT *select_stmt;
  bool saved_available;
  bool created;
  bool inserted;
  bool selected;
  bool fetched;
  bool payload_matches;
  int stored_count;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  insert_stmt = NULL;
  select_stmt = NULL;
  created = mysql_query_safe(
                connection,
                "CREATE TEMPORARY TABLE luminari_cutest_persistence ("
                "id INT PRIMARY KEY, payload VARCHAR(128) NOT NULL, item_count INT NOT NULL)") == 0;

  if (created)
  {
    insert_stmt = mysql_stmt_create(connection);
    inserted =
        insert_stmt != NULL &&
        mysql_stmt_prepare_query(
            insert_stmt, "INSERT INTO luminari_cutest_persistence (id, payload, item_count) VALUES "
                         "(?, ?, ?)") &&
        mysql_stmt_bind_param_int(insert_stmt, 0, 17) &&
        mysql_stmt_bind_param_string(insert_stmt, 1, "O'Brien coverage payload") &&
        mysql_stmt_bind_param_int(insert_stmt, 2, 42) && mysql_stmt_execute_prepared(insert_stmt);
  }
  else
  {
    inserted = false;
  }

  if (inserted)
  {
    select_stmt = mysql_stmt_create(connection);
    selected = select_stmt != NULL &&
               mysql_stmt_prepare_query(
                   select_stmt,
                   "SELECT payload, item_count FROM luminari_cutest_persistence WHERE id = ?") &&
               mysql_stmt_bind_param_int(select_stmt, 0, 17) &&
               mysql_stmt_execute_prepared(select_stmt);
  }
  else
  {
    selected = false;
  }

  fetched = selected && mysql_stmt_fetch_row(select_stmt);
  payload_matches = fetched && mysql_stmt_get_string(select_stmt, 0) != NULL &&
                    strcmp(mysql_stmt_get_string(select_stmt, 0), "O'Brien coverage payload") == 0;
  stored_count = fetched ? mysql_stmt_get_int(select_stmt, 1) : -1;

  mysql_stmt_cleanup(select_stmt);
  mysql_stmt_cleanup(insert_stmt);
  mysql_close(connection);
  conn = saved_conn;
  mysql_available = saved_available;

  CuAssertTrue(tc, created);
  CuAssertTrue(tc, inserted);
  CuAssertTrue(tc, selected);
  CuAssertTrue(tc, fetched);
  CuAssertTrue(tc, payload_matches);
  CuAssertIntEquals(tc, 42, stored_count);
}

void Test_pet_persistence_legacy_schema_migration_is_idempotent(CuTest *tc)
{
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool fixture_created;
  bool first_migration;
  bool first_verification;
  bool second_migration;
  bool second_verification;
  int first_migration_count;
  int second_migration_count;
  int pet_rows;
  int object_rows;
  int linked_rows;
  int runtime_state_null_rows;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  fixture_created = create_legacy_pet_temporary_schema(connection);
  first_migration = fixture_created && run_pet_persistence_migrations();
  first_verification = first_migration && verify_pet_persistence_schema();
  first_migration_count =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM schema_migrations WHERE version BETWEEN 2026080501 "
                       "AND 2026080504",
                       -1);
  pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);
  linked_rows =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data AS pet JOIN pet_save_objs AS object "
                       "ON object.pet_idnum = pet.pet_data_id "
                       "WHERE pet.owner_name = 'LegacyOwner' AND object.owner_name = 'LegacyOwner'",
                       -1);
  runtime_state_null_rows =
      query_single_int(connection, "SELECT COUNT(*) FROM pet_data WHERE runtime_state IS NULL", -1);
  second_migration = first_migration && run_pet_persistence_migrations();
  second_verification = second_migration && verify_pet_persistence_schema();
  second_migration_count =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM schema_migrations WHERE version BETWEEN 2026080501 "
                       "AND 2026080504",
                       -1);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, fixture_created);
  CuAssertTrue(tc, first_migration);
  CuAssertTrue(tc, first_verification);
  CuAssertIntEquals(tc, 4, first_migration_count);
  CuAssertIntEquals(tc, 1, pet_rows);
  CuAssertIntEquals(tc, 1, object_rows);
  CuAssertIntEquals(tc, 1, linked_rows);
  CuAssertIntEquals(tc, 1, runtime_state_null_rows);
  CuAssertTrue(tc, second_migration);
  CuAssertTrue(tc, second_verification);
  CuAssertIntEquals(tc, 4, second_migration_count);
}

void Test_pet_persistence_schema_rejects_incompatible_contract(CuTest *tc)
{
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool fixture_created;
  bool verified;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  fixture_created = create_invalid_pet_temporary_schema(connection);
  verified = fixture_created && verify_pet_persistence_schema();
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, fixture_created);
  CuAssertTrue(tc, !verified);
}

void Test_pet_snapshot_save_commits_whole_owner_and_rolls_back_every_query_failure(CuTest *tc)
{
  struct pet_save_fixture fixture;
  const char *enabled;
  const char *loop_count_text;
  MYSQL *connection;
  MYSQL *saved_conn;
  struct descriptor_data *saved_descriptor_list;
  bool saved_available;
  bool schema_created;
  bool seeded;
  bool snapshot_saved;
  bool rollback_coverage_passed;
  bool overflow_rollback_passed;
  bool repeated_saves_passed;
  bool forced_save_result;
  char *oversized_object_name;
  int save_query_count;
  int pet_rows;
  int object_rows;
  int linked_rows;
  int quoted_pet_rows;
  int runtime_rows;
  int quoted_payload_rows;
  int old_rows;
  int failure_query;
  int repeat_count;
  int repeat_index;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  initialize_pet_save_fixture(&fixture);
  schema_created = create_pet_snapshot_temporary_schema(connection);
  seeded = schema_created && reset_old_pet_snapshot(connection);
  saved_descriptor_list = descriptor_list;
  descriptor_list = &fixture.descriptor;
  mysql_query_counter_reset();
  snapshot_saved = seeded && save_player_pets();
  save_query_count = (int)mysql_query_counter_value();
  descriptor_list = saved_descriptor_list;
  pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);
  linked_rows =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data AS pet JOIN pet_save_objs AS object "
                       "ON object.pet_idnum = pet.pet_data_id "
                       "WHERE pet.owner_name = 'SnapshotOwner' "
                       "AND object.owner_name = 'SnapshotOwner'",
                       -1);
  runtime_rows =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data WHERE owner_name = 'SnapshotOwner' "
                       "AND runtime_state IS NOT NULL AND runtime_state <> ''",
                       -1);
  quoted_pet_rows =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data WHERE LOCATE(CHAR(39), pet_name) > 0 "
                       "AND LOCATE(CHAR(39), pet_sdesc) > 0 AND LOCATE(CHAR(39), pet_ldesc) > 0 "
                       "AND LOCATE(CHAR(39), pet_ddesc) > 0",
                       -1);
  quoted_payload_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_save_objs WHERE LOCATE(CHAR(39), serialized_obj) > 0",
      -1);
  old_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_data WHERE pet_data_id = 700 OR pet_name = 'OldPet'",
      -1);

  rollback_coverage_passed = snapshot_saved && save_query_count == 9;
  for (failure_query = 1; rollback_coverage_passed && failure_query <= save_query_count;
       failure_query++)
  {
    if (!reset_old_pet_snapshot(connection))
    {
      rollback_coverage_passed = false;
      break;
    }
    mysql_query_counter_reset();
    mysql_test_fail_nth_query((unsigned int)failure_query);
    forced_save_result = save_char_pets(&fixture.owner);
    mysql_test_clear_query_failure();
    if (forced_save_result || !old_pet_snapshot_is_intact(connection))
      rollback_coverage_passed = false;
  }

  oversized_object_name = malloc(40000);
  overflow_rollback_passed = oversized_object_name != NULL;
  if (overflow_rollback_passed)
  {
    memset(oversized_object_name, 'x', 39999);
    oversized_object_name[39999] = '\0';
    overflow_rollback_passed = reset_old_pet_snapshot(connection);
    fixture.equipped_object.name = oversized_object_name;
    forced_save_result = save_char_pets(&fixture.owner);
    overflow_rollback_passed =
        overflow_rollback_passed && !forced_save_result && old_pet_snapshot_is_intact(connection);
    fixture.equipped_object.name = (char *)"pet's test collar";
    free(oversized_object_name);
  }

  loop_count_text = getenv("LUMINARI_TEST_PET_SAVE_LOOPS");
  repeat_count = loop_count_text ? atoi(loop_count_text) : 3;
  if (repeat_count < 1)
    repeat_count = 1;
  if (repeat_count > 10000)
    repeat_count = 10000;
  repeated_saves_passed = true;
  for (repeat_index = 0; repeated_saves_passed && repeat_index < repeat_count; repeat_index++)
  {
    fixture.timed_affect.duration = 120 - (repeat_index % 100);
    repeated_saves_passed = save_char_pets(&fixture.owner);
  }
  repeated_saves_passed =
      repeated_saves_passed &&
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data AS pet JOIN pet_save_objs AS object "
                       "ON object.pet_idnum = pet.pet_data_id "
                       "WHERE pet.owner_name = 'SnapshotOwner' "
                       "AND object.owner_name = 'SnapshotOwner'",
                       -1) == 3;

  mysql_test_clear_query_failure();
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, seeded);
  CuAssertTrue(tc, snapshot_saved);
  CuAssertIntEquals(tc, 9, save_query_count);
  CuAssertIntEquals(tc, 2, pet_rows);
  CuAssertIntEquals(tc, 3, object_rows);
  CuAssertIntEquals(tc, 3, linked_rows);
  CuAssertIntEquals(tc, 2, runtime_rows);
  CuAssertIntEquals(tc, 2, quoted_pet_rows);
  CuAssertIntEquals(tc, 3, quoted_payload_rows);
  CuAssertIntEquals(tc, 0, old_rows);
  CuAssertTrue(tc, rollback_coverage_passed);
  CuAssertTrue(tc, overflow_rollback_passed);
  CuAssertTrue(tc, repeated_saves_passed);
}

void Test_pet_snapshot_lifecycle_handles_disconnect_and_follower_removal(CuTest *tc)
{
  struct pet_save_fixture fixture;
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  struct descriptor_data *saved_descriptor_list;
  bool saved_available;
  bool schema_created;
  bool initial_saved;
  bool disconnected_skipped;
  bool detached_saved;
  bool followers_removed;
  int initial_save_queries;
  int disconnected_save_queries;
  int detached_save_queries;
  int removal_save_queries;
  int initial_pet_rows;
  int initial_object_rows;
  int final_pet_rows;
  int final_object_rows;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }

  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  saved_descriptor_list = descriptor_list;
  conn = connection;
  mysql_available = true;
  initialize_pet_save_fixture(&fixture);
  descriptor_list = &fixture.descriptor;

  schema_created = create_pet_snapshot_temporary_schema(connection);
  mysql_query_counter_reset();
  initial_saved = schema_created && save_player_pets();
  initial_save_queries = (int)mysql_query_counter_value();
  initial_pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  initial_object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);

  STATE(&fixture.descriptor) = CON_DISCONNECT;
  mysql_query_counter_reset();
  disconnected_skipped = save_player_pets();
  disconnected_save_queries = (int)mysql_query_counter_value();
  disconnected_skipped =
      disconnected_skipped && disconnected_save_queries == 0 &&
      query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 2 &&
      query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 3;

  fixture.owner.desc = NULL;
  fixture.descriptor.character = NULL;
  mysql_query_counter_reset();
  detached_saved = save_char_pets(&fixture.owner);
  detached_save_queries = (int)mysql_query_counter_value();
  detached_saved = detached_saved && detached_save_queries == 9 &&
                   query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 2 &&
                   query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 3;

  fixture.owner.followers = NULL;
  mysql_query_counter_reset();
  followers_removed = save_char_pets(&fixture.owner);
  removal_save_queries = (int)mysql_query_counter_value();
  final_pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  final_object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);

  mysql_test_clear_query_failure();
  descriptor_list = saved_descriptor_list;
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, initial_saved);
  CuAssertIntEquals(tc, 9, initial_save_queries);
  CuAssertIntEquals(tc, 2, initial_pet_rows);
  CuAssertIntEquals(tc, 3, initial_object_rows);
  CuAssertTrue(tc, disconnected_skipped);
  CuAssertIntEquals(tc, 0, disconnected_save_queries);
  CuAssertTrue(tc, detached_saved);
  CuAssertIntEquals(tc, 9, detached_save_queries);
  CuAssertTrue(tc, followers_removed);
  CuAssertIntEquals(tc, 4, removal_save_queries);
  CuAssertIntEquals(tc, 0, final_pet_rows);
  CuAssertIntEquals(tc, 0, final_object_rows);
}

void Test_follower_runtime_state_round_trip(CuTest *tc)
{
  struct affected_type charm;
  struct affected_type *restored_affect;
  struct char_data source;
  struct char_data restored;
  char *serialized;
  bool restored_ok;
  bool found_charm;
  int restored_charm_duration;
  int restored_charm_specific;

  clear_char(&source);
  clear_char(&restored);
  SET_BIT_AR(MOB_FLAGS(&source), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&source), MOB_GOLEM);
  SET_BIT_AR(MOB_FLAGS(&source), MOB_MERCENARY);
  SET_BIT_AR(AFF_FLAGS(&source), AFF_WATERWALK);
  SET_BIT_AR(AFF_FLAGS(&source), AFF_FLYING);
  GET_REAL_RACE(&source) = RACE_TYPE_CONSTRUCT;
  GET_REAL_SIZE(&source) = SIZE_LARGE;
  GET_MOVE(&source) = 321;
  GET_REAL_MAX_MOVE(&source) = 654;
  GET_PSP(&source) = 45;
  GET_REAL_MAX_PSP(&source) = 80;
  GET_REAL_HITROLL(&source) = 7;
  GET_REAL_DAMROLL(&source) = 9;
  source.mob_specials.damnodice = 3;
  source.mob_specials.damsizedice = 8;
  GET_EXP(&source) = 9876;
  GET_ALIGNMENT(&source) = -420;
  GET_REAL_SAVE(&source, SAVING_WILL) = 11;
  source.mob_specials.spell_slots[3] = 2;
  source.mob_specials.max_spell_slots[3] = 4;
  MOB_SET_FEAT(&source, FEAT_IRON_WILL, 1);
  PROC_FIRED(&source) = TRUE;

  new_affect(&charm);
  charm.spell = SPELL_CHARM_MONSTER;
  charm.duration = 9;
  charm.specific = 17;
  SET_BIT_AR(charm.bitvector, AFF_CHARM);
  affect_to_char(&source, &charm);

  serialized = serialize_pet_runtime_state_for_test(&source);
  if (!serialized)
  {
    free_test_affects(&source);
    CuFail(tc, "could not serialize follower runtime state");
    return;
  }

  SET_BIT_AR(MOB_FLAGS(&restored), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&restored), MOB_SENTINEL);
  GET_REAL_RACE(&restored) = RACE_TYPE_ANIMAL;
  GET_REAL_SIZE(&restored) = SIZE_MEDIUM;
  restored_ok = restore_pet_runtime_state_for_test(&restored, serialized);
  found_charm = false;
  restored_charm_duration = -1;
  restored_charm_specific = -1;
  for (restored_affect = restored.affected; restored_affect;
       restored_affect = restored_affect->next)
  {
    if (restored_affect->spell == SPELL_CHARM_MONSTER)
    {
      found_charm = true;
      restored_charm_duration = restored_affect->duration;
      restored_charm_specific = restored_affect->specific;
      break;
    }
  }

  free(serialized);
  free_test_affects(&source);
  free_test_affects(&restored);

  CuAssertTrue(tc, restored_ok);
  CuAssertTrue(tc, found_charm);
  CuAssertIntEquals(tc, 9, restored_charm_duration);
  CuAssertIntEquals(tc, 17, restored_charm_specific);
  CuAssertTrue(tc, AFF_FLAGGED(&restored, AFF_CHARM));
  CuAssertTrue(tc, AFF_FLAGGED(&restored, AFF_WATERWALK));
  CuAssertTrue(tc, AFF_FLAGGED(&restored, AFF_FLYING));
  CuAssertTrue(tc, MOB_FLAGGED(&restored, MOB_SENTINEL));
  CuAssertTrue(tc, MOB_FLAGGED(&restored, MOB_GOLEM));
  CuAssertTrue(tc, MOB_FLAGGED(&restored, MOB_MERCENARY));
  CuAssertTrue(tc, PROC_FIRED(&restored));
  CuAssertIntEquals(tc, RACE_TYPE_CONSTRUCT, GET_REAL_RACE(&restored));
  CuAssertIntEquals(tc, SIZE_LARGE, GET_REAL_SIZE(&restored));
  CuAssertIntEquals(tc, 321, GET_MOVE(&restored));
  CuAssertIntEquals(tc, 654, GET_REAL_MAX_MOVE(&restored));
  CuAssertIntEquals(tc, 45, GET_PSP(&restored));
  CuAssertIntEquals(tc, 80, GET_REAL_MAX_PSP(&restored));
  CuAssertIntEquals(tc, 7, GET_REAL_HITROLL(&restored));
  CuAssertIntEquals(tc, 9, GET_REAL_DAMROLL(&restored));
  CuAssertIntEquals(tc, 3, restored.mob_specials.damnodice);
  CuAssertIntEquals(tc, 8, restored.mob_specials.damsizedice);
  CuAssertIntEquals(tc, 0, (int)GET_EXP(&restored));
  CuAssertIntEquals(tc, -420, GET_ALIGNMENT(&restored));
  CuAssertIntEquals(tc, 11, GET_REAL_SAVE(&restored, SAVING_WILL));
  CuAssertIntEquals(tc, 2, restored.mob_specials.spell_slots[3]);
  CuAssertIntEquals(tc, 4, restored.mob_specials.max_spell_slots[3]);
  CuAssertIntEquals(tc, 1, MOB_HAS_FEAT(&restored, FEAT_IRON_WILL));
}

void Test_follower_runtime_state_rejects_incomplete_data(CuTest *tc)
{
  struct char_data follower;
  bool restored;

  clear_char(&follower);
  SET_BIT_AR(MOB_FLAGS(&follower), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&follower), MOB_SENTINEL);

  restored = restore_pet_runtime_state_for_test(&follower, "V 1\nB 0 0 0 0 0 0 0 0\nE\n");

  CuAssertTrue(tc, !restored);
  CuAssertTrue(tc, MOB_FLAGGED(&follower, MOB_SENTINEL));
  CuAssertTrue(tc, !AFF_FLAGGED(&follower, AFF_CHARM));
  CuAssertPtrEquals(tc, NULL, follower.affected);
}

void Test_crash_save_single_and_incremental(CuTest *tc)
{
  struct descriptor_data desc;
  struct char_data ch;
  struct player_special_data specials;
  int saved;

  clear_char(&ch);
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  GET_PFILEPOS(&ch) = -1; /* Don't overwrite actual disk file in unit test */
  ch.player.name = strdup("Testsaver");

  /* Test Crash_save_single with NPC/NULL */
  CuAssertIntEquals(tc, 0, Crash_save_single(NULL, NULL, NULL));
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  CuAssertIntEquals(tc, 0, Crash_save_single(&ch, NULL, NULL));
  REMOVE_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);

  /* Set PLR_CRASH */
  SET_BIT_AR(PLR_FLAGS(&ch), PLR_CRASH);
  CuAssertTrue(tc, PLR_FLAGGED(&ch, PLR_CRASH));

  memset(&desc, 0, sizeof(desc));
  desc.connected = CON_PLAYING;
  desc.character = &ch;
  ch.desc = &desc;

  /* Insert in descriptor list for testing incremental save */
  desc.next = descriptor_list;
  descriptor_list = &desc;

  saved = Crash_save_incremental(1);
  CuAssertIntEquals(tc, 1, saved);
  CuAssertTrue(tc, !PLR_FLAGGED(&ch, PLR_CRASH));

  /* Remove descriptor from descriptor_list */
  descriptor_list = desc.next;

  free(ch.player.name);
}
