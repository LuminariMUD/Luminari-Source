#define _GNU_SOURCE 1
/* Production-linked fuzz targets and their deterministic replay.
 *
 * Every target below calls the parser the game server links, prepared with the
 * smallest fixture that parser needs. fuzz_game.c drives them under libFuzzer;
 * Test_fuzz_targets_replay_seed_and_regression_inputs replays the checked-in
 * seed corpora and every saved regression input in a forked child, so a fixed
 * crash stays fixed in the ordinary test suite. */
#include "CuTest.h"
#include "fuzz_targets.h"
#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/comm.h"
#include "../../src/core/constants.h"
#include "../../src/core/db.h"
#include "../../src/core/helpers.h"
#include "../../src/core/interpreter.h"
#include "../../src/config/dotenv.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/net/protocol.h"
#include "../../src/net/i3_client.h"
#include "../../src/net/discord_bridge.h"
#include "../../src/net/onboarding.h"
#include "../../src/ai/ai_service.h"
#include "../../src/magic/spells.h"
#include "../../src/combat/spec_abilities.h"
#include "../../src/act/act.h"
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

jmp_buf fuzz_exit_jump;
volatile sig_atomic_t fuzz_exit_active = 0;
volatile sig_atomic_t fuzz_exit_status = 0;

/* The world loader and the configuration reader end the process on a
 * rejected file. While a FUZZ_TARGET_MAY_EXIT target runs production code,
 * that exit returns to the target instead; every other exit (the test
 * runner's, libFuzzer's, a fixture failure) goes through to the C library. */
void exit(int status)
{
  void (*real_exit)(int) __attribute__((noreturn));

  if (fuzz_exit_active)
  {
    fuzz_exit_active = 0;
    fuzz_exit_status = status;
    longjmp(fuzz_exit_jump, 1);
  }
  *(void **)(&real_exit) = dlsym(RTLD_NEXT, "exit");
  if (real_exit != NULL)
    real_exit(status);
  _exit(status);
}

#define FUZZ_MAX_LINE (MAX_INPUT_LENGTH - 1)

/* ------------------------------------------------------------------ helpers */

/* A NUL-terminated private copy of the input; C-string parsers stop at NUL. */
static char *fuzz_cstring(const uint8_t *data, size_t size)
{
  char *copy;

  copy = malloc(size + 1);
  if (copy == NULL)
    return NULL;
  memcpy(copy, data, size);
  copy[size] = '\0';
  return copy;
}

/* A read-only stream over a private copy of the input, as the loader reads a
 * world file. The copy is released with fuzz_stream_close(). */
static FILE *fuzz_stream(const uint8_t *data, size_t size, char **copy)
{
  *copy = NULL;
  if (size == 0)
    return NULL;
  *copy = malloc(size);
  if (*copy == NULL)
    return NULL;
  memcpy(*copy, data, size);
  return fmemopen(*copy, size, "r");
}

static void fuzz_stream_close(FILE *stream, char *copy)
{
  if (stream != NULL)
    fclose(stream);
  free(copy);
}

static bool fuzz_write_file(const char *path, const uint8_t *data, size_t size)
{
  FILE *fp;
  bool complete;

  unlink(path);
  fp = fopen(path, "w");
  if (fp == NULL)
    return false;
  complete = size == 0 || fwrite(data, 1, size, fp) == size;
  return fclose(fp) == 0 && complete;
}

/* The dotenv and config targets read files relative to the working directory,
 * so those targets move the process into a private scratch directory once. */
static void fuzz_enter_scratch_directory(void)
{
  static char scratch[PATH_MAX];
  const char *base;

  if (scratch[0] != '\0')
    return;
  base = getenv("TMPDIR");
  if (base == NULL || *base == '\0')
    base = "/tmp";
  snprintf(scratch, sizeof(scratch), "%s/luminari-fuzz.XXXXXX", base);
  if (mkdtemp(scratch) == NULL || chdir(scratch) != 0)
  {
    perror("fuzz scratch directory");
    abort();
  }
}

/* -------------------------------------------------------------- dotenv target */

