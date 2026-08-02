-- Vessel System Phase 17 rollback.
-- This permanently removes saved paint and figurehead descriptions.

ALTER TABLE ship_interiors
  DROP COLUMN IF EXISTS paint_scheme,
  DROP COLUMN IF EXISTS figurehead;
