# LuminariMUD Crafting System Reference

This document describes the crafting implementation in the current source tree.
It covers the runtime selector, the legacy kit system, the newer materials-and-
motes system, resource acquisition, activity lifecycle, and persistence. Server
configuration determines which player-facing `craft` workflow is available;
the repository default alone does not establish the production setting.

## Runtime selection

`do_craft()` dispatches according to `CONFIG_CRAFTING_SYSTEM`:

| Setting | Player-facing `craft` behavior |
| --- | --- |
| `CRAFTING_SYSTEM_KITS` | Uses the legacy craft/blueprint catalog. |
| `CRAFTING_SYSTEM_MOTES` | Uses the project-based materials-and-motes system. |
| Any other value | Reports that no crafting system is implemented. |

The built-in default is `0`, and `crafting_system` can be loaded from the game
configuration. Operators must inspect the deployed configuration before
claiming that either workflow is active.

The implementations coexist; enabling one does not remove the data structures,
commands, or help topics associated with the other.

## Legacy kit and blueprint crafting

The legacy path is implemented primarily in `src/craft/crafts.c` and
`src/craft/craft.c`. Its recipes are `struct craft_data` records that can include:

- a produced object;
- a crafting skill and timer;
- object requirements;
- in-room requirements such as a forge;
- flags controlling whether components survive failure or consumption; and
- an optional `ITEM_BLUEPRINT` requirement.

`craft` lists or starts the legacy crafts available to a character when
`CRAFTING_SYSTEM_KITS` is selected. Related older commands and help topics,
including `create`, crafting kits, molds, and crystals, describe separate
legacy workflows and must not be used as documentation for the motes system.

## Materials-and-motes crafting

The newer implementation is in `src/craft/crafting_new.c`, with public types and
constants in `crafting_new.h`. `crafting_recipes.c` populates compiled C recipe
tables declared in `crafting_recipes.h`; recipes are not loaded from external
builder data.

### Recipe model

A `struct craft_recipe_data` identifies an item type and subtype. Each variant
specifies:

- the crafting skill used;
- up to three material requirements;
- a description/keyword phrase; and
- the practical output type, such as weapon type, armor type, instrument type,
  or wear slot.

Each material requirement is a material group plus a quantity. The groups are
hard metals, soft metals, hides, wood, cloth, stone, refining, and resizing.
The player allocates a concrete material from the required group. Material
quality contributes to the resulting item level.

This model has no generic field for a named, recipe-specific adventure
component. Physical material objects deposited into the motes system become
fungible material balances and no longer retain their original object identity.

### Project setup

With `CRAFTING_SYSTEM_MOTES` selected, `craft` exposes the project editor:

| Subcommand | Purpose |
| --- | --- |
| `itemtype` / `type` | Select weapon, armor, instrument, or miscellaneous gear. |
| `specifictype` | Select the concrete weapon, armor piece, instrument, or wear slot. |
| `variant` | Select the recipe variant and its material/skill rules. |
| `materials` | Allocate or return the required mundane materials. |
| `enhancement` and `motes` | Configure and fund magical enhancement. |
| `bonuses` | Configure up to six object affects and their mote costs. |
| `instrument` | Configure instrument quality, effectiveness, and breakability. |
| `keywords`, `shortdesc`, `roomdesc`, `extradesc` | Configure descriptions. |
| `leveladjust` | Adjust the requested output level within the allowed rules. |
| `show` / `display` / `review` | Display the current project. |
| `check` | Report whether the project is ready to start. |
| `reset` | Reset all or part of the project and reimburse reserved resources. |
| `start` / `begin` | Admit a valid project to the crafting activity. |
| `score`, `equipment`, `tools` | Display crafting skills and equipped support gear. |
| `golem` | Enter the golem-construction workflow. |

Related command handlers provide surveying, harvesting, refining, resizing, and
quartermaster supply orders. Potion, wand, and scroll creation are adjacent
crafting features but do not use the normal equipment recipe table.

### Requirements and resolution

The player-facing `craft` command and related handlers are registered in
`cmd_info[]` (`src/core/interpreter.c`).

Normal equipment admission validates the selected recipe, allocated materials
and motes, descriptions, and required equipped crafting tool. The final skill,
DC, and maximum-possible-check rejection are calculated when the activity
completes rather than before its timer starts.

`begin_current_craft()` calls the room-station validator using the skill stored
in the project. Normal recipe selection does not initialize that field, so the
current source does not reliably enforce a station requirement before starting
ordinary equipment. Other workflows populate their state differently. Station
types represented by the system include the forge, loom, tannery, alchemy
laboratory, jewelcrafting station, and carpentry table.

