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

#define MYSQL_DEBUG 0

/* Global connection pool */
MYSQL_POOL *mysql_pool = NULL;

/* Legacy connections - maintained for backwards compatibility */
MYSQL *conn = NULL;
MYSQL *conn2 = NULL;
MYSQL *conn3 = NULL;

/* MySQL connection mutexes for thread safety (legacy) */
pthread_mutex_t mysql_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mysql_mutex3 = PTHREAD_MUTEX_INITIALIZER;

void after_world_load()
{
}

/* ========================================================================== */
/* MySQL Connection Pool Implementation                                       */
/* ========================================================================== */
/* The connection pool manages multiple MySQL connections efficiently,        */
/* reusing connections instead of creating new ones for each query.          */
/* This improves performance and reduces connection overhead.                */

/**
 * Initialize the MySQL connection pool.
 * Creates the initial set of connections and sets up pool management.
 * 
 * This function allocates the global mysql_pool structure and creates
 * MYSQL_POOL_MIN_SIZE connections. Each connection is configured with
 * auto-reconnect enabled for resilience.
 */
void mysql_pool_init(void)
{
  int i;
  MYSQL_POOL_CONN *pc, *prev = NULL;
  
  /* Check if already initialized */
  if (mysql_pool && mysql_pool->initialized) {
    log("WARNING: Connection pool already initialized");
    return;
  }
  
  /* Check pool exists with configuration */
  if (!mysql_pool) {
    log("ERROR: Pool structure not allocated before mysql_pool_init");
    return;
  }
  
  /* Create initial connections */
  for (i = 0; i < MYSQL_POOL_MIN_SIZE; i++) {
    CREATE(pc, MYSQL_POOL_CONN, 1);
    pc->id = i;
    pc->state = CONN_STATE_FREE;
    pc->last_used = time(NULL);
    pc->created = time(NULL);
    pc->conn = NULL;
    pc->thread_id = 0;
    pc->next = NULL;
    pthread_mutex_init(&pc->mutex, NULL);
    
    /* Initialize MySQL connection */
    pc->conn = mysql_init(NULL);
    if (!pc->conn) {
      log("ERROR: Failed to initialize MySQL connection %d in pool", i);
      free(pc);
      continue;
    }
    
    /* Set connection options */
    my_bool reconnect = 1;
    mysql_options(pc->conn, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
    
    /* Connect to database */
    if (!mysql_real_connect(pc->conn, mysql_pool->host, mysql_pool->username,
                           mysql_pool->password, mysql_pool->database,
                           0, NULL, 0)) {
      log("ERROR: Failed to connect MySQL connection %d: %s", 
          i, mysql_error(pc->conn));
      mysql_close(pc->conn);
      free(pc);
      continue;
    }
    
    /* Get thread ID for this connection */
    pc->thread_id = mysql_thread_id(pc->conn);
    
    /* Add to pool linked list */
    if (!mysql_pool->connections) {
      mysql_pool->connections = pc;
    } else {
      prev->next = pc;
    }
    prev = pc;
    mysql_pool->current_size++;
    
    log("INFO: Created pool connection %d (thread_id: %lu)", i, pc->thread_id);
  }
  
  if (mysql_pool->current_size > 0) {
    mysql_pool->initialized = TRUE;
    mysql_available = TRUE;
    log("Success: MySQL connection pool initialized with %d connections", 
        mysql_pool->current_size);
  } else {
    log("ERROR: Failed to create any connections in pool");
  }
}

/**
 * Destroy the connection pool and free all resources.
 * Closes all MySQL connections and deallocates memory.
 */
void mysql_pool_destroy(void)
{
  MYSQL_POOL_CONN *pc, *next;
  
  if (!mysql_pool) {
    return;
  }
  
  /* Lock the pool */
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  /* Close all connections */
  pc = mysql_pool->connections;
  while (pc) {
    next = pc->next;
    
    /* Close MySQL connection */
    if (pc->conn) {
      mysql_close(pc->conn);
    }
    
    /* Destroy connection mutex */
    pthread_mutex_destroy(&pc->mutex);
    
    /* Free connection structure */
    free(pc);
    pc = next;
  }
  
  mysql_pool->initialized = FALSE;
  mysql_available = FALSE;
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
  
  /* Destroy pool mutex and condition variable */
  pthread_mutex_destroy(&mysql_pool->pool_mutex);
  pthread_cond_destroy(&mysql_pool->pool_cond);
  
  /* Free pool structure */
  free(mysql_pool);
  mysql_pool = NULL;
  
  log("Info: MySQL connection pool destroyed");
}

/**
 * Acquire a connection from the pool.
 * Returns an available connection or waits if all are in use.
 * 
 * @return Pointer to acquired connection, or NULL on error
 */
MYSQL_POOL_CONN *mysql_pool_acquire(void)
{
  MYSQL_POOL_CONN *pc;
  time_t now;
  
  if (!mysql_pool || !mysql_pool->initialized) {
    log("ERROR: Connection pool not initialized");
    return NULL;
  }
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  mysql_pool->total_requests++;
  
  /* Find a free connection */
  while (1) {
    now = time(NULL);
    
    /* Check for available connection */
    for (pc = mysql_pool->connections; pc; pc = pc->next) {
      if (pc->state == CONN_STATE_FREE) {
        /* Check if connection needs refresh */
        if (now - pc->last_used > MYSQL_POOL_TIMEOUT) {
          /* Ping to check if still alive */
          if (mysql_ping(pc->conn) != 0) {
            log("INFO: Refreshing stale connection %d", pc->id);
            mysql_close(pc->conn);
            
            /* Reconnect */
            pc->conn = mysql_init(NULL);
            if (pc->conn) {
              my_bool reconnect = 1;
              mysql_options(pc->conn, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
              
              if (!mysql_real_connect(pc->conn, mysql_pool->host,
                                     mysql_pool->username, mysql_pool->password,
                                     mysql_pool->database, 0, NULL, 0)) {
                log("ERROR: Failed to reconnect connection %d: %s",
                    pc->id, mysql_error(pc->conn));
                pc->state = CONN_STATE_ERROR;
                mysql_pool->error_count++;
                continue;
              }
              pc->thread_id = mysql_thread_id(pc->conn);
            }
          }
        }
        
        /* Mark as in use and return */
        pc->state = CONN_STATE_IN_USE;
        pc->last_used = now;
        mysql_pool->active_count++;
        
        pthread_mutex_unlock(&mysql_pool->pool_mutex);
        
        if (MYSQL_DEBUG) {
          log("DEBUG: Acquired connection %d from pool (active: %d/%d)",
              pc->id, mysql_pool->active_count, mysql_pool->current_size);
        }
        
        return pc;
      }
    }
    
    /* No free connections - check if we can expand pool */
    if (mysql_pool->current_size < MYSQL_POOL_MAX_SIZE) {
      pthread_mutex_unlock(&mysql_pool->pool_mutex);
      mysql_pool_expand();
      pthread_mutex_lock(&mysql_pool->pool_mutex);
      continue;
    }
    
    /* Wait for a connection to become available */
    mysql_pool->wait_count++;
    if (MYSQL_DEBUG) {
      log("DEBUG: Waiting for connection (all %d in use)", mysql_pool->current_size);
    }
    
    pthread_cond_wait(&mysql_pool->pool_cond, &mysql_pool->pool_mutex);
  }
  
  /* Should never reach here */
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
  return NULL;
}

/**
 * Release a connection back to the pool.
 * Makes the connection available for other requests.
 * 
 * @param pc Connection to release
 */
void mysql_pool_release(MYSQL_POOL_CONN *pc)
{
  if (!pc || !mysql_pool) {
    return;
  }
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  /* Mark as free */
  pc->state = CONN_STATE_FREE;
  pc->last_used = time(NULL);
  mysql_pool->active_count--;
  
  if (MYSQL_DEBUG) {
    log("DEBUG: Released connection %d to pool (active: %d/%d)",
        pc->id, mysql_pool->active_count, mysql_pool->current_size);
  }
  
  /* Signal waiting threads */
  pthread_cond_signal(&mysql_pool->pool_cond);
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
}

/**
 * Perform health checks on all connections in the pool.
 * Removes dead connections and creates replacements.
 */
void mysql_pool_health_check(void)
{
  MYSQL_POOL_CONN *pc;
  time_t now;
  int errors = 0;
  
  if (!mysql_pool || !mysql_pool->initialized) {
    return;
  }
  
  now = time(NULL);
  
  /* Check if it's time for health check */
  if (now - mysql_pool->last_health_check < MYSQL_HEALTH_CHECK_INTERVAL) {
    return;
  }
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  mysql_pool->last_health_check = now;
  
  /* Check each connection */
  for (pc = mysql_pool->connections; pc; pc = pc->next) {
    if (pc->state == CONN_STATE_FREE) {
      /* Ping the connection */
      if (mysql_ping(pc->conn) != 0) {
        log("WARNING: Connection %d failed health check: %s",
            pc->id, mysql_error(pc->conn));
        pc->state = CONN_STATE_ERROR;
        errors++;
        
        /* Try to reconnect */
        mysql_close(pc->conn);
        pc->conn = mysql_init(NULL);
        
        if (pc->conn) {
          my_bool reconnect = 1;
          mysql_options(pc->conn, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
          
          if (mysql_real_connect(pc->conn, mysql_pool->host,
                               mysql_pool->username, mysql_pool->password,
                               mysql_pool->database, 0, NULL, 0)) {
            pc->state = CONN_STATE_FREE;
            pc->thread_id = mysql_thread_id(pc->conn);
            pc->created = now;
            log("INFO: Reconnected connection %d during health check", pc->id);
            errors--;
          }
        }
      }
    }
  }
  
  if (errors > 0) {
    log("WARNING: %d connections failed health check", errors);
  }
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
  
  /* Shrink pool if too many idle connections */
  mysql_pool_shrink();
}

/**
 * Expand the connection pool by adding new connections.
 * Called when pool is exhausted and below maximum size.
 */
void mysql_pool_expand(void)
{
  MYSQL_POOL_CONN *pc, *last;
  int new_id;
  
  if (!mysql_pool || mysql_pool->current_size >= MYSQL_POOL_MAX_SIZE) {
    return;
  }
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  /* Find last connection in list */
  last = mysql_pool->connections;
  while (last && last->next) {
    last = last->next;
  }
  
  new_id = mysql_pool->current_size;
  
  /* Create new connection */
  CREATE(pc, MYSQL_POOL_CONN, 1);
  pc->id = new_id;
  pc->state = CONN_STATE_FREE;
  pc->last_used = time(NULL);
  pc->created = time(NULL);
  pc->conn = NULL;
  pc->thread_id = 0;
  pc->next = NULL;
  pthread_mutex_init(&pc->mutex, NULL);
  
  /* Initialize MySQL connection */
  pc->conn = mysql_init(NULL);
  if (!pc->conn) {
    log("ERROR: Failed to initialize new connection %d", new_id);
    free(pc);
    pthread_mutex_unlock(&mysql_pool->pool_mutex);
    return;
  }
  
  /* Set options */
  my_bool reconnect = 1;
  mysql_options(pc->conn, MYSQL_OPT_RECONNECT, (const char *)&reconnect);
  
  /* Connect */
  if (!mysql_real_connect(pc->conn, mysql_pool->host, mysql_pool->username,
                         mysql_pool->password, mysql_pool->database,
                         0, NULL, 0)) {
    log("ERROR: Failed to connect new connection %d: %s",
        new_id, mysql_error(pc->conn));
    mysql_close(pc->conn);
    free(pc);
    pthread_mutex_unlock(&mysql_pool->pool_mutex);
    return;
  }
  
  pc->thread_id = mysql_thread_id(pc->conn);
  
  /* Add to pool */
  if (last) {
    last->next = pc;
  } else {
    mysql_pool->connections = pc;
  }
  
  mysql_pool->current_size++;
  
  log("INFO: Expanded pool to %d connections", mysql_pool->current_size);
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
}

/**
 * Shrink the pool by removing idle connections.
 * Maintains at least MYSQL_POOL_MIN_SIZE connections.
 */
void mysql_pool_shrink(void)
{
  MYSQL_POOL_CONN *pc, *prev, *to_remove;
  time_t now;
  int removed = 0;
  
  if (!mysql_pool || mysql_pool->current_size <= MYSQL_POOL_MIN_SIZE) {
    return;
  }
  
  now = time(NULL);
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  prev = NULL;
  pc = mysql_pool->connections;
  
  while (pc && mysql_pool->current_size > MYSQL_POOL_MIN_SIZE) {
    /* Check if connection is idle and old */
    if (pc->state == CONN_STATE_FREE &&
        (now - pc->last_used) > (MYSQL_POOL_TIMEOUT * 2)) {
      
      /* Remove this connection */
      to_remove = pc;
      
      if (prev) {
        prev->next = pc->next;
      } else {
        mysql_pool->connections = pc->next;
      }
      
      pc = pc->next;
      
      /* Close and free the connection */
      if (to_remove->conn) {
        mysql_close(to_remove->conn);
      }
      pthread_mutex_destroy(&to_remove->mutex);
      free(to_remove);
      
      mysql_pool->current_size--;
      removed++;
    } else {
      prev = pc;
      pc = pc->next;
    }
  }
  
  if (removed > 0) {
    log("INFO: Shrunk pool by %d connections (now %d)",
        removed, mysql_pool->current_size);
  }
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
}

/**
 * Get statistics about the connection pool.
 * 
 * @param buf Buffer to write statistics
 * @param size Size of buffer
 */
void mysql_pool_stats(char *buf, size_t size)
{
  if (!mysql_pool) {
    snprintf(buf, size, "Connection pool not initialized");
    return;
  }
  
  pthread_mutex_lock(&mysql_pool->pool_mutex);
  
  snprintf(buf, size,
          "Pool Statistics:\n"
          "  Current Size: %d\n"
          "  Active Connections: %d\n"
          "  Total Requests: %lu\n"
          "  Wait Count: %lu\n"
          "  Error Count: %lu\n"
          "  Uptime: %ld seconds",
          mysql_pool->current_size,
          mysql_pool->active_count,
          mysql_pool->total_requests,
          mysql_pool->wait_count,
          mysql_pool->error_count,
          mysql_pool->connections ? time(NULL) - mysql_pool->connections->created : 0);
  
  pthread_mutex_unlock(&mysql_pool->pool_mutex);
}

/**
 * Pool-aware query function that automatically manages connections.
 * 
 * @param query SQL query to execute
 * @param result Pointer to store result set (can be NULL for non-SELECT)
 * @return 0 on success, non-zero on error
 */
int mysql_pool_query(const char *query, MYSQL_RES **result)
{
  MYSQL_POOL_CONN *pc;
  int ret;
  
  /* Acquire connection from pool */
  pc = mysql_pool_acquire();
  if (!pc) {
    log("ERROR: Failed to acquire connection from pool");
    return -1;
  }
  
  /* Execute query */
  pthread_mutex_lock(&pc->mutex);
  ret = mysql_query(pc->conn, query);
  
  if (ret == 0 && result) {
    /* Store result if requested */
    *result = mysql_store_result(pc->conn);
  }
  
  pthread_mutex_unlock(&pc->mutex);
  
  /* Release connection back to pool */
  mysql_pool_release(pc);
  
  return ret;
}

/**
 * Free a result set obtained from mysql_pool_query.
 * 
 * @param result Result set to free
 */
void mysql_pool_free_result(MYSQL_RES *result)
{
  if (result) {
    mysql_free_result(result);
  }
}

/* ========================================================================== */
/* End of Connection Pool Implementation                                      */
/* ========================================================================== */

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

  /* Initialize connection pool structure first */
  if (!mysql_pool) {
    CREATE(mysql_pool, MYSQL_POOL, 1);
    mysql_pool->connections = NULL;
    mysql_pool->current_size = 0;
    mysql_pool->active_count = 0;
    mysql_pool->initialized = FALSE;
    mysql_pool->total_requests = 0;
    mysql_pool->wait_count = 0;
    mysql_pool->error_count = 0;
    mysql_pool->last_health_check = time(NULL);
    pthread_mutex_init(&mysql_pool->pool_mutex, NULL);
    pthread_cond_init(&mysql_pool->pool_cond, NULL);
  }
  
  /* Read the mysql configuration file from lib/ directory */
  if (!(file = fopen("mysql_config", "r")))
  {
    log("WARNING: Unable to read MySQL configuration from 'lib/mysql_config'.");
    log("WARNING: Running without MySQL support - some features will be disabled.");
    log("WARNING: To enable MySQL: Copy lib/mysql_config_example to lib/mysql_config and edit it.");
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
      if (!str_cmp(key, "mysql_host")) {
        strlcpy(host, val, sizeof(host));
        strlcpy(mysql_pool->host, val, sizeof(mysql_pool->host));
      }
      /* Database name configuration */
      else if (!str_cmp(key, "mysql_database")) {
        strlcpy(database, val, sizeof(database));
        strlcpy(mysql_pool->database, val, sizeof(mysql_pool->database));
      }
      /* Username configuration */
      else if (!str_cmp(key, "mysql_username")) {
        strlcpy(username, val, sizeof(username));
        strlcpy(mysql_pool->username, val, sizeof(mysql_pool->username));
      }
      /* Password configuration */
      else if (!str_cmp(key, "mysql_password")) {
        strlcpy(password, val, sizeof(password));
        strlcpy(mysql_pool->password, val, sizeof(mysql_pool->password));
      }
      
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

  /* Initialize the connection pool with configuration already loaded */
  mysql_pool_init();
  
  /* If pool initialization succeeded, set up legacy connections for compatibility */
  if (mysql_pool && mysql_pool->initialized) {
    /* Set up legacy connection pointers for backward compatibility */
    /* These point to the first three connections in the pool */
    if (mysql_pool->connections) {
      conn = mysql_pool->connections->conn;
      if (mysql_pool->connections->next) {
        conn2 = mysql_pool->connections->next->conn;
        if (mysql_pool->connections->next->next) {
          conn3 = mysql_pool->connections->next->next->conn;
        }
      }
    }
    
    /* Log successful connection - password is intentionally not logged for security */
    log("Success: Connected to MySQL database '%s' on host '%s' as user '%s'", 
        mysql_pool->database, mysql_pool->host, mysql_pool->username);
    log("Info: MySQL configuration loaded from lib/mysql_config");
    log("Info: Using connection pool with %d connections", mysql_pool->current_size);
  } else {
    log("WARNING: Failed to initialize MySQL connection pool");
    mysql_available = FALSE;
  }
}

void disconnect_from_mysql()
{
  /* Destroy the connection pool */
  if (mysql_pool) {
    mysql_pool_destroy();
  }
  
  /* Clear legacy pointers */
  conn = NULL;
  conn2 = NULL;
  conn3 = NULL;
}

void disconnect_from_mysql2()
{
  /* Legacy function - connections now managed by pool */
  /* conn2 is just a pointer into the pool, not owned separately */
  conn2 = NULL;
}

void disconnect_from_mysql3()
{
  /* Legacy function - connections now managed by pool */
  /* conn3 is just a pointer into the pool, not owned separately */
  conn3 = NULL;
}

/* Call this once at program termination */
void cleanup_mysql_library()
{
  /* Destroy connection pool */
  if (mysql_pool) {
    mysql_pool_destroy();
  }
  
  /* Clear legacy pointers */
  conn = NULL;
  conn2 = NULL;
  conn3 = NULL;
  
  /* End MySQL library */
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

/* ========================================================================== */
/* Prepared Statement Implementation for Enhanced SQL Security                */
/* ========================================================================== */
/* These functions provide a secure way to execute parameterized SQL queries,
 * completely preventing SQL injection attacks by separating SQL logic from data.
 * 
 * Prepared statements compile the SQL query once and execute it multiple times
 * with different parameters, providing both security and performance benefits.
 */

/**
 * Creates and initializes a new prepared statement structure.
 * 
 * @param mysql_conn The MySQL connection to use (conn, conn2, or conn3)
 * @return Pointer to new PREPARED_STMT structure, or NULL on error
 * 
 * @note The returned structure must be freed with mysql_stmt_cleanup()
 */
PREPARED_STMT *mysql_stmt_create(MYSQL *mysql_conn)
{
  PREPARED_STMT *pstmt;
  pthread_mutex_t *mutex;
  
  /* Validate connection */
  if (!mysql_conn || !mysql_available) {
    log("SYSERR: mysql_stmt_create called with invalid connection");
    return NULL;
  }
  
  /* Allocate prepared statement structure */
  CREATE(pstmt, PREPARED_STMT, 1);
  pstmt->connection = mysql_conn;
  pstmt->params = NULL;
  pstmt->results = NULL;
  pstmt->param_count = 0;
  pstmt->result_count = 0;
  pstmt->metadata = NULL;
  
  /* Select appropriate mutex based on connection */
  if (mysql_conn == conn)
    mutex = &mysql_mutex;
  else if (mysql_conn == conn2)
    mutex = &mysql_mutex2;
  else if (mysql_conn == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_stmt_create called with unknown connection");
    free(pstmt);
    return NULL;
  }
  
  /* Initialize MySQL statement handle */
  MYSQL_LOCK(*mutex);
  pstmt->stmt = mysql_stmt_init(mysql_conn);
  MYSQL_UNLOCK(*mutex);
  
  if (!pstmt->stmt) {
    log("SYSERR: mysql_stmt_init failed: %s", mysql_error(mysql_conn));
    free(pstmt);
    return NULL;
  }
  
  return pstmt;
}

/**
 * Prepares a SQL query with parameter placeholders for execution.
 * 
 * @param pstmt The prepared statement structure
 * @param query The SQL query with ? placeholders for parameters
 * @return TRUE on success, FALSE on error
 * 
 * @example
 *   mysql_stmt_prepare_query(pstmt, "SELECT * FROM help_entries WHERE tag = ? AND min_level <= ?");
 */
bool mysql_stmt_prepare_query(PREPARED_STMT *pstmt, const char *query)
{
  pthread_mutex_t *mutex;
  int i;
  
  if (!pstmt || !pstmt->stmt || !query) {
    log("SYSERR: mysql_stmt_prepare_query called with invalid parameters");
    return FALSE;
  }
  
  /* Select appropriate mutex */
  if (pstmt->connection == conn)
    mutex = &mysql_mutex;
  else if (pstmt->connection == conn2)
    mutex = &mysql_mutex2;
  else if (pstmt->connection == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_stmt_prepare_query called with unknown connection");
    return FALSE;
  }
  
  /* Prepare the statement */
  MYSQL_LOCK(*mutex);
  if (mysql_stmt_prepare(pstmt->stmt, query, strlen(query))) {
    log("SYSERR: mysql_stmt_prepare failed: %s (Error: %u)", 
        mysql_stmt_error(pstmt->stmt), mysql_stmt_errno(pstmt->stmt));
    log("  Query was: %.200s%s", query, strlen(query) > 200 ? "..." : "");
    MYSQL_UNLOCK(*mutex);
    return FALSE;
  }
  
  /* Get parameter count and allocate bindings */
  pstmt->param_count = mysql_stmt_param_count(pstmt->stmt);
  if (pstmt->param_count > 0) {
    CREATE(pstmt->params, MYSQL_BIND, pstmt->param_count);
    /* Initialize all parameter bindings to prevent undefined behavior */
    for (i = 0; i < pstmt->param_count; i++) {
      memset(&pstmt->params[i], 0, sizeof(MYSQL_BIND));
    }
  }
  
  /* Get result metadata and prepare result bindings */
  pstmt->metadata = mysql_stmt_result_metadata(pstmt->stmt);
  if (pstmt->metadata) {
    pstmt->result_count = mysql_num_fields(pstmt->metadata);
    CREATE(pstmt->results, MYSQL_BIND, pstmt->result_count);
    /* Initialize all result bindings */
    for (i = 0; i < pstmt->result_count; i++) {
      memset(&pstmt->results[i], 0, sizeof(MYSQL_BIND));
    }
  }
  
  MYSQL_UNLOCK(*mutex);
  return TRUE;
}

/**
 * Binds a string parameter to a prepared statement.
 * 
 * @param pstmt The prepared statement structure
 * @param param_index The parameter index (0-based)
 * @param value The string value to bind (can be NULL)
 * @return TRUE on success, FALSE on error
 * 
 * @note The string value is copied and managed internally
 */
bool mysql_stmt_bind_param_string(PREPARED_STMT *pstmt, int param_index, const char *value)
{
  unsigned long *length;
  char *buffer;
  my_bool *is_null;
  
  if (!pstmt || !pstmt->params || param_index < 0 || param_index >= pstmt->param_count) {
    log("SYSERR: mysql_stmt_bind_param_string called with invalid parameters");
    return FALSE;
  }
  
  /* Free any previously allocated buffer for this parameter */
  if (pstmt->params[param_index].buffer) {
    free(pstmt->params[param_index].buffer);
  }
  if (pstmt->params[param_index].length) {
    free(pstmt->params[param_index].length);
  }
  if (pstmt->params[param_index].is_null) {
    free(pstmt->params[param_index].is_null);
  }
  
  /* CRITICAL: Clear the entire binding structure to prevent garbage values */
  memset(&pstmt->params[param_index], 0, sizeof(MYSQL_BIND));
  
  /* Allocate and set up the binding */
  if (value) {
    /* Allocate buffer and copy string */
    buffer = strdup(value);
    CREATE(length, unsigned long, 1);
    *length = strlen(value);
    CREATE(is_null, my_bool, 1);
    *is_null = 0;
    
    pstmt->params[param_index].buffer_type = MYSQL_TYPE_STRING;
    pstmt->params[param_index].buffer = buffer;
    pstmt->params[param_index].buffer_length = *length + 1;
    pstmt->params[param_index].length = length;
    pstmt->params[param_index].is_null = is_null;
  } else {
    /* Handle NULL value */
    CREATE(is_null, my_bool, 1);
    *is_null = 1;
    
    pstmt->params[param_index].buffer_type = MYSQL_TYPE_NULL;
    pstmt->params[param_index].buffer = NULL;
    pstmt->params[param_index].is_null = is_null;
  }
  
  return TRUE;
}

/**
 * Binds an integer parameter to a prepared statement.
 * 
 * @param pstmt The prepared statement structure
 * @param param_index The parameter index (0-based)
 * @param value The integer value to bind
 * @return TRUE on success, FALSE on error
 */
bool mysql_stmt_bind_param_int(PREPARED_STMT *pstmt, int param_index, int value)
{
  int *buffer;
  my_bool *is_null;
  
  if (!pstmt || !pstmt->params || param_index < 0 || param_index >= pstmt->param_count) {
    log("SYSERR: mysql_stmt_bind_param_int called with invalid parameters");
    return FALSE;
  }
  
  /* Free any previously allocated buffer */
  if (pstmt->params[param_index].buffer) {
    free(pstmt->params[param_index].buffer);
  }
  if (pstmt->params[param_index].is_null) {
    free(pstmt->params[param_index].is_null);
  }
  
  /* CRITICAL: Clear the entire binding structure to prevent garbage values */
  memset(&pstmt->params[param_index], 0, sizeof(MYSQL_BIND));
  
  /* Allocate buffer for integer */
  CREATE(buffer, int, 1);
  *buffer = value;
  CREATE(is_null, my_bool, 1);
  *is_null = 0;
  
  pstmt->params[param_index].buffer_type = MYSQL_TYPE_LONG;
  pstmt->params[param_index].buffer = buffer;
  pstmt->params[param_index].buffer_length = sizeof(int);
  pstmt->params[param_index].is_null = is_null;
  pstmt->params[param_index].length = 0;
  
  return TRUE;
}

/**
 * Binds a long parameter to a prepared statement.
 * 
 * @param pstmt The prepared statement structure
 * @param param_index The parameter index (0-based)
 * @param value The long value to bind
 * @return TRUE on success, FALSE on error
 */
bool mysql_stmt_bind_param_long(PREPARED_STMT *pstmt, int param_index, long value)
{
  long *buffer;
  my_bool *is_null;
  
  if (!pstmt || !pstmt->params || param_index < 0 || param_index >= pstmt->param_count) {
    log("SYSERR: mysql_stmt_bind_param_long called with invalid parameters");
    return FALSE;
  }
  
  /* Free any previously allocated buffer */
  if (pstmt->params[param_index].buffer) {
    free(pstmt->params[param_index].buffer);
  }
  if (pstmt->params[param_index].is_null) {
    free(pstmt->params[param_index].is_null);
  }
  
  /* CRITICAL: Clear the entire binding structure to prevent garbage values */
  memset(&pstmt->params[param_index], 0, sizeof(MYSQL_BIND));
  
  /* Allocate buffer for long */
  CREATE(buffer, long, 1);
  *buffer = value;
  CREATE(is_null, my_bool, 1);
  *is_null = 0;
  
  pstmt->params[param_index].buffer_type = MYSQL_TYPE_LONGLONG;
  pstmt->params[param_index].buffer = buffer;
  pstmt->params[param_index].buffer_length = sizeof(long);
  pstmt->params[param_index].is_null = is_null;
  pstmt->params[param_index].length = 0;
  
  return TRUE;
}

/**
 * Executes a prepared statement with bound parameters.
 * 
 * @param pstmt The prepared statement structure
 * @return TRUE on success, FALSE on error
 * 
 * @note Parameters must be bound before calling this function
 */
bool mysql_stmt_execute_prepared(PREPARED_STMT *pstmt)
{
  pthread_mutex_t *mutex;
  int i;
  
  if (!pstmt || !pstmt->stmt) {
    log("SYSERR: mysql_stmt_execute_prepared called with invalid statement");
    return FALSE;
  }
  
  /* Check if MySQL is available */
  if (!mysql_available || !pstmt->connection) {
    log("SYSERR: mysql_stmt_execute_prepared called but MySQL not available");
    return FALSE;
  }
  
  /* Select appropriate mutex */
  if (pstmt->connection == conn)
    mutex = &mysql_mutex;
  else if (pstmt->connection == conn2)
    mutex = &mysql_mutex2;
  else if (pstmt->connection == conn3)
    mutex = &mysql_mutex3;
  else {
    log("SYSERR: mysql_stmt_execute_prepared called with unknown connection");
    return FALSE;
  }
  
  MYSQL_LOCK(*mutex);
  
  /* Bind parameters if any */
  if (pstmt->param_count > 0 && pstmt->params) {
    if (mysql_stmt_bind_param(pstmt->stmt, pstmt->params)) {
      log("SYSERR: mysql_stmt_bind_param failed: %s (Error: %u)", 
          mysql_stmt_error(pstmt->stmt), mysql_stmt_errno(pstmt->stmt));
      MYSQL_UNLOCK(*mutex);
      return FALSE;
    }
  }
  
  /* Execute the statement */
  if (mysql_stmt_execute(pstmt->stmt)) {
    log("SYSERR: mysql_stmt_execute failed: %s (Error: %u, SQLState: %s)", 
        mysql_stmt_error(pstmt->stmt), 
        mysql_stmt_errno(pstmt->stmt),
        mysql_stmt_sqlstate(pstmt->stmt));
    MYSQL_UNLOCK(*mutex);
    return FALSE;
  }
  
  /* For SELECT queries, prepare result bindings */
  if (pstmt->metadata && pstmt->result_count > 0) {
    MYSQL_FIELD *field;
    
    /* Set up result bindings based on field types */
    mysql_field_seek(pstmt->metadata, 0);
    for (i = 0; i < pstmt->result_count; i++) {
      field = mysql_fetch_field(pstmt->metadata);
      
      /* Free existing buffers if any (in case statement is being reused) */
      if (pstmt->results[i].buffer) {
        free(pstmt->results[i].buffer);
      }
      if (pstmt->results[i].length) {
        free(pstmt->results[i].length);
      }
      if (pstmt->results[i].is_null) {
        free(pstmt->results[i].is_null);
      }
      if (pstmt->results[i].error) {
        free(pstmt->results[i].error);
      }
      
      /* CRITICAL: Completely reinitialize the binding structure */
      memset(&pstmt->results[i], 0, sizeof(MYSQL_BIND));
      
      /* Allocate buffer based on field type */
      switch (field->type) {
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
          /* Allocate buffer for string data
           * Use larger buffer for GROUP_CONCAT and other potentially large results
           * field->length may be 0 or too small for aggregated results */
          {
            unsigned long buffer_size;
            
            /* Check if this looks like a GROUP_CONCAT result (huge field->length) 
             * GROUP_CONCAT max_length defaults to 67108864 (64MB) in MySQL
             * We don't need that much - cap at a reasonable size */
            if (field->length > 1000000) {
              /* GROUP_CONCAT or similar aggregate - use reasonable buffer */
              buffer_size = 65536; /* 64KB should be enough for any help keyword list */
            } else if (field->length > 0) {
              /* Use field length but ensure minimum size */
              buffer_size = field->length < 256 ? 256 : field->length;
            } else {
              /* Default for unknown length */
              buffer_size = 4096;
            }
            
            CREATE(pstmt->results[i].buffer, char, buffer_size + 1);
            pstmt->results[i].buffer_type = MYSQL_TYPE_STRING;
            pstmt->results[i].buffer_length = buffer_size + 1;
            CREATE(pstmt->results[i].length, unsigned long, 1);
            CREATE(pstmt->results[i].is_null, my_bool, 1);
            CREATE(pstmt->results[i].error, my_bool, 1);
            /* CRITICAL: Initialize the length value */
            *pstmt->results[i].length = 0;
            /* Initialize error flag */
            *pstmt->results[i].error = 0;
          }
          break;
          
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_TINY:
          /* Allocate buffer for integer data */
          CREATE(pstmt->results[i].buffer, int, 1);
          pstmt->results[i].buffer_type = MYSQL_TYPE_LONG;
          pstmt->results[i].buffer_length = sizeof(int);
          CREATE(pstmt->results[i].is_null, my_bool, 1);
          CREATE(pstmt->results[i].error, my_bool, 1);
          /* Initialize error flag */
          *pstmt->results[i].error = 0;
          /* Integer types don't need length pointer */
          pstmt->results[i].length = NULL;
          break;
          
        case MYSQL_TYPE_DATETIME:
        case MYSQL_TYPE_DATE:
        case MYSQL_TYPE_TIME:
        case MYSQL_TYPE_TIMESTAMP:
          /* Allocate buffer for datetime as string */
          CREATE(pstmt->results[i].buffer, char, 64);
          pstmt->results[i].buffer_type = MYSQL_TYPE_STRING;
          pstmt->results[i].buffer_length = 64;
          CREATE(pstmt->results[i].length, unsigned long, 1);
          CREATE(pstmt->results[i].is_null, my_bool, 1);
          CREATE(pstmt->results[i].error, my_bool, 1);
          /* Initialize values */
          *pstmt->results[i].length = 0;
          *pstmt->results[i].error = 0;
          break;
          
        default:
          /* Default to string for other types - use larger buffer */
          CREATE(pstmt->results[i].buffer, char, 4096);
          pstmt->results[i].buffer_type = MYSQL_TYPE_STRING;
          pstmt->results[i].buffer_length = 4096;
          CREATE(pstmt->results[i].length, unsigned long, 1);
          CREATE(pstmt->results[i].is_null, my_bool, 1);
          CREATE(pstmt->results[i].error, my_bool, 1);
          /* Initialize values */
          *pstmt->results[i].length = 0;
          *pstmt->results[i].error = 0;
          break;
      }
    }
    
    /* Bind the result buffers */
    if (mysql_stmt_bind_result(pstmt->stmt, pstmt->results)) {
      log("SYSERR: mysql_stmt_bind_result failed: %s (Error: %u)", 
          mysql_stmt_error(pstmt->stmt), mysql_stmt_errno(pstmt->stmt));
      /* Log buffer details for debugging */
      for (i = 0; i < pstmt->result_count; i++) {
        log("  Column %d: type=%d, buffer_length=%lu", 
            i, pstmt->results[i].buffer_type, pstmt->results[i].buffer_length);
      }
      MYSQL_UNLOCK(*mutex);
      return FALSE;
    }
    
    /* Store result set for SELECT queries */
    if (mysql_stmt_store_result(pstmt->stmt)) {
      log("SYSERR: mysql_stmt_store_result failed: %s (Error: %u)", 
          mysql_stmt_error(pstmt->stmt), mysql_stmt_errno(pstmt->stmt));
      MYSQL_UNLOCK(*mutex);
      return FALSE;
    }
  }
  
  MYSQL_UNLOCK(*mutex);
  return TRUE;
}

/**
 * Fetches the next row from a prepared statement result set.
 * 
 * @param pstmt The prepared statement structure
 * @return TRUE if a row was fetched, FALSE if no more rows or error
 */
bool mysql_stmt_fetch_row(PREPARED_STMT *pstmt)
{
  pthread_mutex_t *mutex;
  int result;
  
  if (!pstmt || !pstmt->stmt) {
    return FALSE;
  }
  
  /* Select appropriate mutex */
  if (pstmt->connection == conn)
    mutex = &mysql_mutex;
  else if (pstmt->connection == conn2)
    mutex = &mysql_mutex2;
  else if (pstmt->connection == conn3)
    mutex = &mysql_mutex3;
  else {
    return FALSE;
  }
  
  MYSQL_LOCK(*mutex);
  result = mysql_stmt_fetch(pstmt->stmt);
  if (MYSQL_DEBUG) {
    if (result == 0) {
      log("DEBUG: mysql_stmt_fetch successfully fetched a row");
    } else if (result == MYSQL_NO_DATA) {
      log("DEBUG: mysql_stmt_fetch returned MYSQL_NO_DATA (no more rows)");
    } else if (result == MYSQL_DATA_TRUNCATED) {
      int i;
      log("DEBUG: mysql_stmt_fetch returned MYSQL_DATA_TRUNCATED - checking which columns truncated");
      /* Check which columns were truncated */
      for (i = 0; i < pstmt->result_count; i++) {
        if (pstmt->results[i].error && *pstmt->results[i].error) {
          log("DEBUG:   Column %d was truncated (buffer_length=%lu, actual_length=%lu)", 
              i, pstmt->results[i].buffer_length, 
              pstmt->results[i].length ? *pstmt->results[i].length : 0);
        }
      }
    } else {
      log("DEBUG: mysql_stmt_fetch returned error %d: %s", 
          result, mysql_stmt_error(pstmt->stmt));
    }
  }
  MYSQL_UNLOCK(*mutex);
  
  /* mysql_stmt_fetch returns 0 on success, MYSQL_NO_DATA when no more rows
   * MYSQL_DATA_TRUNCATED is also a successful fetch - data is usable even if truncated */
  return (result == 0 || result == MYSQL_DATA_TRUNCATED);
}

/**
 * Gets a string value from the current result row.
 * 
 * @param pstmt The prepared statement structure
 * @param col_index The column index (0-based)
 * @return String value (do not free), or NULL if column is NULL
 */
char *mysql_stmt_get_string(PREPARED_STMT *pstmt, int col_index)
{
  if (!pstmt || !pstmt->results || col_index < 0 || col_index >= pstmt->result_count) {
    return NULL;
  }
  
  if (*pstmt->results[col_index].is_null) {
    return NULL;
  }
  
  return (char *)pstmt->results[col_index].buffer;
}

/**
 * Gets an integer value from the current result row.
 * 
 * @param pstmt The prepared statement structure
 * @param col_index The column index (0-based)
 * @return Integer value, or 0 if column is NULL or error
 */
int mysql_stmt_get_int(PREPARED_STMT *pstmt, int col_index)
{
  if (!pstmt || !pstmt->results || col_index < 0 || col_index >= pstmt->result_count) {
    return 0;
  }
  
  if (*pstmt->results[col_index].is_null) {
    return 0;
  }
  
  return *(int *)pstmt->results[col_index].buffer;
}

/**
 * Gets the number of rows affected by the last INSERT, UPDATE, or DELETE.
 * 
 * @param pstmt The prepared statement structure
 * @return Number of affected rows, or 0 on error
 */
my_ulonglong mysql_stmt_affected_rows_count(PREPARED_STMT *pstmt)
{
  if (!pstmt || !pstmt->stmt) {
    return 0;
  }
  
  return mysql_stmt_affected_rows(pstmt->stmt);
}

/**
 * Cleans up and frees a prepared statement structure.
 * 
 * @param pstmt The prepared statement structure to free
 * 
 * @note This function frees all associated memory and closes the statement
 */
void mysql_stmt_cleanup(PREPARED_STMT *pstmt)
{
  pthread_mutex_t *mutex;
  int i;
  
  if (!pstmt) {
    return;
  }
  
  /* Select appropriate mutex */
  if (pstmt->connection == conn)
    mutex = &mysql_mutex;
  else if (pstmt->connection == conn2)
    mutex = &mysql_mutex2;
  else if (pstmt->connection == conn3)
    mutex = &mysql_mutex3;
  else {
    mutex = NULL;
  }
  
  /* Free parameter buffers */
  if (pstmt->params) {
    for (i = 0; i < pstmt->param_count; i++) {
      if (pstmt->params[i].buffer) {
        free(pstmt->params[i].buffer);
      }
      if (pstmt->params[i].length) {
        free(pstmt->params[i].length);
      }
      if (pstmt->params[i].is_null) {
        free(pstmt->params[i].is_null);
      }
    }
    free(pstmt->params);
  }
  
  /* Free result buffers */
  if (pstmt->results) {
    for (i = 0; i < pstmt->result_count; i++) {
      if (pstmt->results[i].buffer) {
        free(pstmt->results[i].buffer);
      }
      if (pstmt->results[i].length) {
        free(pstmt->results[i].length);
      }
      if (pstmt->results[i].is_null) {
        free(pstmt->results[i].is_null);
      }
      if (pstmt->results[i].error) {
        free(pstmt->results[i].error);
      }
    }
    free(pstmt->results);
  }
  
  /* Free metadata */
  if (pstmt->metadata) {
    mysql_free_result(pstmt->metadata);
  }
  
  /* Close the statement */
  if (pstmt->stmt) {
    if (mutex) {
      MYSQL_LOCK(*mutex);
      mysql_stmt_close(pstmt->stmt);
      MYSQL_UNLOCK(*mutex);
    } else {
      mysql_stmt_close(pstmt->stmt);
    }
  }
  
  /* Free the structure itself */
  free(pstmt);
}

/* Debug function to help diagnose prepared statement issues */
void debug_prepared_stmt(PREPARED_STMT *pstmt) {
  if (!pstmt) {
    log("DEBUG: PREPARED_STMT is NULL");
    return;
  }
  
  log("=== PREPARED STATEMENT DEBUG ===");
  log("  stmt: %p", pstmt->stmt);
  log("  connection: %p", pstmt->connection);
  log("  param_count: %d", pstmt->param_count);
  log("  result_count: %d", pstmt->result_count);
  log("  metadata: %p", pstmt->metadata);
  
  if (pstmt->stmt) {
    log("  stmt_errno: %u", mysql_stmt_errno(pstmt->stmt));
    log("  stmt_error: %s", mysql_stmt_error(pstmt->stmt));
    log("  stmt_sqlstate: %s", mysql_stmt_sqlstate(pstmt->stmt));
    log("  stmt_field_count: %u", mysql_stmt_field_count(pstmt->stmt));
    log("  stmt_param_count: %lu", mysql_stmt_param_count(pstmt->stmt));
    log("  stmt_num_rows: %llu", mysql_stmt_num_rows(pstmt->stmt));
  }
}

/* Test function for direct query execution to compare with prepared statements */
void test_direct_query(const char *query) {
  MYSQL_RES *result;
  MYSQL_ROW row;
  unsigned int num_fields;
  unsigned int i;
  
  if (!conn || !query) {
    log("ERROR: test_direct_query called with invalid parameters");
    return;
  }
  
  log("=== DIRECT QUERY TEST ===");
  log("Query: %s", query);
  
  if (mysql_query(conn, query)) {
    log("ERROR: %s", mysql_error(conn));
    return;
  }
  
  result = mysql_store_result(conn);
  if (!result) {
    log("No result set or error: %s", mysql_error(conn));
    return;
  }
  
  num_fields = mysql_num_fields(result);
  log("Fields: %u, Rows: %llu", num_fields, mysql_num_rows(result));
  
  while ((row = mysql_fetch_row(result))) {
    for (i = 0; i < num_fields; i++) {
      log("  [%u]: %s", i, row[i] ? row[i] : "NULL");
    }
  }
  
  mysql_free_result(result);
}

/* Load the wilderness data for the specified zone. */
struct wilderness_data *load_wilderness(zone_vnum zone)
{

  MYSQL_RES *result;
  MYSQL_ROW row;
  char buf[1024];

  struct wilderness_data *wild = NULL;

  log("Info: Loading wilderness data for zone: %d", zone);

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
    log("Info: Skipping region loading - MySQL not available.");
    return;
  }
  char **it;     /* Token iterator */

  log("Info: Loading region data from MySQL");

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
      log("Info: Region %d (%s) has no polygon data", region_table[i].vnum, region_table[i].name);
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
    log("Info: Loaded %d regions, top_of_region_table set to %d", i, top_of_region_table);
    
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
    log("Info: No regions loaded, top_of_region_table set to -1");
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
  char buf[8192];  /* Increased buffer size for large query */

  /* Polygon-based approach: Check which directional sectors each region intersects */
  /* Uses simpler logic to avoid geometry type issues */
  /* Use non-overlapping rectangular sectors to avoid invalid geometry issues */
  snprintf(buf, sizeof(buf), 
           "SELECT ri.vnum, "
           "       rd.name, "
           "       CASE "
           "         WHEN ST_Within(ST_GeomFromText('POINT(%d %d)'), ri.region_polygon) THEN 'INSIDE' "
           "         WHEN ST_Distance(ri.region_polygon, ST_GeomFromText('POINT(%d %d)')) = 0 THEN 'EDGE' "
           "         ELSE 'NEARBY' "
           "       END AS position, "
           "       ST_Distance(ri.region_polygon, ST_GeomFromText('POINT(%d %d)')) AS distance, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS n, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS ne, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS e, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS se, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS s, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS sw, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS w, "
           "       CASE WHEN ST_Intersects(ri.region_polygon, ST_GeomFromText('POLYGON((%d %d, %d %d, %d %d, %d %d, %d %d))')) THEN 1 ELSE 0 END AS nw "
           "FROM region_index ri "
           "JOIN region_data rd ON ri.vnum = rd.vnum "
           "WHERE rd.region_type = 1 "
           "  AND (ST_Within(ST_GeomFromText('POINT(%d %d)'), ri.region_polygon) "
           "       OR ST_Distance(ri.region_polygon, ST_GeomFromText('POINT(%d %d)')) <= %d) "
           "ORDER BY distance ASC",
           x, y, x, y, x, y,
           /* N: Rectangle directly north */
           x-1, y+1, x+1, y+1, x+1, y+r, x-1, y+r, x-1, y+1,
           /* NE: Rectangle northeast */
           x+1, y+1, x+r, y+1, x+r, y+r, x+1, y+r, x+1, y+1,
           /* E: Rectangle directly east */
           x+1, y-1, x+r, y-1, x+r, y+1, x+1, y+1, x+1, y-1,
           /* SE: Rectangle southeast */
           x+1, y-r, x+r, y-r, x+r, y-1, x+1, y-1, x+1, y-r,
           /* S: Rectangle directly south */
           x-1, y-r, x+1, y-r, x+1, y-1, x-1, y-1, x-1, y-r,
           /* SW: Rectangle southwest */
           x-r, y-r, x-1, y-r, x-1, y-1, x-r, y-1, x-r, y-r,
           /* W: Rectangle directly west */
           x-r, y-1, x-1, y-1, x-1, y+1, x-r, y+1, x-r, y-1,
           /* NW: Rectangle northwest */
           x-r, y+1, x-1, y+1, x-1, y+r, x-r, y+r, x-r, y+1,
           /* WHERE clause coordinates */
           x, y, x, y, r);

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
    new_node->dist = atof(row[3]);  /* Distance from query */

    /* Check position - inside, edge, or nearby the region */
    bool is_inside = (strcmp(row[2], "INSIDE") == 0);
    bool is_edge = (strcmp(row[2], "EDGE") == 0);
    new_node->is_inside = is_inside || is_edge;  /* Treat edge as "inside" for positioning */
    
    /* Set the position constant for room descriptions */
    if (is_inside) {
      new_node->pos = REGION_POS_INSIDE;
    } else if (is_edge) {
      new_node->pos = REGION_POS_EDGE;
    } else {
      new_node->pos = REGION_POS_UNDEFINED;  /* This is for nearby regions */
    }
    
    if (is_inside || is_edge) {
      /* When inside or on edge of a region, set all directions to indicate we're surrounded */
      for (i = 0; i < 8; i++) {
        new_node->dirs[i] = 100.0;  /* Maximum strength for all directions */
      }
    } else {
      /* Process the directional flags from the query (rows 4-11: n, ne, e, se, s, sw, w, nw) */
      for (i = 0; i < 8; i++) {
        int intersects = atoi(row[i + 4]);  /* 1 if region intersects this direction, 0 if not */
        if (intersects) {
          /* Use inverse distance as strength - closer regions have higher influence */
          double strength = (new_node->dist > 0) ? (100.0 / (1.0 + new_node->dist)) : 100.0;
          new_node->dirs[i] = strength;
        } else {
          new_node->dirs[i] = 0.0;
        }
      }
    }

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

  log("Info: Loading wilderness roads and path definitions from MySQL database");
  
  if (!mysql_available) {
    log("Info: Skipping path loading - MySQL not available.");
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
      log("Info: Path %d (%s) has no polygon data", path_table[i].vnum, path_table[i].name);
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

  log("Info: Inserting Path [%d] '%s' into MySQL:", (int)path->vnum, path->name);
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

  log("Info: Deleting Path [%d] from MySQL:", (int)vnum);
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
    if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_ANON))
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
