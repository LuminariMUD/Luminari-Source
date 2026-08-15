# Valgrind Full Command Audit - 2026-08-15

Status: resolved and verified on 2026-08-16; all five findings are closed.

This document records a clean local-development build, a live inventory-driven command sweep under
Valgrind Memcheck, a simple self-target pass, an idle observation window, controlled teardown, and a
separate normal-shutdown baseline. It is an observation and triage record, not proof that every
command's game mechanics are correct.

## Result

The clean build and install succeeded without compiler warnings. The live staff session exposed 982
command entries: 192 from `wizhelp` and 790 from `commands`. Thirty-nine editor, broad world or
persistence mutation, lock, and lifecycle entries were deliberately excluded. All remaining 943
entries were invoked.

Of those 943 entries:

- 938 completed through the normal command marker.
- 2 entered their expected character-menu transition (`qui` and `quit`).
- `hide` ran successfully but defeated the marker protocol and was recorded as a harness timeout.
- `inspirecourage` and `finalstand` independently stopped the game loop until the checkpoint
  watchdog aborted the process.

A separate pass submitted 65 commands with the simple target `Kohdee`; all 65 completed. After
state cleanup, the final instrumented command process remained healthy for 312 seconds, then
accepted the controlled fast shutdown. A fourth boot-only process used plain `shutdown` to
exercise the full normal cleanup path. That baseline reported zero definitely, indirectly, or
possibly lost bytes and no invalid-access or uninitialized-value diagnostics.

Local Ollama model absence and local I3 connection refusal are expected development-environment
conditions and are explicitly outside this report's findings.

## Resolution - 2026-08-16

All five findings were repaired in development without changing local Ollama or I3 behavior.

| Finding | Status | Resolution |
|---------|--------|------------|
| F01 | Resolved | Craft skill identifiers are validated centrally. `-1` is handled as an intentional no-skill requirement, while loaders, OLC, and runtime paths reject every other invalid identifier before accessing the skill array. |
| F02 | Resolved | `mag_groups` now uses a function-local merge iterator, so nested affect and group calculations cannot corrupt a global `simple_list` traversal. Player-plus-pet regressions complete for both Inspire Courage and Final Stand. |
| F03 | Resolved | Zone status labels use static storage, wild-shape modifiers are caller-owned, `whois` frees the duplicated account name, and executed action-queue nodes and arguments are freed after dispatch. |
| F04 | Resolved | Split Enchantment consistently uses perk ownership, exposes a clamped cooldown helper, treats expired timestamps as ready, and never reports negative remaining time. |
| F05 | Resolved | Pager boundaries now include descriptor capacity, existing queued output, protocol expansion, and prompt/footer headroom. Maximum 255-by-200 settings split output into safe pages without losing the final item. |

Production-linked regressions cover the no-skill craft, nested two-member group affects, executed
action ownership, perk/feat namespace disagreement, active and expired cooldowns, and maximum pager
settings. The authoritative release gate passed `make clean`, a warning-free parallel build, and
`make test-all`: 730 CuTests, 414 world-tool tests, 29 protocol-parser tests, both character-rename
checks, and `make install`. The install removed the root-level `circle` artifact.

A focused live Memcheck run then invoked `crafting`, `zlist`, `wildshape`, `whois Kohdee`,
`splitenchantment`, `featlisting`, `shoplist`, `inspirecourage`, and `finalstand` through the same
marker- and pager-aware harness used by the audit. All commands returned; a fresh-action retry
confirmed the full Final Stand effect path. At page length 255 and screen width 200, the large
listings paged without an overflow marker. Plain `shutdown` reached normal termination and MySQL
pool destruction. Memcheck reported zero errors, zero definitely, indirectly, or possibly lost
bytes, and four descriptors at exit (three standard descriptors plus its log).

## Build and runtime identity

| Item | Value |
|------|-------|
| Environment | Development (`APP_ENV=development`) |
| Source commit | `cf15380d9c8bb265d5105e481126ab4188782e3f` |
| Source dirty at build | No |
| Build | `make clean`, `make -j$(nproc)`, `make install` |
| Build result | Successful; no compiler warnings in the captured build output |
| Installed binary | `bin/releases/dae236ea90cf47b18798347d389dddd20f940589/circle` |
| SHA-256 | `56a2c1aada81cd51846a03196df69774d1f125751b4c7393e26087d94be5dbee` |
| Server port | 4100 |
| Staff character | Kohdee, level 34 |
| Primary room | 1204, Staff Simplex |
| Valgrind | 3.22.0 |

