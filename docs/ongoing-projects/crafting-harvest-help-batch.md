# Crafting, harvesting, and help batch (issues 221-225)

Working notes for branch `feat/221-222-223-224-225-crafting-harvest-help` (worktree
`../Luminari-Source-issue-221-222-223-224-225`). Delete this file when the PR merges; the
enduring content lives in the docs, help, and bundles named below.

| Issue | Subject | Kind |
| -- | -- | -- |
| [#221](https://github.com/LuminariMUD/Luminari-Source/issues/221) | Hide recipes use tailoring; leatherworking and the tannery unused | code, data, help |
| [#222](https://github.com/LuminariMUD/Luminari-Source/issues/222) | No player can obtain a harvest tool | data, help |
| [#223](https://github.com/LuminariMUD/Luminari-Source/issues/223) | Zone-node yield against wilderness harvesting | code, docs, help |
| [#224](https://github.com/LuminariMUD/Luminari-Source/issues/224) | Open production help misses and misfiled aliases | help data, small code |
| [#225](https://github.com/LuminariMUD/Luminari-Source/issues/225) | Craft trainer in the production world | operations check |

## Decisions

### #221 leatherworking

- Every recipe variant whose main material (first group) is hides uses leatherworking: all 74
  (leather, studded leather, and hide armor; footwear, gloves, belts, bracers, armbands,
  gorgets, chokers, the hide string, masks; cestus, sling, sap, blackjack, whip, scourge,
  cat o nine tails, bola). Variants whose main material is cloth stay tailoring even when they
  take one hide.
- New recipe skill `CRAFT_SKILL_LEATHERWORKER` (8) maps to `ABILITY_CRAFT_LEATHERWORKING`.
- Tool position: leatherworking shares the knife position (`WEAR_CRAFT_KNIFE`) with hunting.
  Hunting uses it only for room harvesting, which is dormant (`assign_harvest_materials_to_word()`
  has no caller, so no room has a harvest material). A new wear position would touch persistence,
  OLC, and about fifteen tables for no current benefit. The tool is object 389, "a
  leatherworker's knife" (type 57, value 0 = 41, wear `aA`), sold by Jufus (shop 369) like
  391-395, in `data/crafting-tools`.
- Recorded skill: every decision reads the skill from the recipe variant
  (`get_craft_project_skill()`), never the saved `skill_type` (CrSk). The four stale readers
  were the create and supply-order activity rechecks, `start_supply_order()`, and the supply
  reward. A project or order saved with the old tailoring skill therefore resumes, rechecks,
  completes, and pays under leatherworking; completion without the new tool or station makes
  nothing and keeps the allocated materials, as for any unready project.

### #222 harvest tool sources

- Jufus (shop 369, Sanctus III materials vendor) sells all five tiers at their listed costs
  (50, 500, 2500, 12500, 50000). No existing reward system can grant an object (supply orders pay
  gold, experience, and artisan points; artisan points buy nothing), and the costs already form
  the approved progression. Tools set a grade floor only; the difficulty roll still gates every
  success.
- Shipped as `data/harvest-tools/3.zon` (G resets under Jufus) and shop-product install notes.
  Production shop 369 already lists 1252 and 1253 without resets; the notes add only what is
  missing. Production installation is the owner's world-data release.

### #223 node yield

- A node completion takes one round (`NODE_HARVEST_STEPS` 5 to 1) and still yields one unit per
  charge; the wilderness stays at 2-4 units per successful round.
- Per round a node gives about a third of a wilderness success on commodity materials, with no
  failure roll, no grade gate beyond its minimum rank, and its rare drops. Per charge nothing
  changes, so a node's total output (2-6 units or drops) and the rare-drop chances stay as
  authored. Nodes inside wilderness zones keep node-first dispatch and the same rates.

### #224 help

- `point-buy`: STATISTICS gains a short section on buying starting scores in STUDY and the
  POINT-BUY keyword.
- `feat <name>`: the feat help handler accepts a leading `feat`, so `help feat eidolon` shows
  the eidolon feat. EIDOLON stays on DRACONIC-AND-RACIAL-POWERS, which documents the command.
- `house-vendor`: HOUSE-BUILD names the vendor (Sazzy the real estate agent, Real Estate Office,
  Sanctus III room 374, shop 374, also on production) and takes HOUSE-VENDOR.
- `ranger-abilities`, `level-companions`: RANGER takes RANGER-ABILITIES and links
  ANIMAL-COMPANION, which states the companion level rule (druid level + ranger levels above 3,
  +5 Boon Companion, cap 20; `animal_companion_level()` and `do_call()` in `src/act/act.other.c`)
  and takes LEVEL-COMPANIONS and COMPANION-LEVEL.
- Declined (reasons go on the issue): a companion-orders page (ORDER covers pets and followers),
  per-weapon pages (the weapon help handler already shows one weapon by exact name, and WEAPONS
  holds the near-miss spellings), `dominion` (two perks, Psionicist Dominion and Blackguard
  Profane Dominion; `perks info <name>` shows either and no per-perk help exists), `reshape`
  (flavor text only), `monsters`, `custom`, `modification` (no meaning found).
- Production `log/help` (read 2026-09-23) has misses after the triage copy (2026-09-15 to 09-22).
  Answered: DONATE + DONATION-ROOM, RACE + RACE-LIST, CRAFTING-KIT + REDESC, ZONES + ZONES1,
  and the feats hint (#82 in `act.other.c`) no longer advertises the nonexistent FEATTREE and
  FEAT SEARCH. Declined: `planer soul` and `litant-of-defense` (typos; PLANAR-SOUL and
  LITANY-OF-DEFENSE exist and edit-distance suggestions reach them), `divine-shield` (the feat
  is registered but not in game), `dollhouse`, `dagger-falls`, `zones4` (area names are listed in
  ZONES, ZONES2, ZONES3; there is no fourth list), `cities`, `tactics`, `tickets`,
  `forgecraft`, `kailthir` (no matching system; forgecraft is deity portfolio text, kailthir
  appears nowhere). `blinding ray` resolves already.
- Alias review (done): every alias-only token was classified (keep, remove, or move to a named
  entry) against its entry's text: 1,423 tokens on 415 entries, 1,232 kept, 132 removed, 59
  moved. The full list and the declines are in the issue comment.

### #225 craft trainer

- Done 2026-09-23. Production `lib/world/mob/3.mob` has mobile 373 (`SpecProc: Craft Trainer`),
  `lib/world/zon/3.zon` has `M 0 373 1 373 100`, and the supply-order records (372, 363-368)
  are installed. In game, Kohdee's `apprentice` in room 373 listed all twelve tracks. Nothing
  was installed; the transcript is on the issue. The session script logs in through
  `GAME_MASTER_ACCOUNT` from `lib/.env` without echoing credentials and returns Kohdee to the
  Staff Board Room (1204), where it started.

## Steps

- [x] 1. #225 in-game check on production (log in, room 373, `apprentice`); comment on the issue.
- [x] 2. #221 code: recipe skill, 74 variants, tool position, stale readers, labels.
- [x] 3. #221 tests: leather project start/complete (tool, tannery, experience), old tailoring
  record; switch the sling tool test to a cloth recipe.
- [x] 4. #221 data: object 389 in `data/crafting-tools`, dev world install.
- [x] 5. #223 code and tests: one-round nodes.
- [x] 6. #222 data: `data/harvest-tools` shop and reset notes, dev world install.
- [x] 7. Docs: CRAFTING_SYSTEM_NOTES, OEDIT_GUIDE (+ `oedit.html`), WILDERNESS_HARVESTING,
  bundle READMEs.
- [x] 8. Help for 221-223 (dev DB + `help.hlp`): CRAFTING, CRAFT-TOOLS, CRAFTING-STATIONS,
  LEATHERWORKING, TAILORING, HARVEST, HARVEST-TOOLS.
- [x] 9. #224 section 1 terms (dev DB + `help.hlp`), feat-handler code and test.
- [x] 10. #224 alias review (dev DB + `help.hlp`); comment the declines and the review summary.
- [ ] 11. Full `make test` with the database, local CI jobs, then the PR.

Post-merge (owner): world-data release of both bundles (`data/crafting-tools` object 389 and
`data/harvest-tools` shop products and resets), the code release, then help-sync to production
(`sync --authorize-production --repair-layers` from the main checkout after it fast-forwards),
and a fresh look at production `log/help`. The help must not reach production before the code.

## Verification so far

- `make test-all` with the isolated MariaDB (`prepare_test_runtime.sh`, container on
  127.0.0.2:3306): all suites pass, 1797 CuTest tests.
- `scripts/ci/local/run.py --job quality-clang-tidy`: baseline respected (one finding in the
  new help test was fixed).
- New tests fail on the old code: the leather test at its resume case with the old
  `skill_type` readers; the node test at the one-round completion; the feat help test at
  `feat eidolon`.

## Working notes

- Dev help DB: shadow nothing; edits go through `help_versions` archive rows plus updates,
  `changed_by = batch-221-225`. Regenerate `help.hlp` with `catalog.render_help_hlp()` from the
  dev catalog only after checking that the dev DB still projects to master's file plus these
  edits (other sessions share the dev DB).
- Dev world = the main checkout's `lib/world` (the running dev MUD); the worktree has a copy.
