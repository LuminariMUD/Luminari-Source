# Scoped Domain Subscription Acceptance

This gate covers runtime listeners attached directly to typed entity topics.
The implementation must not introduce a feature heartbeat, scan the character
list, or maintain a parallel ready-action routing table.

## Core acceptance cases

1. Two listeners on the same room-entry topic both receive one
   `CharacterMoved` publication. Removing one listener does not consume the
   publication or suppress the other.
2. A listener for room A receives movement into room A, but not movement into
   room B. A listener using an older generation of the same room runtime ID
   receives neither.
3. Wildcard listeners and exact-topic listeners are independently delivered at
   most once per publication, in deterministic priority and registration order
   within their topic.
4. One-shot listeners are detached before callback entry. A nested publication
   from that callback cannot call the same listener again.
5. A callback may cancel itself, another listener, or every subscription owned
   by an entity. Cancelled records are not delivered later in the same causal
   chain, and each context cleanup runs exactly once.
6. Owner extraction uses the owner index to remove every owned subscription.
   Reusing the owner's runtime slot with a newer generation does not expose the
   replacement entity to stale callbacks.
7. Total, per-owner, and per-topic capacity failures are explicit and leave no
   partial subscription behind.
8. Subscription inspection exposes identity, type, topic role and entity,
   owner, flags, calls, and timing without exposing callback payloads.

## Ready-action acceptance cases

1. `ready <command> on entry` subscribes the character's action to the current
   room's destination topic. Any other character entering schedules the exact
   command through the normal interpreter one pulse (about 100 ms) later. It
   does not wait for a semantic combat round.
2. `ready <command> on entry <target>` remains armed when a nonmatching entity
   enters and fires only when a matching player name or mobile keyword enters.
3. The owner entering, leaving, dying, extracting, logging out, or explicitly
   using `ready cancel` removes the action and all three of its subscriptions.
4. Several characters may ready actions in one room; all matching actions fire.
   Triggering one action cannot consume `CharacterMoved` for the others.
5. The delayed callback revalidates the owner and original room generation.
   If either is stale or the owner moved before execution, no command runs.
6. The executed text re-enters `command_interpreter`, retaining ordinary
   position, permission, trigger, activity, action-cost, and queue checks.
7. `eventdebug` can list subscriptions globally or for a selected player,
   mobile, object, or room in lines readable at 80 columns and capped at 120.
8. Ready actions are intentionally ephemeral across copyover. Shutdown cleanup
   removes them exactly once; players must arm them again afterward.

## Validation evidence

- The production-linked CuTest suite passes all 1,060 tests, including two
  independent listeners, stale topic generations, nested one-shots,
  cancellation during dispatch, all capacity boundaries, generation-aware
  owner cancellation, and end-to-end ready execution through the interpreter.
- All 19 clean CMake/CTest targets pass, including the demand-driven,
  native-event, and retired-PubSub architecture contracts.
- Valgrind reports zero errors and zero definite, indirect, or possible leaks
  while running all 1,060 tests.
- The help migration is idempotent and all nine combat, initiative, and ready
  entry/keyword verification queries pass against the local development
  database.
- A copied-production-world session armed `say READY_MATCHED` for `man`, showed
  all three listeners through the player filter and only `ready.entry` through
  the room filter, ignored nonmatching Ian, remained armed, then fired when the
  young man entered. The sanitized transcript is retained under
  `.ci-runtime/scoped-subscriptions-live/ready-session.log`.

That isolated full-world process later encountered a separate existing combat
fault in `handle_successful_attack()` at `src/combat/fight.c:13023`, after the
ready workflow completed. The faulting quivering-palm path does not include
the scoped-subscription, movement-publication, ready-action, or diagnostic
implementation and is not changed by this tranche.
