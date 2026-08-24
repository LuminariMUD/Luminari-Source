# Object File Format Mapping: Luminari vs Realms of Luminari

Status: research complete

Research date: 2026-08-18

This document maps every field of the Luminari `.obj` world file format and the
Realms of Luminari (RoL) `.obj` world file format. Every claim below was traced
to the loader or writer source in this repository. Field meanings that are
inferred rather than read directly out of a loader are labelled as such.

## Method and authority

The authority for each format is different, and this matters:

- Luminari has both a reader and a writer. `parse_object()` in `src/db.c:3684`
  reads the file; `save_objects()` in `src/olc/genobj.c:213` writes it. The two
  agree, so the on-disk format is fully round-trippable through OLC.
- RoL has a reader only. `read_object()` in
  `EXAMPLE/RealmsOfLuminari/src/db.c:3187` reads records lazily by seeking to
  an index offset. There is no object writer anywhere in the RoL tree; RoL
  areas are hand-authored and assembled by `build_areas.c`, which concatenates
  per-area files into `areas/world.obj` without parsing them. The reader is
  therefore the sole definition of the format.

Cross-checked against the existing converter grammar in
`scripts/world/wtool_lib/rol_source.py` (`_parse_obj`), which agrees with the
reader on every structural point, including the trailing-content quirk
documented below.

---

# Part 1: Luminari `.obj`

## 1.1 Record skeleton

A zone object file is a sequence of records, terminated by a line beginning
with `$`. Each record is:

```
#<vnum>
<aliases>~
<short description>~
<room description>~
<action description>~
<numeric line 1: type + flag words>
<numeric line 2: 16 object values>
<numeric line 3: weight cost rent level timer>
[extension blocks, each introduced by a single letter on its own line]
```

The record ends when the parser reads a line starting with `$` or `#`; that
line is returned to the caller and becomes the header of the next record
(`src/db.c:4231`).

## 1.2 String block

Four tilde-terminated strings, read in fixed order by `fread_string()`.

| Order | Field | Struct member | Load-time transform |
|-------|-------|---------------|---------------------|
| 1 | Aliases / keywords | `obj_proto[i].name` | none; NULL or malformed is fatal (`exit(1)`) |
| 2 | Short description | `short_description` | leading article `a`/`an`/`the` is lowercased (`src/db.c:3714`) |
| 3 | Room description | `description` | `CAP()` capitalizes the first letter |
| 4 | Action description | `action_description` | none |

### Colour code encoding

Every tilde-string passes through `parse_at()` (`src/helpers.c:22`) at the end
of `fread_string()` (`src/db.c:6592`). On disk the colour introducer is `@`;
in memory it is a tab (`\t`). A literal at-sign is escaped as `@@`, which
collapses to one `@`. The writer reverses this with `convert_from_tabs()`
(`src/utils.c:4909`).

So `@Da great iron axe@n` on disk is `\tDa great iron axe\tn` in memory. A
converter emitting Luminari `.obj` text must emit `@`, not `\t`, and must
double any literal at-sign in the source text.

## 1.3 Numeric line 1: type and flag words

Parsed at `src/db.c:3729`. The format accepts three token counts, and the count
alone selects the interpretation:

| Token count | Meaning |
|-------------|---------|
| 3 or 4 | Legacy pre-128-bit file. Token 0 is the item type; tokens 1..3 are single 32-bit extra / wear / affect words that get widened into element `[0]` of each array, with `[1..3]` zeroed. If `bitwarning` is TRUE in `config.c`, the server refuses to boot and tells the implementor to convert. Otherwise it converts in place and, if `bitsavetodisk` is set, queues the zone for rewrite. |
| 13 | Current baseline. Type plus 12 flag tokens: 4 extra, 4 wear, 4 affect. |
| 17 | Extended. The 13-token layout plus 4 more tokens for the second affect vector (`bitvector2`). |

Any other count is a fatal format error.

Field positions for the 17-token form:

| Position | Target | Notes |
|----------|--------|-------|
| 0 | `GET_OBJ_TYPE` | plain integer, `ITEM_*` |
| 1..4 | `GET_OBJ_EXTRA[0..3]` | `ITEM_*` extra flags, `EF_ARRAY_MAX 4` |
| 5..8 | `GET_OBJ_WEAR[0..3]` | `ITEM_WEAR_*`, `TW_ARRAY_MAX 4` |
| 9..12 | `GET_OBJ_AFFECT[0..3]` | `AFF_*`, `AF_ARRAY_MAX 4` |
| 13..16 | `GET_OBJ2_PERM[0..3]` | `AFF2_*`, aliased as `GET_OBJ_PERM2` / `GET_OBJ2_AFFECT` / `bitvector2` |

The 13-token form omits positions 13..16 and leaves `bitvector2` zeroed.

### Flag word encoding

Flag words are ASCII bitstrings, not decimal, unless they parse as a pure
number.

`asciiflag_conv()` (`src/db.c:2119`): `a`..`z` set bits 0..25, `A`..`Z` set bits
26..51. If every character is a digit (with an optional leading `-`), the token
is instead read as a decimal integer via `atol()`. The writer
`sprintascii()` (`src/olc/genolc.c:805`) only ever emits `a`..`z` plus `A`..`F`,
i.e. bits 0..31, and emits the literal `0` for an empty word. So a
freshly-written file uses at most 32 bits per word.

`asciiflag_conv_aff()` (`src/db.c:2143`) is the off-by-one legacy variant: it
maps `a` to bit 1 rather than bit 0. It is used only on the legacy 3/4-token
path for the affect word. Do not use it to interpret the 13- or 17-token form.

## 1.4 Numeric line 2: object values

Parsed at `src/db.c:3866`. Accepts exactly 4 or exactly 16 whitespace-separated
integers; anything else is fatal. The 4-value form is the old CircleMUD layout
and is zero-extended (the `t[]` array is cleared to `NUM_OBJ_VAL_POSITIONS`
zeroes first). `NUM_OBJ_VAL_POSITIONS` is 16 (`src/structs.h:5818`).

Values land in `GET_OBJ_VAL(obj, 0..15)`. Their meaning is entirely
type-dependent; see section 1.8.

## 1.5 Numeric line 3: economy and level

Parsed at `src/db.c:3892`. Accepts 3, 4, or 5 integers; missing trailing fields
default to 0.

