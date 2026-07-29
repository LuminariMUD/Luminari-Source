# Vessel System - Manual Testing Guide

Numbered manual regression script for the Vessel System. Run on dev after any
vessel-related change. Every step lists the expected result; any deviation is a
regression. The durable quality gate is in [PRD.md](../PRD.md); unresolved
findings are tracked in
[VESSELS_TODO.md](../project-management-zusuk/vessels/VESSELS_TODO.md).

**Current run status (July 26, 2026): BLOCKED at step 3.** Steps 1 and 2 pass.
The legacy object points to fleet slot 0, while that ship's `shipnum` field is
1; the initialization also binds room 1403 while the object boards into room
70003. Disembark and helm commands therefore read the wrong ship state. Fix the
fixture and canonicalize ship identity before treating this script as passing.

Prerequisites: staff character (LVL_BUILDER+), MySQL running, server booted
with vessel commands and ticks enabled. The current cedit
`CONFIG_VESSEL_SYSTEM` setting is not yet a complete kill switch; see the
outstanding-work document before relying on it operationally.

## A. Legacy world-file vessel (zone 700 test object)

Intended fixtures: object 70002 (test vessel, ITEM_GREYHAWK_SHIP), room 70003
(interior), room 1000389 (Testing Dock, wilderness (-66, 92)). Before running,
verify that the object's ship index, the initialized fleet slot, the `shipnum`
field, `shiproom`, and `world[room].ship` all identify the same ship.

1. `goto 1000389` - you arrive at "Testing Dock", room shows `[ Dockable ]`
   and coordinates (-66, 92); the test vessel object is in the room.
2. `board` - "You board the ship."; you are in room 70003, flagged
   `[ Vehicle ]`.
3. `disembark` - you return to room 1000389 (regression check for the
   IN_ROOM(shipobj) exit-point fix; must NOT print "Unable to find a valid
   exit point").

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

10. `goto 1000389`, then `vedit spawn N` - "Spawned 'The Gull' (Ship) as
    ship S: 4 interior rooms, entrance E, bridge B." Object "The Gull" is
    moored in the room.
11. `board gull` - you enter the generated entrance room; room name contains
    "The Gull" (data-driven template check: if the ship_room_templates DB
    rows were edited, the edited text appears here instead of the
    compiled-in fallback).
12. Move through the interior (n/s/e/w) - all generated rooms connect; no
    dead ends that trap you.
13. From the bridge: `speed 5`, then `heading 90` - status output reflects
    the values; `shipstatus` shows position (-66, 92).
14. `setsail east` (or move command per current helm binding) - ship
    coordinates change on `shipstatus`; syslog shows `[VESSEL_MOVE]` lines
    when VESSEL_SYSTEM_DEBUG is 1.
15. `speed 0`, `disembark` - you exit to the wilderness room at the ship's
    current coordinates.

## D. Autopilot round-trip

16. On the bridge: `setwaypoint dockpoint` - waypoint created at current
    position.
17. Sail east a few steps (repeat step 14), then `setwaypoint eastpoint`.
18. `createroute testrun`, `addtoroute testrun dockpoint`,
    `addtoroute testrun eastpoint` - both additions confirmed.
19. `setroute testrun`, `autopilot on` - ship begins moving toward the
    first waypoint; `[VESSEL_AUTO]` lines appear in syslog.
20. `autopilot status` - shows TRAVELING with waypoint index.
21. `autopilot off` - ship stops; state OFF.
22. Reboot the server (copyover or full restart). `listroutes` - route
    "testrun" persists with both waypoints (route DB round-trip).

## E. Vehicles and vehicle-in-vessel

23. In a wilderness room: create/locate a test cart, `vmount cart`,
    `drive north` - vehicle moves, `vstatus` shows new coordinates.
24. `vdismount`. At the dock with the spawned ship present and speed 0:
    `loadvehicle cart` - "You load ... onto ..." (capacity permitting).
25. Attempt `loadvehicle` of a second heavy vehicle exceeding the class
    cargo capacity - refused with "The vessel cannot carry any more
    vehicles." (per-class cargo weight enforcement).
26. Sail one step, `shipstatus`; `unloadvehicle cart` while moving -
    refused (must be stationary). `speed 0`, `unloadvehicle cart` -
    vehicle unloads at ship's coordinates.

## F. Docking and boarding defense smoke

27. Spawn a second prototype ship in the same room. From ship 1's helm:
    `dock <ship2>` - docking completes; `undock` separates.
28. (Hostile path, staff-only smoke) `board <ship2>` via do_board_hostile
    where allowed - on failure roll, character may fall into the water:
    lands in the wilderness room holding their own ship, and a Swim
    (Athletics) check line prints; on defended ships, idle crew NPCs are
    repositioned to entrance/bridge ("BATTLE STATIONS!" broadcast).

## G. Cleanup

29. `vedit delete N` - prototype removed; `vedit list` no longer shows it.
30. Purge spawned ship objects and any test vehicles; note that generated
    interior rooms persist until reboot (known limitation - runtime room
    reclamation is future work).

## Known Findings

- Disembark "You're not aboard a vessel" - FIXED.
- The `IN_ROOM(shipobj)` exit-point lookup is correct for a self-consistent
  ship, including the `vedit` spawn path.
- Disembark "Unable to find a valid exit point" remains reproducible with the
  legacy zone-700 fixture because its slot, `shipnum`, and interior-room
  identities disagree. This is the current step-3 blocker, not a regression in
  the `IN_ROOM(shipobj)` lookup itself.

# EoF
