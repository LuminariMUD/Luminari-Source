-- Vessel System Phase 10: weapon rows and lifecycle settlement state.

ALTER TABLE ship_runtime_state
  ADD COLUMN IF NOT EXISTS pvp_grace_until BIGINT NOT NULL DEFAULT 0
    AFTER last_attacker,
  ADD COLUMN IF NOT EXISTS pvp_grace_attacker VARCHAR(64) NOT NULL DEFAULT ''
    AFTER pvp_grace_until,
  ADD COLUMN IF NOT EXISTS dock_fee_balance INT NOT NULL DEFAULT 0
    AFTER pvp_grace_attacker,
  ADD COLUMN IF NOT EXISTS dock_fee_port INT NOT NULL DEFAULT 0
    AFTER dock_fee_balance,
  ADD COLUMN IF NOT EXISTS dock_fee_clan INT NOT NULL DEFAULT 0
    AFTER dock_fee_port;

CREATE TABLE IF NOT EXISTS ship_weapons (
  ship_id INT NOT NULL,
  slot_index TINYINT UNSIGNED NOT NULL,
  slot_type TINYINT UNSIGNED NOT NULL DEFAULT 1,
  position TINYINT UNSIGNED NOT NULL DEFAULT 0,
  equipment_weight TINYINT UNSIGNED NOT NULL DEFAULT 0,
  description VARCHAR(255) NOT NULL DEFAULT '',
  val0 SMALLINT NOT NULL DEFAULT 0,
  val1 SMALLINT NOT NULL DEFAULT 0,
  val2 SMALLINT NOT NULL DEFAULT 0,
  val3 SMALLINT NOT NULL DEFAULT 0,
  slot_x TINYINT UNSIGNED NOT NULL DEFAULT 0,
  slot_y TINYINT UNSIGNED NOT NULL DEFAULT 0,
  reload_timer SMALLINT NOT NULL DEFAULT 0,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (ship_id, slot_index),
  CONSTRAINT fk_ship_weapons_interior
    FOREIGN KEY (ship_id) REFERENCES ship_interiors(ship_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_insurance_claims (
  claim_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  ship_id INT NOT NULL,
  owner VARCHAR(64) NOT NULL,
  ship_name VARCHAR(128) NOT NULL,
  amount INT NOT NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'pending',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  paid_at TIMESTAMP NULL DEFAULT NULL,
  INDEX idx_vessel_claim_owner_status (owner, status),
  INDEX idx_vessel_claim_ship (ship_id)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
