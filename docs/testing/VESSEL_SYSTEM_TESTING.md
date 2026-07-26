# Vessel System - Manual Testing Guide

Numbered manual regression script for the Vessel System. Run on dev after any
vessel-related change (quality gate step 4 in
[VESSEL_PRD_FINAL.md](../project-management-zusuk/vessels/VESSEL_PRD_FINAL.md),
Section 10). Every step lists the expected result; any deviation is a
regression.

Prerequisites: staff character (LVL_BUILDER+), MySQL running, server booted
with the vessel system enabled in cedit.

## A. Legacy world-file vessel (zone 700 test object)

Fixtures: object 70002 (test vessel, ITEM_GREYHAWK_SHIP, ship_index 0),
room 70003 (interior), room 1000389 (Testing Dock, wilderness (-66, 92)).

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

## Known fixed issues (do not re-open)

- Disembark "You're not aboard a vessel" - FIXED.
- Disembark "Unable to find a valid exit point" - FIXED via
  IN_ROOM(shipobj) exit-point lookup (src/vessels.c).

# EoF
