# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-12
- Environment: development
- Branch: `master`
- Current task: Phase 6 special-procedure reconciliation
- Completed milestone record: [RoL-Changelog.md](RoL-Changelog.md)
- Phase 4 manual test matrix: [PHASE4_MANUAL_TESTING.md](PHASE4_MANUAL_TESTING.md)

## Current committed checkpoint

Phases 0-5 and the listed Phase 6 checkpoints are implemented, committed, and pushed:

```text
Phase 0 baseline commit: 1619ccd8
Phase 0 checkpoint commit: 57f29005
Phase 1 grammar commit: e6101445
Phase 1/2 discovery and planning commit: 0c3753ee
Phase 3 walking-skeleton commit: a5419818
Phase 4 selection commit: fe523532
Phase 4 selection checkpoint commit: a0b54009
Phase 4 payload preservation commit: 0e869fda
Phase 4 record transform commit: 3a6b7602
Phase 4 reset compatibility commit: 72c2ef24
Phase 4 shop conversion commit: 10a30cf2
Phase 4 quest conversion commit: d5609b1c
Phase 4 SOC compiler commit: 56393f8c
Phase 4 native special commit: 8136c71b
Phase 4 special archive commit: aeaf79d2
Phase 4 complete pilot build commit: 694cf84f
Phase 4 runtime validation commit: 3fe7015f
Phase 5 quest/SOC compatibility commit: 7bbfba3d
Phase 5 rare room compatibility commit: e100bdff
Phase 5 object-trap compatibility commit: 6a1ddb6d
Phase 5 full-corpus audit commit: 51b7bd13
Phase 5 room/zone/affect compatibility commit: f0fb9d8f
Phase 5 object-property compatibility commit: ca037b15
Phase 5 object-apply/affect compatibility commit: f7eabca4
Phase 5 mobile-action compatibility commit: 1f6020de
Phase 5 reset mobile-chain compatibility commit: afeea9d7
Phase 5 exit-trap compatibility commit: c647c5f4
Phase 5 shop compatibility commits: ec1a8cd8, fe38a56e
Phase 6 inventory/shared-service commit: 368adc90
Phase 6 shared-mobile commit: 960f5602
Phase 6 implicit-race evidence commit: ae867c47
Phase 6 automatic-race metadata commit: b58faaea
Phase 6 automatic-race runtime commit: e5b81b14
Phase 6 preserved-race patch commit: 2b55b265
Phase 6 shared combat/conjured-death commit: d447a10b
Phase 6 home-reset compatibility commit: 2849e0a7
Phase 6 magic-pool conversion commit: 4c084ea1
Phase 6 auto-distributor conversion commit: ee096702
Phase 6 source-preprocessor correction commit: 47d12583
Phase 6 shadow-giant conversion commit: 96785da1
Phase 6 converted-ship system commit: 215c0f13
Phase 6 converted-guild-guard commit: 7102d82d
Phase 6 converted-shaman-totem commit: 8159562d
Phase 6 converted-major-beholder commit: 5536e463
Phase 6 converted-trade-bandit commit: 7693ce00
Phase 6 converted-lich-energy-drain commit: 72ba7c8e
Phase 6 converted-class-family-guild commit: f4eefda6
Phase 6 converted-Sister-Knight commit: 4fa18daf
Phase 6 Bloodstone-undead-death commit: 27b5ba59
Phase 6 Bloodstone-critter commit: b2e7bf40
Phase 6 directional-item-blocker commit: acb858c1
Phase 6 designated-follower commit: 9de8ed76
Phase 6 floating-pool commit: e3600f16
Phase 6 Bloodstone-portal commit: 2635c21c
Phase 6 Waterdeep-guild-room commit: 08fcf107
Phase 6 batched combat/death commit: 03111649
```

The authoritative ignored runs are:

```text
Phase 0: lib/rol-conversion/runs/phase0-1619ccd8
         rol-phase0-02a84b2da28503c1
Phase 1: lib/rol-conversion/runs/phase1-e6ea7982
         rol-phase1-1c287d5073293f7c
Phase 2: lib/rol-conversion/runs/phase2-e6ea7982
         rol-phase2-39a6d6d253950dff
Phase 3: lib/rol-conversion/runs/phase3-a5419818-a
         lib/rol-conversion/runs/phase3-a5419818-b
         rol-phase3-11336f1832d8765c
Phase 4 selection: lib/rol-conversion/runs/phase4-select-e6ea7982
                   rol-phase4-select-6f7ae16e5df665ec
Phase 4 build: lib/rol-conversion/runs/phase4-build-e6ea7982
               rol-phase4-build-a2c341dfaa743b26
Phase 5 room/zone/affect audit: lib/rol-conversion/runs/phase5-room-zone-df35cb1f
                                rol-phase5-audit-719a67acc4cb6b01
Phase 5 room/zone/affect pilot: lib/rol-conversion/runs/phase5-room-flags-f0fb9d8f
                                rol-phase4-build-e5d61111edbfcd9d
Phase 5 object-property audit: lib/rol-conversion/runs/phase5-object-flags-e5b998bd
                                 rol-phase5-audit-fb713f798161a78b
Phase 5 object-property pilot: lib/rol-conversion/runs/phase5-object-flags-e5b998bd-pilot
                                 rol-phase4-build-4e6f5f9a132e06cc
Phase 5 object-apply/affect audit: lib/rol-conversion/runs/phase5-object-applies-affects-20260812
                                    rol-phase5-audit-ae0fdf51ee3707ee
Phase 5 object-apply/affect pilot: lib/rol-conversion/runs/phase5-object-applies-affects-20260812-pilot
                                    rol-phase4-build-87b9c7b1b8e214bd
Phase 5 mobile-action audit: lib/rol-conversion/runs/phase5-mobile-actions-20260812-v2
                               rol-phase5-audit-fc1c1ddc402d3800
Phase 5 mobile-action pilot: lib/rol-conversion/runs/phase5-mobile-actions-20260812-pilot
                               rol-phase4-build-f11ba7e2f3909645
Phase 5 reset-chain audit: lib/rol-conversion/runs/phase5-reset-chain-20260812-audit
                           rol-phase5-audit-ed84cba825215e4f
Phase 5 reset-chain pilot: lib/rol-conversion/runs/phase5-reset-chain-20260812-pilot
                           rol-phase4-build-0036becbb939e3ad
Phase 5 exit-trap audit: lib/rol-conversion/runs/phase5-exit-traps-20260812-audit
                         rol-phase5-audit-c6c7050ef434f7b8
Phase 5 exit-trap pilot: lib/rol-conversion/runs/phase5-exit-traps-20260812-pilot
                         rol-phase4-build-7e8fa263dff52098
Phase 5 shop/final audit: lib/rol-conversion/runs/phase5-shop-20260812-audit
                           rol-phase5-audit-3fb8de2d9afd067b
Phase 5 shop/final pilot: lib/rol-conversion/runs/phase5-shop-20260812-pilot
                           rol-phase4-build-35c9c879af63b8d1
Phase 6 special reconciliation: lib/rol-conversion/runs/phase6-special-20260812-inventory-v3
                                rol-phase6-special-7e0556903754990d
Phase 6 shared mobile: lib/rol-conversion/runs/phase6-special-20260812-shared-mobile
                       rol-phase6-special-0f4f1274d95a2941
Phase 6 implicit race: lib/rol-conversion/runs/phase6-special-20260812-race-composition
                       rol-phase6-special-caf72346b7ac8119
Phase 6 automatic race: lib/rol-conversion/runs/phase6-special-20260812-automatic-race
                         rol-phase6-special-519936c88c94c0da
Phase 6 race KEEP stage: lib/rol-conversion/runs/phase6-special-20260812-race-keep-stage
                         rol-phase4-build-174249e9cd9cc337
Phase 6 shared combat/death: lib/rol-conversion/runs/phase6-special-20260812-shared-combat-death
                             rol-phase6-special-dd7798f8ea4681cf
Phase 6 home reset: lib/rol-conversion/runs/phase6-special-20260812-home-reset
                    rol-phase6-special-1d2f58fe08372b1e
Phase 6 magic pool: lib/rol-conversion/runs/phase6-special-20260812-magic-pool
                    rol-phase6-special-1b3f0ef7ec095814
Phase 6 auto distributor: lib/rol-conversion/runs/phase6-special-20260812-auto-distributor
                          rol-phase6-special-053b6c0d19db7fdc
Phase 6 source preprocessor: lib/rol-conversion/runs/phase6-special-20260812-preprocessor
                             rol-phase6-special-bbb3db160a0636aa
Phase 6 shadow giant: lib/rol-conversion/runs/phase6-special-20260812-shadow-giant
                      rol-phase6-special-989f5d0c8b5ac5c6
Phase 6 converted ships: lib/rol-conversion/runs/phase6-special-20260812-ships
                         rol-phase6-special-490933ef03fa1db0
Phase 6 converted guild guards: lib/rol-conversion/runs/phase6-special-20260812-guild-guard
                                 rol-phase6-special-082fb35d02d05212
Phase 6 converted shaman totems: lib/rol-conversion/runs/phase6-special-20260812-shaman-totem
                                 rol-phase6-special-5b3c2b758537ad3a
Phase 6 converted major beholders: lib/rol-conversion/runs/phase6-special-20260812-major-beholder
                                   rol-phase6-special-e2050f070b43faf9
Phase 6 converted trade bandits: lib/rol-conversion/runs/phase6-special-20260812-bandit
                                 rol-phase6-special-62a0531d50e3b71d
Phase 6 converted lich energy drain: lib/rol-conversion/runs/phase6-special-20260812-lich-energy-drain
                                     rol-phase6-special-8eaf63b5965118f6
Phase 6 converted class-family guilds: lib/rol-conversion/runs/phase6-special-20260812-class-guilds
                                       rol-phase6-special-607369f4f87e0a4a
Phase 6 converted Sister Knights: lib/rol-conversion/runs/phase6-special-20260812-sister-knight
                                  rol-phase6-special-3aa909fd9793606b
Phase 6 Bloodstone undead death: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-vapor
                                 rol-phase6-special-fffdce44e3e15b73
Phase 6 Bloodstone critters: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-critter
                             rol-phase6-special-bdf567929b95b5d7
Phase 6 directional item blockers: lib/rol-conversion/runs/phase6-special-20260812-item-blocker
                                    rol-phase6-special-d11cc60b4d4cd56e
Phase 6 designated followers: lib/rol-conversion/runs/phase6-special-20260812-designated-follower
                              rol-phase6-special-2c7939775b253510
Phase 6 floating pools: lib/rol-conversion/runs/phase6-special-20260812-floating-pool
                        rol-phase6-special-c7ae6f16963a5f16
Phase 6 Bloodstone portals: lib/rol-conversion/runs/phase6-special-20260812-bloodstone-portal
                            rol-phase6-special-f629c37b68cdad7d
Phase 6 Waterdeep guild rooms: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-guild
                                  rol-phase6-special-667bf4274a2fd6dd
Phase 6 batched combat/death: lib/rol-conversion/runs/phase6-special-20260812-batched-combat-death
                              rol-phase6-special-bcb867fc0cb376eb
Policy:  rol-conversion-policy-1
```

