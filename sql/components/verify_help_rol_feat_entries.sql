-- Read-only verification for help_rol_feat_entries.sql.

SELECT
  'rol_feat_entries' AS check_name,
  COUNT(*) AS actual,
  6 AS expected,
  IF(COUNT(*) = 6, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('ACCOMPANY', 'ACTIVITY', 'CALM', 'CAMP', 'GARROTE', 'SHADOW')
  AND min_level = 0
  AND auto_generated = FALSE
  AND entry IS NOT NULL
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'rol_feat_keywords' AS check_name,
  COUNT(*) AS actual,
  11 AS expected,
  IF(COUNT(*) = 11, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('ACCOMPANY', 'ACCOMPANY'),
  ('ACTIVITY', 'ACTIVITY'),
  ('ACTIVITY', 'PRIMARY-ACTIVITY'),
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
  11 AS expected,
  IF(COUNT(*) = 11, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag IN ('ACCOMPANY', 'ACTIVITY', 'CALM', 'CAMP', 'GARROTE', 'SHADOW');

SELECT
  'rol_feat_command_keyword_owners' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (UPPER(keyword) = 'ACCOMPANY' AND BINARY help_tag <> 'ACCOMPANY')
   OR (UPPER(keyword) = 'ACTIVITY' AND BINARY help_tag <> 'ACTIVITY')
   OR (UPPER(keyword) = 'PRIMARY-ACTIVITY' AND BINARY help_tag <> 'ACTIVITY')
   OR (UPPER(keyword) = 'CALM' AND BINARY help_tag <> 'CALM')
   OR (UPPER(keyword) = 'CAMP' AND BINARY help_tag <> 'CAMP')
   OR (UPPER(keyword) = 'GARROTE' AND BINARY help_tag <> 'GARROTE')
   OR (UPPER(keyword) = 'SHADOW' AND BINARY help_tag <> 'SHADOW');

SELECT
  'rol_feat_content' AS check_name,
  COUNT(*) AS actual,
  26 AS expected,
  IF(COUNT(*) = 26, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'ACCOMPANY' AS tag, 'any class; 5 ranks of perform' AS required_text
  UNION ALL SELECT 'ACCOMPANY', 'bards gain it for free at level 2'
  UNION ALL SELECT 'ACCOMPANY', 'grouped performer'
  UNION ALL SELECT 'ACCOMPANY', 'take the song over'
  UNION ALL SELECT 'ACTIVITY', 'Usage: activity'
  UNION ALL SELECT 'ACTIVITY', 'occupied capabilities'
  UNION ALL SELECT 'ACTIVITY', 'Incompatible actions'
  UNION ALL SELECT 'ACTIVITY', 'Timed casting also appears here'
  UNION ALL SELECT 'ACTIVITY', 'Casting cannot be paused'
  UNION ALL SELECT 'CALM', 'any class; charisma 19'
  UNION ALL SELECT 'CALM', 'at least 1 plus your charisma bonus'
  UNION ALL SELECT 'CALM', 'mind-affecting'
  UNION ALL SELECT 'CAMP', 'any class; 3 ranks of nature'
  UNION ALL SELECT 'CAMP', '50 percent faster'
  UNION ALL SELECT 'CAMP', 'Anyone there recovers'
  UNION ALL SELECT 'CAMP', 'return point'
  UNION ALL SELECT 'CAMP', 'six seconds'
  UNION ALL SELECT 'CAMP', 'Use activity'
  UNION ALL SELECT 'GARROTE', 'any class; 14 ranks of stealth and BAB 8'
  UNION ALL SELECT 'GARROTE', 'at least one free hand'
  UNION ALL SELECT 'GARROTE', 'more than one size category smaller'
  UNION ALL SELECT 'GARROTE', 'silenced and staggered'
  UNION ALL SELECT 'SHADOW', 'any class; 21 ranks of stealth'
  UNION ALL SELECT 'SHADOW', 'stealth check against'
  UNION ALL SELECT 'SHADOW', 'entering combat'
  UNION ALL SELECT 'SHADOW', 'without joining a group'
) AS expected_content ON BINARY h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.required_text) > 0;
