# RoL Playable Races Without a Close LuminariMUD Equivalent

Status: source audit completed 2026-08-23.

Third document in the Realms of Luminari conversion series, after
`ROL_SPELL_EQUIVALENCE_GAPS.md` and `ROL_SKILL_EQUIVALENCE_GAPS.md`. This one
compares RoL's player-character races with LuminariMUD's playable races.

Unfinished RoL races are in scope. A race counts as in scope if it occupies a
slot in RoL's player-race range, whether or not a player can currently select
it. Two such races exist, and both are documented below.

A close equivalent must preserve the race's playable archetype, not merely
share a name or a stat profile. LuminariMUD's Half Orc is accepted as the
equivalent of RoL's Orc on that basis; a race with no archetype counterpart at
all is a gap.

- RoL player-race slots (`RACE_NONE` through `LAST_PC_RACE`): 17
- Real PC races (excluding the `RACE_NONE` sentinel): 16
- Selectable at character creation: 14
- Defined in the PC range but not selectable: 2
- Reserved empty slots for future PC races: 2
- Races with a close LuminariMUD equivalent: 11
- Races without a close equivalent: 5
- LuminariMUD playable races, for comparison: 30

## Player-facing gaps

| RoL ID | RoL race | Constant | Mob code | Hometown | Classes allowed |
|-------:|----------|----------|----------|----------|-----------------|
| 2 | Barbarian | `RACE_BARBARIAN` | PB | Griffon's Nest (Uthgar) | Warrior, Shaman, Mercenary |
| 9 | Ogre | `RACE_OGRE` | PO | Faang (Tyr) | Warrior, Berserker, Shaman, Mercenary |
| 12 | Illithid | `RACE_ILLITHID` | PI | Ixarkon | Psionicist only |
| 13 | Yuan-Ti | `RACE_YUANTI` | PY | Hyssk (Merrshaulk) | Warrior, Cleric, Necromancer, Conjurer, Enchanter, Invoker, Illusionist, Rogue, Elementalist |
| 15 | Myconid | `RACE_MYCONID` | PS | none | (placeholder row, copied from Lich) |

### Why each is a gap

**Barbarian (2).** In RoL this is a race, not a class: a distinct human-variant
people with its own racewar hometown and a three-class restriction. In
LuminariMUD "barbarian" is a class, and no race models the Uthgardt-style
tribal human. Human and Goliath are the nearest playable races but neither
carries the archetype. Stats: STR 130, DEX 95, AGI 100, CON 130, INT 85,
WIS 100, CHA 95. Base age 17, moves 95, mana 45/80, +1 HP bonus. Innate:
`INNATE_BODYSLAM`.

**Ogre (9).** LuminariMUD defines `RACE_H_OGRE` (28) with the trailing comment
`// not yet implemented`, and it is not registered through `add_race()`, so it
is not playable. Half Troll is the only Large playable race and is the nearest
brute archetype, but it is already the equivalent of RoL's Troll; using it for
both collapses two distinct RoL races into one. Stats: STR 210, DEX 80, AGI 75,
CON 150, INT 60, WIS 60, CHA 50, commented `/* massive strength balances */`.
Base age 12, moves 105, mana 35/75, +2 HP bonus. Innates:
`INNATE_DOORBASH + INNATE_BODYSLAM`.

**Illithid (12).** A mind flayer PC race locked to a single class,
Psionicist. LuminariMUD has no illithid race, no `RACE_ILLITHID` constant, and
no race-locked-to-one-class mechanic. Its only illithid content is the
Illithid Enclave zone (`src/spec/spec_zone_illithid_enclave.c`), which is area
content with a commented-out `GET_RACE(ch) != RACE_ILLITHID` check that no
longer compiles against any such constant. Stats: STR 60, DEX 110, AGI 80,
CON 80, POW 140, INT 175, WIS 110, CHA 50, commented `/* low str and con
balances the massive int and power */`. Base age 40, moves 70, mana 75/600,
-1 HP bonus. Innate: `INNATE_LEVITATE`.

**Yuan-Ti (13).** A serpentfolk PC race and RoL's most innate-rich:
`INNATE_SCALE_SKIN + INNATE_VIPER_MIND + INNATE_BEFRIENDREPTILE +
INNATE_INFRAVISION + INNATE_SNAKEBITE + INNATE_TAILSWEEP`. LuminariMUD has no
yuan-ti or serpentfolk playable race. Dragonborn is reptilian but is a
draconic-breath race, not a snake-kin race with a befriend-reptile and
tail-sweep kit. Stats: STR 95, DEX 100, AGI 125, CON 95, INT 125, WIS 150,
CHA 90. Base age 20, moves 96, mana 75/300, no HP bonus.

**Myconid (15).** Fungal race. Unfinished in RoL as well as absent from
LuminariMUD; see the next section and the myconid notes in
`ROL_SKILL_EQUIVALENCE_GAPS.md`. Stats: STR 210, DEX 60, AGI 75, CON 150,
INT 75, WIS 60, CHA 50. Base age 12, moves 105, mana 35/75, +1 HP bonus. No
innate abilities.

## Unfinished or unselectable in RoL

Both of these occupy PC race slots but no player can create one: the race menu
in `lib/creation/racetable` offers only letters a-k, p, q, and r, and neither
race has a `1` in any row of `avail_hometowns[]`, so neither can be assigned a
starting town.