## Current evidence state

- Phase 0 reproduces all seven source aggregates byte for byte, reserves the candidate
  entity and zone ranges across world, code, and 50 database columns, and freezes the
  41,468-finding pre-existing target baseline.
- Phase 1 parses all 71,680 active records and 420,124 directives, types 355,042
  references, owns the full active dependency closure, inventories runtime and
  persistent bindings, and gives all 89 capabilities a disposition.
- Phase 2 gives every active record a final action: 68,146 `ADD`, 2,458 `KEEP`, 1,070
  `MERGE`, and six `EXCLUDE`, with zero live target writes.
- Phase 3 preserves Jotun source zone 960 as target zone 1960 through a real `KEEP`,
  stages the complete target, validates equivalence, applies twice with zero writes,
  and proves the authoritative target tree is unchanged.
- The two controlled Phase 3 runs have identical manifests, all 12 hashed artifacts
  are byte-identical, and both produce run ID `rol-phase3-11336f1832d8765c`.
- Phase 4 disposes all 3,001 selected actions, all 245 selected SOC records, and all 91
  selected special bindings. It emits 25 files and 194 DG triggers, patches 73 preserved
  mobiles, appends 14 shops, and performs zero implicit overwrites or live target writes.
- The generated overlay parses completely and adds zero active staged errors. The 79
  active staged errors are inherited target-baseline findings; they remain repair work
  for the relevant Phase 7 batches and are not waived.
