# RoL Spells Without a Close LuminariMUD Equivalent

Status: implementation incomplete. The source registry audit was completed and
independently re-verified 2026-08-24 against `sparser.c`, `spells.h`,
`spell_parser.c`, and `psionics.c`. Five implementation checkpoints added 83
functional gaps through native magic paths and focused direct handlers. A
follow-up handler-level review found 14 live RoL spells that had been
incorrectly classified as covered by loose substitutes. Six of those spells
still require native implementations.

This list compares every unique spell registered through `SPELL_CREATE()` in
Realms of Luminari with LuminariMUD's registered spells and closely equivalent
powers, skills, and class mechanics. Duplicate RoL registrations are counted
once by spell constant.

RoL's psionic powers are NOT in the `SPELL_CREATE()` registry; they are
registered through `SKILL_CREATE()` with the psionic discipline skill types and
are covered separately below.

A close equivalent must preserve the spell's primary gameplay purpose and
broad effect shape. A merely similar name, damage type, or generic spell in the
same category is not enough.

Five `SPELL_CREATE()` calls and twenty `SKILL_CREATE()` calls in `sparser.c`
sit inside `#if 0` blocks and are never compiled. They are counted separately
below and must not be treated as live RoL content.

### `SPELL_CREATE()` registry

- RoL registrations reviewed: 332 (327 live, 5 never compiled)
- Live registrations with a close LuminariMUD equivalent: 229
- Live registrations without a close equivalent: 98
- Player-facing or functional gaps: 89 (83 implemented, 6 remaining)
- RoL internal or stub registrations without an equivalent: 9
- Registered only inside `#if 0` (never compiled): 5

### Psionic `SKILL_CREATE()` registry

- RoL psionic registrations reviewed: 40 (33 live, 7 never compiled)
- Live registrations with a close LuminariMUD equivalent: 32
- Player-facing or functional gaps: 0
- Live registrations with an empty handler in RoL: 1
- Registered only inside `#if 0` (never compiled): 7

### Combined

- Total RoL spell and psionic registrations reviewed: 372
- Live: 360. Never compiled: 12.
- Live without a close equivalent: 99 (89 player-facing, 10 stub or internal)

RoL's non-psionic skill registry was audited separately and all identified
player-facing gaps were implemented. Playable race gaps are tracked in
`ROL_RACE_EQUIVALENCE_GAPS.md`.

This remains a feature-equivalence list. The corresponding magic-item
converter map is numerically complete but not semantically complete.
`_SOURCE_SPELL_MAP` contains 340 positive source IDs: all 327 live
`SPELL_CREATE()` IDs, all 335 distinct positive `SPELL_*` numeric IDs in the
source header, and the two non-spell IDs actually found in magic-item spell
slots. Six live source spells currently map to substitutes that do not
preserve their mechanics. Those mappings must move to the new native spell IDs
after the remaining spells are implemented.

The active corpus check converts all 511 magic items: 71 potions, 251 scrolls,
80 wands, and 109 staves. Those records contain 117 distinct positive source
spell references across 734 populated spell slots. Every positive reference
emits its mapped positive target ID in the same spell slot. A zero already
present in an unused source slot remains an empty slot; the converter never
creates zero as a fallback for a positive source spell. A future unknown
positive ID is a conversion error, so registry growth cannot silently disable
an item.

Numerical conversion success therefore does not establish spell-equivalence
or gameplay fidelity. Internal, disabled, and helper-only source IDs also must
not be counted as covered merely because the map assigns them an unrelated
positive target spell.

## Remaining player-facing or functional gaps

These six live RoL spells require distinct native registrations and mechanics.
The current converter targets are listed only to identify the inadequate
substitutes; they are not acceptable final mappings.

