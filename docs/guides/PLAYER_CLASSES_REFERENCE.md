# Player Classes Reference

Status: source-backed reference, verified 2026-09-11 against `src/character/class.c`,
`src/structs.h`, `src/account.c`, `src/act.other.c`, `src/interpreter.c`,
`src/net/onboarding.c`, `src/magic/spell_prep.c`, `src/utils.c`, `src/utils.h`,
`src/handler.c`, `src/character/perks.c`, `src/character/premadebuilds.c`, and
`src/constants.c`.

This document is the reference center for playable (player-character) classes. It
lists every class a player can hold, how it is acquired, its unlock cost, its
progression numbers, alignment and prerequisite restrictions, class skills, every
feat the class grants by level, the class-feat pool it can buy from, its casting
model, and its full spell, power, invocation, or extract list. It fills the gaps
left by the documents in the next section; it does not repeat what they already
cover. Disabled placeholder classes (IDs 36 and 37) are out of scope.

## Related documents

| Document | What it covers |
|----------|----------------|
| [ADDING_NEW_PLAYER_CLASS_GUIDE.md](ADDING_NEW_PLAYER_CLASS_GUIDE.md) | Developer procedure for adding a class: ID allocation, `load_class_list()` registry, progression wiring, entry paths (creation, `gain`, respec, `accexp`), casting models, help, persistence, tests |
| [PLAYER_RACES_REFERENCE.md](PLAYER_RACES_REFERENCE.md) | Playable races, racial feats, race unlock costs, and the race side of race/class alignment compatibility |
| [ADDING_NEW_RACE_GUIDE.md](ADDING_NEW_RACE_GUIDE.md) | Race registry and the race/class compatibility checks character creation enforces |
| [PLAYER_MANAGEMENT_SYSTEM.md](../systems/PLAYER_MANAGEMENT_SYSTEM.md) | Account model, `account->classes[]` unlock storage, the `CON_QCLASS` / `CON_QCLASS_HELP` creation states, and player file persistence |
| [WEB_ONBOARDING_SYSTEM.md](../systems/WEB_ONBOARDING_SYSTEM.md) | Web character creation, including the class catalog and its eligibility rule |
| [SPELL_PREPARATION_SYSTEM.md](../systems/SPELL_PREPARATION_SYSTEM.md) | Prepared, spontaneous, extract, and psionic casting internals: queues, slots, known lists, preparation events |
| [CASTING_VISUALS_SYSTEM.md](../systems/CASTING_VISUALS_SYSTEM.md) | Per-class casting visual styles |
| [GAME_MECHANICS_SYSTEMS.md](../systems/GAME_MECHANICS_SYSTEMS.md) | Overview of classes, races, skills, feats, spells, and combat mechanics |
| [COMBAT_SYSTEM.md](../systems/COMBAT_SYSTEM.md) | Base attack bonus, attacks per round, and combat feat resolution |
| [NEW_PLAYER_GUIDE_LEVEL_1-5.md](NEW_PLAYER_GUIDE_LEVEL_1-5.md) | Player-facing introduction to picking a race and class |
| [DEVELOPER_GUIDE_AND_API.md](DEVELOPER_GUIDE_AND_API.md) | General developer reference, including the feat and class registries |
| `docs/systems/perks/` | Perk trees for Alchemist, Bard, Blackguard, Cleric, Inquisitor, Monk, Psionicist, Ranger, Rogue, Warrior, Wizard (`*_PERKS.md`, `RANGER_PERK_TREES.md`) |
| `lib/text/help/help.hlp` | Player help. Entries `CLASSES`, `CLASS-ROSTER`, `LOCKED-CLASSES`, `PRESTIGE-CLASSES`, `MULTICLASS`, `GAIN`, `RESPEC`, `ACCEXP`, `PREMADE`, `STUDY`, `PERKS`, and one `CLASS-<NAME>` entry per class that has one (see "Help coverage" below). The same content is mirrored in the help database |
| In-game commands | `class list`, `class info <name>`, `class feats <name>`, `class prereqs <name>`, `gain <class>`, `respec <class> [premade]`, `accexp class`, `accexp class <name>`, `account`, `study`, `perks`, `myperks`, `spells`, `powers`, `extracts`, `feats`, `epicfeats` |

Source of truth for every table below:

| Data | Where it is defined |
|------|---------------------|
| Class IDs and synonyms | `src/structs.h` (`CLASS_*` defines, `NUM_CLASSES`, `NUM_CASTERS`) |
| Registry entries | `load_class_list()` in `src/character/class.c` via `classo()`, `assign_class_saves()`, `assign_class_abils()`, `assign_class_titles()`, `feat_assignment()`, `spell_assignment()`, and `class_prereq_*()` |
| Creation alignment rule | `valid_align_by_class()` in `src/character/class.c`; `valid_class_race_alignment()` in `src/character/race.c` |
| `gain` and respec eligibility | `class_is_available()` and `meets_class_prerequisite()` in `src/character/class.c`; `do_gain()` and `do_respec()` in `src/act.other.c` |
| Creation filters | `nanny()` `CON_QCLASS` in `src/interpreter.c`; `web_onboarding_class_selectable()` region in `src/net/onboarding.c` |
| Unlock and cost | `has_unlocked_class()` and `do_accexp()` in `src/account.c` |
| Per-level gains | `advance_level()` in `src/character/class.c` |
| Stat and hit/dam caps | `compute_char_cap()` in `src/handler.c` |
| Perk routing | `class_to_perk_class()` and `check_stage_advancement()` in `src/character/perks.c` |
| Caster levels and circles | `compute_arcane_level()`, `compute_divine_level()`, `compute_bonus_caster_level()` in `src/utils.c`; `get_class_highest_circle()` and `compute_slots_by_circle()` in `src/magic/spell_prep.c`; slot and known tables in `src/constants.c` |
| Feat text | `feato()` calls in `assign_feats()` in `src/character/feats.c` |
| Spell, power, and invocation text | `spello()`, `spellabilo()` in `src/magic/spell_parser.c`; `psiono()` in `src/magic/psionics.c` |
| Premade builds | `advance_premade_build()` in `src/character/premadebuilds.c` |
| Starting gear | `newbieEquipment()` in `src/character/class.c` |

## How classes work

### Base, prestige, and locked

Every registry entry carries three availability fields: `prestige_class`,
`locked_class`, and `unlock_cost`. They are independent flags, but in the current
registry they line up exactly: the 18 base classes are unlocked with cost 0, and the
18 prestige classes are all locked with a cost. There is no locked base class and no
unlocked prestige class today.

- **Base classes** can be picked at character creation (terminal and web), taken
  with `gain`, and chosen as a respec target.
- **Prestige classes** cannot be picked at creation or as a respec target. They are
  only entered through `gain` once the account has unlocked them and the character
  meets every prerequisite. All prestige classes cap at 10 class levels except
  Knight of the Luminous Thread, which caps at 20.
- **Locked classes** must be bought once per account with `accexp class <name>`.
  The unlock is stored in the account unlock array, which holds up to 50 entries
  (`MAX_UNLOCKED_CLASSES`), and applies to every current and future character on
  the account. `accexp class` with no argument lists the locked, in-game classes
  the account has not yet unlocked, with their cost. The name match uses the
  registry name (for example `accexp class stalwart`). The four knight classes
  also accept their legacy names `knight of solamnia`, `knight of the lily`,
  `knight of the thorn`, and `knight of the skull`.

Unlock costs are 5000 account experience for every prestige class except Knight of
the Luminous Thread, which costs 2500. Account experience is shared with race
unlocks and alignment changes; see the races reference.

### Getting a class

- **Character creation** (`CON_QCLASS`, and the web catalog): the class must be
  in game, must not be prestige, must be unlocked if locked, and must share at
  least one legal alignment with the chosen race (`valid_class_race_alignment()`,
  which combines `valid_align_by_class()` with the race alignment table). The
  terminal menu only lists classes that pass; direct input is rejected with a
  message for each failed check. Picking a class shows its `class-<name>` help
  entry before continuing.
- **`gain <class>`**: advances one level in a class the character already has or
  qualifies for. The character must be in normal form (no disguise, wildshape, or
  polymorph), must be below level 30 (`LVL_IMMORT - 1`), and must have the
  experience for the next level. `class_is_available()` then requires: the class
  is in game, the class level cap is not reached, the class is unlocked, and the
  prerequisite list passes. After that `gain` enforces the Cleric/Inquisitor
  exclusion (a character with levels in one can never take the other) and the
  multiclass cap (`MULTICAP` is 3: a fourth distinct class is refused).
- **`respec <class> [premade]`**: resets the character to level 1 in a base
  class. Requires character level 2 or higher, not staff, no group, master, or
  followers, and rejects any locked class (the message calls it a prestige
  class). Eligibility is evaluated by `class_is_available()` in respec mode,
  which is identical to normal mode except that it skips the disabled epic-race
  multiclass check.
- **First class**: `do_start()` puts the creation class at level 1; every later
  level is a `gain`.

### Prerequisites and how they are evaluated

Prerequisite rows are attached with `class_prereq_*()`. `class_is_available()`
walks the list: alignment rows form one OR group, race rows form one OR group, and
every other row (feat, combat feat, BAB, ability ranks, class level, casting) must
pass individually. `class prereqs <name>` prints the same list with a
fulfilled/missing marker per row and a final availability verdict.

Row semantics worth knowing:

- **BAB** compares against `ACTUAL_BAB()`, the character's real base attack bonus
  from all classes.
- **Ability ranks** compare trained ranks in the named skill.
- **Feat** requires the named feat at the given rank count.
- **Combat feat with `CFEAT_SPECIAL_BOW`** (Arcane Archer) requires Weapon Focus
  applied to the ranged weapon family.
- **Spellcasting** rows carry a casting type, a preparation type, and a minimum
  circle. Arcane means Wizard, Sorcerer, Summoner, or Bard levels; divine means
  Cleric, Druid, Inquisitor, Paladin, Blackguard, or Ranger levels; any means any
  positive caster level. The circle check passes if any class of that type has a
  slot at that circle. Preparation type `PREP_TYPE_ANY` (used by every current
  row) accepts either model; `PREP_TYPE_PREPARED` would require Wizard levels and
  `PREP_TYPE_SPONTANEOUS` would forbid them. Warlock, Alchemist, Psionicist, and
  Artificer levels do not satisfy the arcane or divine type checks.
- **Class level** requires the given number of levels in another class (only
  Shifter uses this: 10 Druid levels).

### Alignment

Two separate mechanisms restrict alignment. `valid_align_by_class()` is the
creation-time rule and also feeds `valid_class_race_alignment()`. Alignment
prerequisite rows are the `gain` rule. They agree for Monk, Druid, Berserker,
Bard, Paladin, Blackguard, Necromancer, and Knight of the Luminous Thread. They
disagree in three places, recorded under "Registry observations". Alignment
changes after creation go through `accexp align`.

### Experience, level cap, and epic play

All 36 classes share one experience curve in `level_exp()`; there is no per-class
experience penalty. Race multipliers apply on top (advanced races x2, epic races
x7, Lich and Vampire x10), then the configured experience multiplier. Level 30 is
the player cap. Character level 21 and above is epic (`IS_EPIC()`), which switches
general feat gains to epic feats and enables epic class feats.

## Per-level gains

`advance_level()` applies the following on every level, using the class the level
is taken in:

| Gain | Rule |
|------|------|
| Hit points | full class hit die (no roll) + CON bonus + configured extra HP per level + racial HP bonus + 1 per Toughness rank (+1 for Gold Dwarf Toughness) |
| Movement | random 10 to (class move gain x 10) + configured extra movement + 10-20 for Endurance, Draconian Gallop, or Fast Movement, + 20 for Wood Elf Fleetness |
| Skill points (trains) | max(1, class trains + INT bonus), +1 for humans |
| Power points | Psionicist levels only: character level + 2, +INT bonus on the first psionicist level, +1 with Proficient Psionicist |
| General feat | 1 at every character level divisible by 3 (epic feat instead when epic) |
| Stat boost | 1 at every character level divisible by 4 |
| Class feat | per-class bonus rules listed in the class detail sections |
| Epic class feat | 1 when epic and the class level is divisible by the class's epic class feat interval (0 disables; all prestige classes are 0) |
| Fixed BAB | at character level 20 the base attack bonus is frozen as the sum over classes of level x 1.0 (high), x 0.75 (medium), or x 0.5 (low), capped at 20 levels per class |
| Perk points | see below |

### Perk points

Each character level is split into 4 experience stages. Reaching each of stages 1,
2, and 3 awards 2 perk points to the class most recently gained (`GET_CLASS()`),
so a level yields 6 perk points. `class_to_perk_class()` routes the award: base
classes receive both points in their own tree, single-parent prestige classes
feed one parent tree, and hybrid prestige classes split the two points across
two parent trees. The routing per prestige class is listed in the prestige table.
Perk trees are documented in `docs/systems/perks/` for the classes that have a
document; the other base trees (Berserker, Druid, Paladin, Sorcerer, Summoner,
Warlock, Artificer) exist in `src/character/perks.c` only.

### Stat and hit/dam caps

`compute_char_cap()` raises the per-character caps on ability scores, hitroll, and
damroll by class level. The per-class formulas are listed in the class detail
sections ("Cap bonus"). Levels in several classes stack their bonuses.

### Skills

Every skill is class (`CA`), cross-class (`CC`), or unavailable (`NA`) for a
class. With levels in several classes, the best rating applies. In `study`, class
skills can be trained to (character level + 3) ranks for 1 skill point each;
cross-class skills stop at (character level + 3) / 2 ranks and cost 2 points each.
No current class marks any skill unavailable. Arcana, Religion, History, Boarding,
Survival, and Linguistics are class skills for every class (hardcoded in
`assign_class_abils()`), and so is Heal, which every registry entry marks `CA`.
The skill lists below therefore show only the remaining skills that a class has as
class skills; anything not listed is cross-class.

