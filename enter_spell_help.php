<?php
/**
 * enter_spell_help.php - LuminariMUD Spell/Power Help File Generator
 *
 * SECURITY NOTICE:
 * This tool generates help file content and should be protected with authentication.
 * Only authorized content creators should have access to this tool.
 */

// Security: Start session for authentication and CSRF protection
session_start();

// Security: Authentication check for content generation tools
if (!isset($_SESSION['authenticated']) || $_SESSION['authenticated'] !== true ||
    (!isset($_SESSION['role']) || !in_array($_SESSION['role'], ['developer', 'content_creator'], true))) {
    error_log("Unauthorized access attempt to enter_spell_help.php from IP: " . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
    http_response_code(403);
    die("Access denied. This tool requires content creator authentication.");
}

// Security: Generate CSRF token if not exists
if (!isset($_SESSION['csrf_token'])) {
    $_SESSION['csrf_token'] = bin2hex(random_bytes(32));
}

/**
 * enter_spell_help.php - LuminariMUD Spell/Power Help File Generator
 * 
 * PURPOSE:
 * This tool generates formatted help file entries for spells, psionic powers,
 * and alchemical concoctions in the LuminariMUD system. It ensures consistent
 * formatting and proper color coding for the MUD's help system.
 * 
 * FUNCTIONALITY:
 * - Creates help entries for three ability types: spells, concoctions, psionics
 * - Generates properly formatted text with MUD color codes
 * - Handles word wrapping at 80 characters
 * - Supports augmentation text for psionic powers
 * 
 * HELP SYSTEM OVERVIEW:
 * LuminariMUD uses a standardized help format for abilities:
 * - Usage line showing the command syntax
 * - Ability properties (accumulative, duration, etc.)
 * - School/discipline information
 * - Target and resistance information
 * - Detailed description
 * - Augmentation options (psionics only)
 * 
 * COLOR CODE SYSTEM:
 * The MUD uses special codes for text formatting:
 * - 	D: Dark gray (labels)
 * - 	W: White (values)
 * - 	Y: Yellow (see also section)
 * - 	n: Normal/reset color
 * 
 * GENERATED OUTPUT:
 * The tool generates help text that should be added to the appropriate
 * help files in the MUD's lib/text/help/ directory.
 * 
 * INTEGRATION:
 * Generated help entries should be added to:
 * - Spells: help.spells file
 * - Concoctions: help.concoctions or help.alchemist file
 * - Psionics: help.psionics file
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
<title>LuminariMUD Spell/Psionic Power Help File Creator</title>
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
 * Validate and sanitize input for help file generation
 *
 * @param string $input The input to validate
 * @param string $type The type of input (text, select)
 * @param int $max_length Maximum allowed length
 * @return string|false Sanitized input or false if invalid
 */
function validateInput($input, $type, $max_length = 1000) {
    if (strlen($input) > $max_length) {
        return false;
    }

    switch ($type) {
        case 'text':
            // Remove potentially dangerous characters but allow basic formatting
            $input = preg_replace('/[<>"\'\\\]/', '', $input);
            break;
        case 'select':
            // For dropdown selections, no modification needed
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
 * Form Processing - Convert Form Data to Help File Format
 * ===========================================================================*/

if ($_POST)
{
    // Security: Validate CSRF token first
    validateCSRF();
    /**
     * Process form submission and generate help file text
     * 
     * Processing steps:
     * 1. Determine ability type and set appropriate references
     * 2. Format field labels based on ability type
     * 3. Apply word wrapping for long text fields
     * 4. Add proper color codes for MUD display
     */
    
    // Validate cast command against whitelist
    $allowed_commands = ['cast', 'imbibe', 'manifest'];
    $cast_command = $_POST['cast_command'] ?? '';
    if (!in_array($cast_command, $allowed_commands, true)) {
        http_response_code(400);
        die("Invalid cast command selection.");
    }

    // Determine ability type and set "See also" reference
    if ($cast_command == "cast")
        $see_also = "SPELLS";          // Magic spells
    else if ($cast_command == "imbibe")
        $see_also = "CONCOCTIONS";     // Alchemist abilities
    else
        $see_also = "PSIONICS";        // Psionic powers

    // Validate and sanitize basic ability information
    $spell_name = validateInput($_POST['spell_name'] ?? '', 'text', 100);
    if ($spell_name === false) {
        http_response_code(400);
        die("Invalid spell name. Maximum 100 characters.");
    }

    // Validate targeting option
    $allowed_targeting = ['', ' (target)'];
    $can_target = $_POST['can_target'] ?? '';
    if (!in_array($can_target, $allowed_targeting, true)) {
        http_response_code(400);
        die("Invalid targeting option.");
    }

    $accumulative = validateInput($_POST['accumulative'] ?? '', 'text', 200);
    $duration = validateInput($_POST['duration'] ?? '', 'text', 200);
    if ($accumulative === false || $duration === false) {
        http_response_code(400);
        die("Invalid accumulative or duration description. Maximum 200 characters each.");
    }
    
    // School/discipline formatting
    // Magic uses "School of Magic:" while psionics use "Discipline:"
    $school = $_POST['school'];
    if ($school == "Abjuration" || $school == "Conjuration" || $school == "Divination" || $school == "Enchantment" ||
        $school == "Illusion" || $school == "Necromancy" || $school == "Transmutation")
        $school_label = "School of Magic:";
    else
        $school_label = "Discipine:      ";  // Note: typo preserved for compatibility
    
    // Combat and resistance information
    $targets = $_POST['targets'];           // Who can be targeted
    $magic_resist = $_POST['magic_resist']; // Subject to spell resistance?
    $saving_throw = $_POST['saving_throw']; // Type of save allowed
    $damage_type = $_POST['damage_type'];   // Damage type if applicable
    
    // Description processing - wrap at 80 characters
    $description = $_POST['description'];
    $description = wordwrap($description, 80, "\n	n");
    
    // Augmentation text (psionics only) - special formatting
    $augment = $_POST['augment'];
    $augment = str_replace("Augment ", "	DAugment: 	n", $augment);
    $augment = wordwrap($augment, 80, "\n	n");
    
    // PSP cost (psionics only)
    $psp_cost = $_POST['psp_cost'];

    /**
     * Generate formatted help file text
     * 
     * Format structure:
     * - Header section with usage and properties
     * - Each line uses color codes: \tD for labels, \tW for values
     * - Description section with wrapped text
     * - Optional PSP cost for psionics
     * - Augmentation section for psionics
     * - See also reference at bottom
     * 
     * Color codes:
     * \tD = Dark gray (labels)
     * \tW = White (values)  
     * \tY = Yellow (references)
     * \tn = Normal/reset
     */
    $output = "
	D>Usage:           	W ".$cast_command." '".$spell_name."'".$can_target."	n
	D>Accumulative:    	W ".$accumulative." 	n
	D>Duration:        	W ".$duration." 	n
	D>".$school_label." 	W ".$school." 	n
	D>Target(s):       	W ".$targets." 	n
	D>Magic Resist:    	W ".$magic_resist." 	n
	D>Saving Throw:    	W ".$saving_throw." 	n
	D>Damage Type:     	W ".$damage_type." 	n".
    // Include PSP cost line only for psionic powers
    ((strlen($psp_cost) > 0) ?
"
	D>PSP Cost:        	W ".$psp_cost." 	n": "")."
	D>Description:	n
	n
	n".$description."
	n
	n".$augment."
	n
	YSee also:	n ".$see_also."
	n";
?>
<script>
/**
 * Copy generated help text to clipboard
 * 
 * This function:
 * 1. Selects all text in the output textarea
 * 2. Copies it to the system clipboard
 * 3. Shows confirmation alert
 * 
 * The copied text can then be pasted directly into
 * the appropriate help file in the MUD.
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
        <div class="col-sm-12"><button class="btn btn-warning w-100" onclick="window.location='/enter_spell_help.php';">Go Back to Form</button></div>
    </div>
</div>
<?php
}
else{
    /**
     * Display the spell/power help creation form
     * 
     * The form is displayed when no POST data is present
     * All fields include Bootstrap tooltips explaining their purpose
     * 
     * Form sections:
     * 1. Ability type selection (spell/concoction/power)
     * 2. Basic properties (name, targeting, duration)
     * 3. School/discipline selection
     * 4. Combat properties (saves, resistance, damage)
     * 5. Description and augmentation text
     */
?>
<form action="" method="POST">
    <!-- Security: CSRF Protection -->
    <input type="hidden" name="csrf_token" value="<?=htmlspecialchars($_SESSION['csrf_token'], ENT_QUOTES | ENT_HTML5, 'UTF-8')?>">
<div class="container">
    <div class="row pt-1 pb-1">
        <div class="col-sm-12 pt-2 pb-2 text-center"><h1>LuminariMUD Spell/Psionic Power Help File Creator</h1></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="What kind of power is it? A spell, an alchemical concoction or a psionic power?">Ability Type</div>
        <div class="col-sm-6">
            <select class="w-100" name="cast_command">
                <option value="cast">Spell</option>
                <option value="imbibe">Concoction (Alchemists)</option>
                <option value="manifest">Power (Psionics)</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="This is the name of th spell or psionic power">Spell/Power Name</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="spell_name" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Is this spell able to specify a target?">Can Target Something?</div>
        <div class="col-sm-6">
            <select class="w-100" name="can_target">
                <option value="">No</option>
                <option value=" (target)">Yes</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Description of whether the power is accumulative">Accumulative Description</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="accumulative" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Duration Description">Duration</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="duration" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="What is the school of magic or psychic discipline?">School of Magic/Discipline</div>
        <div class="col-sm-6">
            <?php
            /**
             * School/Discipline selection
             * 
             * Magic Schools (D&D/Pathfinder standard):
             * - Abjuration: Protection and dispelling
             * - Conjuration: Summoning and creation
             * - Divination: Knowledge and detection
             * - Enchantment: Mind control and influence
             * - Illusion: Deception and misdirection
             * - Necromancy: Death and undeath
             * - Transmutation: Transformation and enhancement
             * 
             * Psionic Disciplines:
             * - Clairsentience: Perception and knowledge
             * - Metacreativity: Creation of objects/energy
             * - Psychokinesis: Moving objects with mind
             * - Psychometabolism: Body transformation
             * - Psychoportation: Teleportation
             * - Telepathy: Mind reading and control
             */
            ?>
            <select class="w-100" name="school">
                <!-- Magic Schools -->
                <option value="Abjuration">Abjuration</option>
                <option value="Conjuration">Conjuration</option>
                <option value="Divination">Divination</option>
                <option value="Enchantment">Enchantment</option>
                <option value="Illusion">Illusion</option>
                <option value="Necromancy">Necromancy</option>
                <option value="Transmutation">Transmutation</option>
                
                <!-- Psionic Disciplines -->
                <option value="Clairsentience">Clairsentience</option>
                <option value="Metacreativity">Metacreativity</option>
                <option value="Psychokinesis">Psychokinesis</option>
                <option value="Psychometabolism">Psychometabolism</option>
                <option value="Psychoportation">Psychoportation</option>
                <option value="Telepathy">Telepathy</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Who can be targetted?">Target Type</div>
        <div class="col-sm-6">
            <select class="w-100" name="targets">
                <option value="Self Only">Self Only</option>
                <option value="Single Target">Single Target</option>
                <option value="All Enemies">All Enemies</option>
                <option value="All Group Members">All Group Members</option>
                <option value="Currnt Room">Room</option>
                <option value="Object">Object</option>
            </select>
        </div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Is power succeptible to spell/power resistance?">Magic Resist</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="magic_resist" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Saving Throw Type">Saving Throw</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="saving_throw" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Damage type">Damage Type</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="damage_type" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Psionic power point (PSP) cost to use.">PSP Cost (psionics only)</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="psp_cost" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Augment, if any, for psionic power">Augment (optional)</div>
        <div class="col-sm-6"><input class="w-100" type="text" name="augment" /></div>
    </div>
    <div class="row pt-1 pb-1">
        <div class="col-sm-6 w-100 text-right font-weight-bold" data-toggle="tooltip" data-placement="bottom" title="Spell/Power description.  Will be wrapped at 80 characters.">Spell Description</div>
        <div class="col-sm-6"></div>
        </div><div class="row pt-1 pb-1">
        <div class="col-sm-12">
            <textarea name="description" id="description" cols="30" rows="6" class="w-100"></textarea>
        </div>
    </div>
    </div><div class="row pt-1 pb-1">
        <div class="col-sm-12">
            <input type="submit" value="Submit" class="w-100 btn btn-primary" />
        </div>
    </div>
</div>
<?php
}
?>