| RoL ID | RoL spell | Current substitute | Required source behavior |
|-------:|-----------|--------------------|--------------------------|
| 343 | call lycanthrope | summon creature vi | Summons one randomly selected lycanthrope prototype, permits only one such follower, scales it to at most level 40 from caster level minus 10, assigns scaled hit points, charms it, and schedules charm expiration. Summon creature VI creates a dire tiger. |
| 376 | tazriks frenzied hound | faithful hound | Opens a temporary vortex and makes a hellhound strike one randomly selected eligible room target once per combat pulse for three strikes. It does not create a persistent follower. |
| 482 | elemental water embodiment | geniekind | Transforms an eligible allied PC for a base duration of ten, modified by specialization; grants roughly 5 hit points per shared level, water breathing, fire/gas/acid protection, and 25 percent additional height and weight. |
| 483 | elemental fire embodiment | geniekind | Transforms an eligible allied PC for a base duration of ten, modified by specialization; grants roughly 7 hit points per shared level, fire shield, -65 source armor class, haste, flight, gas/fire protection, and 35 percent additional height and weight. |
| 484 | elemental earth embodiment | geniekind | Transforms an eligible allied PC for a base duration of ten, modified by specialization; grants gas/cold protection and 50 percent additional height and weight. The live RoL handler grants roughly 7 hit points per shared level because it uses `EFHP_FACTOR`; the separately declared `EEHP_FACTOR` value of 10 is unused and should be resolved deliberately during implementation. |
| 485 | elemental air embodiment | geniekind | Transforms an eligible allied PC for a base duration of ten, modified by specialization; grants roughly 3 hit points per shared level, -50 source armor class, haste, flight, gas/acid protection, and 15 percent additional height and weight. |

All four elemental embodiment spells share additional behavior that must remain
common in the target implementation: the target must be a same-side PC, the
target must be unmounted and not already embodying an element, and the caster
can maintain only one embodiment. Each spell creates a linked caster-side
maintenance affect so expiration and removal can clean up the transformation.
Geniekind is not a usable base for this behavior: it is self-only, selects a
genie type, applies different traits, and summons a genie follower.

## Implemented player-facing or functional gaps

### Implementation checkpoint 1: foundational and defensive spells

These spells are registered as distinct spells but deliberately have no class
or domain assignment. They can be used by scripted content, magic items, and
staff without changing any player class spell list.

| Spell | Implemented gameplay purpose |
|-------|------------------------------|
| farsee | Timed far vision; `scan` range increases from three to six rooms. |
| rejuvenate major | Permanently removes 1d3 years from a consenting group member. |
| rejuvenate minor | Temporarily lowers displayed age. |
| age | Permanently adds 2d8 years to a consenting group member. |
| command undead | Charms a lower-level undead NPC as the caster's follower. |
| command horde | Attempts command undead against eligible undead in the room. |
| slow poison | Halves poison intensity for its duration. |
| comprehend languages | Understands spoken languages without granting speech. |
| fumble | Reduces the target's base Dexterity toward 1 on a failed save. |
| stumble | Penalizes armor class, Reflex saves, initiative, and coordination. |
| enervate | Reduces the target's base Constitution toward 1 on a failed save. |
| protect undead | Gives an undead target armor and Will-save wards. |
| protection from undead | Wards defenses and reduces undead-source damage by 25 percent. |
| ancestral shield | Gives the caster's room group 25 percent area-spell mitigation. |
| protection from animals | Wards defenses and reduces animal-source damage by 25 percent. |
| pass without trace | Prevents tracking and greatly improves silent passage. |
| greater realm of protection | Grants broad fire, cold, air, earth, acid, and electricity resistance. |
| feign death | Ends combat involving the target and conceals the target as apparently dead. |
| tranquility | Ends eligible fights in the room and temporarily pacifies those affected. |
| agility | Improves armor class, Reflex saves, and initiative. |
| natures blessing | Improves attacks and saves and reduces area-spell damage by 25 percent. |
| song of travel | Restores group movement, grants flight, and increases travel speed. |

