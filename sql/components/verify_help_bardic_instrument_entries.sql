-- Read-only verification for help_bardic_instrument_entries.sql.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  2 AS expected,
  IF(COUNT(*) = 2, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('PERFORM', 'INSTRUMENT');

SELECT
  'keyword_count' AS check_name,
  COUNT(*) AS actual,
  13 AS expected,
  IF(COUNT(*) = 13, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('PERFORM', 'PERFORM'),
  ('PERFORM', 'PERFORMANCE'),
  ('PERFORM', 'BARDIC-PERFORMANCE'),
  ('PERFORM', 'PERFORMANCE-DIFFICULTY'),
  ('PERFORM', 'PERFORMANCE-EFFECTIVENESS'),
  ('PERFORM', 'PERFORMANCE-STUTTER'),
  ('PERFORM', 'PERFORMANCE-VERSE'),
  ('INSTRUMENT', 'INSTRUMENT'),
  ('INSTRUMENT', 'INSTRUMENTS'),
  ('INSTRUMENT', 'BARDIC-INSTRUMENT'),
  ('INSTRUMENT', 'BARDIC-INSTRUMENTS'),
  ('INSTRUMENT', 'SUMMON-INSTRUMENT'),
  ('INSTRUMENT', 'FLAME-KISSED-INSTRUMENT')
);

SELECT
  'player_access' AS check_name,
  COUNT(*) AS actual,
  2 AS expected,
  IF(COUNT(*) = 2, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('PERFORM', 'INSTRUMENT') AND min_level = 0;

SELECT
  'content_contracts' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'PERFORM' AS tag, '{Used As Instrument}' AS required_text
  UNION ALL SELECT 'PERFORM', 'slot shown by EQUIPMENT'
  UNION ALL SELECT 'PERFORM', 'No instrument applies -3 effectiveness'
  UNION ALL SELECT 'PERFORM', 'non-ideal instrument applies -2 effectiveness'
  UNION ALL SELECT 'PERFORM', 'chance in 11,111 per verse'
  UNION ALL SELECT 'INSTRUMENT', 'difficulty reduction'
  UNION ALL SELECT 'INSTRUMENT', 'summoned instrument appears in your possession'
  UNION ALL SELECT 'INSTRUMENT', 'requires more than 20 hit points'
  UNION ALL SELECT 'INSTRUMENT', 'costs exactly 20 hit points'
  UNION ALL SELECT 'INSTRUMENT', 'cannot reduce you below 1'
) AS expected_content ON h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.required_text) > 0;
