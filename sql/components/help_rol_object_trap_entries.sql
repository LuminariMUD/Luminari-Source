-- Realms of Luminari object-trap compatibility help entries.
--
-- The database help system is authoritative. This migration is idempotent and
-- documents both the existing room-trap command form and the converted object
-- form added for RoL content.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DETECT-TRAP', 'Detect Trap

Usage:
  detecttrap
  detecttrap <object>

Without an argument, DETECTTRAP uses Perception to find a trap in your current
room. A detected room trap can then be targeted with DISABLETRAP.

With an object argument, DETECTTRAP examines an object in your inventory or in
the room for a converted Realms of Luminari object trap. Object traps can be
configured for directional movement, get or put actions, opening, or lock
picking. Some affect everyone in the room.

Object detection reports the result immediately. It does not create a persistent
detection marker; this preserves the source object-trap contract.

See also: DISABLETRAP, PERCEPTION, PICKLOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('DETECT-TRAP', 'DETECT-TRAP');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('DETECT-TRAP', 'DETECTTRAP');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DISABLE-TRAP', 'Disable Trap

Usage:
  disabletrap
  disabletrap <object>

Without an argument, DISABLETRAP uses Disable Device on a room trap that you
previously detected.

With an object argument, DISABLETRAP attempts to disarm a converted Realms of
Luminari object trap in your inventory or in the room. As in the source game,
this form does not require a remembered detection marker. Success sets the
remaining charges to zero and that state persists with the object. A failure by
five or more can trigger the trap immediately.

See also: DETECTTRAP, DISABLE-DEVICE, PICKLOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('DISABLE-TRAP', 'DISABLE-TRAP');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('DISABLE-TRAP', 'DISABLETRAP');
