-- Vessel System Phase 09 rollback.
-- WARNING: destroys live vessel snapshots and schedule definitions.

DROP TABLE IF EXISTS ship_schedules;
DROP TABLE IF EXISTS ship_runtime_state;
