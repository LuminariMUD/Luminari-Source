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

Room flags are bitflags defined in `src/structs.h` and are checked throughout the codebase using the `ROOM_FLAGGED()` macro. There are currently 47 room flags (indices 0-46) that control everything from movement restrictions to magical effects.

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
- `src/movement/movement.c` - Blocks mounted entry (`do_simple_move()`)
- `src/movement/movement.c` - Enforces occupancy limit (`do_simple_move()`)
- `src/wilderness/desc_engine.c` - Dynamic room descriptions (`gen_room_description()`)

### ROOM_SINGLEFILE (Index: 20)
**Effect:** Forces characters to move through the room one at a time in combat situations.
- Prevents backstab and other flanking abilities
- Limits positioning-based combat mechanics
- Used for narrow corridors and single-file passages

**Code References:**
- `src/movement/movement.c` - Movement restrictions (`do_simple_move()`)
- `src/combat/fight.c` - Combat positioning checks (`guard_check()`, `compute_hit_damage()`, `hit()`)
- 20+ additional checks across combat and spell systems

### ROOM_FLY_NEEDED (Index: 18)
**Effect:** Characters must be flying to remain in the room; non-flying characters fall.
- Triggers falling mechanics for grounded characters
- Objects without support also fall
- Used for aerial rooms, cliff edges, and suspended platforms

**Code References:**
- `src/movement/movement_falling.c` - Character falling check (`obj_should_fall()`, `char_should_fall()`, `event_falling()`)
- `src/movement/movement_falling.c` - Object falling check (`obj_should_fall()`, `char_should_fall()`, `event_falling()`)
- `src/craft/craft.c` - Crafting system checks (`reset_harvesting_rooms()`)

### ROOM_CLIMB_NEEDED (Index: 32)
**Effect:** Requires a skill check to enter the room.
- Difficulty based on zone minimum level
- Used for cliffs, mountains, and vertical obstacles

**Code References:**
- `src/movement/movement.c` - Entry skill check (`do_simple_move()`)
- `src/craft/craft.c` - Crafting restrictions (`reset_harvesting_rooms()`)

### ROOM_NOFLY (Index: 26)
**Effect:** Prevents flying creatures from entering the room.
- Forces grounded movement
- Used in low-ceiling areas and confined spaces

**Code References:**
- `src/movement/movement.c` - Entry restriction check (`do_simple_move()`)

### ROOM_PRIVATE (Index: 9)
**Effect:** Limits room to 2 occupants maximum.
- Additional occupants cannot enter or teleport in
- Staff can override this restriction
- Used for private meeting rooms and intimate spaces

**Code References:**
- `src/movement/movement.c` - Entry restriction (`do_enter()`)
- `src/act.wizard.c` - Teleport restrictions (`find_target_room()`)
- `src/magic/spells.c` - Spell targeting restrictions (`valid_mortal_tele_dest()`)
- `src/obj/house.c` - House management integration (`find_house()`, `hcontrol_build_house()`, `hcontrol_destroy_house()`)

### ROOM_NOMOB (Index: 2)
**Effect:** NPCs will not wander into this room.
- Checked by the mobile movement routine, so a mob will not choose this room as a destination
- Does **not** prevent a mobile from being placed here by a zone reset, a summon, or a staff `transfer`
- Use it to keep wandering mobiles out of shops, guild rooms, and quest areas
  without having to make the exits impassable

**Code References:**
- `src/mob/mob_act.c` - Wandering destination check (`mobile_activity()`)

### ROOM_STAFFROOM (Index: 10)
**Effect:** Only staff (LVL_STAFF+) can enter this room.
- Hard block on entry for non-staff
- Used for immortal areas and staff lounges

**Code References:**
- `src/movement/movement.c` - Level check on entry (`do_simple_move()`, `do_enter()`)

---

## Magic Restrictions

### ROOM_NOMAGIC (Index: 7)
**Effect:** Completely prevents all magic from functioning in the room.
- Blocks spell casting by caster or victim in the room
- Exception: Weapon poison (CAST_WEAPON_POISON) still works
- Affects both offensive and beneficial magic
- Strongest magic restriction available

