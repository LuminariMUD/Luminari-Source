# Mount System Audit
**Generated: 2025-08-14**

## Overview
This document provides a comprehensive audit of all mount-related code in the LuminariMUD codebase, including regular mounts, paladin/blackguard mounts, and dragon mounts.

## Core Mount System Files

### 1. Primary Command Implementation
- **src/act.other.c**
  - `ACMD(do_mount)` - src/act.other.c:2421
  - `ACMD(do_dismount)` - src/act.other.c:2500
  - `ACMD(do_buck)` - src/act.other.c:2519
  - `ACMD(do_tame)` - src/act.other.c:2543
  - `ACMD(do_call)` - src/act.other.c:2029

### 2. Handler Functions
- **src/handler.c**
  - `mount_char()` - src/handler.c:3355
  - `dismount_char()` - src/handler.c:3340

### 3. Movement Integration
- **src/movement.c**
  - Mount movement checks - src/movement.c:273-296
  - Mount speed calculations - src/movement.c:730-736
  - Riding skill checks - src/movement.c:767-777
  - Mount movement point usage - src/movement.c:790-794
  - Mount room transitions - src/movement.c:815-877

### 4. Limit Checks
- **src/limits.c**
  - Mount validation checks - src/limits.c:265-291
  - Ensures rider and mount remain synchronized

## Mount Data Structures

### Core Structures (src/structs.h)
```c
struct char_data {
    struct char_data *riding;    // Who are they riding? - src/structs.h:4816
    struct char_data *ridden_by; // Who is riding them? - src/structs.h:4817
    int mounted_blocks_left;     // Mounted combat blocks - src/structs.h:4798
}
```

### Macros (src/utils.h)
- `RIDING(ch)` - src/utils.h:1643
- `RIDDEN_BY(ch)` - src/utils.h:1644

## Mount Types

### 1. Regular Mountable Creatures
- **Flag**: `MOB_MOUNTABLE` - src/structs.h:1100
- Can be mounted by any player with ride skill
- Size requirements: Mount must be 1-2 sizes larger than rider

### 2. Paladin Mounts (src/structs.h:1269-1272)
- `MOB_PALADIN_MOUNT` (vnum 70)
- `MOB_PALADIN_MOUNT_SMALL` (vnum 91)
- `MOB_EPIC_PALADIN_MOUNT` (vnum 79)
- `MOB_EPIC_PALADIN_MOUNT_SMALL` (vnum 92)

### 3. Blackguard Mounts (src/structs.h:1273-1275)
- `MOB_BLACKGUARD_MOUNT` (vnum 20804)
- `MOB_ADV_BLACKGUARD_MOUNT` (vnum 20805)
- `MOB_EPIC_BLACKGUARD_MOUNT` (vnum 20803)

### 4. Dragon Mounts (Dragonrider Class)
- **Standard Campaign**: vnums 1240-1249 (src/act.other.c:1746-1751)
- **DragonLance Campaign**: vnums 40401-40410 (src/act.other.c:1738-1744)
- Dragon types selected via study menu

## Mount-Related Feats

### Combat Feats (src/feats.c)
1. **FEAT_MOUNTED_COMBAT** (src/structs.h:1797)
   - Once per round negate hit with ride check - src/feats.c:1454-1458
   - Prevents being thrown from mount

2. **FEAT_MOUNTED_ARCHERY** (src/structs.h:1797)
   - Removes -4 penalty for mounted archery - src/feats.c:1473-1478

3. **FEAT_RIDE_BY_ATTACK** 
   - Mounted charge attacks - src/feats.c:1459-1463

4. **FEAT_SPIRITED_CHARGE**
   - Double damage on mounted charge - src/feats.c:1464-1469

### Paladin/Blackguard Mount Feats
1. **FEAT_CALL_MOUNT** (src/structs.h:1910)
   - Allows calling mount - src/feats.c:3630-3632

