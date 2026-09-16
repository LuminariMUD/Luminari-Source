# LuminariMUD Crafting System Reference

This document describes the crafting code in the current source tree. It covers
runtime command routing, two older crafting systems, the materials-and-motes
project editor, resource acquisition, timed work, persistence, and known
limitations. Repository source alone does not establish the configuration or
world-object prototypes used by a deployed server.

## Runtime selection and command reachability

`CONFIG_CRAFTING_SYSTEM` has these numeric values:

| Value | Constant | `craft` and `craftscore` behavior |
| -- | -- | -- |
| `0` | `CRAFTING_SYSTEM_NONE` | Report that no crafting system is implemented. |
| `1` | `CRAFTING_SYSTEM_KITS` | Run the normal `practice` handler. |
| `2` | `CRAFTING_SYSTEM_MOTES` | Open the materials-and-motes project editor or score. |

The compiled default is `0`. Operators can set `crafting_system = 0|1|2` in the
active configuration file (normally `lib/etc/config`), which is read at startup
(reboot or copyover).
In `cedit`, choose `E` (Extra Game Play Options), then `B`, and enter the menu
number, which is the stored value plus one: `1` none, `2` kits, `3` motes.
Confirming the editor save applies the choice in memory. It also writes to
disk when `CONFIG_AUTO_SAVE` is enabled; otherwise use `cedit save` to persist
it for the next startup. The file is ignored by Git, so source
control does not prove which value production uses. `lib/etc/crafts`
is also ignored and absent from a fresh clone, so the repository does not ship
any records for the older `crafting` catalog.

The selector gates commands individually rather than enabling one complete
subsystem and disabling all others:

| Command or path | Mode behavior |
| -- | -- |
| `craft`, `craftscore` | Switch as shown above. |
| `newcraft` | Opens the materials-and-motes project editor in every mode. |
| `craft golem`, `newcraft golem` | Refuse unless mode `2` is selected. |
| `destroygolem`, `golemrepair`, bone-golem animation | Refuse unless mode `2` is selected. |
| Standalone `reforge` | Refuses unless mode `2` is selected; a carried, worn, or in-room crafting kit intercepts the command even when empty. |
| `crafting` | Opens the older catalog/blueprint system in every mode. |
| Crafting-kit commands | A carried, worn, or in-room kit's special procedure intercepts them without consulting the selector; actual kit work requires a carried kit. |
| `supplyorder`, `craftmaterials`, `motes`, `salvage`, `harvest`, `survey`, `gather`, `mine` | Registered without a crafting-mode check. |

Consequently, names such as "kits mode" and "motes mode" describe the routing
of selected commands, not mutually exclusive implementations.

## Older crafting systems

Two separate older systems coexist and should not be conflated.

### Catalog and blueprint crafting

`src/craft/crafts.c` owns a `struct craft_data` catalog loaded from
`lib/etc/crafts`. A catalog record can specify:

- a produced object, required skill, and timer;
- inventory and in-room object requirements;
- flags controlling component preservation or consumption; and
- an optional `ITEM_BLUEPRINT` requirement.

The player command is `crafting`, regardless of `CONFIG_CRAFTING_SYSTEM`:

| Command | Behavior |
| -- | -- |
| `crafting` | List catalog crafts for which the character has enough skill; blueprint-only records are omitted. |
| `crafting <name>` | Start a non-blueprint catalog craft when its requirements are met. |
| `crafting <carried blueprint>` | Start the catalog record identified by the blueprint. |

This path schedules its own `eCRAFT` mud event. Builders maintain the catalog
with `craftedit`.

### Crafting-kit object workflow

`SPECIAL(crafting_kit)` in `src/craft/craft.c` intercepts commands when a
crafting-kit object is carried, worn, or in the room, even if empty. Actual
kit work requires the kit to be carried. The registered and handled commands
include:

| Command | Kit workflow |
| -- | -- |
| `create`, `checkcraft` | Create or inspect mold/material/crystal work. |
| `resize` | Resize an item in the kit. |
| `restring`, `redesc` | Change item descriptions. `restring` requires the compile-time `ALLOW_OBJECT_RETSRINGS_BY_PLAYERS` option, which the default example configuration leaves disabled. |
| `augment`, `disenchant` | Work with crystal enchantments. |
| `bonearmor`, `reforge` | Transform eligible equipment. |
| `autocraft` | Produce items for the older room-370 supply-order quest. |

