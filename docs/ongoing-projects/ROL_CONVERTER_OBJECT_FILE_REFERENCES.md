# RoL Converter: Object Conversion File References

Format authority for both sides of this conversion is
[OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md), which maps every
field of the Luminari `.obj` and Realms of Luminari `.obj` formats to the
loaders and writers that define them.

All paths are repo-relative.

## 3.2 Armor type index is populated from an unrelated source field

**Severity: high. 2527 records.**

Target armor value 1 must be an index into `armor_list[]`, which supplies armor
type, max dex bonus, armor check penalty, spell failure, and movement rates.
Source armor value 1 is "warmth", an unrelated RoL statistic. The converter
passes it through, so armor type is effectively random.

In the corpus, 2426 of 2527 armor records carry warmth 0, so most converted
armor lands on `armor_list[0]`; the remaining 101 spread across 19 further
values up to 60. A real fix needs a source-to-target armor type mapping.

Proposal and corpus analysis:
[ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md](ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md).
Two findings there change this item's scope. `SPEC_ARMOR_TYPE_*` is a 13-family
by 4-slot grid running to 58 entries, so the slot is deterministic from the wear
mask and only the family needs inference -- a 13-way decision. And `armor_list[]`
covers only BODY, HEAD, ARMS, LEGS, and SHIELD, so just 1157 of the records are
eligible; the other 1366 sit on slots with no armor semantics at all and are
proposed as separate Item 3.2b. Inferring from AC is specifically ruled out
there.

## 3.3 Weapon type index is populated from the source proc value

**Status: resolved. 1319 records classified.**

Target weapon value 0 must be an index into `weapon_list[]`, which supplies
crit range, crit multiplier, damage types, weapon family, range, and
proficiency. Source weapon value 0 is a proc-value hook, 0 in 1305 of 1319
corpus weapons, so every converted weapon used to land on `weapon_list[0]`,
which is `WEAPON_TYPE_UNDEFINED`. Damage dice do not compensate:
`MAX_WEAPON_NDICE 2` / `MAX_WEAPON_SDICE 12` (`src/olc/oasis.h`) are enforced at
load (`src/db.c:5162`) and flatten 401 of the 1319 records onto a common 2d12
ceiling, which is what makes `weapon_list[]` carry nearly all surviving weapon
differentiation.

`scripts/world/wtool_lib/rol_weapon_mapping.py` now infers the type from a
curated override catalog, an ordered keyword rule engine, and a mechanical
verb-and-handedness fallback matrix; `emit_object()` writes the result into
value 0. All 1319 active source weapons resolve to a defined type in 1..79,
across 43 distinct `WEAPON_TYPE_*` values, and none to a ranged type. Remaining
follow-on work -- builder validation, the downstream `G`/`H`/`I` extension
blocks, and the `value[4]` enhancement-bonus question -- is tracked in
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md).

## 3.4 Primary and `*_MAX` stat applies convert 1:1 on the D20 scale

**Status: resolved. Converted 1:1.**

Luminari operates on the standard D20 (3..18+) ability score scale where every 2
stat points = +1 modifier. The 4.5x inflation factor from RoL's 0..100 engine loader
was removed. All primary stat applies (including `*_MAX` applies mapped to base stats)
convert 1:1 from their raw source values, and race-factor applies (41, 43, 45, 48)
are omitted as source-only attributes. Converted item applies default to
`BONUS_TYPE_UNIVERSAL` (23).

## 3.5 Ranged weapons and their ammunition do not convert

**Status: resolved. 99 object records, 44 quivers, and the archer path.**

Source `ITEM_FIREWEAPON` now becomes `ITEM_WEAPON` on a real `weapon_list[]`
index classified from the range type the record declares; source `ITEM_MISSILE`
keeps its type on a classified `AMMO_TYPE_*`, except for the four thrown
records, which are retyped to the melee weapon they are. The three misread
value slots -- the imbued spell number, the inverted break probability, and the
loaded-ammo counter -- are fixed. Quivers split 24 archery pouches to 20
throwing containers, and `MOB_ROL_ARCHER` mobiles reload. Implementation record:
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md)
sections 2 and 2.10. What was found originally:

Both halves of the ranged chain were broken, and the plan for all of it is
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md)
section 2, which carries the verified mapping tables, value-slot rules, and
implementation order. Summary of what was found:

- **Launchers (51).** Source `ITEM_FIREWEAPON` maps to the target's deprecated
  `ITEM_FIREWEAPON` (7), whose `value[0]` indexes a two-entry `ranged_weapons[]`
  table. `is_using_ranged_weapon()` tests
  `weapon_list[value[0]].weaponFlags & WEAPON_FLAG_RANGED` without looking at
  item type, so every record fails and nothing can fire. They must become
  `ITEM_WEAPON` (5) with a real `weapon_list[]` index.
- **Ammunition (48).** Source `value[3]` is a one-based missile type, 1-21 in
  RoL's `missiles[]` table (not 1-16 -- corrected in mapping Part 2.9), and it
  is never read; source `value[0]` (a damage die) passes through into the
  target's `AMMO_TYPE_*` slot instead.
- **Two target slots mean something else entirely.** Missile `value[1]` is the
  target's imbued-spell number (`imbued_arrow()`, `src/combat/fight.c:12057`),
  so converted arrows currently cast a spell chosen by their source dice size --
  `SPELL_CHILL_TOUCH`, `SPELL_CALL_LIGHTNING`, `SPELL_TELEPORT`. Missile
  `value[2]` is a break-probability percent while the source stores durability
  on an inverted 1-10 scale.
- **Delivery.** RoL quivers already map to `ITEM_AMMO_POUCH`, but 20 of the 44
  are throwing quivers holding thrown weapons, which an ammo pouch may not
  hold; those must become plain containers. And the zone equipment positions
  are shifted by one (gap 3.11), which puts all 103 quiver equips on the badge
  slot.
- **The NPC path already exists.** `ACT_ARCHER` maps to `MOB_ROL_ARCHER` and
  `src/mob/mob_act.c:424` implements it, so the mobile side needs no new work.

## 3.6 Object level is always 1

**Severity: medium. All records.**

The source format has no level field at all (mapping Part 2.5), so
`emit_object` falls back to the economy default of 1. Most converted objects are
therefore usable at level 1, and the source's own gating -- anti-class extra
flags -- is the only restriction that survives. The exception is the 183 records
of gap 3.13, whose fourth parsed economy field is an affect bitmask rather than
a level.

The target clamps level to 1-30 at load, so any future level inference must
stay inside that range to round-trip.

## 3.7 Structured target extensions are never emitted

**Severity: medium. All records.**

`emit_object` writes the header, the three numeric rows, `E`, `A`, `Z`, and `T`
blocks, plus `G` (proficiency), `H` (material), and `I` (size) **for converted
weapons only**, where `weapon_list[]` supplies all three. It never writes `B`
(spellbook), `C` (weapon special abilities), `K` (activated spells), or `S`
(weapon proc spells), and non-weapon records still get none of `G`/`H`/`I`.

Consequences for everything that is not a weapon: material
`MATERIAL_UNDEFINED`, proficiency `ITEM_PROF_NONE`, and a missing `I` block
means the loader's size-0 rewrite silently makes the object `SIZE_MEDIUM`.

`C` is the notable one: it is the target's native representation of exactly the
kind of themed weapon effect RoL encodes as a proc value, so it is the natural
destination for gap 2.3's proc-value data.

## 3.8 Non-`&+` source color forms are not converted

**Severity: low. 54 occurrences in the object corpus.**

`_SOURCE_COLOR` matches `&+X` and `&N`/`&n` only. The corpus also contains
`&-L` (2), `&-<` (1), and a scattering of `&=`, `&_`, `&%`, `&&`, `&$`, `&c`,
`&I`, `&<`, and `&g`. These pass through as literal text and will display as
themselves in the target.

