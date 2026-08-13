# Realms of Luminari Conversion Worknotes

- Updated: 2026-08-13
- Environment: development
- Branch: `master`
- Current task: reconcile the remaining dependency-complete Phase 6 special families
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
Phase 6 lost-totem-restorer commit: 3f773d78
Phase 6 lich-rite commit: 7d28382f
Phase 6 Waterdeep-town-crier commit: 6c64fba1
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
Phase 6 Waterdeep-ambient commit: 27bad343
Phase 6 expanded-Waterdeep-ambient commit: 30132767
Phase 6 generated source-periodic commit: c16e0fe9
Phase 6 generated state-periodic commit: a67c1bfa
Phase 6 cross-zone periodic expansion commit: 2844472f
Phase 6 high-fanout special-adapter commit: 1108fb56
Phase 6 guild-family adapter commit: 99cb8aed
Phase 6 Waterdeep-guard composition commit: ef0571bd
Phase 6 death/periodic bulk-profile commit: 6a81b61e
Phase 6 command-sentinel commit: 827d5f6d
Phase 6 toll/ticket-keeper commit: cecbec9d
Phase 6 travel-portal commit: 3c0ab331
Phase 6 artifact-reconciliation commit: 2656b640
Phase 6 banana/god-toy commit: 1ab77127
Phase 6 undead-drain-family commit: 1f3e5172
Phase 6 Waterdeep-peacekeeper commit: 526b99cb
Phase 6 weapon-procedure commit: 5e4dc1a8
Phase 6 expanded-weapon-procedure commit: b1b42a5c
Phase 6 multi-event-weapon commit: 457672d6
Phase 6 multi-event-weapon archive commit: ec6ef4fb
Phase 6 monster-combat commit: 5925a88f
Phase 6 monster-combat archive commit: 33a91bf6
Phase 6 expanded monster-zone commit: 76aaf29f
Phase 6 composed-periodic commit: 079ca263
Phase 6 Lavatubes commit: 17013cd0
Phase 6 named-guild/utility-object commit: a13f74f7
Phase 6 residual-monster-combat commit: 2c44bf14
Phase 6 residual-mobile-procedure commit: 0c545b1c
Phase 6 called-effect object commit: c334a648
Phase 6 object-service commit: 1849d9ad
Phase 6 utility-service commit: 9d40694b
Phase 6 scheduled-mobile commit: c8704d86
Phase 6 Menden-fisherman commit: 33965efc
Phase 6 special-discovery-repair commit: c2a677a8
Phase 6 Tarrasque-encounter commit: bbdf893a
Phase 6 exact-class-guild commit: b0e924b8
Phase 6 planar-demon-base commit: c90ced37
Phase 6 planar-mobile-initializer commit: 66c3d7a1
Phase 6 Darkhold-elemental-death commit: 41ba7cea
Phase 6 Seelie-faerie commit: f945051b
Phase 6 Seelie-disabled-state compatibility commit: f40a5e48
Phase 6 Hive-manscorpion-venom commit: 236296dd
Phase 6 successful-hit-area commit: 3c9f0afe
Phase 6 Hive-Skriaxit-sandstorm commit: cb964e66
Phase 6 planar-death/burst/weapon commit: 336ee930
Phase 6 planar-capture/charm/dance commit: ba20b88a
Phase 6 Avernus-devil-combat commit: 31282808
Phase 6 Avernus-lifecycle commit: c15a7aa9
Phase 6 Scornubel-family commit: 8a741c3c
Phase 6 Zhentil-periodic commit: afe6b68d
Phase 6 Darkhold-special commit: 90adccdb
Phase 6 drow-equipment commit: be32b3d3
Phase 6 Undermountain-ambient commit: fcfd2be6
Phase 6 source-death-effects commit: 53b6a3e4
Phase 6 Griffon-non-Berserker-aggression commit: 5fb8b1cb
Phase 6 paralysis-gaze-and-venom-tail commit: 04d8d449
Phase 6 drow-conclave-guard commit: eb084a07
Phase 6 Death's-Head-lifecycle commit: 55a19242
Phase 6 remaining-hit-weapons commit: f78984d3
Phase 6 Bhaal/Seelie-hit-weapons commit: 86f8420f
```

The authoritative ignored runs are:

```text
Phase 0: lib/rol-conversion/runs/phase0-1619ccd8
         rol-phase0-02a84b2da28503c1
Phase 1: lib/rol-conversion/runs/phase1-policy2-20260813-special-discovery
         rol-phase1-237602d3ade48138
Phase 2: lib/rol-conversion/runs/phase2-policy2-20260813-special-discovery
         rol-phase2-c93b8c4610b36d1e
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
Phase 5 policy-2 full audit:
  lib/rol-conversion/runs/phase5-policy2-20260813-special-discovery-audit
  rol-phase5-audit-cec58661a4f21a2a
Phase 6 special reconciliation:
  lib/rol-conversion/runs/phase6-special-20260813-bhaal-bards
  rol-phase6-special-003a15934c33cde5
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
Phase 6 Waterdeep ambient: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-ambient
                           rol-phase6-special-7d3a624a62a104e5
Phase 6 expanded Waterdeep ambient: lib/rol-conversion/runs/phase6-special-20260812-waterdeep-ambient-2
                                    rol-phase6-special-af17e0481a21298e
Phase 6 generated source periodic: lib/rol-conversion/runs/phase6-special-20260812-source-periodic
                                   rol-phase6-special-5d67954ff0a9adbc
Phase 6 generated state periodic: lib/rol-conversion/runs/phase6-special-20260812-state-periodic
                                  rol-phase6-special-cb19c31118c1ce47
Phase 6 cross-zone periodic expansion:
  lib/rol-conversion/runs/phase6-special-20260812-periodic-expansion
  rol-phase6-special-ebf492f80a4ac682
Phase 6 high-fanout special adapters:
  lib/rol-conversion/runs/phase6-special-20260812-high-fanout
  rol-phase6-special-cedd3394da53d442
Phase 6 guild-family adapters:
  lib/rol-conversion/runs/phase6-special-20260812-guild-families
  rol-phase6-special-33a4bb8a0371811c
