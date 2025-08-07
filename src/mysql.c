/*
  LuminariMUD
  MySQL Functionality for the MUD

  Author:  Ornir

*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "modify.h"
#include "mysql.h"

#include "wilderness.h"
#include "mud_event.h"

MYSQL *conn = NULL;
MYSQL *conn2 = NULL;
MYSQL *conn3 = NULL;

/* MySQL connection mutexes for thread safety */
pthread_mutex_t mysql_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_mutex3 = PTHREAD_MUTEX_INITIALIZER;

void after_world_load()
{
}

/**
 * Establishes connection to MySQL database using configuration from mysql_config file
 * 
 * This function reads database connection parameters from the 'mysql_config' file
 * located in the lib/ directory. The configuration file should contain:
 *   - mysql_host = <hostname or IP>
 *   - mysql_database = <database name>
 *   - mysql_username = <MySQL username>
 *   - mysql_password = <MySQL password>
 * 
 * The function creates three MySQL connections (conn, conn2, conn3) to handle
 * nested queries and concurrent database operations.
 * 
 * Configuration file format:
 *   - Lines starting with '#' are treated as comments
 *   - Empty lines are ignored
 *   - Parameters use the format: key = value
 * 
 * IMPORTANT: The mysql_config file MUST be located in the lib/ directory
 *            relative to where the MUD is executed from.
 * 
 * @note Exits the program if connection fails or config file is missing
 */
/* Global flag to track MySQL availability */
bool mysql_available = FALSE;

void connect_to_mysql()
{
  char host[128], database[128], username[128], password[128];
  char line[128], key[128], val[128];
  FILE *file;

  /* Initialize to indicate MySQL not available */
  mysql_available = FALSE;

  /* Read the mysql configuration file from lib/ directory */
  if (!(file = fopen("mysql_config", "r")))
  {
    log("WARNING: Unable to read MySQL configuration from 'mysql_config'.");
    log("WARNING: Running without MySQL support - some features will be disabled.");
    log("WARNING: To enable MySQL: Copy mysql_config_example to lib/mysql_config and edit it.");
    return;
  }

  /* Parse configuration file line by line
   * Format: key = value
   * Comments start with '#'
   * Empty lines are ignored
   */
  while (!feof(file) && fgets(line, 127, file))
  {
    /* Skip comments (lines starting with #) and empty lines */
    if (*line == '#' || strlen(line) <= 1)
      continue;
    
    /* Parse key = value pairs */
    else if (sscanf(line, "%s = %s", key, val) == 2)
    {
      /* Host configuration (e.g., localhost, 192.168.1.100, db.example.com) */
      if (!str_cmp(key, "mysql_host"))
        strlcpy(host, val, sizeof(host));
      
      /* Database name configuration */
      else if (!str_cmp(key, "mysql_database"))
        strlcpy(database, val, sizeof(database));
      
      /* Username configuration */
      else if (!str_cmp(key, "mysql_username"))
        strlcpy(username, val, sizeof(username));
      
      /* Password configuration */
      else if (!str_cmp(key, "mysql_password"))
        strlcpy(password, val, sizeof(password));
      
      /* Unknown configuration parameter */
      else
      {
        log("SYSERR: Unknown parameter '%s' in MySQL configuration", key);
        log("SYSERR: Valid parameters: mysql_host, mysql_database, mysql_username, mysql_password");
      }
    }
    else
    {
      log("WARNING: Malformed line in MySQL configuration: %s", line);
      log("WARNING: Expected format: parameter = value");
      fclose(file);
      return;
    }
  }

  if (fclose(file))
  {
    log("WARNING: Unable to read MySQL configuration.");
    return;
  }

  if (mysql_library_init(0, NULL, NULL))
  {
    log("WARNING: Unable to initialize MySQL library.");
    return;
  }

  if (!(conn = mysql_init(NULL)))
  {
    log("WARNING: Unable to initialize MySQL connection.");
    return;
  }

  my_bool reconnect = 1;
  mysql_options(conn, MYSQL_OPT_RECONNECT, (const char *)&reconnect);

  if (!mysql_real_connect(conn, host, username, password, database, 0, NULL, 0))
  {
    log("WARNING: Unable to connect to MySQL: %s", mysql_error(conn));
    mysql_close(conn);
    conn = NULL;
    return;
  }

  // 2nd conn for queries within other query loops
  if (!(conn2 = mysql_init(NULL)))
  {
    log("WARNING: Unable to initialize MySQL connection 2.");
    mysql_close(conn);
    conn = NULL;
    return;
  }

  reconnect = 1;
  mysql_options(conn2, MYSQL_OPT_RECONNECT, (const char *)&reconnect);

  if (!mysql_real_connect(conn2, host, username, password, database, 0, NULL, 0))
  {
    log("WARNING: Unable to connect to MySQL2: %s", mysql_error(conn2));
    mysql_close(conn);
    mysql_close(conn2);
    conn = NULL;
    conn2 = NULL;
    return;
  }

  // 3rd conn for queries within other query loops
  if (!(conn3 = mysql_init(NULL)))
  {
    log("WARNING: Unable to initialize MySQL connection 3.");
    mysql_close(conn);
    mysql_close(conn2);
    conn = NULL;
    conn2 = NULL;
    return;
  }

  reconnect = 1;
  mysql_options(conn3, MYSQL_OPT_RECONNECT, (const char *)&reconnect);

  if (!mysql_real_connect(conn3, host, username, password, database, 0, NULL, 0))
  {
    log("WARNING: Unable to connect to MySQL3: %s", mysql_error(conn3));
    mysql_close(conn);
    mysql_close(conn2);
    mysql_close(conn3);
    conn = NULL;
    conn2 = NULL;
    conn3 = NULL;
    return;
  }
  
  /* If we got here, MySQL is successfully initialized */
  mysql_available = TRUE;
  
  /* Log successful connection - password is intentionally not logged for security */
  log("SUCCESS: Connected to MySQL database '%s' on host '%s' as user '%s'", database, host, username);
  log("INFO: MySQL configuration loaded from lib/mysql_config");
}

void disconnect_from_mysql()
{
  if (conn) {
    mysql_close(conn);
    conn = NULL;
  }
}

