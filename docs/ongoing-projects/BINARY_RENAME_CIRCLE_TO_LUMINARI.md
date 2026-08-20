# Binary Rename: `circle` -> `luminari`

> Planning document. This describes the required end state and rollout. The
> rename is not implemented yet.

Audit refreshed: 2026-08-20, against `master` at `a64f8afc`.

Rename the built server executable from `circle` to `luminari`. The old name is
inherited from CircleMUD and no longer identifies this program. This is a
basename and release-layout migration, not a rename of CircleMUD history,
gameplay terminology, or legacy C APIs.

## 1. Decision and completion criteria

The canonical executable and debug-symbol basenames will be `luminari` and
`luminari.debug`.

The rollout has two releases:

- **Phase A:** build, install, launch, and copy over through `luminari`, while
  retaining `bin/circle -> luminari` as a compatibility symlink.
- **Phase B:** retire that compatibility symlink only after both the live MUD
  and its long-running supervisor have demonstrably migrated.

Phase A is complete only when all of the following are true:

- Autotools and CMake emit a `luminari` target and no root-level `circle`
  target.
- New immutable releases contain `luminari`, `luminari.debug`, and `manifest`.
- `bin/luminari` is the canonical mutable alias.
- `bin/circle` is a symlink to `luminari`, not a second executable or a copy.
- A pre-rename binary can copy over to the Phase A binary through
  `bin/circle`.
- A Phase A binary can then copy over through its compiled-in
  `bin/luminari` path.
- Autorun, deployment, diagnostics, tests, tooling, current documentation, and
  the database and file copies of the GDB help entry use the canonical name.
- Existing immutable `circle` releases remain usable for rollback and core
  analysis.

Phase B is complete only when the compatibility link is no longer created or
present and every gate in section 7 has been proved on production. There is no
deadline for Phase B; retaining one symlink is safer than retiring it early.

## 2. Scope boundaries

### 2.1 Rename these forms

Rename references that identify the executable, build target, release
artifact, debug sidecar, process, or root build artifact, including:

- `circle`, `./circle`, `bin/circle`, and `circle.exe` when they are executable
  paths or build outputs;
- `circle.debug` when it is the debug sidecar for a new release;
- the `circle` Autotools and CMake target;
- `circle_SOURCES`, `circle_LDADD`, and other Automake variables whose prefix
  is derived from that target;
- CMake scratch variables such as `CIRCLE_COMPILE_DEFINITIONS` whose names
  specifically describe properties copied from that target;
- the `argv[0]` value and failure log in the copyover `execl()` call; and
- process-name probes that assume the executable is named `circle`.

### 2.2 Do not rename these forms

- **CircleMUD attribution and licenses.** Preserve `CircleMUD`, `circlemud`,
  CircleMUD URLs, credits, copyright, and heritage descriptions verbatim.
- **Gameplay meaning.** Preserve spell circles, the `circle` combat command,
  room/object descriptions, full-circle kicks, and similar English uses.
- **Legacy C APIs and state.** Preserve `circle_counter`, `circle_srandom`,
  `circle_random`, `circle_shutdown`, `circle_restrict`, `circle_follow`,
  `circle_for_spell`, and other internal `circle_*` identifiers. They are not
  executable artifacts.
- **Legacy configure/platform names.** Preserve `CIRCLE_MYSQL`, `CIRCLE_UNIX`,
  `CIRCLE_CRYPT`, `CIRCLE_AMIGA`, `CIRCLE_MACINTOSH`, and related feature
  macros. They are independent of the target basename.
- **Old immutable releases.** Do not rename existing
  `bin/releases/<build-id>/circle` or `circle.debug` files. Their names are part
  of captured release and crash evidence.
- **Historical records.** Do not rewrite old paths in `docs/CHANGELOG.md`,
  `docs/previous_changelogs/`, completed validation reports, or dated evidence
  paragraphs. Add a new changelog entry and update prescriptive/current text
  instead.
- **Unrelated names.** Keep the repository directory, `lib/` layout, and
  `luminari.service` unit name.
- **Ignored reference snapshots and generated files.** Do not edit the ignored
  `/EXAMPLE/` trees, including the separate Chronicles of Krynn checkout, or
  generated CMake/Autotools output. Regenerate local build files after the
  tracked inputs change. The implementation inventory in this plan covers the
  authoritative Git-tracked Luminari tree.

