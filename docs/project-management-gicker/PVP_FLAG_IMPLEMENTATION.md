# PvP Flag Implementation

## Overview
Implemented a PRF_PVP preference flag that allows players to opt-in to player vs. player combat with a 15-minute cooldown timer before it can be disabled.

## Files Modified

### 1. src/structs.h
- **Line 1318**: PRF_PVP flag already defined as #49
- **Line 5326**: Added `time_t pvp_timer` field to `player_special_data_saved` struct to track when PvP was enabled

### 2. src/constants.c
- **Line 1623**: "PvP-Enabled" already present in `preference_bits` array at position 49

### 3. src/act.h
- **Line 786**: Added `#define SCMD_PVP 65` for the toggle command subcmd

### 4. src/utils.h
- **Line 2241**: Added `#define GET_PVP_TIMER(ch)` macro to access the pvp_timer field

### 5. src/act.other.c (do_gen_tog function)
- **Lines 7437-7438**: Added toggle message for PvP flag (message #65)
- **Lines 7447-7478**: Added `SCMD_PVP` case handler with:
  - Check if CONFIG_PK_ALLOWED is enabled
  - Timer validation when turning PvP OFF (15-minute restriction)
  - Sets timer timestamp when turning PvP ON
  - Displays remaining time if trying to disable too soon

### 6. src/interpreter.c
- **Line 715**: Added "pvp" command that calls do_gen_tog with SCMD_PVP

### 7. src/prefedit.c
- **Lines 328-336**: Added PvP toggle option ('K') to "More Preferences" menu
  - Only displays if CONFIG_PK_ALLOWED is enabled
  - Shows current ON/OFF status with color coding
- **Lines 1091-1124**: Added case handler for 'K'/'k' in extra toggle menu
  - Checks if PK is allowed
  - Validates 15-minute timer when disabling
  - Sets timer when enabling
  - Toggles the PRF_PVP flag

### 8. src/players.c
- **Line 816**: Added loading of "PvPT" tag for pvp_timer (uses atol for time_t)
- **Lines 2153-2154**: Added saving of pvp_timer if non-zero (saves as long integer)

## Features

### Command Usage
```
pvp          - Toggles PvP flag on/off
```

### Toggle Behavior
1. **Enabling PvP**:
   - Sets PRF_PVP flag
   - Records current timestamp in GET_PVP_TIMER(ch)
   - Message: "PvP flag enabled. You are now eligible for player vs. player combat."

2. **Disabling PvP**:
   - Checks if 15 minutes have elapsed since enabling
   - If not, displays: "You must wait X more minute(s) before you can disable your PvP flag."
   - If yes, clears PRF_PVP flag
   - Message: "PvP flag disabled. You are no longer eligible for player vs. player combat."

### Configuration Requirement
- The toggle only works if `CONFIG_PK_ALLOWED` is set to TRUE
- If PK is disabled globally, attempting to toggle shows: "Player killing is not enabled on this MUD."

### Prefedit Integration
- Available in prefedit under "More Preferences" menu as option 'K'
- Only visible if CONFIG_PK_ALLOWED is enabled
- Shows [ON] in green or [OFF] in red
- Same 15-minute restriction applies

### Persistence
- The pvp_timer is saved to the player file with tag "PvPT"
- Loaded on character login
- Timer persists across logins/logouts

## Timer Implementation
- Uses standard time_t (Unix timestamp)
- Calculation: `time_since_enabled = current_time - GET_PVP_TIMER(ch)`
- 15-minute restriction: `time_since_enabled < (15 * 60)` seconds
- Minutes remaining: `15 - (time_since_enabled / 60)`

## Testing Recommendations
1. Enable PvP and verify timer is set
2. Try to disable immediately - should fail with time remaining message
3. Wait 15+ minutes and verify it can be disabled
4. Check that timer persists through logout/login
5. Verify prefedit shows correct status
6. Test with CONFIG_PK_ALLOWED disabled - should reject toggle
7. Verify proper save/load from player file

## Usage Notes
- Players must wait 15 minutes after enabling PvP before they can disable it
- This prevents abuse of enabling PvP for combat then immediately disabling
- The flag is persistent and survives reboots/copyovers
- Staff should ensure CONFIG_PK_ALLOWED matches their desired PvP policy
