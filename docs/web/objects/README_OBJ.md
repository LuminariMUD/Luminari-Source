# Object Database - LuminariMUD Web Portal

This directory contains the static HTML/JavaScript object database for LuminariMUD, allowing players and builders to search and browse all equipment, weapons, and items in the game.

## Features

- **Interactive Search**: Search by object name, type, material, zone, and more
- **Advanced Filtering**: Filter by level requirements, wear slots, bonuses, weapon groups
- **Collapsible Categories**: Objects organized by type with expandable sections
- **Detailed Information**: View stats, bonuses, flags, and special abilities
- **Mobile Responsive**: Works on all screen sizes
- **Fast Performance**: Client-side filtering for instant results

## Files

- `index.html` - Main interface (static HTML + embedded JavaScript)
- `../data/objects.json` - JSON data file containing all object data

## How to Update the Object Data

The object data is generated from the LuminariMUD MySQL database using a Python export script.

### Prerequisites

1. **Python 3** with **pymysql** module:
   ```bash
   sudo apt install python3-pymysql
   ```

2. **Database access** to the LuminariMUD object_database tables

3. **Database credentials** set via environment variables or `mysql.php` config file

### Generating the JSON Data

#### Method 1: Using mysql.php (Recommended)

If you have the `docs/TODO/objectdb/mysql.php` file with your database credentials:

```bash
python3 util/export_objectdb_to_json.py
```

The script will automatically read credentials from `mysql.php`.

#### Method 2: Using Environment Variables

Set credentials as environment variables:

```bash
export DB_HOST=localhost
export DB_USER=your_username
export DB_PASSWORD=your_password
export DB_NAME=your_database

python3 util/export_objectdb_to_json.py
```

#### Method 3: Custom Output Location

Specify a custom output file:

```bash
python3 util/export_objectdb_to_json.py /path/to/output.json
```

### Default Output Location

By default, the script exports to:
```
docs/web/data/objects.json
```

## Database Schema

The object database uses the following MySQL tables:

### object_database_items (Expected Schema)
The web interface expects these columns:
- `idnum` - Unique identifier
- `object_vnum` - Virtual number (in-game ID)
- `object_name` - Display name
- `object_type` - Category (Weapon, Armor/Shield, etc.)
- `specific_type` - Subtype (long sword, plate armor, etc.)
- `material` - Material type (string)
- `zone_name` - Source zone
- `minimum_level` - Level requirement
- `enhancement_bonus` - Magic bonus (+1, +2, etc.)
- `cost` - Gold cost
- `weapon_spell_1`, `weapon_spell_2`, `weapon_spell_3` - Spell procs
- `weapon_special_ability` - Special abilities

### Related Tables (Expected)
- `object_database_wear_slots` - Where the object can be worn
- `object_database_bonuses` - Stat bonuses and their types
- `object_database_obj_flags` - Object flags (magic, no-drop, etc.)
- `object_database_perm_affects` - Permanent effects/sets

### Current Database Schema (As of Nov 2025)

**Note**: The current `luminari` database has a **simpler schema** with integer codes:
- `item_type` (integer) instead of `object_type` (string)
- `material` (integer) instead of material name (string)
- **Missing columns**: `zone_name`, `enhancement_bonus`, `weapon_spell_*`, `weapon_special_ability`
- **Missing tables**: `object_database_obj_flags`, `object_database_perm_affects`
- **Database is currently empty** (0 objects)

To populate the web interface with real data, you'll need to either:
1. **Extend the database schema** with the additional columns/tables (recommended)
2. **Update the export script** to convert integer codes to readable strings and generate missing data

## Filtered Items

The following zones are excluded from the public database:
- Code Items (DO NOT EDIT)
- Builder Academy Zone
- Airship / Carriage Rooms
- Clan Halls
- PLAYER PORT RESTRINGS
- PP Equipment
- PP only items
- PP standard eq for newbies
- PP Unique Zone Only
- QUESTS, PP ONLY
- Uniques Zone
- Unused Zone
- Zone 166

Items with the "Mold" flag are also excluded.

## Search Features

### Basic Search
- **Object Name**: Text search (partial matching)
- **Object Type**: Weapon, Armor/Shield, Container, etc.
- **Specific Type**: Depends on Object Type selected (long sword, plate armor, etc.)
- **Material**: Iron, Steel, Mithril, etc.
- **Zone**: Filter by source zone

### Advanced Filters
- **Weapon Group**: Axe, Blade, Monk, Polearm, Ranged, etc.
- **Level Range**: Min/Max level requirements
- **Wear Slot**: Filter by equipment slot
- **Bonus Type**: Filter by bonus properties (Strength, Hitroll, AC, etc.)

### Weapon Groups
Predefined weapon groups for easy filtering:
- **Axe**: Throwing axe, battle axe, great axe, etc.
- **Double-Weapon**: Two-bladed weapons
- **Hammer**: Maces, clubs, warhammers, flails
- **Heavy-Blade**: Long sword, great sword, scimitar
- **Light-Blade**: Dagger, rapier, short sword, whip
- **Monk**: Monk weapons (quarterstaff, kama, nunchaku)
- **Polearm**: Spear, halberd, lance, trident
- **Ranged**: Bows, crossbows, slings

## Display Format

### Object Rows
Each object displays:
- **VNUM**: Virtual number for builders
- **Level**: Minimum level to equip
- **Type**: Object category
- **Name**: Display name (with enhancement bonus if applicable)
- **Wear Slots**: Where it can be equipped
- **Price**: Gold cost

### Detailed Information
Expanding each row shows:
- **Bonuses**: Stat bonuses with type and modifier
- **Flags**: Object flags (Magic, Glow, etc.)
- **Sets**: Permanent affects/set bonuses
- **Spell Procs**: Spell effects on hit
- **Special Abilities**: Weapon special abilities
- **Material**: What it's made of
- **Zone**: Where it can be found

## Integration with Web Portal

The object database is linked from the main documentation portal at `/web/index.html` under the "Game Resources" section.

## Future Enhancements

Potential improvements:
- [ ] Export to CSV/PDF
- [ ] Comparison tool (compare two objects side-by-side)
- [ ] Advanced statistics (most common materials, level distribution)
- [ ] Direct links to specific objects
- [ ] Bookmark/favorites functionality
- [ ] Print-friendly version

## Troubleshooting

### "Failed to load object data" Error
1. Check that `../data/objects.json` exists
2. Verify the JSON file is valid (no syntax errors)
3. Check browser console for specific errors

### Empty Database
1. Run the export script to generate data
2. Verify database contains object_database tables
3. Check that database credentials are correct

### Slow Performance
1. Large datasets may take time to load initially
2. Consider splitting into multiple JSON files by type
3. Enable browser caching for the JSON file

### Missing Objects
1. Check if objects are in filtered zones (see exclusion list above)
2. Verify objects don't have "Mold" flag
3. Ensure database export completed successfully

## Contributing

When adding features or fixing bugs:
1. Test with large datasets (1000+ objects)
2. Verify mobile responsiveness
3. Maintain consistent design with web portal
4. Update this README with any changes

## Technical Details

- **Framework**: Vanilla JavaScript (no dependencies)
- **Data Format**: JSON with metadata and object array
- **Filtering**: Client-side (no server required)
- **Styling**: Shared web portal CSS with custom overrides
- **Compatibility**: Modern browsers (ES6+)

---

**Last Updated**: November 2025
**Version**: 1.0
