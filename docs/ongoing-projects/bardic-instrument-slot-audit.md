# Bardic Instrument Equipment-Slot Audit

Date: 2026-08-05

Status: Confirmed, repair pending

Scope: Bardic performance instrument discovery, object vnum 34549, instrument creation,
equipment persistence, the flame-kissed transformation procedure, and focused test coverage.
This is a source-and-world-data audit. No production or gameplay code was changed.

## Executive Summary

The character and object are not the cause of the reported message. Object vnum 34549 is a
valid `ITEM_INSTRUMENT`, has valid instrument values, and is equipped in the dedicated
`WEAR_INSTRUMENT` slot. The equipment text `{Used As Instrument}` proves that last point.

The active bardic performance engine does not inspect `WEAR_INSTRUMENT`. It only searches
`WEAR_HOLD_1`, `WEAR_HOLD_2`, and `WEAR_HOLD_2H`. It therefore deterministically treats an
instrument in the game's designated instrument slot as absent.

For vnum 34549 this means that every verse:

- prints `You perform without an instrument...`;
- applies the no-instrument effectiveness penalty;
- ignores its 30-point difficulty reduction;
- ignores its 10-point effectiveness bonus when it is transformed into the ideal subtype; and
- skips the instrument breakability path (irrelevant for this object because its value is zero).

This is systemic rather than object-specific. Boot conversion gives every `ITEM_INSTRUMENT`
the dedicated wear flag, while crafted and summoned instruments are created with only the
take and dedicated instrument wear flags. Those runtime-created instruments cannot be put in
any of the three slots that bardic performance currently searches.

## Reported Symptom and Deterministic Failure Chain

```text
wear flame
  -> find_eq_pos() selects WEAR_INSTRUMENT (slot 32)
  -> equipment displays {Used As Instrument}

perform <performance>
  -> process_bardic_performance_slot_internal()
  -> searches slots 17, 19, and 21 only
  -> does not inspect slot 32
  -> instrument remains NULL
  -> effectiveness -= 3
  -> "You perform without an instrument..."
```

The immediate first verse and later global verse pulses use the same internal function, so the
message and lost bonuses recur. A character with two active performance slots can traverse the
same incorrect discovery path twice on a pulse.

## Evidence

### 1. The equipment label identifies slot 32

`src/constants.c:1698-1718` maps `WEAR_INSTRUMENT` to the exact player-facing text
`{Used As Instrument}`. `src/act.informative.c:7301-7350` prints the label associated with the
actual populated equipment slot. It is not an object wear-capability label.

`src/structs.h:1837-1853` defines the relevant slots:

| Slot | Value |
|------|------:|
| `WEAR_HOLD_1` | 17 |
| `WEAR_HOLD_2` | 19 |
| `WEAR_HOLD_2H` | 21 |
| `WEAR_INSTRUMENT` | 32 |

### 2. Bardic performance omits the designated slot

`src/bardic_performance.c:1495-1506` searches the three held slots above and sets the local
instrument pointer to `NULL` if none contains an `ITEM_INSTRUMENT`. There is no read of
`WEAR_INSTRUMENT` anywhere in the bardic performance subsystem.

`src/bardic_performance.c:1511-1541` then applies one of three paths:

- no instrument: subtract 3 effectiveness and print the reported message;
- ideal instrument: apply value 1 to difficulty and value 2 to effectiveness; or
- wrong instrument: apply value 1 to difficulty and subtract 2 effectiveness.

The omission therefore changes mechanics, not only messaging.

### 3. Normal equipment selection chooses the omitted slot

`src/obj/act.item.c:4395-4510` maps the `instrument` wear keyword to
`WEAR_INSTRUMENT`. With no explicit position, `find_eq_pos()` also selects that slot when the
item has `ITEM_WEAR_INSTRUMENT`. `src/obj/act.item.c:4170-4177` validates that wear bit for
slot 32, and `src/obj/act.item.c:4221` explicitly calls it an equipped instrument.

`src/db.c:4891-4895` adds `ITEM_WEAR_INSTRUMENT` to every loaded object whose type is
`ITEM_INSTRUMENT`. The canonical slot is therefore the normal destination for the entire item
type.

Persistence is not dropping the item or changing its type. `src/obj/objsave.c:680-706`
explicitly validates `WEAR_INSTRUMENT` during auto-equip, and equipment saves use the normal
slot-plus-one location contract.

### 4. Vnum 34549 is valid and correctly configured

`lib/world/obj/345.obj:942-965` defines vnum 34549 with:

| Property | Stored value | Meaning |
|----------|-------------:|---------|
| Item type | 38 | `ITEM_INSTRUMENT` |
| Wear flags | `aou` | take, hold, and instrument |
| Value 0 | 0 | initial subtype: lyre |
| Value 1 | 30 | difficulty reduction / quality |
| Value 2 | 10 | effectiveness bonus |
| Value 3 | 0 | unbreakable |

`src/spec_assign.c:843-844` assigns the `flamekissed_instrument` special procedure. The
procedure at `src/zone_procs.c:2497-2625` changes only value 0 among the six valid instrument
subtypes. It does not change the object type or remove a wear flag. Its `is_wearing()` gate
scans every equipment slot (`src/spec_procs.c:2438-2448`), including slot 32.

