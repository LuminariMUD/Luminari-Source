-- Read-only verification for help_crafting_entries.sql.

SELECT
  'crafting_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'crafting'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '4ca34fa9a99069049bd26c4ad3005488edfff3cf516995dffd3e91a07466d7d9';

SELECT
  'crafting_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFTING')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'crafting';

SELECT
  'craft_itemtype_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-itemtype'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '50d55e518f0e44868092bf0910aec0c4e6ebd37a8a4a9a4d6ec0baaa71cd3c07';

SELECT
  'craft_itemtype_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-ITEMTYPE')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-itemtype';

SELECT
  'craft_materials_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-materials'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'a0bcb870bf225b7058213ded54de2966d5d6a20f30df3079d141a2ab35547958';

SELECT
  'craft_materials_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-MATERIALS')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-materials';

SELECT
  'crafting_recipes_materials_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'crafting-recipes-materials'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'e04076e1dc9d3d5549ff4f8a29d0e73909a9a30ab3699ab239627e1520516e61';

SELECT
  'crafting_recipes_materials_keyword_set' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(
    COUNT(*) = 9 AND COUNT(DISTINCT UPPER(keyword)) = 9
    AND SUM(UPPER(keyword) IN ('CRAFTING-RECIPES-MATERIALS', 'CRAFTING-MATERIAL', 'CRAFT-TOOLS', 'TOOLS', 'CRAFTING-SKILLS', 'ENHANCEMENT', 'ENHANCEMENTS', 'MATERIALS', 'RESOURCE')) = 9,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'crafting-recipes-materials';

SELECT
  'crafts_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'crafts'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'e887f3eaa280353e803fae2de6fa9070f7be93cc1d5a19ca967e44b94f574e5a';

SELECT
  'crafts_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFTS')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'crafts';

SELECT
  'vessels_and_advanced_crafting_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'vessels-and-advanced-crafting'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '28ec753f150b5bced5b910b0102d75b2fc5b5602932e84b10b3d60c3e0316b7f';

SELECT
  'vessels_and_advanced_crafting_keyword_set' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(
    COUNT(*) = 9 AND COUNT(DISTINCT UPPER(keyword)) = 9
    AND SUM(UPPER(keyword) IN ('CRAFTMATERIALS', 'CRAFTSCORE', 'NOCRAFTPROGRESS', 'REFINE', 'SAIL', 'SALVAGE', 'SHIPDISEMBARK', 'SPLITENCHANTMENT', 'VESSELS-AND-ADVANCED-CRAFTING')) = 9,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'vessels-and-advanced-crafting';

SELECT
  'newcraft_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'newcraft'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'd46b2d5a691493fbe8eacb0de6407899c9a76e3163440339bd193bf9e67b00e6';

SELECT
  'newcraft_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('NEWCRAFT')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'newcraft';

SELECT
  'convert_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'convert'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'a47ac62d9e3471e72f7987d66eed0c0a7a0456909ebe619e956be86d2a6cbd4f';

SELECT
  'convert_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CONVERT')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'convert';

SELECT
  'craft_show_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-show'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '1b9e8e0b4ceb40d7934d03078640a3c3cd4001efdf081ed8fad7cdc1688c8a49';

SELECT
  'craft_show_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-SHOW')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-show';

SELECT
  'craft_check_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-check'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'e1e453cceac19c67acd0f9dd63545cf51179dc79316506d717100df44cbd8e8e';

SELECT
  'craft_check_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-CHECK')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-check';

SELECT
  'craft_bonuses_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-bonuses'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'f5fba5f3df2225a52bd1632f5875442c02d2805e7ca43d0bbf8ec32287b0449e';

SELECT
  'craft_bonuses_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-BONUSES')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-bonuses';

SELECT
  'craft_enhancement_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-enhancement'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '3b3bb0ae772a83faa3214d3ed29085731a8436bb7dad0b4caa984fec7425d0e9';

SELECT
  'craft_enhancement_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-ENHANCEMENT')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-enhancement';

SELECT
  'craft_specific_type_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-specific-type'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '9d049cd2c5a6baaf9d5de0dcb030f64b560a9f86df4f267fc51dc71dea86a19d';

SELECT
  'craft_specific_type_keyword_set' AS check_name,
  COUNT(*) AS actual,
  2 AS expected,
  IF(
    COUNT(*) = 2 AND COUNT(DISTINCT UPPER(keyword)) = 2
    AND SUM(UPPER(keyword) IN ('CRAFT-SPECIFIC-TYPE', 'CRAFT-SPECIFICTYPE')) = 2,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-specific-type';

SELECT
  'craft_variant_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-variant'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'f910ef77a86bd5c929a334bc4f4bf4422bb6807c06fce2aec8e288aae9dbe940';

SELECT
  'craft_variant_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFT-VARIANT')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craft-variant';

SELECT
  'supplyorder_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'supplyorder'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '3d617e5c4da1017c0399950ca071207f40a3431d12b6417b89c7f64d4f0941b5';

SELECT
  'supplyorder_keyword_set' AS check_name,
  COUNT(*) AS actual,
  3 AS expected,
  IF(
    COUNT(*) = 3 AND COUNT(DISTINCT UPPER(keyword)) = 3
    AND SUM(UPPER(keyword) IN ('AUTOCRAFT', 'AUTOCRAFTING', 'SUPPLYORDER')) = 3,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'supplyorder';

SELECT
  'restring_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'restring'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'e935fe683ed407ca6ceb9daa33850d3ee5c63e214e76b341f153a344c6364339';

SELECT
  'restring_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('RESTRING')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'restring';

SELECT
  'reforge_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'reforge'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '3b44e87238f0141beca0d868880ed8ba736819293cb2f7d48342055fcff4f88d';

SELECT
  'reforge_keyword_set' AS check_name,
  COUNT(*) AS actual,
  5 AS expected,
  IF(
    COUNT(*) = 5 AND COUNT(DISTINCT UPPER(keyword)) = 5
    AND SUM(UPPER(keyword) IN ('CHANGING-ARMOR-TYPES', 'CHANGING-SHIELD-TYPES', 'CHANGING-WEAPON-TYPES', 'REFORGE', 'REFORGING-ITEMS')) = 5,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'reforge';

SELECT
  'craftedit_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craftedit'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = 'ded1e2156974f474fc576fa1b83c59ab556c33b4685b55159a74147616585c75';

SELECT
  'craftedit_keyword_set' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(
    COUNT(*) = 1 AND COUNT(DISTINCT UPPER(keyword)) = 1
    AND SUM(UPPER(keyword) IN ('CRAFTEDIT')) = 1,
    'PASS', 'FAIL'
  ) AS result
FROM help_keywords
WHERE help_tag = 'craftedit';

SELECT
  'crafting_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords AS managed
INNER JOIN help_keywords AS other
  ON
    UPPER(other.keyword) = UPPER(managed.keyword)
    AND LOWER(other.help_tag) <> LOWER(managed.help_tag)
WHERE LOWER(managed.help_tag) IN (
  'crafting', 'craft-itemtype', 'craft-materials', 'crafting-recipes-materials',
  'crafts', 'vessels-and-advanced-crafting', 'newcraft', 'convert', 'craft-show',
  'craft-check', 'craft-bonuses', 'craft-enhancement', 'craft-specific-type',
  'craft-variant', 'supplyorder', 'restring', 'reforge', 'craftedit'
);
