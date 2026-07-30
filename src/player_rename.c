/**************************************************************************
 * File: player_rename.c                              Part of LuminariMUD *
 * Usage: Transactional, end-to-end player character renames.             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "ban.h"
#include "helpers.h"
#include "mysql.h"
#include "account.h"
#include "pubsub.h"
#include "protocol.h"
#include "movement_tracks.h"
#include "player_rename.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <openssl/evp.h>
#include <sys/stat.h>
#include <unistd.h>

#define PLAYER_RENAME_PATH_SIZE (MAX_FILEPATH + 64)
#define PLAYER_RENAME_DB_KEY_COUNT 22
#define PLAYER_RENAME_DIGEST_SIZE 32

struct rename_db_key
{
  const char *table;
  const char *column;
  int required;
  int exclude_mail_all;
  int preserve_payload;
  int active;
  unsigned long long old_count;
  unsigned long long payload_rows;
  unsigned char payload_digest[PLAYER_RENAME_DIGEST_SIZE];
  unsigned long long affected_count;
  char preserve_assignments[512];
};

struct rename_file_move
{
  int mode;
  int required;
  int exists;
  int moved;
  int digest_ready;
  struct stat source_stat;
  unsigned char source_digest[PLAYER_RENAME_DIGEST_SIZE];
  char old_path[PLAYER_RENAME_PATH_SIZE];
  char new_path[PLAYER_RENAME_PATH_SIZE];
};

struct rename_intro_file
{
  char path[PLAYER_RENAME_PATH_SIZE];
  char backup_path[PLAYER_RENAME_PATH_SIZE];
  int backup_ready;
  int changed;
};

struct rename_account_refresh
{
  struct account_data *account;
  char *character_names[MAX_CHARS_PER_ACCOUNT];
};

struct rename_live_string
{
  char **field;
  char *replacement;
};

struct rename_context
{
  struct char_data *actor;
  struct char_data *victim;
  struct player_rename_report *report;
  struct rename_db_key keys[PLAYER_RENAME_DB_KEY_COUNT];
  struct rename_file_move files[MAX_FILES];
  struct rename_intro_file *intro_files;
  int intro_count;
  struct rename_account_refresh *account_refreshes;
  int account_refresh_count;
  struct rename_live_string *live_strings;
  int live_string_count;
  int player_index_position;
  int transaction_started;
  int database_updates_attempted;
  int commit_attempted;
  int mysql_locked;
  int memory_changed;
  int index_written;
  int level_30_view_active;
  int database_player_id_present;
  int object_header_is_null;
  unsigned int target_intro_changes;
  char old_display_name[MAX_NAME_LENGTH + 1];
  char new_display_name[MAX_NAME_LENGTH + 1];
  char new_index_name[MAX_NAME_LENGTH + 1];
  char database_player_id_column[32];
  unsigned long long database_player_id;
  unsigned long long level_30_view_old_count;
  char object_header_hash[65];
  char old_database_name[MAX_NAME_LENGTH + 1];
  char *escaped_old_name;
  char *escaped_new_name;
  char *account_name;
  char *old_player_index_name;
  char *old_victim_name;
  char *new_player_index_name;
  char *new_victim_name;
  char target_backup[PLAYER_RENAME_PATH_SIZE];
  char index_path[PLAYER_RENAME_PATH_SIZE];
  char index_backup[PLAYER_RENAME_PATH_SIZE];
  int target_backup_ready;
  int index_backup_ready;
  enum player_rename_status status;
};

/*
 * Current ownership and retrieval keys only.  Historical display snapshots
 * (board authors, pubsub message senders, cargo loaders, creator fields) stay
 * unchanged.  Numeric-ID ownership (houses, vehicles, clans, bound objects,
 * corpses, missions, board state, and legacy mudmail) also needs no DB key
 * migration.  player_data2 is obsolete; level_30_characters is verified as a
 * derived view below rather than updated.
 */
static const struct rename_db_key rename_key_template[PLAYER_RENAME_DB_KEY_COUNT] = {
    {"player_data", "name", TRUE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_save_objs", "name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_save_objs_sheathed", "owner_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"pet_data", "owner_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"pet_save_objs", "owner_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_eidolons", "owner", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"loot_chests", "character_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_mail", "sender", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_mail", "receiver", FALSE, TRUE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_mail_deleted", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_mail_read", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_quest_info", "character_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_quest_progress", "character_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_supply_orders", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"supply_orders_available", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"pubsub_player_settings", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"pubsub_subscriptions", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_levelups", "character_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"player_levels", "char_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"vessel_bounties", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0, {0}},
    {"vessel_merchant_consequences", "player_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0,
     {0}},
    {"vessel_npc_merchants", "last_attacker_name", FALSE, FALSE, TRUE, FALSE, 0, 0, {0}, 0,
     {0}}};

static void rename_set_failure(struct rename_context *ctx, enum player_rename_status status,
                               const char *stage);
static int rename_verify_old_index_file(struct rename_context *ctx);

#ifdef LUMINARI_CUTEST
static enum player_rename_test_failure_point rename_test_failure_point =
    PLAYER_RENAME_TEST_FAIL_NONE;
static unsigned int rename_test_failure_occurrence;
static unsigned int rename_test_failure_seen;

void player_rename_set_failure_for_test(enum player_rename_test_failure_point point,
                                        unsigned int occurrence)
{
  rename_test_failure_point = point;
  rename_test_failure_occurrence = occurrence;
  rename_test_failure_seen = 0;
}

static int rename_inject_failure(struct rename_context *ctx,
                                 enum player_rename_test_failure_point point,
                                 enum player_rename_status status, const char *stage)
{
  if (rename_test_failure_point != point)
    return FALSE;

  rename_test_failure_seen++;
  if (rename_test_failure_occurrence != 0 &&
      rename_test_failure_seen != rename_test_failure_occurrence)
    return FALSE;

  rename_set_failure(ctx, status, stage);
  return TRUE;
}
#else
static int rename_inject_failure(struct rename_context *ctx, int point,
                                 enum player_rename_status status, const char *stage)
{
  (void)ctx;
  (void)point;
  (void)status;
  (void)stage;
  return FALSE;
}

#define PLAYER_RENAME_TEST_FAIL_AUXILIARY_MOVE 1
#define PLAYER_RENAME_TEST_FAIL_DATABASE_UPDATE 2
#define PLAYER_RENAME_TEST_FAIL_PLAYER_FILE_WRITE 3
#define PLAYER_RENAME_TEST_FAIL_INTRODUCTION_WRITE 4
#define PLAYER_RENAME_TEST_FAIL_INDEX_WRITE 5
#define PLAYER_RENAME_TEST_FAIL_POSTCONDITION 6
#define PLAYER_RENAME_TEST_FAIL_COMMIT 7
#endif

static void rename_set_failure(struct rename_context *ctx, enum player_rename_status status,
                               const char *stage)
{
  if (!ctx)
    return;

  ctx->status = status;
  if (ctx->report)
  {
    ctx->report->status = status;
    if (stage)
      strlcpy(ctx->report->failure_stage, stage, sizeof(ctx->report->failure_stage));
  }
}

const char *player_rename_status_string(enum player_rename_status status)
{
  switch (status)
  {
  case PLAYER_RENAME_OK:
    return "success";
  case PLAYER_RENAME_INVALID_NAME:
    return "invalid name";
  case PLAYER_RENAME_NAME_EXISTS:
    return "name already exists";
  case PLAYER_RENAME_PLAYER_NOT_FOUND:
    return "player not found";
  case PLAYER_RENAME_DATABASE_UNAVAILABLE:
    return "database unavailable";
  case PLAYER_RENAME_DATABASE_ERROR:
    return "database error";
  case PLAYER_RENAME_FILE_COLLISION:
    return "file collision";
  case PLAYER_RENAME_FILE_ERROR:
    return "file error";
  case PLAYER_RENAME_SAVE_ERROR:
    return "save error";
  case PLAYER_RENAME_POSTCONDITION_FAILED:
    return "postcondition failed";
  default:
    return "unknown error";
  }
}

static int rename_name_is_alpha(const char *name)
{
  const unsigned char *scan;

  if (!name || !*name)
    return FALSE;

  for (scan = (const unsigned char *)name; *scan; scan++)
    if (!isalpha(*scan))
      return FALSE;

  return TRUE;
}

static int rename_name_is_ascii(const char *name)
{
  const unsigned char *scan;

  if (!name)
    return FALSE;
  for (scan = (const unsigned char *)name; *scan; scan++)
    if (*scan > 0x7f)
      return FALSE;
  return TRUE;
}

static int rename_validate_name(struct rename_context *ctx, const char *requested_name)
{
  char parse_input[MAX_INPUT_LENGTH];
  char parsed_name[MAX_INPUT_LENGTH];
  char validation_name[MAX_INPUT_LENGTH];
  char word_name[MAX_INPUT_LENGTH];
  size_t length;

  if (!requested_name)
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "validating name");
    return FALSE;
  }

  if (!rename_name_is_ascii(requested_name) || strlen(requested_name) >= sizeof(parse_input))
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "validating name");
    return FALSE;
  }

  strlcpy(parse_input, requested_name, sizeof(parse_input));
  if (parse_player_name(parse_input, parsed_name))
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "validating name");
    return FALSE;
  }

  length = strlen(parsed_name);
  if (length < 2 || length > MAX_NAME_LENGTH || !rename_name_is_alpha(parsed_name))
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "validating name");
    return FALSE;
  }

  strlcpy(validation_name, parsed_name, sizeof(validation_name));
  strlcpy(word_name, parsed_name, sizeof(word_name));
  if (!valid_name(validation_name) || fill_word(word_name) || reserved_word(word_name))
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "validating name");
    return FALSE;
  }

  strlcpy(ctx->new_display_name, parsed_name, sizeof(ctx->new_display_name));
  CAP(ctx->new_display_name);
  strlcpy(ctx->new_index_name, parsed_name, sizeof(ctx->new_index_name));
  {
    char *scan;
    for (scan = ctx->new_index_name; *scan; scan++)
      *scan = LOWER(*scan);
  }

  if (!strcasecmp(ctx->old_display_name, ctx->new_display_name))
  {
    rename_set_failure(ctx, PLAYER_RENAME_INVALID_NAME, "case-only rename");
    return FALSE;
  }

  return TRUE;
}

static int rename_find_player_index(struct rename_context *ctx)
{
  int i;
  int id_matches = 0;
  int name_matches = 0;

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].id == GET_IDNUM(ctx->victim))
    {
      if (!player_table[i].name || strcasecmp(player_table[i].name, ctx->old_display_name))
      {
        rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                           "verifying player index identity");
        return FALSE;
      }
      ctx->player_index_position = i;
      id_matches++;
    }
    if (player_table[i].name && !strcasecmp(player_table[i].name, ctx->old_display_name))
      name_matches++;
  }

  if (id_matches == 1 && name_matches == 1)
    return TRUE;

  rename_set_failure(
      ctx, id_matches == 0 ? PLAYER_RENAME_PLAYER_NOT_FOUND : PLAYER_RENAME_POSTCONDITION_FAILED,
      id_matches == 0 ? "finding player index entry" : "verifying unique player index identity");
  return FALSE;
}

static int rename_check_memory_collisions(struct rename_context *ctx)
{
  int i;
  struct char_data *scan;

  for (i = 0; i <= top_of_p_table; i++)
    if (i != ctx->player_index_position && player_table[i].name &&
        !strcasecmp(player_table[i].name, ctx->new_display_name))
    {
      rename_set_failure(ctx, PLAYER_RENAME_NAME_EXISTS, "checking player index collision");
      return FALSE;
    }

  for (scan = character_list; scan; scan = scan->next)
    if (!IS_NPC(scan) && scan != ctx->victim && GET_NAME(scan) &&
        !strcasecmp(GET_NAME(scan), ctx->new_display_name))
    {
      rename_set_failure(ctx, PLAYER_RENAME_NAME_EXISTS, "checking live player collision");
      return FALSE;
    }

  return TRUE;
}

static int rename_line_equals(const char *line, const char *value)
{
  size_t line_length;
  size_t value_length;

  if (!line || !value)
    return FALSE;

  line_length = strlen(line);
  while (line_length > 0 && (line[line_length - 1] == '\n' || line[line_length - 1] == '\r'))
    line_length--;

  value_length = strlen(value);
  return line_length == value_length && !strncasecmp(line, value, value_length);
}

static int rename_line_equals_exact(const char *line, const char *value)
{
  size_t line_length;
  size_t value_length;

  if (!line || !value)
    return FALSE;

  line_length = strlen(line);
  while (line_length > 0 && (line[line_length - 1] == '\n' || line[line_length - 1] == '\r'))
    line_length--;

  value_length = strlen(value);
  return line_length == value_length && !strncmp(line, value, value_length);
}

enum rename_tilde_block
{
  RENAME_TILDE_BLOCK_NONE = 0,
  RENAME_TILDE_BLOCK_STRING,
  RENAME_TILDE_BLOCK_TODO,
  RENAME_TILDE_BLOCK_INTRODUCTIONS_START,
  RENAME_TILDE_BLOCK_INTRODUCTIONS_NAMES,
  RENAME_TILDE_BLOCK_INTRODUCTIONS_LEGACY
};