## Base classes at a glance

All base classes are always available, cost nothing, and have no level cap other
than 30 unless noted. Saves list the classes with a good (high) progression.

| Class | ID | Abbrev | BAB | Hit die | Move | Trains | Good saves | Epic class feat every | Max | Alignment |
|-------|----|--------|-----|---------|------|--------|------------|-----------------------|-----|-----------|
| Warrior | 3 | War | high | d10 | 1 | 2 | Fort | 2 | 30 | any |
| Rogue | 2 | Rog | high | d8 | 2 | 8 | Refl | 4 | 30 | any |
| Monk | 4 | Mon | high | d8 | 2 | 4 | Fort, Refl, Will | 3 | 30 | lawful only (LG, LN, LE) |
| Berserker | 6 | Bes | high | d12 | 2 | 4 | Fort | 3 | 30 | non-lawful only |
| Ranger | 9 | Ran | high | d10 | 3 | 4 | Fort | 3 | 30 | any |
| Paladin | 8 | Pal | high | d10 | 1 | 2 | Will | 3 | 30 | LG only |
| Blackguard | 24 | BkG | high | d10 | 1 | 2 | Will | 3 | 30 | evil only (LE, NE, CE) |
| Cleric | 1 | Cle | medium | d8 | 1 | 2 | Fort, Will | 3 | 30 | any |
| Druid | 5 | Dru | medium | d8 | 3 | 4 | Fort, Will | 4 | 30 | neutral on one axis (NG, LN, TN, CN, NE) |
| Inquisitor | 26 | Inq | medium | d8 | 1 | 6 | Fort, Will, Death | 3 | 30 | any |
| Wizard | 0 | Wiz | low | d6 | 1 | 2 | Will | 3 | 30 | any |
| Sorcerer | 7 | Sor | low | d6 | 1 | 2 | Will | 3 | 30 | any |
| Bard | 10 | Bar | medium | d8 | 2 | 6 | Refl, Will | 3 | 30 | non-lawful only |
| Summoner | 27 | Sum | medium | d8 | 1 | 2 | Will, Death | 3 | 30 | any |
| Warlock | 28 | Wlk | medium | d6 | 1 | 2 | Will, Death | 3 | 30 | any |
| Alchemist | 17 | Alc | medium | d8 | 1 | 4 | Fort, Refl, Poison | 3 | 30 | any |
| Psionicist | 21 | Psn | low | d6 | 1 | 2 | Will, Death | 3 | 30 | any |
| Artificer | 35 | Art | medium | d6 | 1 | 5 | Fort, Will | 3 | 20 | any |

Primary attributes as stated in the registry:

| Class | Primary attribute note |
|-------|------------------------|
| Warrior | Strength, alternatively Dex... Con for survivability, 13 Int unlocks feat chains |
| Rogue | Dexterity, Con for survivability, Int for skills, Str for combat |
| Monk | Wisdom, Con/Dex for survivability, Str for combat |
| Berserker | Strength, Con/Dex for survivability - Con helps some of their skills |
| Ranger | Dexterity or Str, Con for survivability, they need a little Wis for spellcasting |
| Paladin | Charisma, Con for survivability, Str for combat |
| Blackguard | Charisma, Con for survivability, Str for combat |
| Cleric | Wisdom, Cha affects some of their abilities.. Con for survivability, Str for combat |
| Druid | Wisdom, Con/Dex for survivability, Str for combat |
| Inquisitor | Wisdom, Con/Dex for survivability, Str for combat |
| Wizard | Intelligence, Con/Dex for survivability |
| Sorcerer | Charisma, Con/Dex for survivability |
| Bard | Charisma, Int for skills, Con/Dex for survivability |
| Summoner | Charisma, Con/Dex for survivability, Str for combat |
| Warlock | Charisma, Con/Dex for survivability, Str for combat |
| Alchemist | Intelligence, Con/Dex for survivability, Str for combat |
| Psionicist | Intelligence, Dex/Con for survivability |
| Artificer | Intelligence for device creation, Dexterity and Constitution for survivability |

## Prestige classes at a glance

All prestige classes are locked, entered only through `gain`, and capped at 10
class levels unless noted. Alignment here is the `gain` prerequisite; "any" means
no alignment row is registered.

| Class | ID | Abbrev | Cost | Max | BAB | Hit die | Move | Trains | Good saves | Alignment | Perk routing |
|-------|----|--------|------|-----|-----|---------|------|--------|------------|-----------|--------------|
| Weaponmaster | 11 | WpM | 5000 | 10 | high | d10 | 1 | 2 | Refl | any | Warrior |
| Stalwart Defender | 13 | SDe | 5000 | 10 | high | d12 | 1 | 2 | Fort, Will | any | Warrior |
| Duelist | 15 | Due | 5000 | 10 | high | d10 | 1 | 4 | Refl | any | Warrior + Rogue |
| Dragonrider | 34 | DRr | 5000 | 10 | high | d10 | 1 | 2 | Fort, Refl, Will, Poison, Death | any | Warrior |
| Arcane Archer | 12 | ArA | 5000 | 10 | high | d10 | 1 | 4 | Fort, Refl | any | Warrior + Wizard |
| Eldritch Knight | 20 | EKn | 5000 | 10 | high | d10 | 2 | 2 | Fort, Poison | any | Wizard + Warrior |
| Spellsword | 22 | SSw | 5000 | 10 | high | d8 | 2 | 2 | Fort, Will, Poison | any | Wizard + Warrior |
| Arcane Shadow | 18 | ArS | 5000 | 10 | medium | d8 | 2 | 5 | Refl, Will | NG, TN, NE, CE, CG, CN | Wizard + Rogue |
| Shadowdancer | 23 | ShD | 5000 | 10 | medium | d8 | 1 | 6 | Refl, Poison | any | Rogue |
| Assassin | 25 | Asn | 5000 | 10 | medium | d8 | 2 | 4 | Refl | any | Rogue |
| Sacred Fist | 19 | SaF | 5000 | 10 | medium | d8 | 4 | 4 | Fort, Refl | any | Monk + Cleric |
| Shifter | 14 | Shf | 5000 | 10 | medium | d8 | 1 | 4 | Fort, Refl, Poison | any | Druid |
| Mystic Theurge | 16 | MTh | 5000 | 10 | low | d4 | 1 | 2 | Will | any | Wizard + Cleric |
| Necromancer | 29 | Nec | 5000 | 10 | low | d6 | 2 | 4 | Will, Death | LE, NE, CE, LN, TN, CN | Wizard + Cleric |
| Knight of the Luminous Thread | 30 | KLT | 2500 | 20 | high | d10 | 1 | 2 | Fort, Will | LG | Warrior + Cleric |
| Knight of the Howling Moon | 33 | KHM | 5000 | 10 | medium | d10 | 1 | 2 | Fort, Will, Poison, Death | any | Warrior + Wizard |
| Knight of the Shattered Mirror | 31 | KSM | 5000 | 10 | medium | d6 | 1 | 2 | Will, Poison, Death | any | Wizard + Warrior |
| Knight of the Pale Throne | 32 | KPT | 5000 | 10 | medium | d10 | 1 | 2 | Fort, Will, Poison, Death | any | Warrior + Cleric |

Registry name synonyms: `CLASS_WEAPONMASTER` is `CLASS_WEAPON_MASTER`,
`CLASS_ARCANEARCHER` is `CLASS_ARCANE_ARCHER`, `CLASS_STALWARTDEFENDER` is
`CLASS_STALWART_DEFENDER`, `CLASS_MYSTICTHEURGE` is `CLASS_MYSTIC_THEURGE`,
`CLASS_SHADOWDANCER` is `CLASS_SHADOW_DANCER`, `CLASS_PSION` is
`CLASS_PSIONICIST`, and `CLASS_PALE_MASTER` is `CLASS_NECROMANCER`. The four
knight classes keep their Dragonlance constant names (`CLASS_KNIGHT_OF_SOLAMNIA`,
`CLASS_KNIGHT_OF_THE_THORN`, `CLASS_KNIGHT_OF_THE_SKULL`, `CLASS_KNIGHT_OF_THE_LILY`)
while their registry names are Knight of the Luminous Thread, Knight of the
Shattered Mirror, Knight of the Pale Throne, and Knight of the Howling Moon.
`parse_class_long()` accepts both spellings of each, with or without hyphens.

## Prestige class prerequisites

Rows are listed in registry order. Alignment and race rows are OR groups; every
other row is required.

### Weaponmaster

- feat weapon focus
- feat combat expertise
- feat dodge
- feat mobility
- feat spring attack
- feat whirlwind attack
- base attack bonus 5
- 4 ranks in Intimidate

### Stalwart Defender

- base attack bonus 7
- feat dodge
- feat endurance
- feat toughness
- feat medium armor proficiency
- feat light armor proficiency

### Duelist

- base attack bonus 6
- 2 ranks in Acrobatics
- 2 ranks in Perform
- feat dodge
- feat mobility
- feat weapon finesse

### Dragonrider

- base attack bonus 8
- 10 ranks in Ride
- feat mounted combat

### Arcane Archer

- base attack bonus 5
- feat point blank shot
- feat precise shot
- arcane spellcasting with circle 1 slots
- weapon focus applied to a ranged (bow) weapon
- race: one of Drow, Moon Elf, Half Elf (checked against the current race, so a Moon Elf or Half Elf qualifies, but High Elf, Wild Elf, and Half Drow do not)
- the arcane spellcasting row asks for circle 1 slots, so any level of Wizard, Sorcerer, Bard, or Summoner qualifies

### Eldritch Knight

- arcane spellcasting with circle 3 slots
- feat martial weapon proficiency

### Spellsword

- arcane spellcasting with circle 2 slots
- base attack bonus 4
- 6 ranks in Arcana
- feat martial weapon proficiency
- feat simple weapon proficiency
- feat light armor proficiency
- feat medium armor proficiency
- feat heavy armor proficiency

### Arcane Shadow

- 4 ranks in Disable Device
- 4 ranks in Escape Artist
- 4 ranks in Spellcraft
- arcane spellcasting with circle 2 slots
- feat sneak attack x2
- alignment: one of NG, TN, NE, CE, CG, CN

### Shadowdancer

- feat combat reflexes
- feat dodge
- 5 ranks in Acrobatics
- 2 ranks in Perform

### Assassin

- 5 ranks in Stealth
- 2 ranks in Sense Motive
- feat two weapon fighting

### Sacred Fist

- divine spellcasting with circle 1 slots
- feat ki strike
- feat combat casting
- base attack bonus 4
- 8 ranks in Religion

### Shifter

- 10 levels of Druid

### Mystic Theurge

- arcane spellcasting with circle 2 slots
- divine spellcasting with circle 2 slots
- 6 ranks in Arcana
- 6 ranks in Religion
- 6 ranks in Spellcraft

### Necromancer

- 5 ranks in Arcana
- 5 ranks in Religion
- any spellcasting with circle 4 slots
- alignment: one of LE, NE, CE, LN, TN, CN

### Knight of the Luminous Thread

- base attack bonus 3
- divine spellcasting with circle 1 slots
- 4 ranks in Religion
- 4 ranks in History
- 4 ranks in Diplomacy
- 4 ranks in Ride
- feat heavy armor proficiency
- feat shield armor proficiency
- feat endurance
- feat iron will
- alignment: one of LG

### Knight of the Howling Moon

- base attack bonus 2
- 2 ranks in Religion
- 3 ranks in Intimidate
- feat honorbound

### Knight of the Shattered Mirror

- base attack bonus 3
- 8 ranks in Arcana
- 2 ranks in Religion
- 8 ranks in Spellcraft
- feat heavy armor proficiency
- feat martial weapon proficiency
- arcane spellcasting with circle 2 slots

### Knight of the Pale Throne

- base attack bonus 3
- 4 ranks in Religion
- feat alertness
- feat iron will
- divine spellcasting with circle 3 slots

## Magic by class

### Casting models

| Class | Model | Stat | Circle access | Table |
|-------|-------|------|---------------|-------|
| Wizard | prepared (memorization), arcane | INT | circle (level+1)/2, max 9; epic spells at 21 | `wizard_slots` |
| Cleric | prepared, divine; two domains | WIS | circle (level+1)/2, max 9; epic spells at 21 | `cleric_slots` |
| Druid | prepared, divine | WIS | circle (level+1)/2, max 9; epic spells at 21 | `druid_slots` |
| Paladin | prepared, divine | CHA | no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15 | `paladin_slots` |
| Blackguard | prepared, divine | CHA | no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15 | `paladin_slots (shared)` |
| Ranger | prepared, divine | WIS | no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15 | `ranger_slots` |
| Sorcerer | spontaneous, arcane; bloodlines | CHA | circle level/2 (min 1), max 9; epic spells at 21 | `sorcerer_known doubles as the slot table` |
| Bard | spontaneous, arcane | CHA | circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21 | `bard_slots / bard_known` |
| Inquisitor | spontaneous, divine; one domain | WIS | circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21 | `inquisitor_slots / inquisitor_known` |
| Summoner | spontaneous, arcane; eidolon | CHA | circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21 | `summoner_slots / summoner_known` |
| Alchemist | extracts (prepared concoctions) | INT | circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16 | `alchemist_slots` |
| Warlock | at-will invocations, arcane | CHA | circle 1 at 1-5, 2 at 6-10, 3 at 11-15, 4 at 16+ | `warlock_known` |
| Psionicist | psionic powers fuelled by power points (PSP) | INT | circle (level+1)/2, max 9; epic psionics at 21, 25, 29 | `PSP pool: +(character level + 2) per psionicist level, +INT bonus at level 1, +1 with Proficient Psionicist` |
| Artificer | weird science devices built from the wizard and cleric spell lists | INT | device spell level 1 from 1, 2 at 3, 3 at 5, 4 at 11 | `weird_science_table` |

