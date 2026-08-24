# Damage Message Coverage Audit

Status: findings complete; no combat data or code changed

Audit date: 2026-08-24

Audited revision: `07766564b4405b1c25684a93381ec91a899daccd`

Audited message file SHA-256:
`a5b76a9b7905c489ec01251b0b8f4ba59eebc2648674156e5cba860cf47fbfa3`

## Executive finding

The damage-message catalog is not complete.

The current source exposes 302 distinct runtime damage-message keys. Only 146
of those keys have at least one record in `lib/misc/messages`; 156 keys are
missing. This is 48.3 percent key coverage.

The file currently contains 184 message sets for 164 unique numeric keys. The
extra 20 sets are intentional or legacy variants for duplicate keys. Eighteen
of the 164 keys are not referenced by any current damage path. After those
unreferenced records are removed or reconciled, complete current coverage still
requires 302 unique keys.

`MAX_MESSAGES` is only 200. The current array and loader therefore cannot hold
complete current coverage. Adding all missing records without changing that
limit would terminate boot with "Too many combat messages. Increase
MAX_MESSAGES and recompile."

All 24 ordinary weapon keys, `TYPE_HIT` through `TYPE_GORE`, are present. The
largest gaps are newer spells, poison families, feat-granted abilities,
psionics, warlock powers, weapon special abilities, and environmental damage.

Missing non-weapon records do not make damage silent in every case. The central
`damage()` path falls back to generic "winces in pain" output when no keyed
record exists, and several area/manual abilities emit their own cast message.
They do, however, lack the keyed hit, miss, and death messages that the combat
message system is intended to provide.

## Audit scope and method

The audit used the numeric value passed to the fourth `damage()` argument as
the authoritative lookup key, because that is what `skill_message()` compares
with `fight_messages[].a_type`.

The audited key universe consists of:

1. All 183 top-level cases in the current `mag_damage_scaled()` switch.
2. Every compile-time key passed directly to `damage()`.
3. Compile-time keys passed through `damage_with_projectile()` and
   `spec_damage_current_target()`.
4. All 24 ordinary weapon types, which reach `damage()` through a dynamic
   weapon-type value.
5. Values reached through bounded dynamic tables, including
   `dragon_type_specab_types[]` and breath-weapon dispatch.
6. Conditionally damaging keys, notably every hostile telepathy power whose
   original power ID can be reused by the Mental Backlash damage path.
7. Manual damage effects, skills, psionics, warlock powers, poison types,
   weapon special abilities, and environmental types reached by those paths.

The source was preprocessed before comparison so aliases were compared by
their actual current numeric values. The message file was parsed using its
actual `M`, numeric ID, and 12-message record layout.

Feat IDs are not damage-message keys. No `FEAT_*` constant is passed as the
message key. A feat that changes normal weapon damage inherits the weapon
message. A feat that grants a damaging active ability uses a `SPELL_*`,
`SKILL_*`, `ABILITY_*`, `AFFECT_*`, or other explicit damage key. The tables
below therefore include feat-granted damage under the key that the runtime
actually emits.

Ranged attack-mode values such as `TYPE_MISSILE` are also not message keys.
They select hard-coded ranged presentation while the underlying weapon or
ability value remains the lookup key.

## Runtime and persistence findings

Combat messages are flat-file data, not database data:

- `MESS_FILE` resolves to `lib/misc/messages` in `src/db.h`.
- `load_messages()` in `src/olc/msgedit.c` loads the file into the fixed
  `fight_messages[MAX_MESSAGES]` array during boot.
- `skill_message()` in `src/combat/fight.c` performs the numeric lookup.
- `save_messages_to_disk()` writes MSGEDIT changes back to the same flat file.
- There is no database read or write in MSGEDIT or the runtime lookup path.

The configured development database and all tracked SQL schemas were checked.
There is no combat-message, fight-message, damage-message, spell-definition,
skill-definition, or feat-definition table. Database tables whose names contain
"message" are PubSub communication tables and are unrelated.

### Should this be in the database?

Not as a second independent copy. A simple DB mirror would introduce another
drift surface and would not solve the missing-key or loader-capacity problems.

