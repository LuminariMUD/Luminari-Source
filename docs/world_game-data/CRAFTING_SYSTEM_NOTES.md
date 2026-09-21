# LuminariMUD Crafting System Reference

This document describes the crafting code in the current source tree after the
crafting consolidation (issue #212,
`docs/ongoing-projects/crafting-consolidation-assessment.md`). There is one
crafting economy: one skill space, one set of material and mote balances, and
one timed-work lifecycle, reached through several command spellings.

## Commands and where they lead

| Command | Handler | What it does |
| -- | -- | -- |
| `craft`, `newcraft` | `do_craft` in `src/craft/crafts.c`, `do_newcraft` in `src/craft/crafting_new.c` | The project editor, in every configuration. `craft score`, `craft mold`, `craft catalog`, and `craft golem` dispatch from it. |
| `craftscore` | `do_craft_score` | Craft and harvest ranks, experience, and next-rank thresholds. |
| `apprentice` | `src/craft/craft_training.c` | Paid training of the same ranks. |
| `crafting`, `craft catalog` | `do_craft_with_kits` in `crafts.c` | Builder catalog recipes (`lib/etc/crafts`, `craftedit`). |
| kit commands (`create`, `checkcraft`, `resize`, `redesc`, `augment`, `disenchant`, `bonearmor`, `reforge`) | `SPECIAL(crafting_kit)` in `src/craft/craft.c` | Mold creation and the kit utilities. `craft mold` reaches `create` and `checkcraft` from the editor. |
| `reforge <item> <type>` | `do_reforge_new` in `crafting_new.c` | The inventory front end of the same reforge operation. |
| `harvest`, `gather`, `mine` | `do_harvest` in `craft.c`, `wilderness_harvest_command()` in `src/wilderness/harvest.c` | Zone nodes and wilderness materials. |
| `supplyorder` | `crafting_new.c` | Paid contracts. The room-370 office is closed. |
| `brew` | `src/craft/brew.c` | Potions. |
| `salvage` | `src/obj/act.item.c` | Gold, materials, and motes from a destroyed item. |
| `craftmaterials`, `motes` | `crafting_new.c` | The balances. |

There is no crafting mode selector. `CONFIG_CRAFTING_SYSTEM`, the `cedit`
entry, and the `crafting_system` configuration line were removed; `craft`
always opens the editor and `craftscore` always shows the score.

## Skills

Every crafting and harvesting gate reads the crafting abilities 34 to 46 and
48 to 51 (`src/magic/spells.h`), stored as ability ranks with experience in
`ability_exp[]`. `crafting_skill_type()` enables eight crafts and four
harvests; bowmaking, trapmaking, poisonmaking, fishing, and cooking keep their
slots without features. `gain_craft_exp()` pays experience once per completed
operation; ranks grant talent points.

The legacy kit skills (slots 2071 to 2085) have no definitions, registrations,
or notch cases. `craft_migrate_legacy_skills()` in `crafting_new.c` converts
them once per character (CrMg stage 1, Decision 1 of the plan): a value at or
below the old seed of 4 becomes rank 0; anything above rounds up by 5 per rank;
knitting fills tailoring and gathering; chemistry becomes alchemy; fast
crafter pays talent points. Gates that were written in legacy units (node
thresholds, mold levels, essence levels, catalog thresholds) read
`craft_legacy_skill_equivalent()`, which is rank times 5 with a floor of 4.
The saved legacy values stay serialized as audit data, and `load_skills()`
still remaps the pre-2000 ids.

## Materials and motes

`craft_mats_owned[]` and `craft_motes_owned[]` (`CfMt` and the mote block in
the player file) are the only spendable stores. Every writer goes through
`craft_balance_add()` and `craft_mote_add()`, which refuse invalid ids,
non-positive quantities, corrupt balances, and overflow before the source is
spent; salvage preflights its whole result.

`craft_material_from_object()` decides what a material object stores as: an
explicit prototype table for the legacy node and shop objects (logs to their
wood grade, graded hides, velvet to cotton, burlap to hemp, fossil eggs
refused), then the material recorded on an `unstore` bundle, then the object
material. `material_grade()` is the grade table (cotton 4).

