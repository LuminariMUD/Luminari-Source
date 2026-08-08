# Project Considerations

**Last updated:** August 7, 2026

This document preserves design and maintenance lessons that should survive the
current backlog. It is the canonical destination for durable considerations
previously maintained in the retired workflow records.

## Special Procedure Architecture Refactor

**Status:** Phases 00-06 delivered; final source consolidation remains open

Durable behavior and evidence live in the
[Phase 00 validation matrix](testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md),
[Phase 01 gateway matrix](testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md),
[Phase 02 assignment matrix](testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md),
[Phase 03 ownership matrix](testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md),
[Phase 04 mechanics matrix](testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md),
[Phase 05 typed-handler matrix](testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md), and
[Phase 06 composition/lifecycle audit](testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md).
Only the remaining removal of the transitional top-level assignment source and umbrella header is
tracked in the [special-procedure todo](ongoing-projects/spec-todo.md).

### Active Concerns

#### Technical Debt

- **Compatibility handlers remain substantial:** Bank and Vampire Cloak dispatch typed context
  behind stable adapters, but 194 source-level legacy behavior implementations remain. Convert only
  when the handler benefits from typed targets, events, flow, or invalidation.
- **Imperative assignment inventory remains:** The two eligible Luminari rows are declarative and
  boot-validated. Numeric, computed, and campaign-compatibility rows remain on the observable direct
  path until they gain traced VNUMs and registered identities; preserve their exact order.
- **Shutdown leak baseline is incomplete:** Live ASan validation found existing process-lifetime
  allocations outside the health work; disabled leak detection is not proof of cleanup.

#### External Dependencies

- **MariaDB is mandatory:** Boot, production-linked tests, and readiness require a reachable
  MySQL/MariaDB service. Keep test databases isolated and never reuse credential-bearing `lib/`.
- **Production health activation is pending:** The local endpoint and rendered unit pass, but the
  approved production release still needs unit installation, restart, and a readiness probe.
- **Infrastructure coverage remains partial:** Health is delivered; security, backup, and deploy
  work still needs bounded production-safe validation when those surfaces change.

#### Performance and Security

- **Mutation-capable callbacks need lifetime contracts:** Cache iteration successors before
  invocation and never dereference owner, actor, or target pointers after a callback may extract
  them.
- **Diagnostics must stay non-authoritative:** Effective-binding allocation or formatting failure
  may log an error but must never suppress or alter the existing callback assignment.
- **Readiness runs in the game loop:** The ready route performs a synchronous database ping. Keep
  probes loopback-only and bounded, and monitor latency before increasing probe frequency.
- **Diagnostic inputs are content-controlled:** Continue rejecting control bytes and truncated
  output so world names and source locations cannot inject or disguise startup records.

#### Architecture

- **Three binding layers are distinct:** Immutable definition metadata, exact authored intent, and
  ordered effective provenance serve different purposes; the callback slot remains runtime truth.
- **Invocation semantics are caller-specific:** Exact tokens, traversal order, return handling,
  activation gates, wrapper nesting, and `no_specials` behavior must survive gateway extraction.
- **Moving rooms own the room callback slot:** A room `M` record and named `Z` procedure are
  mutually exclusive until relocation receives a separately specified typed hook.
- **Hard-coded room assignments remain a legacy exception:** They retain post-load precedence and
  can replace a moving-room callback. The `M` plus `Z` guard is not general callback arbitration.
- **Compatibility inventory is asymmetric:** The registry has 28 canonical definitions but 29
  indexed compatibility names because `Guildmaster` is an alias of canonical `Guild`.
- **Persistence remains single-handler:** Existing world syntax stores one authored name per
  prototype. Phase 06 found no second consumer for a general chain. Do not add one until ordering,
  duplication, invalidation, wrapper migration, OLC operations, and versioned loading are proven.
- **Lifecycle hooks stay owner-local:** DG Scripts cover localized content lifecycle behavior, while
  stateful artifact and vessel hooks call their owners directly. Do not create a general event
  registry before multiple consumers prove the same ordering and lifetime contract.

### Lessons Learned

#### Practices That Worked

- **Production-linked characterization:** Freeze real structures, callers, and writers before
  refactoring. The 78 dedicated tests expose compatibility that mocks would miss.
- **Process isolation for parser state:** Run each parser-backed scenario in a bounded child with a
  private sandbox because restoring public globals cannot reset private static counters.
