# LuminariMUD Database Integration

## Overview

LuminariMUD uses MariaDB (or MySQL) as its primary database system for persistent storage of player data, world state, and game statistics. The database integration provides robust data persistence, player authentication, and advanced querying capabilities for game mechanics.

**Note**: As of January 2025, the codebase has been upgraded to use MariaDB client libraries (libmariadb-dev) for improved compatibility and security. The system remains fully compatible with both MariaDB and MySQL servers.

## Database Architecture

### Connection Management

The database connection is managed through a global MariaDB/MySQL connection handle:

```c
MYSQL *conn;  // Global database connection

// Connection initialization
bool mysql_connect() {
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, MYSQL_SERVER, MYSQL_USER, 
                           MYSQL_PASSWD, MYSQL_DB, 0, NULL, 0)) {
        log("SYSERR: MySQL connection failed: %s", mysql_error(conn));
        return FALSE;
    }
    return TRUE;
}
```

### Connection Persistence

The system maintains persistent connections with automatic reconnection:

```c
// Check and restore connection
void mysql_ping_connection() {
    if (mysql_ping(conn) != 0) {
        log("SYSERR: MySQL connection lost, attempting reconnect...");
        mysql_connect();
    }
}
```

## Core Database Tables

### 1. Player Data Tables

#### `player_data` - Core Player Information
```sql
CREATE TABLE player_data (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(20) UNIQUE NOT NULL,
    password VARCHAR(32) NOT NULL,
    email VARCHAR(100),
    level INT DEFAULT 1,
    experience BIGINT DEFAULT 0,
    class INT DEFAULT 0,
    race INT DEFAULT 0,
    alignment INT DEFAULT 0,
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_logon TIMESTAMP,
    total_sessions INT DEFAULT 0,
    bad_pws INT DEFAULT 0,
    INDEX idx_name (name),
    INDEX idx_level (level)
);
```

#### `player_abilities` - Character Abilities
```sql
CREATE TABLE player_abilities (
    player_id INT,
    strength INT DEFAULT 10,
    dexterity INT DEFAULT 10,
    constitution INT DEFAULT 10,
    intelligence INT DEFAULT 10,
    wisdom INT DEFAULT 10,
    charisma INT DEFAULT 10,
    FOREIGN KEY (player_id) REFERENCES player_data(id)
);
```

#### `player_skills` - Skill Ranks and Bonuses
```sql
CREATE TABLE player_skills (
    player_id INT,
    skill_id INT,
    ranks INT DEFAULT 0,
    bonus INT DEFAULT 0,
    PRIMARY KEY (player_id, skill_id),
    FOREIGN KEY (player_id) REFERENCES player_data(id)
);
```

### 2. World State Tables

#### `room_data` - Room Information
```sql
CREATE TABLE room_data (
    vnum INT PRIMARY KEY,
    name VARCHAR(255),
    description TEXT,
    zone_id INT,
    room_flags BIGINT DEFAULT 0,
    sector_type INT DEFAULT 0,
    INDEX idx_zone (zone_id)
);
```

#### `object_instances` - Object State
```sql
CREATE TABLE object_instances (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    vnum INT NOT NULL,
    location_type ENUM('room', 'player', 'object'),
    location_id BIGINT,
    wear_position INT DEFAULT -1,
    condition_value INT DEFAULT 100,
    created TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

### 3. Game Statistics Tables

#### `combat_logs` - Combat Statistics
```sql
CREATE TABLE combat_logs (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    attacker_id INT,
    defender_id INT,
    damage_dealt INT,
    attack_type VARCHAR(50),
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (attacker_id) REFERENCES player_data(id),
    FOREIGN KEY (defender_id) REFERENCES player_data(id)
);
```

## Database Operations

### Player Data Management

#### Loading Player Data
```c
struct char_data *load_player_from_db(const char *name) {
    MYSQL_RES *result;
    MYSQL_ROW row;
    struct char_data *ch = NULL;
    char query[MAX_STRING_LENGTH];
    
    mysql_ping(conn);
    
