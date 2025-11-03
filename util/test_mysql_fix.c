/* Test the actual fix needed */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



void test_proper_binding(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    char pattern[256] = "score%";
    int level = 34;
    unsigned long pattern_length;
    
    printf("\n=== TESTING WITH PROPER BINDING ===\n");
    
    const char *query = 
        "SELECT COUNT(*) "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ?";
    
    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("ERROR: Failed to prepare: %s\n", mysql_stmt_error(stmt));
        return;
    }
    
    /* PROPERLY clear the entire array */
    memset(params, 0, sizeof(params));
    
    /* Set up string parameter PROPERLY */
    pattern_length = strlen(pattern);
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char *)pattern;
    params[0].buffer_length = sizeof(pattern);  /* Max buffer size */
    params[0].length = &pattern_length;         /* Actual data length */
    params[0].is_null = 0;
    
    /* Set up int parameter */
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = (char *)&level;
    params[1].is_null = 0;
    params[1].length = 0;
    
    printf("Pattern: '%s' (length=%lu)\n", pattern, pattern_length);
    
    if (mysql_stmt_bind_param(stmt, params)) {
        printf("ERROR: Bind failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    if (mysql_stmt_execute(stmt)) {
        printf("ERROR: Execute failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    /* Set up result binding for COUNT(*) */
    MYSQL_BIND result;
    int count;
    my_bool count_is_null;
    
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &count;
    result.is_null = &count_is_null;
    
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);
    mysql_stmt_fetch(stmt);
    
    printf("Result: %d rows\n", count);
    
    mysql_stmt_close(stmt);
}

void test_mud_style_binding(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND *params;
    char *pattern_buffer;
    int *level_buffer;
    unsigned long *pattern_length;
    my_bool *pattern_is_null;
    my_bool *level_is_null;
    
    printf("\n=== TESTING MUD-STYLE DYNAMIC BINDING ===\n");
    
    const char *query = 
        "SELECT COUNT(*) "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ?";
    
    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("ERROR: Failed to prepare: %s\n", mysql_stmt_error(stmt));
        return;
    }
    
    /* Allocate params array like MUD does */
    params = (MYSQL_BIND *)calloc(2, sizeof(MYSQL_BIND));
    
    /* Set up string parameter like MUD's mysql_stmt_bind_param_string */
    pattern_buffer = strdup("score%");
    pattern_length = (unsigned long *)malloc(sizeof(unsigned long));
    *pattern_length = strlen(pattern_buffer);
    pattern_is_null = (my_bool *)malloc(sizeof(my_bool));
    *pattern_is_null = 0;
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = pattern_buffer;
    params[0].buffer_length = *pattern_length + 1;  /* THIS IS THE BUG! */
    params[0].length = pattern_length;
    params[0].is_null = pattern_is_null;
    
    /* Set up int parameter like MUD's mysql_stmt_bind_param_int */
    level_buffer = (int *)malloc(sizeof(int));
    *level_buffer = 34;
    level_is_null = (my_bool *)malloc(sizeof(my_bool));
    *level_is_null = 0;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = level_buffer;
    params[1].buffer_length = sizeof(int);
    params[1].is_null = level_is_null;
    params[1].length = 0;  /* NULL for integers */
    
    printf("Pattern: '%s' (buffer_length=%lu, *length=%lu)\n", 
           pattern_buffer, params[0].buffer_length, *pattern_length);
    
    if (mysql_stmt_bind_param(stmt, params)) {
        printf("ERROR: Bind failed: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }
    
    if (mysql_stmt_execute(stmt)) {
        printf("ERROR: Execute failed: %s\n", mysql_stmt_error(stmt));
        goto cleanup;
    }
    
    /* Set up result */
    MYSQL_BIND result;
    int count;
    my_bool count_is_null;
    
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &count;
    result.is_null = &count_is_null;
    
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);
    mysql_stmt_fetch(stmt);
    
    printf("Result: %d rows (SHOULD BE 1!)\n", count);
    
cleanup:
    free(pattern_buffer);
    free(pattern_length);
    free(pattern_is_null);
    free(level_buffer);
    free(level_is_null);
    free(params);
    mysql_stmt_close(stmt);
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        return 1;
    }
    
    printf("MySQL Binding Fix Test\n");
    printf("======================\n");
    
    test_proper_binding(conn);
    test_mud_style_binding(conn);
    
    mysql_close(conn);
    
    return 0;
}