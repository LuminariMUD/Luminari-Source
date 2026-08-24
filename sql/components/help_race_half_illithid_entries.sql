-- Player help for the epic Half-Illithid race.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-HALF-ILLITHID', 'Half-Illithid

Half-illithids retain a mortal ancestry while bearing an illithid-transformed
mind and unsettling physical traits. They are the Luminari equivalent of the
Realms of Luminari Illithid player race, expressed as an actual Half-Illithid.

Tier: Epic (level adjustment 10)
Unlock: 30,000 account experience
Progression: 7x normal experience requirements
Ability modifiers: +4 Int, +4 Wis, +4 Cha
Size: Medium
Language: Aberration

Racial feats include ultravision, Quick Mind, Stubborn Mind,
innate Levitation, three stacking ranks of Armor Skin, Vital, and Hardy.
Half-illithids gain four additional hit points per level.

See also: ACCEXP, LEVITATE, PSIONICIST, RACE, RACE-YUAN-TI', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN
  ('HALF-ILLITHID', 'HALF-ILLITHID-RACE', 'RACE-HALF-ILLITHID', 'ILLITHID',
   'RACE-ILLITHID');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('RACE-HALF-ILLITHID', 'HALF-ILLITHID'),
  ('RACE-HALF-ILLITHID', 'HALF-ILLITHID-RACE'),
  ('RACE-HALF-ILLITHID', 'RACE-HALF-ILLITHID'),
  ('RACE-HALF-ILLITHID', 'ILLITHID'),
  ('RACE-HALF-ILLITHID', 'RACE-ILLITHID');

COMMIT;
