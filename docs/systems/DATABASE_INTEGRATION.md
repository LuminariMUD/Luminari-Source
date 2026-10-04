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
    PREPARED_STMT *statement;
    struct char_data *ch = NULL;
    const char *value;

    statement = mysql_stmt_create(conn);
    if (statement == NULL ||
        !mysql_stmt_prepare_query(statement,
                                  "SELECT id, name, level, experience, class, race "
                                  "FROM player_data WHERE name = ?") ||
        !mysql_stmt_bind_param_string(statement, 0, name) ||
        !mysql_stmt_execute_prepared(statement)) {
        log("SYSERR: Unable to load player %s.", name);
        mysql_stmt_cleanup(statement);
        return NULL;
    }

    if (mysql_stmt_fetch_row(statement)) {
        ch = create_char();
        GET_IDNUM(ch) = mysql_stmt_get_long(statement, 0);
        value = mysql_stmt_get_string(statement, 1);
        strlcpy(GET_NAME(ch), value != NULL ? value : "", MAX_NAME_LENGTH + 1);
        GET_LEVEL(ch) = mysql_stmt_get_int(statement, 2);
        GET_EXP(ch) = mysql_stmt_get_long(statement, 3);
        GET_CLASS(ch) = mysql_stmt_get_int(statement, 4);
        GET_RACE(ch) = mysql_stmt_get_int(statement, 5);
    }

    mysql_stmt_cleanup(statement);
    return ch;
}
```

#### Saving Player Data

```c
void save_player_to_db(struct char_data *ch) {
    PREPARED_STMT *statement;

    statement = mysql_stmt_create(conn);
    if (statement == NULL ||
        !mysql_stmt_prepare_query(statement,
                                  "INSERT INTO player_data (name, level, experience, class, race, last_logon) "
                                  "VALUES (?, ?, ?, ?, ?, NOW()) "
                                  "ON DUPLICATE KEY UPDATE level = VALUES(level), "
                                  "experience = VALUES(experience), class = VALUES(class), "
                                  "race = VALUES(race), last_logon = NOW()") ||
        !mysql_stmt_bind_param_string(statement, 0, GET_NAME(ch)) ||
        !mysql_stmt_bind_param_int(statement, 1, GET_LEVEL(ch)) ||
        !mysql_stmt_bind_param_long(statement, 2, GET_EXP(ch)) ||
        !mysql_stmt_bind_param_int(statement, 3, GET_CLASS(ch)) ||
        !mysql_stmt_bind_param_int(statement, 4, GET_RACE(ch)) ||
        !mysql_stmt_execute_prepared(statement)) {
        log("SYSERR: Failed to save player %s.", GET_NAME(ch));
    }
    mysql_stmt_cleanup(statement);
}
```

### World State Persistence

#### Room State Management

```c
void save_room_state(room_rnum room) {
    PREPARED_STMT *statement;
    struct room_data *rm = &world[room];

    /* Room names and descriptions are builder text; they are bound, never formatted. */
    statement = mysql_stmt_create(conn);
    if (statement == NULL ||
        !mysql_stmt_prepare_query(statement,
                                  "INSERT INTO room_data (vnum, name, description, zone_id, room_flags, sector_type) "
                                  "VALUES (?, ?, ?, ?, ?, ?) "
                                  "ON DUPLICATE KEY UPDATE name = VALUES(name), "
                                  "description = VALUES(description), room_flags = VALUES(room_flags), "
                                  "sector_type = VALUES(sector_type)") ||
        !mysql_stmt_bind_param_int(statement, 0, rm->number) ||
        !mysql_stmt_bind_param_string(statement, 1, rm->name) ||
        !mysql_stmt_bind_param_string(statement, 2, rm->description) ||
        !mysql_stmt_bind_param_int(statement, 3, rm->zone) ||
        !mysql_stmt_bind_param_long(statement, 4, rm->room_flags) ||
        !mysql_stmt_bind_param_int(statement, 5, rm->sector_type) ||
        !mysql_stmt_execute_prepared(statement)) {
        log("SYSERR: Failed to save room %d.", rm->number);
    }
    mysql_stmt_cleanup(statement);
}
```

### Query Optimization

#### Prepared Statements

`src/database/mysql.h` provides the `PREPARED_STMT` wrapper. It is the default way to
run any SQL that carries a data value: the statement text stays constant and
every value is bound, so quoting, character sets, backslashes, and the session
`sql_mode` cannot change the statement's meaning. `src/player/account.c` is the
reference migration for this pattern.

```c
PREPARED_STMT *statement;
const char *value;