### Implementation checkpoint 2: offensive and control spells

These 29 spells use the existing damage, affect, projectile-loop, and area
magic routines. Poltergeist uses a direct spell handler solely for its random
room-target selection. None has a class or domain assignment.

| Spell | Implemented gameplay purpose |
|-------|------------------------------|
| sandblast | Deals earth damage and can blind, then silence, an already blinded target. |
| fell frost | Deals heavy cold damage, slows targets, and can freeze an already slowed target. |
| nerve dance | Deals negative-energy damage to living, corporeal, non-elemental targets. |
| spectral hand | Strikes one target with negative energy. |
| rain of blood | Deals unholy damage to eligible enemies throughout the room. |
| rot | Deals negative-energy area damage only to living, corporeal targets. |
| ice tomb | Deals heavy cold damage and can paralyze a badly wounded target. |
| constriction | Deals force damage, interrupts casting, and briefly silences the target. |
| sandstorm | Deals area earth damage and can blind and stagger its victims. |
| blacklight burst | Deals area unholy damage and slows victims; undead take reduced damage. |
| minute meteors | Launches up to five small fire projectiles at one target. |
| thunder lance | Deals electrical damage and shatters lesser globes and energy wards. |
| shadow bolt | Launches up to five illusion-damage projectiles at one target. |
| shadow burst | Deals illusion damage to eligible enemies throughout the room. |
| shadow magic | Deals single-target illusion damage. |
| phantasmal blades | Deals slashing illusion damage throughout the room. |
| needle swarm | Deals puncture damage to one target. |
| snapping teeth | Deals slashing damage to one target. |
| beltyns burning blood | Ignites a wounded living target's blood and leaves the target burning. |
| blackmantle | Suppresses normal regeneration and reduces healing. |
| earthblood | Deals earth damage and can briefly solidify a living target in place. |
| soul tempest | Deals force damage to eligible enemies throughout the room. |
| dust devil | Deals air damage and can tear a droppable weapon from the target's grasp. |
| suffocate | Deals air damage and briefly silences the target. |
| blackthorns | Deals puncture damage to one target. |
| shadechill | Deals cold illusion damage to one target. |
| air blast | Deals air damage to one target. |
| shadow flux | Temporarily strips a large amount of spell resistance. |
| poltergeist | Hurls three force strikes at randomly selected eligible room targets. |

### Implementation checkpoint 3: creation, travel, terrain, and utility spells

These final 24 spells use existing object, affect, group, room-affect, combat,
and movement systems. Direct handlers are limited to behavior that cannot be
expressed by one native routine. None has a class or domain assignment.

| Spell | Implemented gameplay purpose |
|-------|------------------------------|
| minor creation | Creates one selected mundane, no-rent, no-sell object without relying on a prototype VNUM. |
| ventriloquate | Throws supplied speech toward a nearby character or object; observers can save to detect the illusion. |
| preserve | Extends a corpse's decay timer, with a larger extension for player corpses. |
| wraithform | Ends combat and temporarily makes the target immaterial without discarding equipment. |
| create spring | Creates a temporary outdoor fountain filled with water. |
| moonwell | Creates temporary paired portals between valid material-plane player locations. |
| embalm | Greatly extends a corpse's decay timer. |
| airy water | Temporarily makes an underwater room breathable. |
| blink | Disengages a supported group tank or moves the target to a random valid adjacent room. |
| unseen servant | Temporarily increases the caster's carrying capacity. |
| mislead | Conceals the caster from pursuit and prevents tracking. |
| sequester | Temporarily prevents teleportation and summoning magic from targeting the subject. |
| dimension shift | Ends combat and temporarily phases the room group beyond ordinary material reach. |
| soul bind | On a failed save, anchors the target and blocks teleportation. |
| death pact | Lets room-group members remain standing below normal death thresholds, down to -120 hit points. |
| spirit walk | Moves the caster to a consenting group member's player corpse. |
| rock to mud | Damages earth elementals or turns the room into temporary difficult terrain. |
| mud to rock | Heals allied earth elementals or removes rock to mud from the room. |
| phantom heal | Grants temporary vitality up to half health, then removes that vitality when the illusion expires. |
| curse item | Makes an inventory item undroppable and weakens a weapon's damage die. |
| corpse glamor | Reduces a corpse's weight to one. |
| sun shadow | Temporarily darkens the room. |
| earth fog | Temporarily fills the room with obscuring earthen fog. |
| fire fog | Temporarily illuminates the room with fiery fog. |

