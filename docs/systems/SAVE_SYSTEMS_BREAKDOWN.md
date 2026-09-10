# Luminari MUD Save Systems - Comprehensive Breakdown

## Overview

Luminari MUD uses a hybrid save system combining **ASCII file storage** for primary game data and **MySQL database storage** for advanced features, analytics, and backup systems. This architecture provides both reliability and performance optimization.

## System Categories

### File-Only Save Systems

These systems use file-based storage exclusively for persistence.

#### 1. Player Character Data (`.plr` files)
- **Location**: `lib/plrfiles/A-E/playername.plr`
- **Format**: ASCII text with structured fields
- **Function**: `save_char()` in `players.c`
- **Contains**:
  - Basic character info (name, level, class, race)
  - Abilities and statistics
  - Equipment and inventory references
  - Spells and skills
  - Player preferences and settings
  - Quest and achievement data
  - Character flags and temporary effects
  - DG Script variables

**Example Structure:**
```
Name: PlayerName
SexC: 1
Clas: 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39
Race: 0
Levl: 1
```

#### 2. Player Object Files (`.objs` files)
- **Location**: `lib/plrobjs/A-E/playername.objs`
- **Format**: Custom binary/text format
- **Functions**: `Crash_save()`, `Crash_rentsave()` in `objsave.c`
- **Contains**:
  - Equipment worn by character
  - Inventory items
  - Bag contents (10 bags per player)
  - Object properties and modifications
  - Rent information and costs

#### 3. Clan System
- **Location**: `lib/etc/clans`
- **Format**: Custom text format
- **Functions**: `save_clans()`, `save_single_clan()` in `clan_edit.c`
- **Contains**:
  - Clan membership data
  - Clan rankings and hierarchy
  - Clan treasury and resources
  - Clan diplomacy settings
  - Zone claims and territories

**Key Note**: Clans explicitly do NOT use MySQL storage.

#### 4. House/Rent System
- **Location**: `lib/house/`
- **Format**: Custom rent file format
- **Functions**: `House_save()`, `House_crashsave()` in `house.c`
- **Contains**:
  - House contents and objects
  - Rent costs and timeouts
  - House control data
  - Private storage items

#### 5. World Data Files
- **Location**: `lib/world/`
- **Format**: CircleMUD standard formats
- **Files**:
  - `.wld` - Room definitions
  - `.mob` - Mobile (NPC) definitions
  - `.obj` - Object prototypes
  - `.zon` - Zone reset commands
  - `.shp` - Shop definitions
  - `.qst` - Quest definitions

#### 6. Mail System
- **Location**: `lib/mail/`
- **Format**: Custom mail format
- **Functions**: Mail handling in `mail.c`

#### 7. Configuration Files
- **Files**: Various `.conf`, `config.*` files
- **Purpose**: Server configuration and settings

---

### MySQL-Only Save Systems

These systems use MySQL database storage exclusively.

#### 1. Account System
- **Table**: `account_data`
- **Function**: `save_account()` in `account.c`
- **Contains**:
  - Account credentials
  - Account-wide experience
  - Email addresses
  - Character name associations
  - Unlocked races and classes

#### 2. Persistent Followers (Pets, Summons, and Cohorts)

- **Tables**: `pet_data`, `pet_save_objs`
- **Functions**: `save_char_pets()` and `load_char_pets()` in `src/players.c`;
  recursive object storage in `src/obj/objsave.c`
- **Contents**: One base row per saved charmed follower, serialized runtime state,
  and its equipped, carried, and nested objects

`save_char_pets()` replaces the active snapshot for one owner. It prepares
every follower row before starting the transaction, then replaces active pet and
object rows together. Stored rows remain intact. Existing `pet_data_id` values
survive replacement; `owner_id` and `owner_created` bind them to the pfile owner,
including across a rename. SQL player IDs are not interchangeable with pfile IDs.
A known query failure rolls back the replacement. An uncertain COMMIT outcome
still needs reconciliation; a transaction does not make pfiles, world items,
and SQL jointly crash-atomic.

