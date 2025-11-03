# MOB_BUFF_OUTSIDE_COMBAT Flag Removal

## Change Summary
Removed the `MOB_BUFF_OUTSIDE_COMBAT` flag functionality while keeping the flag definition to maintain backward compatibility with existing mob files.

## Changes Made

### 1. Flag Marked as UNUSED (src/structs.h)

**Before**:
```c
#define MOB_BUFF_OUTSIDE_COMBAT 96
```

**After**:
```c
#define MOB_BUFF_OUTSIDE_COMBAT 96 /**< UNUSED - kept for backward compatibility */
```

**Purpose**: Documents that the flag is no longer used but preserves the flag number to avoid breaking existing zone files.

### 2. Flag Name Updated (src/constants.c)

**Before**:
```c
"Buff-Out-Of-Combat",
```

**After**:
```c
"UNUSED-96",
```

**Purpose**: Shows builders/admins that this flag is no longer functional when viewing mob flags.

### 3. Functionality Removed (src/mob_act.c)

**Before** (DL Campaign):
```c
#if defined(CAMPAIGN_DL)
      else if (!rand_number(0, 15) && MOB_FLAGGED(ch, MOB_BUFF_OUTSIDE_COMBAT) && IS_NPC_CASTER(ch))
      {
        /* Buffing only if flag set */
        wizard_cast_prebuff(ch);
      }
#else
      else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
      {
        /* Buffing always enabled */
        wizard_cast_prebuff(ch);
      }
#endif
```

**After** (Unified):
```c
      else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
      {
        /* Buffing always enabled for all casters */
        wizard_cast_prebuff(ch);
      }
```

**Purpose**: All spellcaster mobs now buff automatically without requiring a special flag.

## Impact

### Behavior Changes

**Before**:
- **DL Campaign**: Mobs needed `MOB_BUFF_OUTSIDE_COMBAT` flag to buff outside combat
- **Non-DL Campaign**: All caster mobs buffed automatically

**After**:
- **All Campaigns**: All spellcaster mobs buff automatically
- **DL Campaign**: Now matches non-DL behavior (no special flag needed)

### Mob Buffing Now Works For

✅ **ALL spellcaster mobs automatically** (no flag required):
- CLASS_WIZARD
- CLASS_SORCERER  
- CLASS_CLERIC
- CLASS_DRUID
- CLASS_BARD
- CLASS_RANGER
- CLASS_PALADIN
- CLASS_ALCHEMIST
- CLASS_INQUISITOR
- CLASS_SUMMONER
- All other classes in `IS_NPC_CASTER` macro

✅ **All psionic mobs automatically** (no flag required):
- Any mob with `IS_PSIONIC(ch)` true

### Backward Compatibility

✅ **Existing zone files are safe**:
- Mobs with the flag will load without errors
- Flag is silently ignored (has no effect)
- Shows as "UNUSED-96" in stat output
- Can be removed from mobs at leisure

⚠️ **Builders should update zones**:
- Remove `MOB_BUFF_OUTSIDE_COMBAT` flag from mobs
- Flag no longer serves any purpose
- Optional cleanup - not required

## Rationale

### Why Remove This Flag?

1. **Unnecessary Complexity**: The flag created confusion about why some mobs buff and others don't

2. **Campaign-Specific Behavior**: Only DL campaign used it, creating inconsistent behavior

3. **Better Default**: All spellcaster mobs should intelligently use their abilities

4. **Spell Slot System**: With proper spell slot management, mobs won't spam buffs irresponsibly

5. **AI Improvements**: Modern wizard AI with `has_sufficient_slots_for_buff()` provides built-in throttling

### Why Keep the Flag Definition?

1. **Zone File Compatibility**: Existing zone files may have mobs flagged with bit 96

2. **Graceful Migration**: Builders can remove flags gradually without breaking zones

3. **No Harm**: Unused flag definitions don't impact performance or functionality

4. **Clear Documentation**: Marking as UNUSED makes purpose clear

## Testing Performed

