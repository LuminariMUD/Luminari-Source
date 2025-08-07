## Unifying LuminariMUD Event Systems

Goal: consolidate the base DG event queue (generic timed events) and the higher-level MUD event layer (entity-scoped, table-driven) into a single, simple, clean system without reducing safety or features.

### 1) Current State (What we have)
- Core loop/queue: `dg_event.c/.h`
  - Priority multi-bucket queue, event processing every pulse, `event_create`, `event_cancel`, `event_process`.
  - Assumes non-MUD `event_obj` is heap-allocated and uses `free()` in `cleanup_event_obj()`.
  - Special-cases MUD events (`isMudEvent` flag) to call `free_mud_event()`.
- MUD layer: `mud_event.c/.h`, registry in `mud_event_list.c`
  - Entity-scoped lists (world, desc, char, object, room, region), attach/cleanup logic, registry-driven handlers and messages.
  - Extra safety: room/region VNUM copy/validate, guarded cancels, sVariables ownership, change duration/sVariables helpers.
- Direct base-API callers outside MUD layer: `dg_scripts.c` (wait), `ai_events.c` (AI response/retry) call `event_create()` directly.

Pain points
- Ownership and cleanup are implicit for “generic” events, explicit for MUD events via a flag and separate free path.
- Two cancellation/cleanup pathways complicate reasoning and testing.
- Useful MUD features (owner scoping, registry) aren’t available to generic callers without opting into the MUD layer.

### 2) Design Principles
- Single event core with explicit ownership and no hidden assumptions.
- One way to clean up: destructor callback per event instance; avoid flag-based branching.
- Preserve MUD layer ergonomics (registry, entity lists, helpers) atop the unified core.
- Maintain backward compatibility with thin wrappers, then remove after migration.

### 3) Unified Architecture (Target)
- Core event struct (queue-level)
  - Keep: function pointer, queue links, scheduling.
  - Replace `isMudEvent` with an explicit `destructor` callback (nullable) and an optional `context` struct pointer.
  - `cleanup_event_obj()` only calls `destructor(event_obj, context)` if provided; otherwise does nothing. No implicit `free()`.
- Event context model
  - Define a small `event_context` that can carry: owner type (WORLD/DESC/CHAR/ROOM/REGION/OBJECT), owner pointer (void*), and optional metadata.
  - MUD events use this context to manage entity-scoped lists and VNUM copies; generic events can pass NULL.
- MUD registry remains table-driven
  - No change to the registry semantics; handlers unchanged.
  - MUD attach/free paths become instance `destructor`s bound at scheduling time.

### 4) Public API (after merge)
- Core scheduling
  - `event_create_ex(func, event_obj, when, destructor, ctx)`
    - Schedules event; `destructor` is called exactly once (on cancel or completion) with `(event_obj, ctx)`.
  - `event_cancel(event*)`, `event_time(event*)`, `event_is_queued(event*)`, `event_process()` unchanged.
- MUD convenience
  - `new_mud_event(id, owner, sVars)` returns a MUD payload; internal to mud layer.
  - `attach_mud_event(mud_event, when)` wraps `event_create_ex` with the MUD-specific `destructor` and `event_context`.
  - Query helpers (`char_has_mud_event`, etc.) unchanged.
- Back-compat wrappers (temporary)
  - `event_create(func, obj, when)` → calls `event_create_ex(func, obj, when, NULL, NULL)`; no implicit free.
  - `cleanup_event_obj(event)` becomes a thin wrapper on the `destructor` path. Deprecate implicit `free()` with warnings.

### 5) Behavioral Guarantees
- No double-free: cancel during execution keeps the current guard semantics; destructor is invoked at most once.
- No implicit freeing: if a caller wants memory freed, they must provide a destructor.
- MUD room/region VNUM safety preserved: copy-before-free and existence checks live in the MUD destructor.

### 6) Migration Plan (Phased, safe)
Phase 0 — Prep
- Add docs and deprecation notes; enable compile-time warnings when `event_create()` is used without a destructor.