The special procedure contains a `convert` branch, but the `convert` command is
not registered, so players cannot reach that branch through normal command
dispatch. Kit work uses the separate `eCRAFTING` event. Kit `restring`,
`redesc`, `resize`, and `reforge` refuse a kit that holds more than one item.
`src/craft/crafting_molds.c` defines a mold-vendor special, but nothing assigns
or registers it. It is not a reachable mold source. Molds can instead come
from world objects and starting gear (`NOOB_CRAFT_MOLD` in
`src/character/class.c`); the starting-gear path requires `USE_CONTAINER_OBJECTS`
and a loadable prototype. Source alone does not establish deployed availability.

Standalone `reforge <item name> <new type>` requires mode `2`, an inventory
weapon or armor flagged `ITEM_REFORGEABLE`, and any station mapped from the
item's material (for example, a forge for steel; unmapped materials require no
station). It preserves descriptions,
replacing only a builder-set `restring_identifier` with the new subtype name.
The kit form, `reforge <new type>`, instead uses the only item in a carried
kit and renames the result "a reforged <type>". A worn or in-room kit still
intercepts the command but refuses to perform the work. Remove kits from all
three locations to reach standalone reforging. Both forms accept an abbreviated
type name, charge half the item's value, and preserve its material when the new
subtype uses the same material family.

## Materials-and-motes equipment projects

The project implementation is in `src/craft/crafting_new.c`, with public types
in `crafting_new.h`. `crafting_recipes.c` populates compiled C recipe tables
declared in `crafting_recipes.h`; these recipes are not loaded from builder
data.

### Recipe representation

A `struct craft_recipe_data` identifies an item type and subtype. Each variant
stores a recipe skill, up to three material requirements, a phrase, and the
practical output subtype or wear slot.

The material-group enum defines hard metals, soft metals, hides, wood, cloth,
stone, refining, and resizing. The compiled equipment recipe table currently
uses only hard metals, soft metals, hides, wood, and cloth. Stone, refining,
and resizing groups have no equipment-recipe references, so the concrete
stone, coal, and dragonblood balances cannot be allocated to a normal equipment
project.

The player selects a concrete material within each required group. Material
quality contributes to output level. The recipe model has no generic field for
a named adventure component. Depositing a physical material object turns it
into a fungible character balance and discards the object's identity.

`variant_skill` is the project's one crafting skill. For an equipment project
it decides the tool, the station, the efficient and rapid talents, the
completion roll, and the experience; supply orders derive their skill the same
way.

### Reachable project editor

Mode `2` exposes the editor through `craft`; `newcraft` exposes the same editor
in every mode.

| Subcommand | Purpose |
| -- | -- |
| `itemtype` / `type` | Select weapon, armor, miscellaneous, or instrument type. Jewelry is under `misc`. `golem` is refused; use the separate `golem` subcommand. |
| `specifictype` | Select the concrete weapon, armor piece, instrument, or wear slot. Weapon and armor types are listed and accepted only when a recipe variant builds them. |
| `variant` | Select recipe variant and material/tool rules. |
| `materials` | Allocate or return required project materials, one group per `materials add <material>` command; repeat for every required group shown by `show`. |
| `enhancement`, `motes` | Configure and fund magical enhancement. |
| `bonuses` | Configure up to six object affects, then fund each with `motes add <slot>` (slots 1-6). |
| `instrument` | Configure instrument quality, effectiveness, and breakability. |
| `keywords`, `shortdesc`, `roomdesc`, `extradesc` | Configure descriptions; the first three are required after allocating the primary material. They must contain the exact variant phrase and selected primary-material description. Keywords reject dashes; maximum lengths are 100, 100, and 120 characters respectively. |
| `leveladjust` | Adjust requested output level within the implemented rules. |
| `show` / `display` / `review` / `information` | Display the current project and, once a variant is set, the required skill, DC, and output level that object setup records. |
| `check` | Report readiness: type, subtype, variant, descriptions, allocations, motes, the level cap, tool-slot occupancy, and the station. |
| `reset [part]` | No part, or an unrecognized part, runs the full equipment-project reset and its refund routines. Recognized parts (and their abbreviations): `motes`, `materials`, `enhancement`, `instrument`, `bonuses`, `descriptions`, `refine`, `resize`. `motes` also deletes bonus definitions; `materials` also clears all descriptions; `descriptions` also returns and clears recorded material allocations, so re-add the primary material before setting descriptions again. |
| `start` / `begin` | Admit a project that passes `check` to timed work, unless its DC is beyond the maximum possible check. |
| `score` | Display crafting and harvesting skill progress. |
| `equipment` / `tools` / `gear` | Display equipped support gear through the main create-command route. |
| `golem` | Enter golem construction; this separate subcommand requires mode `2`. |

