# RoL Converter: Weapon Type Inference & Mapping (Item 3.3)

Status: proposal / brainstorming. Corpus claims re-measured against
`EXAMPLE/RealmsOfLuminari/areas/obj/` and the target loader; corrections
applied in place and called out in section 8.

Context: [ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md) Item 3.3
Format Authority: [OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md)

---

## 1. Problem Statement

In Realms of Luminari (RoL), weapon definitions are self-contained and flat:
- `value[0]`: Proc-value hook (0 in 1,305 of the 1,319 corpus weapons; non-zero only for specific hardcoded procs).
- `value[1]`: Number of damage dice.
- `value[2]`: Damage die size.
- `value[3]`: weapon verb/class, a zero-based index 0..11 consumed by the
  switch at `EXAMPLE/RealmsOfLuminari/src/combat.c:854`: 0-2 whip, 3 slash,
  4-6 crush, 7 bludgeon, 8-9 claw, 10 bite, 11 pierce. Verb 0 occurs in the
  corpus and is absent from the converter's `SOURCE_WEAPON_MESSAGE_MAP`.
- Extra Flag `BIT_23` (`1 << 22`): `ITEM_TWOHANDS`.

In Luminari (Pathfinder 3.5e D20), weapons are driven by an index into `weapon_list[]` stored in `value[0]` (`WEAPON_TYPE_*` 1..79, defined in `src/structs.h` and initialized in `src/combat/assign_wpn_armor.c:load_weapons()`).

The converter currently passes `record.values["values"][0]` straight through. As a result, converted weapons land on `weapon_list[0]`, which is `WEAPON_TYPE_UNDEFINED` ("unused weapon").

### 1.1 Mechanical Consequences of `WEAPON_TYPE_UNDEFINED`

1. **Critical Hits Disabled**: `critRange` is 0 and `critMult` is 1. Converted weapons can never land a critical hit.
2. **Damage Reduction (DR) Failure**: `damageTypes` bitmask is 0 (neither `DAMAGE_TYPE_SLASHING`, `DAMAGE_TYPE_PIERCING`, nor `DAMAGE_TYPE_BLUDGEONING` is set). DR/slashing, DR/piercing, or DR/bludgeoning bypass checks fail.
3. **Feats and Weapon Mastery Ineffective**: Feats like `FEAT_WEAPON_FOCUS`, `FEAT_WEAPON_SPECIALIZATION`, `FEAT_IMPROVED_CRITICAL`, and `FEAT_GREATER_WEAPON_FOCUS` key off `weapon_list[type].weaponFamily` (e.g. `WEAPON_FAMILY_LARGE_BLADE`, `WEAPON_FAMILY_AXE`). Undefined weapons match no family.
4. **Proficiency & Handedness Gaps**: The engine cannot resolve weapon proficiency groups (`WEAPON_FLAG_SIMPLE`, `WEAPON_FLAG_MARTIAL`, `WEAPON_FLAG_EXOTIC`) or size/handedness properties (`WEAPON_FLAG_LIGHT`, `WEAPON_FLAG_BALANCED`, `WEAPON_FLAG_DOUBLE`).

### 1.2 The Target Clamps Damage Dice, So Weapon Type Carries the Difference

`src/olc/oasis.h` sets `MAX_WEAPON_NDICE 2` and `MAX_WEAPON_SDICE 12`, and
`read_object()` enforces both on every `ITEM_WEAPON` at load
(`src/db.c:5162`). Measured over the corpus:

- `ndice > 2`, truncated to 2: **393 records (30%)**
- `sdice > 12`, truncated to 12: 23 records (2%)
- either: **396 records (30%)**

So the earlier reading -- that damage dice survive intact and the weapon type is
a cosmetic profile bolted on afterwards -- is wrong. Thirty percent of converted
weapons arrive with their damage flattened to the same 2d12 ceiling, and a
source 8d20 and a source 2d12 become indistinguishable. Once dice are capped
uniformly, `weapon_list[]` -- crit range, crit multiplier, damage types, family,
proficiency -- is where essentially all surviving weapon differentiation lives.
That is the argument for doing this work, and it is stronger than the one the
first draft made.

