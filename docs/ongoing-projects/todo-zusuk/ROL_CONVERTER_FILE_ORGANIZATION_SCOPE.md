# RoL Converter File Organization Scope

Status: analysis complete; implementation not started

Analysis date: 2026-08-17

This document scopes a behavior-preserving reorganization of the Realms of
Luminari world converter. The goal is to give each source format an explicit
implementation owner, such as a mobile module for `.mob` records and a room
module for `.wld` records, while preserving the existing conversion and release
contracts.

This is an internal code-organization project. It does not authorize changes to
converted game behavior, world data, VNUM allocation, the database schema, or
the Phase 7/8 release model.

## Executive assessment

The work is a medium-sized structural refactor rather than a converter rewrite.
A complete separation should cover both halves of conversion:

- `scripts/world/wtool_lib/rol_source.py` is 1,372 lines and parses all seven
  RoL source kinds.
- `scripts/world/wtool_lib/rol_transform.py` is 2,164 lines and emits all six
  directly converted target record kinds.
- The two files contain 3,536 lines of conversion core.
- `scripts/world/tests/test_rol_transform.py` is 3,788 lines with 103 tests.
- `scripts/world/tests/test_rol_source.py` has another 11 parser tests, while
  `test_rol_mobile_identity.py` has 12 closely related mobile tests.
- The complete world-tool suite currently contains 428 tests.

Splitting only `rol_transform.py` would improve output-side organization but
would leave all source grammars centralized in `rol_source.py`. The recommended
scope therefore separates both parsing and emission by format.

The safest design keeps `rol_source.py` and `rol_transform.py` as thin public
facades. Existing orchestration and tests can continue importing their current
symbols while the implementations move into focused modules.

## Current format ownership

| Source kind | Current parser | Current emitter or compiler | Target data |
|-------------|----------------|-----------------------------|-------------|
| `.mob` | `rol_source.py:_parse_mob` | `rol_transform.py:emit_mobile` | `.mob` |
| `.obj` | `rol_source.py:_parse_obj` | `rol_transform.py:emit_object` | `.obj` |
| `.wld` | `rol_source.py:_parse_wld` | `rol_transform.py:emit_room` | `.wld` |
| `.zon` | `rol_source.py:_parse_zon` | `rol_transform.py:emit_zone` | `.zon` |
| `.shp` | `rol_source.py:_parse_shp` | `rol_transform.py:emit_shop` | `.shp` |
| `.qst` | `rol_source.py:_parse_qst` | `rol_transform.py:emit_hlquest` | `.hlq` |
| `.soc` | `rol_source.py:_parse_soc` | `rol_soc.py` and special compilation | `.trg` and attachments |

The generic Luminari target readers are already separated into `mobiles.py`,
`objects.py`, `rooms.py`, `zones.py`, `shops.py`, `hlquests.py`, and
`triggers.py`. They validate emitted data and are not candidates for this
reorganization.

## Recommended module layout

Use flat, RoL-prefixed modules inside `scripts/world/wtool_lib`. This matches the
existing library layout, avoids ambiguity with the generic target readers, and
does not change `__file__` parent-depth assumptions used to locate
`rol_conversion_policy.json`.

| Module | Responsibility |
|--------|----------------|
| `rol_conversion_types.py` | Shared RoL record, reference, diagnostic, corpus, transform-result, and resolver types |
| `rol_source_common.py` | Source segmentation, tilde-string reading, numeric-row parsing, diagnostics, and record construction |
| `rol_transform_common.py` | ASCII/LF text conversion, bounded strings, bit operations, and shared directive helpers |
| `rol_mobiles.py` | `.mob` grammar, flag/race/class policy, automatic race behavior, calculator integration, and mobile emission |
| `rol_objects.py` | `.obj` grammar, item/flag/apply/value conversion, trap conversion, and object emission |
| `rol_rooms.py` | `.wld` grammar, room/sector/zone compatibility flags, exits, exit traps, and room emission |
| `rol_zones.py` | `.zon` grammar, reset references, reset normalization, equipment positions, and zone emission |
| `rol_shops.py` | `.shp` grammar, customer restrictions, products, messages, hours, and shop emission |
| `rol_quests.py` | `.qst` grammar, reward/reference conversion, and target HLQ emission |
| `rol_soc.py` | Existing SOC compilation plus the `.soc` source grammar, if this can be done without creating an import cycle |
| `rol_source.py` | Stable source-model exports, parser registry, active-corpus loading, and corpus reporting facade |
| `rol_transform.py` | Stable exports for emitters, public mappings, shared conversion helpers, and `TransformResult` |

