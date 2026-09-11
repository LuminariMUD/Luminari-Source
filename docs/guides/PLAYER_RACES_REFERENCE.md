# Player Races Reference

Status: source-backed reference, verified 2026-09-11 against `src/character/race.c`,
`src/account.c`, `src/structs.h`, `src/quest/quest.c`, `src/quest/hlquest.c`,
`src/spec/spec_rol_conversion.c`, `src/character/feats.c`, `src/constants.c`,
`src/combat/fight.c`, and `src/limits.c`.

This document is the reference center for playable (player-character) races. It
lists every race a player can hold, its ability modifiers, size, alignment
limits, innate feats, unlock cost, and how it is acquired, and it sets the
race point budget used to balance races within and across tiers. Non-player race
families (animals, elementals, plants, wildshape forms) are out of scope; they
are registered with `is_pc = FALSE` and are covered by the NPC and wildshape
material instead.

## Related documents

| Document | What it covers |
|----------|----------------|
| [ADDING_NEW_RACE_GUIDE.md](ADDING_NEW_RACE_GUIDE.md) | Developer procedure for adding a race: ID allocation, `assign_races()` registry, creation wiring, mechanics, unlock gate, transformation-only races, help, persistence, tests, deployment |
| [PLAYER_CLASSES_REFERENCE.md](PLAYER_CLASSES_REFERENCE.md) | Playable classes, prestige prerequisites (including the race rows Arcane Archer checks), class unlock costs, and the class side of race/class alignment compatibility |
| [ADDING_NEW_PLAYER_CLASS_GUIDE.md](ADDING_NEW_PLAYER_CLASS_GUIDE.md) | Class registry and the race/class compatibility and unlock rules that character creation enforces |
| [PLAYER_MANAGEMENT_SYSTEM.md](../systems/PLAYER_MANAGEMENT_SYSTEM.md) | Account model, `account->races[]` unlock storage, and the `CON_QRACE` / `CON_QRACE_HELP` creation states |
| [NEW_PLAYER_GUIDE_LEVEL_1-5.md](NEW_PLAYER_GUIDE_LEVEL_1-5.md) | Player-facing introduction to race, class, and early levels |
| [DEVELOPER_GUIDE_AND_API.md](DEVELOPER_GUIDE_AND_API.md) | General developer reference, including the feat and class registries that racial feats plug into |
| `lib/text/help/help.hlp` | Player help. Entries `RACE`, `RACES`, `LOCKED-RACES`, `EPIC-RACES`, `ACCEXP`, `ACCOUNT-EXPERIENCE`, and one `RACE-<NAME>` entry per race (for example `RACE-MOON-ELF`, `RACE-DUERGAR`, `RACE-YUAN-TI`). The same content is mirrored in the help database |
| In-game commands | `race list`, `race info <name>`, `race feats <name>`, `accexp race`, `accexp race <name>`, `account` |

Source of truth for every table below:

| Data | Where it is defined |
|------|---------------------|
| Race IDs and synonyms | `src/structs.h` (`RACE_*` defines, `NUM_EXTENDED_RACES`) |
| Registry entries | `assign_races()` in `src/character/race.c` via `add_race()`, `set_race_abilities()`, `set_race_alignments()`, `set_race_attack_types()`, `set_race_wear_restriction()`, `feat_race_assignment()`, and `race_list[].racial_language` |
| Unlock and availability | `is_locked_race()`, `has_unlocked_race()`, `locked_race_cost()`, `do_accexp()` in `src/account.c`; `race_is_creation_eligible()`, `race_is_selectable_for_creation()` in `src/character/race.c` |
| Feat text | `feato()` calls in `assign_feats()` in `src/character/feats.c` |
| Lich and Vampire conversion | `src/quest/quest.c`, `src/quest/hlquest.c`, `src/spec/spec_rol_conversion.c` |

## How race tiers and unlocking work

Every registry entry carries three tier fields: `level_adjustment`, `unlock_cost`,
and `epic_adv` (`IS_NORMAL`, `IS_ADVANCE`, or `IS_EPIC_R`).

- **Normal races** have `unlock_cost 0`. They are never locked and are always
  selectable at character creation on both the terminal and web creation paths.
- **Advanced races** cost 1000 account experience and carry level adjustment +2.
- **Epic races** cost 30000 or 50000 account experience and carry level
  adjustment +10.
- **Transformation-only races** (Lich, Vampire) are registered with a cost of
  999999999 and are hard-blocked in `has_unlocked_race()` and
  `race_is_creation_eligible()`. They cannot be bought or picked at creation
  regardless of account experience; see "Acquisition paths" below.

