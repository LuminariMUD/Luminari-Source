# Development burn-in journal

## 2026-09-05 interrupted pass (reconstructed)

Evidence: `/tmp/luminari-burnin-20260905/`, especially `pass4/`.
The earlier pause stopped the staff client and monitor, logged Kohdee out,
and stopped the disposable database container. Development autorun stayed up.

Repairs retained: shopconv parser safety; DG explicit-wait loop accounting;
generated RoL route behavior; saved-object and ferry registry synchronization;
player-registry buff cleanup; bounded global reset removal; and development
player_save_objs.idx_name restoration. World preimages and application receipt
are in `path-world-repair-v2/`; database index receipts are in the evidence root.
No protected configuration/header or production code was changed.

Final-source validation from pass4 (source hashes rechecked below):

| Gate | Evidence | Result |
| --- | --- | --- |
| Pinned formatting | format-final.log | PASS |
| Clean Autotools server/utilities/tests | clean.log, build.log | PASS |
| libevent make test-all | test-all.log | 1135 CuTests; help integration; 509 world tests, 4 skips; 29 protocol tests; shell/schema checks |
| select driver and isolated syntax boot | select.log | 1135 PASS |
| CMake ASan/UBSan | cmake-build.log, ctest.log | 19/19 PASS |
| Production Valgrind | valgrind.log | 33 process reports, zero errors/lost blocks |
| Bounded protocol fuzz | fuzz.log | 30 seconds PASS |
| Protocol Valgrind | protocol-valgrind.log | 29 PASS, zero leaks/errors |
| Instrumented shopconv regression | shopconv.log | 9 PASS |

Earlier runtime passes found defects and are not substitutes for final live
acceptance. Pass4 copyover succeeded, but its extended calendar soak was interrupted.

## 2026-09-05 22:56 UTC resume

APP_ENV is development. Verified local MariaDB listener, game port 4101,
health listener 127.0.0.1:8182, and autorun ownership. Current server PID 2749570,
release f81ded2262768646ac479e33f735b6f350a3022f, Git
 db3e8e0ec5fbcfe2f39ff4aad9d01909d33de9a2 (dirty).
Readiness reports MUD and database healthy. The source manifest matches all
code, tests, and tooling; only burnin SKILL.md changed after that pass.
No build is currently running. Prior turn evidence records useful completed
work and an explicit pause, not a live test to wait on.

Resume evidence directory: `.burnin-runtime-resume-20260906/`.
Reusing audited final-source build evidence; completing the live calendar soak
and rechecking current compatibility and coverage gaps. Staff smoke uses
prompt boundaries, randomized inspection commands and no say markers.

Outstanding:

- Complete final staff/copyover/calendar soak and 60 seconds after clean logout.
- Review remaining runtime diagnostics and final process/release/readiness.
- Four RoL tests lack historical phase artifacts. Existing local/remote search
  receipts show them absent; no fabricated historical fixtures are accepted.
- Fourteen help-verifier failures have a staged, isolated-tested repair at
  `/tmp/luminari-burnin-20260905/help-review-final/review.md`. It changes keyword
  ownership; explicit review requested before application. No help patch applied.

No commit, merge, or publication is requested.

## 2026-09-05 22:59 UTC evidence audit

Re-read complete final logs and saved hashes/tails in
`recovered-validation-audit.json`. Both builds contain no compiler warning/error
lines. All 33 Valgrind error summaries are zero, with no definite, indirect, or
possible loss. All five pass4 status files say PASS. Root luminari is absent.

Current read-only SQL verification produced 193 result sets. All 14 failing
checks concern the pending help repair. Embedded migration versions and actual
pet engines, columns, and leading indexes satisfy the source contract;
player_save_objs.idx_name remains present. Evidence:
`runtime-schema-verification.json`. Reading the procedure definition through
the configured user is unavailable; passwordless sudo is also unavailable.
The maintained verifier ran successfully in a session forced READ ONLY.

The additional fixture search inspected 7,281 directories in /tmp and local
backups, finding no original historical fixture directories or likely archives.
See `fixture-extra-search.json`. A backup location has been requested.

Live monitor PID 2788672 (tool session 95899), staff client PID 2789057
(tool session 52774). Copyover recovered on the same descriptor and PID;
randomized commands have responded. Calendar/route observation is still live.
No new unexpected diagnostic at the most recent review. The disposable
MariaDB container remains stopped; no test fixture is needed for this live step.

## 2026-09-05 23:01 UTC calendar acceptance scope

