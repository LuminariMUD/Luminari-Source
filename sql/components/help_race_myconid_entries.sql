-- Player help for the epic Myconid race.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-MYCONID', 'Myconid

Myconids are Large fungal beings whose alien plant biology resists poison,
paralysis, and sleep. This epic ancestry completes the unfinished Myconid
player concept from Realms of Luminari.

Tier: Epic (level adjustment 10)
Unlock: 30,000 account experience
Progression: 7x normal experience requirements
Ability modifiers: +8 Str, +6 Con, -2 Int, -2 Wis, -4 Dex, -4 Cha
Size: Large
Language: Undercommon

Racial feats include ultravision, Vital, Hardy, poison immunity, sleep
immunity, paralysis immunity, and four stacking ranks of Armor Skin. Myconids
count as plants and gain four additional hit points per level.

See also: ACCEXP, RACE, RACE-HALF-OGRE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('MYCONID', 'MYCANOID', 'RACE-MYCONID');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-MYCONID', 'MYCONID'),
  ('RACE-MYCONID', 'MYCANOID'),
  ('RACE-MYCONID', 'RACE-MYCONID');

COMMIT;
