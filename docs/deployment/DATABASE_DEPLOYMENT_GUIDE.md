# Dynamic Descriptions & Wilderness Resource System
## Database Deployment Guide

## Quick Start

The easiest way to set up the database is using the automated deployment script:

```bash
# Full setup including database
./scripts/deploy.sh --auto --init-world

# Skip database if you want to configure it manually
./scripts/deploy.sh --quick --skip-db --init-world
```

The deployment script automatically:
- Creates the database and user (prompts for MariaDB root password)
- Executes the in-engine database initializer so every wilderness/resource table exists—no external SQL files required
- Applies fresh credentials to `lib/mysql_config` (mode 600)
- Sets up all required permissions
- Seeds required lookup tables such as `path_types` so wilderness paths display correctly on first boot

You can re-run the script at any time; it recreates credentials and reimports the schema without dropping existing data.

## Overview
This deployment guide covers database setup for:
- **Dynamic wilderness descriptions** with weather integration
- **Complete wilderness resource system** with depletion tracking  
- **Ecological interdependencies** and cascade effects
- **Player conservation tracking** and environmental stewardship
- **Regional effects system** for customizable area bonuses
- **Material subtypes system** for detailed resource varieties

## Prerequisites
- MySQL 5.7+ or MariaDB 10.2+
- Database user with CREATE, INSERT, UPDATE, DELETE privileges
- Existing Luminari MUD database (or use deploy.sh to create one)

**Note**: The MUD now supports graceful degradation without MySQL - it will run without database features if MySQL is not available.

## Installation Steps

### 1. Initialize via Admin Command
- Start `./bin/circle -d lib` and log in as an implementor (or use the staff console).
- Run `database init` to execute the full initializer. To refresh only the wilderness stack you can use targeted commands such as:

```
db_init_system wilderness   # Resource depletion, conservation, regional effects
db_init_system region       # Spatial tables, triggers, and indexes
db_init_system vessels      # Ship persistence tables and procedures
db_init_system pubsub       # Messaging queues (if you skipped them earlier)
```

### 2. Verify Installation
- Inside the MUD use `database verify` to run the integrity checks, or
- From MariaDB run sanity queries such as `SHOW TABLES LIKE 'resource_%';` and `SELECT COUNT(*) FROM resource_types;`

### 3. Enable in Code
Make sure your `src/campaign.h` includes:
```c
#define ENABLE_DYNAMIC_RESOURCE_DESCRIPTIONS
```

## Database Tables Created

### Core Resource System
- `resource_types` - Defines the 10 resource categories
- `resource_depletion` - Tracks depletion at each coordinate
- `player_conservation` - Player environmental stewardship scores
- `resource_statistics` - Server-wide resource usage analytics

### Ecological Interdependencies
- `resource_relationships` - How resources affect each other
- `ecosystem_health` - Overall ecosystem status per location
- `cascade_effects_log` - History of ecological cascade effects
- `player_location_conservation` - Detailed per-location tracking

### Regional Effects
- `region_effects` - Available effect types
- `region_effect_assignments` - Which regions have which effects

### Spatial Path Network
- `path_data` - Path definitions with linestring geometry
- `path_index` - Spatial index supporting path queries
- `path_types` - Glyph definitions and metadata for each path type (auto-seeded at startup)

### Weather & Descriptions
- `weather_cache` - Performance optimization for weather
- `room_description_settings` - Per-room customization options

### Material Subtypes
- `material_categories` - Categories within resource types
- `material_subtypes` - Specific materials (e.g., "oak wood", "iron ore")
- `material_qualities` - Quality levels (poor, common, rare, etc.)

### Companion & Pet Data
- `pet_data` - Persists charmed companions and summons for player accounts (stats, descriptions, HP)
- `pet_save_objs` - Stores equipment for saved pets tied to `pet_data` rows

### Analytics & Performance
- `ecosystem_analysis` - View for ecosystem health analysis
- `resource_availability_summary` - Resource scarcity overview
- `player_conservation_ranking` - Player environmental ranking

## Key Features

### Resource Depletion
- Resources decrease with harvesting
- Natural regeneration over time
- Cascade effects between resource types
- Player conservation scores affect regeneration

### Weather Integration
- Coordinate-based weather using Perlin noise
- Weather-specific descriptions
- Different systems for wilderness vs. non-wilderness

### Ecological Realism
- 15 predefined ecological relationships
- Resources affect each other when harvested
- Ecosystem health tracking
- Biodiversity and stability indexes

### Regional Customization
- Apply effects to specific zones/regions
- Resource bonuses/penalties
- Weather pattern modifications
- Description enhancements

## Testing Commands

Once deployed, use these admin commands for testing:

### Time Control
```
settime 14          # Set to 2 PM
settime 22 15       # Set to 10 PM on day 15
settime 6 1 0       # Set to 6 AM, day 1, month 0
```

### Weather Control
```
setweather 0        # Clear weather
setweather 2        # Rainy weather  
setweather 4        # Lightning storms
```

**Note**: Weather commands work differently in wilderness vs. non-wilderness areas.

## Maintenance

### Automatic Cleanup
The script includes a `CleanupOldLogs()` procedure that can be run periodically:
```sql
CALL CleanupOldLogs();
```

This removes:
- Regeneration logs older than 30 days
- Cascade effect logs older than 7 days  
- Expired weather cache entries
- Updates resource statistics

### Performance Monitoring
Use the analytics views to monitor system performance:
```sql
SELECT * FROM ecosystem_analysis WHERE health_state = 'degraded';
SELECT * FROM resource_availability_summary WHERE critical_locations > 0;
SELECT * FROM player_conservation_ranking LIMIT 10;
```

## Configuration Options

### Resource Regeneration Rates
Modify in `resource_types` table:
```sql
UPDATE resource_types SET regeneration_rate = 0.005 WHERE resource_name = 'vegetation';
```

### Ecological Relationships
Add new relationships in `resource_relationships`:
```sql
INSERT INTO resource_relationships 
(source_resource, target_resource, effect_type, effect_magnitude, description) 
VALUES (1, 2, 'depletion', -0.040, 'Mining affects water quality');
```

### Regional Effects
Create custom effects in `region_effects`:
```sql
INSERT INTO region_effects (effect_name, effect_type, effect_description, effect_data)
VALUES ('Ancient Forest', 'resource', 'Mystical wood bonuses', 
        JSON_OBJECT('resource_modifiers', JSON_OBJECT('wood', JSON_OBJECT('multiplier', 2.0))));
```

## Troubleshooting

### Common Issues
1. **Foreign key errors**: Ensure you're using the correct database name
2. **Permission errors**: Database user needs CREATE/ALTER privileges  
3. **JSON support**: Requires MySQL 5.7+ or MariaDB 10.2+
4. **Partial index errors**: The script uses standard indexes compatible with MariaDB/MySQL (no WHERE clauses in CREATE INDEX)

### Verification Queries
```sql
-- Check if all tables were created
SHOW TABLES LIKE '%resource%';

-- Verify resource types were inserted
SELECT COUNT(*) FROM resource_types; -- Should return 10

-- Check ecological relationships
SELECT COUNT(*) FROM resource_relationships; -- Should return 15

-- Test weather integration
SELECT * FROM region_effects WHERE effect_type = 'weather';
```

## Support
- Check MUD logs for any database connection errors
- Use `settime` and `setweather` commands to verify functionality
- Monitor `resource_depletion` table for harvest tracking
- Review `ecosystem_health` for environmental impact tracking

The system is designed to be self-maintaining with automatic regeneration and cleanup procedures.
