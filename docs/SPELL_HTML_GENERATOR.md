# Spell Reference HTML Generator

## Date: October 15, 2025

## Overview
Created an HTML reference document listing all 447 spells in the Luminari MUD alphabetically.

## Generated File
**Location:** `/home/krynn/code/docs/spells_reference.html`

**Size:** 65KB, 2,345 lines

**Total Spells:** 447

## Features

### Navigation
- Sticky navigation bar at top with alphabet links
- Click any letter (A-Z) to jump to spells starting with that letter
- "Back to Top" link at bottom

### Spell Display
- Each spell shown in a clean card layout
- Spell name displayed prominently in blue
- Spell constant ID shown in gray (e.g., SPELL_FIREBALL)
- Grouped alphabetically with letter headers

### Styling
- Modern, clean design with white cards on gray background
- Responsive layout
- Professional color scheme:
  - Green for headers (#4CAF50)
  - Blue for spell names (#2196F3)
  - White cards with subtle shadows
  - Dark navigation bar (#333)

## Sample Spells Included

### A
- acid arrow
- acid fog
- acid sheath
- acid splash
- aid
- air walk
- animal shapes
- animate dead
- anti magic field

### B
- banish
- barkskin
- battletide
- bestow curse
- blade barrier
- bless
- blight
- blindness
- blur

### C-Z
- (and 400+ more spells...)

## Generation Method

### Script Created
`/home/krynn/code/util/generate_spell_html.sh`

This Bash script:
1. Reads `src/spell_parser.c`
2. Extracts all `spello()` function calls
3. Parses spell names and IDs using Python
4. Generates clean HTML with proper formatting
5. Sorts spells alphabetically
6. Groups by first letter
7. Outputs to `docs/spells_reference.html`

### How to Regenerate
```bash
cd /home/krynn/code
bash util/generate_spell_html.sh
```

## Technical Details

### Data Source
Spells are extracted from `/home/krynn/code/src/spell_parser.c` where they are defined using the `spello()` macro.

### Pattern Matching
Uses regex pattern: `spello\(\s*SPELL_(\w+),\s*"([^"]+)"`

This captures:
- Spell constant name (e.g., `FIREBALL`)
- Spell display name (e.g., `"fireball"`)

### Python Processing
The Python inline script:
1. Reads spell_parser.c
2. Finds all spell definitions
3. Sorts alphabetically (case-insensitive)
4. Groups by first letter
5. Generates complete HTML document

## Viewing the File

### In Browser
Open in any web browser:
```bash
firefox /home/krynn/code/docs/spells_reference.html
# or
xdg-open /home/krynn/code/docs/spells_reference.html
```

### On Web Server
If served via web:
```
http://your-server/docs/spells_reference.html
```

## Future Enhancements

### Potential Additions
To add more detailed information, the script would need to:

1. **Parse spell_info struct data:**
   - School of magic
   - Casting time
   - PSP/mana costs
   - Minimum character level per class
   - Targets (self, room, combat, etc.)
   - Violent flag
   - Wear-off messages

2. **Extract spell descriptions:**
   - Would need to parse spell implementation functions
   - Add SPELL_*.c file descriptions
   - Include help file entries

3. **Add filtering/search:**
   - Filter by spell school
   - Filter by class
   - Filter by level
   - Search by name

4. **Add comparison tools:**
   - Compare similar spells
   - Show spell progression (level 1→9 variants)
   - Group by school or class

### Example Enhanced Entry
```html
<div class="spell-block">
    <div class="spell-name">Fireball</div>
    <div class="spell-id">(SPELL_FIREBALL)</div>
    <div class="spell-info">
        <strong>School:</strong> Evocation<br>
        <strong>Level:</strong> Wizard 3, Sorcerer 3<br>
        <strong>Casting Time:</strong> 1 action<br>
        <strong>Range:</strong> Long (400ft + 40ft/level)<br>
        <strong>Duration:</strong> Instantaneous<br>
        <strong>Saving Throw:</strong> Reflex half<br>
        <strong>Description:</strong> A burst of flame erupts...
    </div>
</div>
```

## Files Created/Modified

1. **`/home/krynn/code/util/generate_spell_html.sh`** - Generator script
2. **`/home/krynn/code/docs/spells_reference.html`** - Output HTML file

## Notes

- The HTML includes some "!UNUSED!" entries for spell slots that are defined but not implemented
- Spells are sorted case-insensitively for better alphabetical ordering
- The page is fully self-contained (no external CSS/JS dependencies)
- Works in all modern browsers
- Mobile-responsive design

## Credits
- Generator Script: GitHub Copilot Assistant
- Spell Data: Luminari MUD spell_parser.c

---
**Status:** Complete  
**File Size:** 65KB  
**Spell Count:** 447  
**Format:** HTML5
