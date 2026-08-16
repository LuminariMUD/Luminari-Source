# RoL Conversion Completed Work

Status: Completed-work record; active tasks are kept in the remaining-work plan

Last reviewed: 2026-08-16

## Purpose

This file records RoL conversion work that is already implemented and verified. It
does not contain future tasks. The unfinished project scope is maintained separately
in [ROL_CONVERSION_ACCURACY_RECOVERY_PLAN.md](ROL_CONVERSION_ACCURACY_RECOVERY_PLAN.md).

## Completed structural conversion

The first RoL conversion moved the selected source world into a separate Luminari
namespace. The accepted source inventory selected 258 packages and produced 71,680
record actions. The accepted Phase 8 overlay added 1,206 world-data files and replaced
the seven relevant world indexes.

The completed conversion established these safety properties:

- RoL zones use the reserved range 20000-29999.
- RoL rooms, mobiles, objects, and typed owners use 2000000-2999999.
- Existing Luminari identities are not aliases for similar RoL content.
- Generation occurs in a staging world before a development write.
- Release candidates are hash-guarded and development-only.
- Reapplying the same accepted bundle is a verified no-op.
- The superseded destructive Phase 6.5 rehome is disabled.
- The original overlay was additive and protected pre-existing Luminari records.

This work proved deterministic structure, namespace isolation, baseline-relative
validation, and safe additive application. It did not claim complete semantic
fidelity; that recovery remains in the active plan.

## Completed recovery ownership decision

No building or OLC editing will occur in the Realms of Luminari namespace until the
conversion is accurate, validated, and locked. During recovery, the frozen RoL source,
versioned conversion rules, authoritative calculator, and generated candidate are the
only sources of truth.

Therefore the currently installed converted RoL records are disposable generated
output. Reconversion will replace the complete proven RoL namespace; it will not
preserve current RoL edits, perform three-way merges, or create builder conflicts.
Safety work is limited to proving the RoL regeneration boundary, preserving every
non-RoL Luminari byte, validating removals and references, and providing exact backup
and rollback. A later decision can establish builder workflow only after the
conversion is locked.

## Completed mobile mechanics study

The mobile-scaling investigation traced the source and target loaders, the in-game
`autoroll_mob()` path, runtime combat modifiers, world-file encodings, OLC, and the
current converted corpus.

The study established that:

- old Luminari levels 31-34 implicitly combined competence and encounter strength;
- the old ladder supplied increasing HP, armor, damage, attack, extra-attack,
  critical-confirmation, and critical-defense-bypass pressure;
- five mutually exclusive encounter roles would not fit safely in the two remaining
  mobile action bits;
- encounter role therefore belongs in one persisted scalar field instead of five
  physical `MOB_*` flags;
- level should represent competence while tier represents intended encounter size;
- the copied RoL hitroll uses a different meaning from Luminari's inverse-encoded
  mobile-file field;
- RoL source levels above 34 had been collapsed to target level 34; and
- automatic-stat class treatment and the configurable class-category table disagree
  for at least Sorcerer and Bard, although current 100-percent settings hide it.

The indexed-corpus study found 5,617 of 12,406 active source mobiles above level 34.
It also measured the approximate median spawned HP gap that motivated the tier
compatibility seed:

| Target level | Native Luminari mobiles | Converted RoL mobiles |
|-------------:|------------------------:|----------------------:|
| 31 | 2,800 | 2,300 |
| 32 | 5,800 | 2,600 |
| 33 | 9,700 | 2,900 |
| 34 | 14,700 | 7,900 |

The same study matched native high-level prototype HP additions to the existing
automatic-stat formula:

| Target level | Exact native autoroll matches | Share of native mobiles |
|-------------:|------------------------------:|------------------------:|
| 31 | 313 of 359 | 87.2 percent |
| 32 | 127 of 164 | 77.4 percent |
| 33 | 546 of 672 | 81.2 percent |
| 34 | 405 of 577 | 70.2 percent |

These measurements were used as compatibility evidence, not as final encounter
balance approval.

## Completed race, subrace, and size trace

The mobile identity trace established that Luminari supports one broad NPC race
family plus exactly three NPC subrace slots. The slots are stored in
`mob_specials.subrace[MAX_SUBRACES]`, loaded from `SubRace 1:` through `SubRace 3:`,
and written back by GenOLC. The current RoL converter emits only the broad `Race:`
mapping and does not yet emit those three subrace fields.

