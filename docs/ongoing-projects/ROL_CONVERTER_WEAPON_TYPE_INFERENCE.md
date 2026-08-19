# RoL Converter: Weapon Type Inference (Item 3.3) -- Remaining Work

Status: the classifier is implemented, wired, and green. Everything below is
what is left. The design rationale and corpus analysis that produced it are not
repeated here; read the code, which carries them.

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

- **The classifier emits no ranged types.** `RANGED_WEAPON_TYPES` blocks them at
  both the override loader and `infer_weapon_type()`. This is a scoping rule,
  not a judgement about bows: real ranged weapons are source `ITEM_FIREWEAPON`
  and never reach this code. It affects only the two records RoL builders typed
  as melee `ITEM_WEAPON` while naming them bows (vnums `21` and `14110`), plus
  three ballistae. Revisit alongside section 2.
- **Handedness needs nothing further.** `ITEM_TWOHANDS` (source `BIT_23`) is
  already mapped to `ITEM_ROL_TWO_HANDED` (target extra flag 121) by
  `OBJECT_EXTRA_MAP`, and that flag is what `utils.c:11941` and
  `act.item.c:4130` read. `WEAPON_FLAG_TWO_HANDED` is commented out in
  `load_weapons()` and carries nothing.

---

## 2. Remaining: Ranged Weapons Do Not Work At All

**Severity: high. 99 records.** Not caused by 3.3 and not fixed by it; the
weapon classifier only handles source `ITEM_WEAPON`. Filed here because it is
the same `WEAPON_TYPE_UNDEFINED` defect on a different item type.

### What is broken

| Source | Count | Converts to | Result |
| :--- | :--- | :--- | :--- |
| `ITEM_FIREWEAPON` (6) | 51 | `ITEM_FIREWEAPON` (7) | Cannot fire |
| `ITEM_MISSILE` (7) | 48 | `ITEM_MISSILE` (14) | Ammo type is arbitrary |

**Bows and crossbows.** The target's `ITEM_FIREWEAPON` is marked
`/* deprecated */` (`src/structs.h:4501`) and its `value[0]` indexes a two-entry
`ranged_weapons[]` table, not `weapon_list[]`. But `is_using_ranged_weapon()`
(`src/combat/assign_wpn_armor.c:741`) checks
`weapon_list[value[0]].weaponFlags & WEAPON_FLAG_RANGED` without ever looking at
item type. All 51 records carry `value[0] == 0`, so they resolve
`WEAPON_TYPE_UNDEFINED`, fail the flag check, and `do_fire` / `can_fire_ammo`
decline every one of them.

**Missiles.** The item type is right, but `value[0]` passes through unmapped
into the target's `AMMO_TYPE_*` slot (0..4). 22 records happen to read as arrow
and 19 as bolt; 6 are out of range, one of them 100.

### What needs to be done

1. **Retarget the item type.** `OBJECT_TYPE_MAP[6]` currently yields 7
   (`ITEM_FIREWEAPON`). Emit 5 (`ITEM_WEAPON`) instead, so the record reaches the
   live archery path.
2. **Classify to a ranged `WEAPON_TYPE_*`.** Extend `rol_weapon_mapping.py` with
   a ranged rule set for `source_type == 6` -- longbow, shortbow, composite,
   heavy/light/hand crossbow -- and stop `RANGED_WEAPON_TYPES` from blocking that
   path. The corpus names are clean (`a slender elven longbow`, `a dark oak
   crossbow`), so keyword matching carries it.
3. **Map missile `value[0]` to `AMMO_TYPE_*`.** Targets are 1 arrow, 2 bolt,
   3 sling bullet, 4 dart. Nothing may emit 0 (`Undefined`) or exceed 4.
4. **Keep the pairs consistent.** `has_ammo_in_pouch()`
   (`src/combat/assign_wpn_armor.c:601`) rejects arrows in a crossbow and bolts
   in a bow. Whatever a converted bow becomes has to match the ammo the zone
   actually loads.
5. **Decide the thrown bucket.** The 51 also include javelins, darts, throwing
   knives, and a chatkcha. They are not bows; either give them melee types or a
   thrown-weapon disposition, but do not leave them on a bow.

Sequence matters: step 1 without step 2 leaves every bow on
`WEAPON_TYPE_UNDEFINED` as an `ITEM_WEAPON`, which is no better than today.

---

## 3. Remaining: Builder Validation

The 55 overrides and the 24 fallback records are the two buckets no rule
justified on its own. Both need a builder's eye before the converted corpus is
treated as final.

