# Moon-Based Bonus Spell Slots System

## Overview
A new system that grants arcane casters (Wizard, Sorcerer, Bard) bonus spell slots based on the current moon phases from your existing moon system. These spell slots allow free casting of any known or prepared arcane spell without expending a normal spell slot.

## Key Features

### 1. **Moon Phase Integration**
- Uses existing `weather_info.moons` alignment-based values:
  - **Solinari** (Good moon): `weather_info.moons.solinari_sp`
  - **Lunitari** (Neutral moon): `weather_info.moons.lunitari_sp`
  - **Nuitari** (Evil moon): `weather_info.moons.nuitari_sp`

### 2. **Bonus Spell Mechanics**
- **Arcane Casters Only**: Wizard, Sorcerer, Bard, and related prestige classes
- **Automatic Bonus**: Amount based on character's alignment and current moon phase
- **No Spell Slot Consumption**: Using a moon bonus spell doesn't consume normal spell slots
- **Regeneration**: One bonus spell regenerates every **5 minutes** when used
- **Phase Changes**: When moon phases change, bonus spells automatically adjust (if the new amount is lower, used spells are capped to the new limit)

### 3. **Spell Casting Priority**
When casting an arcane spell, the system checks in this order:
1. Do they have an available moon bonus spell? Use it.
2. Otherwise, use normal spell slots (prepared or spontaneous)

## Implementation Details

### Files Added
1. **`src/moon_bonus_spells.h`** - Header file with function declarations
2. **`src/moon_bonus_spells.c`** - Implementation of all moon bonus spell functions

### Files Modified

#### 1. **`src/structs.h`** - Added variables to track moon bonus spells
```c
/* In player_special_data_saved struct */
int moon_bonus_spells;           /* Maximum moon bonus spells available */
int moon_bonus_spells_used;      /* Number of moon bonus spells used (in recovery) */
int moon_bonus_regen_timer;      /* Timer for next regeneration (in ticks) */
```

#### 2. **`src/limits.c`** - Added regeneration in game heartbeat
- Imported `moon_bonus_spells.h`
- Calls `regenerate_moon_bonus_spell(ch)` once per tick for each character
- Regenerates one spell every 5 minutes (3000 ticks)

#### 3. **`src/spell_prep.c`** - Integrated with spell casting system
- Imported `moon_bonus_spells.h`
- Modified `spell_prep_gen_extract()` function to:
  - Check if character has available moon bonus spells FIRST
  - If yes and spell is arcane, consume the bonus spell and allow casting without using normal slot
  - Otherwise, proceed with normal spell slot consumption

#### 4. **`src/weather.c`** - Updated moon phase handling
- Imported `moon_bonus_spells.h`
- Modified `calc_moon_bonus()` to:
  - Calculate new moon bonuses as before
  - Loop through all online players and call `update_moon_bonus_spells(ch)` for each
  - Automatically adjusts each player's bonus spells when phase changes

#### 5. **`src/db.c`** - Initialize on character creation/login
- Imported `moon_bonus_spells.h`
- Added `init_moon_bonus_spells(ch)` call in `init_char()` function
- Ensures new characters and logging-in characters have their moon bonuses initialized

## Function Reference

### Core Functions

#### `void init_moon_bonus_spells(struct char_data *ch)`
Initializes moon bonus spells when a character enters the game.
- Checks if character is an arcane caster
- Sets maximum bonus spells based on alignment and moon phase
- Resets usage counter

#### `void update_moon_bonus_spells(struct char_data *ch)`
Updates the maximum bonus spells when moon phase changes.
- Recalculates based on current moon phase
- If new max is lower than current usage, caps usage to new max
- Called automatically when moon phases change

#### `bool has_moon_bonus_spells(struct char_data *ch)`
Returns TRUE if the character has at least one available bonus spell.

#### `bool use_moon_bonus_spell(struct char_data *ch)`
Consumes a moon bonus spell.
- Returns TRUE if successful
- Increments the "used" counter
- Character must have at least one available

