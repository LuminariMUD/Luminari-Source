# Luminari MUD - Detailed Spell HTML Generator

## Overview

This document describes the enhanced spell documentation generator that creates a comprehensive, web-friendly HTML reference for all spells in Luminari MUD.

## Generated File

**Location**: `docs/spells_reference.html`  
**Size**: ~464 KB  
**Lines**: 10,445  
**Spells Documented**: 431 total spells  
**Detailed Entries**: 68 spells with full information from help database

## Generator Script

**Location**: `util/generate_spell_html_detailed.py`  
**Language**: Python 3  
**Dependencies**: None (uses subprocess to call mysql CLI)

## Features

### 1. Data Sources

The generator extracts information from two primary sources:

- **spell_parser.c**: Parses `spello()` macro calls to extract all spell names and SPELL_CONSTANT identifiers
- **MySQL help_entries table**: Queries detailed help information for spells that have documentation

### 2. Spell Information Displayed

For each spell with help documentation, the HTML displays:

- **Spell Name**: Human-readable spell name
- **Spell ID**: The SPELL_CONSTANT identifier used in code
- **Keywords**: Related search terms from help_keywords table
- **Usage**: How to cast the spell in-game
- **School of Magic**: Evocation, Conjuration, Transmutation, etc.
- **Target(s)**: Single Target, Area Effect, Self, etc.
- **Duration**: How long the spell effect lasts
- **Saving Throw**: What save can reduce/negate the effect
- **Magic Resist**: Whether spell resistance applies
- **Damage Type**: Fire, Cold, Acid, etc.
- **Accumulative**: Whether the spell can be stacked
- **Description**: Full text description of what the spell does
- **Last Updated**: Timestamp from the help database

### 3. Navigation Features

- **Sticky Navigation Bar**: Alphabetical index (A-Z) that stays at the top while scrolling
- **Letter Headers**: Large section headers for each letter of the alphabet
- **Back to Top Links**: Easy navigation back to the top of the document
- **Anchor Links**: All sections are linkable via `#letter-X` anchors

### 4. Visual Design

