# Player Races Reference

Status: source-backed reference, verified 2026-09-10 against `src/character/race.c`,
`src/account.c`, `src/structs.h`, `src/quest/quest.c`, `src/quest/hlquest.c`, and
`src/spec/spec_rol_conversion.c`.

This document is the reference center for playable (player-character) races. It
lists every race a player can hold, its ability modifiers, size, alignment
limits, innate feats, unlock cost, and how it is acquired. Non-player race
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
