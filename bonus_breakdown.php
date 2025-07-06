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

// This code needs to be copied to a php file located somewhere on the MUD's web site

//   error_reporting(E_ALL);
//   ini_set('display_errors', 1);

  $servername = "";
  $username = ""; // Your MySQL username
  $password = ""; // Your MySQL password
  $database = ""; // Your MySQL database name

  // DB connection setup
  $pdo = new PDO("mysql:host=$servername;dbname=$database", "$username", "$password");
// breakdown_slot.php

// 1) Read & validate wear‑slot from query
if (empty($_GET['slot'])) {
    die("Error: missing ?slot=Wear‑SlotName");
}
$slot = $_GET['slot'];

// 3) Define your bonus types (rows)
$bonus_types = [
    "Strength","Dexterity","Intelligence","Wisdom","Constitution","Charisma",
    "Max-PSP","Max-HP","Max-Move","Hitroll","Damroll","Save-Fortitude",
    "Save-Reflex","Save-Will","Spell-Resist","Armor-Class","Resist-Fire",
    "Resist-Cold","Resist-Air","Resist-Earth","Resist-Acid","Resist-Holy",
    "Resist-Electric","Resist-Unholy","Resist-Slashing","Resist-Piercing",
    "Resist-Bludgeoning","Resist-Sound","Resist-Poison","Resist-Disease",
    "Resist-Negative","Resist-Illusion","Resist-Mental","Resist-Light",
    "Resist-Energy","Resist-Water","Grant-Feat","Skill-Bonus","Power-Resist",
    "HP-Regen","MV-Regen","PSP-Regen","Encumbrance","Fast-Healing","Initiative",
    "Spell-Circle-1","Spell-Circle-2","Spell-Circle-3","Spell-Circle-4",
    "Spell-Circle-5","Spell-Circle-6","Spell-Circle-7","Spell-Circle-8",
    "Spell-Circle-9","Spell-Potency","Spell-DC","Spell-Duration","Spell-Penetration"
];

// 4) Build level‐buckets (1–5, 6–10, … up to 26–30)
$buckets = [];
for ($min = 1; $min <= 30; $min += 5) {
    $max = min(30, $min + 4);
    $buckets[] = [
        'min'   => $min,
        'max'   => $max,
        'label' => "{$min}-{$max}"
    ];
}

// 5) Initialize matrix and row‐totals
$matrix    = [];  // matrix[bonus][bucket_idx] = count
$row_totals = []; // grand total per bonus

foreach ($bonus_types as $bonus) {
    $row_totals[$bonus] = 0;
    $matrix[$bonus] = array_fill(0, count($buckets), 0);
}

// 6) Query: count items by bonus & bucket for this slot
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
$stmt = $pdo->prepare($sql);
$stmt->execute([':slot' => $slot]);

while ($row = $stmt->fetch(PDO::FETCH_ASSOC)) {
    $bonus     = $row['bonus'];
    $idx       = (int)$row['bucket_idx'];
    $count     = (int)$row['cnt'];

    if (isset($matrix[$bonus][$idx])) {
        $matrix[$bonus][$idx] = $count;
        $row_totals[$bonus]  += $count;
    }
}

// 7) Render HTML table
echo "<h2>Breakdown for Wear Slot: <em>" . htmlspecialchars($slot) . "</em></h2>";
echo "<table border='1' cellpadding='4' cellspacing='0'>";
echo "<tr><th>Bonus Type</th><th>Total</th>";
foreach ($buckets as $b) {
    echo "<th>{$b['label']}</th>";
}
echo "</tr>";

foreach ($bonus_types as $bonus) {
    echo "<tr>";
    echo "<td><strong>{$bonus}</strong></td>";
    echo "<td><strong>" . ($row_totals[$bonus] ?: '') . "</strong></td>";

    foreach ($matrix[$bonus] as $cnt) {
        echo "<td>" . ($cnt ?: '') . "</td>";
    }

    echo "</tr>";
}

echo "</table>";
?>
</body>
</html>