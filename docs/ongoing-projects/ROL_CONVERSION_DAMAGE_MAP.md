# Realms of Luminari Conversion Damage Map

Status: production Luminari flat-file world restored and boot-verified in development; isolated
RoL connection graph rebuilt and verified in a non-applied candidate; persistent-state repair,
shared-runtime mechanics isolation, and final RoL release/apply remain outstanding.

Date: 2026-08-14

Environment audited: development (`lib/.env` reports `APP_ENV=development`).

## Restoration checkpoint

On 2026-08-14, `lib/world` was replaced with a read-only pull of the complete production world.
Production was not modified. The replacement was staged and verified before the development server
was stopped and the directories were swapped.

Verification results:

- The production and staged trees each contained 3,804 regular files, and their complete SHA-256
  manifests matched exactly.
- The live post-boot tree still matches that production manifest exactly.
- Trail 1507, Hulburg 1591, Jotunheim 1960, and artifacts 169901-169910 are present at their
  original Luminari identities.
- The artifact runtime registry, provisioning script, CI fixture, distribution manifest, tests,
  and maintained documentation again use the original zone 1699 identities. Provisioning no
  longer deletes objects or resets in the 169901-169910 range.
- Converted zones 20507, 20591, and 20960 are absent.
- Every target named by the eight world indexes exists.
- A syntax-only full-world boot completed normally with 516 zones, 50,383 rooms, 14,660 mobiles,
  and 12,262 objects.
- The managed development server completed `Boot db -- DONE`, entered the game loop, and is
  listening on port 4100.

The damaged 5,000-file world was retained at both of these rollback locations:

```text
lib/rol-conversion/recovery-backups/pre-production-world-restore-20260814T155744Z/world
lib/rol-conversion/recovery-backups/pre-production-world-restore-20260814T155744Z/world-live-before-swap
```

The production, staged, restored, and post-boot manifests are retained under:

```text
lib/rol-conversion/recovery-staging/production-world-20260814T155744Z
```

This checkpoint restores the flat-file Luminari world and the tracked artifact runtime and
provisioning contract. The development database and player/house stores still contain
conversion-era persistent references, and the wider shared-runtime mechanics isolation audit is
still outstanding. The production world also contains legacy validation findings; for example,
the zone 1507 validator reports existing HLQ reference and object-payload errors. Those
production-source findings were recorded and were not changed during this restoration.

Unless explicitly stated otherwise, the forensic measurements below describe the damaged
5,000-file tree at audit time, which is now preserved at the rollback locations above.

## RoL connection repair checkpoint

On 2026-08-14, the active source under `EXAMPLE/RealmsOfLuminari/areas` was compared against the
preserved damaged converted world as a complete directed graph. The old conversion was missing ten
source exits, not only the four cross-package exits found by the initial audit:

| Package | Missing source exits |
|---|---:|
| Trail | 3 |
| Jotunheim | 7 |
| Total | 10 |

The damaged output also contained four exits absent from the active RoL source. These included a
Trail-to-Luminari bridge, a Waterdeep-derived low-VNUM bridge, the hardcoded substitution of
missing RoL room 96003 with a Luminari-only Jotun room, and a Jotun-to-Luminari bridge.

The root causes were removed from the Phase 7 generator:

- Trail, Hulburg, and Jotunheim are no longer treated as preconverted packages whose Luminari
  records should be preserved.
- The hardcoded 96003 -> 2096003 substitution was removed.
- A source room reference can no longer resolve through a target-only Luminari identity.
- Phase 7 now emits all distinct RoL records into the free zone 20000-29999 and entity
  2000000-2999999 namespaces.
- Phase 8 now requires the Phase 7 connection-graph gate to pass.

The repaired candidate and its byte-identical repeat are retained at:

```text
lib/rol-conversion/runs/phase7-graph-repair-20260814
lib/rol-conversion/runs/phase7-graph-repair-20260814-repeat
```

The candidate graph gate reports:

| Measurement | Result |
|---|---:|
| RoL room records mapped | 41,354 |
| Resolvable directed source exits expected/actual | 116,202 / 116,202 |
| Missing or extra directed exits | 0 / 0 |
| Resolvable portal and switch targets expected/actual | 302 / 302 |
| Missing or extra portal/switch targets | 0 / 0 |
| Typed RoL room references audited | 850 |
| Unmapped RoL namespace rooms | 0 |
| Physical or typed cross-world connections | 0 |
| Original-source exits to absent source rooms | 25 |
| Original-source portal targets to absent source rooms | 2 |

The 25 exit destinations and two portal destinations in the last two rows are already absent from
the active original RoL source graph. They are recorded as source defects and excluded; they are
not resolved through Luminari content.

All 71,680 selected source records are now disposed as 70,603 ADD, 1,068 MERGE, and 9 EXCLUDE.
There are zero KEEP or PATCH actions that reuse a Luminari baseline record. The combined staging
tree contains the untouched 3,804-file production Luminari baseline plus 1,206 RoL overlay files.
No baseline content file changes; only the seven relevant indexes change. The regenerated output
and repeat are byte-identical.

The combined candidate completes a full syntax boot with 764 zones, 91,737 rooms, 27,066 mobiles,
and 22,638 objects. All 414 world-tool tests pass. This candidate has not been applied to
`lib/world`. Mechanics isolation and persistent-state repair remain separate required gates before
a final release or live apply.

## Executive conclusion

