# Spells by Class HTML Reference

## Overview

Created a second HTML file (`spells_by_class.html`) that organizes all spell information by class instead of alphabetically by spell name. This provides a class-centric view perfect for players planning character builds or comparing class spell lists.

## New File Details

### File Information
- **Location**: `docs/spells_by_class.html`
- **Size**: 1.9 MB
- **Lines**: 29,208
- **Classes Documented**: 11
- **Total Spell Listings**: Multiple (same spells appear under each class that can cast them)

### Classes Included
1. Alchemist (56 spells)
2. Bard
3. Cleric
4. Druid
5. Inquisitor
6. Paladin
7. Ranger
8. Sorcerer
9. Summoner
10. Wizard (214 spells)
11. And more...

## Features

### 1. Class-Level Organization
- **Primary Grouping**: By class name
- **Secondary Grouping**: By level within each class
- **Tertiary Sorting**: Alphabetically within each level

### 2. Collapsible Interface
- **Class Headers**: Click to expand/collapse entire class spell list
- **Individual Spells**: Click spell name to see detailed information
- **Nested Collapsing**: Two levels of collapse for clean interface

### 3. Visual Design

#### Class Headers
- Large gradient purple header (matches main theme)
- Class name in large white text with shadow
- Spell count displayed (e.g., "214 spells available")
- Animated arrow icon (▼ becomes ▲ when expanded)

#### Level Sections
- Clear level headers (e.g., "Level 5 Spells")
- Blue color scheme with left border accent
- Grouped spells by acquisition level

#### Spell Items
- White cards with hover effects
- Slide-in animation on hover
- Compact display: spell name + constant
- Expandable for full details

### 4. Navigation

#### Top Navigation Bar
- **Top Link**: Return to page top
- **View by Spell**: Link to alphabetical spell list
- **Class Links**: Quick jump to each class section
- **Visual Separators**: Bullets (•) between classes

#### Footer Links
- Back to Top
- Link to alphabetical spell view

### 5. Spell Details

Each spell shows (when expanded):
- **Keywords**: Related search terms
- **Info Grid**: Usage, School, Targets, Duration, Saves, Magic Resist, Damage Type
- **Description**: Full spell description
- **Last Updated**: Timestamp from database

## Usage Examples

### For Players Planning a Build
1. Click on "Wizard" to see all wizard spells
2. Scroll to "Level 5 Spells" section
3. Click "fireball" to see full details
4. Compare with Sorcerer's version (listed at Level 6)

### For Comparing Classes
1. Open "Cleric" to see healing spells
2. Open new tab with "Druid"
3. Compare spell availability and levels
4. See which class gets spells earlier

### For Level Planning
1. Expand your class
2. See spells grouped by level
3. Plan which levels are important for your build
4. Know exactly when you'll get key spells

## Technical Implementation

### Data Organization

```python
class_spell_map = {
    'Wizard': [
        ('fireball', 5, spell_data),
        ('lightning bolt', 5, spell_data),
        # ... more spells
    ],
    'Sorcerer': [
        ('fireball', 6, spell_data),
        # ... more spells
    ]
}
```

### HTML Structure

```html
<div class="class-block" id="class-Wizard">
    <div class="class-header" onclick="toggleClass('Wizard')">
        <div class="class-name">Wizard</div>
        <div class="class-spell-count">214 spells available</div>
        <div class="toggle-icon">▼</div>
    </div>
    <div class="class-details">
        <div class="class-content">
            <div class="level-section">
                <div class="level-header">Level 5 Spells</div>
                <div class="spell-item" onclick="toggleSpell('Wizard-spell42')">
                    <div class="spell-item-name">fireball</div>
                    <div class="spell-item-constant">SPELL_FIREBALL</div>
                    <div class="spell-item-details">
                        <!-- Full spell information here -->
                    </div>
                </div>
            </div>
        </div>
    </div>
</div>
```

### JavaScript Functions

```javascript
// Toggle entire class spell list
function toggleClass(classId) {
    const details = document.getElementById('class-details-' + classId);
    const icon = document.getElementById('class-icon-' + classId);
    // Add/remove 'open' class
}

// Toggle individual spell details
function toggleSpell(spellId) {
    const details = document.getElementById('spell-details-' + spellId);
    const icon = document.getElementById('spell-icon-' + spellId);
    // Add/remove 'open' class
}
```

