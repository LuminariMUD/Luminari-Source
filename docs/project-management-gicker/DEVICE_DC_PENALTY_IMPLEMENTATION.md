# Device DC Penalty System Implementation

## Overview
This document describes the implementation of DC penalty accumulation for weird science devices when used beyond their charge limits.

## Date
Implementation completed: Current session

## Changes Made

### 1. Structure Modification (`src/structs.h`)

Added new field to `player_invention` structure:
```c
struct player_invention {
    char keywords[MAX_INVENTION_KEYWORDS];
    char short_description[MAX_INVENTION_SHORTDESC];
    char long_description[MAX_INVENTION_LONGDESC];
    int spell_effects[MAX_INVENTION_SPELLS];
    int num_spells;
    int duration;
    int reliability;
    int uses;                 /* Number of times this device has been used */
    time_t cooldown_expires;  /* Individual device cooldown timestamp */
    int dc_penalty;           /* +2 DC penalty per failed out-of-charges attempt */
};
```

**Purpose**: Track cumulative DC penalty for each device independently.

### 2. Device Use Logic (`src/act.other.c`)

#### 2.1 DC Calculation Update
Modified the Use Magic Device DC calculation to include the penalty:
```c
int dc = 20 + (times_used - max_uses) * device_spell_count - reliability_bonus + inv->dc_penalty;
```

**Previous**: DC did not include cumulative penalties
**Current**: DC increases by accumulated penalty amount

#### 2.2 Penalty Accumulation on Failure
When a Use Magic Device check fails (roll < DC):
```c
/* Increase DC penalty by 2 for each failed attempt */
inv->dc_penalty += 2;
send_to_char(ch, "The device is becoming increasingly unstable! (DC penalty: +%d)\r\n", inv->dc_penalty);
```

**Behavior**: Every failed attempt when out of charges adds +2 to DC permanently.

#### 2.3 Penalty Accumulation on Success
When a Use Magic Device check succeeds (roll >= DC but device out of charges):
```c
/* Success! But still increase DC penalty since device is out of charges */
inv->dc_penalty += 2;
if (inv->dc_penalty > 2) {
    send_to_char(ch, "You manage to coax the depleted device to work, but it's getting harder... (DC penalty: +%d)\r\n", inv->dc_penalty);
}
```

**Behavior**: Even successful uses when out of charges increase DC by +2, making future attempts progressively harder.

#### 2.4 Standard Action Consumption
Added USE_STANDARD_ACTION call after successful device activation:
```c
/* Consume a standard action for using the device */
USE_STANDARD_ACTION(ch);
```

**Previous**: Action consumption was handled by interpreter command definition but not explicitly enforced in code
**Current**: Explicitly consumes a standard action when device is successfully used

### 3. Device List Display (`src/act.other.c`)

Updated the list command to show DC penalty:

**Header:**
```
Num  Device Name                         Spells                                             Uses     DC Penalty  Status
---  ----------------------------------  --------------------------------------------------  -------  ----------  ------
```

**Row Format:**
```c
send_to_char(ch, "%-3d  %-34.34s  %-50.50s  %d/%-5d  +%-9d  %s\r\n",
             i+1,
             inv->short_description,
             spell_list,
             uses_remaining,
             max_uses,
             inv->dc_penalty,
             status);
```

**Example Output:**
```
1    a fireball/shield device            fireball, shield                                    0/2      +6          OK
```

### 4. Device Info Display (`src/act.other.c`)

Updated info command to show detailed DC penalty information:

```c
if (inv->uses >= max_uses) {
    int dc = 20 + (inv->uses - max_uses) * device_spell_count - reliability_bonus + inv->dc_penalty;
    send_to_char(ch, "  Use Magic Device DC: %d (when charges exhausted", dc);
    if (inv->dc_penalty > 0) {
        send_to_char(ch, ", includes +%d penalty from failed attempts", inv->dc_penalty);
    }
    send_to_char(ch, ")\r\n");
} else {
    send_to_char(ch, "  Use Magic Device DC: N/A (charges remaining)\r\n");
    if (inv->dc_penalty > 0) {
        send_to_char(ch, "  DC Penalty: +%d (from previous failed attempts when out of charges)\r\n", inv->dc_penalty);
    }
}
```

**Shows**: Current DC with penalty breakdown, making it clear how much harder the device has become.

### 5. Device Creation Initialization (`src/act.other.c`)

Added initialization in the creation event handler:
```c
inv->dc_penalty = 0;       /* Initialize DC penalty to 0 */
```

**Purpose**: Ensure new devices start with no penalty.

## Gameplay Impact

### Mechanics
1. **Normal Usage**: Devices work normally within their charge limits with no penalty accumulation
2. **Out of Charges**: When a device runs out of charges, every use attempt (success or failure) increases the DC by +2
3. **Escalating Difficulty**: Each attempt makes the next attempt harder, creating a risk/reward decision
4. **Permanent Penalty**: The penalty persists until the device is destroyed or breaks completely

### Example Scenario
- Device has 2 charges, all used
- First out-of-charge attempt: DC 20, fails → DC penalty now +2
- Second attempt: DC 22, succeeds but uses device → DC penalty now +4
- Third attempt: DC 24, fails → DC penalty now +6
- Fourth attempt: DC 26... and so on

### Strategic Considerations
- Players must decide when to push depleted devices vs. save them
- Devices become progressively unreliable when overused
- Risk of explosion (Brilliance and Blunder feat) increases with difficulty
- Encourages device diversity rather than over-relying on one device

## Technical Notes

### C90 Compliance
All code follows C90 standard with variable declarations at block start.

### Compilation
Successfully compiled with 0 errors, 0 warnings (aside from pre-existing warnings in other files).

### Save Compatibility
**Important**: The new `dc_penalty` field is added to the `player_invention` structure. This will require:
- Player file version handling if using binary saves
- Database schema update if using database storage
- Old characters will load with dc_penalty = 0 (safe default)

### Action System Integration
The USE_STANDARD_ACTION macro properly integrates with the existing action economy system. The device command already had ACTION_STANDARD flag in interpreter.c, but now explicitly consumes the action when successfully used.

## Testing Recommendations

1. **Basic Testing**:
   - Create a device
   - Use all normal charges
   - Attempt to use when depleted multiple times
   - Verify DC increases by 2 each time
   - Check device list shows penalty

2. **Edge Cases**:
   - Gnomish Tinkering feat (extra charge, -2 DC)
   - Brilliance and Blunder feat (4 spells, explosion on break)
   - Device breaking with penalty accumulated
   - Multiple devices with different penalties

3. **Display Testing**:
   - device list - verify penalty column
   - device info <num> - verify detailed DC info
   - In-game messages during use attempts

## Future Enhancements (Optional)

Potential improvements not in current implementation:
- Option to "repair" devices to reset DC penalty
- Penalty reduction over time (device "rests")
- Critical success on UMD check to avoid penalty
- Penalty cap to prevent infinite escalation

## Files Modified

1. `src/structs.h` - Added dc_penalty field to player_invention
2. `src/act.other.c` - All device command modifications

## Compile Command
```bash
cd /home/krynn/code && make
```

## Status
✅ Implementation Complete
✅ Compilation Successful
✅ Ready for Testing