Before copyover closes sockets or writes its handoff file, `save_player_pets()`
in `src/limits.c` snapshots players in the world, including linkdead owners.
A failed snapshot cancels copyover and leaves live pets and equipment available
for retry. NPCs and roomless menu characters are skipped. An incomplete pet
restore prevents snapshot replacement. Unchanged successful snapshots retain
the fingerprint shortcut.

Companion call, Mummy Dust, and Dragon Knight cooldown events are preserved by
the native durable character-event save rather than cancelled before copyover.
This is separate from each pet's finite lifetime or control-break event.

The startup migration runner checks versions `2026080501` through `2026091007`
on every boot, including when both tables already exist. It is the only schema
authority: the runtime `CREATE TABLE` statements in `init_core_player_tables()`
and `sql/master_schema.sql` describe the same base shape, and any change to the
contract is a new migration rather than an edit to either copy. The cascading
`pet_save_objs.pet_idnum -> pet_data.pet_data_id` foreign key is created only
by migration `2026091007`. MariaDB refuses to modify a constrained column, so a
base table that already carried the key would block the earlier migrations
from replaying on a fresh install. Startup verifies the InnoDB engines,
required column types, nullability, and unsigned identifiers, primary keys,
owner/relation indexes, the foreign key with both rules set to `CASCADE`, and
the migration version before loading world data. A failed migration or
contract check stops the boot instead of allowing gameplay against an
incompatible schema.

Saved objects belong to their pet row. The loader selects them by `pet_idnum`
alone; the `owner_name` column on an object row is display data and takes no
part in lookup, so a renamed owner keeps every pet's inventory. Both save paths
delete replaced object rows by pet identity before the pet row, and the foreign
key removes anything that statement could miss. Rows whose pet no longer
existed were purged by migration `2026091005` before the constraint was added.
InnoDB refuses foreign keys on temporary tables, so the temporary fixtures in
the test suite model the tables without it; the cascade and the orphan
rejection are exercised against the live tables instead.

Pet object loading never terminates the server. A query or result failure, an
unknown or unreadable prototype, a partial numeric field, an unclosed nested
container, a doubly claimed wear slot, or an out-of-range location all return
`PET_OBJECT_LOAD_FAILED` after discarding every object decoded so far;
`load_char_pets()` then drops that follower, marks the roster restore failed,
and refuses to replace the stored snapshot until a complete restore succeeds.
`handle_obj()` only moves objects between the inventory, a worn slot, and a
container; a container missing from the row set returns its contents to the
inventory rather than failing. The `exit()` calls in `src/obj/objsave.c` sit in
the player, house, and sheath loaders and are not reachable from a pet restore.

Owner rewrite churn was measured before choosing between dirty-record saves and
batching. A local snapshot held 709 pet rows across 370 owners, averaging under
two pets per owner with a maximum of twenty, and no pet carried more than 17
object rows or two kilobytes of serialized objects. `save_char_pets()` already
skips an unchanged owner through the fingerprint shortcut, so a rewrite only
runs when something changed and then touches at most a few dozen rows inside
one transaction. Per-row dirty tracking or statement batching would add state
to reconcile on rollback without a measurable saving at that scale, so whole
owner replacement stays the behavior.

Failures use bounded, rate-limited operation, owner, pet VNUM, MariaDB, and
schema-version context. Do not restore full SQL-payload logging: pet text and
serialized runtime state belong to player data, not diagnostic output.