    snprintf(query, sizeof(query),
        "SELECT id, name, level, experience, class, race "
        "FROM player_data WHERE name = '%s'", name);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: MySQL query error: %s", mysql_error(conn));
        return NULL;
    }
    
    result = mysql_store_result(conn);
    if ((row = mysql_fetch_row(result))) {
        ch = create_char();
        GET_IDNUM(ch) = atoi(row[0]);
        strcpy(GET_NAME(ch), row[1]);
        GET_LEVEL(ch) = atoi(row[2]);
        GET_EXP(ch) = atoll(row[3]);
        GET_CLASS(ch) = atoi(row[4]);
        GET_RACE(ch) = atoi(row[5]);
    }
    
    mysql_free_result(result);
    return ch;
}
```

#### Saving Player Data
```c
void save_player_to_db(struct char_data *ch) {
    char query[MAX_STRING_LENGTH];
    char escaped_name[MAX_NAME_LENGTH * 2 + 1];
    
    mysql_ping(conn);
    mysql_real_escape_string(conn, escaped_name, GET_NAME(ch), strlen(GET_NAME(ch)));
    
    snprintf(query, sizeof(query),
        "INSERT INTO player_data (name, level, experience, class, race, last_logon) "
        "VALUES ('%s', %d, %lld, %d, %d, NOW()) "
        "ON DUPLICATE KEY UPDATE "
        "level = %d, experience = %lld, class = %d, race = %d, last_logon = NOW()",
        escaped_name, GET_LEVEL(ch), GET_EXP(ch), GET_CLASS(ch), GET_RACE(ch),
        GET_LEVEL(ch), GET_EXP(ch), GET_CLASS(ch), GET_RACE(ch));
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Failed to save player %s: %s", GET_NAME(ch), mysql_error(conn));
    }
}
```

### World State Persistence

#### Room State Management
```c
void save_room_state(room_rnum room) {
    char query[MAX_STRING_LENGTH];
    struct room_data *rm = &world[room];
    
    snprintf(query, sizeof(query),
        "INSERT INTO room_data (vnum, name, description, zone_id, room_flags, sector_type) "
        "VALUES (%d, '%s', '%s', %d, %lld, %d) "
        "ON DUPLICATE KEY UPDATE "
        "name = '%s', description = '%s', room_flags = %lld, sector_type = %d",
        rm->number, rm->name, rm->description, rm->zone, rm->room_flags, rm->sector_type,
        rm->name, rm->description, rm->room_flags, rm->sector_type);
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Failed to save room %d: %s", rm->number, mysql_error(conn));
    }
}
```

### Query Optimization

#### Prepared Statements
```c
MYSQL_STMT *stmt_save_player;

void init_prepared_statements() {
    stmt_save_player = mysql_stmt_init(conn);
    const char *query = "INSERT INTO player_data (name, level, experience) VALUES (?, ?, ?)";
    
    if (mysql_stmt_prepare(stmt_save_player, query, strlen(query))) {
        log("SYSERR: Failed to prepare statement: %s", mysql_stmt_error(stmt_save_player));
    }
}

void save_player_prepared(struct char_data *ch) {
    MYSQL_BIND bind[3];
    memset(bind, 0, sizeof(bind));
    
    // Bind parameters
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = GET_NAME(ch);
    bind[0].buffer_length = strlen(GET_NAME(ch));
    
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &GET_LEVEL(ch);
    
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].buffer = &GET_EXP(ch);
    
    mysql_stmt_bind_param(stmt_save_player, bind);
    mysql_stmt_execute(stmt_save_player);
}
```

## Database Schema Management

### Schema Versioning
```sql
CREATE TABLE schema_version (
    version INT PRIMARY KEY,
    applied_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    description VARCHAR(255)
);