void disconnect_from_mysql2()
{
  if (conn2) {
    mysql_close(conn2);
    conn2 = NULL;
  }
}

void disconnect_from_mysql3()
{
  if (conn3) {
    mysql_close(conn3);
    conn3 = NULL;
  }
}

/* Call this once at program termination */
void cleanup_mysql_library()
{
  disconnect_from_mysql();
  disconnect_from_mysql2();
  disconnect_from_mysql3();
  mysql_library_end();
}

/**
 * Thread-safe wrapper for mysql_query()
 * Automatically selects the correct mutex based on connection
 * 
 * @param mysql_conn The MySQL connection to use (conn, conn2, or conn3)
 * @param query The SQL query to execute
 * @return 0 on success, non-zero on error (same as mysql_query)
 */
int mysql_query_safe(MYSQL *mysql_conn, const char *query)
{
  pthread_mutex_t *mutex;
  int result;
  
  /* Check if MySQL is available */
  if (!mysql_available || !mysql_conn) {
    return -1;
  }
  
  /* Select appropriate mutex based on connection */
  if (mysql_conn == conn)
    mutex = &mysql_mutex;
  else if (mysql_conn == conn2)
    mutex = &mysql_mutex2;
  else if (mysql_conn == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_query_safe called with unknown connection");
    return -1;
  }
  
  MYSQL_LOCK(*mutex);
  result = mysql_query(mysql_conn, query);
  MYSQL_UNLOCK(*mutex);
  
  return result;
}

/**
 * Thread-safe wrapper for mysql_store_result()
 * Must be called AFTER mysql_query_safe() while still holding the lock
 * 
 * @param mysql_conn The MySQL connection to use (conn, conn2, or conn3)
 * @return MYSQL_RES pointer on success, NULL on error
 */
MYSQL_RES *mysql_store_result_safe(MYSQL *mysql_conn)
{
  pthread_mutex_t *mutex;
  MYSQL_RES *result;
  
  /* Select appropriate mutex based on connection */
  if (mysql_conn == conn)
    mutex = &mysql_mutex;
  else if (mysql_conn == conn2)
    mutex = &mysql_mutex2;
  else if (mysql_conn == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_store_result_safe called with unknown connection");
    return NULL;
  }
  
  MYSQL_LOCK(*mutex);
  result = mysql_store_result(mysql_conn);
  MYSQL_UNLOCK(*mutex);
  
  return result;
}

/**
 * Escape a string for safe use in MySQL queries
 * Allocates memory that must be freed by caller
 * 
 * @param mysql_conn The MySQL connection to use
 * @param str The string to escape
 * @return Newly allocated escaped string (must be freed), or NULL on error
 */
char *mysql_escape_string_alloc(MYSQL *mysql_conn, const char *str)
{
  char *escaped;
  size_t len;
  pthread_mutex_t *mutex;
  
  if (!str || !mysql_conn)
    return NULL;
    
  /* Allocate worst-case space: each char could be escaped to 2 chars, plus null */
  len = strlen(str);
  CREATE(escaped, char, (len * 2) + 1);
  
  /* Select appropriate mutex based on connection */
  if (mysql_conn == conn)
    mutex = &mysql_mutex;
  else if (mysql_conn == conn2)
    mutex = &mysql_mutex2;
  else if (mysql_conn == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_escape_string_alloc called with unknown connection");
    free(escaped);
    return NULL;
  }
  
  /* Lock and escape */
  MYSQL_LOCK(*mutex);
  mysql_real_escape_string(mysql_conn, escaped, str, len);
  MYSQL_UNLOCK(*mutex);
  
  return escaped;
}

/* Load the wilderness data for the specified zone. */
struct wilderness_data *load_wilderness(zone_vnum zone)
{

  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[1024];

  struct wilderness_data *wild = NULL;

  log("INFO: Loading wilderness data for zone: %d", zone);

  snprintf(buf, sizeof(buf), "SELECT f.id, f.nav_vnum, f.dynamic_vnum_pool_start, f.dynamic_vnum_pool_end, f.x_size, f.y_size, f.elevation_seed, f.distortion_seed, f.moisture_seed, f.min_temp, f.max_temp from wilderness_data as f where f.zone_vnum = %d", zone);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from wilderness_data: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from wilderness_data: %s", mysql_error(conn));
    exit(1);
  }

  if (mysql_num_rows(result) > 1)
  {
    log("SYSERR: Too many rows returned on SELECT from wilderness_data for zone: %d", zone);
  }

  CREATE(wild, struct wilderness_data, 1);

  /* Just use the first row. */
  row = mysql_fetch_row(result);

  if (row)
  {
    wild->id = atoi(row[0]);
    wild->zone = real_zone(zone);
    wild->nav_vnum = atoi(row[1]);
    wild->dynamic_vnum_pool_start = atoi(row[2]);
    wild->dynamic_vnum_pool_end = atoi(row[3]);
    wild->x_size = atoi(row[4]);
    wild->y_size = atoi(row[5]);
    wild->elevation_seed = atoi(row[6]);
    wild->distortion_seed = atoi(row[7]);
    wild->moisture_seed = atoi(row[8]);
    wild->min_temp = atoi(row[9]);
    wild->max_temp = atoi(row[10]);
  }

  mysql_free_result(result);
  return wild;
}

