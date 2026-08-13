# Realms of Luminari Canonical Plan Historical Baselines

- Archived: 2026-08-13
- Source: material removed from the active canonical conversion plan
- Status: historical evidence only; regenerate every denominator before use

The active
[canonical conversion plan](../REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md)
retains the current policy, work sequence, and acceptance gates. This file preserves
dated measurements and forecasts that explain earlier planning but must not control a
new conversion run.

## Raw source corpus

The complete source corpus measured about 45.9 MB and 1.73 million lines:

| Kind | Files | Bytes | Lines | Raw parsed records or structures |
|------|------:|------:|------:|--------------------------------:|
| `mob` | 260 | 6,448,241 | 200,972 | 13,505 mobiles |
| `obj` | 214 | 3,605,542 | 152,621 | 10,555 objects |
| `qst` | 261 | 2,390,960 | 81,688 | 5,081 blocks; 5,042 unique hosts |
| `shp` | 76 | 337,944 | 11,705 | 458 shops |
| `soc` | 91 | 703,602 | 32,554 | 1,758 lists; 4,284 actions |
| `wld` | 284 | 29,506,576 | 1,169,654 | 54,037 rooms; 54,019 unique VNUMs |
| `zon` | 284 | 2,931,956 | 84,134 | 287 records; 279 unique headers |
| **Total** | **1,470** | **45,924,821** | **1,733,328** | |

All 284 physical basenames had `wld` and `zon` files. Companion coverage was 258
`mob`, 210 `obj`, 255 `qst`, 76 `shp`, and 91 `soc` basenames, with a few additional
basenames in kind directories. These are raw-corpus counts, not active acceptance
denominators.

## Raw identity-collision scan

An unshifted raw-source scan against the then-current target found:

| Namespace | Unique source | Unique target | Same-number collisions |
|-----------|--------------:|--------------:|-----------------------:|
| Mobiles | 13,505 | 14,679 | 733 |
| Objects | 10,555 | 12,321 | 1,225 |
| Rooms | 54,019 | 54,390 | 17,191 |
| Zone headers | 279 | current zone table | 102 |

Most collisions were unrelated records. Only 8 mobile and 12 object collisions shared
a normalized primary name, and all 102 colliding zone headers had different normalized
titles. Source zone `0` was `God Rooms`; target zone `0` was `Builder Academy`.

Verified legacy-lineage examples were zone `507` to `1507`, mobile `50789` to
`150789`, zone `591` to `1591`, room `59433` to `159433`, zone `960` to `1960`, and
object `96001` to `196001`. They established content lineage, not final identity.

## Phase 6 discovery checkpoints

Raw source-special scans covered 80 `specs.*.c` files and about 89,167 lines. They
found at least 926 direct mobile, 282 direct object, and 354 direct room assignments,
plus helper registrations and about 2,769 numeric literals requiring type
classification. These were scale indicators, not completion denominators.

The repaired extractor followed 53 reachable registration wrappers and resolved all
38 active numeric VNUM macros. Its final Phase 6 inventory contained 1,813 static
candidates: 92 preprocessor-excluded and 1,721 active. Historical intermediate
checkpoints included 1,112 of 1,147 bindings, 538 of 562 handlers, and 830 of 848
`ACT_SPEC` rows. Corrected dependent runs were
`rol-phase1-237602d3ade48138`, `rol-phase2-c93b8c4610b36d1e`, and
`rol-phase5-audit-cec58661a4f21a2a`. Final evidence and every completed batch are in the
[Phase 6 archive](archive08_13-phase6-complete-RoL-Changelog.md).

## Dated Phase 6.5 policy-2 baseline

The 2026-08-12 Phase 2 policy-2 artifact contained 64,395 core identity rows and 1,988
noncanonical mappings:

| Group | Noncanonical rows | Legacy state measured at that checkpoint |
|-------|------------------:|------------------------------------------|
| Trail | 353 | `507 -> 1507`; 200 rooms, 151 mobiles, 1 object |
| Hulburg | 1,160 | `591 -> 1591`; 492 rooms, 400 mobiles, 267 objects |
| Jotunheim | 463 | `960 -> 1960`; 287 rooms, 89 mobiles, 86 objects |
| Modern artifacts | 11 | 11 source objects collapsed into `169901-169910` |
| Myth Drannor East | 1 | malformed header allocated to `20002` |
| **Total** | **1,988** | Phase 6.5 target: zero |

Six Jotunheim core records were already planned as canonical additions and were not in
the 463 rehome rows. The plan must preserve them during assembly.

The earlier five-package staged baseline contained 79 active validator findings, with
additional findings previously observed in legacy Trail zone `1507`. These counts are
historical; the enduring rule is that an owning batch repairs every selected or touched
finding and compares all other finding identities with its freshly recorded baseline.

An earlier dependency scan assigned 804 package-specific missing-reference gaps to
Phase 7. Regenerate that inventory after Phase 6.5 and treat the refreshed owned rows,
not 804, as the acceptance denominator.

## Raw grammar scale indicators

The full raw reset corpus contained 32,112 `M`, 4,535 `O`, 3,845 `P`, 4,288 intended
`G`, 13,172 `E`, 6,518 `D`, 684 `R`, 1,485 `F`, 422 `X`, and 275 `T` rows.

The raw SOC corpus contained 410 `LIST`, 33 `PATH`, 1,099 `PERIODIC`, 11 `TIMED`, and
205 `TRIGGER` headers; 1,758 mobile lists; 4,284 actions; and 144 numeric action codes.
About 2,518 actions used indoor, outdoor, all-zone, room, or path behavior, while 1,766
used old command-table indexes.

The raw text scan found 461,060 legacy `&+X` color tokens. All of these measurements
must be regenerated from the active inventory before they are used as coverage counts.

## Superseded estimates

Historical forecasts included 42-66 Phase 7 sessions, 6-10 Phase 8 sessions, 56-84
post-Phase-6 sessions, and 66-104 total remaining sessions. Earlier estimates included
49-79 and 74-114 sessions. They were planning envelopes, not delivery promises, and
were superseded by the requirement to reforecast from post-Phase-6.5 throughput and
remaining record actions.
