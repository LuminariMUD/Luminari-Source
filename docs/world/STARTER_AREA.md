# Starter Area Topology

The minimal world bundle checked into `lib/world/` provides a sane three-room
loop that lets fresh databases boot without importing the full production
world. These rooms cover the default mortal and immortal start vnums defined
in `src/config.c`.

| VNUM | Name               | Notes                                                                    |
|------|--------------------|--------------------------------------------------------------------------|
| 0    | The Void           | Fallback room used by legacy tooling; accessible but not player-facing. |
| 3000 | Arrival Platform   | Entry point for mortal characters; links east into the Hall of Beginnings. |
| 3001 | Hall of Beginnings | Main staging chamber; west returns to the platform, east leads to the nexus. |
| 3002 | Immortal Nexus     | Immortal login target; west returns to the hall.                         |

Key characteristics:

- Every room records wilderness coordinates (`C` lines) bounded within a small grid.
- Direction data is fully populated so MSDP and automappers resolve exits without crashing.
- The zone definition (`lib/world/zon/0.zon`) now spans VNUMs `0–3099`, matching the new room layout.

When expanding the starter world, keep VNUMs in ascending order and ensure the
zone range continues to encapsulate them; `parse_room()` assumes sorted input.
