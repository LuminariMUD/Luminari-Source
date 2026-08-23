# RoL Spells Without a Close LuminariMUD Equivalent

Status: source audit completed 2026-08-23; independently re-verified
2026-08-23 against `sparser.c`, `spells.h`, `spell_parser.c`, and `psionics.c`.

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
- Live registrations with a close LuminariMUD equivalent: 243
- Live registrations without a close equivalent: 84
- Player-facing or functional gaps: 75
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
- Live without a close equivalent: 85 (75 player-facing, 10 stub or internal)

RoL's non-psionic skill registry is audited separately in
`ROL_SKILL_EQUIVALENCE_GAPS.md`, and its playable races in
`ROL_RACE_EQUIVALENCE_GAPS.md`.

This is a feature-equivalence list, not an item-converter mapping list. A RoL
spell omitted here can still require an entry in `_SOURCE_SPELL_MAP` before
scrolls, potions, wands, or staves containing it convert safely.

## Player-facing or functional gaps

| RoL ID | RoL spell | RoL constant |
|-------:|-----------|--------------|
| 20 | minor creation | `SPELL_MINOR_CREATION` |
| 54 | ventriloquate | `SPELL_VENTRILOQUATE` |
| 62 | farsee | `SPELL_FARSEE` |
| 84 | rejuvenate major | `SPELL_REJUVENATE_MAJOR` |
| 88 | rejuvenate minor | `SPELL_REJUVENATE_MINOR` |
| 89 | age | `SPELL_AGE` |
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
| 235 | create spring | `SPELL_CREATE_SPRING` |
| 237 | moonwell | `SPELL_MOONWELL` |
| 297 | nerve dance | `SPELL_NERVE_DANCE` |
| 298 | spectral hand | `SPELL_SPECTRAL_HAND` |
| 299 | rain of blood | `SPELL_RAIN_OF_BLOOD` |
| 301 | embalm | `SPELL_EMBALM` |
| 302 | rot | `SPELL_ROT` |
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
| 475 | phantom heal | `SPELL_PHANTOM_HEAL` |
| 476 | shadechill | `SPELL_SHADECHILL` |
| 478 | agility | `SPELL_AGILITY` |
| 479 | air blast | `SPELL_AIR_BLAST` |
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
3. Exact name matches (95) were accepted as equivalent. The remaining 233 RoL
   names were each resolved by reading the RoL handler and searching
   LuminariMUD for a functional counterpart.
4. Every RoL ID, constant, and name in the tables above was re-checked against
   `RealmsOfLuminari/src/spells.h` and `sparser.c`. All entries verify.

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
| elemental air/earth/fire/water embodiment | wildshape elemental forms (`feats.c`) |
| totem darts, spiritknife, jar the soul, unleash fetish, puppet, spirit wrack | shaman reskins of magic missile, burning hands, shocking grasp, lightning bolt, fireball, and clenched fist; the base spells all exist |
| shillelagh, sticks to snakes | druid reskins of chill touch and magic missile |
| ice layer (`SPELL_SLIPPERY_ICE`) | grease (knocks target prone) |
| ice tongue | silence / power word silence |
| aura of the griffon | mass fly |
| group barkskin | communal stone skin / group shield of faith |
| summon shade, call lycanthrope, control fiend, minor horde | charmed-follower summons; summon creature i-ix / summon swarm |
| doppleganger, massmorph, camouflage | mirror image / mass invisibility / self concealment |
| phantasmal tendrils | black tentacles / greater black tentacles |
| lava burst, dessicate, cyclone, firewave, icewave, conflagration, inferno | area elemental damage; fire storm, ice storm, storm of vengeance |
| ward undead, destroy/annihilate/eradicate undead | disrupt undead / undeath to death / holy javelin |
| locate remains, scry remains | locate object / clairvoyance |
| miracle, full heal, group full heal | heal / group heal / greater planar healing |
| revive, resurrect | resurrection |
| unholy aura | fire shield (RoL sets `AFF_FIRESHIELD`) |
| dark wrath | divine might / destructive aura |
| holy shroud | holy aura / sanctuary |
| changestaff | treant-follower summon; summon creature series |

`agility` is a genuine gap despite `dexterity` being covered: RoL has a separate
Agility stat (`APPLY_AGI`) that LuminariMUD does not model at all.
