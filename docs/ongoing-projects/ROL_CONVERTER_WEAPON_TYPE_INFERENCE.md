# RoL Converter: Weapon Type Inference (Item 3.3) -- Remaining Work

Status: sections 2, 4, and 5 are implemented, wired, and green. The melee
classifier shipped first (commit `689b6e64`); the ranged chain, the
`set_weapon_object()` parity pass, and the enhancement-bonus restatement
followed. What remains open is section 3, the builder review packet, which is
deliberately deferred to the end of the conversion effort.

The plans below are kept as written, with an implementation note under each
that records where the shipped code differs from the plan and why. The design
rationale and corpus analysis are not repeated here; read the code, which
carries them.

## 0. What Shipped

| Artifact | Role |
| :--- | :--- |
| `scripts/world/wtool_lib/rol_weapon_table.py` | Static parse of `load_weapons()` into a Python `weapon_list[]`; the source of dice, cost, weight, material, size, and proficiency for every converted weapon |
| `scripts/world/wtool_lib/rol_weapon_mapping.py` | Melee classifier plus the ranged and ammunition classifiers, the ammo-pairing emission guard, and the one-report `audit()` |
| `scripts/world/wtool_lib/rol_weapon_overrides.json` | 55 melee, 1 ranged, and 1 ammunition curated overrides |
| `scripts/world/wtool_lib/rol_transform.py` | Per-record target item type, the value-slot rules, `set_weapon_object()` parity, `G`/`H`/`I` emission, the enhancement bonus, and the corrected `EQUIPMENT_POSITION_MAP` |
| `src/mob/mob_act.c`, `src/combat/fight.c` | `MOB_ROL_ARCHER` auto-reload, out of combat and in it |
| `scripts/world/tests/test_rol_weapon_mapping.py` | 30 tests, including full-corpus coverage, header-drift guards on both sides, slot guards, and the launcher/pouch/ammo kit simulation |

Measured over the active corpus after the change: 1,319 melee weapons, 51
launchers, and 48 ammunition records all resolve; 4 of the ammunition records
are retyped to `ITEM_WEAPON` because they are thrown; 44 quivers split 24
archery pouches to 20 throwing containers; no `WEAPON_TYPE_UNDEFINED`, no
`AMMO_TYPE_UNDEFINED`, and no emitted ranged type that `has_ammo_in_pouch()`
lacks a case for. Every emitted record re-parses through `parse_object_file()`.

**Not carried to the dev world yet.** All of this changes converter output, and
the world already applied to development came out of the previous Phase 7/8
release. Nothing converted before this change carries the ranged chain, the
weapon-table parity, the enhancement bonus, or the corrected equipment
positions; a fresh Phase 7/8 release is what delivers them.

Context: [ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md) Item 3.3
Format authority: [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md)

---

## 1. What Is Already Done

| Artifact | Role |
| :--- | :--- |
| `scripts/world/wtool_lib/rol_weapon_mapping.py` | Three-tier classifier, `WEAPON_TYPE_*` table, ranged-type guard, `audit()` reporter |
| `scripts/world/wtool_lib/rol_weapon_overrides.json` | 55 curated per-vnum overrides |
| `scripts/world/wtool_lib/rol_transform.py` | `emit_object()` writes the inferred type into `values[0]` for `source_type == 5` |
| `scripts/world/tests/test_rol_weapon_mapping.py` | 13 tests, including full-corpus coverage and header-drift guards |

Measured over the 1,319 active source weapons: 55 override, 1,240 keyword, 24
fallback; 0 undefined; 43 distinct target weapon types; no ranged type emitted.
All 1,319 re-parse through `parse_object_file()` with `values[0]` in 1..79.

Two decisions worth not relitigating:

- **The melee classifier emits no ranged types.** `RANGED_WEAPON_TYPES` blocks
  them at both the override loader and `infer_weapon_type()`. This stays true
  for `source_type == 5`, but section 2 turns it from a global veto into a
  per-source-type gate so `ITEM_FIREWEAPON` records can reach the ranged types.
  Within source type 5 it affects only the two records RoL builders typed as
  melee while naming them bows (vnums `21` and `14110`), plus three ballistae;
  those stay melee.
- **Handedness needs nothing further.** `ITEM_TWOHANDS` (source `BIT_23`) is
  already mapped to `ITEM_ROL_TWO_HANDED` (target extra flag 121) by
  `OBJECT_EXTRA_MAP`, and that flag is what `utils.c:11941` and
  `act.item.c:4130` read. `WEAPON_FLAG_TWO_HANDED` is commented out in
  `load_weapons()` and carries nothing.

---

## 2. Ranged Weapons: Verified End-to-End Plan -- IMPLEMENTED

