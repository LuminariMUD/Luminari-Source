# Tactical effect clocks and hazard exposure

Assigned issue: #107. Status: implementation contract; not implemented or accepted.
Source inspection: fix/open-issue-repairs at d13732245, 2026-09-06.

## Existing behavior and integration points

- `affected_owners.c` gives affected characters and rooms native event owners.
  Character duration callbacks call `affect_update_character_one` once per
  PULSE_VIOLENCE. No replacement scheduler or whole-world scan is needed.
- `magic/magic.c:affect_update_character_one` decrements positive duration and
  removes an affect on a subsequent callback when duration is already zero.
  Thus duration 1 is not currently an exact one-round expiry contract.
- `combat/combat_encounters.c:run_semantic_round` expires readiness before
  incrementing the participant turn and restoring actions. It supplies the
  authoritative actor turn boundary. Participant counters are encounter-local;
  they cannot identify an effect across departure and re-entry by themselves.
- `limits.c:update_damage_and_effects_over_time_one` applies AFF_BLEED damage
  independently of affect duration. `char_regen` also handles SKILL_BLEEDING_ATTACK.
  Migrating only a decrement would leave a second behavior clock active.
- `limits.c` runs room hazards through room-affect behavior, sometimes with a
  temporary caster mobile. `SPELL_BILLOWING_CLOUD` already has a repeated Fortitude
  save and move-action consequence; it is a suitable recurring-save pilot.
- `magic/spells.c:check_wall` handles directional crossing damage and blocking.
  Walking checks the origin wall before relocation; movement entry checks the
  reverse direction at the destination. These are two possible wall instances,
  not inherently duplicate calls to one wall. Missing creators use stored level
  and self-attributed damage. A redundant subsequent null-caster call returns
  zero in mag_damage_scaled; it is not evidence of double damage or a crash.
- `players.c` writes affect values in a versioned text record. Runtime source_id
  is not written. New clock state needs explicit persistence, not struct dumps.

## Duration and phase rules

Keep three explicit clock policies. Existing effects remain legacy world-cadence
until their individual application, expiry, behavior and persistence paths migrate.

| Policy | Anchor | Use |
| --- | --- | --- |
| Subject-relative | Affected character's semantic turns | Defenses, bleeding, recurring personal saves |
| Caster-relative | Source character's semantic turns | Effects explicitly lasting until the caster's next turn |
| World/time | Elapsed scheduler time | Environmental lifetime and non-tactical durations |

A turn has ordered start, action and end phases. At start, expire start-bound
features before restoring actions and dispatching intent. At end, resolve the
subject's admitted recurring effects, then expire end-bound features. Revalidate
character identity after every callback capable of damage, removal or scripting;
end-phase work must not dereference a character extracted during its action.

A one-round defense lasts until the anchor's next turn start. If applied during
that start callback, it targets the following start, not the one in progress.
End-phase effects first run at the next eligible end after admission, at most once
for their admitted effect instance and phase token. Newly applied effects during
an end traversal wait until the following end. Never replay several damaging
turns in one late-dispatch callback; use the scheduler's existing run-once policy.

Use a character-lifetime monotonic phase identity, or an equivalently unique
phase token, rather than only an encounter round number. Merging encounters,
leaving and rejoining must not reset the last-serviced identity. Source-relative
listeners route to the exact source generation; a reused character address is
not the original caster.

Carry the next boundary's remaining pulses and subsequent full rounds when
switching between semantic and elapsed-time scheduling. An active semantic anchor
owns the clock; otherwise its existing native effect owner supplies six-second
boundaries. Preserve the residual interval through encounter admission, merge and
departure. No fresh full round on every transition and no concurrent native and
semantic callback for the same effect. On source loss, lasting effects retain
their remaining time on the world clock; source-required effects end explicitly.

Persist policy, remaining time/rounds, phase, and authored source identity where
needed. Reconstruct runtime handles on load. Ordinary saved personal durations
pause offline as today; environmental world deadlines continue. Never persist
raw pointers, runtime handles or process-relative absolute pulse values.

## Hazard exposure rules