enum rename_device_block
{
  RENAME_DEVICE_BLOCK_NONE = 0,
  RENAME_DEVICE_BLOCK_COUNT,
  RENAME_DEVICE_BLOCK_FIXED,
  RENAME_DEVICE_BLOCK_AFTER_EFFECTS,
  RENAME_DEVICE_BLOCK_LEVELS,
  RENAME_DEVICE_BLOCK_NEXT_INDEX,
  RENAME_DEVICE_BLOCK_TERMINATOR
};

enum rename_pfile_line_role
{
  RENAME_PFILE_LINE_TOP_LEVEL = 0,
  RENAME_PFILE_LINE_NESTED,
  RENAME_PFILE_LINE_INTRODUCTION
};

struct rename_pfile_scanner
{
  enum rename_tilde_block tilde_block;
  enum rename_device_block device_block;
  long opaque_lines_remaining;
  int device_inventions_remaining;
  int device_lines_remaining;
  int invalid;
};

static int rename_line_has_pfile_tag(const char *line, const char *tag)
{
  return line && tag && !strncmp(line, tag, 4);
}

static const char *rename_pfile_tag_value(const char *line)
{
  const char *value = line + 4;

  while (*value == ':' || *value == ' ')
    value++;
  return value;
}

static enum rename_tilde_block rename_tilde_block_start(const char *line)
{
  static const char *const string_tags[] = {"Desc", "BGrd", "Goal", "Pers", "Idel", "Bond", "Flaw"};
  size_t i;

  if (rename_line_has_pfile_tag(line, "Intr"))
    return RENAME_TILDE_BLOCK_INTRODUCTIONS_START;
  if (rename_line_has_pfile_tag(line, "Todo"))
    return RENAME_TILDE_BLOCK_TODO;
  for (i = 0; (size_t)i < sizeof(string_tags) / sizeof(string_tags[0]); i++)
    if (rename_line_has_pfile_tag(line, string_tags[i]))
      return RENAME_TILDE_BLOCK_STRING;
  return RENAME_TILDE_BLOCK_NONE;
}

static int rename_intro_line_starts_legacy_block(const char *line)
{
  size_t line_length;
  long value;

  if (!line)
    return FALSE;
  line_length = strlen(line);
  while (line_length > 0 && (line[line_length - 1] == '\n' || line[line_length - 1] == '\r'))
    line_length--;
  return line_length < 10 && sscanf(line, "%ld", &value) == 1;
}

static void rename_classify_intro_block(enum rename_tilde_block *block, const char *line)
{
  if (*block != RENAME_TILDE_BLOCK_INTRODUCTIONS_START)
    return;
  *block = rename_intro_line_starts_legacy_block(line) ? RENAME_TILDE_BLOCK_INTRODUCTIONS_LEGACY
                                                       : RENAME_TILDE_BLOCK_INTRODUCTIONS_NAMES;
}

static int rename_tilde_block_ends(enum rename_tilde_block block, const char *line)
{
  long value;

  if (!line)
    return FALSE;
  if (block == RENAME_TILDE_BLOCK_STRING)
    return strchr(line, '~') != NULL;
  if (block == RENAME_TILDE_BLOCK_TODO)
    return line[0] == '~';
  if (block == RENAME_TILDE_BLOCK_INTRODUCTIONS_LEGACY)
    return sscanf(line, "%ld", &value) != 1 || value == -1;
  return rename_line_equals(line, "~");
}

static int rename_parse_nonnegative_count(const char *text, long maximum, long *result)
{
  char *end;
  long value;

  if (!text || !result)
    return FALSE;
  while (*text && isspace((unsigned char)*text))
    text++;
  errno = 0;
  value = strtol(text, &end, 10);
  if (text == end || errno == ERANGE || value < 0 || value > maximum)
    return FALSE;
  while (*end && isspace((unsigned char)*end))
    end++;
  if (*end)
    return FALSE;
  *result = value;
  return TRUE;
}

static enum rename_pfile_line_role rename_scan_device_line(struct rename_pfile_scanner *scanner,
                                                           const char *line)
{
  long count;

  switch (scanner->device_block)
  {
  case RENAME_DEVICE_BLOCK_COUNT:
    if (!rename_parse_nonnegative_count(line, MAX_PLAYER_INVENTIONS, &count))
    {
      scanner->invalid = TRUE;
      return RENAME_PFILE_LINE_NESTED;
    }
    scanner->device_inventions_remaining = (int)count;
    if (count == 0)
      scanner->device_block = RENAME_DEVICE_BLOCK_TERMINATOR;
    else
    {
      scanner->device_lines_remaining = MAX_INVENTION_SPELLS + 5;
      scanner->device_block = RENAME_DEVICE_BLOCK_FIXED;
    }
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_FIXED:
    if (--scanner->device_lines_remaining == 0)
    {
      scanner->device_inventions_remaining--;
      scanner->device_block = RENAME_DEVICE_BLOCK_AFTER_EFFECTS;
    }
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_AFTER_EFFECTS:
    if (rename_line_equals(line, "Lvls:"))
    {
      scanner->device_lines_remaining = MAX_INVENTION_SPELLS;
      scanner->device_block = RENAME_DEVICE_BLOCK_LEVELS;
    }
    else if (scanner->device_inventions_remaining > 0)
    {
      scanner->device_lines_remaining = MAX_INVENTION_SPELLS + 4;
      scanner->device_block = RENAME_DEVICE_BLOCK_FIXED;
    }
    else
      scanner->device_block = RENAME_DEVICE_BLOCK_NONE;
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_LEVELS:
    if (--scanner->device_lines_remaining == 0)
      scanner->device_block = scanner->device_inventions_remaining > 0
                                  ? RENAME_DEVICE_BLOCK_NEXT_INDEX
                                  : RENAME_DEVICE_BLOCK_TERMINATOR;
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_NEXT_INDEX:
    scanner->device_lines_remaining = MAX_INVENTION_SPELLS + 4;
    scanner->device_block = RENAME_DEVICE_BLOCK_FIXED;
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_TERMINATOR:
    scanner->device_block = RENAME_DEVICE_BLOCK_NONE;
    return RENAME_PFILE_LINE_NESTED;

  case RENAME_DEVICE_BLOCK_NONE:
    break;
  }

  scanner->invalid = TRUE;
  return RENAME_PFILE_LINE_NESTED;
}

static enum rename_pfile_line_role rename_scan_pfile_line(struct rename_pfile_scanner *scanner,
                                                          const char *line)
{
  long count;

  if (scanner->invalid)
    return RENAME_PFILE_LINE_NESTED;

  if (scanner->opaque_lines_remaining > 0)
  {
    scanner->opaque_lines_remaining--;
    return RENAME_PFILE_LINE_NESTED;
  }

  if (scanner->device_block != RENAME_DEVICE_BLOCK_NONE)
    return rename_scan_device_line(scanner, line);

  if (scanner->tilde_block != RENAME_TILDE_BLOCK_NONE)
  {
    if (rename_tilde_block_ends(scanner->tilde_block, line))
    {
      scanner->tilde_block = RENAME_TILDE_BLOCK_NONE;
      return RENAME_PFILE_LINE_NESTED;
    }
    rename_classify_intro_block(&scanner->tilde_block, line);
    return scanner->tilde_block == RENAME_TILDE_BLOCK_INTRODUCTIONS_NAMES
               ? RENAME_PFILE_LINE_INTRODUCTION
               : RENAME_PFILE_LINE_NESTED;
  }

  scanner->tilde_block = rename_tilde_block_start(line);
  if (scanner->tilde_block != RENAME_TILDE_BLOCK_NONE)
    return RENAME_PFILE_LINE_NESTED;

  if (rename_line_has_pfile_tag(line, "Alis"))
  {
    if (!rename_parse_nonnegative_count(rename_pfile_tag_value(line), LONG_MAX / 3, &count))
      scanner->invalid = TRUE;
    else
      scanner->opaque_lines_remaining = count * 3;
    return RENAME_PFILE_LINE_NESTED;
  }

  if (rename_line_has_pfile_tag(line, "Vars"))
  {
    if (!rename_parse_nonnegative_count(rename_pfile_tag_value(line), LONG_MAX, &count))
      scanner->invalid = TRUE;
    else
      scanner->opaque_lines_remaining = count;
    return RENAME_PFILE_LINE_NESTED;
  }

  if (rename_line_has_pfile_tag(line, "Dvis"))
  {
    scanner->device_block = RENAME_DEVICE_BLOCK_COUNT;
    return RENAME_PFILE_LINE_NESTED;
  }

  return RENAME_PFILE_LINE_TOP_LEVEL;
}

static int rename_pfile_scanner_complete(const struct rename_pfile_scanner *scanner)
{
  return !scanner->invalid && scanner->tilde_block == RENAME_TILDE_BLOCK_NONE &&
         scanner->device_block == RENAME_DEVICE_BLOCK_NONE && scanner->opaque_lines_remaining == 0;
}

static FILE *rename_open_regular(const char *path, const char *mode, struct stat *file_stat)
{
  FILE *file;
  struct stat opened_stat;
  int fd;
  int flags = O_RDONLY;

#ifdef O_NOFOLLOW
  flags |= O_NOFOLLOW;
#endif

  fd = open(path, flags);
  if (fd < 0)
    return NULL;
  if (fstat(fd, &opened_stat) != 0)
  {
    close(fd);
    return NULL;
  }
  if (!S_ISREG(opened_stat.st_mode))
  {
    close(fd);
    errno = EINVAL;
    return NULL;
  }

  file = fdopen(fd, mode);
  if (!file)
  {
    close(fd);
    return NULL;
  }

  if (file_stat)
    *file_stat = opened_stat;
  return file;
}

static int rename_file_contains_intro(const char *path, const char *old_name)
{
  FILE *file;
  struct rename_pfile_scanner scanner = {0};
  char line[MAX_STRING_LENGTH + 2];
  int found = FALSE;

  file = rename_open_regular(path, "r", NULL);
  if (!file)
  {
    if (errno == ENOENT)
      return FALSE;
    return -1;
  }

  while (fgets(line, sizeof(line), file))
  {
    if (rename_scan_pfile_line(&scanner, line) == RENAME_PFILE_LINE_INTRODUCTION &&
        rename_line_equals(line, old_name))
      found = TRUE;
  }

  if (ferror(file) || !rename_pfile_scanner_complete(&scanner))
    found = -1;
  fclose(file);
  return found;
}

static int rename_source_player_identity_matches(const char *path, const char *old_name,
                                                 long player_id)
{
  FILE *file;
  struct rename_pfile_scanner scanner = {0};
  char line[MAX_STRING_LENGTH + 2];
  char *value;
  char extra;
  long parsed_id;
  int matching_names = 0;
  int name_lines = 0;
  int matching_ids = 0;
  int id_lines = 0;
  int account_lines = 0;

  file = rename_open_regular(path, "r", NULL);
  if (!file)
    return FALSE;
  while (fgets(line, sizeof(line), file))
  {
    if (rename_scan_pfile_line(&scanner, line) != RENAME_PFILE_LINE_TOP_LEVEL)
      continue;
    if (!strncmp(line, "Name:", 5))
    {
      name_lines++;
      value = line + 5;
      while (*value && isspace((unsigned char)*value))
        value++;
      if (rename_line_equals_exact(value, old_name))
        matching_names++;
    }
    else if (!strncmp(line, "Id  :", 5))
    {
      id_lines++;
      parsed_id = -1;
      extra = '\0';
      if (sscanf(line, "Id  : %ld %c", &parsed_id, &extra) == 1 && parsed_id == player_id)
        matching_ids++;
    }
    else if (!strncmp(line, "Acct:", 5))
      account_lines++;
  }
  if (ferror(file) || !rename_pfile_scanner_complete(&scanner))
  {
    fclose(file);
    return FALSE;
  }
  fclose(file);
  return name_lines == 1 && matching_names == 1 && id_lines == 1 && matching_ids == 1 &&
         account_lines <= 1;
}

static int rename_add_intro_file(struct rename_context *ctx, const char *path)
{
  struct rename_intro_file *resized;

  resized = realloc(ctx->intro_files, sizeof(struct rename_intro_file) * (ctx->intro_count + 1));
  if (!resized)
  {
    rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "allocating introduction migration plan");
    return FALSE;
  }

  ctx->intro_files = resized;
  memset(&ctx->intro_files[ctx->intro_count], 0, sizeof(struct rename_intro_file));
  strlcpy(ctx->intro_files[ctx->intro_count].path, path,
          sizeof(ctx->intro_files[ctx->intro_count].path));
  ctx->intro_count++;
  return TRUE;
}

