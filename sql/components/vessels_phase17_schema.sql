-- Vessel System Phase 17: optional exterior customization.

ALTER TABLE ship_interiors
  ADD COLUMN IF NOT EXISTS figurehead VARCHAR(80) NOT NULL DEFAULT '' AFTER vessel_name,
  ADD COLUMN IF NOT EXISTS paint_scheme VARCHAR(80) NOT NULL DEFAULT '' AFTER figurehead;
