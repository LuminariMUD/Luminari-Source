# Binary Rename: `circle` to `luminari`

Status: development implementation and rehearsal complete; production
maintenance cutover pending. The rename is not complete while any executable,
alias, release artifact, operational command, or compatibility path still uses
`circle`.

Last reviewed: 2026-08-21 against `master` at `97b0d2ee` plus the audit changes
recorded below.

## 1. Decision

The server executable is named `luminari` everywhere.

This is a clean cutover with planned downtime. There is no compatibility
period, compatibility symlink, mixed release layout, legacy executable
fallback, old-to-new copyover, or binary rollback path.

Git history is the record of the old executable name. The current tree should
not carry obsolete executable paths solely to preserve historical examples or
validation output. The repository-mandated frozen changelog files remain
unchanged because they record the tree as it was.

The rename does not affect:

- CircleMUD attribution, licenses, URLs, or project history;
- gameplay uses such as spell circles and the `circle` command;
- internal C identifiers such as `circle_random` and `circle_shutdown`;
- platform and configure macros such as `CIRCLE_UNIX` and `CIRCLE_MYSQL`; or
- the `luminari.service` unit name.

Those names are unrelated to executable compatibility.

## 2. Required final state

The work is complete only when all of the following are true:

- Autotools and CMake build a `luminari` target.
- The root build artifact is `luminari`; `make install` removes it.
- A release contains exactly `luminari`, `luminari.debug`, and `manifest`.
- `bin/luminari` is the only mutable server alias.
- `bin/circle` does not exist.
- No release below `bin/releases/` contains `circle` or `circle.debug`.
- Copyover executes `bin/luminari` and uses `luminari` as `argv[0]`.
- Autorun, systemd, deployment, diagnostics, tests, CI, and maintained tooling
  use `luminari` only.
- Current documentation and both copies of the `GDB` help entry use
  `bin/luminari` only.
- A static regression fails on executable-name uses of `circle` anywhere in
  the tracked tree, except this plan, the detector's own patterns, and the
  repository-mandated frozen changelog history.
- A stopped development server starts cleanly through
  `scripts/autorun/autorun.sh`, completes a `luminari` to `luminari` copyover,
  and restarts cleanly.

## 3. Audit baseline (removed)

At the start of the clean-cutover work, most build and runtime call sites
already used `luminari`, but the implementation deliberately included
compatibility that this plan rejects:

- the installer creates `bin/circle -> luminari`;
- the installer and autorun understand old `circle` release layouts;
- tests preserve and exercise mixed layouts and old-process copyover;
- the static check maintains broad historical and compatibility allowlists;
- the plan and ongoing-project index describe a two-phase migration; and
- several maintained documents still contain obsolete executable commands.

Delete that compatibility code instead of extending it.

## 4. Implementation

### 4.1 Build and copyover

Verify and retain the already-canonical changes in:

- `Makefile.am`;
- `CMakeLists.txt`;
- `.gitignore`;
- `src/db.h`;
- `src/act.wizard.c`;
- `src/sysdep.h`; and
- `unittests/CuTest/test_syntax_check_boot.c`.

The only server target, root artifact, diagnostic path, copyover path, and
synthetic `argv[0]` is `luminari`.

Do not add a source file for this rename. If that changes, update both
`Makefile.am` and `CMakeLists.txt` as required by repository policy.

### 4.2 Simplify release installation

Reduce `scripts/deployment/install_versioned_binary.sh` to one format:

```text
bin/releases/<ELF-build-id>/luminari
bin/releases/<ELF-build-id>/luminari.debug
bin/releases/<ELF-build-id>/manifest
bin/luminari -> releases/<ELF-build-id>/luminari
```

Keep the useful release integrity behavior:

- validate the candidate's build information, ELF build ID, and SHA-256;
- create and validate the immutable release before switching the alias;
- reject incomplete releases and build-ID/SHA collisions;
- atomically publish `bin/luminari`; and
- reject directories and special files at the canonical alias.

Remove all binary-compatibility behavior:

- `compat_name` and every `bin/circle` operation;
- old-format basename detection;
- mixed `circle`/`luminari` release validation;
- migration or archival of regular old-name aliases;
- old-name debug-sidecar derivation; and
- compatibility messages and recovery instructions.

Update `scripts/deployment/test_versioned_binary_install.sh` to cover only the
canonical format: fresh install, repeated install, atomic alias switching,
live release retention, incomplete-release refusal, collision refusal, broken
canonical-link replacement, and canonical-path type rejection.

### 4.3 Remove runtime and tooling compatibility

Autorun must default to and report `MUD_BINARY=luminari`. Release identity and
debug-sidecar lookup may derive the basename generically, but no branch should
special-case or accept `circle`.

Keep PID ownership based on `.mud.pid` and `/proc/<pid>/exe`; do not use
process-name discovery.

