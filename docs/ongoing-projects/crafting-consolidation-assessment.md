# Crafting and harvesting consolidation plan

Written 2026-09-21; reviewed against `dbef1778c` on the same date. Source line numbers refer to
that revision. Counts below were rechecked against the development `*.plr` files and the world
files listed in the `index` files under `lib/`. These are not production counts.

Tracking issue: #212. Status: in progress on `feat/212-crafting-consolidation`; the phase
checklists below record what is done. This document supersedes the root `CRAFTING_MERGE_PLAN.md`
(deleted in `dbef1778c`) and the earlier assessment draft that occupied this file.

## Outcome

One crafting economy. `apprentice`, `craft`, `craftscore`, `newcraft`, `crafting`, kit commands,
`supplyorder`, `brew`, `salvage`, object-node harvesting, and wilderness harvesting share the
same material and mote balances and, wherever a craft skill applies, the same ability space.
Seventeen tracks are defined at 34 to 46 and 48 to 51; `crafting_skill_type()` currently enables
eight crafts and four harvests, leaving bowmaking, trapmaking, poisonmaking, fishing, and cooking
inactive. Preserve those slots without activating new features. Slot 47 retains the display
name "brewing" but is excluded from the craft definitions; `brew` uses alchemy. Existing class,
feat, and spell prerequisites remain; skill-less utilities do not acquire invented skill gates.

All timed crafting and harvesting uses the existing activity manager. Recipe-specific
ingredients, success rolls, and failure costs remain distinct policies on that machinery.
Veterans retain their progress and access to existing capabilities. Authored molds, crystal
bonuses, essences, node drops, catalog recipes, and kit utilities remain usable through the
shared system. A command's spelling, a carried kit, or a configuration mode must not select a
different progression system or a duplicate implementation of the same operation.

## Evidence

### Existing entry points and state

| Pool | Entry points | Skills | Materials | Work |
| -- | -- | -- | -- | -- |
| Legacy nodes and kits, `src/craft/craft.c` | `harvest <node>`; kit special: `create`, `checkcraft`, `augment`, `disenchant`, `resize`, `reforge`, `bonearmor`, `restring`, `redesc`, `autocraft`, `convert` (unregistered) | skills 2071 to 2085, 1 to 99, notched by use in `increase_skill()` (`src/core/utils.c:2141`) | `ITEM_MATERIAL` objects, molds, crystals, essences | `eCRAFTING`; the node drop and the kit result are handed over before the timer ends |
| Builder catalog, `src/craft/crafts.c` | `crafting`, `craftedit` | legacy skill id and threshold per record | exact object vnums, optional blueprint | `eCRAFT` |
| Materials-and-motes, `src/craft/crafting_new.c` | `newcraft` (every mode), `craft` and `craftscore` in mode 2, `craftmaterials`, `motes`, `supplyorder`, `brew`, `salvage`, `apprentice` | abilities 34 to 46 and 48 to 51 with exp, ranks, talent points | `craft_mats_owned[]`, `craft_motes_owned[]` | activity manager (`PRIMARY_ACTIVITY_CRAFT`); `eBREWING` for brew; standalone reforge still uses `eCRAFTING` |
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
modern contract system. The kit special also intercepts `reforge` before the standalone handler;
removing the configuration gate alone does not remove that duplicate implementation.

### Who holds what

Development player files, 7098 (two of them empty). Legacy ids below 2000 are normalized as the
actual `load_skills()` reader does (`src/player/players.c:5358`, adding 1600):

| Population | Count |
| -- | -- |
| Any legacy craft skill (2071 to 2085) at 10 or more | 1190 |
| Any legacy craft skill at 30 or more | 839 |
| One of the nine mapped skills at 10 or more | 1128 |
| One of the nine mapped skills at 30 or more | 764 |
| Modern craft or harvest rank above 0 | 42 |
| Modern craft experience above 0 | 0 |
| Old wilderness holdings (`WMat` above 0) | 9 |
| Unfinished room-370 orders (`Cmnm` above 0) | 105 |
| Completed, unclaimed room-370 orders (`Cvnm` above 0, `Cmnm` zero or absent) | 17 |

Zero recorded modern experience does not prove the system was never used or establish the
origin of saved ranks. Preserve all existing ranks, experience, talents, balances, allocated
project inputs, supply contracts, and paid training contracts. Production may differ from this
snapshot; recheck its inventory through an explicitly authorized read before release.

### What world content the old code still serves

| Object type | Prototypes | Shop entries | Zone loads |
| -- | -- | -- | -- |
| Crystals (`ITEM_CRYSTAL`, consumed by kit `create`) | 17 | 6 | 10, plus loot |
| Molds (`ITEM_MOLD` extra flag, consumed by kit `create`) | 213 | 188 | 202 |
| Essences (`ITEM_ESSENCE`) | 2 | 0 | 1 |
| Blueprints (`ITEM_BLUEPRINT`, for the catalog) | 4 | 0 | 0 |
| Material objects (`ITEM_MATERIAL`) | 83 | 17 | 15 |

`lib/etc/crafts` holds five catalog records: four require exact ingredients and carry old skill
ids 474, 476, or 477 with thresholds up to 90; the fifth has no output and `Skil: -1 0`.
The reader does not currently remap those catalog ids, unlike the player-file reader.