Slots per circle come from the class table indexed by class level plus bonus
caster levels, plus the `spell_bonus` row for the casting stat, plus any perk or
feat bonus. Sorcerer New Arcana adds one slot at each chosen circle. Warlock
invocations are at-will and limited by known count, not slots. Monk, Warrior,
Rogue, Berserker, and every prestige class have no spell list of their own.
Artificer devices draw from the wizard and cleric lists rather than from a class
spell assignment.

### Caster levels and prestige advancement

Arcane caster level (`compute_arcane_level()`) is the sum of Wizard, Sorcerer,
Bard, Summoner, Warlock, Arcane Shadow, Knight of the Shattered Mirror,
Shadowdancer, and Eldritch Knight levels, plus three quarters of Arcane Archer
levels, half of Mystic Theurge levels, half of Spellsword levels (rounded up),
Necromancer levels when the necromancer casting type is arcane, and the Arcana
Golem racial bonus. Divine caster level (`compute_divine_level()`) is the sum of
Cleric, Druid, Inquisitor, Sacred Fist, and Knight of the Pale Throne levels, plus
Knight of the Luminous Thread levels above 5, Paladin, Blackguard, and Ranger
levels above 3, half of Mystic Theurge levels, and Necromancer levels when the
casting type is divine.

Slot and circle progression for a parent class add bonus levels from
`compute_bonus_caster_level()`:

| Parent classes | Bonus levels added |
|----------------|--------------------|
| Wizard, Sorcerer, Bard, Summoner | Arcane Archer x 3/4, Arcane Shadow, Eldritch Knight, Spellsword (level + 1) / 2, Mystic Theurge, Knight of the Shattered Mirror, Necromancer (if it advances that class) |
| Cleric, Druid, Ranger, Paladin, Inquisitor | Mystic Theurge, Sacred Fist, Knight of the Luminous Thread, Knight of the Pale Throne, Necromancer (if it advances that class) |

Which parent a hybrid prestige class advances is chosen in `study`: "Preferred
Caster Classes (Prestige)" sets a preferred arcane class (default Wizard) and a
preferred divine class (default Cleric). Necromancer instead sets a casting type
(arcane or divine) and advances the preferred class of that type if the character
has levels in it, otherwise the single class of that type the character has; if
the character has several and none is preferred, nothing is advanced. Mystic
Theurge grants the circle-access feats of every parent class as its own levels
rise (`GRANT_SPELL_CIRCLE` in `process_conditional_class_level_feats()`).

Blackguard shares the Paladin slot table. Necromancer, Mystic Theurge, Eldritch
Knight, Spellsword, Arcane Archer, Arcane Shadow, Sacred Fist, and the four knights
have no spell list of their own; they only extend a parent class.

## Base class details

### Warrior

A master of weapons, armor and maximizing the use of both in battle.

- **Primary attribute**: Strength, alternatively Dex... Con for survivability, 13 Int unlocks feat chains
- **Class skills**: Intimidate, Concentration, Discipline, Total Defense, Ride, Athletics, Diplomacy
- **Extra feat rule**: bonus class feat at warrior level 1 and every even level up to 20
- **Cap bonus**: STR, CON +(level/4+1); hitroll/damroll +level/3
- **Starting gear**: scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)

Feats granted by class level (a repeated feat adds a rank):

- L1: martial weapon proficiency; heavy armor proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; tower shield proficiency; simple weapon proficiency
- L3: armor training
- L5: weapon training
- L7: armor training
- L9: weapon training
- L11: armor training
- L13: weapon training
- L15: armor training
- L17: weapon training
- L19: stalwart warrior
- L21: armor mastery i
- L24: weapon mastery i
- L27: armor mastery ii
- L30: weapon mastery ii

Class-feat pool (feats purchasable with warrior class feat points):

armor skin, armor specialization (light), armor specialization (medium), armor specialization (heavy), blind fighting, cleave, combat expertise, combat reflexes, deflect arrows, epic damage reduction, dodge, exotic weapon proficiency, far shot, great cleave, dazzling display, vital strike, improved vital strike, greater vital strike, greater two weapon fighting, greater weapon focus, greater weapon specialization, improved bull rush, improved critical, improved disarm, improved feint, improved grapple, improved initiative, improved overrun, improved precise shot, improved shield punch, knockdown, shield charge, shield slam, improved sunder, improved trip, improved two weapon fighting, improved unarmed strike, manyshot, mobility, mounted archery, mounted combat, point blank shot, power attack, deadly aim, precise shot, quick draw, rapid reload, rapid shot, ride by attack, robilars gambit, shot on the run, snatch arrows, spirited charge, spring attack, stunning fist, swarm of arrows, trample, two weapon defense, two weapon fighting, oversized two weapon fighting, weapon finesse, weapon focus, weapon specialization, whirlwind attack, fast healing, weapon mastery i, weapon flurry, weapon supremacy, double weapon focus, double weapon specialization, double weapon critical, double weapon defense, epic prowess, great strength, great dexterity, great constitution, epic toughness, overwhelming critical, devastating critical, epic weapon specialization.

### Rogue

A skilled agent who strikes from the shadows and is skilled in many areas.

- **Primary attribute**: Dexterity, Con for survivability, Int for skills, Str for combat
- **Class skills**: Acrobatics, Stealth, Perception, Appraise, Total Defense, Ride, Athletics, Sleight of Hand, Bluff, Diplomacy, Disable Device, Disguise, Escape Artist, Sense Motive, Use Magic Device
- **Cap bonus**: DEX +(level/4+1); STR, INT +(level/8+1); hitroll/damroll +level/3
- **Starting gear**: leather sleeves, leather leggings, studded leather, two daggers
- **Premade build**: supported (`respec <class> premade`, or premade at creation)

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; weapon proficiency - rogues; light armor proficiency; weapon finesse; sneak attack; trapfinding
- L2: evasion
- L3: sneak attack; trap sense; slippery mind
- L4: uncanny dodge
- L5: sneak attack
- L6: trap sense; crippling strike
- L7: sneak attack
- L8: improved uncanny dodge
- L9: sneak attack; trap sense; improved evasion
- L10: apply poison
- L11: sneak attack
- L12: trap sense; able learner
- L13: sneak attack
- L15: sneak attack; trap sense; defensive roll
- L17: sneak attack
- L18: trap sense; dirt kick
- L19: sneak attack
- L21: sneak attack; backstab
- L22: trap sense
- L23: sneak attack
- L24: sap
- L25: sneak attack
- L26: trap sense
- L27: sneak attack; vanish
- L29: sneak attack
- L30: trap sense; improved vanish

Class-feat pool: great dexterity.

### Monk

A disciplined warrior who has mastered the mind and body and can kill with just their fist.

- **Primary attribute**: Wisdom, Con/Dex for survivability, Str for combat
- **Class skills**: Acrobatics, Stealth, Perception, Concentration, Discipline, Total Defense, Ride, Athletics, Escape Artist
- **Cap bonus**: WIS, DEX +(level/4+1); hitroll/damroll +level/3
- **Starting gear**: cloth robes
- **Premade build**: supported (`respec <class> premade`, or premade at creation)

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - monks; monk wisdom bonus to AC; monk innate ac bonus; unarmed strike; improved unarmed strike; flurry of blows; stunning fist
- L2: evasion
- L3: ki strike; still mind
- L4: slow fall
- L5: slow fall; purity of body; spring attack
- L6: slow fall; ki strike
- L7: wholeness of body
- L8: slow fall
- L9: ki strike; improved evasion
- L10: slow fall
- L11: diamond body; greater flurry
- L12: abundant step; slow fall; ki strike
- L13: diamond soul
- L14: slow fall
- L15: quivering palm; ki strike
- L16: timeless body
- L17: tongue of the sun and moon
- L18: slow fall; ki strike
- L19: empty body
- L20: perfect self; slow fall
- L21: slow fall
- L23: keen strike
- L24: slow fall
- L26: slow fall; blinding speed
- L28: slow fall
- L29: outsider
- L30: slow fall

Class-feat pool: great wisdom, armor skin, epic prowess, epic toughness.

### Berserker

A wild warrior who relies on brute strength and their ability to enter a battle rage.

- **Primary attribute**: Strength, Con/Dex for survivability - Con helps some of their skills
- **Class skills**: Intimidate, Discipline, Ride, Athletics, Escape Artist
- **Cap bonus**: STR, CON +(level/4+1) plus rage bonus; hitroll/damroll +level/3 (+5 more while raging)
- **Starting gear**: scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; martial weapon proficiency; fast movement; rage
- L2: uncanny dodge
- L3: trap sense; surprise accuracy
- L4: rage; shrug damage
- L5: improved uncanny dodge
- L6: trap sense; powerful blow
- L7: shrug damage
- L8: rage
- L9: trap sense; renewed vigor
- L10: shrug damage
- L11: greater rage; rage
- L12: trap sense; heavy shrug
- L13: shrug damage
- L14: indomitable will
- L15: trap sense; rage; fearless rage
- L16: shrug damage
- L17: tireless rage
- L18: trap sense; come and get me
- L19: shrug damage
- L20: rage; mighty rage
- L22: eater of magic; shrug damage
- L24: rage
- L25: shrug damage
- L26: rage resistance
- L27: indomitable rage
- L28: shrug damage
- L29: rage
- L30: deathless frenzy; raging critical

Class-feat pool: armor skin, epic prowess, epic toughness, great strength, great constitution, chaotic rage, overwhelming critical, devastating critical.

### Ranger

A woodland warrior who is master of the bow and dual weapons, and specializes against specific foes.

- **Primary attribute**: Dexterity or Str, Con for survivability, they need a little Wis for spellcasting
- **Class skills**: Stealth, Perception, Intimidate, Concentration, Discipline, Total Defense, Ride, Athletics, Escape Artist, Handle Animal
- **Cap bonus**: DEX +(level/4+1); WIS, STR +(level/8+1); hitroll/damroll +level/3
- **Starting gear**: studded leather, leather sleeves, leather leggings, long sword, shield
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared, divine; no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; martial weapon proficiency; favored enemy available
- L2: wild empathy
- L3: dual weapon fighting
- L4: point blank shot; endurance; animal companion
- L5: favored enemy available
- L6: improved dual weapon fighting; 1st circle ranger spells
- L7: rapid shot
- L8: wilderness stride
- L9: track
- L10: evasion; favored enemy available; 2nd circle ranger spells
- L11: greater dual weapon fighting
- L12: manyshot; 3rd circle ranger spells
- L13: camouflage
- L14: swift tracker
- L15: favored enemy available; 4th circle ranger spells
- L17: hide in plain sight
- L20: favored enemy available
- L21: perfect dual weapon fighting
- L22: epic manyshot
- L23: improved evasion
- L25: favored enemy available
- L26: bane of enemies
- L29: epic favored enemy
- L30: favored enemy available

Class-feat pool: epic prowess, epic toughness, armor skin, great strength, great dexterity, death of enemies.

### Paladin

A righteous warrior who protects the innocent with spells, weapons, and as a champion against evil.

- **Primary attribute**: Charisma, Con for survivability, Str for combat
- **Class skills**: Intimidate, Concentration, Discipline, Total Defense, Ride, Diplomacy, Handle Animal, Sense Motive
- **Cap bonus**: STR, CHA +(level/4+1); hitroll/damroll +level/3
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, shield, scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared, divine; no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; heavy armor proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; martial weapon proficiency; aura of good; detect evil; smite evil
- L2: divine grace
- L3: lay on hands
- L4: channel energy; aura of courage
- L5: divine health; mounted combat; smite evil
- L6: ride by attack; remove disease; 1st circle paladin spells
- L7: call mount
- L8: divine bond
- L9: spirited charge; remove disease
- L10: smite evil; 2nd circle paladin spells
- L11: aura of justice
- L12: remove disease; 3rd circle paladin spells
- L13: mounted archery
- L14: remove disease; aura of faith
- L15: smite evil; 4th circle paladin spells
- L17: aura of righteousness
- L18: remove disease
- L19: glorious rider; smite evil
- L20: holy warrior
- L21: legendary rider
- L22: remove disease
- L25: smite evil
- L26: remove disease
- L27: epic mount
- L30: remove disease; smite evil; holy champion

Class-feat pool: epic prowess, epic toughness, armor skin, great strength, great charisma, overwhelming critical, devastating critical.

### Blackguard

Blackguards, also referred to as antipaladins, are the quintessential champions of evil.

- **Primary attribute**: Charisma, Con for survivability, Str for combat
- **Class skills**: Stealth, Intimidate, Concentration, Spellcraft, Discipline, Total Defense, Ride, Bluff, Disguise, Handle Animal, Sense Motive
- **Cap bonus**: STR, CHA +(level/4+1); hitroll/damroll +level/3
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, shield, scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared, divine; no spells before level 6; circle 1 at 6, 2 at 10, 3 at 12, 4 at 15. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; heavy armor proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; martial weapon proficiency; aura of evil; detect good; smite good
- L2: unholy resilience; touch of corruption
- L3: aura of cowardice; plague bringer
- L4: channel energy; smite good
- L5: fiendish boon
- L6: improved cruelties; 1st circle blackguard spells
- L7: smite good; call mount
- L8: aura of despair
- L10: smite good; 2nd circle blackguard spells
- L11: aura of vengeance
- L12: 3rd circle blackguard spells
- L13: smite good
- L14: aura of sin; advanced cruelties; aura of depravity
- L15: 4th circle blackguard spells
- L16: smite good
- L19: smite good
- L20: unholy warrior
- L22: smite good
- L23: master cruelties
- L25: smite good
- L27: epic mount
- L28: smite good
- L30: unholy champion; epic cruelties

