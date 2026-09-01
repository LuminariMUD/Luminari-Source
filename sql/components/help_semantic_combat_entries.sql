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

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('initiative-order', 'INITIATIVE

Usage:
  initiative

Initiative determines combat turn order. It is rolled as 1d20 plus Dexterity
and other bonuses when a combatant enters an encounter.

Use INITIATIVE while fighting to see the current encounter round, the time
until the next round, and visible combatants in their scheduled turn order.
Your own name is highlighted in green when color is enabled.

See also: COMBAT, ACTIONS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('INITIATIVE', 'INITIATIVE-ORDER')
  AND help_tag <> 'initiative-order';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('initiative-order', 'INITIATIVE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('initiative-order', 'INITIATIVE-ORDER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ready-action', 'READY READIED-ACTION

Usage:
  ready <command> on entry [target]
  ready
  ready cancel

READY listens for someone entering your current room. With no target, the
first other player or mobile to enter triggers the command. With a target,
nonmatching arrivals are ignored and the action remains ready. The target
filters the arrival; include any command target in the command itself.

The command runs through the normal command interpreter at the next safe event
boundary, about one tenth of a second later. A readied attack therefore starts
or joins combat normally and does not wait for the next six-second combat
round. It does not preempt a command already accepted in the current cycle.

Moving, dying, logging out, or using READY CANCEL removes the action. READY by
itself shows the currently prepared action. Readied actions do not survive a
copyover or reboot.

Examples:
  ready say Welcome! on entry
  ready kill guard on entry guard

See also: COMBAT, INITIATIVE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('READY', 'READIED-ACTION')
  AND help_tag <> 'ready-action';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ready-action', 'READY');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ready-action', 'READIED-ACTION');

COMMIT;
