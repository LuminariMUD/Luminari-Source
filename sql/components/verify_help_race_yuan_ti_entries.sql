-- Read-only verification for help_race_yuan_ti_entries.sql.

SELECT 'race_yuan_ti_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'RACE-YUAN-TI' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'race_yuan_ti_keywords' AS check_name, COUNT(*) AS actual, 3 AS expected,
       IF(COUNT(*) = 3, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'RACE-YUAN-TI'
  AND UPPER(keyword) IN ('YUAN-TI', 'YUANTI', 'RACE-YUAN-TI');

SELECT 'race_yuan_ti_content' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT '+2 Int' AS required_text
  UNION ALL SELECT 'Advanced (level adjustment 2)'
  UNION ALL SELECT '1,000 account experience'
  UNION ALL SELECT '2x normal experience requirements'
  UNION ALL SELECT 'Poison Bite'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'RACE-YUAN-TI';

SELECT 'race_yuan_ti_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('YUAN-TI', 'YUANTI', 'RACE-YUAN-TI')
  AND help_tag <> 'RACE-YUAN-TI';
