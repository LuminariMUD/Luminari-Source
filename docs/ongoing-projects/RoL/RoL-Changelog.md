# Realms of Luminari Project Changelog

Completed entries through Phase 6 are preserved in the
[Phase 6 changelog archive](changelog-archive/archive08_13-phase6-complete-RoL-Changelog.md).

Record completed milestones for the next conversion batch here. Keep forward-looking
requirements and acceptance gates in
[the canonical conversion plan](REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md).

## Unreleased

### 2026-08-14 - Phase 8 final integration complete

- Applied release run `rol-phase8-release-5992b9c59dd3055e` to the development world.
  Its hash-guarded plan changed all 1,201 expected paths and produced tree
  `39c05c7427b941e715491d129d83b17b73f89c332219ec577ea5bf2dc4662b20`.
- Reconciled all 258 active packages and 71,680 records with zero new active error,
  zero converted-zone runtime diagnostic, clean namespace and action audits, passing
  preservation/runtime contracts, and byte-identical repeat generation.
- Added complete behavior evidence for 761 zones, 90,722 rooms, 26,427 mobiles,
  22,273 objects, 1,148 shops, 6,296 HLQs, 3,216 triggers, 115,074 resets, trigger
  attachments, traps, paths, containers, keyed exits, specials, and shops.
- Added typed composite special-procedure bindings for the 14 source multi-binding
  profiles and removed their conflicting hardwired assignments.
- Hardened source-object normalization for omitted action descriptions and misplaced
  extra descriptions, supplied runtime-safe object descriptions, repaired invalid
  charge maxima, and fixed failed object-reset dependency propagation.
- Passed 409 world-tool tests, all 699 production-linked CuTests, install, syntax boot,
  and bounded private-MariaDB runtime boot. Repeat application performs zero writes.

### 2026-08-14 - Phase 7 canonical corpus conversion complete

- Converted the active corpus in 12 dependency-complete batches, with sealed milestone
  checkpoints after batches 4 and 8.
- Sealed final run `rol-phase7-b12-a20bbc98e3513f98` for all 258 packages and 71,680
  records, including 1,228 SOC triggers, 14 composite profiles, and 160 patched
  records, with terminal actions and resolved required references throughout.
- Regenerated the final cumulative output independently in
  `phase7-final-repeat-20260814`; the complete trees are byte-identical.
- Closed Phase 7 with zero staged new active error, no live target writes, and passing
  source-parse, runtime-contract, preservation, special-binding, and SOC gates.

### 2026-08-14 - Phase 6.5 canonical VNUM rebase complete

- Enforced the universal zone `+20000` and entity `+2000000` resolver, including the
  evidence-backed `mytheast` normalization to zone `20817` and entities
  `2081700-2081899`.
- Rehomed Trail, Hulburg, and Jotunheim to zones `20507`, `20591`, and `20960`, rewrote
  8,768 world references, and applied 270 evidence-backed static repairs without a new
  normalized validator finding or touched blocker.
- Rehomed ten first-wave artifacts, restored the distinct Kelrarin object at `2001009`,
  and split canonical prototypes across packages `20010`, `20053`, and `20197` while
  retaining the artifact vault in zone `1699`.
- Migrated 112 player-object and house-object files, all 18 artifact-state rows, and
  1,512 development-database rows. The semantic ledger classifies all 83 database
  bindings, including 53 migration-required bindings. No retired persistent identity
  remains and no ownership or progression state was cloned.
- Hardened application with complete hash preflight, rollback-only SQL preflight,
  base-table guards for MariaDB views, database-first ordering, and repeatable
  zero-write file application. Both sealed release trees are byte-identical.
- Applied run `rol-phase6-5-a11f8a8181c2dd49` to development. The live namespace and
  reference audit is zero across all four exit invariants; global validation improved
  from 3,849 errors / 38,219 warnings to 3,770 / 38,028.
- Added a sealed record-level completion audit for 1,994 rehome/normalization records,
  per-package reference closure, every classified non-world numeric match, all 142
  distinct canonical saved-object prototypes, and 191 explicit Phase 6.5 requirements.
- Passed 396 world-tool tests, 698 production-linked CuTests, build/install, syntax
  boot, a disposable private-MariaDB behavioral boot, all four eligible reset
  observations, and scripted traversal of all 990 rehomed rooms across 16 components.
