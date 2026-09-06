-- Player-facing help for approved Realms of Luminari class and spell kits.
-- Safe to run repeatedly. The same text is maintained in lib/text/help/help.hlp.

UPDATE help_entries
SET entry = REPLACE(entry,
  'level. The four\n                         spells require Master of Elements and their Wizard level.',
  'level.')
WHERE tag = 'ADVANCED-SPELL-COMMANDS';

UPDATE help_entries
SET entry = REPLACE(entry,
  '- elementalembodiment : Merges your physical form with pure elemental energy.',
  '- elementalembodiment : Merges your physical form with pure elemental energy. The four\n                         spells require Master of Elements and their Wizard level.')
WHERE tag = 'ADVANCED-SPELL-COMMANDS'
  AND entry NOT LIKE '%spells require Master of Elements and their Wizard level%';

UPDATE help_entries
SET entry = REPLACE(entry,
  '- masterofelements    : Allows converting spell damage types between acid, cold, electricity, and fire.',
  '- masterofelements    : Requires any two Focused Element perks. Allows converting spell\n                         damage types and unlocks elemental embodiment spells.')
WHERE tag = 'ADVANCED-SPELL-COMMANDS'
  AND entry NOT LIKE '%Requires any two Focused Element perks%';

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('animal-companion', 'Usage:  call companion

Call companion calls your loyal animal companion to assist you. You first have
to select its type with the study command.

A Ranger 4/Warrior 1 multiclass character with Animal Companion can select a
dire wolf. The dire wolf uses the normal animal-companion level, Boon Companion,
Beast Master, cooldown, following, and dismissal rules. It is tame, mountable,
and one size larger than its rider, up to Colossal.

See Also: STUDY, DRUID, RANGER, DIRE-RAIDER, MOUNT', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

UPDATE help_entries
SET entry = REPLACE(entry,
  'Usage: cast ''elemental air embodiment'' <target>\nDuration:',
  'Usage: cast ''elemental air embodiment'' <target>\nAccess: Wizard level 15 and the Master of Elements perk\nDuration:')
WHERE tag = 'ELEMENTAL-AIR-EMBODIMENT'
  AND entry NOT LIKE '%Access: Wizard level 15 and the Master of Elements perk%';

UPDATE help_entries
SET entry = REPLACE(entry,
  'Usage: cast ''elemental earth embodiment'' <target>\nDuration:',
  'Usage: cast ''elemental earth embodiment'' <target>\nAccess: Wizard level 17 and the Master of Elements perk\nDuration:')
WHERE tag = 'ELEMENTAL-EARTH-EMBODIMENT'
  AND entry NOT LIKE '%Access: Wizard level 17 and the Master of Elements perk%';

UPDATE help_entries
SET entry = REPLACE(entry,
  'Usage: cast ''elemental fire embodiment'' <target>\nDuration:',
  'Usage: cast ''elemental fire embodiment'' <target>\nAccess: Wizard level 17 and the Master of Elements perk\nDuration:')
WHERE tag = 'ELEMENTAL-FIRE-EMBODIMENT'
  AND entry NOT LIKE '%Access: Wizard level 17 and the Master of Elements perk%';

UPDATE help_entries
SET entry = REPLACE(entry,
  'Usage: cast ''elemental water embodiment'' <target>\nDuration:',
  'Usage: cast ''elemental water embodiment'' <target>\nAccess: Wizard level 13 and the Master of Elements perk\nDuration:')
WHERE tag = 'ELEMENTAL-WATER-EMBODIMENT'
  AND entry NOT LIKE '%Access: Wizard level 13 and the Master of Elements perk%';

UPDATE help_entries
SET entry = REPLACE(entry,
  'Usage: cast ''song of travel''\nDuration:',
  'Usage: cast ''song of travel''\nAccess: Bard level 16 through normal known-spell selection\nDuration:')
WHERE tag = 'SONG-OF-TRAVEL'
  AND entry NOT LIKE '%Access: Bard level 16 through normal known-spell selection%';

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('BATTLECHANTER', 'Battlechanter

Battlechanter is an approved themed build through the existing Bard class, not
a separate selectable class. Bard performance, instruments, group support, and
known-spell casting provide the war-chant role. Minor creation enters the Bard
list at level 4 and song of travel at level 16. Warrior or Cleric multiclassing
can add martial or shamanic emphasis but is not required for those spells.

See also: BARD, PERFORM, INSTRUMENT, SONG-OF-TRAVEL, ROL-SPELL-KITS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('DIRE-RAIDER', 'Dire Raider

Dire Raider is an approved Ranger/Warrior build, not a separate selectable
class. A character with Ranger 4, Warrior 1, and Animal Companion can choose a
dire wolf through study. The wolf is a normal animal companion for scaling,
Beast Master bonuses, Boon Companion, following, cooldowns, and dismissal. It
is also tame, mountable, and one size larger than its rider, up to Colossal.

The themed Ranger spell kit includes command undead and protection from
animals at level 6, dust devil at 10, and farsee, pass without trace, and
poltergeist at 15.

See also: RANGER, WARRIOR, ANIMAL-COMPANION, MOUNT, ROL-SPELL-KITS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ELEMENTALIST', 'Elementalist

Elementalist is an approved Wizard specialty build, not a separate selectable
class. Intelligence remains its casting ability. Existing Energy Affinity and
Focused Element perks express elemental choice. Master of Elements requires
any two Focused Element perks and unlocks the four embodiment spells.

The core kit grants minor creation at Wizard level 1, air blast at 5, thunder
lance at 9, earth fog and fire fog at 15, and earthblood at 17. Water
embodiment is available at 13, air at 15, and earth and fire at 17 once Master
of Elements is owned.

See also: WIZARD, MASTEROFELEMENTS, ELEMENTAL-WATER-EMBODIMENT,
ELEMENTAL-AIR-EMBODIMENT, ROL-SPELL-KITS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ROL-MERCENARY', 'Mercenary

Mercenary is retained as a documented Warrior/Rogue multiclass build, not a
separate selectable class. Its source class was disabled for character
creation and had no live spells. Warrior weapons, archery, defense, rescue,
and offense combine with Rogue stealth, escape, backstab, dual wield, and
related techniques to cover the surviving role.

See also: WARRIOR, ROGUE, MULTICLASS, DIRE-RAIDER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHAMAN', 'Shaman

Shaman is an approved spirit-themed build through the existing Cleric class,
not a separate selectable class. Wisdom remains its casting ability. Its kit
grants preserve at Cleric level 3, command undead at 5, farsee at 11, soul
tempest at 13, and ancestral shield and spirit walk at 17.

Converted RoL Shaman Totem objects provide the existing totem progression. A
Cleric can bond a totem at any class level; summoning unlocks at Cleric level
21 and uses Cleric level and Wisdom. The converted procedure retains 21 totem
identities, one active spirit, source-race restrictions, and three attempts per
seven MUD days.

See also: CLERIC, ROL-SHAMAN-TOTEM, ROL-SPELL-KITS, WISDOM', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ROL-SPELL-KITS', 'RoL Player Spell Kits

Converted Realms of Luminari spells use the following class levels. Bard,
Sorcerer, and other spontaneous classes learn entries through their normal
known-spell selection. Other listed classes prepare them normally; Wizards
still require a spellbook or scroll source. No spell in this kit is a domain
spell.

Bard: level 4 minor creation; level 16 song of travel.

Blackguard: level 6 command undead; level 10 curse item and spectral hand;
level 12 Tazrik''s frenzied hound; level 15 dark wrath and unholy aura.

Cleric: level 3 preserve and slow poison; level 5 command undead; level 11
curse item and farsee; level 13 soul tempest; level 17 ancestral shield,
greater realm of protection, and spirit walk.

Druid: level 3 preserve and protection from animals; level 7 create spring and
dust devil; level 11 suffocate; level 13 cyclone and pass without trace; level
15 mud to rock and rock to mud; level 17 moonwell.

Ranger: level 6 command undead, create spring, and protection from animals;
level 10 dust devil; level 12 nature''s blessing; level 15 farsee, pass without
trace, and poltergeist.

Sorcerer: level 1 ventriloquate; level 4 minor creation; level 8 farsee.

Wizard: level 1 minor creation, preserve, shadow bolt, and ventriloquate; level
3 blackthorns, command undead, and protection from undead; level 5 air blast,
blink, minute meteors, minor rejuvenation, and soul bind; level 7 command
horde, embalm, farsee, fumble, and spectral hand; level 9 heal undead, shadow
burst, shadow magic, stumble, and thunder lance; level 11 age, enervate, nerve
dance, major rejuvenation, and tranquility; level 13 Beltyn''s burning blood,
camouflage, corpse glamor, water embodiment, ice layer, phantasmal blades,
protect undead, sequester, shadechill, and shadow flux; level 15 airy water,
blacklight burst, blackmantle, earth fog, air embodiment, feign death, fire fog,
mislead, phantom heal, rain of blood, and sun shadow; level 17 constriction,
death pact, dimension shift, earthblood, earth embodiment, fire embodiment,
fell frost, ice tomb, lava burst, lich touch, rot, sandblast, and sandstorm.

The four elemental embodiment spells additionally require Master of Elements.
Comprehend languages, wraithform, unseen servant, needle swarm, snapping teeth,
agility, and call lycanthrope are content-only: their native handlers remain
available to converted content, but no player class or domain grants them.

See also: SHAMAN, ELEMENTALIST, BATTLECHANTER, DIRE-RAIDER, ROL-MERCENARY,
SPELLS, PREPARE, STUDY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('BATTLECHANTER', 'BATTLECHANTER'),
  ('BATTLECHANTER', 'ROL-BATTLECHANTER'),
  ('DIRE-RAIDER', 'DIRE-RAIDER'),
  ('DIRE-RAIDER', 'DIRERAIDER'),
  ('DIRE-RAIDER', 'ROL-DIRE-RAIDER'),
  ('ELEMENTALIST', 'ELEMENTALIST'),
  ('ELEMENTALIST', 'ROL-ELEMENTALIST'),
  ('ROL-MERCENARY', 'ROL-MERCENARY'),
  ('SHAMAN', 'SHAMAN'),
  ('SHAMAN', 'ROL-SHAMAN'),
  ('ROL-SPELL-KITS', 'ROL-SPELL-KITS'),
  ('ROL-SPELL-KITS', 'CONVERTED-SPELL-KITS'),
  ('ROL-SPELL-KITS', 'CONTENT-ONLY-SPELLS'),
  ('ROL-SPELL-KITS', 'SPELL-KITS');

DELETE FROM help_keywords
WHERE help_tag = 'BATTLECHANTER'
  AND keyword NOT IN ('BATTLECHANTER', 'ROL-BATTLECHANTER');
DELETE FROM help_keywords
WHERE help_tag = 'DIRE-RAIDER'
  AND keyword NOT IN ('DIRE-RAIDER', 'DIRERAIDER', 'ROL-DIRE-RAIDER');
DELETE FROM help_keywords
WHERE help_tag = 'ELEMENTALIST'
  AND keyword NOT IN ('ELEMENTALIST', 'ROL-ELEMENTALIST');
DELETE FROM help_keywords
WHERE help_tag = 'ROL-MERCENARY' AND keyword <> 'ROL-MERCENARY';
DELETE FROM help_keywords
WHERE help_tag = 'SHAMAN'
  AND keyword NOT IN ('SHAMAN', 'ROL-SHAMAN');
DELETE FROM help_keywords
WHERE help_tag = 'ROL-SPELL-KITS'
  AND keyword NOT IN ('ROL-SPELL-KITS', 'CONVERTED-SPELL-KITS',
                      'CONTENT-ONLY-SPELLS', 'SPELL-KITS');

/* Remove the superseded draft tag without disturbing the established PETS keyword. */
DELETE k FROM help_keywords AS k
JOIN help_entries AS h ON h.tag = k.help_tag
WHERE BINARY h.tag = 'MERCENARY'
  AND h.entry LIKE '%Mercenary is retained as a documented Warrior/Rogue%';
DELETE FROM help_entries
WHERE BINARY tag = 'MERCENARY'
  AND entry LIKE '%Mercenary is retained as a documented Warrior/Rogue%';
