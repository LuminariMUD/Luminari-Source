# Session Specification

**Session ID**: `phase00-session04-validated-definition-registry`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-06
**Completed**: 2026-08-07
**Base Commit**: e30cbb33eccb7a7546f1335003e2573183bd11d3

---

## 1. Session Overview

This session replaces the sentinel-terminated `spec_func_list` with an immutable registry of
validated special-procedure definitions. Each canonical definition gains explicit identity,
ownership, event, prerequisite, binding, visibility, category, description, and handler metadata.
The registry is validated before `boot_world()` can parse world data.

The existing `SPECIAL` callback ABI and public name/function accessors remain available. Their
historical 29-name ordering is preserved as a compatibility projection over 28 canonical
definitions: `Guild` remains canonical and `Guildmaster` is its explicit alias. New accessors expose
canonical definitions directly and reject invalid indexes, owner masks, and event masks safely.

---

## 2. Objectives

1. Establish one immutable, metadata-rich definition for every currently persisted procedure.
2. Preserve every current canonical name, alias lookup, handler pointer, and legacy list position.
3. Validate the complete registry with deterministic, actionable diagnostics before world parsing.
4. Provide bounds-safe, owner-aware, event-aware, and handler-aware lookup and iteration APIs.
5. Document the registry contract and prove it through production-linked negative-path tests.

---

## 3. Prerequisites

### Required Sessions

- Session 01 is complete and freezes registry identity, alias, accessor, and persistence behavior.
- Sessions 02 and 03 are complete and freeze all currently verified invocation categories.

### Required Tools Or Knowledge

- Root production-linked CuTest harness and generated `AllTests.c` registry.
- Current registry and compatibility accessors in `src/spec_assign.c`.
- Current boot boundary between `boot_db()` and `boot_world()` in `src/db.c`.
- Current callback invocation and activation evidence recorded by Sessions 02 and 03.

### Environment Requirements

- Development checkout; `lib/.env` and `lib/mysql_config` remain read-only.
- Both Autotools and CMake manifests receive every added source.
- `make test` is followed by `make install`.

---

## 4. Scope

### In Scope (MVP)

- A shallow `src/spec/` registry module with an immutable definition table.
- Canonical name, display name, aliases, owner mask, event contracts, prototype and placement
  prerequisites, binding-source mask, builder visibility, category, description, and handler data.
- Complete metadata for all 28 canonical definitions represented by the current 29 persisted names.
- Explicit `Guildmaster` alias metadata with `Guild` as reverse-lookup canonical identity.
- Canonical and compatibility accessors with full low and high boundary checks.
- Case-insensitive lookup by canonical name or alias, plus owner and event compatibility helpers.
- Reusable table validation for focused negative tests and fatal validation of the production table
  before `boot_world()`.
- Public developer documentation for adding and consuming definitions.

### Out Of Scope (Deferred)

- Owner-filtered OLC menus or changes to the current 29-entry selection presentation.
- Authored, unresolved, or effective binding records and provenance.
- Runtime dispatch gateways, typed handler implementations, callback chains, or ABI changes.
- World-file syntax changes, content migrations, or automatic prerequisite flag mutation.

---

## 5. Technical Approach

### Architecture

Create `src/spec/spec_registry.h` and `src/spec/spec_registry.c`. The header defines fixed-width
bitmasks for owner types, supported events, prototype requirements, placement requirements, and
binding sources; an explicit visibility enum; event-contract and definition structures; a typed
handler placeholder; validation results; and read-only lookup APIs.

The source owns one `static const` canonical definition array and a separate immutable compatibility
name projection. The projection preserves the historical 29-name ordering without duplicating
definition objects or letting aliases become reverse-lookup canonical names. The old exported
functions continue to use this projection, while new APIs iterate only canonical definitions.

Move the legacy accessors out of `src/spec_assign.c`; assignments remain there unchanged. Add a
single fatal `spec_registry_boot_validate()` call in `boot_db()` after core definition setup and
before `boot_world()`. Its reusable validator reports the first failure through a caller-provided,
bounded buffer so unit tests can exercise malformed tables without terminating the process.

### Validation Contract

Validation rejects:

- null, empty, or whitespace-only canonical names, display names, aliases, categories, or
  descriptions;
- case-insensitive canonical/canonical, canonical/alias, or alias/alias collisions;
- aliases that repeat their own canonical identity;
- zero or unknown owner, event, prerequisite, placement, or binding-source bits;
- event contracts incompatible with every declared owner type;
- missing event arrays, empty event sets, duplicate events, or invalid visibility values;
- definitions with neither handler or both a legacy and typed handler.

Diagnostics identify the definition index or identity and the failing field. The production boot
wrapper logs the diagnostic as `SYSERR` and exits before any world-file or database world parsing.

### Compatibility Rules

- `get_spec_func_count()` remains 29 and the indexed names remain in the Session 01 order.
- `get_spec_func_name_by_index()` and `get_spec_func_by_index()` return null for every negative or
  out-of-range `int`, including `INT_MIN` and `INT_MAX`.