Phase 6 Waterdeep-guard composition:
  lib/rol-conversion/runs/phase6-special-20260812-waterdeep-guards-v2
  rol-phase6-special-02a63509af7e962d
Phase 6 death/periodic bulk profiles:
  lib/rol-conversion/runs/phase6-special-20260812-death-periodic
  rol-phase6-special-df9ed4c4ca50c8ed
Phase 6 command sentinels:
  lib/rol-conversion/runs/phase6-special-20260812-command-sentinels
  rol-phase6-special-ab673270b393501a
Phase 6 toll and ticket keepers:
  lib/rol-conversion/runs/phase6-special-20260812-toll-keepers
  rol-phase6-special-069b798651151c53
Phase 6 travel portals:
  lib/rol-conversion/runs/phase6-special-20260812-travel-portals
  rol-phase6-special-306471922b67fe8c
Phase 6 artifact reconciliation:
  lib/rol-conversion/runs/phase6-special-20260812-artifacts
  rol-phase6-special-7e5f0048a9e4d79e
Phase 6 banana and god-toy isolation:
  lib/rol-conversion/runs/phase6-special-20260812-god-toys
  rol-phase6-special-ddaefe6a4a82d922
Phase 6 undead drain family:
  lib/rol-conversion/runs/phase6-special-20260812-undead-drain
  rol-phase6-special-3571b059181f795c
Phase 6 Waterdeep peacekeepers:
  lib/rol-conversion/runs/phase6-special-20260812-waterdeep-peacekeepers
  rol-phase6-special-2d173c6ee61f4ba1
Phase 6 weapon procedures:
  lib/rol-conversion/runs/phase6-special-20260812-weapon-procs
  rol-phase6-special-8183ba9f3e112f6c
Phase 6 expanded weapon procedures:
  lib/rol-conversion/runs/phase6-special-20260812-weapon-procs-2
  rol-phase6-special-9aa7a0cdaab7a9d1
Phase 6 multi-event weapon procedures:
  lib/rol-conversion/runs/phase6-special-20260812-weapon-multievent
  rol-phase6-special-5850783628391c08
Phase 6 monster-combat procedures:
  lib/rol-conversion/runs/phase6-special-20260812-monster-combat
  rol-phase6-special-66d30be39f08fcda
Phase 6 expanded monster-zone procedures:
  lib/rol-conversion/runs/phase6-special-20260812-monster-zones
  rol-phase6-special-fb733f9680d8b786
Phase 6 composed periodic profiles:
  lib/rol-conversion/runs/phase6-special-20260812-periodic-composition
  rol-phase6-special-d1472a439e2e94b8
Phase 6 Lavatubes procedures:
  lib/rol-conversion/runs/phase6-special-20260812-lavatubes
  rol-phase6-special-9969533324d768ef
Phase 6 named guild and utility objects:
  lib/rol-conversion/runs/phase6-special-20260812-utility-objects-v2
  rol-phase6-special-35cfdfe1d528d25f
Phase 6 residual monster-combat procedures:
  lib/rol-conversion/runs/phase6-special-20260812-monster-combat-residual
  rol-phase6-special-162570d805f14abd
Phase 6 residual mobile procedures:
  lib/rol-conversion/runs/phase6-special-20260812-residual-mobiles
  rol-phase6-special-c60c0d2b988fd49f
Phase 6 called-effect objects:
  lib/rol-conversion/runs/phase6-special-20260812-called-objects
  rol-phase6-special-abe9fabc332abee0
Phase 6 object services:
  lib/rol-conversion/runs/phase6-special-20260812-object-services
  rol-phase6-special-e50685fc20cfaf75
Phase 6 utility services:
  lib/rol-conversion/runs/phase6-special-20260812-utility-services
  rol-phase6-special-2c12ac866ad07db2
Phase 6 scheduled mobiles:
  lib/rol-conversion/runs/phase6-special-20260812-scheduled-mobiles
  rol-phase6-special-c447dd6b4665cb7a
Phase 6 Menden fisherman:
  lib/rol-conversion/runs/phase6-special-20260812-menden-fisherman
  rol-phase6-special-aab827a742a51ca2
Phase 6 lost totem restorer:
  lib/rol-conversion/runs/phase6-special-20260812-totem-restorer
  rol-phase6-special-9139221a800d60a0
Phase 6 lich rite:
  lib/rol-conversion/runs/phase6-special-20260812-lich-rite
  rol-phase6-special-ded69599851e733e
Phase 6 Waterdeep town crier:
  lib/rol-conversion/runs/phase6-special-20260812-waterdeep-crier
  rol-phase6-special-e0e90cdd3f12895e
Phase 6 planar demon base:
  lib/rol-conversion/runs/phase6-special-20260813-planar-base
  rol-phase6-special-55c1c510a1bc029d
Phase 6 planar mobile initializers:
  lib/rol-conversion/runs/phase6-special-20260813-planar-initializers
  rol-phase6-special-49b79534c90b09d7
Phase 6 Darkhold elemental deaths:
  lib/rol-conversion/runs/phase6-special-20260813-darkhold-elemental-deaths
  rol-phase6-special-cc108f5e1415f677
Phase 6 Seelie faeries:
  lib/rol-conversion/runs/phase6-special-20260813-seelie-faerie
  rol-phase6-special-66858c11e3301a3e
Phase 6 Hive manscorpion venom:
  lib/rol-conversion/runs/phase6-special-20260813-hive-manscorpion-venom
  rol-phase6-special-6cfc16c58f7802e2
Phase 6 successful-hit area:
  lib/rol-conversion/runs/phase6-special-20260813-successful-hit-area
  rol-phase6-special-767041a4f1ffe023
Phase 6 Hive Skriaxit sandstorm:
  lib/rol-conversion/runs/phase6-special-20260813-hive-skriaxit-sandstorm
  rol-phase6-special-337e1c4ccb66c3bc
Phase 6 planar death, bursts, and Balor weapons:
  lib/rol-conversion/runs/phase6-special-20260813-planar-death-bursts-weapons
  rol-phase6-special-1f5bf8c5d82f76a4
