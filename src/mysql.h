/********************************************************************
 * Name:   mysql.h 
 * Author: Ornir (James McLaughlin)
 *
 * MySQL Header file for Luminari MUD.
 ********************************************************************/

#ifndef _MYSQL_H
#define _MYSQL_H

#include <mariadb/mysql.h> /* System headerfile for MariaDB/MySQL. */
#include <pthread.h>
#include <time.h>

/* Connection Pool Configuration */
#define MYSQL_POOL_MIN_SIZE 3      /* Minimum connections in pool */
#define MYSQL_POOL_MAX_SIZE 10     /* Maximum connections in pool */
#define MYSQL_POOL_TIMEOUT 30      /* Seconds before idle connection closes */
#define MYSQL_HEALTH_CHECK_INTERVAL 60  /* Seconds between health checks */

/* Connection states for pool management */
enum mysql_conn_state {
  CONN_STATE_FREE = 0,      /* Connection available for use */
  CONN_STATE_IN_USE,        /* Connection currently in use */
  CONN_STATE_STALE,         /* Connection needs refresh */
  CONN_STATE_ERROR          /* Connection has error */
};

/* MySQL Connection Pool Entry */
typedef struct mysql_pool_conn {
  MYSQL *conn;                      /* MySQL connection handle */
  enum mysql_conn_state state;      /* Current state of connection */
  time_t last_used;                 /* Last time connection was used */
  time_t created;                   /* When connection was created */
  pthread_mutex_t mutex;             /* Per-connection mutex */
  int id;                           /* Connection ID for debugging */
  unsigned long thread_id;          /* MySQL thread ID */
  struct mysql_pool_conn *next;     /* Next in linked list */
} MYSQL_POOL_CONN;

/* MySQL Connection Pool Manager */
typedef struct mysql_pool {
  MYSQL_POOL_CONN *connections;     /* Linked list of connections */
  int current_size;                 /* Current number of connections */
  int active_count;                 /* Number of connections in use */
  pthread_mutex_t pool_mutex;       /* Mutex for pool operations */
  pthread_cond_t pool_cond;         /* Condition variable for waiting */
  bool initialized;                 /* Pool initialization status */
  
  /* Configuration loaded from mysql_config */
  char host[128];
  char database[128];
  char username[128];
  char password[128];
  
  /* Statistics for monitoring */
  unsigned long total_requests;     /* Total connection requests */
  unsigned long wait_count;         /* Times had to wait for connection */
  unsigned long error_count;        /* Connection errors */
  time_t last_health_check;        /* Last health check time */
} MYSQL_POOL;

/* Global connection pool */
extern MYSQL_POOL *mysql_pool;

/* Legacy compatibility - these will be removed in future */
extern MYSQL *conn;

/* Global flag to track MySQL availability */
extern bool mysql_available;

/* MySQL connection mutexes for thread safety (legacy - use pool instead) */
extern pthread_mutex_t mysql_mutex;
extern pthread_mutex_t mysql_mutex2;
extern pthread_mutex_t mysql_mutex3;

/* Macros for safe MySQL operations */
#define MYSQL_LOCK(mutex)   pthread_mutex_lock(&(mutex))
#define MYSQL_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))

/* Connection Pool Management Functions */
void mysql_pool_init(void);                    /* Initialize the connection pool */
void mysql_pool_destroy(void);                 /* Destroy the pool and all connections */
MYSQL_POOL_CONN *mysql_pool_acquire(void);     /* Get a connection from the pool */
void mysql_pool_release(MYSQL_POOL_CONN *pc);  /* Return a connection to the pool */
void mysql_pool_health_check(void);            /* Check health of all connections */
void mysql_pool_expand(void);                  /* Add more connections if needed */
void mysql_pool_shrink(void);                  /* Remove idle connections */
void mysql_pool_stats(char *buf, size_t size); /* Get pool statistics */

/* Thread-safe MySQL query wrappers */
int mysql_query_safe(MYSQL *mysql_conn, const char *query);
MYSQL_RES *mysql_store_result_safe(MYSQL *mysql_conn);

/* Pool-aware query functions - automatically acquire/release connections */
int mysql_pool_query(const char *query, MYSQL_RES **result);
void mysql_pool_free_result(MYSQL_RES *result);

