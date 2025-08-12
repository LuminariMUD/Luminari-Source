/********************************************************************
 * Name:   mysql.h 
 * Author: Ornir (James McLaughlin)
 *
 * MySQL Header file for Luminari MUD.
 ********************************************************************/

#ifndef _MYSQL_H
#define _MYSQL_H

#include <mysql/mysql.h> /* System headerfile for mysql. */
#include <pthread.h>

extern MYSQL *conn;

/* Global flag to track MySQL availability */
extern bool mysql_available;

/* MySQL connection mutexes for thread safety */
extern pthread_mutex_t mysql_mutex;
extern pthread_mutex_t mysql_mutex2;
extern pthread_mutex_t mysql_mutex3;

/* Macros for safe MySQL operations */
#define MYSQL_LOCK(mutex)   pthread_mutex_lock(&(mutex))
#define MYSQL_UNLOCK(mutex) pthread_mutex_unlock(&(mutex))

/* Thread-safe MySQL query wrappers */
int mysql_query_safe(MYSQL *mysql_conn, const char *query);
MYSQL_RES *mysql_store_result_safe(MYSQL *mysql_conn);

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

void connect_to_mysql();
void disconnect_from_mysql();
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
