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

**Severity: high. 1319 records.**

Target weapon value 0 must be an index into `weapon_list[]`, which supplies
crit range, crit multiplier, damage types, weapon family, range, and
proficiency. Source weapon value 0 is a proc-value hook, 0 in 1305 of 1319
corpus weapons. Converted weapons therefore land on `weapon_list[0]`, which is
`WEAPON_TYPE_UNDEFINED`.

Damage dice survive, because both formats carry them in values 1 and 2, so
converted weapons deal correct damage with an undefined weapon profile.

That last sentence is wrong, and the correction matters:
`MAX_WEAPON_NDICE 2` / `MAX_WEAPON_SDICE 12` (`src/olc/oasis.h`) are enforced at
load (`src/db.c:5162`) and flatten 30% of the corpus to a common 2d12 ceiling.
With dice capped uniformly, `weapon_list[]` is where nearly all surviving weapon
differentiation lives. Proposal, corpus analysis, and the full correction list:
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md).

## 3.4 Primary and `*_MAX` stat applies convert 1:1 on the D20 scale

**Status: resolved. Converted 1:1.**

Luminari operates on the standard D20 (3..18+) ability score scale where every 2
stat points = +1 modifier. The 4.5x inflation factor from RoL's 0..100 engine loader
was removed. All primary stat applies (including `*_MAX` applies mapped to base stats)
convert 1:1 from their raw source values, and race-factor applies (41, 43, 45, 48)
are omitted as source-only attributes. Converted item applies default to
`BONUS_TYPE_UNIVERSAL` (23).

## 3.5 Missile type index is passed through untranslated

**Severity: medium. 48 records.**

Source `ITEM_MISSILE` value 3 is a one-based missile type, 1-16 in RoL's
`missiles[]` table, and source values 0-2 carry dice and condition. Target
`ITEM_MISSILE` uses a different value layout. This is the same class of defect
as 1.2 but for missiles, and it needs the target-side missile semantics
confirmed before a map can be written.

## 3.6 Object level is always 1

**Severity: medium. All records.**

The source format has no level field at all (mapping Part 2.5), so
`emit_object` falls back to the economy default of 1. Every converted object is
therefore usable at level 1, and the source's own gating -- anti-class extra
flags -- is the only restriction that survives.

The target clamps level to 1-30 at load, so any future level inference must
stay inside that range to round-trip.

## 3.7 Structured target extensions are never emitted

**Severity: medium. All records.**

`emit_object` writes only the header, the three numeric rows, `E`, `A`, `Z`,
and `T` blocks. It never writes `G` (proficiency), `H` (material), `I` (size),
`B` (spellbook), `C` (weapon special abilities), `K` (activated spells), or `S`
(weapon proc spells).

Consequences: every converted object gets material `MATERIAL_UNDEFINED` and
proficiency `ITEM_PROF_NONE`, and a missing `I` block means the loader's
size-0 rewrite silently makes every converted object `SIZE_MEDIUM`.

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

## 3.11 Checked and found benign

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
