# Vessel System - Manual Testing Guide

Numbered manual regression script for the Vessel System. Run on dev after any
vessel-related change. Every step lists the expected result; any deviation is a
regression. The durable quality gate is in [PRD.md](../PRD.md); unresolved
findings are tracked in
[VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md).

**Current run status (July 29, 2026): all 30 steps pass on local development.**
The legacy identity, generated-room insertion, sailing, route persistence, and
vehicle transport defects found during the run were repaired and retested with
Kohdee. Cleanup left no test vehicles, prototypes, routes, waypoints, or
runtime ship-instance rows in the local database.

Prerequisites: staff character (LVL_BUILDER+), MySQL running, server booted
with vessel commands and ticks enabled. The cedit
`CONFIG_VESSEL_SYSTEM` setting is a load-bearing kill switch: keep it `On` for
the numbered regression. Setting it `Off` blocks vessel command dispatch and
both heartbeat tick groups while retaining staff recovery commands.

## A. Legacy world-file vessel (zone 700 test object)

Intended fixtures: object 70002 (test vessel, ITEM_GREYHAWK_SHIP), room 70003
(interior), room 1000389 (Testing Dock, wilderness (-66, 92)). Before running,
verify that the object's ship index, the initialized fleet slot, the `shipnum`
field, `shiproom`, and `world[room].ship` all identify the same ship, and that
`shiplist` reports aggregate hull structure as 240/240.

1. `goto -66 92` - you arrive at "Testing Dock", room shows `[ Dockable ]`
   and coordinates (-66, 92); the test vessel object is in the room. Do not
   use dynamic wilderness room VNUM 1000389 after a reboot; it may not be
   allocated yet.
2. `board` - "You board the ship."; you are in room 70003, flagged
   `[ Vehicle ]`.
