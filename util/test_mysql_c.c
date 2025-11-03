/* Test MySQL prepared statements in C to verify the fix */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>

/* For compatibility with newer MySQL versions */


#define CREATE(ptr, type, num) do { \
    (ptr) = (type *)calloc((num), sizeof(type)); \
    if (!(ptr)) { \
        fprintf(stderr, "Memory allocation failed\n"); \
        exit(1); \
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

void log_error(const char *msg, MYSQL_STMT *stmt) {
    fprintf(stderr, "ERROR: %s\n", msg);
    if (stmt) {
        fprintf(stderr, "  MySQL Error: %s (Error: %u)\n", 
                mysql_stmt_error(stmt), mysql_stmt_errno(stmt));
    }
}

void test_help_search(MYSQL *conn) {
    PREPARED_STMT pstmt;
    MYSQL_BIND params[2];
    MYSQL_BIND results[5];
    char search_pattern[256] = "score%";
    int level = 34;
    
    /* Result buffers */
    char tag[201];
    char entry[65536];
    int min_level;
    MYSQL_TIME last_updated;
    char keywords[65536];
    
    unsigned long tag_length;
    unsigned long entry_length;
    unsigned long keywords_length;
    
    my_bool tag_is_null;
    my_bool entry_is_null;
    my_bool min_level_is_null;
    my_bool last_updated_is_null;
    my_bool keywords_is_null;
    
    my_bool tag_error;
    my_bool entry_error;
    my_bool min_level_error;
    my_bool last_updated_error;
    my_bool keywords_error;
    
    printf("\n================================================================================\n");
    printf("TESTING HELP SEARCH QUERY (C Implementation)\n");
    printf("================================================================================\n");
    
    const char *query = 
        "SELECT he.tag, he.entry, he.min_level, he.last_updated, "
        "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ') "
        "FROM help_entries he "
        "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
        "LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) "
        "AND he.min_level <= ? "
        "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
        "ORDER BY he.tag";
    
    /* Create statement */
    pstmt.stmt = mysql_stmt_init(conn);
    if (!pstmt.stmt) {
        log_error("Failed to init statement", NULL);
        return;
    }
    
    /* Prepare statement */
    if (mysql_stmt_prepare(pstmt.stmt, query, strlen(query))) {
        log_error("Failed to prepare statement", pstmt.stmt);
        mysql_stmt_close(pstmt.stmt);
        return;
    }
    
    printf("Query prepared successfully\n");
    
    /* Clear and set up parameter bindings */
    memset(params, 0, sizeof(params));
    
    /* First parameter - search pattern */
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = search_pattern;
    params[0].buffer_length = sizeof(search_pattern);
    params[0].is_null = 0;
    params[0].length = 0;
    
    /* Second parameter - level */
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    params[1].is_null = 0;
    params[1].length = 0;
    
    /* Bind parameters */
    if (mysql_stmt_bind_param(pstmt.stmt, params)) {
        log_error("Failed to bind parameters", pstmt.stmt);
        mysql_stmt_close(pstmt.stmt);
        return;
    }
    
    printf("Parameters bound: pattern='%s', level=%d\n", search_pattern, level);
    
    /* Execute statement */
    if (mysql_stmt_execute(pstmt.stmt)) {
        log_error("Failed to execute statement", pstmt.stmt);
        mysql_stmt_close(pstmt.stmt);
        return;
    }
    
    /* Clear and set up result bindings */
    memset(results, 0, sizeof(results));
    
    /* Column 0: tag */
    results[0].buffer_type = MYSQL_TYPE_STRING;
    results[0].buffer = tag;
    results[0].buffer_length = sizeof(tag);
    results[0].length = &tag_length;
    results[0].is_null = &tag_is_null;
    results[0].error = &tag_error;
    
    /* Column 1: entry */
    results[1].buffer_type = MYSQL_TYPE_STRING;
    results[1].buffer = entry;
    results[1].buffer_length = sizeof(entry);
    results[1].length = &entry_length;
    results[1].is_null = &entry_is_null;
    results[1].error = &entry_error;
    
    /* Column 2: min_level */
    results[2].buffer_type = MYSQL_TYPE_LONG;
    results[2].buffer = &min_level;
    results[2].is_null = &min_level_is_null;
    results[2].error = &min_level_error;
    
    /* Column 3: last_updated */
    results[3].buffer_type = MYSQL_TYPE_DATETIME;
    results[3].buffer = &last_updated;
    results[3].is_null = &last_updated_is_null;
    results[3].error = &last_updated_error;
    
    /* Column 4: keywords (GROUP_CONCAT) */
    results[4].buffer_type = MYSQL_TYPE_STRING;
    results[4].buffer = keywords;
    results[4].buffer_length = sizeof(keywords);
    results[4].length = &keywords_length;
    results[4].is_null = &keywords_is_null;
    results[4].error = &keywords_error;
    
    /* Bind result */
    if (mysql_stmt_bind_result(pstmt.stmt, results)) {
        log_error("Failed to bind results", pstmt.stmt);
        mysql_stmt_close(pstmt.stmt);
        return;
    }
    
    /* Store result */
    if (mysql_stmt_store_result(pstmt.stmt)) {
        log_error("Failed to store result", pstmt.stmt);
        mysql_stmt_close(pstmt.stmt);
        return;
    }
    
    printf("Results stored: %lu rows\n", (unsigned long)mysql_stmt_num_rows(pstmt.stmt));
    
    /* Fetch results */
    int row_count = 0;
    int fetch_result;
    while ((fetch_result = mysql_stmt_fetch(pstmt.stmt)) == 0 || fetch_result == MYSQL_DATA_TRUNCATED) {
        row_count++;
        
        if (fetch_result == MYSQL_DATA_TRUNCATED) {
            printf("WARNING: Data truncated on row %d\n", row_count);
            if (tag_error) printf("  - tag truncated\n");
            if (entry_error) printf("  - entry truncated\n");
            if (min_level_error) printf("  - min_level truncated\n");
            if (last_updated_error) printf("  - last_updated truncated\n");
            if (keywords_error) printf("  - keywords truncated (length=%lu)\n", keywords_length);
        }
        
        printf("\nRow %d:\n", row_count);
        printf("  Tag: %s\n", tag_is_null ? "(NULL)" : tag);
        printf("  Entry: %.100s%s\n", 
               entry_is_null ? "(NULL)" : entry,
               (!entry_is_null && strlen(entry) > 100) ? "..." : "");
        printf("  Min Level: %d\n", min_level_is_null ? -1 : min_level);
        printf("  Keywords: %s\n", keywords_is_null ? "(NULL)" : keywords);
    }
    
    if (row_count == 0) {
        printf("\n*** NO RESULTS FOUND - THE BUG IS STILL PRESENT! ***\n");
    } else {
        printf("\n*** SUCCESS! Found %d rows - THE FIX WORKS! ***\n", row_count);
    }
    
    mysql_stmt_close(pstmt.stmt);
}

void test_soundex(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    MYSQL_BIND results[2];
    char search_term[256] = "sscore";
    int level = 34;
    
    char help_tag[201];
    char keyword[201];
    unsigned long tag_length;
    unsigned long keyword_length;
    my_bool tag_is_null;
    my_bool keyword_is_null;
    my_bool tag_error;
    my_bool keyword_error;
    
    printf("\n================================================================================\n");
    printf("TESTING SOUNDEX QUERY (C Implementation)\n");
    printf("================================================================================\n");
    
    const char *query = 
        "SELECT hk.help_tag, hk.keyword "
        "FROM help_entries he, help_keywords hk "
        "WHERE he.tag = hk.help_tag "
        "AND hk.keyword SOUNDS LIKE ? "
        "AND he.min_level <= ? "
        "ORDER BY LENGTH(hk.keyword) ASC";
    
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        log_error("Failed to init statement", NULL);
        return;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        log_error("Failed to prepare statement", stmt);
        mysql_stmt_close(stmt);
        return;
    }
    
    /* Clear and set up parameter bindings - THIS IS THE KEY FIX */
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = search_term;
    params[0].buffer_length = sizeof(search_term);
    params[0].is_null = 0;
    params[0].length = 0;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    params[1].is_null = 0;
    params[1].length = 0;
    
    if (mysql_stmt_bind_param(stmt, params)) {
        log_error("Failed to bind parameters", stmt);
        mysql_stmt_close(stmt);
        return;
    }
    
    printf("Parameters bound: term='%s', level=%d\n", search_term, level);
    
    if (mysql_stmt_execute(stmt)) {
        log_error("Failed to execute statement", stmt);
        mysql_stmt_close(stmt);
        return;
    }
    
    /* Clear and set up result bindings - ALSO CRITICAL */
    memset(results, 0, sizeof(results));
    
    results[0].buffer_type = MYSQL_TYPE_STRING;
    results[0].buffer = help_tag;
    results[0].buffer_length = sizeof(help_tag);
    results[0].length = &tag_length;
    results[0].is_null = &tag_is_null;
    results[0].error = &tag_error;
    
    results[1].buffer_type = MYSQL_TYPE_STRING;
    results[1].buffer = keyword;
    results[1].buffer_length = sizeof(keyword);
    results[1].length = &keyword_length;
    results[1].is_null = &keyword_is_null;
    results[1].error = &keyword_error;
    
    if (mysql_stmt_bind_result(stmt, results)) {
        log_error("Failed to bind results", stmt);
        mysql_stmt_close(stmt);
        return;
    }
    
    if (mysql_stmt_store_result(stmt)) {
        log_error("Failed to store result", stmt);
        mysql_stmt_close(stmt);
        return;
    }
    
    printf("Results stored: %lu rows\n", (unsigned long)mysql_stmt_num_rows(stmt));
    
    int row_count = 0;
    int fetch_result;
    while ((fetch_result = mysql_stmt_fetch(stmt)) == 0 || fetch_result == MYSQL_DATA_TRUNCATED) {
        row_count++;
        printf("  Row %d: Tag='%s', Keyword='%s'\n", row_count, help_tag, keyword);
    }
    
    if (row_count == 0) {
        printf("\n*** NO SOUNDEX MATCHES - BUG PRESENT! ***\n");
    } else {
        printf("\n*** SUCCESS! Found %d soundex matches - FIX WORKS! ***\n", row_count);
    }
    
    mysql_stmt_close(stmt);
}

int main() {
    MYSQL *conn;
    
    printf("MySQL C Prepared Statement Test\n");
    printf("Testing the exact fix for the help system bug\n");
    
    /* Initialize MySQL */
    conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "Failed to initialize MySQL\n");
        return 1;
    }
    
    /* Connect to database */
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }
    
    printf("Connected to database successfully\n");
    
    /* Run tests */
    test_help_search(conn);
    test_soundex(conn);
    
    /* Cleanup */
    mysql_close(conn);
    
    printf("\n================================================================================\n");
    printf("TEST COMPLETE\n");
    printf("If both tests show SUCCESS, the fix is working!\n");
    printf("================================================================================\n");
    
    return 0;
}