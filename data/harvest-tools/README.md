# Wilderness harvest tools

These five object prototypes implement the tool identities for
https://github.com/LuminariMUD/Luminari-Source/issues/145.

VNUMs 1251-1255 correspond to Poor, Common, Uncommon, Rare and Legendary.
Each object is a portable, holdable tool; carrying it is sufficient.
The feature checks VNUM, not an object name or a generic tool flag.

## Prototype records

The following names and VNUMs are taken directly from `harvest-tools.obj`.
Each record has type `ITEM_OTHER` (12), wear flags `ao` (`ITEM_WEAR_TAKE`
at bit 0 and `ITEM_WEAR_HOLD` at bit 14), weight 1, level 1 and medium size,
with a room description and an examine description. Material and cost rise
with the tier: a Poor floor adds nothing because Poor is already the lowest
roll, while a Legendary floor makes every successful harvest top grade.
The file contains exactly these five records followed by the `$~` terminator.

| VNUM | Keywords | Short description | Material | Cost |
| -- | -- | -- | -- | -- |
| 1251 | poor harvest tool | a poor harvest tool | wood | 50 |
| 1252 | common harvest tool | a common harvest tool | iron | 500 |
| 1253 | uncommon harvest tool | an uncommon harvest tool | steel | 2500 |
| 1254 | rare harvest tool | a rare harvest tool | mithril | 12500 |
| 1255 | legendary harvest tool | a legendary harvest tool | adamantine | 50000 |

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

The quality-tier rule and the crafting mappings are documented in
[WILDERNESS_HARVESTING.md](../../docs/systems/WILDERNESS_HARVESTING.md).
