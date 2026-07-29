# Local Development Login Quick Guide

**Last verified:** July 29, 2026

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

Each command is followed by a private in-game completion marker, so the next
command is not mistaken for delayed output from the previous one. The final
`PASS` confirms command delivery and clean logout, not that the gameplay output
was correct. Read each command's output and compare it with the relevant test
guide.

For wilderness tests, prefer stable coordinates such as `goto -66 92`.
Dynamic wilderness room VNUMs such as `1000389` may not be allocated after a
reboot and can legitimately fail with `No room exists with that number`.

If a later command depends on an ID printed by an earlier command, run the
smallest useful first batch, capture the ID, and put the remaining commands in
one second batch. This should be the exception, not one login per command.

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
2026 run checked 74 keywords in 67 seconds. This replaces 74 separate login
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