Phase 1 — Core enhancements (no behavior change)
- Add `event_create_ex`, `event_context`, and `event->destructor/ctx` fields.
- Make `cleanup_event_obj()` invoke `event->destructor` if set; remove implicit `free()` path. Keep `isMudEvent` temporarily for compatibility.
- Keep old `event_create` and `isMudEvent` working.

Phase 2 — MUD layer refactor to new core
- In `attach_mud_event()`, use `event_create_ex` with a MUD-specific destructor that:
  - Handles list removal, VNUM copy/validation, sVariables free, and payload free (current `free_mud_event` logic moved behind the callback).
- Stop setting `event->isMudEvent` in new codepaths; pass `ctx` with owner type/pointer.
- Make `free_mud_event()` callable as the destructor and as a direct helper (for legacy).

Phase 3 — Update call sites
- `mud_event.c`: switched in Phase 2.
- `dg_scripts.c`, `ai_events.c`:
  - Provide simple destructors for their payloads (if they own heap memory) or pass NULL if not needed.
  - Convert to `event_create_ex`. Leave `event_create` wrapper in place until all sites are migrated.

Phase 4 — Remove legacy branches
- Remove `isMudEvent` from `struct event` and all related branching.
- Simplify `cleanup_event_obj()` to “if destructor present, call it”.
- Remove implicit `free()` path; compilation fails if code still relies on it.
- Keep back-compat macro `NEW_EVENT` mapped to `attach_mud_event(new_mud_event(...))` (unchanged behavior).

Phase 5 — Validation & cleanup
- Audit all `event_create` usages via grep to ensure all are migrated.
- Run soak tests for: cancel during execution, bulk frees, room/region OLC edits, daily-use math.
- Remove deprecation warnings, finalize docs.

### 7) Tests and Instrumentation
- Unit/system scenarios
  - Schedule/cancel/complete with and without destructor.
  - Cancel during execution (ensure no double destructor call).
  - MUD room/region attach, OLC delete during active event, ensure no crashes.
  - Daily-use cooldown loops; overflow and clamp paths.
  - World/char/object list integrity after heavy churn.
- Instrumentation
  - Add logs for missing destructor at schedule time (warn level) during Phase 1.
  - Counters for destructor calls vs event lifecycle outcomes.

### 8) Risk Mitigation
- Back-compat wrapper maintains stability during migration.
- Destructor-first model confines ownership to the creator; removes hidden frees.
- Existing safety guards (queue overflow checks, processing guard) remain.

### 9) Work Breakdown (Actionable tasks)
1. Add `event_create_ex` and `event_context` to `dg_event.h/.c`; thread through processing and cancel paths; keep legacy API.
2. Refactor `cleanup_event_obj()` to use destructor; retain old branches behind a compile-time flag for a short window.
3. Implement MUD destructor wrapper (reusing `free_mud_event` logic) and switch `attach_mud_event()` to `event_create_ex` + context.
4. Migrate `dg_scripts.c` and `ai_events.c` to `event_create_ex` with appropriate destructors (or NULL if not needed).
5. Remove `isMudEvent` and the implicit-free path; update headers and all references.
6. Expand tests (unit + runtime) per section 7.
7. Update docs (`docs/systems/MUD_EVENTS.md`) reflecting the unified API.

### 10) Acceptance Criteria
- No `isMudEvent` in `struct event` and no implicit `free()` anywhere in event cleanup.
- All event creators either provide a destructor or intentionally pass NULL for non-owned memory.
- MUD behavior and messages unchanged; all existing events work.
- All current direct `event_create` sites migrated to `event_create_ex`.
- Soak tests run clean; no regressions in cancel/processing edge cases.

### 11) Simplicity Wins (Why this is “best”)
- One explicit ownership model (destructor) replaces split logic and assumptions.
- One core queue serves all events; MUD layer remains a thin ergonomic facade.
- Compatibility preserved during migration; end state is smaller, clearer, and safer.

