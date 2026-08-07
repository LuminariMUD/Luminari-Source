# Special Procedure Phase 04 Validation

## Purpose

This document is the durable acceptance and test-ownership record for Phase 04: Narrow Shared
Mechanics. It records the exact context, phrase, cooldown, combat, and affect contracts extracted
from real consumers after the Phase 03 ownership split.

Phase 04 was completed and verified on 2026-08-07. It does not introduce typed handlers, change the
legacy `SPECIAL` ABI, add a general callback chain, or alter world-file persistence.

## Delivered Boundary

- `src/spec/spec_context.c|.h` validates typed event payloads, exact worn-object identity, and live,
  colocated combat participants.
- `src/spec/spec_phrase.c|.h` performs opt-in canonical command plus exact phrase matching.
- `src/spec/spec_cooldown.c|.h` reads and commits legacy object-instance `spec_timer[]` cooldowns.
- `src/spec/spec_combat.c|.h` validates the current opponent, calls the existing `damage()` engine,
  preserves its raw return, and classifies the outcome.
- `src/spec/spec_effects.c|.h` creates stable negative affect sources and atomically applies
  spell-scoped stacking groups without conflating `source_id` and `specific`.
- All five production sources are listed in both `Makefile.am` and `CMakeLists.txt`; focused coverage
  lives in `unittests/CuTest/test_spec_mechanics.c` in both test manifests.

## Contract and Consumer Matrix

| Contract | Explicit Rule | Real Consumers | Focused Evidence |
|----------|---------------|----------------|------------------|
| Typed event shape | One owner bit and one event bit; compatible owner/event pair; required owner, actor, target, argument, and moving-room payload | Every event-specific gateway through `spec_dispatch_legacy()` | Valid and invalid owner, event, target, and moving-room contexts; invalid dispatch never calls the handler |
| Exact worn object | Actor is live, not pending extraction, in a valid room, and both `worn_by` and the wear slot point to the invoking object | `obj_proc_ready()`, both monk gloves, `stability_boots`, `hellfire` | Same-rnum copy is rejected while the exact instance succeeds |
| Live combat target | Actor and target are live, not pending extraction, in valid equal rooms; current-opponent identity is optional | `snake`, `wizard`, both monk gloves, safe damage wrapper | Current, missing-current, cross-room, and dead-target cases |
| Exact phrase | Resolved command and phrase are byte-exact; optional leading spaces follow legacy `skip_spaces()`; tabs, case, punctuation, and trailing whitespace are retained | `stability_boots`, `hellfire` | Both callbacks accept their established phrase and reject case, tab, trailing-space, and copied-instance variants |
| Legacy object cooldown | Object instance owns slot `[0, SPEC_TIMER_MAX)`; one `point_update()` decrement per MUD hour; no objsave serialization; recreation/restart reset; positive commit after effect success | `stability_boots`, `hellfire` | Ready, active, remaining hours, invalid bounds, invalid duration, and callback-active paths |
| Damage result | Validate the live colocated current opponent; reject negative amount; preserve `damage()` return; classify zero, positive, and negative as no effect, applied, and target invalidated | `monk_glove`, `monk_glove_cold` | Invalid target, invalid amount, no-effect, and nonlethal applied-damage paths |
| Affect source | Stable negative ID is `-(namespace * 1000000 + key)` for keys 1-999999; positive runtime IDs remain separate | Artifact passive application/removal and temporary artifact groups | Stable/distinct namespaces, source-selective removal, and actual Wyrmfang passive ownership |
| Stacking group | Group 1-`SHRT_MAX` is scoped by spell and stored in `specific`; source stays in `source_id`; a conflicting group rejects the whole modifier batch | Artifact ward, combat surge, group valor, frost ward, dragon sight, and enrage | Two-modifier atomic insert, cross-source conflict with no partial insert, independent group, and selective removal |

## Compatibility and Safety Corrections

The two invoked objects retain their accepted command and phrase behavior, cooldown duration, effect
order, messages, and remaining-time display. Their equipment check now proves that the invoking
instance is worn; another object with the same VNUM can no longer activate it. `obj_proc_ready()`
uses the same exact-instance rule for all of its existing consumers. The generic `is_wearing()` API
retains its same-VNUM policy for callers that intentionally want it.

Weapon-hit dispatch now receives the actual `victim` held by `handle_successful_attack()` instead
of reconstructing a target through `FIGHTING(ch)`. Invalid typed contexts are logged and rejected
before a legacy callback runs. The null-handler path still returns safely without requiring a valid
context, preserving pointer-gate behavior.

