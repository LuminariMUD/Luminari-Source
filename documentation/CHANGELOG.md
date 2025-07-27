# CHANGELOG

## 2025-07-27

### Features
- **Enhanced Score Display (do_skore) - Phase 2.6 & 2.7 Complete**: Implemented layout templates and custom section ordering for the enhanced character display:
  - **Phase 2.6 - Layout Templates**: Added predefined layouts optimized for different playstyles
    - Added `score_layout_template` field to player_special_data_saved
    - Defined 5 layout templates: Default, Combat, Roleplay, Explorer, Caster
    - Each template reorders sections to prioritize relevant information
    - Implemented `scoreconfig template <name>` command
    - Added `get_layout_template()` function with template section arrays
  - **Phase 2.7 - Section Ordering**: Added custom section ordering capability
    - Added `score_section_order[8]` array to player_special_data_saved
    - Defined SECTION_* enums for all 8 display sections
    - Implemented `scoreconfig order <section> <position>` command
    - Added section order initialization in db.c for new characters
    - Reset command now restores both template and custom ordering
  - **Implementation Details:**
    - All preferences persist via binary player file saves
    - Custom ordering overrides template selection when modified
    - Infrastructure in place for future dynamic section reordering
- **Enhanced Score Display (do_skore) - Phase 2.4 & 2.5 Complete**: Extended the enhanced character display with context detection and active effects display:
  - **Phase 2.4 - Context Detection**: Added automatic context awareness to reorder sections based on character activity
    - Implemented `get_display_context()` function detecting combat, exploring, roleplay, and shopping contexts
    - Combat context prioritizes combat stats, vitals, and abilities at the top
    - Shopping context (future) prioritizes wealth and equipment sections
    - Exploring context (sneaking/hiding) prioritizes vitals and abilities
    - Added context constants to structs.h: CONTEXT_NORMAL, CONTEXT_COMBAT, CONTEXT_ROLEPLAY, CONTEXT_EXPLORING, CONTEXT_SHOPPING, CONTEXT_CRAFTING
    - Section reordering logic implemented with section_order[] array manipulation
  - **Phase 2.5 - Active Effects Display**: Added visual spell effect tracking to magic section
    - Implemented `display_active_effects()` function showing all active spell affects
    - Duration progress bars with color coding: green (>1hr), yellow (10-60min), red (<10min)
    - Permanent effects display with full white bar
    - Expiring effects show warning indicator
    - Integrated into magic section after spell slots and psionic information
    - Shows spell number and remaining duration in minutes
  - **Implementation Details:**
    - Context detection uses ch->char_specials.fighting for combat detection
    - Active effects use ch->affected linked list traversal
    - Progress bars scale based on 24-hour maximum duration
    - All features maintain C90 compliance with proper variable declarations
- **Enhanced Score Display (do_skore) - Phase 2.3 Complete**: Added new color themes to the enhanced character display system:
  - **Phase 2.3 - New Color Themes**: Implemented three additional color themes for accessibility and preference
    - **High Contrast**: Bold distinct colors for improved visibility
    - **Dark**: Muted color palette for reduced eye strain
    - **Colorblind**: Carefully selected colors avoiding red-green combinations
    - Color themes now include: Enhanced (default), Classic, Minimal, High Contrast, Dark, Colorblind
    - Updated `get_health_color()` and `get_class_color()` functions with theme-specific mappings
    - Extended `scoreconfig theme` command to support all six themes
    - Toggle with `scoreconfig theme <enhanced|classic|minimal|highcontrast|dark|colorblind>`
  - **Implementation Details:**
    - Added theme constants SCORE_THEME_HIGHCONTRAST (3), SCORE_THEME_DARK (4), SCORE_THEME_COLORBLIND (5) to structs.h
    - Updated theme-aware color functions in act.informative.c
    - Enhanced scoreconfig validation and display for new themes
