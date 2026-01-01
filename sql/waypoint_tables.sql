/* ========================================================================= */
/* Waypoint and Route Management Tables                                      */
/* Part of LuminariMUD Vessel Autopilot System                               */
/* Phase 01 - Session 02: Waypoint/Route Management                          */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/* ship_waypoints - Individual navigation points                             */
/* Stores coordinates, name, tolerance and timing for navigation waypoints   */
/* ------------------------------------------------------------------------- */
CREATE TABLE IF NOT EXISTS ship_waypoints (
  waypoint_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) DEFAULT '',
  x FLOAT NOT NULL,
  y FLOAT NOT NULL,
  z FLOAT NOT NULL DEFAULT 0,
  tolerance FLOAT NOT NULL DEFAULT 5.0,
  wait_time INT NOT NULL DEFAULT 0,
  flags INT NOT NULL DEFAULT 0,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_waypoint_name (name),
  INDEX idx_waypoint_coords (x, y, z)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

/* ------------------------------------------------------------------------- */
/* ship_routes - Named collections of waypoints                              */
/* Routes organize waypoints into ordered sequences for autopilot navigation */
/* ------------------------------------------------------------------------- */
CREATE TABLE IF NOT EXISTS ship_routes (
  route_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(64) NOT NULL,
  loop_route TINYINT(1) NOT NULL DEFAULT 0,
  active TINYINT(1) NOT NULL DEFAULT 1,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_route_name (name),
  INDEX idx_route_active (active)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

/* ------------------------------------------------------------------------- */
/* ship_route_waypoints - Route-waypoint associations with ordering          */
/* Links waypoints to routes with sequence numbers for ordered navigation    */
/* Foreign keys ensure referential integrity with cascade deletes            */
/* ------------------------------------------------------------------------- */
CREATE TABLE IF NOT EXISTS ship_route_waypoints (
  id INT AUTO_INCREMENT PRIMARY KEY,
  route_id INT NOT NULL,
  waypoint_id INT NOT NULL,
  sequence_num INT NOT NULL,
  FOREIGN KEY (route_id) REFERENCES ship_routes(route_id) ON DELETE CASCADE,
  FOREIGN KEY (waypoint_id) REFERENCES ship_waypoints(waypoint_id) ON DELETE CASCADE,
  UNIQUE KEY route_sequence (route_id, sequence_num),
  INDEX idx_route_waypoint (route_id, waypoint_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
