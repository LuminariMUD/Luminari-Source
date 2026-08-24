-- Read-only verification for help_race_half_illithid_entries.sql.

SELECT 'race_half_illithid_entry' AS check_name, COUNT(*) AS actual, 1 AS expected,
       IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'RACE-HALF-ILLITHID' AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT 'race_half_illithid_keywords' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'RACE-HALF-ILLITHID'
  AND UPPER(keyword) IN
    ('HALF-ILLITHID', 'HALF-ILLITHID-RACE', 'RACE-HALF-ILLITHID', 'ILLITHID',
     'RACE-ILLITHID');

SELECT 'race_half_illithid_content' AS check_name, COUNT(*) AS actual, 5 AS expected,
       IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT '+4 Int' AS required_text
  UNION ALL SELECT 'Epic (level adjustment 10)'
  UNION ALL SELECT '30,000 account experience'
  UNION ALL SELECT '7x normal experience requirements'
  UNION ALL SELECT 'innate Levitation'
) AS expected_content ON INSTR(h.entry, expected_content.required_text) > 0
WHERE h.tag = 'RACE-HALF-ILLITHID';

SELECT 'race_half_illithid_keyword_conflicts' AS check_name, COUNT(*) AS actual, 0 AS expected,
       IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN
    ('HALF-ILLITHID', 'HALF-ILLITHID-RACE', 'RACE-HALF-ILLITHID', 'ILLITHID',
     'RACE-ILLITHID')
  AND help_tag <> 'RACE-HALF-ILLITHID';
