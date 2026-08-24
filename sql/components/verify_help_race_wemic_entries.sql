-- Read-only verification for help_race_wemic_entries.sql.

SELECT 'race_wemic_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'RACE-WEMIC' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'race_wemic_keywords' AS check_name, COUNT(*) AS actual, 4 AS expected,
       IF(COUNT(*) = 4, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'RACE-WEMIC'
  AND UPPER(keyword) IN ('WEMIC', 'RACE-WEMIC', 'BARBARIAN', 'RACE-BARBARIAN');

SELECT 'race_wemic_content' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT '+8 Str' AS required_text
  UNION ALL SELECT 'Advanced (level adjustment 2)'
  UNION ALL SELECT '1,000 account experience'
  UNION ALL SELECT '2x normal experience requirements'
  UNION ALL SELECT 'Claws and Bite'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'RACE-WEMIC';

SELECT 'race_wemic_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('WEMIC', 'RACE-WEMIC', 'BARBARIAN', 'RACE-BARBARIAN')
  AND help_tag <> 'RACE-WEMIC';