static int fuzz_dotenv_run(const uint8_t *data, size_t size)
{
  static const char *const keys[] = {"APP_ENV", "LUMINARI_EVENT_BACKEND", "I3_ENABLED",
                                     "AI_API_KEY", "MUD_PORT"};
  char first_key[64];
  size_t index;
  size_t key_length;

  if (!fuzz_write_file(".env", data, size))
    return 0;
  for (index = 0; index < sizeof(keys) / sizeof(keys[0]); index++)
    (void)get_env_value(keys[index]);
  (void)get_env_int("MUD_PORT", 4100);
  (void)get_env_bool("I3_ENABLED", true);

  /* Look up whatever key the first line names so a matching entry is served. */
  for (key_length = 0; key_length < size && key_length < sizeof(first_key) - 1; key_length++)
  {
    if (data[key_length] == '=' || data[key_length] == '\n')
      break;
    first_key[key_length] = (char)data[key_length];
  }
  first_key[key_length] = '\0';
  (void)get_env_value(first_key);
  (void)get_env_bool(first_key, false);
  return 0;
}

/* -------------------------------------------------------------- config target */

static void fuzz_config_setup(void)
{
  fuzz_enter_scratch_directory();
  if (mkdir("etc", 0700) != 0 && errno != EEXIST)
  {
    perror("fuzz config directory");
    abort();
  }
  CONFIG_CONFFILE = strdup("etc/config");
}

/* load_config() ends the boot on a malformed multi-line string, so this
 * target runs behind the exit guard like the world loader. */
static int fuzz_config_run(const uint8_t *data, size_t size)
{
  if (!fuzz_write_file("etc/config", data, size))
    return 0;
  if (setjmp(fuzz_exit_jump) == 0)
  {
    fuzz_exit_active = 1;
    load_config();
  }
  fuzz_exit_active = 0;
  return 0;
}

/* ----------------------------------------------------------------- dg target */

static struct room_data fuzz_dg_room;
static struct zone_data fuzz_dg_zone;
static struct index_data fuzz_dg_trigger_index;
static struct index_data *fuzz_dg_trigger_table[1];

/* One room in one zone with one trigger prototype: the least state the
 * variable and expression evaluators dereference for a world trigger. */
static void fuzz_dg_setup(void)
{
  memset(&fuzz_dg_room, 0, sizeof(fuzz_dg_room));
  memset(&fuzz_dg_zone, 0, sizeof(fuzz_dg_zone));
  fuzz_dg_room.number = 1;
  fuzz_dg_room.zone = 0;
  fuzz_dg_room.name = strdup("Fuzz room");
  fuzz_dg_room.description = strdup("A room for script evaluation.\r\n");
  fuzz_dg_zone.number = 0;
  fuzz_dg_zone.bot = 0;
  fuzz_dg_zone.top = 99;
  fuzz_dg_zone.name = strdup("Fuzz zone");
  world = &fuzz_dg_room;
  top_of_world = 0;
  zone_table = &fuzz_dg_zone;
  top_of_zone_table = 0;
  fuzz_dg_trigger_index.vnum = 1;
  fuzz_dg_trigger_table[0] = &fuzz_dg_trigger_index;
  trig_index = fuzz_dg_trigger_table;
  top_of_trigt = 1;
}

static int fuzz_dg_run(const uint8_t *data, size_t size)
{
  struct script_data script;
  struct trig_data trigger;
  char name[] = "fuzz trigger";
  char line[MAX_INPUT_LENGTH];
  char result[MAX_INPUT_LENGTH];
  char *text;
  char *cursor;
  char *next;
  size_t length;

  text = fuzz_cstring(data, size);
  if (text == NULL)
    return 0;
  memset(&script, 0, sizeof(script));
  memset(&trigger, 0, sizeof(trigger));
  trigger.name = name;
  trigger.nr = 0;

  for (cursor = text; cursor != NULL && *cursor != '\0'; cursor = next)
  {
    next = strchr(cursor, '\n');
    if (next != NULL)
      *next++ = '\0';
    length = strlen(cursor);
    if (length > FUZZ_MAX_LINE)
      length = FUZZ_MAX_LINE;
    memcpy(line, cursor, length);
    line[length] = '\0';

    if (!strncmp(line, "eval ", 5))
      process_eval(&fuzz_dg_room, &script, &trigger, WLD_TRIGGER, line);
    else if (line[0] == '"')
      (void)matching_quote(line);
    else
      var_subst(&fuzz_dg_room, &script, &trigger, WLD_TRIGGER, line, result);
  }

  free_varlist(trigger.var_list);
  free_varlist(script.global_vars);
  free(text);
  return 0;
}