Some are probably typos in the source rather than real codes, so this needs a
pass over the actual occurrences before deciding which to map and which to
strip.

## 3.9 Source spell indices are assumed zero-based and unverified

**Severity: unknown. Needs verification.**

Source scroll, potion, wand, and staff spell slots are stored one-based -- the
source `stat` display does `sprinttype(value[i] - 1, spells, ...)`
(mapping Part 2.9). `_SOURCE_SPELL_MAP` in `rol_transform.py` is a small
curated table keyed on the raw file value, and whether its keys already account
for the one-based offset could not be established from the converter alone;
RoL's `spells[]` table is not defined in the vendored source tree.

If the keys are off by one, every mapped magic item is converting the wrong
spell. This should be checked before any further entries are added to that
table.

## 3.10 `ITEM_SHIP` does not carry the forced light bit

**Severity: low.**

`read_object()` force-sets `ITEM_LIT` on every `ITEM_SHIP` (mapping Part 2.10).
Source type 28 maps to target `ITEM_BOAT` (22) and the forced light bit is not
reproduced.

## 3.11 Zone equipment positions above 16 are shifted by one

**Status: resolved. 1,247 resets.** `EQUIPMENT_POSITION_MAP` is now built from
the source `WEAR_*` constants the zone `E` command actually carries, and
`test_equipment_positions_map_to_the_target_wear_constants` reparses both
headers so neither side can renumber a slot silently. Source position 25
(`WEAR_TAIL`) has no target equivalent and is still dropped, with a diagnostic.
What was found:

`EQUIPMENT_POSITION_MAP` in `rol_transform.py` was built from RoL's
`equipment_types[]` display table, which omits `SECONDARY_WEAPON`, rather than
the `WEAR_*` constants the zone `E` command actually uses. Every source
position from 17 up lands one slot early: quivers (23) become badges, held
items become eye gear, eye gear becomes face gear, offhand weapons become held
items, and positions 24 and 25 are rejected outright so those resets are
dropped. The target's `E` handler validates only slot bounds
(`src/db.c:5967`), so it all loads silently. The corrected table and the corpus
evidence for it are in
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md)
section 2.6.2. This is not a ranged-specific defect, but it blocks the ranged
chain completely.

## 3.12 Checked and found benign

Recorded so they are not re-investigated:

- **Object timer default of 1.** `emit_object` defaults the target timer to 1.
  Timer-driven extraction in `point_update` (`src/limits.c`) is gated on
  `IS_DECAYING_PORTAL`, `ITEM_DECAY`, or `IS_CORPSE`, none of which the
  converter sets on ordinary objects, so converted objects do not decay.
- **Trailing content after a source `T` block.** The source loader stops
  reading after the trap block, and the converter already classifies later
  content as `IGNORED_SOURCE_CONTENT` with diagnostic `ROLOBJ004`, which is the
  behavior-preserving reading.
- **Negative source weights.** Clamped to zero by an existing guard, both
  before and after the drink-container change in 1.3.

## 3.13 The source economy row swallows the affect flag words

**Severity: high. 183 object records, 35 of them weapons. Found 2026-08-22.**

The source economy fields are read with three separate `fscanf(" %d ")` calls
and are not line-bound (mapping Part 2.5), and the two affect-flag words that
follow are read the same way. 183 active source objects put one or both affect
words on the same physical line as their three economy fields.
`rol_source._parse_obj` reads the economy row as a whole line, so for those
records the trailing affect words are parsed as economy fields 4 and 5.

Two consequences. The affect words are lost -- they never reach the
`AFFECT_FLAGS` handling, so those objects convert with no affects at all. And
`emit_object` maps economy field 4 to the target object level, so an affect
bitmask lands there: source object 19730, a longsword, converts to level
536,870,972, which the target clamps to 30. That contradicts gap 3.6's claim
that every converted object is level 1, and it gates 35 converted weapons
behind level 30.

