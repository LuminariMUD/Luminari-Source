-- Vessel System Phase 24 rollback.
-- Drops the waypoint and route creators. Pre-S12 code lets any captain delete
-- or extend any waypoint or route that no hull depends on.

ALTER TABLE ship_waypoints
DROP COLUMN IF EXISTS creator_id;

ALTER TABLE ship_routes
DROP COLUMN IF EXISTS creator_id;