static int rename_plan_introduction_files(struct rename_context *ctx)
{
  int i;
  int contains;
  char path[PLAYER_RENAME_PATH_SIZE];

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].id == GET_IDNUM(ctx->victim) || !player_table[i].name)
      continue;
    if (!get_filename(path, sizeof(path), PLR_FILE, player_table[i].name))
    {
      rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "planning introduction files");
      return FALSE;
    }

    contains = rename_file_contains_intro(path, ctx->old_display_name);
    if (contains < 0)
    {
      rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "reading introduction files");
      return FALSE;
    }
    if (contains && !rename_add_intro_file(ctx, path))
      return FALSE;
  }

  return TRUE;
}

static int rename_digest_regular_file(const char *path, const struct stat *expected_stat,
                                      unsigned char digest[PLAYER_RENAME_DIGEST_SIZE])
{
  EVP_MD_CTX *digest_context = NULL;
  struct stat before_stat;
  struct stat after_stat;
  unsigned char buffer[8192];
  unsigned int digest_length = 0;
  ssize_t count;
  int fd = -1;
  int ok = FALSE;
  int open_flags = O_RDONLY;

#ifdef O_NOFOLLOW
  open_flags |= O_NOFOLLOW;
#endif

  if (!path || !expected_stat || (fd = open(path, open_flags)) < 0 ||
      fstat(fd, &before_stat) != 0 || !S_ISREG(before_stat.st_mode) ||
      before_stat.st_dev != expected_stat->st_dev || before_stat.st_ino != expected_stat->st_ino ||
      before_stat.st_size != expected_stat->st_size ||
      (before_stat.st_mode & 07777) != (expected_stat->st_mode & 07777) ||
      before_stat.st_uid != expected_stat->st_uid || before_stat.st_gid != expected_stat->st_gid ||
      before_stat.st_mtim.tv_sec != expected_stat->st_mtim.tv_sec ||
      before_stat.st_mtim.tv_nsec != expected_stat->st_mtim.tv_nsec ||
      !(digest_context = EVP_MD_CTX_new()) ||
      EVP_DigestInit_ex(digest_context, EVP_sha256(), NULL) != 1)
    goto cleanup;

  while ((count = read(fd, buffer, sizeof(buffer))) != 0)
  {
    if (count < 0)
    {
      if (errno == EINTR)
        continue;
      goto cleanup;
    }
    if (EVP_DigestUpdate(digest_context, buffer, (size_t)count) != 1)
      goto cleanup;
  }

  if (fstat(fd, &after_stat) != 0 || after_stat.st_dev != before_stat.st_dev ||
      after_stat.st_ino != before_stat.st_ino || after_stat.st_size != before_stat.st_size ||
      (after_stat.st_mode & 07777) != (before_stat.st_mode & 07777) ||
      after_stat.st_uid != before_stat.st_uid || after_stat.st_gid != before_stat.st_gid ||
      after_stat.st_mtim.tv_sec != before_stat.st_mtim.tv_sec ||
      after_stat.st_mtim.tv_nsec != before_stat.st_mtim.tv_nsec ||
      EVP_DigestFinal_ex(digest_context, digest, &digest_length) != 1 ||
      digest_length != PLAYER_RENAME_DIGEST_SIZE)
    goto cleanup;

  ok = TRUE;

cleanup:
  EVP_MD_CTX_free(digest_context);
  if (fd >= 0)
    close(fd);
  return ok;
}

static int rename_plan_files(struct rename_context *ctx)
{
  int i;
  struct stat old_stat;
  struct stat new_stat;
  int modes[MAX_FILES] = {PLR_FILE, CRASH_FILE, SCRIPT_VARS_FILE, ETEXT_FILE};

  for (i = 0; i < MAX_FILES; i++)
  {
    ctx->files[i].mode = modes[i];
    ctx->files[i].required = (modes[i] == PLR_FILE);
    if (!get_filename(ctx->files[i].old_path, sizeof(ctx->files[i].old_path), modes[i],
                      ctx->old_display_name) ||
        !get_filename(ctx->files[i].new_path, sizeof(ctx->files[i].new_path), modes[i],
                      ctx->new_display_name))
    {
      rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "generating player file paths");
      return FALSE;
    }

    if (lstat(ctx->files[i].old_path, &old_stat) == 0)
    {
      if (!S_ISREG(old_stat.st_mode))
      {
        rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "checking source player file type");
        return FALSE;
      }
      ctx->files[i].exists = TRUE;
      ctx->files[i].source_stat = old_stat;
      if (i > 0 && !rename_digest_regular_file(ctx->files[i].old_path, &ctx->files[i].source_stat,
                                               ctx->files[i].source_digest))
      {
        rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "snapshotting auxiliary player file");
        return FALSE;
      }
      ctx->files[i].digest_ready = i > 0;
    }
    else if (errno != ENOENT || ctx->files[i].required)
    {
      rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "checking source player files");
      return FALSE;
    }

    if (lstat(ctx->files[i].new_path, &new_stat) == 0 || errno != ENOENT)
    {
      rename_set_failure(ctx, PLAYER_RENAME_FILE_COLLISION, "checking destination player files");
      return FALSE;
    }
  }

  if (snprintf(ctx->index_path, sizeof(ctx->index_path), "%s%s", LIB_PLRFILES, INDEX_FILE) >=
      (int)sizeof(ctx->index_path))
  {
    rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "generating player index path");
    return FALSE;
  }
  if (lstat(ctx->index_path, &old_stat) != 0 || !S_ISREG(old_stat.st_mode))
  {
    rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "checking player index file");
    return FALSE;
  }

  if (!rename_source_player_identity_matches(ctx->files[0].old_path, ctx->old_display_name,
                                             GET_IDNUM(ctx->victim)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                       "verifying source player file identity");
    return FALSE;
  }
  if (!rename_verify_old_index_file(ctx))
  {
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                       "verifying source player index identity");
    return FALSE;
  }

  return rename_plan_introduction_files(ctx);
}

static int rename_copy_to_backup(const char *source, char *backup, size_t backup_size)
{
  FILE *input = NULL;
  FILE *output = NULL;
  struct stat source_stat;
  struct timespec source_times[2];
  char buffer[8192];
  size_t count;
  int fd = -1;
  int failed = FALSE;

  if (snprintf(backup, backup_size, "%s.rename-bak.XXXXXX", source) >= (int)backup_size)
    return FALSE;

  input = rename_open_regular(source, "rb", &source_stat);
  if (!input)
    return FALSE;

  if ((fd = mkstemp(backup)) < 0)
  {
    fclose(input);
    return FALSE;
  }
  if (fchown(fd, source_stat.st_uid, source_stat.st_gid) != 0 ||
      fchmod(fd, source_stat.st_mode & 07777) != 0 || !(output = fdopen(fd, "wb")))
  {
    close(fd);
    fclose(input);
    unlink(backup);
    return FALSE;
  }

  while ((count = fread(buffer, 1, sizeof(buffer), input)) > 0)
    if (fwrite(buffer, 1, count, output) != count)
    {
      failed = TRUE;
      break;
    }

  if (ferror(input) || fflush(output) != 0 || ferror(output))
    failed = TRUE;
  source_times[0] = source_stat.st_atim;
  source_times[1] = source_stat.st_mtim;
  if (futimens(fileno(output), source_times) != 0 || fsync(fileno(output)) != 0)
    failed = TRUE;
  if (fclose(input) != 0)
    failed = TRUE;
  if (fclose(output) != 0)
    failed = TRUE;

  if (failed)
  {
    unlink(backup);
    return FALSE;
  }
  return TRUE;
}

static int rename_create_backups(struct rename_context *ctx)
{
  int i;

  if (!rename_copy_to_backup(ctx->files[0].old_path, ctx->target_backup,
                             sizeof(ctx->target_backup)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "backing up player file");
    return FALSE;
  }
  ctx->target_backup_ready = TRUE;

  if (!rename_copy_to_backup(ctx->index_path, ctx->index_backup, sizeof(ctx->index_backup)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "backing up player index");
    return FALSE;
  }
  ctx->index_backup_ready = TRUE;

  for (i = 0; i < ctx->intro_count; i++)
  {
    if (!rename_copy_to_backup(ctx->intro_files[i].path, ctx->intro_files[i].backup_path,
                               sizeof(ctx->intro_files[i].backup_path)))
    {
      rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "backing up introduction file");
      return FALSE;
    }
    ctx->intro_files[i].backup_ready = TRUE;
  }

  return TRUE;
}

static int rename_file_matches_snapshot(const char *path, const char *snapshot)
{
  FILE *current = NULL;
  FILE *saved = NULL;
  struct stat current_stat;
  struct stat saved_stat;
  unsigned char current_buffer[8192];
  unsigned char saved_buffer[8192];
  size_t current_count;
  size_t saved_count;
  int matches = FALSE;

  current = rename_open_regular(path, "rb", &current_stat);
  saved = rename_open_regular(snapshot, "rb", &saved_stat);
  if (!current || !saved || current_stat.st_size != saved_stat.st_size ||
      (current_stat.st_mode & 07777) != (saved_stat.st_mode & 07777) ||
      current_stat.st_uid != saved_stat.st_uid || current_stat.st_gid != saved_stat.st_gid ||
      current_stat.st_mtim.tv_sec != saved_stat.st_mtim.tv_sec ||
      current_stat.st_mtim.tv_nsec != saved_stat.st_mtim.tv_nsec)
    goto cleanup;

  do
  {
    current_count = fread(current_buffer, 1, sizeof(current_buffer), current);
    saved_count = fread(saved_buffer, 1, sizeof(saved_buffer), saved);
    if (current_count != saved_count ||
        (current_count > 0 && memcmp(current_buffer, saved_buffer, current_count)))
      goto cleanup;
  } while (current_count > 0);

  matches = !ferror(current) && !ferror(saved);

cleanup:
  if (current)
    fclose(current);
  if (saved)
    fclose(saved);
  return matches;
}

static int rename_file_ready_to_move(struct rename_context *ctx, int file_index)
{
  struct rename_file_move *file = &ctx->files[file_index];
  struct stat current_stat;

  if (lstat(file->old_path, &current_stat) != 0 || !S_ISREG(current_stat.st_mode) ||
      current_stat.st_dev != file->source_stat.st_dev ||
      current_stat.st_ino != file->source_stat.st_ino ||
      current_stat.st_size != file->source_stat.st_size ||
      (current_stat.st_mode & 07777) != (file->source_stat.st_mode & 07777) ||
      current_stat.st_uid != file->source_stat.st_uid ||
      current_stat.st_gid != file->source_stat.st_gid ||
      current_stat.st_mtim.tv_sec != file->source_stat.st_mtim.tv_sec ||
      current_stat.st_mtim.tv_nsec != file->source_stat.st_mtim.tv_nsec)
  {
    rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "rechecking source player file");
    return FALSE;
  }

  if (lstat(file->new_path, &current_stat) == 0 || errno != ENOENT)
  {
    rename_set_failure(ctx, PLAYER_RENAME_FILE_COLLISION, "rechecking destination player file");
    return FALSE;
  }
  return TRUE;
}

static int rename_move_files(struct rename_context *ctx)
{
  int i;

  for (i = 1; i < MAX_FILES; i++)
    if (ctx->files[i].exists)
    {
      if (rename_inject_failure(ctx, PLAYER_RENAME_TEST_FAIL_AUXILIARY_MOVE,
                                PLAYER_RENAME_FILE_ERROR, "moving auxiliary player file"))
        return FALSE;
      if (!rename_file_ready_to_move(ctx, i))
        return FALSE;
      if (rename(ctx->files[i].old_path, ctx->files[i].new_path) != 0)
      {
        log("SYSERR: Character rename could not move %s to %s: %s", ctx->files[i].old_path,
            ctx->files[i].new_path, strerror(errno));
        rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "moving auxiliary player file");
        return FALSE;
      }
      ctx->files[i].moved = TRUE;
      ctx->report->files_moved++;
    }

  if (!rename_file_ready_to_move(ctx, 0))
    return FALSE;
  if (rename(ctx->files[0].old_path, ctx->files[0].new_path) != 0)
  {
    log("SYSERR: Character rename could not move %s to %s: %s", ctx->files[0].old_path,
        ctx->files[0].new_path, strerror(errno));
    rename_set_failure(ctx, PLAYER_RENAME_FILE_ERROR, "moving player file");
    return FALSE;
  }
  ctx->files[0].moved = TRUE;
  ctx->report->files_moved++;
  return TRUE;
}

static int rename_write_replacement_line(FILE *output, const char *prefix, const char *value)
{
  return fprintf(output, "%s%s\n", prefix, value) >= 0;
}

