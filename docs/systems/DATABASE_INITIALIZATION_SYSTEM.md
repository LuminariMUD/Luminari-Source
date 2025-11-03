# LuminariMUD Database Initialization System

## Overview

This system provides a complete, production-safe database initialization framework for LuminariMUD. It creates all necessary database tables if they don't exist and can populate them with essential reference data while preserving existing production data.

## ⚠️ CRITICAL SAFETY FEATURES

**This system is designed to be production-safe:**
- ✅ **NEVER overwrites existing data**
- ✅ **Checks for existing table data before populating**
- ✅ **Only adds data to newly created empty tables**
- ✅ **Never touches region_data, path_data, region_index, or path_index** (managed by wildedit)
- ✅ **Uses exact production database schema**
- ✅ **Safe to run on existing production databases**

## System Components

### Core Files

1. **`src/db_init.h`** - Header with all function prototypes
2. **`src/db_init.c`** - Table creation functions (34+ tables)
3. **`src/db_init_data.c`** - Safe data population functions
4. **`src/db_startup_init.c`** - Startup initialization
5. **`src/db_admin_commands.c`** - Admin commands for database management

### Database Tables Managed

**Account System:**
- account_data
- account_stats

**Player Data:**
- player_data (character information)
- player_saves (save data)

**Equipment & Items:**
- object_data
- weapon_data
- armor_data
- treasure_data

**Spells & Magic:**
- spell_data
- scroll_data
- spellbook_data

**World & Geography:**
- room_data
- zone_data
- shop_data
- help_data

**Clans & Organizations:**
- clan_data
- member_data

**Combat & Skills:**
- fight_data
- affect_data

**Crafting & Resources:**
- material_categories (with reference data)
- material_types
- resource_types (with reference data)
- resource_depletion
- resource_depletion_events

**Mail System:**
- mail_data
- mudmail_data

**Admin & Logging:**
- ban_data
- staff_events

**Region System (Spatial):**
- region_data (NEVER populated - managed by wildedit)
- path_data (NEVER populated - managed by wildedit)
- region_index (NEVER populated - managed by wildedit)
- path_index (NEVER populated - managed by wildedit)
- region_resource_effects
- region_type_resource_effects

## Usage

### Automatic Startup Initialization

The system automatically runs during MUD startup:
```c
// Called in boot_db() after MySQL connection
startup_database_init();
```

This performs essential table creation and safe data population.

### Manual Administration Commands

**Database Status Command:**
```
database status
```
Shows status of all database tables and reference data.

**Full Database Initialization:**
```
db_init_system
```
Performs complete database initialization (Level 34 Implementor only).

**Individual System Initialization:**
```
db_init_system accounts
db_init_system player
db_init_system equipment
db_init_system spells
db_init_system world
db_init_system clans
db_init_system combat
db_init_system resources
db_init_system region
db_init_system mail
db_init_system admin
```

## Safety Mechanisms

### Data Protection

Every data population function includes:
```c
// Check if table already has data
if (table_has_data(table_name)) {
    log("SYSINIT: %s already has data, skipping population.", table_name);
    return;
}
```

### Region System Special Handling

The region system is managed by the `wildedit` program:
```c
void populate_region_system_data() {
    log("SYSINIT: Region data (region_data, path_data, region_index, path_index) "
        "is managed by the wildedit program and should not be populated by this system.");
    // Explicitly does nothing - these tables are managed externally
}
```

### Production Schema Compliance

All table structures and INSERT statements use exact production database schema:
- Uses `resource_id` (not `id`) in resource_types table
- Uses `category_id` (not `id`) in material_categories table  
- Uses `vnum` (not `region_id`) in region_data table
- Preserves all existing column names and types

## Reference Data Included

### Material Categories
- Stone
- Wood
- Metal
- Crystal
- Fabric
- Leather
- Organic
- Magical
- Special

### Resource Types
- Stone (Granite, Marble, Obsidian, etc.)
- Wood (Oak, Pine, Mahogany, etc.)
- Metal (Iron, Steel, Mithril, etc.)
- Crystal (Quartz, Sapphire, Diamond, etc.)
- Magical (Dragonscale, Moonstone, etc.)

## Integration Points

### Build System
```makefile
# Added to Makefile.am
circle_SOURCES = ... db_init.c db_init_data.c db_startup_init.c db_admin_commands.c
```

### Command System
```c
// Added to interpreter.c command table
{"database", POS_DEAD, do_database, LVL_IMPL, 0},
{"db_init_system", POS_DEAD, do_db_init_system, LVL_IMPL, 0},
```

### Startup Sequence
```c
// Added to db.c boot_db() function
startup_database_init();
```

## Database Procedures

The system creates essential stored procedures:
- `bresenham_line()` - Spatial line algorithm for region pathfinding
- Additional spatial geometry procedures as needed

## Logging

All operations are logged with appropriate detail:
- Table creation attempts
- Data population results
- Safety check outcomes
- Error conditions

## Testing

The system has been tested with:
- ✅ Clean database initialization
- ✅ Existing production database preservation
- ✅ Partial table scenarios
- ✅ Build system integration
- ✅ Command execution

## Maintenance

### Adding New Tables
1. Add table creation to appropriate `init_*_tables()` function in `db_init.c`
2. Add data population to corresponding function in `db_init_data.c`
3. Include safety checks using `table_has_data()`
4. Update function prototypes in `db_init.h`

### Modifying Reference Data
1. Update population functions in `db_init_data.c`
2. Ensure safety checks remain in place
3. Test on development database first

## Production Deployment

This system is safe for production deployment:
1. Backup your database (standard precaution)
2. Deploy code with normal MUD restart
3. System will automatically initialize missing tables
4. Existing data remains untouched
5. Monitor logs for initialization results

The system provides a foolproof way to ensure your LuminariMUD database is properly set up and configured while maintaining complete safety for existing production data.
