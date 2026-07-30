-- Vessel System Phase 14: durable NPC merchant shipping.
--
-- Merchant definitions assemble existing builder-authored prototypes, routes,
-- pilot mobiles, commodities, and wilderness coordinates into killable hulls.
-- Consequences provide an auditable, exactly-once faction-standing queue.

CREATE TABLE IF NOT EXISTS vessel_npc_merchants (
  merchant_id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(127) NOT NULL,
  faction_id TINYINT NOT NULL DEFAULT 0,
  prototype_id INT NOT NULL,
  route_id INT NOT NULL,
  pilot_mob_vnum INT NOT NULL,
  spawn_x INT NOT NULL,
  spawn_y INT NOT NULL,
  spawn_z INT NOT NULL DEFAULT 0,
  cargo_commodity_id INT NOT NULL,
  cargo_quantity INT NOT NULL,
  schedule_interval_hours INT NOT NULL DEFAULT 1,
  respawn_delay_seconds INT NOT NULL DEFAULT 3600,
  active_ship_id INT NULL,
  next_respawn_at BIGINT NOT NULL DEFAULT 0,
  generation INT UNSIGNED NOT NULL DEFAULT 0,
  last_spawn_at BIGINT NOT NULL DEFAULT 0,
  last_destroyed_at BIGINT NOT NULL DEFAULT 0,
  last_destroyed_by VARCHAR(63) NOT NULL DEFAULT '',
  last_attacker_name VARCHAR(63) NOT NULL DEFAULT '',
  last_attacked_at BIGINT NOT NULL DEFAULT 0,
  loss_count INT UNSIGNED NOT NULL DEFAULT 0,
  enabled TINYINT NOT NULL DEFAULT 1,
  last_error VARCHAR(255) NOT NULL DEFAULT '',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY uk_vessel_merchant_name (name),
  UNIQUE KEY uk_vessel_merchant_active_ship (active_ship_id),
  INDEX idx_vessel_merchant_last_attacker (last_attacker_name),
  INDEX idx_vessel_merchant_due (enabled, next_respawn_at)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS vessel_merchant_consequences (
  consequence_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  merchant_id INT NOT NULL,
  generation INT UNSIGNED NOT NULL,
  player_name VARCHAR(63) NOT NULL,
  faction_id TINYINT NOT NULL DEFAULT 0,
  standing_penalty INT NOT NULL DEFAULT 0,
  bounty_delta INT NOT NULL DEFAULT 0,
  cargo_units INT NOT NULL DEFAULT 0,
  event_type VARCHAR(15) NOT NULL,
  dedupe_key VARCHAR(191) NULL,
  status VARCHAR(16) NOT NULL DEFAULT 'pending',
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  applied_at TIMESTAMP NULL DEFAULT NULL,
  UNIQUE KEY uk_vessel_merchant_consequence_dedupe (dedupe_key),
  INDEX idx_vessel_merchant_consequence_player (player_name, status),
  INDEX idx_vessel_merchant_consequence_merchant (merchant_id, generation)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
