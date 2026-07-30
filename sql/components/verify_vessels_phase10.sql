-- Vessel System Phase 10 verification.

SELECT COUNT(*) AS required_tables_present
  FROM information_schema.TABLES
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME IN ('ship_weapons', 'vessel_insurance_claims');

SELECT COUNT(*) AS runtime_columns_present
  FROM information_schema.COLUMNS
 WHERE TABLE_SCHEMA = DATABASE()
   AND TABLE_NAME = 'ship_runtime_state'
   AND COLUMN_NAME IN (
     'pvp_grace_until', 'pvp_grace_attacker',
     'dock_fee_balance', 'dock_fee_port', 'dock_fee_clan'
   );

SELECT weapon.ship_id, weapon.slot_index
  FROM ship_weapons weapon
  LEFT JOIN ship_interiors interior ON interior.ship_id = weapon.ship_id
 WHERE interior.ship_id IS NULL
    OR weapon.slot_index >= 10
    OR weapon.slot_type <> 1;

SELECT claim_id, owner, amount, status
  FROM vessel_insurance_claims
 WHERE owner = ''
    OR amount <= 0
    OR status NOT IN ('pending', 'paid', 'void');

SELECT ship_id, dock_fee_balance, dock_fee_port, dock_fee_clan
 FROM ship_runtime_state
 WHERE dock_fee_balance < 0
    OR (dock_fee_balance > 0 AND dock_fee_port <= 0)
    OR (dock_fee_port = 0 AND dock_fee_clan <> 0);

SELECT COUNT(*) AS installed_weapons FROM ship_weapons;
SELECT status, COUNT(*) AS claims
  FROM vessel_insurance_claims
 GROUP BY status
 ORDER BY status;
