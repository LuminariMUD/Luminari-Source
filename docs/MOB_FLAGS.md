# Mobile (MOB) Flags Documentation

This document provides comprehensive information about all mobile flags (MOB_*) used in LuminariMUD. Mobile flags control NPC behaviors, abilities, restrictions, and special mechanics throughout the game world.

## Table of Contents
- [Overview](#overview)
- [Basic Behavior Flags](#basic-behavior-flags)
- [Aggression & Alignment Flags](#aggression--alignment-flags)
- [Immunity & Restriction Flags](#immunity--restriction-flags)
- [Movement & Blocking Flags](#movement--blocking-flags)
- [Combat Helper Flags](#combat-helper-flags)
- [Companion & Summoned Creature Flags](#companion--summoned-creature-flags)
- [Special Ability Flags](#special-ability-flags)
- [System & Internal Flags](#system--internal-flags)
- [Complete Flag Reference](#complete-flag-reference)

---

## Overview

Mobile flags are bitflags defined in `src/structs.h` (lines 1131-1232) and are checked throughout the codebase using the `MOB_FLAGGED()` macro. There are currently **101 mobile flags** (indices 0-100) that control everything from basic AI behavior to special monster abilities.

**Usage Pattern:**
```c
if (MOB_FLAGGED(mob, MOB_FLAGNAME)) {
    // Flag is set, apply behavior/restriction
}
```

---

## Basic Behavior Flags

### MOB_SPEC (Index: 0)
**Effect:** Indicates the mobile has a special procedure (spec-proc) attached.
- Special procedures provide custom scripted behaviors
- Executed during mobile activity updates
- Used for quest NPCs, shopkeepers, trainers, etc.

**Code References:** `spec_procs.c`, `spec_assign.c`

### MOB_SENTINEL (Index: 1)
**Effect:** Mobile will not move from its current location.
- Prevents wandering and random movement
- Mobile stays in its spawn location
- Can still be dragged/summoned by magic
- Used for stationary guards, shopkeepers, and landmark NPCs

**Code References:**
- `mob_act.c:397` - Prevents random movement
- `mob_act.c:419, 431` - Position management for sentinels
- `utils.c:7171` - Drag restrictions

### MOB_SCAVENGER (Index: 2)
**Effect:** Mobile picks up items from the ground.
- Randomly collects objects in the same room
- 1 in 10 chance per update cycle
- Used for goblin looters, pack rats, etc.

**Code References:** `mob_act.c:162`

### MOB_AWARE (Index: 4)
**Effect:** Mobile cannot be backstabbed or surprise attacked.
- Prevents backstab attacks from rogues
- Blocks sneak attacks and similar abilities
- Used for alert guards, paranoid NPCs, creatures with special senses

**Code References:**
- `act.offensive.c:4076` - Backstab prevention
- `act.offensive.c:8940` - Sneak attack prevention

### MOB_WIMPY (Index: 7)
**Effect:** Mobile flees when severely injured.
- Automatically flees when HP drops below threshold
- Used for cowardly creatures and civilians
- Behavior similar to player wimpy setting

**Code References:**
- `fight.c:5779` - Flee trigger check
- `mob_act.c:196` - Wimpy behavior handling

### MOB_SENTIENT (Index: 19)
**Effect:** Marks mobile as sentient/intelligent.
- Used for intelligent creatures and humanoids
- May affect certain spell interactions
- Thematic flag for creature classification

### MOB_LISTEN (Index: 34)
**Effect:** Mobile enters room if it hears fighting nearby.
- Responds to combat sounds in adjacent rooms
- Used for reinforcement mechanics
- Guards and aggressive creatures use this

### MOB_LIT (Index: 35)
**Effect:** Mobile emits light.
- Acts as a light source in dark rooms
- Creatures made of fire, glowing undead, etc.
- Affects visibility calculations

---

## Aggression & Alignment Flags

### MOB_AGGRESSIVE (Index: 5)
**Effect:** Mobile automatically attacks all nearby characters.
- Attacks everyone who enters the room
- Conflicts with MOB_HELPER flag
- Used for hostile monsters and aggressive creatures

**Code References:**
- `mob_act.c:221` - Aggression trigger
- `limits.c:2245` - Set during rage/charm effects
- `db.c:3173-3177` - Boot-time validation with alignment aggro flags

### MOB_AGGR_EVIL (Index: 8)
**Effect:** Mobile attacks evil-aligned characters on sight.
- Only attacks players/NPCs with evil alignment
- Requires MOB_AGGRESSIVE to also be set
- Used for paladins, good-aligned guards

### MOB_AGGR_GOOD (Index: 9)
**Effect:** Mobile attacks good-aligned characters on sight.
- Only attacks players/NPCs with good alignment
- Requires MOB_AGGRESSIVE to also be set
- Used for demons, evil priests, undead

### MOB_AGGR_NEUTRAL (Index: 10)
**Effect:** Mobile attacks neutral-aligned characters on sight.
- Only attacks players/NPCs with neutral alignment
- Requires MOB_AGGRESSIVE to also be set
- Used for extremist NPCs

---

## Immunity & Restriction Flags

### MOB_NOCHARM (Index: 13)
**Effect:** Mobile cannot be charmed or mind-controlled.
- Blocks charm person, dominate, and similar spells
- Used for strong-willed creatures, undead, constructs
- Essential for boss monsters

**Code References:**
- `spells.c:388` - Charm spell blocking
- `spec_abilities.c:611, 622, 640, 1359` - Ability-based charm blocking

### MOB_NOSUMMON (Index: 14)
**Effect:** Mobile cannot be summoned or teleported.
- Prevents summon monster spells from bringing this mob
- Blocks gate and similar effects
- Used for bosses and unique creatures

### MOB_NOSLEEP (Index: 15)
**Effect:** Mobile cannot be put to sleep.
- Blocks sleep spells and effects
- Used for elves, undead, constructs
- Essential for creatures that don't sleep

### MOB_NOBASH (Index: 16)
**Effect:** Mobile cannot be bashed or knocked down.
- Prevents shield bash and similar attacks
- Used for trees, huge creatures, incorporeal beings
- Affects tactical combat options

### MOB_NOBLIND (Index: 17)
**Effect:** Mobile cannot be blinded.
- Blocks blindness spells and effects
- Used for creatures with special vision (tremorsense, blindsight)
- Oozes, creatures without eyes

### MOB_NOKILL (Index: 18)
**Effect:** Mobile cannot be attacked by players.
- Complete immunity to player attacks
- Used for quest NPCs, essential storyline characters
- Can still be damaged by environmental effects

### MOB_NODEAF (Index: 22)
**Effect:** Mobile cannot be deafened.
- Blocks deafness effects
- Used for creatures without hearing or magical entities

### MOB_NOGRAPPLE (Index: 25)
**Effect:** Mobile cannot be grappled or grabbed.
- Prevents grapple combat maneuvers
- Used for incorporeal creatures, very large/small creatures
- Affects melee combat tactics

### MOB_NOCONFUSE (Index: 62)
**Effect:** Mobile cannot be confused.
- Blocks confusion spells and effects
- Used for mindless creatures, high-intelligence beings

### MOB_NOPARALYZE (Index: 97)
**Effect:** Mobile cannot be paralyzed.
- Blocks hold person, paralysis, and similar effects
- Used for creatures immune to paralysis
- Essential for certain creature types

### MOB_NOSTEAL (Index: 37)
**Effect:** Mobile cannot be stolen from.
- Prevents pickpocket attempts
- Used for important NPCs, alert guards
- Protects quest items

---

## Movement & Blocking Flags

### MOB_STAY_ZONE (Index: 6)
**Effect:** Mobile will not wander outside its home zone.
- Restricts random movement to current zone
- Prevents mobs from wandering into neighboring zones
- Used for zone-specific creatures

### MOB_BLOCK_N through MOB_BLOCK_D (Index: 45-54)
**Effect:** Mobile physically blocks movement in specific directions.
- **MOB_BLOCK_N** (45) - Blocks north
- **MOB_BLOCK_E** (46) - Blocks east
- **MOB_BLOCK_S** (47) - Blocks south
- **MOB_BLOCK_W** (48) - Blocks west
- **MOB_BLOCK_NE** (49) - Blocks northeast
- **MOB_BLOCK_SE** (50) - Blocks southeast
- **MOB_BLOCK_SW** (51) - Blocks southwest
- **MOB_BLOCK_NW** (52) - Blocks northwest
- **MOB_BLOCK_U** (53) - Blocks up
- **MOB_BLOCK_D** (54) - Blocks down

Characters cannot pass through in the blocked direction unless they meet bypass conditions.

**Code References:** `movement.c:622-640`

### MOB_BLOCK_CLASS (Index: 55)
**Effect:** When blocking, allows characters of same class to pass.
- Exempts characters with matching class from directional blocks
- Used for class-specific guardians

**Code References:** `movement.c:645`

### MOB_BLOCK_RACE (Index: 56)
**Effect:** When blocking, allows characters of same race to pass.
- Exempts characters with matching race from directional blocks
- Used for racial guardians and city gates

**Code References:** `movement.c:643`

### MOB_BLOCK_LEVEL (Index: 57)
**Effect:** When blocking, allows lower-level characters to pass.
- Characters above mob's level are blocked
- Used for level-gated areas

**Code References:** `movement.c:649`

### MOB_BLOCK_ALIGN (Index: 58)
**Effect:** When blocking, allows characters of same alignment to pass.
- Checks good/evil/neutral alignment match
- Used for alignment-restricted areas

**Code References:** `movement.c:651-655`

### MOB_BLOCK_ETHOS (Index: 59)
**Effect:** When blocking, allows characters of same ethos to pass.
- Checks lawful/neutral/chaotic ethos match
- Used for ethos-restricted areas

### MOB_BLOCK_EVIL (Index: 90)
**Effect:** Specifically blocks evil-aligned characters.
- Good and neutral characters can pass
- Used for holy sanctuaries

**Code References:** `movement.c:657`

### MOB_BLOCK_NEUTRAL (Index: 91)
**Effect:** Specifically blocks neutral-aligned characters.
- Good and evil characters can pass
- Used for extreme-alignment areas

**Code References:** `movement.c:659`

### MOB_BLOCK_GOOD (Index: 92)
**Effect:** Specifically blocks good-aligned characters.
- Neutral and evil characters can pass
- Used for unholy temples

**Code References:** `movement.c:661`

---

## Combat Helper Flags

### MOB_MEMORY (Index: 11)
**Effect:** Mobile remembers characters who attack it.
- Tracks attackers even after combat ends
- Will attack remembered enemies on sight
- Memory persists until mob dies or memory is cleared
- Used for intelligent creatures and guards

**Code References:**
- `mob_memory.c:31` - Memory system check
- `fight.c:5154, 5475, 12301` - Adding to memory
- `graph.c:411` - Pathfinding to remembered enemies

### MOB_HELPER (Index: 12)
**Effect:** Mobile assists other NPCs fighting against players.
- Jumps into combat to help fellow NPCs
- Conflicts with MOB_AGGRESSIVE flag
- Used for pack tactics and coordinated defense

**Code References:**
- `mob_act.c:190, 285, 291` - Helper behavior logic
- `limits.c:2246` - Removed when mob becomes aggressive

### MOB_GUARD (Index: 31)
**Effect:** Mobile protects citizens and assists them in combat.
- Specifically protects NPCs with MOB_CITIZEN flag
- More selective than MOB_HELPER
- Used for city guards and protectors

**Code References:**
- `mob_act.c:285, 290, 311, 313` - Guard protection logic
- `missions.c:511, 633, 732` - Mission guard assignment

### MOB_CITIZEN (Index: 32)
**Effect:** Mobile is protected by guards.
- Guards will defend this NPC
- Used for shopkeepers, quest NPCs, civilians
- Creates natural guard/citizen relationships

**Code References:**
- `mob_act.c:311, 313` - Protection checks
- `missions.c:518, 640, 739` - Citizen designation

### MOB_HUNTER (Index: 33)
**Effect:** Mobile actively tracks down and hunts enemies.
- Aggressively pursues targets across multiple rooms
- Combines with MOB_MEMORY for persistent pursuit
- Used for predators and assassins

**Code References:**
- `mob_act.c:371` - Hunter behavior trigger
- `fight.c:5486` - Hunt initiation

### MOB_MOB_ASSIST (Index: 61)
**Effect:** Mobile will assist other mobs in its group/following.
- Enables mob-to-mob cooperation
- Used for coordinated monster groups
- Affects tactical combat

**Code References:** `mob_act.c:303` - Assistance logic

### MOB_HUNTS_TARGET (Index: 63)
**Effect:** Mobile hunts specific target(s).
- More focused than general hunter behavior
- Used for assassination missions and bounty hunters

---

## Companion & Summoned Creature Flags

### MOB_C_ANIMAL (Index: 26)
**Effect:** Mobile is a ranger's animal companion.
- Summoned via animal companion abilities
- Follows companion mechanics
- Scales with ranger level

### MOB_C_FAMILIAR (Index: 27)
**Effect:** Mobile is a wizard's/sorcerer's familiar.
- Summoned via find familiar spell
- Shares senses with master
- Provides bonuses to caster

### MOB_C_MOUNT (Index: 28)
**Effect:** Mobile is a summoned/companion mount.
- Summoned via paladin abilities or mount spells
- Can be ridden by summoner
- Scales with character level

### MOB_MOUNTABLE (Index: 21)
**Effect:** Mobile can be mounted and ridden.
- Players can use 'mount' command on this NPC
- Not necessarily a summoned mount
- Used for horses, griffons, etc.

**Code References:**
- `act.other.c:2457, 2561` - Mount command checks
- `movement_cost.c:38` - Movement cost adjustments
- `evolutions.c:715` - Eidolon mountable flag

### MOB_ELEMENTAL (Index: 29)
**Effect:** Mobile is an elemental creature.
- Summoned via summon elemental spells
- Subject to elemental rules and dismissal
- Fire, water, earth, air elementals

### MOB_ANIMATED_DEAD (Index: 30)
**Effect:** Mobile is animated undead.
- Created via animate dead spell
- Subject to turn undead and control undead
- Skeletons, zombies, etc.

### MOB_PLANAR_ALLY (Index: 36)
**Effect:** Mobile is a summoned planar ally.
- Summoned via planar ally spells
- Currently unused/planned feature
- Angels, demons, celestials

### MOB_MERCENARY (Index: 41)
**Effect:** Mobile is a hired mercenary.
- Purchased companion, not magically summoned
- Only one per person allowed
- Costs gold to maintain

### MOB_SHADOW (Index: 43)
**Effect:** Mobile is a shadowdancer's shadow companion.
- Summoned via shadowdancer abilities
- Special shadow-based powers
- Scales with shadowdancer level

### MOB_C_O_T_N (Index: 85)
**Effect:** Mobile is a Children of the Night vampire companion.
- Summoned via vampire abilities
- Wolves, rats, bats
- Vampire-specific summoning

### MOB_VAMP_SPWN (Index: 86)
**Effect:** Mobile is a vampire spawn.
- Created by vampires
- Lesser undead servant
- Vampire-specific creation

### MOB_DRAGON_KNIGHT (Index: 87)
**Effect:** Mobile is a dragon knight's mount.
- Special draconic mount
- Dragon-themed abilities
- Class-specific companion

### MOB_MUMMY_DUST (Index: 88)
**Effect:** Mobile created via mummy dust spell.
- Temporary undead servant
- Spell-specific summoning

### MOB_EIDOLON (Index: 89)
**Effect:** Mobile is a summoner's eidolon.
- Powerful customizable companion
- Summoner class feature
- Highly customizable with evolutions

### MOB_GENIEKIND (Index: 93)
**Effect:** Mobile is a genie or genie-type creature.
- Djinn, efreet, marid, dao
- Subject to genie-specific rules
- Elemental outsiders

### MOB_C_DRAGON (Index: 94)
**Effect:** Mobile is a companion dragon.
- Dragon companion for specific classes
- Scales with character
- Powerful draconic ally

### MOB_RETAINER (Index: 95)
**Effect:** Mobile is a leadership retainer/cohort.
- Gained via leadership feats
- Follows cohort/follower rules
- Permanent companion

---

## Special Ability Flags

### MOB_ABIL_GRAPPLE (Index: 64)
**Effect:** Mobile can perform grapple attacks.
- Special grapple combat maneuver
- Used by tentacled creatures, wrestlers

### MOB_ABIL_PETRIFY (Index: 65)
**Effect:** Mobile can petrify enemies.
- Turn targets to stone
- Used by medusas, basilisks, cockatrices

### MOB_ABIL_TAIL_SPIKES (Index: 66)
**Effect:** Mobile can fire tail spikes.
- Ranged spike attack
- Used by manticores, similar creatures

### MOB_ABIL_LEVEL_DRAIN (Index: 67)
**Effect:** Mobile can drain character levels.
- Energy drain attacks
- Used by wights, wraiths, vampires

### MOB_ABIL_CHARM (Index: 68)
**Effect:** Mobile can charm enemies.
- Mind control special attack
- Used by vampires, nymphs, succubi

### MOB_ABIL_BLINK (Index: 69)
**Effect:** Mobile can blink (short-range teleport).
- Random teleportation in combat
- Hard to hit in melee
- Used by blink dogs, phase spiders

### MOB_ABIL_ENGULF (Index: 70)
**Effect:** Mobile can engulf enemies.
- Swallow or surround attack
- Used by oozes, gelatinous cubes

### MOB_ABIL_CAUSE_FEAR (Index: 71)
**Effect:** Mobile can cause fear in enemies.
- Fear aura or special attack
- Used by dragons, demons, undead

### MOB_ABIL_CORRUPTION (Index: 72)
**Effect:** Mobile has corruption touch/attack.
- Disease or corruption effect
- Used by diseased creatures, demons

**Code References:** `fight.c:11963` - Corruption attack trigger

### MOB_ABIL_SWALLOW (Index: 73)
**Effect:** Mobile can swallow enemies whole.
- Internal damage while swallowed
- Used by large creatures, purple worms

### MOB_ABIL_FLIGHT (Index: 74)
**Effect:** Mobile can fly.
- Airborne movement
- Ignores ground-based obstacles
- Used by dragons, birds, flying creatures

### MOB_ABIL_POISON (Index: 75)
**Effect:** Mobile has poisonous attacks.
- Applies poison on successful hits
- Used by snakes, spiders, venomous creatures

**Code References:** `fight.c:11956` - Poison attack trigger

### MOB_ABIL_REGENERATION (Index: 76)
**Effect:** Mobile regenerates HP quickly.
- Enhanced natural healing
- Used by trolls, hydras, regenerating creatures

### MOB_ABIL_PARALYZE (Index: 77)
**Effect:** Mobile can paralyze enemies.
- Paralysis special attack
- Used by carrion crawlers, ghouls

### MOB_ABIL_FIRE_BREATH (Index: 78)
**Effect:** Mobile can breathe fire.
- Fire breath weapon attack
- Used by red dragons, salamanders

### MOB_ABIL_LIGHTNING_BREATH (Index: 79)
**Effect:** Mobile can breathe lightning.
- Lightning breath weapon attack
- Used by blue dragons, storm elementals

### MOB_ABIL_POISON_BREATH (Index: 80)
**Effect:** Mobile can breathe poison gas.
- Poison breath weapon attack
- Used by green dragons

### MOB_ABIL_ACID_BREATH (Index: 81)
**Effect:** Mobile can breathe acid.
- Acid breath weapon attack
- Used by black dragons

### MOB_ABIL_FROST_BREATH (Index: 82)
**Effect:** Mobile can breathe frost/cold.
- Cold breath weapon attack
- Used by white dragons, frost giants

### MOB_ABIL_MAGIC_IMMUNITY (Index: 83)
**Effect:** Mobile is immune to magic.
- Spells have no effect
- Used for golems, antimagic creatures
- Extremely powerful defensive ability

### MOB_ABIL_INVISIBILITY (Index: 84)
**Effect:** Mobile is naturally invisible.
- Permanently invisible
- Can still be detected by special senses
- Used by invisible stalkers, certain fey

---

## System & Internal Flags

### MOB_ISNPC (Index: 3)
**Effect:** Read-only flag automatically set on all NPCs.
- Distinguishes NPCs from player characters
- System flag - never manually set
- Used throughout codebase for NPC checks

**Important:** This is a system flag that should never be manually toggled.

### MOB_NOTDEADYET (Index: 20)
**Effect:** Read-only flag indicating mobile is being extracted.
- Set during mob removal from game
- Prevents double-extraction bugs
- System flag for cleanup process

**Important:** This is a system flag that should never be manually toggled.

### MOB_NOFIGHT (Index: 23)
**Effect:** Mobile will not engage in combat.
- Passive creatures that won't fight back
- Used for ambient NPCs, decorative creatures
- Different from MOB_NOKILL (which prevents being attacked)

### MOB_NOCLASS (Index: 24)
**Effect:** Mobile has no character class.
- Used for monsters without PC-style classes
- Affects certain class-specific interactions

### MOB_INFO_KILL (Index: 38)
**Effect:** Broadcasts message to entire game when killed.
- Global notification of death
- Used for world bosses, unique enemies
- Creates server-wide event

### MOB_INFO_KILL_PLR (Index: 60)
**Effect:** Broadcasts message when this mob kills a player.
- Global notification when player dies to this mob
- Used for particularly dangerous enemies
- Creates server-wide death announcement

### MOB_CUSTOM_GOLD (Index: 39)
**Effect:** Mobile uses custom gold drop amounts.
- Overrides standard treasure calculation
- Used for specific treasure amounts
- Quest rewards, unique drops

### MOB_NO_AI (Index: 40)
**Effect:** Disables AI routines for this mobile.
- Prevents standard AI behavior
- Used for special scripted NPCs
- Manual control only

### MOB_AI_ENABLED (Index: 98)
**Effect:** Mobile uses advanced AI for responses.
- Enhanced decision-making
- More intelligent behavior patterns
- Used for challenging enemies

### MOB_ENCOUNTER (Index: 42)
**Effect:** Mobile is part of wilderness random encounter system.
- Spawns in random encounters
- Used for wandering monsters
- Integrates with encounter tables

### MOB_IS_OBJ (Index: 44)
**Effect:** Mobile represents an object (quest board, etc.).
- Special case: mob used as interactive object
- Used for bulletin boards, unique interfaces
- Non-standard usage

### MOB_QUARTERMASTER (Index: 99)
**Effect:** Mobile can accept and complete supply orders.
- Special NPC for supply management
- Mission/quest system integration
- Resource gathering quests

### MOB_UNLIMITED_SPELL_SLOTS (Index: 100)
**Effect:** Mobile has unlimited spell slots.
- Bypasses normal spell slot system
- Can cast spells without running out
- Used for powerful casters, bosses

### MOB_BUFF_OUTSIDE_COMBAT (Index: 96)
**Effect:** UNUSED - Kept for backward compatibility.
- No longer functional
- Reserved for future use
- Do not use

---

## Complete Flag Reference

### Quick Reference Table

| Index | Flag Name | Category | Primary Purpose |
|-------|-----------|----------|----------------|
| 0 | MOB_SPEC | System | Has special procedure |
| 1 | MOB_SENTINEL | Behavior | Won't move |
| 2 | MOB_SCAVENGER | Behavior | Picks up items |
| 3 | MOB_ISNPC | System | Is NPC (read-only) |
| 4 | MOB_AWARE | Combat | Can't be backstabbed |
| 5 | MOB_AGGRESSIVE | Combat | Attacks everyone |
| 6 | MOB_STAY_ZONE | Movement | Stays in zone |
| 7 | MOB_WIMPY | Behavior | Flees when hurt |
| 8 | MOB_AGGR_EVIL | Combat | Attacks evil |
| 9 | MOB_AGGR_GOOD | Combat | Attacks good |
| 10 | MOB_AGGR_NEUTRAL | Combat | Attacks neutral |
| 11 | MOB_MEMORY | Combat | Remembers attackers |
| 12 | MOB_HELPER | Combat | Assists other NPCs |
| 13 | MOB_NOCHARM | Immunity | Can't be charmed |
| 14 | MOB_NOSUMMON | Immunity | Can't be summoned |
| 15 | MOB_NOSLEEP | Immunity | Can't be slept |
| 16 | MOB_NOBASH | Immunity | Can't be bashed |
| 17 | MOB_NOBLIND | Immunity | Can't be blinded |
| 18 | MOB_NOKILL | Immunity | Can't be attacked |
| 19 | MOB_SENTIENT | Classification | Intelligent being |
| 20 | MOB_NOTDEADYET | System | Being extracted |
| 21 | MOB_MOUNTABLE | Companion | Can be ridden |
| 22 | MOB_NODEAF | Immunity | Can't be deafened |
| 23 | MOB_NOFIGHT | Behavior | Won't fight |
| 24 | MOB_NOCLASS | Classification | No class |
| 25 | MOB_NOGRAPPLE | Immunity | Can't be grappled |
| 26 | MOB_C_ANIMAL | Companion | Animal companion |
| 27 | MOB_C_FAMILIAR | Companion | Familiar |
| 28 | MOB_C_MOUNT | Companion | Summoned mount |
| 29 | MOB_ELEMENTAL | Companion | Elemental |
| 30 | MOB_ANIMATED_DEAD | Companion | Animated undead |
| 31 | MOB_GUARD | Combat | Protects citizens |
| 32 | MOB_CITIZEN | Classification | Protected by guards |
| 33 | MOB_HUNTER | Combat | Tracks foes |
| 34 | MOB_LISTEN | Behavior | Hears fighting |
| 35 | MOB_LIT | Environment | Emits light |
| 36 | MOB_PLANAR_ALLY | Companion | Planar ally (unused) |
| 37 | MOB_NOSTEAL | Immunity | Can't be stolen from |
| 38 | MOB_INFO_KILL | System | Broadcasts death |
| 39 | MOB_CUSTOM_GOLD | System | Custom gold drops |
| 40 | MOB_NO_AI | System | AI disabled |
| 41 | MOB_MERCENARY | Companion | Hired mercenary |
| 42 | MOB_ENCOUNTER | System | Wilderness encounter |
| 43 | MOB_SHADOW | Companion | Shadow companion |
| 44 | MOB_IS_OBJ | System | Represents object |
| 45 | MOB_BLOCK_N | Blocking | Blocks north |
| 46 | MOB_BLOCK_E | Blocking | Blocks east |
| 47 | MOB_BLOCK_S | Blocking | Blocks south |
| 48 | MOB_BLOCK_W | Blocking | Blocks west |
| 49 | MOB_BLOCK_NE | Blocking | Blocks northeast |
| 50 | MOB_BLOCK_SE | Blocking | Blocks southeast |
| 51 | MOB_BLOCK_SW | Blocking | Blocks southwest |
| 52 | MOB_BLOCK_NW | Blocking | Blocks northwest |
| 53 | MOB_BLOCK_U | Blocking | Blocks up |
| 54 | MOB_BLOCK_D | Blocking | Blocks down |
| 55 | MOB_BLOCK_CLASS | Blocking | Class exemption |
| 56 | MOB_BLOCK_RACE | Blocking | Race exemption |
| 57 | MOB_BLOCK_LEVEL | Blocking | Level restriction |
| 58 | MOB_BLOCK_ALIGN | Blocking | Alignment exemption |
| 59 | MOB_BLOCK_ETHOS | Blocking | Ethos exemption |
| 60 | MOB_INFO_KILL_PLR | System | Broadcasts PC death |
| 61 | MOB_MOB_ASSIST | Combat | Assists other mobs |
| 62 | MOB_NOCONFUSE | Immunity | Can't be confused |
| 63 | MOB_HUNTS_TARGET | Combat | Hunts specific target |
| 64 | MOB_ABIL_GRAPPLE | Ability | Grapple attack |
| 65 | MOB_ABIL_PETRIFY | Ability | Petrification |
| 66 | MOB_ABIL_TAIL_SPIKES | Ability | Spike attack |
| 67 | MOB_ABIL_LEVEL_DRAIN | Ability | Level drain |
| 68 | MOB_ABIL_CHARM | Ability | Charm attack |
| 69 | MOB_ABIL_BLINK | Ability | Blink/teleport |
| 70 | MOB_ABIL_ENGULF | Ability | Engulf attack |
| 71 | MOB_ABIL_CAUSE_FEAR | Ability | Cause fear |
| 72 | MOB_ABIL_CORRUPTION | Ability | Corruption touch |
| 73 | MOB_ABIL_SWALLOW | Ability | Swallow whole |
| 74 | MOB_ABIL_FLIGHT | Ability | Can fly |
| 75 | MOB_ABIL_POISON | Ability | Poison attack |
| 76 | MOB_ABIL_REGENERATION | Ability | Regenerates HP |
| 77 | MOB_ABIL_PARALYZE | Ability | Paralyze attack |
| 78 | MOB_ABIL_FIRE_BREATH | Ability | Fire breath |
| 79 | MOB_ABIL_LIGHTNING_BREATH | Ability | Lightning breath |
| 80 | MOB_ABIL_POISON_BREATH | Ability | Poison breath |
| 81 | MOB_ABIL_ACID_BREATH | Ability | Acid breath |
| 82 | MOB_ABIL_FROST_BREATH | Ability | Frost breath |
| 83 | MOB_ABIL_MAGIC_IMMUNITY | Ability | Immune to magic |
| 84 | MOB_ABIL_INVISIBILITY | Ability | Naturally invisible |
| 85 | MOB_C_O_T_N | Companion | Children of Night |
| 86 | MOB_VAMP_SPWN | Companion | Vampire spawn |
| 87 | MOB_DRAGON_KNIGHT | Companion | Dragon knight mount |
| 88 | MOB_MUMMY_DUST | Companion | Mummy dust creation |
| 89 | MOB_EIDOLON | Companion | Eidolon |
| 90 | MOB_BLOCK_EVIL | Blocking | Blocks evil |
| 91 | MOB_BLOCK_NEUTRAL | Blocking | Blocks neutral |
| 92 | MOB_BLOCK_GOOD | Blocking | Blocks good |
| 93 | MOB_GENIEKIND | Classification | Genie-type |
| 94 | MOB_C_DRAGON | Companion | Companion dragon |
| 95 | MOB_RETAINER | Companion | Leadership retainer |
| 96 | MOB_BUFF_OUTSIDE_COMBAT | Deprecated | UNUSED |
| 97 | MOB_NOPARALYZE | Immunity | Can't be paralyzed |
| 98 | MOB_AI_ENABLED | System | Advanced AI |
| 99 | MOB_QUARTERMASTER | System | Supply orders |
| 100 | MOB_UNLIMITED_SPELL_SLOTS | System | No spell limits |

---

## Usage Guidelines

### For Builders

**Common Mob Archetypes:**

1. **City Guard:**
   - MOB_SENTINEL + MOB_GUARD + MOB_MEMORY
   - Protects citizens, doesn't wander, remembers criminals

2. **Aggressive Monster:**
   - MOB_AGGRESSIVE + MOB_MEMORY + MOB_HUNTER
   - Attacks on sight, pursues, remembers enemies

3. **Alignment Guardian:**
   - MOB_SENTINEL + MOB_AGGRESSIVE + MOB_AGGR_EVIL (or GOOD/NEUTRAL)
   - Stationary guardian that attacks specific alignments

4. **Pack Hunter:**
   - MOB_HELPER + MOB_MEMORY + MOB_MOB_ASSIST
   - Works with other mobs, remembers threats

5. **Boss Monster:**
   - MOB_NOCHARM + MOB_NOSUMMON + MOB_MEMORY + relevant MOB_ABIL_* flags
   - Immune to cheap tactics, dangerous abilities

6. **Ambient NPC:**
   - MOB_SENTINEL + MOB_NOKILL + MOB_NOFIGHT
   - Decorative NPC that can't fight and can't be attacked

**Flag Conflicts:**
- MOB_AGGRESSIVE conflicts with MOB_HELPER (removed automatically)
- MOB_AGGRESSIVE requires alignment aggro flags to have effect
- Multiple MOB_BLOCK_* direction flags work together

### For Developers

**Checking Flags:**
```c
if (MOB_FLAGGED(mob, MOB_AGGRESSIVE)) {
    // Mob is aggressive
}

// Multiple flag check
if (MOB_FLAGGED(mob, MOB_GUARD) && MOB_FLAGGED(mob, MOB_MEMORY)) {
    // Guard with memory
}
```

**Setting Flags:**
```c
SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);
```

**Removing Flags:**
```c
REMOVE_BIT_AR(MOB_FLAGS(mob), MOB_AGGRESSIVE);
```

**System Flags to Never Modify:**
- MOB_ISNPC (automatically set)
- MOB_NOTDEADYET (extraction system)
- MOB_BUFF_OUTSIDE_COMBAT (deprecated)

---

## Code References

**Primary Files:**
- `src/structs.h` (lines 1131-1232) - Flag definitions
- `src/mob_act.c` - Mobile AI and behavior
- `src/fight.c` - Combat interactions
- `src/movement.c` - Movement and blocking
- `src/spec_abilities.c` - Special abilities
- `src/spells.c` - Spell interactions

---

## Notes

- Mobile flags use array-based bitflags for storage
- Multiple flags can and often should be combined
- Some flags require other flags to function (e.g., MOB_AGGR_EVIL requires MOB_AGGRESSIVE)
- System flags should not be manually set by builders
- Companion flags are automatically set when creatures are summoned
- Special ability flags should match creature capabilities

---

**Last Updated:** November 6, 2024  
**Version:** 1.0  
**Maintainer:** LuminariMUD Development Team