2. **FEAT_GLORIOUS_RIDER** (src/structs.h:2139)
   - Use CHA instead of DEX for ride - src/feats.c:3637-3641

3. **FEAT_LEGENDARY_RIDER** (src/structs.h:2140)
   - No armor penalty, block extra attack - src/feats.c:3642-3646

4. **FEAT_EPIC_MOUNT** (src/structs.h:2141)
   - More powerful mount - src/feats.c:3647-3649

## Dragon Rider System

### Dragon Rider Class
- **CLASS_DRAGONRIDER** (src/structs.h:459)

### Dragon Mount Feats
1. **FEAT_DRAGON_BOND** - src/feats.c:4802-4804
   - Summon dragon mount via 'call' command

2. **FEAT_DRAGON_LINK** - src/feats.c:4805-4807
   - Share buff spells with mount

3. **FEAT_RIDERS_BOND** (src/structs.h:2856) - src/feats.c:4808-4810
   - Select bond type: champion/scion/kin

4. **FEAT_DRAGON_MOUNT_BOOST** (src/structs.h:2086) - src/feats.c:5198
   - +18 HP, +1 AC, +1 hit/damage per rank

5. **FEAT_DRAGON_MOUNT_BREATH** (src/structs.h:2087) - src/feats.c:5199
   - Use dragon breath weapon

6. **FEAT_ADEPT_RIDER** (src/structs.h:2858) - src/feats.c:4814-4816
   - +2 ride skill, spell abilities

7. **FEAT_SKILLED_RIDER** (src/structs.h:2860) - src/feats.c:4820-4822
   - +2 ride skill, heal mount/acid arrow

8. **FEAT_MASTER_RIDER** (src/structs.h:2861) - src/feats.c:4823-4825
   - +2 ride skill, lightning bolt/slow

9. **FEAT_DRAGOON_POINTS** - src/feats.c:4829-4833
   - Points for casting rider spells

### Dragon Rider Utility Functions (src/utils.c)
- `is_dragon_rider_mount()` - src/utils.c:9690
- `is_riding_dragon_mount()` - src/utils.c:9674
- `is_paladin_mount()` - src/utils.c:7721

### Dragon Rider Macros (src/utils.h)
- `IS_DRAGONRIDER(ch)` - src/utils.h:2079
- `GET_DRAGON_RIDER_DRAGON_TYPE(ch)` - src/utils.h:2731
- `HAS_DRAGON_BOND_ABIL(ch, level, type)` - src/utils.h:2733

## Call System

### Call Command Implementation (src/act.other.c:1619-2027)
Handles summoning of various companion types:
- `MOB_C_MOUNT` - Paladin/Blackguard mounts
- `MOB_C_DRAGON` - Dragon mounts
- `MOB_C_ANIMAL` - Animal companions
- `MOB_C_FAMILIAR` - Familiars
- `MOB_SHADOW` - Shadow dancer shadows
- `MOB_EIDOLON` - Summoner eidolons

### Cooldown Events (src/mud_event.h)
- `eC_MOUNT` - Paladin mount cooldown
- `eC_DRAGONMOUNT` - Dragon mount cooldown (src/mud_event.h:198)
- `eC_ANIMAL` - Animal companion cooldown
- `eC_FAMILIAR` - Familiar cooldown
- `eSUMMONSHADOW` - Shadow cooldown
- `eC_EIDOLON` - Eidolon cooldown

### Mount Summoning Process
1. Check if player has required feat/class
2. Verify mount type selected (via study)
3. Check cooldown status
4. Load mobile from vnum
5. Set level and stats
6. Apply charm effect
7. Add to group
8. Start cooldown (4 MUD days)

## Study System Integration

### Dragon Rider Study Menu (src/study.c)
- `show_dragon_rider_menu()` - src/study.c:2491
- `show_dragon_rider_bond_menu()` - src/study.c:2508
- Dragon type selection - src/study.c:4829-4863
- Bond type selection - src/study.c:4872-4882

