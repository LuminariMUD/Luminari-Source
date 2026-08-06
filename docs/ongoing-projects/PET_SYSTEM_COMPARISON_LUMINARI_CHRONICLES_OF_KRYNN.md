# Pet System Comparison: Luminari and Chronicles of Krynn

Status: comparison complete; design and remediation decisions remain open

Verified against source: 2026-08-06

## Purpose and scope

This audit compares the current Luminari pet implementation with the detached
sample at `EXAMPLE/ChroniclesOfKrynn`. It covers the runtime model, acquisition,
limits, control, movement, combat, progression, persistence, database schema,
tests, and player documentation.

In both codebases, "pet" is an umbrella term. It includes permanent class
companions, purchased pets, charmed creatures, temporary summons, animated
dead, crafted golems, retainers, and several special followers. This document
therefore treats the follower framework and pet persistence as part of the pet
system.

The sample is reference code only. It has no Git metadata and is not an
upstream or merge target.

## Executive findings

1. The two systems still share the same basic model: a pet is an ordinary NPC
   linked through `master` and the master's `followers` list, normally with
   `AFF_CHARM`. Mobile flags identify companion families. Neither codebase has
   a dedicated runtime pet object or a single pet-system boundary.
2. Luminari has substantially better state persistence. Its versioned
   `runtime_state` captures flags, affects, combat values, resources, spell
   state, feat overrides, and mercenary state with bounded parsing. Chronicles
   saves only a small set of base fields, but it does preserve the remaining
   purge timer for temporary pets, which Luminari currently loses.
3. Chronicles has a clearer category-based admission model. It names twelve
   follower categories and accounts for special free summon slots. Luminari's
   older mixture of free followers, Charisma slots, and per-flag checks is
   harder to reason about and contains counting defects.
4. The Chronicles limiter is not ready to port unchanged. Its display and
   spare-slot calculation disagree with enforcement, and Twin Eidolon appears
   to be rejected by the same one-eidolon rule that the feature invokes.
5. Both implementations persist every charmed NPC follower, including
   temporary summons and ordinary charmed creatures, and both reload saved
   followers without rechecking current admission limits.
6. Both persistence paths delete the owner's previous rows before rebuilding
   them and do not use a transaction. A failed insert can therefore destroy a
   previously valid snapshot. This is already material to Luminari's separate
   production incident investigation.
7. A selective startup initializer prevents pet column migrations from running
   on an existing `pet_data` table in both trees. Schema files and runtime table
   creation also disagree.
8. The best direction is a combined design: retain Luminari's validated runtime
   state, add explicit lifetime persistence, and replace admission with one
   typed, centralized category engine whose result is shared by enforcement,
   display, loading, and tests.

## Source map

The primary traces used in this comparison are:

| Concern | Luminari | Chronicles of Krynn sample |
|---------|----------|-----------------------------|
| Pet identity and follower limits | `src/utils.h`, `src/utils.c` | `src/utils.h`, `src/utils.c` |
| Runtime structures and flags | `src/structs.h` | `src/structs.h` |
| Call, dismiss, order, and pets commands | `src/act.other.c` | `src/act.other.c` |
| Companion selection and class grants | `src/character/study.c`, `src/character/class.c` | `src/study.c`, `src/class.c` |
| Summoning and charm | `src/magic/magic.c`, `src/magic/spells.c` | `src/magic.c`, `src/spells.c` |
| Combat integration | `src/combat/fight.c`, `src/combat/act.offensive.c` | `src/fight.c`, `src/act.offensive.c` |
| Movement and NPC behavior | `src/movement/movement.c`, `src/mob/mob_act.c` | `src/movement.c`, `src/mob_act.c` |
| Pet base persistence | `src/players.c` | `src/players.c` |
| Pet object persistence | `src/obj/objsave.c` | `src/objsave.c` |
| Schema creation and startup | `src/db_init.c`, `src/db_startup_init.c`, `sql/master_schema.sql` | `src/db_init.c`, `src/db_startup_init.c`, `sql/master_schema.sql` |
| Golems, shops, and special followers | `src/craft/crafting_new.c`, `src/obj/shop.c`, `src/spec_procs.c` | `src/crafting_new.c`, `src/shop.c`, `src/spec_procs.c` |
| Focused tests | `unittests/CuTest/test_database_persistence.c`, `unittests/CuTest/test_upstream_regressions.c` | No focused pet persistence or admission tests found |

