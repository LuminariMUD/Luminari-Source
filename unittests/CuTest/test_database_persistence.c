#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/mysql.h"
#include "../../src/magic/spells.h"

#include <stdlib.h>
#include <string.h>

extern MYSQL *conn;
extern bool mysql_available;
extern char *serialize_pet_runtime_state_for_test(struct char_data *pet);
extern bool restore_pet_runtime_state_for_test(struct char_data *pet, const char *serialized);

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