statement = mysql_stmt_create(conn);
if (statement == NULL ||
    !mysql_stmt_prepare_query(statement,
                              "SELECT id, email FROM account_data WHERE lower(name) = lower(?)") ||
    !mysql_stmt_bind_param_string(statement, 0, name) ||
    !mysql_stmt_execute_prepared(statement))
{
  log("SYSERR: Unable to load account row.");
  mysql_stmt_cleanup(statement); /* NULL-safe; every error path releases it */
  return -1;
}
while (mysql_stmt_fetch_row(statement))
{
  account_id = mysql_stmt_get_int(statement, 0);
  value = mysql_stmt_get_string(statement, 1); /* NULL for SQL NULL */
}
mysql_stmt_cleanup(statement);
```

Binding notes:

- `mysql_stmt_bind_param_string()` with a NULL pointer binds SQL `NULL`.
- `mysql_stmt_bind_param_int()` and `mysql_stmt_bind_param_long()` bind numbers;
  never format a number into the SQL text.
- `mysql_stmt_get_long()` reads `BIGINT` columns and `COUNT(*)` aggregates;
  `mysql_stmt_get_ulong()` reads `BIGINT UNSIGNED` columns whose values can
  exceed the signed range; `mysql_stmt_get_int()` reads smaller integer columns.
- `mysql_stmt_affected_rows_count()` and `mysql_stmt_insert_id(statement->stmt)`
  replace `mysql_affected_rows()` and `mysql_insert_id()`.
- Variable-length lists (`IN (...)`, multi-row `VALUES`) append placeholders
  only, then bind each value. Table and column names come from a compile-time
  table indexed by an enum, never from data.
- Executions are attributed to the performance monitor exactly like direct
  queries, so `perfmon` SQL families keep working after a migration.

## Database Schema Management

### Schema Versioning

`schema_migrations` records every applied migration by a date-based version
(`YYYYMMDDNN`). `startup_database_init()` in `src/database/db_startup_init.c`
first creates any missing table system, then runs the migration groups in
`src/database/db_init.c` on every boot. `apply_migration()` skips a version
that is already recorded, runs its single statement, and records it:

| Group | Versions | On failure |
| -- | -- | -- |
| `run_database_migrations()` | help content contract, 2026082401-08 | boot stops |
| `run_pet_persistence_migrations()` | pet tables, 2026080501-2026091007 | boot stops |
| `run_account_migrations()` | account password width, 2026091101 | boot stops |
| `run_legacy_table_migrations()` | `weather_cache`, `player_save_objs`, `hint_usage_log`, 2026092701-03; `ship_waypoints` arrival tolerance, 2026092901 | logged, retried next boot |

`CREATE TABLE IF NOT EXISTS` never changes a table that already exists, so a
column or index added to a create statement reaches existing databases only
through a new versioned migration. Write each statement so it is idempotent
(`ADD COLUMN IF NOT EXISTS`, `ADD INDEX IF NOT EXISTS`) and correct for both
the old and the current table shape. Statements that need a `DELIMITER` block
belong in `create_database_procedures()`, not in a migration or a `.sql` file.

### Recent Schema Changes (2025)

Several tables have been updated to include missing `idnum` columns for proper foreign key relationships:

- Added `idnum` column to various player-related tables for consistent referencing
- Ensures all player data can be properly linked via player ID
- Fixes issues with orphaned records in related tables

## Performance Optimization

### Connection Pooling

`src/database/mysql.c` keeps a small pool of connections (`MYSQL_POOL_MIN_SIZE` to
`MYSQL_POOL_MAX_SIZE`). The global handles `conn`, `conn2` and `conn3` are the pool's first three
handles, so the pool never closes a handle to replace it: every connection is made with
`MYSQL_OPT_RECONNECT` and reconnects in place.

`mysql_pool_query()` runs one statement on a pooled connection, for side lookups such as the
weather cache and wilderness descriptions. `mysql_pool_acquire()` checks a connection that has
been idle longer than `MYSQL_POOL_TIMEOUT` seconds. When the database cannot be reached it returns
`NULL`, and `mysql_pool_query()` reports a failed query: the pool never waits for the database,
because the game runs on the thread that asks. A failed query leaves its caller's result pointer
`NULL`.

### Batch Operations

Multi-row statements append placeholders only; the row values are bound after
the statement is prepared, exactly as `save_account_integer_set()` does for
unlock sets in `src/player/account.c`.

```c
void batch_save_players() {
    char query[MAX_STRING_LENGTH * 10];
    PREPARED_STMT *statement;
    struct char_data *ch;
    bool bound;
    int used;
    int count = 0;
    int slot = 0;

    used = snprintf(query, sizeof(query),
                    "INSERT INTO player_data (name, level, experience) VALUES ");
    for (ch = character_list; ch; ch = ch->next) {
        if (IS_NPC(ch) || !ch->desc) continue;
        used = snprintf_append(query, sizeof(query), used, "%s(?, ?, ?)", count > 0 ? ", " : "");
        count++;
    }
    if (count == 0) return;
    snprintf_append(query, sizeof(query), used,
                    " ON DUPLICATE KEY UPDATE level = VALUES(level), experience = VALUES(experience)");

    statement = mysql_stmt_create(conn);
    bound = statement != NULL && mysql_stmt_prepare_query(statement, query);
    for (ch = character_list; bound && ch; ch = ch->next) {
        if (IS_NPC(ch) || !ch->desc) continue;
        bound = mysql_stmt_bind_param_string(statement, slot++, GET_NAME(ch)) &&
                mysql_stmt_bind_param_int(statement, slot++, GET_LEVEL(ch)) &&
                mysql_stmt_bind_param_long(statement, slot++, GET_EXP(ch));
    }
    if (!bound || !mysql_stmt_execute_prepared(statement)) {
        log("SYSERR: Batch save failed.");
    }
    mysql_stmt_cleanup(statement);
}
```

## Error Handling and Recovery

### Transaction Management

A transaction is written with plain statements on `conn`: `START TRANSACTION`, the writes, then
`COMMIT`, with `ROLLBACK` on every path that fails. Schema statements (`CREATE`, `ALTER`) commit
an open transaction, so run a subsystem's ensure function before the transaction, never inside
it.

**A connection lost inside a transaction.** Every connection reconnects by itself. When one drops
inside a transaction, the server rolls the transaction back and one command fails; the client
library then puts the following statements on a new session, where each would commit on its own
and a `COMMIT` would report success. The query layer prevents that for every caller:

- Every command goes through it: `mysql_query()` (a macro for `luminari_mysql_query()`), the
  prepared-statement wrappers, and `ensure_mysql_connection()` (`MYSQL_PING_CONN`).
- A command marks the connection when, with a transaction open, it fails with client error 2006
  or 2013, fails after the library gave the transaction up (a reconnect it refused or could not
  make), or reconnects (a ping does, and reports success).
- A deadlock ends the same way with the connection up: the server rolls its victim's whole
  transaction back and fails one statement with error 1213. That marks the connection too.
- A marked connection sends nothing: queries and prepared statements fail at once.
- The mark ends with the transaction. `ROLLBACK` is sent, on the new session if need be. A
  `COMMIT` fails and is answered with a `ROLLBACK`. A new `START TRANSACTION` starts clean.

So a writer inside a transaction needs no check of its own for this: after a lost connection
every later statement fails, the `COMMIT` fails, and nothing of the transaction is stored.

A function that opens a transaction must end it on every path. All database work is done within
the game pulse that starts it, so a mark still there in a later pulse means its transaction was
never ended. The layer then rolls it back itself and logs a `SYSERR`, so one missed `ROLLBACK`
cannot leave the connection refusing everyone's statements; until that next pulse it does refuse
them. At boot no pulse passes, and nothing heals.

**A statement the server refuses** (a constraint, a missing table) loses nothing: the connection
and the transaction go on, and the caller decides whether to roll back. The object and house
saves commit the rest and count as incomplete (`docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`). The one
exception is the deadlock above, where the refusal takes the transaction with it.

**A `COMMIT` without a reply.** A connection lost while the reply is on its way leaves the
outcome unknown: the server may have committed. `mysql_commit_transaction()` tells the three
endings apart and rolls back whatever is not confirmed:

| Result | Meaning | What the caller does |
| -- | -- | -- |
| `MYSQL_COMMIT_DONE` | The server confirmed it | Goes on |
| `MYSQL_COMMIT_REFUSED` | Not committed: the server refused it, or the transaction was already lost | Treats it as rolled back |
| `MYSQL_COMMIT_UNANSWERED` | No reply came | Writes again, or reads the database back |

Use it wherever memory is changed, or left alone, on the strength of the commit. Where the writes
set absolute values, write them again (cargo trades do, and the object save a character leaves
the game with). Otherwise read a row the transaction wrote, with `LOCK IN SHARE MODE`: the
locking read waits until the lost session's transaction has ended on the server, so it sees the
final state. Pet storage and retrieval, a hull's change of owner and the end of a vessel event do
this.

A reply is mostly lost to a server restart, and then the read-back cannot connect either. A site
must not take that for a rollback. Either the work can be done again without harm (the vessel
event's finish writes its scores only when its status write changed the event's row), or no state
is chosen until the row can be read (the pet keeper puts the owner's roster back to "restore
failed", and `pets restore` settles it from the rows).

**Session locks.** `GET_LOCK()` belongs to the session, so a reconnect drops it without telling
the holder. `help_sync_database_lock_held()` asks the server; the help writers call it inside
their transaction, where the session can no longer change unnoticed.

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

- Bind every data value (user, world, file, service, or database derived) with
  the `PREPARED_STMT` API. Escaping with `mysql_real_escape_string()` is a
  legacy compatibility technique, not the default abstraction.
- Select dynamic table, column, and ordering identifiers from compile-time
  allowlists; keep the values they operate on bound separately.
- Validate input data before database operations and implement proper access
  controls.
- `make test` runs `scripts/ci/check_sql_interpolation.py`, which fails when a
  source file gains a formatted SQL statement with `%` conversions beyond the
  recorded baseline in `scripts/ci/sql_interpolation_baseline.txt`. After
  migrating sites to bound statements, run the script with `--update`; it
  refuses to record growth, so the baseline only shrinks. Use `--list` to see
  the remaining inventory for #98.

### Connection Security

- Use secure passwords for database users
- Limit database user privileges
- Enable SSL connections when possible
- Monitor database access logs

---

*For additional database administration and advanced configuration, refer to the MySQL documentation and the [Troubleshooting Guide](../guides/TROUBLESHOOTING_AND_MAINTENANCE.md).*
