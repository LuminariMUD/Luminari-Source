-- Keep corrected crafting topics aligned with lib/text/help/help.hlp.
-- Generated from the canonical flat-file entries; safe to run repeatedly.
-- Alias cleanup is tag-scoped so unrelated topic ownership is preserved.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafting', 'CRAFT uses the crafting mode selected by server configuration: mode 0 reports
no system, mode 1 opens PRACTICE, and mode 2 opens the materials-and-motes
project editor. CRAFTSCORE uses the same mode split, but mode 2 displays the
crafting and harvesting score. NEWCRAFT opens the editor in every mode.
CRAFTING is a separate catalog/blueprint command available in every mode.

MATERIALS-AND-MOTES EQUIPMENT PROJECT
Use CRAFT in mode 2, or substitute NEWCRAFT in any mode:
  newcraft itemtype <weapon|armor|instrument|misc>
  newcraft specifictype <type>
  newcraft variant <variant>
  newcraft materials add <material>     (once per required group in SHOW)
  newcraft keywords <words>
  newcraft shortdesc <text>
  newcraft roomdesc <text>
  newcraft enhancement <1-8>            (weapons and armor, optional)
  newcraft instrument <quality|effectiveness|breakability> <amount>
  newcraft bonuses <slot 1-6> ...       (optional)
  newcraft motes add <enhancement|quality|effectiveness|breakability|1-6>
                                        (fund each value set above)
  newcraft show
  newcraft check
  newcraft start

Set the variant and allocate the primary material before the three required
descriptions. Each must contain the exact variant phrase and selected primary-
material description. Keywords reject dashes. The maximum lengths for keywords,
shortdesc, and roomdesc are 100, 100, and 120 characters.

Instrument quality, effectiveness, and breakability selectors apply only to
instrument projects.

The recipe variant sets the project''s crafting skill, which decides the tool,
the crafting station, talents, the roll, and experience. CHECK and START test
the recipe, descriptions, allocations, the station, and occupancy of the
skill''s dedicated tool slot; they do not validate the occupying object''s type
or values. Woodworking has no tool slot and needs no tool. CRAFT TOOLS uses a
different rule: it displays ITEM_CRAFTING_TOOL objects whose value 0 matches
an ability, and it has no woodworking row.

START also refuses a project whose DC no roll can reach, and rapid talents for
the project skill shorten the work. Eligible successful objects with enhancement
or affects get chainable 5-percent critical-success rolls.

CRAFTMATERIALS lists material and elemental-mote balances. STORE deposits an
ITEM_MATERIAL and UNSTORE recreates a physical bundle, which keeps its hide
grade when stored again. MOTES lists mote types and shows the bonuses
associated with a selected type. When the wilderness integration is enabled,
HARVEST, GATHER, and MINE use the same timed category harvest and can credit
those crafting balances.

Crafting is timed work. Moving, combat, damage, or a failed recheck can cancel
it, and the project cannot change until the work ends or ACTIVITY CANCEL stops
it. Completion makes an item only if the project still passes CHECK. Offline
time does not advance the saved duration. Login, reconnect, and copyover
attempt to reconstruct saved create, golem, and supply-order work. Resize is
refunded and cleared during load. A golem project saves its type, size, and
chosen wood, so resumed golem work can finish.

A crafting project and a supply order share one project record, so you hold
one at a time: SUPPLYORDER REQUEST and SELECT refuse while a project exists,
and project subcommands refuse while an order is held. CRAFT RESET (mode 2) or
NEWCRAFT RESET refunds a project''s allocations; SUPPLYORDER RESET, ABANDON, and
CANCEL give up an order and return the materials no finished item used.

OTHER CRAFTING PATHS
CRAFTING lists or starts catalog crafts and carried blueprints. A carried
CRAFTING-KIT separately handles its registered workflows. RESTRING is disabled
in the default build unless ALLOW_OBJECT_RETSRINGS_BY_PLAYERS is compiled in.
SUPPLYORDER covers a room-370 kit/autocraft quest and materials-and-motes orders,
with a general command and a builder-bindable mobile special. See its topic for
the different syntax and location requirements.