Phase 6 planar capture, charm, and Vrock dance:
  lib/rol-conversion/runs/phase6-special-20260813-planar-capture-charm-dance
  rol-phase6-special-dd88e66849111ed1
Phase 6 Avernus devil combat:
  lib/rol-conversion/runs/phase6-special-20260813-avernus-devil-combat
  rol-phase6-special-bc771a97be7031ca
Phase 6 Avernus lifecycle:
  lib/rol-conversion/runs/phase6-special-20260813-avernus-lifecycle
  rol-phase6-special-80e5cc527bee919a
Phase 6 Scornubel family:
  lib/rol-conversion/runs/phase6-special-20260813-scornubel
  rol-phase6-special-885945d3c68756af
Phase 6 Zhentil periodic family:
  lib/rol-conversion/runs/phase6-special-20260813-zhentil-periodic
  rol-phase6-special-da5452c48ee34106
Phase 6 Darkhold special family:
  lib/rol-conversion/runs/phase6-special-20260813-darkhold
  rol-phase6-special-2b80ad51d6738778
Phase 6 drow-equipment decay family:
  lib/rol-conversion/runs/phase6-special-20260813-drow-equipment
  rol-phase6-special-477a41d7687e3e25
Phase 6 Undermountain ambient and inert-stench family:
  lib/rol-conversion/runs/phase6-special-20260813-undermountain-socials
  rol-phase6-special-b66f1c18acb2ce12
Phase 6 source death-effects family:
  lib/rol-conversion/runs/phase6-special-20260813-source-death-effects
  rol-phase6-special-59a0f3f782fc7f30
Phase 6 Griffon's Nest non-Berserker aggression family:
  lib/rol-conversion/runs/phase6-special-20260813-griffon-nonbarbarian
  rol-phase6-special-443dfa53de680c8e
Phase 6 Dusk Road paralysis gaze and Undermountain venom tails:
  lib/rol-conversion/runs/phase6-special-20260813-paralysis-tails
  rol-phase6-special-869dda48a01594d1
Phase 6 Undermountain drow conclave guards:
  lib/rol-conversion/runs/phase6-special-20260813-drow-conclave
  rol-phase6-special-1b44c259bece3bf6
Phase 6 Undermountain Death's Head lifecycle:
  lib/rol-conversion/runs/phase6-special-20260813-deaths-head
  rol-phase6-special-cc7c00546c5db049
Phase 6 remaining hit-only weapons:
  lib/rol-conversion/runs/phase6-special-20260813-hit-weapons
  rol-phase6-special-db296ed8ac49dda1
Phase 6 Bhaal and Seelie hit weapons:
  lib/rol-conversion/runs/phase6-special-20260813-bhaal-bards
  rol-phase6-special-003a15934c33cde5
