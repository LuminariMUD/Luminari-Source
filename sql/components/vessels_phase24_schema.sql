-- Vessel System Phase 24: study step S12, owned waypoints and routes
-- (docs/adr/0003-durismud-naval-model.md; GitLab work item #11).
-- Mirrors the runtime DDL in vessel_ownership_ensure_schema()
-- (src/vessels/vessels_ownership.c).

-- The player who set a waypoint or created a route (player_data.player_idnum).
-- Only that player or the staff may change or delete it. 0 is no player:
-- content, and rows made before S12, which only the staff may change.
ALTER TABLE ship_waypoints
ADD COLUMN IF NOT EXISTS creator_id INT UNSIGNED NOT NULL DEFAULT 0;

ALTER TABLE ship_routes
ADD COLUMN IF NOT EXISTS creator_id INT UNSIGNED NOT NULL DEFAULT 0;
