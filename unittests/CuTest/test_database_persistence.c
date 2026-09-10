#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/account.h"
#include "../../src/act.h"
#include "../../src/character/race.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/mysql.h"
#include "../../src/net/protocol.h"
#include "../../src/db_init.h"
#include "../../src/mudlim.h"
#include "../../src/magic/spells.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/event_runtime.h"
#include "../../src/periodic_owners.h"
#include "../../src/point_update_periodic.h"

#include <stdlib.h>
#include <string.h>

extern MYSQL *conn;
extern bool mysql_available;
extern char *serialize_pet_runtime_state_for_test(struct char_data *pet);
extern bool restore_pet_runtime_state_for_test(struct char_data *pet, const char *serialized);
extern char *build_pet_keyword_list_for_test(const char *saved_keywords,
                                             const char *prototype_keywords);
extern bool save_char_pets(struct char_data *ch);
extern void load_char_pets(struct char_data *ch);
extern bool pet_save_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum);
extern int objsave_save_obj_record_db_pet(struct obj_data *obj, struct char_data *pet,
                                          struct char_data *owner, long int pet_idnum, int locate);
extern void reset_pet_save_cache_for_test(void);

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

static void check_restored_object_registries(CuTest *tc, bool database)
{
  const char records[] = "#91001\nFlag: 0 0 0 0\nVals: 0 0\n"
                         "#91002\nFlag: 0 4096 0 0\nType: 14\nVals: 0 1\n";
  struct obj_data prototypes[2];
  struct index_data indexes[2] = {0};
  struct zone_data zone = {0};
  struct zone_data *saved_zones = zone_table;
  zone_rnum saved_top_zone = top_of_zone_table;
  struct obj_data *saved_proto = obj_proto;
  struct index_data *saved_index = obj_index;
  struct obj_data *saved_objects = object_list;
  obj_rnum saved_top = top_of_objt;
  unsigned long saved_pulse = pulse;
  MYSQL *saved_conn = conn;
  MYSQL *connection = NULL;
  obj_save_data *loaded;
  obj_save_data *entry;
  FILE *fixture = NULL;
  char escaped[sizeof(records) * 2 + 1];
  char query[sizeof(escaped) + 128];
  bool removed = false;
  bool added = false;
  bool valid;
  int count = 0;
  int i;

  if (database)
  {
    connection = open_test_database();
    if (connection == NULL)
    {
      CuFail(tc, "could not connect to the explicitly configured test database");
      return;
    }
    if (mysql_query(connection, "CREATE TEMPORARY TABLE house_data (vnum INT, "
                                "serialized_obj TEXT, idnum INT, creation_date INT)") != 0)
    {
      mysql_close(connection);
      CuFail(tc, "could not create isolated house object fixture");
      return;
    }
    mysql_real_escape_string(connection, escaped, records, strlen(records));
    snprintf(query, sizeof(query), "INSERT INTO house_data VALUES (900, '%s', 1, 1)", escaped);
    if (mysql_query(connection, query) != 0)
    {
      mysql_close(connection);
      CuFail(tc, "could not seed isolated house object fixture");
      return;
    }
    conn = connection;
  }
  else
  {
    fixture = tmpfile();
    if (fixture == NULL)
    {
      CuFail(tc, "could not create saved object fixture");
      return;
    }
    fputs(records, fixture);
    rewind(fixture);
  }

  event_free_all();
  periodic_owners_reset_for_test();
  autoproc_registry_reset_for_test();
  point_update_periodic_reset_for_test();
  object_list = NULL;
  for (i = 0; i < 2; i++)
  {
    clear_object(&prototypes[i]);
    prototypes[i].item_number = i;
    prototypes[i].name = (char *)"registry fixture";
    prototypes[i].short_description = (char *)"a registry fixture";
    prototypes[i].description = (char *)"A registry fixture is here.";
    indexes[i].vnum = 91001 + i;
  }
  SET_BIT_AR(GET_OBJ_EXTRA(&prototypes[0]), ITEM_AUTOPROC);
  GET_OBJ_TYPE(&prototypes[0]) = ITEM_MISSILE;
  GET_OBJ_VAL(&prototypes[0], 1) = 1;
  obj_proto = prototypes;
  obj_index = indexes;
  top_of_objt = 1;
  zone.number = 910;
  zone.bot = 91000;
  zone.top = 91099;
  zone_table = &zone;
  top_of_zone_table = 0;
  periodic_owners_select_for_test(true, false);
  point_update_periodic_select_for_test(true);
  event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER);
  pulse = 100U;
  event_init();
  periodic_owners_init();
  point_update_periodic_init();

  loaded = database ? objsave_parse_objects_db(NULL, 900) : objsave_parse_objects(fixture);
  for (entry = loaded; entry != NULL; entry = entry->next)
  {
    count++;
    if (GET_OBJ_VNUM(entry->obj) == indexes[0].vnum)
      removed = !entry->obj->autoproc_registered && !entry->obj->point_update_registered &&
                event_runtime_handle_is_none(entry->obj->autoproc_event_handle);
    if (GET_OBJ_VNUM(entry->obj) == indexes[1].vnum)
      added = entry->obj->autoproc_registered && entry->obj->point_update_registered &&
              !event_runtime_handle_is_none(entry->obj->autoproc_event_handle);
  }
  valid = autoproc_registry_validate() == 0 && point_update_object_registry_validate() == 0 &&
          autoproc_registry_count() == 1 && periodic_autoproc_scheduled_count() == 1 &&
          point_update_object_count() == 1;
  while (loaded != NULL)
  {
    entry = loaded;
    loaded = loaded->next;
    extract_obj(entry->obj);
    free(entry);
  }
  point_update_periodic_reset_for_test();
  periodic_owners_reset_for_test();
  event_free_all();
  obj_proto = saved_proto;
  obj_index = saved_index;
  top_of_objt = saved_top;
  zone_table = saved_zones;
  top_of_zone_table = saved_top_zone;
  object_list = saved_objects;
  pulse = saved_pulse;
  conn = saved_conn;
  if (connection != NULL)
    mysql_close(connection);
  if (fixture != NULL)
    fclose(fixture);

  CuAssertIntEquals(tc, 2, count);
  CuAssertTrue(tc, removed);
  CuAssertTrue(tc, added);
  CuAssertTrue(tc, valid);
}