static int rename_rewrite_player_file(const char *path, const char *old_name, const char *new_name,
                                      const char *account_name, int rewrite_identity,
                                      unsigned int *intro_changes)
{
  FILE *input = NULL;
  FILE *output = NULL;
  struct rename_pfile_scanner scanner = {0};
  struct stat source_stat;
  char temporary[PLAYER_RENAME_PATH_SIZE];
  char line[MAX_STRING_LENGTH + 2];
  enum rename_pfile_line_role line_role;
  int fd = -1;
  int failed = FALSE;
  int name_count = 0;
  int account_count = 0;

  if (snprintf(temporary, sizeof(temporary), "%s.rename-tmp.XXXXXX", path) >=
      (int)sizeof(temporary))
    return FALSE;
  input = rename_open_regular(path, "r", &source_stat);
  if (!input)
    return FALSE;
  if ((fd = mkstemp(temporary)) < 0)
  {
    fclose(input);
    return FALSE;
  }
  if (fchown(fd, source_stat.st_uid, source_stat.st_gid) != 0 ||
      fchmod(fd, source_stat.st_mode & 07777) != 0 || !(output = fdopen(fd, "w")))
  {
    close(fd);
    fclose(input);
    unlink(temporary);
    return FALSE;
  }

  while (fgets(line, sizeof(line), input))
  {
    line_role = rename_scan_pfile_line(&scanner, line);
    if (line_role != RENAME_PFILE_LINE_TOP_LEVEL)
    {
      if (line_role == RENAME_PFILE_LINE_INTRODUCTION && rename_line_equals(line, old_name))
      {
        if (!rename_write_replacement_line(output, "", new_name))
          failed = TRUE;
        if (intro_changes)
          (*intro_changes)++;
        continue;
      }
      if (fputs(line, output) == EOF)
        failed = TRUE;
      continue;
    }

    if (rewrite_identity && !strncmp(line, "Name:", 5))
    {
      name_count++;
      if (!rename_write_replacement_line(output, "Name: ", new_name))
        failed = TRUE;
      continue;
    }
    if (rewrite_identity && !strncmp(line, "Acct:", 5) && account_name)
    {
      account_count++;
      if (!rename_write_replacement_line(output, "Acct: ", account_name))
        failed = TRUE;
      continue;
    }
    if (fputs(line, output) == EOF)
      failed = TRUE;
  }

  if (rewrite_identity && account_name && account_count == 0)
    if (!rename_write_replacement_line(output, "Acct: ", account_name))
      failed = TRUE;

  if (ferror(input) || !rename_pfile_scanner_complete(&scanner) ||
      (rewrite_identity && (name_count != 1 || account_count > 1)) || fflush(output) != 0 ||
      ferror(output) || fsync(fileno(output)) != 0)
    failed = TRUE;
  if (fclose(input) != 0)
    failed = TRUE;
  if (fclose(output) != 0)
    failed = TRUE;

  if (failed || rename(temporary, path) != 0)
  {
    unlink(temporary);
    return FALSE;
  }
  return TRUE;
}

static int rename_rewrite_files(struct rename_context *ctx)
{
  int i;
  unsigned int target_changes;

  target_changes = 0;
  if (rename_inject_failure(ctx, PLAYER_RENAME_TEST_FAIL_PLAYER_FILE_WRITE,
                            PLAYER_RENAME_SAVE_ERROR, "rewriting player file"))
    return FALSE;
  if (!rename_rewrite_player_file(ctx->files[0].new_path, ctx->old_display_name,
                                  ctx->new_display_name, ctx->account_name, TRUE, &target_changes))
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "rewriting player file");
    return FALSE;
  }
  ctx->target_intro_changes = target_changes;

  for (i = 0; i < ctx->intro_count; i++)
  {
    unsigned int file_changes = 0;
    if (rename_inject_failure(ctx, PLAYER_RENAME_TEST_FAIL_INTRODUCTION_WRITE,
                              PLAYER_RENAME_SAVE_ERROR, "rewriting introduction file"))
      return FALSE;
    if (!rename_rewrite_player_file(ctx->intro_files[i].path, ctx->old_display_name,
                                    ctx->new_display_name, NULL, FALSE, &file_changes))
    {
      rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "rewriting introduction file");
      return FALSE;
    }
    ctx->intro_files[i].changed = TRUE;
    if (file_changes == 0)
    {
      rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "verifying introduction rewrite");
      return FALSE;
    }
    ctx->report->introduction_files_changed++;
  }

  return TRUE;
}

static int rename_db_query(struct rename_context *ctx, const char *query, const char *stage)
{
  if (mysql_query(conn, query))
  {
    log("SYSERR: Character rename failed during %s: %s", stage, mysql_error(conn));
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, stage);
    return FALSE;
  }
  return TRUE;
}

/* Caller holds mysql_mutex for the full rename transaction. */
static char *rename_escape_name(const char *name)
{
  char *escaped;
  size_t length;

  if (!name)
    return NULL;
  length = strlen(name);
  if (length > (((size_t)-1) - 1) / 2)
    return NULL;
  escaped = malloc(length * 2 + 1);
  if (!escaped)
    return NULL;
  mysql_real_escape_string(conn, escaped, name, length);
  return escaped;
}

static int rename_identifier_is_safe(const char *identifier)
{
  const unsigned char *scan;

  if (!identifier || !*identifier)
    return FALSE;
  for (scan = (const unsigned char *)identifier; *scan; scan++)
    if (!isalnum(*scan) && *scan != '_')
      return FALSE;
  return TRUE;
}

static int rename_load_preserve_assignments(struct rename_context *ctx, struct rename_db_key *key)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[512];
  size_t used = 0;
  int written;

  key->preserve_assignments[0] = '\0';
  snprintf(query, sizeof(query),
           "SELECT COLUMN_NAME FROM information_schema.COLUMNS "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s' "
           "AND LOWER(EXTRA) LIKE '%%on update%%' ORDER BY ORDINAL_POSITION",
           key->table);
  if (!rename_db_query(ctx, query, "checking automatic-update columns"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading automatic-update columns");
    return FALSE;
  }

  while ((row = mysql_fetch_row(result)))
  {
    if (!row[0] || !rename_identifier_is_safe(row[0]))
    {
      mysql_free_result(result);
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "validating automatic-update column");
      return FALSE;
    }
    if (!strcasecmp(row[0], key->column))
      continue;
    written = snprintf(key->preserve_assignments + used, sizeof(key->preserve_assignments) - used,
                       ", `%s`=`%s`", row[0], row[0]);
    if (written < 0 || (size_t)written >= sizeof(key->preserve_assignments) - used)
    {
      mysql_free_result(result);
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR,
                         "building automatic-update preservation");
      return FALSE;
    }
    used += (size_t)written;
  }
  mysql_free_result(result);
  return TRUE;
}

static int rename_activate_db_key(struct rename_context *ctx, struct rename_db_key *key)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[512];

  key->preserve_assignments[0] = '\0';
  snprintf(query, sizeof(query),
           "SELECT TABLE_TYPE, ENGINE FROM information_schema.TABLES "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",
           key->table);
  if (!rename_db_query(ctx, query, "checking rename table"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading rename table metadata");
    return FALSE;
  }

  row = mysql_fetch_row(result);
  if (!row)
  {
    mysql_free_result(result);
    if (key->required)
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "missing required rename table");
      return FALSE;
    }
    return TRUE;
  }

  if (!row[0] || strcasecmp(row[0], "BASE TABLE") || !row[1] || strcasecmp(row[1], "InnoDB"))
  {
    log("SYSERR: Character rename requires transactional InnoDB table %s", key->table);
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "checking transactional table engine");
    return FALSE;
  }
  mysql_free_result(result);

  snprintf(query, sizeof(query),
           "SELECT COUNT(*) FROM information_schema.COLUMNS "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s' AND COLUMN_NAME='%s'",
           key->table, key->column);
  if (!rename_db_query(ctx, query, "checking rename column"))
    return FALSE;
  if (!(result = mysql_store_result(conn)) || !(row = mysql_fetch_row(result)))
  {
    if (result)
      mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading rename column metadata");
    return FALSE;
  }
  if (!row[0] || atoll(row[0]) != 1)
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "missing active rename column");
    return FALSE;
  }
  mysql_free_result(result);
  if (!rename_load_preserve_assignments(ctx, key))
    return FALSE;
  key->active = TRUE;
  return TRUE;
}

static int rename_activate_db_keys(struct rename_context *ctx)
{
  int i;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
    if (!rename_activate_db_key(ctx, &ctx->keys[i]))
      return FALSE;
  return TRUE;
}

static int rename_require_innodb_table(struct rename_context *ctx, const char *table)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[512];

  snprintf(query, sizeof(query),
           "SELECT COUNT(*) FROM information_schema.TABLES "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s' "
           "AND TABLE_TYPE='BASE TABLE' AND ENGINE='InnoDB'",
           table);
  if (!rename_db_query(ctx, query, "checking required transactional table"))
    return FALSE;
  if (!(result = mysql_store_result(conn)) || !(row = mysql_fetch_row(result)))
  {
    if (result)
      mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR,
                       "reading required transactional table metadata");
    return FALSE;
  }
  if (!row[0] || atoll(row[0]) != 1)
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR,
                       "checking required transactional table engine");
    return FALSE;
  }
  mysql_free_result(result);
  return TRUE;
}

static int rename_revalidate_db_keys(struct rename_context *ctx)
{
  struct rename_db_key current;
  int i;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    current = ctx->keys[i];
    current.active = FALSE;
    current.preserve_assignments[0] = '\0';
    if (!rename_activate_db_key(ctx, &current))
      return FALSE;
    if (current.active != ctx->keys[i].active ||
        strcmp(current.preserve_assignments, ctx->keys[i].preserve_assignments))
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "checking rename schema stability");
      return FALSE;
    }
  }
  return TRUE;
}

static int rename_activate_level_30_view(struct rename_context *ctx)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[512];

  snprintf(query, sizeof(query),
           "SELECT TABLE_TYPE FROM information_schema.TABLES "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='level_30_characters'");
  if (!rename_db_query(ctx, query, "checking level-30 character view"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR,
                       "reading level-30 character view metadata");
    return FALSE;
  }

  row = mysql_fetch_row(result);
  if (!row)
  {
    mysql_free_result(result);
    return TRUE;
  }
  if (!row[0] || strcasecmp(row[0], "VIEW"))
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "verifying level-30 character view type");
    return FALSE;
  }
  mysql_free_result(result);

  snprintf(query, sizeof(query),
           "SELECT COUNT(*) FROM information_schema.COLUMNS "
           "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='level_30_characters' "
           "AND COLUMN_NAME='name'");
  if (!rename_db_query(ctx, query, "checking level-30 character view column"))
    return FALSE;
  if (!(result = mysql_store_result(conn)) || !(row = mysql_fetch_row(result)))
  {
    if (result)
      mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading level-30 character view column");
    return FALSE;
  }
  if (!row[0] || atoll(row[0]) != 1)
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR,
                       "verifying level-30 character view column");
    return FALSE;
  }
  mysql_free_result(result);
  ctx->level_30_view_active = TRUE;
  return TRUE;
}

static int rename_configure_player_data_shape(struct rename_context *ctx)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!rename_db_query(ctx,
                       "SELECT COLUMN_NAME FROM information_schema.COLUMNS "
                       "WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='player_data' "
                       "AND COLUMN_NAME IN "
                       "('player_idnum','id','account_id','obj_save_header')",
                       "checking canonical player schema"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading canonical player schema");
    return FALSE;
  }

  {
    int has_account_id = FALSE;
    int has_object_header = FALSE;

    while ((row = mysql_fetch_row(result)))
    {
      if (!row[0])
        continue;
      if (!strcasecmp(row[0], "player_idnum"))
        strlcpy(ctx->database_player_id_column, "player_idnum",
                sizeof(ctx->database_player_id_column));
      else if (!strcasecmp(row[0], "id") && !*ctx->database_player_id_column)
        strlcpy(ctx->database_player_id_column, "id", sizeof(ctx->database_player_id_column));
      else if (!strcasecmp(row[0], "account_id"))
        has_account_id = TRUE;
      else if (!strcasecmp(row[0], "obj_save_header"))
        has_object_header = TRUE;
    }
    mysql_free_result(result);

    if (!has_account_id || !has_object_header)
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "verifying canonical player schema");
      return FALSE;
    }
  }

  ctx->database_player_id_present = *ctx->database_player_id_column != '\0';
  return TRUE;
}

static int rename_count_db_name(struct rename_context *ctx, struct rename_db_key *key,
                                const char *escaped_name, unsigned long long *count, int lock_rows)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[1024];

  snprintf(query, sizeof(query), "SELECT COUNT(*) FROM `%s` WHERE LOWER(`%s`)=LOWER('%s')%s%s",
           key->table, key->column, escaped_name,
           key->exclude_mail_all ? " AND LOWER(`receiver`) <> LOWER('All')" : "",
           lock_rows ? " FOR UPDATE" : "");
  if (!rename_db_query(ctx, query, "counting rename rows"))
    return FALSE;
  if (!(result = mysql_store_result(conn)) || !(row = mysql_fetch_row(result)))
  {
    if (result)
      mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading rename row count");
    return FALSE;
  }
  *count = row[0] ? strtoull(row[0], NULL, 10) : 0;
  mysql_free_result(result);
  return TRUE;
}

static int rename_payload_hash_compare(const void *left, const void *right)
{
  return memcmp(left, right, PLAYER_RENAME_DIGEST_SIZE);
}

static int rename_digest_unsigned_long_long(EVP_MD_CTX *digest, unsigned long long value)
{
  unsigned char encoded[8];
  int i;

  for (i = 7; i >= 0; i--)
  {
    encoded[i] = (unsigned char)(value & 0xff);
    value >>= 8;
  }
  return EVP_DigestUpdate(digest, encoded, sizeof(encoded)) == 1;
}

