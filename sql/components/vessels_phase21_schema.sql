-- Vessel System Phase 21: DurisMUD study step S5, crew, repair and loss
-- (docs/ongoing-projects/vessels-ships.md 3.3.5-3.3.7 and 3.3.10). Mirrors the
-- runtime DDL in vessel_persistence_ensure_schema() (src/vessels/vessels_db.c);
-- it requires Phases 6 and 10.

-- Each hired crew position's experience, in Duris skill points. Crew hired
-- before S5 read 0 and start at the floor of their tier.
ALTER TABLE ship_crew_roster
ADD COLUMN IF NOT EXISTS experience DOUBLE NOT NULL DEFAULT 0
AFTER loyalty_rating;
