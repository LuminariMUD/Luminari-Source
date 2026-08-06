-- Read-only verification for help_necromancer_entries.sql.

SELECT
  'entry_count' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(COUNT(*) = 9, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'class-necromancer', 'animate-dead', 'greater-animation',
  'touch-of-undeath', 'bone-armor', 'undead-cohort',
  'tough-as-bone', 'essence-of-undeath', 'animatedead'
);

SELECT
  'required_keywords' AS check_name,
  COUNT(*) AS actual,
  23 AS expected,
  IF(COUNT(*) = 23, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE (help_tag, keyword) IN (
  ('class-necromancer', 'CLASS-NECROMANCER'),
  ('class-necromancer', 'NECROMANCER'),
  ('animate-dead', 'ANIMATE-DEAD'),
  ('animate-dead', 'SPELL-ANIMATE-DEAD'),
  ('animate-dead', 'SUMMON-UNDEAD'),
  ('greater-animation', 'GREATER-ANIMATION'),
  ('greater-animation', 'SPELL-GREATER-ANIMATION'),
  ('greater-animation', 'SUMMON-GREATER-UNDEAD'),
  ('touch-of-undeath', 'TOUCH-OF-UNDEATH'),
  ('touch-of-undeath', 'UNDEATH'),
  ('touch-of-undeath', 'PARALYZING-TOUCH'),
  ('touch-of-undeath', 'WEAKENING-TOUCH'),
  ('touch-of-undeath', 'DEGENERATIVE-TOUCH'),
  ('touch-of-undeath', 'DESTRUCTIVE-TOUCH'),
  ('touch-of-undeath', 'DEATHLESS-TOUCH'),
  ('bone-armor', 'BONE-ARMOR'),
  ('bone-armor', 'BONEARMOR'),
  ('undead-cohort', 'UNDEAD-COHORT'),
  ('undead-cohort', 'CALL-COHORT'),
  ('undead-cohort', 'COHORT'),
  ('tough-as-bone', 'TOUGH-AS-BONE'),
  ('essence-of-undeath', 'ESSENCE-OF-UNDEATH'),
  ('animatedead', 'ANIMATEDEAD')
);

SELECT
  'player_manual_entries' AS check_name,
  COUNT(*) AS actual,
  9 AS expected,
  IF(COUNT(*) = 9, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'class-necromancer', 'animate-dead', 'greater-animation',
  'touch-of-undeath', 'bone-armor', 'undead-cohort',
  'tough-as-bone', 'essence-of-undeath', 'animatedead'
)
AND min_level = 0
AND auto_generated = FALSE;

SELECT
  'removed_animatedead_spell_aliases' AS check_name,
  COUNT(*) AS actual,
  0 AS expected,
  IF(COUNT(*) = 0, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag = 'animatedead' AND LOWER(keyword) IN ('animate', 'animate-dead');

SELECT
  'content_contracts' AS check_name,
  SUM(INSTR(LOWER(h.entry), LOWER(expected_content.required_text)) > 0) AS actual,
  20 AS expected,
  IF(SUM(INSTR(LOWER(h.entry), LOWER(expected_content.required_text)) > 0) = 20,
     'PASS', 'FAIL') AS result,
  GROUP_CONCAT(
    IF(INSTR(LOWER(h.entry), LOWER(expected_content.required_text)) > 0,
       NULL, CONCAT(expected_content.tag, ': ', expected_content.required_text))
    ORDER BY expected_content.tag SEPARATOR '; '
  ) AS missing_contracts
FROM (
  SELECT 'class-necromancer' AS tag, '5,000 account points' AS required_text
  UNION ALL SELECT 'class-necromancer', 'four plus Intelligence'
  UNION ALL SELECT 'class-necromancer', '8  Medium Armor'
  UNION ALL SELECT 'animate-dead', '10 percent summon failure chance'
  UNION ALL SELECT 'animate-dead', 'exactly two animated undead'
  UNION ALL SELECT 'animate-dead', 'prepared spell or spontaneous spell slot'
  UNION ALL SELECT 'greater-animation', '10 percent summon failure chance'
  UNION ALL SELECT 'greater-animation', 'final follower level also scales'
  UNION ALL SELECT 'touch-of-undeath', 'whether the touch attack hits or misses'
  UNION ALL SELECT 'touch-of-undeath', 'selected preferred spellcasting class'
  UNION ALL SELECT 'touch-of-undeath', '1d4+1 rounds'
  UNION ALL SELECT 'bone-armor', 'exactly one armor piece or shield'
  UNION ALL SELECT 'bone-armor', 'one third of the item'
  UNION ALL SELECT 'bone-armor', 'when at least one relevant'
  UNION ALL SELECT 'undead-cohort', 'combined Summoner and Necromancer'
  UNION ALL SELECT 'undead-cohort', 'granted for free'
  UNION ALL SELECT 'tough-as-bone', 'disease and stun'
  UNION ALL SELECT 'essence-of-undeath', 'physical ability drain'
  UNION ALL SELECT 'animatedead', 'separate daily class ability'
  UNION ALL SELECT 'animatedead', 'does not target or consume'
) AS expected_content
LEFT JOIN help_entries AS h ON h.tag = expected_content.tag;
