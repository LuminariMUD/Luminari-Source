# RoL Converter: Armor Type Inference & Mapping (Item 3.2)

Status: proposal / brainstorming

Context: [ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md) Item 3.2
Format Authority: [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md)
Companion: [ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md) (Item 3.3)

All corpus figures in this document were measured against
`EXAMPLE/RealmsOfLuminari/areas/obj/` (216 files) and are reproducible.

---

## 1. Problem Statement

Realms of Luminari armor is flat and self-contained. `ITEM_ARMOR` is source type
9, and its four values are named by the source `stat` display at
`EXAMPLE/RealmsOfLuminari/src/actwiz.c:1495`:

| Slot | Source meaning |
|:--|:--|
| `value[0]` | AC-apply |
| `value[1]` | Warmth |
| `value[2]` | Prestige |
| `value[3]` | ProcVal |

There is no armor type, no material class, and no proficiency category. Warmth,
prestige, and procval have no target equivalent.

Luminari armor is driven by an index into `armor_list[]` stored in **`value[1]`**,
which supplies armor type (none/light/medium/heavy/shield), max dex bonus, armor
check penalty, arcane spell failure, and the 30ft/20ft movement rates. The table
is built by `load_armor()` in `src/combat/assign_wpn_armor.c`.

The converter passes source `value[1]` -- warmth -- straight through to target
`value[1]`. Armor type is therefore populated from an unrelated statistic. 2426
of 2527 records carry warmth 0 and land on `armor_list[0]`,
`SPEC_ARMOR_TYPE_UNDEFINED`; the remaining 101 scatter across 19 further values
up to 60, several of which exceed `NUM_SPEC_ARMOR_TYPES`.

### 1.1 Mechanical Consequences of `SPEC_ARMOR_TYPE_UNDEFINED`

`initialize_armor()` leaves index 0 with `armorType` 0, `dexBonus` 0,
`armorCheck` 0, `spellFail` 0, and `thirtyFoot`/`twentyFoot` 0.

1. **No armor proficiency category.** `compute_gear_armor_type()`
   (`assign_wpn_armor.c:1420`) returns `ARMOR_TYPE_NONE` for every converted
   piece, so class armor proficiency, `FEAT_ARMORED_MOBILITY`, and every
   light/medium/heavy gate see an unarmored character regardless of what they
   are wearing.
2. **No shields.** `compute_gear_shield_type()` requires
   `ARMOR_TYPE_SHIELD` or `ARMOR_TYPE_TOWER_SHIELD`. All 190 converted
   shield-slot records return `ARMOR_TYPE_NONE`, and the `SPEC_ARMOR_TYPE_BUCKLER`
   / `SPEC_ARMOR_TYPE_SMALL_SHIELD` check in `src/utils.c:8692` never matches.
3. **Max dex bonus of 0.** `armor_list[0].dexBonus` is 0, not 99. Any code path
   reading the cap off an undefined piece treats the wearer as having no dex
   bonus to AC at all -- strictly worse than the clothing entry, which is 99.
4. **No armor check penalty, no spell failure, no movement effect.** Converted
   plate is mechanically weightless. This is the one consequence that favors the
   player, and it is the reason a naive fix can make the corpus worse rather
   than better (section 6.1).

---

## 2. The Target List Is a Grid, Not a List

`SPEC_ARMOR_TYPE_*` in `src/structs.h:5206-5274` runs to `NUM_SPEC_ARMOR_TYPES`
**58**, not the 18 that the body-slot block alone suggests. The identifiers form
a family-by-slot grid:

| Family | BODY | HEAD | ARMS | LEGS |
|:--|--:|--:|--:|--:|
| `CLOTHING` | 1 | 18 | 32 | 45 |
| `PADDED` | 2 | 19 | 33 | 46 |
| `LEATHER` | 3 | 20 | 34 | 47 |
| `STUDDED_LEATHER` | 4 | 21 | 35 | 48 |
| `LIGHT_CHAIN` | 5 | 22 | 36 | 49 |
| `HIDE` | 6 | 23 | 37 | 50 |
| `SCALE` | 7 | 24 | 38 | 51 |
| `CHAINMAIL` | 8 | 25 | 39 | 52 |
| `PIECEMEAL` | 9 | 27 | 40 | 53 |
| `SPLINT` | 10 | 28 | 41 | 54 |
| `BANDED` | 11 | 29 | 42 | 55 |
| `HALF_PLATE` | 12 | 30 | 43 | 56 |
| `FULL_PLATE` | 13 | 31 | 44 | 57 |