Tracing utils.h confirms 75 seconds per game hour, hence 30 minutes per game
day. Zone 20203 includes midnight-sensitive work; the inherited 73-sample
client alone ends before the next midnight after this copyover. Keep runtime
observation through a complete post-copyover day (after 23:26 UTC), then collect
late staff diagnostics without another copyover. A continuous extended monitor
starts before stopping the initial monitor, retaining both evidence directories.
This prevents the earlier midnight reset finding from escaping the resumed soak.

## 2026-09-05 23:09 UTC user duration correction and logout

User specified that 8-10 minutes of soak is sufficient. This supersedes the
30-minute extension above. The resumed post-copyover observation had already
exceeded 12 minutes, so the inherited longer client was interrupted at its
wait, its socket closed, and only that known test session was reconnected for
final diagnostics and a complete character/account logout at 23:09:29 UTC.
No other player's session was taken over. The cleanup transcript and receipt
are under `cleanup/`. The interrupted client's exit 130 is intentional, not
a server crash or test assertion failure.

Final runtime diagnostics: 12 persistence cycles completed, 48 operations,
zero failures or budget overruns, maximum operation 5,599 microseconds.
No pulse exceeded the recorder's 100 ms threshold. Autoproc and point-update
registries validate with zero mismatch. Event diagnostics show zero failed
callbacks, admission rejections, stale owners, ready backlog or overdue work.
The scheduler did count 14,728 late callbacks and 1,050 skipped recurring
occurrences in 2,499,135 callbacks. Source reschedule_dispatched_event and
skip_late_event explicitly account for late recurring deadlines this way;
this is not a claim of zero lateness. No accompanying failed event, persistent
backlog, slow-pulse record or unexpected log diagnostic was observed.

The randomized inspection included score, inventory, equipment, time, weather,
who, activity, show stats, perfmon saves, perfmon entities, eventdebug,
eventdebug queue 10 and eventdebug domain. Copyover retained the same connection
and PID. Additional script-owner and moving-route inspection responded. Exact
commands and route counts are in `staff-smoke-final-result.json`; credential
values are excluded. Final post-logout monitoring and handoff checks follow.

## 2026-09-05 23:12 UTC handoff audit

The live smoke and user-bounded soak are complete. Continuous observation ran
from 22:54:58 UTC through 23:11:35 UTC, with overlapping monitors and more than
125 seconds after clean logout. There were 45 moving-route samples across 16
rooms. No fresh unexpected log diagnostic, crash file, restart, persistent
backlog or registry error was found. The final audit records periodic readiness
samples, exact interval endpoints, release hash and source/config invariants.
Known unavailable local Ollama and I3 connection messages remain excluded from
required integration health, as documented by the burnin skill.

Final MUD: PID 2749570, game port 4101, health 127.0.0.1:8182, normal immutable
release f81ded2262768646ac479e33f735b6f350a3022f. Autorun is still owned by the
persistent luminari-burnin-autorun user unit; required MariaDB readiness is
healthy. No game client remains connected. Temporary clients and monitors have
been stopped; the disposable database remains stopped with data preserved.
No root luminari artifact exists. Protected files and tested code still match.

Evidence: final-audit.json, final-autorun-status.log, final-health.json,
final-listeners.log, health-soak.jsonl, runtime*/log-review.json, and
cleanup/logout-result.json under `.burnin-runtime-resume-20260906/`.

Full qualification remains INCOMPLETE, for exactly these outstanding gates:

1. Four historical RoL fixture-dependent tests remain skipped. Original
   conversion artifacts must be restored; the test expectations were preserved.
2. Fourteen help-content verifier checks still fail. The corrected local patch
   is prepared and isolated-tested but unapplied pending explicit review of its
   keyword ownership changes. The review and exact patch remain at
   `/tmp/luminari-burnin-20260905/help-review-final/`.

The duration correction removes the full-game-day observation requirement;
it does not waive either of the coverage/help gates above. No commit, merge,
production write or publication was performed. The active goal is not marked
complete while those required gates are outstanding.

## 2026-09-05 23:14 UTC requested publication preparation

The active objective now explicitly requests commit/push to master after
completion, and leaving the development MUD running for user testing. This
supersedes the earlier note that publication was not requested.

Fetched origin/master and verified zero commits of divergence from local master
(db3e8e0ec). Reviewed the repair diff and regression coverage; prepared the
publication file inventory and draft commit message in the resume evidence
directory. Rechecked the staged help patch against current file hashes and
with git apply --check; its preimages remain unchanged. No commit/push has
occurred because the objective makes publication conditional on completion.
The missing-fixture and explicit help-review gates remain outstanding.