- `find_spec_func_by_name()` resolves canonical names and aliases case-insensitively.
- `get_spec_func_name()` returns canonical identity and therefore returns `Guild` for `guild`.
- The canonical definition count is 28; aliases do not consume canonical indexes.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `src/spec/spec_registry.h` | Public metadata, masks, structures, and accessors | ~180 |
| `src/spec/spec_registry.c` | Immutable registry, validation, and compatibility adapters | ~700 |
| `unittests/CuTest/test_spec_registry_validation.c` | Production metadata and malformed-table tests | ~650 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `src/spec_assign.c` | Remove the superseded untyped table and accessor bodies | ~-100 |
| `src/db.c` | Invoke fatal registry validation before `boot_world()` | ~3 |
| `Makefile.am` | Add production and test sources to both relevant memberships | ~3 |
| `CMakeLists.txt` | Add production and test sources to matching memberships | ~2 |
| `docs/guides/DEVELOPER_GUIDE_AND_API.md` | Document the definition and validation contract | ~70 |
| `docs/guides/OLC_SpecProcs.md` | Correct the registry location and current metadata status | ~10 |

---

## 7. Success Criteria

### Functional Requirements

- [x] All 28 canonical definitions expose complete valid metadata and exactly one handler.
- [x] `Guildmaster` resolves as an explicit alias while reverse lookup remains `Guild`.
- [x] Canonical, owner-aware, event-aware, and handler lookups are case-insensitive and safe.
- [x] The complete historical 29-name compatibility projection is unchanged.
- [x] Registry validation runs before `boot_world()` and reports actionable failures.

### Testing Requirements

- [x] Negative tests cover every validation family without mutating production registry data.
- [x] Extreme indexes and unknown owner/event bits return safely.
- [x] Existing Sessions 01-03 characterization suites continue to pass unchanged.
- [x] Root `make test`, CMake `production-cutest`, and `make install` pass.

### Non-Functional Requirements

- [x] The `SPECIAL` ABI, runtime dispatch, assignments, world syntax, and OLC behavior do not change.
- [x] New production and test sources are synchronized across Automake and CMake.
- [x] Public structures and diagnostics are bounded, const-correct, and documented.

### Quality Gates

- [x] All changed text files are ASCII-encoded UTF-8 with LF endings.
- [x] Code follows project C23 conventions and introduces zero compiler warnings.
- [x] No protected configuration, credential, world-data, or production environment file changes.

---

## 8. Implementation Notes

### Working Assumptions

- A definition may support multiple owner types when the same legacy handler is intentionally
  installed on mobile, object, or room prototypes.
- Each event-contract entry represents one event bit and states only prerequisites relevant to that
  event; definitions may have several event-contract entries.
- Builder visibility and world binding are separate explicit metadata dimensions even though OLC
  filtering is delivered in Session 05.
- Typed handler storage is part of the validation shape, but no typed handler is introduced here.

### Key Considerations

- Preserve the old indexed alias position for OLC compatibility until Session 05 deliberately
  changes presentation.
- Do not infer owner types solely from handler names; use assignments, parser contracts, and
  invocation bodies.
- Do not claim runtime prerequisites that are not enforced by the characterized call sites.
- Avoid a sentinel row; every production and compatibility loop uses compile-time counts.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:
- Collapsing `Guild` and `Guildmaster` can accidentally change indexed OLC saves or reverse lookup.
- An incomplete event contract can mislead the owner-filtered OLC work in Session 05.
- A validation call placed inside `boot_world()` would occur after MySQL/world initialization and
  violate the early-failure contract.
- Generic bitmask validation can accept multi-bit event entries unless one-hot checks are explicit.

---

## 9. Testing Strategy

### Production Registry Tests

- Assert all canonical definitions validate and expose complete, non-empty metadata.
- Assert exact canonical count, identity inventory, handler mapping, Guild alias behavior, and legacy
  projection inventory.
- Assert canonical, alias, handler, owner-aware, and event-aware accessors across valid and invalid
  inputs.
- Assert every extreme signed index and unknown bitmask input is rejected without dereference.

### Malformed Table Tests

- Construct local immutable fixtures for empty metadata and each name collision class.
- Exercise invalid owner, event, prototype, placement, binding, and visibility values.
- Exercise missing, duplicate, incompatible, and multi-bit event contracts.
- Exercise handlerless and dual-handler definitions.
- Check stable diagnostic field and identity text for each failure family.

### Integration Gates

- Verify source ordering places boot validation before the `boot_world()` call.
- Run focused compilation and the production-linked CuTest binary.
- Run configured formatting and static checks for new C and header files.
- Run the full Autotools and CMake suites, then install and check artifact hygiene.

---

## 10. Security And Reliability

- The validator accepts explicit counts and never scans untrusted sentinels.
- All diagnostic writes use bounded formatting and tolerate a null or zero-length output buffer.
- Public index APIs check bounds before reading arrays.
- Registry data and strings are immutable for process lifetime.
- No world data, credentials, network behavior, or database schema changes are in scope.

---

## 11. Completion Definition

The session is complete when all tasks are checked, review has no unresolved blocker, validation and
security gates pass, the PRD/session state is updated, `make test` and `make install` pass, and the
committed result is published on the existing development branch.