- Reset-reference and scripted walkthrough evidence passes for all five pilot zones and
  all 1,160 selected rooms. The isolated test-database boot enters the game loop,
  observes eligible resets for zones 1591 and 20586, and terminates normally with no
  pilot-related spell, reference, reset, trigger, extraction, or `SYSERR` diagnostics.
- The world-tool suite passes 281 tests; the production-linked CuTest suite passes 640;
  `make install` succeeds and leaves no root-level `circle` artifact.
- Twenty-eight bounded Phase 6 delivery sessions are archived. Dependency-complete
  batches now target 15-30 source handler families per checkpoint. The measured
  remaining forecast is 63-105 sessions: Phase 6 is 15-29, Phase 7 is 42-66, and
  Phase 8 is 6-10.
- Phase 5 now handles argument-free quest attacks, configured experience, signed
  quest-point deltas, all 29 active spell/skill reward identities, and explicit SOC
  `LISTDONE` termination. Existing HLQuest persisted command indexes remain stable.
- Phase 5 now handles all three active room level ranges. The one active fall-chance
  row and 36 obsolete mana rows are source-inert and are omitted with explicit
  diagnostics after source runtime tracing.
- Phase 5 now converts all 29 valid active object-trap payloads into persistent target
  object values and runtime behavior. Four empty source rows are omitted explicitly.
  The five pilots contain no active object traps; the trap-only checkpoint's restage
  was byte-identical with run ID `rol-phase4-build-1dc8a681fa1595d5`.
- The current Phase 5 full-corpus audit emits all 69,920 convertible active records,
  totaling 42,089,791 bytes, with zero transform exceptions and zero writes. Complete
  ownership of room, zone, sector, mobile action, object type, object wear, object extra,
  object apply, and mobile/object affect values reduces unmapped symbolic observations
  from 26,006 to zero.
- Nine appended object flags preserve RoL identify, summon, sleep, charm, two-handed,
  race-restriction, whole-body, and whole-head behavior. Source `NOSHOW` maps to hidden;
  source-inert `DARK` is explicitly omitted. The refreshed five-zone pilot exposes five
  of these behavior families for manual testing.
- Source agility and power applies now map to Dexterity and Wisdom; maximum-stat applies
  map to their target attributes, and the four active race-factor tuples use bounded
  fixed-stat equivalents. Karma and Luck fields are explicitly source-only. Corrected
  object parsing prevents post-extension numeric payload from becoming false affects.
- RoL slow poison and docile behavior now use extensible target secondary affects;
  meditation uses the target rapid-preparation affect. Transient/inert prototype
  affects and three malformed source rows have explicit, reproducible dispositions.
- All 15,243 active mobile-action observations now have an explicit disposition. New
  compatibility flags preserve nice-thief, stay-sector, delayed-hunter, archer,
  independent class-role, and race-aggression behavior; protector expands to existing
  helper/listener behavior. `ACT_SPEC` is explicitly owned by Phase 6 binding
  reconciliation, while relationship-only or source-inert flags are logged omissions.
- Converted zones now carry `RoL-Reset-Compat`: their `E` and `G` chains remain bound
  to the most recent successful `M`, matching the source even after an intermediate
  equipment failure. Native zones retain result-offset behavior, and the three active
  non-boolean source dependencies are normalized to the source's actual boolean rule.
- All 34 valid active exit-trap payloads now use a persistent target room record and
  runtime adapter. Blade, poison, rock, fire, lightning, random, falling, area, boot
  load, detection, disabling, open/pick triggering, and reset rearming are preserved.
  The one malformed source row is excluded with a source-located diagnostic.
- All 1,467 active quest item-reward directions carry one fixed object VNUM. The source
  engine's optional random-range upper bound is unused by active content and blocks no
  record.
