#!/usr/bin/env python3
"""
Export LuminariMUD Object Database to JSON for Static Web Portal

This script connects to the MySQL database and exports all object data
to a JSON file suitable for use in a static HTML/JavaScript interface.

Usage:
    python3 export_objectdb_to_json.py [output_file]

Output:
    Creates a JSON file with all object data and related tables.
    Default output: ../docs/web/data/objects.json
"""

import json
import sys
import os
from pathlib import Path

try:
    import pymysql
    import pymysql.cursors
except ImportError:
    print("Error: pymysql module not found.")
    print("Install with: pip3 install pymysql")
    sys.exit(1)


def get_db_connection():
    """
    Get database connection using credentials from environment or config.
    You can set these via environment variables:
        DB_HOST, DB_USER, DB_PASSWORD, DB_NAME

    Or create a mysql.php file in docs/TODO/objectdb/ directory.
    """
    config = {
        'host': os.environ.get('DB_HOST', 'localhost'),
        'user': os.environ.get('DB_USER', ''),
        'password': os.environ.get('DB_PASSWORD', ''),
        'database': os.environ.get('DB_NAME', ''),
        'charset': 'utf8mb4',
        'cursorclass': pymysql.cursors.DictCursor
    }

    # Try to read from lib/mysql_config first (LuminariMUD standard location)
    if not config['user'] or not config['password']:
        mysql_config_path = Path(__file__).parent.parent / 'lib/mysql_config'
        if mysql_config_path.exists():
            try:
                with open(mysql_config_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line.startswith('#') or not line:
                            continue
                        if '=' in line:
                            key, value = line.split('=', 1)
                            key = key.strip()
                            value = value.strip()

                            if key == 'mysql_host':
                                config['host'] = value
                            elif key == 'mysql_username':
                                config['user'] = value
                            elif key == 'mysql_password':
                                config['password'] = value
                            elif key == 'mysql_database':
                                config['database'] = value

                print(f"✓ Loaded credentials from {mysql_config_path}")
            except Exception as e:
                print(f"Warning: Could not read mysql_config: {e}")

    # Try to read from mysql.php if still not set (fallback for web tools)
    if not config['user'] or not config['password']:
        mysql_php_path = Path(__file__).parent.parent / 'docs/TODO/objectdb/mysql.php'
        if mysql_php_path.exists():
            try:
                import re
                with open(mysql_php_path, 'r') as f:
                    content = f.read()

                # Extract credentials using regex
                servername_match = re.search(r'\$servername\s*=\s*["\']([^"\']+)["\']', content)
                username_match = re.search(r'\$username\s*=\s*["\']([^"\']+)["\']', content)
                password_match = re.search(r'\$password\s*=\s*["\']([^"\']+)["\']', content)
                database_match = re.search(r'\$database\s*=\s*["\']([^"\']+)["\']', content)

                if servername_match:
                    config['host'] = servername_match.group(1)
                if username_match:
                    config['user'] = username_match.group(1)
                if password_match:
                    config['password'] = password_match.group(1)
                if database_match:
                    config['database'] = database_match.group(1)

                print(f"✓ Loaded credentials from {mysql_php_path}")
            except Exception as e:
                print(f"Warning: Could not read mysql.php: {e}")

    # Prompt for password if still not set
    if not config['password']:
        import getpass
        config['password'] = getpass.getpass(
            f"Enter password for {config['user']}@{config['host']}: "
        )

    return pymysql.connect(**config)


def export_object_database(output_file):
    """Export complete object database to JSON."""

    print("Connecting to database...")
    conn = get_db_connection()

    try:
        with conn.cursor() as cursor:
            # Define zones to skip (from PHP code)
            skip_zones = [
                'Code Items (DO NOT EDIT)', '<*> Builder Academy Zone',
                'Airship / Carriage Rooms', 'Clan Halls (Chem)',
                'PLAYER PORT RESTRINGS', 'PP Equipment', 'PP only',
                'PP standard eq for newbies', 'PP Unique Zone Only',
                'QUESTS, PP ONLY', 'Uniques Zone', 'Unused Zone', 'Zone 166'
            ]

            skip_zones_sql = "', '".join(skip_zones)

            print("Fetching main object data...")
            # Main query - get all objects
            cursor.execute(f"""
                SELECT DISTINCT a.*
                FROM object_database_items a
                WHERE a.zone_name NOT IN ('{skip_zones_sql}')
                ORDER BY a.object_name ASC
            """)

            items = cursor.fetchall()
            print(f"Found {len(items)} objects")

            # Now fetch related data for each object
            print("Fetching related data (wear slots, bonuses, flags, affects)...")

            for item in items:
                idnum = item['idnum']

                # Skip mold items
                cursor.execute("""
                    SELECT * FROM object_database_obj_flags
                    WHERE object_idnum = %s AND obj_flag = 'Mold'
                """, (idnum,))
                if cursor.fetchone():
                    continue

                # Get wear slots
                cursor.execute("""
                    SELECT worn_slot FROM object_database_wear_slots
                    WHERE object_idnum = %s
                    ORDER BY worn_slot ASC
                """, (idnum,))
                item['wear_slots'] = [row['worn_slot'] for row in cursor.fetchall()]

                # Get bonuses
                cursor.execute("""
                    SELECT bonus_location, bonus_type, bonus_specific, bonus_modifier
                    FROM object_database_bonuses
                    WHERE object_idnum = %s
                    ORDER BY bonus_location ASC
                """, (idnum,))
                item['bonuses'] = cursor.fetchall()

                # Get obj flags
                cursor.execute("""
                    SELECT obj_flag FROM object_database_obj_flags
                    WHERE object_idnum = %s
                    ORDER BY obj_flag ASC
                """, (idnum,))
                item['obj_flags'] = [row['obj_flag'] for row in cursor.fetchall()]

                # Get permanent affects
                cursor.execute("""
                    SELECT perm_affect FROM object_database_perm_affects
                    WHERE object_idnum = %s
                    ORDER BY perm_affect ASC
                """, (idnum,))
                item['perm_affects'] = [row['perm_affect'] for row in cursor.fetchall()]

            # Get unique values for filters
            print("Building filter metadata...")

            cursor.execute("""
                SELECT object_type FROM object_database_items
                WHERE zone_name NOT IN ('{skip_zones_sql}')
                GROUP BY object_type
                ORDER BY object_type ASC
            """)
            object_types = [row['object_type'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT material FROM object_database_items
                WHERE zone_name NOT IN ('{skip_zones_sql}')
                GROUP BY material
                ORDER BY material ASC
            """)
            materials = [row['material'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT zone_name FROM object_database_items
                WHERE zone_name NOT IN ('{skip_zones_sql}')
                GROUP BY zone_name
                ORDER BY zone_name ASC
            """)
            zones = [row['zone_name'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT perm_affect FROM object_database_perm_affects
                GROUP BY perm_affect
                ORDER BY perm_affect ASC
            """)
            perm_affects = [row['perm_affect'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT worn_slot FROM object_database_wear_slots
                GROUP BY worn_slot
                ORDER BY worn_slot ASC
            """)
            wear_slots = [row['worn_slot'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT bonus_location FROM object_database_bonuses
                GROUP BY bonus_location
                ORDER BY bonus_location ASC
            """)
            bonus_locations = [row['bonus_location'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT bonus_type FROM object_database_bonuses
                GROUP BY bonus_type
                ORDER BY bonus_type ASC
            """)
            bonus_types = [row['bonus_type'] for row in cursor.fetchall()]

            cursor.execute("""
                SELECT bonus_specific FROM object_database_bonuses
                WHERE bonus_specific != ''
                GROUP BY bonus_specific
                ORDER BY bonus_specific ASC
            """)
            bonus_specifics = [row['bonus_specific'] for row in cursor.fetchall()]

            # Prepare final data structure
            data = {
                'metadata': {
                    'generated': str(Path(__file__).resolve()),
                    'source': 'LuminariMUD Object Database',
                    'total_objects': len(items),
                    'filters': {
                        'object_types': object_types,
                        'materials': materials,
                        'zones': zones,
                        'perm_affects': perm_affects,
                        'wear_slots': wear_slots,
                        'bonus_locations': bonus_locations,
                        'bonus_types': bonus_types,
                        'bonus_specifics': bonus_specifics,
                        'weapon_groups': [
                            'Axe', 'Double-Weapon', 'Hammer', 'Heavy-Blade',
                            'Light-Blade', 'Monk', 'Polearm', 'Ranged'
                        ]
                    }
                },
                'objects': items
            }

            # Write to JSON file
            print(f"Writing to {output_file}...")
            output_path = Path(output_file)
            output_path.parent.mkdir(parents=True, exist_ok=True)

            with open(output_file, 'w', encoding='utf-8') as f:
                json.dump(data, f, ensure_ascii=False, indent=2, default=str)

            print(f"✓ Successfully exported {len(items)} objects to {output_file}")
            print(f"  File size: {output_path.stat().st_size / 1024:.2f} KB")

    finally:
        conn.close()


def main():
    """Main entry point."""

    # Default output location
    script_dir = Path(__file__).parent
    default_output = script_dir / '../docs/web/data/objects.json'

    # Get output file from command line or use default
    output_file = sys.argv[1] if len(sys.argv) > 1 else str(default_output)

    try:
        export_object_database(output_file)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