The conversion violated the requested non-destructive import boundary. It did not place all Realms
of Luminari (RoL) content in a new, disjoint namespace while leaving the existing Luminari world
untouched. Instead, it made three existing Luminari zones and ten existing Luminari artifacts part
of a canonical RoL identity policy, retired their original VNUMs, rewrote their consumers, and
migrated persistent data to the replacement identities.

The required compatibility boundary is absolute: the two worlds have **zero compatibility**. Name,
theme, historical lineage, or superficially similar records do not authorize deduplication. The
recovered result must have separate identities, separate directed world graphs with zero edges
between them, and RoL-specific mechanics confined to RoL content. Original Luminari content and
mechanics must remain unchanged.

The direct answer to "how many original zones were removed?" is **three**:

1. Trail, zone 1507, rehomed to zone 20507.
2. Hulburg, zone 1591, rehomed to zone 20591.
3. Jotunheim, zone 1960, rehomed to zone 20960.

No other pre-conversion `.zon` package path was absent from the audit-time damaged tree. The three
areas were recoverable because their complete pre-conversion files remained in the retained Phase
3 staging snapshot. They were absent from their original live identities at audit time, and the
replacement files were not byte-for-byte copies. The production restoration has now returned all
three original packages to `lib/world`.

The broader damage is larger than three zone deletions:

- 20 pre-existing world files were deliberately removed from `lib/world`.
- 2,402 existing zone/room/mobile/object/trigger/high-level-quest identities in the three packages
  were retired and remapped. Ten artifact identities were also retired, for 2,412 retired original
  identities in total.
- Hulburg quest 159100 was left in `qst/1591.qst` after its zone was removed. Its mobile references
  were rebased, but the quest itself has no owning zone and cannot be edited safely through OLC.
- The imported world contains 530 new explicit zone-ownership defects across 47 packages: 387
  records have no owning zone and 143 are owned by a different zone than their file package.
- The two world graphs were welded at the three rehomed areas instead of preserved independently.
  Four of ten intended RoL cross-package exits around those areas are absent, while the original
  Luminari entrances were rewritten to enter the same composite nodes.
- Mechanically incompatible RoL records were reconciled against Luminari records and supported by
  changes across shared runtime subsystems. Any RoL behavior that is not strictly scoped to RoL
  identities can alter the original game and must be treated as damage until isolation is proven.
- The ten RoL object prototypes whose canonical destinations collided with the old Luminari
  artifacts were not preserved as distinct imported objects. The current prototypes are the old
  Luminari definitions, so ten distinct RoL definitions were suppressed.
- Nine additional RoL source records were explicitly excluded.
- Persistent references were rewritten in 1,512 development database rows and 1,063 serialized
  object headers across 112 player/house files. Artifact state was also migrated. A safe recovery
  therefore cannot be performed by copying three zone files back in isolation.

No world file, database row, or persistent object was changed during this audit.

## Audit boundary and authoritative evidence

The authoritative pre-conversion target-world baseline used for the original damage audit is:

```text
lib/rol-conversion/runs/phase3-a5419818-a/staging/world
```

That Phase 3 staging tree contains the complete target world before the canonical rebase. Its run
manifest records that the target tree was unchanged during that phase. At audit time, the damaged
tree was `lib/world`; it is now preserved in the restoration backup, while `lib/world` is the
production-restored Luminari tree described above.

The comparison covers every regular file in both world trees, all eight indexed world-data kinds,
the Phase 6.5 rehome/removal/persistence ledgers, the Phase 7 action ledger, the original RoL source
under `EXAMPLE/RealmsOfLuminari`, and the current OLC ownership rules.

File inventory result:

| Measurement | Count |
|---|---:|
| Files in pre-conversion world snapshot | 3,808 |
| Files in damaged world tree at audit time | 5,000 |
| Pre-conversion paths absent now | 20 |
| New paths relative to the baseline | 1,212 |
| Shared paths with changed bytes, including indexes/state | 25 |

The 20 absent paths exactly match the canonical rebase `removals.jsonl`; there are no additional
missing baseline paths hidden outside the indexes.

Indexed package counts changed as follows:

| Kind | Baseline | Current | Removed original package entries | Added package entries |
|---|---:|---:|---:|---:|
| Zone | 516 | 761 | 3 | 248 |
| Room | 517 | 761 | 3 | 247 |
| Mobile | 517 | 736 | 3 | 222 |
| Object | 516 | 705 | 3 | 192 |
| Shop | 509 | 577 | 3 | 71 |
| Trigger | 332 | 420 | 1 | 89 |
| Quest | 162 | 161 | 1 | 0 |
| High-level quest | 283 | 420 | 3 | 140 |

Thus 248 damaged-tree zone identities were new relative to the baseline, while exactly the three
original zone identities listed below disappeared. The production restoration has since returned
those original identities and removed the converted packages from the live flat-file world.

## Original Luminari world damage

### Removed files

These are the exact pre-existing files removed from the live world:

| Area | Removed files |
|---|---|
| Trail 1507 | `zon/1507.zon`, `wld/1507.wld`, `mob/1507.mob`, `obj/1507.obj`, `shp/1507.shp`, `hlq/1507.hlq` |
| Hulburg 1591 | `zon/1591.zon`, `wld/1591.wld`, `mob/1591.mob`, `obj/1591.obj`, `shp/1591.shp`, `hlq/1591.hlq` |
| Jotunheim 1960 | `zon/1960.zon`, `wld/1960.wld`, `mob/1960.mob`, `obj/1960.obj`, `shp/1960.shp`, `trg/1960.trg`, `qst/1960.qst`, `hlq/1960.hlq` |