Material golems use CRAFT GOLEM or NEWCRAFT GOLEM with
TYPE|SIZE|SHOW|RESET|START and require mode 2. Bone animation is immediate:
CRAFT GOLEM ANIMATE <corpse> or NEWCRAFT GOLEM ANIMATE <corpse>. No registered
command exposes motes refining or motes resizing, and no registered command
creates wands.

See Also: NEWCRAFT CRAFT-MATERIALS CRAFTING-MOTES CRAFT-SHOW CRAFT-CHECK
          HARVEST SUPPLYORDER ACTIVITY CRAFTS CREATE CRAFTING-KIT
          GOLEM-MAINTENANCE BONE-GOLEM SCRIBE

Special thanks to Ellyanor for the original crafting help contribution.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-itemtype', 'Usage: craft itemtype <weapon|armor|instrument|misc>
       newcraft itemtype <weapon|armor|instrument|misc>

This selects the broad type for an ordinary equipment project. Jewelry-oriented
recipes are miscellaneous subtypes, so use MISC rather than JEWELRY.

ITEMTYPE does not accept GOLEM. Golem work uses the separate CRAFT GOLEM or
NEWCRAFT GOLEM workflow.

See Also: CRAFTING GOLEM-MAINTENANCE BONE-GOLEM', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-materials', 'Usage: craft materials <add|remove> <material type>
       newcraft materials <add|remove> <material type>
       craftmaterials
       craftmaterials store <item>
       craftmaterials unstore <quantity> <material>

NEWCRAFT MATERIALS (CRAFT MATERIALS in mode 2) allocates the exact amount a
selected recipe variant needs, or returns that allocation. Select item type,
specific type, and variant first, then use NEWCRAFT SHOW to see required groups
and quantities. Repeat MATERIALS ADD once per required group.

NEWCRAFT RESET MATERIALS refunds and clears project allocations. It also clears
keywords, shortdesc, roomdesc, and extradesc. NEWCRAFT RESET DESCRIPTIONS
likewise refunds and clears material allocations; re-add the primary material
before setting descriptions again. An unrecognized RESET part resets the whole
project.

CRAFTMATERIALS lists material and elemental-mote balances. STORE deposits a
physical ITEM_MATERIAL object; UNSTORE recreates a physical material bundle.
A bundle made by UNSTORE records its crafting material, so storing it again
keeps a hide''s grade.

In the default build, MATERIALS displays separate wilderness material storage;
its behavior depends on a compile-time option.

See Also: CRAFTING CRAFT-SHOW CRAFTMATERIALS MATERIALS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafting-recipes-materials', 'Topic: Crafting Materials, Tools, and Skills

NEWCRAFT MATERIALS (CRAFT MATERIALS in mode 2) allocates resources to an
equipment project. CRAFTMATERIALS lists material and elemental-mote balances;
STORE deposits ITEM_MATERIAL objects and UNSTORE recreates physical bundles,
which keep their hide grade when stored again.

NEWCRAFT TOOLS, EQUIPMENT, or GEAR (CRAFT in mode 2) lists equipped
ITEM_CRAFTING_TOOL objects whose value 0 matches tailoring, armorsmithing,
weaponsmithing, jewelcrafting, alchemy, forestry, mining, hunting, or
gathering. Woodworking has no row. A listed bonus does not change crafting,
golem, harvesting, or brewing rolls. Equipment admission uses a different rule:
any object in the matching dedicated slot passes, and woodworking needs no
tool.

MATERIALS is separate wilderness storage in the default build. When
WILDERNESS_HARVEST_CRAFTING is enabled, HARVEST, GATHER, and MINE enter the same
timed category-harvest path and can credit project-system balances. SALVAGE
requires Salvage or Scavenger and destroys an eligible carried, takeable,
non-NOSAC item. Only a container that still holds items is refused; a quiver
or other ammo pouch is destroyed together with its contents. Gold-capacity
overflow is also refused. It always pays at least 1 gold (15 percent of item
cost), then rolls a (your level)/3 + 10 percent mapped-material chance and half
that chance for a mapped mote from each nonempty affect.

