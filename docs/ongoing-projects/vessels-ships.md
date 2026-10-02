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
| S-immediate Luminari Web for S9 | Merged `1c7e4bffb` (MR !14) | [S-immediate](vessels-ships-history.md#s-immediate-progress) |
| S9 Claude Code play tests | In review (tag `vessels-s9`, MR !15) | [Phase 9](#phase-9-s9-progress) |
| S10 Player guide | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S8 (help sync plan `86c842c5a62a`, 2026-10-01); S-immediate
changed no help; S9 changes help, synced after its merge. The study's steps, S1-S8, are merged,
and so is S-immediate, which readied the local Luminari Web client for S9. S9 played the whole
system in game and recorded it (in review), and S10 turns that record into a player guide.

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

### Phase 9 (S9) progress

In review (played 2026-10-02). This plan, with S-immediate's, was the first commit on `feat/vessels-s9`, branched
from master `1e0f2f64c` (the S8 merge and its close-out) with the annotated tag `vessels-s9-base`
there. S-immediate merged from the same branch as `1c7e4bffb` (MR !14), and the branch was
fast-forwarded onto that merge, so S9's own commits are `git log 1c7e4bffb..vessels-s9`
(`vessels-s9-base..` also lists the plan and S-immediate). S-immediate left S9's server, the
bridge and the client running; its [For S9 notes](vessels-ships-history.md#s-immediate-progress)
say how they run and what it found.
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
- Client: the local Luminari Web as S-immediate leaves it (branch `feat/ship-panel` in its
  checkout; S9's client fixes go on top), the first-party browser client: ANSI
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
| 1 | Finding a ship | Vesselmate | His level and gold for a hull; the Testing Dock | `help vessels` and the ship help entries, `shipbrowse` | Played |
| 2 | Buying and knowing her | Vesselmate | - | `shipbuy`, `shipchristen`, `shipcustomize`, `board`, `disembark`, `ship_rooms`, `shipstatus`, `shipcrew` | Played |
| 3 | Crew, weapons and refits | Vesselmate | Gold as needed | `shiphire`, `shipdismiss`, `shipweapon` buy and sell, `shipequip`, `shiprearm`, `shipupgrade` | Played |
| 4 | Sailing | Vesselmate | - | `undock`, `setsail`, `heading`, `speed`, `anchor`, `tactical`, `lookout`, `contacts`, `seastate`, `shiptalk`, at-sea narrative and weather, legal waters, `dock`, `dockfees` | Played |
| 5 | Routes, autopilot and schedules | Vesselmate | - | `setwaypoint`, `listwaypoints`, `delwaypoint`, `createroute`, `addtoroute`, `delroute`, `listroutes`, `setroute`, `autopilot`, `setschedule`, `showschedule`, `clearschedule`, `assignpilot`, `unassignpilot` | Played |
| 6 | Passage on public ships | Vesselmate | - | The harbor ferry and its fare, the Vailand merchant | Played |
| 7 | Trade and freight | Vesselmate | - | `market`, `cargobuy`, `cargomanifest`, `cargosell` and its modifiers, `contracts`, `contractaccept`, `contractdeliver`, `contractabandon` | Played |
| 8 | Gunnery against a raider | Vesselmate | `vesseldebug raider 0` from aboard his hull | `shipsight`, `shipscan`, `shiplock`, battle stations, `shipfire` by arc, reloads and ammunition, raider tactics and boarding, `shipram` | Played |
| 9 | Damage, repair and salvage | Vesselmate | `shipfix` between runs when needed | Arcs, breaches, sails and rudder, criticals, the sink timer, `shiprepair` at sea and at a dock, cargo spill and `shipsalvage` | Played |
| 10 | Boarding and taking a prize | Vesselmate | A raider to disable | `boardcheck`, `board_hostile`, `claimship`, `plunder`, the D6 prize rules | Played |
| 11 | Two captains | Vesselmate and the new mortal | Both hulls at sea, PvP on, the pair grouped and then apart | `shippermit`, `shiprevoke`, `shipdeed`, a PvP fight to a sinking, `strikecolors`, renown and `shiprenown`, prize money and its mail, allies' shares, `bounty`, `marque` | Played |
| 12 | Contraband and customs | Vesselmate | - | Contraband with the renown won in chapter 11, the smugglers' `market` listings, customs at a lawful port | Played |
| 13 | Loss and recovery | The new mortal | - | The wreck registry, the insurance claim, `shipsummon`, trade-in (`shipbuy <id> trade`), the rename fee, the ownership cap | Played |
| 14 | Other hulls and vehicles | Vesselmate | `vedit spawn` of the frontier classes where each can travel | River craft, airship altitude, submarine depth, transport, magical hulls; `vmount`, `drive`, `vstatus`, `vdismount`, `loadvehicle`, `unloadvehicle`, `tenter`, `tgo`, `tstatus`, `texit` | Played |
| 15 | The living world | Vesselmate and the new mortal | `vevent start` | The Blackwake derelict, `vevent status`, `join`, `enlist` and `leaderboard`, encounters and bounty hunters | Played |
| 16 | Staff tools | Kohdee | - | `vedit`, `vmerchant`, `shiplist`, `shipgoto`, `shipfix`, `shippurge`, `boardfind`, `vesseldebug`, `vevent end`, `cancel` and `recover`, the `cedit` vessel switch and ownership cap | Played |
| 17 | Client data | Vesselmate | - | The Ship tab aboard, in a fight and ashore, and the 21 vessel MSDP variables behind it | Played |

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

Progress log (2026-10-02, kept current as play goes):

- Setup, rerun fresh for S9 (harness job `x04-s9setup`): the server stopped, Kohdee's and
  Vesselmate's pfiles restored from `pfiles-before`, the dump reloaded, ship 11's schedule
  enabled, both provisioners run. Brinewick's re-creation then failed ("character-name
  confirmation timeout") because the previous Brinewick's pfile and index line still held the
  name; job `x05-brinewick` removed them with the server stopped and recreated him on Sailtest.
- Setup amended from play (the plan's content list missed what chapters 1, 14 and 15 need): the
  dump's help predates S4, so `help_vessel_entries.sql` is applied to the disposable database
  (production help is current through S8); the dump lists no hull for sale (the frontier content
  that sets `for_sale` is not in it), so `provision_vessel_frontier.sh`,
  `provision_vessel_derelict.sh` and `vessels_narrative_content.sql` join the setup (job
  `x06-content`). The provisioners refuse a dirty tree, so they run before play leaves
  uncommitted notes or screenshots.
- Client: the owner merged `feat/ship-panel` into the client's `main` (`be28d96`); S9's client
  fixes, if any, go on a branch from that `main`.

Ablation (starting play): chapter 17 reuses the Ship-tab screenshots that chapters 2, 4 and 8 take
at sea and in a fight, plus one ashore, instead of replaying them; the rest of the plan stands.

Chapter state is in the table above; `guide-notes.md` has each played chapter's screenshots and
notes. All 17 chapters played (6 while the shipwrights worked on chapter 3's refit; 13 after 14,
while the summoned Kestrel sailed). Left: the routine's verification, tag, push and merge
request, and cleanup.

Play findings that are not defects (recorded in the notes): `boardcheck` and `boardfind` are
bulletin-board commands, not vessel ones (chapter 16 covers the board commands it names only as
what they are); a wreck is rebuilt as the cheapest boat (decision D3) and is worth nothing in
trade; the level gate holds departures only; the sandbox's only encounter row is the rafts-only
Admiralty hunter patrol; a HUNTED captain is refused all business at lawful ports.

How play runs (for a session taking over): the harness, bridge and client run as S-immediate
left them (history, For S9). Helpers in `/tmp/claude-1000/s9/`: `login.sh <session> master <Character>`, `cmd.sh <session> <wait-ms> <command>` (types a command, prints the terminal's
tail; `LINES=n` for more), `shot.sh <session> <file.png> [<inspector tab>]` (saves into
`guide-screenshots/`), and `testenv.sh` (source it for DB-backed CuTest runs against the
`luminari-vessels-testdb` container on 127.0.0.3). Database queries go through harness jobs
(`/tmp/claude-1000/vs4/jobs/qNN-*.sh`, `mariadb "$(cat /tmp/claude-1000/vs4/dbname)"`). To put a
fix in play: `make -j$(nproc) && make install`, then a restart job (copy
`/tmp/claude-1000/vs4/running/r01-restart.sh` into `jobs/`), wait for a new "Entering game loop"
in `/tmp/luminari-dev-login-smoke.log`, and log the sessions back in. A restart drops a player to
the tutorial start (the harness kills the server without a quit), so Kohdee transfers them back.
The harness's `systemctl` stand-in learned `show -p MainPID` for the derelict provisioner.

Staging so far: Vesselmate level 16, gold set to 60,000 before chapter 2, a lantern (play
started at night). He owns the Sea Wren (slot 13, Starfall Survey Ship, prototype 26) at the
Testing Dock with four green hands, a Medium Ballista fore, Large Ballistas port and starboard, a
ram, plating and rigging. The first Sea Wren, bought before the interior fix, was purged and
bought again.

Defects found and fixed (each with a production-linked test; help in both places where it
changed):

| Finding | Fix | Commit |
| -- | -- | -- |
| `shipbrowse`: the 14-letter "Magical Vessel" overflowed the 10-wide Class column | Column widened to 14 | `fbeed236c` |
| `shipbuy` showed a player the builders' spawn line (fleet slot, interior room numbers) | The spawn report is staff-only; the purchase names the hull handed over | `7955887f5` |
| Every hull read "It has a soft glowing aura! ..It emits a faint humming sound!": hulls are instances of object 70002, a builder's fixture flagged GLOW and HUM (world data outside the repository) | Placing a hull drops those two flags, as it already sets the type | `b821585ca` |
| After `shipchristen`, the bridge kept the old name ("Starfall Survey Ship's Bridge" aboard the Sea Wren) until a reboot | Christening re-renders the interior from its templates | `6504c9b82` |
| The purchase said to christen her; christening and customizing work only aboard, and their help did not say so | The purchase says to board her; SHIPCHRISTEN and SHIPCUSTOMIZE help say aboard | `61aed49c7` |
| A new ship's interior contradicted itself (the hold two rooms north of the bridge and also one east), so the minimap drew three "you are here" markers; larger hulls used up and down as spokes, and past nine rooms the spokes wrapped and overwrote the bridge's first exit | Rooms lie on eight level rays out from the bridge; side passages join only neighbors. Interiors persisted earlier keep their stored passages | `49ab1fb55`, test fix `b1f90991e` |
| `lookout` named vessels by fleet slot ([13]) while every other command uses the contact ID ([AN]) | Lookout prints the contact ID; help and the lookout gate say so | `e7018c0a3` |
| A piloted merchant under way read "Speed: 9 / 9 (ordered 0)" in `shipstatus` (and `speed`), as if stopping | Both show the speed the helm is converging on, from the helm tick's own function | `6ca2b5a71` |
| A passenger trying to step off a ship under way was told to "Bring the vessel to a stop first" | Only someone at the helm is told that; others are told to wait | `3d9ddfa3d` |
| Any passenger on a public ferry's or merchant's bridge could set her heading and speed, anchor her, reroute her, clear her schedule, or unassign her NPC pilot (unowned hulls were open to anyone) | An unowned hull with an NPC pilot answers only to NPCs and immortals; help and `VESSEL_SYSTEM.md` say so | `9a3be3361` |
| `help ships` showed the 2014 `boats` entry (SHIPS claimed by it and VESSELS; "enter <boatname>", a staff script pointer, a stray backslash, the keyword TRANSPORTSS) | SHIPS belongs to VESSELS; `boats` (BOATS FERRY FERRIES PASSAGE) is a passenger's guide to public ships; its old alternate keywords cleared | `a9c3ad413`, `753344f9a` |
| Luminari Web: the tactical chart lost its land cells and left the rest of the output orange (the client read the chart's `^` as a color code; the server sends ANSI and compiles out `^` codes) | The terminal stream keeps carets literal (client branch `fix/literal-carets` from `main`, commit `bb7c707`, local) | client `bb7c707` |
| An order given while casting off was told to "order 'undock' to cast off first" | The helm hears how long casting off or weighing anchor has left; the two copies of the refusal are one | `2a3809ee8` |
| `lookout` measured from whole rooms while contacts, tactical and gunnery use exact positions (32.0 vs 31.4 rooms for the same hull) | Lookout reads the one contact list | `a641246e0` |
| Dock fees named the port by room number ("due at port 1000389") | `shipstatus` and `dockfees` name the port | `5a70fdc3d` |
| A one-way route ending in a port paused "before departure" at her last waypoint instead of completing (the berthing fee was checked before arrival) | Arrival is checked first | `548d41cf2` |
| The pilot's lines began in lower case and every autopilot announcement left a blank line | Sentence-start names capitalized; the doubled line breaks dropped | `530fdd072` |
| ADDTOROUTE's help numbered positions from 0; the game numbers from 1 | Help example corrected | `1aba0729c` |
| The freight board offered "10 grain to an unknown port" (room 70000, a ship interior, had market rows) | The board offers only rooms that are ports | `d293b3774` |
| Market, cargo and freight keyed the port by the hull object's room though "in port" was judged by coordinates (and a missing hull object would read past the world) | `vessel_port_room()` serves both | `bf8af8153` |
| Accepting and abandoning a contract loaded its freight for nothing, repeatedly: two rounds sold for 1,218 gold without leaving the dock | Accepting takes the goods' worth as a bond, shown on the board; the payout repays it | `862c944a3` |
| "fit Sea Wren with a Neutral Colors" | No article before Neutral Colors | `aa8c42649` |
| A holed raider dead in the water read "sound" in contacts, tactical, lookout, seastate and shipscan | Any holed hull is at least crippled | `a973e22d2` |
| People put off a hull landed at the coordinates they last stood ashore: a ferry passenger stepping off at the east dock landed at the west dock, and a boarder thrown into the sea at (-91, 77) came up at (-66, 91) (`char_to_room()` uses the character's coordinates; disembark set them after the move, the boarding fall and sinking not at all) | `vessel_char_to_room()` sets them first, for every place that puts people off a hull | `d97a724bd` |
| Boarding an empty hull and failing told the boarder "the defenders drive you back" | With nobody aboard the hull alone resists ("Your grappling lines fail to take hold.") | `c206f6140` |
| A plunder that moved nothing said "nothing worth taking, or no room to take it" | It says the prize's hold is empty, or that her own hull has no room | `c4e795d6e` |
| The raider's brass key answered to `strongbox`, so `unlock`, `open` and `get all strongbox` found the key ("A brass strongbox key is not a container.") | The key answers to key, brass and raider (world file `lib/world/vessel_raiders/700.obj`; production's live record needs the same keywords, as the raider provisioner adds only missing records) | `1182093b4` |
| Grouped captains could fire on, board and claim each other's ships and stay grouped | A recorded hostile engagement against a groupmate's hull costs the aggressor the group, as in person (`fight.c`) | `a3e448e2c` |
| `bounty` told a clean captain "You carries no price." | "You carry no price." | `420e8735b` |
| `cargobuy tomes 10` found nothing (goods matched only from the start of "forbidden tomes"); `marque` and `bounty pay` aboard a berthed hull did not say the admiralty office is ashore | Any word of a commodity's name matches; the refusals and help say ashore | `ab2701d0b` |
| A riverboat steered onto the bank was told she is "designed for coastal waters only" while sailing a river | "Your boat cannot go there! She keeps to rivers and shallow coastal water." | `6fc108a55` |
| Vehicles were never listed in a room: a player beside a cart saw an empty field | `look` lists vehicles standing in the room ("River Cart, a cart, stands here.") | `03f6b6a76` |
| `tgo` moved the cart and left its rider behind | On a land vehicle `tgo` is `drive`, which carries the riders | `1ad24f5df` |
| A magical hull hovering over a field loaded and unloaded a cart from the ground; a submerged hull could too | Vehicles load and unload only with the hull at the surface | `b7681232e` |
| A hull under way in the harbor was told "the harbor watch forbids gunfire from a berth" | "...forbids gunfire in port" | `c428116cf` |
| `shiplock wraith` found nothing beside "Ghost Fleet Wraith 2-1": contacts matched only from the start of the name | Any word of a contact's name matches, nearest first | `d8fb17b16` |
| A summoned hull made port after up to an hour with word only to the dock; her captain elsewhere never heard | The harbor sends word to an online owner elsewhere, naming the port | `34bfe9a51` |
| The Blackwake derelict's chain began with `searchashlog`, a made-up word nothing in game mentions | The bridge trigger answers a plain `search` (`searchashlog` still works); the derelict gate types `search` | `68d3f1230` |
| A new raft could come with a mess hall, medical bay or crew quarters (extra rooms ignored the templates' minimum hull size) | The draw re-rolls rooms the class is too small for: a raft's only extra room is a hold | `56295c437` |

The first ferry ride's mis-landing (in the Testing Dock with the east dock's coordinates) was
explained in chapter 10: `char_to_room()` enters a wilderness room at the character's own
coordinates, which `disembark` set only after the move (fixed in `d97a724bd`, above).

Verification (2026-10-02):

- `make test-all` with the database cases (the `luminari-vessels-testdb` container, see
  `testenv.sh` above) at `29a05cd6f`: 1,984 CuTest cases OK (seed 1) and the protocol harness's
  32; later commits change only gate scripts and tests, which the CI matrix ran.
- Help: on a fresh reload of the development dump, `help_vessel_entries.sql` applied and
  `verify_help_vessel_entries.sql` passed all seven checks (34 entries, 91 command keywords, 52
  content contracts, no obsolete duplicates or retired aliases). S9 adds no schema SQL; its SQL
  edits passed sqlfluff in the commit hook.
- All 20 live gates of S8's run passed in the namespace harness on the installed build
  (`bin/luminari` SHA-256 `4163357897054c0c...`, built at `29a05cd6f`): merchant 36 s,
  campaign 129 s, Vailand merchant 146 s, builder 46 s, gunnery 70 s, tactical 356 s, lookout
  23 s, boarding 47 s, narrative 22 s, rules 35 s, events 41 s, movement 104 s, loss 75 s,
  damage 558 s, derelict 30 s, hunter 89 s, frontier 218 s, raider 196 s, economy 276 s, client
  25 s. The tactical and rules gates first failed on S9's own rewording (the shot target now
  reads crippled, as a holed hull does; the bounty pay-off refusal says ashore) and passed once
  their expectations were updated (`53a1c603f`); the raider gate broke when that commit rewrote
  the tactical script it was running, and passed on rerun.
- Luminari Web, `fix/literal-carets` at `bb7c707`: `npm run lint` clean, `npm test` 353
  passed, `npm run build` passed.
- The local CI matrix (`scripts/ci/local/run.py --base gitlab/master`, 33 jobs) first ran on
  `53a1c603f`: 29 passed. clang-tidy found a dead store in the freight bond test (its second
  acceptance unchecked; `13305c989`), and both sanitizer jobs and the memory check found two
  test-fixture leaks (the `tgo` test's room occupancy events, the rename fee test's bridge
  strings; `fab5cb9ca`). The rerun on `fab5cb9ca` passed
  all 33 jobs in 292 s; the head handed to review adds only this record.

Cleanup: Kohdee's and Vesselmate's pfiles and the player index restored byte-identical from
`/tmp/claude-1000/s9/pfiles-before` after the last gate; Brinewick's pfile and objects removed
(his Sailtest account lived in the disposable database). S9 ran no autorun, so there were no
autorun artifacts. The client (`npm run dev`), both ends of the socat bridge, and the harness are
stopped; the harness's disposable database stopped with its namespace and is reloaded from the
dump on next use.

Hand-off: tag `vessels-s9` and MR !15 from `feat/vessels-s9` (range `1c7e4bffb..vessels-s9`; the
plan commit is in S-immediate's range). The client fix is reviewed in its local checkout
(`git log -p be28d96..fix/literal-carets` in `/home/aiwithapex/projects/luminariweb`). Review
fixes go on top, one commit each. After the merge:

- Sync the help to production. Ten entries changed (by first keyword): ADDTOROUTE, ANCHOR (the
  vessel commands), BATTLE-STATIONS, BOATS (SHIPS moved to VESSELS), BOUNTY, CARGO, CONTRACTS,
  LAND-VEHICLES, LOADVEHICLE and SHIP-OWNERSHIP.
- Production world data: the raider provisioner adds only missing records, so the live object
  #70021 needs the keywords `key brass raider` (`1182093b4`); rerunning the derelict
  provisioner replaces trigger 70010 with the `search` version (`68d3f1230`).
- Then S10 writes the player guide from `guide-notes.md` and the screenshots.

Review fixes (MR !15, on top of `vessels-s9`; range `vessels-s9..feat/vessels-s9`):

| Finding | Fix | Commit |
| -- | -- | -- |
| [P2] Accepting freight debited the bond in memory only while the contract and cargo went straight to the database, so a crash before the next character save kept the freight and returned the bond; a failed manifest write was silent, so a later save could keep the debit without the freight | The contract claim and the manifest commit in one transaction, the bond is debited after it and saved with `save_char_checked()`, and a failed save restores the gold, reopens the contract and unloads the freight; `vessel_db_save_cargo()` reports failure. The DB-backed bond test covers a refused manifest write and a failed save | `a703572e3` |

Review-fix verification: the bond test fails on the old acceptance and passes on
the fix; the focused vessel suite (208 cases) passes; the local CI matrix passed all 33 jobs on
`a703572e3` in 265 s. Its first run, on an earlier version of the fix, failed the SQL
interpolation check (the contract reopen formatted its values into the query; it now binds them
in a prepared statement). No live gate accepts freight.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
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
