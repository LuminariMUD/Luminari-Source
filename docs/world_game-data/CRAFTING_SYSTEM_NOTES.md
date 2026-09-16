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
dispatch. Kit work uses the separate `eCRAFTING` event.
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
The kit form, `reforge <new type>`, instead uses the first item in a carried
kit and renames the result "a reforged <type>". A worn or in-room kit still
intercepts the command but refuses to perform the work. Remove kits from all
three locations to reach standalone reforging. Both forms charge half the
item's value and preserve its material when the new subtype uses the same
material family.

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

`variant_skill` drives the admission-time tool check and supply-order skill
selection. It is not necessarily the ability rolled at equipment completion:

- weapons roll weaponsmithing;
- armor rolls armorsmithing; and
- instruments and miscellaneous items roll the ability derived from the
  completed object's material.

### Reachable project editor

Mode `2` exposes the editor through `craft`; `newcraft` exposes the same editor
in every mode.

| Subcommand | Purpose |
| -- | -- |
| `itemtype` / `type` | Select weapon, armor, miscellaneous, or instrument type. Jewelry is under `misc`. Although the parser also accepts `golem`, that value is a dead end; use the separate `golem` subcommand. |
| `specifictype` | Select the concrete weapon, armor piece, instrument, or wear slot. |
| `variant` | Select recipe variant and material/tool rules. |
| `materials` | Allocate or return required project materials, one group per `materials add <material>` command; repeat for every required group shown by `show`. |
| `enhancement`, `motes` | Configure and fund magical enhancement. |
| `bonuses` | Configure up to six object affects, then fund each with `motes add <slot>`. A bug treats variant index `0` as unset, so bonuses fail on the first variant of every recipe and on all 60 recipes that have no other variant. |
| `instrument` | Configure instrument quality, effectiveness, and breakability. |
| `keywords`, `shortdesc`, `roomdesc`, `extradesc` | Configure descriptions; the first three are required after allocating the primary material. They must contain the exact variant phrase and selected primary-material description. Keywords reject dashes; maximum lengths are 100, 100, and 120 characters respectively. |
| `leveladjust` | Adjust requested output level within the implemented rules. |
| `show` / `display` / `review` / `information` | Display the current project. Object setup can also update its stored skill, DC, and output level; this is not a read-only preview. |
| `check` | Report whether the project can be admitted. |
| `reset [part]` | No part, or an unrecognized part, runs the full equipment-project reset and its refund routines. Recognized parts (and their abbreviations): `motes`, `materials`, `enhancement`, `instrument`, `bonuses`, `descriptions`, `refine`, `resize`. `motes` also deletes bonus definitions; `materials` also clears all descriptions; `descriptions` also returns and clears recorded material allocations, so re-add the primary material before setting descriptions again. |
| `start` / `begin` | Admit a valid equipment project to timed work. |
| `score` | Display crafting and harvesting skill progress. |
| `equipment` / `tools` / `gear` | Display equipped support gear through the main create-command route. |
| `golem` | Enter golem construction; this separate subcommand requires mode `2`. |

A complete ordinary setup includes item type, specific type, variant, allocated
materials, `keywords`, `shortdesc`, and `roomdesc`. Allocate the primary
material before setting those descriptions; each must include the exact variant
phrase and selected primary-material description. Enhancement, bonuses,
instrument options, and `extradesc` are conditional or optional. The compiled
no-argument editor banner still lists `jewelry` and omits the separate `golem`
subcommand; that banner is stale relative to the parser and corrected help.

Material golems use `craft golem` or `newcraft golem` with
`type|size|show|reset|start` (and the `display`/`begin` aliases). Wood, stone,
and iron construction respectively require Construct Wood Golem, Construct
Stone Golem, or Construct Iron Golem. Accepted work runs as a timed craft and
finishes with an Arcana check. Starting only checks balances; nothing is
reserved. Completion rechecks the recipe amounts; if any are short, nothing
is spent and the golem project is discarded. A wood golem start accepts any
single wood type, but completion requires the full amount in ash wood. A failed
roll consumes the recipe amounts; prototype and follower-capacity failures
spend nothing. The immediate `craft golem animate <corpse>` or
`newcraft golem animate <corpse>` path creates a bone golem and requires
Construct Wood Golem or Summon Greater Undead. See `GOLEM-MAINTENANCE` and
`BONE-GOLEM`.

### Tool admission and current availability limits

Ordinary equipment admission checks the recipe, descriptions, resources
already allocated to the project, and an equipped tool selected from the
variant skill. It does not merely check stored balances.

`is_wearing_tool_for_crafting_ability()` recognizes only these slots:

| Ability | Required equipped slot |
| -- | -- |
| Tailoring | crafting needle |
| Alchemy | alchemy tool |
| Armorsmithing | armor hammer |
| Weaponsmithing | weapon hammer |
| Jewelcrafting | jewel pliers |

There is no accepted woodworking tool slot. All 38 carpenter variants in the
compiled recipe table map to woodworking and therefore fail admission by
construction. For the other five abilities, admission only tests whether any
object occupies the dedicated slot; it does not check object type or values.

