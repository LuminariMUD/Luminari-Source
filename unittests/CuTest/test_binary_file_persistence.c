/*
 * Production-linked tests for the durable binary files: boards and the house
 * control file upgrade legacy files once while keeping a backup, move files
 * they cannot decode aside instead of deleting or overwriting them, and keep
 * the previous file when a save is killed mid-write; "last all" reads the
 * retired login log through the portable decoder.
 */

#include "CuTest.h"
#include "binary_format_fixtures.h"
#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/comms/boards.h"
#include "../../src/core/binary_formats.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/house.h"
#include <dirent.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>

extern struct house_control_rec house_control[MAX_HOUSES];
extern int num_of_houses;
void House_save_control(void);

static bool write_test_file(const char *path, const unsigned char *data, size_t size)
{
  FILE *file = fopen(path, "wb");
  bool written;

  if (file == NULL)
    return false;
  written = fwrite(data, 1, size, file) == size;
  return fclose(file) == 0 && written;
}

/* Returns the file's bytes (NULL when absent) with one spare byte. */
static unsigned char *read_test_file(const char *path, size_t *size)
{
  unsigned char *data;

  *size = 0;
  if (read_durable_file(path, 1U << 24, &data, size) != DURABLE_FILE_READ)
    return NULL;
  return data;
}

static bool file_has_bytes(const char *path, const unsigned char *expected, size_t expected_size)
{
  unsigned char *data;
  size_t size;
  bool same;

  data = read_test_file(path, &size);
  same = data != NULL && size == expected_size && memcmp(data, expected, size) == 0;
  free(data);
  return same;
}

/* Counts entries of directory whose names start with prefix; the last match is copied to name. */
static int count_entries(const char *directory, const char *prefix, char *name, size_t name_size)
{
  struct dirent *entry;
  DIR *listing;
  int count = 0;

  listing = opendir(directory);
  if (listing == NULL)
    return -1;
  while ((entry = readdir(listing)) != NULL)
    if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0)
    {
      count++;
      if (name != NULL)
        snprintf(name, name_size, "%s/%s", directory, entry->d_name);
    }
  closedir(listing);
  return count;
}

/* Removes the regular files in directory, then the directory. */
static void remove_test_directory(const char *directory)
{
  char path[PATH_MAX];
  struct dirent *entry;
  DIR *listing;

  listing = opendir(directory);
  if (listing != NULL)
  {
    while ((entry = readdir(listing)) != NULL)
    {
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;
      snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);
      unlink(path);
    }
    closedir(listing);
  }
  rmdir(directory);
}

/* Short scratch names keep every derived path inside board_info's filename. */
#define SCRATCH_DIRECTORY_SIZE 32

struct board_file_fixture
{
  char directory[SCRATCH_DIRECTORY_SIZE];
  char saved_filename[sizeof(board_info[0].filename)];
};

static void board_file_fixture_enter(CuTest *tc, struct board_file_fixture *fixture)
{
  snprintf(fixture->directory, sizeof(fixture->directory), "/tmp/luminari-boards-XXXXXX");
  CuAssertPtrNotNull(tc, mkdtemp(fixture->directory));
  strlcpy(fixture->saved_filename, board_info[0].filename, sizeof(fixture->saved_filename));
  snprintf(board_info[0].filename, sizeof(board_info[0].filename), "%s/board", fixture->directory);
  board_clear_all();
}

static void board_file_fixture_leave(struct board_file_fixture *fixture)
{
  board_clear_all();
  strlcpy(board_info[0].filename, fixture->saved_filename, sizeof(board_info[0].filename));
  remove_test_directory(fixture->directory);
}

void Test_board_legacy_file_upgrades_once_and_keeps_a_backup(CuTest *tc)
{
  struct board_file_fixture fixture;
  unsigned char *legacy, *current;
  size_t legacy_size, current_size;
  char backup[PATH_MAX];

  legacy = binary_format_fixture(FIXTURE_LEGACY_BOARD_FILE, &legacy_size);
  current = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &current_size);
  CuAssertTrue(tc, legacy != NULL && current != NULL);
  board_file_fixture_enter(tc, &fixture);
  CuAssertTrue(tc, write_test_file(board_info[0].filename, legacy, legacy_size));

  board_load_board(0);
  board_save_board(0);
  CuAssertTrue(tc, file_has_bytes(board_info[0].filename, current, current_size));
  snprintf(backup, sizeof(backup), "%s.legacy-%08" PRIx32, board_info[0].filename,
           binary_format_crc32(legacy, legacy_size));
  CuAssertTrue(tc, file_has_bytes(backup, legacy, legacy_size));

  /* Saving and reloading the upgraded file changes nothing and backs up nothing. */
  board_save_board(0);
  board_clear_all();
  board_load_board(0);
  board_save_board(0);
  CuAssertTrue(tc, file_has_bytes(board_info[0].filename, current, current_size));
  CuAssertIntEquals(tc, 1, count_entries(fixture.directory, "board.legacy-", NULL, 0));
  CuAssertIntEquals(tc, 0, count_entries(fixture.directory, "board.tmp", NULL, 0));

  board_file_fixture_leave(&fixture);
  free(current);
  free(legacy);
}

