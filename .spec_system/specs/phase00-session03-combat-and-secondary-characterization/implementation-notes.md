# Implementation Notes

**Session ID**: `phase00-session03-combat-and-secondary-characterization`
**Started**: 2026-08-06
**Base Commit**: 61886b718b1cc5934f85934ac144a3ef2fb83bb2
**Status**: Complete

---

## Planning And Prerequisites

- `check-prereqs.sh` passed for the development checkout, spec system, jq, and git.
- Session 01 and Session 02 are complete; `.spec_system/state.json` had no active session.
- The worktree was clean on `master` at the Session 03 base commit.
- Session 02 left all 495 production-linked tests passing.

## Retraced Compatibility Matrix

- `display_item_object_values()` resolves `obj_index[GET_OBJ_RNUM(item)].func` and calls a non-null
  pointer as `(ch, item, 0, "identify")`; both display branches discard the integer result.
- `weapon_special()` resolves the object callback directly, forwards `(ch, wpn, 0, hit_msg)`, and
  returns the callback value. `handle_successful_attack()` calls that wrapper for a wielded weapon
  or bare-hand gloves as a statement and therefore discards the wrapper result.
- `skill_message()` dispatches shield block, parry, armor glance, and dodge notifications with the
  defending character as actor, the reacting equipment as owner, command zero, and exact tokens
  `"shieldblock"`, `"parry"`, `"glance"`, and `"dodge"`. Each result is discarded.
- `perform_shieldpunch()`, `perform_shieldcharge()`, and `perform_shieldslam()` resolve the equipped
  shield callback after a successful hit and send their exact maneuver token. `perform_charge()`
  resolves the ridden mobile callback and sends `"charge"`. All four results are discarded.
- `perform_violence()` requires `MOB_SPEC`, a non-null callback, absence of `MOB_NOTDEADYET`, and
  positive hit points. It calls `(ch, ch, 0, "")` after the normal attack block and conditional
  cleave handling, discards the result, and does not inspect `no_specials`.
- `shop_keeper()` finds the shop by keeper rnum, calls `SHOP_FUNC` first with its incoming actor,
  owner, command, and argument unchanged, returns `TRUE` on any nonzero result, and otherwise falls
  through to shop behavior.
- `questmaster()` finds a quest by questmaster VNUM, calls `QST_FUNC` before quest-command handling
  with the incoming context unchanged, returns `TRUE` on any nonzero result, and otherwise falls
  through.
- `assign_the_shopkeepers()` saves a non-null non-shop callback in `SHOP_FUNC` before installing
  `shop_keeper`. `assign_the_quests()` similarly saves a non-quest callback in `QST_FUNC` before
  installing `questmaster`, allowing quest-over-shop-over-original composition.
- `boot_world()` skips shop loading under `no_specials`. `boot_db()` places mobile, shopkeeper,
  object, room, and questmaster assignment inside one `if (!no_specials)` block. Direct identify,
  combat, shop-wrapper, and quest-wrapper call sites contain no such gate.

## Behavioral Quality Focus

- Keep runtime combat setup deterministic and narrow enough to avoid unrelated combat actions.
- Bound source contracts to the intended production function, not a file-wide token occurrence.
- Snapshot and restore every mutable table pointer and count used by wrappers.
- Preserve exact incoming pointer identity as well as copied callback argument text.

## Implementation Log

- Created the Session 03 specification and 22-task checklist.
- Retraced every covered production caller, wrapper, assignment site, and boot gate at the Session
  03 base commit before implementing assertions.
- Added `unittests/CuTest/test_spec_combat_secondary.c` with snapshot/restore support for world,
  index, shop, quest, command, and `no_specials` globals plus a bounded exact callback recorder.
- Added runtime tests for item identification, weapon-hit forwarding, deterministic mobile combat
  turns, shop and quest secondary forwarding, zero fallthrough, and quest-over-shop-over-original
  nesting.
- Added bounded source-region tests for ignored high-level weapon returns, all defense/maneuver/
  charge tokens, mobile combat ordering and gates, assignment preservation, boot order, and
  `no_specials` gates.
- Added the new test source to both Automake memberships and CMake `CUTEST_TEST_SOURCES`.
- `make -j$(nproc) cutest` compiled the source with GNU C23 `-Wall -Wextra` and no warning; direct
  execution passed all 509 tests, including 14 new Session 03 tests.
- Independent CMake compilation and the `production-cutest` CTest passed after the final review
  repairs.
- Configured `clang-tidy` reports no warning in the new file; only three inherited padding
  advisories from unchanged `src/structs.h` remain.
- Formal review tightened callback-pointer and shop-load-boundary evidence. `code-review.md` records
  the two Low findings as fixed and has no open Critical, High, Medium, or Low finding.
- Full `make test` passed seven auxiliary checks and all 509 production-linked CuTests.
- Mandatory `make install` activated release `1fd8b0324adf44d61b3ca9d9088bb73692145c9d` and
  removed the root `circle` artifact.
- Checked-in world data retained digest
  `28d30cda73e9dd19e6ea1bf85260aefba0f621d6144401eda421a9fca2be2d98` across validation.
- Encoding, LF, formatting, manifest parity, protected-file, credential, security, and diff-hygiene
  checks passed. `validation.md` and `security-compliance.md` record PASS.
- No production code, world data, user-facing documentation, or credential-bearing file changed.

**Next command**: `plansession` for Session 04