A corollary for section 4: any rule that branches on average damage is
branching on a pre-clamp number the target discards. Use damage as a weak
tiebreaker only, and prefer name and handedness.

---

## 2. RoL Corpus Weapon Analysis

An audit of all 1,319 `ITEM_WEAPON` records in the active RoL corpus reveals the following distribution of signals:

### 2.1 Attack Verb (`value[3]`) Distribution
- **Slash (3)**: 495 records (37.5%)
- **Pierce (11)**: 410 records (31.1%)
- **Bludgeon (7)**: 190 records (14.4%)
- **Crush (4, 5, 6)**: 167 records (12.7%)
- **Whip (1, 2)**: 55 records (4.2%)
- **Claw / Bite / Other (0, 8, 9, 10)**: 2 records (<0.2%)

### 2.2 Handedness (`ITEM_TWOHANDS`)
- **Two-Handed (`BIT_23`, that is `1 << 22`)**: 237 records (18.1%)
- **One-Handed / Unspecified**: 1,071 records (81.9%)

### 2.3 Semantic Keyword Availability
- **Standard Weapon Keywords**: Over 88% (~1,160) of weapon records contain exact or canonical weapon names in their aliases or short descriptions (e.g. `longsword`, `dagger`, `warhammer`, `greataxe`, `quarterstaff`, `morningstar`, `rapier`, `scimitar`, `halberd`).
- **Compound & Ambiguous Blades**: ~6% of records use generic terms (`sword`, `blade`, `axe`, `mace`) that require handedness or damage/weight disambiguation.
- **Exotic / Improvised / Natural Objects**: ~10% (about 128 records) are non-standard objects (e.g. monster body parts like `claws of kazgoroth`, `giant femur`, `beetle mandible`; tools like `rolling pin`, `heavy iron wrench`, `rusty hoe`; or siege pieces like `huge ballista`).

---

## 3. Architecture Proposal: Multi-Tier Hybrid Inference

To achieve 100% classification coverage without runtime overhead, a four-tier architecture is proposed:

```
+-------------------------------------------------------------------+
|               RoL Source Record (1,319 Weapons)                   |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| Tier 1: Explicit Override Catalog (rol_weapon_overrides.json)     |
|         - Handles ~79 exotic, improvised, and monster items       |
|         - Curated via offline AI analysis + builder validation    |
+-------------------------------------------------------------------+
                                  | (if not in overrides)
                                  v
+-------------------------------------------------------------------+
| Tier 2: Deterministic Semantic NLP / Keyword Matcher              |
|         - Matches compound names, specific blades, polearms, etc. |
|         - Gated by handedness (ITEM_TWOHANDS) & weight/dice       |
|         - Covers ~88-94% of the corpus                            |
+-------------------------------------------------------------------+
                                  | (if no keyword match)
                                  v
+-------------------------------------------------------------------+
| Tier 3: Mechanical Fallback Matrix                                |
|         - Maps (value[3] verb, 2H flag, weight, damage dice)      |
|         - Fallback safety net guarantees no unmapped records      |
+-------------------------------------------------------------------+
                                  |
                                  v
+-------------------------------------------------------------------+
| Tier 4: Invariant & Diagnostic Audit Gate                         |
|         - Asserts target weapon_type != WEAPON_TYPE_UNDEFINED (0) |
|         - Emits diagnostic trail for conversion reporting         |
+-------------------------------------------------------------------+
```

---

## 4. Detailed Tier Specifications

### 4.1 Tier 1: Curated Override Catalog (`rol_weapon_overrides.json`)

Stored as a version-controlled JSON dictionary keyed by source VNUM. Handles edge cases where text is evocative but non-standard:

| VNUM | Keywords / Description | RoL Stats | Target Weapon Type | Rationale |
| :--- | :--- | :--- | :--- | :--- |
| `508` | `giant femur` | 2d6 Crush, 2H | `WEAPON_TYPE_GREAT_CLUB` | Large improvised bludgeon |
| `6` | `The Disk of Night, chakram` | see note, Slash, 1H | `WEAPON_TYPE_SHURIKEN` | Exotic thrown/slashing disk |
| `7979` | `garroting wire` | 1d4 Whip | `WEAPON_TYPE_WHIP` | Flexible strangling weapon |
| `16015` | `rolling pin` | 1d3 Bludgeon | `WEAPON_TYPE_CLUB` | Improvised light club |
| `20224` | `Claws of Kazgoroth` | 4d10 Pierce | `WEAPON_TYPE_UNARMED` | Natural claw/fist weapon |
| `40927` | `old wooden sign` | 1d2 Crush | `WEAPON_TYPE_CLUB` | Improvised bludgeon |
| `46015` | `wet towel` | 1d3 Whip | `WEAPON_TYPE_WHIP` | Improvised whip |
| `50405` | `heavy iron wrench` | 2d8 Crush, 2H | `WEAPON_TYPE_GREAT_CLUB` | Heavy two-handed tool |
| `59203` | `uprooted tree` | 5d4 Crush, 2H | `WEAPON_TYPE_GREAT_CLUB` | Massive two-handed bludgeon |
| `81022` | `huge ballista` | 20d20 Pierce | `WEAPON_TYPE_HEAVY_CROSSBOW` | Large-scale missile weapon |

**Note on the 100d100 records.** Twelve corpus weapons carry damage dice of
exactly 100d100: vnums 6, 13, 17, 21, 22, 24, 25, 28, 44, 45, 47, and 46803
(46803 and 17 are the same dagger, `Stealthwhisper`, duplicated). All twelve
carry proc value 0. A shared, physically impossible value across a low-vnum
artifact block is a sentinel, not a statistic -- these are almost certainly
driven by spec procs rather than by their dice. Do not read 100d100 as evidence
of weapon character; classify these twelve from their names and dispose of the
dice as a group decision, and note that the target clamps them to 2d12
regardless.

### 4.2 Tier 2: Semantic NLP / Keyword Rule Engine

Evaluated in strict order of specificity (specific compound names before general nouns):

