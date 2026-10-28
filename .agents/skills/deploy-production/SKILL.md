---
name: deploy-production
description: >-
  Deploy a reviewed LuminariMUD release to the authorized production host, keeping players
  online during preparation and the clean production build. Install an immutable release,
  use a three-minute copyover countdown when compatible, and verify runtime identity,
  database readiness, player login, and release news.
---

# Deploy Luminari Production

An explicit deployment request authorizes updating the identified production checkout to
the approved release, its required build/install and startup migrations, release news,
and the announced cutover described here. Reuse that authorization; do not ask again for
ordinary steps. A request to create, adapt, review, or explain this skill does not execute it.
Connection details in `lib/.env` alone never authorize remote actions.

Keep the game running through research, backups, content preparation, and compilation.
Complete the build before the player countdown. Treat the reviewed release as qualified;
do not turn deployment into burn-in, a production test suite, or a broad audit. Diagnose
build failures in development and release reviewed fixes; do not edit production source,
resolve source conflicts there, or create production branches/worktrees.

Paths are repository-relative. Read `AGENTS.md` and the current
[deployment guide](../../../docs/deployment/DEPLOYMENT_GUIDE.md). Trace commands and
scripts at the release being deployed; documentation is not proof of the live host's state.

## 1. Confirm the target and update the checkout

- Read only `APP_ENV` from the starting checkout's `lib/.env`, without dumping credentials.
  Local authoring requires `development`; the actual deployment target must identify as
  `production`. Verify the SSH host, checkout, runtime directory, service owner, and database
  target using protected configuration. Do not log passwords or put them in command arguments.
- Inspect the live unit with `systemctl cat/show luminari.service`, autorun status,
  `.autorun.state`, and the actual game child's executable/listener ownership. The canonical
  production unit is system-scope, supervising `scripts/autorun/autorun.sh`; do not assume
  Duris's user-scope service or staff character. The MUD game port is **4100 only**.
- Discover the intended remote and branch; default to the verified repository default branch
  (currently `master`) unless the user selects a release. Inspect branch/status and record
  pre-update HEAD, the current immutable release, executable hash/build ID, and service state.
  Preserve unrelated tracked edits and ignored runtime data.
- Fetch and pin the approved remote revision, then fast-forward the existing release checkout
  to that exact revision. For example, after verifying `origin/master`, use
  `git fetch origin master` and `git merge --ff-only PINNED_SHA`. Never reset, clean, force a
  merge, or discard production changes to make the update pass. Divergence or an overlapping
  local edit requires reconciliation in development; keep the current game running.

Preserve `lib/.env`, `lib/mysql_config`, and the real headers under `src/config/`.
Never replace them from examples. If old headers remain directly under `src/`, stop the
build path and report the owner-required move from `AGENTS.md`; agents do not move them.
Use local autorun with `MUD_PORT=4100` for any separately requested development run,
never the production service.

## 2. Prepare recovery, data, and release content

Preserve the current immutable executable, matching debug symbols and manifest, world/player
data, news, and a current database backup in a protected recovery location. Record rollback
compatibility before installing anything. Retain existing `bin/releases/` and crash evidence.
Check backup success and contents; compressed output existing is not proof of a usable dump.

Inspect backup tooling before using it. The current `scripts/backup.sh` puts the database
password in `mysqldump` arguments and prunes older backups by default. Use an established
host backup procedure or a protected client option file with the actual `lib/mysql_config`
target instead; keep credentials and player records out of logs and Git. Ensure database
engine/DDL activity permits a consistent online backup, and preserve runtime files affected
by the release. Do not introduce an outage simply to rehearse a restore.

Luminari performs missing-table initialization and versioned migrations during boot.
Inspect the release changes in `src/database/db_startup_init.c`, `src/database/db_init.c`,
`src/database/db_init_data.c`, account migrations, and affected subsystem SQL. Compare
required columns/indexes and applicable migration records with the live database. There is
no Duris migration runner or universal runtime-compatibility command here.

