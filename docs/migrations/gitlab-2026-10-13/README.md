# GitLab migration - 2026-10-13

This archive accompanies the migration from [max757/Luminari-Source on GitLab](https://gitlab.com/max757/Luminari-Source) to [LuminariMUD/Luminari-Source on GitHub](https://github.com/LuminariMUD/Luminari-Source).

## Scope and preservation

- Merge the 60 GitLab-only commits into GitHub master, preserving their original commit IDs and GitHub's Sponsors-file commit. No history is rewritten.
- Copy all 17 non-default source branches and all 38 original tags with identical object IDs. The source master is an ancestor of the migrated GitHub master.
- Import all 34 issues, their 38 non-system comments, labels, assignees, and open/closed states. The 17 open and 17 closed issues remain in those states.
- Preserve all 30 completed merge requests, 168 non-system review comments, system activity, thread resolution, line positions, and 71 diff-version records in this archive. All 101 review anchor commits are retained in the transferred Git history.
- Preserve original authors and timestamps as provenance. GitHub's API creates new issue/comment timestamps; it cannot recreate the original GitLab authorship or historical merged-PR status.
- Keep the original GitLab project and its records intact as a recovery source. The hint correction remains in the separate, unmerged GitHub PR #234.

## Project settings and empty components

The source's protected-master policy is carried to GitHub: force pushes and deletion are disabled, and existing GitHub administrators/maintainers may push. Existing GitHub collaborators and Actions workflows are preserved. Local default pushes and master tracking use origin (GitHub).

The source Bug label maps to GitHub's existing bug label; Feature retains the source color. Original label metadata remains in the records. The source has one empty Development board with no custom lists; its metadata is archived, and migrated work is available through GitHub Issues.

The source inventory contains no milestones, releases, wiki pages, snippets, uploads, packages, container repositories, CI pipelines, schedules, variables, webhooks, active integrations, or deploy keys. No Git LFS rules were found in the repository's attribute history.

## Validation

- The merged code passed the root CuTest suite: 2,059 tests.
- The help-sync checks skipped 10 opt-in isolated MariaDB integration cases; those cases were not exercised by this migration run.
- The repository build and installed pre-push checks passed.
- make install completed and removed the root-level server executable.
- This migration does not deploy to production or merge the separate hint PR.
- Machine-readable source snapshots, original reference inventories, ID mappings, and canonical record checksums are in [manifest.json](manifest.json) and [records](records/).

## Issues

- [GitLab #1](issues/1.md) -> [GitHub #235](https://github.com/LuminariMUD/Luminari-Source/issues/235) (closed): Price Extra Arms and the monk third hand after the arm count
- [GitLab #2](issues/2.md) -> [GitHub #236](https://github.com/LuminariMUD/Luminari-Source/issues/236) (closed): Add the 17 Duris player races
- [GitLab #3](issues/3.md) -> [GitHub #237](https://github.com/LuminariMUD/Luminari-Source/issues/237) (opened): Add the quests that grant the five Duris descend forms
- [GitLab #4](issues/4.md) -> [GitHub #238](https://github.com/LuminariMUD/Luminari-Source/issues/238) (closed): Artificer device add rejects 3rd and 4th circle spells
- [GitLab #5](issues/5.md) -> [GitHub #239](https://github.com/LuminariMUD/Luminari-Source/issues/239) (closed): Grant saved characters the single-rank racial feats their race gained later
- [GitLab #6](issues/6.md) -> [GitHub #240](https://github.com/LuminariMUD/Luminari-Source/issues/240) (opened): Publish the artificer help from #4 to production
- [GitLab #7](issues/7.md) -> [GitHub #241](https://github.com/LuminariMUD/Luminari-Source/issues/241) (opened): Read-only device subcommands should not need a standard action
- [GitLab #8](issues/8.md) -> [GitHub #242](https://github.com/LuminariMUD/Luminari-Source/issues/242) (opened): Devices accept entries that need an object or a typed argument
- [GitLab #9](issues/9.md) -> [GitHub #243](https://github.com/LuminariMUD/Luminari-Source/issues/243) (opened): Artificer device feat text describes temporary devices
- [GitLab #10](issues/10.md) -> [GitHub #244](https://github.com/LuminariMUD/Luminari-Source/issues/244) (closed): Cargo buy and sell move gold without a checked save
- [GitLab #11](issues/11.md) -> [GitHub #245](https://github.com/LuminariMUD/Luminari-Source/issues/245) (closed): Vessel waypoints and routes have no owner: any captain can delete another's idle routes
- [GitLab #12](issues/12.md) -> [GitHub #246](https://github.com/LuminariMUD/Luminari-Source/issues/246) (closed): Vessel settlements: two-phase gold and cargo records across a crash
- [GitLab #13](issues/13.md) -> [GitHub #247](https://github.com/LuminariMUD/Luminari-Source/issues/247) (closed): Database writes can land outside their transaction after an auto-reconnect
- [GitLab #14](issues/14.md) -> [GitHub #248](https://github.com/LuminariMUD/Luminari-Source/issues/248) (closed): Vessel purchases and payouts move gold without a checked save
- [GitLab #15](issues/15.md) -> [GitHub #249](https://github.com/LuminariMUD/Luminari-Source/issues/249) (closed): An object with no prototype leaks its arcane mark when freed
- [GitLab #16](issues/16.md) -> [GitHub #250](https://github.com/LuminariMUD/Luminari-Source/issues/250) (closed): Bagged objects stay in memory when their owner leaves the game
- [GitLab #17](issues/17.md) -> [GitHub #251](https://github.com/LuminariMUD/Luminari-Source/issues/251) (closed): A loaded sheath loses its weapons on a pet and leaks them when extracted
- [GitLab #18](issues/18.md) -> [GitHub #252](https://github.com/LuminariMUD/Luminari-Source/issues/252) (closed): Pet legacy-migration test fails on a database built from master_schema.sql alone
- [GitLab #19](issues/19.md) -> [GitHub #253](https://github.com/LuminariMUD/Luminari-Source/issues/253) (closed): A saved special ability without a command word is rejected at load
- [GitLab #20](issues/20.md) -> [GitHub #254](https://github.com/LuminariMUD/Luminari-Source/issues/254) (closed): Close the vessel release gates: player-data balance, human beta, staged rollout
- [GitLab #21](issues/21.md) -> [GitHub #255](https://github.com/LuminariMUD/Luminari-Source/issues/255) (closed): Vessel scale gate has no current threshold or measurement
- [GitLab #22](issues/22.md) -> [GitHub #256](https://github.com/LuminariMUD/Luminari-Source/issues/256) (opened): Staff recovery for vessel records the game could not put right
- [GitLab #23](issues/23.md) -> [GitHub #257](https://github.com/LuminariMUD/Luminari-Source/issues/257) (opened): Test gaps left by the vessel steps S9-S15
- [GitLab #24](issues/24.md) -> [GitHub #258](https://github.com/LuminariMUD/Luminari-Source/issues/258) (opened): Harbor provisioner stops when the merchant's schedule is disabled
- [GitLab #25](issues/25.md) -> [GitHub #259](https://github.com/LuminariMUD/Luminari-Source/issues/259) (opened): A command that waits in the action queue is followed by a second, empty prompt
- [GitLab #26](issues/26.md) -> [GitHub #260](https://github.com/LuminariMUD/Luminari-Source/issues/260) (opened): Vessel cleanups: dead code, stale comments, and stale document figures
- [GitLab #27](issues/27.md) -> [GitHub #261](https://github.com/LuminariMUD/Luminari-Source/issues/261) (opened): Two hulls at the same position can fire only with the arc that faces north
- [GitLab #28](issues/28.md) -> [GitHub #262](https://github.com/LuminariMUD/Luminari-Source/issues/262) (opened): Only the first special ability of an object is saved
- [GitLab #29](issues/29.md) -> [GitHub #263](https://github.com/LuminariMUD/Luminari-Source/issues/263) (opened): downgrade and applyoil drop an object's special abilities without freeing them, and downgrade leaves its preview object behind
- [GitLab #30](issues/30.md) -> [GitHub #264](https://github.com/LuminariMUD/Luminari-Source/issues/264) (closed): An OLC save of an object prototype leaves its live objects sharing the prototype's lists
- [GitLab #31](issues/31.md) -> [GitHub #265](https://github.com/LuminariMUD/Luminari-Source/issues/265) (opened): Mail letters and forged notes take rnum 1 without being objects of that prototype
- [GitLab #32](issues/32.md) -> [GitHub #266](https://github.com/LuminariMUD/Luminari-Source/issues/266) (opened): stat file on a player who is not online leaves the loaded body in room 0
- [GitLab #33](issues/33.md) -> [GitHub #267](https://github.com/LuminariMUD/Luminari-Source/issues/267) (opened): The player sheath loader logs a debug trace and its query at every load
- [GitLab #34](issues/34.md) -> [GitHub #268](https://github.com/LuminariMUD/Luminari-Source/issues/268) (opened): otransform leaves the object on its old rnum hash chain and miscounts both prototypes

## Completed reviews

- [GitLab !1](merge-requests/1.md) (merged): Replace the four-arm flag with an arm count
- [GitLab !2](merge-requests/2.md) (merged): Add the 17 Duris player races
- [GitLab !3](merge-requests/3.md) (merged): Migrate tables that older installs created before their columns existed
- [GitLab !4](merge-requests/4.md) (merged): Make help-sync run the full sync and fix its keyword verification
- [GitLab !5](merge-requests/5.md) (merged): Fix artificer devices and the artificer class
- [GitLab !6](merge-requests/6.md) (merged): Vessels S1: DurisMUD study defect fixes
- [GitLab !7](merge-requests/7.md) (merged): Vessels S2: DurisMUD movement and pacing
- [GitLab !8](merge-requests/8.md) (merged): Vessels S3: the Duris damage model
- [GitLab !9](merge-requests/9.md) (merged): Vessels S4: weapons and gunnery
- [GitLab !10](merge-requests/10.md) (merged): Vessels study S5: crew, repair and loss
- [GitLab !11](merge-requests/11.md) (merged): Vessels study step S6: NPC raiders, their AI, and ramming
- [GitLab !12](merge-requests/12.md) (merged): Vessels S7: rewards, renown, contraband and customs
- [GitLab !13](merge-requests/13.md) (merged): Vessels S8: client data over MSDP
- [GitLab !14](merge-requests/14.md) (merged): Vessels S-immediate: Luminari Web ready for S9
- [GitLab !15](merge-requests/15.md) (merged): Vessels S9: play tests, the guide record and their fixes
- [GitLab !16](merge-requests/16.md) (merged): Vessels S10: the illustrated player guide and the fixes its fact check found
- [GitLab !17](merge-requests/17.md) (merged): Vessels S11: checked cargo trades
- [GitLab !18](merge-requests/18.md) (merged): Vessels S12: owned waypoints and routes
- [GitLab !19](merge-requests/19.md) (merged): Vessels S12: fixes from the post-merge review
- [GitLab !20](merge-requests/20.md) (merged): Vessels S13: transactions that survive a lost connection
- [GitLab !21](merge-requests/21.md) (merged): Vessels S14: two-phase vessel settlements
- [GitLab !22](merge-requests/22.md) (merged): Vessels S15: checked vessel purchases and payouts
- [GitLab !23](merge-requests/23.md) (merged): Load a saved special ability that has no command word
- [GitLab !24](merge-requests/24.md) (merged): Give the pet legacy-migration test the permanent tables it reads
- [GitLab !25](merge-requests/25.md) (merged): Free the arcane mark of an object with no prototype
- [GitLab !26](merge-requests/26.md) (merged): Keep a live object's own links and lists when another object is written over it
- [GitLab !27](merge-requests/27.md) (merged): Take a leaving character's bagged objects along with the rest
- [GitLab !28](merge-requests/28.md) (merged): Save a sheath's weapons with a pet and extract them with the sheath
- [GitLab !29](merge-requests/29.md) (merged): Price Extra Arms and the monk third hand after the arm count
- [GitLab !30](merge-requests/30.md) (closed): Fix spelling and grammar in player hints
