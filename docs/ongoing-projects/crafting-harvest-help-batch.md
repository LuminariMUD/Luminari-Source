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
- Other terms and the alias review: see the step list; declines are recorded on the issue.

### #225 craft trainer

- Production `lib/world/mob/3.mob` has mobile 373 (`SpecProc: Craft Trainer`) and
  `lib/world/zon/3.zon` has `M 0 373 1 373 100` and the quartermaster reset (checked
  2026-09-23, read-only). Remaining: the in-game `apprentice` check on production.

## Steps

- [ ] 1. #225 in-game check on production (log in, room 373, `apprentice`); comment on the issue.
- [ ] 2. #221 code: recipe skill, 74 variants, tool position, stale readers, labels.
- [ ] 3. #221 tests: leather project start/complete (tool, tannery, experience), old tailoring
  record; switch the sling tool test to a cloth recipe.
- [ ] 4. #221 data: object 389 in `data/crafting-tools`, dev world install.
- [ ] 5. #223 code and tests: one-round nodes.
- [ ] 6. #222 data: `data/harvest-tools` shop and reset notes, dev world install.
- [ ] 7. Docs: CRAFTING_SYSTEM_NOTES, OEDIT_GUIDE (+ `oedit.html`), WILDERNESS_HARVESTING,
  bundle READMEs.
- [ ] 8. Help for 221-223 (dev DB + `help.hlp`): CRAFTING, CRAFT-TOOLS, CRAFTING-STATIONS,
  LEATHERWORKING, TAILORING, HARVEST, HARVEST-TOOLS.
- [ ] 9. #224 section 1 terms (dev DB + `help.hlp`), feat-handler code and test.
- [ ] 10. #224 alias review (dev DB + `help.hlp`); comment the declines and the review summary.
- [ ] 11. Full `make test` with the database, local CI jobs, then the PR.

Post-merge (owner): world-data release of both bundles, help-sync to production, and a fresh
look at production `log/help`.

## Working notes

- Dev help DB: shadow nothing; edits go through `help_versions` archive rows plus updates,
  `changed_by = batch-221-225`. Regenerate `help.hlp` with `catalog.render_help_hlp()` from the
  dev catalog only after checking that the dev DB still projects to master's file plus these
  edits (other sessions share the dev DB).
- Dev world = the main checkout's `lib/world` (the running dev MUD); the worktree has a copy.
