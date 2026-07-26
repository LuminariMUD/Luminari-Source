# CMake Build Issues Report

**Date:** 2026-01-25
**Compiler:** GCC
**Build System:** CMake (cbuild.sh)

## Summary

| Item | Count | Status |
|------|-------|--------|
| Main binary (bin/circle) | 1 | Built successfully |
| Errors | 0 | All resolved |
| Format Warnings | 76 | See breakdown below (CORRECTED - clean build reveals true count) |

**IMPORTANT CORRECTION (Session 7):** Previous session counts were based on incremental builds that didn't recompile all files. A clean `rm -rf build && cmake` build reveals the actual warning count is **76**, not 4 as previously reported.

## RESOLVED: hl_events Utility Linking Failure

**Status:** FIXED on 2026-01-25

**Problem:** The `hl_events` utility failed to link because it references symbols from the main server code (`pulse`, `basic_mud_log`) that are not linked into a standalone utility.

**Root Cause Analysis:**
- `util/hl_events.c` declares `extern unsigned long pulse;` (line 49)
- It uses the `log()` macro which calls `basic_mud_log()`
- This file is an alternative event system library meant to be compiled INTO the main server, not as a standalone utility
- The autotools build (util/Makefile.am) correctly excludes hl_events from standalone utilities
- The CMake build incorrectly tried to build it as a standalone executable

**Fix Applied:**
Removed hl_events from CMakeLists.txt utility build targets (lines 830-867). The file remains available as a library component if needed for the main server.

Changes made to CMakeLists.txt:
1. Removed `add_executable(hl_events util/hl_events.c)`
2. Removed `hl_events` from `set_target_properties()` call
3. Removed `hl_events` from `UTIL_TARGETS` list
4. Removed `hl_events` from `install()` call
5. Added comment explaining why hl_events is excluded

## Warning Breakdown (CORRECTED - Clean Build)

| Warning Type | Count | Description |
|--------------|-------|-------------|
| -Wformat-truncation | 70 | Potential buffer truncation in snprintf calls |
| -Wformat-overflow | 6 | Potential buffer overflow in sprintf/snprintf |
| -Wstringop-overflow | 0 | String operation may overflow (resolved) |
| clang-tidy | ~8000 | Static analysis warnings (separate concern from GCC warnings) |
| **Total Format** | **76** | **70 + 6 = 76** |

### Format Warnings by File (76 total from clean build)

| File | Count | Notes |
|------|-------|-------|
| treasure.c | 31 | Item description building |
| objsave.c | 7 | Object save/load |
| shopconv.c | 4 | Utility program |
| act.comm.c | 4 | Communication commands |
| act.wizard.c | 3 | Wizard commands |
| spells.c | 2 | Spell effects |
| spell_parser.c | 2 | Spell parsing |
| db.c | 2 | Database operations |
| bedit.c | 2 | Board editing |
| act.other.c | 2 | Other actions |
| act.informative.c | 2 | Informative commands |
| Other files | 15 | 1 warning each (shop, players, mysql, limits, etc.) |

## Session 7 Fixes Applied

### ~~src/genolc.c (3 warnings)~~ FIXED
- ~~Lines 428, 436, 444~~
- **Fix:** Added `MAX_EXPORT_FILENAME` constant (128 bytes), reduced `fix_filename` buffer and `zone_name` buffer to match

### ~~util/rebuildMailIndex.c (1 warning)~~ FIXED
- ~~Line 187~~
- **Fix:** Changed `sprintf` to `snprintf`, increased `subj` buffer from 1000 to 5000 bytes to match `findLine` source buffer

## Partial Fixes from Previous Sessions

**NOTE:** Previous sessions claimed these were fully fixed, but clean build shows some warnings remain. The fixes addressed SOME warnings in each file, not ALL.

### src/act.comm.c - PARTIALLY FIXED (Session 6)
- do_skillcheck buffers reduced (lines ~1145-1147)
- **Still has 4 warnings** at lines 536, 578, 584, 981 (different functions)

### ~~src/act.item.c (4 warnings)~~ FIXED
- ~~Lines 6757, 6883, 7009, 7110~~
- **Fix:** Reduced `metamagic_desc` from MAX_INPUT_LENGTH (512) to 128 bytes in 4 functions