**3.1 The 55 overrides** (`rol_weapon_overrides.json`). Each entry carries a
`rationale` string; the file is the review surface. The bulk are improvised
objects (hoes, a frying pan, a banana, a stuffed parrot), body parts, siege
pieces, and a handful of real weapons whose names no rule knows (`schiavona`,
`unholy avenger`). Disagreements are one-line edits -- the loader validates the
constant name, rejects `WEAPON_TYPE_UNDEFINED` and ranged types, and the test
suite asserts every key names an active source weapon.

**3.2 The 24 fallback records.** These are builder placeholders, and the
fallback matrix reproduces exactly the verb and handedness they were built to
carry, which is the whole of their identity:

- `7073`-`7098`, the 21 standard quest weapon templates (1h/2h x
  slash/pierce/crush/pound x L1/M1/H1).
- `50403` and `33033`, the two noshow placeholders.
- `1294`, the standard god weapon item.

Decide whether these should convert at all. If they should, the current output
is correct by construction; if they should be excluded, that belongs in the
record-disposition layer, not here.

**3.3 Re-running the audit.** `rol_weapon_mapping.audit(corpus.records)` returns
per-tier and per-rule counts plus a row per record. Use it after any corpus or
rule change; `test_keyword_and_override_tiers_carry_the_corpus` will fail if the
fallback bucket grows past 30.

---

## 4. Remaining: Downstream Extension Blocks (feeds Gap 3.7)

`weapon_list[]` is now reachable from `emit_object()`, which unblocks three of
the extension blocks Item 3.7 tracks. None of these are written yet.

1. **`G` (proficiency).** Read `weapon_list[type].weaponFlags` and emit
   `ITEM_PROF_SIMPLE` / `ITEM_PROF_MARTIAL` / `ITEM_PROF_EXOTIC`. Every
   converted object currently gets `ITEM_PROF_NONE`.
2. **`H` (material).** Infer from description keywords (`steel`, `mithril`,
   `iron`, `wood`, `adamantine`, `bone`, `obsidian`, `silver`), falling back to
   `weapon_list[type].material`. Everything is `MATERIAL_UNDEFINED` today.
3. **`I` (size).** Derive from `weapon_list[type].size`. Without an `I` block the
   loader's size-0 rewrite makes every converted object `SIZE_MEDIUM`.

Doing this means reading `weapon_list[]` from Python. There is no bridge for it
yet; `rol_mob_calculator.py` is the precedent for going to the C side when a
table is the authority, and a static parse of the `setweapon()` calls in
`assign_wpn_armor.c` is the cheaper alternative that
`test_rol_weapon_mapping.py` already demonstrates twice.

---

## 5. Remaining: Open Question -- Enhancement Bonus (`value[4]`)

Not a free win, and not resolved by this work. RoL weapons carrying
`APPLY_HITROLL` (`A 18 <n>`) and `APPLY_DAMROLL` (`A 19 <n>`) could in principle
restate their `+N/+N` as Luminari's native enhancement bonus in `value[4]`. Two
blockers:

- **Double counting.** `compute_gear_enhancement_bonus()` reads `value[4]` while
  the `A` blocks apply to hitroll and damroll independently. Emitting both grants
  the bonus twice, so the applies would have to be dropped in the same change --
  a behavior change, not a lossless addition.
- **The target may discard it anyway.** `read_object()` zeroes `value[4]` when
  object cost is 100 or less (`src/db.c:5203`), and `set_weapon_object()` only
  runs under `ITEM_SET_STATS_AT_LOAD`, which the converter does not set.

This needs its own gap entry with its own corpus measurement. It is listed here
only so it is not mistaken for something 3.3 delivered.

---

## 6. Remaining: The 100d100 Sentinel Records

Eleven active corpus weapons carry damage dice of exactly 100d100: vnums 6, 13,
17, 21, 22, 24, 25, 28, 44, 45, and 47. All carry proc value 0. A shared,
physically impossible value across a low-vnum artifact block is a sentinel, not
a statistic -- these are driven by spec procs, not by their dice.

Their weapon types are settled: all eleven classify from their names (four
daggers, two quarterstaves, a longsword, a mace, a whip, a shuriken, and vnum 21
by override). What is not settled is the dice. The target clamps them to 2d12
like everything else, which is survivable but arbitrary. Decide as a group
whether these artifacts should instead be given deliberate dice, and whether
their real effect belongs in a `C` block (weapon special abilities) -- which is
the same destination Item 3.7 names for gap 2.3's proc-value data.
