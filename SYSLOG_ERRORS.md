# LuminariMUD Error Analysis & Task Worklist
*Generated from SYSLOG_ERRORS.md - July 24, 2025*

## 🚨 EXECUTIVE SUMMARY

### Critical Issues Impacting Gameplay:
1. **Database Schema Errors**: ✅ FIXED - Missing 'idnum' column was preventing player saves and house data loading
2. **Severe Performance Degradation**: Game pulses taking 740ms instead of 100ms (7.4x slower)
3. **NPC System Errors**: Mobs accessing player-only functions causing potential crashes

### Impact Assessment:
- **Players Experience**: Severe lag, lost items, unable to save progress
- **Server Stability**: At risk due to performance issues and code safety problems
- **Content Creation**: Blocked by missing triggers and broken scripts

---

## 📋 TASK WORKLIST BY RESPONSIBILITY

## 🔧 CODER TASKS (Requires Source Code Changes)

### PRIORITY 1: BLOCKER Issues (Fix Immediately)

| Task | Description | File/Location | Impact |
|------|-------------|---------------|---------|
| ✅ | Add 'idnum' column to `house_data` table | mysql.c / database schema | 10 errors on boot |
| ✅ | Add 'idnum' column to `player_save_objs` table | mysql.c / database schema | Blocks player item saves |

### PRIORITY 2: CRITICAL Performance Issues

| Task | Description | Current Impact | Target |
|------|-------------|----------------|---------|
| ☐ | Optimize `Crash_save_all()` - convert to async/incremental | 445% CPU spike | <50% |
| ☐ | Fix `mobile_activity()` bottleneck | 158-182% constant CPU | <50% |
| ☐ | Reduce `do_gen_cast()` calls by NPCs | 300-400 calls/pulse | <50 calls |
| ☐ | Optimize `affect_update()` processing | 30-40% CPU constant | <10% |
| ☐ | Fix `do_save()` performance with DB errors | 285-513% CPU on login | <50% |

### PRIORITY 3: HIGH Code Safety Issues

| Task | Description | Location | Error Count |
|------|-------------|----------|-------------|
| ☐ | Add IS_NPC() check before PRF_FLAGGED | act.informative.c:845 | 3 errors |
| ☐ | Fix combat targeting dead/corpse validation | fight.c | 3 errors |
| ☐ | Implement or remove `award_magic_item()` calls | Zone reset #77 | 2 errors |

### PRIORITY 4: MEDIUM Spec Proc Issues

| Task | Description | Missing Vnums |
|------|-------------|---------------|
| ☐ | Remove invalid mob spec assignments | #103802, #103803 |
| ☐ | Remove invalid obj spec assignments | #139203, #120010, #100513, #111507, #100599 |

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

---

## 📊 PERFORMANCE ANALYSIS DETAILS

### Performance Degradation Timeline
- **09:32:46** - Initial: 2.50% (2.5ms) ✅ Normal
- **09:32:51** - Minor: 8.82% (8.8ms) ✅ Acceptable  
- **09:32:52** - Major spike: 197.76% (197ms) ⚠️ Laggy
- **09:32:58** - Sustained: 208.24% (208ms) ⚠️ 
- **09:33:08** - Login spike: 286.21% (286ms) 🔴
- **09:33:20** - Another login: 305.94% (306ms) 🔴
- **09:33:43** - Degrading: 376.66% (377ms) 🔴
- **09:33:50** - Severe: 514.06% (514ms) 🔴
- **09:34:47** - CRITICAL: 740.24% (740ms) 🚨

### Top Performance Offenders
1. **Crash_save_all** - 445.62% (needs async rewrite)
2. **mobile_activity** - 158-182% (too many NPCs or inefficient loops)
3. **do_gen_cast** - 125-154% (NPCs casting too frequently)
4. **do_save** - 285-513% (failing due to DB errors)

---

## 📈 PROGRESS TRACKING

### Summary Statistics
- **Total Tasks**: 55 (2 completed ✅, 53 remaining)
- **Coder Tasks**: 17 (2 completed, 15 remaining)
- **Builder Tasks**: 38 (all can be done in-game)

### Estimated Time to Resolution
- **Critical DB fixes**: ✅ COMPLETED
- **Performance fixes**: 2-3 days of profiling and optimization
- **Builder fixes**: 1-2 days with experienced builders

### Recently Completed (July 24, 2025)
- ✅ Database schema fixes: Added 'idnum' column to both `house_data` and `player_save_objs` tables in production and development databases

---

## 📝 ORIGINAL BOOT LOG DATA

<details>
<summary>Click to expand full boot sequence log</summary>

autorun starting game Thu Jul 24 09:32:14 UTC 2025
running bin/circle -q 4101
nohup: ignoring input
Jul 24 09:32:14 :: Loading configuration.
Jul 24 09:32:14 :: LuminariMUD 2.4839 (tbaMUD 3.64)
Make time: Thu Jul 24 09:08:37 UTC 2025
Make user: zusuk
Make host: None
Branch: * master
Parent: 9b06b2dc784d9014fd93dec1f5e5457162129f93