void Test_file_object_restore_reconciles_scheduled_work(CuTest *tc)
{
  check_restored_object_registries(tc, false);
}

void Test_database_object_restore_reconciles_scheduled_work(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");

  if (enabled != NULL && strcmp(enabled, "1") == 0)
    check_restored_object_registries(tc, true);
}

static bool create_legacy_pet_temporary_schema(MYSQL *connection)
{
  const char *queries[] = {"CREATE TEMPORARY TABLE schema_migrations ("
                           "version INT NOT NULL PRIMARY KEY, description VARCHAR(255) NOT NULL, "
                           "applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB",
                           /* The pre-migration production shape: signed row
                            * identifier, nullable descriptions, unsigned-free
                            * object identifier. */
                           "CREATE TEMPORARY TABLE pet_data ("
                           "pet_data_id INT AUTO_INCREMENT PRIMARY KEY, "
                           "owner_name VARCHAR(50) NOT NULL, "
                           "pet_name VARCHAR(50) DEFAULT NULL, "
                           "pet_sdesc VARCHAR(255) DEFAULT NULL, "
                           "pet_ldesc TEXT DEFAULT NULL, pet_ddesc TEXT DEFAULT NULL, "
                           "vnum INT NOT NULL DEFAULT 1, level INT NOT NULL DEFAULT 1, "
                           "hp INT NOT NULL DEFAULT 1, max_hp INT NOT NULL DEFAULT 1, "
                           "str INT NOT NULL DEFAULT 10, con INT NOT NULL DEFAULT 10, "
                           "dex INT NOT NULL DEFAULT 10, ac INT NOT NULL DEFAULT 10, "
                           "intel INT DEFAULT 10, wis INT DEFAULT 10, cha INT DEFAULT 10"
                           ") ENGINE=InnoDB",
                           "CREATE TEMPORARY TABLE pet_save_objs ("
                           "idnum INT UNSIGNED AUTO_INCREMENT, owner_name VARCHAR(50) NOT NULL, "
                           "pet_idnum INT NOT NULL, serialized_obj TEXT NOT NULL, "
                           "UNIQUE KEY IDNUM (idnum)) ENGINE=InnoDB",
                           "INSERT INTO pet_data (owner_name, wis, cha) "
                           "VALUES ('LegacyOwner', NULL, 17)",
                           "INSERT INTO pet_save_objs (owner_name, pet_idnum, serialized_obj) "
                           "VALUES ('LegacyOwner', 1, '#1234'), ('LegacyOwner', 9, '#orphan'), "
                           "('LegacyOwner', 0, '#unbound')",
                           /* InnoDB refuses foreign keys on temporary tables, so
                            * the cascade migration is recorded as applied here.
                            * The booted suite applies it for real: the base
                            * tables carry no constraint, so every fresh test
                            * database runs 2026091007 at startup, and
                            * Test_pet_live_schema_cascades_objects_and_rejects_orphans
                            * then checks its outcome on those live tables. */
                           "INSERT INTO schema_migrations (version, description) "
                           "VALUES (2026091007, 'temporary fixture cannot carry a foreign key')",
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
      "cha INT NOT NULL, runtime_state LONGTEXT, owner_id INT UNSIGNED NOT NULL DEFAULT 0, "
      "owner_created BIGINT NOT NULL DEFAULT 0, pet_state TINYINT NOT NULL DEFAULT 0"
      ") ENGINE=InnoDB",
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
  reset_pet_save_cache_for_test();
  memset(fixture, 0, sizeof(*fixture));
  clear_char(&fixture->owner);
  fixture->owner.pet_roster_load_state = PET_ROSTER_LOADED;
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

void Test_race_equivalence_account_unlock_database_round_trip(CuTest *tc)
{
  const int races[] = {RACE_WEMIC, RACE_HALF_OGRE, RACE_HALF_ILLITHID, RACE_YUAN_TI, RACE_MYCONID};
  const char *race_names[] = {"Wemic", "Half-Ogre", "Half-Illithid", "Yuan-Ti", "Myconid"};
  const char *queries[] = {
      "CREATE TEMPORARY TABLE account_data ("
      "id INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(64) NOT NULL, "
      "password VARCHAR(64) NOT NULL, experience INT NOT NULL, email VARCHAR(255) NULL, "
      "quit_survey_completed BOOLEAN NOT NULL) ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE unlocked_races ("
      "account_id INT NOT NULL, race_id INT NOT NULL, "
      "UNIQUE KEY unique_account_race (account_id, race_id)) ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE unlocked_classes ("
      "account_id INT NOT NULL, class_id INT NOT NULL, "
      "UNIQUE KEY unique_account_class (account_id, class_id)) ENGINE=InnoDB",
      NULL};
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data saved_account;
  struct account_data loaded_account;
  bool saved_available;
  bool schema_created = true;
  bool saved = false;
  bool found = false;
  bool protocol_created = false;
  char command[MAX_INPUT_LENGTH];
  char count_query[256];
  int stored_count = -1;
  int query_index = 0;
  int race_index = 0;
  int slot = 0;

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
  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&saved_account, 0, sizeof(saved_account));
  memset(&loaded_account, 0, sizeof(loaded_account));
  ch.player_specials = &specials;
  ch.desc = &descriptor;
  descriptor.character = &ch;
  descriptor.account = &saved_account;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  protocol_created = descriptor.pProtocol != NULL;
  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();

  for (query_index = 0; queries[query_index] != NULL; query_index++)
    if (mysql_query(connection, queries[query_index]))
    {
      schema_created = false;
      break;
    }

  if (schema_created && protocol_created)
  {
    saved_account.name = (char *)"RaceEquivalenceAccount";
    strlcpy(saved_account.password, "test-password", sizeof(saved_account.password));
    saved_account.experience = 63000;
    for (race_index = 0; race_index < (int)(sizeof(races) / sizeof(races[0])); race_index++)
    {
      snprintf(command, sizeof(command), "race %s", race_names[race_index]);
      do_accexp(&ch, command, 0, 0);
    }
    saved = saved_account.id > 0 && saved_account.experience == 0;
  }

  if (saved)
  {
    loaded_account.id = saved_account.id;
    load_account_unlocks(&loaded_account);
    snprintf(count_query, sizeof(count_query),
             "SELECT COUNT(*) FROM unlocked_races WHERE account_id = %d", saved_account.id);
    stored_count = query_single_int(connection, count_query, -1);
  }

  if (descriptor.pProtocol != NULL)
  {
    ProtocolDestroy(descriptor.pProtocol);
    descriptor.pProtocol = NULL;
  }
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  mysql_close(connection);
  conn = saved_conn;
  mysql_available = saved_available;

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, protocol_created);
  CuAssertTrue(tc, saved);
  CuAssertTrue(tc, saved_account.id > 0);
  CuAssertIntEquals(tc, (int)(sizeof(races) / sizeof(races[0])), stored_count);
  for (race_index = 0; race_index < (int)(sizeof(races) / sizeof(races[0])); race_index++)
  {
    found = false;
    for (slot = 0; slot < MAX_UNLOCKED_RACES; slot++)
      if (loaded_account.races[slot] == races[race_index])
      {
        found = true;
        break;
      }
    CuAssertTrue(tc, found);
  }
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
  int max_owner_id_rows;
  int filled_description_rows;

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
                       "AND 2026091007",
                       -1);
  max_owner_id_rows =
      mysql_query(connection, "UPDATE pet_data SET owner_id = 4294967295") == 0
          ? query_single_int(connection,
                             "SELECT COUNT(*) FROM pet_data WHERE owner_id = 4294967295", -1)
          : -1;
  pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  filled_description_rows =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data WHERE pet_name = '' AND pet_sdesc = '' "
                       "AND pet_ldesc = '' AND pet_ddesc = '' AND wis = 10 AND cha = 17",
                       -1);
  /* The orphan and unbound object rows were purged ahead of the constraint. */
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
                       "AND 2026091007",
                       -1);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, fixture_created);
  CuAssertTrue(tc, first_migration);
  CuAssertTrue(tc, first_verification);
  CuAssertIntEquals(tc, 15, first_migration_count);
  CuAssertIntEquals(tc, 1, max_owner_id_rows);
  CuAssertIntEquals(tc, 1, pet_rows);
  CuAssertIntEquals(tc, 1, filled_description_rows);
  CuAssertIntEquals(tc, 1, object_rows);
  CuAssertIntEquals(tc, 1, linked_rows);
  CuAssertIntEquals(tc, 1, runtime_state_null_rows);
  CuAssertTrue(tc, second_migration);
  CuAssertTrue(tc, second_verification);
  CuAssertIntEquals(tc, 15, second_migration_count);
}

