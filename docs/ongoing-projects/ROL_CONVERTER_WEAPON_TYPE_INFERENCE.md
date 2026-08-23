# RoL Converter: Weapon Type Inference -- Remaining Work

Status: implementation is current through native thrown weapons. Builder
validation and the next development data release remain.

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

### 1.3 Ranged name/type disagreements and resolved engine contracts

The declared RoL ranged type currently wins when the object text suggests a
different type. Review all 14 rows in `audit()["name_disagreements"]`; the
current VNUMs are:

`1011`, `4798`, `20112`, `20261`, `21721`, `58634`, `58912`, `58916`, `59366`,
`83036`, `83217`, `91237`, `94534`, and `94719`. VNUM `94719` is the deliberate
physical-dart split: its declared source type makes it a thrown weapon even though the generic
ammunition name rules recognize the word `dart`.

Also review the target-engine compromises that are not all name disagreements:

- Javelin records preserve `WEAPON_TYPE_JAVELIN`. Javelins remain ordinary melee weapons until a
  player explicitly uses `throw`.
- Source range type 10 and source missile type 10 become thrown `WEAPON_TYPE_DART` objects. Source
  range type 16 becomes the append-only `WEAPON_TYPE_BLOWGUN`; its type-16 missile remains
  `AMMO_TYPE_DART`. The current active corpus contains no authored type-16 record.
- All 44 source quivers now become `ITEM_AMMO_POUCH`: 24 archery quivers and 20 throwing quivers.
- Four `MOB_ROL_ARCHER` mobiles have throwable-only loadouts and now select thrown mode. Twenty-five
  have launchers; the remaining 13 have no launcher or throwable reset and remain inactive under
  the compatibility flag.
- Joke, siege, improvised, body-part, and placeholder records need an explicit
  keep/change/exclude decision even when their mechanical classification is
  deterministic.

The affected-object and NPC review is recorded in
[THROWN_WEAPONS_CONVERSION_AUDIT.md](THROWN_WEAPONS_CONVERSION_AUDIT.md).

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
