# Local Development Login Quick Guide

**Last verified:** July 30, 2026

Use this smoke test to boot the development MUD, authenticate with the game
master account, enter the level-34 character `Kohdee`, and leave both the
character and account cleanly.

`Kohdee` is the character name. Do not search titles or look for the word
`forger`. Agents should run the fast path before inspecting the database,
process list, or menu layout.

## Fast Path

From the repository root, run:

```bash
./scripts/dev_kohdee_login_smoke.sh
```

That single command:

- Refuses to run unless `APP_ENV=development`.
- Reads credentials from `lib/.env` without displaying them.
- Reuses the local MUD if it is already listening, or starts `bin/circle` and
  waits for the game loop.
- Finds `Kohdee` by the exact account-menu Name column rather than a menu
  number.
- Enters the world, runs `quit`, selects `0`, and selects `Q`.
- Leaves a newly started MUD running as the user service
  `luminari-dev-login-smoke.service` and prints its PID/log.

Expected completion time is about 3-5 seconds with a running server, or only the
15-20 second boot time plus the login when the server is stopped. Success ends
with:

```text
PASS: Kohdee entered the world, left the character, and logged out of the account (Ns).
```

## Fast Shared Vessel Harbor

After `make install`, provision and verify the complete reusable harbor with:

```bash
./scripts/provision_vessel_harbor.sh
```

This development-only command reuses the configured master account and Kohdee;
it does not create another account or character. It installs only missing
harbor world records, seeds the database fixture, hard-restarts the supervised
local MUD, and verifies the two docks, public ferry, NPC pilot, hourly route,
and generated bridge/cargo triggers in one batched login. The first run may
need about one minute because it creates the ferry and proves a second restart.
An already provisioned harbor takes about 30 seconds.

## Durable Vessel Ferry Soak

Start the release-gate ferry run as a user service:

```bash
./scripts/run_vessel_ferry_soak.sh start
./scripts/run_vessel_ferry_soak.sh status
```

The default is 24 continuous hours with database/process checks every 60
seconds and an actual Kohdee inspection every hour. The monitor refuses
non-development environments, discovers the ferry and route IDs instead of
assuming slot 5, repairs the ferry once before starting, and holds a
non-character connection in unconfirmed account-name state so the game loop
does not sleep between inspections. The generated hold name is never
confirmed, so no account is created. The monitor requires that socket to
remain `ESTABLISHED` every 20 seconds and fails if the server reports that it
went to sleep. Live checks use the configured master account and existing
Kohdee character; they do not create an account or character.

At the end, the monitor uses Kohdee to pause the ferry, verifies that the exact
coordinates and route were committed, hard-restarts the local service, checks
the recovered state, and resumes the ferry. Results and every sample remain
under the run directory printed by `start`. Only `PASS` from `status` closes
the gate.

For a short monitor shakedown:

```bash
./scripts/run_vessel_ferry_soak.sh start 150 10 60
```

The three values are duration, database/process interval, and live-character
interval in seconds. Keep the shakedown longer than the server's login timeout
so it proves the hold connection rather than relying on periodic character
logins to wake a sleeping loop.

## Fast Vessel Builder Gate

Use one logged-in Kohdee session to exercise the complete no-C builder path:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-builder-check
```

The helper reads `vedit` usage, moves to the shared harbor, creates a uniquely
named Boat prototype, tunes and shows it, spawns it with `vedit`, discovers the
returned prototype ID and fleet slot, and verifies that the generated hull
sails from `(-66, 92)` to `(-67, 92)`. It then stops and purges the temporary
hull, deletes the temporary prototype, and confirms the prototype is gone.
Creation, editing, and spawning use only the builder-facing `vedit` command;
staff commands are used afterward for sailing verification and cleanup.

Do not split this workflow across logins or query MySQL for generated IDs. The
helper derives both IDs from actual game output in the same connection. The
July 30, 2026 local run completed the in-game workflow in 2.7 seconds and the
entire login, test, cleanup, character logout, and account logout in 8 seconds.
Success ends with:

```text
PASS: builder created, tuned, spawned, and sailed a vessel in N.N seconds.
PASS: temporary ship N and prototype N were removed.
PASS: Kohdee completed the vessel builder check and logged out cleanly (Ns total).
```

## Fast Command Batches

Do not repeat the login flow for every test command. Pass a whole sequence as
single-line arguments to `--commands`:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands \
  "goto -66 92" \
  "board gull" \
  "south" \
  "speed 5" \
  "shipstatus"
```

