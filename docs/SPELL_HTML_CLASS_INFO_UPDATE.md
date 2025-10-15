# Class Information Added to Spell Reference

## Overview

Updated the spell HTML generator to include which classes can cast each spell and at what level they learn it. This information is extracted directly from the `class.c` file where spell assignments are defined using the `spell_assignment()` function.

## Update Summary

### Changes Made (October 15, 2025)

1. **New Function**: `extract_class_spells()`
   - Parses `src/class.c` for all `spell_assignment()` calls
   - Maps class constants (e.g., `CLASS_WIZARD`) to human-readable names
   - Builds a dictionary of spells to class/level pairs
   - Extracted assignments for **408 spells**

2. **Class Display Section**
   - Added gold-themed section showing "Available to Classes"
   - Displays class name and level in rounded badge format
   - Shows for both spells with and without help documentation
   - Sorted by level first, then alphabetically by class name

3. **Comprehensive Class Support**
   - Wizard, Cleric, Rogue, Warrior
   - Monk, Druid, Berserker, Sorcerer
   - Paladin, Blackguard, Ranger, Bard
   - Psionicist, Weapon Master, Arcane Archer
   - Arcane Shadow, Eldritch Knight, Spellsword
   - Sacred Fist, Stalwart Defender
   - Alchemist, Inquisitor, Summoner, Necromancer, Shadow Dancer

## Example Output

### Fireball
Shows the spell is available to:
- **Wizard** (Level 5)
- **Sorcerer** (Level 6)

### Cure Light Wounds
Shows the spell is available to:
- **Alchemist** (Level 1)
- **Bard** (Level 1)
- **Cleric** (Level 1)
- **Druid** (Level 1)
- **Inquisitor** (Level 1)
- **Paladin** (Level 6)
- **Ranger** (Level 6)

## Visual Design

The class information appears in a distinctive gold/orange themed box:
- **Background**: Light yellow (#fff8e1)
- **Border**: 4px solid gold (#ffa000) on the left
- **Badge Style**: White background with gold border
- **Icon**: 🎓 graduation cap emoji
- **Layout**: Flexbox with wrapping for responsive display

## Technical Details

### Data Extraction

From `class.c`, the pattern matched is:
```c
spell_assignment(CLASS_NAME, SPELL_NAME, level);
```

For example:
```c
spell_assignment(CLASS_WIZARD, SPELL_FIREBALL, 5);
spell_assignment(CLASS_SORCERER, SPELL_FIREBALL, 6);
```

### Spell Name Matching

The system tries multiple matching strategies:
1. Exact match (lowercase spell name)
2. With hyphens (e.g., "acid-arrow" for "acid arrow")
3. Without spaces (e.g., "acidarrow" for "acid arrow")

This ensures maximum compatibility with the various naming conventions used in the codebase.

### HTML Structure

```html
<div style="margin: 20px 0; padding: 15px; background: #fff8e1; 
     border-radius: 8px; border-left: 4px solid #ffa000;">
    <strong>🎓 Available to Classes:</strong><br>
    <div style="margin-top: 10px; display: flex; flex-wrap: wrap; gap: 8px;">
        <span style="background: #fff; padding: 6px 12px; border-radius: 15px; 
              border: 2px solid #ffa000; font-size: 13px;">
            <strong>Wizard</strong> (Level 5)
        </span>
        <!-- More class badges... -->
    </div>
</div>
```

## Statistics

### File Information
- **File Size**: ~1 MB (1009 KB)
- **Line Count**: 18,406 lines
- **Total Spells**: 431 documented
- **Spells with Help**: 68 detailed entries
- **Spells with Class Info**: 408 spells

### Extraction Results
- **Spells from spell_parser.c**: 432 extracted
- **Help entries from database**: 1,619 retrieved
- **Spell-to-help matches**: 68 matched
- **Class assignments**: 408 spells have class data

## Benefits

1. **Player Information**: Players can quickly see which classes can learn each spell
2. **Level Planning**: Shows at what level each class gains access
3. **Class Comparison**: Easy to see which classes share spells
4. **Build Planning**: Helps players plan character progression
5. **Complete Reference**: Combined with other spell info provides full picture

## Code Location

- **Generator Script**: `util/generate_spell_html_detailed.py`
- **Output File**: `docs/spells_reference.html`
- **Data Sources**: 
  - `src/spell_parser.c` (spell definitions)
  - `src/class.c` (class spell assignments)
  - MySQL `help_entries` table (spell descriptions)

## Usage

To regenerate the HTML with class information:

```bash
cd /home/krynn/code
python3 util/generate_spell_html_detailed.py
```

Output:
```
Luminari MUD - Detailed Spell HTML Generator
==================================================
Extracted 432 spells from spell_parser.c
Retrieved 1619 help entries from database
Matched 68 spells with help entries
Extracted spell assignments for 408 spells from class.c

✅ HTML file generated: docs/spells_reference.html
📊 Total spells documented: 431
```

## Future Enhancements

Possible additions:
1. **Multi-class Filtering**: Allow filtering by class
2. **Level Range**: Show only spells available at certain levels
3. **Spell Progression**: Timeline of when each class learns spells
4. **Class Comparisons**: Side-by-side spell list comparisons
5. **Spell Circles**: Group by spell circle/level
6. **Caster Level Info**: Show caster level requirements beyond class level
7. **Prerequisites**: Display any feat or ability prerequisites
8. **Spell Slots**: Information about spell slots/memorization

## Notes

- Some spells may not have class assignments if they're NPC-only or triggered by items/abilities
- The sorting prioritizes level first, so you can quickly see which classes get the spell earliest
- Classes that appear as constants (e.g., CLASS_ALCHEMIST) are now properly mapped to friendly names
- The collapsible format keeps the interface clean while still displaying this important information

## Credits

**Updated**: October 15, 2025  
**Feature**: Class spell availability information  
**Data Source**: `class.c` spell_assignment() calls  
**Generator**: `util/generate_spell_html_detailed.py`  
**Output**: `docs/spells_reference.html`
