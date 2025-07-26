# LuminariMUD Builder Development Task List

This task list covers builder-related issues that require in-game fixes using OLC (Online Level Creation) commands.

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