The target-derived Automake and CMake variables above are deliberate
exceptions to the general `circle_*` exclusion. A blanket replacement of the
bare word `circle` is prohibited.

## 3. Current mechanics and actual failure modes

### 3.1 Copyover

`src/db.h` currently defines `EXE_FILE` three times:

```c
#if defined(CIRCLE_AMIGA)
#define EXE_FILE "/bin/circle"
#elif defined(CIRCLE_MACINTOSH)
#define EXE_FILE "::bin:circle"
#else
#define EXE_FILE "bin/circle"
#endif
```

Only the final branch is functional on supported deployments, but all three
values should be internally consistent.

`perform_do_copyover()` in `src/act.wizard.c` first calls
`realpath("../" EXE_FILE, ...)`. If an old process cannot resolve
`bin/circle`, it logs the error and returns before descriptor handoff or
service teardown; the game process remains running. Later validation also
checks that the resolved file is executable. If `execl()` itself returns, the
code restores the checkpoint timer and attempts to continue, although MySQL
connections and auxiliary bridges have already been shut down by that point,
so that late failure is a degraded state and still requires operator action.

The compatibility link remains load-bearing despite those guards:

- The old process can only discover the newly installed image through its
  compiled-in `bin/circle` path.
- The old autorun supervisor keeps `MUD_BINARY=circle` in memory until that
  shell process is restarted, even after the MUD performs a same-PID
  copyover.
- Planned restarts or crash recovery by that old supervisor also require
  `bin/circle`.

The same copyover call currently passes literal `"circle"` as `argv[0]` and
uses it in the failure log. Those two references must become `"luminari"`.

### 3.2 Release installation

`scripts/deployment/install_versioned_binary.sh` currently creates:

```text
bin/releases/<ELF-build-id>/circle
bin/releases/<ELF-build-id>/circle.debug
bin/releases/<ELF-build-id>/manifest
bin/circle -> releases/<ELF-build-id>/circle
```

It validates build IDs and SHA-256 values, publishes the release directory
before switching the alias, and refuses to replace a regular `bin/circle` if
`.mud.pid` identifies that exact inode as the live executable. A stopped
regular legacy binary is archived by build ID before the alias replaces it.

Phase A must preserve those safety properties while allowing old and new
release layouts to coexist. New releases use the new basename; old release
directories are never rewritten.

### 3.3 Build and supervisor behavior

- `Makefile.am` uses `bin_PROGRAMS = circle`, the target-derived
  `circle_SOURCES` and `circle_LDADD` variables, and staging directory
  `bin/.circle-install`.
- `CMakeLists.txt` has 23 standalone `circle` occurrences in the target and
  install path, including
  `add_executable(circle ...)`, target property calls, test-property copies,
  `$<TARGET_FILE:circle>`, and an old-name error message.
- `scripts/autorun/autorun.sh` defaults `MUD_BINARY` to `circle` and validates
  only a `circle`/`circle.debug` release pair.
- `luminari.service` launches `autorun.sh`, not the MUD directly. Its unit name
  and `ExecStart` remain correct, but Phase A should explicitly set
  `MUD_BINARY=luminari` so the installed service records the choice and stale
  external defaults cannot silently win.
- `.mud.identity` and `.autorun.state` currently retain the executable identity
  captured when autorun forked the MUD. A same-PID `exec` can make that metadata
  stale. Rollout verification must use `/proc/<mud-pid>/exe` and the executable's
  ELF build ID as authority unless Phase A also refreshes the metadata from
  `/proc`.

## 4. Phase A implementation

### 4.1 Build target and C source

1. In `Makefile.am`:
   - change `bin_PROGRAMS` to `luminari`;
   - rename `circle_SOURCES` and `circle_LDADD` plus every use of them;
   - change staging to `bin/.luminari-install`;
   - pass and remove `$(bindir)/luminari` in `install-exec-hook`; and
   - update comments and root-artifact checks.
2. In `CMakeLists.txt`:
   - rename the executable target and every target argument;
   - rename target-derived scratch variables such as
     `CIRCLE_COMPILE_DEFINITIONS`, `CIRCLE_COMPILE_OPTIONS`, and
     `CIRCLE_LINK_LIBRARIES` to `LUMINARI_*`;
   - update `$<TARGET_FILE:circle>` and the install failure text; and
   - leave legacy `CIRCLE_*` feature and platform variables unchanged.
