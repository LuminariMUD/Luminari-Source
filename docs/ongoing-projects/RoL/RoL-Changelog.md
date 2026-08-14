# Realms of Luminari Project Changelog

Completed entries through Phase 6 are preserved in the
[Phase 6 changelog archive](changelog-archive/archive08_13-phase6-complete-RoL-Changelog.md).

Record completed milestones for the next conversion batch here. Keep forward-looking
requirements and acceptance gates in
[the canonical conversion plan](REALMS_OF_LUMINARI_CANONICAL_CONVERSION_PLAN.md).

## Unreleased

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
