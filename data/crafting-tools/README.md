# Crafting tools

These six object prototypes are the crafting tools of
https://github.com/LuminariMUD/Luminari-Source/issues/219 and, for leatherworking,
https://github.com/LuminariMUD/Luminari-Source/issues/221. Every materials-and-motes project
except woodworking needs its skill's tool, and no world object was one, so only woodworking
projects could start.

A tool counts when it is a crafting tool (item type 57) whose first value is the project's skill
and it is worn in that skill's tool slot. The same rule decides project readiness and completion,
`craft tools`, and the tool's skill bonus (its second value; these tools give none). The code is
`worn_crafting_tool()` in `src/craft/crafting_new.c`.

## Prototype records

`3.obj` holds six records, objects 389 and 391-395, followed by the `$~` terminator, and `3.zon`
holds the six resets that give them to the vendor. Each is item type 57
with wear flags Take plus its tool slot, level 1, a room description, and an examine description.
They are tiny: wearing checks an item's size against the wearer's, and a medium tool would not fit
a fae. Value 0 is the skill's ability number, value 1 the bonus (0), value 2 the quality
(100).

| VNUM | Keywords | Short description | Skill (value 0) | Wear flags | Cost |
| -- | -- | -- | -- | -- | -: |
| 389 | leatherworker knife tool leatherworking | a leatherworker's knife | leatherworking (41) | `aA` (skinning knife) | 150 |
| 391 | sewing needle tool tailoring | a tailor's sewing needle | tailoring (35) | `aF` (needle) | 100 |
| 392 | armorsmith hammer tool armorsmithing | an armorsmith's hammer | armorsmithing (37) | `aD` (armor hammer) | 200 |
| 393 | weaponsmith hammer tool weaponsmithing | a weaponsmith's hammer | weaponsmithing (38) | `a a` (weapon hammer, bit 32) | 200 |
| 394 | jeweler pliers tool jewelcrafting | a pair of jeweler's pliers | jewelcrafting (40) | `aE` (jeweler's pliers) | 150 |
| 395 | alchemy set alchemist kit tool | an alchemy set | alchemy (36) | `aC` (alchemy set) | 250 |

Leatherworking and hunting share the skinning knife slot (the OLC's Hunting-Tool wear flag). Each
skill still needs a tool made for it, so a leatherworker's knife counts only for leatherworking.
Hunting uses its tool only for room harvesting, which no world room offers at present.

## Where players get them

Jufus the materials vendor (mobile 369) sells them in the Supply Materials Shop, room 369 of the
Sanctus III crafting district, north of the Slanting Passageway (room 368) and two rooms from the
crafting benches and the quartermaster (room 372). Shop 369 already sells the crafting materials
there; the tools join its product list. A shopkeeper sells only what it carries, so zone 3 gives
Jufus one of each tool when it loads him, as it does the materials, and the product list makes each
sale a new copy.

## World installation

The main source repository keeps authored additions here because each site's live world is
maintained separately. Before installing, confirm that the site's world has no objects 389 or
391-395 and no other crafting tools (item type 57, or any crafting tool wear flag) that should be
sold instead. A world that already has 391-395 from the first release needs only object 389.

1. Merge the records from `3.obj` into `lib/world/obj/3.obj` in ascending VNUM order (389
   between 386 and 390, and 391-395 between 390 and 396 in the development world), preserving the
   other records and the final `$~` terminator.

2. In `lib/world/shp/3.shp`, add the product lines `389` and `391` through `395` to the product
   list of shop `#369~`, before the `-1` that ends it. The shop's buy types, messages, keeper, and
   room stay as they are.

3. In `lib/world/zon/3.zon`, add the `G` resets from `3.zon` after Jufus's reset
   (`M 0 369 1 369 100`) and the `G` resets that follow it.

4. Validate the merged world, then load it through the site's usual world-data release procedure;
   the new objects, shop products, and resets take effect at the next boot or copyover.

```sh
python3 scripts/world/wtool.py validate --paths data/crafting-tools/3.obj --strict
python3 scripts/world/wtool.py validate --zone 3 --strict
```

Alternatively, build them in game: `oedit 389` and `oedit 391` through `395` (type crafting tool,
the values and wear flags above), `sedit 369` to add them to the shop's products, and `zedit` to
give them to Jufus. Like `data/supply-orders`,
the bundle has no installer script and no build-list entry. Players see where tools come from in
`HELP CRAFTING` and `HELP CRAFTING-STATIONS`.
