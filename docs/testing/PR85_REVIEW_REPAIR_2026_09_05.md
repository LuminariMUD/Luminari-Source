# PR 85 review repair

Date: 2026-09-05
Original PR head: 677e0d9454181308d7a6188dd23ed47f401516b1
Rebase target: 668c9d4adb5601466407c65d335493368ea11fd6 (master)

## Scope and inventory

`gh api repos/LuminariMUD/Luminari-Source/pulls/85/comments --paginate`
and the GraphQL `reviewThreads` connection identified six unresolved CodeQL
comments, all in `load_events_v2()` in `src/players.c`. The reviews endpoint
contained one CodeQL review; the issue-comments endpoint contained only the
review-size notice. CodeRabbit skipped its review because the PR exceeded its
300-file limit; it supplied no code findings.

`gh pr checks 85` also identified failed sanitizer jobs. All other executable
checks passed on the original head, but the aggregate CodeQL gate failed on the
six parser findings. These failures were included in this repair.

The branch was 34 commits behind master. `git rebase --rebase-merges
origin/master` replayed its history. The two historical merge resolutions kept
the already-integrated runtime implementation and newer specification documents.
Comparing the rebased tree against the original head showed only master's
README and header-image additions. No implementation was lost in the rebase.

## Findings and resolutions

### CodeQL alerts 862 through 867: durable-event parser guards

The six comments concern event type, schema version, owner ID, remaining ticks,
save epoch, and payload value, respectively. The old combined condition relied
on the earlier header-version check to prove that one scanner must execute.
Each supported format now has an explicit branch whose scanner must return the
exact field count before any parsed value reaches the pending-record queue.
Unsupported headers still discard their section, and malformed rows still allow
later valid rows to load. Neither the persisted format nor gameplay changes.

The production-parser test seam copies queued records and frees the temporary
queue. New tests cover truncated records and invalid tokens at every conversion,
trailing garbage, valid records following invalid rows, signed payload values,
version-one recovery defaults, current-version recovery intervals, unsupported
headers, and preservation of the following player-file tag.

### Sanitizer failure: stale room registration in the Darkness fixture

The original sanitizer run reported 616 leaked bytes in 14 allocations from
readied-attack cooldown restoration. A local ASan/UBSan build reproduced the
same failure. GDB watched the actor's event-list pointer during shutdown and
identified `affected_room_forget()` overwriting it through a stale room pointer.
Tracing room registration located the stale owner in
`Test_warlock_darkness_lasts_fifteen_rounds()`.

That test used `free(darkness)` after `mag_room()` registered its stack room.
It now calls `rem_room_aff()` while the fixture world is still installed. New
assertions check that the effect chain, room registration, and effect count are
cleared, and that the registered-room count returns to its prior value. The
original spell-duration assertions remain. No production cleanup was bypassed
or sanitizer suppression added.

## Validation

- `make -j8 test`, followed by `make install`: passed, including 1,126 CuTests
  and the root architecture, lifecycle, help-sync, and tooling checks.
- CMake with `BUILD_TESTS=ON`, `-O1 -g -fsanitize=address,undefined
  -fno-omit-frame-pointer`, and sanitizer linker flags: production-linked
  `cutest` passed all 1,126 tests with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`
  and `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`. As in the CI sanitizer
  job, `LUMINARI_TEST_SKIP_SYNTAX_BOOT=1` excludes the separate boot subprocess.
- `make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=15`: passed.
- Pinned pre-commit formatting and hygiene hooks, plus `git diff --check`: passed.

The local development machine lacked libevent development headers. The Ubuntu
libevent development package was extracted under `/tmp/pr85-libevent` and its
pkg-config prefix pointed there, using the installed shared runtime library.
Local customized configuration headers and credential files were preserved.
