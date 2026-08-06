#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"

#include "../../src/db.h"
#include "../../src/olc/genmob.h"
#include "../../src/olc/genobj.h"
#include "../../src/olc/genwld.h"
#include "../../src/olc/oasis.h"
#include "../../src/olc/spec_menu.h"
#include "../../src/net/protocol.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/spec_procs.h"
#include "test_spec_fixtures.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SPEC_TEST_MOBILE_VNUM 1201
#define SPEC_TEST_OBJECT_VNUM 1402
#define SPEC_TEST_ROOM_VNUM 1403
#define SPEC_TEST_MOBILE_ZONE 12
#define SPEC_TEST_OBJECT_ROOM_ZONE 14
#define SPEC_TEST_MAX_SAVED_FILE (1024L * 1024L)
#define SPEC_TEST_CHILD_TIMEOUT 30
#define SPEC_TEST_CHILD_ERROR_SIZE 512

struct spec_test_child_result
{
  int success;
  char error[SPEC_TEST_CHILD_ERROR_SIZE];
};

static const char spec_test_mobile_record_format[] = "postmaster~\n"
                                                     "the test postmaster~\n"
                                                     "The test postmaster is here.\n~\n"
                                                     "A test postmaster.\n~\n"
                                                     "0 0 0 0 0 0 0 0 0 E\n"
                                                     "1 20 10 1d1+0 1d1+0\n"
                                                     "0 0\n"
                                                     "8 8 0\n"
                                                     "SpecProc: %s\n"
                                                     "DR_MOD: 0\n"
                                                     "E\n"
                                                     "$~\n";

static const char spec_test_object_record_format[] = "test vessel~\n"
                                                     "a test vessel~\n"
                                                     "A test vessel is here.~\n"
                                                     "~\n"
                                                     "12 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                                                     "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0\n"
                                                     "1 0 0 1 0\n"
                                                     "Z\n"
                                                     "%s\n"
                                                     "$~\n";

static const char spec_test_room_record_format[] = "A Test Vessel Room~\n"
                                                   "A test vessel room.\n~\n"
                                                   "14 0 0 0 0 0\n"
                                                   "C\n"
                                                   "0 0\n"
                                                   "Z\n"
                                                   "%s\n"
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
  bool olc_subject_owned;
  enum spec_test_owner olc_subject_owner;
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

static bool spec_test_write_all(int descriptor, const void *buffer, size_t length)
{
  const char *cursor;
  ssize_t written;

  cursor = buffer;
  while (length > 0)
  {
    written = write(descriptor, cursor, length);
    if (written < 0 && errno == EINTR)
      continue;
    if (written <= 0)
      return false;
    cursor += written;
    length -= (size_t)written;
  }

  return true;
}