The immediate correction should keep `lib/misc/messages` authoritative, raise
or replace the fixed 200-key container, and add automated coverage validation.
If production MSGEDIT history, concurrent editing, or cross-environment content
reconciliation is required later, the sound design is a single DB authoring
catalog with a deterministic `lib/misc/messages` projection. The file should
remain the tracked fallback. A DB migration is useful only as that complete
source-of-truth design, not as a duplicate table added in isolation.

## Missing keys

The following 156 current runtime keys have no record in
`lib/misc/messages`.

### Spells: 61 missing

| ID | Runtime key | Representative damage source |
|---:|---|---|
| 267 | `SPELL_CALL_LIGHTNING_STORM` | `src/magic/magic.c:3160` |
| 272 | `SPELL_SPIKE_GROWTH` | `src/movement/movement_events.c:159` |
| 273 | `SPELL_BLIGHT` | `src/magic/magic.c:2655` |
| 276 | `SPELL_SPIKE_STONES` | `src/movement/movement_events.c:142` |
| 295 | `SPELL_SUNBEAM` | `src/magic/magic.c:3539` |
| 298 | `SPELL_FINGER_OF_DEATH` | `src/magic/magic.c:2748` |
| 300 | `SPELL_GENERIC_AOE` | `src/combat/act.offensive.c:6169` |
| 382 | `SPELL_ESHIELD_DAM` | `src/combat/fight.c:14232` |
| 387 | `SPELL_ACID_BREATHE` | `src/magic/magic.c:3337` |
| 388 | `SPELL_POISON_BREATHE` | `src/combat/act.offensive.c:15060` |
| 393 | `SPELL_CIRCLE_OF_DEATH` | `src/magic/magic.c:3592` |
| 394 | `SPELL_UNDEATH_TO_DEATH` | `src/magic/magic.c:3602` |
| 395 | `SPELL_GRASP_OF_THE_DEAD` | `src/magic/magic.c:3612` |
| 398 | `SPELL_LESSER_MISSILE_STORM` | `src/magic/magic.c:2997` |
| 406 | `SPELL_HEDGING_WEAPONS` | `src/combat/act.offensive.c:13307` |
| 416 | `SPELL_LIFE_SHIELD` | `src/combat/fight.c:6412` |
| 435 | `SPELL_SEARING_LIGHT` | `src/magic/magic.c:2804` |
| 441 | `SPELL_GAS_BREATHE` | `src/magic/magic.c:3311` |
| 443 | `SPELL_MOONBEAM` | `src/magic/magic.c:2552` |
| 444 | `SPELL_HELLISH_REBUKE` | `src/magic/magic.c:2687` |
| 450 | `SPELL_CORROSIVE_TOUCH` | `src/magic/magic.c:2678` |
| 509 | `SPELL_FLAME_ARROW` | `src/magic/magic.c:3071` |
| 511 | `SPELL_FIRE_BOLT` | `src/magic/magic.c:2629` |
| 512 | `SPELL_JOLT` | `src/magic/magic.c:2592` |
| 513 | `SPELL_DISRUPT_UNDEAD` | `src/magic/magic.c:2608` |
| 525 | `SPELL_SPLINTER_STORM` | `src/magic/magic.c:3478` |
| 526 | `SPELL_SHOCKWAVE` | `src/magic/magic.c:3492` |
| 527 | `SPELL_POISON_BREATH` | `src/magic/magic.c:3426` |
| 538 | `SPELL_SANDBLAST` | `src/magic/magic.c:1800` |
| 539 | `SPELL_FELL_FROST` | `src/magic/magic.c:1808` |
| 546 | `SPELL_NERVE_DANCE` | `src/magic/magic.c:1816` |
| 547 | `SPELL_SPECTRAL_HAND` | `src/magic/magic.c:1830` |
| 548 | `SPELL_RAIN_OF_BLOOD` | `src/magic/magic.c:1838` |
| 549 | `SPELL_ROT` | `src/magic/magic.c:1846` |
| 550 | `SPELL_ICE_TOMB` | `src/magic/magic.c:1856` |
| 551 | `SPELL_CONSTRICTION` | `src/magic/magic.c:1864` |
| 554 | `SPELL_SANDSTORM` | `src/magic/magic.c:1872` |
| 555 | `SPELL_BLACKLIGHT_BURST` | `src/magic/magic.c:1880` |
| 556 | `SPELL_MINUTE_METEORS` | `src/magic/magic.c:1890` |
| 558 | `SPELL_THUNDER_LANCE` | `src/magic/magic.c:1898` |
| 559 | `SPELL_SHADOW_BOLT` | `src/magic/magic.c:1906` |
| 560 | `SPELL_SHADOW_BURST` | `src/magic/magic.c:1914` |
| 564 | `SPELL_SHADOW_MAGIC` | `src/magic/magic.c:1922` |
| 565 | `SPELL_PHANTASMAL_BLADES` | `src/magic/magic.c:1930` |
| 568 | `SPELL_NEEDLE_SWARM` | `src/magic/magic.c:1938` |
| 569 | `SPELL_SNAPPING_TEETH` | `src/magic/magic.c:1946` |
| 570 | `SPELL_BELTYNS_BURNING_BLOOD` | `src/magic/magic.c:1954` |
| 572 | `SPELL_EARTHBLOOD` | `src/magic/magic.c:1968` |
| 573 | `SPELL_SOUL_TEMPEST` | `src/magic/magic.c:1981` |
| 577 | `SPELL_DUST_DEVIL` | `src/magic/magic.c:2024` |
| 578 | `SPELL_SUFFOCATE` | `src/magic/magic.c:2032` |
| 580 | `SPELL_ROCK_TO_MUD` | `src/magic/spells.c:5464` |
| 583 | `SPELL_BLACKTHORNS` | `src/magic/magic.c:2040` |
| 587 | `SPELL_SHADECHILL` | `src/magic/magic.c:2048` |
| 589 | `SPELL_AIR_BLAST` | `src/magic/magic.c:2056` |
| 593 | `SPELL_POLTERGEIST` | `src/magic/magic.c:2064` |
| 603 | `SPELL_CYCLONE` | `src/magic/magic.c:1989` |
| 604 | `SPELL_LICH_TOUCH` | `src/magic/magic.c:1997` |
| 605 | `SPELL_LAVA_BURST` | `src/magic/magic.c:2006` |
| 606 | `SPELL_ICE_LAYER` | `src/magic/spells.c:5746` |
| 608 | `SPELL_TAZRIKS_FRENZIED_HOUND` | `src/magic/magic.c:2014` |

