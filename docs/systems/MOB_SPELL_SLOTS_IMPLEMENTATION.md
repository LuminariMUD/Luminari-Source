# Mob Spell Slot System - Implementation Summary

## Date: October 19, 2025

## Overview
Successfully implemented a spell slot system for NPCs that tracks and limits their spellcasting, replacing the previous unlimited casting system. Mobs now regenerate ONE random spell slot per minute while out of combat.

## Files Created

### 1. src/mob_spellslots.c
- **get_spell_circle()**: Calculates which spell circle (0-9) a spell belongs to based on class
- **init_mob_spell_slots()**: Initializes spell slots when mob is loaded/reset
- **has_spell_slot()**: Checks if mob has available slot for a given spell
- **consume_spell_slot()**: Deducts a spell slot when mob casts successfully
- **regenerate_mob_spell_slot()**: Restores one random depleted spell slot every 60 seconds (out of combat only)
- **show_mob_spell_slots()**: Admin command to display current/max slots per circle

### 2. src/mob_spellslots.h
- Header file with function prototypes for the spell slot system

### 3. docs/systems/MOB_SPELL_SLOTS_DESIGN.md
- Comprehensive design document detailing the system architecture
- Implementation checklist
- Future enhancement ideas

## Files Modified

### 1. src/structs.h
**Added to mob_special_data structure (lines ~5493-5496):**
```c
int spell_slots[10];        /* Current spell slots per circle (0-9) */
int max_spell_slots[10];    /* Maximum spell slots per circle (0-9) */
time_t last_slot_regen;     /* Timestamp of last spell slot regeneration */
```

**Added MOB flag (line 1103):**
```c
#define MOB_UNLIMITED_SPELL_SLOTS 100 /**< Mob has unlimited spell slots (bypasses slot system) */
```

**Note**: Spell slots are now **enabled by default**. The flag is used to **opt-out** and give unlimited casting.

**Updated NUM_MOB_FLAGS:**
```c
#define NUM_MOB_FLAGS 101
```

### 2. src/constants.c
**Added to action_bits array (line ~1567):**
```c
"Unlimited-Spell-Slots",
```

### 3. src/spell_parser.c
**Added include:**
```c
#include "mob_spellslots.h"
```

**Modified npc_can_cast() (line ~6272):**
- Added spell slot availability check before allowing NPC to cast
- Returns false if mob has no slots available (unless MOB_UNLIMITED_SPELL_SLOTS is set)

**Modified call_magic() (line ~797):**
- Added consume_spell_slot() call after spell validation but before execution
- Only consumes slots for NPCs using CAST_SPELL casttype

### 4. src/db.c
**Added include:**
```c
#include "mob_spellslots.h"
```

**Modified read_mobile() (line ~4348):**
- Added init_mob_spell_slots() call before returning new mob
- Initializes all slot arrays based on mob's class and level

### 5. src/mob_act.c
**Added include:**
```c
#include "mob_spellslots.h"
```

**Modified mobile_activity() (line ~124):**
- Added regenerate_mob_spell_slot() call in main mob activity loop
- Runs every tick for every awake mob (function internally checks timing)

### 6. Makefile.am
**Added source file to build:**
```plaintext
src/mob_spellslots.c \
```

## How It Works

### 1. Mob Loading
When a mob is loaded via `read_mobile()`:
1. Check if mob has MOB_UNLIMITED_SPELL_SLOTS flag
2. If NO (default), calculate max slots per circle based on class/level using `compute_slots_by_circle()`
3. Initialize current slots to maximum (fully rested)
4. Set regeneration timestamp to current time
5. Circle 0 (cantrips) gets 100 slots (effectively unlimited)

### 2. Spell Casting
When a mob attempts to cast:
1. `npc_can_cast()` checks if slot available via `has_spell_slot()`
2. Returns false if no slot available, preventing cast
3. If cast proceeds to `call_magic()`, `consume_spell_slot()` is called
4. Spell slot for that circle is decremented

### 3. Slot Regeneration
Every game tick in `mobile_activity()`:
1. `regenerate_mob_spell_slot()` is called for each mob
2. Function checks:
   - Does mob have MOB_UNLIMITED_SPELL_SLOTS flag? (skip if yes)
   - Is mob NOT in combat?
   - Has 120 seconds (2 minutes) elapsed since last regeneration?
3. If all checks pass:
   - Build list of circles with less than max slots
   - Randomly pick one circle
   - Restore one slot to that circle
   - Update regeneration timestamp

## Default Enabled System
- **All mobs use spell slots by default**
- Mobs WITH MOB_UNLIMITED_SPELL_SLOTS flag have unlimited casting (opt-out)
- This encourages tactical gameplay and resource management across the entire game world
- Special/boss mobs can be flagged for unlimited slots if desired

## Spell Circle Calculation
Based on class type and spell's min_level:
- **Full casters** (Wizard, Cleric, Druid, Inquisitor): `(min_level + 1) / 2`
- **Spontaneous casters** (Sorcerer, Bard): `min_level / 2`
- **Half casters** (Paladin, Ranger, Blackguard): `(min_level + 1) / 2`

## Slot Allocation
Uses existing `compute_slots_by_circle()` function from spell_prep.c:
- Same slot progression as player casters
- Varies by class (full/half/spontaneous)
- Level-based scaling
- Circle 0 = 100 slots (unlimited cantrips)

## Compilation Status
✅ **Successfully compiled** with no errors or warnings

## Testing Needed
1. Set MOB_USES_SPELLSLOTS flag on test mob
2. Verify mob can cast until slots depleted
3. Verify mob cannot cast when slots exhausted
4. Verify regeneration occurs every 60 seconds out of combat
5. Verify regeneration does NOT occur during combat
6. Test with various caster classes and levels
7. Test admin display command (to be implemented)

## Future Work
### Phase 2 - Admin Commands (TODO #7)
- Add spell slot display to `mstat` command
- Show current/max slots per circle
- Show time since last regeneration

### Phase 3 - Enhancements
- Variable regeneration rates based on mob type
- Special mob types with faster/slower regeneration
- Spell slot trading (higher slots for multiple lower)
- Mob AI improvements to manage spell slot resources
- Retreat behavior when slots run low

## Known Limitations
- Cantrips (circle 0) are effectively unlimited (100 slots)
- All mobs regenerate at same rate (60 seconds)
- No differentiation between spell schools or types
- Mob AI doesn't actively manage slot depletion

## Configuration Options
To enable spell slots on a mob:
1. Use `medit <vnum>` to edit mob
2. Toggle "Uses-Spell-Slots" flag in mob flags
3. Slots will auto-calculate on mob load based on class/level

## Performance Impact
- Minimal: O(1) slot checks on cast
- O(10) regeneration check (10 circles to scan)
- No significant performance degradation expected
- Regeneration only runs for awake mobs

## Backward Compatibility
⚠️ **Breaking change - spell slots now default**
- All existing mobs will now use spell slots
- To restore unlimited casting: add MOB_UNLIMITED_SPELL_SLOTS flag
- No changes to player spellcasting
- May require zone reviews to flag special mobs for unlimited slots
- Regeneration time increased to 2 minutes (was 1 minute)

---

**Status**: ✅ Core system implemented and compiled successfully
**Next Step**: Add admin commands for debugging/testing (#7 in TODO)
**Final Step**: Test in live environment (#8 in TODO)