1. **Exotic & Specialized Blades**:
   - `bastard sword`, `hand and a half` -> `WEAPON_TYPE_BASTARD_SWORD`
   - `two bladed sword`, `2 bladed sword`, `double bladed sword` -> `WEAPON_TYPE_2_BLADED_SWORD`
   - `greatsword`, `great sword`, `claymore`, `zweihander`, `flamberge` -> `WEAPON_TYPE_GREAT_SWORD`
   - `short sword`, `shortsword`, `gladius`, `wakizashi`, `drusus` -> `WEAPON_TYPE_SHORT_SWORD`
   - `rapier`, `foil`, `epee`, `estoc` -> `WEAPON_TYPE_RAPIER`
   - `scimitar`, `cutlass`, `saber`, `tulwar`, `shamshir` -> `WEAPON_TYPE_SCIMITAR` (or `WEAPON_TYPE_FALCHION` if 2H)
   - `falchion` -> `WEAPON_TYPE_FALCHION`
   - `khopesh` -> `WEAPON_TYPE_KHOPESH`
   - `kukri` -> `WEAPON_TYPE_KUKRI`
   - `athame` -> `WEAPON_TYPE_ATHAME`
   - `dagger`, `dirk`, `stiletto`, `poniard`, `main gauche`, `bodkin`, `tanto` -> `WEAPON_TYPE_DAGGER`
   - `knife`, `knives`, `scalpel`, `razor`, `shiv` -> `WEAPON_TYPE_KNIFE`
   - `machete`, `cleaver` -> `WEAPON_TYPE_SHORT_SWORD` if Slash else `WEAPON_TYPE_KNIFE`
   - Generic `sword`, `blade`, `katana`, `longsword`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_GREAT_SWORD`
     - Else -> `WEAPON_TYPE_LONG_SWORD`

2. **Axes & Polearms**:
   - `double axe` -> `WEAPON_TYPE_DOUBLE_AXE`
   - `dwarven war axe`, `dwarven axe` -> `WEAPON_TYPE_DWARVEN_WAR_AXE`
   - `hand axe`, `handaxe`, `hatchet`, `tomahawk` -> `WEAPON_TYPE_HAND_AXE`
   - `throwing axe` -> `WEAPON_TYPE_THROWING_AXE`
   - `halberd`, `poleaxe`, `polearm`, `bill`, `bardiche` -> `WEAPON_TYPE_HALBERD`
   - `glaive`, `naginata` -> `WEAPON_TYPE_GLAIVE`
   - `guisarme` -> `WEAPON_TYPE_GUISARME`
   - `ranseur`, `partisan` -> `WEAPON_TYPE_RANSEUR`
   - `lance` -> `WEAPON_TYPE_LANCE`
   - `scythe` -> `WEAPON_TYPE_SCYTHE`
   - `sickle` -> `WEAPON_TYPE_SICKLE`
   - `kama` -> `WEAPON_TYPE_KAMA`
   - `spear`, `longspear`, `shortspear`, `pike`, `trident`, `javelin`, `pitchfork`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_LONGSPEAR`
     - Else if `trident` in text -> `WEAPON_TYPE_TRIDENT`
     - Else if `javelin` in text -> `WEAPON_TYPE_JAVELIN`
     - Else -> `WEAPON_TYPE_SPEAR`
   - Generic `axe`, `battleaxe`, `battle axe`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_GREAT_AXE`
     - Else -> `WEAPON_TYPE_BATTLE_AXE`

3. **Bludgeoning, Flails, Hammers & Staves**:
   - `quarterstaff`, `staff`, `stave`, `bo staff`, `crozier` -> `WEAPON_TYPE_QUARTERSTAFF`
   - `morningstar`, `morning star` -> `WEAPON_TYPE_MORNINGSTAR`
   - `dire flail` -> `WEAPON_TYPE_DIRE_FLAIL`
   - `flail`, `flindbar`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_HEAVY_FLAIL`
     - Else -> `WEAPON_TYPE_FLAIL`
   - `warmaul`, `maul`, `sledgehammer`, `sledge` -> `WEAPON_TYPE_WARMAUL`
   - `warhammer`, `hammer`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_WARMAUL`
     - Else -> `WEAPON_TYPE_WARHAMMER`
   - `light hammer` -> `WEAPON_TYPE_LIGHT_HAMMER`
   - `mace`, `scepter`:
     - If `ITEM_TWOHANDS` or `weight >= 6` or `avg_damage >= 4.5` -> `WEAPON_TYPE_HEAVY_MACE`
     - Else -> `WEAPON_TYPE_LIGHT_MACE`
   - `club`, `cudgel`, `baton`, `nightstick`, `sap`, `truncheon`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_GREAT_CLUB`
     - Else -> `WEAPON_TYPE_CLUB`

4. **Exotic & Chains**:
   - `spiked chain`, `chain` -> `WEAPON_TYPE_SPIKED_CHAIN`
   - `nunchaku`, `nunchuck` -> `WEAPON_TYPE_NUNCHAKU`
   - `sai` -> `WEAPON_TYPE_SAI`
   - `pick`, `pickaxe`:
     - If `ITEM_TWOHANDS` -> `WEAPON_TYPE_HEAVY_PICK`
     - Else -> `WEAPON_TYPE_LIGHT_PICK`
   - `whip`, `bullwhip`, `scourge`, `lash` -> `WEAPON_TYPE_WHIP`

5. **Natural / Unarmed**:
   - `claw`, `bite`, `fist`, `gauntlet`, `knuckle`, `cestus`, `tentacle`, `mandible`, `horn`, `stinger`, `fang`, `tail` -> `WEAPON_TYPE_UNARMED`

### 4.3 Tier 3: Mechanical Verb & Handedness Fallback Matrix

If no keyword matches, the fallback matrix guarantees deterministic assignment.

**Read the verb from the source record, not from the emitted values.**
`_object_values()` in `rol_transform.py` rewrites `values[3]` in place for
`source_type == 5`, remapping it through `SOURCE_WEAPON_MESSAGE_MAP` to the
target damage-message id. After that call, crush 4/5/6 have all become 6,
bludgeon 7 has become 5, bite 10 has become 4, and claw 8/9 have become 8.
Slash (3) and pierce (11) are identity-mapped, which is exactly what would let
this mistake pass a spot check while mis-classifying the 353 crush, bludgeon,
and bite records. See section 7 step 3.

