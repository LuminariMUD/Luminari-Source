-- Read-only verification for help_dg_damage_trigger.sql.

SELECT
  'dg_damage_menu_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'MOB-TRIGGERS' AND min_level = 31 AND auto_generated = FALSE
  AND INSTR(entry, '21) Damage') > 0
  AND INSTR(entry, 'TRIGEDIT-MOB-DAMAGE') > 0;

SELECT
  'dg_damage_menu_keyword' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'MOB-TRIGGERS' AND keyword = 'MOB-TRIGGERS';

SELECT
  'dg_damage_entry' AS check_name,
  COUNT(*) AS actual,
  1 AS expected,
  IF(COUNT(*) = 1, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag = 'TRIGEDIT-MOB-DAMAGE' AND min_level = 31 AND auto_generated = FALSE;

SELECT
  'dg_damage_keywords' AS check_name,
  COUNT(*) AS actual,
  3 AS expected,
  IF(COUNT(*) = 3, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('TRIGEDIT-MOB-DAMAGE', 'TRIGEDIT-MOB-DAMAGE'),
  ('TRIGEDIT-MOB-DAMAGE', 'MOB-DAMAGE-TRIGGER'),
  ('TRIGEDIT-MOB-DAMAGE', 'MTRIG-DAMAGE')
);

SELECT
  'dg_damage_keyword_owners' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE keyword IN ('TRIGEDIT-MOB-DAMAGE', 'MOB-DAMAGE-TRIGGER', 'MTRIG-DAMAGE')
  AND help_tag <> 'TRIGEDIT-MOB-DAMAGE';

SELECT
  'dg_damage_content' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'positive pending damage' AS required_text
  UNION ALL SELECT 'No explicit return preserves'
  UNION ALL SELECT 'A wait cannot delay the current hit'
  UNION ALL SELECT '%attackid% / %attackname%'
  UNION ALL SELECT '%damagetype% / %damagetypename%'
  UNION ALL SELECT '%attackmodeid% / %attackmode%'
  UNION ALL SELECT 'direct DG damage commands'
  UNION ALL SELECT 'player victims'
  UNION ALL SELECT 'damage cap'
  UNION ALL SELECT 'training dummy'
) AS expected_content
WHERE h.tag = 'TRIGEDIT-MOB-DAMAGE'
  AND INSTR(h.entry, expected_content.required_text) > 0;
