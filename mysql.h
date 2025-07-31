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
void load_paths();
struct path_list *get_enclosing_paths(zone_rnum zone, int x, int y);
bool get_random_region_location(region_vnum region, int *x, int *y);
struct region_proximity_list *get_nearby_regions(zone_rnum zone, int x, int y, int r);
char **tokenize(const char *input, const char *delim);
void free_tokens(char **tokens);

#endif