Enhancement and object bonuses are funded with motes. MOTES lists mote types and
the bonuses associated with a selected type; CRAFTMATERIALS displays balances.
No registered command currently exposes motes refining or wand creation.

See Also: CRAFTING CRAFT-MATERIALS CRAFTING-MOTES HARVEST CRAFT-BONUSES
          CRAFT-ENHANCEMENT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafts', 'CRAFTING is a separate catalog of special objects and rare blueprint recipes.
It is available regardless of the configured CRAFT mode.

Usage: crafting
       crafting <catalog craft name>
       crafting <carried blueprint>

With no argument, CRAFTING lists non-blueprint catalog crafts for which you
have enough skill. A name starts a listed craft when its object and room
requirements are met. A carried blueprint identifies a blueprint-only craft.

See Also: CRAFTING-KIT CHECKCRAFT CRAFTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('vessels-and-advanced-crafting', 'Topic  : Vessels, Salvaging, and Crafting Progress
Usage  : sail [destination]
         shipdisembark
         salvage <item>
         craftscore
         craftmaterials [store <item>|unstore <quantity> <material>]
         splitenchantment
         nocraftprogress
         mine <resource>

COMMANDS:
- sail: at a sailing port, no argument lists destinations with cost, distance,
  and time; SAIL <destination> pays the fare and starts scheduled travel. The
  Sailor background waives the fare.
- shipdisembark: leaves a docked legacy ship; it refuses while not docked.
- salvage: destroys an eligible carried item for guaranteed gold and possible
  mapped materials or motes; requires Salvage or Scavenger. A quiver or other
  ammo pouch is destroyed together with its contents.
- craftscore: displays crafting/harvesting progress according to the configured
  mode.
- craftmaterials: lists project balances; STORE deposits a material object and
  UNSTORE recreates a bundle, which keeps its hide grade when stored again.
- splitenchantment: with the Wizard Split Enchantment perk and no argument,
  primes the next enchantment-school spell to affect all enemies in the room;
  it is subject to a cooldown.
- nocraftprogress: toggles crafting progress messages.
- mine: gathers supported wilderness mineral resources.

The source contains a motes refining handler, but no registered REFINE command
reaches it. REFINE is retained as a search keyword for this limitation, not as
an available command.

See Also: VESSELS CRAFTING HARVEST', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('newcraft', 'NEWCRAFT opens the materials-and-motes project editor in every configured
crafting mode. In mode 2, CRAFT opens the same editor.

Ordinary equipment setup:
  newcraft itemtype <weapon|armor|instrument|misc>
  newcraft specifictype <type>
  newcraft variant <variant>
  newcraft materials add <material>     (once per required group in SHOW)
  newcraft keywords <words>
  newcraft shortdesc <text>
  newcraft roomdesc <text>
  newcraft enhancement <1-8>            (weapons and armor, optional)
  newcraft instrument <quality|effectiveness|breakability> <amount>
  newcraft bonuses <slot 1-6> <location> <type> <modifier> [specific] (optional)
  newcraft motes add <enhancement|quality|effectiveness|breakability|1-6>
                                        (fund each value set above)
  newcraft show
  newcraft check
  newcraft start

Allocate the primary material before setting descriptions. Keywords, shortdesc,
and roomdesc must contain the exact variant phrase and selected primary-material
description. Keywords cannot contain dashes.

BONUSES use slots 1 through 6; slot 0 is never valid. NEWCRAFT RESET MATERIALS
also clears all project descriptions; RESET DESCRIPTIONS also refunds and
clears material allocations, RESET MOTES also deletes bonus definitions, and an
unrecognized RESET part resets the whole project. Re-add the primary material
before setting descriptions again.

START begins the timed work. Until the work ends or ACTIVITY CANCEL stops it,
the project cannot change; SHOW, CHECK, and SCORE still work.

Other routes include NEWCRAFT TOOLS, NEWCRAFT SCORE, and, in mode 2 only,
NEWCRAFT GOLEM <type|size|show|reset|start|animate>.

See Also: CRAFTING CRAFT-ITEMTYPE CRAFT-VARIANT CRAFT-MATERIALS
          CRAFT-BONUSES CRAFT-SHOW GOLEM-MAINTENANCE BONE-GOLEM', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('convert', 'CONVERT is not registered as a player command in the default command table.
A carried crafting kit contains an internal convert branch, but unknown commands
are rejected before that special procedure can handle them. The source also has
unreachable materials-and-motes conversion/refining handlers.

See Also: CRAFTING CRAFT-MATERIALS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-show', 'Usage: craft show
       newcraft show

Displays the current materials-and-motes equipment project, including
item type, variant, descriptions, materials, bonuses, and related settings.
Once a variant is set, it also shows the required skill, the project DC, and
the object level.

See Also: CRAFTING NEWCRAFT CRAFT-CHECK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-check', 'Usage: craft check
       newcraft check

Reports whether the project passes the type, variant, description, material,
mote, level-cap, tool-slot, and crafting-station checks, and lists what is
missing. START runs the same checks and also refuses a project whose DC no roll
can reach.

See Also: CRAFTING NEWCRAFT CRAFT-SHOW', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-bonuses', 'Usage: craft bonuses <slot 1-6> <location> <type> <modifier> [specific]
       newcraft bonuses <slot 1-6> <location> <type> <modifier> [specific]

Adds one of up to six object affects. Valid slots are 1 through 6. The location
is an APPLY name; type is normally enhancement or universal, with natural armor
and deflection also allowed for armor class. SPECIFIC identifies a skill, feat,
or spell-slot class when that location requires one.

Fund each slot afterwards with CRAFT MOTES ADD <slot> (or NEWCRAFT MOTES ADD
<slot>). CRAFT CHECK fails until every bonus slot has its motes.

See Also: CRAFTING NEWCRAFT CRAFT-ENHANCEMENT CRAFTING-MOTES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-enhancement', 'Usage: craft enhancement <modifier>
       newcraft enhancement <modifier>

For weapons, armor, and shields, this sets a separate enhancement bonus funded
with motes. Weapons apply it to hit and damage. Armor and shields apply it to
armor class; body, arms, legs, and head use the average of those four slots.
This is independent of the six CRAFT BONUSES slots.

See Also: CRAFTING NEWCRAFT CRAFT-BONUSES CRAFTING-MOTES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-specific-type', 'Usage: craft specifictype <type>
       newcraft specifictype <type>

Sets the concrete subtype after ITEMTYPE. For weapons it is a weapon type; for
armor it is an armor piece; for MISC it is a wear slot; and for instruments it
is the instrument type. Run the command without a value to list choices; the
weapon and armor lists show only the types a recipe can build.

See Also: CRAFTING NEWCRAFT CRAFT-ITEMTYPE CRAFT-VARIANT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-variant', 'Usage: craft variant <variant name>
       newcraft variant <variant name>

Sets the recipe variant after item type and specific type. The variant selects
material groups, quantities, the crafting skill (and with it the tool and the
crafting station), and an exact phrase used by the description validators.

Allocate the selected primary material before setting keywords, shortdesc, or
roomdesc. All three must contain both the exact variant phrase and the selected
primary-material description.

See Also: CRAFTING NEWCRAFT CRAFT-MATERIALS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('supplyorder', 'SUPPLYORDER has handlers in two systems.

LEGACY ROOM-370 AUTOCRAFT QUEST
In the Sanctus supply-order office (room 370), the room special intercepts:
  supplyorder              (shows the current order)
  supplyorder new          (replaces an unfinished order; its progress is lost)
  supplyorder complete
  supplyorder quit
Put exactly three single-unit objects of the requested material, and nothing
else, in a carried CRAFTING-KIT, then type AUTOCRAFT. Repeat for all five items.

BUILDER-BOUND MOBILE HANDLER
A mobile with the "New Supply Orders" special intercepts SUPPLYORDER in its
room and runs the general command below, so every subcommand works there.
Subcommands that need a quartermaster still need a mobile flagged QUARTERMASTER
in the room. The tracked source does not bind this special, but builders can
assign it.

MATERIALS-AND-MOTES GENERAL COMMAND
Outside the room-370 office, the general command accepts:
  supplyorder list|available          (artisan points only; no offers)
  supplyorder select|choose <number>  (offer numbers are not displayed first)
  supplyorder request
  supplyorder show|status|info
  supplyorder start|begin
  supplyorder material add <material>
  supplyorder material remove         (MATERIALS is also accepted)
  supplyorder complete|finish
  supplyorder reset|abandon|cancel
  supplyorder cooldown|cooldowns|timers

LIST, SELECT, REQUEST, and COMPLETE require a quartermaster in the room. START
requires the crafting station for the contract recipe skill, and timed work
rechecks that station. SELECT and COOLDOWN refresh eligible empty offer slots
and replace an offer that has no recipe variant; LIST and SHOW do not. No
command displays the generated offers before SELECT.

Each item takes its materials only when every group can pay for it, and
COMPLETE needs exactly the ordered quantity. A quality contract accepts only
materials graded above its tier, and a selected contract keeps its type and
tier when you log out.

Supply orders share one project record with NEWCRAFT, so you hold one or the
other. REQUEST and SELECT refuse while a crafting project exists, and project
subcommands refuse while an order is held; CRAFT SCORE and the golem commands
still work, but golem construction cannot start. RESET, ABANDON, and CANCEL
give up the order and return the materials no finished item used. Every new
order starts with no progress.

See Also: CRAFTING CRAFTING-KIT CRAFTMATERIALS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('restring', 'RESTRING renames an item through a carried crafting kit:
  restring <new name>

The command is compiled only when ALLOW_OBJECT_RETSRINGS_BY_PLAYERS is enabled.
That option is disabled in the default example configuration, so RESTRING is not
registered in the default build. REDESC remains a separate kit workflow.

See Also: CRAFTING CRAFTING-KIT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('reforge', 'Standalone usage: reforge <item name> <new type>
Crafting-kit usage: reforge <new type>

Standalone REFORGE needs materials-and-motes mode and an inventory weapon or
armor flagged REFORGEABLE. It checks the room for any crafting station mapped
from the item''s material (for example, a forge for steel); unmapped materials
require no station. It keeps the item''s
descriptions, replacing only a builder-set restring identifier with the new
subtype name.

The kit form works in any mode on the only item inside a carried crafting kit
and renames the result "a reforged <type>". Both forms accept an abbreviated
type name.

Any crafting kit you carry or wear, or that lies in the room, takes over
REFORGE even when empty. A worn or in-room kit refuses the work; remove kits
from all three locations to reach the standalone form.

Both forms cost one-half the item''s value in gold and keep the original
material when the new subtype uses the same material family. Otherwise the
new subtype''s material is used.

See Also: CRAFTING CRAFTING-KIT RESTRING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craftedit', '	n
-- Craftedit Menu : [1]             	WID number	n
1) Craft Name     : <No Name>       	WThe name of this craft	n
2) Craft Timer    : 0 seconds       	WHow long to create	n
3) Craft Item     : "None"          	WThe result product (vnum)	n
4) Craft Self Msg : "You craft $p." 	WMessage for crafting item $p	n
5) Craft Room Msg : "$n crafts $p." 	WTo-room for crafting item $p	n
S) Craft Skill    : No Skill (0)    	WSkill required to create this	n
F) Flags          : None            	WSet flags here, see below	n
R) Requirements   :                 	WList of req. objs, see below	n
  None