### Abilities, effects, and poisons: 34 missing

| ID | Runtime key | Representative damage source |
|---:|---|---|
| 1205 | `BLACKGUARD_TOUCH_OF_CORRUPTION` | `src/combat/act.offensive.c:12866` |
| 1207 | `ABILITY_CHANNEL_POSITIVE_ENERGY` | `src/magic/magic.c:2826` |
| 1208 | `ABILITY_CHANNEL_NEGATIVE_ENERGY` | `src/magic/magic.c:2860` |
| 1209 | `WEAPON_POISON_BLACK_ADDER_VENOM` | `src/magic/magic.c:1729` |
| 1214 | `RACIAL_LICH_TOUCH` | `src/combat/act.offensive.c:12610` |
| 1264 | `ABILITY_DEATHLESS_TOUCH` | `src/magic/magic.c:2760` |
| 1280 | `SPELL_AFFECT_CREEPING_DOOM_BITE` | `src/magic/magic.c:2903` |
| 1282 | `MOB_ABILITY_CORRUPTION` | `src/magic/magic.c:1788` |
| 1283 | `ABILITY_BOZAK_DRACONIAN_DEATH_THROES` | `src/magic/magic.c:2772` |
| 1284 | `AFFECT_FALLING_DAMAGE` | `src/movement/movement_validation.c:256` |
| 1318 | `AFFECT_BARD_FROSTBITE_REFRAIN_I` | `src/combat/fight.c:14354` |
| 1322 | `AFFECT_BARD_WINTERS_WAR_MARCH` | `src/bardic_performance.c:1681` |
| 1470 | `POISON_TYPE_SCORPION_WEAK` | `src/magic/magic.c:1739` |
| 1471 | `POISON_TYPE_SCORPION_NORMAL` | `src/magic/magic.c:1756` |
| 1472 | `POISON_TYPE_SCORPION_STRONG` | `src/magic/magic.c:1772` |
| 1473 | `POISON_TYPE_SNAKE_WEAK` | `src/magic/magic.c:1740` |
| 1474 | `POISON_TYPE_SNAKE_NORMAL` | `src/magic/magic.c:1757` |
| 1475 | `POISON_TYPE_SNAKE_STRONG` | `src/magic/magic.c:1773` |
| 1476 | `POISON_TYPE_SPIDER_WEAK` | `src/magic/magic.c:1741` |
| 1477 | `POISON_TYPE_SPIDER_NORMAL` | `src/magic/magic.c:1758` |
| 1478 | `POISON_TYPE_SPIDER_STRONG` | `src/magic/magic.c:1774` |
| 1479 | `POISON_TYPE_CENTIPEDE_WEAK` | `src/magic/magic.c:1742` |
| 1480 | `POISON_TYPE_CENTIPEDE_NORMAL` | `src/magic/magic.c:1759` |
| 1481 | `POISON_TYPE_CENTIPEDE_STRONG` | `src/magic/magic.c:1775` |
| 1482 | `POISON_TYPE_WASP_WEAK` | `src/magic/magic.c:1743` |
| 1483 | `POISON_TYPE_WASP_NORMAL` | `src/magic/magic.c:1760` |
| 1484 | `POISON_TYPE_WASP_STRONG` | `src/magic/magic.c:1776` |
| 1485 | `POISON_TYPE_FUNGAL_WEAK` | `src/magic/magic.c:1744` |
| 1486 | `POISON_TYPE_FUNGAL_NORMAL` | `src/magic/magic.c:1761` |
| 1487 | `POISON_TYPE_FUNGAL_STRONG` | `src/magic/magic.c:1777` |
| 1491 | `POISON_TYPE_WYVERN` | `src/magic/magic.c:1762` |
| 1492 | `POISON_TYPE_PURPLE_WORM` | `src/magic/magic.c:1778` |
| 1493 | `POISON_TYPE_COCKATRICE` | `src/magic/magic.c:1745` |
| 1494 | `POISON_TYPE_KAPAK` | `src/magic/magic.c:1746` |

