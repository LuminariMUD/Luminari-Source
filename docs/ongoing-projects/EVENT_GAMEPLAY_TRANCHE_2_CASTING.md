# Event gameplay tranche 2: casting activities

Branch: `refactor/fight-combat-safety`

## Gameplay change

Timed player and NPC casts now belong to the native primary activity manager.
Players can inspect them with `activity`, and cancel with `abort` or
`activity cancel`. Casts cannot be paused. Combat entry does not change their
existing pulse timing or charge another action for each progress step.

Actual positive `CharacterDamaged` events request concentration checks while a
cast is active, including outside combat. The check uses the existing spell
minimum-level difficulty and feat/class/situational modifiers, plus 10 and the
committed damage amount. Difficulty saturates at INT_MAX. This is an explicit
local gameplay rule, not a change to fifth-edition sustained concentration.
Alchemist and shadowdancer concentration exemptions remain. Deafness retains
its initial casting check rather than being rerolled on damage or progress.

The initial concentration check remains; progress alone no longer requests
random concentration checks. Preventing damage therefore protects a caster,
while landing damage can interrupt a spell before it resolves. A successful
damage check does not restart or delay the cast. Zero damage does nothing.

## Ownership and ordering

- `cast_spell` admits all timed PC spells/extracts/powers and NPC spells through
  `start_casting_activity`. The activity's ID and character generation identify
  its lifetime. Existing casting fields hold spell parameters, not a second timer.
- `activity.primary.step` owns the only casting deadline. PCs initially wait one
  second and NPCs two seconds; subsequent steps retain the former 10-pulse delay.
  Quick Chant, Quick Mind, psionic focus and time-stop completion rules remain.
- `ActivityTransitioned` now carries `activity_id` and `end_reason`, and routes
  on the actor's SUBJECT topic. `PRIMARY_ACTIVITY_CASTING` distinguishes casts.
  NONE -> ACTIVE is started; ACTIVE -> COMPLETED/CANCELLED is terminal. These
  are synchronous facts, not an interrupt or counterspell decision window.
- Cancellation detaches the activity and cancels its timer before clearing the
  casting fields. External reset calls from combat, spells, position changes,
  extraction and staff commands therefore cannot leave a pending cast behind.
- Completion detaches the activity before resolving magic. Damage or movement
  caused by the completed spell cannot interrupt that same completed activity.
- Movement of the caster cancels. Character target movement gets a scoped
  recheck, preserving world-target spells; target death or extraction cancels.
  Object destruction publishes EntityExtracted before freeing object data.
- DG script damage, direct-damage feats, invention explosions and the audited
  special-procedure damage writers use `combat_apply_raw_damage`. It publishes
  actual committed HP loss, respecting existing nonlethal floors, without
  adding resistance, combat admission or a different death policy. HP costs,
  rage/temporary-health removal, overheal decay and terminal death assignments
  remain distinct from damage. The core damage path already publishes and is
  not routed through this helper, preventing duplicate notifications.
- Position and disabling conditions are revalidated before progress/resolution;
  existing position-change interruption hooks remain immediate. Existing still
  spell exceptions remain, but a dead caster cannot finish a cast.
- Shutdown/copyover and character disposal cancel transient activities. There
  is no restoration of an in-flight cast after reconnect/copyover.

Damage committed before a due casting callback cancels that callback on a failed
check. If casting resolution has already begun, subsequent damage cannot undo
it. One positive damage fact causes one check: misses, attempted attacks and
progress notifications are not damage facts.

## Resources and deliberate synchronous paths

Prepared-spell extraction, PSP expenditure, NPC spell-slot accounting, action
costs and completion-only perks retain their existing call sites. Interruptions
and cancellation do not refund resources already spent. A second cast is
rejected before the cast entry point consumes another prepared spell; the
player command also rejects an occupied activity before spending PSP.

Instant PC spells, staff instant casts, commanded pet/eidolon spells through
`handle_npc_cast`, and NPC `manifest_power` retain synchronous resolution and have no timed activity or interruption period. Direct `call_magic`
uses (items, innate abilities, scripted effects and already-completed casts) are
also synchronous effect resolution, not an alternative casting scheduler.
`manifest_power` and `cast_spell` reject a currently occupied primary activity.

The former numeric casting event ID remains reserved as `eRETIRED_CASTING` so
other event IDs do not shift. It has no handler and no registered timer type.
The architecture guard rejects reintroducing `eCASTING` or `event_casting`.

## Migration audit

| Former path | Current owner/contract |
| --- | --- |
| `event_casting`, `NEW_EVENT(eCASTING, ...)` | Removed; native primary activity timed step |
| `char_has_mud_event(..., eCASTING)` admission/position checks | Casting state plus activity admission |
| `resetCastingData` callers | Cancel casting activity first; clear fields after detachment |
| Casting progress concentration rolls | Removed; initial check and damage-triggered checks |
| Character target pointers | Activity generation handle plus scoped movement/death and extraction cancellation |
| Object target destruction | EntityExtracted before object data is freed |
| Direct script/special-attack HP damage | Commit helper publishes actual damage once |
| Activity lifecycle broadcasts | Actor-scoped routed facts with stable ID and reason |
| Instant magic and reactive damage FIFO | Intentional synchronous execution; no independent timer |

Crafting, self-buffing, transit, moving rooms, supplies and staff-event countdowns
remain tracked in `docs/systems/EVENT_MECHANISM_INVENTORY.md`. This tranche does
not claim those migrations are complete. Full counterspelling and readied
spell-start interrupts remain separate work requiring pre-resolution ordering
and explicit action costs.

## Validation

See `docs/testing/EVENT_GAMEPLAY_TRANCHE_2_ACCEPTANCE_2026_09_05.md` for final build,
regression and memory-check results. Tests cover production PC/NPC cast entry,
resource consumption, both outcomes of damage checks, final-deadline damage,
class exemptions, target movement/death/extraction, actor movement/extraction,
incapacitation, shutdown, recasting, and object destruction notification.