The same trace established that `autoroll_mob()` is not a general size calculator.
It reads the current size only in the Giant race branch, where it tries to raise a
mobile smaller than Large to Large. Prototype and live-mobile calls do not handle
that mutation consistently because current size and real size are separate fields,
and the live path can overwrite the Giant assignment later in the function.

Accordingly, the remaining-work design makes automatically resolved source identity
authoritative: the converter selects up to three subraces, determines and holds final
size outside the calculator, and assigns that retained final size once after automatic
stats. No converted mobile requires per-mobile human classification.

## Completed source-to-target mobile field inventory

The field audit traced the physical RoL record grammar through the RoL loader, the
current Python parser and emitter, and Luminari's loader and GenOLC serializer. It
established that a normal RoL mobile record contains:

- one vnum and four tilde-terminated text fields;
- one action mask, two affect masks, alignment, and the `S` format marker;
- race code, base height, base weight, and an optional race-aggression list;
- level, direct hitroll, bounded armor, HP dice, and damage dice;
- either aggregate money or four coin denominations, followed by experience; and
- current position, default position, sex, optional class, optional magic resistance,
  and optional prestige bonus.

The audit also established that a syntactically valid target record does not prove
value fidelity. The current emitter ignores height, weight, the optional aggression
list, magic resistance, and prestige. It directly copies combat fields whose target
encodings differ, flattens currency without owning target custom-gold behavior, and
does not account for source loader changes to rewards and spawned stats.

The active-corpus inventory measured:

- 228 active mobile files: 227 declare source file version 1, while the empty
  `end.mob` sentinel declares version 0;
- 12,407 parsed mobile headers, of which 12,406 have all four expected numeric rows;
- 73 active race codes, 356 valid optional race-aggression lists, 65 positive authored
  heights, 100 positive authored weights, and only 61 records with both dimensions
  positive;
- 11,350 three-value final rows, 591 four-value rows, 344 five-value rows, and 121
  six-value rows; therefore 1,056 records author a class field, 465 author a
  magic-resistance field, and 121 author a prestige field;
- 286 positive effective explicit magic-resistance values, 108 positive prestige
  bonuses, and 13 negative prestige values that the source reward code ignores;
- five invalid sex values and four level-zero special or utility records; and
- one missing position row, two malformed race rows, and one money row with a trailing
  token ignored by the source loader.

All active action and affect bits have a current code-level disposition: mapped,
adapted, deferred to special reconciliation, or explicitly source-only. Their final
behavioral acceptance remains unfinished and is kept in the remaining-work plan.

The target-side inventory also covered every enhanced mobile field accepted by
`interpret_espec()` and emitted by GenOLC. It identified a persistence hole:
Luminari has runtime spell resistance and automatic stats can assign it, but mobile
records currently have no canonical enhanced field that loads and saves the value.
The required `SpellRes:` persistence work and every unresolved source-field
disposition are recorded in the active plan.

## Completed encounter-tier design

The implemented scalar values are:

| Value | Name | Encounter role |
|------:|------|----------------|
| 0 | `MOB_TIER_STANDARD` | Ordinary same-level mobile. |
| 1 | `MOB_TIER_ELITE` | Tough solo encounter. |
| 2 | `MOB_TIER_SMALL_GROUP` | Intended for roughly two or three players. |
| 3 | `MOB_TIER_BIG_GROUP` | Intended for roughly four to six players. |
| 4 | `MOB_TIER_RAID` | Intended for a larger coordinated group. |
| 5 | `MOB_TIER_WORLD_BOSS` | Individually designed boss with the Raid result as its generic floor. |

`MOB_TIER_FORMULA_V1` preserves the former high-level combat shape while separating
it from level. Let `B` be the positive ordinary automatic-stat HP budget and let `t`
be 1 for Elite, 2 for Small Group, 3 for Big Group, or 4 for Raid:

```text
H = (2 * t * B) + 500
repeat t times:
    H = H + floor(H / 10)
```

Standard uses `H = B`. World Boss uses the Raid result as its generic floor. The
implemented non-HP modifiers are:

```text
conditional_attack_bonus(t)   = t + 1 + max(0, t - 2)
armor_class_bonus(t)           = t + 1
damage_bonus(t)                = t
extra_max_bab_attacks(t)       = t
critical_confirmation_bonus(t) = 2 * t
defense_bypass_percent(t)      = 20 + (10 * t)
saving_throw_bonus(t)          = 0
spell_resistance_bonus(t)      = 0
```

The resulting compatibility rows are:

| Tier | HP calculation | Conditional attack | Armor | Damage | Extra attacks | Critical confirm | Defense bypass |
|------|----------------|-------------------:|------:|-------:|--------------:|-----------------:|---------------:|
| Standard | `B` | +0 | +0 | +0 | +0 | +0 | 0 percent |
| Elite | compound `(2B + 500)` once | +2 | +2 | +1 | +1 | +2 | 30 percent |
| Small Group | compound `(4B + 500)` twice | +3 | +3 | +2 | +2 | +4 | 40 percent |
| Big Group | compound `(6B + 500)` three times | +5 | +4 | +3 | +3 | +6 | 50 percent |
| Raid | compound `(8B + 500)` four times | +7 | +5 | +4 | +4 | +8 | 60 percent |
| World Boss | Raid floor | +7 floor | +5 floor | +4 floor | +4 floor | +8 floor | 60 percent floor |

For the level-34 warrior baseline `B = 1,496`, the exact Standard through Raid HP
vectors are 1,496, 3,841, 7,845, 12,611, and 18,252. For the level-34 arcane baseline
`B = 598`, the Elite through Raid vectors are 1,865, 3,499, 5,439, and 7,735.

## Completed encounter-tier implementation

### Data model and persistence

- `struct mob_special_data` contains a signed scalar tier field.
- `GET_MOB_TIER()` exposes the field to runtime and OLC code.
- Enhanced mobile records accept and emit `Tier: 0` through `Tier: 5`.
- Invalid explicit tier values stop the runtime loader.
- The Python world validator reports invalid and non-integer tier values.
- New MEDIT mobiles begin at Standard.
- Older records without `Tier:` use the signed `MOB_TIER_UNSPECIFIED` sentinel.

### Builder and inspection support

- MEDIT displays and edits one named, mutually exclusive encounter tier.
- `stat` displays the effective tier.
- Old omitted fields are visibly labeled `Legacy` instead of pretending they were
  explicitly classified.
- Builder documentation explains the intended player-count roles and the required
  race/subrace/size/class/level/tier autoroll order.

### Autoroll and runtime behavior

- `src/mob/mob_autoroll.c` contains the versioned tier calculations.
- Prototype autoroll stores exact generated HP as `1d1+(H-1)`.
- Tier armor and damage are applied to persisted autoroll results.
- Conditional attack bonus and its cap increase are derived from tier.
- Extra maximum-BAB attacks are derived from tier.
- Critical-confirmation bonus is derived from tier.
- Critical-defense bypass is derived from tier and uses an inclusive percentage
  boundary.
- Explicit tier results do not receive the former level-31-to-34 HP or armor bonuses
  a second time.

### Legacy compatibility

Records with no `Tier:` line temporarily derive Standard at level 30 or below and
Elite through Raid at levels 31 through 34. Their old spawn-time HP addition and
runtime armor behavior remain in place until explicit classification. Autorolling a
Legacy mobile resolves and stores its effective tier, after which the explicit tier
owns the modifiers exactly once.

This compatibility preserved existing high-level content during migration without
silently classifying every level-34 mobile as Raid.

### Build and test integration

- The new source and production-linked test are registered in both `Makefile.am` and
  `CMakeLists.txt`.
- Golden-vector tests cover warrior and arcane HP budgets.
- Modifier tests cover Standard through Raid and the World Boss Raid floor.
- Invalid tier, non-positive HP, null output, and overflow inputs are rejected.
- Legacy level fallback is tested.
- End-to-end autoroll tests cover every tier and prove exact HP serialization, armor,
  and damage application.
- Mobile parser tests cover valid and invalid `Tier:` records.
- The generated web mobile-flags guide was refreshed from its Markdown source.

## Completed verification

The completed tier slice passed these gates on 2026-08-16:

- warning-free GNU C23 production and CuTest builds;
- all 749 production-linked CuTests;
- all 420 world-tool tests;
- world documentation drift, ASCII, UTF-8, and LF checks;
- Python mobile parser tests;
- `git diff --check`;
- `make install`; and
- root-level `circle` artifact removal after installation.

The verified binary was installed through the repository's versioned installation
path. No production deployment or converted-world rewrite was performed.

## Authoritative implementation references

- `src/mob/mob_autoroll.c`
- `src/mob/mob_autoroll.h`
- `src/olc/medit.c`
- `src/olc/genmob.c`
- `src/db.c`
- `src/combat/fight.c`
- `src/utils.c`
- `unittests/CuTest/test_mob_autoroll.c`
- `scripts/world/wtool_lib/mobiles.py`
- `docs/world_game-data/builder_manual.md`
- `docs/world_game-data/MOB_FLAGS.md`
