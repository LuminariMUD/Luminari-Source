-- Vessel System Phase 25 rollback.
-- Drops the settlement records. A settlement still open is no longer undone:
-- its ship side stands whether or not its gold was saved. Pre-S14 code writes
-- no settlement, and S14 code creates the table again at boot.

DROP TABLE IF EXISTS vessel_settlements;
