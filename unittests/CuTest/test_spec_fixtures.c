#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/db.h"
#include "../../src/olc/genmob.h"
#include "../../src/olc/genobj.h"
#include "../../src/olc/genwld.h"
#include "../../src/olc/oasis.h"
#include "../../src/net/protocol.h"
#include "../../src/spec_procs.h"
#include "test_spec_fixtures.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SPEC_TEST_MOBILE_VNUM 1201
#define SPEC_TEST_OBJECT_VNUM 1402
#define SPEC_TEST_ROOM_VNUM 1403
#define SPEC_TEST_MOBILE_ZONE 12
#define SPEC_TEST_OBJECT_ROOM_ZONE 14
#define SPEC_TEST_MAX_SAVED_FILE (1024L * 1024L)

static const char spec_test_mobile_record[] = "postmaster~\n"
                                              "the test postmaster~\n"
                                              "The test postmaster is here.\n~\n"
                                              "A test postmaster.\n~\n"
                                              "0 0 0 0 0 0 0 0 0 E\n"
                                              "1 20 10 1d1+0 1d1+0\n"
                                              "0 0\n"
                                              "8 8 0\n"
                                              "SpecProc: Postmaster\n"
                                              "DR_MOD: 0\n"
                                              "E\n"
                                              "$~\n";

static const char spec_test_object_record[] = "test vessel~\n"
                                              "a test vessel~\n"
                                              "A test vessel is here.~\n"
                                              "~\n"
                                              "12 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                                              "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                                              "1 0 0 1 0\n"
                                              "Z\n"
                                              "Greyhawk Ship\n"
                                              "$~\n";

static const char spec_test_room_record[] = "A Test Vessel Room~\n"
                                            "A test vessel room.\n~\n"
                                            "14 0 0 0 0 0\n"
                                            "C\n"
                                            "0 0\n"
                                            "Z\n"
                                            "Greyhawk Ship Commands\n"
                                            "S\n"
                                            "$~\n";

struct spec_test_fixture
{
  struct room_data test_world[1];
  struct char_data test_mob_proto[1];
  struct index_data test_mob_index[1];
  struct obj_data test_obj_proto[1];
  struct index_data test_obj_index[1];
  struct zone_data test_zone_table[2];

  struct room_data *saved_world;
  struct char_data *saved_mob_proto;
  struct index_data *saved_mob_index;
  struct obj_data *saved_obj_proto;
  struct index_data *saved_obj_index;
  struct zone_data *saved_zone_table;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
  obj_rnum saved_top_of_objt;
  zone_rnum saved_top_of_zone_table;
  int saved_diagonal_dirs;
  ubyte saved_wilderness_system;

  struct descriptor_data descriptor;
  struct char_data builder;
  struct player_special_data builder_specials;
  struct oasis_olc_data olc;
  struct txt_block output_block;
  char output_buffer[LARGE_BUFSIZE];

  char original_cwd[PATH_MAX];
  char sandbox[PATH_MAX];
  char *saved_text[SPEC_TEST_OWNER_COUNT];
  bool mobile_loaded;
  bool object_loaded;
  bool room_loaded;
  bool sandbox_created;
};

static void spec_test_set_error(char *error, size_t error_size, const char *message)
{
  if (error == NULL || error_size == 0)
    return;

  snprintf(error, error_size, "%s", message != NULL ? message : "unknown fixture error");
}

static bool spec_test_build_path(char *path, size_t path_size, const char *root,
                                 const char *relative)
{
  int result;

  result = snprintf(path, path_size, "%s/%s", root, relative);
  return result >= 0 && (size_t)result < path_size;
}

static bool spec_test_make_directory(const char *root, const char *relative, char *error,
                                     size_t error_size)
{
  char path[PATH_MAX];
  char message[PATH_MAX + 80];

  if (!spec_test_build_path(path, sizeof(path), root, relative))
  {
    spec_test_set_error(error, error_size, "special-procedure sandbox path is too long");
    return false;
  }

  if (mkdir(path, 0700) == 0)
    return true;

  snprintf(message, sizeof(message), "unable to create sandbox directory %s: %s", relative,
           strerror(errno));
  spec_test_set_error(error, error_size, message);
  return false;
}

