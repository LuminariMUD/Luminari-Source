# Crafting on the native event runtime

The newer crafting system uses PRIMARY_ACTIVITY_CRAFT in the existing primary
activity manager. It shares the same native event_runtime as casting and camp;
there is no separate scheduler or descriptor scan. Only admitted work has a
scheduled owner. `activity` and the existing event diagnostics expose its
semantic name and progress. `activity pause`, `resume` and `cancel` use the
manager's existing controls.

Create, refine, resize, golem construction, survey, harvest and supply-order
work keep their existing durations and progress messages. Each active owner
advances in one-second steps, matching the prior countdown semantics without
examining idle players. Completion invokes the existing crafting routine once,
after the manager releases the primary activity. Resize retains the generation
identity of its admitted object and cannot silently finish on a replacement
with the same prototype.

Work requires hands and attention. Informational and unrelated commands follow
the activity manager's capability rules. Committed relocation, combat, damage,
invalid targets or missing required stations/tools cancel work. A provisional
move rolled back by an entry script does not cancel surveying. Project material
reservations remain with the project; existing reset/refund and completion
routines retain responsibility for their accounting.

Offline time does not advance crafting. Loss of the descriptor retires the
active timer while preserving CrDu, the saved number of seconds remaining.
Login, reconnection and copyover reconstruct an owned activity from that state.
The existing load-time resize reimbursement/reset policy remains unchanged.
Idle players and finished/cancelled projects receive no craft timer. If native
admission fails, no work is completed and the project state is retained.

Supply offers have a different policy: their existing timestamps measure wall
clock time, including offline time. Listing available offers or asking for
supply timing refreshes eligible empty slots lazily. Active offers and slot
cooldowns retain their existing rules. No per-player refresh timer is needed.
The earlier inventory's claim of online-only refresh accounting was incorrect.

Validation includes native scheduling without descriptor-list membership,
offline suspension and resume, cancellation after scripted relocation, and
lazy refresh preserving existing offers. Existing activity-manager tests cover
single-primary admission, capability checks, cancellation and owner lifecycle.