Policy:  rol-conversion-policy-2
```

## Current evidence state

- Phase 0 reproduces all seven source aggregates byte for byte, reserves the candidate
  entity and zone ranges across world, code, and 50 database columns, and freezes the
  41,468-finding pre-existing target baseline.
- Phase 1 parses all 71,680 active records and 420,124 directives, types 355,042
  references, owns the full active dependency closure, inventories runtime and
  persistent bindings, and gives all 89 capabilities a disposition.
- Phase 2 gives every active record a final action: 68,135 `ADD`, 2,469 `KEEP`, 1,070
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
- The world-tool suite passes 357 tests; the production-linked CuTest suite passes 689;
  `make install` succeeds and leaves no root-level `circle` artifact.
- The corrected discovery repair and twenty-five subsequent denominator-bearing batches are
  archived. Those batches closed 339 bindings across 141 source handlers. The measured remaining
  Phase 6 forecast is 10-21 sessions, or 20-84 focused engineering hours; the full remaining
  project range is 66-105 sessions, or 132-420 focused hours.
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
  totaling 42,100,085 bytes, with zero transform exceptions and zero writes. Complete
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
- The repaired Phase 6 inventory accounts for 1,721 active direct bindings across 795 source
  handlers and locates all 795 source definitions. The current reconciliation resolves 1,585
  bindings across 679 handlers and leaves 136 bindings across 116 handlers.
- The independent `ACT_SPEC` cross-check resolves 816 of 848 records and leaves 32
  pending. It remains a scheduling cross-check rather than the direct-binding denominator;
  composition-safe flags and room or object procedures can resolve source handlers without
  changing this mobile-only count.
- The 116 pending handlers comprise 103 singletons and 13 families with two to four bindings;
  no family with five or more bindings remains.
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
- Thirty-six active mobile death bindings are complete through converted-VNUM runtime
  profiles. The original tentacle, treant, phantom-steed, dark-shade, mephit, and
  elemental profiles retain their no-corpse behavior. The expanded profiles preserve
  source bursts, darkness, poison, returned possessions, stone-pile inventory,
  replacement forms, dropped objects, ordinary-corpse messages, and cleric retargeting
  without using another named slot.
- Thirty-four active Waterdeep ambient bindings across 23 source handler families are
  complete through `RoL Waterdeep Ambient`. Data-driven converted-VNUM profiles preserve
  the standing gate, two-die distributions, authored action sequences, casino
  fall-through, and merchant 2005310's converted harbor-room restriction.
- A second Waterdeep ambient batch adds 22 active bindings across 21 source handler
  families to the same adapter. It preserves the source two-d5 outcome tables and
  multi-message ordering, plus the Waterdeep guards' combat-suppression gate.
- One strict, source-hashed generator now closes 98 regular source handler families and 104
  active bindings through `RoL Source Periodic`. Its 104 converted mobile profiles retain
  380 random outcomes and 621 ordered speech or room actions from Bloodstone, Icecrag,
  Menden, Fun, Mobile, Realm, Lavatubes, Tower of Sorcery, and Waterdeep source files.
  Sorted generated tables support binary runtime lookup; random ranges, dice expressions,
  awake or sleeping gates, and combat gates retain source behavior. Bloodstone wolf and
  Waterdeep dog profiles explicitly compose the existing corpse/food devourer before or
  after their generated action tables in the source-authored order.
- A second strict, source-hashed generator closes 27 direct state-aware Waterdeep handler
  families through `RoL Stateful Periodic` and supplies seven composed Waterdeep guild
  guards through `RoL Guild Guard`. Its 34 converted mobile profiles retain 266
  idle/fighting outcomes and 274 ordered speech or room actions. Casino owner 2003206
  preserves the source's independent fighting and standing rolls during combat.
  Combat selects the explicitly authored fighting table before the target standing-state
  gate. `rogue_one` is separately excluded because its only registered event always
  triggers its source early return.
- Four mobile passage guards and two room command wards are complete through the shared,
  owner-aware `RoL Command Sentinel`. The converted rules preserve source race, class,
  level, direction, room, chance, staff, and glyph-damage behavior. Three Foggy Woods
  warning rooms share one generated entry trigger with their source warning sequence.
- Five toll, bridge, and ticket handler families covering ten active bindings are complete
  through `RoL Toll Keeper`. Nine converted mobile profiles preserve three fixed-fee
  passages, two bridge throws, four ticketed ship entries, target-currency conversion,
  underpayment retention, NPC passage, source periodic speech, exact rooms, destinations,
  ticket identities, and entered ship identities.
- Six travel-handler families covering nine active object bindings are complete through
  `RoL Travel Portal`. The identity-keyed profiles preserve dimensional-fold preview and
  arena parity, Waterdeep fixed damage and fountain class admission, four-slot random
  elfgates, carried Shaman-spore consumption and stun behavior through the target Cleric
  mapping, and the Blip portal's converted badge reward.
- Ten artifact-handler families covering eleven active bindings are reconciled through
  the existing modern artifact subsystem. Eleven policy-confirmed source identities map
  to target artifacts 169901-169910; source objects 1007 and 1009 intentionally converge
  on Kelrarin 169906. No duplicate artifact prototype or second persisted procedure is
  emitted. The separate `NeverLooseItem` callback is excluded because it exposes unsafe
  teleport, healing, resurrection, currency, permanent-stat, forced-death, invisibility,
  and unlock commands; its ordinary Raven earring data remains eligible for conversion.
- The two active `banana` bindings are complete through the typed, object-owned `RoL
  Banana` procedure. Eating converted fruit 2001235 preserves the source hunger gain,
  command delay, and temporary peel 2001234; the peel preserves the source Intelligence
  avoidance, Dexterity outcomes, sleep, bounded self-damage, movement interruption, and
  ten-real-minute decay. Seventeen separate destructive god-toy callbacks are excluded
  without excluding their ordinary object data.
- Seven consecutive undead-drain handlers are complete through one identity-profiled
  `RoL Undead Drain` mobile procedure. Converted mobiles 2001256-2001262 retain their
  one-in-16 or one-in-21 chances, shared melee/spell exclusion groups, two-to-three-tick
  armor, Dexterity, Strength, save, and slow profiles, failed-Will gate, and immunity for
  undead or Death-Warded victims. The source NPC-hit/critical callback cadence maps to
  the available target combat-turn event.
- Four tavern bouncer handlers, the casino bouncer, and the off-duty militia guard are
  complete through `RoL Waterdeep Peacekeeper`. Tavern bouncers 2005523 and
  2005541-2005543 preserve their converted return routes and drag eligible aggressors to
  2003258; casino bouncer 2003207 returns to its load room and ejects aggressors to
  2003254. Off-duty guard 2003229 retains its drunken ambient table and joins eligible
  fights. The target-required `MOB_SPEC` flag is supplied for all six bindings.
- All 19 active `specs.weapons.c` handler families covering 20 object bindings are
  complete through the typed, identity-profiled `RoL Weapon Proc`. The weapon-hit
  gateway now supplies exact damage, attack type, and critical state. Converted profiles
  preserve critical and sneak payloads, random hit procs, typed energy damage, spell
  effects, extra swings, wielder rejection, Gith charges and reclaimers, and Starsong's
  equipped `say labelas` weekly group barkskin invocation.
- Another 19 weapon-hit handler families covering 25 object bindings are complete through
  the same adapter. The 45 total profiles now include Mielikki, Flamberge, Orb,
  Doombringer, Tahlshara, Rockcrusher, Cymric, Torment, Pahluruk, dirk, Frulghiem,
  sphere-lightning, Halruaan staff, Magebane, dwarven-hammer, and Myth Drannor effects.
  Unavailable source debuffs map explicitly to target-native slow, faerie fire, and ray
  of enfeeblement behavior.
- Five multi-event weapon handlers are complete through the same adapter: Halruaan
  elemental and necromancer staves, the Hive gythka, the holy weapon, and Kor's
  battleaxe. The 50 profiles now support source command phrases, three-day cooldowns,
  object pulses, elemental summons, corpse preservation, venom and paralysis, holy
  wielder rejection and Paladin magic, dragon-scaled modifiers, and critical reverse
  swings. The converter supplies `ITEM_AUTOPROC` for the shared periodic contract.
- Thirty-five mobile-combat handler families covering 49 bindings are complete through the
  typed, identity-profiled `RoL Monster Combat`. Forty-five identity profiles preserve the
  earlier poison, lycan, shockwave, celestial, prismatic, and Elemental Tower behavior plus
  kobold, piercer, purple-worm, phalanx, skeleton, transformation, tree-spirit, Dranum,
  swallow, Canthus, and Jotun mechanics. Elemental Tower alerts and pit-fiend tails compose
  through the same persisted procedure, preserving the single-slot mobile binding contract.
- All six active Lavatubes handlers are complete through three typed, owner-specific
  procedures. The mobile adapter preserves snow-vulture activity and the automaton's
  alone reset; the object adapter preserves crystal-spike charges, skeleton-key unlocks,
  and the cellar lever; the room adapter preserves the paired trapdoor close, move, and
  block cycle. Invalid room pairs now log and fail safely instead of crashing the server.
- Six remaining named Waterdeep guild-guard handlers are complete through `RoL Guild Guard`.
  Their converted class gates, destinations, protection behavior, and `MOB_SPEC` flags are
  preserved; the Paladin guard retains its reachable idle table without inventing the five
  other source-unreachable periodic branches.
- Five active utility-object handlers are complete through the typed `RoL Utility Object`
  gateway. Converted goodberry, altar child, necromancer child, figurine, and ruby-monocle
  identities retain their source command or pulse behavior, with `ITEM_AUTOPROC` and the
  figurine mobile reference supplied only where required. `blackPlagueCure` and
  `craine_serpent` are source-inert because neither assigned callback registers an event.
- Ten residual mobile-combat handlers covering 13 active bindings are complete through
  the existing `RoL Monster Combat` gateway. Thirteen new identity profiles preserve the
  bounded Moonshae summons, Jurtrem sanctuary removal, Kamerynn teleport strike, Crimson
  Fury minion purge and fire blast, spiritist curse/disarm/cyclone branches, Tako pit
  interception, werewolf activity, and Mimer gate and return behavior. Helper load triggers
  run after setup so an extracting trigger cannot leave a subsequently dereferenced mobile.
- Seven additional residual mobile handlers covering 13 active bindings are complete through
  the existing `RoL Monster Combat` gateway. Thirteen identity profiles preserve delayed
  extraplanar vanishing, Beavis and Butthead social activity, Finn speech, faerie mischief and
  theft, spell-cast interception and counterstrikes, and the ancient brownie's ankle attack.
  The target damage, immunity, casting, and purge-event paths retain the intended mechanics
  without restoring unsafe direct state mutation. The assigned `clock_tower` object callback
  is separately source-inert: initialization returns no event bits, and the source tree has
  no separate clock-tower event registration.
- Eight called-effect object handlers are complete through the existing typed `RoL Utility
  Object` gateway. Exact source phrases, worn-item gates, instance cooldowns, target-native
  spell mappings, combat targeting, a charmed basilisk-snake summon, and the Staff of Magius
  light toggle are preserved through identity profiles. Item identification now exposes each
  invocation contract without adding a registry definition or changing persistence.
- Five more object-service handlers are complete through the existing typed `RoL Utility Object`
  and `RoL Weapon Proc` gateways. Lathander's disc preserves its exact rub-and-consume renewal,
  sleep, and stun contract; Llym's altar preserves held-treasure valuation, consumption, favor,
  summons, and rewards; the smoke shield preserves its block and punch discharges; the Crescent
  Moon preserves exact-case invocation and invisibility; and the Hellish Fury bow preserves its
  ranged fire payload. The source `FIREWEAPON` event maps to the target ranged-hit gateway, the
  Crescent Moon's pulse recharge maps to one actor combat-round wait, source vitality maps to
  target aid, and source coin types map to unified target gold. The assigned `nuclear_bomb`
  callback is source-inert because initialization returns no event bits, leaving its destructive
  missile-hit body unreachable.
- Four utility-service handlers are complete through the typed `RoL Utility Object` and new typed
  `RoL Utility Room` gateways. The Black Plague reservoir preserves its experienced-mortal disease
  exposure through target-native contagion; the source global plague toggle has no target
  equivalent. The loot blocker preserves aggressive-NPC protection for room containers and
  non-player corpses plus the 120-second corpse sweep, using a one-MUD-tick target-native decay.
  The newbie room preserves source-race east routing and maps the unavailable source birthplace to
  the target saved load room with the mortal start as fallback. The weight trigger preserves the
  5,000-unit transition and source messages; its source body contains no implemented door effect.
- Five scheduled-mobile handlers are complete through the builder-visible legacy `RoL Scheduled
  Mobile` gateway. The Waterdeep and Gloomhaven gate guards preserve their separate open, repair,
  and close windows, including the source's inactive 19-21 hour gap, gate-state corrections,
  speeches, glare, and ambient tables. The lighthouse keeper preserves its shared counter and the
  source hour-eight reset quirk. The naval combatant preserves the source standing-before-fighting
  branch order; the reachable defensive cast maps the source helper's actual stoneskin behavior,
  while the post-loop disarm remains excluded because the source loop makes it unreachable.
  Waterdeep town crier 2003008 preserves its 2d42 ambient distribution, 41 authored cases,
  source-shared hour gates, ship and shop warnings, zone shouts, combat alarm, and outdoor-only
  city responses.
- The Menden fisherman is complete through the source-hashed `RoL Source Periodic` gateway. Its
  awake gate, unrestricted fighting behavior, 1-80 roll table, 21 active outcomes, exact ambient
  messages, and room-visible socials are preserved. The generator and runtime now also preserve
  source-targeted social room and victim messages for named room occupants and self-targets; this
  reusable path covers the fisherman's wench, magus, and self interactions. The source `CMD_SIP`
  call has no action-table record and therefore contributes no room-visible output.
- The Outpost lost-totem restorer is complete through the builder-visible `RoL Totem Restorer`
  mobile procedure. It preserves the exact phrase, established Shaman-to-Cleric mapping,
  level-21 and saved-choice gates, source-equivalent 10,000-gold payment, exact persistent totem
  identity, and character binding. The helper is consumed only after the mapped object validates
  and loads.
- Both active `lichConverter` bindings are complete through the builder-visible `RoL Lich Rite`
  mobile procedure. It preserves the exact case-sensitive phrase, Necromancer and maximum-mortal
  gates, both keeper-held offerings, the full rite narrative, and helper consumption. Equipped
  offerings now retain safe concrete pointers, and the irreversible transformation uses the
  target's established no-group/follower preflight and Lich-to-Wizard respec contract.
- Phase 6 discovery now follows every active zero-argument registration call from
  `assign_mobiles()`, `assign_objects()`, and `assign_rooms()` through 53 reachable wrappers.
  Each binding retains its boot call path, original VNUM token, and literal or preprocessor
  resolution evidence. All 38 active planar macros resolve without a manual exception.
- The corrected static inventory contains 1,813 active-record candidates: the checked-in source
  preprocessor excludes 92 and leaves 1,721 live bindings. The live owner split is 1,098 mobile,
  323 object, and 300 room bindings across 795 direct handler names; all 795 definitions are
  located.
- The corrected reconciler resolves 1,585 static bindings and leaves 136 pending. It resolves
  679 direct handler names and leaves 116 pending across 27 source files. The pending set has
  103 singleton handlers and 13 handlers with two to four bindings; no handler with five or more
  bindings remains.
- Dynamic registration is explicit rather than counted as an unresolved symbolic VNUM. The
  quester path accounts for 5,078 active quest blocks across 5,039 unique hosts, and the
  shopkeeper path accounts for 453 active shops and hosts. Both are resolved through the target
  data-driven HLQuest and shop services. Static and dynamic paths total 7,252 active binding
  instances across 797 handler names.
- The regenerated `ACT_SPEC` cross-check resolves 816 of 848 records and leaves 32 pending.
  Automatic race composition still resolves all 247 implicit bindings; 85 now compose with a
  direct binding and 162 are implicit-only.
- The prior 1,112/1,147 binding, 538/562 handler, and 830/848 `ACT_SPEC` split is historical
  direct-only evidence and must not be used for progress or forecast claims.
- The dependency-complete Tarrasque encounter is reconciled through one typed owner-aware
  procedure. Mobile 2002601 preserves healing, pet execution, swallow, tail-fling, room sweep,
  and special death behavior; objects 2002604 and 2002610 preserve corpse entry and stomach
  acid. Death creates the special corpse, weighted internal loot, and a normal-schema return
  portal while suppressing the ordinary NPC corpse.
- Phase 6 now has a flow-bearing mobile-death event that is invoked only for registered
  definitions that advertise that contract. Existing mobile procedures therefore do not gain
  accidental death calls.
- Source direct-hit mutation is routed through target-native typed damage, saves, acid
  resistance, safe stun, and validated random teleport destinations. The source's obsolete
  random `teleport` spell meaning is preserved by bounded destination selection because the
  target spell now teleports its caster to a target character instead.
- The Tarrasque batch closes four direct bindings and four handler names. It is the first
  corrected-denominator closure and supplies the encounter-specific end of the measured sample.
- The exact-class guild batch closes 37 room bindings across 14 source callbacks by reusing the
  already production-tested mage, thief, cleric, and warrior family adapters. It introduces no
  new runtime surface, procedure identity, or prototype flag and preserves target multiclass
  admission for the migrated source roles.
- The planar demon base batch closes 33 bindings across two source handlers. All 25 explicit
  `abyssForgedWeapons` bindings receive `MOB_ROL_ABYSS_FORGED`, which dissolves the three wield
  slots before either typed special-death or ordinary corpse handling. The eight directly
  authored `standardDemon` bindings compose with their existing race-X `MOB_ROL_DEMON` runtime
  and consume no persistent procedure slot.
- The planar initializer batch closes eight bindings across six handlers. Bar-lgura and both
  Cambions retain source rogue roles plus permanent hide or sneak, while Lemure and Nupperibo
  gain target-native charm immunity. Alu-fiend regeneration is disabled in the active source;
  Dretch removes an absent wimpy flag; and Rutterkin changes no state, so those callbacks are
  recorded as source-inert rather than gaining invented target behavior. Owner-level staging now
  unions every composable action and affect requirement and rejects conflicting persisted procs.
- The Darkhold elemental-death batch closes four bindings across four handlers through the
  existing composable death-profile runtime. Converted fire, air, water, and earth elementals
  drop their mapped ruby, diamond, aquamarine, or golden nugget while retaining the source's
  ordinary-corpse path.
- The Seelie faerie batch closes 38 bindings across three handlers on 18 converted mobiles.
  Exact identity profiles compose the source prismatic, faerie-fire, and hidden-target search
  capabilities through `RoL Monster Combat`; the gateway also preserves the source chance to
  act and recover while disabled without changing unrelated mobiles.
- The Hive manscorpion-venom batch closes 16 bindings across four handlers. A typed
  successful-mobile-hit gateway preserves the source one-in-31, one-in-7, one-in-11, and
  one-in-25 chances, random mortal targeting, poison eligibility and saves, nonstacking
  Constitution loss, and the king's slow-poison-dependent lethal branch.
- The successful-hit area batch closes six bindings across six handlers. Dobluth bladestorm and
  wail, the Hive sandstorm beast, the Greycloak banshee and Urgutha Forka, and Aralesh Tandar
  now use exact `RoL Monster Combat` profiles on the typed successful-hit gateway. Their source
  chances, aggregate weapon damage, area damage, save, stun, blindness, silence, poison-fume,
  and execution behavior route through target-native safety and invalidate an extracted outer
  hit target before later riders can run.
- The Hive Skriaxit batch closes two bindings across one handler. Converted mobiles 2043741 and
  2043742 run their sandstorm every three mobile activity pulses even while fighting or disabled,
  reaching the current and open orthogonally adjacent rooms. The target preserves source
  eligibility, immunity, resistance, and first-failed-affect dispel behavior without inventing
  damage: the bound source room loop resets its counted Skriaxits before calculating `3 * num`.
- The planar death, burst, and Balor weapon batch closes 18 bindings across eight handlers.
  Manes retain their save-gated acidic death burst and ordinary corpse; Balors preserve
  elemental protection, weapon provisioning, and corpse suppression. The Balor whip and sword
  enforce demon ownership and preserve direct force damage, critical negative-energy area or
  ability-penalty branches, pet suppression, and safe object invalidation. Vrocks retain
  independently cooling low-health screeches and spore bursts, while Spinagons retain the
  source code's actual 20d2 spike damage, five-in-six trigger, 2-5 targets, save bonus, and
  three-round cooldown. Chasme buzz is explicitly source-inert because its callback tests the
  demon owner rather than the victim.
- The planar capture, charm, and Vrock-dance batch closes 11 bindings across five handlers.
  Glabrezus and Mariliths preserve their one-in-11 capture, strict raw-stat escape and death
  thresholds, captive command whitelist, and delayed ordinary-combat attack. Succubi preserve
  male targeting, one-in-four charm attempts, the source -2 save pressure, higher-level
  Blackguard service, captive commands, and one-to-four-hour lethal kisses while adding native
  charm immunity and resistance gates. Five active Vrocks now perform three timed dance stages,
  deal one shared 20d10 lightning roll with a Reflex save for half, and cool down for one MUD
  day. Cohort completion and abort repair the source's wrong-variable flag-clear defect.
- The Avernus devil-combat batch closes 22 bindings across seven handlers. Dragon 2032622
  composes the guild-guard procedure with its exact 30-room Tiamat alert. Fourteen Barbazu
  preserve one-in-20 received-hit rage; their two glaives preserve critical-only immediate NPC
  wounds and independently stacked, recurring PC blood loss. Gelugons 2033015 and 2033016 share
  the active source code's one-in-seven freezing tail, while Meritos adds caster silence and
  Hanariel intercepts disarm. Spear 2033012 preserves one-in-three slow and its devil-owner
  penalty. The port adds the typed mobile-was-hit gateway and uses target-native damage, saves,
  immunity, resistance, affect state, waits, and event lifecycle.
- The Avernus lifecycle batch closes 21 bindings across 14 handlers. Fifteen exact mobile
  profiles preserve Kri'ik transformation, forward and reverse ring and citadel patrols,
  per-mobile prison patrol state, rehiding, prisoner return, the Erinyes room illusion, deva
  echoes, Bel's chamber and guard lifecycle, the black altar, and dancing-dagger helpers. Four
  exact object profiles preserve the Kri'ik rod bargain, Bel's returning fire sword, and both
  dancing daggers. One typed room profile schedules the 16-room garden from its authored start
  room without scanning the world. `avernus_seal_unload` is source-inert because it never parses
  an event and therefore never registers its extraction callback.
- The Scornubel batch closes 18 bindings across 14 handlers. Seventeen mobile assignments use
  generated source-periodic profiles with exact roll ranges, awake and combat gates, actions,
  and messages. Parchimil 2006061 dispatches that table through `RoL Guild Guard`, retaining its
  passage behavior in the sole persisted procedure slot. Fiery mace 2006084 uses `RoL Weapon
  Proc` for its one-in-36 fixed 100-point source-untyped corona on wielded hits.
- The Zhentil periodic batch closes six bindings across six handlers through generated exact
  profiles. It extends the generator with a fail-closed zero-roll conditional shape for the
  one-in-11 little-girl wiggle and one-in-16 terrified-merchant shudder. The multi-event gate
  guard remains pending so its commands, timed gate state, and rescheduling are ported together.
- The Darkhold special batch closes all 16 active bindings across nine handlers. Six summon
  skulls, the passage skull, and four passage gems share an exact object adapter; warhammer
  2094571 and bastard sword 2094566 extend `RoL Weapon Proc`; and the shadow fiend and shadow
  dragon extend `RoL Monster Combat`. The previously converted four elemental deaths continue
  to compose independently through death profiles.
- The drow-equipment batch closes all 16 active `genericDrowEq` bindings through one exact
  object procedure and one lifecycle-safe object event. It preserves the source's mapped
  Underdark stop, surface restart, hourly cadence and jitter, object-placement and sunlight
  rates, no-sell state, source integer field reductions, messages, and terminal extraction.
- The Undermountain ambient batch closes eight live periodic handlers and records all six
  `um2_troglodyteStench` registrations as source-inert. The generated profiles cover converted
  mobiles 2093012, 2093021-2093023, 2093202, 2093211, 2093225, and 2093304, preserving 61
  authored outcomes, exact gates, speech and room actions, and the expanded `frown` social.
  Stench event registration and behavior remain absent because the active source compiles both
  out under `#if 0`.