- **Modern Gradient Background**: Purple gradient (from #667eea to #764ba2)
- **Card-Based Layout**: Each spell in a white card with rounded corners and shadows
- **Responsive Grid**: Structured information displayed in a clean grid layout
- **Color-Coded Elements**: 
  - Blue for spell names and headers
  - Yellow for missing help entries
  - Light blue backgrounds for descriptions
  - Gray for metadata
- **Keyword Tags**: Rounded tag design for spell keywords
- **Professional Typography**: Clean, readable fonts with proper spacing

### 5. Color Code Handling

The generator automatically strips MUD color codes (like `\tn`, `\tW`, `\tD`) from help text to ensure clean HTML output.

### 6. Smart Spell Matching

The system handles various spell name formats:
- Exact matches (case-insensitive)
- Hyphenated names (e.g., "acid-arrow" matches "acid arrow")
- Names without spaces (e.g., "acidarrow")

## Usage

### Running the Generator

```bash
cd /home/krynn/code
python3 util/generate_spell_html_detailed.py
```

### Prerequisites

- Python 3.x
- MySQL client (`mysql` command-line tool)
- Database credentials configured in `lib/mysql_config`
- Access to `krynn_prod` database with `help_entries` and `help_keywords` tables

### Output

The script will:
1. Parse `src/spell_parser.c` to extract all spell definitions
2. Query the MySQL database for help entries
3. Match spells with their help documentation
4. Generate `docs/spells_reference.html`
5. Report statistics:
   - Number of spells extracted
   - Number of help entries retrieved
   - Number of successful matches

### Example Output

```
Luminari MUD - Detailed Spell HTML Generator
==================================================
Extracted 432 spells from spell_parser.c
Retrieved 1619 help entries from database
Matched 68 spells with help entries

✅ HTML file generated: docs/spells_reference.html
📊 Total spells documented: 431
```

## Database Schema

### help_entries Table

- `tag` (varchar): Unique identifier for the help entry
- `entry` (text): Full help text with optional structured data
- `category` (varchar): Category classification
- `alternate_keywords` (text): Alternative search terms
- `min_level` (int): Minimum level to access the help
- `max_level` (int): Maximum level for the help
- `auto_generated` (boolean): Whether auto-generated
- `last_updated` (timestamp): Last modification time

### help_keywords Table

- `help_tag` (varchar): References help_entries.tag
- `keyword` (varchar): Associated keyword for searching

## Example Spell Entry (Fireball)

```html
<div class="spell-block">
    <div class="spell-header">
        <div class="spell-name">fireball</div>
        <div class="spell-id">SPELL_FIREBALL</div>
    </div>
    <div class="keyword-tag">🔑 FIREBALL</div>
    <div class="info-grid">
        <div class="info-label">Usage:</div>
        <div class="info-value">cast 'fireball' <target></div>
        <div class="info-label">School of Magic:</div>
        <div class="info-value">Evocation</div>
        <div class="info-label">Target(s):</div>
        <div class="info-value">Single Opponent</div>
        <div class="info-label">Duration:</div>
        <div class="info-value">N/A</div>
        <div class="info-label">Saving Throw:</div>
        <div class="info-value">Reflex</div>
        <div class="info-label">Magic Resist:</div>
        <div class="info-value">Resistable</div>
        <div class="info-label">Damage Type:</div>
        <div class="info-value">Fire</div>
    </div>
    <div class="spell-description">
        <p>This is a damaging fire spell.</p>
    </div>
</div>
```

## Maintenance

### Adding New Spells

New spells are automatically picked up when:
1. They are added to `spell_parser.c` using the `spello()` macro
2. Re-run the generator script

### Adding Help Documentation

To add detailed information for a spell:
1. Add/update entry in the `help_entries` table
2. Ensure the `tag` field matches the spell name (with or without hyphens)
3. Format the entry with structured fields:
   - Usage:
   - School of Magic:
   - Target(s):
   - Duration:
   - Saving Throw:
   - Magic Resist:
   - Damage Type:
   - Description:
4. Add keywords to `help_keywords` table
5. Re-run the generator script

### Updating Styles

To modify the HTML appearance, edit the `<style>` section in the `generate_html()` function in `util/generate_spell_html_detailed.py`.

## Technical Details

### Color Code Pattern

The generator removes MUD color codes matching the pattern: `\t[a-zA-Z0-9]`

### Structured Data Parsing

The system looks for lines containing these keywords followed by a colon:
- Usage
- School (of Magic)
- Target
- Duration
- Saving (Throw)
- Magic (Resist)
- Damage (Type)
- Accumulative
- Discipline

### MySQL Connection

The script uses subprocess to call the `mysql` command-line client rather than requiring Python MySQL libraries. This makes it more portable and avoids dependency issues.

## Future Enhancements

Potential improvements for future versions:

1. **Spell Level Information**: Extract `min_level` data from spell_info struct
2. **PSP/Mana Costs**: Add resource costs for casting spells
3. **Class Availability**: Show which classes can learn each spell
4. **Spell Components**: Display material/somatic/verbal requirements
5. **Related Spells**: Link to similar or prerequisite spells
6. **Search Functionality**: Add JavaScript search/filter capability
7. **Mobile Optimization**: Improve responsive design for small screens
8. **PDF Export**: Add option to generate PDF version
9. **Spell Categories**: Group spells by school or type
10. **Comparison Tool**: Side-by-side spell comparison feature

## Credits

**Created**: October 2024  
**Author**: AI Assistant (GitHub Copilot)  
**Project**: Luminari MUD  
**Purpose**: Player documentation and reference

## License

This tool is part of the Luminari MUD project and follows the same license as the main codebase.
