/* Debug prepared statement issue with exact game query */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



int main(void) {
  MYSQL *conn;
  MYSQL_STMT *stmt;
  MYSQL_BIND bind[2];
  MYSQL_BIND result[5];
  MYSQL_RES *prepare_meta_result;
  FILE *config_file;
  char host[256] = {0};
  char username[256] = {0};
  char password[256] = {0};
  char database[256] = {0};
  char line[512];
  char search_pattern[256];
  int level = 34;
  
  /* Result buffers */
  char tag[201];
  char entry[65536];
  int min_level;
  MYSQL_TIME last_updated;
  char keywords[65536];
  
  unsigned long tag_length;
  unsigned long entry_length;
  unsigned long keywords_length;
  
  my_bool tag_is_null;
  my_bool entry_is_null;
  my_bool min_level_is_null;
  my_bool last_updated_is_null;
  my_bool keywords_is_null;
  
  my_bool tag_error;
  my_bool entry_error;
  my_bool min_level_error;
  my_bool last_updated_error;
  my_bool keywords_error;
  
  /* Read MySQL configuration */
  config_file = fopen("lib/mysql_config", "r");
  if (!config_file) {
    printf("ERROR: Cannot open lib/mysql_config\n");
    return 1;
  }
  
  while (fgets(line, sizeof(line), config_file)) {
    char *value;
    if (line[0] == '#' || line[0] == '\n') continue;
    
    if ((value = strchr(line, '='))) {
      *value++ = '\0';
      while (*value == ' ') value++;
      char *newline = strchr(value, '\n');
      if (newline) *newline = '\0';
      char *end = line + strlen(line) - 1;
      while (end > line && *end == ' ') *end-- = '\0';
      
      if (strcmp(line, "mysql_host") == 0) {
        strncpy(host, value, sizeof(host) - 1);
      } else if (strcmp(line, "mysql_username") == 0) {
        strncpy(username, value, sizeof(username) - 1);
      } else if (strcmp(line, "mysql_password") == 0) {
        strncpy(password, value, sizeof(password) - 1);
      } else if (strcmp(line, "mysql_database") == 0) {
        strncpy(database, value, sizeof(database) - 1);
      }
    }
  }
  fclose(config_file);
  
  /* Connect to database */
  conn = mysql_init(NULL);
  if (!mysql_real_connect(conn, host, username, password, database, 0, NULL, 0)) {
    printf("ERROR: Failed to connect: %s\n", mysql_error(conn));
    return 1;
  }
  
  printf("Connected to database.\n");
  
  /* Create prepared statement */
  stmt = mysql_stmt_init(conn);
  if (!stmt) {
    printf("ERROR: mysql_stmt_init failed\n");
    mysql_close(conn);
    return 1;
  }
  
  /* The EXACT query from search_help */
  const char *query = 
    "SELECT DISTINCT he.tag, he.entry, he.min_level, he.last_updated, "
    "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ') "
    "FROM help_entries he "
    "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
    "LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag "
    "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ? "
    "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
    "ORDER BY LENGTH(hk.keyword) ASC";
  
  printf("\nPreparing query...\n");
  if (mysql_stmt_prepare(stmt, query, strlen(query))) {
    printf("ERROR: mysql_stmt_prepare failed: %s\n", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Query prepared successfully.\n");
  printf("Parameter count: %lu\n", mysql_stmt_param_count(stmt));
  
  /* Set up parameter bindings */
  memset(bind, 0, sizeof(bind));
  
  /* First parameter: search pattern */
  snprintf(search_pattern, sizeof(search_pattern), "score%%");
  bind[0].buffer_type = MYSQL_TYPE_STRING;
  bind[0].buffer = search_pattern;
  bind[0].buffer_length = strlen(search_pattern);
  bind[0].is_null = 0;
  bind[0].length = 0;
  
  /* Second parameter: level */
  bind[1].buffer_type = MYSQL_TYPE_LONG;
  bind[1].buffer = &level;
  bind[1].is_null = 0;
  bind[1].length = 0;
  
  printf("\nBinding parameters:\n");
  printf("  Pattern: '%s'\n", search_pattern);
  printf("  Level: %d\n", level);
  
  if (mysql_stmt_bind_param(stmt, bind)) {
    printf("ERROR: mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Parameters bound successfully.\n");
  
  /* Execute the statement */
  printf("\nExecuting statement...\n");
  if (mysql_stmt_execute(stmt)) {
    printf("ERROR: mysql_stmt_execute failed: %s\n", mysql_stmt_error(stmt));
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Statement executed successfully.\n");
  
  /* Get metadata */
  prepare_meta_result = mysql_stmt_result_metadata(stmt);
  if (!prepare_meta_result) {
    printf("ERROR: No result metadata\n");
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Result columns: %d\n", mysql_num_fields(prepare_meta_result));
  
  /* Set up result bindings */
  memset(result, 0, sizeof(result));
  
  /* Column 0: tag */
  result[0].buffer_type = MYSQL_TYPE_STRING;
  result[0].buffer = tag;
  result[0].buffer_length = sizeof(tag);
  result[0].is_null = &tag_is_null;
  result[0].length = &tag_length;
  result[0].error = &tag_error;
  
  /* Column 1: entry */
  result[1].buffer_type = MYSQL_TYPE_STRING;
  result[1].buffer = entry;
  result[1].buffer_length = sizeof(entry);
  result[1].is_null = &entry_is_null;
  result[1].length = &entry_length;
  result[1].error = &entry_error;
  
  /* Column 2: min_level */
  result[2].buffer_type = MYSQL_TYPE_LONG;
  result[2].buffer = &min_level;
  result[2].is_null = &min_level_is_null;
  result[2].length = 0;
  result[2].error = &min_level_error;
  
  /* Column 3: last_updated */
  result[3].buffer_type = MYSQL_TYPE_DATETIME;
  result[3].buffer = &last_updated;
  result[3].is_null = &last_updated_is_null;
  result[3].length = 0;
  result[3].error = &last_updated_error;
  
  /* Column 4: keywords (GROUP_CONCAT) */
  result[4].buffer_type = MYSQL_TYPE_STRING;
  result[4].buffer = keywords;
  result[4].buffer_length = sizeof(keywords);
  result[4].is_null = &keywords_is_null;
  result[4].length = &keywords_length;
  result[4].error = &keywords_error;
  
  printf("\nBinding result buffers...\n");
  if (mysql_stmt_bind_result(stmt, result)) {
    printf("ERROR: mysql_stmt_bind_result failed: %s\n", mysql_stmt_error(stmt));
    mysql_free_result(prepare_meta_result);
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Result buffers bound.\n");
  
  /* Store result */
  printf("\nStoring result...\n");
  if (mysql_stmt_store_result(stmt)) {
    printf("ERROR: mysql_stmt_store_result failed: %s\n", mysql_stmt_error(stmt));
    mysql_free_result(prepare_meta_result);
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 1;
  }
  
  printf("Result stored.\n");
  printf("Number of rows: %llu\n", mysql_stmt_num_rows(stmt));
  
  /* Fetch results */
  printf("\n=== RESULTS ===\n");
  int row_count = 0;
  int fetch_result;
  
  while ((fetch_result = mysql_stmt_fetch(stmt)) == 0 || fetch_result == MYSQL_DATA_TRUNCATED) {
    row_count++;
    printf("\nRow %d:\n", row_count);
    
    if (fetch_result == MYSQL_DATA_TRUNCATED) {
      printf("  [Note: Some data was truncated]\n");
      if (tag_error) printf("    - tag truncated\n");
      if (entry_error) printf("    - entry truncated\n");
      if (keywords_error) printf("    - keywords truncated\n");
    }
    
    printf("  Tag: %s\n", tag_is_null ? "NULL" : tag);
    printf("  Entry (first 100 chars): %.100s%s\n", 
           entry_is_null ? "NULL" : entry,
           (entry_length > 100) ? "..." : "");
    printf("  Min Level: %d\n", min_level_is_null ? -1 : min_level);
    printf("  Keywords: %s\n", keywords_is_null ? "NULL" : keywords);
  }
  
  if (row_count == 0) {
    printf("No rows returned!\n");
  }
  
  /* Clean up */
  mysql_free_result(prepare_meta_result);
  mysql_stmt_close(stmt);
  mysql_close(conn);
  
  printf("\nTest complete.\n");
  return 0;
}