# LuminariMUD Development Task List

This document tracks ongoing development tasks, bug fixes, and improvements for the LuminariMUD project. Tasks are organized by priority and category to help contributors identify areas where help is needed.
---

## CODER TASKS

### Memory Leaks and Issues (From Valgrind Analysis - July 24, 2025)

Log file for reference if needed: valgrind_20250724_221634.md

#### Critical Memory Leaks (460KB total definitely lost)
| ☐ | Location | Issue | Size | Priority |
|---|----------|-------|------|----------|
| ☑ | hlquest.c:768, 769 | clear_hlquest() strdup without free | 113KB total | CRITICAL |
| ☑ | hlquest.c:925 | boot_the_quests() calloc without free | 15.7KB | HIGH |
| ☑ | mysql.c:211, 218 | tokenize() malloc/realloc leaks | 318KB total | CRITICAL |
| ☐ | db.c:4021 | read_object() object creation leaks | ~7KB | MEDIUM |
| ☐ | db.c:4004 | read_object() larger object leaks | 2.4KB | MEDIUM |
| ☐ | dg_variables.c:65 | Script variable memory not freed | Multiple small | LOW |

#### Uninitialized Values
| ☐ | Location | Issue | Errors | Priority |
|---|----------|-------|--------|----------|
| ☐ | db.c:4937, 4939 | fread_clean_string() uninitialized stack vars | 60 errors | MEDIUM |

#### Use-After-Free
| ☐ | Location | Issue | Details | Priority |
|---|----------|-------|---------|----------|
| ☐ | lists.c/mud_event.c | Accessing freed event memory | 8 bytes inside freed block | HIGH |

**Details**:
- **hlquest.c**: Major leaks in quest system - clear_hlquest() allocates strings with strdup (lines 768, 769) but never frees them. Called from boot_the_quests() at lines 901 and 914. Total: 9,840 + 10,496 + 44,970 + 47,952 = 113,258 bytes lost.
- **mysql.c**: tokenize() function has severe memory leaks from malloc (line 211) and realloc (line 218) operations. Affects load_paths() and other database operations. Total: ~318KB lost.
- **db.c:4937**: fread_clean_string() has uninitialized stack variables causing 60 conditional jump errors. Affects IBT file loading.
- **Use-after-free**: Event system accessing memory 8 bytes inside a 24-byte block that was freed by free_mud_event().

Log file for reference if needed: valgrind_20250724_221634.md

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