Molds are identified by `OBJ_FLAGGED(obj, ITEM_MOLD)`, not object type 24 (clan armor).
`create()` (`craft.c:2047` onward) consumes a mold, materials, an optional crystal whose affects
are copied onto the result, and an optional essence for a masterwork roll. `disenchant()` makes
essences; `augment()` combines two essences, as its help text says. Neither
substitutes for the crystal-to-equipment path. `crafting_molds.c` also builds molds dynamically,
but its `buymolds` callback has no current assignment; the indexed shops already supply molds.

### Defects the trace found

- Node drops lose identity at `craftmaterials store`: alderwood, yew, oak, and darkwood logs
  carry generic wood materials that `obj_material_to_craft_material()` (`crafting_new.c:3455`)
  does not map, medium and high hides (prototypes 3205, 3206) store as low grade hide, velvet
  and burlap have no balance, and fossil eggs store as stone although catalog recipes need the
  egg. The approved harvesting plan (`harvesting-system.md`) verified these in the live game on
  2026-09-19.
- `material_grade()` (`crafting_new.c:640`) has no cotton case while the group-and-grade
  selector (`:227`) places cotton at grade 4. That selector also intentionally rolls lower-grade
  rewards, including stone for metals; it is not a deterministic grade lookup or migration map.
- The node harvest hands over the drop, decrements the charge, and notches the skill at
  admission (`craft.c:3572` onward), so cancelling the 30-second timer costs nothing.
- `get_harvest_skill()` mixes a legacy skill id with modern ability ids and its caller reads
  the ability id from the skill array. Only the fallback path reaches it.
- `craftmaterials store` adds to a balance without the overflow guard that
  `award_wilderness_harvest()` (`harvest.c:173`) applies.
- Direct node deposits would break catalog requirements such as adamantine prototype 3193:
  `unstore` creates `ITEM_PROTOTYPE` bundles, while `find_requirement()` in `crafts.c` compares
  exact vnums. Catalog commodity requirements must accept the shared balances before that cutover.
- The draft's floor conversion loses access: legacy 48 becomes rank 9, equivalent 45, below a
  mithril node's 48 gate; chemistry 87 becomes equivalent 85, below a level-29 essence's 87 gate.
- `CPts` is clan points (`players.c:1041`, `:3178`), not a crafting-order tag. Completed orders
  have unclaimed rewards, and partial orders have already consumed materials (`autocraft()`).
- Keeping `eCRAFT`, `eCRAFTING`, and `eBREWING` indefinitely contradicts the shared activity
  outcome. `event_craft()` also awards no craft experience; changing its skill reader alone
  cannot make catalog work advance the shared progression.
- `save_char_checked()` reports write failures but opens the live player file with `"w"`
  (`players.c:2519`). It does not yet provide atomic replacement; a migration marker alone
  cannot protect the old file from an interrupted save.

## Decisions

These decisions preserve the consolidation outcome against the traced consumers. Conversion
rates and new experience awards below are proposed defaults, not evidence of equivalent game
balance or additional owner-approved tuning.

### 1. Skill space: the existing abilities, with a versioned conversion

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
| bone armor, elven, masterwork, draconic, dwarven crafting | 2081 to 2085 | nothing | no active gate reads these skill slots; the distinct `FEAT_*` capabilities in mold creation remain |

Conversion rule: a legacy value at or below the `init_char()` seed of 4 converts to rank 0;
above the seed, `rank = ceil(legacy / CRAFT_LEGACY_SKILL_PER_RANK)` in integer arithmetic with
the divisor initially 5. Thus 4 stays 0, 5 becomes 1, 48 becomes 10, 87 becomes 18, and 99
becomes 20. Rounding up deliberately grants at most four legacy points of access rather than
taking away an existing gate. The seed exception matters: every character carries 4 in all
fifteen legacy slots (`src/core/db.c:7877`), so a rule that converted the seed would grant nine
rank-1 tracks, nine talent points, and 9000 experience to every character; the seed is starter
access, which the equivalent below preserves, not progress. The divisor changes progression
economics; do not describe this as identical success odds or silently replace legacy creation
rolls with the editor's `d20 + rank` roll.

- Reconcile earned ranks from experience first. For each mapped ability, let `old_rank` be the
  higher of its saved rank and `craft_skill_rank_for_exp(ch, old_exp)`. Set the result to the
  higher of `old_rank` and the converted rank; set experience to the higher of `old_exp` and
  `craft_skill_level_exp(ch, resulting_rank)`. Preserve higher staff ranks too. Set migration
  experience directly, without applying insightful talent bonuses to the conversion.
- Account for the later immortal initialization in `load_char()` (`players.c:2248`), which grants
  skills 100 and abilities 40 after the current reconciliation loop. Preserve staff overrides
  without treating those automatic values as earned conversion ranks or fast-crafter compensation;
  verify actual staff login ordering, not just a manually initialized rank-40 fixture.
- Award talent points only for newly granted ranks, preserving spent talents and unspent points.
  Knitting grants both ranks but pays the larger of the two rank increases once, not their sum.
  Add `fast crafter / 5` points once as compensation. This is a migration policy, not a claim
  that two converted tracks earned the same points as two independently trained tracks.
- Reuse the post-load reconciliation in `src/player/players.c`. Store `CrMg` outside the
  resettable project state. Use ordered stages: 1 for skills, 2 for order settlement, 3 for old
  wilderness holdings. Each stage runs only if its version is missing, and advances the marker
  only after its changes succeed. A character migrated in Phase 2 must still receive the work
  introduced in Phases 4 and 5; a character returning after all phases runs every missing stage.
