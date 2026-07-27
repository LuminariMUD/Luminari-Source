/* ***************************************************************************
 *  File: test_artifacts.c                            Part of LuminariMUD
 *  Usage: Production-linked tests for the artifact system.
 *
 *  These exercise the parts of src/world/spec_artifacts.c that do not need a
 *  booted world: registry lookup, the XP curve, binding-name mapping, and
 *  save/load round-tripping of the data file.
 *************************************************************************** */

#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/spells.h"
#include "../../src/world/spec_artifacts.h"

/* --------------------------------------------------------------------------
 * Helpers
 *
 * artifact_boot() needs a loaded object table, which these tests do not have.
 * The registry is built by hand instead so the pure logic can be exercised.
 * -------------------------------------------------------------------------- */

static void artifact_test_registry(int count)
{
  int i = 0;

  artifact_shutdown();

  art_index = calloc(count, sizeof(struct artifact_data));
  total_artifacts = count;

  for (i = 0; i < count; i++)
  {
    art_index[i].vnum = ART_VNUM_TRORXEK + i;
    art_index[i].owner = strdup(ARTIFACT_OWNER_NONE);
    art_index[i].account = strdup(ARTIFACT_OWNER_NONE);
    art_index[i].level = 1;
    art_index[i].experience = 0;
    art_index[i].binding_type = ARTIFACT_BIND_NONE;
    art_index[i].instance_persisted = FALSE;
  }
}

struct artifact_test_object_fixture
{
  struct index_data indexes[11];
  struct index_data *saved_obj_index;
  obj_rnum saved_top_of_objt;
};

static void artifact_test_begin_objects(struct artifact_test_object_fixture *fixture)
{
  int i = 0;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_obj_index = obj_index;
  fixture->saved_top_of_objt = top_of_objt;

  for (i = 0; i < 11; i++)
    fixture->indexes[i].vnum = ART_VNUM_TRORXEK + i;

  obj_index = fixture->indexes;
  top_of_objt = 10;
}

static void artifact_test_end_objects(struct artifact_test_object_fixture *fixture)
{
  obj_index = fixture->saved_obj_index;
  top_of_objt = fixture->saved_top_of_objt;
}

static void artifact_test_object(struct obj_data *obj, obj_rnum rnum)
{
  clear_object(obj);
  GET_OBJ_RNUM(obj) = rnum;
  obj->short_description = "a test artifact";
}

/* artifact_save() writes to a path relative to the working directory, which
 * for the test binary is the project root rather than lib/.  Run the save
 * tests inside a scratch directory so they never touch real game data. */
static char artifact_test_oldcwd[PATH_MAX];

static int artifact_test_enter_sandbox(void)
{
  char dir[] = "/tmp/lum_artifact_test_XXXXXX";
  char worlddir[PATH_MAX];

  if (!getcwd(artifact_test_oldcwd, sizeof(artifact_test_oldcwd)))
    return 0;

  if (!mkdtemp(dir))
    return 0;

  snprintf(worlddir, sizeof(worlddir), "%s/world", dir);
  if (mkdir(worlddir, 0700) != 0)
    return 0;

  return (chdir(dir) == 0);
}

static void artifact_test_leave_sandbox(void)
{
  if (*artifact_test_oldcwd)
    if (chdir(artifact_test_oldcwd) != 0)
      artifact_test_oldcwd[0] = '\0';
}

static int artifact_test_write_file(const char *contents)
{
  FILE *fl = fopen(ARTIFACT_FILE, "w");

  if (!fl)
    return FALSE;

  fputs(contents, fl);
  fclose(fl);
  return TRUE;
}

static int artifact_test_file_contains(const char *path, const char *needle)
{
  FILE *fl = NULL;
  char line[READ_SIZE] = {'\0'};

  if (!(fl = fopen(path, "r")))
    return FALSE;

  while (fgets(line, sizeof(line), fl))
  {
    if (strstr(line, needle))
    {
      fclose(fl);
      return TRUE;
    }
  }

  fclose(fl);
  return FALSE;
}

static const char *artifact_test_source_root(void)
{
  const char *root = getenv("LUMINARI_TEST_ROOT");

  return root && *root ? root : ".";
}

/* --------------------------------------------------------------------------
 * Binary search
 * -------------------------------------------------------------------------- */