Paths in the right column are relative to
`EXAMPLE/ChroniclesOfKrynn`.

## 1. Shared runtime model

Both trees define a pet through the same relationship:

```c
IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master
```

The master owns a linked list of `follow_type` records. The follower itself is
still a normal `char_data`. Consequently, the generic character, affect,
movement, combat, extraction, and event systems all participate in pet
behavior.

The major companion flags are also shared: animal companion, familiar, mount,
elemental, animated dead, planar ally, mercenary, shadow, cohort, vampire spawn,
dragon knight, mummy dust, eidolon, dragon companion, retainer, and golem.
Chronicles additionally defines a solar summon category and a flightless flag.

This design has useful reuse, but it creates two systemic risks:

- `AFF_CHARM` is both a control effect and the persistence eligibility test.
  Temporary magical control and durable ownership are not distinct concepts.
- Direct calls to `add_follower()` can bypass policy because relationship
  mutation and admission are separate operations.

Persistent player choices such as selected familiar or animal companion live
in player special-ability fields. The live NPC instance is separate and is
saved through the pet database tables.

## 2. Acquisition and lifecycle

### Shared acquisition routes

Both codebases create or acquire followers through several independent paths:

- `call` for animal companions, familiars, paladin or blackguard mounts,
  shadows, eidolons or undead cohorts, and dragon mounts;
- charm, dominate, and plant-control spells;
- summon, planar, genie, epic summon, and animate-dead spell paths;
- legacy room pet shops and shop objects of type `ITEM_PET`;
- crafted golems, retainers, quest followers, background features, and special
  abilities.

This breadth is why a limiter embedded in only the call or spell command cannot
be authoritative.

Luminari also has current artifact-created followers, including the Oaken
Defender and horn-created charmed follower. These are absent from the sample
and currently consult the old spare-slot interface.

### Called companion progression

The ordinary companion formulas remain close between the trees:

- Animal companion level is full druid level plus ranger levels after level 3.
  Boon Companion adds up to five effective levels, capped at 20.
- Familiar durability scales from caster level, with Improved Familiar ranks
  adding hit points, armor, and physical ability scores.
- Ordinary mounts cap at level 20; special mounts can reach 27.
- Dragon mounts cap at 25, shadows at 25, and eidolons at 30.
- Called companions receive charm and ultravision, automatically join the
  group, and normally impose a four-MUD-day recall delay.
- Dismissing a present called companion reduces the remaining delay to at most
  59 seconds. Calling an existing separated companion moves it to the owner
  rather than creating a replacement.

The selection arrays define ten animals, familiars, and mounts, while the menu
displays only entries 1 through 8. Validation accepts entry 9 but rejects entry
10 because the count constant is used as an exclusive upper bound. This shared
off-by-one and hidden-choice behavior should be fixed independently of any
larger port.

### Chronicles-only progression

Chronicles extends the shared call path in several ways:

- Warlocks receive and scale a familiar; Luminari only counts wizard and
  sorcerer levels in its familiar call path.
- Summoner perks increase eidolon hit points, natural armor, damage, magic
  attacks, fast healing, and recall availability.
- Twin Eidolon attempts to create a second, weaker, short-lived eidolon.
- Master Summoner and Superior Summoner grant additional free summon slots.
- Further summoner combat hooks include spell resistance, save or DC changes,
  and Planar Unity effects.

The ranger companion perk integrations are substantially shared. Chronicles
also has broader golem management, including transfer behavior that is absent
from current Luminari.

These features are candidates for product review, not automatic ports. Their
rules depend on the follower-limit model and need tests before adoption.

## 3. Admission and follower limits

### Luminari: hybrid free and paid slots

Luminari's current logic combines three ideas:

