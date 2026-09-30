# vessels-ships.md improvement pass based on duris code

The goal is to improve the vessel/ship system implementation BASED on DurisMUD system.

This is the entry point: where the work stands, how a step is worked, the rules and decisions that
still bind, the design values not yet built, the active step's record, and what remains. The
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
| S6 NPC raiders and AI | In review: MR !11 from `feat/vessels-s6`, tag `vessels-s6` = `32f513ab3` | [Phase 6 below](#phase-6-s6-progress) |
| S7 Rewards and economy | Not started | [Still to build](#design-values-still-to-build-s7-s8) |
| S8 Client data | Not started | [Still to build](#design-values-still-to-build-s7-s8) |

Production help is current through S5 (help sync plan `ac945fec9d52`, 2026-09-30). Next: answer the
MR !11 review, merge S6, sync its help, then start S7 from the S6 merge.

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

1. S1 Defects that do not wait for the redesign: L4 weather bands, L5 gunnery authorization and
   owner consent, L7 miss lag, L8 contact list and IDs, L9 port immunity, L11 `for_sale`,
   `min_level`, departure level check and the ownership cap (D5), L12 wage removal (D4), L13
   bounty pay-off and decay.
2. S2 Movement and pacing (D1): fractional movement with per-cell validation, class accel and
   turn with the sailmaster and rudder factors, the load and sail limits on maximum speed,
   `setsail` as the maneuver command, `undock` departure and `anchor`, SEADOG +1 speed, the class
   table's speeds, and every automated mover rebased. Ferry soak and scale benchmark are
   re-baselined in `docs/testing/VESSEL_BENCHMARKS.md`.
3. S3 Damage model: Duris arcs and armor profiles with the 229 armor limit, sail HP, breach
   states and sink timers, criticals, weapon damage, knockdown saves, cargo spill and `salvage`,
   and the D6 capture, plunder and boarding rules with `strikecolors`; also the 3.3.1 refit
   rescaling (plating and reinforcement +20%, rigging +10% speed), moved here from S2.
4. S4 Weapons and gunnery: the weapon and equipment tables, 16 slots, fitting, ammo and resupply
   commands, `lock`, battle stations, arc fire, the geometry DC hit model, `sight`, `scan`,
   crew-stun and flight rules; the duel harness moved to the D2 bounds; and, with battle
   stations, the 3.3.2 crash check for refused rooms and shallows, moved here from S2.
5. S5 Crew, repair and loss: one-time hire prices and gates, crew experience, promotion and
   casualties, stamina, the repair stock and dock repairs, the D3 wreck registry with automatic
   insurance, `shipsummon`, trade-in and the rename fee.
6. S6 NPC raiders and AI: the hunter lifecycle generalized into the raider tiers with the 3.3.8
   cadence, fit-outs from prototypes and weapon rows, basic and advanced AI, NPC boarding and
   looting, despawn rules, ramming with the ram, and neutral colors.
7. S7 Rewards and economy: renown, salvage and bounty payouts, the renown board, Ship Damage
   Control, contraband and customs, and the trade modifiers.
8. S8 Client: the MSDP additions.

## Design values still to build (S7, S8)

From 3.3.7 (the replacement hull, automatic insurance, and trade-in there are built):

- Rewards go to the owners of the victor and of every allied hull in sight whose owner is grouped
  with the victor's owner, split equally, through the same settlement path: salvage
  `(hull price * fraction of armor and internal left + surviving weapons at half price) / 8`, a
  renown bounty of `2.5 gold * target renown` when that renown exceeds 100, and the WANTED or
  HUNTED bounty of the target's owner when the owner was aboard.
- Renown lives on the hull. Sinking a consenting player's hull adds the target's hull weight,
  split among the allied hulls in sight; the loser drops that share, floored at zero. NPC kills
  give crew experience, not renown. `shiprenown` lists the top 10 of a top-20 board.
- Ship Damage Control is an epic feat with 5 ranks: `4 + 4 * rank` percent less hull and sail
  damage from other ships while its holder owns the hull and is aboard (24% at rank 5), minimum 1
  point.

From 3.3.9 (the summons and `scan` there are built):

- Contraband: a flag on `trade_commodities`. Seed three goods: forbidden tomes (190 gold, renown
  150), rare poisons (210, renown 200) and dragon eggs (310, renown 250). Buying needs that renown
  or an able sailmaster and quartermaster; warships cannot buy it, nor can a captain at alignment
  1,000.
- Customs at every lawful port (not pirate coves), per contraband unit not stocked by that port:
  `c = 35 + units / 2 - sqrt(renown) / 5`, raised by `(100 - c) * (1 - load / capacity)`, capped
  at 100, 5 if negative.
- Cargo sales: SEADOG +10%, neutral colors -10%, warships -40%.
- MSDP adds `SHIP_ID`, `SHIP_TARGET`, `SHIP_ARMOR`, `SHIP_INTERNAL` (fore/port/rear/starboard,
  current and maximum), `SHIP_SAIL`, `SHIP_SAIL_MAX`, `SHIP_RUDDER`, `SHIP_RUDDER_MAX`,
  `SHIP_STAMINA`, `SHIP_STAMINA_MAX`, `SHIP_WEAPONS` (slot, name, arc, ammo, ready, damage) and
  `SHIP_CONTACTS` (the contact list fields), keeping the existing variables.

Carried forward to S7 from the built steps:

- Renown gates: able and veteran crew hires (the 3.3.5 table) and capital weapons (3.3.4, renown
  1,600-2,000) wait for renown; until then a capital weapon needs a veteran gunner and only
  promotion brings able or veteran hands to a mortal's hull (Phases 4 and 5). The crew casualty
  share for a player kill (`10 + renown lost / 30` percent, 3.3.5) is the flat 10% until then.
- Raiders: the renown each tier carries (the 3.3.8 table: 150-300, 500-600, 700-1,000,
  2,000-3,000) and the renown term of the tier roll (`random(0, hull weight) + renown`).
- Neutral colors: the -10% cargo sale above; they already keep raiders off (S6).
- Bounties: collection by victors (L13); the pay-off and decay are built.
- The DurisMUD background, in the history:
  [1.9](vessels-ships-history.md#19-sinking-insurance-and-rewards) (rewards, frags, the
  leaderboard), [1.11](vessels-ships-history.md#111-economy) (contraband and customs),
  [1.12](vessels-ships-history.md#112-services-and-information) (the GMCP client data, for S8), and
  [1.13](vessels-ships-history.md#113-derived-balance-anchors) (balance anchors).

## Active step

### Phase 6 (S6) progress

Branch `feat/vessels-s6` from the S5 merge commit `23a0726e4`. The annotated tag `vessels-s6-base`
(pushed) marks that merge commit, so `git log vessels-s6-base..vessels-s6` lists only S6 commits.
Hand-off: annotated tag `vessels-s6` at the head given to review and a GitLab merge request from
`feat/vessels-s6`; review fixes go on top. Scope: 3.3.8 without the neutral-colors sale penalty
and the renown a raider carries (S7), ramming (1.6, the 3.2 checklist row, and the sailmaster's
ram training gain of 3.3.5, moved here from S5), and NPC merchants that run (Part 5, step 6). The
raiders and their AI live in the new `src/vessels/vessels_raiders.c`, ramming in the new
`src/vessels/vessels_ramming.c`.

| Item | State | Where |
| -- | -- | -- |
| Ambush roll: eligible hulls, 1 in 2002 per tick, pirate cove x2, territorial /2, neutral colors /60, ships and transports once per voyage (reset on berthing) | Done | `vessel_raider_ambush_odds()`, `vessel_raider_tick_one()` (the periodic owner event, after the autopilot and hunter ticks); `raided` (runtime), cleared by `vessel_berth()` |
| Tier roll and spawn: `random(0, hull weight)` (renown in S7), the tier's prototype at least as fast as the target's design speed minus 3, sight range plus 10 rooms off her bow within 45 degrees, headed at her at full speed | Done | `vessel_raider_pick_tier()`, `vessel_raider_pick_prototype()`, `vessel_raider_spawn()` through `vessel_spawn_public_from_prototype_at()` |
| Tier content: a tier table in code (crew count, advanced-AI chance, crew tier, one fit-out, chest gold); `vessel_raider_tiers` rows tie prototypes to tiers; zone 700 captains, crew, chest and key | Done | `raider_tiers[]`; Phase 22 SQL; `vessels_raider_content.sql` (six Corsair prototypes, ten tier rows) with rollback and verifier; `lib/world/vessel_raiders/700.mob`, `700.obj`, installed by `provision_vessel_harbor.sh` |
| The captain on the bridge (the NPC pilot), crew mobiles aboard, the chest in the hold with its key on the captain; killing the captain stops the AI and the guns | Done | `vessel_assign_npc_pilot()` (was the hunter's), `vessel_raider_stow_chest()`; a raider without her captain clears her pilot assignment, which silences NPC return fire |
| AI modes: engaging (basic: rank the arcs, turn the best onto the target, open or close to the band; advanced: project both hulls, pick the target's weakest side and a broadside), running (no ammo or a breached arc), cruising, leaving; land braking | Done | `vessel_raider_engage()`, `vessel_raider_basic()`, `vessel_raider_advanced()` (`vessel_project()`, was the gunnery's one-second projection), `vessel_raider_set_course()`; her guns are NPC return fire at her quarry |
| Boarding (target at speed 3 or less, merchant classes, or 0) through the boarding contest; boarders on the bridge and three quarters or half of the rooms; pirates loot the hold and leave, hunters fight on | Done | `vessel_raider_board()`, `vessel_raider_loot()`; `vessel_best_boarding_defender()` shared |
| Despawn 600 ticks after losing the target, 20 more while a player hull is in sight or a player is aboard; never kept: a restored raider is retired at boot; raiders cannot be captured | Done | `vessel_raider_countdown()`, `vessel_raider_boot()`, `vessel_retire_npc_hull()` (was the hunter's), `vessel_raider_handle_sink()`, `do_claimship()` |
| Ramming: `shipram [off]` with a lock at speed 6 or more, the Duris bow cone, closing speed, hull weights, the fitted ram, knockdowns, 100/50-tick cooldowns and a 50-tick gun lock; the AI rams too | Done | `vessel_ram_chance()`, `vessel_ram()`, `vessel_ram_tick_one()` (combat tick), `do_shipram()` in `vessels_ramming.c`; `vessel_reload_tick()` and `vessel_hull_fire_problem()` honor the ram |
| NPC merchants under fire run from the attacker while returning fire | Done | `vessel_raider_merchant_run()` |
| Neutral colors: raiders do not pick the hull, ambushes 60 times rarer | Done | `vessel_raider_ambush_odds()`, `vessel_raider_find_quarry()` (`vessel_equipment_slot()`, now shared) |
| Staff: `vesseldebug raider <tier> [hunter]` forces an ambush on the hull you are aboard | Done | `do_vesseldebug()` |
| Help in both places, `VESSEL_SYSTEM.md`, unit tests, an actual-character raider gate, the existing gates, local CI | Done | SHIPFIRE (SHIPRAM, RAMMING, RAIDERS), SHIPEQUIP, VESSELDEBUG (verifier: 90 keywords, 34 content checks); `test_vessel_raiders.c`; `scripts/vessels/test_vessel_raider_in_game.sh` (tactical harness `--raider`, login helper `--vessel-raider-check`); results below |

Interpretations decided while planning S6:

- The tier values (crew count, advanced-AI chance, crew tier, fit-out, chest gold) are a static
  table in code, as S4 made the weapon catalogue: nothing edits them in play. The data are the
  raider prototypes (ordinary `ship_prototypes` rows, not for sale) and `vessel_raider_tiers`,
  which ties each to one or more tiers, since Duris's hulls recur across tiers with different
  fits: tier 0 the clipper, ketch and caravel (ship class at speeds 26, 23, 20 and beam armor 36,
  50, 66), tier 1 the ketch, caravel and corvette (warship class, 22 and 63), tier 2 the corvette
  and destroyer (19 and 84), tier 3 the destroyer and frigate (17 and 109).
- One fit-out per tier after Duris's typical fits, each weapon skipped where the class cannot
  legally mount it: tier 0 a small catapult fore and a small ballista on each other arc; tier 1 a
  small catapult fore and two medium ballistae a beam; tier 2 two medium catapults fore and two
  large ballistae a beam; tier 3 a large catapult fore, two large ballistae a beam, and a small
  catapult aft. Crews are green, green, able and veteran (Duris 200, 400, 1,500, 3,000-4,000
  skill), chest gold twice Duris's platinum (800-1,600, 1,200-2,000, 2,400-3,000, 3,000-6,000).
- Merchant classes are the raft, boat, ship and transport (Duris's merchant hulls); the others
  are warship classes. Only surface hulls (Z 0) are ambushed.
- Mobiles are 70020-70023 (captains, tiers 0-3) and 70024-70027 (crew); objects 70020 (the chest)
  and 70021 (its key). Boarders are fresh crew mobiles, as Duris loads its boarding grunts.
- A raider keeps her target within her sight range plus the 10-room spawn margin, so she can
  close from the spawn point. A target in port, submerged, or sinking is lost.
- Both AIs fire through NPC return fire at the target: whatever bears inside its band. Duris's
  advanced fire restraint and multi-target fire are dropped. Its turn braking is dropped too:
  LuminariMUD hulls turn faster at speed (3.3.2), so slowing down would not help.
- The land brake is Duris's at the half-room resolution of the probe: along the new and the
  current heading, land within half a room holds her to speed 1, within a room to 6, within 2
  rooms to 12 (Duris's 5, 20, and 40 times 0.3). A course that meets land within the lookout (2
  rooms engaging, 5 cruising, 10 running) swings 30 degrees at a time, to each side in turn, to
  open water, as Duris's cruise and run do. Duris's path search around land is dropped: the
  swing slides her along a coast toward her quarry.
- Boarding is one attempt per target, as Duris boards a ship once: the raider's captain is the
  attacker in the grapple and crossing contests. A repelled pirate leaves as a successful one does;
  a hunter fights on either way. Looting (Duris's empty mode) is the pirate's boarding step: each
  lot loses a random 40-60% share of what is taken in the transfer and keeps a 40-60% share, and
  the raider's hold takes what fits.
- Ramming speeds are Duris's times 0.3: arming needs speed 6, a ram fails at speed 3 or less, and
  the rammed hull slows to 4. Duris ticks are seconds, so the cooldowns are 100 ticks after a hit
  and 50 after a miss, less 15% per crew mod, and the guns lock for 50 ticks. An armed ram pauses
  reloading, as Duris's does. The crew mods are the tiers (Duris's 0-3).
- A raider without its captain heaves to and runs the despawn countdown.
- A merchant under fire (at battle stations with its attacker in sight) runs from it until the
  crew stands down, then the autopilot takes her back to her route.

Ablation (planning): dropped a tier table in the database (the values are design constants), the
renown a raider carries and the renown in the tier roll (S7), Duris's 45 fit-outs (one per tier),
escorts, the unique ships, jettison under fire, NPC repair and resupply while cruising (raiders
are short-lived), persisted raider state (a restart retires them, as Duris never saves them), and
capture of a raider (it would be retired at the next boot). Kept the land brake (without it a
raider at battle stations crashes on every coast), silencing a captainless raider's guns, the
staff command (an ambush comes once per 17 minutes of sailing), and the player-aboard despawn
check (a despawn would drop boarders into the sea).

Decided while building S6:

- Raiders are persisted as any public hull is (the constructor saves every hull it launches), and
  `vessel_raider_boot()` retires every unowned hull restored from a raider prototype. Keeping them
  out of persistence would have meant guarding every save path, and a crash would still have left
  rows behind for the next hull in the slot.
- A raider whose captain is gone clears her pilot assignment (`pilot_mob_vnum` -1), which is what
  silences NPC return fire. Making return fire look for the pilot on the bridge for every NPC hull
  would have changed merchants, ferries, and hunters too.
- Duris halves crash damage on a bow only when that ship's ram struck; here a ram halves it on
  its own bow whenever it is fitted (the 1.6 wording). The rammer does not move onto the target's
  position, as Duris's does: within a room is close enough to board.
- The raider's captain is the attacker in the boarding contest (the captain's Boarding ability)
  against the best defender aboard (`vessel_best_boarding_defender()`, now shared).
- The advanced brain projects both hulls 6 ticks (Duris's 3 seconds) by sailing copies on their
  orders, as the gunnery DC projects one second, rather than extrapolating the target's last turn.
- Found by the raider gate: a restarted server opens its port before the world has loaded, so the
  harness waits for boot to log the retirement (`31f3045c1`). Mobiles knocked down by a ram stay
  prone until they next fight, as any knocked-down mobile does.
- The development dump's harbor merchant (ship 11) has had her schedule disabled since the S3
  stall, so `provision_vessel_harbor.sh` stops at its NPC merchant check on a fresh reload; the
  raider content is installed before that check.
- Found by the local CI matrix: clang-tidy wanted the land probe counted in whole half-room steps,
  the course swing's and the ram's integer divisions kept out of floating-point expressions, one
  arc-turn branch in the basic brain instead of two, and the raider tick to engage only with a
  quarry in hand; the coverage gate's changed-line floor for `boot_db()` refused the two boot
  calls no test runs, so the raider table and boot retirement run from the hunter lifecycle's
  schema and boot calls (`39d2c4035`).

Verification (2026-09-30): `make test-all` with the database cases on (isolated `.ci-runtime/lib`,
test MariaDB rebuilt from `master_schema.sql` plus every `apply` component, Phase 22 included)
passes 1,955 CuTest cases and the protocol harness. The Phase 22 schema, rollback, and verifier
and the raider content, its rollback, and its verifier apply to the test database, and the content
reapplies to the same six prototypes and ten tier rows. The vessel help verifier passes (90
keywords, 34 content checks). All 18 live gates pass inside the private namespace on the installed
build of `aec02dcaa`: harbor merchant 45 s, campaign 130 s, Vailand merchant 18 s (the campaign and
merchant runs each on a fresh reload of the development dump), builder 45 s, gunnery 72 s,
tactical 283 s, lookout 22 s, boarding 51 s, narrative 24 s, rules 36 s, events 42 s, movement
105 s, loss 76 s, damage 629 s, derelict 33 s, hunter 84 s, frontier 229 s, and the new raider
gate 196 s (with the raider content applied). In the raider gate Kohdee's frigate rams a stopped
warship at 99%; a Corsair raider launched with `vesseldebug raider 0` closes from beyond sight in
under two minutes, rams (and in the earlier runs opened fire first), and grapples, her boarders are
beaten off, and with her captain purged she heaves to; the restart retires her. The raider gate
passes again on the installed build of `465a7c7e4` (133 s). The local CI matrix
(`scripts/ci/local/run.py --base gitlab/master`) passes all 33 jobs on `39d2c4035` (608 s) and
again on `465a7c7e4` (355 s), the head's code after a return-type tidy-up.

## Estimate (remaining)

From the [original estimate](vessels-ships-history.md#original-estimate), in working days of
focused implementation:

| Step | What drives the size | Days |
| -- | -- | -: |
| S7 Rewards and economy | Renown and payouts, contraband and customs, damage-control feat | 2 |
| S8 Client data | MSDP tables and protocol tests | 1 |

S7 and S8 do not depend on S6. The Open player-data balance and human beta gates follow and depend
on player availability, not engineering time.

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
