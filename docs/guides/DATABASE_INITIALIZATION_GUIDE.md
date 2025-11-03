# LuminariMUD Database Initialization System

## Overview

This comprehensive database initialization system provides foolproof setup and management for all LuminariMUD database tables. The system is designed to automatically detect missing tables, create them with proper structure, and populate essential reference data.

## Features

- **Automatic Detection**: Identifies missing or incomplete database tables
- **Modular Initialization**: Separate functions for each major system
- **Reference Data Population**: Automatically populates standard game data
- **Admin Commands**: Complete command interface for database management
- **Startup Integration**: Automatic initialization during MUD boot
- **Integrity Verification**: Comprehensive table verification system

## Database Systems Supported

### Core Systems
1. **Player System** - Player data, accounts, unlocked races/classes
2. **Object Database** - Item exports, wear slots, bonuses
3. **Housing System** - Player house data storage
4. **Help System** - Help entries and keyword search

### Advanced Systems
5. **Wilderness Resource System** - Complete ecosystem with 16 interconnected tables
6. **Region System** - Spatial regions, paths, and geographic data with indexes  
7. **AI Service System** - AI configuration, caching, and NPC personalities  
8. **Crafting System** - Supply orders and material tracking

## Installation

### 1. Add Files to Your Build

Copy these files to your `src/` directory:
- `db_init.h` - Header file with function prototypes
- `db_init.c` - Main initialization functions  
- `db_init_data.c` - Reference data population functions
- `db_startup_init.c` - Startup integration functions
- `db_admin_commands.c` - Admin command implementations

### 2. Update Your Makefile

Add the new object files to your Makefile:
```makefile
OBJFILES = ... db_init.o db_init_data.o db_startup_init.o db_admin_commands.o
```

### 3. Add Includes

In your main header files (typically `structs.h` or `db.h`), add:
```c
#include "db_init.h"
```

### 4. Update Command Table

In your command interpreter (typically `interpreter.c`), add these commands:
```c
{ "database"       , POS_DEAD    , do_database      , LVL_GRGOD, 0 },
{ "db_init_system" , POS_DEAD    , do_db_init_system, LVL_IMPL , 0 },
{ "db_info"        , POS_DEAD    , do_db_info       , LVL_GRGOD, 0 },
{ "db_export_schema", POS_DEAD   , do_db_export_schema, LVL_IMPL, 0 },
```

### 5. Add Startup Integration

In your main boot sequence (typically in `main.c` or `db.c`), add:
```c
/* After MySQL connection is established */
startup_database_init();
```

Optional: Add periodic health checking in your main game loop:
```c
/* In your main game loop */
periodic_database_health_check();
```

## Usage

### Automatic Initialization

The system will automatically initialize missing tables during MUD startup. If your database is empty or incomplete, you'll see:

```
=== LuminariMUD Database Startup Initialization ===
INFO: Database connection detected - verifying setup...
WARNING: Database tables missing or incomplete
INFO: Attempting automatic database initialization...
Initializing core player system tables...
Initializing object database system tables...
[... etc ...]
SUCCESS: Database initialized successfully during startup
=== Database Startup Initialization Complete ===
```

### Manual Administration

#### Check Database Status
```
database status
```
Shows connection status and table verification for all systems.

#### Initialize All Tables
```
database init
```
Creates all missing tables and populates reference data (requires LVL_IMPL).

#### Initialize Specific Systems
```
db_init_system wilderness
db_init_system regions
db_init_system ai  
db_init_system player
```
Initialize only specific subsystems (requires LVL_IMPL).

#### Verify Database Integrity
```
database verify
```
Checks that all required tables exist and are accessible.

#### Reset Reference Data
```
database resetdata
```
Repopulates standard reference data without affecting user data (requires LVL_IMPL).

#### Get Database Information
```
db_info tables    # List all tables
db_info counts    # Show row counts for important tables
db_info sizes     # Show table sizes in MB
```

#### Export Schema
```
db_export_schema
```
Exports complete database structure to `db_schema_export.sql` for backup/documentation.

## Database Tables Created

### Core Player System (7 tables)
- `player_data` - Core character information
- `account_data` - Account management
- `unlocked_races` - Account unlocked races
- `unlocked_classes` - Account unlocked classes
- `player_save_objs` - Character inventory saves
- `player_save_objs_sheathed` - Sheathed weapon saves
- `pet_save_objs` - Pet inventory saves

