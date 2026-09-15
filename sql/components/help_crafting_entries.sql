-- Keep the database CRAFTING topic aligned with lib/text/help/help.hlp.
-- Safe to run repeatedly; unrelated crafting aliases and entries are preserved.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('crafting', 'The CRAFT command uses the crafting mode selected by server configuration.
Commands from the other mode may not be available on this server.

MATERIALS AND MOTES MODE
Build a persistent equipment project with these steps:
  craft itemtype <weapon|armor|instrument|misc>
  craft specifictype <type>
  craft variant <variant>
  craft materials add <material>
  craft enhancement <amount>       (optional)
  craft motes add <enhancement|bonus slot>  (when needed)
  craft bonuses ...                (optional)
  craft show
  craft check
  craft start

Use CRAFT with no argument for the complete command list. Recipes require
specific material groups. Normal project admission checks the recipe,
descriptions, stored resources and equipped crafting tool. Some workflows use
a room station such as a forge, loom, tannery or carpentry table. MATERIALS and
MOTES show stored balances. MATERIALS STORE <item>, HARVEST and SALVAGE <item>
are among the ways resources enter those balances.

Crafting is timed work. Moving, combat, damage or losing a required station or
target can cancel the activity. Harvesting can also stop if its required tool is
no longer valid. Offline time does not advance the timer; login can resume saved
work. ACTIVITY shows or cancels current work. CRAFT RESET returns resources
reserved by an equipment project. A natural 1 on the completion skill check
loses reserved materials and motes, while an ordinary failure keeps the project
available for another attempt.

KIT AND BLUEPRINT MODE
In the legacy mode, CRAFT lists known crafts and starts a named craft or
blueprint when its object and room requirements are present. The CREATE,
CRAFTING-KIT, MOLD and CRYSTAL topics describe related legacy workflows.

Special thanks to Ellyanor for the original crafting help contribution.

Refining, resizing, supply orders, golems, potions, wands and scrolls have
separate commands or subcommands. CRAFT SCORE shows crafting and harvesting
skill progress in materials-and-motes mode.

See Also: CRAFT-MATERIALS CRAFT-MOTES CRAFT-SHOW CRAFT-CHECK HARVEST REFINE
          RESIZE SUPPLYORDER ACTIVITY CRAFTS CREATE CRAFTING-KIT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING'
  AND help_tag <> 'crafting';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('crafting', 'CRAFTING');

COMMIT;