Previous goal turn classification: progress (completed the user-bounded live
soak, final diagnostics, logout, monitoring cleanup, and journal). This turn
adds publication preparation and upstream-state evidence. The development MUD
remains healthy; temporary test processes remain stopped.

## 2026-09-05 23:20 UTC help review completed and repaired

The earlier permission interpretation was too restrictive. The skill requires
explicit review of conflicts, not separate user approval for every confirmed
keyword cleanup. Existing burn-in authorization covers these narrow local
repairs. Completed the review against command registrations/handlers and the
current authored entries; the earlier pending-review notes are superseded.

The previous staged candidate had incorrectly retained BLAST as an ammunition
keyword. interpreter.c and do_blast prove it invokes eldritch blast. Corrected
both SQL and fallback ownership, strengthened the verifier, and retained older
ranged and special-procedure articles under LEGACY-RANGED-WEAPONS and
LEGACY-SPECIAL-PROCEDURES. Current article aliases remain available. No help
entry was deleted. Preserved current camp, casting, readied-action, and broader
combat prose; removed the unsupported initiative Dexterity prerequisite.

New evidence: `.burnin-runtime-help-20260906/`. A fresh snapshot qualified on
burnin_help_resume_test passed 28 verifier checks, idempotence, 11 affected
DB/file body-and-level comparisons, and preservation of all 2,162 fallback
entries. Qualification exposed and resolved the fallback's old duplicate
keyword owners too. Applied only after validating unchanged database and file
preimages, with an atomic database transaction and preserved file/SQL backups.
The MUD was stopped through autorun and process exit was verified before this
application. Fresh final-source validation follows; the 8-10 minute user soak
limit remains in force. Historical fixture coverage is still outstanding.

## 2026-09-05 23:36 UTC coverage gap repair

User clarified the default is to fix broken behavior and fill gaps. No further
routine permission gate remains. A broader read-only archive search found no
original historical phase directories. Replaced the four tests' historical
folder dependencies with fresh, isolated current-policy conversion inputs from
the installed RoL corpus. The helper copies the tracked minimal target and
converts one genuine Hulburg room into it to exercise canonical KEEP coverage;
it neither impersonates a historical run nor modifies installed world files.
Both build manifests include the helper.

The first focused run passed all special-binding and full reconciliation ledger
assertions. It exposed two obsolete counts: current policy emits 69,922 records
rather than 69,920; a minimal target requires all 333 pilot quests rather than
57 remaining additions from an old partially imported target. Traced the new
counts to exact source records. Added per-kind emission counts, the four exact
source-defect exclusions, one KEEP, and per-package quest totals (1/276/31/12/13).
All existing parser/content and special-reconciliation assertions remain.
Corrected an audit-summary filename typo exposed by this first run as well.

Rechecked the actual development database read-only: all 194 schema/help result
sets pass, no embedded migration is missing, and all 11 affected help entries
match fallback bodies/levels with unique expected aliases. Evidence is under
`.burnin-runtime-final-20260906/`. Fresh final builds and full validation are
running with the isolated database. Development autorun remains stopped until
the tested normal release is ready for the final 8-10 minute live pass.

## 2026-09-05 23:44 UTC final validation passed

Final evidence root: `.burnin-runtime-final-20260906/`. The final source manifest
and protected-input hash comparison are recorded there; protected headers and
credential files are unchanged. Pinned formatting and all applicable staged
hygiene hooks pass. Both fresh builds have zero compiler warnings/errors.

| Gate | Command/evidence | Result |
| --- | --- | --- |
| Clean Autotools | normal-pass.sh: make clean; make -j all cutest | PASS |
| Production/integration | libevent make test-all, isolated MariaDB/help enabled | 1135 CuTests, 36 help tests, 509 world tests, 29 protocol tests; no suite skips |
| Alternate driver | LUMINARI_IO_DRIVER=select ./cutest | 1135 PASS, isolated syntax boot enabled |
| CMake ASan/UBSan | fresh cmake-asan; ctest --output-on-failure | 19/19 PASS |
| Production Valgrind | full definite-leak/origin check, error exit enabled | 33 reports, zero errors or lost bytes |
| Protocol Valgrind | make -C unittests/CuTest valgrind-protocol | 29 PASS; zero leaks/errors |
| Protocol fuzz | make -C unittests/CuTest protocol-fuzz FUZZ_SECONDS=30 | PASS |
| Instrumented shopconv | ASan/UBSan tests.test_shops | 9 PASS |
| Runtime schema/help | verify-runtime.py, read-only development DB | 194 result sets pass; 11 affected entries aligned |