A complete ordinary setup includes item type, specific type, variant, allocated
materials, `keywords`, `shortdesc`, and `roomdesc`. Allocate the primary
material before setting those descriptions; each must include the exact variant
phrase and selected primary-material description. Enhancement, bonuses,
instrument options, and `extradesc` are conditional or optional.

Material golems use `craft golem` or `newcraft golem` with
`type|size|show|reset|start` (and the `display`/`begin` aliases). Wood, stone,
and iron construction respectively require Construct Wood Golem, Construct
Stone Golem, or Construct Iron Golem. Accepted work runs as a timed craft and
finishes with an Arcana check. Starting only checks balances; nothing is
reserved. Completion rechecks the recipe amounts; if any are short, nothing
is spent and the golem project is discarded. A wood golem uses the first single
wood type with enough units at start, and completion requires that wood. A
failed roll consumes the recipe amounts; prototype and follower-capacity
failures spend nothing. Golem construction cannot start while a supply order is
held, and golem work that finishes or is cancelled clears its work method. The
immediate `craft golem animate <corpse>` or `newcraft golem animate <corpse>`
path creates a bone golem and requires Construct Wood Golem or Summon Greater
Undead. See `GOLEM-MAINTENANCE` and `BONE-GOLEM`.

### Tool admission and current availability limits

Ordinary equipment admission checks the recipe, descriptions, resources
already allocated to the project, the station, and an equipped tool selected
from the variant skill. It does not merely check stored balances.

`is_wearing_tool_for_crafting_ability()` recognizes only these slots:

| Ability | Required equipped slot |
| -- | -- |
| Tailoring | crafting needle |
| Alchemy | alchemy tool |
| Armorsmithing | armor hammer |
| Weaponsmithing | weapon hammer |
| Jewelcrafting | jewel pliers |
| Woodworking | none; no tool is needed |

There is no woodworking tool slot, so the 38 carpenter variants in the compiled
recipe table need no worn tool; the carpentry table station still applies. For
the other five abilities, admission only tests whether any object occupies the
dedicated slot; it does not check object type or values.

The reachable `craft tools|equipment|gear` display uses a different rule. It
scans all equipped slots for an `ITEM_CRAFTING_TOOL` whose value 0 matches the
ability, and it lists tailoring, armorsmithing, weaponsmithing, jewelcrafting,
alchemy, forestry, mining, hunting, and gathering, with no woodworking row. It
can therefore report `None` while an arbitrary object in a dedicated slot still
passes admission. A matching tool's value 1 is added only by
`compute_ability()` for skill listings. Equipment-completion, golem Arcana,
category-harvest, and brewing rolls read raw `GET_ABILITY()` through
`get_craft_skill_value()`, so tools do not change their outcomes. Tracked
source has one grant path: when the deployment's
local vnums header defines `NOOB_CRAFTING_TAILORING`,
`NOOB_CRAFTING_ALCHEMY`, `NOOB_CRAFTING_ARMORSMITHING`,
`NOOB_CRAFTING_WEAPONSMITHING`, and `NOOB_CRAFTING_JEWELCRAFTING`,
`newbieEquipment()` equips those prototypes directly into the matching
admission slots. The definitions are commented out in the tracked
`src/config/vnums.example.h` template. The routine runs for a level-0 character
entering the game and on staff demotion to level 1; ordinary existing characters
receive nothing automatically. Check the local header, the named prototypes,
and deployed world data before promising tool availability.

### Station sequencing and completion

The project skill comes from the selected recipe variant, so `check`, `start`,
timer rechecks, and completion require the same station whatever the command
order. `start` builds the object once to record the DC and output level. While
the work runs, subcommands that change the project are refused (`show`,
`check`, and `score` still work); `activity cancel` stops the work first.

