-- Read-only verification for help_crafting_entries.sql.

SELECT
  'crafting_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'crafting'
  AND min_level = 0
  AND auto_generated = FALSE
  AND INSTR(entry, 'crafting mode selected by server configuration') > 0
  AND INSTR(entry, 'MATERIALS AND MOTES MODE') > 0
  AND INSTR(entry, 'KIT AND BLUEPRINT MODE') > 0
  AND INSTR(entry, 'Offline time does not advance the timer') > 0
  AND INSTR(entry, 'MATERIALS STORE <item>') > 0
  AND INSTR(entry, 'completion skill check') > 0
  AND INSTR(entry, 'ordinary failure keeps') > 0;

SELECT
  'crafting_keyword' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'crafting'
  AND UPPER(keyword) = 'CRAFTING';

SELECT
  'crafting_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE UPPER(keyword) = 'CRAFTING'
  AND help_tag <> 'crafting';