- Persist the marker and its results together at the player-entry boundary before allowing play.
  First make `save_char_checked()` publish a fully written/flushed temporary file by replacement,
  following the existing `save_player_index_checked()` pattern and preserving file permissions.
  A failure before publication must leave the old file intact; never report a published conversion
  as uncommitted and then repay it. Test write, flush, and replacement failures. Loading a
  character for inspection must not itself rewrite the file. A project reset or respec cannot
  reset the marker. Preserve `CrTr` and settle it through its existing training path.
- Leave legacy skill values readable and serialized as audit data, even after their gameplay
  definitions retire. Keep their numeric slots reserved. The old-id remap lives in
  `players.c:load_skills()`, not the commented list in `db.c:7856`.
- Legacy-scale gates read `craft_legacy_skill_equivalent(ch, ability)`, which returns
  `MAX(4, rank * divisor)`; the floor is today's seed, so a fresh character keeps the level-1
  creation and minimum-skill-1 node access the seed gave. Preserve kit `/ 3` level checks and
  node thresholds. Catalog conversion and rolls are specified
  in Decision 7. Replace all fast-crafter consumers, including standalone reforge in
  `crafting_new.c`, with the appropriate existing rapid-talent calculation in seconds, clamped
  to a positive duration. Do not pass seconds into the old six-second tick subtraction.
- When retiring `init_char()`'s legacy seed of 4, grant nothing in its place: the equivalent's
  floor of 4 keeps starter node and level-1 creation access without ranks, experience, or
  talent points. Test starter node access as well as veteran access; do not leave new
  characters below every node's gate.

### 2. Material identity: explicit prototype table, no new balances

A code table keyed by the prototype defines already in `src/craft/craft.h` gives the legacy
node and shop objects their balance. `craft_material_from_object()` (`crafting_new.c:3616`)
consults it before the generic material fallback. Explicit non-storable entries must stop the
lookup rather than falling through to stone. Preserve the validated material id in value 1 of
canonical `unstore` bundles; a generic material lookup must not collapse their hide grades.

| Prototype | Stores as today | Stores as |
| -- | -- | -- |
| 3134 generic wood material | nothing | ash wood (grade 1; also sold by the material shop) |
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
`CfMt` block, and the owner's harvesting decision 1 already chose reuse. `material_grade()` is
the authoritative grade table: add cotton at 4, retaining silk 4, satin 5, pristine hide 4, and
dragonscale 5. Targeted harvesting uses those grades. Do not rewrite the random lower-tier
choices in `determine_material_type_by_group_and_grade()` merely to force its output grade to
equal its input tier, and never call that random selector to migrate saved holdings.

Checked credit functions validate the id, quantity, current balance, and overflow, returning
success or failure. A rejected store leaves its object; a rejected node award leaves its charge.
Salvage preflights its combined gold, material, and mote result before extracting the item or
crediting any part. Refunds must retain their allocated inputs if the destination cannot accept
them. Cover existing project/mote refunds, efficient-material returns, supply-order abandonment,
golem cancellation, and load-time resize refunds as well as new awards; otherwise the new guard
can still be bypassed through an existing writer. Reuse the current balances and project fields.

### 3. Nodes credit the balance at completion

Object nodes stay: they are the harvest source outside the wilderness and carry authored drop
tables and rare drops. Their harvest uses the same activity-manager API and admission rules as
`start_wilderness_crafting_harvest()` (`harvest.c:306`), with its own node context and completion
callback. Target the node through `domain_event_object_handle()` so extraction cancels the work:

- Admission checks the node, the minimum skill through the equivalent above, and the full-round
  action, then starts five steps at `PULSE_VIOLENCE` (30 seconds). No drop object is allocated,
  charge spent, or reward rolled at admission. Keep node-first command dispatch in wilderness.
- Completion rechecks that the node is still in the room with a charge left; when two players
  contest the last charge the second sees "depleted" and receives nothing. It rolls the existing
  drop table into a prototype vnum. When that prototype has a balance it credits one unit
  through the checked add; gems, fossil eggs, and anything else without a balance are created as
  objects. Confirm credit or object creation/delivery succeeded before decrementing the charge
  or granting experience. On depletion, update the corresponding `mining_nodes`, `farming_nodes`,
  `hunting_nodes`, or `foresting_nodes` counter before extraction. Preserve the node family's
  `AQ_CRAFT_*` quest hook once on successful completion.
- Experience is `20 + 10 * material_grade` to the node family's harvest ability: metals to
  mining, wood to forestry, hides to hunting, cloth to gathering. An unmapped rare object uses
  the source node's ordinary material grade, not a lookup of `CRAFT_MAT_NONE`. Recheck inventory
  capacity for object rewards; an undeliverable reward leaves the charge and experience alone.
- Cancellation awards nothing and touches no charge.
- Yield (one unit per 30 seconds) is unchanged. Rates against the wilderness (two to four units
  per round) are a tuning decision for after Phase 3, with the step count and the unit count as
  the two constants.

### 4. Wilderness: preserve the approved gathering behavior

[harvesting-system.md](harvesting-system.md) (owner-approved 2026-09-18, unimplemented) replaces
the quality-rolled ladder with `harvest <material>`, grade access from skill, richness, and
tools, and deletes `wilderness_crafting_bridge.c`. It is Phase 3a by reference. Its material
pools, tool and richness rules, difficulty, aliases, and acceptance tests remain. Integrate it
with Phase 1's checked credits rather than adding another balance writer. Preserve the old
reward mapping as a compatibility reader for Decision 10 before removing its use in live
harvesting. Its promise to leave nodes and the toggle unchanged applies to Phase 3a, not to the
later consolidation phases.