The root-level `circle` artifact was absent after `make install`. The runtime used the installed
`bin/circle` symlink, not a root build artifact.

## Memcheck configuration

All four processes used the same installed binary and the following material options:

```text
--tool=memcheck
--leak-check=full
--show-leak-kinds=all
--errors-for-leak-kinds=all
--track-origins=yes
--read-var-info=yes
--show-reachable=yes
--error-limit=no
--num-callers=50
--track-fds=all
--trace-children=yes
--keep-debuginfo=yes
--gen-suppressions=all
--xtree-leak=yes
--time-stamp=yes
--fair-sched=yes
--malloc-fill=0xAB
--free-fill=0xCD
```

Because `--errors-for-leak-kinds=all` was enabled, each `ERROR SUMMARY` includes leak contexts,
including still-reachable allocations. It must not be read as a count of invalid memory accesses.
Across all four runs, Memcheck reported exactly one invalid-access context: F01 below.

## Run chronology

| Run | PID | Purpose and termination | Invalid access | Definitely lost | Indirectly lost | Possibly lost | Still reachable | FDs at exit |
|-----|----:|-------------------------|---------------:|----------------:|----------------:|--------------:|----------------:|------------:|
| 1 | 101889 | Inventory entries 1-838; watchdog SIGABRT after `inspirecourage` | 1 | 40,566 B / 861 | 912,428 B / 5,276 | 400 B / 1 | 775,327,292 B / 823,894 | 12 (3 std) |
| 2 | 196196 | Entries 839-845; watchdog SIGABRT after `finalstand` | 0 | 0 | 0 | 400 B / 1 | 756,323,860 B / 726,112 | 13 (3 std) |
| 3 | 223838 | Entries 846-943, self targets, cleanup, 312-second observation; fast reboot exit | 0 | 6,022 B / 593 | 45 B / 3 | 0 | 761,496,185 B / 751,653 | 7 (3 std) |
| 4 | 284679 | Boot/login and plain `shutdown`; full normal cleanup baseline | 0 | 0 | 0 | 0 | 295,404 B / 10,788 | 4 (3 std) |

Runs 1 and 2 ended via the checkpoint watchdog, so their exit-time leak and descriptor totals
mostly describe forced-abort state. Run 3 used `shutdown now`, whose intentional fast-reboot path
calls `exit(52)` before `main` destroys the database and global registries. Run 4 is the
authoritative normal-cleanup comparison: it reached `Normal termination of game`, destroyed the
MySQL pool, reported no lost blocks, and retained only Valgrind's own log descriptor beyond standard
streams.

## Coverage

The authoritative inventory was captured from the live `wizhelp` and `commands` displays after
setting page length to 255 and screen width to 200. Entries and aliases were preserved as displayed,
including duplicate regular registrations.

| Surface | Live entries | Invoked | Deliberately skipped |
|---------|-------------:|--------:|---------------------:|
| Privileged, levels 31-34 | 192 | 156 | 36 |
| Regular, levels 0-30 | 790 | 787 | 3 |
| Total | 982 | 943 | 39 |

The three skipped regular entries were `prefedit`, `study`, and `write` because they enter an
interactive menu or editor.

The 36 skipped privileged entries were:

```text
aedit analyzeworld bedit cedit copyover craftedit hedit hlqedit hsedit iedit
medit msgedit oedit qedit redit saveall saveeverything saveobjstodb sedit
setroomdesc setroomflags setroomname setroomsect settime setweather setworldsect
shutdow shutdown tedit trigedit wizlock wizupdate zedit zlock zpurge zunlock
```

These entries open OLC/editors, perform broad analysis or persistence writes, alter global world
state, control process lifecycle, or lock/purge world content. The lifecycle command was submitted
only after coverage and the required idle observation were complete.

