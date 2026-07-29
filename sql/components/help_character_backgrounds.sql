-- Character Background and Background-ability help entries.
--
-- The database help system is authoritative. This migration is idempotent and
-- provides the entries promised by HELP BACKGROUND-ARCHTYPES, plus operational
-- help for the commands used by Background abilities.

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ACOLYTE', 'ACOLYTE BACKGROUND

Skill bonuses: +2 Sense Motive and +2 Religion.

Temple Service: When a grouped Acolyte is present at a city temple, members
of that group can receive the temple\'s listed blessings free of charge.

See also: BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ACOLYTE', 'ACOLYTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CHARLATAN', 'CHARLATAN BACKGROUND

Skill bonuses: +2 Sleight of Hand and +2 Bluff.

False Identity: Gain the SWINDLE command. A successful con against an
eligible non-player character can earn coins and occasionally an item. A
failed con provokes the target.

See also: SWINDLE, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CHARLATAN', 'CHARLATAN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('CRIMINAL-SPY', 'CRIMINAL / SPY BACKGROUND

Skill bonuses: +2 Bluff and +2 Stealth.

Criminal Contact: Gain the RELAY command, understand and speak Thieves\'
Cant, and receive access to shops marked for the black market.

See also: RELAY, FORGEAS, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CRIMINAL-SPY', 'CRIMINAL');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CRIMINAL-SPY', 'CRIMINAL-SPY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CRIMINAL-SPY', 'CRIMINAL/SPY');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('CRIMINAL-SPY', 'SPY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ENTERTAINER', 'ENTERTAINER BACKGROUND

Skill bonuses: +2 Acrobatics and +2 Perform.

By Popular Demand: Gain the ENTERTAIN command. A successful performance
for an eligible non-player character can earn a tip, occasionally produce
an item, and temporarily improve Persuasion, Deception, and Perform.

See also: ENTERTAIN, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ENTERTAINER', 'ENTERTAINER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('FOLK-HERO', 'FOLK HERO BACKGROUND

Skill bonuses: +2 Handle Animal and +2 Survival.

Rustic Hospitality: Gain the hometown-only TRIBUTE command. While in your
hometown, shop purchases cost exactly 10 percent less and shop sales pay
exactly 10 percent more, subject to each shop\'s normal limits.

See also: TRIBUTE, HOMETOWN, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FOLK-HERO', 'FOLK HERO');
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FOLK-HERO', 'FOLK-HERO');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('GLADIATOR', 'GLADIATOR BACKGROUND

Skill bonuses: +2 Acrobatics and +2 Perform.

Arena Renown: Gain +1 to attack and damage while you belong to a clan and
fight in an area allied with that clan.

See also: BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('GLADIATOR', 'GLADIATOR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('TRADER', 'TRADER BACKGROUND

Skill bonuses: +2 Sense Motive and +2 Diplomacy.

Guild Membership: Gain +1 to every crafting skill.

See also: CRAFTING, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('TRADER', 'TRADER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('HERMIT', 'HERMIT BACKGROUND

Skill bonuses: +2 Heal and +2 Religion.

Life of Seclusion: While genuinely alone in the room, with no companion
present, gain +1 damage and 5 percent more experience. The bonuses stop
when another party member is present.

See also: BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('HERMIT', 'HERMIT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SQUIRE', 'SQUIRE BACKGROUND

Skill bonuses: +2 History and +2 Diplomacy.

Hired Retainer: Gain the RETAINER command. Your retainer can be called to
carry and sell items or to deliver a message to an eligible player.

See also: RETAINER, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SQUIRE', 'SQUIRE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('NOBLE', 'NOBLE BACKGROUND

Skill bonuses: +2 History and +2 Diplomacy.

Position of Privilege: Shops reserved for nobles recognize you. While in
your hometown, shop purchases cost exactly 10 percent less and shop sales
pay exactly 10 percent more, subject to each shop\'s normal limits.

See also: HOMETOWN, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('NOBLE', 'NOBLE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('OUTLANDER', 'OUTLANDER BACKGROUND

Skill bonuses: +2 Athletics and +2 Survival.

Wanderer: Gain +5 on FORAGE checks and a one-time increase of 20 maximum
hit points. The hit-point increase is applied once even if the Background
is selected after the character has gained levels.

See also: FORAGE, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('OUTLANDER', 'OUTLANDER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('PIRATE', 'PIRATE BACKGROUND

Skill bonuses: +2 Athletics and +2 Perception.

Bad Reputation: Gain the EXTORT command. A successful intimidation attempt
against an eligible non-player character can earn coins and occasionally
an item.

See also: EXTORT, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('PIRATE', 'PIRATE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SAGE', 'SAGE BACKGROUND

Skill bonuses: +2 Arcana and +2 History.

Researcher: Gain +2 on Lore checks about items. After you successfully use
LORE to identify a creature, your party gains +1 to hit and damage against
that researched creature while the effect applies.

See also: LORE, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SAGE', 'SAGE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SAILOR', 'SAILOR BACKGROUND

Skill bonuses: +2 Athletics and +2 Perception.

Ship\'s Passage: Scheduled sailing is free and takes half the usual time.
Gain +1 to hit, damage, and armor class in or near water, and +5 to the
Fishing crafting skill.

See also: SAILING, FISHING, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SAILOR', 'SAILOR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SOLDIER', 'SOLDIER BACKGROUND

Skill bonuses: +2 Athletics and +2 Intimidate.

Military Rank: Gain +1 to attack and armor class while at least one actual
grouped companion is present in the room. The companion does not need the
Soldier Background.

See also: GROUP, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SOLDIER', 'SOLDIER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('URCHIN', 'URCHIN BACKGROUND

Skill bonuses: +2 Sleight of Hand and +2 Stealth.

Streetwise Survivor: Gain +1 competence armor class while in your
character\'s hometown.

See also: HOMETOWN, BACKGROUND-ARCHTYPES', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('URCHIN', 'URCHIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SWINDLE', 'Usage: swindle <target>

Charlatan Background command. Target an intelligent non-player character
in the room. Your Deception opposes the target\'s Insight. The target
becomes wary after one attempt until its cooldown expires.

Success awards coins and has a small chance to award an item. Failure
reveals the con and causes the target to attack you.

See also: CHARLATAN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SWINDLE', 'SWINDLE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('ENTERTAIN', 'Usage: entertain <target>

Entertainer Background command. Target an intelligent non-player character
in the room. Your Perform check opposes the target\'s Discipline. The target
will accept only one attempt until its cooldown expires.

Success awards a coin tip, has a small chance to award an item, and can
temporarily grant +3 morale to Persuasion, Deception, and Perform. Failure
leaves the target unimpressed but does not make it attack.

See also: ENTERTAINER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('ENTERTAIN', 'ENTERTAIN');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('TRIBUTE', 'Usage: tribute <target>

Folk Hero Background command. It works only in your hometown and targets
an intelligent non-player character in the room. Your Persuasion opposes
the target\'s Insight. The target will consider only one request until its
cooldown expires.

Success awards coins and has a small chance to award an item. Failure means
the request is denied.

See also: FOLK-HERO, HOMETOWN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('TRIBUTE', 'TRIBUTE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('EXTORT', 'Usage: extort <target>

Pirate Background command. Target an intelligent non-player character in
the room. Your Intimidate check opposes the target\'s Discipline. The target
can be subjected to only one attempt until its cooldown expires.

Success awards coins and has a small chance to award an item. Failure means
the target refuses the demand.

See also: PIRATE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('EXTORT', 'EXTORT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RELAY', 'Usage: relay <player> <message>

Criminal / Spy Background command. Your contacts deliver a written message
to an online player who is currently in a city. Non-player characters and
players outside cities are not valid recipients.

The note is signed with your name unless FORGEAS has prepared a different
signature. A prepared signature is consumed by the delivery.

See also: CRIMINAL-SPY, FORGEAS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('RELAY', 'RELAY');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('FORGEAS', 'Usage: forgeas <signature>

Prepare a signature for the next note you write or message you send with
RELAY. The attempted forgery carries a Linguistics-based check and does not
guarantee that an examiner will believe it. The prepared signature is
consumed when used. Player characters only; the signature is limited to 50
visible characters.

See also: RELAY, CRIMINAL-SPY', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FORGEAS', 'FORGEAS');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('FORAGE', 'Usage: forage

Search for food while in the wilderness. The attempt rolls Nature against
difficulty 15. A successful attempt finds food. A failed attempt finds
nothing and starts a cooldown before you can try again.

Characters with the Outlander Background gain +5 on this check.

See also: OUTLANDER, NATURE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('FORAGE', 'FORAGE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RETAINER', 'Usage:
  retainer call
  retainer sell
  retainer recipient <player>
  retainer mail <message>

Squire Background command. CALL summons your hired retainer when the
ability is off cooldown. Give items to the retainer, then use SELL to turn
them into a bank-note payment and dismiss the retainer.

To send mail, set an online player with RECIPIENT, CALL the retainer, then
use MAIL. Delivery is limited by the recipient\'s location and faction
requirements. The retainer departs after delivering the note.

See also: SQUIRE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('RETAINER', 'RETAINER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('SHORTCUT', 'Usage: shortcut <player|mobile|room-vnum>

Travel to a reachable target room inside your own hometown. The character
must have a hometown, and the named character, mobile, or room must also be
inside that hometown. This is not an Urchin Background benefit.

See also: HOMETOWN, URCHIN', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level);
INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES ('SHORTCUT', 'SHORTCUT');
