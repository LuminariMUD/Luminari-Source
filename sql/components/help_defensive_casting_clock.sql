-- Native and semantic Defensive Casting expiry.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('DEFENSIVE-CASTING-PERK', 'The Wizard Versatile Caster perk grants +4 dodge AC after casting a spell.

When activated in combat, the bonus expires at the start of your next turn,
before your actions become available. Outside combat it lasts six seconds.
An interval already running outside combat keeps its remaining time if you
enter combat. Leaving combat preserves the time left until your next turn;
it does not grant a fresh six seconds. Saved remaining time resumes when your
character is loaded again. A character still in the world after a lost connection
keeps its live clock. Casting again refreshes the bonus.

This perk is separate from the Concentration check used to cast defensively.

See also: PERKS, DEFENSIVE-CASTING, COMBAT, INITIATIVE
', 0, 0) ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DEFENSIVE-CASTING-PERK', 'DEFENSIVE-CASTING-PERK');

COMMIT;