1. Many named companion types are declared free by `not_npc_limit()`.
2. General paid capacity starts at one and grows with a positive Charisma
   modifier.
3. Separate checks attempt to reserve or limit mercenary, golem, genie,
   summon, undead, and other follower classes.

The `pets` command reports each follower and whether it consumes a slot, then
summarizes free and paid capacity. The model preserves the traditional
Charisma-based follower budget, but behavior is spread across
`not_npc_limit()`, `can_add_follower()`, `can_add_follower_by_flag()`, and old
`NPC_MODE_SPARE` callers.

Two source defects make the result unreliable:

- While counting existing followers for a requested genie or summon,
  `can_add_follower()` tests the requested mobile VNUM rather than each
  existing follower. Every existing pet can therefore decrement the requested
  category's allowance.
- `can_add_follower_by_flag()` returns failure as soon as a matching undead is
  found using an inverted comparison. Its intended second necromancer allowance
  is not actually available.

Called class companions are normally free under this model, and the call path
does not perform a separate general-slot check.

### Chronicles: explicit categories

Chronicles replaces most of that policy with `can_add_follower_new()`, which
classifies a prospective follower into twelve categories:

| Category | Intended capacity |
|----------|-------------------|
| Epic summon | One across dragon knight, mummy dust, and solar |
| Animal companion | One |
| Familiar | One |
| Paladin or blackguard mount | One |
| Shadow | One |
| Eidolon or cohort | One |
| Mercenary | One |
| Summoned creature | Free summoner slots, then general Other slot |
| Necromantic undead | One free necromancer slot |
| Golem | One |
| Dragon-rider dragon | One |
| Other follower | One |

The explicit taxonomy is easier to understand and is the strongest design idea
in the sample. It also removes Charisma from ordinary capacity. Whether that is
desirable is a game-design decision, not merely a refactor.

The implementation nevertheless has important internal contradictions:

- Enforcement counts perk-granted summoner slots, but `check_npc_followers()`
  displays only a single free summon slot and its spare calculation does not
  account consistently for excess summons. UI, legacy callers, and enforcement
  can disagree.
- Twin Eidolon creates another mobile carrying `MOB_EIDOLON`. The first
  eidolon already occupies the one eidolon/cohort category, so the shared
  admission function appears to reject the twin the feature is trying to add.
- Persistent loading still calls `add_follower()` directly and does not
  reconcile the loaded set against the current policy.
- Other direct relationship mutations remain outside the central gateway.

### Admission conclusion

Do not copy either limiter unchanged. Introduce a public follower-category enum
and one admission result structure that contains category, used capacity,
allowed capacity, whether a general slot is consumed, and a rejection reason.
The same calculation must serve creation, charm, loading, the `pets` command,
and tests. Intentional exceptions such as a temporary twin must be represented
explicitly rather than bypassing or accidentally colliding with policy.

## 4. Player control, movement, and combat

The core behavior is nearly the same:

- `order <pet|followers> <command>` requires a charmed NPC owned by the player
  and present in the room, then dispatches the ordered command through the
  normal interpreter.
- `pets` enumerates controlled NPC followers.
- summon-to-owner helpers reunite separated charmed followers after several
  travel, teleport, and script operations.
- ordinary follower movement uses the shared follower list; riding uses the
  separate mount system.
- `stop_follower()` removes charm-related affects. Several summoned or called
  mobile families receive a delayed purge event after separation.
- death and extraction use ordinary NPC lifecycle handling and sever follower
  links.

Shared combat integrations include:

- kill experience can be credited through a present owner;
- pets are excluded from the normal group-member experience split and from
  direct quest-kill credit;
- owner auto-loot and auto-sacrifice behavior can run after a pet kill;
- careful-pet mode prevents attacks between owner and pet or sibling pets;
- otherwise, attacking one's own follower can break the relationship;
- hostile NPCs can remember or pursue an owner when the owner's pet attacks,
  limiting disposable-pet abuse;
- eidolon Life Link, Bond, evolutions, and ranger companion perks hook into
  combat.

