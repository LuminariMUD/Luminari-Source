# Duris Racial Imports

Status: record of merged work, verified 2026-09-23 against master `e33ed0d6f` and the Duris
checkout at `/home/aiwithapex/projects/duris` (revision `a71fdfee7`).

LuminariMUD imported every implemented racial innate of the 37 Duris player races as feats, plus
racial casting speed, the Minotaur charge stun and bloodlust rage, and the Thri-Kreen four arms.
No Duris race was imported, and none of these feats is granted to a race.
Each is a non-learnable innate (`in_game` TRUE, `can_learn` FALSE, `FEAT_TYPE_INNATE_ABILITY`)
whose mechanic tests `HAS_FEAT()`, never `GET_RACE()`, so a race can take it later with one
`feat_race_assignment()` line in `assign_races()`. The only race rows touched are two
compatibility grants that kept existing races unchanged when their race checks became feat
checks: `FEAT_STABILITY` to CrystalDwarf and `FEAT_BODYSLAM` to HalfTroll. Source comments call
this set the "Sep 2026 racial innates".

How the feats work:
[GAME_MECHANICS_SYSTEMS.md](systems/GAME_MECHANICS_SYSTEMS.md#racial-innate-feats-and-spell-like-abilities).
Race point prices:
[PLAYER_RACES_REFERENCE.md](guides/PLAYER_RACES_REFERENCE.md#race-point-rp-pricing-table).
Four-arm design record:
[THRI_KREEN_FOUR_ARMS.md](https://github.com/LuminariMUD/Luminari-Source/blob/e33ed0d6f98d28d4218c98aa57b16de9742ac8c5/docs/ongoing-projects/THRI_KREEN_FOUR_ARMS.md).

| PR | Merged | Imported |
| -- | -- | -- |
| #159 | 2026-09-12 | Feats 1268-1315, nine existing feats wired or repurposed, 19 commands, two summons, help, tests |
| #182 | 2026-09-14 | Fast and slow casting (1316-1317); racial spell power settled on an existing feat |
| #183 | 2026-09-14 | Bull charge and bloodlust (1318-1319) |
| #181 | 2026-09-14 | Extra arms and four arms (1320-1321), seven wear slots |

## What we imported

### New innate feats

IDs are the `FEAT_*` constants in `src/core/structs.h`; names are the `feato()` rows in
`src/character/feats.c`. Effects: `feat info <name>` in game or the help entry of the same name.
"Duris source" is the Duris innate from `src/classes/innates.c` and the player races that register
it. "Commented out" means Duris keeps the code but disabled that race registration; the import
covered those innates as well.

| ID | Feat | Command | Duris source |
| -- | -- | -- | -- |
| 1268 | `FEAT_SUN_VULNERABILITY` (sun vulnerability) | - | `INNATE_VULN_SUN`: Drow Elf, Duergar Dwarf, Ogre, Troll, Illithid, Githyanki, Lich, Vampire, Death Knight, Orog, Planetbound Illithid, Kuo Toa |
| 1269 | `FEAT_DAYBLIND` (dayblind) | - | `INNATE_DAYBLIND`: Planetbound Illithid; commented out on 13 other races |
| 1270 | `FEAT_MAGIC_VULNERABILITY` (magic vulnerability) | - | `MAGIC_VULNERABILITY`: Ogre, Firbolg |
| 1271 | `FEAT_MAGICAL_REDUCTION` (magical reduction) | - | `MAGICAL_REDUCTION`: Mountain Dwarf, Duergar Dwarf |
| 1272 | `FEAT_THICK_HIDE` (thick hide) | - | `INNATE_TROLL_SKIN`: Troll |
| 1273 | `FEAT_SACRILEGIOUS_POWER` (sacrilegious power) | - | `INNATE_SACRILEGIOUS_POWER`: Vampire |
| 1274 | `FEAT_SPELL_ABSORB` (spell absorb) | - | `INNATE_SPELL_ABSORB`: Lich |
| 1275 | `FEAT_EYELESS` (eyeless) | - | `INNATE_EYELESS`: Lich |
| 1276 | `FEAT_QUICK_THINKING` (quick thinking) | - | `INNATE_QUICK_THINKING`: Halfling, Half-Elf |
| 1277 | `FEAT_GROUNDFIGHTING` (groundfighting) | - | `INNATE_GROUNDFIGHTING`: Barbarian, Troll, Drider |
| 1278 | `FEAT_QUADRUPED_BODY` (quadruped body) | - | `INNATE_HORSE_BODY`: Centaur; `INNATE_SPIDER_BODY`: Drider |
| 1279 | `FEAT_WATER_BREATHING` (water breathing) | - | `INNATE_WATERBREATH`: Kuo Toa |
| 1280 | `FEAT_UNDEAD_FEALTY` (undead fealty) | - | `INNATE_UNDEAD_FEALTY`: Lich |
| 1281 | `FEAT_AXE_MASTERY` (axe mastery) | - | `INNATE_AXE_MASTER`: Mountain Dwarf |
| 1282 | `FEAT_HAMMER_MASTERY` (hammer mastery) | - | `INNATE_HAMMER_MASTER`: Mountain Dwarf |
| 1283 | `FEAT_LONGSWORD_MASTERY` (longsword mastery) | - | `INNATE_LONGSWORD_MASTER`: Drow Elf, Grey Elf, Half-Elf, Wood Elf |
| 1284 | `FEAT_GREATSWORD_MASTERY` (greatsword mastery) | - | `TWO_HANDED_SWORD_MASTERY`: Centaur |
| 1285 | `FEAT_HATRED` (hatred) | - | `INNATE_HATRED`: Mountain Dwarf |
| 1286 | `FEAT_BATTLE_FRENZY` (battle frenzy) | - | `INNATE_BATTLE_FRENZY`: Duergar Dwarf |
| 1287 | `FEAT_WARCALLERS_FURY` (warcaller's fury) | - | `INNATE_WARCALLERS_FURY`: Orog |
| 1288 | `FEAT_RRAKKMA` (rrakkma) | - | `INNATE_RRAKKMA`: Githzerai |
| 1289 | `FEAT_OUTDOOR_STEALTH` (outdoor stealth) | - | `INNATE_OUTDOOR_SNEAK`: Grey Elf, Wood Elf |
| 1290 | `FEAT_SWAMP_STEALTH` (swamp stealth) | - | `INNATE_SWAMP_SNEAK`: Troll, Kuo Toa |
| 1291 | `FEAT_UNDERDARK_STEALTH` (underdark stealth) | - | `INNATE_UD_SNEAK`: Duergar Dwarf, Shadow Beast, Kobold |
| 1292 | `FEAT_FOREST_SIGHT` (forest sight) | - | `INNATE_FOREST_SIGHT`: Grey Elf, Wood Elf, Firbolg |
| 1293 | `FEAT_SEADOG` (seadog) | - | `INNATE_SEADOG`: Human, Orc |
| 1294 | `FEAT_MINER` (miner) | - | `INNATE_MINER`: commented out on Mountain Dwarf, Duergar Dwarf, Gnome, Kobold |
| 1295 | `FEAT_BARTER` (barter) | - | `INNATE_BARTER`: commented out on Halfling |
| 1296 | `FEAT_CALMING` (calming) | - | `INNATE_CALMING`: commented out on Halfling, Kobold |
| 1297 | `FEAT_SLA_FARSEE` (innate farsee) | `farsee` | `INNATE_FARSEE`: Gnome |
| 1298 | `FEAT_SLA_STONESKIN` (innate stoneskin) | `stoneskin` | `INNATE_STONE`: Wight |
| 1299 | `FEAT_SLA_LIGHTNING_BOLT` (innate lightning bolt) | `throwlightning` | `INNATE_THROW_LIGHTNING`: Kuo Toa; commented out on Storm Giant |
| 1300 | `FEAT_SLA_FIRE_SHIELD` (innate fire shield) | `fireshield` | `INNATE_FIRESHIELD`: Death Knight |
| 1301 | `FEAT_SLA_FIRE_STORM` (innate fire storm) | `firestorm` | `INNATE_FIRESTORM`: Death Knight |
| 1302 | `FEAT_SLA_SHADOW_JUMP` (innate shadow jump) | `shadowdoor` | `INNATE_SHADOW_DOOR`: Revenant |
| 1303 | `FEAT_SLA_PLANE_SHIFT` (innate plane shift) | `planeshift` | `INNATE_SHIFT_PRIME`, `INNATE_SHIFT_ASTRAL`: Illithid, Githyanki, Phantom, Githzerai, Planetbound Illithid |
| 1304 | `FEAT_SLA_PSIONIC_BLAST` (innate psionic blast) | `mindblast` | `INNATE_BLAST`: Illithid |
| 1305 | `FEAT_SLA_SCARE` (innate scare) | `roar` | `INNATE_OGREROAR`: Ogre |
| 1306 | `FEAT_SLA_FIREBALL` (innate fireball) | `fireball` | `INNATE_FIREBALL`: commented out on Drow Elf |
| 1307 | `FEAT_SLA_MASS_DISPEL` (innate mass dispel) | `massdispel` | `INNATE_MASS_DISPEL`: commented out on Drow Elf |
| 1308 | `FEAT_SLA_FROST_BREATH` (innate frost breath) | `frostbreath` | `INNATE_BARB_BRATH`: commented out on Barbarian |
| 1309 | `FEAT_SLA_WEB` (innate web) | `webwrap` | `INNATE_WEBWRAP`: commented out on Drider |
| 1310 | `FEAT_BODYSLAM` (bodyslam) | `bodyslam` | `INNATE_BODYSLAM`: Barbarian, Ogre, Troll, Revenant, Wight, Firbolg |
| 1311 | `FEAT_DOORBASH` (doorbash) | `doorbash` | `INNATE_DOORBASH`: Barbarian, Ogre, Troll, Minotaur, Revenant, Wight, Firbolg; commented out on Storm Giant. `INNATE_DOORKICK`: Centaur |
| 1312 | `FEAT_STAMPEDE` (stampede) | `stampede` | `INNATE_STAMPEDE`: Centaur |
| 1313 | `FEAT_RACIAL_FLURRY` (racial flurry) | `onslaught` | `INNATE_FLURRY`: commented out on Halfling, Goblin |
| 1314 | `FEAT_SUMMON_WARG` (summon warg) | `summonwarg` | `INNATE_SUMMON_WARG`: Orc |
| 1315 | `FEAT_SUMMON_HORDE` (summon horde) | `summonhorde` | `INNATE_SUMMON_HORDE`: Orc |
| 1316 | `FEAT_FAST_CASTING` (fast casting, stackable) | - | `spellcast.pulse.racial.<Race>` below 1.0 in `lib/duris.properties` |
| 1317 | `FEAT_SLOW_CASTING` (slow casting, stackable) | - | `spellcast.pulse.racial.<Race>` above 1.0 |
| 1318 | `FEAT_BULL_CHARGE` (bull charge) | `charge <direction> <target>` | `INNATE_CHARGE`: Minotaur (its stun and one-room reach) |
| 1319 | `FEAT_BLOODLUST` (bloodlust) | - | `minotaur_race_proc()` in `src/classes/drannak.c`: Minotaur |
| 1320 | `FEAT_EXTRA_ARMS` (extra arms, stackable) | - | None: general extra-arm trait kept from the rejected Thri-Kreen stand-in |
| 1321 | `FEAT_FOUR_ARMS` (four arms) | - | `HAS_FOUR_HANDS()` in `src/core/utils.h`: Thri-Kreen |

### Existing feats wired or repurposed

| Feat | Change | Duris source |
| -- | -- | -- |
| `FEAT_WEAKNESS_TO_FIRE` (weakness to fire) | Fire vulnerability now feat-gated; was a HalfTroll race check | `INNATE_VULN_FIRE`: Troll, Shade, Revenant, Shadow Beast, Wight, Phantom; commented out on Lich, Vampire |
| `FEAT_VULNERABLE_TO_COLD` (vulnerable to cold) | Cold vulnerability now feat-gated; was a Trelux race check | `INNATE_VULN_COLD`: Thri-Kreen |
| `FEAT_LEAP` (leap) | Weapon-attack avoidance now feat-gated; was a Trelux race check | `INNATE_LEAP`: Thri-Kreen |
| `FEAT_COMBAT_TRAINING_VS_GIANTS` (combat training vs giants) | AC bonus against larger attackers now feat-gated; was a race list | `INNATE_GIANT_AVOIDANCE`: Mountain Dwarf |
| `FEAT_LEONINE_FRAME` (leonine frame) | Refuses leg and foot equipment when held | Centaur and Drider leg and foot slot loss |
| `FEAT_STABILITY` (stability) | Knockdown resistance now feat-gated; was a dwarf race list | Knockdown part of `INNATE_HORSE_BODY` and `INNATE_SPIDER_BODY` |
| `FEAT_LICH_SPELL_RESIST` (lich spell resist) | Spell resistance now feat-gated; was a lich check | Magic resistance at shrug 50 or more: Illithid, Lich, Planetbound Illithid |
| `FEAT_KENDER_FEARLESSNESS` (fearlessness) | Renamed from the kender text | `INNATE_DAUNTLESS`: Barbarian |
| `FEAT_HASTE` (innate haste) | Unused feat repurposed as the 1/day `battlehaste` | `INNATE_BATTLE_RAGE`: Duergar Dwarf |
| `FEAT_ENHANCED_SPELL_DAMAGE` (enhanced spell damage) | No code change; verified race-assignable (#182) | The Duris Pow (spell power) stat |

### Duris innates already covered by existing feats

No code changed for these; a race import grants the existing feat.

| Duris innate | LuminariMUD feat |
| -- | -- |
| `INNATE_INFRAVISION` | `FEAT_INFRAVISION` (low light vision) |
| `INNATE_ULTRAVISION` | `FEAT_ULTRAVISION` |
| `INNATE_LEVITATE` | `FEAT_SLA_LEVITATE` |
| `INNATE_FAERIE_FIRE` | `FEAT_SLA_FAERIE_FIRE` |
| `INNATE_DARKNESS`, `INNATE_GLOBE_OF_DARKNESS` | `FEAT_SLA_DARKNESS` |
| `INNATE_ENLARGE` | `FEAT_SLA_ENLARGE` |
| `INNATE_STRENGTH` | `FEAT_SLA_STRENGTH` |
| `INNATE_UD_INVISIBILITY` | `FEAT_SLA_INVIS` |
| `INNATE_FLY` | `FEAT_WINGS` |
| `INNATE_PROT_FIRE` | `FEAT_TIEFLING_HELLISH_RESISTANCE` |
| `INNATE_PROT_COLD` | `FEAT_MOUNTAIN_BORN` |
| `INNATE_MAGIC_RESISTANCE` (shrug) | `FEAT_HALF_DROW_SPELL_RESISTANCE` at shrug 20-25, `FEAT_DROW_SPELL_RESISTANCE` at 35-40, `FEAT_LICH_SPELL_RESIST` at 50 or more; Wood Elf shrug 5 has no band |
| `INNATE_REGENERATION` | `FEAT_TROLL_REGENERATION` |
| `INNATE_HIDE` | `FEAT_NATURALLY_STEALTHY` |
| `INNATE_DISAPPEAR` | `FEAT_ONE_WITH_SHADOW` |
| `INNATE_SHADE_MOVEMENT` | `FEAT_ONE_WITH_SHADOW` and `FEAT_PRACTICED_SNEAK` |
| `INNATE_PERCEPTION` | `FEAT_KEEN_SENSES` |
| `INNATE_VAMPIRIC_TOUCH` | `FEAT_VAMPIRE_BLOOD_DRAIN`, `FEAT_VAMPIRE_ENERGY_DRAIN` |
| `INNATE_BITE` | `FEAT_POISON_BITE` or `FEAT_DRACONIAN_BITE` |
| `INNATE_PHANTASMAL_FORM` | `FEAT_VAMPIRE_GASEOUS_FORM` |
| `INNATE_CALL_GRAVE` | `FEAT_SUMMON_UNDEAD` |
| `INNATE_CHARGE` | The general `charge` command; the stun and reach are `FEAT_BULL_CHARGE` |

### Supporting pieces

- Commands: `do_racial_sla` and `racial_sla_table[]` in `src/act/act.other.c` serve the 16
  spell-like verbs (`SCMD_RSLA_*`, `NUM_RACIAL_SLAS` in `src/core/interpreter.h`). `onslaught` is
  `do_racial_flurry` in the same file, `doorbash` is in `src/movement/movement.c`, and `stampede`
  and the bull charge are in `src/combat/act.offensive.c`. The existing `bodyslam` skill now
  requires `FEAT_BODYSLAM` (`src/character/skill_lists.c`).
- Cooldowns: the daily uses and the stampede cooldown are events `eSLA_FARSEE` to
  `eRACIAL_FLURRY` in `src/events/mud_event.h`.
- Spell numbers in `src/magic/spells.h`: `AFFECT_RACIAL_FLURRY` 1342, `ABILITY_SUMMON_WARG` 1343,
  `ABILITY_SUMMON_HORDE` 1344, and the bloodlust rage affect `SKILL_BLOODLUST` 2244.
- Summoned mobs: warg 19502 and orc warrior 19503 (`PET_RACIAL_WARG`, `PET_RACIAL_ORC_WARRIOR` in
  `src/config/pet_vnums.h`), shipped in `data/pet-lycanthropes/195.mob` and installed by
  `scripts/world/install_pet_constructs.py`.
- Equipment: seven four-arm wear positions, `WEAR_WIELD_3` (44) to `WEAR_WRIST_L2` (50), with
  `NUM_WEARS` 51, and the attack types `ATTACK_TYPE_THIRD` and `ATTACK_TYPE_FOURTH`.
- Help: 55 entries, one per new feat plus `INNATE-HASTE`, in
  `sql/components/help_other_racial_innate_entries.sql` and `lib/text/help/help.hlp`.
- Tests: `unittests/CuTest/test_racial_innate_feats.c` (46 tests) and
  `unittests/CuTest/test_four_arms.c` (21 tests).

### Not imported

- `INNATE_DAYVISION` (Orc, Thri-Kreen, Minotaur, Harpy, Githzerai, Kobold), `INNATE_SUMMON_TOTEM`
  (Goblin), and `INNATE_PROJECT_IMAGE` (Shadow Beast): registered in Duris but not implemented
  there.
- Innates registered only on Duris NPC races.
- Race-level levers, closed as not planned: the experience factor, racial hit points as race data,
  troll regeneration ranks, and a Huge player size (#163); melee output multipliers and attack
  round speed (#164, which also dropped giant wield and bonus damage against smaller foes as
  already covered by size rules); a race-side class deny list and a shared descend routine
  (#166).
- Race data (stat factors, shrug values, racial saves, size, and slot limits) comes with a race.

## Duris races not imported

Duris defines 37 player races, `RACE_HUMAN` (1) to `RACE_TIEFLING` (37, `RACE_PLAYER_MAX`) in
`src/core/defines.h`. Availability comes from `playable_races[]` and `restricted_races[]` in
`src/core/constant.c`; rows below follow those tables. None was imported, and the NPC races (38
and up) were out of scope. The last column names the nearest LuminariMUD race where one exists;
each is our own earlier design and this work did not change it.

| Duris race | Constant | Duris availability | Nearest LuminariMUD race |
| -- | -- | -- | -- |
| Human | `RACE_HUMAN` (1) | Creation, good | Human |
| Barbarian | `RACE_BARBARIAN` (2) | Creation, good | - |
| Grey Elf | `RACE_GREY` (4) | Creation, good | Moon Elf, High Elf |
| Mountain Dwarf | `RACE_MOUNTAIN` (5) | Creation, good | Mountain Dwarf |
| Halfling | `RACE_HALFLING` (7) | Creation, good | Lightfoot Halfling, Stout Halfling |
| Gnome | `RACE_GNOME` (8) | Creation, good | Rock Gnome, Forest Gnome |
| Centaur | `RACE_CENTAUR` (15) | Creation, good | Wemic (four-legged body) |
| Githzerai | `RACE_GITHZERAI` (30) | Creation, good | - |
| Firbolg | `RACE_FIRBOLG` (36) | Creation, good | - |
| Drow Elf | `RACE_DROW` (3) | Creation, evil | Drow |
| Duergar Dwarf | `RACE_DUERGAR` (6) | Creation, evil | Duergar |
| Ogre | `RACE_OGRE` (9) | Creation, evil | Half-Ogre |
| Troll | `RACE_TROLL` (10) | Creation, evil | HalfTroll |
| Orc | `RACE_ORC` (13) | Creation, evil | HalfOrc |
| Githyanki | `RACE_GITHYANKI` (16) | Creation, evil | - |
| Goblin | `RACE_GOBLIN` (20) | Creation, evil | Goblin |
| Kobold | `RACE_KOBOLD` (32) | Creation, evil | - |
| Drider | `RACE_DRIDER` (31) | Creation, evil | - |
| Thri-Kreen | `RACE_THRIKREEN` (14) | Creation, either side | Trelux (insectoid) |
| Minotaur | `RACE_MINOTAUR` (17) | Creation, either side | - |
| Tiefling | `RACE_TIEFLING` (37) | Creation, either side | Tiefling |
| Lich | `RACE_LICH` (21) | Descend: necromancer | Lich |
| Vampire | `RACE_PVAMPIRE` (22) | Descend: sorcerer or dreadlord | Vampire |
| Death Knight | `RACE_PDKNIGHT` (23) | Descend: anti-paladin | - |
| Wight | `RACE_WIGHT` (26) | Descend: warrior | - |
| Revenant | `RACE_REVENANT` (19) | Descend: mercenary | - |
| Shadow Beast | `RACE_PSBEAST` (24) | Descend: assassin | - |
| Phantom | `RACE_PHANTOM` (27) | Descend: conjurer | - |
| Shade | `RACE_SHADE` (18) | Descend: thief or illusionist | Shade |
| Half-Elf | `RACE_HALFELF` (11) | Legacy | Half Elf |
| Wood Elf | `RACE_WOODELF` (35) | Legacy | Wild Elf |
| Kuo Toa | `RACE_KUOTOA` (34) | Legacy | - |
| Orog | `RACE_OROG` (29) | Legacy | - |
| Harpy | `RACE_HARPY` (28) | Lore-restricted | - |
| Illithid | `RACE_ILLITHID` (12) | Lore-restricted, no racewar side | - |
| Planetbound Illithid | `RACE_PILLITHID` (33) | Lore-restricted, no racewar side | - |
| Storm Giant | `RACE_SGIANT` (25) | Lore-restricted, no racewar side | - |

To add one, register it with `add_race()` and grant feats from the tables above with
`feat_race_assignment()`, following [ADDING_NEW_RACE_GUIDE.md](guides/ADDING_NEW_RACE_GUIDE.md).
Per-race stat conversions, Duris level gates, and race point scores are in the retired
[race conversion study](https://github.com/LuminariMUD/Luminari-Source/blob/dba4ca2de4afdbe47fc0d1f6831a1a1f75db1e7e/docs/ongoing-projects/DURIS_RACE_CONVERSION.md).
