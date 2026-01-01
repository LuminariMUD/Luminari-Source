#!/usr/bin/env python3
"""
Generate detailed spell HTML with information from MySQL help system
"""

import os
import re
import sys
import subprocess
import json
from collections import defaultdict
from pathlib import Path
from datetime import datetime


def strip_color_codes(text):
    """Remove MUD color codes like \\tn, \\tW, \\tD, etc."""
    # Remove color codes like \tn, \tW, \tD, etc.
    text = re.sub(r'\\t[a-zA-Z0-9]', '', text)
    return text


def clean_display_text(text):
    """Clean text for display by removing literal backslash escapes."""
    # Remove literal \n and \t that are stored as backslash-n and backslash-t
    text = text.replace('\\n', '\n').replace('\\t', '    ')
    # Remove backslashes at end of lines (likely from formatting)
    text = re.sub(r'\\\s*$', '', text, flags=re.MULTILINE)
    text = re.sub(r'\\\s*\n', '\n', text)
    # Clean up excessive whitespace but preserve line breaks
    lines = [line.strip() for line in text.split('\n')]
    # Remove empty lines
    lines = [line for line in lines if line]
    return '\n'.join(lines)


# Read MySQL config
def get_mysql_config():
    config = {
        'host': 'localhost',
        'port': 3306,
        'database': 'luminari',
        'user': 'luminari',
        'password': ''
    }

    # Try to read from mysql_config file
    try:
        with open('lib/mysql_config', 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split('=', 1)
                    if len(parts) == 2:
                        key = parts[0].strip().lower()
                        value = parts[1].strip().strip('"\'')
                        if 'host' in key:
                            config['host'] = value
                        elif 'database' in key or key == 'db':
                            config['database'] = value
                        elif 'user' in key or key == 'username':
                            config['user'] = value
                        elif 'password' in key or key == 'pass':
                            config['password'] = value
                        elif 'port' in key:
                            config['port'] = int(value)
    except Exception as e:
        print(f"Warning: Could not read mysql_config: {e}")

    return config


# Extract class spell assignments from class.c
def extract_class_spells():
    """Extract which classes can cast each spell and at what level"""
    class_spells = {}  # spell_name -> [(class_name, level), ...]

    # Map of class numbers to names
    class_names = {
        'CLASS_WIZARD': 'Wizard',
        'CLASS_CLERIC': 'Cleric',
        'CLASS_ROGUE': 'Rogue',
        'CLASS_WARRIOR': 'Warrior',
        'CLASS_MONK': 'Monk',
        'CLASS_DRUID': 'Druid',
        'CLASS_BERSERKER': 'Berserker',
        'CLASS_SORCERER': 'Sorcerer',
        'CLASS_PALADIN': 'Paladin',
        'CLASS_BLACKGUARD': 'Blackguard',
        'CLASS_RANGER': 'Ranger',
        'CLASS_BARD': 'Bard',
        'CLASS_PSIONICIST': 'Psionicist',
        'CLASS_WEAPON_MASTER': 'Weapon Master',
        'CLASS_ARCANE_ARCHER': 'Arcane Archer',
        'CLASS_ARCANE_SHADOW': 'Arcane Shadow',
        'CLASS_ELDRITCH_KNIGHT': 'Eldritch Knight',
        'CLASS_SPELLSWORD': 'Spellsword',
        'CLASS_SACRED_FIST': 'Sacred Fist',
        'CLASS_STALWART_DEFENDER': 'Stalwart Defender',
        'CLASS_ALCHEMIST': 'Alchemist',
        'CLASS_INQUISITOR': 'Inquisitor',
        'CLASS_SUMMONER': 'Summoner',
        'CLASS_NECROMANCER': 'Necromancer',
        'CLASS_SHADOW_DANCER': 'Shadow Dancer',
    }

    try:
        with open('src/class.c', 'r', encoding='latin-1') as f:
            content = f.read()

        # Find all spell_assignment() calls
        # Pattern: spell_assignment(CLASS_NAME, SPELL_NAME, level);
        pattern = r'spell_assignment\(\s*(CLASS_\w+),\s*SPELL_(\w+),\s*(\d+)\s*\);'
        matches = re.findall(pattern, content)

        for class_const, spell_const, level in matches:
            # Convert SPELL_CONSTANT to spell name (lowercase with underscores to spaces)
            spell_name = spell_const.lower().replace('_', ' ')
            class_name = class_names.get(class_const, class_const)
            level = int(level)

            if spell_name not in class_spells:
                class_spells[spell_name] = []

            class_spells[spell_name].append((class_name, level))

        print(f"Extracted spell assignments for {len(class_spells)} spells from class.c")

    except Exception as e:
        print(f"Warning: Could not read class.c: {e}")

    return class_spells


# Extract spells from spell_parser.c
def extract_spells():
    spells = {}

    with open('src/spell_parser.c', 'r', encoding='latin-1') as f:
        content = f.read()

    # Find all spello() calls
    spell_pattern = r'spello\(\s*SPELL_(\w+),\s*"([^"]+)"[^;]*\);'
    matches = re.findall(spell_pattern, content, re.MULTILINE | re.DOTALL)

    for spell_const, spell_name in matches:
        spells[spell_name.lower()] = {
            'name': spell_name,
            'constant': spell_const,
            'help': None
        }

    print(f"Extracted {len(spells)} spells from spell_parser.c")
    return spells

# Get help entries from MySQL
def get_help_entries():
    help_data = {}

    try:
        config = get_mysql_config()

        # Query help_entries for spell entries
        # Replace newlines and carriage returns in entry to prevent line splitting
        query = """
            SELECT CONCAT_WS('||FIELD||',
                h.tag,
                REPLACE(REPLACE(h.entry, CHAR(13), ''), CHAR(10), '\\\\n'),
                h.min_level,
                h.last_updated,
                COALESCE(GROUP_CONCAT(hk.keyword SEPARATOR ', '), 'NULL')
            ) as row_data
            FROM help_entries h
            LEFT JOIN help_keywords hk ON h.tag = hk.help_tag
            GROUP BY h.tag, h.entry, h.min_level, h.last_updated
            ORDER BY h.tag
        """

        # Execute via mysql command-line client
        cmd = ['mysql']
        cmd.extend(['-h', config['host']])
        cmd.extend(['-P', str(config['port'])])
        cmd.extend(['-u', config['user']])

        # Add password if present (no space between -p and password)
        if config.get('password'):
            cmd.append(f"-p{config['password']}")

        cmd.extend(['-D', config['database']])
        cmd.extend(['-e', query])
        cmd.extend(['--batch', '--skip-column-names'])

        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print(f"Error querying MySQL: {result.stderr}")
            return help_data

        # Parse output using custom separator
        for line in result.stdout.strip().split('\n'):
            if not line or '||FIELD||' not in line:
                continue
            parts = line.split('||FIELD||')
            if len(parts) >= 5:  # We expect 5 parts (tag, entry, min_level, last_updated, keywords)
                tag = parts[0]
                entry = parts[1] if parts[1] != 'NULL' else ''
                # Handle embedded newlines in entry (\\n becomes actual newline)
                entry = entry.replace('\\n', '\n')

                try:
                    min_level = int(parts[2]) if parts[2] != 'NULL' and parts[2].isdigit() else 0
                except (ValueError, AttributeError):
                    min_level = 0

                help_data[tag.lower()] = {
                    'tag': tag,
                    'entry': entry,
                    'min_level': min_level,
                    'last_updated': parts[3] if parts[3] != 'NULL' else '',
                    'keywords': parts[4] if len(parts) > 4 and parts[4] != 'NULL' else ''
                }

        print(f"Retrieved {len(help_data)} help entries from database")

    except Exception as e:
        print(f"Error connecting to MySQL: {e}")
        print("Continuing without help data...")

    return help_data

# Generate HTML
def generate_html(spells, help_data, class_spells):
    # Filter spells to only include those assigned to classes
    filtered_spells = {}
    for spell_key, spell_data in spells.items():
        spell_name = spell_data['name']

        # Skip UNUSED spells
        if '!UNUSED!' in spell_name:
            continue

        # Only include if assigned to a class
        found = False
        if spell_key in class_spells or spell_name.lower() in class_spells:
            found = True
        else:
            # Try alternate formats
            for key in class_spells:
                if key == spell_name.lower().replace(' ', '-') or key == spell_name.lower().replace(' ', ''):
                    found = True
                    break

        if found:
            filtered_spells[spell_key] = spell_data

    spell_count_total = len(filtered_spells)

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Luminari MUD - Detailed Spell Reference</title>
    <style>
        body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }}
        .container {{ max-width: 1200px; margin: 0 auto; }}
        h1 {{ color: white; text-align: center; font-size: 2.5em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }}
        .subtitle {{ color: #f0f0f0; text-align: center; margin-bottom: 30px; }}
        .spell-block {{ background: white; margin: 10px 0; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); transition: all 0.3s; }}
        .spell-block:hover {{ box-shadow: 0 6px 12px rgba(0,0,0,0.15); }}
        .spell-header {{ padding: 20px 25px; cursor: pointer; user-select: none; display: flex; justify-content: space-between; align-items: center; }}
        .spell-header:hover {{ background: #f8f9fa; border-radius: 12px; }}
        .spell-header-left {{ flex: 1; }}
        .spell-name {{ font-size: 24px; font-weight: bold; color: #667eea; margin-bottom: 5px; }}
        .spell-id {{ color: #999; font-size: 13px; font-family: 'Courier New', monospace; }}
        .toggle-icon {{ font-size: 24px; color: #667eea; transition: transform 0.3s; }}
        .toggle-icon.open {{ transform: rotate(180deg); }}
        .spell-details {{ max-height: 0; overflow: hidden; transition: max-height 0.3s ease-out; }}
        .spell-details.open {{ max-height: 2000px; transition: max-height 0.5s ease-in; }}
        .spell-content {{ padding: 0 25px 25px 25px; border-top: 2px solid #e9ecef; margin-top: 0; }}
        .info-grid {{ display: grid; grid-template-columns: 200px 1fr; gap: 12px; margin: 20px 0; background: #f8f9fa; padding: 15px; border-radius: 8px; }}
        .info-label {{ font-weight: bold; color: #495057; }}
        .info-value {{ color: #212529; }}
        .spell-description {{ background: #e7f3ff; padding: 20px; border-radius: 8px; border-left: 4px solid #2196F3; margin-top: 15px; line-height: 1.6; }}
        .spell-description pre {{ white-space: pre-wrap; font-family: inherit; margin: 0; }}
        .alpha-header {{ font-size: 36px; font-weight: bold; color: white; margin: 40px 0 20px 0; padding: 15px; background: rgba(255,255,255,0.2); border-radius: 12px; text-align: center; backdrop-filter: blur(10px); }}
        .navbar {{ position: sticky; top: 0; background: rgba(51, 51, 51, 0.95); padding: 15px; margin: -20px -20px 30px -20px; z-index: 1000; backdrop-filter: blur(10px); box-shadow: 0 2px 10px rgba(0,0,0,0.2); }}
        .navbar-content {{ max-width: 1200px; margin: 0 auto; display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; }}
        .navbar a {{ color: white; text-decoration: none; padding: 8px 12px; border-radius: 5px; transition: all 0.3s; }}
        .navbar a:hover {{ background: #667eea; transform: translateY(-2px); }}
        .no-help {{ background: #fff3cd; padding: 15px; border-radius: 8px; border-left: 4px solid #ffc107; margin-top: 15px; }}
        .stats-badge {{ display: inline-block; background: #667eea; color: white; padding: 5px 12px; border-radius: 20px; font-size: 14px; margin: 5px; }}
        .keyword-tag {{ display: inline-block; background: #e9ecef; padding: 4px 10px; border-radius: 15px; font-size: 12px; margin: 3px; color: #495057; }}
    </style>
    <script>
        function toggleSpell(spellId) {{
            const details = document.getElementById('details-' + spellId);
            const icon = document.getElementById('icon-' + spellId);

            if (details.classList.contains('open')) {{
                details.classList.remove('open');
                icon.classList.remove('open');
            }} else {{
                details.classList.add('open');
                icon.classList.add('open');
            }}
        }}

        // Optional: Add keyboard navigation
        document.addEventListener('keydown', function(e) {{
            if (e.key === 'Escape') {{
                // Close all open spells
                document.querySelectorAll('.spell-details.open').forEach(detail => {{
                    detail.classList.remove('open');
                }});
                document.querySelectorAll('.toggle-icon.open').forEach(icon => {{
                    icon.classList.remove('open');
                }});
            }}
        }});
    </script>
</head>
<body>

<div class="navbar">
    <div class="navbar-content">
        <a href="#top">⬆ Top</a>
        <span style="color: #999; margin: 0 10px;">|</span>
        <a href="spells_by_class.html">🎓 View by Class</a>
        <span style="color: #999; margin: 0 10px;">|</span>
"""

    # Add alphabet navigation
    for letter in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
        html += f'        <a href="#letter-{letter}">{letter}</a>\n'

    html += """    </div>
</div>

<div class="container">
    <h1 id="top">⚡ Luminari MUD Spell Compendium ⚡</h1>
    <div class="subtitle">
        <p>Generated: """ + datetime.now().strftime('%B %d, %Y at %H:%M') + f"""</p>
        <p><span class="stats-badge">📚 {spell_count_total} Total Spells</span></p>
        <p style="margin-top: 10px; font-size: 14px;">💡 <em>Click on any spell name to expand and view details</em></p>
    </div>

"""

    # Sort spells alphabetically
    sorted_spells = sorted(filtered_spells.items(), key=lambda x: x[0])

    # Group by first letter
    current_letter = ''
    spell_count = 0

    for spell_key, spell_data in sorted_spells:
        spell_name = spell_data['name']
        first_letter = spell_name[0].upper() if spell_name else 'A'

        spell_count += 1

        if first_letter != current_letter:
            current_letter = first_letter
            html += f'    <div class="alpha-header" id="letter-{current_letter}">{current_letter}</div>\n\n'

        spell_id = f"spell{spell_count}"

        html += '    <div class="spell-block">\n'
        html += f'        <div class="spell-header" onclick="toggleSpell(\'{spell_id}\')">\n'
        html += '            <div class="spell-header-left">\n'
        html += f'                <div class="spell-name">{spell_name}</div>\n'
        html += '            </div>\n'
        html += f'            <div class="toggle-icon" id="icon-{spell_id}">▼</div>\n'
        html += '        </div>\n'
        html += f'        <div class="spell-details" id="details-{spell_id}">\n'
        html += '            <div class="spell-content">\n'

        # Check if we have help data
        # Try multiple variations: exact match, with hyphen, without spaces, with spell- prefix
        lookup_keys = [
            spell_key,
            spell_name.lower(),
            spell_name.lower().replace(' ', '-'),
            'spell-' + spell_name.lower().replace(' ', '-'),
            spell_name.lower().replace(' ', '')
        ]

        help_entry = None
        for key in lookup_keys:
            if key in help_data:
                help_entry = help_data[key]
                break

        if help_entry:
            # Parse help entry for structured data
            entry_text = help_entry['entry']

            # Display keywords if available
            if help_entry.get('keywords'):
                html += '                <div style="margin-bottom: 15px;">\n'
                for keyword in help_entry['keywords'].split(','):
                    keyword = keyword.strip()
                    if keyword:
                        html += f'                    <span class="keyword-tag">🔑 {keyword}</span>\n'
                html += '                </div>\n'

            # Try to extract structured information
            # First strip color codes for clean parsing
            clean_text = strip_color_codes(entry_text)
            info_dict = {}
            lines = clean_text.split('\n')
            for line in lines:
                if ':' in line and ('Usage' in line or 'School' in line or 'Target' in line or
                                   'Duration' in line or 'Saving' in line or 'Magic' in line or
                                   'Damage' in line or 'Accumulative' in line or 'Discipline' in line):
                    parts = line.split(':', 1)
                    if len(parts) == 2:
                        key = parts[0].strip(' >-')
                        value = clean_display_text(parts[1])
                        if value and key:
                            info_dict[key] = value

            # Display structured info if found
            if info_dict:
                html += '                <div class="info-grid">\n'

                for key in ['Usage', 'School of Magic', 'Discipline', 'Target(s)', 'Duration',
                           'Saving Throw', 'Magic Resist', 'Damage Type', 'Accumulative']:
                    if key in info_dict:
                        html += f'                    <div class="info-label">{key}:</div>\n'
                        html += f'                    <div class="info-value">{info_dict[key]}</div>\n'

                html += '                </div>\n'

            # Display class information
            # Check if this spell is available to any classes
            spell_classes = class_spells.get(spell_key) or class_spells.get(spell_name.lower())
            if not spell_classes:
                # Try with hyphens and without spaces
                spell_classes = (class_spells.get(spell_name.lower().replace(' ', '-')) or
                               class_spells.get(spell_name.lower().replace(' ', '')))

            if spell_classes:
                # Sort by level, then by class name
                spell_classes = sorted(spell_classes, key=lambda x: (x[1], x[0]))

                html += '                <div style="margin: 20px 0; padding: 15px; background: #fff8e1; border-radius: 8px; border-left: 4px solid #ffa000;">\n'
                html += '                    <strong>🎓 Available to Classes:</strong><br>\n'
                html += '                    <div style="margin-top: 10px; display: flex; flex-wrap: wrap; gap: 8px;">\n'

                for class_name, level in spell_classes:
                    html += f'                        <span style="background: #fff; padding: 6px 12px; border-radius: 15px; border: 2px solid #ffa000; font-size: 13px;">\n'
                    html += f'                            <strong>{class_name}</strong> (Level {level})\n'
                    html += '                        </span>\n'

                html += '                    </div>\n'
                html += '                </div>\n'

            # Display description
            # Extract description part (after the >Description: marker)
            description = ''
            desc_started = False
            for line in lines:
                if desc_started:
                    # Stop at "See also" section
                    if line.strip().startswith('>See also') or 'See also' in line:
                        break
                    description += line + '\n'
                elif '>Description:' in line or ('>Description' in line and ':' in line):
                    desc_started = True
                    # Check if description is on same line
                    parts = line.split(':', 1)
                    if len(parts) == 2 and parts[1].strip():
                        description += parts[1].strip() + '\n'

            if description.strip():
                # Clean up the description - remove extra whitespace and "See also" lines
                desc_lines = [line for line in description.strip().split('\n') if line.strip() and 'See also' not in line]
                clean_desc = clean_display_text('\n'.join(desc_lines))

                html += '                <div class="spell-description">\n'
                html += '                    <strong>📖 Description:</strong><br>\n'
                html += f'                    <p>{clean_desc}</p>\n'
                html += '                </div>\n'

            # Full entry if we want it
            # html += f'        <details><summary>View Full Help Entry</summary><pre>{entry_text}</pre></details>\n'

            # Metadata
            if help_entry.get('last_updated'):
                html += f'                <div style="margin-top: 15px; color: #6c757d; font-size: 12px;">Last Updated: {help_entry["last_updated"]}</div>\n'

        else:
            # No help entry, but still show class info if available
            html += '                <div class="no-help">⚠️ No detailed help information available for this spell yet.</div>\n'

            # Display class information even without help data
            spell_classes = class_spells.get(spell_key) or class_spells.get(spell_name.lower())
            if not spell_classes:
                # Try with hyphens and without spaces
                spell_classes = (class_spells.get(spell_name.lower().replace(' ', '-')) or
                               class_spells.get(spell_name.lower().replace(' ', '')))

            if spell_classes:
                # Sort by level, then by class name
                spell_classes = sorted(spell_classes, key=lambda x: (x[1], x[0]))

                html += '                <div style="margin: 20px 0; padding: 15px; background: #fff8e1; border-radius: 8px; border-left: 4px solid #ffa000;">\n'
                html += '                    <strong>🎓 Available to Classes:</strong><br>\n'
                html += '                    <div style="margin-top: 10px; display: flex; flex-wrap: wrap; gap: 8px;">\n'

                for class_name, level in spell_classes:
                    html += f'                        <span style="background: #fff; padding: 6px 12px; border-radius: 15px; border: 2px solid #ffa000; font-size: 13px;">\n'
                    html += f'                            <strong>{class_name}</strong> (Level {level})\n'
                    html += '                        </span>\n'

                html += '                    </div>\n'
                html += '                </div>\n'

        html += '            </div>\n'  # Close spell-content
        html += '        </div>\n'      # Close spell-details
        html += '    </div>\n\n'        # Close spell-block

    # Footer
    html += """    <div style="margin-top: 50px; padding: 30px; background: white; border-radius: 12px; text-align: center;">
        <h3 style="color: #667eea;">Luminari MUD Spell Reference</h3>
        <p>For more information and to experience these spells in action,<br>
        visit <strong>Luminari MUD</strong></p>
        <p>
            <a href="#top" style="color: #667eea; text-decoration: none; font-weight: bold; margin: 0 15px;">⬆ Back to Top</a>
            <span style="color: #999;">|</span>
            <a href="spells_by_class.html" style="color: #667eea; text-decoration: none; font-weight: bold; margin: 0 15px;">🎓 View by Class</a>
        </p>
    </div>
</div>

</body>
</html>
"""

    return html


# Generate HTML organized by class
def level_to_circle(class_name, level):
    """Convert character level to spell circle based on class progression"""
    # Epic spells are level 21+
    if level >= 21:
        return "Epic"

    # Wizard, Cleric, Druid: Circle 1 at level 1, Circle 2 at 3, then every 2 levels up to Circle 9 at 17
    if class_name in ['Wizard', 'Cleric', 'Druid']:
        if level == 1: return 1
        if level >= 3:
            circle = 2 + ((level - 3) // 2)
            return min(circle, 9)
        return None

    # Sorcerer: Circle 1 at level 1, Circle 2 at 4, then every 2 levels up to Circle 9 at 18
    if class_name == 'Sorcerer':
        if level == 1: return 1
        if level >= 4:
            circle = 2 + ((level - 4) // 2)
            return min(circle, 9)
        return None

    # Alchemist, Bard, Inquisitor, Summoner: Circle 1 at level 1, Circle 2 at 4, every 3 levels up to Circle 6 at 16
    if class_name in ['Alchemist', 'Bard', 'Inquisitor', 'Summoner']:
        if level == 1: return 1
        if level >= 4:
            circle = 2 + ((level - 4) // 3)
            return min(circle, 6)
        return None

    # Paladin, Ranger, Blackguard: Circle 1 at level 6, Circle 2 at 10, Circle 3 at 12, Circle 4 at 15
    if class_name in ['Paladin', 'Ranger', 'Blackguard']:
        if level >= 15: return 4
        if level >= 12: return 3
        if level >= 10: return 2
        if level >= 6: return 1
        return None

    # For any other class, return level as-is (fallback)
    return level


def generate_class_html(spells, help_data, class_spells):
    """Generate HTML organized by class instead of alphabetically by spell"""

    # First, organize spells by class and circle
    class_spell_map = {}  # class_name -> [(spell_name, circle, level, spell_data), ...]

    for spell_name, class_list in class_spells.items():
        for class_name, level in class_list:
            if class_name not in class_spell_map:
                class_spell_map[class_name] = []

            # Find the spell data
            spell_data = spells.get(spell_name)
            if not spell_data:
                # Try alternate formats
                for key, data in spells.items():
                    if data['name'].lower() == spell_name:
                        spell_data = data
                        break

            if spell_data:
                # Convert level to circle for this class
                circle = level_to_circle(class_name, level)
                if circle is not None:
                    class_spell_map[class_name].append((spell_name, circle, level, spell_data))

    # Sort spells within each class by circle, then alphabetically
    # Epic spells (string) should come last, numeric circles come first
    for class_name in class_spell_map:
        class_spell_map[class_name].sort(key=lambda x: (x[1] == "Epic", x[1] if x[1] != "Epic" else 999, x[0]))

    # Generate HTML
    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Luminari MUD - Spells by Class</title>
    <style>
        body {{ font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); }}
        .container {{ max-width: 1400px; margin: 0 auto; }}
        h1 {{ color: white; text-align: center; font-size: 2.5em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }}
        .subtitle {{ color: #f0f0f0; text-align: center; margin-bottom: 30px; }}
        .class-block {{ background: white; margin: 20px 0; border-radius: 12px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); transition: all 0.3s; }}
        .class-block:hover {{ box-shadow: 0 6px 12px rgba(0,0,0,0.15); }}
        .class-header {{ padding: 25px; cursor: pointer; user-select: none; display: flex; justify-content: space-between; align-items: center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); border-radius: 12px 12px 0 0; }}
        .class-header:hover {{ opacity: 0.95; }}
        .class-name {{ font-size: 32px; font-weight: bold; color: white; text-shadow: 2px 2px 4px rgba(0,0,0,0.3); }}
        .class-spell-count {{ color: #f0f0f0; font-size: 16px; }}
        .toggle-icon {{ font-size: 28px; color: white; transition: transform 0.3s; }}
        .toggle-icon.open {{ transform: rotate(180deg); }}
        .class-details {{ max-height: 0; overflow: hidden; transition: max-height 0.3s ease-out; }}
        .class-details.open {{ max-height: 10000px; transition: max-height 0.8s ease-in; }}
        .class-content {{ padding: 25px; }}
        .level-section {{ margin: 20px 0; }}
        .level-header {{ font-size: 20px; font-weight: bold; color: #667eea; padding: 12px 15px; background: #f8f9fa; border-left: 4px solid #667eea; margin-bottom: 15px; border-radius: 4px; }}
        .level-header.epic {{ color: #d32f2f; background: #fff3e0; border-left: 4px solid #d32f2f; }}
        .spell-item {{ background: white; margin: 8px 0; padding: 15px 20px; border-radius: 8px; border: 2px solid #e9ecef; cursor: pointer; transition: all 0.2s; }}
        .spell-item:hover {{ border-color: #667eea; background: #f8f9fa; transform: translateX(5px); }}
        .spell-item-header {{ display: flex; justify-content: space-between; align-items: center; }}
        .spell-item-name {{ font-size: 18px; font-weight: bold; color: #667eea; }}
        .spell-item-constant {{ color: #999; font-size: 12px; font-family: 'Courier New', monospace; margin-top: 3px; }}
        .spell-item-icon {{ font-size: 16px; color: #667eea; transition: transform 0.3s; }}
        .spell-item-icon.open {{ transform: rotate(180deg); }}
        .spell-item-details {{ max-height: 0; overflow: hidden; transition: max-height 0.3s ease-out; margin-top: 0; }}
        .spell-item-details.open {{ max-height: 2000px; transition: max-height 0.5s ease-in; margin-top: 15px; padding-top: 15px; border-top: 2px solid #e9ecef; }}
        .info-grid {{ display: grid; grid-template-columns: 180px 1fr; gap: 10px; margin: 15px 0; background: #f8f9fa; padding: 12px; border-radius: 6px; font-size: 14px; }}
        .info-label {{ font-weight: bold; color: #495057; }}
        .info-value {{ color: #212529; }}
        .spell-description {{ background: #e7f3ff; padding: 15px; border-radius: 6px; border-left: 4px solid #2196F3; margin-top: 12px; line-height: 1.6; font-size: 14px; }}
        .navbar {{ position: sticky; top: 0; background: rgba(51, 51, 51, 0.95); padding: 15px; margin: -20px -20px 30px -20px; z-index: 1000; backdrop-filter: blur(10px); box-shadow: 0 2px 10px rgba(0,0,0,0.2); }}
        .navbar-content {{ max-width: 1400px; margin: 0 auto; display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; align-items: center; }}
        .navbar a {{ color: white; text-decoration: none; padding: 8px 12px; border-radius: 5px; transition: all 0.3s; font-size: 14px; }}
        .navbar a:hover {{ background: #667eea; transform: translateY(-2px); }}
        .navbar-divider {{ color: #999; margin: 0 10px; }}
        .stats-badge {{ display: inline-block; background: #667eea; color: white; padding: 5px 12px; border-radius: 20px; font-size: 14px; margin: 5px; }}
        .keyword-tag {{ display: inline-block; background: #e9ecef; padding: 4px 10px; border-radius: 15px; font-size: 12px; margin: 3px; color: #495057; }}
        .no-help {{ background: #fff3cd; padding: 12px; border-radius: 6px; border-left: 4px solid #ffc107; margin-top: 12px; font-size: 14px; }}
    </style>
    <script>
        function toggleClass(classId) {{
            const details = document.getElementById('class-details-' + classId);
            const icon = document.getElementById('class-icon-' + classId);

            if (details.classList.contains('open')) {{
                details.classList.remove('open');
                icon.classList.remove('open');
            }} else {{
                details.classList.add('open');
                icon.classList.add('open');
            }}
        }}

        function toggleSpell(spellId) {{
            const details = document.getElementById('spell-details-' + spellId);
            const icon = document.getElementById('spell-icon-' + spellId);

            if (details.classList.contains('open')) {{
                details.classList.remove('open');
                icon.classList.remove('open');
            }} else {{
                details.classList.add('open');
                icon.classList.add('open');
            }}
        }}

        // Close all with ESC key
        document.addEventListener('keydown', function(e) {{
            if (e.key === 'Escape') {{
                document.querySelectorAll('.class-details.open, .spell-item-details.open').forEach(detail => {{
                    detail.classList.remove('open');
                }});
                document.querySelectorAll('.toggle-icon.open, .spell-item-icon.open').forEach(icon => {{
                    icon.classList.remove('open');
                }});
            }}
        }});
    </script>
</head>
<body>

<div class="navbar">
    <div class="navbar-content">
        <a href="#top">⬆ Top</a>
        <span class="navbar-divider">|</span>
        <a href="spells_reference.html">📖 View by Spell</a>
        <span class="navbar-divider">|</span>
"""

    # Add class navigation links
    sorted_classes = sorted(class_spell_map.keys())
    for i, class_name in enumerate(sorted_classes):
        html += f'        <a href="#class-{class_name.replace(" ", "-")}">{class_name}</a>\n'
        if i < len(sorted_classes) - 1:
            html += '        <span class="navbar-divider">•</span>\n'

    html += """    </div>
</div>

<div class="container">
    <h1 id="top">⚡ Luminari MUD - Spells by Class ⚡</h1>
    <div class="subtitle">
        <p>Generated: """ + datetime.now().strftime('%B %d, %Y at %H:%M') + f"""</p>
        <p><span class="stats-badge">🎓 {len(class_spell_map)} Classes</span></p>
        <p style="margin-top: 10px; font-size: 14px;">💡 <em>Click on any class name to expand and view their spells</em></p>
    </div>

"""

    # Generate each class section
    class_count = 0
    for class_name in sorted_classes:
        class_count += 1
        spell_list = class_spell_map[class_name]
        class_id = class_name.replace(' ', '-')

        html += f'    <div class="class-block" id="class-{class_id}">\n'
        html += f'        <div class="class-header" onclick="toggleClass(\'{class_id}\')">\n'
        html += '            <div>\n'
        html += f'                <div class="class-name">{class_name}</div>\n'
        html += f'                <div class="class-spell-count">{len(spell_list)} spells available</div>\n'
        html += '            </div>\n'
        html += f'            <div class="toggle-icon" id="class-icon-{class_id}">▼</div>\n'
        html += '        </div>\n'
        html += f'        <div class="class-details" id="class-details-{class_id}">\n'
        html += '            <div class="class-content">\n'

        # Group spells by circle
        spells_by_circle = {}
        for spell_name, circle, level, spell_data in spell_list:
            if circle not in spells_by_circle:
                spells_by_circle[circle] = []
            spells_by_circle[circle].append((spell_name, level, spell_data))

        # Generate each circle section
        spell_counter = 0
        for circle in sorted(spells_by_circle.keys(), key=lambda x: (x != "Epic", x)):
            html += f'                <div class="level-section">\n'
            epic_class = ' epic' if circle == "Epic" else ''
            circle_label = circle if circle == "Epic" else f"Circle {circle}"
            html += f'                    <div class="level-header{epic_class}">{circle_label} Spells</div>\n'

            for spell_name, level, spell_data in sorted(spells_by_circle[circle], key=lambda x: x[0]):
                spell_counter += 1
                spell_id = f"{class_id}-spell{spell_counter}"

                html += f'                    <div class="spell-item" onclick="toggleSpell(\'{spell_id}\')">\n'
                html += '                        <div class="spell-item-header">\n'
                html += '                            <div>\n'
                html += f'                                <div class="spell-item-name">{spell_data["name"]} <span style="color: #666; font-size: 0.85em;">(Level {level})</span></div>\n'
                html += '                            </div>\n'
                html += f'                            <div class="spell-item-icon" id="spell-icon-{spell_id}">▼</div>\n'
                html += '                        </div>\n'
                html += f'                        <div class="spell-item-details" id="spell-details-{spell_id}">\n'

                # Get help entry - try multiple variations including spell- prefix
                lookup_keys = [
                    spell_name,
                    spell_data['name'].lower(),
                    spell_data['name'].lower().replace(' ', '-'),
                    'spell-' + spell_data['name'].lower().replace(' ', '-'),
                    spell_data['name'].lower().replace(' ', '')
                ]

                help_entry = None
                for key in lookup_keys:
                    if key in help_data:
                        help_entry = help_data[key]
                        break

                if help_entry:
                    entry_text = help_entry['entry']

                    # Display keywords
                    if help_entry.get('keywords'):
                        html += '                            <div style="margin-bottom: 10px;">\n'
                        for keyword in help_entry['keywords'].split(','):
                            keyword = keyword.strip()
                            if keyword:
                                html += f'                                <span class="keyword-tag">🔑 {keyword}</span>\n'
                        html += '                            </div>\n'

                    # Extract structured info
                    clean_text = strip_color_codes(entry_text)
                    info_dict = {}
                    lines = clean_text.split('\n')
                    for line in lines:
                        if ':' in line and ('Usage' in line or 'School' in line or 'Target' in line or
                                           'Duration' in line or 'Saving' in line or 'Magic' in line or
                                           'Damage' in line or 'Accumulative' in line or 'Discipline' in line):
                            parts = line.split(':', 1)
                            if len(parts) == 2:
                                key = parts[0].strip(' >-')
                                value = clean_display_text(parts[1])
                                if value and key:
                                    info_dict[key] = value

                    # Display info grid
                    if info_dict:
                        html += '                            <div class="info-grid">\n'
                        for key in ['Usage', 'School of Magic', 'Discipline', 'Target(s)', 'Duration',
                                   'Saving Throw', 'Magic Resist', 'Damage Type', 'Accumulative']:
                            if key in info_dict:
                                html += f'                                <div class="info-label">{key}:</div>\n'
                                html += f'                                <div class="info-value">{info_dict[key]}</div>\n'
                        html += '                            </div>\n'

                    # Extract and display description (look for >Description: marker)
                    description = ''
                    desc_started = False
                    for line in lines:
                        if desc_started:
                            # Stop at "See also" section
                            if line.strip().startswith('>See also') or 'See also' in line:
                                break
                            description += line + '\n'
                        elif '>Description:' in line or ('>Description' in line and ':' in line):
                            desc_started = True
                            parts = line.split(':', 1)
                            if len(parts) == 2 and parts[1].strip():
                                description += parts[1].strip() + '\n'

                    if description.strip():
                        desc_lines = [line for line in description.strip().split('\n') if line.strip() and 'See also' not in line]
                        clean_desc = clean_display_text('\n'.join(desc_lines))
                        html += '                            <div class="spell-description">\n'
                        html += '                                <strong>📖 Description:</strong><br>\n'
                        html += f'                                <p>{clean_desc}</p>\n'
                        html += '                            </div>\n'

                    # Last updated
                    if help_entry.get('last_updated'):
                        html += f'                            <div style="margin-top: 12px; color: #6c757d; font-size: 11px;">Last Updated: {help_entry["last_updated"]}</div>\n'
                else:
                    html += '                            <div class="no-help">⚠️ No detailed help information available for this spell yet.</div>\n'

                html += '                        </div>\n'  # Close spell-item-details
                html += '                    </div>\n'  # Close spell-item

            html += '                </div>\n'  # Close level-section

        html += '            </div>\n'  # Close class-content
        html += '        </div>\n'  # Close class-details
        html += '    </div>\n\n'  # Close class-block

    # Footer
    html += """    <div style="margin-top: 50px; padding: 30px; background: white; border-radius: 12px; text-align: center;">
        <h3 style="color: #667eea;">Luminari MUD Spell Reference - By Class</h3>
        <p>For more information and to experience these spells in action,<br>
        visit <strong>Luminari MUD</strong></p>
        <p>
            <a href="#top" style="color: #667eea; text-decoration: none; font-weight: bold; margin: 0 15px;">⬆ Back to Top</a>
            <span style="color: #999;">|</span>
            <a href="spells_reference.html" style="color: #667eea; text-decoration: none; font-weight: bold; margin: 0 15px;">📖 View Spells Alphabetically</a>
        </p>
    </div>
</div>

</body>
</html>
"""

    return html


def main():
    print("Luminari MUD - Detailed Spell HTML Generator")
    print("=" * 50)

    # Extract spell data
    spells = extract_spells()

    # Get help entries
    help_data = get_help_entries()

    # Match help to spells
    matched = 0
    for spell_key in spells:
        if spell_key in help_data:
            spells[spell_key]['help'] = help_data[spell_key]
            matched += 1

    print(f"Matched {matched} spells with help entries")

    # Get class spell assignments
    class_spells = extract_class_spells()

    # Generate spell-organized HTML
    html = generate_html(spells, help_data, class_spells)

    # Write spell-organized file
    output_file = 'docs/spells_reference.html'
    with open(output_file, 'w') as f:
        f.write(html)

    print(f"\n✅ Spell HTML file generated: {output_file}")

    # Generate class-organized HTML
    class_html = generate_class_html(spells, help_data, class_spells)

    # Write class-organized file
    class_output_file = 'docs/spells_by_class.html'
    with open(class_output_file, 'w') as f:
        f.write(class_html)

    print(f"✅ Class HTML file generated: {class_output_file}")

    # Count spells that are assigned to classes (not effects or unimplemented)
    assigned_spells = set()
    for spell_name in class_spells:
        assigned_spells.add(spell_name)

    print(f"📊 Total spells documented: {len(assigned_spells)}")
    print(f"🎓 Total classes with spells: {len(set(cn for sl in class_spells.values() for cn, _ in sl))}")

if __name__ == '__main__':
    main()