An ordinary item starts with 60 seconds before speed modifiers; rapid-crafting
talents for the project skill shorten it. At completion the project must still
pass `check`, or nothing is made and the project is kept. The base DC is
recomputed as `10 + object level - level_adjust`, then the level-adjustment
modifier is applied. The roll adds the proficient-talent bonus and, where
applicable, +5 from Craft Wondrous Item or Craft Magical Arms and Armor.

- If the maximum possible check cannot reach the DC, completion rejects it.
- A natural 1 critically fails and resets the project without refund:
  allocated materials and motes are lost, and method, item type, subtype,
  variant, descriptions, enhancement, bonuses, instrument settings, stored
  skill, DC, and level adjustment are cleared.
- A natural 20 succeeds and marks the result masterwork.
- An ordinary failure grants crafting experience and keeps the project.
- Success creates the object, awards pending efficient-talent savings, and
  clears the project. Efficiency rolls once for each material group when it is
  added, using the project skill; removing or resetting materials discards the
  pending savings for those groups. Pending savings are not serialized and are
  lost when the character is reloaded from its saved file. An eligible object
  with enhancement or affects receives
  chainable 5-percent critical-success rolls; each successful link increases at
  least one enhancement or bonus.

`craft reset` (mode `2`) or `newcraft reset` refunds an equipment project's
allocations. An equipment project and a supply order share one project record,
so a character holds only one: `supplyorder request` and `select` refuse while
a project exists, and the project subcommands refuse while an order is held
(`craft score` and the golem commands other than starting construction still
work). `supplyorder reset`, `abandon`, and `cancel` require a held order; they
return the materials no finished item used and clear its progress.

## Implemented but unreachable materials-and-motes handlers

`do_newcraft()` contains static handlers for motes refining, motes resizing,
surveying, old room-node harvesting, and a separate equipment mode. No
registered command invokes `do_newcraft()` with the corresponding
`SCMD_NEWCRAFT_REFINE`, `RESIZE`, `SURVEY`, `HARVEST`, or `EQUIPMENT` subcommand.
They are not player entry points in the current command table.

Consequences include:

- the `refining_recipes` path is unreachable, and no `refine` command exists;
- the registered `resize` command is the crafting-kit workflow, not motes
  resizing;
- registered `survey`, `harvest`, `gather`, and `mine` use the separate
  wilderness/legacy handlers described below; and
- `craft equipment|tools|gear` in mode `2`, or the equivalent `newcraft`
  arguments in any mode, remain reachable through the create route and call
  `newcraft_show_tools()` rather than the dormant equipment-mode handler; and
- potion creation uses `brew`, scroll creation uses `scribe`, and no registered
  command creates wands. `recharge` is not wand creation: it requires a carried
  wand or staff, Craft Wand for a wand or Craft Staff for a staff, and 5,000
  gold; a device already at maximum charges is unchanged.

Player-reachable timed materials-and-motes work is ordinary creation, material-
golem work in mode `2`, and the general `supplyorder` contract path. The dormant
handlers still matter to save compatibility and source maintenance but must not
be advertised as commands.

### Supply-order systems and interception

`supplyorder` has three handlers across two systems. In room 370, the
`crafting_quest` room special intercepts it for the older
`supplyorder new|complete|quit` quest, which is fulfilled with materials in a
carried crafting kit and repeated `autocraft` commands.

A mobile bound to the builder-visible "New Supply Orders" special intercepts
`supplyorder` in its room and runs the general contract parser below, so every
subcommand works there. Tracked source does not bind this special, but builders
can assign it. The parser's quartermaster checks still require an NPC flagged
`MOB_QUARTERMASTER` (mob flag 99, "Quartermaster") in the room, and `start`
still requires the contract's crafting station, so flag the bound mobile or
place a quartermaster with it.

Outside the room-370 interception, the materials-and-motes contract parser
accepts `list|available`, `select|choose <n>`, `request`, `show|status|info`,
`start|begin`, `material(s)`, `complete|finish`, `reset`, `abandon|cancel`, and
`cooldown(s)|timers`. Listing, selecting, requesting, and completing require a
quartermaster in the room. Starting requires the station mapped from the
contract recipe skill, and timed work rechecks that station.
`list|available` shows artisan points and request advice, not offers or offer
numbers. `select|choose <n>` can accept a generated offer, but no command
displays the offers first. Selecting or running `cooldown` refreshes eligible
empty offer slots and replaces an offer that has no recipe variant; `list` and
`show` do not. Material allocation uses
`supplyorder material add <material>` or `supplyorder material remove`
(`materials` is also accepted). See the shared project record above.