The four historical-folder skips are resolved and supersede the earlier
incomplete notes. All special-reconciliation ledger assertions passed unchanged.
The normal test log's expected negative-fixture diagnostics match the reviewed
prior pass; varying Zzcd process-specific names trace to test_gameplay_e2e.c.
Instrumentation uses the CI-matching syntax-boot skip only; normal runs prove
that boot against disposable data.

The normal installed immutable release is
`bin/releases/f81ded2262768646ac479e33f735b6f350a3022f/luminari`, SHA-256
`704d1e94893aa11809bc173f8bb23407521d8140acc7170df00e60ce76f2f57d`.
It is byte-identical to the earlier repaired server because final changes affect
help/tooling/tests; build identity records base db3e8e0ec with dirty source. The
source manifest ties that tested executable and the final tooling to publication.
There is no root-level luminari artifact. Final live acceptance is in progress.

## 2026-09-05 23:46 UTC live copyover recovered

Autorun started successfully, staff login and corrected help inspections passed,
and a real copyover preserved the descriptor and PID 2917796. The HTTP health
endpoint temporarily closes during the deliberate world reload; its initial
strict sampler recorded one connection-refused observation at 23:45:09 UTC and
exited. Kept that evidence, then resumed append-only health sampling after the
23:45:41 recovery. This planned reload interruption is not reported as an
uninterrupted-readiness pass. Runtime log monitoring covered startup and reload
continuously; no unexpected diagnostics were found. The post-copyover route soak
and final clean logout remain in progress.

## 2026-09-05 23:57 UTC completed qualification and handoff

Final audit passed. Observation covered 23:44:00-23:55:57 UTC on September 5,
including startup, staff help/inspection commands, same-descriptor/PID copyover,
514.7 seconds (8 minutes 35 seconds) of route observations across 31 samples and
16 rooms, final read-only telemetry, full character/account logout, and 70.3
seconds after the last logout at 23:54:47 UTC. The route interval honors the
user's 8-10 minute limit. There were no new crash dumps or unplanned restarts.

Kohdee inspections covered score, inventory, equipment, time, weather, who,
activity, show stats, perfmon saves/entities, eventdebug (queue/domain/types and
mob script views), and help BLAST/AMMO/ACTIVITY/CAMP/INITIATIVE/READY/SPECIALS.
The final short session captured perfmon summ, slow 10, sql, top max 15,
saves/entities, and eventdebug. Both sessions completed character and account
logout, without in-game say markers.

Persistence completed 8 cycles and 32 operations with zero failures, budget
or hard overruns; maximum operation 5,005 usec. Final scheduling had zero failed
callbacks, registry mismatches, stale owners, ready/overdue backlog, or admission
rejections. Lifecycle counters reported 12,114 late and 940 skipped recurrences
among 1,722,127 callbacks, with zero missed callbacks; this is not a claim of
zero scheduler lateness.

One bounded latency observation remains explicitly recorded: the non-rate-limited
flight recorder and counters show exactly one pulse above 100 ms among 9,654
logged outer loops (0.01%), maximum 151.808 ms. Its largest callback was
DG trigger wait at 133.183 ms, with 17.339 ms of pending character extraction,
zero SQL in that pulse, and no catch-up backlog. No recurring stall or correctness
failure reproduced. No speculative gameplay change was made from this isolated
timing sample. SQL telemetry showed zero errors/reconnects. This qualifies the
stability pass with the recorded latency observation, not a zero-warning claim.

Health monitoring recorded 63 healthy samples across the overlapping monitors
and the single preserved, planned copyover interruption described above. The
last observation window had zero unexpected diagnostics. Current readiness is
healthy for both MUD and MariaDB. Autorun remains active in the persistent user
unit luminari-burnin-autorun.service, executing autorun.sh, on development port
4101; server PID 2917796 and the tested immutable release are unchanged.
The disposable MariaDB container and all burn-in monitor/client processes are
stopped. Protected files and final code/test/help/tooling hashes still match the
validated inputs; all 194 local schema/help checks passed again after disposable
DB shutdown. No production code/data was modified.

Evidence: final-audit.json, validation-audit.json, runtime-schema-verification.json,
source.json, full command logs, staff transcripts/results, and runtime/tail log
reviews under `.burnin-runtime-final-20260906/`. Help and world preimages remain
in the previously named evidence directories. The published-source executable
was built before the commit and therefore reports base db3e8e0ec with dirty=1;
its exact byte hash and source manifest above identify what was tested and left
running. Publication target is master; the final commit identity is available
through git history for this journal and the local publication receipt.
