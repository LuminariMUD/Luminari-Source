# tbaMUD DG Mobile Damage Trigger Audit

Status: remediation complete

Research date: 2026-08-23

Remediation completed: 2026-08-23

## Executive conclusion

Welcor's Mobile Damage trigger is a useful interception point, but the upstream
patch is not safe to adopt verbatim. At the audited baseline, its central
contract was underspecified and had several surprising behaviors:

- a trigger body with no explicit `return` changes the pending hit to 1 damage;
- a `wait` reached before an explicit `return` also returns 1 to combat
  immediately, and a return after the wait is too late to affect that hit;
- zero-damage misses fire the trigger even though the feature is described as
  running when a mob is harmed;
- the returned amount is modified again by later combat defenses and caps, so
  it is not necessarily the damage the mob endures;
- upstream does not check whether the trigger killed or purged its owner or
  attacker before combat continues;
- upstream permits the supposedly mobile-only hook to run on players when
  player scripts are enabled; this can override the earlier immortal no-hassle
  protection;
- the shared OLC type count exposes a bogus option 21 for object and room
  triggers.

Luminari imported the feature in commit `f272409702c081ba0c000e25a3a821ee1469a3aa`
and had already fixed several upstream runtime hazards. The remediation in this
repository now also fixes the blocking OLC regression, makes no-return and wait
behavior safe, limits the event to positive pending damage, exposes complete
combat metadata, documents the actual interception boundary, and supplies the
missing automated and live builder coverage.

The release gate defined by this audit is met. The implementation should be
retained; upstream PR #151 still should not be cherry-picked verbatim into
another codebase.

## Remediation result

The detailed findings below retain the 2026-08-23 pre-remediation evidence for
audit history. Their disposition in the current implementation is:

| Finding | Resolution and evidence |
|---------|-------------------------|
| DTRIG-001 | Fixed. `trigedit` accepts mobile option 21, and CuTest plus the live builder smoke test prove select, clear, save, and reload behavior. |
| DTRIG-002 | Fixed. OLC derives a separate sentinel-bounded count for mobile, object, and room arrays; object and room menus stop at 20. |
| DTRIG-003 | Fixed. Driver status distinguishes a valid explicit return from the driver's ordinary default; no return and `halt` preserve pending damage. |
| DTRIG-004 | Fixed and documented. Yield status is reported separately; a wait before a result preserves pending damage and logs a warning, while a post-resume return cannot revise the completed hit. |
| DTRIG-005 | Retained and extended. Owner purge and participant death/position checks cancel the outer path; owner/actor purge and lethal DG-damage cases are tested. |
| DTRIG-006 | Retained and tested. Driver errors preserve pending damage; invalid values below -1 log and cancel. |
| DTRIG-007 | Retained and tested. Only NPC victims are eligible, even when player scripts exist. |
| DTRIG-008 | Resolved as positive-damage-only. Zero-damage misses do not fire the hook. |
| DTRIG-009 | Resolved by a precise scope contract. Help and web/system documentation state that only positive damage routed through `damage()` is intercepted and enumerate exclusions. |
| DTRIG-010 | Resolved as a pre-mitigation contract. The returned value replaces pending damage; later defenses, redirects, bonuses, and `cap_combat_damage()` may change final HP loss. Tests cover resistance and the cap. |
| DTRIG-011 | Fixed compatibly. `%attackid%`, `%attackname%`, `%damagetype%`, `%damagetypename%`, `%attackmodeid%`, and `%attackmode%` supplement legacy `%attacktype%`. |
| DTRIG-012 | Fixed. Flat-file and development-database help, SQL migration/verifier, web and system references, and a real `0 u 100` minimal-world example now ship together. |
| DTRIG-013 | Fixed. Production-linked CuTest covers OLC, serialization, chance/order, all return classes, waits, scope, metadata, lifecycle, detachment, mitigation/cap ordering, docs, SQL, and the real world example. The existing E2E test now loads that example. |
| DTRIG-014 | Fixed. The shared global count was removed, bounded formatting remains, declarations and API comments match the implementation, and the synchronous result contract is explicit. |

## Scope and sources

### Upstream feature