static int rename_capture_payload(struct rename_context *ctx, struct rename_db_key *key,
                                  const char *escaped_name,
                                  unsigned char digest[PLAYER_RENAME_DIGEST_SIZE],
                                  unsigned long long *row_count)
{
  MYSQL_RES *result = NULL;
  MYSQL_ROW row;
  MYSQL_FIELD *fields;
  unsigned long *lengths;
  EVP_MD_CTX *row_digest = NULL;
  EVP_MD_CTX *aggregate_digest = NULL;
  unsigned char *row_hashes = NULL;
  unsigned char marker;
  unsigned int digest_length;
  unsigned int field_count;
  unsigned int field_index;
  unsigned int key_index = 0;
  unsigned int matching_key_columns = 0;
  my_ulonglong database_row_count;
  size_t allocated_row_count;
  size_t fetched_row_count = 0;
  char query[1024];
  int ok = FALSE;

  snprintf(query, sizeof(query), "SELECT * FROM `%s` WHERE LOWER(`%s`)=LOWER('%s')", key->table,
           key->column, escaped_name);
  if (!rename_db_query(ctx, query, "capturing rename payload"))
    goto cleanup;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading rename payload");
    goto cleanup;
  }

  field_count = mysql_num_fields(result);
  fields = mysql_fetch_fields(result);
  for (field_index = 0; field_index < field_count; field_index++)
    if (!strcasecmp(fields[field_index].name, key->column))
    {
      key_index = field_index;
      matching_key_columns++;
    }
  if (matching_key_columns != 1 || field_count < 2)
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "verifying rename payload schema");
    goto cleanup;
  }

  database_row_count = mysql_num_rows(result);
  allocated_row_count = (size_t)database_row_count;
  if ((my_ulonglong)allocated_row_count != database_row_count ||
      allocated_row_count > ((size_t)-1) / PLAYER_RENAME_DIGEST_SIZE)
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating rename payload snapshot");
    goto cleanup;
  }
  if (allocated_row_count > 0)
  {
    row_hashes = malloc(allocated_row_count * PLAYER_RENAME_DIGEST_SIZE);
    if (!row_hashes)
    {
      rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating rename payload snapshot");
      goto cleanup;
    }
  }

  row_digest = EVP_MD_CTX_new();
  aggregate_digest = EVP_MD_CTX_new();
  if (!row_digest || !aggregate_digest)
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating rename payload digest");
    goto cleanup;
  }

  while ((row = mysql_fetch_row(result)))
  {
    lengths = mysql_fetch_lengths(result);
    if (!lengths || fetched_row_count >= allocated_row_count ||
        EVP_DigestInit_ex(row_digest, EVP_sha256(), NULL) != 1)
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload");
      goto cleanup;
    }

    for (field_index = 0; field_index < field_count; field_index++)
    {
      /*
       * sender and receiver are both intentionally mutable in player_mail.
       * Hash every other field so a row addressed from a character to itself
       * can migrate both keys without weakening subject/message/ID proof.
       */
      if (field_index == key_index || (!strcmp(key->table, "player_mail") &&
                                       (!strcasecmp(fields[field_index].name, "sender") ||
                                        !strcasecmp(fields[field_index].name, "receiver"))))
        continue;
      marker = row[field_index] ? 1 : 0;
      if (!rename_digest_unsigned_long_long(row_digest, field_index) ||
          EVP_DigestUpdate(row_digest, &marker, sizeof(marker)) != 1)
      {
        rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload");
        goto cleanup;
      }
      if (row[field_index] &&
          (!rename_digest_unsigned_long_long(row_digest, lengths[field_index]) ||
           EVP_DigestUpdate(row_digest, row[field_index], lengths[field_index]) != 1))
      {
        rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload");
        goto cleanup;
      }
    }

    digest_length = 0;
    if (EVP_DigestFinal_ex(row_digest, row_hashes + fetched_row_count * PLAYER_RENAME_DIGEST_SIZE,
                           &digest_length) != 1 ||
        digest_length != PLAYER_RENAME_DIGEST_SIZE)
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload");
      goto cleanup;
    }
    fetched_row_count++;
  }
  if (fetched_row_count != allocated_row_count)
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "counting rename payload rows");
    goto cleanup;
  }

  if (fetched_row_count > 1)
    qsort(row_hashes, fetched_row_count, PLAYER_RENAME_DIGEST_SIZE, rename_payload_hash_compare);
  if (EVP_DigestInit_ex(aggregate_digest, EVP_sha256(), NULL) != 1 ||
      !rename_digest_unsigned_long_long(aggregate_digest, fetched_row_count) ||
      (fetched_row_count > 0 &&
       EVP_DigestUpdate(aggregate_digest, row_hashes,
                        fetched_row_count * PLAYER_RENAME_DIGEST_SIZE) != 1))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload snapshot");
    goto cleanup;
  }
  digest_length = 0;
  if (EVP_DigestFinal_ex(aggregate_digest, digest, &digest_length) != 1 ||
      digest_length != PLAYER_RENAME_DIGEST_SIZE)
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "hashing rename payload snapshot");
    goto cleanup;
  }

  *row_count = fetched_row_count;
  ok = TRUE;

cleanup:
  EVP_MD_CTX_free(row_digest);
  EVP_MD_CTX_free(aggregate_digest);
  free(row_hashes);
  if (result)
    mysql_free_result(result);
  return ok;
}

static int rename_capture_payloads(struct rename_context *ctx, const char *escaped_name,
                                   int verify_snapshot)
{
  unsigned char digest[PLAYER_RENAME_DIGEST_SIZE];
  unsigned long long row_count;
  int i;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    if (!ctx->keys[i].active || !ctx->keys[i].preserve_payload)
      continue;
    if (!rename_capture_payload(ctx, &ctx->keys[i], escaped_name, digest, &row_count))
      return FALSE;
    if (verify_snapshot)
    {
      if (row_count != ctx->keys[i].payload_rows ||
          memcmp(digest, ctx->keys[i].payload_digest, sizeof(digest)))
      {
        rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                           "verifying preserved database payload");
        return FALSE;
      }
    }
    else
    {
      ctx->keys[i].payload_rows = row_count;
      memcpy(ctx->keys[i].payload_digest, digest, sizeof(digest));
    }
  }
  return TRUE;
}

static int rename_lock_canonical_player(struct rename_context *ctx)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[1024];
  my_ulonglong rows;

  snprintf(query, sizeof(query),
           "SELECT name, %s, account_id, obj_save_header IS NULL, "
           "SHA2(COALESCE(obj_save_header, ''), 256) "
           "FROM player_data "
           "WHERE LOWER(name)=LOWER('%s') FOR UPDATE",
           ctx->database_player_id_present ? ctx->database_player_id_column : "0",
           ctx->escaped_old_name);
  if (!rename_db_query(ctx, query, "locking canonical player row"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading canonical player row");
    return FALSE;
  }

  rows = mysql_num_rows(result);
  if (rows != 1)
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_PLAYER_NOT_FOUND, "verifying canonical player row");
    return FALSE;
  }
  row = mysql_fetch_row(result);
  if (!row || !row[0] || strlen(row[0]) > MAX_NAME_LENGTH ||
      strcasecmp(row[0], ctx->old_display_name) || !row[3] || !row[4] ||
      (ctx->database_player_id_present && !row[1]))
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "capturing canonical player identity");
    return FALSE;
  }

  strlcpy(ctx->old_database_name, row[0], sizeof(ctx->old_database_name));
  ctx->database_player_id = row[1] ? strtoull(row[1], NULL, 10) : 0;
  ctx->object_header_is_null = atoi(row[3]) != 0;
  strlcpy(ctx->object_header_hash, row[4], sizeof(ctx->object_header_hash));
  if (row[2])
  {
    ctx->report->account_id = atoi(row[2]);
    ctx->report->account_linked = TRUE;
  }
  else
  {
    ctx->report->account_id = -1;
    ctx->report->account_linked = FALSE;
  }
  mysql_free_result(result);

  if (ctx->victim->desc && ctx->victim->desc->account &&
      (!ctx->report->account_linked || ctx->victim->desc->account->id != ctx->report->account_id))
  {
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "verifying live account identity");
    return FALSE;
  }

  if (ctx->report->account_linked)
  {
    snprintf(query, sizeof(query), "SELECT name FROM account_data WHERE id=%d FOR UPDATE",
             ctx->report->account_id);
    if (!rename_db_query(ctx, query, "loading owning account"))
      return FALSE;
    if (!(result = mysql_store_result(conn)) || mysql_num_rows(result) != 1 ||
        !(row = mysql_fetch_row(result)) || !row[0] || !*row[0])
    {
      if (result)
        mysql_free_result(result);
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "verifying owning account");
      return FALSE;
    }
    ctx->account_name = strdup(row[0]);
    mysql_free_result(result);
    if (!ctx->account_name)
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "copying owning account name");
      return FALSE;
    }
    if (ctx->victim->desc && ctx->victim->desc->account &&
        (!ctx->victim->desc->account->name ||
         strcmp(ctx->victim->desc->account->name, ctx->account_name)))
    {
      rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "verifying live account name");
      return FALSE;
    }
  }

  return TRUE;
}

static int rename_preflight_db_rows(struct rename_context *ctx)
{
  int i;
  unsigned long long destination_count;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    if (!ctx->keys[i].active)
      continue;
    if (!rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_new_name, &destination_count, TRUE))
      return FALSE;
    if (destination_count != 0)
    {
      rename_set_failure(ctx, PLAYER_RENAME_NAME_EXISTS, "checking destination database rows");
      return FALSE;
    }
    if (!rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_old_name, &ctx->keys[i].old_count,
                              TRUE))
      return FALSE;
  }

  if (ctx->keys[0].old_count != 1)
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "counting canonical player rows");
    return FALSE;
  }

  if (ctx->level_30_view_active)
  {
    struct rename_db_key view_key = {
        "level_30_characters", "name", FALSE, FALSE, FALSE, TRUE, 0, 0, {0}, 0, {0}};
    if (!rename_count_db_name(ctx, &view_key, ctx->escaped_old_name, &ctx->level_30_view_old_count,
                              FALSE))
      return FALSE;
  }
  return rename_capture_payloads(ctx, ctx->escaped_old_name, FALSE);
}

static int rename_update_db_rows(struct rename_context *ctx)
{
  int i;
  int written;
  char query[1024];
  my_ulonglong affected;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    if (!ctx->keys[i].active)
      continue;

    if (rename_inject_failure(ctx, PLAYER_RENAME_TEST_FAIL_DATABASE_UPDATE,
                              PLAYER_RENAME_DATABASE_ERROR, ctx->keys[i].table))
      return FALSE;
    written = snprintf(
        query, sizeof(query), "UPDATE `%s` SET `%s`='%s'%s WHERE LOWER(`%s`)=LOWER('%s')%s",
        ctx->keys[i].table, ctx->keys[i].column, ctx->escaped_new_name,
        ctx->keys[i].preserve_assignments, ctx->keys[i].column, ctx->escaped_old_name,
        ctx->keys[i].exclude_mail_all ? " AND LOWER(`receiver`) <> LOWER('All')" : "");
    if (written < 0 || written >= (int)sizeof(query))
    {
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "building database key migration");
      return FALSE;
    }
    if (!rename_db_query(ctx, query, ctx->keys[i].table))
      return FALSE;

    affected = mysql_affected_rows(conn);
    ctx->keys[i].affected_count = affected;
    if (affected != ctx->keys[i].old_count)
    {
      log("SYSERR: Character rename changed %llu of %llu expected rows in %s.%s",
          (unsigned long long)affected, ctx->keys[i].old_count, ctx->keys[i].table,
          ctx->keys[i].column);
      rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "verifying affected database rows");
      return FALSE;
    }
    ctx->report->database_rows_changed += (unsigned int)affected;
    if (!strcmp(ctx->keys[i].table, "player_save_objs"))
      ctx->report->object_rows_changed = (unsigned int)affected;
  }
  return TRUE;
}

static int rename_verify_db_rows(struct rename_context *ctx)
{
  int i;
  unsigned long long old_count;
  unsigned long long new_count;

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    if (!ctx->keys[i].active)
      continue;
    if (!rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_old_name, &old_count, FALSE) ||
        !rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_new_name, &new_count, FALSE))
      return FALSE;
    if (old_count != 0 || new_count != ctx->keys[i].old_count)
    {
      rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                         "verifying database postconditions");
      return FALSE;
    }
  }
  return rename_capture_payloads(ctx, ctx->escaped_new_name, TRUE);
}

