# Tactical effect clocks and hazard exposure

Assigned issue: #107. Status: short-defense, bleeding, billowing-cloud and
directional-wall pilots implemented; remaining migration and acceptance open.
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
- Directional walls are object sources placed on one authored side of an edge.
  Passability and damaging exposure have separate operation phases, described
  below. Missing creators use the wall's stored level and self-attributed damage.
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
| Blocking wall/trap | Check both sides and veto before placement | No crossing damage for a failed move | Only if explicitly authored |
| Damaging directional wall | Normal movement legality | Once per wall source and committed edge traversal | None by default |
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

## Defensive Casting pilot

`tactical_effects.c` owns the first migrated feature. A fresh activation in a
semantic encounter stores the following character turn serial and expires before
actions are announced. Outside combat one native character-owned expiry runs six
seconds after activation. The old proc_d20_round_one decrement is removed.

A pre-existing elapsed interval retains its exact remaining time when combat
starts. It is not stretched to the newly admitted turn boundary. A fresh cast
inside combat refreshes the feature using the subject-relative rule. Leaving
combat converts the next semantic deadline to one native residual interval.
Shutdown captures semantic residuals before destroying participant clocks.
Character removal pauses/cancels native expiry. Loading a saved character resumes
the saved interval through character_periodic_sync. A link-dead character still
in the world keeps its live clock; loss of connection cannot freeze its bonus. Native payloads resolve a
character generation and match the stored event ID before expiring the bonus.

PDCt now writes both the legacy display-round value and residual pulses. The
loader accepts old one-value records as full remaining rounds. A two-value record
with zero residual is expired and must not be restored as a fresh round. Runtime
event IDs, pulse deadlines and turn serials are never persisted. The deployment
help component is sql/components/help_defensive_casting_clock.sql.

Production-linked coverage includes exact native expiry, no old decrement,
expiry before semantic action, combat departure, paused elapsed time, legacy
player-file loading, residual save/load and expired save/load. Further lifecycle
and merge/re-entry acceptance, bleeding and recurring-save/hazard migration remain
open. This module deliberately contains no generalized effect scripting language.

Additional transition coverage verifies refresh through an old deadline, combat
entry and re-entry with an elapsed remainder, semantic shutdown capture, and
merged offset encounter clocks. A subject-relative defense follows that subject's
actual next semantic turn after merging; an elapsed residual remains an elapsed
interval. These are distinct policies, not two competing expiry callbacks.

## Bleeding Critical pilot

Ordinary ABILITY_BLEEDING_CRITICAL affects (APPLY_NONE, source_id zero, AFF_BLEED,
finite duration) now have one clock for damage and duration. Native expiry is
registered as tactical.bleeding-critical.tick; combat uses the subject's end
boundary after actions, guarded by character generation and participant liveness.
The old affect-duration traversal and AFF_BLEED damage traversal skip these nodes.
Other bleeding sources, including SKILL_BLEEDING_ATTACK's regeneration behavior,
retain their existing processing.

Each tick decrements the affect once, applies its stored damage using existing
self-attribution, and removes the effect after its final tick. Damage callbacks
may cure, replace or extract the subject; no affect pointer crosses the damage
call. A character-generation lookup and clock version distinguish the original
from a replacement. A due interval is charged before callbacks, so a save or
replacement callback cannot replay it.

Reapplication retains the pending deadline while adding damage and replacing
duration through affect_join. This avoids indefinitely postponing damage through
repeated applications. Curing and creating a new effect admits a new clock.
Native and semantic boundaries at the same deadline share one paid interval in
both callback orders. Leaving during an action preserves a due end tick as a
one-pulse native deadline; it does not postpone that tick for another round.