The helper logs in once, runs each command as `Kohdee`, prints the output under
`>>> command` headings, then leaves the character and account cleanly. A
one-command batch takes about 4-6 seconds against a running server; an
18-command vessel batch takes about 20-25 seconds.

Each command is followed by a unique in-game completion marker, so the next
command is not mistaken for delayed output from the previous one. The marker
uses ordinary local speech so it also works for level-1 test characters; run
this helper only on development. The final `PASS` confirms command delivery and
clean logout, not that the gameplay output was correct. Read each command's
output and compare it with the relevant test guide.

For wilderness tests, prefer stable coordinates such as `goto -66 92`.
Rooms 1000389 and 1000390 are static harbor fixtures after provisioning.
Other runtime wilderness VNUMs are pool allocations and can legitimately
change or disappear after reboot.

If a later command depends on an ID printed by an earlier command, run the
smallest useful first batch, capture the ID, and put the remaining commands in
one second batch. This should be the exception, not one login per command.

`@wait N` is a helper-side pause of 1-60 seconds and is not sent to the game.
Use it only to synchronize two local character sessions or wait for a real
heartbeat/reload interval:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands \
  "@wait 2" \
  "trans Testcaptain"
```

## Alternate Local Test Characters

The same fast path can enter another character by exact name. This is intended
for disposable lifecycle and multiplayer fixtures, not production accounts:

```bash
./scripts/dev_create_test_character.sh localtestaccount Testcaptain

DEV_MUD_ACCOUNT=localtestaccount \
DEV_MUD_CHARACTER=Testcaptain \
./scripts/dev_kohdee_login_smoke.sh --commands \
  "score" \
  "look"
```

The creation helper refuses non-development environments, boots or reuses the
local MUD through the established Kohdee preflight, creates one reusable test
account with its first default human warrior, enters the world once, and logs
out cleanly. It refuses to replace an existing account.

An account can contain multiple characters. Do not create one account per
character for ordinary multiplayer testing. Add later fixtures to the same
test account through its account-menu `C` option, then use
`DEV_MUD_CHARACTER` to select each exact Name. Separate accounts are warranted
only for account-isolation or destructive account/deletion tests. The creation
helper currently bootstraps a new account only; the login helper works with
any number of characters already present on that account and never assumes a
stable menu slot.

The password comes from `DEV_MUD_ACCOUNT_PASSWORD` when set; otherwise the
helper uses `GAME_MASTER_ACCOUNT_PASSWORD` from `lib/.env`. This allows local
test accounts deliberately created with the development master password to be
used without putting that password in shell arguments or logs. The default
with no overrides remains the exact level-34 `Kohdee` path.

The helper validates the alternate name, finds exactly one matching Name
column, uses non-staff completion markers, and performs the same clean
character/account logout. It also reports a soft-deleted selection immediately
instead of waiting for the normal login timeout. Keep one entire command
sequence in one invocation.

For menu-driven editors, use one `--dialog` invocation. Each argument is one
input line; the helper confirms that the final input returned to normal command
mode before it logs out:

```bash
./scripts/dev_kohdee_login_smoke.sh --dialog \
  "cedit" "e" "l" "1" "q" "q" "y"
```

This is intended for deterministic menu sequences such as `cedit`. End the
sequence by saving or cancelling out of the editor. If it is still inside an
editor, the helper fails instead of silently reporting success.

For the vessel option, choice `1` is `Off` and choice `2` is `On`. A kill-switch
test must use a second dialog with `2` before cleanup; confirm
`lib/etc/config` ends with `vessel_system = 1`. Do not leave development
disabled for the next agent.

## Fast Copyover Check

Use `--copyover-check` instead of logging in manually, issuing `copyover`, and
reconnecting. The helper keeps Kohdee's descriptor open across the process
replacement, requires the server's recovery confirmation, runs every supplied
command after recovery, and then logs out cleanly:

```bash
./scripts/dev_kohdee_login_smoke.sh --copyover-check \
  "shiplist" \
  "shipgoto 2" \
  "shipstatus"
