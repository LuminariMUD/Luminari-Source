# Campaign Variant Retirement Live Test Report

Status: pass; findings F-1, F-3, and F-4 resolved and verified

Test date: 2026-08-20

Branch: `codex/retire-campaign-variants`

Original tested commit: `e5bad72450950bed90cc2b59961e329aca7465c4`

Follow-up verification date: 2026-08-20

## Verdict

No blocking regression from retiring the DragonLance and Forgotten Realms variants was found in
the live Luminari game. The current branch built cleanly, booted the development world, accepted a
real account and character login, and remained running after tests across character data, combat,
movement, wilderness resources, crafting, travel, quests, missions, hunts, artifacts, boards,
shops, and item commands.

The live session did expose several cleanup opportunities. The most relevant is that some
DragonLance-named abilities remain registered and visible to the staff test character. These are
not selectable current race content, and they did not cause a runtime failure, but they are a good
example of residual implementation that exact campaign-macro searches cannot find. The account
class, landmark selection, and walk-to logout findings were fixed in the same branch and verified
with production-linked tests and live sessions. The remaining findings are pre-existing residual
content and world-script defects rather than changes introduced by this branch.

## Environment and build identity

- The checkout was confirmed as a development environment through `lib/.env`. No credential file
  was modified or printed.
- The server was built with `make clean`, `make -j$(nproc)`, and `make install`.
- The installed and running executable both report Git commit
  `e5bad72450950bed90cc2b59961e329aca7465c4` with a clean source state.
- ELF build ID: `95d86409e01113d36c8d10558454d7c253b0396f`.
- Installed binary SHA-256:
  `4cb572a3ed2c8af341dea7f59826088f25d703e9bf9305b3450c0ec7e94c4259`.
- The follow-up fixes were exercised from installed working-tree build ID
  `045c060c70e5e1a4cf59825997629818cf658f8d`, SHA-256
  `093390ec45eda271a9c2f0071b766fa4dd2a98560118682f8c7df8c787bdf2e9`.
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
| Class data | `classes`, `class info warrior`, `class feats warrior`, `accexp class` | PASS; F-1 resolved |
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
| Landmarks | `landmarks`, `landmarks Ashenport`, `landmarks city`, and `landmarks 1030` | PASS; F-3 resolved |
| Automatic walking | `walkto jade jug inn`, automatic movement, cancellation, and logout during an active route | PASS; F-4 resolved |
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

### F-1 [RESOLVED]: `accexp class` offered placeholder classes

Severity: low

`accexp class` lists `placeholder 1` and `placeholder 2`, each at zero account experience. Both
classes are defined as locked and not in game, but `do_accexp` filters only on the lock and account
unlock state; it does not filter `CLSLIST_INGAME(i)`.

This behavior is present on `origin/master`: the same placeholder definitions and listing filter
precede the campaign-retirement branch. It is therefore not a regression from variant removal.
The account experience class listing and purchase lookup now both exclude classes whose `in_game`
field is false. A production-linked regression test confirms an active locked class remains
available while both placeholder classes are absent.

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

### F-3 [RESOLVED]: `landmarks` region guidance was incomplete

Severity: low usability defect

`landmarks` says to specify a region but does not print valid region keys. `landmarks Ondius` and
`landmarks city` found no entries from the tested locations, while `landmarks 1030` correctly
listed 19 Ashenport landmarks. The active landmark table uses numeric zone keys, not the continent
name presented by nearby travel commands.

The command now lists each available landmark area with its numeric zone, accepts either an area
name or exact zone number, and routes `landmarks city` to the current-area view. Live checks from
Ashenport confirmed the area list, `Ashenport`, `city`, and `1030` forms all expose the expected 19
landmarks. A partial numeric key such as `103` is deliberately rejected instead of matching 1030.

### F-4 [RESOLVED]: Quitting during `walkto` logged an invalid route lookup

Severity: low runtime defect

Automatic walking itself worked: Kohdee moved room by room from Ashenport's north gate toward the
Jade Jug Inn and could cancel the route after reconnecting. When an earlier test logged out while
the route was active, the server recorded:

```text
SYSERR: Illegal value -1 or 17883 passed to find_first_step. (src/graph.c)
```

The walk scheduler now clears an active route before pathfinding whenever its character is no
longer playing or has no room. It also clears a route whose destination has disappeared and uses
the correct landmark-table row when reporting an interrupted route. A production-linked test
covers the logout state directly. The live logout replay completed without an illegal
`find_first_step()` value, server error, crash, assertion, or abort.

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

The follow-up logout-during-walk replay produced no route lookup error. Besides finding F-5, the
log contained:

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
3. Guard delayed room-trigger messages when their original actor has left.

## Follow-up verification

- `make -j$(nproc)` completed as a warning-free GNU C23 production build.
- `make test` passed all 781 production-linked CuTests and the repository regression scripts.
- `make install` installed the tested executable and removed the root-level `luminari` artifact.
- Live `accexp class` output retained the available locked classes without either placeholder.
- Live Ashenport landmark checks passed for the area list, area name, current city, and exact zone
  number forms.
- Logging out during an active route canceled the route before pathfinding; the server remained
  healthy and its follow-up log contained no route lookup error.
