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
  AND SHA2(entry, 256) = '8ccdce5fc3a27ad52267cbe9784726faea22041bc3db3aa0282c22308690dd78';

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
  AND SHA2(entry, 256) = '6bbfb14f6589488887b15023a9fffbbd036f4b502b294fba3e8d196dbccbfa3d';

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
  AND SHA2(entry, 256) = '32ab00c48af5419ef4242ae52d9acb9a576d170ef19c859c07143a2aaf50048d';

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
  AND SHA2(entry, 256) = '6c973e1751d436a27a791aee17fc346493a064871bce297e50c0c32f7c28a250';

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
  AND SHA2(entry, 256) = '0d33f345c64b1f29f9f9199ac44876e320cadfec8e1f8fe6b9bccf5e0b1266f3';

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
  AND SHA2(entry, 256) = 'ecf41d758347241bcaa06c309d62058f7bb83673833bd37d5237bed7c6dc5a9f';

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
  AND SHA2(entry, 256) = '2e43af97020d88bbf276931c0b742f9fc210890b1b2dfbf9da7fc573ed471f22';

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
  AND SHA2(entry, 256) = '16d5831fbc972da71584a9f6d84a263d9d6efe5285f0303bea9bfff7a14d757d';

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
  AND SHA2(entry, 256) = 'c599150eaf7587066d91f5ce94c23cf19bc10f8e46185d6e377ebc7e4669002e';

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
