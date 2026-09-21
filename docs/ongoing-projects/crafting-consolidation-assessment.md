# Crafting and harvesting consolidation: final plan

Written 2026-09-21 from a trace of `master` at `548c4eef5`. Line numbers refer to that revision.
Player and world counts come from the development copies under `lib/`, which git does not track.

Status: plan. Nothing implemented. This document supersedes the root `CRAFTING_MERGE_PLAN.md`
(deleted with this revision) and the earlier assessment draft that occupied this file.

## Outcome

One crafting economy. `apprentice`, `craft`, `craftscore`, `newcraft`, `supplyorder`, `brew`,
`salvage`, object-node harvesting, and wilderness harvesting read and advance the same eighteen
craft and harvest abilities, spend and credit the same material and mote balances, and run their
timed work on the same activity machinery. Veteran crafters keep the progress they earned under
the old skills. Live content that only the old code serves (crystals, node drop tables, the
builder catalog, kit utility commands) keeps working. Nothing is left that a player can reach
only by knowing which of two systems a command happens to route to.

## Evidence

### Four pools, three skill spaces, three material stores

| Pool | Entry points | Skills | Materials | Work |
| -- | -- | -- | -- | -- |
| Legacy nodes and kits, `src/craft/craft.c` | `harvest <node>`; kit special: `create`, `checkcraft`, `augment`, `disenchant`, `resize`, `reforge`, `bonearmor`, `restring`, `redesc`, `autocraft`, `convert` (unregistered) | skills 2071 to 2085, 1 to 99, notched by use in `increase_skill()` (`src/core/utils.c:2141`) | `ITEM_MATERIAL` objects, molds, crystals, essences | `eCRAFTING`; the node drop and the kit result are handed over before the timer ends |
| Builder catalog, `src/craft/crafts.c` | `crafting`, `craftedit` | legacy skill id and threshold per record | exact object vnums, optional blueprint | `eCRAFT` |
| Materials-and-motes, `src/craft/crafting_new.c` | `newcraft` (every mode), `craft` and `craftscore` in mode 2, `craftmaterials`, `motes`, `supplyorder`, `brew`, `salvage`, `apprentice` | abilities 34 to 51 with exp, ranks, talent points | `craft_mats_owned[]`, `craft_motes_owned[]` | activity manager (`PRIMARY_ACTIVITY_CRAFT`), `eBREWING` for brew |
| Wilderness, `src/wilderness/harvest.c` | `harvest`, `gather`, `mine` in wilderness rooms | harvest abilities 48 to 51 | the balances above | activity manager (`PRIMARY_ACTIVITY_HARVEST`), one round, payout at completion |
| Wilderness fallback, `src/wilderness/resource_system.c:2251` | same commands with `WILDERNESS_HARVEST_CRAFTING=false`; `materials` under `USE_VARIABLE_QUALITY_MATERIALS` | `get_harvest_skill()` (`:2709`) returns ability ids that `get_harvest_skill_level()` (`:2695`) reads through `GET_SKILL()` | `stored_materials[]`, which no recipe spends | immediate |

Selection is not one switch. `lib/etc/config` holds `crafting_system = 1`, so `do_craft`
(`crafts.c:804`) opens `practice` and `do_craft_score` (`crafting_new.c:6658`) does the same,
while `newcraft` opens the editor regardless. Golems and standalone `reforge` carry extra mode-2
gates (`crafting_new.c:4476`, `:10511`, `:10974`; `src/act/act.other.c:2609`, `:2966`). The
local build header defines `USE_OLD_CRAFTING_SYSTEM`, whose only consumer is the loot table in
`src/obj/treasure.c:852` (crystals drop from 10 percent of magic-item awards), and leaves
`USE_NEW_CRAFTING_SYSTEM` off, which keeps the pickup auto-deposit in `src/obj/act.item.c:2280`
compiled out. Room 370 assigns `crafting_quest` (`src/spec/spec_assign_rooms.c:41`), which
intercepts `supplyorder` for the old autocraft quest; everywhere else the command reaches the
modern contract system.

### Who holds what

Development player files, 7098 characters:

| Population | Count |
| -- | -- |
| Legacy craft skill at 10 or more | 1085 |
| Legacy craft skill at 30 or more | 754 |
| Modern craft or harvest rank above 0 | 42, of which 36 are implementors given 40 everywhere by `src/core/db.c:7765` |
| Modern craft experience above 0 | 0 |
| Old wilderness holdings (`WMat` above 0) | 9 |
| Unfinished room-370 orders (`Cmnm` above 0) | 105 |

The modern progression has never been played on this data. Migration therefore converts a large
legacy population into an empty modern one, and the only modern state to protect is whatever the
trainer has granted on production since it went live.

### What world content the old code still serves

| Object type | Prototypes | Shop entries | Zone loads |
| -- | -- | -- | -- |
| Crystals (`ITEM_CRYSTAL`, for `augment` and `disenchant`) | 17 | 6 | 10, plus loot |
| Molds (`ITEM_MOLD`, for kit `create`) | 1 | 0 | 0 |
| Essences (`ITEM_ESSENCE`) | 2 | 0 | 1 |
| Blueprints (`ITEM_BLUEPRINT`, for the catalog) | 4 | 0 | 0 |
| Material objects (`ITEM_MATERIAL`) | 84 | 18 | 15 |

`lib/etc/crafts` holds five catalog records; three name exact ingredients (Sword of Aegon,
Shield of Taris, Robes of Ah'magan), one has no output, and all carry legacy skill ids (474,
476, 477) with thresholds up to 90. The one mold prototype is the newbie starting mold; no shop
or zone supplies molds. Crystals are live content.

### Defects the trace found

- Node drops lose identity at `craftmaterials store`: alderwood, yew, oak, and darkwood logs
  carry generic wood materials that `obj_material_to_craft_material()` (`crafting_new.c:3455`)
  does not map, medium and high hides (prototypes 3205, 3206) store as low grade hide, velvet
  and burlap have no balance, and fossil eggs store as stone although catalog recipes need the
  egg. The approved harvesting plan (`harvesting-system.md`) verified these in the live game on
  2026-09-19.
- `material_grade()` (`crafting_new.c:640`) has no cotton case while the group-and-grade
  ladder (`:227`) places cotton at grade 4; the two tables also disagree on silk and pristine
  hide.
- The node harvest hands over the drop, decrements the charge, and notches the skill at
  admission (`craft.c:3572` onward), so cancelling the 30-second timer costs nothing.
- `get_harvest_skill()` mixes a legacy skill id with modern ability ids and its caller reads
  the ability id from the skill array. Only the fallback path reaches it.
- `craftmaterials store` adds to a balance without the overflow guard that
  `award_wilderness_harvest()` (`harvest.c:173`) applies.

## Decisions

Each item below was an open question in one of the two drafts. The evidence above settles it;
where the owner may want a different number, the plan names the single constant to change.

### 1. Skill space: the eighteen abilities, converted once at load

| Legacy skill | Id | Converts to | Note |
| -- | -- | -- | -- |
| mining | 2071 | mining (48) |  |
| hunting | 2072 | hunting (49) |  |
| foresting | 2073 | forestry (50) |  |
| knitting | 2074 | tailoring (35) and gathering (51) | knitting gated both cloth nodes and cloth armor; one rank cannot be split, so both receive it |
| chemistry | 2075 | alchemy (36) | chemistry gates `augment` and `disenchant`; those gates read alchemy afterwards |
| armor smithing | 2076 | armorsmithing (37) |  |
| weapon smithing | 2077 | weaponsmithing (38) |  |
| jewelry making | 2078 | jewelcrafting (40) |  |
| leather working | 2079 | leatherworking (41) |  |
| fast crafter | 2080 | talent points only | speed is bought as rapid talents in the modern system |
| bone armor, elven, masterwork, draconic, dwarven crafting | 2081 to 2085 | nothing | present only in the `increase_skill()` table; no gate reads them |

