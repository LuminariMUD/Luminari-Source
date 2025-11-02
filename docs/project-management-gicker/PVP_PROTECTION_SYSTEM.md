# PVP Protection System Implementation

## Overview
This document describes the comprehensive Player vs Player (PVP) protection system that prevents players from attacking each other unless both parties have explicitly enabled PVP, or the MUD configuration allows player killing globally.

## System Design

### Core Philosophy
- **Opt-in PVP**: Players must explicitly enable PVP (via the `pvp` command) before they can participate in player combat
- **Mutual Consent**: BOTH the attacker and defender must have PVP enabled
- **Comprehensive Coverage**: All forms of combat initiation are protected (direct attacks, spells, abilities, pets)
- **Arena Exception**: The arena area (rooms 138600-138608) bypasses all PVP restrictions
- **CONFIG_PK_ALLOWED Override**: If the global PK config is disabled, no PVP is allowed except in the arena

## Implementation Details

### 1. Core PVP Check Function (`utils.c`)

**Function**: `bool pvp_ok(struct char_data *ch, struct char_data *target, bool display)`

**Location**: `src/utils.c` (around line 6405)

**Logic Flow**:
1. **NPC vs NPC**: Always allowed (return true)
2. **CONFIG_PK_ALLOWED Check**: If disabled, only arena combat is allowed
3. **NPC Pet/Follower Handling**:
   - If attacker is an NPC with a player master, check the master's PVP flag
   - If target is an NPC with a player master, check the master's PVP flag
4. **PC vs PC**: Both players must have PRF_PVP enabled (or be in arena)
5. **Arena Exception**: Rooms 138600-138608 bypass all PVP flag requirements

**Key Features**:
- Handles all combinations: PC vs PC, PC vs NPC, NPC vs PC, NPC pets
- Arena rooms always allow combat
- Optional display parameter controls whether error messages are shown
- Returns true if combat is allowed, false otherwise

### 2. Direct Combat Commands

#### Hit/Kill/Attack (`act.offensive.c`)

**Function**: `do_hit()` (around line 2900)

**Protection Added**:
```c
/* PVP CHECK - prevent attacking players without mutual PVP consent */
if (!IS_NPC(vict) || (IS_NPC(vict) && vict->master && !IS_NPC(vict->master)))
{
  if (!pvp_ok(ch, vict, true))
    return;
}
```

**Location**: After ROOM_SINGLEFILE check, before combat initiation

**Coverage**: This protects:
- `hit` command
- `kill` command (calls do_hit)
- `attack` command (aliased to hit)

#### Assist Command (`act.offensive.c`)

**Function**: `perform_assist()` (around line 1720)

**Protection Added**: Replaced CONFIG_PK_ALLOWED check with full pvp_ok() validation

**Coverage**: Prevents assisting in combat against players without PVP consent

### 3. Spell System Protection

#### Spell Casting (`spell_parser.c`)

**Function**: `call_magic()` (around line 550)

**Protection Added**:
```c
/* PVP CHECK - prevent casting harmful spells on players without mutual PVP consent */
if (cvict && (SINFO.violent || IS_SET(SINFO.routines, MAG_DAMAGE)))
{
  if (!IS_NPC(cvict) || (IS_NPC(cvict) && cvict->master && !IS_NPC(cvict->master)))
  {
    if (!pvp_ok(caster, cvict, true))
      return (0);
  }
}
```

**Location**: After ROOM_PEACEFUL check, before spell effects are applied

**Coverage**: This protects ALL offensive spells including:
- Direct damage spells (magic missile, fireball, etc.)
- Debuff spells (hold person, blindness, etc.)
- Charm spells (already had CONFIG check, now also checks PVP flags)
- Any spell marked as `violent` or with `MAG_DAMAGE` routine

### 4. Special Combat Abilities

#### Backstab (`act.offensive.c`)

**Function**: `do_backstab()` (around line 3148)

**Protection Added**: PVP check before calling perform_backstab()

**Coverage**: Prevents backstabbing players without PVP consent

#### Knockdown Abilities (`act.offensive.c`)

**Function**: `perform_knockdown()` (around line 605)

**Protection Added**: PVP check within the perform function

**Coverage**: This protects:
- `bash` command
- `trip` command
- `bodyslam` command
- Shield charge
- Any other ability that uses perform_knockdown()

### 5. Other Protected Abilities

The following abilities already had pvp_ok() checks or now benefit from the comprehensive system:

- **Lich Touch** (`perform_lichtouch`): Already had pvp_ok() check
- **Touch of Corruption** (`do_touchofcorruption`): Already had pvp_ok() check
- **Shieldslam** (`perform_shieldslam`): Calls perform_knockdown (protected)
- **Kick** (`perform_kick`): Calls hit() which triggers combat (protected)
- **Charge** (`perform_charge`): Calls hit() which triggers combat (protected)
- **All other combat-initiating abilities**: Protected via hit() or spell system

## Error Messages

When PVP checks fail, players receive context-appropriate messages:

### Attacker Messages:
- "You must enable PVP (type 'pvp') before attacking other players."
- "Your target does not have PVP enabled and cannot be attacked."
- "You cannot attack another player's pet unless you have PVP enabled."
- "You cannot attack the pet of a player who doesn't have PVP enabled."
- "Player killing is not allowed on this MUD." (if CONFIG_PK_ALLOWED is off)
- "Your pet cannot attack other players unless you have PVP enabled." (for pet masters)

### Pet Master Messages:
- "Your pet cannot attack players who don't have PVP enabled."

## Configuration Requirements

### Global Settings

**File**: `src/config.c`

