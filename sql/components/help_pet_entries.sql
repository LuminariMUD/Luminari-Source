-- Native pet command help. Apply to the development help database with the
-- matching lib/text/help/help.hlp update; repeated application is safe.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ORDER', 'Usage: order <character> <command>
       order followers <command>

Order a loyal charmed NPC in your room, or use FOLLOWERS for all eligible pets
there. Use a numbered keyword, such as 2.companion, to select among matching names.
An assigned pet ID from PETS also works: order #123 stand.
A named or ID-selected pet must be visible to you. Invisible pets may require
detect invisibility; group orders still reach eligible pets in your room.

An accepted order uses one swift action, including an order to the whole group.
Wait for that action to recover before ordering again. Each pet still obeys the
normal action costs, ability restrictions, and cooldowns of its command.

Pets with known spells can cast them when ordered, for example:
  order healer cast ''cure critic'' Yourname
Each known spell has two slots. Casting uses one slot and the pet''s standard
action. One spent slot recovers each minute out of combat. An invalid target
does not spend a spell slot. Eidolon at-will abilities retain their own rules.

Pets that leave, lose your control, become incapacitated, or are removed before
their turn in a group order are skipped. Orders do not continue remotely if you
leave the room. Someone else''s follower and a non-charmed follower cannot be
ordered. A charmed character cannot issue orders of its own.

Examples:

  > order puppy eat bread
  > order 2.companion sleep
  > order followers stand

See also: CHARM, CHARMEE, DISMISS, SWIFT-ACTION', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ORDER', 'ORDER');

-- Update established topics through their keywords, preserving their tags and aliases.
INSERT IGNORE INTO help_keywords (help_tag, keyword)
SELECT h.tag, aliases.keyword
FROM help_entries AS h
JOIN (SELECT 'CHARMEE' AS keyword UNION ALL SELECT 'CHARMEES'
      UNION ALL SELECT 'CHARMIE' UNION ALL SELECT 'CHARMIES'
      UNION ALL SELECT 'FOLLOWERS') AS aliases
WHERE h.tag = 'charmee'
  AND NOT EXISTS (SELECT 1 FROM help_keywords AS k WHERE k.keyword = aliases.keyword);

UPDATE help_entries AS h
JOIN help_keywords AS k ON k.help_tag = h.tag
SET h.entry = 'Charmee is a term coined for mobile (non-player) characters that are
either charmed, or otherwise controlled by PC (player characters).

Use ''pets'' to see your controlled NPCs, their locations and categories, and
your available general follower slots. Ordinary social followers are separate.

General followers have one base slot plus your positive Charisma modifier.
The first ordinary summon has its own slot; additional ordinary summons also
use general slots. Companion bonds, mercenaries, golems, genies, and special
summons use their own limits. An animal companion is not also an ordinary
summon just because it shares a creature type with one.

Animation uses 2 control points, or 4 for a necromancer. Lesser undead cost 1
point and elite undead cost 2; see ANIMATE-DEAD for form costs. Epic summons such as
Mummy Dust have separate allowances. An authored multi-creature spell can
produce its whole group; this does not grant unlimited repeated casts.

Persistence and duration depend on how a follower was acquired. Do not assume
every controlled creature survives death, dismissal, or expiry.

You can recall all of your charmies to your current room by typing ''summon''.

See Also: PETS, ORDER, DISMISS, SUMMON-CHARMIES', h.min_level = 0, h.auto_generated = FALSE
WHERE k.keyword = 'CHARMEE';

UPDATE help_entries AS h
JOIN help_keywords AS k ON k.help_tag = h.tag
SET h.entry = '   A few stores within the realm will allow you to buy pets. These pets are
your obedient servants. As such you will be able to order them to do as you
wish.

Similarly you can find henchmen and mercenaries that can be hired by gold or tasks.

Limits depend on follower type. Use ''pets'' to see current categories and
capacity; see CHARMEE for control rules.

