# Binary Rename: `circle` -> `luminari`

> Planning document. Not a description of current behavior. Nothing here is
> implemented yet.

Analysis date: 2026-08-20, against `master` at `4a61df85`.

Rename the built server executable from `circle` to `luminari`. The name is
inherited from CircleMUD and no longer describes the program. This is a
cosmetic rename of one artifact with a small number of load-bearing
references, not a refactor.

## 1. Scope summary

| Category | Files | References | Risk |
|----------|-------|------------|------|
| C source (`EXE_FILE`) | 1 | 3 | **High** - copyover re-exec path |
| Release and deploy scripts | 6 | ~45 | High - release layout and launch alias |
| Build system | 2 | ~30 | Medium |
| Runtime and debug scripts | 5 | ~20 | Medium |
| Test harnesses | ~16 | ~60 | Low |
| CI workflows | 2 | ~6 | Low |
| Docs | 38 | 123 | Low, but the bulk of the diff |
| Misc (`.gitignore`, helpfile, templates) | 6 | ~10 | Low |

Roughly 44 non-doc files and 277 total references.

### 1.1 Explicit non-goals

Two categories of `circle` must **not** be renamed:

- **CircleMUD attribution.** 325 occurrences of `CircleMUD` / `circlemud`
  across the tree record the codebase's heritage and appear in license and
  credits text. A blanket `sed s/circle/luminari/g` would rewrite these and is
  the single most likely way to get this change wrong. Every substitution must
  be anchored to a path or identifier form, never to the bare word.
- **Internal C identifiers.** About 120 occurrences of `circle_*` symbols
  (`circle_counter` 47, `circle_level` 15, `circle_ind` 13, `circle_shutdown`
  10, `circle_restrict` 10, `circle_follow` 10, `circle_for_spell` 7,
  `circle_reboot` 6, and others). These are internal state, unrelated to the
  artifact name. Renaming them is a separate optional cosmetic pass with its
  own risk profile; it is out of scope here.

Also out of scope: the repository directory name (`Luminari-Source`), the
systemd unit name (`luminari.service`), and the `lib/` data layout.

## 2. The one dangerous part: copyover

`src/db.h` defines the path the running server hands to `execl()` when it
copies over:

```c
#if defined(CIRCLE_AMIGA)
#define EXE_FILE "/bin/circle"
#elif defined(CIRCLE_MACINTOSH)
#define EXE_FILE "::bin:circle"
#else
#define EXE_FILE "bin/circle"
#endif
```

The failure mode is specific and it lands on a live server with players
connected. During the deploy that introduces the rename, the **running**
process is the old build, and its compiled-in `EXE_FILE` is still
`bin/circle`. It will `execl("bin/circle", ...)`. If that path no longer
resolves, the copyover does not fall back - the process is gone and the
session dies with it.

Therefore `bin/circle` must survive as a resolvable path across the
transition copyover, and may only be retired after a server that was *started
from* `bin/luminari` has completed a copyover.

The two non-`#else` branches are dead on every supported platform. Rename them
for consistency but treat only the `#else` value as functional.

## 3. Current mechanics the plan has to preserve

- **Release layout.** `scripts/deployment/install_versioned_binary.sh` builds
  `bin/releases/<elf-build-id>/` containing `circle`, `circle.debug`, and
  `manifest`, then atomically repoints the `bin/circle` symlink at
  `releases/<id>/circle` via `ln -s` + `mv -Tf`.
- **Legacy alias migration.** The same script already has an
  `archive_legacy_alias` path for the case where `bin/circle` is a regular
  file rather than a symlink: it reads the ELF build ID, files the binary as a
  `legacy` release, and removes it. It refuses to act if `.mud.pid` shows that
  file is the live process (`refusing to replace a live legacy bin/circle`).
  This is the natural model to imitate for the rename, and the refusal check
  must keep working.
- **Autotools staging.** `Makefile.am` sets `bin_PROGRAMS = circle` and stages
  through `bindir = $(top_builddir)/bin/.circle-install`, then
  `install-exec-hook` calls the versioned installer and deletes the staged
  copy and the root-level artifact.
- **CMake.** `add_executable(circle ...)` plus roughly 20 `target_*(circle ...)`
  calls, and an `install(CODE ...)` block passing `$<TARGET_FILE:circle>` to the
  same installer script.
- **Launcher indirection.** `scripts/autorun/autorun.sh` already parameterizes
  the name: `readonly MUD_BINARY="${MUD_BINARY:-circle}"` with
  `BIN_DIR="${BIN_DIR:-bin}"`. Changing one default covers its several call
  sites. `luminari.service` invokes `autorun.sh`, never the binary directly,
  so the unit file needs no change.

## 4. Phasing

The rename ships in two deploys. Phase B must not begin until a production
copyover has succeeded on a Phase A build.

### Phase A - rename, with `bin/circle` retained as a compatibility symlink

1. **Build system.** `Makefile.am`: `bin_PROGRAMS = luminari`, staging dir
   `bin/.luminari-install`, and the `install-exec-hook` paths.
   `CMakeLists.txt`: `add_executable(luminari ...)`, every `target_*` call, and
   `$<TARGET_FILE:luminari>`. Source-file lists are untouched.