**Severity: high. 99 object records, 42 archer mobiles, 1,844 zone resets.**
Not caused by 3.3 and not fixed by it; the melee classifier only handles source
`ITEM_WEAPON`. The decision is to convert every ranged record correctly, and
that means the whole firing chain, not just the weapon-type slot.

Everything below was checked against the live target code and the RoL source
tree in `EXAMPLE/RealmsOfLuminari/`, which carries the original `missile.c`
engine. Line references are from that check.

### 2.1 What the target requires before anything fires

Each link is a hard gate; one broken link produces silence, not a warning.

1. **A wielded `ITEM_WEAPON`** whose `weapon_list[value[0]].weaponFlags` has
   `WEAPON_FLAG_RANGED` -- `is_using_ranged_weapon()`
   (`src/combat/assign_wpn_armor.c:718`) never looks at item type, which is why
   the target's own `ITEM_FIREWEAPON` (7) is dead: it is marked
   `/* deprecated */` at `src/structs.h:4348` and indexes a two-entry
   `ranged_weapons[]` table instead.
2. **An ammo pouch worn at `WEAR_AMMO_POUCH` (23)** -- `has_ammo_in_pouch()`
   (`src/combat/assign_wpn_armor.c:566`) reads `GET_EQ(ch, WEAR_AMMO_POUCH)`
   and rejects an empty one.
3. **Its first content is an `ITEM_MISSILE`** -- the check is on
   `ammo_pouch->contains`, the head of the list, not a search.
4. **The ammo type pairs with the weapon type** -- arrow to the bow family,
   bolt to the crossbow family, `AMMO_TYPE_STONE` to `WEAPON_TYPE_SLING`,
   `AMMO_TYPE_DART` to `WEAPON_TYPE_DART`. There is no case for any other
   ranged type.
5. **Crossbows and slings must be loaded** -- `weapon_is_loaded()`
   (`assign_wpn_armor.c:549`) requires `value[5] > 0`, decremented per shot at
   `src/combat/fight.c:14278`.
6. **NPC archers need `MOB_ROL_ARCHER` (108)** -- `src/mob/mob_act.c:424` fires
   into an adjacent room when `can_fire_ammo()` passes.

Also true, and it shapes the mapping: the target has **no throwing command**.
`WEAPON_FLAG_THROWN` grants no verb; `fire`, `reload`, and `collect` are the
only ranged commands in `cmd_info[]`. And `do_hit`
(`src/combat/act.offensive.c:4667`) refuses to start melee for a player
wielding any `WEAPON_FLAG_RANGED` weapon -- so a ranged type with no ammo
pairing is an item that can neither fire nor fight.

### 2.2 The source declares its own types -- use them, not the names

The earlier plan proposed keyword-matching the short descriptions. That is the
wrong tier. `EXAMPLE/RealmsOfLuminari/src/missile.c:51-83` documents the value
layout, and `rangeweapons[]` / `missiles[]` (`missile.c:347` and `:402`) are a
shared 21-entry, one-based numbering used by both item types:

| # | `rangeweapons[]` (fireweapon `value[7]`) | `missiles[]` (missile `value[3]`) |
|---|---|---|
| 1-4 | Short Bow, Long Bow, Elven Short Bow, Elven Long Bow | the four matching arrow grades |
| 5-7 | Hand, Light, Heavy Crossbow | the three matching quarrels |
| 8 | Special Missile Weapon | Special Flight |
| 9-12 | Throwing Dagger, Dart, Throwing Hammer, Throwing Handaxe | same, as thrown ammo |
| 13-16 | Sling, Javelin, Spear, Blowgun | Sling Stone, Javelin, Spear, Blowgun Dart |
| 17-21 | Special Thrown, Common Object, Ballista, Catapult, Spell Missile | same |

Every one of the 99 corpus records carries a valid type in that range. Measured:

| Source | Declared types present |
| :--- | :--- |
| `ITEM_FIREWEAPON` (51) | Long Bow 21, Elven Long Bow 6, Light Crossbow 4, Hand Crossbow 1, Heavy Crossbow 1, Sling 1, Throwing Dagger 5, Throwing Handaxe 4, Javelin 3, Dart 2, Throwing Hammer 2, Special Missile 1 |
| `ITEM_MISSILE` (48) | arrows (1-4) 32, quarrels (5-7) 8, Special Flight 4, Throwing Dagger 2, Throwing Hammer 1, Dart 1 |

So the classifier tiers invert relative to melee: **tier 1 is the declared
type**, tier 2 is the name (only for types 8, 17 and 18, the "special" and
"common object" buckets), tier 3 is the per-vnum override file. Names still
matter as a cross-check -- roughly a dozen records disagree with their declared
type (`a blackwood shortbow` declared Long Bow, `a throwing hammer` declared
Javelin, `a large chunk of granite` declared Heavy Crossbow Quarrel, `an old,
moss-covered rock` declared Sling). The rule is that the declared type wins,
because that is what the RoL engine used for range, rate of fire, and quiver
compatibility; every disagreement is logged into the section 3 review packet.

