# Event-Driven Core Native AI Events

**Status:** Accepted 2026-09-01
**Date:** 2026-09-01
**Branch:** `event-driven-core-refactor`
**Scope:** Native AI delivery/retry timers and worker ingress

## Delivered Slice

AI provider workers no longer traverse game entities, mutate the event system,
or write the response cache. They submit immutable owned results to a bounded
256-entry mutex-protected inbox and signal the main reactor through a
nonblocking wake pipe. The main thread drains that inbox before timed dispatch,
updates cache state, validates generation-safe player and NPC handles, and
admits the timer.

The normal scheduler registers two owner-required semantic types:

- `ai.response.delivery` preserves the prior one-pulse deferred delivery;
- `ai.request.retry` preserves the one-second initial delay and bounded
  exponential retry backoff.

Both use the initiating player as owner when present, so player extraction
cancels pending AI work through the same owner lifecycle as other character
events. Response delivery resolves both live character generations again and
requires them to remain in the same loaded room. Provider workers are tracked,
serialized around their current shared provider state, and joined before AI
state is freed during normal shutdown or copyover.

The physical legacy backend retains two localized terminal-cleanup adapters.
They are not entered by the normal scheduler path and leave with the externally
gated rollback backend.

## Diagnostics

- `eventdebug player <name> [limit]` shows player-owned AI timers.
- `eventdebug type ai. [limit]` shows all live AI timer types.
- `eventdebug` includes bounded AI ingress depth, capacity, high-water,
  accepted/processed counts, rejections, and wake/schedule failures.
- Prompts, generated responses, cache keys, and backend payload data remain
  redacted.

## Validation Contract

- Worker submission does not increase scheduler depth until the main thread
  drains ingress.
- Native and rollback backends expose identical semantic names and owner kinds.
- Failed admission, ingress rejection, cancellation, normal completion, and
  shutdown release each compound payload exactly once.
- Shutdown stops worker admission, joins all detached provider workers, rejects
  late submissions, drains ingress ownership, and only then frees shared AI and
  CURL state.
- Copyover applies the same worker/ingress teardown and reinitializes the AI
  service if `exec` fails and the old process continues.

The production binary and all 1,051 production-linked CuTests passed locally on
2026-09-01. Live-MUD validation was intentionally deferred because local test
servers remain down at maintainer request.
