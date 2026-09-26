-- Player help for the Duris races: twelve creation races and five quest-only
-- descend forms, one RACE-<SLUG> topic each, plus Thri-Kreen in EPIC-RACES.
-- The database help system is authoritative; lib/text/help/help.hlp carries
-- the same text. This migration is safe to run repeatedly.
-- See docs/guides/PLAYER_RACES_REFERENCE.md

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-CENTAUR', 'Centaur

Centaurs have the torso of a human rising from the body of a great draft
horse. They are quiet, shy folk of the deep forests, slow to trust outsiders
and tireless on the move. Their strength and hardy constitution make them
fearsome warriors, and a centaur at full gallop can trample foes underfoot.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +3 Str, +5 Con, -1 Int
Size: Large
Family: monstrous humanoid
Alignments: any
Language: Elven

Racial feats:
Level 1: Quadruped Body, Tauric Frame, Doorbash.
Level 11: Stampede.
Level 16: Greatsword Mastery.

Tauric Frame: the horse body cannot wear leg or foot equipment; the ankle
slots remain. Quadruped Body: only a larger attacker can knock a centaur down,
and a centaur cannot ride a mount.

See also: ACCEXP, RACE, QUADRUPED-BODY, STAMPEDE, GREATSWORD-MASTERY,
RACE-WEMIC', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-CENTAUR', 'CENTAUR');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-CENTAUR', 'RACE-CENTAUR'),
('RACE-CENTAUR', 'CENTAUR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-GITHZERAI', 'Githzerai

Githzerai are an ascetic, disciplined planar people who raised their
monasteries in the chaos of Limbo. Slender and gaunt, with yellow-tinged skin
and unblinking eyes, they give their lives to mental discipline and the
martial arts. Ages of war against the illithids and their githyanki cousins
have honed them into patient hunters of mind flayers who shrug off hostile
magic and shift between the planes.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +4 Int, +3 Wis, -1 Cha
Size: Medium
Family: humanoid
Alignments: any
Language: Common

Racial feats:
Level 1: Ultravision, Half Drow Spell Resist, Innate Plane Shift, Quick
  Thinking.
Level 6: Drow Levitate.
Level 11: Rrakkma.

Half Drow Spell Resist gives spell resistance that grows with level. Innate
Plane Shift, Drow Levitate, and Rrakkma are used as commands.

See also: ACCEXP, RACE, INNATE-PLANE-SHIFT, QUICK-THINKING, RRAKKMA,
RACE-GITHYANKI', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-GITHZERAI', 'GITHZERAI');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-GITHZERAI', 'RACE-GITHZERAI'),
('RACE-GITHZERAI', 'GITHZERAI');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-FIRBOLG', 'Firbolg

Firbolgs are gentle giant-kin of the highland forests and mountain glens,
standing nine to ten feet tall with flowing beards and dense, massive frames.
They live in harmony with the woodland spirits and are slow to anger, but a
roused firbolg smashes gates and bodyslams armored foes. Their bulk makes them
slow spellcasters, and magic bites deeper into them than into other folk.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +5 Str, +4 Con, -1 Int, -1 Dex
Size: Large
Family: giant
Alignments: any
Language: Giant

Racial feats:
Level 1: Bodyslam, Doorbash, Forest Sight, Magic Vulnerability, Slow Casting.
Level 6: Outdoor Stealth.
Level 11: Hatred.
Level 16: Hammer Mastery.

Drawbacks: Magic Vulnerability (spells deal 10 percent more damage) and one
rank of Slow Casting (timed casts take 10 percent longer).

