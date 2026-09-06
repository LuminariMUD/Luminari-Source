-- Nature is the canonical name for persisted skill slot 29.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('CAMP', 'Establish Camp

Usage: camp

Requires: the establish camp feat (any class; 3 ranks of nature)
Action:   Standard and Move Action
Check:    nature against a difficulty set by terrain and weather

You spend about six seconds clearing a site, setting your gear and getting a
fire going. Use activity to inspect, pause, resume or cancel the work. Moving
ends the attempt, damage delays it, and entering combat pauses it until the
fight ends. The camp remains in the room for a time. Anyone there recovers
hitpoints and movement 50 percent faster while sleeping, reclining, resting
or sitting. The campsite becomes the return point for you and grouped
companions present when it is completed, so quitting from camp brings you
back to it.

Camps cannot be pitched indoors, on or under water, while flying, or during
combat. Rough ground such as desert, marsh, mountains and the deep underdark
raises the difficulty, as does rain or a lightning storm.

See also: FEAT INFO ESTABLISH CAMP, SURVEY, HARVEST, WILDERNESS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CAMP', 'CAMP');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CAMP', 'ESTABLISH-CAMP');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('ENCOUNTERS', 'Usage: encounters (depart|escape|distract|intimidate|diplomacy|bluff|bribe)
 
The encounters system allows for random encounters as one travels through the wilderness (ascii 
worldmap) of Lumia.
 
Currently we only have combat-oriented encounters, but have plans to add non-combat encounters such
as vendors, skill-tests, treasure and more.
 
Combat encounters will be loaded based on party level and terrain type of the room entered. Some
encounters will be hostile, while others you can ignore altogether if you choose (just use the 
''encounter depart'' command).
 
When an encounter has been found, the player''s options are as follows:
depart     - leave a non-hostile encounter peacefully
escape     - leave a hostile encounter through various means (HELP ESCAPE)
distract   - leave a hostile encounter using stealth skill
intimidate - make a sentient hostile mob non-hostile using intimidate skill
diplomacy  - make a sentient hostile mob non-hostile using diplomacy skill
bluff      - make a sentient hostile mob non-hostile using bluff skill
bribe      - make a sentient hostile mob non-hostile by giving them gold
 
And of course the party can opt to kill the encounter mobs as well.

Characters with the nature skill can increase or reduce the chance for finding an encounter
with an option that can be toggled in the ''prefedit'' screen.

It is also possible to avoid encounters using your stealth skill, as long as ''sneak''
is enabled, and you have set encounters to ''avoid'' in the prefedit screen.', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ENCOUNTERS', 'ENCOUNTERS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('BLOODLINE-FEY', 'The capricious nature of the fey runs in your family due to some intermingling of
fey blood or magic. You are more emotional than most, prone to bouts of joy and rage.
The fey bloodline offers a great knowledge of nature as well as enhanced abilities in
traversing or dealing with nature.  It also offers abilities using fey magic.
 
NEW CLASS SKILL: Nature

BONUS SPELLS: charm person (3rd), hideous laughter (5th), deep slumber (7th),
poison (9th), feeblemind (11th), true seeing (13th), prismatic spray (15th),
irresistible dance (17th), polymorph (19th).
 
CLASS FEATS: dodge, improved initiative, lightning reflexes, mobility, point blank shot,
precise shot, quicken spell, skill focus
 
The fey bloodline also gives the following bonus abilities:
+2 to dcs of enchantment spells at sorcerer level 1. (HELP FEY ARCANA)
Can cause debilitating laughter in a foe at sorcerer level 1. (HELP LAUGHING TOUCH)
Can travel wilderness faster at sorcerer level 2. (HELP WILDERNESS STRIDE)
Can go invisible 3 times per day at sorcerer level 9. (HELP FLEETING GLANCE)
Can roll twice to overcome spell resistence at sorcerer level 15. (HELP FEY MAGIC)
Gain immunity to poison, +3 damage reduction, animals won''t aggro you
and can cast shadow walk once per day at sorcerer level 20. (HELP SOUL OF THE FEY)
 
See Also: SORCERER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BLOODLINE-FEY', 'BLOODLINE-FEY');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BLOODLINE-FEY', 'FEY-BLOODLINE');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('BLOODLINE-FEY', 'SORCERER-BLOODLINE-FEY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('FLY-NEEDED', 'Rooms that are flagged ''fly-needed'' means that they are rooms
that represent free-space in the air.  If one is flying, they
can move through these rooms normally.  If one is not flying
and arrives at one of these rooms, they will fall.  If
falling, you can try to break one''s fall with spells or
skills

Damage suffered from falling increases per-room falling
straight down.  Characters CAN die from falling.

See Also:  ROOM-FLAGS
#31
FOLK-HERO

FOLK HERO BACKGROUND

Skill bonuses: +2 Handle Animal and +2 Nature.

Rustic Hospitality: Gain the hometown-only TRIBUTE command. While in your
hometown, shop purchases cost exactly 10 percent less and shop sales pay
exactly 10 percent more, subject to each shop''s normal limits.

See also: TRIBUTE, HOMETOWN, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FLY-NEEDED', 'FLY-NEEDED');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('NATURE-SKILL', 'Nature

Nature measures your knowledge of terrain, plants, animals, weather and natural
cycles, and your practical ability to travel and survive outdoors. It is used
for tracking and avoiding tracks, wilderness movement costs, natural lore, and
Establish Camp. Tracking another creature also requires the Track feat.

Survival is the older name for this same skill. Commands accept both names;
existing skill ranks and bonuses are unchanged. There is no separate Survival
skill to train. Establish Camp requires three ranks in Nature.

See also: CAMP, TRACK, LORE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NATURE-SKILL', 'NATURE-SKILL');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NATURE-SKILL', 'SKILL-NATURE');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NATURE-SKILL', 'SKILL-SURVIVAL');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NATURE-SKILL', 'SURVIVAL-SKILL');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NATURE-SKILL', 'NATURE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('OUTLANDER', 'OUTLANDER BACKGROUND

Skill bonuses: +2 Athletics and +2 Nature.

Wanderer: Gain +5 on FORAGE checks and a one-time increase of 20 maximum
hit points. The hit-point increase is applied once even if the Background
is selected after the character has gained levels.

See also: FORAGE, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('OUTLANDER', 'OUTLANDER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('GOLIATH', '	G-----------------------------------------------------------------	n
	G>Description:	n
	n
At the highest mountain peaks far above the slopes where trees
grow and where the air is thin and the frigid winds howl  dwell the reclusive
goliaths. Few folk can claim to have seen a goliath, and fewer still can claim
friendship with them. Goliaths wander a bleak realm of rock, wind, and cold.
Their bodies look as if they are carved from mountain stone and give them great
physical power. Their spirits take after the wandering wind, making them nomads
who wander from peak to peak. Their hearts are infused with the cold regard of
their frigid realm, leaving each goliath with the responsibility to earn a place
in the tribe or die trying.For goliaths, competition exists only when it is
supported by a level playing field. Competition measures talent, dedication, and
effort. Those factors determine survival in their home territory, not reliance
on magic items, money, or other elements that can tip the balance one way or the
other. Goliaths happily rely on such benefits, but they are careful to remember
that such an advantage can always be lost. A goliath who relies too much on them
can grow complacent, a recipe for disaster in the mountains.This trait manifests
most strongly when goliaths interact with other folk. The relationship between
peasants and nobles puzzles goliaths. If a king lacks the intelligence or
leadership to lead, then clearly the most talented person in the kingdom should
take his place. Goliaths rarely keep such opinions to themselves, and mock folk
who rely on society''s structures or rules to maintain power.
	n
	G>Racial Abilities:	n
	n
	WStone''s Endurance	n  - Can reduce damage of the next attack when activated.
	WNatural Athlete	n    - +2 to swim, climb and nature skills.
	WMountain Born	n      - 50% cold resistance.
	WPowerful Build	n     - Can wield large weapons in one hand at -2 to attacks.
	n
	G>Ability Adjustments:  	W +2 Str, +1 Con 	n
	n
	G>Experience Modifier:  	W None 	n
	n
	YSee also:	n RACES
	n', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GOLIATH', 'GOLIATH');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GOLIATH', 'RACE-GOLIATH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated) VALUES ('SURVIVAL', 'Survival is the legacy name for Nature. Both names refer to the same skill and
the same trained ranks. Use HELP NATURE-SKILL for its current description.

See also: NATURE-SKILL, CAMP, TRACK', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level), auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SURVIVAL', 'SURVIVAL');

COMMIT;