Requests and offers use weapon, armor, and miscellaneous recipes only. A
selected contract saves its type and quality tier (`CrCT`). A quality contract
accepts only materials graded above its tier: superior needs grade 2 or better,
exceptional grade 3 or better. Each finished item takes its materials only when
every group can pay for it, every new order starts with no progress, and
`complete` needs exactly the ordered quantity.

## Timed activity lifecycle

Reachable materials-and-motes creation, golem work, and supply-order work use
`PRIMARY_ACTIVITY_CRAFT` in the shared activity manager. Wilderness category
harvesting uses `PRIMARY_ACTIVITY_HARVEST`. The older `crafting` catalog instead
uses `eCRAFT`, and crafting-kit work uses `eCRAFTING`.

Craft activities require hands and attention. Committed movement, combat,
damage, invalid targets, or a failed recheck can end work. Ordinary equipment
readiness and admission check only tool-slot occupancy, not the tool's type
or values; the timer does not recheck it. Category harvesting never requires a tool; a
harvest-tool prototype carried or worn only raises the quality floor when the
round completes. The tool recheck belongs to the unreachable node-harvest
adapter, not these reachable timed paths.

`activity` shows current work and supports `cancel`, `pause`, and `resume` when
the activity's capabilities permit them. Offline time does not advance a saved
craft duration: losing the activity owner preserves the remainder. Login,
reconnect, and copyover call `resume_craft_activity()`, which attempts to hand a
positive saved duration and method back to the activity manager. Admission can
still reject invalid runtime state. The persistence exceptions below impose
additional method-specific limits.

Supply-order offers use wall-clock timestamps and lazy refresh instead of the
saved activity countdown.

## Resource storage and acquisition

### Separate character stores

The project system keeps ordinary crafting material balances in `GET_CRAFT_MAT()`
and mote balances in `GET_CRAFT_MOTES()`, separately from project fields.

`craftmaterials` (alias `craftmat`) lists material and elemental-mote balances.
`craftmaterials store <item>` deposits a physical `ITEM_MATERIAL` object, and
`craftmaterials unstore <quantity> <material>` recreates a physical bundle from
the balance. The bundle records its crafting material in value 1, so storing it
again keeps a hide's grade. The meaning of the separate `materials` command is
compile-time selected: with the shipped `USE_VARIABLE_QUALITY_MATERIALS`
template it displays wilderness material storage and accepts optional
`details`; under `USE_BASIC_MATERIALS` it routes to the crafting-balance
handler.

### Wilderness and object paths

The compiled category-harvest integration is `src/wilderness/harvest.c`.
`do_harvest()` routes a wilderness category there unless the player names an
explicit older room node. The `WILDERNESS_HARVEST_CRAFTING` key in `lib/.env`
defaults true and is checked at admission and again during timed work:

- when true, completed category harvests map supported resources directly to
  `GET_CRAFT_MAT()` or `GET_CRAFT_MOTES()` balances; and
- when false, category harvesting falls back to the earlier immediate
  wilderness-material storage.

`gather` and `mine` are separate command front ends implemented in
`src/wilderness/resource_system.c`, but when the toggle is true both enter the
same timed category-harvest activity and credit the same crafting balances as
`harvest <category>`. The false-toggle fallback and explicit older nodes retain
their earlier behavior. See [Wilderness Harvesting](../systems/WILDERNESS_HARVESTING.md)
and [Craft Activity Lifecycle](../systems/CRAFT_ACTIVITY_LIFECYCLE.md).

`salvage <item>` requires `FEAT_SALVAGE` or `TALENT_SCAVENGER`. It accepts a
carried, takeable, non-`ITEM_NOSAC` item, subject to gold-capacity preflight.
Only an `ITEM_CONTAINER` that still holds objects is refused; any other holder,
such as an `ITEM_AMMO_POUCH` quiver, is destroyed together with its contents.
Accepted salvage always destroys the item and awards at least 1 gold (15 percent
of object cost). It independently rolls a material chance of the salvager's
level / 3 + 10 percent, and half that chance for a mote from each nonempty
affect, where a recognized material or affect mapping exists.