Class-feat pool: epic prowess, epic toughness, armor skin, great strength, great charisma, overwhelming critical, devastating critical.

### Cleric

A servant of a deity whose divine magic is unparalled.

- **Primary attribute**: Wisdom, Cha affects some of their abilities.. Con for survivability, Str for combat
- **Class skills**: Concentration, Spellcraft, Appraise, Total Defense, Ride, Diplomacy
- **Cap bonus**: STR, CHA, WIS +(level/4+1)
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, shield, scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared, divine; two domains; circle (level+1)/2, max 9; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; heavy armor proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; channel energy; 1st circle cleric spells
- L3: 2nd circle cleric spells
- L5: 3rd circle cleric spells
- L7: 4th circle cleric spells
- L9: 5th circle cleric spells
- L11: 6th circle cleric spells
- L13: 7th circle cleric spells
- L15: 8th circle cleric spells
- L17: 9th circle cleric spells
- L21: epic cleric spells

Class-feat pool: mummy dust, great wisdom, great charisma.

Domain spells and domain feats are (re)applied by `init_class()` on the first
cleric level and on every login.

### Druid

A keeper of the wild using spells and the ability to shapechange into many different creatures.

- **Primary attribute**: Wisdom, Con/Dex for survivability, Str for combat
- **Class skills**: Perception, Concentration, Spellcraft, Total Defense, Ride, Diplomacy, Handle Animal, Sense Motive
- **Cap bonus**: STR, DEX, WIS +(level/4+1)
- **Starting gear**: leather sleeves, leather leggings, steel scimitar, wooden shield, studded leather
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared, divine; circle (level+1)/2, max 9; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - druids; light armor proficiency; medium armor proficiency; shield armor proficiency; animal companion; nature sense; 1st circle druid spells
- L2: wild empathy; wilderness stride
- L3: trackless step; 2nd circle druid spells
- L4: resist nature's lure; wild shape
- L5: 3rd circle druid spells
- L6: wild shape; wild shape ii
- L7: 4th circle druid spells
- L8: wild shape; wild shape iii
- L9: venom immunity; 5th circle druid spells
- L10: wild shape; wild shape iv
- L11: 6th circle druid spells
- L12: wild shape; wild shape v
- L13: a thousand faces; 7th circle druid spells
- L14: wild shape
- L15: timeless body; 8th circle druid spells
- L16: wild shape
- L17: 9th circle druid spells
- L18: wild shape
- L20: wild shape
- L21: epic druid spells
- L22: wild shape
- L24: wild shape
- L26: wild shape
- L28: wild shape
- L30: wild shape

Class-feat pool: mummy dust, gargantuan wild shape, colossal wild shape, great wisdom, automatic quicken spell, automatic silent spell, automatic still spell.

### Inquisitor

Grim and determined, the inquisitor roots out enemies of the faith, using trickery and guile when righteousness and purity is not enough.

- **Primary attribute**: Wisdom, Con/Dex for survivability, Str for combat
- **Class skills**: Stealth, Perception, Intimidate, Concentration, Spellcraft, Discipline, Ride, Athletics, Bluff, Diplomacy, Disguise, Sense Motive
- **Cap bonus**: STR, CHA, WIS +(level/4+1)
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, shield, scale mail
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: spontaneous, divine; one domain; circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; inquisitor weapon proficiency; light armor proficiency; medium armor proficiency; shield armor proficiency; 1st circle inquisitor spells; judgement; monster lore; stern gaze
- L2: cunning initiative; detect evil; detect good; track
- L3: solo tactics; teamwork
- L4: 2nd circle inquisitor spells; judgement
- L5: hunter's bane
- L6: teamwork
- L7: 3rd circle inquisitor spells; judgement
- L8: second judgement
- L9: teamwork
- L10: 4th circle inquisitor spells; judgement
- L11: stalwart
- L12: greater bane; teamwork
- L13: 5th circle inquisitor spells; judgement
- L14: exploit weakness
- L15: teamwork
- L16: 6th circle inquisitor spells; judgement; third judgement
- L17: slayer
- L18: teamwork
- L19: judgement; true judgement
- L21: epic inquisitor spells
- L22: judgement
- L24: fourth judgement
- L25: judgement
- L28: judgement
- L29: fifth judgement
- L30: perfect judgement

Class-feat pool: great wisdom, great strength.

Domain spells and domain feats are (re)applied by `init_class()` like the
Cleric. Inquisitor and Cleric levels are mutually exclusive in `gain`.

### Wizard

A skilled user of magic whose knowledge of the arcane is unmatched.

- **Primary attribute**: Intelligence, Con/Dex for survivability
- **Class skills**: Concentration, Spellcraft, Appraise, Ride, Sense Motive, Use Magic Device
- **Extra feat rule**: bonus class feat at every 5th wizard level up to 20
- **Cap bonus**: INT, DEX, CON +(level/4+1)
- **Starting gear**: wizard note, spellbook
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: prepared (memorization), arcane; circle (level+1)/2, max 9; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - wizards; scribe scroll; summon familiar; 1st circle wizard spells
- L3: 2nd circle wizard spells
- L5: 3rd circle wizard spells
- L7: 4th circle wizard spells
- L9: 5th circle wizard spells
- L10: wizard memorization
- L11: 6th circle wizard spells
- L13: 7th circle wizard spells
- L15: 8th circle wizard spells; construct wood golem
- L17: 9th circle wizard spells
- L19: wizard quick chant
- L21: epic wizard spells
- L25: construct stone golem
- L30: wizard debuff

Class-feat pool: combat casting, spell penetration, greater spell penetration, armored spellcasting, faster memorization, spell focus, greater spell focus, improved familiar, quick chant, augment summoning, enhanced spell damage, maximize spell, empower spell, quicken spell, extend spell, silent spell, still spell, greater ruin, dragon knight, hellball, epic mage armor, epic warding, great intelligence, automatic quicken spell, automatic still spell, automatic silent spell.

### Sorcerer

An arcane spell master who gains his ability from mystical sources.

- **Primary attribute**: Charisma, Con/Dex for survivability
- **Class skills**: Concentration, Spellcraft, Ride, Bluff, Use Magic Device
- **Extra feat rule**: bonus class feat at sorcerer levels 7, 13, 19, and 25
- **Cap bonus**: INT, DEX, CHA +(level/4+1)
- **Starting gear**: cloth sleeves, cloth pants, dagger, cloth robes
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: spontaneous, arcane; bloodlines; circle level/2 (min 1), max 9; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - wizards; simple weapon proficiency; summon familiar; 1st circle sorcerer spells
- L4: 2nd circle sorcerer spells
- L6: 3rd circle sorcerer spells
- L8: 4th circle sorcerer spells
- L10: 5th circle sorcerer spells
- L12: 6th circle sorcerer spells
- L14: 7th circle sorcerer spells
- L15: construct wood golem
- L16: 8th circle sorcerer spells
- L18: 9th circle sorcerer spells
- L21: epic sorcerer spells
- L25: construct stone golem

Class-feat pool: the Wizard pool with great charisma in place of great intelligence.

Bloodline feats are granted conditionally by `process_conditional_class_level_feats()`
once a bloodline is chosen in `study`:

- Draconic: claws and bloodline arcana at once, dragon resistances at 3, breath weapon at 9, wings at 15, power of wyrms and blindsense at 20.
- Arcane: bloodline arcana and improved familiar at once, metamagic adept at 3, new arcana at 9 (rank 2 at 13, rank 3 at 17), school power at 15, arcane apotheosis at 20.
- Fey: bloodline arcana and laughing touch at once, woodland stride at 3, fleeting glance at 9, fey magic at 15, soul of the fey at 20.
- Undead: bloodline arcana and grave touch at once, death's gift at 3, grasp of the dead at 9, incorporeal form at 15, one of us at 20.

### Bard

A jack-of-all-trades who combines magic with their party-boosting songs and performances.

- **Primary attribute**: Charisma, Int for skills, Con/Dex for survivability
- **Class skills**: Acrobatics, Stealth, Perception, Concentration, Spellcraft, Appraise, Total Defense, Ride, Athletics, Sleight of Hand, Bluff, Diplomacy, Disguise, Escape Artist, Use Magic Device, Perform
- **Extra feat rule**: bonus general feat at every 3rd bard level up to 20, epic feat instead past 20
- **Cap bonus**: DEX, CHA +(level/4+1); STR, INT +(level/8+1); hitroll/damroll +level/6
- **Starting gear**: lyre, flute, drum, horn, harp, mandolin
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: spontaneous, arcane; circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; weapon proficiency - bards; weapon proficiency - rogues; light armor proficiency; shield armor proficiency; bardic music; bardic knowledge; countersong; song of healing; 1st circle bard spells
- L2: dance of protection; accompany
- L3: song of focused mind
- L4: 2nd circle bard spells
- L5: song of heroism
- L7: oratory of rejuvenation; 3rd circle bard spells
- L9: song of flight
- L10: efficient performance; 4th circle bard spells
- L11: song of revelation
- L13: song of fear; 5th circle bard spells
- L15: skit of forgetfulness
- L16: 6th circle bard spells
- L17: song of rooting
- L21: epic bard spells
- L22: song of dragons
- L26: song of the magi

Class-feat pool: great dexterity, great charisma, deafening song.

### Summoner

While many who dabble in the arcane become adept at beckoning monsters from the farthest reaches of the planes, none are more skilled at it than the summoner.

- **Primary attribute**: Charisma, Con/Dex for survivability, Str for combat
- **Class skills**: Concentration, Spellcraft, Appraise, Ride, Diplomacy, Handle Animal, Use Magic Device
- **Cap bonus**: all six stats +(level/10+1); hitroll/damroll +level/10 (default branch, no Summoner case)
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, studded leather
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: spontaneous, arcane; eidolon; circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16; epic spells at 21. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; eidolon; life link; conjure monster; 1st circle summoner spells
- L2: bond senses
- L4: shield ally; 2nd circle summoner spells
- L6: makers call
- L7: 3rd circle summoner spells
- L8: transposition
- L10: aspect; 4th circle summoner spells
- L12: greater shield ally
- L13: 5th circle summoner spells
- L14: life bond
- L15: construct wood golem
- L16: merge forms; 6th circle summoner spells
- L18: greater aspect
- L20: grand eidolon
- L21: epic summoner spells
- L25: epic aspect; construct stone golem
- L30: epic eidolon

Class-feat pool: great charisma, epic augment summoning, epic spell focus, automatic quicken spell, automatic still spell, automatic silent spell.

### Warlock

Champions of dark and chaotic powers, warlocks are born of perilous magic or extraplanar pacts.

- **Primary attribute**: Charisma, Con/Dex for survivability, Str for combat
- **Class skills**: Intimidate, Concentration, Spellcraft, Appraise, Discipline, Bluff, Diplomacy, Use Magic Device
- **Cap bonus**: INT, DEX, CHA +(level/4+1)
- **Starting gear**: cloth sleeves, cloth pants, dagger, cloth robes
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: at-will invocations, arcane; circle 1 at 1-5, 2 at 6-10, 3 at 11-15, 4 at 16+. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; eldritch blast
- L2: eldritch lore
- L3: eldritch blast; damage reduction - warlock
- L4: deceive item
- L5: eldritch blast
- L7: eldritch blast; damage reduction - warlock
- L8: fiendish resilience; energy resistance
- L9: eldritch blast
- L11: eldritch blast; damage reduction - warlock
- L13: fiendish resilience
- L14: eldritch blast
- L15: damage reduction - warlock
- L17: eldritch blast
- L18: fiendish resilience; fiendish resilience; fiendish resilience
- L19: damage reduction - warlock
- L20: eldritch blast; energy resistance
- L22: eldritch blast
- L23: damage reduction - warlock
- L24: eldritch blast
- L26: eldritch blast
- L27: damage reduction - warlock
- L28: eldritch blast
- L30: eldritch blast; energy resistance; epic eldritch power

Class-feat pool: eldritch master, epic eldritch blast, greater ruin, dragon knight, hellball, epic mage armor, epic warding, great intelligence.

### Alchemist

A brilliant inventor whose magical concoctions, mutagens and bombs make him a fearsome and deadly foe.

- **Primary attribute**: Intelligence, Con/Dex for survivability, Str for combat
- **Class skills**: Perception, Spellcraft, Appraise, Discipline, Ride, Sleight of Hand, Disable Device, Sense Motive, Use Magic Device
- **Cap bonus**: INT, STR, CHA +(level/4+1); hitroll/damroll +level/3
- **Starting gear**: leather sleeves, leather leggings, slender iron mace, studded leather
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: extracts (prepared concoctions); circle 1 at 1, 2 at 4, 3 at 7, 4 at 10, 5 at 13, 6 at 16. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; 1st circle alchemical concoctions; brew potion; mutagen; bombs
- L2: poison resist; alchemical discovery
- L3: swift alchemy; bombs
- L4: 2nd circle alchemical concoctions; alchemical discovery
- L5: poison resist; bombs
- L6: swift poisoning; alchemical discovery
- L7: 3rd circle alchemical concoctions; bombs
- L8: poison resist; alchemical discovery
- L9: bombs
- L10: 4th circle alchemical concoctions; poison immunity; alchemical discovery
- L11: bombs
- L12: apply poison; alchemical discovery
- L13: 5th circle alchemical concoctions; bombs
- L14: persistent mutagen; alchemical discovery
- L15: bombs
- L16: 6th circle alchemical concoctions; alchemical discovery
- L17: bombs
- L18: instant alchemy; alchemical discovery
- L19: bombs
- L20: grand alchemical discovery; alchemical discovery
- L21: bombs
- L22: bombs; alchemical discovery
- L23: bombs
- L24: bombs; alchemical discovery
- L25: bombs
- L26: bombs; alchemical discovery
- L27: bombs
- L28: bombs; alchemical discovery
- L29: bombs
- L30: bombs; alchemical discovery; bomb mastery

