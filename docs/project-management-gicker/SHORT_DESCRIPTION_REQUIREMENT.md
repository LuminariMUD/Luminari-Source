# Short Description Requirement for Non-Introduction System

## Overview
When the introduction system is disabled via CONFIG_USE_INTRO_SYSTEM, players must set their short description before they can enter the game for the first time.

## Implementation

### Location
**File**: `src/interpreter.c`
**Function**: CON_MENU state handler, case '1' (Enter Game)
**Lines**: ~4295-4305

### Logic Flow

When a player attempts to enter the game from the main menu (option 1), the system now checks:

1. **Is the introduction system disabled?** (`!CONFIG_USE_INTRO_SYSTEM`)
2. **Has the player NOT set their short description?** 
   - Either `GET_PC_DESCRIPTOR_1(d->character) == 0` 
   - OR `GET_PC_ADJECTIVE_1(d->character) == 0`

If BOTH conditions are true:
- Player is shown a message explaining the requirement
- Player is redirected to the short description setup menu
- Player cannot enter the game until they complete the setup

If either condition is false:
- Player enters the game normally

### Code Added

```c
case '1':
  /* Check if introduction system is OFF and player hasn't set short description */
  if (!CONFIG_USE_INTRO_SYSTEM && 
      (GET_PC_DESCRIPTOR_1(d->character) == 0 || GET_PC_ADJECTIVE_1(d->character) == 0))
  {
    write_to_output(d, "\r\n");
    write_to_output(d, "\tYThe introduction system is currently disabled on this MUD.\tn\r\n");
    write_to_output(d, "\tYYou must set up your character's short description before entering the game.\tn\r\n");
    write_to_output(d, "\r\n");
    show_short_description_main_menu(d);
    break;
  }

  load_result = enter_player_game(d);
  // ... rest of normal game entry code
```

## Purpose

### When Introduction System is Enabled
- Players use the introduction system to meet other characters
- Short descriptions are optional or secondary
- Players can enter the game immediately

### When Introduction System is Disabled
- Players are identified by their short descriptions
- Without a short description, they would appear as "someone" or with no proper description
- This becomes confusing and breaks immersion
- **Solution**: Require short description setup before first game entry

## Short Description System

### Components

**Descriptor 1** (`GET_PC_DESCRIPTOR_1`):
- Stored in: `ch->player_specials->saved.sdesc_descriptor_1`
- Examples: "complexion", "build", "height", etc.
- Must be > 0 to be considered set

**Adjective 1** (`GET_PC_ADJECTIVE_1`):
- Stored in: `ch->player_specials->saved.sdesc_adjective_1`
- Examples: "pale", "muscular", "tall", etc.
- Must be > 0 to be considered set

### Valid States

| Descriptor 1 | Adjective 1 | Status | Action |
|--------------|-------------|--------|--------|
| 0 | 0 | Not set | Require setup |
| 0 | > 0 | Partial | Require setup |
| > 0 | 0 | Partial | Require setup |
| > 0 | > 0 | Complete | Allow game entry |

## Player Experience

### Scenario 1: New Character, Intro System OFF

1. Player creates new character
2. Player reaches main menu
3. Player selects option 1 to enter game
4. System detects no short description set
5. Player sees:
   ```
   The introduction system is currently disabled on this MUD.
   You must set up your character's short description before entering the game.
   
   SET CHARACTER SHORT DESCRIPTION: PRESS ENTER
   ```
6. Player is redirected to `CON_GEN_DESCS_INTRO` state
7. Player completes short description setup
8. Player can now enter the game

### Scenario 2: Existing Character, Intro System OFF

1. Player with already-set short description
2. Player selects option 1 to enter game
3. System checks: descriptor and adjective are both > 0
4. Player enters game normally

### Scenario 3: Any Character, Intro System ON

1. Player with or without short description
2. Player selects option 1 to enter game
3. System check passes (intro system is enabled)
4. Player enters game normally

