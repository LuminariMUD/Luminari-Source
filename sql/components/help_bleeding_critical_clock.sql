-- Bleeding Critical tactical clock help.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('BLEEDING-CRITICAL', 'Bleeding Critical adds bleeding damage when a critical hit applies the feat.
Its 2d6 damage is rolled on application and stored for each later tick. Repeated
applications add their damage and replace the remaining duration, without
postponing the pending tick. The initial Fortitude save and duration roll retain
their existing rules.

In combat, each tick happens at the end of the affected character''s next turn,
after that character''s actions. Outside combat, ticks are six seconds apart.
Each tick applies damage and consumes one remaining round; the final tick ends
the effect. Curing the effect cancels its future ticks.

An elapsed interval already running when combat begins finishes before the
clock adopts turn-end timing. Leaving combat preserves the time remaining to
the next tick. Player saves preserve remaining rounds and the partial interval;
saving does not restart the clock. A character remaining in the world after
losing its connection continues to bleed normally.

See also: FEATS, CRITICAL, COMBAT, INITIATIVE
', 0, 0) ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level), auto_generated=VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BLEEDING-CRITICAL', 'BLEEDING-CRITICAL');

COMMIT;