### 2.3 Weapon type mapping

Source `ITEM_FIREWEAPON` becomes target `ITEM_WEAPON` (5), classified from
`value[7]`:

| Source range type | Target | Note |
| :--- | :--- | :--- |
| 1 Short Bow | `WEAPON_TYPE_SHORT_BOW` (44) | |
| 2 Long Bow | `WEAPON_TYPE_LONG_BOW` (43) | |
| 3 Elven Short Bow | `WEAPON_TYPE_COMPOSITE_SHORTBOW` (46) | elven is RoL's better tier (range 100 vs 65); the `_2`..`_5` variants encode a strength rating no source record carries |
| 4 Elven Long Bow | `WEAPON_TYPE_COMPOSITE_LONGBOW` (45) | as above (range 200 vs 120) |
| 5 Hand Crossbow | `WEAPON_TYPE_HAND_CROSSBOW` (60) | |
| 6 Light Crossbow | `WEAPON_TYPE_LIGHT_CROSSBOW` (13) | |
| 7 Heavy Crossbow | `WEAPON_TYPE_HEAVY_CROSSBOW` (12) | |
| 13 Sling | `WEAPON_TYPE_SLING` (16) | ranged, pairs with `AMMO_TYPE_STONE` |
| 9 Throwing Dagger | `WEAPON_TYPE_DAGGER` (2) | thrown, not ranged |
| 10 Dart | `WEAPON_TYPE_DART` (14) | ranged; the target pairs it with `AMMO_TYPE_DART` and the corpus has one |
| 11 Throwing Hammer | `WEAPON_TYPE_LIGHT_HAMMER` (18) | thrown, not ranged |
| 12 Throwing Handaxe | `WEAPON_TYPE_THROWING_AXE` (17) | thrown, not ranged |
| 14 Javelin | `WEAPON_TYPE_SHORTSPEAR` (8) | **not** `WEAPON_TYPE_JAVELIN`: that type is `WEAPON_FLAG_RANGED` with no case in `has_ammo_in_pouch()`, so it can never fire, and `do_hit` would refuse to melee with it. `SHORTSPEAR` is the same 1d6 piercing thrown profile without the ranged flag |
| 15 Spear | `WEAPON_TYPE_SPEAR` (11) | no corpus records |
| 16 Blowgun | `WEAPON_TYPE_DART` (14) | no corpus records |
| 8, 17, 18 special/common | name tier, then override | 1 record: the joke rocket launcher (vnum 1017) |
| 19, 20 Ballista/Catapult | override | no corpus records here; the three source-type-5 ballistae already have melee overrides |

**Emission guard.** A `WEAPON_TYPE_*` carrying `WEAPON_FLAG_RANGED` may only be
emitted when `has_ammo_in_pouch()` has a case for it -- bows, the three
crossbows, `SLING`, `DART`. This replaces the current blanket
`RANGED_WEAPON_TYPES` veto, which stays in force for `source_type == 5`.

### 2.4 Ammunition mapping

Source `ITEM_MISSILE` stays target `ITEM_MISSILE` (14), classified from
`value[3]`:

| Source missile type | Target `value[0]` |
| :--- | :--- |
| 1-4 arrows | `AMMO_TYPE_ARROW` (1) |
| 5-7 quarrels | `AMMO_TYPE_BOLT` (2) |
| 13 Sling Stone | `AMMO_TYPE_STONE` (3) |
| 10 Dart, 16 Blowgun Dart | `AMMO_TYPE_DART` (4) |
| 9, 11, 12, 14, 15 thrown | not ammo in the target: retype the record to `ITEM_WEAPON` with the melee type from the table above (3 records) |
| 8, 17, 18 special | name tier, then override (4 records: two `a magical arrow` become arrows, `a hiltless throwing dagger` becomes a dagger, the joke rocket is an override) |

Nothing may emit 0 (`AMMO_TYPE_UNDEFINED`) or exceed 4 -- `has_ammo_in_pouch()`
rejects both. Retyping a record from `ITEM_MISSILE` to `ITEM_WEAPON` needs a
per-vnum item-type override, which `emit_object()` does not have today: the
target type comes only from `OBJECT_TYPE_MAP[source_type]`.

### 2.5 Value slots -- three live defects, not just unset fields

Passing source values through is not merely lossy here; three target slots mean
something entirely different from the source slot sitting in them.

**`ITEM_FIREWEAPON` -> `ITEM_WEAPON`:**

