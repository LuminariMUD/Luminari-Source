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
| S6 NPC raiders and AI | Merged `85914a03d` (MR !11) | [Phase 6](vessels-ships-history.md#phase-6-s6-progress) |
| S7 Rewards and economy | In progress on `feat/vessels-s7` (tag `vessels-s7-base` = `85914a03d`) | [Phase 7](#phase-7-s7-progress) |
| S8 Client data | Not started | [Still to build](#design-values-still-to-build-s7-s8) |

Production help is current through S6 (help sync plan `e067e0f57ed1`, 2026-10-01). S7 is in
progress on `feat/vessels-s7` ([Phase 7](#phase-7-s7-progress)).

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

### Phase 7 (S7) progress

Branch `feat/vessels-s7` from the S6 merge commit `85914a03d`. The annotated tag `vessels-s7-base`
(pushed) marks that merge commit, so `git log vessels-s7-base..vessels-s7` lists only S7 commits.
Hand-off: annotated tag `vessels-s7` at the head given to review and a GitLab merge request from
`feat/vessels-s7`; review fixes go on top. Scope: the S7 design values above (3.3.7 rewards,
renown and Ship Damage Control; 3.3.9 contraband, customs and cargo sales) and the carry-overs
from S4-S6 (renown gates on hires and capital weapons, the renown term in the crew casualty
share, raider renown and the renown term of the tier roll, the neutral-colors sale penalty, and
bounty collection by victors). Renown, the sinking rewards and the `shiprenown` board live in the
new `src/vessels/vessels_rewards.c`; contraband, customs and the sale modifiers join
`vessels_trade.c`.

| Item | State | Where |
| -- | -- | -- |
| Renown on the hull: kept through the wreck registry and trade-in, persisted, shown by `shipcrew`; `shiprenown` lists the ten player hulls with the most | Planned | `renown` on the hull and `ship_runtime_state.renown` (Phase 23); `do_shiprenown()` |
| Sinking rewards: the victor and allied hulls in sight split salvage, the renown bounty and the target owner's WANTED or HUNTED bounty, paid to their owners through the claim queue; renown moves between players' hulls; the crew casualty share for a player kill | Planned | `vessel_settle_sinking()` from `vessel_sink()`; the claim queue in `vessels_upgrades.c`; `vessel_wreck_hull()` |
| Renown gates: able and veteran hires (the 3.3.5 table) and capital weapons (3.3.4, or a veteran gunner) | Planned | `do_shiphire()`, `vessel_buy_weapon()` and the weapon table |
| Raiders carry their tier's renown, and the quarry's renown joins the tier roll | Planned | `raider_tiers[]`, `vessel_raider_spawn()`, `vessel_raider_pick_tier()` |
| Ship Damage Control, an epic feat of 5 ranks | Planned | `FEAT_SHIP_DAMAGE_CONTROL`; `vessel_damage_hull()`, `vessel_damage_sail()` |
| Contraband: the flag and buying renown on `trade_commodities`, three goods each stocked at one port, the buying gates; customs at lawful ports on arrival | Planned | `trade_commodities.contraband_renown` (Phase 23); `vessels_contraband_content.sql`; `do_market()`, `do_cargobuy()`, `vessel_customs_inspection()` from `vessel_update_port_berth()` |
| Cargo sales: SEADOG +10%, neutral colors -10%, warships -40% | Planned | `do_cargosell()` |
| Help in both places, `VESSEL_SYSTEM.md`, unit tests, an actual-character economy gate, the existing gates, local CI | Planned | SHIPRENOWN (new), SHIPHIRE, MARKET, PLUNDER; `test_vessel_rewards.c`; `scripts/vessels/test_vessel_economy_in_game.sh` (tactical harness `--economy`) |

Interpretations decided while planning S7:

- The victor is the hull that sank her, her `last_attacker`, as for crew training and insurance.
  Rewards need a player's hull as victor. Her allies are player's hulls afloat within the sinking
  hull's sight range whose owners are online and in the online victor owner's group. Hulls owned
  by the target's owner never share, and a hull sunk by her own owner's other hull earns nothing:
  "a consenting player's hull" is one sunk by another player, since gunfire and rams against a
  player's hull already demand consent.
- Salvage is `vessel_hull_price()` times the fraction of armor and structure left, plus half the
  price of each weapon not destroyed, divided by 8, for any hull sunk, NPC hulls included (Duris
  `calc_salvage()`). The renown bounty is `2.5 gold * renown` of any hull above 100 renown, so
  raiders pay it. The owner's bounty is her owner's current WANTED or HUNTED bounty (500 gold or
  more after decay) when the owner is aboard at the sinking; it is collected and cleared.
- Each sharing hull's owner gets one settlement, the hull's equal share of the three, through the
  existing claim queue (`vessel_insurance_claims`) with a mail receipt: at once to an online owner,
  at next login otherwise.
- Renown moves only between players' hulls: each sharing hull gains the target's hull weight (the
  class table's) divided among them; the loser drops the whole hull weight, floored at zero, as
  Duris's `ship_loss_on_sink()` does, and her crew's casualty share becomes `10 + weight / 30`
  percent. NPC kills train the crew only.
- `shiprenown` lists the ten player's hulls with the most renown, afloat or stowed, with owner,
  class and renown. Duris keeps a 20-row table to show 10; a sort at display time needs none.
- Hire and capital weapon gates read the hull's renown; staff still hire freely. A capital weapon
  needs her renown to reach its gate or a veteran gunner aboard.
- Raider renown is a random value in the tier's range at spawn (150-300, 500-600, 700-1,000,
  2,000-3,000); the tier roll adds the quarry's renown to `random(0, hull weight)`.
- Ship Damage Control: while the hull's owner is aboard and holds the feat, each blow to her hull
  or sails loses `4 + 4 * rank` percent, the fraction of a point as the chance of one more, never
  below 1 point (Duris `epic_ship_damage_control()`). Duris applies it inside `damage_hull()` and
  `damage_sail()`, groundings included; "from other ships" in the design value marks Duris's line
  against a character's blows, so S7 applies it in `vessel_damage_hull()` and
  `vessel_damage_sail()` to every blow. An epic general feat with no other prerequisite.
- Contraband: `trade_commodities.contraband_renown` above 0 marks a good as contraband and is the
  renown needed to buy it. A port stocks a contraband good when `port_commodities` holds its row;
  only content creates those rows (the first-visit seeding covers lawful goods only). Elsewhere
  the good is not stocked: the market quotes the scarce price (`TRADE_SUPPLY_MIN`) and a sale
  leaves no row behind, so smuggling pays while the source port's own supply throttles it, and
  customs spares units a port stocks (Duris: a port never confiscates its own contraband). The
  content seeds the three goods (forbidden tomes, rare poisons and dragon eggs, at 4, 1 and 10 lbs
  a unit) at three real sea ports; the harbor fixture also stocks forbidden tomes at the Harbor
  Sandbox East Dock for the gate.
- Buying contraband needs the hull's renown or an able (or better) sailmaster and quartermaster;
  warships cannot buy it, nor a buyer at alignment 1,000; staff are exempt.
- Customs runs when an owned hull enters a lawful port (not in pirate-cove waters) from outside
  it, once per arrival: for each contraband lot the port does not stock, each unit is confiscated
  with the 3.3.9 chance, with `units` the lot's size and load over capacity by weight.
- The sale modifiers multiply (Duris): the seller's SEADOG feat x1.1, neutral colors x0.9, a
  warship x0.6, on every sale, contraband included.

Ablation (planning): dropped Duris's 20-row renown table (a sort at display time), a separate
reward queue (the claim queue already settles for online and offline owners), per-port contraband
columns or a home-port column (a stock row is the stocking), persisting demand at non-stocking
ports (the source port's supply already throttles smuggling), the Duris crew-skill thresholds
for contraband (the design value's able crew), a staff command to set renown (the gate earns it
in a fight), the 20-renown epic progress (LuminariMUD has no epic skill track), and fleet-size
rules for docked hulls and sloops (the design value's sight-and-group rule). Kept a new rewards
file (renown, rewards and the board are one unit used by the sink path) and the contraband
content in its own SQL file with rollback and verifier, as S6 kept its raider content.

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