A race is locked when its `unlock_cost` is greater than zero. `accexp race` with
no argument lists the locked, creation-eligible races the account has not yet
unlocked, with their cost. `accexp race <name>` spends account experience and
records the race ID in the account's unlock array, which holds up to 50 entries
(`MAX_UNLOCKED_RACES`). An unlock applies to every current and future character
on that account. Alignment changes through `accexp align` cost 2000 account
experience each and are unrelated to race.

`level_adjustment` is stored in the registry and shown as a "Level adjustment"
fact by web onboarding (`src/net/onboarding.c`). No experience-table code
currently reads it; the "require a lot more experience" note in the
`LOCKED-RACES` help entry is not backed by a traced experience penalty. The
`EPIC-RACES` help note that epic races cannot multiclass is likewise not
enforced: the check in `src/character/class.c` is commented out and marked
"disabled!".

Racial ability modifiers come only from the registry (`set_race_abilities()`).
The older per-race switches in `comp_base_str()` and friends in `src/utils.c`
are commented out and inactive.

## Playable races at a glance

Ability modifiers are listed as STR / CON / INT / WIS / DEX / CHA. Alignment
lists only the restriction; "any" means all nine alignments are allowed.

### Normal races (always available)

| Race | ID | Size | STR/CON/INT/WIS/DEX/CHA | Alignment | Language |
|------|----|------|-------------------------|-----------|----------|
| Human | 0 | Medium | 0/0/0/0/0/0 | any | - |
| Moon Elf | 1 | Medium | 0/0/0/+1/+2/0 | any | - |
| Mountain Dwarf | 2 | Medium | +2/+2/0/0/0/0 | any | - |
| Lightfoot Halfling | 5 | Small | 0/0/0/0/+2/+1 | any | - |
| Half Elf | 6 | Medium | 0/0/0/0/0/+2 | any | - |
| HalfOrc | 7 | Medium | +2/+1/0/0/0/0 | any | - |
| Rock Gnome | 8 | Small | 0/+1/+2/0/0/0 | any | - |
| High Elf | 13 | Medium | 0/0/+1/0/+2/0 | any | Elven |
| Wild Elf | 14 | Medium | +1/0/0/0/+2/0 | any | - |
| Half Drow | 15 | Medium | 0/0/+2/0/0/0 | any | Undercommon |
| Dragonborn | 16 | Medium | +2/0/0/0/0/+1 | any | Draconic |
| Tiefling | 17 | Medium | 0/0/+1/0/0/+2 | any | Abyssal |
| Stout Halfling | 18 | Small | 0/+1/0/0/+2/0 | any | Halfling |
| Forest Gnome | 19 | Small | 0/0/+2/0/+1/0 | any | Gnomish |
| Gold Dwarf | 20 | Medium | 0/+2/0/+1/0/0 | any | - |
| Aasimar | 21 | Medium | 0/0/0/+1/0/+2 | any | Celestial |
| Tabaxi | 22 | Medium | +1/0/0/0/+2/0 | any | - |
| Goliath | 23 | Medium | +2/+1/0/0/0/0 | any | Giant |
| Shade | 24 | Medium | 0/0/0/0/+2/+1 | any | Common |
| Goblin | 26 | Small | -2/+2/0/0/+4/-2 | any | Goblin |
| Hobgoblin | 27 | Medium | 0/+2/0/0/+1/0 | any | Goblin |

Registry ID synonyms: `RACE_MOON_ELF` is `RACE_ELF`, `RACE_SHIELD_DWARF` is
`RACE_DWARF`, `RACE_LIGHTFOOT_HALFLING` is `RACE_HALFLING`, and
`RACE_ROCK_GNOME` is `RACE_GNOME`. These pairs share one persistent identity;
the older constant name still appears in legacy help keywords (`RACE-ELF`,
`RACE-DWARF`, `RACE-HALFLING`).

### Advanced races (1000 account experience, level adjustment +2)

| Race | ID | Size | Family | STR/CON/INT/WIS/DEX/CHA | Alignment | Language |
|------|----|------|--------|-------------------------|-----------|----------|
| HalfTroll | 3 | Large | Humanoid | +2/+4/-2/-2/+2/-2 | not LG or NG | - |
| ArcanaGolem | 10 | Medium | Humanoid | 0/0/+3/+3/0/+3 | any | - |
| Drow | 11 | Medium | Humanoid | 0/0/+4/+2/+2/+2 | any | - |
| Duergar | 12 | Medium | Humanoid | +2/+4/0/0/0/0 | any | - |
| Half-Ogre | 28 | Large | Giant | +6/+2/-2/0/-2/-2 | any | Giant |
| Wemic | 149 | Large | Monstrous humanoid | +8/+4/-2/+2/+2/-2 | any | Common |
| Yuan-Ti | 151 | Medium | Monstrous humanoid | 0/0/+2/0/+4/+2 | non-good only | Draconic |

### Epic races (level adjustment +10)

