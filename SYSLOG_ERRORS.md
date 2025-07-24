# LuminariMUD Error Analysis & Task Worklist
*ToDo list generated from our game SySlogs

## 🚨 EXECUTIVE SUMMARY

### Critical Issues Impacting Gameplay:
2. **Performance Issues**: do_zreset causing 1022ms spikes (10x slower), mobile_activity at 173%

### Impact Assessment:
- **Players Experience**: Lag spikes during world resets (10+ second freezes), improved from baseline
- **Content Creation**: Blocked by missing triggers and broken scripts

### Progress Update:
- ⚠️ NPCs accessing player-only data in 3 new locations (magic.c, utils.c, spec_procs.c)

---

## 📋 TASK WORKLIST BY RESPONSIBILITY

## 🔧 CODER TASKS (Requires Source Code Changes)

### PRIORITY 2: CRITICAL Performance Issues

| Task | Description | Current Impact | Target |
|------|-------------|----------------|---------|
| ☑ | Fix `mobile_activity()` bottleneck | ~~133-173% constant CPU~~ Optimized | <50% ✓ |
| ☐ | Reduce `do_gen_cast()` calls by NPCs | 111% (374 calls/pulse) | <50% |
| ☐ | Optimize `affect_update()` processing | 30% CPU constant | <10% |


### PRIORITY 5: LOW System Issues

| Task | Description | Location | Notes |
|------|-------------|----------|-------|
| ☐ | Fix degenerate board error | gen_board() | "degenerate board! (what the hell...)" |

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

## 📊 PERFORMANCE ANALYSIS DETAILS

### Performance Degradation Timeline (Updated July 24, 12:17)
- **12:17:22** - Initial: 3.57% (3.6ms) ✅ Normal
- **12:17:25** - Minor: 4.76% (4.8ms) ✅ Acceptable  
- **12:17:27** - Spike: 41.12% (41ms) ⚠️ Noticeable
- **12:17:28** - Major spike: 173.35% (173ms) 🔴 Laggy (mobile_activity: 133%)
- **12:17:30** - Login spike: 214.79% (214ms) 🔴
- **12:17:32** - Save spike: 257.08% (257ms) 🔴 (do_save: 257%)
- **12:17:37** - CRITICAL: 1022.25% (1022ms) 🚨 (do_zreset: 1022%)

### Top Performance Offenders (Updated)
1. ~~**do_zreset** - 1022% (world reset triggered by player)~~ ✅ FIXED
2. ~~**mobile_activity** - 133-173% (consistent high CPU usage)~~ ✅ FIXED
3. **do_gen_cast** - 111% with 374 calls per pulse
4. **do_save** - 257% (improved from 513%, but still high)
5. **affect_update** - 30% (many active effects)

---
