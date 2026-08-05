-- Bardic performance and instrument help entries.
--
-- The database help system is authoritative. This migration replaces the
-- legacy PERFORM and INSTRUMENT text with the implemented equipment, value,
-- durability, and flame-kissed contracts. It is safe to run repeatedly.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('PERFORM', 'Bardic Performance

Usage:
  perform <name>
  perform replace <name>
  perform add <name>
  perform stop <name>
  perform

The first verse occurs immediately. Later verses repeat about every 11 seconds
until you stop, stutter, become unable to perform, disconnect, or interrupt the
performance. Master of Motifs permits one primary and one secondary performance.

An instrument is optional. Bardic performance searches the dedicated instrument
slot shown by EQUIPMENT as {Used As Instrument} first. It then accepts instruments
in the legacy held slots for compatibility. A non-instrument object never counts.

Instrument values apply on every verse:
  Difficulty reduction lowers the performance difficulty even when the subtype
  is not ideal.
  Effectiveness bonus applies only when the subtype is ideal for the performance.
  No instrument applies -3 effectiveness.
  A non-ideal instrument applies -2 effectiveness.
  Breakability is a chance in 11,111 per verse; zero is unbreakable.

Audible performances do not affect recipients who are deaf. Dance and act
performances are visual and do not require the recipient to hear them.

See also: INSTRUMENT, PERFORMANCE-DIFFICULTY, PERFORMANCE-EFFECTIVENESS,
PERFORMANCE-STUTTER, PERFORMANCE-VERSE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORM');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'BARDIC-PERFORMANCE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCE-DIFFICULTY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCE-EFFECTIVENESS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCE-STUTTER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PERFORM', 'PERFORMANCE-VERSE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INSTRUMENT', 'Bardic Instruments

Instrument subtypes are lyre, flute, horn, drum, harp, and mandolin. Use WEAR
to equip an instrument in the dedicated slot displayed as {Used As Instrument}.
Explicitly held legacy instruments remain compatible, but the dedicated slot has
priority when more than one instrument is equipped.

An instrument stores four values:
  subtype
  difficulty reduction (called quality by the established crafting command)
  effectiveness bonus for performances that prefer that subtype
  breakability chance in 11,111 per performance verse

Crafted and summoned instruments use the dedicated slot and do not need a hold
wear flag. A summoned instrument appears in your possession; wear it before
performing if you want its benefits.

A flame-kissed multifaceted instrument can change subtype while it is worn. Say
the full subtype name, such as "say mandolin". The match is case-insensitive.
Transformation requires more than 20 hit points, costs exactly 20 hit points,
and cannot reduce you below 1. If the cost cannot be paid, the subtype and action
state do not change.

See also: PERFORM, CRAFT, SUMMON-INSTRUMENT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INSTRUMENT', 'INSTRUMENT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INSTRUMENT', 'INSTRUMENTS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INSTRUMENT', 'BARDIC-INSTRUMENT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INSTRUMENT', 'BARDIC-INSTRUMENTS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INSTRUMENT', 'SUMMON-INSTRUMENT');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('INSTRUMENT', 'FLAME-KISSED-INSTRUMENT');
