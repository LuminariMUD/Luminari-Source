# PvP Cooldown Display in Cooldowns Command

## Overview
Added the PvP flag cooldown timer to the `cooldowns` command output so players can see how much time remains before they can disable their PvP flag.

## Implementation Details

### File Modified
- **src/act.informative.c** - `perform_cooldowns()` function

### Changes Made
Added a check at the end of the cooldown list (line 2263-2270) that:
1. Checks if the character has PRF_PVP flag enabled
2. Checks if GET_PVP_TIMER is set (greater than 0)
3. Calculates the remaining time in the 15-minute cooldown period
4. Displays the remaining time in seconds if there's still time left

### Code Logic
```c
/* PvP cooldown timer - only shows if PvP is enabled and time remaining */
if (PRF_FLAGGED(k, PRF_PVP) && GET_PVP_TIMER(k) > 0)
{
  time_t current_time = time(0);
  time_t time_since_enabled = current_time - GET_PVP_TIMER(k);
  int seconds_remaining = (15 * 60) - time_since_enabled;

  if (seconds_remaining > 0)
    send_to_char(ch, "PvP Flag Cooldown - Duration: %d seconds\r\n", seconds_remaining);
}
```

### Display Format
When the cooldown is active, the player will see:
```
PvP Flag Cooldown - Duration: XXX seconds
```

### When It Shows
The PvP cooldown line only appears when:
- The player has their PvP flag enabled (PRF_PVP is set)
- A timer has been recorded (GET_PVP_TIMER > 0)
- There is still time remaining in the 15-minute window (seconds_remaining > 0)

### When It Doesn't Show
The line will NOT appear when:
- PvP flag is disabled
- PvP was never enabled (timer is 0)
- The 15-minute cooldown period has elapsed
- The cooldown command is used to check another player without PvP enabled

### Integration
- Placed after all other timer checks (forage, retainer, scrounge, etc.)
- Before the `list_item_activate_ability_cooldowns()` call
- Consistent formatting with other cooldown displays
- Shows time in seconds like other cooldowns

### Testing Recommendations
1. Enable PvP flag with `pvp` command
2. Immediately run `cooldowns` - should show full ~900 seconds
3. Wait some time and check again - should show decreasing seconds
4. After 15 minutes, run `cooldowns` - should not show PvP line
5. Verify `pvp` command can now disable the flag
6. Test with another character's cooldowns (group member) if they have PvP enabled

## Related Files
- See `docs/PVP_FLAG_IMPLEMENTATION.md` for full PvP system documentation
- The PvP timer is set in `src/act.other.c` (do_gen_tog function)
- Timer is saved/loaded in `src/players.c`
