/* Simple check for score help entry */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mariadb/mysql.h>

int main(void) {
  MYSQL *conn;
  MYSQL_RES *res;
  MYSQL_ROW row;
  FILE *config_file;
  char host[256] = {0};
  char username[256] = {0};
  char password[256] = {0};
  char database[256] = {0};
  char line[512];
  
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
      /* Skip spaces after = */
      while (*value == ' ') value++;
      char *newline = strchr(value, '\n');
      if (newline) *newline = '\0';
      /* Trim trailing spaces from key */
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
  
  printf("Connected to database.\n\n");
  
  /* Check for score entries with various queries */
  printf("=== Query 1: Direct keyword search ===\n");
  if (!mysql_query(conn, "SELECT keyword, help_tag FROM help_keywords WHERE keyword LIKE 'score%'")) {
    res = mysql_store_result(conn);
    if (res) {
      printf("Results:\n");
      while ((row = mysql_fetch_row(res))) {
        printf("  keyword='%s', help_tag='%s'\n", row[0], row[1]);
      }
      printf("Total rows: %lu\n\n", (unsigned long)mysql_num_rows(res));
      mysql_free_result(res);
    }
  }
  
  printf("=== Query 2: Case-insensitive search (LOWER both sides) ===\n");
  if (!mysql_query(conn, "SELECT keyword, help_tag FROM help_keywords WHERE LOWER(keyword) LIKE LOWER('score%')")) {
    res = mysql_store_result(conn);
    if (res) {
      printf("Results:\n");
      while ((row = mysql_fetch_row(res))) {
        printf("  keyword='%s', help_tag='%s'\n", row[0], row[1]);
      }
      printf("Total rows: %lu\n\n", (unsigned long)mysql_num_rows(res));
      mysql_free_result(res);
    }
  }
  
  printf("=== Query 3: The exact query from search_help ===\n");
  if (!mysql_query(conn, 
      "SELECT DISTINCT he.tag, LEFT(he.entry, 50), he.min_level "
      "FROM help_entries he "
      "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
      "WHERE LOWER(hk.keyword) LIKE LOWER('score%') AND he.min_level <= 60")) {
    res = mysql_store_result(conn);
    if (res) {
      printf("Results:\n");
      while ((row = mysql_fetch_row(res))) {
        printf("  tag='%s', entry='%s...', min_level=%s\n", row[0], row[1], row[2]);
      }
      printf("Total rows: %lu\n\n", (unsigned long)mysql_num_rows(res));
      mysql_free_result(res);
    }
  }
  
  mysql_close(conn);
  return 0;
}