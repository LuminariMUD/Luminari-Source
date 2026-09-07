-- Read-only verification for help_rol_player_kits.sql.

SELECT
  'rol_player_kit_entries' AS check_name,
  COUNT(*) AS actual,
  13 AS expected,
  IF(COUNT(*) = 13, 'PASS', 'FAIL') AS result
FROM help_entries
WHERE tag IN (
  'ADVANCED-SPELL-COMMANDS', 'animal-companion', 'BATTLECHANTER', 'DIRE-RAIDER',
  'ELEMENTALIST', 'ELEMENTAL-AIR-EMBODIMENT', 'ELEMENTAL-EARTH-EMBODIMENT',
  'ELEMENTAL-FIRE-EMBODIMENT', 'ELEMENTAL-WATER-EMBODIMENT', 'ROL-MERCENARY',
  'ROL-SPELL-KITS', 'SHAMAN', 'SONG-OF-TRAVEL'
)
  AND min_level = 0
  AND auto_generated = FALSE;

SELECT
  'rol_player_kit_keywords' AS check_name,
  COUNT(*) AS actual,
  14 AS expected,
  IF(COUNT(*) = 14, 'PASS', 'FAIL') AS result
FROM help_keywords
WHERE help_tag IN (
  'BATTLECHANTER', 'DIRE-RAIDER', 'ELEMENTALIST', 'ROL-MERCENARY', 'ROL-SPELL-KITS', 'SHAMAN'
);

SELECT
  'rol_player_kit_content' AS check_name,
  COUNT(*) AS actual,
  16 AS expected,
  IF(COUNT(*) = 16, 'PASS', 'FAIL') AS result
FROM help_entries AS h
JOIN (
  SELECT 'ADVANCED-SPELL-COMMANDS' AS tag, 'Requires any two Focused Element perks' AS text
  UNION ALL SELECT 'animal-companion', 'Ranger 4/Warrior 1'
  UNION ALL SELECT 'animal-companion', 'tame, mountable'
  UNION ALL SELECT 'BATTLECHANTER', 'song of travel at level 16'
  UNION ALL SELECT 'DIRE-RAIDER', 'Ranger 4, Warrior 1'
  UNION ALL SELECT 'DIRE-RAIDER', 'poltergeist at 15'
  UNION ALL SELECT 'ELEMENTALIST', 'Master of Elements requires'
  UNION ALL SELECT 'ELEMENTALIST', 'earthblood at 17'
  UNION ALL SELECT 'ELEMENTAL-AIR-EMBODIMENT', 'Access: Wizard level 15'
  UNION ALL SELECT 'ELEMENTAL-EARTH-EMBODIMENT', 'Access: Wizard level 17'
  UNION ALL SELECT 'ELEMENTAL-FIRE-EMBODIMENT', 'Access: Wizard level 17'
  UNION ALL SELECT 'ELEMENTAL-WATER-EMBODIMENT', 'Access: Wizard level 13'
  UNION ALL SELECT 'ROL-MERCENARY', 'Warrior/Rogue multiclass build'
  UNION ALL SELECT 'ROL-SPELL-KITS', 'content-only'
  UNION ALL SELECT 'SHAMAN', 'summoning unlocks at Cleric level'
  UNION ALL SELECT 'SONG-OF-TRAVEL', 'Access: Bard level 16'
) AS expected_content ON BINARY h.tag = expected_content.tag
WHERE INSTR(h.entry, expected_content.text) > 0;
