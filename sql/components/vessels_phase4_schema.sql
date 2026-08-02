-- Vessel System Phase 04: ship prototype table (vedit)
-- Mirrors the auto-creation in src/vessels/vessels_edit.c (vedit_ensure_table).
-- Apply manually only when auto-creation is not desired.

CREATE TABLE IF NOT EXISTS ship_prototypes (
  prototype_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(127) NOT NULL,
  vessel_class INT NOT NULL DEFAULT 2,
  max_speed INT NOT NULL DEFAULT 10,
  armor INT NOT NULL DEFAULT 10,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
