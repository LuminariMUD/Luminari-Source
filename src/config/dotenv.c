/**
 * @file dotenv.c
 * @author LuminariMUD Dev Team
 * @brief Simple .env file parser for configuration
 *
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "core/sysdep.h"
#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "dotenv.h"

#include <sys/stat.h>
#include <time.h>

#define MAX_ENV_LINE 512
#define MAX_ENV_KEY 64
#define MAX_ENV_VALUE 256

struct env_entry
{
  char key[MAX_ENV_KEY];
  char value[MAX_ENV_VALUE];
  struct env_entry *next;
};

static struct env_entry *env_values;
static struct stat env_file_status;
static bool env_cache_loaded;

/** @brief Discard parsed values when the selected environment file changes. */
static void clear_env_values(void)
{
  struct env_entry *entry;

  while (env_values)
  {
    entry = env_values;
    env_values = entry->next;
    free(entry);
  }
  env_cache_loaded = false;
}

/** @brief Detect replacement, in-place edits, and metadata changes at nanosecond resolution. */
static bool same_env_file(const struct stat *first, const struct stat *second)
{
  return first->st_dev == second->st_dev && first->st_ino == second->st_ino &&
         first->st_size == second->st_size && first->st_mtim.tv_sec == second->st_mtim.tv_sec &&
         first->st_mtim.tv_nsec == second->st_mtim.tv_nsec &&
         first->st_ctim.tv_sec == second->st_ctim.tv_sec &&
         first->st_ctim.tv_nsec == second->st_ctim.tv_nsec;
}

/** @brief Parse one assignment with the existing whitespace, quoting, and length rules. */
static bool parse_env_assignment(char *line, struct env_entry *entry)
{
  char *line_start = line;
  char *equals_pos, *value_start;
  size_t key_length, value_len;

  while (*line_start && isspace((unsigned char)*line_start))
    line_start++;
  if (!*line_start || *line_start == '#')
    return false;
  equals_pos = strchr(line_start, '=');
  if (!equals_pos)
    return false;
  key_length = equals_pos - line_start;
  if (key_length >= MAX_ENV_KEY)
    return false;
  memcpy(entry->key, line_start, key_length);
  while (key_length > 0 && isspace((unsigned char)entry->key[key_length - 1]))
    key_length--;
  entry->key[key_length] = '\0';
  value_start = equals_pos + 1;
  while (*value_start && isspace((unsigned char)*value_start))
    value_start++;
  strlcpy(entry->value, value_start, sizeof(entry->value));
  value_len = strlen(entry->value);
  while (value_len > 0 && isspace((unsigned char)entry->value[value_len - 1]))
    value_len--;
  entry->value[value_len] = '\0';
  if (value_len >= 2 && entry->value[0] == '"' && entry->value[value_len - 1] == '"')
  {
    memmove(entry->value, entry->value + 1, value_len - 2);
    entry->value[value_len - 2] = '\0';
  }
  return true;
}

/**
 * @brief Share parsed configuration across callers, refreshing on the next lookup after edits.
 * Check current-directory precedence and readability on each lookup. Cache only a complete,
 * unchanged regular-file read; missing or unreadable configuration never serves stale values.
 */
static bool refresh_env_values(void)
{
  static const char *const paths[] = {".env", "lib/.env"};
  struct stat status, after;
  struct env_entry parsed, *entry, **tail;
  FILE *fp = NULL;
  char line[MAX_ENV_LINE];
  size_t i;
  bool complete;

  for (i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
  {
    if (stat(paths[i], &status) != 0 || access(paths[i], R_OK) != 0)
      continue;
    if (!S_ISREG(status.st_mode))
    {
      clear_env_values();
      return false;
    }
    /* Coarse filesystem clocks can stamp rapid writes identically. Read recent
     * files again until their ctime is safely older than the current clock tick. */
    if (env_cache_loaded && same_env_file(&status, &env_file_status) &&
        difftime(time(NULL), status.st_ctime) >= 2.0)
      return true;
    fp = fopen(paths[i], "r");
    if (fp)
      break;
  }
  clear_env_values();
  if (!fp)
    return false;
  if (fstat(fileno(fp), &status) != 0 || !S_ISREG(status.st_mode))
  {
    fclose(fp);
    return false;
  }
  tail = &env_values;
  while (fgets(line, sizeof(line), fp))
  {
    if (!parse_env_assignment(line, &parsed))
      continue;
    entry = calloc(1, sizeof(*entry));
    if (!entry)
    {
      fclose(fp);
      clear_env_values();
      return false;
    }
    strlcpy(entry->key, parsed.key, sizeof(entry->key));
    strlcpy(entry->value, parsed.value, sizeof(entry->value));
    *tail = entry;
    tail = &entry->next;
  }
  complete = !ferror(fp) && fstat(fileno(fp), &after) == 0 && same_env_file(&status, &after);
  fclose(fp);
  if (!complete)
  {
    clear_env_values();
    return false;
  }
  env_file_status = status;
  env_cache_loaded = true;
  return true;
}

/**
 * @brief Return the first matching assignment, or an empty string when absent.
 * The process-wide cache observes file changes on every lookup. The returned static buffer
 * is overwritten by the next call and must not be freed.
 */
char *get_env_value(const char *key)
{
  static char value[MAX_ENV_VALUE];
  struct env_entry *entry;

  value[0] = '\0';
  if (!key || !refresh_env_values())
    return value;
  for (entry = env_values; entry; entry = entry->next)
  {
    if (strcmp(entry->key, key) == 0)
    {
      strlcpy(value, entry->value, sizeof(value));
      break;
    }
  }
  return value;
}

/**
 * Get environment variable as integer
 */
int get_env_int(const char *key, int default_value)
{
  char *value = get_env_value(key);
  if (!value || !*value)
  {
    return default_value;
  }
  return atoi(value);
}

/**
 * Get environment variable as boolean
 */
bool get_env_bool(const char *key, bool default_value)
{
  char *value = get_env_value(key);
  if (!value || !*value)
  {
    return default_value;
  }

  /* Check for various true values */
  if (!strcasecmp(value, "true") || !strcasecmp(value, "yes") || !strcasecmp(value, "1") ||
      !strcasecmp(value, "on"))
  {
    return TRUE;
  }

  return FALSE;
}