| Source | Target slot | Rule |
| :--- | :--- | :--- |
| `value[0]` unused | weapon type | inferred, from `value[7]` |
| `value[1]`, `value[2]` dice | num dice, dice size | from `weapon_list[]` per section 4 |
| `value[3]` hit message | attack message | map through `SOURCE_WEAPON_MESSAGE_MAP`; only a fallback at runtime, since `fight.c:11922` prefers `weapon_list[].damageTypes` |
| `value[4]` firing delay | enhancement bonus | drop the source value; slot is written by section 5 |
| `value[5]` max rate of fire | **loaded-ammo counter** | **must be zeroed.** `weapon_is_loaded()` reads this slot; a source ROF of 3-5 silently pre-loads a converted crossbow |
| `value[6]` bowstring durability | -- | no target equivalent, drop |
| `value[7]` range weapon type | -- | classifier input; not emitted |

**`ITEM_MISSILE` -> `ITEM_MISSILE`:**

| Source | Target slot | Rule |
| :--- | :--- | :--- |
| `value[0]` num dice | ammo type | replaced by the section 2.4 classification |
| `value[1]` dice size | **imbued spell number** | **must be zeroed.** `imbued_arrow()` (`fight.c:12057`) casts `GET_OBJ_VAL(missile, 1)` through `call_magic()` on every shot. Today `a flight arrow` carries 8 = `SPELL_CHILL_TOUCH`, `a smooth oaken bolt` carries 6 = `SPELL_CALL_LIGHTNING`, and `a silver arrow` carries 2 = `SPELL_TELEPORT` |
| `value[2]` durability 1-10, 10 best | **break probability percent** | **inverted meaning.** `fight.c:12210` breaks the missile when `value[2] >= dice(1, 100)`, so the best source ammo becomes the most fragile. Emit `11 - durability` |
| `value[3]` missile type | unused | classifier input; emit 0 |
| -- | `value[4]` | enhancement bonus, section 5 |

Ammo carries no damage dice in the target at all: ranged damage comes from
`weapon_list[]` on the launcher plus enhancement bonuses, including the pouch
ammo's own (`fight.c:7265`). Dropping the source ammo dice loses nothing the
target can represent.

### 2.6 The delivery chain: quivers and zone resets

Converting the objects is not sufficient -- the ammo has to reach the pouch
slot. Three findings here, one of them a blocker that also affects
non-ranged gear.

**2.6.1 Quivers already map, and split in two.** RoL `ITEM_QUIVER` (30) maps to
`ITEM_AMMO_POUCH` (36) in `OBJECT_TYPE_MAP`, and source wear bit
`ITEM_WEAR_QUIVER` maps to `ITEM_WEAR_AMMO_POUCH` (16). But RoL quivers carry a
kind in `value[3]`: 24 of the 44 active quivers are archery quivers (1) and 20
are throwing quivers (2). Zone resets fill them accordingly -- 1,085 `P` resets
put missiles into archery quivers, 314 put thrown `ITEM_FIREWEAPON` records
into throwing quivers.

A throwing quiver must therefore become an `ITEM_CONTAINER` (15), not an
`ITEM_AMMO_POUCH`: its contents convert to `ITEM_WEAPON`, and both
`has_ammo_in_pouch()` and `do_put` (`src/obj/act.item.c:2022`) require an ammo
pouch to hold `ITEM_MISSILE` only. Its value layout (capacity, container flags,
key vnum) already matches the target container layout; give it the same key-vnum
identity resolution source type 15 gets, which today it does not (all corpus
keys are 0 or -1, so nothing is broken yet).

**2.6.2 Zone equipment positions are shifted by one -- blocker.**
`EQUIPMENT_POSITION_MAP` (`rol_transform.py:556`) was built against RoL's
`equipment_types[]` display table (`EXAMPLE/RealmsOfLuminari/src/constant.c:1394`),
which omits
`SECONDARY_WEAPON`. The zone `E` command's position argument is a `WEAR_*`
constant (`EXAMPLE/RealmsOfLuminari/src/structs.h:1136-1146`), resolved at reset
through `restore_wear[ZCMD.arg3]`
(`EXAMPLE/RealmsOfLuminari/src/files.c:547`). The two lists diverge from 17 up, so
every mapped position above 16 is off by one:

| Source position | Means | Maps to today | Should be |
| ---: | :--- | :--- | :--- |
| 17 | `SECONDARY_WEAPON` | 17 `WEAR_HOLD_1` | 18 `WEAR_WIELD_OFFHAND` |
| 18 | `HOLD` | 26 `WEAR_EYES` | 17 `WEAR_HOLD_1` |
| 19 | `WEAR_EYES` | 22 `WEAR_FACE` | 26 `WEAR_EYES` |
| 20 | `WEAR_FACE` | 24 `WEAR_EAR_R` | 22 `WEAR_FACE` |
| 21 | `EARRING_R` | 25 `WEAR_EAR_L` | 24 `WEAR_EAR_R` |
| 22 | `EARRING_L` | 23 `WEAR_AMMO_POUCH` | 25 `WEAR_EAR_L` |
| 23 | `WEAR_QUIVER` | 27 `WEAR_BADGE` | 23 `WEAR_AMMO_POUCH` |
| 24 | `GUILD_INSIGNIA` | rejected, reset dropped | 27 `WEAR_BADGE` |
| 25 | `WEAR_TAIL` | rejected, reset dropped | no target slot; drop with a diagnostic |