**Code References:**
- `src/magic/spell_parser.c` - Spell casting prevention (`call_magic()`, `do_gen_cast()`)
- `src/combat/fight.c` - Combat magic checks (`weapon_spells()`, `idle_weapon_spells()`)
- `src/magic/magic.c` - General magic check (`mag_room()`)

### ROOM_NOTELEPORT (Index: 21)
**Effect:** Prevents teleportation into or out of the room.
- Blocks dimension door, teleport, and similar spells
- Blocks both incoming and outgoing teleportation
- Used for secured areas and restricted zones

**Code References:**
- `src/magic/spells.c` - Teleport destination check (`valid_mortal_tele_dest()`, `spell_recall()`, `spell_luskan_recall()`)
- `src/magic/spells.c` - Multiple recall/teleport spell checks (`valid_mortal_tele_dest()`, `spell_recall()`, `spell_luskan_recall()`)
- `src/act.other.c` - Recall and portal restrictions (`do_abundantstep()`, `do_shadowstep()`)

### ROOM_NOSUMMON (Index: 24)
**Effect:** Prevents summoning spells from targeting this room.
- Blocks summoning creatures to or from the room
- Used for protected sanctuaries and anti-summoning zones

**Code References:**
- `src/magic/spells.c` - Summon spell checks (`spell_summon()`)

### ROOM_NORECALL (Index: 19)
**Effect:** Prevents recall/return spells from working.
- Blocks escape via recall magic
- Used in dungeons and combat zones where escape should be prevented

**Code References:**
- `src/magic/spells.c` - Multiple recall spell checks (8 matches) (`spell_recall()`, `spell_luskan_recall()`, `spell_triboar_recall()`)

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
- `src/combat/fight.c` - Combat initiation blocks (`check_killer()`, `damage()`, `hit()`)
- `src/magic/spells.c` - Offensive spell blocks (`event_acid_arrow()`, `event_aqueous_orb()`, `event_implode()`)
- `src/combat/grapple.c` - Grappling prevention (`do_grapple()`)
- `src/magic/domain_powers.c` - Domain power checks (`do_eviltouch()`, `do_lightningarc()`, `do_aciddart()`)
- `src/magic/spell_parser.c` - Spell casting restrictions (`call_magic()`, `do_gen_cast()`)

### ROOM_ARENA (Index: 43)
**Effect:** Applies arena combat, PvP, and death handling to a room regardless of its VNUM.
- Allows PvP actions under the same rules as the legacy arena VNUM range
- Uses arena-specific death handling instead of ordinary character death
- Added for converted Realms of Luminari rooms whose arena identity is flag-based

**Code References:**
- `src/utils.h` - Arena-room classification (`IS_ARENA()`, `IN_ARENA()`)
- `src/utils.c` - Arena bypass for PvP eligibility (`pvp_ok()`, `pvp_ok_single()`)
- `src/combat/fight.c` - Arena combat and death handling

### ROOM_DEATH (Index: 1)
**Effect:** Kills characters or deals severe damage when entering/remaining in room.
- Used for death traps and hazardous zones
- NPCs avoid entering death rooms
- Teleportation into death rooms is blocked
- Staff can bypass with appropriate level

**Code References:**
- `src/act.informative.c` - Display warnings (`look_at_room_number()`, `look_at_room()`)
- `src/movement/movement.c` - Entry damage/death (`do_enter()`)
- `src/mob/mob_act.c` - NPC avoidance (`mobile_activity()`)
- `src/magic/spells.c` - Teleport blocking (`valid_mortal_tele_dest()`)
- `src/act.other.c` - Additional teleport checks (`do_shadowstep()`)

### ROOM_SOUNDPROOF (Index: 5)
**Effect:** Blocks sound-based abilities and prevents communication.
- Blocks tells, shouts, and speech-based communication
- Prevents bardic performances from functioning
- Blocks sound-based spells (exception for psionics)
- Staff can override (LVL_STAFF+)

