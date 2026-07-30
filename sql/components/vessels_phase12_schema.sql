-- Vessel System Phase 12: persistent public passenger fares.

ALTER TABLE ship_schedules
  ADD COLUMN IF NOT EXISTS passenger_fare INT NOT NULL DEFAULT 0
    AFTER enabled;