static int rename_verify_canonical_player(struct rename_context *ctx, const char *escaped_name,
                                          const char *expected_name, const char *stage)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[1024];
  my_ulonglong rows;

  snprintf(query, sizeof(query),
           "SELECT name, %s, account_id, obj_save_header IS NULL, "
           "SHA2(COALESCE(obj_save_header, ''), 256) "
           "FROM player_data WHERE LOWER(name)=LOWER('%s')",
           ctx->database_player_id_present ? ctx->database_player_id_column : "0", escaped_name);
  if (!rename_db_query(ctx, query, stage))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading canonical player identity");
    return FALSE;
  }

  rows = mysql_num_rows(result);
  row = rows == 1 ? mysql_fetch_row(result) : NULL;
  if (!row || !row[0] || strcmp(row[0], expected_name) || !row[3] || !row[4] ||
      (ctx->database_player_id_present &&
       (!row[1] || strtoull(row[1], NULL, 10) != ctx->database_player_id)) ||
      (atoi(row[3]) != 0) != ctx->object_header_is_null ||
      strcmp(row[4], ctx->object_header_hash) ||
      (ctx->report->account_linked ? (!row[2] || atoi(row[2]) != ctx->report->account_id)
                                   : row[2] != NULL))
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, stage);
    return FALSE;
  }
  mysql_free_result(result);
  return TRUE;
}

static int rename_verify_level_30_view(struct rename_context *ctx)
{
  struct rename_db_key view_key = {
      "level_30_characters", "name", FALSE, FALSE, FALSE, TRUE, 0, 0, {0}, 0, {0}};
  unsigned long long old_count;
  unsigned long long new_count;

  if (!ctx->level_30_view_active)
    return TRUE;
  if (!rename_count_db_name(ctx, &view_key, ctx->escaped_old_name, &old_count, FALSE) ||
      !rename_count_db_name(ctx, &view_key, ctx->escaped_new_name, &new_count, FALSE))
    return FALSE;
  if (old_count != 0 || new_count != ctx->level_30_view_old_count)
  {
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                       "verifying level-30 character view");
    return FALSE;
  }
  return TRUE;
}

static int rename_add_account_refresh(struct rename_context *ctx, struct account_data *account)
{
  struct rename_account_refresh *resized;
  int i;

  for (i = 0; i < ctx->account_refresh_count; i++)
    if (ctx->account_refreshes[i].account == account)
      return TRUE;

  resized = realloc(ctx->account_refreshes,
                    sizeof(struct rename_account_refresh) * (ctx->account_refresh_count + 1));
  if (!resized)
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating account cache refresh plan");
    return FALSE;
  }

  ctx->account_refreshes = resized;
  memset(&ctx->account_refreshes[ctx->account_refresh_count], 0,
         sizeof(struct rename_account_refresh));
  ctx->account_refreshes[ctx->account_refresh_count].account = account;
  ctx->account_refresh_count++;
  return TRUE;
}

static int rename_prepare_account_refreshes(struct rename_context *ctx)
{
  struct descriptor_data *descriptor;
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[256];
  int account_index;
  int character_index = 0;
  int new_name_count = 0;

  if (!ctx->report->account_linked)
    return TRUE;

  for (descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
    if (descriptor->account && descriptor->account->id == ctx->report->account_id)
      if (!rename_add_account_refresh(ctx, descriptor->account))
        return FALSE;

  if (ctx->account_refresh_count == 0)
    return TRUE;

  snprintf(query, sizeof(query),
           "SELECT name FROM player_data WHERE account_id=%d ORDER BY name FOR UPDATE",
           ctx->report->account_id);
  if (!rename_db_query(ctx, query, "preparing account cache refresh"))
    return FALSE;
  if (!(result = mysql_store_result(conn)))
  {
    rename_set_failure(ctx, PLAYER_RENAME_DATABASE_ERROR, "reading account cache refresh");
    return FALSE;
  }
  if (mysql_num_rows(result) > MAX_CHARS_PER_ACCOUNT)
  {
    mysql_free_result(result);
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "checking account character limit");
    return FALSE;
  }

  while ((row = mysql_fetch_row(result)))
  {
    if (!row[0] || character_index >= MAX_CHARS_PER_ACCOUNT)
    {
      mysql_free_result(result);
      rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                         "reading account character identity");
      return FALSE;
    }
    if (!strcasecmp(row[0], ctx->old_display_name))
    {
      mysql_free_result(result);
      rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED,
                         "checking account cache old name");
      return FALSE;
    }
    if (!strcmp(row[0], ctx->new_display_name))
      new_name_count++;

    for (account_index = 0; account_index < ctx->account_refresh_count; account_index++)
    {
      ctx->account_refreshes[account_index].character_names[character_index] = strdup(row[0]);
      if (!ctx->account_refreshes[account_index].character_names[character_index])
      {
        mysql_free_result(result);
        rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "copying account character cache");
        return FALSE;
      }
    }
    character_index++;
  }
  mysql_free_result(result);

  if (new_name_count != 1)
  {
    rename_set_failure(ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "checking account cache new name");
    return FALSE;
  }
  return TRUE;
}

static void rename_apply_account_refreshes(struct rename_context *ctx)
{
  struct rename_account_refresh *refresh;
  int account_index;
  int character_index;

  for (account_index = 0; account_index < ctx->account_refresh_count; account_index++)
  {
    refresh = &ctx->account_refreshes[account_index];

    /*
     * Preserve the normal account refresh side effects, then install the
     * transactionally staged snapshot so a post-commit query failure cannot
     * leave a connected descriptor stale or empty.
     */
    load_account_characters(refresh->account);
    for (character_index = 0; character_index < MAX_CHARS_PER_ACCOUNT; character_index++)
    {
      free(refresh->account->character_names[character_index]);
      refresh->account->character_names[character_index] =
          refresh->character_names[character_index];
      refresh->character_names[character_index] = NULL;
    }
  }
}

static int rename_change_memory(struct rename_context *ctx)
{
  ctx->new_player_index_name = strdup(ctx->new_index_name);
  ctx->new_victim_name = strdup(ctx->new_display_name);
  if (!ctx->new_player_index_name || !ctx->new_victim_name)
  {
    free(ctx->new_player_index_name);
    free(ctx->new_victim_name);
    ctx->new_player_index_name = NULL;
    ctx->new_victim_name = NULL;
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating renamed character identity");
    return FALSE;
  }

  ctx->old_player_index_name = player_table[ctx->player_index_position].name;
  ctx->old_victim_name = GET_PC_NAME(ctx->victim);
  player_table[ctx->player_index_position].name = ctx->new_player_index_name;
  GET_PC_NAME(ctx->victim) = ctx->new_victim_name;
  ctx->memory_changed = TRUE;
  return TRUE;
}

static int rename_verify_player_file(struct rename_context *ctx)
{
  FILE *file;
  struct rename_pfile_scanner scanner = {0};
  char line[MAX_STRING_LENGTH + 2];
  char *value;
  char extra;
  long parsed_id;
  int name_count = 0;
  int name_lines = 0;
  int account_count = 0;
  int account_lines = 0;
  int id_count = 0;
  int id_lines = 0;

  file = rename_open_regular(ctx->files[0].new_path, "r", NULL);
  if (!file)
    return FALSE;

  while (fgets(line, sizeof(line), file))
  {
    enum rename_pfile_line_role line_role = rename_scan_pfile_line(&scanner, line);

    if (line_role != RENAME_PFILE_LINE_TOP_LEVEL)
    {
      if (line_role == RENAME_PFILE_LINE_INTRODUCTION &&
          rename_line_equals(line, ctx->old_display_name))
      {
        fclose(file);
        return FALSE;
      }
      continue;
    }

    if (!strncmp(line, "Name:", 5))
    {
      name_lines++;
      value = line + 5;
      while (*value && isspace((unsigned char)*value))
        value++;
      if (rename_line_equals_exact(value, ctx->new_display_name))
        name_count++;
    }
    else if (!strncmp(line, "Acct:", 5) && ctx->account_name)
    {
      account_lines++;
      value = line + 5;
      while (*value && isspace((unsigned char)*value))
        value++;
      if (rename_line_equals_exact(value, ctx->account_name))
        account_count++;
    }
    else if (!strncmp(line, "Id  :", 5))
    {
      id_lines++;
      parsed_id = -1;
      extra = '\0';
      if (sscanf(line, "Id  : %ld %c", &parsed_id, &extra) == 1 &&
          parsed_id == ctx->report->player_id)
        id_count++;
    }
  }

  if (ferror(file) || !rename_pfile_scanner_complete(&scanner))
  {
    fclose(file);
    return FALSE;
  }
  fclose(file);
  return name_lines == 1 && name_count == 1 && id_lines == 1 && id_count == 1 &&
         (!ctx->account_name || (account_lines == 1 && account_count == 1));
}

static int rename_verify_index_file(struct rename_context *ctx)
{
  FILE *file;
  char line[MAX_INPUT_LENGTH];
  char name[MAX_NAME_LENGTH + 1];
  long id;
  int target_count = 0;

  file = rename_open_regular(ctx->index_path, "r", NULL);
  if (!file)
    return FALSE;
  while (fgets(line, sizeof(line), file))
  {
    name[0] = '\0';
    id = -1;
    if (sscanf(line, "%ld %20s", &id, name) != 2)
      continue;
    if (!strcasecmp(name, ctx->old_display_name))
    {
      fclose(file);
      return FALSE;
    }
    if (id == GET_IDNUM(ctx->victim) && !strcmp(name, ctx->new_index_name))
      target_count++;
  }
  if (ferror(file))
  {
    fclose(file);
    return FALSE;
  }
  fclose(file);
  return target_count == 1;
}

static int rename_verify_files(struct rename_context *ctx)
{
  int i;
  struct stat file_stat;
  unsigned char digest[PLAYER_RENAME_DIGEST_SIZE];

  for (i = 0; i < MAX_FILES; i++)
  {
    if (!ctx->files[i].exists)
      continue;
    if (lstat(ctx->files[i].old_path, &file_stat) == 0 || errno != ENOENT ||
        lstat(ctx->files[i].new_path, &file_stat) != 0 || !S_ISREG(file_stat.st_mode) ||
        (file_stat.st_mode & 07777) != (ctx->files[i].source_stat.st_mode & 07777) ||
        file_stat.st_uid != ctx->files[i].source_stat.st_uid ||
        file_stat.st_gid != ctx->files[i].source_stat.st_gid)
      return FALSE;
    if (i > 0 && (file_stat.st_dev != ctx->files[i].source_stat.st_dev ||
                  file_stat.st_ino != ctx->files[i].source_stat.st_ino ||
                  file_stat.st_size != ctx->files[i].source_stat.st_size ||
                  file_stat.st_mtim.tv_sec != ctx->files[i].source_stat.st_mtim.tv_sec ||
                  file_stat.st_mtim.tv_nsec != ctx->files[i].source_stat.st_mtim.tv_nsec))
      return FALSE;
    if (i > 0 &&
        (!ctx->files[i].digest_ready ||
         !rename_digest_regular_file(ctx->files[i].new_path, &ctx->files[i].source_stat, digest) ||
         memcmp(digest, ctx->files[i].source_digest, sizeof(digest))))
      return FALSE;
  }

  for (i = 0; i < ctx->intro_count; i++)
    if (rename_file_contains_intro(ctx->intro_files[i].path, ctx->old_display_name) != FALSE ||
        rename_file_contains_intro(ctx->intro_files[i].path, ctx->new_display_name) != TRUE)
      return FALSE;

  if (ctx->target_intro_changes > 0 &&
      rename_file_contains_intro(ctx->files[0].new_path, ctx->new_display_name) != TRUE)
    return FALSE;

  return rename_verify_player_file(ctx) && rename_verify_index_file(ctx);
}

static int rename_verify_old_index_file(struct rename_context *ctx)
{
  FILE *file;
  char line[MAX_INPUT_LENGTH];
  char name[MAX_NAME_LENGTH + 1];
  long id;
  int matching_identity_count = 0;
  int player_id_count = 0;
  int old_name_count = 0;

  file = rename_open_regular(ctx->index_path, "r", NULL);
  if (!file)
    return FALSE;
  while (fgets(line, sizeof(line), file))
  {
    name[0] = '\0';
    id = -1;
    if (sscanf(line, "%ld %20s", &id, name) != 2)
      continue;
    if (!strcasecmp(name, ctx->new_display_name))
    {
      fclose(file);
      return FALSE;
    }
    if (id == ctx->report->player_id)
      player_id_count++;
    if (!strcasecmp(name, ctx->old_display_name))
      old_name_count++;
    if (id == ctx->report->player_id && !strcasecmp(name, ctx->old_display_name))
      matching_identity_count++;
  }
  if (ferror(file))
  {
    fclose(file);
    return FALSE;
  }
  fclose(file);
  return matching_identity_count == 1 && player_id_count == 1 && old_name_count == 1;
}

