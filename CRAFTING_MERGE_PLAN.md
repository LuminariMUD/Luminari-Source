# Crafting merge plan

Written 2026-09-21 from a trace of `master` at `dc00bc9cf`. Goal: one crafting system, so that
`apprentice`, `craft`, `craftscore`, harvesting in zones and in the wilderness, brewing, and the
project editor all read and advance the same skills and the same material store.

## Current state

Four pools exist, across three skill spaces and three material stores.

| Pool | Skills | Materials | Reachable today |
| -- | -- | -- | -- |
| Legacy kit system, `src/craft/craft.c` | skills 2071 to 2085 in the skill array, 1 to 99, raised by use in `increase_skill()` | `ITEM_MATERIAL` objects from `HARVESTING_NODE` objects spawned at random in ordinary zones | `craft` and `craftscore` in mode 1 (production), `harvest <node>`, kit workflows |
| Materials-and-motes system, `src/craft/crafting_new.c` | abilities 34 to 51 with an exp curve, ranks, talent points | `craft_mats_owned[]` and `craft_motes_owned[]` balances | `newcraft` and `supplyorder` in every mode, `craft` in mode 2, `brew`, `apprentice` |
| Wilderness harvest, `src/wilderness/harvest.c` | the harvest abilities 48 to 51 | credits the balances above | `harvest`, `gather`, `mine` in wilderness with `WILDERNESS_HARVEST_CRAFTING` on (default) |
| Wilderness fallback, `src/wilderness/resource_system.c` | mixes legacy skill and ability ids; `get_harvest_skill_level()` reads an ability id through `GET_SKILL()` | `stored_materials[]`, which nothing consumes | only with the toggle off |

`src/craft/crafts.c` (builder catalog, `crafting` command) gates on legacy skill ids; the catalog
file is not shipped.

Player-file snapshot in this checkout: 1082 characters hold a legacy craft skill of 10 or more,
718 hold 30 or more, and none holds a rank in any of the 18 new abilities.

## Target

- One skill space: abilities `START_CRAFT_ABILITIES` to `END_HARVEST_ABILITIES`.
- One material store: the craft material and mote balances.
- One command surface: `craft` opens the project editor, `craftscore` shows the score, in every
  configuration. No crafting mode selector.
- Zone nodes and wilderness resources are two sources for the same store and the same exp.
- Kit utility workflows (resize, augment, disenchant, reforge, bonearmor, redesc) survive and
  gate on ability rank. Mold-based `create` retires.
- The trainer is unchanged; it already trains the target skill set.

## Skill mapping

| Legacy skill | Id | Ability | Id |
| -- | -- | -- | -- |
| mining | 2071 | mining | 48 |
| hunting | 2072 | hunting | 49 |
| foresting | 2073 | forestry | 50 |
| knitting | 2074 | tailoring | 35 |
| chemistry | 2075 | alchemy | 36 |
| armor smithing | 2076 | armorsmithing | 37 |
| weapon smithing | 2077 | weaponsmithing | 38 |
| jewelry making | 2078 | jewelcrafting | 40 |
| leather working | 2079 | leatherworking | 41 |
| fast crafter | 2080 | none: dropped, rapid talents replace it |  |
| bone armor, elven, masterwork, draconic, dwarven crafting | 2081 to 2085 | none: only referenced in the advance table, no gate reads them |  |

Rank conversion: `rank = legacy / 5`, so 99 becomes 19, one below the trainer ceiling of 20.
Exp is set to `craft_skill_level_exp(rank)` so the next rank costs the normal amount. When a
character already holds a higher ability rank, keep it. Talent points are granted for the
converted ranks through the existing level-up grant so converted crafters are not behind a
character who earned the same ranks in play.

## Steps

Each step builds, passes `make test`, and is committed on its own.

### 1. Migrate legacy ranks and route `craft` to the new system

- Add `craft_migrate_legacy_skills(ch)` in `src/craft/crafting_new.c`, run from player load after
  the skill and ability blocks are read. It applies the mapping table, sets rank and exp, grants
  talent points, zeroes the legacy skills, and records a player-file flag so it runs once. Log
  each conversion at the `SYSERR`-free info level with the character name and the ranks.
- `do_craft` and `do_craft_score` always take the mode-2 path. Remove `CONFIG_CRAFTING_SYSTEM`
  and `CRAFTING_SYSTEM_*`: the config field, its `cedit` menu entry, the `lib/etc/config` parse,
  and every `crafting_system` check in `crafting_new.c`, `crafts.c`, `act.other.c`, and `db.c`.