The old wilderness store (`stored_materials[]`, `WMat`/`Mat`) is pending
migration data only: `craft_migrate_wilderness_holdings()` (CrMg stage 3)
moves it through the frozen `wilderness_harvest_material()` mapping once,
aggregated and capacity-checked; no command reads it.

## Timed work

Every crafting and harvesting job is a primary activity
(`src/events/activity_manager.c`): one job per character, cancellation on
movement, damage, combat, target loss, or `activity cancel`, and nothing
spent, rolled, or delivered before completion.

| Operation | File | Target | Resolution |
| -- | -- | -- | -- |
| Editor projects, supply orders, golems | `crafting_new.c` | room | Existing allocation, roll, refund, and resume rules. |
| Wilderness harvest | `harvest.c` | room | Roll, grade access, checked credit, depletion, experience. |
| Zone node harvest | `craft.c` | the node object | Recheck node and charge, roll the drop table, credit or deliver, then spend the charge and pay. |
| Kit operations, mold creation | `craft.c` (`plan_*` / `apply_*`) | the kit object | Re-plan, apply, pay gold, quest hook, craft and legacy character experience once. |
| Reforge command | `crafting_new.c` | the item | Shared `reforge_plan()` / `reforge_apply()`. |
| Catalog | `crafts.c` | room | `timed_step`: success, retry with the recipe timer, or failure. |
| Brew | `brew.c` | room | Recheck motes, gold, and spells, then the existing roll and costs. |

`eCRAFTING`, `eCRAFT`, and `eBREWING` keep their event ids behind
`event_retired`; nothing schedules them.

## Mold creation

A mold (`ITEM_MOLD` extra flag) becomes an item through the kit's `create` or
`craft mold`. The kit holds the mold and, optionally, one crystal and one
essence. The material comes from the balance of the material named in the
item description, validated by canonical group (`craft_group_by_material()`)
against the mold's material family, with the bone-armor substitution and the
dragonscale and dragonbone rules preserved. `mats_needed` is the mold's weight
over `WEIGHT_FACTOR`, at least `MIN_MATS`, halved for Elven Crafting. The
crystal's affects and enhancement, the essence's masterwork roll with the
Masterwork, Dwarven, and Draconic Crafting feats, the level, the cost, and
the strings all apply at completion.

## Catalog

`lib/etc/crafts` records carry `Abil: <ability> <rank>`. Old `Skil` records
in either numbering convert on load (level rounded up to a rank); an unmapped
legacy skill loads as `CRAFT_SKILL_UNSUPPORTED`, cannot execute, and writes
back unchanged. The roll stays in legacy-equivalent units. Consumable
inventory requirements for storable material prototypes are checked and
debited in aggregate from the balances; in-room, no-remove, and non-storable
requirements keep their object semantics; save-on-fail applies to both.

## Persistence and migration

`CrMg` in the player file records the highest consolidation stage applied:
1 skills, 2 room-370 order settlement, 3 wilderness holdings. `load_char()`
runs the missing stages in order before the immortal initialization and marks
the character; `enter_player_game()` publishes the result with
`save_char_checked()`, which writes a temporary file beside the live one and
renames it into place after flush, sync, and close. A stage that cannot
complete (a reward that does not fit, an unmappable material, an invalid
holding) logs, keeps its records, and leaves the marker so the next login
retries. New characters start at the current marker.

## Source and validation map

- `src/craft/craft.c`: nodes, kit operations, mold creation, shared reforge.
- `src/craft/crafting_new.c`: editor, balances, conversion, settlement,
  holdings migration, reforge command.
- `src/craft/crafts.c`: catalog. `src/craft/brew.c`: potions.
- `src/wilderness/harvest.c`: wilderness materials and motes.
- Tests: `unittests/CuTest/test_crafting_projects.c`, `test_craft_training.c`,
  `test_wilderness_material_pool.c`, and the crafting scenarios in
  `test_gameplay_e2e.c`.
