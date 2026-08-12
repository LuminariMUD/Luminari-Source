# Realms of Luminari Project Changelog
**Previous Changelog entries can be found in changelog-archive/**

This file records completed milestones removed from the active
[canonical conversion plan](REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md), which
retains the forward-looking requirements, decisions, phases, and acceptance gates. The
superseded [feature-first plan](plan-archive/REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md),
[Phase 6.5 plan](plan-archive/PHASE6_5_CANONICAL_VNUM_REBASE_PLAN.md), and
[zone conversion scope](plan-archive/REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md) are
preserved in `plan-archive/`.

## 2026-08-13 - Phase 6 Tarrasque encounter

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Added the typed, builder-visible `RoL Tarrasque Encounter` procedure for source mobile 2601
  and objects 2604 and 2610. The encounter preserves periodic healing, pet execution, ordered
  swallow/tail-fling/tail-sweep combat, stomach acid, casting and preparation interruption,
  corpse entry, special death loot, and the return portal.
- Added a flow-bearing typed mobile-death event and invoked it from NPC death processing only
  when a registered procedure advertises the event. A successful handler suppresses the
  ordinary corpse and extracts the mobile after its replacement death behavior completes.
- Preserved the source weighted 6/6/6/2 loot distribution and adapted the source portal's old
  single-value destination to the target's normal portal schema. The return portal records the
  death-room VNUM in both target destination fields.
- Routed encounter damage, saves, acid resistance, stun eligibility, and random relocation
  through target-native safety contracts. The source random-teleport meaning is implemented by
  bounded valid-destination selection because the target `teleport` spell has different
  semantics.
- Added exact converter dispositions for `tarrasque_swallow_smack`, `tarrasque_die`,
  `tarrasque_stomache`, and `tarrasque_corpse_enter`. The converter supplies `MOB_SPEC` to the
  mobile and `ITEM_AUTOPROC` only to the stomach-acid object.
- Added registry, persistence, owner-aware OLC, event-contract, dispatch, loot-weight,
  corpse-alias, transformation, and reconciliation coverage. The production-linked suite now
  contains 663 tests and the world-tool suite contains 319 tests.
- Regenerated the corrected Phase 6 evidence. Resolution increases from 1,246 to 1,250 static
  bindings and from 538 to 542 direct handlers, leaving 471 bindings across 253 handlers in 35
  source files. The independent `ACT_SPEC` cross-check remains 798 resolved and 50 pending.
- Retained the 18-30-session Phase 6 envelope. This encounter-specific four-handler closure is
  the first corrected-denominator throughput sample; reforecast waits for at least three
  corrected batches.

### Acceptance evidence

```text
Delivery commit: PENDING_DELIVERY_COMMIT
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260813-tarrasque
Reconciliation run: rol-phase6-special-de980a28a3be846e
Active direct bindings: 1,721
Direct bindings resolved: 1,250
Direct bindings pending: 471
Source handlers resolved: 542
Source handlers pending: 253
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 761
Native adapted composable bindings: 165
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 319 passed
Production-linked CuTest suite: 663 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 1735ea1cadc25a1776aa09ec56f0ef3c6afde2e6
Installed SHA-256: e303a922907e55964b8cde3d596fccff509d6f9cdf809c6224c4c31377f6c89e
Evidence artifact hashes: 7 verified
Repeat reconciliation generation: byte-identical
Live target writes: 0
```

Phase 6 continues with a second source-local or behavior-shared batch from the remaining
corrected inventory. The estimate will be recalibrated after at least three such batches.

## 2026-08-13 - Phase 6 special discovery repair

Status: Completed checkpoint; corrected Phase 6 binding reconciliation in progress

### Delivered

- Replaced the direct-file-only special extractor with active boot call-path traversal through
  all 53 reachable registration wrappers. Every emitted row retains its wrapper path, source
  line, original VNUM token, and literal or preprocessor resolution evidence.
- Resolved all 38 active numeric planar macros automatically and added regression fixtures for
  direct, wrapped, symbolic, preprocessor-excluded, and dynamic registrations.
- Added explicit dynamic ledgers for `assign_the_questers()` and
  `assign_the_shopkeepers()`. Their 5,078 quest and 453 shop binding instances are resolved
  through the target data-driven HLQuest and shop services rather than misclassified as unknown
  numeric bindings.
- Regenerated the dependent Phase 1, Phase 2, Phase 5, and Phase 6 ignored evidence chain. The
  corrected static denominator is 1,721 live bindings across 795 direct handlers; 1,246 bindings
  and 538 handlers are resolved, leaving 475 bindings across 257 handlers.
- Recomputed the independent `ACT_SPEC` cross-check to 798 resolved and 50 pending records.
  Automatic race reconciliation remains complete for all 247 implicit bindings.
- Reforecast the remaining Phase 6 work at 18-30 sessions from the corrected pending inventory,
  its 36 source files, 190 singleton handlers, and 67 multi-binding handlers. The previous
  1-3-session estimate is retired.

### Acceptance evidence

```text
Delivery commit: c2a677a8
Phase 1 path: lib/rol-conversion/runs/phase1-policy2-20260813-special-discovery
Phase 1 run: rol-phase1-237602d3ade48138
Phase 2 path: lib/rol-conversion/runs/phase2-policy2-20260813-special-discovery
Phase 2 run: rol-phase2-c93b8c4610b36d1e
Phase 5 path: lib/rol-conversion/runs/phase5-policy2-20260813-special-discovery-audit
Phase 5 run: rol-phase5-audit-cec58661a4f21a2a
Phase 6 path: lib/rol-conversion/runs/phase6-special-20260813-discovery-repair
Phase 6 run: rol-phase6-special-df585be75f0574e3
Static binding candidates: 1,813
Preprocessor-excluded bindings: 92
Active static bindings: 1,721
Static bindings resolved: 1,246
Static bindings pending: 475
Direct source handlers: 795
Direct source handlers resolved: 538
Direct source handlers pending: 257
Dynamic registration paths: 2
Active dynamic binding instances: 5,531
Total active binding instances: 7,252
Total source handlers: 797
ACT_SPEC records resolved: 798
ACT_SPEC records pending: 50
Complete world-tool suite: 317 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 232dd588da93e455de32b496c2ed5a92efa08951
Installed SHA-256: 59f8510f6e1379df99e6e4fef44745397bdc735f8a512c6bed7c07a3bc5c4a3f
Evidence artifact hashes: 30 verified
Repeat Phase 6 generation: byte-identical
Live target writes: 0
```

Phase 6 continues with the dependency-complete Tarrasque encounter, then source-local and
behavior-shared batches from the corrected 475-binding pending inventory.

## 2026-08-12 - Phase 6 Waterdeep town crier

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted the active `crier_one` binding for northern Waterdeep mobile 2003008 through the
  existing builder-visible `RoL Scheduled Mobile` procedure and enforced `MOB_SPEC`.
- Preserved the standing-gated 2d42 ambient distribution, all 41 authored cases, ordered speech
  and room actions, zone shouts, and the deliberately silent 43-84 half of the roll range.
- Preserved both shared once-per-hour state gates and their source reset order: Moonshae and
  Calimport ship warnings, the hour-5 shop-opening warning, and the source-normal suppression of
  the hour-18 shop-closing warning until a reset or fresh load permits it.
- Preserved the source combat help shout and outdoor-only city cheering, plus the outdoor-only
  housewife response after the two welcome shouts. Target zone and indoor-room metadata replace
  the source descriptor filter without broadening the audience.
- Added converter, required-flag, schedule-boundary, exact-disposition, persistence, registry,
  OLC, plan, and manual-test coverage without adding a second named procedure.
- Reconciliation now resolves 1,112 of 1,147 active direct bindings and 538 of 562 source
  handlers; 35 bindings and 24 handlers remain. The independent `ACT_SPEC` checkpoint resolves
  830 records and leaves 18 pending.
- Archived the sixty-second Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 6c64fba1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-crier
Reconciliation run: rol-phase6-special-e0e90cdd3f12895e
Active direct bindings: 1,147
Direct bindings resolved: 1,112
Direct bindings pending: 35
Source handlers resolved: 538
Source handlers pending: 24
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 654
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 830
ACT_SPEC records pending: 18
Special registry definitions: 111 total / 98 legacy / 13 typed
Compatibility names: 112
Complete world-tool suite: 316 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: cbffe163e2dd9066d30e31e862938eecc9cc3438
Installed SHA-256: e8782120146413ce2153376982b1c0b31995a7e247bde0a310be08677b373bf7
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 35 direct bindings across 24 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 lich rite

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted both active `lichConverter` mobile bindings through the new builder-visible
  `RoL Lich Rite` procedure and enforced the required `MOB_SPEC` flag.
- Preserved the exact case-sensitive `say immortality` trigger, maximum-mortal level gate,
  Necromancer admission, complete source narrative, both keeper-held converted offerings, and
  consumption of the offerings and keeper on success.
- Repaired the source equipped-offering defect by retaining concrete pointers for worn or carried
  offerings 2089471 and 2046999 and validating both before either is consumed.
- Applied the target's established safety contract for irreversible race conversion: the player
  must be ungrouped, neither following nor leading, then becomes the target Lich race and is rebuilt
  as a Wizard with zero experience and -1000 alignment through the current respec engine.
- Added registry, compatibility, OLC inventory, converter flag, exact-phrase, eligibility,
  offering-location, and reconciliation coverage plus builder and manual-test documentation.
- Reconciliation now resolves 1,111 of 1,147 active direct bindings and 537 of 562 source
  handlers; 36 bindings and 25 handlers remain. The independent `ACT_SPEC` checkpoint resolves
  829 records and leaves 19 pending.
- Archived the sixty-first Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 7d28382f
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-lich-rite
Reconciliation run: rol-phase6-special-ded69599851e733e
Active direct bindings: 1,147
Direct bindings resolved: 1,111
Direct bindings pending: 36
Source handlers resolved: 537
Source handlers pending: 25
Additional handler families resolved: 1
Additional direct bindings resolved: 2
Native adapted bindings: 653
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 829
ACT_SPEC records pending: 19
Special registry definitions: 111 total / 98 legacy / 13 typed
Compatibility names: 112
Complete world-tool suite: 316 passed
Production-linked CuTest suite: 661 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 5c2cdc496d9dbd52d6b2be77ec1bea2294c7ed92
Installed SHA-256: 9fcff4b60681d2c4423b689aa8807c736ec9d9287741deb39e26314248e24ae2
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 36 direct bindings across 25 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 lost totem restorer

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted the active `lostTotemRestorer` binding through the new builder-visible
  `RoL Totem Restorer` mobile procedure in the existing converted-totem subsystem.
- Preserved the exact `say spiritworld` trigger, mapped source Shaman progression to the
  established target Cleric progression, retained the level-21 and saved-spirit-choice gates,
  and converted the source 1,000-platinum threshold to 10,000 target gold.
- Recreates the exact good or evil totem selected by persistent `GET_ROL_TOTEM_CHOICE`, binds
  it to the requesting character, and consumes the paid helper only after the object prototype
  has been validated and loaded. Invalid choices and missing prototypes fail without consuming
  the helper.
- Added converter `MOB_SPEC` enforcement, registry and compatibility coverage, eligibility and
  phrase regressions, reconciliation expectations, and manual testing instructions.
- Reconciliation now resolves 1,109 of 1,147 active direct bindings and 536 of 562 source
  handlers; 38 bindings and 26 handlers remain. The independent `ACT_SPEC` checkpoint remains
  at 828 resolved records and 20 pending because this direct binding has no source `ACT_SPEC`
  record.
- Archived the sixtieth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions.

### Acceptance evidence

```text
Delivery commit: 3f773d78
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-totem-restorer
Reconciliation run: rol-phase6-special-9139221a800d60a0
Active direct bindings: 1,147
Direct bindings resolved: 1,109
Direct bindings pending: 38
Source handlers resolved: 536
Source handlers pending: 26
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 651
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 828
ACT_SPEC records pending: 20
Special registry definitions: 110 total / 97 legacy / 13 typed
Compatibility names: 111
Complete world-tool suite: 315 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 9e49f4696e6076b93d55806cc3ef2172379f6552
Installed SHA-256: b437baa2005d7652d34f0703fd3d888f4907063475c0eff0b7fe78bbc60b0913
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 38 direct bindings across 26 source handlers. Continue
using dependency-complete batches and reserve the full build/test/install gate for published
checkpoints.

## 2026-08-12 - Phase 6 Menden fisherman

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted `menden_fisherman` through the existing source-hashed `RoL Source Periodic`
  gateway. No registry definition or storage schema changed.
- Preserved the awake gate, absence of fighting suppression, `number(1, 80)` selection,
  21 active outcomes, 40 generated actions, source message order, and exact room-visible text.
- Extended the periodic source parser, generated profile schema, and runtime to preserve targeted
  source socials. Named targets retain distinct room and victim messages, and `me` resolves to the
  acting mobile; the fisherman exercises wench, magus, and self targets across five actions.
- Preserved the source social-table boundary: `CMD_SIP` has no source action record and therefore
  contributes no room-visible action. Required `MOB_SPEC` activity is now enforced for persisted
  source-periodic bindings.
- Added generator, reconciliation, and production-linked profile regressions. The checked-in
  source-periodic and state-periodic tables remain reproducible from the assessed source tree.
- Reconciliation now resolves 1,108 of 1,147 active direct bindings and 535 of 562 source handlers;
  39 bindings and 27 handlers remain. The independent `ACT_SPEC` checkpoint resolves 828 records
  and leaves 20 pending.
- Archived the fifty-ninth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 33965efc
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-menden-fisherman
Reconciliation run: rol-phase6-special-aab827a742a51ca2
Active direct bindings: 1,147
Direct bindings resolved: 1,108
Direct bindings pending: 39
Source handlers resolved: 535
Source handlers pending: 27
Additional handler families resolved: 1
Additional direct bindings resolved: 1
Native adapted bindings: 650
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 828
ACT_SPEC records pending: 20
Special registry definitions: 109 total / 96 legacy / 13 typed
Compatibility names: 110
Complete world-tool suite: 315 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Installed ELF build ID: 271844a0d4ad97d0293cd1d6cd20668485efd214
Installed SHA-256: fb560d3b5560bdce62c81b4eb9004872a821d832161955c2adfd08240b341f68
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 39 direct bindings across 27 source handlers. Continue
grouping compatible irregular mechanics behind shared gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 scheduled-mobile batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted four scheduled-mobile handlers through one shared, builder-visible legacy `RoL
  Scheduled Mobile` gateway. No storage schema changed.
- Preserved the Waterdeep and Gloomhaven gate guards' distinct opening, repair, and closing
  windows, gate-state correction, speeches, glare, and ambient tables. The inactive 19-21 hour
  interval is retained because neither source handler performs a corrective transition there.
- Preserved the lighthouse keeper's shared counter and the source hour-eight reset behavior,
  including its repeated first line during that hour and its later staged announcements.
- Preserved the naval combatant's source standing-before-fighting branch order and idle table.
  Its reachable defensive helper maps to the source helper's actual stoneskin operation; the
  post-loop disarm remains excluded because the source victim loop makes it unreachable.
- Added converter dispositions, exact reconciliation expectations, required mobile activity bits,
  registry coverage, public behavior helpers, and production-linked regressions.
- Reconciliation now resolves 1,107 of 1,147 active direct bindings and 534 of 562 source handlers;
  40 bindings and 28 handlers remain. The independent `ACT_SPEC` checkpoint resolves 827 records
  and leaves 21 pending.
- Archived the fifty-eighth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: c8704d86
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-scheduled-mobiles
Reconciliation run: rol-phase6-special-c447dd6b4665cb7a
Active direct bindings: 1,147
Direct bindings resolved: 1,107
Direct bindings pending: 40
Source handlers resolved: 534
Source handlers pending: 28
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 649
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 827
ACT_SPEC records pending: 21
Special registry definitions: 109 total / 96 legacy / 13 typed
Compatibility names: 110
Complete world-tool suite: 314 passed
Production-linked CuTest suite: 660 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 40 direct bindings across 28 source handlers. Continue
grouping compatible irregular mechanics behind shared gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 utility-service batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted four utility-service handlers through typed `RoL Utility Object` and `RoL Utility
  Room` gateways. The room gateway adds one builder-visible registry and persistence name; the
  object gateway remains shared.
- Preserved the Black Plague reservoir's level gate, exact room ownership, drink/fill exposure,
  and disease immunity through target-native contagion. The source global plague toggle has no
  target equivalent and is recorded as an explicit compatibility boundary.
- Preserved the loot blocker's aggressive-NPC interception for room containers and non-player
  corpses while allowing carried containers and player corpses. Its exact 120-second corpse sweep
  maps the source 60-second decay request to one target MUD tick without adding persistent state.
- Preserved newbie-room east routing by converted source-race alignment and mapped the unavailable
  source birthplace model to the target saved load room with the mortal start as fallback.
- Preserved the weight trigger's 5,000-unit threshold, immortal exemption, transition state, and
  source messages. The source callback's door-effect branch is itself unimplemented.
- Added converter dispositions, exact reconciliation expectations, registry and profile coverage,
  item identification text, and production-linked behavior regressions.
- Reconciliation now resolves 1,103 of 1,147 active direct bindings and 530 of 562 source handlers;
  44 bindings and 32 handlers remain. The independent `ACT_SPEC` checkpoint remains 824 resolved /
  24 pending.
- Archived the fifty-seventh Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 9d40694b
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-utility-services
Reconciliation run: rol-phase6-special-2c12ac866ad07db2
Active direct bindings: 1,147
Direct bindings resolved: 1,103
Direct bindings pending: 44
Source handlers resolved: 530
Source handlers pending: 32
Additional handler families resolved: 4
Additional direct bindings resolved: 4
Native adapted bindings: 645
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 108 total / 95 legacy / 13 typed
Compatibility names: 109
Complete world-tool suite: 312 passed
Production-linked CuTest suite: 659 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 44 direct bindings across 32 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 object-service batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted five object-service handlers through the existing typed `RoL Utility Object` and
  `RoL Weapon Proc` gateways. No registry definition, persisted procedure name, or storage schema
  was added.
- Preserved Lathander's disc renewal, consumption, sleep, and stun behavior behind a new target
  `rub` gateway command. Preserved Llym's held-treasure offering, valuation, consumption, blessing,
  gold, summon, and object-reward paths with all Phase 2 identities verified by source kind.
- Preserved the smoke shield's one-in-ten punch stun and block discharge, including the source
  nonlethal guard and target-safe invalidation. Preserved the Crescent Moon's exact-case invocation
  and invisibility, and the Hellish Fury bow's ranged fire message and one-in-26 heavy fire proc.
- Mapped the source bow `FIREWEAPON` callback to the available successful ranged-hit gateway, the
  Crescent Moon's pulse object recharge to one actor combat-round wait, source vitality to target
  aid, and source coin varieties to the target's unified gold field.
- Classified `nuclear_bomb` as source-inert: its assigned initializer returns no event bits, so the
  destructive missile-hit body is unreachable through the active binding.
- Added converter dispositions, exact reconciliation expectations, utility registry-event coverage,
  object and weapon profile coverage, identify text, and the target command boundary.
- Reconciliation now resolves 1,099 of 1,147 active direct bindings and 526 of 562 source handlers;
  48 bindings and 36 handlers remain. The independent `ACT_SPEC` checkpoint remains 824 resolved /
  24 pending.
- Archived the fifty-sixth Phase 6 delivery session. The remaining Phase 6 envelope remains 1-3
  sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering hours.

### Acceptance evidence

```text
Delivery commit: 1849d9ad
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-object-services
Reconciliation run: rol-phase6-special-e50685fc20cfaf75
Active direct bindings: 1,147
Direct bindings resolved: 1,099
Direct bindings pending: 48
Source handlers resolved: 526
Source handlers pending: 36
Additional handler families resolved: 6
Additional direct bindings resolved: 6
Native adapted bindings: 641
Native adapted composable bindings: 159
Source-inert excluded bindings: 26
Source-unsafe excluded bindings: 18
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 658 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 48 direct bindings across 36 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 called-effect object batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted eight called-effect object handlers through the existing typed, object-owned
  `RoL Utility Object` gateway. Identity profiles keep all behavior behind the single existing
  registry and persistence name.
- Preserved exact, case-sensitive source phrases; worn-item validation; 24-, 48-, and 72-hour
  instance cooldowns; target-native spell effects; combat target validation; and the Staff of
  Magius `shirak` and `dulak` light toggle.
- Preserved the basilisk legging stoneskin, charmed basilisk-snake summon, Dragon Cult elemental
  protection, Earthmother random elemental aid, Tyr favor, Ashentoris combat aid, and haste
  sleeves. Summon ownership is complete before load triggers that may extract the mobile.
- Extended item identification for this gateway so each converted item describes its invocation
  and cooldown. No registry definition, persisted procedure name, or storage schema was added.
- Added converter dispositions, exact reconciliation expectations, profile coverage, registry
  event-contract coverage, and a production-linked light-toggle behavior regression.
- Reconciliation now resolves 1,093 of 1,147 active direct bindings and 520 of 562 source
  handlers; 54 bindings and 42 handlers remain. The independent `ACT_SPEC` checkpoint remains
  824 resolved / 24 pending.
- Archived the fifty-fifth Phase 6 delivery session. The remaining Phase 6 envelope remains
  1-3 sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused engineering
  hours.

### Acceptance evidence

```text
Delivery commit: c334a648
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-called-objects
Reconciliation run: rol-phase6-special-abe9fabc332abee0
Active direct bindings: 1,147
Direct bindings resolved: 1,093
Direct bindings pending: 54
Source handlers resolved: 520
Source handlers pending: 42
Additional handler families resolved: 8
Additional direct bindings resolved: 8
Native adapted bindings: 636
Native adapted composable bindings: 159
Source-inert excluded bindings: 25
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 658 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 54 direct bindings across 42 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.

## 2026-08-12 - Phase 6 residual mobile-procedure batch

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Converted seven residual mobile handlers across 13 active bindings through the existing
  typed, identity-profiled `RoL Monster Combat` gateway. The shared registry and persistence
  contract remain unchanged.
- Preserved delayed extraplanar vanishing, Beavis and Butthead social activity, Finn's idle
  and combat speech, faerie player selection and gold theft, six spell-cast interception
  assignments, and the ancient brownie's combat attack.
- Mapped destructive source behavior through the target's purge event, damage, spell,
  paralysis-immunity, and invalidation paths. This retains the intended mechanics without
  unsafe direct extraction, hit-point mutation, or stale-target use.
- Classified the active `clock_tower` object binding as source-inert. Its assigned direct
  callback returns no event bits during initialization, and the source tree contains no
  separate clock-tower event registration.
- Added both build-manifest entries, profile and converter coverage, explicit disposition
  tests, and production-linked profile inventory tests.
- Reconciliation now resolves 1,085 of 1,147 active direct bindings and 512 of 562 source
  handlers; 62 bindings and 50 handlers remain. The independent `ACT_SPEC` checkpoint is
  824 resolved / 24 pending.
- Archived the fifty-fourth Phase 6 delivery session. The remaining Phase 6 envelope remains
  1-3 sessions, leaving the Phases 6-8 forecast at 49-79 sessions, or 98-316 focused
  engineering hours.

### Acceptance evidence

```text
Delivery commit: 0c545b1c
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-residual-mobiles
Reconciliation run: rol-phase6-special-c60c0d2b988fd49f
Active direct bindings: 1,147
Direct bindings resolved: 1,085
Direct bindings pending: 62
Source handlers resolved: 512
Source handlers pending: 50
Additional handler families resolved: 8
Additional direct bindings resolved: 14
Native adapted bindings: 628
Native adapted composable bindings: 159
Source-inert excluded bindings: 25
ACT_SPEC records resolved: 824
ACT_SPEC records pending: 24
Special registry definitions: 107 total / 95 legacy / 12 typed
Compatibility names: 108
Complete world-tool suite: 311 passed
Production-linked CuTest suite: 657 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Warning-free Autotools build, test, and install: passed
Root-level circle artifact: absent
Evidence manifest hashes: verified
Live target writes: 0
```

Phase 6 continues with the remaining 62 direct bindings across 50 source handlers. Continue
grouping compatible irregular mechanics behind shared typed gateways, and preserve the full
build/test/install gate at each substantial commit boundary.