#### `void regenerate_moon_bonus_spell(struct char_data *ch)`
Called every game tick to regenerate one bonus spell per 5 minutes.
- Only regenerates if spells are currently used (in recovery)
- Decrements timer; when it reaches 0, decrements used counter and resets timer
- This is integrated into the main game loop in `point_update()`

#### `void reset_moon_bonus_spells(struct char_data *ch)`
Resets usage when a character levels up.
- Sets used counter to 0
- Resets regeneration timer

#### `int get_max_moon_bonus_spells(struct char_data *ch)`
Returns the maximum moon bonus spells available.

#### `int get_available_moon_bonus_spells(struct char_data *ch)`
Returns number of unused spells (max - used).

#### `int get_used_moon_bonus_spells(struct char_data *ch)`
Returns number of spells currently in recovery.

#### `bool is_arcane_caster(struct char_data *ch)`
Helper function to determine if character is an arcane caster.

## Usage Example

```c
// Check if a wizard has moon bonus spells
if (has_moon_bonus_spells(ch)) {
    // Try to cast a spell
    if (use_moon_bonus_spell(ch)) {
        send_to_char(ch, "You cast this spell using a moon bonus spell slot!\r\n");
    }
}
```

## Game Flow

### On Player Login
1. `init_char()` is called
2. `init_moon_bonus_spells(ch)` initializes the bonus spells based on current moon phase and alignment

### During Combat/Spell Casting
1. Player casts an arcane spell
2. `spell_prep_gen_extract()` checks for moon bonus spells FIRST
3. If available, `use_moon_bonus_spell(ch)` consumes one and spell casts for free
4. Otherwise, normal spell slot consumption happens

### Every Game Tick
1. `point_update()` runs
2. For each online character, `regenerate_moon_bonus_spell(ch)` is called
3. If timer expires, one bonus spell regenerates and timer resets (5 minute cycle)

### When Moon Phase Changes
1. `calc_moon_bonus()` is called
2. Moon phase bonuses are recalculated
3. `update_moon_bonus_spells(ch)` is called for each online player
4. Each player's bonus spell maximum is updated
5. If new maximum is lower, used spells are capped to the new limit

## Balance Considerations

### Advantages for Arcane Casters
- Free spell casts (equivalent to a 5th-level spell slot every 5 minutes)
- Scales with moon phases (better during favorable alignments)
- No stat dependency - purely based on moon phase

### Limitations
- Arcane casters only
- Regenerates slowly (1 spell per 5 minutes)
- Can only cast spells they know or have prepared
- Reduces when moon phase is unfavorable

### Integration with Existing Systems
- Works alongside normal spell slots
- Compatible with spell slot preservation perks
- Works with all spell preparation systems
- Arcane archer, eldritch knight, etc. all benefit if they have arcane levels

## Customization

To change regeneration rate, modify in `moon_bonus_spells.c`:
```c
#define MOON_BONUS_REGEN_TICKS (5 * 60 * 10)  /* Change this value */
```

Current calculation: 5 minutes × 60 ticks/minute × 10 (tick rate)

To adjust which classes get bonuses, modify `is_arcane_caster()` in `moon_bonus_spells.c`.

To change bonus amounts, modify the moon phase values in `weather.c`:
```c
weather_info.moons.solinari_sp = 1;  /* or 2, etc */
```

## Technical Notes

### Thread Safety
All functions are thread-safe as they operate on individual character data structures.

### Memory Usage
Minimal overhead - only 12 bytes per character (3 integers in saved data structure).

### Compilation
Add `moon_bonus_spells.c` to your makefile/build system.

### No Database Changes
All data is stored in existing `player_special_data_saved` structure. No database schema changes needed.

## Testing Checklist

- [ ] New characters initialize with correct bonus spells
- [ ] Logging in with moon bonuses works correctly  
- [ ] Casting an arcane spell uses moon bonus first if available
- [ ] Moon bonus spells regenerate at correct rate (1 per 5 minutes)
- [ ] Moon phase changes update all online players' bonuses
- [ ] Lowering moon phase caps the used spells correctly
- [ ] Non-arcane casters don't get moon bonuses
- [ ] Works with spell slot preservation perks
- [ ] Works with prepared and spontaneous casters
- [ ] Spells cast with moon bonuses don't affect normal spell slots