Monk-glove damage still uses the production `damage()` primitive with the same dice, attack type,
damage type, and return handling. The wrapper adds precondition and post-result vocabulary; a caller
must not touch a target after `SPEC_DAMAGE_TARGET_INVALIDATED`.

Artifact passives now use a namespaced source derived from artifact VNUM. Removal first deletes only
that source, then removes pre-Phase-04 persisted passives whose `source_id` is zero and whose old
registry tag is in `specific`. Temporary artifact powers retain their spell, modifier, bonus type,
duration, flags, group, messages, cooldown decisions, and XP behavior. Their modifiers are admitted
as one batch, so a stacking conflict cannot leave a partial power behind.

No player command, registry identity, builder selection, help keyword, world grammar, boot order,
assignment precedence, pulse schedule, artifact custody/progression rule, or callback ABI changed.
Existing identify text documents the two invoked phrases, so no helpfile migration was required.

## Acceptance Matrix

| Exit Criterion | Evidence | Result |
|----------------|----------|--------|
| Pointer-identity context validation for representative objects and mobiles | Exact worn-object validation is shared by object callbacks; live combat validation is shared by mobile and object callbacks. | PASS |
| Opt-in parsing after accepted-input characterization | Pure matcher and direct callback tests preserve exact command, leading-space, case, tab, and trailing-space behavior. | PASS |
| Explicit cooldown clock, storage, bounds, persistence, and commit rules | Header contract, bounded implementation, two object consumers, and focused state tests agree. | PASS |
| Safe target and combat-result contracts around existing primitives | Actual weapon victim reaches the context; combat validation and damage result classification use production state and `damage()`. | PASS |
| Affect source ownership remains separate from stacking identity | Negative namespace uses `source_id`; spell/group uses `specific`; source-selective removal and atomic conflict behavior are tested. | PASS |
| Every helper has at least two real consumers | Consumer matrix names the object, mobile, dispatch, damage, and artifact paths for each module. | PASS |
| Both build systems and test manifests remain synchronized | Five production modules and `test_spec_mechanics.c` occur in both manifests. | PASS |
| Documentation matches the delivered boundary | PRD, architecture, developer guide, ongoing index, master index, changelog, and this record describe Phase 04. | PASS |

## Test Ownership

`unittests/CuTest/test_spec_mechanics.c` adds eight production-linked tests. One additional dispatch
test proves invalid typed contexts do not invoke a handler. Existing combat-secondary coverage now
proves weapon-hit dispatch does not require ambient `FIGHTING()` state, and artifact integration
coverage proves a real passive uses its stable source independently from its stacking field.

The root suite contains 583 passing tests. The independent CMake matrix contains 12 passing targets,
including the production-linked CuTest binary and existing documentation, installation, health,
world-tooling, and vessel regressions.

## Reproducible Validation

### Autotools Production Gate

```sh
make clean
make -j"$(nproc)"         # no -Wall -Wextra warnings
make test                 # OK (583 tests)
make install              # installs bin/circle and removes root circle
test -x bin/circle
test ! -e circle
```

### Independent CMake and CTest

Use a fresh directory outside the source tree:

```sh
phase04_cmake_dir=$(mktemp -d /tmp/luminari-spec-phase04-cmake-XXXXXX)
case "$phase04_cmake_dir" in
  /tmp/luminari-spec-phase04-cmake-*) ;;
  *) exit 1 ;;
esac
cleanup_phase04_cmake() {
  rm -r -- "$phase04_cmake_dir"
}
trap cleanup_phase04_cmake EXIT

cmake -S . -B "$phase04_cmake_dir" \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$phase04_cmake_dir" --target circle cutest --parallel "$(nproc)"
ctest --test-dir "$phase04_cmake_dir" --output-on-failure
cleanup_phase04_cmake
trap - EXIT
```

### Manifest and Source Integrity

```sh
for source in spec_combat spec_context spec_cooldown spec_effects spec_phrase; do
  test "$(rg -n "src/spec/${source}\\.c" Makefile.am CMakeLists.txt | wc -l)" -eq 2
done
test "$(rg -n 'test_spec_mechanics\.c' Makefile.am CMakeLists.txt | wc -l)" -eq 3
git diff --check
```

## Acceptance Rule

Phase 04 is accepted when all five contracts name their ownership and lifetime rules, each is used by
at least two real behavior paths, focused production-linked tests cover success and rejection, both
build systems pass independently, installation leaves only the installed launch alias, and the
documentation distinguishes delivered mechanics from later typed-handler work. All held on
2026-08-07.
