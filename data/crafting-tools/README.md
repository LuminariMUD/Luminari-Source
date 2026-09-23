# Crafting tools

These five object prototypes are the crafting tools of
https://github.com/LuminariMUD/Luminari-Source/issues/219. Every materials-and-motes project
except woodworking needs its skill's tool, and no world object was one, so only woodworking
projects could start.

A tool counts when it is a crafting tool (item type 57) whose first value is the project's skill
and it is worn in that skill's tool slot. The same rule decides project readiness and completion,
`craft tools`, and the tool's skill bonus (its second value; these tools give none). The code is
`worn_crafting_tool()` in `src/craft/crafting_new.c`.

## Prototype records

`3.obj` holds five records, objects 391-395, followed by the `$~` terminator. Each is item type 57
with wear flags Take plus its tool slot, level 1, a room description, and an examine description.
They are tiny: wearing checks an item's size against the wearer's, and a medium tool would not fit
a fae. Value 0 is the skill's ability number, value 1 the bonus (0), value 2 the quality
(100).

| VNUM | Keywords | Short description | Skill (value 0) | Wear flags | Cost |
| -- | -- | -- | -- | -- | -: |
| 391 | sewing needle tool tailoring | a tailor's sewing needle | tailoring (35) | `aF` (needle) | 100 |
| 392 | armorsmith hammer tool armorsmithing | an armorsmith's hammer | armorsmithing (37) | `aD` (armor hammer) | 200 |
| 393 | weaponsmith hammer tool weaponsmithing | a weaponsmith's hammer | weaponsmithing (38) | `a a` (weapon hammer, bit 32) | 200 |
| 394 | jeweler pliers tool jewelcrafting | a pair of jeweler's pliers | jewelcrafting (40) | `aE` (jeweler's pliers) | 150 |
| 395 | alchemy set alchemist kit tool | an alchemy set | alchemy (36) | `aC` (alchemy set) | 250 |

## Where players get them

Jufus the materials vendor (mobile 369) sells them in the Supply Materials Shop, room 369 of the
Sanctus III crafting district, north of the Slanting Passageway (room 368) and two rooms from the
crafting benches and the quartermaster (room 372). Shop 369 already sells the crafting materials
there; the tools join its product list.

## World installation

The main source repository keeps authored additions here because each site's live world is
maintained separately. Before installing, confirm that the site's world has no objects 391-395 and
no other crafting tools (item type 57, or any crafting tool wear flag) that should be sold instead.

1. Merge the five records from `3.obj` into `lib/world/obj/3.obj` in ascending VNUM order (between
   390 and 396 in the development world), preserving the other records and the final `$~`
   terminator.

2. In `lib/world/shp/3.shp`, add the five lines `391` through `395` to the product list of shop
   `#369~`, before the `-1` that ends it. The shop's buy types, messages, keeper, and room stay as
   they are.

3. Validate the merged world, then load it through the site's usual world-data release procedure.

```sh
python3 scripts/world/wtool.py validate --paths data/crafting-tools/3.obj --strict
python3 scripts/world/wtool.py validate --zone 3 --strict
```

Alternatively, build them in game: `oedit 391` through `395` (type crafting tool, the values and
wear flags above), and `sedit 369` to add them to the shop's products. Like `data/supply-orders`,
the bundle has no installer script and no build-list entry. Players see where tools come from in
`HELP CRAFTING` and `HELP CRAFTING-STATIONS`.
