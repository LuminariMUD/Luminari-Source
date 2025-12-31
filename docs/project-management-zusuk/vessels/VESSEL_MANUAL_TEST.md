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

- Board - when typing board in a room that has a vessel object, you successfully go to the interior room set by the vessel object


## Issues Found During Testing (2025-12-31)

### Issue 1: Disembark says "You're not aboard a vessel" (FIXED)

**Cause:** `greyhawk_ship_object` special procedure in `spec_procs.c` moved player to interior room but never set `world[interior_room].ship` pointer. The `do_greyhawk_disembark` function checks this pointer at `vessels.c:1908`.

**Fix Applied:** Added to `spec_procs.c` in `greyhawk_ship_object`:
- Added `#include "vessels.h"` and extern for `greyhawk_ships[]`
- Added `world[interior_room].ship = &greyhawk_ships[ship_index];` before moving player

### Issue 2: Ship object not linked to ship data (FIXED)

**Cause:** `greyhawk_ships[0].shipobj` was never set in the new vessel code.

**Impact:** The sync code at `vessels.c:899-912` that moves the ship object when coordinates change would never execute because `greyhawk_ships[shipnum].shipobj` was NULL.

**Fix Applied:** Added to `spec_procs.c` in `greyhawk_ship_object` (line 12006):
```c
greyhawk_ships[ship_index].shipobj = obj;
```

### Required Linkages Summary

| Linkage | Code Location | Status |
|---------|---------------|--------|
| Interior Room -> Ship Data | `world[room].ship = &greyhawk_ships[idx]` | FIXED (boarding proc) |
| Ship Data -> Ship Object | `greyhawk_ships[idx].shipobj = obj` | FIXED (boarding proc) |
| Ship Object -> Ship Index | `GET_OBJ_VAL(obj, 1) = idx` | OK (object file) |


# EoF
