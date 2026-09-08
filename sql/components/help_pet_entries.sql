-- Native pet command help. Apply to the development help database with the
-- matching lib/text/help/help.hlp update; repeated application is safe.
START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ORDER', 'Usage: order <character> <command>
       order followers <command>

Order a loyal charmed NPC in your room, or use FOLLOWERS for all eligible pets
there. Use a numbered keyword, such as 2.companion, to select among matching names.
An assigned pet ID from PETS also works: order #123 stand.

An accepted order uses one swift action, including an order to the whole group.
Wait for that action to recover before ordering again. Each pet still obeys the
normal action costs, ability restrictions, and cooldowns of its command.

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

Animation allows one undead, or two for a necromancer. Epic summons such as
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
The keeper holds up to 10 followers for you.

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
COMMIT;