/* The live tables carry the constraint that temporary fixtures cannot: the
 * migration runner and validator accept the booted schema unchanged, deleting a
 * pet row removes its saved objects, and an object row cannot point at a pet
 * that does not exist.  Every write happens inside a transaction that is rolled
 * back, so the configured database is left as it was found. */
void Test_pet_live_schema_cascades_objects_and_rejects_orphans(CuTest *tc)
{
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool live_tables;
  bool migrations_idempotent;
  bool schema_verified;
  bool seeded;
  bool cascaded;
  bool orphan_rejected;
  unsigned int orphan_errno;
  int pet_id;
  int object_rows;
  char query[512];

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

  live_tables = query_single_int(connection,
                                 "SELECT COUNT(*) FROM information_schema.TABLES "
                                 "WHERE TABLE_SCHEMA = DATABASE() "
                                 "AND TABLE_NAME IN ('pet_data', 'pet_save_objs')",
                                 -1) == 2;
  if (!live_tables)
  {
    mysql_close(connection);
    CuFail(tc, "the test database must hold the live pet tables from sql/master_schema.sql");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  migrations_idempotent = run_pet_persistence_migrations();
  schema_verified = migrations_idempotent && verify_pet_persistence_schema();
  conn = saved_conn;
  mysql_available = saved_available;

  seeded = mysql_query(connection, "START TRANSACTION") == 0 &&
           mysql_query(connection,
                       "INSERT INTO pet_data (owner_name, vnum, hp, max_hp, str, con, dex, level, "
                       "ac, pet_name, pet_sdesc, pet_ldesc, pet_ddesc) VALUES "
                       "('CutestCascadeOwner', 1, 10, 10, 10, 10, 10, 1, 10, 'cascade pet', "
                       "'a cascade pet', 'A cascade pet stands here.', 'Test fixture.')") == 0;
  pet_id = seeded ? (int)mysql_insert_id(connection) : 0;
  snprintf(query, sizeof(query),
           "INSERT INTO pet_save_objs (pet_idnum, owner_name, serialized_obj) VALUES "
           "(%d, 'CutestCascadeOwner', '#-1'), (%d, 'CutestCascadeOwner', '#-1')",
           pet_id, pet_id);
  seeded = seeded && pet_id > 0 && mysql_query(connection, query) == 0;
  snprintf(query, sizeof(query), "SELECT COUNT(*) FROM pet_save_objs WHERE pet_idnum = %d", pet_id);
  object_rows = seeded ? query_single_int(connection, query, -1) : -1;

  snprintf(query, sizeof(query), "DELETE FROM pet_data WHERE pet_data_id = %d", pet_id);
  cascaded = seeded && mysql_query(connection, query) == 0;
  snprintf(query, sizeof(query), "SELECT COUNT(*) FROM pet_save_objs WHERE pet_idnum = %d", pet_id);
  cascaded = cascaded && query_single_int(connection, query, -1) == 0;

  snprintf(query, sizeof(query),
           "INSERT INTO pet_save_objs (pet_idnum, owner_name, serialized_obj) VALUES "
           "(%d, 'CutestCascadeOwner', '#-1')",
           pet_id);
  orphan_rejected = seeded && mysql_query(connection, query) != 0;
  orphan_errno = mysql_errno(connection);

  mysql_query(connection, "ROLLBACK");
  mysql_close(connection);

  CuAssertTrue(tc, migrations_idempotent);
  CuAssertTrue(tc, schema_verified);
  CuAssertTrue(tc, seeded);
  CuAssertIntEquals(tc, 2, object_rows);
  CuAssertTrue(tc, cascaded);
  CuAssertTrue(tc, orphan_rejected);
  /* ER_NO_REFERENCED_ROW_2: the foreign key refused the orphan row. */
  CuAssertIntEquals(tc, 1452, (int)orphan_errno);
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

void Test_pet_restore_failure_blocks_snapshot_replacement(CuTest *tc)
{
  struct pet_save_fixture fixture;
  MYSQL *connection, *saved_conn = conn;
  bool saved_available = mysql_available;
  bool blocked, retained, empty, failed, malformed, loaded, discarded, runtime_rejected;
  struct obj_data *saved_objects;
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;
  connection = open_test_database();
  CuAssertPtrNotNull(tc, connection);
  initialize_pet_save_fixture(&fixture);
  conn = connection;
  mysql_available = true;
  retained = create_pet_snapshot_temporary_schema(connection) && reset_old_pet_snapshot(connection);
  fixture.owner.pet_roster_load_state = PET_ROSTER_UNLOADED;
  mysql_query_counter_reset();
  blocked = !save_char_pets(&fixture.owner) && mysql_query_counter_value() == 0;
  IN_ROOM(&fixture.owner) = 0;
  mysql_test_fail_nth_query(1);
  load_char_pets(&fixture.owner);
  mysql_test_clear_query_failure();
  mysql_query_counter_reset();
  blocked = blocked && fixture.owner.pet_roster_load_state == PET_ROSTER_LOAD_FAILED &&
            !save_char_pets(&fixture.owner) && mysql_query_counter_value() == 0;
  retained = retained && old_pet_snapshot_is_intact(connection);
  mysql_query_counter_reset();
  load_char_pets(&fixture.owner);
  blocked = blocked && mysql_query_counter_value() == 0;
  retained =
      retained &&
      mysql_query(connection, "UPDATE pet_data SET runtime_state='invalid-versioned-record'") == 0;
  fixture.owner.pet_roster_load_state = PET_ROSTER_UNLOADED;
  load_char_pets(&fixture.owner);
  runtime_rejected = fixture.owner.pet_roster_load_state == PET_ROSTER_LOAD_FAILED &&
                     fixture.owner.followers == &fixture.first_follower;
  /* The save fixture uses stack-owned inventory; restore needs an empty actor. */
  fixture.second_pet.carrying = NULL;
  retained =
      retained &&
      mysql_query(
          connection,
          "ALTER TABLE pet_save_objs ADD creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP") == 0;
  /* Saved objects are addressed by pet row, so the owner name plays no part in
   * the lookup: a pet row without objects is empty whatever the owner is called. */
  fixture.owner.player.name = (char *)"No Pet's Owner";
  empty = pet_load_objs(&fixture.second_pet, &fixture.owner, 699) == PET_OBJECT_LOAD_EMPTY;
  mysql_test_fail_nth_query(1);
  failed = pet_load_objs(&fixture.second_pet, &fixture.owner, 699) == PET_OBJECT_LOAD_FAILED;
  mysql_test_clear_query_failure();
  retained =
      retained &&
      mysql_query(connection, "INSERT INTO pet_save_objs (pet_idnum,owner_name,serialized_obj) "
                              "VALUES (701,'SnapshotOwner','')") == 0;
  fixture.owner.player.name = (char *)"SnapshotOwner";
  malformed = pet_load_objs(&fixture.second_pet, &fixture.owner, 701) == PET_OBJECT_LOAD_FAILED;
  retained = retained &&
             mysql_query(connection,
                         "UPDATE pet_save_objs SET serialized_obj='#-1\\nName: restored token\\n' "
                         "WHERE pet_idnum=701") == 0;
  loaded = pet_load_objs(&fixture.second_pet, &fixture.owner, 701) == PET_OBJECT_LOAD_OK &&
           fixture.second_pet.carrying != NULL;
  while (fixture.second_pet.carrying != NULL)
    extract_obj(fixture.second_pet.carrying);
  retained =
      retained &&
      mysql_query(connection,
                  "INSERT INTO pet_save_objs (pet_idnum,owner_name,serialized_obj,creation_date) "
                  "VALUES (702,'SnapshotOwner','#-1\\nName: staged token\\n','2000-01-01'),"
                  "(702,'SnapshotOwner','','2000-01-02')") == 0;
  saved_objects = object_list;
  discarded = pet_load_objs(&fixture.second_pet, &fixture.owner, 702) == PET_OBJECT_LOAD_FAILED &&
              fixture.second_pet.carrying == NULL && object_list == saved_objects &&
              query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs WHERE pet_idnum=702",
                               -1) == 2;
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);
  CuAssertTrue(tc, blocked);
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, empty);
  CuAssertTrue(tc, failed);
  CuAssertTrue(tc, malformed);
  CuAssertTrue(tc, loaded);
  CuAssertTrue(tc, discarded);
  CuAssertTrue(tc, runtime_rejected);
}