- [tbaMUD PR #151](https://github.com/tbamud/tbamud/pull/151), opened by
  `welcor` and merged 2025-07-02.
- [Merge commit `bdaca46`](https://github.com/tbamud/tbamud/commit/bdaca46e794404d3fd1227f9bc5a85405b119276).
- Upstream current master at audit time:
  `03d7ba7a48495b270e113821bc59b3cbde43c4b0`. The Damage implementation is
  unchanged from the merge commit.

The upstream patch changed only:

- `src/constants.c`
- `src/dg_olc.h`
- `src/dg_scripts.h`
- `src/dg_triggers.c`
- `src/fight.c`

It did not add tests, a builder help entry, a manual update, a sample trigger,
or a changelog entry. GitHub records no PR review, review comment, issue
comment, status, or check run for the merge.

### Luminari adoption

Authority for the current Luminari state is master commit
`370f84dad272e4d4c47f2913bad22fbf99dfb34f` at audit time, especially:

- `src/dgscript/dg_triggers.c:706`
- `src/combat/fight.c:5857`
- `src/dgscript/dg_olc.c:283`
- `src/dgscript/dg_scripts.c:2798`
- `src/dgscript/dg_scripts.h:54`
- `src/constants.c:3081`
- `unittests/CuTest/test_gameplay_e2e.c:1401`
- `lib/text/help/help.hlp:26306`, `31580`, and `34859`
- `docs/web/assets/js/dg-reference.js:27`
- `docs/web/dg-scripts/trigger-types.html:83`

The original research did not query or change the production database. During
remediation, the new idempotent component
`sql/components/help_dg_damage_trigger.sql` was applied only to the local
development database. `sql/components/verify_help_dg_damage_trigger.sql`
passed all four checks: one help entry, three keywords, no orphan ownership,
and all ten required content markers. Production was not accessed.

### Verification performed

The upstream current master was configured and compiled successfully with its
documented build process. Its full current Unity suite passed: 124 tests, zero
failures. None of those tests references `MTRIG_DAMAGE` or
`damage_mtrigger()`, so this proves general build health, not feature behavior.

The audit traced the hook, driver return processing, wait processing, character
extraction, OLC display and input handling, trigger serialization, all shipped
world trigger records, builder manuals, in-game help source, Luminari web guide,
and Luminari's one Damage-trigger integration test.

Remediation verification added and passed:

- the full production-linked suite (`make test`): 820 tests, zero failures;
- the focused protocol parser suite: 29 tests, zero failures;
- installation through `make install`, leaving no root-level `luminari` artifact;
- DG documentation and generated-world consistency checks;
- the local SQL help verifier, with all four checks passing;
- live help lookups for all three Damage-trigger aliases;
- a development-server OLC smoke test that created VNUM 11885, selected option
  21, saved, re-opened, edited, attached it to mobile 11850, fired it in combat,
  observed `return 0`, detached it, and saved again.

The temporary live-smoke prototype and attachment were removed afterward. OLC
rewrote the ignored zone mobile file during the test, so that file was restored
byte-for-byte from the repository's pre-apply development backup before the
server was stopped cleanly.

## Audited baseline code contract

The upstream execution order is:

```text
corpse / PK / peaceful / protected-mob checks
  -> immortal no-hassle changes pending damage to 0
  -> damage_mtrigger()
  -> start combat and apply memory/follower state
  -> sanctuary halves qualifying damage
  -> clamp damage to 0..100
  -> subtract hit points, award damage XP, message, and process death
```

The Luminari order is longer:

```text
PvP check
  -> deflective-screen and artifact reductions
  -> corpse / peaceful / protected-mob / mission checks
  -> immortal no-hassle changes pending damage to 0
  -> damage_mtrigger()
  -> start combat and update relationship/memory/hunting state
  -> damage_handling() defenses and reductions
  -> later bonuses and cap_combat_damage()
  -> redirects or further reductions where applicable
  -> subtract hit points and run post-damage effects
```

For an eligible owner, `damage_mtrigger()` walks attached triggers in list
order. The first idle Damage trigger whose Numeric Arg chance roll succeeds is
run, and no later Damage trigger is considered. If no roll succeeds, the
original pending damage is returned.

The event variables are:

| Variable | Actual value |
|----------|--------------|
| `%self%` | The trigger owner. |
| `%actor%` | UID of the character passed as the damage source. For poison, suffering, and other self-damage, this can equal the victim. |
| `%victim%` | UID of the trigger owner; normally redundant with `%self%`. |
| `%damage%` | Pending integer damage at the hook point, not guaranteed final HP loss. Luminari exposes `MAX(0, dam)`; upstream exposes `dam` directly. |
| `%attacktype%` | A spell/skill name when the numeric attack identifier maps through `skill_name()`; physical weapon attacks are `UNDEFINED`. |

The Numeric Arg is a 0-100 activation chance in OLC. The flat-file loader does
not validate the range, so hand-authored values at or above 100 always fire and
values at or below 0 never fire.

The intended return meanings are `-1` to cancel before combat starts, `0` for
a miss, and a positive replacement amount. The actual edge behavior is covered
below.

## Audited baseline findings

### DTRIG-001 - Luminari cannot select Damage in `trigedit` (high)

Luminari displays `NUM_TRIG_TYPE_FLAGS` entries. That constant is 21, so the
mobile menu displays Damage as option 21 (`src/dgscript/dg_olc.c:303`). The
input parser uses:

```c
else if (i >= 0 && i < NUM_TRIG_TYPE_FLAGS)
```

After the separate zero case, this accepts 1 through 20 only
(`src/dgscript/dg_olc.c:429`). Entering 21 redraws the menu without setting bit
20. The existing integration test bypasses OLC by parsing the hand-written
record `0 u 100`, so it cannot detect this defect.

Impact:

- builders see a feature they cannot enable;
- an existing hand-authored Damage flag can be removed accidentally and cannot
  be restored through the same editor;
- the successful parser/runtime test gives false confidence about the builder
  path.

Fix the check with a per-attachment count and accept `1 <= i && i <= count`.
Do not merely restore the upstream global upper-bound check, because that has
the cross-type defect in DTRIG-002.

### DTRIG-002 - one global OLC count creates bogus room/object option 21 (medium)

The upstream patch changed the single `NUM_TRIG_TYPE_FLAGS` from 20 to 21. The
same loop is used for mobile, object, and room type arrays. Mobile index 20 is
`Damage`, but object and room index 20 is their `"\n"` sentinel. Upstream
therefore displays a malformed/blank option 21 for objects and rooms and allows
it to toggle bit 20, even though no `OTRIG_*` or `WTRIG_*` event owns that bit.

Luminari still displays the sentinel as option 21 for objects and rooms, but its
off-by-one input check happens to prevent selecting the bogus option while also
preventing the valid mobile option.

Use separate counts (21 mobile, 20 object, 20 room), derive them from arrays
that do not include the sentinel, or stop iteration at the `"\n"` sentinel.

### DTRIG-003 - omitted return silently replaces damage with 1 (high)

Both upstream and Luminari initialize `script_driver()`'s result to 1. A
`return N` command changes that value, but `return` is not mandatory and does
not terminate the script. `damage_mtrigger()` treats the driver's result as
replacement damage without knowing whether any return command ran.

Consequences:

- an echo-only or logging-only Damage trigger changes every qualifying hit to
  1;
- a zero-damage miss becomes a 1-damage hit upstream;
- `halt` before a return also yields 1;
- a builder who thinks return is optional can radically weaken or strengthen
  an encounter without a script error.

The web guide says the driver starts with a true result and that Damage uses the
returned integer, but neither it nor the in-game help states the critical
consequence: a firing Damage trigger must explicitly preserve `%damage%` if it
does not intend to replace it.

The robust fix is for the hook to distinguish "no explicit return" from an
explicit `return 1` and preserve the original damage in the former case. A
specialized default result equal to the pending damage is another option, but
it must not silently change default returns for all other trigger families.

### DTRIG-004 - waits cannot asynchronously determine the current hit (high)

When the driver reaches `wait`, it schedules a resume and immediately returns
its current result to the combat caller. With no earlier return, that result is
1. Combat completes before the trigger resumes; a later `return` cannot revise
already-applied damage.

This is a synchronous interception hook, so its control result cannot safely
span a wait. At minimum:

- if the trigger waits before an explicit return, preserve the original pending
  damage rather than using 1;
- document that post-wait returns do not affect the initiating hit;
- consider logging a builder-facing warning whenever a Damage trigger yields
  before establishing its synchronous result.

An explicit `return %damage%` before `wait` can preserve the current behavior,
because DG `return` only sets a value and does not stop execution, but relying
on that subtlety is poor builder ergonomics.

### DTRIG-005 - upstream continues after trigger-side purge or death (high, fixed in Luminari)

A mobile script can purge itself, purge an NPC attacker, or inflict lethal DG
damage while the hook runs. Upstream `extract_char()` marks characters for
delayed extraction; it does not immediately free them. `script_driver()` also
tracks owner purge with `dg_owner_purged`, but the new upstream wrapper ignores
that state and the caller does not check `DEAD(actor)` or `DEAD(victim)`.

The outer `damage()` path can therefore continue to start combat, update
followers and memory, subtract HP, and process death using participants already
marked for extraction. If the trigger already killed its owner through direct
DG damage, the outer path can reach death handling again and risk duplicate
death/corpse/quest side effects. Delayed extraction prevents this from being an
immediate free-memory use-after-free in the traced implementation, but the
post-trigger state is still invalid.

Luminari correctly checks `dg_owner_purged`, `DEAD(actor)`, and `DEAD(victim)`
after the driver and checks both participants again in `damage()`. Preserve
that hardening.

Coverage is still missing for self-purge, lethal self-DG-damage, actor purge,
and actor death.

### DTRIG-006 - upstream mishandles driver errors and invalid negatives (medium, fixed in Luminari)

`script_driver()` can return `SCRIPT_ERROR_CODE` (`-9999999`), notably when
maximum recursion depth is exceeded. The upstream caller recognizes only exact
`-1`. Any value below -1 proceeds into combat, is eventually clamped to zero,
and produces a miss after combat has started. This hides the script failure and
does not match the documented valid return set.

Luminari improves the contract:

- `SCRIPT_ERROR_CODE` is logged and preserves the original damage;
- values below -1 are logged and converted to cancellation;
- `damage()` cancels on any negative wrapper result.

Keep these changes. Add tests so a future driver error cannot silently become a
miss again.

### DTRIG-007 - upstream mobile-only claim is not enforced (high upstream, fixed in Luminari)

Upstream supports player-attached scripts when `CONFIG_SCRIPT_PLAYERS` is
enabled (`src/dg_scripts.c:939`), but `damage_mtrigger()` checks only whether
the victim has the Damage bit and is not charmed. It does not require
`IS_NPC(victim)`.

This contradicts the PR's mobile-only contract. More importantly, `damage()`
sets damage to zero for an immortal with no-hassle and then calls the trigger.
A Damage trigger attached to that player can return a positive number and
restore damage that the immortal protection just canceled.

The option is disabled by default upstream, which reduces exposure but does not
repair the contract. Luminari's explicit `!IS_NPC(victim)` early return fixes
this and should remain.

### DTRIG-008 - the hook fires on misses, not only harm (medium)

The normal upstream hit path calls `damage(..., 0, ...)` when an attack misses.
Luminari has the same central miss path plus many skill-specific zero-damage
calls. `damage_mtrigger()` does not require `dam > 0`, so a Damage trigger rolls
and can turn a miss into positive damage. With DTRIG-003, a firing body with no
explicit return turns a miss into 1 damage automatically.

This may be a useful interception capability, but it is inconsistent with
"every time the mob is harmed." Decide and document one of two contracts:

- `Damage`: fire only when pending damage is positive; or
- `Damage Attempt`: fire on misses and allow the trigger to convert them.

Tests must cover both a zero-damage input and an ordinary hit.

### DTRIG-009 - "through any means" is false (medium)

The hook sees only damage routed through the combat `damage()` function. DG's
own `mdamage`, `odamage`, `wdamage`, and mapped `%damage%` commands call
`script_damage()`, which subtracts HP directly. DG character-field mutation of
`hitp`, direct HP assignments, and `raw_kill()` paths also bypass the hook.

Other exclusions include:

- charmed mobs, explicitly skipped by `damage_mtrigger()`;
- protected mobs and peaceful-room attempts, rejected before the hook;
- any Damage trigger instance already running or waiting;
- all but the first attached Damage trigger whose chance roll succeeds.

The feature description should say "damage attempts routed through
`damage()`" and enumerate exclusions. If the design truly requires every HP
loss source, direct mutation helpers need consolidation behind a common event
boundary; merely adding more scattered calls would make ordering and recursion
harder to reason about.

### DTRIG-010 - return value is pending damage, not guaranteed HP loss (medium)

Upstream applies sanctuary after the hook and then clamps to 0..100. A trigger
return of 80 becomes 40 against sanctuary; 500 becomes at most 100. This
contradicts the PR wording that a positive return is "the damage the mob will
endure."

Luminari has an even larger post-hook pipeline. `damage_handling()` can apply
concealment, mirror image, energy absorption, damage-type reduction, and other
defenses; later code applies bonuses and `cap_combat_damage()`. Thus its web
guide's phrase "return replaces the damage amount" is true only for the
intermediate pending value, not final HP loss.

Choose a stable semantic:

- move the hook immediately before HP subtraction if builders must control the
  final HP delta; or
- keep it as a pre-mitigation interception point and name/document the variable
  as pending damage, including which mitigation has already happened and which
  still follows.

The second option is less invasive but must be explicit, especially in
Luminari where two reductions already occur before the hook and many occur
after it.

### DTRIG-011 - attack metadata is too lossy (medium; larger Luminari impact)

`%attacktype%` is a display string only. Upstream intentionally reports
`UNDEFINED` for physical weapon attacks because `skill_name()` recognizes
spells/skills, not the weapon-message range. No numeric identifier is exposed.

Luminari inherited that contract even though its combat call carries separate
`w_type`, `dam_type`, and attack-mode/offhand information. All ordinary
physical weapon attacks are forced to `UNDEFINED`, and the elemental/physical
damage type is not passed at all. A builder cannot reliably distinguish fire
from cold, slashing from piercing, ranged from melee, or primary from offhand
damage at this hook.

Add stable, separately named variables rather than overloading one string:

- numeric attack/source identifier;
- human-readable attack/source name;
- numeric and readable damage type;
- attack mode (primary, offhand, ranged) where available.

Backward compatibility can retain `%attacktype%` while documenting its limited
meaning.

### DTRIG-012 - builder-facing documentation is absent upstream and incomplete in Luminari (high)

Upstream omissions:

- `lib/text/help/help.hlp` has no `TRIGEDIT-MOB-DAMAGE` entry;
- the `MOB-TRIGGERS` numbered list stops at Door and omits even Time and Damage;
- the general trigger-type table stops at Mobile Time;
- `doc/building.txt` stops at Mobile Time;
- `doc/coding.txt` describes `damage()` without the new hook;
- there is no changelog entry or example trigger.

The shipped trigger named `Damage trigger` at VNUM 1285 is unrelated: it is a
room Enter trigger (`2 g 100`) demonstrating the `%damage%` command. It is not
a Mobile Damage event trigger. No shipped `.trg` record in upstream or
Luminari uses the mobile Damage bit `u`.

Luminari omissions:

- `lib/text/help/help.hlp` still has no `TRIGEDIT-MOB-DAMAGE` entry;
- its `MOB-TRIGGERS` list also stops at Door;
- its general trigger-type table stops at Mobile Time;
- no repository SQL component creates/updates a database help entry or
  keywords for Damage;
- the web reference lists variables and replacement behavior but does not warn
  that omitted return defaults to 1, explain synchronous wait behavior, define
  miss handling, list exclusions, or explain pre/post mitigation ordering;
- no usable world example exists.

Repository policy requires help updates in both the database and
`lib/text/help/help.hlp`. A complete documentation change should update those
two sources plus the web guide and add an actual `0 u 100` example. Use distinct
keywords such as `TRIGEDIT-MOB-DAMAGE`, `MOB-DAMAGE-TRIGGER`, and
`MTRIG-DAMAGE` so searches do not collide with the existing `%damage%` command.

### DTRIG-013 - test coverage is absent upstream and narrow in Luminari (medium)

The upstream suite has no Damage-trigger test. Luminari has one valuable
production-linked integration test that parses `0 u 100`, runs a trigger with
`return 5`, confirms `%damage%` was 17, and confirms the victim loses 5 HP. It
does not exercise OLC and covers only the simplest successful return.

Required regression matrix:

| Area | Cases |
|------|-------|
| OLC | Mobile option 21 sets/clears bit 20; room/object menus stop at 20; save/reload writes and reads `u`. |
| Chance | 0, 100, and multiple attached Damage triggers in list order. |
| Default | Empty/echo-only body preserves original damage under the chosen contract. |
| Return | `-1`, `0`, `1`, ordinary positive, very large positive, invalid `< -1`, and driver error. |
| Wait | Wait before return, explicit result before wait, and proof that a resumed return does not revise completed combat. |
| Inputs | Zero-damage miss, spell, skill, physical weapon, self-damage, and charmed mob. |
| Lifecycle | Owner self-purge, owner death via DG damage, actor purge/death, and detached trigger. |
| Ordering | Mitigation before/after hook and damage cap behavior. |
| Scope | NPC victim fires; player victim never fires in Luminari. |
| Variables | Actor, victim/self, pending damage, attack name/ID, damage type, and attack mode. |
| Documentation | Static presence/keyword checks for file help and SQL help source. |

### DTRIG-014 - minor implementation quality issues (low)

The upstream addition uses `sprintf()` for integer formatting even though the
project already has `snprintf()`. The present buffer is large enough for the
specific integer/UID values, so no independent overflow was established, but
new code should follow bounded-string practice.

The upstream header names the first parameter `ch` while the implementation
calls it `actor`, and the function has no API comment documenting ordering,
chance, default result, waits, liveness, or post-processing. The added constant
also left the unused bit 18 implicit. None is severe alone, but together they
made the actual contract harder to review and helped the documentation and OLC
errors survive.

## What Luminari had already improved at the audited baseline

The Luminari port should not be replaced wholesale with upstream. It contains
material hardening that the upstream current master still lacks:

| Improvement | Luminari behavior |
|-------------|-------------------|
| Owner scope | Requires non-null actor/victim and an NPC victim. |
| Formatting | Uses `snprintf()` and initializes the local buffer. |
| Damage variable | Exposes a non-negative value. |
| Attack-name safety | Bounds-checks the source identifier and deliberately maps weapon identifiers to `UNDEFINED`. |
| Owner address | Passes a dedicated local owner pointer to the driver. |
| Lifecycle | Cancels if the owner was purged or either participant is marked dead. |
| Driver error | Logs and preserves original damage. |
| Invalid negative | Logs and cancels instead of allowing the value to become a miss later. |
| Caller | Cancels on any negative result and rechecks participant death. |
| Integration proof | Parses a `u` trigger and proves explicit positive replacement and HP loss. |

At the audited baseline, these changes directly addressed DTRIG-005,
DTRIG-006, and DTRIG-007. The remediation result above records how the remaining
findings were resolved.

## Completed remediation sequence

1. Repaired `trigedit` with per-owner type counts and added OLC/serialization
   tests.
2. Defined synchronous result semantics, preserving pending damage when no
   valid explicit return exists and reporting wait behavior.
3. Retained Luminari's lifecycle, scope, error, and invalid-negative hardening
   and covered the branches with production-linked tests.
4. Defined the hook as positive pending damage before later mitigation and
   documented its exact boundaries.
5. Extended attack metadata for damage type and attack mode without changing
   legacy `%attacktype%`.
6. Updated flat-file and database help, added the SQL verifier, completed the
   web and long-form references, and added a real `0 u 100` world example.
7. Passed the full suites and completed the manual create, select, save,
   reload, attach, fire, edit, detach, and cleanup smoke test.

## Adoption decision

Do not cherry-pick upstream PR #151 into another codebase unchanged.

For Luminari, retain the remediated implementation. The minimum release gate
was:

- Damage can be selected and round-tripped through OLC without exposing a bogus
  object/room option;
- no-return and wait semantics cannot silently turn damage into 1;
- return, error, purge/death, zero-damage, and ordering cases have automated
  coverage;
- both in-game help sources and the web guide describe the actual contract;
- one real builder example exists and has been exercised in game.

Every item is now satisfied. The feature is ready for builder use under the
documented synchronous, positive-pending-damage contract.