3. In `src/db.h`, change all three `EXE_FILE` values to `luminari`.
4. In `src/act.wizard.c`, change copyover's `argv[0]` and matching failure log
   to `luminari`.
5. In `unittests/CuTest/test_syntax_check_boot.c`, change the synthetic
   `argv[0]` to `luminari`.
6. Update the diagnostic command in `src/sysdep.h`; use both
   `/tmp/luminari-trace` and `bin/luminari`.
7. Replace the broad `.gitignore` entry `circle` with the root-anchored
   `/luminari`. Do not hide a stale root-level `circle` after the migration.

No source-file membership changes are required. If implementation adds a C
source file for any reason, update both `Makefile.am` and `CMakeLists.txt`.

### 4.2 Installer and mixed release layouts

For a new build ID, install exactly:

```text
bin/releases/<ELF-build-id>/luminari
bin/releases/<ELF-build-id>/luminari.debug
bin/releases/<ELF-build-id>/manifest
bin/luminari -> releases/<ELF-build-id>/luminari
bin/circle -> luminari
```

Use the stable symlink chain `circle -> luminari`; do not point both aliases
independently at a release. Future installs then switch only `bin/luminari`,
and both names change together atomically.

Required installer behavior:

- Create and fully validate the immutable release before changing either
  alias.
- Atomically publish `bin/luminari` with a temporary link in `bin/` and
  `mv -Tf`.
- On the first Phase A install, publish `bin/luminari` before atomically
  replacing the old `bin/circle` link with `circle -> luminari`. At every
  instant, an old copyover resolves either the previous valid release or the
  new valid release.
- Validate an existing new-format release using `luminari` and
  `luminari.debug`.
- Preserve old-format release directories and teach shared identity helpers to
  derive the debug sidecar from the resolved executable basename. This permits
  both `circle`/`circle.debug` and `luminari`/`luminari.debug` rollbacks.
- Keep the regular-file migration and live-inode refusal for legacy
  `bin/circle`. Archive a stopped old binary without rewriting an existing
  old-format release.
- Apply equivalent no-clobber handling to a pre-existing regular
  `bin/luminari`: refuse if it is live; otherwise archive it or fail with an
  explicit recovery instruction before any replacement.
- Remove broken alias symlinks safely. Reject directories and other special
  files at either alias path.
- Treat an incomplete release directory or build-ID/SHA collision as a hard
  failure and leave both aliases unchanged.
- Never delete or bulk-rename an immutable release as part of installation.

Extend `scripts/deployment/test_versioned_binary_install.sh` to prove every
state above, including first migration from the old layout, repeated installs,
the symlink chain, live and stopped regular files, broken links, failure
atomicity, mixed-format release validation, and retention of the running old
release.

### 4.3 Autorun, systemd, deploy, and diagnostics

1. In `scripts/autorun/autorun.sh`:
   - default `MUD_BINARY` to `luminari`;
   - validate the debug sidecar by resolved basename rather than hardcoding
     `circle.debug`;
   - recognize any executable below `bin/releases/<build-id>/`, not only a
     path ending in `/circle`;
   - publish the supervisor's configured binary basename in `.autorun.state`;
     and
   - make `.mud.identity`, `.autorun.state`, and crash selection copyover-aware
     by refreshing the active executable and build identity from
     `/proc/<pid>/exe`. Add a same-PID `exec` regression; otherwise crash
     analysis can select the pre-copyover executable.
2. Add `Environment="MUD_BINARY=luminari"` to `luminari.service`. Keep the
   unit name, `PIDFile`, and autorun-based `ExecStart`/`ExecStop` unchanged.
3. Update `scripts/deployment/deploy.sh` build checks, active-release
   verification, messages, and direct-start advice to use `bin/luminari`.
4. Update these maintained wrappers:
   - `scripts/cbuild.sh`;
   - `scripts/debugging/debug_game.sh`;
   - `scripts/debugging/vgrind.sh`;
   - `scripts/deployment/move_bin.sh`;
   - `scripts/deployment/setup.sh`;
   - `scripts/permissions/check_permissions.sh`; and
   - `scripts/permissions/fix_permissions.sh`.