BlCt stores only the residual pulse interval; the existing affect record stores
remaining rounds and damage. Player serialization removes/restores affects, so
save_char_checked separately captures and restores the live clock policy and
boundary. Only the residual is written to disk, never runtime IDs/turn serials.
Loaded effects without BlCt begin with a full interval. Removed characters pause;
live link-dead characters continue. Registry detach now marks the character
non-live before owner removal, preventing refill from re-admitting a detached
character's effect owner.

Eleven added production-linked cases cover native and semantic ticks, departure,
removal/resume, cure/replacement during damage, stacking without tick delay,
equal-deadline callback orders, leaving during the action, and live/saved residual
preservation through real player serialization. Native registration allows one
queued event alongside a cancelled dispatching predecessor; generation/event IDs
prevent the predecessor from changing its replacement.

Admission exhaustion currently logs failure and retries through the existing
affected-owner callback. Its delay behavior and rejection telemetry need the
final capacity audit; the pilot's successful-admission tests do not prove that
gate. Recurring saves and movement-hazard source/exposure accounting remain open.

## Room-effect late-dispatch accounting

Room lifetime callbacks now account for every elapsed world-round boundary even
when the scheduler dispatches after the exact cadence pulse. Behavior callbacks
still run at most once per dispatch, so a late cloud cannot burst several hazard
checks at once. Expiry runs before behavior and an effect that expires during
catch-up cannot execute an overdue hazard action.

The last accounted world round is stored on each room-affect source. A new source
added while another source's room owner is overdue begins at its admission round
and does not inherit the older source's elapsed lifetime. Reindexing the room
owner preserves that source-local clock. This uses the existing native room owner
and adds no scheduler or world scan.

## Billowing Cloud exposure pilot

Each Billowing Cloud room affect now receives a process-local source identity.
The source owns a bounded list of character-generation exposure records. An
entry record is admitted and its next interval is marked before the save or
action consequence runs. If admission fails, the exposure is rejected and
counted rather than allowed to run repeatedly without accounting.

Committed CharacterMoved facts check only the destination room's active source
list. A new cloud checks characters already in that room. Leaving and re-entering
within the same six-second interval does not repeat a check, while distinct cloud
sources keep distinct interval budgets. Level 13 and higher remains ineligible.

Outside semantic combat, a character-owned native event handles the source's
next exposure. In combat, continued exposure runs after the subject's action at
an eligible turn end. The shared next-due deadline arbitrates native, entry and
semantic callbacks, including transitions in both directions. Expiry is also
checked from the source's world lifetime, so an encounter callback cannot act at
a deadline where the room owner has not yet dispatched the source's expiry.

Removing a source cancels its pending exposure events and releases every record.
Native payloads carry character generation, stable room vnum/generation and
source identity; no room-affect pointer crosses an asynchronous callback. The old
room-wide Billowing Cloud loop is removed. Other room hazards retain their legacy
behavior until migrated individually.

## Directional wall crossing pilot

Walking now checks blocking walls on both authored sides of an edge before
changing the character's room. A destination-side Wall of Force therefore vetoes
the operation directly; it no longer moves the character and then rolls the
placement back. A rejected move publishes no CharacterMoved fact and causes no
crossing damage.

Damaging wall behavior consumes the one committed CharacterMoved fact after all
entry decisions accept the final destination. WALK and FORCED relocations with a
real direction inspect the origin-facing and destination-reverse sides. Teleport,
spawn and other directionless relocation do not invent a crossed edge. Each wall
object's generation-safe entity handle is the source identity. The handler visits
each authored object once, so two independently placed walls can both apply while
one wall cannot apply twice for the same relocation.

Missing casters retain the stored wall level and use the subject as legacy damage
attribution in one mag_damage call. The prior branch invoked mag_damage once with
the subject and then evaluated a second null-caster call when the first was
nonfatal. The null call did not add damage, but it obscured exactly-once
accounting; the single explicit attribution path replaces it.

Production-linked cases cover distinct source objects on the two edge sides, a
destination blocking veto with no relocation exposure, forced crossing and a
source removed before movement. Continued exposure is absent by definition.
