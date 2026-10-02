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
| S9 Claude Code play tests | Merged `a6adb46a8` (MR !15) | [Phase 9](vessels-ships-history.md#phase-9-s9-progress) |
| S10 Player guide | In review: tag `vessels-s10`, MR !16 | [Phase 10](#phase-10-s10-progress) |

Production help is current through S9 (help sync plan `3b26c50a826f`, 2026-10-02). The study's
steps, S1-S8, are merged, and so are S-immediate, which readied the local Luminari Web client for
S9, and S9, which played the whole system in game and recorded it for S10. S10, in review, turns that
record into the [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) and fixes what checking
its facts against the code found.

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

### Phase 10 (S10) progress

In review (2026-10-02). Branch `feat/vessels-s10` from the S9 merge `a6adb46a8`, where the
annotated tag `vessels-s10-base` stands, so `git log vessels-s10-base..vessels-s10` lists only
S10. The first commit after the S9 close-out (`bf5e6bf64`) is this plan. Hand-off as in the
routine: tag `vessels-s10` and a merge request; review fixes go on top. Scope: an illustrated
player guide to the whole vessel system, written from `guide-notes.md` and the S9 screenshots.

Items:

1. The guide: `docs/guides/VESSEL_PLAYER_GUIDE.md`, ASCII Markdown beside the existing
   `NEW_PLAYER_GUIDE_LEVEL_1-5.md`, in the order a player meets the system: passage on public
   ships, finding and buying a hull, crew and refits, sailing, routes, trade, gunnery, damage and
   repair, prizes, other captains, contraband, loss and recovery, other hulls and vehicles, the
   living world, client data; then a staff appendix. Each chapter shows the commands, what the
   player sees (the screenshots with captions), the numbers that matter, the refusals a player
   meets, and tips.
2. The screenshots move from `docs/ongoing-projects/guide-screenshots/` to
   `docs/guides/vessel-guide/` (`git mv`, unchanged files): `ongoing-projects/` is temporary, and
   the guide is permanent documentation. `guide-notes.md` stays as S9's record (the history cites
   it), its screenshot pointer updated.
3. Accuracy: every screenshot audited against the current code (S9 fixed defects mid-play, so a
   shot or a note can predate its fix), and every number and rule in the notes checked against
   the source and help. A wrong note is corrected in the guide; a game or help defect found on the
   way is fixed to Part 5's standard (production-linked test, help in both places,
   `VESSEL_SYSTEM.md`), one commit each, recorded below. A screenshot that shows behavior since
   changed is retaken or captioned.
4. Links: the master index (`docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`) and
   `VESSEL_SYSTEM.md` point to the guide.

Interpretations decided while planning S10:

- The guide is for players; the development world's names (the Testing Dock, the Harbor Sandbox,
  the Sea Wren) and its catalog and market prices are examples, said so once. Code-fixed values
  (crew and weapon prices, fees, timings, shares, thresholds) are stated as the rules.
- Passage on public ships comes first: a new character meets the ferries long before the level
  for a hull of their own.
- Staff tools are an appendix, as S9 played them, so the guide covers the whole system.

