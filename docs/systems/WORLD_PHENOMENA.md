# World Phenomena and Sensory Events

## Purpose

`WorldPhenomenon` is the native typed-domain-event boundary for things players
may see or hear beyond their current location. Examples include epic spells,
airships, flying dragons, major weather, terrain changes, explosions, and
fighting in a nearby room.

The gameplay owner publishes one immutable fact when the phenomenon occurs.
The synchronous spatial subscriber then finds eligible active observers,
delivers player-facing sight or sound, and publishes one typed perception fact
for each admitted observer. There is no durable message row, string topic,
delivery retry queue, or heartbeat scan.

## Contract

The payload in `src/domain_event_types.h` carries:

- a generation-aware origin-room handle for room propagation;
- source coordinates and elevation for coordinate propagation;
- independent visual and audio ranges;
- visual, audible, or combined channels;
- intensity and audio frequency;
- borrowed visual and audio descriptions; and
- an explicit coordinate or room-graph propagation mode.

Descriptions are borrowed only for synchronous publication. The bus and
subscriber do not retain or log their content.

## Propagation

Coordinate mode uses the existing visual/audio strategy engine. It evaluates
materialized observers in wilderness zones and applies range, elevation,
terrain, weather, obstruction, and intensity rules. NPCs are admitted for AI
response; connected players are admitted to typed perception only when an
active quest requires the exact phenomenon kind.

Room mode performs a bounded breadth-first walk from the generation-validated
origin room. The maximum accepted radius is eight room hops and each
publication visits at most 256 rooms. Visual effects do not cross closed doors;
audible effects may. `minimum_range` lets a publisher omit the origin room when
normal local combat or spell messages already cover it.

Both modes run only in response to a publication. They do not poll dormant
rooms, characters, mobiles, or wilderness coordinates. Candidate and
publication caps keep each propagation bounded.

`PhenomenonPerceived` records the usable senses, distance, attenuated intensity
and whether the source is known through faction or sight. NPC behavior and
witnessed quest objectives consume that typed result; they never infer state
from the descriptive text.

## Current Publishers

Meteor Swarm publishes approach, descent, and impact phenomena. Approach and
impact combine visual and audible channels in one fact; descent is visual.
This preserves the existing long-range effect while moving its delivery behind
the typed event boundary.

Future owners should publish the same contract for airships, dragons, weather,
terrain alteration, nearby combat, and explosive spells. Each migration still
requires gameplay-specific range, wording, frequency, visibility, lifecycle,
and anti-duplication tests.

## Retired System

The former `src/pubsub/` implementation was a database-backed player topic and
message system. It did not provide this typed gameplay contract. Phase 6b
removed that runtime, its heartbeat, commands, wilderness adapter, rename
hooks, and automatic schema setup. Its SQL tables remain deprecated and
untouched until a separately reviewed backup and removal migration is approved.
