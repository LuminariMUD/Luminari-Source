-- Launcher, thrown-weapon, pouch, and projectile-recovery player help.
--
-- The database help system is authoritative. This migration is safe to apply
-- repeatedly and keeps the related command keywords under three focused tags.

START TRANSACTION;

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('THROWN-WEAPONS', 'Thrown Weapons

Usage:
  throw <target>
  throw <target> <direction>

THROW starts ranged combat with a throwable weapon. The first eligible weapon
wielded two-handed, in the primary hand, or in the off hand becomes the anchor.
Weapons marked as thrown are eligible; the Throwing special ability can also
make an otherwise melee weapon eligible. Returning alone does not.

Each attack uses one actual copy of the anchor''s object VNUM. Copies are chosen
in this exact order: the equipped ammo pouch, top-level inventory, then the
wielded anchor itself. Ordinary containers and other equipment slots are not
searched. The wielded copy is unequipped only when all reserves are gone, and
throwing mode ends if that last copy does not return.

Without a direction, the target must be in the same room. With a valid
direction and the required Far Shot or ranger Longshot benefit, THROW uses the
same one-adjacent-room targeting rules as FIRE.

Thrown weapons use ranged accuracy and defenses, but add the thrower''s full
Strength modifier to damage. Rapid Shot and ranger Quick Draw can add throws;
Manyshot remains launcher-only. A Returning weapon comes back after the attack
unless it was destroyed or successfully caught with Snatch Arrows. Use COLLECT
to recover other thrown weapons from the room or corpses.

See also: AMMO, COLLECT, FIRE, QUIVERS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('RANGED-WEAPONS', 'Ammunition, Launchers, and Quivers

Usage:
  fire <target>
  fire <target> <direction>
  collect

FIRE starts a launcher attack with a wielded bow, crossbow, sling, or blowgun.
You must equip an ammo pouch containing a compatible arrow, bolt, stone, or
blowgun dart. Mixed pouches are supported: FIRE scans past incompatible
missiles and throwable weapons for the first compatible missile.

Ammo-pouch capacity is a number of objects, not a weight. A pouch accepts
missiles and eligible transferable throwable weapons, but rejects launchers,
ordinary melee weapons, nested containers, and unrelated items. It uses the
single equipped ammo-pouch slot; containers worn elsewhere are not ammunition
sources.

FIRE is launcher-only. A dart used as a weapon is thrown with THROW; a blowgun
fires ITEM_MISSILE darts with FIRE. Use RELOAD for launchers that require it.
COLLECT recovers owner-tagged missiles and thrown weapons from the room and
corpses.

See also: COLLECT, COMBAT, RELOAD, THROW, THROWN-WEAPONS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

INSERT INTO help_entries (tag, entry, min_level, auto_generated)
VALUES ('COLLECT', 'Collect Projectiles

Usage:
  collect

COLLECT gathers physical projectiles that you fired or threw. It searches the
current room and corpses in that room, and never collects projectiles belonging
to another character.

An equipped ammo pouch receives recovered missiles and eligible throwable
weapons while it has object-count capacity. A throwable weapon falls back to
top-level inventory when the pouch is absent, incompatible, or full and you
can carry it. Launcher ammunition still requires a usable ammo pouch.

See also: AMMO, AUTOCOLLECT, FIRE, QUIVERS, THROW, THROWN-WEAPONS', 0, FALSE)
ON DUPLICATE KEY UPDATE entry = VALUES(entry), min_level = VALUES(min_level),
  auto_generated = VALUES(auto_generated);

DELETE FROM help_keywords
WHERE UPPER(keyword) IN (
  'RETURNING', 'THROW', 'THROWING', 'THROWN', 'THROWN-WEAPON', 'THROWN-WEAPONS',
  'AMMO', 'AMMUNITION', 'ARCHERY', 'BLAST', 'BOWS', 'FIRE', 'FIRE-WEAPONS',
  'MISSILES', 'QUIVER', 'QUIVERS', 'RANGED-WEAPONS', 'SHOOT', 'COLLECT'
);

INSERT IGNORE INTO help_keywords (help_tag, keyword) VALUES
  ('THROWN-WEAPONS', 'RETURNING'),
  ('THROWN-WEAPONS', 'THROW'),
  ('THROWN-WEAPONS', 'THROWING'),
  ('THROWN-WEAPONS', 'THROWN'),
  ('THROWN-WEAPONS', 'THROWN-WEAPON'),
  ('THROWN-WEAPONS', 'THROWN-WEAPONS'),
  ('RANGED-WEAPONS', 'AMMO'),
  ('RANGED-WEAPONS', 'AMMUNITION'),
  ('RANGED-WEAPONS', 'ARCHERY'),
  ('eldritch-blast', 'BLAST'),
  ('RANGED-WEAPONS', 'BOWS'),
  ('RANGED-WEAPONS', 'FIRE'),
  ('RANGED-WEAPONS', 'FIRE-WEAPONS'),
  ('RANGED-WEAPONS', 'MISSILES'),
  ('RANGED-WEAPONS', 'QUIVER'),
  ('RANGED-WEAPONS', 'QUIVERS'),
  ('RANGED-WEAPONS', 'RANGED-WEAPONS'),
  ('RANGED-WEAPONS', 'SHOOT'),
  ('COLLECT', 'COLLECT');

/* Preserve the superseded ammunition article without competing command keywords. */
INSERT IGNORE INTO help_keywords (help_tag, keyword)
SELECT tag, 'LEGACY-RANGED-WEAPONS' FROM help_entries WHERE tag = 'blast';

COMMIT;