static bool spec_test_sandbox_path_is_safe(const char *sandbox)
{
  static const char prefix[] = "/tmp/luminari-spec-registry-";
  const char *suffix;

  if (sandbox == NULL || strncmp(sandbox, prefix, sizeof(prefix) - 1) != 0)
    return false;

  suffix = sandbox + sizeof(prefix) - 1;
  return *suffix != '\0' && strchr(suffix, '/') == NULL;
}

bool spec_test_cleanup_sandbox(const char *sandbox, char *error, size_t error_size)
{
  static const char *const files[] = {"world/mob/12.new",
                                      "world/mob/12.mob",
                                      "world/obj/14.new",
                                      "world/obj/14.obj",
                                      "world/wld/14.new",
                                      "world/wld/14.wld",
                                      NULL};
  static const char *const directories[] = {"world/mob", "world/obj", "world/wld", "world", NULL};
  char path[PATH_MAX];
  bool success;
  int index;

  if (error != NULL && error_size > 0)
    *error = '\0';
  if (!spec_test_sandbox_path_is_safe(sandbox))
  {
    spec_test_set_error(error, error_size, "refusing to clean an unsafe test sandbox path");
    return false;
  }

  success = true;

  for (index = 0; files[index] != NULL; index++)
  {
    if (!spec_test_build_path(path, sizeof(path), sandbox, files[index]))
    {
      if (success)
        spec_test_set_error(error, error_size, "test sandbox file path is too long");
      success = false;
    }
    else if (unlink(path) != 0 && errno != ENOENT)
    {
      if (success)
        spec_test_set_error(error, error_size, "unable to remove a test sandbox file");
      success = false;
    }
  }

  for (index = 0; directories[index] != NULL; index++)
  {
    if (!spec_test_build_path(path, sizeof(path), sandbox, directories[index]))
    {
      if (success)
        spec_test_set_error(error, error_size, "test sandbox directory path is too long");
      success = false;
    }
    else if (rmdir(path) != 0 && errno != ENOENT)
    {
      if (success)
        spec_test_set_error(error, error_size, "unable to remove a test sandbox directory");
      success = false;
    }
  }

  if (rmdir(sandbox) != 0 && errno != ENOENT)
  {
    if (success)
      spec_test_set_error(error, error_size, "unable to remove the test sandbox root");
    success = false;
  }

  return success;
}

static bool spec_test_create_sandbox(struct spec_test_fixture *fixture, const char *sandbox,
                                     char *error, size_t error_size)
{
  const char *const directories[] = {"world", "world/mob", "world/obj", "world/wld", NULL};
  struct stat sandbox_stat;
  int index;

  if (sandbox != NULL)
  {
    if (!spec_test_sandbox_path_is_safe(sandbox) ||
        snprintf(fixture->sandbox, sizeof(fixture->sandbox), "%s", sandbox) >=
            (int)sizeof(fixture->sandbox))
    {
      spec_test_set_error(error, error_size, "invalid special-procedure sandbox path");
      return false;
    }
    if (stat(fixture->sandbox, &sandbox_stat) != 0 || !S_ISDIR(sandbox_stat.st_mode) ||
        sandbox_stat.st_uid != geteuid() || (sandbox_stat.st_mode & 0777) != 0700)
    {
      spec_test_set_error(error, error_size,
                          "special-procedure sandbox is not a private directory");
      return false;
    }
  }
  else
  {
    if (snprintf(fixture->sandbox, sizeof(fixture->sandbox),
                 "/tmp/luminari-spec-registry-XXXXXX") >= (int)sizeof(fixture->sandbox))
    {
      spec_test_set_error(error, error_size, "special-procedure sandbox template is too long");
      return false;
    }

    if (mkdtemp(fixture->sandbox) == NULL)
    {
      spec_test_set_error(error, error_size, "unable to create special-procedure sandbox");
      return false;
    }
  }
  fixture->sandbox_created = true;

  for (index = 0; directories[index] != NULL; index++)
    if (!spec_test_make_directory(fixture->sandbox, directories[index], error, error_size))
      return false;

  return true;
}