Class-feat pool: great dexterity, great intelligence.

### Psionicist

A master of the mind who can manipulate reality and destroy his foes with psionic power.

- **Primary attribute**: Intelligence, Dex/Con for survivability
- **Class skills**: Perception, Concentration, Spellcraft, Appraise, Discipline, Ride, Diplomacy, Sense Motive, Use Magic Device
- **Extra feat rule**: bonus class feat at psionicist level 1 and every 5th level up to 20
- **Cap bonus**: INT, DEX, CON +(level/4+1)
- **Starting gear**: cloth sleeves, cloth pants, dagger, cloth robes
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: psionic powers fuelled by power points (PSP); circle (level+1)/2, max 9; epic psionics at 21, 25, 29. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - psionicist; psionic focus; 1st circle psionicist powers
- L3: 2nd circle psionicist powers
- L5: 3rd circle psionicist powers
- L7: 4th circle psionicist powers
- L8: breach power resistance
- L9: 5th circle psionicist powers
- L11: 6th circle psionicist powers
- L13: 7th circle psionicist powers
- L14: double manifest
- L15: 8th circle psionicist powers
- L17: 9th circle psionicist powers
- L20: perpetual foresight
- L21: epic psionics
- L23: epic augmenting
- L25: epic psionics
- L27: epic augmenting
- L29: epic psionics
- L30: master of the mind

Class-feat pool: combat manifestation, aligned attack (good), aligned attack (evil), aligned attack (chaotic), aligned attack (lawful), critical focus, elemental focus (fire), elemental focus (acid), elemental focus (cold), elemental focus (electric), elemental focus (sonic), power penetration, greater power penetration, mighty power penetration, quick mind, psionic recovery, proficient psionicist, enhanced power damage, expanded knowledge, psionic endowment, empowered magic, proficient augmenting, expert augmenting, master augmenting, great intelligence.

### Artificer

Beyond the veil of the mundane hide the secrets of absolute power through the fusion of magic and technology.

- **Primary attribute**: Intelligence for device creation, Dexterity and Constitution for survivability
- **Class skills**: Perception, Concentration, Spellcraft, Appraise, Ride, Sleight of Hand, Diplomacy, Disable Device, Sense Motive, Use Magic Device
- **Cap bonus**: INT, DEX, CON +(level/4+1)
- **Starting gear**: no class-specific gear (common kit only)
- **Premade build**: supported (`respec <class> premade`, or premade at creation)
- **Magic**: weird science devices built from the wizard and cleric spell lists; device spell level 1 from 1, 2 at 3, 3 at 5, 4 at 11. Full list in Appendix A.

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; shield armor proficiency; artificer's lore; elbow grease; jack of all trades; weird science
- L2: scribe scroll; artificer item creation
- L3: brew potion
- L4: craft wonderous item
- L5: craft magical arms and armor; salvage
- L6: metamagic science
- L7: craft wand
- L9: craft rod
- L10: construct wood golem
- L11: improved metamagic science
- L12: craft staff
- L13: improved jack of all trades
- L14: forge ring
- L20: exemplar; construct stone golem
- L30: construct iron golem

Class-feat pool: combat casting, spell penetration, greater spell penetration, improved initiative, toughness, skill focus, magical aptitude, empower spell, enlarge spell, extend spell, heighten spell, maximize spell, quicken spell, silent spell, still spell, great intelligence, great dexterity, great constitution.

## Prestige class details

### Weaponmaster

A master of specific weapons who, while wielding them, is without equal in their use.

- **Primary attribute**: Strength, Con/Dex for survivability
- **Class skills**: Intimidate, Discipline, Total Defense, Ride, Bluff, Sense Motive
- **Cap bonus**: STR, CON +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: weapons of choice
- L2: superior weapon focus
- L3: unstoppable strike
- L4: critical specialist
- L5: unstoppable strike
- L6: unstoppable strike
- L7: unstoppable strike
- L8: critical specialist
- L9: unstoppable strike
- L10: increased multiplier

### Stalwart Defender

A defensive specialist who stands against the fray while protecting their allies.

- **Primary attribute**: Strength, Con/Dex for survivability
- **Class skills**: Perception, Intimidate, Discipline, Total Defense, Ride, Athletics, Sense Motive
- **Cap bonus**: STR, CON +(level/4+1) plus defensive stance bonus; hitroll/damroll +level/3
- **Perk points route to**: Warrior
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: AC bonus; defensive stance
- L2: fearless defense
- L3: uncanny dodge; defensive stance
- L4: AC bonus; immobile defense
- L5: shrug damage; defensive stance
- L7: AC bonus; shrug damage; shrug damage; improved uncanny dodge; defensive stance
- L8: renewed defense
- L9: mobile defense; defensive stance
- L10: last word; AC bonus; shrug damage; shrug damage; smash defense; defensive stance

### Duelist

A master of the single blade, deadly on both the offensive and defensive.

