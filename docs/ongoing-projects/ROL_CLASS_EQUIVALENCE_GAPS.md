# RoL Player Classes Without a Close LuminariMUD Equivalent

Status: source audit completed 2026-08-23.

Fourth document in the Realms of Luminari conversion series, after
`ROL_SPELL_EQUIVALENCE_GAPS.md`, `ROL_SKILL_EQUIVALENCE_GAPS.md`, and
`ROL_RACE_EQUIVALENCE_GAPS.md`. This document compares RoL's player classes
with LuminariMUD's current class, prestige-class, specialty-school, race, feat,
and multiclass systems.

All 75 audited functional spell gaps are now implemented, but they deliberately
have no class or domain assignments. References below therefore describe
remaining class-kit integration, not missing spell handlers.

This is necessarily a best-effort comparison. RoL stores one current class and
advances it to mortal level 50. LuminariMUD supports multiclass characters,
base and prestige classes, wizard specialty schools, feats, perks, and a mortal
level cap of 30. Matching class names alone would therefore overstate some
equivalences and understate others.

A class is considered covered when a normal LuminariMUD character can preserve
its central player archetype through one of these routes:

1. a direct or renamed class;
2. a deliberately supported wizard specialty or prestige-class progression;
3. a coherent multiclass, race, feat, or converted-system combination.

The operational `class_map` in `scripts/world/rol_conversion_policy.json` is
reported separately. It is authoritative for converting RoL world content and
mobile class IDs, but it is not proof that the target is an equivalent player
class.

- Real RoL class IDs (`CLASS_WARRIOR` through `CLASS_DIRERAIDER`): 25
- Selectable during RoL character creation: 17
- Attainable only by an in-game class transformation: 1 (`Lich`)
- Defined but unavailable at new character creation: 7
- RoL classes with no dedicated LuminariMUD class: 5
- Active, substantial standalone-class gaps: 2 (`Shaman`, `Elementalist`)
- Active formal-identity gaps with strong build equivalents: 2
  (`Battlechanter`, `Dire Raider`)
- Legacy-only potential class gap: 1 (`Mercenary`)
- Current LuminariMUD class registry slots: 38, comprising 36 named in-game
  classes and 2 disabled placeholders

The five missing dedicated class names should not be treated as equally urgent.
Shaman and Elementalist lose defining progression systems when collapsed into
their converter targets. Battlechanter and Dire Raider lose formal identity and
some bespoke progression, but most of their play can already be assembled from
current mechanics. Mercenary was no longer selectable in RoL and is already
well modeled by multiclassing.

## Dedicated-class gaps

| RoL ID | RoL class | RoL state | Converter target | Best current player expression | Assessment |
|-------:|-----------|-----------|------------------|--------------------------------|------------|
| 9 | Shaman | Selectable | Cleric | Cleric plus converted RoL totem support | Active, substantial gap |
| 15 | Mercenary | Creation disabled | Warrior | Warrior/Rogue multiclass | Legacy potential only |
| 22 | Battlechanter | Selectable, Orc only | Bard | Bard with Warrior or Cleric levels | Active formal-identity gap |
| 24 | Elementalist | Selectable | Wizard | Invoker/Conjurer Wizard or Summoner | Active, substantial gap |
| 25 | Dire Raider | Selectable, Orc only | Warrior | Ranger/Warrior mounted build | Active formal-identity gap |

### Shaman (9)

Shaman is the clearest missing player class. It is a live RoL choice with a
priestly, nature, ancestor, and spirit identity rather than merely a differently
named Cleric. Its live registry contains 67 spell assignments and 24 skill
assignments. Its defining progression includes a spirit specialization and a
choice among 21 totem identities, followed by summoning the bonded totem spirit.

LuminariMUD's world converter maps Shaman to Cleric. The converted totem system
in `src/spec/spec_rol_totem.c` preserves the 21 totem identities, the permanent
player choice, restoration, use limits, and the summoned spirit. It substitutes
Cleric level 21 and Wisdom for the source class and skill requirements. This is
valuable compatibility support for converted content, but it is not a native
Shaman choice or a full Shaman progression.

