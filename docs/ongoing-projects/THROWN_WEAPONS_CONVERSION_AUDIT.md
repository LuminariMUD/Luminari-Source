# Thrown Weapons Conversion Audit

- Audit date: 2026-08-23
- Source corpus: canonical active Realms of Luminari reference corpus
- Source records parsed: 71,680
- Scope: native type-14 objects, darts, blowguns, throwing quivers, and `MOB_ROL_ARCHER`
- Data policy: read-only source review; no production operation

## Summary

`rol_weapon_mapping.audit()` reports 1,319 melee weapons, 51 source ranged weapons, and 48 source
ammunition records. Five ammunition records become physical `ITEM_WEAPON` objects. All target
weapon and ammunition types are defined. The inference tiers are 94 declared, 25 fallback, 1,242
keyword, and 57 curated override decisions.

The local indexed native world in this feature worktree contains only `obj/1699.obj`. It has no
`ITEM_WEAPON` prototype whose `value[0]` is native weapon type 14, so there is no native blowgun
prototype to retag and no native dart prototype to change.

## Dart and blowgun records

The active source corpus contains three type-10 records and no type-16 record:

| Source VNUM | Source path | Source family | Result |
|-------------|-------------|---------------|--------|
| 91243 | `areas/obj/baldurs.obj` | range type 10 | `ITEM_WEAPON` / `WEAPON_TYPE_DART` |
| 52873 | `areas/obj/trollbark.obj` | range type 10 | `ITEM_WEAPON` / `WEAPON_TYPE_DART` |
| 94719 | `areas/obj/bloodtusk.obj` | missile type 10 | `ITEM_WEAPON` / `WEAPON_TYPE_DART` |

No source range type 16 or missile type 16 is authored in this corpus. Synthetic converter tests
therefore own the empty-corpus blowgun path: a type-16 launcher becomes append-only weapon type 80,
and type-16 ammunition remains `ITEM_MISSILE` / `AMMO_TYPE_DART`.

## Throwing quivers

All 44 source quivers now become `ITEM_AMMO_POUCH`: 24 declare archery kind 1 and 20 declare
throwing kind 2. Their source capacity is retained in `value[0]` as an object count, while the
source kind in `value[3]` is cleared because that target slot is the corpse flag.

| Source VNUM | Source path | Capacity | Identity |
|-------------|-------------|----------|----------|
| 50410 | `areas/obj/arndir.obj` | 120 | buffalo-skin quiver |
| 50465 | `areas/obj/arndir.obj` | 120 | buffalo-skin quiver |
| 91240 | `areas/obj/baldurs.obj` | 100 | black leather bandolier |
| 91242 | `areas/obj/baldurs.obj` | 150 | red velvet dart pouch |
| 94718 | `areas/obj/bloodtusk.obj` | 40 | tanned hide bandolier |
| 94720 | `areas/obj/bloodtusk.obj` | 45 | wolfhide dart pouch |
| 94535 | `areas/obj/darkhold.obj` | 100 | leather ammo belt |
| 43327 | `areas/obj/demi.obj` | 20 | leather pouch |
| 21724 | `areas/obj/dobluth.obj` | 100 | black leather ammo belt |
| 7971 | `areas/obj/gen-obj.obj` | 30 | heavy canvas sack |
| 21006 | `areas/obj/heartlnd.obj` | 6 | curious belt |
| 59204 | `areas/obj/hulburg.obj` | 30 | hard leather bandolier |
| 88909 | `areas/obj/longhollow.obj` | 6 | ragged skin sack |
| 20329 | `areas/obj/nhavan.obj` | 100 | mist-writhing pouch |
| 3114 | `areas/obj/northern_waterdeep.obj` | 100 | black-leather ammo belt |
| 20065 | `areas/obj/pirate.obj` | 30 | thick leather bandolier |
| 1014 | `areas/obj/quests.obj` | 30 | black-leather ammo belt |
| 34852 | `areas/obj/stalag.obj` | 200 | stretched-skin dagger belt |
| 20262 | `areas/obj/trahern.obj` | 50 | thorn quiver |
| 52882 | `areas/obj/trollbark.obj` | 40 | knotted vine |

## Archer loadout review

The corpus has 42 mobiles with source archer action 17, which maps to target compatibility flag
108. Every equipped weapon reset for all 42 was classified:

- 25 have at least one launcher and continue to use launcher mode: `7984`, `7985`, `16861`,
  `20093`, `20204`, `20205`, `20211`, `20270`, `20271`, `43788`, `49008`, `50505`, `52867`,
  `58809`, `63709`, `81924`, `81925`, `81926`, `83048`, `87018`, `94531`, `94532`, `94533`,
  `94534`, and `96809`.
- Four have a throwable anchor but no launcher and now select thrown mode:
  - mobile `7983`, large troll: object `7970`, light hammer;
  - mobile `52811`, troll lookout: object `52861`, dagger;
  - mobile `52824`, thornslinger: object `52873`, dart;
  - mobile `88904`, scout: object `7970`, light hammer.
- Twelve have no applicable weapon reset: `88`, `7986`, `19701`, `20281`, `50706`, `50707`,
  `51323`, `51324`, `51349`, `62718`, `81715`, and `81914`.
- Mobile `20969` has only a longsword reset. It remains melee-only.

The last two groups total the 13 inactive archer flags. Runtime selection is additionally gated on
`MOB_ROL_ARCHER`, so unrelated mobiles that happen to wield a dagger or another throwable do not
automatically enter thrown mode.

## Reproduction and release boundary

Two independent full Phase 7 regenerations were run from the established frozen converter inputs
and the read-only development baseline. Both produced run ID
`rol-phase7-b12-d98783e56e3b3f2a` with identical staged-tree identity. Each completed all 12 batches,
258 packages, and 71,680 record dispositions; generated 1,206 files; reported zero new active
errors; and passed the runtime-contract and preservation audits. The generated evidence is under
the ignored local run directories `thrown-weapons-phase7-a` and
`thrown-weapons-phase7-repeat`.

Spot checks in the assembled candidate confirmed target objects `2052873` and `2094719` as
`ITEM_WEAPON` / weapon type 14 and throwing-quiver targets such as `2050410`, `2052882`, and
`2094720` as `ITEM_AMMO_POUCH` with their source object-count capacities.

The full-corpus guard is:

```sh
PYTHONPATH=scripts/world python3 -m unittest \
  scripts.world.tests.test_rol_weapon_mapping -v
```

Run it with the ignored canonical reference corpus installed at
`EXAMPLE/RealmsOfLuminari`. The test asserts the three dart records, the empty-corpus blowgun path,
all 44 quivers, all 42 archer outcomes, numeric/header drift, and zero undefined types.

Applying regenerated data is intentionally a separate guarded Phase 8 operation. This worktree has
neither `lib/.env` nor `lib/mysql_config`, and its indexed world is the minimal artifact fixture.
Those are required safety inputs, not files to synthesize or copy. A Phase 8 bundle must be sealed,
backed up, applied, and verified in the established development checkout before any production
release is considered.
