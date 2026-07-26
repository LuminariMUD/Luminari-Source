-- Vessel System Phase 08 rollback.
-- WARNING: destroys all encounter table definitions. Wilderness regions
-- themselves are untouched (the vessel system does not own them).

DROP TABLE IF EXISTS vessel_encounters;
