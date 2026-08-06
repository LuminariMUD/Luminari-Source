# Bard Perk Trees - DDO-Inspired Expertise System

## Overview

Bards blend martial skill, arcane magic, and inspiring performance. These three
perk trees provide clear specialization paths aligned with classic Bard
archetypes seen across tabletop and MMO implementations. Each tree has four
tiers of progression, with capstones at Tier IV.

**The Three Trees:**
1. **Spellsinger** - Spellcasting, song power, control, and support
2. **Warchanter** - Battle anthems, melee presence, and party-wide martial buffs
3. **Swashbuckler** - Agile duelist, finesse weapons, ripostes, and mobility

**Performance Contract:** Bardic performances have no rounds-per-day, daily-use,
or shared-pool cost. Starting or changing a performance spends the action shown
by the `perform` command. A successful performance applies its first verse
immediately, repeats every eleven seconds, and continues until stopped by the
bard, a stutter, an interruption, or an invalid state. Unless a perk says
otherwise, an aura that applies "while performing" affects the performer and
grouped allies in the same room.

**Instrument Contract:** An `ITEM_INSTRUMENT` in the dedicated
`WEAR_INSTRUMENT` slot is preferred; the three legacy hold slots remain
compatibility fallbacks. Every equipped instrument applies its difficulty
reduction. Its effectiveness bonus applies only when its subtype is ideal for
the performance; a non-ideal instrument instead applies -2 effectiveness, and
no instrument applies -3. Breakability is a numerator in 11,111 checked per
verse, with zero meaning unbreakable. The crafting command retains `quality` as
the established name for the stored difficulty-reduction field.

## Base Performance Runtime Contract

The active player engine defines thirteen feat-gated performances. Group
performances affect the performer and grouped characters in the same room.
Offensive performances use normal foe eligibility and engage valid targets.

| Performance | Scope | Successful verse |
|-------------|-------|------------------|
| Song of Healing | Group | Restores hit points immediately and creates no marker affect. |
| Dance of Protection | Group, visual | Grants AC, Will saves, and damage reduction. |
| Song of Focused Mind | Group | Grants Intelligence, Wisdom, and Charisma and accelerates spell preparation while active. |
| Song of Heroism | Group | Grants hit, damage, Strength, Dexterity, and Constitution; grants haste at Bard level 10. |
| Oratory of Rejuvenation | Group | Restores hit points and movement and can remove poison; creates no marker affect. |
| Song of Flight | Group | Grants flight and restores movement. |
| Song of Revelation | Group | Adds detection abilities at Bard levels 1, 5, 10, 15, and 20. |
| Song of Fear | Foes | Will-negated fear and attack penalty; uses fear and mind-affecting immunities. |
| Skit of Forgetfulness | Foes, visual | Will-negated memory clearing for NPCs and disengagement from the performer. |
| Song of Rooting | Foes | Reflex-negated entangle, slow, damage penalty, and AC penalty. |
| Song of Dragons | Group | Grants AC, five save bonuses, Constitution, and maximum hit points. |
| Song of the Magi | Foes | Will-negated penalties to Will, spell resistance, Intelligence, Wisdom, and Charisma. |
| Deafening Song | Foes | Fortitude-negated deafness and AC penalty; respects standard deafness immunity. |

Audible performances reject deaf recipients; Dance of Protection and Skit of
Forgetfulness are visual. Magical healing performances reject constructs and
golems. Persistent effects last two affect rounds by default. Lingering
Performance adds three rounds, and each combined Songweaver rank adds one
round and one point of effectiveness.