Determine which changes run during ordinary startup and whether they are compatible with
other active database writers and the old release. Do not launch a second server or a
`-c` boot against production to inspect migrations: startup can write data. Do not import
`sql/master_schema.sql`, replay historical repair scripts, or run the fresh-install
`deploy.sh` workflow over production. It can provision credentials and world data.

Prepare only release-required data/configuration steps, following their maintained runbooks.
Keep external writers online when compatible. A pending migration alone does not justify
stopping them. If stopped writers are necessary, identify the concrete incompatible operation,
prepare the exact short maintenance sequence, and finish all independent work first. A missing
safe procedure leaves the current game running; never bypass a migration guard or improvise
production source edits. Credential or protected-header edits require their own authorization.

Follow [news-update](../news-update/SKILL.md) for the actual production news file and the
pinned release SHA. Prepare complete player coverage and preserve the existing archive.
Publish the reviewed file near cutover and read it in-game after activation. Do not use
Duris paths, SQL news imports, or a forced Git add for ignored runtime news.

When help changes or cross-environment help synchronization are explicitly included, follow
[help-sync](../help-sync/SKILL.md) end to end. An invocation/request to sync authorizes the
full `sync --authorize-production` workflow and its verification; never add a second
confirmation or defer help because production code lags. A general deployment does not
silently authorize merging the entire development help catalog. Never substitute copying
`help.hlp` or ad hoc production SQL for the sync engine. Include verified new help topics
in news when relevant.

If the user also requests feedback triage, trace Luminari's actual sources: `lib/misc/bugs`,
`lib/misc/ideas`, `lib/misc/typos`, and the help search/history paths in `src/core/help.c`.
Snapshot privately, research each report against current source and existing issues, and
report dispositions. Issue filing or clearing live feedback needs that scope to be authorized.
Preserve concurrent arrivals; never import Duris's feedback paths or force-track empty
runtime files. Feedback housekeeping must not lengthen the cutover outage.

## 3. Complete the production build while players remain online

Use the configured Autotools build. Record existing configure options and preserve required
host choices when enabling the production profile. Regenerate with `autoreconf -fvi` only
when configure/build inputs require it. For a clean release build, run each step only after
the previous succeeds, from the verified production checkout:

```bash
make clean
./configure --enable-production
make -j"$(nproc)"
./scripts/deployment/verify_hardened_binary.sh ./luminari
./luminari --build-info
```

The configure command above shows the required profile; retain other verified host options.
If no generated Makefile exists, configure before cleaning. The clean release build is
deliberate here; routine source edits elsewhere do not require `make clean`.

Inspect the full build log for errors and warnings, required dynamic libraries, and the
candidate's commit/dirty flag. Do not deploy an unexplained dirty build. A failure leaves
the old game and its installed release intact. Repair code in development, then deploy the
reviewed corrected revision; do not bypass the production hardening check.

Before installation, inspect `Makefile.am`'s install hook and `scripts/provision_artifacts.sh`:
`make install` also provisions artifact world/help files and indexes. Preserve affected live
files and coordinate with OLC writers so installation cannot overwrite concurrent edits.
Resolve required migrations and rollback compatibility before switching the launch alias,
since an unexpected autorun restart can use it immediately.

Install only the successful candidate:

```bash
make install
./scripts/deployment/verify_hardened_binary.sh ./bin/luminari
./bin/luminari --build-info
test ! -e ./luminari
```

The supported installer creates `bin/releases/<ELF-build-ID>/` with the executable,
debug file, and manifest, then atomically changes the `bin/luminari` symlink. Never copy
over a running executable inode, delete old releases, or use Duris `dms_new` promotion.
If the installer refuses a legacy regular alias, use the reviewed maintenance procedure;
do not remove the launch target while the running service may need it.

