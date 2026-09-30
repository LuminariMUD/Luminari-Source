-- Vessel System Phase 22 rollback.
-- Drops the raider tier table. Pre-S6 code launches no raiders; roll back the
-- raider content first to remove its prototypes.

DROP TABLE IF EXISTS vessel_raider_tiers;
