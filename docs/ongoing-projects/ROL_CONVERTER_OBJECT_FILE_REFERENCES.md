# RoL Converter: Object Conversion -- Remaining Work

Status: the released object-conversion fixes are applied to the development
world. Only the unresolved fidelity work below remains.

Format authority:
[OBJ_FILE_FORMAT_MAPPING.md](OBJ_FILE_FORMAT_MAPPING.md)

All paths are repo-relative.

## 3.2 Armor type and non-armor-slot AC are not converted correctly

**Severity: high. 2,527 active source armor records.**

Target armor `value[1]` must index `armor_list[]`, but source `value[1]` is the
unrelated RoL warmth statistic. The converter still passes it through. Of the
2,527 active records, 2,426 therefore land on `SPEC_ARMOR_TYPE_UNDEFINED`; the
rest scatter across unrelated or out-of-range indices.

The detailed proposal is
[ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md](ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md).
Its well-formed 2,523-record analysis separates two required changes:

- **Item 3.2a: 1,157 eligible records.** BODY, HEAD, ARMS, LEGS, and SHIELD
  have `armor_list[]` rows. Infer only the armor family, compose it with the
  deterministic wear slot, and emit the resulting index. Decide first whether
  classification should favor conservative penalties or literal item flavor.
- **Item 3.2b: 1,366 ineligible records.** Other wear slots have no target
  armor semantics. Retype them to `ITEM_WORN` and restate source AC as an
  `APPLY_AC_NEW` affect without reversing its sign. Decide explicit
  dispositions for negative-AC curse items, empty wear masks, and malformed
  records.

Implementation still requires the classifier, curated overrides, emitter
wiring, corpus audit, and the tests listed in the armor proposal.

## 3.6 Object level has no conversion policy

**Severity: medium. All converted object records.**

The source format has no level field. `emit_object()` still supplies the target
economy-row default of level 1, so converted objects are usable at level 1
unless another surviving restriction prevents it.

Decide a reproducible level-inference policy, emit a value in the target's
loadable 1..30 range, and add corpus and round-trip tests. If level 1 is the
intended permanent policy, record that as an explicit design decision and
close this item rather than leaving it as an accidental default.

## 3.7 Structured target extensions are incomplete

**Severity: medium.**

`emit_object()` writes `E`, `A`, `Z`, and `T` blocks. Converted weapons also
receive `G` (proficiency), `H` (material), and `I` (size) from `weapon_list[]`.
The converter still emits no `B` (spellbook), `C` (weapon special abilities),
`K` (activated spells), or `S` (weapon proc spells) blocks, and non-weapons
receive none of `G`/`H`/`I`.

Inventory the source semantics that can be represented by each extension,
decide mappings, and add emission plus parser round-trip coverage. In
particular, source weapon and armor proc values and the special behavior behind
sentinel weapon records need an explicit disposition; a `C` or `S` block may
be appropriate where a converted `Z` special procedure does not already carry
the behavior. Non-weapons otherwise remain `MATERIAL_UNDEFINED`,
`ITEM_PROF_NONE`, and implicitly `SIZE_MEDIUM`.

## 3.8 Non-`&+` source color forms are not normalized

**Severity: low. 54 occurrences in the object corpus.**

`_SOURCE_COLOR` recognizes `&+X` and `&N`/`&n`. Forms including `&-L`, `&-<`,
`&=`, `&_`, `&%`, `&&`, `&$`, `&c`, `&I`, `&<`, and `&g` still pass through as
literal text.

Review every occurrence to distinguish real source color syntax from typos,
then map or strip each accepted form and add focused conversion tests.

## 3.10 Converted ships lose their forced light

**Severity: low. 12 active source ship records.**

The RoL loader force-sets `ITEM_LIT` on every `ITEM_SHIP`. Source type 28 becomes
target `ITEM_BOAT`, but the converter does not reproduce that loader-added
state because it is absent from the flat-file flags.

Force the mapped target `ITEM_MAGLIGHT` extra bit on converted source ships and
add a test covering emission and target-parser round trip for all active ship
records.
