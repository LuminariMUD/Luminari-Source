# Verified Production Lib Bug Backlog

This file contains bugs whose fixes belong on the production/remote server in
`lib/`, production help data, world files, quest data, DG Scripts, or shop
data. These are not dev/local source-code tasks. Verify the live production
record and take an appropriate backup before changing it.

The entries were separated from [BUGS_LIST.md](BUGS_LIST.md) after the
end-to-end review of source revision `ff3bf9e9` on 2026-08-03. This file
contains 13 production-owned work items drawn from 16 production records.
Records #111 and #114 also appear in `BUGS_LIST.md` because the Ghost Wolf
reports require one source-code fix and one mobile-data fix.

File paths below identify the corresponding local mirror. Production data is
authoritative, so confirm the live record and its surrounding data before
editing. This was a source/data review rather than a live reproduction pass.

Reporter metadata uses `L` for character level, `R` for room VNUM, and `#`
for the record's position in the 2026-08-03 production snapshot.

## 2022

- **Hood of Swirling Clouds is configured as heavy full-plate armor** - Object
  144674 uses the full-plate-head armor type even though its name and
  description present it as a hood. Production surface:
  `lib/world/obj/1445.obj`. Reporter: Ertai (L28, R23831, 2022-12-17; #024).

## 2023

- **Rum Runners retains placeholder quest information** - Quest 20602's long
  information field still says that there is no information, leaving the
  normal quest-information display without useful directions. Production
  surface: `lib/world/qst/206.qst`. Reporters: Talendor (L25, R105096,
  2023-01-26; #032), Neurrone (L23, R105096, 2023-03-26; #052), and Galeron
  (L20, R1004010, 2023-12-19; #095).
- **The Underdark workbench has no working push action** - Room 40629 resets
  switch object 40608, but the object has no attached command trigger and no
  production-data action that makes `push workbench` operate it. Production
  surfaces: `lib/world/obj/406.obj`, `lib/world/trg/406.trg`, and
  `lib/world/wld/406.wld`. Reporter: Ogoun (L30, R40629, 2023-04-14; #059).
- **Finger of Death help gives the wrong magic-resistance behavior** - The
  implemented spell permits spell resistance, while production help says
  `Magic Resist: No`. Update production help to say that resistance applies.
  Production surface: production help data and `lib/text/help/help.hlp`.
  Reporter: Mallyrn (L21, R1004000, 2023-09-27; #089).

## 2024

- **The Newbie Quest 4 target can be killed during Quest 3** - Skirker 14110 is
  zone-loaded before quest 14102 is accepted. The one-instance target can be
  killed early, and quest acceptance has no recovery or respawn path.
  Production surfaces: `lib/world/zon/141.zon` and
  `lib/world/qst/141.qst`. Reporter: Asterix (L1, R14109, 2024-08-14; #099).
- **First Step Is the Hilt returns the objective instead of a hilt** - Quest
  6703 consumes object 6712, narrates its transformation into a hilt, and then
  rewards the same chunk-of-wood object 6712. Production surfaces:
  `lib/world/qst/67.qst` and `lib/world/obj/67.obj`. Reporter: Duirren (L7,
  R6777, 2024-09-07; #105).

## 2025

- **Crafting tutorial triggers can be re-entered to duplicate supplies** -
  Trigger 14107 delays before setting its completion guard, so repeated
  `say ready` activations can schedule duplicate rings. Trigger 14109 has no
  one-time guard before loading copper. Production surface:
  `lib/world/trg/141.trg`. Reporter: Mddljeu (L2, R14107, 2025-07-05; #108).
- **Ghost Wolf mobile data does not make the summon mountable** - Mobile 801
  lacks `MOB_MOUNTABLE` despite the spell's intended mount behavior. The
  separate source bug that applies mobility flags to the caster remains in
  `BUGS_LIST.md`. Production surface: `lib/world/mob/8.mob`. Reporter:
  Syman (L7/L18, R145352/R148130, 2025-11-26/27; #111, #114).
- **The Underdark boulder remains visible after it is pushed** - Switch object
  102600 opens the north route but is neither moved nor given a post-use room
  description, so it continues to appear against the wall. Production surface:
  `lib/world/obj/1025.obj`. Reporter: Syman (L19, R102516, 2025-11-27; #115).
- **Damp Stone Passage's upward exit is not hidden** - Room 5931 defines the up
  exit as a door without the hidden flag, contradicting the quest instruction
  to search for concealed floorboards. Production surface:
  `lib/world/wld/59.wld`. Reporter: Tsoli (L9, R5931, 2025-11-30; #122).
- **Ravanda's Shield tutorial listens for Mage Armor instead** - Trigger 14117
  waits for Mage Armor's room message, while the tutorial instructs the Wizard
  to cast Shield, whose message cannot satisfy that trigger. Production
  surface: `lib/world/trg/141.trg`. Reporter: Frondbrau (L1, R14104,
  2025-12-05; #127).

## 2026

- **Crisrion is described as a shopkeeper but has no shop record** - Mobile
  103803 is loaded into room 103498, whose text directs players to `list` and
  `buy`, but no shop definition registers that mobile. Production surfaces:
  `lib/world/mob/1030.mob`, `lib/world/wld/1030.wld`, and
  `lib/world/shp/1030.shp`. Reporter: Elumanate (L8, R103498, 2026-03-25;
  #135).
- **`help arcana` selects Arcana Golem before the Arcana skill** - The help
  entries have `ARCANA-GOLEM` and `ARCANA-SKILL` keywords but no exact
  `ARCANA` keyword. Current ordering therefore returns the Golem entry first.
  Production surface: production help data and `lib/text/help/help.hlp`.
  Reporter: Mjodvitnir (L16, R11809, 2026-07-26; #143).