## Configuration

### Enable/Disable Introduction System

**Via CEDIT (in-game)**:
```
cedit
[Navigate to introduction system setting]
```

**Via config file**: `src/config.c`
```c
int use_introduction_system = YES;  // or NO
```

**Runtime check**: `CONFIG_USE_INTRO_SYSTEM` macro

## Related Functions

### `show_short_description_main_menu()`
**Location**: `src/interpreter.c` line ~4663
**Purpose**: 
- Checks if player has already set description
- If yes: Shows "already chosen" message
- If no: Initiates short description setup (CON_GEN_DESCS_INTRO)

### Description Setup States
1. `CON_GEN_DESCS_INTRO` - Introduction/instruction screen
2. `CON_GEN_DESCS_DESCRIPTORS_1` - Choose descriptor type
3. `CON_GEN_DESCS_ADJECTIVES_1` - Choose adjective for descriptor
4. `CON_GEN_DESCS_MENU` - Review and confirm choices

## Edge Cases

### Player Logs Out During Setup
- Short description values remain at 0
- Next login will trigger requirement again
- Player must complete setup to enter game

### Admin Resets Description
- Admin can reset values to 0 via appropriate command
- Player will be required to set description again on next login

### CONFIG Changes Mid-Game
- Affects only new login attempts
- Players already in-game are not affected
- Next login will check new CONFIG value

## Testing Scenarios

### Test 1: Fresh Character, Intro OFF
**Setup**: 
- New character creation
- CONFIG_USE_INTRO_SYSTEM = NO
- GET_PC_DESCRIPTOR_1 = 0
- GET_PC_ADJECTIVE_1 = 0

**Expected**: 
- Cannot enter game
- Redirected to short description setup
- Must complete setup to enter

### Test 2: Existing Character, Intro OFF
**Setup**:
- Character with description set
- CONFIG_USE_INTRO_SYSTEM = NO
- GET_PC_DESCRIPTOR_1 > 0
- GET_PC_ADJECTIVE_1 > 0

**Expected**:
- Enters game normally
- No interruption

### Test 3: Fresh Character, Intro ON
**Setup**:
- New character creation
- CONFIG_USE_INTRO_SYSTEM = YES
- GET_PC_DESCRIPTOR_1 = 0
- GET_PC_ADJECTIVE_1 = 0

**Expected**:
- Enters game normally
- No requirement for short description

### Test 4: Partial Description, Intro OFF
**Setup**:
- CONFIG_USE_INTRO_SYSTEM = NO
- GET_PC_DESCRIPTOR_1 > 0
- GET_PC_ADJECTIVE_1 = 0 (OR vice versa)

**Expected**:
- Cannot enter game
- Redirected to complete setup
- Both values must be > 0

## Files Modified

1. **src/interpreter.c** (line ~4295):
   - Added check in CON_MENU case '1'
   - Prevents game entry without short description when intro system is OFF

## Related Documentation

- Introduction System: `docs/systems/INTRODUCTION_SYSTEM.md` (if exists)
- Character Description System: `src/char_descs.c`
- Player Data: `src/players.c` (save/load for descriptor/adjective values)

## Benefits

1. **Prevents Confusion**: Players always have a proper description when intro system is off
2. **Improves Immersion**: No "someone" or blank descriptions in game
3. **Clear Communication**: Players understand why they can't enter and what to do
4. **Flexible**: Requirement only applies when intro system is disabled
5. **Consistent**: All players follow same rules based on configuration

## Future Enhancements

Possible improvements:

1. **Admin Override**: Allow staff to force-enter players without description
2. **Default Descriptions**: Provide generic default based on race/gender
3. **Skip Option**: Allow players to use basic description initially and set custom later
4. **Validation**: Ensure description choices make sense for character race/class
5. **Preview**: Show player what their short description will look like before confirming
