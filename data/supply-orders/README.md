# Supply orders: quartermaster and crafting stations

These prototypes make the materials-and-motes supply order system (`supplyorder`,
`src/craft/crafting_new.c`) reachable in the Sanctus III crafting district. The
code needs two things that no tracked world record provided: a mobile flagged
QUARTERMASTER in the room for LIST, SELECT, REQUEST and COMPLETE, and an object
carrying the crafting-station extra flag that START and timed work recheck. The
same station objects also unblock ordinary NEWCRAFT projects, which require the
station for every skill except the station-free ones.

## Prototype records

`3.mob` holds one record, mobile 372, and `3.obj` holds six records, objects
363-368, each followed by the `$~` terminator. `3.zon` holds the seven resets.

| Record | Keywords | Short description | Extra flag |
| -- | -- | -- | -- |
| mobile 372 | quartermaster supply master | the crafting quartermaster | mob flag Quartermaster (99) |
| object 363 | forge anvil | a stone forge | Crafting-Forge (108) |
| object 364 | loom | a wooden loom | Crafting-Loom (107) |
| object 365 | tannery rack | a tanning rack | Crafting-Tannery (111) |
| object 366 | alchemy lab bench | an alchemy lab | Crafting-Alchemy-Lab (109) |
| object 367 | carpentry table workbench | a carpentry table | Crafting-Carpentry-Table (112) |
| object 368 | jewelcrafting station bench | a jewelcrafting station | Crafting-Jewelcrafting-Station (110) |

The mobile's flags (`bdnosx 0 0 d`) are sentinel, uncharmable, unsummonable,
unkillable, does not fight, plus Quartermaster; the first word matches the
district's other service mobiles such as the master artisan (mobile 373). The
mobile needs no special procedure: the general `supplyorder` command already
checks the flag. The stations are type Other with no wear flags, so they cannot
be picked up, and they map to the station each skill needs in
`get_required_crafting_station()`: forge for weaponsmithing, armorsmithing and
metalworking; loom for tailoring; tannery for leatherworking; alchemy lab for
alchemy; carpentry table for woodworking; jewelcrafting station for
jewelcrafting.

Everything loads into room 372, the central Crafting Benches room (north of
the Real Estate Office, room 374; west of room 375; south of the master
artisan in room 373; east of the Slanting Passageway, room 368). A player
can request, work and complete an order without leaving the room. The
quartermaster is deliberately not placed in room 370: that room's special
intercepts `supplyorder` for the legacy kit quest.

## World installation

The main source repository keeps authored additions here because each site's
live world is maintained separately. Before installing, confirm that the site's
world has no mobile 372, no objects 363-368, and no resets that load them.

1. Merge the record from `3.mob` into `lib/world/mob/3.mob` in ascending VNUM
   order (between 371 and 373 in the development world), preserving the other
   records and the final `$~` terminator.

2. Merge the six records from `3.obj` into `lib/world/obj/3.obj` in ascending
   VNUM order (between 362 and 370 in the development world).

3. Add the seven resets from `3.zon` to `lib/world/zon/3.zon`, with the other
   mobile and object resets, before the `S` line.

4. Validate the merged world, then load it through the site's usual world-data
   release procedure.

```sh
python3 scripts/world/wtool.py validate --paths data/supply-orders/3.mob data/supply-orders/3.obj --strict
python3 scripts/world/wtool.py validate --zone 3 --strict
```

Alternatively, build it in game: `medit 372` with the Quartermaster flag,
`oedit 363` through `368` with the matching Crafting-\* extra flag, and the
resets with `zedit`. Like `data/craft-trainers`, the bundle has no installer
script and no build-list entry. Players use `supplyorder` beside the
quartermaster; see `HELP SUPPLYORDER` and `HELP CRAFTING-STATION`.
