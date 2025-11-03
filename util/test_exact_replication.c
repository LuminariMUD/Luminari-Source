/* Exact replication of MUD's mysql.c code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



#define CREATE(result, type, number) \
  do { \
    if (!((result) = (type *)calloc((number), sizeof(type)))) { \
      perror("malloc failure"); \
      abort(); \
    } \
  } while(0)

typedef struct {
    MYSQL_STMT *stmt;
    MYSQL *connection;
    MYSQL_BIND *params;
    MYSQL_BIND *results;
    int param_count;
    int result_count;
    MYSQL_RES *metadata;
} PREPARED_STMT;

PREPARED_STMT *mysql_stmt_create(MYSQL *conn) {
    PREPARED_STMT *pstmt;
    CREATE(pstmt, PREPARED_STMT, 1);
    pstmt->stmt = mysql_stmt_init(conn);
    pstmt->connection = conn;
    return pstmt;
}

bool mysql_stmt_prepare_query(PREPARED_STMT *pstmt, const char *query) {
    int i;
    
    if (mysql_stmt_prepare(pstmt->stmt, query, strlen(query))) {
        printf("ERROR: mysql_stmt_prepare failed: %s\n", mysql_stmt_error(pstmt->stmt));
        return false;
    }
    
    /* Get parameter count and allocate bindings */
    pstmt->param_count = mysql_stmt_param_count(pstmt->stmt);
    if (pstmt->param_count > 0) {
        CREATE(pstmt->params, MYSQL_BIND, pstmt->param_count);
        /* Initialize all parameter bindings */
        for (i = 0; i < pstmt->param_count; i++) {
            memset(&pstmt->params[i], 0, sizeof(MYSQL_BIND));
        }
    }
    
    /* Get result metadata */
    pstmt->metadata = mysql_stmt_result_metadata(pstmt->stmt);
    if (pstmt->metadata) {
        pstmt->result_count = mysql_num_fields(pstmt->metadata);
        CREATE(pstmt->results, MYSQL_BIND, pstmt->result_count);
        for (i = 0; i < pstmt->result_count; i++) {
            memset(&pstmt->results[i], 0, sizeof(MYSQL_BIND));
        }
    }
    
    return true;
}

bool mysql_stmt_bind_param_string(PREPARED_STMT *pstmt, int param_index, const char *value) {
    unsigned long *length;
    char *buffer;
    my_bool *is_null;
    
    /* Free any previously allocated buffer */
    if (pstmt->params[param_index].buffer) {
        free(pstmt->params[param_index].buffer);
    }
    if (pstmt->params[param_index].length) {
        free(pstmt->params[param_index].length);
    }
    if (pstmt->params[param_index].is_null) {
        free(pstmt->params[param_index].is_null);
    }
    
    /* CRITICAL: Clear the entire binding structure */
    memset(&pstmt->params[param_index], 0, sizeof(MYSQL_BIND));
    
    /* Allocate and set up the binding */
    if (value) {
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
    }
    
    return true;
}

bool mysql_stmt_bind_param_int(PREPARED_STMT *pstmt, int param_index, int value) {
    int *buffer;
    my_bool *is_null;
    
    /* Free any previously allocated buffer */
    if (pstmt->params[param_index].buffer) {
        free(pstmt->params[param_index].buffer);
    }
    if (pstmt->params[param_index].is_null) {
        free(pstmt->params[param_index].is_null);
    }
    
    /* CRITICAL: Clear the entire binding structure */
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
    
    return true;
}

bool mysql_stmt_execute_prepared(PREPARED_STMT *pstmt) {
    /* Bind parameters if any */
    if (pstmt->param_count > 0 && pstmt->params) {
        if (mysql_stmt_bind_param(pstmt->stmt, pstmt->params)) {
            printf("ERROR: mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(pstmt->stmt));
            return false;
        }
    }
    
    /* Execute the statement */
    if (mysql_stmt_execute(pstmt->stmt)) {
        printf("ERROR: mysql_stmt_execute failed: %s\n", mysql_stmt_error(pstmt->stmt));
        return false;
    }
    
    /* For SELECT queries, store result */
    if (pstmt->metadata) {
        if (mysql_stmt_store_result(pstmt->stmt)) {
            printf("ERROR: mysql_stmt_store_result failed: %s\n", mysql_stmt_error(pstmt->stmt));
            return false;
        }
        printf("Stored %lu rows\n", (unsigned long)mysql_stmt_num_rows(pstmt->stmt));
    }
    
    return true;
}

void mysql_stmt_cleanup(PREPARED_STMT *pstmt) {
    int i;
    
    if (!pstmt) return;
    
    if (pstmt->stmt) {
        mysql_stmt_close(pstmt->stmt);
    }
    
    /* Free parameter bindings */
    if (pstmt->params) {
        for (i = 0; i < pstmt->param_count; i++) {
            if (pstmt->params[i].buffer) free(pstmt->params[i].buffer);
            if (pstmt->params[i].length) free(pstmt->params[i].length);
            if (pstmt->params[i].is_null) free(pstmt->params[i].is_null);
        }
        free(pstmt->params);
    }
    
    /* Free result bindings */
    if (pstmt->results) {
        for (i = 0; i < pstmt->result_count; i++) {
            if (pstmt->results[i].buffer) free(pstmt->results[i].buffer);
            if (pstmt->results[i].length) free(pstmt->results[i].length);
            if (pstmt->results[i].is_null) free(pstmt->results[i].is_null);
            if (pstmt->results[i].error) free(pstmt->results[i].error);
        }
        free(pstmt->results);
    }
    
    if (pstmt->metadata) {
        mysql_free_result(pstmt->metadata);
    }
    
    free(pstmt);
}

void test_exact_mud_implementation(MYSQL *conn) {
    PREPARED_STMT *pstmt;
    char search_pattern[256];
    int level = 34;
    
    printf("\n=== TESTING EXACT MUD IMPLEMENTATION ===\n");
    
    /* Create prepared statement */
    pstmt = mysql_stmt_create(conn);
    if (!pstmt) {
        printf("Failed to create prepared statement\n");
        return;
    }
    
    /* The exact query from help.c with LOWER(?) */
    if (!mysql_stmt_prepare_query(pstmt,
        "SELECT DISTINCT he.tag, he.entry, he.min_level, he.last_updated, "
        "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ') "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ? "
        "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
        "ORDER BY LENGTH(hk.keyword) ASC")) {
        printf("Failed to prepare query\n");
        mysql_stmt_cleanup(pstmt);
        return;
    }
    
    /* Build search pattern exactly like help.c does */
    snprintf(search_pattern, sizeof(search_pattern), "%s%%", "score");
    printf("Search pattern: '%s'\n", search_pattern);
    
    /* Bind parameters exactly like help.c does */
    if (!mysql_stmt_bind_param_string(pstmt, 0, search_pattern) ||
        !mysql_stmt_bind_param_int(pstmt, 1, level)) {
        printf("Failed to bind parameters\n");
        mysql_stmt_cleanup(pstmt);
        return;
    }
    
    /* Execute the prepared statement */
    if (!mysql_stmt_execute_prepared(pstmt)) {
        printf("Failed to execute query\n");
        mysql_stmt_cleanup(pstmt);
        return;
    }
    
    printf("Success! Query executed.\n");
    
    /* Clean up */
    mysql_stmt_cleanup(pstmt);
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        return 1;
    }
    
    printf("Testing Exact MUD mysql.c Implementation\n");
    printf("=========================================\n");
    
    test_exact_mud_implementation(conn);
    
    mysql_close(conn);
    
    return 0;
}