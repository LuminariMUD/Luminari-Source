# Room Flags Documentation

This document provides comprehensive information about all room flags (ROOM_*) used in LuminariMUD. Room flags control various mechanics, restrictions, and behaviors of rooms throughout the game world.

## Table of Contents
- [Overview](#overview)
- [Movement & Access Restrictions](#movement--access-restrictions)
- [Magic Restrictions](#magic-restrictions)
- [Combat & PvP Restrictions](#combat--pvp-restrictions)
- [Environmental Effects](#environmental-effects)
- [Vision & Perception](#vision--perception)
- [Special Room Types](#special-room-types)
- [System & Administrative Flags](#system--administrative-flags)
- [Size Restrictions](#size-restrictions)
- [Zone Features](#zone-features)
- [Complete Flag Reference](#complete-flag-reference)

---

## Overview

Room flags are bitflags defined in `src/structs.h` (lines 225-271) and are checked throughout the codebase using the `ROOM_FLAGGED()` macro. There are currently 42 room flags (indices 0-41) that control everything from movement restrictions to magical effects.

**Usage Pattern:**
```c
if (ROOM_FLAGGED(room_rnum, ROOM_FLAGNAME)) {
    // Flag is set, apply restrictions/effects
}
```

---

## Movement & Access Restrictions

### ROOM_TUNNEL (Index: 8)
**Effect:** Limits the number of characters that can be in the room simultaneously.
- Maximum occupants determined by `CONFIG_TUNNEL_SIZE`
- Prevents mounted characters from entering
- Blocks formations and grouped movement when full
- Used for narrow passages, tunnels, and chokepoints

**Code References:**
- `movement.c:545` - Blocks mounted entry
- `movement.c:551` - Enforces occupancy limit
- `desc_engine.c:683` - Dynamic room descriptions

### ROOM_SINGLEFILE (Index: 20)
**Effect:** Forces characters to move through the room one at a time in combat situations.
- Prevents backstab and other flanking abilities
- Limits positioning-based combat mechanics
- Used for narrow corridors and single-file passages

**Code References:**
- `movement.c:306, 914` - Movement restrictions
- `fight.c:216, 7895, 12269, 13997` - Combat positioning checks
- 20+ additional checks across combat and spell systems

### ROOM_FLY_NEEDED (Index: 18)
**Effect:** Characters must be flying to remain in the room; non-flying characters fall.
- Triggers falling mechanics for grounded characters
- Objects without support also fall
- Used for aerial rooms, cliff edges, and suspended platforms

**Code References:**
- `movement_falling.c:72` - Character falling check
- `movement_falling.c:47` - Object falling check
- `craft.c:664, 700` - Crafting system checks

### ROOM_CLIMB_NEEDED (Index: 32)
**Effect:** Requires a skill check to enter the room.
- Difficulty based on zone minimum level
- Used for cliffs, mountains, and vertical obstacles

**Code References:**
- `movement.c:570` - Entry skill check
- `craft.c:666, 702` - Crafting restrictions

### ROOM_NOFLY (Index: 26)
**Effect:** Prevents flying creatures from entering the room.
- Forces grounded movement
- Used in low-ceiling areas and confined spaces

**Code References:**
- `movement.c:501` - Entry restriction check

### ROOM_PRIVATE (Index: 9)
**Effect:** Limits room to 2 occupants maximum.
- Additional occupants cannot enter or teleport in
- Staff can override this restriction
- Used for private meeting rooms and intimate spaces

**Code References:**
- `movement.c:1249` - Entry restriction
- `act.wizard.c:268, 347` - Teleport restrictions
- `spells.c:299` - Spell targeting restrictions
- `house.c:445, 627, 664` - House management integration

### ROOM_STAFFROOM (Index: 10)
**Effect:** Only staff (LVL_STAFF+) can enter this room.
- Hard block on entry for non-staff
- Used for immortal areas and staff lounges

**Code References:**
- `movement.c:563` - Level check on entry

---

## Magic Restrictions

### ROOM_NOMAGIC (Index: 7)
**Effect:** Completely prevents all magic from functioning in the room.
- Blocks spell casting by caster or victim in the room
- Exception: Weapon poison (CAST_WEAPON_POISON) still works
- Affects both offensive and beneficial magic
- Strongest magic restriction available

**Code References:**
- `spell_parser.c:540, 547, 3046` - Spell casting prevention
- `fight.c:8518, 8522, 8639` - Combat magic checks
- `magic.c:12643` - General magic check

### ROOM_NOTELEPORT (Index: 21)
**Effect:** Prevents teleportation into or out of the room.
- Blocks dimension door, teleport, and similar spells
- Blocks both incoming and outgoing teleportation
- Used for secured areas and restricted zones

**Code References:**
- `spells.c:314` - Teleport destination check
- `spells.c:2199-2398` - Multiple recall/teleport spell checks
- `act.other.c:808, 896, 11637` - Recall and portal restrictions

### ROOM_NOSUMMON (Index: 24)
**Effect:** Prevents summoning spells from targeting this room.
- Blocks summoning creatures to or from the room
- Used for protected sanctuaries and anti-summoning zones

**Code References:**
- `spells.c:2912-2913` - Summon spell checks

### ROOM_NORECALL (Index: 19)
**Effect:** Prevents recall/return spells from working.
- Blocks escape via recall magic
- Used in dungeons and combat zones where escape should be prevented

**Code References:**
- `spells.c:2200-2399` - Multiple recall spell checks (8 matches)

---

## Combat & PvP Restrictions

### ROOM_PEACEFUL (Index: 4)
**Effect:** Prevents all forms of combat and aggressive actions.
- Blocks physical attacks (hit, backstab, etc.)
- Blocks offensive spells and abilities
- Prevents grappling and combat maneuvers
- Blocks domain powers and offensive class abilities
- Used in safe zones, towns, and newbie areas

**Code References:**
- `fight.c:1509, 5370, 12251, 12259` - Combat initiation blocks
- `spells.c:698, 756, 881, 916, 1319, 1810, 2569, 2598` - Offensive spell blocks
- `grapple.c:218` - Grappling prevention
- `domain_powers.c:76, 818, 903, 988, 1073, 1156` - Domain power checks
- `spell_parser.c:554, 3062` - Spell casting restrictions

### ROOM_DEATH (Index: 1)
**Effect:** Kills characters or deals severe damage when entering/remaining in room.
- Used for death traps and hazardous zones
- NPCs avoid entering death rooms
- Teleportation into death rooms is blocked
- Staff can bypass with appropriate level

**Code References:**
- `act.informative.c:1263, 1475` - Display warnings
- `movement.c:1256` - Entry damage/death
- `mob_act.c:400` - NPC avoidance
- `spells.c:302` - Teleport blocking
- `act.other.c:11637` - Additional teleport checks

### ROOM_SOUNDPROOF (Index: 5)
**Effect:** Blocks sound-based abilities and prevents communication.
- Blocks tells, shouts, and speech-based communication
- Prevents bardic performances from functioning
- Blocks sound-based spells (exception for psionics)
- Staff can override (LVL_STAFF+)

**Code References:**
- `act.comm.c:465, 473, 810, 984` - Communication blocking
- `bardic_performance.c:264` - Bardic performance prevention
- `spell_parser.c:1770` - Spell casting restriction (psionics exempt)
- `act.social.c:275` - Social action blocking

---

## Environmental Effects

### ROOM_INDOORS (Index: 3)
**Effect:** Marks room as indoors for various game mechanics.
- Affects weather exposure
- Influences light calculations (indoor rooms don't go dark at night)
- Affects outdoor-only abilities and effects
- Used for buildings, caves, and sheltered areas

**Code References:**
- `utils.c:823, 911` - Weather and outdoor checks
- `movement.c:1372` - Outdoor transition detection
- `vessels_rooms.c:40-96, 190-191, 606` - Vehicle room templates

### ROOM_REGEN (Index: 17)
**Effect:** Doubles natural regeneration rates for HP, mana, and movement.
- Minimum regen of 2 before doubling (2 becomes 4, etc.)
- Stacks with other regen bonuses
- Used for healing sanctuaries and rest areas

**Code References:**
- `limits.c:595` - HP regeneration doubling
- `spell_prep.c:3778` - Spell preparation bonus

### ROOM_NOHEAL (Index: 25)
**Effect:** Prevents natural healing and regeneration.
- Blocks HP regeneration tick
- Similar to Blackmantle spell effect
- Used for cursed areas and unholy ground

**Code References:**
- `limits.c:622` - Regeneration blocking

---

## Vision & Perception

### ROOM_DARK (Index: 0)
**Effect:** Forces the room to be dark regardless of other conditions.
- Overrides normal light calculations
- Can be temporarily removed/added by darkness/light spells
- Requires light source or darkvision to see
- Stacks with ROOM_MAGICDARK for absolute darkness

**Code References:**
- `utils.c:3594, 3656, 3697` - Darkness calculations
- `act.informative.c:1282-1383` - Room description handling
- `magic.c:12654, 12658` - Darkness spell interaction
- `mud_event.c:169` - Temporary darkness removal

### ROOM_MAGICDARK (Index: 22)
**Effect:** Creates magical darkness that penetrates normal light sources.
- Overrides mundane light sources
- Only defeated by magical light or certain abilities
- When combined with ROOM_DARK, creates absolute darkness
- Used for shadow magic zones and deep darkness effects

**Code References:**
- `utils.c:3596, 3643, 3697` - Darkness calculations
- `act.informative.c:1205, 1211, 1296, 8463` - Display and description handling

### ROOM_MAGICLIGHT (Index: 23)
**Effect:** Creates magical light that prevents the room from being dark.
- Overrides normal darkness conditions
- Cannot be defeated by normal darkness
- Used for magically illuminated areas

**Code References:**
- `utils.c:3626` - Light calculation override

### ROOM_FOG (Index: 27)
**Effect:** Creates fog that obscures vision and limits visibility.
- Hides room descriptions and contents from mortal characters
- Limits automap functionality
- Reduces scan range and visibility
- Staff (LVL_IMMORT+) can see through fog
- Can be removed by gust of wind spell

**Code References:**
- `act.informative.c:1199, 1259, 1447, 1550, 1572, 8458, 8617, 8977` - Vision restrictions
- `asciimap.c:760` - Automap limitation
- `spells.c:515, 519` - Gust of wind removes fog
- `utils.c:917, 3699` - Weather and vision checks

### ROOM_NOTRACK (Index: 6)
**Effect:** Prevents tracking abilities from working through this room.
- Blocks track skill usage
- Breaks pathfinding algorithms
- NPCs won't track players through these rooms
- Used for anti-tracking zones and rivers

**Code References:**
- `movement_tracks.c:256` - Track skill blocking
- `graph.c:59` - Pathfinding restriction
- `mob_act.c:383` - NPC tracking limitation

---

## Special Room Types

### ROOM_HOUSE (Index: 11)
**Effect:** Marks room as part of a player house.
- Integrates with house ownership system
- Restricts entry to house owners and guests
- Enables house crash-save functionality
- Used with ROOM_HOUSE_CRASH for persistence

**Code References:**
- `house.c:296, 444, 626, 663, 665, 714, 849, 904` - House management
- `handler.c:2212-2213, 2247-2248` - Crash save integration
- `act.wizard.c:270, 349, 1712, 5581` - Teleport restrictions and admin tools

### ROOM_HOUSE_CRASH (Index: 12)
**Effect:** Marks that items in this house room should be saved.
- Auto-set when items are modified in house rooms
- Triggers house save-to-disk operations
- Cleared after successful save
- Used for house persistence system

**Code References:**
- `house.c:296, 714` - Save operations
- `handler.c:2213, 2248` - Auto-flagging on item changes
- `hsedit.c:211` - House editor cleanup

### ROOM_ATRIUM (Index: 13)
**Effect:** Marks room as a house atrium (entry point).
- Entry point for player houses
- Special restrictions on entering houses apply
- Managed by house system automatically

**Code References:**
- `house.c:446, 628, 657, 678, 682` - House system integration
- `movement.c:520` - Entry point detection
- `hsedit.c:168, 206, 224, 230` - House editor management

### ROOM_WORLDMAP (Index: 16)
**Effect:** Marks room as part of the overworld/wilderness map system.
- Enables special worldmap automap display
- Used for wilderness travel system
- Affects routing and navigation commands

**Code References:**
- `act.informative.c:8554` - Automap checks
- `asciimap.c:729` - Map rendering
- `routing.c:290` - Navigation system
- `act.wizard.c:10211` - Administrative flag setting

### ROOM_VEHICLE (Index: 40)
**Effect:** Marks room as part of a vehicle or vessel.
- Allows vehicle movement through this room
- Used for ship interiors, wagons, etc.
- Required for directional vehicle navigation
- Integrates with vessel system

**Code References:**
- `vessels_rooms.c:40-103, 188-189` - Vehicle room templates
- `vessels_src.c:282, 301, 368, 384, 505-506` - Vehicle movement system
- `vessels_docking.c:562` - Docking restrictions

### ROOM_HARVEST_NODE (Index: 38)
**Effect:** Marks room as a resource harvesting location.
- Enables gathering/harvesting of materials
- Integrates with crafting system
- Used for mining nodes, herb gardens, etc.

**Code References:**
- `crafting_new.c:531` - Harvest node detection

### ROOM_PLAYER_SHOP (Index: 35)
**Effect:** Marks room as a player-run shop.
- Enables player merchant functionality
- Integrates with player economy system

**Note:** Limited code references suggest this may be partially implemented or planned feature.

### ROOM_ROAD (Index: 39)
**Effect:** Marks room as a road or path.
- May affect travel speed or mount usage
- Used for road networks and travel routes

**Note:** No direct code references found; likely used by zone builders for thematic purposes or planned features.

---

## System & Administrative Flags

### ROOM_OLC (Index: 14)
**Effect:** Marks room as being edited in the Online Level Creator (OLC).
- Used by building/editing system
- Prevents certain cleanup operations
- Temporary flag during world editing

**Code References:**
- `act.wizard.c:5581, 5587` - Administrative cleanup avoidance

### ROOM_BFS_MARK (Index: 15)
**Effect:** Temporary flag used by pathfinding algorithms.
- Marks rooms as visited during Breadth-First Search
- Automatically cleared after pathfinding completes
- Should never be permanently set on rooms
- Internal system flag

**Code References:**
- `graph.c:59` - Pathfinding algorithm
- `act.wizard.c:5581` - Cleanup detection

---

## Size Restrictions

### ROOM_SIZE_TINY (Index: 30)
**Effect:** Marks room as extremely small.
- May restrict large creatures from entering
- Used for mouse holes, small tunnels, etc.

**Note:** Limited code references suggest this is primarily a builder/thematic flag.

### ROOM_SIZE_DIMINUTIVE (Index: 31)
**Effect:** Marks room as very small.
- May restrict entry based on creature size
- Used for pixie homes, tiny passages, etc.

**Note:** Limited code references suggest this is primarily a builder/thematic flag.

### ROOM_AIRY (Index: 28)
**Effect:** Marks room as having an airy, open quality.
- May affect wind-based spells or effects
- Thematic flag for open, breezy areas

**Note:** No direct code references found; likely thematic or planned feature.

---

## Zone Features

### ROOM_RANDOM_TRAP (Index: 36)
**Effect:** Forces a random trap to always generate in this room.
- Overrides normal trap generation probability
- Guarantees trap presence on zone reset
- Used for specific trap encounters

**Code References:**
- `traps_new.c:547, 575-576` - Forced trap generation
- `db.c:5431` - Zone loading trap setup

### ROOM_RANDOM_CHEST (Index: 37)
**Effect:** Marks room as eligible for random treasure chest spawning.
- Integrates with random treasure system
- Used for treasure hunt mechanics

**Note:** Limited code references suggest this may be a planned or partially implemented feature.

### ROOM_HASTRAP (Index: 33)
**Effect:** Indicates the room contains a trap.
- May be set by trap generation system
- Used for trap detection and disarmament

**Note:** Limited code references suggest integration with trap system.

### ROOM_GENDESC (Index: 34)
**Effect:** Indicates room description should be generated dynamically.
- Used with procedural generation systems
- Allows for dynamic room descriptions

**Note:** Limited code references suggest this is a specialized builder flag.

### ROOM_OCCUPIED (Index: 29)
**Effect:** Marks room as occupied or reserved.
- Purpose unclear from code analysis
- May relate to instance or phasing systems

**Note:** No direct code references found; may be planned feature.

### ROOM_DOCKABLE (Index: 41)
**Effect:** Marks room as a valid docking location for vessels.
- Intended for ship docking mechanics
- Part of vessel system

**Note:** No code references found; likely planned feature for vessel system expansion.

---

## Complete Flag Reference

### Quick Reference Table

| Index | Flag Name | Primary Purpose | Category |
|-------|-----------|----------------|----------|
| 0 | ROOM_DARK | Forces darkness | Vision |
| 1 | ROOM_DEATH | Death trap | Hazard |
| 2 | ROOM_NOMOB | NPC restriction | Movement |
| 3 | ROOM_INDOORS | Indoor room | Environment |
| 4 | ROOM_PEACEFUL | No combat | Combat |
| 5 | ROOM_SOUNDPROOF | Blocks sound | Perception |
| 6 | ROOM_NOTRACK | Blocks tracking | Perception |
| 7 | ROOM_NOMAGIC | No magic | Magic |
| 8 | ROOM_TUNNEL | Occupancy limit | Movement |
| 9 | ROOM_PRIVATE | 2 person limit | Movement |
| 10 | ROOM_STAFFROOM | Staff only | Access |
| 11 | ROOM_HOUSE | Player house | Special |
| 12 | ROOM_HOUSE_CRASH | House save flag | System |
| 13 | ROOM_ATRIUM | House entrance | Special |
| 14 | ROOM_OLC | Being edited | System |
| 15 | ROOM_BFS_MARK | Pathfinding temp | System |
| 16 | ROOM_WORLDMAP | Wilderness map | Special |
| 17 | ROOM_REGEN | Double regen | Environment |
| 18 | ROOM_FLY_NEEDED | Must fly | Movement |
| 19 | ROOM_NORECALL | No recall | Magic |
| 20 | ROOM_SINGLEFILE | One at a time | Movement |
| 21 | ROOM_NOTELEPORT | No teleport | Magic |
| 22 | ROOM_MAGICDARK | Magical darkness | Vision |
| 23 | ROOM_MAGICLIGHT | Magical light | Vision |
| 24 | ROOM_NOSUMMON | No summoning | Magic |
| 25 | ROOM_NOHEAL | No healing | Environment |
| 26 | ROOM_NOFLY | Flying blocked | Movement |
| 27 | ROOM_FOG | Fog obscurement | Vision |
| 28 | ROOM_AIRY | Airy atmosphere | Environment |
| 29 | ROOM_OCCUPIED | Occupied status | Special |
| 30 | ROOM_SIZE_TINY | Tiny size | Size |
| 31 | ROOM_SIZE_DIMINUTIVE | Diminutive size | Size |
| 32 | ROOM_CLIMB_NEEDED | Requires climbing | Movement |
| 33 | ROOM_HASTRAP | Contains trap | Zone Feature |
| 34 | ROOM_GENDESC | Generated desc | System |
| 35 | ROOM_PLAYER_SHOP | Player shop | Special |
| 36 | ROOM_RANDOM_TRAP | Random trap spawn | Zone Feature |
| 37 | ROOM_RANDOM_CHEST | Random chest spawn | Zone Feature |
| 38 | ROOM_HARVEST_NODE | Harvest location | Special |
| 39 | ROOM_ROAD | Road/path | Environment |
| 40 | ROOM_VEHICLE | Vehicle room | Special |
| 41 | ROOM_DOCKABLE | Docking location | Special |

---

## Usage Guidelines

### For Builders

1. **Stacking Flags**: Many flags can be combined for compound effects
   - ROOM_DARK + ROOM_MAGICDARK = absolute darkness
   - ROOM_PEACEFUL + ROOM_NOMAGIC = complete safe zone
   - ROOM_INDOORS + ROOM_REGEN = safe indoor rest area

2. **Common Combinations**:
   - Safe Town: ROOM_PEACEFUL + ROOM_INDOORS
   - Death Trap: ROOM_DEATH + ROOM_NOTELEPORT + ROOM_NORECALL
   - Private Meeting: ROOM_PRIVATE + ROOM_SOUNDPROOF
   - Challenging Dungeon: ROOM_DARK + ROOM_NORECALL + ROOM_NOTELEPORT

3. **Flags to Avoid Setting Manually**:
   - ROOM_BFS_MARK (system temporary flag)
   - ROOM_HOUSE_CRASH (auto-set by house system)
   - ROOM_OLC (OLC system flag)

### For Developers

1. **Checking Flags**: Always use `ROOM_FLAGGED(room, FLAG)` macro
2. **Setting Flags**: Use `SET_BIT_AR(ROOM_FLAGS(room), FLAG)`
3. **Removing Flags**: Use `REMOVE_BIT_AR(ROOM_FLAGS(room), FLAG)`
4. **Temporary Flags**: BFS_MARK is the only truly temporary flag; others may be modified by spells/events but should persist in world files

### Common Patterns

**Safe Zones**: Typically use ROOM_PEACEFUL to prevent combat. Consider adding ROOM_NORECALL for dungeons where escape should be limited.

**Darkness**: Use ROOM_DARK for natural darkness, ROOM_MAGICDARK for magical darkness that resists light sources.

**Movement Restrictions**:
- ROOM_TUNNEL limits total occupants
- ROOM_SINGLEFILE limits combat positioning
- ROOM_PRIVATE limits to 2 people

**Magic Control**:
- ROOM_NOMAGIC = complete magic shutdown
- ROOM_NOTELEPORT = prevents teleportation only
- ROOM_NOSUMMON = prevents summoning only
- ROOM_NORECALL = prevents recall only

---

## Code References

**Primary Files:**
- `src/structs.h` (lines 225-271) - Flag definitions
- `src/movement.c` - Movement restriction checks
- `src/spell_parser.c` - Magic restriction checks
- `src/fight.c` - Combat restriction checks
- `src/utils.c` - Light and vision calculations
- `src/act.informative.c` - Room description handling
- `src/house.c` - House system integration
- `src/vessels_rooms.c` - Vehicle system integration

**Checking Flags:**
```c
// Standard check
if (ROOM_FLAGGED(room_rnum, ROOM_PEACEFUL)) {
    // Peaceful room logic
}

// Multiple flags
if (ROOM_FLAGGED(room, ROOM_DARK) && ROOM_FLAGGED(room, ROOM_MAGICDARK)) {
    // Absolute darkness
}
```

**Modifying Flags:**
```c
// Set flag
SET_BIT_AR(ROOM_FLAGS(room), ROOM_DARK);

// Remove flag
REMOVE_BIT_AR(ROOM_FLAGS(room), ROOM_DARK);

// Check if set
if (IS_SET_AR(ROOM_FLAGS(room), ROOM_DARK)) {
    // Flag is set
}
```

---

## Notes

- Flags are stored as bitflags in room data structures
- Multiple flags can be active simultaneously
- Some flags interact with each other (e.g., ROOM_DARK + ROOM_MAGICDARK)
- Several flags appear to be planned features with limited implementation (ROOM_DOCKABLE, ROOM_OCCUPIED, ROOM_AIRY, etc.)
- System flags (BFS_MARK, OLC, HOUSE_CRASH) should generally not be manually set by builders

---

**Last Updated:** November 6, 2024  
**Version:** 1.0  
**Maintainer:** LuminariMUD Development Team
