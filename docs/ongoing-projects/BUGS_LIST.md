# Verified Local Code Bug Backlog

This file contains bugs whose fixes belong in the dev/local source tree. Any
item requiring a change under `lib/`, in a help file, or in production help
data is tracked separately in [LIB-BUGS.md](LIB-BUGS.md).

The backlog was reviewed against source revision `ff3bf9e9` on 2026-08-03,
then split by fix ownership. This file contains 9 actionable code items drawn
from 10 production records. `LIB-BUGS.md` contains 14 production-owned items
drawn from 17 records. Records #111 and #114 appear in both files because the
Ghost Wolf reports require separate source-code and mobile-data fixes.

Across both files, 25 unique records remain from the original 146-record
snapshot. The other 121 were removed because they are resolved, intentional,
contradicted by current code or data, or cannot be tied to a current defect.

This was a source-based review rather than a live reproduction pass. Reproduce
each symptom before implementing its fix. Reporter metadata uses `L` for
character level, `R` for room VNUM, and `#` for the record's position in the
2026-08-03 production snapshot.

## 2025

- **Rogues lack composite shortbow proficiency** - Rogue weapon handling
  includes shortbows but omits both composite shortbow types, unlike the
  corresponding Assassin handling. Reporter: Kazne (L2, R145207, 2025-09-18;
  #110).
- **Ghost Wolf mobility flags are applied to the caster** - The summon path
  sets the wolf's water-walk and flight flags on the caster rather than the
  summoned mobile. Reporter: Syman (L7/L18, R145352/R148130,
  2025-11-26/27; #111, #114).
- **A spell cannot be aborted while it is being cast** - The casting gate tells
  the player to use `abort`, but `abort` is absent from the commands allowed
  through that same gate. Reporter: Syman (L24, R23807, 2025-11-29; #119).
- **Positive Channel Energy skips grouped living targets** - Its group
  condition skips a living target when that target is grouped with the caster,
  even though the caller supplies only the caster and group members. Reporter:
  Tsoli (L9, R145370, 2025-11-30; #121).
- **Blackguard spell lists are not recognized as spellcaster lists** -
  `IS_SPELLCASTER_CLASS` excludes Blackguard. With no other casting class the
  list is empty; after adding Warlock, even an explicit Blackguard request
  resolves to Warlock. Reporter: Falwel (L9, R40400, 2025-12-03; #126).
- **Boon Companion calculates but never applies its level bonus** - The summon
  path adds five to a local `level` after assigning the companion's level, and
  that adjusted value is not used before the companion is rolled. Reporter:
  Bijori (L9, R6758, 2025-12-13; #130).
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
