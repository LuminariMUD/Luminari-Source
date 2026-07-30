-- Vessel System Phase 15: durable HUNTED-state bounty-hunter warships.
--
-- A normal vessel_encounters row still supplies shared REGION_ENCOUNTER
-- geography, class/depth filtering, chance, and player-facing messages. The
-- extension supplies the public warship and pursuit policy. One lifecycle row
-- per target prevents duplicate hunters across restarts and fleet-slot reuse.

CREATE TABLE IF NOT EXISTS vessel_hunter_encounters (
  encounter_id INT NOT NULL PRIMARY KEY,
  prototype_id INT NOT NULL,
  pilot_mob_vnum INT NOT NULL,
  min_bounty INT NOT NULL DEFAULT 2000,
  pursuit_speed INT NOT NULL DEFAULT 5,
  hunt_duration_seconds INT NOT NULL DEFAULT 600,
  target_grace_seconds INT NOT NULL DEFAULT 60,
  cooldown_seconds INT NOT NULL DEFAULT 3600,
  enabled TINYINT NOT NULL DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_vessel_hunter_enabled (enabled)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_bounty_hunts (
  target_player VARCHAR(63) NOT NULL PRIMARY KEY,
  encounter_id INT NOT NULL,
  target_ship_id INT NOT NULL DEFAULT 0,
  hunter_ship_id INT NULL,
  hunter_name VARCHAR(127) NOT NULL DEFAULT '',
  generation BIGINT UNSIGNED NOT NULL DEFAULT 0,
  status VARCHAR(15) NOT NULL DEFAULT 'cooldown',
  started_at BIGINT NOT NULL DEFAULT 0,
  expires_at BIGINT NOT NULL DEFAULT 0,
  next_eligible_at BIGINT NOT NULL DEFAULT 0,
  ended_at BIGINT NOT NULL DEFAULT 0,
  end_reason VARCHAR(63) NOT NULL DEFAULT '',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_vessel_bounty_hunter_ship (hunter_ship_id),
  INDEX idx_vessel_bounty_hunt_due (status, next_eligible_at),
  INDEX idx_vessel_bounty_hunt_encounter (encounter_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
