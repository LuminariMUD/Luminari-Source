-- Arcane mark command and cantrip help.
--
-- The database help system is authoritative. These entries distinguish the
-- signature-setting command from the object-targeting spell and are safe to
-- apply repeatedly.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ARCANEMARK', 'Arcane Mark Signature

Usage:
  arcanemark
  arcanemark <signature>
  arcanemark clear

ARCANEMARK configures the signature text used by the ARCANE MARK cantrip. It
does not target or change an object. With no argument, the command displays your
current signature and setup instructions.

A signature may contain up to 250 characters, including color codes. Use the
command again to change it at any time, or use ARCANEMARK CLEAR to remove it.
Clearing or changing your configured signature does not alter objects you marked
earlier. Keep the text in-character and tasteful.

After configuring the signature, apply it with:
  cast ''arcane mark'' <object>

The target must be in your inventory.

See also: ARCANE-MARK, CAST, LOOK, EXAMINE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ARCANE-MARK', 'Arcane Mark

Usage:
  cast ''arcane mark'' <object>

ARCANE MARK is a cantrip available to wizards, sorcerers, and summoners. It
copies the signature configured with ARCANEMARK onto one object in your
inventory. The spell refuses an object that already has a mark; it does not
replace the existing signature.

On success, the spell reports the applied text. Anyone who can inspect the
object can read it with LOOK <object> or EXAMINE <object>. The mark is stored on
the individual object and survives normal player inventory save and load.

Arcane marks are for signatures, provenance, and roleplay. They grant no
ownership, theft protection, tracking, identification bonus, or other combat or
gameplay benefit.

Staff diagnostics show a character''s configured signature in STAT PLAYER and
STAT FILE, and an object''s applied mark in STAT OBJECT.

See also: ARCANEMARK, CAST, LOOK, EXAMINE, STAT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE help_tag = 'ARCANEMARK'
  AND keyword NOT IN ('ARCANEMARK', 'ARCANE-MARK-SIGNATURE');

DELETE FROM help_keywords
WHERE help_tag = 'ARCANE-MARK'
  AND keyword NOT IN ('ARCANE-MARK', 'SPELL-ARCANE-MARK');

DELETE FROM help_keywords
WHERE keyword IN ('ARCANEMARK', 'ARCANE-MARK-SIGNATURE') AND help_tag <> 'ARCANEMARK';

DELETE FROM help_keywords
WHERE keyword IN ('ARCANE-MARK', 'SPELL-ARCANE-MARK') AND help_tag <> 'ARCANE-MARK';

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ARCANEMARK', 'ARCANEMARK');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ARCANEMARK', 'ARCANE-MARK-SIGNATURE');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ARCANE-MARK', 'ARCANE-MARK');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ARCANE-MARK', 'SPELL-ARCANE-MARK');