The reachable `craft tools|equipment|gear` display and crafting-skill bonus use
a different rule: they scan all equipped slots for an `ITEM_CRAFTING_TOOL`
whose value 0 matches the ability. The display omits woodworking entirely. It
can therefore report `None` while an arbitrary object in a dedicated slot still
passes admission. Tracked source does not establish which prototypes a
deployment grants, so operators must inspect deployed world data.

### Station sequencing and completion

Type, subtype, and variant selection alone do not set `skill_type`. However,
`show` can initialize it through object setup before the first `start` when
the required prototype can be loaded. Its `display`, `review`, and `information`
aliases do the same. Equipment start and timer rechecks use the current stored
skill for station requirements. A still-zero skill requires no station;
a preview-initialized skill can require one on the first attempt. Completion
sets the final ability and DC, which an ordinary failure retains. A later
preview can recompute the stored skill again, so station behavior depends on
command order and current state, not simply first attempt versus retry.

An ordinary item starts with 60 seconds before speed modifiers. Rapid-crafting
talents use the stored skill at `start`; skill `0` gives no reduction, but
preview or completion can populate it. At completion, the base DC is recomputed as
`10 + object level - level_adjust`, then the level-adjustment modifier is
applied. The roll adds the proficient-talent bonus and, where applicable, +5
from Craft Wondrous Item or Craft Magical Arms and Armor.

- If the maximum possible check cannot reach the DC, completion rejects it.
- A natural 1 critically fails and clears the project's material and mote allocations.
- A natural 20 succeeds and marks the result masterwork.
- An ordinary failure grants crafting experience and keeps the project.
- Success creates the object, awards pending efficient-talent savings, and
  clears the project. Efficiency uses the stored skill at allocation time,
  when each material group is added; skill `0` gives no efficient-talent chance.
  Preview or completion can populate the skill, but does not retroactively
  reroll existing allocations. Pending savings are not serialized and are
  lost when the character is reloaded from its saved file. An eligible object
  with enhancement or affects receives
  chainable 5-percent critical-success rolls; each successful link increases at
  least one enhancement or bonus.

`craft reset` (mode `2`) or `newcraft reset` refunds an equipment project's
allocations. In contrast, the general `supplyorder reset`, `abandon`, and
`cancel` handlers clear the shared project fields and discard allocated
materials without refund, even when no supply order exists. `supplyorder select`
can replace an unstarted equipment project's recipe/type fields
without returning its old allocations. Use `newcraft reset` before switching
an equipment project to a supply order.

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
`supplyorder` in its room. It accepts `request`, `info|show`, `start`,
`material`, `complete`, and `reset`; `list`, `select`, and `cooldown` print
usage there, and `abandon` does nothing. Tracked source does not bind this
special, but builders can assign it.

Outside those interceptions, the materials-and-motes contract parser accepts
`list|available`, `select|choose <n>`, `request`, `show|status`, `start|begin`,
`material(s)`, `complete|finish`, `reset`, `abandon|cancel`, and
`cooldown(s)|timers`. Listing, selecting, requesting, and completing require a
quartermaster in the room. Starting requires the station mapped from the
contract recipe skill, and timed work rechecks that station.
`list|available` shows artisan points and request advice, not offers or offer
numbers. `select|choose <n>` can accept a generated offer, but no command
displays the offers first. Selecting or running `cooldown` refreshes eligible
empty offer slots; `list` and `show` do not. Material allocation uses
`supplyorder material add <material>` or `supplyorder material remove`
(`materials` is also accepted). See the shared-project reset warning above.

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
the balance. Unstoring medium-, high-, or pristine-grade hide creates generic
leather; storing that bundle again credits low-grade hide, losing the higher
grade. The runtime usage strings incorrectly call this command
`materials`. The meaning of the separate `materials` command is compile-time selected:
with the shipped `USE_VARIABLE_QUALITY_MATERIALS` template it displays
wilderness material storage and accepts optional `details`; under
`USE_BASIC_MATERIALS` it routes to the crafting-balance handler.

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
carried, takeable, non-`ITEM_NOSAC` item except a nonempty container, subject to
gold-capacity preflight. Accepted salvage always destroys the item and awards
at least 1 gold (15 percent of object cost). It independently rolls a
`level / 3 + 10` percent material chance and half that chance for a mote from
each nonempty affect, where a recognized material or affect mapping exists.

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
- supply-order slots, offers, and cooldowns; and
- instrument type, quality, effectiveness, breakability, and mote allocations.

Golem type, size, and the selected concrete golem material are not saved. Mote
requirements can be recomputed, but a loaded character can retain the golem
method and duration without the completion-critical selection. Automatic
resume therefore reaches completion and refuses it while retaining materials.

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
| Legacy/supply skill tests | `unittests/CuTest/test_spells_skills_production.c` |
| Crafted-instrument tests | `unittests/CuTest/test_bardic_performance.c` |

Test coverage is narrower than the table names imply. The owned-craft fixture
manually seeds the unreachable survey adapter and calls `resume_craft_activity()`
directly; it does not exercise login, reconnect, copyover, or command admission.
Material-golem tests call the completion helper directly, while bone-golem tests
do cover the immediate command path. Wilderness tool-quality tests cover
harvest tools, not ordinary crafting tools. No direct test covers equipment
admission/tool/station/roll chronology or the reachable timed create, material-
golem, and supply-order lifecycles. Tests also do not establish a deployment's
selected mode or world-object availability.
