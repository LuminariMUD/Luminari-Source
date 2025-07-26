# LuminariMUD Development Task List

## CODER TASKS

### ✅ RESOLVED: Player Death Crash - Root Cause Found and Fixed (2025-07-26)

**Problem**: malloc_consolidate crash during player death when save_char() allocates memory

**Root Cause Identified**: Uninitialized `next` pointer in `affected_type` structure causing heap corruption

**Final Solution (Attempt #6 - SUCCESSFUL)**:
1. Fixed `new_affect()` to initialize the `next` pointer to NULL
2. Applied defensive programming by zero-initializing all stack-allocated `affected_type` structures
3. See CHANGELOG 2025-07-26 for full details

**Previous Failed Attempts** (for historical reference):
1. **Clear events + safety checks** (2025-07-25) - Added clear_char_event_list() and checks in event_combat_round()
2. **Use affect_remove_no_total for NPCs** (2025-07-25) - Changed NPC death to skip affect_total() 
3. **Check POS_DEAD in update_msdp_affects()** (2025-07-26) - Didn't work, update_pos() changes DEAD→RESTING
4. **Use affect_remove_no_total for players** (2025-07-26) - Skip affect_total() during player death
5. **Skip update_pos() if target POS_DEAD** (2025-07-26) - Added check in valid_fight_cond() [FAILED]

**ROOT CAUSE ANALYSIS**: The crash in malloc_consolidate() indicates heap corruption that occurred BEFORE the allocation attempt. All previous fixes targeted symptoms, not the root cause.

### COMPREHENSIVE MULTI-ANGLED AUDIT PLAN

#### Phase 1: Memory Corruption Detection (GDB Analysis)
1. **Enable malloc debugging**:
   - Set `MALLOC_CHECK_=3` environment variable
   - Run with valgrind: `valgrind --leak-check=full --track-origins=yes --malloc-fill=0xde --free-fill=0xad`
   - Use AddressSanitizer: compile with `-fsanitize=address -g`

2. **GDB watchpoints on the dying character**:
   ```gdb
   # Set watchpoint on character structure
   watch *(struct char_data *)0x25279100
   # Watch for any writes to the memory region
   watch -l *((char*)0x25279100)@sizeof(struct char_data)
   # Look for writes after character should be "dead"
   ```

3. **Heap analysis commands**:
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

2. **AFFECT STRUCTURE CORRUPTION** (CRITICAL)
   - The `af` structure in raw_kill() has corrupted `next` pointer
   - Value 0x72656820676e6979 = "reh gniy" (part of "dying her...")
   - Likely source: death message like "Pyret is dying here"
   - Check affect creation/removal during death

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
- [ ] Identify exact source of heap corruption
- [ ] Reproduce reliably in controlled environment
- [ ] Fix addresses root cause, not symptoms
- [ ] No performance degradation
- [ ] Passes stress testing with multiple deaths
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
   - [ ] Check `raw_kill()` - where af structure gets corrupted
   - [ ] Audit all sprintf/snprintf in death sequence
   - [ ] Look for strcpy/strcat without bounds checking
   - [ ] Check act() macro usage with long strings

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

### GDB RESULTS - CRITICAL FINDINGS

**🚨 HEAP CORRUPTION CONFIRMED - SMOKING GUN FOUND! 🚨**

#### 1. Corrupted Register Values (Frame 0 - malloc_consolidate)
```
rbx = 0x20657370726f6370  // ASCII: " esproc" (corrupted pointer)
r15 = 0x20657370726f6370  // Same corruption pattern!
```
This is ASCII text in pointer registers! The value translates to " esproc" which appears to be part of a string that overwrote critical malloc metadata.

#### 2. Suspicious String in raw_kill (Frame 4)
```
af = {spell = 1229, duration = 30, modifier = 0, location = 0 '\000', bitvector = {0, 0, 0, 0}, 
      bonus_type = 7, next = 0x72656820676e6979, specific = 0}
                            ^^^^^^^^^^^^^^^^^^^
                            ASCII: "reh gniy" (reversed: "ying her")
```
The `af.next` pointer contains ASCII text! This suggests buffer overflow corrupting the affect structure.

#### 3. Character State Analysis
- **Victim (Pyret)**: Level 30 player, position = 6 (FIGHTING), hit = 1 (near death)
- **Killer (githyanki soldier)**: NPC, has active events (0x25332dc0)
- Both have `fighting = 0x0` (already cleared)
- Both have `events = 0x0` for victim, but killer still has events

#### 4. Memory Layout Before Character
```
0x252790f0: 0x00000000  0x00000000  0x000045f1  0x00000000
                                     ^^^^^^^^^^
                                     Malloc chunk size (17905 bytes)
```
The malloc metadata appears intact here, but the corruption is in the malloc free list.

### ROOT CAUSE IDENTIFIED

The heap corruption is caused by a **buffer overflow** that writes ASCII text into memory regions that should contain pointers. The pattern " esproc" and "ying her" suggests combat message text is overflowing its buffer.

---