Conversion rule: `rank = legacy / CRAFT_LEGACY_SKILL_PER_RANK` with the constant at 5, so a
maxed legacy skill becomes rank 19, one below the trainer ceiling. Rank 19 completes level-19
projects on an average roll and level-29 on a natural 20 (`d20 + rank` against `10 + level`);
legacy 99 built level-33 items without a roll. The constant is the owner's tuning knob.

- Experience is set to `craft_skill_level_exp(rank)`, so the next rank costs the normal amount.
- An ability whose rank or experience is already higher keeps what it has. Implementors keep
  their 40s.
- Talent points: one per converted rank, the modern rate, counted once for knitting (tailoring
  receives the points, gathering the rank only), plus `fast crafter / 5` points. A converted
  crafter therefore holds what a modern crafter of the same ranks would hold.
- The conversion runs in the post-load reconciliation that already exists in
  `src/player/players.c` (the `earned_rank` loop after the tag switch), guarded by a new
  player-file tag `CrMg` carrying the migration version, so it runs exactly once per character.
- Legacy skill values are left in the file untouched until Phase 5 removes their definitions.
  They are the audit trail while the conversion is proven.
- Every legacy gate that read `GET_SKILL(ch, skill)` reads
  `craft_legacy_skill_equivalent(ch, ability)`, which returns `rank * 5`. Kit level gates keep
  their `/ 3`, node minimum skills keep their numbers, and catalog thresholds convert the same
  way. This preserves every threshold the players know.

### 2. Material identity: explicit prototype table, no new balances

A code table keyed by the prototype defines already in `src/craft/craft.h` gives the legacy
node and shop objects their balance. `craft_material_from_object()` (`crafting_new.c:3616`)
consults it before the generic material fallback.

| Prototype | Stores as today | Stores as |
| -- | -- | -- |
| 3197 alderwood log | nothing | ash wood (grade 1) |
| 3201 yew log | nothing | maple wood (grade 2) |
| 3202 oak log | nothing | mahagony wood (grade 3) |
| 3203 darkwood log | nothing | ironwood (grade 5; node needs skill 38 and the log costs ten times the others) |
| 3205 medium quality hide | low grade hide | medium grade hide |
| 3206 high quality hide | low grade hide | high grade hide |
| 3133 velvet | nothing | cotton (grade 4; the velvet node sits between the wool and satin nodes) |
| 3127 burlap | nothing | hemp (grade 1) |
| 3198, 3199, 3200, 3207 fossil eggs | stone | not storable; they stay objects for the catalog |
| 3187, 3188, 3191, 3192, 3195, 3196 gems | nothing | not storable; unchanged |

No new `CRAFT_MAT_*` ids: adding materials would touch recipes, grade tables, ladders, and the
`CfMt` block, and the owner's harvesting decision 1 already chose reuse. Each row is one line
to change. `material_grade()` becomes the one grade table: it gains cotton at 4, and the ladder
in `determine_material_type_by_group_and_grade()` is corrected to agree with it (silk 4, satin
5, pristine hide 4, dragonscale 5), which the approved harvesting plan also requires.

### 3. Nodes credit the balance at completion

Object nodes stay: they are the only harvest source outside the wilderness and they carry the
authored drop tables and rare drops. Their harvest moves onto the activity manager with the
definition `start_wilderness_crafting_harvest()` (`harvest.c:306`) already uses, targeting the
node through `domain_event_object_handle()` so losing the node cancels the work:

- Admission checks the node, the minimum skill through the equivalent above, and the full-round
  action, then starts five steps at `PULSE_VIOLENCE`: the same 30 seconds as today.
- Completion rechecks that the node is still in the room with a charge left; when two players
  contest the last charge the second sees "depleted" and receives nothing. It rolls the existing
  drop table into a prototype vnum. When that prototype has a balance it credits one unit
  through the checked add; gems, fossil eggs, and anything else without a balance are created as
  objects exactly as today. Then it decrements the charge, extracts an empty node, and grants
  `20 + 10 * material_grade` experience to the node family's harvest ability: metals to mining,
  wood to forestry, hides to hunting, cloth to gathering.
- Cancellation awards nothing and touches no charge.
- Yield (one unit per 30 seconds) is unchanged. Rates against the wilderness (two to four units
  per round) are a tuning decision for after Phase 3, with the step count and the unit count as
  the two constants.