## Cross-Linking

Both HTML files link to each other:

### In spells_reference.html (alphabetical)
- **Navbar**: "🎓 View by Class" link
- **Footer**: Link to class-organized view

### In spells_by_class.html (by class)
- **Navbar**: "📖 View by Spell" link
- **Footer**: Link to alphabetical spell view

This allows easy switching between organizational schemes.

## File Comparison

| Feature | spells_reference.html | spells_by_class.html |
|---------|----------------------|----------------------|
| Organization | Alphabetical by spell | By class, then level |
| Size | 1.0 MB | 1.9 MB |
| Lines | 18,413 | 29,208 |
| Primary Use | Looking up specific spell | Planning class build |
| Navigation | A-Z letters | Class names |
| Duplication | Each spell once | Spell per class |

## Generation

Both files are generated by the same script:

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

✅ Spell HTML file generated: docs/spells_reference.html
✅ Class HTML file generated: docs/spells_by_class.html
📊 Total spells documented: 431
🎓 Total classes with spells: 11
```

## Benefits

### For Players
1. **Build Planning**: See entire spell progression for your class
2. **Level Goals**: Know what spells you'll get at each level
3. **Class Comparison**: Easy to compare spell availability
4. **Multiclass Planning**: See spell lists for multiple classes
5. **Quick Reference**: Fast lookup of class capabilities

### For Developers
1. **Class Balance**: Visualize spell distribution across classes
2. **Level Distribution**: See if spells are well-distributed by level
3. **Spell Gaps**: Identify levels with too few/many spells
4. **Duplicate Detection**: See which spells are shared across classes
5. **Documentation**: Complete class spell list reference

## Example Use Cases

### Scenario 1: New Player
*"I want to play a spellcaster but don't know which class"*
- Open spells_by_class.html
- Click through Wizard, Sorcerer, Cleric, Druid
- Compare spell lists and acquisition levels
- Make informed class choice

### Scenario 2: Level Planning
*"I'm level 4 Wizard, what do I get at level 5?"*
- Navigate to Wizard
- Scroll to "Level 5 Spells"
- See: fireball, lightning bolt, flame arrow, etc.
- Plan character advancement

### Scenario 3: Multiclass Build
*"Can both my Wizard and Sorcerer levels cast fireball?"*
- Check Wizard section: fireball at level 5
- Check Sorcerer section: fireball at level 6
- Understand interaction between class levels

### Scenario 4: Spell Research
*"Which classes can cast healing spells?"*
- Open each class
- Look for cure light, cure serious, heal, etc.
- See: Cleric (L1), Druid (L1), Paladin (L6), Ranger (L6)

## Style Features

### Color Scheme
- **Background**: Purple gradient (matches main site)
- **Class Headers**: Purple gradient with white text
- **Level Headers**: Blue accent with gray background
- **Spell Items**: White cards with blue accents
- **Hover Effects**: Transform and color changes

### Responsive Design
- Flexbox layouts for content wrapping
- Proper spacing for readability
- Mobile-friendly collapsible sections
- Touch-friendly click targets

### Typography
- **Headers**: Large, bold, shadowed
- **Spell Names**: Medium bold blue
- **Details**: Smaller, readable body text
- **Monospace**: For spell constants

## Future Enhancements

Potential additions:
1. **Filter by Level**: Show only spells for specific level range
2. **Search Function**: Live search within class
3. **Comparison Mode**: Side-by-side class comparison
4. **Spell Circle View**: Group by spell circles (1st-9th)
5. **Print Stylesheet**: Optimized for printing class spell list
6. **Export Function**: Download class spell list as PDF/text
7. **Spell Progression Chart**: Visual timeline of spell acquisition
8. **Prerequisites Display**: Show any feat/ability requirements
9. **Related Spells**: Link to similar spells in other classes
10. **Statistics**: Spell count by level graph for each class

## Credits

**Created**: October 15, 2025  
**Feature**: Class-organized spell reference  
**Generator**: `util/generate_spell_html_detailed.py`  
**Files Created**:
- `docs/spells_reference.html` (alphabetical)
- `docs/spells_by_class.html` (by class)  
**Data Sources**:
- `src/spell_parser.c`
- `src/class.c`
- MySQL `help_entries` table