3. `disembark` - you return to Testing Dock at (-66, 92) (regression check for
   the `IN_ROOM(shipobj)` exit-point fix; must NOT print "Unable to find a
   valid exit point"). The dynamic wilderness room VNUM is not stable across
   reboots.

## B. Prototype editor (vedit)

4. `vedit` - usage text lists list/new/show/set/delete/spawn.
5. `vedit new 2 The Gull` - "Created Ship prototype N: 'The Gull'
   (speed 15, armor 20)."
6. `vedit list` - table includes prototype N "The Gull", class Ship.
7. `vedit set N speed 12` - "Prototype N updated: speed = 12."
8. `vedit set N armor 999` - rejected: "Armor must be 0-100."
9. `vedit show N` - shows class Ship, speed 12, armor 20->per set, cargo
   12000 lbs.

## C. Spawn and sail

10. `goto -66 92`, then `vedit spawn N` - "Spawned 'The Gull' (Ship) as
    ship S: R interior rooms, entrance E, bridge B." A Ship has at least four
    rooms and may gain discovery rooms; object "The Gull" is moored in the
    room.
11. `board gull` - you enter the generated entrance room. Its name and
    description match the active `ship_room_templates` database row (the
    default is "Crew Quarters"); edited database text appears instead of the
    compiled-in fallback.
12. Move through every listed interior exit, including diagonals when
    discovery rooms exist - all generated rooms connect and every spoke
    returns to the bridge; no dead end traps you.
13. From the bridge: `speed 5`, then `heading 90` - status output reflects
    the values; `shipstatus` shows position (-66, 92).
14. `setsail west` - the ship moves from (-66, 92) to (-67, 92);
    `shipstatus` reflects the new coordinates. An explicit debug build shows
    `[VESSEL_MOVE]` lines only after `vdebug on move`. East of Testing Dock is
    land and must remain non-navigable to this hull.
15. `speed 0`, `disembark` - you exit to the wilderness room at the ship's
    current coordinates.

## D. Autopilot round-trip

16. On the bridge: `setwaypoint dockpoint` - waypoint created at current
    position.
17. Sail west a few steps (repeat step 14), then `setwaypoint westpoint`.
18. `createroute testrun`, `addtoroute testrun dockpoint`,
    `addtoroute testrun westpoint` - both additions confirmed.
19. `setroute testrun`, `autopilot on` - ship begins moving toward the
    first waypoint. In an explicit debug build, `vdebug on auto` enables
    `[VESSEL_AUTO]` lines in syslog.
20. `autopilot status` - shows TRAVELING with waypoint index.
21. `autopilot off` - ship stops; state OFF.
22. Reboot the server (copyover or full restart). `listroutes` - route
    "testrun" persists with both waypoints (route DB round-trip).

## E. Vehicles and vehicle-in-vessel

23. At Testing Dock: `vehiclecreate cart regression_cart`,
    `vmount regression_cart`, `drive east` - the cart and rider move to
    (-65, 92), and `vstatus` agrees. `drive west` returns both to the dock;
    `vdismount`. The other adjacent cells are water and are not a valid cart
    movement test.
24. With The Gull present at the dock and speed 0: board it, move to the
    bridge, then `loadvehicle regression_cart` - "You load regression_cart
    onto The Gull." `unloadvehicle` lists it by name.
25. At the dock, create six wagons named `heavy_1` through `heavy_6`. Load
    `heavy_1` through `heavy_5`. With the 500-lb cart already aboard, those
    five 2,000-lb wagons use 10,500 of the Ship class's 12,000-lb capacity.
    `loadvehicle heavy_6` is refused with "The vessel cannot carry any more
    vehicles." This is the actual boundary; a single second wagon does not
    exceed this hull's capacity.
26. On the bridge, `speed 5`, then `unloadvehicle 1` - refused because the
    vessel is moving. `setsail west`, `shipstatus` - position becomes
    (-67, 92), and loaded vehicle coordinates follow it. `speed 0`, then
    `unloadvehicle 1` - refused because shallow water is unsuitable for a
    cart. Return with `speed 5`, `setsail east`, `speed 0`; at the seaport,
    `unloadvehicle 1` succeeds at (-66, 92).

## F. Docking and boarding defense smoke

27. Create/spawn a distinctly named second prototype, such as "The Tern", in
    the same wilderness cell. From The Gull's bridge: `dock tern` - docking
    completes. Move to the entrance and traverse the temporary gangway in both
    directions; `undock` removes that exit while leaving both hull objects and
    coordinates together until one sails away.
28. (Hostile path, staff-only smoke) From The Gull after undocking:
    `board_hostile tern` - on success, the character enters The Tern and sees
    the warning and "BATTLE STATIONS!" broadcast. On a failed roll, the
    character stays aboard their ship; on a critical failure, they fall into
    its exterior wilderness room and receive a Swim (Athletics) check. On
    defended ships, idle crew NPCs reposition to the entrance/bridge.

## G. Cleanup

29. Use `shippurge <slot>` on each prototype-spawned vessel. `shiplist` drops
    the slot, its hull disappears, its generated room VNUMs no longer resolve,
    and its loaded vehicles are released beside the hull. Respawn once into
    the same slot to prove the room VNUM range can be reused without reboot,
    then purge it again.
30. Purge each test vehicle with `vehiclepurge <vehicle-id>`. Delete the test
    prototypes with `vedit delete <id>` and verify `vedit list` is empty.
    Delete test navigation data with `delroute testrun`, then
    `delwaypoint dockpoint` and `delwaypoint westpoint`; both list commands
    report no remaining test data.

## Release-Boundary Evidence

The July 29, 2026 local-development run also proved the boundaries around the
numbered gameplay flow:

- With a real autopilot route active, cedit `Off` held Test Vessel at
  `(-70, 92)` across two separate Kohdee sessions. `autopilot status` and
  `speed` were refused, while `look` and recovery `shiplist` remained
  available. Cedit `On` restored tick processing and persisted to
  `lib/etc/config`.
- An explicit `-DVESSEL_SYSTEM_DEBUG=1` development build enabled only the
  requested `move` category at runtime and produced `[VESSEL_MOVE]` diagnostics
  during a Kohdee sailing test. The clean default build then refused
  `vdebug on move` and reported debug support `compiled out`.
- `verify_help_vessel_entries.sql` passed 31 maintained entries, 75 exact
  command keywords, access levels, nonempty content, and zero obsolete
  duplicates. `--vessel-help-check` found a database `Help Tag` for all 75
  commands in one 54-second Kohdee login.
- All 21 component migrations classified as current by
  `ci_schema_manifest.txt` applied independently to a fresh MariaDB 10.11
  master schema.
- The Phase 09 runtime migration and verifier passed against local MariaDB.
  `ship_runtime_state` held the expected parent-linked live snapshots and
  `ship_schedules` held the scheduled route.
- The Phase 10 verifier found both lifecycle tables, all five runtime columns,
  four normalized installed-weapon rows, no orphan or invalid weapons, no
  invalid insurance claims, and no invalid dock-fee state.
- A graceful full restart reconstructed two prototype-spawned hull objects and
  their 7-room and 6-room dynamic interiors. The transport retained Kohdee as
  owner, 400 pounds of timber, able sailmaster and green quartermaster, hull
  reinforcement, 20,000 gold insurance, and starboard armor damage at 6/20.
  The warship retained position `(-64, 85)`, heading 345, port armor damage at
  36/40, paused route progress, and its 24-hour schedule.
- `--copyover-check` kept Kohdee's live descriptor across a real process
  replacement. In the stronger active-voyage run, the warship was Traveling
  toward waypoint 1 at `(-63, 81)` before copyover and recovered Traveling on
  the same route, advancing to `(-62, 82)`. The transport's ownership, combat
  link, cargo, crew, refit, insurance, and damage remained intact.
- Kohdee sailed the owned transport into Testing Dock and received one
  35-gold fee. `shipstatus` and `dockfees` reported the debt, departure was
  refused, and the debt survived copyover and a full service restart. Kohdee
  then paid 35 gold, departed, returned, received exactly one new fee for the
  new visit, paid it, and departed again. The final balance was clear. These
  runs also exposed and fixed stale dynamic-room port identity after recovery.
- The level-1 Veska bought a 50-gold policy for 10 gold, verified a 9,990-gold
  balance, and logged out as owner of the insured raft. Kohdee then sank it
  with `shipfire`. Before Veska returned, one pending claim and one underwriter
  receipt mail existed. Her next actual login showed 10,040 gold and changed
  the claim to paid with player-file high-water mark `VIns: 1`; a second login
  remained at 10,040 with no duplicate claim or payment.
- Corven owned a raft and held a helm permit on Kohdee's transport. Setting the
  reversible deleted flag blocked Corven's login while preserving both
  relationships; clearing it restored play aboard the same raft. Corven then
  used the actual character-menu password and `yes` flow with fast wipe
  enabled. The player file and `player_data` row disappeared, the raft became
  unclaimed, the permit disappeared, and a controlled pending claim changed to
  `void`. SQL plus live `shiplist` and `shipcrew` agreed. Database-failure
  injection remains to prove the removal defer path.
- The full production-linked root suite passed 218 tests, followed by
  `make install`; no root-level `circle` artifact remained.

## Known Findings

- Legacy ship slot, `shipnum`, object value, and interior room mismatch -
  FIXED and live-tested.
- Runtime-generated interior rooms are inserted in sorted VNUM order and
  reindex live characters, objects, exits, and vehicle room references -
  FIXED and live-tested.
- Vehicle loading now finds the named vehicle beside the exterior hull,
  persists its parent vessel, follows ship coordinates, and unloads beside
  the hull only on compatible terrain - FIXED and live-tested.
- Dock targets resolve by displayed ID, fleet ID, exact name, or name keyword;
  docked gangways are traversable only between mutually docked ships, and
  undocking preserves coherent exterior positions - FIXED and live-tested.
- `shippurge` transactionally removes ship-instance persistence, evacuates
  occupants and loose objects, releases carried vehicles, removes the hull,
  and immediately reclaims generated interior rooms. Same-slot respawn works
  without reboot - FIXED and live-tested.
- Legacy and builder-spawned hulls now share complete condition
  initialization. Test Vessel reports 240/240 internal structure and no longer
  sinks when armor absorbs its first weather hit - FIXED and live-tested.
- Boot now relinks zone-reset hull objects to active fleet slots. Kohdee logged
  out inside room 70003, the server fully restarted, and `disembark` then
  returned to Testing Dock without requiring another `board` command - FIXED
  and live-tested.
- Prototype-spawned vessels now save complete runtime snapshots before full
  shutdown and before copyover descriptor handoff. Boot reconstructs dynamic
  interiors and hull objects, then restores position, condition, combat,
  weapon-slot, autopilot, schedule, owner, cargo, crew, upgrade, and insurance
  state - FIXED and live-tested through both lifecycle paths.
- Owned hulls are charged once per port visit, retain unpaid dock fees through
  copyover and restart, cannot depart or resume autopilot while indebted, and
  can settle safely after dynamic wilderness rooms are recycled - FIXED and
  live-tested.
- An insured loss queues exactly one claim and underwriter receipt while its
  owner is offline, then credits the owner once on login without duplication on
  a later login - FIXED and live-tested with Veska and Kohdee.
- Reversible character deletion preserves ship relationships and restoration;
  normal permanent removal transactionally unowns ships, removes the deleted
  name from permits, and voids pending claims - FIXED and live-tested with
  Corven. Transaction-failure recovery remains to be injected.

# EoF