bool spec_test_run_isolated_with_path(spec_test_isolated_scenario scenario, char *sandbox_result,
                                      size_t sandbox_result_size, char *error, size_t error_size)
{
  struct spec_test_child_result result;
  char cleanup_error[SPEC_TEST_CHILD_ERROR_SIZE];
  char sandbox[PATH_MAX];
  int result_pipe[2];
  int child_status;
  pid_t child_pid;
  pid_t waited_pid;
  size_t received;
  ssize_t read_result;

  memset(&result, 0, sizeof(result));
  if (scenario == NULL)
  {
    spec_test_set_error(error, error_size, "cannot run a null isolated test scenario");
    return false;
  }
  if (snprintf(sandbox, sizeof(sandbox), "/tmp/luminari-spec-registry-run-XXXXXX") >=
          (int)sizeof(sandbox) ||
      mkdtemp(sandbox) == NULL)
  {
    spec_test_set_error(error, error_size, "unable to create isolated test sandbox");
    return false;
  }
  if (sandbox_result != NULL &&
      snprintf(sandbox_result, sandbox_result_size, "%s", sandbox) >= (int)sandbox_result_size)
  {
    spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error));
    spec_test_set_error(error, error_size, "isolated test sandbox result buffer is too small");
    return false;
  }
  if (pipe(result_pipe) != 0)
  {
    if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
    {
      spec_test_set_error(error, error_size, cleanup_error);
      return false;
    }
    spec_test_set_error(error, error_size, "unable to create isolated test result pipe");
    return false;
  }

  child_pid = fork();
  if (child_pid < 0)
  {
    close(result_pipe[0]);
    close(result_pipe[1]);
    if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
    {
      spec_test_set_error(error, error_size, cleanup_error);
      return false;
    }
    spec_test_set_error(error, error_size, "unable to fork isolated parser test");
    return false;
  }

  if (child_pid == 0)
  {
    close(result_pipe[0]);
    alarm(SPEC_TEST_CHILD_TIMEOUT);
    result.success = scenario(sandbox, result.error, sizeof(result.error));
    if (!result.success && result.error[0] == '\0')
      spec_test_set_error(result.error, sizeof(result.error), "isolated scenario failed");
    if (!spec_test_write_all(result_pipe[1], &result, sizeof(result)))
      _exit(2);
    close(result_pipe[1]);
    _exit(result.success ? EXIT_SUCCESS : EXIT_FAILURE);
  }

  close(result_pipe[1]);
  received = 0;
  while (received < sizeof(result))
  {
    read_result = read(result_pipe[0], (char *)&result + received, sizeof(result) - received);
    if (read_result < 0 && errno == EINTR)
      continue;
    if (read_result <= 0)
      break;
    received += (size_t)read_result;
  }
  close(result_pipe[0]);

  do
  {
    waited_pid = waitpid(child_pid, &child_status, 0);
  } while (waited_pid < 0 && errno == EINTR);

  if (!spec_test_cleanup_sandbox(sandbox, cleanup_error, sizeof(cleanup_error)))
  {
    spec_test_set_error(error, error_size, cleanup_error);
    return false;
  }
  if (waited_pid != child_pid || received != sizeof(result))
  {
    spec_test_set_error(error, error_size, "isolated parser test exited before reporting a result");
    return false;
  }
  if (!WIFEXITED(child_status))
  {
    spec_test_set_error(error, error_size, "isolated parser test did not exit normally");
    return false;
  }
  if (WEXITSTATUS(child_status) != EXIT_SUCCESS || !result.success)
  {
    spec_test_set_error(error, error_size,
                        result.error[0] != '\0' ? result.error : "isolated parser test failed");
    return false;
  }

  return true;
}