| Position | Field | Notes |
|----------|-------|-------|
| 0 | `GET_OBJ_WEIGHT` | |
| 1 | `GET_OBJ_COST` | |
| 2 | `GET_OBJ_RENT` | `cost_per_day` |
| 3 | `GET_OBJ_LEVEL` | clamped to 1..30 at load (`src/db.c:3918`) |
| 4 | `GET_OBJ_TIMER` | |

The level clamp is silent and destructive. A file storing level 0 or level 40
will not round-trip.

## 1.6 Extension blocks

After line 3 the parser loops on single-character tokens. Every letter except
`E`, `T`, and `R` is followed by one line of integers.

| Token | Payload | Target |
|-------|---------|--------|
| `A` | one line, 2/3/4 ints: `location modifier bonus_type specific` | `obj->affected[j]`, `MAX_OBJ_AFFECT` is 6 |
| `B` | one line, 2 ints: `spellnum pages` | `obj->sbinfo[j]`, spellbook contents; allocates `SPELLBOOK_SIZE` (200) entries on first use |
| `C` | one line, 7 ints + optional word: `ability level activation_method value0..value3 [command_word]` | prepends a `struct obj_special_ability` |
| `E` | two tilde-strings: keyword, description | prepends an `extra_descr_data` |
| `G` | one int | `GET_OBJ_PROF` (`ITEM_PROF_*`, 0..10) |
| `H` | one int | `GET_OBJ_MATERIAL` (`NUM_MATERIALS` 60) |
| `I` | one int | `GET_OBJ_SIZE`; a stored 0 is rewritten to `SIZE_MEDIUM` (5) |
| `J` | one int | `mob_recepient`, vnum of the mob allowed to receive the item |
| `K` | one line, 5 ints: `level spellnum current_uses max_uses cooldown` | `obj->activate_spell[]` |
| `P` | none | accepted and ignored; a dead slot from an older AFF2 scheme |
| `R` | one raw line | `restring_identifier` |
| `S` | one line, 4 ints: `spellnum level percent inCombat` | `obj->wpn_spells[]`, also sets `has_spells`; `MAX_WEAPON_SPELLS` is 3 |
| `T` | the token line itself | passed whole to `dg_obj_trigger()`, DG script attachment |
| `Z` | one raw line | spec-proc name, resolved via `load_world_spec_binding()` |
| `$` / `#` | terminator | ends the record; `check_object()` runs, then the line is returned |

### Extension hazards

Three parser defects were found here and have since been fixed in `parse_object()`.
They are recorded because any file written before the fix may still carry the
data shapes that triggered them:

1. `A` and `B` shared the same counter `j`, so an object carrying both affects
   and spellbook pages indexed them into overlapping slots. `B` now uses its own
   `sbnum` counter.
2. The `A` handler's `specific` field read `t[3]` even when `sscanf` matched only
   2 or 3 arguments, storing whatever the previous record left in the `t[]`
   array. Both short forms now clear `t[3]` explicitly. The world validator
   reports the affected data shapes as `OBJ021`; 2863 prototypes in the
   development world still carry 2- or 3-integer `A` payloads, which now load
   deterministically as `specific = 0` instead of stale data.
3. The `S` handler's bounds check against `MAX_WEAPON_SPELLS` was commented out,
   so a file with more than three `S` blocks wrote past `wpn_spells[]`. The check
   is restored.

## 1.7 Flag namespaces

| Namespace | Count | Header location |
|-----------|-------|-----------------|
| `ITEM_*` item types | `NUM_ITEM_TYPES` 58 | `src/structs.h:4495` |
| `ITEM_*` extra flags | `NUM_ITEM_FLAGS` 125 | `src/structs.h:4882` |
| `ITEM_WEAR_*` | `NUM_ITEM_WEARS` 35 | `src/structs.h` |
| `AFF_*` | `NUM_AFF_FLAGS` 127 | `src/structs.h:1705` |
| `AFF2_*` | `NUM_AFF2_FLAGS` 5 | `src/structs.h:1709` |
| `APPLY_*` | `NUM_APPLIES` 75 | `src/structs.h:4975` |
| `ITEM_PROF_*` | `NUM_ITEM_PROFS` 11 | `src/structs.h:4613` |
| sizes | `NUM_SIZES` 10 | `src/structs.h:468` |
| materials | `NUM_MATERIALS` 60 | `src/structs.h:4697` |

Extra flags exceed 125 entries but the file only carries 4 words. With
`sprintascii()` writing 32 bits per word, the addressable range is 128 bits,
which is why `EF_ARRAY_MAX` is 4.

`AFF2_*` currently defines only five values, two of which
(`AFF2_ROL_SLOW_POISON`, `AFF2_ROL_DOCILE`) exist specifically as RoL
compatibility targets. `AFF2_DONTUSE` is bit 0 and is reserved.

## 1.8 Object value semantics

Authority: `display_item_object_values()` in `src/obj/act.item.c:106` and the
`oedit_disp_valN_menu()` family in `src/olc/oedit.c`. Selected types:

| Type | val 0 | val 1 | val 2 | val 3 |
|------|-------|-------|-------|-------|
| `ITEM_LIGHT` | unused | unused | hours left, `-1` = infinite | unused |
| `ITEM_SCROLL`, `ITEM_POTION` | spell level | spell 1 | spell 2 | spell 3 |
| `ITEM_WAND`, `ITEM_STAFF` | spell level | max charges | charges left | spellnum |
| `ITEM_WEAPON` | weapon type (`weapon_list` index) | num damage dice | dice size | attack-message index, used as `value[3] + TYPE_HIT` only when `weapon_list[].damageTypes` is empty (`src/combat/fight.c:11922`) |
| `ITEM_ARMOR` | AC apply, tenths | armor type (`armor_list` index) | | |
| `ITEM_FIREWEAPON` | index into the two-entry `ranged_weapons[]`, **not** `weapon_list[]` | damage | break chance percent | deprecated (`src/structs.h:4348`); `is_using_ranged_weapon()` tests `weapon_list[value[0]].weaponFlags` without checking item type, so objects of this type can never fire |
| `ITEM_MISSILE` | `AMMO_TYPE_*` 1..4, paired against the weapon type by `has_ammo_in_pouch()` | imbued spell number, cast through `call_magic()` on every shot (`fight.c:12057`); 0 = none | break probability percent, breaks when `value[2] >= dice(1, 100)` | unused; the imbue duration is the object timer |
| `ITEM_AMMO_POUCH` | capacity, counted in items not weight (`act.item.c:2028`) | container flags (`CONT_*`) | key vnum, `-1` for none | corpse flag |
| `ITEM_TRAP` | trap type | direction, or target obj vnum by trap type | effect (spellnum, or `trap_effects` offset by `TRAP_SPECIAL_PARALYSIS`) | trap DC; val 4 is the "detected" flag |
| `ITEM_SWITCH` | 0 push / 1 pull | affected room vnum | direction 0..5 | 0 unhide / 1 unlock / 2 open |
| `ITEM_FOOD`, `ITEM_DRINK` | duration | | | |

