#ifndef NEWCRAFT_RECIPES_H
#define NEWCRAFT_RECIPES_H

#define CRAFT_RECIPE_NONE                   0
#define CRAFT_RECIPE_WEAPON_UNARMED         1
#define CRAFT_RECIPE_WEAPON_DAGGER          2
#define CRAFT_RECIPE_WEAPON_LIGHT_MACE      3
#define CRAFT_RECIPE_WEAPON_SICKLE          4
#define CRAFT_RECIPE_WEAPON_CLUB            5
#define CRAFT_RECIPE_WEAPON_HEAVY_MACE      6
#define CRAFT_RECIPE_WEAPON_MORNINGSTAR     7
#define CRAFT_RECIPE_WEAPON_SHORTSPEAR      8
#define CRAFT_RECIPE_WEAPON_LONGSPEAR       9
#define CRAFT_RECIPE_WEAPON_QUARTERSTAFF    10
#define CRAFT_RECIPE_WEAPON_SPEAR           11
#define CRAFT_RECIPE_WEAPON_HEAVY_CROSSBOW  12
#define CRAFT_RECIPE_WEAPON_LIGHT_CROSSBOW  13
#define CRAFT_RECIPE_WEAPON_DART            14
#define CRAFT_RECIPE_WEAPON_JAVELIN         15
#define CRAFT_RECIPE_WEAPON_SLING           16
#define CRAFT_RECIPE_WEAPON_THROWING_AXE    17
#define CRAFT_RECIPE_WEAPON_LIGHT_HAMMER    18
#define CRAFT_RECIPE_WEAPON_HAND_AXE        19
#define CRAFT_RECIPE_WEAPON_KUKRI           20
#define CRAFT_RECIPE_WEAPON_LIGHT_PICK      21
#define CRAFT_RECIPE_WEAPON_SAP             22
#define CRAFT_RECIPE_WEAPON_SHORT_SWORD     23
#define CRAFT_RECIPE_WEAPON_BATTLE_AXE      24
#define CRAFT_RECIPE_WEAPON_FLAIL           25
#define CRAFT_RECIPE_WEAPON_LONG_SWORD      26
#define CRAFT_RECIPE_WEAPON_HEAVY_PICK      27
#define CRAFT_RECIPE_WEAPON_RAPIER          28
#define CRAFT_RECIPE_WEAPON_SCIMITAR        29
#define CRAFT_RECIPE_WEAPON_TRIDENT         30
#define CRAFT_RECIPE_WEAPON_WARHAMMER       31
#define CRAFT_RECIPE_WEAPON_FALCHION        32
#define CRAFT_RECIPE_WEAPON_GLAIVE          33
#define CRAFT_RECIPE_WEAPON_GREAT_AXE       34
#define CRAFT_RECIPE_WEAPON_GREAT_CLUB      35
#define CRAFT_RECIPE_WEAPON_HEAVY_FLAIL     36
#define CRAFT_RECIPE_WEAPON_GREAT_SWORD     37
#define CRAFT_RECIPE_WEAPON_GUISARME        39
#define CRAFT_RECIPE_WEAPON_HALBERD         40
#define CRAFT_RECIPE_WEAPON_LANCE           41
#define CRAFT_RECIPE_WEAPON_RANSEUR         42
#define CRAFT_RECIPE_WEAPON_SCYTHE          43
#define CRAFT_RECIPE_WEAPON_LONG_BOW        44
#define CRAFT_RECIPE_WEAPON_SHORT_BOW       45
#define CRAFT_RECIPE_WEAPON_COMPOSITE_LONGBOW 46
#define CRAFT_RECIPE_WEAPON_COMPOSITE_SHORTBOW 47
#define CRAFT_RECIPE_WEAPON_KAMA            48
#define CRAFT_RECIPE_WEAPON_NUNCHAKU        49
#define CRAFT_RECIPE_WEAPON_SAI             50
#define CRAFT_RECIPE_WEAPON_SIANGHAM        51
#define CRAFT_RECIPE_WEAPON_BASTARD_SWORD   52
#define CRAFT_RECIPE_WEAPON_DWARVEN_WAR_AXE 53
#define CRAFT_RECIPE_WEAPON_WHIP            54
#define CRAFT_RECIPE_WEAPON_SPIKED_CHAIN    55
#define CRAFT_RECIPE_WEAPON_DOUBLE_AXE      56
#define CRAFT_RECIPE_WEAPON_DIRE_FLAIL      57
#define CRAFT_RECIPE_WEAPON_HOOKED_HAMMER   58
#define CRAFT_RECIPE_WEAPON_2_BLADED_SWORD   59
#define CRAFT_RECIPE_WEAPON_DWARVEN_URGOSH  60
#define CRAFT_RECIPE_WEAPON_HAND_CROSSBOW   61
#define CRAFT_RECIPE_WEAPON_HEAVY_REP_XBOW  62
#define CRAFT_RECIPE_WEAPON_LIGHT_REP_XBOW  63
#define CRAFT_RECIPE_WEAPON_BOLA            64
#define CRAFT_RECIPE_WEAPON_NET             65
#define CRAFT_RECIPE_WEAPON_SHURIKEN        66
#define CRAFT_RECIPE_WEAPON_WARMAUL         67
#define CRAFT_RECIPE_WEAPON_KHOPESH         68
#define CRAFT_RECIPE_WEAPON_KNIFE           69
#define CRAFT_RECIPE_WEAPON_HOOPAK          70
#define CRAFT_RECIPE_WEAPON_FOOTMANS_LANCE  71
#define CRAFT_RECIPE_ARMOR_CLOTHING         72
#define CRAFT_RECIPE_ARMOR_PADDED           73
#define CRAFT_RECIPE_ARMOR_LEATHER          74
#define CRAFT_RECIPE_ARMOR_STUDDED_LEATHER  75
#define CRAFT_RECIPE_ARMOR_LIGHT_CHAIN      76
#define CRAFT_RECIPE_ARMOR_HIDE             77
#define CRAFT_RECIPE_ARMOR_SCALE            78
#define CRAFT_RECIPE_ARMOR_CHAINMAIL        79
#define CRAFT_RECIPE_ARMOR_PIECEMEAL        80
#define CRAFT_RECIPE_ARMOR_SPLINT           81
#define CRAFT_RECIPE_ARMOR_BANDED           82
#define CRAFT_RECIPE_ARMOR_HALF_PLATE       83
#define CRAFT_RECIPE_ARMOR_FULL_PLATE       84
#define CRAFT_RECIPE_ARMOR_BUCKLER          85
#define CRAFT_RECIPE_ARMOR_SMALL_SHIELD     86
#define CRAFT_RECIPE_ARMOR_LARGE_SHIELD     87
#define CRAFT_RECIPE_ARMOR_TOWER_SHIELD     88
#define CRAFT_RECIPE_ARMOR_CLOTHING_HEAD    89
#define CRAFT_RECIPE_ARMOR_PADDED_HEAD      90
#define CRAFT_RECIPE_ARMOR_LEATHER_HEAD     91
#define CRAFT_RECIPE_ARMOR_STUDDED_LEATHER_HEAD 92
#define CRAFT_RECIPE_ARMOR_LIGHT_CHAIN_HEAD 93
#define CRAFT_RECIPE_ARMOR_HIDE_HEAD        94
#define CRAFT_RECIPE_ARMOR_SCALE_HEAD       95
#define CRAFT_RECIPE_ARMOR_CHAINMAIL_HEAD   96
#define CRAFT_RECIPE_ARMOR_PIECEMEAL_HEAD   97
#define CRAFT_RECIPE_ARMOR_SPLINT_HEAD      98
#define CRAFT_RECIPE_ARMOR_BANDED_HEAD      99
#define CRAFT_RECIPE_ARMOR_HALF_PLATE_HEAD  100
#define CRAFT_RECIPE_ARMOR_FULL_PLATE_HEAD  101
#define CRAFT_RECIPE_ARMOR_CLOTHING_ARMS    102
#define CRAFT_RECIPE_ARMOR_PADDED_ARMS      103
#define CRAFT_RECIPE_ARMOR_LEATHER_ARMS     104
#define CRAFT_RECIPE_ARMOR_STUDDED_LEATHER_ARMS 105 
#define CRAFT_RECIPE_ARMOR_LIGHT_CHAIN_ARMS 106
#define CRAFT_RECIPE_ARMOR_HIDE_ARMS        107
#define CRAFT_RECIPE_ARMOR_SCALE_ARMS       108
#define CRAFT_RECIPE_ARMOR_CHAINMAIL_ARMS   109
#define CRAFT_RECIPE_ARMOR_PIECEMEAL_ARMS   110
#define CRAFT_RECIPE_ARMOR_SPLINT_ARMS      111
#define CRAFT_RECIPE_ARMOR_BANDED_ARMS      112
#define CRAFT_RECIPE_ARMOR_HALF_PLATE_ARMS  113
#define CRAFT_RECIPE_ARMOR_FULL_PLATE_ARMS  114
#define CRAFT_RECIPE_ARMOR_CLOTHING_LEGS    115
#define CRAFT_RECIPE_ARMOR_PADDED_LEGS      116
#define CRAFT_RECIPE_ARMOR_LEATHER_LEGS     117
#define CRAFT_RECIPE_ARMOR_STUDDED_LEATHER_LEGS 118 
#define CRAFT_RECIPE_ARMOR_LIGHT_CHAIN_LEGS 119
#define CRAFT_RECIPE_ARMOR_HIDE_LEGS        120
#define CRAFT_RECIPE_ARMOR_SCALE_LEGS       121
#define CRAFT_RECIPE_ARMOR_CHAINMAIL_LEGS   122
#define CRAFT_RECIPE_ARMOR_PIECEMEAL_LEGS   123
#define CRAFT_RECIPE_ARMOR_SPLINT_LEGS      124
#define CRAFT_RECIPE_ARMOR_BANDED_LEGS      125
#define CRAFT_RECIPE_ARMOR_HALF_PLATE_LEGS  126
#define CRAFT_RECIPE_ARMOR_FULL_PLATE_LEGS  127
#define CRAFT_RECIPE_MISC_RING              128
#define CRAFT_RECIPE_MISC_NECKLACE          129
#define CRAFT_RECIPE_MISC_BOOTS             130
#define CRAFT_RECIPE_MISC_GLOVES            131
#define CRAFT_RECIPE_MISC_CLOAK             132
#define CRAFT_RECIPE_MISC_BELT              133
#define CRAFT_RECIPE_MISC_BRACER            134
#define CRAFT_RECIPE_MISC_MASK              135
#define CRAFT_RECIPE_MISC_EARRING           136
#define CRAFT_RECIPE_MISC_GLASSES           137
#define CRAFT_RECIPE_INSTRUMENT_LYRE        138
#define CRAFT_RECIPE_INSTRUMENT_FLUTE       139
#define CRAFT_RECIPE_INSTRUMENT_HORN        140
#define CRAFT_RECIPE_INSTRUMENT_DRUM        141
#define CRAFT_RECIPE_INSTRUMENT_HARP        142
#define CRAFT_RECIPE_INSTRUMENT_MANDOLIN    143
#define CRAFT_RECIPE_MISC_PAULDRONS         144
#define CRAFT_RECIPE_MISC_ANKLET            145