Jul 24 09:32:14 :: Using lib as data directory.
Jul 24 09:32:14 :: Running game on port 4101.
Jul 24 09:32:14 :: Finding player limit.
Jul 24 09:32:14 ::    Setting player limit to 300 using rlimit.
Jul 24 09:32:14 :: Opening mother connection.
Jul 24 09:32:14 :: Binding to all IP interfaces on this host.
Jul 24 09:32:14 :: Boot db -- BEGIN.
Jul 24 09:32:14 :: Resetting the game time:
Jul 24 09:32:14 ::    Current Gametime: 20H 2D 11M 1004Y.
Jul 24 09:32:14 :: Initialize Global / Group Lists
Jul 24 09:32:14 :: Initializing Events
Jul 24 09:32:14 :: Reading news, credits, help, ihelp, bground, info & motds.
Jul 24 09:32:14 :: Loading spell definitions.
Jul 24 09:32:14 :: Loading weapon and armor special ability definitions.
Jul 24 09:32:14 :: SUCCESS: Connected to MySQL database 'luminari_muddev' on host 'localhost' as user 'luminari_mud'
Jul 24 09:32:14 :: Loading zone table.
Jul 24 09:32:14 ::    514 zones, 45232 bytes.
Jul 24 09:32:14 :: Loading triggers and generating index.
Jul 24 09:32:14 :: Loading rooms.
Jul 24 09:32:14 ::    50233 rooms, 11654056 bytes.
Jul 24 09:32:14 :: SYSERR: dg_read_trigger: Trigger vnum #2315 asked for but non-existant! (room:-1)
Jul 24 09:32:15 :: Loading regions. (MySQL)
Jul 24 09:32:15 :: INFO: Loading region data from MySQL
Jul 24 09:32:15 ::  adding event for vnum 1000012
Jul 24 09:32:15 :: TEST DEBUG REGION EVENTS: vnum 1000012 rnum 11
Jul 24 09:32:15 :: Loading paths. (MySQL)
Jul 24 09:32:15 :: INFO: Loading path data from MySQL
Jul 24 09:32:15 :: Renumbering rooms.
Jul 24 09:32:15 :: Checking start rooms.
Jul 24 09:32:15 :: Loading mobs and generating index.
Jul 24 09:32:15 ::    14563 mobs, 466016 bytes in index, 260502944 bytes in prototypes.
Jul 24 09:32:16 :: SYSERR: dg_read_trigger: Trigger vnum #2314 asked for but non-existant! (mob: the Dark Knight - -1)
Jul 24 09:32:16 :: SYSERR: dg_read_trigger: Trigger vnum #2316 asked for but non-existant! (mob: the Dark Knight - -1)
Jul 24 09:32:16 :: SYSERR: dg_read_trigger: Trigger vnum #2310 asked for but non-existant! (mob: a giant mother spider - -1)
Jul 24 09:32:16 :: SYSERR: dg_read_trigger: Trigger vnum #2313 asked for but non-existant! (mob: a bat-like creature - -1)
Jul 24 09:32:42 :: Loading objs and generating index.
Jul 24 09:32:42 ::    12160 objs, 389120 bytes in index, 7587840 bytes in prototypes.
Jul 24 09:32:42 :: SYSERR: Trigger vnum #2308 asked for but non-existant! (Object: a jet black pearl - -1)
Jul 24 09:32:42 :: SYSERR: Trigger vnum #2311 asked for but non-existant! (Object: a large stone chest - -1)
Jul 24 09:32:42 :: SYSERR: Trigger vnum #2317 asked for but non-existant! (Object: Helm of Brilliance - -1)
Jul 24 09:32:42 :: Renumbering zone table.
Jul 24 09:32:42 :: SYSERR: zone file: Invalid vnum 15802, cmd disabled
Jul 24 09:32:42 :: SYSERR: ...offending cmd: 'O' cmd in zone #158, line 8
Jul 24 09:32:42 :: Loading shops.
Jul 24 09:32:42 :: Placing Harvesting Nodes
Jul 24 09:32:42 :: Loading quests.
Jul 24 09:32:42 ::    169 entries, 29744 bytes.
Jul 24 09:32:42 :: Loading Homeland quests.
Jul 24 09:32:42 :: Loading Deities
Jul 24 09:32:42 :: Loading Domains.
Jul 24 09:32:42 :: Loading Weapons.
Jul 24 09:32:42 :: Loading Armor.
Jul 24 09:32:42 :: Loading Extended Races
Jul 24 09:32:42 :: Loading feats.
Jul 24 09:32:42 :: Loading Class List
Jul 24 09:32:42 :: Loading evolutions.
Jul 24 09:32:42 :: Object (V) 19216 does not exist in database.
Jul 24 09:32:42 :: Object (V) 19216 does not exist in database.
Jul 24 09:32:42 :: Initializing perlin noise generator.
Jul 24 09:32:42 :: Indexing wilderness rooms.
Jul 24 09:32:42 :: Writing wilderness map image.
Jul 24 09:32:42 :: Loading help entries.
Jul 24 09:32:42 ::    3279 entries, 104928 bytes.
Jul 24 09:32:42 :: Generating player index.
Jul 24 09:32:42 :: Loading fight messages.
Jul 24 09:32:42 :: Loaded 163 Combat Messages...
Jul 24 09:32:42 :: Loading social messages.
Jul 24 09:32:42 :: Social table contains 501 socials.
Jul 24 09:32:42 :: Building command list.
Jul 24 09:32:42 :: Command info rebuilt, 1248 total commands.
Jul 24 09:32:42 :: Assigning function pointers:
Jul 24 09:32:42 ::    Mobiles.
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant mob #103802
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant mob #103803
Jul 24 09:32:42 ::    Shopkeepers.
Jul 24 09:32:42 ::    Objects.
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant obj #139203
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant obj #120010
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant obj #100513
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant obj #111507
Jul 24 09:32:42 :: SYSERR: Attempt to assign spec to non-existant obj #100599
Jul 24 09:32:42 ::    Rooms.
Jul 24 09:32:42 ::    Questmasters.
Jul 24 09:32:42 :: SYSERR: Quest #0 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #2011 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20309 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20315 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20316 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20317 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20318 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20319 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20320 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20321 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20322 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20323 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20324 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #20325 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #102412 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #102413 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #102414 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #102415 has no questmaster specified.
Jul 24 09:32:42 :: SYSERR: Quest #128102 has no questmaster specified.
Jul 24 09:32:42 :: Assigning spell and skill levels.
Jul 24 09:32:42 :: Sorting command list...
Jul 24 09:32:42 :: Sorting spells/skills...
Jul 24 09:32:42 :: Booting mail system.
Jul 24 09:32:42 ::    Mail file read -- 46 messages.
Jul 24 09:32:42 :: Reading banned site and invalid-name list.
Jul 24 09:32:42 :: Loading Ideas.
Jul 24 09:32:42 :: Loading Bugs.
Jul 24 09:32:42 :: Loading Typos.
Jul 24 09:32:42 :: Loading random encounter tables.
Jul 24 09:32:42 :: Loading hunts table.
Jul 24 09:32:42 :: Spawning hunts for the first time this boot.
Jul 24 09:32:42 :: Booting crafts.
Jul 24 09:32:42 :: Booting clans.
Jul 24 09:32:42 :: Loading clan zone claim info.
Jul 24 09:32:42 ::    Clan file does not exist. Will create a new one
Jul 24 09:32:42 :: Cleaning up last log.
Jul 24 09:32:42 :: Resetting #0: <*> Builder Academy Zone (rooms 0-99).
Jul 24 09:32:42 :: Resetting #1: <*> Sanctus (rooms 100-199).
Jul 24 09:32:42 :: Resetting #2: <*> Sanctus II (rooms 200-299).
Jul 24 09:32:42 :: Resetting #3: <*> Sanctus III (rooms 300-399).
Jul 24 09:32:42 :: Resetting #4: Jade Forest (rooms 400-499).
Jul 24 09:32:42 :: Resetting #5: Newbie Farm (rooms 500-599).
Jul 24 09:32:42 :: Resetting #6: Sea of Souls (rooms 600-699).
Jul 24 09:32:42 :: Resetting #7: Camelot (rooms 700-799).
Jul 24 09:32:42 :: Resetting #8: <*> Houses / Internal (rooms 800-899).
Jul 24 09:32:42 :: Resetting #9: River Island of Minos (rooms 900-999).
Jul 24 09:32:42 :: Resetting #10: Orme's Script Test Zone (rooms 1000-1099).
Jul 24 09:32:42 :: Resetting #11: Frozen Castle (rooms 1100-1199).
Jul 24 09:32:42 :: Resetting #12: <*> Staff Simplex (rooms 1200-1299).
Jul 24 09:32:42 :: Resetting #13: <*> Luminari Examples (rooms 1300-1399).
Jul 24 09:32:42 :: Resetting #14: <*> Luminari Examples II (rooms 1400-1499).
Jul 24 09:32:42 :: Resetting #15: Straight Path (rooms 1500-1599).
Jul 24 09:32:42 :: Resetting #16: Camelot II (rooms 1600-1699).
Jul 24 09:32:42 :: Resetting #17: Camelot III (rooms 1700-1799).
Jul 24 09:32:42 :: Resetting #18: War Torn Wasteland (rooms 1800-1899).
Jul 24 09:32:42 :: Resetting #19: Spider Swamp (rooms 1900-1999).
Jul 24 09:32:42 :: Resetting #20: Untitled (Arena) (rooms 2000-2099).
Jul 24 09:32:42 :: Resetting #21: Hobbiton (rooms 2100-2199).
Jul 24 09:32:42 :: Resetting #22: Tower of the Undead (rooms 2200-2299).
Jul 24 09:32:42 :: Resetting #23: 3.5E testing zone (rooms 2300-2399).
Jul 24 09:32:42 :: Resetting #24: Acererak Zone (rooms 2400-2499).
Jul 24 09:32:42 :: Resetting #25: Shadow Grove (rooms 2500-2599).
Jul 24 09:32:42 :: Resetting #26: High Tower of Magic II (rooms 2600-2699).
Jul 24 09:32:42 :: Resetting #27: Memlin Caverns (rooms 2700-2799).
Jul 24 09:32:42 :: Resetting #28: Mudschool (rooms 2800-2899).
Jul 24 09:32:42 :: Resetting #29: Unname Zone (rooms 2900-2999).
Jul 24 09:32:42 :: Resetting #30: <*> Northern Midgen (rooms 3000-3099).
Jul 24 09:32:42 :: Resetting #31: <*> Southern Midgen (rooms 3100-3199).
Jul 24 09:32:42 :: Resetting #32: <*> Midgen (rooms 3200-3299).
Jul 24 09:32:42 :: Resetting #33: <*> Internal / Three of Swords (rooms 3300-3399).
Jul 24 09:32:42 :: Resetting #34: Nordmaar Keep (rooms 3400-3499).
Jul 24 09:32:42 :: Resetting #35: Miden'Nir (rooms 3500-3599).
Jul 24 09:32:42 :: Resetting #36: Chessboard of Midgen (rooms 3600-3699).
Jul 24 09:32:42 :: Resetting #37: Capital Sewer System (rooms 3700-3799).
Jul 24 09:32:42 :: Resetting #38: Capital Sewer System II (rooms 3800-3899).
Jul 24 09:32:42 :: Resetting #39: Haven (rooms 3900-3999).
Jul 24 09:32:42 :: Resetting #40: Mines of Moria (rooms 4000-4099).
Jul 24 09:32:42 :: Resetting #41: Mines of Moria (rooms 4100-4199).
Jul 24 09:32:42 :: Resetting #42: Dragon Chasm (rooms 4200-4299).
Jul 24 09:32:42 :: Resetting #43: Arctic Zone (rooms 4300-4399).
Jul 24 09:32:42 :: Resetting #44: Orc Camp (rooms 4400-4499).
Jul 24 09:32:42 :: Resetting #45: Woodland Monastery (rooms 4500-4599).
Jul 24 09:32:42 :: Resetting #46: Ant Hill (rooms 4600-4699).
Jul 24 09:32:42 :: Resetting #47: Valcrest I (rooms 4700-4799).
Jul 24 09:32:42 :: Resetting #48: Valcrest II (rooms 4800-4899).
Jul 24 09:32:42 :: Resetting #49: The Underworld (rooms 4900-4999).
Jul 24 09:32:42 :: Resetting #50: Great Eastern Desert (rooms 5000-5099).
Jul 24 09:32:42 :: Resetting #51: Drow City (rooms 5100-5199).
Jul 24 09:32:42 :: Resetting #52: City of Thalos (rooms 5200-5299).
Jul 24 09:32:42 :: Resetting #53: Great Pyramid (rooms 5300-5399).
Jul 24 09:32:42 :: Resetting #54: New Thalos (rooms 5400-5499).
Jul 24 09:32:42 :: Resetting #55: New Thalos II (rooms 5500-5599).
Jul 24 09:32:42 :: Resetting #56: New Thalos Wilderness (rooms 5600-5699).
Jul 24 09:32:42 :: Resetting #57: Untitled (Zodiac) (rooms 5700-5799).
Jul 24 09:32:42 :: Resetting #58: Cottage (rooms 5800-5899).
Jul 24 09:32:42 :: Resetting #59: Wizard Training Mansion (rooms 5900-5999).
Jul 24 09:32:42 :: Resetting #60: Haon-Dor, Light Forest (rooms 6000-6099).
Jul 24 09:32:42 :: Resetting #61: Haon-Dor, Light Forest II (rooms 6100-6199).
Jul 24 09:32:42 :: Resetting #62: Orc Enclave (rooms 6200-6299).
Jul 24 09:32:42 :: Resetting #63: Arachnos (rooms 6300-6399).
Jul 24 09:32:42 :: Resetting #64: Rand's Tower (rooms 6400-6499).
Jul 24 09:32:42 :: Resetting #65: Dwarven Kingdom (rooms 6500-6599).
Jul 24 09:32:42 :: Resetting #66: Faedon Forest (rooms 6600-6699).
Jul 24 09:32:42 :: Resetting #67: Graven Hollow Camp (rooms 6700-6799).
Jul 24 09:32:42 :: Resetting #68: New Zone for Anson (rooms 6800-6899).
Jul 24 09:32:42 :: Resetting #69: The Giant Darkwood Tree (rooms 6900-6999).
Jul 24 09:32:42 :: Resetting #70: Sewer, First Level (rooms 7000-7099).
Jul 24 09:32:42 :: Resetting #71: Second Sewer (rooms 7100-7199).
Jul 24 09:32:42 :: Resetting #72: Sewer Maze (rooms 7200-7299).
Jul 24 09:32:42 :: Resetting #73: Tunnels in the Sewer (rooms 7300-7399).
Jul 24 09:32:42 :: Resetting #74: Newbie Graveyard (rooms 7400-7499).
Jul 24 09:32:42 :: Resetting #75: Zamba (rooms 7500-7599).
Jul 24 09:32:42 :: Resetting #76: Grandmother's House (rooms 7600-7699).
Jul 24 09:32:42 :: Resetting #77: The Crystal Swamp (rooms 7700-7799).
Jul 24 09:32:42 :: award_magic_item called but no crafting system is defined
Jul 24 09:32:42 :: award_magic_item called but no crafting system is defined
Jul 24 09:32:42 :: Resetting #78: Gideon (rooms 7800-7899).
Jul 24 09:32:42 :: Resetting #79: Redferne's Residence (rooms 7900-7999).
Jul 24 09:32:42 :: Resetting #80: Maze of Madness (rooms 8000-8099).
Jul 24 09:32:42 :: Resetting #81: <*> Common Items/Mobs (rooms 8100-8299).
Jul 24 09:32:42 :: Resetting #83: Tomb of Horrors (rooms 8300-8399).
Jul 24 09:32:42 :: Resetting #84: Isis' New Zone (rooms 8400-8499).
Jul 24 09:32:42 :: Resetting #85: City of Delo (rooms 8500-8599).
Jul 24 09:32:42 :: Resetting #86: Duke Kalithorn's Keep (rooms 8600-8699).
Jul 24 09:32:42 :: Resetting #87: Glumgold's Sea (rooms 8700-8799).
Jul 24 09:32:42 :: Resetting #88: New Zone for Rosealee (rooms 8800-8899).
Jul 24 09:32:42 :: Resetting #89: Semada's unfinished zone (rooms 8900-8999).
Jul 24 09:32:42 :: Resetting #90: Oasis (rooms 9000-9099).
Jul 24 09:32:42 :: Resetting #91: The Orphanage (rooms 9100-9199).
Jul 24 09:32:42 :: Resetting #92: The Depths: Scorching Desert (rooms 9200-9299).
Jul 24 09:32:42 :: Resetting #93: New Zone for Ambanya (rooms 9300-9399).
Jul 24 09:32:42 :: Resetting #94: <*> Internal Use Only (rooms 9400-9599).
Jul 24 09:32:42 :: Resetting #96: Domiae (rooms 9600-9699).
Jul 24 09:32:42 :: Resetting #97: New Zone for Makiel (rooms 9700-9799).
Jul 24 09:32:42 :: Resetting #98: Radiant Heart Sanctuary (rooms 9800-9999).
Jul 24 09:32:42 :: Resetting #100: Northern Highway (rooms 10000-10099).
Jul 24 09:32:42 :: Resetting #101: Untitled (South Road) (rooms 10100-10199).
Jul 24 09:32:42 :: Resetting #102: Ratchet's Untitled Zone (rooms 10200-10299).
Jul 24 09:32:42 :: Resetting #103: Untitled (DBZ World) (rooms 10300-10399).
Jul 24 09:32:42 :: Resetting #104: Land of Orchan (rooms 10400-10499).
Jul 24 09:32:42 :: Resetting #105: The Pilgrimage of the Petitioners (rooms 10500-10599).
Jul 24 09:32:42 :: Resetting #106: Elcardo (rooms 10600-10699).
Jul 24 09:32:42 :: Resetting #107: Realms of Iuel (rooms 10700-10799).
Jul 24 09:32:42 :: Resetting #108: Wilderness Encounters (rooms 10800-10899).
Jul 24 09:32:42 :: Resetting #109: Hidden Campgrounds (rooms 10900-10999).
Jul 24 09:32:42 :: Resetting #110: Crystal City (rooms 11000-11099).
Jul 24 09:32:42 :: Resetting #111: Descent into Hell (rooms 11100-11199).
Jul 24 09:32:42 :: Resetting #113: The Jumping Slug (rooms 11300-11399).
Jul 24 09:32:42 :: Resetting #114: Glass Tower (rooms 11400-11499).
Jul 24 09:32:42 :: Resetting #115: Monestary Omega (rooms 11500-11599).
Jul 24 09:32:42 :: Resetting #116: Swinton Goldwater's Hideout (rooms 11600-11699).
Jul 24 09:32:42 :: Resetting #117: Los Torres (rooms 11700-11799).
Jul 24 09:32:42 :: Resetting #118: The Dollhouse (rooms 11800-11899).
Jul 24 09:32:42 :: Resetting #119: Arcanite Caves (rooms 11900-11999).
Jul 24 09:32:42 :: Resetting #120: Rome (rooms 12000-12099).
Jul 24 09:32:42 :: Resetting #121: Ant Caves (rooms 12100-12199).
Jul 24 09:32:42 :: Resetting #122: The Dreamscape (rooms 12200-12299).
Jul 24 09:32:42 :: Resetting #123: Rychus - New Zone (rooms 12300-12399).
Jul 24 09:32:42 :: Resetting #124: Bagoggle's Unnamed Zone (rooms 12400-12499).
Jul 24 09:32:42 :: Resetting #125: Hannah (rooms 12500-12599).
Jul 24 09:32:42 :: Resetting #126: Grammaton Fortress (rooms 12600-12699).
Jul 24 09:32:42 :: Resetting #128: English's Test Zone (rooms 12800-12899).
Jul 24 09:32:42 :: Resetting #130: Mist Maze (rooms 13000-13099).
Jul 24 09:32:42 :: Resetting #131: Quagmire I (rooms 13100-13199).
Jul 24 09:32:42 :: Resetting #132: Quagmire II (rooms 13200-13299).
Jul 24 09:32:42 :: Resetting #133: Quagmire III (rooms 13300-13399).
Jul 24 09:32:42 :: Resetting #134: Mad Scientist Lab (rooms 13400-13499).
Jul 24 09:32:42 :: Resetting #135: Dudris's Unnamed Zone (rooms 13500-13599).
Jul 24 09:32:42 :: Resetting #136: Unnamed (rooms 13600-13699).
Jul 24 09:32:42 :: Resetting #137: Plains of Elysium (rooms 13700-13799).
Jul 24 09:32:42 :: Resetting #138: Out Riders Fortress (rooms 13800-13899).
Jul 24 09:32:42 :: Resetting #139: Unnamed (rooms 13900-13999).
Jul 24 09:32:42 :: Resetting #140: Wyvern City (rooms 14000-14099).
Jul 24 09:32:42 :: Resetting #141: Training Halls (rooms 14100-14599).
Jul 24 09:32:42 :: Resetting #143: reserved for training hall expansion (rooms 14300-14399).
Jul 24 09:32:42 :: Resetting #145: reserved for training hall expansion (rooms 14500-14599).
Jul 24 09:32:42 :: Resetting #147: Jithor's Unnamed Zone (rooms 14700-14799).
Jul 24 09:32:42 :: Resetting #148: Jharkendar walley (rooms 14800-14899).
Jul 24 09:32:42 :: Resetting #150: King Welmar's Castle (rooms 15000-15099).
Jul 24 09:32:42 :: Resetting #152: Azagh New Zone (rooms 15200-15299).
Jul 24 09:32:42 :: Resetting #154: Eastern Village (rooms 15400-15499).
Jul 24 09:32:42 :: Resetting #156: Cade's Unfinished Zone (rooms 15600-15699).
Jul 24 09:32:42 :: Resetting #158: Temple of Solomon (rooms 15800-15899).
Jul 24 09:32:42 :: Resetting #160: New Zone (rooms 16000-16099).
Jul 24 09:32:42 :: Resetting #162: New Zone (rooms 16200-16299).
Jul 24 09:32:42 :: Resetting #165: Tyrion's Revenge (rooms 16500-16599).
Jul 24 09:32:42 :: Resetting #167: The Silver Lining Inn (rooms 16700-16799).
Jul 24 09:32:42 :: Resetting #169: Gibberling Caves (rooms 16900-16999).
Jul 24 09:32:42 :: Resetting #172: New Zone - Darvin (rooms 17200-17299).
Jul 24 09:32:42 :: Resetting #175: Cardinal Wizards (rooms 17500-17599).
Jul 24 09:32:42 :: Resetting #177: New UnNamed Zone (rooms 17700-17799).
Jul 24 09:32:42 :: Resetting #180: Ornir Test Zone (rooms 18000-18099).
Jul 24 09:32:42 :: Resetting #183: New Zone (rooms 18300-18399).
Jul 24 09:32:42 :: Resetting #186: Newbie Zone (rooms 18600-18699).
Jul 24 09:32:42 :: Resetting #187: Circus (rooms 18700-18799).
Jul 24 09:32:42 :: Resetting #188: New Zone (rooms 18800-18899).
Jul 24 09:32:42 :: Resetting #190: The Depths: Living Forest (rooms 19000-19999).
Jul 24 09:32:42 :: Resetting #192: Training Halls (rooms 19200-19399).
Jul 24 09:32:42 :: Resetting #200: Western Highway (rooms 20000-20099).
Jul 24 09:32:42 :: Resetting #201: Sapphire Islands (rooms 20100-20199).
Jul 24 09:32:42 :: Resetting #203: Fernbrook Farm (rooms 20300-20399).
Jul 24 09:32:42 :: Resetting #204: Blackstone Keep (rooms 20400-20599).
Jul 24 09:32:42 :: Resetting #206: Coral Cove: Open Seas (rooms 20600-20699).
Jul 24 09:32:42 :: Resetting #207: Coral Cove: Coral Reef (rooms 20700-20899).
Jul 24 09:32:42 :: Resetting #210: The Onyx Obelisk (rooms 21000-21399).
Jul 24 09:32:42 :: Resetting #216: Training Halls (rooms 21600-21899).
Jul 24 09:32:42 :: Resetting #220: (Enchanted Kitchen) (rooms 22000-22099).
Jul 24 09:32:42 :: Resetting #222: The Grumblecakery (rooms 22200-22299).
Jul 24 09:32:42 :: Resetting #224: New Zone (rooms 22400-22499).
Jul 24 09:32:42 :: Resetting #232: Terringham (rooms 23200-23299).
Jul 24 09:32:42 :: Resetting #233: Dragon Plains (rooms 23300-23399).
Jul 24 09:32:42 :: Resetting #234: Newbie School (rooms 23400-23499).
Jul 24 09:32:42 :: Resetting #235: Dwarven Mines (rooms 23500-23599).
Jul 24 09:32:42 :: Resetting #236: Aldin (rooms 23600-23699).
Jul 24 09:32:42 :: Resetting #237: Dwarven Trade Route (rooms 23700-23799).
Jul 24 09:32:42 :: Resetting #238: Crystal Castle (rooms 23800-23899).
Jul 24 09:32:42 :: Resetting #239: South Pass (rooms 23900-23999).
Jul 24 09:32:42 :: Resetting #240: Dun Maura (rooms 24000-24099).
Jul 24 09:32:42 :: Resetting #241: (Starship Enterprise) (rooms 24100-24199).
Jul 24 09:32:42 :: Resetting #242: New Southern Midgen (rooms 24200-24299).
Jul 24 09:32:42 :: Resetting #243: Snowy Valley (rooms 24300-24399).
Jul 24 09:32:42 :: Resetting #244: Cooland Prison (rooms 24400-24499).
Jul 24 09:32:42 :: Resetting #245: The Nether (rooms 24500-24599).
Jul 24 09:32:42 :: Resetting #246: The Nether II (rooms 24600-24699).
Jul 24 09:32:42 :: Resetting #247: Graveyard (rooms 24700-24799).
Jul 24 09:32:42 :: Resetting #248: Elven Woods (rooms 24800-24899).
Jul 24 09:32:42 :: Resetting #249: Untitled (Jedi Clan House) (rooms 24900-24999).
Jul 24 09:32:42 :: Resetting #250: DragonSpyre (rooms 25000-25099).
Jul 24 09:32:42 :: Resetting #251: Ape Village (rooms 25100-25199).
Jul 24 09:32:42 :: Resetting #252: Castle of the Vampyre (rooms 25200-25299).
Jul 24 09:32:42 :: Resetting #253: Windmill (rooms 25300-25399).
Jul 24 09:32:42 :: Resetting #254: Mordecai's Village (rooms 25400-25499).
Jul 24 09:32:42 :: Resetting #255: Shipwreck (rooms 25500-25599).
Jul 24 09:32:42 :: Resetting #256: Lord's Keep (rooms 25600-25699).
Jul 24 09:32:42 :: Resetting #257: Jareth Main City (rooms 25700-25799).
Jul 24 09:32:42 :: Resetting #258: Light Forest (rooms 25800-25899).
Jul 24 09:32:42 :: Resetting #259: Haunted Mansion (rooms 25900-25999).
Jul 24 09:32:42 :: Resetting #260: Grasslands (rooms 26000-26099).
Jul 24 09:32:42 :: Resetting #261: Inna & Igor's Castle (rooms 26100-26199).
Jul 24 09:32:42 :: Resetting #262: Forest Trails (rooms 26200-26299).
Jul 24 09:32:42 :: Resetting #263: Farmlands (rooms 26300-26399).
Jul 24 09:32:42 :: Resetting #264: Banshide (rooms 26400-26499).
Jul 24 09:32:42 :: Resetting #265: Beach & Lighthouse (rooms 26500-26599).
Jul 24 09:32:42 :: Resetting #266: Realm of Lord Ankou (rooms 26600-26699).
Jul 24 09:32:42 :: Resetting #267: Vice Island (rooms 26700-26799).
Jul 24 09:32:42 :: Resetting #268: Vice Island II (rooms 26800-26899).
Jul 24 09:32:42 :: Resetting #269: Southern Desert (rooms 26900-26999).
Jul 24 09:32:42 :: Resetting #270: Wasteland (rooms 27000-27099).
Jul 24 09:32:42 :: Resetting #271: Sundhaven (rooms 27100-27199).
Jul 24 09:32:43 :: Resetting #272: Sundhaven II (rooms 27200-27299).
Jul 24 09:32:43 :: Resetting #273: (Space Station Alpha) (rooms 27300-27399).
Jul 24 09:32:43 :: Resetting #274: Portablo (Smurfville) (rooms 27400-27499).
Jul 24 09:32:43 :: Resetting #275: New Sparta (rooms 27500-27599).
Jul 24 09:32:43 :: Resetting #276: New Sparta II (rooms 27600-27699).
Jul 24 09:32:43 :: Resetting #277: Shire (rooms 27700-27799).
Jul 24 09:32:43 :: Resetting #278: Oceania (rooms 27800-27899).
Jul 24 09:32:43 :: Resetting #279: Notre Dame (rooms 27900-27999).
Jul 24 09:32:43 :: Resetting #280: (Living Motherboard) (rooms 28000-28099).
Jul 24 09:32:43 :: Resetting #281: Forest of Khanjar (rooms 28100-28199).
Jul 24 09:32:43 :: Resetting #282: Infernal (rooms 28200-28299).
Jul 24 09:32:43 :: Resetting #283: Haunted House (rooms 28300-28399).
Jul 24 09:32:43 :: Resetting #284: Ghenna (rooms 28400-28499).
Jul 24 09:32:43 :: Resetting #285: Descent to Hell II (rooms 28500-28599).
Jul 24 09:32:43 :: Resetting #286: Descent to Hell (rooms 28600-28699).
Jul 24 09:32:43 :: Resetting #287: Ofingia / Goblin Town (rooms 28700-28799).
Jul 24 09:32:43 :: Resetting #288: Galaxy (rooms 28800-28899).
Jul 24 09:32:43 :: Resetting #289: Werith's Wayhouse (rooms 28900-28999).
Jul 24 09:32:43 :: Resetting #290: Lizard Lair (rooms 29000-29099).
Jul 24 09:32:43 :: Resetting #291: Black Forest (rooms 29100-29199).
Jul 24 09:32:43 :: Resetting #292: Kerofk (rooms 29200-29299).
Jul 24 09:32:43 :: Resetting #293: Kerofk II (rooms 29300-29399).
Jul 24 09:32:43 :: Resetting #294: Trade Road (rooms 29400-29499).
Jul 24 09:32:43 :: Resetting #295: Jungle (rooms 29500-29599).
Jul 24 09:32:43 :: Resetting #296: Froboz Fun Factory (rooms 29600-29699).
Jul 24 09:32:43 :: Resetting #298: Castle of Desire (rooms 29800-29899).
Jul 24 09:32:43 :: Resetting #299: Abandoned Cathedral (rooms 29900-29999).
Jul 24 09:32:43 :: Resetting #300: Ancalador (rooms 30000-30099).
Jul 24 09:32:43 :: Resetting #301: Campus (rooms 30100-30199).
Jul 24 09:32:43 :: Resetting #302: Campus II (rooms 30200-30299).
Jul 24 09:32:43 :: Resetting #303: Campus III (rooms 30300-30399).
Jul 24 09:32:43 :: Resetting #304: Temple of the Bull (rooms 30400-30499).
Jul 24 09:32:43 :: Resetting #305: Chessboard (rooms 30500-30599).
Jul 24 09:32:43 :: Resetting #306: Newbie Tree (rooms 30600-30699).
Jul 24 09:32:43 :: Resetting #307: Castle (rooms 30700-30799).
Jul 24 09:32:43 :: Resetting #308: Baron Cailveh (rooms 30800-30899).
Jul 24 09:32:43 :: Resetting #309: Keep of Baron Westlawn (rooms 30900-30999).
Jul 24 09:32:43 :: Resetting #310: Graye Area (rooms 31000-31099).
Jul 24 09:32:43 :: Resetting #311: The Dragon's Teeth (rooms 31100-31199).
Jul 24 09:32:43 :: Resetting #312: Leper Island (rooms 31200-31299).
Jul 24 09:32:43 :: Resetting #313: Farmlands of Ofingia (rooms 31300-31399).
Jul 24 09:32:43 :: Resetting #314: X'Raantra's Altar (rooms 31400-31499).
Jul 24 09:32:43 :: Resetting #315: McGintey Business District (rooms 31500-31599).
Jul 24 09:32:43 :: Resetting #316: McGintey Guild Area (rooms 31600-31699).
Jul 24 09:32:43 :: Resetting #317: Wharf (rooms 31700-31799).
Jul 24 09:32:43 :: Resetting #318: Dock Area (rooms 31800-31899).
Jul 24 09:32:43 :: Resetting #319: Yllythad Sea (rooms 31900-31999).
Jul 24 09:32:43 :: Resetting #320: Yllythad Sea II (rooms 32000-32099).
Jul 24 09:32:43 :: Resetting #321: Yllythad Sea III (rooms 32100-32199).
Jul 24 09:32:43 :: Resetting #322: McGintey Bay (rooms 32200-32299).
Jul 24 09:32:43 :: Resetting #323: Caverns of the Pale Man (rooms 32300-32399).
Jul 24 09:32:43 :: Resetting #324: Army Encampment (rooms 32400-32499).
Jul 24 09:32:43 :: Resetting #325: Revelry (rooms 32500-32599).
Jul 24 09:32:43 :: Resetting #326: Army Perimeter (rooms 32600-32699).
Jul 24 09:32:43 :: Resetting #328: Anvil's Boreal Forest (rooms 32800-32899).
Jul 24 09:32:43 :: Resetting #330: Dwarven Royal Chambers (rooms 33000-33099).
Jul 24 09:32:43 :: Resetting #345: Fire Giant Keep (rooms 34500-34699).
Jul 24 09:32:43 :: Resetting #347: A Ruined Farm (VQL) (rooms 34700-34799).
Jul 24 09:32:43 :: Resetting #348: New Zone (rooms 34800-34899).
Jul 24 09:32:43 :: Resetting #400: Astral Plane (rooms 40000-40099).
Jul 24 09:32:43 :: Resetting #401: Ethereal Plane (rooms 40100-40199).
Jul 24 09:32:43 :: Resetting #402: Elemental Plane (rooms 40200-40299).
Jul 24 09:32:43 :: Resetting #403: Mystery Warehouse (rooms 40300-40399).
Jul 24 09:32:43 :: Resetting #404: Blindbreak Rest (rooms 40400-40499).
Jul 24 09:32:43 :: Resetting #405: the Pirate Ship "Bloody Rum" (rooms 40500-40599).
Jul 24 09:32:43 :: Resetting #406: Mosaic Cave (rooms 40600-40699).
Jul 24 09:32:43 :: Resetting #407: Dark Darkling Crypt (rooms 40700-40799).
Jul 24 09:32:43 :: Resetting #408: Kayos Test Zone For Toril (rooms 40800-40899).
Jul 24 09:32:43 :: Resetting #555: Ultima (rooms 55500-55599).
Jul 24 09:32:43 :: Resetting #556: Ultima II (rooms 55600-55699).
Jul 24 09:32:43 :: Resetting #600: <*> Monster Manual [RESERVED Zones 600-623] (rooms 60000-62299).
Jul 24 09:32:43 :: Resetting #655: <*> Internal NOWHERE (rooms 65500-65534).
Jul 24 09:32:43 :: Resetting #666: <*> Internal NOWHERE (rooms 66600-66699).
Jul 24 09:32:43 :: Resetting #667: <*> RESERVED Transport Rooms (rooms 66700-66799).
Jul 24 09:32:43 :: Resetting #1001: <*> Internal Common Items (rooms 100100-100499).
Jul 24 09:32:43 :: Resetting #1005: <*> Random Encounter Distro (rooms 100500-100999).
Jul 24 09:32:43 :: Resetting #1010: <*> Quest Zone (rooms 101000-101199).
Jul 24 09:32:43 :: Resetting #1012: <*> Extended Staff Area (rooms 101200-101399).
Jul 24 09:32:43 :: Resetting #1014: <*> Spirit Sanctuary (rooms 101400-101499).
Jul 24 09:32:43 :: Resetting #1015: The Trade Way (rooms 101500-101699).
Jul 24 09:32:43 :: Resetting #1017: The Ruined Keep (rooms 101700-101799).
Jul 24 09:32:43 :: Resetting #1018: Dawn Pass & Lonely Moor (rooms 101800-101999).
Jul 24 09:32:43 :: Resetting #1020: Delimiyr Route (rooms 102000-102099).
Jul 24 09:32:43 :: Resetting #1021: The High Road - North (rooms 102100-102299).
Jul 24 09:32:43 :: Resetting #1023: The Long Road (rooms 102300-102399).
Jul 24 09:32:43 :: Resetting #1024: Skull Gorge (rooms 102400-102499).
Jul 24 09:32:43 :: Resetting #1025: Bloodfist Caverns (rooms 102500-102799).
Jul 24 09:32:43 :: Resetting #1030: Ashenport (rooms 103000-103899).
Jul 24 09:32:43 :: Resetting #1039: North of Ashenport (rooms 103900-103999).
Jul 24 09:32:43 :: Resetting #1040: The Coast Way (rooms 104000-104299).
Jul 24 09:32:43 :: Resetting #1043: <*> Mercenary Camps (rooms 104300-104399).
Jul 24 09:32:43 :: Resetting #1044: The Stag Forest (rooms 104400-104499).
Jul 24 09:32:43 :: Resetting #1045: Eastern Roads (rooms 104500-104699).
Jul 24 09:32:43 :: Resetting #1047: Bargewright Inn (rooms 104700-104899).
Jul 24 09:32:43 :: Resetting #1049: Amphail (rooms 104900-104999).
Jul 24 09:32:43 :: Resetting #1050: Corm Orp (rooms 105000-105199).
Jul 24 09:32:43 :: Resetting #1052: Corm Orp Caverns (rooms 105200-105299).
Jul 24 09:32:43 :: Resetting #1053: The Way Inn (rooms 105300-105399).
Jul 24 09:32:43 :: Resetting #1054: Eveningstar (rooms 105400-105599).
Jul 24 09:32:43 :: Resetting #1056: Gracklstugh (rooms 105600-105899).
Jul 24 09:32:43 :: Resetting #1060: Crimson Flame (rooms 106000-106199).
Jul 24 09:32:43 :: Resetting #1062: Orc Ruins (rooms 106200-106399).
Jul 24 09:32:43 :: Resetting #1064: Elven Settlement (rooms 106400-106599).
Jul 24 09:32:43 :: Resetting #1066: Red Larch (rooms 106600-106699).
Jul 24 09:32:43 :: Resetting #1067: Fire Giants (rooms 106700-106799).
Jul 24 09:32:43 :: Resetting #1068: Longsaddle (rooms 106800-106899).
Jul 24 09:32:43 :: Resetting #1070: A Dwarven Stronghold (rooms 107000-107099).
Jul 24 09:32:43 :: Resetting #1071: Gentle Oak Valley (rooms 107100-107199).
Jul 24 09:32:43 :: Resetting #1072: The Trade Way II (rooms 107200-107399).
Jul 24 09:32:43 :: Resetting #1075: Shadowdale (rooms 107500-107799).
Jul 24 09:32:43 :: Resetting #1078: Aumvor's Castle (rooms 107800-107999).
Jul 24 09:32:43 :: Resetting #1081: Mithril Hall (rooms 108100-108599).
Jul 24 09:32:43 :: Resetting #1086: The Dusk Road (rooms 108600-108699).
Jul 24 09:32:43 :: Resetting #1087: The High Road South (rooms 108700-108799).
Jul 24 09:32:43 :: Resetting #1088: The Moonsea Ride (rooms 108800-108899).
Jul 24 09:32:43 :: Resetting #1089: The North Ride (rooms 108900-108999).
Jul 24 09:32:43 :: Resetting #1090: The Evermoor Way (rooms 109000-109099).
Jul 24 09:32:43 :: Resetting #1091: The Rauvin Ride (rooms 109100-109199).
Jul 24 09:32:43 :: Resetting #1092: Ogre Lair (rooms 109200-109299).
Jul 24 09:32:43 :: Resetting #1094: Tesh Trail (rooms 109400-109499).
Jul 24 09:32:43 :: Resetting #1095: Dagger Falls (rooms 109500-109699).
Jul 24 09:32:43 :: Resetting #1097: The Neverwinter Wood (rooms 109700-109999).
Jul 24 09:32:43 :: Resetting #1100: The Tower of Twilight (rooms 110000-110099).
Jul 24 09:32:43 :: Resetting #1101: Ardeep Forest (rooms 110100-110299).
Jul 24 09:32:43 :: Resetting #1103: The Stump Bog (rooms 110300-110399).
Jul 24 09:32:43 :: Resetting #1104: Tugrahk Gol (rooms 110400-110499).
Jul 24 09:32:43 :: Resetting #1106: Lost City of Thunderholme (rooms 110600-110699).
Jul 24 09:32:43 :: Resetting #1107: Westwood (rooms 110700-110799).
Jul 24 09:32:43 :: Resetting #1108: Skull Crag (rooms 110800-110999).
Jul 24 09:32:43 :: Resetting #1110: Tethyamar Trail (rooms 111000-111099).
Jul 24 09:32:43 :: Resetting #1113: Tilverton (rooms 111300-111499).
Jul 24 09:32:43 :: Resetting #1115: Bleak Palace (rooms 111500-111699).
Jul 24 09:32:43 :: Resetting #1117: <*> Spirit Walk (rooms 111700-111899).
Jul 24 09:32:43 :: Resetting #1119: <*> Spirit Walk (rooms 111900-112099).
Jul 24 09:32:43 :: Resetting #1121: Plane of the Abyss (rooms 112100-112299).
Jul 24 09:32:43 :: Resetting #1123: Mount Hotenow (rooms 112300-112399).
Jul 24 09:32:43 :: Resetting #1124: The Fire Plane (rooms 112400-112599).
Jul 24 09:32:43 :: Resetting #1126: Flaming Tower (rooms 112600-112999).
Jul 24 09:32:43 :: Resetting #1130: Mantol-Derith (rooms 113000-113199).
Jul 24 09:32:43 :: Resetting #1132: Mantol-Derith Tunnels (rooms 113200-113299).
Jul 24 09:32:43 :: Resetting #1133: Ancient Mines (rooms 113300-113399).
Jul 24 09:32:43 :: Resetting #1134: Candlekeep Proper (rooms 113400-113699).
Jul 24 09:32:43 :: Resetting #1137: Ashabenford (rooms 113700-113899).
Jul 24 09:32:43 :: Resetting #1141: Soubar Underhalls (rooms 114100-114199).
Jul 24 09:32:43 :: Resetting #1142: Rogue's Lair (rooms 114200-114399).
Jul 24 09:32:43 :: Resetting #1144: Temple in the Sky (rooms 114400-114699).
Jul 24 09:32:43 :: Resetting #1147: Kobold Caverns (rooms 114700-114799).
Jul 24 09:32:43 :: Resetting #1148: South Wood (rooms 114800-114999).
Jul 24 09:32:43 :: Resetting #1150: Cloud Labyrinth (rooms 115000-115299).
Jul 24 09:32:43 :: Resetting #1153: The Friendly Arm (rooms 115300-115399).
Jul 24 09:32:43 :: Resetting #1155: The Rat Hills (rooms 115500-115699).
Jul 24 09:32:43 :: Resetting #1157: Hidden Mine (rooms 115700-115899).
Jul 24 09:32:43 :: Resetting #1159: The Abbey of Lost Tears (rooms 115900-115999).
Jul 24 09:32:43 :: Resetting #1160: The Abyss (rooms 116000-116199).
Jul 24 09:32:43 :: Resetting #1170: Dragonspear Castle (rooms 117000-117399).
Jul 24 09:32:43 :: Resetting #1174: Grunwald (rooms 117400-117699).
Jul 24 09:32:43 :: Resetting #1177: Eveningstar Haunted Halls (rooms 117700-117899).
Jul 24 09:32:43 :: Resetting #1179: Triel (rooms 117900-117999).
Jul 24 09:32:43 :: Resetting #1180: Logger's Camp (rooms 118000-118099).
Jul 24 09:32:43 :: Resetting #1181: Caverns of the Underdark (rooms 118100-118299).
Jul 24 09:32:43 :: Resetting #1183: Serpent Path (rooms 118300-118399).
Jul 24 09:32:43 :: Resetting #1184: Crystal Caverns (rooms 118400-118499).
Jul 24 09:32:43 :: Resetting #1185: Hardbuckler (rooms 118500-118699).
Jul 24 09:32:43 :: Resetting #1187: The Lost Vale (rooms 118700-118899).
Jul 24 09:32:43 :: Resetting #1189: Yellow Snake Pass (rooms 118900-119099).
Jul 24 09:32:43 :: Resetting #1191: Llawryn Keep (rooms 119100-119599).
Jul 24 09:32:43 :: Resetting #1196: Sunken Pirate Ship (rooms 119600-119699).
Jul 24 09:32:43 :: Resetting #1197: Sewers of CandleKeep (rooms 119700-119899).
Jul 24 09:32:43 :: Resetting #1199: Hardbuckler Caverns (rooms 119900-119999).
Jul 24 09:32:43 :: Resetting #1200: Zhent Graveyard (rooms 120000-120099).
Jul 24 09:32:43 :: Resetting #1201: Bugbear Caverns (rooms 120100-120199).
Jul 24 09:32:43 :: Resetting #1202: Barbarian Encampment (rooms 120200-120299).
Jul 24 09:32:43 :: Resetting #1203: Kuo-Toa Village (rooms 120300-120399).
Jul 24 09:32:43 :: Resetting #1204: Hill's Edge I (rooms 120400-120799).
Jul 24 09:32:43 :: Resetting #1208: Evereska Way (rooms 120800-120899).
Jul 24 09:32:43 :: Resetting #1209: City of Mirabar (rooms 120900-121099).
Jul 24 09:32:43 :: Resetting #1211: Dark Dominion (rooms 121100-121199).
Jul 24 09:32:43 :: Resetting #1212: Lizard Marsh (rooms 121200-121399).
Jul 24 09:32:44 :: Resetting #1214: Arabel (rooms 121400-121649).
Jul 24 09:32:44 :: Resetting #1217: Soubar (rooms 121700-121799).
Jul 24 09:32:44 :: Resetting #1218: Beregost (rooms 121800-121899).
Jul 24 09:32:44 :: Resetting #1219: Underdark Tunnels (rooms 121900-121999).
Jul 24 09:32:44 :: Resetting #1220: <*> Trade Center Zone (rooms 122000-122099).
Jul 24 09:32:44 :: Resetting #1221: Empty Zone (rooms 122100-122199).
Jul 24 09:32:44 :: Resetting #1222: Luskan Outpost (rooms 122200-122299).
Jul 24 09:32:44 :: Resetting #1224: Neverwinter - North City (rooms 122400-122599).
Jul 24 09:32:44 :: Resetting #1226: Neverwinter - South City (rooms 122600-122899).
Jul 24 09:32:44 :: Resetting #1229: Neverwinter - Cloaktower (rooms 122900-122999).
Jul 24 09:32:44 :: Resetting #1230: Neverwinter - Castle (rooms 123000-123199).
Jul 24 09:32:44 :: Resetting #1232: Neverwinter - Catacombs (rooms 123200-123399).
Jul 24 09:32:44 :: Resetting #1234: Neverwinter - Sewers (rooms 123400-123699).
Jul 24 09:32:44 :: Resetting #1237: Neverwinter - Upland Rise (rooms 123700-123799).
Jul 24 09:32:44 :: Resetting #1250: Secomber (rooms 125000-125299).
Jul 24 09:32:44 :: Resetting #1253: Iron Road (rooms 125300-125499).
Jul 24 09:32:44 :: Resetting #1255: Darklake (rooms 125500-125899).
Jul 24 09:32:44 :: Resetting #1259: Pesh (rooms 125900-125999).
Jul 24 09:32:44 :: Resetting #1260: High Horn (rooms 126000-126199).
Jul 24 09:32:44 :: Resetting #1262: Misty Forest (rooms 126200-126299).
Jul 24 09:32:44 :: Resetting #1263: Mithril Hall Palace (rooms 126300-126399).
Jul 24 09:32:44 :: Resetting #1264: Roads of Amn (rooms 126400-126599).
Jul 24 09:32:44 :: Resetting #1266: Trollclaws (rooms 126600-126699).
Jul 24 09:32:44 :: Resetting #1267: Mere of Dead Men (rooms 126700-126899).
Jul 24 09:32:44 :: Resetting #1269: Illithid Enclave (rooms 126900-126999).
Jul 24 09:32:44 :: Resetting #1272: The Reaching Woods (rooms 127200-127399).
Jul 24 09:32:44 :: Resetting #1274: Goblinoid Warrens (rooms 127400-127499).
Jul 24 09:32:44 :: Resetting #1275: Evereska (rooms 127500-127999).
Jul 24 09:32:44 :: Resetting #1280: The Halfway Inn (rooms 128000-128099).
Jul 24 09:32:44 :: Resetting #1281: The Labyrinth (rooms 128100-128599).
Jul 24 09:32:44 :: Resetting #1286: Greycloak Hills Side Path (rooms 128600-128999).
Jul 24 09:32:44 :: Resetting #1290: The Astral Plane (rooms 129000-129199).
Jul 24 09:32:44 :: Resetting #1292: The Lurkwood (rooms 129200-129399).
Jul 24 09:32:44 :: Resetting #1294: King's Lodge (rooms 129400-129499).
Jul 24 09:32:44 :: Resetting #1295: The Ethereal Plane (rooms 129500-129699).
Jul 24 09:32:44 :: Resetting #1297: Forest of Wyrms (rooms 129700-129899).
Jul 24 09:32:44 :: Resetting #1299: Crypts of Darekoth (rooms 129900-129999).
Jul 24 09:32:44 :: Resetting #1300: Undermountain [Level I] (rooms 130000-130599).
Jul 24 09:32:44 :: Resetting #1321: Avernus (rooms 132100-132399).
Jul 24 09:32:44 :: Resetting #1325: The Serpent Hills (rooms 132500-132699).
Jul 24 09:32:44 :: Resetting #1327: Snake Pit (rooms 132700-132799).
Jul 24 09:32:44 :: Resetting #1328: Amiskal's Keep (rooms 132800-132899).
Jul 24 09:32:44 :: Resetting #1329: Tower of Kenjin (rooms 132900-132999).
Jul 24 09:32:44 :: Resetting #1330: Settlestone (rooms 133000-133099).
Jul 24 09:32:44 :: Resetting #1331: The Plane of Shadows (rooms 133100-133199).
Jul 24 09:32:44 :: Resetting #1332: Ulcaster's College (rooms 133200-133299).
Jul 24 09:32:44 :: Resetting #1335: Wormwrithings (rooms 133500-133699).
Jul 24 09:32:44 :: Resetting #1337: The East Way (rooms 133700-133899).
Jul 24 09:32:44 :: Resetting #1339: High Forest (South) (rooms 133900-134099).
Jul 24 09:32:44 :: Resetting #1341: High Forest II (rooms 134100-134299).
Jul 24 09:32:44 :: Resetting #1350: Menzoberranzan (rooms 135000-135999).
Jul 24 09:32:44 :: Resetting #1360: Deep Eveningstar Halls (rooms 136000-136099).
Jul 24 09:32:44 :: Resetting #1361: Air Plane (rooms 136100-136299).
Jul 24 09:32:44 :: Resetting #1363: Water Plane (rooms 136300-136499).
Jul 24 09:32:44 :: Resetting #1365: Temple of Ghaundaur (rooms 136500-136699).
Jul 24 09:32:44 :: Resetting #1367: Earth Plane (rooms 136700-136899).
Jul 24 09:32:44 :: Resetting #1369: The Deep Caverns (rooms 136900-136999).
Jul 24 09:32:44 :: Resetting #1370: Battle of Bones (rooms 137000-137199).
Jul 24 09:32:44 :: Resetting #1372: Ch'Chitl (rooms 137200-137499).
Jul 24 09:32:44 :: Resetting #1375: Malaugrym Castle (rooms 137500-137799).
Jul 24 09:32:44 :: Resetting #1380: The Stonelands (rooms 138000-138399).
Jul 24 09:32:44 :: Resetting #1384: Daurgothoth's Domain (rooms 138400-138499).
Jul 24 09:32:44 :: Resetting #1385: Dwarven Mines (rooms 138500-138599).
Jul 24 09:32:44 :: Resetting #1386: <*> Arena (rooms 138600-138699).
Jul 24 09:32:44 :: Resetting #1387: Dragon Cult Fortress (rooms 138700-138799).
Jul 24 09:32:44 :: Resetting #1388: Zzsessak Zuhl (rooms 138800-139099).
Jul 24 09:32:44 :: Resetting #1391: The Grimlock Burrows (rooms 139100-139199).
Jul 24 09:32:44 :: Resetting #1392: Abyssal Vortex (rooms 139200-139299).
Jul 24 09:32:44 :: Resetting #1393: The Hive of Passion (rooms 139300-139399).
Jul 24 09:32:44 :: Resetting #1394: Northeastern Underdark (rooms 139400-139799).
Jul 24 09:32:44 :: Resetting #1398: The Plane of Magma (rooms 139800-139999).
Jul 24 09:32:44 :: Resetting #1402: Scorched Forest (rooms 140200-140299).
Jul 24 09:32:44 :: Resetting #1403: Urd Caverns (rooms 140300-140399).
Jul 24 09:32:44 :: Resetting #1408: Outcast Encampment (rooms 140800-140899).
Jul 24 09:32:44 :: Resetting #1409: Mines of Tugrahk Gol (rooms 140900-140999).
Jul 24 09:32:44 :: Resetting #1416: Ashenport Harbor (rooms 141600-141799).
Jul 24 09:32:44 :: Resetting #1418: Serene Forest (rooms 141800-141899).
Jul 24 09:32:44 :: Resetting #1419: Amenth'G'narr (rooms 141900-141999).
Jul 24 09:32:44 :: Resetting #1420: Elg'cahl Niar (rooms 142000-142099).
Jul 24 09:32:44 :: Resetting #1423: The Abyssal Stronghold (rooms 142300-142499).
Jul 24 09:32:44 :: Resetting #1425: Daggerford (rooms 142500-142799).
Jul 24 09:32:44 :: Resetting #1430: High Forest - East (rooms 143000-143199).
Jul 24 09:32:44 :: Resetting #1432: Beneath the Eagletower (rooms 143200-143299).
Jul 24 09:32:44 :: Resetting #1433: Thethyr Bandit Castle (rooms 143300-143399).
Jul 24 09:32:44 :: Resetting #1434: Bee Zone (rooms 143400-143499).
Jul 24 09:32:44 :: Resetting #1436: Plane of Ice (rooms 143600-143999).
Jul 24 09:32:44 :: Resetting #1440: Ardeep Forest (rooms 144000-144199).
Jul 24 09:32:44 :: Resetting #1445: Cloud Realm of Stronmaus (rooms 144500-144999).
Jul 24 09:32:44 :: Resetting #1451: Temple of Twisted Flesh (rooms 145100-145199).
Jul 24 09:32:45 :: Resetting #1452: Mosswood (rooms 145200-145399).
Jul 24 09:32:45 :: Resetting #1455: Sloopdilmonpolop (rooms 145500-146199).
Jul 24 09:32:45 :: Resetting #1468: Trollbark Forest (rooms 146800-146999).
Jul 24 09:32:45 :: Resetting #1480: Arath Zuul (rooms 148000-148099).
Jul 24 09:32:45 :: Resetting #1481: Orcish Fort (rooms 148100-148199).
Jul 24 09:32:45 :: SYSERR: zone file: invalid equipment pos number (mob 	RHigh Priest	D of Grummsh	n, obj 109, pos 148181)
Jul 24 09:32:45 :: SYSERR: ...offending cmd: 'E' cmd in zone #1481, line 31
Jul 24 09:32:45 :: Resetting #1500: <*> Quest Staging Area (rooms 150000-150599).
Jul 24 09:32:45 :: Resetting #1507: Hulburg Trail (rooms 150700-150899).
Jul 24 09:32:45 :: Resetting #1555: Labyrinth of the Mad Drow (rooms 155500-155699).
Jul 24 09:32:45 :: Resetting #1557: <*> Random Encounter Distro (rooms 155700-156199).
Jul 24 09:32:45 :: Resetting #1575: Luskan Southbank (rooms 157500-157999).
Jul 24 09:32:45 :: Resetting #1591: Hulburg (rooms 159100-159599).
Jul 24 09:32:45 :: Resetting #1596: Minotaur Outpost (rooms 159600-159699).
Jul 24 09:32:45 :: Resetting #1688: Evermeet Main Rd (rooms 168800-168899).
Jul 24 09:32:45 :: Resetting #1689: Evermeet E Coast Rd N (rooms 168900-168999).
Jul 24 09:32:45 :: Resetting #1690: Evermeet W Coast Rd N (rooms 169000-169099).
Jul 24 09:32:45 :: Resetting #1691: Evermeet Ancient Forest (rooms 169100-169199).
Jul 24 09:32:45 :: Resetting #1692: Evermeet Misc Rooms/Mobs (rooms 169200-169299).
Jul 24 09:32:45 :: Resetting #1693: Evermeet Rd to Elven Settl (rooms 169300-169399).
Jul 24 09:32:45 :: Resetting #1800: MarblePyramid HighForest E (rooms 180000-180299).
Jul 24 09:32:45 :: Resetting #1803: MarblePyramid HighForest I (rooms 180300-180499).
Jul 24 09:32:45 :: Resetting #1960: Jotunheim (rooms 196000-196299).
Jul 24 09:32:45 :: Resetting #2000: <*> Guildhalls (rooms 200000-200099).
Jul 24 09:32:45 :: Resetting #2001: Treshia (rooms 200100-200199).
Jul 24 09:32:45 :: Resetting #2002: Treshia Forest (rooms 200200-200299).
Jul 24 09:32:45 :: Resetting #2003: Within Mosswood Forest (rooms 200300-200399).
Jul 24 09:32:45 :: Resetting #5515: Delimbiyr Vale (rooms 551500-551999).
Jul 24 09:32:45 :: Resetting #5520: Road West of Secomber (rooms 552000-552099).
Jul 24 09:32:45 :: Resetting #5521: Road through Orlbar (rooms 552100-552299).
Jul 24 09:32:45 :: Resetting #5523: Skullport Port & Island (rooms 552300-552699).
Jul 24 09:32:45 :: Resetting #5527: Skullport Trade Lanes (rooms 552700-552899).
Jul 24 09:32:45 :: Resetting #5529: Skullport Heart (rooms 552900-553099).
Jul 24 09:32:45 :: Resetting #5531: Pelleor's Prairie (rooms 553100-553299).
Jul 24 09:32:45 :: Resetting #5536: Hellgate Keep (rooms 553600-554099).
Jul 24 09:32:45 :: Resetting #9999: New Zone (rooms 999900-999999).
Jul 24 09:32:45 :: Resetting #10000: <*> Wilderness of Luminari (rooms 1000000-1009999).
Jul 24 09:32:45 :: Resetting #12157521: <*> THE UPPER LIMIT (rooms 1215752100-1215752191).
Jul 24 09:32:45 :: Booting houses.
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:45 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:46 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:46 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:46 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:46 :: SYSERR: Unable to SELECT from house_data: Unknown column 'idnum' in 'field list'
Jul 24 09:32:46 :: Boot db -- DONE.
Jul 24 09:32:46 :: Signal trapping.
Jul 24 09:32:46 :: Entering game loop.
Jul 24 09:32:46 :: No connections.  Going to sleep.
Jul 24 09:32:46 :: New connection.  Waking up.

