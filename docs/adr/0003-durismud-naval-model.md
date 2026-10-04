# 3. DurisMUD Naval Model for the Vessel System

**Status:** Accepted
**Decision date:** 2026-09-28
**Last reviewed:** 2026-10-04

## Context

The unified vessel system ([ADR 0001](0001-unified-vessel-system.md)) gave LuminariMUD one
transport system on the shared wilderness map. A study of 2026-09-28 compared it with DurisMUD's
ship code (DurisMUD revision `cca1d43d`, LuminariMUD revision `fdc0eabe`). Code, tests, SQL, and
documents cite that study as the "vessels-ships study"; this record keeps its decisions and the
[key to those citations](#key-to-study-citations).

The study found:

- LuminariMUD was ahead on world integration and breadth: 3D wilderness navigation with
  bathymetry and weather, routes, schedules and NPC pilots, regional piracy law, supply-driven
  markets and freight, durable merchant fleets, D20 boarding, showcase events, MSDP, and operator
  tooling.
- DurisMUD was ahead on the naval combat game: momentum sailing with per-class acceleration and
  turn rate, firing arcs over asymmetric armor, a weapon catalogue with range bands and
  ammunition, a hit model driven by range, relative motion, target size and crew, breach states
  with a sink timer, ramming, crew training and stamina, a repair stock, rewards, and NPC raiders
  that maneuver, board and loot.
- LuminariMUD combat was a stand-in over the same frame. Heading and speed changed at once,
  rudder and rigging damage changed nothing (against requirement 4.4, "a disabled subsystem
  changes behavior in an observable way"), the hit roll was the firing character's level against
  the target's speed setting, armor was equal on all sides, and a hull sank the moment its total
  structure reached zero.
- The structural gap was pacing. Autopilot moved a hull `speed` rooms every 0.5-second tick, so
  a 359-room passage took about 15 seconds. At that scale maneuvering, ambushes, storms and
  encounters could not matter, and combat only worked between hulls that had stopped.

## Decision

Rebuild the naval game on DurisMUD's model inside the unified vessel system, and keep every area
where LuminariMUD was ahead.

### Rules

- This is not an exact-parity port. DurisMUD's numbers are the player-tested starting point,
  translated into LuminariMUD's D20 rules, units and economy.
- Where an existing LuminariMUD mechanic already produces a DurisMUD effect, that mechanic
  closes the gap.
- Where LuminariMUD lacks a mechanic, the real mechanic is built, never a cheaper stand-in.
- The design values are implementation targets. `vesseldebug balance` and the human beta tune
  numbers later without reopening the design. For time to kill the tuning lever is weapon reload
  time; armor and damage stay at DurisMUD's values.

### Owner decisions

| ID | Decision |
| -- | -- |
| D1 | Pacing: the DurisMUD scale, `speed / 90` rooms per 0.5-second tick. |
| D2 | Time to kill: a 3-8 minute median for equal warships, a 12-minute p95, nothing under 90 seconds. |
| D3 | Loss: the hull is lost; the ship's identity survives. |
| D4 | Crew economics: one-time hire, no wages. |
| D5 | Ownership: a small configurable cap, 3 hulls per owner by default (`cedit`, 1-10). Public and NPC hulls do not count, and a capture at the cap is refused. |
| D6 | Capture: only disabled prizes: a breached arc, immobile, colors struck, or abandoned at sea. Hostile boarding needs the target at speed 3 or less or disabled. |

### Units and conversions

- Range: both games measure range in map rooms, so DurisMUD ranges transfer unchanged.
- Time: a DurisMUD ship tick is 1 second and a LuminariMUD vessel tick is 0.5 seconds
  (`VESSEL_PERIODIC_FAST_CADENCE`). Durations in seconds double into ticks, and per-tick rates
  halve.
- Speed: DurisMUD speed times 0.3, at most 30; acceleration and turn rate likewise, then halved
  per tick. `speed / 90` rooms per tick is DurisMUD's `speed / 150` rooms per second at that
  scale. The raft's speed 5 and the submarine's 12 are LuminariMUD-only values.
- Price: 2 gold per DurisMUD platinum, from comparing the default hull prices. It is a starting
  conversion, not an economy measurement.
- Level: DurisMUD's hull level times 30/56 (the two games' mortal level caps), checked on the
  captain at departure, not at purchase.
- Class analogs: raft and sloop, boat and yacht, ship and caravel, transport and galleon, warship
  and frigate, airship and corvette, submarine and destroyer (bow and stern mounts only), magical
  hull and cruiser. Each class takes its analog's per-arc armor and structure, sail, mounts,
  and weight budget.
- Hit model: DurisMUD's targeting geometry sets a D20 difficulty class instead of a new table.

### Balance anchors

Derived from DurisMUD's code and tables. They are the priors for the open player-data balance
gate in the
[release gate state](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md#release-gate-state).

- Position and motion matter more than the crew. Against a frigate with a large ballista, a
  stopped target at a third of the range is a near-certain hit for any crew; on parallel
  courses at two thirds of the range an untrained crew hits about 75% of the time and an elite
  one 90%; at maximum range against a moving target, 36% and 72%.
- Time to sink: equal mid-tier warships need 5-15 minutes with one broadside bearing
  throughout, and about 3 for a skilled captain alternating both broadsides at close range.
  Before this decision LuminariMUD's equal-warship duel resolved in 67 seconds.
- Pacing: a hull at speed covers about one weapon range per reload, so each volley follows a
  maneuver decision.
- Costs against the hull price: a combat fit is 35-45%, a full dockside repair 2-3%, a hull
  change credits 90%, and insurance returns 50-90%. A sinking costs the owner a share of the
  investment, never all of it.
- Threat: one raider ambush per about 17 minutes of sailing, and a merchant hull at most once
  per voyage.
- Weapons: the standard weapons deal about 1.5-2.5 hull damage per minute per point of weight
  at 100% hits. The exotic ones buy more with tight range bands, small magazines, the limit of
  one capital weapon per hull, and renown gates.

### Time-to-kill tuning record

`vesseldebug balance` duels two equal warships with DurisMUD's frigate combat fit. Medians over
1,000 duels, by the reload of the standard and the capital weapons:

| Reloads | Median | p95 | Notes |
| -- | -: | -: | -- |
| 30 s and 45 s (DurisMUD's) | 538 s | 725 s | Outside D2 |
| 20 s and 30 s | 434 s | 604 s | Minimum 249 s; steady across five seeds |
| 15 s and 22.5 s | about 10% shorter |  | Maneuvering dominates the time; 2-3% of duels drawn |
| 20 s and 30 s, once crews tire | 521 s | 729 s | Stamina lengthened the duel (200-duel sample) |
| 17 s and 25.5 s, with crew repairs (current) | 437 s | 636 s | Minimum 208 s, 0.7% drawn; 437-443 s on four other seeds |

A duel with no kill in an hour is a draw, allowed up to 2%: stern hits foul the rudder, and a
hull that cannot come about can limp off. Shorter reloads alone pushed draws past 2%; crew
repairs, which mend a rudder at sea, brought them back. The harness captain steers
proportionally around 7.5 rooms off her enemy's beam; a captain that held a 6-9 room band let 6%
of duels separate for good.

## Consequences

### Positive

- Position, range, and firing arcs decide fights, and a damaged sail or rudder changes how a
  hull handles, as requirement 4.4 asks.
- Every automated mover (autopilot, NPC pilots, schedules, merchants, hunters, raiders, and
  ferries) sails through the same physics as a player's hull.
- A lost ship keeps its name, owner, crew and renown, so a sinking is a setback and not a
  deletion.

### Negative

- Voyages take real time: the Vailand Iron Passage went from about 15 seconds to about 23
  minutes. Schedules, soak runs, and benchmarks were re-derived
  ([VESSEL_BENCHMARKS.md](../testing/VESSEL_BENCHMARKS.md#s2-momentum-re-baseline)).
- Every room a hull enters is checked, so a route that clipped a coast corner no longer sails.
  `setschedule` and each scheduled departure validate a route by sailing a copy of the hull over
  it.
- The numbers are DurisMUD's, translated; they are not measured on LuminariMUD players. The
  player-data balance and human beta gates stay open until that evidence exists.

## Scope

What the study took from DurisMUD, what it reshaped, and what it left out.

### Kept

Everything that satisfies requirement 4.4 (range, bearing, arcs, reloads, armor sections,
subsystem damage, repair, sinking, wrecks, boarding, capture, NPC doctrine, observable disabled
subsystems), the DurisMUD features added for parity (stamina, anchor, contraband, damage control,
equipment), and the pacing change without which none of it can matter.

### Simplified

An existing structure carries the DurisMUD mechanic:

- Continuous crew skills became experience on the existing four positions and three tiers, and
  DurisMUD's chiefs are those positions.
- NPC raiders extend the bounty-hunter lifecycle instead of a new spawner.
- The hit model reuses DurisMUD's geometry as a D20 difficulty class.
- Volley flight time is omitted. DurisMUD freezes the hit chance at firing, so resolving at once
  changes only the delay.

### Dropped

DurisMUD-specific, or already covered by a LuminariMUD mechanic:

- Racewar ocean-PvP state and `signal` (PvP consent and `shiptalk` cover them).
- The Redis and flat-file backends, and runtime identity references (generation-aware events
  cover them).
- DurisMUD's ferries and autopilot (schedules and routes cover them).
- The levistone (airships cover flight).
- Delayed market prices (per-unit batch pricing already stops quick flips).
- Ship coffers (the settlement path pays owners).
- The CTF speed penalty, the Trader achievement, the Sailor's Tattoo, and the unique Cyric's
  Revenge and automatons quest content.
- From DurisMUD's raiders: advanced fire restraint and multi-target fire, turn braking
  (LuminariMUD hulls turn faster at speed, so slowing would not help), the path search around
  land (a raider swings 30 degrees at a time toward open water instead), the 45 fit-outs (one
  per tier), escorts, the unique ships, jettison under fire, repair and resupply while cruising
  (raiders are short-lived), and capture of a raider (a restart retires every raider).
- From DurisMUD's rewards: the 20-row renown table (a sort at display time), the 20-renown epic
  progress (LuminariMUD has no epic skill track), a staff command to set renown, crew-skill
  thresholds for contraband (an able sailmaster and quartermaster instead), and demand kept at
  ports that do not stock a contraband good (the source port's supply already throttles
  smuggling).
- From DurisMUD's client data: per-contact heading, speed and status flags, full magazines,
  equipment, cargo and the crew sheet, and a GMCP ship package (MSDP carries the vessel
  variables, and a GMCP client receives them in the `MSDP` package).

## Key to Study Citations

A citation reads "vessels-ships study" or "study", then a label. The study itself is archived
(see References); the built values are in the documents below, which stay authoritative where
they differ from the study's targets.

| Label | Subject | Current reference |
| -- | -- | -- |
| 1.1-1.13 | How DurisMUD does it; 1.6 is ramming, crew-stun weapons and flight, and 1.13 the derived balance anchors (1.13d the frigate combat fit) | The DurisMUD sources in References; [Balance anchors](#balance-anchors) |
| 3.3.1 | Hull classes: analogs, handling, armor profiles, mounts, prices, levels, insurance shares, refits | [VESSEL_SYSTEM.md](../systems/VESSEL_SYSTEM.md): Movement and Pacing, Damage Model (S3), Weapons and Gunnery (S4), Ownership & Shipyard Commands |
| 3.3.2 | Movement at D1's pacing | VESSEL_SYSTEM.md: Movement and Pacing |
| 3.3.3 | Damage: arcs, hit resolution, knockdown, breaches, sinking, salvage | VESSEL_SYSTEM.md: Damage Model (S3) |
| 3.3.4 | Weapons, fitting and gunnery: the hit difficulty class, criticals, flight rules (one room per 10 Z), crew stun, D2 | VESSEL_SYSTEM.md: Weapons and Gunnery (S4) |
| 3.3.5 | Crew: hire prices, experience, casualties, stamina | VESSEL_SYSTEM.md: Crew, Repair and Loss (S5) |
| 3.3.6 | Repair: stores, crew and character repairs, dock repairs | VESSEL_SYSTEM.md: Crew, Repair and Loss (S5) |
| 3.3.7 | Loss, insurance and renown; Ship Damage Control; trade-in and the rename fee | VESSEL_SYSTEM.md: Crew, Repair and Loss (S5); Rewards, Renown and Contraband (S7) |
| 3.3.8 | NPC raiders: ambush odds, tiers, AI, boarding, neutral colors | VESSEL_SYSTEM.md: Raiders and Ramming (S6) |
| 3.3.9 | Economy, services and client data: contraband, customs, sale modifiers, summons, scan, MSDP | VESSEL_SYSTEM.md: Rewards, Renown and Contraband (S7); [MSDP_VARIABLES.md](../systems/MSDP_VARIABLES.md) |
| 3.3.10 | Migration of prototypes and live hulls to the new model | [VESSEL_SCHEMA_DEPLOYMENT.md](../deployment/VESSEL_SCHEMA_DEPLOYMENT.md); VESSEL_SYSTEM.md: Damage Model (S3) |
| D1-D6 | Owner decisions | [Owner decisions](#owner-decisions) |
| L1-L14 | The study's fourteen defects in the baseline code, each fixed by the step that rebuilt its area. Cited outside the study: L9, no safe harbor and no combat lockout, and L10, free unlimited repairs and refits that repaired | VESSEL_SYSTEM.md |
| S1-S15 | The implementation steps | [Implementation steps](#implementation-steps) |

### Implementation steps

Each step merged with a merge commit, so the hashes its records cite stay reachable. The merge
request holds each step's review.

| Step | Scope | Merge | Merge request |
| -- | -- | -- | -- |
| S1 | Defect fixes that did not wait for the redesign: weather bands, gunnery authorization and owner consent, miss lag, contact list and IDs, port immunity, shipyard listing and levels, the ownership cap, wage removal, bounty pay-off and decay | `34dcbb34e` | !6 |
| S2 | Movement and pacing (D1) | `89cfabcbe` | !7 |
| S3 | Damage model, and the D6 capture, plunder and boarding rules | `a85e97d9f` | !8 |
| S4 | Weapons and gunnery | `c8bab4576` | !9 |
| S5 | Crew, repair and loss (D3, D4) | `23a0726e4` | !10 |
| S6 | NPC raiders, raider AI, ramming | `85914a03d` | !11 |
| S7 | Rewards and economy: renown, salvage and bounty payouts, Ship Damage Control, contraband and customs | `cce9ff323` | !12 |
| S8 | Client data: the MSDP additions | `2a4815b1a` | !13 |
| S-immediate | The Luminari Web client readied for S9 | `1c7e4bffb` | !14 |
| S9 | The whole system played in game through a real client; every defect found fixed | `a6adb46a8` | !15 |
| S10 | The [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md) | `60ef66ad1` | !16 |
| S11 | Checked cargo trades (work item #10) | `f18208549` | !17 |
| S12 | Owned waypoints and routes (work item #11) | `e6881c1d1`, `bde4a0f44` | !18, !19 |
| S13 | Database transactions that survive a lost connection (work item #13) | `9eb723913` | !20 |
| S14 | Two-phase vessel settlements (work item #12) | `e7bc5242c` | !21 |
| S15 | Checked vessel purchases and payouts (work item #14) | `1ab373f02` | !22 |

## Validation

The decision remains valid when:

- `vesseldebug balance` reports the equal-warship duel inside D2's bounds.
- A hull with a damaged sail, rudder, or breached arc handles differently from a sound one.
- Automated movers and player hulls move through one movement tick.
- The production-linked vessel tests and the actual-character gates in `scripts/vessels/` pass.

## References

- The archived study, at commit `d5cd6a735e1ba7cf9a9422c3467bf6ae0ed10cb3`:
  [vessels-ships.md](https://gitlab.com/max757/Luminari-Source/-/blob/d5cd6a735e1ba7cf9a9422c3467bf6ae0ed10cb3/docs/ongoing-projects/vessels-ships.md)
  (decisions and sequence),
  [vessels-ships-history.md](https://gitlab.com/max757/Luminari-Source/-/blob/d5cd6a735e1ba7cf9a9422c3467bf6ae0ed10cb3/docs/ongoing-projects/vessels-ships-history.md)
  (Parts 0-4 and each step's record), and
  [guide-notes.md](https://gitlab.com/max757/Luminari-Source/-/blob/d5cd6a735e1ba7cf9a9422c3467bf6ae0ed10cb3/docs/ongoing-projects/guide-notes.md)
  (the S9 play record).
- DurisMUD sources at revision `cca1d43d`: `docs/reference/SHIPS.md`,
  `docs/reference/SHIP_GAMEPLAY.md`, `lib/information/helpships`, and `src/ships/` (`ships.h`
  holds the module map and each `.c` file an `OVERVIEW` block).
- [Vessel system behavior reference](../systems/VESSEL_SYSTEM.md)
- [Vessel system product requirements](../product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md)
- [Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md)
- [ADR 0001: Unified Vessel System Architecture](0001-unified-vessel-system.md)
