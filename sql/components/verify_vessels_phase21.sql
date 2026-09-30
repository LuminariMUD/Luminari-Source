-- Vessel System Phase 21 verification.

SELECT COUNT(*) AS phase21_columns_present
FROM information_schema.COLUMNS
WHERE
  TABLE_SCHEMA = DATABASE()
  AND IS_NULLABLE = 'NO'
  AND (
    (TABLE_NAME = 'ship_crew_roster' AND COLUMN_NAME = 'experience')
    OR (
      TABLE_NAME = 'ship_runtime_state'
      AND COLUMN_NAME IN ('stowed', 'wreck_hull', 'summon_due')
    )
  );

SELECT COUNT(*) AS unrefunded_policies
FROM ship_interiors
WHERE insured_for > 0;

SELECT
  ship_id,
  stowed,
  summon_due
FROM ship_runtime_state
WHERE summon_due > 0 AND stowed = 0;