void Test_board_file_that_cannot_be_decoded_is_moved_aside(CuTest *tc)
{
  struct board_file_message *messages = NULL;
  struct board_file_fixture fixture;
  unsigned char *corrupt, *saved;
  size_t corrupt_size, saved_size, count = 1;
  char rejected[PATH_MAX];
  int version = -1;

  corrupt = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &corrupt_size);
  CuAssertPtrNotNull(tc, corrupt);
  corrupt[40] ^= 0x01; /* a heading byte: the checksum no longer matches */
  board_file_fixture_enter(tc, &fixture);
  CuAssertTrue(tc, write_test_file(board_info[0].filename, corrupt, corrupt_size));

  board_load_board(0);
  CuAssertTrue(tc, access(board_info[0].filename, F_OK) != 0);
  CuAssertIntEquals(
      tc, 1, count_entries(fixture.directory, "board.rejected-", rejected, sizeof(rejected)));
  CuAssertTrue(tc, file_has_bytes(rejected, corrupt, corrupt_size));

  /* Nothing was half-loaded, and saving leaves the rejected file alone. */
  board_save_board(0);
  saved = read_test_file(board_info[0].filename, &saved_size);
  CuAssertPtrNotNull(tc, saved);
  CuAssertIntEquals(
      tc, BINARY_FORMAT_OK,
      board_file_decode(saved, saved_size, MAX_BOARD_MESSAGES, &messages, &count, &version));
  CuAssertIntEquals(tc, 0, (int)count);
  CuAssertTrue(tc, file_has_bytes(rejected, corrupt, corrupt_size));

  free(saved);
  board_file_fixture_leave(&fixture);
  free(corrupt);
}

void Test_board_save_killed_mid_write_keeps_the_previous_file(CuTest *tc)
{
  const size_t limit = 64;
  struct board_file_message larger_board[3];
  struct board_file_fixture fixture;
  unsigned char *current, *replacement = NULL;
  size_t current_size, replacement_size = 0;
  char live[sizeof(board_info[0].filename)], other[sizeof(board_info[0].filename)];
  char temporary[PATH_MAX];
  struct stat temporary_status;
  struct rlimit rlimit_value;
  int status = 0;
  pid_t child;

  larger_board[0].level = 1;
  larger_board[0].heading = CuMutableString("First replacement post");
  larger_board[0].message = CuMutableString("A body long enough to pass the file size limit.");
  larger_board[1].level = 2;
  larger_board[1].heading = CuMutableString("Second replacement post");
  larger_board[1].message = CuMutableString("Another body that keeps the save writing.");
  larger_board[2].level = 3;
  larger_board[2].heading = CuMutableString("Third replacement post");
  larger_board[2].message = NULL;
  current = binary_format_fixture(FIXTURE_CURRENT_BOARD_FILE, &current_size);
  CuAssertPtrNotNull(tc, current);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    board_file_encode(larger_board, 3, &replacement, &replacement_size));
  CuAssertTrue(tc, replacement_size > limit && current_size > limit);
  board_file_fixture_enter(tc, &fixture);
  strlcpy(live, board_info[0].filename, sizeof(live));
  snprintf(other, sizeof(other), "%s/other", fixture.directory);
  CuAssertTrue(tc, write_test_file(live, current, current_size));
  CuAssertTrue(tc, write_test_file(other, replacement, replacement_size));

  fflush(NULL);
  child = fork();
  CuAssertTrue(tc, child >= 0);
  if (child == 0)
  {
    /* Log to a device so only the save's temporary file meets the limit. */
    logfile = fopen("/dev/null", "w");
    strlcpy(board_info[0].filename, other, sizeof(board_info[0].filename));
    board_load_board(0);
    strlcpy(board_info[0].filename, live, sizeof(board_info[0].filename));
    rlimit_value.rlim_cur = rlimit_value.rlim_max = 0;
    setrlimit(RLIMIT_CORE, &rlimit_value);
    rlimit_value.rlim_cur = rlimit_value.rlim_max = limit;
    setrlimit(RLIMIT_FSIZE, &rlimit_value);
    board_save_board(0); /* SIGXFSZ ends the process inside this call */
    _exit(0);
  }
  CuAssertIntEquals(tc, child, waitpid(child, &status, 0));
  CuAssertTrue(tc, WIFSIGNALED(status) && WTERMSIG(status) == SIGXFSZ);

  CuAssertTrue(tc, file_has_bytes(live, current, current_size));
  snprintf(temporary, sizeof(temporary), "%s.tmp", live);
  CuAssertIntEquals(tc, 0, stat(temporary, &temporary_status));
  CuAssertIntEquals(tc, (int)limit, (int)temporary_status.st_size);

  /* The next save starts over from its own temporary file. */
  board_load_board(0);
  board_save_board(0);
  CuAssertTrue(tc, file_has_bytes(live, current, current_size));

  board_file_fixture_leave(&fixture);
  free(replacement);
  free(current);
}

