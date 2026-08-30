# Active World

The active-world manager replaces the global character-list scan with one
owner-scoped deadline for every NPC capable of autonomous behavior. It is the
first Phase 7 consumer of the scheduler and typed domain-event runtime.

## States

- **Active:** the NPC is in the world and may perform autonomous behavior.
  Exactly one owner-scoped thinking event is scheduled.
- **Cooling:** the NPC has just become ineligible for autonomous work. Its
  existing owner event receives a bounded final transition callback.
- **Dormant:** the NPC is outside the world, pending extraction, or explicitly
  marked `MOB_NO_AI`. It owns no thinking event.

Player presence is deliberately not an eligibility rule. Patrols continue,
ordinary mobs wander, hunters pursue targets, scripts and special procedures
run, and NPC factions can start or continue battles when no player is nearby.
This persistent simulation is gameplay: players may later encounter its
results, sneak past a battle, or join it.

`CharacterMoved` synchronously re-evaluates NPCs in only the origin and
destination rooms. `CombatStateChanged` re-evaluates only its two character
owners. `EntityExtracted` removes registry membership and cancels the owner's
event. No handler traverses `character_list` to discover work.

Scheduled deadlines are distributed across the six-second mobile interval.
This retains the old cadence without releasing every active NPC in one due
batch. A callback may reschedule only itself. Movement or extraction during a
callback is safe because in-flight cancellation wins over recurrence.

## Bounds and Diagnostics

At most 65,536 active plus cooling NPCs may own thinking events. Admission above
that limit fails closed, remains dormant, and emits a rate-limited warning.
The production legacy-adapter queue is bounded at 131,072 events, leaving equal
headroom for non-NPC owners. Dispatch callback/time budgets still apply
independently.

`perfmon entities` reports mode, active and cooling counts, the admission
limit, rejected admissions, and callback count. Scheduler callback telemetry
reports `active_world_mobile` cost and delay distribution. `perfmon reset`
starts a new window for both sets of counters without changing registry state
or scheduled deadlines.

## Rollback

Selection is immutable for one boot:

- `LUMINARI_ACTIVE_WORLD=active` is the default.
- `LUMINARI_ACTIVE_WORLD=legacy` restores `mobile_activity_pulse()`.

The heartbeat checks `active_world_enabled()` and never invokes the legacy path
while scheduled mode is active. Changing the variable requires a restart.

Each due NPC callback invokes the existing `mobile_activity` behavior for that
owner, preserving the old six-second semantics without rediscovering work by
walking `character_list`. Initial deadlines are spread across that interval so
the migration also removes the old synchronized burst.
