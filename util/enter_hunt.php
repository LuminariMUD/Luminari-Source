<?php
/**
 * enter_hunt.php - LuminariMUD Hunt System Creator Tool
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
    error_log("Unauthorized access attempt to enter_hunt.php from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
    http_response_code(403);
    die("Access denied. This tool requires developer authentication.");
}

// Security: Generate CSRF token if not exists
if (!isset($_SESSION['csrf_token'])) {
    $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
}

/**
 * enter_hunt.php - LuminariMUD Hunt System Creator Tool
 * 
 * PURPOSE:
 * This tool generates C code for hunt mob definitions in the LuminariMUD
 * hunt system. Hunts are special high-level boss encounters that players
 * can seek out for rewards and challenges.
 * 
 * FUNCTIONALITY:
 * - Creates hunt mob definitions with unique abilities
 * - Generates properly formatted C code for hunts.c
 * - Supports multiple special abilities per hunt
 * - Handles mob descriptions and characteristics
 * 
 * HUNT SYSTEM OVERVIEW:
 * The hunt system provides:
 * - Named boss monsters with special abilities
 * - Level-appropriate challenges
 * - Special rewards for defeating hunts
 * - Unique abilities not found on regular mobs
 * 
 * GENERATED CODE FORMAT:
 * The tool generates:
 * 1. add_hunt() - Main hunt definition with all parameters
 * 2. add_hunt_ability() - Special ability assignments
 * 3. Macro definitions for hunts.h
 * 
 * INTEGRATION:
 * Generated code should be added to:
 * - hunts.c (function calls in init_hunts())
 * - hunts.h (HUNT_TYPE_ macro definitions)
 * 
 * SPECIAL ABILITIES:
 * Hunt mobs can have unique abilities like:
 * - Petrification, tail spikes, level drain
 * - Charm, blink, engulf
 * - Various breath weapons
 * - Magic immunity, invisibility, etc.
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
<title>LuminariMUD Hunt Creator</title>
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
     * Process form submission and generate C code for hunt definition
     * 
     * Data processing steps:
     * 1. Sanitize and format input values
     * 2. Convert spaces to underscores for C macros
     * 3. Add HUNT_TYPE_ prefix for constants
     * 4. Escape special characters in descriptions
     */
    
    // Validate and sanitize hunt record name
    $hunt_record_raw = validateInput($_POST['hunt_record'] ?? '', 'identifier', 50);
    if ($hunt_record_raw === false) {
        http_response_code(400);
        die("Invalid hunt record name. Use only letters, numbers, and underscores.");
    }

    // Generate unique hunt identifier
    // Format: HUNT_TYPE_NAME_HERE
    $hunt_record = "HUNT_TYPE_" . str_replace(" ", "_", strtoupper($hunt_record_raw));

    // Validate hunt level
    $level = validateInput($_POST['level'] ?? '', 'number');
    if ($level === false || $level < 1 || $level > 50) {
        http_response_code(400);
        die("Invalid level. Must be a number between 1 and 50.");
    }

    // Validate mob name
    $name = validateInput($_POST['name'] ?? '', 'text', 100);
    if ($name === false) {
        http_response_code(400);
        die("Invalid mob name. Maximum 100 characters, no special characters.");
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

    // Validate alignment against whitelist
    $allowed_alignments = ['LAWFUL_GOOD', 'LAWFUL_NEUTRAL', 'LAWFUL_EVIL',
                          'NEUTRAL_GOOD', 'TRUE_NEUTRAL', 'NEUTRAL_EVIL',
                          'CHAOTIC_GOOD', 'CHAOTIC_NEUTRAL', 'CHAOTIC_EVIL'];
    $alignment = $_POST['alignment'] ?? '';
    if (!in_array($alignment, $allowed_alignments, true)) {
        http_response_code(400);
        die("Invalid alignment selection.");
    }
    
    // Race information (primary type + up to 3 subtypes)
    $race_type = $_POST['race_type'];   // Main race (humanoid, dragon, etc.)
    $subrace1 = $_POST['subrace1'];     // Subrace modifier 1
    $subrace2 = $_POST['subrace2'];     // Subrace modifier 2
    $subrace3 = $_POST['subrace3'];     // Subrace modifier 3
    
    // Physical characteristics
    $size = $_POST['size'];             // Size category (fine to colossal)
    
    // Descriptions
    $long_desc = $_POST['long_desc'];   // Room description when present
    $desc = $_POST['description'];      // Detailed look description

    /**
     * Generate C code for hunt definition
     * 
     * Function: add_hunt()
     * Parameters:
     * - hunt_id: Unique identifier (HUNT_TYPE_xxx)
     * - level: Hunt difficulty level
     * - name: Hunt mob name
     * - description: Detailed description when examined
     * - long_desc: Room description
     * - class: Mob class (affects stats and abilities)
     * - alignment: Alignment constant
     * - race_type: Primary race category
     * - subrace1-3: Subrace modifiers
     * - size: Size category
     */
    $output = "    add_hunt(".$hunt_record.", ".$level.", \"".$name."\", ".
              " \"".addslashes(str_replace("\"", "'", $desc))."\", "."\n".
              "      \"".addslashes(str_replace("\"", "'", $long_desc))."\"".
              ", ".$class.", ".$alignment.", ".$race_type.", \n".
              "      ".$subrace1.", ".$subrace2.", ".$subrace3.", ".$size." );\n";
    
    /**
     * Process special abilities
     * 
     * Each hunt can have multiple special abilities that make it unique
     * Abilities are added with separate add_hunt_ability() calls
     * 
     * Code formatting: 2 abilities per line for readability
     */
    $i = 0;
    foreach ($_POST['abilities'] as $key)
    {
        $output .= "    add_hunt_ability(".$hunt_record.", ".$key.");";
        
        // Format: 2 abilities per line
        if (($i % 2) == 1)
            $output .= "\n";
        else
            $output .= "  ";
        $i++;
    }
    $output .= "\n";
    
    // Add macro definition for hunts.h
    $output .= "#define ".$hunt_record."\n";
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
            <textarea name="codeOutput" id="codeOutput" class="w-100" style="height: 400px;"><?=htmlspecialchars($output, ENT_QUOTES | ENT_HTML5, 'UTF-8')?></textarea>
        </div>
    </div>
    <div class="row">
        <div><br /></div>
        <div class="col-sm-12"><button class="btn btn-success w-100" onclick="copyCode()">Copy Code to Clipboard</button></div>
    </div>
    <div class="row">
        <div><br /></div>
        <div class="col-sm-12"><button class="btn btn-warning w-100" onclick="window.location='/util/enter_hunt.php';">Go Back to Form</button></div>
    </div>
</div>
<?php
}
else{
    /**
     * Display the hunt creation form
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
        <div class="col-sm-12 pt-2 pb-2 text-center"><h1>LuminariMUD Hunt Entry Creator</h1></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="This must be a unique name from any other hunt.">Hunt Unique Name / Number</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="hunt_record" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The level of the hunt mob.">Level</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="level" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="The name of the mob in the hunt.  Do not preceed with 'a', 'an', 'the', etc.">Hunt Mob Name</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="name"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Shown when you look in a room. Eg. 'A tall, muscular orcish warrior jabs its spear in your direction.'">Long Description</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="long_desc"></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Shown when you look at the mobj.">Mob Description</div>
        <div class="col-sm-6">
            <textarea name="description" class="w-100" style="height: 100px;"></textarea>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="What class will the mob be? Determines statistics like hp, hitroll, ability scores, etc. as well as special abilities (actions, spells, etc.)">Mob Class</div>
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
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Alignment of mob in this hunt.">Mob Alignment</div>
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
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Size of the mob.">Creature Size</div>
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
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Special abilities the hunt mob can perform.">Hunt Mob Special Abilities (Multi Select with CTRL+Click)</div>
        <div class="col-sm-6">
            <?php
            /**
             * Special ability selection
             * 
             * Ability categories:
             * - Status effects: Petrify, charm, fear, paralyze
             * - Physical attacks: Tail spikes, engulf, swallow, grapple
             * - Magical attacks: Level drain, corruption
             * - Breath weapons: Fire, lightning, poison, acid, frost
             * - Defenses: Magic immunity, regeneration, flight
             * - Special: Blink, invisibility
             * 
             * Multiple abilities can be selected to create unique challenges
             */
            ?>
            <select class="w-100" name="abilities[]" size="8" multiple="multiple">
                <!-- Status Effect Abilities -->
                <option value="HUNT_ABIL_PETRIFY">Petrification</option>
                <option value="HUNT_ABIL_CHARM">Charm</option>
                <option value="HUNT_ABIL_CAUSE_FEAR">Cause Fear</option>
                <option value="HUNT_ABIL_PARALYZE">Paralyze</option>
                <option value="HUNT_ABIL_POISON">Poison</option>
                
                <!-- Physical Attack Abilities -->
                <option value="HUNT_ABIL_TAIL_SPIKES">Tail Spikes</option>
                <option value="HUNT_ABIL_ENGULF">Engulf</option>
                <option value="HUNT_ABIL_SWALLOW">Swallow Whole</option>
                <option value="HUNT_ABIL_GRAPPLE">Grapple</option>
                
                <!-- Magical Abilities -->
                <option value="HUNT_ABIL_LEVEL_DRAIN">Level Drain</option>
                <option value="HUNT_ABIL_CORRUPTION">Corruption</option>
                <option value="HUNT_ABIL_BLINK">Blink</option>
                
                <!-- Breath Weapons -->
                <option value="HUNT_ABIL_FIRE_BREATH">Fire Breath</option>
                <option value="HUNT_ABIL_LIGHTNING_BREATH">Lightning Breath</option>
                <option value="HUNT_ABIL_POISON_BREATH">Poison Breath</option>
                <option value="HUNT_ABIL_ACID_BREATH">Acid Breath</option>
                <option value="HUNT_ABIL_FROST_BREATH">Frost Breath</option>
                
                <!-- Defensive Abilities -->
                <option value="HUNT_ABIL_MAGIC_IMMUNITY">Magic Immunity</option>
                <option value="HUNT_ABIL_REGENERATION">Regeneration</option>
                <option value="HUNT_ABIL_FLIGHT">Can Fly</option>
                <option value="HUNT_ABIL_INVISIBILITY">Invisible</option>
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