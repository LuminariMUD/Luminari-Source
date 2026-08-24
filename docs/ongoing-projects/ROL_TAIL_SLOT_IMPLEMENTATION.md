# RoL Tail Equipment Slot Implementation

Status: runtime and converter implemented; help, documentation, and final validation remain

## Progress log

### 2026-08-25 - RoL converter checkpoint implemented

- Corrected source `ITEM_WEAR_TAIL` conversion from the previous erroneous
  about-body mapping to appended target wear bit 34.
- Classified source tail objects before normalization: the active corpus has
  12 dedicated non-ring tail items and one finger-and-tail ring (object
  32644).
- Normalized dedicated tail objects to take-plus-tail only, while source tail
  rings remain ordinary target rings and receive tail eligibility from the
  runtime ring rule.
- Mapped source equipment position 25 to target `WEAR_TAIL` and recovered the
  known source position-24 reset defect using the referenced object's wear
  classification.
- Threaded the classification through pilot, capability-audit, and Phase 7
  generation paths and added converter regression coverage.

The 135-test focused RoL transform suite passes at this checkpoint. Pending:
builder/player documentation, synchronized help, final world-tool and full
production test/install validation, and subsequent commits/pushes.

### 2026-08-25 - Core runtime checkpoint implemented

- Appended `WEAR_TAIL` and `ITEM_WEAR_TAIL` without renumbering existing
  equipment positions or object wear flags.
- Added the Yuan-Ti tail-anatomy predicate and enforced it in both command and
  direct `equip_char()` paths, including NPC/reset equipment.
- Defined the target ring predicate from the existing authoritative
  `ITEM_WEAR_FINGER` classification used by crafting, treasure, and zone-check
  code.
- Added ring-or-dedicated-tail compatibility, dedicated tail-only enforcement,
  explicit/default `wear ... tail` selection, equipment display, persistence,
  DG Script lookup, zone-editor selection, and RoL-compatible tail-armor AC.
- Added production-linked CuTest coverage for ring eligibility, dedicated
  tail-only gear, anatomy rejection, direct equip, persistence restore, and AC.

The production build and the 899-test CuTest executable pass at this checkpoint,
and the checked-in world constants manifest has been refreshed.

RoL source root:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari`

> **IMPORTANT - PLANNED LUMINARIMUD CONTRACT**
>
> RoL behavior is source evidence, not the final target rule. The LuminariMUD
> implementation will use these rules:
>
> 1. A non-ring item explicitly marked `ITEM_WEAR_TAIL` is dedicated tail gear
>    and is tail-only. It cannot be worn in another equipment position, even if
>    imported data carries another wear flag.
> 2. Any ring can be worn on the tail by a character that has a tail slot. An
>    ordinary ring does not need `ITEM_WEAR_TAIL` and remains eligible for a
>    finger slot as well.
> 3. The ring rule wins for imported RoL rings that also carry
>    `ITEM_WEAR_TAIL`: they remain finger-or-tail rings, not tail-only gear.
> 4. Characters without a tail slot cannot equip either dedicated tail gear or
>    a ring on the tail.
>
> The implementation must use the target game's authoritative ring predicate;
> it must not silently treat every generic finger-wearable object as a ring
> unless that predicate defines rings that way.

## Runtime contract

RoL implements the tail as one real character equipment position, restricted to
Yuan-Ti characters.

| Component | RoL value | Authority |
|---|---:|---|
| Object capability | `ITEM_WEAR_TAIL` (`BIT_23`, value `4194304`) | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/structs.h:129` and `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/structs.h:355` |
| Equipment position | `WEAR_TAIL` (`25`) | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/structs.h:1146` |
| Equipment capacity | `MAX_WEAR` (`32`) | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/config.h:152` |
| Highest active position | `CUR_MAX_WEAR` (`25`) | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/structs.h:1148` |
| Display label | `<worn on tail>` | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/constant.c:975` |

`CUR_MAX_WEAR` is used as the highest valid zone equipment position, while
`MAX_WEAR` is the allocated `char_data.equipment[]` capacity. These meanings
must not be collapsed into one count during a port.

## Equip flow

The command path is implemented in
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c`:

1. `do_wear()` recognizes the explicit location `tail` and maps it to internal
   wear keyword `20`
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4320`).
   An item carrying `ITEM_WEAR_TAIL` also selects keyword `20` when no location
   is supplied
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4371`).
2. Before the position switch, `wear()` rejects keyword `20` unless the wearer
   is `RACE_YUANTI`
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:3242`).
3. Case `20` verifies `ITEM_WEAR_TAIL`, requires `equipment[WEAR_TAIL]` to be
   empty, removes the item from inventory, and calls
   `equip_char(ch, obj, WEAR_TAIL, ...)`
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4228`).
4. `wear all` and reset fallback include
   `{ITEM_WEAR_TAIL, 20, WEAR_TAIL}` in `equipment_pos_table`
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4288`).
   `try_wear()` also tests the tail flag
   (`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:5127`).