static bool spec_test_read_saved_file(struct spec_test_fixture *fixture, const char *relative,
                                      char **contents, char *error, size_t error_size)
{
  char path[PATH_MAX];
  FILE *file;
  char *loaded;
  long length;
  size_t read_length;
  int close_result;
  bool success;

  if (!spec_test_build_path(path, sizeof(path), fixture->sandbox, relative))
  {
    spec_test_set_error(error, error_size, "saved special-procedure path is too long");
    return false;
  }

  file = fopen(path, "rb");
  if (file == NULL)
  {
    spec_test_set_error(error, error_size, "unable to open saved special-procedure fixture");
    return false;
  }

  success = fseek(file, 0, SEEK_END) == 0;
  length = success ? ftell(file) : -1;
  if (length < 0 || length > SPEC_TEST_MAX_SAVED_FILE || fseek(file, 0, SEEK_SET) != 0)
  {
    fclose(file);
    spec_test_set_error(error, error_size, "saved special-procedure fixture has invalid size");
    return false;
  }

  loaded = malloc((size_t)length + 1);
  if (loaded == NULL)
  {
    fclose(file);
    spec_test_set_error(error, error_size, "unable to allocate saved fixture buffer");
    return false;
  }

  read_length = fread(loaded, 1, (size_t)length, file);
  close_result = fclose(file);
  success = read_length == (size_t)length && close_result == 0;
  if (!success)
  {
    free(loaded);
    spec_test_set_error(error, error_size, "unable to read complete saved fixture");
    return false;
  }

  loaded[length] = '\0';
  free(*contents);
  *contents = loaded;
  return true;
}

static FILE *spec_test_open_record(const char *record, char *error, size_t error_size)
{
  FILE *file;

  file = tmpfile();
  if (file == NULL)
  {
    spec_test_set_error(error, error_size, "unable to create in-memory world record");
    return NULL;
  }

  if (fputs(record, file) == EOF || fflush(file) != 0 || fseek(file, 0, SEEK_SET) != 0)
  {
    fclose(file);
    spec_test_set_error(error, error_size, "unable to initialize in-memory world record");
    return NULL;
  }

  return file;
}

static void spec_test_free_loaded_data(struct spec_test_fixture *fixture)
{
  if (fixture->mobile_loaded)
  {
    free_mobile_strings(&fixture->test_mob_proto[0]);
    fixture->mobile_loaded = false;
  }

  if (fixture->object_loaded)
  {
    free_object_strings(&fixture->test_obj_proto[0]);
    fixture->object_loaded = false;
  }

  if (fixture->room_loaded)
  {
    free_room_strings(&fixture->test_world[0]);
    free_trail_data_list(fixture->test_world[0].trail_tracks);
    fixture->test_world[0].trail_tracks = NULL;
    fixture->room_loaded = false;
  }
}

static void spec_test_restore_globals(const struct spec_test_fixture *fixture)
{
  world = fixture->saved_world;
  mob_proto = fixture->saved_mob_proto;
  mob_index = fixture->saved_mob_index;
  obj_proto = fixture->saved_obj_proto;
  obj_index = fixture->saved_obj_index;
  zone_table = fixture->saved_zone_table;
  top_of_world = fixture->saved_top_of_world;
  top_of_mobt = fixture->saved_top_of_mobt;
  top_of_objt = fixture->saved_top_of_objt;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  CONFIG_DIAGONAL_DIRS = fixture->saved_diagonal_dirs;
  CONFIG_WILDERNESS_SYSTEM = fixture->saved_wilderness_system;
}

static void spec_test_reset_output(struct spec_test_fixture *fixture)
{
  fixture->output_buffer[0] = '\0';
  fixture->output_block.text = fixture->output_buffer;
  fixture->output_block.next = NULL;
  fixture->descriptor.output = fixture->output_buffer;
  fixture->descriptor.large_outbuf = &fixture->output_block;
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = LARGE_BUFSIZE - 1;
}

