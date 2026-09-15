-- Keep corrected crafting topics aligned with lib/text/help/help.hlp.
-- Safe to run repeatedly. Every flat-file alias is reassigned explicitly.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafting', 'CRAFT and CRAFTSCORE use the crafting mode selected by server configuration:
mode 0 reports no system, mode 1 opens PRACTICE, and mode 2 opens the
materials-and-motes project editor. NEWCRAFT opens that editor in every mode.
CRAFTING is a separate catalog/blueprint command available in every mode.

MATERIALS-AND-MOTES EQUIPMENT PROJECT
Use CRAFT in mode 2, or substitute NEWCRAFT in any mode:
  newcraft itemtype <weapon|armor|instrument|misc>
  newcraft specifictype <type>
  newcraft variant <variant>
  newcraft materials add <material>
  newcraft keywords <words>
  newcraft shortdesc <text>
  newcraft roomdesc <text>
  newcraft enhancement <amount>       (optional)
  newcraft motes add <enhancement|bonus slot>  (when needed)
  newcraft bonuses ...                (optional)
  newcraft show
  newcraft check
  newcraft start

Admission checks the selected recipe, descriptions, project allocations and an
equipped crafting tool. The current code has no woodworking tool slot, so
carpenter variants cannot pass this check. Other variants also depend on staff
providing the appropriate crafting-tool objects. The first attempt does not
reliably require a room station; after an ordinary failure, a retry requires
the station for the completion skill.

CRAFTMATERIALS lists crafting balances. CRAFTMATERIALS STORE <item> deposits a
physical material object. MOTES lists mote balances. SALVAGE <item> requires
the Salvage feat or Scavenger talent. Wilderness HARVEST can also award
balances when that integration is enabled.

Crafting is timed work. Moving, combat, damage or losing a required station or
target can cancel the activity. Harvesting can also stop if its required tool is
no longer valid. ACTIVITY shows, cancels, pauses or resumes work when supported.
Offline time does not advance a saved timer, but login/reconnect/copyover does
not currently reconstruct its activity automatically. Resize work is refunded
and cleared at load; manually resumed golem work lacks saved selections and
cannot complete correctly. CRAFT RESET returns equipment-project resources. A
natural 1 loses reserved materials and motes; an ordinary failure keeps the
project for another attempt.

OTHER CRAFTING PATHS
CRAFTING lists or starts catalog crafts and carried blueprints. A carried
CRAFTING-KIT separately handles CREATE, CHECKCRAFT, RESIZE, RESTRING, REDESC,
AUGMENT, DISENCHANT, BONEARMOR, REFORGE and AUTOCRAFT where registered.
SUPPLYORDER and mode-2 golem work use additional project paths. BREW creates
potions and SCRIBE creates scrolls. No player command currently exposes motes
refining or motes resizing, and no registered command creates wands.

See Also: CRAFT-MATERIALS CRAFTING-MOTES CRAFT-SHOW CRAFT-CHECK HARVEST
          RESIZE SUPPLYORDER ACTIVITY CRAFTS CREATE CRAFTING-KIT BREW SCRIBE

Special thanks to Ellyanor for the original crafting help contribution.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING' AND help_tag <> 'crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting', 'CRAFTING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-itemtype', 'Usage: craft itemtype <weapon|armor|instrument|misc|golem>
       newcraft itemtype <weapon|armor|instrument|misc|golem>

This selects the broad project type before most other project settings.
Jewelry-oriented recipes are miscellaneous subtypes, so use MISC rather than
JEWELRY. Golem setup is entered with CRAFT GOLEM or NEWCRAFT GOLEM and requires
materials-and-motes mode.

