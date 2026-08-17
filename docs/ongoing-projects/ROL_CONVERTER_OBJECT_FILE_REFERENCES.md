# RoL Converter: Object Conversion File References

Status: six fidelity fixes applied, released through Phase 8, and live in the
development world; gap list open

Compiled: 2026-08-18. Fixes applied: 2026-08-18. Released and applied to
`lib/world`: 2026-08-18 (`rol-phase8-release-f48c21233dc70ca1`).

Format authority for both sides of this conversion is
[OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md), which maps every
field of the Luminari `.obj` and Realms of Luminari `.obj` formats to the
loaders and writers that define them. This document records the converter files
that participate in object conversion, the fidelity fixes made by auditing the
converter against that mapping, and the gaps that remain.

All paths are repo-relative.

---

# Part 1: Improvements applied

Each item below was found by comparing converter behavior against a loader
behavior established in the format mapping, was confirmed against the live RoL
corpus, and is covered by a new regression test.

Verification for the whole set:

- Full world-tool suite: 438 tests, all passing (was 428; 10 added).
- All 10378 active-corpus object records emit and re-parse through
  `parse_object_file` with zero error findings.
- Released through the full Phase 7 and Phase 8 pipeline and applied to the
  development world. See Part 2.

## 1.1 Literal `@` was not escaped for the target color parser

**Mapping reference:** Part 1.2, "Colour code encoding".

The target reader runs `parse_at()` over every tilde string
(`src/db.c:6592`), so a bare `@` becomes the color introducer and consumes the
following character; `@@` is the literal escape. `convert_text` translated RoL
`&+X` codes into `@X` but never escaped at-signs already present in the source
text, so any literal `@` in RoL prose silently became a color code in the
emitted file.

**Fix:** `convert_text` now escapes `@` to `@@` before introducing its own `@`
color codes. Ordering matters and is asserted by test.

**Scope:** `convert_text` is the single text funnel for every record kind, so
this corrects rooms, zones, and quests as well as objects.

**Corpus impact:** 27 object records. Source lines carrying a literal `@`:
109 in `areas/obj`, 31 in `areas/zon`, 4 in `areas/qst`, 2 in `areas/wld`.

## 1.2 Weapon damage-message index was passed through untranslated

**Mapping reference:** Part 2.9 (source `ITEM_WEAPON` value 3, one-based into
RoL `weapons[]`) against Part 1.8 (target weapon value 3, zero-based into
`attack_hit_text[]`).

The two verb tables overlap at different offsets, so a pass-through is right
for some values and wrong for others. Source 7 is `weapons[6]` "Bludgeon", but
target 7 is "pound"; the correct target value is 5.

**Fix:** added `SOURCE_WEAPON_MESSAGE_MAP` and applied it in `_object_values`
for source type 5. Out-of-range values, which the source runtime itself reports
as bugged, fall back to the target's "hit" verb with a diagnostic.

**Corpus impact:** 194 of 1319 weapons emit a different verb than before; 1 had
an out-of-range value. The remaining 1124 were already coincidentally aligned
and are unchanged.

## 1.3 Drink-container weight was emitted in source quarter pounds

**Mapping reference:** Part 2.5 and Part 2.10.

`read_object()` divides `ITEM_DRINKCON` weight by four at load, so the stored
number is quarter pounds. The target reader applies no such division, so
converted drink containers weighed four times their source weight.

**Fix:** `emit_object` divides source drink-container weight by four before the
existing non-negative clamp and the existing capacity-based weight bump, which
is the order the two runtimes imply.

**Corpus impact:** 289 of 385 drink containers. The rest had a source weight of
zero or negative, which the existing clamp already handled.

## 1.4 `APPLY_CHA` was excluded from the source stat rescale

**Mapping reference:** Part 2.7, "The `A` modifier scaling hack".

The source loader rescales applies in `APPLY_STR..APPLY_CON` (1-5) and
`APPLY_AGI..APPLY_LUCK` (26-30). The converter scaled 1-5, 26, and 27 but not
28, so converted charisma items came out at roughly a fifth of the magnitude of
every other stat item, while 29 and 30 are dropped earlier by
`OBJECT_SOURCE_ONLY_APPLIES` and never reach the scale.