static bool pet_test_payload(MYSQL *connection, const char *payload)
{
  char *escaped, *query;
  size_t size;
  bool success;

  if (mysql_query(connection, "DELETE FROM pet_save_objs") != 0)
    return false;
  escaped = mysql_escape_string_alloc(connection, payload);
  if (escaped == NULL)
    return false;
  size = strlen(escaped) + 160;
  query = malloc(size);
  if (query == NULL)
  {
    free(escaped);
    return false;
  }
  snprintf(query, size,
           "INSERT INTO pet_save_objs (pet_idnum,owner_name,serialized_obj) "
           "VALUES (701,'CodecOwner','%s')",
           escaped);
  success = mysql_query(connection, query) == 0;
  free(query);
  free(escaped);
  return success;
}

void Test_pet_object_decoder_validates_fields_and_preserves_text(CuTest *tc)
{
  const char *invalid[] = {"#-1\nx\n",
                           "#999999999999999999999999999999999999\n",
                           "#-1\nAff : -1 1 2 3 4\n",
                           "#-1\nAff : 2147483648 1 2 3 4\n",
                           "#-1\nAff : 0 1\n",
                           "#-1\nADes:\n",
                           "#-1\nEDes:\nkey~\n",
                           "#-1\nActv: 1 2\n",
                           "#-1\nFlag: 1 2 3\n",
                           "#-1\nFlag: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                           "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 0 0 0\n",
                           "#-1\nVals: not-a-number\n",
                           "#-1\nSpbk: 1\n",
                           "#-1\nType: 999\n",
                           "#-1\nLoc : -2147483648\n",
                           "#-1\nLoc : 999999\n",
                           "#-1\nLoc : -1\n",
                           "#-1\nLoc : -2\n#-1\nType: 15\n",
                           "#-1\nLoc : -1\n#-1\n",
                           "#-1\nLoc : 1\n#-1\nLoc : 1\n"};
  const char *valid = "#-1\nName: old token\nName: final token\n"
                      "ADes:\nfirst line\n\nlast line~\n"
                      "EDes:\nfirst key~\nfirst description\n\nend~\n"
                      "EDes:\nsecond key~\nsecond description~\n"
                      "Aff : 0 1 2 3 4\nVals: 1 2\nFlag: 0 0 0 0\n";
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  MYSQL *connection, *saved_conn = conn;
  struct char_data owner, pet;
  struct obj_data *saved_objects, *obj;
  bool rejected = true, text_preserved = true, graph_preserved;
  size_t index;
  int pass;
  char container_payload[256];

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;
  connection = open_test_database();
  CuAssertPtrNotNull(tc, connection);
  clear_char(&owner);
  clear_char(&pet);
  owner.player.name = (char *)"CodecOwner";
  SET_BIT_AR(MOB_FLAGS(&pet), MOB_ISNPC);
  conn = connection;
  rejected = create_pet_snapshot_temporary_schema(connection) &&
             mysql_query(connection, "ALTER TABLE pet_save_objs "
                                     "ADD creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP") == 0;
  for (index = 0; index < sizeof(invalid) / sizeof(invalid[0]); index++)
  {
    saved_objects = object_list;
    if (!pet_test_payload(connection, invalid[index]) ||
        pet_load_objs(&pet, &owner, 701) != PET_OBJECT_LOAD_FAILED || pet.carrying != NULL ||
        object_list != saved_objects ||
        query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) != 1)
      rejected = false;
    while (pet.carrying != NULL)
      extract_obj(pet.carrying);
  }
  text_preserved = pet_test_payload(connection, valid);
  for (pass = 0; pass < 2; pass++)
  {
    if (pet_load_objs(&pet, &owner, 701) != PET_OBJECT_LOAD_OK || (obj = pet.carrying) == NULL)
    {
      text_preserved = false;
      break;
    }
    text_preserved = text_preserved && !strcmp(obj->name, "final token") &&
                     obj->action_description != NULL &&
                     !strcmp(obj->action_description, "first line\n\nlast line") &&
                     obj->ex_description != NULL && obj->ex_description->next != NULL &&
                     !strcmp(obj->ex_description->keyword, "first key") &&
                     !strcmp(obj->ex_description->description, "first description\n\nend") &&
                     !strcmp(obj->ex_description->next->keyword, "second key");
    if (pass == 0)
      text_preserved = text_preserved &&
                       mysql_query(connection, "DELETE FROM pet_save_objs") == 0 &&
                       objsave_save_obj_record_db_pet(obj, &pet, &owner, 701, 0) == 1;
    while (pet.carrying != NULL)
      extract_obj(pet.carrying);
  }
  /* An incompatible worn container falls back to inventory with its contents. */
  snprintf(container_payload, sizeof(container_payload),
           "#-1\nName: child\nLoc : -1\n#-1\nName: bag\nType: %d\nLoc : %d\n", ITEM_CONTAINER,
           WEAR_HEAD + 1);
  graph_preserved = pet_test_payload(connection, container_payload) &&
                    pet_load_objs(&pet, &owner, 701) == PET_OBJECT_LOAD_OK &&
                    pet.carrying != NULL && pet.carrying->contains != NULL &&
                    !strcmp(pet.carrying->contains->name, "child");
  /* The recursive writer's row order remains authoritative if timestamps tie
   * or the wall clock moves backwards during a snapshot. */
  graph_preserved =
      graph_preserved && mysql_query(connection, "DELETE FROM pet_save_objs") == 0 &&
      pet_save_objs(&pet, &owner, 701) &&
      query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 2 &&
      mysql_query(connection, "UPDATE pet_save_objs SET creation_date="
                              "TIMESTAMP('2026-09-08 12:00:00') - INTERVAL idnum SECOND") == 0;
  while (pet.carrying != NULL)
    extract_obj(pet.carrying);
  graph_preserved = graph_preserved && pet_load_objs(&pet, &owner, 701) == PET_OBJECT_LOAD_OK &&
                    pet.carrying != NULL && pet.carrying->contains != NULL &&
                    !strcmp(pet.carrying->name, "bag") &&
                    !strcmp(pet.carrying->contains->name, "child");
  while (pet.carrying != NULL)
    extract_obj(pet.carrying);
  conn = saved_conn;
  mysql_close(connection);
  CuAssertTrue(tc, rejected);
  CuAssertTrue(tc, text_preserved);
  CuAssertTrue(tc, graph_preserved);
}