- **Enhanced Score Display (do_skore) - Phase 2.1 & 2.2 Complete**: Extended the enhanced character display with visual polish features:
  - **Phase 2.1 - Class Borders**: Added decorative borders themed by character class
    - Warriors/Berserkers/Monks: Red borders (╔═══╗)
    - Wizards/Sorcerers/Bards: Blue borders
    - Clerics/Druids/Paladins/Rangers: Green borders
    - Rogues: Magenta borders
    - Other classes: Cyan borders
    - Toggle with `scoreconfig borders on/off`
  - **Phase 2.2 - Race Symbols**: Added unique symbols for each race
    - Each race has a distinctive symbol (Human: ☺, Elf: ♠, Dwarf: ♦, etc.)
    - Symbols display next to race name in identity panel
    - Toggle with `scoreconfig symbols on/off`
  - **Implementation Details:**
    - Added PRF_SCORE_BORDERS (81) and PRF_SCORE_RACE_SYMBOLS (82) flags
    - Implemented get_class_border_style() and get_race_symbol() functions
    - Updated preference_bits array in constants.c
    - Extended scoreconfig command with borders and symbols options
- **Enhanced Score Display (do_skore) - Phase 1 MVP Complete**: Implemented a new enhanced character display command `do_skore` as an alternative to the classic `do_score`. Features include:
  - Visual progress bars for HP/Movement/PSP with health-based color coding
  - Class-based color themes for different character types
  - Spell slot tracking for all casting classes showing used/total slots
  - Section-specific views: `skore combat`, `skore magic`, `skore stats`
  - Customizable display via `scoreconfig` command (width, theme, density, colors)
  - Performance monitoring integration with PERF_PROF_ENTER/EXIT macros
  - Preferences persist via binary player file saves
  - Responsive layout supporting 80/120/160 character widths
  - Organized information panels: Identity, Vitals, Experience, Abilities, Combat, Magic, Wealth, Equipment
  - **Implementation Details:**
    - ACMD(do_skore) implemented in act.informative.c:3848
    - ACMD(do_scoreconfig) implemented in act.informative.c:4194
    - Commands registered in interpreter.c:785-786
    - Preferences stored in player_special_data_saved (4 bytes added)
    - PRF_FLAGS added: PRF_SCORE_CLASSIC (77), PRF_SCORE_COMPACT (78), PRF_SCORE_WIDE (79), PRF_SCORE_NOCOLOR (80)
    - Helper functions: get_health_color() and get_class_color()
    - Development footer removed for production-ready display
- **Spell Collection Display Stacking**: Implemented spell stacking in the spell collection display for all spell-preparing classes (Wizard, Cleric, Druid, Ranger, Paladin, Blackguard, Alchemist). Modified `print_collection()` in spell_prep.c to group identical spells (same spell, metamagic, and domain) and display them with a count (e.g., "ball of lightning [quickened] x7"). The preparation queue display remains unchanged to preserve individual spell ordering for future prioritization features.

