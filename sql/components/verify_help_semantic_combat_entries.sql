-- Read-only verification for help_semantic_combat_entries.sql.

SELECT
  'semantic_combat_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'combat'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'One encounter round lasts 6 seconds') > 0
  AND INSTR(entry, 'highest initiative to lowest') > 0
  AND INSTR(entry, 'One valid queued command is attempted first') > 0
  AND INSTR(entry, 'full attack rotation') > 0;

SELECT
  'semantic_combat_keywords' AS check_name,
  COUNT(*) AS actual,
  4 AS expected,
  IF(COUNT(*) = 4, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'combat'
  AND UPPER(keyword) IN ('COMBAT', 'COMBAT-PHASE', 'COMBAT-ROUNDS', 'FIGHTING');

SELECT
  'semantic_combat_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) IN ('COMBAT', 'COMBAT-PHASE', 'COMBAT-ROUNDS', 'FIGHTING')
  AND help_tag <> 'combat';
