-- Help entries for the Duris racial innates converted to feats.
--
-- One entry per feat; active feats carry their command verb as an extra
-- keyword where it does not collide with an existing spell entry.
-- The database help system is authoritative; lib/text/help/help.hlp carries
-- the same text. This migration is safe to run repeatedly.
-- See docs/systems/GAME_MECHANICS_SYSTEMS.md

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SUN-VULNERABILITY', 'Sun Vulnerability (racial drawback)

Requires: the sun vulnerability feat

In direct sunlight you regain neither hit points nor movement, and you take
1d8 damage every combat round. Night, cloud, indoors, magical darkness, a
forest or a marsh all shelter you, as does a cloak that covers you
completely.

See also: FEAT INFO SUN VULNERABILITY, DAYBLIND', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SUN-VULNERABILITY', 'SUN-VULNERABILITY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DAYBLIND', 'Dayblind (racial drawback)

Requires: the dayblind feat

Direct sunlight blinds you: while outdoors under a clear daytime sky you
cannot see. Indoors, under magical darkness, or when covered you see
normally, and a creature with no eyes to dazzle (see EYELESS) is never
dayblinded.

See also: FEAT INFO DAYBLIND, SUN-VULNERABILITY, EYELESS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DAYBLIND', 'DAYBLIND');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('MAGIC-VULNERABILITY', 'Magic Vulnerability (racial drawback)

Requires: the magic vulnerability feat

You take 10 percent more damage from any spell or spell-like attack. Weapon
blows are unaffected.

