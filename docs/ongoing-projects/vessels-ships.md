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
| S7 Rewards and economy | Merged `cce9ff323` (MR !12) | [Phase 7](vessels-ships-history.md#phase-7-s7-progress) |
| S8 Client data | Not started: branch `feat/vessels-s8`, tag `vessels-s8-base` = `cce9ff323` | [Still to build](#design-values-still-to-build-s8) |

Production help is current through S7 (help sync plan `83ce5db82aad`, 2026-10-01). Next: S8 on
`feat/vessels-s8`, starting with its plan commit.

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

## Design values still to build (S8)

From 3.3.9:

- MSDP adds `SHIP_ID`, `SHIP_TARGET`, `SHIP_ARMOR`, `SHIP_INTERNAL` (fore/port/rear/starboard,
  current and maximum), `SHIP_SAIL`, `SHIP_SAIL_MAX`, `SHIP_RUDDER`, `SHIP_RUDDER_MAX`,
  `SHIP_STAMINA`, `SHIP_STAMINA_MAX`, `SHIP_WEAPONS` (slot, name, arc, ammo, ready, damage) and
  `SHIP_CONTACTS` (the contact list fields), keeping the existing variables.

The DurisMUD background, in the history:
[1.12](vessels-ships-history.md#112-services-and-information) (the GMCP client data) and
[1.13](vessels-ships-history.md#113-derived-balance-anchors) (balance anchors).

## Active step

None yet. S8 is next: its first commit on `feat/vessels-s8` adds its "Phase 8 (S8) progress"
section here.

## Estimate (remaining)

From the [original estimate](vessels-ships-history.md#original-estimate), in working days of
focused implementation:

| Step | What drives the size | Days |
| -- | -- | -: |
| S8 Client data | MSDP tables and protocol tests | 1 |

S8 does not depend on S7. The Open player-data balance and human beta gates follow and depend
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