- The source death-effects batch closes eight bindings across seven handlers. Exact profiles
  preserve both elemental-fire weevils, Lady Aleanrahel's banshee replacement and item transfer,
  mapped Helmed Horror and Butcher Knife drops, two shatter deaths, and the active white-pudding
  split. The weevil effect retains one shared `25d2` roll and source-order cumulative protection
  halving; all authored corpse dispositions remain exact.
- The Griffon's Nest batch closes all 15 active `aggroNonBarbarian` bindings through one exact
  monster-combat family. Eligible visible mortals without any Berserker level trigger an
  immediate battle cry and attack while their command continues; staff, NPCs, Berserkers, and
  unseen targets are exempt. An occupied memory-enabled guard remembers an eligible target
  without switching opponents, and periodic scans preserve source room-list order.
- The paralysis-gaze-and-venom-tail batch closes five bindings across three handlers. Dusk Road
  basilisks preserve their exact level-derived chance, room-list eligibility and visibility,
  source save pressure, continue-after-resist scan, and ten-round paralysis on the first failed
  save. The manscorpion and two wyvern identities preserve critical-only tail effects: random
  two-to-twelve-round paralysis for the manscorpion and death after a failed wyvern save. All
  three effects use target-native paralysis immunity, saves, and hit-context invalidation.