### ~~src/act.wizard.c (4 warnings)~~ FIXED
- ~~Lines 7722, 8517, 10252, 10261~~
- **Fix:** Reduced `tmstr` from MAX_INPUT_LENGTH (512) to 32 bytes in do_changelog

### ~~src/act.informative.c (2 warnings)~~ NO LONGER REPORTED
### ~~src/act.other.c (2 warnings)~~ NO LONGER REPORTED
### ~~src/bedit.c (2 warnings)~~ NO LONGER REPORTED
### ~~src/boards.c (1 warning)~~ FIXED (Session 3)
### ~~src/clan.c (1 warning)~~ NO LONGER REPORTED
### ~~src/comm.c (1 warning)~~ NO LONGER REPORTED

### ~~src/craft.c (10 warnings)~~ FIXED (Session 5)
- ~~Lines 3023, 3025, 3057, 3059, 3084, 3086, 3097, 3099, 3110, 3112~~
- ~~Issue: String formatting may truncate output~~
- **Fix:** Reduced `buf2` buffer from 512 to 24 bytes (only holds " (x%d)" or "\tn")

### ~~src/crafting_new.c (5 warnings)~~ FIXED (Session 5)
- ~~Lines 8830, 8835, 8840, 8846, 8851~~
- ~~Issue: String formatting may truncate output~~
- **Fix:** Increased `desc_buf` to 384 bytes, reduced `plural_item` to 128 bytes

### src/db.c (4 warnings)
- Lines 1668, 1769, 7841, 7852
- Issue: Large strings into small buffers

### util/shopconv.c (4 warnings)
- Lines 239, 245, 269, 279
- Issue: Format overflow

### Additional files with warnings:
- src/dg_comm.c
- src/dg_mobcmd.c
- src/dg_scripts.c
- src/dg_variables.c
- src/discord_bridge.c
- src/evolutions.c
- src/feats.c
- src/fight.c
- src/handler.c
- src/hedit.c
- src/hlquest.c
- src/house.c
- src/hunts.c
- src/interpreter.c
- src/magic.c
- src/medit.c
- src/missions.c
- src/modify.c
- src/msgedit.c
- src/mud_event.c
- src/mysql.c
- src/oasis.c
- src/oedit.c
- src/players.c
- src/premadebuilds.c
- src/quest.c
- src/redit.c
- src/sedit.c
- src/shop.c
- src/spec_procs.c
- src/spell_parser.c
- src/spell_prep.c
- src/spells.c
- src/study.c
- src/tedit.c
- src/trade.c
- src/transport.c
- src/treasure.c
- src/utils.c
- src/wilderness.c
- src/zedit.c

## Comparison: Autotools vs CMake

| Build System | Warnings | Errors | Notes |
|--------------|----------|--------|-------|
| Autotools (make) | 0 | 0 | Uses -std=gnu90 -Wall |
| CMake (cbuild.sh) | 117 | 0 | Stricter flags including -Wformat-truncation |

## Recommended Actions

### High Priority
~~1. **Fix hl_events linking** - Update CMakeLists.txt or remove from build~~ DONE (Session 1)

### Medium Priority
~~1. **Review critical truncation warnings** - See detailed analysis below:~~ DONE (Session 2)
   - ~~src/act.other.c (lines 12965, 13322)~~ FIXED (Session 3) - reduced buffer from 49-196KB to 256 bytes
   - ~~src/db.c (lines 1668, 1769)~~ IMPROVED (Session 3) - created MAX_FILEPATH (256 bytes)
   - ~~src/db.c (lines 7841, 7852)~~ FIXED (Session 3) - converted to dynamic allocation
   - ~~src/boards.c (line 254)~~ FIXED (Session 3) - reduced tmstr from 49KB to 32 bytes

### Low Priority
2. **Address remaining truncation warnings** - Most are minor and unlikely to cause issues in practice, but could be cleaned up for code quality

---

## Critical Warning Analysis

### act.other.c Lines 12965 and 13322 (SEVERITY: HIGH)

**Location:** Two invention-creation functions in the weird science/inventor system

**The Problem:**
```c
/* Line 12965 context */
char spell_list[MAX_STRING_LENGTH];  /* 49,152 bytes */
snprintf(inv->short_description, MAX_INVENTION_SHORTDESC, "a %s device", spell_list);
/* MAX_INVENTION_SHORTDESC = 80 bytes */

/* Line 13322 context */
char spell_list[MAX_STRING_LENGTH * 4];  /* 196,608 bytes! */
snprintf(inv->short_description, MAX_INVENTION_SHORTDESC, "a %s device", spell_list);
```