Separate a hazard's lifetime from each subject's exposure. A live source instance
has a generation identity and immutable damage/save attribution captured at
creation. Sources surviving caster logout use that attribution and their stored
level/DC, rather than requiring a live caster or silently gaining victim stats.

| Hazard | Decision before movement | Committed movement | Continued exposure |
| --- | --- | --- | --- |
| Blocking wall/trap | Check/veto before placement | No new arrival damage for a failed move | Only if explicitly authored |
| Damaging directional wall | Crossing attempt, once per wall instance and traversal | Do not repeat the same crossing | None by default |
| Area cloud | Normal movement legality | First eligible entry in subject's exposure interval | Subject end phase while still inside |
| Tentacles | Existing grapple/escape decisions remain authoritative | Entry exposure | Subject end phase; same interval accounting |

Entry and continued exposure share one per-source/per-subject interval budget.
Entering repeatedly, forced movement and DG relocation cannot produce duplicate
entry-plus-end damage in that interval. A later interval may expose again. A
teleport has no crossed edge but can enter an area hazard. WALK/FORCED moves with
an actual direction can cross an edge; UNKNOWN must not invent one. Two distinct
hazard instances retain distinct budgets, subject to existing stacking rules.

Mark the exposure before invoking damage or DG callbacks. Resolve source and
subject generations again before any further work. A removed source has no future
exposure. Record entries only for active hazards and exposed subjects; retire them
with the source/subject or bounded expiry. Capacity exhaustion must be explicit
and tested; it must not silently admit an untracked repeatedly damaging source.

Committed CharacterMoved supplies location/cause/actor/direction. It currently
has no traversal ID. Add an operation identity only where needed to correlate
pre-crossing decisions with post-placement notifications; do not deduplicate on
room pair or pulse alone, since two real traversals may share both.

## Implementation sequence and acceptance

1. Add explicit phase integration and clock ownership using existing combat and
   affected owners. Migrate Wizard Defensive Casting (+4 AC for one round),
   including its saved timer and old decrement, as the short-defense pilot.
2. Migrate ABILITY_BLEEDING_CRITICAL duration and damage together. Preserve its
   initial save, damage amount and stacking policy. Exclude migrated nodes from
   the old AFF_BLEED callback. Do not accidentally migrate SKILL_BLEEDING_ATTACK
   without reconciling its separate regeneration behavior.
3. Give billowing cloud a source identity and subject exposure accounting; move
   its recurring save to the subject end boundary and remove its old room-wide
   behavior callback. Preserve its eligibility and consequence rules.
4. Integrate crossing/entry policies with committed relocation, then cover walls
   and tentacles without moving blocking decisions behind an event publication.
5. Update flat help, development database help and deployment SQL for changed
   rules. Document diagnostics and persist/load clock state explicitly.

Tests must prove phase order, exact expiry, no legacy double decrement/damage,
equal deadlines, delayed dispatch, movement/re-entry/forced moves, missing sources,
participant extraction during callbacks, encounter merge/exit/re-entry, bounded
admission cleanup, logout/restart and exactly-once exposure. Run the production-
linked suite and install after testing. This contract is not completion evidence.

Plan ablation: reuse native owner admission, cleanup and generation resolution;
add no separate timer service, global effect scan or general scripting language.
Only migrated pilots acquire the new clock metadata. Keep legacy behavior outside
those pilots until its own authoritative paths are reconciled.

## Turn-clock integration foundation

`combat_encounter_get_turn` exposes a managed character's lifetime turn serial,
remaining pulses to its next turn, and whether that character is currently
resolving a turn. During dispatch the next boundary is the following six-second
turn, including when the current event arrived late. Outside dispatch an overdue
boundary reports zero remaining pulses. An unmanaged character has no semantic
snapshot. Pair the serial with a character generation; the serial alone is not a
world-global identity. Saturation makes the snapshot unavailable rather than
wrapping and reusing an old phase identity.

The serial resides on char_data and survives membership replacement and merging.
New mobile instances reset it after copying their prototype. This is runtime
identity, not a player save field. The snapshot is read-only and owns no event;
effect duration and behavior migration still need to consume it.
