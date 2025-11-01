# AoE Effect System Documentation

## Overview

The `aoe_effect()` function provides a centralized way to handle Area of Effect (AoE) abilities that affect everyone in a room. This system reduces code duplication and provides a single location for maintaining AoE logic.

## Location

- **Implementation**: `src/magic.c`
- **Declaration**: `src/spells.h`

## Function Signature

```c
int aoe_effect(struct char_data *ch, int spellnum,
               int (*callback)(struct char_data *ch, struct char_data *tch, void *data),
               void *callback_data)
```

### Parameters

- `ch` - The character using the AoE effect
- `spellnum` - The spell/skill number used for `aoeOK()` checking
- `callback` - Function pointer that applies the effect to each valid target
- `callback_data` - Optional pointer to pass additional data to the callback

### Returns

- Number of targets successfully affected

## How It Works

1. Iterates through all characters in the caster's room
2. For each character, calls `aoeOK(ch, tch, spellnum)` to check if they're a valid target
3. If valid, calls the callback function to apply the effect
4. Counts and returns the number of successfully affected targets

## Callback Function Pattern

Callbacks must follow this signature:

```c
int callback_function(struct char_data *ch, struct char_data *tch, void *data)
```

- `ch` - The caster/attacker
- `tch` - The current target being processed
- `data` - Custom data passed from aoe_effect
- **Return**: 1 if target was affected, 0 if not

## Usage Examples

### Example 1: Simple Damage AoE

```c
/* Callback for tailspikes AoE damage */
static int tailspikes_damage_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  damage(ch, tch, dice(3, 6) + 10, SPELL_GENERIC_AOE, DAM_PUNCTURE, FALSE);
  return 1;
}

ACMD(do_tailspikes)
{
  /* ... prerequisite checks ... */
  
  act("You lift your tail and send a spray of tail spikes to all your foes.", 
      FALSE, ch, 0, 0, TO_CHAR);
  act("$n lifts $s tail and sends a spray of tail spikes to all $s foes.", 
      FALSE, ch, 0, 0, TO_ROOM);

  aoe_effect(ch, SPELL_GENERIC_AOE, tailspikes_damage_callback, NULL);
  
  USE_SWIFT_ACTION(ch);
}
```

### Example 2: AoE with Custom Data

```c
/* Data structure for dragonborn breath weapon callback */
struct dragonborn_breath_data {
  int level;
  int dam_type;
};

/* Callback for dragonborn breath weapon AoE damage */
static int dragonborn_breath_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct dragonborn_breath_data *breath_data = (struct dragonborn_breath_data *)data;
  
  if (breath_data->level <= 15)
    damage(ch, tch, dice(breath_data->level, 6), SPELL_DRAGONBORN_ANCESTRY_BREATH, 
           breath_data->dam_type, FALSE);
  else
    damage(ch, tch, dice(breath_data->level, 14), SPELL_DRAGONBORN_ANCESTRY_BREATH, 
           breath_data->dam_type, FALSE);
  
  return 1;
}

ACMD(do_dragonborn_breath_weapon)
{
  struct dragonborn_breath_data breath_data;
  
  /* ... prerequisite checks ... */

  breath_data.dam_type = draconic_heritage_energy_types[GET_DRAGONBORN_ANCESTRY(ch)];
  breath_data.level = GET_LEVEL(ch);

  aoe_effect(ch, SPELL_DRAGONBORN_ANCESTRY_BREATH, dragonborn_breath_callback, &breath_data);
  
  /* ... cleanup ... */
}
```

### Example 3: AoE with Status Effects

```c
/* Callback for dragonfear AoE effect */
static int dragonfear_callback(struct char_data *ch, struct char_data *tch, void *data)
{
  struct affected_type af;
  int *cast_level = (int *)data;
  
  /* Immunity checks */
  if (is_immune_fear(ch, tch, TRUE))
    return 0;
  if (is_immune_mind_affecting(ch, tch, TRUE))
    return 0;
  if (mag_resistance(ch, tch, 0))
    return 0;
  if (savingthrow(ch, tch, SAVING_WILL, affected_by_aura_of_cowardice(tch) ? -4 : 0, 
                  CAST_INNATE, *cast_level, ENCHANTMENT))
    return 0;

  /* Apply effect */
  act("You have been shaken by $n's might.", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been shaken by $n's might.", FALSE, ch, 0, tch, TO_ROOM);
  new_affect(&af);
  af.spell = SPELL_DRAGONFEAR;
  af.duration = dice(5, 6);
  SET_BIT_AR(af.bitvector, AFF_SHAKEN);
  affect_join(tch, &af, FALSE, FALSE, FALSE, FALSE);

  /* Additional effects */
  send_to_char(tch, "You PANIC!\r\n");
  perform_flee(tch);
  perform_flee(tch);

  return 1;
}

int perform_dragonfear(struct char_data *ch)
{
  int cast_level;

  if (!ch)
    return 0;

  act("You raise your head and let out a bone chilling roar.", FALSE, ch, 0, 0, TO_CHAR);
  act("$n raises $s head and lets out a bone chilling roar", FALSE, ch, 0, 0, TO_ROOM);

  cast_level = CLASS_LEVEL(ch, CLASS_DRUID) + GET_SHIFTER_ABILITY_CAST_LEVEL(ch);

  return aoe_effect(ch, SPELL_DRAGONFEAR, dragonfear_callback, &cast_level);
}
```