`load_char_pets()` rebuilds active followers from their mobile prototypes, saved
fields, runtime state, and validated object rows. Keeper retrieval reuses the
same row decoder. Each pet and its inventory are prepared outside any room;
keeper retrieval commits the stored-to-active transition before placement,
following, and mobile load triggers. Known activation/commit failures discard the
roomless copy and retain the stored record and inventory. Login decodes every
active row before publishing any of them: one undecodable row keeps the whole
roster unpublished and retained. The staged roster is then ordered
keeper-eligible first, then timed, oldest `pet_data_id` first, and
`select_restorable_followers()` in `src/utils.c` admits it first-fit against the
same category accounting used by live admission. Rejected keeper-eligible rows
are moved to `PET_STATE_STORED` in one transaction before anything is published,
so the next active snapshot cannot drop them; a rejected timed follower is spent
and its row leaves with the next snapshot. Keeper reclaim applies the same check
to the staged pet, so classification uses the saved source and flags rather than
the prototype. Post-publication callback reconciliation remains open. Legacy rows with a NULL runtime state retain the compatibility
load path. Custom names use the existing name/short/long text fields.
`pets <pet|#id> name <name>` stages the new strings and restores the old pointers
on known save failure. Prototype keywords remain available for targeting;
repeated renaming does not accumulate prior custom names. Saved eidolon identity
is restored before considering legacy owner-description defaults. Malformed
records remain saved for recovery; `pets restore` offers a bounded retry and skips already published pet IDs.

Follower persistence follows an explicit policy (`pet_lifetime_kind()` in
`src/utils.c`). Durable followers persist until dismissed, killed, or stored.
Timed control is carried by the saved charm affect; its remaining duration
pauses while stored or offline. Deadline followers are those with a live
`ePURGEMOB` event, plus illusory decoys, which never persist without one. The
runtime-state record (version 4, `T <kind> <epoch>`) stores the absolute
real-time deadline rather than the scheduler handle. The deadline keeps
elapsing offline: restore rejects an expired record, discards the roomless
copy, and lets the next snapshot drop the row; a live record gets a fresh
native `ePURGEMOB` event for the remaining time; if that event cannot be
scheduled the record is rejected rather than restored without an expiry.
Session followers are spell summons outside the kept families (companion,
familiar, mount, dragon mount, eidolon, golem, mercenary, animated dead,
lycanthrope, totem spirit); they are skipped by the snapshot and a saved record
of one is rejected on restore. The keeper boards only durable and timed-control
followers; reclaiming a stored row whose lifetime has ended deletes that row
and its objects inside the reclaim transaction. `pets` shows each pet's
policy and remaining real time. Expiry gear handling and uncertain-commit
reconciliation remain open in `docs/ongoing-projects/PET_SYSTEM_REFACTOR_PLAN.md`.
Save/load tests do not replace the plan's executable copyover acceptance gate.

#### 3. Wilderness System Data
- **Tables**: `region_data`, `path_data`, `region_index`, `path_index`
- **Functions**: `load_regions()`, `load_paths()` in `mysql.c`
- **Contains**:
  - Geographic regions with polygon boundaries
  - Path data (roads, rivers) with linestring geometry
  - Spatial indexes for performance
  - Wilderness configuration

#### 4. Game Statistics and Analytics
- **Tables**: Multiple analytics tables
- **Contains**:
  - Combat logs and statistics
  - Player behavior tracking
  - Performance metrics
  - Economy data

#### 5. Object Database (Analytics)
- **Tables**: `object_database_*` series
- **Function**: `save_objects_to_database()` in `db.c`
- **Purpose**: Complete object catalog for analysis
- **Contains**:
  - All object prototypes and their properties
  - Bonus breakdowns
  - Wear slot information
  - Object flags and affects

#### 6. Template System
- **Tables**: Template-related tables
- **Functions**: Various in `templates.c`
- **Contains**:
  - Character build templates
  - Pre-made character configurations

---

### Hybrid Save Systems (Both File and MySQL)

These systems maintain data in both locations for redundancy or different purposes.

#### 1. Player Object Storage (Optional Database Backup)
- **Primary**: File-based (`.objs` files)
- **Backup**: MySQL (`player_save_objs` table)
- **Control**: `#ifdef OBJSAVE_DB` in `objsave.c`
- **Purpose**: Database backup provides additional security and analytics