static int rename_verify_file_rollback(struct rename_context *ctx)
{
  struct stat restored_stat;
  unsigned char digest[PLAYER_RENAME_DIGEST_SIZE];
  int i;

  for (i = 0; i < MAX_FILES; i++)
  {
    if (!ctx->files[i].exists)
      continue;
    if (lstat(ctx->files[i].new_path, &restored_stat) == 0 || errno != ENOENT ||
        lstat(ctx->files[i].old_path, &restored_stat) != 0 || !S_ISREG(restored_stat.st_mode) ||
        (restored_stat.st_mode & 07777) != (ctx->files[i].source_stat.st_mode & 07777) ||
        restored_stat.st_uid != ctx->files[i].source_stat.st_uid ||
        restored_stat.st_gid != ctx->files[i].source_stat.st_gid ||
        restored_stat.st_dev != ctx->files[i].source_stat.st_dev ||
        restored_stat.st_size != ctx->files[i].source_stat.st_size ||
        restored_stat.st_mtim.tv_sec != ctx->files[i].source_stat.st_mtim.tv_sec ||
        restored_stat.st_mtim.tv_nsec != ctx->files[i].source_stat.st_mtim.tv_nsec)
      return FALSE;
    if (i == 0)
    {
      if (!rename_source_player_identity_matches(ctx->files[i].old_path, ctx->old_display_name,
                                                 ctx->report->player_id))
        return FALSE;
    }
    else if (restored_stat.st_ino != ctx->files[i].source_stat.st_ino)
      return FALSE;
    else if (!ctx->files[i].digest_ready ||
             !rename_digest_regular_file(ctx->files[i].old_path, &ctx->files[i].source_stat,
                                         digest) ||
             memcmp(digest, ctx->files[i].source_digest, sizeof(digest)))
      return FALSE;
  }

  if (ctx->target_backup_ready &&
      !rename_file_matches_snapshot(ctx->files[0].old_path, ctx->target_backup))
    return FALSE;
  if (ctx->index_backup_ready && !rename_file_matches_snapshot(ctx->index_path, ctx->index_backup))
    return FALSE;

  for (i = 0; i < ctx->intro_count; i++)
    if ((ctx->intro_files[i].backup_ready &&
         !rename_file_matches_snapshot(ctx->intro_files[i].path,
                                       ctx->intro_files[i].backup_path)) ||
        rename_file_contains_intro(ctx->intro_files[i].path, ctx->old_display_name) != TRUE)
      return FALSE;

  return !ctx->index_written || rename_verify_old_index_file(ctx);
}

static int rename_verify_database_rollback(struct rename_context *ctx)
{
  enum player_rename_status original_status = ctx->status;
  char original_stage[sizeof(ctx->report->failure_stage)];
  unsigned long long old_count;
  unsigned long long new_count;
  int rollback_ok = TRUE;
  int i;

  strlcpy(original_stage, ctx->report->failure_stage, sizeof(original_stage));
  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])); i++)
  {
    if (!ctx->keys[i].active)
      continue;
    if (!rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_old_name, &old_count, FALSE) ||
        !rename_count_db_name(ctx, &ctx->keys[i], ctx->escaped_new_name, &new_count, FALSE))
    {
      rollback_ok = FALSE;
      break;
    }
    if (old_count != ctx->keys[i].old_count || new_count != 0)
    {
      log("SYSERR: Character rename rollback left %llu old and %llu new rows in %s.%s; "
          "expected %llu old and zero new",
          old_count, new_count, ctx->keys[i].table, ctx->keys[i].column, ctx->keys[i].old_count);
      rollback_ok = FALSE;
    }
  }
  if (rollback_ok && !rename_capture_payloads(ctx, ctx->escaped_old_name, TRUE))
    rollback_ok = FALSE;
  if (rollback_ok &&
      !rename_verify_canonical_player(ctx, ctx->escaped_old_name, ctx->old_database_name,
                                      "verifying canonical player rollback"))
    rollback_ok = FALSE;
  ctx->status = original_status;
  ctx->report->status = original_status;
  strlcpy(ctx->report->failure_stage, original_stage, sizeof(ctx->report->failure_stage));
  return rollback_ok;
}

static int rename_restore_backup(char *backup, const char *destination)
{
  char restoration[PLAYER_RENAME_PATH_SIZE];

  if (!*backup)
    return TRUE;
  if (!rename_copy_to_backup(backup, restoration, sizeof(restoration)))
  {
    log("SYSERR: Character rename could not copy rollback snapshot %s: %s", backup,
        strerror(errno));
    return FALSE;
  }
  if (rename(restoration, destination) != 0)
  {
    log("SYSERR: Character rename could not restore %s to %s: %s", backup, destination,
        strerror(errno));
    unlink(restoration);
    return FALSE;
  }
  return TRUE;
}

static void rename_discard_backup(char *backup, int *ready)
{
  if (!ready || !*ready)
    return;
  if (unlink(backup) != 0 && errno != ENOENT)
    log("SYSERR: Character rename could not remove rollback snapshot %s: %s", backup,
        strerror(errno));
  *ready = FALSE;
}

static int rename_rollback(struct rename_context *ctx)
{
  int i;
  int rollback_ok = TRUE;

  if (ctx->transaction_started)
  {
    if (mysql_query(conn, "ROLLBACK"))
    {
      log("SYSERR: Character rename database rollback failed: %s", mysql_error(conn));
      rollback_ok = FALSE;
    }
    ctx->transaction_started = FALSE;
  }
  if (ctx->database_updates_attempted && !rename_verify_database_rollback(ctx))
    rollback_ok = FALSE;

  if (ctx->memory_changed)
  {
    player_table[ctx->player_index_position].name = ctx->old_player_index_name;
    GET_PC_NAME(ctx->victim) = ctx->old_victim_name;
    free(ctx->new_player_index_name);
    free(ctx->new_victim_name);
    ctx->new_player_index_name = NULL;
    ctx->new_victim_name = NULL;
    ctx->memory_changed = FALSE;
  }

  if (ctx->index_written && ctx->index_backup_ready)
  {
    if (!rename_restore_backup(ctx->index_backup, ctx->index_path))
      rollback_ok = FALSE;
  }

  for (i = ctx->intro_count - 1; i >= 0; i--)
    if (ctx->intro_files[i].changed && ctx->intro_files[i].backup_ready)
    {
      if (!rename_restore_backup(ctx->intro_files[i].backup_path, ctx->intro_files[i].path))
        rollback_ok = FALSE;
    }

  if (ctx->files[0].moved)
  {
    if (ctx->target_backup_ready)
    {
      if (!rename_restore_backup(ctx->target_backup, ctx->files[0].old_path))
        rollback_ok = FALSE;
      else if (unlink(ctx->files[0].new_path) != 0 && errno != ENOENT)
        rollback_ok = FALSE;
    }
    else if (rename(ctx->files[0].new_path, ctx->files[0].old_path) != 0)
      rollback_ok = FALSE;
  }

  for (i = MAX_FILES - 1; i >= 1; i--)
    if (ctx->files[i].moved)
    {
      struct stat restored_stat;

      if (rename(ctx->files[i].new_path, ctx->files[i].old_path) != 0)
      {
        log("SYSERR: Character rename could not roll back %s to %s: %s", ctx->files[i].new_path,
            ctx->files[i].old_path, strerror(errno));
        rollback_ok = FALSE;
      }
      else if (lstat(ctx->files[i].old_path, &restored_stat) != 0 ||
               restored_stat.st_dev != ctx->files[i].source_stat.st_dev ||
               restored_stat.st_ino != ctx->files[i].source_stat.st_ino ||
               restored_stat.st_size != ctx->files[i].source_stat.st_size ||
               (restored_stat.st_mode & 07777) != (ctx->files[i].source_stat.st_mode & 07777) ||
               restored_stat.st_uid != ctx->files[i].source_stat.st_uid ||
               restored_stat.st_gid != ctx->files[i].source_stat.st_gid ||
               restored_stat.st_mtim.tv_sec != ctx->files[i].source_stat.st_mtim.tv_sec ||
               restored_stat.st_mtim.tv_nsec != ctx->files[i].source_stat.st_mtim.tv_nsec)
      {
        log("SYSERR: Character rename could not verify rolled-back auxiliary file %s",
            ctx->files[i].old_path);
        rollback_ok = FALSE;
      }
    }

  if (ctx->target_backup_ready && !rename_verify_file_rollback(ctx))
  {
    log("SYSERR: Character rename could not verify filesystem rollback for player ID %ld",
        ctx->report->player_id);
    rollback_ok = FALSE;
  }

  if (rollback_ok)
  {
    rename_discard_backup(ctx->target_backup, &ctx->target_backup_ready);
    rename_discard_backup(ctx->index_backup, &ctx->index_backup_ready);
    for (i = 0; i < ctx->intro_count; i++)
      rename_discard_backup(ctx->intro_files[i].backup_path, &ctx->intro_files[i].backup_ready);
  }

  ctx->report->rollback_succeeded = rollback_ok;
  return rollback_ok;
}

static void rename_remove_backups(struct rename_context *ctx)
{
  int i;

  rename_discard_backup(ctx->target_backup, &ctx->target_backup_ready);
  rename_discard_backup(ctx->index_backup, &ctx->index_backup_ready);
  for (i = 0; i < ctx->intro_count; i++)
    rename_discard_backup(ctx->intro_files[i].backup_path, &ctx->intro_files[i].backup_ready);
}

static int rename_is_name_boundary(char character)
{
  return character == '\0' || !isalpha((unsigned char)character);
}

static int rename_build_generated_name(const char *field, const char *old_name,
                                       const char *new_name, char **replacement)
{
  const char *scan;
  char *write;
  size_t old_length;
  size_t new_length;
  size_t count = 0;
  size_t field_length;

  *replacement = NULL;
  if (!field)
    return TRUE;
  old_length = strlen(old_name);
  new_length = strlen(new_name);
  field_length = strlen(field);

  for (scan = field; *scan; scan++)
    if ((scan == field || rename_is_name_boundary(scan[-1])) &&
        !strncasecmp(scan, old_name, old_length) && rename_is_name_boundary(scan[old_length]))
    {
      count++;
      scan += old_length - 1;
    }
  if (!count)
    return TRUE;

  *replacement = malloc(field_length - count * old_length + count * new_length + 1);
  if (!*replacement)
    return FALSE;
  write = *replacement;
  scan = field;
  while (*scan)
  {
    if ((scan == field || rename_is_name_boundary(scan[-1])) &&
        !strncasecmp(scan, old_name, old_length) && rename_is_name_boundary(scan[old_length]))
    {
      memcpy(write, new_name, new_length);
      write += new_length;
      scan += old_length;
    }
    else
      *write++ = *scan++;
  }
  *write = '\0';
  return TRUE;
}

#ifdef LUMINARI_CUTEST
static void rename_replace_generated_name(char **field, const char *old_name, const char *new_name)
{
  char *replacement;

  if (!field || !*field)
    return;
  if (!rename_build_generated_name(*field, old_name, new_name, &replacement) || !replacement)
    return;
  free(*field);
  *field = replacement;
}
#endif

static int rename_add_live_string(struct rename_context *ctx, char **field, char *replacement)
{
  struct rename_live_string *resized;
  int i;

  for (i = 0; i < ctx->live_string_count; i++)
    if (ctx->live_strings[i].field == field)
    {
      free(replacement);
      return TRUE;
    }

  resized =
      realloc(ctx->live_strings, sizeof(struct rename_live_string) * (ctx->live_string_count + 1));
  if (!resized)
  {
    free(replacement);
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "allocating live name refresh plan");
    return FALSE;
  }

  ctx->live_strings = resized;
  ctx->live_strings[ctx->live_string_count].field = field;
  ctx->live_strings[ctx->live_string_count].replacement = replacement;
  ctx->live_string_count++;
  return TRUE;
}

static int rename_plan_exact_live_string(struct rename_context *ctx, char **field)
{
  char *replacement;

  replacement = strdup(ctx->new_display_name);
  if (!replacement)
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "copying live name refresh");
    return FALSE;
  }
  return rename_add_live_string(ctx, field, replacement);
}

static int rename_plan_generated_live_string(struct rename_context *ctx, char **field)
{
  char *replacement;

  if (!field || !*field)
    return TRUE;
  if (!rename_build_generated_name(*field, ctx->old_display_name, ctx->new_display_name,
                                   &replacement))
  {
    rename_set_failure(ctx, PLAYER_RENAME_SAVE_ERROR, "building live name refresh");
    return FALSE;
  }
  if (!replacement)
    return TRUE;
  return rename_add_live_string(ctx, field, replacement);
}