- Phase 5 is complete. The final audit emits all 453 active shops, preserves fixed and
  roaming operation, exact source price formulas, authored customer restrictions,
  adverse `CHEATS` pricing, and the converted `CASTING` policy. It reports zero generic
  capability gaps, zero unmapped symbolic observations, zero transform exceptions, and
  zero live target writes.
- The Phase 6 inventory accounts for 1,234 active direct bindings across 605 source
  handlers and locates all 605 source definitions. Shared service reuse accounts for
  72 bindings; source-inert dump and cityguard callbacks account for 22; bounded
  corpse-devourer, poison-bite, and thief adapters account for 29.
- The independent `ACT_SPEC` cross-check resolves 563 of 848 records and leaves 285
  pending. It remains a scheduling cross-check rather than the direct-binding denominator;
  composition-safe flags and room or object procedures can resolve source handlers without
  changing this mobile-only count.
- All 247 source boot-time race procedures are complete through composition-safe mobile
  flags and activity/combat hooks: 134 demons, 101 devils, and 12 umber hulks. The
  converter emits the flags on 239 `ADD` records and deterministically patches the eight
  preserved target prototypes. A refreshed pilot proves all six Hulburg `KEEP` patches
  with zero new staged errors.
- Seven named breath procedures close 27 direct bindings with the source four-turn
  cadence, half-level single-target attacks, and full-level room-wide weapons. Three
  composition-safe conjured-death flags close another 45 bindings while preserving
  familiar, mount, and summoned-monster fade messages and suppressing corpses.
- All 44 `home_reset` room bindings are complete through a composition-safe room flag
  and successful-movement hook. Converted NPCs update their remembered home only after
  leaving the marked room; blocked and trigger-rejected attempts do not retarget them.
- All 13 active `magic_pool` bindings are complete through a named object procedure and
  converter-owned value-reference remapping. All 12 distinct destination rooms resolve
  through Phase 2 identities; fixed damage remains in object value 1.
- All 22 active `autoDistributor` room bindings are complete through the named
  `RoL Auto Distributor` procedure. Mortal commands move the actor to a random loaded
  room in the same zone, staff are exempt, and the source's inert periodic callbacks
  are not reproduced as unnecessary scheduling work.
- All eight active `shadow_giant` bindings are complete through the named
  `RoL Shadow Giant` procedure. Its periodic 1-in-21 pulse applies source-equivalent
  level-30 `spook` damage, save, and stun behavior to players and pets; converted angel
  identity remains distinct for the source immunity list.
- All 57 active ship-family bindings are complete through five named procedures and a
  shared fixed-interior runtime adapter. The seven hulls retain boarding, controls,
  combat, lookout, disembarking, navigator protection, crew calls, and their original
  two-way scheduled routes without exceeding the target Greyhawk interior limit.
- All 47 active `guild_guard` bindings are complete through the named `RoL Guild Guard`
  procedure. It preserves the 45 active gate rules across 44 converted load rooms,
  multiclass and race admission, protected-guard retaliation, combat cleanup, and
  same-zone safe relocation while adding the target-required `MOB_SPEC` flag.
- All 21 active `shaman_totem` object bindings and all 21 matching spirit death handlers
  are complete. The object adapter preserves permanent totem/player bonding, good/evil
  source-race gating, the Cleric-21 mapped unlock, three attempts per seven MUD days,
  bounded spirit scaling, follower assistance, and animal-specific corpse-free deaths.
- All eight active `major_beholder` bindings are complete through a mobile-owned combat
  adapter. It preserves ten independent eye identities, one-in-three ready-eye checks,
  three-turn per-eye cooldowns, player/pet targeting, and target-native spell mappings.
  The source-only all-unused-eyes critical burst is explicitly unavailable because the
  target gateway has no source weapon-critical event.
- All seven active `bandit` bindings are complete through the named `RoL Trade Bandit`
  procedure. It preserves player capture, movement and object-command interception,
  variant-specific gold and wagon demands, underpayment hostility, successful-payment
  disappearance, repeat-attempt aggression, and lazy expiry while repairing the source
  missing-wagon null extraction defect.
- All six active `lich_energy_drain` bindings are complete through the named
  `RoL Lich Energy Drain` procedure. It preserves current-opponent party targeting,
  independent one-in-five checks, full-current-hit-point transfer, Blackmantle healing
  suppression, Death Ward protection mapping, cumulative stun, and casting suppression.
