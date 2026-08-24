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
Ability modifiers: +2 Int, +4 Dex, +2 Cha
Size: Medium
Language: Draconic

Racial feats include ultravision, Poison Bite, poison immunity, Stubborn Mind,
and two stacking ranks of Armor Skin. Poison Bite applies when fighting
bare-handed. Their serpentine anatomy has no face, leg, or foot equipment slots.
Yuan-ti instead have a tail equipment slot, where they can wear any ring or use
dedicated tail gear that cannot be worn in another slot.

See also: ACCEXP, POISON, RACE, RACE-HALF-ILLITHID, WEAR', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('YUAN-TI', 'YUANTI', 'RACE-YUAN-TI');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-YUAN-TI', 'YUAN-TI'),
  ('RACE-YUAN-TI', 'YUANTI'),
  ('RACE-YUAN-TI', 'RACE-YUAN-TI');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('WEAR', 'Usage: wear <item> [location]

If you want to wear some clothes, armor or the likes.

Also, to wear everything in your inventory (or at least try to, as wearing
things like loaves of bread is not a good way to win friends and influence
people) you can type "wear all".

Optionally, you can specify what part of your body to wear the equipment on.

Examples:

  > wear boots
  > wear all.bronze
  > wear all
  > wear ring finger
  > wear ring tail

Yuan-ti have one tail equipment slot. Any ring may be worn there, even when
the ring is not specifically marked as tail gear. Items specifically made for
the tail can only be worn in that slot. Characters without a tail slot cannot
wear rings or dedicated gear there.

See also: EQUIPMENT, REMOVE, RACE-YUAN-TI', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE help_tag = 'WEAR';

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('WEAR', 'TAIL-SLOT', 'TAIL-EQUIPMENT')
  AND BINARY help_tag <> 'WEAR';

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('WEAR', 'WEAR'),
  ('WEAR', 'TAIL-SLOT'),
  ('WEAR', 'TAIL-EQUIPMENT');

COMMIT;