The corpus confirms the reading: `areas/zon/gen-obj.zon:257` is
`E 1 7964 999 23` with the trailing comment `* quiver`, position 22 appears
five times commented `*earing`, position 18 carries `Brass Lamp`, and position
19 carries `a pair of crystal spectacles`. Blast radius, counted over the active
corpus -- 1,247 `E` resets sit at source position 17 or above:

| Source position | Resets | Lands on |
| ---: | ---: | :--- |
| 17 offhand weapon | 454 | held slot |
| 18 held | 523 | eyes |
| 19 eyes | 36 | face |
| 20 face | 60 | right ear |
| 21 right ear | 56 | left ear |
| 22 left ear | 5 | ammo pouch |
| 23 **quiver** | **103** | **badge** |
| 24 insignia, 25 tail | 10 | dropped entirely |

The target's `E` handler (`src/db.c:5967`) validates only the slot bounds, never
the wear flags, so all of this loads silently.

No amount of weapon-type work fires a single arrow until this is fixed.

**2.6.3 The NPC side is already wired.** Source `ACT_ARCHER` (17) already maps
to `MOB_ROL_ARCHER` (108) at `rol_transform.py:209`, and `mob_act.c:424`
implements the behavior. Of the 42 archer mobiles in the corpus, 26 are given a
launcher by their zone -- 22 long bow, 3 elven long bow, 1 heavy crossbow -- and
25 of those 26 also get a quiver with ammunition put into it. Three carry only
thrown weapons, and 13 get no ranged item at all. Every launcher-and-ammo pair a
zone actually builds is bow-with-arrow, so nothing in the corpus depends on a
pairing the target rejects.

### 2.7 Known-inert outcomes, accepted

- **Three archer mobiles wield only thrown weapons** (two throwing hammers, one
  dart) and fed themselves from throwing quivers. The target cannot throw, so
  their `MOB_ROL_ARCHER` flag becomes inert -- `can_fire_ammo()` is false and
  the branch is skipped. They melee instead. No error, no crash.
- **One heavy-crossbow archer mobile** (vnum 50707) will fire until `value[5]`
  reaches zero and never reload: `do_reload` is a player command and the mob AI
  has no reload step. Either accept one mob firing once per load, or add the
  three-line auto-reload to the `MOB_ROL_ARCHER` branch. Recommend the latter;
  it is target-side code written for this conversion already. The branch also
  skips charmed followers (`!ch->master`), so a converted archer taken as a pet
  never reaches this path at all.
- **Players cannot melee with a converted bow** -- `do_hit` refuses, by target
  design, and points them at `fire`.

### 2.8 Implementation order

1. Fix `EQUIPMENT_POSITION_MAP` (2.6.2). Independent of everything else and
   fixes non-ranged gear too.
2. Split quivers by `value[3]` (2.6.1) and give them container key resolution.
3. Retarget `OBJECT_TYPE_MAP[6]` to 5, and add the per-vnum item-type override
   hook the four thrown `ITEM_MISSILE` records need. Note the same map also
   translates shop buy-types (`rol_transform.py:937`), so a shop that bought
   fireweapons will list `ITEM_WEAPON` afterwards; the emitter already dedupes
   repeated buy-types, so no shop gains a duplicate entry.
4. Extend `rol_weapon_mapping.py` with the declared-type tier for source types
   6 and 7, the name tier for the "special" buckets, the ranged emission guard,
   and the ammo classifier. Steps 3 and 4 must land together: step 3 alone
   leaves every bow an `ITEM_WEAPON` on `WEAPON_TYPE_UNDEFINED`.
5. Apply the value-slot rules (2.5), including the three zeroing/inversion
   fixes.
6. Let section 4 (`set_weapon_object()` parity) and section 5 (enhancement
   bonus) cover these records too -- they are `ITEM_WEAPON` and `ITEM_MISSILE`
   after step 3, so both apply without special-casing.
7. Extend `audit()` to cover the ranged tiers and emit the review packet.

### 2.9 Tests

Alongside the melee equivalents already in `test_rol_weapon_mapping.py`:

- full-corpus coverage: all 51 + 48 records classify, no `WEAPON_TYPE_UNDEFINED`,
  no `AMMO_TYPE_UNDEFINED`, no ammo value outside 1..4;
