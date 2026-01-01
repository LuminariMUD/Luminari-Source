# VESSEL_MANUAL_TEST.md

File for manual testing/tweaking the Vessel System

## Vessel Object for testing:

lib/world/obj/700.obj:

#70002
vessel test~
a test vessel~
A test vessel is here.~
~
56 abcd 0 0 0 a 0 0 0 0 0 0 0 0 0 0 0
70003 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
99999 0 0 1 0
G
0
H
8
I
5
J
0
Z
Greyhawk Ship
$~

## Vessel Interior Room for Testing

lib/world/wld/700.wld

#70003
A Vessel Test Room 01~
You are in an unfinished room.
~
700 0 256 0 0 0
C
0 0
Z
Greyhawk Ship Commands
S
$~

## Vessel Object Location in Wilderness

lib/world/wld/10000.wld

Current Location  : (-66, 92)

#1000389
Testing Dock~
This unfinished room was created by Zusuk.
~
10000 0 512 0 0 34
D0
~
~
0 0 1000000
D1
~
~
0 0 1000000
D2
~
~
0 0 1000000
D3
~
~
0 0 1000000
C
-66 92
S

## Testing commands

### Board - when typing board in a room that has a vessel object, you successfully go to the interior room set by the vessel object
### Disembark - "You're not aboard a vessel" (FIXED)
### Disembark - "Unable to find a valid exit point" (FIXED)

**Issue:** After boarding a vessel, disembark command failed with "Error: Unable to find a valid exit point."

**Root Cause:** `do_greyhawk_disembark()` used `find_room_by_coordinates()` which returned `NOWHERE` because the wilderness room wasn't registered in the dynamic room system.

**Fix:** Changed to use `IN_ROOM(greyhawk_ships[shipnum].shipobj)` which directly accesses the room where the ship object is located. This linkage is properly established during boarding.

**Files Modified:** `src/vessels.c` - lines 1948-1953 and 2024-2028

**Test Log (before fix):**
```
[1000389] Testing Dock [ Dockable ]  [Sea Port] ( None )
This unfinished room was created by Zusuk.
 Current Location  : (-66, 92)
 Weather           : 133
[ Exits: N E S W ]
[70002] A test vessel is here. ..It has a soft glowing aura! ..It emits a faint humming sound!
15922/15922H 17000/17000P 830/830V [-mw] EX:NESW board
You board the ship.
[70003] A Vessel Test Room 01 [ Vehicle ]  [Inside] ( None )
You are in an unfinished room.
[ Exits: None!]
15922/15922H 17000/17000P 830/830V [smw] EX:None!  disembark
Error: Unable to find a valid exit point.
```

**Retest Required:** Board vessel, then disembark - should return to room 1000389



# EoF