**Fix:** introduced `SOURCE_SCALED_STAT_APPLIES`, which mirrors the loader's two
ranges exactly, and `SOURCE_MAX_STAT_APPLIES` for the separate `*_MAX` set. Both
are named and commented so the divergence noted in gap 3.4 is visible in code.

**Corpus impact:** 130 apply directives.

## 1.5 Negative stat applies rounded away from zero

**Mapping reference:** Part 2.7.

The loader computes `(tmp * 45) / 10` in C, which truncates toward zero. The
converter used Python `//`, which floors, so every negative stat apply whose
scale did not divide evenly came out one point harsher than the source runtime
produced.

**Fix:** added `_scaled_stat_modifier`, which truncates toward zero.

**Corpus impact:** 83 apply directives. Combined with 1.4, 204 distinct object
records change.

## 1.6 `AFF_HIDE` was converted into a working target affect

**Mapping reference:** Part 2.10.

`read_object()` strips `AFF_HIDE` from every object as it loads ("No hide
items."), so a source object carrying the bit confers nothing at runtime.
The converter mapped source affect 21 onto target `AFF_HIDE` (20), granting
behavior the source never had.

**Fix:** added `OBJECT_SOURCE_ONLY_AFFECTS`, following the existing
`OBJECT_SOURCE_ONLY_FLAGS` and `OBJECT_SOURCE_ONLY_WEAR_FLAGS` precedent, and
subtracted it before mapping. This is deliberately object-scoped: the source
clears the bit only in `read_object()`, so mobiles keep `AFF_HIDE` and the
shared `MOB_SOURCE_ONLY_AFFECTS` is untouched. `rol_capability_audit.py` was
updated so the `object_affect_flag` family counts the new set as covered.

**Corpus impact:** 3 objects.

## 1.7 Files changed

| File | Change |
|------|--------|
| `scripts/world/wtool_lib/rol_transform.py` | all six fixes: `convert_text` at-escape, `SOURCE_WEAPON_MESSAGE_MAP`, drink-container weight, `SOURCE_SCALED_STAT_APPLIES` / `SOURCE_MAX_STAT_APPLIES`, `_scaled_stat_modifier`, `OBJECT_SOURCE_ONLY_AFFECTS` |
| `scripts/world/wtool_lib/rol_capability_audit.py` | `object_affect_flag` coverage set includes `OBJECT_SOURCE_ONLY_AFFECTS` |
| `scripts/world/tests/test_rol_transform.py` | 10 new regression tests |
| `src/magic/spell_parser.c` | unrelated release blocker: four group and self buffs corrected to `violent = FALSE` (see 2.5) |
| `unittests/CuTest/test_spells_skills_production.c` | `Test_group_morale_buffs_are_registered_non_violent` |

---

# Part 2: Production conversion run

The fixes were released through the sanctioned pipeline and applied to the
development world on 2026-08-18.

## 2.1 Pipeline

Discovery, planning, capability audit, and Phase 6 were **not** re-run. The
accepted release reuses frozen upstream bundles by run id, and Phase 7
regenerates conversion from them against the frozen development baseline. The
same four bundles were reused:

| Phase | Bundle | Run id |
|-------|--------|--------|
| 1 | `phase1-canonical-20260814-release-a` | `rol-phase1-03cf9122d3b6e469` |
| 2 | `phase2-canonical-20260814-repeat` | `rol-phase2-663674cff12936c8` |
| 5 | `phase5-canonical-20260813-audit` | `rol-phase5-audit-1cdeebcf8afe38d3` |
| 6 | `phase6-canonical-20260814-final-a` | `rol-phase6-special-c12d66df6135ca25` |

Phase 7 was run twice into separate directories for the byte-identical repeat
that acceptance requires:

- `lib/rol-conversion/runs/phase7-objfix-20260818-a`
- `lib/rol-conversion/runs/phase7-objfix-20260818-repeat`

Both produced run id `rol-phase7-b12-44e089b7bd51e7dc` and the same
`staged_tree_sha256` `1fb92746b924d2f8ff783765fa552cd102d6079e78b70fc729a59f30db23dee5`.

Phase 7 acceptance matched the prior accepted release exactly: 12/12 batches,
258/258 packages, 71680/71680 records disposed, 1206 generated files, 1228 SOC
triggers, 14 special triggers, **0 new active errors**, runtime contract and
preservation both passing.

## 2.2 Fix diagnostics in the real run

Every fix fired, with counts matching the isolated measurements:

| Diagnostic | Count |
|------------|-------|
| `converted source drink-container weight ...` | 289 |
| `mapped source weapon damage message ...` | 194 |
| `escaped literal '@' as '@@' ...` | 32 |
| `omitted source-inert object affects ...` | 3 |
| `replaced out-of-range source weapon damage message ...` | 1 |

The at-escape count is 32 rather than the 27 measured over objects alone
because `convert_text` serves every record kind.

The at-escape fix turned out to be more consequential than first documented.
Source `god_rooms.obj` line 477 reads `&+yan Email address because of the
&+C@&n&+y sign,`. The old converter emitted `@C@@n@y`, in which the literal `@`
merged with the following reset code and consumed it, leaving a stray `n` in the
text. The new output `@C@@@n@y` preserves the literal at-sign *and* the reset.
The defect silently destroyed colour reset codes wherever a literal `@`
preceded one.

## 2.3 Gates and boots

Boots ran against a candidate lib assembled at
`lib/rol-conversion/runtime-validation-objfix-20260818/candidate-lib` (a copy of
`lib/` with the Phase 7 staged world overlaid), never against the live tree.

| Gate | Result |
|------|--------|
| `world_tools` | pass, 438 tests |
| `production_cutests` | pass, 774 tests |
| `install` | pass, root `circle` absent |
| `syntax_boot` | pass |
| `bounded_runtime_boot` | pass |
| `converted_boot_diagnostics` | pass, 0 findings |

Phase 8 acceptance: 258 packages, 71680 records, 1206 apply paths,
`repeat_generation_byte_identical` true, `new_active_errors` 0,
`cross_world_typed_references` 0, `ready_to_apply` true.

## 2.4 Apply and live boot

`rol-phase8-apply` changed 310 paths and left 896 already current.
`rol-phase8-completion` reported `complete`, `documentation_pass`, and
`repeat_apply_no_op` all true.

An independently verified snapshot of `lib/world` was taken before the apply, as
the runbook requires: a tar archive plus a 5010-entry sha256 manifest. The
post-apply manifest differs in exactly the 310 paths the apply reported.

The development MUD was then restarted through `autorun.sh` against the live
`lib` tree. It booted to `Boot db -- DONE.` and `Entering game loop.` on port
4100 with the newly installed binary. Boot diagnostics:

| Class | Count | Assessment |
|-------|-------|------------|
| `ITEM_AUTOPROC` without spec proc | 17 | pre-existing, native zones (`132xxx`, `15800`); identical count in the prior accepted release |
| `ZONE ERROR` zone 1481 line 31 | 1 | pre-existing, native zone; `lib/world/zon/1481.zon` is byte-identical before and after the apply |
| Converted-range (`2xxxxx`) diagnostics | 0 | none |

No new errors were introduced by the conversion changes.

## 2.5 Unrelated blocker fixed to reach the gate

`production_cutests` initially failed with
`Test_group_inspiration_affects_finish_with_nested_group_calculations`
(`unittests/CuTest/test_spells_skills_production.c`). This is C code and cannot
be affected by the Python converter work; it was pre-existing on master, and the
suite was green at `16ee5127` when the previous release was cut.

Root cause: `spello`'s eighth parameter is `violent`, and four group and self
buffs were registered `violent = TRUE`:

| Ability | Was | Sibling abilities |
|---------|-----|-------------------|
| `AFFECT_INSPIRE_COURAGE` | TRUE | `AFFECT_RALLYING_CRY` FALSE |
| `AFFECT_INSPIRE_GREATNESS` | TRUE | `AFFECT_GLORYS_CALL` FALSE |
| `AFFECT_FINAL_STAND` | TRUE | `AFFECT_PRESCIENCE` FALSE |
| `AFFECT_KNIGHTHOODS_FLOWER` | TRUE | |

Marking a buff violent has three wrong consequences in `mag_affects_full`:

1. The Slippery Mind branch rolls a Will save against the buff's own recipient
   and returns without applying it on success. Any character with
   `FEAT_SLIPPERY_MIND` therefore lost these buffs at random. This is what made
   the test intermittent.
2. Recasting hits the violent branch of the already-affected guard and reports
   `CONFIG_NOEFFECT` instead of refreshing the buff.
3. Beast Master "Shared Spells" explicitly shares only non-violent spells, so
   these buffs were never shared to animal companions.

All four were set to `FALSE` in `src/magic/spell_parser.c`, matching their
siblings. The C suite went from 773 with one failure to 774 passing, the extra
test being a new deterministic guard,
`Test_group_morale_buffs_are_registered_non_violent`, which asserts the flag
directly rather than relying on the random save.

## 2.6 Operational finding: do not re-run discovery against a converted world

An attempt to re-run `rol-discover` against the current `lib/world` produced a
`lineage-candidates.jsonl` of 4.7 GB and still growing before it was aborted;
historical runs of the same command produced about 62 MB. It was aborted on disk
headroom and the artifact deleted.

This is not caused by the object fixes: `rol_discovery` imports neither
`rol_transform` nor `rol_capability_audit`, verified by loading the module and
inspecting `sys.modules`.

The mechanism is `build_target_catalog`, which buckets every target record by
normalized display identity, and `lineage_candidates`, which renders every
record in a source record's bucket as a full candidate row. Measured against the
current tree: 74042 identity buckets holding 153400 records, with the largest
bucket, room identity `the trackless sea`, holding 5126 records. The wilderness
rooms alone make the rendering quadratic.

Re-running discovery is not required to re-release; Phase 7 consumes the frozen
Phase 1 bundle. Treat re-running `rol-discover` against a post-apply world as
unsupported until the candidate rendering is bounded.

---

# Part 3: Remaining gaps and issues

Ordered by corpus impact. None of these were changed in this pass; each either
needs a conversion-policy decision or a mapping table that does not exist yet.

## 3.1 Armor AC is emitted at one tenth of its source magnitude

**Severity: high. 2527 records.**

Target armor value 0 is AC in tenths -- `display_item_object_values` renders it
as `(float)GET_OBJ_VAL(item, 0) / 10.0` (`src/obj/act.item.c`). Source armor
value 0 is whole AC points, with a corpus median around 5. The converter passes
the number through, so a source 5 AC breastplate becomes 0.5 AC in the target.

The fix is a multiplication by 10, but it is a balance change across every
converted armor piece, so it needs an explicit decision rather than a quiet
correction.

## 3.2 Armor type index is populated from an unrelated source field

**Severity: high. 2527 records.**

Target armor value 1 must be an index into `armor_list[]`, which supplies armor
type, max dex bonus, armor check penalty, spell failure, and movement rates.
Source armor value 1 is "warmth", an unrelated RoL statistic. The converter
passes it through, so armor type is effectively random.

In the corpus, 2426 of 2527 armor records carry warmth 0, so most converted
armor lands on `armor_list[0]`; the remaining 101 spread across 19 further
values up to 60. A real fix needs a source-to-target armor type mapping,
probably inferred from wear slot and AC.

## 3.3 Weapon type index is populated from the source proc value

**Severity: high. 1319 records.**

Target weapon value 0 must be an index into `weapon_list[]`, which supplies
crit range, crit multiplier, damage types, weapon family, range, and
proficiency. Source weapon value 0 is a proc-value hook, 0 in 1305 of 1319
corpus weapons. Converted weapons therefore land on `weapon_list[0]`, which is
`WEAPON_TYPE_UNDEFINED`.

Damage dice survive, because both formats carry them in values 1 and 2, so
converted weapons deal correct damage with an undefined weapon profile. A fix
needs a dice-and-verb to weapon-type inference table.

## 3.4 `*_MAX` applies are scaled although the source loader does not scale them

**Severity: medium. Policy question. 101 corpus directives.**

The source loader rescales only `APPLY_STR..APPLY_CON` and
`APPLY_AGI..APPLY_LUCK`. It does not rescale `APPLY_STR_MAX..APPLY_CHA_MAX`
(31-38), whose file values are already on the source 0-100 scale. The converter
maps those onto the base stats they cap and applies the 4.5x scale anyway,
which overstates them by that factor relative to the source runtime.

This behavior predates this pass and is pinned by
`test_emitted_object_maps_extended_stats_and_repairs_source_defects`, which
asserts source `A 31 2` becomes target modifier 9. It was left alone because
changing it is a balance decision, not a format correction. The two sets are now
named separately in `rol_transform.py` so the divergence is visible at the call
site.

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
