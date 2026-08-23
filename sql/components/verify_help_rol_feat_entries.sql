-- Read-only verification for help_rol_feat_entries.sql.

SELECT
  'rol_feat_entries' AS check_name,
  COUNT(*) AS actual,
  5 AS expected,
  IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('ACCOMPANY', 'CALM', 'CAMP', 'GARROTE', 'SHADOW')
  AND min_level = 0
  AND auto_generated = FALSE
  AND entry IS NOT NULL
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'rol_feat_keywords' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(COUNT(*) = 9, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('ACCOMPANY', 'ACCOMPANY'),
  ('CALM', 'CALM'),
  ('CALM', 'PACIFY'),
  ('CAMP', 'CAMP'),
  ('CAMP', 'ESTABLISH-CAMP'),
  ('GARROTE', 'GARROTE'),
  ('GARROTE', 'STRANGLE'),
  ('SHADOW', 'SHADOW'),
  ('SHADOW', 'TAIL')
);

SELECT
  'rol_feat_keyword_sets' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(COUNT(*) = 9, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag IN ('ACCOMPANY', 'CALM', 'CAMP', 'GARROTE', 'SHADOW');

SELECT
  'rol_feat_command_keyword_owners' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (UPPER(keyword) = 'ACCOMPANY' AND BINARY help_tag <> 'ACCOMPANY')
   OR (UPPER(keyword) = 'CALM' AND BINARY help_tag <> 'CALM')
   OR (UPPER(keyword) = 'CAMP' AND BINARY help_tag <> 'CAMP')
   OR (UPPER(keyword) = 'GARROTE' AND BINARY help_tag <> 'GARROTE')
   OR (UPPER(keyword) = 'SHADOW' AND BINARY help_tag <> 'SHADOW');

SELECT
  'rol_feat_content' AS check_name,
  COUNT(*) AS actual,
  17 AS expected,
  IF(COUNT(*) = 17, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'ACCOMPANY' AS tag, 'any class; 5 ranks of perform' AS required_text
  UNION ALL SELECT 'ACCOMPANY', 'grouped performer'
  UNION ALL SELECT 'ACCOMPANY', 'take the song over'
  UNION ALL SELECT 'CALM', 'any class; charisma 13'
  UNION ALL SELECT 'CALM', 'at least 1 plus your charisma bonus'
  UNION ALL SELECT 'CALM', 'mind-affecting'
  UNION ALL SELECT 'CAMP', 'any class; 3 ranks of survival'
  UNION ALL SELECT 'CAMP', '50 percent faster'
  UNION ALL SELECT 'CAMP', 'return point'
  UNION ALL SELECT 'GARROTE', 'any class; 8 ranks of stealth and BAB 4'
  UNION ALL SELECT 'GARROTE', 'at least one free hand'
  UNION ALL SELECT 'GARROTE', 'more than one size category smaller'
  UNION ALL SELECT 'GARROTE', 'silenced and staggered'
  UNION ALL SELECT 'SHADOW', 'any class; 5 ranks of stealth'
  UNION ALL SELECT 'SHADOW', 'contested stealth check'
  UNION ALL SELECT 'SHADOW', 'entering combat'
  UNION ALL SELECT 'SHADOW', 'without joining a group'
) AS expected_content ON BINARY h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.required_text) > 0;