Weapon and armor stats are largely *not* stored in the object. Damage dice,
crit range, crit multiplier, damage types, range, and family come from
`weapon_list[val 0]`; AC bonus, max dex, armor check, spell fail come from
`armor_list[val 1]`. The file stores only the index.

Two weapon slots past val 3 carry meaning and have no `oedit` prompt of their
own: val 4 is the enhancement bonus (`GET_ENHANCEMENT_BONUS`, `src/utils.h:1596`,
clamped 0..10 in OLC) and val 5 is the loaded-ammo counter that
`weapon_is_loaded()` requires for crossbows and slings
(`src/combat/assign_wpn_armor.c:549`, decremented at `src/combat/fight.c:14278`).
Ammunition also reads val 4 as its enhancement bonus.

The remaining 40-odd types (crafting, vessels, portals, pets, blueprints,
outfits) each carry their own layout in the same switch. Treat
`display_item_object_values()` as the index when you need one of them.

## 1.9 Load-time mutations

`parse_object()` does not load the file verbatim. It applies these changes:

- `GET_OBJ_BOUND_ID` is forced to `NOBODY` regardless of file contents;
  prototypes never bind.
- Weapons have `ITEM_WEAR_HOLD` stripped from their wear flags, so a weapon
  can never be held rather than wielded.
- Object level is clamped to 1..30.
- Object size 0 becomes `SIZE_MEDIUM`.
- For `ITEM_DRINKCON` and `ITEM_FOUNTAIN` that are takeable, if weight is less
  than value 1 (capacity), weight is raised to capacity + 5.
- All `MAX_OBJ_AFFECT` affect slots are pre-cleared to `APPLY_NONE` / 0.

---

# Part 2: Realms of Luminari `.obj`

## 2.1 Record skeleton

RoL object files live in `areas/obj/<area>.obj` and are concatenated into
`areas/world.obj` by `build_obj_file()` (`build_areas.c:473`), which also
produces the offset table `lib/misc/lookup.obj`. Objects are then read
on demand: `read_object()` seeks to `obj_index[nr].pos` and parses one record.

```
#<vnum>
<aliases>~
<short description>~
<room description>~
<action description>~
<header row: type extra wear [anti]>
<values row: up to 8 ints>
<economy: weight cost durability>
[<affect word 1> [<affect word 2>]]
[E / A / T extension blocks]
```

There is no `$` sentinel requirement in the reader; parsing simply stops when
the record's own fields are exhausted. Because reads are offset-based and
never scan forward past the last field the reader wants, **any content after
the last recognized field is silently ignored** rather than being an error.

## 2.2 String block

Four tilde-terminated strings, same order as Luminari, read by RoL's
`fread_string()`.

| Order | Field | Load-time transform |
|-------|-------|---------------------|
| 1 | `name` (aliases) | every character is lowercased |
| 2 | `short_description` | if NULL, replaced with the literal `"No description"` |
| 3 | `description` | none |
| 4 | `action_description` | none |

All four strings are interned in `obj_index[]` (`keys`, `desc2`, `desc1`,
`desc3`) so every instance of a vnum shares one allocation. On the second and
later reads the strings are skipped with `skip_fread()`, not re-parsed.

Note the storage order in the index is `keys`, `desc2`, `desc1`, `desc3` -- the
index field names do not follow the file order.

Strings routinely embed RoL colour codes of the form `&+r`, `&+W`, `&N`, `&n`.
These are not part of the format proper but are pervasive in the corpus.

Observed corpus deviation: some records omit the action description entirely,
leaving three strings before the header row. The converter detects this by
probing whether the next content line is numeric with 3 or more integers, and
synthesizes an empty field (diagnostic `ROLOBJ005`).

## 2.3 Header row

`sscanf(Gbuf1, "%u %u %u %u\n", ...)` at `db.c:3263`. The fourth field is
optional -- its variable is pre-initialized to 0.

| Position | Field | Notes |
|----------|-------|-------|
| 0 | `obj->type` | `ITEM_*`, 1..40 |
| 1 | `obj->extra_flags` | 32-bit decimal bitmask, `BIT_1`..`BIT_32` |
| 2 | `obj->wear_flags` | 32-bit decimal bitmask |
| 3 | `obj->anti_flags` | optional; added 9/98; class/race/sex restrictions |

RoL flag words are plain decimal integers. There is no ASCII-letter flag
encoding anywhere in the RoL format. `BIT_1` is `1U`, so bit N of the file
value corresponds to `BIT_(N+1)`.

## 2.4 Values row

`sscanf(tmp_valuebuf, "%d %d %d %d %d %d %d %d", ...)` reading into
`obj->value[0..7]`. The buffer is 256 bytes, read with a single `fgets`, so the
row must be one physical line. Trailing values may be omitted; omitted slots
keep whatever `GetNewObj()` left there.

`value[]` is 8 wide in RoL versus 16 in Luminari.

## 2.5 Economy fields

Read with three separate `fscanf(" %d ")` calls, so these are whitespace-
delimited and **not line-bound** -- they may legally span lines.

| Order | Field | Notes |
|-------|-------|-------|
| 1 | `obj->weight` | for `ITEM_DRINKCON` only, the file value is divided by 4 at load; drink containers are stored in quarter-pound units |
| 2 | `obj->cost` | |
| 3 | `obj->durability` | historically `cost_per_day`, repurposed |

There is no level field and no timer field in the RoL object format.

## 2.6 Affect flag words

Two optional `fscanf(" %u ")` reads. `obj->sets_affs` is a 16-byte
(`AFF_BYTES`) bit array, and the conversion is deliberately one-based:

- word 1 bit `i` sets `sets_affs` bit `i + 1`, covering `AFF_BLIND` (1)
  through `AFF_CHARM`-and-beyond up to bit 32