Shields occupy `WEAR_SHIELD` only and have no per-slot row:
`BUCKLER` 14, `SMALL_SHIELD` 15, `LARGE_SHIELD` 16, `TOWER_SHIELD` 17.

Index **26** is `SPEC_ARMOR_TYPE_CHAIN_HEAD`, commented `/* duplicate :( */` in
the header. `load_armor()` populates both 25 and 26 with a chainmail helm. It is
a redundant entry, not a gap; emit 25 and leave 26 alone.

`load_armor()` populates all 58 indices, and each carries the matching
`ITEM_WEAR_*` in its `wear` field, so the grid is complete and self-consistent.

**This decomposes the problem into two independent axes:**

- **Slot axis** -- fully determined by the source wear mask. Deterministic, no
  inference, no judgment.
- **Family axis** -- 13 choices for worn armor, 4 for shields. This is the only
  place inference is needed.

It is a 13-way classification, not a 58-way one. That is materially smaller than
the 79-way weapon problem in Item 3.3.

---

## 3. Corpus Analysis

**2523 `ITEM_ARMOR` records** parsed from the corpus. (The 2527 quoted in Item
3.2 comes from the production parser; the four-record difference is malformed
records skipped by the measurement script and does not affect any proportion
below.)

### 3.1 Fewer Than Half the Records Are Even Eligible

`armor_list[]` covers `WEAR_BODY`, `WEAR_HEAD`, `WEAR_ARMS`, `WEAR_LEGS`, and
`WEAR_SHIELD`. Nothing else. Source wear distribution:

| Slot | Records | Has an `armor_list[]` row? |
|:--|--:|:--|
| BODY | 421 | yes |
| HEAD | 235 | yes |
| SHIELD | 190 | yes (shield block) |
| LEGS | 171 | yes |
| ARMS | 140 | yes |
| **eligible subtotal** | **1157** | |
| NECK | 246 | no |
| ABOUT | 182 | no |
| FEET | 180 | no |
| HANDS | 153 | no |
| WRIST | 143 | no |
| WAIST | 123 | no |
| FACE | 86 | no |
| FINGER | 86 | no |
| EYES | 69 | no |
| EARRING | 56 | no |
| (no wear bits) | 33 | no |
| TAIL | 9 | no |
| QUIVER | 3 | no |

Rows count records carrying that wear bit, so the eligible rows sum to exactly
1157 -- no record carries two eligible bits -- while the ineligible rows sum to
1369 against 1366 ineligible records. The difference is the three multi-slot
records: `6802` is ABOUT+SHIELD and `93222` is ARMS+WRIST, both eligible through
their second bit, and `32644` is FINGER+TAIL, ineligible and counted in two
rows. **Eligible 1157 + ineligible 1366 = 2523.**

This split is not an accident of the data. RoL's `apply_ac()`
(`EXAMPLE/RealmsOfLuminari/src/handler.c:1737`) grants armor class from
`value[0]` on sixteen slots, including neck, feet, hands, waist, about, wrists,
eyes, and face. Commit 14b1b711 correctly gated Luminari's `apply_ac()` to the
five slots that also carry armor check penalty, spell failure, and the max-dex
cap.

**Consequence:** 1366 records -- 54% of the corpus -- have no valid
`armor_list[]` index available, because no `SPEC_ARMOR_TYPE_LEATHER_FEET`
exists. 1158 of them carry non-zero AC in `value[0]` which the target now
ignores.

Those records must not be given an armor type. They should convert to
`ITEM_WORN` with their AC restated as an `APPLY_AC_NEW` affect -- exactly the
treatment 14b1b711 already applied to source apply 17. That is a separate change
with its own scope and risk; it is tracked here as **section 7** rather than
folded into 3.2, but it has to be decided first, because it determines whether
3.2's scope is 2523 records or 1157.

### 3.2 AC-Apply Distribution on the Eligible Records

Source `value[0]`, which is on RoL's descending-AC scale where a larger positive
number is better protection:

| Slot | n | min | p25 | median | p75 | max |
|:--|--:|--:|--:|--:|--:|--:|
| BODY | 421 | 0 | 6 | 10 | 16 | 100 |
| HEAD | 235 | -10 | 3 | 6 | 9 | 25 |
| ARMS | 140 | 0 | 5 | 6 | 8 | 15 |
| LEGS | 171 | -100 | 4 | 6 | 8 | 15 |
| SHIELD | 190 | 0 | 5 | 8 | 12 | 25 |

Twelve records across the whole armor corpus carry negative AC; six of them are
the `35601`-`35606` shackles at -100 with weight 100. These are curse/trap items
and need an explicit disposition, not a family (section 6.3).

