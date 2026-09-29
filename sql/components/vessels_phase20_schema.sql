-- Vessel System Phase 20: DurisMUD study step S4, weapons and gunnery
-- (docs/ongoing-projects/vessels-ships.md 3.3.4 and 3.3.10). Mirrors the
-- runtime DDL in vessel_persistence_ensure_schema() (src/vessels/vessels_db.c);
-- it requires Phases 10 and 19.

-- Every weapon and equipment slot is a row. catalog_id names the weapon or
-- equipment catalogue row; a weapon row saved before S4 reads 0, and the
-- server converts it at load to the class default weapon with full ammo.
-- ammo is the rounds left in a weapon. The ship_runtime_state.slot_data
-- snapshot is no longer written or read.
ALTER TABLE ship_weapons
ADD COLUMN IF NOT EXISTS catalog_id TINYINT UNSIGNED NOT NULL DEFAULT 0
AFTER weapon_damage,
ADD COLUMN IF NOT EXISTS ammo SMALLINT UNSIGNED NOT NULL DEFAULT 0
AFTER catalog_id;
