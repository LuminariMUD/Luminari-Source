-- Player help for Trelux anatomy-based equipment restrictions.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('TRELUX-EQ', 'Trelux Equipment

Trelux anatomy does not support several traditional equipment slots.

Their pincer-like forelimbs cannot wield weapons, hold items, use shields,
wear gloves, or wear rings. Their insect-like legs cannot wear leg or foot
equipment. Arm, wrist, and ankle equipment slots remain available.

These restrictions apply to ordinary wear commands, saved equipment, and
equipment placed by game systems.

See also: RACE-TRELUX, TRELUX-PINCERS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('TRELUX-EQ', 'TRELUX-EQUIPMENT');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('TRELUX-EQ', 'TRELUX-EQ'),
  ('TRELUX-EQ', 'TRELUX-EQUIPMENT');

COMMIT;