Jul 24 09:32:52 :: SYSERR: 	YBrother Spire	n (#200103): Attempting to call non-existing mob function.
Jul 24 09:32:52 :: SYSERR: Jakur the tanner (#125913): Attempting to call non-existing mob function.
Jul 24 09:32:52 :: SYSERR: Adoril (#21605): Attempting to call non-existing mob function.

Jul 24 09:33:05 :: Zusuk [147.235.211.56] has connected.
Jul 24 09:33:06 :: INFO: Loading saved object data from db for: Zusuk
Jul 24 09:33:06 :: INFO: Object save header found for: Zusuk
Jul 24 09:33:06 :: Zusuk retrieving crash-saved items and entering game.
Jul 24 09:33:06 :: SYSERR: Unable to SELECT from player_save_objs: Unknown column 'idnum' in 'field list'
Jul 24 09:33:06 :: Zusuk (level 34) has 1 object (max 99999).

Jul 24 09:33:15 :: Zusuk has quit the game.
Jul 24 09:33:19 :: Bwarg [147.235.211.56] has connected.
Jul 24 09:33:20 :: INFO: Loading saved object data from db for: Bwarg
Jul 24 09:33:20 :: INFO: Object save header found for: Bwarg
Jul 24 09:33:20 :: Bwarg retrieving crash-saved items and entering game.
Jul 24 09:33:20 :: SYSERR: Unable to SELECT from player_save_objs: Unknown column 'idnum' in 'field list'
Jul 24 09:33:20 :: Bwarg (level 30) has 796 objects (max 99999).
Jul 24 09:33:20 :: SYSERR: Mob using '((i)->player_specials->saved.pref)' at act.informative.c:845.

Jul 24 09:33:31 :: SYSERR: Attempt to damage corpse 'a bird-eating spider' in room #1918 by 'a bird-eating spider'.

Jul 24 09:34:00 :: SYSERR: Mob using '((i)->player_specials->saved.pref)' at act.informative.c:845.
Jul 24 09:34:13 :: SYSERR: Mob using '((i)->player_specials->saved.pref)' at act.informative.c:845.

Jul 24 09:34:56 :: SYSERR: Attempt to damage corpse 'a jet-black crow' in room #1920 by 'a jet-black crow'.
Jul 24 09:34:56 :: SYSERR: Attempt to damage corpse 'a small cricket' in room #1918 by 'a small cricket'.

</details>

---

## Performance Analysis Report: Pulse Usage High Water Marks

### Executive Summary
The server experienced severe performance degradation on July 24, with game pulse processing times increasing from 2.5% (2.5ms) to 740% (740ms) of the allocated 100ms time window. This represents a 296x performance degradation that would result in severe lag for players.

### Performance Degradation Timeline
- **09:32:46** - Initial baseline: 2.50% (2.5ms) - Normal operation
- **09:32:51** - Minor increase: 8.82% (8.8ms) - Still acceptable
- **09:32:52** - First major spike: 197.76% (197ms) - Game becomes laggy
- **09:32:58** - Sustained high load: 208.24% (208ms)
- **09:33:08** - Player login spike: 286.21% (286ms)
- **09:33:20** - Another player login: 305.94% (306ms)
- **09:33:43** - Continued degradation: 376.66% (377ms)
- **09:33:50** - Severe lag: 514.06% (514ms)
- **09:34:47** - Critical performance: 740.24% (740ms)

### Primary Performance Bottlenecks Identified

#### 1. **Crash_save_all (445.62% alone)**
- Single largest contributor to the worst spike
- Occurs during periodic autosaves
- Blocking operation that saves all player data

#### 2. **mobile_activity (158-182% consistently)**
- Core AI/NPC processing loop
- Shows consistent high usage across multiple spikes
- Likely processing too many NPCs or inefficient AI routines

#### 3. **do_gen_cast (125-154% usage)**
- Spell casting system
- Called 300-400 times per pulse in some cases
- Indicates excessive spell casting by NPCs or inefficient spell processing

#### 4. **affect_update (30-40% usage)**
- Updates temporary effects on characters
- Consistent medium-high usage suggests many active effects

#### 5. **do_save (285-513% usage)**
- Player save operations
- Triggered by player logins/actions
- Database operations with "Unknown column 'idnum'" errors indicate schema issues

#### 6. **do_mhunt (8-9% per call)**
- Monster hunting/tracking system
- While not huge individually, adds to overall load

### Critical Issues Contributing to Performance Problems

1. **Database Schema Mismatch**
   - `SYSERR: Unable to SELECT from player_save_objs: Unknown column 'idnum' in 'field list'`
   - Failed database queries during player loads/saves

2. **Script System Errors**
   - Multiple "Attempting to call non-existing mob function" errors
   - NPCs trying to execute broken scripts, wasting CPU cycles

3. **Invalid NPC State Access**
   - `SYSERR: Mob using '((i)->player_specials->saved.pref)'`
   - NPCs accessing player-only data structures

4. **Combat System Issues**
   - "Attempt to damage corpse" errors
   - Combat continuing against already-dead targets

### Recommendations for Immediate Action

1. **Fix Database Schema** (CRITICAL)
   - Add missing 'idnum' column to player_save_objs table
   - This will reduce save/load times significantly

2. **Optimize Autosave System** (HIGH)
   - Implement incremental saves instead of saving all players at once
   - Consider async/background saves
   - Current Crash_save_all is blocking for 445ms+

3. **Review NPC Scripts** (HIGH)
   - Fix or remove broken mob functions
   - Audit all NPCs mentioned in errors (Brother Spire #200103, Jakur #125913, Adoril #21605)

4. **Optimize Mobile Activity** (MEDIUM)
   - Profile mobile_activity function to find specific bottlenecks
   - Consider reducing NPC AI frequency or implementing zone-based processing

5. **Fix Combat Target Validation** (MEDIUM)
   - Add checks to prevent attacking corpses
   - Clean up combat targets properly when entities die

6. **Spell System Optimization** (MEDIUM)
   - Investigate why do_gen_cast is called 300+ times per pulse
   - Consider batching or rate-limiting NPC spellcasting

### Long-term Recommendations

1. Implement proper performance monitoring with alerts when pulse usage exceeds 50%
2. Consider moving to an event-driven architecture rather than polling every entity every pulse
3. Implement zone instancing to distribute load
4. Add database connection pooling and query optimization
5. Create automated tests for performance regression

### Conclusion
The performance issues are severe and multifaceted, stemming from database problems, inefficient NPC processing, and accumulating technical debt. The most critical fixes are the database schema and autosave system, which alone could reduce the worst spikes by over 50%.