5. Replace unsafe process-name discovery rather than merely changing the name:
   - `scripts/process-memory/monitor_process_memory.sh` should prefer the
     project `.mud.pid`, verify `/proc/<pid>/exe` belongs to this checkout's
     release tree, and reject ambiguity;
   - `scripts/copyover/enhanced_copyover_diagnostic.sh` should use the same
     PID/executable verification; and
   - `scripts/copyover/copyover_watchdog.sh` should stop reading the nonexistent
     `lib/misc/circle.pid` and use the authoritative project `.mud.pid`.
6. Remove process-name assertions from
   `scripts/development/dev_kohdee_login_smoke.sh` and
   `scripts/vessels/run_vessel_ferry_soak.sh`; verify the listener PID and
   `/proc/<pid>/exe` instead.

The PID changes close a pre-existing shared-host hazard: a bare `pgrep circle`
can select another MUD on the same machine. Renaming the probe to
`pgrep luminari` would reduce collisions today but would preserve the faulty
ownership model.

### 4.4 CI, test harnesses, and operational tooling

Update the executable path, expected process name, root-artifact assertion,
and provenance hash as applicable in:

- `.github/workflows/integration.yml`;
- `.github/workflows/test.yml`;
- `scripts/autorun/test_autorun_supervision.sh`;
- `scripts/process-memory/test_monitor_process_memory.sh`;
- `scripts/test_pubsub.sh`;
- `scripts/development/dev_kohdee_login_smoke.sh`;
- `scripts/vessels/provision_vessel_campaign.sh`;
- `scripts/vessels/provision_vessel_derelict.sh`;
- `scripts/vessels/provision_vessel_frontier.sh`;
- `scripts/vessels/provision_vessel_harbor.sh`;
- `scripts/vessels/run_vessel_ferry_soak.sh`;
- `scripts/vessels/run_vessel_scale_benchmark.sh`;
- `scripts/vessels/test_vessel_derelict_in_game.sh`;
- `scripts/vessels/test_vessel_events_in_game.sh`;
- `scripts/vessels/test_vessel_hunter_in_game.sh`;
- `scripts/vessels/test_vessel_merchant_in_game.sh`;
- `scripts/vessels/test_vessel_scale_benchmark_parsers.sh`;
- `scripts/vessels/test_vessel_tactical_in_game.sh`;
- `scripts/world/wtool_lib/rol_phase8.py`;
- `lib/world/validate-zone.ps1`; and
- `util/powershell/ProfileTemplate.ps1`.

For `rol_phase8.py`, update the executable path and require both the canonical
root `luminari` artifact and any stale root `circle` artifact to be absent.
Prefer a generic `root_build_artifacts_absent` result while retaining legacy
`root_circle_absent` compatibility if existing manifests consume it; do not
silently change a persisted manifest schema without checking its readers.

Add a focused static regression check for high-signal obsolete forms in active
files. Register it in the Autotools and CMake test paths. Its allowlist must be
path-specific and limited to the Phase A compatibility implementation, its
tests, this plan, and intentionally historical evidence. It must not flag
CircleMUD attribution, gameplay spell circles, or internal C identifiers.

### 4.5 Help, templates, and current documentation

Update the `GDB` entry in `lib/text/help/help.hlp` from `gdb bin/circle` to
`gdb bin/luminari`.

Mirror that change in the database by adding an idempotent SQL component for
the `help_entries` row whose tag is `GDB`, and register the component as
`apply` in `sql/components/ci_schema_manifest.txt`. Verify before and after
deployment that exactly one `GDB` row exists and that neither the row nor the
file copy contains `bin/circle`. Do not edit `lib/.env` or
`lib/mysql_config`.

Update these maintained instruction/template files:

- `AGENTS.md`, `CONTRIBUTING.md`, `README.md`, and
  `unittests/README_unittests.md`;
- `.agents/skills/mergetree/references/merge-worktree.md`;
- `util/aider/aider_config_template.md` and
  `util/aider/aider_generic_config_template.md`;
- `util/claude_code/CLAUDE.example.md`;
- `docs/CONVENTIONS.md`, `docs/GETTING_STARTED.md`, `docs/deployment.md`,
  `docs/development.md`, `docs/environments.md`, `docs/known-issues.md`, and
  `docs/onboarding.md`;
- `docs/admin/FAQ.md` and
  `docs/deployment/DATABASE_DEPLOYMENT_GUIDE.md`;
