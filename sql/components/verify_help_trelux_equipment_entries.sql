-- Read-only verification for help_trelux_equipment_entries.sql.

SELECT 'trelux_equipment_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'TRELUX-EQ' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'trelux_equipment_keywords' AS check_name, COUNT(*) AS actual, 2 AS expected,
       IF(COUNT(*) = 2, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'TRELUX-EQ'
  AND UPPER(keyword) IN ('TRELUX-EQ', 'TRELUX-EQUIPMENT');

SELECT 'trelux_equipment_content' AS check_name, COUNT(*) AS actual, 8 AS expected,
       IF(COUNT(*) = 8, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'wield weapons' AS required_text
  UNION ALL SELECT 'hold items'
  UNION ALL SELECT 'use shields'
  UNION ALL SELECT 'wear gloves'
  UNION ALL SELECT 'wear rings'
  UNION ALL SELECT 'insect-like legs'
  UNION ALL SELECT 'ankle equipment slots remain available'
  UNION ALL SELECT 'saved equipment'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'TRELUX-EQ';

SELECT 'trelux_equipment_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('TRELUX-EQ', 'TRELUX-EQUIPMENT')
  AND help_tag <> 'TRELUX-EQ';
