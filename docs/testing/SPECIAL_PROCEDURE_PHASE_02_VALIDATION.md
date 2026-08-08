# Special Procedure Phase 02 Validation

> Historical phase snapshot: source paths and inventory commands below record the Phase 02
> baseline. Phase 07 moved compiled assignments under `src/spec/`; use the
> [Phase 07 validation matrix](SPECIAL_PROCEDURE_PHASE_07_VALIDATION.md) for the current source map.

## Purpose

This document is the durable acceptance and test-ownership record for Phase 02: Declarative Legacy
Assignments. It maps the phase exit criteria to production source, the audited legacy inventory,
production-linked tests, staff diagnostics, help content, and reproducible validation commands.

Phase 02 was completed and verified on 2026-08-07. Phase 00 remains the binding control plane and
Phase 01 remains the invocation gateway prerequisite. See
[Phase 00 Validation](SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md) and
[Phase 01 Validation](SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md).

## Delivered Boundary

- `src/spec/spec_assign_table.h` defines separate mobile, object, and room assignment rows using
  `mob_vnum`, `obj_vnum`, and `room_vnum` fields rather than a shared compact integer row.
- `src/spec/spec_assign_table.c` resolves row names through the immutable registry and rejects empty
  or unknown names, owner mismatches, and definitions that forbid legacy assignment.
- `spec_assign_table_boot_validate()` runs after registry validation and before world parsing. A bad
  table row is a boot-fatal programmer error rather than a silently missing callback.
- The Luminari object table converts `NOOB_CRAFTING_KIT` / `Crafting Kit` and
  `VAMPIRE_CLOAK_OBJ_VNUM` / `Vampire Cloak`.
- Declarative rows and direct `ASSIGN*` calls use `assign_object_spec()` and the same corresponding
  owner helpers. Callback installation, effective provenance, collisions, source ordering, and
  `-s` behavior therefore remain unchanged.
- `specbind <mob|obj|room> <vnum>` gives immortal staff a read-only view of the recorded post-boot
  chain, including every source and outcome, source locations, collisions, wrapper secondaries, and
  the final source.

Phase 02 moves no binding to world data, changes no world-file grammar, registers no handler solely
to increase conversion count, and does not flatten shop or quest composition.

## Conversion Inventory and Decision

The original broad lexical inventory at parent commit `fd87586d` contained 785
`ASSIGNMOB` / `ASSIGNOBJ` / `ASSIGNROOM` tokens in `src/spec_assign.c`. The Phase 02 source contains
783 such tokens plus the two declarative rows. Of the remaining tokens, 777 still begin with a raw
numeric literal. The six non-literal tokens are the three assignment macro definitions, the computed
`harpell` loop row, and two `VAMPIRE_CLOAK_OBJ_VNUM` rows in alternate campaign branches.

An active-direct-call scan across all conditional source branches finds 752 rows: 459 mobile, 182
object, and 111 room calls. Of those, 749 use numeric literals. These counts are source snapshots,
not runtime counts, because only the Luminari preprocessor branch is a supported build target.

Only the two converted Luminari rows have both prerequisites required by the PRD:

| Owner/VNUM | Definition | Eligibility Evidence |
|------------|------------|----------------------|
| Object `NOOB_CRAFTING_KIT` | `Crafting Kit` | Symbolic VNUM exists in the configuration template and local development header; registry metadata permits object legacy assignment. |
| Object `VAMPIRE_CLOAK_OBJ_VNUM` | `Vampire Cloak` | Symbolic VNUM is owned by `src/magic/spells.h`; registry metadata permits object legacy assignment. |

The unsupported inventory intentionally remains direct. Copying numeric literals into a table would
violate the traced-symbol requirement without improving identity, while adding hundreds of
definitions or modifying local VNUM configuration would expand the phase and violate repository
constraints. Computed setup stays procedural, and alternate campaign branches remain compatibility
inventory. This is completion of the documented Phase 02 boundary, not a claim that every legacy
assignment is declarative.

## Binding and Precedence Evidence

The effective callback sequence is unchanged:

1. Named world and moving-room parser records contribute during world load.
2. Direct and declarative mobile assignments use the legacy-assignment source.
3. Shop wrappers save the callback active at their point in boot.
4. Direct and declarative object and room assignments use the same legacy-assignment source.
5. Quest wrappers save the callback active at their point in boot.

`apply_object_assignments()` resolves canonical registry metadata, then calls `assign_object_spec()`.
The previous direct rows called that same helper through `ASSIGNOBJ`. All remaining direct helpers
still call `record_legacy_assignment()`, so unsupported rows remain traceable. Existing production
tests in `test_spec_effective_binding.c` and `test_spec_combat_secondary.c` continue to own collision,
`-s`, saved-secondary, and quest-over-shop-over-original behavior.

## Test Ownership

`unittests/CuTest/test_spec_assign_table.c` owns 11 Phase 02 tests:

- production definition resolution for both converted rows;
- canonicalization of an explicit alias;
- empty and unknown name rejection;
- mobile, object, and room owner mismatch rejection;
- invalid combined and empty owner masks;
- rejection when a definition forbids legacy assignment;
- valid mobile, object, room, empty, and null-zero tables;
- failing-row index and VNUM diagnostics;
- null table rejection when count is nonzero;
- safe operation without an error buffer; and
- stable public labels for world, legacy, parser, shop, and quest sources.

The full production-linked suite contains 574 passing tests at Phase 02 close. The special-procedure
inventory through this phase is 101 dedicated tests: 78 from Phase 00, 12 from Phase 01, and 11 from
Phase 02.

## Staff and Help Acceptance

The authoritative database topic in `sql/components/help_specproc_entries.sql` maps `SPECBIND` and
the existing special-procedure keywords to tag `spec-proc`. It documents command syntax, fields,
read-only behavior, and the fact that history is a boot snapshot. The matching verifier checks
content, access level, all six keywords, and conflicting ownership.

On a development server, verify these representative runtime chains:

```text
specbind obj 3118
specbind mob 1201
```

The object result must show the declarative `Crafting Kit` assignment. The mobile result must show
the ordered authored `Postmaster` plus direct `postmaster` reassertion and its collision count. The
command must not alter either prototype.

## Reproducible Validation

### Autotools Production Gate

```sh
make -j"$(nproc)"         # no new -Wall -Wextra warnings
make test                 # OK (574 tests)
make install              # installs bin/circle and removes root circle
test -x bin/circle
test -L bin/circle
test ! -e circle
```

### Assignment Inventory

```sh
git show fd87586d:src/spec_assign.c |
  rg -o 'ASSIGN(MOB|OBJ|ROOM)\(' | wc -l       # 785
rg -o 'ASSIGN(MOB|OBJ|ROOM)\(' src/spec_assign.c | wc -l  # 783
```

For the active-direct-call count, match only lines beginning with optional whitespace followed by an
`ASSIGN*` call and group the first and second arguments. The expected owner totals are 459 mobile,
182 object, and 111 room; 749 first arguments are numeric literals.

### Independent CMake and CTest

Use a fresh directory outside the source tree:

```sh
phase02_cmake_dir=$(mktemp -d /tmp/luminari-spec-phase02-cmake-XXXXXX)
case "$phase02_cmake_dir" in
  /tmp/luminari-spec-phase02-cmake-*) ;;
  *) exit 1 ;;
esac
cleanup_phase02_cmake() {
  rm -r -- "$phase02_cmake_dir"
}
trap cleanup_phase02_cmake EXIT

cmake -S . -B "$phase02_cmake_dir" \
  -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$phase02_cmake_dir" --target cutest --parallel "$(nproc)"
ctest --test-dir "$phase02_cmake_dir" --output-on-failure
cleanup_phase02_cmake
trap - EXIT
```

Phase 02 close result: all 12 CTest targets passed, including `production-cutest`, world tooling,
documentation validation, supervision, installation, health, and vessel regression targets.

### Help SQL and Live Development Smoke

On a database explicitly classified by `lib/.env` as development, create connection-local temporary
copies of `help_entries` and `help_keywords`. Source `help_specproc_entries.sql` twice, then source
`verify_help_specproc_entries.sql`; all four checks must report `PASS`. Apply the migration to the
persistent development database through the normal migration path, then run:

```sh
./scripts/development/dev_kohdee_login_smoke.sh --help-check \
  SPECIALS SPEC SPEC-PROC SPECIAL-PROCEDURE SPECBIND SPECPROC
./scripts/development/dev_kohdee_login_smoke.sh --commands \
  "specbind obj 3118" "specbind mob 1201"
```

Phase 02 close result: all four SQL checks passed after two isolated and two persistent development
applications; all six live help keywords resolved to `spec-proc`; both representative `specbind`
commands returned the expected ordered chains.

Never run this acceptance procedure against production.

### Integrity and Hygiene

```sh
git diff --check
test "$(rg -l '[^ -~]' \
  docs/testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md \
  docs/ongoing-projects/spec-todo.md \
  docs/guides/OLC_SpecProcs.md \
  docs/guides/DEVELOPER_GUIDE_AND_API.md \
  docs/guides/TESTING_GUIDE.md \
  docs/ARCHITECTURE.md \
  docs/systems/CORE_SERVER_ARCHITECTURE.md \
  docs/systems/HELP_SYSTEM.md \
  sql/components/help_specproc_entries.sql \
  sql/components/verify_help_specproc_entries.sql | wc -l)" -eq 0
test "$(rg -n 'spec_assign_table.c' Makefile.am CMakeLists.txt | wc -l)" -ge 2
test "$(rg -n 'test_spec_assign_table.c' Makefile.am CMakeLists.txt | wc -l)" -ge 3
```

## Acceptance Rule

Phase 02 is accepted when every eligible compatibility row is table-driven and boot-validated,
unsupported rows remain observable without violating VNUM or registry constraints, the complete
post-boot chain is available to staff, shop and quest secondaries preserve their established order,
the help surface matches the command, and all production/build/documentation gates pass. All held on
2026-08-07.