static int rename_prepare_live_strings(struct rename_context *ctx)
{
  struct char_data *character;
  struct obj_data *object;
  room_rnum room;
  int i;

  for (character = character_list; character; character = character->next)
  {
    if (!IS_NPC(character) && character->player_specials)
      for (i = 0; i < MAX_INTROS && character->player_specials->saved.intro_list[i]; i++)
        if (!strcasecmp(character->player_specials->saved.intro_list[i], ctx->old_display_name))
          if (!rename_plan_exact_live_string(ctx, &character->player_specials->saved.intro_list[i]))
            return FALSE;

    if (IS_NPC(character) && GET_MOB_VNUM(character) == MOB_CLONE && character->master &&
        !IS_NPC(character->master) && GET_IDNUM(character->master) == ctx->report->player_id)
    {
      if (!rename_plan_exact_live_string(ctx, &character->player.name) ||
          !rename_plan_exact_live_string(ctx, &character->player.short_descr))
        return FALSE;
    }

    if (IS_NPC(character) && character->mission_owner == ctx->report->player_id)
      if (!rename_plan_generated_live_string(ctx, &character->player.name) ||
          !rename_plan_generated_live_string(ctx, &character->player.long_descr))
        return FALSE;
  }

  for (room = 0; room <= top_of_world; room++)
    if (world[room].trail_tracks)
    {
      struct trail_data *trail;
      for (trail = world[room].trail_tracks->head; trail; trail = trail->next)
        if (trail->name && !strcasecmp(trail->name, ctx->old_display_name))
          if (!rename_plan_exact_live_string(ctx, &trail->name))
            return FALSE;
    }

  for (object = object_list; object; object = object->next)
    if (object->name && !strncmp(object->name, "pcorpse ", 8) &&
        GET_OBJ_VAL(object, 4) == ctx->report->player_id && GET_OBJ_VAL(object, 3) == 1)
      if (!rename_plan_generated_live_string(ctx, &object->name) ||
          !rename_plan_generated_live_string(ctx, &object->short_description) ||
          !rename_plan_generated_live_string(ctx, &object->description))
        return FALSE;

  return TRUE;
}

static void rename_apply_live_strings(struct rename_context *ctx)
{
  int i;

  for (i = 0; i < ctx->live_string_count; i++)
  {
    free(*ctx->live_strings[i].field);
    *ctx->live_strings[i].field = ctx->live_strings[i].replacement;
    ctx->live_strings[i].replacement = NULL;
  }
}

#ifdef LUMINARI_CUTEST
enum player_rename_status player_rename_validate_name_for_test(const char *old_name,
                                                               const char *requested_name,
                                                               char *display_name,
                                                               size_t display_size,
                                                               char *index_name, size_t index_size)
{
  struct rename_context ctx;

  memset(&ctx, 0, sizeof(ctx));
  ctx.status = PLAYER_RENAME_OK;
  if (old_name)
    strlcpy(ctx.old_display_name, old_name, sizeof(ctx.old_display_name));
  if (!rename_validate_name(&ctx, requested_name))
    return ctx.status;
  if (display_name)
    strlcpy(display_name, ctx.new_display_name, display_size);
  if (index_name)
    strlcpy(index_name, ctx.new_index_name, index_size);
  return PLAYER_RENAME_OK;
}

enum player_rename_status player_rename_memory_preflight_for_test(struct char_data *victim,
                                                                  const char *old_name,
                                                                  const char *new_name)
{
  struct rename_context ctx;

  memset(&ctx, 0, sizeof(ctx));
  ctx.victim = victim;
  ctx.player_index_position = -1;
  ctx.status = PLAYER_RENAME_OK;
  strlcpy(ctx.old_display_name, old_name, sizeof(ctx.old_display_name));
  strlcpy(ctx.new_display_name, new_name, sizeof(ctx.new_display_name));
  if (!rename_find_player_index(&ctx) || !rename_check_memory_collisions(&ctx))
    return ctx.status;
  return PLAYER_RENAME_OK;
}

int player_rename_rewrite_file_for_test(const char *path, const char *old_name,
                                        const char *new_name, const char *account_name,
                                        int rewrite_identity, unsigned int *intro_changes)
{
  return rename_rewrite_player_file(path, old_name, new_name, account_name, rewrite_identity,
                                    intro_changes);
}

int player_rename_file_identity_matches_for_test(const char *path, const char *name, long player_id)
{
  return rename_source_player_identity_matches(path, name, player_id);
}

void player_rename_replace_generated_name_for_test(char **field, const char *old_name,
                                                   const char *new_name)
{
  rename_replace_generated_name(field, old_name, new_name);
}
#endif

static void rename_refresh_live_state(struct rename_context *ctx)
{
  if (ctx->account_name)
  {
    if (GET_ACCOUNT_NAME(ctx->victim))
      free(GET_ACCOUNT_NAME(ctx->victim));
    GET_ACCOUNT_NAME(ctx->victim) = ctx->account_name;
    ctx->account_name = NULL;
  }

  rename_apply_account_refreshes(ctx);
  rename_apply_live_strings(ctx);

  pubsub_invalidate_player_cache(ctx->old_display_name);
  pubsub_invalidate_player_cache(ctx->new_display_name);

  if (ctx->victim->desc && ctx->victim->desc->pProtocol)
  {
    MSDPSetString(ctx->victim->desc, eMSDP_CHARACTER_NAME, ctx->new_display_name);
    MSDPFlush(ctx->victim->desc, eMSDP_CHARACTER_NAME);
  }
}

static void rename_log_audit(struct rename_context *ctx)
{
  char audit[MAX_STRING_LENGTH];
  char account_id[32];
  size_t used;
  int i;

  if (ctx->report->account_linked)
    snprintf(account_id, sizeof(account_id), "%d", ctx->report->account_id);
  else
    strlcpy(account_id, "NULL", sizeof(account_id));

  used =
      snprintf(audit, sizeof(audit),
               "CHAR_RENAME actor=%s actor_id=%ld old=%s new=%s player_id=%ld "
               "account_id=%s files=%u intro_files=%u status=%s rollback=%s rows={",
               ctx->actor && GET_NAME(ctx->actor) ? GET_NAME(ctx->actor) : "(null)",
               ctx->actor ? GET_IDNUM(ctx->actor) : -1L, ctx->old_display_name,
               ctx->new_display_name, ctx->report->player_id, account_id, ctx->report->files_moved,
               ctx->report->introduction_files_changed, player_rename_status_string(ctx->status),
               ctx->report->rollback_succeeded ? "ok" : "failed");

  for (i = 0; i < (int)(sizeof(ctx->keys) / sizeof(ctx->keys[0])) && used < sizeof(audit); i++)
    if (ctx->keys[i].active)
      used += snprintf(audit + used, sizeof(audit) - used, "%s%s.%s=%llu",
                       used && audit[used - 1] != '{' ? "," : "", ctx->keys[i].table,
                       ctx->keys[i].column, ctx->keys[i].affected_count);
  if (used < sizeof(audit))
    snprintf(audit + used, sizeof(audit) - used, "}");
  mudlog(BRF, LVL_IMMORT, TRUE, "%s", audit);
}

static void rename_free_context(struct rename_context *ctx)
{
  int account_index;
  int character_index;
  int live_string_index;

  free(ctx->escaped_old_name);
  free(ctx->escaped_new_name);
  free(ctx->account_name);
  if (!ctx->memory_changed)
  {
    free(ctx->new_player_index_name);
    free(ctx->new_victim_name);
  }
  for (account_index = 0; account_index < ctx->account_refresh_count; account_index++)
    for (character_index = 0; character_index < MAX_CHARS_PER_ACCOUNT; character_index++)
      free(ctx->account_refreshes[account_index].character_names[character_index]);
  free(ctx->account_refreshes);
  for (live_string_index = 0; live_string_index < ctx->live_string_count; live_string_index++)
    free(ctx->live_strings[live_string_index].replacement);
  free(ctx->live_strings);
  free(ctx->intro_files);
}

enum player_rename_status rename_player_everywhere(struct char_data *actor,
                                                   struct char_data *victim,
                                                   const char *requested_name,
                                                   struct player_rename_report *report)
{
  struct rename_context ctx;
  struct player_rename_report local_report;
  int success = FALSE;

  if (!report)
    report = &local_report;
  memset(&ctx, 0, sizeof(ctx));
  memset(report, 0, sizeof(*report));
  memcpy(ctx.keys, rename_key_template, sizeof(rename_key_template));
  ctx.actor = actor;
  ctx.victim = victim;
  ctx.report = report;
  ctx.player_index_position = -1;
  ctx.status = PLAYER_RENAME_PLAYER_NOT_FOUND;
  report->status = PLAYER_RENAME_PLAYER_NOT_FOUND;
  report->account_id = -1;
  report->rollback_succeeded = TRUE;

  if (!actor || !victim || IS_NPC(victim) || !victim->player_specials || !GET_NAME(victim))
  {
    rename_set_failure(&ctx, PLAYER_RENAME_PLAYER_NOT_FOUND, "validating rename target");
    goto finished;
  }

  strlcpy(ctx.old_display_name, GET_NAME(victim), sizeof(ctx.old_display_name));
  strlcpy(report->old_name, ctx.old_display_name, sizeof(report->old_name));
  report->player_id = GET_IDNUM(victim);

  if (!rename_validate_name(&ctx, requested_name))
    goto finished;
  strlcpy(report->new_name, ctx.new_display_name, sizeof(report->new_name));

  if (!rename_find_player_index(&ctx) || !rename_check_memory_collisions(&ctx) ||
      !rename_plan_files(&ctx))
    goto finished;

  if (!mysql_available || !conn)
  {
    rename_set_failure(&ctx, PLAYER_RENAME_DATABASE_UNAVAILABLE, "checking database connection");
    goto finished;
  }

  MYSQL_LOCK(mysql_mutex);
  ctx.mysql_locked = TRUE;

  if (!MYSQL_PING_CONN(conn))
  {
    rename_set_failure(&ctx, PLAYER_RENAME_DATABASE_UNAVAILABLE, "checking database connection");
    goto finished;
  }

  ctx.escaped_old_name = rename_escape_name(ctx.old_display_name);
  ctx.escaped_new_name = rename_escape_name(ctx.new_display_name);
  if (!ctx.escaped_old_name || !ctx.escaped_new_name)
  {
    rename_set_failure(&ctx, PLAYER_RENAME_DATABASE_ERROR, "escaping character names");
    goto finished;
  }

  if (!rename_activate_db_keys(&ctx) || !rename_require_innodb_table(&ctx, "account_data") ||
      !rename_activate_level_30_view(&ctx) || !rename_configure_player_data_shape(&ctx))
    goto finished;

  if (!rename_db_query(&ctx, "START TRANSACTION", "starting database transaction"))
    goto finished;
  ctx.transaction_started = TRUE;

  if (!rename_lock_canonical_player(&ctx) || !rename_preflight_db_rows(&ctx) ||
      !rename_revalidate_db_keys(&ctx) || !rename_require_innodb_table(&ctx, "account_data") ||
      !rename_create_backups(&ctx) || !rename_move_files(&ctx))
    goto finished;

  ctx.database_updates_attempted = TRUE;
  if (!rename_update_db_rows(&ctx) || !rename_change_memory(&ctx) || !rename_rewrite_files(&ctx))
    goto finished;

  if (rename_inject_failure(&ctx, PLAYER_RENAME_TEST_FAIL_INDEX_WRITE, PLAYER_RENAME_SAVE_ERROR,
                            "saving player index") ||
      !save_player_index_checked())
  {
    rename_set_failure(&ctx, PLAYER_RENAME_SAVE_ERROR, "saving player index");
    goto finished;
  }
  ctx.index_written = TRUE;

  if (rename_inject_failure(&ctx, PLAYER_RENAME_TEST_FAIL_POSTCONDITION,
                            PLAYER_RENAME_POSTCONDITION_FAILED,
                            "verifying rename postconditions") ||
      !rename_verify_db_rows(&ctx) ||
      !rename_verify_canonical_player(&ctx, ctx.escaped_new_name, ctx.new_display_name,
                                      "verifying canonical player identity") ||
      !rename_verify_level_30_view(&ctx))
    goto finished;
  if (!rename_verify_files(&ctx))
  {
    rename_set_failure(&ctx, PLAYER_RENAME_POSTCONDITION_FAILED, "verifying file postconditions");
    goto finished;
  }
  if (!rename_prepare_account_refreshes(&ctx) || !rename_prepare_live_strings(&ctx))
    goto finished;

  ctx.commit_attempted = TRUE;
  if (rename_inject_failure(&ctx, PLAYER_RENAME_TEST_FAIL_COMMIT, PLAYER_RENAME_DATABASE_ERROR,
                            "committing database transaction") ||
      !rename_db_query(&ctx, "COMMIT", "committing database transaction"))
    goto finished;
  ctx.transaction_started = FALSE;
  success = TRUE;

finished:
  if (!success)
  {
    rename_rollback(&ctx);
    if (!report->rollback_succeeded)
      log("SYSERR: HIGH-SEVERITY character rename recovery is not proven for player ID %ld; "
          "rollback snapshots were retained%s",
          report->player_id, ctx.commit_attempted ? " after a commit attempt" : "");
  }

  if (ctx.mysql_locked)
  {
    MYSQL_UNLOCK(mysql_mutex);
    ctx.mysql_locked = FALSE;
  }

  if (success)
  {
    ctx.status = PLAYER_RENAME_OK;
    report->status = PLAYER_RENAME_OK;
    report->rollback_succeeded = TRUE;
    if (ctx.memory_changed)
    {
      free(ctx.old_player_index_name);
      free(ctx.old_victim_name);
      ctx.old_player_index_name = NULL;
      ctx.old_victim_name = NULL;
    }
    rename_refresh_live_state(&ctx);
    rename_remove_backups(&ctx);
  }

  rename_log_audit(&ctx);
  rename_free_context(&ctx);
  return report->status;
}