The unassigned Shaman kit spells are `farsee`, `preserve`, `command undead`,
`soul tempest`, `spirit walk`, and `ancestral shield`. Those are kit-integration
gaps rather than proof that six entirely new class mechanics are needed; their
implementations are specified in `ROL_SPELL_EQUIVALENCE_GAPS.md`.

**Assessment:** add a native Shaman only if preserving the RoL class identity is
a product goal. The existing converted totem implementation is the natural
foundation, while Cleric and Druid mechanics can supply much of the ordinary
divine/nature casting. A smaller alternative is a Cleric archetype, perk tree,
or class feature package that exposes the totem and spirit progression to normal
players.

### Elementalist (24)

Elementalist is the other substantial active gap. RoL explicitly migrated old
Conjurers into Elementalists through `convert_conjurer_to_elementalist()` and
made Elementalist a live creation choice. It combines elemental spellcasting,
creation and summoning themes, and dedicated elemental embodiment mechanics.
Its live registry contains 48 spell assignments and 24 skill assignments.

LuminariMUD maps it to Wizard. Wizard is a strong chassis, but the current
specialty list has Universalist and the eight conventional schools; it has no
Elementalist specialty. An Invoker emphasizes evocation and a Conjurer or
Summoner covers the summoning half, but neither reproduces a persistent
elemental specialization and embodiment progression.

The unassigned Elementalist kit spells are `minor creation`, `thunder lance`,
`air blast`, `earthblood`, `earth fog`, and `fire fog`.

**Assessment:** preserve this either as a class or as an explicit Wizard
elemental specialty with an embodiment/perk track. A full base class should be
chosen only if the non-spell progression is intended to remain distinct from
Wizard and Summoner.

### Battlechanter (22)

Battlechanter is an active Orc-only RoL class: a martial and shamanic war
performer using drums, whoops, and chants to support troops. The converter maps
it to Bard. That preserves the performance role, and current Bard performance
mechanics plus `FEAT_ACCOMPANY` cover the core group-support loop. Adding
Warrior or Cleric levels supplies most of the martial or shamanic side.

No ordinary `SPELL_ADD` spell from Battlechanter's registered kit lacks an
implementation. Its shared Bard performance engine also supplies `song of
travel`, whose handler is implemented but remains unassigned. LuminariMUD still
lacks the Orc-locked war-chant identity and its dedicated level progression.

**Assessment:** a formal class-identity gap, but a lower mechanical priority.
Prefer a Bard archetype, perk line, or themed build guidance over a new base
class unless source-faithful class selection is required.

### Dire Raider (25)

Dire Raider is an active Orc-only light-cavalry class built around a dire-wolf
mount, outdoor survival, stealth, archery, slashing weapons, nature/spirit
magic, and mounted combat. Source code restricts its mount identity to a wolf
and supplies dedicated dire-wolf summoning and mounted modifiers.

The converter maps it to Warrior, which is safe for mobile combat statistics
but is too narrow as a player-equivalence claim. A Ranger/Warrior build with an
animal companion and the mounted-combat feat chain is the closer current
expression. It still lacks a formal dire-wolf bond and source-faithful class
progression. Dragonrider is not a close substitute because its mount and
fantasy identity are materially different.

Its unassigned kit spells are `farsee`, `command undead`, `protection from
animals`, `dust devil`, `pass without trace`, and `poltergeist`.

**Assessment:** an active formal-identity gap with substantial existing
mechanical coverage. A Ranger archetype, companion option, or feat/perk package
is a better first fit than a new base class.

### Mercenary (15)

Mercenary's creation case is disabled in RoL and it has no dedicated class help
entry. Its live registry is a 26-skill Warrior/Rogue mixture: broad weapons,
archery, bash, kick, rescue, and offense alongside hide, escape, backstab,
dodge, unbind, dual wield, riposte, and related martial techniques. It has no
registered spells and no remaining live skill-equivalence gaps.

The converter maps it to Warrior, while a Warrior/Rogue multiclass is a closer
player build.

**Assessment:** retain as a documented legacy mapping, not a default port
candidate. A new Mercenary class would mostly duplicate the reason
LuminariMUD supports multiclassing.

## Complete 25-class mapping