The fix belongs in the source parser, not the emitter: split the economy row at
three fields and hand any remainder to the affect-word path, which already
carries the correct one-based bit offsets.

---

# Part 4: File reference index

## Conversion core

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_lib/rol_source.py` | `_parse_obj` -- the RoL `.obj` source grammar: string block, `FLAGS`/`VALUES`/`ECONOMY` rows, affect-flag rows, and `E`/`A`/`T` extensions. Emits diagnostics `ROLOBJ001` through `ROLOBJ006`. Also carries the object reference extraction for container keys, teleport destinations, summoned mobiles, and switch rooms. 1372 lines total. |
| `scripts/world/wtool_lib/rol_transform.py` | `emit_object` -- target `.obj` emission. Owns `OBJECT_TYPE_MAP`, `OBJECT_EXTRA_MAP`, `OBJECT_WEAR_MAP`, `OBJECT_SOURCE_ONLY_FLAGS`, `OBJECT_SOURCE_ONLY_WEAR_FLAGS`, and the object trap constants `ROL_OBJECT_TRAP_EXTRA_BIT`, `ROL_OBJECT_TRAP_EFFECT_MASK`, `ROL_OBJECT_TRAP_DAMAGE_TYPES`, `ROL_OBJECT_TRAP_VALUE_OFFSET`. 2164 lines total. |

## Emission callers

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_lib/rol_phase7.py` | imports and calls `emit_object` during Phase 7 conversion |
| `scripts/world/wtool_lib/rol_pilot_build.py` | imports and calls `emit_object` when building pilot bundles |
| `scripts/world/wtool_lib/rol_capability_audit.py` | audits `OBJECT_TYPE_MAP`, `OBJECT_EXTRA_MAP`, `OBJECT_WEAR_MAP`, and `OBJECT_SOURCE_ONLY_FLAGS` for coverage; routes object records through `emit_object` |

## Object-aware orchestration and analysis

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_lib/rol_discovery.py` | `obj` to `object` kind mapping; names `EXAMPLE/RealmsOfLuminari/src/db.c` as the source-format authority and `src/obj/shop.c` for the owned extended-shop adapter |
| `scripts/world/wtool_lib/rol_graph.py` | object records and object destinations in the cross-reference graph; object-owner role classification |
| `scripts/world/wtool_lib/rol_planner.py` | includes `obj` in `_ENTITY_KINDS` and in the per-kind planning walk |
| `scripts/world/wtool_lib/rol_inventory.py` | `obj` in `SOURCE_KINDS`; maps `obj` to the `AREA.mobobj` inventory grouping |
| `scripts/world/wtool_lib/rol_identity.py` | includes `obj` in `_CORE_KINDS` for identity assignment |
| `scripts/world/wtool_lib/rol_baseline.py` | includes `obj` in the new-entity VNUM range set |
| `scripts/world/wtool_lib/rol_phase8.py` | `obj` to `object` kind mapping; baseline object VNUM set for release comparison |
| `scripts/world/wtool_lib/rol_special.py` | `RECONCILED_OBJECT_RUNTIME_HANDLERS`; object spec-proc owner resolution, instrument replacement, and the `DG_OBJECT_TRANSFORM` disposition |
| `scripts/world/wtool_lib/rol_special_reconciliation.py` | `object` to `obj` record-kind mapping for spec-proc reconciliation |
| `scripts/world/wtool_lib/rol_pilot.py` | `object` to `obj` binding-kind mapping |
| `scripts/world/wtool_lib/rol_persistence.py` | object-prototype persistence bindings: loot chest state keys, MySQL board object prototypes, `object_database_items` |

## Target-side readers and validators

These validate emitted Luminari object data rather than converting RoL source.

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_lib/objects.py` | parser and validator for Luminari object prototype files and their extension records; owns finding codes `OBJ001`, `OBJ010`, `OBJ019`, `OBJ020`, `OBJ021`. 762 lines. |
| `scripts/world/wtool_lib/world.py` | imports `parse_object_file` and wires object records into the world load |
| `scripts/world/wtool_lib/flags.py` | `_decode_alpha`, `decode_tokens`, `encode_bits` -- the ASCII flag-token encoding used by every Luminari object flag word |
| `scripts/world/wtool_lib/constants.py` | harvests `item-types`, `obj-extra`, and `obj-wear` tables from `src/structs.h` |
| `scripts/world/wtool_lib/lookup.py` | `obj` and `object` record-type normalization for record lookup |
| `scripts/world/wtool_lib/indexes.py` | object index file validation |
| `scripts/world/wtool_lib/semantics.py` | unfinished-object placeholder detection and color-code stripping |
| `scripts/world/wtool_lib/models.py` | `ObjectRecord`; `prerequisite_object_vnum` and `reward_object_vnum` fields |
| `scripts/world/wtool_lib/cli.py` | exposes the `trigger-types-object` table under the `object` entry |

