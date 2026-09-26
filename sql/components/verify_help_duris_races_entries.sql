-- Read-only verification for help_duris_races_entries.sql.

SELECT
  'duris_race_entries' AS check_name,
  COUNT(*) AS actual,
  17 AS expected,
  IF(COUNT(*) = 17, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE
  tag IN ('RACE-CENTAUR', 'RACE-GITHZERAI', 'RACE-FIRBOLG', 'RACE-GITHYANKI', 'RACE-KOBOLD', 'RACE-DRIDER', 'RACE-THRI-KREEN', 'MINOTAUR', 'RACE-KUO-TOA', 'RACE-OROG', 'RACE-HARPY', 'RACE-STORMKIN', 'RACE-DEATH-KNIGHT', 'RACE-WIGHT', 'RACE-REVENANT', 'RACE-SHADOW-BEAST', 'RACE-PHANTOM')
  AND min_level = 0 AND auto_generated = FALSE
  AND CHAR_LENGTH(TRIM(entry)) > 0;

SELECT
  'duris_race_keywords' AS check_name,
  COUNT(*) AS actual,
  38 AS expected,
  IF(COUNT(*) = 38, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE
  (help_tag, UPPER(keyword)) IN (
    ('RACE-CENTAUR', 'RACE-CENTAUR'),
    ('RACE-CENTAUR', 'CENTAUR'),
    ('RACE-GITHZERAI', 'RACE-GITHZERAI'),
    ('RACE-GITHZERAI', 'GITHZERAI'),
    ('RACE-FIRBOLG', 'RACE-FIRBOLG'),
    ('RACE-FIRBOLG', 'FIRBOLG'),
    ('RACE-GITHYANKI', 'RACE-GITHYANKI'),
    ('RACE-GITHYANKI', 'GITHYANKI'),
    ('RACE-KOBOLD', 'RACE-KOBOLD'),
    ('RACE-KOBOLD', 'KOBOLD'),
    ('RACE-DRIDER', 'RACE-DRIDER'),
    ('RACE-DRIDER', 'DRIDER'),
    ('RACE-THRI-KREEN', 'RACE-THRI-KREEN'),
    ('RACE-THRI-KREEN', 'THRI-KREEN'),
    ('RACE-THRI-KREEN', 'THRIKREEN'),
    ('MINOTAUR', 'RACE-MINOTAUR'),
    ('MINOTAUR', 'MINOTAUR'),
    ('RACE-KUO-TOA', 'RACE-KUO-TOA'),
    ('RACE-KUO-TOA', 'KUO-TOA'),
    ('RACE-KUO-TOA', 'KUOTOA'),
    ('RACE-OROG', 'RACE-OROG'),
    ('RACE-OROG', 'OROG'),
    ('RACE-HARPY', 'RACE-HARPY'),
    ('RACE-HARPY', 'HARPY'),
    ('RACE-STORMKIN', 'RACE-STORMKIN'),
    ('RACE-STORMKIN', 'STORMKIN'),
    ('RACE-DEATH-KNIGHT', 'RACE-DEATH-KNIGHT'),
    ('RACE-DEATH-KNIGHT', 'DEATH-KNIGHT'),
    ('RACE-DEATH-KNIGHT', 'DEATHKNIGHT'),
    ('RACE-WIGHT', 'RACE-WIGHT'),
    ('RACE-WIGHT', 'WIGHT'),
    ('RACE-REVENANT', 'RACE-REVENANT'),
    ('RACE-REVENANT', 'REVENANT'),
    ('RACE-SHADOW-BEAST', 'RACE-SHADOW-BEAST'),
    ('RACE-SHADOW-BEAST', 'SHADOW-BEAST'),
    ('RACE-SHADOW-BEAST', 'SHADOWBEAST'),
    ('RACE-PHANTOM', 'RACE-PHANTOM'),
    ('RACE-PHANTOM', 'PHANTOM')
  );

SELECT
  'duris_race_keyword_conflicts' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE
  UPPER(keyword) IN ('RACE-CENTAUR', 'CENTAUR', 'RACE-GITHZERAI', 'GITHZERAI', 'RACE-FIRBOLG', 'FIRBOLG', 'RACE-GITHYANKI', 'GITHYANKI', 'RACE-KOBOLD', 'KOBOLD', 'RACE-DRIDER', 'DRIDER', 'RACE-THRI-KREEN', 'THRI-KREEN', 'THRIKREEN', 'RACE-MINOTAUR', 'MINOTAUR', 'RACE-KUO-TOA', 'KUO-TOA', 'KUOTOA', 'RACE-OROG', 'OROG', 'RACE-HARPY', 'HARPY', 'RACE-STORMKIN', 'STORMKIN', 'RACE-DEATH-KNIGHT', 'DEATH-KNIGHT', 'DEATHKNIGHT', 'RACE-WIGHT', 'WIGHT', 'RACE-REVENANT', 'REVENANT', 'RACE-SHADOW-BEAST', 'SHADOW-BEAST', 'SHADOWBEAST', 'RACE-PHANTOM', 'PHANTOM')
  AND help_tag NOT IN ('RACE-CENTAUR', 'RACE-GITHZERAI', 'RACE-FIRBOLG', 'RACE-GITHYANKI', 'RACE-KOBOLD', 'RACE-DRIDER', 'RACE-THRI-KREEN', 'MINOTAUR', 'RACE-KUO-TOA', 'RACE-OROG', 'RACE-HARPY', 'RACE-STORMKIN', 'RACE-DEATH-KNIGHT', 'RACE-WIGHT', 'RACE-REVENANT', 'RACE-SHADOW-BEAST', 'RACE-PHANTOM');

SELECT
  'duris_race_content' AS check_name,
  COUNT(*) AS actual,
  10 AS expected,
  IF(COUNT(*) = 10, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT
    'RACE-CENTAUR' AS required_tag,
    'Stampede' AS required_text
  UNION ALL
  SELECT
    'RACE-THRI-KREEN',
    '50,000 account experience'
  UNION ALL
  SELECT
    'RACE-THRI-KREEN',
    'Four Arms'
  UNION ALL
  SELECT
    'RACE-KOBOLD',
    'Tier: Normal'
  UNION ALL
  SELECT
    'MINOTAUR',
    'Bloodlust'
  UNION ALL
  SELECT
    'RACE-DRIDER',
    'Tauric Frame'
  UNION ALL
  SELECT
    'RACE-DEATH-KNIGHT',
    'levels in Blackguard or Warrior'
  UNION ALL
  SELECT
    'RACE-WIGHT',
    '9/-'
  UNION ALL
  SELECT
    'RACE-PHANTOM',
    'descend form'
  UNION ALL
  SELECT
    'EPIC-RACES',
    'Thri-Kreen'
) AS expected_content
  ON
    h.tag = expected_content.required_tag
    AND INSTR(h.entry, expected_content.required_text) > 0;