- `docs/deployment/DEPLOYMENT_GUIDE.md`;
- `docs/development/CMAKE_BUILD_GUIDE.md` and
  `docs/development/PERFORMANCE_OPTIMIZATIONS.md`;
- `docs/guides/SETUP_AND_BUILD_GUIDE.md`,
  `docs/guides/TESTING_GUIDE.md`, and
  `docs/guides/TROUBLESHOOTING_AND_MAINTENANCE.md`;
- `docs/ongoing-projects/ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md`;
- `docs/runbooks/LOCAL_INTERMUD3_E2E.md` and
  `docs/runbooks/incident-response.md`;
- `docs/systems/ARTIFACT_SYSTEM.md`, `docs/systems/SKORE_SYSTEM.md`,
  `docs/systems/VESSEL_SYSTEM.md`, and
  `docs/systems/narrative-weaver/DEPLOYMENT_INSTRUCTIONS.md`;
- the prescriptive/current portions of
  `docs/testing/LOCAL_DEV_LOGIN_QUICK_GUIDE.md`;
- `docs/testing/WILDERNESS_CRAFTING_INTEGRATION_TESTING.md`; and
- `docs/world_game-data/BUILDER_QUICKSTART.md`.

Incident-response text must remain able to analyze old cores: use the exact
executable recorded for the crashed process and the same-basename `.debug`
sidecar, rather than claiming every historical release contains
`luminari.debug`.

Add a current changelog entry for the rename, but preserve old `circle` paths
inside existing changelog entries. When Phase A lands, mark this plan and
`docs/ongoing-projects/README_ongoing-projects.md` as implemented with the
compatibility link still active. Mark Phase B separately when it actually
lands.

Do not rewrite these known historical occurrences:

- `docs/CHANGELOG.md` entries predating the rename and all of
  `docs/previous_changelogs/`;
- the obsolete historical reports
  `docs/deployment/DEPLOYMENT_FIX.md` and
  `docs/deployment/DEPLOYMENT_STATUS.md`;
- `docs/ongoing-projects/CAMPAIGN_VARIANT_RETIREMENT_LIVE_TEST_REPORT.md` and
  the dated executable path in `docs/ongoing-projects/todo.md`;
- completed `docs/testing/SPECIAL_PROCEDURE_PHASE_*` validation/progress
  evidence;
- dated results in `docs/testing/VESSEL_BENCHMARKS.md` and
  `docs/testing/VESSEL_SYSTEM_TESTING.md`;
- dated evidence paragraphs in
  `docs/testing/LOCAL_DEV_LOGIN_QUICK_GUIDE.md`; and
- the completed 2026-08-05 evidence in
  `docs/utilities/WORLD_VALIDATOR_CLI.md`.

If a historical file also contains a maintained procedure, update only the
procedure and leave its recorded command/result intact. All edited
documentation and help/SQL text must remain ASCII, UTF-8, and LF.

## 5. Phase A verification

### 5.1 Static and focused checks

1. Run `git diff --check` and the new binary-name static regression.
2. Run `bash -n` on every changed shell script.
3. Run the focused installer, autorun, process-memory, vessel parser, Python
   world-tool, and PowerShell checks that cover changed files.
4. Review every remaining high-signal `circle` result and classify it as one
   of:
   - Phase A compatibility code/test;
   - old immutable release documentation;
   - historical evidence;
   - CircleMUD attribution;
   - gameplay/domain terminology; or
   - an internal legacy identifier.

Useful searches include:

```bash
git grep -n -I -E 'bin/circle|\./circle|circle\.debug|circle\.exe'
git grep -n -I -E 'add_executable\(circle|TARGET_FILE:circle|bin_PROGRAMS *= *circle'
git grep -n -I -E \
  'circle_(SOURCES|LDADD)|CIRCLE_(COMPILE_DEFINITIONS|COMPILE_OPTIONS|LINK_LIBRARIES)'
git grep -n -I -E 'pgrep[^[:cntrl:]]*circle|grep[^[:cntrl:]]*circle' -- scripts
```

The goal is a reviewed, explicit allowlist, not zero occurrences of the word
`circle`.

### 5.2 Autotools and installed layout

Use the repository's configured Autotools path. If generated configuration is
stale after the target rename, run `autoreconf -fvi && ./configure` first.

```bash
make clean
make -j"$(nproc)"
LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480 make test-all
```