### Player Data Storage
- `dragon_rider_dragon_type` - src/structs.h:5173, 5476
- `dragon_rider_bond_type` - src/structs.h:5477

## Commands Registry (src/interpreter.c)

| Command | Position | Function | Line |
|---------|----------|----------|------|
| mount | POS_FIGHTING | do_mount | 608 |
| dismount | POS_FIGHTING | do_dismount | 348 |
| buck | POS_FIGHTING | do_buck | 235 |
| tame | POS_FIGHTING | do_tame | 938 |
| call | POS_FIGHTING | do_call | 288 |

## Mount Mechanics

### Mounting Requirements
1. Target must be NPC (or player is immortal)
2. Must have ride skill
3. Mount must be 1-2 sizes larger than rider
4. Must pass ride skill check
5. NPC must have MOB_MOUNTABLE flag

### Movement While Mounted
- Uses mount's movement points
- Uses mount's speed for movement delay
- Chance of being thrown based on ride skill
- Mount follows rider between rooms
- Wilderness coordinate synchronization

### Combat While Mounted
- Mounted combat feat allows blocking attacks
- Mounted archery removes penalties
- Charge attacks gain damage bonuses
- Dragon riders share damage resistance

### Taming System
- Requires ride skill
- Applies AFF_TAMED to mount
- Tamed mounts won't buck riders
- Cooldown of 3 hours

## Special Mount Abilities

### Dragon Mount Abilities
1. **Breath Weapon** - Uses FEAT_DRAGON_MOUNT_BREATH
2. **Stat Boosts** - Via FEAT_DRAGON_MOUNT_BOOST
3. **Spell Sharing** - Via FEAT_DRAGON_LINK
4. **Energy Resistance** - Based on dragon type

### Paladin Mount Abilities
- Higher level mounts for epic characters
- Size adjusts to rider
- Enhanced movement points (500)

## Files With Mount References

### Core Implementation Files
1. src/act.other.c - Primary mount commands
2. src/handler.c - Mount/dismount functions
3. src/movement.c - Movement integration
4. src/limits.c - Mount validation
5. src/utils.c - Mount utility functions
6. src/feats.c - Mount-related feats
7. src/study.c - Dragon rider selection
8. src/interpreter.c - Command registration

### Support Files
1. src/structs.h - Data structures and defines
2. src/utils.h - Macros and function declarations
3. src/handler.h - Function prototypes
4. src/act.h - Command declarations
5. src/mud_event.h - Event definitions

### Combat Integration
1. src/fight.c - Mounted combat mechanics
2. src/act.offensive.c - Dragon breath weapons
3. src/magic.c - Spell sharing with mounts

### Class Files
1. src/class.c - Paladin/Blackguard mount assignment
2. src/race.c - Size considerations

## Known Mount VNums

### Paladin Mounts
- 70 - Standard paladin mount
- 91 - Small paladin mount
- 79 - Epic paladin mount
- 92 - Epic small paladin mount

### Blackguard Mounts
- 20804 - Standard blackguard mount
- 20805 - Advanced blackguard mount
- 20803 - Epic blackguard mount

### Dragon Mounts
- 1240-1249 - Standard campaign dragons
- 40401-40410 - DragonLance campaign dragons

## Notes and Observations

1. **Comprehensive System**: The mount system is well-integrated with combat, movement, and character progression systems.

2. **Multiple Mount Types**: Supports regular mounts, class-specific mounts, and dragon mounts with different mechanics.

3. **Skill Integration**: Heavy reliance on ride skill for mount control and combat effectiveness.

4. **Size Mechanics**: Proper size validation ensures realistic mounting scenarios.

5. **Cooldown System**: Uses MUD events for managing summon cooldowns.

6. **Campaign Variations**: Different vnums for different campaign settings (standard vs DragonLance).

7. **Future Expansion**: System appears designed to easily add new mount types and abilities.