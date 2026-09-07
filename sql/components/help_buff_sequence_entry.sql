-- Native buff sequence command help.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('AUTO-BUFF', 'Usage: buff add <spell or power>
       buff remove <spell or power>
       buff <psp> <power>
       buff list
       buff target [name|self]
       buff perform
       buff cancel

Create a saved list of spells and psionic powers, then use buff perform to
cast them in order. Normal knowledge, preparation, resource and action checks
apply to each cast. Unavailable spells are skipped; preparations are consumed
by normal casting, once per attempted cast that reaches that stage.

A timed cast finishes before the next spell begins. Casting speed changes
apply through normal casting. Instant casts retain the sequence''s pacing.
You cannot start a sequence while another primary activity is in progress.

The selected target is captured when the sequence starts. Changing buff target
sets the target for your next sequence. With no target, you buff yourself.
Movement by you or the selected target, target loss, interrupted casting,
disconnect or restart stops the sequence. Your saved buff list remains intact.
Use buff perform again when ready. Buff cancel stops further spells; a spell
already being cast retains the normal casting and interruption rules.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry=VALUES(entry), min_level=VALUES(min_level), auto_generated=VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'AUTO-BUFF');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'AUTOBUFF');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'BUFF-SELF');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'BUFFING');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'SELF-BUFFING');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AUTO-BUFF', 'BUFF');

COMMIT;
