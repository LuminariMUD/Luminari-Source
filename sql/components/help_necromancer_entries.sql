-- Necromancer class, feat, spell, and command help entries.
--
-- The database help system is authoritative. This migration replaces stale
-- Necromancer help, adds the missing feature topics, and separates the
-- animatedead daily command from the animate dead corpse spell. It is safe to
-- run repeatedly.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('class-necromancer', 'NECROMANCER PRESTIGE CLASS

Necromancers master undeath while continuing one chosen spellcasting class.
The class has ten locked levels and costs 5,000 account points to unlock.

Prerequisites:
  Arcana 5
  Religion 5
  Any fourth-circle spellcasting
  Any non-good alignment

Class progression:
  Hit Die: d6
  Base Attack: low
  Skill trains: four plus Intelligence per level, minimum one
  Spellcasting: choose arcane or divine, then one eligible preferred base class

Eligible arcane classes are Wizard, Sorcerer, Bard, and Summoner. Eligible divine
classes are Cleric, Druid, Ranger, Paladin, and Inquisitor. Each Necromancer level
advances only the selected preferred class. If an old character has no
unambiguous selection, Necromancer-only abilities fall back to Necromancer level.

Features by Necromancer level:
  1  Necromancer Weapons; Undead Cohort
  2  Summon Undead
  3  Ultravision
  4  Light Armor; Bone Armor rank 1
  5  Deathless Vigor; Weapon Focus: Polearms
  6  Undead Graft; Touch of Undeath; Paralyzing Touch
  7  Tough as Bone; Weakening Touch; Weapon Specialization: Polearms;
     one bonus class-feat point
  8  Medium Armor; Bone Armor rank 2; Degenerative Touch
  9  Summon Greater Undead; Destructive Touch
 10  Essence of Undeath; Deathless Touch

Core commands:
  call cohort
  cast ''animate dead'' <corpse>
  cast ''greater animation'' <corpse>
  bonearmor <new item description>
  undeath <target> <paralyze|weaken|degenerate|destroy|death>

See also: UNDEAD-COHORT, ANIMATE-DEAD, GREATER-ANIMATION, BONE-ARMOR,
TOUCH-OF-UNDEATH, TOUGH-AS-BONE, ESSENCE-OF-UNDEATH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('class-necromancer', 'CLASS-NECROMANCER');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('class-necromancer', 'NECROMANCER');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('animate-dead', 'ANIMATE DEAD

Usage:
  cast ''animate dead'' <corpse>

Animate Dead turns a corpse in the room into a permanent charmed undead follower
and transfers the corpse contents to that follower. The corpse is consumed only
after a follower is successfully created.

There is a 10 percent summon failure chance. Holy rooms reject the spell. An
invalid corpse, holy-room rejection, random failure, follower-cap rejection, or
mobile-load failure leaves the corpse in place.

Animated-undead limit:
  Characters without Necromancer levels may control one animated undead.
  Necromancers may control exactly two animated undead in total.

The effective level selects the creature:
  below 10  zombie
  10-19     ghoul
  20-29     giant skeleton
  30+       mummy

At Necromancer level 2, Summon Undead makes this spell at-will. It does not
consume a prepared spell or spontaneous spell slot. Its effective level follows
the selected preferred spellcasting class and applicable bonus caster levels,
with a Necromancer-level fallback for an ambiguous legacy selection.

Deathless Touch can empower the next successful Animate Dead or Greater
Animation follower. Failed or rejected casts do not consume that empowerment.
Use dismiss <follower> to release a follower. Summons are saved through the pet
persistence system; heed any warning that asks you to save again.

See also: CLASS-NECROMANCER, SUMMON-UNDEAD, GREATER-ANIMATION,
TOUCH-OF-UNDEATH, ANIMATEDEAD', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('animate-dead', 'ANIMATE-DEAD');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('animate-dead', 'SPELL-ANIMATE-DEAD');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('animate-dead', 'SUMMON-UNDEAD');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('greater-animation', 'GREATER ANIMATION

Usage:
  cast ''greater animation'' <corpse>

Greater Animation turns a corpse in the room into a permanent charmed undead
follower and transfers the corpse contents to that follower. The corpse is
consumed only after a follower is successfully created.

There is a 10 percent summon failure chance. Holy rooms reject the spell. An
invalid corpse, holy-room rejection, random failure, follower-cap rejection, or
mobile-load failure leaves the corpse in place. Non-Necromancers may control one
animated undead; Necromancers may control exactly two in total.

The effective level selects the creature:
  below 20  ghost
  20-24     spectre
  25-29     banshee
  30+       wight

The final follower level also scales within the effective-level tier instead of
remaining fixed at the prototype level.

At Necromancer level 9, Summon Greater Undead makes this spell at-will. It does
not consume a prepared spell or spontaneous spell slot. Its effective level
follows the selected preferred spellcasting class and applicable bonus caster
levels, with a Necromancer-level fallback for an ambiguous legacy selection.

Deathless Touch can empower the next successful Animate Dead or Greater
Animation follower. Failed or rejected casts do not consume that empowerment.
Use dismiss <follower> to release a follower. Summons are saved through the pet
persistence system; heed any warning that asks you to save again.

See also: CLASS-NECROMANCER, SUMMON-GREATER-UNDEAD, ANIMATE-DEAD,
TOUCH-OF-UNDEATH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('greater-animation', 'GREATER-ANIMATION');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('greater-animation', 'SPELL-GREATER-ANIMATION');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('greater-animation', 'SUMMON-GREATER-UNDEAD');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('touch-of-undeath', 'TOUCH OF UNDEATH

Usage:
  undeath <target> <paralyze|weaken|degenerate|destroy|death>

Touch of Undeath is a swift-action attack against a living target. Peaceful-room,
reach, undead-target, player-killing, missing-target, and invalid-variant checks
do not spend a use or action. Once those checks pass, the valid attempt consumes
one daily use and the swift action whether the touch attack hits or misses.

Daily uses by Necromancer level:
  levels 6-7   one
  levels 8-9   two
  level 10     three

Variants:
  paralyze    Level 6. Fortitude negates paralysis for 1d4+1 rounds.
  weaken      Level 7. -1d6 Strength, no save, for effective level rounds.
  degenerate  Level 8. -2 hit and damage, no save, for effective level rounds.
  destroy     Level 9. -1d6 Constitution, no save, for effective level rounds.
  death       Level 10. Up to 600 lethal damage; Fortitude halves the damage.

Magic resistance can negate an effect after a touch hits. The effect level uses
the selected preferred spellcasting class and applicable bonus caster levels,
with a Necromancer-level fallback for an ambiguous legacy selection.

If Deathless Touch kills the target, the next successful Animate Dead or Greater
Animation follower gains increased attributes. Failed or rejected summons do not
consume that one-shot empowerment.

See also: CLASS-NECROMANCER, ANIMATE-DEAD, GREATER-ANIMATION', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'TOUCH-OF-UNDEATH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'UNDEATH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'PARALYZING-TOUCH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'WEAKENING-TOUCH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'DEGENERATIVE-TOUCH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'DESTRUCTIVE-TOUCH');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('touch-of-undeath', 'DEATHLESS-TOUCH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('bone-armor', 'BONE ARMOR

Usage:
  bonearmor <new item description>

You need Bone Armor, a held crafting kit or a crafting station in the room, and
exactly one armor piece or shield inside the kit or station. The new description
must contain the lowercase word bone. Conversion costs one third of the item
value, changes its material to bone, and returns the item to your inventory while
the crafting action completes.

Bone Armor rank 1 is granted at Necromancer level 4 and rank 2 at level 8. Each
rank reduces spell failure by 10 percentage points when at least one relevant
armor piece is equipped and every equipped body, head, arms, legs, and shield
piece is made of bone. No armor gives no reduction, and any mixed non-bone piece
removes the entire Bone Armor reduction.

Example:
  put breastplate kit
  bonearmor a bone breastplate

See also: CLASS-NECROMANCER, CRAFTING-KIT, SPELL-FAILURE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('bone-armor', 'BONE-ARMOR');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('bone-armor', 'BONEARMOR');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('undead-cohort', 'UNDEAD COHORT

Usage:
  call cohort
  call eidolon

Undead Cohort is granted at Necromancer level 1. It calls an eidolon whose level
is the combined Summoner and Necromancer class levels, capped at 30 and never
above total character level.

Use study to choose the cohort base form, descriptions, and evolutions. A
Necromancer gains one cohort evolution point per Necromancer level. The mandatory
first rank of Undead Appearance is granted for free and does not consume one of
those points. Resistance evolutions apply to the cohort itself.

Calling the cohort starts the shared eidolon cooldown, approximately 14 minutes.
Use dismiss <cohort> to release it and reduce a longer remaining cooldown to
about one minute. The normal pet persistence system saves the active cohort.

See also: CLASS-NECROMANCER, STUDY, EVOLUTIONS, DISMISS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('undead-cohort', 'UNDEAD-COHORT');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('undead-cohort', 'CALL-COHORT');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('undead-cohort', 'COHORT');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('tough-as-bone', 'TOUGH AS BONE

Tough as Bone is granted at Necromancer level 7. It makes the Necromancer immune
to disease and stun. Stun immunity applies to both affect-based stuns and timed
stun events, including combat abilities, traps, spells, poisons, and weapon
effects that use the shared status paths.

See also: CLASS-NECROMANCER, ESSENCE-OF-UNDEATH', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('tough-as-bone', 'TOUGH-AS-BONE');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('essence-of-undeath', 'ESSENCE OF UNDEATH

Essence of Undeath is granted at Necromancer level 10. It provides undead-like
immunity to poison damage and poison effects, sleep effects, death magic,
paralysis, sneak attacks, critical hits, and physical ability drain applied by
magic.

This feat does not change the character race to undead and does not by itself
grant every trait of an undead mobile.

See also: CLASS-NECROMANCER, TOUGH-AS-BONE', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('essence-of-undeath', 'ESSENCE-OF-UNDEATH');

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('animatedead', 'ANIMATEDEAD COMMAND

Usage:
  animatedead

This is a separate daily class ability, not the Animate Dead corpse spell. It
takes no argument, uses a standard action, and does not target or consume a
corpse. You gain one daily use per rank of the Animate Dead feat.

The command creates a charmed undead follower from composite caster level:
  below 10  zombie
  10-19     ghoul
  20-29     giant skeleton
  30+       mummy

Holy rooms and the animated-undead follower cap can reject the command. To cast
the corpse spell instead, use cast ''animate dead'' <corpse>.

See also: ANIMATE-DEAD, CLASS-NECROMANCER', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE help_tag = 'animatedead' AND LOWER(keyword) IN ('animate', 'animate-dead');
INSERT IGNORE INTO help_keywords (help_tag, keyword)
VALUES ('animatedead', 'ANIMATEDEAD');

COMMIT;
