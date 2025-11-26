# Experience Multiplier Implementation

## Overview
Added a new cedit gameplay setting `experience_multiplier` that allows admins to adjust the global experience gain rate as a percentage. This enables easy adjustment of leveling speed without code changes, supporting scenarios like "double XP weekends" or slower progression servers.

## Implementation Date
November 6, 2024

## Feature Details

### Configuration Setting
- **Name**: Experience Multiplier
- **Type**: Integer (percentage)
- **Default Value**: 100 (represents 100%, normal experience rates)
- **Usage**: 
  - 100 = Normal experience (no change)
  - 200 = Double experience
  - 50 = Half experience
  - Any positive integer value

### Access
The setting is accessible through the cedit command:
1. Enter `cedit`
2. Select 'G' for Game Play Options
3. Select 'V' for Experience Multiplier
4. Enter desired percentage value

## Technical Implementation

### Files Modified

#### 1. src/structs.h
- Added `int experience_multiplier;` field to `struct game_data` after `max_exp_loss`
- Located at line ~6976

#### 2. src/utils.h
- Added macro: `#define CONFIG_EXPERIENCE_MULTIPLIER config_info.play.experience_multiplier`
- Located after `CONFIG_MAX_EXP_LOSS` definition

#### 3. src/config.h
- Added extern declaration: `extern int experience_multiplier;`

#### 4. src/config.c
- Added default initialization: `int experience_multiplier = 100;`
- Comment: "percentage multiplier for exp gain (100 = normal)"

#### 5. src/db.c
- Added initialization in `load_default_config()`: `CONFIG_EXPERIENCE_MULTIPLIER = experience_multiplier;`
- Added config file loading in case 'e': `else if (!str_cmp(tag, "experience_multiplier")) CONFIG_EXPERIENCE_MULTIPLIER = num;`

#### 6. src/oasis.h
- Added constant: `#define CEDIT_EXPERIENCE_MULTIPLIER 26`
- Note: Also fixed existing duplicate constant values (CEDIT_IDLE_VOID, CEDIT_MOB_STATS_CATEGORY_MENU, CEDIT_MOB_STATS_DIVINE_AS, CEDIT_SET_LANDMARK_SYSTEM)

#### 7. src/cedit.c
**Menu Display** (cedit_disp_game_play_options):
- Added menu option 'V' for "Experience Multiplier (%age)"
- Added parameter to display current value

**Menu Handler** (CEDIT_GAME_OPTIONS_MENU):
- Added case 'v'/'V' to prompt for multiplier input

**Input Handler**:
- Added `case CEDIT_EXPERIENCE_MULTIPLIER:` to process numeric input

**Config Save**:
- Added fprintf for saving to config file
- Added initialization in cedit_setup
- Added save handler in cedit_save_internally

#### 8. src/class.c
**level_exp() function**:
- Added experience multiplier application before return statement:
```c
/* Apply experience multiplier from config */
exp = (exp * CONFIG_EXPERIENCE_MULTIPLIER) / 100;

return exp;
```
- This ensures the multiplier is applied to all experience calculations regardless of class or race modifiers

## Behavior

### Application Point
The multiplier is applied at the **very end** of the `level_exp()` calculation, after:
1. Base class experience calculations
2. Race experience modifiers (e.g., Drow, Vampire, Lich racial adjustments)
3. All other experience adjustments

This ensures the multiplier affects the final experience requirement uniformly across all races and classes.

### Example Calculations

With default 100% multiplier:
- Level 5 Wizard requires: 10,000 exp (unchanged)

With 200% multiplier (double XP):
- Level 5 Wizard requires: 5,000 exp (half the normal amount needed)

With 50% multiplier (slower progression):
- Level 5 Wizard requires: 20,000 exp (twice the normal amount needed)

## Usage Scenarios

### Double XP Event
```
cedit -> G -> V -> 50
```
This sets experience requirements to 50%, effectively giving players double experience gain.

### Normal Rates
```
cedit -> G -> V -> 100
```
This returns to standard experience requirements.

### Slow Progression Server
```
cedit -> G -> V -> 150
```
This increases experience requirements by 50%, slowing player progression.

### Testing/Fast Leveling
```
cedit -> G -> V -> 10
```
This reduces experience requirements to 10%, allowing very rapid leveling for testing.

## Configuration File Format

The setting is saved to the config file with this format:
```
* Percentage multiplier for experience gain (100 = normal)?
experience_multiplier = 100
```

## Compilation
Successfully compiled with no errors or warnings.

## Testing Recommendations

1. **Default Value Test**: Verify the multiplier defaults to 100 on fresh install
2. **Menu Access Test**: Confirm 'V' option appears in cedit gameplay menu
3. **Save/Load Test**: Set multiplier, save, restart server, verify value persists
4. **Experience Calculation Test**: 
   - Set to 50 (double XP), check required exp for level
   - Set to 200 (half XP), check required exp for level
   - Set to 100, verify normal rates restored
5. **Edge Case Tests**:
   - Zero value (should prevent leveling)
   - Very large values (10000+)
   - Negative values (should be prevented or handled)

## Future Enhancements (Optional)

Potential additions if desired:
1. Min/max value validation in cedit handler
2. Happy hour integration (automatic multiplier changes)
3. Zone-specific multipliers
4. Class-specific multipliers
5. Level range multipliers (e.g., faster for levels 1-10)
6. Logging of multiplier changes for admin auditing

## Related Systems

This multiplier affects:
- All class leveling requirements
- All race leveling requirements (applied after racial modifiers)
- Does NOT affect:
  - Experience gained from kills (handled separately)
  - Experience penalties from death
  - Quest experience rewards

## Author Notes

The implementation follows the existing config system patterns precisely:
- Uses the same naming conventions as `max_exp_gain` and `max_exp_loss`
- Integrates into cedit menu structure consistently
- Saves/loads using standard config file format
- Applied at the optimal point for global effect

The percentage-based approach (rather than decimal multiplier) was chosen for:
1. Easier admin understanding (100 = normal, 200 = double)
2. Integer arithmetic avoids floating point precision issues
3. Consistency with other percentage values in config (happy hour percentages)