- header-drift guards for the ranged `WEAPON_TYPE_*`, `AMMO_TYPE_*`,
  `WEAR_AMMO_POUCH`, and the `WEAR_*` position constants on both sides;
- the emission guard: no emitted ranged type lacks a `has_ammo_in_pouch()` case;
- slot guards: emitted missile `value[1]` is always 0, `value[2]` is `11 -
  durability`, emitted weapon `value[5]` is 0;
- a pairing simulation that walks every zone's `M`/`E`/`P` chain and asserts
  each converted launcher-plus-pouch-plus-ammo kit would pass
  `has_ammo_in_pouch()`;
- an equipment-position round trip asserting source 23 emits 23;
- re-parse: every emitted record survives `parse_object_file()`.

### 2.10 Implementation notes

What shipped follows 2.1 through 2.9 with four deliberate differences.

**Tier order puts curated overrides first.** 2.2 ordered the tiers declared,
name, override. The shipped classifier is override, declared, name, melee
fallback. The point 2.2 was making -- the declared type beats the record's own
name -- holds unchanged; an override is a builder-review decision rather than an
inference, and letting it lose to a declared type would make the file unable to
correct the two records it exists for. Both joke records are curated there: the
`SKM-77` launcher (source 1017) becomes `WEAPON_TYPE_HEAVY_CROSSBOW` and its
`SMIR-42` rocket (source 1019) becomes `AMMO_TYPE_BOLT`, so the pair still fires
as its builders intended.

**The override catalog grew two sections rather than one.** `ranged_overrides`
is validated against the ammo-pairing guard, and `ammunition_overrides` accepts
either an `AMMO_TYPE_*`, which keeps the record ammunition, or a
`WEAPON_TYPE_*`, which retypes it. That last form is the per-vnum item-type
override 2.4 called for; the melee `overrides` section keeps its blanket
ranged veto.

**The special buckets fall through to the melee classifier.** 2.3 and 2.4 named
the name tier and the override file for source types 8, 17, and 18 but left no
default for a record that matches neither. Rather than invent one, an
unmatched special record runs through the melee classifier, which always
resolves and never emits a ranged type. In the corpus this catches exactly one
record, `a hiltless throwing dagger` (source 21005), which becomes a dagger --
the disposition 2.4 predicted for it.

**The archer auto-reload landed in two places.** 2.7 recommended adding it to
the `MOB_ROL_ARCHER` branch in `mob_act.c`, which covers the out-of-combat
opening shot. The in-combat auto-reload at `fight.c:15291` was player-only
(`!IS_NPC(ch) && PRF_AUTORELOAD`), so a mob that opened fire would still empty
its crossbow and fall silent for the rest of the fight. That gate now reads
`MOB_ROL_ARCHER` for NPCs; no other NPC behavior changes.

---

## 3. Builder Validation Happens At The End

Human review is deferred to the end of the conversion effort, not gated per
item. Nothing here blocks implementation; it accumulates into one review packet.

**3.1 The 55 melee overrides** (`rol_weapon_overrides.json`). Each entry carries
a `rationale` string; the file is the review surface. The bulk are improvised
objects (hoes, a frying pan, a banana, a stuffed parrot), body parts, siege
pieces, and a handful of real weapons whose names no rule knows (`schiavona`,
`unholy avenger`). Disagreements are one-line edits -- the loader validates the
constant name and the test suite asserts every key names an active source
weapon. Ranged overrides added by section 2 join the same file.

**3.2 The 24 fallback records.** Builder placeholders whose converted output is
correct by construction, since the fallback matrix reproduces exactly the verb
and handedness they were built to carry:

- `7073`-`7098`, the 21 standard quest weapon templates (1h/2h x
  slash/pierce/crush/pound x L1/M1/H1).
- `50403` and `33033`, the two noshow placeholders.
- `1294`, the standard god weapon item.

Whether they should convert at all is a record-disposition question, not a
classifier one.

**3.3 The ranged name/type disagreements.** Section 2 resolves these
mechanically -- the declared type wins -- but each one is a builder judgement
the review should see. The known set: `a blackwood shortbow` and `a slender
short bow` declared Long Bow; `a throwing hammer` declared Javelin; `a stone`
declared Throwing Hammer; `an old, moss-covered rock` declared Sling, which
makes it a launcher; `a large chunk of granite` declared Heavy Crossbow
Quarrel; `a hiltless throwing dagger` declared Special Flight; and the two
`SKM-77` joke rockets. Also on this list: the javelin-to-`SHORTSPEAR` and
dart-to-`WEAPON_TYPE_DART` calls in 2.3, and the 19 archer mobiles whose thrown
weapons leave them meleeing (2.7).

