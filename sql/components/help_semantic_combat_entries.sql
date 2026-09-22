-- Combat phase and initiative player help.
--
-- The database help system is authoritative. This migration is safe to run
-- repeatedly and replaces the combat entries with the current phase rules.
--
-- help.hlp lists every tag among its entry's keywords, so each tag here owns the
-- keyword of its own name and no other tag may claim it: INITIATIVE belongs to
-- initiative, INITIATIVE-ORDER to initiative-order, and READY to ready-action.
-- READIED-ACTION belongs to the READIED-ACTION tag (help_counterspell_readiness.sql).

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('combat', 'The usual way to gain experience in Luminari is via combat.  Combat in Luminari
is multi-layered and is designed for a fast-paced, active experience.

Initiating Combat:
Combat can be initiated in a couple ways.  The most usual way to initate combat
is to use the kill or hit commands : kill <name> or hit <name>.  This will
initiate combat between you and the specified mobile or player.  If
you have a Combat Maneuver (CM) in the attack queue, that CM will be used
instead of a melee attack.
Combat may also be initiated via a CM, trip for example : trip <name>.  This
command will initiate combat with a trip CM.

Combat Rounds:
A full round of attacks takes 6 seconds and is split into three 2-second
phases. Your attacks are spread across those phases, so a combatant with
several attacks swings more than once per round. When a fight starts, the
side with the better initiative gets its first phase 2 seconds later and the
other side 4 seconds later, so the two sides alternate. Joining or merging
fights keeps each combatant''s own timing.

You can check the number of attacks you have in your rotation by using the ATTACKS
command.  The number of attacks available is directly based on your Base Attack
Bonus (BAB), which increases based on your class and level.

Actions in Combat:
Your automatic attacks do not spend your standard or move action. Commands,
combat maneuvers, spells, and readied actions do, for exactly as long as each
one takes, and you get them back at that moment whether or not you are in
combat. Without a standard action you make no automatic attacks; without a
move action you make only the first portion of your full attack rotation.
Your attacks of opportunity refresh every phase. See ''help ACTIONS'' and
''help ACTION-QUEUE''.

Combat Modes:
There are a number of combat modes available that change the way you fight.
Some modes are always available and some are unlocked by feats.  Knowing when to
use the different modes is vital for mastering combat.  see ''help COMBAT-MODES''
for more information.

Attacks of Opportunity:
Sometimes a combatant in a melee lets his guard down or takes a reckless action.
In this case, combatants engaged to this combatant can take advantage of his
lapse in defense to attack him for free. These free attacks are called attacks
of opportunity, and are marked 	W[	RAOO	W]	n next to the attack.  Usually, you
are able to make one Attack of opportunity (AOO) every 6 seconds.  If you have
the Combat Reflexes feat, however, you may make a number of Attacks of
opportunity equal to your dexterity bonus every 6 seconds. See ''help AAO'' for
more information

Combat Maneuvers:
During combat, you can attempt to perform a number of maneuvers that can hinder
or even cripple your foe.  Many of these attacks can be used in place of a
melee attack, allowing you to combine different combat maneuvers together to
stack bonuses on your allies or penalties on your enemies.  Some maneuvers may
trigger an AOO from your opponents.  See ''help COMBAT-MANEUVERS'' for more
information.

See Also: COMBAT-MANEUVERS, ATTACK-QUEUE, ACTION-QUEUE, ACTIONS, COMBAT-MODES, AOO', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE
  UPPER(keyword) IN ('COMBAT', 'COMBAT-PHASE', 'COMBAT-ROUNDS', 'FIGHTING')
  AND help_tag <> 'combat';
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT-PHASE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'COMBAT-ROUNDS');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('combat', 'FIGHTING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('initiative-order', 'INITIATIVE

Usage:
  initiative

Initiative determines who strikes first. It is rolled as 1d20 plus Dexterity
and other bonuses when combat starts and when a combatant joins a fight. The
winner of the opening roll strikes first. A combatant whose roll matches or
beats its opponent''s gets its first attack phase 2 seconds later, otherwise 4.

Use INITIATIVE while fighting to see each visible combatant''s roll, upcoming
attack phase, and the seconds until it. Your own name is highlighted in green
when color is enabled.

See also: COMBAT, ACTIONS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE
  UPPER(keyword) = 'INITIATIVE-ORDER'
  AND help_tag <> 'initiative-order';
DELETE FROM help_keywords
WHERE
  help_tag = 'initiative-order'
  AND UPPER(keyword) = 'INITIATIVE';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('initiative-order', 'INITIATIVE-ORDER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('initiative', 'INITIATIVE

Usage:
  initiative

Initiative determines who strikes first. It is rolled as 1d20 plus Dexterity
and other bonuses when combat starts and when a combatant joins a fight. The
winner of the opening roll strikes first. A combatant whose roll matches or
beats its opponent''s gets its first attack phase 2 seconds later, otherwise 4.

Use INITIATIVE while fighting to see each visible combatant''s roll, upcoming
attack phase, and the seconds until it. Your own name is highlighted in green
when color is enabled.

See also: COMBAT, ACTIONS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE
  UPPER(keyword) = 'INITIATIVE'
  AND help_tag <> 'initiative';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('initiative', 'INITIATIVE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ready-action', 'Usage: ready attack on ally <ally> attacked
       ready counterspell <target> on casting
       ready attack <target> on casting
       ready attack <target> on entry
       ready attack <target> on door open <direction>
       ready <command> on entry [target]
       ready <command> on door open <direction>
       ready
       ready cancel

A readied attack spends your standard action now to prepare one normal melee
strike. HIT and KILL are accepted in place of ATTACK. It expires when your next
combat turn begins, or after six seconds outside semantic combat. Cancelling,
replacing, or losing the opportunity does not refund the action. The strike
costs no second action and does not execute a queued special attack.

ON CASTING watches the named, visible caster in your room. When they begin a
timed spell, your attack is queued before that spell can complete. A hit uses
the normal damage and concentration rules; it does not automatically interrupt
casting. A miss or successful concentration check lets the spell continue.
You need only name the caster, and do not need to identify their spell. Instant
spells cannot trigger this reaction. A cancelled spell cannot redirect a queued
attack to a replacement spell.

COUNTERSPELL reserves the same standard action and expiry for one counter.
You must be an eligible spellcaster with speech and hands available. Watch a
visible local caster before they start a timed spell. You need a perceptible
verbal or somatic component and a Spellcraft check above 20 to identify it.
Deafness prevents hearing verbal components; visible gestures can still reveal
a spell. The relevant senses must remain available when the counter executes.
Failure ends readiness without spending a spell resource. Instant spells and
psionic powers cannot be countered this way.

A successful identification queues the counter. At execution, you must still
see the caster and have the identical base spell available as a preparation,
spontaneous slot or eligible moon bonus. Normal resource-preservation rules
apply once. The counter cancels that exact spell; it does not cast your spell''s
normal effect. An ended or replaced cast consumes no counterspell resource.
Dispel and Improved Counterspell alternatives are not supported. The old
counterspell mode now directs you to this explicit READY command.

ON ALLY <ally> ATTACKED watches a visible group member or one of your NPC
followers in the room. It reserves your standard action for one normal melee
strike against the first eligible attacker you witness. A committed miss or
prevented strike can trigger it; rejected commands cannot. You must still see
the ally and attacker, and the ally relationship must still hold when triggered.

The triggering attack resolves before your queued retaliation. This does not
redirect damage or grant cover. Once triggered, the reaction watches the bound
attacker: their departure, death or extraction cancels it. The ally dying after
the trigger does not erase your already-readied retaliation. Later attacks
cannot retarget it or grant another strike. Normal melee and PvP rules apply.

ON ENTRY waits for the named target to arrive. ON DOOR OPEN watches a visible,
closed door; its attack target must already be visible in your room. Opening
from either side triggers it, including scripted openings. Unlocking, zone
resets and builder edits do not. The door does not reveal a target beyond it.

The attack runs at the next native event boundary,
about one tenth of a second later. You must still be able to act, see and reach
the target, and use a melee weapon or unarmed attack. Normal defenses and PvP
restrictions apply.

Command readiness supports SAY, EMOTE, LOOK, REST, STAND, SIT, OPEN and CLOSE.
These commands run through the normal interpreter and retain their usual costs
and requirements. Other combat commands and aliases cannot be readied. With no
entry filter, a noncombat command triggers on the next arrival.

Moving, dying, logging out, or READY CANCEL removes readiness. A bound attack
target moving, dying or being extracted also cancels it. Replacing or redirecting
a watched exit cancels it. Closing the door after it triggers cancels pending
execution even if it reopens. Only one action can be readied at a time. READY
shows it. Readied actions do not survive copyover or reboot.

Examples:
  ready attack on ally companion attacked
  ready counterspell mage on casting
  ready attack mage on casting
  ready attack guard on entry
  ready say Hold the doorway! on door open north

See also: COMBAT, INITIATIVE, CONCENTRATION, CASTING-TIME', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE
  UPPER(keyword) = 'READY'
  AND help_tag <> 'ready-action';
DELETE FROM help_keywords
WHERE
  help_tag = 'ready-action'
  AND UPPER(keyword) = 'READIED-ACTION';
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('ready-action', 'READY');

COMMIT;