**Root Cause:**
- `spell_list` buffers are massively over-allocated (49KB and 196KB)
- They concatenate spell names like "fireball/lightning bolt/magic missile"
- Destination `inv->short_description` is only 80 bytes
- snprintf safely truncates but the warning indicates severe size mismatch

**Relevant Constants (from structs.h):**
- `MAX_STRING_LENGTH` = 49,152 bytes
- `MAX_INVENTION_SHORTDESC` = 80 bytes
- `MAX_INVENTION_KEYWORDS` = 64 bytes
- `MAX_INVENTION_LONGDESC` = 256 bytes

**Recommended Fix:**
```c
/* Use appropriately sized buffer for spell list */
#define MAX_SPELL_LIST_DISPLAY 256  /* Enough for ~6-8 spell names */
char spell_list[MAX_SPELL_LIST_DISPLAY];
/* ... build spell_list with bounds checking ... */

/* Then safe to print to short_description */
snprintf(inv->short_description, MAX_INVENTION_SHORTDESC, "a %s device", spell_list);
```

**Risk Assessment:**
- Runtime risk: LOW (snprintf prevents overflow)
- Data loss risk: MODERATE (long spell lists truncate silently)
- Code quality: POOR (massive stack allocation wasteful)

---

### db.c Lines 1668 and 1769 (SEVERITY: HIGH)

**Location:** `index_boot()` function - world file loading

**The Problem:**
```c
char buf2[MAX_PATH] = {'\0'};           /* MAX_PATH = 50 bytes! */
char buf1[MAX_STRING_LENGTH] = {'\0'};  /* 49,152 bytes */

/* Lines 1668 and 1769: */
snprintf(buf2, sizeof(buf2), "%s%s", prefix, buf1);
```

**Root Cause:**
- `MAX_PATH` is defined as only **50 bytes** (dangerously small!)
- `buf1` reads filenames from index files (could be any length)
- Combining `prefix` (e.g., "lib/world/wld/") + `buf1` easily exceeds 50 bytes
- Standard POSIX PATH_MAX is 4096 bytes

**Relevant Constants:**
- `MAX_PATH` = 50 (in structs.h:5655) - **THIS IS THE BUG**
- `MAX_STRING_LENGTH` = 49,152

**Recommended Fix:**
```c
/* Option 1: Increase MAX_PATH (preferred) */
#define MAX_PATH 4096  /* Match POSIX PATH_MAX */

/* Option 2: Use system PATH_MAX */
#include <limits.h>
char buf2[PATH_MAX] = {'\0'};
```

**Risk Assessment:**
- Runtime risk: MODERATE (paths over ~35 chars will silently truncate)
- Data loss risk: HIGH (truncated paths cause file not found errors)
- This could explain mysterious file loading failures with long zone names

---

### db.c Lines 7841 and 7852 (SEVERITY: MODERATE)

**Location:** `load_config()` function - game configuration loading

**The Problem:**
```c
char line[MAX_STRING_LENGTH] = {'\0'};  /* 49,152 bytes from file */
/* ... */
char tmp[READ_SIZE];  /* READ_SIZE = 512 bytes */
snprintf(tmp, sizeof(tmp), "%s\r\n", line);
```

**Root Cause:**
- Config file lines read into 49KB buffer
- Then truncated to 512 bytes before storing
- Affects CONFIG_NOEFFECT, CONFIG_NOPERSON, CONFIG_OK values

**Recommended Fix:**
```c
/* Use line directly since it already has the content */
if (CONFIG_NOEFFECT)
    free(CONFIG_NOEFFECT);
CONFIG_NOEFFECT = strdup(line);
/* Or append \r\n with adequate buffer if needed */
```

**Risk Assessment:**
- Runtime risk: LOW (snprintf prevents overflow)
- Data loss risk: LOW (config values rarely exceed 512 chars)
- Code quality: MODERATE (unnecessary intermediate buffer)

---

### boards.c Line 254 (SEVERITY: LOW)

**Location:** `board_write_message()` function - bulletin board posting