The three shop files and Jotunheim quest file contain no records, but they were still original
package files. Trail had no trigger or quest file. Hulburg had no trigger file. Hulburg's non-empty
quest file was not removed and was not rehomed, which caused the orphan described below.

### Retired record identities and destination contents

| Area | Original records retired | Destination record counts | Result |
|---|---:|---:|---|
| Trail 1507 -> 20507 | 1 zone, 200 rooms, 151 mobiles, 1 object, 116 HLQs = 469 | Same counts, plus 70 new RoL triggers | Original identities absent; destination is a merge, not an untouched copy |
| Hulburg 1591 -> 20591 | 1 zone, 492 rooms, 400 mobiles, 268 objects, 277 HLQs = 1,438 | Same counts; no destination quest file | Original identities absent; quest 159100 remains orphaned at the retired namespace |
| Jotunheim 1960 -> 20960 | 1 zone, 298 rooms, 92 mobiles, 86 objects, 7 triggers, 11 HLQs = 495 | 1 zone, 298 rooms, 93 mobiles, 91 objects, 7 triggers, 11 HLQs | Original identities absent; one RoL mobile and five RoL objects were merged into the destination |
| Total | 2,402 identities | - | All 2,402 original identities were retired |

The record headers confirm that every mapped non-empty original record has a destination record.
That means the bulk of the three areas was rehomed rather than simply dropped. It does not make the
operation non-destructive: external identities, builder ownership, persistent consumers, and some
content changed.

### Rehomed content is not byte-equivalent

The following comparison first rewrote expected VNUM tokens using the conversion's own mappings,
then diffed the normalized old files against the destinations. "Hunks" are diff hunks, not record
counts.

| Area | Kind | Normalized result |
|---|---|---|
| Trail | zone | 3 hunks, 3 additions, 3 deletions |
| Trail | rooms | Exact after renumbering |
| Trail | mobiles | 73 hunks, 73 additions, 2 deletions |
| Trail | objects | 1 hunk, 1 addition, 1 deletion |
| Trail | shops | Exact |
| Trail | HLQs | 5 hunks, 5 additions, 9 deletions |
| Hulburg | zone | 99 hunks, 138 additions, 138 deletions |
| Hulburg | rooms | 2 hunks, 2 additions, 2 deletions |
| Hulburg | mobiles | 103 hunks, 103 additions, 30 deletions |
| Hulburg | objects | 25 hunks, 25 additions, 25 deletions |
| Hulburg | shops | Exact |
| Hulburg | quest | Destination missing |
| Hulburg | HLQs | 30 hunks, 21 additions, 86 deletions |
| Jotunheim | zone | 3 hunks, 3 additions, 3 deletions |
| Jotunheim | rooms | Exact after renumbering |
| Jotunheim | mobiles | 18 hunks, 34 additions, 10 deletions |
| Jotunheim | objects | 2 hunks, 42 additions, no deletions |
| Jotunheim | shops | Exact |
| Jotunheim | triggers | Exact after renumbering |
| Jotunheim | quest | Original file was empty; destination missing |
| Jotunheim | HLQs | 2 hunks, 2 additions, 2 deletions |

These changes include generated annotations, flag normalization, reset changes, and imported RoL
records. Each non-exact category needs content review during recovery; it must not be treated as a
pure mechanical rename.

### Orphaned Hulburg quest

`lib/world/qst/1591.qst` still contains quest 159100, "Red Eye! / Bring Gharrick Silkred's Hide."
The conversion changed its mobile references from 159228 to 2059228 but left the quest header at
159100. No current zone range owns 159100 because zone 1591 was removed.

Consequences:

- `qedit 159100` cannot find an owning zone.
- The quest package is still indexed under a retired namespace.
- Restoring zone 1591 without also deciding whether to reverse the two 2059228 references would
  produce a mixed-namespace quest.

### Ten original artifacts were rehomed

The canonical rebase removed object definitions 169901-169910 from `obj/1699.obj` and the mirrored
`artifacts/1699.obj`, rewrote the Vault of Ages resets, and migrated persistent state:

| Original Luminari VNUM | Current VNUM | Artifact |
|---:|---:|---|
| 169901 | 2001043 | Trorxek |
| 169902 | 2001044 | Amaukekel |
| 169903 | 2001042 | Fade |
| 169904 | 2001046 | Horn of Henekar |
| 169905 | 2001050 | Doombringer |
| 169906 | 2001007 | Kelrarin's Hammer |
| 169907 | 2001048 | Kelrom |
| 169908 | 2005343 | Gesen |
| 169909 | 2001008 | Tiamat's Stinger |
| 169910 | 2019730 | Avernus |

The current destination prototypes match the original Luminari definitions semantically after
accounting for VNUM/serialization changes. Thus the old Luminari artifact content survived, but its
ten identities did not. A second Kelrarin-related artifact was added at 2001009.

### Other pre-existing files changed in place

Excluding the eight index files, 17 shared pre-conversion paths have changed bytes. Thirteen are
ordinary indexed world-data files:

| File | Change relative to baseline |
|---|---|
| `zon/18.zon` | Object reference 159125 -> 2059125 |
| `zon/158.zon` | Removed reset loading nonexistent object 15802; this was a later Valgrind repair, not a Phase 6.5 rebase action |
| `zon/1204.zon` | Reset dependency flag changed from 0 to 1; this was a later Valgrind repair |
| `zon/1699.zon` | Ten artifact reset VNUMs replaced and a second Kelrarin hammer reset added |
| `wld/1290.wld` | Exit 196004 -> 2096004 |
| `wld/1557.wld` | Exits 159243 -> 2059243 and 159286 -> 2059286 |
| `wld/10000.wld` | Exit 150850 -> 2050850 |
| `obj/1699.obj` | Definitions 169901-169910 removed; surviving serialization also changed |
| `trg/0.trg` | `jotunheim` room variable 196004 -> 2096004 |
| `qst/347.qst` | Quest target 196052 -> 2096052 |
| `qst/1591.qst` | Mobile references 159228 -> 2059228 while quest 159100 remained unmoved |
| `hlq/1445.hlq` | Item 196009 -> 2096009 |
| `hlq/1451.hlq` | Item 159319 -> 2059319 |

The other four changed shared paths are artifact state/mirrors:

- `world.artifact`: ten state rows were migrated to new VNUMs and one unowned row was added; the
  current file also contains subsequent development activity and must not be replaced with the stale
  baseline wholesale.
- `artifacts/1699.zon`: mirrors the artifact reset changes.
- `artifacts/1699.obj`: removes the ten rehomed definitions and changes surviving serialization.
- `artifacts/artifacts.hlp`: adds text about two distinct artifacts sharing a tradition.

After the Phase 8 candidate was applied, `zon/20199.zon` was also manually changed during the
Valgrind follow-up (`D 1` -> `D 0` for its first door reset). It is the only current imported output
path whose bytes differ from the final Phase 8 candidate.

## RoL source-world loss and alteration

### 2,579 RoL records were conflated with the target baseline

The final action ledger marks 2,579 source records `PRESERVE_CANONICAL_BASELINE`: 2,419 KEEP and
160 PATCH. These records were not emitted as an independent RoL copy. Instead, the converter
declared them lineage-equivalent to existing target content, kept the Luminari baseline as the
canonical record, and optionally patched it.

| Source group | KEEP | PATCH | Total conflated |
|---|---:|---:|---:|
| Trail | 489 | 71 | 560 |
| Hulburg | 1,449 | 79 | 1,528 |
| Jotunheim | 470 | 10 | 480 |
| Artifact source packages | 11 | 0 | 11 |
| Total | 2,419 | 160 | 2,579 |

The 2,568 Trail/Hulburg/Jotunheim rows cover 979 rooms, 641 mobiles, 404 quests, 359 objects, 168
SOC records, 14 shops, and 3 zones. Their apparent lineage is irrelevant because the worlds and
their mechanics are incompatible. None of these records was eligible for reuse or deduplication.
There is no separate live RoL version to compare, edit, or retain. The destination packages are
invalid composites of the old Luminari baseline, RoL patches, compiled SOC triggers, and added
source records.

The original RoL source files remain under `EXAMPLE/RealmsOfLuminari`, so these versions can be
regenerated. The current world alone is not a faithful standalone copy of either input world.

### Ten collided RoL artifacts were suppressed

The ten destination VNUMs selected for the old Luminari artifacts were already the canonical
destinations of ten distinct RoL source objects. Direct source/current comparison shows that every
pair differs in names/descriptions and in 14-16 parsed semantic fields, including flags, values,
affects, wear rules, weight, cost, or item type.

| RoL source VNUM | Canonical/current VNUM | RoL source identity | Current identity |
|---:|---:|---|---|
| 1007 | 2001007 | mystical warhammer of the barbarian kings | Kelrarin's Hammer |
| 1008 | 2001008 | Tiamat's poison stinger | Tiamat's Stinger (different prototype) |
| 1042 | 2001042 | Fade, enchanted drusus / bringer of Revenge | Fade, the Shadowblade |
| 1043 | 2001043 | Trorxek, living staff of ancient oaks | Trorxek, Staff of Ancient Oaks (different prototype) |
| 1044 | 2001044 | Amaukekel, holy Rod of Resurrection | Amaukekel, Rod of Light |
| 1046 | 2001046 | Horn of Henekar, relic of lost Alteria | Horn of Henekar (different prototype) |
| 1048 | 2001048 | ancient war axe of Kelrom | Kelrom, Axe of Pahluruk (different prototype) |
| 1050 | 2001050 | Doombringer, mystical sword of Eternal Chaos | Doombringer (different prototype) |
| 5343 | 2005343 | battleaxe of the barbarian warlords | Gesen, the Returning Axe |
| 19730 | 2019730 | Avernus, life stealer of Arex the Great | Avernus, the Black Blade |

This is loss from the imported RoL world even though the action ledger labels the destinations as
preserved canonical baseline. Both source objects should have received distinct identities in a
non-destructive import.

### Nine explicitly excluded RoL records

The final Phase 7 ledger accounts for 71,680 selected records as 68,112 ADD, 2,419 KEEP, 980 MERGE,
160 PATCH, and 9 EXCLUDE. The nine exclusions are:

| Kind | Source package and VNUM | Recorded reason/evidence |
|---|---|---|
| Mobile | `llyrath` 51348 | Source mobile lacks its position row |
| Object | `quest_1` 7067 | Action description is omitted; ledger chose smallest-unit exclusion |
| Object | `muspel` 59060 | Incomplete string block and missing flags row |
| Quest | `moonshae` 26253 | Incomplete quest or missing `S` terminator |
| Quest | `moonshae` 26254 | Incomplete quest or missing `S` terminator |
| SOC | `bs3` 22496, source line 1 | Referenced mobile 22496 has no typed identity |
| SOC | `bs3` 22496, source line 72 | Referenced mobile 22496 has no typed identity |
| SOC | `bandits` 89128 | Referenced mobile 89128 has no typed identity |
| Room | `quests` 1000 | Split direction opcode could not be parsed by the source loader |

The three SOC exclusions also caused dependent instructions/resets to be excluded where the absent
mobile identities were referenced. The ledgers classify these as owned exclusions rather than
unaccounted records, but the content is absent from the imported world.

### Source zone packaging was consolidated

The source had 255 zone records. The final world action ledger added 241 zones, kept 3 (the three
destructive rehomes), and merged 11 source zone records into four destination zones:

- `calship1`, `calship2`, `hyskship`, `hyskship2`, and `pirateisleship` -> zone 20466.
- `bgdruids` and `gtower` -> zone 20509.
- `mirar_ferry` and `northern_highroad` -> zone 20903.
- `baldurs-dancer` and `baldurs-harbor` -> zone 20919.

This consolidation is not proven record loss, but it is another departure from a simple
package-preserving import and must be reviewed when assigning the clean replacement namespace.

## World-graph topology damage

The correct import operation is an injective rename of the RoL graph into unused VNUMs. In graph
terms, the result must contain the unchanged Luminari graph and a separately renamed RoL graph as
two disconnected components. There must be no bridge exits, portals, scripted transfers, shared
rooms, or other cross-world traversal edges.

The conversion instead identified the old and new Trail, Hulburg, and Jotunheim nodes as the same
nodes. It then rewrote original Luminari edges and selected RoL edges to those composite nodes. This
formed a quotient graph, not a non-destructive union. A destination VNUM can therefore resolve
successfully while the source-world path it represents has been changed or lost.

### Original Luminari entrances were moved into composite nodes

Examples of original connections rewritten in place:

| Original Luminari edge | Current edge |
|---|---|
| 1000264 <-> 150850 (Stojanow Crossroads / Trail) | 1000264 <-> 2050850 |
| 129018 <-> 196004 (Yggdrasil / Jotunheim) | 129018 <-> 2096004 |
| 150700 <-> 159297 (Trail / Hulburg) | 2050700 <-> 2059297 |
| 155745 -> 159243 | 155745 -> 2059243 |
| 155773 -> 159286 | 155773 -> 2059286 |

The first two pairs now connect the old Luminari world directly into packages that also receive RoL
connections. The Trail/Hulburg pair is worse conceptually: the old Luminari pair and the equivalent
RoL source pair were collapsed into one pair of exits, so neither graph has an independent copy.

### Audit-time cross-package review found four missing RoL boundary exits

The source reference ledger contains ten directed cross-package RoL exits involving Trail,
Hulburg, or Jotunheim. After applying the final canonical mapping, the current world contains only
six of them:

| RoL source edge | Expected current edge | Current result |
|---|---|---|
| `flesh` 45100 -> `trail` 50863 | 2045100 -> 2050863 | Present |
| `trail` 50863 -> `flesh` 45100 | 2050863 -> 2045100 | **Missing** |
| `trail` 50700 -> `hulburg` 59297 | 2050700 -> 2059297 | Present, but conflated with old Luminari edge |
| `hulburg` 59297 -> `trail` 50700 | 2059297 -> 2050700 | Present, but conflated with old Luminari edge |
| `trail` 50899 -> `zhentil` 81232 | 2050899 -> 2081232 | **Missing; current destination is -1** |
| `zhentil` 81232 -> `trail` 50899 | 2081232 -> 2050899 | Present |
| `muspel` 58604 -> `jotun` 96073 | 2058604 -> 2096073 | Present |
| `jotun` 96073 -> `muspel` 58604 | 2096073 -> 2058604 | **Missing** |
| `yggdrasil` 62804 -> `jotun` 96010 | 2062804 -> 2096010 | Present |
| `jotun` 96010 -> `yggdrasil` 62804 | 2096010 -> 2062804 | **Missing** |

This is direct RoL topology loss, not merely an unwanted extra connection. Four originally
bidirectional package relationships became one-way because KEEP preserved the Luminari room version
without merging the RoL outbound exit, while the other package's converted inbound exit was added.

The complete repair audit later found six additional missing intra-package exits and four extra
non-source exits. The repaired candidate described above restores all ten missing source exits and
removes all four extras.

There is also a source edge 58601 -> 96003 that became 2058601 -> 2096003 even though the selected
RoL Jotun room corpus has no independent room 96003 action. It resolves by landing on an old
Luminari-only Jotun room, another example of source topology being satisfied numerically through
target substitution.

### Why the graph gate passed

The acceptance evidence checked that selected destination rooms exist and that a small set of
walkthroughs/cross-zone exits resolve. It did not compare the full directed edge set of each input
graph against the output under an injective mapping. Consequently, the gate accepted one-way edges,
conflated duplicate edges, and forbidden cross-world edges into composite nodes as long as their
numeric targets loaded.

## New zone-ownership and OLC damage

### Counts

OLC determines ownership from each zone's explicit bottom/top room range, not from an object's file
name. A full indexed-model comparison found:

| World | Ownership defects | Multi-owner | Zero-owner | Package mismatch |
|---|---:|---:|---:|---:|
| Pre-conversion baseline | 15 | 13 | 1 | 1 |
| Current world | 545 | 13 | 388 | 144 |
| Newly introduced | 530 | 0 | 387 | 143 |

The 530 new defects affect 47 packages:

| Type | New defects |
|---|---:|
| Mobile | 83 |
| Object | 371 |
| Trigger | 51 |
| Shop | 3 |
| Quest | 1 |
| High-level quest | 21 |
| Room | 0 |

The reported scepter is one of these. Object 2019912 is stored in `obj/20199.obj` and reset by
`zon/20199.zon`, but zone 20198 ends at 2019910 and zone 20199 begins at 2019928. VNUM 2019912 lies
in the gap, so `oedit 2019912` correctly reports that no zone owns it.

For a zero-owner record, OLC refuses to enter the editor. For a package mismatch, OLC selects the
other zone; range-based save functions can then write a different package and split, duplicate, or
drop records on reboot. Until recovery, do not use OLC save operations on the affected packages.

### Complete new-defect map

| Package | Type | Failure | Runtime owner | VNUMs | Count |
|---:|---|---|---|---|---:|
| 1591 | quest | zero owner | none | 159100 | 1 |
| 20001 | mobile | package mismatch | 20009 | 2000900-2000910 | 11 |
| 20010 | object | zero owner | none | 2001000 | 1 |
| 20010 | trigger | zero owner | none | 2001000 | 1 |
| 20013 | object | zero owner | none | 2001300-2001305 | 6 |
| 20014 | mobile | zero owner | none | 2001400 | 1 |
| 20014 | object | zero owner | none | 2001400 | 1 |
| 20030 | mobile | zero owner | none | 2003000 | 1 |
| 20030 | object | zero owner | none | 2003000 | 1 |
| 20030 | shop | zero owner | none | 2003000 | 1 |
| 20040 | mobile | zero owner | none | 2004000 | 1 |
| 20040 | object | zero owner | none | 2004200, 2004205, 2004210, 2004220, 2004230, 2004240, 2004250, 2004260, 2004270, 2004280, 2004290, 2004300, 2004310, 2004320, 2004330, 2004340, 2004350, 2004360, 2004370, 2004374-2004376 | 22 |
| 20044 | mobile | zero owner | none | 2004400 | 1 |
| 20044 | object | package mismatch | 20048 | 2004750, 2004760, 2004770, 2004780-2004786, 2004790-2004799 | 20 |
| 20044 | object | zero owner | none | 2004400, 2004700, 2004705, 2004708, 2004710, 2004720, 2004730, 2004740 | 8 |
| 20049 | mobile | package mismatch | 20048 | 2004817-2004818, 2004820-2004834 | 17 |
| 20050 | mobile | zero owner | none | 2005000 | 1 |
| 20050 | object | zero owner | none | 2005000 | 1 |
| 20070 | object | zero owner | none | 2007066, 2007068-2007099 | 33 |
| 20080 | mobile | zero owner | none | 2008000 | 1 |
| 20080 | object | zero owner | none | 2008000 | 1 |
| 20080 | trigger | zero owner | none | 2008000 | 1 |
| 20098 | object | zero owner | none | 2009800-2009899 | 100 |
| 20099 | object | zero owner | none | 2009900-2009997 | 98 |
| 20103 | object | zero owner | none | 2010500, 2010599 | 2 |
| 20121 | mobile | package mismatch | 20120 | 2012100 | 1 |
| 20143 | trigger | zero owner | none | 2014300 | 1 |
| 20166 | trigger | zero owner | none | 2016600 | 1 |
| 20197 | mobile | package mismatch | 20198 | 2019830, 2019840, 2019850, 2019870, 2019880, 2019885 | 6 |
| 20197 | mobile | zero owner | none | 2019700 | 1 |
| 20197 | object | package mismatch | 20198 | 2019830, 2019835, 2019840, 2019850, 2019855-2019857, 2019860-2019879, 2019881, 2019885-2019890 | 34 |
| 20197 | object | zero owner | none | 2019700 | 1 |
| 20197 | trigger | zero owner | none | 2019700 | 1 |
| 20199 | mobile | package mismatch | 20198 | 2019900, 2019910 | 2 |
| 20199 | mobile | zero owner | none | 2019920-2019921 | 2 |
| 20199 | object | package mismatch | 20198 | 2019900-2019906, 2019910 | 8 |
| 20199 | object | zero owner | none | 2019912-2019915, 2019917-2019922, 2019924, 2019926-2019927 | 13 |
| 20204 | object | zero owner | none | 2020400 | 1 |
| 20213 | trigger | package mismatch | 20210 | 2021300-2021305 | 6 |
| 20227 | mobile | zero owner | none | 2022700 | 1 |
| 20227 | object | zero owner | none | 2022700 | 1 |
| 20244 | mobile | zero owner | none | 2024400 | 1 |
| 20244 | object | zero owner | none | 2024400 | 1 |
| 20250 | mobile | zero owner | none | 2025000 | 1 |
| 20251 | mobile | package mismatch | 20250 | 2025100 | 1 |
| 20251 | mobile | zero owner | none | 2025110, 2025120, 2025130, 2025140, 2025150, 2025160, 2025170, 2025180, 2025190, 2025200 | 10 |
| 20251 | object | package mismatch | 20250 | 2025101 | 1 |
| 20251 | object | zero owner | none | 2025110, 2025120, 2025130, 2025140, 2025150, 2025160, 2025170, 2025180, 2025190-2025191 | 10 |
| 20254 | mobile | zero owner | none | 2025400 | 1 |
| 20254 | object | zero owner | none | 2025400 | 1 |
| 20254 | trigger | zero owner | none | 2025400 | 1 |
| 20260 | trigger | zero owner | none | 2026000 | 1 |
| 20264 | mobile | zero owner | none | 2026400 | 1 |
| 20264 | object | zero owner | none | 2026400 | 1 |
| 20380 | high-level quest | zero owner | none | 2038000 | 1 |
| 20380 | mobile | zero owner | none | 2038000 | 1 |
| 20385 | mobile | zero owner | none | 2038500 | 1 |
| 20402 | high-level quest | zero owner | none | 2040200 | 1 |
| 20402 | mobile | zero owner | none | 2040200 | 1 |
| 20402 | object | zero owner | none | 2040200 | 1 |
| 20405 | high-level quest | zero owner | none | 2040531-2040540 | 10 |
| 20405 | mobile | zero owner | none | 2040531-2040542 | 12 |
| 20409 | trigger | zero owner | none | 2040900 | 1 |
| 20429 | high-level quest | zero owner | none | 2042998-2042999 | 2 |
| 20429 | mobile | zero owner | none | 2042998-2042999 | 2 |
| 20433 | trigger | zero owner | none | 2043300 | 1 |
| 20436 | trigger | zero owner | none | 2043600 | 1 |
| 20437 | trigger | zero owner | none | 2043700 | 1 |
| 20440 | trigger | zero owner | none | 2044000 | 1 |
| 20523 | trigger | zero owner | none | 2052300 | 1 |
| 20528 | mobile | zero owner | none | 2052800 | 1 |
| 20528 | object | zero owner | none | 2052800 | 1 |
| 20528 | trigger | zero owner | none | 2052800 | 1 |
| 20532 | high-level quest | package mismatch | 20533 | 2053334, 2053342-2053344, 2053352, 2053354 | 6 |
| 20532 | object | zero owner | none | 2053200 | 1 |
| 20532 | shop | package mismatch | 20533 | 2053343, 2053356 | 2 |
| 20532 | trigger | zero owner | none | 2053200 | 1 |
| 20533 | trigger | package mismatch | 20532 | 2053300-2053327 | 28 |
| 20598 | mobile | zero owner | none | 2059800 | 1 |
| 20598 | trigger | zero owner | none | 2059800 | 1 |
| 20800 | high-level quest | zero owner | none | 2080000 | 1 |
| 20800 | mobile | zero owner | none | 2080000 | 1 |
| 20800 | object | zero owner | none | 2080000 | 1 |
| 20926 | trigger | zero owner | none | 2092600 | 1 |
| 20970 | mobile | zero owner | none | 2097000 | 1 |
| 20970 | object | zero owner | none | 2097000 | 1 |