| Race | ID | Size | Family | Cost | STR/CON/INT/WIS/DEX/CHA | Alignment | Language |
|------|----|------|--------|------|-------------------------|-----------|----------|
| CrystalDwarf | 4 | Medium | Humanoid | 30000 | +2/+4/0/+4/+2/+2 | any | - |
| Trelux | 9 | Small | Humanoid | 30000 | +4/+4/0/0/+4/0 | any | - |
| Fae | 25 | Tiny | Humanoid | 50000 | -4/0/0/0/+10/+6 | non-lawful only | Elven |
| Half-Illithid | 150 | Medium | Aberration | 30000 | 0/0/+4/+4/0/+4 | any | Aberration |

### Transformation-only races (not purchasable, not selectable at creation)

| Race | ID | Size | Family | STR/CON/INT/WIS/DEX/CHA | Alignment |
|------|----|------|--------|-------------------------|-----------|
| Lich | 45 | Medium | Undead | 0/+2/+6/+2/+2/+6 | evil only (LE, NE, CE) |
| Vampire | 46 | Medium | Undead | +6/+4/+2/+2/+4/+4 | any |

Both are registered as `IS_EPIC_R` with level adjustment +10 and a nominal cost
of 999999999 that is never checked because the account gate rejects them first.

## Acquisition paths

- **Normal races**: pick at character creation. `race_is_selectable_for_creation()`
  returns true for any creation-eligible race that is not locked.
- **Advanced and epic races**: unlock once per account with `accexp race <name>`,
  then pick at character creation on any character. The `accexp` name match uses
  the registry `type` string (for example `accexp race HalfTroll`,
  `accexp race CrystalDwarf`).
- **Lich**: three owners set `GET_REAL_RACE(ch) = RACE_LICH` on an existing
  character.
  - Quest reward: a quest whose reward race is Lich (editable in `qedit`)
    converts the character and respecs them to Wizard (`src/quest/quest.c`).
  - Legacy hlquest: the `LICH_QUEST` command requires level 30 and no group,
    master, or followers, then converts and respecs to Wizard
    (`src/quest/hlquest.c`).
  - Lich Rite special procedure: requires Necromancer levels, character level
    exactly 30 (`LVL_IMMORT - 1`), no group, master, or followers, and two
    offering objects held by the rite keeper (`src/spec/spec_rol_conversion.c`).
- **Vampire**: quest reward only. A quest whose reward race is Vampire converts
  the character and respecs them to Warrior (`src/quest/quest.c`).

## Innate feats and special traits by race

Every feat below is granted at level 1 and does not stack unless noted. Feat
names are the `FEAT_*` constants with the prefix dropped; `race feats <name>` in
game and `feat info <name>` show the player-facing text. Unarmed attack verbs
default to hit and punch; only races with a different set are listed.

### Human
Quick To Master, Skilled.

### Moon Elf
Infravision, Weapon Proficiency Elf, Sleep Enchantment Immunity, Keen Senses,
Resistance To Enchantments, Elf Racial Adjustment, Moon Elf Racial Adjustment,
Moon Elf Bathed In Moonlight, Moon Elf Lunar Magic.

### High Elf
Infravision, Weapon Proficiency Elf, Sleep Enchantment Immunity, Keen Senses,
Resistance To Enchantments, Elf Racial Adjustment, High Elf Racial Adjustment,
High Elf Cantrip, High Elf Linguist.

### Wild Elf
Infravision, Weapon Proficiency Elf, Sleep Enchantment Immunity, Keen Senses,
Resistance To Enchantments, Elf Racial Adjustment, Wood Elf Racial Adjustment,
Wood Elf Fleetness, Wood Elf Mask Of The Wild.

### Half Elf
Infravision, Weapon Proficiency Elf, Half Blood, Adaptability, Keen Senses,
Resistance To Enchantments, Half Elf Racial Adjustment.

### Half Drow
Infravision, Weapon Proficiency Drow, Half Blood, Half Drow Spell Resistance,
Keen Senses, Resistance To Enchantments, Half Drow Racial Adjustment.

### Dragonborn
Dragonborn Breath, Dragonborn Resistance, Dragonborn Fury, Dragonborn Racial
Adjustment.

### Tiefling
Infravision, Tiefling Hellish Resistance, Bloodhunt, Tiefling Racial
Adjustment, Tiefling Magic.

### Aasimar
Ultravision, Astral Majesty, Celestial Resistance, Aasimar Racial Adjustment,
Aasimar Healing Hands, Aasimar Light Bearer.

### Tabaxi
Infravision, Tabaxi Racial Adjustment, Tabaxi Cats Claws, Tabaxi Cats Talent,
Tabaxi Feline Agility. Unarmed attacks: hit, claw, punch, rake.