`RoL state` is based on both the race/class permission matrix and the live
`select_class()` switch. A class appearing in only one of those is not actually
selectable. The static creation table is not used as sole authority because it
contains stale class labels.

| ID | RoL class | RoL state | Converter target | Closest LuminariMUD player expression | Result |
|---:|-----------|-----------|------------------|----------------------------------------|--------|
| 1 | Warrior | Selectable | Warrior | Warrior | Close direct equivalent |
| 2 | Ranger | Selectable | Ranger | Ranger | Close direct equivalent |
| 3 | Berserker | Creation disabled | Berserker | Berserker | Direct equivalent for a legacy class |
| 4 | Paladin | Selectable | Paladin | Paladin | Close direct equivalent |
| 5 | Anti-Paladin | Selectable | Blackguard | Blackguard, explicitly described as an antipaladin | Close renamed equivalent |
| 6 | Cleric | Selectable | Cleric | Cleric | Close direct equivalent |
| 7 | Monk | Creation disabled | Monk | Monk | Direct equivalent for a legacy class |
| 8 | Druid | Selectable | Druid | Druid | Close direct equivalent |
| 9 | Shaman | Selectable | Cleric | Cleric plus converted totem support | Dedicated-class gap |
| 10 | Sorcerer | No legal PC race | Sorcerer | Sorcerer | Direct equivalent for an inaccessible class |
| 11 | Necromancer | Selectable | Necromancer | Necromancy Wizard plus Necromancer prestige class | Composite partial equivalent |
| 12 | Conjurer | Retired and auto-converted | Wizard | Conjuration Wizard or Summoner | Close structural equivalent |
| 13 | Thief | No legal PC race | Rogue | Rogue | Close successor equivalent |
| 14 | Assassin | No legal PC race | Assassin | Rogue into Assassin prestige class | Close structural equivalent |
| 15 | Mercenary | Creation disabled | Warrior | Warrior/Rogue multiclass | Legacy dedicated-class gap |
| 16 | Bard | Selectable | Bard | Bard | Close direct equivalent |
| 17 | Psionicist | Selectable | Psionicist | Psionicist | Close direct equivalent |
| 18 | Lich | Transformation only | Necromancer | Necromancer-gated rite into Lich race plus Wizard | Composite close equivalent |
| 19 | Enchanter | Selectable | Wizard | Enchantment-specialist Wizard | Close structural equivalent |
| 20 | Invoker | Selectable | Wizard | Evocation-specialist Wizard | Close structural equivalent |
| 21 | Illusionist | Selectable | Wizard | Illusion-specialist Wizard | Close structural equivalent |
| 22 | Battlechanter | Selectable, Orc only | Bard | Bard plus Warrior or Cleric | Active formal-identity gap |
| 23 | Rogue | Selectable | Rogue | Rogue | Close direct equivalent |
| 24 | Elementalist | Selectable | Wizard | Invoker/Conjurer Wizard or Summoner | Dedicated-class gap |
| 25 | Dire Raider | Selectable, Orc only | Warrior | Ranger/Warrior mounted build | Active formal-identity gap |

## Structural and composite cases

These classes need more explanation than a same-name search provides, but they
do not justify additional standalone gap entries by themselves.

### Necromancer and Lich

RoL Necromancer is a level-50 base-class path. At its cap it can perform a rite
that changes the character to `CLASS_LICH` and returns it at level 46 with a
distinct Lich kit.

LuminariMUD expresses the same lifecycle differently:

- Wizard can select Necromancy as a specialty school.
- Necromancer is a ten-level prestige class that advances a chosen arcane or
  divine casting progression and supplies undead cohorts, undead summoning,
  bone armor, grafts, and touch abilities.
- The converted RoL lich rite requires overall level 30 and at least one
  Necromancer prestige level, consumes its two offerings, changes the character
  to the fully registered Lich race, and respecs the character as a Wizard.
- A separate current quest reward can also grant the Lich race.

That is a deliberate composite equivalent, not a missing Lich class. It is not
mechanically identical: Necromancer has 17 and Lich has 21 implemented spells
still awaiting class-kit assignment, and the level-reset/class-change behavior
differs. Those residuals belong to kit and progression parity work rather than
an automatic proposal for two more class IDs.