See Also: CRAFTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFT-ITEMTYPE' AND help_tag <> 'craft-itemtype';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('craft-itemtype', 'CRAFT-ITEMTYPE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('craft-materials', 'Usage: craft materials <add|remove> <material type>
       newcraft materials <add|remove> <material type>
       craftmaterials
       craftmaterials store <item>

CRAFT MATERIALS allocates the exact amount a selected recipe variant needs, or
returns that allocation. Select item type, specific type and variant first, then
use CRAFT SHOW to see the required groups and quantities.

CRAFTMATERIALS lists the separate materials-and-motes crafting balances.
CRAFTMATERIALS STORE deposits a physical ITEM_MATERIAL object into those
balances. In the default build, MATERIALS displays wilderness material storage
instead; its behavior depends on a compile-time option.

See Also: CRAFTING CRAFT-SHOW CRAFTMATERIALS MATERIALS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFT-MATERIALS' AND help_tag <> 'craft-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('craft-materials', 'CRAFT-MATERIALS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafting-recipes-materials', 'Topic: Crafting Materials and Adjacent Skills

CRAFT MATERIALS allocates materials to an equipment project. CRAFT TOOLS,
CRAFT EQUIPMENT and CRAFT GEAR display equipped support gear in mode 2; the
equivalent NEWCRAFT arguments work in every mode. CRAFTMATERIALS lists the
project-system balances and CRAFTMATERIALS STORE deposits a physical material
object.

MATERIALS shows separate wilderness storage in the default build. HARVEST,
GATHER, MINE and SCROUNGE are resource paths with their own rules. SALVAGE
requires the Salvage feat or Scavenger talent.

BREW creates potions, ALCHEMY covers discoveries and bombs, and SCRIBE creates
scrolls. The source contains dormant motes refining/convert-style handlers but
no registered REFINE or CRAFT WAND creation command. A crafting kit contains a
CONVERT branch, but CONVERT is not registered for player dispatch.

Enhancement and other magical bonuses are funded with motes in an equipment
project. CRAFTING-SKILLS and tool requirements are summarized by CRAFTING.

See Also: CRAFTING CRAFT-MATERIALS CRAFTING-MOTES HARVEST ALCHEMY BREW SCRIBE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ALCHEMICAL-SILVER' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'ALCHEMICAL-SILVER');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ALCHEMY' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'ALCHEMY');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'BREW-POTION' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'BREW-POTION');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CHEMISTRY' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CHEMISTRY');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFT-TOOLS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFT-TOOLS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFT-WAND' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFT-WAND');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING-CONVERT' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFTING-CONVERT');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING-MATERIAL' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFTING-MATERIAL');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING-RECIPES-MATERIALS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFTING-RECIPES-MATERIALS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING-SKILLS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'CRAFTING-SKILLS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'DISMANTLE' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'DISMANTLE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ENCHANTING' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'ENCHANTING');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ENHANCEMENT' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'ENHANCEMENT');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ENHANCEMENTS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'ENHANCEMENTS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'FAST-CRAFTER' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'FAST-CRAFTER');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'LUMINOUS-THREAD' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'LUMINOUS-THREAD');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'MAGIC-ITEMS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'MAGIC-ITEMS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'MATERIALS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'MATERIALS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'QUICK-ALCHEMY' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'QUICK-ALCHEMY');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'RESOURCE' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'RESOURCE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'RESOURCES' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'RESOURCES');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SCROUNGE' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'SCROUNGE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SWIFT-ALCHEMY' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'SWIFT-ALCHEMY');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'TOOLS' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'TOOLS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'WEAPONTOUCH' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'WEAPONTOUCH');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'WILDERNESS-MAT' AND help_tag <> 'crafting-recipes-materials';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting-recipes-materials', 'WILDERNESS-MAT');

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
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTS' AND help_tag <> 'crafts';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafts', 'CRAFTS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('vessels-and-advanced-crafting', 'Topic  : Vessels, Salvaging, and Crafting Progress
Usage  : sail [speed | heading]
         shipdisembark
         salvage <item>
         craftscore
         craftmaterials
         craftmaterials store <item>
         splitenchantment <item>
         nocraftprogress
         mine

NAUTICAL & CRAFTING COMMANDS:
- sail: controls a captained vessel''s speed and heading.
- shipdisembark: leaves a docked or anchored vessel.
- salvage: dismantles eligible equipment; requires Salvage or Scavenger.
- craftscore: displays crafting and harvesting skill progress according to the
  configured mode.
- craftmaterials: lists project-system material balances; STORE deposits a
  physical material object.
- splitenchantment: extracts enchantment resources from eligible equipment.
- nocraftprogress: toggles crafting progress messages.
- mine: gathers supported wilderness mineral resources.

The source contains a motes refining handler, but no registered REFINE command
reaches it. REFINE is retained as a search keyword for this limitation, not as
an available command.

See Also: VESSELS CRAFTING HARVEST ALCHEMY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTMATERIALS' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'CRAFTMATERIALS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTSCORE' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'CRAFTSCORE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'DISEMBARK' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'DISEMBARK');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'NOCRAFTPROGRESS' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'NOCRAFTPROGRESS');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'REFINE' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'REFINE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SAIL' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'SAIL');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SALVAGE' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'SALVAGE');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SHIPDISEMBARK' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'SHIPDISEMBARK');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SPLITENCHANTMENT' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'SPLITENCHANTMENT');

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'VESSELS-AND-ADVANCED-CRAFTING' AND help_tag <> 'vessels-and-advanced-crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('vessels-and-advanced-crafting', 'VESSELS-AND-ADVANCED-CRAFTING');

COMMIT;