static bool spec_test_initialize_descriptor(struct spec_test_fixture *fixture)
{
  memset(&fixture->descriptor, 0, sizeof(fixture->descriptor));
  memset(&fixture->builder, 0, sizeof(fixture->builder));
  memset(&fixture->builder_specials, 0, sizeof(fixture->builder_specials));
  memset(&fixture->olc, 0, sizeof(fixture->olc));

  fixture->builder.player_specials = &fixture->builder_specials;
  fixture->builder.desc = &fixture->descriptor;
  fixture->descriptor.character = &fixture->builder;
  fixture->descriptor.olc = &fixture->olc;
  fixture->descriptor.pProtocol = ProtocolCreate();
  spec_test_reset_output(fixture);
  return fixture->descriptor.pProtocol != NULL;
}

static struct spec_test_fixture *spec_test_fixture_create_internal(const char *sandbox, char *error,
                                                                   size_t error_size)
{
  struct spec_test_fixture *fixture;
  char cleanup_error[256];

  if (error != NULL && error_size > 0)
    *error = '\0';

  fixture = calloc(1, sizeof(*fixture));
  if (fixture == NULL)
  {
    spec_test_set_error(error, error_size, "unable to allocate special-procedure fixture");
    return NULL;
  }

  if (getcwd(fixture->original_cwd, sizeof(fixture->original_cwd)) == NULL)
  {
    spec_test_set_error(error, error_size, "unable to capture the test working directory");
    free(fixture);
    return NULL;
  }

  fixture->saved_world = world;
  fixture->saved_mob_proto = mob_proto;
  fixture->saved_mob_index = mob_index;
  fixture->saved_obj_proto = obj_proto;
  fixture->saved_obj_index = obj_index;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_diagonal_dirs = CONFIG_DIAGONAL_DIRS;
  fixture->saved_wilderness_system = CONFIG_WILDERNESS_SYSTEM;

  if (!spec_test_create_sandbox(fixture, sandbox, error, error_size))
  {
    spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error));
    return NULL;
  }

  fixture->test_zone_table[0].number = SPEC_TEST_MOBILE_ZONE;
  fixture->test_zone_table[0].bot = 1200;
  fixture->test_zone_table[0].top = 1299;
  fixture->test_zone_table[1].number = SPEC_TEST_OBJECT_ROOM_ZONE;
  fixture->test_zone_table[1].bot = 1400;
  fixture->test_zone_table[1].top = 1499;

  world = fixture->test_world;
  mob_proto = fixture->test_mob_proto;
  mob_index = fixture->test_mob_index;
  obj_proto = fixture->test_obj_proto;
  obj_index = fixture->test_obj_index;
  zone_table = fixture->test_zone_table;
  top_of_world = NOWHERE;
  top_of_mobt = NOBODY;
  top_of_objt = NOTHING;
  top_of_zone_table = 1;
  CONFIG_DIAGONAL_DIRS = 0;
  CONFIG_WILDERNESS_SYSTEM = 0;
  if (!spec_test_initialize_descriptor(fixture))
  {
    spec_test_set_error(error, error_size, "unable to initialize descriptor protocol state");
    spec_test_fixture_destroy(fixture, cleanup_error, sizeof(cleanup_error));
    return NULL;
  }

  return fixture;
}

struct spec_test_fixture *spec_test_fixture_create(char *error, size_t error_size)
{
  return spec_test_fixture_create_internal(NULL, error, error_size);
}

struct spec_test_fixture *spec_test_fixture_create_at(const char *sandbox, char *error,
                                                      size_t error_size)
{
  if (sandbox == NULL)
  {
    spec_test_set_error(error, error_size, "cannot create a fixture at a null sandbox path");
    return NULL;
  }

  return spec_test_fixture_create_internal(sandbox, error, error_size);
}

