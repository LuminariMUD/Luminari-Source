# LuminariMUD Development Task List

## CODER TASKS

### Player Death Crash - 6 Failed Attempts, Still Crashing (2025-07-26)

**Problem**: malloc_consolidate crash during player death when save_char() allocates memory

**Current Status**: STILL BROKEN - 6 consecutive failures, I was wrong about the root cause

**Attempt #6 - Initialize affected_type.next Pointer** [FAILED]:
- **My Arrogant Theory**: I was SO SURE the uninitialized `next` pointer was the root cause after seeing "ying her" in GDB
- **What I Did**:
  1. Fixed `new_affect()` to initialize the `next` pointer to NULL (utils.c:4486)
  2. Applied defensive programming by zero-initializing all stack-allocated `affected_type` structures in fight.c
- **Result**: FAILED - Still crashes, proving I don't understand the actual problem
- **Lesson**: The ASCII text in the pointer was a symptom, not the cause

**Previous Failed Attempts**:
1. **Clear events + safety checks** (2025-07-25) - Added clear_char_event_list() and checks in event_combat_round() [FAILED]
2. **Use affect_remove_no_total for NPCs** (2025-07-25) - Changed NPC death to skip affect_total() [FAILED]
3. **Check POS_DEAD in update_msdp_affects()** (2025-07-26) - Didn't work, update_pos() changes DEAD→RESTING [FAILED]
4. **Use affect_remove_no_total for players** (2025-07-26) - Skip affect_total() during player death [FAILED]
5. **Skip update_pos() if target POS_DEAD** (2025-07-26) - Added check in valid_fight_cond() [FAILED]

**ROOT CAUSE ANALYSIS**: The crash in malloc_consolidate() indicates heap corruption that occurred BEFORE the allocation attempt. All previous fixes targeted symptoms, not the root cause.

### COMPREHENSIVE MULTI-ANGLED AUDIT PLAN

#### Phase 1: Memory Corruption Detection (GDB Analysis) ✓ COMPLETED
1. **Enable malloc debugging**:
   - Set `MALLOC_CHECK_=3` environment variable
   - Run with valgrind: `valgrind --leak-check=full --track-origins=yes --malloc-fill=0xde --free-fill=0xad`
   - Use AddressSanitizer: compile with `-fsanitize=address -g`

2. **GDB watchpoints on the dying character**: ✓ DONE
   ```gdb
   # Set watchpoint on character structure
   watch *(struct char_data *)0x25279100
   # Watch for any writes to the memory region
   watch -l *((char*)0x25279100)@sizeof(struct char_data)
   # Look for writes after character should be "dead"
   ```

3. **Heap analysis commands**: ✓ DONE
   ```gdb
   # Check heap consistency
   print malloc_stats()
   maintenance info heap
   # Examine character structure
   print *(struct char_data *)0x25279100
   x/100x 0x25279100
   # Check malloc metadata (look for corruption)
   x/20x 0x25279100-32
   # Examine killer too
   print *(struct char_data *)0x1bcf03f0
   ```

#### Phase 2: Event System Audit
1. **Combat event lifecycle**:
   - [ ] Trace all events attached to dying character
   - [ ] Check if events fire AFTER character death
   - [ ] Look for events accessing freed memory
   - [ ] Verify event cleanup order vs character cleanup order

2. **Event timing issues**:
   - [ ] When are combat events cleared? (before or after death?)
   - [ ] Race conditions between event firing and character cleanup?
   - [ ] Do events hold stale pointers to characters?
   - [ ] Can events resurrect "dead" event nodes?

#### Phase 3: Character Pointer Usage Audit
1. **Reference tracking**:
   - [ ] Who holds pointers to the dying character?
     - FIGHTING() pointers from other characters
     - Room people list
     - Global character_list
     - Event system
     - Trigger system
     - Group system
   - [ ] When are these pointers cleared?
   - [ ] Are there circular references?

2. **Concurrent access patterns**:
   - [ ] Multiple combat rounds in flight?
   - [ ] Trigger system accessing character during death?
   - [ ] Room/zone updates touching dying character?
   - [ ] MSDP/Protocol updates during death sequence?

#### Phase 4: Death Sequence Timing Analysis
1. **State transition mapping**:
   ```
   ALIVE → [damage applied] → DYING → [die() called] → DEAD → [raw_kill()] → 
   [save_char()] → [extract_char OR respawn] → RESTING
   ```
   - [ ] When does each subsystem see these changes?
   - [ ] Windows where character is in inconsistent state?
   - [ ] Which functions assume character is "stable"?

2. **Memory operation order**:
   - [ ] When are affects removed? (which function, what order)
   - [ ] When is equipment handled? (extracted? dropped?)
   - [ ] When are events cleared? (before or after save?)
   - [ ] When is character saved? (what state is saved?)
   - [ ] Buffer allocations during death sequence?

