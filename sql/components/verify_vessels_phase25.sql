-- Vessel System Phase 25 verification.

SELECT COUNT(*) AS phase25_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'vessel_settlements'
  AND IS_NULLABLE = 'NO'
  AND COLUMN_NAME IN (
    'settlement_id',
    'player_id',
    'ship_id',
    'port_vnum',
    'commodity_id',
    'supply_delta',
    'cargo_delta',
    'contract_id',
    'fee_amount',
    'fee_port',
    'fee_clan',
    'created_at'
  );

SELECT COUNT(*) AS phase25_unique_keys_present
FROM information_schema.STATISTICS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND TABLE_NAME = 'vessel_settlements'
  AND NON_UNIQUE = 0
  AND INDEX_NAME IN ('uk_vessel_settlement_ship', 'uk_vessel_settlement_player');