✅ **Compilation**: Clean compile with no errors or warnings
✅ **Flag Definition**: Kept at position 96 (no flag number changes)
✅ **Code Unification**: DL and non-DL campaigns now use same logic
✅ **Backward Compatibility**: Mobs with old flag will load normally

## Migration Guide for Builders

### Checking Existing Mobs

```
stat <mob>
```

Look in "NPC flags:" for "UNUSED-96". If present, the mob has the old flag.

### Removing the Flag

```
mset <mob> buff-outside-combat
```

or using the UNUSED name:

```
mset <mob> unused-96
```

**Note**: This is optional - the flag has no effect, so it's harmless to leave it.

### Zone File Updates

In zone files (.zon), look for mobs with action flag "V" (or the specific flag pattern).

**Before**:
```
M 0 12345 1 12345    * Mob #12345 (wizard)
E 1 67890 99 16      * Equipment
V 0 MOB 96           * OLD: MOB_BUFF_OUTSIDE_COMBAT flag
```

**After**:
```
M 0 12345 1 12345    * Mob #12345 (wizard)
E 1 67890 99 16      * Equipment
* Flag removed - buffing is now automatic
```

## Files Modified

1. **src/structs.h**
   - Added UNUSED comment to `MOB_BUFF_OUTSIDE_COMBAT` definition
   - No functional change - flag number preserved

2. **src/constants.c**
   - Changed flag name from "Buff-Out-Of-Combat" to "UNUSED-96"
   - Makes unused status visible in stat/mset commands

3. **src/mob_act.c**
   - Removed `MOB_FLAGGED(ch, MOB_BUFF_OUTSIDE_COMBAT)` checks
   - Removed `#if defined(CAMPAIGN_DL)` conditional compilation
   - Unified buffing behavior across all campaigns

## Related Systems

### Spell Slot Conservation
The removal is safe because the spell slot system provides natural throttling:
- `has_sufficient_slots_for_buff()` prevents excessive buffing
- Requires >50% spell slots remaining
- Prevents mobs from depleting resources on buffs

### Wizard AI
The wizard prebuff system is intelligent:
- Only casts long-duration buffs
- Skips already-active buffs
- Checks spell slot availability
- Uses appropriate group buffs when allies present

### Buffing Frequency
Unchanged - still 6.25% chance per mobile activity pulse (~4 seconds):
```c
else if (!rand_number(0, 15) && IS_NPC_CASTER(ch))
```

## Future Considerations

### Flag Number 96 Can Be Reused
In future major version updates, flag number 96 could be reassigned to a new mob flag. This would require:
1. Deprecation notice to builders
2. Zone file update tool
3. Major version increment
4. Clear migration path

### Alternative Approaches Considered

**Option A: Keep Flag for Fine-Grained Control**
- Rejected: Added complexity without clear benefit
- Spell slot system provides sufficient control

**Option B: Remove Flag Definition Entirely**
- Rejected: Would break existing zone files
- Current approach is safer

**Option C: Add New Buffing Control Flags**
- Future consideration: Could add flags like:
  - MOB_NO_PREBUFF (disable prebuffing)
  - MOB_BUFF_AGGRESSIVE (buff more frequently)
  - MOB_BUFF_CONSERVATIVE (buff less frequently)
- Not implemented now - current system sufficient

## Compilation Status
✅ **COMPLETE** - Compiled cleanly with no errors or warnings

## Backward Compatibility Status
✅ **MAINTAINED** - Existing mobs with flag will load normally

## Documentation Status
✅ **UPDATED** - WIZARD_MOB_BUFFING_TROUBLESHOOTING.md notes flag removal

## Summary

**What Changed**: Removed requirement for `MOB_BUFF_OUTSIDE_COMBAT` flag - all caster mobs now buff automatically

**Why**: Simplifies system, removes campaign-specific quirks, makes mob behavior more consistent

**Impact**: Positive - more intelligent mob behavior without builder micromanagement

**Compatibility**: Full backward compatibility - old flags ignored, no zone file changes required

**Result**: Cleaner code, better default behavior, easier to understand and maintain
