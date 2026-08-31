-- Semantic encounter-round player help.
--
-- The database help system is authoritative. This migration is safe to run
-- repeatedly and replaces the combat entry with the current round rules.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('combat', 'COMBAT

Start combat with KILL <target>, HIT <target>, or a hostile combat maneuver.
An attack maneuver already in your attack queue can replace a normal attack.

COMBAT ROUNDS

One encounter round lasts 6 seconds. Everyone whose turn is ready acts from
highest initiative to lowest. Ties use Dexterity and then a stable final order.
Someone joining a fight becomes eligible on the encounter''s next round;
joining or merging fights never grants an extra early turn.

ACTIONS IN COMBAT

Your reaction allowance refreshes before initiative begins each round. At the
start of your turn, due standard, move, and swift actions recover.
One valid queued command is attempted first.
Any actions it spends are unavailable to your automatic attack. If both your
standard and move actions remain, you perform your full attack rotation. With
only a standard action, you perform the first attack portion. With no standard
action, you make no automatic attack.

See also: ACTIONS, ACTION-QUEUE, ATTACK-QUEUE, ATTACKS, COMBAT-MODES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('COMBAT', 'COMBAT-PHASE', 'COMBAT-ROUNDS', 'FIGHTING')
  AND help_tag <> 'combat';
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT-PHASE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT-ROUNDS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'FIGHTING');

COMMIT;