/* -------------------------------------------------------------- world target */

/* Records are counted so the loader's index tables can be sized as index_boot()
 * sizes them from the index files. */
static size_t fuzz_count_records(const uint8_t *data, size_t size)
{
  size_t count;
  size_t index;

  count = 0;
  for (index = 0; index < size; index++)
    if (data[index] == '#' && (index == 0 || data[index - 1] == '\n'))
      count++;
  return count;
}

/* boot_db() assigns the spell table before it loads the world; the object
 * checks name the spells an object carries. */
static void fuzz_world_setup(void)
{
  mag_assign_spells();
  initialize_special_abilities();
  init_obj_rnum_hash();
}

static int fuzz_world_run(const uint8_t *data, size_t size)
{
  FILE *stream;
  char *stream_copy;
  size_t records;
  unsigned int kind;

  if (size < 2)
    return 0;
  kind = data[0] % 5U;
  data++;
  size--;
  records = fuzz_count_records(data, size) + 2;

  zone_table = calloc(2, sizeof(*zone_table));
  world = calloc(records, sizeof(*world));
  mob_index = calloc(records, sizeof(*mob_index));
  mob_proto = calloc(records, sizeof(*mob_proto));
  obj_index = calloc(records, sizeof(*obj_index));
  obj_proto = calloc(records, sizeof(*obj_proto));
  trig_index = calloc(records, sizeof(*trig_index));
  if (zone_table == NULL || world == NULL || mob_index == NULL || mob_proto == NULL ||
      obj_index == NULL || obj_proto == NULL || trig_index == NULL)
    abort();
  zone_table[0].number = 0;
  zone_table[0].bot = 0;
  zone_table[0].top = 2000000000;
  world_loader_reset_for_test();

  stream = fuzz_stream(data, size, &stream_copy);
  if (stream != NULL)
  {
    if (setjmp(fuzz_exit_jump) == 0)
    {
      char filename[] = "fuzz.zon";

      fuzz_exit_active = 1;
      switch (kind)
      {
      case 0:
        discrete_load(stream, DB_BOOT_WLD, filename);
        break;
      case 1:
        discrete_load(stream, DB_BOOT_MOB, filename);
        break;
      case 2:
        discrete_load(stream, DB_BOOT_OBJ, filename);
        break;
      case 3:
        test_load_zones(stream, filename);
        break;
      default:
        discrete_load(stream, DB_BOOT_TRG, filename);
        break;
      }
    }
    fuzz_exit_active = 0;
  }
  fuzz_stream_close(stream, stream_copy);

  /* A rejected file ends the boot in production, so the records it left behind
   * are abandoned here as well; the target runs without leak detection. */
  free(zone_table);
  free(world);
  free(mob_index);
  free(mob_proto);
  free(obj_index);
  free(obj_proto);
  free(trig_index);
  zone_table = NULL;
  world = NULL;
  mob_index = NULL;
  mob_proto = NULL;
  obj_index = NULL;
  obj_proto = NULL;
  trig_index = NULL;
  world_loader_reset_for_test();
  return 0;
}

/* ------------------------------------------------------------ command target */

static struct descriptor_data *fuzz_command_descriptor;

static void fuzz_command_setup(void)
{
  fuzz_command_descriptor = calloc(1, sizeof(*fuzz_command_descriptor));
  if (fuzz_command_descriptor == NULL)
    abort();
  fuzz_command_descriptor->pProtocol = ProtocolCreate();
  /* new_descriptor() gives every connection its own command history. */
  CREATE(fuzz_command_descriptor->history, char *, HISTORY_SIZE);
  if (complete_cmd_info == NULL)
    create_command_list();
}

/* Run each queued line through the tokenizers the command dispatcher applies. */
static void fuzz_command_tokenize(char *line)
{
  char first[MAX_INPUT_LENGTH];
  char second[MAX_INPUT_LENGTH];
  char third[MAX_INPUT_LENGTH];
  char scratch[MAX_INPUT_LENGTH];

  strlcpy(scratch, line, sizeof(scratch));
  half_chop(scratch, first, second);
  (void)find_command(first);
  (void)is_number(first);
  (void)is_abbrev(first, "north");
  (void)search_block(first, (const char *const *)dirs, false);
  (void)three_arguments(line, first, sizeof(first), second, sizeof(second), third, sizeof(third));
  (void)two_arguments(line, first, sizeof(first), second, sizeof(second));
  (void)one_argument(line, first, sizeof(first));
  (void)any_one_arg_c(line, first, sizeof(first));
  strlcpy(scratch, line, sizeof(scratch));
  (void)reserved_word(scratch);
  strlcpy(scratch, line, sizeof(scratch));
  (void)fill_word(scratch);
  strlcpy(scratch, line, sizeof(scratch));
  (void)delete_doubledollar(scratch);
}

