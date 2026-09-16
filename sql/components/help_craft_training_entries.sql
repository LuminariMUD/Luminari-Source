-- Paid craft trainer help (issue 196). Apply to the development help database with the
-- matching lib/text/help/help.hlp update; repeated application is safe.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('apprentice', 'Usage: apprentice
       apprentice <skill>
       apprentice <skill> confirm

A craft trainer teaches a crafting or harvesting skill while you spend a day
away from the world. Stand beside a trainer, such as the master artisan in the
Sanctus crafting district, and type APPRENTICE to see each skill it teaches,
your rank, the experience a contract grants, and the fee. APPRENTICE <skill>
quotes the terms and changes nothing; add CONFIRM to begin.

Trainers teach alchemy, armorsmithing, jewelcrafting, leatherworking,
metalworking, tailoring, weaponsmithing, woodworking, forestry, gathering,
hunting, and mining, and only below rank 20. A contract grants half of the
experience your next rank requires, plus any insightful talent bonus, and never
raises the skill by more than one rank. The fee rises with your rank and comes
from the gold you carry: 100 gold at rank 0, 2500 at rank 4, 10000 at rank 9,
and 40000 at rank 19.

You cannot start a contract while fighting, while busy with another activity,
or while another contract is open. Starting one works like QUIT: your belongings
are saved, your followers are dismissed, and timed quests end.

The character then stays out of play for 24 hours of real time. The account
menu shows the time left, and the character cannot enter the game before the
contract ends. The first time you select it afterwards, it receives the
experience and returns beside the trainer.

To end a contract early, type RECALL <number> at the account menu to review it,
then RECALL <number> CONFIRM. The character returns without the experience, and
the fee is not refunded.

See also: CRAFT-SCORE, CRAFTING, HARVEST, QUIT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('apprentice', 'APPRENTICE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('apprentice', 'CRAFT-TRAINER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('apprentice', 'CRAFT-TRAINING');

-- Point the established craft score topic at the trainer, through its keyword.
UPDATE help_entries AS h
JOIN help_keywords AS k ON k.help_tag = h.tag
SET h.entry = 'Usage: craft score

This command will display your crafting and harvesting skills, their rank, experience, and
experience required for the next rank.

See also: APPRENTICE'
WHERE k.keyword = 'CRAFT-SCORE';

COMMIT;