The prototype and special procedure therefore do not explain the absence result.

### 5. Respec is not corrupting the item

The respec path removes equipment by iterating every slot from zero through `NUM_WEARS - 1`
(`src/character/class.c:2895-2898`). It does not rewrite an equipped object's item type or
instrument values. Re-equipping the object through the normal wear path places it in slot 32,
which exposes the pre-existing discovery mismatch.

## Findings

| ID | Severity | Type | Finding |
|----|----------|------|---------|
| BI-001 | High | Confirmed bug | Bardic performance ignores the dedicated instrument equipment slot. |
| BI-002 | High | Confirmed systemic impact | Crafted and summoned instruments cannot enter any slot the engine searches. |
| BI-003 | Medium | Test gap | The production-linked bard suite has no equipped-instrument coverage. |
| BI-004 | Medium | Confirmed rules/UI defect | Breakability code, comments, crafting text, and OLC text describe different probabilities. |
| BI-005 | Medium | State-integrity warning | The flame-kissed transform subtracts hit points directly without normal damage/death handling. |
| BI-006 | Low | Robustness warning | Instrument subtype display paths index a table without validating object value 0. |
| BI-007 | Low | Builder-facing drift | Value 2 is inconsistently called level or effectiveness, and identify misspells effectiveness. |

### BI-001: Dedicated Slot Is Ignored

Severity: High

This is the direct cause of the report. Slot 32 is purpose-built, visible in `equipment`, saved
and restored, selected by `wear`, and automatically enabled for instrument objects. The one
consumer that needs it does not read it.

The defect applies to every base performance because all thirteen entries pass through
`process_bardic_performance_slot_internal()`. It is independent of character class state,
respec state, selected performance, object vnum, or instrument subtype.

Recommended repair:

1. Look in `WEAR_INSTRUMENT` first.
2. Retain the three held slots as compatibility fallbacks for old objects and explicit `hold`
   usage.
3. Require `ITEM_INSTRUMENT` in every candidate slot.
4. Define dedicated-slot precedence when a bard somehow has more than one instrument equipped.

A small helper such as `get_equipped_bardic_instrument()` would keep slot policy in one place
and make it directly testable.

Temporary workaround for vnum 34549 only:

```text
remove flame
hold flame
```

The object has the legacy hold wear flag, so this can place it in a searched slot if the
character has a free hand. This is not a general workaround: newer crafted and summoned
instruments lack the hold flag.

### BI-002: Crafted and Summoned Instruments Have No Searchable Slot

Severity: High

`src/craft/crafting_new.c:3770-3798` clears every wear flag on a completed crafted instrument,
then sets only `ITEM_WEAR_TAKE` and `ITEM_WEAR_INSTRUMENT`.

`src/magic/spells.c:2767-2801` does the same for the `summon instrument` spell. The spell says
the object appears "in your hands," but it actually puts the object in inventory. Wearing it
uses slot 32. The `hold` command rejects it because `src/obj/act.item.c:4794-4799` requires the
hold wear flag for ordinary objects.

These instruments can be equipped exactly as their creators intend, but the performance engine
cannot discover them. Fixing BI-001 repairs this entire path without adding inappropriate hold
flags.

The summon message should separately be changed to say the object appears in the bard's
possession/inventory, or the spell should intentionally equip it into an empty instrument slot.

### BI-003: Tests Exercise Only the No-Instrument Branch

Severity: Medium

`unittests/CuTest/test_bardic_performance.c` has broad state, timing, targeting, perk, and
protocol coverage. It never declares an `obj_data`, never sets `ITEM_INSTRUMENT`, and never
populates any hold or instrument equipment slot. Existing `do_perform()` tests therefore run
through the reported no-instrument path without asserting its message.

This explains how the earlier bardic performance repair suite could pass while the canonical
instrument path remained broken.

Required regression cases:

- an `ITEM_INSTRUMENT` in `WEAR_INSTRUMENT` is recognized;
- the no-instrument message is absent when slot 32 contains a valid instrument;
- quality and ideal-subtype effectiveness values affect the verse;
- a wrong subtype uses the wrong-instrument path rather than the absent path;
- a non-instrument object in slot 32 is rejected safely;
- legacy hold slots remain compatible, with documented precedence;
- crafted and summoned instruments work through their normal wear path;
- save/reload preserves recognition; and
- primary and secondary performance slots use the same instrument policy.

No new test source file is required; these cases belong in the existing production-linked
`test_bardic_performance.c` suite.

### BI-004: Breakability Probability Is Internally Inconsistent

Severity: Medium

This does not affect vnum 34549 because its breakability is zero, but it affects ordinary,
crafted, and summoned instruments once discovery is fixed.

The engine at `src/bardic_performance.c:1521-1528` requires both:

```c
!rand_number(0, 9)
rand_number(2, 11111) <= breakability
```

`rand_number()` is inclusive (`src/utils.c:2502-2520`). For a breakability value `b` from 2
through 11111, the per-evaluation probability is therefore:

```text
(1 / 10) * ((b - 1) / 11110) = (b - 1) / 111100
```

Consequences:

- value 1 can never break, despite crafting text claiming a 1-in-11,111 chance;
- value 30 breaks about 0.0261 percent per evaluation, roughly one tenth of the documented
  30-in-11,111 rate;
- value 2000 breaks about 1.80 percent per evaluation, while OLC says it breaks on first use;
- the summoned instrument's value 10 is commented as "10% breakability," but actually breaks
  about 0.0081 percent per evaluation; and
- the engine comment says "quality <= 0" even though the tested field is breakability value 3.

Relevant contradictory text appears at `src/craft/crafting_new.c:1597-1608`,
`src/olc/oedit.c:1360-1362`, and `src/magic/spells.c:2772-2776`.

Choose one probability contract, implement it with one clearly bounded roll, and update every
builder/player-facing description. If the crafting contract is authoritative, a direct roll
from 1 through 11,111 is the clearest expression.

### BI-005: Flame-Kissed Transformation Bypasses Damage Handling

Severity: Medium

Each transform branch at `src/zone_procs.c:2519-2623` performs:

```c
GET_HIT(ch) -= 20;
```

There is no minimum-HP precondition, position update, death processing, damage event, or damage
type. A character at 20 or fewer hit points can be left at zero or negative hit points without
the normal damage pipeline resolving the state. The text describes this as taking damage, but
the implementation also bypasses any intended fire/damage rules.

Recommended repair:

- decide whether the cost may kill;
- either reject transformation when the cost cannot be paid or use the standard damage path;
- apply the cost and action consumption once in a shared helper; and
- replace the six duplicated exact-case branches with validated subtype lookup.

The current exact `strcmp()` checks also make `say Lyre` fail while `say lyre` succeeds, an
avoidable usability inconsistency.

### BI-006: Instrument Subtype Display Is Not Range-Safe

Severity: Low

`src/obj/act.item.c:930-946` and `src/olc/oasis_list.c:514-519` index
`instrument_names[value_0]` without checking that value 0 is between zero and
`MAX_INSTRUMENTS - 1`. Normal OLC, crafting, summoning, and vnum 34549 produce valid values, so
this is not part of the report. A malformed world prototype or persisted override could still
cause an out-of-bounds read during identify/lore or object listing.

Use a shared subtype validation/name helper and show an explicit `INVALID` label for bad data.

### BI-007: Instrument Field Names Have Drifted

Severity: Low

The same object values are described differently across the subsystem:

| Value | Runtime behavior | Other labels found |
|------:|------------------|--------------------|
| 1 | Reduces performance difficulty | Quality, difficulty reduction |
| 2 | Adds ideal-instrument effectiveness | Level, instrument level, effectiveness |
| 3 | Controls break probability | Breakability; incorrectly called quality in one comment |

The identify output at `src/obj/act.item.c:933-945` also spells `Effectiveness` as
`Effextiveness`. Standardize named value macros and UI terminology so builders do not create
content against the wrong interpretation.

## Recommended Repair Order

1. Repair BI-001 with a centralized dedicated-first instrument lookup.
2. Add the BI-003 regression matrix before changing any other instrument behavior.
3. Verify vnum 34549, one crafted instrument, and one summoned instrument in the development
   server across equip, perform, save, and reconnect.
4. Resolve the breakability contract and add deterministic boundary tests.
5. Refactor and harden the flame-kissed transform.
6. Add subtype validation and reconcile field labels/help text.

Do not change vnum 34549's prototype to work around BI-001. Its type, wear flags, subtype
values, and special assignment are all valid.

## Acceptance Criteria

- `{Used As Instrument}` equipment is recognized by all base bardic performances.
- Vnum 34549 no longer prints the no-instrument message while in slot 32.
- In ideal form, vnum 34549 contributes value 1 (30) and value 2 (10).
- In a non-ideal form, it takes the wrong-instrument path while retaining its difficulty
  reduction.
- Crafted and summoned instruments work without adding a hold flag.
- An absent instrument still prints the existing message and applies the documented penalty.
- Legacy explicitly held instruments continue to work if compatibility is retained.
- Transforming vnum 34549 at low hit points follows a defined, valid damage/cost contract.
- Instrument identify/list output handles malformed subtype values without an invalid read.
- The production-linked test suite covers the dedicated slot and passes warning-free.
- After `make test`, `make install` is run so no root-level `circle` artifact is left behind.

## Validation Performed for This Audit

- Confirmed `APP_ENV=development` without modifying `lib/.env`.
- Traced equipment display, wear selection, save/load validation, bard performance, crafted and
  summoned instrument construction, vnum 34549 world data, and its special procedure.
- Searched the focused production-linked bard tests for instrument objects and equipment slots;
  none are present.
- Checked the inclusive RNG implementation and calculated the breakability probabilities above.
- Made no source, world-data, credential, configuration, or production changes.

No build was needed for this documentation-only audit. The primary failure is a direct,
deterministic slot mismatch, while the existing suite has no test capable of accepting or
rejecting that behavior.
