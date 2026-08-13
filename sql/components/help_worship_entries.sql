-- Player help for the WORSHIP command used by converted RoL sacred objects.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('WORSHIP', 'Worship

Usage:
  worship

WORSHIP is a contextual act of devotion. Most places have no local ritual, but
certain altars or sacred objects may respond when you worship in the required
posture. Examine the shrine for clues before trying the command.

The converted Spiderhaunt altar of Cyric requires you to be sitting. If the
altar still has power, worshipping there grants a limited blessing and draws
your alignment slightly toward evil.

See also: ALIGNMENT, DEITIES, SIT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE help_tag = 'WORSHIP' AND keyword <> 'WORSHIP';

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('WORSHIP', 'WORSHIP');