**Variable**: `pk_allowed` (default: NO)

**Effect**: When set to NO, all PVP is disabled except in the arena

### Arena Definition

**File**: `src/utils.h`

**Constants**:
- `ARENA_START`: 138600
- `ARENA_END`: 138608

**Effect**: Rooms in this range bypass all PVP restrictions

### Player Preferences

**Flag**: `PRF_PVP` (preference flag #49)

**Commands**:
- `pvp` - Toggle PVP on/off (15-minute cooldown after enabling)
- `prefedit` - Option 'K' in "More Preferences" menu

**Requirements**:
- CONFIG_PK_ALLOWED must be enabled to toggle PVP
- 15-minute cooldown after enabling before it can be disabled
- Persists across login/logout (saved in pfile)

## Testing Scenarios

### Test Case 1: Attack with PVP Disabled
**Setup**: Player A (PVP OFF) tries to attack Player B (PVP ON)
**Expected**: Attack fails with message: "You must enable PVP (type 'pvp') before attacking other players."

### Test Case 2: Attack Target with PVP Disabled
**Setup**: Player A (PVP ON) tries to attack Player B (PVP OFF)
**Expected**: Attack fails with message: "Your target does not have PVP enabled and cannot be attacked."

### Test Case 3: Both Players with PVP Enabled
**Setup**: Player A (PVP ON) attacks Player B (PVP ON)
**Expected**: Combat initiates normally

### Test Case 4: Arena Combat
**Setup**: Player A (PVP OFF) attacks Player B (PVP OFF) in arena (room 138600)
**Expected**: Combat initiates normally (arena bypasses PVP flags)

### Test Case 5: Spell Casting
**Setup**: Player A (PVP OFF) casts fireball at Player B (PVP ON)
**Expected**: Spell fails with PVP error message

### Test Case 6: Pet Combat
**Setup**: Player A's pet (A has PVP OFF) attacks Player B (PVP ON)
**Expected**: Attack fails with message to Player A about pet restriction

### Test Case 7: CONFIG_PK_ALLOWED Disabled
**Setup**: CONFIG_PK_ALLOWED = NO, Player A (PVP ON) attacks Player B (PVP ON) outside arena
**Expected**: Attack fails with message: "Player killing is not allowed on this MUD."

### Test Case 8: NPC Combat
**Setup**: Player A (any PVP status) attacks a regular NPC (no master)
**Expected**: Combat initiates normally (NPCs always attackable)

### Test Case 9: Backstab
**Setup**: Player A (PVP OFF) tries to backstab Player B (PVP ON)
**Expected**: Backstab fails with PVP error message

### Test Case 10: Bash/Trip/Bodyslam
**Setup**: Player A (PVP ON) tries to bash Player B (PVP OFF)
**Expected**: Knockdown fails with PVP error message

## Files Modified

### Core Files
1. **src/utils.c** (line ~6405): Completely rewrote pvp_ok() function
2. **src/act.offensive.c**:
   - Line ~2900: Added PVP check in do_hit()
   - Line ~1720: Updated perform_assist() with pvp_ok()
   - Line ~3148: Added PVP check in do_backstab()
   - Line ~605: Added PVP check in perform_knockdown()
3. **src/spell_parser.c** (line ~550): Added PVP check in call_magic()

### Related Files (Previously Implemented)
- **src/structs.h**: PRF_PVP flag definition (line 1318)
- **src/utils.h**: GET_PVP_TIMER macro (line 2241)
- **src/act.other.c**: PVP toggle command logic
- **src/prefedit.c**: PVP preference editor integration
- **src/players.c**: PVP timer save/load/initialization
- **src/constants.c**: PVP preference bit name

## Future Enhancements

Potential improvements to consider:

1. **PVP Zones**: Designate specific zones as PVP-enabled areas
2. **Guild Wars**: Allow guild-based PVP declarations
3. **Duel System**: Formal duel requests that bypass PVP flags temporarily
4. **PVP Rankings**: Track PVP victories/defeats for leaderboards
5. **Alignment-Based PVP**: Allow evil vs good combat regardless of flags
6. **Level Restrictions**: Prevent high-level players from attacking low-level players
7. **Combat Logging**: Enhanced logging for PVP encounters

## Debugging Tips

### Common Issues

**Problem**: Players with PVP enabled can't attack each other
**Check**: 
- Verify CONFIG_PK_ALLOWED is enabled
- Check if they're in a ROOM_PEACEFUL flagged room
- Verify both players have PRF_PVP flag set

**Problem**: Arena combat not working
**Check**:
- Verify room vnums are between 138600 and 138608
- Check ARENA_START and ARENA_END constants in utils.h

**Problem**: Pet combat issues
**Check**:
- Verify pet master's PVP flag
- Check if pet actually has a master (ch->master)
- Verify target's PVP flag (if target is a PC or pet)

### Log Messages

The system uses existing logging. To debug, check:
- Player feedback messages (sent via send_to_char)
- Combat logs (if enabled)
- Syslog for any errors

## Conclusion

This PVP protection system provides comprehensive, multi-layered protection against unwanted player vs player combat. It covers all major combat initiation methods including direct attacks, spells, abilities, and pets. The system is designed to be fair, transparent, and easy to understand for players while being robust and maintainable for developers.

## Related Documentation

- `docs/PVP_FLAG_IMPLEMENTATION.md` - PVP flag and toggle system
- `docs/PVP_COOLDOWN_DISPLAY.md` - PVP cooldown display in cooldowns command
- `docs/PVP_TIMER_INITIALIZATION_FIX.md` - PVP timer save/load implementation
