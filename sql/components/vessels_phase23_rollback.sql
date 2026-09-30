-- Vessel System Phase 23 rollback.
-- Drops hull renown and the contraband flag. Roll back the contraband content
-- first: pre-S7 code would trade its goods as lawful ones at every port.

ALTER TABLE ship_runtime_state
DROP COLUMN IF EXISTS renown;

ALTER TABLE trade_commodities
DROP COLUMN IF EXISTS contraband_renown;
