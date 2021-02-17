<?php
//ini_set('display_errors', 1);
//ini_set('display_startup_errors', 1);
//error_reporting(E_ALL);
?>
<html>
<head>
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
if ($_POST)
{
    $cast_command = $_POST['cast_command'];
    if ($cast_command == "cast")
        $see_also = "SPELLS";
    else if ($cast_command == "imbibe")
        $see_also = "CONCOCTIONS";
    else
        $see_also = "PSIONICS";
    $spell_name = $_POST['spell_name'];
    $can_target = $_POST['can_target'];
    $accumulative = $_POST['accumulative'];
    $duration = $_POST['duration'];
    $school = $_POST['school'];
    if ($school == "Abjuration" || $school == "Conjuration" || $school == "Divination" || $school == "Enchantment" ||
        $school == "Illusion" || $school == "Necromancy" || $school == "Transmutation")
        $school_label = "School of Magic:";
    else
        $school_label = "Discipine:      ";
    $targets = $_POST['targets'];
    $magic_resist = $_POST['magic_resist'];
    $saving_throw = $_POST['saving_throw'];
    $damage_type = $_POST['damage_type'];
    $description = $_POST['description'];
    $description = wordwrap($description, 80, "\n	n");
    $augment = $_POST['augment'];
    $augment = str_replace("Augment ", "	DAugment: 	n", $augment);
    $augment = wordwrap($augment, 80, "\n	n");
    $psp_cost = $_POST['psp_cost'];

    $output = "
	D>Usage:           	W ".$cast_command." '".$spell_name."'".$can_target."	n
	D>Accumulative:    	W ".$accumulative." 	n
	D>Duration:        	W ".$duration." 	n
	D>".$school_label." 	W ".$school." 	n
	D>Target(s):       	W ".$targets." 	n
	D>Magic Resist:    	W ".$magic_resist." 	n
	D>Saving Throw:    	W ".$saving_throw." 	n
	D>Damage Type:     	W ".$damage_type." 	n".
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
            <textarea name="codeOutput" id="codeOutput" class="w-100" style="height: 400px;"><?=$output?></textarea>
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
?>
<form action="" method="POST">
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
            <select class="w-100" name="school">
                <option value="Abjuration">Abjuration</option>
                <option value="Conjuration">Conjuration</option>
                <option value="Divination">Divination</option>
                <option value="Enchantment">Enchantment</option>
                <option value="Illusion">Illusion</option>
                <option value="Necromancy">Necromancy</option>
                <option value="Transmutation">Transmutation</option>
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