| RoL ID | RoL race | Constant | State |
|-------:|----------|----------|-------|
| 14 | Lich | `RACE_LICH` | Not in the creation menu and has no hometown. Stat row is all 100s with the comment `/* potential new player races, due */`. RoL delivers lichdom as `CLASS_LICH` (18) instead: `necro.c:707` sets `GET_CLASS(ch) = CLASS_LICH` with the very next line, `GET_RACE(ch) = RACE_LICH;`, commented out. Live code in `files.c:633` still branches on a PC with `RACE_LICH`, so the race was anticipated but never wired to creation. |
| 15 | Myconid | `RACE_MYCONID` | Not in the creation menu and has no hometown. Its class-permission row is a byte-for-byte copy of the Lich row. Its seven `spores of *` skills sit inside an `#if 0` block, `CLASS_MYCONID` has no `#define` anywhere, and the `myconid.c` that `prototypes.h:3157` declares 15 functions for does not exist. |

RoL's Lich is **not** a LuminariMUD gap: LuminariMUD ships a fully registered
playable Lich (`RACE_TYPE_UNDEAD`, unlock cost 999999999), which is more
complete than RoL's. RoL's Myconid **is** a gap and is listed in the gaps table
above.

### Reserved empty slots

| RoL ID | Constant | Note |
|-------:|----------|------|
| 17 | `RACE_UNDEFINED_17` | Commented `/* this 2 slot 'gap', allows for contiguous future player races */` |
| 18 | `RACE_UNDEFINED_18` | Same block |

These are named `"None17"` and `"None18"` in `race_types[]` and carry filler
stat rows. There is nothing to port.

## Near-miss cases ruled covered

| RoL race | LuminariMUD equivalent | Note |
|----------|------------------------|------|
| Human (1) | Human | |
| Drow-Elf (3) | Drow | Unlock cost 1000 in LuminariMUD |
| Grey-Elf (4) | High Elf | LuminariMUD also has Moon Elf and Wild Elf |
| Dwarf (5) | Mountain Dwarf | LuminariMUD also has Gold Dwarf and Crystal Dwarf |
| Duergar (6) | Duergar | Unlock cost 1000 |
| Halfling (7) | Lightfoot Halfling | LuminariMUD also has Stout Halfling |
| Gnome (8) | Rock Gnome | LuminariMUD also has Forest Gnome |
| Troll (10) | Half Troll | RoL's is a full Swamp Troll; Half Troll is LuminariMUD's only Large playable race. Archetype preserved, power level is not |
| Half-Elf (11) | Half Elf | LuminariMUD also has Half Drow |
| Lich (14) | Lich | LuminariMUD's is a real playable race; RoL's is unfinished. See above |
| Orc (16) | Half Orc | RoL's `RACE_PORC` is a full orc with `INNATE_SUMMON_HORDE` and its own racewar hometown, Bloodtusk. LuminariMUD's `RACE_ORC` (27) is a legacy alias sharing a slot with `RACE_HOBGOBLIN` and is not registered as playable, so Half Orc carries the archetype |

## LuminariMUD-side notes

- LuminariMUD has 30 playable races against RoL's 14 selectable ones, so this
  comparison is lopsided in LuminariMUD's favor overall. Races such as
  Dragonborn, Tiefling, Aasimar, Tabaxi, Goliath, Shade, Fae, Trelux,
  Arcana Golem, Crystal Dwarf, Vampire, Goblin, and Hobgoblin have no RoL
  counterpart at all. That direction is out of scope here.
- `RACE_H_OGRE` (28) is defined with `// not yet implemented` and is the
  natural landing slot if Ogre is ported.
- Watch the LuminariMUD race ID range when porting. `NUM_RACES` is 28, and the
  three constants immediately after it reuse live IDs:
  `RACE_DEEP_GNOME` 26 collides with `RACE_GOBLIN`, `RACE_ORC` 27 collides with
  `RACE_HOBGOBLIN`, and `RACE_H_OGRE` 28 sits on the `NUM_RACES` boundary.
  Retired IDs from 29 up are reserved for persisted character and world data
  (`LEGACY_RACE_START`), so a new playable race cannot simply take the next
  free number.

## Verification method

1. RoL player races were taken from `race_class.h`, bounded by
   `LAST_PC_RACE` (16), giving `PC_RACES` (17) including the `RACE_NONE`
   sentinel.
2. Selectability was confirmed two ways: the `select_race()` switch in
   `nanny.c:2248` and the menu text in `lib/creation/racetable`. Both list
   exactly 14 races.
3. Every race's starting-town availability was resolved by decoding the
   `avail_hometowns[][PC_RACES]` matrix in `race_class.c`. Lich and Myconid
   have no `1` in any row.
4. LuminariMUD playable races were taken from the `add_race()` calls in
   `src/character/race.c` where the `is_pc` argument is `TRUE`, giving 30.
   Reading the race `#define` block alone is not sufficient: it contains
   colliding aliases and unimplemented entries that `add_race()` never
   registers.

## Source authority

- RoL race constants and PC range: `RealmsOfLuminari/src/race_class.h`
- RoL race names, stat factors, racial data, innates, class permissions, and
  hometown matrix: `RealmsOfLuminari/src/race_class.c`
- RoL race selection: `RealmsOfLuminari/src/nanny.c`, `select_race()`;
  `RealmsOfLuminari/lib/creation/racetable`
- RoL lich transformation: `RealmsOfLuminari/src/necro.c`
- LuminariMUD race constants: `src/structs.h`
- LuminariMUD race registration: `src/character/race.c`, `add_race()`