### Why the validation gates missed this

The world validator's packaging check emits a warning only when exactly one owner exists and that
owner differs from the file package. It does not report zero owners for mobiles, objects, triggers,
shops, quests, or high-level quests. Room ownership has a separate exact-one check, which is why the
conversion introduced no new room ownership defects while hundreds of other records passed the
acceptance gate.

The Phase 8 acceptance statement "namespace audit pass" therefore did not establish OLC ownership
integrity.

## Persistent-state mutation and recovery coupling

The Phase 6.5 persistence execution was run against the development database. Its own before/after
reports show 1,512 retired-reference rows changed to canonical references:

| Database table | Rows rewritten |
|---|---:|
| `player_save_objs` | 1,114 |
| `house_data` | 398 |
| Total | 1,512 |

The file migration report separately records 1,063 object prototype headers rewritten across 112
serialized stores:

| Store | Files | Headers rewritten |
|---|---:|---:|
| Player object files | 104 | 668 |
| House object files | 8 | 395 |
| Total | 112 | 1,063 |

The difference between database row counts and serialized header counts is expected because a row
can contain serialized object data and the reports count different units. Both scopes must be
handled in recovery.

Artifact persistence changed as follows at migration time:

- Ten old artifact-state rows were moved to the rehomed VNUMs.
- One unowned state row was added for the distinct 2001009 Kelrarin artifact.
- No state clones were recorded.
- The migration output preserved the old ownership/progression fields at cutover.

The current `world.artifact` has subsequently changed through development activity. Recovery must
reverse identities against the latest state, not restore the August 7 baseline file and erase later
claims/progression.

## Cause and policy failure

This was not an accidental file-copy collision. The behavior was explicitly encoded:

- `scripts/world/wtool_lib/rol_rebase.py` defines `_CORE_PACKAGES` that moves 1507 -> 20507,
  1591 -> 20591, and 1960 -> 20960.
- The same module defines `_ARTIFACT_REHOMES` for objects 169901-169910.
- The generated removal/rehome ledgers then applied those policies to live development data.
- Permanent documentation says the old identities are retired migration history and explicitly
  says not to restore aliases.
- The testing guide makes acceptance conditional on Trail, Hulburg, and Jotunheim existing only at
  the canonical RoL identities.

