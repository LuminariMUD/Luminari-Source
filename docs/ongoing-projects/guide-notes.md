# Vessel guide notes (S9): one finding still to file

These were the notes of the S9 vessel play test. Their content is in the
[Vessel Player Guide](../guides/VESSEL_PLAYER_GUIDE.md), and the full notes are archived (see
the references of [ADR 0003](../adr/0003-durismud-naval-model.md)). One defect they record has
no work item yet, which is why this document remains.

GitLab's spam check answered the API request for this work item with a CAPTCHA challenge on
2026-10-04 (three attempts; the seven other work items of the same cleanup, #20 through #26,
were created normally). It has to be created in the web interface: type Issue, label Bug,
assignee max757, with the title and description below. Delete this document once it is filed.

## Title

Two hulls at the same position can fire only with the arc that faces north

## Description

### Problem

Two hulls at exactly the same position can fire at each other only with the weapons on whichever
arc happens to face north, and a hit lands on the target's north-facing arc.

`vessel_bearing_between()` (`src/vessels/vessels.c`) computes the bearing with
`atan2(dx, dy)`. For coincident positions that is `atan2(0, 0)`, which is 0: due north.
`vessel_arc_toward()` (`src/vessels/vessels_combat.c`) turns that bearing into an arc relative
to the heading, and `shipfire`/`shipsight` (`src/vessels/vessels_gunnery.c`) refuse any weapon
whose arc is not that one: "The port Large Ballista cannot bear - <target> lies off your fore
arc." The contact list's arc column, the MSDP `SHIP_CONTACTS` arc, the raider AI's facing, and
the ram's impact arcs read the same bearing.

Coincident positions are not rare. A `setsail` maneuver and a hull coming to rest in a berth both
put the hull at the centre of its room (`dx = dy = 0` in `src/vessels/vessels_movement.c`), so
two hulls that maneuver into the same room are at range 0.0.

### Current evidence

- Found in the S9 play test (2026-10-02): two captains at range 0.0 could not fire unless a
  weapon faced north.
- Not fixed; the player guide documents the workaround instead
  (`docs/guides/VESSEL_PLAYER_GUIDE.md`, "Fighting another captain": "Two hulls on the same spot
  (range 0.0) cannot fire at each other unless a weapon faces north, since at range 0 the bearing
  is taken as north. Keep a room or two apart.").
- The code is unchanged on master `d5cd6a735`.

### Completion criteria

- A defined rule for coincident hulls (for example: every arc bears and the struck arc is chosen
  from the hulls' relative headings), applied wherever the bearing picks an arc: gunnery sight
  and fire, the arc a hit lands on, the contact list's arc column, raider facing, and ramming.
- A production-linked test at range 0.0 in `unittests/CuTest/test_vessel_gunnery.c`.
- The guide's workaround tip, and help if it changes, updated in both `lib/text/help/help.hlp`
  and the database.