#define NUM_CRAFTING_RECIPES                146

#define REFINE_RECIPE_NONE              0
#define REFINE_RECIPE_BRONZE            1
#define REFINE_RECIPE_STEEL             2
#define REFINE_RECIPE_SATIN             3
#define REFINE_RECIPE_BRASS             4
#define REFINE_RECIPE_LINEN             5
#define REFINE_RECIPE_ALCHEMICAL_SILVER 6
#define REFINE_RECIPE_COLD_IRON         7

#define NUM_REFINING_RECIPES            8

#define NUM_CRAFT_VARIANTS      10

struct craft_recipe_data
{
    const char *name;
    int object_type;
    int object_subtype;
    int variant_skill[NUM_CRAFT_VARIANTS]; // variants of the type. Eg. bracelet, bracer and wristguard
    int materials[3][NUM_CRAFT_VARIANTS][2]; // type and num units required Eg. bracelet needs soft metal 5 units, bracer needs leather 7 units
    const char *variant_descriptions[NUM_CRAFT_VARIANTS];
    int practical_type; // for weapons, this is the weapon type, for armor, the armor type, instruments, the intrument type, and others, the wear slot.
};

struct refine_recipe_data
{
    int skill;
    int dc;
    int materials[3][2];
    int result[2];
    int crafting_station_flag;
};

extern struct craft_recipe_data crafting_recipes[NUM_CRAFTING_RECIPES];
extern struct refine_recipe_data refining_recipes[NUM_REFINING_RECIPES];
void populate_crafting_recipes(void);
void initialize_refining_recipes(void);
void populate_refining_recipes(void);

#endif