-- Debug query to check resource depletion data at coordinates (0,0)
-- Run this to see the current state of your resource data

SELECT 
    resource_type,
    ROUND(depletion_level * 100, 1) as 'Current %',
    last_harvest,
    ROUND(TIMESTAMPDIFF(MINUTE, last_harvest, NOW()), 1) as 'Minutes Since Harvest',
    ROUND(TIMESTAMPDIFF(MINUTE, last_harvest, NOW()) / 60.0, 1) as 'Hours Since Harvest'
FROM resource_depletion 
WHERE x_coord = 0 AND y_coord = 0 
ORDER BY resource_type;

-- Resource type mapping:
-- 0 = Vegetation, 1 = Minerals, 2 = Water, 3 = Herbs, 4 = Game
-- 5 = Wood, 6 = Stone, 7 = Crystal, 8 = Clay, 9 = Salt