Ablation (planning): dropped an HTML copy in the web portal (`docs/web/`; the Markdown renders with
its images on GitLab, and the existing player guide is Markdown), a new play session (S9's
screenshots and notes are the source by plan; a retake only where a shot shows behavior since
changed), an in-game help pointer to the guide (a game client cannot show its images, and the
repository's public address is not settled), and the routine's `make test-all`, gates and help
verifier unless S10 changes code, help or SQL (documentation cannot affect them). Simplified: the
screenshots move instead of being copied. Kept: the screenshot audit and fact check, because the
guide states numbers as facts and the notes were written while fixes landed; and the staff
appendix.

Verification: the commit hooks (mdformat, ASCII hygiene, 500 KB file limit), a check that every
image and anchor in the guide resolves, and the local CI matrix
(`scripts/ci/local/run.py --base gitlab/master`); plus the routine's full verification if S10
changes code, help or SQL.

Progress log (2026-10-02, kept current as the work goes):

- Plan committed. Screenshot audit (four lanes, 01-79) and fact check (chapters 1-7, 8-17)
  done. The fact check found about 30 notes that the code contradicts or that state a special
  case as a rule (the guide states the code's rule; the notes stay as S9 recorded them), and the
  defects below. The audit found no account name, password or address in any shot, and no shot
  that needs a new play session: 21 duplicates 20 and 42 shows a refused shot rather than the
  group forfeit, so the guide describes those two in text and does not use them; 35 shows a ram
  rather than gunfire and is captioned as what it shows; about twenty need a crop or a
  redaction (pre-fix text in 02, 10, 19, 31 and 38, the staff character's staging lines in 17,
  25, 29, 30, 31, 68 and 79, interiors persisted before `49ab1fb55` drawing several "you are
  here" markers in 18, 66 and 67) or a caption limited to what is visible.
- The guide, `docs/guides/VESSEL_PLAYER_GUIDE.md` (`ea5a1da5c`): 17 chapters in the order a player
  meets the system, a command summary and a staff appendix. The screenshots moved to
  `docs/guides/vessel-guide/` (76 kept: ten cropped, five with staging lines or old interior
  minimaps blanked; 21 and 72, copies of 20 and 71, and 42 removed). `guide-notes.md` stays as
  S9's record with its pointer updated.
- A second, fresh review of the finished guide against the code (`049f06aa7`) found about 30
  statements put too broadly or wrongly and 13 captions claiming more than their image shows, all
  corrected, and the four defects at the end of the table below. Its point that waypoints and
  routes have no owner, so any captain can delete another's idle route, needs a schema change and
  is GitLab work item #11.

Ablation (building): no new play session; the three shots that do not show what their notes
say are dropped and their moments described in text, and stale lines are cropped or blanked
rather than retaken. Each fix is the smallest that closes the defect: an in-use guard instead of
a creator column for shared waypoints and routes (work item #11), the absolute MUD hour in the
existing `next_departure` column instead of a new one, a refusal for `autopilot off` under a
pilot instead of new pilot state, and the existing contact resolver for `board_hostile` and
`dock`, which retired `find_ship_by_name()`.

Defects found and fixed (each with a production-linked test where behavior changed; help in
both places where it changed):

| Finding | Fix | Commit |
| -- | -- | -- |
| `unassignpilot` on a hull sailing a route freed the route while the autopilot still held it: `autopilot status` read freed memory, and `setroute`, `autopilot on` or cleanup freed it again (a crash) | Relieving the pilot disengages the autopilot as `autopilot off` does and keeps the route; DB-backed test | `3ad7f52f3` |
| A ship steered at the shore, in shoal water she may sail, was told "It requires deep water to sail" | "Your ship cannot go there! She keeps to the water's surface, clear of beach and land." | `73bae886a` |
| `tenter [target]` ignored its target and entered the first vehicle in the room | It finds the vehicle as `vmount` does and refuses a name that matches none | `7d26fb597` |
| `tgo` hinted "Usage: go ..." and "Try 'enter' ...", other commands | The hints name `tgo` and `tenter` | `6cae7a8c9` |
| `vevent` showed players fleet slots ("Entered Sea Wren (slot 13)", "slot 16 Wren Skiff", "Ghost contact: slot 33") | Entry, roster and ghost contacts show the contact ID; DB-backed test | `32e11bbfb` |
| Every hull's room line read "is moored here", at sea and sinking too | Look builds the line as she lies: moored, at anchor, sinking, hovering overhead, or here; the lookout gate's open-water hull reads "is here" | `4d4607aec` |
| `shiplist` cut "Magical Vessel" to "Magical Ve" and names to 25 letters | Class 14 and Name 30 wide | `01c79ad70` |
| Customs could seize at 0-4% though help promised never below 5 (Duris floors only a negative chance, against its own comment) | Never under 5 | `aa03131db` |
| Help said otherwise than the code: SEASTATE (grounding), VESSELS (a pilotless unowned hull answers anyone), SETSCHEDULE, ASSIGNPILOT (engages only with the autopilot off), SHIPRAM (the slew rule), SHIPFIRE (in port, stunned, after a ram), SHIPFIX (weapons, sinking) | Corrected in both places | `fe8d054b8` |
| `VESSEL_SYSTEM.md`'s vehicle table, terrain flags and speeds, `unloadvehicle` syntax, and a workflow that mounts a vehicle before loading it | Corrected to the code | `2f1adf001` |
| A schedule's next departure was an hour of the day: one set at hour 20 for every 6 hours read hour 2 as passed and departed again at once, and an interval of 24 departed every hour | An absolute MUD hour (`schedule_mud_hour()`); a row saved in the old form departs once | `f51c34b55` |
| `autopilot off` under an NPC pilot printed "Autopilot disengaged." and the pilot engaged her again half a second later | Refused with the orders that hold, `autopilot pause` and `unassignpilot`; AUTOPILOT help | `44bf37eaa` |
| Any captain could delete any waypoint or route, the public ferry's loop included, which stopped her departures | `delwaypoint` keeps a waypoint a route uses, `delroute` a route a hull runs on a schedule or is sailing; help; ownership is work item #11 | `523417bbd`, `05a5ead8a` |
| `board_hostile` and `dock` took the first hull anywhere whose name had the word, so a namesake out of sight answered "too far away" | They resolve contacts as `shiplock` does; `find_ship_by_name()`, left without a caller, is removed | `2d2ac9b8b` |

Verification (2026-10-02):

- `make test-all` with the database cases (the `luminari-vessels-testdb` container; environment as
  in S9's `testenv.sh`) on the final code, `05a5ead8a`: 1,994 CuTest cases OK (seed 1) and the
  protocol harness's 32. The first run, on `ea5a1da5c`, passed 1,991.
- Help: on a fresh reload of the development dump, `help_vessel_entries.sql` applied and
  `verify_help_vessel_entries.sql` passed all seven checks (34 entries, 91 command keywords, 52
  content contracts), before and after the second round of fixes.
- Live gates in the namespace harness (`/tmp/claude-1000/vs4`, batch jobs `w*` and `u*`), on the
  installed build. On `ea5a1da5c` all 20 passed: merchant 36 s, campaign 128 s, Vailand merchant
  148 s, builder 49 s, gunnery 71 s, tactical 359 s (with the open-water hull now reading "Azure
  Watch is here"), lookout 25 s, boarding 49 s, narrative 23 s, rules 35 s, events 96 s,
  movement 104 s, loss 77 s, damage 603 s, derelict 33 s, hunter 91 s, frontier 217 s, raider
  179 s, economy 257 s, client 26 s. On the final code, `05a5ead8a` (batches `u*` and `t*`),
  all 20 passed again: merchant 37 s, campaign 134 s, Vailand merchant 145 s, builder 48 s,
  gunnery 71 s, tactical 279 s, lookout 22 s, boarding 48 s, narrative 21 s, rules 35 s, events
  42 s, movement 104 s, loss 75 s, damage 597 s, derelict 31 s, hunter 97 s, frontier 222 s,
  raider 157 s, economy 271 s, client 26 s. Twelve of them first refused to start ("source
  worktree must be clean": an uncommitted edit to this record was in the tree) and passed when
  rerun on a fresh reload with the tree clean. The gates refuse a dirty worktree, so no edit may
  sit uncommitted while a batch runs.
- The local CI matrix (`scripts/ci/local/run.py --base gitlab/master`, 33 jobs) on `ea5a1da5c`
  passed 32; clang-tidy's analyzer flagged a second `getenv()` in the event test as a possible
  null (`48e0a141b` reads it once; the job then passed alone). On the final code, `05a5ead8a`,
  all 33 passed in 581 s.

Cleanup: the harness is stopped (its disposable database stops with its namespace); S10 created
no characters or pfiles, ran no autorun, and left no scratch files in the worktree.

Hand-off: tag `vessels-s10` and MR !16 from `feat/vessels-s10` (range
`vessels-s10-base..vessels-s10`). Review fixes go on top, one commit each. After the merge:

- Sync the help to production. Eleven entries changed (by first keyword): ANCHOR (the vessel
  commands), ASSIGNPILOT, AUTOPILOT, BATTLE-STATIONS (SHIPFIRE and SHIPRAM), DELROUTE,
  DELWAYPOINT, LOADVEH (the transport commands, TENTER), SEA-STATE, SETSCHEDULE, SHIP-ADMIN
  (SHIPFIX) and UNASSIGNPILOT.
- The code fixes go with the next production deploy. A production schedule row saved as an hour
  of the day reads as overdue and departs once after that deploy.
- Then close the study's documents: S10 is the last step of Part 5.

## Estimate (remaining)

| Step | What drives the size | Days |
| -- | -- | -: |
| S10 Player guide | Review of MR !16 and the merge; nothing else remains in Part 5 | - |

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
