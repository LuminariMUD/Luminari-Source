# Considerations

> Institutional memory for AI assistants. Updated between phases via carryforward.
> **Line budget**: 600 max | **Last updated**: Phase 00 (2026-08-07)

---

## Active Concerns

Items requiring attention in upcoming phases. Review before each session.

### Technical Debt
<!-- Max 5 items -->

- [P00] **Legacy dispatch remains authoritative**: Definitions, authored state, and effective
  provenance are safe, but event gateways, typed contexts, and invalidation outcomes remain deferred.
- [P00] **Imperative assignments remain**: Legacy owner/VNUM assignments are observable but are not
  yet validated declarative data; preserve their exact order until Phase 02 migrates them.
- [P00] **Shutdown leak baseline is incomplete**: Live ASan validation found existing process-lifetime
  allocations outside the Health bundle; do not treat disabled leak detection as proof of cleanup.

### External Dependencies
<!-- Max 5 items -->

- [P00] **MariaDB is mandatory**: Boot, production-linked tests, and readiness require a reachable
  local MySQL/MariaDB service; keep test databases isolated and never reuse credential-bearing `lib/`.
- [P00] **Production health activation is pending**: The local endpoint and rendered unit pass, but
  the approved production release still needs unit installation, restart, and a readiness probe.
- [P00] **Infrastructure coverage remains partial**: Health is delivered; the Security, Backup, and
  Deploy bundles still need separate bounded transition runs and production-safe validation.

### Performance / Security
<!-- Max 5 items -->

- [P00] **Mutation-capable callbacks need lifetime contracts**: Cache iteration successors before
  invocation and never dereference owner, actor, or target pointers after a callback may extract them.
- [P00] **Diagnostics must stay non-authoritative**: Effective-binding allocation or formatting
  failure may log an error but must never suppress or alter the existing callback assignment.
- [P00] **Readiness runs in the game loop**: The ready route performs a synchronous database ping;
  keep probes loopback-only and bounded, and monitor latency before increasing probe frequency.
- [P00] **Diagnostic inputs are content-controlled**: Continue rejecting control bytes and truncated
  output so world names and source locations cannot inject or disguise structured startup records.

### Architecture
<!-- Max 5 items -->

- [P00] **Three binding layers are distinct**: Immutable definition metadata, exact authored intent,
  and ordered effective provenance serve different purposes; the callback slot remains runtime truth.
- [P00] **Invocation semantics are caller-specific**: Exact tokens, traversal order, return handling,
  activation gates, wrapper nesting, and `no_specials` behavior must survive gateway extraction.
- [P00] **Moving rooms own the room callback slot**: A room `M` record and named `Z` procedure are
  mutually exclusive until relocation receives a separately specified typed hook.
- [P00] **Compatibility inventory is asymmetric**: The registry has 28 canonical definitions but 29
  indexed compatibility names because `Guildmaster` is an alias of canonical `Guild`.
- [P00] **Persistence remains single-handler**: Existing world syntax stores one authored name per
  prototype; do not add chains until ordering, duplication, invalidation, and wrapper rules are proven.

---

## Lessons Learned

Proven patterns and anti-patterns. Reference during implementation.

### What Worked
<!-- Max 15 items -->

- [P00] **Production-linked characterization**: Freeze real structures, callers, and writers before
  refactoring; the 78 dedicated tests expose compatibility that mocks would miss.
- [P00] **Process isolation for parser state**: Run each parser-backed scenario in a bounded child
  with a private sandbox because restoring public globals cannot reset private static counters.
- [P00] **Parent-owned cleanup**: Let the parent validate and remove exact sandbox paths so alarms,
  assertion exits, and parser termination cannot strand fixtures.
- [P00] **Allocate before release**: Build complete replacement or copy state before mutating owners;
  this kept authored/effective records transactional across prototype and OLC lifecycles.
- [P00] **Record provenance at assignment boundaries**: Capturing each actual callback write preserves
  legacy precedence and wrapper secondaries without converting behavior prematurely.
