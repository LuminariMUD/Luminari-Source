# Campaign Variant Retirement Live Test Report

Status: pass with non-blocking follow-up findings

Test date: 2026-08-20

Branch: `codex/retire-campaign-variants`

Tested commit: `e5bad72450950bed90cc2b59961e329aca7465c4`

Related scope: [CAMPAIGN_VARIANT_RETIREMENT_SCOPE.md](CAMPAIGN_VARIANT_RETIREMENT_SCOPE.md)

## Verdict

No blocking regression from retiring the DragonLance and Forgotten Realms variants was found in
the live Luminari game. The current branch built cleanly, booted the development world, accepted a
real account and character login, and remained running after tests across character data, combat,
movement, wilderness resources, crafting, travel, quests, missions, hunts, artifacts, boards,
shops, and item commands.

The live session did expose several cleanup opportunities. The most relevant is that some
DragonLance-named abilities remain registered and visible to the staff test character. These are
not selectable current race content, and they did not cause a runtime failure, but they are a good
example of residual implementation that exact campaign-macro searches cannot find. Other findings
are pre-existing command, travel, and world-script defects rather than changes introduced by this
branch.

## Environment and build identity

- The checkout was confirmed as a development environment through `lib/.env`. No credential file
  was modified or printed.
- The server was built with `make clean`, `make -j$(nproc)`, and `make install`.
- The installed and running executable both report Git commit
  `e5bad72450950bed90cc2b59961e329aca7465c4` with a clean source state.
- ELF build ID: `95d86409e01113d36c8d10558454d7c253b0396f`.
- Installed binary SHA-256:
  `4cb572a3ed2c8af341dea7f59826088f25d703e9bf9305b3450c0ec7e94c4259`.
- The MUD was started locally through `autorun.sh`, reached the game loop on port 4100, and was
  still running after the final verification.
- Tests used the existing development account and the `Kohdee` character through
  `scripts/development/dev_kohdee_login_smoke.sh`.

## Live regression matrix

| Area | Commands and behavior exercised | Result |
|------|---------------------------------|--------|
| Account and login | Repeated account authentication, character selection, world entry, logout, and reconnect | PASS |
| Character identity | `whoami`, `who`, `score`, `equipment`, `inventory`, `rpsheet` | PASS |
| Race data | `races`, `race info human`, `accexp race` | PASS |
| Class data | `classes`, `class info warrior`, `class feats warrior`, `accexp class` | PASS with finding F-1 |
| Deities | `devote list all`, `devote info Aethyra` | PASS |
| Feats and abilities | `feats info power attack`, `abilities`, `skills` | PASS with finding F-2 |
| Spells | `spells wizard 0`, `spelllist wizard 0`, `memorize` | LIMITED; registry passed, prepared casting was not available to this warrior |
| World and status data | `areas 1`, `happyhour`, ordinary room look and exit display | PASS |
| Room movement | North and south between staff rooms | PASS |
| Wilderness movement | `goto -390 -267`, east, west, coordinate and room updates | PASS |
| Wilderness survey | `survey`, `survey resources`, `survey terrain`, `survey conservation` at two coordinate sets | PASS |
| Resource gathering | `gather herbs`, material award, depletion display, and material storage cleanup | PASS |
| Crafting | `materials`, `craftscore`, `craft`, `crafting`, plus invalid-context `gather`, `mine`, and `harvest` paths | PASS |
| Carriage routes | `carriage` outside a stop and at Ashenport room 103000 | PASS |
| Sailing routes | `sail` outside a port and at Ashenport room 34801 | PASS |
| Flight routes | `flightlist` with 34 active Luminari destinations | PASS |
| Landmarks | `landmarks`, `landmarks city`, `landmarks Ondius`, and working `landmarks 1030` output | PASS with finding F-3 |
| Automatic walking | `walkto jade jug inn`, automatic multi-room movement, reconnect persistence, and `walkto cancel` | PASS with finding F-4 |
| Quests | `quest`, `quest list` outside a questmaster | PASS for rejection and usage paths |
| Missions | `mission` and faction representative handling in room 103009 | PASS for listing and validation paths |
| Hunts | `hunts` at the Ashenport Huntsmaster in room 103492 | PASS |
| Artifacts | `artifact`, `artifact roster`, `artifact list`, `artifact progress`, `artifact abilities` | PASS for registry and empty-inventory paths |
| Combat display | `damage hit`, `attacks` | PASS |
| Live combat | Loaded mobile 103679, attacked and killed it, observed combat procs, reward, extraction, and continued server health | PASS |
| Boards | `look board` and `read` usage on the staff board | PASS |
| Shops | `list` at Essimuth's Equipment in room 103022 | PASS |
| Item commands | `drop`, `junk`, and `put` usage paths | PASS |
| Communication | `say campaign retirement live test` | PASS |

## Findings

### F-1: `accexp class` offers placeholder classes

Severity: low

`accexp class` lists `placeholder 1` and `placeholder 2`, each at zero account experience. Both
classes are defined as locked and not in game, but `do_accexp` filters only on the lock and account
unlock state; it does not filter `CLSLIST_INGAME(i)`.

This behavior is present on `origin/master`: the same placeholder definitions and listing filter
precede the campaign-retirement branch. It is therefore not a regression from variant removal.
The command should eventually exclude classes whose `in_game` field is false.