/* A reused character name must not hand an earlier character's saved pets to
 * the new owner: the pfile identity and its creation time bind each row. */
void Test_pet_snapshot_binds_saved_rows_to_the_pfile_owner(CuTest *tc)
{
  struct pet_save_fixture original;
  struct pet_save_fixture successor;
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool schema_created;
  bool saved_original;
  bool saved_successor;
  bool foreign_rows_skipped;
  int bound_rows;
  int retained_rows;
  int successor_rows;
  int retained_objects;

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
  schema_created = create_pet_snapshot_temporary_schema(connection);

  initialize_pet_save_fixture(&original);
  GET_IDNUM(&original.owner) = 4001;
  original.owner.player.time.birth = (time_t)1000;
  saved_original = schema_created && save_char_pets(&original.owner);
  bound_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_data WHERE owner_id = 4001 AND owner_created = 1000",
      -1);

  /* The same name and reused pfile number, created later: a different owner. */
  initialize_pet_save_fixture(&successor);
  GET_IDNUM(&successor.owner) = 4001;
  successor.owner.player.time.birth = (time_t)2000;
  saved_successor = save_char_pets(&successor.owner);
  retained_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_data WHERE owner_id = 4001 AND owner_created = 1000",
      -1);
  successor_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_data WHERE owner_id = 4001 AND owner_created = 2000",
      -1);
  retained_objects =
      query_single_int(connection,
                       "SELECT COUNT(*) FROM pet_data AS pet JOIN pet_save_objs AS object "
                       "ON object.pet_idnum = pet.pet_data_id "
                       "WHERE pet.owner_created = 1000",
                       -1);

  /* Only the earlier owner's rows remain, and restore must not adopt them. */
  foreign_rows_skipped =
      mysql_query(connection, "DELETE FROM pet_data WHERE owner_created = 2000") == 0;
  successor.owner.pet_roster_load_state = PET_ROSTER_UNLOADED;
  successor.owner.followers = NULL;
  IN_ROOM(&successor.owner) = 0;
  load_char_pets(&successor.owner);
  foreign_rows_skipped = foreign_rows_skipped &&
                         successor.owner.pet_roster_load_state == PET_ROSTER_LOADED &&
                         successor.owner.followers == NULL &&
                         query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 2;

  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, saved_original);
  CuAssertIntEquals(tc, 2, bound_rows);
  CuAssertTrue(tc, saved_successor);
  CuAssertIntEquals(tc, 2, retained_rows);
  CuAssertIntEquals(tc, 2, successor_rows);
  CuAssertIntEquals(tc, 3, retained_objects);
  CuAssertTrue(tc, foreign_rows_skipped);
}