Removal needs no tail-specific branch. The generic `unequip_char()` path clears
the slot and recalculates affects in
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/handler.c:1921`.

## Rings and dual-location items

RoL does not decide that an item is tail gear from its name or object type. A
ring can be worn on the tail when its wear bitvector includes
`ITEM_WEAR_TAIL`. It can support both locations when it also includes
`ITEM_WEAR_FINGER`.

Object 32644, `a studded diamond ring`, is the direct example at
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/areas/obj/avernus.obj:438`:

```text
9 32832 4194307
```

The wear value `4194307` decodes to
`ITEM_TAKE | ITEM_WEAR_FINGER | ITEM_WEAR_TAIL`. Consequently:

- `wear ring finger` selects an ordinary finger slot.
- `wear ring tail` selects `WEAR_TAIL` and is allowed only for a Yuan-Ti.
- `wear ring` selects the tail because the no-location scan tests the tail flag
  after the finger flag at
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4371`.
  A non-Yuan-Ti must explicitly select `finger` or the later tail selection is
  rejected by the race check.
- `wear all` attempts the tail before either finger because tail precedes the
  finger entries in `equipment_pos_table` at
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actobj.c:4292`.

RoL also directly equips object 32644 at runtime position 25 in
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/areas/zon/avernus.zon:581`.

## Gameplay effects and display

- The character equipment display explicitly includes position 25 in
  `wear_order` at
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/actinf.c:887`.
- If the tail item is `ITEM_ARMOR`, `apply_ac()` returns its `value[0]` AC at
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/handler.c:1737`.
- Ordinary object affects and wear procedures are handled by the generic
  `equip_char()` path. The tail slot has no special attack or proc behavior.
- Tail-wearable objects are included in the auction system's wearable-item mask
  at
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/newauction.c:1093`.

## World data and persistence

The flat object format stores the tail capability in the normal wear bitvector.
For example, object 50959 in
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/areas/obj/bgdruids.obj:115`
contains:

```text
9 0 4194305
```

Here `4194305` is `ITEM_TAKE | ITEM_WEAR_TAIL`. The loader assigns the third
numeric field directly to `obj->wear_flags` in
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/db.c:3263`.

Player equipment persistence is position-preserving:

- Saving writes an equipped position as `index + 1` in
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/files.c:1335`.
- Loading restores the saved index, then maps position 25 back to wear keyword
  20 through `restore_wear[]` in
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/files.c:545`.

## RoL compatibility traps

1. **RoL zone examples use position 24 for tail gear.** For example,
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/areas/zon/hyssk.zon:600`
   equips a tail item with `arg3 = 24`, although the runtime tail position is
   25. The first requested placement therefore fails as an insignia, after
   which `reset_zone()` calls `try_wear()` and recovers from the object's tail
   flag. This fallback is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/db.c:3853`.
2. **`equipment_types[]` is shifted after the primary weapon entry.** It omits
   a secondary-weapon label, placing `Worn on tail` at table index 24 rather
   than runtime position 25. See
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/constant.c:1394`.
3. **The SQL export helpers omit `ITEM_WEAR_TAIL`.** Their wear columns end at
   `ITEM_GUILD_INSIGNIA` in
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/mysql.obj.c:125`
   and
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/RealmsOfLuminari/src/mysql.pfile.c:100`.
   Flat world objects and binary player saves support the slot; these SQL
   export paths do not preserve its wear flag.

## LuminariMUD port surface

A faithful implementation should append, rather than insert, a target equipment
position and object wear flag so existing numeric data remains stable. It must
then update the synchronized position/flag counts and tables, `wear` selection,
equipment display, object save validation, AC handling, zone conversion, OLC,
help, and production-linked tests.

The target must provide an authoritative character predicate for whether a tail
slot exists and an authoritative item predicate for whether an object is a ring.
Equip selection must enforce the prominent target contract above: dedicated
non-ring tail gear is tail-only, while every ring gains tail eligibility for a
character with that slot.

Converter logic should identify source tail gear from `ITEM_WEAR_TAIL`, not
trust the inconsistent RoL zone position 24. It should classify rings before
normalizing dedicated tail gear. Non-ring tail items become tail-only target
data, while RoL rings that carry both finger and tail flags remain rings and
receive tail eligibility from runtime ring handling rather than from
`ITEM_WEAR_TAIL`.