**Code References:**
- `src/act.comm.c` - Communication blocking (`is_tell_ok()`, `do_gen_comm()`)
- `src/bardic_performance.c` - Bardic performance prevention (`can_perform()`)
- `src/magic/spell_parser.c` - Spell casting restriction (psionics exempt) (`say_spell()`, `cast_spell()`)
- `src/act.social.c` - Social action blocking (`do_gmote()`)

---

## Environmental Effects

### ROOM_INDOORS (Index: 3)
**Effect:** Marks room as indoors for various game mechanics.
- Affects weather exposure
- Influences light calculations (indoor rooms don't go dark at night)
- Affects outdoor-only abilities and effects
- Used for buildings, caves, and sheltered areas

**Code References:**
- `src/utils.c` - Weather and outdoor checks (`is_room_outdoors()`, `ultra_blind()`)
- `src/movement/movement.c` - Outdoor transition detection (`do_leave()`)
- `src/vessels/vessels_rooms.c` - Vehicle room templates (`load_ship_room_templates_from_db()`, `create_ship_room()`, `room_has_outside_view()`)

### ROOM_REGEN (Index: 17)
**Effect:** Doubles natural regeneration rates for HP, mana, and movement.
- Minimum regen of 2 before doubling (2 becomes 4, etc.)
- Stacks with other regen bonuses
- Used for healing sanctuaries and rest areas

**Code References:**
- `src/limits.c` - HP regeneration doubling (`regen_hps()`)
- `src/magic/spell_prep.c` - Spell preparation bonus (`compute_spells_prep_time()`)

### ROOM_NO_PRECIP (Index: 42)
**Effect:** Suppresses ordinary weather and precipitation messages while leaving the room outdoors.
- Does not make the room indoors
- Does not suppress sunrise, sunset, or other outdoor-only mechanics
- Used by converted Realms of Luminari `NO_PRECIP` rooms

**Code References:**
- `src/weather.c` - Weather-message filtering (`sect_no_weather()`)

### ROOM_PSP_REGEN (Index: 45)
**Effect:** Doubles the net PSP gained during each non-combat regeneration tick.
- Applies after the normal PSP, feat, position, and psionic-level bonuses
- Caps the resulting PSP at the character's maximum
- Used by converted Realms of Luminari `PSPREGEN` rooms

**Code References:**
- `src/limits.c` - PSP tick acceleration (`regen_psp()`)

### ROOM_ROL_HOME_RESET (Index: 46)
**Effect:** Updates an NPC's remembered home room after it successfully walks out of the marked room.
- Preserves converted RoL `home_reset` behavior without occupying the room's special-procedure slot
- Applies only to NPCs and only after successful movement
- Failed or trigger-rejected movement does not retarget the NPC's home

**Code References:**
- `src/movement/movement.c` - Successful-movement integration (`do_simple_move()`)
- `src/spec/spec_rol_conversion.c` - RoL home update (`rol_update_mobile_home_after_move()`)

### ROOM_NOHEAL (Index: 25)
**Effect:** Prevents natural healing and regeneration.
- Blocks HP regeneration tick
- Similar to Blackmantle spell effect
- Used for cursed areas and unholy ground

**Code References:**
- `src/limits.c` - Regeneration blocking (`regen_hps()`)

---

## Vision & Perception

### ROOM_DARK (Index: 0)
**Effect:** Forces the room to be dark regardless of other conditions.
- Overrides normal light calculations
- Can be temporarily removed/added by darkness/light spells
- Requires light source or darkvision to see
- Stacks with ROOM_MAGICDARK for absolute darkness

**Code References:**
- `src/utils.c` - Darkness calculations (`room_is_daylit()`, `room_is_dark()`, `is_room_in_sunlight()`)
- `src/wilderness/desc_engine.c` - Dynamic description generation (`gen_room_description()`)
- `src/magic/magic.c` - Darkness spell interaction (`mag_room()`)
- `src/mud_event.c` - Temporary darkness removal (`event_countdown()`)

### ROOM_MAGICDARK (Index: 22)
**Effect:** Creates magical darkness that penetrates normal light sources.
- Overrides mundane light sources
- Only defeated by magical light or certain abilities
- When combined with ROOM_DARK, creates absolute darkness
- Used for shadow magic zones and deep darkness effects

**Code References:**
- `src/utils.c` - Darkness calculations (`room_is_daylit()`, `room_is_dark()`, `is_room_in_sunlight()`)
- `src/act.informative.c` - Display and description handling (`look_at_room_number()`, `look_at_room()`, `do_scan()`)

### ROOM_MAGICLIGHT (Index: 23)
**Effect:** Creates magical light that prevents the room from being dark.
- Overrides normal darkness conditions
- Cannot be defeated by normal darkness
- Used for magically illuminated areas

**Code References:**
- `src/utils.c` - Light calculation override (`room_is_dark()`)

### ROOM_FOG (Index: 27)
**Effect:** Creates fog that obscures vision and limits visibility.
- Hides room descriptions and contents from mortal characters
- Limits automap functionality
- Reduces scan range and visibility
- Staff (LVL_IMMORT+) can see through fog
- Can be removed by gust of wind spell

**Code References:**
- `src/act.informative.c` - Vision restrictions (`look_at_room_number()`, `look_at_room()`, `look_in_direction()`)
- `src/asciimap.c` - Automap limitation (`do_map()`)
- `src/magic/spells.c` - Gust of wind removes fog (`perform_dispel()`)
- `src/utils.c` - Weather and vision checks (`ultra_blind()`, `is_room_in_sunlight()`)

### ROOM_NOTRACK (Index: 6)
**Effect:** Prevents tracking abilities from working through this room.
- Blocks track skill usage
- Breaks pathfinding algorithms
- NPCs won't track players through these rooms
- Used for anti-tracking zones and rivers

**Code References:**
- `src/movement/movement_tracks.c` - Track skill blocking (`should_create_tracks()`)
- `src/graph.c` - Pathfinding restriction
- `src/mob/mob_act.c` - NPC tracking limitation (`mobile_activity()`)

---

## Special Room Types

### ROOM_HOUSE (Index: 11)
**Effect:** Marks room as part of a player house.
- Integrates with house ownership system
- Restricts entry to house owners and guests
- Enables house crash-save functionality
- Used with ROOM_HOUSE_CRASH for persistence

**Code References:**
- `src/obj/house.c` - House management (`find_house()`, `hcontrol_build_house()`, `hcontrol_destroy_house()`)
- `src/handler.c` - Crash save integration (`obj_to_room()`, `obj_from_room()`)
- `src/act.wizard.c` - Teleport restrictions and admin tools (`find_target_room()`, `do_switch()`, `do_zcheck()`)

### ROOM_HOUSE_CRASH (Index: 12)
**Effect:** Marks that items in this house room should be saved.
- Auto-set when items are modified in house rooms
- Triggers house save-to-disk operations
- Cleared after successful save
- Used for house persistence system

**Code References:**
- `src/obj/house.c` - Save operations (`hcontrol_destroy_house()`, `hcontrol_pay_house()`)
- `src/handler.c` - Auto-flagging on item changes (`obj_to_room()`, `obj_from_room()`)
- `src/olc/hsedit.c` - House editor cleanup (`hsedit_delete_house()`)

### ROOM_ATRIUM (Index: 13)
**Effect:** Marks room as a house atrium (entry point).
- Entry point for player houses
- Special restrictions on entering houses apply
- Managed by house system automatically

**Code References:**
- `src/obj/house.c` - House system integration (`find_house()`, `hcontrol_build_house()`, `hcontrol_destroy_house()`)
- `src/movement/movement.c` - Entry point detection (`do_simple_move()`)
- `src/olc/hsedit.c` - House editor management (`hsedit_save_internally()`, `hsedit_delete_house()`)

### ROOM_WORLDMAP (Index: 16)
**Effect:** Marks room as part of the overworld/wilderness map system.
- Enables special worldmap automap display
- Used for wilderness travel system
- Affects routing and navigation commands

**Code References:**
- `src/act.informative.c` - Automap checks (`do_survey()`)
- `src/asciimap.c` - Map rendering (`show_worldmap()`)
- `src/vessels/routing.c` - Navigation system (`start_fr_flight_to_zone()`)
- `src/act.wizard.c` - Administrative flag setting (`do_setworldsect()`)

### ROOM_VEHICLE (Index: 40)
**Effect:** Marks room as part of a vehicle or vessel.
- Allows vehicle movement through this room
- Used for ship interiors, wagons, etc.
- Required for directional vehicle navigation
- Integrates with vessel system

**Code References:**
- `src/vessels/vessels_rooms.c` - `room_templates[]` sets `ROOM_VEHICLE` on every (`load_ship_room_templates_from_db()`, `create_ship_room()`)
  built-in ship room template; `load_ship_room_templates_from_db()` uses the same
  pair as the fallback when a database row omits the flags column
- `src/vessels/vessels_rooms.c` - `create_ship_room()` copies the flag onto the (`load_ship_room_templates_from_db()`, `create_ship_room()`)
  generated room

Note: the flag is set by the vessel room generator and read by the vessel
system as a whole. It is not consulted directly by the docking or movement
code, so do not expect to find it there.

### ROOM_HARVEST_NODE (Index: 38)
**Effect:** Marks room as a resource harvesting location.
- Enables gathering/harvesting of materials
- Integrates with crafting system
- Used for mining nodes, herb gardens, etc.

**Code References:**
- `src/craft/crafting_new.c` - Harvest node detection (`will_room_have_harvest_materials()`)

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

### ROOM_ROL_JAIL (Index: 44)
**Effect:** Persists the jail identity of a converted Realms of Luminari room.
- Reserved for the RoL conversion and its justice/special-procedure adapters
- Does not create a generic jail mechanic by itself
- Builders should not set this flag outside imported RoL content

**Code References:**
- `scripts/world/wtool_lib/rol_transform.py` - RoL room-flag conversion

### ROOM_OLC (Index: 14)
**Effect:** Marks room as being edited in the Online Level Creator (OLC).
- Used by building/editing system
- Prevents certain cleanup operations
- Temporary flag during world editing

**Code References:**
- `src/act.wizard.c` - Administrative cleanup avoidance (`do_zcheck()`)

### ROOM_BFS_MARK (Index: 15)
**Effect:** Temporary flag used by pathfinding algorithms.
- Marks rooms as visited during Breadth-First Search
- Automatically cleared after pathfinding completes
- Should never be permanently set on rooms
- Internal system flag

**Code References:**
- `src/graph.c` - Pathfinding algorithm
- `src/act.wizard.c` - Cleanup detection (`do_zcheck()`)

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
- `src/combat/traps_new.c` - Forced trap generation (`auto_generate_object_trap()`, `auto_generate_zone_traps()`)
- `src/db.c` - Zone loading trap setup (`reset_zone()`)

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

| Index | Flag Name | OLC Display Name | Primary Purpose | Category |
|-------|-----------|------------------|----------------|----------|
| 0 | ROOM_DARK | Dark | Forces darkness | Vision |
| 1 | ROOM_DEATH | Death-Trap | Death trap | Hazard |
| 2 | ROOM_NOMOB | No-Mob | NPC restriction | Movement |
| 3 | ROOM_INDOORS | Indoors | Indoor room | Environment |
| 4 | ROOM_PEACEFUL | Peaceful | No combat | Combat |
| 5 | ROOM_SOUNDPROOF | Soundproof | Blocks sound | Perception |
| 6 | ROOM_NOTRACK | No-Track | Blocks tracking | Perception |
| 7 | ROOM_NOMAGIC | No-Magic | No magic | Magic |
| 8 | ROOM_TUNNEL | Tunnel | Occupancy limit | Movement |
| 9 | ROOM_PRIVATE | Private | 2 person limit | Movement |
| 10 | ROOM_STAFFROOM | Staff-Room | Staff only | Access |
| 11 | ROOM_HOUSE | House | Player house | Special |
| 12 | ROOM_HOUSE_CRASH | House-Crash | House save flag | System |
| 13 | ROOM_ATRIUM | Atrium | House entrance | Special |
| 14 | ROOM_OLC | OLC | Being edited | System |
| 15 | ROOM_BFS_MARK | * | Pathfinding temp | System |
| 16 | ROOM_WORLDMAP | Worldmap | Wilderness map | Special |
| 17 | ROOM_REGEN | Regenerating | Double regen | Environment |
| 18 | ROOM_FLY_NEEDED | Fly-Needed | Must fly | Movement |
| 19 | ROOM_NORECALL | No-Recall | No recall | Magic |
| 20 | ROOM_SINGLEFILE | Singlefile | One at a time | Movement |
| 21 | ROOM_NOTELEPORT | No-Teleport | No teleport | Magic |
| 22 | ROOM_MAGICDARK | Magical-Darkness | Magical darkness | Vision |
| 23 | ROOM_MAGICLIGHT | Magical-Light | Magical light | Vision |
| 24 | ROOM_NOSUMMON | No-Summon | No summoning | Magic |
| 25 | ROOM_NOHEAL | No-Heal | No healing | Environment |
| 26 | ROOM_NOFLY | No-Fly | Flying blocked | Movement |
| 27 | ROOM_FOG | Fogged | Fog obscurement | Vision |
| 28 | ROOM_AIRY | Airy | Airy atmosphere | Environment |
| 29 | ROOM_OCCUPIED | Occupied | Occupied status | Special |
| 30 | ROOM_SIZE_TINY | Tiny-Sized-Room | Tiny size | Size |
| 31 | ROOM_SIZE_DIMINUTIVE | Diminutive-Sized-Room | Diminutive size | Size |
| 32 | ROOM_CLIMB_NEEDED | Climb-Needed | Requires climbing | Movement |
| 33 | ROOM_HASTRAP | Trapped | Contains trap | Zone Feature |
| 34 | ROOM_GENDESC | Wild-Generated-Desc | Generated desc | System |
| 35 | ROOM_PLAYER_SHOP | Player-Shop | Player shop | Special |
| 36 | ROOM_RANDOM_TRAP | Random-Trap | Random trap spawn | Zone Feature |
| 37 | ROOM_RANDOM_CHEST | Random-Chest | Random chest spawn | Zone Feature |
| 38 | ROOM_HARVEST_NODE | Always-Load-Harvest-Node | Harvest location | Special |
| 39 | ROOM_ROAD | Road | Road/path | Environment |
| 40 | ROOM_VEHICLE | Vehicle | Vehicle room | Special |
| 41 | ROOM_DOCKABLE | Dockable | Docking location | Special |
| 42 | ROOM_NO_PRECIP | No-Precipitation | Suppress weather messages | Environment |
| 43 | ROOM_ARENA | Arena | Arena combat and death rules | Combat |
| 44 | ROOM_ROL_JAIL | RoL-Jail | RoL justice compatibility marker | System |
| 45 | ROOM_PSP_REGEN | Psionic-Regeneration | Double PSP tick gain | Environment |
| 46 | ROOM_ROL_HOME_RESET | RoL-Home-Reset | Retarget NPC home after successful exit | System |

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
- `src/structs.h` - Flag definitions (the `ROOM_*` define block ending at `ROOM_ROL_HOME_RESET`)
- `src/movement/movement.c` - Movement restriction checks
- `src/magic/spell_parser.c` - Magic restriction checks
- `src/combat/fight.c` - Combat restriction checks
- `src/utils.c` - Light and vision calculations
- `src/act.informative.c` - Room description handling
- `src/obj/house.c` - House system integration
- `src/vessels/vessels_rooms.c` - Vehicle system integration

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
