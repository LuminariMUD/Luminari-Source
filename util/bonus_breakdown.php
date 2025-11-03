<?php
/**
 * bonus_breakdown.php - LuminariMUD Item Bonus Analysis Tool (Wear Slot Breakdown)
 *
 * SECURITY NOTICE:
 * This tool contains sensitive game data and should be protected with authentication.
 * Ensure proper access controls are in place before deploying to production.
 * 
 * @see ../documentation/PHP_TOOLS_README.md for comprehensive security audit,
 *      deployment guide, and security best practices for all PHP tools.
 */

// Define tool identifier for security
define('LUMINARI_TOOLS', true);

// Include shared configuration and utilities
require_once 'config.php';

// Security: Authentication check for data analysis tools
if (!SecurityConfig::isAuthenticated(['developer', 'admin', 'data_analyst'])) {
    ErrorHandler::authenticationError('bonus_breakdown.php');
}

/**
 * bonus_breakdown.php - LuminariMUD Item Bonus Analysis Tool (Wear Slot Breakdown)
 * 
 * PURPOSE:
 * This tool provides a detailed breakdown of item bonuses for a specific wear slot,
 * showing how bonuses are distributed across different level ranges. It's used by
 * game designers to analyze game balance and item distribution patterns.
 * 
 * FUNCTIONALITY:
 * - Accepts a wear slot parameter (e.g., "Finger", "Neck", "Body")
 * - Queries the MUD database for all items that can be worn in that slot
 * - Groups items by level ranges (1-5, 6-10, 11-15, etc.)
 * - Displays a matrix showing which bonuses appear at which level ranges
 * - Helps identify gaps or oversaturation in item bonuses
 * 
 * DATABASE SCHEMA REQUIREMENTS:
 * This tool expects the following tables and columns:
 * 
 * 1. object_database_items
 *    - idnum (INT): Unique item identifier
 *    - minimum_level (INT): Minimum level required to use the item
 * 
 * 2. object_database_wear_slots
 *    - object_idnum (INT): Foreign key to object_database_items.idnum
 *    - worn_slot (VARCHAR): The slot where item can be worn (e.g., "Finger", "Neck")
 * 
 * 3. object_database_bonuses
 *    - object_idnum (INT): Foreign key to object_database_items.idnum
 *    - bonus_location (VARCHAR): Type of bonus (e.g., "Strength", "Max-HP")
 *    - bonus_value (INT): The numerical bonus value (not used in this query)
 * 
 * INTEGRATION:
 * - This file is called from bonuses.php via hyperlinks on each wear slot
 * - The wear slot parameter is passed via GET request
 * - Output is a standalone HTML page showing the detailed breakdown
 * 
 * USAGE:
 * bonus_breakdown.php?slot=Finger
 * bonus_breakdown.php?slot=Body
 * 
 * MAINTENANCE NOTES:
 * - The bonus types list must match the MUD's internal bonus system
 * - Level ranges are hardcoded as 5-level buckets (can be adjusted)
 * - Database credentials must be configured before deployment
 */
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="robots" content="noindex, nofollow">
    <title>Chronicles of Krynn Bonuses Cross-Reference</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background-color: #333;
            margin: 20px;
            color: white;
        }
        table
        {
            width: 100%;
            border-collapse: collapse;
            margin-bottom: 20px;
            border: 1px solid #f5b501;
        }
        table tr td
        {
                text-align: center;
                top: 20px;
        }
        table tr td:hover
        {
                background-color: #111;
        }
        table tr th
        {
                background-color: black;
                position: sticky;
                top: 0;
        }

    </style>
</head>
<body>
<?php

/* ===========================================================================
 * Configuration Section
 * Update these values with your database credentials
 * ===========================================================================*/

// Get database connection using shared manager
try {
    $pdo = DatabaseManager::getConnection();
} catch (Exception $e) {
    ErrorHandler::databaseError($e);
}

/* ===========================================================================
 * Input Validation and Parameter Processing
 * ===========================================================================*/

/**
 * Validate and sanitize the wear-slot parameter
 *
 * Security measures:
 * - Check parameter exists
 * - Validate against whitelist of allowed slots
 * - Limit length to prevent buffer overflow
 * - Sanitize for safe database usage
 */

// Define allowed wear slots (whitelist for security)
$allowed_slots = [
    "Finger", "Neck", "Body", "Head", "Legs", "Feet", "Hands", "Arms", "Shield",
    "About-Body", "Waist", "Wrist", "Wield", "Hold", "Face", "Ammo-Pouch",
    "Ears", "Eyes", "Badge", "Instrument", "Shoulders", "Ankle"
];

// Read and validate the wear-slot parameter from the query string
if (empty($_GET['slot'])) {
    ErrorHandler::validationError("missing ?slot=Wear-SlotName parameter");
}

$slot = InputValidator::validateWhitelist(trim($_GET['slot']), $allowed_slots);
if ($slot === false) {
    ErrorHandler::validationError("Invalid slot parameter. Allowed values: " . implode(", ", $allowed_slots));
}

/* ===========================================================================
 * Bonus Type Configuration
 * ===========================================================================*/

