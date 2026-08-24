-- Read-only verification for help_race_myconid_entries.sql.

SELECT 'race_myconid_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'RACE-MYCONID' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'race_myconid_keywords' AS check_name, COUNT(*) AS actual, 3 AS expected,
       IF(COUNT(*) = 3, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'RACE-MYCONID'
  AND UPPER(keyword) IN ('MYCONID', 'MYCANOID', 'RACE-MYCONID');

SELECT 'race_myconid_content' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT '+8 Str' AS required_text
  UNION ALL SELECT 'Epic (level adjustment 10)'
  UNION ALL SELECT '30,000 account experience'
  UNION ALL SELECT '7x normal experience requirements'
  UNION ALL SELECT 'count as plants'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'RACE-MYCONID';

SELECT 'race_myconid_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('MYCONID', 'MYCANOID', 'RACE-MYCONID')
  AND help_tag <> 'RACE-MYCONID';
