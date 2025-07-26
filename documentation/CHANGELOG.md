# CHANGELOG

## 2025-07-26

### Minor Fixes

#### Fixed Shutdown Warning - "Attempting to merge iterator to empty list"
- **Issue**: Warning during shutdown when groups are disbanded
- **Root Cause**: 
  - In leave_group(), after removing a character from the group, the code checks if group->members->iSize > 0
  - However, there appears to be a mismatch where iSize indicates members exist but the list is actually empty
  - This causes merge_iterator() to warn about attempting to iterate over an empty list
- **Solution**: 
  - Modified leave_group() to check if merge_iterator() actually returns a valid item before iterating
  - This prevents trying to iterate when the list is unexpectedly empty despite iSize > 0
- **Files Modified**: 
  - handler.c:3151-3159 (added check for valid iterator result in leave_group)
- **Impact**: Eliminates warning during shutdown while maintaining proper error detection for actual issues

### Critical Bug Fix Attempts

#### Fifth Fix Attempt for Player Death Crash - Prevent update_pos() on Dead Victims
- **Issue**: Server crash with malloc_consolidate error when player dies during combat
- **Theory Root Cause**: 
  - When a player dies, they are set to POS_DEAD, given 1 HP, and moved to respawn room
  - The attacker's combat event continues and calls `valid_fight_cond()` to validate the target
  - `valid_fight_cond()` calls `update_pos(FIGHTING(ch))` on the dead victim
  - Since victim has 1 HP, `update_pos()` changes their position from POS_DEAD to POS_RESTING
  - Combat continues against a player who should be dead, causing memory corruption
  - The corruption manifests as a crash in `save_char()` when it tries to allocate memory
- **Attempted Solution**: 
  - Added check in `valid_fight_cond()` to skip `update_pos()` if target is already POS_DEAD
  - This prevents dead characters from being changed to POS_RESTING during combat
  - Combat properly stops when it sees POS_DEAD instead of incorrectly updated position
- **Files Modified**: 
  - fight.c:11564-11565 (skip update_pos if target is POS_DEAD in non-strict mode)
  - fight.c:11574-11575 (skip update_pos if target is POS_DEAD in strict mode)
- **Hopeful Impact**: Prevents combat from continuing against dead players, eliminating the memory corruption
