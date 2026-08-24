-- Player help for the advanced Wemic race.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-WEMIC', 'Wemic

Wemics are Large monstrous humanoids with a humanoid torso and a leonine lower
body. They are the Luminari equivalent of the Realms of Luminari Barbarian
people, expressed as an actual Wemic ancestry rather than a class.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +8 Str, +4 Con, -2 Int, +2 Wis, +2 Dex, -2 Cha
Size: Large
Language: Common

Racial feats include low-light vision, Natural Athlete, Powerful Build,
Claws and Bite, Survival Instinct, and Hardy. Wemics also gain one additional
hit point per level.

See also: ACCEXP, RACE, RACE-HALF-OGRE, RACE-YUAN-TI', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('WEMIC', 'RACE-WEMIC', 'BARBARIAN', 'RACE-BARBARIAN');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-WEMIC', 'WEMIC'),
  ('RACE-WEMIC', 'RACE-WEMIC'),
  ('RACE-WEMIC', 'BARBARIAN'),
  ('RACE-WEMIC', 'RACE-BARBARIAN');

COMMIT;
