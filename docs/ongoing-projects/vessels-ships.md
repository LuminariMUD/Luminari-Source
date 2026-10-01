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
| S9 Claude Code play tests | Not started: next | [Phase 9](#phase-9-s9-progress) |
| S10 Player guide | Not started | [Part 5](#part-5-implementation-sequence) |

Production help is current through S8 (help sync plan `86c842c5a62a`, 2026-10-01); S-immediate
changed no help. The study's steps, S1-S8, are merged, and so is S-immediate, which readied the
local Luminari Web client for S9. S9 plays the whole system in game and records it, and S10 turns
that record into a player guide.

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

Not started. This plan, with S-immediate's, was the first commit on `feat/vessels-s9`, branched
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