- **Primary attribute**: Dexterity/Intelligence, Con for survivability, Str for combat
- **Class skills**: Acrobatics, Perception, Total Defense, Ride, Bluff, Escape Artist, Sense Motive, Perform
- **Cap bonus**: DEX, INT +(level/4+1); STR +(level/8+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior + Rogue
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: canny defense; precise strike
- L2: improved reaction; parry
- L3: enhanced mobility
- L4: grace; combat reflexes
- L5: riposte
- L6: dexterous charge
- L7: elaborate parry
- L8: improved reaction
- L9: deflect arrows; no retreat
- L10: crippling critical

### Dragonrider

.

- **Primary attribute**: Strength, Dexterity and Constitution
- **Class skills**: Perception, Intimidate, Concentration, Discipline, Total Defense, Ride, Athletics, Diplomacy, Handle Animal, Sense Motive
- **Extra feat rule**: bonus class feat at class levels 4 and 8
- **Cap bonus**: STR, CON +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: dragon mount; dragoon points
- L2: dragon link; ultravision
- L3: draconic protection; riders bond
- L5: adept rider
- L6: draconic resistance
- L7: skilled rider
- L8: blindsense
- L9: master rider
- L10: united we stand

Class-feat pool: alertness, medium armor proficiency, light armor proficiency, blind fighting, combat expertise, dazzling display, diehard, endurance, great fortitude, iron will, lightning reflexes, improved intimidation, leadership, mounted archery, persuasive, ride by attack, spirited charge, toughness, weapon focus.

### Arcane Archer

A master archer who combines spellcasting with the bow.

- **Primary attribute**: Dexterity, your primary casting class stats
- **Class skills**: Stealth, Perception, Intimidate, Concentration, Spellcraft, Ride
- **Cap bonus**: DEX +(level/4+1); INT, CHA +(level/8+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior + Wizard
- **Spell progression note**: arcane: 3/4 of arcane archer level

Feats granted by class level (a repeated feat adds a rank):

- L1: enhance arrow
- L2: seeker arrow
- L3: enhance arrow
- L4: imbue arrow
- L5: enhance arrow
- L6: seeker arrow; imbue arrow
- L7: enhance arrow
- L8: seeker arrow; swarm of arrows
- L9: enhance arrow
- L10: arrow of death

### Eldritch Knight

A warrior spellcaster whose weapon allows them to swiftly barrage their enemy with arcane magics.

- **Primary attribute**: Strength, Con/Dex for survivability, your primary casting class stats
- **Class skills**: Concentration, Spellcraft, Discipline, Total Defense, Ride, Athletics, Sense Motive, Use Magic Device
- **Extra feat rule**: bonus class feat at class levels 1, 5, and 9
- **Cap bonus**: INT, STR, CHA +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Wizard + Warrior
- **Spell progression note**: Arcane advancement every level

Feats granted by class level (a repeated feat adds a rank):

- L1: diverse training
- L10: spell critical

Class-feat pool: the Warrior pool without devastating critical, overwhelming critical.

### Spellsword

An arcane warrior who imbues his weapon with magic to unleash on his enemies with each blow.

- **Primary attribute**: Strength, Con/Dex for survivability, your primary casting class stats
- **Class skills**: Concentration, Spellcraft, Discipline, Total Defense, Ride, Athletics, Use Magic Device
- **Extra feat rule**: bonus class feat at class levels 2, 6, and 10
- **Cap bonus**: INT, STR, CHA +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Wizard + Warrior
- **Spell progression note**: Arcane advancement at level one and every second level after

Feats granted by class level (a repeated feat adds a rank):

- L1: ignore spell failure
- L2: channel spell
- L3: ignore spell failure
- L4: improved channelling
- L5: ignore spell failure
- L6: channel spell; ignore spell failure; advanced channelling
- L7: ignore spell failure
- L8: channel spell; ignore spell failure
- L9: ignore spell failure
- L10: multiple channel spell; ignore spell failure; ignore spell failure; ignore spell failure; greater channelling

Class-feat pool: the Warrior pool without devastating critical, double weapon critical, double weapon defense, double weapon focus, double weapon specialization, overwhelming critical, plus empower spell, extend spell, increase spell damage, maximize spell, quicken spell, silent spell, still spell.

### Arcane Shadow

A shadowy spellcaster who combines stealth and magic to defeat their foes unawares.

- **Primary attribute**: Dexterity, Con for survivability, Int for skills, your primary casting class stats
- **Class skills**: Acrobatics, Stealth, Perception, Concentration, Spellcraft, Discipline, Ride, Athletics, Sleight of Hand, Disable Device, Disguise, Escape Artist, Sense Motive, Use Magic Device
- **Cap bonus**: DEX, INT +(level/4+1); STR +(level/8+1); hitroll/damroll +level/3
- **Perk points route to**: Wizard + Rogue
- **Spell progression note**: Arcane advancement every level

Feats granted by class level (a repeated feat adds a rank):

- L1: impromptu sneak attack
- L2: sneak attack
- L3: impromptu sneak attack
- L4: sneak attack
- L5: impromptu sneak attack
- L6: sneak attack
- L7: invisible rogue; impromptu sneak attack
- L8: sneak attack
- L9: magical ambush; impromptu sneak attack
- L10: sneak attack; surprise spells; impromptu sneak attack

### Shadowdancer

Shadowdancers exist in the boundary between light and darkness, where they weave together the shadows to become half-seen artists of deception.

- **Primary attribute**: Dexterity, Con for survivability, Int for skills, Str for combat
- **Class skills**: Acrobatics, Stealth, Perception, Ride, Sleight of Hand, Bluff, Diplomacy, Disguise, Escape Artist, Perform
- **Cap bonus**: DEX, INT +(level/4+1); STR +(level/8+1); hitroll/damroll +level/3
- **Perk points route to**: Rogue
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: hide in plain sight; light armor proficiency; weapon proficiency - shadowdancer
- L2: evasion; low light vision; uncanny dodge
- L3: sneak attack; shadow illusion; summon shadow
- L4: shadow call; shadow jump
- L5: defensive roll; improved uncanny dodge
- L6: sneak attack; shadow jump
- L7: slippery mind
- L8: shadow jump; shadow power
- L9: sneak attack
- L10: improved evasion; shadow jump; shadow master

### Assassin

A mercenary undertaking his task with cold, professional detachment, the assassin is equally adept at espionage, bounty hunting, and terrorism.

- **Primary attribute**: Dexterity, Con for survivability, Str for combat, Int for skills
- **Class skills**: Acrobatics, Stealth, Perception, Intimidate, Discipline, Ride, Athletics, Sleight of Hand, Bluff, Disable Device, Disguise, Escape Artist, Sense Motive, Use Magic Device
- **Cap bonus**: DEX, INT +(level/4+1); STR +(level/8+1); hitroll/damroll +level/3
- **Perk points route to**: Rogue
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - assassin; light armor proficiency; sneak attack; death attack
- L2: poison save bonus; uncanny dodge
- L3: sneak attack
- L4: hidden weapons; true death; poison save bonus
- L5: sneak attack; improved uncanny dodge
- L6: poison save bonus; quiet death
- L7: sneak attack; apply poison
- L8: poison save bonus; hide in plain sight
- L9: sneak attack; swift death
- L10: poison save bonus; angel of death

### Sacred Fist

A master of unarmed combat and divine magic whose fists blaze with burning flame.

- **Primary attribute**: Wisdom, Con/Dex for survivability
- **Class skills**: Acrobatics, Concentration, Spellcraft, Discipline, Ride, Athletics, Escape Artist, Use Magic Device
- **Cap bonus**: WIS, DEX +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Monk + Cleric
- **Spell progression note**: Divine advancement every level

Feats granted by class level (a repeated feat adds a rank):

- L1: weapon proficiency - monks; monk wisdom bonus to AC; monk innate ac bonus; unarmed strike; improved unarmed strike
- L2: AC bonus; sacred flames
- L3: sacred flames
- L4: sacred flames
- L5: sacred flames
- L6: uncanny dodge
- L7: AC bonus; sacred flames
- L8: sacred flames
- L9: inner fire (FEAT_INNER_FIRE, no feato() registration)
- L10: greater flurry; AC bonus

### Shifter

A master shapechanger who can take the form of many powerful entities.

- **Primary attribute**: Wisdom, Con/Dex for survivability, Str for combat
- **Class skills**: Perception, Concentration, Spellcraft, Ride, Diplomacy, Disguise, Handle Animal
- **Cap bonus**: STR, DEX, WIS +(level/4+1)
- **Perk points route to**: Druid
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: limitless shapes
- L2: shifter shapes i
- L4: shifter shapes ii
- L6: shifter shapes iii
- L8: shifter shapes iv
- L10: shifter shapes v

### Mystic Theurge

A powerful spellcaster who combines the use of both divine and arcane mystic arts.

- **Primary attribute**: Your primary casting class stats, Con/Dex for survivability
- **Class skills**: Concentration, Spellcraft, Appraise, Ride, Sense Motive, Use Magic Device
- **Cap bonus**: INT, CHA, WIS +(level/4+1)
- **Perk points route to**: Wizard + Cleric
- **Spell progression note**: each level in -both- divine/arcane choice

Feats granted by class level (a repeated feat adds a rank):

- L1: theurge spellcasting
- L2: theurge spellcasting
- L3: theurge spellcasting
- L4: theurge spellcasting
- L5: theurge spellcasting
- L6: theurge spellcasting
- L7: theurge spellcasting
- L8: theurge spellcasting
- L9: theurge spellcasting
- L10: theurge spellcasting

### Necromancer

While others use magic to do paltry things like conjure fire or fly, the Necromancer is a master over death itself.

- **Primary attribute**: Necromancers do not have preferred ability scores. Instead it depends on their spellcasting classes.
- **Class skills**: Stealth, Perception, Concentration, Spellcraft, Discipline, Diplomacy, Sense Motive, Use Magic Device
- **Extra feat rule**: bonus class feat at class level 7; Weapon Focus (polearm) at 5, Weapon Specialization (polearm) at 7, +4 STR at 6; knows the Undead Appearance evolution from level 1; Animate Dead granted at level 2
- **Cap bonus**: INT, DEX, CON +(level/4+1)
- **Perk points route to**: Wizard + Cleric
- **Spell progression note**: choose between arcane or divine, +1 spellcasting level per necromancer level.

Feats granted by class level (a repeated feat adds a rank):

- L1: necromancer weapons; undead cohort
- L2: summon undead; animate dead
- L3: ultravision
- L4: light armor proficiency; bone armor
- L5: deathless vigor
- L6: undead graft; touch of undeath; paralyzing touch
- L7: tough as bone; weakening touch
- L8: medium armor proficiency; bone armor; degenerative touch
- L9: summon greater undead; destructive touch
- L10: essence of undeath; deathless touch

Class-feat pool: weapon specialization.

### Knight of the Luminous Thread

The Knights of the Luminous Thread represent the full evolution of oath-bound warriors, from novice oath-takers to master weavers of destiny.

- **Primary attribute**: Strength, Wisdom for spellcasting
- **Class skills**: Perception, Intimidate, Spellcraft, Discipline, Total Defense, Ride, Athletics, Diplomacy, Handle Animal, Sense Motive
- **Extra feat rule**: bonus class feat at every 4th class level
- **Cap bonus**: STR, CON +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior + Cleric
- **Spell progression note**: divine advancement after level 5

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; light armor proficiency; medium armor proficiency; heavy armor proficiency; martial weapon proficiency; strength of honor; knightly courage
- L2: heroic initiative; diehard
- L3: honorable will
- L4: might of honor; armored mobility
- L5: crown of knighthood
- L6: channel energy
- L7: smite evil; aura of courage
- L8: smite evil; demoralizing strike
- L9: smite evil
- L10: smite evil; soul of knighthood
- L11: aura of good; rallying cry
- L12: inspire courage
- L13: leadership; divine grace
- L14: inspire greatness; channel energy
- L15: inspire courage
- L16: wisdom of the measure
- L17: leadership
- L18: inspire courage
- L19: final stand
- L20: knighthood's flower

Class-feat pool: the Warrior pool without devastating critical, double weapon critical, double weapon defense, double weapon focus, double weapon specialization, improved initiative, oversized two weapon fighting, overwhelming critical, weapon flurry, weapon mastery i, weapon supremacy, plus augment summoning, combat casting, craft magical arms and armor, great charisma, great intelligence, great wisdom, greater spell penetration, improved weapon finesse, lightning reflexes, spell penetration.

### Knight of the Howling Moon

The Knights of the Howling Moon are warriors who have embraced their primal nature, running between the civilized and wild as guardians of the boundary.

- **Primary attribute**: Str for melee combat, Con/Dex for survivability
- **Class skills**: Perception, Intimidate, Discipline, Total Defense, Ride, Athletics, Diplomacy, Handle Animal
- **Extra feat rule**: bonus class feat at class levels 2, 5, and 8
- **Cap bonus**: STR, CON +(level/4+1); hitroll/damroll +level/3
- **Perk points route to**: Warrior + Wizard
- **Spell progression note**: none

Feats granted by class level (a repeated feat adds a rank):

- L1: simple weapon proficiency; martial weapon proficiency; light armor proficiency; medium armor proficiency; heavy armor proficiency; shield armor proficiency; sneak attack
- L2: demoralize
- L3: fight to the death
- L4: unbreakable will; sneak attack
- L6: armored mobility
- L7: sneak attack
- L8: unbreakable will
- L10: sneak attack; one thought

Class-feat pool: the Warrior pool without devastating critical, overwhelming critical.

### Knight of the Shattered Mirror

The Knights of the Shattered Mirror are masters of paradox who blend arcane magic with martial prowess.

- **Primary attribute**: Con/Dex for survivability, INT/CHA depending on base spellcasting class.
- **Class skills**: Perception, Intimidate, Concentration, Spellcraft, Discipline, Ride, Diplomacy, Handle Animal, Sense Motive
- **Cap bonus**: INT, DEX, CON +(level/4+1)
- **Perk points route to**: Wizard + Warrior
- **Spell progression note**: +1 arcane caster level per knight of the shattered mirror level.

Feats granted by class level (a repeated feat adds a rank):

- L1: diviner; read omens
- L2: armored spellcasting
- L3: aura of terror
- L4: weapon touch
- L5: armored spellcasting
- L6: read portents
- L8: armored spellcasting
- L10: armored spellcasting; cosmic understanding

### Knight of the Pale Throne

The Knights of the Pale Throne are death-priests who channel negative divine energy.

- **Primary attribute**: Str for melee combat, Con/Dex for survivability, Wis for spellcasting.
- **Class skills**: Perception, Intimidate, Concentration, Spellcraft, Discipline, Ride, Diplomacy, Handle Animal, Sense Motive
- **Cap bonus**: STR, CHA, WIS +(level/4+1)
- **Perk points route to**: Warrior + Cleric
- **Spell progression note**: +1 divine caster level per knight of the pale throne level.

Feats granted by class level (a repeated feat adds a rank):

- L1: aura of evil; detect good; smite good
- L2: unholy resilience
- L3: heart of truth
- L4: smite good
- L5: channel energy
- L6: smite good; heart of truth
- L8: aura of the vision; smite good
- L9: heart of truth
- L10: favor of darkness; smite good

## Help coverage

`lib/text/help/help.hlp` has a class entry (keyword `CLASS-<NAME>` or the bare
class name) for Wizard, Cleric, Rogue, Warrior, Monk, Druid, Berserker, Sorcerer,
Paladin, Ranger, Bard, Weapon Master, Arcane Archer, Stalwart Defender, Eldritch
Knight, Psionicist, Shadowdancer, Alchemist, Inquisitor, Summoner, Necromancer,
and Artificer, plus the `CLASS-ROSTER` directory. No dedicated class entry exists
for Arcane Shadow, Shifter (only `LOCKED-SHIFTER` under wildshape), Duelist,
Mystic Theurge, Sacred Fist, Spellsword, Blackguard, Assassin, Warlock (only
invocation entries), Dragonrider, or the four knight classes. Character creation
still calls `perform_help()` with `class-<slug>` for every base class, so the
missing base entries (Blackguard, Warlock) show a "no help available" result.

## Registry observations worth knowing

These are facts of the current registry and code paths, recorded so readers do
not assume otherwise. None of them is a documented design decision.

- **Alignment mismatches between creation and `gain`**: Arcane Shadow has
  non-lawful alignment prerequisite rows but `valid_align_by_class()` allows any
  alignment; Knight of the Howling Moon, Knight of the Shattered Mirror, and
  Knight of the Pale Throne are LE/LN in `valid_align_by_class()` but register no
  alignment prerequisite rows, so `gain` never checks their alignment; Assassin
  has an evil-only block commented out in both places and is unrestricted.
- **`respec` message**: rejects `CLSLIST_LOCK()` while describing it as a
  prestige restriction. Today every locked class is prestige, so the behavior
  matches the message.
- **Duplicate feat rows**: Spellsword grants `ignore spell failure` three times
  at level 10, Warlock grants `fiendish resilience` three times at level 18,
  and Stalwart Defender grants `shrug damage` twice at levels 7 and 10. All are
  marked stacking, so each row adds a rank.
- **Sacred Fist level 9** grants `FEAT_INNER_FIRE`, which has a `dailyfeat()`
  cooldown entry but no `feato()` registration, so it shows with the default
  unregistered feat text.
- **Shadowdancer** grants `sneak attack` at 3, 6, and 9 with `stacks = N`, unlike
  every other sneak attack grant.
- **Heal** is a class skill for all 36 classes; Lore and Swim parameters of
  `assign_class_abils()` are ignored, and `psp_gain` in `classo()` is unused (the
  Psionicist power point rule lives in `advance_level()`).
- **Spell progression strings** in `classo()` are display text only; the real
  advancement is in `compute_bonus_caster_level()` and the caster level
  functions. The Knight of the Luminous Thread string says "after level 5" and
  the code matches (divine levels above 5 count).
- **Epic class feat interval** is 0 for every prestige class, so prestige levels
  never award epic class feats even when the character is epic.
- **Psionicist** is the only base class with an explicit level cap (30, the same
  as the global cap); **Artificer** caps at 20.
- **`parse_class_long()`** has a commented-out early `assassin` match but a live
  one later, so `assassin` still parses.
- **`IS_SPELLCASTER_CLASS()`** in `src/utils.h` lists 12 base classes and omits
  Psionicist and Artificer; `IS_CASTER()` omits Warlock, Psionicist, Artificer,
  Shadowdancer, and Knight of the Howling Moon. Code that uses these macros
  treats those classes as non-casters.

## Appendix A: spell, power, invocation, and extract lists

Lists come from `spell_assignment()` rows in `load_class_list()`, grouped by the
class level at which the entry becomes available. Level 0 entries are cantrips
and orisons. Names are the registered spell names shown by `spells`, `powers`,
`extracts`, and `cast`. Entries assigned by `assign_rol_conversion_spell_kits()`
are included; domain spells for Cleric and Inquisitor and bonus spells from
feats or perks are not, because they depend on the character's choices.

### Ranger (46 entries)

- L6: ant haul, charm animal, command undead, create spring, cure light, detect poison, entangle, faerie fire, jump, magic fang, natures ally i, protection from animals, sun metal, vigorize light
- L10: barkskin, dust devil, effortless armor, endurance, grace, hold animal, mass ant haul, natures ally ii, protection from energy, strength, vigorize serious, wind wall, wisdom
- L12: communal protection from energy, contagion, cure moderate, greater magic fang, natures ally iii, natures blessing, remove disease, remove poison, spike growth, vigorize critical
- L15: cure serious, dispel magic, farsee, free movement, group vigorize, natures ally iv, pass without trace, poltergeist, splinter storm

### Paladin (51 entries)

- L6: cure light, detect poison, divine favor, endurance, endure elements, hedging weapons, honeyed tongue, lesser restoration, protection from evil, resistance, shield of faith, shield of fortification, stunning barrier, sun metal, tactical acumen, veil of positive energy
- L10: bestow weapon proficiency, blinding ray, charisma, create food, create water, cure moderate, detect poison, effortless armor, fire of entanglement, holy javelin, life shield, litany of defense, litany of righteousness, remove paralysis, resist energy, strength, undetectable alignment, weapon of awe, wisdom
- L12: bless, communal resist energy, cure blind, cure serious, detect alignment, greater magic weapon, heal mount
- L15: aid, banishing blade, cure critical, holy weapon, infravision, planar soul, remove curse, remove poison, stone skin

### Blackguard (47 entries)

- L6: cause light wound, command undead, detect poison, doom, endurance, endure elements, hedging weapons, negative energy ray, protection from good, summon creature iii
- L10: bestow weapon proficiency, blindness, charisma, curse item, darkness, deafness, hold person, infravision, invisibility, litany of defense, silence, spectral hand, strength, summon creature iv, undetectable alignment
- L12: animate dead, bestow curse, cause moderate wound, circle against good, contagion, dispel magic, greater magic weapon, summon creature vi, tazriks frenzied hound, vampiric touch
- L15: banishing blade, cause fear, cause serious wound, dark wrath, dispel good, greater invisibility, planar soul, poison, stone skin, summon creature viii, unholy aura, unholy weapon

### Cleric (175 entries)

- L0: continual light, detect poison, disrupt undead, enhanced diplomacy, flare, guidance, stabilize, virtue
- L1: ant haul, bless, cause light wound, cure light, divine favor, doom, effortless armor, endurance, endure elements, grace, hedging weapons, negative energy ray, planar healing, protection from evil, protection from good, remove fear, shield of faith, strength, stunning barrier, summon creature i, sun metal
- L3: augury, bestow weapon proficiency, blinding ray, cause moderate wound, charisma, create food, create water, cure moderate, darkness, detect magic, detect poison, gird allies, lesser restoration, mass ant haul, preserve, remove paralysis, resist energy, scare, shield of fortification, silence, slow poison, spiritual weapon, summon creature ii, undetectable alignment, vigorize light, weapon of awe, wisdom
- L5: animate dead, blindness, cause serious wound, command undead, communal resist energy, control summoned creature, cunning, cure blind, cure deafness, cure serious, deafness, detect alignment, dispel magic, faerie fog, holy javelin, invisibility purge, life shield, magic vestment, protection from energy, searing light, summon creature iii, weapon of impact, wind wall
- L7: aid, air walk, bestow curse, bravery, cause critical wound, circle against evil, circle against good, communal protection from energy, cure critical, daylight, dispel invis, divine power, greater magic weapon, greater planar healing, infravision, mass cure light, remove curse, summon creature iv, vigorize serious
- L9: cause fear, caustic blood, flame strike, free movement, geniekind, group shield of faith, mass cure moderate, poison, protection from evil, protection from good, regeneration, remove poison, strengthen bones, summon creature v, vigorize critical, water breathe, waterwalk
- L11: curse item, dispel evil, dispel good, eyebite, farsee, harm, heal, levitate, mass charisma, mass cunning, mass cure serious, mass wisdom, planar ally, planar soul, prayer, remove disease, summon creature vi, undeath to death
- L13: battletide, blade barrier, call lightning, greater dispelling, magic resistance, mass cure critical, mass enhance, restoration, sense life, soul tempest, summon, summon creature vii, word of recall
- L15: anti magic field, destruction, dimensional lock, earthquake, greater animation, holy aura, resurrection, salvation, spell mantle, spring of life, summon creature viii, true seeing, word of faith
- L17: ancestral shield, energy drain, greater realm of protection, group heal, group summon, implode, plane shift, refuge, spirit walk, storm of vengeance, summon creature ix, sunburst
- L21: dragon knight, greater ruin, hellball, mummy dust

### Druid (133 entries)

- L0: continual light, detect poison, enhanced diplomacy, flare, grasp, guidance, root, stabilize, virtue
- L1: ant haul, charm animal, cure light, entangle, faerie fire, goodberry, jump, magic fang, magic stone, natures ally i, obscuring mist, produce flame, resistance, vigorize light
- L3: barkskin, endurance, flame blade, flaming sphere, gird allies, grace, hold animal, lesser restoration, mass ant haul, natures ally ii, preserve, protection from animals, spider climb, strength, summon swarm, vigorize serious, wisdom
- L5: aqueous orb, call lightning, communal resist energy, communal spider climb, contagion, cure moderate, greater magic fang, life shield, moonbeam, natures ally iii, poison, protection from energy, remove disease, remove poison, siphon might, spike growth, splinter storm, vigorize critical, wind wall
- L7: air walk, blight, caustic blood, communal protection from energy, create spring, cure serious, dispel invis, dispel magic, dust devil, flame strike, free movement, group vigorize, ice storm, locate creature, natures ally iv, spike stone
- L9: call lightning storm, cure critical, death ward, geniekind, hallow, insect plague, natures ally v, shockwave, stone skin, unhallow, wall of fire, wall of thorns
- L11: communal stone skin, fire seeds, greater black tentacles, greater dispelling, mass cure light, mass endurance, mass grace, mass strength, mass wisdom, natures ally vi, spellstaff, suffocate, transport via plants
- L13: control weather, creeping doom, cyclone, fire storm, heal, mass cure moderate, natures ally vii, pass without trace, sunbeam
- L15: animal shapes, control plants, earthquake, finger of death, mass cure serious, mud to rock, natures ally viii, rock to mud, sunburst, whirlwind, word of recall
- L17: elemental swarm, mass cure critical, moonwell, natures ally ix, polymorph self, regeneration, resurrection, shambler, storm of vengeance
- L21: dragon knight, greater ruin, hellball

### Inquisitor (98 entries)

- L0: acid splash, continual light, detect poison, disrupt undead, enhanced diplomacy, flare, guidance, stabilize, virtue
- L1: bless, cause light wound, cure light, detect alignment, divine favor, doom, expeditious retreat, hedging weapons, protection from evil, protection from good, remove fear, shield of faith, shield of fortification, true strike
- L4: aid, bestow weapon proficiency, cause moderate wound, cure moderate, darkness, detect invisibility, effortless armor, hold person, honeyed tongue, invisibility, lesser restoration, litany of defense, remove paralysis, resist energy, silence, spiritual weapon, tactical acumen, undetectable alignment, vigorize light, weapon of awe
- L7: blinding ray, cause serious wound, communal resist energy, continual flame, cure serious, daylight, deathly heroism, dispel magic, greater magic weapon, holy javelin, invisibility purge, keen edge, litany of righteousness, locate object, magic vestment, nondetection, prayer, protection from energy, remove curse, remove disease, righteous vigor, searing light, weapon of impact
- L10: cause critical wound, cause fear, communal protection from energy, cure critical, death ward, dismissal, divine power, free movement, greater invisibility, hold animal, hold monster, mass daze monster, remove poison, stone skin, vigorize serious
- L13: banish, banishing blade, communal stone skin, dispel evil, dispel good, flame strike, magic resistance, mass cure light, true seeing, vigorize critical
- L16: blade barrier, greater dispelling, harm, heal, undeath to death
- L21: greater ruin, mummy dust

### Wizard (316 entries)

- L0: acid splash, arcane mark, continual light, detect poison, disrupt undead, enhanced diplomacy, fire bolt, flare, jolt, ray of frost, touch of fatigue
- L1: ant haul, burning hands, charm person, chill touch, color spray, corrosive touch, enchant item, endure elements, expeditious retreat, grease, horizikauls boom, ice dagger, identify, iron guts, mage armor, mage shield, magic missile, minor creation, negative energy ray, planar healing, preserve, protection from evil, protection from good, ray of enfeeblement, resistance, scare, shadow bolt, shelgarns blade, sleep, stunning barrier, summon creature i, summon mount, true strike, ventriloquate
- L3: acid arrow, bestow weapon proficiency, blackthorns, blindness, blur, command undead, communal summon mount, continual flame, cushioning bands, dancing weapon, darkness, daze monster, deafness, detect invisibility, detect magic, endurance, energy sphere, false life, gird allies, glitterdust, grace, hideous laughter, human potential, invisibility, mass ant haul, mirror image, protection from arrows, protection from undead, resist energy, scorching ray, shocking grasp, spider climb, strength, summon creature ii, tactical acumen, touch of idiocy, warding weapon, web
- L5: air blast, aqueous orb, blink, charisma, circle against evil, circle against good, clairvoyance, communal protection from arrows, communal resist energy, communal spider climb, control summoned creature, cunning, daylight, deathly heroism, deep slumber, dispel magic, fireball, flame arrow, fly, gaseous form, greater magic weapon, halt undead, haste, hold person, invisibility sphere, keen edge, lightning bolt, mass identify, minute meteors, nondetection, phantom steed, protection from energy, rage, rejuvenate minor, siphon might, slow, soul bind, stinking cloud, summon creature iii, vampiric touch, wall of fog, water breathe, weapon of impact, wind wall, wisdom
- L7: animate dead, bestow curse, billowing cloud, black tentacles, cause fear, charm monster, cold shield, command horde, communal protection from energy, confusion, embalm, enlarge person, farsee, fire shield, fumble, ghost wolf, greater invisibility, greater planar healing, ice storm, infravision, lesser missile storm, locate creature, mass daze monster, mass enlarge person, mass reduce person, minor globe, poison, rainbow pattern, reduce person, remove curse, shadow jump, spectral hand, stone skin, summon creature iv, wizard eye
- L9: acid sheath, ball of lightning, banishing blade, caustic blood, cloudkill, communal stone skin, cone of cold, dismissal, dominate person, faithful hound, feeblemind, firebrand, geniekind, grand destiny, heal undead, hold monster, hostile juxtaposition, interposing hand, mind fog, nightmare, overland flight, shadow burst, shadow magic, stumble, summon creature v, symbol of pain, telekinesis, thunder lance, wall of force, waves of fatigue
- L11: acid fog, age, anti magic field, circle of death, clone, enervate, eyebite, freezing sphere, globe of invuln, greater black tentacles, greater dispelling, greater heroism, greater mirror image, levitate, locate object, mass haste, mass human potential, nerve dance, rejuvenate major, shadow walk, summon creature vi, tranquility, transformation, true seeing, undeath to death, waterwalk
- L13: beltyns burning blood, camouflage, control weather, corpse glamor, detect poison, displacement, elemental water embodiment, finger of death, grasping hand, greater hostile juxtaposition, ice layer, mass charisma, mass cunning, mass false life, mass fly, mass hold person, mass invisibility, mass wisdom, missile storm, phantasmal blades, power word blind, power word silence, power word stun, prismatic spray, protect undead, protection from spells, sequester, shadechill, shadow flux, spell mantle, summon creature vii, teleport, thunderclap, waves of exhaustion
- L15: airy water, banish, blacklight burst, blackmantle, chain lightning, clenched fist, earth fog, elemental air embodiment, feign death, fire fog, greater animation, horrid wilting, incendiary cloud, iron skin, irresistible dance, mass charm monster, mass domination, mind blank, mislead, phantom heal, portal, rain of blood, refuge, scint pattern, spell turning, summon creature viii, sun shadow, sunburst
- L17: blade of disaster, constriction, death pact, dimension shift, earthblood, elemental earth embodiment, elemental fire embodiment, energy drain, enfeeblement, fell frost, gate, greater spell mantle, ice tomb, implode, lava burst, lich touch, mass enhance, meteor swarm, polymorph self, power word kill, prismatic sphere, rot, sandblast, sandstorm, shadow shield, summon creature ix, timestop, wail of the banshee, weird
- L21: dragon knight, epic mage armor, epic warding, greater ruin, hellball, mummy dust

### Sorcerer (258 entries)

- L0: acid splash, arcane mark, continual light, detect poison, disrupt undead, enhanced diplomacy, fire bolt, flare, jolt, ray of frost, touch of fatigue
- L1: ant haul, burning hands, charm person, chill touch, color spray, corrosive touch, enchant item, endure elements, expeditious retreat, grease, horizikauls boom, ice dagger, identify, iron guts, mage armor, mage shield, magic missile, negative energy ray, planar healing, protection from evil, protection from good, ray of enfeeblement, resistance, scare, shelgarns blade, sleep, stunning barrier, summon creature i, summon mount, true strike, ventriloquate
- L4: acid arrow, bestow weapon proficiency, blindness, blur, communal summon mount, continual flame, cushioning bands, dancing weapon, darkness, daze monster, deafness, detect invisibility, detect magic, endurance, energy sphere, false life, gird allies, glitterdust, grace, hideous laughter, human potential, invisibility, mass ant haul, minor creation, mirror image, protection from arrows, resist energy, scorching ray, shocking grasp, spider climb, strength, summon creature ii, tactical acumen, touch of idiocy, warding weapon, web
- L6: aqueous orb, charisma, circle against evil, circle against good, clairvoyance, communal protection from arrows, communal resist energy, communal spider climb, control summoned creature, cunning, daylight, deathly heroism, deep slumber, dispel magic, fireball, flame arrow, fly, gaseous form, greater magic weapon, halt undead, haste, hold person, invisibility sphere, keen edge, lightning bolt, mass identify, nondetection, phantom steed, protection from energy, rage, siphon might, slow, stinking cloud, summon creature iii, vampiric touch, wall of fog, water breathe, weapon of impact, wind wall, wisdom
- L8: animate dead, bestow curse, billowing cloud, black tentacles, cause fear, charm monster, cold shield, communal protection from energy, confusion, enlarge person, farsee, fire shield, ghost wolf, greater invisibility, greater planar healing, ice storm, infravision, lesser missile storm, locate creature, mass daze monster, mass enlarge person, mass reduce person, minor globe, poison, rainbow pattern, reduce person, remove curse, shadow jump, stone skin, summon creature iv, wizard eye
- L10: acid sheath, ball of lightning, banishing blade, caustic blood, cloudkill, communal stone skin, cone of cold, dismissal, dominate person, faithful hound, feeblemind, firebrand, geniekind, grand destiny, hold monster, hostile juxtaposition, interposing hand, mind fog, nightmare, overland flight, summon creature v, symbol of pain, telekinesis, wall of force, waves of fatigue
- L12: acid fog, anti magic field, circle of death, clone, eyebite, freezing sphere, globe of invuln, greater black tentacles, greater dispelling, greater heroism, greater mirror image, levitate, locate object, mass haste, mass human potential, shadow walk, summon creature vi, transformation, true seeing, undeath to death, waterwalk
- L14: control weather, detect poison, displacement, finger of death, grasping hand, greater hostile juxtaposition, mass charisma, mass cunning, mass false life, mass fly, mass hold person, mass invisibility, mass wisdom, missile storm, power word blind, power word silence, power word stun, prismatic spray, protection from spells, spell mantle, summon creature vii, teleport, thunderclap, waves of exhaustion
- L16: banish, chain lightning, clenched fist, greater animation, horrid wilting, incendiary cloud, iron skin, irresistible dance, mass charm monster, mass domination, mind blank, portal, refuge, scint pattern, spell turning, summon creature viii, sunburst
- L18: blade of disaster, energy drain, enfeeblement, gate, greater spell mantle, implode, mass enhance, meteor swarm, polymorph self, power word kill, prismatic sphere, shadow shield, summon creature ix, timestop, wail of the banshee, weird
- L21: dragon knight, epic mage armor, epic warding, greater ruin, hellball, mummy dust

### Bard (80 entries)

- L0: continual light, enhanced diplomacy, flare, lullaby, summon instrument
- L1: charm person, cure light, endure elements, horizikauls boom, identify, mage shield, magic missile, protection from evil, protection from good, resistance, summon creature i, undetectable alignment
- L4: cure moderate, deafness, detect invisibility, detect magic, endurance, glitterdust, grace, hideous laughter, honeyed tongue, human potential, invisibility, mass identify, minor creation, mirror image, rage, silence, strength, summon creature ii, tactical acumen
- L7: charisma, charm monster, circle against evil, circle against good, confusion, control summoned creature, cunning, cure serious, deep slumber, dispel magic, gaseous form, haste, lightning bolt, summon creature iii, weapon of impact, wisdom
- L10: cure critical, greater invisibility, hold monster, ice storm, mass daze monster, rainbow pattern, remove curse, shadow jump, summon creature iv
- L13: acid sheath, cone of cold, grand destiny, greater dispelling, mass cure light, mind fog, nightmare, shadow walk, summon creature v
- L16: freezing sphere, greater heroism, mass charm monster, mass cure moderate, mass human potential, song of travel, stone skin, summon creature vii
- L21: greater ruin, mummy dust

### Summoner (113 entries)

- L0: acid splash, arcane mark, detect poison
- L1: ant haul, corrosive touch, daze monster, enlarge person, expeditious retreat, grease, identify, lesser rejuvenate eidolon, mage armor, mage shield, magic fang, planar healing, protection from evil, protection from good, reduce person, summon mount
- L4: barkskin, blur, charisma, circle against evil, circle against good, communal summon mount, cunning, cushioning bands, detect invisibility, endurance, ghost wolf, gird allies, glitterdust, grace, haste, human potential, invisibility, lesser evolution surge, lesser restore eidolon, levitate, mass ant haul, protection from arrows, resist energy, slow, spider climb, strength, warding weapon, wind wall, wisdom
- L7: aqueous orb, black tentacles, charm monster, communal protection from arrows, communal resist energy, communal spider climb, control summoned creature, deathly heroism, dispel magic, displacement, evolution surge, fire shield, fly, greater invisibility, greater magic fang, locate creature, mass enlarge person, mass identify, mass reduce person, nondetection, protection from energy, rage, rejuvenate eidolon, restore eidolon, siphon might, stone skin, wall of fire, water breathe
- L10: acid sheath, caustic blood, communal protection from energy, communal stone skin, dismissal, greater evolution surge, greater planar healing, hold monster, hostile juxtaposition, mass charisma, mass cunning, mass daze monster, mass endurance, mass grace, mass strength, mass wisdom, overland flight, teleport
- L13: banish, banishing blade, creeping doom, geniekind, grand destiny, greater dispelling, greater heroism, greater rejuvenate eidolon, invisibility sphere, planar soul, spell turning, true seeing
- L16: dimensional lock, greater hostile juxtaposition, incendiary cloud, mass charm monster, mass human potential, planar ally, purified calling

### Warlock (35 entries)

- L1: beguiling influence, dark ones own luck, devils sight, draining blast, eldritch spear, entropic warding, frightful blast, hideous blow, leaps and bounds, otherworldly whispers, see the unseen, warlocks darkness
- L6: beshadowed blast, brimstone blast, curse of despair, dread seizure, eldritch chain, flee the scene, hellrime blast, the dead walk, voracious dispelling, walk unseen, warlock charm
- L11: bewitching blast, chilling tentacles, devour magic, eldritch cone, noxious blast, tenacious plague, vitriolic blast, wall of perilous flame
- L16: binding blast, dark foresight, eldritch doom, retributive invisibility

### Alchemist (64 entries)

- L1: ant haul, cure light, detect alignment, enlarge person, expeditious retreat, identify, mage shield
- L4: aid, barkskin, bestow weapon proficiency, blur, charisma, cunning, cure moderate, detect invisibility, endurance, false life, grace, human potential, infravision, invisibility, lesser restoration, mass ant haul, protection from arrows, resist energy, spider climb, strength, undetectable alignment, wisdom
- L7: communal protection from arrows, communal resist energy, communal spider climb, cure blind, cure deafness, cure serious, displacement, fly, gaseous form, haste, mass identify, protection from energy, rage, remove curse, remove disease
- L10: caustic blood, cure critical, death ward, fire shield, greater invisibility, minor globe, remove poison, stone skin
- L13: communal stone skin, grand destiny, nightmare, overland flight, polymorph self
- L16: eyebite, heal, shadow walk, transformation, true seeing
- L21: greater ruin, mummy dust

### Psionicist (82 entries)

- L1: broker, call to mind, catfall, crystal shard, deceleration, defensive precognition, demoralize, energy ray, force screen, fortify, inertial armor, inevitable strike, mind thrust, offensive precognition, offensive prescience, psionic vigor, slumber
- L3: bestow power, biofeedback, body equilibrium, breach, concealing amorpha, concussion blast, detect hostile intent, elfsight, energy push, energy stun, inflict pain, mental disruption, psychic bodyguard, recall agony, specified energy adaptation, swarm of crystals, thought shield
- L5: body adjustment, concussive onslaught, endorphin surge, energy burst, energy retort, eradicate invisibility, heightened vision, mental barrier, mind trap, psionic blast, sharpened edge, ubiquitous vision
- L7: deadly fear, death urge, empathetic feedback, energy adaptation, incite passion, intellect fortress, moment of terror, power leech, slip the bonds, wall of ectoplasm, wither
- L9: ectoplasmic shambler, pierce the veils, planar travel, power resistance, psychic crush, psychoport, shatter mind blank, shrapnel burst, tower of iron will, upheaval
- L11: breath of the black dragon, brutalize wounds, disintegration, sustained flight
- L13: cosmic awareness, energy conversion, evade burst, oak body, psychosis, ultrablast
- L15: body of iron, recall death, shadow body, true metabolism
- L17: assimilate

### Artificer (weird science devices)

Artificers have no spell assignment rows. `weird science` builds devices from
spells on the Wizard or Cleric lists whose assignment level fits the device
spell level the artificer can reach: spell level 1 from artificer level 1, 2
from 3, 3 from 5, and 4 from 11. The number of simultaneous devices per spell
level follows `weird_science_table` (for example 1 at level 1, 3/1/1/0 at level
5, 4/3/3/1 at level 11, and 5/5/5/5 at level 20).

## Appendix B: class titles

Titles are assigned by `assign_class_titles()` for level bands 1-4, 5-9, 10-14,
15-19, 20-24, 25-29, 30, immortal, staff, greater staff, and the default. Empty
bands are shown as "-".

| Class | 1-4 | 5-9 | 10-14 | 15-19 | 20-24 | 25-29 | 30 | Immortal | Staff | Greater staff | Default |
|-------|-----|-----|-------|-------|-------|-------|----|----------|-------|---------------|---------|
| Warrior | - | the Mostly Harmless | the Useful in Bar-Fights | the Friend to Violence | the Strong | the Bane of All Enemies | the Exceptionally Dangerous | the Immortal Warlord | the Extirpator | the God of War | the Warrior |
| Rogue | - | the Rover | the Multifarious | the Illusive | the Swindler | the Marauder | the Volatile | the Immortal Assassin | the Demi God of Thieves | the God of Thieves and Tradesmen | the Rogue |
| Monk | - | of the Crushing Fist | of the Stomping Foot | of the Directed Motions | of the Disciplined Body | of the Disciplined Mind | of the Mastered Self | the Immortal Monk | the Inquisitor Monk | the God of the Fist | the Monk |
| Berserker | - | the Ripper of Flesh | the Shatterer of Bone | the Cleaver of Organs | the Wrecker of Hope | the Effulgence of Rage | the Foe-Hewer | the Immortal Warlord | the Extirpator | the God of Rage | the Berserker |
| Ranger | - | the Dirt-watcher | the Hunter | the Tracker | the Finder of Prey | the Hidden Stalker | the Great Seeker | the Avatar of the Wild | the Wrath of the Wild | the Cyclone of Nature | the Ranger |
| Paladin | - | the Initiated | the Accepted | the Hand of Mercy | the Sword of Justice | who Walks in the Light | the Defender of the Faith | the Immortal Justicar | the Immortal Sword of Light | the Immortal Hammer of Justice | the Paladin |
| Blackguard | - | the Novice Blackguard | the Adept Blackguard | the Veteran Blackguard | the Master Blackguard | the Champion Blackguard | the Chosen Blackguard | the Immortal Blackguard | the Immortal Blackguard | the Immortal Blackguard | the Blackguard |
| Cleric | - | the Devotee | the Example | the Truly Pious | the Mighty in Faith | the God-Favored | the One Who Moves Mountains | the Immortal Cardinal | the Inquisitor | the God of Good and Evil | the Cleric |
| Druid | - | the Walker on Loam | the Speaker for Beasts | the Watcher from Shade | the Whispering Winds | the Balancer | the Still Waters | the Avatar of Nature | the Wrath of Nature | the Storm of Earth's Voice | the Druid |
| Inquisitor | the Novice Inquisitor | the Apprentice Inquisitor | the Inquisitor Adept | the Journeyman Inquisitor | the Master Inquisitor | the Grandmaster Inquisitor | the Inquisition Guru | the Immortal Inquisitor | the Limitless Inquisitor | the God of Inquisition | the Inquisitor |
| Wizard | - | the Reader of Arcane Texts | the Ever-Learning | the Advanced Student | the Channel of Power | the Delver of Mysteries | the Knower of Hidden Things | the Immortal Warlock | the Avatar of Magic | the God of Magic | the Wizard |
| Sorcerer | - | the Awakened | the Torch | the Firebrand | the Destroyer | the Crux of Power | the Near-Divine | the Immortal Magic Weaver | the Avatar of the Flow | the Hand of Mystical Might | the Sorcerer |
| Bard | - | the Melodious | the Hummer of Harmonies | Weaver of Song | Keeper of Chords | the Composer | the Maestro | the Immortal Songweaver | the Master of Sound | the Lord of Dance | the Bard |
| Summoner | the Novice Summoner | the Apprentice Summoner | the Summoner Adept | the Journeyman Summoner | the Master Summoner | the Grandmaster Summoner | the Summoning Guru | the Immortal Summoner | the Limitless Summoner | the God of Summoning | the Summoner |
| Warlock | the Fledgling Warlock | the Fledgling Warlock | the Fledgling Warlock | the Fledgling Warlock | the Fledgling Warlock | the Warlock | the Warlock | the Warlock | the Limitless Warlock | the God of Warlocking | the Warlock |
| Alchemist | the Novice Alchemist | the Apprentice Alchemist | the Alchemist Adept | the Journeyman Alchemist | the Master Alchemist | the Grandmaster Alchemist | the Alchemy Guru | the Immortal Alchemist | the Limitless Alchemist | the God of Alchemy | the Alchemist |
| Psionicist | - | the Novice of the Mind | the Adept of the Mind | the Psionicist | the Mind Shaper | the Master of the Mind | the Master of the Mind | the Immortal Mind | the Master of Minds | the Omniscient | the Psionicist |
| Artificer | - | the Reader of Esoteric Devices | the Crafter of Wonders | the Advanced Inventor | the Master of Weird Science | the Delver of Arcane Mechanics | the Artificer Supreme | the Immortal Inventor | the Avatar of Creation | the God of Innovation | the Artificer |
| Weaponmaster | - | the Inexperienced Weapon | the Weapon | the Skilled Weapon | the Master Weapon | the Master of All Weapons | the Unmatched Weapon | the Immortal WeaponMaster | the Relentless Weapon | the God of Weapons | the WeaponMaster |
| Stalwart Defender | - | the Defender | the Immovable | the Wall | the Stalwart Wall | the Wall of Iron | the Wall of Steel | the Immortal Defender | the Indestructible Wall | the God of Defense | the Stalwart Defender |
| Duelist | - | the Acrobatic | the Nimble Maneuver | the Agile Duelist | the Crossed Blade | the Graceful Weapon | the Elaborate Parry | the Immortal Duelist | the Untouchable Duelist | the God of Duelist | the Duelist |
| Dragonrider | - | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider | the Dragon Rider |
| Arcane Archer | - | the Precise Shot | the Magical Archer | the Masterful Archer | the Mystical Arrow | the Arrow Wizard | the Arrow Storm | the Immortal ArcaneArcher | the Limitless Archer | the God of Archery | the ArcaneArcher |
| Eldritch Knight | - | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight | the Eldritch Knight |
| Spellsword | - | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword | the Spell Sword |
| Arcane Shadow | - | the Hidden Cantrip | the Magical Rogue | the Arcane Shadow | the Mystical Shadow | the Rogue Magician | the Stealth Tornado | the Immortal ArcaneShadow | the Limitless ArcaneShadow | the God of ArcaneShadow | the ArcaneShadow |
| Shadowdancer | - | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer | the Shadow Dancer |
| Assassin | - | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin | the Assassin |
| Sacred Fist | - | the Adept Fist | the Holy Hand | the Sacred Fist | the Holy Martial Artist | the Fist of Holy Fire | the Divine Flurry | the Immortal SacredFist | the Limitless SacredFist | the God of SacredFists | the SacredFist |
| Shifter | - | the Shape Changer | the Changeling | the Formless | the Shape Shifter | the Form Changer | the Perpetually Changing | the Immortal Shifter | the Limitless Changeling | the Guru of Shifting | the Shifter |
| Mystic Theurge | - | Acolyte of Duality | - | - | - | - | - | the Immortal MysticTheurge | the Limitless Caster | the God of Magic | the MysticTheurge |
| Necromancer | - | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer | the Necromancer |
| Knight of the Luminous Thread | - | the Squire | the Knight of the Crown | the Knight of the Sword | the Knight of the Rose | the High Knight | the Grand Master | the Legendary Champion of Solamnia | the Immortal Knight | the Avatar of Knighthood | the Knight of Krynn |
| Knight of the Howling Moon | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily | Knight of the Lily |
| Knight of the Shattered Mirror | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn | Knight of the Thorn |
| Knight of the Pale Throne | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull | Knight of the Skull |