### Mountain Dwarf
Infravision, Poison Resist, Stability, Spell Hardiness, Combat Training Vs
Giants, Dwarf Racial Adjustment, Shield Dwarf Racial Adjustment, Shield Dwarf
Armor Training, Armor Proficiency Light, Armor Proficiency Medium, Encumbered
Resilience, Dwarven Weapon Proficiency.

### Gold Dwarf
Ultravision, Poison Resist, Stability, Spell Hardiness, Combat Training Vs
Giants, Dwarf Racial Adjustment, Gold Dwarf Racial Adjustment, Gold Dwarf
Toughness, Encumbered Resilience, Dwarven Weapon Proficiency.

### Lightfoot Halfling
Infravision, Shadow Hopper, Lucky, Combat Training Vs Giants, Halfling Racial
Adjustment, Lightfoot Halfling Racial Adjustment, Naturally Stealthy.

### Stout Halfling
Infravision, Shadow Hopper, Lucky, Combat Training Vs Giants, Halfling Racial
Adjustment, Stout Halfling Racial Adjustment, Stout Resilience.

### Rock Gnome
Infravision, Combat Training Vs Giants, Resistance To Illusions, Illusion
Affinity, Tinker Focus, Gnome Racial Adjustment, Gnomish Tinkering, Rock Gnome
Racial Adjustment, Artificers Lore, Tinker.

### Forest Gnome
Infravision, Combat Training Vs Giants, Resistance To Illusions, Illusion
Affinity, Tinker Focus, Gnome Racial Adjustment, Forest Gnome Racial
Adjustment, Speak With Beasts, Natural Illusionist.

### HalfOrc
Ultravision, Half Orc Racial Adjustment, Menacing, Relentless Endurance, Savage
Attacks.

### Shade
Ultravision, One With Shadow, Shadowfell Mind, Practiced Sneak, Shade Racial
Adjustment.

### Goliath
Natural Athlete, Mountain Born, Powerful Build, Stones Endurance, Goliath
Racial Adjustment.

### Goblin
Ultravision, Goblin Racial Adjustment, Nimble Escape, Fast Movement, Stubborn
Mind, Fury Of The Small.

### Hobgoblin
Ultravision, Hobgoblin Racial Adjustment, Stubborn Mind, Authoritative, Fortune
Of The Many.

### HalfTroll (advanced)
Ultravision, Troll Regeneration, Weakness To Fire, Weakness To Acid, Strong
Against Poison, Strong Against Disease, Half Troll Racial Adjustment.

### ArcanaGolem (advanced)
Spellbattle, Spell Vulnerability, Enchantment Vulnerability, Physical
Vulnerability, Magical Heritage, Arcana Golem Racial Adjustment.

### Drow (advanced)
Ultravision, Sleep Enchantment Immunity, Keen Senses, Resistance To
Enchantments, Weapon Proficiency Drow, Drow Racial Adjustment, Drow Spell
Resistance, SLA Faerie Fire, SLA Levitate, SLA Darkness, Light Blindness, Drow
Innate Magic.

### Duergar (advanced)
Ultravision, Light Blindness, Duergar Racial Adjustment, Duergar Magic,
Paralysis Resist, Phantasm Resist, Strong Spell Hardiness, SLA Enlarge, SLA
Strength, SLA Invis, Affinity Spot, Affinity Listen, Affinity Move Silent,
Poison Resist, Stability, Combat Training Vs Giants.

### Half-Ogre (advanced)
Ultravision, Powerful Build, Strong Against Poison, Armor Skin x2 (stacking).
Unarmed attacks: hit, pound, punch, smash.

### Wemic (advanced)
Infravision, Natural Athlete, Powerful Build, Claws And Bite, Survival
Instinct, Hardy, Leonine Frame. Unarmed attacks: bite, claw, trample. Cannot
wear leg or foot equipment.

### Yuan-Ti (advanced)
Ultravision, Poison Bite, Poison Immunity, Stubborn Mind, Armor Skin x2
(stacking). Unarmed attacks: bite, thrash. Cannot wear face, leg, or foot
equipment.

### CrystalDwarf (epic)
Infravision, Crystal Body, Crystal Fist, Vital, Hardy, Crystal Skin, Poison
Resist, Combat Training Vs Giants, Crystal Dwarf Racial Adjustment.

### Trelux (epic)
Ultravision, Vital, Hardy, Vulnerable To Cold, Trelux Exoskeleton, Leap, Wings,
Trelux Eq, Trelux Pincers, Insectbeing. Unarmed attacks: bite, claw, pierce,
stab. Cannot use finger, hands, shield, wield, hold, leg, or foot slots.

### Half-Illithid (epic)
Ultravision, Quick Mind, Stubborn Mind, SLA Levitate, Armor Skin x3 (stacking),
Vital, Hardy. Unarmed attacks: hit, thrash, punch.