bool spec_test_run_isolated(spec_test_isolated_scenario scenario, char *error, size_t error_size)
{
  return spec_test_run_isolated_with_path(scenario, NULL, 0, error, error_size);
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

static FILE *spec_test_open_named_record(const char *format, const char *name, char *error,
                                         size_t error_size)
{
  char record[MAX_STRING_LENGTH];
  int length;

  if (format == NULL || name == NULL || strchr(name, '\n') != NULL || strchr(name, '\r') != NULL)
  {
    spec_test_set_error(error, error_size, "invalid named world record input");
    return NULL;
  }

  length = snprintf(record, sizeof(record), format, name);
  if (length < 0 || (size_t)length >= sizeof(record))
  {
    spec_test_set_error(error, error_size, "named world record is too large");
    return NULL;
  }

  return spec_test_open_record(record, error, error_size);
}

static FILE *spec_test_open_saved_record(const struct spec_test_fixture *fixture,
                                         const char *relative, const char *expected_header,
                                         char *error, size_t error_size)
{
  char path[PATH_MAX];
  char header[64];
  FILE *file;

  if (fixture == NULL || relative == NULL || expected_header == NULL ||
      !spec_test_build_path(path, sizeof(path), fixture->sandbox, relative))
  {
    spec_test_set_error(error, error_size, "saved world record path is invalid");
    return NULL;
  }

  file = fopen(path, "r");
  if (file == NULL)
  {
    spec_test_set_error(error, error_size, "unable to open a saved world record for reload");
    return NULL;
  }
  if (fgets(header, sizeof(header), file) == NULL ||
      strncmp(header, expected_header, strlen(expected_header)) != 0 ||
      (header[strlen(expected_header)] != '\n' && header[strlen(expected_header)] != '\r'))
  {
    fclose(file);
    spec_test_set_error(error, error_size, "saved world record has an unexpected header");
    return NULL;
  }

  return file;
}

static void spec_test_free_loaded_data(struct spec_test_fixture *fixture)
{
  if (fixture->mobile_loaded)
  {
    spec_binding_free(&fixture->test_mob_index[0].spec_binding);
    free_mobile_strings(&fixture->test_mob_proto[0]);
    fixture->mobile_loaded = false;
  }

  if (fixture->object_loaded)
  {
    spec_binding_free(&fixture->test_obj_index[0].spec_binding);
    free_object_strings(&fixture->test_obj_proto[0]);
    fixture->object_loaded = false;
  }

  if (fixture->room_loaded)
  {
    spec_binding_free(&fixture->test_world[0].spec_binding);
    free_room_strings(&fixture->test_world[0]);
    free_trail_data_list(fixture->test_world[0].trail_tracks);
    fixture->test_world[0].trail_tracks = NULL;
    fixture->room_loaded = false;
  }
}

static void spec_test_release_olc_subject(struct spec_test_fixture *fixture)
{
  if (fixture == NULL || !fixture->olc_subject_owned)
    return;

  switch (fixture->olc_subject_owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    free_mobile(fixture->olc.mob);
    fixture->olc.mob = NULL;
    break;
  case SPEC_TEST_OWNER_OBJECT:
    if (fixture->olc.obj != NULL)
    {
      free_object_strings(fixture->olc.obj);
      free(fixture->olc.obj);
      fixture->olc.obj = NULL;
    }
    break;
  case SPEC_TEST_OWNER_ROOM:
    if (fixture->olc.room != NULL)
    {
      if (CONFIG_WILDERNESS_SYSTEM != 2 && fixture->olc.room->trail_tracks != NULL)
      {
        free_trail_data_list(fixture->olc.room->trail_tracks);
        fixture->olc.room->trail_tracks = NULL;
      }
      free_room(fixture->olc.room);
      fixture->olc.room = NULL;
    }
    break;
  case SPEC_TEST_OWNER_COUNT:
    break;
  }

  fixture->olc_subject_owned = false;
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

  spec_test_release_olc_subject(fixture);
  spec_binding_free(&fixture->olc.specmob_binding);
  spec_binding_free(&fixture->olc.specobj_binding);
  spec_binding_free(&fixture->olc.specroom_binding);
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
  return spec_test_fixture_load_binding_names(fixture, "Postmaster", "Greyhawk Ship",
                                              "Greyhawk Ship Commands", error, error_size);
}

bool spec_test_fixture_load_binding_names(struct spec_test_fixture *fixture,
                                          const char *mobile_name, const char *object_name,
                                          const char *room_name, char *error, size_t error_size)
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

  record =
      spec_test_open_named_record(spec_test_mobile_record_format, mobile_name, error, error_size);
  if (record == NULL)
    return false;
  parse_mobile(record, SPEC_TEST_MOBILE_VNUM);
  fixture->mobile_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close mobile fixture record");
    return false;
  }

  record =
      spec_test_open_named_record(spec_test_object_record_format, object_name, error, error_size);
  if (record == NULL)
    return false;
  parse_object(record, SPEC_TEST_OBJECT_VNUM);
  fixture->object_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close object fixture record");
    return false;
  }

  record = spec_test_open_named_record(spec_test_room_record_format, room_name, error, error_size);
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

bool spec_test_fixture_load_saved_bindings(struct spec_test_fixture *fixture, char *error,
                                           size_t error_size)
{
  FILE *record;

  if (error != NULL && error_size > 0)
    *error = '\0';
  if (fixture == NULL)
  {
    spec_test_set_error(error, error_size, "cannot reload bindings into a null fixture");
    return false;
  }
  if (fixture->mobile_loaded || fixture->object_loaded || fixture->room_loaded)
  {
    spec_test_set_error(error, error_size, "saved binding fixture cannot be loaded twice");
    return false;
  }
  if (!spec_test_read_saved_file(fixture, "world/mob/12.mob",
                                 &fixture->saved_text[SPEC_TEST_OWNER_MOBILE], error, error_size) ||
      !spec_test_read_saved_file(fixture, "world/obj/14.obj",
                                 &fixture->saved_text[SPEC_TEST_OWNER_OBJECT], error, error_size) ||
      !spec_test_read_saved_file(fixture, "world/wld/14.wld",
                                 &fixture->saved_text[SPEC_TEST_OWNER_ROOM], error, error_size))
    return false;

  record = spec_test_open_saved_record(fixture, "world/mob/12.mob", "#1201", error, error_size);
  if (record == NULL)
    return false;
  parse_mobile(record, SPEC_TEST_MOBILE_VNUM);
  fixture->mobile_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close reloaded mobile world record");
    return false;
  }

  record = spec_test_open_saved_record(fixture, "world/obj/14.obj", "#1402", error, error_size);
  if (record == NULL)
    return false;
  parse_object(record, SPEC_TEST_OBJECT_VNUM);
  fixture->object_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close reloaded object world record");
    return false;
  }
  record = spec_test_open_saved_record(fixture, "world/wld/14.wld", "#1403", error, error_size);
  if (record == NULL)
    return false;
  parse_room(record, SPEC_TEST_ROOM_VNUM, "saved special-procedure round-trip fixture");
  fixture->room_loaded = true;
  if (fclose(record) != 0)
  {
    spec_test_set_error(error, error_size, "unable to close reloaded room world record");
    return false;
  }

  return true;
}

