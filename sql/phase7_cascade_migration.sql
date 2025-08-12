-- Phase 7 Cascade System Database Migration
-- This script adds cascade effect tracking to the resource depletion system

-- Add cascade_effects column to track ecological impacts
ALTER TABLE resource_depletion 
ADD COLUMN IF NOT EXISTS cascade_effects TEXT 
COMMENT 'Tracks cascade effects applied to this resource location';

-- Create index for improved performance on cascade effect queries
CREATE INDEX IF NOT EXISTS idx_resource_depletion_cascade 
ON resource_depletion (zone_vnum, x_coord, y_coord, resource_type);

-- Add ecosystem health tracking (for future Phase 8 expansion)
ALTER TABLE resource_depletion 
ADD COLUMN IF NOT EXISTS ecosystem_health FLOAT DEFAULT 1.0 
COMMENT 'Overall ecosystem health for this location (0.0-1.0)';

-- Migration complete message
SELECT 'Phase 7 cascade system database migration completed successfully' AS status;
