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
  AND SHA2(entry, 256) = 'ad414367af727262c87de5ec190695621572ec0f9ab276eef1d3818c6d1b9c6a';

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
  AND SHA2(entry, 256) = '53fc13dc3bcb8260f96ea8af6ff48b4ca7f4caa5df3f0fb5dedae8be2a1ba6d9';

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
  AND SHA2(entry, 256) = '927a61358b7b50bcdaf219de7e1410a027d626f40b09c20479c2377ee14087c3';

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
  AND SHA2(entry, 256) = 'cb051e75382866731412fba1112ab58fa52dc9468bc55a055059aea619f358e3';

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
  AND SHA2(entry, 256) = '52c5f535ff2a4c6daa5795e7aea53c25ebd2dcc2bfbf4996b471884b4443df31';

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
  AND SHA2(entry, 256) = '877d6ef3fd548b4f3b0f9088d64bfdbd9b226214810e18e7a630b6ae5a677aaf';

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
  AND SHA2(entry, 256) = '591015a5ce6c4cd1ed94880910dbe77aba54d8173270a5829b63eee3dd7948b4';

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
  'craft_bonuses_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag = 'craft-bonuses'
  AND min_level = 0
  AND auto_generated = FALSE
  AND SHA2(entry, 256) = '546dd4eb0c50f30060ceb25fd54e34fab61da830a2074a4e9082043770fe33fa';

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
  AND SHA2(entry, 256) = '6d08b9d6ee70bbb192f3c19df08b0ed1c27dee23527324004a87b5b1cebdf1f4';

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
  AND SHA2(entry, 256) = '44ca7a6d1f510554f421e36d6db0511b3fe1cc74f6ee638708c0cba70c254eba';

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
  AND SHA2(entry, 256) = 'a1f1f44d50bceb8208973837ed5f89c668d390a40f7f3536deb24960222d79be';

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
  AND SHA2(entry, 256) = '597e8ea0ab175ef084e380f29c99e000c2b53e55955d0cb51e7b8f5822aaba52';

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
  AND SHA2(entry, 256) = '028fd7ff35a033a101f8c2101a2225365e4c5ccc239ddfa790153d70dd3e948d';

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
  AND SHA2(entry, 256) = 'f2dcd45160de9606ac3bf2c27c1325d44448b1f57dcd99a9234d32db5b9f3ddb';

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
