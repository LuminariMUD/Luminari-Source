-- Read-only verification for help_command_sweep_entries.sql.

SELECT
  'command_sweep_entries' AS check_name,
  COUNT(*) AS actual,
  5 AS expected,
  IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('action-queue', 'autoblast', 'consumables', 'spellrecall', 'forum')
  AND min_level = 0
  AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'command_sweep_keywords' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, UPPER(keyword)) IN (
  ('action-queue', 'ACTION-QUEUE'),
  ('action-queue', 'QUEUE'),
  ('autoblast', 'AUTOBLAST'),
  ('consumables', 'CONSUMABLES'),
  ('consumables', 'STORED-CONSUMABLES'),
  ('consumables', 'USESTOREDCONSUMABLES'),
  ('spellrecall', 'SPELLRECALL'),
  ('spellrecall', 'SPELL-RECALL'),
  ('forum', 'FORUM'),
  ('forum', 'LUMINARI-WEBSITE')
);

SELECT
  'command_sweep_content' AS check_name,
  COUNT(*) AS actual,
  8 AS expected,
  IF(COUNT(*) = 8, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'action-queue' AS tag, 'non-mutating availability check' AS required_text
  UNION ALL SELECT 'autoblast', 'enabled or disabled'
  UNION ALL SELECT 'consumables', 'USESTOREDCONSUMABLES enables or disables'
  UNION ALL SELECT 'spellrecall', 'once per real-world day'
  UNION ALL SELECT 'spellrecall', 'randomly selected spell currently being prepared'
  UNION ALL SELECT 'spellrecall', 'not consumed when there is nothing'
  UNION ALL SELECT 'forum', 'https://luminarimud.com/'
  UNION ALL SELECT 'forum', 'ornir@luminarimud.com'
) AS required_content
  ON h.tag = required_content.tag
  AND INSTR(h.entry, required_content.required_text) > 0;

SELECT
  'obsolete_help_content' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('action-queue', 'autoblast', 'consumables', 'spellrecall', 'forum')
  AND (
    LOWER(entry) LIKE '%live.com%'
    OR LOWER(entry) LIKE '%future work%'
    OR LOWER(entry) LIKE '%will no use%'
  );

SELECT
  'command_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE
  (UPPER(keyword) IN ('ACTION-QUEUE', 'QUEUE') AND help_tag <> 'action-queue')
  OR (UPPER(keyword) = 'AUTOBLAST' AND help_tag <> 'autoblast')
  OR (UPPER(keyword) IN ('SPELLRECALL', 'SPELL-RECALL') AND help_tag <> 'spellrecall');
