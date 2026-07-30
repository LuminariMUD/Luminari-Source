-- Vessel System Phase 14 rollback.
-- Runtime merchant hulls must be retired before applying this rollback.

DROP TABLE IF EXISTS vessel_merchant_consequences;
DROP TABLE IF EXISTS vessel_npc_merchants;
