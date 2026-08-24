-- Player help for the advanced Half-Ogre race.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-HALF-OGRE', 'Half-Ogre

Half-ogres are Large giant-blooded people who combine tremendous strength and
reach with mortal adaptability. They are the Luminari equivalent of the Realms
of Luminari Ogre player race, expressed as an actual Half-Ogre ancestry.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +6 Str, +2 Con, -2 Int, -2 Dex, -2 Cha
Size: Large
Language: Giant

Racial feats include ultravision, Powerful Build, strong poison resistance,
and two stacking ranks of Armor Skin. Half-ogres gain two additional hit
points per level.

See also: ACCEXP, RACE, RACE-WEMIC, RACE-MYCONID', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN
  ('HALF-OGRE', 'HALF-OGRE-RACE', 'RACE-HALF-OGRE', 'OGRE', 'RACE-OGRE');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-HALF-OGRE', 'HALF-OGRE'),
  ('RACE-HALF-OGRE', 'HALF-OGRE-RACE'),
  ('RACE-HALF-OGRE', 'RACE-HALF-OGRE'),
  ('RACE-HALF-OGRE', 'OGRE'),
  ('RACE-HALF-OGRE', 'RACE-OGRE');

COMMIT;
