# Crafting on the native event runtime

Reachable materials-and-motes creation, mode-2 golem construction and supply-
order work use PRIMARY_ACTIVITY_CRAFT in the existing primary activity manager.
They share the same native event_runtime as casting and camp; there is no
separate scheduler or descriptor scan for those paths. Only admitted work has a
scheduled owner. `activity` and the existing event diagnostics expose its
semantic name and progress. `activity pause`, `resume` and `cancel` use the
manager's existing controls.

The source also has primary-activity adapters for motes refining, resizing,
surveying and old node harvesting, but no registered command dispatches to
those static handlers. The registered wilderness category-harvest path uses
PRIMARY_ACTIVITY_HARVEST. The older `crafting` catalog uses its own eCRAFT mud
event, and crafting-kit work uses eCRAFTING.

Each admitted primary-activity owner advances in one-second steps, matching the
prior countdown semantics without examining idle players. Completion invokes
the existing crafting routine once, after the manager releases the activity.
The dormant resize adapter retains the generation identity of its admitted
object and cannot silently finish on a replacement with the same prototype.

Work requires hands and attention. Informational and unrelated commands follow
the activity manager's capability rules. Committed relocation, combat, damage,
invalid targets or a failed station/tool recheck cancel work. The synthetic
owned-craft test uses the unreachable survey adapter and proves that a
provisional move rolled back by an entry script does not cancel that adapter;
it is not coverage of a reachable surveying command. Project material
reservations remain with the project; existing reset/refund and completion
routines retain responsibility for their accounting.

Normal equipment selection does not initialize its completion skill. A first
start and its activity recheck therefore require no station; after an ordinary
completion failure stores the skill, a retry requires that skill's station.
Equipment tools are checked at admission but are not rechecked during the
timer. Category harvesting does recheck its harvest tool.

Offline time does not advance crafting. Loss of the descriptor retires the
active timer while preserving CrDu, the saved number of seconds remaining.
Production login, reconnect and copyover paths call `resume_craft_activity()`,
which attempts to reconstruct an owner from the saved method and positive
duration. Ordinary creation and supply-order work then use their normal
rechecks. Golem type, size and selected concrete material are not persisted, so
automatic golem resume reaches completion without required state, refuses the
result and retains materials. Load-time resize handling refunds its resources
and clears its method and duration before reconstruction. Idle players and
finished/cancelled projects receive no craft timer. If native admission fails,
no work is completed and the project state is retained.

Supply offers have a different policy: their existing timestamps measure wall
clock time, including offline time. Listing available offers or asking for
supply timing refreshes eligible empty slots lazily. Active offers and slot
cooldowns retain their existing rules. No per-player refresh timer is needed.
The earlier inventory's claim of online-only refresh accounting was incorrect.

Validation includes generic native scheduling without descriptor-list
membership, offline suspension/resume through a directly called helper,
cancellation after scripted relocation, and lazy refresh preserving existing
offers. Existing activity-manager tests cover single-primary admission,
capability checks, cancellation and owner lifecycle. No direct test covers the
reachable equipment admission/tool/station/roll path or the timed create,
material-golem and supply-order lifecycles through their production commands.
