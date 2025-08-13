# Help System Import Functionality - Implementation Plan

## Overview
Add an "import" option to the `helpgen` command that will import the legacy help.hlp file content into the MySQL database with intelligent duplicate handling for entries that share primary keywords.

## Command Syntax
```
helpgen import [preview|force|merge]
  preview - Show what would be imported without making changes
  force   - Overwrite existing entries completely  
  merge   - Intelligently merge duplicate keywords (default)
```

## Implementation Requirements

### 1. Parser for help.hlp Format
The help.hlp file has a specific format:
- Keywords on first line (space-separated)
- Help content follows
- Entry ends with `#` followed by minimum level number
- Example:
```
! ^ !

Use ! to repeat the last command typed at the keyboard.
...
#0
```

### 2. Import Function Components

#### A. File Parser (`parse_help_hlp_file`)
- Open and read lib/text/help/help.hlp
- Parse each entry extracting:
  - Keywords (first line, space-separated)
  - Content (all lines until #)
  - Minimum level (number after #)
- Handle special characters and formatting codes
- Validate each entry before processing

#### B. Duplicate Detection (`check_existing_help_entry`)
- For each keyword in the entry:
  - Query database for existing entries with that keyword
  - Track which keywords already exist
  - Identify primary vs secondary keywords

#### C. Conflict Resolution Strategies

##### Strategy 1: Skip (default for preview)
- If ANY keyword exists, skip the entire entry
- Report which entries would be skipped

##### Strategy 2: Force Overwrite
- Delete existing entries that share keywords
- Insert new entry fresh
- Maintain referential integrity

##### Strategy 3: Intelligent Merge (recommended)
- For entries with shared keywords:
  - Compare content similarity
  - If content is substantially different:
    - Create new entry with suffix (e.g., "help_legacy")
    - Add cross-references in both entries
  - If content is similar:
    - Merge unique content sections
    - Preserve higher detail version
    - Update timestamp

#### D. Database Operations (`import_help_entry_to_db`)
- Use prepared statements for security
- Transaction support for atomicity
- Generate unique tags for entries:
  - Use primary keyword as base
  - Add suffix if duplicate (_2, _3, etc.)
  - Maintain tag uniqueness constraint

### 3. Special Handling Cases

#### Multiple Keywords per Entry
- First keyword becomes primary tag
- Additional keywords stored in help_keywords table
- All keywords searchable

#### Shared Primary Keywords
When multiple entries share the same primary keyword:
1. **Disambiguation Page**: Create a main entry that lists all variations
2. **Suffix System**: Add descriptive suffixes (e.g., "cast_spell" vs "cast_fishing")
3. **Context Merging**: If appropriate, merge into single comprehensive entry

#### Format Conversion
- Convert color codes properly
- Handle line endings consistently  
- Preserve formatting tags
- Clean up legacy artifacts

### 4. Safety Features

#### Preview Mode
- Dry run showing:
  - Number of entries to import
  - Conflicts detected
  - Proposed resolutions
  - No database changes

#### Backup Recommendation
- Suggest database backup before import
- Option to export existing help to file

#### Rollback Support
- Mark imported entries with import batch ID
- Allow rollback of specific import batch

### 5. Progress Reporting
- Show progress during import (X of Y entries)
- Report:
  - Successful imports
  - Skipped entries
  - Merged entries
  - Errors encountered
- Log detailed results to file

### 6. Code Structure

```c
/* Main import handler */
static int import_help_hlp_file(struct char_data *ch, const char *mode) {
  // Open file
  // Parse entries
  // Process based on mode
  // Report results
}

/* Parse single entry from file */
static struct help_entry_list *parse_help_entry(FILE *fp) {
  // Read keywords line
  // Read content until #
  // Parse min level
  // Return structured entry
}

/* Check for existing entries */
static int check_duplicate_keywords(struct help_entry_list *entry) {
  // Query database for each keyword
  // Return conflict type
}

/* Import single entry with conflict resolution */
static int import_entry_with_resolution(struct help_entry_list *entry, int conflict_mode) {
  // Handle based on conflict mode
  // Insert/update database
  // Return success/failure
}
```

### 7. Testing Considerations
- Test with empty database
- Test with partial overlaps
- Test with complete duplicates
- Test special characters
- Test very long entries
- Test malformed file entries

### 8. Future Enhancements
- Import from other MUD formats
- Export current database to help.hlp format
- Bulk edit capabilities
- Version control for help entries
- Automatic cross-reference generation