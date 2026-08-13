-- Read-only verification for help_worship_entries.sql.

SELECT
  'worship_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'WORSHIP' AND min_level = 0 AND auto_generated = FALSE;

SELECT
  'worship_keywords' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'WORSHIP' AND keyword = 'WORSHIP';

SELECT
  'worship_content' AS check_name,
  COUNT(*) AS actual,
  3 AS expected,
  IF(COUNT(*) = 3, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'Usage:' AS required_text
  UNION ALL SELECT 'Spiderhaunt altar of Cyric'
  UNION ALL SELECT 'alignment slightly toward evil'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'WORSHIP';