/**
 * Master list of all possible bonus types in the MUD system
 * These must match exactly with the bonus_location values in the database
 * 
 * Categories:
 * - Basic Attributes: Strength through Charisma
 * - Derived Stats: Max-PSP, Max-HP, Max-Move
 * - Combat Stats: Hitroll, Damroll
 * - Saving Throws: Save-Fortitude, Save-Reflex, Save-Will
 * - Resistances: Various damage type resistances
 * - Special: Grant-Feat, Skill-Bonus, etc.
 * - Regeneration: HP-Regen, MV-Regen, PSP-Regen
 * - Spell-related: Spell circles and spell modifiers
 */
$bonus_types = [
    // Primary Attributes (D&D/Pathfinder style)
    "Strength","Dexterity","Intelligence","Wisdom","Constitution","Charisma",
    
    // Derived Statistics
    "Max-PSP",      // Psionic Spell Points
    "Max-HP",       // Hit Points
    "Max-Move",     // Movement Points
    
    // Combat Statistics
    "Hitroll",      // Attack bonus
    "Damroll",      // Damage bonus
    
    // Saving Throws (D&D 3.5/Pathfinder style)
    "Save-Fortitude",   // Physical resistance
    "Save-Reflex",      // Dodge/agility
    "Save-Will",        // Mental resistance
    
    // Defense and Resistances
    "Spell-Resist",     // Magic resistance
    "Armor-Class",      // Physical defense
    
    // Elemental Resistances
    "Resist-Fire","Resist-Cold","Resist-Air","Resist-Earth","Resist-Acid",
    "Resist-Holy","Resist-Electric","Resist-Unholy",
    
    // Physical Damage Type Resistances
    "Resist-Slashing","Resist-Piercing","Resist-Bludgeoning","Resist-Sound",
    
    // Status/Special Resistances
    "Resist-Poison","Resist-Disease","Resist-Negative","Resist-Illusion",
    "Resist-Mental","Resist-Light","Resist-Energy","Resist-Water",
    
    // Special Bonuses
    "Grant-Feat",       // Grants specific feats
    "Skill-Bonus",      // Bonus to skill checks
    "Power-Resist",     // Psionic resistance
    
    // Regeneration and Recovery
    "HP-Regen",         // Hit point regeneration rate
    "MV-Regen",         // Movement regeneration rate
    "PSP-Regen",        // PSP regeneration rate
    "Encumbrance",      // Carrying capacity modifier
    "Fast-Healing",     // Accelerated healing
    "Initiative",       // Combat initiative bonus
    
    // Spell Slot Bonuses (by spell level/circle)
    "Spell-Circle-1","Spell-Circle-2","Spell-Circle-3","Spell-Circle-4",
    "Spell-Circle-5","Spell-Circle-6","Spell-Circle-7","Spell-Circle-8",
    "Spell-Circle-9",
    
    // Spell Power Modifiers
    "Spell-Potency",    // Spell damage/effect modifier
    "Spell-DC",         // Spell difficulty class modifier
    "Spell-Duration",   // Spell duration modifier
    "Spell-Penetration" // Spell resistance penetration
];

/* ===========================================================================
 * Level Bucket Configuration
 * ===========================================================================*/

/**
 * Build level buckets for grouping items
 * Currently uses 5-level ranges (1-5, 6-10, 11-15, etc.)
 * This can be adjusted by changing the increment value in the loop
 * 
 * The buckets help identify at which level ranges certain bonuses become
 * available or common, which is crucial for game balance analysis
 */
$buckets = [];
for ($min = 1; $min <= 30; $min += 5) {
    $max = min(30, $min + 4);
    $buckets[] = [
        'min'   => $min,        // Minimum level in this bucket
        'max'   => $max,        // Maximum level in this bucket
        'label' => "{$min}-{$max}"  // Display label for the bucket
    ];
}

/* ===========================================================================
 * Data Structure Initialization
 * ===========================================================================*/

/**
 * Initialize the data matrix that will hold our results
 * 
 * $matrix structure:
 * - First dimension: bonus type (e.g., "Strength", "Max-HP")
 * - Second dimension: bucket index (0 = levels 1-5, 1 = levels 6-10, etc.)
 * - Value: count of items with that bonus in that level range
 * 
 * $row_totals structure:
 * - Key: bonus type
 * - Value: total count of items with that bonus across all level ranges
 */
$matrix    = [];  // matrix[bonus][bucket_idx] = count
$row_totals = []; // grand total per bonus

// Initialize all cells to 0 to ensure proper display even for empty cells
foreach ($bonus_types as $bonus) {
    $row_totals[$bonus] = 0;
    $matrix[$bonus] = array_fill(0, count($buckets), 0);
}

/* ===========================================================================
 * Database Query and Data Collection with Caching
 * ===========================================================================*/

// Check cache first for performance
$cache_key = "bonus_breakdown_" . $slot;
$cached_data = CacheManager::get($cache_key);