### Conjurer

Conjurer is not an active RoL destination. Its creation switch is absent, and
the player loader calls `convert_conjurer_to_elementalist()` for old characters.
LuminariMUD nevertheless has two coherent homes for its older identity:
Conjuration-specialist Wizard for the school-based caster and Summoner for a
companion-centered class. Its two unassigned kit spells are `minor creation`
and `ventriloquate`.

### Enchanter, Invoker, and Illusionist

RoL makes these separate classes. LuminariMUD makes Enchantment, Evocation, and
Illusion explicit Wizard specialty choices and applies school-specific
restrictions and enhancements. That is a system-model conversion, not three
missing classes. Their residual spell kits are listed below so the structural
mapping is not mistaken for complete spell parity.

### Anti-Paladin, Thief, and Assassin

Blackguard's current class description explicitly calls Blackguards
antipaladins, so Anti-Paladin is a rename rather than a gap. RoL Thief and
Assassin have no race that can select them at creation; active Rogue is the
successor to the old mundane skill-class role. LuminariMUD Rogue plus its
locked Assassin prestige class preserves the relevant player route.

## Unassigned spell and performance-kit integrations by class

This section joins RoL's live class registrations to the 75 implemented but
intentionally unassigned spells audited in `ROL_SPELL_EQUIVALENCE_GAPS.md`. It
also assigns `song of travel` to Bard and Battlechanter from the live shared
performance engine in `newbard.c`; that performance is not registered with
`SPELL_ADD`.
A spell learned by several classes appears in several rows, so the per-class
counts must not be summed. A zero count means only that the class has no spell
awaiting assignment from this inventory; it does not prove complete
class-behavior parity.

| RoL class | Count | Implemented spells awaiting class-kit integration |
|-----------|------:|------------------------------------------------------|
| Ranger | 5 | `natures blessing`, `create spring`, `protection from animals`, `dust devil`, `pass without trace` |
| Anti-Paladin | 3 | `curse item`, `command undead`, `spectral hand` |
| Cleric | 5 | `curse item`, `preserve`, `command undead`, `slow poison`, `greater realm of protection` |
| Druid | 9 | `preserve`, `create spring`, `moonwell`, `protection from animals`, `dust devil`, `suffocate`, `pass without trace`, `rock to mud`, `mud to rock` |
| Shaman | 6 | `farsee`, `preserve`, `command undead`, `soul tempest`, `spirit walk`, `ancestral shield` |
| Sorcerer | 3 | `minor creation`, `farsee`, `ventriloquate` |
| Necromancer | 17 | `rejuvenate major`, `age`, `rejuvenate minor`, `minor creation`, `preserve`, `ventriloquate`, `command undead`, `protect undead`, `protection from undead`, `command horde`, `corpse glamor`, `spectral hand`, `nerve dance`, `rain of blood`, `beltyns burning blood`, `blackmantle`, `soul bind` |
| Conjurer | 2 | `minor creation`, `ventriloquate` |
| Bard | 2 | `minor creation`, `song of travel` |
| Lich | 21 | `rejuvenate major`, `age`, `rejuvenate minor`, `minor creation`, `preserve`, `ventriloquate`, `command undead`, `protect undead`, `protection from undead`, `command horde`, `corpse glamor`, `spectral hand`, `nerve dance`, `rain of blood`, `beltyns burning blood`, `blackmantle`, `death pact`, `soul bind`, `embalm`, `rot`, `ice tomb` |
| Enchanter | 9 | `fumble`, `stumble`, `enervate`, `minor creation`, `farsee`, `constriction`, `airy water`, `blink`, `blacklight burst` |
| Invoker | 6 | `minor creation`, `farsee`, `sandstorm`, `sandblast`, `minute meteors`, `fell frost` |
| Illusionist | 17 | `minor creation`, `farsee`, `shadow bolt`, `phantasmal blades`, `shadow burst`, `mislead`, `sequester`, `dimension shift`, `shadow magic`, `blackthorns`, `feign death`, `tranquility`, `phantom heal`, `shadechill`, `shadow flux`, `corpse glamor`, `sun shadow` |
| Battlechanter | 1 | `song of travel` |
| Elementalist | 6 | `minor creation`, `thunder lance`, `air blast`, `earthblood`, `earth fog`, `fire fog` |
| Dire Raider | 6 | `farsee`, `command undead`, `protection from animals`, `dust devil`, `pass without trace`, `poltergeist` |