Update systemd, deploy scripts, wrappers, diagnostics, CI, test harnesses,
PowerShell tooling, and Python tooling so they use only `luminari`. Remove
tests whose only purpose is old-name compatibility or old-to-new copyover.

The static regression should detect at least:

```text
bin/circle
./circle
circle.debug
circle.exe
bin_PROGRAMS = circle
add_executable(circle
$<TARGET_FILE:circle>
circle_SOURCES
circle_LDADD
MUD_BINARY=circle
process-name probes for circle
```

Do not use a compatibility allowlist for those executable forms. Exclude this
plan and the detector itself because they necessarily name the rejected forms.
The repository-mandated frozen changelog history is the only historical
exemption. Continue ignoring unrelated CircleMUD, gameplay, and internal API
uses.

### 4.4 Documentation and help

Replace obsolete executable commands in all maintained documentation,
templates, examples, and help text. Do not preserve an obsolete path merely
because it appeared in a dated report; Git already preserves that version.
Leave the repository-mandated frozen changelog paths unchanged.

If a completed report has no continuing operational value, delete it instead
of maintaining exceptions for it. If the report remains useful, update its
executable paths to the canonical name.

Set the file help entry and the database `GDB` row to `gdb bin/luminari`.
Keep the SQL update idempotent, require exactly one `GDB` row, and avoid
retaining an old executable string as migration machinery after the database
has been updated.

Update this plan and `docs/ongoing-projects/README_ongoing-projects.md` only
after the implementation and cutover checks pass.

All edited documentation, help text, and SQL must remain ASCII, UTF-8, and LF.

## 5. Clean cutover

There is no live migration from an old executable. Rehearse this sequence on
development before production:

1. Build and test the `luminari` candidate while the existing service runs.
2. Begin a maintenance window and stop the autorun supervisor and MUD.
3. Verify no MUD or supervisor process from this checkout remains.
4. Remove `bin/circle`, root-level old-name artifacts, and old-name release
   directories. Resolve and inspect exact paths before deleting them.
5. Install the candidate, producing only the canonical release layout and
   `bin/luminari` alias.
6. Install and reload the updated `luminari.service` definition.
7. Apply and verify the `GDB` help update.
8. Start through the normal supervisor and wait for the health check.
9. Verify `/proc/<mud-pid>/exe`, ELF build ID, manifest, and SHA-256 all match
   the active `luminari` release.
10. Perform one `luminari` to `luminari` copyover and one normal stop/start.

If the cutover fails, keep the service stopped, correct the fault, reinstall,
and start the canonical binary. Fix forward; do not recreate old aliases or
old-name releases.

Production source edits remain prohibited. Build and rehearse in development,
then deploy the reviewed result during the maintenance window.

## 6. Verification gates

Run on development:

```bash
git diff --check
bash scripts/deployment/test_binary_name_static.sh
make clean
make -j"$(nproc)"
LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480 make test-all
```

Then use a fresh CMake directory:

```bash
cmake -S . -B <scratch> -DBUILD_TESTS=ON
cmake --build <scratch> -j"$(nproc)"
ctest --test-dir <scratch> --output-on-failure
cmake --install <scratch>
```

After the final install, prove:

```text
root luminari                         absent
root circle                           absent
bin/luminari                          executable symlink
bin/circle                            absent
active release/luminari               executable regular file
active release/luminari.debug         matching ELF build ID
active release/manifest               matching build ID and SHA-256
old-name release artifacts            absent
```

Also run `bin/luminari --build-info`, validate all changed shell scripts with
`bash -n`, run the focused installer and autorun regressions, and review every
remaining tracked `circle` occurrence. Every remaining occurrence must be
unrelated to the executable rename, be necessary detector/spec text, or be in
the repository-mandated frozen changelog history.

## 7. Completion rule

Do not mark this project complete because the new target builds. Completion
requires removal of the compatibility implementation, removal of the old
artifacts and executable references, a clean development restart and
copyover, and a successful production maintenance cutover.

There is no later compatibility-removal phase. The clean cutover is the rename.

## 8. Progress log

Append one entry per working session so an interrupted session can resume.

### 2026-08-21 - compatibility implementation removed

Done:

- `scripts/deployment/install_versioned_binary.sh` reduced to the canonical
  release layout. `compat_name`, `bin/circle`, old-format basename detection,
  mixed release validation, and regular-alias archival are gone. A canonical
  alias that is not a symbolic link is now rejected outright.
- `scripts/deployment/test_versioned_binary_install.sh` rewritten for the
  canonical format only: fresh install, live release retention, alias
  switching, idempotent reinstall, incomplete-release refusal, build-ID
  collision refusal, broken-link replacement, and canonical-path type
  rejection. PASS.