### 3.3 Family Signal Availability

A three-tier keyword pass over the eligible 1157, using name and short
description first, then long description and `E` blocks, then bare material
words:

| Tier | Records | Share |
|:--|--:|--:|
| name / short description | 861 | 74% |
| long description / `E` blocks | 81 | 7% |
| material word only | 112 | 10% |
| **unresolved** | **103** | **9%** |

Of the 103 unresolved, **67 are builder placeholders** -- the
`9824`-`9919` "standard quest ... item --- string me" block and the
`1281`-`1296` "standard god wear_* item - string this" block. `semantics.py`
already carries unfinished-object placeholder detection, so those route to an
existing mechanism rather than to curation.

**The genuinely ambiguous residual is about 36 records.** Examples:
`a suit of barbed rings`, `a mystical cranial defender`, `a coif of living
spiders`, `a wreath of ivy`, `a troll sized condom`, `a helm of mists`,
`a glowing mesh of magical energies`.

Indicative family distribution from that pass (a feasibility estimate, not a
committed classifier -- the material tier in particular is crude and will move):

```
CLOTHING 239   CHAINMAIL 169   SMALL_SHIELD 132   LEATHER 126
HIDE 114       FULL_PLATE  87  STUDDED_LEATHER 42  SCALE 37
PADDED 35      HALF_PLATE  25  LIGHT_CHAIN 15      BUCKLER 13
TOWER_SHIELD 5 BANDED 5        LARGE_SHIELD 5      PIECEMEAL 3   SPLINT 2
```

---

## 4. Architecture Proposal

```
+-------------------------------------------------------------------+
|          RoL ITEM_ARMOR record (2523 in the corpus)               |
+-------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------+
| Gate 0: Slot eligibility (deterministic, from the wear mask)      |
|   BODY/HEAD/ARMS/LEGS/SHIELD -> continue          (1157)          |
|   anything else              -> ITEM_WORN + APPLY_AC_NEW, done    |
|                                                    (1366, sec 7)  |
+-------------------------------------------------------------------+
                                 | eligible
                                 v
+-------------------------------------------------------------------+
| Gate 1: Disposition overrides (curse items, placeholders)         |
|   negative AC, "string me"/"string this" stubs -> explicit rule   |
+-------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------+
| Tier 1: Curated override catalog (rol_armor_overrides.json)       |
|   keyed by source VNUM; ~36 genuinely ambiguous records           |
|   produced by offline AI curation, reviewed, committed            |
+-------------------------------------------------------------------+
                                 | (miss)
                                 v
+-------------------------------------------------------------------+
| Tier 2: Family keyword matcher, specific -> general                |
|   name + short desc, then long desc + E blocks     (~81%)         |
+-------------------------------------------------------------------+
                                 | (miss)
                                 v
+-------------------------------------------------------------------+
| Tier 3: Material-word fallback                                    |
|   metal -> chainmail, hide/skin -> hide, cloth -> clothing (~10%) |
+-------------------------------------------------------------------+
                                 | (miss)
                                 v
+-------------------------------------------------------------------+
| Tier 4: Conservative default -- CLOTHING for worn,                |
|         SMALL_SHIELD for shield slot (section 6.1)                |
+-------------------------------------------------------------------+
                                 |
                                 v
+-------------------------------------------------------------------+
| Compose: armor_index = GRID[family][slot]                         |
| Audit gate: index in 1..57, != 0, != 26; wear bit of the chosen   |
|             index equals the emitted wear bit                     |
+-------------------------------------------------------------------+
```

The composition step is what makes this tractable: every tier decides only a
**family**, and the slot is applied last from a lookup table. No tier ever names
a 58-value constant directly, so no tier can produce a slot/index mismatch.

---

## 5. Tier Specifications

### 5.1 Tier 2: Family Keywords, Evaluated Specific to General

Order matters. `full plate` must be tested before `plate`, `chain shirt` before
`chain`, `tower shield` before `shield`, `studded` before `leather`.

**Shield slot (`WEAR_SHIELD`), 4-way:**

- `tower shield`, `wall shield` -> `TOWER_SHIELD`
- `large shield`, `heavy shield`, `kite shield`, `great shield` -> `LARGE_SHIELD`
- `buckler`, `targe` -> `BUCKLER`
- `shield`, `aegis`, `pavise`, anything else -> `SMALL_SHIELD`

**Worn slots, 13-way:**

- `full plate`, `plate mail`, `platemail`, `field plate`, `plate armor`,
  `suit of plate` -> `FULL_PLATE`