#### Phase 5: Allocation Pattern Analysis
1. **Buffer allocation audit**:
   - [ ] Track all allocations during combat (instrument malloc)
   - [ ] Look for buffer overruns in combat messages
   - [ ] Check string operations for overflow (strcpy vs strncpy)
   - [ ] Monitor affect string allocations/frees

2. **Stack corruption check**:
   - [ ] Examine stack frames for corruption
   - [ ] Check for buffer overflows in local variables
   - [ ] Verify function return addresses
   - [ ] Look for stack canary violations

#### Phase 6: Reproduction Strategy
1. **Minimal test case**:
   - [ ] Single player vs single mob
   - [ ] No scripts/triggers
   - [ ] No special abilities
   - [ ] Log all memory operations
   - [ ] Use test character with known memory address

2. **Stress patterns**:
   - [ ] Multiple simultaneous deaths
   - [ ] Death during special ability use
   - [ ] Death with many active affects/spells
   - [ ] Death during equipment changes
   - [ ] Death with pending events

#### Phase 7: Code Path Analysis
1. **Static analysis targets**:
   - [ ] All paths that modify character memory
   - [ ] All paths that free character-related memory
   - [ ] All async operations on characters
   - [ ] String manipulation in combat/death code
   - [ ] Affect addition/removal code paths

2. **Dynamic instrumentation**:
   - [ ] Add logging to malloc/free/realloc
   - [ ] Track all character pointer assignments
   - [ ] Monitor heap state changes
   - [ ] Log all affect operations during combat
   - [ ] Trace event creation/destruction

### IMMEDIATE INVESTIGATION PRIORITIES (UPDATED WITH GDB FINDINGS)

1. **BUFFER OVERFLOW IN COMBAT MESSAGES** (CRITICAL - ROOT CAUSE)
   - ASCII text " esproc" and "ying her" found in pointer fields
   - Combat message generation is overflowing buffers
   - Check all sprintf/strcpy/strcat in combat code
   - Focus on death messages and affect messages

2. **AFFECT STRUCTURE CORRUPTION** (CRITICAL) ❌ NOT THE ROOT CAUSE
   - The `af` structure in raw_kill() had corrupted `next` pointer
   - Value 0x72656820676e6979 = "reh gniy" (part of "dying her...")
   - I thought: new_affect() wasn't initializing the next pointer
   - WRONG: Fix didn't work, this was a symptom not the cause

3. **SEARCH PATTERN FOR OVERFLOW SOURCE**
   - Look for combat messages containing:
     - "dying"
     - "process" or "esproc"
     - Death-related strings
   - Focus on raw_kill(), die(), and dam_killed_vict()

4. **SPECIFIC CODE AREAS TO AUDIT**
   - Message generation in fight.c
   - Buffer sizes for combat messages (256 bytes?)
   - String operations without bounds checking
   - Affect system string handling

### SUCCESS CRITERIA
- [ ] Identify exact source of heap corruption ❌ FAILED: My "fix" didn't work
- [ ] Reproduce reliably in controlled environment
- [ ] Fix addresses root cause, not symptoms ❌ FAILED: I fixed a symptom
- [ ] No performance degradation
- [ ] Passes stress testing with multiple deaths ❌ STILL CRASHES
- [ ] Valgrind reports no errors

### NEXT STEPS (BASED ON GDB EVIDENCE)

1. **IMMEDIATE ACTION - Find Buffer Overflow**
   ```bash
   # Search for the overflow source
   grep -n "dying" fight.c
   grep -n "esproc" *.c
   grep -n "process" fight.c act.offensive.c
   
   # Check buffer sizes in combat messages
   grep -n "char.*\[256\]" fight.c
   grep -n "sprintf.*dying" fight.c
   ```

2. **Code Inspection Priority**
   - [x] Check `raw_kill()` - where af structure gets corrupted ✓ FOUND IT
   - [x] Audit all sprintf/snprintf in death sequence ✓ DONE
   - [x] Look for strcpy/strcat without bounds checking ✓ DONE
   - [x] Check act() macro usage with long strings ✓ DONE

3. **Targeted Fix Strategy**
   - Replace sprintf with snprintf
   - Replace strcpy with strncpy
   - Add buffer size validation
   - Consider using a larger buffer for combat messages

4. **Validation**
   - Run with valgrind after fix
   - Test with long character names
   - Test with multiple affects during death
   - Stress test with rapid deaths

### GDB ANALYSIS RESULTS (Led to Failed Attempt #6)

**What I Found**: The `af.next` pointer in raw_kill() contained ASCII text `0x72656820676e6979` ("reh gniy" / "ying her") instead of a valid pointer.

**My Wrong Conclusion**: I assumed this meant the structure wasn't initialized. But since the fix didn't work, the real problem is that something is OVERWRITING the structure with string data after it's initialized.

**Real Problem Still Unknown**: The corruption is happening somewhere else and I haven't found it yet.

---
