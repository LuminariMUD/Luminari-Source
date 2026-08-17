-- Read-only verification for help_arcane_mark_entries.sql.

SELECT
  'arcane_mark_entries' AS check_name,
  COUNT(*) AS actual,
  2 AS expected,
  IF(COUNT(*) = 2, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('ARCANEMARK', 'ARCANE-MARK') AND min_level = 0 AND auto_generated = FALSE;

SELECT
  'arcane_mark_keywords' AS check_name,
  COUNT(*) AS actual,
  4 AS expected,
  IF(COUNT(*) = 4, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('ARCANEMARK', 'ARCANEMARK'),
  ('ARCANEMARK', 'ARCANE-MARK-SIGNATURE'),
  ('ARCANE-MARK', 'ARCANE-MARK'),
  ('ARCANE-MARK', 'SPELL-ARCANE-MARK')
);

SELECT
  'arcane_mark_keyword_owners' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (keyword IN ('ARCANEMARK', 'ARCANE-MARK-SIGNATURE') AND help_tag <> 'ARCANEMARK')
   OR (keyword IN ('ARCANE-MARK', 'SPELL-ARCANE-MARK') AND help_tag <> 'ARCANE-MARK');

SELECT
  'arcane_mark_content' AS check_name,
  COUNT(*) AS actual,
  12 AS expected,
  IF(COUNT(*) = 12, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'ARCANEMARK' AS tag, 'does not target or change an object' AS required_text
  UNION ALL SELECT 'ARCANEMARK', 'up to 250 characters'
  UNION ALL SELECT 'ARCANEMARK', 'ARCANEMARK CLEAR'
  UNION ALL SELECT 'ARCANEMARK', 'cast ''arcane mark'' <object>'
  UNION ALL SELECT 'ARCANEMARK', 'The target must be in your inventory'
  UNION ALL SELECT 'ARCANE-MARK', 'wizards, sorcerers, and summoners'
  UNION ALL SELECT 'ARCANE-MARK', 'already has a mark'
  UNION ALL SELECT 'ARCANE-MARK', 'LOOK <object> or EXAMINE <object>'
  UNION ALL SELECT 'ARCANE-MARK', 'survives normal player inventory save and load'
  UNION ALL SELECT 'ARCANE-MARK', 'They grant no'
  UNION ALL SELECT 'ARCANE-MARK', 'STAT PLAYER'
  UNION ALL SELECT 'ARCANE-MARK', 'STAT OBJECT'
) AS expected_content ON h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.required_text) > 0;