`make test-all` ends with `make install`; if any later command recreates the
root target, run `make install` again. Do not leave a root executable behind.

Verify:

```text
root luminari                         absent after install
root circle                           absent
bin/luminari                          executable symlink
bin/circle                            symlink whose text target is luminari
bin/releases/<id>/luminari            executable regular file
bin/releases/<id>/luminari.debug      regular file with matching ELF build ID
bin/releases/<id>/manifest            matching build ID and SHA-256
readlink -f bin/luminari              same as readlink -f bin/circle
```

Also run `bin/luminari --build-info`, validate an existing old-format release,
and prove a failed install leaves both aliases and the live process unchanged.

### 5.3 CMake

Configure, build, test, and install in a fresh scratch build directory. Confirm
the candidate is `<build>/bin/luminari`, `$<TARGET_FILE:luminari>` reaches the
installer, the production-linked `cutest` target inherited the main target's
properties, and installation produces the same source-tree release layout as
Autotools.

### 5.4 Local runtime and copyover rehearsal

Run only on development and use `scripts/autorun/autorun.sh`, not
`luminari.service`.

1. Record a valid pre-rename immutable `circle` release, its build ID, the MUD
   PID, and a connected test descriptor.
2. Start that old release through the old `bin/circle` alias.
3. Install Phase A without restarting the MUD or supervisor.
4. Copy over old -> Phase A. Confirm the PID and player descriptor survive,
   `/proc/<pid>/exe` changes to a new `.../luminari`, and the startup log reports
   the new executable's build ID.
5. Copy over Phase A -> Phase A. Confirm the same properties again. This second
   copyover is the direct test of the new `EXE_FILE`.
6. With the compatibility link still present, perform a normal local autorun
   stop/start. Confirm the newly loaded supervisor reports
   `MUD_BINARY=luminari` and directly launches `bin/luminari`.
7. Boot to the game loop and review startup/copyover logs for unexpected
   `SYSERR` lines. Ollama, I3, and Discord connectivity are not required for
   this local gate unless those integrations are explicitly under test.

Use `/proc/<pid>/exe` as the independent authority for steps 4 and 5, and also
verify that the copyover-aware identity files converge on that value.

## 6. Production rollout for Phase A

Read `lib/.env` before touching a checkout. Develop and rehearse only where
`APP_ENV` identifies development. On production, perform only the approved
deployment and verification steps; do not edit source or create a branch or
worktree there.

1. Before installation, record:
   - the MUD PID and autorun supervisor PID;
   - `/proc/<mud-pid>/exe`, ELF build ID, Git commit, and SHA-256;
   - the type, link text, and resolved target of both `bin/circle` and any
     existing `bin/luminari`;
   - the active release manifest and debug sidecar;
   - service unit/drop-in environment, especially any `MUD_BINARY` override;
     and
   - a connected staff/player descriptor for continuity verification.
2. Require a valid `bin/circle` symlink for a zero-downtime transition. If it
   is the live regular executable, stop the rollout and schedule a controlled
   maintenance stop; never bypass the installer's live-inode refusal.
3. Back up the database row being changed and verify the `GDB` help tag is
   unique.
4. Deploy and install Phase A without stopping the old MUD or supervisor.
   Verify `bin/luminari` first, then `bin/circle -> luminari`, before issuing
   copyover.
5. Install and daemon-reload the updated `luminari.service` without restarting
   the active service. Verify the effective unit will set
   `MUD_BINARY=luminari` on its next start.
6. Apply and verify the help SQL migration and deploy the matching
   `help.hlp`.
7. Copy over old -> Phase A. Verify same PID, descriptor continuity, resolved
   executable, manifest, build ID, boot completion, and expected service
   health.
8. On a separately observed copyover, copy over Phase A -> Phase A to prove
   the new compiled path. Do not infer this from step 7.
9. Leave `bin/circle` in place. At a later maintenance restart or host reboot,
   reload the supervisor while the compatibility link still exists and verify
   that the new supervisor is configured for `luminari` and launches the
   canonical alias.
10. Retain logs and the old release directory through the Phase A bake period.

If either copyover fails, stop the rollout. A missing old alias is normally
caught before state teardown, but an `execl()` failure after teardown may
leave the surviving process without all auxiliary services.

## 7. Phase B: retire `bin/circle`