- `scripts/autorun/test_autorun_supervision.sh` no longer builds an old-name
  alias or accepts an old-name systemd `ExecStart`.
- `.github/workflows/test.yml` asserts the canonical alias only.
- `scripts/world/wtool_lib/rol_phase8.py` dropped the `root_circle_absent`
  gate key. `scripts/world/tests/test_rol_phase8.py` PASS (9 tests).
- `sql/components/help_gdb_binary_rename.sql` replaced by
  `sql/components/help_gdb_binary_path.sql`, which normalizes any `gdb bin/*`
  path to `gdb bin/luminari` instead of carrying the old string as migration
  machinery. `sql/components/ci_schema_manifest.txt` updated. Verified on the
  development database: exactly one `GDB` row, canonical before and after, the
  component is a no-op.
- Maintained documentation, validation reports, and dated progress tables now
  use `bin/luminari` and the `luminari` build target.
- `scripts/deployment/test_binary_name_static.sh` lost the compatibility and
  historical allowlists. The only exempt paths are this plan, the frozen
  changelog history, and the detector's own pattern list. Added a
  `MUD_BINARY=circle` pattern. PASS.
- `docs/ongoing-projects/README_ongoing-projects.md` describes the clean
  cutover.

### 2026-08-21 - development cutover and verification gates

The development cutover in section 5 is complete. No MUD or supervisor process
from this checkout was running, so the maintenance window was empty.

- Removed `bin/circle`, and removed 303 old-name release directories under
  `bin/releases/`. Each held only `circle`, `circle.debug`, and `manifest`; no
  release held both basenames, and the active alias already pointed at a
  canonical release. Three canonical releases were retained.
- No root-level `luminari` or `circle` artifact existed before or after.

Verification gates, all on development:

- `git diff --check` clean.
- `make clean && make -j"$(nproc)"`: success, zero compiler warnings.
- `LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480 make test-all`: PASS,
  782 production-linked tests and 29 protocol-parser tests.
- Fresh CMake directory with `-DBUILD_TESTS=ON`: configure, build, and
  `ctest`: 14 of 14 tests passed. `cmake --install` published a canonical
  release and alias. `make install` then restored the Autotools release as
  the active alias.
- Post-install proof: root `luminari` absent, root `circle` absent,
  `bin/luminari` an executable symlink, `bin/circle` absent, the active
  release a regular executable with a matching `luminari.debug` build ID and a
  manifest matching both the build ID and the SHA-256, and no old-name release
  artifacts anywhere.
- `bin/luminari --build-info` reports the expected version and commit.
- `bash -n` clean on every changed shell script.
- `scripts/deployment/test_binary_name_static.sh`: PASS.
- `scripts/deployment/test_versioned_binary_install.sh`: PASS.
- `scripts/autorun/test_autorun_supervision.sh`: PASS.
- Runtime rehearsal through `scripts/autorun/autorun.sh`: clean start on the
  canonical release, a `luminari` to `luminari` copyover through
  `scripts/development/dev_kohdee_login_smoke.sh --copyover-check score` that
  kept the same PID and the connected character, then a clean stop and
  restart. Active identity matched the installed release at every step.

Second sweep of the tracked tree found and fixed executable references the
original pattern list missed: `ps aux | grep circle` in three guides,
`cmake --build --target circle` in the CMake guide, root-artifact claims in
two vessel test reports and one campaign report, and a release executable path
in a completed copyover report. The static check gained `grep circle`,
`--target circle`, and `MUD_BINARY=circle` patterns. Every remaining tracked
`circle` occurrence is CircleMUD heritage, a `CIRCLE_*` platform macro, an
internal `circle_*` C identifier, gameplay spell-circle text, or one of the
two exempt files.

### 2026-08-21 - production dry run, read only

Surveyed production over `ssh lumi` without stopping or changing anything.
Production is entirely pre-rename:

| Item | State |
|------|-------|
| Checkout | `/home/luminari/Luminari-Source`, `master` at `4a61df85`, clean |
| Live MUD | PID from `.mud.pid`, executing `bin/releases/760ea71f.../circle` |
| Supervisor | `luminari.service` active, autorun and watchdog running |
| `bin/circle` | symlink to `releases/760ea71f.../circle` |
| `bin/luminari` | absent |
| Releases | 21 directories, all `circle` and `circle.debug`, 0 canonical, 1.1 GB |
| Root artifacts | none |
| `GDB` help row | exactly 1 row, still `gdb bin/circle` |
| Installed unit | differs from the repo copy of `luminari.service` |

What the maintenance window must therefore do, in this order:

1. Announce downtime and stop `luminari.service`; confirm no MUD, autorun, or
   watchdog process from that checkout survives.
2. Update the checkout to the reviewed commit and build the candidate.
3. Remove `bin/circle` and the 21 old-name release directories, resolving each
   path first. This reclaims about 1.1 GB.