### Bug Fixes
- **Fixed compute_spell_res() NULL Pointer Crash**: Fixed crashes in `compute_spell_res()` function in magic.c by adding NULL check for victim parameter and correcting incorrect function call in `do_skore`. The function now safely returns 0 when victim is NULL instead of attempting to dereference it.
- **Fixed Array Bounds Violation in Spell Slots**: Fixed critical array bounds violation in `assign_feat_spell_slots()` function in spell_prep.c that could cause segmentation faults. Added bounds checking to ensure level_counter doesn't exceed the slot array size (84 entries), and added safety checks to prevent negative array indexing when accessing level_counter-1. This prevents crashes when character levels exceed expected bounds due to bonuses or other modifiers.
- **Fixed Use-After-Free in Spell Preparation Event**: Fixed critical use-after-free vulnerability in `event_preparation()` function in spell_prep.c where the event system could access a freed character pointer. Added validation to check if the character still exists in the global character_list before processing the event. This prevents crashes when a character is extracted/freed while having active spell preparation events.
- **Fixed Domain Array Bounds Checking**: Fixed potential array bounds violations in spell_prep.c when accessing domain_list array. Added validation to ensure domain values are within valid range (0 < domain < NUM_DOMAINS) before array access in three locations: print_prep_queue(), print_innate_magic_queue(), and print_collection(). This prevents crashes from invalid domain values.
- **Fixed Integer Underflow in star_circlet_proc()**: Fixed potential crash in `star_circlet_proc()` function in spell_prep.c where dice() could be called with num_classes=0. Added validation to check if character has any classes (num_classes > 0) before calling dice(1, num_classes). This prevents crashes when processing spell restoration for characters with no class levels.
- **Fixed Integer Overflow in Caster Level Calculation**: Fixed potential integer overflow in `known_spells_add()` function in spell_prep.c where CLASS_LEVEL + BONUS_CASTER_LEVEL could exceed array bounds. Added overflow protection that caps caster_level at 95 (safe maximum for spell known arrays) and checks for negative values. This prevents array out-of-bounds access when characters have excessive level bonuses.
- **Fixed Dangling Pointer in spell_counts Array**: Fixed potential use-after-free issue in `print_collection()` function in spell_prep.c where spell_counts array pointers were not set to NULL after freeing memory. Added NULL assignment after cleanup to prevent dangling pointer access if the array is accidentally accessed after cleanup.
- **Fixed Save/Load Input Validation**: Fixed missing validation of sscanf() return values in spell_prep.c save/load functions. Added checks in `load_spell_prep_queue()`, `load_innate_magic_queue()`, `load_spell_collection()`, and `load_known_spells()` to ensure all expected values are successfully parsed from player files. This prevents use of uninitialized values if player file data is corrupted or malformed.
- **Added Missing CREATE() NULL Checks**: Fixed missing NULL checks after CREATE() macro calls in spell_prep.c. Added defensive NULL checks in `prep_queue_add()`, `innate_magic_add()`, `collection_add()`, `known_spells_add()`, and `print_collection()` functions. This prevents crashes if memory allocation fails, logging errors and gracefully handling the failure conditions instead.
- **Fixed Buffer Overflow in print_collection()**: Fixed potential buffer overflow in `print_collection()` function in spell_prep.c where multiple strcat() calls to a fixed 256-byte buffer could exceed capacity when all metamagic flags and domain names are present. Replaced unsafe strcat() operations with safe snprintf() calls using a macro that tracks remaining buffer space. Also fixed an unsafe sprintf() used for count display. This prevents potential crashes or memory corruption when displaying spells with many metamagic modifiers.
- **Optimized star_circlet_proc() Performance**: Added loop bounds and early exit conditions to `star_circlet_proc()` function in spell_prep.c to prevent excessive iterations. Reduced LOOP_MAX from 1000 to 100, added early exit after 10 empty iterations, pre-check for classes with spells, removed unnecessary class loop, added performance monitoring in debug mode, and warning logs for hitting iteration limits. This prevents the function from consuming excessive CPU time when processing spell restoration.

