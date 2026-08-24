-- Player help for the advanced Yuan-Ti race.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-YUAN-TI', 'Yuan-Ti

Yuan-ti are serpentfolk whose controlled minds, scaled bodies, venom, and
innate magic make them dangerous adventurers. Pureblooded yuan-ti can pass
among humanoids, but their reptilian heritage is never entirely hidden.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +2 Int, +2 Dex, +2 Cha
Size: Medium
Language: Draconic

Racial feats include ultravision, Poison Bite, poison immunity, Stubborn Mind,
and two stacking ranks of Armor Skin. Poison Bite applies when fighting
bare-handed.

See also: ACCEXP, POISON, RACE, RACE-HALF-ILLITHID', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('YUAN-TI', 'YUANTI', 'RACE-YUAN-TI');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-YUAN-TI', 'YUAN-TI'),
  ('RACE-YUAN-TI', 'YUANTI'),
  ('RACE-YUAN-TI', 'RACE-YUAN-TI');

COMMIT;
