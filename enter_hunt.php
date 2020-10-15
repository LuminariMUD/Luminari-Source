<?php
//ini_set('display_errors', 1);
//ini_set('display_startup_errors', 1);
//error_reporting(E_ALL);
?>
<html>
<head>
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
if ($_POST)
{
    $hunt_record = "HUNT_TYPE_" . str_replace(" ", "_", strtoupper(trim($_POST['hunt_record'])));
    $level = $_POST['level'];
    $name = $_POST['name'];
    $class = $_POST['class'];
    $alignment = $_POST['alignment'];
    $race_type = $_POST['race_type'];
    $subrace1 = $_POST['subrace1'];
    $subrace2 = $_POST['subrace2'];
    $subrace3 = $_POST['subrace3'];
    $size = $_POST['size'];
    $long_desc = $_POST['long_desc'];
    $desc = $_POST['description'];

    $output = "    add_hunt(".$hunt_record.", ".$level.", \"".$name."\", ".
              " \"".addslashes(str_replace("\"", "'", $desc))."\", "."\n".
              "      \"".addslashes(str_replace("\"", "'", $long_desc))."\"".
              ", ".$class.", ".$alignment.", ".$race_type.", \n".
              "      ".$subrace1.", ".$subrace2.", ".$subrace3.", ".$size." );\n";
    $i = 0;
    foreach ($_POST['abilities'] as $key)
    {
        $output .= "    add_hunt_ability(".$hunt_record.", ".$key.");";
        if (($i % 2) == 1)
            $output .= "\n";
        else
            $output .= "  ";
        $i++;
    }
    $output .= "\n";
    $output .= "#define ".$hunt_record."\n";
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
        <div class="col-sm-12"><button class="btn btn-warning w-100" onclick="window.location='/enter_hunt.php';">Go Back to Form</button></div>
    </div>
</div>
<?php
}
else{
?>

<form action="" method="POST">
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
            <select class="w-100" name="abilities[]" size="8" multiple="multiple">
                <option value="HUNT_ABIL_PETRIFY">Petrification</option>
                <option value="HUNT_ABIL_TAIL_SPIKES">Tail Spikes</option>
                <option value="HUNT_ABIL_LEVEL_DRAIN">Level Drain</option>
                <option value="HUNT_ABIL_CHARM">Charm</option>
                <option value="HUNT_ABIL_BLINK">Blink</option>
                <option value="HUNT_ABIL_ENGULF">Engulf</option>
                <option value="HUNT_ABIL_CAUSE_FEAR">Cause Fear</option>
                <option value="HUNT_ABIL_CORRUPTION">Corruption</option>
                <option value="HUNT_ABIL_SWALLOW">Swallow Whole</option>
                <option value="HUNT_ABIL_FLIGHT">Can Fly</option>
                <option value="HUNT_ABIL_POISON">Poison</option>
                <option value="HUNT_ABIL_REGENERATION">Regeneration</option>
                <option value="HUNT_ABIL_PARALYZE">Paralyze</option>
                <option value="HUNT_ABIL_FIRE_BREATH">Fire Breath</option>
                <option value="HUNT_ABIL_LIGHTNING_BREATH">Lightning Breath</option>
                <option value="HUNT_ABIL_POISON_BREATH">Poison Breath</option>
                <option value="HUNT_ABIL_ACID_BREATH">Acid Breath</option>
                <option value="HUNT_ABIL_FROST_BREATH">Frost Breath</option>
                <option value="HUNT_ABIL_MAGIC_IMMUNITY">Magic Immunity</option>
                <option value="HUNT_ABIL_INVISIBILITY">Invisible</option>
                <option value="HUNT_ABIL_GRAPPLE">Grapple</option>
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