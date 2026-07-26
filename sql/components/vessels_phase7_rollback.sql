-- Vessel System Phase 07 rollback.
-- WARNING: destroys all commodity definitions, port supply state, and
-- bulk cargo aboard every ship.

DELETE FROM ship_cargo_manifest WHERE cargo_room = 0;
DROP TABLE IF EXISTS vessel_bounties;
DROP TABLE IF EXISTS freight_contracts;
DROP TABLE IF EXISTS port_commodities;
DROP TABLE IF EXISTS trade_commodities;