See also: ACCEXP, RACE, MAGIC-VULNERABILITY, FOREST-SIGHT, HATRED,
HAMMER-MASTERY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-FIRBOLG', 'FIRBOLG');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-FIRBOLG', 'RACE-FIRBOLG'),
('RACE-FIRBOLG', 'FIRBOLG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-GITHYANKI', 'Githyanki

Githyanki are a ruthless people of the Astral Plane, once slaves of the
illithids, who broke free after learning many of their masters'' mental powers.
Tall, thin, and gaunt, with faces like drawn corpses, they are sharper of mind
than humans and hate their former masters above all else. Their psionic
blasts, planar shifting, and silver-bladed swordplay make them feared raiders.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +4 Int, -1 Cha
Size: Medium
Family: humanoid
Alignments: any but good
Language: Common

Racial feats:
Level 1: Ultravision, Half Drow Spell Resist, Innate Plane Shift, Enhanced
  Spell Damage, Innate Psionic Blast.
Level 6: Drow Levitate, Longsword Mastery.

Half Drow Spell Resist gives spell resistance that grows with level, and
Enhanced Spell Damage adds to the damage of your spells.

See also: ACCEXP, RACE, INNATE-PLANE-SHIFT, INNATE-PSIONIC-BLAST,
LONGSWORD-MASTERY, RACE-GITHZERAI', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-GITHYANKI', 'GITHYANKI');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-GITHYANKI', 'RACE-GITHYANKI'),
('RACE-GITHYANKI', 'GITHYANKI');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-KOBOLD', 'Kobold

Kobolds are small, scaly, reptilian folk of the deep caverns who take fierce
pride in their claimed draconic heritage. Cunning, quick, and industrious,
they are natural miners and trap-builders who slip silently through dark
tunnels and calm the dangerous beasts they meet there. Their reflexes make
them swift casters and shrewd traders.

Tier: Normal
Unlock: none
Progression: normal experience requirements
Ability modifiers: -1 Str, +2 Int, +2 Dex
Size: Small
Family: humanoid
Alignments: any but good
Language: Kobold

Racial feats:
Level 1: Ultravision, Underdark Stealth, Calming, Barter, Fast Casting.
Level 26: Miner.

A normal race, free to play. One rank of Fast Casting makes timed casts 10
percent quicker.

See also: ACCEXP, RACE, UNDERDARK-STEALTH, CALMING, BARTER, MINER,
FAST-CASTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-KOBOLD', 'KOBOLD');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-KOBOLD', 'RACE-KOBOLD'),
('RACE-KOBOLD', 'KOBOLD');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-DRIDER', 'Drider

Driders are the upper torso and head of a drow elf fused at the waist to the
eight-legged body of a giant spider. Revering the dark spider goddesses, they
combine drow magic with the deadly traits of the arachnid: webs that wrap
their prey, a spider''s balance that keeps them on their feet, and a natural
resistance to hostile spells. Their spider bodies cannot wear leg or foot
equipment.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +5 Con, +3 Dex, -1 Cha
Size: Large
Family: aberration
Alignments: evil only
Language: Undercommon

Racial feats:
Level 1: Ultravision, Half Drow Spell Resist, Quadruped Body, Tauric Frame,
  Innate Web.
Level 11: Groundfighting, Innate Fireball.
Level 27: Innate Mass Dispel.

Tauric Frame: the spider body cannot wear leg or foot equipment; the ankle
slots remain. Quadruped Body: only a larger attacker can knock a drider down,
and a drider cannot ride a mount.

