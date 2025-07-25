# LuminariMUD Development Task List

## CODER TASKS

### Memory Leaks and Issues (From Valgrind Analysis - July 25, 2025)

Log file for reference if needed: valgrind_20250725_122944.md

#### 🔴 CRITICAL ISSUES - Program Crashes/Corruption

| ☐ | Location | Issue | Details | Priority |
|---|----------|-------|---------|----------|
| ☑ | spell_prep.c:97 | **SEGFAULT** - Invalid read causing crash | Reading freed memory (0xbbbbbbbbbbbbbbcb) in clear_prep_queue_by_class() | CRITICAL |
| ☑ | db.c:5349 (free_char) | Use-after-free in affect_total_plus() | Reading 18KB inside freed 90KB block | CRITICAL |
| ☑ | spell_prep.c:91 | Use-after-free in destroy_spell_prep_queue() | Reading 51KB inside freed block | CRITICAL |
| ☑ | helpers.c:47 (half_chop) | String overlap errors | Source/destination overlap in strcpy() - 2 instances | HIGH |

#### 💧 Memory Leaks (Total: 26,795 bytes definitely lost + 17,392 bytes indirectly lost)

##### DG Scripts System Leaks
| ☐ | Location | Issue | Details | Priority |
|---|----------|-------|---------|----------|
| ☐ | dg_scripts.c:2990 | script_driver() leaks in mob commands | Memory allocated during script execution not freed | HIGH |
| ☐ | dg_scripts.c:2996 | script_driver() leaks in world commands | Memory allocated during wld_command_interpreter not freed | HIGH |
| ☐ | dg_variables.c:56,62,65 | Script variables not freed | add_var() allocations persist in multiple contexts | MEDIUM |
| ☐ | dg_db_scripts.c:133 | trig_data_copy() strdup leaks | Trigger name strings not freed | MEDIUM |
| ☐ | dg_handler.c:266,285 | copy_proto_script() leaks | Script prototype copying allocations | MEDIUM |

##### String Handling Leaks
| ☐ | Location | Issue | Count | Priority |
|---|----------|-------|-------|----------|
| ☑ | handler.c:134 (isname) | strdup() in object searches | Multiple 5-7 byte leaks | HIGH |
| ☑ | spell_parser.c:3055 (spello) | Spell wear-off messages | ~250 instances, 30-40 bytes each | MEDIUM |
| ☐ | db.c:4031 (read_object) | Object name strings | Multiple 6 byte leaks | MEDIUM |
| ☐ | players.c:877 (load_char) | Character loading strings | 7 bytes per load | MEDIUM |
| ☐ | account.c:402 (load_account) | Account name strdup | 2 bytes per account load | LOW |

##### System/Module Leaks
| ☐ | Location | Issue | Size | Priority |
|---|----------|-------|------|----------|
| ☑ | db.c:4025 (read_object) | Object structure allocation | 48 bytes per object, accumulates during zone resets | HIGH |
| ☑ | crafting_new.c:2319-2321 | reset_current_craft() | 8 bytes x 3 allocations | MEDIUM |
| ☐ | assign_wpn_armor.c:922 | setweapon() name strings | 4 bytes each, multiple | LOW |
| ☐ | race.c:186-189 | add_race() string allocations | 4 bytes each, multiple | LOW |
| ☐ | class.c:201 | class_prereq_spellcasting() | 30-33 bytes each | LOW |

#### ⚠️ Uninitialized Values

| ☐ | Location | Issue | Count | Priority |
|---|----------|-------|-------|----------|
| ☑ | db.c:4941 | fread_clean_string() stack variables | 12 conditional jumps | HIGH |
| ☐ | ibt.c:149 | IBT file loading affected by above | Cascading from db.c | MEDIUM |

#### 📊 Summary Statistics
- **Total Errors**: 99 errors from 48 contexts
- **Definitely Lost**: 26,795 bytes in 911 blocks
- **Indirectly Lost**: 17,392 bytes in 92 blocks
- **Still Reachable**: 841,771,663 bytes (expected, not leaks)
- **Heap Usage**: 851,598 allocations, 86,486 frees

**Notes**:
- The CRITICAL issues cause actual crashes and must be fixed immediately
- String handling leaks are widespread but individually small
- DG Scripts system has systematic memory management issues
- Many "still reachable" allocations are one-time initializations (OK)
- **Zone Reset Accumulation**: The read_object() leaks at db.c:4025 are particularly concerning as they occur during zone resets, which happen periodically. This means the leaks accumulate over time and can eventually consume significant memory in long-running servers

---

## 🏗️ BUILDER TASKS (Fixable In-Game with OLC)

### Missing Triggers (12 total)

| ☐ | Trigger | Type | Affected Entity | Zone |
|---|---------|------|-----------------|------|
| ☐ | #2315 | Room | (room:-1) | Unknown |
| ☐ | #2314 | Mob | the Dark Knight | Unknown |
| ☐ | #2316 | Mob | the Dark Knight | Unknown |
| ☐ | #2310 | Mob | a giant mother spider | Unknown |
| ☐ | #2313 | Mob | a bat-like creature | Unknown |
| ☐ | #2308 | Obj | a jet black pearl | Unknown |
| ☐ | #2311 | Obj | a large stone chest | Unknown |
| ☐ | #2317 | Obj | Helm of Brilliance | Unknown |

### Zone File Corrections

| ☐ | Zone | Issue | Fix Required |
|---|------|-------|--------------|
| ☐ | #158 | Invalid object vnum 15802 in 'O' command (line 8) | Update to valid vnum |
| ☐ | #1481 | Invalid equipment position 148181 for High Priest of Grummsh | Use valid pos (0-21) |

### Quest Assignments (19 quests need questmasters)

| ☐ | Quest Vnum | Priority | Notes |
|---|------------|----------|-------|
| ☐ | #0 | HIGH | Quest 0 usually important |
| ☐ | #2011 | MEDIUM | |
| ☐ | #20309 | MEDIUM | |
| ☐ | #20315-20325 | LOW | Series of 11 quests |
| ☐ | #102412-102415 | LOW | Series of 4 quests |
| ☐ | #128102 | LOW | |

### Mob Script Fixes

| ☐ | Mob | Vnum | Issue |
|---|-----|------|-------|
| ☐ | Brother Spire | #200103 | Calling non-existing mob function |
| ☐ | Jakur the tanner | #125913 | Calling non-existing mob function |
| ☐ | Adoril | #21605 | Calling non-existing mob function |

### Missing Objects

| ☐ | Object Vnum | References | Action |
|---|-------------|------------|---------|
| ☐ | #19216 | 2 references during boot | Create object or remove refs |
| ☐ | #40252 | award_misc_magic_item() | Create object or fix award function |

---