bool spec_test_fixture_destroy(struct spec_test_fixture *fixture, char *error, size_t error_size)
{
  char cleanup_error[256];
  bool success;
  int owner;

  if (error != NULL && error_size > 0)
    *error = '\0';
  if (fixture == NULL)
    return true;

  success = true;
  if (*fixture->original_cwd != '\0' && chdir(fixture->original_cwd) != 0)
  {
    spec_test_set_error(error, error_size,
                        "unable to restore working directory during fixture cleanup");
    success = false;
  }

  spec_test_free_loaded_data(fixture);
  spec_test_restore_globals(fixture);

  if (fixture->descriptor.pProtocol != NULL)
  {
    ProtocolDestroy(fixture->descriptor.pProtocol);
    fixture->descriptor.pProtocol = NULL;
  }

  for (owner = 0; owner < SPEC_TEST_OWNER_COUNT; owner++)
    free(fixture->saved_text[owner]);

  if (fixture->sandbox_created &&
      !spec_test_cleanup_sandbox(fixture->sandbox, cleanup_error, sizeof(cleanup_error)))
  {
    if (success)
      spec_test_set_error(error, error_size, cleanup_error);
    success = false;
  }
  fixture->sandbox_created = false;
  free(fixture);
  return success;
}

bool spec_test_fixture_load_named_bindings(struct spec_test_fixture *fixture, char *error,
                                           size_t error_size)
{
  FILE *record;

  if (error != NULL && error_size > 0)
    *error = '\0';
  if (fixture == NULL)
  {
    spec_test_set_error(error, error_size, "cannot load bindings into a null fixture");
    return false;
  }
  if (fixture->mobile_loaded || fixture->object_loaded || fixture->room_loaded)
  {
    spec_test_set_error(error, error_size, "named binding fixture cannot be loaded twice");
    return false;
  }

  record = spec_test_open_record(spec_test_mobile_record, error, error_size);
  if (record == NULL)
    return false;
  parse_mobile(record, SPEC_TEST_MOBILE_VNUM);
  fixture->mobile_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close mobile fixture record");
    return false;
  }

  record = spec_test_open_record(spec_test_object_record, error, error_size);
  if (record == NULL)
    return false;
  parse_object(record, SPEC_TEST_OBJECT_VNUM);
  fixture->object_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close object fixture record");
    return false;
  }

  record = spec_test_open_record(spec_test_room_record, error, error_size);
  if (record == NULL)
    return false;
  parse_room(record, SPEC_TEST_ROOM_VNUM, "special-procedure test fixture");
  fixture->room_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close room fixture record");
    return false;
  }

  return true;
}

SPECIAL_DECL(*spec_test_fixture_loaded_handler(const struct spec_test_fixture *fixture,
                                               enum spec_test_owner owner))
{
  if (fixture == NULL)
    return NULL;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    return fixture->mobile_loaded ? fixture->test_mob_index[0].func : NULL;
  case SPEC_TEST_OWNER_OBJECT:
    return fixture->object_loaded ? fixture->test_obj_index[0].func : NULL;
  case SPEC_TEST_OWNER_ROOM:
    return fixture->room_loaded ? fixture->test_world[0].func : NULL;
  case SPEC_TEST_OWNER_COUNT:
    return NULL;
  }

  return NULL;
}

bool spec_test_fixture_save_named_bindings(struct spec_test_fixture *fixture, char *error,
                                           size_t error_size)
{
  int mobile_result;
  int object_result;
  int room_result;
  int restore_result;

  if (error != NULL && error_size > 0)
    *error = '\0';
  if (fixture == NULL || !fixture->mobile_loaded || !fixture->object_loaded ||
      !fixture->room_loaded || !fixture->sandbox_created)
  {
    spec_test_set_error(error, error_size, "cannot save an incomplete named binding fixture");
    return false;
  }

  if (chdir(fixture->sandbox) != 0)
  {
    spec_test_set_error(error, error_size, "unable to enter special-procedure sandbox");
    return false;
  }

  mobile_result = save_mobiles(0);
  object_result = save_objects(1);
  room_result = save_rooms(1);
  restore_result = chdir(fixture->original_cwd);

  if (restore_result != 0)
  {
    spec_test_set_error(error, error_size, "unable to restore working directory after OLC save");
    return false;
  }
  if (mobile_result <= 0 || object_result == FALSE || room_result == FALSE)
  {
    spec_test_set_error(error, error_size, "a production OLC writer rejected the named fixture");
    return false;
  }

  if (!spec_test_read_saved_file(fixture, "world/mob/12.mob",
                                 &fixture->saved_text[SPEC_TEST_OWNER_MOBILE], error, error_size) ||
      !spec_test_read_saved_file(fixture, "world/obj/14.obj",
                                 &fixture->saved_text[SPEC_TEST_OWNER_OBJECT], error, error_size) ||
      !spec_test_read_saved_file(fixture, "world/wld/14.wld",
                                 &fixture->saved_text[SPEC_TEST_OWNER_ROOM], error, error_size))
    return false;

  return true;
}

