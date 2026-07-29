-- Vessel System Phase 10 rollback.
-- WARNING: destroys normalized weapons, queued settlements, and logout grace.

DROP TABLE IF EXISTS ship_weapons;
DROP TABLE IF EXISTS vessel_insurance_claims;

ALTER TABLE ship_runtime_state
  DROP COLUMN IF EXISTS dock_fee_clan,
  DROP COLUMN IF EXISTS dock_fee_port,
  DROP COLUMN IF EXISTS dock_fee_balance,
  DROP COLUMN IF EXISTS pvp_grace_attacker,
  DROP COLUMN IF EXISTS pvp_grace_until;
