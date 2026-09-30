-- Vessel System Phase 20 rollback.
-- Removes the catalogue row and the ammunition from weapon rows. Pre-S4 code
-- reads a description and damage dice that S4 does not write, so a rolled-back
-- weapon fires as that code's unnamed 2d6 default until it is refitted, and
-- equipment rows read as unused slots.

ALTER TABLE ship_weapons
DROP COLUMN IF EXISTS catalog_id,
DROP COLUMN IF EXISTS ammo;