- The drow-conclave-guard batch closes all six active `um2_drowConclaveGuard` bindings. Exact
  profiles preserve one global alarm per boot, visible mortal and staff gates, detection for the
  five authored guard identities across all 11 conclave rooms, first-three-guard and sergeant
  barracks redeployment, and six combat lines on their one-in-20 rolls. Target-native room-path
  movement replaces the source hunt events, and room-list traversal safely repairs the source's
  wrong global-list advance without changing its documented one-pass barracks order.
- The Death's Head batch closes all five active bindings across `um2_deathsHeadTree`,
  `um2_deathsHead`, and `um2_deathsHeadSeed`. One owner-aware runtime preserves the four mobile
  identities, implanted seed, corpse-fed growth, head and fruit cycles, cries, bites, carrier
  damage, and corpse germination. The source's always-eleven mature regrowth, unreachable wood
  branch, already-seeded-corpse gate, and successful-sprout seed lifetime remain explicit.
- The remaining hit-only weapon batch closes seven bindings across six handlers through `RoL
  Weapon Proc`. Frostbite preserves its primary-slot one-in-22 `30d10` cold burst. The Trahern
  swords preserve primary/offhand one-in-33 level-derived damage plus a daytime scorching ray or
  nighttime one-round movement and armor penalties. Dancing Shadows preserves its primary-slot
  one-in-26 `37d9` strike and sequential protection, demon/devil, and save reductions. The two
  snake whips retain critical level-40 poison and the searing rod retains critical level-35
  burning hands. Converter output adds the automatic-procedure flag with the persisted binding.