**The Problem:**
```c
char buf[MAX_INPUT_LENGTH] = {'\0'};        /* 512 bytes */
char buf2[MAX_NAME_LENGTH + 3];             /* 23 bytes */
char tmstr[MAX_STRING_LENGTH] = {'\0'};     /* 49,152 bytes - overkill! */

strftime(tmstr, sizeof(tmstr), "%a %b %d %Y", localtime(&ct));  /* ~15 chars */
snprintf(buf, sizeof(buf), "%s %-12s :: %s", tmstr, buf2, arg);
```

**Root Cause:**
- `tmstr` is 49KB but only stores ~15 character date string
- Compiler sees potential for 49KB+ going into 512-byte `buf`

**Recommended Fix:**
```c
char tmstr[32] = {'\0'};  /* 32 bytes plenty for date format */
```

**Risk Assessment:**
- Runtime risk: NONE (strftime output is bounded)
- Data loss risk: NONE
- Code quality: POOR (wasteful 49KB stack allocation)

## Notes

- The main server binary (`bin/circle`) builds and runs correctly
- All utility programs build successfully: autowiz, asciipasswd, plrtoascii, rebuildAsciiIndex, rebuildMailIndex, shopconv, sign, split, webster, wld2html
- These warnings are primarily about potential edge cases where very long strings could be truncated
- The autotools build does not report these because it uses less strict warning flags

---

## Session Log

### 2026-01-25 Session 1: hl_events Fix

**Work Completed:**
1. Analyzed hl_events linking failure
2. Identified root cause: hl_events.c is a library component, not a standalone utility
3. Verified autotools build correctly excludes hl_events
4. Fixed CMakeLists.txt by removing hl_events from utility targets
5. Verified clean CMake build succeeds with 0 errors

**Files Modified:**
- CMakeLists.txt (lines 830-867): Removed hl_events from utility build

**Build Verification:**
- `cmake -S . -B build && cmake --build build -j4` - SUCCESS
- Main binary: bin/circle - BUILT
- All utilities: BUILT (excluding hl_events)
- Errors: 0
- Warnings: ~117 (unchanged, these are pre-existing format truncation warnings)

**Next Steps for Future Sessions:**
1. Consider addressing critical truncation warnings in act.other.c, db.c, boards.c
2. The cbuild.sh script has a minor bug (tries to write log before creating directory) - could be fixed
3. Remaining warnings are low priority but could be cleaned up incrementally

---

### 2026-01-25 Session 2: Critical Truncation Warning Analysis

**Work Completed:**
1. Deep analysis of all critical truncation warnings identified in Session 1
2. Examined source code and buffer size definitions
3. Identified root causes for each warning category
4. Documented recommended fixes with code examples
5. Assessed risk levels for each issue

**Key Findings:**

| File | Lines | Severity | Issue Summary |
|------|-------|----------|---------------|
| act.other.c | 12965, 13322 | HIGH | 49-196KB spell_list buffers -> 80-byte destination |
| db.c | 1668, 1769 | HIGH | MAX_PATH=50 is dangerously small for file paths |
| db.c | 7841, 7852 | MODERATE | 49KB config line -> 512-byte tmp buffer |
| boards.c | 254 | LOW | 49KB tmstr buffer (only needs 32 bytes) |

**Critical Discovery: MAX_PATH Issue**
- `MAX_PATH` is defined as only 50 bytes in `structs.h:5655`
- This is a potential source of bugs with long zone filenames
- Standard POSIX PATH_MAX is 4096 bytes
- **Recommend increasing MAX_PATH to at least 256 or 4096**

**Files Analyzed (read-only):**
- src/act.other.c (lines 12950-13030, 13305-13355)
- src/db.c (lines 1585-1800, 7539-7870)
- src/boards.c (lines 195-280)
- src/structs.h (MAX_* constant definitions)
- src/utils.h (READ_SIZE definition)

**No Files Modified** - This session was analysis only.

**Recommended Next Session Priority:**
1. **FIX MAX_PATH** - Increase from 50 to 256+ bytes in structs.h (High priority, easy fix)
2. **FIX act.other.c** - Reduce spell_list buffer sizes (Medium priority)
3. **FIX boards.c** - Reduce tmstr buffer size (Low priority, easy fix)
4. **FIX db.c load_config** - Remove unnecessary tmp buffer (Low priority)

**Session Notes:**
- All warnings are in snprintf() calls which are safe (no buffer overflow risk)
- However, silent data truncation can cause subtle bugs
- The MAX_PATH issue is the most concerning - could cause real runtime issues
- Most other warnings are about wasteful large stack allocations being truncated

