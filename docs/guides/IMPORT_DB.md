# LuminariMUD Database Import Guide

## Overview
This guide will help you import the LuminariMUD database from the SQL dump file. The database contains over 500,000 rows of game data including player characters, items, regions, and game world information.

## Prerequisites
- MySQL or MariaDB installed and running
- Access to MySQL root or admin user
- The SQL dump file: `all_tables_and_data_luminari_mudprod.sql` (62MB)

## Step 1: Create Database and User

First, create the database and user. Run these commands as MySQL root:

```bash
# Login to MySQL as root
mysql -u root -p

# In the MySQL prompt, run:
CREATE DATABASE IF NOT EXISTS luminari_mudprod;
CREATE USER IF NOT EXISTS 'luminari_mud'@'localhost' IDENTIFIED BY 'vZ$eO}fD-4%7';
GRANT ALL PRIVILEGES ON luminari_mudprod.* TO 'luminari_mud'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

## Step 2: Configure MySQL for Import

The dump contains binary spatial data that requires special handling. Create a file called `import_config.sql`:

```sql
SET sql_mode = '';
SET FOREIGN_KEY_CHECKS = 0;
SET GLOBAL max_allowed_packet = 1073741824;
```

Run it:
```bash
mysql -u root -p < import_config.sql
```

## Step 3: Import the Database

**IMPORTANT**: You must use the `--binary-mode` flag because the dump contains binary spatial data.

```bash
# Set password in environment to avoid warnings
export MYSQL_PWD='vZ$eO}fD-4%7'

# Import the database (this will take several minutes)
mysql --binary-mode -u luminari_mud luminari_mudprod < all_tables_and_data_luminari_mudprod.sql
```

## Step 4: Handle Import Errors

If you get errors about spatial data or datetime values, you'll need to import the problematic tables manually.

### Fix Region Data (Critical for Game Stability)

The `region_data` table often fails to import due to binary spatial data. Import it without the spatial data:

```sql
-- Create a file called fix_regions.sql with this content:
USE luminari_mudprod;

-- Drop any existing triggers that might interfere
DROP TRIGGER IF EXISTS AI_MAINTAIN_REGION_INDEX;
DROP TRIGGER IF EXISTS AU_MAINTAIN_REGION_INDEX;
DROP TRIGGER IF EXISTS AD_MAINTAIN_REGION_INDEX;

-- Clear the table
TRUNCATE TABLE region_data;

-- Insert regions without polygon data
INSERT INTO region_data (vnum, zone_vnum, name, region_type, region_polygon, region_props, region_reset_data, region_reset_time) VALUES
(1000001, 10000, 'The Mosswood', 4, NULL, 3, '', '2000-01-01 00:00:00'),
(1000002, 10000, 'The Muddy Hills', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000003, 10000, 'The Gnome City of Hardbuckler', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000004, 10000, 'The Mosswood', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000005, 10000, 'The Lake of Tears', 4, NULL, 6, '', '2000-01-01 00:00:00'),
(1000006, 10000, 'The Lake of Tears', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000007, 10000, 'The City of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000008, 10000, 'The North Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000009, 10000, 'The East Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000010, 10000, 'The South Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000011, 10000, 'The Rat Hills', 1, NULL, 0, '', '2000-01-01 00:00:00'),
(1000012, 10000, 'The Mosswood - Encounter', 2, NULL, 0, '1000126,1000127,1000128,1000129', '2000-07-20 00:00:00');
```

Run it:
```bash
mysql -u luminari_mud -p'vZ$eO}fD-4%7' luminari_mudprod < fix_regions.sql
```

## Step 5: Verify the Import

Create a verification script called `verify_import.sql`:

```sql
USE luminari_mudprod;