See also: ACCEXP, RACE, QUADRUPED-BODY, GROUNDFIGHTING, INNATE-WEB,
RACE-CENTAUR', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-DRIDER', 'DRIDER');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-DRIDER', 'RACE-DRIDER'),
('RACE-DRIDER', 'DRIDER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-THRI-KREEN', 'Thri-Kreen

Thri-kreen, the mantis warriors, are desert nomads who resemble a giant
praying mantis walking on two legs with four arms. Light for their seven-foot
height, they are swift and fearsomely dexterous, and they can wield a second
pair of weapons. They have no true fingers or ears, and their shape cannot
wear body armor or footwear. A venomous bite and great leaps serve them well,
but the cold is their bane.

Tier: Epic (level adjustment 10)
Unlock: 50,000 account experience
Progression: 7x normal experience requirements
Ability modifiers: -2 Int, -2 Wis, +4 Dex, -2 Cha
Size: Medium
Family: monstrous humanoid
Alignments: any
Language: Common

Racial feats:
Level 1: Ultravision, Four Arms, Psionic Resistance, Vulnerable to Cold.
Level 6: Poison Bite.
Level 11: Leap.

Four Arms: a second pair of arms wields a second pair of weapons and wears a
second set of sleeves, gloves, and wrist items. Thri-kreen cannot wear body
armor, footwear, rings, or earrings, and they take 20 percent more cold
damage. Psionic Resistance cuts mental damage by 20 percent.

See also: ACCEXP, RACE, FOUR-ARMS, EPIC-RACES, PSIONIC-RESISTANCE, LEAP', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-THRI-KREEN', 'THRI-KREEN', 'THRIKREEN');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-THRI-KREEN', 'RACE-THRI-KREEN'),
('RACE-THRI-KREEN', 'THRI-KREEN'),
('RACE-THRI-KREEN', 'THRIKREEN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('MINOTAUR', 'Minotaur

Minotaurs have the torso of a man and the head and legs of a bull, with great
horns sweeping from their brows. Stronger than nearly any other folk and able
to take tremendous punishment, they hold honor and pride above all else. Their
tempers are short: a badly wounded minotaur falls into a bloodlust that will
not let it cast or flee. Their horns keep them from wearing anything on their
heads.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +3 Str, +4 Con
Size: Large
Family: monstrous humanoid
Alignments: any
Language: Giant

Racial feats:
Level 1: Ultravision, Doorbash, Bloodlust.
Level 6: Bull Charge, Axe Mastery, Innate Scare.
Level 21: Fearlessness.

Great horns keep minotaurs from wearing anything on the head. Drawback:
Bloodlust. Below half hit points a minotaur rages and cannot cast or flee.

See also: ACCEXP, RACE, BULL-CHARGE, BLOODLUST, AXE-MASTERY, INNATE-SCARE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-MINOTAUR', 'MINOTAUR');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('MINOTAUR', 'RACE-MINOTAUR'),
('MINOTAUR', 'MINOTAUR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-KUO-TOA', 'Kuo Toa

Kuo-toa are fish-folk of the flooded Underdark: scaled, bulging-eyed, and
web-handed, with a zealot''s devotion to their strange gods. They are at home
in swamps and the water, keen of sense and hard to catch unawares, and in time
they breathe water and call down lightning. The surface sun burns them.

Tier: Normal
Unlock: none
Progression: normal experience requirements
Ability modifiers: +1 Str, +2 Con
Size: Medium
Family: monstrous humanoid
Alignments: evil only
Language: Undercommon

Racial feats:
Level 1: Ultravision, Keen Senses, Swamp Stealth, Seadog, Sun Vulnerability,
  Slow Casting.
Level 8: Water Breathing.
Level 15: Innate Lightning Bolt.

A normal race, free to play. Drawbacks: Sun Vulnerability and one rank of Slow
Casting (timed casts take 10 percent longer).

See also: ACCEXP, RACE, SUN-VULNERABILITY, SWAMP-STEALTH, WATER-BREATHING,
INNATE-LIGHTNING-BOLT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-KUO-TOA', 'KUO-TOA', 'KUOTOA');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-KUO-TOA', 'RACE-KUO-TOA'),
('RACE-KUO-TOA', 'KUO-TOA'),
('RACE-KUO-TOA', 'KUOTOA');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-OROG', 'Orog

Orogs, the deep orcs, are a towering and disciplined race of great-orcs who
hold the deepest fortresses of the Underdark. Larger, stronger, and shrewder
than their surface kin, they are born war leaders who forge heavy plate in
magma foundries and call their hordes and wargs to battle. The surface sun
weakens them, and they are slow to work magic.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +3 Str, +4 Con, -2 Int
Size: Medium
Family: humanoid
Alignments: any but good
Language: Orcish

Racial feats:
Level 1: Ultravision, Hardy, Armor Skin, Magical Reduction, Sun Vulnerability,
  Slow Casting (6 ranks).
Level 6: Summon Horde.
Level 8: Summon Warg.
Level 11: Warcaller''s Fury.

Orogs gain one extra hit point per level. Drawbacks: Sun Vulnerability and six
ranks of Slow Casting (timed casts take 60 percent longer).

See also: ACCEXP, RACE, WARCALLERS-FURY, MAGICAL-REDUCTION, SUMMON-HORDE,
SUN-VULNERABILITY, SLOW-CASTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-OROG', 'OROG');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-OROG', 'RACE-OROG'),
('RACE-OROG', 'OROG');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-HARPY', 'Harpy