void Test_artifact_search_finds_every_entry(CuTest *tc)
{
  int i = 0;

  artifact_test_registry(11);

  for (i = 0; i < 11; i++)
    CuAssertIntEquals(tc, i, artifact_search(ART_VNUM_TRORXEK + i));

  artifact_shutdown();
}

void Test_artifact_search_rejects_unknown_vnums(CuTest *tc)
{
  artifact_test_registry(11);

  CuAssertIntEquals(tc, -1, artifact_search(ART_VNUM_TRORXEK - 1));
  CuAssertIntEquals(tc, -1, artifact_search(ART_VNUM_TRORXEK + 11));
  CuAssertIntEquals(tc, -1, artifact_search(0));
  CuAssertIntEquals(tc, -1, artifact_search(-5));
  CuAssertIntEquals(tc, -1, artifact_search(999999));

  artifact_shutdown();
}

void Test_artifact_search_safe_on_empty_registry(CuTest *tc)
{
  artifact_shutdown();

  CuAssertIntEquals(tc, -1, artifact_search(ART_VNUM_TRORXEK));
  CuAssertPtrEquals(tc, NULL, artifact_by_vnum(ART_VNUM_TRORXEK));
  CuAssertIntEquals(tc, FALSE, artifact_is_owned(ART_VNUM_TRORXEK));
}

void Test_artifact_by_vnum_matches_search(CuTest *tc)
{
  struct artifact_data *art = NULL;

  artifact_test_registry(11);

  art = artifact_by_vnum(ART_VNUM_DOOMBRINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_VNUM_DOOMBRINGER, art->vnum);

  CuAssertPtrEquals(tc, NULL, artifact_by_vnum(ART_VNUM_TRORXEK - 100));

  artifact_shutdown();
}

/* --------------------------------------------------------------------------
 * Ownership sentinels
 * -------------------------------------------------------------------------- */

void Test_artifact_sentinel_names_mean_unowned(CuTest *tc)
{
  struct artifact_data *art = NULL;
  const char *sentinels[] = {ARTIFACT_OWNER_NONE, ARTIFACT_OWNER_INIT, "none", "no"};
  int i = 0;

  artifact_test_registry(2);
  art = &art_index[0];

  for (i = 0; i < 4; i++)
  {
    free(art->owner);
    art->owner = strdup(sentinels[i]);
    CuAssertIntEquals(tc, FALSE, artifact_is_owned(art->vnum));
  }

  free(art->owner);
  art->owner = strdup("Zusuk");
  CuAssertIntEquals(tc, TRUE, artifact_is_owned(art->vnum));

  artifact_shutdown();
}

void Test_artifact_empty_owner_is_unowned(CuTest *tc)
{
  struct artifact_data *art = NULL;

  artifact_test_registry(2);
  art = &art_index[0];

  free(art->owner);
  art->owner = strdup("");
  CuAssertIntEquals(tc, FALSE, artifact_is_owned(art->vnum));

  free(art->owner);
  art->owner = NULL;
  CuAssertIntEquals(tc, FALSE, artifact_is_owned(art->vnum));

  artifact_shutdown();
}

/* --------------------------------------------------------------------------
 * Progression curve
 * -------------------------------------------------------------------------- */

void Test_artifact_xp_curve_is_monotonic(CuTest *tc)
{
  int level = 0, previous = 0, needed = 0;

  for (level = 1; level < ARTIFACT_MAX_LEVEL; level++)
  {
    needed = artifact_xp_to_next(level);
    CuAssertTrue(tc, needed > previous);
    previous = needed;
  }
}

void Test_artifact_xp_curve_known_values(CuTest *tc)
{
  CuAssertIntEquals(tc, 100, artifact_xp_to_next(1));
  CuAssertIntEquals(tc, 300, artifact_xp_to_next(2));
  CuAssertIntEquals(tc, 600, artifact_xp_to_next(3));
  CuAssertIntEquals(tc, 1000, artifact_xp_to_next(4));
}

