# Session Specification

**Session ID**: `phase00-session03-combat-and-secondary-characterization`
**Phase**: 00 - Registry Safety and Observability
**Status**: Complete
**Created**: 2026-08-06
**Completed**: 2026-08-06
**Base Commit**: 61886b718b1cc5934f85934ac144a3ef2fb83bb2

---

## 1. Session Overview

This session completes the Phase 00 compatibility freeze for legacy `SPECIAL` invocation paths.
It adds executable evidence for combat turns, item identification, weapon hits, defense reactions,
shield maneuvers, mounted charge, and shop and quest secondary composition. Later sessions may
introduce typed metadata and observable bindings, so these exact payloads, activation gates,
ordering rules, and caller-specific return meanings must first be fixed against production code.

The implementation is test-only. Runtime tests exercise narrow production entry points and the
shop and quest wrappers. Bounded source contracts cover branch-heavy combat call sites whose full
execution would require unrelated random combat outcomes. No dispatch behavior or application
code changes in this session.

---

## 2. Objectives

1. Freeze the remaining exact legacy callback tokens and actor/owner payloads.
2. Distinguish ignored notification returns from propagated shop and quest secondary returns.
3. Freeze mobile combat activation and its position after normal attacks and cleave.
4. Characterize shop-over-original and quest-over-existing nested composition.
5. Complete the Phase 00 invocation and return-semantics matrix, including `no_specials` behavior.

---

## 3. Prerequisites

### Required Sessions

- Sessions 01 and 02 are complete and their production-linked characterization tests pass.

### Required Tools Or Knowledge

- Root CuTest harness and generated `AllTests.c` registry.
- Current combat call sites in `src/combat/fight.c` and `src/combat/act.offensive.c`.
- Current identification call site in `src/obj/act.item.c`.
- Current secondary wrappers and assignment code in `src/obj/shop.c` and `src/quest/quest.c`.
- Current boot gates and precedence in `src/db.c`.

### Environment Requirements

- Development checkout; `lib/.env` and `lib/mysql_config` remain read-only.
- All replaced production globals are snapshotted and restored before a test returns.
- `make test` is followed by `make install`.

---

## 4. Scope

### In Scope (MVP)

- Mobile combat callbacks using `(ch, ch, 0, "")`, requiring `MOB_SPEC` and a callback, and
  ignoring callback returns.
- Mobile combat callback ordering after normal attacks and cleave handling.
- Identification token `"identify"` and weapon-hit forwarding of the exact hit token.
- Defense tokens `"shieldblock"`, `"parry"`, `"glance"`, and `"dodge"`.
- Shield tokens `"shieldpunch"`, `"shieldcharge"`, and `"shieldslam"`, plus mounted
  `"charge"`.
- Shop and quest secondary calls with unchanged incoming context and nonzero propagation.
- Shop-over-original and quest-over-shop-over-original composition.
- Direct-call and boot-level `no_specials` behavior applicable to these paths.

### Out Of Scope (Deferred)

- Typed runtime contexts, gateways, invalidation results, or dispatch rewrites.
- Unsafe traversal or owner-lifetime changes assigned to later gateway work.
- Registry metadata, OLC compatibility filtering, authored binding state, or provenance.

---

## 5. Technical Approach

### Architecture

Add one production-linked CuTest source with isolated world, object-index, mobile-index, shop, quest,
command-table, and `no_specials` fixtures. A recorder captures callback actor, owner, command,
argument, call count, nesting order, and configured return.

Invoke the item-display and weapon wrapper paths directly for exact runtime payload and return
evidence. Invoke `perform_violence()` with deterministic combat state for the mobile combat callback.
Invoke `shop_keeper()` and `questmaster()` directly with minimal valid tables for exact forwarding,
propagation, and manually installed production-wrapper nesting.

Use repository-rooted, bounded source-contract assertions for random or branch-heavy defense and
maneuver paths, ignored high-level weapon returns, combat ordering, assignment composition, and boot
gates. Each source assertion is bounded to the production function that runtime behavior cannot
safely isolate.

### Design Patterns

- Snapshot/restore fixture for mutable production globals.
- Exact callback recorder with configurable return and nesting trace.
- Deterministic combat-state fixture that disables unrelated attacks.
- Runtime callee evidence paired with narrow call-site source evidence.
- Bounded repository source reads rooted at `LUMINARI_TEST_ROOT`.