struct temp_lib_fixture
{
  char directory[SCRATCH_DIRECTORY_SIZE];
  char previous_directory[PATH_MAX];
  struct house_control_rec *saved_houses;
  int saved_count;
  struct player_index_element *saved_table;
  int saved_top;
};

/* Runs a test inside a scratch lib directory with an etc/ subdirectory, with
 * the house globals and player table saved and emptied. */
static void temp_lib_fixture_enter(CuTest *tc, struct temp_lib_fixture *fixture)
{
  snprintf(fixture->directory, sizeof(fixture->directory), "/tmp/luminari-houses-XXXXXX");
  CuAssertPtrNotNull(tc, getcwd(fixture->previous_directory, sizeof(fixture->previous_directory)));
  CuAssertPtrNotNull(tc, mkdtemp(fixture->directory));
  CuAssertIntEquals(tc, 0, chdir(fixture->directory));
  CuAssertIntEquals(tc, 0, mkdir("etc", 0700));

  fixture->saved_houses = malloc(sizeof(house_control));
  CuAssertPtrNotNull(tc, fixture->saved_houses);
  memcpy(fixture->saved_houses, house_control, sizeof(house_control));
  fixture->saved_count = num_of_houses;
  fixture->saved_table = player_table;
  fixture->saved_top = top_of_p_table;
  /* No owner is known, so boot skips every record without touching the world. */
  player_table = NULL;
  top_of_p_table = -1;
  num_of_houses = 0;
}

static void temp_lib_fixture_leave(struct temp_lib_fixture *fixture)
{
  char etc[PATH_MAX];

  memcpy(house_control, fixture->saved_houses, sizeof(house_control));
  free(fixture->saved_houses);
  num_of_houses = fixture->saved_count;
  player_table = fixture->saved_table;
  top_of_p_table = fixture->saved_top;
  if (chdir(fixture->previous_directory) != 0)
    return;
  snprintf(etc, sizeof(etc), "%s/etc", fixture->directory);
  remove_test_directory(etc);
  remove_test_directory(fixture->directory);
}

