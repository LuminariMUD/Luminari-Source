-- Read-only verification for help_intermud3_entries.sql.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'INTERMUD3', 'I3TELL', 'I3CHAT', 'I3WHO', 'I3FINGER',
  'I3LOCATE', 'I3MUDLIST', 'I3CHANNELS', 'I3CONFIG', 'I3ADMIN'
);

SELECT
  'required_keywords' AS check_name,
  COUNT(*) AS actual,
  31 AS expected,
  IF(COUNT(*) = 31, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('INTERMUD3', 'I3'),
  ('INTERMUD3', 'I3-COMMANDS'),
  ('INTERMUD3', 'INTERMUD'),
  ('INTERMUD3', 'INTERMUD3'),
  ('I3TELL', 'I3-TELL'),
  ('I3TELL', 'I3TELL'),
  ('I3TELL', 'INTERMUD-TELL'),
  ('I3CHAT', 'I3-CHAT'),
  ('I3CHAT', 'I3CHAT'),
  ('I3CHAT', 'INTERMUD-CHAT'),
  ('I3WHO', 'I3-WHO'),
  ('I3WHO', 'I3WHO'),
  ('I3WHO', 'INTERMUD-WHO'),
  ('I3FINGER', 'I3-FINGER'),
  ('I3FINGER', 'I3FINGER'),
  ('I3FINGER', 'INTERMUD-FINGER'),
  ('I3LOCATE', 'I3-LOCATE'),
  ('I3LOCATE', 'I3LOCATE'),
  ('I3LOCATE', 'INTERMUD-LOCATE'),
  ('I3MUDLIST', 'I3-MUDLIST'),
  ('I3MUDLIST', 'I3MUDLIST'),
  ('I3MUDLIST', 'INTERMUD-MUDLIST'),
  ('I3CHANNELS', 'I3-CHANNELS'),
  ('I3CHANNELS', 'I3CHANNELS'),
  ('I3CHANNELS', 'INTERMUD-CHANNELS'),
  ('I3CONFIG', 'I3-CONFIG'),
  ('I3CONFIG', 'I3CONFIG'),
  ('I3CONFIG', 'INTERMUD-CONFIG'),
  ('I3ADMIN', 'I3-ADMIN'),
  ('I3ADMIN', 'I3ADMIN'),
  ('I3ADMIN', 'INTERMUD-ADMIN')
);

SELECT
  'access_levels' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  (tag = 'I3ADMIN' AND min_level = 31)
  OR
  (
    tag IN (
      'INTERMUD3', 'I3TELL', 'I3CHAT', 'I3WHO', 'I3FINGER',
      'I3LOCATE', 'I3MUDLIST', 'I3CHANNELS', 'I3CONFIG'
    )
    AND min_level = 0
  );

SELECT
  'nonempty_entries' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'INTERMUD3', 'I3TELL', 'I3CHAT', 'I3WHO', 'I3FINGER',
  'I3LOCATE', 'I3MUDLIST', 'I3CHANNELS', 'I3CONFIG', 'I3ADMIN'
)
AND entry IS NOT NULL
AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'primary_keywords' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('INTERMUD3', 'INTERMUD3'),
  ('I3TELL', 'I3TELL'),
  ('I3CHAT', 'I3CHAT'),
  ('I3WHO', 'I3WHO'),
  ('I3FINGER', 'I3FINGER'),
  ('I3LOCATE', 'I3LOCATE'),
  ('I3MUDLIST', 'I3MUDLIST'),
  ('I3CHANNELS', 'I3CHANNELS'),
  ('I3CONFIG', 'I3CONFIG'),
  ('I3ADMIN', 'I3ADMIN')
);
