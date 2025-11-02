# Wilderness System Documentation Index

## Primary Documentation Location

The comprehensive wilderness system documentation is located at:
**`/docs/world_game-data/wilderness_system.md`**

This document contains complete technical details about the LuminariMUD wilderness system including:
- System architecture and components
- Coordinate system (-1024 to +1024 grid)
- Terrain generation using Perlin noise
- Room management (static and dynamic)
- Regions and paths
- Resource system
- PubSub integration
- Spatial audio system
- Builder tools and guides
- Configuration and troubleshooting

## Related Documentation in This Folder

- `WILDERNESS-RESOURCE-PLAN.md` - Resource system planning and implementation phases
- `WILDERNESS_PROJECT.md` - Project development tracking and milestones

## Quick Reference

### World Map Generation
The current world map is generated using these Perlin noise seeds:
- **Elevation**: 822344
- **Moisture**: 834
- **Elevation Distortion**: 74233
- **Weather**: 43425

These seeds create the unique geography of Luminari's 2048x2048 wilderness grid.

### Key Files
- **Source Code**: `src/wilderness.c`, `src/wilderness.h`
- **Perlin Noise**: `src/perlin.c`, `src/perlin.h`
- **Resources**: `src/resource_system.c`, `src/resource_descriptions.c`

For complete information, see the main documentation at `/docs/world_game-data/wilderness_system.md`