-- Vessel System Phase 12 rollback.
-- Existing schedules remain active but stop collecting passenger fares.

ALTER TABLE ship_schedules
  DROP COLUMN IF EXISTS passenger_fare;
