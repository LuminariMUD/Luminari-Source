# Full Command Sweep Findings - 2026-08-15

Status: resolved in development; verification complete.

This document records a local-development clean-build and live staff-command sweep. It is an
observation and triage record, not a statement that every command's game mechanics are correct.

## Result

The clean build installed and booted successfully. The installed server remained alive after the
sweep and continued listening on port 4100. No server crash occurred during command execution.

## Resolution

All 19 findings were repaired on 2026-08-15. The original observations below remain unchanged as
the evidence that motivated the work.

| Finding | Resolution |
|---------|------------|
| F01 | `objcheck` is read-only by default, has an explicit bounded repair mode, and reports checked, repaired, and unresolved objects separately without one log line per runtime object. |
| F02 | Jotunheim special assignments and saved-object references use the restored 196xxx identities, generated-vessel triggers 70001 and 70002 are present, and a fresh syntax boot contains none of the reported missing-reference signatures. |
| F03 | Command, feat, and shop listings use dynamically sized/pageable output, and fixed-width columns always retain a separator. |
| F04 | Raw command text enters the action queue only after a registered non-mutating preflight validates its current arguments; unsupported commands must be retried when their action recovers. |
| F05 | Cooldown registry entries without daily-use metadata use the countdown callback, and a registry-wide regression prevents invalid daily-use bindings. |
| F06 | Exit 52 is a named planned reboot in both server and supervisor behavior; it no longer increments crash accounting or creates crash artifacts. |
| F07 | Command and catch-up telemetry now records command identity, pulse backlog, exhausted budgets, and recovery behavior for actionable latency diagnosis. |
| F08 | Persistent pets reload both their saved names and prototype keywords, restoring room lookup and targeted staff cleanup. |
| F09 | `zcheck` armor rules use designated wear-slot initializers and skip undefined rules safely. |
| F10 | Player debug lines were removed, invalid enum zeroes display as `none`, and score currency uses shared formatted accessors. |
| F11 | `immtitle`, `setmaterials`, and `oconvert` reject missing arguments before mutation or lookup. |
| F12 | Unsupported `exempt` and `unconjure` registrations were removed, with command-table coverage for placeholder removal. |
| F13 | `stand` is available while sleeping so the handler's wake-up guidance is reachable. |
| F14 | Tame, guard, and charm/dominate paths reject self-targeting before applying state. |
| F15 | Board lookup consistently resolves the room board and safely handles missing board metadata, with production-linked board regressions. |
| F16 | AI startup reports `healthy`, `degraded`, or `unavailable` and identifies the active provider instead of claiming unconditional success. |
| F17 | I3 can be disabled, reconnects use bounded exponential backoff, supervisor state is refreshed independently, and the watchdog verifies exact process or listener identity before restart. |
| F18 | List reset diagnostics, feat bounds, `last`, no-clan investment, and route/waypoint traversability were guarded and regression-tested. |
| F19 | Toggle copy, command grammar, summon/heal text, branding, contacts, feat listing, Spell Recall, and registered placeholders were corrected; authoritative database help now documents the changed behavior. |

Verification completed with a warning-free parallel build, 725 production-linked CuTests, all
shell supervision/deployment tests, 29 focused protocol-parser tests, two idempotent applications
and a passing verifier for the database help migration, `make install`, and a clean syntax-check
boot. The syntax boot contained no missing mobile/object/trigger, prevented house-object, invalid
`SPEC`, 20960, 70001, or 70002 diagnostic from F02. The installed release is available through
`bin/circle`; the root-level `circle` build artifact is absent.

The sweep exercised 946 of the 985 command-table entries available in the source, including aliases
and duplicate registrations. The other 39 entries were deliberately skipped because they enter an
editor/menu or perform destructive lifecycle, persistence, or world-control operations.

The run exposed several high-value defects:

- `objcheck` blocked command processing for about nine seconds, logged 167 objects with rnum `-1`,
  and claimed the errors were corrected even though that branch does not correct those objects.
