# Vessel System Considerations

**Last updated:** July 29, 2026

**Status:** Gameplay layer implemented; production release not yet approved

This document preserves design and maintenance lessons that should survive the
current backlog. Use [VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md) for current
behavior, [PRD.md](PRD.md) for the product contract, and
[VESSELS_TODO.md](project-management-zusuk/vessels/VESSELS_TODO.md) for
outstanding work.

## Active Concerns

1. The complete 500-ship workload has not been measured with combat,
   encounters, economy, wear, persistence, and client updates active. Historical
   navigation microbenchmarks are not release evidence.
2. The gameplay code needs a harbor sandbox, NPC shipping, balance simulation,
   a 72-hour soak, player beta, and migration rollback rehearsal before broad
   release.

## Enduring Integration Rules

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

## Resource Budgets

| Resource | Current measurement or limit |
|---|---:|
| Base `greyhawk_ship_data` | 4,856 bytes |
| Maximum fleet | 500 ships |
| Base maximum-fleet storage | About 2.32 MiB |
| Base per-ship budget | At most 5 KB |
| Base `vehicle_data` | 152 bytes |
| Maximum vehicles | 1,000 |
| Complete vessel tick target | At most 25 ms at 500 active ships |

The fixed fleet memory is within budget. The complete tick target and runtime
allocation behavior remain release gates; see
[VESSEL_BENCHMARKS.md](testing/VESSEL_BENCHMARKS.md).

## Practices That Endure

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

## Implemented Capability Baseline

The maintained implementation includes wilderness navigation, multi-room
interiors, docking, routes and autopilot, land vehicles, vehicle loading,
builder prototypes, combat, ownership, permits, crew, upgrades, insurance,
cargo, markets, freight, piracy, weather hazards, encounters, staff tooling,
and MSDP vessel state.

That list describes implemented capability, not production readiness. Current
limitations and the dependency-ordered path to release remain in
[VESSELS_TODO.md](project-management-zusuk/vessels/VESSELS_TODO.md).

## Evidence Hygiene

- The July 26, 2026 production-linked vessel test and Valgrind results are
  historical snapshots. Rerun current gates after behavior changes.
- Older claims of a 1,016-byte ship structure, 353 current standalone tests, or
  a `test_runner` binary are obsolete.
- Older navigation-only stress figures may be retained as foundation history,
  but they do not prove the current all-subsystem 25 ms target.