### F-2: Retired-campaign ability content remains registered

Severity: low for this release, broader cleanup recommended

The staff character's broad `skills` and ability views still expose entries such as Baaz, Kapak,
and Bozak Draconian death throes and Kapak Draconian poison. Their feat, spell, event, and combat
registrations remain in files including:

- `src/character/feats.c`
- `src/magic/spell_parser.c`
- `src/combat/spec_abilities.c`
- `src/mud_event_list.c`

No current selectable race exercised these mechanics, and no failure occurred. Some old
knighthood-named abilities are used by renamed active Luminari knight classes, so names alone are
not sufficient grounds for deletion. The Draconian registrations, however, are concrete residual
code that deserves a separate reachability and persisted-identifier review. This is outside the
compiler-led macro retirement completed by the current branch.

### F-3: `landmarks` region guidance is incomplete

Severity: low usability defect

`landmarks` says to specify a region but does not print valid region keys. `landmarks Ondius` and
`landmarks city` found no entries from the tested locations, while `landmarks 1030` correctly
listed 19 Ashenport landmarks. The active landmark table uses numeric zone keys, not the continent
name presented by nearby travel commands.

The same Luminari behavior exists on `origin/master`; the branch removed only alternate-campaign
region lists. This is not a campaign-retirement regression.

### F-4: Quitting during `walkto` logs an invalid route lookup

Severity: low runtime defect

Automatic walking itself worked: Kohdee moved room by room from Ashenport's north gate toward the
Jade Jug Inn and could cancel the route after reconnecting. When an earlier test logged out while
the route was active, the server recorded:

```text
SYSERR: Illegal value -1 or 17883 passed to find_first_step. (src/graph.c)
```

The walk scheduler called `find_first_step()` after the character's room had become `NOWHERE`.
The relevant validation and `process_walkto_actions()` lifecycle are unchanged from `master`; the
campaign diff in `graph.c` only removes an alternate-campaign cross-zone conditional after this
validation. This is reproducible test evidence, but not a regression caused by the branch.

### F-5: Ashenport welcome trigger messages can outlive their actor

Severity: low world-script defect

Room trigger 103003 schedules delayed welcome messages after a player enters room 103000. Quick
test logouts caused several `no target found for wsend` script errors when those delayed messages
ran after the actor disconnected. The trigger is existing world data and was not changed by this
branch.

### F-6: `do_homelands` is an unregistered handler

Severity: informational dead-code finding

`do_homelands` remains implemented in `src/act.informative.c`, but it has no command-table entry in
`src/interpreter.c`. Entering `homelands` therefore returns the normal unknown-command response.
This was already true on `origin/master`. It is a straightforward example of dead command code:
compiled implementation with no player-facing route to invoke it.

## Runtime log review

The server did not crash, restart, abort, or emit an assertion failure during the test. The active
process remained the installed binary after all sessions.

Besides findings F-4 and F-5, the log contained:

- local I3 connection failures to `127.0.0.1:8081`;
- a missing local Ollama model and failed Ollama warmup;
- an existing zone-script route failure and loop warning for mobile 2062800; and
- performance monitor warnings around command/logout processing.

The repository instructions explicitly say I3 and Ollama are not expected to work in local
development unless they are the test target. None of these messages correlated with a
campaign-retirement failure or server loss.

## Test footprint and restoration

The test deliberately performed a small amount of mutable gameplay so that it covered more than
display commands:

- Gathering awarded seven units of legendary wolfsbane, then
  `materialadmin remove Kohdee all` cleared the test material storage.
- The herb resource at coordinate `(-390, -267)` was reduced to 75 percent availability. It is a
  regenerating development-world resource node and is the only remaining world footprint noted by
  this test.
- Combat with loaded mobile 103679 awarded 43 gold. Kohdee was restored, alignment reset to 0,
  and gold reset to 89991.
- The loaded mobile was killed and extracted; no test corpse remained.
- Kohdee ended in staff room 1204 with full 632 hit points and 1160 movement points, empty visible
  inventory, empty wilderness material storage, no active automatic walk, page length 40, and
  screen width 80.

## Coverage limits

The following paths were not forced because the existing test character lacked the required state
or because the action would create avoidable persistent data:

- actual spell preparation and casting by a prepared caster;
- new character creation and newbie equipment assignment;
- artifact activation with an artifact in inventory;
- accepting or completing a quest or faction mission; and
- completing a timed carriage or sailing trip.

Their registries, menus, route resolution, validation, or empty-state behavior were exercised
where possible. These limits do not change the live-test verdict, but they should remain visible
if later work changes one of those systems directly.

## Recommended follow-up

1. Merge-block only if review finds an active Luminari path tied to the residual Draconian
   registrations. The live test found no such path.
2. Open a separate dead-code and persisted-identifier audit for retired lore mechanics, beginning
   with the Draconian feat, spell, event, and combat registrations and the unregistered
   `do_homelands` handler.
3. Fix `do_accexp` to omit classes for which `CLSLIST_INGAME(i)` is false.
4. Make `landmarks` print valid numeric zone keys or accept the region names presented to players.
5. Cancel or ignore `walkto` processing when a descriptor's character is no longer in a room.
6. Guard delayed room-trigger messages when their original actor has left.