**Implementation:**
```c
#ifdef OBJSAVE_DB
  // Database transaction for object saving
  if (mysql_query(conn, "start transaction;")) {
    // Handle error
  }
  // Save to database
  if (mysql_query(conn, "commit;")) {
    mysql_query(conn, "rollback;");
  }
#endif
```

#### 2. Player Character Data (Backup System)
- **Primary**: ASCII files (`.plr`)
- **Backup**: MySQL (`player_data` table)
- **Purpose**: Database provides backup and enables advanced queries

#### 3. House Data (Partial Overlap)
- **Files**: House contents and crash data
- **Database**: House control information and indexes
- **Tables**: `house_data`

---

## Save Triggers and Frequency

### Automatic Save Events

1. **Character Auto-Save**
   - Triggered every 5 minutes (configurable)
   - On level gain, important events
   - On logout/quit

2. **Persistent Follower Snapshot**
   - Saves playing characters' follower snapshots once per minute
   - Important summon, charm, dismissal, quit, idle, and administrative paths
     save at their lifecycle boundary

3. **Object Crash-Save**
   - When `PLR_CRASH` flag is set
   - Equipment changes, inventory modifications
   - Rent/quit situations

4. **House Auto-Save**
   - Periodic saves via `House_save_all()`
   - On house modifications

5. **Account Auto-Save**
   - On account changes
   - Character creation/deletion

### Manual Save Commands

- `save` - Force character save
- `saveall` - Save all connected players (admin)
- `clanset save` - Force clan data save

---

## Performance Optimizations

### File System Optimizations

1. **Buffered I/O**: 64KB write buffers in `save_char()`
2. **Directory Organization**: Files organized A-E by first letter
3. **Atomic Writes**: Temporary files with rename operations

### Database Optimizations

1. **Connection Pooling**: Persistent MySQL connections
2. **Prepared Statements**: Used for frequent queries
3. **Spatial Indexes**: For wilderness region/path data
4. **Transaction Handling**: Proper rollback on errors

---

## Error Handling and Recovery

### File System Recovery
- Backup files (`.bak`, `.orig`)
- Crash recovery for incomplete saves
- File locking mechanisms

### Database Recovery
- Transaction rollbacks on errors
- Connection recovery and reconnection
- Data validation before commits

---

## Configuration

### File Storage Settings
```c
#define PLR_FILE     0    // Player files
#define CRASH_FILE   1    // Object crash files
#define ALIAS_FILE   2    // Player aliases
```

### Database Integration
```c
#define OBJSAVE_DB 1      // Enable database object backup
```

### Performance Tuning
- Buffer sizes for file operations
- Database connection parameters
- Auto-save intervals

---

## System Dependencies

### File Dependencies
- Proper file permissions
- Adequate disk space
- Directory structure integrity

### Database Dependencies
- MySQL server availability
- Spatial extension support
- Proper table schemas and indexes

---

## Development Guidelines

### Adding New Save Data

1. **File-Based Data**: Follow ASCII format conventions
2. **Database Data**: Use proper transactions and error handling
3. **Hybrid Data**: Implement both systems with fallback logic

### Performance Considerations
- Use buffered I/O for large saves
- Implement proper indexing for database queries
- Consider save frequency vs. performance impact

### Error Handling
- Always implement proper cleanup
- Use transactions for database operations
- Log errors appropriately for debugging

---

## Backup and Maintenance

### File Backup Strategy
- Regular file system backups
- Player file rotation policies
- Log file management

### Database Backup Strategy
- MySQL dump procedures
- Point-in-time recovery capabilities
- Spatial data backup considerations

---

This comprehensive breakdown shows how Luminari MUD's hybrid save system provides both reliability through file-based storage and advanced functionality through database integration, ensuring data persistence while enabling modern MUD features like spatial wilderness systems and detailed analytics.