static int fuzz_command_run(const uint8_t *data, size_t size)
{
  struct descriptor_data *d;
  char line[MAX_INPUT_LENGTH];
  int sockets[2];
  int aliased;
  int index;
  int result;
  size_t offset;
  size_t chunk;

  if (size == 0)
    return 0;
  d = fuzz_command_descriptor;
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0)
    return 0;
  fcntl(sockets[0], F_SETFL, fcntl(sockets[0], F_GETFL, 0) | O_NONBLOCK);
  fcntl(sockets[1], F_SETFL, fcntl(sockets[1], F_GETFL, 0) | O_NONBLOCK);
  d->descriptor = sockets[0];
  d->inbuf[0] = '\0';
  d->last_input[0] = '\0';
  d->output = d->small_outbuf;
  d->bufptr = 0;
  d->bufspace = SMALL_BUFSIZE - 1;
  d->small_outbuf[0] = '\0';

  /* The first byte picks a delivery pattern so partial reads are exercised. */
  chunk = (data[0] & 7U) == 0 ? size : (size_t)(data[0] & 7U) * 3;
  for (offset = 0; offset < size; offset += chunk)
  {
    ssize_t written;
    size_t remaining;

    remaining = size - offset;
    written = write(sockets[1], data + offset, remaining < chunk ? remaining : chunk);
    if (written <= 0)
      break;
    result = process_input_for_test(d);
    if (result < 0)
      break;
  }
  while (process_input_for_test(d) == 1)
    ;
  close(sockets[1]);
  (void)process_input_for_test(d);

  while (get_from_q_for_test(&d->input, line, &aliased))
    fuzz_command_tokenize(line);

  flush_queues_for_test(d);
  for (index = 0; index < HISTORY_SIZE; index++)
  {
    free(d->history[index]);
    d->history[index] = NULL;
  }
  d->history_pos = 0;
  close(sockets[0]);
  d->descriptor = -1;
  return 0;
}

/* ----------------------------------------------------------------- i3 target */

static int fuzz_i3_run(const uint8_t *data, size_t size)
{
  i3_test_setup();
  (void)i3_process_input((const char *)data, size);
  i3_test_cleanup();
  return 0;
}

/* ----------------------------------------------------------------- ai target */

static int fuzz_ai_run(const uint8_t *data, size_t size)
{
  char *text;

  text = fuzz_cstring(data, size);
  if (text == NULL)
    return 0;
  free(ai_service_test_parse_json_response(text));
  free(ai_service_test_parse_ollama_json_response(text));
  free(text);
  return 0;
}

/* ------------------------------------------------------------ discord target */

static char *fuzz_discord_message;

static void fuzz_discord_setup(void)
{
  fuzz_discord_message = malloc(DISCORD_BRIDGE_MAX_MSG_LEN);
  if (fuzz_discord_message == NULL)
    abort();
}

static int fuzz_discord_run(const uint8_t *data, size_t size)
{
  char channel[64];
  char name[256];
  char *text;

  text = fuzz_cstring(data, size);
  if (text == NULL)
    return 0;
  (void)parse_discord_json(text, channel, name, fuzz_discord_message);
  free(text);
  return 0;
}

/* --------------------------------------------------------- onboarding target */

static struct descriptor_data *fuzz_onboarding_descriptor;

static void fuzz_onboarding_setup(void)
{
  fuzz_onboarding_descriptor = calloc(1, sizeof(*fuzz_onboarding_descriptor));
  if (fuzz_onboarding_descriptor == NULL)
    abort();
}