### Code Quality
- **Replaced Magic Numbers with Constants**: Improved code maintainability in spell_prep.c by defining constants for hardcoded values. Added METAMAGIC_STR_SIZE (256) for buffer sizes, INVALID_PREP_TIME (99) for initial prep_time values, PREP_QUEUE_SENTINEL ("-1 -1 -1 -1 -1") for save file section terminators, and KNOWN_SPELLS_SENTINEL ("-1 -1") for known spells section terminator. Replaced all instances of these magic numbers throughout the file with the named constants, making the code more readable and maintainable.
- **Refactored compute_spells_circle() Function**: Improved code maintainability by breaking down the 400+ line `compute_spells_circle()` function in spell_prep.c into smaller, reusable components. Extracted 6 helper functions: `validate_spell_for_class()` for spell number validation by class type, `calculate_metamagic_modifier()` for metamagic circle adjustments with overflow protection, `apply_automatic_metamagic_reduction()` for automatic metamagic feat processing, `level_to_circle_conversion()` for converting character levels to spell circles based on caster type, and `check_campaign_spell_override()` for campaign-specific spell circles. This refactoring eliminated massive code duplication where each class had nearly identical switch statements, reducing the function from over 500 lines to a more manageable size with clear, single-purpose helper functions.
- **Refactored event_preparation() Function**: Improved code maintainability by breaking down the 220+ line `event_preparation()` function in spell_prep.c into smaller, focused components. Extracted 5 helper functions: `validate_event_character()` to safely validate character pointers and prevent use-after-free bugs, `validate_preparation_queue()` to check if characters have spells/slots to prepare, `process_spontaneous_caster_prep()` to handle spell slot recovery for spontaneous casters, `process_prepared_caster_prep()` to handle spell memorization for prepared casters, and `check_preparation_complete()` to check completion status and perform cleanup. This refactoring significantly improved code readability, reduced the function to ~60 lines, and made the event handling logic much clearer and easier to maintain.
- **Refactored print_collection() Function**: Improved code maintainability by breaking down the complex nested logic in `print_collection()` function in spell_prep.c. Extracted 7 helper functions: `find_or_create_spell_count()` to handle spell counting with proper memory management, `build_metamagic_string()` to safely build metamagic and domain descriptors, `append_count_to_string()` to add count display for stacked spells, `get_ordinal_suffix()` to format circle numbers (1st, 2nd, 3rd, etc.), `count_collection_spells()` to count all spells in the collection, `display_circle_spells()` to display spells for each circle, and `cleanup_spell_counts()` to properly free allocated memory. This refactoring eliminated deeply nested loops and complex conditional logic, reducing the main function from ~180 lines to ~30 lines with a much clearer control flow.
- **Standardized Error Messages**: Improved error reporting consistency in spell_prep.c by ensuring all error messages use the SYSERR: prefix. Fixed 3 log messages that were missing the prefix: two in `assign_feat_spell_slots()` function that previously used "Error in assign_feat_spell_slots()" now use "SYSERR: assign_feat_spell_slots() - invalid class in X switch statement", and one in `star_circlet_proc()` that previously used "WARNING:" now uses "SYSERR:". All 59 log() calls in the file now consistently use the SYSERR: prefix for proper syslog categorization and error tracking.

### Security
- **Added Input Validation for Spell Commands**: Enhanced security in spell_prep.c by adding comprehensive input validation to `do_gen_preparation()` and `do_consign_to_oblivion()` commands. Validates class is within bounds (0 to NUM_CLASSES), spell number is valid (1 to MAX_SPELLS), spell is actually a spell (not a skill) using IS_SPELL(), spell is not disabled, metamagic flags contain only valid bits, requested metamagic is compatible with the spell, and computed circle is within bounds (1 to TOP_CIRCLE). All validation failures are logged with SYSERR prefix. This prevents potential crashes, exploits, and undefined behavior from malformed input.
- **Added Integer Overflow Protection**: Added comprehensive integer overflow protection throughout spell_prep.c to prevent potential security vulnerabilities. Protected arithmetic operations in `compute_spells_prep_time()` including circle calculations, level bonuses, stat bonuses, concentration bonuses, feat bonuses, room bonuses, and CONFIG percentage multiplications. Also protected metamagic modifier additions in `compute_spells_circle()` with bounds checking before each addition. Added INT_MAX include and appropriate overflow checks that log SYSERR messages and use safe fallback values when overflow is detected. This prevents potential crashes or exploits from arithmetic overflow attacks.
- **Added Queue Size Limits**: Implemented denial-of-service protection in spell_prep.c by adding queue size limits. Defined constants MAX_PREP_QUEUE_SIZE (125), MAX_COLLECTION_SIZE (250), MAX_INNATE_QUEUE_SIZE (125), and MAX_KNOWN_SPELLS (250) in spell_prep.h. Originally set to 50/100/50/100, but increased by 250% to better accommodate high-level gameplay where casters may need extensive spell repertoires. Added runtime size checking to `prep_queue_add()`, `innate_magic_add()`, `collection_add()`, and `known_spells_add()` functions that count current queue sizes and reject additions when limits are exceeded. Also protected save file loading functions (`load_spell_prep_queue()`, `load_innate_magic_queue()`, `load_spell_collection()`, `load_known_spells()`) to prevent malicious save files from creating oversized queues. All limit violations are logged with SYSERR prefix including character name and class. This prevents memory exhaustion attacks through excessive spell queue manipulation.