- **Parent-owned cleanup:** Let the parent validate and remove exact sandbox paths so alarms,
  assertion exits, and parser termination cannot strand fixtures.
- **Allocate before release:** Build complete replacement or copy state before mutating owners;
  this kept authored and effective records transactional across prototype and OLC lifecycles.
- **Record provenance at assignment boundaries:** Capturing each callback write preserves legacy
  precedence and wrapper secondaries without converting behavior prematurely.
- **Preflight before mutation or output:** Whole-zone conflict validation before opening files
  prevents partial writes and moving-room state changes on rejected data.
- **Database-first help with a verifier:** Idempotent static SQL plus read-only content checks keeps
  authoritative staff guidance testable without touching production tables during validation.
- **Isolated CI runtime:** A guarded local MariaDB/world fixture made clean-checkout boot,
  sanitizer, coverage, and network tests reproducible without protected data.
- **Fail-closed operational probes:** Validate both status and bounded JSON content. Readiness,
  liveness, method rejection, graceful shutdown, and systemd rendering share executable evidence.
- **Dual-manifest assertions:** Exact Automake/CMake set comparisons catch stale consumers and keep
  every production and CuTest source buildable through both supported systems.

#### Practices To Avoid

- **Inferring contracts from names:** Trace every call site. Identical `SPECIAL` signatures hide
  different tokens, return semantics, activation gates, and post-callback lifetime risks.
- **Treating handler pointers as authored identity:** Boot overrides and wrappers change the
  effective pointer, so reverse lookup can silently corrupt persisted builder intent.
- **Flattening shop and quest wrappers:** Their saved secondaries and nesting order are observable
  behavior and must remain explicit until an intentional migration is specified.
- **Using global restoration as full isolation:** Private parser statics survive within a process;
  fresh child lifecycles are required for independent load/save/reload scenarios.
- **Writing before cross-field validation:** Reject incompatible room ownership before opening
  output or changing mover state, not after a partial serialization.
- **Borrowing argv as owned configuration:** The `-o` path must be duplicated before storage;
  freeing an argument-vector pointer caused an ASan-detected invalid free at shutdown.
- **Using production paths in validation:** Test setup must reject protected `lib/`, broad cleanup
  targets, non-loopback database hosts, and database names without a test/CI marker.
- **Redefining completion around transitional files:** Delivered runtime phases do not complete a
  source-ownership refactor while its explicitly targeted top-level assignment source and umbrella
  header remain. Keep the original structural finish line visible until deletion and full validation.

#### Tool and Library Notes

- **CuTest has executable-level granularity:** There is no per-function filter. Use focused child
  scenarios inside the production-linked suite and always follow `make test` with `make install`.
- **CMake tests are opt-in:** Fresh validation trees need `-DBUILD_TESTS=ON`; otherwise the
  production-linked test target is intentionally absent.
- **Sanitizers need live-path coverage:** The suite found memory issues, while a real startup,
  health request, and graceful shutdown found an argv ownership defect outside test-only paths.
- **World tooling consumes source structure:** Registry extractors must parse canonical arrays and
  aliases deliberately and fail closed when referenced initializers are missing.

### Phase 00 Resolutions

| Item | Resolution |
|------|------------|
| Unsafe sentinel registry | Replaced with 28 immutable, boot-validated definitions and bounds-safe typed accessors. |
| Unfiltered OLC procedure selection | Medit, oedit, and redit show only owner-compatible, builder-visible world bindings. |
| Authored identity reconstructed from callbacks | Owned authored records and authored-first writers preserve aliases, unknown names, and explicit clears. |
| Opaque boot precedence | Ordered effective records expose world, parser, legacy, shop, and quest contributions and final callbacks. |
| Moving-room and named-procedure collision | Loader, REdit, and writer boundaries reject shared room-slot ownership before mutation. |
| Clean-checkout CI runtime failures | Isolated fixtures repaired production tests, sanitizers, coverage, syntax boot, and network smoke tests. |
| Missing service readiness contract | Loopback health routes, a bounded probe, systemd startup enforcement, and CI smoke coverage are complete locally. |

## Vessel System

**Last updated:** August 2, 2026

**Status:** Gameplay layer implemented; production release not yet approved

Use [VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md) for current behavior and the
[Vessel System Product Requirements](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md)
for the product contract and authoritative release-gate state.

### Active Concerns