Harpies are slender bird-folk of the jungles, humanoid in shape but with great
wings and taloned feet, living in towns ruled by their females. Frail beside
most races but quick, hardy, and keen-eyed, they take to the mystic arts far
more readily than to the sword, and they cast with a bird''s swiftness. They
are born neutral and may follow any path.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: -2 Str, +1 Con, +2 Int, +1 Wis, +2 Dex
Size: Small
Family: monstrous humanoid
Alignments: any
Language: Common

Racial feats:
Level 1: Ultravision, Wings, Keen Senses, Hardy, Fast Casting (3 ranks).
Level 11: Innate Farsee.
Level 16: Innate Haste.

Harpies gain one extra hit point per level, and three ranks of Fast Casting
make timed casts 30 percent quicker.

See also: ACCEXP, RACE, FLY, INNATE-FARSEE, INNATE-HASTE, FAST-CASTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-HARPY', 'HARPY');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-HARPY', 'RACE-HARPY'),
('RACE-HARPY', 'HARPY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-STORMKIN', 'Stormkin

Stormkin carry the blood of the storm giants: towering, good-natured folk
several feet taller and far stronger than any human. They are slow of foot and
slower to work magic, but their hides are thick, they smash through doors, and
in time they hurl lightning like their giant forebears.

Tier: Advanced (level adjustment 2)
Unlock: 1,000 account experience
Progression: 2x normal experience requirements
Ability modifiers: +5 Str, +4 Con, -2 Dex
Size: Large
Family: giant
Alignments: any
Language: Giant

Racial feats:
Level 1: Low Light Vision, Doorbash, Slow Casting (6 ranks).
Level 10: Innate Lightning Bolt.
Level 11: Thick Hide.

Drawback: six ranks of Slow Casting (timed casts take 60 percent longer).

See also: ACCEXP, RACE, THICK-HIDE, INNATE-LIGHTNING-BOLT, SLOW-CASTING,
RACE-FIRBOLG', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-STORMKIN', 'STORMKIN');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-STORMKIN', 'RACE-STORMKIN'),
('RACE-STORMKIN', 'STORMKIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-DEATH-KNIGHT', 'Death Knight

Death knights are fallen paladins raised into undeath and bound in eternal
torment, their rage at their fate spent on all that lives. Unlike liches, they
fight in heavy armor with the great blades of warriors, call down hellfire,
and wreathe themselves in flame; lesser undead bow to them.

Tier: Epic, quest only (level adjustment 10)
Progression: 10x normal experience requirements
Ability modifiers: +8 Str, +6 Con, -2 Int, +6 Wis, -2 Dex
Size: Large
Family: undead
Alignments: evil only
Language: none

A Death Knight is a descend form, gained only by completing its quest. The
quest needs level 30, levels in Blackguard or Warrior, and no group, leader,
or followers. The conversion cannot be undone: it rebuilds you as a level one
Blackguard, resets your experience to 0, and sets your alignment to evil. A
Lich, Vampire, or other descend form cannot take it.

Every descend form shares Armor Skin (5 ranks), Vital, Hardy, Toughness, Fast
Healing, and three ranks of Epic Damage Reduction (damage reduction 9/-), 10
extra hit points plus 1 per level, and the traits of the undead.

Racial feats beyond these:
Level 1: Greatsword Mastery, Ultravision, Hellish Resistance, Undead Fealty,
  Sun Vulnerability, Slow Casting (5 ranks).
Level 13: Innate Fire Storm.
Level 16: Innate Fire Shield.
Level 23: Sacrilegious Power.

Drawbacks: Sun Vulnerability and five ranks of Slow Casting.

See also: RACE, UNDEAD, LICH, VAMPIRE, INNATE-FIRE-STORM, INNATE-FIRE-SHIELD,
SACRILEGIOUS-POWER, UNDEAD-FEALTY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-DEATH-KNIGHT', 'DEATH-KNIGHT', 'DEATHKNIGHT');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-DEATH-KNIGHT', 'RACE-DEATH-KNIGHT'),
('RACE-DEATH-KNIGHT', 'DEATH-KNIGHT'),
('RACE-DEATH-KNIGHT', 'DEATHKNIGHT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-WIGHT', 'Wight

Wights are towering undead, something between a zombie and a skeleton, as tall
as an ogre though leaner. Immensely strong and cold to the core, they breathe
frost, harden their skin like stone, and batter down doors and foes alike, but
fire is their undoing.

Tier: Epic, quest only (level adjustment 10)
Progression: 10x normal experience requirements
Ability modifiers: +10 Str, +10 Con, -2 Int, -2 Cha
Size: Large
Family: undead
Alignments: evil only
Language: none

A Wight is a descend form, gained only by completing its quest. The quest
needs level 30, levels in Warrior, and no group, leader, or followers. The
conversion cannot be undone: it rebuilds you as a level one Warrior, resets
your experience to 0, and sets your alignment to evil. A Lich, Vampire, or
other descend form cannot take it.

Every descend form shares Armor Skin (5 ranks), Vital, Hardy, Toughness, Fast
Healing, and three ranks of Epic Damage Reduction (damage reduction 9/-), 10
extra hit points plus 1 per level, and the traits of the undead.

Racial feats beyond these:
Level 1: Immune to Cold, Ultravision, Bodyslam, Doorbash, Weakness to Fire,
  Slow Casting (9 ranks).
Level 6: Innate Frost Breath.
Level 13: Innate Stoneskin.

Drawbacks: Weakness to Fire and nine ranks of Slow Casting.

See also: RACE, UNDEAD, LICH, VAMPIRE, INNATE-FROST-BREATH, INNATE-STONESKIN,
BODYSLAM, DOORBASH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-WIGHT', 'WIGHT');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-WIGHT', 'RACE-WIGHT'),
('RACE-WIGHT', 'WIGHT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-REVENANT', 'Revenant

Revenants are corpses reanimated by a burning will for revenge, bound to the
world by a compact with unholy powers. They keep much of their living strength
and stamina, regenerate their wounds, step from shadow to shadow, and fight
with a frenzy that makes them the elite of the undead armies. Fire burns them
badly.

Tier: Epic, quest only (level adjustment 10)
Progression: 10x normal experience requirements
Ability modifiers: +7 Str, +7 Con, -2 Int, +6 Dex, -2 Cha
Size: Large
Family: undead
Alignments: evil only
Language: none

A Revenant is a descend form, gained only by completing its quest. The quest
needs level 30, levels in Warrior or Rogue, and no group, leader, or
followers. The conversion cannot be undone: it rebuilds you as a level one
Warrior, resets your experience to 0, and sets your alignment to evil. A Lich,
Vampire, or other descend form cannot take it.

Every descend form shares Armor Skin (5 ranks), Vital, Hardy, Toughness, Fast
Healing, and three ranks of Epic Damage Reduction (damage reduction 9/-), 10
extra hit points plus 1 per level, and the traits of the undead.

Racial feats beyond these:
Level 1: Ultravision, Troll Regeneration, Bodyslam, Doorbash, Weakness to
  Fire, Slow Casting (3 ranks).
Level 8: Battle Frenzy.
Level 13: Innate Shadow Jump.

Drawbacks: Weakness to Fire and three ranks of Slow Casting.

See also: RACE, UNDEAD, LICH, VAMPIRE, BATTLE-FRENZY, INNATE-SHADOW-JUMP,
BODYSLAM, DOORBASH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-REVENANT', 'REVENANT');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-REVENANT', 'RACE-REVENANT'),
('RACE-REVENANT', 'REVENANT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-SHADOW-BEAST', 'Shadow Beast

Shadow beasts are undead killers woven from darkness, swift and silent in the
deep places. They swell their own strength and size with dark magic and strike
in a blur of claws, but fire tears through their shadowy flesh.

Tier: Epic, quest only (level adjustment 10)
Progression: 10x normal experience requirements
Ability modifiers: +7 Con, +3 Int, -2 Wis, +10 Dex, -2 Cha
Size: Medium
Family: undead
Alignments: evil only
Language: none

A Shadow Beast is a descend form, gained only by completing its quest. The
quest needs level 30, levels in Rogue or Assassin, and no group, leader, or
followers. The conversion cannot be undone: it rebuilds you as a level one
Rogue, resets your experience to 0, and sets your alignment to evil. A Lich,
Vampire, or other descend form cannot take it.

Every descend form shares Armor Skin (5 ranks), Vital, Hardy, Toughness, Fast
Healing, and three ranks of Epic Damage Reduction (damage reduction 9/-), 10
extra hit points plus 1 per level, and the traits of the undead.

Racial feats beyond these:
Level 1: Ultravision, Underdark Stealth, Duergar Strength, Duergar Enlarge,
  Weakness to Fire, Slow Casting.
Level 18: Racial Flurry.

Drawbacks: Weakness to Fire and one rank of Slow Casting.

See also: RACE, UNDEAD, LICH, VAMPIRE, UNDERDARK-STEALTH, RACIAL-FLURRY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-SHADOW-BEAST', 'SHADOW-BEAST', 'SHADOWBEAST');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-SHADOW-BEAST', 'RACE-SHADOW-BEAST'),
('RACE-SHADOW-BEAST', 'SHADOW-BEAST'),
('RACE-SHADOW-BEAST', 'SHADOWBEAST');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RACE-PHANTOM', 'Phantom