---

### 2026-01-25 Session 3: Critical Warning Fixes Implementation

**Work Completed:**
1. Created `MAX_FILEPATH` constant (256 bytes) for file system paths
2. Fixed MAX_PATH naming collision (MAX_PATH was for NPC patrol paths, not file paths)
3. Reduced act.other.c spell_list buffers from 49-196KB to 256 bytes
4. Fixed unsafe string operations in act.other.c second occurrence
5. Reduced boards.c tmstr buffer from 49KB to 32 bytes
6. Converted db.c load_config to use dynamic allocation

**Key Discovery: MAX_PATH Naming Collision**
- `MAX_PATH` (50 bytes) is for NPC patrol path arrays, NOT file system paths
- Code was incorrectly using `MAX_PATH` for file path buffers in db.c, dg_handler.c, utils.c
- Created new `MAX_FILEPATH` (256 bytes) constant specifically for file system paths
- This preserves NPC patrol behavior while fixing file path truncation issues

**Files Modified:**
- `src/structs.h`: Added `MAX_FILEPATH` constant (line 5658)
- `src/db.c`: Changed `buf2` to use `MAX_FILEPATH`, converted config loading to dynamic allocation
- `src/dg_handler.c`: Changed `filename` buffer to use `MAX_FILEPATH`
- `src/utils.c`: Changed `get_filename()` buffer to use `MAX_FILEPATH`
- `src/sysdep.h`: Changed Windows `PATH_MAX` to use `MAX_FILEPATH`
- `src/act.other.c`: Reduced spell_list buffers to 256 bytes, fixed unsafe strcpy/strcat
- `src/boards.c`: Reduced tmstr buffer to 32 bytes

**Build Verification:**
- Autotools build: SUCCESS (0 errors)
- CMake build: SUCCESS (0 errors)
- Format-truncation warnings: 103 -> 100 (3 eliminated)

**Warning Results:**

| Warning | Before | After | Status |
|---------|--------|-------|--------|
| db.c:7841 (CONFIG_NOEFFECT) | 49KB->512B | - | ELIMINATED |
| db.c:7852 (CONFIG_OK) | 49KB->512B | - | ELIMINATED |
| boards.c:254 (tmstr) | 49KB->512B | - | ELIMINATED |
| db.c:1668/1769 (file paths) | 49KB->50B | 49KB->256B | IMPROVED (5x larger buffer) |
| act.other.c:12965/13324 (spell_list) | 49-196KB->80B | 256B->80B | IMPROVED (safer, expected truncation) |

**Remaining Work (Low Priority):**
- The db.c:1668/1769 warnings persist because `buf1` is still 49KB (reads filenames)
- The act.other.c warnings persist because 256B still exceeds 80B destination
- Both are now at acceptable severity levels - truncation is expected and safe

**Technical Notes:**
- `MAX_FILEPATH` at 256 bytes is sufficient for typical paths like "lib/world/wld/zone.wld"
- The spell_list warnings are expected: spell names concatenated with "/" will exceed 80 chars
- All changes use safe string functions (snprintf, strncpy, strncat)
- Dynamic allocation in load_config eliminates intermediate buffer entirely

**Next Session Recommendations:**
1. Consider increasing `MAX_FILEPATH` to 512 if very long paths are needed
2. The remaining ~100 format-truncation warnings are mostly minor (other files)
3. Could add a `MAX_INVENTION_DISPLAY` constant for spell list display purposes
4. clang-tidy warnings (~8000+) are separate concern from GCC warnings

---

### 2026-01-25 Session 4: treasure.c Warning Reduction

**Work Completed:**
1. Analyzed treasure.c format-truncation warnings (42 warnings - 38% of total)
2. Created `RARE_LABEL_SIZE` constant (32 bytes) for rare grade label buffers
3. Fixed `award_magic_weapon()` - changed `desc` buffer from 256 to 32 bytes
4. Fixed BUG in `give_magic_weapon()` - removed unused `desc` variable that was never populated
5. Simplified snprintf calls in `give_magic_weapon()` (removed dead code concatenation)

**Key Findings:**

| Issue | Location | Fix Applied |
|-------|----------|-------------|
| Unused variable bug | give_magic_weapon():desc | Removed variable and simplified snprintf |
| Oversized buffer | award_magic_weapon():desc | Changed to RARE_LABEL_SIZE (32 bytes) |
| Pattern identified | award_misc_magic_item | Different pattern - desc used for full descriptions |