Six other implemented spells have no live class registration and therefore
cannot be attributed to a class from source evidence: `comprehend languages`,
`wraithform`, `unseen servant`,
`needle swarm`, `snapping teeth`, and `agility`.

The following classes have zero spells awaiting assignment from this inventory:
Warrior, Berserker, Paladin, Monk, Thief, Assassin, Mercenary, Psionicist, and
Rogue. Psionicist was cross-checked against the separate live psionic registry
in the spell audit; its zero is not an artifact of looking only at ordinary
spells.

`ROL_SKILL_EQUIVALENCE_GAPS.md` records that all five formerly missing live
non-psionic skill equivalents have now been ported. Consequently there is no
separate outstanding class-by-class skill-gap table here. This says nothing
about exact learn rates, level gates, percentage caps, or class progression.

## Defined but unavailable in RoL

Seven class IDs cannot be chosen by a new RoL character. They remain in scope
because old characters, content, save conversion, and class registries can
still refer to them.

| RoL ID | Class | Why it is unavailable |
|-------:|-------|-----------------------|
| 3 | Berserker | Its creation switch case is inside `#if 0`. An old disabled migration describes removal for balance and conversion to Warrior. |
| 7 | Monk | Its creation switch case is inside `#if 0`; registered mechanics remain. |
| 10 | Sorcerer | The selection switch has a case, but no PC race permits the class. |
| 12 | Conjurer | The race matrix permits it, but the live selection switch does not; old characters are converted to Elementalist on load. |
| 13 | Thief | The selection switch has a case, but no PC race permits the class. |
| 14 | Assassin | The selection switch has a case, but no PC race permits the class. |
| 15 | Mercenary | Its creation switch case is inside `#if 0`. |

Lich is not in this table because its absence from character creation is
intentional: it is reached through the Necromancer transformation path.

## System differences that are not class gaps

- RoL's single-class, level-50 progression cannot be compared level-for-level
  with LuminariMUD's multiclass, level-30 progression.
- Separate RoL school classes can legitimately map to Wizard specialties.
- A RoL class can map to a base/prestige sequence, such as Rogue into Assassin
  or Wizard into Necromancer, without requiring a same-shaped base class.
- RoL's race/class permission matrix and fixed creation choices do not need to
  be preserved when a current race, alignment, prerequisite, or unlock system
  carries the same player-facing constraint.
- An operational converter target is allowed to be deliberately broad. Mobile
  and guild conversion needs a stable current class ID; it does not need to
  express every player-build nuance.
- Registered spells and skills describe a kit but do not capture every class
  rule. The bespoke Shaman totem, Elementalist embodiment, Dire Raider mount,
  and Lich transformation code were therefore traced separately.

## Recommendation and port order

If the question is specifically which RoL class should be imported as a full
LuminariMUD class, the serious shortlist contains one entry: **Shaman**. It has
a durable fantasy identity, a distinctive spirit/totem progression, substantial
source mechanics, and an already ported 21-totem foundation. A design pass
should still compare a full class with a Cleric-based archetype, but Shaman is
the only source class for which a new class ID is presently justified enough to
investigate.

Elementalist is the serious second feature candidate, not the second automatic
class candidate. Preserve its elemental choice and embodiment mechanics first
as a Wizard specialty, perk tree, or archetype. Promote it to a full class only
if that design cannot express the intended progression without excessive
exceptions or overlap with Invoker, Conjurer, and Summoner.

Dire Raider and Battlechanter should begin as Ranger and Bard packages,
respectively. Mercenary should remain a legacy Warrior/Rogue mapping.
Necromancer/Lich and the three conventional school specialists already have
coherent current structural homes and should not be imported as duplicate base
classes.

Recommended sequence:

1. Prototype Shaman around the existing converted totem implementation and the
   missing spirit kit; compare a full class against a Cleric archetype.
