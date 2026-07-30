-- Vessel System Phase 14 verification.

SELECT COUNT(*) AS vessel_phase14_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN (
     'vessel_npc_merchants',
     'vessel_merchant_consequences'
   );

SELECT COUNT(*) AS vessel_merchant_definition_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'vessel_npc_merchants'
   AND COLUMN_NAME IN (
     'merchant_id',
     'faction_id',
     'prototype_id',
     'route_id',
     'pilot_mob_vnum',
     'cargo_commodity_id',
     'active_ship_id',
     'next_respawn_at',
     'generation',
     'last_attacked_at'
   );

SELECT merchant_id, name, faction_id, prototype_id, route_id,
       pilot_mob_vnum, cargo_commodity_id, cargo_quantity,
       schedule_interval_hours, respawn_delay_seconds
  FROM vessel_npc_merchants
 WHERE faction_id NOT BETWEEN 0 AND 3
    OR prototype_id <= 0
    OR route_id <= 0
    OR pilot_mob_vnum <= 0
    OR cargo_commodity_id <= 0
    OR cargo_quantity <= 0
    OR schedule_interval_hours NOT BETWEEN 1 AND 24
    OR respawn_delay_seconds NOT BETWEEN 1 AND 604800;

SELECT merchant.merchant_id, merchant.name, merchant.active_ship_id
  FROM vessel_npc_merchants AS merchant
  LEFT JOIN ship_runtime_state AS runtime
    ON runtime.ship_id = merchant.active_ship_id
 WHERE merchant.active_ship_id IS NOT NULL
   AND runtime.ship_id IS NULL;

SELECT consequence_id, merchant_id, generation, player_name, event_type,
       standing_penalty, bounty_delta, status
  FROM vessel_merchant_consequences
 WHERE status NOT IN ('pending', 'applied', 'void')
    OR standing_penalty < 0
    OR bounty_delta < 0;
