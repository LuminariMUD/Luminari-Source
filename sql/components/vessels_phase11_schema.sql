-- Vessel System Phase 11: persistent DG attachments for generated interiors.

CREATE TABLE IF NOT EXISTS ship_room_template_triggers (
  room_type VARCHAR(50) NOT NULL,
  vessel_type INT NOT NULL DEFAULT 0,
  trigger_vnum INT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (room_type, vessel_type, trigger_vnum),
  INDEX idx_ship_room_trigger_vnum (trigger_vnum)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
