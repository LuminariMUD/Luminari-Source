-- Read-only verification for help_crafting_entries.sql.

SELECT
  'crafting_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'crafting'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'mode 1 opens PRACTICE') > 0
  AND INSTR(entry, 'NEWCRAFT opens that editor') > 0
  AND INSTR(entry, 'CRAFTMATERIALS STORE <item>') > 0
  AND INSTR(entry, 'carpenter variants cannot pass') > 0
  AND INSTR(entry, 'login/reconnect/copyover does') > 0
  AND INSTR(entry, 'no registered command creates wands') > 0;

SELECT
  'craft_itemtype_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'craft-itemtype'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'weapon|armor|instrument|misc|golem') > 0
  AND INSTR(entry, 'use MISC rather than') > 0
  AND INSTR(entry, 'Golem setup is entered') > 0;

SELECT
  'craft_materials_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'craft-materials'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'CRAFTMATERIALS STORE') > 0
  AND INSTR(entry, 'default build, MATERIALS displays wilderness') > 0;

SELECT
  'crafting_recipes_materials_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'crafting-recipes-materials'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'CRAFT EQUIPMENT and CRAFT GEAR') > 0
  AND INSTR(entry, 'equivalent NEWCRAFT arguments') > 0
  AND INSTR(entry, 'no registered REFINE or CRAFT WAND') > 0
  AND INSTR(entry, 'CONVERT is not registered') > 0;

SELECT
  'crafts_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'crafts'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'Usage: crafting') > 0
  AND INSTR(entry, 'carried blueprint') > 0;

SELECT
  'advanced_crafting_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'vessels-and-advanced-crafting'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'no registered REFINE command') > 0
  AND INSTR(entry, 'Salvage or Scavenger') > 0;

SELECT
  'crafting_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
     AND SUM(UPPER(keyword) IN ('CRAFTING')) = 1,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'crafting';

SELECT
  'craft-itemtype_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
     AND SUM(UPPER(keyword) IN ('CRAFT-ITEMTYPE')) = 1,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'craft-itemtype';

SELECT
  'craft-materials_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
     AND SUM(UPPER(keyword) IN ('CRAFT-MATERIALS')) = 1,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'craft-materials';

SELECT
  'crafting-recipes-materials_keyword_set' AS check_name,
  COUNT(*) AS actual,
  26 AS expected,
  IF(COUNT(*) = 26 AND COUNT(DISTINCT UPPER(keyword)) = 26
     AND SUM(UPPER(keyword) IN ('ALCHEMICAL-SILVER', 'ALCHEMY', 'BREW-POTION', 'CHEMISTRY', 'CRAFT-TOOLS', 'CRAFT-WAND', 'CRAFTING-CONVERT', 'CRAFTING-MATERIAL', 'CRAFTING-RECIPES-MATERIALS', 'CRAFTING-SKILLS', 'DISMANTLE', 'ENCHANTING', 'ENHANCEMENT', 'ENHANCEMENTS', 'FAST-CRAFTER', 'LUMINOUS-THREAD', 'MAGIC-ITEMS', 'MATERIALS', 'QUICK-ALCHEMY', 'RESOURCE', 'RESOURCES', 'SCROUNGE', 'SWIFT-ALCHEMY', 'TOOLS', 'WEAPONTOUCH', 'WILDERNESS-MAT')) = 26,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'crafting-recipes-materials';

SELECT
  'crafts_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
     AND SUM(UPPER(keyword) IN ('CRAFTS')) = 1,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'crafts';

SELECT
  'vessels-and-advanced-crafting_keyword_set' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10 AND COUNT(DISTINCT UPPER(keyword)) = 10
     AND SUM(UPPER(keyword) IN ('CRAFTMATERIALS', 'CRAFTSCORE', 'DISEMBARK', 'NOCRAFTPROGRESS', 'REFINE', 'SAIL', 'SALVAGE', 'SHIPDISEMBARK', 'SPLITENCHANTMENT', 'VESSELS-AND-ADVANCED-CRAFTING')) = 10,
     'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'vessels-and-advanced-crafting';

SELECT
  'crafting_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('ALCHEMICAL-SILVER', 'ALCHEMY', 'BREW-POTION', 'CHEMISTRY', 'CRAFT-ITEMTYPE', 'CRAFT-MATERIALS', 'CRAFT-TOOLS', 'CRAFT-WAND', 'CRAFTING', 'CRAFTING-CONVERT', 'CRAFTING-MATERIAL', 'CRAFTING-RECIPES-MATERIALS', 'CRAFTING-SKILLS', 'CRAFTMATERIALS', 'CRAFTS', 'CRAFTSCORE', 'DISEMBARK', 'DISMANTLE', 'ENCHANTING', 'ENHANCEMENT', 'ENHANCEMENTS', 'FAST-CRAFTER', 'LUMINOUS-THREAD', 'MAGIC-ITEMS', 'MATERIALS', 'NOCRAFTPROGRESS', 'QUICK-ALCHEMY', 'REFINE', 'RESOURCE', 'RESOURCES', 'SAIL', 'SALVAGE', 'SCROUNGE', 'SHIPDISEMBARK', 'SPLITENCHANTMENT', 'SWIFT-ALCHEMY', 'TOOLS', 'VESSELS-AND-ADVANCED-CRAFTING', 'WEAPONTOUCH', 'WILDERNESS-MAT')
  AND NOT ((help_tag = 'crafting' AND UPPER(keyword) IN ('CRAFTING'))
    OR (help_tag = 'craft-itemtype' AND UPPER(keyword) IN ('CRAFT-ITEMTYPE'))
    OR (help_tag = 'craft-materials' AND UPPER(keyword) IN ('CRAFT-MATERIALS'))
    OR (help_tag = 'crafting-recipes-materials' AND UPPER(keyword) IN ('ALCHEMICAL-SILVER', 'ALCHEMY', 'BREW-POTION', 'CHEMISTRY', 'CRAFT-TOOLS', 'CRAFT-WAND', 'CRAFTING-CONVERT', 'CRAFTING-MATERIAL', 'CRAFTING-RECIPES-MATERIALS', 'CRAFTING-SKILLS', 'DISMANTLE', 'ENCHANTING', 'ENHANCEMENT', 'ENHANCEMENTS', 'FAST-CRAFTER', 'LUMINOUS-THREAD', 'MAGIC-ITEMS', 'MATERIALS', 'QUICK-ALCHEMY', 'RESOURCE', 'RESOURCES', 'SCROUNGE', 'SWIFT-ALCHEMY', 'TOOLS', 'WEAPONTOUCH', 'WILDERNESS-MAT'))
    OR (help_tag = 'crafts' AND UPPER(keyword) IN ('CRAFTS'))
    OR (help_tag = 'vessels-and-advanced-crafting' AND UPPER(keyword) IN ('CRAFTMATERIALS', 'CRAFTSCORE', 'DISEMBARK', 'NOCRAFTPROGRESS', 'REFINE', 'SAIL', 'SALVAGE', 'SHIPDISEMBARK', 'SPLITENCHANTMENT', 'VESSELS-AND-ADVANCED-CRAFTING')));
