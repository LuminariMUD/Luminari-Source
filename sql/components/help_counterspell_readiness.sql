-- Counterspell and ally-defense native ready-action help.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('READIED-ACTION', 'Usage: ready attack on ally <ally> attacked
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

See also: COMBAT, INITIATIVE, CONCENTRATION, CASTING-TIME
', 0, 0) ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('READIED-ACTION', 'READIED-ACTION');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('READIED-ACTION', 'READY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('READIED-ACTION', 'COUNTERSPELL');

COMMIT;