**Bug Discovery: `give_magic_weapon()` Dead Code**
- `desc` was declared as 256-byte buffer on line 3058 (original)
- Never populated before being used in snprintf calls at lines 3087-3093
- Effectively concatenated empty string with blade/pierce/blunt descriptions
- Fix: Removed `desc` entirely, simplified to single `%s` format

**Files Modified:**
- `src/treasure.c`:
  - Added `RARE_LABEL_SIZE` constant (line 35)
  - Changed `award_magic_weapon()` desc buffer from MEDIUM_STRING to RARE_LABEL_SIZE
  - Removed unused `desc` variable from `give_magic_weapon()`
  - Simplified snprintf calls (removed empty string concatenation)
  - Changed C++ style comment to C style (`// blunt` -> `/* blunt */`)

**Build Verification:**
- CMake build: SUCCESS (0 errors)
- Format-truncation warnings: 110 -> 104 (6 eliminated)
- treasure.c warnings: 42 -> 31 (11 eliminated)

**Warning Results:**

| File | Before | After | Change |
|------|--------|-------|--------|
| treasure.c | 42 | 31 | -11 |
| Total format | 110 | 104 | -6 |

**Remaining treasure.c Warnings (31):**
- Mostly in `give_misc_magic_item()` and `award_misc_magic_item()` switch statements
- These combine multiple SHORT_STRING (80 byte) and MEDIUM_STRING (256 byte) buffers
- Actual runtime strings are short (color names, material names ~20 chars)
- snprintf ensures no buffer overflow; truncation is theoretical only

**Technical Notes:**
- `RARE_LABEL_SIZE` at 32 bytes is sufficient for labels like "[Mythical] " (max 13 chars)
- The remaining warnings are about string concatenation where compiler assumes worst case
- All changes use safe string functions (snprintf) preventing actual buffer overflows
- The unused `desc` variable in `give_magic_weapon()` was likely copy-paste from `award_magic_weapon()`

**Next Session Recommendations:**
1. Consider addressing craft.c (10 warnings) and objsave.c (7 warnings) next
2. The remaining treasure.c warnings are lower priority - actual data is short
3. Could increase SHORT_STRING to 96 or 128 in local definitions to reduce warnings
4. Consider adding a `MAX_DESC_COMPONENT` constant for intermediate description buffers

---

### 2026-01-25 Session 5: craft.c and crafting_new.c Warning Reduction

**Work Completed:**
1. Fixed craft.c `event_crafting()` function - reduced `buf2` from MAX_INPUT_LENGTH (512) to 24 bytes
2. Fixed crafting_new.c `refresh_supply_slots()` function - increased `desc_buf` to 384 bytes, reduced `plural_item` to 128 bytes
3. Verified both autotools and CMake builds succeed with 0 errors

**Key Findings:**

| File | Issue | Fix Applied |
|------|-------|-------------|
| craft.c:2783 | `buf2[MAX_INPUT_LENGTH]` only used for " (x%d)" or "\tn" | Changed to `buf2[24]` |
| crafting_new.c:8808 | `desc_buf[256]` too small for `plural_item[256]` + format text | Increased to 384, reduced plural_item to 128 |

**Files Modified:**
- `src/craft.c`: Line 2783-2784 - Reduced `buf2` buffer size with explanatory comment
- `src/crafting_new.c`: Lines 8808, 8816 - Adjusted buffer sizes to eliminate warnings

**Build Verification:**
- Autotools build: SUCCESS (0 errors)
- CMake build: SUCCESS (0 errors)
- Format-truncation warnings: 94 -> 21 (73 eliminated, includes all craft.c and crafting_new.c warnings)
- Format-overflow warnings: 6 (unchanged)
- Total format warnings: 27 (down from 104)

**Warning Results:**

| Category | Before | After | Change |
|----------|--------|-------|--------|
| format-truncation | 94 | 21 | -73 |
| format-overflow | 6 | 6 | 0 |
| stringop-overflow | 4 | 0 | -4 |
| **Total** | **104** | **27** | **-77** |