### Implementation checkpoint 4: undead healing, divine wards, and concealment

These four spells preserve source-specific identities and lifecycle rules that
their former substitutes could not represent. None has a class or domain
assignment.

| Spell | Implemented gameplay purpose |
|-------|------------------------------|
| heal undead | Heals only undead for `4d(level)`, respects blackmantle, and restores 100 hit points in the PC-lich-to-PC-lich case. |
| dark wrath | While out of combat, grants the source level-scaled damage and all-spell-save bonuses for the source duration bands. |
| unholy aura | Applies its own duration-scaled affect identity while supplying the fire-shield state. |
| camouflage | Ends all combat involving the unmounted caster and applies persistent hide under a distinct spell affect until concealment breaks. |

### Implementation checkpoint 5: environmental damage and direct control

These four spells replace loose damage or control substitutes with their traced
source-specific rules. None has a class or domain assignment.

| Spell | Implemented gameplay purpose |
|-------|------------------------------|
| cyclone | Deals room-wide air damage and halves a PC caster's damage when the latest cached zone wind speed is 25 or lower. A missing cache safely uses the low-wind fallback. |
| lich touch | Deals unsaved unholy touch damage, applies the source fire-elemental and shield adjustments, and separately saves against accumulating Strength loss and slow; dragons ignore the secondary effects. |
| lava burst | Deals room-wide fire damage and ignites successfully damaged survivors for five rounds. RoL calls its merge hook, but its live merge table has no lava-burst pairing, so there is no additional merge behavior to reproduce. |
| ice layer | Uses Reflex as the target system's Agility-save counterpart, deals `2d10` bludgeoning damage on failure, knocks the target to the sitting/prone state, and applies one combat pulse of lag while preserving all source immunities. |

The converter now preserves RoL beholder identity with a dedicated converted
mobile flag so ice layer can distinguish beholders from generic aberrations.

The complete inventory of the 83 implemented gaps remains below, preserving
the original source-to-target accounting. The six remaining gaps are listed in
the preceding section.

