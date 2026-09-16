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
invalid targets or a failed recheck cancel work. The synthetic owned-craft
tests seed the unreachable survey adapter. One proves that a committed script
relocation cancels it and clears the saved remainder. The other proves that it
pauses offline and resumes through the helper. No craft test covers a move that
an entry script rolls back, or a reachable surveying command.

While craft work runs, `craft` subcommands that change the project are refused
(show, check and score still work), and completion makes an item only if the
project still passes `craft check`. While a supply order is held, the
subcommands that read or change the project are refused and golem construction
cannot start; `craft score` and the other golem commands still work. The recipe
variant's skill decides the project's tool, station, talents, roll and
experience; check, start, timer rechecks and completion all use that skill's
station. Equipment readiness and
admission check tool-slot occupancy (woodworking needs no tool); the timer does
not recheck the tool, but completion does. Category harvesting has no tool
requirement or tool recheck; a carried or worn harvest-tool prototype only
raises the quality floor at completion. Only the unreachable node-harvest
adapter rechecks `has_proper_harvesting_tool_equipped()`.

Offline time does not advance crafting. Loss of the descriptor retires the
active timer while preserving CrDu, the saved number of seconds remaining.
Production login, reconnect and copyover paths call `resume_craft_activity()`,
which attempts to reconstruct an owner from the saved method and positive
duration. Ordinary creation and supply-order work then use their normal
rechecks. Golem construction also saves its type, size and chosen wood
(`CrGo`), so resumed golem work can finish. Load-time resize handling refunds
its resources and clears its method and duration before reconstruction. Idle
players and finished/cancelled projects receive no craft timer. If native
admission fails, no work is completed and the project state is retained.

Supply offers have a different policy: their existing timestamps measure wall
clock time, including offline time. Selecting an offer or asking for supply
timing (`supplyorder cooldown`) refreshes eligible empty slots lazily;
`list` and `show` do not. Active offers and slot
cooldowns retain their existing rules. No per-player refresh timer is needed.
The earlier inventory's claim of online-only refresh accounting was incorrect.

Paid training contracts from a Craft Trainer also measure wall-clock time. A
contract stores only its end time; nothing runs while the character is away.
Selecting the character at the account menu after that time grants the
experience through gain_craft_exp() and clears the contract in one save, so
there is no timer and no scan. See docs/ongoing-projects/craft-trainers.md.

Validation includes generic native scheduling without descriptor-list
membership, offline suspension/resume through a directly called helper,
cancellation after scripted relocation, and lazy refresh preserving existing
offers. Existing activity-manager tests cover single-primary admission,
capability checks, cancellation and owner lifecycle. No direct test covers the
reachable equipment admission/tool/station/roll path or the timed create,
material-golem and supply-order lifecycles through their production commands.
