# LuminariMUD Development Task List

This document tracks ongoing development tasks, bug fixes, and improvements for the LuminariMUD project. Tasks are organized by priority and category to help contributors identify areas where help is needed.
---

## CODER TASKS

### 🚨 Critical Code Issues (Require Developer Attention)

#### Object Handling Errors
| ☐ | Issue | Frequency | Priority |
|---|-------|-----------|----------|
| ☐ | NULL object passed to obj_to_obj() | Multiple occurrences | HIGH |
| ☐ | Extraction counting mismatch | Multiple occurrences | MEDIUM |

**Description**: Object manipulation functions receiving NULL pointers or having counting issues during object extraction.

#### Missing Damage Types
| ☐ | Damage ID | File Reference | Priority |
|---|-----------|----------------|----------|
| ☐ | 1527 | Unknown | MEDIUM |
| ☐ | 1507 | Unknown | MEDIUM |

**Description**: Damage types are missing DAM_ definitions, which could cause combat calculation errors.

### Memory Leaks and Issues (From Valgrind Analysis - July 24, 2025)

#### Critical Memory Leaks
| ☐ | Location | Issue | Size | Priority |
|---|----------|-------|------|----------|
| ☑ | objsave.c:476,484 | Temp object not freed on MySQL error | 460KB total | CRITICAL |
| ☐ | lists.c:553 | Use-after-free in simple_list() iterator | N/A | CRITICAL |
| ☐ | db.c:4937 | Uninitialized values in fread_clean_string() | N/A | MEDIUM |
| ☐ | dg_variables.c:65 | Script variable memory not freed | Multiple small | LOW |

**Details**:
- **objsave.c**: In `objsave_save_obj_record_db()`, when MySQL operations fail, the function returns without calling `extract_obj(temp)`. Fix: Add cleanup before returns at lines 476 and 484.
- **lists.c**: Iterator retains stale pointers after list modifications. Address: 0xbbbbbbbbbbbbbbcb indicates freed memory access.
- **db.c**: Valgrind reports conditional jumps on uninitialized values (may be false positive).
- **Total memory leaked**: 460,218 bytes in 11,147 blocks
- **Source**: Analysis from valgrind log `valgrind_20250724_210758.log`

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
