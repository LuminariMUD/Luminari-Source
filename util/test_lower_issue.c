/* Test to isolate the LOWER() issue with prepared statements */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



void test_query(MYSQL *conn, const char *query_desc, const char *query, const char *pattern) {
    MYSQL_STMT *stmt;
    MYSQL_BIND param;
    MYSQL_BIND result;
    int count;
    my_bool count_is_null;
    
    printf("\n%s:\n", query_desc);
    printf("Query: %s\n", query);
    printf("Pattern: '%s'\n", pattern);
    
    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("ERROR: Failed to prepare: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    memset(&param, 0, sizeof(param));
    param.buffer_type = MYSQL_TYPE_STRING;
    param.buffer = (void*)pattern;
    param.buffer_length = strlen(pattern);
    
    if (mysql_stmt_bind_param(stmt, &param)) {
        printf("ERROR: Bind failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    if (mysql_stmt_execute(stmt)) {
        printf("ERROR: Execute failed: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &count;
    result.is_null = &count_is_null;
    
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);
    mysql_stmt_fetch(stmt);
    
    printf("Result: %d matches\n", count);
    
    mysql_stmt_close(stmt);
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        return 1;
    }
    
    printf("Testing LOWER() function with prepared statements\n");
    printf("==================================================\n");
    
    /* Show the actual data */
    printf("\nActual data in help_keywords:\n");
    if (mysql_query(conn, "SELECT DISTINCT keyword FROM help_keywords WHERE keyword LIKE '%score%'") == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            printf("  '%s'\n", row[0]);
        }
        mysql_free_result(res);
    }
    
    /* Test different query patterns */
    test_query(conn, "TEST 1: No LOWER(), exact match", 
               "SELECT COUNT(*) FROM help_keywords WHERE keyword = ?", 
               "Score");
    
    test_query(conn, "TEST 2: No LOWER(), LIKE with wildcard", 
               "SELECT COUNT(*) FROM help_keywords WHERE keyword LIKE ?", 
               "Score%");
    
    test_query(conn, "TEST 3: LOWER() on column only", 
               "SELECT COUNT(*) FROM help_keywords WHERE LOWER(keyword) LIKE ?", 
               "score%");
    
    test_query(conn, "TEST 4: LOWER() on both sides", 
               "SELECT COUNT(*) FROM help_keywords WHERE LOWER(keyword) LIKE LOWER(?)", 
               "score%");
    
    test_query(conn, "TEST 5: LOWER() on both sides, uppercase pattern", 
               "SELECT COUNT(*) FROM help_keywords WHERE LOWER(keyword) LIKE LOWER(?)", 
               "SCORE%");
    
    test_query(conn, "TEST 6: Case-insensitive collation", 
               "SELECT COUNT(*) FROM help_keywords WHERE keyword COLLATE utf8mb4_general_ci LIKE ?", 
               "score%");
    
    /* Now test with actual help query structure */
    printf("\n\nTesting full help query:\n");
    printf("========================\n");
    
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    char pattern[256] = "score%";
    int level = 34;
    int count;
    MYSQL_BIND result;
    my_bool count_is_null;
    
    const char *full_query = 
        "SELECT COUNT(*) "
        "FROM help_entries he "
        "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) "
        "AND he.min_level <= ?";
    
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, full_query, strlen(full_query));
    
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = pattern;
    params[0].buffer_length = strlen(pattern);
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    
    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);
    
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &count;
    result.is_null = &count_is_null;
    
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);
    mysql_stmt_fetch(stmt);
    
    printf("Full query with pattern='%s', level=%d: %d matches\n", pattern, level, count);
    
    mysql_stmt_close(stmt);
    
    /* Test without LOWER() */
    const char *no_lower_query = 
        "SELECT COUNT(*) "
        "FROM help_entries he "
        "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE hk.keyword LIKE ? "
        "AND he.min_level <= ?";
    
    char pattern2[256] = "Score%";  /* Capital S */
    
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, no_lower_query, strlen(no_lower_query));
    
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = pattern2;
    params[0].buffer_length = strlen(pattern2);
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    
    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);
    
    memset(&result, 0, sizeof(result));
    result.buffer_type = MYSQL_TYPE_LONG;
    result.buffer = &count;
    result.is_null = &count_is_null;
    
    mysql_stmt_bind_result(stmt, &result);
    mysql_stmt_store_result(stmt);
    mysql_stmt_fetch(stmt);
    
    printf("Query without LOWER(), pattern='%s', level=%d: %d matches\n", pattern2, level, count);
    
    mysql_stmt_close(stmt);
    mysql_close(conn);
    
    return 0;
}