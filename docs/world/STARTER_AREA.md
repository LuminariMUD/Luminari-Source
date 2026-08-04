# Starter Area Topology

The minimal world bundle provides a sane three-room loop that lets fresh
databases boot without importing the full production world. These rooms cover
the default mortal and immortal start vnums defined in `src/config.c`.

## Where it lives

The bundle is checked in at `lib/world/minimal/` as a **flat** directory - all
files sit side by side, with no `wld/`, `mob/`, `obj/`, or `zon/` subdirectories:

```
lib/world/minimal/
    0.wld  0.mob  0.obj  0.zon  16.mob
    index.wld  index.mob  index.obj  index.zon
    index.shp  index.trg  index.qst  index.hlq
```

`scripts/setup.sh` copies these into the live `lib/world/<type>/` directories on
a fresh install. The live directories themselves are **not** version controlled -
`.gitignore` excludes `lib/world/wld/*.wld` and its siblings, because those files
are edited in-game through OLC. If you are looking for the starter rooms in
`lib/world/wld/0.wld` and the file is missing or unfamiliar, you are looking at
a working copy, not the tracked source.

## Rooms

| VNUM | Name               | Notes                                                                    |
|------|--------------------|--------------------------------------------------------------------------|
| 0    | The Void           | Fallback room used by legacy tooling; accessible but not player-facing. |
| 3000 | Arrival Platform   | Entry point for mortal characters; links east into the Hall of Beginnings. |
| 3001 | Hall of Beginnings | Main staging chamber; west returns to the platform, east leads to the nexus. |
| 3002 | Immortal Nexus     | Immortal login target; west returns to the hall.                         |

Key characteristics:

- Every room records wilderness coordinates (`C` lines) bounded within a small grid.
- Direction data is fully populated so MSDP and automappers resolve exits without crashing.
- The zone definition (`lib/world/minimal/0.zon`) spans VNUMs `0-3099`, matching
  the room layout. Its full contents are a zone named `The Void` with a 30 minute
  lifespan, reset mode 2, and no reset commands.

When expanding the starter world, keep VNUMs in ascending order and ensure the
zone range continues to encapsulate them; `parse_room()` assumes sorted input.
See the [Zone File Format Reference](../world_game-data/ZONE_FILE_FORMAT.md) for
the header and reset-command syntax.