- All ten active class-family guild bindings are complete through four named room
  procedures. They preserve source mage, thief, warrior, and cleric admission gates,
  accept any matching class in a target multiclass build, and delegate accepted commands
  to the current guild service instead of restoring obsolete source practice mechanics.
- All five active `sister_knight` bindings are complete through the named
  `RoL Sister Knight` procedure. An attacked Sister shouts once per combat encounter
  and sends awake, idle, reachable converted sisters in the same zone to pursue the
  attacker, while preserving source sound, casting, paralysis, and distance gates.
- All four active `bs_undead_die` bindings are complete through the composition-safe
  `RoL-Black-Vapor-Death` mobile flag. The converted undead retain their source vapor
  message and target-native no-corpse policy without consuming their named SpecProc slot.
- All four active `bs_critter` bindings are complete through the named
  `RoL Bloodstone Critter` procedure. Awake, idle critters use the target's current
  snarl or growl social on the source 2-in-81 activity cadence.
- All six active `item_block` object bindings are complete through the named
  `RoL Item Blocker` procedure. Each object preserves its authored cardinal direction
  and blocks mortal player or pet movement and matching unlock attempts only while an
  aggressive NPC occupies the room.
- All five active `follow_that_mob` bindings are complete through the named
  `RoL Designated Follower` procedure. Awake Icecrag guards attach to their fixed,
  colocated NPC leader, follow its movement, and assist its fights while retaining
  source docile and no-kill suppression.
- All four active `floating_pool` object bindings are complete through the named
  `RoL Floating Pool` procedure. Room pools move once per auto-pulse roll through a
  random eligible cardinal exit on the source-documented 12 percent cadence.
- All four active `bs_portal` object bindings are complete through the named
  `RoL Bloodstone Portal` procedure. Exact visible-object entry remaps each destination,
  applies target-safe admission, preserves mortal hit-point and movement stress, exempts
  staff, and retains the source's below-negative-ten death threshold.
- All twelve active Waterdeep guild wrappers are complete through the named
  `RoL Waterdeep Guild Room` procedure. Exact and family class gates use the target
  multiclass model, accepted commands delegate to the current guild service, and the
  source Mercenary gate maps explicitly to target Warrior.
- Seven alert bindings are complete through `RoL Alert Caller` or composition with the
  existing Imix/Yancbin breath procedure. The converted callers preserve their
  source-specific messages, helper identities, once-per-fight gate, and same-zone
  reachability and state checks.
- All five active `yggdrasil_branch` bindings are complete through the named
  `RoL Yggdrasil Branch` procedure. Source target weighting, attempt/save gates, timed
  entangle, release, and current-movement halving are preserved.
- Sixteen active tentacle, treant, phantom-steed, dark-shade, mephit, and elemental
  death bindings are complete through converted-VNUM runtime profiles. Each preserves
  its source-family message and no-corpse outcome without using another named slot.
- The source C preprocessor removes 87 of the 1,234 discovered binding candidates under
  the checked-in RoL configuration. The active denominator is 1,147 bindings across 562
  handlers; a separate ledger preserves every exclusion, and none affected the current
  five-package staged pilot.
- The current Phase 6 checkpoint resolves 634 of 1,147 active direct bindings and 130 of
  562 source handlers, leaving 513 bindings and 432 handlers. The independent
  `ACT_SPEC` cross-check resolves 568 of 848 records and leaves 280 pending.
- The 804 record-specific reference gaps remain owned by Phase 7 dependency batches.

## Immediate next actions

1. Reconcile the remaining 513 direct bindings across 432 handlers in
   dependency-complete batches of 15-30 related families. Use shared data-driven
   profiles and current target procedures before adapting or porting.
2. Preserve record-specific missing-reference repairs for their Phase 7
   dependency-closure batches.
3. Preserve the six locked malformed record exclusions as explicit, logged
   smallest-unit exclusions.
4. Regenerate the special-binding inventory after each shared-family checkpoint and
   repeat structural, syntax-boot, isolated behavioral, reset, and walkthrough gates.
