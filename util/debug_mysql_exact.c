/* Debug the EXACT issue with the MUD's prepared statements */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <mariadb/mysql.h>



void hexdump(const char *desc, const void *addr, int len) {
    int i;
    unsigned char *pc = (unsigned char*)addr;
    
    printf("%s:\n", desc);
    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0) printf("\n");
            printf("  %04x ", i);
        }
        printf(" %02x", pc[i]);
    }
    printf("\n");
}

void test_exact_mud_query(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    char pattern[256];
    int level = 34;
    
    printf("\n=== TESTING EXACT MUD QUERY ===\n");
    
    /* The EXACT query from the MUD with LOWER(?) added */
    const char *query = 
        "SELECT DISTINCT he.tag, he.entry, he.min_level, he.last_updated, "
        "GROUP_CONCAT(DISTINCT CONCAT(UCASE(LEFT(hk2.keyword, 1)), LCASE(SUBSTRING(hk2.keyword, 2))) SEPARATOR ', ') "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "LEFT JOIN help_keywords hk2 ON he.tag = hk2.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ? "
        "GROUP BY he.tag, he.entry, he.min_level, he.last_updated "
        "ORDER BY LENGTH(hk.keyword) ASC";
    
    stmt = mysql_stmt_init(conn);
    if (!stmt) {
        printf("ERROR: Failed to init statement\n");
        return;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        printf("ERROR: Failed to prepare: %s\n", mysql_stmt_error(stmt));
        mysql_stmt_close(stmt);
        return;
    }
    
    /* Test different patterns */
    const char *test_patterns[] = {"score%", "Score%", "SCORE%"};
    
    for (int i = 0; i < 3; i++) {
        strcpy(pattern, test_patterns[i]);
        printf("\n--- Testing with pattern: '%s' ---\n", pattern);
        
        /* CLEAR the bindings completely */
        memset(params, 0, sizeof(params));
        
        /* Set up string parameter */
        params[0].buffer_type = MYSQL_TYPE_STRING;
        params[0].buffer = pattern;
        params[0].buffer_length = strlen(pattern) + 1;  /* Include null terminator */
        params[0].is_null = 0;
        params[0].length = 0;
        
        /* Set up int parameter */
        params[1].buffer_type = MYSQL_TYPE_LONG;
        params[1].buffer = &level;
        params[1].buffer_length = sizeof(int);
        params[1].is_null = 0;
        params[1].length = 0;
        
        printf("Binding params...\n");
        hexdump("params[0] structure", &params[0], sizeof(MYSQL_BIND));
        
        if (mysql_stmt_bind_param(stmt, params)) {
            printf("ERROR: Bind failed: %s\n", mysql_stmt_error(stmt));
            continue;
        }
        
        if (mysql_stmt_execute(stmt)) {
            printf("ERROR: Execute failed: %s\n", mysql_stmt_error(stmt));
            continue;
        }
        
        /* Don't bind results, just check row count */
        if (mysql_stmt_store_result(stmt)) {
            printf("ERROR: Store failed: %s\n", mysql_stmt_error(stmt));
            continue;
        }
        
        unsigned long num_rows = (unsigned long)mysql_stmt_num_rows(stmt);
        printf("Result: %lu rows\n", num_rows);
        
        /* Free the stored result for next iteration */
        mysql_stmt_free_result(stmt);
    }
    
    mysql_stmt_close(stmt);
}

void test_without_group_concat(MYSQL *conn) {
    MYSQL_STMT *stmt;
    MYSQL_BIND params[2];
    char pattern[256] = "score%";
    int level = 34;
    
    printf("\n=== TESTING WITHOUT GROUP_CONCAT ===\n");
    
    /* Simplified query */
    const char *query = 
        "SELECT DISTINCT he.tag "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE LOWER(?) AND he.min_level <= ?";
    
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, query, strlen(query));
    
    memset(params, 0, sizeof(params));
    
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = pattern;
    params[0].buffer_length = strlen(pattern) + 1;
    
    params[1].buffer_type = MYSQL_TYPE_LONG;
    params[1].buffer = &level;
    
    mysql_stmt_bind_param(stmt, params);
    mysql_stmt_execute(stmt);
    mysql_stmt_store_result(stmt);
    
    printf("Result: %lu rows\n", (unsigned long)mysql_stmt_num_rows(stmt));
    
    mysql_stmt_close(stmt);
}

void test_inner_vs_left_join(MYSQL *conn) {
    printf("\n=== TESTING JOIN TYPES ===\n");
    
    /* Test with INNER JOIN */
    if (mysql_query(conn, 
        "SELECT COUNT(DISTINCT he.tag) "
        "FROM help_entries he "
        "INNER JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE 'score%'") == 0) {
        
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        printf("INNER JOIN result: %s rows\n", row[0]);
        mysql_free_result(res);
    }
    
    /* Test with LEFT JOIN */
    if (mysql_query(conn, 
        "SELECT COUNT(DISTINCT he.tag) "
        "FROM help_entries he "
        "LEFT JOIN help_keywords hk ON he.tag = hk.help_tag "
        "WHERE LOWER(hk.keyword) LIKE 'score%'") == 0) {
        
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        printf("LEFT JOIN result: %s rows\n", row[0]);
        mysql_free_result(res);
    }
    
    /* Check what keywords exist */
    if (mysql_query(conn, 
        "SELECT hk.keyword, hk.help_tag "
        "FROM help_keywords hk "
        "WHERE LOWER(hk.keyword) LIKE 'score%'") == 0) {
        
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;
        printf("\nKeywords matching 'score%%':\n");
        while ((row = mysql_fetch_row(res))) {
            printf("  keyword='%s', tag='%s'\n", row[0], row[1]);
        }
        mysql_free_result(res);
    }
}

void test_character_set(MYSQL *conn) {
    printf("\n=== TESTING CHARACTER SET ===\n");
    
    /* Check connection character set */
    if (mysql_query(conn, "SHOW VARIABLES LIKE 'character_set%'") == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            printf("  %s = %s\n", row[0], row[1]);
        }
        mysql_free_result(res);
    }
    
    /* Check collation */
    if (mysql_query(conn, "SHOW VARIABLES LIKE 'collation%'") == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            printf("  %s = %s\n", row[0], row[1]);
        }
        mysql_free_result(res);
    }
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    
    if (!mysql_real_connect(conn, "localhost", "luminari_mud", "vZ$eO}fD-4%7",
                           "luminari_mudprod", 0, NULL, 0)) {
        fprintf(stderr, "Failed to connect: %s\n", mysql_error(conn));
        return 1;
    }
    
    printf("MySQL Debug - Finding the exact issue\n");
    printf("======================================\n");
    
    test_character_set(conn);
    test_inner_vs_left_join(conn);
    test_without_group_concat(conn);
    test_exact_mud_query(conn);
    
    mysql_close(conn);
    
    return 0;
}