- `half plate`, `breastplate`, `cuirass` -> `HALF_PLATE`
- `banded` -> `BANDED`
- `splint` -> `SPLINT`
- `piecemeal`, `patchwork`, `mismatched` -> `PIECEMEAL`
- `elven chain`, `light chain`, `mithril chain`, `chain shirt`, `fine mesh`
  -> `LIGHT_CHAIN`
- `chainmail`, `chain mail`, `ringmail`, `ring mail`, `hauberk`, `maille`,
  `chain `, `mail ` -> `CHAINMAIL`
- `scale`, `lamellar`, `dragonscale` -> `SCALE`
- `studded` -> `STUDDED_LEATHER`
- `leather`, `suede` -> `LEATHER`
- `hide`, `pelt`, `fur`, `skin`, `shell`, `carapace`, `chitin`, `bone`, `tusk`
  -> `HIDE`
- `padded`, `quilted`, `gambeson`, `aketon`, `felt`, `knit`, `wool`, `velvet`
  -> `PADDED`
- `robe`, `cloak`, `tunic`, `shirt`, `silk`, `cloth`, `vest`, `gown`, `dress`,
  `linen`, `sash`, `toga`, `headband`, `tiara`, `circlet`, `crown`, `bandana`,
  `hood`, `cowl`, `mitre`, `hat`, `cap`, `veil`, `pants`, `skirt` -> `CLOTHING`

### 5.2 Tier 3: Material Words

Only reached when no family word appears anywhere in the record's text.

| Material words | Family | Reasoning |
|:--|:--|:--|
| `iron`, `steel`, `alloy`, `bronze`, `brass`, `metal`, `adamantine`, `mithril`, `cyanite` | `CHAINMAIL` | a generic metal piece is a mail piece by default; deliberately not plate |
| `oaken`, `ironwood`, `wooden`, `wood`, `vine`, `root`, `reed`, `straw` | `PADDED` | non-metal rigid or woven, closest light non-leather family |
| `gold`, `silver`, `jewel`, `gem`, `crystal`, `pearl`, `ivory`, `feather` | `CLOTHING` | decorative pieces, not protective construction |

`CHAINMAIL` rather than `FULL_PLATE` for unqualified metal is a deliberate
conservatism choice; see 6.1.

### 5.3 Tier 1: Curated Override Catalog

`scripts/world/wtool_lib/rol_armor_overrides.json`, keyed by source VNUM, same
shape as the weapon catalog proposed in Item 3.3. Scope is roughly 36 records
after placeholders are routed elsewhere. This is where offline AI curation earns
its place -- reading the full record including long description and extra
descriptions to decide that `a coif of living spiders` is `HIDE` and
`a mystical cranial defender` is `FULL_PLATE`. It is a one-time pass whose
output is reviewed, committed, and deterministic forever after; there is no
runtime model dependency and no network call in the conversion path.

---

## 6. Open Questions That Should Be Answered Before Coding

### 6.1 Does the family choice have to justify the AC? (the important one)

`set_armor_object()` -- which overwrites `value[0]` with
`armor_list[type].armorBonus`, plus cost, weight, material, and the wear bits --
runs only under `ITEM_SET_STATS_AT_LOAD` (`src/db.c:5199`). The converter does
not set that flag, so it never fires. **Source AC in `value[0]` survives
independently of the family assigned to `value[1]`.**

That is a real degree of freedom: the family can be chosen for flavor without
disturbing the item's protective value. But the wearer still pays the family's
armor check penalty, arcane spell failure, max-dex cap, and movement rate.
Assigning `FULL_PLATE` costs -6 armor check, 35% spell failure, and a dex cap of
7 -- on an item that, today, costs nothing.

Every converted piece therefore gets strictly worse for its wearer than it is
now, and the size of that regression is entirely a function of how aggressively
the classifier reaches for heavy families. Hence the conservative defaults in
5.2 and 5.3. Whether that is the right posture, or whether the corpus should be
converted at "true" flavor weight and rebalanced afterwards, is a design call
that is not the converter's to make.

**Do not infer family from AC magnitude.** Source AC medians are BODY 10 /
HEAD 6 / ARMS 6 / LEGS 6 / SHIELD 8 on a descending scale, while Luminari's
`armorBonus` is tenth-scale (full plate is 60, meaning +6 AC). The scales do not
correspond, and unlike the ability-score question settled in Item 3.4 there is
no clean 1:1 story. Inferring family from AC would import RoL's power inflation
directly into the D20 penalty model.

### 6.2 Warmth, prestige, and procval

