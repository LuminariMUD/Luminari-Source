# Special Procedure Phase 05 Validation

**Status:** Complete

**Validated:** 2026-08-07

## Delivered Scope

Phase 05 adds incremental typed-handler dispatch without replacing the prototype callback slot or
the legacy `SPECIAL` ABI. Registry definitions now declare exactly one of these implementation
shapes:

- one complete legacy handler; or
- one callback-slot adapter plus one typed handler.

`spec_definition_callback()` exposes the pointer stored in prototypes for either shape. Authored
bindings, OLC selection, declarative assignments, reverse lookup, and compatibility name access all
use that callback identity, so existing canonical names, aliases, world records, and assignment
precedence remain unchanged.

`spec_dispatch()` reverse-resolves the callback at each existing event gateway. A registered typed
adapter receives the validated `spec_event_context`; every other callback follows the exact legacy
translation through `spec_dispatch_legacy()`. Typed dispatch validates the definition's owner/event
contract, keeps flow separate from pointer invalidation, accepts STOP only for flow-bearing events,
and logs and clears STOP for notification-only events without discarding invalidation.

## Converted Procedures

| Definition | Owners and Events | Removed Inference | Compatibility Result |
|------------|-------------------|-------------------|----------------------|
| Bank | Mobile/object command; object identify | Identification uses `SPEC_EVENT_ITEM_IDENTIFY`, not command zero plus the string `identify`. | Canonical name `Bank`, callback pointer `bank`, mobile/object assignments, commands, messages, and returns remain stable. |
| Vampire Cloak | Object command and identify | Identification uses event identity; command execution uses the context owner instead of locating another same-VNUM object through ambient equipment state. | Canonical name `Vampire Cloak`, callback pointer `vampire_cloak`, `setcloak` command/output, assignment, and persistence remain stable. The invoking cloak must be the exact instance worn in `WEAR_ABOUT`. |

Both adapters fail safely and log if called outside a gateway. Their typed implementations are
declared by `src/obj/vendor.h` and `src/character/vampire_cloak.h`.

## Remaining Compatibility Inventory

The source inventory after conversion is:

| Population | Count |
|------------|------:|
| Source-level `SPECIAL(...)` definitions | 196 |
| Typed callback-slot adapters | 2 |
| Remaining legacy behavior implementations | 194 |
| Canonical registry definitions | 28 |
| Typed registry definitions | 2 |
| Legacy registry definitions | 26 |

The source-level inventory includes registered, hard-coded, feature-private, zone-private, and
compatibility procedures. It is deliberately broader than the registry. With 194 legacy behavior
implementations still present, the legacy ABI, fallback dispatch, direct assignment support, and
shop/quest secondary support remain required.

Reproduce the broad inventory with:

```sh
test "$(rg -n '^[[:space:]]*SPECIAL\(' src --glob '*.c' | wc -l)" -eq 196
```

The production registry counts are asserted through `spec_registry_count()`,
`spec_registry_typed_count()`, and `spec_registry_legacy_count()` in the production-linked suite.

## Acceptance Matrix

| Exit Criterion | Evidence | Result |
|----------------|----------|--------|
| Typed handlers run behind existing event gateways | All gateways use `spec_dispatch()`; reverse lookup selects typed or exact legacy dispatch. | PASS |
| Definitions have one valid implementation shape | Boot validation rejects mixed, incomplete, or colliding typed-adapter definitions. | PASS |
| Persisted identities remain stable | Callback reverse lookup, authored binding, OLC selection, and assignment tests retain `Bank` and `Vampire Cloak`. | PASS |
| Converted handlers do not infer identify events from magic strings | Both converted handlers branch on `context->event`; focused tests use deliberately unrelated argument strings. | PASS |
| Converted object behavior uses the invoking owner | A carried duplicate cloak is rejected while the exact worn owner reaches the established vampire check. | PASS |
| Flow and invalidation remain independent | Typed dispatch tests cover flow-bearing return mapping, notification-only STOP rejection, and retained target invalidation. | PASS |
| Remaining legacy population is explicit | The 196/2/194 source inventory and 28/2/26 registry inventory are recorded above. | PASS |
| OLC and both assignment paths remain compatible | OLC, authored-binding, round-trip, declarative, direct-assignment, and secondary tests pass through callback adapters. | PASS |
| Both test manifests include focused coverage | `test_spec_typed_handlers.c` is present in the two Autotools test lists and the CMake test list. | PASS |

## Test Ownership

`unittests/CuTest/test_spec_typed_handlers.c` adds five production-linked tests covering:

1. callback-slot and persisted identity across typed definitions;
2. typed flow, context target, and invalidation;
3. invalid STOP handling for notification-only events;
4. Bank event identity without the legacy magic argument; and
5. Vampire Cloak event identity and exact invoking-object ownership.

Existing registry, binding, OLC, assignment, gateway, effective-precedence, and round-trip suites
continue to exercise the same production callback slots. The complete root suite contains 588
passing tests. The independent CMake matrix contains 12 passing targets.

## Reproducible Validation

### Autotools Production Gate

```sh
make clean
make -j"$(nproc)"         # no -Wall -Wextra warnings
make test                 # OK (588 tests)
make install              # installs bin/circle and removes root circle
test -x bin/circle
test ! -e circle
```

### Independent CMake and CTest

Use a fresh directory outside the source tree:

```sh
phase05_cmake_dir=$(mktemp -d /tmp/luminari-spec-phase05-cmake-XXXXXX)
case "$phase05_cmake_dir" in
  /tmp/luminari-spec-phase05-cmake-*) ;;
  *) exit 1 ;;
esac
cleanup_phase05_cmake() {
  rm -r -- "$phase05_cmake_dir"
}
trap cleanup_phase05_cmake EXIT

cmake -S . -B "$phase05_cmake_dir" \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$phase05_cmake_dir" --target circle cutest --parallel "$(nproc)"
ctest --test-dir "$phase05_cmake_dir" --output-on-failure
cleanup_phase05_cmake
trap - EXIT
```

### Inventory and Integrity

```sh
test "$(rg -n '^[[:space:]]*SPECIAL\(' src --glob '*.c' | wc -l)" -eq 196
test "$(rg -n 'test_spec_typed_handlers\.c' Makefile.am CMakeLists.txt | wc -l)" -eq 3
git diff --check
```

No player command, accepted input, message, help keyword, world grammar, canonical identity, OLC
menu row, boot precedence, pulse schedule, or callback slot changed. Existing help already describes
Bank and Vampire Cloak behavior, so no player helpfile update was required.

## Acceptance Rule

Phase 05 is accepted when at least two real procedures dispatch typed context behind their existing
callback identities, converted behavior no longer infers event identity from magic strings or
ambient object lookup, typed outcomes are validated, persistence and OLC remain stable, the legacy
population is counted, both build systems pass independently, and installation leaves only the
installed launch binary. All held on 2026-08-07.
