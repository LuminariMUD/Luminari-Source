# OLC: SpecProc Editing and Persistence

This note explains how builders can assign and persist special procedures (SpecProcs) using the OLC editors for mobs, objects, and rooms.

## Overview
- New menu option in all three editors: `Z) SpecProc`.
- Select from the centralized registry (defined in `src/spec_assign.c` via `spec_func_list[]`).
- Selections apply at save time and now persist across reboots via world files.

## Usage (medit/oedit/redit)
- From the main menu, press `Z` to open the SpecProc selector.
- Enter a number to choose a SpecProc; enter `0` to clear.
- The current selection is shown in the menu. Save as usual to apply.

## File Format Persistence
The selected SpecProc is stored by name and resolved at boot.

- Mobs (E-spec block):
  - Line written inside the `E` specs section:
    - `SpecProc: <Name>`

- Objects:
  - A `Z` block is added:
    - Line 1: `Z`
    - Line 2: `<Name>`

- Rooms:
  - A `Z` block is added before the terminating `S` line:
    - Line 1: `Z`
    - Line 2: `<Name>`

If the name isn’t recognized in the SpecProc registry, the function won’t be assigned at boot.

## Notes and Tips
- Names must match entries in `spec_func_list[]` (see `src/spec_assign.c`).
- Some SpecProcs are intended for specific types (mob/object/room). Assigning a mismatched SpecProc won’t crash, but may do nothing.
- Clearing a SpecProc removes the corresponding lines from the world file on next save.
- The selector is 1-based; `0` always clears.

## Troubleshooting
- Change not taking effect after save: ensure the zone was saved and the game reloaded the zone or rebooted.
- Persistence missing after reboot: verify the saved name exists in `spec_func_list[]` and has not been renamed.
- File merge conflicts: the `SpecProc`/`Z` entries are safe to keep; ensure the SpecProc name remains on its own line as shown above.

## Examples
- Mob E-spec example snippet:
  - `SpecProc: Receptionist`
- Object snippet:
  - `Z` then `Bank`
- Room snippet:
  - `Z` then `Wizard Library`