See also: FEAT INFO MAGIC VULNERABILITY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MAGIC-VULNERABILITY', 'MAGIC-VULNERABILITY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('MAGICAL-REDUCTION', 'Magical Reduction

Requires: the magical reduction feat (racial innate)

Raw magic slides off you. You take 20 percent less force and energy damage.
Other damage types are unaffected.

See also: FEAT INFO MAGICAL REDUCTION', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MAGICAL-REDUCTION', 'MAGICAL-REDUCTION');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('THICK-HIDE', 'Thick Hide

Requires: the thick hide feat (racial innate)

Your hide turns aside blows. You take 15 percent less slashing, piercing and
bludgeoning damage. Elemental and magical damage is unaffected.

See also: FEAT INFO THICK HIDE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('THICK-HIDE', 'THICK-HIDE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SACRILEGIOUS-POWER', 'Sacrilegious Power

Requires: the sacrilegious power feat (racial innate)

Your profane nature blunts holy power. You take 25 percent less holy damage
from level 20, 50 percent less from level 25 and 75 percent less from level
30.

See also: FEAT INFO SACRILEGIOUS POWER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SACRILEGIOUS-POWER', 'SACRILEGIOUS-POWER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SPELL-ABSORB', 'Spell Absorb

Requires: the spell absorb feat (racial innate)

When a damaging spell strikes you there is a chance equal to half your
level, in percent, that you absorb the magic and take no damage at all.
Everyone in the room sees it happen.

See also: FEAT INFO SPELL ABSORB', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SPELL-ABSORB', 'SPELL-ABSORB');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('EYELESS', 'Eyeless

Requires: the eyeless feat (racial innate)

You have no eyes to blind. Blindness spells and effects cannot take hold of
you, and you keep perceiving your surroundings while others would be
blinded. This does not let you see in the dark.

See also: FEAT INFO EYELESS, DAYBLIND', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('EYELESS', 'EYELESS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('QUICK-THINKING', 'Quick Thinking

Requires: the quick thinking feat (racial innate)

Your mind recovers from a shock at once. When you fail a will saving throw
there is a 15 percent chance that you roll it again. Fortitude and reflex
saves are unaffected.

See also: FEAT INFO QUICK THINKING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('QUICK-THINKING', 'QUICK-THINKING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('GROUNDFIGHTING', 'Groundfighting

Requires: the groundfighting feat (racial innate)

You fight as well from the ground as on your feet. Being prone, sitting or
resting costs you no armor class and no attack roll penalty. Being stunned
or asleep still does.

See also: FEAT INFO GROUNDFIGHTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GROUNDFIGHTING', 'GROUNDFIGHTING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('QUADRUPED-BODY', 'Quadruped Body

Requires: the quadruped body feat (racial innate)

Four legs are very hard to topple. Bash, trip and every other knockdown
automatically fails against you unless the attacker is larger than you. You
cannot ride a mount; your own legs carry you.

See also: FEAT INFO QUADRUPED BODY, FEAT INFO LEONINE FRAME, WEMIC', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('QUADRUPED-BODY', 'QUADRUPED-BODY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('WATER-BREATHING', 'Water Breathing

Requires: the water breathing feat (racial innate)

You breathe water as easily as air. You never drown underwater and water
that would smother others does not harm you.

See also: FEAT INFO WATER BREATHING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('WATER-BREATHING', 'WATER-BREATHING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('UNDEAD-FEALTY', 'Undead Fealty

Requires: the undead fealty feat (racial innate)

The restless dead recognize something in you. An aggressive undead creature
at least ten levels below you will not attack you unprovoked. Attack it and
it fights back as usual.

See also: FEAT INFO UNDEAD FEALTY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('UNDEAD-FEALTY', 'UNDEAD-FEALTY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('AXE-MASTERY', 'Axe Mastery

Requires: the axe mastery feat (racial innate)

While the weapon you attack with is an axe you gain +1 to attack and damage
rolls for every 8 character levels, to a maximum of +3 at level 24.

See also: FEAT INFO AXE MASTERY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('AXE-MASTERY', 'AXE-MASTERY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('HAMMER-MASTERY', 'Hammer Mastery

Requires: the hammer mastery feat (racial innate)

While the weapon you attack with is a hammer you gain +1 to attack and
damage rolls for every 8 character levels, to a maximum of +3 at level 24.

See also: FEAT INFO HAMMER MASTERY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('HAMMER-MASTERY', 'HAMMER-MASTERY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('LONGSWORD-MASTERY', 'Longsword Mastery

Requires: the longsword mastery feat (racial innate)

While the weapon you attack with is a long sword you gain +1 to attack and
damage rolls for every 8 character levels, to a maximum of +3 at level 24.

See also: FEAT INFO LONGSWORD MASTERY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('LONGSWORD-MASTERY', 'LONGSWORD-MASTERY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('GREATSWORD-MASTERY', 'Greatsword Mastery

Requires: the greatsword mastery feat (racial innate)

While the weapon you attack with is a great sword you gain +1 to attack and
damage rolls for every 8 character levels, to a maximum of +3 at level 24.

See also: FEAT INFO GREATSWORD MASTERY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GREATSWORD-MASTERY', 'GREATSWORD-MASTERY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('HATRED', 'Hatred

Requires: the hatred feat (racial innate)

An old hatred drives your blows. Against evil-aligned opponents you gain +1
to attack rolls and +2 to damage.

See also: FEAT INFO HATRED', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('HATRED', 'HATRED');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('BATTLE-FRENZY', 'Battle Frenzy

Requires: the battle frenzy feat (racial innate)

Each successful melee hit against a humanoid has a 5 percent chance to whip
you into a frenzy, granting one extra attack on the spot.

See also: FEAT INFO BATTLE FRENZY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BATTLE-FRENZY', 'BATTLE-FRENZY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('WARCALLERS-FURY', 'Warcaller''s Fury

Requires: the warcaller''s fury feat (racial innate)

The press of allies drives you on. You gain +1 damage for every member of
your group standing in your room, yourself included, to a maximum of +5.
Alone and ungrouped you gain nothing.

See also: FEAT INFO WARCALLER''S FURY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('WARCALLERS-FURY', 'WARCALLERS-FURY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RRAKKMA', 'Rrakkma

Requires: the rrakkma feat (racial innate)

You fight best beside your own kind. For each other grouped character in
your room who also has this feat, up to five, you gain +1 armor class and +2
on saving throws.

See also: FEAT INFO RRAKKMA', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('RRAKKMA', 'RRAKKMA');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('OUTDOOR-STEALTH', 'Outdoor Stealth

Requires: the outdoor stealth feat (racial innate)

You blend into open country. You gain +6 to stealth checks in any outdoor
sector outside the underdark. Terrain stealth bonuses do not stack with one
another.

See also: FEAT INFO OUTDOOR STEALTH, SWAMP-STEALTH, UNDERDARK-STEALTH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('OUTDOOR-STEALTH', 'OUTDOOR-STEALTH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SWAMP-STEALTH', 'Swamp Stealth

Requires: the swamp stealth feat (racial innate)

You are at home in the mire. You gain +6 to stealth checks in marshland.
Terrain stealth bonuses do not stack with one another.

See also: FEAT INFO SWAMP STEALTH, OUTDOOR-STEALTH, UNDERDARK-STEALTH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SWAMP-STEALTH', 'SWAMP-STEALTH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('UNDERDARK-STEALTH', 'Underdark Stealth

Requires: the underdark stealth feat (racial innate)

The deep places hide you. You gain +6 to stealth checks in any underdark
sector. Terrain stealth bonuses do not stack with one another.

See also: FEAT INFO UNDERDARK STEALTH, OUTDOOR-STEALTH, SWAMP-STEALTH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('UNDERDARK-STEALTH', 'UNDERDARK-STEALTH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('FOREST-SIGHT', 'Forest Sight

Requires: the forest sight feat (racial innate)

Your eyes were made for the woods. You gain +4 to perception checks in
forest sectors.

See also: FEAT INFO FOREST SIGHT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FOREST-SIGHT', 'FOREST-SIGHT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SEADOG', 'Seadog

Requires: the seadog feat (racial innate)

Born to the waves, you coax more out of any vessel you pilot: every move you
steer carries the ship one extra map tile.

See also: FEAT INFO SEADOG, VESSELS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SEADOG', 'SEADOG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('MINER', 'Miner

Requires: the miner feat (racial innate)

You read the rock. When harvesting minerals, stone or crystal you gather as
if your mining skill were 4 higher.

See also: FEAT INFO MINER, HARVEST', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('MINER', 'MINER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('BARTER', 'Barter

Requires: the barter feat (racial innate)

You haggle by instinct. Shopkeepers treat you as if you had 10 more points
of charisma when they set the price you pay and the price they offer you.

See also: FEAT INFO BARTER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BARTER', 'BARTER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CALMING', 'Calming

Requires: the calming feat (racial innate)

Something about you soothes hostile creatures. An aggressive creature within
five levels of you ignores you half the time it would otherwise attack
unprovoked.

See also: FEAT INFO CALMING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CALMING', 'CALMING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-FARSEE', 'Innate Farsee

Usage: farsee
Requires: the innate farsee feat (racial innate)
Uses: 3 per day

You sharpen your sight until it reaches far beyond the horizon, as the
farsee spell, cast at your character level on yourself. This is a spell-like
ability and needs no components. Type help farsee for the spell itself.

See also: FEAT INFO INNATE FARSEE, FARSEE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FARSEE', 'INNATE-FARSEE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-STONESKIN', 'Innate Stoneskin

Usage: stoneskin
Requires: the innate stoneskin feat (racial innate)
Uses: 1 per day

You harden your skin against blows, as the stoneskin spell cast at your
character level on yourself. It is refused while you are already
stoneskinned. Type help stoneskin for the spell itself.

See also: FEAT INFO INNATE STONESKIN, STONESKIN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-STONESKIN', 'INNATE-STONESKIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-LIGHTNING-BOLT', 'Innate Lightning Bolt

Usage: throwlightning [opponent]
Requires: the innate lightning bolt feat (racial innate)
Uses: 3 per day

While fighting, you hurl a lightning bolt at an opponent in the room (your
current opponent if you name none), as the lightning bolt spell cast at your
character level.

See also: FEAT INFO INNATE LIGHTNING BOLT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-LIGHTNING-BOLT', 'INNATE-LIGHTNING-BOLT');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-LIGHTNING-BOLT', 'THROWLIGHTNING');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-FIRE-SHIELD', 'Innate Fire Shield

Usage: fireshield
Requires: the innate fire shield feat (racial innate)
Uses: 1 per day

You wreathe yourself in flame, as the fire shield spell cast at your
character level on yourself. It is refused while you already carry a fire
shield.

See also: FEAT INFO INNATE FIRE SHIELD', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FIRE-SHIELD', 'INNATE-FIRE-SHIELD');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FIRE-SHIELD', 'FIRESHIELD');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-FIRE-STORM', 'Innate Fire Storm

Usage: firestorm
Requires: the innate fire storm feat (racial innate)
Uses: 1 per day

You call a fire storm down on the room, striking everyone who is not with
you, as the fire storm spell cast at your character level.

See also: FEAT INFO INNATE FIRE STORM', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FIRE-STORM', 'INNATE-FIRE-STORM');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FIRE-STORM', 'FIRESTORM');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-SHADOW-JUMP', 'Innate Shadow Jump

Usage: shadowdoor <target>
Requires: the innate shadow jump feat (racial innate)
Uses: 1 per day

You step through shadow to the side of a character anywhere in the world, as
the shadow jump spell cast at your character level. Rooms and planes that
refuse teleportation refuse this too.

See also: FEAT INFO INNATE SHADOW JUMP', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-SHADOW-JUMP', 'INNATE-SHADOW-JUMP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-SHADOW-JUMP', 'SHADOWDOOR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-PLANE-SHIFT', 'Innate Plane Shift

Usage: planeshift <astral | ethereal | elemental | prime>
Requires: the innate plane shift feat (racial innate)
Uses: 1 per day

You cross to the named plane, as the plane shift spell cast at your
character level. Prime is only reachable from another plane. Type help
planeshift for the spell itself.

See also: FEAT INFO INNATE PLANE SHIFT, PLANESHIFT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-PLANE-SHIFT', 'INNATE-PLANE-SHIFT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-PSIONIC-BLAST', 'Innate Psionic Blast

Usage: mindblast [opponent]
Requires: the innate psionic blast feat (racial innate)
Uses: 3 per day

You unleash a stunning psionic blast, as the psionic blast power manifested
at your character level. The power strikes every hostile in the room; you
need an opponent present to use it.

See also: FEAT INFO INNATE PSIONIC BLAST', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-PSIONIC-BLAST', 'INNATE-PSIONIC-BLAST');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-PSIONIC-BLAST', 'MINDBLAST');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-SCARE', 'Innate Scare

Usage: roar [opponent]
Requires: the innate scare feat (racial innate)
Uses: 3 per day

You let out a roar that terrifies one opponent in the room (your current
opponent if you name none), as the scare spell cast at your character level.

See also: FEAT INFO INNATE SCARE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-SCARE', 'INNATE-SCARE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-SCARE', 'ROAR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-HASTE', 'Innate Haste

Usage: battlehaste
Requires: the innate haste feat (racial innate)
Uses: 1 per day

You work yourself into a hasted battle fury, as the haste spell cast at your
character level on yourself. It is refused while you are already hasted.

See also: FEAT INFO INNATE HASTE, HASTE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-HASTE', 'INNATE-HASTE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-HASTE', 'BATTLEHASTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-FIREBALL', 'Innate Fireball

Usage: fireball [opponent]
Requires: the innate fireball feat (racial innate)
Uses: 3 per day

You hurl a ball of flame at one opponent in the room (your current opponent
if you name none), as the fireball spell cast at your character level. Type
help fireball for the spell itself.

See also: FEAT INFO INNATE FIREBALL, FIREBALL', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FIREBALL', 'INNATE-FIREBALL');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-MASS-DISPEL', 'Innate Mass Dispel

Usage: massdispel
Requires: the innate mass dispel feat (racial innate)
Uses: 1 per day

You strip magic from everyone else in the room, friend and foe alike, as the
dispel magic spell cast at your character level on each of them.

See also: FEAT INFO INNATE MASS DISPEL', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-MASS-DISPEL', 'INNATE-MASS-DISPEL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-MASS-DISPEL', 'MASSDISPEL');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-FROST-BREATH', 'Innate Frost Breath

Usage: frostbreath [opponent]
Requires: the innate frost breath feat (racial innate)
Uses: 3 per day

You breathe a cone of cold at one opponent in the room (your current
opponent if you name none), as the cone of cold spell cast at your character
level.

See also: FEAT INFO INNATE FROST BREATH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FROST-BREATH', 'INNATE-FROST-BREATH');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-FROST-BREATH', 'FROSTBREATH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('INNATE-WEB', 'Innate Web

Usage: webwrap [opponent]
Requires: the innate web feat (racial innate)
Uses: 3 per day

You bind one opponent in the room in webbing, as the web spell cast at your
character level. The target may be at most one size larger than you.

See also: FEAT INFO INNATE WEB', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-WEB', 'INNATE-WEB');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('INNATE-WEB', 'WEBWRAP');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('BODYSLAM', 'Bodyslam

Usage: bodyslam <victim>
Requires: the bodyslam feat (Half-Troll racial innate)

You hurl your whole body at an opponent to start a fight, much like bash. A
successful bodyslam sends the victim sprawling; a failed one leaves you
stunned for several rounds. It costs a standard and a move action.

See also: FEAT INFO BODYSLAM, BASH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BODYSLAM', 'BODYSLAM');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DOORBASH', 'Doorbash

Usage: doorbash <direction>
Requires: the doorbash feat (racial innate)

You throw yourself at a closed door. Roll a d300 at or below your strength
plus your level and the door shatters open on both sides with its lock
broken; fail and it holds, and you take 1d6 damage. Pickproof doors cannot
be forced.

See also: FEAT INFO DOORBASH, OPEN, UNLOCK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('DOORBASH', 'DOORBASH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('STAMPEDE', 'Stampede

Usage: stampede
Requires: the stampede feat (racial innate)

You lower your head and trample every opponent fighting you, and the one
you are fighting. Each one you overrun (as a bash) is knocked down and
struck with an unarmed blow. Flying opponents cannot be trampled, and the
charge only starts with someone on the ground to run over. Usable once
every three rounds, never in a single-file room, and only while fighting.

See also: FEAT INFO STAMPEDE, BASH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('STAMPEDE', 'STAMPEDE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACIAL-FLURRY', 'Racial Flurry

Usage: onslaught
Requires: the racial flurry feat (racial innate)
Uses: 1 per day

You explode into a flurry of blows: one extra attack every round for four
rounds. It does not stack with haste and is refused while you are hasted or
already in a flurry. This is not the monk''s flurry of blows.

See also: FEAT INFO RACIAL FLURRY, HASTE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('RACIAL-FLURRY', 'RACIAL-FLURRY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('RACIAL-FLURRY', 'ONSLAUGHT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SUMMON-WARG', 'Summon Warg

Usage: summonwarg
Requires: the summon warg feat (racial innate)
Uses: 1 per day

Outdoors, you call a warg out of the wilds. It follows you as a charmed
companion at two thirds of your level and can be ridden (see MOUNT).

See also: FEAT INFO SUMMON WARG, MOUNT, PETS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SUMMON-WARG', 'SUMMON-WARG');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SUMMON-WARG', 'SUMMONWARG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SUMMON-HORDE', 'Summon Horde

Usage: summonhorde
Requires: the summon horde feat (racial innate)
Uses: 1 per day

You call two to four orc warriors to your side. They follow you as charmed
companions at half your level and drift away after about fifteen minutes.

See also: FEAT INFO SUMMON HORDE, PETS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SUMMON-HORDE', 'SUMMON-HORDE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SUMMON-HORDE', 'SUMMONHORDE');