The weight and damage columns below are tiebreakers of last resort. Source
weights in this corpus are frequently non-physical, and per section 1.2 the
target flattens 30% of damage dice at load, so neither column carries the
signal it appears to.

| Source Verb (`value[3]`) | Handedness | Weight / Damage Profile | Inferred Target `WEAPON_TYPE_*` |
| :--- | :--- | :--- | :--- |
| **Slash (3)** | 2-Handed | Any | `WEAPON_TYPE_GREAT_SWORD` |
| **Slash (3)** | 1-Handed | Light (<= 2 lbs or <= 3.5 avg dam) | `WEAPON_TYPE_DAGGER` |
| **Slash (3)** | 1-Handed | Medium / Heavy | `WEAPON_TYPE_LONG_SWORD` |
| **Pierce (11)** | 2-Handed | Any | `WEAPON_TYPE_LONGSPEAR` |
| **Pierce (11)** | 1-Handed | Light (<= 2 lbs) | `WEAPON_TYPE_DAGGER` |
| **Pierce (11)** | 1-Handed | Medium (<= 4 lbs) | `WEAPON_TYPE_SHORT_SWORD` |
| **Pierce (11)** | 1-Handed | Heavy (> 4 lbs) | `WEAPON_TYPE_SPEAR` |
| **Crush (4-6)** | 2-Handed | Any | `WEAPON_TYPE_GREAT_CLUB` |
| **Crush (4-6)** | 1-Handed | Heavy (>= 6 lbs) | `WEAPON_TYPE_HEAVY_MACE` |
| **Crush (4-6)** | 1-Handed | Light (< 6 lbs) | `WEAPON_TYPE_LIGHT_MACE` |
| **Bludgeon (7)** | 2-Handed | Any | `WEAPON_TYPE_GREAT_CLUB` |
| **Bludgeon (7)** | 1-Handed | Any | `WEAPON_TYPE_CLUB` |
| **Whip (1, 2)** | Any | Any | `WEAPON_TYPE_WHIP` |
| **Claw / Bite (8-10)**| Any | Any | `WEAPON_TYPE_UNARMED` |

---

## 5. Offline AI Batch Curation Strategy

### Why Offline Curation?
1. **Zero Runtime Dependency**: No external API calls, network requirements, or latency during build or server boot.
2. **Deterministic & Auditable**: All mappings are stored in JSON and tracked in git.
3. **High Fidelity on Nuance**: An offline script can inspect complete records (including room descriptions and extra descriptions) to suggest high-accuracy overrides for the ~79 ambiguous items.

### Workflow:
1. Run audit script to extract all weapons that fall through to Tier 3.
2. Generate candidate overrides in `rol_weapon_overrides.json`.
3. Verify against builder expectations.
4. Commit `rol_weapon_overrides.json` to repository.

---

## 6. Downstream Synergies with Other Gaps

Resolving `value[0]` unlocks automatic fixes for several other gaps in `ROL_CONVERTER_OBJECT_FILE_REFERENCES.md`:

1. **Proficiency Extension (`G` Block, Gap 3.7)**:
   `emit_object()` can look up `weapon_list[type].weaponFlags` to emit native `G` blocks (`ITEM_PROF_SIMPLE`, `ITEM_PROF_MARTIAL`, `ITEM_PROF_EXOTIC`).
2. **Material Extension (`H` Block, Gap 3.7)**:
   Inferred from description keywords (`steel`, `mithril`, `iron`, `wood`, `adamantine`, `bone`, `obsidian`, `silver`) or defaults to `weapon_list[type].material`.
3. **Size Extension (`I` Block, Gap 3.7)**:
   Derived from `weapon_list[type].size` (`SIZE_TINY`, `SIZE_SMALL`, `SIZE_MEDIUM`, `SIZE_LARGE`).
