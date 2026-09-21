# Wilderness category harvesting

Issue: https://github.com/LuminariMUD/Luminari-Source/issues/145

`harvest <material>` uses wilderness terrain, coordinate resource levels and
regenerating depletion to credit that exact crafting material. `harvest` lists
the materials and mote categories available at the current location. Each attempt occupies one full
round (six seconds) and awards its result at completion. Movement, damage,
combat, loss of eligibility, or `activity cancel` ends the attempt without a
reward. Commands cannot start overlapping activities. The same rules apply to
`gather` and `mine` for their supported categories.

An explicitly named zone node still wins in the same room; it runs its own
five-round activity that credits the same balances (see `do_harvest()` in
`src/craft/craft.c`). `search` is reserved and has no harvesting behavior or
dependency.

## Configuration

There is no configuration. Wilderness harvesting always credits the crafting
balances; the former `WILDERNESS_HARVEST_CRAFTING` toggle and the separate
wilderness store it selected were retired by the crafting consolidation
(`docs/ongoing-projects/crafting-consolidation-assessment.md`). A character's
old wilderness holdings convert once at login (CrMg stage 3).

The maintained environment example is `lib/.env_example`, as used by deployment
scripts. `lib/.env.example` is a symlink to that same template.

## Materials, grades, and the quality tier

`harvest` with no argument lists the materials this terrain and coordinate can
yield, grouped by category with each material's grade, plus the mote
categories. `harvest <material>` starts a full-round attempt for that exact
material; a category name only lists. `gather` accepts vegetation and game
materials and herbs; `mine` accepts minerals and the crystal, salt, and stone
categories (`stone` names the material where minerals are allowed).

The pool (`wilderness_pool_material()` in `src/craft/crafting_new.c`) is the
group-and-grade ladder plus the storable node drops: 31 materials. Brass,
linen, dragonmetal, dragonbone, dragonblood, and bone have no source and are
excluded. Which categories a sector allows is unchanged
(`can_harvest_resource_in_terrain()`).

| Category | Pool with grade |
| -- | -- |
| minerals | tin 1, zinc 1, copper 1, stone 1, bronze 2, iron 3, coal 3, silver 3, steel 4, cold iron 4, alchemical silver 4, gold 4, mithril 5, adamantite 5, platinum 5 |
| wood | ash 1, maple 2, mahagony 3, valenwood 4, ironwood 5 |
| game | low grade hide 1, medium grade hide 2, high grade hide 3, pristine grade hide 4, dragonscale 5 |
| vegetation | hemp 1, flax 2, wool 3, cotton 4, silk 4, satin 5 |

Resolution (`complete_material_harvest()` in `src/wilderness/harvest.c`):

- Skill: `harvesting_skill_by_material()`, with the proficient talent and the
  Miner feat (`wilderness_harvest_rank()`).
- Difficulty: the category's difficulty plus five per grade
  (`wilderness_material_difficulty()`), then the terrain success modifier.
- Failure: message, one unit of depletion, the small experience award.
- Grade access: the attempt's quality tier is the highest of the skill roll
  (`calculate_harvest_quality()`), the coordinate's richness band (0.3, 0.5,
  0.7, 0.9), and the best harvest tool carried. A material whose grade exceeds
  the tier is "beyond your reach": nothing is credited, nothing depletes, and
  the failure experience is paid.
- Quantity: `dice(2, 2)`, plus `dice(2, 2)` on a natural 100, plus the
  efficient talent bonus, credited through `craft_balance_add()`.
- Experience: `20 + 10 * grade` on success, matching zone nodes.
- Depletion, cascades, conservation, and the node-style bonus motes are
  unchanged.

The mote categories are unchanged: each raw unit yields 1 through 5 motes for
Poor through Legendary quality, with the tool floor applied.

| Category | Crafting mote |
| -- | -- |
| water (all subtypes) | water |
| herbs (all subtypes) | light |
| stone and clay (all subtypes) | earth |
| salt (all subtypes) | ice |
| crystal: arcanite | air |
| crystal: nethermote, voidshards | dark |
| crystal: sunstone, bloodstone | fire |
| crystal: dreamquartz | light |
| crystal: frostgem | ice |
| crystal: stormcrystal | lightning |

The pre-merge quality ladder (`wilderness_harvest_material()`) is retained only
as the frozen compatibility reader for old holdings.

## Harvest tools

The authored object prototypes are in
[data/harvest-tools/harvest-tools.obj](../../data/harvest-tools/harvest-tools.obj).
VNUM definitions live in `src/config/harvest_vnums.h`, which is included by the example
VNUM configuration. Existing customized `src/config/vnums.h` files need no edits.

| VNUM | Name | Guaranteed tier | Cost |
| -- | -- | -- | -- |
| 1251 | poor harvest tool | 1 | 50 |
| 1252 | common harvest tool | 2 | 500 |
| 1253 | uncommon harvest tool | 3 | 2500 |
| 1254 | rare harvest tool | 4 | 12500 |
| 1255 | legendary harvest tool | 5 | 50000 |

The highest qualifying VNUM in top-level inventory or any equipment slot is one
of the three tier inputs at completion; the legendary tool alone reaches every
grade-5 material a spot allows. Names alone do not identify a tool. Tools
inside containers must be taken out. Tools are retained after use, do not
stack, and do not grant success or bypass terrain and depletion checks. A tool
removed before completion supplies no benefit. Zone nodes ignore harvest tools.

## Delivery and verification

Install the five prototypes into the site's world data as described in
[data/harvest-tools/README.md](../../data/harvest-tools/README.md).
The HARVEST, HARVEST-TOOLS, and related help entries live in the help database
and `lib/text/help/help.hlp`; publish them with the help-sync workflow.

Production-linked tests in `unittests/CuTest/test_gameplay_e2e.c` and
`unittests/CuTest/test_wilderness_material_pool.c` cover the pool per sector,
difficulty per grade, the tier inputs, command dispatch, delayed payout, the
tool floors, the mountain and scarce-spot cases, material mapping, overflow,
and interrupted work. Run the normal `make test` gate and follow with
`make install`. World content validation:

```sh
python3 scripts/world/wtool.py validate --paths data/harvest-tools/harvest-tools.obj --strict
python3 scripts/ci/check_build_parity.py
```
