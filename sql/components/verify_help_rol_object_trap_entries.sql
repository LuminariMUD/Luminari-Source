-- Read-only verification for help_rol_object_trap_entries.sql.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  2 AS expected,
  IF(COUNT(*) = 2, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('DETECT-TRAP', 'DISABLE-TRAP');

SELECT
  'keyword_count' AS check_name,
  COUNT(*) AS actual,
  4 AS expected,
  IF(COUNT(*) = 4, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('DETECT-TRAP', 'DETECT-TRAP'),
  ('DETECT-TRAP', 'DETECTTRAP'),
  ('DISABLE-TRAP', 'DISABLE-TRAP'),
  ('DISABLE-TRAP', 'DISABLETRAP')
);

SELECT
  'content_contracts' AS check_name,
  COUNT(*) AS actual,
  4 AS expected,
  IF(COUNT(*) = 4, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'DETECT-TRAP' AS tag, 'detecttrap <object>' AS required_text
  UNION ALL SELECT 'DETECT-TRAP', 'directional movement'
  UNION ALL SELECT 'DISABLE-TRAP', 'disabletrap <object>'
  UNION ALL SELECT 'DISABLE-TRAP', 'remaining charges to zero'
) AS expected_content ON h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.required_text) > 0;
