# Task List for LuminariMUD Code Fixes

## Issue #1: "feat info dragon mount" Command Not Working - **RESOLVED**

### Resolution Date: July 24, 2025

### Problem Description
The command `feat info dragon mount` returns "Could not find that feat" even though:
- The feat "dragon mount" exists and is visible in `feats all`
- Similar commands like `feat info dragon link` work correctly
- The feat is properly defined in the code as FEAT_DRAGON_BOND with name "dragon mount"

### Root Cause (Identified)
The actual problem was with the search order in `find_feat_num`:
- The function checked for abbreviations before exact matches
- "dragon mount" matched as an abbreviation to "dragon mount boost" (FEAT_DRAGON_MOUNT_BOOST)
- "dragon mount boost" has `in_game = FALSE`, so it returned "Could not find that feat"
- The search never reached the exact match "dragon mount" (FEAT_DRAGON_BOND)

### Resolution Applied
Fixed the search order in 5 functions to prioritize exact matches:

1. **`find_feat_num`** in feats.c (line 8325)
2. **`find_evolution_num`** in evolutions.c (line 1518)
3. **`find_skill_num`** in spell_parser.c (line 386)
4. **`find_ability_num`** in spell_parser.c (line 431)
5. **`find_discovery_num`** in alchemy.c (line 3163)

Each function now uses a 3-phase search:
- **Phase 1**: Exact match (case-insensitive) - highest priority
- **Phase 2**: Word-by-word matching for multi-word names
- **Phase 3**: Abbreviation matching as fallback

### Verification
- Code compiled successfully without errors
- The fix ensures "dragon mount" will find FEAT_DRAGON_BOND before checking abbreviations
- All similar search functions have been updated to prevent the same issue

### Previous Fix Attempts - FAILED
1. **July 23, 2025 Fix**: Added `&& !*first` to matching conditions in multiple files
   - Modified: feats.c, evolutions.c, spell_parser.c, alchemy.c
   - Result: Did not resolve the issue
   - Reason for failure: Misdiagnosed the root cause

---

## Issue #2: System Error Analysis - Multiple Recurring Issues

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

3. **Mobs accessing player-only data** - NOT FIXED
   - Multiple locations trying to use PRF_FLAGGED on NPCs
   - Error: "Mob using '((target)->player_specials->saved.pref)'"
   - **Status**: Still needs fixes in utils.c and magic.c

4. **Database schema mismatches** - PARTIALLY FIXED
   - Missing table: `luminari_muddev.pet_save_objs`
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
- **NOT FIXED**: Still needs code changes

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

#### Fix 3: PRF_FLAGGED on Mobs - STILL NEEDED

**utils.c:10447**:
```c
// Change from:
if (PRF_FLAGGED(target, PRF_NON_ROLEPLAYER))

// To:
if (!IS_NPC(target) && PRF_FLAGGED(target, PRF_NON_ROLEPLAYER))
```

**magic.c:9203**:
```c
// Change from:
if (PRF_FLAGGED(ch, PRF_CONTAIN_AOE) || (IS_NPC(ch) && ch->master && !IS_NPC(ch->master) && (ch->master, PRF_CONTAIN_AOE)))

// To:
if ((!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_CONTAIN_AOE)) || (IS_NPC(ch) && ch->master && !IS_NPC(ch->master) && PRF_FLAGGED(ch->master, PRF_CONTAIN_AOE)))
```

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
- PRF_FLAGGED usage still needs to be audited for NPC safety
- DG Script syntax errors require builder-level access to fix
- Database compatibility is partially implemented but may need expansion

### Related Files
- `dg_variables.c` - Script variable handling (FIXED)
- `utils.c` - PRF_FLAGGED usage at line 10447 (NEEDS FIX)
- `magic.c` - PRF_FLAGGED usage at line 9203 (NEEDS FIX)
- `objsave.c` - Database compatibility code (PARTIALLY FIXED)
- DG Script files for VNums 1015, 1920, 193 (NEED BUILDER ACCESS)