`src/wilderness/wilderness_crafting_bridge.c` and its header are not in either
build manifest and have no compiled caller. `src/craft/enhanced_crafting_recipes.h`
is likewise not included by compiled source. They describe inactive designs,
not runtime integration. The older
`docs/testing/WILDERNESS_CRAFTING_INTEGRATION_TESTING.md` guide also targets
that inactive design and is retained only as explicitly marked historical
material, not as a valid command checklist.

Wilderness harvesting supplies fungible resources; it is not a generic system
for recipe-specific quest, boss, dungeon, or scripted key components.

## Persistence and resume limits

`src/player/players.c` saves and loads:

- on-hand and allocated material/mote balances;
- item type, subtype, recipe, variant, `crafting_method`, completion skill/DC,
  output level, `level_adjust`, descriptions, and remaining duration;
- enhancement, affects, refining fields, resize fields, and `survey_rooms`;
- supply-order slots, offers, and cooldowns, and a selected contract's type
  and quality tier (`CrCT`);
- a golem project's type, size, and chosen wood (`CrGo`); and
- instrument type, quality, effectiveness, breakability, and mote allocations.

Golem material and mote requirements are recomputed at completion from the
saved type and size, and a wood golem uses its saved wood, so resumed golem
work can finish.

Resize fields are serialized, but load processing refunds allocated resize
materials and clears the resize size, material, method, and duration. Resize
work is cancelled at load rather than resumed.

Other positive saved methods, including ordinary creation and supply-order
work, are handed back to the activity manager on login, reconnect, and
copyover. Their normal activity rechecks still apply.

## Source and validation map

| Concern | Primary source or validation |
| -- | -- |
| Mode constants/config parsing | `src/core/utils.h`, `src/core/db.c`, `src/olc/cedit.c` |
| Command registration | `src/core/interpreter.c` |
| Catalog/blueprint records, `crafting`, `craftedit`, `eCRAFT` | `src/craft/crafts.c`, `lib/etc/crafts` |
| Kit special procedure, legacy harvest, autocraft quest | `src/craft/craft.c` |
| Motes project editor and craft activity | `src/craft/crafting_new.c`, `crafting_new.h` |
| Compiled equipment recipes | `src/craft/crafting_recipes.c`, `crafting_recipes.h` |
| Material/mote names | `src/core/constants.c` |
| Character persistence | `src/player/players.c` |
| Shared activity command/manager | `src/events/activity_manager.c` |
| Category harvest and crafting rewards | `src/wilderness/harvest.c` |
| Wilderness storage, `gather`, and `mine` | `src/wilderness/resource_system.c` |
| `materials` and `survey` commands | `src/act/act.informative.c` |
| `salvage` | `src/obj/act.item.c` |
| Potions, bombs, and scrolls | `src/craft/brew.c`, `src/craft/alchemy.c`, `src/magic/spellbook_scroll.c` |
| Unbound mold-vendor special; starting mold | `src/craft/crafting_molds.c`; `src/character/class.c` (`NOOB_CRAFT_MOLD`) |
| Uncompiled designs | `src/wilderness/wilderness_crafting_bridge.c`, `src/craft/enhanced_crafting_recipes.h` |
| Player help mirror | `lib/text/help/help.hlp`, `sql/components/help_crafting_entries.sql` |
| Core crafting/harvest tests | `unittests/CuTest/test_gameplay_e2e.c` |
| Project, supply-order, golem, storage, and reforge tests | `unittests/CuTest/test_crafting_projects.c` |
| Legacy/supply skill tests | `unittests/CuTest/test_spells_skills_production.c` |
| Crafted-instrument tests | `unittests/CuTest/test_bardic_performance.c` |

Test coverage is narrower than the table names imply. The owned-craft fixture
manually seeds the unreachable survey adapter and calls `resume_craft_activity()`
directly; it does not exercise login, reconnect, copyover, or command admission.
Material-golem tests call the completion helper directly, while bone-golem tests
do cover the immediate command path. Wilderness tool-quality tests cover
harvest tools, not ordinary crafting tools. `test_crafting_projects.c` drives
equipment admission, the edit lock while work runs, supply-order commands, the
New Supply Orders special, golem start and cancellation, material storage, and
reforge through their command handlers, and it calls the create and golem
completion routines directly. No test lets a crafting timer run to completion,
so the timed create, material-golem, and supply-order lifecycles are not covered
end to end. Tests also do not establish a deployment's selected mode or
world-object availability.