/* String tokenizer. */
char **tokenize(const char *input, const char *delim)
{
  char *str;
  char **result;
  char *tok;
  int count = 0;
  int capacity = 10;
  const char *trimmed_input;
  
  /* Sanity check inputs */
  if (!input || !delim) {
    log("SYSERR: tokenize() called with NULL input or delim");
    return NULL;
  }
  
  /* Skip leading delimiters to avoid empty first token */
  trimmed_input = input;
  while (*trimmed_input && strchr(delim, *trimmed_input)) {
    trimmed_input++;
  }
  
  /* Check if entire string was delimiters */
  if (!*trimmed_input) {
    /* Return array with single NULL element */
    result = malloc(sizeof(char*));
    if (!result) {
      log("SYSERR: tokenize() failed to allocate memory for empty result");
      return NULL;
    }
    result[0] = NULL;
    return result;
  }
  
  str = strdup(trimmed_input);
  if (!str) {
    log("SYSERR: tokenize() failed to allocate memory for input string");
    return NULL;
  }
  
  /* Allocate space including room for NULL terminator */
  result = malloc((capacity + 1) * sizeof(*result));
  if (!result) {
    log("SYSERR: tokenize() failed to allocate memory for result array");
    free(str);
    return NULL;
  }

  /* Use strtok - strtok_r may not be available in all C90 environments */
  tok = strtok(str, delim);

  while (tok)
  {
    char *dup;
    
    if (count >= capacity) {
      char **new_result;
      int i;
      
      capacity *= 2;
      new_result = realloc(result, (capacity + 1) * sizeof(*result));
      if (!new_result) {
        log("SYSERR: tokenize() failed to realloc result array to size %d", capacity);
        /* Clean up and bail out */
        for (i = 0; i < count; i++)
          free(result[i]);
        free(result);
        free(str);
        return NULL;
      }
      result = new_result;
    }

    dup = strdup(tok);
    if (!dup) {
      int i;
      log("SYSERR: tokenize() failed to duplicate token: %s", tok);
      /* Clean up everything */
      for (i = 0; i < count; i++)
        free(result[i]);
      free(result);
      free(str);
      return NULL;
    }
    result[count++] = dup;
    tok = strtok(NULL, delim);
  }
  
  /* NULL-terminate the array */
  result[count] = NULL;

  free(str);
  return result;
}

/* Test function for tokenize - remove after debugging */
void test_tokenize(void)
{
  char **tokens;
  char **it;
  const char *test_input = "#3183\nLoc : -1\nFlag: 64 0 0 0\nName: a small leather pouch";
  char test_str[256];
  char *tok;
  
  log("DEBUG: Testing tokenize with input: '%s'", test_input);
  
  /* First test strtok directly */
  log("DEBUG: Testing strtok directly:");
  strcpy(test_str, test_input);
  tok = strtok(test_str, "\n");
  log("DEBUG: Direct strtok first token: '%s'", tok ? tok : "NULL");
  
  /* Now test our tokenize function */
  tokens = tokenize(test_input, "\n");
  if (!tokens) {
    log("DEBUG: tokenize returned NULL!");
    return;
  }
  
  log("DEBUG: Tokenize results:");
  for (it = tokens; *it; ++it) {
    log("DEBUG:   Token: '%s'", *it);
  }
  
  free_tokens(tokens);
}

/* Free the memory allocated by tokenize() */
void free_tokens(char **tokens)
{
  char **it;
  
  if (!tokens)
    return;
    
  /* Free each individual token string */
  for (it = tokens; *it; ++it)
  {
    free(*it);
  }
  
  /* Free the array itself */
  free(tokens);
}

void load_regions()
{
  /* region_data* region_table */

  MYSQL_RES *result;
  MYSQL_ROW row;

  int i = 0, vtx = 0;
  int j = 0;
  // int k = 0;

  int numrows;

  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  char **tokens; /* Storage for tokenized linestring points */

  /* Check if MySQL is available */
  if (!mysql_available || !conn) {
    log("INFO: Skipping region loading - MySQL not available.");
    return;
  }
  char **it;     /* Token iterator */

  log("INFO: Loading region data from MySQL");

  snprintf(buf, sizeof(buf), "SELECT vnum, "
                             "zone_vnum, "
                             "name, "
                             "region_type, "
                             "IFNULL(ST_NumPoints(ST_ExteriorRing(`region_polygon`)), 0), "
                             "IFNULL(ST_AsText(ST_ExteriorRing(region_polygon)), ''), "
                             "region_props, "
                             "region_reset_data, "
                             "region_reset_time "
                             "from region_data");

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from region_data 1: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from region_data 2: %s", mysql_error(conn));
    exit(1);
  }

  if ((numrows = mysql_num_rows(result)) < 1)
    return;
  else
  {
    if (region_table != NULL)
    {
      /* CRITICAL: Clear all region events FIRST before any memory operations.
       * This prevents use-after-free when events try to access region_table 
       * during cancellation. We must clear events for ALL regions before
       * freeing ANY region memory. */
      for (j = 0; j <= top_of_region_table; j++)
      {
        clear_region_event_list(&region_table[j]);
      }
      
      /* Now it's safe to free region memory */
      for (j = 0; j <= top_of_region_table; j++)
      {
        free(region_table[j].name);
        free(region_table[j].vertices);
        if (region_table[j].reset_data)
          free(region_table[j].reset_data);
      }
      free(region_table);
      region_table = NULL;
    }
    /* Allocate memory for all of the region data. */
    CREATE(region_table, struct region_data, numrows);
  }

  while ((row = mysql_fetch_row(result)))
  {
    /* Skip rows with NULL values in required fields (except row[5] which can be empty for regions without polygons) */
    if (!row[0] || !row[1] || !row[2] || !row[3] || !row[4] || !row[6]) {
      log("SYSERR: NULL value in region row, skipping");
      /* Initialize empty region to avoid uninitialized memory */
      region_table[i].vnum = -1;
      region_table[i].rnum = i;
      region_table[i].zone = NOWHERE;
      region_table[i].name = strdup("INVALID");
      region_table[i].region_type = 0;
      region_table[i].num_vertices = 0;
      region_table[i].vertices = NULL;
      region_table[i].region_props = 0;
      region_table[i].reset_time = 0;
      region_table[i].reset_data = NULL;
      region_table[i].events = NULL;  /* CRITICAL: Initialize events list to NULL */
      i++;
      continue;
    }
    
    region_table[i].vnum = atoi(row[0]);
    region_table[i].rnum = i;
    region_table[i].zone = real_zone(atoi(row[1]));
    region_table[i].name = strdup(row[2]);
    region_table[i].region_type = atoi(row[3]);
    region_table[i].num_vertices = atoi(row[4]);
    region_table[i].region_props = atoi(row[6]);
    region_table[i].reset_time = 0;
    region_table[i].reset_data = NULL;
    region_table[i].events = NULL;  /* CRITICAL: Initialize events list to NULL */

    /* Validate num_vertices is within reasonable bounds */
    if (region_table[i].num_vertices < 0 || region_table[i].num_vertices > 1024) {
      log("SYSERR: Invalid num_vertices (%d) for region %s (vnum %d), setting to 0",
          region_table[i].num_vertices, row[2], region_table[i].vnum);
      region_table[i].num_vertices = 0;
      region_table[i].vertices = NULL;
    }
    /* Check if we have polygon data */
    else if (region_table[i].num_vertices == 0 || !row[5] || strlen(row[5]) == 0) {
      /* No polygon data - create empty region */
      region_table[i].vertices = NULL;
      log("INFO: Region %d (%s) has no polygon data", region_table[i].vnum, region_table[i].name);
    } else {
      /* Parse the polygon text data to get the vertices, etc.
         eg: LINESTRING(0 0,10 0,10 10,0 10,0 0) */
      sscanf(row[5], "LINESTRING(%[^)])", buf2);
      tokens = tokenize(buf2, ",");
      if (!tokens) {
        log("SYSERR: tokenize() failed in load_regions for region %s", row[2]);
        /* Set vertices to NULL but keep the region entry */
        region_table[i].num_vertices = 0;
        region_table[i].vertices = NULL;
      } else {

      CREATE(region_table[i].vertices, struct vertex, region_table[i].num_vertices);

      vtx = 0;

      for (it = tokens; it && *it && vtx < region_table[i].num_vertices; ++it)
      {
        sscanf(*it, "%d %d", &(region_table[i].vertices[vtx].x), &(region_table[i].vertices[vtx].y));
        vtx++;
      }
      free_tokens(tokens);
      }
    }

    /* Store event data for later creation */
    if (region_table[i].region_type == REGION_ENCOUNTER &&
        row[8] && atoi(row[8]) > 0)
    {
      region_table[i].reset_time = atoi(row[8]);
      if (row[7])
        region_table[i].reset_data = strdup(row[7]);
      else
        region_table[i].reset_data = strdup("");
    }
    i++;
  }

  /* Set top_of_region_table to the last valid index */
  if (i > 0) {
    top_of_region_table = i - 1;
    log("INFO: Loaded %d regions, top_of_region_table set to %d", i, top_of_region_table);
    
    /* Now create events after top_of_region_table is set */
    for (j = 0; j <= top_of_region_table; j++) {
      if (region_table[j].region_type == REGION_ENCOUNTER &&
          region_table[j].reset_time > 0)
      {
        region_vnum *vnum_ptr = NULL;
        CREATE(vnum_ptr, region_vnum, 1);
        *vnum_ptr = region_table[j].vnum;
        
        log("Creating encounter reset event for region #%d (%s) - resets every %d seconds", 
            region_table[j].vnum, region_table[j].name, region_table[j].reset_time);
        NEW_EVENT(eENCOUNTER_REG_RESET, vnum_ptr, region_table[j].reset_data, region_table[j].reset_time RL_SEC);
      }
    }
  } else {
    top_of_region_table = -1;
    log("INFO: No regions loaded, top_of_region_table set to -1");
  }

  mysql_free_result(result);
}

