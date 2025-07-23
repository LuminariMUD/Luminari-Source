# Task List for LuminariMUD Code Fixes

## System Error Analysis - Multiple Recurring Issues

### Problem Description
The system logs show several recurring errors that indicate both code and database issues:

1. **Objects in NOWHERE executing scripts** - PARTIALLY FIXED
   - Dragon eggs (VNum 1015)
   - Webbed cocoons (VNum 1920)
   - Red liquid streams (VNum 193)
   - Error: "oecho called by object in NOWHERE"
   - **Status**: Room validity checks were added in dg_variables.c on 2025-07-23

2. **Script syntax errors** - NOT FIXED
   - Dragon egg timer script has malformed `remote` command
   - Error: "remote: invalid arguments 'remote egg_hatch_total_time '"
   - **Status**: Requires DG Script editing, not C code changes

3. **Mobs accessing player-only data** - FIXED
   - Multiple locations trying to use PRF_FLAGGED on NPCs
   - Error: "Mob using '((target)->player_specials->saved.pref)'"
   - **Status**: Fixed in utils.c and magic.c on 2025-07-23

4. **Database schema mismatches** - PARTIALLY FIXED
   - Missing table: `luminari_muddev.pet_save_objs` -- TABLE ADDED
   - Missing column: `character_info` in PLAYER_DATA table
   - **Status**: Compatibility code added in objsave.c on 2025-07-23

### Root Cause Analysis

#### 1. Objects in NOWHERE
- Objects without valid room locations are executing scripts
- **FIXED**: Room validity checks added to dg_variables.c
- **FIXED**: VALID_ROOM_RNUM macro corrected in utils.h

#### 2. Script Syntax Error (VNum 1015)
- The `remote` command is missing required parameters
- Syntax should be: `remote <target> <variable> <value>`
- Current script only has: `remote egg_hatch_total_time` (missing target and value)
- **NOT FIXED**: Requires editing the DG Script itself

#### 3. PRF_FLAGGED on Mobs
- **utils.c:10447**: `PRF_FLAGGED(target, PRF_NON_ROLEPLAYER)` doesn't check if target is NPC
- **magic.c:9203**: `PRF_FLAGGED(ch, PRF_CONTAIN_AOE)` doesn't check if ch is NPC
- PRF_FLAGGED macro accesses player_specials which NPCs don't have
- **FIXED**: NPC checks added on 2025-07-23

#### 4. Database Schema
- Development server is missing tables/columns that code expects
- **PARTIALLY FIXED**: Compatibility code added for some tables

### Proposed Fixes

#### Fix 1: Objects in NOWHERE - COMPLETED
Room validity checks have been implemented.

#### Fix 2: Dragon Egg Script (VNum 1015) - STILL NEEDED
Fix the `remote` command in the DG Script:
```
* Current (broken):
remote egg_hatch_total_time

* Fixed (example):
remote %self% egg_hatch_total_time 100
```

#### Fix 3: PRF_FLAGGED on Mobs - COMPLETED

**utils.c:10447**: Added NPC check before PRF_FLAGGED
**magic.c:9203**: Fixed syntax error and added NPC check

Both fixes have been applied on 2025-07-23.

#### Fix 4: Database Schema - PARTIALLY COMPLETED
Compatibility code has been added for some missing columns.

### Testing Requirements

1. **Object Scripts:**
   - Verify objects in NOWHERE don't crash when executing scripts
   - Test that valid objects still execute scripts normally

2. **DG Scripts:**
   - Fix and test the dragon egg timer script
   - Verify `remote` command works with proper syntax

3. **PRF_FLAGGED:**
   - Test combat with NPCs to ensure no crashes
   - Verify player preferences still work correctly

4. **Database:**
   - Test with both complete and incomplete schemas
   - Verify graceful degradation without crashes

### Priority
High - These errors are occurring frequently and could cause server instability or crashes

### Implementation Notes
- The "object in NOWHERE" fixes have been applied and verified
- PRF_FLAGGED usage has been fixed in utils.c and magic.c (2025-07-23)
- DG Script syntax errors require builder-level access to fix (see DG_SCRIPT_FIXES_NEEDED.md)
- Database compatibility is partially implemented but may need expansion

### Related Files
- `dg_variables.c` - Script variable handling (FIXED)
- `utils.c` - PRF_FLAGGED usage at line 10447 (FIXED)
- `magic.c` - PRF_FLAGGED usage at line 9203 (FIXED)
- `objsave.c` - Database compatibility code (PARTIALLY FIXED)
- DG Script files for VNums 1015, 1920, 193 (NEED BUILDER ACCESS)
- `DG_SCRIPT_FIXES_NEEDED.md` - Document listing required DG Script fixes