```

Set up any required pre-copyover state in one earlier `--commands` batch. Keep
all post-copyover verification in the single command above. A copyover includes
the normal development-data boot time, but does not require a second account
login or a hardcoded character slot.

When timing matters, place pre-copyover commands before a standalone `--`. The
helper runs them, starts copyover immediately on the same connection, and runs
the remaining commands only after recovery:

```bash
./scripts/dev_kohdee_login_smoke.sh --copyover-check \
  "shipgoto 3" \
  "autopilot on" \
  -- \
  "autopilot status" \
  "shipstatus"
```

## Pager-Safe Help Checks

Do not use `--commands "help ..."` for a long help entry: the MUD pager can
consume the private completion marker as pager input. Use `--help-check`
instead:

```bash
./scripts/dev_kohdee_login_smoke.sh --help-check \
  delroute vesseldebug loadvehicle
```

This mode exits paged entries safely, requires a database `Help Tag` for every
keyword, prints one compact result per keyword, and fails on missing or
file-fallback help.

For the exhaustive vessel release check, use the single-command form:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-help-check
```

It derives every command carrying `CMD_FEATURE_VESSEL` directly from
`src/interpreter.c`, adds the intentionally ungated boarding and staff recovery
commands, and verifies the resulting set in one Kohdee login. The July 29,
2026 run checked 75 keywords in 54 seconds. This replaces 75 separate login
cycles and automatically includes newly gated commands.

Use the remainder of this document only to diagnose a failed fast-path run or
to perform the process manually.

## Fast-Path Preconditions

- Run from the repository root.
- `lib/.env` must say `APP_ENV=development`.
- `GAME_MASTER_ACCOUNT` and `GAME_MASTER_ACCOUNT_PASSWORD` must be set in
  `lib/.env`. Never print, log, or copy their values into a command or document.
- MariaDB must be active.
- `bin/circle` must exist and be executable. Build and install first if it does
  not.
- `expect`, `nc`, `ss`, `systemctl`, and `systemd-run` must be installed.
- The helper reads `DFLT_PORT` from `lib/etc/config`; it is currently 4100.

The helper checks all of these automatically.

## Manual Fallback

Start the server:

```bash
./bin/circle -d lib
```

Wait for `Entering game loop.`, then connect from a second terminal:

```bash
nc 127.0.0.1 4100
```

Then follow this sequence:

1. At the account-name prompt, enter the value of `GAME_MASTER_ACCOUNT`.
2. At `Password:`, enter `GAME_MASTER_ACCOUNT_PASSWORD`.
3. At the account menu, choose the numbered row whose Name column is exactly
   `Kohdee`.
4. At `PRESS RETURN`, press Enter.
5. At the character menu, enter `1` to enter the game.
6. Confirm that the welcome message and room display appear.
7. Enter `quit` to leave the game world for the character menu.
8. At the character menu, enter `0` to return to the account menu.
9. At the account menu, enter `Q`.
10. Confirm `Quitting.` and close the local client if it does not exit by
    itself.

The important state path is:

```text
account menu -> Kohdee row -> character menu -> 1 -> game world
game world -> quit -> character menu -> 0 -> account menu -> Q
```

Do not hardcode the account-menu character number. `load_account_characters()`
currently loads `player_data` without an `ORDER BY`, so slot order is not a
stable contract. Locate `Kohdee` in the Name column on every run.

## Troubleshooting Notes

- Boot takes roughly 15-20 seconds on this development data set.
- Existing world-data warnings, a missing local Ollama model, and an
  unavailable local I3 gateway did not block the verified run.
- The helper suppresses terminal output so neither credential can appear in
  logs.
- It strips ANSI color before matching the exact `Kohdee` Name column.
- It accepts both the configured and compiled-fallback welcome messages.
- It explicitly closes `nc` after `Quitting.` because this local `nc` can keep
  waiting after the server removes the account connection.

Useful server-side confirmation is:

```text
<character> has connected.
<character> ... entering game.
<character> has quit the game.
Losing player: <null>.
No connections.  Going to sleep.
```

If started manually, stop the foreground server with `Ctrl-C`. If the helper
started it, stop the supervised development server with:

```bash
systemctl --user stop luminari-dev-login-smoke.service
```