const char *spec_test_fixture_saved_text(const struct spec_test_fixture *fixture,
                                         enum spec_test_owner owner)
{
  if (fixture == NULL || owner < SPEC_TEST_OWNER_MOBILE || owner >= SPEC_TEST_OWNER_COUNT)
    return NULL;

  return fixture->saved_text[owner];
}

bool spec_test_fixture_reset_olc(struct spec_test_fixture *fixture, enum spec_test_owner owner,
                                 SPECIAL_DECL(*initial_handler))
{
  if (fixture == NULL)
    return false;

  memset(&fixture->olc, 0, sizeof(fixture->olc));
  fixture->descriptor.olc = &fixture->olc;
  spec_test_reset_output(fixture);

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    if (!fixture->mobile_loaded)
      return false;
    fixture->olc.mode = MEDIT_SPEC_PROC;
    fixture->olc.number = SPEC_TEST_MOBILE_VNUM;
    fixture->olc.mob = &fixture->test_mob_proto[0];
    fixture->olc.specmob = initial_handler;
    break;
  case SPEC_TEST_OWNER_OBJECT:
    if (!fixture->object_loaded)
      return false;
    fixture->olc.mode = OEDIT_SPEC_PROC;
    fixture->olc.number = SPEC_TEST_OBJECT_VNUM;
    fixture->olc.obj = &fixture->test_obj_proto[0];
    fixture->olc.specobj = initial_handler;
    break;
  case SPEC_TEST_OWNER_ROOM:
    if (!fixture->room_loaded)
      return false;
    fixture->olc.mode = REDIT_SPEC_PROC;
    fixture->olc.zone_num = 1;
    fixture->olc.number = SPEC_TEST_ROOM_VNUM;
    fixture->olc.room = &fixture->test_world[0];
    fixture->olc.specroom = initial_handler;
    break;
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return true;
}

bool spec_test_fixture_parse_olc(struct spec_test_fixture *fixture, enum spec_test_owner owner,
                                 const char *argument)
{
  char mutable_argument[MAX_INPUT_LENGTH];
  int result;

  if (fixture == NULL || argument == NULL)
    return false;

  result = snprintf(mutable_argument, sizeof(mutable_argument), "%s", argument);
  if (result < 0 || (size_t)result >= sizeof(mutable_argument))
    return false;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    medit_parse(&fixture->descriptor, mutable_argument);
    return true;
  case SPEC_TEST_OWNER_OBJECT:
    oedit_parse(&fixture->descriptor, mutable_argument);
    return true;
  case SPEC_TEST_OWNER_ROOM:
    redit_parse(&fixture->descriptor, mutable_argument);
    return true;
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return false;
}

SPECIAL_DECL(*spec_test_fixture_olc_handler(const struct spec_test_fixture *fixture,
                                            enum spec_test_owner owner))
{
  if (fixture == NULL)
    return NULL;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    return fixture->olc.specmob;
  case SPEC_TEST_OWNER_OBJECT:
    return fixture->olc.specobj;
  case SPEC_TEST_OWNER_ROOM:
    return fixture->olc.specroom;
  case SPEC_TEST_OWNER_COUNT:
    return NULL;
  }

  return NULL;
}

int spec_test_fixture_olc_changed(const struct spec_test_fixture *fixture)
{
  return fixture != NULL ? fixture->olc.value : 0;
}

const char *spec_test_fixture_olc_output(const struct spec_test_fixture *fixture)
{
  return fixture != NULL ? fixture->descriptor.output : NULL;
}