A normal item begins with a base duration of 60 seconds before applicable
speed modifiers. Completion resolves a d20 skill check against the saved DC:

- a character whose maximum possible check cannot meet the DC is rejected;
- a natural 1 is a critical failure and loses reserved materials and motes;
- a natural 20 succeeds and marks the result masterwork;
- an ordinary failed check leaves the project available for another attempt;
- success creates the configured object and clears the project; and
- efficient-crafting effects can return saved materials after success.

`craft reset` is the explicit resource-refund path. Callers that reset corrupted
or invalid state can choose whether reimbursement is appropriate.

## Activity lifecycle

Motes-system creation, refining, resizing, golem work, surveying, node
harvesting, and supply-order work use `PRIMARY_ACTIVITY_CRAFT` in the shared
activity manager. Wilderness category harvesting uses
`PRIMARY_ACTIVITY_HARVEST`. There is no crafting-only scheduler or descriptor
scan.

Craft work requires hands and attention. Committed movement, combat, damage,
invalid targets, and a missing required station can cancel it. Harvesting also
rechecks its harvesting tool. Ordinary equipment creation and refining require
a crafting tool at admission but do not recheck that tool during the activity.
Offline time does not advance the timer: disconnect retires the active event
while the remaining duration stays in character state, and login/reconnection
can resume it. See
[`../systems/CRAFT_ACTIVITY_LIFECYCLE.md`](../systems/CRAFT_ACTIVITY_LIFECYCLE.md)
for the activity-manager contract.

Supply-order offers are different: their offer and cooldown timestamps use wall
clock time and refresh lazily when queried.

## Resource storage and acquisition

### Character balances

The motes system stores ordinary materials and elemental motes as character-
owned integer balances accessed through `GET_CRAFT_MAT()` and
`GET_CRAFT_MOTES()`. Allocating resources moves them into the current project;
it does not require the ordinary inputs to remain as inventory objects.

Material families include metals and alloys, precious metals, hides, woods,
cloth, stone, bone, and dragon-derived materials. Motes cover air, dark, earth,
fire, ice, light, lightning, and water.

### Acquisition paths

Current integrations include:

- wilderness category harvesting, which can award compatible material or mote
  balances through `src/wilderness/wilderness_crafting_bridge.c`;
- older explicit room/node harvesting, including physical `ITEM_MATERIAL`
  results;
- `materials store <item>`, which deposits a physical material object into the
  corresponding material balance;
- `salvage <item>`, which dismantles eligible equipment for materials and
  possible motes; and
- authored hunt/content rewards, including dragon-derived resources.

Wilderness harvesting is an exploration-based source of fungible resources. It
is not a generic recipe-key-component system: the recipe table cannot currently
require a named quest, boss, dungeon, or scripted reward while preserving that
reward's identity.

## Persistence

`src/player/players.c` saves and loads the motes-system state with the
character. This includes:

- on-hand material and mote balances;
- allocated project materials, motes, enhancement, and affects;
- recipe, variant, item type, skill roll, DC, output level, and descriptions;
- the remaining activity duration;
- refining and resizing state;
- supply-order state and cooldowns; and
- instrument configuration.

The remaining duration is the handoff between character persistence and the
activity manager. Offline time does not count down ordinary craft work.

## Source and validation map

| Concern | Primary source or validation |
| --- | --- |
| Runtime dispatch and legacy recipes | `src/craft/crafts.c`, `src/craft/craft.c` |
| Motes commands and lifecycle | `src/craft/crafting_new.c`, `crafting_new.h` |
| Compiled recipe model | `src/craft/crafting_recipes.c`, `crafting_recipes.h` |
| Material and mote names | `src/core/constants.c` |
| Character persistence | `src/player/players.c` |
| Wilderness resource bridge | `src/wilderness/harvest.c`, `wilderness_crafting_bridge.c` |
| Hunt rewards | `src/quest/hunts.c` |
| Player help mirror | `lib/text/help/help.hlp` |
| Gameplay coverage | `unittests/CuTest/test_gameplay_e2e.c` |
| Wilderness manual checks | `docs/testing/WILDERNESS_CRAFTING_INTEGRATION_TESTING.md` |

Production-linked tests cover command abbreviation, crafting activity ownership
and interruption, golem resource/lifecycle contracts, wilderness reward
mapping, tool quality, delayed harvest completion, and consumption of harvested
resources by existing crafting. They do not establish which crafting mode a
particular deployment enables.