void Test_artifact_xp_curve_out_of_range(CuTest *tc)
{
  /* Max level and beyond have nothing to advance to. */
  CuAssertIntEquals(tc, 0, artifact_xp_to_next(ARTIFACT_MAX_LEVEL));
  CuAssertIntEquals(tc, 0, artifact_xp_to_next(ARTIFACT_MAX_LEVEL + 1));
  CuAssertIntEquals(tc, 0, artifact_xp_to_next(0));
  CuAssertIntEquals(tc, 0, artifact_xp_to_next(-1));
  CuAssertIntEquals(tc, 0, artifact_xp_to_next(9999));
}

void Test_artifact_levelup_advances_one_level(CuTest *tc)
{
  struct artifact_data *art = NULL;

  CuAssertTrue(tc, artifact_test_enter_sandbox());
  artifact_test_registry(2);
  art = &art_index[0];

  /* Just short of the threshold - no advance. */
  art->experience = artifact_xp_to_next(1) - 1;
  artifact_check_levelup(art);
  CuAssertIntEquals(tc, 1, art->level);

  /* On the threshold - exactly one level. */
  art->experience = artifact_xp_to_next(1);
  artifact_check_levelup(art);
  CuAssertIntEquals(tc, 2, art->level);

  /* A huge total still only advances one level per call. */
  art->experience = 100000;
  artifact_check_levelup(art);
  CuAssertIntEquals(tc, 3, art->level);

  artifact_shutdown();
  artifact_test_leave_sandbox();
}

void Test_artifact_levelup_stops_at_max(CuTest *tc)
{
  struct artifact_data *art = NULL;
  int i = 0;

  artifact_test_registry(2);
  art = &art_index[0];

  art->level = ARTIFACT_MAX_LEVEL;
  art->experience = 100000;

  for (i = 0; i < 10; i++)
    artifact_check_levelup(art);

  CuAssertIntEquals(tc, ARTIFACT_MAX_LEVEL, art->level);

  artifact_shutdown();
}

void Test_artifact_levelup_tolerates_null(CuTest *tc)
{
  /* Must not crash. */
  artifact_check_levelup(NULL);
  CuAssertTrue(tc, TRUE);
}

/* --------------------------------------------------------------------------
 * Binding
 * -------------------------------------------------------------------------- */

void Test_artifact_binding_names_cover_all_types(CuTest *tc)
{
  int i = 0;

  for (i = 0; i < NUM_ARTIFACT_BINDINGS; i++)
  {
    CuAssertPtrNotNull(tc, artifact_binding_name(i));
    CuAssertTrue(tc, strcmp(artifact_binding_name(i), "Unknown") != 0);
  }
}

void Test_artifact_binding_name_rejects_out_of_range(CuTest *tc)
{
  CuAssertStrEquals(tc, "Unknown", artifact_binding_name(-1));
  CuAssertStrEquals(tc, "Unknown", artifact_binding_name(NUM_ARTIFACT_BINDINGS));
  CuAssertStrEquals(tc, "Unknown", artifact_binding_name(9999));
}

/* --------------------------------------------------------------------------
 * Persistence round-trip
 * -------------------------------------------------------------------------- */

void Test_artifact_save_writes_every_record(CuTest *tc)
{
  FILE *fl = NULL;
  char line[256] = {'\0'};
  int records = 0;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the save test");
    return;
  }

  artifact_test_registry(11);

  art_index[0].level = 4;
  art_index[0].experience = 777;
  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  art_index[0].bound_time = 12345;

  artifact_save();

  fl = fopen(ARTIFACT_FILE, "r");
  if (!fl)
  {
    artifact_test_leave_sandbox();
    artifact_shutdown();
    CuFail(tc, "artifact_save() did not create the data file");
    return;
  }

  while (fgets(line, sizeof(line), fl))
    if (line[0] != '#' && line[0] != '\n' && line[0] != '\r')
      records++;

  fclose(fl);

  artifact_test_leave_sandbox();
  artifact_shutdown();

  CuAssertIntEquals(tc, 11, records);
}