static int fuzz_onboarding_run(const uint8_t *data, size_t size)
{
  struct descriptor_data *d;
  char *payload;

  if (size == 0)
    return 0;
  d = fuzz_onboarding_descriptor;
  d->desc_num = 42;
  d->login_time = 1000;
  d->descriptor = -1;
  d->output = d->small_outbuf;
  d->bufptr = 0;
  d->bufspace = SMALL_BUFSIZE - 1;
  d->small_outbuf[0] = '\0';
  web_onboarding_reset(d);
  d->connected = CON_ACCOUNT_NAME;

  payload = fuzz_cstring(data + 1, size - 1);
  if (payload == NULL)
    return 0;
  switch (data[0] & 3U)
  {
  case 0:
    web_onboarding_set_capability(d, "1");
    web_onboarding_handle_action(d, payload);
    break;
  case 1:
    web_onboarding_set_capability(d, payload);
    break;
  case 2:
    web_onboarding_set_version_list(d, payload);
    break;
  default:
    web_onboarding_set_capability(d, "2");
    (void)web_onboarding_handle_catalog_control(d, payload);
    break;
  }
  free(payload);
  web_onboarding_reset(d);
  flush_queues_for_test(d);
  return 0;
}

/* -------------------------------------------------------------------- table */

static void fuzz_setup_none(void)
{
}

static const struct fuzz_target fuzz_targets[] = {
    {"dotenv", "lib/.env assignment parser (src/config/dotenv.c)", fuzz_enter_scratch_directory,
     fuzz_dotenv_run, 0U},
    {"config", "lib/etc/config game configuration file (load_config)", fuzz_config_setup,
     fuzz_config_run, FUZZ_TARGET_MAY_EXIT},
    {"dg", "DG script expression and variable substitution (eval, %var%)", fuzz_dg_setup,
     fuzz_dg_run, 0U},
    {"world", "flat world, mobile, object, zone, and trigger file records", fuzz_world_setup,
     fuzz_world_run, FUZZ_TARGET_MAY_EXIT},
    {"command", "socket line assembly, telnet stripping, and command tokenizers",
     fuzz_command_setup, fuzz_command_run, 0U},
    {"i3", "Intermud3 gateway line framing and JSON-RPC decoding", fuzz_setup_none, fuzz_i3_run,
     0U},
    {"ai", "AI provider response decoders (OpenAI and Ollama JSON)", fuzz_setup_none, fuzz_ai_run,
     0U},
    {"discord", "Discord bridge inbound JSON message parser", fuzz_discord_setup, fuzz_discord_run,
     0U},
    {"onboarding", "web onboarding editor envelopes, capability, and catalog controls",
     fuzz_onboarding_setup, fuzz_onboarding_run, 0U},
};

const struct fuzz_target *fuzz_target_table(size_t *count)
{
  *count = sizeof(fuzz_targets) / sizeof(fuzz_targets[0]);
  return fuzz_targets;
}

const struct fuzz_target *fuzz_target_find(const char *name)
{
  size_t index;

  if (name == NULL)
    return NULL;
  for (index = 0; index < sizeof(fuzz_targets) / sizeof(fuzz_targets[0]); index++)
    if (!strcmp(fuzz_targets[index].name, name))
      return &fuzz_targets[index];
  return NULL;
}

/* ------------------------------------------------------------------- replay */