Performance effects carry a stable source identifier. Refresh and removal
therefore replace only effects from the same performer, allowing another
bard's lingering effects to survive. Logical effect replacement is batched so
clients receive only the final `AFFECTS` state; see
[Protocol Systems](../PROTOCOL_SYSTEMS.md#outbound-frame-and-backpressure-contract).

The primary and optional Master of Motifs slot are independent. Failure of the
secondary leaves the primary active; failure of the primary promotes a valid
secondary. Invalid command input is resolved before state mutation, linkless
player state is cleared, active NPC state can pulse, and Bard spellcasting ends
all active performances unless Harmonic Casting applies.

---

## TREE 1: SPELLSINGER
*Amplify your magic and songs to control battle flow and empower allies.*

### TIER I - Cost: 1 point each

#### Songweaver I
- **Max Ranks:** 3
- **Description:** Your bard songs gain +1 affect round and +1 effectiveness per rank.
- **Effect Type:** Bardic performance scaling
- **Prerequisites:** None

#### Enchanter's Guile I
- **Max Ranks:** 3
- **Description:** +1 DC to Enchantment and Illusion spells per rank.
- **Effect Type:** Spell DC bonus (school-specific)
- **Prerequisites:** None

#### Resonant Voice I
- **Max Ranks:** 3
- **Description:** Group members affected by your songs gain +1 competence to
  Will saves against mind-affecting effects per rank.
- **Effect Type:** Party defensive support
- **Prerequisites:** None

#### Harmonic Casting
- **Max Ranks:** 1
- **Description:** Casting a bard spell no longer interrupts your active performances.
- **Effect Type:** Performance continuity
- **Prerequisites:** None

---

### TIER II - Cost: 2 points each

#### Songweaver II
- **Max Ranks:** 2
- **Description:** Additional +1 affect round and +1 effectiveness per rank
  (stacks with Songweaver I).
- **Effect Type:** Bardic performance scaling
- **Prerequisites:** Songweaver I (2 ranks)

#### Enchanter's Guile II
- **Max Ranks:** 2
- **Description:** Additional +1 DC to Enchantment and Illusion spells per rank.
- **Effect Type:** Spell DC bonus
- **Prerequisites:** Enchanter's Guile I (2 ranks)

#### Crescendo
- **Max Ranks:** 1
- **Description:** The first Bard spell you cast while performing after starting
  a song gains +2 save DC and deals +1d6 sonic damage once to each target it
  damages.
- **Effect Type:** Opening burst/control
- **Prerequisites:** Harmonic Casting

#### Sustaining Melody
- **Max Ranks:** 1
- **Description:** While performing in combat, each five-second pulse has a 20%
  chance to recover one expended Bard spell slot.
- **Effect Type:** Resource regeneration
- **Prerequisites:** Songweaver I (1 rank)

---

### TIER III - Cost: 3 points each

#### Master of Motifs
- **Max Ranks:** 1
- **Description:** Maintain up to two distinct bard songs simultaneously.
- **Effect Type:** Performance utility
- **Prerequisites:** Sustaining Melody

#### Dirge of Dissonance
- **Max Ranks:** 1
- **Description:** While performing, enemies in the room suffer -2 to
  concentration checks and take 1d6 sonic damage on each eleven-second verse.
- **Effect Type:** Area debuff + attrition damage
- **Prerequisites:** Crescendo

#### Heightened Harmony
- **Max Ranks:** 1
- **Description:** When you spend metamagic on a Bard spell, you gain +5 to your
  Perform skill for one minute.
- **Effect Type:** Metamagic synergy
- **Prerequisites:** Enchanter's Guile II (1 rank)

#### Protective Chorus
- **Max Ranks:** 1
- **Description:** While performing, you and grouped allies in the room gain +2
  to saves vs. spells and +2 AC vs. attacks of opportunity.
- **Effect Type:** Party protection
- **Prerequisites:** Resonant Voice I (2 ranks)

---

### TIER IV - Cost: 5 points each (CAPSTONE)

#### Spellsong Maestra
- **Max Ranks:** 1
- **Description:** While performing, Bard spells gain +2 caster level and +2
  spell DC, and metamagic adds no spell-circle surcharge.
- **Effect Type:** Spellcasting capstone
- **Prerequisites:** Master of Motifs

#### Aria of Stasis
- **Max Ranks:** 1
- **Description:** While performing, you and grouped allies in the room gain +4
  to all saves and immunity to slow; other creatures in the room suffer -2 to
  hit and 10% lower movement speed.
- **Effect Type:** Persistent defensive and control aura
- **Prerequisites:** Protective Chorus

#### Symphonic Resonance
- **Max Ranks:** 1
- **Description:** Each eleven-second verse while performing grants 1d6
  temporary HP, capped at 30 above maximum HP. After a successful Enchantment
  or Illusion Bard spell, valid room enemies that fail a Will save are dazed for
  1 round.
- **Effect Type:** Personal durability and spell-triggered control
- **Prerequisites:** Crescendo

#### Endless Refrain
- **Max Ranks:** 1
- **Description:** On each eleven-second verse while performing, recover one
  expended Bard spell slot.
- **Effect Type:** Spell-slot regeneration
- **Prerequisites:** Sustaining Melody

---

## TREE 2: WARCHANTER
*Rally your allies and dominate the melee with battle anthems and cold-iron resolve.*

### TIER I - Cost: 1 point each

#### Battle Hymn I
- **Max Ranks:** 3
- **Description:** Song of Heroism grants +1 competence to damage per rank to its recipients.
- **Effect Type:** Party damage support
- **Prerequisites:** None

#### Drummer's Rhythm I
- **Max Ranks:** 3
- **Description:** While performing, you gain +1 to hit in melee per rank.
- **Effect Type:** Melee accuracy
- **Prerequisites:** None

#### Rallying Cry
- **Max Ranks:** 1
- **Description:** As a swift action, remove shaken from yourself and grouped
  allies in the room; you and those allies gain +1 to hit, +2 to Will saves,
  and +5 movement speed for 5 rounds.
- **Effect Type:** Condition cleanse and group rally
- **Prerequisites:** None

#### Frostbite Refrain I
- **Max Ranks:** 3
- **Description:** While performing, your melee hits deal +1 cold damage per
  rank; a natural 20 gives the target -1 to attack for 1 round.
- **Effect Type:** Elemental rider + minor debuff
- **Prerequisites:** None

---

### TIER II - Cost: 2 points each

#### Battle Hymn II
- **Max Ranks:** 2
- **Description:** Song of Heroism grants an additional +1 competence damage per
  rank to its recipients (stacks with Battle Hymn I).
- **Effect Type:** Party damage support
- **Prerequisites:** Battle Hymn I (2 ranks)

#### Drummer's Rhythm II
- **Max Ranks:** 2
- **Description:** While performing, gain an additional +1 melee to-hit per rank
  (stacks with Drummer's Rhythm I).
- **Effect Type:** Melee accuracy
- **Prerequisites:** Drummer's Rhythm I (2 ranks)

#### Warbeat
- **Max Ranks:** 1
- **Description:** While performing, make an extra melee attack at your highest
  bonus on your first turn in combat; on hit, you and grouped allies in the room
  gain +1d4 damage for 2 rounds.
- **Effect Type:** Offensive trigger + party buff
- **Prerequisites:** Rallying Cry

#### Frostbite Refrain II
- **Max Ranks:** 2
- **Description:** While performing, melee hits deal an additional +1 cold
  damage per rank; your natural 20 debuff becomes -2 to attack and -1 to AC for
  1 round.
- **Effect Type:** Elemental rider + improved debuff
- **Prerequisites:** Frostbite Refrain I (2 ranks)

---

### TIER III - Cost: 3 points each

#### Anthem of Fortitude
- **Max Ranks:** 1
- **Description:** While performing, you and grouped allies in the room gain +10%
  maximum HP and +2 to Fortitude saves.
- **Effect Type:** Party durability
- **Prerequisites:** Battle Hymn II (1 rank)

#### Commanding Cadence
- **Max Ranks:** 1
- **Description:** While performing, enemies you hit in melee must make a Will
  save or be dazed for 1 round (once per target per 5 rounds).
- **Effect Type:** Soft control on hit
- **Prerequisites:** Warbeat

#### Steel Serenade
- **Max Ranks:** 1
- **Description:** While performing, you gain +2 natural AC and 10% physical damage resistance.
- **Effect Type:** Personal durability
- **Prerequisites:** Drummer's Rhythm II (1 rank)

#### Banner Verse
- **Max Ranks:** 1
- **Description:** While performing, you and grouped allies in the room gain +2
  to hit and +2 to all saves.
- **Effect Type:** Persistent team aura
- **Prerequisites:** Rallying Cry

---

### TIER IV - Cost: 5 points each (CAPSTONE)

#### Warchanter's Dominance
- **Max Ranks:** 1
- **Description:** Song of Heroism now also grants +1 attack and +1 AC; your
  Warbeat gives its recipients an additional +1d4 damage and +1 AC.
- **Effect Type:** Party-wide martial capstone
- **Prerequisites:** Anthem of Fortitude

#### Winter's War March
- **Max Ranks:** 1
- **Description:** On each eleven-second verse while performing, valid enemies
  in the room take 4d6 cold damage and are slowed for 3 rounds; a successful
  Fortitude save halves damage and reduces the slow to 1 round.
- **Effect Type:** Repeating room-wide damage and control
- **Prerequisites:** Commanding Cadence

---

## TREE 3: SWASHBUCKLER
*Dance through combat with finesse, precision, and style.*

### TIER I - Cost: 1 point each

#### Fencer's Footwork I
- **Max Ranks:** 3
- **Description:** +1 Dodge AC and +1 Reflex save per rank while wielding a finesse weapon or single one-handed weapon.
- **Effect Type:** Mobility defense
- **Prerequisites:** None

#### Precise Strike I
- **Max Ranks:** 3
- **Description:** +1 precision damage per rank with finesse or one-handed piercing/slashing weapons (not multiplied on crits).
- **Effect Type:** Precision damage
- **Prerequisites:** None

#### Riposte Training I
- **Max Ranks:** 3
- **Description:** 3% chance per rank to make an immediate counterattack after you successfully dodge or parry.
- **Effect Type:** Reactive extra attack
- **Prerequisites:** None

#### Flourish
- **Max Ranks:** 1
- **Description:** Activate for +2 to hit and +2 AC for 2 rounds; ends if you are knocked prone or grappled. RerquireS and Uses a move action.
- **Effect Type:** Short burst stance
- **Prerequisites:** None

---

### TIER II - Cost: 2 points each

#### Fencer's Footwork II
- **Max Ranks:** 2
- **Description:** Additional +1 Dodge AC and +1 Reflex per rank while using a finesse/single weapon.
- **Effect Type:** Mobility defense
- **Prerequisites:** Fencer's Footwork I (2 ranks)

#### Precise Strike II
- **Max Ranks:** 2
- **Description:** Additional +1 precision damage per rank (stacks with Precise Strike I).
- **Effect Type:** Precision damage
- **Prerequisites:** Precise Strike I (2 ranks)

#### Duelist's Poise
- **Max Ranks:** 1
- **Description:** Gain +2 to critical confirmation and +1 critical threat range when using a finesse weapon.
- **Effect Type:** Critical reliability
- **Prerequisites:** Flourish

#### Agile Disengage
- **Max Ranks:** 1
- **Description:** On a failed flee attempt, you gain +4 AC for 3 rounds. This bonus ends if you move out of the room you're in.
- **Effect Type:** Skirmish utility
- **Prerequisites:** Fencer's Footwork I (1 rank)

---

### TIER III - Cost: 3 points each

#### Perfect Tempo
- **Max Ranks:** 1
- **Description:** If you avoid all melee hits for a full round, your next attack gains +4 to hit and +2d6 precision damage.
- **Effect Type:** Payoff for clean positioning
- **Prerequisites:** Duelist's Poise

#### Showstopper
- **Max Ranks:** 1
- **Description:** On a confirmed crit, impose -2 to enemy AC and -2 to attack rolls for 2 rounds (once per target per 5 rounds).
- **Effect Type:** Debuff on crit
- **Prerequisites:** Precise Strike II (1 rank)

#### Acrobatic Charge
- **Max Ranks:** 1
- **Description:** You can charge through difficult terrain and around allies; you gain +2 to hit on charges.
- **Effect Type:** Mobility + accuracy
- **Prerequisites:** Agile Disengage

#### Feint and Finish
- **Max Ranks:** 1
- **Description:** After successfully feinting, your next attack deals +2d6 precision damage and gains +2 to confirm criticals.
- **Effect Type:** Tactical precision combo
- **Prerequisites:** Riposte Training I (2 ranks)

---

### TIER IV - Cost: 5 points each (CAPSTONE)

#### Swashbuckler's Supreme Style
- **Max Ranks:** 1
- **Description:** While wielding a finesse or single one-handed weapon, you gain +2 to hit, +2 Dodge AC, +2 to critical confirmation, and +1 attack per 3 rounds (does not stack with other extra-attack capstones).
- **Effect Type:** Comprehensive duelist capstone
- **Prerequisites:** Perfect Tempo

#### Curtain Call
- **Max Ranks:** 1
- **Description:** 1/5 minutes, unleash a dazzling flourish: make a free attack against up to three adjacent enemies, each struck takes +2d6 precision damage and must save or be disoriented (disadvantage on attacks) for 2 rounds.
- **Effect Type:** Multi-target finisher
- **Prerequisites:** Showstopper

---

## Notes & Implementation Hints
- Treat "precision damage" as non-multiplied damage that stacks with sneak-like sources where appropriate.
- Performances are free and indefinite after their start action; there is no performance-round pool.
- Song "effective level" can scale existing song formulas (duration, potency, save DC adjustments) without creating new song IDs.
- Control effects (stun/daze/slow/silence) should reuse your standard save/DC pipelines and immunity checks.
- Where abilities are "1/encounter" or "1/long rest," adapt to your engine's timers (cooldowns or daily flags) for parity with other class capstones.
