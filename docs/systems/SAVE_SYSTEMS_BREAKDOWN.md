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

`save_char_pets()` replaces the complete snapshot for one owner. It prepares
every follower row before starting the transaction, then deletes the old linked
rows and inserts the new pet and object rows in one transaction. A failed delete,
pet insert, recursive object save, or commit rolls back the whole replacement;
callers must not treat an in-memory follower as proof that the snapshot was saved.

The startup migration runner checks versions `2026080501` through `2026080504`
on every boot, including when both tables already exist. Startup verifies the
InnoDB engines, required column types and nullability, primary keys,
owner/relation indexes, and migration version before loading world data. A
failed migration or contract check stops the boot instead of allowing gameplay
against an incompatible schema.

Failures use bounded, rate-limited operation, owner, pet VNUM, MariaDB, and
schema-version context. Do not restore full SQL-payload logging: pet text and
serialized runtime state belong to player data, not diagnostic output.

`load_char_pets()` rebuilds the follower from its mobile prototype, saved fields,
runtime state, and object rows. Legacy rows with a NULL runtime state retain the
compatibility load path. A nonempty malformed runtime record is rejected rather
than applied to the follower.

A saved follower is reconstructed as an ordinary mobile with ownership/charm
state; flags alone do not establish an explicit durable companion category.
The runtime-state record does not preserve `ePURGEMOB` expiry. Timed affects
and event-based lifetime are separate: preserving one does not restore the
other. Track explicit durable categories and temporary expiry in [#119](https://github.com/LuminariMUD/Luminari-Source/issues/119),
and remaining schema/identity work in [#120](https://github.com/LuminariMUD/Luminari-Source/issues/120).

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
