/* ========================================================================= */
/* ship_schedules - Timer-based route scheduling for vessel automation       */
/* Part of Phase 01 Session 06: Scheduled Route System                      */
/* ========================================================================= */

CREATE TABLE IF NOT EXISTS ship_schedules (
  schedule_id INT AUTO_INCREMENT PRIMARY KEY,
  ship_id INT NOT NULL,
  route_id INT NOT NULL,
  interval_hours INT NOT NULL,
  next_departure INT NOT NULL DEFAULT 0,
  enabled TINYINT NOT NULL DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

  /* Constraints */
  UNIQUE KEY uk_ship_schedule (ship_id),

  /* Validation: interval must be within bounds (1-24 MUD hours) */
  CONSTRAINT chk_interval CHECK (interval_hours >= 1 AND interval_hours <= 24),

  /* Foreign key to routes table */
  CONSTRAINT fk_schedule_route FOREIGN KEY (route_id)
    REFERENCES ship_routes(route_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

/* Index for efficient schedule lookups during tick processing */
CREATE INDEX idx_schedule_enabled ON ship_schedules(enabled, next_departure);

/* ========================================================================= */
/* End of ship_schedules table definition                                    */
/* ========================================================================= */
