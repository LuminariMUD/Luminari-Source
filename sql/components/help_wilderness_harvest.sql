-- Issue #145. Matches the HARVEST and HARVEST-TOOLS entries in help.hlp.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES
('harvest', 'Usage: harvest <resource category>
       harvest <node name>
       gather <herbs|vegetation|game>
       mine <minerals|crystal|stone|salt>

In the wilderness, harvest with no argument lists available categories.
Categories are vegetation, minerals, water, herbs, game, wood, stone,
crystal, clay and salt. Terrain and local depletion limit what is available.

A category harvest takes one full round (six seconds). Materials arrive
only when the round finishes. Moving, taking damage, entering combat or
using ACTIVITY CANCEL interrupts it. You need a free full round to begin.
You do not need a room node or a discovery step for category harvesting.

Vegetation, minerals, wood and game yield existing crafting materials.
Herbs, crystal, water, stone, clay and salt yield usable crafting motes;
better quality gives more motes per unit. Results go directly to your
crafting balances. Harvesting skills and talents affect success and rewards,
and attempts can improve the appropriate harvesting skill.

Carry or equip a harvest tool to guarantee its minimum quality on a
successful category harvest. The five tools are poor, common, uncommon,
rare and legendary. Your best tool applies; tools do not stack, are not
consumed and need not be wielded. Better natural results are still possible.
The tool must still be carried or equipped when the round finishes. Tools
do not guarantee success or make depleted resources available.

An explicit existing node, such as harvest vein, still uses its original
harvesting rules. Room-node crafting also keeps its existing behavior.
If wilderness crafting rewards are disabled by staff configuration,
category harvests use the earlier immediate wilderness-material storage.

See also: CRAFTING, HARVEST-TOOLS, ACTIVITY
', 0, 0)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM
  help_keywords
WHERE
  UPPER(keyword) IN ('HARVEST', 'WILDERNESS-HARVEST', 'GATHER', 'MINE')
  AND help_tag <> 'harvest';

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('harvest', 'HARVEST'),
('harvest', 'WILDERNESS-HARVEST'),
('harvest', 'GATHER'),
('harvest', 'MINE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES
('harvest-tools', 'Harvest tools improve the minimum quality of successful wilderness
category harvests while carried in your top-level inventory or equipped.
A tool inside a container must be taken out first.

Tool                      Minimum quality
poor harvest tool         Poor
common harvest tool       Common
uncommon harvest tool     Uncommon
rare harvest tool         Rare
legendary harvest tool    Legendary

Your best tool applies. It is not consumed and does not need to be wielded.
Removing a tool before the harvest completes removes its guarantee. Tools
set a minimum, not a maximum, and never turn a failed attempt into success.

Material rewards use crafting grades 1 through 5 for these quality tiers.
For mote rewards, each raw unit yields 1 through 5 motes respectively.
These tool guarantees apply to wilderness category harvesting; existing
node systems retain their own tool rules.

See also: HARVEST, CRAFTING
', 0, 0)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM
  help_keywords
WHERE
  UPPER(keyword) IN ('HARVEST-TOOLS')
  AND help_tag <> 'harvest-tools';

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('harvest-tools', 'HARVEST-TOOLS');

COMMIT;