if ($cached_data !== false) {
    // Use cached data
    $matrix = $cached_data['matrix'];
    $row_totals = $cached_data['row_totals'];
} else {
    /**
     * Main query to collect item bonus distribution data
 * 
 * This query performs the following operations:
 * 1. Joins three tables to connect items, wear slots, and bonuses
 * 2. Filters by the specified wear slot
 * 3. Groups items into level buckets using FLOOR division
 * 4. Counts how many items have each bonus type in each level range
 * 
 * The bucket_idx calculation: FLOOR((minimum_level - 1) / 5)
 * - Level 1-5: bucket_idx = 0
 * - Level 6-10: bucket_idx = 1
 * - Level 11-15: bucket_idx = 2
 * - etc.
 * 
 * Security: Uses prepared statements to prevent SQL injection
 */
$sql = "
  SELECT
    b.bonus_location AS bonus,
    FLOOR((i.minimum_level - 1) / 5) AS bucket_idx,
    COUNT(*) AS cnt
  FROM object_database_items i
  JOIN object_database_wear_slots ws
    ON i.idnum = ws.object_idnum
  JOIN object_database_bonuses b
    ON i.idnum = b.object_idnum
  WHERE ws.worn_slot = :slot
    AND i.minimum_level BETWEEN 1 AND 30
  GROUP BY b.bonus_location, bucket_idx
";

// Prepare and execute the query with the wear slot parameter
try {
    $stmt = $pdo->prepare($sql);
    $stmt->execute([':slot' => $slot]);
} catch (PDOException $e) {
    error_log("Database query failed in bonus_breakdown.php: " . $e->getMessage());
    http_response_code(500);
    die("Database query error. Please contact administrator.");
}

/**
 * Process query results and populate the data matrix
 *
 * For each row returned:
 * - Extract the bonus type, bucket index, and count
 * - Update the corresponding cell in our matrix
 * - Update the running total for that bonus type
 *
 * Note: We only update if the bonus exists in our predefined list
 * to avoid issues with unexpected database values
 */
try {
    while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
        $bonus = $row['bonus'] ?? '';
        $idx = (int)($row['bucket_idx'] ?? 0);
        $count = (int)($row['cnt'] ?? 0);

        // Security: Validate bonus type against our whitelist
        if (!in_array($bonus, $bonus_types, true)) {
            error_log("Unexpected bonus type in database: " . $bonus);
            continue;
        }

        // Security: Validate bucket index is within expected range
        if ($idx < 0 || $idx >= count($buckets)) {
            error_log("Invalid bucket index: " . $idx);
            continue;
        }

        // Validate that this bonus type exists in our configuration
        if (isset($matrix[$bonus][$idx])) {
            $matrix[$bonus][$idx] = $count;
            $row_totals[$bonus] += $count;
        }
    }
} catch (PDOException $e) {
    error_log("Error processing query results: " . $e->getMessage());
    http_response_code(500);
    die("Data processing error. Please contact administrator.");
}

    // Cache the results for future requests (cache for 30 minutes)
    CacheManager::set($cache_key, [
        'matrix' => $matrix,
        'row_totals' => $row_totals
    ], 1800);
}

/* ===========================================================================
 * HTML Output Generation
 * ===========================================================================*/

/**
 * Generate the HTML table displaying the results
 * 
 * Table structure:
 * - Header row: "Bonus Type" | "Total" | Level bucket columns
 * - Data rows: One per bonus type, showing counts in each bucket
 * - Empty cells are displayed as blank (not 0) for better readability
 */

// Page title showing which wear slot we're analyzing
echo "<h2>Breakdown for Wear Slot: <em>" . HTMLHelper::escape($slot) . "</em></h2>";

// Start the data table
echo "<table border='1' cellpadding='4' cellspacing='0'>";

// Generate table header row
echo "<tr><th>Bonus Type</th><th>Total</th>";
foreach ($buckets as $b) {
    echo "<th>{$b['label']}</th>";
}
echo "</tr>";

/**
 * Generate data rows
 * 
 * For each bonus type:
 * 1. Display the bonus name in the first column
 * 2. Display the total count across all levels in the second column
 * 3. Display counts for each level bucket in subsequent columns
 * 
 * Empty values are displayed as blank strings for cleaner appearance
 */
foreach ($bonus_types as $bonus) {
    echo "<tr>";
    
    // Bonus type name
    echo "<td><strong>{$bonus}</strong></td>";
    
    // Total count for this bonus (blank if zero)
    echo "<td><strong>" . ($row_totals[$bonus] ?: '') . "</strong></td>";

    // Count for each level bucket (blank if zero)
    foreach ($matrix[$bonus] as $cnt) {
        echo "<td>" . ($cnt ?: '') . "</td>";
    }

    echo "</tr>";
}

// Close the table
echo "</table>";

/**
 * Additional notes for developers:
 * 
 * This table helps identify:
 * - Which bonuses are rare or common at different level ranges
 * - Gaps where certain bonuses might be needed
 * - Oversaturation of specific bonuses at certain levels
 * - Progression patterns (e.g., do spell bonuses appear only at higher levels?)
 * 
 * The data can inform decisions about:
 * - New item creation
 * - Game balance adjustments
 * - Loot table modifications
 */
?>
</body>
</html>