/* Move this out to another file... */
bool is_point_within_region(region_vnum region, int x, int y)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool retval;

  char buf[1024];

  /* Need an ORDER BY here, since we can have multiple regions. */
  snprintf(buf, sizeof(buf), "SELECT 1 "
                             "from region_index "
                             "where vnum = %d and "
                             "ST_Within(ST_GeomFromText('POINT(%d %d)'), region_polygon)",
           region,
           x, y);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    exit(1);
  }

  retval = false;
  while ((row = mysql_fetch_row(result)))
  {
    retval = true;
  }
  mysql_free_result(result);

  return retval;
}

struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  struct region_list *regions = NULL;
  struct region_list *new_node = NULL;

  char buf[1024];

  /* Need an ORDER BY here, since we can have multiple regions. */
  snprintf(buf, sizeof(buf), "SELECT vnum,  "
                             "case "
                             "  when ST_Within(ST_GeomFromText('Point(%d %d)'), region_polygon) then "
                             "  case "
                             "    when (ST_GeomFromText('Point(%d %d)') = ST_Centroid(region_polygon)) then '1' "
                             "    when (ST_Distance(ST_GeomFromText('Point(%d %d)'), ST_ExteriorRing(region_polygon)) > "
                             "          ST_Distance(ST_GeomFromText('Point(%d %d)'), ST_Centroid(region_polygon))/2) then '2' "
                             "    else '3' "
                             "  end "
                             "  else NULL "
                             "end as loc "
                             "  from region_index "
                             "  where zone_vnum = %d "
                             "  and ST_Within(ST_GeomFromText('POINT(%d %d)'), region_polygon)",
           x, y,
           x, y,
           x, y,
           x, y,
           zone_table[zone].number,
           x, y);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {
    region_rnum rnum = real_region(atoi(row[0]));
    
    /* Skip regions that don't exist in the region table */
    if (rnum == NOWHERE) {
      log("SYSERR: Region vnum %d from database not found in region table", atoi(row[0]));
      continue;
    }

    /* Allocate memory for the region data. */
    CREATE(new_node, struct region_list, 1);
    new_node->rnum = rnum;
    if (atoi(row[1]) == 1)
      new_node->pos = REGION_POS_CENTER;
    else if (atoi(row[1]) == 2)
      new_node->pos = REGION_POS_INSIDE;
    else if (atoi(row[1]) == 3)
      new_node->pos = REGION_POS_EDGE;
    else
      new_node->pos = REGION_POS_UNDEFINED;
    new_node->next = regions;
    regions = new_node;
    new_node = NULL;
  }
  mysql_free_result(result);

  return regions;
}

/* Free a region list created by get_enclosing_regions() */
void free_region_list(struct region_list *regions)
{
  struct region_list *temp;
  
  while (regions) {
    temp = regions;
    regions = regions->next;
    free(temp);
  }
}

/* Free a path list created by get_enclosing_paths() */
void free_path_list(struct path_list *paths)
{
  struct path_list *temp;
  
  while (paths) {
    temp = paths;
    paths = paths->next;
    free(temp);
  }
}

#define ROUND(num) (num < 0 ? num - 0.5 : num + 0.5)

