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

The compiled default is `0`. Operators can set `crafting_system = N` in the
runtime `lib/etc/config` file or through `cedit`. That file is ignored by Git,
so source control does not prove which value production uses. `lib/etc/crafts`
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
| Standalone `reforge` | Refuses unless mode `2` is selected; a carried crafting kit can intercept its own kit workflow. |
| `crafting` | Opens the older catalog/blueprint system in every mode. |
| Crafting-kit commands | A carried kit's special procedure handles them without consulting the selector. |
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
carried crafting-kit object is present. The registered and handled commands
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
dispatch. Kit work uses the separate `eCRAFTING` event. Molds are created by a
mob special procedure in `src/craft/crafting_molds.c`.

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
| `materials` | Allocate or return required project materials. |
| `enhancement`, `motes` | Configure and fund magical enhancement. |
| `bonuses` | Configure up to six object affects and mote costs. A bug treats variant index `0` as unset, so bonuses fail on the first variant of every recipe and on all 60 recipes that have no other variant. |
| `instrument` | Configure instrument quality, effectiveness, and breakability. |
| `keywords`, `shortdesc`, `roomdesc`, `extradesc` | Configure descriptions; the first three are required after allocating the primary material. They must contain the exact variant phrase and selected primary-material description. Keywords reject dashes; maximum lengths are 100, 100, and 120 characters respectively. |
| `leveladjust` | Adjust requested output level within the implemented rules. |
| `show` / `display` / `review` / `information` | Display the current project. |
| `check` | Report whether the project can be admitted. |
| `reset [part]` | Refund/reset all or `motes`, `materials`, `enhancement`, `instrument`, `bonuses`, `descriptions`, `refine`, or `resize`. Resetting `materials` also clears keywords, short description, room description, and extra description. |
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
finishes with an Arcana check; a failed roll consumes the reserved resources.
Prototype and follower-capacity failures retain them. The immediate
`golem animate <corpse>` path creates a bone golem and requires Construct Wood
Golem or Summon Greater Undead. See `GOLEM-MAINTENANCE` and `BONE-GOLEM`.

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

Normal recipe selection does not set `skill_type`. On the first `start`, the
station check therefore receives skill `0` and requires no station. The
in-activity recheck sees the same value. Completion constructs the temporary
object and sets the final ability and DC. If that roll is an ordinary failure,
the project remains and retains the completion ability; a retry then requires
that ability's station, such as a forge for weapons or armor. This asymmetric
first-attempt/retry behavior is a current implementation defect, not reliable
pre-start station enforcement.

An ordinary item starts with 60 seconds before speed modifiers. A normally
initialized first attempt has skill `0`, so rapid-crafting talents do not
shorten it. An ordinary failure retains the completion skill, allowing the
rapid-talent reduction on a retry. At completion, the base DC is recomputed as
`10 + object level - level_adjust`, then the level-adjustment modifier is
applied. The roll adds the proficient-talent bonus and, where applicable, +5
from Craft Wondrous Item or Craft Magical Arms and Armor.

- If the maximum possible check cannot reach the DC, completion rejects it.
- A natural 1 critically fails and loses reserved materials and motes.
- A natural 20 succeeds and marks the result masterwork.
- An ordinary failure grants crafting experience and keeps the project.
- Success creates the object, may return efficient-crafting savings, and clears
  the project. An eligible object with enhancement or affects then receives
  chainable 5-percent critical-success rolls; each successful link increases at
  least one enhancement or bonus.

`craft reset` or `newcraft reset` is the explicit refund path for an equipment
project.

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

### Two supply-order systems

`supplyorder` has two independent implementations. In room 370, the
`crafting_quest` room special intercepts it for the older
`supplyorder new|complete|quit` quest, which is fulfilled with materials in a
carried crafting kit and repeated `autocraft` commands.

Outside that interception, the materials-and-motes contract parser accepts
`list|available`, `select|choose <n>`, `request`, `show|status`, `start|begin`,
`material(s)`, `complete|finish`, `reset`, `abandon|cancel`, and
`cooldown(s)|timers`. Listing, selecting, requesting, and completing require a
quartermaster in the room. Starting requires the station mapped from the
contract recipe skill, and timed work rechecks that station.

## Timed activity lifecycle

Reachable materials-and-motes creation, golem work, and supply-order work use
`PRIMARY_ACTIVITY_CRAFT` in the shared activity manager. Wilderness category
harvesting uses `PRIMARY_ACTIVITY_HARVEST`. The older `crafting` catalog instead
uses `eCRAFT`, and crafting-kit work uses `eCRAFTING`.

Craft activities require hands and attention. Committed movement, combat,
damage, invalid targets, or a failed recheck can end work. Category harvesting
also rechecks its harvesting tool. Ordinary equipment creation checks a tool at
admission but does not recheck that tool during the timer.

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

The project system keeps ordinary crafting materials in `GET_CRAFT_MAT()` and
motes in `GET_CRAFT_MOTES()`. Allocating a resource moves it from an on-hand
balance into the current project.

`craftmaterials` (alias `craftmat`) lists material and elemental-mote balances.
`craftmaterials store <item>` deposits a physical `ITEM_MATERIAL` object, and
`craftmaterials unstore <quantity> <material>` recreates a physical bundle from
the balance. The runtime usage strings incorrectly call this command
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
| Mold mob special | `src/craft/crafting_molds.c`, `src/spec/spec_assign_mobiles.c` |
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