The 65-command self-target pass included the 14 staff checks from the earlier sweep
(`astat`, `goto`, `last`, `mute`, `notitle`, `pardon`, `players`, `restore`,
`snoop`, `stat`, `switch`, `thaw`, `transfer`, and `unaffect`) plus 51 regular combat,
inspection, support, social, and ability commands. Tame, guard, and dominate correctly rejected
self-targeting.

Final cleanup returned Kohdee to room 1204, standing at 632/632 hit points and 1160/1160 movement
with five attacks. Buildwalk, AFK, role-playing, PvP, mute, notitle, temporary combat modes, queued
actions, and removable spell effects were off. The group contained only Kohdee, inventory and
equipment were empty, and no pets remained. Page length and screen width were restored to 40 and
80.

## Findings

### F01 - `crafting` reads before the character skill array (high; resolved)

Invoking `crafting` without arguments produced the audit's only invalid-access context:

```text
Invalid read of size 4
  list_available_crafts                     src/craft/crafts.c:527
  impl_do_craft_with_kits                   src/craft/crafts.c:772
  do_craft_with_kits                        src/craft/crafts.c:761
  command_interpreter                       src/interpreter.c:6327
Address is 4 bytes before the 104,352-byte character allocation
```

`list_available_crafts` passes `CRAFT_SKILL(craft)` directly to `GET_SKILL`. Craft records
explicitly use `-1` for "No Skill" elsewhere in the same file, so a no-skill craft indexes one
integer before the character's skill storage. The same unguarded craft-skill value is also consumed
at `src/craft/crafts.c:705`, `:802`, and `:805`.

Resolution: craft identifiers now pass through `craft_skill_id_is_valid`; no-skill crafts bypass
the character skill array deliberately, invalid persisted or edited identifiers are rejected, and
a production-linked regression covers the legacy `-1` record.

### F02 - Group inspiration abilities can freeze the game loop (high; resolved)

`inspirecourage` and `finalstand` each stopped command processing in a separate fresh process.
Neither command returned to the game loop, reconnect attempts were not accepted, and the checkpoint
thread aborted each server after reporting:

```text
SYSERR: CHECKPOINT shutdown: tics not updated. (Infinite loop suspected)
```

Both handlers call `call_magic` with an affect registered as `MAG_GROUPS`
(`src/combat/act.offensive.c:15247-15258`, `:15451-15459`, and
`src/magic/spell_parser.c:5703-5710`). Code evidence points to the group iterator in
`src/magic/magic.c:10808-10838`: it uses the global, stateful `simple_list` iterator while
calling the full affect path for each member. `simple_list` explicitly forbids nested or
re-entrant use at `src/lists.c:444-480`. The affected test group contained Kohdee and loaded pets,
which provides the multi-member condition for the loop.

The watchdog signal stack identifies the checkpoint thread that called `abort`, not the blocked
main-thread frame, so the iterator diagnosis is a source-supported inference rather than a captured
main-thread backtrace.

Resolution: `mag_groups` now snapshots the group and traverses members with a local merge iterator.
The unreset global-iterator probe is gone, and a grouped player-plus-pet regression exercises both
Inspire Courage and Final Stand through nested affect calculations.

### F03 - Four command paths leak 6,067 bytes (medium; resolved)

The completed command process reported 6,022 definitely lost bytes and 45 indirectly lost bytes.
The boot/login normal-shutdown baseline reported zero lost bytes, and the XTree allocation paths
account for the complete 6,067-byte difference:

| Command path | Total lost | Blocks | Allocation evidence |
|--------------|-----------:|-------:|---------------------|
| `zlist` / `list_zones` | 5,676 B | 516 | Status strings duplicated at `src/olc/oasis_list.c:1841-1845` and never freed |
| `wildshape` listing | 292 B | 73 | Initial allocation at `src/act.other.c:3974`, per-race allocations at `:3512`, and disabled free at `:4188` |
| `whois Kohdee` | 6 B | 1 | `get_char_account_name` returns `strdup` storage at `src/account.c:941`; caller at `src/act.informative.c:9128` does not free it |
| Aborted queued actions | 93 B total (48 direct, 45 indirect) | 3 | Action nodes and duplicated arguments allocated at `src/interpreter.c:6317-6319` remain lost after queue cleanup |

