# TASK LIST - Coder ToDo List

# LuminariMUD Potentially Valid Bug List

This file contains bugs that have been verified as potentially legitimate code-related issues after auditing the codebase.

## Potentially Valid Bugs

### Sending Password Too Early
- **Description**: Sending password in same packet as username causes echo loop (DO ECHO, WILL ECHO, DONT ECHO, WONT ECHO)
- **Reported by**: Yarea (Level 4)
- **Status**: FIXED (2025-07-26)
- **Details**: If a client sends the password too early (in the same packet as username), there could be a race condition with the echo negotiation (ProtocolNoEcho).
- **Location**: interpreter.c:2367, protocol.c:1078
- **Fix**: Modified all password input states to clear pending input queue before entering password mode, preventing early password processing

### Clairvoyance Fall Damage
- **Description**: Casting clairvoyance on eternal staircase causes fall damage
- **Reported by**: Hibbidy (Level 26)
- **Status**: FIXED (2025-07-26)
- **Details**: The clairvoyance spell temporarily moves the caster to the target's room. If the target is in a falling room, the caster could take fall damage during this temporary move.
- **Location**: spells.c:1398-1447
- **Fix**: Modified spell_clairvoyance to use look_at_room_number() instead of physically moving the character

### Post Death Bash
- **Description**: Mob can bash player immediately after mob's death
- **Reported by**: Chentu (Level 15)
- **Status**: FIXED (2025-07-26)
- **Details**: There could be a race condition where a mob's bash action executes after the mob's position is set to POS_DEAD but before the mob is extracted from the game.
- **Location**: fight.c (combat resolution order)
- **Fix**: Added safety checks to perform_knockdown() in act.offensive.c to check both position and extraction flags

## Bugs Requiring Further Investigation

### Magic Stone Spell
- **Description**: Magic stone gives explosion message followed by random codes and numbers
- **Notes**: Previously working, now only works on PC targets
- **Reported by**: Andross (Level 7)
- **Status**: NEEDS INVESTIGATION
- **Details**: The spell is implemented and creates objects (vnums 20871 or 9401), but without access to object database files, cannot verify explosion message issues.

### Damage Reduction Display
- **Description**: Feats show 21 DR but combat rolls only show 18
- **Reported by**: Melow (Level 29)
- **Status**: NEEDS INVESTIGATION
- **Details**: Could be a discrepancy between DR displayed in feats list versus actual combat calculations. Need to compare display functions with calculation functions.



