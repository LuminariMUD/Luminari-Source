-- Read-only verification for help_race_half_ogre_entries.sql.

SELECT 'race_half_ogre_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'RACE-HALF-OGRE' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'race_half_ogre_keywords' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'RACE-HALF-OGRE'
  AND UPPER(keyword) IN
    ('HALF-OGRE', 'HALF-OGRE-RACE', 'RACE-HALF-OGRE', 'OGRE', 'RACE-OGRE');

SELECT 'race_half_ogre_content' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT '+6 Str' AS required_text
  UNION ALL SELECT 'Advanced (level adjustment 2)'
  UNION ALL SELECT '1,000 account experience'
  UNION ALL SELECT '2x normal experience requirements'
  UNION ALL SELECT 'two stacking ranks of Armor Skin'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'RACE-HALF-OGRE';

SELECT 'race_half_ogre_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN
    ('HALF-OGRE', 'HALF-OGRE-RACE', 'RACE-HALF-OGRE', 'OGRE', 'RACE-OGRE')
  AND help_tag <> 'RACE-HALF-OGRE';