### Fae (epic)
Ultravision, Dodge, Fae Racial Adjustment, Fae Magic, Fae Resistance, Fae
Senses, Fae Flight.

### Lich (transformation only)
Unarmed Strike, Improved Unarmed Strike, Armor Skin x5 (stacking), Ultravision,
Vital, Hardy, Lich Racial Adjustment, Lich Spell Resist, Lich Dam Resist, Lich
Touch, Lich Rejuv, Lich Fear, Electric Immunity, Cold Immunity. Unarmed
attacks: hit, thrash, punch, rake, smash.

### Vampire (transformation only)
Alertness, Combat Reflexes, Dodge, Improved Initiative, Lightning Reflexes,
Toughness, Vampire Natural Armor, Vampire Damage Reduction, Vampire Energy
Resistance, Vampire Fast Healing, Vampire Weaknesses, Vampire Blood Drain,
Vampire Children Of The Night, Vampire Create Spawn, Vampire Dominate, Vampire
Energy Drain, Vampire Change Shape, Vampire Gaseous Form, Vampire Spider Climb,
Vampire Skill Bonuses, Vampire Ability Score Boosts, Vampire Bonus Feats,
Vital, Hardy. Unarmed attacks: hit, bite, claw, thrash, punch, rake, smash.

## Balance: race point budgets by tier

This section is a design standard, not a code trace. Nothing in the server
computes these numbers. It exists so that new races land in a consistent
place and existing races can be measured against the same yardstick. The
mechanical facts it rests on (ability modifiers, feat effects, size modifiers)
are traced from `src/character/race.c`, `src/character/feats.c`,
`src/constants.c`, `src/combat/fight.c`, and `src/limits.c`.

### Why a point budget

Stock d20 races are assumed balanced against each other by publication. The
game already has four distinct acquisition classes, and the two upper ones
have no published reference to lean on:

| Tier | Registry marker | How acquired | Existing standard |
|------|-----------------|--------------|-------------------|
| Normal | `IS_NORMAL`, cost 0, LA +0 | Creation | d20 core races: assumed balanced |
| Advanced | `IS_ADVANCE`, cost 1000, LA +2 | Account unlock | d20 races published with a level adjustment (drow, duergar) |
| Epic | `IS_EPIC_R`, cost 30000 or 50000, LA +10 | Account unlock | None |
| Epic quest | `IS_EPIC_R`, cost 999999999, LA +10 | End-game quest transformation | None |

Because `level_adjustment` is not read by any experience or level code, the
tiers are today distinguished only by unlock friction. The point budget below
is the tool that makes "how much stronger is an advanced race allowed to be"
a number instead of a feeling. It is modelled on the Pathfinder Advanced Race
Guide race builder but priced against what this server's feats actually do.

### Race point (RP) pricing table

Score a race as:

    RP = ability points + size points + trait points - drawback refund

**Ability points.** Sum every positive modifier at face value. Sum the
magnitudes of the penalties and subtract them, but credit at most 4 points of
penalty in total. Penalties are cheap to place on a stat the intended build
does not use, so uncapped penalties would let a race buy real power with
imaginary weakness.

**Size points.** Size affects attack, AC, damage and combat maneuvers through
`size_modifiers[]` and `size_modifiers_inverse[]` (Medium is 0). Small gains
+1 attack and AC, loses 2 damage and 1 CMB/CMD. Large is the mirror. Tiny
doubles the Small numbers. These net out close to zero, so Small and Medium
cost 0 and Large and Tiny cost 1 for the trade being more favourable in
practice (reach-free damage for Large, very high AC for Tiny).

**Trait points.** Price each innate feat or special from the table below.
When a trait is not listed, price it by the closest row; when two rows fit,
take the higher.