/* Move this out to another file... */
struct region_proximity_list *get_nearby_regions(zone_rnum zone, int x, int y, int r)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  struct region_proximity_list *regions = NULL;
  struct region_proximity_list *new_node = NULL;

  int i = 0;

  char buf[6000];

  /* Need an ORDER BY here, since we can have multiple regions. */
  snprintf(buf, sizeof(buf), "select * from (select "
                             "  ri.vnum, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as n, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as ne, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as e, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as se, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as s, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as sw, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as w, "
                             "  case "
                             "    when ST_Intersects(ri.region_polygon, "
                             "                       ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')) "
                             "    then "
                             "      CASE "
                             "        WHEN ST_GeometryType(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) = 'GEOMETRYCOLLECTION' "
                             "        THEN COALESCE(ST_Area(ST_GeometryN(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))')), 1)), 0.0) "
                             "        ELSE ST_Area(ST_Intersection(ri.region_polygon, ST_GeomFromText('polygon((%d %d, %f %f, %f %f, %d %d))'))) "
                             "      END "
                             "    else 0.0 end as nw, "
                             "  ST_Distance(ri.region_polygon, ST_GeomFromText('Point(%d %d)')) as dist "
                             "  from region_index as ri, "
                             "       region_data as rd "
                             "  where ri.vnum = rd.vnum and"
                             "        rd.region_type = 1 "
                             "  order by ST_Distance(ri.region_polygon, ST_GeomFromText('Point(%d %d)')) desc " // GEOGRAPHIC regions only.
                             " ) nearby_regions "
                             "  where ((n > 0) or (ne > 0) or (e > 0) or (se > 0) or (s > 0) or (sw > 0) or (w > 0) or (nw > 0));",
           /* n - 4 polygons */
           x, y, (r * -.5 + x), (r * .87 + y), (r * .5 + x), (r * .87 + y), x, y,
           x, y, (r * -.5 + x), (r * .87 + y), (r * .5 + x), (r * .87 + y), x, y,
           x, y, (r * -.5 + x), (r * .87 + y), (r * .5 + x), (r * .87 + y), x, y,
           x, y, (r * -.5 + x), (r * .87 + y), (r * .5 + x), (r * .87 + y), x, y,
           /* ne - 4 polygons */
           x, y, (r * .5 + x), (r * .87 + y), (r * .87 + x), (r * .5 + y), x, y,
           x, y, (r * .5 + x), (r * .87 + y), (r * .87 + x), (r * .5 + y), x, y,
           x, y, (r * .5 + x), (r * .87 + y), (r * .87 + x), (r * .5 + y), x, y,
           x, y, (r * .5 + x), (r * .87 + y), (r * .87 + x), (r * .5 + y), x, y,
           /* e - 4 polygons */
           x, y, (r * .87 + x), (r * .5 + y), (r * .87 + x), (r * -.5 + y), x, y,
           x, y, (r * .87 + x), (r * .5 + y), (r * .87 + x), (r * -.5 + y), x, y,
           x, y, (r * .87 + x), (r * .5 + y), (r * .87 + x), (r * -.5 + y), x, y,
           x, y, (r * .87 + x), (r * .5 + y), (r * .87 + x), (r * -.5 + y), x, y,
           /* se - 4 polygons */
           x, y, (r * .87 + x), (r * -.5 + y), (r * .5 + x), (r * -.87 + y), x, y,
           x, y, (r * .87 + x), (r * -.5 + y), (r * .5 + x), (r * -.87 + y), x, y,
           x, y, (r * .87 + x), (r * -.5 + y), (r * .5 + x), (r * -.87 + y), x, y,
           x, y, (r * .87 + x), (r * -.5 + y), (r * .5 + x), (r * -.87 + y), x, y,
           /* s - 4 polygons */
           x, y, (r * .5 + x), (r * -.87 + y), (r * -.5 + x), (r * -.87 + y), x, y,
           x, y, (r * .5 + x), (r * -.87 + y), (r * -.5 + x), (r * -.87 + y), x, y,
           x, y, (r * .5 + x), (r * -.87 + y), (r * -.5 + x), (r * -.87 + y), x, y,
           x, y, (r * .5 + x), (r * -.87 + y), (r * -.5 + x), (r * -.87 + y), x, y,
           /* sw - 4 polygons */
           x, y, (r * -.5 + x), (r * -.87 + y), (r * -.87 + x), (r * -.5 + y), x, y,
           x, y, (r * -.5 + x), (r * -.87 + y), (r * -.87 + x), (r * -.5 + y), x, y,
           x, y, (r * -.5 + x), (r * -.87 + y), (r * -.87 + x), (r * -.5 + y), x, y,
           x, y, (r * -.5 + x), (r * -.87 + y), (r * -.87 + x), (r * -.5 + y), x, y,
           /* w - 4 polygons */
           x, y, (r * -.87 + x), (r * -.5 + y), (r * -.87 + x), (r * .5 + y), x, y,
           x, y, (r * -.87 + x), (r * -.5 + y), (r * -.87 + x), (r * .5 + y), x, y,
           x, y, (r * -.87 + x), (r * -.5 + y), (r * -.87 + x), (r * .5 + y), x, y,
           x, y, (r * -.87 + x), (r * -.5 + y), (r * -.87 + x), (r * .5 + y), x, y,
           /* nw - 4 polygons */
           x, y, (r * -.87 + x), (r * .5 + y), (r * -.5 + x), (r * .87 + y), x, y,
           x, y, (r * -.87 + x), (r * .5 + y), (r * -.5 + x), (r * .87 + y), x, y,
           x, y, (r * -.87 + x), (r * .5 + y), (r * -.5 + x), (r * .87 + y), x, y,
           x, y, (r * -.87 + x), (r * .5 + y), (r * -.5 + x), (r * .87 + y), x, y,
           /* Points for distance calculations */
           x, y,
           x, y);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    log("SYSERR: get_nearby_regions failed for zone %d, coords (%d,%d)", zone, x, y);
    return NULL;  /* Return empty region list on error */
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from region_index: %s", mysql_error(conn));
    log("SYSERR: get_nearby_regions failed to store result for zone %d, coords (%d,%d)", zone, x, y);
    return NULL;  /* Return empty region list on error */
  }

  while ((row = mysql_fetch_row(result)))
  {
    region_rnum rnum = real_region(atoi(row[0]));
    
    /* Skip regions that don't exist in the region table */
    if (rnum == NOWHERE) {
      log("SYSERR: Region vnum %d from database not found in region table", atoi(row[0]));
      continue;
    }

    /* Allocate memory for the region data. */
    CREATE(new_node, struct region_proximity_list, 1);
    new_node->rnum = rnum;

    for (i = 0; i < 8; i++)
    {
      new_node->dirs[i] = atof(row[i + 1]);
    }
    new_node->dist = atof(row[9]);

    new_node->next = regions;
    regions = new_node;
    new_node = NULL;
  }
  mysql_free_result(result);

  return regions;
}