Source `value[1]`, `value[2]`, and `value[3]` all become dead once `value[1]` is
reassigned. Warmth and prestige have no target equivalent and should be dropped
with a diagnostic. Procval is the same class of data as the weapon procval in
Item 2.3 and shares its natural destination, the `C` block (Item 3.7).

### 6.3 Curse items and negative AC

Twelve records carry negative `value[0]`, six of them the -100 shackles. A
family assignment is meaningless for these. They need an explicit disposition --
most likely `ITEM_WORN` plus a negative `APPLY_AC_NEW`, matching the section 7
treatment -- decided as a group.

### 6.4 The 33 records with no wear bits

`ITEM_ARMOR` with an empty wear mask has no slot, therefore no grid row. They
cannot be armor in the target. Route them with the section 7 records or classify
them as source corruption.

---

## 7. Prerequisite: The 1366 Ineligible Records (proposed Item 3.2b)

Stated separately because it is a different change with a different risk
profile, and because 3.2's scope depends on its outcome.

Source `ITEM_ARMOR` on NECK, ABOUT, FEET, HANDS, WRIST, WAIST, FACE, FINGER,
EYES, EARRING, TAIL, or QUIVER has no target armor semantics at all. Today those
1366 records emit as `ITEM_ARMOR` with a meaningless `value[1]` and a `value[0]`
that `apply_ac()` discards, so 1158 records silently lose all their protective
value.

Proposal: for source type 9 on an ineligible slot, emit target `ITEM_WORN` and
restate `value[0]` as an `APPLY_AC_NEW` affect, reusing
`_convert_armor_apply_modifier()`'s scale handling. Note that source `value[0]`
on the item and source apply 17 have **opposite sign conventions** -- apply 17 is
descending (negative is protection, hence the inversion in
`_convert_armor_apply_modifier`), while `apply_ac()` returns `value[0]` and the
caller negates it at `handler.c:814`, so a positive `value[0]` is protection.
The existing helper cannot be called directly on `value[0]` without accounting
for that; getting this backwards would convert every one of those 1158 records
into a penalty.

---

## 8. Implementation Plan

1. **Decide section 6.1** -- the conservatism posture -- and section 7. Neither
   is a coding question.
2. **Create `scripts/world/wtool_lib/rol_armor_mapping.py`**: the family
   classifier and the family-by-slot grid, exposing
   `infer_armor_type(record) -> (int, str)` returning the composed index and a
   diagnostic string.
3. **Add `scripts/world/wtool_lib/rol_armor_overrides.json`**: the curated
   catalog for the ~36 ambiguous records.
4. **Wire into `rol_transform.py`**: for `source_type == 9`, set `values[1]`
   from the classifier. Classify from the `RolRecord` -- the wear mask lives in
   `record.values["flags"][2]`, and the emitted wear bits are the mapped form --
   so that the slot decision reads the same source data the emitted wear bits
   are derived from. Emit a diagnostic naming the tier that decided.
5. **Add the audit gate**: every eligible record produces an index in 1..57 that
   is not 0 and not 26, and `armor_list[index].wear` matches the emitted wear
   bit.
6. **Tests in `scripts/world/tests/test_rol_transform.py`**: one case per tier,
   one per shield family, one slot/family composition case per row of the grid,
   the negative-AC disposition, and the empty-wear-mask disposition.
7. **Update documentation**: mark Item 3.2 resolved in
   `ROL_CONVERTER_OBJECT_FILE_REFERENCES.md`, and open 3.2b if section 7 is
   accepted.

---

## 9. Relationship to Item 3.3

The two share a shape -- override catalog, keyword tiers, mechanical fallback,
audit gate -- but differ in three ways worth stating, because the weapon
document's framing does not transfer:

1. **Armor decomposes; weapons do not.** Slot and family are independent here,
   which is why a 58-value target list is a 13-way decision. Weapon type has no
   such factorization.
2. **Armor conversion makes items worse; weapon conversion makes them better.**
   Assigning a weapon type restores crits, damage types, and feat eligibility
   that `WEAPON_TYPE_UNDEFINED` denies. Assigning an armor type adds check
   penalties, spell failure, and a dex cap that `SPEC_ARMOR_TYPE_UNDEFINED`
   waives. The correct default posture is the opposite in each case.
3. **Armor has no dice-clamp equivalent.** The weapon case is urgent partly
   because `MAX_WEAPON_NDICE 2` flattens 30% of weapon damage, leaving
   `weapon_list[]` as the only differentiator. Armor `value[0]` is not clamped
   and survives intact, so `armor_list[]` is additive rather than load-bearing.
