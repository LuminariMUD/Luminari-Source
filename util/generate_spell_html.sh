#!/bin/bash
# Generate spell HTML documentation from game output

cd /home/krynn/code

# Create a simple temp file to execute in game
cat > /tmp/spell_dump.txt << 'EOF'
# This would need to be run from within the game
# For now, let's extract spell data from spell_parser.c
EOF

# Extract spell information from spell_parser.c
echo "Generating spell HTML from spell_parser.c..."

python3 << 'PYEOF'
import re
import sys
from datetime import datetime

# Read spell_parser.c
with open('src/spell_parser.c', 'r', encoding='latin-1') as f:
    content = f.read()

# Find all spello() calls
spell_pattern = r'spello\(\s*SPELL_(\w+),\s*"([^"]+)"'
matches = re.findall(spell_pattern, content)

print(f"Found {len(matches)} spells")

# Create HTML output
html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Luminari MUD - Spell Reference</title>
    <style>
        body {{ font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }}
        h1 {{ color: #333; border-bottom: 3px solid #4CAF50; padding-bottom: 10px; }}
        .spell-block {{ background-color: white; margin: 15px 0; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .spell-name {{ font-size: 20px; font-weight: bold; color: #2196F3; }}
        .spell-id {{ color: #999; font-size: 14px; margin-left: 10px; }}
        .alpha-header {{ font-size: 28px; font-weight: bold; color: #4CAF50; margin-top: 30px; padding: 10px; background-color: white; border-radius: 8px; }}
        .navbar {{ position: sticky; top: 0; background-color: #333; padding: 10px; margin: -20px -20px 20px -20px; z-index: 1000; }}
        .navbar a {{ color: white; text-decoration: none; margin: 0 10px; }}
        .navbar a:hover {{ color: #4CAF50; }}
    </style>
</head>
<body>

<div class="navbar">
    <a href="#top">Top</a>
"""

# Add alphabet navigation
for letter in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
    html += f'    <a href="#letter-{letter}">{letter}</a>\n'

html += """</div>

<h1 id="top">Luminari MUD - Spell Reference Guide</h1>
<p>Generated: """ + datetime.now().strftime('%Y-%m-%d') + f"""</p>
<p>Total Spells: {len(matches)}</p>

"""

# Sort spells alphabetically
sorted_spells = sorted(matches, key=lambda x: x[1].lower())

# Group by first letter
current_letter = ''
for spell_id, spell_name in sorted_spells:
    first_letter = spell_name[0].upper() if spell_name else 'A'
    
    if first_letter != current_letter:
        current_letter = first_letter
        html += f'<div class="alpha-header" id="letter-{current_letter}">{current_letter}</div>\n\n'
    
    html += f'''<div class="spell-block">
    <span class="spell-name">{spell_name}</span>
    <span class="spell-id">(SPELL_{spell_id})</span>
</div>

'''

html += """
<div style="margin-top: 50px; padding: 20px; background-color: white; border-radius: 8px; text-align: center;">
    <p><strong>Luminari MUD Spell Reference</strong></p>
    <p>For detailed spell information, use the 'spellinfo' command in-game.</p>
    <p><a href="#top">Back to Top</a></p>
</div>

</body>
</html>
"""

# Write HTML file
with open('docs/spells_reference.html', 'w') as f:
    f.write(html)

print("HTML file generated: docs/spells_reference.html")
print(f"Total spells: {len(sorted_spells)}")

PYEOF

