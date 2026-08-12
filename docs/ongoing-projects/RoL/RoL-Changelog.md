# Realms of Luminari Project Changelog
**Previous Changelog entries can be found in changelog-archive/**

This file records completed milestones removed from the active
[feature-first conversion plan](REALMS_OF_LUMINARI_FEATURE_FIRST_CONVERSION_PLAN.md)
and [zone conversion scope](REALMS_OF_LUMINARI_ZONE_CONVERSION_SCOPE.md). The plans
retain only forward-looking requirements, decisions, phases, and acceptance gates.

## 2026-08-12 - Phase 6 Bloodstone critter behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `bs_critter` bindings for converted Bloodstone mobiles
  2007110-2007112 and 2007141 through the persistent, mobile-owned
  `RoL Bloodstone Critter` procedure and required `MOB_SPEC` activity gateway.
- Preserved the source 2-in-81 ambient cadence: roll zero uses the current target
  `snarl` social, roll one uses `growl`, and all other outcomes are inert.
- Preserved the awake-and-idle boundary. Sleeping or fighting critters do not run the
  ambient behavior, and the normal loaded target socials provide current presentation.
- Registered the procedure and persistence/index contracts, taught the converter its
  canonical target name and required flag, and updated builder help, database-first
  help, manual testing, OLC inventories, converter fixtures, and characterization tests.
- Reconciliation now resolves 575 of 1,147 active direct bindings and 95 of 562 source
  handlers; 572 bindings and 467 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 563 resolved / 285 pending.
- Archived the twenty-second bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 26-58 Phase 6 sessions and 74-134 total sessions
  for Phases 6-8, or 148-536 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: b2e7bf40
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-critter
Reconciliation run: rol-phase6-special-bdf567929b95b5d7
Active direct bindings: 1,147
Direct bindings resolved: 575
Direct bindings pending: 572
Source handlers resolved: 95
Source handlers pending: 467
Bloodstone critter bindings resolved: 4
Native adapted bindings: 200
ACT_SPEC records resolved: 563
ACT_SPEC records pending: 285
Complete world-tool suite: 274 passed
Production-linked CuTest suite: 633 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 572 direct bindings across 467 source handlers.

## 2026-08-12 - Phase 6 Bloodstone undead death behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all four active `bs_undead_die` bindings for Bloodstone mobiles 7119, 7162,
  7167, and 7197 through the converter-owned `RoL-Black-Vapor-Death` mobile flag.
- Preserved composition with each mobile's other source procedure instead of consuming
  or replacing the target's single named SpecProc slot.
- Replaced Luminari's generic undead crumble presentation with the source black-vapor
  message while retaining the target's native no-corpse policy for converted undead.
- Extended the mobile-flag constants manifest, converter, runtime death hook, builder
  documentation, generated web guide, database-first help, manual test matrix, and
  production-linked characterization coverage.
- Reconciliation now resolves 571 of 1,147 active direct bindings and 94 of 562 source
  handlers; 576 bindings and 468 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 559 resolved / 289 pending because these composable bindings do not consume
  the named-procedure gateway.
- Archived the twenty-first bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 27-59 Phase 6 sessions and 75-135 total sessions
  for Phases 6-8, or 150-540 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 27b5ba59
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-vapor
Reconciliation run: rol-phase6-special-fffdce44e3e15b73
Active direct bindings: 1,147
Direct bindings resolved: 571
Direct bindings pending: 576
Source handlers resolved: 94
Source handlers pending: 468
Bloodstone undead death bindings resolved: 4
Native adapted composable bindings: 113
ACT_SPEC records resolved: 559
ACT_SPEC records pending: 289
Complete world-tool suite: 273 passed
Production-linked CuTest suite: 632 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 576 direct bindings across 468 source handlers.

## 2026-08-12 - Phase 6 converted Sister Knight reinforcements

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all five active `sister_knight` mobile bindings in the Moonshae package
  through the persistent, mobile-owned `RoL Sister Knight` procedure and the required
  `MOB_SPEC` activity and combat-turn gateways.
- Preserved the source zone-wide attack shout and bounded helper family. Awake, idle,
  reachable converted Sisters within 100 rooms in the same zone begin pursuing the
  caller's opponent; fighting, hunting, charmed, protected, unreachable, out-of-range,
  and cross-zone candidates remain ineligible.
- Preserved soundproof-room, silence, paralysis, sleep, and casting suppression. The
  source justice witness guard maps to the target's per-combat `PROC_FIRED` state so a
  caller shouts once during an encounter and resets after combat ends.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 567 of 1,147 active direct bindings and 93 of 562 source
  handlers; 580 bindings and 469 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 559 resolved / 289 pending.
- Archived the twentieth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 28-60 Phase 6 sessions and 76-136 total sessions
  for Phases 6-8, or 152-544 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 4fa18daf
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-sister-knight
Reconciliation run: rol-phase6-special-3aa909fd9793606b
Active direct bindings: 1,147
Direct bindings resolved: 567
Direct bindings pending: 580
Source handlers resolved: 93
Source handlers pending: 469
Sister Knight bindings resolved: 5
ACT_SPEC records resolved: 559
ACT_SPEC records pending: 289
Complete world-tool suite: 272 passed, 52 subtests passed
Production-linked CuTest suite: 632 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 580 direct bindings across 469 source handlers.

## 2026-08-12 - Phase 6 converted class-family guild rooms

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all ten active `guild_classtype_mage`, `guild_classtype_thief`,
  `guild_classtype_warrior`, and `guild_classtype_cleric` room bindings through four
  persistent, room-owned RoL class-family guild procedures.
- Preserved the source rule that a mismatched class family cannot use the guild while
  adapting its single-class check to the target multiclass model. Any matching class
  level admits the character; NPCs and unrelated commands retain ordinary current
  guild behavior.
- Delegated accepted `practice`, `train`, and `boosts` commands to Luminari's current
  training service instead of reviving the source's obsolete skill-practice and spell
  copying implementation.
- Registered the four procedures and their command contracts, taught the converter
  their canonical target names, and updated builder help, database-first help, manual
  testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 562 of 1,147 active direct bindings and 92 of 562 source
  handlers; 585 bindings and 470 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 554 resolved / 294 pending.
- Archived the nineteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 29-61 Phase 6 sessions and 77-137 total sessions
  for Phases 6-8, or 154-548 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: f4eefda6
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-class-guilds
Reconciliation run: rol-phase6-special-607369f4f87e0a4a
Active direct bindings: 1,147
Direct bindings resolved: 562
Direct bindings pending: 585
Source handlers resolved: 92
Source handlers pending: 470
Class-family guild bindings resolved: 10
ACT_SPEC records resolved: 554
ACT_SPEC records pending: 294
Complete world-tool suite: 271 passed, 52 subtests passed
Production-linked CuTest suite: 631 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 585 direct bindings across 470 source handlers.

## 2026-08-12 - Phase 6 converted lich energy drain

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all six active `lich_energy_drain` mobile bindings through the persistent,
  mobile-owned `RoL Lich Energy Drain` procedure and the required `MOB_SPEC` activity
  and combat-turn gateways.
- Preserved room-list targeting of the current opponent and its party, the independent
  one-in-five eligibility checks, the first-success rule, casting suppression, and the
  source callback's deliberate fall-through to ordinary NPC activity.
- Preserved the full-current-hit-point life transfer, Blackmantle healing suppression,
  over-maximum healing, and cumulative two-combat-round stun. Target Death Ward maps
  the unavailable source protection-from-undead spell and leaves the victim at zero
  rather than minus five hit points.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 552 of 1,147 active direct bindings and 88 of 562 source
  handlers; 595 bindings and 474 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 554 resolved / 294 pending.
- Archived the eighteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 30-62 Phase 6 sessions and 78-138 total sessions
  for Phases 6-8, or 156-552 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 72ba7c8e
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-lich-energy-drain
Reconciliation run: rol-phase6-special-8eaf63b5965118f6
Active direct bindings: 1,147
Direct bindings resolved: 552
Direct bindings pending: 595
Source handlers resolved: 88
Source handlers pending: 474
Lich-energy-drain bindings resolved: 6
ACT_SPEC records resolved: 554
ACT_SPEC records pending: 294
Complete world-tool suite: 270 passed, 52 subtests passed
Production-linked CuTest suite: 630 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 595 direct bindings across 474 source handlers.

## 2026-08-12 - Phase 6 converted trade bandits

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all seven active `bandit` mobile bindings through the persistent,
  mobile-owned `RoL Trade Bandit` procedure and the required `MOB_SPEC` command and
  combat-turn gateway.
- Preserved one-player capture, movement/flee/get interception, source demand variants,
  ordinary give-command processing, underpayment followed by attack, successful
  payment followed by disappearance, the repeat-attempt attack chance, and the lazy
  ten-MUD-hour expiry when the bandit is alone.
- Mapped source platinum demands to ten target gold per platinum and valued carried
  resources plus cargo in the player's owned wagon. The wagon-seizure variant uses the
  target `ITEM_WAGON` ownership model; a missing required wagon safely becomes an attack
  instead of reproducing the source null extraction defect.
- Registered the procedure and event contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 546 of 1,147 active direct bindings and 87 of 562 source
  handlers; 601 bindings and 475 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the seventeenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 31-63 Phase 6 sessions and 79-139 total sessions
  for Phases 6-8, or 158-556 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 7693ce00
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-bandit
Reconciliation run: rol-phase6-special-62a0531d50e3b71d
Active direct bindings: 1,147
Direct bindings resolved: 546
Direct bindings pending: 601
Source handlers resolved: 87
Source handlers pending: 475
Trade-bandit bindings resolved: 7
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 269 passed, 52 subtests passed
Production-linked CuTest suite: 629 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 601 direct bindings across 475 source handlers.

## 2026-08-12 - Phase 6 converted major beholders

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all eight active `major_beholder` mobile bindings through the persistent,
  mobile-owned `RoL Major Beholder` procedure and the required `MOB_SPEC` combat-turn
  gateway.
- Preserved all ten independent eye identities, the source one-in-three check for every
  ready eye, three-combat-turn per-eye cooldowns, multiple rays in one turn, room combat
  retargeting, and redirection from a selected charmed pet to its present master.
- Mapped the eye suite onto target-native fireball, acid arrow, slow, ray of
  enfeeblement plus feeblemind, wither, room-wide dispel, prismatic spray, hold monster,
  harm, and finger of death effects. The source-only all-unused-eyes weapon-critical
  burst is explicitly unavailable because the target special gateway does not expose a
  source weapon-critical event.
- Registered the procedure and combat contract, taught the converter its canonical
  target name and required action flag, and updated builder help, database-first help,
  manual testing, converter fixtures, registry persistence and OLC inventories, and
  production-linked characterization tests.
- Reconciliation now resolves 539 of 1,147 active direct bindings and 86 of 562 source
  handlers; 608 bindings and 476 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the sixteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 32-64 Phase 6 sessions and 80-140 total sessions
  for Phases 6-8, or 160-560 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 5536e463
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-major-beholder
Reconciliation run: rol-phase6-special-e2050f070b43faf9
Active direct bindings: 1,147
Direct bindings resolved: 539
Direct bindings pending: 608
Source handlers resolved: 86
Source handlers pending: 476
Major-beholder bindings resolved: 8
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 268 passed, 52 subtests passed
Production-linked CuTest suite: 628 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 608 direct bindings across 476 source handlers.

## 2026-08-12 - Phase 6 converted shaman totems

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 21 active `shaman_totem` object bindings and all 21 corresponding spirit
  death handlers through the persistent, object-owned `RoL Shaman Totem` procedure and
  the converter-owned `RoL-Totem-Spirit` mobile identity.
- Preserved permanent player/object bonding, good-versus-evil source-race admission,
  the mapped Cleric level-21 summon unlock, one active spirit, peaceful-room refusal,
  and three consumed attempts per seven MUD days. The missing source shaman skill maps
  to a documented Cleric-level and Wisdom progression that remains fallible until its
  bounded maximum.
- Preserved bounded companion scaling at ten Cleric levels below the summoner, target
  autorolling, the source 25-percent hit-point bonus, zero reward, charm/follower
  ownership, existing follower combat assistance, and all animal-specific corpse-free
  fade messages.
- Registered the persistent procedure and command/identify contracts, taught the
  converter all 21 target VNUM mappings, and updated both build manifests, constants,
  builder help, database-first help, manual testing, generated references, converter
  fixtures, registry inventories, and production-linked tests.
- Reconciliation now resolves 531 of 1,147 active direct bindings and 85 of 562 source
  handlers; 616 bindings and 477 handlers remain. The independent `ACT_SPEC` checkpoint
  remains 552 resolved / 296 pending.
- Archived the fifteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 33-65 Phase 6 sessions and 81-141 total sessions
  for Phases 6-8, or 162-564 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 8159562d
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shaman-totem
Reconciliation run: rol-phase6-special-5b3c2b758537ad3a
Active direct bindings: 1,147
Direct bindings resolved: 531
Direct bindings pending: 616
Source handlers resolved: 85
Source handlers pending: 477
Shaman-totem object bindings resolved: 21
Totem-spirit death handlers resolved: 21
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 267 passed, 52 subtests passed
Production-linked CuTest suite: 627 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed after applying the migration twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 616 direct bindings across 477 source handlers.

## 2026-08-12 - Phase 6 converted guild guards

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 47 active `guild_guard` mobile bindings through the persistent,
  mobile-owned `RoL Guild Guard` procedure.
- Reduced the source's 710-line global room switch to the 45 gate rules reached by
  active converted guards. The adapter covers 44 distinct converted load rooms, one
  two-direction entrance, class and race admission, unconditional barriers, and the
  source rule that a displaced guard no longer controls its entrance.
- Mapped the active source class families onto the target's multiclass model and mapped
  Grey Elf/Half-Elf admission onto target Elf/Half-Elf identities. Immortals and target
  town guards keep their movement bypasses.
- Preserved protection behavior in all 33 active protected-room rules: mortal attackers
  lose at most level times 5,000 experience without falling below two experience, take
  the source dispel/curse/poison/blind/slow suite, are reduced to one hit point, leave
  combat, and move to a safe random loaded room in the same zone.
- Registered the procedure with command and mobile-combat contracts, taught the
  converter its canonical target name, and made conversion add `MOB_SPEC`, which the
  target combat-turn gateway requires.
- Updated database-first help, builder and manual-test references, converter fixtures,
  registry persistence and OLC inventories, and production-linked tests. Reconciliation
  now resolves 489 of 1,147 active direct bindings and 63 of 562 handlers; 658 bindings
  and 499 handlers remain. The independent `ACT_SPEC` checkpoint advances to 552
  resolved / 296 pending.
- Archived the fourteenth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 34-66 Phase 6 sessions and 82-142 total sessions
  for Phases 6-8, or 164-568 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 7102d82d
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-guild-guard
Reconciliation run: rol-phase6-special-082fb35d02d05212
Active direct bindings: 1,147
Direct bindings resolved: 489
Direct bindings pending: 658
Source handlers resolved: 63
Source handlers pending: 499
Guild-guard bindings resolved: 47
ACT_SPEC records resolved: 552
ACT_SPEC records pending: 296
Complete world-tool suite: 265 passed
Production-linked CuTest suite: 626 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL checks: 4 passed
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 658 direct bindings across 499 source handlers.

## 2026-08-12 - Phase 6 converted ship system

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 57 active bindings for the shared RoL ship subsystem: seven hull objects,
  seven control panels, seven exit rooms, 29 lookout rooms, and seven navigators across
  Chionthar, Realms Master, Silver Lady, Gloom, Mirar, Captain's Fancy, and Spirit Raven.
- Added a bounded fixed-interior adapter rather than forcing the converted 37-room ships
  through the target Greyhawk system's 20-room interior limit. Boarding, capacity,
  instruments, speed, directional sailing and repeat movement, firing, ramming, ship
  docking, lookout, disembarking, hull damage, and sinking retain the source contracts.
- Preserved all seven two-way authored routes at the source 2.5-second cadence, including
  game-hour departure windows, navigator presence requirements, and departure, arrival,
  lost-route, and blocked-route announcements.
- Added navigator order protection and composition-safe combat-turn crew calls. Repaired
  the source Silver Lady helper list, which incorrectly named the Realms Master crew, to
  use the corresponding Silver Lady mobile family.
- Registered five persistent owner-aware procedures, taught the converter their canonical
  target names, and made the converter add `MOB_SPEC` to all seven navigators because the
  target combat-turn gateway requires it.
- Updated database-first help, builder and manual-test references, both build manifests,
  converter fixtures, registry persistence and OLC inventories, and production-linked
  tests. Reconciliation now resolves 442 of 1,147 active direct bindings and 62 of 562
  handlers; 705 bindings and 500 handlers remain. The independent `ACT_SPEC` checkpoint
  advances to 521 resolved / 327 pending.
- Archived the thirteenth bounded Phase 6 delivery session since the Phase 5 closeout. The
  forward-looking estimate is now 35-67 Phase 6 sessions and 83-143 total sessions for
  Phases 6-8, or 166-572 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 215c0f13
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-ships
Reconciliation run: rol-phase6-special-490933ef03fa1db0
Active direct bindings: 1,147
Direct bindings resolved: 442
Direct bindings pending: 705
Source handlers resolved: 62
Source handlers pending: 500
Ship-family bindings resolved: 57
ACT_SPEC records resolved: 521
ACT_SPEC records pending: 327
Complete world-tool suite: 264 passed
Production-linked CuTest suite: 625 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 705 direct bindings across 500 source handlers.

## 2026-08-12 - Phase 6 shadow-giant procedure

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all eight active `shadow_giant` mobile bindings in the `abandon` package.
  Reconciliation now resolves 385 of 1,147 active direct bindings and 57 of 562
  handlers; 762 bindings and 505 handlers remain.
- Added the mobile-owned `RoL Shadow Giant` procedure on the source periodic activity
  cadence. While fighting, its 1-in-21 trigger applies level-30 `spook` behavior to
  every player and charmed pet in the room: 25d8 mental damage, a Will save for half,
  and the source level-based chance for a one-to-three-round stun.
- Preserved the source immunity list for undead, dragons, demons, devils, and angels.
  Added the converter-owned `RoL-Angel` identity flag because source angels otherwise
  collapse into the target's broader outsider race and cannot be distinguished from
  other outsider pets.
- Repaired the source face-removal messages to use the actual fighting target rather
  than the source callback's null local pointer. Ordinary non-pet NPCs remain excluded.
- Updated the registry, owner-aware OLC, converter, constants manifest, database-first
  help, builder references, manual-test guidance, and production-linked tests. The
  independent `ACT_SPEC` checkpoint remains 517 resolved / 331 pending because none of
  these eight assignment-table consumers carried the source `ACT_SPEC` flag.
- Archived the twelfth bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 36-68 Phase 6 sessions and 84-144 total sessions
  for Phases 6-8, or 168-576 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 96785da1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shadow-giant
Reconciliation run: rol-phase6-special-989f5d0c8b5ac5c6
Active direct bindings: 1,147
Direct bindings resolved: 385
Direct bindings pending: 762
Source handlers resolved: 57
Source handlers pending: 505
Shadow-giant bindings resolved: 8
ACT_SPEC records resolved: 517
ACT_SPEC records pending: 331
Complete world-tool suite: 262 passed
Production-linked CuTest suite: 624 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 762 direct bindings across 505 source handlers.

## 2026-08-12 - Phase 6 source-preprocessor binding correction

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Corrected the Phase 6 denominator by running the source assignment table through the
  C preprocessor with RoL's checked-in configuration. Of 1,234 discovered candidates,
  87 are compiled out; the active inventory is 1,147 direct bindings across 562 source
  handlers.
- Added a separate immutable ledger for every preprocessor exclusion. It accounts for
  disabled jail and witness systems, `#if 0` item switches and experiments, inactive
  alternate branches, and five bindings previously counted as resolved.
- Updated Phase 1 discovery to apply the same preprocessor gate on future runs. This
  prevents disabled assignments from entering pilot selection or later corpus builds;
  all 91 bindings in the existing five-package pilot were independently confirmed
  active, so its staged output is unchanged.
- Recomputed composition and `ACT_SPEC` evidence from active assignments only. The
  direct ledger is 377 resolved / 770 pending, automatic-race composition is 22 beside
  direct procedures / 225 implicit-only, and `ACT_SPEC` is 517 resolved / 331 pending.
- Archived the eleventh bounded Phase 6 delivery session since the Phase 5 closeout.
  The forward-looking estimate is now 37-69 Phase 6 sessions and 85-145 total sessions
  for Phases 6-8, or 170-580 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: 47d12583
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-preprocessor
Reconciliation run: rol-phase6-special-bbb3db160a0636aa
Discovered direct-binding candidates: 1,234
Source-preprocessor exclusions: 87
Active direct bindings: 1,147
Direct bindings resolved: 377
Direct bindings pending: 770
Source handlers resolved: 56
Source handlers pending: 506
Automatic-race bindings resolved: 247
ACT_SPEC records resolved: 517
ACT_SPEC records pending: 331
Complete world-tool suite: 261 passed
Production-linked CuTest suite: 623 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build, test, and install: passed
Root-level circle artifact: absent
Existing pilot inactive bindings: 0 of 91
Live target writes: 0
```

Phase 6 continues with the remaining 770 direct bindings across 506 source handlers.

## 2026-08-12 - Phase 6 auto-distributor room procedure

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 22 active direct bindings for the shared `autoDistributor` room handler.
  Reconciliation now resolves 382 of 1,234 direct bindings and 56 of 605 handlers;
  852 bindings and 549 handlers remain.
- Added the room-owned `RoL Auto Distributor` named procedure. Any command from a
  non-staff character is intercepted and moves that character to a uniformly selected
  loaded room in the same zone; staff remain exempt and an empty or invalid zone stops
  safely with a diagnostic.
- Preserved the source callback's effective command behavior without adding a periodic
  scheduler. The source requested initialization pulses, but every pulse passed a null
  character and returned without action.
- Updated registry, owner-aware OLC, database-first help, converter, reconciliation, and
  runtime tests. The independent `ACT_SPEC` cross-check remains at 500 of 848 resolved
  because this family is room-owned.
- Archived the tenth bounded Phase 6 delivery session since the Phase 5 closeout. The
  forward-looking estimate is now 38-70 Phase 6 sessions and 86-146 total sessions for
  Phases 6-8, or 172-584 focused engineering hours at 2-4 hours per session.

### Acceptance evidence

```text
Delivery commit: ee096702
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-auto-distributor
Reconciliation run: rol-phase6-special-053b6c0d19db7fdc
Direct bindings resolved: 382
Direct bindings pending: 852
Source handlers resolved: 56
Source handlers pending: 549
Auto-distributor bindings resolved: 22
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 260 passed
Production-linked CuTest suite: 623 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 852 direct bindings across 549 source handlers.

## 2026-08-12 - Phase 6 magic-pool conversion

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 13 active direct bindings for the shared `magic_pool` object handler.
  Reconciliation now resolves 360 of 1,234 direct bindings and 55 of 605 handlers;
  874 bindings and 550 handlers remain.
- Added the object-owned `RoL Magic Pool` named procedure. Entering a matching pool
  preserves its authored fixed damage, transition messages, and destination transport;
  invalid destinations stop safely and identify the object in the server log.
- Extended native-binding metadata with explicit object-value references. The converter
  now remaps each pool's room destination in value 0 while retaining fixed damage in
  value 1; all 12 distinct active destinations have Phase 2 identity mappings.
- Updated registry, OLC, database-first help, converter, reconciliation, and runtime
  tests. The independent `ACT_SPEC` cross-check remains at 500 of 848 resolved because
  magic pools are object procedures rather than mobile flags.

### Acceptance evidence

```text
Delivery commit: 4c084ea1
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-magic-pool
Reconciliation run: rol-phase6-special-1b3f0ef7ec095814
Direct bindings resolved: 360
Direct bindings pending: 874
Source handlers resolved: 55
Source handlers pending: 550
Magic-pool bindings resolved: 13
Distinct destination identities resolved: 12 of 12
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 259 passed
Production-linked CuTest suite: 622 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build, test, and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 874 direct bindings across 550 source handlers.

## 2026-08-12 - Phase 6 home-reset room behavior

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed all 44 direct bindings for the shared `home_reset` room handler. The
  reconciliation now resolves 347 of 1,234 direct bindings and 54 of 605 handlers;
  887 bindings and 551 handlers remain.
- Added persistent `ROOM_ROL_HOME_RESET` conversion metadata and a composition-safe
  successful-movement hook. An NPC that walks out of a marked room now remembers the
  destination as its home without consuming the room's named SpecProc slot.
- Repaired the source callback's failed-movement edge case: blocked, invalid, or
  trigger-rejected movement no longer changes the NPC's home before it has moved.
- Extended room emission, constants extraction, generated builder documentation, and
  reconciliation evidence. The independent `ACT_SPEC` cross-check remains at 500 of
  848 resolved because `home_reset` is a room procedure rather than a mobile flag.

### Acceptance evidence

```text
Delivery commit: 2849e0a7
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-home-reset
Reconciliation run: rol-phase6-special-1d2f58fe08372b1e
Direct bindings resolved: 347
Direct bindings pending: 887
Source handlers resolved: 54
Source handlers pending: 551
Home-reset bindings resolved: 44
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 258 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Constants and generated-guide drift checks: passed
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 887 direct bindings across 551 source handlers.

## 2026-08-12 - Phase 6 shared combat and conjured-death procedures

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Closed 72 direct bindings across ten reusable source handlers. The reconciliation
  now resolves 303 of 1,234 direct bindings and 53 of 605 handlers; 931 bindings and
  552 handlers remain.
- Registered seven named mobile combat procedures: acid and lightning single-target
  breath attacks plus fire, cold, acid, gas, and lightning room-wide breath weapons.
  Each preserves the source four-turn cadence; attack variants use half-level damage,
  while weapon variants use the target's corresponding full-level area spell.
- Added three persistent, composition-safe conjured-death flags for 25 familiars, ten
  mounts, and ten summoned monsters. The flags preserve each source fade message and
  suppress corpse creation without consuming the mobile's named SpecProc slot.
- Extended converter emission and preserved-mobile patching to merge the new action
  flags with existing flags, automatic-race behavior, trigger attachments, and named
  procedures. The reconciler records these callbacks as
  `NATIVE_ADAPTED_COMPOSABLE` rather than inventing a second persisted SpecProc slot.
- Updated the owner-aware registry, builder documentation, generated web guide,
  constants manifest, and database-first help source. The independent `ACT_SPEC`
  cross-check now resolves 500 of 848 records and leaves 348 pending.

### Acceptance evidence

```text
Delivery commit: d447a10b
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shared-combat-death
Reconciliation run: rol-phase6-special-dd7798f8ea4681cf
Direct bindings resolved: 303
Direct bindings pending: 931
Source handlers resolved: 53
Source handlers pending: 552
Conjured-death bindings resolved: 45
Breath-procedure bindings resolved: 27
ACT_SPEC records resolved: 500
ACT_SPEC records pending: 348
Complete world-tool suite: 257 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed twice
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 continues with the remaining 931 direct bindings across 552 source handlers.

## 2026-08-12 - Phase 6 automatic race procedures

Status: Completed checkpoint; Phase 6 direct-binding reconciliation in progress

### Delivered

- Implemented all 247 active boot-time race procedures independently of the ordinary
  persisted special-procedure slot: 134 demons, 101 devils, and 12 umber hulks. This
  preserves composition for the 23 prototypes that also have a direct assignment.
- Added persistent `RoL-Demon`, `RoL-Devil`, and `RoL-Umberhulk` mobile flags and
  converter-owned initialization affects. Demon and devil prototypes receive
  infravision and aggregate elemental protection; umber hulks receive aggregate
  elemental protection.
- Added composition-safe activity and combat hooks. Recognized demons and devils use
  source-derived gate chances, cooldowns, follower rules, control restrictions,
  `nogate` templates, temporary followers, combat joining, and four-game-hour expiry.
  Complex source branches that could create several independent groups are represented
  by one bounded mixed group of at most five creatures.
- Added the umber-hulk claw initialization, level-scaled combat chance, confusion gaze,
  and extra mandible attack. The canonical claw prototype is found by its exact source
  alias rather than a hard-coded converted VNUM.
- Extended preserved-mobile staging so all four action-flag and all four affect-flag
  chunks can be merged without clobbering existing flags, named procedures, trigger
  attachments, or local content. This closes the eight automatic-race records whose
  record action is `KEEP`; the refreshed Hulburg pilot proves six of those patches.
- Reconciled all 247 implicit bindings to `NATIVE_ADAPTED_COMPOSABLE`. The independent
  `ACT_SPEC` cross-check now resolves 495 of 848 records and leaves 353 records, all of
  which are tied to still-pending direct assignments.
- Documented the new builder-visible flags and automatic behavior in the mobile flag,
  OLC special-procedure, generated web, and database-first help sources.

### Acceptance evidence

```text
Delivery commits: b58faaea, e5b81b14, 2b55b265
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-automatic-race
Reconciliation run: rol-phase6-special-519936c88c94c0da
KEEP-patch stage: lib/rol-conversion/runs/phase6-special-20260812-race-keep-stage
KEEP-patch run: rol-phase4-build-174249e9cd9cc337
Active implicit race bindings: 247
Implicit race bindings resolved: 247
Implicit race bindings pending: 0
Preserved automatic-race prototypes: 8
Refreshed pilot preserved-mobile patches: 79
Direct bindings resolved: 231
Direct bindings pending: 1,003
ACT_SPEC records resolved: 495
ACT_SPEC records pending: 353
Complete world-tool suite: 256 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Help SQL idempotency checks: 4 passed
Autotools build and install: passed
Root-level circle artifact: absent
Staged new active errors: 0
Reset observations: passed
Walkthroughs: passed
Live target writes: 0
```

Phase 6 continues with the remaining 1,003 direct bindings across 562 source handlers.

## 2026-08-12 - Phase 6 implicit race-binding correction

Status: Completed evidence checkpoint; Phase 6 runtime reconciliation in progress

### Delivered

- Corrected the Phase 6 evidence model to inventory source boot-time race procedures
  independently of `ACT_SPEC` and direct assignment tables. The active corpus has 247
  implicit bindings: 134 `standardDemon`, 101 `standardDevil`, and 12
  `standardUmberhulk` bindings.
- Proved that 23 implicit race bindings coexist with direct mobile assignments; the
  other 224 are implicit-only. The source supports both callbacks on one prototype, so
  the target port must use a composition-safe runtime path rather than consuming or
  replacing the target's single persistent special-procedure slot.
- Added a deterministic `automatic-race-ledger.jsonl` artifact with source definition
  hashes, record and destination identities, direct-binding relationships, dispositions,
  and explicit pending status for all 247 bindings.
- Corrected the `ACT_SPEC` cross-check without changing its total status counts. Of the
  386 pending records, 343 have direct assignments only, 10 combine direct and implicit
  race procedures, and 33 expose an implicit race procedure without a direct assignment.
  The remaining 214 implicit race procedures are active despite lacking `ACT_SPEC` in
  authored source data.
- Bumped the reconciliation evidence schema to version 2 and added production-corpus
  regression coverage for the race, composition, definition, and artifact counts.

### Acceptance evidence

```text
Delivery commit: ae867c47
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-race-composition
Reconciliation run: rol-phase6-special-caf72346b7ac8119
Active direct bindings: 1,234
Active implicit race bindings: 247
Implicit bindings alongside direct assignments: 23
Implicit-only bindings: 224
Implicit race handlers located: 3 of 3
ACT_SPEC records resolved: 462
ACT_SPEC records pending: 386
Complete world-tool suite: 254 passed
Live target writes: 0
```

The earlier Phase 6 checkpoints' count of 33 automatic procedures described only the
pending `ACT_SPEC` subset. This checkpoint supersedes that count for full active-binding
coverage; their direct-binding and handler counts remain valid.

## 2026-08-12 - Phase 6 shared mobile procedures

Status: Completed checkpoint; Phase 6 special-procedure reconciliation in progress

### Delivered

- Traced and closed 46 direct bindings across four reusable mobile families. The
  reconciliation now resolves 231 of 1,234 direct bindings and 43 of 605 handlers;
  1,003 bindings and 562 handlers remain.
- Classified all 17 source `cityguard` bindings as source-inert. The callback returns
  before its disabled aggression code, so conversion emits neither an active target
  city guard nor `MOB_SPEC`. Mapping it to the target's active city guard would have
  invented behavior.
- Added a shared RoL corpse-devourer adapter for 11 bindings. It consumes food and
  non-player corpses, preserves player corpses, and spills corpse contents before
  extraction as the source does.
- Added a shared RoL poison-bite adapter for 15 bindings. It preserves the source
  level-scaled `1 / (62 - level)` proc chance rather than using the target snake's
  different `1 / (level + 1)` chance.
- Added a shared RoL thief adapter for three bindings. It attempts the source theft
  path for every eligible mortal player on each activity call, respects peaceful
  rooms, and uses the converted target gold economy.
- Registered all three adapters as owner-aware, world-persistable mobile procedures,
  added them to both supported build manifests, and documented the expanded registry.
  The `ACT_SPEC` cross-check now resolves 462 of 848 records and leaves 386 pending.

### Acceptance evidence

```text
Delivery commit: 960f5602
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-shared-mobile
Reconciliation run: rol-phase6-special-0f4f1274d95a2941
Active direct bindings: 1,234
Direct bindings resolved: 231
Direct bindings pending: 1,003
Distinct source handlers: 605
Source handlers resolved: 43
Source handlers pending: 562
ACT_SPEC records resolved: 462
ACT_SPEC records pending: 386
Complete world-tool suite: 254 passed
Production-linked CuTest suite: 620 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

The next pass continues with automatic demon/devil race procedures and the next
high-reuse shared families.

## 2026-08-12 - Phase 6 inventory and shared-service reconciliation

Status: Completed checkpoint; Phase 6 special-procedure reconciliation in progress

### Delivered

- Added a deterministic Phase 6 reconciliation bundle that verifies its Phase 1,
  Phase 2, and Phase 5 inputs before accounting for all 1,234 active direct bindings,
  all 605 distinct source handlers, and all 848 active `ACT_SPEC` records. Every
  source handler definition is located and hashed; the bundle performs zero live
  target writes.
- Separated direct assignment-table truth from the mobile `ACT_SPEC` cross-check. Of
  the 848 flagged mobiles, source boot clears 444 because no callback is assigned,
  33 receive automatic demon or devil race procedures and remain Phase 6 work, and
  371 have direct assignment-table bindings.
- Closed 185 direct bindings: 108 native or DG bindings established by the Phase 4
  families, 72 shared guild, janitor, pet-shop, or receptionist bindings, and five
  source `dump` bindings whose callback returns before its unreachable behavior.
  Binding the target's active Dump procedure would have invented behavior.
- Added persistent named room-binding output to the converter and registered
  `RoL Guild Room` as a room-owned canonical procedure backed by the current target
  training service. The mobile-owned `Guild` contract remains unchanged.
- Updated the builder guide and database-first `SPECIALS` help source with the
  current registry counts, owner-aware selection rules, and the distinction between
  `Guild` and `RoL Guild Room`.
- Removed the completed Phase 6 startup instruction from the active zone scope. The
  active plans retain only the remaining 1,049 direct bindings, 566 source handlers,
  360 directly assigned `ACT_SPEC` mobiles, 33 automatic race procedures, and later
  phase gates.

### Acceptance evidence

```text
Delivery commit: 368adc90
Reconciliation path: lib/rol-conversion/runs/phase6-special-20260812-inventory-v3
Reconciliation run: rol-phase6-special-7e0556903754990d
Active direct bindings: 1,234
Direct bindings resolved: 185
Direct bindings pending: 1,049
Distinct source handlers: 605
Source handler definitions located: 605
Source handlers resolved: 39
Source handlers pending: 566
ACT_SPEC records: 848
ACT_SPEC records resolved: 455
ACT_SPEC records pending: 393
Complete world-tool suite: 253 passed
Production-linked CuTest suite: 619 passed
Documentation findings: 0 errors, 0 warnings, 0 info
Autotools build and install: passed
Root-level circle artifact: absent
Live target writes: 0
```

Phase 6 is not complete. The next pass continues with reusable generic mobile
families and the automatic race assignments before moving into consuming-package
specific procedures.