Usage: pets <pet|followers> <follow|wait|passive|assist|guard>
Use a numbered name such as 2.wolf to select one of several matching pets.
PETS shows assigned stable IDs as #123. Use that ID instead of a name in
pets #123 wait, order #123 stand, dismiss #123, or stable store #123.
The ID stays the same through saving and keeper storage. Pets without an
assigned ID can still be selected by name. IDs do not bypass room or control
restrictions. Stable reclaim continues to accept its listed number or raw ID.
Only loyal pets in your room can accept a behavior change.
Follow keeps normal following and assistance. Wait stops automatic following
and assistance. Passive follows without automatically assisting or rescuing.
Assist follows and automatically assists only you. Guard follows and attempts
to protect you using normal rescue rules and your charmie-rescue preference.
Changing behavior does not stop an existing fight or prevent direct orders.
Behavior is saved with eligible pets. Explicit summon recalls waiting pets;
automatic travel does not.

PETS also shows each pet''s persistence policy. Durable pets (companions,
familiars, mounts, eidolons, mercenaries, golems, animated dead, and other
kept followers) stay saved until dismissed, killed, or stored. Timed control,
such as a charm whose control has a duration, is saved with its remaining time
and that time pauses while you are offline. Timed summons such as an illusory
decoy keep a real-time deadline: it keeps counting while you are offline,
across reboots and copyovers, and a pet whose deadline has passed is gone
when you return. Ordinary spell summons, such as summoned creatures, allies,
and genies, last only for your current session: they are never saved and do
not return after you log out, a reboot, or a copyover.

Use pets <pet|#id> name <name> to name one loyal pet in your room. Names are
3-24 ASCII letters, with optional internal apostrophes or hyphens; spaces,
color codes, parser words such as the, with, all, self, or someone, and the
reserved names followers and restore are not accepted. Example: pets #123 name Rowan. The original creature keywords still
work, and repeated naming replaces the previous custom name. The name survives
saving and keeper storage; a failed save leaves the previous name unchanged.
Clones keep their owner-derived identity. Eidolons retain their saved name.

Companion call cooldowns, Mummy Dust, and Dragon Knight keep their remaining
recharge time through a copyover.
If current pets cannot be saved, copyover is cancelled and they stay in play
so the save can be retried. This also protects pets of linkdead owners.

A failed companion creation does not start its call cooldown. Golem completion
retains materials if creation is unavailable or your control limit is reached.
A failed crafting roll still consumes the selected materials and motes.
Item-summon charges and innate animation uses are spent after placement.
Failed totem prayers count as attempts; unavailable spirit creation does not.
A separated retainer must be recalled with summon before calling another.
Horn of Henekar recruitment uses each creature''s control category. Failed
recruitment or Oaken Defender creation leaves that power''s recharge ready.

Use ''pets restore'' after a login where some saved pets could not be
restored; the saved records are kept and pets already with you are not
duplicated.

If you get separated from your NPC followers, use the ''summon'' command to
bring them back to you.

There is a place to hire some henchmen in Ashenport''s Jade Jug Inn.', h.min_level = 0, h.auto_generated = FALSE
WHERE k.keyword = 'PETS';

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DISMISS', 'Usage: dismiss <pet>
       dismiss

Dismiss a controlled pet in your room. Numbered keywords, such as
2.companion, select among matching names. An assigned ID from PETS also works:
dismiss #123. With no argument, dismiss selects
your non-present controlled pets. Ordinary social followers are released
without being destroyed.

Retrieve carried and worn gear first. If any selected pet still holds gear,
nothing is dismissed. Dismissal must also save successfully before pets are
removed; a save failure leaves them with you so you can try again later.

Dismissing a bonded companion, familiar, mount, shadow, or eidolon reduces
its remaining call cooldown to at most 59 seconds. You cannot dismiss pets
while charmed or dismiss someone else''s followers.