### Example 4: Complex AoE with Multiple Effects

```c
/* Data structure for Fist of Unbroken Air callback */
struct fist_air_data {
  int perk_rank;
  int monk_level;
};

/* Callback for Fist of Unbroken Air AoE */
int fist_air_callback(struct char_data *ch, struct char_data *tch, void *data) {
  struct fist_air_data *fist_data = (struct fist_air_data *)data;
  int force_dam = dice(2, 6) + 2; /* Base: 2d6+2 */
  force_dam *= fist_data->perk_rank; /* Multiply by perk rank */
  int actual_force_dam;

  /* Condensed mode check */
  if (!IS_NPC(tch) && PRF_FLAGGED(tch, PRF_CONDENSED))
  {
  }
  else
  {
    send_to_char(tch, "[\tCFIST-OF-UNBROKEN-AIR\tn] ");
  }

  act("\tCThe wave of force slams into $N!\tn", FALSE, ch, 0, tch, TO_NOTVICT);

  /* Apply force damage with proper resistance/absorption checking */
  actual_force_dam = damage(ch, tch, force_dam, SKILL_FIST_OF_UNBROKEN_AIR, DAM_AIR, FALSE);

  /* Only apply prone effect if damage was dealt */
  if (actual_force_dam > 0 && tch)
  {
    /* Reflex save to avoid being knocked prone */
    if (!savingthrow(ch, tch, SAVING_REFL, 0, CAST_INNATE, fist_data->monk_level / 2, NOSCHOOL))
    {
      /* Failed save - knock them down */
      if (GET_POS(tch) > POS_SITTING)
      {
        send_to_char(tch, "\tCThe force knocks you off your feet!\tn\r\n");
        act("\tC$N is knocked to the ground by the force!\tn", FALSE, ch, 0, tch, TO_NOTVICT);
        change_position(tch, POS_SITTING);
      }
    }
    else
    {
      send_to_char(tch, "You maintain your balance against the forceful blast!\r\n");
    }
  }
  
  return 1;
}
```

## Refactored Abilities

The following abilities have been refactored to use `aoe_effect()`:

### act.offensive.c

1. **Arrow Swarm** (`do_arrowswarm`)
   - Ranged AoE attack with ammo checks

2. **Frightful Presence** (`do_frightful`)
   - Complex fear AoE with group aura modifiers and saves

3. **Tailspikes** (`do_tailspikes`)
   - Simple damage AoE with no custom data

4. **Dragon Fear** (`perform_dragonfear`)
   - Status effect AoE with saves, immunities, and flee effect

5. **Fear Aura** (`perform_fear_aura`)
   - Status effect AoE similar to dragonfear

6. **Dragon Breath** (`do_breathe`)
   - Complex damage AoE with iron golem immunity check

7. **Dragonborn Breath Weapon** (`do_dragonborn_breath_weapon`)
   - Damage AoE with custom level and damage type

8. **Sorcerer Draconic Breath** (`do_sorcerer_breath_weapon`)
   - Draconic bloodline breath weapon with damage scaling

9. **Tailsweep** (`perform_tailsweep`)
   - Complex knockdown AoE with stability boots check and hit chance

10. **Whirlwind Attack** (`do_whirlwind`)
    - Melee AoE with limited attack count

11. **Hit All** (`do_hitall`)
    - NPC melee AoE with lag calculation

12. **Lich Fear** (`perform_lich_fear`)
    - Racial lich fear ability with status effect

### fight.c

13. **Fist of Unbroken Air** (monk ability in `hit()`)
    - Complex damage AoE with knockdown effect and saves

### spells.c

14. **Mass Domination** (`spell_mass_domination`)
    - Enchantment AoE that charms multiple targets

## Benefits

1. **Reduced Code Duplication**: The `for` loop and `aoeOK()` check are centralized
2. **Consistent Behavior**: All AoE effects use the same targeting logic
3. **Easier Maintenance**: Changes to AoE mechanics only need to be made in one place
4. **Cleaner Code**: Separates targeting logic from effect logic
5. **Flexible**: The callback pattern allows for any type of effect

## Migration Guide

To migrate existing AoE code to use `aoe_effect()`:

1. **Identify the AoE loop pattern**:
   ```c
   for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
   {
     next_vict = vict->next_in_room;
     if (aoeOK(ch, vict, SPELL_NUM))
     {
       // effect code here
     }
   }
   ```

2. **Extract the effect code** into a callback function

3. **Identify any data** needed by the callback and create a data structure if needed

4. **Replace the loop** with a call to `aoe_effect()`

5. **Test thoroughly** to ensure behavior is unchanged

## Future Enhancements

Potential improvements to the AoE system:

- Add support for AoE range/distance checking
- Add support for line-of-sight checking
- Add support for friendly-fire toggles
- Add support for shape-based AoE (cone, line, etc.)
- Add performance metrics for AoE effect tracking

## Notes

- The `aoeOK()` function handles all the complex logic for determining valid targets
- Callbacks should return 1 if the target was affected, 0 if not
- The callback pattern allows for early returns (e.g., immunity checks)
- Always use `next_tch = tch->next_in_room` pattern when the callback might kill/remove targets