- word 2 bit `i` sets `sets_affs` bit `i + 33`, covering bits 33..64

The second word is read only if the first succeeded. If the next token is not
numeric (typically `E`, `A`, or `T`), `fscanf` fails, returns 0, and both words
are left clear -- this is how records with no affects are represented.

Consequence: **`AFF_*` constants above 64 cannot be expressed in a RoL object
file.** The RoL `AFF_*` space runs past `AFF_DISPLACEMENT` (79), so roughly a
quarter of the affect namespace is unreachable from `.obj` source.

`AFF_HIDE` (21) is force-cleared immediately after load: no object may confer
hide.

## 2.7 Extension blocks

The reader uses `fscanf(" %s ")` to fetch each token, which skips arbitrary
whitespace including newlines. As a result the extension grammar is
**token-oriented, not line-oriented**. All of these are equivalent:

```
A
13 12
```
```
A 13 12
```
```
A 13
12
```

Both the one-line and two-line `A` forms occur in the shipped corpus.

Read order is fixed: all `E` blocks, then up to `MAX_OBJ_AFFECT` `A` blocks,
then at most one `T` block.

| Token | Payload | Target |
|-------|---------|--------|
| `E` | two tilde-strings: keyword, description | prepends an `extra_descr_data`; loops while the next token is `E` |
| `A` | 2 ints: `location modifier` | `obj->affected[i]`; **`MAX_OBJ_AFFECT` is 2** (`config.h:141`) |
| `T` | 6 ints: `trap_eff trap_dam trap_charge trap_level trap_dnum trap_dsize` | trap fields, read exactly once |

### The `A` modifier scaling in RoL vs Luminari

At `db.c:3372`, applies whose location is in `APPLY_STR`..`APPLY_CON` (1..5) or
`APPLY_AGI`..`APPLY_LUCK` (26..30) have their modifier multiplied by 45/10
(integer arithmetic) at load time in the RoL engine. RoL character stats ran on a
0..100 scale, while the area `.obj` files stored small integers. Because Luminari
operates on the standard D20 (3..18+) ability score scale, raw source file modifiers
are already at the appropriate D20 magnitude (+1, +2, +3, etc.) and are converted 1:1
without applying the 4.5x inflation factor. Converted item applies default to
`BONUS_TYPE_UNIVERSAL` (23).

### Ordering quirk

Because `A` is read before `T`, and because the reader stops after `T`, a
record that writes `T` before its `A` blocks loses those applies entirely. This
occurs in the shipped corpus -- `areas/obj/arndir.obj` has a record with

```
T
507 16 -1 16 0 0
A 3 1
A 27 2
```

where the two applies are dead data as far as the RoL server is concerned. The
converter classifies exactly this case as `IGNORED_SOURCE_CONTENT` with
diagnostic `ROLOBJ004`, which is the behavior-preserving reading.

## 2.8 Flag namespaces

All in `EXAMPLE/RealmsOfLuminari/src/structs.h` unless noted.

| Namespace | Range | Location |
|-----------|-------|----------|
| `ITEM_*` types | 1..40, `LAST_ITEM_TYPE` 40 | line 283 |
| wear flags | `BIT_1`..`BIT_23`, `BIT_17` unused | line 333 |
| extra flags | `BIT_1`..`BIT_32`, fully populated | line 356 |
| anti flags | `BIT_1`..`BIT_32`, fully populated | line 396 |
| `AFF_*` | 1..79+, only 1..64 file-addressable | line 1162 |
| `APPLY_*` | 0..52+ | line 1300 |
| `CONT_*` container flags | `BIT_1`..`BIT_5` | line 464 |
| `LIQ_*` | 0..28 | line 429 |
| `TRAP_*` damage types | 0..11, sparse | line 1049 |
| trap effect bits | `trap_bits[]`, 12 named + padding | `constant.c:1243` |

The anti-flag word is fully consumed: all 32 bits are assigned to specific
classes, races, or sexes, with a source comment noting the space is exhausted
("Seriously need to expand this to allow new races and classes in"). There is
no room to add a restriction without a format change.

Extra flags are also fully consumed at 32 bits.

## 2.9 Object value semantics

Authority: the `switch(j->type)` in RoL's `do_stat_object` at
`actwiz.c:1416`, plus `missile.h` for the ranged accessors.

| Type | value[] layout |
|------|----------------|
| `ITEM_LIGHT` (1) | 0 colour, 1 type, 2 hours |
| `ITEM_SCROLL` (2), `ITEM_POTION` (10) | 0 level, 1..3 spellnums, stored **1-based** (display subtracts 1 before the `spells[]` lookup) |
| `ITEM_WAND` (3), `ITEM_STAFF` (4) | 0 level, 1 max charges, 2 charges left, 3 spellnum, 1-based |
| `ITEM_WEAPON` (5) | 0 proc value, 1 num dice, 2 dice size, 3 weapon class 1..11, 1-based |
| `ITEM_FIREWEAPON` (6) | 0 unused, 1 damage, 2 dice size, 3 hit message, 4 firing delay, 5 max rate of fire, 6 durability 1..10, 7 ranged weapon type 1..21 |
| `ITEM_MISSILE` (7) | 0 num dice, 1 dice size, 2 durability 1..10 (10 best), 3 missile type 1..21 |
| `ITEM_ARMOR` (9) | 0 AC apply, 1 warmth, 2 prestige, 3 proc value |
| `ITEM_WORN` (11) | 1 warmth, 2 prestige |
| `ITEM_TRAP` (14) | 0 spell, 1 hitpoints |
| `ITEM_CONTAINER` (15) | 0 capacity, 1 lock type (`CONT_*`), 2 key vnum |
| `ITEM_NOTE` (16) | 0 tongue |
| `ITEM_DRINKCON` (17) | 0 capacity, 1 contents, 2 liquid (`LIQ_*`), 3 poisoned |
| `ITEM_KEY` (18) | 0 key type, 1 break chance, 2 proc value |
| `ITEM_FOOD` (19) | 0 fullness, 1 proc value, 3 poisoned |
| `ITEM_MONEY` (20) | 0 copper, 1 silver, 2 gold, 3 platinum |
| `ITEM_CORPSE` (24) | 0 weight held, 1 corpse class (`PC_CORPSE`/`NPC_CORPSE`/`ARENA_CORPSE`), 2 unlooted flag, 3 mob vnum |
| `ITEM_TELEPORT` (25) | 0 destination room vnum, 1 command index, 2 charges, 3 min level, 4 max level |
| `ITEM_POISON` (26) | 0 poison type, 1 level, 2 applications left, 3 hits per application |
| `ITEM_SUMMON` (27) | 0 command index, 1 mob vnum, 2 charges, 3 summon type |
| `ITEM_SWITCH` (29) | 0 command index, 1 room vnum with blocked exit, 2 direction 0..5, 3 switch type |
| `ITEM_QUIVER` (30) | 0 max missiles, 1 container flags, 2 key vnum, 3 quiver kind: `1` archery (holds `ITEM_MISSILE`), `2` throwing (holds thrown `ITEM_FIREWEAPON`) |
| `ITEM_PSP_CRYSTAL` (37) | 0 max charge, 1 current charge |
| `ITEM_DISTRIBUTION` (40) | 0 odds denominator, 1 room range low, 2 room range high, 3 sector bitmask |