4. **Enhancement Bonus (`value[4]`)**: NOT a free win -- verify before doing
   this. RoL weapons carrying `APPLY_HITROLL` (`A 18 <n>`) and `APPLY_DAMROLL`
   (`A 19 <n>`) could in principle restate their `+N/+N` as Luminari's native
   enhancement bonus in `value[4]`. Two blockers:
   - **Double counting.** `compute_gear_enhancement_bonus()` reads `value[4]`
     while the `A` blocks apply to hitroll and damroll independently. Emitting
     both grants the bonus twice. The applies would have to be dropped in the
     same change, which is a behavior change, not a lossless addition.
   - **The target may discard it anyway.** `read_object()` zeroes `value[4]`
     when object cost is 100 or less (`src/db.c:5203`), and `set_weapon_object()`
     only runs under `ITEM_SET_STATS_AT_LOAD`, which the converter does not set.
   Treat this as an open question with its own gap entry, not as a downstream
   freebie of 3.3.

---

## 7. Implementation Plan

1. **Create `scripts/world/wtool_lib/rol_weapon_mapping.py`**:
   Implement the 4-tier classifier (`infer_weapon_type(record) -> (int, str)`).
2. **Add `scripts/world/wtool_lib/rol_weapon_overrides.json`**:
   Populate the curated overrides for the ~79 ambiguous/unusual records.
3. **Wire into `rol_transform.py:emit_object()`**:
   For `source_type == 5`, set `values[0] = target_weapon_type`. Classify from
   the `RolRecord` -- `record.values["values"]` and `record.values["flags"]` --
   *before* or independently of `_object_values()`, never from its return value.
   `_object_values()` has already overwritten `values[3]` with the target damage
   message by the time it returns, so the source verb is no longer recoverable
   from the list this step receives. See section 4.3.
4. **Add Unit Tests in `scripts/world/tests/test_rol_transform.py`**:
   Assert that 100% of all 1,319 RoL weapons map to a valid `WEAPON_TYPE_*` (1..79) and none to 0.
5. **Update Documentation**:
   Mark Item 3.3 as resolved in `docs/ongoing-projects/ROL_CONVERTER_OBJECT_FILE_REFERENCES.md`.

---

## 8. Corrections Applied to the First Draft

Recorded so the same errors are not reintroduced. Everything not listed here
was re-measured and held up: the verb decode, the verb distribution, the
proc-value distribution, the roughly 90% keyword coverage, and every one of the
37 cited `WEAPON_TYPE_*` names (all exist; the range 1..79 is correct, with
`NUM_WEAPON_TYPES` 80).

| # | Error in the first draft | Correction |
|:--|:--|:--|
| 1 | "Damage dice survive... converted weapons deal correct damage" | False. `MAX_WEAPON_NDICE 2` / `MAX_WEAPON_SDICE 12` clamp 30% of the corpus at load. New section 1.2; it strengthens the case for this work rather than weakening it. |
| 2 | Tier 3 thresholds keyed on average damage and weight | Both are pre-clamp or non-physical. Demoted to last-resort tiebreakers in 4.3. |
| 3 | Implementation step 3 set `values[0]` on the output of `_object_values()` | That function has already rewritten `values[3]`; reading the verb there mis-classifies 353 crush/bludgeon/bite records. Corrected in 4.3 and step 3. |
| 4 | VNUM 6's "100d100" read as evidence of an exotic thrown disk | 100d100 is a sentinel shared by 12 low-vnum artifact records, not a statistic. Note added under 4.1. |
| 5 | Two-handed count 142 (10.8%) | 237 (18.1%), measured on `BIT_23 == 1 << 22`, the bit the draft itself names. |
| 6 | Exotic/improvised bucket "~79 records" | About 128. |
| 7 | `value[3]` described as "1-based (1..11)" | Zero-based 0..11; verb 0 occurs in the corpus and is missing from `SOURCE_WEAPON_MESSAGE_MAP`. |
| 8 | Section 6.4 enhancement bonus presented as a free win | Double-counts against the retained `A` blocks, and the target may zero `value[4]` anyway. Reopened as a question. |
| 9 | Non-ASCII en-dashes and LaTeX math (`$\le 2$`) in the tables | Normalized; the repo requires valid ASCII documentation. |
