-- Vessel System Phase 22 rollback.
-- Drops the raider tier table. Pre-S6 code launches no raiders. Roll back the
-- raider content first, until it keeps no tier rows: a raider still at sea
-- keeps hers until a restart retires her.

DROP TABLE IF EXISTS vessel_raider_tiers;