Types not in the switch (`ITEM_SPELLBOOK`, `ITEM_INSTRUMENT`, `ITEM_COMMODITY`,
`ITEM_CART`, `ITEM_PICK`, `ITEM_CRUCIBLE`, `ITEM_CURE_COMPONENT`, `ITEM_SHIP`,
`ITEM_BOOK`, `ITEM_PEN`, `ITEM_BOAT`, `ITEM_TREASURE`, `ITEM_OTHER`,
`ITEM_TRASH`) fall to the default branch and their values are only ever shown
raw. Their meanings must be recovered from the systems that consume them, not
from the loader.

Two types are internal-only and must never appear in source files:
`ITEM_CORPSE` (24, explicitly commented "do NOT assign this type") and
`ITEM_MISSILE_INFLIGHT` (34).

### 2.9.1 Ranged type numbering

`ITEM_FIREWEAPON` value 7 and `ITEM_MISSILE` value 3 index the same one-based,
21-entry space. Authority: the layout comment at
`EXAMPLE/RealmsOfLuminari/src/missile.c:51-83` and the two name tables
`rangeweapons[]` (`missile.c:347`) and `missiles[]` (`missile.c:402`), which
list the launcher and the ammunition sharing each number.

| # | Launcher (`rangeweapons[]`) | Ammunition (`missiles[]`) |
|---|---|---|
| 1 | Short Bow | Normal Flight Arrow |
| 2 | Long Bow | Elven Flight Arrow |
| 3 | Elven Short Bow | Normal Sheaf Arrow |
| 4 | Elven Long Bow | Elven Sheaf Arrow |
| 5 | Hand Crossbow | Hand Crossbow Quarrel |
| 6 | Light Crossbow | Light Crossbow Quarrel |
| 7 | Heavy Crossbow | Heavy Crossbow Quarrel |
| 8 | Special Missile Weapon | Special Flight |
| 9 | Throwing Dagger | Throwing Dagger |
| 10 | Dart | Dart |
| 11 | Throwing Hammer | Throwing Hammer |
| 12 | Throwing Handaxe | Throwing Handaxe |
| 13 | Sling | Sling Stone |
| 14 | Javelin | Javelin |
| 15 | Spear | Spear |
| 16 | Blowgun | Blowgun Dart |
| 17 | Special Thrown Weapon | Special Throw |
| 18 | Common Object | Object |
| 19 | Scorpion Ballista | Ballista Missile |
| 20 | Catapult | Catapult Missile |
| 21 | Spell Missile | Spell Missile |

1-8 are the archery range and 9-18 the thrown range (`missile.h:17-21`).
`Fireweapon_MaxRange[]` (`missile.c:218`) carries the per-type range in the
source engine; the target derives range from `weapon_list[]` instead.

## 2.10 Load-time mutations

`read_object()` also mutates as it loads:

- `ITEM_SHIP` objects get `ITEM_LIT` forcibly set.
- `AFF_HIDE` is forcibly cleared.
- `ITEM_DRINKCON` weight is divided by 4.
- Stat applies are multiplied by 4.5 (section 2.7).
- Aliases are lowercased.
- Four item types auto-attach a spec proc if none is already bound:
  `ITEM_TELEPORT` to `item_teleport`, `ITEM_SWITCH` to `item_switch`,
  `ITEM_SUMMON` to `item_summon`, `ITEM_DISTRIBUTION` to `mob_distro_system`.
  This is type-driven behavior with no representation in the file at all.

---

# Part 3: Side-by-side

| Aspect | Luminari | RoL |
|--------|----------|-----|
| Reader | `src/db.c:3684` `parse_object()` | `EXAMPLE/.../src/db.c:3187` `read_object()` |
| Writer | `src/olc/genobj.c:213` `save_objects()` | none; hand-authored |
| Load timing | whole file at boot | lazy, per instance, via offset index |
| Record terminator | `$` or next `#`, enforced | none; reader stops when fields are satisfied |
| Structure | strictly line-oriented | token-oriented, whitespace-delimited |
| Strings | 4, tilde-terminated | 4, tilde-terminated, interned per vnum |
| Flag encoding | ASCII letters (`a`=bit 0) or decimal | decimal only |
| Extra flag width | 4 words / 128 bits | 1 word / 32 bits, fully used |
| Wear flag width | 4 words / 128 bits | 1 word / 32 bits |
| Affect width | 4 words plus 4 more for AFF2 | 2 words / 64 bits of a 128-bit array |
| Anti-class/race | encoded as `ITEM_ANTI_*` extra flags | separate 4th header field, its own 32-bit namespace |
| `value[]` slots | 16 | 8 |
| Applies per object | 6 | 2 |
| Apply record | location, modifier, bonus type, specific | location, modifier |
| Apply scaling | verbatim | stats rescaled x4.5 at load |
| Economy fields | weight, cost, rent, level, timer | weight, cost, durability |
| Object level | present, clamped 1..30 | absent |
| Traps | `ITEM_TRAP` type with `value[]` | orthogonal `T` block on any object |
| Extra descriptions | `E` blocks, any position in the extension list | `E` blocks, must precede `A` and `T` |
| Extension tokens | A B C E G H I J K P R S T Z | E A T |
| Material / size / proficiency | `H` / `I` / `G` blocks | not modelled |
| Spellbook contents | `B` blocks | not modelled |
| Weapon special abilities | `C` blocks | not modelled; RoL uses proc values in `value[]` |
| Weapon proc spells | `S` blocks | not modelled |
| DG script attachment | `T` blocks | not modelled; RoL uses named spec procs |
| Spec proc binding | explicit `Z` block | implicit, derived from item type |