If folding the SOC parser into `rol_soc.py` creates avoidable coupling, a small
`rol_soc_source.py` is preferable to making `rol_source.py` retain one special
case. This is the only format boundary that needs a judgment call during
implementation.

## Dependency rules

The split should establish one-way dependencies and avoid a new collection of
cyclic imports.

1. Shared type and helper modules must not import a format module.
2. Format modules may import shared types and helpers.
3. `rol_objects.py` may reuse the mobile affect mappings currently shared by
   mobile and object prototypes. That dependency must be explicit or the maps
   must move to a narrowly named shared flag module.
4. `rol_shops.py` may reuse the object-type map because source shop buy types are
   converted to target object types.
5. `rol_zones.py` should own the equipment-position map, which is used by zone
   equipment reset conversion rather than object emission.
6. Facade modules may import and re-export format symbols. Format modules must
   not import the facades.
7. Phase orchestration should continue depending on the facades rather than on
   every implementation module.

The public compatibility surface presently includes more than the six emitter
functions. `rol_capability_audit.py` imports mapping tables and the private
`_source_mask_bits` helper from `rol_transform.py`. Those imports should either
remain available through the facade or be deliberately redirected in the same
change.

## Work included

### Source parsing

- Move each `_parse_*` function and its format-specific constants into its owner
  module.
- Keep record segmentation, source spans, tilde strings, diagnostic construction,
  and reference construction shared.
- Preserve `_PARSERS` dispatch behavior, record ordering, hashes, completeness,
  diagnostic codes, diagnostic ordering, and typed references exactly.
- Preserve exact mobile syntax-repair policy lookup. Moving that code into a
  nested package would change the meaning of `Path(__file__).parents[1]`, which
  is one reason to prefer flat modules.

### Target emission

- Move each `emit_*` function with its associated constants and private helpers.
- Keep `TransformResult`, `IdentityResolver`, text canonicalization, tilde
  framing, bounded-line handling, and bit encoding shared.
- Preserve target bytes and diagnostic/ledger ordering exactly.
- Keep `convert_text`, all emitters, `mobile_automatic_race_flags`, and the
  capability-audit mappings import-compatible through `rol_transform.py`.

### Orchestration

The following files should need little or no logic change if the facades remain
stable:

- `rol_pilot_build.py`
- `rol_phase7.py`
- `rol_capability_audit.py`
- `rol_mobile_identity.py`
- `rol_discovery.py`
- `rol_graph.py`
- `rol_special.py`
- `rol_soc.py`

The refactor must not split the release pipeline by file extension. Phase 7
selection is package- and dependency-based, and Phase 8 requires the complete
accepted Phase 7 corpus. A dedicated `.mob` implementation module does not imply
or permit a `.mob`-only release mode.

### Tests

Split direct parser and emitter tests along the same boundaries, using names
that cannot be confused with the generic target-reader tests:

- `test_rol_conversion_mobiles.py`
- `test_rol_conversion_objects.py`
- `test_rol_conversion_rooms.py`
- `test_rol_conversion_zones.py`
- `test_rol_conversion_shops.py`
- `test_rol_conversion_quests.py`
- `test_rol_conversion_soc.py`, if SOC parsing moves
- `test_rol_conversion_common.py`
- `test_rol_conversion_integration.py` for special bindings and behavior that
  intentionally crosses record types

The existing large transform test contains many special-procedure integration
tests in addition to direct serialization tests. Those tests should remain
integration tests rather than being assigned arbitrarily to the first record
type they touch.

Test movement must not reduce coverage or silently rename tests out of unittest
discovery. The test count and the set of discovered test method names should be
captured before and after the change.

### Build and release evidence

Adding Python files requires synchronized updates to both:

- `world_tool_sources` in `Makefile.am`
- `WORLD_TOOL_SOURCES` in `CMakeLists.txt`