### Object Database System (3 tables)
- `object_database_items` - Exported item data
- `object_database_wear_slots` - Item wear locations
- `object_database_bonuses` - Item stat bonuses

### Wilderness Resource System (16 tables)
- `resource_types` - Resource definitions (vegetation, minerals, etc.)
- `resource_depletion` - Location-based resource tracking
- `player_conservation` - Player environmental scores
- `resource_statistics` - Global resource statistics
- `resource_regeneration_log` - Regeneration history
- `resource_relationships` - Ecological interdependencies
- `ecosystem_health` - Ecosystem state tracking
- `cascade_effects_log` - Environmental cascade effects
- `player_location_conservation` - Location-specific tracking
- `region_effects` - Environmental effects
- `region_effect_assignments` - Region-effect mappings
- `weather_cache` - Weather data cache
- `room_description_settings` - Room customization
- `material_categories` - Material classification
- `material_subtypes` - Specific material types
- `material_qualities` - Quality levels (poor, excellent, etc.)

### AI Service System (4 tables)
- `ai_config` - AI service configuration
- `ai_requests` - Request logging and analytics
- `ai_cache` - Response caching
- `ai_npc_personalities` - NPC personality data

### Other Systems
- `supply_orders_available` - Crafting supply orders
- `house_data` - Housing system data
- `help_entries` - Help system content
- `help_keywords` - Help search keywords

## Reference Data Populated

The system automatically populates essential reference data:

### Resource Types (10 standard resources)
- Vegetation, Minerals, Water, Herbs, Game, Wood, Stone, Crystal, Clay, Salt
- Each with appropriate rarity and regeneration rates

### Material Qualities (6 quality levels)
- Poor, Common, Good, Excellent, Masterwork, Legendary
- With appropriate rarity and value multipliers

### Material Categories (9 categories)
- Plant Fibers, Metal Ores, Precious Stones, Animal Parts, Wood Types, 
- Textile Materials, Alchemical Components, Stone Materials, Building Stone

### Regional Effects (10 standard effects)
- Weather effects (rain, storms, drought)
- Geological effects (fertile soil, mineral veins)
- Seasonal effects (spring bloom, autumn harvest)
- Biological effects (wildlife migration, ancient groves)

### Ecological Relationships (15 predefined relationships)
- Complex interdependencies between resource types
- Enhancement and depletion relationships
- Realistic ecosystem modeling

### AI Configuration
- Default settings for AI service
- Disabled by default for security
- Standard rate limits and configuration

## Troubleshooting

### Common Issues

**"Database not available"**
- Check MySQL connection in `lib/mysql_config`
- Verify MySQL server is running
- Check database credentials

**"Permission test failed"**
- Database user needs CREATE, INSERT, UPDATE, DELETE privileges
- Grant permissions: `GRANT ALL ON luminari.* TO 'user'@'localhost';`

**"Table creation failed"**
- Check database user has CREATE privilege
- Verify database exists
- Check MySQL error logs

**"Foreign key constraint fails"**
- Ensure referenced tables exist first
- Check foreign key relationships
- May need to create tables in specific order

### Reset Everything
If you need to completely reset the database:
```sql
-- In MySQL command line
DROP DATABASE luminari;
CREATE DATABASE luminari;
```
Then use `database init` in the MUD to recreate everything.

### Selective Reset
To reset only specific systems, drop the relevant tables and run:
```
db_init_system <system_name>
```

## Security Notes

- Database initialization requires LVL_IMPL access
- Reference data reset requires LVL_IMPL access  
- Status/verification available to LVL_GRGOD+
- All database operations are logged
- No user data is modified during reference data reset

## Performance Considerations

- Tables include appropriate indexes for performance
- Large wilderness systems may take time to initialize
- Periodic health checks are lightweight (every 5 minutes)
- Export operations may take time on large databases

## Integration with Existing Code

This system is designed to work alongside existing database code:

- Uses existing `mysql_query_safe()` functions
- Respects existing MySQL connection management
- Does not modify existing table structures
- Safe to run multiple times (uses `IF NOT EXISTS`)

## Future Expansion

To add new database systems:

1. Create initialization function in `db_init.c`
2. Add verification function 
3. Add to `init_luminari_database()` call list
4. Add to `verify_database_integrity()` check list
5. Update admin commands as needed

The modular design makes it easy to add new database systems while maintaining the existing functionality.
