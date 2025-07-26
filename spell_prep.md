# LuminariMUD Spell Preparation System Knowledge Base

## Table of Contents
1. [System Architecture Overview](#1-system-architecture-overview)
2. [Core Components Catalog](#2-core-components-catalog)
3. [Data Structures & State Management](#3-data-structures--state-management)
4. [Key Workflows](#4-key-workflows)
5. [API Reference](#5-api-reference)
6. [Configuration & Constants](#6-configuration--constants)
7. [Integration Points](#7-integration-points)
8. [Business Rules & Game Mechanics](#8-business-rules--game-mechanics)
9. [Code Conventions & Patterns](#9-code-conventions--patterns)
10. [Modification Hotspots](#10-modification-hotspots)
11. [Known Issues & Technical Debt](#11-known-issues--technical-debt)
12. [Quick Start Guide](#12-quick-start-guide)

---

## 1. System Architecture Overview

The LuminariMUD spell preparation system handles how spellcasters prepare and manage their spells. It supports two fundamentally different casting paradigms:

### **Preparation-Based Casters** (Traditional D&D/Pathfinder Model)
- **Classes**: Wizard, Cleric, Druid, Ranger, Paladin, Blackguard, Alchemist
- **Process**: Choose specific spells ’ Queue for preparation ’ Wait for completion ’ Cast once ’ Repeat
- **Storage**: Preparation Queue ’ Spell Collection (when ready)

### **Spontaneous/Innate Casters** (Flexible Casting Model)
- **Classes**: Sorcerer, Bard, Inquisitor, Summoner
- **Process**: Know spells permanently ’ Have spell slots by circle ’ Use any slot for any known spell
- **Storage**: Known Spells List + Innate Magic Queue (available slots)

### **Data Flow Architecture**
```
Player Input (memorize/pray/etc.)
        “
Validation (level, slots, prerequisites)
        “
Add to Preparation Queue
        “
Event System (ePREPARATION - fires every second)
        “
Decrement prep_time
        “
When prep_time = 0:
  - Prepared Casters: Move to Collection
  - Spontaneous: Slot becomes available
        “
Casting System checks Collection/Available Slots
        “
On successful cast:
  - Prepared: Move back to Prep Queue
  - Spontaneous: Consume slot, add to recovery
```

### **Key Design Decisions**
- **Real-time preparation**: Uses MUD event system, updates every second
- **Persistent storage**: All spell data saved to player files
- **Class isolation**: Each class has separate queues/collections
- **Metamagic integration**: Metamagic treated as part of spell identity
- **Interrupt handling**: Position/combat changes stop preparation

---

## 2. Core Components Catalog

### **spell_prep.c** (Main Implementation - ~4500 lines)
- **Purpose**: Complete spell preparation system implementation
- **Key responsibilities**:
  - Queue management (add/remove/clear)
  - Preparation timing calculations
  - Save/load functionality
  - Display functions
  - Command handlers
  - Event integration
- **Dependencies**: 
  - Depends on: mud_event.c, class.c, spells.c, domains_schools.c
  - Depended on by: spell_parser.c, limits.c, db.c, players.c
- **Critical functions**:
  - `spell_prep_gen_extract()` - Consumes spell when cast
  - `spell_prep_gen_check()` - Validates if spell can be cast
  - `do_gen_preparation()` - Main command handler
  - `event_preparation()` - Per-second update handler
  - `compute_spells_circle()` - Complex circle calculation

### **spell_prep.h** (Interface Definition)
- **Purpose**: Public API and macro definitions
- **Key responsibilities**:
  - Function declarations
  - Macro definitions (SPELL_PREP_QUEUE, etc.)
  - Constants (preparation times, circles)
  - Documentation
- **Critical macros**:
  - `SPELL_PREP_QUEUE(ch, class)` - Access prep queue
  - `SPELL_COLLECTION(ch, class)` - Access ready spells
  - `INNATE_MAGIC(ch, class)` - Access spell slots
  - `KNOWN_SPELLS(ch, class)` - Access known spells

### **mud_event.c** (Event Handler)
- **Purpose**: Manages timed events including spell preparation
- **Key function**:
  - `event_preparation()` - Called every second during preparation
- **Integration**: ePREPARATION event type

### **spell_parser.c** (Casting Interface)
- **Purpose**: Handles spell casting commands
- **Integration points**:
  - Calls `spell_prep_gen_check()` before casting
  - Calls `spell_prep_gen_extract()` after successful cast

### **players.c** (Save/Load Integration)
- **Purpose**: Player file I/O
- **Integration**: Calls spell_prep save/load functions

---

## 3. Data Structures & State Management

### **Core Data Structures** (from structs.h)

```c
/* Prepared spell or spell in preparation */
struct prep_collection_spell_data {
    int spell;      /* Spell number (SPELL_* constant) */
    int metamagic;  /* Bitvector of metamagic flags */
    int prep_time;  /* Seconds remaining (0 = ready) */
    int domain;     /* Domain spell? (clerics) */
    struct prep_collection_spell_data *next;
};

/* Spell slot for spontaneous casters */
struct innate_magic_data {
    int circle;     /* Spell circle (1-9) */
    int metamagic;  /* Pre-applied metamagic */
    int prep_time;  /* Recovery time remaining */
    int domain;     /* Usually 0 for spontaneous */
    struct innate_magic_data *next;
};

/* Known spell for spontaneous casters */
struct known_spell_data {
    int spell;      /* Spell number */
    int metamagic;  /* Usually 0 */
    int prep_time;  /* Unused */
    int domain;     /* Unused */
    struct known_spell_data *next;
};
```

### **Storage Locations** (in player_special_data_saved)
```c
/* Per-class linked lists */
struct prep_collection_spell_data *preparation_queue[NUM_CLASSES];
struct prep_collection_spell_data *spell_collection[NUM_CLASSES];
struct innate_magic_data *innate_magic_queue[NUM_CLASSES];
struct known_spell_data *known_spells[NUM_CLASSES];
```

### **State Tracking**
- **Preparation state**: `PREPARING_STATE(ch, class)` - Boolean per class
- **Event tracking**: MUD event system handles timing
- **Metamagic state**: Stored as bitvector with spell
- **Domain tracking**: Special field for divine casters

### **Important Constants**
- `NUM_CLASSES`: Total classes in game
- `NUM_CIRCLES`: 10 (spell circles 0-9, 0 unused)
- `MAX_SPELLS`: Total spell count
- `TOP_CIRCLE`: 9 (highest spell circle)

---

## 4. Key Workflows

### **Basic Spell Preparation Sequence** (Wizard Example)
1. **Player types**: `memorize 'magic missile'`
2. **Command handler** (`do_gen_preparation`):
   - Validates class can cast spell
   - Checks available spell slots
   - Calculates preparation time
   - Adds to preparation queue
3. **Player types**: `memorize` (no args to start preparing)
4. **Preparation begins**:
   - Validates position (must be resting)
   - No combat allowed
   - Creates ePREPARATION event
5. **Every second** (`event_preparation`):
   - Decrements prep_time on first spell in queue
   - When prep_time = 0:
     - Removes from prep queue
     - Adds to collection
     - Starts next spell if any
6. **Spell ready**: Shows in collection, can be cast

### **Spell Component Checking**
- Currently minimal implementation
- Wizards must have spellbook (equipment check)
- No material component consumption yet

### **Preparation Interruption**
1. **Triggers**: Standing up, entering combat, being moved
2. **Effect**: 
   - Event cancelled
   - Preparation state cleared
   - Prep time NOT reset (resumes where left off)
3. **Resume**: Type preparation command again

### **Casting Workflow**
1. **Player casts**: `cast 'magic missile'`
2. **Validation** (`spell_prep_gen_check`):
   - Prepared casters: Check collection
   - Spontaneous: Check known + available slots
3. **Success** (`spell_prep_gen_extract`):
   - Prepared: Remove from collection, add to prep queue
   - Spontaneous: Add slot to recovery queue
4. **Failure**: "You don't have that spell prepared!"

### **Spontaneous Caster Workflow**
1. **Learn spell**: Via level up, trainer, or feat
2. **Add to known spells**: `known_spells_add()`
3. **Recover slots**: `meditate` command
4. **Slots regenerate**: Over time via event system
5. **Cast**: Any known spell using available slots

---

## 5. API Reference

### **Primary Interface Functions**

```c
Function: spell_prep_gen_check()
Parameters: ch (character), spellnum (spell to check), metamagic (flags)
Returns: Class number if available, CLASS_UNDEFINED if not
Side effects: None (read-only check)
Common usage: if (spell_prep_gen_check(ch, SPELL_FIREBALL, 0) != CLASS_UNDEFINED)
```

```c
Function: spell_prep_gen_extract()
Parameters: ch (character), spellnum (spell cast), metamagic (flags used)
Returns: Class number that had spell, CLASS_UNDEFINED if failed
Side effects: Moves spell from ready to recovering state
Common usage: class = spell_prep_gen_extract(ch, spellnum, metamagic);
```

```c
Function: compute_spells_circle()
Parameters: ch, char_class, spellnum, metamagic, domain
Returns: Effective circle (1-9) or NUM_CIRCLES+1 if invalid
Side effects: None
Common usage: circle = compute_spells_circle(ch, CLASS_WIZARD, spell, META_QUICKEN, FALSE);
```

```c
Function: is_a_known_spell()
Parameters: ch, class, spellnum
Returns: TRUE if character knows spell, FALSE otherwise
Side effects: None
Common usage: if (is_a_known_spell(ch, CLASS_SORCERER, SPELL_FIREBALL))
```

```c
Function: compute_slots_by_circle()
Parameters: ch, class, circle
Returns: Number of spell slots available for that circle
Side effects: None
Common usage: slots = compute_slots_by_circle(ch, CLASS_WIZARD, 3);
```

### **Queue Management Functions**

```c
prep_queue_add(ch, class, spell, metamagic, prep_time, domain)
prep_queue_remove_by_class(ch, class, spell, metamagic)
collection_add(ch, class, spell, metamagic, 0, domain)
collection_remove_by_class(ch, class, spell, metamagic)
innate_magic_add(ch, class, circle, metamagic, prep_time, domain)
known_spells_add(ch, class, spell, loading_flag)
```

### **Display Functions**

```c
print_prep_collection_data(ch, class)  // Master display function
print_prep_queue(ch, class)            // Shows preparing spells
print_collection(ch, class)            // Shows ready spells
display_available_slots(ch, class)     // Shows remaining slots
```

---

## 6. Configuration & Constants

### **Spell Circles**
- `NUM_CIRCLES`: 10 (0-9, 0 is unused)
- `TOP_CIRCLE`: 9 (full casters)
- `TOP_BARD_CIRCLE`: 6
- `TOP_RANGER_CIRCLE`: 4
- `TOP_PALADIN_CIRCLE`: 4
- `TOP_BLACKGUARD_CIRCLE`: 4

### **Preparation Times**
- `BASE_PREP_TIME`: 6 seconds (1st circle base)
- `PREP_TIME_INTERVALS`: 2 seconds per circle above 1st
- Class multipliers:
  - `WIZ_PREP_TIME_FACTOR`: 2.0 (fastest)
  - `CLERIC_PREP_TIME_FACTOR`: 2.5
  - `DRUID_PREP_TIME_FACTOR`: 2.5
  - `RANGER_PREP_TIME_FACTOR`: 3.0 (slowest)
  - `PALADIN_PREP_TIME_FACTOR`: 3.0

### **Command Subcommands**
```c
#define SCMD_MEMORIZE    1   // Wizard
#define SCMD_PRAY        2   // Cleric  
#define SCMD_COMMUNE     3   // Druid
#define SCMD_MEDITATE    4   // Sorcerer
#define SCMD_CHANT       5   // Paladin
#define SCMD_ADJURE      6   // Ranger
#define SCMD_COMPOSE     7   // Bard
#define SCMD_CONCOCT     8   // Alchemist
#define SCMD_CONDEMN    10   // Blackguard
#define SCMD_COMPEL     11   // Inquisitor
#define SCMD_CONJURE    12   // Summoner
```

### **Slot Tables** (from constants.c)
- Located in arrays like `wizard_slots[][]`, `sorcerer_slots[][]`
- Format: `[level][circle]` = number of slots
- Bonus slots from high ability scores in `spell_bonus[][]`

### **File Paths**
- Player files: Defined by MUD configuration
- No spell-specific data files (all in code)

### **Magic Numbers**
- Prep queue sentinel: `-1 -1 -1 -1 -1`
- Known spells sentinel: `-1 -1`
- Max metamagic circle adjustment: +4
- Room prep bonus (tavern): 25% reduction

---

## 7. Integration Points

### **Player Character System**
- **Level checks**: `get_class_highest_circle()`, `is_min_level_for_spell()`
- **Ability scores**: INT/WIS/CHA affect slots and prep time
- **Skills**: Concentration skill reduces prep time
- **Feats**: Fast Memorization, Extra Slot, bloodlines

### **Inventory/Component System**
- **Current**: Minimal - only spellbook check for wizards
- **Interface**: Equipment checks in `ready_to_prep_spells()`
- **Future**: Material component consumption planned

### **Combat System**
- **Interruption**: Combat stops preparation
- **Casting**: Calls extract/check functions
- **Metamagic**: Affects casting time and spell level

### **Magic/Mana System**
- **Separation**: Spell slots != mana points
- **Integration**: Some feats/abilities may restore slots

### **Game Loop Integration**
- **Event system**: ePREPARATION fires every second
- **Position updates**: Check for interruption
- **Save system**: Hooks in players.c

### **Message Handling**
- **Player messages**: Via `send_to_char()`
- **Room messages**: Via `act()` function
- **Color codes**: Uses MUD color system

---

## 8. Business Rules & Game Mechanics

### **Preparation Requirements**
1. **Position**: Must be resting (POS_RESTING)
2. **Combat**: Cannot prepare during combat
3. **Status effects**: No paralysis, sleep, etc.
4. **Equipment**: Wizards need spellbook
5. **Level**: Must meet minimum level for spell
6. **Slots**: Must have available slots for circle

### **Spell Circle Calculation** (Complex!)
```
Base Circle = spell_info[spellnum].min_level[class] / 2
+ Metamagic adjustments:
  - Quicken: +4 circles
  - Maximize: +3 circles  
  - Extend: +1 circle
  - Silent/Still: +1 circle each
- Feat reductions (e.g., auto-quicken)
- Domain spell: -1 circle for clerics
Final = 1-9 (capped at class maximum)
```

### **Preparation Time Formula**
```
Base = 6 + (2 * (circle - 1))
× Class factor (2.0 to 3.0)
- Ability bonus (INT/WIS/CHA modifier * 2)
- Concentration ranks
- Feat bonuses (20% for Fast Memorization)
× Room modifier (0.75 for taverns)
Minimum = 1 second
```

### **Slot Calculation**
```
Base slots = class_slots[level][circle]
+ Ability bonus slots = spell_bonus[ability_mod][circle]
+ Feat slots (Extra Slot, New Arcana)
+ Item bonuses
- Used slots (in any queue/collection)
```

### **Known Spell Limits** (Spontaneous)
- Base from class tables
- Bonus from INT (psionicist)
- Bonus from feats (Expanded Knowledge)
- Bloodline spells don't count against limit

### **Special Cases**
- **Epic spells**: Planned but not implemented
- **Domain spells**: Separate slots for clerics
- **Bloodline spells**: Free for sorcerers
- **Psionics**: Use spell system with special rules
- **NPCs**: Can cast any spell of their class

---

## 9. Code Conventions & Patterns

### **Naming Conventions**
- Functions: `snake_case` (e.g., `prep_queue_add`)
- Macros: `UPPER_CASE` (e.g., `SPELL_PREP_QUEUE`)
- Structs: `snake_case_data` suffix
- Events: `ePASCALCASE` (e.g., `ePREPARATION`)

### **Error Handling**
```c
/* Standard pattern */
if (!ch || !ch->player_specials) {
    log("SYSERR: spell_prep function called with invalid character");
    return;
}

/* User feedback */
if (slots_available <= 0) {
    send_to_char(ch, "You have no available spell slots of that circle.\r\n");
    return;
}
```

### **Memory Management**
```c
/* Allocation pattern */
CREATE(new_spell, struct prep_collection_spell_data, 1);
new_spell->spell = spellnum;
new_spell->next = SPELL_PREP_QUEUE(ch, class);
SPELL_PREP_QUEUE(ch, class) = new_spell;

/* Deallocation pattern */
tmp = current->next;
free(current);
current = tmp;
```

### **Linked List Patterns**
- Always add to head (O(1) insertion)
- Walk with next pointer caching
- NULL terminate all lists

### **Common Macros Used**
- `CREATE()`: Allocate memory
- `GET_LEVEL()`: Character level
- `GET_CLASS()`: Character class
- `IS_NPC()`: NPC check
- `MIN()`, `MAX()`: Bounds checking

### **Debug Mode**
- `#define DEBUGMODE FALSE`
- When TRUE, sends verbose messages
- Production should be FALSE

---

## 10. Modification Hotspots

### **To Add a New Spell**
1. **Define spell**: In spells.h and spell_parser.c
2. **Set levels**: In `spell_info[].min_level[]`
3. **Add to lists**: Class spell lists in class.c
4. **Help entry**: In help files

### **To Add a New Caster Class**
1. **Constants**: Add CLASS_* constant
2. **Tables**: Add to slot tables in constants.c
3. **Commands**: Add SCMD_* and command in interpreter.c
4. **Switch statements**: Update all class switches in spell_prep.c
5. **Prep factor**: Add *_PREP_TIME_FACTOR

### **To Modify Preparation Mechanics**
1. **Timing**: Adjust in `compute_spells_prep_time()`
2. **Requirements**: Modify `ready_to_prep_spells()`
3. **Interruption**: Check movement/combat hooks
4. **Slots**: Modify `compute_slots_by_circle()`

### **To Change Balance/Timing**
1. **Base times**: BASE_PREP_TIME, PREP_TIME_INTERVALS
2. **Class factors**: *_PREP_TIME_FACTOR defines
3. **Ability impact**: In prep time calculation
4. **Feat bonuses**: In respective functions

### **Extension Points**
- **Components**: Hook in `ready_to_prep_spells()`
- **Special prep**: Add checks in validation
- **New metamagic**: Update circle calculation
- **Prep bonuses**: Add to time calculation

---

## 11. Known Issues & Technical Debt

### **TODO/FIXME Comments**
1. **Epic spells**: `isEpicSpell()` always returns FALSE
2. **Feat-based slots**: `assign_feat_spell_slots()` incomplete
3. **Material components**: No consumption system
4. **Domain spell entry**: Noted as needing fixes
5. **Search mode**: Some functions have unused parameters

### **Technical Debt**
1. **Dual systems**: Some innate magic code duplicates prep code
2. **Magic numbers**: Circle/level conversions scattered
3. **State redundancy**: Preparing state + event existence
4. **NPC handling**: Lots of special cases

### **Performance Considerations**
1. **Linked lists**: O(n) searches, could use hash tables
2. **Event spam**: One event per preparing character
3. **Save/load**: Inefficient text format

### **Potential Bugs**
1. **Metamagic + domains**: Complex interaction
2. **Multiclass**: Each class has separate pools
3. **Level loss**: No cleanup of high-level spells
4. **Queue corruption**: If save/load interrupted

---

## 12. Quick Start Guide

### **"To add a new spell type, modify..."**
1. Define in spells.h: `#define SPELL_NEW_SPELL 299`
2. Set info in spell_parser.c: `spello()` call
3. Add to class lists in class.c spell assignment
4. Create help entry

### **"To change preparation time calculation, look at..."**
- Function: `compute_spells_prep_time()` (~line 2670)
- Factors: Class multipliers at top of spell_prep.h
- Base values: BASE_PREP_TIME, PREP_TIME_INTERVALS

### **"To debug preparation failures, check..."**
1. Enable DEBUGMODE in spell_prep.c
2. Check `ready_to_prep_spells()` conditions
3. Verify slots with `compute_slots_by_circle()`
4. Check event with `char_has_mud_event(ch, ePREPARATION)`

### **"To modify component requirements, update..."**
- Currently: Only spellbook check in `ready_to_prep_spells()`
- Add checks: In validation section of `do_gen_preparation()`
- Consumption: After `spell_prep_gen_extract()` in casting

### **"To add a new metamagic feat..."**
1. Define flag in structs.h metamagic section
2. Update `compute_spells_circle()` for adjustment
3. Add parsing in command handlers
4. Update display functions for name

### **"To fix slot calculation issues..."**
- Base slots: Tables in constants.c
- Calculation: `compute_slots_by_circle()`
- Counting: `count_total_slots()` family
- Display: `display_available_slots()`

### **Common Command Testing**
```
# Wizard preparation
memorize 'magic missile'      # Add to queue
memorize quicken 'fireball'   # With metamagic  
memorize                      # Start preparing
forget 'magic missile'        # Remove from queue
forget all                    # Clear everything

# Spontaneous caster
meditate                      # Show slots and start recovery
compose                       # Bard version

# Debugging
stat file <player>            # Check saved spells
set <player> prep             # Force preparation state
```

### **Integration Checklist**
When modifying the spell system:
- [ ] Update all class switch statements
- [ ] Check save/load compatibility
- [ ] Test preparation interruption
- [ ] Verify metamagic interactions
- [ ] Test with multiclass characters
- [ ] Check NPC casting still works
- [ ] Verify help files updated
- [ ] Test queue display formatting

---

## Additional Notes

The spell preparation system is one of the most complex subsystems in LuminariMUD, touching character progression, combat, game balance, and player interaction. Its design reflects D&D 3.5/Pathfinder mechanics while adapting to real-time MUD gameplay.

The separation between prepared and spontaneous casters is fundamental and affects nearly every function. When making changes, always consider both paradigms and test with representatives of each type.

The system's real-time nature (1-second updates) means performance is important. Avoid expensive operations in the event handler or validation functions that run frequently.