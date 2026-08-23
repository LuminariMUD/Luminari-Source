-- Read-only verification for help_rol_feat_entries.sql.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  5 AS expected,
  IF(COUNT(*) = 5, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN ('ACCOMPANY', 'CALM', 'CAMP', 'GARROTE', 'SHADOW');

SELECT
  'keyword_count' AS check_name,
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