- Fresh boot logged missing special-procedure targets, generated-ship triggers, house objects, and
  invalid mobile `SPEC` flags.
- Large command and data listings truncate or concatenate entries.
- The interpreter can enqueue an action before the command handler validates its arguments.
- A daily-use cooldown event reached its callback with a NULL `sVariables` field.
- An intentional reboot exits with code 52, but `autorun.sh` records every nonzero code other than
  134 and 139 as an unexpected process failure.

## Build and runtime identity

| Item | Value |
|------|-------|
| Environment | Development (`APP_ENV=development`) |
| Source commit | `6cde92b9d1f305f3e0ecaf2f57f8878835c5b984` |
| Source dirty at build | No |
| Build | `make clean`, `make -j$(nproc)`, `make install` |
| Build result | Successful; no warnings observed in the captured build output |
| Installed binary | `bin/releases/49d2c54e7679b2bd94076dd818be008bd9fccef5/circle` |
| SHA-256 | `d3cf18f6bb937bc110a1ea6379987f0b31298f31e1ea22465b10ee20396e25cf` |
| Runtime | PID 312236, port 4100 |
| Staff character | Kohdee, level 34 |
| Primary room | 1204, Staff Simplex |

`make test` was not part of this request and was not run. Runtime verification used the newly
installed binary, not the root build artifact. The root-level `circle` artifact was absent after
`make install`.

## Coverage

The authoritative inventory came from the preprocessed `complete_cmd_info` table rather than the
live `commands` display, because the live display itself truncates. Coverage counts are distinct
source entries, so aliases and duplicate registrations count separately.

| Surface | Source entries | Invoked | Deliberately skipped |
|---------|---------------:|--------:|---------------------:|
| Regular, levels 0-30 | 792 | 789 | 3 |
| Privileged, levels 31-34 | 193 | 157 | 36 |
| Total | 985 | 946 | 39 |

Coverage included:

- All 14 direction spellings, returning to room 1204 after each attempt.
- Live `commands` and `wizhelp` enumeration.
- Paged list entry points, with paging exited after validation rather than traversing every page.
- Stateful toggles in pairs where safe, followed by an explicit final-state cleanup.
- 62 regular commands with the simple self target `Kohdee` where a target was accepted.
- Staff self-target checks for `astat`, `goto`, `last`, `mute`, `notitle`, `pardon`, `players`,
  `restore`, `snoop`, `stat`, `switch`, `thaw`, `transfer`, and `unaffect`.
- `account` with its output suppressed from the captured transcript to avoid retaining private
  account data.
- `purge` and `zreset` only as controlled cleanup in Staff Simplex after the summon tests.

The three skipped regular entries were `prefedit`, `study`, and `write` because each enters an
interactive menu/editor.

The 36 skipped privileged entries were:

```text
aedit analyzeworld bedit cedit copyover craftedit hedit hlqedit hsedit iedit
medit msgedit oedit qedit redit saveall saveeverything saveobjstodb sedit
setroomdesc setroomflags setroomname setroomsect settime setweather setworldsect
shutdow shutdown tedit trigedit wizlock wizupdate zedit zlock zpurge zunlock
```

These entries open OLC/editors, perform broad analysis or persistence writes, control process
lifecycle, alter global time/weather, or lock/purge world content. The old process received one
intentional fast reboot to load the new binary; fresh-binary lifecycle commands remained skipped so
the requested server would stay up.

## Findings

### F01 - `objcheck` is slow, noisy, mutating, and overstates repair (high)

At 11:58:53, performance telemetry reported 10.955 seconds of pulse usage, including 8.985 seconds
in command processing. At 11:59:03, `objcheck` emitted 167 identical errors:

```text
SYSERR: Object with invalid rnum -1 found in object_list
```