### Documentation
- **Enhanced MySQL Configuration Documentation**: Improved mysql_config setup documentation and error handling
  - Updated mysql_config_example with prominent notice about lib/ directory requirement
  - Added clear setup instructions with example commands (cp, chmod)
  - Enhanced connect_to_mysql() function documentation in mysql.c
  - Added detailed error messages when mysql_config file is missing
  - Improved inline comments explaining configuration parameters
  - Added .gitignore entry for mysql_config to protect credentials
- **Added Thread Safety Documentation**: Added comprehensive comments to spell_prep.c and spell_prep.h clarifying that LuminariMUD is single-threaded and the spell preparation system has no concurrency issues. Added comments at `event_preparation()`, event creation check, and in the header file overview to prevent future confusion about race conditions. This addresses audit issue #6 which was incorrectly identified as a potential race condition.
- **Enhanced compute_spell_res() Documentation**: Added comprehensive in-code documentation to the `compute_spell_res()` function in magic.c to improve code readability for new developers. Added detailed function header explaining spell resistance mechanics in D&D/Pathfinder, clarified the dual purpose of the 'ch' parameter (combat caster vs informational viewer), organized spell resistance sources into clear sections (feat-based, evolution-based, creature type, spell effects), documented each SR calculation with formulas and game mechanics context, added notes about conditional SR (Holy Aura vs evil casters), and explained the MAX() vs addition logic for different SR sources. This makes the complex spell resistance system much more understandable for developers unfamiliar with the codebase or D&D mechanics.

## 2025-07-26

### Bug Fixes
- **Fixed Post Death Bash**: Added safety checks to `perform_knockdown()` in act.offensive.c to prevent dead characters from performing bash attacks. This fixes a race condition where mobs could bash players after the mob's death but before extraction from the game. The fix checks both position (POS_DEAD) and extraction flags (DEAD() macro) for both attacker and target.
- **Fixed Clairvoyance Fall Damage**: Modified `spell_clairvoyance()` in spells.c to use `look_at_room_number()` instead of physically moving the character to the target room. This prevents the caster from taking fall damage when casting clairvoyance on targets in falling rooms (like the eternal staircase).
- **Fixed Password Echo Loop**: Fixed a race condition in interpreter.c where sending password in the same packet as username could cause echo negotiation loops. Modified all password input states (CON_PASSWORD, CON_NEWPASSWD, CON_CHPWD_GETOLD, CON_DELCNF1, CON_ACCOUNT_ADD_PWD, CON_CHPWD_GETNEW) to clear pending input queue before entering password mode, preventing premature password processing.
- **Fixed Dead NPCs Taking Damage**: Fixed issue where dead NPCs continued receiving damage from DG Script area effects. Added death check (GET_POS() <= POS_DEAD || DEAD()) to script_damage() in dg_misc.c to prevent damage to already dead characters.

### Code Cleanup
- **Commented out zone loading debug messages**: Commented out "failed percentage check" log messages in db.c for 'P', 'G', and 'E' zone commands to reduce log spam during zone resets.