4. `make install`, producing only `bin/luminari` and one canonical release.
5. Install the repo copy of `luminari.service` and `systemctl daemon-reload`.
   The installed unit currently differs, so this is a real change: it adds the
   readiness probe and pins `MUD_BINARY=luminari`.
6. Apply `sql/components/help_gdb_binary_path.sql`; the production row is
   still obsolete, so this one is not a no-op there.
7. Start through the supervisor, wait for the health check, and prove
   `/proc/<pid>/exe`, the ELF build ID, the manifest, and the SHA-256 all
   agree with the active `luminari` release.
8. Perform one `luminari` to `luminari` copyover and one normal stop and start.

Because production has no canonical release at all, there is no rollback to a
`luminari` build there. The rollback material is the old `circle` releases, so
step 3 is the point of no return: do not delete them until step 4 has produced
a verified canonical release, or accept that recovery means rebuilding.

Not done yet:

- The production maintenance cutover itself. It needs an announced window and
  explicit authorization to take the live game down.

### 2026-08-21 - independent implementation audit and hardening

Audited the required final state against the current development worktree
rather than relying on the earlier progress entries. The audit found and fixed
the following gaps:

- `scripts/deployment/install_versioned_binary.sh` now rejects an existing
  release unless it is a real directory containing exactly three regular
  files: `luminari`, `luminari.debug`, and `manifest`. It also validates every
  manifest field against the candidate, not only the build ID and SHA-256.
- `scripts/deployment/test_versioned_binary_install.sh` now covers exact
  release contents, unexpected entries, malformed manifests, symlinked release
  metadata, and a FIFO at the canonical alias in addition to the original
  canonical-format cases. PASS.
- `sql/components/help_gdb_binary_path.sql` now fails unless exactly one `GDB`
  row exists and verifies the canonical path before committing. Applied twice
  on development: one row, one canonical reference, and no helper procedure
  retained.
- Fixed missed executable references in both Aider templates, the narrative
  weaver rollback instructions, and the vessel testing guide. The edited
  documentation is ASCII, UTF-8, and LF. The static detector now covers the
  missed process-probe and root-artifact forms. Both build systems invoke it
  through Bash, so its tracked mode of 0644 works in a fresh checkout.
- Refreshed the ignored in-tree CMake configuration and moved its stale
  pre-rename binary and target directory out of the workspace. No old-name
  build or release artifact remains in the development checkout.

Verification after the fixes:

- `git diff --check` and all changed shell syntax checks passed.
- Warning-free clean Autotools build passed.
- `LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480 make test-all` passed: 782
  production-linked tests, 29 protocol-parser tests, 454 world-tool tests, and
  all focused shell/schema regressions. `make install` completed afterward.
- Fresh CMake configure/build/install passed; CTest passed 14 of 14 tests.
  The final Autotools install restored the preferred active release.
- Final artifact proof passed: no root artifact, no `bin/circle`, no old-name
  release artifact, an executable `bin/luminari` symlink, exact three-file
  releases, and matching executable/debug/manifest build IDs and SHA-256.
- Runtime rehearsal passed through `scripts/autorun/autorun.sh`: clean boot,
  health check, connected-character `luminari` to `luminari` same-PID copyover,
  clean stop, clean restart after supervisor exit, and final clean stop.

The production maintenance cutover remains the only unfinished section. It was
not attempted because taking down the live game requires an announced window
and explicit authorization.

### 2026-08-21 - production preflight refreshed, read only

Repeated the production survey after the independent implementation audit.
The SSH account required a command-scoped Git safe-directory override and
read-only `sudo` for owner-only runtime and release details. No production
file, process, service, database row, or configuration was changed.

- The production checkout remains clean on `master` at `4a61df85`.
- `luminari.service` remains active and enabled. Its supervisor PID is 1194067,
  and `.mud.pid` identifies live MUD PID 1194248.
- The live MUD still executes
  `bin/releases/760ea71f.../circle`; its ELF build ID matches that release.
- `bin/circle` still points to that release, while `bin/luminari` remains
  absent. All 21 releases contain `circle` and `circle.debug`; none contains
  `luminari`. The release tree occupies 1,176,294,585 bytes.
- The production `GDB` help state is exactly one row, zero canonical paths,
  and one `gdb bin/circle` path.
- The installed unit still executes the project-root `autorun.sh`, has no
  `MUD_BINARY` environment setting, and therefore still differs from the
  reviewed unit in this repository.
- The checkout filesystem has 188,809,392,128 bytes free, so the canonical
  candidate and temporary build/install files have ample space.

The required maintenance sequence and point-of-no-return warning from the
earlier production dry run remain current. The cutover still requires explicit
authorization and an announced downtime window.
