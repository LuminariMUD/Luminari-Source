# Phase 10 Activity Policy Matrix

**Status:** Accepted for implementation on 2026-08-31

This review fixes the initial activity semantics before code migration. The
one-primary-activity rule remains authoritative; capabilities describe what the
admitted activity occupies, not permission to run a second primary activity.

## Shared policy

| Input or event | Default response | Progress | Command result |
|---|---|---|---|
| Informational command | Ignore | Preserved | Runs immediately |
| Activity status/control | Ignore | Preserved unless explicitly cancelled | Runs immediately |
| Command with no claimed-capability conflict | Ignore | Preserved | Runs immediately |
| Incompatible command | Activity-defined reject/cancel/pause/delay/recheck | Activity-defined | Explicitly reported |
| Actor extraction or death | Cancel | Character-owned progress discarded | No completion |
| Target extraction/loss | Activity-defined, default cancel | Ownership policy applies | No stale target access |
| Wall-clock deadline in combat | Ignore | Preserved | Timer never grants an action |
| Semantic combat turn | Commit only if claimed actions are available | Advances once | Consumes existing actions |

`ignore`, `cancel`, `pause`, `delay`, `recheck`, and `reject` are explicit
manager outcomes. Every terminal path detaches one timer and one activity at
most once. Character-owned progress disappears on cancellation; target-owned
progress may be retained by a later migrated activity.

## First migration: establish camp

| Property | Accepted value |
|---|---|
| Target | Typed room handle |
| Progress | Progressive, three two-second steps |
| Progress owner | Character |
| Capabilities | Movement, hands, attention, standard action, move action |
| Traits | Stationary, distracted, hands occupied, obvious |
| Movement/room change | Cancel and discard progress |
| Damage | Delay the next step by two seconds; preserve progress |
| Combat entry | Pause; preserve progress |
| Combat exit | Resume from the preserved next-step delay |
| Target loss or failed validity recheck | Cancel without completion |
| Incompatible action command | Reject; status and explicit cancel remain available |
| Informational and communication commands | Ignore and run immediately |
| Explicit `activity cancel` | Cancel and discard progress |

The Survival result is determined when work begins but revealed only on
completion. This preserves the existing roll and action costs while making the
work interruptible. `LUMINARI_CAMP_ACTIVITY=legacy` restores the prior immediate
command during the Phase 10 rollback window.

## Command metadata

Command-table flags identify information, communication, movement, and
activity control. Unclassified commands conservatively claim attention. This
replaces command-handler busy allowlists with one central admission decision;
the metadata inventory can be expanded as later activities migrate.
