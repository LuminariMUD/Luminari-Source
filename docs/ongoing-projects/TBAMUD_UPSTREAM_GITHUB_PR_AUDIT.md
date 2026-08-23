# TBA MUD Upstream GitHub Pull Request Audit

Status: Research complete; three local follow-ups identified

Audit date: 2026-08-23

Upstream repository: <https://github.com/tbamud/tbamud>

Upstream pull requests: <https://github.com/tbamud/tbamud/pulls>

Luminari reference revision: `930cf015`

## Executive summary

The upstream repository had 150 pull request records at the audit snapshot:

- 10 open
- 121 merged
- 19 closed without merge

Every record is represented in the coverage ledger below. The ten open pull
requests received individual source-level review. The 140 historical records
were classified by intent, then bug fixes and features that could still matter
were traced against their current Luminari equivalents.

Three upstream proposals expose useful work in the current tree:

1. [PR #191](https://github.com/tbamud/tbamud/pull/191) identifies two real
   out-of-bounds read paths in `src/utils.c`. These should be fixed locally and
   covered by the existing production-linked CuTest suite.
2. [PR #194](https://github.com/tbamud/tbamud/pull/194) highlights unsafe file
   replacement in OLC saves. Luminari still removes the live file before an
   unchecked rename, then can report success and clear the save-list entry.
   The portability branch should not be imported wholesale, but its atomic
   replacement direction is valid.
3. [PR #193](https://github.com/tbamud/tbamud/pull/193) is a valid optional DG
   Script editor improvement. It also exposes token-boundary and incomplete-
   block behavior in Luminari's current formatter that should be corrected even
   if syntax highlighting is deferred.

The remaining seven open pull requests need no direct adoption. Five are
already fixed in Luminari, one has no current local defect, and one proposes an
obsolete test architecture. No additional unported security or crash fix was
found in the 140 historical records.

## What "valid" means in this audit

A pull request was treated as valid for Luminari when its behavior is still
relevant, the current tree does not already provide an equal or stronger fix,
and the change fits Luminari's architecture and operating rules.

This is an intent and implementation audit, not a cherry-pick audit. The current
repositories have no usable common Git merge base, and Luminari has moved many
files into feature directories and substantially evolved the affected systems.
Each candidate was therefore traced from the upstream diff to current call
sites, tests, and build manifests.

The audit used the GitHub pull request list, pull request metadata, reviews,
checks, and diffs. The upstream default branch was also inspected at
`03d7ba7a48495b270e113821bc59b3cbde43c4b0`. Luminari's pre-existing working
tree changes were left untouched.

## Recommended local work queue

### Priority 1: harden the two string utilities from PR #191

#### `prune_crlf()` underflows on empty or newline-only input

`src/utils.c:2817` initializes an `int` with `strlen(txt) - 1`, then reads
`txt[i]` without checking that `i` is nonnegative. An empty string starts at
`txt[-1]`. A string containing only carriage returns or line feeds eventually
reaches the same state. A null pointer also reaches `strlen()` unchecked.

Current callers are in `src/db.c:531`, `src/db.c:625`, and `src/db.c:1292`.
Their normal input is expected to be allocated, but the utility's contract and
existing tests do not enforce nonempty content.

Required local fix:

- Return immediately for a null or empty string.
- Use a `size_t` length and inspect `txt[length - 1]` only while `length > 0`.
- Add cases for null, empty, CR-only, LF-only, and mixed newline-only strings to
  `unittests/CuTest/test_upstream_regressions.c`.

The current tests at `unittests/CuTest/test_upstream_regressions.c:244` cover
only strings with a non-newline prefix, so they cannot expose the underflow.

#### `count_non_protocol_chars()` can walk past the terminator

`src/utils.c:3653` advances once after an `@` or tab protocol marker. If that
marker is the final byte, the next branch advances again from the null byte.
The next loop condition then dereferences one byte beyond the string. The same
problem occurs for an unterminated `@[...]` sequence: the scan stops at the
terminator and then increments past it.

This helper is used for output measurement in `src/utils.c:5608` and for
builder-supplied room names in `src/olc/redit.c:959`.

Required local fix:

- Check for the terminator immediately after consuming a protocol marker.
- Advance past `]` only when a closing bracket was actually found.
- Define and test how a trailing marker or unterminated detailed code counts.
- Add cases for `@`, a trailing tab, `@[`, and `@[unterminated`.

The current tests at `unittests/CuTest/test_upstream_regressions.c:500` cover
ordinary text and a terminated detailed code only.

PR #191 also adds several libFuzzer targets. Luminari already has a focused
protocol parser fuzzer, while the root suite uses production-linked CuTest. The
additional fuzzing idea is useful, but its build integration should be ported to
the current test layout instead of copying the upstream makefiles.

### Priority 1: make OLC file replacement durable, informed by PR #194

Several save paths still follow this sequence:

1. Write a temporary file.
2. Close it without checking the result.
3. Remove the live file.
4. Rename the temporary file without checking the result.
5. Clear the save-list entry or return success.

Confirmed examples include:

- `src/dgscript/dg_olc.c:680` and `src/dgscript/dg_olc.c:743`
- `src/olc/genmob.c:468`
- `src/olc/genobj.c:443`
- `src/olc/genqst.c:315`
- `src/olc/genshp.c:461`
- `src/olc/genwld.c:681`
- `src/olc/genzon.c:373` and `src/olc/genzon.c:646`
- `src/comms/mail.c:258`

For example, `save_mobiles()` removes the current `.mob` file and ignores the
rename result at `src/olc/genmob.c:468-469`. It then removes the zone from the
save list and logs success at `src/olc/genmob.c:471-473`. If the rename fails,
the valid old file is already gone and the builder is told the save succeeded.

PR #194 replaces these pairs with a cross-platform wrapper. On POSIX, its core
improvement is calling `rename(temp, live)` directly, which atomically replaces
the destination on the same filesystem. However, most upstream callers still
discard the wrapper's return value. Luminari should implement the safer idea,
not reproduce that omission.

Required local design:

- Add one bounded file-replacement helper with explicit success/failure
  semantics and a Windows implementation if Windows remains supported.
- Do not remove the live destination before a same-filesystem POSIX rename.
- Check write, flush, close, and rename failures.
- Keep the save-list entry and report an error when any stage fails.
- Preserve the temporary file when useful for recovery and log both paths.
- Add failure-path tests, including a forced rename failure.

The rest of PR #194 is not suitable as a unit. It removes `autorun.sh`, which
conflicts with this repository's documented local run path; replaces large
parts of the already-modern GNU C23 build; and introduces an archive wrapper
that still constructs a shell command. Luminari's zone exporter already has a
stronger allowlist plus `fork()` and `execvp()` implementation.

### Priority 2: correct the DG formatter and consider PR #193's editor feature

Luminari has no current DG syntax-highlighting module, so the upstream feature
would be new rather than a fix for a local highlighter. Its validator and visual
feedback could still be useful to builders.

The current `format_script()` in `src/dgscript/dg_olc.c:960` has two confirmed
correctness gaps:

- Structural words use prefix comparisons at lines 998-1056. Text such as
  `ending`, `done_work`, `elsewhere`, `casefold`, or `breakfast` can be treated
  as a control token instead of an ordinary command.
- An unclosed block only emits a warning at lines 1105-1106. The function still
  replaces the editor buffer and returns success at lines 1108-1120.

Luminari already has bounds and nesting checks that are missing from older
upstream formatter code, so those protections must be preserved.

If implemented locally:

- Separate validation from formatting and highlighting.
- Require complete command tokens rather than prefixes.
- Do not mutate the editor buffer when structural validation fails.
- Add production-linked tests for nested `if`, `while`, and `switch` blocks,
  token-prefix false positives, unmatched closers, unclosed blocks, deep
  nesting, long lines, and output limits.
- Put any new source file in both `Makefile.am` and `CMakeLists.txt`.
- Update the DG/trigedit help in both the database and
  `lib/text/help/help.hlp` if builder-visible behavior changes.

## Open pull request findings

| PR | Upstream state at snapshot | Luminari disposition | Finding |
|----|----------------------------|----------------------|---------|
| [#195](https://github.com/tbamud/tbamud/pull/195) | Draft; mergeable; no checks | Already resolved locally | Luminari commit `b7a2c6da` repairs generic-list lifetimes with deferred destruction, iterator cleanup, a non-reentrant simple cursor with explicit reset behavior, `size_t` sizing, and production-linked tests in `unittests/CuTest/test_lists_production.c`. The local implementation is broader than the PR. |
| [#194](https://github.com/tbamud/tbamud/pull/194) | Ready; mergeable; build passes | Partially valid; port a bounded subset | The atomic replacement direction is valid and exposes a current OLC data-loss window. Do not import the broad portability/build branch wholesale. See the Priority 1 finding above. |
| [#193](https://github.com/tbamud/tbamud/pull/193) | Ready; mergeable; build passes | Valid feature candidate and partial correctness fix | Luminari lacks the highlighter but has an evolved formatter. Port the validator/highlighter design only after preserving local bounds checks and correcting token boundaries and failure semantics. |
| [#192](https://github.com/tbamud/tbamud/pull/192) | Ready; mergeable; build passes; approved | No current action | Luminari already has modern GNU C23 warning settings, safe editor number formatting, corrected MXP scoping/tag paths, and substantially reworked utilities. `util/plrtoascii.c` still uses an `fread()`/`feof()` pattern, but the return value is used later and the file compiles cleanly with the current warning flags; the upstream one-line removal is not a complete local error-handling fix. |
| [#191](https://github.com/tbamud/tbamud/pull/191) | Draft; mergeable; build and unit checks pass | Valid; port the utility fixes and tests | The `prune_crlf()` and `count_non_protocol_chars()` defects are present. Broader fuzzer coverage is useful but must fit the local CuTest and protocol-fuzzer layout. |
| [#189](https://github.com/tbamud/tbamud/pull/189) | Draft; mergeable; no checks | Already resolved locally | Player password loading uses `strlcpy(GET_PASSWD(ch), line, sizeof(ch->player.passwd))` at `src/players.c:1449`. Account credential loading is bounded as well. |
| [#187](https://github.com/tbamud/tbamud/pull/187) | Draft; mergeable; no checks | Already resolved locally | Copyover recovery uses `%511s %1023s %1023s` and validates the conversion count at `src/comm.c:594`. |
| [#184](https://github.com/tbamud/tbamud/pull/184) | Draft; mergeable; no checks | Already resolved by a different implementation | `var_subst()` bounds the initial copy with `strlcpy()` and terminates output through the destination pointer. The unbounded `strcpy()` path in the PR's base code is absent. PR #190 also supersedes part of this branch upstream. |
| [#182](https://github.com/tbamud/tbamud/pull/182) | Draft; mergeable; no checks | Already resolved more strongly | Luminari commit `a57845a5` validates names with an allowlist and creates the archive through `fork()`/`execvp()` without a shell. Regression tests cover the policy. |
| [#134](https://github.com/tbamud/tbamud/pull/134) | Draft; conflicting; no checks | Superseded and not applicable | This old unit-test branch targets a different tree and test framework. Luminari has a production-linked CuTest suite, a protocol parser harness, fuzzing support, and explicit source lists in both build systems. |

Only PR #192 had an approving review. PRs #193, #194, and #192 had successful
upstream build checks; #191 also had successful build and unit checks. The lack
of checks on the other drafts is another reason not to treat mergeability as
evidence that they should be imported.

## Historical pull request review

The 140 merged or closed records did not produce another current action item.
They fall into three groups. The number lists are exhaustive and mutually
exclusive; together with the ten open records above they account for all 150
pull requests.

### Current correctness lineage already contains the fix: 77 records

These bug fixes, warning cleanups, safer-string changes, or behavior corrections
are present in current code, have been replaced by a stronger local design, or
target code that has since been removed:

`#190`, `#180`, `#176`, `#175`, `#174`, `#173`, `#172`, `#171`, `#169`,
`#168`, `#167`, `#165`, `#164`, `#152`, `#149`, `#146`, `#145`, `#143`,
`#140`, `#138`, `#136`, `#133`, `#132`, `#131`, `#128`, `#125`, `#120`,
`#119`, `#118`, `#115`, `#114`, `#113`, `#112`, `#110`, `#103`, `#101`,
`#97`, `#94`, `#88`, `#87`, `#84`, `#82`, `#76`, `#75`, `#74`, `#55`,
`#54`, `#53`, `#52`, `#51`, `#50`, `#48`, `#44`, `#43`, `#41`, `#40`,
`#38`, `#37`, `#36`, `#34`, `#33`, `#32`, `#30`, `#29`, `#28`, `#26`,
`#25`, `#24`, `#23`, `#21`, `#15`, `#14`, `#13`, `#12`, `#7`, `#1`.

Representative source traces include:

- PR #190's pointer-size terminator bug is absent in current `var_subst()`;
  output is terminated through `*buf` in `src/dgscript/dg_variables.c`.
- PR #180's alias expansion is bounded at each output write in
  `src/interpreter.c:6250`, and an overlong expansion reports an error at
  `src/interpreter.c:6376`.
- PRs #168 and #164's traversal lifetime fixes are reflected in current
  next-pointer-safe memory and follower traversal.
- PR #165's returned quest item is extracted in `src/quest/quest.c`.
- PR #151's damage trigger exists through `MTRIG_DAMAGE` and
  `damage_mtrigger()`; PR #150's MTTS negotiation is in `src/net/protocol.c`.
- PR #113's `NOTHING` key guard is present and stronger in
  `src/movement/movement_doors.c:257`.
- PR #101's recent-player name is `MAX_NAME_LENGTH + 1` in
  `src/structs.h:7709`.
- PR #97's fragile pointer-decrement reader has been replaced by the bounded
  `normalize_fread_line()` based path in `src/db.c:6527`.
- PR #75's database teardown uses an explicit follower cleanup pass before
  freeing characters in `src/db.c:943`.
- PR #54's escaped `@@` editor-toggle case is handled explicitly in
  `src/olc/improved-edit.c`.
- PR #48's DG teleport validates and moves `vict`, then runs the entry trigger
  for `vict` in `src/dgscript/dg_mobcmd.c:807`.
- PR #12's OLC list generation uses bounded formatting and stops at the output
  limit in `src/olc/oasis_list.c`.

Some of these records are closed duplicates or intermediate attempts, such as
#112, #118, and #115. They remain in this group because the final intended
behavior was checked in the current source.

### Feature or tooling intent is already present or has a local equivalent: 21 records

These records added optional gameplay/editor behavior, test/build support, or
administrative tooling whose useful intent is already represented locally or
does not justify reopening the old patch:

`#178`, `#177`, `#160`, `#158`, `#154`, `#153`, `#151`, `#150`, `#142`,
`#127`, `#117`, `#99`, `#45`, `#42`, `#39`, `#35`, `#31`, `#22`, `#11`,
`#10`, `#6`.

Notable decisions:

- PR #177's Unity test layout is superseded by the root production-linked
  CuTest suite and focused protocol harness.
- PR #160's three-state upstream player-kill policy is intentionally not used.
  Luminari has a mutual-consent policy enforced at central combat boundaries.
- PRs #153 and #127's CMake direction is already covered by the maintained
  `CMakeLists.txt`; Autotools remains the preferred incremental build.
- PR #158 and PR #99 concern the obsolete Webster utility, which is absent.
- PR #142's verbose immortal `where` display and PR #178's richer script-door
  diagnostics are optional UX ideas, not unresolved correctness defects.
- Early player commands and DG editor features have either local equivalents or
  gameplay-specific replacements; old patches should not override current
  Pathfinder/Luminari behavior.

### Upstream-only, rejected, reverted, or obsolete: 42 records

These records affect upstream CI, upstream documentation or world data, retired
platform paths, abandoned branches, duplicates, or changes explicitly reverted
or closed without merge:

`#170`, `#166`, `#163`, `#162`, `#161`, `#156`, `#139`, `#137`, `#130`,
`#126`, `#123`, `#122`, `#121`, `#111`, `#102`, `#100`, `#73`, `#72`,
`#71`, `#70`, `#69`, `#68`, `#67`, `#66`, `#65`, `#64`, `#63`, `#62`,
`#61`, `#60`, `#58`, `#49`, `#46`, `#27`, `#20`, `#19`, `#18`, `#17`,
`#16`, `#9`, `#8`, `#5`, `#4`.

The world-data and spelling pull requests in #62-#73 apply to TBA MUD's stock
world, not Luminari's independently evolved content. PR #46 was explicitly
reverted by #49. The macOS/BSD branches (#58, #100, #137) predate the current
Autoconf and CMake setup. CI and repository-guidance changes (#163, #166, #170)
belong to the upstream repository and do not imply a Luminari runtime change.

## Suggested acceptance criteria for the three follow-ups

### String utilities

- AddressSanitizer and normal CuTest runs accept null/empty/newline-only prune
  cases and truncated protocol sequences without invalid reads.
- Existing text-length behavior remains unchanged for valid protocol strings.
- `make test` passes, followed by `make install` so no root `luminari` artifact
  remains.

### Durable saves

- A failed temporary-file write, close, or rename leaves the prior live file
  readable.
- Failure leaves the affected save-list entry pending and produces a `SYSERR`
  with both paths.
- Success replaces the file and clears the save-list entry exactly once.
- All relevant OLC writers use the shared helper; isolated raw remove/rename
  pairs have documented reasons if any remain.

### DG editor

- Full-token parsing distinguishes control words from ordinary commands with a
  shared prefix.
- Invalid or incomplete structure does not replace the editor buffer.
- Bounds, maximum nesting, and long-line failures retain current protections.
- Builder-visible behavior and help text are updated together.

## Recheck trigger

This is a point-in-time audit. Re-run the open-PR portion before implementing a
candidate if its upstream branch, review state, or checks have changed. New pull
requests numbered above #195 are outside this snapshot.