Resolution: zone status text now uses static literals; wild-shape modifiers use stack storage;
`whois` stores and frees the duplicated account name; and action execution frees both the dequeued
node and its duplicated argument after dispatch. Queue clearing already owned both allocations and
continues to free them.

### F04 - Split Enchantment reports an expired negative cooldown (medium; resolved)

The no-argument pass printed:

```text
Split Enchantment is on cooldown.
Available in: -38189 seconds
```

`do_splitenchantment` gates with `HAS_FEAT(ch, PERK_WIZARD_SPLIT_ENCHANTMENT)` at
`src/act.other.c:5365`, then calls `can_use_split_enchantment_perk`, which correctly uses
`has_perk` at `src/character/perks.c:18354`. When those two namespaces disagree, the command
enters the cooldown branch even though the stored timestamp is already expired, and line 5374
subtracts the current time without clamping.

Resolution: the command and cooldown predicate now use `has_perk` consistently. A shared remaining-
cooldown helper clamps expired values to zero and oversized values to `INT_MAX`; regressions cover
the feat/perk collision, an active cooldown, and an expired persisted cooldown.

### F05 - Maximum page length still overflows descriptor output (low; resolved)

At the supported `pagelength 255` and `screenwidth 200` settings, `featlisting` appended
`**OVERFLOW**` near feat 120 and `shoplist` appended it near shop 106. Both commands now build
dynamic rows and pass them to `column_list`, but `page_string` can still fill the fixed
descriptor output buffer before `process_output` appends the marker at
`src/comm.c:2979-2988`.

Resolution: `next_page` now applies a conservative encoded-output budget beneath descriptor
capacity, accounts for already queued output, and reserves footer and prompt headroom. A regression
at page length 255 and screen width 200 verifies multiple safe pages and complete final output.

## Non-findings and interpretation notes

- Missing local Ollama and refused local I3 connections were declared expected and are excluded.
- The harness intentionally disconnected and reconnected around menus and timeout recovery; its
  corresponding EOF socket warnings are not server defects.
- The `hide` timeout was a marker-visibility interaction, not a stopped game loop.
- The fast-reboot process retained three MariaDB sockets because exit 52 bypasses `main` cleanup.
  Plain `shutdown` destroyed the pool and left no application descriptor open.
- Run 4's 295,404 still-reachable bytes are not lost memory. With
  `--errors-for-leak-kinds=all`, they contribute to the numerical error summary even though
  normal teardown reported zero lost blocks.

## Evidence

Local evidence is retained under:

```text
log/valgrind/full-command-audit-20260815/
  build.log
  install.log
  run1-command-sweep/
  run2-finalstand/
  run3-completion/
  run4-normal-shutdown/
```

The directory is intentionally ignored by Git because it contains approximately 30 MB of raw
runtime, transcript, coverage, and XTree data. The report is the durable, reviewable summary.

Key evidence files:

- `run1-command-sweep/inventories.raw.log`: live command inventories.
- `run1-command-sweep/full-sweep*.coverage.tsv`,
  `run2-finalstand/full-sweep-part3.coverage.tsv`, and
  `run3-completion/full-sweep-part4.coverage.tsv`: inventory coverage.
- `run3-completion/self-target.coverage.tsv`: 65-command target pass.
- `run1-command-sweep/runtime.log`: invalid read, first watchdog abort, and first Memcheck summary.
- `run2-finalstand/runtime.log` and `valgrind.196196.log`: second watchdog abort.
- `run3-completion/valgrind.223838.log` and `xtleak.kcg.223838`: command-run leak summary and
  allocation paths.
- `run4-normal-shutdown/runtime.log`, `valgrind.284679.log`, and
  `xtleak.kcg.284679`: normal-cleanup baseline.
- `resolution-20260816/commands.coverage.tsv`, `commands.raw.log`, and
  `finalstand.raw.log`: repaired command completion and maximum-pager evidence.
- `resolution-20260816/runtime.log`, `valgrind.427926.log`, and
  `xtleak.kcg.427926`: repaired normal-shutdown and zero-error Memcheck evidence.

## Closure

The repair followed the original recommended order. No remediation item from this audit remains
open; retain this report and its ignored raw evidence as the command-sweep and resolution record.
