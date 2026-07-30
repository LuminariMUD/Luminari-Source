-- Vessel System Phase 13: wilderness-region piracy law.
--
-- region_vnum points at builder-authored REGION_GEOGRAPHIC data. Geometry
-- stays in region_data/region_index; this table only supplies vessel law.
-- waters_type: 1=territorial waters, 2=free seas, 3=pirate cove.

CREATE TABLE IF NOT EXISTS vessel_region_law (
  region_vnum INT PRIMARY KEY,
  waters_type TINYINT NOT NULL,
  priority INT NOT NULL DEFAULT 0,
  bounty_percent INT NOT NULL DEFAULT 100,
  authority VARCHAR(63) NOT NULL DEFAULT '',
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  INDEX idx_vessel_region_law_priority (priority)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
