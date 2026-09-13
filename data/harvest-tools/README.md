# Wilderness harvest tools

These five object prototypes implement the tool identities for
https://github.com/LuminariMUD/Luminari-Source/issues/145.

VNUMs 1251-1255 correspond to Poor, Common, Uncommon, Rare and Legendary.
Each object is a portable, holdable tool; carrying it is sufficient.
The feature checks VNUM, not an object name or a generic tool flag.

## World installation

The main source repository keeps authored additions here because each site's
live world is maintained separately. Merge the five records in
`harvest-tools.obj` into the object's existing zone-12 file in ascending VNUM
order, preserving all other records and the final `$~` terminator. Check for
existing uses of 1251-1255 before installation; conflicting objects must not be
overwritten. If no zone-12 object file exists and the range is unused, the
fragment can be installed as `lib/world/obj/12.obj` and included in the object
index in VNUM order.

Validate the merged world, then load the updated prototypes through the site's
usual world-data release procedure. Staff can inspect a tool with
`load obj 1251 1` (and similarly through 1255). Distribution through shops or
rewards is a world-building choice; the prototypes themselves are complete.

```sh
python3 scripts/world/wtool.py validate --paths data/harvest-tools/harvest-tools.obj --strict
```

The minimum-quality rule, crafting mappings and environment switch are documented
in [WILDERNESS_HARVESTING.md](../../docs/systems/WILDERNESS_HARVESTING.md).