See also: ORDER, CHARMEE, CALL-COMPANION, PETS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DISMISS', 'DISMISS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('STABLE', 'Usage: stable
       stable store <follower>
       stable reclaim <number>

A pet keeper holds your loyal followers out of play until you want them back.
Use STABLE alone to list what the keeper holds for you, with the number used to
reclaim each one.

STABLE STORE hands one charmed follower in your room to the keeper. You may
use its name or its assigned ID from PETS, such as stable store #123. You cannot
stable a follower while either of you is fighting, or while it is being ridden.
The keeper boards only followers that stay with you: durable pets and timed
control. Timed summons and ordinary spell summons are refused. A stabled
follower whose time ran out before the policy existed is released when you try
to reclaim it, freeing its slot. The keeper holds up to 10 followers for you.

STABLE RECLAIM returns a stabled follower by its listed number - the small
number shown beside it, such as ''stable reclaim 1''. The longer stable ID in the
listing also works. A reclaimed follower returns with the same
identity, statistics, effects, and equipment it had when stored. Reclaiming
obeys your ordinary follower limits, and only the character who stored a
follower can reclaim it. Renaming your character does not change that.

A stabled follower is kept even when your other followers are saved. Reclaim
prepares its equipment and saves its return before bringing it into the room.
If preparation or that save fails, the keeper retains the stored follower.

See also: PETS, ORDER, DISMISS, CHARMEE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
-- The stable command owns this keyword; the hired-pet article keeps its others.
DELETE FROM help_keywords WHERE keyword = 'STABLE' AND help_tag <> 'STABLE';
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('STABLE', 'STABLE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('STABLE', 'STABLES');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('STABLE', 'STABLE-MASTER');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('STABLE', 'PET-KEEPER');
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SERVANT', 'Usage: servant
       servant get <item>
       servant put <item> <container>

While the unseen servant spell attends you, the servant handles items on your
behalf. It is a utility conjuration: it never fights, and it refuses to work
while you are fighting.

SERVANT alone reports how much more weight the servant can bear for you. That
allowance is the same one the spell adds to your carrying capacity.

SERVANT GET has the servant fetch an item from the room, even when your hands
are already full of other items. The item''s weight must still fit within your
extended carrying capacity.

SERVANT PUT has the servant stow one item you carry inside a container you carry
or one that is in the room.

The servant fades when the spell ends. Anything it fetched stays with you.

See also: UNSEEN SERVANT, PETS, CHARMEE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SERVANT', 'SERVANT');
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('golem-maintenance', 'GOLEM MAINTENANCE

Usage: golemrepair <golem>
       destroygolem <golem>

Material recipes create dedicated constructs in small, medium, large, and huge
sizes. Wood favors accurate attacks and agility; stone gains armor, durability,
and damage reduction 5; iron gains heavier melee damage and damage reduction 8.
They retain native construct defenses. Each uses the ordinary one-golem allowance,
has no natural expiry, and supports ordinary equipment, ORDER, and DISMISS.
Acquisition still requires the recipe''s class feat, materials, motes, crafting
time, and Arcana check. No additional daily cooldown is added.

These commands require the motes crafting system to be enabled.
Repair requires your controlled golem to be alive, present, and out of combat.
You must also be out of combat. A successful Arcana check restores all missing
hit points; a failed check still consumes the repair materials.

Each started 10 percent of missing health costs 10 percent of the recipe''s
primary material requirement. Even one lost hit point requires materials.
Wood repairs require enough units of a single wood type. Full-health golems
need no repair. Golems pending destruction cannot be repaired or dismantled.

Destroygolem recovers half of the original recipe materials and removes the
golem. Repeating the command cannot recover materials from the same golem.
Retrieve equipment before dismantling. To release it without material recovery,
use DISMISS instead.

Crafting completion retains materials and motes if the required creature is
unavailable or your golem allowance is full. An otherwise valid failed ritual
consumes its authored materials and motes. Crafted golem corpses cannot be
animated, including after control is lost.

See also: CRAFT, PETS, DISMISS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('GOLEM-MAINTENANCE', 'GOLEMREPAIR', 'DESTROYGOLEM');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('golem-maintenance', 'GOLEM-MAINTENANCE'),
  ('golem-maintenance', 'GOLEMREPAIR'),
  ('golem-maintenance', 'DESTROYGOLEM');
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('bone-golem', 'BONE GOLEM

Usage: craft create golem animate <corpse>

Bind an eligible room corpse into a medium bone construct. This requires the
motes crafting system and either Construct Wood Golem or Summon Greater Undead
(Necromancer level 9). It uses the ordinary one-golem allowance, shared with
wood, stone, and iron constructs.

The ritual costs 40 bone and 6 of each of the eight mote types. It takes a
standard action and a move action, with an Arcana check against DC 25.
A failed check spends those reagents and actions but leaves the corpse and its
contents intact. Successful binding consumes the corpse exactly once and moves
its contents to the golem. Missing prototypes, full capacity, missing resources,
and ineligible corpses spend nothing.

Player corpses and corpses marked as summoned or artificial minions cannot be
used. The corpse must be present in the room. You must be able to act, free of
charm, out of combat, and not casting or performing another activity. Holy and
antimagic rooms prevent the ritual. There is no additional daily cooldown.

The bone golem is an agile melee construct with damage reduction 3 and native
construct defenses. It is permanent until death or removal and cannot become
another animation source. Use PETS, ORDER, and DISMISS normally. It can carry
and wear equipment; retrieve all gear before dismissal or dismantling.

GOLEMREPAIR uses bone, charging 4 units per started 10 percent of missing HP.
DESTROYGOLEM recovers 20 bone once, with no corpse or mote refund. Repair and
dismantling use the same control and lifecycle rules as material golems.

See also: GOLEM-MAINTENANCE, PETS, ANIMATE-DEAD, CLASS-NECROMANCER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('bone-golem', 'BONE-GOLEM');
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('summon-creature-vii', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''summon creature vii''  	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W Conjuration 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell conjures a loyal elemental of a chosen element (random if no choice is supplied).
	n
See the CHARMEE help file for important info.
	n
	YSee also:	n SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''summon creature vii'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('summon-creature-viii', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''summon creature viii'' 	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W Conjuration 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell will summon a greater elemental to assist you.
	n
Note:  See CHARMEE help file for important info.
	n
	YSee also:	n SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''summon creature viii'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('summon-creature-ix', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''summon creature ix'' 	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W Conjuration 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell will summon an elder elemental to assist you.
	n
Note:  See CHARMEE help file for important info.
	n
	YSee also:	n SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''summon creature ix'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('natures-ally-vii', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''natures ally vii''  	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W None 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell will summon an ally to assist you.
	n
Note:  See CHARMEE help file for important info.
	n
See also: SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''natures ally vii'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('natures-ally-viii', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''natures ally viii''  	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W None 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell will summon an ally to assist you.
	n
Note:  See CHARMEE help file for important info.
	n
See also: SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''natures ally viii'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('natures-ally-ix', '	D-----------------------------------------------------------------	n
	D>Usage:           	W cast ''natures ally ix''  	n
	D>Accumulative:    	W N/A 	n
	D>Duration:        	W Permanent 	n
	D>School of Magic: 	W None 	n
	D>Target(s):       	W N/A 	n
	D>Magic Resist:    	W N/A 	n
	D>Saving Throw:    	W N/A 	n
	D>Damage Type:     	W N/A 	n
	D>Description:	n
	n
This spell will summon an ally to assist you.
	n
Note:  See CHARMEE help file for important info.
	n
See also: SPELLS CHARMEE
	n

Choose air, earth, fire, or water: cast ''natures ally ix'' water.
Omit the choice for a random element. See SUMMON-CHOICES for details.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('elemental-swarm', 'Usage: cast ''elemental swarm'' [air|earth|fire|water]

Druids gain this ninth-circle Conjuration spell at level 17. It uses normal
preparation and casting actions, with no separate material or daily-use cost.

The spell summons 2d4 elementals of the chosen kind, up to eight in one cast.
Omitting the choice selects a random kind. Their level follows caster level,
capped at 20. Only one elemental or elemental-swarm batch can be controlled
at a time; every member must be released before another batch is admitted.

A missing chosen prototype or full allowance rejects casting before spending
the prepared spell. Admission is checked again at completion; normal costs
apply to interruption or changes after casting starts.

The elementals follow you and accept orders, including group combat orders.
They have no timed summon expiry and remain until death or removal. Standard
NPC carrying and equipment checks apply. Recover their gear before dismissal.
Use PETS to find their IDs and dismiss #<id> for each member. Named and ID
commands require visibility; air elementals can require detect invisibility.

See also: SUMMON-CHOICES, PETS, ORDER, DISMISS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('summon-choices', 'SUMMON CHOICES

Optional choices:
  cast ''summon creature vii'' air
  cast ''natures ally vii'' earth
  cast ''elemental swarm'' water
  cast ''geniekind'' marid

Summon Creature VII, VIII, IX and Natures Ally VII, VIII, IX accept air, earth,
fire, or water. Elemental Swarm accepts the same choices and creates 2d4 of the
selected elemental, up to eight in one cast. Shambler retains its separate
three-to-six plant batch. Repeated casts cannot bypass the control limits.

Geniekind accepts djinni, efreeti, marid, or shaitan. Marid supplies healing and
water utility, djinni wind/lightning, efreeti fire offense, and shaitan stone
protection. See GENIEKIND for the caster benefits. An unavailable chosen genie
or a full genie allowance rejects the cast before spending its spell. Admission
is checked again at completion; rejection preserves existing genie benefits.

Omitting a choice retains the random selection. Unknown choices are rejected
before casting begins. Your choice belongs to your cast and is discarded when
it ends or is cancelled; another caster cannot change it.

Native sources: Wizards and Clerics receive Summon Creature VII/VIII/IX at
levels 13/15/17; Sorcerers at 14/16/18. Druids receive Natures Ally VII/VIII/IX
at 13/15/17, and Elemental Swarm and Shambler at 17. Summoners with Summon
Monster can invoke Summon Creature VII/VIII/IX at will from levels 13/15/17.
Other class or domain routes follow their normal spell lists.

You may control one elemental or one elemental-swarm batch at a time, plus a
separate shambler batch. Release every member of a batch before calling another.
A missing chosen prototype or full allowance rejects casting before a prepared
spell is spent. Once casting begins, normal interruption and failure costs
apply; admission is checked again at completion if the world has changed.
The high Summon Creature and Natures Ally spells retain their normal summon
failure roll. Choices add no separate material payment or daily cooldown.

These elemental and plant pets follow the caster, accept ordinary orders, and
have no timed summon expiry. Death and dismissal remove them. They use standard
NPC carrying and equipment rules; recover their gear before dismissal. Air
elementals can be invisible: a named or ID-selected pet must be visible to you
for orders or dismissal. Detect invisibility can provide the needed sight.

Choices use the spell''s normal class access, preparation or at-will rules,
action costs, failure chance, pet lifetime, and equipment rules. They do not
add a daily cooldown or a separate control allowance. Use PETS to inspect the
roster, ORDER to command a controlled pet, and DISMISS to release it after
retrieving its equipment.

See also: GENIEKIND, ELEMENTAL-SWARM, SHAMBLER, CHARMEE, PETS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('summon-choices', 'SUMMON-CHOICES');
INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('planar-ally', 'PLANAR ALLY

Usage: cast ''planar ally'' [guardian|healer]

Available to Clerics at level 11 and Summoners at level 16 through their normal
spell access and preparation rules. It uses a prepared spell and normal casting
time/actions. There is no extra item payment or daily-use cooldown.

Guardian (default): a flying celestial melee ally with strong armor, heavier
attacks, and damage reduction 5.
Healer: a flying celestial support ally with cure critic, bless, and remove
paralysis. Each known spell has two slots; the native mobile recovery system
restores one random missing slot per minute while out of combat.

Both are level-15 outsiders with native angel defenses. They share a separate
one-planar-ally allowance: choosing another role does not grant another slot.
Use PETS to inspect the roster. An unavailable prototype or full allowance
rejects casting before spending the prepared spell. Once casting begins, normal
interruption and spell-consumption rules apply, including later world changes.

The ally is permanent until death or removal. It is owned, follows the caster,
and supports ordinary ORDER commands, equipment, and DISMISS. Retrieve carried
and worn equipment before dismissal. Summoned provenance prevents its corpse
from being used for another animation. Usual pet PvP protections apply.

Examples:
  cast ''planar ally'' guardian
  cast ''planar ally'' healer
  order healer cast ''cure critic'' <name>

See also: PETS, ORDER, DISMISS, SUMMON-CHOICES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('planar-ally', 'PLANAR-ALLY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('mislead', 'Usage: cast ''mislead''
School of Magic: Illusion
Target(s): Self only

Mislead conceals the caster from pursuit and ordinary tracking while its
shadow effect lasts. It also creates one illusory double for two minutes of
live game time, using the spell''s normal class access and spell/action costs.
Missing prototypes or an existing double reject a new cast before spending
its spell. There is no separate daily use or dismissal refund.

The double follows and assists its owner automatically. It is owned but
cannot accept ORDER commands or behavior changes. Its attacks deal illusion
damage, retain normal pet owner credit, and obey player-combat protections.
CHARMIECOMBATROLL supplies optional owner-visible combat details.

An illusory double cannot get, receive, or wear physical equipment. Use PETS
to inspect it and DISMISS decoy to release it. It has a separate one-double
allowance. Its live expiry does not depend on the caster''s shadow effect.
Death leaves no reusable body; ordinary pet cleanup handles dismissal.

See also: ILLUSION, PASS-WITHOUT-TRACE, TRACK, PETS, DISMISS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('mount', 'Usage: mount <creature>

Mounting uses the Ride ability. Mortal riders need a mountable NPC at least
one size larger than themselves. Both rider and mount must have physical
substance, and the mount must be awake and able to carry a rider. You cannot
mount a creature controlled by another owner or an already occupied mount.

Ride skill determines whether you can mount and remain on an untamed creature.
A failed riding check or successful mounting spends a move action. Rejected
size, control, or physical-form checks do not spend that action.

Riding does not make the creature your pet and does not grant ORDER rights.
Use DISMOUNT to get off. Separation clears the riding relationship; it does
not transfer pet ownership. Recall spells dismount you before teleporting;
your mount stays behind and retains its owner. Ordinary mounted travel moves
both creatures together. TAME reduces the risk of being bucked off.

Mounted Combat permits one Ride check per round to block an attack against
the rider. Legendary Rider adds one attempt. Attempts reset each round rather
than accumulating, and a separated mount cannot supply mounted defenses.

See also: RIDE, TAME, BUCK, DISMOUNT, CHARMEE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('dragon-mount', 'Usage: call dragon

Dragon Riders gain Dragon Bond at class level 1. Select your mount type with
STUDY, then CALL DRAGON. All ten standard dragon types are available: black,
blue, green, red, white, brass, bronze, copper, silver, and gold. Use MOUNT to
ride it and DISMOUNT to get off.

You may control one bonded dragon mount. A fresh call costs no money or
materials and creates a mount at your overall level, capped at 25. It starts
a four-MUD-day call cooldown (two real hours). A missing prototype does not
start that cooldown. The bond has no timed expiry.

CALL recalls your existing dragon without changing its stats or restoring
resources. Retrieve its carried and worn gear before DISMISS; dismissal
reduces the remaining call cooldown to at most 59 seconds.

Dragon Rider mounted bond bonuses require your own controlled dragon mount,
with you actually riding it in the same room. A different rider, a broken
control bond, separation, or a dying mount does not supply those bonuses.
United We Stand applies its native bonuses to both rider and dragon while
that riding bond is valid. Riding a different creature does not create a bond.

See also: STUDY, MOUNT, DISMOUNT, PETS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('call', 'Usage: call <companion|familiar|mount|dragon|shadow|eidolon|cohort>

CALL uses your class abilities to summon a bonded follower. Select a form with
STUDY when required. New summons use the ability''s normal level rules and
cooldown; unavailable forms do not create a replacement.

If your existing bonded pet is elsewhere, CALL brings that same pet to you.
It retains its level, statistics, injuries, movement, PSP, identity, equipment,
and existing cooldown. Repeating CALL does not reroll it or refill resources,
even if your call level or bonuses have changed. Calling a mounted pet first
clears its riding relationship; it does not bring another rider along.

CALL will not relocate a creature owned by somebody else. An ordinary respec
requires you to dismiss your followers first; a rejected respec leaves their
state intact. Retrieve equipment before dismissing a pet.

See also: STUDY, PETS, DISMISS, DRAGON-MOUNT, MOUNT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('call', 'CALL');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CALL-LYCANTHROPE', 'Usage: hold mooncall; use mooncall self
Source: mooncall wand, Training Halls Shop
School of Magic: Conjuration

Call Lycanthrope remains content-only: it is not granted to a class or domain.
The Training Halls Shop sells one-charge mooncall wands at a base price of
5,000 gold, before shop modifiers. Hold the wand and USE MOONCALL SELF.
The native DC 20 Use Magic Device check supplies access without a class grant.
A failed activation check leaves the charge intact.

A successful call spends one charge and opens a black door, bringing through
a werewolf or weretiger. Its level is your character level minus 10, bounded
from 1 to 40. Its hit points scale with that level and Constitution. You may
control one called lycanthrope; it has its own control category. Missing
prototypes or an occupied lycanthrope slot leave the wand charge intact.
There is no separate call cooldown. A new call uses another charge, or three
Arcane Apotheosis points through the normal wand-powering ability.

The creature follows and accepts orders while charmed. After 30 seconds, an
idle lycanthrope leaves. A fighting lycanthrope checks your Charisma every
30 seconds: success maintains control, while failure breaks the charm and
turns it against you. This is a risky charm, not a loyal class companion.

It can carry and wear physical equipment subject to its anatomy. Retrieve
carried and worn gear before DISMISS. Expiry and death use ordinary pet gear
handling; do not depend on a volatile summon to safeguard your possessions.

See also: USE, PETS, ORDER, DISMISS, CHARM', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), auto_generated = FALSE;
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('CALL-LYCANTHROPE', 'MOONCALL');

-- Verified native genie sources, abilities, and separate pet/caster lifetimes.
UPDATE help_entries SET entry = 'Usage: cast ''geniekind'' [djinni|efreeti|marid|shaitan]

Available to Wizards, Clerics, and Druids at level 9, Sorcerers at level 10,
and Summoners at level 13 through their normal spell access. Casting uses a
prepared spell or spontaneous slot and normal casting time/actions. There is
no separate gold, material, or daily-use cost. Omitting a choice selects one
randomly; unknown choices are rejected before casting.

The four choices provide distinct allies and inherent caster benefits:

Djinni: lightning bolt and wind wall; resistance 20 to air and electricity,
+2 Dexterity, and flight.
Efreeti: fireball; resistance 20 to fire, +2 Intelligence, and added fire
damage on melee attacks.
Marid: cure critic, water breathe, and water walk; resistance 20 to water and
cold, +2 Constitution, water breathing, and walking on water.
Shaitan: stone skin; resistance 20 to earth and acid, and +2 Strength.

Only one genie can be controlled at a time, using its separate genie allowance.
A full allowance or unavailable chosen genie rejects the cast before spending
the spell. Existing genie benefits are preserved when admission fails.

Genies follow and obey ordinary ORDER commands, use equipment allowed by their
body, and remain until death or removal; they have no timed summon expiry.
Retrieve their carried and worn equipment before using DISMISS.
Their known spells have two slots each, recovering one random spent slot per
minute out of combat. Ordered casting spends a slot and the pet''s standard
action. The owner also spends the normal swift action for ORDER.

Caster benefits have a base duration of 16,000 rounds (26 hours 40 minutes),
subject to applicable spell-duration modifiers. They are separate from the
pet''s lifetime: dismissing the genie does not remove them. A successful new
geniekind cast replaces the previous kind''s benefits.

Examples:
  cast ''geniekind'' marid
  order marid cast ''cure critic'' Yourname
  order shaitan cast ''stone skin'' Yourname

See also: SUMMON-CHOICES, PETS, ORDER, DISMISS', auto_generated = FALSE
WHERE tag = 'geniekind';

UPDATE help_entries SET entry = 'Usage: cast ''shambler''

Druids gain this ninth-circle Conjuration spell at level 17. It uses normal
preparation and casting actions, with no corpse, separate material payment,
or daily-use cooldown.

One cast creates 1d4+2 shambling mounds: three to six plant melee allies.
Their level follows caster level, capped at 20. The whole group uses a separate
shambler allowance. Every member must be released before another batch is
admitted; this allowance is separate from elemental and genie limits.

A missing prototype or existing shambler group rejects casting before spending
the prepared spell. Admission is checked again at completion; normal costs
apply to interruption or changes after casting starts.

Shamblers follow and obey ordinary orders, including order followers kill
<target>. They have no timed summon expiry and remain until death or removal.
Standard NPC carrying and equipment checks apply. Recover their gear before
dismissal. Use PETS and dismiss #<id> to release each member of a batch.

The psionic ectoplasmic shambler is a different power and creature.

See also: SUMMON-CHOICES, PETS, ORDER, DISMISS', auto_generated = FALSE
WHERE tag = 'shambler';

COMMIT;