## Data files

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_constants.json` | tables `item-types`, `obj-extra`, `obj-wear`; flag set aliases `obj-affect`, `obj-affect2`; limits `MAX_OBJ_AFFECT`, `MAX_SHOP_OBJ`, `ITEM_SPECAB_HORN_OF_SUMMONING`, `ITEM_SPECAB_ITEM_SUMMON` |
| `scripts/world/rol_conversion_policy.json` | `identity.historic_target_lineage` entries with `source_kind: "obj"` and `target_type: "object"` |

## Tests

| File | Object connection |
|------|-------------------|
| `scripts/world/tests/test_rol_transform.py` | 103 tests total; 15 named object tests plus the object flag, value, apply, and trap conversion coverage. 3788 lines. |
| `scripts/world/tests/test_rol_source.py` | 11 parser tests including the RoL `.obj` grammar cases. 220 lines. |
| `scripts/world/tests/test_objects.py` | 4 tests for the target object prototype parser. 65 lines. |
| `scripts/world/tests/test_rol_phase7.py` | object emission within Phase 7 conversion |
| `scripts/world/tests/test_rol_pilot_build.py` | object emission within pilot bundle builds |
| `scripts/world/tests/test_rol_identity.py` | object identity and VNUM assignment |
| `scripts/world/tests/test_rol_graph.py` | object nodes and edges in the reference graph |
| `scripts/world/tests/test_rol_baseline.py` | object entries in the baseline range checks |
| `scripts/world/tests/test_rol_planner.py` | object records in planning output |
| `scripts/world/tests/test_rol_inventory.py` | object source-kind inventory |
| `scripts/world/tests/test_graph.py` | target-side object reference graph |
| `scripts/world/tests/test_lookup.py` | object record lookup |
| `scripts/world/tests/test_indexes.py` | object index validation |
| `scripts/world/tests/test_semantics.py` | unfinished-object semantics |
| `scripts/world/tests/test_cli.py` | object-related CLI surface |

## Test fixtures

| Path | Object connection |
|------|-------------------|
| `scripts/world/tests/fixtures/phase2/complete/obj/100.obj` | target-format object fixture |
| `scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/alpha.obj` | RoL source object fixture |
| `scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/companion.obj` | RoL source object fixture |
| `scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/disabled.obj` | RoL source object fixture |
| `scripts/world/tests/fixtures/rol_inventory/valid/areas/obj/orphan.obj` | RoL source object fixture |
| `scripts/world/tests/fixtures/rol_inventory/valid/areas/AREA.mobobj` | inventory grouping fixture |
| `scripts/world/tests/fixtures/rol_inventory/malformed/areas/AREA.mobobj` | inventory grouping fixture |

## Documentation gate

| File | Object connection |
|------|-------------------|
| `scripts/world/wtool_lib/docs_check.py` | requires `docs/world_game-data/OEDIT_GUIDE.md`, the builder-facing object reference, to be present |