### 4. Wilderness: the approved plan, unchanged

`harvesting-system.md` (owner-approved 2026-09-18, unimplemented) replaces the quality-rolled
ladder with `harvest <material>`, grade access from skill, richness, and tools, and deletes
`wilderness_crafting_bridge.c`. It is Phase 3a of this plan by reference. Its scope statement
keeps the toggle and the store step; this plan retires the toggle later, in Phase 5, after the
nine old holdings are converted.

### 5. One command surface

`craft` opens the project editor and `craftscore` shows the score in every configuration;
`newcraft` stays as an alias. `CONFIG_CRAFTING_SYSTEM`, the three `CRAFTING_SYSTEM_*` values,
the `cedit` entry (`src/olc/cedit.c:195`, `:379`, `:986`), the config parse (`db.c:8383`,
`:8468`), and the five mode-2 gates go. `crafting` (the catalog) and the kit commands stay
registered as they are.

### 6. Kits keep their utility commands; mold creation retires

Keep `augment`, `disenchant`, `resize`, `reforge`, `bonearmor`, `restring`, `redesc`: crystals
are live content, and each of these has no modern equivalent. Their gates read the equivalent
skill from Decision 1 and their experience goes through `gain_craft_exp()` on the mapped
ability. Retire `create`, `checkcraft`, `convert`, and the four node-harvest branches of
`event_crafting`: one mold prototype exists, nothing sells or loads it, and the editor builds
weapons, armor, jewelry, and instruments. The starting mold (`NOOB_CRAFT_MOLD`) leaves the
newbie kit.

### 7. Catalog keeps its records; its skill field converts

`crafts.c` keeps `lib/etc/crafts`, blueprints, exact-component requirements, and `craftedit`.
Its loader converts a legacy skill id and threshold through the Decision 1 table when it reads
a record (`Skil: 477 90` becomes weaponsmithing at rank 18) and `craftedit` lists abilities.
Its `eCRAFT` timer is not migrated: it has its own consumption policy and no duplicate.

### 8. One supply-order implementation

Unassign room 370, delete `crafting_quest` and the kit `autocraft` branch, and let the modern
`supplyorder` handle the room. The 105 unfinished legacy orders are cleared at conversion with a
message; the quest offered a reward for completion and no penalty for abandonment, so nothing is
owed. The `Cvnm`, `Cmnm`, `CPts`, `Cqps`, `Cexp` tags stay readable for one release and are then
dropped from the writer.

### 9. Loot and pickup

