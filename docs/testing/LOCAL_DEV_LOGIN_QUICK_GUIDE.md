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
10-gold passenger fare, territorial/free-sea/pirate-cove wilderness regions,
and generated bridge/cargo triggers. After restart it confirms that `seastate`
resolves the ferry's canonical legal waters, boards through the ordinary
hull-object path as Kohdee, proves exactly one fare was deducted, restores
Kohdee's original gold, resumes the ferry, and waits up to 45 seconds for a
real territorial/free-sea boundary announcement. It immediately correlates
that announcement with `seastate`, then returns Kohdee to room 1000389. The
provisioner discovers and passes the ferry slot automatically; do not spend
time looking it up or create a second account or disposable character. The
first run may need about one minute because it creates the ferry and proves a
second restart. Before the fare and crossing checks were added, an already
provisioned harbor took about 30 seconds; remeasure the augmented path after
the active ferry soak releases the installed build.

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
The launch metadata records the source commit and SHA-256 of `bin/circle`.
Each process sample rejects a changed executable fingerprint, and the final
restart must launch the same SHA-256 that served the continuous window.

At the end, the monitor uses Kohdee to pause the ferry, verifies that the exact
coordinates and route were committed, hard-restarts the local service, checks
the recovered state and executable hash, and resumes the ferry. Results and
every sample remain under the run directory printed by `start`. Only `PASS`
from `status` closes the gate.

For a short monitor shakedown:

```bash
./scripts/run_vessel_ferry_soak.sh start 150 10 60
```

The three values are duration, database/process interval, and live-character
interval in seconds. Keep the shakedown longer than the server's login timeout
so it proves the hold connection rather than relying on periodic character
logins to wake a sleeping loop. The corrected July 30, 2026 shakedown ran 150
continuous seconds with 84 movement steps, 22 distinct cells, both docks, 5
Kohdee samples, 16 database/process samples, an unchanged travel PID, exact
state recovery across the final restart, and automatic resume.
A separate provenance shakedown proved the same executable SHA-256 before and
after restart. Deliberate SIGTERM then produced an immediate terminal `FAIL`
artifact, so an interrupted agent or service does not leave a false `RUNNING`
result.

## Reproducible 500-Vessel Scale Gate

After the definitive ferry soak passes and the current clean source is built
and installed, launch the development-only scale gate with:

```bash
./scripts/run_vessel_scale_benchmark.sh start
./scripts/run_vessel_scale_benchmark.sh status
```

The default steady measurement window is 660 seconds. An explicit value from
600 through 7200 seconds may follow `start`. The command returns immediately
after launching a supervised user service; use `status` for preparation,
measurement, result, and restoration progress.

The runner reuses the configured master account and exact `Kohdee` character
for every in-game phase. It does not create an account or character, and it
does not log in one character per vessel. One Kohdee session creates all
missing public hulls with `vedit spawnpublic`; later sessions on that same
account verify the reconstructed workload, warm it, collect `perfmon csv`, and
leave Kohdee in room 1000389.

The runner refuses production, a dirty source worktree, an active ferry soak,
an older installed binary, or stale benchmark data. Before mutation it takes
an atomic snapshot of every vessel/economy table it can change. It then fills
active slots 1-500 across all eight vessel classes, configures routes, pilots,
crew, schedules, cargo, weapons, encounters, wear, and economy state, and
holds Kohdee aboard an airship so the normal MSDP path runs. Before timing,
Kohdee proves that a surface hull cannot leave Z 0, an airship cannot exceed
Z 500, and a submarine cannot cross above the waterline. The air route then
changes altitude between Z 120 and Z 220. Minute-by-minute `shipstatus`
samples must contain at least two distinct Z values inside the airship's
class bounds.

The 100-percent benchmark encounter is attached to the air route's real
`REGION_ENCOUNTER`. Co-located airships share their exterior wilderness room;
the run requires a live encounter that notifies multiple hulls once and
requires Kohdee to receive its arrival message. Ten scheduled ships are also
staged to depart inside the measured window. `shiplist summary` proves the
live fleet count without overflowing the MUD's socket output buffer. A paused
reciprocal submarine pair continuously fires two synchronized, one-damage
weapons; its fixture defense speed is above the NPC attack ceiling, so no shot
can land, and its negative Z excludes surface weather. This makes message
suppression deterministic after the profiler reset even during the longest
supported measurement. Kohdee's `perfmon csv` transcript must report a
nonzero `vessel_messages_throttled` count. The runner discovers that fixture
slot, invokes `--vessel-message-check` automatically, and preserves Kohdee's
observed return-fire traffic plus counter in
`vessel-message-throttling.log`. Do not guess the slot or add a separate
manual login.

Whether the result passes, fails, or receives SIGTERM, the worker stops the
local MUD, restores the snapshot, verifies that benchmark marker rows are
gone, restarts development, and returns Kohdee to the static room. If an
interrupted run needs operator recovery, use:

```bash
./scripts/run_vessel_scale_benchmark.sh cleanup
```

Do not start this gate while the 24-hour ferry run is active. The scale runner
intentionally refuses to disturb that pinned process and executable.

## Fast 1,000-Trade Economy Gate

After the current source is installed on local development, reuse the master
account and exact Kohdee character for the sustained-market proof:

```bash
./scripts/dev_kohdee_login_smoke.sh --commands "vtradecheck 1000"
```

This staff diagnostic runs the production batch-pricing and supply functions
without changing Kohdee's gold, cargo, or the live port tables. It executes
1,000 deliberately oversized transfers that alternate direction, checks the
10-400 inventory bounds, follows a legitimate profitable route until its
price gap closes, and restocks both simulated ports to 100. The reversal
profit must be negative; a positive value reproduces the old unlimited
bulk-quote exploit. Success includes:

```text
Vessel economy simulation: PASS
  Trades executed: 1000/1000
  Supply range: 10..400 (hard bounds 10..400)
  Adversarial reversal profit: -... gold (must be <= 0)
  Restock convergence: 100/100 (baseline 100)
```

The 500-vessel runner performs and records this same Kohdee command
automatically. Do not run either path while the pinned ferry soak is active.

## Fast Ship-Wide Channel Gate

Run this only after the active soak releases the installed build:

```bash
./scripts/dev_kohdee_login_smoke.sh \
  --vessel-channel-check <ship-slot>
```

The helper opens two simultaneous local connections, authenticates both with
the configured master account, selects `Kohdee`, and automatically selects the
first other usable character Name from that same account. It ignores deleted
rows and never assumes a stable menu slot. To require a particular existing
character instead, append its exact Name after the ship slot. It keeps both
descriptors open while it:

1. Moves Kohdee to the requested vessel and transfers the second character
   aboard.
2. Moves Kohdee into a different interior room.
3. Sends a unique `shiptalk` marker from each character and requires the other
   socket to receive the vessel name, speaker name, and exact marker.
4. Moves Kohdee ashore, sends another aboard marker, and requires Kohdee's
   socket to remain quiet.
5. Requires the aboard-only refusal from both characters after bringing the
   second character ashore.
6. Cleanly leaves both characters and both account sessions.

One helper process owns both sockets and the normal login lock, so no manual
character lookup, client synchronization, or second helper process is needed.
This is the preferred release transcript.

If the master account has no second usable character, add one to that same
account first:

```bash
./scripts/dev_create_test_character.sh Vesselmate
```

The one-argument creation form uses the master account. Do not create a second
account for this check. A read-only July 30 database check found that the
current local master account contains only Kohdee, so run this once after the
active soak and before the channel gate. The gate will then discover
`Vesselmate` automatically; do not pass or look up its account-menu slot.

## Fast Native MSDP Vessel-State Gate

After the current source is installed on local development and a vessel slot
exists, run:

```bash
./scripts/dev_kohdee_login_smoke.sh --vessel-msdp-check <ship-slot>
```

The helper reuses the master account and exact Kohdee character. It enables
native MSDP on the socket, teleports Kohdee aboard the requested vessel,
requests all nine `SHIP_*` variables, and compares the position and navigation
frames with `shipstatus`. It also validates the hull totals and status band.
Kohdee then goes to static room 1000389, and the helper requires the two string
variables to clear and all seven numeric variables to become zero before it
logs out of the character and account.

The 500-vessel runner performs this check automatically against its benchmark
airship and preserves `native-msdp-vessel-state.log`. This is a real
Telnet-option-69 exchange; merely leaving Kohdee aboard while the server calls
the setters does not prove client delivery. Do not run this path while the
pinned ferry soak is active.

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
While paused, the helper continuously drains server output so a long
character observation cannot fill the client pipe or MUD socket buffer. Use
it only to synchronize two local character sessions or wait for a real
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
./scripts/dev_create_test_character.sh Testcaptain

DEV_MUD_CHARACTER=Testcaptain \
./scripts/dev_kohdee_login_smoke.sh --commands \
  "score" \
  "look"
```

The creation helper refuses non-development environments, boots or reuses the
local MUD through the established Kohdee preflight, logs into the configured
master account, creates the default human warrior through that account's `C`
option, enters the world once, and logs out cleanly.

An account can contain multiple characters. Do not create one account per
character for ordinary multiplayer testing. Keep fixtures on the master
account and use `DEV_MUD_CHARACTER` to select each exact Name. The login helper
never assumes a stable menu slot.

Only account-isolation or destructive account/deletion tests warrant another
account. For those cases, pass the account explicitly:

```bash
./scripts/dev_create_test_character.sh localtestaccount Testcaptain
```

The same helper logs into that account if it exists or bootstraps it if it
does not.

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
commands, and verifies the resulting set in one Kohdee login. The current
source derives 77 keywords. The July 29, 2026 installed-build run checked the
then-current 75 in 54 seconds. This replaces one login cycle per keyword and
automatically includes newly gated commands.

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