void Test_artifact_save_round_trips_values(CuTest *tc)
{
  FILE *fl = NULL;
  char line[256] = {'\0'};
  char owner[128] = {'\0'};
  char account[128] = {'\0'};
  int vnum = 0, level = 0, exp = 0, persisted = 0, found = 0;
  long bound = 0;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the save test");
    return;
  }

  artifact_test_registry(11);

  art_index[0].level = 4;
  art_index[0].experience = 777;
  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  free(art_index[0].account);
  art_index[0].account = strdup("zusuk_account");
  art_index[0].bound_time = 12345;
  art_index[0].instance_persisted = TRUE;

  artifact_save();

  fl = fopen(ARTIFACT_FILE, "r");
  if (!fl)
  {
    artifact_test_leave_sandbox();
    artifact_shutdown();
    CuFail(tc, "artifact_save() did not create the data file");
    return;
  }

  while (fgets(line, sizeof(line), fl))
  {
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
      continue;

    if (sscanf(line, "%d %127s %127s %d %d %ld %d", &vnum, owner, account, &level, &exp, &bound,
               &persisted) != 7)
      continue;

    if (vnum != ART_VNUM_TRORXEK)
      continue;

    found = 1;
    CuAssertStrEquals(tc, "Zusuk", owner);
    CuAssertStrEquals(tc, "zusuk_account", account);
    CuAssertIntEquals(tc, 4, level);
    CuAssertIntEquals(tc, 777, exp);
    CuAssertIntEquals(tc, 12345, (int)bound);
    CuAssertIntEquals(tc, TRUE, persisted);
    break;
  }

  fclose(fl);

  artifact_test_leave_sandbox();
  artifact_shutdown();

  CuAssertIntEquals(tc, 1, found);
}

void Test_artifact_save_safe_on_empty_registry(CuTest *tc)
{
  artifact_shutdown();

  /* Must be a no-op rather than a crash or a truncated file. */
  artifact_save();
  CuAssertPtrEquals(tc, NULL, art_index);
  CuAssertIntEquals(tc, 0, total_artifacts);
}

void Test_artifact_subthreshold_xp_flushes_when_dirty(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct char_data ch;
  struct obj_data obj;
  FILE *fl = NULL;
  char line[256] = {'\0'};
  char owner[128] = {'\0'}, account[128] = {'\0'};
  int vnum = 0, level = 0, exp = -1, persisted = 0;
  int found = FALSE;
  long bound = 0;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the dirty XP test");
    return;
  }

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  clear_char(&ch);
  artifact_test_object(&obj, 0);

  artifact_grant_xp_obj(&ch, &obj, ARTIFACT_XP_FIRST_EQUIP);
  artifact_save_if_dirty();

  if ((fl = fopen(ARTIFACT_FILE, "r")))
  {
    while (fgets(line, sizeof(line), fl))
    {
      if (sscanf(line, "%d %127s %127s %d %d %ld %d", &vnum, owner, account, &level, &exp, &bound,
                 &persisted) == 7 &&
          vnum == ART_VNUM_TRORXEK)
      {
        found = TRUE;
        break;
      }
    }
    fclose(fl);
  }

  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  artifact_test_leave_sandbox();

  CuAssertIntEquals(tc, TRUE, found);
  CuAssertIntEquals(tc, ARTIFACT_XP_FIRST_EQUIP, exp);
}

void Test_artifact_v20_layout_loads_without_column_shift(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  char account[MAX_INPUT_LENGTH] = {'\0'};
  int level = 0, exp = 0, persisted = FALSE;
  long bound = 0;
  int wrote = FALSE;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the v2.0 load test");
    return;
  }

  artifact_test_begin_objects(&fixture);
  wrote = artifact_test_write_file("# Artifact Ownership File v2.0\n"
                                   "169901 Zusuk 4 777 2 12345\n");
  if (wrote)
  {
    artifact_boot();
    snprintf(owner, sizeof(owner), "%s", art_index[0].owner);
    snprintf(account, sizeof(account), "%s", art_index[0].account);
    level = art_index[0].level;
    exp = art_index[0].experience;
    bound = (long)art_index[0].bound_time;
    persisted = art_index[0].instance_persisted;
  }

  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  artifact_test_leave_sandbox();

  CuAssertIntEquals(tc, TRUE, wrote);
  CuAssertStrEquals(tc, "Zusuk", owner);
  CuAssertStrEquals(tc, ARTIFACT_OWNER_NONE, account);
  CuAssertIntEquals(tc, 4, level);
  CuAssertIntEquals(tc, 777, exp);
  CuAssertIntEquals(tc, 12345, (int)bound);
  CuAssertIntEquals(tc, TRUE, persisted);
}