The player-facing summary said that 167 errors were "found and corrected." In
`src/act.wizard.c:11047-11099`, invalid-rnum objects only increment `errors` and log; they are not
corrected. The same command also rewrites `obj_index[i].number` for count mismatches, despite its
check-oriented name. It therefore combines an expensive full scan, mutations, a log storm, and a
misleading repair count.

Recommended direction: split check and repair modes, identify or explicitly exempt valid runtime
objects with `NOTHING` rnums, report corrected and uncorrected counts separately, and make the scan
incremental or move it outside the game loop.

### F02 - Fresh boot has unresolved world-data references (high)

Before entering the game loop, the new binary logged:

- 4 missing mobile special-procedure targets: 2096052, 2096033, 2096032, and 2096200.
- 9 missing object special-procedure targets: 2096012, 2096059, 2096062, 2096056, 2096081,
  2096090, 2096066, 2096073, and 2096087.
- 22 generated ship rooms unable to attach triggers 70001 or 70002.
- 5 prevented house-object loads: 2096021, 2096009 three times, and 2096065.
- 10 runtime mobile `SPEC` errors across vnums 196027, 196032, 196033, 196052, 196070, and
  196077. The server automatically removed those flags in memory.

The server completed boot, but the missing assignments and in-memory flag repair mean that the
running world differs from the persisted world definition.

Recommended direction: reconcile the assignment inventory and flat-file/database content, restore
or intentionally remove the ship triggers, repair house inventories, and remove or correctly bind
the six mobile `SPEC` flags in persistent data.

### F03 - Large listings truncate or concatenate command names (high)

Observed examples included:

- `featlisting` ending in `**OVERFLOW**` near feat 1017.
- `shoplist` ending in `**OVERFLOW**` near shop 27563.
- `commands` and `wizhelp` visually joining adjacent 14-character entries, including
  `db_init_systemdiscord` and `saveeverythingsaveobjstodb`.
- Narrow-screen `skills`, `skillset`, `weaponlist`, and `helpcheck` output joining adjacent values.
- Three `simple_column_list` width warnings in `syslog`.

`do_commands` gathers entries into a fixed 1000-pointer array and sends them through `column_list`
(`src/act.informative.c:8860-8947`). `column_list` builds the entire result in one
`MAX_STRING_LENGTH` buffer and replaces the tail with an overflow marker
(`src/utils.c:4068-4136`). Direct multi-line senders can also exhaust the descriptor output buffer.

Recommended direction: stream or page list rows without constructing one fixed-size result, add at
least one separator after a field that exactly fills its width, and show an explicit continuation or
range syntax instead of silently losing entries.

### F04 - Action queuing happens before command-specific validation (high)

When action resources were unavailable, many no-argument commands were accepted into the action
queue. Their missing-target or usage checks ran later, delaying unrelated markers and causing the
harness to time out while a valid session remained alive.

The interpreter enqueues solely from action availability at `src/interpreter.c:6314-6329`; it calls
the handler, where argument validation normally lives, only afterward at line 6334.

Recommended direction: provide a non-mutating validation callback for every queued action and run it
before enqueue, or let handlers build validated action records rather than queueing raw command
text.

### F05 - Daily-use cooldown event lost its variables (high)

At 11:38:08, `syslog` recorded:

```text
SYSERR: 1 sVariables field is NULL for daily-use-cooldown-event: 204
```

The guard is in `src/mud_event.c:326-368`. The sweep exercised several daily-use abilities and
disconnect/reconnect cycles, so this is evidence that at least one persisted or detached event can
reach the callback without its required serialized state.

Recommended direction: trace event 204 through creation, character save/unload, and reload; reject
creation without variables and add a production-linked disconnect/reload regression test.

### F06 - Intentional reboot is classified as a crash (high)

The controlled fast reboot used to load the new binary shut down cleanly and exited with code 52.
`autorun.sh` classified it as unexpected, incremented its crash accounting, wrote
`syslog.CRASH`, and created a structured last-error record.

`src/comm.c:787` intentionally uses exit code 52 for reboot. The supervisor recognizes only code 0,
139, and 134 at `scripts/autorun/autorun.sh:1922-1955`; every other code is treated as a process
failure.

