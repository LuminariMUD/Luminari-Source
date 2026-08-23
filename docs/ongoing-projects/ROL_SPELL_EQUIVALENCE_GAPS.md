# RoL Spells Without a Close LuminariMUD Equivalent

Status: source audit completed 2026-08-23.

This list compares every unique spell registered through `SPELL_CREATE()` in
Realms of Luminari with LuminariMUD's registered spells and closely equivalent
powers, skills, and class mechanics. Duplicate RoL registrations are counted
once by spell constant.

A close equivalent must preserve the spell's primary gameplay purpose and
broad effect shape. A merely similar name, damage type, or generic spell in the
same category is not enough.

- RoL registrations reviewed: 332
- Registrations with a close LuminariMUD equivalent: 247
- Registrations without a close equivalent: 85
- Player-facing or functional gaps: 74
- RoL internal or stub registrations without an equivalent: 11

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
| 307 | xxxlich curse | `SPELL_LICH_CURSE` |
| 308 | xxxreconstruction | `SPELL_RECONSTRUCTION` |
| 309 | xxxreanimate flesh | `SPELL_REANIMATE_FLESH` |
| 374 | special proc effect | `SPELL_PROC_SPECIAL` |
| 498 | elemental embodiment maintain | `SPELL_ELEMENTAL_MAINTAIN` |

## Source authority

- RoL spell registration: `RealmsOfLuminari/src/sparser.c`, `SPELL_CREATE()`
- RoL spell IDs: `RealmsOfLuminari/src/spells.h`
- LuminariMUD spell registration: `src/magic/spell_parser.c`, `spello()`
- LuminariMUD spell and power IDs: `src/magic/spells.h`
- Close non-spell equivalents were verified in the relevant LuminariMUD skill,
  feat, bardic-performance, familiar, wildshape, and psionics registrations.