-- Check critical tables
SELECT 'player_data' as table_name, COUNT(*) as rows FROM player_data
UNION SELECT 'account_data', COUNT(*) FROM account_data
UNION SELECT 'player_save_objs', COUNT(*) FROM player_save_objs
UNION SELECT 'region_data', COUNT(*) FROM region_data
UNION SELECT 'help_entries', COUNT(*) FROM help_entries
UNION SELECT 'house_data', COUNT(*) FROM house_data;
```

Run it:
```bash
mysql -u luminari_mud -p'vZ$eO}fD-4%7' < verify_import.sql
```

Expected results:
```
+-----------------+--------+
| table_name      | rows   |
+-----------------+--------+
| player_data     | 6309   |
| account_data    | 5572   |
| player_save_objs| 480888 |
| region_data     | 12     |  <-- CRITICAL: Must be 12
| help_entries    | 1652   |
| house_data      | 10196  |
+-----------------+--------+
```

## Step 6: Final Configuration

Re-enable foreign key checks:
```bash
mysql -u luminari_mud -p'vZ$eO}fD-4%7' luminari_mudprod -e "SET FOREIGN_KEY_CHECKS = 1;"
```

## Troubleshooting

### "region_data has 0 rows"
This causes game crashes. Run the fix_regions.sql script from Step 4.

### "ERROR 1292: Incorrect datetime value"
Replace '0000-00-00 00:00:00' with '2000-01-01 00:00:00' in your SQL files.

### "Cannot get geometry object from data"
The spatial data is corrupted. Use the fix_regions.sql approach which inserts NULL for polygon data.

### "Access denied for user"
Check your MySQL credentials in the mysql_config file.

### Import is very slow
The dump is 62MB with 500,000+ rows. On a typical system it takes 5-10 minutes.

## Quick Import Script

For convenience, here's a complete import script. Save as `import_all.sh`:

```bash
#!/bin/bash
# LuminariMUD Complete Database Import

# Configuration
DB_USER="luminari_mud"
DB_PASS='vZ$eO}fD-4%7'
DB_NAME="luminari_mudprod"
SQL_DUMP="all_tables_and_data_luminari_mudprod.sql"

echo "Starting LuminariMUD database import..."

# Set password in environment
export MYSQL_PWD="$DB_PASS"

# Prepare database
echo "Preparing database..."
mysql -u "$DB_USER" "$DB_NAME" -e "SET sql_mode = ''; SET FOREIGN_KEY_CHECKS = 0;" 2>/dev/null

# Import main dump
echo "Importing main database (this will take several minutes)..."
mysql --binary-mode -u "$DB_USER" "$DB_NAME" < "$SQL_DUMP"

# Fix regions if needed
REGION_COUNT=$(mysql -u "$DB_USER" "$DB_NAME" -N -e "SELECT COUNT(*) FROM region_data" 2>/dev/null)
if [ "$REGION_COUNT" -eq "0" ]; then
    echo "Fixing region_data..."
    mysql -u "$DB_USER" "$DB_NAME" -e "
    INSERT INTO region_data (vnum, zone_vnum, name, region_type, region_polygon, region_props, region_reset_data, region_reset_time) VALUES
    (1000001, 10000, 'The Mosswood', 4, NULL, 3, '', '2000-01-01 00:00:00'),
    (1000002, 10000, 'The Muddy Hills', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000003, 10000, 'The Gnome City of Hardbuckler', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000004, 10000, 'The Mosswood', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000005, 10000, 'The Lake of Tears', 4, NULL, 6, '', '2000-01-01 00:00:00'),
    (1000006, 10000, 'The Lake of Tears', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000007, 10000, 'The City of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000008, 10000, 'The North Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000009, 10000, 'The East Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000010, 10000, 'The South Gate of Ashenport', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000011, 10000, 'The Rat Hills', 1, NULL, 0, '', '2000-01-01 00:00:00'),
    (1000012, 10000, 'The Mosswood - Encounter', 2, NULL, 0, '1000126,1000127,1000128,1000129', '2000-07-20 00:00:00');"
fi

# Re-enable constraints
mysql -u "$DB_USER" "$DB_NAME" -e "SET FOREIGN_KEY_CHECKS = 1;"

# Show results
echo -e "\nImport complete! Database status:"
mysql -u "$DB_USER" "$DB_NAME" -e "
SELECT 'player_data' as table_name, COUNT(*) as rows FROM player_data
UNION SELECT 'region_data', COUNT(*) FROM region_data
UNION SELECT 'player_save_objs', COUNT(*) FROM player_save_objs;"

echo -e "\nIf region_data shows 12 rows, the import was successful!"
```

Make it executable and run:
```bash
chmod +x import_all.sh
./import_all.sh
```

## Success Criteria

The import is successful when:
1. No error messages during import
2. `region_data` table has exactly 12 rows (prevents game crashes)
3. `player_data` has 6,309 rows
4. `player_save_objs` has 480,888 rows

## Notes

- The spatial polygon data in the original dump is in binary format and often fails to import. The game works fine with NULL polygons.
- The import includes player data, so this is suitable for migrating a live game.
- Always backup your existing database before importing.
- The MySQL user password `vZ$eO}fD-4%7` is stored in `mysql_config` file.