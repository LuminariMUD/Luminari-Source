<html>
<head>
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

// This code needs to be copied to a php file located somewhere on the MUD's web site

//   error_reporting(E_ALL);
//   ini_set('display_errors', 1);

  $servername = "";
  $username = ""; // Your MySQL username
  $password = ""; // Your MySQL password
  $database = ""; // Your MySQL database name

  // DB connection setup
  $pdo = new PDO("mysql:host=$servername;dbname=$database", "$username", "$password");
  
// Wear slots (will now be X‑axis headers)
$wear_slots = [
    "Finger", "Neck", "Body", "Head", "Legs", "Feet", "Hands", "Arms", "Shield",
    "About-Body", "Waist", "Wrist", "Wield", "Hold", "Face", "Ammo-Pouch", "Ears",
    "Eyes", "Badge", "Instrument", "Shoulders", "Ankle"
];

// Bonus types (will now be Y‑axis rows)
$bonus_types = [
    "Strength", "Dexterity", "Intelligence", "Wisdom", "Constitution", "Charisma",
    "Max-PSP", "Max-HP", "Max-Move", "Hitroll", "Damroll", "Save-Fortitude",
    "Save-Reflex", "Save-Will", "Spell-Resist", "Armor-Class", "Resist-Fire",
    "Resist-Cold", "Resist-Air", "Resist-Earth", "Resist-Acid", "Resist-Holy",
    "Resist-Electric", "Resist-Unholy", "Resist-Slashing", "Resist-Piercing",
    "Resist-Bludgeoning", "Resist-Sound", "Resist-Poison", "Resist-Disease",
    "Resist-Negative", "Resist-Illusion", "Resist-Mental", "Resist-Light",
    "Resist-Energy", "Resist-Water", "Grant-Feat", "Skill-Bonus", "Power-Resist",
    "HP-Regen", "MV-Regen", "PSP-Regen", "Encumbrance", "Fast-Healing", "Initiative",
    "Spell-Circle-1", "Spell-Circle-2", "Spell-Circle-3", "Spell-Circle-4",
    "Spell-Circle-5", "Spell-Circle-6", "Spell-Circle-7", "Spell-Circle-8",
    "Spell-Circle-9", "Spell-Potency", "Spell-DC", "Spell-Duration", "Spell-Penetration"
];

// Prepare data structures
$matrix    = [];  // matrix[wear_slot][bonus_type] = count
$row_totals = []; // total items per wear slot (for footer)
$col_totals = []; // total occurrences per bonus type (for first column)

// Initialize
foreach ($wear_slots as $slot) {
    $row_totals[$slot] = 0;
    foreach ($bonus_types as $bonus) {
        $matrix[$slot][$bonus] = 0;
        if (!isset($col_totals[$bonus])) {
            $col_totals[$bonus] = 0;
        }
    }
}

// 1) Total items per wear slot
$sql = "
  SELECT worn_slot, COUNT(DISTINCT object_idnum) AS total
    FROM object_database_wear_slots
   GROUP BY worn_slot
";
foreach ($pdo->query($sql)->fetchAll(PDO::FETCH_ASSOC) as $r) {
    if (isset($row_totals[$r['worn_slot']])) {
        $row_totals[$r['worn_slot']] = $r['total'];
    }
}

// 2) Count bonus occurrences per wear slot & bonus type
$sql = "
  SELECT ws.worn_slot AS slot,
         b.bonus_location AS bonus,
         COUNT(*) AS cnt
    FROM object_database_items i
    JOIN object_database_wear_slots ws ON i.idnum = ws.object_idnum
    JOIN object_database_bonuses     b  ON i.idnum = b.object_idnum
   GROUP BY ws.worn_slot, b.bonus_location
";
foreach ($pdo->query($sql)->fetchAll(PDO::FETCH_ASSOC) as $r) {
    $s = $r['slot'];
    $b = $r['bonus'];
    if (isset($matrix[$s][$b])) {
        $matrix[$s][$b] = $r['cnt'];
        $col_totals[$b] += $r['cnt'];
    }
}

// 3) Render table
echo "<table border='1' cellpadding='4' cellspacing='0'>";

// Header row: blank cell, “Total”, then each wear slot
echo "<tr>";
echo "<th>Bonus Type</th>";
echo "<th>Total</th>";
foreach ($wear_slots as $slot) {
    echo "<th><a href=\"/objectdb/bonus_breakdown.php?slot={$slot}\" target=\"_blank\">{$slot}</a></th>";
}
echo "</tr>";

// One row per bonus type
foreach ($bonus_types as $bonus) {
    echo "<tr>";
    // Bonus name + its total occurrences
    echo "<td><strong>{$bonus}</strong></td>";
    echo "<td><strong>" . ($col_totals[$bonus] ?: '') . "</strong></td>";
    // One cell per wear slot
    foreach ($wear_slots as $slot) {
        $val = $matrix[$slot][$bonus];
        echo "<td>" . ($val ? $val : '') . "</td>";
    }
    echo "</tr>";
}

// Footer: “Total Items” row under each slot
echo "<tr>";
echo "<td><strong>Total Items</strong></td>";
echo "<td></td>"; // empty under the “Total” column
foreach ($wear_slots as $slot) {
    echo "<td><strong>" . ($row_totals[$slot] ?: '') . "</strong></td>";
}
echo "</tr>";

echo "</table>";
?>
  </body>
  </html>