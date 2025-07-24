# LuminariMUD Development Task List

This document tracks ongoing development tasks, bug fixes, and improvements for the LuminariMUD project. Tasks are organized by priority and category to help contributors identify areas where help is needed.
---

## CODER TASKS

### 🚨 Critical Code Issues (Require Developer Attention)

#### Player Data Structure Access Violations
| ☐ | File | Line | Issue | Priority |
|---|------|------|-------|----------|
| ☐ | treasure.c | 1525 | Mobs accessing `((ch)->player_specials->saved.pref)` | HIGH |
| ☐ | spec_procs.c | 6315 | Mobs accessing `((vict)->player_specials->saved.pref)` | HIGH |
| ☐ | magic.c | 1284,1287,1290-1292,1307 | Mobs accessing `((ch)->player_specials->saved.psionic_energy_type)` | HIGH |
| ☐ | magic.c | 1250,1265,1268 | Mobs accessing `((ch)->player_specials->saved.psionic_energy_type)` | HIGH |

**Description**: Mobs are incorrectly trying to access player-specific data structures. This causes system errors and could lead to crashes or undefined behavior.

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

#### Performance Monitoring
| ☐ | System | Current Load | Action Required |
|---|--------|--------------|-----------------|
| ☐ | affect_update() | 28,795 chars processed | Monitor for performance degradation |

**Description**: System is processing a large number of characters and affects. Monitor for performance issues as player base grows.

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