X) Delete                           	WThis options will delete this	n
Q) Quit                             	WThis options will exit	n
	n
	WFLAGS:	n
	c"Needs recipe"	n - does this particular craft require you have the recipe?
                   a recipe is a object of ''blueprint'' type that refers to
                   the ID number of this craft
	c--more flags to come soon!--	n
	n
	WREQUIREMENTS:	n
Here you enter a list of requirements to create the object. Requirements can
include components to assemble or an in-room component such as a forge. Items
can be destroyed or preserved depending on failure or success. Requirement
flags are:
	c"INROOM"	n - This requirement must be in your room. It is consumed like other
           components; add "!REMOVE" for a permanent station, such as a forge.
	c"SAVEonFAIL"	n - A failed craft normally loses all components; this flag
                 preserves this item when the craft fails.
	c"!REMOVE"	n - Components are normally consumed; this item is not consumed.
	n
	WNotes:	n
	c*There is no VNUM system for this crafting system, only a reference ID.	n
	c*You can use > show craft <name> to display info on a craft. 	n
	cThe information varies for recipe (blueprint) and non-recipe crafts.	n
	c*Typing "crafting" displays the crafts list and color-codes which crafts	n
	cyou have the goods to create.	n
	n', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE help_tag IN ('crafting', 'craft-itemtype', 'craft-materials', 'crafting-recipes-materials', 'crafts', 'vessels-and-advanced-crafting', 'newcraft', 'convert', 'craft-show', 'craft-check', 'craft-bonuses', 'craft-enhancement', 'craft-specific-type', 'craft-variant', 'supplyorder', 'restring', 'reforge', 'craftedit');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('crafting', 'CRAFTING'),