Recommended direction: give clean reboot/copyover exits named constants shared by the server and
supervisor, and classify those codes as planned lifecycle transitions without crash artifacts.

### F07 - Command sweep exposed severe game-loop stalls and catch-up pressure (high)

Through the 12:23:30 audit cutoff, `syslog` contained 8 critical and 1 moderate performance reports,
plus 376 catch-up summaries. Of those summaries, 371 reported a nonzero exhausted budget. The worst
critical pulse was 10.955 seconds; its profile attributed 8.985 seconds to command processing.

Some load is specific to this unusually dense command sweep, but synchronous staff commands can
still block all players because the server has one command/game loop.

Recommended direction: correlate the other critical samples to their command names, establish a
staff-command latency budget, and move full-world scans and large formatted reports off the hot
loop or break them into bounded work units.

### F08 - Summoned undead became visible but could not be addressed consistently (medium)

`animatedead` correctly requires no corpse for the feat version; the feat description explicitly
documents that distinction. The resulting mummy appeared in the room, group, and pet display, and
was restored after reconnect. However, `where mummy` omitted it, `stat mummy` selected an unrelated
world mobile, and `purge mummy` returned `Nothing here by that name.` Cleanup required a room-wide
`purge` followed by `zreset .`.

The summon path is `src/act.other.c:705-786`. Recommended direction: verify the summoned prototype's
keywords and visibility/search flags, then add tests for room lookup, staff stat/purge targeting,
pet persistence, and targeted extraction.

### F09 - `zcheck` walks beyond its initialized armor-rule descriptions (medium)

`zcheck` repeatedly printed `Has AC 5 ((null) limit is 0)`. The loop uses
`TOTAL_WEAR_CHECKS` (`NUM_ITEM_WEARS - 1`) at `src/act.wizard.c:5954-5958`, while the explicit
`zarmor` initializer at lines 5688-5716 covers only the older set of wear slots. Remaining elements
are zero-initialized, including a NULL `message`.

Recommended direction: use designated initializers keyed by wear slot, skip rules with NULL
messages, and add compile-time or runtime completeness checks when wear locations change.

### F10 - Score surfaces leak debug text and invalid enum state (medium)

`skore` displayed `[DEBUG] display_score_section called...` for every section and another debug line
before experience. Those sends are unconditional in `src/act.informative.c:6639-6652`.

Both score surfaces also exposed `Energy Type: RESERVED`, and the sorcerer bloodline appeared as
`(none/RESERVED)`. `skore` reported bank gold differently from the standard `score` output. These
indicate either invalid saved defaults or inconsistent display accessors.

Recommended direction: remove player-facing debug sends, map unset enum zero values to `none`, and
make both score implementations use the same currency and character-state accessors.

### F11 - Several no-argument commands do not fail closed (medium)

- `immtitle` with no text cleared the staff title. Its handler passes the empty argument directly to
  `set_imm_title` at `src/act.other.c:7034-7050`.
- `setmaterials` printed usage and then continued to `There is no one by that name online.` because
  the usage branch lacks a return at `src/craft/crafting_new.c:5933-5944`.
- `oconvert` checks whether the argument pointer is NULL rather than whether the string is empty.
  With no arguments it entered conversion code as type 0 and reported zero conversions
  (`src/act.wizard.c:8868` onward).

Recommended direction: consistently reject `!argument || !*argument` before mutation and add empty,
whitespace-only, and malformed-argument tests for staff commands.

### F12 - Command registration and handler subcommands disagree (medium)

Invoking `exempt` and `unconjure` produced `do_consign_to_oblivion()` SYSERR entries for subcommands
8 and 9. Both are registered to that handler, but its switch supports only blank, forget, unadjure,
omit, uncondemn, uncommune, and discard (`src/magic/spell_prep.c:4872-4900`).