**3.4 Re-running the audit.** `rol_weapon_mapping.audit(corpus.records)` returns
per-tier and per-rule counts plus a row per record, and now covers all three
source families in one report -- melee weapons, launchers, and ammunition -- with
separate `weapons`, `ranged_weapons`, `ammunition`, and `retyped_ammunition`
counts. Its `name_disagreements` list is the mechanical form of 3.3: every
record whose text argues with the type its builder declared, thirteen of them in
the current corpus. Use it after any corpus or rule change;
`test_keyword_and_override_tiers_carry_the_corpus` will fail if the fallback
bucket grows past 30.

---

## 4. Replicate `set_weapon_object()` (decided: full parity) -- IMPLEMENTED

**Decision: a converted weapon must come out mechanically identical to one an
immortal builds in OLC by picking a weapon type.** This supersedes the earlier
"emit the G/H/I extension blocks" framing, which was a subset of it.

OLC does this in one call: `oedit.c:2644` hands `ITEM_WEAPON` value-1 input
straight to `set_weapon_object()` (`src/obj/treasure.c:2562`) and skips to
value 5, because everything between is derived. The converter replicates that
function's effects at emit time:

| `set_weapon_object()` does | Converter emits |
| :--- | :--- |
| `GET_OBJ_TYPE = ITEM_WEAPON` | item type 5 |
| `value[0] = type` | already done (Item 3.3) |
| `value[1] = weapon_list[type].numDice` | dice from the table |
| `value[2] = weapon_list[type].diceSize` | dice from the table |
| `GET_OBJ_COST = weapon_list[type].cost + 1` | economy cost field |
| `GET_OBJ_WEIGHT = weapon_list[type].weight` | weight field |
| `GET_OBJ_MATERIAL = weapon_list[type].material` | `H` block |
| `GET_OBJ_SIZE = weapon_list[type].size` | `I` block |
| clears all wear bits, then sets `TAKE` and `WIELD` | wear flags |

Two things `set_weapon_object()` does not do, which the converter still should:

- **`G` (proficiency).** Derive from `weapon_list[type].weaponFlags` --
  `ITEM_PROF_SIMPLE` / `ITEM_PROF_MARTIAL` / `ITEM_PROF_EXOTIC`. Every converted
  object currently gets `ITEM_PROF_NONE`. Without an `I` block the loader's
  size-0 rewrite would make everything `SIZE_MEDIUM`, so `I` is required even
  when the table value happens to be 5.
- **Handedness.** Already handled: source `BIT_23` (`ITEM_TWOHANDS`) maps to
  target extra flag 121 (`ITEM_ROL_TWO_HANDED`) via `OBJECT_EXTRA_MAP`, which is
  what `utils.c:11941` and `act.item.c:4130` read.

**Consequences accepted with this decision:**

- Source damage dice are discarded for weapons. That is what makes the 100d100
  sentinel records (section 6) a non-issue.
- Source cost and weight are discarded for weapons. Shop pricing on converted
  weapons becomes table-derived. Flagged in the section 3 review packet, not
  treated as a defect.
- The wear-flag reset means a converted weapon carries exactly `TAKE|WIELD`.
  Any source weapon that also wore in another slot loses that; report those
  records in the audit.

**Getting `weapon_list[]` into Python.** There is no bridge yet. Statically
parse the `setweapon()` calls in `src/combat/assign_wpn_armor.c:970+` -- the
argument order is fixed by the function signature at line 907 (`type, name,
numDice, diceSize, critRange, critMult, weaponFlags, cost, damageTypes, weight,
range, weaponFamily, size, material, handle_type, head_type, description`) and
`test_rol_weapon_mapping.py` already demonstrates this parse twice.
`rol_mob_calculator.py` is the precedent for treating the C side as the
authority. Add a drift test that fails when the parse stops matching the
`WEAPON_TYPE_*` count in `structs.h`.

### 4.1 Implementation notes

`rol_weapon_table.py` parses the `setweapon()` calls and every integer
`#define` in `src/structs.h` they reference, and
`test_weapon_table_covers_every_declared_weapon_type` fails if the parse stops
covering the `WEAPON_TYPE_*` block -- the drift guard this section asked for.
`_apply_weapon_object()` in `rol_transform.py` then replicates the function,
including the wear-word reset, and reports each discarded source value as a
diagnostic so the review packet can see it.

One deviation. This section named `ITEM_PROF_SIMPLE` / `ITEM_PROF_MARTIAL` /
`ITEM_PROF_EXOTIC` for the `G` block; the target has no such constants. Its
ladder is `ITEM_PROF_MINIMAL` / `BASIC` / `ADVANCED` / `MASTER` / `EXOTIC`
(`src/structs.h:4460`), so the D20 tiers land on `MINIMAL`, `BASIC`, and
`EXOTIC`. This is descriptive rather than restrictive today: `invalid_prof()`
is commented out at every call site (`src/handler.c:2490`,
`src/obj/objsave.c:801`) and no class grants the `SKILL_PROF_*` chain, so a
converted martial or exotic weapon is not gated. If that system is ever
revived, these values become live restrictions and belong in the review packet.