Phase B is a separate reviewed change. All of these conditions are required:

- The live MUD's `/proc/<pid>/exe` matches `readlink -f bin/luminari`, has
  basename `luminari`, and matches its manifest/build ID.
- Production has completed a copyover initiated by a Phase A binary, proving
  that `bin/luminari` was used. The old -> Phase A transition alone is not
  enough.
- The autorun supervisor itself has restarted since Phase A and its own state
  or startup log proves `MUD_BINARY=luminari`.
- Systemd unit content, drop-ins, cron jobs, operator wrappers, and environment
  overrides have been checked for `circle` executable references.
- Current operational docs and database/file help copies use `luminari`.
- `bin/circle` is exactly the transition symlink to `luminari`; it is not a
  regular file, directory, unrelated link, or live executable.
- The exact command to recreate the compatibility link has been rehearsed.

Then:

1. Stop creating `bin/circle` in the installer and update its tests.
2. Install Phase B; the install itself should not blindly delete an existing
   path.
3. Recheck every precondition on the live host.
4. Remove only the verified compatibility symlink.
5. Confirm a normal restart and a copyover still use `bin/luminari`.
6. Update this plan/index and transition notes. Preserve historical references
   and old release directories.

If any precondition is uncertain, leave the link in place.

## 8. Rollback

### 8.1 Phase A rollback

- Before the first copyover, repoint `bin/circle` to the recorded old release
  if necessary; the old process remains the authority.
- After Phase A is live, a new supervisor can launch an old-format release only
  if its identity logic accepts `<release>/circle` with `circle.debug`.
- To roll back to a pre-rename executable, keep or recreate
  `bin/circle -> luminari`, then atomically point `bin/luminari` at the selected
  old `<release>/circle`. The old binary's compiled-in copyover path will then
  continue to resolve.
- Verify the selected release's manifest, executable, debug build ID, and
  SHA-256 before changing an alias.

### 8.2 Phase B rollback

Recreate `bin/circle -> luminari` atomically before launching or copying over
to any pre-rename executable. Merely repointing `bin/luminari` to an old
`circle` release is not sufficient: that old process still has
`EXE_FILE=bin/circle` compiled into it.

Never reconstruct rollback by renaming files inside an immutable release.

## 9. Effort and scheduling

The edits are mostly mechanical, but the operational work is not a half-day
single deploy. Allow one focused implementation/review cycle for Phase A, a
separate local copyover rehearsal, a production bake period that includes two
distinct copyover directions and a supervisor restart, and a later Phase B
change. Phase B code is small; its evidence gates determine its schedule.

The pre-commit clang-format hook may realign trailing comments in the
`EXE_FILE` block when the string length changes. Accept that focused
formatting, then rebuild and rerun the affected tests before committing.

## 10. Adjacent finding kept out of the rename

`unittests/CuTest/test_syntax_check_boot.c` defaults to a 60-second alarm for a
full world syntax boot. This plan uses
`LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480` to give slower hosts explicit
headroom. Changing the global default is an independent test-policy decision
and should not be hidden in the binary rename.

## 11. Implementation progress log

Phase A is being implemented incrementally. This section records what has
landed so an interrupted session can resume without re-deriving state.

- [x] 4.1 Build target and C source (`Makefile.am`, `CMakeLists.txt`,
      `src/db.h`, `src/act.wizard.c`,
      `unittests/CuTest/test_syntax_check_boot.c`, `src/sysdep.h`,
      `.gitignore`).
- [x] 4.2 Installer and mixed release layouts (`install_versioned_binary.sh`,
      `test_versioned_binary_install.sh`).
- [x] 4.3 Autorun, systemd, deploy, and diagnostics. Autorun defaults to
      `luminari`, derives the debug sidecar from the resolved basename,
      publishes `MUD_BINARY` in `.autorun.state`, and refreshes the active
      executable from `/proc/<pid>/exe` after a same-PID copyover. Process
      discovery in the memory monitor, copyover diagnostics/watchdog, login
      smoke, and ferry soak now uses `.mud.pid` plus `/proc/<pid>/exe`
      instead of a process-name probe.
- [ ] 4.4 CI, test harnesses, and operational tooling.
- [ ] 4.5 Help, templates, and current documentation.
- [ ] 5 Phase A verification.
- [ ] 6 Production rollout (not started; development checkout only).
