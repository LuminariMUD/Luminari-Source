/* Simplified test to isolate the issue */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



void test_simple_query(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    MYSQL_BIND results[3];
    
    char search_pattern[256] = "score%";
    int level = 34;
    
    char tag[201];
    char entry[65536];
    int min_level;
    
    unsigned long tag_length;
    unsigned long entry_length;
    
    my_bool tag_is_null;
    my_bool entry_is_null;
    my_bool min_level_is_null;
    
    my_bool tag_error;
    my_bool entry_error;
    my_bool min_level_error;
    
    printf("\n=== TEST 1: Simple query without GROUP_CONCAT ===\n");
    
    const char *query = 
        "SELECT he.tag, he.entry, he.min_level "
        "FROM help_entries he "
        "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) "
        "AND he.min_level <= ?";
    
    stmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("ERROR: Failed to prepare: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = search_pattern;
    params[0].buffer_length = sizeof(search_pattern);
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    
    if (mysql_stmt_bind_param(stmt, params)) {
        printf("ERROR: Failed to bind params: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    printf("Executing with pattern='%s', level=%d\n", search_pattern, level);
    
    if (mysql_stmt_execute(stmt)) {
        printf("ERROR: Failed to execute: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    memset(results, 0, sizeof(results));
    
    results[0].buffer_type = MYSQL_TYPE_STRING;
    results[0].buffer = tag;
    results[0].buffer_length = sizeof(tag);
    results[0].length = &tag_length;
    results[0].is_null = &tag_is_null;
    results[0].error = &tag_error;
    
    results[1].buffer_type = MYSQL_TYPE_STRING;
    results[1].buffer = entry;
    results[1].buffer_length = sizeof(entry);
    results[1].length = &entry_length;
    results[1].is_null = &entry_is_null;
    results[1].error = &entry_error;
    
    results[2].buffer_type = MYSQL_TYPE_LONG;
    results[2].buffer = &min_level;
    results[2].is_null = &min_level_is_null;
    results[2].error = &min_level_error;
    
    if (mysql_stmt_bind_result(stmt, results)) {
        printf("ERROR: Failed to bind results: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    if (mysql_stmt_store_result(stmt)) {
        printf("ERROR: Failed to store: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    unsigned long num_rows = (unsigned long)mysql_stmt_num_rows(stmt);
    printf("Stored %lu rows\n", num_rows);
    
    int row_count = 0;
    int fetch_result;
    while ((fetch_result = mysql_stmt_fetch(stmt)) == 0 || fetch_result == MYSQL_DATA_TRUNCATED) {
        row_count++;
        printf("Row %d: tag='%s'\n", row_count, tag);
    }
    
    printf("Result: %s\n", row_count > 0 ? "SUCCESS!" : "FAILED - No rows!");
    
    mysql_stmt_close(stmt);
}

void test_direct_query(MYSQL *conn) {
    printf("\n=== TEST 2: Direct non-prepared query ===\n");
    
    if (mysql_query(conn, 
        "SELECT he.tag, hk.keyword "
        "FROM help_entries he "
        "JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE hk.keyword LIKE 'score%'")) {
        printf("ERROR: Query failed: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        printf("ERROR: No result: %s\n", mysql_error(conn));
        return;
    }
    
    int num_rows = mysql_num_rows(result);
    printf("Found %d rows\n", num_rows);
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("  Tag: %s, Keyword: %s\n", row[0], row[1]);
    }
    
    mysql_free_result(result);
}

void test_case_sensitivity(MYSQL *conn) {
    printf("\n=== TEST 3: Case sensitivity check ===\n");
    
    MYSQL_STMT *stmt;
    MYSQL_BIND param;
    MYSQL_BIND result;
    int count;
    my_bool count_is_null;
    
    const char *patterns[] = {"score", "Score", "SCORE", "score%", "Score%"};
    
    for (int i = 0; i < 5; i++) {
        const char *query = "SELECT COUNT(*) FROM help_keywords WHERE LOWER(keyword) LIKE LOWER(?)";
        
        stmt = mysql_stmt_init(conn);
        mysql_stmt_prepare(stmt, query, strlen(query));
        
        memset(&param, 0, sizeof(param));
        param.buffer_type = MYSQL_TYPE_STRING;
        param.buffer = (void*)patterns[i];
        param.buffer_length = strlen(patterns[i]);
        
        mysql_stmt_bind_param(stmt, &param);
        mysql_stmt_execute(stmt);
        
        memset(&result, 0, sizeof(result));
        result.buffer_type = MYSQL_TYPE_LONG;
        result.buffer = &count;
        result.is_null = &count_is_null;
        
        mysql_stmt_bind_result(stmt, &result);
        mysql_stmt_store_result(stmt);
        mysql_stmt_fetch(stmt);
        
        printf("  Pattern '%s': %d matches\n", patterns[i], count);
        
        mysql_stmt_close(stmt);
    }
}

void test_keyword_data(MYSQL *conn) {
    printf("\n=== TEST 4: Actual keyword data ===\n");
    
    if (mysql_query(conn, 
        "SELECT keyword, help_tag FROM help_keywords WHERE keyword LIKE '%score%' LIMIT 10")) {
        printf("ERROR: Query failed: %s\n", mysql_error(conn));
        return;
    }
    
    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row;
    
    printf("Keywords containing 'score':\n");
    while ((row = mysql_fetch_row(result))) {
        printf("  Keyword: '%s' -> Tag: '%s'\n", row[0], row[1]);
    }
    
    mysql_free_result(result);
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        return 1;
    }
    
    printf("MySQL Simple Test - Isolating the issue\n");
    printf("=========================================\n");
    
    test_direct_query(conn);
    test_keyword_data(conn);
    test_case_sensitivity(conn);
    test_simple_query(conn);
    
    mysql_close(conn);
    
    return 0;
}