# Wilderness harvesting of specific node materials

Tracking issue: to be filed (supersedes the reward model of #145 / PR #180).
Written 2026-09-18 from a trace of `master` at `a9bd7add1`; verified against the live game on
2026-09-19 with the game-master account. Line numbers refer to `a9bd7add1`.

Status: plan approved by the owner. Implemented as Phase 3a of
`crafting-consolidation-assessment.md` (issue #212) on `feat/212-crafting-consolidation`; the
step list below records what is done. Help and the wilderness harvesting document (step 6) are
delivered with that plan's Phase 6.

## The one crafting system this work targets

There is exactly one crafting system that a player can use end to end on the live server, and
every change in this plan feeds it. It is the **materials-and-motes system** in
`src/craft/crafting_new.c`. Its parts, each confirmed by typing it in the live game:

| Step | Command | What it does |
| -- | -- | -- |
| Hold materials | `craftmaterials` | Shows the crafting balance: a per-character count of each of 37 materials plus 8 motes. Not inventory. No items, no weight, no per-unit quality. |
| Bank a material item | `craftmaterials store <item>` | Consumes a material item from inventory and adds to the matching balance. Verified: a steel bar became `1 steel`. |
| Make things | `newcraft ...` | The project editor. Recipes need material groups (hard metals, soft metals, hides, wood, cloth); the player picks the specific material in the group with `newcraft materials add <material>`, and that choice sets the item's material and adjusts its level. Spends from the balance. |
| Contracts | `supplyorder ...` | Paid orders that consume balances. |
| Motes | `motes` | Elemental mote balances for bonuses. |

Live configuration: `lib/etc/config` has `crafting_system = 1`, so `craft` opens PRACTICE.
`newcraft` reaches the editor in every mode, so the system above is reachable regardless.

### Where the balance gets its materials today

1. **Object nodes.** At boot, "Placing Harvesting Nodes" (`reset_harvesting_rooms()`,
   `src/craft/craft.c:678`) drops object 811 into about one in 33 outdoor rooms. The node's
   material sets its name; there are 16 node types. `harvest <node>` (`do_harvest`,
   `craft.c:3207`) runs a 30-second timer and hands the player a **material item**. Verified in
   the Llyrath Forest, room 2051328: a steel vein produced a bar of obsidian, then a steel bar.
2. **Wilderness category harvest.** `harvest <category>` in a wilderness room (`harvest.c`) takes
   six seconds and adds **directly to the balance** with no item. Verified at coordinate
   -61, 93: `harvest vegetation` added `2 silk`.
3. **Shops.** Jufus in room 369 sells material items (bars, cloth) that store like node items.

### What is not reachable, and is therefore not part of this work

- The room-based survey and harvest in `crafting_new.c` (`newcraft_survey`, `newcraft_harvest`,
  `assign_harvest_materials_to_word`). No command in `cmd_info[]` passes their subcommands.
  Dead.
- Refining and resizing in `crafting_new.c` (`newcraft_refine`, `SCMD_NEWCRAFT_REFINE`). Same
  reason. Dead. Consequence: bronze, steel, cold iron, alchemical silver, satin, and silk are
  **raw** materials in practice, because nodes drop them and nothing refines them.
- Auto-store on pickup (`get_check_craft_material`, `src/obj/act.item.c:2289`). Compiled out
  under `USE_OLD_CRAFTING_SYSTEM`.
- `src/wilderness/wilderness_crafting_bridge.c`. Unreferenced.
- The blueprint command `crafting` (`do_craft_with_kits`). A separate item-recipe system; it
  does not read or write the balance and is out of scope.

## What the node system actually delivers

Sixteen node types, from `do_harvest` in `craft.c:3300` onward. Minimum skill is the
harvesting skill needed to touch the node at all. The item's material decides which balance
`craftmaterials store` credits (`obj_material_to_craft_material()`, `crafting_new.c:3455`).

| Node type | Yields (items) | Min skill | Balance credited |
| -- | -- | -- | -- |
| steel vein ("dull ore") | bronze 40%, iron 35%, steel 21%, onyx, obsidian | 1 | bronze, iron, steel; gems cannot be stored |
| cold iron vein | cold iron 48%, iron, onyx | 35 | cold iron, iron |
| mithril vein ("bright ore") | mithril, ruby, sapphire | 48 | mithril; gems cannot be stored |
| adamantine vein ("sparkling ore") | adamantine, platinum, diamond, emerald | 61 | adamantite, platinum; gems cannot be stored |
| silver vein ("dull speckled ore") | copper, silver, alchemical silver, onyx, obsidian | 1 | copper, silver, alchemical silver |
| gold vein ("yellowish ore") | gold, platinum, ruby, sapphire, diamond | 1 | gold, platinum |
| wood ("harvest tree") | alderwood, yew, oak logs, fossil eggs | 1 | **nothing**: logs have material WOOD, which maps to no balance; eggs map to stone |
| darkwood | darkwood log, fossil dragon egg | 38 | **nothing** for the log |
| leather ("game live area") | low 82%, medium 12%, high 6% quality hide, fossil eggs | 1 | **low grade hide for all three grades** |
| dragonhide | high quality hide 70%, dragonhide 30% | 58 | low grade hide, dragonscale |
| hemp / cotton / wool | hemp, cotton, wool cloth | 1 / 5 / 10 | hemp, cotton, wool |
| velvet | velvet | 25 | **nothing**: velvet has no balance |
| satin | satin | 31 | satin |
| silk | velvet, satin, silk | 38 | satin, silk |

Two gaps follow, and they matter for what the wilderness must cover:

- **No node can put wood into the balance.** Every wood node yields logs whose material has
  no balance mapping. The five wood balances (ash, maple, mahagony, valenwood, ironwood) are
  fed today only by the wilderness ladder.
- **Hide grade is lost at the store step.** Medium and high quality hide items store as low
  grade hide. Medium, high, and pristine hide balances are fed today only by the wilderness
  ladder.

## What #145 built and why it fails the requirement

PR #180 (b7483d370, merged 2026-09-13) routed `harvest <category>` into the balance, which is
the right destination, but chose the material by a quality roll:

- `wilderness_harvest_material()` (`harvest.c:104`) maps four categories to a fixed five-rung
  ladder and the rolled quality tier picks the rung. The player cannot ask for copper.
- The coordinate's subtype (`determine_harvested_material_subtype()`,
  `resource_system.c:1866`) is ignored except for two ore exceptions.
- Copper, zinc, coal, silver, gold, platinum, stone, cotton, bronze-by-choice, and cold iron
  are unreachable or luck-only from the wilderness.
- The other six categories pay motes only; the node system has nothing equivalent, so that
  part is not in conflict and stays.

## The requirement, in one sentence

A player in the wilderness sees which balance materials exist at that coordinate, targets one
by name (`harvest copper`), and after one full round that exact material is added to the
balance the materials-and-motes system spends from. Skill, the richness of the spot, and the
five harvest tools decide how high a grade of material the player can reach.

## Decisions (owner, 2026-09-18)

1. **Reuse, do not reinvent.** The material table is the group-and-grade ladder already in
   `crafting_new.c` (`determine_material_type_by_group_and_grade()`, `:220`), extended with
   the node-drop materials that the ladder omits (below). The existing richness bands
   (0.3/0.5/0.7/0.9) are the richness tiers. No new settings.
2. **Rarer is harder.** Difficulty rises with the material's grade.
3. **Quality is grade access, the same as nodes.** In the node system a material's quality is
   its grade: a steel vein's tiers are bronze, iron, steel. In the wilderness the player's
   quality tier for an attempt is the highest of three things: the skill roll, the richness
   of the coordinate, and the best harvest tool carried or equipped. A material can be pulled
   only if its grade is at or below that tier. No per-unit quality, no yield bonus.
4. **Harvest tools exist to reach higher grades.** A tool's tier is a guaranteed floor on the
   quality tier. The legendary tool (1255) guarantees grade-5 access wherever the terrain has
   that group. The poor tool (1251) guarantees grade 1, which is always available; it does
   nothing, and that is a world-building fact.
5. **Delete `src/wilderness/wilderness_crafting_bridge.c`** in this work.

## Design

### The material pool per category

Group membership comes from `craft_group_by_material()` (`crafting_new.c:692`) and grade from
`material_grade()` (`:640`). A material is in the pool if it has a source in the live game: it
is on the `crafting_new.c` ladder, or a node drops an item that stores into it. Materials
with no source anywhere (brass, linen, dragonmetal, dragonbone, dragonblood, bone) are
excluded; adding them is a separate content decision.

| Category | Groups | Pool with grade |
| -- | -- | -- |
| minerals | hard metals, soft metals, stone | tin 1, zinc 1, copper 1, stone 1, bronze 2, iron 3, coal 3, silver 3, steel 4, cold iron 4, alchemical silver 4, gold 4, mithril 5, adamantite 5, platinum 5 |
| wood | wood | ash 1, maple 2, mahagony 3, valenwood 4, ironwood 5 |
| game | hides | low grade hide 1, medium grade hide 2, high grade hide 3, pristine grade hide 4, dragonscale 5 |
| vegetation | cloth | hemp 1, flax 2, wool 3, cotton 4, silk 4, satin 5 |

`material_grade()` has no case for cotton; step 1 adds it at grade 4 to match the ladder.

Which categories a sector allows is unchanged (`can_harvest_resource_in_terrain()`,
`resource_system.c:2485`). A mountain therefore lists all fifteen minerals, plus the cloths and
hides its terrain allows; a forest lists the six cloths, five woods, and five hides; a field
lists the six cloths and five hides.

### What the player can actually pull: the quality tier

For one attempt, the quality tier is the highest of:

- the skill roll tier from `calculate_harvest_quality()` (`resource_system.c:2780`);
- the richness tier of the coordinate for that category, from the 0.3/0.5/0.7/0.9 bands of
  `calculate_current_resource_level()` (`:89`);
- the best harvest tool tier from `wilderness_harvest_tool_quality()` (`harvest.c:86`).

A material can be pulled only if `material_grade(material) <= tier`. The listing shows each
material's grade and marks which grades the player is guaranteed by tool and richness alone.

### Command surface

- `harvest` with no argument lists the pool grouped by category, one line per category, in
  grade order with the grade shown, plus the abundance word. Mote categories keep their
  current line.
- `harvest <material>` starts the full-round action for that material. Matching uses
  `crafting_materials[]` names with `is_abbrev()` on the full argument, so
  `harvest low grade hide` works. A material outside the pool is refused with the reason:
  not in this terrain, or depleted.
- `harvest <category>` for a material category lists that category's pool and starts nothing.
  For a mote category (herbs, crystal, water, salt, clay) it starts the mote harvest as today.
  `stone` names the material when minerals are allowed; otherwise the mote path.
- `gather` and `mine` are the same command limited to their categories, as today.
- Legacy node objects in the same room still win: `do_harvest()` checks for object 811
  matching the argument before the wilderness path. Unchanged.

### Resolution when the round completes

- Skill: `harvesting_skill_by_material()` (`crafting_new.c:366`), with the proficient talent
  and the Miner feat bonus as today.
- Difficulty (decision 2): `get_harvest_difficulty()` for the category plus
  `material_grade * 5`, then the existing terrain success modifier.
- Failure: existing message, one unit of depletion, the small experience award.
- Grade check (decision 3): on success, compute the quality tier. If the material's grade is
  above the tier, nothing is credited, the message says the material is beyond reach this
  time, no depletion is applied, and experience is awarded as for a failure.
- Quantity: `dice(2, 2)`, plus `dice(2, 2)` on a natural 100, plus the efficient talent bonus.
- Credit: `GET_CRAFT_MAT(ch, material) += quantity` with the overflow guard from
  `award_wilderness_harvest()`. Message names the material and its grade.
- Depletion, cascades, conservation: unchanged, keyed by the material's category.
- Experience: `20 + 10 * material_grade` on success, matching nodes.
- Bonus motes: unchanged.

### Configuration

`WILDERNESS_HARVEST_CRAFTING` keeps its meaning. `FALSE` still restores the pre-#180 storage
behavior. No new setting.

## Ablation

- Dropped: a new subtype-to-material table. The ladder plus the node-drop set is the table.
- Dropped: any raw-versus-refined rule. Refining is unreachable, so node drops define what is
  raw.
- Dropped: a per-unit quality on balances and any yield bonus from quality (decision 3).
- Dropped: the richness band as a hard cap on the pool; it is one of three tier inputs
  (decision 4).
- Dropped: `wilderness_crafting_bridge.c` and its header (decision 5).
- Dropped: requiring the old node tools (sickle, pickaxe, knife, axe). Not required today,
  not asked for.
- Dropped: touching the node object system, the store step, `newcraft`, or `supplyorder`.
  The wilderness only needs to feed the same balance they already use.
- Kept: the full-round activity, terrain gate, depletion, experience, mote categories, the
  toggle, aliases, node dispatch order, and the tool VNUM lookup. Each has a traced consumer.

## Steps

Update this list with every commit, so a new session can resume from it.

- [x] Step 1: Expose `determine_material_type_by_group_and_grade()` through `crafting_new.h`.
  Add the cotton case to `material_grade()`. Add `wilderness_pool_material()` returning
  whether a material is in the sourced set above. Delete `wilderness_crafting_bridge.c`
  and `.h`, remove them from `Makefile.am` and `CMakeLists.txt`, run
  `python3 scripts/ci/check_build_parity.py`. (The bridge was in neither manifest; the
  selector was already exported.)
- [x] Step 2: In `harvest.c`, add `wilderness_material_pool(ch, category, out[], max)`,
  `wilderness_quality_tier(ch, category, success, rank)`, and
  `wilderness_material_refusal(ch, material)`. Replace `wilderness_harvest_material()`
  and its ladder arrays. Keep `wilderness_harvest_mote()`. (The ladder reader is retained,
  unused by live harvesting, as the consolidation plan's Decision 10 compatibility reader.)
- [x] Step 3: Change `struct wilderness_harvest_context` to carry `material` (0 for a mote
  harvest). Update `start_wilderness_crafting_harvest()` and
  `complete_wilderness_harvest()` per the resolution section.
- [x] Step 4: Replace `show_harvestable_resources()` and the `parse_resource_type()` use in
  `do_wilderness_harvest()`, `do_wilderness_gather()`, and `do_wilderness_mine()` with the
  material-aware listing and parser. Keep `do_harvest()` dispatch order in `craft.c`. (The
  three commands and the listing hand off to `wilderness_harvest_command()` and
  `wilderness_show_pools()` while the toggle is on; the pre-#180 path remains behind the
  toggle until the consolidation's Phase 5 removes it.)
- [x] Step 5: Tests. Rewrite the command scenarios of `Test_wilderness_harvest_*` in
  `test_gameplay_e2e.c` (the reader and tool-tier cases stand) and add
  `unittests/CuTest/test_wilderness_material_pool.c`
  covering: every harvestable sector yields only pool materials; every pool material
  appears in some sector; an excluded material never appears; `harvest copper` on a
  mountain credits copper after the round; a material outside the pool starts no
  activity; a grade-5 material is refused at tier 4 and pulled at tier 5; each tier input
  alone reaches tier 5; a legendary tool on a poor spot pulls grade 5; difficulty grows
  by 5 per grade. Add the file to both build manifests, rerun parity.
- [ ] Step 6 (delivered with consolidation Phase 6): Help and docs. Update HARVEST, WILDERNESS-HARVEST, GATHER, MINE, and
  HARVEST-TOOLS in `lib/text/help/help.hlp` and the help database through the help-sync
  workflow. Rewrite the reward section of `docs/systems/WILDERNESS_HARVESTING.md`. Remove
  bridge references.
- [x] Step 7: Verification. `make -j$(nproc) test` then `make install` (done; the in-game
  walk-through below is covered by the command scenarios in `test_gameplay_e2e.c`). In a
  forest, `harvest` lists five woods and five hides with grades and `harvest maple`
  credits maple wood in `craftmaterials`; on a mountain `harvest copper` credits copper;
  `harvest brass` is refused; with no tool on a poor spot `harvest mithril` is refused as
  beyond reach; holding tool 1255 it succeeds on a passing roll; `newcraft materials add     copper` then accepts the harvested copper.

## Acceptance criteria

1. Every material in the pool table is harvestable at some wilderness coordinate, and the
   result lands in the same balance that `craftmaterials store` and `newcraft` use.
2. `harvest` at a coordinate lists exactly the materials `harvest <name>` accepts there, each
   with its grade.
3. `harvest <name>` credits that material and only that material.
4. No wilderness harvest ever credits a material outside the pool.
5. Higher grade means higher difficulty, five points per grade.
6. A material can be pulled only when its grade is at or below the quality tier, and the tier
   is the highest of skill roll, coordinate richness, and best harvest tool. A legendary tool
   alone reaches grade 5.
7. Object-node harvesting, `craftmaterials store`, `newcraft`, `supplyorder`, `search`, and
   the toggle are unchanged.
8. `wilderness_crafting_bridge.c` is gone and both build manifests agree.
9. All production-linked tests pass.

## Known gaps outside this plan, recorded so they are not lost

- Wood logs, velvet, burlap, and all gems from nodes cannot be stored into the balance.
- Medium and high quality hide items store as low grade hide.
- Refining and resizing exist in code but no command reaches them.
- Rare and legendary harvest tools (1254, 1255) are not sold or dropped anywhere yet.