| RoL ID | RoL spell | RoL constant |
|-------:|-----------|--------------|
| 20 | minor creation | `SPELL_MINOR_CREATION` |
| 54 | ventriloquate | `SPELL_VENTRILOQUATE` |
| 62 | farsee | `SPELL_FARSEE` |
| 84 | rejuvenate major | `SPELL_REJUVENATE_MAJOR` |
| 88 | rejuvenate minor | `SPELL_REJUVENATE_MINOR` |
| 89 | age | `SPELL_AGE` |
| 90 | cyclone | `SPELL_CYCLONE` |
| 119 | preserve | `SPELL_PRESERVE` |
| 154 | command undead | `SPELL_COMMAND_UNDEAD` |
| 163 | slow poison | `SPELL_SLOW_POISON` |
| 171 | comprehend languages | `SPELL_COMPREHEND_LANGUAGES` |
| 174 | fumble | `SPELL_FUMBLE` |
| 175 | stumble | `SPELL_STUMBLE` |
| 176 | enervate | `SPELL_ENERVATE` |
| 181 | sandblast | `SPELL_SANDBLAST` |
| 182 | fell frost | `SPELL_FELL_FROST` |
| 228 | wraithform | `SPELL_WRAITHFORM` |
| 230 | protect undead | `SPELL_PROT_UNDEAD` |
| 231 | protection from undead | `SPELL_PROT_FROM_UNDEAD` |
| 232 | command horde | `SPELL_COMMAND_HORDE` |
| 233 | heal undead | `SPELL_HEAL_UNDEAD` |
| 235 | create spring | `SPELL_CREATE_SPRING` |
| 237 | moonwell | `SPELL_MOONWELL` |
| 297 | nerve dance | `SPELL_NERVE_DANCE` |
| 298 | spectral hand | `SPELL_SPECTRAL_HAND` |
| 299 | rain of blood | `SPELL_RAIN_OF_BLOOD` |
| 301 | embalm | `SPELL_EMBALM` |
| 302 | rot | `SPELL_ROT` |
| 303 | lich touch | `SPELL_LICH_TOUCH` |
| 305 | ice tomb | `SPELL_ICE_TOMB` |
| 319 | constriction | `SPELL_CONSTRICTION` |
| 321 | airy water | `SPELL_AIRY_WATER` |
| 322 | blink | `SPELL_BLINK` |
| 329 | sandstorm | `SPELL_SANDSTORM` |
| 332 | blacklight burst | `SPELL_BLACKLIGHT_BURST` |
| 334 | minute meteors | `SPELL_MINUTE_METEORS` |
| 341 | unseen servant | `SPELL_UNSEEN_SERVANT` |
| 349 | thunder lance | `SPELL_THUNDER_LANCE` |
| 350 | shadow bolt | `SPELL_SHADOW_BOLT` |
| 351 | shadow burst | `SPELL_SHADOW_BURST` |
| 353 | mislead | `SPELL_MISLEAD` |
| 354 | sequester | `SPELL_SEQUESTER` |
| 359 | dimension shift | `SPELL_DIMENSION_SHIFT` |
| 362 | shadow magic | `SPELL_SHADOW_MAGIC` |
| 366 | phantasmal blades | `SPELL_PHANTASMAL_BLADES` |
| 370 | soul bind | `SPELL_SOUL_BIND` |
| 371 | death pact | `SPELL_DEATH_PACT` |
| 377 | dark wrath | `SPELL_DARK_WRATH` |
| 378 | unholy aura | `SPELL_UNHOLY_AURA` |
| 380 | needle swarm | `SPELL_NEEDLE_SWARM` |
| 381 | snapping teeth | `SPELL_SNAPPING_TEETH` |
| 392 | beltyns burning blood | `SPELL_BELTYNS_BURNING_BLOOD` |
| 397 | blackmantle | `SPELL_BLACKMANTLE` |
| 426 | earthblood | `SPELL_EARTHBLOOD` |
| 435 | soul tempest | `SPELL_SOUL_TEMPEST` |
| 437 | spirit walk | `SPELL_SPIRIT_WALK` |
| 438 | ancestral shield | `SPELL_ANCESTRAL_SHIELD` |
| 442 | protection from animals | `SPELL_PROTECTION_FROM_ANIMALS` |
| 445 | dust devil | `SPELL_DUST_DEVIL` |
| 447 | suffocate | `SPELL_SUFFOCATE` |
| 450 | pass without trace | `SPELL_PASS_WITHOUT_TRACE` |
| 452 | rock to mud | `SPELL_ROCK_TO_MUD` |
| 453 | mud to rock | `SPELL_MUD_TO_ROCK` |
| 459 | greater realm of protection | `SPELL_GREATER_REALM_OF_PROTECTION` |
| 467 | blackthorns | `SPELL_BLACKTHORNS` |
| 470 | feign death | `SPELL_FEIGN_DEATH` |
| 471 | tranquility | `SPELL_TRANQUILITY` |
| 473 | camouflage | `SPELL_CAMOUFLAGE` |
| 475 | phantom heal | `SPELL_PHANTOM_HEAL` |
| 476 | shadechill | `SPELL_SHADECHILL` |
| 478 | agility | `SPELL_AGILITY` |
| 479 | air blast | `SPELL_AIR_BLAST` |
| 487 | lava burst | `SPELL_LAVA_BURST` |
| 488 | ice layer | `SPELL_ICE_LAYER` |
| 492 | shadow flux | `SPELL_SHADOW_FLUX` |
| 505 | natures blessing | `SPELL_NATURES_BLESSING` |
| 514 | song of travel | `BARD_TRAVEL` |
| 515 | poltergeist | `SPELL_POLTERGEIST` |
| 518 | curse item | `SPELL_CURSE_OBJ` |
| 520 | corpse glamor | `SPELL_CORPSE_GLAMOR` |
| 522 | sun shadow | `SPELL_SUN_SHADOW` |
| 524 | earth fog | `SPELL_EARTH_FOG` |
| 525 | fire fog | `SPELL_FIRE_FOG` |

