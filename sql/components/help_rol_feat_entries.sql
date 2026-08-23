-- Help entries for the feats converted from Realms of Luminari skills.
--
-- Covers the shadow, calm, establish camp, garrote and accompany feats.
-- The database help system is authoritative; lib/text/help/help.hlp carries the
-- same text. This migration is safe to run repeatedly.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ACCOMPANY', 'Accompany

Usage: accompany <performer>
       accompany

Requires: the accompany feat (any class; 5 ranks of perform)

Instead of leading a song of your own, you back a grouped performer''s song in
your room. Your perform ability and instrument raise the quality of every verse
the lead performs, up to a cap, and if the lead''s performance falters or
stutters you take the song over rather than letting it end.

Type accompany with no argument to stop accompanying. You must stop your own
performance before you can accompany someone else, and you cannot accompany
while silenced.

See also: PERFORM, INSTRUMENT, FEAT INFO ACCOMPANY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ACCOMPANY', 'ACCOMPANY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CALM', 'Calm

Usage: calm

Requires: the calm feat (any class; charisma 13)
Action:   Standard Action
Uses:     limited per day, at least 1 plus your charisma bonus

You intone a settling chant that tries to end every fight in the room. Each
combatant resists with a will save against 10 + half your level + your charisma
bonus. Those who fail disengage, forget their current quarrel, and are left too
settled to be calmed again for a few rounds. Creatures immune to mind-affecting
effects ignore the chant, and you cannot chant while silenced.

See also: FEAT INFO CALM, PACIFY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CALM', 'CALM');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CALM', 'PACIFY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CAMP', 'Establish Camp

Usage: camp

Requires: the establish camp feat (any class; 3 ranks of survival)
Action:   Standard and Move Action
Check:    survival against a difficulty set by terrain and weather

You clear a site, set your gear and get a fire going. You and any grouped
companions in the room recover hitpoints and movement 50 percent faster while
sleeping, reclining, resting or sitting in the camp, and the campsite becomes
your return point, so quitting from camp brings you back to it.

Camps cannot be pitched indoors, on or under water, while flying, or during
combat. Rough ground such as desert, marsh, mountains and the deep underdark
raises the difficulty, as does rain or a lightning storm.

See also: FEAT INFO ESTABLISH CAMP, SURVEY, HARVEST, WILDERNESS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CAMP', 'CAMP');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CAMP', 'ESTABLISH-CAMP');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('GARROTE', 'Garrote

Usage: garrote <target>

Requires: the garrote feat (any class; 8 ranks of stealth and BAB 4), sneaking
          and hiding (in that order), and at least one free hand
Action:   Standard and Move Action

You loop a cord around the throat of a target that cannot see you. The attack
roll gains a bonus for striking from concealment. On a hit you deal strangling
damage, and unless the target makes a fortitude save against 10 + half your
level + your dexterity bonus it is left choking: silenced and staggered for a
short time.

Creatures that do not breathe, such as undead, constructs, golems and
elementals, cannot be garroted, and neither can anything incorporeal. The target
cannot be larger than you or more than one size category smaller.

See also: FEAT INFO GARROTE, BACKSTAB, SAP, SNEAK, HIDE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GARROTE', 'GARROTE');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GARROTE', 'STRANGLE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHADOW', 'Shadow

Usage: shadow <target>
       shadow

Requires: the shadow feat (any class; 5 ranks of stealth), sneaking

You covertly tail a target from room to room. Taking up the trail is a contested
stealth check against your mark''s perception, and the contest is repeated every
time your mark leaves the room. While the tail holds you move with your mark
without joining a group and without being announced.

Losing a contest, being unable to keep pace, entering combat, stopping sneaking
or being knocked from your feet all end the tail. Type shadow with no argument to
break off.

This is not the Shadowdancer shadow line: shadow jump, shadow walk, one with
shadow and shadow master are separate abilities.

See also: FEAT INFO SHADOW, SNEAK, HIDE, STEALTH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHADOW', 'SHADOW');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHADOW', 'TAIL');

/* Keep the maintained keyword sets exact on repeat runs. */
DELETE FROM help_keywords
WHERE help_tag = 'ACCOMPANY' AND keyword NOT IN ('ACCOMPANY');
DELETE FROM help_keywords
WHERE help_tag = 'CALM' AND keyword NOT IN ('CALM', 'PACIFY');
DELETE FROM help_keywords
WHERE help_tag = 'CAMP' AND keyword NOT IN ('CAMP', 'ESTABLISH-CAMP');
DELETE FROM help_keywords
WHERE help_tag = 'GARROTE' AND keyword NOT IN ('GARROTE', 'STRANGLE');
DELETE FROM help_keywords
WHERE help_tag = 'SHADOW' AND keyword NOT IN ('SHADOW', 'TAIL');

/* Each exact player command has one authoritative help owner. */
DELETE FROM help_keywords
WHERE UPPER(keyword) = 'ACCOMPANY' AND BINARY help_tag <> 'ACCOMPANY';
DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CALM' AND BINARY help_tag <> 'CALM';
DELETE FROM help_keywords
WHERE UPPER(keyword) = 'CAMP' AND BINARY help_tag <> 'CAMP';
DELETE FROM help_keywords
WHERE UPPER(keyword) = 'GARROTE' AND BINARY help_tag <> 'GARROTE';
DELETE FROM help_keywords
WHERE UPPER(keyword) = 'SHADOW' AND BINARY help_tag <> 'SHADOW';
