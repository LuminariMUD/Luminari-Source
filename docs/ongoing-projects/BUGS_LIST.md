# Verified Local Code Bug Backlog

This file contains bugs whose fixes belong in the dev/local source tree. Any
item requiring a change under `lib/`, in a help file, or in production help
data is tracked separately in [LIB-BUGS.md](LIB-BUGS.md).

The backlog was reviewed against source revision `ff3bf9e9` on 2026-08-03,
then split by fix ownership. This file contains 3 actionable code items drawn
from 3 production records. `LIB-BUGS.md` contains 14 production-owned items
drawn from 17 records.

Across both files, 20 unique records remain from the original 146-record
snapshot. The other 126 were removed because they are resolved, intentional,
contradicted by current code or data, or cannot be tied to a current defect.

This was a source-based review rather than a live reproduction pass. Reproduce
each symptom before implementing its fix. Reporter metadata uses `L` for
character level, `R` for room VNUM, and `#` for the record's position in the
2026-08-03 production snapshot.

## 2025

- **Magic Fang rejects wild-shaped player characters** - Magic Fang and
  Greater Magic Fang require an animal target, but their validation does not
  accept a player with the wild-shape state. Reporter: Luos (L9, R40431,
  2025-12-14; #131).

## 2026

- **Eidolon Basic Magic is blocked by the generic NPC slot check** - NPC
  casting rejects an eidolon with no ordinary spell slots before reaching the
  Basic Magic evolution exception, making the purchased evolution unusable.
  Reporter: Grimthur (L10, R29002, 2026-04-14; #137).
- **Follower persistence drops type-specific state** - Logout saves every
  charmed NPC with only its prototype, basic stats, descriptions, and objects.
  Login recreates hired, summoned, and companion followers through one generic
  loader without their original affects or type-specific runtime state,
  providing a current path to the reported inconsistent states. Reporter:
  Gerok (L9, R6746, 2026-07-28; #144).