## RoL internal or stub registrations

These are part of the complete RoL spell registry, but their source handlers
are internal maintenance effects or explicit stubs rather than ordinary
player-cast spells.

| RoL ID | RoL registration | RoL constant |
|-------:|------------------|--------------|
| 64 | xxxrecharger | `SPELL_RECHARGER` |
| 93 | xxxvitalize mana | `SPELL_VITALIZE_MANA` |
| 291 | elemental embodiment maintain | `SPELL_CASTER_EARTH_EMBODIMENT` |
| 292 | elemental embodiment maintain | `SPELL_CASTER_WATER_EMBODIMENT` |
| 293 | elemental embodiment maintain | `SPELL_CASTER_FIRE_EMBODIMENT` |
| 294 | elemental embodiment maintain | `SPELL_CASTER_AIR_EMBODIMENT` |
| 361 | simulacrum | `SPELL_SIMULACRUM` |
| 374 | special proc effect | `SPELL_PROC_SPECIAL` |
| 498 | elemental embodiment maintain | `SPELL_ELEMENTAL_MAINTAIN` |

`xxxrecharger` and `xxxvitalize mana` have lower-level function bodies in the
RoL source, but their live registrations explicitly dispatch to
`cast_spell_stub`, so they are disabled rather than missing playable spells.
IDs 291-294 and 498 are caster-side embodiment maintenance records, and ID 374
is an affect marker used by special-proc code. They must not become geniekind,
arcane mark, restoration, enchant item, or any other castable spell. If one is
encountered in a castable world-data spell slot, conversion should fail with a
specific non-castable-source-ID diagnostic.

`SPELL_HEAL_LICH` (source ID 314) is not registered through `SPELL_CREATE()`.
It is a helper invoked only by `cast_heal_undead()` for the PC-lich-to-PC-lich
case. Its behavior belongs inside the new heal-undead implementation; it does
not require a separate target spell and must not map to negative energy ray.

## Spell registrations never compiled

These five `SPELL_CREATE()` calls exist only inside `#if 0` blocks in
`sparser.c`. `SPELL_ROT` and `SPELL_MAGE_FLAME` also appear inside `#if 0` but
have a live registration elsewhere, so they are not listed here.

| RoL ID | RoL registration | RoL constant |
|-------:|------------------|--------------|
| 307 | xxxlich curse | `SPELL_LICH_CURSE` |
| 308 | xxxreconstruction | `SPELL_RECONSTRUCTION` |
| 309 | xxxreanimate flesh | `SPELL_REANIMATE_FLESH` |
| 526 | aura of the griffon | `SPELL_GRIFFON_AURA` |
| n/a | color spray | `SPELL_COLOR_SPRAY` (no `#define` exists) |

## RoL psionic powers

