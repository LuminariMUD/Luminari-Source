# OLC: SpecProc Editing and Persistence

This note explains how builders can assign and persist special procedures (SpecProcs) using the OLC
editors for mobs, objects, and rooms.

## Overview

- All three editors provide `Z) SpecProc`.
- Select from the centralized registry defined in `src/spec/spec_registry.c`.
- Each editor lists only builder-visible definitions compatible with that mobile, object, or room.
- Selections apply at save time and now persist across reboots via world files.

## Usage (medit/oedit/redit)

- From the main menu, press `Z` to open the SpecProc selector.
- Read the category, description, supported events, and per-event prerequisites beneath each entry.
- Enter a number to choose a SpecProc; enter `0` to clear.
- The current selection is shown in the menu. Save as usual to apply.

Numbers are specific to the filtered editor view. The current views contain 18 mobile definitions,
5 object definitions, and 6 room definitions in canonical registry order. The saved world record
uses the procedure name, not the displayed number.

Prerequisites describe runtime scheduling; selecting a procedure does not set them automatically:

- `MOB_SPEC` enables mobile activity and combat-turn callbacks that require it.
- `ITEM_AUTOPROC` enables object auto-pulses that require it.
- `carried`, `equipped`, and `combat` state describe where an event can run.
- `prerequisites: none` means that event has no registry-level flag or placement requirement.

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

If the name is not recognized in the SpecProc registry, the function is not assigned at boot, but
the authored name remains attached to the prototype for diagnostics and later saves.

## Authored Binding State

World loading now keeps an owned authored-binding record on each mobile, object, or room prototype.
The record includes the exact requested name, owner and VNUM, source kind, source location, resolved
registry definition, and resolution status. Canonical names and aliases resolve to the same immutable
definition while preserving the spelling that appeared in the world file.

Unknown names and definitions that are incompatible with the owner or source remain available for
diagnostics, but they do not install a callback. Boot warnings identify the persisted field, owner,
VNUM, requested name, and reason. Prototype copies and OLC editing use independent owned records so
editing or deleting one prototype cannot invalidate another.

Disk writers use this authored record whenever it exists. Exact loaded aliases, unknown names, and
owner- or source-incompatible names therefore survive unrelated OLC saves. A later hard-coded
callback override remains effective at runtime but is not promoted into the authored world field.
Only legacy prototypes with no authored record use callback reverse lookup as a compatibility
fallback.

Builder actions are explicit. Selecting a registry entry replaces any prior record with its
canonical name. Entering `0` clears both the authored record and callback, so the field is omitted
on the next zone save. Merely opening and saving an editor preserves the existing requested name,
including unresolved content that a builder may need to repair later.

## Notes and Tips

- Names should match a canonical name or explicit alias in `src/spec/spec_registry.c`; other names
  remain persisted and diagnosable but do not install a callback.
- The selector shows canonical definitions only. An alias such as `Guildmaster` still loads for
  compatibility but does not create a duplicate menu row; selecting the entry saves `Guild`.
- A procedure hidden from builders, disallowed for world binding, or incompatible with the edited
  owner is not selectable. Invalid and out-of-range input leaves the current selection unchanged.
- Registry metadata is validated before world parsing. An invalid registry is a programmer error
  that stops boot; an unknown persisted name remains a content error and is not assigned.
- Clearing a SpecProc removes the corresponding lines from the world file on next save.
- The selector is 1-based; `0` always clears.

## Troubleshooting

- Change not taking effect after save: ensure the zone was saved and the game reloaded the zone or rebooted.
- Procedure does not run on a pulse: reopen the selector and check the event's required flags and
  placement. The selector deliberately does not change those flags for you.
- Procedure is absent from one editor: it is not compatible with that owner type or is not allowed
  for builder-authored world binding.
- Persistence missing after reboot: verify the saved name is a canonical name or alias in the
  definition registry and has not been renamed.
- File merge conflicts: the `SpecProc`/`Z` entries are safe to keep; ensure the SpecProc name remains on its own line as shown above.

## Examples
- Mob E-spec example snippet:
  - `SpecProc: Receptionist`
- Object snippet:
  - `Z` then `Bank`
- Room snippet:
  - `Z` then `Wizard Library`