---

## 6. Deliverables

### Files To Create

| File | Purpose | Est. Lines |
|------|---------|------------|
| `unittests/CuTest/test_spec_combat_secondary.c` | Combat and secondary characterization | ~750 |

### Files To Modify

| File | Changes | Est. Lines |
|------|---------|------------|
| `Makefile.am` | Add the suite to CuTest compile and generated-test inputs | ~2 |
| `CMakeLists.txt` | Add the suite to `CUTEST_TEST_SOURCES` | ~1 |

---

## 7. Success Criteria

### Functional Requirements

- [x] Mobile combat callback payload, activation, ignored return, `no_specials` independence, and
  after-attacks/cleave ordering are fixed.
- [x] Identification and weapon-hit paths preserve exact signals and current return handling.
- [x] All four defense, three shield-maneuver, and mounted-charge tokens are fixed exactly.
- [x] Shop and quest secondary callbacks receive unchanged context and propagate nonzero results.
- [x] Shop-over-original and quest-over-existing nested composition is characterized.
- [x] Boot gating and direct-call `no_specials` behavior match the current implementation.

### Testing Requirements

- [x] New tests run from the generated root `cutest` executable.
- [x] Root `make test` passes, followed by a passing `make install`.
- [x] Test teardown leaves no mutated global state or root-level `circle` artifact.

### Non-Functional Requirements

- [x] No production callback ABI, dispatch, combat, shop, quest, or boot behavior changes.
- [x] Automake and CMake test membership remains synchronized.
- [x] Source reads and recorder strings are bounded and report setup failures safely.

### Quality Gates

- [x] All files are ASCII-encoded with Unix LF endings.
- [x] Code follows project conventions.
- [x] Zero new `-Wall -Wextra` warnings.

---

## 8. Implementation Notes

### Working Assumptions

- Combat notification callbacks use direct function pointers and discard their integer result.
- `perform_violence()` requires both `MOB_SPEC` and the mobile callback and does not consult
  `no_specials` at the combat callback site.
- Shop and quest wrappers call saved secondaries before their native command handling and return
  immediately on a nonzero secondary result.
- Shop assignment saves the existing callback before installing `shop_keeper`; quest assignment
  can then save that wrapper before installing `questmaster`.

### Key Considerations

- Do not drive random defense or maneuver outcomes merely to reach a callback token.
- Runtime wrapper tests must use command indices valid for the temporary command table.
- Manually composed wrapper nesting avoids mutating static command indices held by assignment code;
  assignment and boot precedence are verified in bounded production source regions.
- Restore all table pointers and counts even if a scenario records an assertion failure.

### Behavioral Quality Focus

Checklist active: Yes
Top behavioral risks for this session:
- A broad combat fixture can execute unrelated attacks or dereference incomplete character state.
- Shop and quest wrappers can fall through into native behavior unless the nonzero path or an inert
  command is chosen deliberately.
- Token checks can produce false confidence if they find an unrelated string outside the intended
  function, so every source assertion must be region-bounded.

---

## 9. Testing Strategy

### Production-Linked Tests

- Record item-identification and weapon-hit callback payloads with both zero and nonzero returns.
- Run a deterministic mobile combat turn with `no_specials` enabled and verify activation gates.
- Assert defense, maneuver, mounted-charge, and high-level ignored-return call sites in bounded
  production function regions.
- Exercise shop and quest saved secondaries independently with unchanged payloads and zero/nonzero
  results.
- Compose `questmaster -> shop_keeper -> original` with production wrappers and assert order,
  context, and nonzero propagation.
- Assert assignment preservation and `no_specials` boot gates against bounded production regions.

### Regression And Build Gates

- Run the generated production-linked suite through `make test`.
- Run a CMake build and CTest to detect manifest drift.
- Run formatting and focused static analysis for the new test source.
- Run `make install` and verify no root-level `circle` remains.

---

## 10. Security And Reliability

- No credential-bearing file is modified or emitted.
- Source reads are repository-rooted and size-bounded.
- Callback arguments are copied during invocation and never retained across fixture teardown.
- Mutable globals are restored exactly after each scenario.

---

## 11. Completion Definition

The session is complete when all tasks are checked, review has no unresolved blocker, validation and
security gates pass, the PRD/session state is updated, `make test` and `make install` pass, and the
committed result is published on the existing development branch.

Session complete. Run `plansession` for Session 04.
