-- Read-only verification for help_thrown_weapons_entries.sql.

SELECT
  'thrown_help_entries' AS check_name,
  COUNT(*) AS actual,
  3 AS expected,
  IF(COUNT(*) = 3, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('THROWN-WEAPONS', 'RANGED-WEAPONS', 'COLLECT')
  AND min_level = 0
  AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'thrown_help_keywords' AS check_name,
  COUNT(*) AS actual,
  19 AS expected,
  IF(COUNT(*) = 19, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, UPPER(keyword)) IN (
  ('THROWN-WEAPONS', 'RETURNING'),
  ('THROWN-WEAPONS', 'THROW'),
  ('THROWN-WEAPONS', 'THROWING'),
  ('THROWN-WEAPONS', 'THROWN'),
  ('THROWN-WEAPONS', 'THROWN-WEAPON'),
  ('THROWN-WEAPONS', 'THROWN-WEAPONS'),
  ('RANGED-WEAPONS', 'AMMO'),
  ('RANGED-WEAPONS', 'AMMUNITION'),
  ('RANGED-WEAPONS', 'ARCHERY'),
  ('RANGED-WEAPONS', 'BLAST'),
  ('RANGED-WEAPONS', 'BOWS'),
  ('RANGED-WEAPONS', 'FIRE'),
  ('RANGED-WEAPONS', 'FIRE-WEAPONS'),
  ('RANGED-WEAPONS', 'MISSILES'),
  ('RANGED-WEAPONS', 'QUIVER'),
  ('RANGED-WEAPONS', 'QUIVERS'),
  ('RANGED-WEAPONS', 'RANGED-WEAPONS'),
  ('RANGED-WEAPONS', 'SHOOT'),
  ('COLLECT', 'COLLECT')
);

SELECT
  'thrown_help_content' AS check_name,
  COUNT(*) AS actual,
  15 AS expected,
  IF(COUNT(*) = 15, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'THROWN-WEAPONS' AS tag, 'equipped ammo pouch, top-level inventory' AS required_text
  UNION ALL SELECT 'THROWN-WEAPONS', 'wielded anchor itself'
  UNION ALL SELECT 'THROWN-WEAPONS', 'Throwing special ability'
  UNION ALL SELECT 'THROWN-WEAPONS', 'Returning alone does not'
  UNION ALL SELECT 'THROWN-WEAPONS', 'Manyshot remains launcher-only'
  UNION ALL SELECT 'THROWN-WEAPONS', 'Snatch Arrows'
  UNION ALL SELECT 'RANGED-WEAPONS', 'bow, crossbow, sling, or blowgun'
  UNION ALL SELECT 'RANGED-WEAPONS', 'first compatible missile'
  UNION ALL SELECT 'RANGED-WEAPONS', 'number of objects, not a weight'
  UNION ALL SELECT 'RANGED-WEAPONS', 'dart used as a weapon is thrown with THROW'
  UNION ALL SELECT 'RANGED-WEAPONS', 'blowgun'
  UNION ALL SELECT 'COLLECT', 'current room and corpses'
  UNION ALL SELECT 'COLLECT', 'never collects projectiles belonging'
  UNION ALL SELECT 'COLLECT', 'falls back to'
  UNION ALL SELECT 'COLLECT', 'Launcher ammunition still requires'
) AS expected_content
  ON h.tag = expected_content.tag
  AND INSTR(h.entry, expected_content.required_text) > 0;

SELECT
  'thrown_help_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE
  (UPPER(keyword) IN (
    'RETURNING', 'THROW', 'THROWING', 'THROWN', 'THROWN-WEAPON', 'THROWN-WEAPONS'
  )
    AND help_tag <> 'THROWN-WEAPONS')
  OR
  (UPPER(keyword) IN (
    'AMMO', 'AMMUNITION', 'ARCHERY', 'BLAST', 'BOWS', 'FIRE', 'FIRE-WEAPONS',
    'MISSILES', 'QUIVER', 'QUIVERS', 'RANGED-WEAPONS', 'SHOOT'
  ) AND help_tag <> 'RANGED-WEAPONS')
  OR (UPPER(keyword) = 'COLLECT' AND help_tag <> 'COLLECT');
