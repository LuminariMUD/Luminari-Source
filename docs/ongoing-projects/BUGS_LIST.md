# Verified Local Code Bug Backlog

This file contains bugs whose fixes belong in the dev/local source tree. Any
item requiring a change under `lib/`, in a help file, or in production help
data is tracked separately in [LIB-BUGS.md](LIB-BUGS.md).

The backlog was reviewed against source revision `ff3bf9e9` on 2026-08-03,
then split by fix ownership. This file contains 15 actionable code items drawn
from 17 production records. `LIB-BUGS.md` contains 14 production-owned items
drawn from 17 records. Records #111 and #114 appear in both files because the
Ghost Wolf reports require separate source-code and mobile-data fixes.

Across both files, 32 unique records remain from the original 146-record
snapshot. The other 114 were removed because they are resolved, intentional,
contradicted by current code or data, or cannot be tied to a current defect.

This was a source-based review rather than a live reproduction pass. Reproduce
each symptom before implementing its fix. Reporter metadata uses `L` for
character level, `R` for room VNUM, and `#` for the record's position in the
2026-08-03 production snapshot.

## 2023

- **Kill output can award XP and then report zero XP** - Damage-based XP can
  bring a character to the XP cap before the final solo award. The final path
  then prints the zero returned by `gain_exp`, producing contradictory lines.
  Reporter: Gor (L12, R40428, 2023-04-02; #053).
- **Practiced Sneak does not make Stealth a class skill** - The Shade racial
  feat's help promises class-skill treatment, but the class-skill and study
  checks do not recognize the feat and continue charging cross-class cost.
  Reporters: Moriens (L1, R145202, 2023-04-09; #056) and Therius (L1, R14101,
  2023-08-20; #084).
- **Warlock Darkness lasts fewer rounds than documented** - The invocation's
  duration is the character's Warlock level in rounds, while its help promises
  15 rounds. Reporter: Aelin (L6, R145294, 2023-04-15; #060).
- **Crafting reports pre-modifier XP and can improve an unrelated skill** -
  Completion messaging prints base XP even though `gain_exp` applies the
  newbie bonus, and the legacy completion path can randomly improve a
  different crafting skill. Reporter: Zylese (L6, R370, 2023-08-19; #082).
- **Death effects can grant account XP without killing the target** - Recall
  Death and Psychic Crush calculate more than the generic 1,499 damage cap.
  Damage XP is awarded before death resolution, so a high-HP target can survive
  while still granting account XP. Reporter: Iliri (L19, R1004006,
  2023-09-20; #086).
- **Litany of Righteousness applies Dazzled to the caster** - In the
  good-caster/evil-target branch, the code builds the Dazzled affect for the
  victim but attaches it to `ch`, matching the reported self-debuff. Reporter:
  Gwyndaryn (L11, R40606, 2023-10-04; #092).

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