## 3.1 Conversion hazards

Ranked by how quietly they corrupt data:

1. **The 4.5x apply scale.** Silent, systematic, and affects most stat gear.
   Section 2.7.
2. **1-based spell and weapon indices.** RoL stores spellnum+1 for scrolls,
   potions, wands, and staves, and weapon class 1..11 for weapons. Reading
   them as 0-based shifts every enchantment by one.
3. **Content after `T`.** Legal to write, invisible to the RoL server. Any
   converter that reads it is *adding* behavior, not preserving it.
4. **Anti-flags are a separate namespace.** RoL bit 1 is `ITEM_ANTI_WARRIOR`;
   Luminari has no fourth header field and expresses the same restriction as
   an extra flag. The two 32-bit spaces do not align at any bit.
5. **Extra flags collide numerically.** RoL `BIT_1` is `ITEM_GLOW`, which
   happens to match Luminari bit 0 `ITEM_GLOW`, but the agreement ends
   immediately: RoL `BIT_2` is `ITEM_NOSHOW` while Luminari bit 1 is
   `ITEM_HUM`. Never pass a flag word through unconverted.
6. **Affects above bit 64 are unreachable in RoL source**, so a round trip
   Luminari to RoL to Luminari cannot preserve them.
7. **Six applies down to two.** Any Luminari object with three or more applies
   has no RoL representation.
8. **DRINKCON weight units.** RoL stores quarter pounds for that one type.
9. **Level clamping.** Luminari silently pins object level to 1..30 on load,
   so an out-of-range value in a generated file is lost without a warning.

## 3.2 Source references

| Topic | File and line |
|-------|---------------|
| Luminari object reader | `src/db.c:3684` |
| Luminari object writer | `src/olc/genobj.c:213` |
| Luminari flag decode | `src/db.c:2119`, `src/db.c:2143` |
| Luminari flag encode | `src/olc/genolc.c:805` |
| Luminari value semantics | `src/obj/act.item.c:106` |
| Luminari OLC value prompts | `src/olc/oedit.c` |
| Luminari object constants | `src/structs.h:4495` onward |
| Luminari object struct | `src/structs.h`, `struct obj_flag_data` |
| RoL object reader | `EXAMPLE/RealmsOfLuminari/src/db.c:3187` |
| RoL area assembly | `EXAMPLE/RealmsOfLuminari/src/build_areas.c:473` |
| RoL value semantics | `EXAMPLE/RealmsOfLuminari/src/actwiz.c:1416` |
| RoL ranged accessors | `EXAMPLE/RealmsOfLuminari/src/missile.h:163` |
| RoL object constants | `EXAMPLE/RealmsOfLuminari/src/structs.h:275` onward |
| RoL object struct | `EXAMPLE/RealmsOfLuminari/src/structs.h`, `struct obj_data` |
| RoL `MAX_OBJ_AFFECT` | `EXAMPLE/RealmsOfLuminari/src/config.h:141` |
| Converter source grammar | `scripts/world/wtool_lib/rol_source.py`, `_parse_obj` |
| Converter emitter | `scripts/world/wtool_lib/rol_transform.py`, `emit_object` |

---

# Part 4: Worked examples

Two real records, one from each corpus, decoded field by field. They are the
same weapon: a frost-enchanted great axe from the Jotunheim area, with the same
alias list and the same extra description down to the line breaks.

They are **not** a converter input/output pair, despite the vnum offset of
100000 matching the conversion scheme. Luminari zone 1960 is its own
hand-built port of that area and predates the RoL conversion work. Verified two
ways: the converter's staged `1960.obj` is identical to the live file except for
the AFF2 widening from 13 to 17 header tokens, and the header flags do not
match what `OBJECT_EXTRA_MAP` would produce from 96000 (see section 4.3). The
pairing is still the clearest available illustration of how differently the two
formats encode one item.

## 4.1 Luminari object 196000

From `lib/world/obj/1960.obj`:

```
#196000
axe rimed frost frostbite~
@Da great iron axe @Crimmed @Dwith @Wfrost@n~
A glowing mithril axe rests on the ground.~
~
5 g gm 0 0 an 0 0 0 C 0 0 0 0 0 0 0
34 1 12 3 6 0 0 0 0 0 0 0 0 0 0 0
12 2000 0 30 0
C
11 15 24 0 0 0 0 frostbite
E
axe frostbite~
A huge glowing mithril axe.  Sparks of energy flicker through the
axe attesting to some inner power.  An intense feeling of cold
radiates from the axe, which is deathly cold to the touch.  The
blade of the axe is chiseled with scenes of great battles.  The
haft of the axe is wrapped in thick hides to provide a
comfortable grip.
~
G
4
H
7
I
6
J
0
```

### Header and strings

| Line | Raw | Meaning |
|------|-----|---------|
| `#196000` | vnum | Object virtual number. Zone 1960 owns 196000-196099. |
| `axe rimed frost frostbite~` | aliases | Four keywords a player can type to target the item. |
| `@Da great iron axe @Crimmed @Dwith @Wfrost@n~` | short desc | Inventory / equipment line. `@D` dark grey, `@C` cyan, `@W` bright white, `@n` reset. Each `@` becomes `\t` at load. Note the article is already lowercase, so the load-time article-lowercasing rule is a no-op here. |
| `A glowing mithril axe rests on the ground.~` | room desc | Shown when the item lies in a room. `CAP()` capitalizes the first character; it is already capital. |
| `~` | action desc | Empty. `fread_string()` returns NULL for an empty string, so `action_description` is NULL. |

### Numeric line 1: `5 g gm 0 0 an 0 0 0 C 0 0 0 0 0 0 0`

Seventeen tokens, so this is the extended form with AFF2.

| Token | Position | Decodes to |
|-------|----------|------------|
| `5` | item type | `ITEM_WEAPON` |
| `g` | extra[0] | bit 6 -> flag 6 = `ITEM_MAGIC` |
| `gm` | extra[1] | bits 6 and 12 of word 1 -> flags 38 and 44 = `ITEM_FLOAT`, `ITEM_AUTOPROC` |
| `0 0` | extra[2..3] | none |
| `an` | wear[0] | bits 0 and 13 -> `ITEM_WEAR_TAKE`, `ITEM_WEAR_WIELD` |
| `0 0 0` | wear[1..3] | none |
| `C` | affect[0] | uppercase `C` -> bit 26 + 2 = 28 -> `AFF_ELEMENT_PROT` |
| `0 0 0` | affect[1..3] | none |
| `0 0 0 0` | perm2[0..3] | no `AFF2_*` flags |