- The Bhaal and Seelie hit-weapon batch closes three bindings across three handlers through `RoL
  Weapon Proc`. Warrior weapon 2063747 and rogue weapon 2063794 answer a target fire or cold
  shield with the triggering strike's damage, translate the source save pressure, and remain
  blocked by elemental protection or Globe of Invulnerability. Bard's glaive 2062750 preserves
  its level and Dexterity gates, two-round blindness, and saved `10d10` burst against an
  already-blinded target. All three retain source primary/offhand gating and typed invalidation.
- The twenty-five corrected batches close 339 bindings across 141 handlers. That measured
  throughput puts the binding-count projection near 10 sessions and the handler-diversity
  projection near 21. Because 103 of 116 remaining handlers are singletons, the published Phase 6
  envelope is 10-21 sessions, or 20-84 focused engineering hours. Reforecast after another
  material batch or an inventory correction.
- The 804 record-specific reference gaps remain owned by Phase 7 dependency batches.

## Immediate next actions

1. Reconcile the corrected pending inventory in dependency-complete shared-runtime batches.
2. Select the highest-value dependency-complete combat or utility family; do not batch
   merely by name or VNUM proximity.
   The Jotun object-passive family remains pending until source hit/damage slot mutation can be
   mapped without overwriting or double-counting the target's distinct enhancement applies.
   The Acheron portal family remains pending until its recorded entrance, `achexit`, logout,
   death, follower, and delayed-return lifecycle can be ported together without a one-way trap.
