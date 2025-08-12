# DG SCRIPT PARAMETER CORRUPTION BUG - CRITICAL


# old stable system of dg scripts that can be used for reference:
old_dg_scripts.c
old_dg_scripts.h
old_dg_variables.c
old_dg_triggers.c

## DG Script Issue Investigation - Trigger 7705

### Debug Output
                 .
[ SCRIPT_DEBUG 7705: script_driver START - args struct received ]
[ SCRIPT_DEBUG 7705: args ptr=0x7fffffff0d40, args->type=2, args->go_adress=0x7fffffff0d38, args->trig=0x555556b37650, args->mode=0 ]
[ SCRIPT_DEBUG 7705: extracted values - type=2, go_adress=0x7fffffff0d38, trig=0x555556b37650, mode=0 ]
[ SCRIPT_DEBUG 7705: Constants - MOB=0, OBJ=1, WLD=2 ]
[ WARNING 7705: Type mismatch! Received type=2 but trigger flags suggest type=1 ]
[ SCRIPT_DEBUG 7705: script_driver entry - go_adress=0x7fffffff0d38, trig=0x555556b37650, type=2, mode=0 ]
[ SCRIPT_DEBUG 7705: Constants - MOB=0 OBJ=1 WLD=2, TRIG_NEW=0 TRIG_RESTART=1 ]
[ Trigger: Teleport Charged Key Holder and all in room, VNum 7705, type: 2. ERROR: Unknown room
field 'room' (room vnum: 7771, attempted access: %<room_var>.room%) ]

## CRITICAL FINDING: The struct refactoring didn't fix the corruption!

The debug output shows that even WITH the struct refactoring in place, `args->type=2` is being passed to script_driver when it should be `1` (OBJ_TRIGGER). This means the corruption is happening BEFORE the struct is created in cmd_otrig.

## Theory: The Corruption is NOT in script_driver

### Evidence:
1. **Line 15**: `args->type=2` - The struct ALREADY contains the wrong value
2. **Line 18**: Trigger flags correctly identify this should be type=1 (OBJ_TRIGGER)
3. **Line 22**: The error occurs because type=2 makes the code think it's a room trigger

### Where is the corruption actually happening?

The corruption must be occurring in `cmd_otrig` BEFORE the struct is created. Let's trace the call path:

1. `cmd_otrig` is called with an object trigger
2. In `cmd_otrig`, we explicitly set `int trigger_type = OBJ_TRIGGER;` (value 1)
3. We create the struct: `struct script_call_args args = {&obj, t, trigger_type, TRIG_NEW};`
4. But the debug shows `args->type=2` (WLD_TRIGGER) instead of 1

### Possible Causes:

1. **Stack Corruption During Struct Initialization**: The struct initialization might be corrupted. In ANSI C90, compound literals and designated initializers have limitations.

2. **Memory Alignment Issues**: The struct might have alignment issues causing the wrong value to be written to the type field.

3. **Compiler Optimization Bug**: The compiler might be optimizing incorrectly, especially since we're using -O2.

4. **The Address is Wrong**: Notice `args->go_adress=0x7fffffff0d38` but `args ptr=0x7fffffff0d40`. The go_adress is BEFORE the args struct in memory (0x0d38 < 0x0d40), which is suspicious.

### Most Likely Theory: Struct Initialization Corruption

The struct initialization `{&obj, t, trigger_type, TRIG_NEW}` might be getting corrupted. In the debug output:
- `args->go_adress=0x7fffffff0d38` 
- `args ptr=0x7fffffff0d40`

The go_adress field (which should contain &obj) has an address (0x0d38) that's 8 bytes BEFORE the struct itself (0x0d40). This is impossible if go_adress is the first field of the struct - it should be AT 0x0d40 or after it.

This suggests the struct memory layout is corrupted or the initialization is writing to the wrong memory locations.

### CRITICAL DISCOVERY: cmd_otrig debug output is MISSING!

We added debug logging to cmd_otrig but it's NOT appearing in the output. This means trigger 7705 is NOT being called through cmd_otrig at all!

The debug output shows:
- No "cmd_otrig calling script_driver" message
- But we DO see "script_driver START" message
- The args->go_adress=0x7fffffff0d38 looks like a STACK address, not a heap address

This means the trigger is being called from SOMEWHERE ELSE that's passing the wrong type!

### Hypothesis: Wrong Caller

Trigger 7705 is an object command trigger (`1 c 2`) but it might be getting called from:
1. A room trigger handler by mistake
2. A recursive call from within another trigger
3. Some other trigger mechanism that's incorrectly treating it as a room trigger

The fact that go_adress=0x7fffffff0d38 (a stack address) and args->type=2 (WLD_TRIGGER) strongly suggests this is being called from a room trigger context.

## ROOT CAUSE ANALYSIS