Phase 8 hashes a fixed `_CODE_EVIDENCE_PATHS` list. Every new module that can
change converted output must be added to that list. Existing Phase 8 bundles
will correctly fail the code-unchanged check after this refactor; a new bundle
must record the new code layout before any future release acceptance run.

### Documentation

Update current documentation that names `rol_transform.py` as the sole room or
sector conversion owner. Add a short converter-layout section to
`docs/utilities/WORLD_VALIDATOR_CLI.md` if the new boundaries would otherwise be
discoverable only from source.

Historical paths in `docs/CHANGELOG.md` and `docs/previous_changelogs/` must
remain unchanged. No player helpfile update is necessary because the project is
not intended to change player-visible behavior.

## Work excluded

- Changing conversion mappings, repair policy, special-procedure behavior, or
  diagnostics.
- Modifying generated or installed world data.
- Adding independent per-extension conversion or release commands.
- Changing VNUM allocation, identity resolution, batching, package selection,
  persistence checks, or database access.
- Refactoring the large Phase 7, Phase 8, discovery, reconciliation, or special
  modules merely because they exceed 1,000 lines. Their primary responsibilities
  are cross-format and outside this file-ownership objective.
- Changing C runtime files, schemas, configuration headers, credentials, or
  production state.

## Principal risks

### Serialized-output drift

Small changes to helper ownership, iteration order, imports, or constants can
alter target bytes or diagnostic order. Existing parse-validity tests are not a
substitute for byte equality.

Mitigation: capture a deterministic digest of every emitted active-corpus record
before moving code and require the post-refactor output and ordered diagnostics
to match.

### Import cycles

Object conversion reuses mobile affect mappings, shops reuse object types, SOC
compilation consumes RoL records, and facade modules re-export symbols. Moving
types first and enforcing facade-only outward imports prevents most cycles.

### Incomplete release evidence

If new format modules are absent from `_CODE_EVIDENCE_PATHS`, Phase 8 would no
longer hash all code capable of changing conversion output.

### Accidental API breakage

The orchestrators and tests import public and semi-private symbols from both
monoliths. Thin facades minimize churn and allow format movement to proceed one
module at a time.

### Test reclassification mistakes

The transform test suite mixes unit, corpus, SOC, and special-binding checks.
Mechanical splitting by the emitter called inside a test could obscure the
behavior actually being protected.

## Suggested implementation sequence

1. Record the baseline list of discovered tests and a deterministic full-corpus
   conversion digest, including ordered diagnostics and mobile ledgers.
2. Extract shared types without changing existing public imports.
3. Extract source and transform common helpers.
4. Move one format at a time, beginning with rooms or zones because their
   dependencies are relatively narrow.
5. Move mobiles, then objects and shops after their shared mapping decisions are
   explicit.
6. Move quests and SOC parsing.
7. Reduce `rol_source.py` and `rol_transform.py` to documented facades.
8. Split tests by behavior, update both build manifests, update Phase 8 code
   evidence, and repair current documentation references.
9. Run all acceptance checks and compare the complete before/after corpus.

This can be implemented as one focused refactor. Two reviewable changes are
safer if preserving `git blame` and limiting diff size are priorities: first
split production modules behind compatibility facades, then reorganize the test
files without changing test bodies.

## Acceptance criteria

The project is complete only when all of the following are true:

- Each RoL source kind has one obvious parser and emitter/compiler owner.
- `rol_source.py` and `rol_transform.py` are small, documented facades rather
  than mixed-format implementations.
- Existing imports used by orchestration remain valid or are intentionally
  migrated in the same change.
- Every active-corpus output byte, ordered diagnostic, and mobile ledger matches
  the pre-refactor baseline.
- The discovered world-tool test set does not shrink.
- `make test-world-tools` passes.
- `make test` passes and is followed by `make install`, leaving no root-level
  `luminari` artifact.
- A complete Phase 7 repeat, when its sealed inputs are available, is
  byte-identical to the accepted candidate tree.
- Both build-system source lists and the Phase 8 code-evidence list contain all
  new files.
- Current documentation points to the new format owners.
- No world data, database state, credentials, production code, or protected
  local configuration headers are changed.

## Baseline verification

At analysis time:

- `APP_ENV` was `development`.
- The worktree was clean.
- `make test-world-tools` passed.
- The suite contained 428 discovered world-tool tests.
- No converter or world files were modified while producing this assessment.
