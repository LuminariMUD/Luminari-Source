# vessels-ships.md improvement pass based on duris code

The goal is to improve the vessel/ship system implementation BASED on DurisMUD system.

This is the entry point: where the work stands, how a step is worked, the rules and decisions that
still bind, the active step's record, and what remains. The
companion [vessels-ships-history.md](vessels-ships-history.md) keeps everything else unabridged:
where the code and documents are, the 2026-09-28 study (Parts 0-4: DurisMUD and LuminariMUD at the
baseline, the feature matrix, the combat parity checklist, the design values already built, and
the defects), each merged step's progress record, and the original estimate. Both documents use
the study's section numbers, which the code, the SQL, and `VESSEL_SYSTEM.md` cite as "study
3.3.x". When a step merges, its progress section moves to the history document and its status row
here records the merge.

## Status

| Step | State | Record |
| -- | -- | -- |
| S1 Defect fixes | Merged `34dcbb34e` (MR !6) | [Phase 1](vessels-ships-history.md#phase-1-s1-progress) |
| S2 Movement and pacing | Merged `89cfabcbe` (MR !7) | [Phase 2](vessels-ships-history.md#phase-2-s2-progress) |
| S3 Damage model | Merged `a85e97d9f` (MR !8) | [Phase 3](vessels-ships-history.md#phase-3-s3-progress) |
| S4 Weapons and gunnery | Merged `c8bab4576` (MR !9) | [Phase 4](vessels-ships-history.md#phase-4-s4-progress) |
| S5 Crew, repair and loss | Merged `23a0726e4` (MR !10) | [Phase 5](vessels-ships-history.md#phase-5-s5-progress) |
| S6 NPC raiders and AI | Merged `85914a03d` (MR !11) | [Phase 6](vessels-ships-history.md#phase-6-s6-progress) |
| S7 Rewards and economy | Merged `cce9ff323` (MR !12) | [Phase 7](vessels-ships-history.md#phase-7-s7-progress) |
| S8 Client data | Merged `2a4815b1a` (MR !13) | [Phase 8](vessels-ships-history.md#phase-8-s8-progress) |
| S-immediate Luminari Web for S9 | In review: MR !14, tag `vessels-s-immediate` (base `vessels-s9-base` = `1e0f2f64c`) | [S-immediate](#s-immediate-progress) |
| S9 Claude Code play tests | Not started: planned | [Phase 9](#phase-9-s9-progress) |
| S10 Player guide | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S8 (help sync plan `86c842c5a62a`, 2026-10-01). The study's
steps, S1-S8, are merged. S-immediate readies the local Luminari Web client for S9, S9 plays the
whole system in game and records it, and S10 turns that record into a player guide.

## Working a step

- Branch `feat/vessels-sN` from the previous step's merge commit, and push an annotated tag
  `vessels-sN-base` on that commit, so `git log vessels-sN-base..vessels-sN` lists only the step.
  The first commit is the step's plan: a "Phase N (SN) progress" section under Active step, with
  the items, the interpretations, and the planning ablation.
- Build to Part 5's standard, and keep the step's section current as the work goes (decisions
  made while building included), so another session can take it over.
- Verify with `make test-all` with the database cases on, the new SQL (schema, rollback,
  verifier) on the test database, the vessel help verifier, every live gate in `scripts/vessels/`
  (the private-namespace harness on a reload of the development dump), and the local CI matrix
  `scripts/ci/local/run.py --base gitlab/master`; record the results in the step's section.
- Hand off with an annotated tag `vessels-sN` at the head given to review and a GitLab merge
  request from the branch, with remove-source-branch and squash off. Review fixes go on top, one
  commit each, recorded in the step's section with a finding, fix, and commit table. Pushed tags
  never move.
- Merge with a merge commit, never a squash (the records cite commit hashes), and keep the branch.
  Then sync the step's help to production, move the step's section to the history document, update
  the status row, and tag and branch the next step from the merge.

## Key locations

- DurisMUD: `/home/aiwithapex/projects/duris/`. The ship code is in `src/ships/`: `ships.h`
  holds the module map and each `.c` file an `OVERVIEW` block. The references are
  `docs/reference/SHIPS.md`, `docs/reference/SHIP_GAMEPLAY.md`, and `lib/information/helpships`.
- LuminariMUD (this repository):
  - `src/vessels/` holds the code; start with `vessels.h`.
  - `docs/systems/VESSEL_SYSTEM.md` is the current-behavior reference.
  - `docs/product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md` holds the release gates.
  - Help lives in `lib/text/help/help.hlp` and `sql/components/help_vessel_entries.sql`, checked
    by `verify_help_vessel_entries.sql`.
  - Schema and content SQL is `sql/components/vessels_*`.
  - The tests are `unittests/CuTest/test_vessel_*.c` and `test_transport_production.c`.
  - The live gates and content provisioners are in `scripts/vessels/`.
- The full lists, with line references at the study baseline, are in the
  [history](vessels-ships-history.md#durismud-vessel--ship-code).

## Rules and units

From the study's scope ([history](vessels-ships-history.md#study-scope-and-baselines)):

- Rules applied, from the earlier Duris conversion work: when an existing LuminariMUD mechanic
  already produces a Duris effect, the gap is closed in one line; when LuminariMUD lacks the
  mechanic, the recommendation is the real mechanic, never a cheaper stand-in. This is not an
  exact-parity port. Duris numbers are the player-tested prior for the Open "Player-data
  balance" gate in `VESSEL_SYSTEM_REQUIREMENTS.md`, translated into LuminariMUD's D20 rules, units
  and economy.
- Units. Both games measure range in map rooms (a Duris ocean map room and a LuminariMUD
  wilderness coordinate are both one room), so Duris ranges transfer unchanged. A Duris ship tick
  is 1 s; a LuminariMUD vessel tick is 0.5 s (`VESSEL_PERIODIC_FAST_CADENCE`, 5 pulses at 10
  pulses per second, `src/vessels/vessel_periodic.c:15`). Duris prices are platinum (pp). On the
  default hull prices, 1 pp is about 2 LuminariMUD gold for ship, transport and warship hulls
  (Part 2.2); that is a starting conversion, not an economy measurement.

## 3.4 Owner decisions (2026-09-28)

- D1 Pacing: the Duris scale, `speed / 90` rooms per 0.5 s tick.
- D2 Time to kill: 3-8 minute median for equal warships.
- D3 Loss model: Duris; the hull is lost, the ship's identity survives (3.3.7).
- D4 Crew economics: one-time hire, no wages (3.3.5).
- D5 Ownership: a small configurable cap, 3 hulls per owner by default, `cedit` range 1-10;
  public and NPC hulls do not count, and a capture at the cap is refused.
- D6 Capture: only disabled prizes: a breached arc, immobile, colors struck (`strikecolors`,
  owner or permit holder, until the hull moves or 10 minutes pass), or abandoned at sea (no
  conscious character or crew aboard). Hostile boarding needs the target at speed 3 or less or
  disabled.

## Part 5: Implementation sequence

Each step ships with production-linked CuTest coverage in `unittests/CuTest/`, help in both
`lib/text/help/help.hlp` and the database (`sql/components/help_vessel_entries.sql`), schema and
rollback SQL where tables change, `VESSEL_SYSTEM.md` updates, and, where play changes, an
actual-character gate in the `scripts/vessels/` pattern.

01. S1 Defects that do not wait for the redesign: L4 weather bands, L5 gunnery authorization and
    owner consent, L7 miss lag, L8 contact list and IDs, L9 port immunity, L11 `for_sale`,
    `min_level`, departure level check and the ownership cap (D5), L12 wage removal (D4), L13
    bounty pay-off and decay.
02. S2 Movement and pacing (D1): fractional movement with per-cell validation, class accel and
    turn with the sailmaster and rudder factors, the load and sail limits on maximum speed,
    `setsail` as the maneuver command, `undock` departure and `anchor`, SEADOG +1 speed, the class
    table's speeds, and every automated mover rebased. Ferry soak and scale benchmark are
    re-baselined in `docs/testing/VESSEL_BENCHMARKS.md`.
03. S3 Damage model: Duris arcs and armor profiles with the 229 armor limit, sail HP, breach
    states and sink timers, criticals, weapon damage, knockdown saves, cargo spill and `salvage`,
    and the D6 capture, plunder and boarding rules with `strikecolors`; also the 3.3.1 refit
    rescaling (plating and reinforcement +20%, rigging +10% speed), moved here from S2.
04. S4 Weapons and gunnery: the weapon and equipment tables, 16 slots, fitting, ammo and resupply
    commands, `lock`, battle stations, arc fire, the geometry DC hit model, `sight`, `scan`,
    crew-stun and flight rules; the duel harness moved to the D2 bounds; and, with battle
    stations, the 3.3.2 crash check for refused rooms and shallows, moved here from S2.
05. S5 Crew, repair and loss: one-time hire prices and gates, crew experience, promotion and
    casualties, stamina, the repair stock and dock repairs, the D3 wreck registry with automatic
    insurance, `shipsummon`, trade-in and the rename fee.
06. S6 NPC raiders and AI: the hunter lifecycle generalized into the raider tiers with the 3.3.8
    cadence, fit-outs from prototypes and weapon rows, basic and advanced AI, NPC boarding and
    looting, despawn rules, ramming with the ram, and neutral colors.
07. S7 Rewards and economy: renown, salvage and bounty payouts, the renown board, Ship Damage
    Control, contraband and customs, and the trade modifiers.
08. S8 Client: the MSDP additions.
09. S9 Claude Code play tests: the whole system played end to end in game through a real client,
    a staff character staging the mortal players, with screenshots and guide notes for S10; every
    defect found is fixed to the standard above.
10. S10 Player guide: an illustrated guide to the whole system, written from `guide-notes.md`
    and the S9 screenshots.

S-immediate runs before S9: it gives the local Luminari Web client every feature S9 needs, the
ship data panel among them.

## Active step

### S-immediate progress

In review (2026-10-01). S-immediate makes sure the local Luminari
Web (`LOCAL_WEBCLIENT_PATH`: `/home/aiwithapex/projects/luminariweb`) has every client feature
S9's chapters need, checks each one against the real game, and builds the ones missing. The client
work is committed on the local branch `feat/ship-panel` in that checkout, from `main` at
`41ced8a`, under its own checks (`npm run lint`, `npm run build`, `npm test`); it is not pushed or
deployed. This document records it on `feat/vessels-s9`, after the plan commit. The live checks
ran S9's server (Setup, Server, below) and the bridge; S9 continues on them.

| # | S9 needs | Luminari Web before | Work | State |
| -- | -- | -- | -- | -- |
| 1 | To reach the namespaced MUD on 127.0.0.1:4100 | A local preset behind the documented development opt-in (`PROXY_PUBLIC_MODE=false`, `PROXY_ALLOW_LOCAL_DESTINATIONS=true`, `LOCAL_MUD_PORT=4100`) | Checked live | Done: with the opt-in the dev server offers "Local development MUD" (127.0.0.1:4100) as its default, and it connects through the bridge |
| 2 | Kohdee and Vesselmate from the master account's character menu, and a new character on a test account | Structured onboarding v1 with the existing-character menu (merged into `main`) | Checked live | Done: onboarding signed in to the master account, listed its five characters, and entered Vesselmate and Kohdee; Brinewick, made on the new account Sailtest, entered the same way |
| 3 | Three sessions at once | The proxy allows 4 WebSocket connections per IP | Checked live | Done: `who` showed 3 players from 2 accounts |
| 4 | Sessions that stay up through staging and long passages | Idle connections close after 5 minutes; `PROXY_IDLE_TIMEOUT_MS` goes to 1 hour | Ran at the 1-hour maximum; checked live | Done: the proxy runs with `PROXY_IDLE_TIMEOUT_MS=3600000`, and Brinewick sat idle 11 minutes and stayed connected. The page's 30-second heartbeat also re-arms the idle timer, so the limit closes only a stalled page; the MUD's own idle limits (`idle_void` 610 and `idle_rent_time` 600 ticks, mortals below level 2) are hours long |
| 5 | Vessel output drawn as the game draws it: `tactical`, `lookout`, `shipstatus`, `contacts`, the wilderness map, colors, wide lines | The terminal renders ANSI and Luminari color codes and asks for 256 colors | Checked each live aboard a hull; fixed what rendered wrong | Done after three renderer fixes (Defects, below) |
| 6 | The ship data: the 21 vessel MSDP variables | Not requested, mapped, or shown | Built a Ship panel | Done: the Ship tab, below |

Built on `feat/ship-panel`:

- `shared/mud.ts`: the 21 names join the default variable map and the source-confirmed list, so
  the session REPORTs them; `shared/msdp-state.ts` maps them, the arc tables and the weapon and
  contact arrays as sent.
- `shared/msdp-ship-display.ts`, the display model: `[ID] name`, condition, lock, position
  `(x, y, z)`, heading and speed; hull, sails, rudder and crew stamina as current of maximum; armor
  and structure per arc in `shipstatus` order (fore, port, starboard, rear); each weapon with the
  `shipstatus` words for its state (ready, reloading, out of ammunition, disabled with its damage,
  destroyed); and the contacts with range, bearing and arc, the locked one marked. A ship without
  a contact ID is "not aboard": ashore the server empties `SHIP_ID`, an older server never sends
  it, and the client clears its state on any disconnect.
- `src/App.tsx` and `src/App.css`: the Ship tab after Combat, with an Arc / Armor / Structure table
  (one row per arc fits the narrow inspector; text rows wrapped).
- Tests: `tests/fixtures/msdp/ship-data.json`, five fixtures in the S8 wire format (the scalars
  with crew stamina in deficit, the arc tables, the weapon and contact arrays of tables, the empty
  state ashore), feed the existing parser and mapping tests; `tests/msdp-ship-display.test.ts`
  covers the model. The protocol checklist (`shared/protocol-feature-status.ts`,
  `docs/protocol-feature-checklist.md`) lists the vessel variables as supported.

| Client commit | Change |
| -- | -- |
| `39df437` | Lint ignores `.kilo/`: a Kilo Code worktree there held a second copy of the sources, so `npm run lint` failed on `main` itself |
| `74d0317` | The 21 variables, the display model, the Ship tab, fixtures and tests, the checklist row |
| `01752ac`, `8292ef7` | Split color sequences (Defects) |
| `e899a57` | Line breaks at chunk boundaries (Defects) |
| `8b08195` | The arc table |
| `70b80df` | Zero-padded 256-color codes (Defects) |

Decisions while building:

- A value that has not arrived shows `-`: the server sends the variables one state message each,
  so just after boarding `SHIP_ID` can arrive before the rest.
- The weapon states read the `shipstatus` words off `AMMO`, `READY` and `DAMAGE` as the S8 contract
  defines them, rather than new labels.
- Ablation (building): dropped a settings-editor group for the ship names (the defaults return on
  every load, and S9 needs no override), offline, loading and error states for the tab (the client
  clears its state on any disconnect, so "not aboard" covers them), and new styles beyond the arc
  table (the gauges and rows reuse the group and inventory styles). Added from evidence: the
  `.kilo` lint ignore.

Defects found and fixed:

| Finding | Fix | Commit |
| -- | -- | -- |
| The harbor provisioner failed its ferry fare check on a fresh dump. Room 1000389, the Testing Dock at (-66, 92), lived only in the untracked development world file and was lost when that file was replaced on 2026-08-19, so the ferry's west stop was shallow water (known since S2, which staged at the east dock instead) | The harbor package carries the room beside the east dock, and the provisioner merges it back in; `VESSEL_SYSTEM_TESTING.md` updated | `9f49b3f1f` |
| Luminari Web: a reply the proxy split inside a color sequence printed the sequence's tail (`830/830[0;33mV` after `lookout`) | The stream converter holds an unfinished escape until the next chunk completes it | client `01752ac`, `8292ef7` |
| Luminari Web: in stream mode ansi-to-html 0.7.2 replays each chunk's `<br/>` tokens at the start of the next chunk, so blank lines piled up and a `contacts` row split between two chunks broke in two | Line breaks stay text, which the terminal's `white-space: pre-wrap` shows | client `e899a57` |
| Luminari Web: LuminariMUD zero-pads 256-color codes (`ESC[38;5;018m`), which rendered `color:undefined`, losing every wilderness terrain color | The renderers drop the padding before conversion | client `70b80df` |

Live check (2026-10-01, S9's server on the namespace harness, three browser sessions): Kohdee
spawned Starfall Bastion (prototype 27) at (900, 225) and transferred Vesselmate aboard. His Ship
tab showed `[AN] Starfall Bastion`, sound, no lock, (900, 225, 0), hull 135/135, sails 140/140,
rudder 20/20, crew stamina 500/500, armor 76/95/95/57 over structure 33/41/41/20, and three ready
Large Ballistas; at speed 6 on heading 90 it followed her to (902, 226). `vesseldebug raider 0`
launched three Corsair Clippers in turn. The tab tracked each one in from about 45 rooms, their
rams and grapples (fore armor 76, then 53, then 34; sails 136/140; rudder 19/20), Kohdee's lock
(`Locked on [AO]`, the contact marked locked), and the starboard volley (a direct hit; 29 rounds,
reloading, then ready). Transferred ashore, Vesselmate's tab said "Not aboard a vessel".
`tactical`, `lookout`, `shipstatus`, `contacts` and the wilderness map rendered as sent once the
fixes were in.

For S9:

- Editing the client while sessions are open hot-reloads `App.tsx`, which closes every session's
  link; make client fixes between chapters and log the characters back in.
- The server prints a second, bare prompt (with IAC GA) when a command waits a pulse in the action
  queue, so a reply follows two prompts; Luminari Web ignores GA and shows both on one line. The
  gate transcripts show the same: it is core prompt behavior, not vessel or client behavior.
- The dump's harbor merchant (ship 11) is battered (armor 0, sails 7/110, rudder 1/20, top speed
  1); `shipfix 11` before chapter 6 if she is needed.
- The harbor provisioner's channel check logs in Accessprobe; that is its established behavior.
- Kohdee's and Vesselmate's pfiles and the player index were copied before the live check to
  `/tmp/claude-1000/s9/pfiles-before/`; S9's cleanup restores them from there.
- How it runs: the harness `/tmp/claude-1000/vs4` (start with `rm stop` and
  `setsid nohup unshare -rn --pid --fork --mount-proc bash stage1.sh &`; jobs go in `jobs/`); the
  MUD is the stand-in unit `luminari-dev-login-smoke`. The setup job (`running/x02-setup.sh`)
  reloads the dump, enables ship 11's schedule, runs both provisioners, creates Brinewick on
  Sailtest, and starts the namespace end of the bridge (unit `s9-bridge`, unix socket
  `/tmp/claude-1000/s9/mud.sock`). The host end is
  `socat TCP-LISTEN:4100,bind=127.0.0.1,fork,reuseaddr UNIX-CONNECT:/tmp/claude-1000/s9/mud.sock`,
  and the client runs in its checkout as `PROXY_PUBLIC_MODE=false PROXY_ALLOW_LOCAL_DESTINATIONS=true LOCAL_MUD_PORT=4100 PROXY_IDLE_TIMEOUT_MS=3600000 npm run dev`
  (page http://localhost:5190, log `/tmp/claude-1000/s9/client.log`).
- Helpers in `/tmp/claude-1000/s9/`: `login.sh <session> <master|account> <character>` signs a
  browser session in through onboarding (the password comes from `lib/.env` and is never printed),
  and `cmd.sh <session> <wait-ms> <command>` types a command and prints the terminal's tail.
  S-immediate's screenshots are there as `shot-NN-moment.png`; S9 takes its own.

Interpretations decided while planning S-immediate:

- "All the features we need" is the six needs above, taken from S9's chapters, not the client's
  own backlog.
- The panel shows each value as the game reports it; it computes no game rules of its own.
- Local copy only: the upstream client (`LuminariMUD/luminariweb`) and the second local copy
  (`webclient-luminari`) stay untouched.

Ablation (planning): dropped GMCP and MCCP support and the client's deferred MSDP fields (S9 needs
none of them), any server change (S8's variables are what the panel shows), a vessel overlay on
the client's Map tab (the terminal's `tactical` chart draws the waters), and browser end-to-end
specs for the panel (the fixtures prove the mapping, and the live session the rest). Simplified:
the idle and connection limits are settings, not code. Kept: the not-aboard state, without which
the tab would show a ship the character has left, and the fixtures, which the client's checklist
requires before it claims a protocol feature.

Verification (2026-10-01):

- Client, `feat/ship-panel` at `70b80df`: `npm run lint` clean, `npm test` 352 passed,
  `npm run build` passed.
- Source: S-immediate changes no C, SQL or help, so `make test-all`, the SQL checks and the help
  verifier were not rerun; the CI matrix's build and test jobs cover the code. On a fresh reload
  of the development dump the harbor provisioner passed with the west dock restored (ferry fare
  collected, named-water crossing, captain's channel, merchant), as did the Vailand campaign
  provisioner and the test-character creation.
- All 21 live gates passed in the namespace harness on the installed build (`bin/luminari`
  SHA-256 `830a367d38e71761...`): merchant 41 s, campaign 149 s, Vailand merchant 168 s, builder
  58 s, gunnery 82 s, tactical 371 s, lookout 22 s, boarding 53 s, narrative 21 s, rules 41 s,
  events 71 s, movement 108 s, loss 79 s, damage 587 s, derelict 39 s, hunter 86 s, frontier
  234 s, raider 202 s, economy 227 s, client 25 s. The hunter gate first failed: it stages its
  target at the Testing Dock, which now berths the hull, and its 33-second wait for the 30-second
  cast-off ran out on a host loaded by the CI matrix. It now waits 43 seconds, the Tcl checks'
  grace (`e70d0f106`), and passed on the rerun.
- The local CI matrix (`scripts/ci/local/run.py --base gitlab/master`, 33 jobs) passed on
  `1726b48f2` in 298 s and again on the head handed to review.

Hand-off: tag `vessels-s-immediate` and MR !14 from `feat/vessels-s9` (the plan commit
`13679d5f6` and S-immediate's commits; range `vessels-s9-base..vessels-s-immediate`); the client
branch is reviewed in its local checkout (`git log -p 41ced8a..feat/ship-panel`). Review fixes go
on top, one commit each. S9 continues on `feat/vessels-s9` after the merge, with the S9 server,
bridge and client left running as described above.

### Phase 9 (S9) progress

Not started; this plan, with S-immediate's, is S9's first commit, and S-immediate comes first.
Branch `feat/vessels-s9` from master (`1e0f2f64c` when planned: the S8 merge and its close-out),
with the annotated tag `vessels-s9-base` there.
Hand-off as in the routine: tag `vessels-s9` and a merge request; review fixes go on top. Scope:
play the whole vessel system in game, end to end, the way a new captain meets it, through a real
client; capture the screenshots and notes from which S10 writes the player guide; fix every
defect found on the way.

Setup:

- Server: the installed build of the branch in the private-namespace harness (the Phase 1 record
  in the history; the S2-S8 copy is `/tmp/claude-1000/vs4`), on a fresh reload of the development
  dump plus the content the gates apply: `provision_vessel_harbor.sh`,
  `provision_vessel_campaign.sh`, the Phase 22 and 23 schemas, the raider, contraband and harbor
  sandbox content SQL, and ship 11's schedule enabled. The disposable database keeps every
  purchase, sinking and payout out of the shared development database.
- Client: the local Luminari Web as S-immediate leaves it, the first-party browser client: ANSI
  rendered in the page beside its MSDP panels and the Ship tab, started with `npm run dev` and its
  development settings, at its local preset, 127.0.0.1:4100. A socat bridge joins host
  127.0.0.1:4100 to the MUD inside the namespace through a unix socket. Host 4100 was free when
  planned; if the main checkout's MUD holds it at the start, ask before stopping it. Chromium,
  driven by `agent-browser`, holds one session per character.
- Characters: Kohdee (staff, on the master account in `lib/.env`) stages; Vesselmate (the
  existing level 1 mortal on the same account) plays; one new mortal, made with
  `dev_create_test_character.sh <test-account> <name>` on its own test account, joins for the
  chapters that need a second captain, so no account has more than two characters online.
  Accessprobe, Accessrecs and Gizmotest belong to other work and stay untouched. Kohdee gets the
  players into position with existing commands: `advance`, `set <player> gold`, `transfer`,
  `goto`, `restore`, `vedit spawn` and `spawnpublic`, `vesseldebug raider <tier>` from aboard a
  player's hull, `shipgoto`, `shipfix`, and `vevent start`.
- Screenshots: PNG captures of the browser at one fixed viewport, from the player's session, in
  play order under `docs/ongoing-projects/guide-screenshots/` as `NN-chapter-moment.png`, each
  under the 500 KB commit limit (crop to the terminal and the panel that matters when larger).
  None shows the master account's name or a password; a login screen, if the guide needs one,
  comes from the new test account.
- Notes: `docs/ongoing-projects/guide-notes.md`, ASCII, one section per chapter. For each
  screenshot: the file, the character, the exact commands typed, what it shows, and what had to
  be true first; then the prices, timings, refusals and tips met on the way. S10 writes from these
  notes and the screenshots alone.
- Defects: a crash, wrong behavior, a misleading message, or wrong or missing help is fixed in S9
  to Part 5's standard (production-linked test, help in both places, `VESSEL_SYSTEM.md`), one
  commit each, recorded below with the finding, the fix and the commit; then the namespaced MUD
  restarts on the new build and the chapter is replayed, its screenshots retaken. A Luminari Web
  defect is fixed in its local copy, as in S-immediate.
- Verification: the play record. If S9 changes code, help or SQL, the routine's verification runs
  once at the end.
- Cleanup: Kohdee's and Vesselmate's pfiles restored from copies taken before play, the new
  mortal's files removed, autorun's artifacts moved out of the worktree, and the harness, bridge
  and client stopped.

Chapters, in play order:

| # | Chapter | Played by | Kohdee stages | Covers | State |
| -- | -- | -- | -- | -- | -- |
| 1 | Finding a ship | Vesselmate | His level and gold for a hull; the Testing Dock | `help vessels` and the ship help entries, `shipbrowse` | Not started |
| 2 | Buying and knowing her | Vesselmate | - | `shipbuy`, `shipchristen`, `shipcustomize`, `board`, `disembark`, `ship_rooms`, `shipstatus`, `shipcrew` | Not started |
| 3 | Crew, weapons and refits | Vesselmate | Gold as needed | `shiphire`, `shipdismiss`, `shipweapon` buy and sell, `shipequip`, `shiprearm`, `shipupgrade` | Not started |
| 4 | Sailing | Vesselmate | - | `undock`, `setsail`, `heading`, `speed`, `anchor`, `tactical`, `lookout`, `contacts`, `seastate`, `shiptalk`, at-sea narrative and weather, legal waters, `dock`, `dockfees` | Not started |
| 5 | Routes, autopilot and schedules | Vesselmate | - | `setwaypoint`, `listwaypoints`, `delwaypoint`, `createroute`, `addtoroute`, `delroute`, `listroutes`, `setroute`, `autopilot`, `setschedule`, `showschedule`, `clearschedule`, `assignpilot`, `unassignpilot` | Not started |
| 6 | Passage on public ships | Vesselmate | - | The harbor ferry and its fare, the Vailand merchant | Not started |
| 7 | Trade and freight | Vesselmate | - | `market`, `cargobuy`, `cargomanifest`, `cargosell` and its modifiers, `contracts`, `contractaccept`, `contractdeliver`, `contractabandon` | Not started |
| 8 | Gunnery against a raider | Vesselmate | `vesseldebug raider 0` from aboard his hull | `shipsight`, `shipscan`, `shiplock`, battle stations, `shipfire` by arc, reloads and ammunition, raider tactics and boarding, `shipram` | Not started |
| 9 | Damage, repair and salvage | Vesselmate | `shipfix` between runs when needed | Arcs, breaches, sails and rudder, criticals, the sink timer, `shiprepair` at sea and at a dock, cargo spill and `shipsalvage` | Not started |
| 10 | Boarding and taking a prize | Vesselmate | A raider to disable | `boardcheck`, `board_hostile`, `claimship`, `plunder`, the D6 prize rules | Not started |
| 11 | Two captains | Vesselmate and the new mortal | Both hulls at sea, PvP on, the pair grouped and then apart | `shippermit`, `shiprevoke`, `shipdeed`, a PvP fight to a sinking, `strikecolors`, renown and `shiprenown`, prize money and its mail, allies' shares, `bounty`, `marque` | Not started |
| 12 | Contraband and customs | Vesselmate | - | Contraband with the renown won in chapter 11, the smugglers' `market` listings, customs at a lawful port | Not started |
| 13 | Loss and recovery | The new mortal | - | The wreck registry, the insurance claim, `shipsummon`, trade-in (`shipbuy <id> trade`), the rename fee, the ownership cap | Not started |
| 14 | Other hulls and vehicles | Vesselmate | `vedit spawn` of the frontier classes where each can travel | River craft, airship altitude, submarine depth, transport, magical hulls; `vmount`, `drive`, `vstatus`, `vdismount`, `loadvehicle`, `unloadvehicle`, `tenter`, `tgo`, `tstatus`, `texit` | Not started |
| 15 | The living world | Vesselmate and the new mortal | `vevent start` | The Blackwake derelict, `vevent status`, `join`, `enlist` and `leaderboard`, encounters and bounty hunters | Not started |
| 16 | Staff tools | Kohdee | - | `vedit`, `vmerchant`, `shiplist`, `shipgoto`, `shipfix`, `shippurge`, `boardfind`, `vesseldebug`, `vevent end`, `cancel` and `recover`, the `cedit` vessel switch and ownership cap | Not started |
| 17 | Client data | Vesselmate | - | The Ship tab aboard, in a fight and ashore, and the 21 vessel MSDP variables behind it | Not started |

Interpretations decided while planning S9:

- End to end means every system and player command once, in the order a new captain meets them,
  plus the staff tools as an appendix. The options of each command are the gates' work, not S9's.
- The screenshots show what a player sees, so they come from the mortals' sessions; Kohdee's
  staging appears only in the staff chapter.
- Contraband comes after chapter 11's renown, as a player reaches it, instead of renown granted
  by staff.
- At sea the screenshots carry the Ship tab beside the terminal where the moment needs it; chapter
  17 covers the client data itself, for players who script their clients.

Ablation (planning): dropped a new scripted gate or play automation (the 20 gates exist; S9 is
interactive play), Mudlet and LuminariGUI (Mudlet is not installed here, and Luminari Web is the
configured local client that a browser can drive and capture), play on the shared development or
production database, writing the guide (S10), and re-running the gates when S9 changes nothing.
Simplified: staging uses existing staff commands rather than pfile or SQL edits, and the harness,
dump and provisioning are the gates' own. Kept: a second mortal, without whom permits, deeds, PvP
renown, allies and the loss of a player's hull cannot be played; the fixes with their tests; and the
screenshot size limit, which the commit hook enforces.

Defects found: none yet.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S-immediate Luminari Web for S9 | The Ship panel (21 variables, tables and arrays) with its fixtures and tests, and five live checks | 1 |
| S9 Claude Code play tests | 17 chapters of live play at D1 pacing and D2 fight lengths, plus the defects found | 2-3 |
| S10 Player guide | Writing from the notes and screenshots | 1 |

The Open player-data balance and human beta gates depend on player availability, not engineering
time.

## Ablation record

- Dropped as Duris-specific or already covered: racewar ocean-PvP state and `signal` (PvP consent
  and `shiptalk` cover them), the Redis and flat-file backends, Duris ferries and autopilot
  (schedules and routes cover them), runtime identity references (generation-aware events cover
  them), the levistone (airships cover flight), delayed market prices (per-unit batch pricing
  already stops quick flips), ship coffers (the settlement path pays owners), CTF speed penalty,
  the Trader achievement, the Sailor's Tattoo, and the unique Cyric's Revenge and automatons
  quest content.
- Simplified: continuous crew skills become experience on the existing four positions and three
  tiers, chiefs are those positions, NPC raiders extend the hunter lifecycle instead of a new
  spawner, the hit model reuses Duris's geometry as a D20 DC instead of a new table, and volley
  flight time is omitted (Duris freezes the chance at firing, so resolving at once changes only
  the delay).
- Kept: everything that satisfies requirement 4.4 (range, bearing, arcs, reloads, armor sections,
  subsystem damage, repair, sinking, wrecks, boarding, capture, NPC doctrine, observable disabled
  subsystems), the Duris features added for parity (stamina, anchor, contraband, damage control,
  equipment), and the pacing change without which none of it can matter.