#undef ROUND

void save_regions()
{
}

void load_paths()
{
  /* path_data* path_table */

  MYSQL_RES *result;
  MYSQL_ROW row;

  int i = 0, vtx = 0, j = 0;
  int numrows;

  char buf[1024];
  char buf2[1024];

  char **tokens; /* Storage for tokenized linestring points */
  char **it;     /* Token iterator */

  log("INFO: Loading wilderness roads and path definitions from MySQL database");
  
  if (!mysql_available) {
    log("INFO: Skipping path loading - MySQL not available.");
    return;
  }

  snprintf(buf, sizeof(buf), "SELECT p.vnum, "
                             "p.zone_vnum, "
                             "p.name, "
                             "p.path_type, "
                             "ST_NumPoints(p.path_linestring), "
                             "ST_AsText(p.path_linestring), "
                             "p.path_props, "
                             "pt.glyph_ns, "
                             "pt.glyph_ew, "
                             "pt.glyph_int "
                             "  from path_data p,"
                             "       path_types pt"
                             "  where p.path_type = pt.path_type");

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from path_data: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from path_data: %s", mysql_error(conn));
    exit(1);
  }

  if ((numrows = mysql_num_rows(result)) < 1)
    return;
  else
  {
    /* Allocate memory for all of the region data. */
    if (path_table != NULL)
    {
      /* Clear it */
      for (j = 0; j <= top_of_path_table; j++)
      {
        free(path_table[j].name);
        // free(path_table[i].glyphs[GLYPH_TYPE_PATH_NS]);
        // free(path_table[i].glyphs[GLYPH_TYPE_PATH_EW]);
        // free(path_table[i].glyphs[GLYPH_TYPE_PATH_INT]);
        free(path_table[j].vertices);
      }
      free(path_table);
    }
    CREATE(path_table, struct path_data, numrows);
  }

  while ((row = mysql_fetch_row(result)))
  {
    path_table[i].vnum = atoi(row[0]);
    path_table[i].rnum = i;
    path_table[i].zone = real_zone(atoi(row[1]));
    path_table[i].name = strdup(row[2]);
    path_table[i].path_type = atoi(row[3]);
    path_table[i].num_vertices = atoi(row[4]);
    path_table[i].path_props = atoi(row[6]);

    path_table[i].glyphs[GLYPH_TYPE_PATH_NS] = strdup(row[7]);
    path_table[i].glyphs[GLYPH_TYPE_PATH_EW] = strdup(row[8]);
    path_table[i].glyphs[GLYPH_TYPE_PATH_INT] = strdup(row[9]);

    parse_at(path_table[i].glyphs[GLYPH_TYPE_PATH_NS]);
    parse_at(path_table[i].glyphs[GLYPH_TYPE_PATH_EW]);
    parse_at(path_table[i].glyphs[GLYPH_TYPE_PATH_INT]);

    /* Validate num_vertices is within reasonable bounds */
    if (path_table[i].num_vertices < 0 || path_table[i].num_vertices > 1024) {
      log("SYSERR: Invalid num_vertices (%d) for path %s (vnum %d), setting to 0",
          path_table[i].num_vertices, row[2], path_table[i].vnum);
      path_table[i].num_vertices = 0;
      path_table[i].vertices = NULL;
    }
    /* Check if we have valid polygon data */
    else if (path_table[i].num_vertices == 0) {
      /* No polygon data - create empty path */
      path_table[i].vertices = NULL;
      log("INFO: Path %d (%s) has no polygon data", path_table[i].vnum, path_table[i].name);
    } else {
    /* Parse the polygon text data to get the vertices, etc.
       eg: LINESTRING(0 0,10 0,10 10,0 10,0 0) */
    sscanf(row[5], "LINESTRING(%[^)])", buf2);
    tokens = tokenize(buf2, ",");
    if (!tokens) {
      log("SYSERR: tokenize() failed in load_paths for path %s", row[2]);
      /* Set vertices to NULL but keep the path entry */
      path_table[i].num_vertices = 0;
      path_table[i].vertices = NULL;
    } else {

    CREATE(path_table[i].vertices, struct vertex, path_table[i].num_vertices);

    vtx = 0;

    for (it = tokens; it && *it && vtx < path_table[i].num_vertices; ++it)
    {
      sscanf(*it, "%d %d", &(path_table[i].vertices[vtx].x), &(path_table[i].vertices[vtx].y));
      vtx++;
    }
    free_tokens(tokens);
    }
    }

    top_of_path_table = i;
    i++;
  }
  mysql_free_result(result);
}

/* Insert a path into the database. */
void insert_path(struct path_data *path)
{
  /* path_data* path_table */
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int vtx = 0;
  char linestring[MAX_STRING_LENGTH] = {'\0'};

  snprintf(linestring, sizeof(linestring), "ST_GeomFromText('LINESTRING(");

  for (vtx = 0; vtx < path->num_vertices; vtx++)
  {
    char buf2[100];
    snprintf(buf2, sizeof(buf2), "%d %d%s", path->vertices[vtx].x, path->vertices[vtx].y, (vtx + 1 == path->num_vertices ? ")')" : ","));
    strlcat(linestring, buf2, sizeof(linestring));
  }

  log("INFO: Inserting Path [%d] '%s' into MySQL:", (int)path->vnum, path->name);
  snprintf(buf, sizeof(buf), "insert into path_data "
                             "(vnum, "
                             "zone_vnum, "
                             "path_type, "
                             "name, "
                             "path_props, "
                             "path_linestring) "
                             "VALUES ("
                             "%d, "
                             "%d, "
                             "%d, "
                             "'%s', "
                             "%d, "
                             "%s);",
           path->vnum, zone_table[path->zone].number, path->path_type, path->name, path->path_props, linestring);

  log("QUERY: %s", buf);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to INSERT into path_data: %s", mysql_error(conn));
  }
}