Check that installation and artifact provisioning both completed, the alias/manifest/hash
match the candidate, and the current child still runs the prior release. If a later install
hook failed after alias promotion, report that partial state and resolve it before cutover.

## 4. Announce and activate

Log in on port 4100 with the authorized staff account and select a character with the
required privileges. Never assume Duris's Veridian account. Handle account menus, reconnects,
and pagers without recording credentials. Check for an existing countdown through the
available staff/event state and observed broadcasts; do not create a competing schedule.

For an ordinary compatible code deployment, use:

```text
copyover 180
```

Luminari's `do_copyover()` accepts **seconds** and checks `LVL_GRSTAFF`. It schedules
native player warnings and executes the installed immutable release. Stay connected and
in play until it completes; the initiating character's event/state matters. Observe the
countdown through its three-minute, one-minute, 30-second, 10-second, and final warnings.
Use `copyover cancel` only for this session's own schedule when cancellation is required.
Do not resend the timer to check progress or use bare `copyover`, which is immediate.

**Do not use `shutdown reboot 3 ...`.** Luminari's `do_shutdown()` reads only the first
argument: `shutdown reboot` acts immediately and skips OLC autosave. `reloadimm news`
only reloads text; it does not activate code. Recheck these handlers in
`src/act/act.wizard.c`, `src/core/db.c`, and `src/core/interpreter.c` if behavior changes.

A copyover retains the process's supervisor/environment and can keep the same PID. If a
launcher/unit/environment change or an incompatible migration needs a full service stop,
prepare that separate cutover before scheduling anything. Give players a full 180-second
warning using a source-verified global broadcast command with remaining-time updates.
At the announced deadline, use the actual production service's graceful stop path, verify
the game and other required writers have stopped, execute only the prepared operations,
then restore their recorded service states. Keep MariaDB up. Do not race autorun's restart
loop or replace a graceful stop with broad process kills. Installation of a changed unit
must preserve host overrides and use the maintained deployment procedure.

No compilation, release research, or unrelated housekeeping belongs in the interruption.
If required migration/compatibility verification fails, leave incompatible writers stopped
and report the actual state; never blindly restart an old binary against changed data.

## 5. Verify the running release and finish

Start observing before activation: systemd journal, root `syslog`, `log/`, autorun state,
and new crash output under `dumps/`. Follow rotation and new files. Inspect actual output,
not only a keyword grep. Require successful world/database initialization and copyover
recovery or ordinary startup with no unresolved release-related errors.

Compare the actual game child's `/proc/<pid>/exe` hash and ELF build ID with the installed
release and manifest. A matching symlink, `--build-info` invocation, healthy old process,
or changed/unchanged PID alone does not establish activation. Confirm supervisor state and
port 4100 ownership, then run:

```bash
./scripts/autorun/autorun.sh status
./scripts/operations/healthcheck.sh --wait
```

Use the host's verified loopback health configuration (default port 8182). Readiness must
report service `luminari-mud`, status `healthy`, and database `healthy`; liveness alone
does not prove MariaDB readiness. Do not reuse Duris website/health URLs.

Verify the staff session survived copyover where applicable and that a fresh login works.
Run `news` and complete its first-page and full-new-section readback. Verify explicitly
included help publication through its engine evidence and actual lookup. Quit and close
test sessions, then observe at least one further minute for restart loops or recurring errors.

On failure, preserve logs and executable identity. Correct operational mistakes within the
authorized scope; do not repeatedly reboot, modify production code, or roll back onto an
incompatible schema. Follow the [incident runbook](../../../docs/runbooks/incident-response.md)
and the recorded recovery procedure. Report a failed or partial deployment honestly.

Report the deployed SHA/build ID, backup location, build/install result, migrations/content
applied, cutover method, runtime/health/login evidence, and remaining gaps. Runtime news is
ignored; this workflow does not force-add it or commit from production. If the user also
requested source commits/pushes, make only the reviewed changes in development, preserve
unrelated work, and verify the push separately from deployment activation.
