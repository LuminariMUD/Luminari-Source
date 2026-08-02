-- Vessel System Phase 16 rollback.
-- End or recover an active event and retire all ghost hulls before applying.

DROP TABLE IF EXISTS vessel_event_runtimes;
DROP TABLE IF EXISTS vessel_event_participants;
DROP TABLE IF EXISTS vessel_event_leaderboards;
DROP TABLE IF EXISTS vessel_showcase_events;