Because these checks use the broad `IS_PET` predicate, an ordinary charmed mob
often receives the same ownership behavior as a permanent class companion.
That is another reason to make follower kind explicit.

## 5. Persistence comparison

### Shared database flow

Both implementations use:

- `pet_data` for the live follower's base record;
- `pet_save_objs` for equipped, carried, and nested objects;
- the owner's name as the ownership key;
- a generated `pet_data` row ID to associate saved objects with a pet.

`save_char_pets()` selects NPC followers with `AFF_CHARM`. It does not restrict
persistence to permanent companion flags. Loading recreates the prototype,
applies saved fields, restores objects, adds charm, and links the follower to
the player.

Both trees save pets frequently, including the periodic player update path,
and both rewrite the complete owner snapshot. The rewrite sequence deletes
old object rows and pet rows before inserting replacements, without a
transaction or staging generation. A later query failure can leave no usable
snapshot.

### Luminari strengths

Luminari adds a bounded, versioned `runtime_state` record. It preserves far
more of the actual pet than the sample does, including:

- runtime affect and mobile flags separated from prototype, equipment, and
  timed-affect contributions;
- intelligence, race, size, movement, psionic points, alignment, attack values,
  damage dice, armor, and saving throws;
- spell slots and maxima;
- timed affects;
- feat overrides;
- mercenary identity and procedure state.

The serializer allocates based on required size. The parser validates the
version, field bounds, complete consumption, and bit masks. Loading has better
NULL handling, clone identity ordering, dead-pet rejection, post-load position
repair, and load-trigger extraction checks. Legacy rows can still load without
the runtime record.

Focused CuTests cover runtime-state round trips and malformed-state rejection.
Luminari also extracted the animal companion level calculation for regression
testing.

### Luminari gaps

Luminari does not persist the remaining `ePURGEMOB` event lifetime. A temporary
summon can therefore reload without its original expiration event. Timed spell
affects may survive through `runtime_state`, but event-based lifetime and affect
duration are separate mechanisms.

Its strongest state model also increases the cost of the shared destructive
rewrite and startup-migration defects. The confirmed production incident and
containment work are tracked separately in
[production-crash-2026-08-05-pet-persistence.md](production-crash-2026-08-05-pet-persistence.md).

### Chronicles strengths

Chronicles saves `purge_mob_timer`. It locates the pet's purge event, records
the remaining queue time, and recreates the event during load. This preserves
temporary follower expiry across a disconnect better than Luminari.

### Chronicles gaps

Chronicles saves only prototype VNUM, level, hit points, six ability-related
fields, armor, descriptions, and the purge timer. Its `SELECT` expects
intelligence, but its insert does not store intelligence, so the value falls
back to the table default. It does not durably preserve most runtime flags,
timed affects, combat modifiers, dice, saves, spell state, feats, evolutions,
movement, psionics, race, size, alignment, or mercenary procedure state.

The code also has weaker failure boundaries:

- fixed-size query buffers are filled through pointer advancement without a
  complete remaining-capacity contract;
- name validation and loaded field access have missing NULL guards;
- clone owner identity can be overwritten by the subsequently loaded saved
  descriptions;
- dead-pet, final-position, and load-trigger extraction checks are absent;
- the pet object loader can terminate the entire process when
  `mysql_store_result()` fails.

No focused admission or persistence tests were found in the sample.

### Lifetime recommendation

Retain Luminari's validated runtime record and add explicit lifetime metadata.
Prefer an event kind plus an absolute expiry time over a raw queue pulse count,
then refuse to restore an already expired temporary follower. The product rule
must also state which follower categories are durable. Persisting every
`AFF_CHARM` follower is an accidental policy, not a clear design decision.

## 6. Schema and migration findings

Both startup initializers call `init_core_player_tables()` only when
`player_data` or `pet_data` is missing. Column-addition logic placed inside that
initializer is therefore skipped when the tables already exist. In Luminari,
that includes the `runtime_state` migration. In Chronicles, it includes the
`purge_mob_timer` migration. Column migrations need an unconditional,
idempotent startup path with a schema version or migration ledger.

There is additional schema drift:

- Luminari's runtime initializer and `sql/master_schema.sql` disagree on the
  `pet_save_objs` primary-key name, owner length, and serialized-data type.
- The Chronicles master schema contains `pet_save_objs` but no `pet_data`
  definition at all; runtime initialization is required to complete a fresh
  schema.
- Neither design establishes a relational foreign key from saved pet objects
  to their base pet row.
- Ownership uses a mutable player name rather than a stable player identifier.

All executable schema definitions should be generated from or checked against
one authoritative migration series.

## 7. Documentation and observability

Luminari has player help entries for call, dismiss, eidolon, companion, and
related commands. The sample's flat `lib/text/help` directory does not contain
an equivalent help corpus, so runtime help parity cannot be established from
the detached source snapshot. Chronicles does include design documentation for
summoner perks; Luminari has its own mount and ranger-perk documentation.

Both systems expose a `pets` command, but neither provides a durable pet ID,
remaining lifetime, persistence class, or a canonical admission explanation.
A redesigned status view should show category, slot source, persistence policy,
and expiry when applicable. Operational logs should report query identity and
error context without logging the complete serialized SQL payload.

## 8. Recommended Luminari plan

### P0: protect durable state

1. Move pet schema changes to unconditional, versioned, idempotent migrations.
2. Replace delete-before-insert with a transaction or staged snapshot swap.
3. Reconcile `db_init.c` and `sql/master_schema.sql` from one authoritative
   schema definition.
4. Treat any partial save as failure and preserve the last valid snapshot.

### P1: establish one pet policy

1. Add a typed follower category and a single admission calculation.
2. Route every player-controlled follower acquisition through it, including
   charm, summons, shops, call, crafting, artifacts, retainers, and loading.
3. Decide whether Charisma capacity remains part of Luminari's rules.
4. Define explicit behavior for perk-granted summon slots, necromancer slots,
   epic mutual exclusion, and intentional temporary duplicates.
5. Reconcile or reject over-cap saved sets deterministically during load.

### P1: merge persistence strengths

1. Keep Luminari's versioned runtime-state parser and prototype-delta approach.
2. Persist temporary lifetime and reject expired pets at load.
3. Define durable categories instead of saving every charmed NPC.
4. Use a stable player ID for ownership and enforce pet/object relationships.
5. Return recoverable errors from object loading rather than terminating the
   server.

### P2: reduce churn and improve clarity

1. Mark pet state dirty and save on meaningful change or a slower checkpoint,
   rather than rewriting all followers on every periodic player update.
2. Make `pets` use the exact admission result used by creation.
3. Expose category, consumed slot, lifetime, and persistence class to staff
   diagnostics.
4. Correct the shared companion-selection menu and bound mismatch.
5. Update help files once the policy decisions are final.

## 9. Required regression coverage

Before replacing the current limiter or expanding pet features, add production-
linked tests for:

- classification of every follower flag and relevant spell-summon VNUM;
- the complete category-by-category admission matrix;
- Charisma capacity, if retained;
- summoner perk slot counts and display parity;
- an intentional Twin Eidolon exception or replacement design;
- necromancer and epic-summon mutual-exclusion rules;
- all acquisition routes using the central gateway;
- over-cap and obsolete saved followers at login;
- runtime-state round trips for each durable category;
- purge lifetime across save, disconnect, expiry, and reload;
- expired temporary pets not loading;
- nested pet equipment and inventory;
- transaction rollback after each simulated query failure;
- migrations from every supported prior `pet_data` schema;
- player rename or stable-ID ownership behavior.

## Final assessment

Chronicles of Krynn is useful as a design reference for named follower
categories, summoner-specific capacity, purge-timer persistence, and additional
eidolon or golem features. Luminari is the better base for state fidelity,
validation, defensive loading, and existing focused tests.

The safe path is selective synthesis, not synchronization: first repair
Luminari's migration and atomicity risks, then introduce a tested central
category policy, and finally add explicit lifetime and durability rules. Feature
ports should follow that foundation so they do not deepen the current split
between relationship management, limits, persistence, and display.