3. Preserve record-specific missing-reference repairs for their Phase 7
   dependency-closure batches.
4. Preserve the six locked malformed record exclusions as explicit, logged
   smallest-unit exclusions.
5. Regenerate the special-binding inventory after each shared-family checkpoint and
   repeat structural, syntax-boot, isolated behavioral, reset, and walkthrough gates.
6. Measure throughput against the 10-21-session Phase 6 envelope and reforecast after another
   material batch or an inventory correction.

## Latest session handoff

- Implementation commit `86f8420f` reconciles three active hit-weapon bindings through the
  existing typed `RoL Weapon Proc` definition: Bhaal warrior weapon 2063747, Bhaal rogue weapon
  2063794, and the ordinary Seelie bard's glaive 2062750.
- Both Torment weapons retain primary/offhand gating and answer a target fire or cold shield with
  the triggering strike's damage. A successful translated save halves the extra strike;
  elemental protection and Globe of Invulnerability on the wielder block it. Rogue and warrior
  flare wording, including the source cold-shield distinction, remains identity-specific.
- The bard's glaive retains its level-above-25 gate and exact Dexterity-weighted `0..2000` roll.
  Its first proc directly applies two rounds of blindness; a proc against an already-blinded
  target rolls `10d10` source-untyped damage, halved by a successful translated save. Typed
  damage and context invalidation protect lethal paths.
- Reconciliation maps all three handler names to `RoL Weapon Proc`; no registry definition or
  player command was added. The Spider venom pouch, Jotun skull, and master bard's glaive remain
  pending because their source callbacks can suppress or replace the triggering hit before base
  damage, while the current target weapon gateway is post-damage. The Bhaal mage and priest
  callbacks remain pending until their periodic cadence and missing Agility/Farsee dependencies
  can be ported together.
- The authoritative evidence is
  `lib/rol-conversion/runs/phase6-special-20260813-bhaal-bards`, run
  `rol-phase6-special-003a15934c33cde5`. A same-timestamp repeat was byte-identical. The standard
  path-and-content tree digest is
  `cffb149c90f72be5d493db5eaed59d36b22fda776f80f9faf937c89e3fca42cd`.
- The binding ledger SHA-256 is
  `0819a1aa9c464f401f4e541988ccb4e317fba1a1accf85ef477e9e1f7c5e586c`; the handler inventory
  SHA-256 is `82901f913db4cccc3a652fab376fdb11bc1e28a6bd2e8320849aa14a30eba028`;
  and the summary SHA-256 is
  `73bfb7bfe0ce2db6912040b39a583ca1c5b6a9fbfac088ee385a2c56a4de60d5`.
- The denominator remains 1,721 live static plus 5,531 resolved dynamic binding instances.
  Resolution is now 1,585 static bindings across 679 handlers, leaving 136 bindings across 116
  handlers in 27 source files. The independent `ACT_SPEC` cross-check remains 816 resolved and 32
  pending. The pending shape is 103 singletons, seven two-binding, five three-binding, and one
  four-binding family. All automatic race and dynamic paths remain complete, with zero live
  target writes.
- The target registry remains 117 definitions: 98 legacy and 19 typed, with 118 persisted
  compatibility names. Final validation passed: 357 world-tool tests, 689 production-linked
  CuTests, a complete CMake build and all 12 CTest targets, zero documentation findings, a
  warning-free Autotools build/test/install, and no root-level `circle`. Installed build ID
  `b05bb4ab67c100c4461503ceb2bd2a791982f784`; SHA-256
  `06e1f58c43f25ef552ea9ef13ae4b1b6a9e7da5785636015b91e68b5d7c4d685`.
- No player helpfile changed because the batch adds no player command or syntax. The staff manual
  covers all three identities, slot and level gates, shield and ward interactions, save
  translation, blindness progression, and the dependency-stage boundary.
- Twenty-five measured batches have closed 339 bindings across 141 handlers. The remaining Phase
  6 envelope stays 10-21 sessions, or 20-84 focused hours; the full remaining project envelope
  stays 66-105 sessions, or 132-420 focused hours. Next, select the highest-value
  dependency-complete combat or utility family while leaving pre-damage weapon and Acheron return
  lifecycle dependencies explicit.