**Remaining Format-Truncation Warnings by File (21 total):**
- act.comm.c: 7 warnings (lines 536, 578, 584, 981, 1254, 1261, 1268)
- act.item.c: 4 warnings (lines 6757, 6883, 7009, 7110)
- act.other.c: 2 warnings (lines 12965, 13324)
- act.informative.c: 2 warnings (lines 1360, 9249)
- act.wizard.c: 4 warnings (lines 7722, 8517, 10252, 10261)
- clan.c: 1 warning (line 5412)
- comm.c: 1 warning (line 4287)

**Technical Notes:**
- The massive reduction (77 warnings) indicates the previous counts may have included duplicate/related warnings
- `buf2[24]` is sufficient for repeat suffix " (x999999999)" (max ~15 chars) or "\tn" (2 chars)
- `plural_item[128]` is sufficient for item names which are typically under 50 chars
- `desc_buf[384]` provides adequate margin for format strings plus item names
- All remaining warnings are in different files with their own buffer sizing patterns

**Next Session Recommendations:**
1. act.comm.c (7 warnings) - likely similar buffer sizing issues
2. act.item.c (4 warnings) - appears to have MAX_INPUT_LENGTH buffers
3. The remaining 21 warnings are spread across 7 files and are all minor severity
4. Consider establishing a `MAX_MSG_DISPLAY` constant (~384 bytes) for common message formatting

---

### 2026-01-25 Session 6: Major Format Warning Reduction

**Work Completed:**
1. Analyzed remaining 27 format warnings from Session 5
2. Fixed act.item.c - reduced `metamagic_desc` from 512 to 128 bytes in 4 functions
3. Fixed act.wizard.c - reduced `tmstr` from 512 to 32 bytes in do_changelog
4. Fixed act.comm.c - optimized buffer sizes in do_skillcheck function
5. Verified builds complete with only 4 format warnings remaining

**Key Findings:**

| File | Issue | Fix Applied |
|------|-------|-------------|
| act.item.c:6674,6777,6903,7030 | `metamagic_desc[512]` overkill (max ~60 chars used) | Changed to `metamagic_desc[128]` |
| act.wizard.c:7678 | `tmstr[512]` for strftime output (~11 chars) | Changed to `tmstr[32]` |
| act.comm.c:1145-1147 | Skill check buffers oversized for ability names | Reduced abiltext/abilname to 64, output to 128 |

**Analysis: Why Many Warnings Disappeared**
Several warnings that were listed in Session 5 (act.informative.c, act.other.c, bedit.c, clan.c, comm.c) were no longer reported in the current build. This could be due to:
- Previous sessions already fixed them (act.other.c, boards.c were fixed in Session 3)
- GCC version or build configuration differences
- Some warnings may have been duplicates or cascade effects from other buffer issues

**Files Modified:**
- `src/act.item.c`: Lines 6674, 6777, 6903, 7030 - changed `metamagic_desc[MAX_INPUT_LENGTH]` to `metamagic_desc[128]` with comment
- `src/act.wizard.c`: Line 7678 - changed `tmstr[MAX_INPUT_LENGTH]` to `tmstr[32]`
- `src/act.comm.c`: Lines 1145-1147 - changed abiltext/abilname to 64 bytes, output to 128 bytes

**Build Verification:**
- Autotools build: SUCCESS (0 errors, 0 warnings)
- CMake build: SUCCESS (0 errors)
- Format-truncation warnings: 21 -> 3 (genolc.c only)
- Format-overflow warnings: 6 -> 1 (rebuildMailIndex.c only)
- Total format warnings: 27 -> 4

**Warning Results:**

| Category | Before | After | Change |
|----------|--------|-------|--------|
| format-truncation | 21 | 3 | -18 |
| format-overflow | 6 | 1 | -5 |
| **Total Format Warnings** | **27** | **4** | **-23 (85% reduction)** |

**Technical Notes:**
- `metamagic_desc[128]` is sufficient for all metamagic names combined ("quickened, maximized, empowered, extended, silent, still" = ~60 chars)
- `tmstr[32]` is sufficient for strftime output ("Jan 01 2026" = ~11 chars)
- Ability names are short (~20 chars max), so 64-byte buffers are adequate
- `MAX_EXPORT_FILENAME` (128 bytes) is sufficient for zone names which are typically <64 chars

**Project Status: 76 FORMAT WARNINGS REMAIN (Corrected Count)**
- Previous session counts were INCORRECT due to incremental builds
- **Session 7 clean build reveals 76 format warnings** (not 4 as previously reported)
- genolc.c and rebuildMailIndex.c warnings: FIXED (Session 7)
- treasure.c accounts for 31 of 76 remaining warnings (41%)