| Trait class | Example in game | RP |
|-------------|-----------------|----|
| Infravision | Most races | 0.5 |
| Ultravision | Drow, Duergar, HalfOrc | 1 |
| Weapon or armor proficiency group | Elf, Dwarven, Shield Dwarf armor training | 0.5 to 1 |
| +2 to one or two skills | Keen Senses, Shadow Hopper | 0.5 |
| +3 to one skill, or +6 situational | Menacing, Bathed In Moonlight | 0.5 |
| +8 to four skills | Vampire Skill Bonuses | 2 |
| +1 to all saves | Lucky | 1 |
| +2 to a save category | Resistance To Enchantments, Stubborn Mind | 0.5 |
| +4 to a save category | Strong Spell Hardiness, Phantasm Resist | 1 |
| Immunity to one condition | Sleep Enchantment Immunity | 0.5 |
| Immunity to poison | Yuan-Ti Poison Immunity | 1.5 |
| +10 hit points once | Vital | 0.5 |
| +1 hit point per level | Hardy, Gold Dwarf Toughness | 2 |
| +1 natural or dodge AC | Armor Skin (per stack), Dodge | 1 each |
| Scaling AC (+1 per 3 levels) | Trelux Exoskeleton (with its resistances) | 6 |
| +3 flat melee damage | Crystal Fist | 2 |
| Scaling damage (+1 per 4 levels) | Trelux Pincers | 3 |
| Conditional +1 hit / +2 damage | Bloodhunt, Dragonborn Fury, Fury Of The Small | 1 |
| Extra 1d6 on crits | Savage Attacks | 1 |
| Energy resistance 5, one type | Dragonborn Resistance | 0.5 |
| Energy resistance 10, one type | Tiefling Hellish Resistance | 1 |
| Energy resistance 5, four types | Celestial Resistance | 1.5 |
| 50 percent resistance, one type | Mountain Born | 1 |
| Immunity to one energy type | Lich Electric or Cold Immunity | 2 each |
| DR X/- | Lich Dam Resist (DR 4) | 1 per point |
| DR 10 bypassed by a common material | Vampire DR 10/magic+silver | 6 |
| Spell resistance 5 + half level | Half Drow | 2 |
| Spell resistance 10 + level | Drow, Lich | 4 |
| Spell resistance 15 + level | Fae | 6 |
| Regeneration | Troll (3, plus 3 in combat) | 1 per hp per tick |
| Spell-like ability 1/day | Duergar Magic, Lunar Magic | 0.5 each |
| Spell-like ability 3/day | Drow and Duergar SLAs | 1 each |
| One low-circle spell at will | High Elf Cantrip, Natural Illusionist | 2 |
| Battery of strong spells at will | Fae Magic | 8 |
| Flight | Wings, Fae Flight | 4 |
| Bonus feat at level 1 | Quick To Master | 3 |
| Bonus skill points | Skilled | 1.5 |
| Extra ability points at creation | Half Elf | 2 |
| 20 percent chance to avoid any attack | Leap | 5 |
| 25 percent chance to avoid a killing blow | Relentless Endurance | 1 |
| Reroll d20 results under 5 | Fortune Of The Many | 1.5 |
| Party-wide +1 to hit | Authoritative | 1.5 |
| Short self-buff, limited uses | Crystal Body, Stones Endurance, Insectbeing | 1 to 2 |
| Charm or dominate at will | Vampire Dominate | 3 |
| Level drain, gaseous form | Vampire | 2 each |
| Minor utility | Stability, Encumbered Resilience, Tinker, Speak With Beasts, Spider Climb | 0.5 |
| Caster level bonus (level / 6) | Magical Heritage | 2 |

**Drawback refund.** Drawbacks subtract from RP but, like ability penalties,
are capped: total refund may not exceed 25 percent of the tier budget. A race
that is only affordable because of its drawbacks is fragile in play, because
players route around drawbacks and keep the power.

| Drawback | Example | RP |
|----------|---------|----|
| Light Blindness | Drow, Duergar | -2 |
| 50 percent vulnerability, one type | Weakness To Fire | -2 |
| 20 to 25 percent vulnerability, one type | Weakness To Acid, Vulnerable To Cold | -1 |
| -2 AC | Physical Vulnerability | -2 |
| -2 to a save category | Spell or Enchantment Vulnerability | -1 |
| Cannot use an equipment slot | Wemic, Yuan-Ti, Trelux | -1 per slot, cap -6 |
| Environmental damage or disable | Vampire Weaknesses | -4 |

Alignment restrictions and forced class respecs are not priced. They shape
who plays the race; they do not change how strong the race is once played.

### Tier budgets

| Tier | Target RP | Acceptable band | Notes |
|------|-----------|-----------------|-------|
| Normal | 7 | 5 to 9 | Matches the spread of the stock d20 races already in the registry |
| Advanced | 14 | 12 to 16 | Roughly double a normal race, consistent with LA +2 in d20 |
| Epic | 24 | 20 to 28 | Chosen from the current epic median; see calibration below |
| Epic quest | 50 | 40 to 60 | Reward for a level 30 quest line; may exceed epic by about double |

The step from each tier to the next is deliberately about +7 to +10 RP for
Advanced and Epic, and about +25 for Epic quest. Expressed as a formula that
also covers future tiers:

    budget(tier) = 7 + 7 * tier_index          for tier_index 0..2
    budget(quest) = 2 * budget(epic)

with tier_index 0 = Normal, 1 = Advanced, 2 = Epic. Quest races are allowed a
larger jump because they are gated by content difficulty rather than by
account experience, and because their acquisition destroys the character's
prior class build (respec to Wizard or Warrior).

Composition rules, applied inside any tier:

1. Ability points may not exceed 50 percent of the budget. A race is a set of
   traits, not a stat stick. (Wemic and Vampire currently break this.)
2. No single trait may exceed 30 percent of the budget. Above that the race
   is defined by one mechanic and every other choice becomes noise.
3. Drawback refund and penalty credit together may not exceed 25 percent of
   the budget.
4. An advanced or epic race should carry at least one trait that scales with
   level. Flat bonuses that matter at level 5 are irrelevant at level 30 and
   make the race feel worse than a normal race late.
5. Unlock cost within the epic tier follows RP: 30000 at or below target,
   50000 above it. This is the split the registry already uses (Fae is the
   only 50000 race and scores highest).

### Calibration: every current race scored

Scores use the pricing table above with no per-race tuning. Ability is after
the penalty cap, Trait is traits minus drawbacks.

| Race | Tier | Ability | Size | Trait | RP | Versus band |
|------|------|---------|------|-------|----|-------------|
| Human | Normal | 0 | 0 | 4.5 | 4.5 | low |
| Moon Elf | Normal | 3 | 0 | 4.5 | 7.5 | in band |
| Mountain Dwarf | Normal | 4 | 0 | 4.5 | 8.5 | in band |
| Lightfoot Halfling | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Half Elf | Normal | 2 | 0 | 5.5 | 7.5 | in band |
| HalfOrc | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Rock Gnome | Normal | 3 | 0 | 4.0 | 7.0 | in band |
| High Elf | Normal | 3 | 0 | 5.0 | 8.0 | in band |
| Wild Elf | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Half Drow | Normal | 2 | 0 | 4.5 | 6.5 | in band |
| Dragonborn | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Tiefling | Normal | 3 | 0 | 4.0 | 7.0 | in band |
| Stout Halfling | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Forest Gnome | Normal | 3 | 0 | 5.5 | 8.5 | in band |
| Gold Dwarf | Normal | 3 | 0 | 6.0 | 9.0 | top of band |
| Aasimar | Normal | 3 | 0 | 5.0 | 8.0 | in band |
| Tabaxi | Normal | 3 | 0 | 3.0 | 6.0 | in band |
| Goliath | Normal | 3 | 0 | 3.5 | 6.5 | in band |
| Shade | Normal | 3 | 0 | 3.0 | 6.0 | in band |
| Goblin | Normal | 2 | 0 | 3.5 | 5.5 | in band |
| Hobgoblin | Normal | 3 | 0 | 4.5 | 7.5 | in band |
| HalfTroll | Advanced | 4 | 1 | 3.5 | 8.5 | low |
| ArcanaGolem | Advanced | 9 | 0 | -1.0 | 8.0 | low |
| Drow | Advanced | 10 | 0 | 9.0 | 19.0 | high |
| Duergar | Advanced | 6 | 0 | 8.5 | 14.5 | in band |
| Half-Ogre | Advanced | 4 | 1 | 4.5 | 9.5 | low |
| Wemic | Advanced | 12 | 1 | 3.5 | 16.5 | top of band |
| Yuan-Ti | Advanced | 8 | 0 | 3.0 | 11.0 | low |
| CrystalDwarf | Epic | 14 | 0 | 8.5 | 22.5 | in band |
| Trelux | Epic | 12 | 0 | 16.5 | 28.5 | top of band |
| Fae | Epic | 12 | 1 | 28.5 | 41.5 | high |
| Half-Illithid | Epic | 12 | 0 | 8.5 | 20.5 | in band |
| Lich | Epic quest | 18 | 0 | 26.5 | 44.5 | in band |
| Vampire | Epic quest | 22 | 0 | 37.5 | 59.5 | top of band |

What the calibration says:

- **Normal races are consistent.** Twenty of twenty-one fall between 5.5 and
  9.0. This is the empirical basis for the Normal band and supports the
  assumption that stock d20 races can be treated as balanced. Human is the
  one outlier and it is a known d20 property: a bonus feat is worth far more
  in a feat-hungry build than any flat price captures. Leave Human alone.
- **Advanced is the least consistent tier.** Drow at 19 is more than double
  HalfTroll and ArcanaGolem at 8 to 8.5, yet all three cost the same 1000
  account experience. ArcanaGolem is an especially weak buy: its +9 ability
  total is offset by -2 AC and two save penalties, leaving it below several
  normal races. Half-Ogre and Yuan-Ti are also below the band.
- **Epic is bimodal.** CrystalDwarf and Half-Illithid sit at 20 to 23 and are
  essentially "advanced race plus Hardy and more stats". Trelux and Fae are a
  different product: scaling defence, flight, and avoidance. Fae at 41.5 is
  an epic-quest-grade race sold for account experience, and its 50000 price
  reflects that someone already noticed.
