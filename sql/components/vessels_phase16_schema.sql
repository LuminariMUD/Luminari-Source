-- Vessel System Phase 16: staff showcase events and durable leaderboards.
--
-- Event history and participant results survive restarts. Ghost-fleet hulls
-- are tracked separately so boot recovery can retire every temporary runtime
-- before another event opens.

CREATE TABLE IF NOT EXISTS vessel_showcase_events (
  event_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  event_type VARCHAR(16) NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'active',
  staff_idnum BIGINT NOT NULL DEFAULT 0,
  start_x INT NOT NULL DEFAULT 0,
  start_y INT NOT NULL DEFAULT 0,
  finish_x INT NOT NULL DEFAULT 0,
  finish_y INT NOT NULL DEFAULT 0,
  started_at BIGINT NOT NULL DEFAULT 0,
  ended_at BIGINT NOT NULL DEFAULT 0,
  end_reason VARCHAR(127) NOT NULL DEFAULT '',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_vessel_showcase_status (status, event_type),
  INDEX idx_vessel_showcase_staff (staff_idnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_event_participants (
  event_id BIGINT UNSIGNED NOT NULL,
  ship_id INT NOT NULL,
  player_idnum BIGINT NOT NULL DEFAULT 0,
  team VARCHAR(8) NOT NULL DEFAULT 'none',
  score INT NOT NULL DEFAULT 0,
  finish_seconds INT NOT NULL DEFAULT 0,
  placement INT NOT NULL DEFAULT 0,
  status VARCHAR(16) NOT NULL DEFAULT 'active',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (event_id, ship_id),
  INDEX idx_vessel_event_player (player_idnum, event_id),
  CONSTRAINT fk_vessel_event_participant_event FOREIGN KEY (event_id)
    REFERENCES vessel_showcase_events(event_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_event_leaderboards (
  event_type VARCHAR(16) NOT NULL,
  player_idnum BIGINT NOT NULL,
  entries INT NOT NULL DEFAULT 0,
  wins INT NOT NULL DEFAULT 0,
  points BIGINT NOT NULL DEFAULT 0,
  best_time_seconds INT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (event_type, player_idnum),
  INDEX idx_vessel_event_rank (event_type, wins, points),
  INDEX idx_vessel_event_player_rank (player_idnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_event_runtimes (
  ship_id INT NOT NULL PRIMARY KEY,
  event_id BIGINT UNSIGNED NOT NULL,
  role VARCHAR(16) NOT NULL DEFAULT 'ghost',
  ordinal_num INT NOT NULL DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  INDEX idx_vessel_event_runtime_event (event_id),
  CONSTRAINT fk_vessel_event_runtime_event FOREIGN KEY (event_id)
    REFERENCES vessel_showcase_events(event_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