void Test_house_control_file_upgrades_legacy_data_and_saves_the_current_format(CuTest *tc)
{
  struct house_file_record *records = NULL;
  struct temp_lib_fixture fixture;
  unsigned char *legacy, *current, *saved;
  size_t legacy_size, current_size, saved_size, count = 1;
  char backup[PATH_MAX];
  int version = -1;

  legacy = binary_format_fixture(FIXTURE_LEGACY_HOUSE_FILE, &legacy_size);
  current = binary_format_fixture(FIXTURE_CURRENT_HOUSE_FILE, &current_size);
  CuAssertTrue(tc, legacy != NULL && current != NULL);
  temp_lib_fixture_enter(tc, &fixture);
  CuAssertTrue(tc, write_test_file(HCONTROL_FILE, legacy, legacy_size));

  /* Both owners are unknown here, so boot drops both houses, but the legacy
   * bytes survive in the backup. */
  House_boot();
  CuAssertIntEquals(tc, 0, num_of_houses);
  saved = read_test_file(HCONTROL_FILE, &saved_size);
  CuAssertPtrNotNull(tc, saved);
  CuAssertIntEquals(tc, BINARY_FORMAT_OK,
                    house_file_decode(saved, saved_size, MAX_HOUSES, &records, &count, &version));
  CuAssertIntEquals(tc, HOUSE_FILE_VERSION, version);
  CuAssertIntEquals(tc, 0, (int)count);
  free(saved);
  snprintf(backup, sizeof(backup), "%s.legacy-%08" PRIx32, HCONTROL_FILE,
           binary_format_crc32(legacy, legacy_size));
  CuAssertTrue(tc, file_has_bytes(backup, legacy, legacy_size));

  /* The server's field mapping produces the golden file. */
  memset(house_control, 0, sizeof(house_control[0]) * 2);
  house_control[0].vnum = 103496;
  house_control[0].atrium = 103495;
  house_control[0].exit_num = 2;
  house_control[0].built_on = 1600000000;
  house_control[0].mode = HOUSE_CLAN;
  house_control[0].owner = 4242;
  house_control[0].num_of_guests = 3;
  house_control[0].guests[0] = 7;
  house_control[0].guests[1] = 8;
  house_control[0].guests[2] = 9;
  house_control[0].last_payment = 1700000000;
  house_control[0].bitvector = HOUSE_FREE | HOUSE_NOIMMS;
  house_control[0].builtby = 1;
  house_control[1].vnum = 11395;
  house_control[1].atrium = 11394;
  house_control[1].exit_num = 5;
  house_control[1].built_on = 1500000000;
  house_control[1].mode = HOUSE_PRIVATE;
  house_control[1].owner = 99;
  num_of_houses = 2;
  House_save_control();
  CuAssertTrue(tc, file_has_bytes(HCONTROL_FILE, current, current_size));
  CuAssertIntEquals(tc, 1, count_entries("etc", "hcontrol.legacy-", NULL, 0));

  temp_lib_fixture_leave(&fixture);
  free(current);
  free(legacy);
}

void Test_house_boot_moves_an_undecodable_control_file_aside(CuTest *tc)
{
  struct temp_lib_fixture fixture;
  unsigned char *corrupt;
  size_t corrupt_size;
  char rejected[PATH_MAX];

  corrupt = binary_format_fixture(FIXTURE_CURRENT_HOUSE_FILE, &corrupt_size);
  CuAssertPtrNotNull(tc, corrupt);
  corrupt[30] ^= 0x10; /* a house field: the checksum no longer matches */
  temp_lib_fixture_enter(tc, &fixture);
  CuAssertTrue(tc, write_test_file(HCONTROL_FILE, corrupt, corrupt_size));

  House_boot();
  CuAssertIntEquals(tc, 0, num_of_houses);
  CuAssertTrue(tc, access(HCONTROL_FILE, F_OK) != 0);
  CuAssertIntEquals(tc, 1, count_entries("etc", "hcontrol.rejected-", rejected, sizeof(rejected)));
  CuAssertTrue(tc, file_has_bytes(rejected, corrupt, corrupt_size));

  temp_lib_fixture_leave(&fixture);
  free(corrupt);
}

void Test_last_all_lists_the_retired_login_log(CuTest *tc)
{
  struct temp_lib_fixture fixture;
  struct player_special_data staff_specials;
  struct descriptor_data descriptor;
  struct char_data staff;
  unsigned char *log_file;
  size_t log_size;
  bool listed, unreadable;

  log_file = binary_format_fixture(FIXTURE_LEGACY_LAST_LOG, &log_size);
  CuAssertPtrNotNull(tc, log_file);
  temp_lib_fixture_enter(tc, &fixture);
  CuAssertTrue(tc, write_test_file(LAST_FILE, log_file, log_size));

  clear_char(&staff);
  memset(&staff_specials, 0, sizeof(staff_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.character = &staff;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  staff.desc = &descriptor;
  staff.player_specials = &staff_specials;
  staff.player.name = CuMutableString("laststaff");
  GET_LEVEL(&staff) = LVL_IMPL;

  do_last(&staff, "all", 0, 0);
  listed = strstr(descriptor.output, "Last log") != NULL &&
           strstr(descriptor.output, "Zusuk") != NULL &&
           strstr(descriptor.output, "Quit") != NULL &&
           strstr(descriptor.output, "Abcdefghijklmnop") != NULL &&
           strstr(descriptor.output, "Unknown") != NULL;

  /* A torn record makes the whole log unreadable rather than misaligned. */
  descriptor.output[0] = '\0';
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  CuAssertTrue(tc, write_test_file(LAST_FILE, log_file, log_size - 1));
  do_last(&staff, "all", 0, 0);
  unreadable = strstr(descriptor.output, "The last log is unreadable.") != NULL &&
               strstr(descriptor.output, "Zusuk") == NULL;

  staff.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  temp_lib_fixture_leave(&fixture);
  free(log_file);
  CuAssertTrue(tc, listed);
  CuAssertTrue(tc, unreadable);
}