void Test_artifact_v1_layout_loads_timestamp_separately(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  char account[MAX_INPUT_LENGTH] = {'\0'};
  int level = 0, exp = -1, persisted = FALSE;
  long bound = 0;
  int wrote = FALSE;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the v1 load test");
    return;
  }

  artifact_test_begin_objects(&fixture);
  wrote = artifact_test_write_file("# Artifact Ownership File v1\n"
                                   "169901 Zusuk 12345\n");
  if (wrote)
  {
    artifact_boot();
    snprintf(owner, sizeof(owner), "%s", art_index[0].owner);
    snprintf(account, sizeof(account), "%s", art_index[0].account);
    level = art_index[0].level;
    exp = art_index[0].experience;
    bound = (long)art_index[0].bound_time;
    persisted = art_index[0].instance_persisted;
  }

  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  artifact_test_leave_sandbox();

  CuAssertIntEquals(tc, TRUE, wrote);
  CuAssertStrEquals(tc, "Zusuk", owner);
  CuAssertStrEquals(tc, ARTIFACT_OWNER_NONE, account);
  CuAssertIntEquals(tc, 1, level);
  CuAssertIntEquals(tc, 0, exp);
  CuAssertIntEquals(tc, 12345, (int)bound);
  CuAssertIntEquals(tc, TRUE, persisted);
}

void Test_artifact_world_package_contains_all_deployable_records(CuTest *tc)
{
  char path[PATH_MAX] = {'\0'};
  char needle[64] = {'\0'};
  const char *root = artifact_test_source_root();
  int complete = TRUE;
  int vnum = 0;

  snprintf(path, sizeof(path), "%s/lib/world/artifacts/1699.obj", root);
  for (vnum = ART_VNUM_TRORXEK; vnum <= ART_VNUM_AEGIS; vnum++)
  {
    snprintf(needle, sizeof(needle), "#%d", vnum);
    if (!artifact_test_file_contains(path, needle))
      complete = FALSE;
  }

  snprintf(path, sizeof(path), "%s/lib/world/artifacts/1699.zon", root);
  for (vnum = ART_VNUM_TRORXEK; vnum <= ART_VNUM_AEGIS; vnum++)
  {
    snprintf(needle, sizeof(needle), "O 0 %d 1 169900", vnum);
    if (!artifact_test_file_contains(path, needle))
      complete = FALSE;
  }

  snprintf(path, sizeof(path), "%s/lib/world/artifacts/1699.wld", root);
  complete = complete && artifact_test_file_contains(path, "#169900");
  snprintf(path, sizeof(path), "%s/lib/world/artifacts/1699.mob", root);
  complete = complete && artifact_test_file_contains(path, "$");
  snprintf(path, sizeof(path), "%s/lib/world/artifacts/artifacts.hlp", root);
  complete = complete && artifact_test_file_contains(path, "ARTIFACT ARTIFACTS");
  snprintf(path, sizeof(path), "%s/scripts/provision_artifacts.sh", root);
  complete = complete && artifact_test_file_contains(path, "ensure_index_entry");

  CuAssertIntEquals(tc, TRUE, complete);
}

/* --------------------------------------------------------------------------
 * Shutdown
 * -------------------------------------------------------------------------- */

void Test_artifact_shutdown_is_idempotent(CuTest *tc)
{
  artifact_test_registry(11);

  artifact_shutdown();
  CuAssertPtrEquals(tc, NULL, art_index);
  CuAssertIntEquals(tc, 0, total_artifacts);

  /* A second call must not double-free. */
  artifact_shutdown();
  CuAssertPtrEquals(tc, NULL, art_index);
  CuAssertIntEquals(tc, 0, total_artifacts);
}

void Test_artifact_persistence_extract_preserves_owned_instance(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct char_data ch;
  struct obj_data obj;
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  int bound = 0, persisted = FALSE;

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  clear_char(&ch);
  artifact_test_object(&obj, 0);
  obj.carried_by = &ch;

  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  art_index[0].bound_time = 12345;
  art_index[0].instance_persisted = TRUE;

  artifact_begin_persistence_extract();
  artifact_on_extract(&obj);
  artifact_end_persistence_extract();

  snprintf(owner, sizeof(owner), "%s", art_index[0].owner);
  bound = (int)art_index[0].bound_time;
  persisted = art_index[0].instance_persisted;

  artifact_shutdown();
  artifact_test_end_objects(&fixture);

  CuAssertStrEquals(tc, "Zusuk", owner);
  CuAssertIntEquals(tc, 12345, bound);
  CuAssertIntEquals(tc, TRUE, persisted);
}