---

## 5. Enhancement Bonus (decided: average the applies, then drop them) -- IMPLEMENTED

**Decision.** RoL has no enhancement-bonus concept; it expresses `+N` weapons as
`APPLY_HITROLL` (`A 18 <n>`) and `APPLY_DAMROLL` (`A 19 <n>`) affects. The
converter restates that as Luminari's native `value[4]`:

```
enhancement = (hitroll_total + damroll_total) / 2      # integer division
```

then **removes** the source `APPLY_HITROLL` and `APPLY_DAMROLL` entries from the
emitted `A` blocks. Removing them is what makes this safe: `fight.c:7135` and
`fight.c:10500` read `GET_ENHANCEMENT_BONUS()` into damage and to-hit already,
so emitting both would grant the bonus twice.

Rules:

- Sum all `APPLY_HITROLL` modifiers and all `APPLY_DAMROLL` modifiers separately,
  then average the two totals. A record carrying only one of the two averages
  against zero (a `+2 hitroll only` weapon becomes `+1`), which is the intended
  reading of a half-stated bonus.
- Clamp to `0..10`, matching what OLC accepts for `ITEM_WEAPON` value 5
  (`oedit.c:2978`). Negative averages clamp to 0 and the source applies are still
  dropped -- a cursed `-3/-3` weapon becomes a plain weapon, and the loss is
  reported.
- Applies to `ITEM_WEAPON`, `ITEM_MISSILE`, and (post-section-2) converted
  ranged weapons alike; `GET_ENHANCEMENT_BONUS` (`utils.h:1596`) reads `value[4]`
  for all of them, and `fight.c:7265` reads it off pouch ammo. For ammunition
  this is a strict gain rather than a restatement: an `A` block on an arrow
  sitting in a pouch applies nothing, because object affects only take effect
  on equipped gear, while `value[4]` is read straight off the pouch contents.
  RoL's own documentation tells builders to express arrow bonuses exactly this
  way (`missile.c:80`: assign the first `A` slot `APPLY_HITROLL`, +1 to +5).
- Every clamp and every dropped apply is an audit diagnostic.

**The `read_object()` cost-100 concern is void.** The `value[4] = 0` rewrite at
`src/db.c:5182` lives inside the `if (OBJ_FLAGGED(obj, ITEM_SET_STATS_AT_LOAD))`
block opened at line 5167. The converter does not set that flag, so converted
`value[4]` survives load intact regardless of cost.

### Measured corpus impact

| Source type | Records | Carry hitroll and/or damroll | Both | Hit only | Dam only |
| :--- | ---: | ---: | ---: | ---: | ---: |
| `ITEM_WEAPON` (5) | 1,319 | 537 | 363 | 105 | 69 |
| `ITEM_FIREWEAPON` (6) | 51 | 21 | 16 | 5 | 0 |
| `ITEM_MISSILE` (7) | 48 | 27 | 21 | 5 | 1 |

Resulting enhancement values for the 537 melee weapons, before clamping:
17 negative, 47 zero, 241 at +1, 98 at +2, 73 at +3, 19 at +4, 9 at +5, 10 at
+6, and 23 above +6 -- including two at +55 and nine at +100, which are the
same low-vnum artifact block as section 6 and clamp to 10.

### 5.1 Implementation notes

Implemented exactly as decided, in `_object_enhancement_bonus()`, and applied
to `ITEM_WEAPON` and `ITEM_MISSILE` alike -- which by then includes every
converted launcher and every retyped thrown record. The source `A` blocks for
`APPLY_HITROLL` and `APPLY_DAMROLL` are dropped from the emitted record, and
both the restatement and every clamp are diagnostics.

---

## 6. The 100d100 Sentinel Records (resolved by section 4)

Eleven active corpus weapons carry damage dice of exactly 100d100: vnums 6, 13,
17, 21, 22, 24, 25, 28, 44, 45, and 47, all with proc value 0. A shared,
physically impossible value across a low-vnum artifact block is a sentinel, not
a statistic -- these are driven by spec procs, not by their dice.

Their weapon types were already settled by the classifier (four daggers, two
quarterstaves, a longsword, a mace, a whip, a shuriken, and vnum 21 by
override). Under the section 4 decision their dice come from `weapon_list[]`
like every other converted weapon, so there is nothing left to decide about the
dice themselves. What remains is only whether their real effect should be
restated in a `C` block (weapon special abilities) -- the same destination Item
3.7 names for gap 2.3's proc-value data. Their `+100/+100` applies clamp to an
enhancement bonus of 10 under section 5.