- **Epic quest races are the ceiling.** Both land in the 40 to 60 band, with
  Vampire well above Lich. Vampire's DR, fast healing, +6 natural armor, and
  six bonus feats stack multiplicatively with its +22 ability total. Its
  weaknesses (sunlight and running water) are the largest refund in the
  table and still do not bring it near Lich.

### Suggested adjustments for existing races

These are recommendations for a future balance pass, listed in priority
order. None has been applied.

| Race | Direction | Change that reaches the band |
|------|-----------|------------------------------|
| ArcanaGolem | raise to about 14 | Remove Physical Vulnerability (-2 AC), or replace the three vulnerabilities with a single -2 to fortitude saves and add Hardy |
| HalfTroll | raise to about 13 | Add Hardy (+1 hp per level) and either Powerful Build or Strong Against Disease upgraded to disease immunity; the regeneration is already the right kind of scaling trait |
| Half-Ogre | raise to about 13 | Add a third Armor Skin stack and a scaling trait, for example +1 damage per 4 levels with two-handed weapons |
| Yuan-Ti | raise to about 13 | Lift one of the three equipment restrictions (face is the least thematic) and add a 3/day spell-like ability such as charm person |
| Drow | lower to about 16 | Reduce the ability total from +10 to +8 (drop INT to +2), or make the three SLAs 1/day instead of 3/day |
| Fae | move or trim | Either reclassify Fae as an epic quest race, or cut Fae Resistance to DR 5 and SR 10 + level and drop two of the seven at-will spells. That brings it to about 30 |
| CrystalDwarf, Half-Illithid | raise to about 24 | Each needs one scaling trait, for example Crystal Fist growing +1 per 6 levels or a Half-Illithid mind blast on a per-day scale |
| Vampire | monitor | Above target but within band; leave until Lich is compared in live play. If trimmed, Fast Healing 3 rather than 5 is the cleanest single change |

### Designing a new race: procedure

1. Pick the tier. This decides the acquisition path (see "How race tiers and
   unlocking work") and the budget.
2. Write the ability line first and check it against composition rule 1.
3. Add traits from the pricing table until the race is at or slightly below
   target. Every advanced or epic race must include a level-scaling trait
   (rule 4). Where a needed trait does not yet exist as a feat, price it by
   analogy before writing code, so the feat is designed to a budget rather
   than priced after the fact.
4. Add drawbacks only for flavour or to reach the band from above. Check
   rule 3.
5. Record the RP line in the race's design notes and in this table when the
   race ships, and add its `RACE-<NAME>` help entry.
6. Follow [ADDING_NEW_RACE_GUIDE.md](ADDING_NEW_RACE_GUIDE.md) for the
   registry, creation, and persistence work.

Worked example, an advanced race "Shadar-kai" (budget 14, band 12 to 16):
DEX +2, CON +2, WIS +2, CHA -2 gives ability 4 (penalty credited). Traits:
Ultravision 1, Shadowfell Mind 0.5, One With Shadow 1, DR 2/- scaling to
DR 5/- at level 20 (priced as DR 5) 5, shadow step 3/day (short teleport
within the room group, priced as a 3/day SLA) 1, Hardy 2. Total 14.5. The
DR is the scaling trait and is 5 of 14 points, under the 30 percent limit.

### Limits of this model

- RP is additive. Real power is multiplicative: Vampire's DR, fast healing
  and natural armor together are worth more than their sum. Treat scores
  within 2 points of each other as equal and use play data for finer calls.
- Class interaction is not priced. Drow spell resistance is a cost, not a
  benefit, for a Drow cleric buffing allies; Skilled is worth much more to a
  rogue than to a fighter. When a race is clearly built for one class family,
  score it as that class would experience it.
- The pricing table is a starting position, not a law. When play shows a
  trait is under or over priced, change the row here first and rescore the
  affected races, so the table stays the single yardstick.
- The registry's `level_adjustment` values are not enforced by any
  experience code. If a future change makes them live, the Advanced and Epic
  budgets should be revisited, because an actual experience penalty is itself
  a large drawback.

## Registry observations worth knowing

These are facts of the current registry, recorded so readers do not assume
otherwise. None of them is a documented design decision.

- Every playable race allows male and female and disallows neuter.
- The Tiefling registry `name` string is spelled `tielfing`; the display
  `type` is `Tiefling`. Name-based lookups that use the lower-case `name`
  field will only match the misspelling.
- `FEAT_ELF_RACIAL_ADJUSTMENT` is assigned twice to Moon Elf and
  `FEAT_HALF_ORC_RACIAL_ADJUSTMENT` twice to HalfOrc. Both are non-stacking, so
  the duplicate is harmless.
- `initialize_races()` does not set `racial_language`, so races without an
  explicit assignment keep the zero value of the global `race_list[]` array;
  they are shown as "-" above.
