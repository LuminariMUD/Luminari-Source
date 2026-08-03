# Verified Local Code Bug Backlog

This file contains bugs whose fixes belong in the dev/local source tree. Any
item requiring a change under `lib/`, in a help file, or in production help
data is tracked separately in [LIB-BUGS.md](LIB-BUGS.md).

The backlog was reviewed against source revision `ff3bf9e9` on 2026-08-03,
then split by fix ownership. This file contains 1 actionable code item drawn
from 1 production record. `LIB-BUGS.md` contains 14 production-owned items
drawn from 17 records.

Across both files, 18 unique records remain from the original 146-record
snapshot. The other 128 were removed because they are resolved, intentional,
contradicted by current code or data, or cannot be tied to a current defect.

This was a source-based review rather than a live reproduction pass. Reproduce
each symptom before implementing its fix. Reporter metadata uses `L` for
character level, `R` for room VNUM, and `#` for the record's position in the
2026-08-03 production snapshot.

## 2026

- **Follower persistence drops type-specific state** - Logout saves every
  charmed NPC with only its prototype, basic stats, descriptions, and objects.
  Login recreates hired, summoned, and companion followers through one generic
  loader without their original affects or type-specific runtime state,
  providing a current path to the reported inconsistent states. Reporter:
  Gerok (L9, R6746, 2026-07-28; #144).
