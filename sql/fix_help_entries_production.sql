-- Migration script to fix help_entries table on production server
-- This adds the missing max_level column first, then auto_generated column
-- Run this on the production database to fix the error:
-- "Unknown column 'max_level' in 'help_entries'"

-- Step 1: Add max_level column if it doesn't exist
ALTER TABLE help_entries 
ADD COLUMN IF NOT EXISTS max_level INT DEFAULT 1000 
COMMENT 'Maximum level required to see this help entry'
AFTER min_level;

-- Step 2: Add auto_generated column if it doesn't exist
-- This must come after max_level is added
ALTER TABLE help_entries 
ADD COLUMN IF NOT EXISTS auto_generated BOOLEAN DEFAULT FALSE 
COMMENT 'TRUE if auto-generated, FALSE if manual'
AFTER max_level;

-- Step 3: Add index on auto_generated column for performance
ALTER TABLE help_entries 
ADD INDEX IF NOT EXISTS idx_auto_generated (auto_generated);

-- Step 4: Verify the table structure
-- Uncomment to check the final structure
-- DESCRIBE help_entries;

-- Step 5: Show any auto-generated entries (should be empty initially)
-- SELECT COUNT(*) as auto_generated_count FROM help_entries WHERE auto_generated = TRUE;