/* A rename moves only the owner name; the pfile binding still identifies the
 * pets, and a new character taking the freed name inherits nothing. */
void Test_pet_rows_follow_a_renamed_owner_and_ignore_the_freed_name(CuTest *tc)
{
  struct pet_save_fixture renamed;
  struct pet_save_fixture newcomer;
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool schema_created;
  bool saved_before_rename;
  bool saved_after_rename;
  bool newcomer_saved;
  int renamed_rows;
  int surviving_rows;

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
  schema_created = create_pet_snapshot_temporary_schema(connection);

  initialize_pet_save_fixture(&renamed);
  renamed.owner.player.name = (char *)"OldName";
  GET_IDNUM(&renamed.owner) = 7001;
  renamed.owner.player.time.birth = (time_t)500;
  saved_before_rename = schema_created && save_char_pets(&renamed.owner);

  /* This is exactly what src/player_rename.c rewrites for the pet tables. */
  saved_after_rename =
      mysql_query(connection, "UPDATE pet_data SET owner_name = 'NewName'") == 0 &&
      mysql_query(connection, "UPDATE pet_save_objs SET owner_name = 'NewName'") == 0;
  renamed.owner.player.name = (char *)"NewName";
  renamed.timed_affect.duration = 30;
  saved_after_rename = saved_after_rename && save_char_pets(&renamed.owner);
  renamed_rows = query_single_int(
      connection, "SELECT COUNT(*) FROM pet_data WHERE owner_name = 'NewName' AND owner_id = 7001",
      -1);

  /* A different character created with the freed name owns none of them. */
  initialize_pet_save_fixture(&newcomer);
  newcomer.owner.player.name = (char *)"OldName";
  newcomer.owner.followers = NULL;
  GET_IDNUM(&newcomer.owner) = 7002;
  newcomer.owner.player.time.birth = (time_t)600;
  newcomer_saved = save_char_pets(&newcomer.owner);
  surviving_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);

  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, saved_before_rename);
  CuAssertTrue(tc, saved_after_rename);
  CuAssertIntEquals(tc, 2, renamed_rows);
  CuAssertTrue(tc, newcomer_saved);
  CuAssertIntEquals(tc, 2, surviving_rows);
}

void Test_pet_snapshot_save_commits_whole_owner_and_rolls_back_every_query_failure(CuTest *tc)
{
  struct pet_save_fixture fixture;
  const char *enabled;
  const char *loop_count_text;
  MYSQL *connection;
  MYSQL *saved_conn;
  struct descriptor_data *saved_descriptor_list;
  struct char_data *saved_characters = character_list;
  bool saved_available;
  bool schema_created;
  bool seeded;
  bool snapshot_saved;
  bool rollback_coverage_passed;
  bool overflow_rollback_passed;
  bool repeated_saves_passed;
  bool forced_save_result;
  bool stable_ids_passed;
  long first_pet_id;
  long second_pet_id;
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
  character_list = &fixture.owner;
  IN_ROOM(&fixture.owner) = 0;
  schema_created = create_pet_snapshot_temporary_schema(connection);
  seeded = schema_created && reset_old_pet_snapshot(connection);
  saved_descriptor_list = descriptor_list;
  descriptor_list = &fixture.descriptor;
  mysql_query_counter_reset();
  snapshot_saved = seeded && save_player_pets();
  save_query_count = (int)mysql_query_counter_value();
  first_pet_id = fixture.first_pet.pet_data_id;
  second_pet_id = fixture.second_pet.pet_data_id;
  stable_ids_passed = first_pet_id > 0 && second_pet_id > 0 && first_pet_id != second_pet_id;
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
    fixture.timed_affect.duration = 12 + failure_query;
    if (!reset_old_pet_snapshot(connection))
    {
      rollback_coverage_passed = false;
      break;
    }
    mysql_query_counter_reset();
    mysql_test_fail_nth_query((unsigned int)failure_query);
    forced_save_result = save_char_pets(&fixture.owner);
    mysql_test_clear_query_failure();
    stable_ids_passed = stable_ids_passed && fixture.first_pet.pet_data_id == first_pet_id &&
                        fixture.second_pet.pet_data_id == second_pet_id;
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
    stable_ids_passed = stable_ids_passed && fixture.first_pet.pet_data_id == first_pet_id &&
                        fixture.second_pet.pet_data_id == second_pet_id;
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
  character_list = saved_characters;
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
  CuAssertTrue(tc, stable_ids_passed);
}