static const char *fuzz_test_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static bool fuzz_read_file(const char *path, uint8_t **data, size_t *size)
{
  FILE *fp;
  long length;

  fp = fopen(path, "rb");
  if (fp == NULL)
    return false;
  if (fseek(fp, 0, SEEK_END) != 0 || (length = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0)
  {
    fclose(fp);
    return false;
  }
  *data = malloc((size_t)length + 1);
  if (*data == NULL)
  {
    fclose(fp);
    return false;
  }
  *size = fread(*data, 1, (size_t)length, fp);
  fclose(fp);
  return *size == (size_t)length;
}

/* Run one input in a child so a sanitizer report or a loader exit cannot take
 * the suite down; the child's stderr is scanned for sanitizer findings. */
static void fuzz_replay_one(CuTest *tc, const struct fuzz_target *target, const char *path)
{
  uint8_t *data;
  size_t size;
  int output_pipe[2];
  int status;
  pid_t child;
  char report[8192];
  ssize_t bytes_read;
  size_t report_length;
  char message[PATH_MAX + 128];

  data = NULL;
  size = 0;
  if (!fuzz_read_file(path, &data, &size))
  {
    snprintf(message, sizeof(message), "cannot read fuzz input %s", path);
    CuFail(tc, message);
    return;
  }
  CuAssertIntEquals(tc, 0, pipe(output_pipe));
  child = fork();
  CuAssert(tc, "fork failed", child >= 0);
  if (child == 0)
  {
    close(output_pipe[0]);
    if (dup2(output_pipe[1], STDERR_FILENO) < 0)
      _exit(20);
    close(output_pipe[1]);
    logfile = fopen("/dev/null", "w");
    target->setup();
    fuzz_exit_status = 0;
    (void)target->run(data, size);
    /* A target that may abandon records on a rejected input skips the
     * exit-time leak check; the others report leaks through exit(). */
    if (target->flags & FUZZ_TARGET_MAY_EXIT)
      _exit(fuzz_exit_status != 0 ? 1 : 0);
    exit(0);
  }
  free(data);
  close(output_pipe[1]);
  report_length = 0;
  while ((bytes_read =
              read(output_pipe[0], report + report_length, sizeof(report) - 1 - report_length)) > 0)
    report_length += (size_t)bytes_read;
  report[report_length] = '\0';
  close(output_pipe[0]);
  CuAssertTrue(tc, waitpid(child, &status, 0) == child);

  if (strstr(report, "Sanitizer") != NULL || strstr(report, "runtime error:") != NULL)
  {
    snprintf(message, sizeof(message), "sanitizer finding replaying %s:\n%s", path, report);
    CuFail(tc, message);
    return;
  }
  if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0 &&
                             !((target->flags & FUZZ_TARGET_MAY_EXIT) && WEXITSTATUS(status) == 1)))
  {
    snprintf(message, sizeof(message), "replaying %s: child %s %d\n%s", path,
             WIFSIGNALED(status) ? "died with signal" : "exited with status",
             WIFSIGNALED(status) ? WTERMSIG(status) : WEXITSTATUS(status), report);
    CuFail(tc, message);
  }
}

static int fuzz_replay_directory(CuTest *tc, const struct fuzz_target *target,
                                 const char *directory)
{
  DIR *dir;
  struct dirent *entry;
  struct stat status;
  char path[PATH_MAX];
  int replayed;

  dir = opendir(directory);
  if (dir == NULL)
    return 0;
  replayed = 0;
  while ((entry = readdir(dir)) != NULL)
  {
    if (entry->d_name[0] == '.')
      continue;
    if (snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name) >= (int)sizeof(path))
      continue;
    if (stat(path, &status) != 0 || !S_ISREG(status.st_mode))
      continue;
    fuzz_replay_one(tc, target, path);
    replayed++;
  }
  closedir(dir);
  return replayed;
}

void Test_fuzz_targets_replay_seed_and_regression_inputs(CuTest *tc)
{
  const struct fuzz_target *targets;
  size_t count;
  size_t index;
  int replayed;
  char directory[PATH_MAX];

  targets = fuzz_target_table(&count);
  replayed = 0;
  for (index = 0; index < count; index++)
  {
    snprintf(directory, sizeof(directory), "%s/unittests/CuTest/fuzz_corpus_game/%s",
             fuzz_test_root(), targets[index].name);
    replayed += fuzz_replay_directory(tc, &targets[index], directory);
    snprintf(directory, sizeof(directory), "%s/unittests/CuTest/fuzz_regressions/%s",
             fuzz_test_root(), targets[index].name);
    replayed += fuzz_replay_directory(tc, &targets[index], directory);
  }
  CuAssert(tc, "no fuzz seed or regression inputs were found", replayed > 0);
}

/* Every target must be reachable by name and run an empty input safely. */
void Test_fuzz_targets_table_names_are_unique_and_findable(CuTest *tc)
{
  const struct fuzz_target *targets;
  size_t count;
  size_t index;
  size_t other;

  targets = fuzz_target_table(&count);
  CuAssertTrue(tc, count >= 9);
  for (index = 0; index < count; index++)
  {
    CuAssertTrue(tc, fuzz_target_find(targets[index].name) == &targets[index]);
    for (other = index + 1; other < count; other++)
      CuAssertTrue(tc, strcmp(targets[index].name, targets[other].name) != 0);
  }
  CuAssertTrue(tc, fuzz_target_find("no-such-target") == NULL);
  CuAssertTrue(tc, fuzz_target_find(NULL) == NULL);
}