INSERT INTO schema_version (version, description) 
VALUES (1, 'Initial schema creation');
```

### Migration System
```c
void check_database_version() {
    MYSQL_RES *result;
    MYSQL_ROW row;
    int current_version = 0;
    
    if (mysql_query(conn, "SELECT MAX(version) FROM schema_version")) {
        log("SYSERR: Cannot check database version: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    if ((row = mysql_fetch_row(result))) {
        current_version = row[0] ? atoi(row[0]) : 0;
    }
    mysql_free_result(result);
    
    if (current_version < REQUIRED_DB_VERSION) {
        log("SYSERR: Database version %d is outdated. Required: %d", 
            current_version, REQUIRED_DB_VERSION);
        exit(1);
    }
}
```

### Recent Schema Changes (2025)
Several tables have been updated to include missing `idnum` columns for proper foreign key relationships:
- Added `idnum` column to various player-related tables for consistent referencing
- Ensures all player data can be properly linked via player ID
- Fixes issues with orphaned records in related tables

## Performance Optimization

### Connection Pooling
```c
#define MAX_DB_CONNECTIONS 5
MYSQL *connection_pool[MAX_DB_CONNECTIONS];
bool connection_in_use[MAX_DB_CONNECTIONS];

MYSQL *get_db_connection() {
    int i;
    for (i = 0; i < MAX_DB_CONNECTIONS; i++) {
        if (!connection_in_use[i]) {
            connection_in_use[i] = TRUE;
            mysql_ping(connection_pool[i]);
            return connection_pool[i];
        }
    }
    return NULL; // All connections in use
}

void release_db_connection(MYSQL *conn) {
    int i;
    for (i = 0; i < MAX_DB_CONNECTIONS; i++) {
        if (connection_pool[i] == conn) {
            connection_in_use[i] = FALSE;
            break;
        }
    }
}
```

### Batch Operations
```c
void batch_save_players() {
    char query[MAX_STRING_LENGTH * 10];
    strcpy(query, "INSERT INTO player_data (name, level, experience) VALUES ");
    
    struct char_data *ch;
    bool first = TRUE;
    
    for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || !ch->desc) continue;
        
        if (!first) strcat(query, ", ");
        
        char values[256];
        snprintf(values, sizeof(values), "('%s', %d, %lld)",
                GET_NAME(ch), GET_LEVEL(ch), GET_EXP(ch));
        strcat(query, values);
        first = FALSE;
    }
    
    strcat(query, " ON DUPLICATE KEY UPDATE level = VALUES(level), experience = VALUES(experience)");
    
    if (mysql_query(conn, query)) {
        log("SYSERR: Batch save failed: %s", mysql_error(conn));
    }
}
```

## Error Handling and Recovery

### Transaction Management
```c
bool execute_transaction(const char **queries, int count) {
    int i;
    mysql_autocommit(conn, 0); // Start transaction
    
    for (i = 0; i < count; i++) {
        if (mysql_query(conn, queries[i])) {
            log("SYSERR: Transaction query %d failed: %s", i, mysql_error(conn));
            mysql_rollback(conn);
            mysql_autocommit(conn, 1);
            return FALSE;
        }
    }
    
    mysql_commit(conn);
    mysql_autocommit(conn, 1);
    return TRUE;
}
```

### Backup and Recovery
```c
void backup_player_data() {
    char backup_file[256];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    strftime(backup_file, sizeof(backup_file), 
             "../backup/players_%Y%m%d_%H%M%S.sql", tm_info);
    
    char command[512];
    snprintf(command, sizeof(command),
             "mysqldump -u %s -p%s %s player_data > %s",
             MYSQL_USER, MYSQL_PASSWD, MYSQL_DB, backup_file);
    
    if (system(command) == 0) {
        log("Player data backed up to %s", backup_file);
    } else {
        log("SYSERR: Backup failed for %s", backup_file);
    }
}
```

## Configuration

### Database Configuration Options
```c
// In campaign.h
#define MYSQL_SERVER "localhost"
#define MYSQL_USER "luminari"
#define MYSQL_PASSWD "secure_password"
#define MYSQL_DB "luminari"
#define MYSQL_PORT 3306

// Connection options
#define DB_RECONNECT_ATTEMPTS 3
#define DB_QUERY_TIMEOUT 30
#define DB_CONNECTION_TIMEOUT 10
```

### Runtime Configuration
```c
void configure_mysql_connection() {
    unsigned int timeout = DB_CONNECTION_TIMEOUT;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    timeout = DB_QUERY_TIMEOUT;
    mysql_options(conn, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(conn, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    
    my_bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);
}
```

## Monitoring and Maintenance

### Database Health Checks
```c
void check_database_health() {
    MYSQL_RES *result;
    
    // Check connection status
    if (mysql_ping(conn) != 0) {
        log("SYSERR: Database connection unhealthy");
        return;
    }
    
    // Check table status
    if (mysql_query(conn, "SHOW TABLE STATUS")) {
        log("SYSERR: Cannot check table status: %s", mysql_error(conn));
        return;
    }
    
    result = mysql_store_result(conn);
    log("Database health check: %d tables found", mysql_num_rows(result));
    mysql_free_result(result);
}
```

### Performance Monitoring
```c
void log_database_stats() {
    MYSQL_RES *result;
    MYSQL_ROW row;
    
    if (mysql_query(conn, "SHOW STATUS LIKE 'Queries'")) return;
    
    result = mysql_store_result(conn);
    if ((row = mysql_fetch_row(result))) {
        log("Database queries executed: %s", row[1]);
    }
    mysql_free_result(result);
}
```

## Security Considerations

### SQL Injection Prevention
- Always use `mysql_real_escape_string()` for user input
- Use prepared statements for complex queries
- Validate input data before database operations
- Implement proper access controls

### Connection Security
- Use secure passwords for database users
- Limit database user privileges
- Enable SSL connections when possible
- Monitor database access logs

---

*For additional database administration and advanced configuration, refer to the MySQL documentation and the [Troubleshooting Guide](TROUBLESHOOTING_AND_MAINTENANCE.md).*