void Test_pet_snapshot_lifecycle_handles_disconnect_and_follower_removal(CuTest *tc)
{
  struct pet_save_fixture fixture;
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  struct descriptor_data *saved_descriptor_list;
  struct char_data *saved_characters = character_list;
  bool saved_available;
  bool schema_created;
  bool initial_saved;
  bool disconnected_saved;
  bool detached_saved;
  bool queued_extraction_saved;
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
  character_list = &fixture.owner;
  IN_ROOM(&fixture.owner) = 0;
  descriptor_list = &fixture.descriptor;

  schema_created = create_pet_snapshot_temporary_schema(connection);
  mysql_query_counter_reset();
  initial_saved = schema_created && save_player_pets();
  initial_save_queries = (int)mysql_query_counter_value();
  initial_pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  initial_object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);

  STATE(&fixture.descriptor) = CON_DISCONNECT;
  fixture.timed_affect.duration--;
  mysql_query_counter_reset();
  disconnected_saved = save_player_pets();
  disconnected_save_queries = (int)mysql_query_counter_value();
  disconnected_saved = disconnected_saved && disconnected_save_queries == 9 &&
                       query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 2 &&
                       query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 3;

  fixture.owner.desc = NULL;
  fixture.descriptor.character = NULL;
  mysql_query_counter_reset();
  detached_saved = save_char_pets(&fixture.owner);
  detached_save_queries = (int)mysql_query_counter_value();
  detached_saved = detached_saved && detached_save_queries == 0 &&
                   query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 2 &&
                   query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1) == 3;

  SET_BIT_AR(MOB_FLAGS(&fixture.first_pet), MOB_NOTDEADYET);
  queued_extraction_saved = save_char_pets(&fixture.owner) &&
                            query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1) == 1;
  fixture.owner.followers = NULL;
  mysql_query_counter_reset();
  followers_removed = save_char_pets(&fixture.owner);
  removal_save_queries = (int)mysql_query_counter_value();
  final_pet_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_data", -1);
  final_object_rows = query_single_int(connection, "SELECT COUNT(*) FROM pet_save_objs", -1);

  mysql_test_clear_query_failure();
  descriptor_list = saved_descriptor_list;
  character_list = saved_characters;
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, initial_saved);
  CuAssertIntEquals(tc, 9, initial_save_queries);
  CuAssertIntEquals(tc, 2, initial_pet_rows);
  CuAssertIntEquals(tc, 3, initial_object_rows);
  CuAssertTrue(tc, disconnected_saved);
  CuAssertIntEquals(tc, 9, disconnected_save_queries);
  CuAssertTrue(tc, detached_saved);
  CuAssertTrue(tc, queued_extraction_saved);
  CuAssertIntEquals(tc, 0, detached_save_queries);
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
  source.pet_source_spell = SPELL_SHAMBLER;
  source.pet_behavior = PET_BEHAVIOR_GUARD;
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
  CuAssertIntEquals(tc, SPELL_SHAMBLER, restored.pet_source_spell);
  CuAssertIntEquals(tc, PET_BEHAVIOR_GUARD, restored.pet_behavior);
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

void Test_follower_runtime_source_supports_legacy_and_rejects_invalid_source(CuTest *tc)
{
  struct char_data source, restored;
  char *serialized, *line, *next;
  bool legacy, previous, invalid, invalid_behavior, missing_behavior;

  clear_char(&source);
  clear_char(&restored);
  SET_BIT_AR(MOB_FLAGS(&source), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&restored), MOB_ISNPC);
  source.pet_source_spell = SPELL_SHAMBLER;
  serialized = serialize_pet_runtime_state_for_test(&source);
  if (serialized == NULL)
  {
    CuFail(tc, "could not serialize source marker fixture");
    return;
  }
  line = strstr(serialized, "\nH ");
  if (line == NULL)
  {
    free(serialized);
    CuFail(tc, "missing behavior marker");
    return;
  }
  line[3] = '9';
  invalid_behavior = !restore_pet_runtime_state_for_test(&restored, serialized);
  next = strchr(line + 1, '\n');
  memmove(line, next, strlen(next) + 1);
  missing_behavior = !restore_pet_runtime_state_for_test(&restored, serialized);
  serialized[2] = '2';
  restored.pet_behavior = PET_BEHAVIOR_WAIT;
  previous = restore_pet_runtime_state_for_test(&restored, serialized) &&
             restored.pet_source_spell == SPELL_SHAMBLER &&
             restored.pet_behavior == PET_BEHAVIOR_FOLLOW;
  line = strstr(serialized, "\nP ");
  if (line == NULL)
  {
    free(serialized);
    CuFail(tc, "missing source marker");
    return;
  }
  line[3] = '-';
  invalid = !restore_pet_runtime_state_for_test(&restored, serialized);
  /* Reconstruct a complete V1 record by removing the new source record. */
  next = strchr(line + 1, '\n');
  memmove(line, next, strlen(next) + 1);
  serialized[2] = '1';
  legacy =
      restore_pet_runtime_state_for_test(&restored, serialized) && restored.pet_source_spell == 0;
  free(serialized);
  free_test_affects(&source);
  free_test_affects(&restored);
  CuAssertTrue(tc, invalid);
  CuAssertTrue(tc, invalid_behavior);
  CuAssertTrue(tc, missing_behavior);
  CuAssertTrue(tc, previous);
  CuAssertTrue(tc, legacy);
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
  CuAssertIntEquals(tc, PERSISTENCE_STEP_FAILURE, saved);
  CuAssertTrue(tc, PLR_FLAGGED(&ch, PLR_CRASH));

  /* Remove descriptor from descriptor_list */
  descriptor_list = desc.next;
  saved = Crash_save_incremental(1);
  CuAssertIntEquals(tc, PERSISTENCE_STEP_COMPLETE, saved);

  free(ch.player.name);
}