const struct spec_binding *spec_test_fixture_loaded_binding(const struct spec_test_fixture *fixture,
                                                            enum spec_test_owner owner)
{
  if (fixture == NULL)
    return NULL;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    return fixture->mobile_loaded ? fixture->test_mob_index[0].spec_binding : NULL;
  case SPEC_TEST_OWNER_OBJECT:
    return fixture->object_loaded ? fixture->test_obj_index[0].spec_binding : NULL;
  case SPEC_TEST_OWNER_ROOM:
    return fixture->room_loaded ? fixture->test_world[0].spec_binding : NULL;
  case SPEC_TEST_OWNER_COUNT:
    return NULL;
  }

  return NULL;
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

bool spec_test_fixture_set_loaded_handler(struct spec_test_fixture *fixture,
                                          enum spec_test_owner owner, SPECIAL_DECL(*handler))
{
  if (fixture == NULL)
    return false;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    if (!fixture->mobile_loaded)
      return false;
    fixture->test_mob_index[0].func = handler;
    return true;
  case SPEC_TEST_OWNER_OBJECT:
    if (!fixture->object_loaded)
      return false;
    fixture->test_obj_index[0].func = handler;
    return true;
  case SPEC_TEST_OWNER_ROOM:
    if (!fixture->room_loaded)
      return false;
    fixture->test_world[0].func = handler;
    return true;
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return false;
}

bool spec_test_fixture_discard_loaded_binding(struct spec_test_fixture *fixture,
                                              enum spec_test_owner owner)
{
  if (fixture == NULL)
    return false;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    if (!fixture->mobile_loaded)
      return false;
    spec_binding_free(&fixture->test_mob_index[0].spec_binding);
    return true;
  case SPEC_TEST_OWNER_OBJECT:
    if (!fixture->object_loaded)
      return false;
    spec_binding_free(&fixture->test_obj_index[0].spec_binding);
    return true;
  case SPEC_TEST_OWNER_ROOM:
    if (!fixture->room_loaded)
      return false;
    spec_binding_free(&fixture->test_world[0].spec_binding);
    return true;
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return false;
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

