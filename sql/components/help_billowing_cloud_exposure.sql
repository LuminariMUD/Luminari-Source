-- Billowing Cloud source and exposure help.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('BILLOWING-CLOUD', 'Usage: cast ''billowing cloud''
Duration: 15 world rounds
School: Conjuration
Target: Room
Saving throw: Fortitude

The cloud tests creatures of level 12 or lower when it forms around them or
when they enter it. A failed save consumes a move action, or a standard action
if their move action is already spent.

A creature that remains inside is tested no more than once per six-second
interval. In combat, continued exposure is resolved after that creature''s
eligible turn. Leaving and re-entering during the same interval does not cause
another save. Each cloud source tracks exposure separately, and an expired or
removed cloud cannot cause a later exposure.

See also: SPELLS, COMBAT, ACTIONS
', 0, 0) ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level), auto_generated=VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BILLOWING-CLOUD', 'BILLOWING-CLOUD');

COMMIT;