2. Prototype an Elementalist Wizard specialty and embodiment/perk track.
3. Add a Dire Raider-style dire-wolf companion and mounted Ranger package if
   that player fantasy is wanted.
4. Add a Battlechanter war-performance Bard package if the Orc war-chanter
   identity is wanted.
5. Leave Mercenary as a legacy mapping unless old-player compatibility reveals
   requirements that multiclassing cannot meet.
6. Track per-class spells through `ROL_SPELL_EQUIVALENCE_GAPS.md`; do not make a
   new class merely to house an unassigned spell.

## Verification method

1. RoL's real class range was taken from `race_class.h`: IDs 1 through 25,
   excluding `CLASS_NONE` (0). `LAST_CLASS` is 25.
2. Creation availability was resolved by intersecting the PC-race permission
   rows in `class_table[][]` with the compiled cases in `select_class()`.
   Preprocessor-disabled cases were excluded. This produces 17 selectable
   classes.
3. Lich attainability was confirmed in `convertNecroToLich()`. Conjurer's
   retirement was confirmed in `convert_conjurer_to_elementalist()` and its
   player-load call site.
4. RoL class kits were generated from the live, preprocessed `SPELL_ADD` and
   `SKILL_ADD` registrations in `sparser.c`, so disabled registry blocks were
   not counted. `song of travel` was then linked to Bard and Battlechanter from
   their shared live engine in `newbard.c`.
5. LuminariMUD classes were taken from `NUM_CLASSES`, the class constants, and
   live `classo()` registrations. The `in_game` field distinguishes the two
   placeholders from the 36 named registrations.
6. Wizard specialty equivalence was confirmed from the registered schools and
   specialty behavior in `src/magic/domains_schools.c` and `src/magic/magic.c`.
7. Converter targets were taken from the current policy file rather than
   inferred from names.
8. The residual spell lists were mechanically joined against the audited live
   spell inventory. The separate psionic registry and the completed skill-gap
   audit were also checked.

## Source authority

- RoL class IDs and class range:
  `RealmsOfLuminari/src/race_class.h`
- RoL class names, race permissions, help text, and class support data:
  `RealmsOfLuminari/src/race_class.c`
- RoL live class selection:
  `RealmsOfLuminari/src/nanny.c`, `select_class()`
- RoL player class persistence, legacy migrations, and Conjurer load upgrade:
  `RealmsOfLuminari/src/files.c`
- RoL spell and skill registration:
  `RealmsOfLuminari/src/sparser.c`
- RoL Bard and Battlechanter performance engine:
  `RealmsOfLuminari/src/newbard.c`
- RoL Shaman registration and totem mechanics:
  `RealmsOfLuminari/src/sparser.c`, `RealmsOfLuminari/src/specs.object.c`,
  `RealmsOfLuminari/src/specs.mobile.c`, and
  `RealmsOfLuminari/src/specs.assign.c`
- RoL Elementalist migration and mechanics:
  `RealmsOfLuminari/src/elementalist.c`
- RoL Dire Raider mount and ranged-combat behavior:
  `RealmsOfLuminari/src/actnoff.c`, `RealmsOfLuminari/src/missile.c`
- RoL Necromancer-to-Lich transformation:
  `RealmsOfLuminari/src/necro.c`, `convertNecroToLich()`
- LuminariMUD class IDs and registry:
  `src/structs.h`, `src/character/class.c`
- LuminariMUD Wizard schools:
  `src/magic/domains_schools.c`, `src/magic/magic.c`
- LuminariMUD converted Shaman totems:
  `src/spec/spec_rol_totem.c`
- LuminariMUD converted Lich rite and guild compatibility:
  `src/spec/spec_rol_conversion.c`
- LuminariMUD Lich race reward:
  `src/quest/quest.c`
- Operational RoL class conversion map:
  `scripts/world/rol_conversion_policy.json`
- Existing kit-gap audits:
  `docs/ongoing-projects/ROL_SPELL_EQUIVALENCE_GAPS.md`,
  `docs/ongoing-projects/ROL_SKILL_EQUIVALENCE_GAPS.md`
