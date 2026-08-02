-- Vessel System Phase 16 verification.

SELECT COUNT(*) AS vessel_phase16_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN (
     'vessel_showcase_events',
     'vessel_event_participants',
     'vessel_event_leaderboards',
     'vessel_event_runtimes'
   );

SELECT COUNT(*) AS vessel_event_core_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND (
     (TABLE_NAME = 'vessel_showcase_events' AND COLUMN_NAME IN (
       'event_id', 'event_type', 'status', 'staff_idnum', 'start_x', 'start_y',
       'finish_x', 'finish_y', 'started_at', 'ended_at', 'end_reason'
     ))
     OR (TABLE_NAME = 'vessel_event_participants' AND COLUMN_NAME IN (
       'event_id', 'ship_id', 'player_idnum', 'team', 'score',
       'finish_seconds', 'placement', 'status'
     ))
     OR (TABLE_NAME = 'vessel_event_leaderboards' AND COLUMN_NAME IN (
       'event_type', 'player_idnum', 'entries', 'wins', 'points',
       'best_time_seconds'
     ))
     OR (TABLE_NAME = 'vessel_event_runtimes' AND COLUMN_NAME IN (
       'ship_id', 'event_id', 'role', 'ordinal_num'
     ))
   );

SELECT event_id, event_type, status, staff_idnum, started_at, ended_at,
       end_reason
  FROM vessel_showcase_events
 WHERE event_type NOT IN ('regatta', 'skirmish', 'ghost')
    OR status NOT IN (
      'active', 'spawning', 'completed', 'cancelled', 'recovered',
      'recovery_failed'
    )
    OR started_at <= 0
    OR (status IN ('completed', 'cancelled', 'recovered') AND ended_at <= 0);

SELECT participant.event_id, participant.ship_id, participant.player_idnum,
       participant.team, participant.score, participant.finish_seconds,
       participant.placement, participant.status
  FROM vessel_event_participants AS participant
  LEFT JOIN vessel_showcase_events AS event
    ON event.event_id = participant.event_id
 WHERE event.event_id IS NULL
    OR participant.team NOT IN ('none', 'red', 'blue')
    OR participant.status NOT IN ('active', 'finished', 'withdrawn')
    OR participant.score < 0
    OR participant.finish_seconds < 0
    OR participant.placement < 0;

SELECT event_type, player_idnum, entries, wins, points, best_time_seconds
  FROM vessel_event_leaderboards
 WHERE event_type NOT IN ('regatta', 'skirmish', 'ghost')
    OR player_idnum <= 0
    OR entries < 0
    OR wins < 0
    OR wins > entries
    OR points < 0
    OR best_time_seconds <= 0;

SELECT runtime.ship_id, runtime.event_id, runtime.role, runtime.ordinal_num,
       event.status AS event_status
  FROM vessel_event_runtimes AS runtime
  LEFT JOIN vessel_showcase_events AS event
    ON event.event_id = runtime.event_id
  LEFT JOIN ship_runtime_state AS ship
    ON ship.ship_id = runtime.ship_id
 WHERE event.event_id IS NULL
    OR ship.ship_id IS NULL
    OR event.status NOT IN ('active', 'spawning', 'recovery_failed')
    OR runtime.role <> 'ghost'
    OR runtime.ordinal_num NOT BETWEEN 1 AND 5;
