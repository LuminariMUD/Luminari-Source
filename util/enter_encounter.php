<?php
/**
 * enter_encounter.php - LuminariMUD Random Encounter Generator Tool
 *
 * SECURITY NOTICE:
 * This tool generates C code and should be heavily restricted.
 * Only authorized developers should have access to this tool.
 * 
 * @see ../documentation/PHP_TOOLS_README.md for comprehensive security audit,
 *      deployment guide, and security best practices for all PHP tools.
 */

// Security: Start session for authentication and CSRF protection
session_start();

// Security: Strict authentication check for code generation tools
if (!isset($_SESSION['authenticated']) || $_SESSION['authenticated'] !== true ||
    !isset($_SESSION['role']) || $_SESSION['role'] !== 'developer') {
    error_log("Unauthorized access attempt to enter_encounter.php from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
    http_response_code(403);
    die("Access denied. This tool requires developer authentication.");
}

// Security: Generate CSRF token if not exists
if (!isset($_SESSION['csrf_token'])) {
    $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
}

/**
 * enter_encounter.php - LuminariMUD Random Encounter Generator Tool
 * 
 * PURPOSE:
 * This tool generates C code for random encounter definitions in the LuminariMUD
 * encounter system. It provides a web form interface for game designers to create
 * new encounter types without manually writing code.
 * 
 * FUNCTIONALITY:
 * - Accepts encounter parameters through a web form
 * - Validates and processes input data
 * - Generates properly formatted C code for encounters.c
 * - Handles encounter groups (multiple mobs in one encounter)
 * - Supports terrain-based encounter spawning
 * 
 * ENCOUNTER SYSTEM OVERVIEW:
 * Random encounters in LuminariMUD spawn dynamically based on:
 * - Player level range
 * - Terrain type (forest, mountains, underdark, etc.)
 * - Encounter probability tables
 * - Group composition (single or multiple mobs)
 * 
 * GENERATED CODE FORMAT:
 * The tool generates three types of C code:
 * 1. add_encounter_record() - Main encounter definition
 * 2. set_encounter_description() - Look description
 * 3. add_encounter_sector() - Terrain assignments
 * 
 * INTEGRATION:
 * Generated code should be added to:
 * - encounters.c (function calls)
 * - encounters.h (macro definitions)
 * 
 * SECURITY NOTES:
 * - Input sanitization is performed on all text fields
 * - Quotes are escaped to prevent code injection
 * - Database credentials should never be exposed
 */

// Security: Enable error reporting only in development
if (defined('DEVELOPMENT_MODE') && DEVELOPMENT_MODE) {
    error_reporting(E_ALL);
    ini_set('display_errors', 1);
} else {
    error_reporting(0);
    ini_set('display_errors', 0);
    ini_set('log_errors', 1);
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta name="robots" content="noindex, nofollow">
<title>LuminariMUD Encounter Creator</title>
<link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/css/bootstrap.min.css" integrity="sha384-Gn5384xqQ1aoWXA+058RXPxPg6fy4IWvTNh0E263XmFcJlSAwiGgFAW/dAiS6JXm" crossorigin="anonymous">
<script src="https://code.jquery.com/jquery-3.5.1.min.js" integrity="sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0=" crossorigin="anonymous"></script>
<script src="https://code.jquery.com/ui/1.12.1/jquery-ui.min.js" integrity="sha256-VazP97ZCwtekAsvgPBSUwPFKdrwD3unUfSGVYrahUqU=" crossorigin="anonymous"></script>
<style>
  body {
    background-color: #333333;
    color: #e69710;
  }
</style>
<script>
    $( document ).ready(function() {
        $(function () {
            $('[data-toggle="tooltip"]').tooltip()
        });
    });
</script>
</head>
<body>
<?php
/* ===========================================================================
 * Security Functions and Input Validation
 * ===========================================================================*/

/**
 * Validate and sanitize input for C code generation
 *
 * @param string $input The input to validate
 * @param string $type The type of input (identifier, text, number)
 * @param int $max_length Maximum allowed length
 * @return string|false Sanitized input or false if invalid
 */
function validateInput($input, $type, $max_length = 255) {
    if (empty($input) || strlen($input) > $max_length) {
        return false;
    }

    switch ($type) {
        case 'identifier':
            // Only allow alphanumeric and underscores for C identifiers
            if (!preg_match('/^[a-zA-Z_][a-zA-Z0-9_]*$/', $input)) {
                return false;
            }
            break;
        case 'text':
            // Remove potentially dangerous characters
            $input = preg_replace('/[<>"\'\\\]/', '', $input);
            break;
        case 'number':
            if (!is_numeric($input) || $input < 0 || $input > 999999) {
                return false;
            }
            $input = (int)$input;
            break;
    }

    return $input;
}

/**
 * Validate CSRF token
 */
function validateCSRF() {
    if (!isset($_POST['csrf_token']) || !isset($_SESSION['csrf_token']) ||
        !hash_equals($_SESSION['csrf_token'], $_POST['csrf_token'])) {
        error_log("CSRF token validation failed");
        http_response_code(403);
        die("CSRF token validation failed. Please refresh and try again.");
    }
}

/* ===========================================================================
 * Form Processing - Convert Form Data to C Code
 * ===========================================================================*/

if ($_POST)
{
    // Security: Validate CSRF token first
    validateCSRF();
    /**
     * Process form submission and generate C code
     *
     * Data processing steps:
     * 1. Validate and sanitize all input values
     * 2. Convert spaces to underscores for C macros
     * 3. Add appropriate prefixes for constants
     * 4. Escape special characters in descriptions
     */

    // Validate and sanitize all inputs
    $encounter_record_raw = validateInput($_POST['encounter_record'] ?? '', 'identifier', 50);
    if ($encounter_record_raw === false) {
        http_response_code(400);
        die("Invalid encounter record name. Use only letters, numbers, and underscores.");
    }

    // Generate unique encounter identifier
    // Format: ENCOUNTER_TYPE_NAME_HERE
    $encounter_record = "ENCOUNTER_TYPE_" . str_replace(" ", "_", strtoupper($encounter_record_raw));

    // Validate encounter type against whitelist
    $allowed_encounter_types = ['ENCOUNTER_CLASS_COMBAT'];
    $encounter_type = $_POST['encounter_type'] ?? '';
    if (!in_array($encounter_type, $allowed_encounter_types, true)) {
        http_response_code(400);
        die("Invalid encounter type.");
    }

    // Validate level ranges
    $min_level = validateInput($_POST['min_level'] ?? '', 'number');
    $max_level = validateInput($_POST['max_level'] ?? '', 'number');
    if ($min_level === false || $max_level === false || $min_level > $max_level) {
        http_response_code(400);
        die("Invalid level range. Must be numbers with min <= max.");
    }
    
    // Validate encounter group
    $encounter_group_raw = validateInput($_POST['encounter_group'] ?? '', 'identifier', 50);
    if ($encounter_group_raw === false) {
        http_response_code(400);
        die("Invalid encounter group name. Use only letters, numbers, and underscores.");
    }
    $encounter_group = "ENCOUNTER_GROUP_TYPE_" . str_replace(" ", "_", strtoupper($encounter_group_raw));

    // Validate object name
    $object_name = validateInput($_POST['object_name'] ?? '', 'text', 100);
    if ($object_name === false) {
        http_response_code(400);
        die("Invalid object name. Maximum 100 characters, no special characters.");
    }

    // Validate load chance (0-100)
    $load_chance = validateInput($_POST['load_chance'] ?? '', 'number');
    if ($load_chance === false || $load_chance > 100) {
        http_response_code(400);
        die("Invalid load chance. Must be a number between 0 and 100.");
    }

    // Validate number ranges
    $min_number = validateInput($_POST['min_number'] ?? '', 'number');
    $max_number = validateInput($_POST['max_number'] ?? '', 'number');
    if ($min_number === false || $max_number === false || $min_number > $max_number) {
        http_response_code(400);
        die("Invalid number range. Must be numbers with min <= max.");
    }

    // Validate encounter strength against whitelist
    $allowed_strengths = ['ENCOUNTER_STRENGTH_NORMAL', 'ENCOUNTER_STRENGTH_BOSS'];
    $encounter_strength = $_POST['encounter_strength'] ?? '';
    if (!in_array($encounter_strength, $allowed_strengths, true)) {
        http_response_code(400);
        die("Invalid encounter strength.");
    }

    // Validate class against whitelist
    $allowed_classes = ['CLASS_WIZARD', 'CLASS_CLERIC', 'CLASS_ROGUE', 'CLASS_WARRIOR',
                       'CLASS_MONK', 'CLASS_DRUID', 'CLASS_BERSERKER', 'CLASS_SORCERER',
                       'CLASS_PALADIN', 'CLASS_RANGER', 'CLASS_BARD', 'CLASS_ALCHEMIST'];
    $class = $_POST['class'] ?? '';
    if (!in_array($class, $allowed_classes, true)) {
        http_response_code(400);
        die("Invalid class selection.");
    }
    
    // Extra treasure table reference
    $treasure_table = $_POST['treasure_table'];
    
    // Mob characteristics
    $alignment = $_POST['alignment'];
    $race_type = $_POST['race_type'];
    $subrace1 = $_POST['subrace1'];
    $subrace2 = $_POST['subrace2'];
    $subrace3 = $_POST['subrace3'];
    $size = $_POST['size'];
    
    // Behavior flags
    $hostile = $_POST['hostile'];      // Can players leave without fighting?
    $sentient = $_POST['sentient'];    // Can be negotiated with?
    
    // Descriptions
    $long_desc = $_POST['long_desc'];   // Room description
    $desc = $_POST['description'];      // Look at mob description

    /**
     * Generate C code for encounter definition
     * 
     * Function: add_encounter_record()
     * Parameters:
     * - encounter_id: Unique identifier
     * - type: ENCOUNTER_CLASS_COMBAT, etc.
     * - min/max_level: Level range
     * - group: Group identifier for multi-mob encounters
     * - name: Mob name
     * - load_chance: Probability (0-100)
     * - min/max_number: Spawn count range
     * - treasure: Treasure table reference
     * - class: Mob class (warrior, wizard, etc.)
     * - strength: Normal or boss
     * - alignment: Alignment constant
     * - race_type: Primary race
     * - subraces: Up to 3 subrace modifiers
     * - hostile: Combat requirement flag
     * - sentient: Negotiation possibility flag
     * - size: Size category
     */
    $output = "    add_encounter_record(".$encounter_record.", ".$encounter_type.", ".$min_level.", ".$max_level.", ".$encounter_group.", \"".$object_name."\", ".
               $load_chance.", ".$min_number.", ".$max_number.", \n      ".$treasure_table.
              ", ".$class.", ".$encounter_strength.", ".$alignment.", ".$race_type.", \n".
            "      ".$subrace1.", ".$subrace2.", ".$subrace3.", ".$hostile.", ".$sentient.", ".$size." );\n";
    
    // Generate description setter calls
    // Note: Double quotes are converted to single quotes to avoid C string issues
    $output .= "    set_encounter_description(".$encounter_record.", \"".addslashes(str_replace("\"", "'", $desc))."\");\n";
    $output .= "    set_encounter_long_description(".$encounter_record.", \"".addslashes(str_replace("\"", "'", $long_desc))."\");\n";

    /**
     * Process terrain selections
     * 
     * Terrain assignment methods:
     * - set_encounter_terrain_any(): All terrains
     * - set_encounter_terrain_all_surface(): All surface world terrains
     * - set_encounter_terrain_all_underdark(): All underdark terrains
     * - set_encounter_terrain_all_roads(): All road types
     * - set_encounter_terrain_all_water(): All water types
     * - add_encounter_sector(): Specific terrain types
     * 
     * Code formatting: 2 terrain assignments per line for readability
     */
    $i = 0;
    foreach ($_POST['terrain'] as $key)
    {
        // Check for special terrain groupings first
        if ($key == "SECT_ALL")
            $output .= "    set_encounter_terrain_any(".$encounter_record.");\n";
        else if ($key == "SECT_ALL_SURFACE")
            $output .= "    set_encounter_terrain_all_surface(".$encounter_record.");";
        else if ($key == "SECT_ALL_UD")
            $output .= "    set_encounter_terrain_all_underdark(".$encounter_record.");";
        else if ($key == "SECT_ROADS")  // Fixed: was checking SECT_ALL_SURFACE twice
            $output .= "    set_encounter_terrain_all_roads(".$encounter_record.");";
        else if ($key == "SECT_ALL_WATER")
            $output .= "    set_encounter_terrain_all_water(".$encounter_record.");";
        else
            // Individual terrain type
            $output .= "    add_encounter_sector(".$encounter_record.", ".$key.");";
        
        // Format output: 2 entries per line
        if (($i % 2) == 1)
            $output .= "\n";
        else
            $output .= "  ";
        $i++;
    }
    $output .= "\n";
    
    // Add macro definitions for encounters.h
    $output .= "#define ".$encounter_record."\n";
    $output .= "#define ".$encounter_group."\n";
?>
<script>
/**
 * Copy generated code to clipboard
 * 
 * This function:
 * 1. Selects all text in the output textarea
 * 2. Copies it to the system clipboard
 * 3. Shows confirmation alert
 * 
 * Browser compatibility: Works in all modern browsers
 * Mobile support: setSelectionRange ensures mobile compatibility
 */
function copyCode() {
  /* Get the text field */
  var copyText = document.getElementById("codeOutput");

  /* Select the text field */
  copyText.select();
  copyText.setSelectionRange(0, 99999); /*For mobile devices*/

  /* Copy the text inside the text field */
  document.execCommand("copy");

  /* Alert the copied text */
  alert("Code Copied to Clipboard");
}
</script>
<div class="container" style="max-width: 1300px !important;">
    <div class="row">
        <div class="col-sm-12">
            <!-- Generated C code output textarea -->
            <textarea name="codeOutput" id="codeOutput" class="w-100" style="height: 400px;"><?=htmlspecialchars($output, ENT_QUOTES | ENT_HTML5, 'UTF-8')?></textarea>
        </div>
    </div>
    <div class="row">
        <div><br /></div>
        <div class="col-sm-12"><button class="btn btn-success w-100" onclick="copyCode()">Copy Code to Clipboard</button></div>
    </div>
    <div class="row">
        <div><br /></div>
        <div class="col-sm-12"><button class="btn btn-warning w-100" onclick="window.location='/util/enter_encounter.php';">Go Back to Form</button></div>
    </div>
</div>
<?php
}
else{
    /**
     * Display the encounter creation form
     * 
     * The form is displayed when no POST data is present
     * All fields include Bootstrap tooltips explaining their purpose
     */
?>

<form action="" method="POST">
    <!-- Security: CSRF Protection -->
    <input type="hidden" name="csrf_token" value="<?=htmlspecialchars($_SESSION['csrf_token'], ENT_QUOTES | ENT_HTML5, 'UTF-8')?>">
<div class="container">
    <div class="row pt-1 pb-1">
        <div class="col-sm-12 pt-2 pb-2 text-center"><h1>LuminariMUD Random Encounter Entry Creator</h1></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="This must be a unique name from any other encounter.">Encounter Unique Name / Number</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="encounter_record" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="What kind of encounter will this be?  Currently only combat.">Encounter Type</div>
        <div class="col-sm-6"><select class="w-100" name="encounter_type"><option value="ENCOUNTER_CLASS_COMBAT">Combat</option></select></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The minimum character level for which this encounter will appear.">Minimum Level</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="min_level" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The maximum character level for which this encounter will appear.">Maximum Level</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="max_level" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="This field must be the same for all different encounter mobs/objs that appear together.">Encounter Group</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="encounter_group"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The name of the mob/object/chest/trap/etc. in the encounter.  Do not preceed with 'a', 'an', 'the', etc.">Encounter Object Name (mob, obj, container, trap, etc).</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="object_name"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Shown when you look in a room. Eg. 'A tall, muscular orcish warrior jabs its spear in your direction.'">Long Description</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="long_desc"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Shown when you look at the mob/obj.">Mob/Object Description</div>
        <div class="col-sm-6">
            <textarea name="description" class="w-100" style="height: 100px;"></textarea>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The percentage chance that this encounter object/mob will load. Mainly to be used when an encounter has multiple objs/mobs/etc, and randomized which will actually be chosen.">Load Chance</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="load_chance"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The minimum number of this obj/mob/etc that will load.">Minimum Number</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="min_number" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The maximum number of this obj/mob/etc that will load. Will never exceed party size.">Maximum Number</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="max_number" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="If we want to have this mob/obj/etc load extra treasure beyond the normal random treasure system, which table will we use?">Treasure Table</div>
        <div class="col-sm-6">
          <select class="w-100" name="treasure_table">
            <option value="TREASURE_TABLE_NONE">No Extra Treasure</option>
            <option value="TREASURE_TABLE_LOW_NORM">Low Level (1-4)</option>
            <option value="TREASURE_TABLE_LOW_BOSS">Low Level (1-4) Boss</option>
            <option value="TREASURE_TABLE_LOW_MID_NORM">Low-Mid Level (5-8)</option>
            <option value="TREASURE_TABLE_LOW_MID_BOSS">Low-Mid Level (5-8) Boss</option>
            <option value="TREASURE_TABLE_MID_NORM">Mid Level (9-12)</option>
            <option value="TREASURE_TABLE_MID_BOSS">Mid Level (9-12)Boss</option>
            <option value="TREASURE_TABLE_MID_HIGH_NORM">Mid-High Level (13-16)</option>
            <option value="TREASURE_TABLE_MID_HIGH_BOSS">Mid-High Level (13-16) Boss</option>
            <option value="TREASURE_TABLE_HIGH_NORM">High Level (17-20)</option>
            <option value="TREASURE_TABLE_HIGH_BOSS">High Level (17-20) Boss</option>
            <option value="TREASURE_TABLE_EPIC_LOW_NORM">Epic Low Level (21-24)</option>
            <option value="TREASURE_TABLE_EPIC_LOW_BOSS">Epic Low Level (21-24) Boss</option>
            <option value="TREASURE_TABLE_EPIC_MID_NORM">Epic Mid Level (25-28)</option>
            <option value="TREASURE_TABLE_EPIC_MID_BOSS">Epic Mid Level (25-28) Boss</option>
            <option value="TREASURE_TABLE_EPIC_HIGH_NORM">Epic High Level (29+)</option>
            <option value="TREASURE_TABLE_EPIC_HIGH_BOSS">Epic High Level (29+) Boss</option>
          </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="In the case of mob encounters, what class will, the mob be? Determines statistics like hp, hitroll, ability scores, etc. as well as special abilities (actions, spells, etc.)">Mob Class</div>
        <div class="col-sm-6">
            <select class="w-100" name="class">
                <option value="CLASS_WIZARD">Wizard</option>
                <option value="CLASS_CLERIC">Cleric</option>
                <option value="CLASS_ROGUE">Rogue</option>
                <option value="CLASS_WARRIOR" selected>Warrior</option>
                <option value="CLASS_MONK">Monk</option>
                <option value="CLASS_DRUID">Druid</option>
                <option value="CLASS_BERSERKER">Berserker</option>
                <option value="CLASS_SORCERER">Sorcerer</option>
                <option value="CLASS_PALADIN">Paladin</option>
                <option value="CLASS_RANGER">Ranger</option>
                <option value="CLASS_BARD">Bard</option>
                <option value="CLASS_ALCHEMIST">Alchemist</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Mainly for combat encounters.  Boss strength mobs will have better statistics and award more exp.">Encounter Strength</div>
        <div class="col-sm-6">
            <select class="w-100" name="encounter_strength">
                <option value="ENCOUNTER_STRENGTH_NORMAL">Normal</option>
                <option value="ENCOUNTER_STRENGTH_BOSS">Boss</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Alignment of mob in this encounter.">Mob Alignment</div>
        <div class="col-sm-6">
            <select class="w-100" name="alignment">
                <option value="LAWFUL_GOOD">Lawful Good</option>
                <option value="LAWFUL_NEUTRAL">Lawful Neutral</option>
                <option value="LAWFUL_EVIL">Lawful Evil</option>
                <option value="NEUTRAL_GOOD">Neutral Good</option>
                <option value="TRUE_NEUTRAL">True Neutral</option>
                <option value="NEUTRAL_EVIL">Neutral Evil</option>
                <option value="CHAOTIC_GOOD">Chaotic Good</option>
                <option value="CHAOTIC_NEUTRAL">Chaotic Neutral</option>
                <option value="CHAOTIC_EVIL">Chaotic Evil</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Main 'parent' racial type.">Racial Type</div>
        <div class="col-sm-6">
            <select class="w-100" name="race_type">
                <option value="RACE_TYPE_UNKNOWN">Unknown</option>
                <option value="RACE_TYPE_HUMANOID">Humanoid</option>
                <option value="RACE_TYPE_UNDEAD">Undead</option>
                <option value="RACE_TYPE_ANIMAL">Animal</option>
                <option value="RACE_TYPE_DRAGON">Dragon</option>
                <option value="RACE_TYPE_GIANT">Giant</option>
                <option value="RACE_TYPE_ABERRATION">Abberation</option>
                <option value="RACE_TYPE_CONSTRUCT">Construct</option>
                <option value="RACE_TYPE_ELEMENTAL">Elemental</option>
                <option value="RACE_TYPE_FEY">Fey</option>
                <option value="RACE_TYPE_MAGICAL_BEAST">Magical Beast</option>
                <option value="RACE_TYPE_MONSTROUS_HUMANOID">Monstrous Humanoid</option>
                <option value="RACE_TYPE_OOZE">Ooze</option>
                <option value="RACE_TYPE_OUTSIDER">Outsider</option>
                <option value="RACE_TYPE_PLANT">Plant</option>
                <option value="RACE_TYPE_VERMIN">Vermin</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Subracial type.  Up to three can be chosen.">Sub Race 1</div>
        <div class="col-sm-6">
            <select class="w-100" name="subrace1">
                <option value="SUBRACE_UNKNOWN">Unknown</option>
                <option value="SUBRACE_AIR">Air</option>
                <option value="SUBRACE_ANGEL">Angel</option>
                <option value="SUBRACE_AQUATIC">Aquatic</option>
                <option value="SUBRACE_ARCHON">Archon</option>
                <option value="SUBRACE_AUGMENTED">Augmented</option>
                <option value="SUBRACE_CHAOTIC">Chaotic</option>
                <option value="SUBRACE_COLD">Cold</option>
                <option value="SUBRACE_EARTH">Earth</option>
                <option value="SUBRACE_EVIL">Evil</option>
                <option value="SUBRACE_EXTRAPLANAR">Extraplanar</option>
                <option value="SUBRACE_FIRE">Fire</option>
                <option value="SUBRACE_GOBLINOID">Goblinoid</option>
                <option value="SUBRACE_GOOD">Good</option>
                <option value="SUBRACE_INCORPOREAL">Incorporeal</option>
                <option value="SUBRACE_LAWFUL">Lawful</option>
                <option value="SUBRACE_NATIVE">Native</option>
                <option value="SUBRACE_REPTILIAN">Reptilian</option>
                <option value="SUBRACE_SHAPECHANGER">Shapechanger</option>
                <option value="SUBRACE_SWARM">Swarm</option>
                <option value="SUBRACE_WATER">Water</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Subracial type.  Up to three can be chosen.">Sub Race 2</div>
        <div class="col-sm-6">
            <select class="w-100" name="subrace2">
                <option value="SUBRACE_UNKNOWN">Unknown</option>
                <option value="SUBRACE_AIR">Air</option>
                <option value="SUBRACE_ANGEL">Angel</option>
                <option value="SUBRACE_AQUATIC">Aquatic</option>
                <option value="SUBRACE_ARCHON">Archon</option>
                <option value="SUBRACE_AUGMENTED">Augmented</option>
                <option value="SUBRACE_CHAOTIC">Chaotic</option>
                <option value="SUBRACE_COLD">Cold</option>
                <option value="SUBRACE_EARTH">Earth</option>
                <option value="SUBRACE_EVIL">Evil</option>
                <option value="SUBRACE_EXTRAPLANAR">Extraplanar</option>
                <option value="SUBRACE_FIRE">Fire</option>
                <option value="SUBRACE_GOBLINOID">Goblinoid</option>
                <option value="SUBRACE_GOOD">Good</option>
                <option value="SUBRACE_INCORPOREAL">Incorporeal</option>
                <option value="SUBRACE_LAWFUL">Lawful</option>
                <option value="SUBRACE_NATIVE">Native</option>
                <option value="SUBRACE_REPTILIAN">Reptilian</option>
                <option value="SUBRACE_SHAPECHANGER">Shapechanger</option>
                <option value="SUBRACE_SWARM">Swarm</option>
                <option value="SUBRACE_WATER">Water</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Subracial type.  Up to three can be chosen.">Sub Race 3</div>
        <div class="col-sm-6">
            <select class="w-100" name="subrace3">
                <option value="SUBRACE_UNKNOWN">Unknown</option>
                <option value="SUBRACE_AIR">Air</option>
                <option value="SUBRACE_ANGEL">Angel</option>
                <option value="SUBRACE_AQUATIC">Aquatic</option>
                <option value="SUBRACE_ARCHON">Archon</option>
                <option value="SUBRACE_AUGMENTED">Augmented</option>
                <option value="SUBRACE_CHAOTIC">Chaotic</option>
                <option value="SUBRACE_COLD">Cold</option>
                <option value="SUBRACE_EARTH">Earth</option>
                <option value="SUBRACE_EVIL">Evil</option>
                <option value="SUBRACE_EXTRAPLANAR">Extraplanar</option>
                <option value="SUBRACE_FIRE">Fire</option>
                <option value="SUBRACE_GOBLINOID">Goblinoid</option>
                <option value="SUBRACE_GOOD">Good</option>
                <option value="SUBRACE_INCORPOREAL">Incorporeal</option>
                <option value="SUBRACE_LAWFUL">Lawful</option>
                <option value="SUBRACE_NATIVE">Native</option>
                <option value="SUBRACE_REPTILIAN">Reptilian</option>
                <option value="SUBRACE_SHAPECHANGER">Shapechanger</option>
                <option value="SUBRACE_SWARM">Swarm</option>
                <option value="SUBRACE_WATER">Water</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Size of the mob/obj.">Creature Size</div>
        <div class="col-sm-6">
            <select class="w-100" name="size">
                <option value="SIZE_FINE">Fine</option>
                <option value="SIZE_DIMINUTIVE">Diminutive</option>
                <option value="SIZE_TINY">Tiny</option>
                <option value="SIZE_SMALL">Small</option>
                <option value="SIZE_MEDIUM">Medium</option>
                <option value="SIZE_LARGE">Large</option>
                <option value="SIZE_HUGE">Huge</option>
                <option value="SIZE_GARGANTUAN">Gargantuan</option>
                <option value="SIZE_COLOSSAL">Colossal</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="If hostile, the characters can only leave the encounter by defeating the mob, or using a skill or ability to escape.">Hostile?</div>
        <div class="col-sm-6">
            <select class="w-100" name="hostile">
                <option value="NON_HOSTILE">Non-Hostile</option>
                <option value="HOSTILE">Hostile</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="If sentient, they can potentially be bribed, intimidated, bluffed, or use diplomacy with to avoid the encounter.">Sentient?</div>
        <div class="col-sm-6">
            <select class="w-100" name="sentient">
                <option value="NON_SENTIENT">Non-Sentient</option>
                <option value="SENTIENT">Sentient</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Different terrain types the mob can appear in. All surface/underdark terrains do not include flying or water.">Terrains Encounter Can Spawn In (Multi Select with CTRL+Click)</div>
        <div class="col-sm-6">
            <?php
            /**
             * Terrain selection list
             * 
             * Terrain categories:
             * - Special groups (ALL, ALL_SURFACE, etc.) - Apply multiple terrains at once
             * - Surface world terrains - Normal overworld locations
             * - Underdark terrains - Underground specific locations
             * - Special terrains - Water, flying, lava, etc.
             * 
             * Note: Using a special group (like SECT_ALL_SURFACE) will override
             * individual terrain selections for that category
             */
            ?>
            <select class="w-100" name="terrain[]" size="8" multiple="multiple">
                <!-- Special terrain groups -->
                <option value="SECT_ALL">All Terrains</option>
                <option value="SECT_ALL_SURFACE">All Surface Terrains</option>
                <option value="SECT_ALL_UD">All Underdark Terrains</option>
                <option value="SECT_ROADS">All Surface Roads</option>
                <option value="SECT_ALL_WATER">All Water Types</option>
                
                <!-- Surface world terrains -->
                <option value="SECT_INSIDE">Inside (Eg. Buildings)</option>
                <option value="SECT_CITY">City Streets</option>
                <option value="SECT_FIELD">Field</option>
                <option value="SECT_FOREST">Forest</option>
                <option value="SECT_HILLS">Hills</option>
                <option value="SECT_MOUNTAIN">Mountains</option>
                <option value="SECT_WATER_SWIM">Swimmable Water</option>
                <option value="SECT_WATER_NOSWIM">Unswimmable Water</option>
                <option value="SECT_FLYING">Flying (Outdoors)</option>
                <option value="SECT_UNDERWATER">Underwater</option>
                <option value="SECT_DESERT">Desert</option>
                <option value="SECT_OCEAN">Ocean</option>
                <option value="SECT_MARSHLAND">Marshland</option>
                <option value="SECT_HIGH_MOUNTAIN">High Mountains (Requires Flight or Climbing)</option>
                
                <!-- Underdark specific terrains -->
                <option value="SECT_UD_WILD">Underdark Wild</option>
                <option value="SECT_UD_CITY">Underdark City Streets</option>
                <option value="SECT_UD_INSIDE">Underdark Inside (Eg. Buildings)</option>
                <option value="SECT_UD_WATER">Underdark Swimmable Water</option>
                <option value="SECT_UD_NOSWIM">Underdark Unswimmable Water</option>
                <option value="SECT_UD_NOGROUND">Underdark Flying (Eg. Chasm)</option>
                
                <!-- Other terrain types -->
                <option value="SECT_LAVA">Lava</option>
                <option value="SECT_CAVE">Cave</option>
                <option value="SECT_JUNGLE">Jungle</option>
                <option value="SECT_TUNDRA">Tundra</option>
                <option value="SECT_TAIGA">Taiga</option>
                <option value="SECT_BEACH">Beach</option>
                <option value="SECT_SEAPORT">Sea Port</option>
                <option value="SECT_INSIDE_ROOM">Inside Room (Eg. Buildings, non-hallway)</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-12"><input class="w-100 btn btn-warning" type="submit" value="Convert to Code"></div>
    </div>
</div>
</form>
<?php
}
?>
<script src="https://cdnjs.cloudflare.com/ajax/libs/popper.js/1.12.9/umd/popper.min.js" integrity="sha384-ApNbgh9B+Y1QKtv3Rn7W3mgPxhU9K/ScQsAP7hUibX39j7fakFPskvXusvfa0b4Q" crossorigin="anonymous"></script>
<script src="https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0/js/bootstrap.min.js" integrity="sha384-JZR6Spejh4U02d8jOt6vLEHfe/JQGiRRSQQxSfFWpi1MquVdAyjUar5+76PVCmYl" crossorigin="anonymous"></script>
</body>
</html>