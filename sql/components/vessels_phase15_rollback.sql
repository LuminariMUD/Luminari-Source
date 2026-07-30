-- Vessel System Phase 15 rollback.
-- Retire active bounty-hunter hulls before applying this rollback.

DROP TABLE IF EXISTS vessel_bounty_hunts;
DROP TABLE IF EXISTS vessel_hunter_encounters;
