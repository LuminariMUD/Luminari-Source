# Craft trainer

This mobile prototype places the paid craft trainer from
https://github.com/LuminariMUD/Luminari-Source/issues/196 in the Sanctus III
crafting district. The design is described under "Craft trainers" in
[CRAFTING_SYSTEM_NOTES.md](../../docs/world_game-data/CRAFTING_SYSTEM_NOTES.md#craft-trainers),
and the tunables are constants in `src/craft/craft_training.h`.

## Prototype record

`3.mob` holds one record, mobile 373, followed by the `$~` terminator.

| Field | Value |
| -- | -- |
| Keywords | artisan master trainer |
| Short description | the master artisan |
| Special procedure | `SpecProc: Craft Trainer` |
| Flags | sentinel, uncharmable, unsummonable, unkillable, does not fight (8675338) |
| Level | 15, standing, male human |

The flags match the district's other service mobiles, such as Sazzy (mobile 374). The procedure
binds through the world file, so the record needs no `MOB_SPEC` flag: the trainer answers only the
`apprentice` command.

## World installation

The main source repository keeps authored additions here because each site's
live world is maintained separately. Before installing, confirm that the site's
world has no mobile 373 and no reset that loads one.

1. Merge the record into `lib/world/mob/3.mob` in ascending VNUM order (between
   371 and 374 in the development world), preserving the other records and the
   final `$~` terminator.

2. Add this reset to `lib/world/zon/3.zon`, with the other mobile resets, to
   place the trainer in room 373, the Crafting Benches room north of room 372
   (east of the Slanting Passageway, room 368):

   ```text
   M 0 373 1 373 100 	(the master artisan)
   ```

3. Validate the merged world, then load it through the site's usual world-data
   release procedure.

Alternatively, build the trainer in game: `medit 373`, set the descriptions,
flags, and level, choose `Craft Trainer` from the special procedure menu (Z),
save, and add the reset with `zedit`.

```sh
python3 scripts/world/wtool.py validate --paths data/craft-trainers/3.mob --strict
```

Like `data/harvest-tools`, the bundle has no installer script and no build-list
entry. Players use `apprentice` beside the trainer; see `HELP APPRENTICE`.