/* Delete a path from the database. */
bool delete_path(region_vnum vnum)
{
  /* path_data* path_table */
  char buf[MAX_STRING_LENGTH] = {'\0'};

  log("INFO: Deleting Path [%d] from MySQL:", (int)vnum);
  snprintf(buf, sizeof(buf), "delete from path_data "
                             "where vnum = %d;",
           (int)vnum);

  log("QUERY: %s", buf);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to delete from path_data: %s", mysql_error(conn));
    return false;
  }

  if (mysql_affected_rows(conn))
    return true;
  else
    return false;
}

struct path_list *get_enclosing_paths(zone_rnum zone, int x, int y)
{
  MYSQL_RES *result;
  MYSQL_ROW row;

  struct path_list *paths = NULL;
  struct path_list *new_node = NULL;

  char buf[1024];

  snprintf(buf, sizeof(buf), "SELECT vnum, "
                             "  CASE WHEN (ST_Within(ST_GeomFromText('POINT(%d %d)'), path_linestring) AND "
                             "             ST_Within(ST_GeomFromText('POINT(%d %d)'), path_linestring)) THEN %d"
                             "    WHEN (ST_Within(ST_GeomFromText('POINT(%d %d)'), path_linestring) AND "
                             "               ST_Within(ST_GeomFromText('POINT(%d %d)'), path_linestring)) THEN %d "
                             "    ELSE %d"
                             "  END AS glyph "
                             "  from path_index "
                             "  where zone_vnum = %d "
                             "  and ST_Within(ST_GeomFromText('POINT(%d %d)'), path_linestring)",
           x, y - 1, x, y + 1, GLYPH_TYPE_PATH_NS, x - 1, y, x + 1, y, GLYPH_TYPE_PATH_EW, GLYPH_TYPE_PATH_INT, zone_table[zone].number, x, y);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from path_index: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from path_index: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {

    /* Allocate memory for the region data. */
    CREATE(new_node, struct path_list, 1);
    new_node->rnum = real_path(atoi(row[0]));
    new_node->glyph_type = atoi(row[1]);
    new_node->next = paths;
    paths = new_node;
    new_node = NULL;
  }
  mysql_free_result(result);

  return paths;
}

void save_paths()
{
}

/*
 * Get a random point within the given region.  Note, the parameters x and y will
 * contain the random point!  If no point can be found then the function will not
 * change the values of x and y and will return false.  IF a point can be found that
 * point will be returned in x and y and the function will return true.
 *
 * This function accesses the database several times.
 */
bool get_random_region_location(region_vnum region, int *x, int *y)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int xlow, xhigh, ylow, yhigh;
  int xp, yp;

  char buf[MAX_STRING_LENGTH] = {'\0'};
  char buf2[MAX_STRING_LENGTH] = {'\0'};

  char **tokens; /* Storage for tokenized linestring points */
  char **it;     /* Token iterator */

  xlow = 99999;
  xhigh = -99999;
  ylow = 99999;
  yhigh = -99999;

  log(" Getting random point in region with vnum : %d", region);

  snprintf(buf, sizeof(buf), "SELECT ST_AsText(ST_Envelope(region_polygon)) "
                             "from region_data "
                             "where vnum = %d;",
           region);

  /* Check the connection, reconnect if necessary. */
  mysql_ping(conn);

  if (mysql_query(conn, buf))
  {
    log("SYSERR: Unable to SELECT from region_data 3: %s", mysql_error(conn));
    return false;
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from region_data 4: %s", mysql_error(conn));
    return false;
  }

  while ((row = mysql_fetch_row(result)))
  {

    /* Parse the polygon text data to get the vertices, etc.
       eg: LINESTRING(0 0,10 0,10 10,0 10,0 0) */
    log(" Envelope: %s", row[0]);
    sscanf(row[0], "POLYGON((%[^)]))", buf2);
    tokens = tokenize(buf2, ",");
    if (!tokens) {
      log("SYSERR: tokenize() failed in envelope()");
      /* Skip this envelope entry */
      continue;
    }

    int newx, newy;
    for (it = tokens; it && *it; ++it)
    {
      log(" Token: %s", *it);
      sscanf(*it, "%d %d", &newx, &newy);
      if (newx < xlow)
        xlow = newx;
      if (newx > xhigh)
        xhigh = newx;
      if (newy < ylow)
        ylow = newy;
      if (newy > yhigh)
        yhigh = newy;
    }
    free_tokens(tokens);
  }

  mysql_free_result(result);

  log("xrange: %d - %d yrange: %d - %d", xlow, xhigh, ylow, yhigh);

  do
  {
    xp = rand_number(xlow, xhigh);
    yp = rand_number(ylow, yhigh);
    log("new point: (%d, %d)", xp, yp);
  } while (!is_point_within_region(region, xp, yp));

  log("Returning point within region %d : (%d, %d)", region, xp, yp);
  *x = xp;
  *y = yp;
  return true;
}

#ifdef DO_NOT_COMPILE_EXAMPLES

