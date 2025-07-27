/**
 * @file dotenv.c
 * @author LuminariMUD Dev Team
 * @brief Simple .env file parser for configuration
 * 
 * Part of the LuminariMUD distribution.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"

#define MAX_ENV_LINE 512
#define MAX_ENV_KEY 64
#define MAX_ENV_VALUE 256

/**
 * Get environment variable from .env file
 * Returns static buffer - do not free!
 */
char *get_env_value(const char *key) {
  static char value[MAX_ENV_VALUE];
  FILE *fp;
  char line[MAX_ENV_LINE];
  char file_key[MAX_ENV_KEY];
  char *equals_pos;
  char *line_start;
  char *value_start;
  size_t key_len;
  
  value[0] = '\0';
  
  /* Try to open .env file in lib directory */
  fp = fopen("lib/.env", "r");
  if (!fp) {
    /* Try current directory as fallback */
    fp = fopen(".env", "r");
    if (!fp) {
      return value;
    }
  }
  
  key_len = strlen(key);
  
  /* Read each line */
  while (fgets(line, sizeof(line), fp)) {
    line_start = line;
    
    /* Skip whitespace */
    while (*line_start && isspace(*line_start)) {
      line_start++;
    }
    
    /* Skip comments and empty lines */
    if (!*line_start || *line_start == '#') {
      continue;
    }
    
    /* Find equals sign */
    equals_pos = strchr(line_start, '=');
    if (!equals_pos) {
      continue;
    }
    
    /* Extract key */
    strncpy(file_key, line_start, equals_pos - line_start);
    file_key[equals_pos - line_start] = '\0';
    
    /* Remove trailing whitespace from key */
    while (strlen(file_key) > 0 && isspace(file_key[strlen(file_key) - 1])) {
      file_key[strlen(file_key) - 1] = '\0';
    }
    
    /* Check if this is our key */
    if (strcmp(file_key, key) == 0) {
      /* Extract value */
      value_start = equals_pos + 1;
      
      /* Skip leading whitespace */
      while (*value_start && isspace(*value_start)) {
        value_start++;
      }
      
      /* Copy value */
      strlcpy(value, value_start, sizeof(value));
      
      /* Remove trailing whitespace and newline */
      while (strlen(value) > 0 && 
             (isspace(value[strlen(value) - 1]) || 
              value[strlen(value) - 1] == '\n' || 
              value[strlen(value) - 1] == '\r')) {
        value[strlen(value) - 1] = '\0';
      }
      
      /* Remove quotes if present */
      if (value[0] == '"' && value[strlen(value) - 1] == '"') {
        memmove(value, value + 1, strlen(value) - 2);
        value[strlen(value) - 2] = '\0';
      }
      
      fclose(fp);
      return value;
    }
  }
  
  fclose(fp);
  return value;
}

/**
 * Get environment variable as integer
 */
int get_env_int(const char *key, int default_value) {
  char *value = get_env_value(key);
  if (!value || !*value) {
    return default_value;
  }
  return atoi(value);
}

/**
 * Get environment variable as boolean
 */
bool get_env_bool(const char *key, bool default_value) {
  char *value = get_env_value(key);
  if (!value || !*value) {
    return default_value;
  }
  
  /* Check for various true values */
  if (!strcasecmp(value, "true") || 
      !strcasecmp(value, "yes") || 
      !strcasecmp(value, "1") || 
      !strcasecmp(value, "on")) {
    return TRUE;
  }
  
  return FALSE;
}