('craft-itemtype', 'CRAFT-ITEMTYPE'),
('craft-materials', 'CRAFT-MATERIALS'),
('crafting-recipes-materials', 'CRAFTING-RECIPES-MATERIALS'),
('crafting-recipes-materials', 'CRAFTING-MATERIAL'),
('crafting-recipes-materials', 'CRAFT-TOOLS'),
('crafting-recipes-materials', 'TOOLS'),
('crafting-recipes-materials', 'CRAFTING-SKILLS'),
('crafting-recipes-materials', 'ENHANCEMENT'),
('crafting-recipes-materials', 'ENHANCEMENTS'),
('crafting-recipes-materials', 'MATERIALS'),
('crafting-recipes-materials', 'RESOURCE'),
('crafts', 'CRAFTS'),
('vessels-and-advanced-crafting', 'CRAFTMATERIALS'),
('vessels-and-advanced-crafting', 'CRAFTSCORE'),
('vessels-and-advanced-crafting', 'NOCRAFTPROGRESS'),
('vessels-and-advanced-crafting', 'REFINE'),
('vessels-and-advanced-crafting', 'SAIL'),
('vessels-and-advanced-crafting', 'SALVAGE'),
('vessels-and-advanced-crafting', 'SHIPDISEMBARK'),
('vessels-and-advanced-crafting', 'SPLITENCHANTMENT'),
('vessels-and-advanced-crafting', 'VESSELS-AND-ADVANCED-CRAFTING'),
('newcraft', 'NEWCRAFT'),
('convert', 'CONVERT'),
('craft-show', 'CRAFT-SHOW'),
('craft-check', 'CRAFT-CHECK'),
('craft-bonuses', 'CRAFT-BONUSES'),
('craft-enhancement', 'CRAFT-ENHANCEMENT'),
('craft-specific-type', 'CRAFT-SPECIFIC-TYPE'),
('craft-specific-type', 'CRAFT-SPECIFICTYPE'),
('craft-variant', 'CRAFT-VARIANT'),
('supplyorder', 'AUTOCRAFT'),
('supplyorder', 'AUTOCRAFTING'),
('supplyorder', 'SUPPLYORDER'),
('restring', 'RESTRING'),
('reforge', 'CHANGING-ARMOR-TYPES'),
('reforge', 'CHANGING-SHIELD-TYPES'),
('reforge', 'CHANGING-WEAPON-TYPES'),
('reforge', 'REFORGE'),
('reforge', 'REFORGING-ITEMS'),
('craftedit', 'CRAFTEDIT');

COMMIT;
