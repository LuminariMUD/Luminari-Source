# Mob Spell Slot System Design Document

## Overview
Implementation of a spell slot system for NPCs that limits their spell casting instead of allowing infinite spell use. Mobs will have spell slots per circle (0-9) and regenerate one random slot per minute while out of combat.

## Core Mechanics

### Spell Slot Structure
- Mobs have **spell slots per spell circle** (0-9, total of 10 circles)
- Number of slots based on mob level/class (similar to player casters)
- Casting a spell consumes one slot from the appropriate circle
- If no slots available for a circle, cannot cast spells of that circle

### Slot Regeneration
- **Out of Combat Only**: Regeneration only occurs when mob is NOT fighting
- **Random Restoration**: Every 120 seconds (2 minutes), restore ONE random spell slot
- **Selection Process**: 
  1. Build list of all circles with less than maximum slots
  2. Randomly select one circle from this list
  3. Restore one slot to that circle
- **Maximum**: Cannot exceed maximum slots for that circle

### Spell Slot Calculation
Based on mob level and class, calculated similarly to players:
- **Clerics/Druids/Wizards/Sorcerers**: Full caster progression
- **Paladins/Rangers/Blackguards**: Half caster progression  
- **Bards/Warlocks**: 3/4 caster progression
- Use existing `compute_slots_by_circle()` function where possible

## Implementation Details

### Data Structure Changes

#### char_data structure (structs.h)
```c
// In mob_special_data or char_special_data
struct {
    int spell_slots[10];      // Current spell slots per circle (0-9)
    int max_spell_slots[10];  // Maximum spell slots per circle (0-9)
    time_t last_slot_regen;   // Timestamp of last regeneration
} mob_spell_data;
```

### Key Functions

#### Initialization
```c
void init_mob_spell_slots(struct char_data *ch)
```
- Called when mob is loaded/reset
- Calculate max spell slots based on class and level
- Set current slots equal to maximum (fully rested)
- Initialize regeneration timestamp

#### Slot Checking
```c
bool has_spell_slot(struct char_data *ch, int spellnum)
```
- Determine spell circle from spellnum
- Check if mob has available slot for that circle
- Return TRUE if slot available, FALSE otherwise

#### Slot Consumption
```c
void consume_spell_slot(struct char_data *ch, int spellnum)
```
- Determine spell circle from spellnum
- Decrement spell slot for that circle
- Called after successful spell cast

#### Slot Regeneration
```c
void regenerate_mob_spell_slot(struct char_data *ch)
```
- Check if mob is in combat (return if TRUE)
- Check if 60 seconds have elapsed since last regeneration
- Build list of circles with less than max slots
- Randomly select one circle and restore one slot
- Update regeneration timestamp

### Integration Points

#### Mob Spellcasting (mob_spells.c)
- Modify spell selection to check `has_spell_slot()` before attempting cast
- Call `consume_spell_slot()` after successful cast
- Fallback to lower circle spells if higher circles depleted

#### Mobile Activity Loop (mob_act.c)
- Add periodic call to `regenerate_mob_spell_slot()` 
- Likely in `mobile_activity()` or similar function
- Check on interval (every game pulse or minute)

#### Mob Reset/Loading
- Call `init_mob_spell_slots()` when mob is created
- Ensure slots are fully restored on mob reset/repop

#### Combat Status
- Track when mob enters/exits combat
- Prevent regeneration during combat
- Resume regeneration when combat ends

## Default Behavior

**IMPORTANT**: Spell slots are **ENABLED BY DEFAULT** for all mobs.

To give a mob unlimited spell slots (old behavior):
1. Edit the mob with `medit <vnum>`
2. Add the `MOB_UNLIMITED_SPELL_SLOTS` flag
3. Save the mob

This makes the system opt-out rather than opt-in, encouraging tactical spell usage across the game world.

## Configuration

### Slot Calculations
Use similar formulas to player spell slot calculations:
- Circle 0 (cantrips): Unlimited or large number
- Circles 1-9: Based on caster level and class

Example for Wizard (level 20):
- Circle 1: 4 slots
- Circle 2: 4 slots
- Circle 3: 4 slots
- Circle 4: 4 slots
- Circle 5: 4 slots
- Circle 6: 3 slots
- Circle 7: 3 slots
- Circle 8: 2 slots
- Circle 9: 2 slots

### Special Cases
- **Epic Spells**: May use special handling (daily uses, not slots)
- **At-Will Abilities**: Not affected by spell slots
- **Unlimited Slots**: Mobs with MOB_UNLIMITED_SPELL_SLOTS flag bypass the system entirely

## Display/Debugging

### Admin Commands
```
mstat <mob> - Show current/max spell slots per circle
```

### Logging
Log when:
- Mob depletes a spell circle completely
- Mob regenerates a spell slot
- Mob attempts to cast without available slot

## Benefits

### Gameplay
- **Tactical Depth**: Players can deplete mob spell resources
- **Extended Fights**: Powerful casters can't spam high-level spells indefinitely
- **Resource Management**: Encourages mob AI to use spells strategically
- **Realism**: More consistent with player spell casting limitations

### Balance
- **Prevents Spam**: Limits repetitive spell use
- **Scaling Difficulty**: Tougher mobs have more slots
- **Recovery Time**: Out-of-combat regeneration prevents immediate re-engagement

## Potential Future Enhancements

### Phase 2 Features
1. **Mob Metamagic**: Allow mobs to use metamagic (consuming higher slots)
2. **Spell Prep System**: Some mob casters "prepare" specific spells
3. **Slot Trading**: Sacrifice higher slots for multiple lower slots
4. **Fast Regeneration**: Special mob types regenerate faster
5. **Aura Effects**: Environmental effects that enhance/reduce regeneration

### AI Improvements
1. **Spell Selection**: Prefer lower-circle spells when high circles are depleted
2. **Retreat Behavior**: Flee to regenerate when spell slots low
3. **Group Tactics**: Mob casters coordinate spell usage
4. **Adaptive Casting**: Switch spell types based on available slots

## Implementation Checklist

- [ ] Add spell slot data structures to char_data
- [ ] Implement `init_mob_spell_slots()`
- [ ] Implement `has_spell_slot()`
- [ ] Implement `consume_spell_slot()`
- [ ] Implement `regenerate_mob_spell_slot()`
- [ ] Integrate slot checking into mob spell casting
- [ ] Add regeneration to mobile activity loop
- [ ] Update mob reset/loading to initialize slots
- [ ] Add combat status tracking for regeneration
- [ ] Implement admin display commands
- [ ] Add logging for debugging
- [ ] Test with various mob types and levels
- [ ] Balance slot regeneration rate
- [ ] Document system for builders

## Notes
- System is **enabled by default** for all mobs
- Mobs with **MOB_UNLIMITED_SPELL_SLOTS** flag bypass the system (unlimited casting)
- This encourages tactical gameplay and resource management
- Opt-out design means spell slots become the standard for the game world

---

*Document Version: 1.0*
*Created: October 19, 2025*
*Status: Design Phase - Ready for Implementation*
