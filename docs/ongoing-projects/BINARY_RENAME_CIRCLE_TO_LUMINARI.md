# Binary Rename: `circle` to `luminari`

Status: clean-cutover plan. The rename is not complete while any executable,
alias, release artifact, operational command, or compatibility path still uses
`circle`.

Last reviewed: 2026-08-21 against `master` at `6a6fa64d`.

## 1. Decision

The server executable is named `luminari` everywhere.

This is a clean cutover with planned downtime. There is no compatibility
period, compatibility symlink, mixed release layout, legacy executable
fallback, old-to-new copyover, or binary rollback path.

Git history is the record of the old executable name. The current tree should
not carry obsolete executable paths solely to preserve historical examples or
validation output.

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
  the tracked tree, except this plan while it describes the rename.
- A stopped development server starts cleanly through
  `scripts/autorun/autorun.sh`, completes a `luminari` to `luminari` copyover,
  and restarts cleanly.

## 3. Current state to remove

Most build and runtime call sites already use `luminari`, but the existing
implementation deliberately added compatibility that this plan rejects:

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

Do not use a compatibility or historical allowlist for those executable
forms. Exclude this plan itself, because it necessarily names the rejected
forms. Continue ignoring unrelated CircleMUD, gameplay, and internal API uses.

### 4.4 Documentation and help

Replace obsolete executable commands in all maintained documentation,
templates, examples, and help text. Do not preserve an obsolete path merely
because it appeared in a dated report; Git already preserves that version.

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
unrelated to the executable rename or be inside this plan.

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

Not done yet:

- Full build and test gates from section 6.
- The development cutover in section 5, including removing `bin/circle`, the
  root-level old-name artifacts, and old-name release directories.
- The production maintenance window.