  spec_test_release_olc_subject(fixture);
  spec_binding_free(&fixture->olc.specmob_binding);
  spec_binding_free(&fixture->olc.specobj_binding);
  spec_binding_free(&fixture->olc.specroom_binding);
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

bool spec_test_fixture_setup_existing_olc(struct spec_test_fixture *fixture,
                                          enum spec_test_owner owner, char *error,
                                          size_t error_size)
{
  if (error != NULL && error_size > 0)
    error[0] = '\0';
  if (fixture == NULL)
  {
    spec_test_set_error(error, error_size, "cannot set up OLC on a null fixture");
    return false;
  }

  spec_test_release_olc_subject(fixture);
  spec_binding_free(&fixture->olc.specmob_binding);
  spec_binding_free(&fixture->olc.specobj_binding);
  spec_binding_free(&fixture->olc.specroom_binding);
  memset(&fixture->olc, 0, sizeof(fixture->olc));
  fixture->descriptor.olc = &fixture->olc;
  spec_test_reset_output(fixture);
  fixture->olc_subject_owner = owner;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    if (!fixture->mobile_loaded)
      break;
    fixture->olc.number = SPEC_TEST_MOBILE_VNUM;
    medit_setup_existing(&fixture->descriptor, 0, QMODE_NONE);
    fixture->olc.mode = MEDIT_SPEC_PROC;
    fixture->olc_subject_owned = fixture->olc.mob != NULL;
    return fixture->olc_subject_owned;
  case SPEC_TEST_OWNER_OBJECT:
    if (!fixture->object_loaded)
      break;
    fixture->olc.number = SPEC_TEST_OBJECT_VNUM;
    oedit_setup_existing(&fixture->descriptor, 0, QMODE_NONE);
    fixture->olc.mode = OEDIT_SPEC_PROC;
    fixture->olc_subject_owned = fixture->olc.obj != NULL;
    return fixture->olc_subject_owned;
  case SPEC_TEST_OWNER_ROOM:
    if (!fixture->room_loaded)
      break;
    fixture->olc.zone_num = 1;
    fixture->olc.number = SPEC_TEST_ROOM_VNUM;
    redit_setup_existing(&fixture->descriptor, 0, QMODE_NONE);
    fixture->olc.mode = REDIT_SPEC_PROC;
    fixture->olc_subject_owned = fixture->olc.room != NULL;
    return fixture->olc_subject_owned;
  case SPEC_TEST_OWNER_COUNT:
    break;
  }

  spec_test_set_error(error, error_size, "requested OLC prototype is not loaded");
  return false;
}

bool spec_test_fixture_save_current_olc(struct spec_test_fixture *fixture,
                                        enum spec_test_owner owner)
{
  if (fixture == NULL || !fixture->olc_subject_owned || fixture->olc_subject_owner != owner)
    return false;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    medit_save_internally(&fixture->descriptor);
    return true;
  case SPEC_TEST_OWNER_OBJECT:
    oedit_save_internally(&fixture->descriptor);
    return true;
  case SPEC_TEST_OWNER_ROOM:
    redit_save_internally(&fixture->descriptor);
    return true;
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return false;
}

const struct spec_binding *spec_test_fixture_olc_binding(const struct spec_test_fixture *fixture,
                                                         enum spec_test_owner owner)
{
  if (fixture == NULL)
    return NULL;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    return fixture->olc.specmob_binding;
  case SPEC_TEST_OWNER_OBJECT:
    return fixture->olc.specobj_binding;
  case SPEC_TEST_OWNER_ROOM:
    return fixture->olc.specroom_binding;
  case SPEC_TEST_OWNER_COUNT:
    return NULL;
  }

  return NULL;
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

bool spec_test_fixture_open_olc_menu(struct spec_test_fixture *fixture, enum spec_test_owner owner)
{
  static const int owner_main_modes[SPEC_TEST_OWNER_COUNT] = {
      MEDIT_MAIN_MENU,
      OEDIT_MAIN_MENU,
      REDIT_MAIN_MENU,
  };
  int owner_index;

  if (fixture == NULL)
    return false;

  owner_index = (int)owner;
  if (owner_index < 0 || owner_index >= SPEC_TEST_OWNER_COUNT)
    return false;

  OLC_MODE(&fixture->descriptor) = owner_main_modes[owner_index];
  return spec_test_fixture_parse_olc(fixture, owner, "Z");
}

bool spec_test_fixture_display_olc_menu(struct spec_test_fixture *fixture, spec_owner_mask owner)
{
  if (fixture == NULL)
    return false;

  spec_test_reset_output(fixture);
  spec_olc_display_menu(&fixture->descriptor, owner);
  return true;
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

bool spec_test_fixture_activation_enabled(const struct spec_test_fixture *fixture,
                                          enum spec_test_owner owner)
{
  if (fixture == NULL)
    return false;

  switch (owner)
  {
  case SPEC_TEST_OWNER_MOBILE:
    return fixture->mobile_loaded && IS_SET_AR(MOB_FLAGS(&fixture->test_mob_proto[0]), MOB_SPEC);
  case SPEC_TEST_OWNER_OBJECT:
    return fixture->object_loaded &&
           IS_SET_AR(GET_OBJ_EXTRA(&fixture->test_obj_proto[0]), ITEM_AUTOPROC);
  case SPEC_TEST_OWNER_ROOM:
  case SPEC_TEST_OWNER_COUNT:
    return false;
  }

  return false;
}
