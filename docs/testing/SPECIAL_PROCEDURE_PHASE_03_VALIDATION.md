# Special Procedure Phase 03 Validation

## Purpose

This document is the durable acceptance and test-ownership record for Phase 03:
Behavior-Preserving Content Extraction. It maps the phase exit criteria to the completed ownership
split, compatibility checks, build manifests, checkpoint evidence, and reproducible validation
commands.

Phase 03 was completed and verified on 2026-08-07. Phases 00-02 remain the registry, invocation,
binding, and assignment prerequisites. The detailed move history is retained in
[Phase 03 Progress](SPECIAL_PROCEDURE_PHASE_03_PROGRESS.md).

## Delivered Boundary

- The complete 12,212-line `src/spec_procs.c` baseline is split among general procedure modules,
  cohesive zone packages, and the character, clan, craft, magic, object, quest, and vessel systems
  that own its non-procedure behavior.
- The complete 4,202-line `src/zone_procs.c` baseline is split into cohesive zone packages under
  `src/spec/`. The legacy file is retired rather than retained as an empty dispatcher.
- General mobile, object, and room callbacks use focused `src/spec/spec_mobiles.c`,
  `spec_mobile_archetypes.c`, `spec_objects.c`, and `spec_rooms.c` owners.
- Zone implementations retain their private helpers, encounter state, relative-VNUM calculations,
  assignment loops, and behavior order. Each zone publishes a dedicated owner header where another
  source needs its API.
- The private `zone_yell()` helper and its Fire Plane, Water Plane, and Snake Pit consumers share
  `src/spec/spec_zone_alarm_group.c`. Their separate public headers preserve zone ownership without
  exporting the helper.
- `src/spec_procs.h` remains a compatibility include surface for older consumers. New and updated
  sources include their true owner headers directly.
- Every source addition and both legacy-source removals appear identically in `Makefile.am` and
  `CMakeLists.txt` for production and production-linked CuTest builds.

Phase 03 changes no callback ABI, invocation gateway, binding source, registry identity, authored
world data, assignment VNUM, boot order, scheduling point, traversal rule, player command, or
runtime behavior. It introduces no shared-mechanic redesign and converts no callback to a typed
handler.

## Acceptance Matrix

| Exit Criterion | Evidence | Result |
|----------------|----------|--------|
| Extract general object procedures after gateway coverage. | Checkpoints 1-2 moved reusable objects and routed feature-specific objects to their true owners. | PASS |
| Extract reusable mobile and room procedures. | Checkpoints 5-6 established general mobile, room, archetype, guild, clan, magic, and vendor owners. | PASS |
| Split `zone_procs.c` by cohesive package. | Checkpoints 7-13 moved every package intact and retired all 4,202 baseline lines. | PASS |
| Move vessel, vendor, crafting, and ability work to true owners. | Checkpoints 1-4 place each block under its traced primary subsystem. | PASS |
| Preserve exported names, ABI, private state, initialization, and behavior. | Per-checkpoint exact-body comparisons, a final complete exported-symbol comparison, and production-linked tests all pass. | PASS |
| Keep both build systems synchronized. | Final manifest comparison adds `spec_zone_alarm_group.c` and `spec_zone_menzoberranzan.c` and removes `spec_procs.c` in both manifests. | PASS |
| Retire the remaining legacy monolith safely. | Checkpoints 14-31 moved every compiled callback; the final residual audit found only dormant commented blocks before deletion. | PASS |
| Keep documentation aligned. | PRD status, developer guide, architecture guide, master index, checkpoint record, and this acceptance record describe the final boundary. | PASS |

## Compatibility Evidence

Each checkpoint compared the moved implementation text with its parent checkpoint and rebuilt before
continuing. The final batch preserves 310 legacy helper, state, and callback lines exactly. The
complete global defined-symbol table from the final production binary is identical to the installed
Checkpoint 30 binary: no symbol was added, removed, or retyped. In particular, `zone_yell()` and
`gr_stalled` retain internal linkage.

Direct assignments remain in `src/spec_assign.c` with the same callback and VNUM expressions. No
final-batch callback has a definition in the special-procedure registry or a named authored world
binding. Existing registry, binding, gateway, combat, vessel, assignment, world-loading, and OLC
tests therefore continue to exercise the same production callbacks through the same boot and
dispatch paths.

## Legacy Source Retirement Audit

After the final six compiled callbacks moved, the remaining `src/spec_procs.c` content was audited
before deletion:

- `guild_golem` had only a stale declaration; both its historical assignment and implementation
  were commented and no linked definition existed;
- the commented `guild_guard` body was superseded by the active owner in
  `src/character/guild_services.c`; and
- the Emporium section was wholly commented and contributed no symbol or assignment.

Removing those blocks changes neither the preprocessed program nor its global symbol table. Git
retains the historical source. The active compatibility header remains because multiple consumers
still rely on its aggregated owner includes and legacy declarations.

## Test Ownership

Phase 03 adds no new behavior contract, so it does not manufacture isolated tests for copied bodies.
Its acceptance depends on the production-linked characterization established in Phases 00-02 and
the existing subsystem suites. The root suite contains 574 passing tests. The independent CMake
matrix contains 12 passing targets, including production CuTest, world tooling, documentation,
supervision, installation, health, and vessel regressions.

## Reproducible Validation

### Autotools Production Gate

```sh
make clean
make -j"$(nproc)"         # no -Wall -Wextra warnings
make test                 # OK (574 tests)
make install              # installs bin/circle and removes root circle
test -x bin/circle
test ! -e circle
```

### Independent CMake and CTest

Use a fresh directory outside the source tree:

```sh
phase03_cmake_dir=$(mktemp -d /tmp/luminari-spec-phase03-cmake-XXXXXX)
case "$phase03_cmake_dir" in
  /tmp/luminari-spec-phase03-cmake-*) ;;
  *) exit 1 ;;
esac
cleanup_phase03_cmake() {
  rm -r -- "$phase03_cmake_dir"
}
trap cleanup_phase03_cmake EXIT

cmake -S . -B "$phase03_cmake_dir" \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$phase03_cmake_dir" --target circle cutest --parallel "$(nproc)"
ctest --test-dir "$phase03_cmake_dir" --output-on-failure
cleanup_phase03_cmake
trap - EXIT
```

### Manifest, Source, and Symbol Integrity

```sh
test "$(rg -n 'spec_zone_alarm_group.c' Makefile.am CMakeLists.txt | wc -l)" -eq 2
test "$(rg -n 'spec_zone_menzoberranzan.c' Makefile.am CMakeLists.txt | wc -l)" -eq 2
test "$(rg -n 'src/spec_procs.c' Makefile.am CMakeLists.txt | wc -l)" -eq 0
test ! -e src/spec_procs.c
git diff --check
```

For a refactor-only checkpoint, capture `nm -g --defined-only` from the installed parent binary and
the rebuilt candidate, normalize each row to its type and symbol name, sort under `LC_ALL=C`, and
require an empty `diff -u`. Phase 03 close produced an empty diff against Checkpoint 30.

## Acceptance Rule

Phase 03 is accepted when every legacy monolith responsibility has a coherent traced owner, private
state remains with its consumers, both build systems contain identical source membership, the
callback and symbol contracts remain unchanged, all production-linked gates pass, and documentation
describes the resulting boundary. All held on 2026-08-07.