RoL's psionicist powers are registered with `SKILL_CREATE()` in `sparser.c`
under the disciplines `SKILL_TYPE_CLAIRSENTIENCE`, `SKILL_TYPE_PSYCHOKINESIS`,
`SKILL_TYPE_PSYCHOMETABOLISM`, `SKILL_TYPE_PSYCHOPORTATION`,
`SKILL_TYPE_TELEPATHY`, and `SKILL_TYPE_METAPSIONICS` (40 registrations), and
implemented in `RealmsOfLuminari/src/psionic.c`. LuminariMUD's counterparts are
registered with `psiono()` in `src/magic/psionics.c` (97 powers).

Note: RoL's `SKILL_*` and `SPELL_*` constants share and overlap the same
numeric range in `spells.h` (for example `SPELL_CASTER_WATER_EMBODIMENT` and
`SKILL_ENHANCE_AGI` are both 292), so the IDs below are only meaningful within
the skill registry and must not be cross-referenced against the spell IDs
above.

### Player-facing or functional gaps

None. All 33 live RoL psionic powers resolve to a LuminariMUD `psiono()` power
or spell.

### Live registration with an empty handler in RoL

| RoL ID | RoL registration | RoL constant | Handler |
|-------:|------------------|--------------|---------|
| 282 | attraction | `SKILL_ATTRACTION` | `do_attraction()` is an empty body |

`SKILL_ATTRACTION` is marked `/* Unused */` in `spells.h`.

### Psionic registrations never compiled

These seven sit inside `#if 0` blocks in `sparser.c`. `enhance agility` would
otherwise have been a gap, for the same reason as the `agility` spell:
LuminariMUD does not model an Agility stat. It is not a gap because RoL never
compiles it.

| RoL ID | RoL registration | RoL constant |
|-------:|------------------|--------------|
| 274 | alter aura | `SKILL_ALTER_AURA` |
| 276 | enhance skill | `SKILL_ENHANCE_SKILL` |
| 291 | enhance dexterity | `SKILL_ENHANCE_DEX` |
| 292 | enhance agility | `SKILL_ENHANCE_AGI` |
| 293 | enhance constitution | `SKILL_ENHANCE_CON` |
| 294 | enhance vision | `SKILL_ENHANCE_VISION` |
| 295 | enhance stamina | `SKILL_ENHANCE_STAMINA` |

### Psionic near-miss cases ruled covered

| RoL power | LuminariMUD equivalent |
|-----------|------------------------|
| catfall, tower of iron will, ultrablast | same-name `psiono()` powers |
| mindblast | psionic blast / mind thrust |
| combatmind | offensive prescience / inevitable strike |
| vipermind | energy retort / empathetic feedback |
| danger sense | defensive precognition / detect hostile intent |
| aurasight | detect alignment |
| project force | energy push / concussion blast |
| detonate | energy burst / shrapnel burst |
| adrenalize | endorphin surge / fortify |
| body control, equalibrium | body adjustment / body equilibrium |
| flesh armor, scale skin | inertial armor / oak body |
| reduction, expansion | reduce person / enlarge person |
| deathfield | energy wave / recall death |
| sustain | spends moves to satisfy hunger and thirst; create food / create water |
| planar rift, shift | planar travel / psychoport |
| dominate, mass domination | dominate person / mass domination |
| synaptic static | mental disruption / psionic lock |
| globe of darkness | darkness |
| canibalize | energy conversion / power leech |
| battle trance | rage / psionic vigor |
| stasis field | deceleration / slow |
| globe (`SKILL_METAGLOBE`), interference shield | force screen / epic psionic ward |
| charge | recharges a psionic crystal; recharge |
| enhance strength | strength / mass strength |


## Source authority

- RoL spell registration: `RealmsOfLuminari/src/sparser.c`, `SPELL_CREATE()`
- RoL psionic registration: `RealmsOfLuminari/src/sparser.c`, `SKILL_CREATE()`
  with a psionic discipline skill type; behavior in
  `RealmsOfLuminari/src/psionic.c`