### Psionic and warlock damage: 29 missing

| ID | Runtime key | Representative damage source |
|---:|---|---|
| 1505 | `PSIONIC_DEMORALIZE` | `src/magic/magic.c:1171` |
| 1516 | `PSIONIC_SLUMBER` | `src/magic/magic.c:1171` |
| 1529 | `PSIONIC_INFLICT_PAIN` | `src/magic/magic.c:1171` |
| 1530 | `PSIONIC_MENTAL_DISRUPTION` | `src/magic/magic.c:1171` |
| 1543 | `PSIONIC_ENERGY_RETORT` | `src/combat/fight.c:14283` |
| 1548 | `PSIONIC_PSIONIC_BLAST` | `src/magic/magic.c:1171` |
| 1553 | `PSIONIC_DEATH_URGE` | `src/magic/magic.c:1171` |
| 1554 | `PSIONIC_EMPATHIC_FEEDBACK` | `src/combat/fight.c:14253` |
| 1556 | `PSIONIC_INCITE_PASSION` | `src/magic/magic.c:1171` |
| 1558 | `PSIONIC_MOMENT_OF_TERROR` | `src/magic/magic.c:1171` |
| 1559 | `PSIONIC_POWER_LEECH` | `src/magic/magic.c:1171` |
| 1570 | `PSIONIC_SHATTER_MIND_BLANK` | `src/magic/magic.c:1171` |
| 1574 | `PSIONIC_BREATH_OF_THE_BLACK_DRAGON` | `src/magic/magic.c:2448` |
| 1575 | `PSIONIC_BRUTALIZE_WOUNDS` | `src/magic/magic.c:1171` |
| 1576 | `PSIONIC_DISINTEGRATION` | `src/magic/magic.c:2459` |
| 1581 | `PSIONIC_ENERGY_CONVERSION` | `src/magic/psionics.c:561` |
| 1585 | `PSIONIC_PSYCHOSIS` | `src/magic/magic.c:1171` |
| 1586 | `PSIONIC_ULTRABLAST` | `src/magic/magic.c:2481` |
| 1592 | `PSIONIC_ASSIMILATE` | `src/magic/magic.c:2504` |
| 1650 | `WARLOCK_ELDRITCH_SPEAR` | `src/magic/magic.c:2079` |
| 1662 | `WARLOCK_ELDRITCH_CHAIN` | `src/magic/magic.c:2076` |
| 1671 | `WARLOCK_VORACIOUS_DISPELLING` | `src/magic/spells.c:585` |
| 1673 | `WARLOCK_ELDRITCH_CONE` | `src/magic/magic.c:2078` |
| 1677 | `WARLOCK_VITRIOLIC_BLAST` | `src/limits.c:751` |
| 1678 | `WARLOCK_CHILLING_TENTACLES` | `src/limits.c:232` |
| 1680 | `WARLOCK_TENACIOUS_PLAGUE` | `src/magic/magic.c:2100` |
| 1681 | `WARLOCK_WALL_OF_PERILOUS_FLAME` | `src/magic/magic.c:3127` |
| 1686 | `WARLOCK_RETRIBUTIVE_INVISIBILITY` | `src/magic/magic.c:2110` |
| 1949 | `WARLOCK_CRITICAL_ELDRITCH_BLAST` | `src/magic/magic.c:2080` |

