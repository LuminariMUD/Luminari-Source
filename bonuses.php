<?php
/**
 * bonuses.php - LuminariMUD Item Bonus Cross-Reference Matrix Tool
 *
 * SECURITY NOTICE:
 * This tool contains sensitive game data and should be protected with authentication.
 * Ensure proper access controls are in place before deploying to production.
 */

// Security: Start session for authentication
session_start();

// Security: Basic authentication check
// TODO: Implement proper authentication system
if (!isset($_SESSION['authenticated']) || $_SESSION['authenticated'] !== true) {
    // For now, we'll allow access but log it
    error_log("Unauthenticated access to bonuses.php from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
    // Uncomment the following lines to require authentication:
    // http_response_code(401);
    // die("Authentication required. Please contact administrator.");
}

/**
 * bonuses.php - LuminariMUD Item Bonus Cross-Reference Matrix Tool
 * 
 * PURPOSE:
 * This tool provides a comprehensive overview of item bonuses across all wear slots
 * in the MUD. It creates a matrix showing which bonus types are available in which
 * equipment slots, helping game designers identify balance issues and gaps in itemization.
 * 
 * FUNCTIONALITY:
 * - Displays a matrix with wear slots as columns and bonus types as rows
 * - Shows counts of how many items provide each bonus in each slot
 * - Provides totals for each bonus type and each wear slot
 * - Each wear slot links to bonus_breakdown.php for detailed analysis
 * 
 * DATABASE SCHEMA REQUIREMENTS:
 * This tool expects the following tables and columns:
 * 
 * 1. object_database_items
 *    - idnum (INT): Unique item identifier
 * 
 * 2. object_database_wear_slots
 *    - object_idnum (INT): Foreign key to object_database_items.idnum
 *    - worn_slot (VARCHAR): The slot where item can be worn
 * 
 * 3. object_database_bonuses
 *    - object_idnum (INT): Foreign key to object_database_items.idnum
 *    - bonus_location (VARCHAR): Type of bonus (e.g., "Strength", "Max-HP")
 * 
 * USAGE:
 * - Access this file directly via web browser
 * - Click on any wear slot header to see detailed breakdown for that slot
 * - Use the matrix to identify:
 *   * Which slots lack certain bonuses
 *   * Which bonuses are over/under-represented
 *   * Itemization patterns across the game
 * 
 * MAINTENANCE NOTES:
 * - The wear slots and bonus types lists must match the MUD's internal systems
 * - Database credentials must be configured before deployment
 * - Consider caching results for large databases to improve performance
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
        }
        table tr td:hover
        {
                background-color: #111;
        }
        a
        {
            color: white;
        }
        a:hover
        {
            color: #f5b501;
        }

    </style>
</head>
<body>
<?php

/* ===========================================================================
 * Configuration Section
 * Update these values with your database credentials
 * ===========================================================================*/

// Security: Enable error reporting only in development
if (defined('DEVELOPMENT_MODE') && DEVELOPMENT_MODE) {
    error_reporting(E_ALL);
    ini_set('display_errors', 1);
} else {
    error_reporting(0);
    ini_set('display_errors', 0);
    ini_set('log_errors', 1);
}

// Database connection parameters - Use environment variables for security
$servername = $_ENV['DB_HOST'] ?? getenv('DB_HOST') ?? "localhost";
$username = $_ENV['DB_USER'] ?? getenv('DB_USER') ?? "";
$password = $_ENV['DB_PASS'] ?? getenv('DB_PASS') ?? "";
$database = $_ENV['DB_NAME'] ?? getenv('DB_NAME') ?? "";

// Validate database credentials are provided
if (empty($servername) || empty($username) || empty($password) || empty($database)) {
    error_log("Database credentials not properly configured");
    die("Database configuration error. Please contact administrator.");
}

// Establish PDO connection to MySQL with security options
// PDO is used for better security and prepared statement support
try {
    $pdo = new PDO(
        "mysql:host=$servername;dbname=$database;charset=utf8mb4",
        $username,
        $password,
        [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
            PDO::MYSQL_ATTR_FOUND_ROWS => true
        ]
    );
} catch (PDOException $e) {
    error_log("Database connection failed: " . $e->getMessage());
    die("Database connection error. Please contact administrator.");
}

/* ===========================================================================
 * Wear Slot Configuration
 * ===========================================================================*/

/**
 * Master list of all equipment wear slots in the MUD
 * These will be displayed as column headers (X-axis) in the matrix
 * 
 * The order here determines the display order in the table
 * Common slots (like Finger, Neck) are listed first for easier reference
 * 
 * Each slot name must match exactly with the worn_slot values in the database
 */
$wear_slots = [
    // Primary equipment slots
    "Finger", "Neck", "Body", "Head", "Legs", "Feet", "Hands", "Arms", "Shield",
    
    // Secondary equipment slots
    "About-Body",   // Cloaks, capes, etc.
    "Waist",        // Belts, sashes
    "Wrist",        // Bracers, bracelets
    
    // Weapon/held slots
    "Wield",        // Primary weapon
    "Hold",         // Held items (orbs, books, etc.)
    
    // Additional slots
    "Face",         // Masks, veils
    "Ammo-Pouch",   // Ammunition containers
    "Ears",         // Earrings
    "Eyes",         // Goggles, spectacles
    "Badge",        // Insignias, medals
    "Instrument",   // Musical instruments (bard equipment)
    "Shoulders",    // Pauldrons, shoulder guards
    "Ankle"         // Anklets, ankle guards
];

/* ===========================================================================
 * Bonus Type Configuration
 * ===========================================================================*/

/**
 * Master list of all possible bonus types in the MUD system
 * These will be displayed as row headers (Y-axis) in the matrix
 * 
 * Categories are arranged logically:
 * - Primary attributes (STR, DEX, etc.)
 * - Derived stats (HP, Move, PSP)
 * - Combat stats (Hitroll, Damroll)
 * - Saves and resistances
 * - Special abilities and modifiers
 * 
 * Each bonus type must match exactly with bonus_location values in the database
 */
$bonus_types = [
    // Primary Attributes (D&D/Pathfinder style)
    "Strength", "Dexterity", "Intelligence", "Wisdom", "Constitution", "Charisma",
    
    // Derived Statistics
    "Max-PSP",      // Psionic Spell Points
    "Max-HP",       // Hit Points
    "Max-Move",     // Movement Points
    
    // Combat Statistics
    "Hitroll",      // Attack bonus
    "Damroll",      // Damage bonus
    
    // Saving Throws
    "Save-Fortitude", "Save-Reflex", "Save-Will",
    
    // Defense
    "Spell-Resist", "Armor-Class",
    
    // Elemental Resistances
    "Resist-Fire", "Resist-Cold", "Resist-Air", "Resist-Earth", "Resist-Acid",
    "Resist-Holy", "Resist-Electric", "Resist-Unholy",
    
    // Physical Damage Resistances
    "Resist-Slashing", "Resist-Piercing", "Resist-Bludgeoning", "Resist-Sound",
    
    // Status/Special Resistances
    "Resist-Poison", "Resist-Disease", "Resist-Negative", "Resist-Illusion",
    "Resist-Mental", "Resist-Light", "Resist-Energy", "Resist-Water",
    
    // Special Abilities
    "Grant-Feat",    // Grants specific feats
    "Skill-Bonus",   // Skill check bonuses
    "Power-Resist",  // Psionic resistance
    
    // Regeneration
    "HP-Regen", "MV-Regen", "PSP-Regen",
    
    // Miscellaneous
    "Encumbrance",   // Carrying capacity
    "Fast-Healing",  // Healing rate
    "Initiative",    // Combat initiative
    
    // Spell Slots (by circle/level)
    "Spell-Circle-1", "Spell-Circle-2", "Spell-Circle-3", "Spell-Circle-4",
    "Spell-Circle-5", "Spell-Circle-6", "Spell-Circle-7", "Spell-Circle-8",
    "Spell-Circle-9",
    
    // Spell Modifiers
    "Spell-Potency",     // Spell power
    "Spell-DC",          // Save difficulty
    "Spell-Duration",    // Effect duration
    "Spell-Penetration"  // SR penetration
];

/* ===========================================================================
 * Data Structure Initialization
 * ===========================================================================*/

/**
 * Initialize the data structures that will hold our analysis results
 * 
 * Three main data structures:
 * 1. $matrix - 2D array holding counts for each slot/bonus combination
 * 2. $row_totals - Total unique items per wear slot (bottom row)
 * 3. $col_totals - Total bonus occurrences per type (second column)
 */
$matrix    = [];  // matrix[wear_slot][bonus_type] = count
$row_totals = []; // total items per wear slot (for footer)
$col_totals = []; // total occurrences per bonus type (for first column)

// Initialize all matrix cells to 0 for proper display
foreach ($wear_slots as $slot) {
    $row_totals[$slot] = 0;
    foreach ($bonus_types as $bonus) {
        $matrix[$slot][$bonus] = 0;
        if (!isset($col_totals[$bonus])) {
            $col_totals[$bonus] = 0;
        }
    }
}

/* ===========================================================================
 * Database Queries
 * ===========================================================================*/

/**
 * Query 1: Count total unique items per wear slot
 * 
 * This gives us the total number of distinct items that can be worn in each slot
 * Used for the bottom "Total Items" row in the matrix
 * 
 * Note: An item might have multiple bonuses, but we count it only once per slot
 */
$sql = "
  SELECT worn_slot, COUNT(DISTINCT object_idnum) AS total
    FROM object_database_wear_slots
   GROUP BY worn_slot
";

// Execute query and populate row totals
try {
    foreach ($pdo->query($sql)->fetchAll(PDO::FETCH_ASSOC) as $r) {
        $worn_slot = $r['worn_slot'] ?? '';
        $total = (int)($r['total'] ?? 0);

        if (isset($row_totals[$worn_slot])) {
            $row_totals[$worn_slot] = $total;
        }
    }
} catch (PDOException $e) {
    error_log("Database query failed for row totals: " . $e->getMessage());
    http_response_code(500);
    die("Database query error. Please contact administrator.");
}

/**
 * Query 2: Count bonus occurrences per wear slot and bonus type
 * 
 * This is the main data query that populates our matrix
 * It counts how many times each bonus type appears in each wear slot
 * 
 * Important: This counts bonus instances, not unique items
 * If one item has 3 bonuses, it contributes 3 to the count
 * 
 * The JOIN operations:
 * - Links items to their wear slots
 * - Links items to their bonuses
 * - Groups by slot and bonus type for counting
 */
$sql = "
  SELECT ws.worn_slot AS slot,
         b.bonus_location AS bonus,
         COUNT(*) AS cnt
    FROM object_database_items i
    JOIN object_database_wear_slots ws ON i.idnum = ws.object_idnum
    JOIN object_database_bonuses     b  ON i.idnum = b.object_idnum
   GROUP BY ws.worn_slot, b.bonus_location
";

// Execute query and populate the matrix
try {
    foreach ($pdo->query($sql)->fetchAll(PDO::FETCH_ASSOC) as $r) {
        $s = $r['slot'] ?? '';
        $b = $r['bonus'] ?? '';
        $cnt = (int)($r['cnt'] ?? 0);

        // Security: Validate slot and bonus against our whitelists
        if (!in_array($s, $wear_slots, true)) {
            error_log("Unexpected wear slot in database: " . $s);
            continue;
        }

        if (!in_array($b, $bonus_types, true)) {
            error_log("Unexpected bonus type in database: " . $b);
            continue;
        }

        // Only update if both slot and bonus exist in our configuration
        if (isset($matrix[$s][$b])) {
            $matrix[$s][$b] = $cnt;
            $col_totals[$b] += $cnt;  // Update column total
        }
    }
} catch (PDOException $e) {
    error_log("Database query failed for matrix data: " . $e->getMessage());
    http_response_code(500);
    die("Database query error. Please contact administrator.");
}

/* ===========================================================================
 * HTML Table Generation
 * ===========================================================================*/

/**
 * Generate the cross-reference matrix table
 * 
 * Table structure:
 * - First row: Headers with clickable wear slot links
 * - Data rows: One per bonus type showing distribution
 * - Last row: Total items per slot
 * 
 * Visual features:
 * - Hover effects on cells
 * - Clickable slot headers link to detailed breakdowns
 * - Empty cells shown as blank for cleaner appearance
 */

// Start the main data table
echo "<table border='1' cellpadding='4' cellspacing='0'>";

/**
 * Generate header row
 * 
 * Columns:
 * 1. "Bonus Type" - Row labels
 * 2. "Total" - Sum across all slots
 * 3. Individual wear slots - Each links to detailed breakdown
 */
echo "<tr>";
echo "<th>Bonus Type</th>";
echo "<th>Total</th>";
foreach ($wear_slots as $slot) {
    // Each slot header links to bonus_breakdown.php for detailed analysis
    // Security: URL encode the slot parameter and escape HTML output
    $encoded_slot = urlencode($slot);
    $escaped_slot = htmlspecialchars($slot, ENT_QUOTES | ENT_HTML5, 'UTF-8');
    echo "<th><a href=\"/objectdb/bonus_breakdown.php?slot={$encoded_slot}\" target=\"_blank\">{$escaped_slot}</a></th>";
}
echo "</tr>";

/**
 * Generate data rows - one per bonus type
 * 
 * For each bonus:
 * - Show the bonus name
 * - Show total occurrences across all slots
 * - Show count for each individual slot
 */
foreach ($bonus_types as $bonus) {
    echo "<tr>";
    
    // Bonus type name (row header)
    echo "<td><strong>{$bonus}</strong></td>";
    
    // Total occurrences of this bonus across all slots
    echo "<td><strong>" . ($col_totals[$bonus] ?: '') . "</strong></td>";
    
    // Count for each wear slot (blank if zero)
    foreach ($wear_slots as $slot) {
        $val = $matrix[$slot][$bonus];
        echo "<td>" . ($val ? $val : '') . "</td>";
    }
    echo "</tr>";
}

/**
 * Generate footer row showing total items per slot
 * 
 * This helps identify which slots have more/fewer items overall
 * Useful for identifying slots that might need more itemization
 */
echo "<tr>";
echo "<td><strong>Total Items</strong></td>";
echo "<td></td>"; // Empty cell under "Total" column
foreach ($wear_slots as $slot) {
    echo "<td><strong>" . ($row_totals[$slot] ?: '') . "</strong></td>";
}
echo "</tr>";

// Close the table
echo "</table>";

/**
 * Usage tips for game designers:
 * 
 * 1. Identify gaps: Look for empty cells where bonuses might be needed
 * 2. Check balance: Compare totals across slots to ensure even distribution
 * 3. Spot patterns: See which bonuses are common vs. rare
 * 4. Plan itemization: Use data to guide new item creation
 * 
 * Click any slot header to see level-based distribution of bonuses
 */
?>
  </body>
  </html>