### 5. One command surface

`craft` opens the project editor and `craftscore` shows the score in every configuration;
`newcraft` stays as an alias. `CONFIG_CRAFTING_SYSTEM`, the three `CRAFTING_SYSTEM_*` values,
the `cedit` entry (`src/olc/cedit.c:195`, `:379`, `:986`), the config parse (`db.c:8383`,
`:8468`), and the mode-2 gates go. Expose catalog and mold-based creation from `craft`'s menu
and dispatch; retain `crafting` and kit command spellings as front ends to those operations.
Kit and inventory `reforge` must call one implementation, with argument/context selection
separate from validation and execution. Update special-procedure dispatch and its tests as well
as `cmd_info[]`; registration alone does not establish which handler a player reaches.

### 6. Preserve mold creation and kit capabilities on the shared engine

Keep the authored mold as an alternative project template, using the existing project/activity
machinery rather than retaining a second skill system. Preserve its base item/wear data, crystal
affects and enhancement, optional essence/masterwork outcome, and crafting-feat benefits.
Ordinary materials come from the shared balances; the mold, crystal, and essence stay physical
ingredients. Validate commodities using canonical material groups; the old `IS_WOOD` macro does
not recognize the modern ash/maple materials. The common `craft` interface and kit
`create`/`checkcraft` front ends must reach the same validation, preview, and completion functions.
Explain the explicit material deposit
step to existing kit users. `NOOB_CRAFT_MOLD`, shops, and zone loads stay useful.

Keep `augment`, `disenchant`, `resize`, `reforge`, `bonearmor`, `restring`, and `redesc`.
Use mapped abilities where a skill applies: chemistry becomes alchemy; bonearmor and reforge
retain their traced armor/weapon skill associations. `resize`, `restring`, and `redesc` have
no existing skill gate; do not invent one or award craft experience for renaming an item.
Replace legacy notches with a single operation-appropriate `gain_craft_exp()` award at resolution,
not one award per timer tick. Preserve existing non-craft rewards explicitly rather than
accidentally treating character experience as craft experience.

Move mutations, payment, and ingredient consumption out of admission and into the checked
completion path (Decision 11). In particular, `disenchant()` currently reads the object's level
after `extract_obj()`; capture needed values before extraction and test this path. Remove the
unregistered `convert` branch and stale kit instructions advertising it. Retire old execution
branches only after each retained capability works through the common path.

### 7. Catalog keeps its format and authored ingredients, joining skills and balances

`crafts.c` keeps `lib/etc/crafts`, blueprints, and `craftedit`; no new recipe language is needed.

- Read old `Skil` records in either 471 to 485 or 2071 to 2085 numbering. Use an explicit new
  `Abil` record for ability ids and rank requirements, and write that form from `craftedit`.
  `Skil: 477 90` becomes weaponsmithing rank 18; round nonmultiples up. Preserve `-1` as no skill.
  Reject unsupported skill mappings with a record-specific diagnostic; never interpret an old
  skill id as an ability or silently make an invalid recipe free. Test repeated save/load.
- Update validation, listings, previews, OLC, admission, and resolution together. The existing
  random check caps the roll at 151; merely swapping rank 18 for skill 90 changes the odds.
  Keep that check in legacy-equivalent units (rank and threshold multiplied by the divisor),
  retaining success, retry, and failure-consumption policy. Unsupported output/ingredient vnums
  disable execution with a diagnostic while keeping the record editable.
- Resolve consumable inventory requirements for storable `ITEM_MATERIAL` prototypes into
  shared material quantities using Decision 2. Aggregate quantities for duplicate mappings before
  checking or spending. Exact gems, fossil eggs, blueprint items, and unique quest components
  remain objects. `REQ_FLAG_IN_ROOM` and `REQ_FLAG_NO_REMOVE` keep their object semantics;
  `REQ_SAVE_ON_FAIL` applies to material debits as well as object removal.
- Preview the resolved requirements. Recheck all balances, objects, and output availability
  before consuming any input. A saved catalog record need not be rewritten to resolve its
  material requirements; the existing requirement flags and prototype are sufficient.
- Successful catalog and mold creation award the editor's existing output-level craft experience
  (`MAX(CREATE_BASE_EXP, output_level * CREATE_BASE_EXP)`) on the selected ability, once.
  No-skill recipes and catalog retries grant none. The catalog's timer moves to the activity
  manager with its existing retry policy, as specified in Decision 11.

### 8. One supply-order implementation

Unassign room 370 and remove the `crafting_quest` registry binding and kit `autocraft` execution
path when the modern `supplyorder` handles all new orders. Settle old state under `CrMg` stage 2:

- A completed, unclaimed order receives its saved gold, quest points, and character experience
  once through the existing award helpers. Do not reinterpret these as artisan or talent points.
- For an unfinished valid order, refund the material units represented by completed installments:
  `(AUTOCQUEST_MAKENUM - remaining) * SUPPLYORDER_MATS`, currently `(5 - remaining) * 3`.
  Map its `Cmat` material id through `obj_material_to_craft_material()`, extended so generic
  wood (`MATERIAL_WOOD`) and burlap map as Decision 2 maps their prototypes. Then clear
  the order with a message. This preserves recorded material investment without awarding an
  unearned completion reward. Drain in-flight old autocraft before cutover; its admission consumes
  materials before its timer decrements `remaining`.