The 12 effect-oriented telepathy powers in this table are conditionally
damaging. On a successful save, the Mental Backlash perk at
`src/magic/magic.c:1166-1172` calls `damage()` with the original hostile
telepathy power ID. They therefore need damage-message coverage even though
their primary registered routine is `MAG_AFFECTS` or `MAG_MASSES` rather than
`MAG_DAMAGE`.

### Skills and feat-granted active damage: 14 missing

| ID | Runtime key | Representative damage source |
|---:|---|---|
| 2143 | `SKILL_BOMB_TOSS` | `src/craft/alchemy.c:1281` |
| 2151 | `SKILL_DRAGON_BITE` | `src/combat/act.offensive.c:10272` |
| 2152 | `SKILL_SLAM` | `src/combat/act.offensive.c:14600` |
| 2153 | `SKILL_GORE` | `src/combat/act.offensive.c:5997` |
| 2154 | `SKILL_BITE` | `src/combat/act.offensive.c:6035` |
| 2203 | `SKILL_WATER_WHIP` | `src/combat/fight.c:13156` |
| 2211 | `SKILL_FLAMES_OF_PHOENIX` | `src/combat/act.offensive.c:616` |
| 2212 | `SKILL_WAVE_OF_ROLLING_EARTH` | `src/combat/act.offensive.c:689` |
| 2214 | `SKILL_FIST_OF_FOUR_THUNDERS` | `src/combat/act.offensive.c:901` |
| 2216 | `SKILL_BREATH_OF_WINTER` | `src/combat/act.offensive.c:1030` |
| 2224 | `SKILL_EARTHSHAKER` | `src/combat/act.offensive.c:4434` |
| 2232 | `SKILL_RADIANT_AURA` | `src/combat/act.offensive.c:10215` |
| 2238 | `WARLOCK_CHILLING_TENTACLES_COLD` | `src/limits.c:234` |
| 2242 | `SKILL_GARROTE` | `src/rol_feats.c:611` |

### Weapon special abilities and environmental damage: 18 missing

| ID | Runtime key | Representative damage source |
|---:|---|---|
| 3000 | `TYPE_SPECAB_FLAMING` | `src/character/evolutions.c:872` |
| 3001 | `TYPE_SPECAB_FLAMING_BURST` | `src/combat/spec_abilities.c:1250` |
| 3002 | `TYPE_SPECAB_FROST` | `src/character/evolutions.c:876` |
| 3004 | `TYPE_SPECAB_CORROSIVE` | `src/character/evolutions.c:874` |
| 3005 | `TYPE_SPECAB_HOLY` | `src/character/evolutions.c:889` |
| 3007 | `TYPE_SPECAB_THUNDERING` | `src/character/evolutions.c:880` |
| 3008 | `TYPE_SPECAB_BLEEDING` | `src/combat/spec_abilities.c:1352` |
| 3009 | `TYPE_SPECAB_SHOCK` | `src/character/evolutions.c:878` |
| 3010 | `TYPE_SPECAB_SHOCKING_BURST` | `src/combat/spec_abilities.c:1811` |
| 3011 | `TYPE_SPECAB_ANARCHIC` | `src/combat/spec_abilities.c:535` |
| 3012 | `TYPE_SPECAB_UNHOLY` | `src/character/evolutions.c:891` |
| 3013 | `TYPE_SPECAB_POISON` | `src/constants.c:3193` |
| 3390 | `TYPE_ON_FIRE` | `src/limits.c:2902` |
| 3391 | `TYPE_LAVA_DAMAGE` | `src/limits.c:329` |
| 3392 | `TYPE_DROWNING` | `src/limits.c:336` |
| 3393 | `TYPE_MOVING_WATER` | `src/limits.c:2919` |
| 3394 | `TYPE_SUN_DAMAGE` | `src/limits.c:2915` |
| 3399 | `TYPE_SUFFERING` | `src/limits.c:2938` |