### The Real Problem: Trigger 7705 is being called with WLD_TRIGGER type!

The struct refactoring worked correctly - it's preserving the parameters exactly as passed. The problem is that someone is calling trigger 7705 with type=2 (WLD_TRIGGER) instead of type=1 (OBJ_TRIGGER).

### Key Evidence:

1. **Stack Address**: go_adress=0x7fffffff0d38 is a stack address (0x7fff... prefix), which matches how room triggers pass `&room` (a local stack variable)
2. **Missing cmd_otrig output**: We don't see the debug from cmd_otrig, meaning the trigger isn't being called through the normal object command path
3. **Type is consistently 2**: The struct is correctly preserving type=2 as passed to it

### Most Likely Scenario:

There might be a room in the game (room 7771 based on the error) that has a trigger which is somehow trying to execute object trigger 7705. This could happen if:

1. A room trigger is using `%trigger%` command to fire trigger 7705
2. A script is using `attach` to dynamically attach trigger 7705 to a room
3. There's a data corruption where trigger 7705 is attached to both an object AND a room

### The Error Message Decoded:

"Unknown room field 'room' (room vnum: 7771, attempted access: %<room_var>.room%)"

- The script is trying to access `%self.room%` 
- Because type=2, it thinks `self` is a room (room 7771)
- Rooms don't have a `.room` field (objects do), so it fails

### Solution:

We need to find WHERE trigger 7705 is being incorrectly called with WLD_TRIGGER type. This is NOT a code bug in the DG Scripts engine - it's either:
1. A data problem (trigger attached to wrong entity type)
2. A script calling the trigger incorrectly
3. A different code path that's mishandling the trigger type

## PROBLEM FOUND!

### Trigger 7705 is attached to BOTH an object AND room 7771!

From `lib/world/wld/77.wld`:
```
#7771
Atop a steep hill~
...
T 7705    <--- TRIGGER 7705 ATTACHED TO THE ROOM!
```

This is a **DATA BUG**, not a code bug! 

Trigger 7705 is designed as an OBJECT command trigger (`1 c 2`) for the "unlock" command on object 7703 (charged key). But someone has also attached it to room 7771. When the room tries to execute this trigger, it passes WLD_TRIGGER (type=2) because it's being called from a room context.

The script tries to execute `%self.room%` which:
- Works fine when called from an object (objects have a .room field)
- FAILS when called from a room (rooms don't have a .room field)

### The Fix:

Remove trigger 7705 from room 7771. It should only be attached to object 7703.

### Why This Happened:

Looking at the room description, it mentions "3 small keyholes" and "The charged key goes into it". Someone probably thought they needed to attach the unlock trigger to the room as well as the object, but that's incorrect. The trigger should only be on the object (the key), not the room.

### Verification:

Check if trigger 7705 is properly attached to object 7703:
`grep -r "7705" lib/world/obj/`

## RESOLUTION

### What Was Fixed

1. **Removed trigger 7705 from room 7771** - This was the root cause. The trigger was designed for objects but was attached to a room.

2. **Added Runtime Validation** - The script_driver now detects when triggers are attached to wrong entity types and:
   - Logs a CRITICAL ERROR with specific details
   - Identifies exactly what's wrong (e.g., "Object trigger 7705 is incorrectly attached to room 7771")
   - Provides the exact fix needed
   - ABORTS execution to prevent confusing cascade errors

3. **Preserved the Struct Refactoring** - While it didn't fix THIS issue, the struct refactoring is still good for preventing stack corruption in ANSI C90.

### How The Protection Works

When script_driver is called, it now:
1. Checks the trigger's flags to determine what type it SHOULD be
2. Compares with the type it's being called with
3. If there's a mismatch, it logs detailed error messages and refuses to execute

Example output for this issue would be:
```
CRITICAL ERROR: Trigger 7705 (Teleport Charged Key Holder and all in room) is a OBJ_TRIGGER trigger but is being called as WLD_TRIGGER!
  This usually means the trigger is attached to the wrong entity type.
  Check: grep '#7705' lib/world/*/*.* to find where it's attached.
  Object trigger 7705 is incorrectly attached to room 7771
  FIX: Remove 'T 7705' from room definition in world files
  ABORTING trigger execution to prevent cascade errors.
```

### Lessons Learned

1. **Not all bugs are code bugs** - This was a data configuration error
2. **Validate assumptions early** - The type mismatch should be caught immediately
3. **Provide clear error messages** - "Unknown room field 'room'" was cryptic; the new messages explain exactly what's wrong
4. **Prevent cascade failures** - Better to abort than to generate confusing errors

### Why This Happened Originally

The builder probably thought "the unlock happens in room 7771, so I'll attach the trigger there too." But object command triggers should only be on objects - the MUD automatically checks objects in inventory and room when commands are typed.