Phantoms are ethereal, half-transparent undead, solitary by nature, whose
insubstantial forms slip past obstacles and turn aside hostile magic. They
shift between the planes, cast with frightening speed and power, drift as
mist, and in time take to the air, but fire harms them.

Tier: Epic, quest only (level adjustment 10)
Progression: 10x normal experience requirements
Ability modifiers: -2 Str, +5 Con, +10 Int, +3 Dex
Size: Medium
Family: undead
Alignments: evil only
Language: none

A Phantom is a descend form, gained only by completing its quest. The quest
needs level 30, levels in Wizard, Summoner, or Psionicist, and no group,
leader, or followers. The conversion cannot be undone: it rebuilds you as a
level one Wizard, resets your experience to 0, and sets your alignment to
evil. A Lich, Vampire, or other descend form cannot take it.

Every descend form shares Armor Skin (5 ranks), Vital, Hardy, Toughness, Fast
Healing, and three ranks of Epic Damage Reduction (damage reduction 9/-), 10
extra hit points plus 1 per level, and the traits of the undead.

Racial feats beyond these:
Level 1: Ultravision, Half Drow Spell Resist, Innate Plane Shift, Enhanced
  Spell Damage, Eyeless, Weakness to Fire, Fast Casting (3 ranks).
Level 10: Vampiric Gaseous Form.
Level 11: Wings, Spell Absorb.

Drawback: Weakness to Fire. Three ranks of Fast Casting make timed casts 30
percent quicker.

See also: RACE, UNDEAD, LICH, VAMPIRE, EYELESS, SPELL-ABSORB,
INNATE-PLANE-SHIFT, FAST-CASTING', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN ('RACE-PHANTOM', 'PHANTOM');

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
('RACE-PHANTOM', 'RACE-PHANTOM'),
('RACE-PHANTOM', 'PHANTOM');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('EPIC-RACES', 'Crystal Dwarf
Trelux
Thri-Kreen

See staff for more info, epic races still early in development.

You can use the ACCEXP command to unlock epic races.

See Also:  RACES

*** NOTE ***
These races are not eligible to multiclass.
', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES (entry), min_level = VALUES (min_level),
auto_generated = VALUES (auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('EPIC-RACES', 'EPIC-RACES');

COMMIT;
