# Forced Short Description Setup Implementation

## Overview
This document describes the implementation of forced short description setup when the introduction system is enabled.

## Functionality
When the introduction system is **enabled** (`CONFIG_USE_INTRO_SYSTEM` is ON) and a player attempts to enter the game without having set their short descriptions, they will be forced to set them before entering the game.

## Implementation Details

### Files Modified

#### 1. `src/structs.h` (line ~5822)
Added a new field to `descriptor_data` struct:
```c
bool forced_short_desc_setup; /**< TRUE if forced to set short desc before game entry */
```

This flag tracks when a player has been forced into the short description setup process from the game entry menu, so we know to enter them into the game after completion rather than sending them to the RP menu.

#### 2. `src/interpreter.c` (lines 4295-4306)
Modified the `CON_MENU` state, case '1' (Enter Game):
```c
if (CONFIG_USE_INTRO_SYSTEM && 
    (!GET_PC_DESCRIPTOR_1(d->character) || !GET_PC_ADJECTIVE_1(d->character)))
{
    SEND_TO_Q("\r\n\tcYou must set your short description before entering the game"
              " while the introduction system is enabled.\r\n\tn", d);
    d->forced_short_desc_setup = TRUE;
    STATE(d) = CON_GEN_DESCS_INTRO;
    start_generic_descs(d, 1);
}
```

This checks if the intro system is ON and the player hasn't set their short description. If so, it:
1. Sends a message explaining the requirement
2. Sets the `forced_short_desc_setup` flag to TRUE
3. Redirects them to the short description setup state

#### 3. `src/char_descs.c` (lines 842-920)
Modified `HandleStateGenericDescsParseMenuChoice()` function:

**Case 0 (Cancel):**
- If `forced_short_desc_setup` is TRUE, prevents canceling and sends the player back to the setup menu
- Otherwise, allows normal cancellation

**Case 1 (Complete):**
- If `forced_short_desc_setup` is TRUE:
  - Clears the flag
  - Executes the complete game entry sequence:
    - `enter_player_game()`
    - Welcome message
    - Save character
    - Greet triggers
    - "has entered the game" message
    - Update last on time
    - Set state to `CON_PLAYING`
    - Handle new character setup if needed
    - Show room to player
    - Check for mail
  - Player enters the game immediately
- Otherwise, follows normal path to `CON_CHAR_RP_MENU`

## Flow Diagram

### Normal Flow (Intro System OFF or Short Desc Already Set)
```
CON_MENU (select 1) → enter_player_game() → CON_PLAYING
```

### Forced Setup Flow (Intro System ON, No Short Desc)
```
CON_MENU (select 1) 
  ↓
Check: intro system ON + no short desc
  ↓
Set forced_short_desc_setup = TRUE
  ↓
CON_GEN_DESCS_INTRO (forced setup)
  ↓
Player completes short description
  ↓
HandleStateGenericDescsParseMenuChoice case 1
  ↓
Check: forced_short_desc_setup == TRUE
  ↓
Clear flag + enter_player_game() → CON_PLAYING
```

### Voluntary Setup Flow (Player Goes to RP Menu)
```
CON_MENU (select other option)
  ↓
CON_CHAR_RP_MENU (select short desc option)
  ↓
CON_GEN_DESCS_INTRO (voluntary setup)
  ↓
Player completes short description
  ↓
HandleStateGenericDescsParseMenuChoice case 1
  ↓
Check: forced_short_desc_setup == FALSE
  ↓
show_character_rp_menu() → CON_CHAR_RP_MENU
```

## Testing Checklist

- [ ] Enable intro system (`CONFIG_USE_INTRO_SYSTEM` ON)
- [ ] Create new character without short description
- [ ] Try to enter game (select option 1)
- [ ] Verify forced into short description setup
- [ ] Try to cancel (option 0) - should be prevented
- [ ] Complete short description setup (option 1)
- [ ] Verify entered game directly without RP menu
- [ ] Verify flag is cleared after entry
- [ ] Test voluntary short desc setup (via RP menu)
- [ ] Verify voluntary setup goes back to RP menu
- [ ] Test with intro system OFF
- [ ] Verify can enter game without short desc when intro OFF

## Configuration
This feature is controlled by the `CONFIG_USE_INTRO_SYSTEM` config option. When this is:
- **ON**: Players must set short descriptions before entering the game
- **OFF**: Players can enter the game without short descriptions

## Related Files
- `src/structs.h` - Data structures
- `src/interpreter.c` - Command interpretation and game entry
- `src/char_descs.c` - Character description handling
- `src/interpreter.h` - Function declarations