Recommended direction: implement the missing class mappings or remove/repoint the two command-table
entries, then add a table-driven test that every registered subcommand is accepted by its handler.

### F13 - `stand` cannot reach its sleeping-position message (medium)

From sleeping, `stand` returned the interpreter-level `In your dreams, or what?` rather than the
handler's `You have to wake up first!`. The command requires `POS_RECLINING` at
`src/interpreter.c:4068`, while `do_stand` explicitly handles `POS_SLEEPING` at
`src/movement/movement_position.c:118-120`. The interpreter rejects the command first.

Recommended direction: set the minimum position low enough for the handler's sleeping case or
remove the unreachable case and define the intended behavior in one place.

### F14 - Several self-target paths accept nonsensical state transitions (medium)

Examples from the requested simple-target pass:

- `tame Kohdee`: `You tame Kohdee. Kohdee tames you.`
- `guard Kohdee`: `You now guard Kohdee`.
- `dominate Kohdee`: `You like yourself even better!`.

The tame and guard handlers do not reject `vict == ch` (`src/act.other.c:3081-3124` and
`src/combat/act.offensive.c:10786-10816`). The charm engine has an explicit self-message rather than
rejecting self-targeting (`src/magic/spells.c:363-364`).

Recommended direction: define which abilities may self-target and enforce that policy in shared
target validation before applying affects or relationships.

### F15 - Board lookup is inconsistent in Staff Simplex (medium)

Six interactions logged:

```text
SYSERR: degenerate board! Character Kohdee in room #1204, board obj #3098
Board 0: vnum=2201, rnum=775
```

The room visibly contained its board after zone reset, but no-argument `read` fell through to room
display rather than giving board usage. Recommended direction: reconcile the legacy board rnum table
with MySQL board-object discovery and test commands against the actual Staff Simplex board object.

### F16 - AI startup reports success after its configured local model fails (medium)

Boot reported that Ollama model `qwen2.5:7b` was not found and warmup failed, then immediately logged
`AI Service initialized successfully.` The final line is unconditional at `src/ai_service.c:283`.

The in-game `ai` command showed that a fallback chain remained configured, so this may be degraded
rather than failed service. Recommended direction: report `healthy`, `degraded`, or `unavailable`
with the active provider rather than a blanket success.

### F17 - I3 reconnect and watchdog state create avoidable operational noise (medium)

By the audit cutoff, the disabled local I3 dependency at 127.0.0.1:8081 had produced 120 refused
connections and 240 ERROR lines, retrying roughly every 31-32 seconds.

At 12:20:01, the watchdog also treated the active autorun state as stale, attempted a restart, and
failed because another autorun instance was already running. It recovered at 12:21:04 without the
game process dying.

Recommended direction: use bounded/exponential I3 retry logging and make the development service
explicitly disableable. Refresh autorun liveness state independently of player activity and have the
watchdog confirm the process/listener before launching a duplicate supervisor.

### F18 - Additional runtime diagnostics require cleanup (medium)

The fresh log also contained:

- 3 `simple_list() forced to reset itself` SYSERR entries.
- 6 invalid feat lookups: feat 0 once and feat 1262 five times.
- 1 scheduled autopilot failure for ship 3 at `(-62, 83, 0)` because the ship could not occupy the
  destination.
- `last Kohdee` displaying `[34 (null)]`.
- `claninvest` reporting clan-data corruption for a character who is not in a clan.

Recommended direction: correlate the list and feat errors to their invoking display commands, guard
class/name lookups used by `last`, treat no-clan as a normal `claninvest` state, and validate route
waypoints before scheduling departure.

### F19 - Player-visible copy, formatting, and incomplete surfaces remain (low)

Observed items:

- `autoblast` first printed `(null)` and the paired toggle produced contradictory on/off text.
- Stored-consumables toggle text says `You will no use the stock consumables system`.
- `mission` omits a newline between `mission.` and `You may` (`src/quest/missions.c:315-318`).
- `weapontouch` says `You cannot queued a spell`.
- `dragbreath` can say `You exhale breathing out RESERVED!`.
- Children-of-the-night summon output included `Kohdee raises Kohdee!`; the summon message table
  contains `$n raises $n!` at `src/magic/magic.c:11632` and 11677.