## Existing coverage

The 146 covered active keys consist of:

- 80 spell keys;
- 3 ability/effect keys;
- 20 psionic or warlock keys;
- 19 damaging skill keys; and
- all 24 ordinary weapon attack keys.

All ordinary weapon keys from 2300 through 2323 are present:
`TYPE_HIT`, `TYPE_STING`, `TYPE_WHIP`, `TYPE_SLASH`, `TYPE_BITE`,
`TYPE_BLUDGEON`, `TYPE_CRUSH`, `TYPE_POUND`, `TYPE_CLAW`, `TYPE_MAUL`,
`TYPE_THRASH`, `TYPE_PIERCE`, `TYPE_BLAST`, `TYPE_PUNCH`, `TYPE_STAB`,
`TYPE_SLICE`, `TYPE_THRUST`, `TYPE_HACK`, `TYPE_RAKE`, `TYPE_PECK`,
`TYPE_SMASH`, `TYPE_TRAMPLE`, `TYPE_CHARGE`, and `TYPE_GORE`.

The covered non-weapon IDs are:

```text
5 6 8 10 22 23 25 26 27 30 32 33 37 46 60 61 64 65 66 67 68 69 70
71 72 73 74 76 81 82 85 96 101 106 113 129 130 142 143 144 151 154
158 159 160 161 162 174 181 184 188 191 198 206 207 209 212 217 219
253 264 265 268 271 275 280 282 283 293 304 308 310 422 423 426 427
459 464 465 481 1203 1228 1239 1503 1507 1512 1523 1527 1528 1534
1536 1539 1542 1552 1568 1571 1573 1589 1597 1599 1600 1648 1682
2001 2002 2004 2120 2121 2122 2123 2124 2125 2126 2127 2200 2204
2205 2206 2207 2208 2209 2210
```

## Stale and unreferenced records

Eighteen current file keys are not selected by any audited current damage path:

```text
307 1649 1930 2006 2031 2070 2103 2119 2201 2202
2391 2392 2393 2394 2396 2397 2398 2399
```

The 2391-2399 records reveal a concrete numeric migration defect. Their text
identifies lava, drowning, moving water, sunlight, elemental shields, and
suffering, but the current environmental type range is 3390-3399. For example,
the file has lava under 2391 while runtime sends `TYPE_LAVA_DAMAGE` as 3391.
These records can never match their current runtime keys.

The shield records at 2396-2398 overlap newer spell-specific shield keys.
Current cold, fire, and acid shield damage uses 143, 142, and 144, which are
already present. Electric shield damage uses 382 and is missing. The stale
2396-2398 content should be reconciled with the active spell-specific records,
not blindly renumbered.

The remaining unreferenced skill records are mostly legacy control or attack
messages whose skills now modify a normal weapon attack, apply an affect, or
perform a maneuver without using that skill ID as the `damage()` key. Keeping
them is harmless to lookup but consumes fixed-array slots.

## Present but incomplete records

`#` becomes a null message pointer. For non-weapon damage, `skill_message()`
uses the selected record and does not then try another record or the generic
missing-key fallback. A present key can therefore still suppress a hit, miss,
or death audience message.

Fifteen active non-weapon keys have at least one incomplete selected record:

| ID | Key | Incomplete section |
|---:|---|---|
| 280 | `SPELL_INSECT_PLAGUE` | hit |
| 283 | `SPELL_WALL_OF_THORNS` | hit |
| 293 | `SPELL_FIRE_STORM` | miss and hit |
| 304 | `SPELL_BLADES` | death, miss, and hit |
| 481 | `SPELL_HOSTILE_JUXTAPOSITION` | miss |
| 1239 | `AFFECT_CAUSTIC_BLOOD_DAMAGE` | miss |
| 1539 | `PSIONIC_CONCUSSIVE_ONSLAUGHT` | miss |
| 2200 | `SKILL_BLEEDING_ATTACK` | miss |
| 2204 | `SKILL_GONG_OF_SUMMIT` | miss |
| 2205 | `SKILL_FIST_OF_UNBROKEN_AIR` | miss |
| 2206 | `SKILL_FLOWING_RIVER` | miss |
| 2207 | `SKILL_SWEEPING_CINDER_STRIKE` | miss |
| 2208 | `SKILL_RUSH_OF_GALE_SPIRITS` | miss in both variants |
| 2209 | `SKILL_CLENCH_OF_NORTH_WIND` | miss |
| 2210 | `SKILL_SWARMING_ICE_RABBIT` | miss |

Some missing miss sections are currently masked by a custom attack-roll failure
message before `damage()` is called. They are still structurally incomplete and
would become visible if call behavior changes. IDs 280, 283, 293, and 304 have
incomplete successful-hit output and are the highest priority among existing
records.

Weapon records intentionally omit their ordinary successful-hit section.
Successful weapon hits use hard-coded severity text in `dam_message()`; their
file records are used for melee misses and death blows. Those intentional weapon
nulls are not counted in the 15 non-weapon incomplete records above.

## Damage paths that cannot use the file

The keyed coverage count excludes calls that deliberately pass no key. There
are 71 direct `damage()` call sites using literal `-1` and 28 using
`TYPE_UNDEFINED`, which also expands to -1. Those calls always use the generic
pain fallback or caller-authored output; no `lib/misc/messages` record can match
them.

There are also live paths that subtract `GET_HIT()` directly and never enter
`damage()` at all. Important examples include:

- DG script damage in `src/dgscript/dg_misc.c`;
- Blackguard Cataclysmic Smite in `src/character/perks.c`;
- Bard Curtain Call in `src/combat/act.offensive.c`;
- Brilliance and Blunder invention explosions in `src/act.other.c`;
- multiple legacy object and zone procedures in `src/spec/`;
- several legacy death bursts and environmental procedures in `src/spec/`;
  and
- explicit hit-point costs and temporary-hit-point decay, which are resource
  mechanics rather than attacks.

Most true-damage bypasses print custom text, but they have no stable damage key,
do not participate in this catalog, and can bypass parts of normal combat/death
handling. Cataclysmic Smite and Curtain Call are especially notable because
they are current player abilities, not merely legacy world procedures.

## Recommended correction order

1. Raise or replace `MAX_MESSAGES` before adding coverage. A value of 512 gives
   enough headroom; 302 is the current minimum after obsolete keys are removed.
2. Add an automated parser/coverage test that fails when a reachable named
   damage key has no message record, when an active non-weapon record lacks a
   required audience message, or when the file exceeds loader capacity.
3. Correct the stale 2391-2399 environmental records. Add current 3390-3394 and
   3399 keys, and reconcile the obsolete shield records with 142-144 and the
   missing electric-shield key 382.
4. Fill the 156 missing records in bounded groups: current player abilities
   first, then common spells and effects, then poison/NPC/environmental keys.
5. Repair the incomplete successful-hit records for 280, 283, 293, and 304.
6. Convert true combat harm that directly edits `GET_HIT()` to `damage()` with
   a stable key. Keep explicit no-message bypasses only for resource costs or
   mechanics that document why normal combat handling must not run.
7. Do not add a duplicate DB table. Revisit DB-backed authoring only as a full
   authoritative-catalog plus deterministic-file-projection project.

## Verification criteria for a future fix

A complete correction should prove all of the following:

- every reachable named damage key has at least one valid record;
- every non-weapon record has safe hit, miss, and death behavior for all
  reachable outcomes;
- all ordinary weapon keys retain their hard-coded successful-hit fallback and
  valid file-backed miss/death behavior;
- the unique-key count remains below the configured container capacity;
- obsolete numeric IDs do not conceal current-ID gaps;
- current player abilities no longer subtract hit points outside the normal
  damage/death pipeline without an explicit reviewed reason;
- MSGEDIT round-trips the complete file without loss; and
- server boot loads the final catalog without warnings or truncation.