The crystal-inclusive loot table (today's `USE_OLD_CRAFTING_SYSTEM` branch) becomes
unconditional; the other branch and both flags go. The pickup auto-deposit
(`get_check_craft_material`) is deleted rather than enabled: bundles remain tradable through
shops and `give`, and `craftmaterials store` stays the one explicit deposit.

### 10. Old wilderness holdings and the fallback

At load, each `stored_materials[]` record converts through the group-and-grade ladder (mote
categories through `wilderness_harvest_mote()`), the result is credited with the checked add,
records that map to nothing are written to the syslog with the character name, and the array is
cleared under the same `CrMg` version. Nine characters are affected, which is small enough to
review by hand from the log. The fallback harvest, `stored_materials[]`, the `WMat` and `Mat`
tags, the `materials` command, `USE_VARIABLE_QUALITY_MATERIALS`, `get_harvest_skill()`, and
`WILDERNESS_HARVEST_CRAFTING` are then deleted.

## Phases

Each phase builds, passes `make -j$(nproc) test` followed by `make install`, and is committed on
its own. Update the checkboxes with every commit so a new session can resume.

### Phase 1: material identity and checked balances

- [ ] Add `craft_balance_add(ch, material, quantity)` and `craft_mote_add()` with the overflow
  guard from `award_wilderness_harvest()`; route `craftmaterials store`, `salvage`
  (`act.item.c:9189`), and `award_wilderness_harvest()` through them.
- [ ] Add the prototype table from Decision 2 and consult it first in
  `craft_material_from_object()`; mark fossil eggs unstorable.
- [ ] Make `material_grade()` the one grade table (cotton 4) and align the ladder with it.
- [ ] Tests in `test_crafting_projects.c`: each row of Decision 2 stores as its balance and
  `unstore` returns the same material; a fossil egg is refused; the store add refuses overflow;
  every ladder entry agrees with `material_grade()`.

Completion evidence: a node drop of every kind either stores into its named balance and comes
back as the same material, or stays an object.

### Phase 2: progression conversion and one command surface

- [ ] Add `craft_legacy_skill_equivalent()` and the conversion table to `crafting_new.c`; run
  the conversion from the post-load reconciliation in `players.c` under the `CrMg` tag;
  log each conversion with the character name and the ranks granted.
- [ ] Switch every `GET_SKILL(ch, SKILL_*)` and `increase_skill()` in `craft.c` and `crafts.c`
  to the equivalent and `gain_craft_exp()`; convert catalog skill ids in the `crafts.c` loader
  and `craftedit`.
- [ ] `do_craft` opens the editor and `do_craft_score` the score; remove the selector, its
  config field, the `cedit` entry, and the mode-2 gates; remove the crafting-skill listing from
  `practice`.
- [ ] Tests in `test_craft_training.c`: a fixture with legacy 99 converts to rank 19 with the
  right experience and talent points; knitting fills both abilities and pays once; a higher
  existing rank or experience survives; the conversion is idempotent across save and load; a
  contract in progress (`CrTr`) is untouched. Tests in `test_crafting_projects.c`: kit `resize`
  and `augment` pass and fail on the converted gate; a catalog record with `Skil: 477 90` needs
  weaponsmithing 18.

Completion evidence: a veteran's `craft`, `craftscore`, and `apprentice` show the same ranks,
and every kit and catalog threshold they could meet yesterday they can meet today.

### Phase 3a: wilderness targeted gathering

- [ ] Implement `harvesting-system.md` steps 1 to 7 as written, against the Phase 1 grade table
  (its step 1 cotton case is already done). Its tests, help, and acceptance criteria apply.

### Phase 3b: node harvesting on the activity manager

- [ ] Replace the body of `do_harvest()` after node lookup with an activity per Decision 3,
  sharing the definition and recheck shape of `start_wilderness_crafting_harvest()`; delete the
  harvest branches of `event_crafting` and the harvest use of `GET_CRAFTING_TYPE`.
- [ ] Replace `resource_configs[].harvest_skill` legacy ids in `resource_system.c:44` with the
  harvest abilities.
- [ ] Tests in `test_gameplay_e2e.c`: a node harvest credits nothing at start and one unit of
  the right balance at completion; a gem drop arrives as an object; cancel by movement awards
  nothing and keeps the charge; two characters on a one-charge node leave one award and no
  node; save, reload, and copyover during the work produce no award and no duplicate.

Completion evidence: `harvest <node>` in a zone and `harvest <material>` in the wilderness raise
the same balance and the same ability, and neither can be cancelled for free or paid twice.

### Phase 4: legacy capability closure

- [ ] Retire kit `create`, `checkcraft`, `convert`, and the mold from the newbie kit; keep the
  seven utility commands.
- [ ] Unassign room 370; delete `crafting_quest` and `autocraft`; clear legacy orders at
  conversion with a message.
- [ ] Tests: the kit refuses `create` with a pointer to `craft`; `supplyorder` in room 370
  reaches the modern handler; a file with `Cmnm: 3` loads with the order cleared and a message.

### Phase 5: retire duplicate paths

- [ ] Convert old wilderness holdings at load (Decision 10), then delete `stored_materials[]`,
  its tags, `do_materials`, `USE_VARIABLE_QUALITY_MATERIALS`, the fallback branch of
  `do_wilderness_harvest()`, `get_harvest_skill()`, `get_harvest_skill_level()`, and the
  `WILDERNESS_HARVEST_CRAFTING` toggle in code, `lib/.env.example`, and the docs that name it.
- [ ] Make the crystal-inclusive loot table unconditional; delete the other branch,
  `get_check_craft_material`, and both `USE_*_CRAFTING_SYSTEM` flags in the example header.
- [ ] Delete `enhanced_crafting_recipes.h` (no includer) if Phase 3a has not already.
- [ ] Remove skills 2071 to 2085: the `skillo()` lines (`src/magic/spell_parser.c:6813`), the
  `CRAFTING_SKILL` school and `list_crafting_skills()`, the `increase_skill()` cases, the
  `class.c:2836` respec exception, and the defines. Keep the old-id remap in `db.c:7856` so
  pre-merge files still load, and keep the array slots.
- [ ] Update both build manifests for every removed file and run
  `python3 scripts/ci/check_build_parity.py`.
- [ ] Tests: a file with `WMat` records loads with the mapped balances credited and the array
  empty; a pre-merge file with `Skil` ids 471 to 485 loads without a `SYSERR`.

### Phase 6: help and documentation

- [ ] Help, in the database and `lib/text/help/help.hlp` through the help-sync workflow:
  rewrite `CRAFTING`, `CRAFT-SCORE`, `CRAFTING-SKILLS`, `HARVEST`, `CRAFTING-KIT`,
  `APPRENTICE`, `SUPPLYORDER`, and `CRAFTS`; remove the mode text, `CHECKCRAFT`,
  `AUTOCRAFT`, and `LEGACY ROOM-370 AUTOCRAFT QUEST`; Phase 3a covers the wilderness topics.
- [ ] Replace `docs/world_game-data/CRAFTING_SYSTEM_NOTES.md` with a description of the merged
  system; update `docs/systems/WILDERNESS_HARVESTING.md` and `docs/deployment/environments.md`;
  mark `craft-training-apprentice-skills.md` resolved and point it here.

## Verification and deployment

- Every phase: `make -j$(nproc) test && make install`, plus the parity script when files change.
- After Phase 2 and again after Phase 3b, on the 4100 development server with copies of three
  veteran player files (a maxed miner, a mid-rank knitter, an implementor): `craft`,
  `craftscore`, and `apprentice` agree; a zone node and a wilderness coordinate credit the same
  balance and ability; a kit `resize` and an `augment` still work; `crafting` lists the catalog
  with the converted threshold.
- Deployment is a normal code release. The conversion runs on each character's first login and
  needs no offline pass; the player-file backup that precedes any release is the rollback. A
  production restart needs its own approval.

## Recorded, not in scope

- Tool rules: equipment admission checks slot occupancy while `craft tools` looks for an
  `ITEM_CRAFTING_TOOL`; a modern-system inconsistency, not a merge concern.
- `supplyorder list` prints advice and artisan points but no offers for `select`.
- Refining and resizing handlers in `crafting_new.c` are reachable by no command; they are the
  modern system's own dormant features and their save fields stay.
- `brew` on `eBREWING` and `scribe` keep their own timers.
- Rates: node versus wilderness yield, and node behaviour inside wilderness zones, are tuned
  after Phase 3 with the constants named in Decision 3.
- Many hide equipment recipes use tailoring rather than leatherworking; a recipe decision.
- Rare and legendary harvest tools (1254, 1255) are sold and dropped nowhere.
- Any new material, recipe, or talent.

## Ablation notes

- Dropped from the assessment draft: new material identities and balances (an explicit table
  onto existing balances covers every traced object), a second migration wallet, a parallel
  recipe language, mandatory refining, migrating brew and scribe, unifying tool rules, and the
  supply-order listing fix. None serves the outcome.
- Dropped from the root draft: retiring `augment` and `disenchant` (crystals are live content),
  enabling the pickup auto-deposit (bundles stay tradable), zeroing legacy values at conversion
  (they are the audit trail), deleting the toggle before the holdings convert, dropping fast
  crafter without compensation, and moving node code into a new file (no requirement).
- Simplified: the conversion hooks into the existing post-load reconciliation instead of a new
  command; node harvesting reuses the wilderness activity definition and the existing drop
  tables instead of a new table; the catalog reuses the conversion table instead of its own.
- Kept, each with a traced consumer: the kit utility commands, the catalog and blueprints, node
  objects and their rare drops, the crystal loot table, the old-id remap in the loader.
