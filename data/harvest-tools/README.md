# Wilderness harvest tools

These five object prototypes implement the tool identities for
https://github.com/LuminariMUD/Luminari-Source/issues/145, and `3.zon` holds the resets that let
players buy them (https://github.com/LuminariMUD/Luminari-Source/issues/222).

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

## Where players get them

Jufus the materials vendor (mobile 369) sells all five in the Supply Materials Shop, room 369 of
the Sanctus III crafting district, beside the crafting tools, at the costs above. No reward
system can grant an object (supply orders pay gold, experience, and artisan points), and the
costs already rise five times per tier. A tool only sets a grade floor; every harvest still has
to pass its difficulty roll. A shopkeeper sells only what it carries, so zone 3 gives Jufus one of
each tool when it loads him, and the product list makes each sale a new copy.

## World installation

The main source repository keeps authored additions here because each site's
live world is maintained separately.

1. Merge the five records in `harvest-tools.obj` into the object's existing zone-12 file in
   ascending VNUM order, preserving all other records and the final `$~` terminator. Check for
   existing uses of 1251-1255 before installation; conflicting objects must not be overwritten.
   If no zone-12 object file exists and the range is unused, the fragment can be installed as
   `lib/world/obj/12.obj` and included in the object index in VNUM order.

2. In `lib/world/shp/3.shp`, add the product lines `1251` through `1255` to the product list of
   shop `#369~`, before the `-1` that ends it, skipping any already listed. (The production
   world listed 1252 and 1253 on 2026-09-23 but gave Jufus no copies, so neither could be
   bought.) The development world lists them after `3142`, ahead of the crafting tools.

3. In `lib/world/zon/3.zon`, add the five `G` resets from `3.zon` after Jufus's reset
   (`M 0 369 1 369 100`) and the `G` resets that follow it.

4. Validate the merged world, then load it through the site's usual world-data release
   procedure; the objects, shop products, and resets take effect at the next boot or copyover.
   Staff can inspect a tool with `load obj 1251 1` (and similarly through 1255).

```sh
python3 scripts/world/wtool.py validate --paths data/harvest-tools/harvest-tools.obj --strict
python3 scripts/world/wtool.py validate --zone 3 12 --strict
```

The quality-tier rule and the crafting mappings are documented in
[WILDERNESS_HARVESTING.md](../../docs/systems/WILDERNESS_HARVESTING.md).