- `layonhands` self-healing displayed a negative heal quantity.
- `classes` and `listraces` still use `Races of Krynn` branding
  (`src/character/race.c:6525`; `src/interpreter.c:8024,8287`).
- `staffevents statcap` exposes `TBD - need to check with Gicker :)`
  (`src/handler.c:829`).
- `spellrecall` consumes its cooldown while stating that full functionality is future work
  (`src/combat/act.offensive.c:8201-8211`).
- `spellquests` and `shipload` are registered commands that only report not implemented
  (`src/quest/hlquest.c:1323-1334`; `src/vessels/vessels.c:2701-2704`).
- Help output still contains an old `live.com` contact and raw URLs.
- `featlisting` exposes many `Unused Feat` entries.

Recommended direction: remove registered placeholders from normal discovery until functional, audit
campaign branding and contacts, and add golden-output tests for toggles and high-traffic command
responses.

## Fresh-log audit

The audit boundary was 2026-08-15 12:23:30 IDT. The active `syslog` began with the new release at
11:11:12. During the fresh run, the only other continuously updated project log was
`log/watchdog.log`. Logs rotated at 11:11:04 belong to the intentional old-binary reboot transition;
they were inspected separately because the supervisor labeled that reboot as a crash.

Counts in the active log through the boundary:

| Pattern | Count | Interpretation |
|---------|------:|----------------|
| `SYSERR:` | 226 | 40 boot references, 167 invalid-rnum objects, and 19 other runtime errors |
| `MOB ERROR:` | 10 | Six unique mobs with `SPEC` but no assigned procedure |
| `PERFMON [CRITICAL]` | 8 | Includes the 10.955-second pulse |
| `PERFMON [MODERATE]` | 1 | First-login pulse at 206.72 percent |
| `PERFMON [CATCHUP]` | 376 | 371 had nonzero `budget_exhausted` |
| I3 connection refused | 120 | Each attempt also emitted a second gateway ERROR line |
| EOF socket warnings | 19 | Expected harness disconnects, not game failures |
| Column-width warnings | 3 | Matches malformed narrow-screen listings |

`Losing player: <null>` entries and EOF warnings tracked short-lived harness sessions. They were not
classified as product failures because the process and listener remained healthy and later logins
succeeded.

## Cleanup and final state

The staff session was explicitly returned to a stable state:

- Room 1204, standing, full hit points and movement, five attacks.
- No inventory, no pets, and only Kohdee in the group.
- Vital Strike, flurry, total defense, spot, buildwalk, temporary affects, combat, and queued actions
  were cleared or disabled.
- Page length and screen width were restored to 40 and 80.
- Staff title restored to `[   Forger   ]`.
- PvP returned to disabled after its intentional 15-minute lockout.
- The summon-test mummy was removed and Staff Simplex was reset.
- The character quit successfully; the server remained online.

One minor persistent test-side change remains: `scrounge` created a tea item and junking it increased
Kohdee's gold from 89990 to 89991. The test did not edit source, configuration, credentials, or world
files.

## Suggested repair order

1. Correct `objcheck` semantics and the invalid-rnum object source; add a loop-latency regression.
2. Reconcile boot-time missing prototypes, generated-ship triggers, house objects, and mob specs.
3. Fix daily-use event serialization across character unload/reload.
4. Validate commands before queueing and repair the registered consign subcommands.
5. Replace fixed-buffer list rendering and repair narrow-screen separation.
6. Recognize planned reboot exit codes in `autorun.sh` and harden watchdog liveness checks.
7. Fix `zcheck`, score defaults/debug output, no-argument staff handlers, and self-target rules.
8. Clean up degraded-service status, reconnect noise, incomplete commands, branding, and copy.