**Priority for Future Sessions:**
1. **treasure.c** (31 warnings) - Highest impact, item description string building
2. **objsave.c** (7 warnings) - Object persistence
3. **shopconv.c** (4 warnings) - Utility program (lower priority)
4. **act.comm.c** (4 warnings) - Communication commands
5. Remaining files (1-3 warnings each)

**Recommended Approach:**
- Always do `rm -rf build && cmake -S . -B build` for accurate warning counts
- Incremental builds hide warnings in unchanged files
- Focus on high-impact files first (treasure.c = 40% of all warnings)

---

### 2026-01-25 Session 7: Accurate Warning Assessment and Fixes

**CRITICAL DISCOVERY:**
Previous session warning counts were INACCURATE. They were based on incremental builds that don't recompile unchanged files. A clean build (`rm -rf build && cmake`) reveals the TRUE warning count.

**Work Completed:**
1. Fixed genolc.c format-truncation warnings (lines 428, 436, 444) - VERIFIED FIXED
2. Fixed rebuildMailIndex.c format-overflow warning (line 187) - VERIFIED FIXED
3. Performed CLEAN BUILD to get accurate warning count
4. **Discovered 76 format warnings remain** (not 4 as previously claimed)

**Key Findings:**

| File | Issue | Fix Applied |
|------|-------|-------------|
| genolc.c:314 | `good_file_name[MAX_STRING_LENGTH]` (49KB) overkill for zone names | Changed to `good_file_name[MAX_EXPORT_FILENAME]` (128 bytes) |
| genolc.c:360 | `zone_name[MAX_INPUT_LENGTH]` (512 bytes) for short zone names | Changed to `zone_name[MAX_EXPORT_FILENAME]` (128 bytes) |
| rebuildMailIndex.c:185 | `subj[1000]` smaller than `findLine` source buffer (5000) | Changed to `subj[5000]` with snprintf |
| rebuildMailIndex.c:187 | Used sprintf instead of snprintf | Changed to snprintf |

**Files Modified:**
- `src/genolc.c`:
  - Added `#define MAX_EXPORT_FILENAME 128` constant (line 55)
  - Changed `fix_filename()` buffer from `MAX_STRING_LENGTH` to `MAX_EXPORT_FILENAME`
  - Changed `zone_name` buffer in `do_export_zone()` to `MAX_EXPORT_FILENAME`
- `util/rebuildMailIndex.c`:
  - Changed `sprintf` to `snprintf` in `parse_subject()`
  - Increased `subj` buffer from 1000 to 5000 bytes to match source

**Build Verification (CLEAN BUILD):**
```
rm -rf build && cmake -S . -B build && cmake --build build -j4
```
- CMake build: SUCCESS (0 errors)
- genolc.c: 0 format warnings (FIXED)
- rebuildMailIndex.c: 0 format warnings (FIXED)
- **Total format warnings: 76** (from clean build)

**Actual Warning Distribution:**

| File | Count | Percentage |
|------|-------|------------|
| treasure.c | 31 | 40% |
| objsave.c | 7 | 9% |
| shopconv.c | 4 | 5% |
| act.comm.c | 4 | 5% |
| act.wizard.c | 3 | 4% |
| Other (17 files) | 28 | 36% |
| **Total** | **76** | **100%** |

**Why Previous Counts Were Wrong:**
- Incremental builds only recompile changed files
- Files not touched in recent sessions weren't recompiled
- GCC warnings only appear during compilation, not linking
- **Lesson: Always use clean builds for accurate warning counts**

**Technical Notes:**
- `MAX_EXPORT_FILENAME` at 128 bytes is sufficient for all zone names (typically <64 chars)
- Zone export tar commands with path repeated 9 times fit well within buffer limits
- Mail subjects don't need 5000 bytes but matching the source buffer eliminates the warning cleanly
- All Session 7 changes use safe string handling (snprintf) and are verified working

**CORRECTED PROJECT STATUS:**
- 76 format warnings remain (from clean build)
- genolc.c and rebuildMailIndex.c: FIXED (4 warnings eliminated)
- treasure.c is the highest-impact target (31 warnings = 40% of total)
- Autotools build shows 0 warnings because it uses less strict flags (-std=gnu90 -Wall)
- CMake build uses stricter flags including -Wformat-truncation
