-- Vessel System Phase 04 rollback: remove the ship prototype table.
-- WARNING: destroys all builder-authored vessel prototypes.

DROP TABLE IF EXISTS ship_prototypes;