- Invalid, unmappable, or capped settlements retain their source state and do not advance stage
  2\. Log them for review instead of silently discarding holdings or partially paying a reward.
- The legacy order tags are `Cvnm`, `Cmnm`, `Cqps`, `Cexp`, `Cgld`, `Cdsc`, and `Cmat`.
  Keep the compatibility readers for returning characters and serialize unresolved state.
  `CPts` remains clan points and is untouched. Modern `CrCT` supply contracts and `CrTr` training
  contracts are separate and remain intact.

### 9. Loot and pickup

The crystal-inclusive loot table (today's `USE_OLD_CRAFTING_SYSTEM` branch) becomes
unconditional after the preserved crystal-to-equipment path is verified; the other branch and
both flags go. The pickup auto-deposit
(`get_check_craft_material`) is deleted rather than enabled: bundles remain tradable through
shops and `give`, and `craftmaterials store` stays the one explicit deposit.

### 10. Old wilderness holdings and the fallback

Under `CrMg` stage 3, convert `WMat`/`Mat` records through the pre-merge
`wilderness_harvest_material(category, subtype, quality)` mapping, including its cold-iron and
adamantine exceptions. Mote categories use `wilderness_harvest_mote()` and `quantity * quality`,
as the current award function does. Freeze this compatibility mapping independently of future
harvest tuning. Aggregate additions per destination, check multiplication and balance capacity,
then credit and clear the source records together. No experience or talents are earned by moving
holdings. Invalid or overflowing data remains recoverable and unmarked; a log is not a refund.

Delete the fallback harvest, the player-facing `materials` command, gameplay access to
`stored_materials[]`, `USE_VARIABLE_QUALITY_MATERIALS`, `get_harvest_skill()`, and
`WILDERNESS_HARVEST_CRAFTING`. Retain a bounded compatibility reader and serialization of any
unconverted records; these are pending migration data, not a second spendable wallet. An elapsed
release does not prove dormant characters have logged in. Remove the remaining compatibility
data only after a verified complete conversion or an explicitly approved offline migration.

### 11. One activity lifecycle, with operation-specific resolution

Use the current `primary_activity_definition`, recheck, completion, cleanup, and `timed_step`
interfaces. The latter already supports variable delays, so catalog retries do not require a
separate scheduler. All command and special-procedure front ends use the same admission checks
and prevent another crafting or harvesting job from starting while occupied.

| Operation | Inputs and resolution | Interruption and persistence |
| -- | -- | -- |
| Editor projects, modern supply orders, golems | Preserve existing allocations, rolls, refunds, and progression | Keep current saved project progress and resume behavior; cancellation retains refundable allocations |
| Nodes and wilderness | Recheck source, award, then consume charge/resource and grant experience | Cancel on movement, combat, damage, target loss, logout, or copyover; no partial reward |
| Mold creation and kit utilities, including both reforge entry points | Stable object handles plus selected material/cost data; recheck ownership, inputs, gates, and capacity before one resolution | Keep ingredients and objects unchanged until completion; cancel on interruption or logout/copyover without spending; no new offline queue |
| Catalog | Recheck authored requirements; preserve success/retry/failure costs; award craft experience once on success | A retry remains the same activity; cancellation spends nothing; no automatic offline completion |
| Brew | Preserve alchemy roll, gold/mote costs, spell costs, failure fractions, and potion storage choice; recheck affordability and spell availability at resolution | Cancel on interruption or logout/copyover without spending; no automatic offline completion |

Remove `eCRAFT`, `eCRAFTING`, and `eBREWING` callbacks and their command blockers once their
callers have moved. Keep serialized event ids reserved where needed rather than renumbering
unrelated events. Preserve quest hooks and output delivery once; no effect, skill gain, or cost
may occur merely because a work timer starts or ticks. Existing project allocation is a
refundable reservation, not completion or an extra charge. Existing immediate `salvage` and
spellbook `scribe` operations do not acquire artificial timers.

## Phases

Each phase builds and passes `make -j$(nproc) test` followed by `make install`. Use coherent
commits and update the checkboxes so a new session can resume. Intermediate phases are development
checkpoints, not proof that a partial consolidation is ready for production. Player-facing help
changes accompany their phase in both help stores; Phase 6 is the final consistency pass.

### Phase 1: material identity and checked balances

- [x] Add `craft_balance_add(ch, material, quantity)` and `craft_mote_add()` with the overflow
  guard from `award_wilderness_harvest()`; route store, salvage, harvest including bonus motes,
  project refunds/returns, golem refunds, and load-time refunds through checked credits. Preflight
  multi-balance operations before modifying their source state (Decision 2). Done: the four
  checked helpers (`craft_balance_can_add`, `craft_balance_add`, `craft_mote_can_add`,
  `craft_mote_add`) live in `crafting_new.c`; every `+=` writer in `crafting_new.c`,
  `harvest.c`, `act.item.c` (salvage, which now rolls and preflights gold, material, and motes
  before extracting), and the load-time resize refund in `players.c` goes through them. A
  refused project refund leaves the allocation on the project and says so.
- [x] Add the prototype table from Decision 2 and consult it first in
  `craft_material_from_object()`; mark fossil eggs unstorable. Done:
  `craft_material_for_prototype()`; generic `MATERIAL_WOOD` and `MATERIAL_BURLAP` objects also
  map to ash wood and hemp (Decision 8).
- [x] Make `material_grade()` authoritative (cotton 4); use it in the targeted material pool
  while preserving unrelated random reward selection. Done for the grade table; the targeted pool
  is Phase 3a.
- [x] Tests in `test_crafting_projects.c`: each row of Decision 2 stores as its balance and
  `unstore` preserves the balance identity and quantity across save/reload and re-store; a fossil
  egg is refused without extraction; a generic high-hide bundle keeps its grade. Store and
  multi-reward salvage refuse overflow without partial credit or source loss; a capped refund
  retains its allocation. Check the intended grade for every mapped material and, in Phase 3a,
  every targeted pool material; do not assert exact tier equality for a random reward selector.

Completion evidence: a node drop of every kind either stores into its named balance and comes
back as the same material, or stays an object.

### Phase 2: progression conversion and one command surface

- [x] Add `craft_legacy_skill_equivalent()` and the conversion table to `crafting_new.c`; run
  stage 1 from the post-load reconciliation in `players.c` under `CrMg`, persisting it safely at
  entry; log old/new ranks and talent grants. Make the existing player writer publish atomically
  before enabling conversion, with failure-injection coverage. Keep the marker outside project
  reset state. Done: `craft_legacy_rank_for_skill()`, `craft_legacy_ability_for_skill()`,
  `craft_legacy_skill_equivalent()`, and `craft_migrate_legacy_skills()` (stage 1) in
  `crafting_new.c`; the marker is `craft_migration_version` beside the balances, read and
  written as `CrMg`; `load_char()` runs stage 1 before the immortal initialization and sets a
  non-saved pending flag that `enter_player_game()` publishes with `save_char_checked()`, which
  now writes a temporary file beside the live one and renames it in after flush, sync, and
  close. New characters start at the current marker.
- [x] Map legacy skill consumers to abilities and replace use-based notches with operation-level
  craft experience. Audit symbolic and indirect readers across the tree, including standalone
  reforge's fast-crafter dependency and resource display code. Preserve skill-less utility gates.
  Done: every kit utility, mold creation, node harvest, and the standalone reforge read
  abilities; legacy-unit gates read `craft_legacy_skill_equivalent()`; the per-tick fast-crafter
  notch and the admission notches are gone and `event_crafting` awards `gain_craft_exp()` once
  at completion (`20 + 10 * grade` for nodes, `craft_operation_exp(level)` otherwise); kit
  timers use `craft_legacy_kit_seconds()` (rapid talents, clamped to one tick, none for
  skill-less utilities); `resource_configs[].harvest_skill` and `get_harvest_skill()` return
  harvest abilities and `get_harvest_skill_level()` reads ranks. The `increase_skill()` cases
  and skill definitions stay until Phase 5.
- [x] Convert catalog reader/writer, validation, display, and `craftedit` to Decision 7's explicit
  ability record. Resolve ordinary material requirements against shared balances before nodes
  switch to direct credit; retain exact unique ingredients and requirement flags. Done: `Skil`
  records in either numbering convert on load (level rounded up to a rank); an unmapped id is
  kept as `CRAFT_SKILL_UNSUPPORTED` with its raw id, cannot execute, and writes back as `Skil`;
  the writer emits `Abil: <ability> <rank>`; the roll stays in legacy-equivalent units; success
  awards `craft_operation_exp()` once; consumable inventory requirements for storable
  `ITEM_MATERIAL` prototypes are checked and debited in aggregate from the balances (save-on-fail
  honored), in-room and no-remove requirements keep object semantics.
- [x] `do_craft` opens the editor and `do_craft_score` the score; remove the selector, its
  config field, the `cedit` entry, and the mode-2 gates; remove the crafting-skill listing from
  `practice`. Add common-menu discovery for catalog and mold creation without dropping their
  existing front ends while Phase 4 completes their execution changes. Done: `craft catalog [recipe]` and `craft score` dispatch from the editor; `crafting_system` is gone from the
  config struct, parser, `cedit`, constants, and `lib/etc/config`; `practice` and the guild
  procedure no longer list legacy skills.
- [x] Tests in `test_craft_training.c`: conversion boundaries 0, 1, 4, 5, 6, 48, 98, 99 and
  saved 100; chemistry 87 still admits a level-29 essence and mining 48 a mithril node. Knitting
  fills both abilities and pays once; higher rank/experience, spent talents, and paid `CrTr`
  survive. Test repeat load/save, project reset, respec, save failure, fresh characters (rank 0,
  no points), and entry from pre-migration and already-versioned files; slot 47 receives no new
  progression. Done (five tests; the essence and node gates are asserted through the legacy
  equivalent the kit and node code now read).
- [x] Tests in `test_crafting_projects.c`: old and new catalog records round-trip without a
  second conversion; unmapped ids cannot execute; 477/2077 at threshold 90 becomes weaponsmithing
  18 and uses the correct roll scale. Material balances plus exact gems/eggs satisfy a real
  catalog recipe; flags and failure costs survive, and a missing ingredient consumes nothing.
  Skill-less resize remains available; commands reach the same ranks in each former mode. Done
  (three tests through `crafts.c` test seams and the real commands).

Completion evidence: `craft`, `craftscore`, and the eligible `apprentice` tracks use the same
ranks, existing gates remain reachable, and catalog commodity requirements can spend harvested
balances. Existing projects, orders, and training contracts still load and resume correctly.

### Phase 3a: wilderness targeted gathering

- [x] Implement `harvesting-system.md` steps 1 to 7 against Phase 1's grade table and checked
  balances, with Decision 4's integration qualifications. Its gameplay tests, help, and
  acceptance criteria apply. Preserve Decision 10's old-holdings conversion mapping before
  replacing the live reward ladder. Done: `wilderness_pool_material()` names the sourced set;
  `harvest.c` carries the pool per sector, the richness/skill/tool tier, the grade-based
  difficulty, name parsing, the listing, and one `wilderness_harvest_command()` behind
  `harvest`, `gather`, and `mine`; a named material resolves through the checked credit,
  awards `20 + 10 * grade`, and a category name lists its pool. `wilderness_harvest_material()`
  stays as the frozen compatibility reader for Decision 10. `wilderness_crafting_bridge.c/.h`
  are deleted (they were in neither manifest). Tests: `test_wilderness_material_pool.c` (new,
  in both manifests) and the rewritten command scenarios in `test_gameplay_e2e.c`. The help and
  `WILDERNESS_HARVESTING.md` updates (its step 6) are folded into Phase 6 with the other help
  work. The terrain gate is unchanged, so a forest also lists cloth (the plan's "five woods
  and five hides" understated it).

### Phase 3b: node harvesting on the activity manager

- [ ] Replace the body of `do_harvest()` after node lookup with an activity per Decision 3,
  reusing the manager API and admission pattern of `start_wilderness_crafting_harvest()`;
  preserve counters, quest hooks, and rare drops. Delete the harvest branches of
  `event_crafting` and the harvest use of `GET_CRAFTING_TYPE`.
- [ ] Replace `resource_configs[].harvest_skill` legacy ids in `resource_system.c:44` with the
  harvest abilities and update its display consumers; do not feed the new ids to `GET_SKILL()`.
- [ ] Tests in `test_gameplay_e2e.c`: a node harvest credits nothing at start and one unit of
  the right balance at completion; gems and eggs arrive as objects; movement, combat, damage,
  and node movement/extraction cancel without rewards. Test two characters contesting the last
  charge, counters and quest hooks firing once, full balances/inventory, missing reward
  prototypes, and save/reload or copyover producing no early award or duplicate.

Completion evidence: `harvest <node>` in a zone and `harvest <material>` in the wilderness raise
the same balance and the same ability, and neither can be cancelled for free or paid twice.

### Phase 4: capability and activity closure

- [ ] Port mold creation/preview and the seven kit utilities to the shared execution path
  (Decision 6). Preserve authored templates, crystal effects, essence/masterwork and feat
  benefits, novice equipment, and scripts; remove only the replaced implementation and the
  unregistered `convert` branch. Both reforge front ends delegate to one operation.
- [ ] Move kit work, standalone reforge, catalog retries, and brew to the activity manager per
  Decision 11. Move kit mutation/payment to completion; retain editor project resume behavior.
  Remove old event callers, callbacks, and blockers after the replacements work.
- [ ] Implement `CrMg` stage 2 settlement before retiring room-370 orders. Drain old work;
  preserve completed rewards and refund recorded partial material investment, with checked
  save/retry behavior. Remove the room assignment, registry binding, and `autocraft` execution.
- [ ] Tests in the existing crafting and gameplay suites: shop and newbie molds can become
  equipment with crystal affects, essence outcomes, and feat benefits preserved; kit and `craft`
  entry points produce the same result, including a wood mold supplied from wilderness balances.
  Both reforge contexts hit the same validation. Utilities modify nothing at admission;
  interrupted work preserves inputs; successful completion applies
  effects, costs, quest hooks, and experience exactly once.
- [ ] Test catalog success/retry/failure and brew success/failure costs through real commands;
  no overlapping craft, catalog, brew, or harvest job is admitted. Test loss of required objects,
  changed balances/spell availability, logout/copyover, and missing output prototypes. Existing
  editor, supply-order, golem, and training persistence tests remain green.
- [ ] Migration tests: both a never-migrated file and a `CrMg: 1` file settle old orders once;
  `Cmnm: 3` refunds six recorded material units, completed orders pay their saved rewards,
  capped/invalid records survive, and `CPts`, `CrCT`, and `CrTr` are unchanged.

Completion evidence: every retained creation or utility capability works on shared progression,
balances, and activity admission. Crystals and molds have usable consumers; no timed crafting
operation still starts `eCRAFT`, `eCRAFTING`, or `eBREWING`.

### Phase 5: retire duplicate paths

- [ ] Implement `CrMg` stage 3 for old wilderness holdings (Decision 10). Delete their gameplay
  store and commands, the fallback branch of `do_wilderness_harvest()`, its old skill helpers,
  `USE_VARIABLE_QUALITY_MATERIALS`, and `WILDERNESS_HARVEST_CRAFTING` in code, example
  configuration, and docs. Keep migration readers and unresolved-data serialization independent
  of removed compile-time flags; do not remove them on a one-release deadline.
- [ ] Make the crystal-inclusive loot table unconditional; delete the other branch,
  `get_check_craft_material`, and both `USE_*_CRAFTING_SYSTEM` flags in the example header.
- [ ] Delete `enhanced_crafting_recipes.h` (no includer) if Phase 3a has not already.
- [ ] Remove skills 2071 to 2085: the `skillo()` lines (`src/magic/spell_parser.c:6813`), the
  `CRAFTING_SKILL` school and `list_crafting_skills()`, the `increase_skill()` cases, the
  old respec exception, and live defines. Delete `init_char()`'s legacy seed; Decision 1's
  equivalent floor covers starter access. Keep `players.c:load_skills()`'s old-id remap,
  reserved slots, and explicit historical ids in migration code. Preserve the distinct crafting
  feats. Search all callers, configs, special bindings, tests, and documentation for retired
  selectors and skill readers.
- [ ] Update both build manifests for every removed file and run
  `python3 scripts/ci/check_build_parity.py`.
- [ ] Tests: `CrMg` versions 0, 1, and 2 reach stage 3 with each applicable migration exactly
  once. All old material categories and quality tiers, ore exceptions, mote multipliers, duplicate
  destinations, unknown mappings, overflow, and save failure are covered. Returning pre-merge
  files still convert after live legacy definitions disappear. No invalid or uncredited input
  is erased; no ordinary command reads the old holdings as a second wallet.

### Phase 6: help and documentation

- [ ] Help, in the database and `lib/text/help/help.hlp` through the help-sync workflow:
  rewrite `CRAFTING`, `CRAFT-SCORE`, `CRAFTING-SKILLS`, `HARVEST`, `CRAFTING-KIT`,
  `APPRENTICE`, `SUPPLYORDER`, and `CRAFTS`; update retained create/checkcraft, crystal,
  essence, utility, and brew instructions. Remove mode text and obsolete `AUTOCRAFT` and
  `LEGACY ROOM-370 AUTOCRAFT QUEST` directions; Phase 3a covers wilderness topics. Review
  help deletions, renames, and conflicts explicitly under the help-sync workflow.
- [ ] Replace `docs/world_game-data/CRAFTING_SYSTEM_NOTES.md` with a description of the merged
  system; update `docs/systems/WILDERNESS_HARVESTING.md` and `docs/deployment/environments.md`;
  mark `craft-training-apprentice-skills.md` resolved and point it here.

## Verification and deployment

- Every phase: `make -j$(nproc) test && make install`, plus the parity script when files change.
- After Phase 2, use isolated copies of a maxed miner, a mid-rank knitter, a character with
  existing modern progression/training, an implementor, and a fresh character. Verify common
  displays and threshold access, idempotent migration, catalog balance spending, and untouched
  project/contract state. A helper-only test does not prove command or special-procedure routing.
- After Phase 3b, verify zone nodes and wilderness materials feed the same balances and harvest
  abilities, including the last-charge race and cancellation. After Phase 4, verify the complete
  mold/crystal/essence path, utilities, catalog, brew, and supply orders on the common lifecycle.
- Read `APP_ENV` before any local mutation or runtime work. Start the development server only
  with `MUD_PORT=4100 ./scripts/autorun/autorun.sh`. Do not edit local configuration headers or
  credential files; remove obsolete settings from templates and stop reading the old settings.
- Before release, generate a read-only conversion report against an authorized current data copy:
  old/new ranks, talent deltas, each holding's destination, pending order settlement, and unresolved
  records. Include dormant characters and object persistence, not just characters who log in for
  the smoke test. No unreviewed unmapped record may be silently discarded by the release.
- Quiesce work for cutover and take a coordinated backup of player files, rent files, the
  MariaDB object tables, the catalog, and affected help/world/config data together with the
  previous binary.
  Verify restore on an isolated copy. Player-file-only rollback can restore a balance without
  restoring a consumed object, or vice versa; code-only rollback is insufficient after conversion.
  Normal first-login conversion can remain lazy because compatibility readers survive. Production
  access, data changes, help publication, and restart require their applicable authorization.

## Recorded, not in scope

- Tool rules: equipment admission checks slot occupancy while `craft tools` looks for an
  `ITEM_CRAFTING_TOOL`; a modern-system inconsistency, not a merge concern.
- `supplyorder list` prints advice and artisan points but no offers for `select`.
- Refining and resizing handlers in `crafting_new.c` are reachable by no command; they are the
  modern system's own dormant features and their save fields stay.
- `scribe` in `magic/spellbook_scroll.c` is an immediate spell/feat-gated scroll and spellbook
  operation; it has no craft-rank wallet or work timer to merge. Alchemist bombs and mutagens in
  `alchemy.c` are class combat/magic features, not an additional material economy. Their rules stay.
- Rates: node versus wilderness yield, and node behaviour inside wilderness zones, are tuned
  after Phase 3 with the constants named in Decision 3.
- Many hide equipment recipes use tailoring rather than leatherworking; a recipe decision.
- Rare and legendary harvest tools (1254, 1255) are sold and dropped nowhere.
- New material ids, newly authored recipes, or talents. Supporting the existing catalog and mold
  recipes through the common engine is part of this consolidation.

## Ablation notes

- Dropped: new material ids, a second migration wallet, a parallel recipe language, mandatory
  refining, a scribe timer, tool-rule redesign, unrelated random-reward retuning, the supply-order
  listing fix, and a new node module. None is required to unite the traced crafting paths.
- Corrected the proposed deletions: molds and crystal creation are live, `CPts` is clan state,
  finished orders are owed rewards, and unconverted save records cannot be retired by calendar.
  These corrections preserve the requested outcome rather than shrinking the consolidation.
- Simplified: use the existing reconciliation/save path with ordered migration stages, the
  existing activity manager including its retry callback, the existing catalog requirement
  records with material resolution, and one implementation behind command aliases.
- Kept: commodity balances plus unique physical ingredients, migration compatibility data,
  atomic player-file publication using the existing index-writer pattern, existing project
  persistence, shared timers for every timed crafting operation, and explicit completion tests
  for legacy capabilities. Each has a traced consumer or data-loss boundary.