/* String escaping for SQL injection prevention */
char *mysql_escape_string_alloc(MYSQL *mysql_conn, const char *str);

/* Prepared Statement Support for Enhanced Security */
/* These functions provide a secure way to execute parameterized SQL queries,
 * completely preventing SQL injection attacks by separating SQL logic from data.
 * 
 * Usage pattern:
 *   1. Create statement: mysql_stmt_create()
 *   2. Prepare query: mysql_stmt_prepare_query()
 *   3. Bind parameters: mysql_stmt_bind_param_*() functions
 *   4. Execute: mysql_stmt_execute_prepared()
 *   5. Fetch results: mysql_stmt_fetch_row() in loop
 *   6. Clean up: mysql_stmt_cleanup()
 */

/* Prepared statement wrapper structure */
typedef struct prepared_stmt {
  MYSQL_STMT *stmt;           /* MySQL statement handle */
  MYSQL_BIND *params;         /* Parameter bindings */
  MYSQL_BIND *results;        /* Result bindings */
  int param_count;            /* Number of parameters */
  int result_count;           /* Number of result columns */
  MYSQL_RES *metadata;        /* Result metadata */
  MYSQL *connection;          /* Associated connection */
} PREPARED_STMT;

/* Create and initialize a prepared statement */
PREPARED_STMT *mysql_stmt_create(MYSQL *mysql_conn);

/* Prepare a SQL query with parameter placeholders (?) */
bool mysql_stmt_prepare_query(PREPARED_STMT *pstmt, const char *query);

/* Bind parameters - different types */
bool mysql_stmt_bind_param_string(PREPARED_STMT *pstmt, int param_index, const char *value);
bool mysql_stmt_bind_param_int(PREPARED_STMT *pstmt, int param_index, int value);
bool mysql_stmt_bind_param_long(PREPARED_STMT *pstmt, int param_index, long value);

/* Execute the prepared statement */
bool mysql_stmt_execute_prepared(PREPARED_STMT *pstmt);

/* Fetch results */
bool mysql_stmt_fetch_row(PREPARED_STMT *pstmt);
char *mysql_stmt_get_string(PREPARED_STMT *pstmt, int col_index);
int mysql_stmt_get_int(PREPARED_STMT *pstmt, int col_index);

/* Get number of affected rows (for INSERT/UPDATE/DELETE) */
my_ulonglong mysql_stmt_affected_rows_count(PREPARED_STMT *pstmt);

/* Clean up and free the prepared statement */
void mysql_stmt_cleanup(PREPARED_STMT *pstmt);

/* Debug functions for troubleshooting */
void debug_prepared_stmt(PREPARED_STMT *pstmt);
void test_direct_query(const char *query);

/* MySQL Error Handling Policy:
 * - During startup/data loading: exit(1) on critical errors
 * - During runtime operations: log error and return failure
 * - Always log errors with mysql_error() for debugging
 */

/* Error handling macros */
#define MYSQL_ERROR_CRITICAL(conn, msg) do { \
  log("SYSERR: %s: %s", msg, mysql_error(conn)); \
  exit(1); \
} while(0)

#define MYSQL_ERROR_RUNTIME(conn, msg) do { \
  log("SYSERR: %s: %s", msg, mysql_error(conn)); \
} while(0)

void connect_to_mysql();     /* Legacy - initializes pool now */
void disconnect_from_mysql(); /* Legacy - destroys pool now */
void disconnect_from_mysql2();
void disconnect_from_mysql3();
void cleanup_mysql_library();

/* Wilderness */
struct wilderness_data *load_wilderness(zone_vnum zone);
void load_regions();
struct region_list *get_enclosing_regions(zone_rnum zone, int x, int y);
void free_region_list(struct region_list *regions);
void load_paths();
struct path_list *get_enclosing_paths(zone_rnum zone, int x, int y);
void free_path_list(struct path_list *paths);
bool get_random_region_location(region_vnum region, int *x, int *y);
struct region_proximity_list *get_nearby_regions(zone_rnum zone, int x, int y, int r);
char **tokenize(const char *input, const char *delim);
void free_tokens(char **tokens);

#endif