void Test_object_saves_bind_player_house_and_serialized_text(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  const char *owner_name = "Owner' OR '1'='1";
  const char *modes[] = {"SET SESSION sql_mode = ''",
                         "SET SESSION sql_mode = 'NO_BACKSLASH_ESCAPES'"};
  struct char_data ch;
  struct player_special_data specials = {0};
  struct obj_data *obj;
  struct extra_descr_data *extra;
  MYSQL *connection;
  MYSQL *saved_conn;
  MYSQL_RES *result;
  MYSQL_ROW row;
  FILE *fixture;
  char serialized[8192];
  size_t length;
  size_t mode;
  bool saved_available;
  bool matched = true;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;
  connection = open_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }
  fixture = tmpfile();
  if (fixture == NULL)
  {
    mysql_close(connection);
    CuFail(tc, "could not create object-save fixture");
    return;
  }
  saved_conn = conn;
  saved_available = mysql_available;
  conn = connection;
  mysql_available = true;
  clear_char(&ch);
  ch.player_specials = &specials;
  ch.player.name = (char *)owner_name;
  obj = create_obj();
  obj->name = strdup("blade'); DROP TABLE player_save_objs; --");
  obj->short_description = strdup("a 'quoted' blade\\edge");
  obj->description = strdup("A blade with a \\ mark is here.");
  obj->action_description = strdup("First 'line'\nSecond \\ line\n");
  obj->arcane_mark = strdup("Maker's mark");
  obj->restring_identifier = strdup("Owner's identifier");
  CREATE(extra, struct extra_descr_data, 1);
  extra->keyword = strdup("blade's edge");
  extra->description = strdup("A 'quoted' description with \\ and a newline.\n");
  obj->ex_description = extra;

  matched =
      mysql_query(connection, "CREATE TEMPORARY TABLE player_save_objs ("
                              "idnum INT AUTO_INCREMENT PRIMARY KEY, name VARCHAR(100), "
                              "serialized_obj TEXT)") == 0 &&
      mysql_query(connection,
                  "CREATE TEMPORARY TABLE house_data ("
                  "idnum INT AUTO_INCREMENT PRIMARY KEY, vnum INT, serialized_obj TEXT)") == 0;
  for (mode = 0; matched && mode < sizeof(modes) / sizeof(modes[0]); mode++)
  {
    matched = mysql_query(connection, modes[mode]) == 0 &&
              mysql_query(connection, "DELETE FROM player_save_objs") == 0 &&
              mysql_query(connection, "DELETE FROM house_data") == 0;
    matched = matched && fflush(fixture) == 0 && ftruncate(fileno(fixture), 0) == 0;
    rewind(fixture);
    objsave_save_obj_record_db(obj, &ch, NOWHERE, fixture, 3);
    fflush(fixture);
    rewind(fixture);
    length = fread(serialized, 1, sizeof(serialized) - 1, fixture);
    /* The file has a final blank line separating objects; the database does not. */
    if (length == 0 || serialized[length - 1] != '\n')
      matched = false;
    else
      serialized[length - 1] = '\0';
    if (mysql_query(connection, "SELECT name, serialized_obj FROM player_save_objs") != 0)
      matched = false;
    result = mysql_store_result(connection);
    row = result != NULL ? mysql_fetch_row(result) : NULL;
    matched = matched && row != NULL && mysql_num_rows(result) == 1 && row[0] != NULL &&
              row[1] != NULL && strcmp(row[0], owner_name) == 0 && strcmp(row[1], serialized) == 0;
    if (result != NULL)
      mysql_free_result(result);

    objsave_save_obj_record_db(obj, NULL, NOWHERE, fixture, 3);
    if (mysql_query(connection, "SELECT vnum, serialized_obj FROM house_data") != 0)
      matched = false;
    result = mysql_store_result(connection);
    row = result != NULL ? mysql_fetch_row(result) : NULL;
    matched = matched && row != NULL && mysql_num_rows(result) == 1 && row[0] != NULL &&
              row[1] != NULL && atoi(row[0]) == (int)NOWHERE && strcmp(row[1], serialized) == 0;
    if (result != NULL)
      mysql_free_result(result);
  }

  /* Oversized keywords must not become valid but partial database records. */
  free(extra->keyword);
  CREATE(extra->keyword, char, 100001U);
  memset(extra->keyword, 'k', 100000U);
  extra->keyword[100000] = '\0';
  objsave_save_obj_record_db(obj, &ch, NOWHERE, fixture, 3);
  objsave_save_obj_record_db(obj, NULL, NOWHERE, fixture, 3);
  matched = matched &&
            query_single_int(connection, "SELECT COUNT(*) FROM player_save_objs", -1) == 1 &&
            query_single_int(connection, "SELECT COUNT(*) FROM house_data", -1) == 1;

  /* Delimiters still terminate both fields, even with an oversized suffix. */
  extra->keyword[4] = '~';
  free(extra->description);
  extra->description = strdup("visible~discarded");
  objsave_save_obj_record_db(obj, &ch, NOWHERE, fixture, 3);
  if (mysql_query(connection,
                  "SELECT serialized_obj FROM player_save_objs ORDER BY idnum DESC LIMIT 1") != 0)
    matched = false;
  result = mysql_store_result(connection);
  row = result != NULL ? mysql_fetch_row(result) : NULL;
  matched = matched && row != NULL && row[0] != NULL &&
            strstr(row[0], "EDes:\nkkkk~\nvisible~\n") != NULL &&
            strstr(row[0], "discarded") == NULL &&
            query_single_int(connection, "SELECT COUNT(*) FROM player_save_objs", -1) == 2;
  if (result != NULL)
    mysql_free_result(result);
  matched = matched && mysql_query(connection,
                                   "DELETE FROM player_save_objs ORDER BY idnum DESC LIMIT 1") == 0;

  /* A full payload buffer must not become a valid but partial database record. */
  for (mode = 0; mode < 10U; mode++)
  {
    CREATE(extra, struct extra_descr_data, 1);
    extra->keyword = strdup("overflow");
    CREATE(extra->description, char, 4001U);
    memset(extra->description, 'x', 4000U);
    extra->description[4000] = '\0';
    extra->next = obj->ex_description;
    obj->ex_description = extra;
  }
  objsave_save_obj_record_db(obj, &ch, NOWHERE, fixture, 3);
  objsave_save_obj_record_db(obj, NULL, NOWHERE, fixture, 3);
  matched = matched &&
            query_single_int(connection, "SELECT COUNT(*) FROM player_save_objs", -1) == 1 &&
            query_single_int(connection, "SELECT COUNT(*) FROM house_data", -1) == 1;

  free(obj->arcane_mark);
  obj->arcane_mark = NULL;
  extract_obj(obj);
  fclose(fixture);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);
  CuAssertTrue(tc, matched);
}