The wearer gets `AFF_ELEMENT_PROT` (endure elements) for free while the axe is
equipped -- that is what a permanent affect word on an object means.
`ITEM_AUTOPROC` marks the item as eligible for `proc_update()` polling, which is
how the frost special ability gets a chance to fire outside a direct hit.

Recall the writer only ever emits `a`-`z` and `A`-`F`, so `C` is bit 28 and
never anything higher.

### Numeric line 2: `34 1 12 3 6 0 0 0 0 0 0 0 0 0 0 0`

Sixteen values. For `ITEM_WEAPON`:

| Index | Value | Meaning |
|-------|-------|---------|
| 0 | 34 | `WEAPON_TYPE_GREAT_AXE`. Index into `weapon_list[]`. |
| 1 | 1 | number of damage dice |
| 2 | 12 | damage die size, so 1d12 |
| 3 | 3 | attack message index; `w_type = val3 + TYPE_HIT` (`src/combat/fight.c:11987`), and `attack_hit_text[3]` is "slash"/"slashes" |
| 4 | 6 | enhancement bonus, via `GET_ENHANCEMENT_BONUS` (`src/utils.h:1596`), so a +6 weapon |
| 5-15 | 0 | unused for this type |

Most of the axe's combat profile is *not* in this record. `weapon_list[34]`
(`src/combat/assign_wpn_armor.c:1147`) supplies 1d12, threat range 20, crit
x3, `WEAPON_FLAG_MARTIAL`, slashing damage, the axe family, suggested size
large, suggested material steel, handle and head types, and the flavour text.
The file stores index 34 and the dice are duplicated into values 1 and 2.

### Numeric line 3: `12 2000 0 30 0`

| Position | Value | Meaning |
|----------|-------|---------|
| 0 | 12 | weight; matches the great axe's suggested weight |
| 1 | 2000 | cost in coins |
| 2 | 0 | rent per real day |
| 3 | 30 | minimum level; sits exactly on the load-time clamp ceiling of 30 |
| 4 | 0 | timer, 0 = does not decay |

### Extension blocks

**`C` -- weapon special ability**, seven integers plus a command word:

| Field | Value | Meaning |
|-------|-------|---------|
| ability | 11 | `WEAPON_SPECAB_FROST` (`src/combat/spec_abilities.h:56`) |
| level | 15 | caster level the ability fires at |
| activation_method | 24 | bitmask: 8 = `ACTMTD_COMMAND_WORD`, 16 = `ACTMTD_ON_HIT` |
| value[0..3] | 0 0 0 0 | ability-specific parameters, unused by frost |
| command_word | `frostbite` | what the wielder utters to trigger it |

So the axe wreathes itself in frost either on a successful hit or on the spoken
word "frostbite" -- which is also why `frostbite` appears in the alias list and
in the extra-description keyword.

**`E` -- extra description**: keyword `axe frostbite`, then the multi-line body.
This is what `look at axe` prints.

**Single-value blocks:**

| Block | Value | Meaning |
|-------|-------|---------|
| `G` | 4 | `ITEM_PROF_MASTER` -- proficiency tier required to use it without penalty |
| `H` | 7 | `MATERIAL_STEEL` |
| `I` | 6 | `SIZE_LARGE`. A stored 0 would have been rewritten to `SIZE_MEDIUM`; 6 is kept. |
| `J` | 0 | `mob_recepient` -- no mob is designated to receive this item |

There is no `A` block: the axe grants no `APPLY_*` modifiers. Its power is in
the enhancement bonus, the permanent affect, and the frost ability.

The record ends at the next `#`, at which point `check_object()` runs and the
weapon has `ITEM_WEAR_HOLD` stripped (it was never set here).

## 4.2 Realms of Luminari object 96000

From `EXAMPLE/RealmsOfLuminari/areas/obj/jotun.obj`:

```
#96000
axe rimed frost frostbite~
&+La great iron axe&+C rimed&+L with&+W frost&N~
a glowing iron axe rests on the ground.~
~
5 473964608 8193
0 3 8 3
15 10000 0
E
axe frostbite~
A huge glowing iron axe.  Sparks of energy flicker through the
axe attesting to some inner power.  An intense feeling of cold
radiates from the axe, which is deathly cold to the touch.  The
blade of the axe is chiseled with scenes of great battles.  The
haft of the axe is wrapped in thick hides to provide a
comfortable grip.
~
A
18 3
A
19 3
```

### Header and strings

| Line | Raw | Meaning |
|------|-----|---------|
| `#96000` | vnum | Object virtual number, area `jotun`. |
| `axe rimed frost frostbite~` | aliases | Lowercased character by character at load; already lowercase. |
| `&+La great iron axe&+C rimed&+L with&+W frost&N~` | short desc | `&+L` bold black/dark grey, `&+C` cyan, `&+W` white, `&N` reset. RoL codes pass through `fread_string()` unmodified -- no `@`-to-tab transform exists in RoL. |
| `a glowing iron axe rests on the ground.~` | room desc | Note the lowercase `a`: RoL applies no `CAP()`, so what is written is what displays. |
| `~` | action desc | Empty. |

All four strings are interned into `obj_index[96000]` on first read and shared
by every instance of the axe.

### Header row: `5 473964608 8193`

Three fields, so the optional fourth anti-flag word is absent and
`obj->anti_flags` stays 0.

| Field | Value | Decodes to |
|-------|-------|------------|
| type | 5 | `ITEM_WEAPON` |
| extra_flags | 473964608 | bits set, in `BIT_n` 1-based numbering: 7, 14, 23, 27, 28, 29 |
| wear_flags | 8193 | 8192 + 1 = `BIT_14` + `BIT_1` = `ITEM_WIELD`, `ITEM_TAKE` |

Extra flags in full:

| Bit | Constant | Meaning |
|-----|----------|---------|
| `BIT_7` | `ITEM_MAGIC` | detects as magical |
| `BIT_14` | `ITEM_FLOAT` | floats rather than sinking |
| `BIT_23` | `ITEM_TWOHANDS` | requires both hands to wield |
| `BIT_27` | `ITEM_ANTI_CL` | clerics cannot use it |
| `BIT_28` | `ITEM_ANTI_TH` | thieves cannot use it |
| `BIT_29` | `ITEM_ANTI_MU` | magic users cannot use it |

This is the decimal-only encoding: one 32-bit integer carrying six flags from
three unrelated concerns (material properties, handedness, class gating).

### Values row: `0 3 8 3`

For `ITEM_WEAPON`, per `actwiz.c:1470`:

| Index | Value | Meaning |
|-------|-------|---------|
| 0 | 0 | proc value -- hook for a weapon special proc; 0 = none |
| 1 | 3 | number of damage dice |
| 2 | 8 | die size, so 3d8 |
| 3 | 3 | weapon class, **1-based**: the display does `sprinttype(value[3] - 1, weapons, ...)` and `weapons[2]` is `"Slash"` (`constant.c:298`) |

Values 4 through 7 are absent from the line and stay at whatever `GetNewObj()`
left; for a plain weapon nothing reads them.

### Economy row: `15 10000 0`

| Order | Value | Meaning |
|-------|-------|---------|
| 1 | 15 | weight; not a drink container, so no divide-by-4 |
| 2 | 10000 | cost |
| 3 | 0 | durability -- no breakage budget set |

There is no level and no timer field. The axe has no minimum level in the file
at all; RoL gates it purely through the three anti-class flags.

### Affect words

None. The next token after the economy fields is `E`, so `fscanf(" %u ")` fails
on the first affect word, both words stay clear, and `obj->sets_affs` is all
zeroes. The axe confers no `AFF_*` state on its wielder.

This is worth contrasting with the Luminari copy, which carries
`AFF_ELEMENT_PROT`.

### Extension blocks

**`E`** -- one extra description, keyword `axe frostbite`. Read first, before
any `A`, as the reader requires.

**`A` blocks** -- two applies, which is exactly `MAX_OBJ_AFFECT` for RoL:

| Block | Raw | location | modifier as written | modifier after load |
|-------|-----|----------|---------------------|---------------------|
| 1 | `18 3` | `APPLY_HITROLL` (18) | 3 | 3 |
| 2 | `19 3` | `APPLY_DAMROLL` (19) | 3 | 3 |

Neither location falls in `APPLY_STR`..`APPLY_CON` (1-5) or
`APPLY_AGI`..`APPLY_LUCK` (26-30), so the 4.5x stat rescale does **not** apply
here. Had this been `APPLY_STR 3`, the loaded value would have been
`(3 * 45) / 10` = 13.

Both blocks use the two-line form. The one-line form `A 18 3` would parse
identically -- the reader tokenizes with `fscanf(" %s ")` and does not care
about newlines.

There is no `T` block, so all six trap fields stay 0.

The record simply ends. Nothing marks the end; the reader has satisfied every
field it wants and returns, and the next `#96001` line is only ever reached
through the offset index.

## 4.3 What the pair shows

| Concern | RoL 96000 | Luminari 196000 |
|---------|-----------|-----------------|
| Damage | 3d8 stored directly in the object | 1d12, but authoritative dice come from `weapon_list[34]` |
| Weapon identity | value[3] = 3, a damage verb ("Slash") | value[0] = 34, a full weapon definition (great axe) |
| Attack verb | same field as weapon identity | separate: value[3] = 3 -> "slashes" |
| Enchantment | +3 hitroll and +3 damroll as two `A` blocks | +6 enhancement bonus in value[4] |
| Frost theming | flavour text only; proc value is 0 | a real `C` block: `WEAPON_SPECAB_FROST` at level 15, on-hit or command word `frostbite` |
| Wielder affect | none | `AFF_ELEMENT_PROT` |
| Two-handedness | explicit `ITEM_TWOHANDS` extra flag | implicit in `WEAPON_TYPE_GREAT_AXE` |
| Class gating | `ITEM_ANTI_CL`, `ITEM_ANTI_TH`, `ITEM_ANTI_MU` extra flags | none; gated by `ITEM_PROF_MASTER` in the `G` block instead |
| Level gate | none | 30 |
| Material | implied by the description text | `H 7` = `MATERIAL_STEEL` |
| Size | not modelled | `I 6` = `SIZE_LARGE` |
| Cost / weight | 10000 / 15 | 2000 / 12 |

The general shape: RoL puts the item's mechanics in the record. Luminari puts
an index in the record and the mechanics in a table, then adds structured
extensions (`C`, `G`, `H`, `I`) for the concerns RoL either encodes as flag bits
or does not model at all.

### Why this is not a converter output pair

Running 96000's extra word through `OBJECT_EXTRA_MAP`
(`scripts/world/wtool_lib/rol_transform.py:417`) gives source 0-based bits
{6, 13, 22, 26, 27, 28}, which map to target flags:

| Source | Target flag | Name |
|--------|-------------|------|
| 6 | 6 | `ITEM_MAGIC` |
| 13 | 38 | `ITEM_FLOAT` |
| 22 | 121 | `ITEM_ROL_TWO_HANDED` |
| 26 | 13 | `ITEM_ANTI_CLERIC` |
| 27 | 14 | `ITEM_ANTI_ROGUE` |
| 28 | 12 | `ITEM_ANTI_WIZARD` |

Expected target set: {6, 12, 13, 14, 38, 121}. The actual Luminari record
carries {6, 38, 44} -- it is missing all three anti-class flags and the
two-handed compatibility flag, and it adds `ITEM_AUTOPROC` (44), which has no
RoL source bit. The applies, dice, cost, weight, and level all differ too.

Note the compatibility flags `ITEM_ROL_TWO_HANDED` (121) and its neighbours
`ITEM_ROL_NO_IDENTIFY`, `ITEM_ROL_NO_SUMMON`, `ITEM_ROL_NO_SLEEP`,
`ITEM_ROL_NO_CHARM`, `ITEM_ROL_ANTI_GOOD_RACE`, `ITEM_ROL_ANTI_EVIL_RACE`,
`ITEM_ROL_WHOLE_BODY`, `ITEM_ROL_WHOLE_HEAD` (116-124) exist in
`src/structs.h` specifically so RoL extra flags with no native Luminari
equivalent survive conversion. Genuine converter output for this axe would
carry flag 121; this record does not.