- Tests: a CuTest that loads a fixture character with legacy ranks, runs the migration, and
  checks rank, exp, talent points, idempotence, and the keep-higher-rank rule.

### 2. Node harvesting credits the shared store

- In `do_harvest`, map the node material to a harvest category (metals to mining, wood to
  forestry, leather to hunting, cloth to gathering) and to a `CRAFT_MAT_*` id, then award through
  the same routine wilderness uses, `award_wilderness_harvest()`, with the minimum-skill gate read
  from the ability rank. Grant exp with `gain_craft_exp()`.
- Move the node spawn and `do_harvest` out of `craft.c` into `src/craft/harvest_nodes.c` so the
  kit code and the node code no longer share a 3700-line file. Update `Makefile.am`,
  `CMakeLists.txt`, and run `scripts/ci/check_build_parity.py`.
- Tests: harvesting a node of each material family credits the expected balance and ability.

### 3. Kit workflows gate on ability rank

- Every `GET_SKILL(ch, SKILL_*)` and `increase_skill()` in the kit special procedure switches to
  the mapped ability and `gain_craft_exp()`. Level gates that read `skill / 3` become
  `rank * 5 / 3`, which preserves the current thresholds after the rank conversion.
- Remove the `create` and `checkcraft` mold branches and the `SCMD_MINE`, `SCMD_HUNT`,
  `SCMD_KNIT`, `SCMD_FOREST` timed-craft branches; the editor and step 2 cover them.
- `crafts.c`: the catalog `CRAFT_SKILL` field becomes an ability id and `craftedit` lists abilities.
- Tests: resize, disenchant, and reforge through a kit succeed and fail on the rank gate.

### 4. Delete the duplicate stores and the legacy skills

- Remove `stored_materials[]`, `stored_material_count`, their player-file tags, the fallback branch
  in `do_wilderness_harvest`, `get_harvest_skill()`, `get_harvest_skill_level()`, and the
  `WILDERNESS_HARVEST_CRAFTING` toggle (code, `lib/.env.example`, and the four docs that mention
  it). Wilderness harvesting always credits the store.
- Remove `USE_NEW_CRAFTING_SYSTEM`: the material auto-pickup in `act.item.c` and the treasure
  hooks become unconditional. Update `mud_options.example.h`.
- Remove skills 2071 to 2085 from `spell_parser.c`, the `CRAFTING_SKILL` school, the
  `increase_skill()` cases, `list_crafting_skills()`, the `class.c` reset exception, and the
  old-id remap in `db.c`. Keep the player-file reader tolerant of the old `Skil` ids so pre-merge
  files still load and migrate.

### 5. Help and docs

- Rewrite `CRAFTING`, `CRAFT`, `CRAFTSCORE`, `HARVEST`, `APPRENTICE`, and `CRAFTING-KIT` in
  `lib/text/help/help.hlp` and the help database: one system, one skill list, both harvest
  sources. Remove the mode text.
- Replace `docs/world_game-data/CRAFTING_SYSTEM_NOTES.md` with a description of the merged
  system; update `docs/systems/WILDERNESS_HARVESTING.md` and
  `docs/ongoing-projects/craft-training-apprentice-skills.md` (mark resolved).

## Verification

- `make -j$(nproc) test && make install` after every step.
- Load a copy of a veteran player file from `lib/plrfiles` on the 4100 dev server: `craft`,
  `craftscore`, and `apprentice` show the converted ranks; `harvest` on a zone node and `mine` in
  the wilderness both raise the same balance and the same ability; a kit resize still works.
- `python3 scripts/ci/check_build_parity.py` after step 2.

## Out of scope

- No new recipes, materials, or talents.
- No change to the trainer's fees, ceiling, or duration.
- No rebalancing of the exp curve or node spawn rates.
- Production data migration runs through the normal player-load path on the first login after
  deployment; no offline batch conversion.

## Ablation notes

- A separate migration command for staff was considered and dropped; load-time conversion covers
  every character that ever logs in, and a file that never loads needs no conversion.
- Keeping `create` behind the kit as an alternate recipe path was dropped; it would keep the mold
  prototypes alive for no gain over the editor.
- Keeping the crafting mode selector as a kill switch was dropped; after step 1 there is no
  second system for it to select.
