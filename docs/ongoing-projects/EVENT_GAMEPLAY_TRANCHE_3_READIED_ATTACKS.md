# Tranche 3: tactical readied attacks

Status: implemented and validated. See the [acceptance report](../testing/EVENT_GAMEPLAY_TRANCHE_3_ACCEPTANCE_2026_09_05.md).

## Intended behavior

A player spends a standard action to prepare one normal attack. A matching
entry, door-open, or timed casting-start event releases that attack if the
actor and target are still eligible. Damage uses the existing concentration
check; readiness does not automatically cancel a spell. Full counterspelling,
instant-cast interruption, and ally-attacked triggers are outside this tranche.

The reservation expires at the actor's next semantic turn. Outside semantic
combat, a native six-second owner deadline bounds readiness. Cancellation and
expiry do not refund the action. Execution never charges a second action.

## Implementation contracts

- CastingStarted carries the committed activity ID, stable caster and target
  handles, source room, spell number and casting class. Spell metadata is
  internal; receiving it does not let a player identify the spell.
- Publication occurs after the casting activity is installed and scheduled.
  If an ActivityTransitioned observer cancels or replaces it, no stale start
  fact is published. Instant spells have no casting activity or start fact.
- Casting-start subscribers enqueue a native reaction, rather than executing
  damage on the casting setup stack. Execution must match the live cast ID.
- Timed casting's first callback is at least one second after admission.
  A one-pulse reaction must precede it, including when callbacks are overdue.
  The scheduler orders deadlines before insertion sequence. Production-linked
  tests advance both deadlines into the same overdue dispatch and verify that
  damage cancels the cast before its callback can complete.
- One normal strike bypasses the special-attack queue and the opening
  initiative/full-attack routines. Normal hit, damage, defenses and
  concentration remain authoritative.
- Revalidate position, incapacity, visibility, room/reach, weapon eligibility,
  PvP permission, and target lifetime at execution.
- Entry and door combat readiness must use the same reservation path. Raw
  commands and aliases must not provide a free combat-action bypass.
- Cancel subscriptions and deadlines on expiry, cancellation, movement,
  death, extraction and runtime shutdown. Recasting cannot reuse a queued
  reaction intended for a previous cast.

## Implemented interface and ownership

Players use `ready attack <caster> on casting`; no cast ID is displayed or
required. `hit` and `kill` are aliases for the same single normal attack.
Entry and door triggers use that same action reservation and execution path.
Casting and door targets bind at arming; entry targets bind to the matching
arrival. Combat begins normally if the strike engages a new opponent.

Noncombat readiness is restricted to explicit say, emote, look, rest, stand,
sit, open and close commands. These retain interpreter admission and normal
costs. Arbitrary commands and aliases are rejected, so queued spells, special
attacks and alternate combat commands cannot evade readiness accounting.

The actor owns at most one execution deadline and one outside-combat expiry
deadline. Entering semantic combat cancels the latter; leaving restores a
six-second expiry. The semantic turn hook cancels readiness before action
recovery and action dispatch. Nothing polls armed characters.

The bus forbids subscription changes during dispatch. Entry readiness therefore
subscribes to room departures at arming, and binds only the arriving character
handle during dispatch. Departure cancels immediately; death or extraction in
the one-pulse pending window is rejected when execution resolves that handle.
Casting and door targets have subject-scoped movement, death and extraction
subscriptions from arming. Owner extraction and shutdown destroy the scoped
subscriptions and cancel their native deadlines.

`combat_readied_attack_allowed` rechecks the actor's position, activity,
incapacity, visibility, melee weapon eligibility and room/single-file reach.
The normal hit path retains PvP and peaceful-room admission and defenses.
`resolve_hit` bypasses the special-attack queue only for a readied strike;
ordinary callers of `hit` retain their existing queue behavior.

## Validation

Production-linked tests cover arming costs across all three triggers, rejected
rearming without an available action, cancellation without refunds, native and
semantic expiry, watched target movement/extraction, caster/cast identity,
replacement casts, visibility loss, hit/miss/concentration outcomes, overdue
ordering and preservation of queued special attacks. Existing owner cleanup,
door invalidation and casting lifetime tests remain in the full suite.

READY help is updated in the flat file and the development database's existing
`ready-action` entry. Its aliases are retained and the prior body is saved in
`help_versions`. No production database was modified.

Full repository checks, CMake and Valgrind pass. The acceptance report records
the verification scope and the implementation limits.
