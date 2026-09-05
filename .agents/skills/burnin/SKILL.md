---
name: burnin
description: >-
  Exhaustively qualify a DurisMUD checkout by stopping its server, running every
  regression and isolated database test, clean-building all maintained code, repairing every
  finding, then booting and smoke-testing with the configured staff character. Use for an
  explicit full burn-in or stability pass, not routine focused validation.
---

# Burn in DurisMUD

Drive the checkout to one uninterrupted clean end-to-end pass after the last fix. Do not stop
merely because one phase passes.

## Boundaries

- Work from the repository root. Read `AGENTS.md`, `README.md`, the root `Makefile`, and the
  starting/stopping section of `docs/operations/RUNBOOK.md`; inspect the worktree first.
- Read only the required `.env` fields without printing their values. Unless the user authorizes
  otherwise, proceed only when `ENVIRONMENT=local`. User authorization overrides this local-only
  rule. Preserve unrelated changes and all player/runtime data.
- Treat migration authority separately from burn-in authority. Before mutating any configured
  runtime database, confirm that the exact target and migration operation are explicitly allowed
  by the user and repository policy. Never relabel, redirect, or weaken a guard to make a live
  production target acceptable to a local-only migration tool.
- Stop the instance under test. Prefer the matching user service or the in-game immortal
  `shutdown`; otherwise identify the supervisor and child by working directory, command line, and
  listener before a graceful signal. Never use broad `kill`/`pkill`. Wait for both processes and
  their ports to close. Treat another checkout owning a required port as a blocker, not as
  authorization to stop it.
- Before stopping an authorized production instance, identify its exact production service unit
  and manager and record the known-good artifact and rollback/start path. After every production
  stop, restart through that same service path, which must invoke `cycle_mud.sh --production`;
  never invoke `start_mud.sh --dev` when `ENVIRONMENT=production`. If a later phase fails, restore
  the known-good artifact and restart the production service before reporting the failure unless
  the user explicitly requested that it remain stopped.
- Do not skip an unavailable gate. Diagnose safe prerequisites; if Docker, credentials, or
  another external dependency remains unavailable, report the burn-in as incomplete.

## Repair loop

1. With the MUD stopped, complete the migration and compatibility gate before any build.
   `make test-db` does not satisfy this step; it only qualifies isolated fixtures.

   For an allowed local/development runtime target, inspect and apply the checked-in immutable
   migration prefix, then prove that the configured runtime schema matches the source contract:

   ```bash
   python3 scripts/migration_runner.py inspect
   python3 scripts/migration_runner.py run
   ./migrations/verify_runtime_compatibility.sh
   ```

   For production, follow `AGENTS.md` and the migration procedure in
   `docs/operations/RUNBOOK.md`. Qualify migrations on the required disposable clone and require
   the approved production schema rollout or cutover to be complete; then run the read-only
   compatibility verifier against production. Do not start a build while the configured runtime
   target is stale, and do not substitute migration qualification on a clone for advancing the
   runtime target.

2. Run every canonical gate and inspect output even when it exits zero:

   ```bash
   ./scripts/format.sh --check
   make test-all
   make test-db
   ```

   `make test-db` must use its isolated Docker/MySQL fixtures, never the configured game database.

3. Fix every error, warning, crash, hang, flaky result, sanitizer-like symptom, or credible defect
   encountered. Find the root cause, keep fixes narrow, add or update a focused regression for
   changed behavior, and do not weaken checks or hide diagnostics. Run the focused check after each
   repair, then repeat the full affected gate.

4. From a stopped state, prove a completely fresh build:

   ```bash
   make clean-all
   make
   ```

   The root build covers all maintained binaries. Review the entire build output for diagnostics,
   not just its status. After any repair, repeat the migration/compatibility gate first, followed by
   the clean build and all canonical gates. Continue until the complete migration, compatibility,
   test, database-test, and clean-build sequence passes without findings.

## Live burn-in

1. Record current log boundaries. For `ENVIRONMENT=local`, start this checkout with
   `./scripts/start_mud.sh --dev`. For an authorized `ENVIRONMENT=production` run, start the exact
   production service identified before shutdown, using the same service manager. Wait for
   `./scripts/healthcheck.sh` to pass. Throughout boot and the smoke test, follow
   `logs/duris-console.log` and every current regular file under `logs/log/`, including files created
   after monitoring begins. Investigate unexpected output as well as obvious warning, error, fatal,
   assertion, crash, and persistence messages.
2. Without exposing secrets, log in through the account menu using `GAME_ACCOUNT_NAME`,
   `GAME_ACCOUNT_PASSWORD`, and `GAME_ACCOUNT_CHARACTER_NAME`. Run a randomized selection of safe,
   non-destructive player and staff inspection commands across several subsystems; record the exact
   commands. Avoid combat, movement, administration, or commands that alter players, world state,
   configuration, or data. Verify sensible responses, a stable connection, a healthy endpoint, and
   a clean logout.
3. Keep monitoring through a short post-logout soak. If anything is wrong, capture evidence, stop
   this exact instance, repair it, and restart the entire repair and live-burn-in loop. Finish only
   after a full clean pass following the final change; leave the healthy MUD running
   unless the user requested otherwise.

Report fixes, exact commands and results, log files and observation interval, staff smoke commands,
final health/runtime state, and any genuine blocker. Never describe a skipped or partial pass as
clean.