- [P00] **Preflight before mutation or output**: Whole-zone conflict validation before opening files
  prevents partial writes and moving-room state changes on rejected data.
- [P00] **Database-first help with a verifier**: Idempotent static SQL plus read-only content checks
  keeps authoritative staff guidance testable without touching production tables during validation.
- [P00] **Isolated CI runtime**: A guarded local MariaDB/world fixture made clean-checkout boot,
  sanitizer, coverage, and network tests reproducible without protected data.
- [P00] **Fail-closed operational probes**: Validate both status and bounded JSON content; readiness,
  liveness, method rejection, graceful shutdown, and systemd rendering now share executable evidence.
- [P00] **Dual-manifest assertions**: Exact Automake/CMake set comparisons caught stale consumers and
  kept every new production and CuTest source buildable through both supported systems.

### What to Avoid
<!-- Max 10 items -->

- [P00] **Inferring contracts from names**: Trace every call site; identical `SPECIAL` signatures hide
  different tokens, return semantics, activation gates, and post-callback lifetime risks.
- [P00] **Treating handler pointers as authored identity**: Boot overrides and wrappers change the
  effective pointer, so reverse lookup can silently corrupt persisted builder intent.
- [P00] **Flattening shop and quest wrappers**: Their saved secondaries and nesting order are observable
  behavior and must remain explicit until an intentional migration is specified.
- [P00] **Using global restoration as full isolation**: Private parser statics survive within a process;
  fresh child lifecycles are required for independent load/save/reload scenarios.
- [P00] **Writing before cross-field validation**: Reject incompatible room ownership before opening
  output or changing mover state, not after a partial serialization.
- [P00] **Borrowing argv as owned configuration**: The `-o` path must be duplicated before storage;
  freeing an argument-vector pointer caused an ASan-detected invalid free at shutdown.
- [P00] **Using production paths in validation**: Test setup must reject protected `lib/`, broad
  cleanup targets, non-loopback database hosts, and database names without a test/CI marker.
- [P00] **Claiming future architecture as delivered**: Keep typed gateways, declarative assignments,
  shared mechanics, extraction, and composition labeled as deferred until tested implementation lands.

### Tool/Library Notes
<!-- Max 5 items -->

- [P00] **CuTest has executable-level granularity**: There is no per-function filter; use focused
  child scenarios inside the production-linked suite and always follow `make test` with `make install`.
- [P00] **CMake tests are opt-in**: Fresh validation trees need `-DBUILD_TESTS=ON`; otherwise the
  production-linked test target is intentionally absent.
- [P00] **Sanitizers need live-path coverage**: The suite found memory issues, while a real startup,
  health request, and graceful shutdown found an argv ownership defect outside test-only paths.
- [P00] **World tooling consumes source structure**: Registry extractors must parse canonical arrays
  and aliases deliberately and fail closed when referenced initializers are missing.

---

## Resolved

Recently closed items (buffer - rotates out after 2 phases).

| Phase | Item | Resolution |
|-------|------|------------|
| P00 | Unsafe sentinel registry | Replaced with 28 immutable, boot-validated definitions and bounds-safe typed accessors. |
| P00 | Unfiltered OLC procedure selection | Medit, oedit, and redit now show only owner-compatible, builder-visible world bindings. |
| P00 | Authored identity reconstructed from callbacks | Owned authored records and authored-first writers preserve aliases, unknown names, and explicit clears. |
| P00 | Opaque boot precedence | Ordered effective records expose world, parser, legacy, shop, and quest contributions and final callbacks. |
| P00 | Moving-room and named-procedure collision | Loader, REdit, and writer boundaries now reject shared room-slot ownership before mutation. |
| P00 | Clean-checkout CI runtime failures | Isolated fixtures repaired production tests, sanitizers, coverage, syntax boot, and network smoke tests. |
| P00 | Missing service readiness contract | Loopback health routes, a bounded probe, systemd startup enforcement, and CI smoke coverage are complete locally. |

---

*Auto-generated by carryforward. Direct edits allowed but may be overwritten.*
