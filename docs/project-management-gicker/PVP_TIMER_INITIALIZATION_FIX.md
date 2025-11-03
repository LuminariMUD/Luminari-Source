# PvP Timer Initialization Fix

## Issue
The PvP timer was not being properly initialized when loading characters, which could cause issues with saving/loading the timer value.

## Root Cause
While the save and load code for the PvP timer was correctly implemented:
- **Load**: `GET_PVP_TIMER(ch) = atol(line);` (line 816)
- **Save**: `BUFFER_WRITE("PvPT : %ld\n", (long)GET_PVP_TIMER(ch));` (line 2156)

The timer was not being explicitly initialized to 0 in the `load_char()` function's initialization section. This could potentially cause undefined behavior if the memory wasn't zeroed properly.

## Solution
Added explicit initialization of the PvP timer in the `load_char()` function in `src/players.c` at line 483:

```c
GET_PVP_TIMER(ch) = 0;
```

This initialization is placed alongside other similar cooldown timers:
- GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) = 0;
- GET_FORAGE_COOLDOWN(ch) = 0;
- GET_RETAINER_COOLDOWN(ch) = 0;
- GET_SCROUNGE_COOLDOWN(ch) = 0;
- GET_PVP_TIMER(ch) = 0; (new)

## Files Modified
- **src/players.c** (line 483): Added `GET_PVP_TIMER(ch) = 0;` initialization

## How It Works

### On Character Load
1. Character structure is initialized with default values
2. `GET_PVP_TIMER(ch) = 0;` sets the timer to 0
3. If a "PvPT" tag exists in the player file, it's loaded and overwrites the 0
4. If no "PvPT" tag exists (first time, or never enabled PvP), timer remains 0

### On Character Save
1. Checks if `GET_PVP_TIMER(ch) != 0`
2. If non-zero, writes `PvPT : <timestamp>` to the player file
3. If zero (PvP never enabled or cooldown expired), doesn't write the tag

### PvP Timer Lifecycle
1. **Initial state**: timer = 0 (PvP never enabled)
2. **Enable PvP**: timer = current_time (timestamp recorded)
3. **Save character**: if timer != 0, saves to pfile
4. **Load character**: loads timer from pfile (or 0 if not present)
5. **After 15 minutes**: cooldown check passes, can disable PvP
6. **Disable PvP**: timer remains set (for future reference)

## Testing Recommendations
1. Create a new character - timer should be 0
2. Enable PvP with `pvp` command
3. Check `cooldowns` - should show PvP cooldown
4. Save character (save command or quit)
5. Check player file in lib/plrfiles/ - should have "PvPT : <number>"
6. Log back in
7. Check `cooldowns` again - cooldown should persist
8. Wait 15+ minutes, disable PvP
9. Save and reload - timer should still be saved
10. Test with character that never enabled PvP - no "PvPT" tag in file

## Related Files
- `docs/PVP_FLAG_IMPLEMENTATION.md` - Full PvP system documentation
- `docs/PVP_COOLDOWN_DISPLAY.md` - Cooldown display documentation
- `src/act.other.c` - PvP toggle command implementation
- `src/act.informative.c` - Cooldown display implementation
