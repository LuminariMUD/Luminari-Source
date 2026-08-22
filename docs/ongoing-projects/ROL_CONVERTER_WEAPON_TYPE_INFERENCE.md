# RoL Converter: Weapon Type Inference -- Remaining Work

Status: implementation and development release are complete. Only builder
validation remains.

Context:
[ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md)
Item 3.3

The melee, ranged-weapon, ammunition, quiver, equipment-position,
`set_weapon_object()`, and enhancement-bonus work is implemented and tested.
There is no further engineering work tracked in this file.

## 1. Complete the Builder Review Packet

Run `rol_weapon_mapping.audit()` against the canonical active corpus and review
the following decisions with the builders. The report, rather than the counts
copied here, is authoritative after any corpus or classifier change.

### 1.1 Curated overrides

Review every entry in
`scripts/world/wtool_lib/rol_weapon_overrides.json`: 55 melee overrides, one
ranged override, and one ammunition override in the current catalog. Confirm
the selected target type and the recorded rationale, or change the catalog.

### 1.2 Fallback classifications and record disposition

The current audit has 25 fallback records:

- 21 standard quest templates in the `7073`-`7098` range.
- The noshow placeholders `50403` and `33033`.
- The standard god weapon `1294`.
- The hiltless throwing dagger `21005`, retyped from special-flight ammunition.

Confirm each inferred type and decide whether every placeholder should convert
at all. Record exclusion is a builder disposition decision, not a classifier
rule.

### 1.3 Ranged name/type disagreements and compromises

The declared RoL ranged type currently wins when the object text suggests a
different type. Review all 13 rows in `audit()["name_disagreements"]`; the
current VNUMs are:

`1011`, `4798`, `20112`, `20261`, `21721`, `58634`, `58912`, `58916`, `59366`,
`83036`, `83217`, `91237`, and `94534`.

Also review the target-engine compromises that are not all name disagreements:

- Javelins become `WEAPON_TYPE_SHORTSPEAR` so they remain usable; the target's
  ranged javelin type has no ammunition-pouch pairing.
- Source darts use the target dart weapon/ammunition pairing.
- Thrown source ammunition becomes a melee weapon because the target has no
  throwing command.
- Three `MOB_ROL_ARCHER` mobiles equipped only with thrown weapons remain melee
  combatants, and 13 receive no ranged item at all; their archer flag is inert.
- Joke, siege, improvised, body-part, and placeholder records need an explicit
  keep/change/exclude decision even when their mechanical classification is
  deterministic.

### 1.4 Completion gates

The review is complete only when:

1. A builder decision is recorded for every override, fallback, disagreement,
   and explicit compromise above.
2. Any accepted changes are applied to the override catalog, classifier, or
   record-disposition policy.
3. `rol_weapon_mapping.audit()` is rerun and reports no undefined weapon or
   ammunition type.
4. `python3 -m unittest discover -s scripts/world/tests -t scripts/world -v`
   passes, including the full-corpus and header-drift guards in
   `test_rol_weapon_mapping.py`.