void Test_artifact_inventory_destruction_releases_ownership(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct char_data ch;
  struct obj_data obj;
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  int bound = -1, persisted = TRUE;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the destruction test");
    return;
  }

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  clear_char(&ch);
  artifact_test_object(&obj, 0);
  obj.carried_by = &ch;

  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  art_index[0].bound_time = 12345;
  art_index[0].instance_persisted = TRUE;

  artifact_on_extract(&obj);

  snprintf(owner, sizeof(owner), "%s", art_index[0].owner);
  bound = (int)art_index[0].bound_time;
  persisted = art_index[0].instance_persisted;

  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  artifact_test_leave_sandbox();

  CuAssertStrEquals(tc, ARTIFACT_OWNER_NONE, owner);
  CuAssertIntEquals(tc, 0, bound);
  CuAssertIntEquals(tc, FALSE, persisted);
}

void Test_artifact_temporary_clone_does_not_release_live_owner(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct obj_data obj;
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  int bound = 0, persisted = FALSE;

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  artifact_test_object(&obj, 0);

  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  art_index[0].bound_time = 12345;
  art_index[0].instance_persisted = TRUE;

  artifact_on_extract(&obj);

  snprintf(owner, sizeof(owner), "%s", art_index[0].owner);
  bound = (int)art_index[0].bound_time;
  persisted = art_index[0].instance_persisted;

  artifact_shutdown();
  artifact_test_end_objects(&fixture);

  CuAssertStrEquals(tc, "Zusuk", owner);
  CuAssertIntEquals(tc, 12345, bound);
  CuAssertIntEquals(tc, TRUE, persisted);
}

void Test_artifact_bound_normal_room_drop_is_recoverable(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct room_data room;
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  struct obj_data obj;
  int persisted = TRUE, blocked = TRUE;

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the room-drop test");
    return;
  }

  memset(&room, 0, sizeof(room));
  world = &room;
  top_of_world = 0;
  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  artifact_test_object(&obj, 0);
  IN_ROOM(&obj) = 0;

  free(art_index[0].owner);
  art_index[0].owner = strdup("Zusuk");
  art_index[0].bound_time = 12345;
  art_index[0].instance_persisted = TRUE;

  artifact_obj_to_room(&obj);
  persisted = art_index[0].instance_persisted;
  blocked = artifact_block_zone_load(0);

  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  world = saved_world;
  top_of_world = saved_top_of_world;
  artifact_test_leave_sandbox();

  CuAssertIntEquals(tc, FALSE, persisted);
  CuAssertIntEquals(tc, FALSE, blocked);
}

void Test_artifact_zone_load_blocks_second_live_instance(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  int blocks_first = FALSE, blocks_second = FALSE;

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);

  fixture.indexes[0].number = 0;
  blocks_first = artifact_block_zone_load(0);
  fixture.indexes[0].number = 1;
  blocks_second = artifact_block_zone_load(0);

  artifact_shutdown();
  artifact_test_end_objects(&fixture);

  CuAssertIntEquals(tc, FALSE, blocks_first);
  CuAssertIntEquals(tc, TRUE, blocks_second);
}