void who_to_mysql()
{
  struct descriptor_data *d;
  struct char_data *tch;
  char buf[1024], buf2[MAX_TITLE_LENGTH * 2];

  if (mysql_query(conn, "DELETE FROM who"))
  {
    mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Unable to DELETE in who: %s", mysql_error(conn));
    return;
  }

  for (d = descriptor_list; d; d = d->next)
  {
    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    /* Hide those who are not 'playing' */
    if (!IS_PLAYING(d))
      continue;
    /* Hide invisible immortals */
    if (GET_INVIS_LEV(tch) > 1)
      continue;
    /* Hide invisible and hidden mortals */
    if (AFF_FLAGGED((tch), AFF_INVISIBLE) || AFF_FLAGGED((tch), AFF_HIDE))
      continue;

    /* title could have ' characters, we need to escape it */
    /* the mud crashed here on 16 Oct 2012, made some changes and checks */
    if (GET_TITLE(tch) != NULL && strlen(GET_TITLE(tch)) <= MAX_TITLE_LENGTH)
      mysql_real_escape_string(conn, buf2, GET_TITLE(tch), strlen(GET_TITLE(tch)));
    else
      buf2[0] = '\0';

    /* Escape player name */
    char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(tch));
    if (!escaped_name) {
      log("SYSERR: Failed to escape player name in write_who_to_mysql");
      continue;
    }

    /* Hide level for anonymous players */
    if (!IS_NPC(ch) && PRF_FLAGGED(tch, PRF_ANON))
    {
      snprintf(buf, sizeof(buf), "INSERT INTO who (player, title, killer, thief) VALUES ('%s', '%s', %d, %d)",
               escaped_name, buf2, PLR_FLAGGED(tch, PLR_KILLER) ? 1 : 0, PLR_FLAGGED(tch, PLR_THIEF) ? 1 : 0);
    }
    else
    {
      snprintf(buf, sizeof(buf), "INSERT INTO who (player, level, title, killer, thief) VALUES ('%s', %d, '%s', %d, %d)",
               escaped_name, GET_LEVEL(tch), buf2,
               PLR_FLAGGED(tch, PLR_KILLER) ? 1 : 0, PLR_FLAGGED(tch, PLR_THIEF) ? 1 : 0);
    }
    free(escaped_name);

    if (mysql_query(conn, buf))
    {
      mudlog(NRM, LVL_GOD, TRUE, "SYSERR: Unable to INSERT in who: %s", mysql_error(conn));
    }
  }
}

void read_factions_from_mysql()
{
  struct faction_data *fact = NULL;
  MYSQL_RES *result;
  MYSQL_ROW row;
  int i = 0, total;

  if (mysql_query(conn, "SELECT f.id, f.name, f.flags, f.gold, f.tax, COUNT(r.rank) FROM factions AS f LEFT JOIN faction_ranks AS r ON f.id = r.faction GROUP BY f.id ORDER BY f.name"))
  {
    log("SYSERR: Unable to SELECT from factions: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from factions: %s", mysql_error(conn));
    exit(1);
  }

  faction_count = mysql_num_rows(result);
  CREATE(factions, struct faction_data, faction_count);

  while ((row = mysql_fetch_row(result)))
  {
    factions[i].id = strdup(row[0]);
    factions[i].name = strdup(row[1]);
    factions[i].flags = atoi(row[2]);
    factions[i].gold = atoll(row[3]);
    factions[i].tax = atof(row[4]);
    factions[i].num_ranks = atoi(row[5]);

    if (factions[i].num_ranks > 0)
      CREATE(factions[i].ranks, struct faction_rank, factions[i].num_ranks);

    i++;
  }

  mysql_free_result(result);

  /* Read faction ranks */
  if (mysql_query(conn, "SELECT faction, rank, name FROM faction_ranks ORDER BY faction, rank"))
  {
    log("SYSERR: Unable to SELECT from faction_ranks: %s", mysql_error(conn));
    exit(1);
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from faction_ranks: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {
    /* Select correct faction */
    if (!fact || strcmp(fact->id, row[0]))
      fact = find_faction(row[0], NULL);

    /* If we were unable to select the correct faction, exit with a serious error */
    if (!fact)
    {
      log("SYSERR: Rank for unexisting faction %s.", row[0]);
      exit(1);
    }

    fact->ranks[atoi(row[1])].name = strdup(row[2]);
  }

  mysql_free_result(result);

  /* Read faction skillgroups */
  if (mysql_query(conn, "SELECT faction_id, skillgroup_id FROM faction_skillgroups ORDER BY faction_id"))
  {
    log("SYSERR: Unable to SELECT from faction_skillgroups: %s", mysql_error(conn));
    exit(1);
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from faction_skillgroups: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {
    /* Select correct faction */
    if (!fact || strcmp(fact->id, row[0]))
      fact = find_faction(row[0], NULL);

    /* If we were unable to select the correct faction, exit with a serious error */
    if (!fact)
    {
      log("SYSERR: Skillgroup for unexisting faction %s.", row[0]);
      exit(1);
    }

    i = atoi(row[1]);

    if (i < 0 || i >= NUM_SKILLGROUPS)
    {
      log("SYSERR: Invalid skillgroup (%d) for faction %s.", i, fact->id);
      exit(1);
    }

    fact->skillgroups[i] = 1;
  }

  mysql_free_result(result);

  /* Read monster ownership */
  if (mysql_query(conn, "SELECT monster, faction, count FROM mobile_shares ORDER BY count DESC"))
  {
    log("SYSERR: Unable to SELECT from mobile_shares: %s", mysql_error(conn));
    exit(1);
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from mobile_shares: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {
    if ((fact = find_faction(row[1], NULL)))
    {
      i = atoi(row[0]);
      total = atoi(row[2]);

      if (i <= 0 || total <= 0 || total > TOTAL_SHARES)
        log("SYSERR: Invalid mob %s or sharecount %s for faction %s.", row[0], row[2], row[1]);
      else
        set_monster_ownership(i, fact, total);
    }
    else
    {
      log("SYSERR: Mob %s owns shares in unknown faction %s.", row[0], row[1]);
    }
  }

  mysql_free_result(result);

  /* Read player ownership */
  if (mysql_query(conn, "SELECT player, faction, count FROM player_shares ORDER BY count DESC"))
  {
    log("SYSERR: Unable to SELECT from player_shares: %s", mysql_error(conn));
    exit(1);
  }
  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from player_shares: %s", mysql_error(conn));
    exit(1);
  }

  while ((row = mysql_fetch_row(result)))
  {
    if ((fact = find_faction(row[1], NULL)))
    {
      total = atoi(row[2]);

      if (total <= 0 || total > TOTAL_SHARES)
        log("SYSERR: Invalid player %s sharecount %s for faction %s.", row[0], row[2], row[1]);
      else
        set_player_ownership(row[0], fact, total);
    }
    else
    {
      log("SYSERR: Player %s owns shares in unknown faction %s.", row[0], row[1]);
    }
  }

  mysql_free_result(result);

  /* Check that we don't go above TOTAL_SHARES */
  for (i = 0; i < faction_count; i++)
  {
    total = faction_total_ownership(&factions[i]);
    if (total > TOTAL_SHARES)
    {
      log("SYSERR: Faction %s has %d total shares (max %d).", factions[i].id, total, TOTAL_SHARES);
    }
  }
}

#endif