1. The complete current 500-ship workload and mechanical balance diagnostics
   pass, but the bounded process-memory result is not a long-horizon leak claim
   and automated characters are not real player balance evidence.
2. Broad release still requires structured human beta, player-data balance
   sign-off, and an authorized staff-to-cohort-to-public rollout with rollback
   authority and monitoring.

### Enduring Integration Rules

- Vessels extend the wilderness; they do not own a second geography. Terrain,
  bathymetry, weather, paths, regions, and dynamic rooms remain shared
  wilderness signals.
- The 6,000 dynamic wilderness rooms are a shared capacity. Fleet features must
  monitor pressure, reclaim rooms safely, and degrade without starving other
  wilderness users.
- Geographic names, territorial waters, encounter areas, and trade lanes belong
  in builder-authored regions or paths, not coordinate tables embedded in
  vessel code.
- Ship interiors may be generated, but the exterior wilderness coordinate
  remains authoritative. Z is altitude for airships and depth for submarines.
- Core C remains campaign-neutral. Campaign variation belongs in data and
  content.
- MySQL/MariaDB is required. Persistent vessel features need install, rollback,
  verification, fresh-database creation, and lifecycle coverage.
- A fleet slot has one canonical identity. Do not infer identity from a
  sentinel, object value, room pointer, and struct field independently.
- The cedit vessel setting gates the player/builder command surface and both
  heartbeat tick groups. Keep diagnosis and recovery commands available while
  the gameplay surface is stopped.
- Debug call sites compile out by default. Explicit development builds start
  with an empty runtime category mask and must be returned to the default build
  before release.

### Resource Budgets

| Resource | Current measurement or limit |
|---|---:|
| Base `greyhawk_ship_data` | 4,928 bytes |
| Maximum fleet | 500 ships |
| Base maximum-fleet storage | About 2.35 MiB |
| Base per-ship budget | At most 5 KB |
| Base `vehicle_data` | 152 bytes |
| Maximum vehicles | 1,000 |
| Complete vessel tick target | At most 25 ms at 500 active ships |

The fixed fleet memory and current complete-tick measurement are within budget.
Repeat the live gate after relevant behavior changes, and continue treating
bounded runtime-allocation evidence as `REPORT_ONLY`; see
[VESSEL_BENCHMARKS.md](testing/VESSEL_BENCHMARKS.md).

### Practices That Endure

1. Trace both legacy world-file and builder-spawn paths before declaring a
   behavior fixed. Similar names and fields do not prove shared identity.
2. Prefer data-driven hulls, room templates, routes, markets, encounters, and
   balance values so builders can create content without recompiling.
3. Keep the root CuTest suite production-linked. Standalone source mirrors age
   into false evidence and must not be recreated.
4. Treat automated tests, manual world tests, live performance, soak, and
   recovery as different forms of evidence; none substitutes for all the
   others.
5. Auto-create current tables for fresh databases, but retain explicit migration,
   verification, and rollback components for controlled deployments.
6. Use the unified transport boundary where vessel and vehicle behavior truly
   overlaps; keep class-specific navigation and interior behavior explicit.
7. Allocate optional route, automation, and encounter state only when needed.
8. Preserve the repository's GNU C23 style: block comments, declarations at the
   top of blocks, no variable-length arrays, two-space Allman formatting, and
   no mechanical restyling of legacy code.
9. Use named configuration and VNUM definitions. Never embed environment-specific
   VNUMs or edit local configuration headers as part of a general change.
10. Validate every pointer and array boundary and use bounded string functions.

### Implemented Capability Baseline

The maintained implementation includes wilderness navigation, multi-room
interiors, docking, routes and autopilot, land vehicles, vehicle loading,
builder prototypes, combat, ownership, permits, crew, upgrades, insurance,
cargo, markets, freight, piracy, weather hazards, encounters, staff tooling,
and MSDP vessel state.

That list describes implemented capability, not production readiness. Current
release state and the owned exit conditions are maintained in
[Vessel System Product Requirements](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md).

### Evidence Hygiene

- The July 26, 2026 production-linked vessel test and Valgrind results are
  historical snapshots. Rerun current gates after behavior changes.
- Older claims of a 1,016-byte ship structure, 353 current standalone tests, or
  a `test_runner` binary are obsolete.
- Older navigation-only stress figures may be retained as foundation history,
  but they do not prove the current all-subsystem 25 ms target.
