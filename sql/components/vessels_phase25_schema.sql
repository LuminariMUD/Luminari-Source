-- Vessel System Phase 25: study step S14, two-phase vessel settlements
-- (docs/ongoing-projects/vessels-ships.md, Phase 14; GitLab work item #12).
-- Mirrors the runtime DDL in vessel_settlement_ensure_schema()
-- (src/vessels/vessels_settlement.c).

-- A cargo trade, a freight acceptance or a dock-fee payment whose ship side
-- is recorded and whose gold may not yet be in the captain's player file. The
-- row is written in the ship-side transaction and deleted once the file is
-- saved with its id (the file's VSet line). A row left behind is settled from
-- that file: deleted when the file names it, and otherwise undone with the
-- values it carries. The unique keys keep one open settlement for a ship and
-- one for a player, so neither opens another account before it is settled.
CREATE TABLE IF NOT EXISTS vessel_settlements (
  settlement_id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  -- player_data.player_idnum of the captain; 0 for a mob, which has no file.
  player_id INT UNSIGNED NOT NULL,
  ship_id INT NOT NULL,
  -- The undo of a trade or of a freight acceptance: supply_delta is added to
  -- the port's supply of the commodity and cargo_delta to the hold's lot.
  port_vnum INT NOT NULL DEFAULT 0,
  commodity_id INT NOT NULL DEFAULT 0,
  supply_delta INT NOT NULL DEFAULT 0,
  cargo_delta INT NOT NULL DEFAULT 0,
  -- The freight contract to put back on the board.
  contract_id INT NOT NULL DEFAULT 0,
  -- The dock fee owed again, at the port and to the clan it was assessed for.
  fee_amount INT NOT NULL DEFAULT 0,
  fee_port INT NOT NULL DEFAULT 0,
  fee_clan INT NOT NULL DEFAULT 0,
  created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  UNIQUE KEY uk_vessel_settlement_ship (ship_id),
  UNIQUE KEY uk_vessel_settlement_player (player_id)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4;