2. **`src/db.h`.** `EXE_FILE` becomes `bin/luminari` in all three branches.
3. **Installer.** `install_versioned_binary.sh` writes `luminari` and
   `luminari.debug` into each release directory, points a `bin/luminari`
   symlink at them, and *additionally* creates `bin/circle` as a symlink to
   `luminari`. Keep `archive_legacy_alias` working for a regular-file
   `bin/circle`, and keep the live-process refusal.
4. **Launcher.** `autorun.sh` default becomes `MUD_BINARY="${MUD_BINARY:-luminari}"`.
   Update the watchdog, `debug_game.sh`, `vgrind.sh`, `deploy.sh`,
   `move_bin.sh`, `setup.sh`.
5. **Everything else.** Test harnesses (`scripts/vessels/*`,
   `test_autorun_supervision.sh`, `test_versioned_binary_install.sh` - 14
   references, the densest single file), CI workflows, `.gitignore:383`,
   `lib/text/help/help.hlp`, `util/` templates, `docs/`.
6. **Helpfile.** Per repository rule, any `help.hlp` change must also be
   applied to the database copy.

At the end of Phase A both `bin/luminari` and `bin/circle` resolve to the same
release. Old builds copying over still find `bin/circle`; new builds use
`bin/luminari`.

### Phase B - retire the compatibility symlink

Preconditions, all verified on production:

- The live process was launched from `bin/luminari`.
- At least one copyover has completed on a Phase A build.
- The boot log records the expected build identity, and `readlink` on the
  live process confirms the new path.

Then remove the `bin/circle` creation from `install_versioned_binary.sh`,
delete the existing symlink, and drop the compatibility note from the docs.

Phase B is a one-line revert of a symlink. If any precondition is unmet,
simply leave the symlink in place - it costs nothing.

## 5. Verification

Per phase, in order:

1. `make clean && make -j$(nproc)` - confirm the emitted artifact is
   `luminari` and no target still references `circle`.
2. `make install` - confirm `bin/releases/<id>/luminari`, `luminari.debug`,
   `manifest`, and that both `bin/luminari` and (Phase A) `bin/circle`
   resolve.
3. `LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS=480 make test` - full suite. The
   default 60s syntax-check timeout is too short on slower hosts; see
   section 7.
4. `cmake` configure + build + install in a scratch dir - the CMake path is
   separate from autotools and is easy to miss.
5. `scripts/deployment/test_versioned_binary_install.sh` - the densest
   consumer of the release layout.
6. `scripts/autorun/test_autorun_supervision.sh`.
7. **Copyover rehearsal on dev, twice**: old build -> new build (exercises the
   `bin/circle` fallback), then new build -> new build (exercises
   `bin/luminari`). This is the test that actually matters. Confirm player
   descriptors survive both.
8. Boot and confirm `0` SYSERRs and an unchanged special-procedure binding
   report (`1922 selected`, `1291 wrapped`, `0` non-wrapper collisions as of
   `4a61df85`).

## 6. Rollback

Phase A is a normal revert plus a rebuild; the `bin/circle` symlink means an
old binary still launches. The irreversible-feeling step is Phase B, which is
why it is gated behind a completed copyover. Release directories are immutable
and keyed by build ID, so any prior release can be relaunched by repointing
the alias.

## 7. Adjacent findings (not part of this change)

Recorded here because they surfaced during scoping; each is independent.

1. **Shared-host process matching.** Production (`plesk.luminarimud.com`) runs
   several unrelated MUDs whose binaries are also named `circle` (users `aod`,
   `swrpg`, `frmud`). Two scripts match on the bare name and can select
   another game's process:
   - `scripts/process-memory/monitor_process_memory.sh:88` - `pgrep -x circle | tail -n 1`
   - `scripts/copyover/enhanced_copyover_diagnostic.sh:15,17` - `pgrep -f circle`

   The rename incidentally fixes this, which is a genuine secondary benefit,
   but both should be pinned to the project path or `.mud.pid` regardless.
2. **Syntax-check boot timeout.** `unittests/CuTest/test_syntax_check_boot.c`
   defaults to 60s (`SYNTAX_CHECK_DEFAULT_TIMEOUT_SECONDS`) for a world boot
   that takes roughly 180s on a WSL development host, so
   `Test_syntax_check_encounter_world_boots_and_cleans_up_once` fails by
   `SIGALRM` there. `LUMINARI_TEST_SYNTAX_TIMEOUT_SECONDS` overrides it. Worth
   either raising the default or documenting it in the testing guide.

## 8. Estimate

Roughly half a day. Low intellectual difficulty and a large but mechanical
diff, with concentrated risk in exactly two places: `EXE_FILE` and the
release-alias transition. Docs are the bulk of the line count and the least
risky part.

Expect the pre-commit `clang-format` hook to realign trailing comments on the
`EXE_FILE` block, since the replacement strings change length. Accept the
reformat, then rebuild and re-test before committing.