void Test_artifact_reload_reassociates_holder_and_refreshes_bonus(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct char_data ch;
  struct obj_data obj;
  struct obj_data *saved_object_list = object_list;
  struct affected_type *af = NULL, *next_af = NULL;
  struct artifact_data *art = NULL;
  int wrote = FALSE, holder_restored = FALSE, modifier = 0;

  memset(&ch, 0, sizeof(ch));
  memset(&obj, 0, sizeof(obj));

  if (!artifact_test_enter_sandbox())
  {
    CuFail(tc, "could not create a scratch directory for the reload test");
    return;
  }

  artifact_test_begin_objects(&fixture);
  wrote = artifact_test_write_file("# Artifact Ownership File v2.2\n"
                                   "169901 Zusuk noone 1 50 12345 1\n");
  if (wrote)
  {
    artifact_boot();
    clear_char(&ch);
    ch.player_specials = &dummy_mob;
    SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
    ch.mute_equip_messages = TRUE;
    artifact_test_object(&obj, 0);
    obj.worn_by = &ch;
    obj.worn_on = WEAR_WIELD_1;
    GET_EQ(&ch, WEAR_WIELD_1) = &obj;
    object_list = &obj;

    artifact_apply_bonuses(&ch, &obj);
    artifact_test_write_file("# Artifact Ownership File v2.2\n"
                             "169901 Zusuk noone 3 650 12345 1\n");
    artifact_reload();

    art = artifact_by_vnum(ART_VNUM_TRORXEK);
    holder_restored = art && art->ch == &ch;
    for (af = ch.affected; af; af = af->next)
      if (af->spell == SPELL_ARTIFACT_BONUS && af->specific == 1 && af->location == APPLY_WIS)
        modifier = af->modifier;
  }

  for (af = ch.affected; af; af = next_af)
  {
    next_af = af->next;
    free(af);
  }
  ch.affected = NULL;
  object_list = saved_object_list;
  artifact_shutdown();
  artifact_test_end_objects(&fixture);
  artifact_test_leave_sandbox();

  CuAssertIntEquals(tc, TRUE, wrote);
  CuAssertIntEquals(tc, TRUE, holder_restored);
  CuAssertIntEquals(tc, 6, modifier);
}

void Test_artifact_bonus_messages_honor_save_mute(CuTest *tc)
{
  struct artifact_test_object_fixture fixture;
  struct char_data ch;
  struct descriptor_data desc;
  struct obj_data obj;
  int output_length = -1, affects_removed = FALSE;

  artifact_test_begin_objects(&fixture);
  artifact_test_registry(11);
  clear_char(&ch);
  memset(&desc, 0, sizeof(desc));
  ch.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&ch), MOB_ISNPC);
  ch.mute_equip_messages = TRUE;
  ch.desc = &desc;
  desc.character = &ch;
  desc.output = desc.small_outbuf;
  desc.bufspace = SMALL_BUFSIZE - 1;
  artifact_test_object(&obj, 0);
  art_index[0].stat_bonus[ART_STAT_STR] = 1;

  artifact_apply_bonuses(&ch, &obj);
  artifact_remove_bonuses(&ch, &obj);
  output_length = desc.bufptr;
  affects_removed = ch.affected == NULL;

  artifact_shutdown();
  artifact_test_end_objects(&fixture);

  CuAssertIntEquals(tc, 0, output_length);
  CuAssertIntEquals(tc, TRUE, affects_removed);
}

/* --------------------------------------------------------------------------
 * NULL-safety of the hook surface
 *
 * Every hook is called from a hot core-file path, so none of them may crash
 * on a NULL argument.
 * -------------------------------------------------------------------------- */

void Test_artifact_hooks_tolerate_null(CuTest *tc)
{
  artifact_shutdown();

  artifact_obj_to_char(NULL, NULL);
  artifact_obj_from_char(NULL);
  artifact_obj_to_room(NULL);
  artifact_on_unequip(NULL, NULL);
  artifact_on_extract(NULL);
  artifact_tag_nested(NULL, NULL);
  artifact_get_nested(NULL, NULL);
  artifact_drop_nested(NULL);
  artifact_apply_bonuses(NULL, NULL);
  artifact_remove_bonuses(NULL, NULL);
  artifact_grant_xp(NULL, 10);
  artifact_grant_xp_obj(NULL, NULL, 10);
  artifact_weapon_proc(NULL, NULL, NULL);

  CuAssertIntEquals(tc, TRUE, artifact_on_equip(NULL, NULL, 0));
  CuAssertIntEquals(tc, FALSE, artifact_can_use(NULL, NULL, TRUE));
  CuAssertIntEquals(tc, FALSE, artifact_is_artifact(NULL));
  CuAssertPtrEquals(tc, NULL, artifact_of_obj(NULL));
  CuAssertIntEquals(tc, FALSE, artifact_block_zone_load(NOTHING));

  /* Damage passes through untouched when there is no victim. */
  CuAssertIntEquals(tc, 100, artifact_damage_resist(NULL, 100, DAM_FIRE));
}

/*EOF*/