- RoL spell and skill IDs: `RealmsOfLuminari/src/spells.h`
- LuminariMUD spell registration: `src/magic/spell_parser.c`, `spello()`
- LuminariMUD spell and power IDs: `src/magic/spells.h`
- Close non-spell equivalents were verified in the relevant LuminariMUD skill,
  feat, bardic-performance, familiar, wildshape, and psionics registrations.

## Verification method

The audit was reproduced mechanically before this revision:

1. Every `SPELL_CREATE()` call in `RealmsOfLuminari/src/sparser.c` was extracted
   (340 calls, 332 unique spell constants, 328 unique registration names).
2. Every registration name in LuminariMUD's `spello()`, `skillo()`, and
   `psiono()` tables was extracted (831 unique names across
   `src/magic/spell_parser.c` and `src/magic/psionics.c`).
3. Exact name matches (95) were accepted provisionally as equivalent. The
   remaining 233 RoL names were each resolved by reading the RoL handler and
   searching LuminariMUD for a functional counterpart.
4. Every RoL ID, constant, and name in the tables above was re-checked against
   `RealmsOfLuminari/src/spells.h` and `sparser.c`. All entries verify.
5. A follow-up comparison of source and target handlers rejected 14 earlier
   equivalence decisions. Similar names, elements, summon categories, or affect
   flags were not accepted when targeting, timing, secondary effects, linked
   state, or spell identity differed materially.

`psiono()` registrations must be included in step 2. Omitting them produces at
least one false gap (`SPELL_WITHER`, covered by `PSIONIC_WITHER`).

Steps 1-4 were then repeated for RoL's psionic `SKILL_CREATE()` registry, which
`SPELL_CREATE()` extraction does not reach. Scanning only `SPELL_CREATE()`
misses all 40 psionic registrations.

Extraction must be preprocessor-aware and comment-aware. A plain `grep` for
`SPELL_CREATE(` / `SKILL_CREATE(` over `sparser.c` reports 25 registrations
that are inside `#if 0` blocks and one (`SKILL_FEIGN_DEATH`) that is `//`
commented out.

## Near-miss cases ruled covered

These RoL spells have no same-name LuminariMUD registration but were confirmed
to have a close equivalent, and are deliberately not listed as gaps:

| RoL spell | LuminariMUD equivalent |
|-----------|------------------------|
| disintegrate | `PSIONIC_DISINTEGRATION` |
| wither | `PSIONIC_WITHER` |
| dexterity | grace / mass grace |
| vitality | false life / mass false life |
| greater thought | cunning / mass cunning |
| beautify | charisma / mass charisma |
| totem darts, spiritknife, jar the soul, unleash fetish, puppet, spirit wrack | shaman reskins of magic missile, burning hands, shocking grasp, lightning bolt, fireball, and clenched fist; the base spells all exist |
| shillelagh, sticks to snakes | druid reskins of chill touch and magic missile |
| ice tongue | silence / power word silence |
| aura of the griffon | mass fly |
| group barkskin | communal stone skin / group shield of faith |
| summon shade, control fiend, minor horde | charmed-follower summons; summon creature i-ix / summon swarm |
| doppleganger, massmorph | mirror image / mass invisibility |
| phantasmal tendrils | black tentacles / greater black tentacles |
| dessicate, firewave, icewave, conflagration, inferno | area elemental damage; fire storm, ice storm, storm of vengeance |
| ward undead, destroy/annihilate/eradicate undead | disrupt undead / undeath to death / holy javelin |
| locate remains, scry remains | locate object / clairvoyance |
| miracle, full heal, group full heal | heal / group heal / greater planar healing |
| revive, resurrect | resurrection |
| holy shroud | holy aura / sanctuary |
| changestaff | treant-follower summon; summon creature series |

`agility` is a genuine gap despite `dexterity` being covered: RoL has a separate
Agility stat (`APPLY_AGI`) that LuminariMUD does not model at all.