The planner treated these pre-existing areas as prior-lineage copies of RoL source content and
optimized for one canonical identity. That design objective overrode the actual user requirement:
preserve the entire existing Luminari world and import RoL into a clean incremental namespace.

The correct decision at the collision boundary was to retain every existing Luminari identity and
assign every RoL record a new disjoint identity. No RoL record was eligible to be skipped as a
duplicate or substituted with a Luminari record. Existing content should never have been rehomed to
make the source formula work.

## Tracked code rollback surface

The conversion was not isolated to ignored world files. Between the pre-conversion parent
`ce4537c1` and the Phase 8 merge `05c5fea3`, the tracked tree changed 327 files with 118,345
insertions and 725 deletions. That commit window includes some interleaved non-conversion work, so
these figures are an upper bound rather than a clean damage count.

Within `src/`, 92 files changed (30,580 insertions and 311 deletions): 69 existing runtime files
were modified and 23 RoL runtime files were added. The touched existing subsystems include combat,
movement, magic, class/feat handling, DB loading, DG scripts, object/shop/artifact behavior, OLC,
quests, special dispatch, and vessels. The added files are the `spec_rol_*` handlers and
`vessels_rol.*`.

Some runtime code may be needed to emulate or translate RoL mechanics, but it is valid only when it
is strictly scoped to RoL identities. Because the conversion modified shared combat, movement,
magic, object, quest, OLC, and dispatch systems, every such path is part of the damage audit until
tests prove that original Luminari content behaves exactly as it did before the import. A blanket
Git revert would also discard unrelated interleaved fixes, so recovery must isolate or remove RoL
behavior without reverting unrelated work.

## Required recovery sequence

This section records dependencies, not authorization to apply changes.

The production-world checkpoint above completes the live flat-file restoration portion of this
sequence. It does not complete persistent-state repair, mechanics isolation, or the clean RoL
reimport.

1. Freeze and copy the current development world, player/house stores, database, and
   `world.artifact` before any repair. The baseline snapshots are recovery sources, not substitutes
   for a current backup.
2. Restore Trail 1507, Hulburg 1591, Jotunheim 1960, and artifacts 169901-169910 from the Phase 3
   baseline, then reapply only independently approved post-baseline fixes.
3. Reverse every current consumer listed in this report and the Phase 6.5 reference/persistence
   ledgers. Resolve Hulburg quest 159100 as part of this step.
4. Allocate a genuinely disjoint RoL namespace based on the complete existing Luminari inventory,
   not a formula that claims existing identities. Preserve all distinct RoL artifacts under their
   own new VNUMs.
5. Rebuild the RoL conversion into that namespace. Repair and import the nine excluded records or
   document an explicit content decision for each one. Scope every RoL-specific mechanic to RoL
   identities; it must never select, patch, or alter an original Luminari record.
6. Compare the complete directed exit graph of each input world with the rebuilt output. Every
   original Luminari edge must remain unchanged; every RoL edge must have exactly its mapped
   counterpart. Assert that the two graphs are disconnected and that no exit, portal, script,
   teleport, reset, quest, or special-procedure transfer crosses between them.
7. Give every room/mobile/object/trigger/shop/quest/HLQ exactly one explicit owning zone and make
   its package agree with that owner. Add a validator gate that rejects both zero-owner and
   multi-owner records for every type, not rooms alone.
8. Migrate the latest persistent state from the bad canonical identities to the final identities
   transactionally. Preserve post-cutover ownership, artifact progression, inventories, house
   contents, and serialized suffixes.
9. Run full world validation, OLC edit/save/reboot round trips for every affected package, root
   production-linked tests, install, and a bounded full-world boot before considering recovery
   complete.

Do not hand-edit a few reported VNUMs and declare recovery complete. The ownership table, artifact
collisions, source exclusions, cross-zone consumers, serialized stores, and database rows are one
coupled migration problem.

## Evidence ledger

Primary retained evidence:

- Production-restored world: `lib/world`
- Damaged-world rollback:
  `lib/rol-conversion/recovery-backups/pre-production-world-restore-20260814T155744Z`
- Production and restoration manifests:
  `lib/rol-conversion/recovery-staging/production-world-20260814T155744Z`
- Baseline world: `lib/rol-conversion/runs/phase3-a5419818-a/staging/world`
- Removal ledger: `lib/rol-conversion/runs/phase6-5-canonical-20260814-release3-a/removals.jsonl`
- Rehome ledger: `lib/rol-conversion/runs/phase6-5-canonical-20260814-release3-a/rehome.jsonl`
- Persistence report: `lib/rol-conversion/runs/phase6-5-canonical-20260814-release3-a/persistence-report.json`
- Persistence execution audit:
  `lib/rol-conversion/runs/phase6-5-completion-20260814-final/persistence-development-execution`
- Phase 7 action ledger: `lib/rol-conversion/runs/phase7-final-20260814/action-ledger.jsonl`
- Phase 7 source diagnostics: `lib/rol-conversion/runs/phase7-final-20260814/diagnostics/source.json`
- Phase 7 reference exceptions:
  `lib/rol-conversion/runs/phase7-final-20260814/reference-exceptions.jsonl`
- Final candidate: `lib/rol-conversion/runs/phase8-release-20260814/output/world`
- RoL source: `EXAMPLE/RealmsOfLuminari/areas`

The world data is ignored by Git, so the Phase 3 snapshot and conversion ledgers are the practical
recovery record. They must be retained until the repaired world and all persistent consumers have
been independently verified.
