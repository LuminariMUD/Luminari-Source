/**
* @file constants.c
* Numeric and string contants used by the MUD.
*
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
*
* All rights reserved.  See license for complete information.
* Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
* CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
*
* @todo Come up with a standard for descriptive arrays. Either all end with
* newlines or all of them don not.
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"	/* alias_data */
#include "spells.h"

/** Current tbaMUD version.
 * @todo defined with _TBAMUD so we don't have multiple constants to change.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "LuminariMUD 1.0 (tbaMUD 3.64)";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */

/* note - in utils.c there are two functions for alignment as well 
 * char *get_align_by_num(int align)
 * char *get_align_by_num_cnd(int align)
*/
const char *alignment_names[] = {
  "\tYLawful \tWGood\tn",
  "\tcNeutral \tWGood\tn",
  "\tRChaotic \tWGood\tn",
  "\tYLawful \tcNeutral\tn",
  "\tcTrue Neutral\tn",
  "\tRChaotic \tcNeutral\tn",
  "\tYLawful \tDEvil\tn",
  "\tcNeutral \tDEvil\tn",
  "\tRChaotic \tDEvil\tn",
  "\n"
};


/* structure for immortal prefix */
const char *admin_level_names[] = {
  "\tB[  \tC Staff   \tB ]\tn", // LVL_IMMORTAL
  "\tB[\tCSenior Staff\tB]\tn", // LVL_GOD
  "\tB[\tCWorld Forger\tB]\tn", // LVL_GRGOD
  "\tB[   \tCForger\tB   ]\tn", // LVL_IMPL
  "\n",
};

const char *craft_type[NUM_CRAFT+1] = {
  "RESERVED",
  "brew",
  "craft",
  "fletch",
  "knit",
  "mine",
  "disenchant",
  "synthesize",
  "hunt",
  "forest",
  "divide",
  "resize",
  "augment",
  "work on a supplyorder for",
  "convert",
  "restring",
  "wand-craft",
  "staff-craft",
  "\n"
};
  
const char *size_names[NUM_SIZES+1] = {
  "RESERVED",
  "Fine",
  "Diminutive",
  "Tiny",
  "Small",
  "Medium",
  "Large",
  "Huge",
  "Gargantuan",  
  "Colossal",
  "\n"
};


const char *damtype_display[] = {
  "RESERVED",
  "Damage Fire",
  "Damage Cold",
  "Damage Air",
  "Damage Earth",
  "Damage Acid",
  "Damage Holy",
  "Damage Electric",
  "Damage Unholy",
  "Damage Slice",
  "Damage Puncture",
  "Damage Force",
  "Damage Sound",
  "Damage Poison",
  "Damage Disease",
  "Damage Negative",
  "Damage Illusion",
  "Damage Mental",
  "Damage Light",
  "Damage Energy",
  "Damage Water",
  "\n"
};


/** Description of cardinal directions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest", /* Diagonals only used if CONFIG_DIAGONAL_DIRS is set */
  "northeast",
  "southeast",
  "southwest",
  "\n"
};

const char *autoexits[] =
{
  "N",
  "E",
  "S",
  "W",
  "U",
  "D",
  "nw",
  "ne",
  "se",
  "sw",
  "\n"
};

/** Room flag descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *room_bits[] = {
  "Dark",
  "Death-Trap",
  "No-Mob",
  "Indoors",
  "Peaceful",
  "Soundproof",
  "No-Track",
  "No-Magic",
  "Tunnel",
  "Private",
  "Staff-Room",
  "House",
  "House-Crash",
  "Atrium",
  "OLC",
  "*",				/* The BFS Mark. */
  "Worldmap",
  "Regenerating",
  "\n"
};


// room affect effect
const char *room_affections[] = {
  "Fogged",  
  "Darkened",
  "Lighted",
  "Stench",
  "Billowing",
  "Anti-Magic Field",
  "Acid Fogged",
  "\n"
};


/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[] = {
  "CLOSED",
  "NO_IMMORT",
  "QUEST",
  "GRID",
  "NOBUILD",
  "!ASTRAL",
  "WORLDMAP",
  "!CLAIM",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Low Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "Zone Entrance",
  "Road North-South",
  "Road East-West",
  "Road Intersection",
  "Desert",
  "Ocean",
  "Marshland",
  "High Mountains",
  "\n"
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[] = {
  "KILLER",		//0
  "THIEF",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",		//5
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",		//10
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",		//15
  "DEAD",    /* You should never see this flag on a character in game. */
  "IBT_BUG",
  "IBT_IDEA",
  "IBT_TYPO",
  "\n"
};

/** Mob action flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *action_bits[] = {
  "<spec>",
  "Sentinel",
  "Scavenger",
  "Mob",
  "Aware",
  "Aggro",  //5
  "Zone-Sentinel",
  "Wimpy",
  "Aggro-Evil",
  "Aggro-Good",
  "Aggro-Neutral",  //10
  "Memory",
  "Helper",
  "Uncharmable",
  "Unsummonable",
  "Immune-Sleep",  //15
  "Unbashable",
  "Unblindable",
  "Unkillable",
  "Sentient",
  "!DEAD!",    /* You should never see this. (20)*/
  "Mountable",
  "Immune-Deaf",
  "Does-Not-Fight",
  "Classless",
  "Ungrappleable",  //25
  "Animal-Companion",
  "Familiar",
  "Paladin-Mount",
  "Summoned-Elemental",
  "Animated-Dead",  //30
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "NO_SHOUT",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "C1",
  "C2",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "AUTOSAC",
  "AUTOASSIST",
  "AUTOMAP",
  "AUTOKEY",
  "AUTODOOR",
  "NO_CT",
  "AUTOSCAN",
  "Display-Exp",
  "Display-Exits",
  "Display-Room",
  "Display-Memtime",
  "\n"
};

/** Affected bits.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "Blinded",
  "Invisible",
  "Detect-Alignment",
  "Detect-Invisible",
  "Detect-Magic",  //5
  "Sense-Life",
  "Water-Walk",
  "Sanctuary",
  "Groupped",
  "Cursed",  //10
  "Infravision",
  "Poison",
  "Protect-Evil",
  "Protect-Good",
  "Sleep",  //15
  "Not-Trackable",
  "Flying",
  "Underwater-Breathing",
  "Sneaking",
  "Hiding",  //20
  "!UNUSED!",
  "Charmed",
  "Blurred",
  "Mode-PowerAttack",
  "Mode-Expertise",  //25
  "Hasted",
  "Mode-Parry",
  "Protect-Elements",
  "Deaf",
  "Fear",  //30
  "Stunned",
  "Paralyzed",
  "Ultravision",
  "Grappled",
  "Tamed",  //35
  "Climb",
  "Nauseated",
  "Non-Detection",
  "Slowed",
  "Fire-Shielded",  //40
  "Cold-Shielded",
  "Minor-Globed",
  "Acid-Shielded",
  "Size-Changed",
  "True-Sight",  //45
  "Spot-Mode",
  "Fatigued",
  "Regenning",
  "Diseased",
  "T-Transform",  //50
  "Globe of Invulnerability",
  "Listen-Mode",
  "Displaced",
  "Spell-Mantled",
  "Confused",  //55
  "Refuged",
  "Spell Turning",
  "\n"
};


/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[] = {
  "Playing",		//0
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",		//5
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",		//10
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",	//15
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",		//20
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Social edit",	//25
  "Trigger edit",
  "Help edit",
  "Quest edit",
  "Preference edit",
  "IBT edit",		//30
  "Protocol Detection",
  "Select Race",
  "Clan edit",
  "Message Edit",
  "Spells Known Edit",	//35
  "Class Help",	//36
  "Alignment Selection",	//37
  "\n"  /* make sure this matches NUM_CON_STATES:  38 */
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \n. */
const char *wear_where[] = {
  "\tY{Used As Light}\tn      ",
  "\tY{Worn On Finger}\tn     ",
  "\tY{Worn On Finger}\tn     ",
  "\tY{Worn Around Neck}\tn   ",
  "\tY{Worn Around Neck}\tn   ",
  "\tY{Worn On Body}\tn       ",
  "\tY{Worn On Head}\tn       ",
  "\tY{Worn On Legs}\tn       ",
  "\tY{Worn On Feet}\tn       ",
  "\tY{Worn On Hands}\tn      ",
  "\tY{Worn On Arms}\tn       ",
  "\tY{Worn As Shield}\tn     ",
  "\tY{Worn About Body}\tn    ",
  "\tY{Worn About Waist}\tn   ",
  "\tY{Worn Around Wrist}\tn  ",
  "\tY{Worn Around Wrist}\tn  ",
  "\tY{Wielded}\tn            ",
  "\tY{Held}\tn               ",
  "\tY{Wielded Offhand}\tn    ",
  "\tY{Held Offhand}\tn       ",
  "\tY{Wielded Twohanded}\tn  ",
  "\tY{Held Twohanded}\tn     "
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Wielded offhand",
  "Held offhand",
  "Wielded twohands",
  "Held twohands",
  "\n"
};


// labels for item proficiency category
const char *item_profs[] = {
  "None Required",
  "Minimal Weapon Proficiency",
  "Basic Weapon Proficiency",
  "Advanced Weapon Proficiency",
  "Master Weapon Proficiency",
  "Exotic Weapon Proficiency",
  "Light Armor Proficiency",
  "Medium Armor Proficiency",
  "Heavy Armor Proficiency",
  "Shield Proficiency",
  "Tower Shield Proficiency",
  "\n"
};

const char *material_name[NUM_MATERIALS+1] = {
  "RESERVED",
  "cotton",
  "leather",
  "glass",
  "gold",
  "organic",
  "paper",
  "steel",
  "wood",
  "bone",
  "crystal",
  "ether",
  "adamantine",
  "mithril",
  "iron",
  "copper",
  "ceramic",
  "satin",
  "silk",
  "dragonhide",
  "burlap",
  "velvet",
  "platinum",
  "obsidian",
  "wool",
  "onyx",
  "ivory",
  "brass",
  "marble",
  "bronze",
  "pewter",
  "ruby",
  "sapphire",
  "emerald",
  "gemstone",
  "granite",
  "stone",
  "energy",
  "hemp",
  "diamond",
  "earth",
  "silver",
  "alchemal silver",
  "cold iron",
  "darkwood",
  "\n"
};


/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[] = {
  "UNDEFINED",
  "Light",
  "Scroll",
  "Wand",
  "Staff",
  "Weapon",
  "Furniture",
  "FREE",
  "Treasure",
  "Armor",
  "Potion",
  "Wearable",
  "OTHER",
  "Trash",
  "FREE2",
  "Container",
  "Note",
  "Liquid-Cont",
  "Key",
  "Food",
  "Money",
  "Pen",
  "Boat",
  "Fountain",
  "Clan-Armor",
  "Crystal",
  "Essence",
  "Crafting Material",
  "Spellbook",
  "Portal",
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[] = {
  "(Takeable)",
  "Finger",
  "Neck",
  "Body",
  "Head",
  "Legs",
  "Feet",
  "Hands",
  "Arms",
  "Shield",
  "About-Body",
  "Waist",
  "Wrist",
  "Wield",
  "Hold",
  "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[] = {
  "Glows",  //0
  "Hums",
  "Not-Rentable",
  "Not-Donateable",
  "Immune-Invis",
  "Invisible",  //5
  "Magical",
  "Not-Droppable",
  "Blessed",
  "Anti-Good",
  "Anti-Evil",  //10
  "Anti-Neutral",
  "Anti-Wizard",
  "Anti-Cleric",
  "Anti-Rogue",
  "Anti-Warrior",  //15
  "Not-Sellable",
  "Quest-Item",
  "Anti-Human",
  "Anti-Elf",
  "Anti-Dwarf",  //20
  "Anti-Troll",
  "Anti-Monk",
  "Anti-Druid",
  "Mold",
  "Anti-Crystal-Dwarf",  //25
  "Anti-Halfling",
  "Anti-Half-Elf",
  "Anti-Half-Orc",
  "Anti-Gnome",
  "Anti-Berserker",  //30
  "Anti-Trelux",
  "Anti-Sorcerer",
  "Decaying",
  "Anti-Paladin",
  "Anti-Ranger",  //35
  "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[] = {
  "<None>",
  "Strength",
  "Dexterity",
  "Intelligence",
  "Wisdom",
  "Constitution",
  "Charisma",
  "Class",
  "Level",
  "Age",
  "Weight",
  "Height",
  "Max-Mana",
  "Max-HP",
  "Max-Move",
  "Gold",
  "Experience",
  "Armor-Class",
  "Hitroll",
  "Damroll",
  "Save-Fortitude",
  "Save-Reflex",
  "Save-Will",
  "Save-Poison",
  "Save-Death",
  "Spell-Resist",
  "Size",
  "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[] = {
  "Closeable",
  "Pick-Proof",
  "Closed",
  "Locked",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/** Describes a one word alias for each type of liquid.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};

/** Define the effect of liquids on hunger, thirst, and drunkenness, in that
 * order. See values.doc for more information.
 * @pre Must be in the same order as the defines. */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};

/** Describes the color of the various drinks.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/** Used to describe the level of fullness of a drink container. Not used in
 * sprinttype() so no \n. */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


const char *portal_types[] = 
{ 
   "Normal", 
   "Random", 
   "CheckFlags", 
   "Clanhall", 
   "\n" 
}; 


int spell_bonus[51][10] = {
  // 1     2    3    4    5    6    7    8    9    10
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },	//casting stat, 0
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },	//10
  {  0,    0,   0,   0,   0,   0,   0,   0,   0,    0 },
  {  1,    0,   0,   0,   0,   0,   0,   0,   0,    0 },	//12
  {  1,    0,   0,   0,   0,   0,   0,   0,   0,    0 },	//13
  {  1,    1,   0,   0,   0,   0,   0,   0,   0,    0 },	//14
  {  1,    1,   0,   0,   0,   0,   0,   0,   0,    0 },	//15
  {  1,    1,   1,   0,   0,   0,   0,   0,   0,    0 },	//16
  {  1,    1,   1,   0,   0,   0,   0,   0,   0,    0 },	//17
  {  1,    1,   1,   1,   0,   0,   0,   0,   0,    0 },	//18
  {  1,    1,   1,   1,   0,   0,   0,   0,   0,    0 },	//19
  {  2,    1,   1,   1,   1,   0,   0,   0,   0,    0 },	//20
  {  2,    1,   1,   1,   1,   0,   0,   0,   0,    0 },	//21
  {  2,    2,   1,   1,   1,   1,   0,   0,   0,    0 },	//22
  {  2,    2,   1,   1,   1,   1,   0,   0,   0,    0 },	//23
  {  2,    2,   2,   1,   1,   1,   1,   0,   0,    0 },	//24
  {  2,    2,   2,   1,   1,   1,   1,   0,   0,    0 },	//25
  {  2,    2,   2,   2,   1,   1,   1,   1,   0,    0 },	//26
  {  2,    2,   2,   2,   1,   1,   1,   1,   0,    0 },	//27
  {  3,    2,   2,   2,   2,   1,   1,   1,   1,    0 },	//28
  {  3,    2,   2,   2,   2,   1,   1,   1,   1,    0 },	//29
  {  3,    3,   2,   2,   2,   2,   1,   1,   1,    0 },	//30
  {  3,    3,   2,   2,   2,   2,   1,   1,   1,    0 },	//31
  {  3,    3,   3,   2,   2,   2,   2,   1,   1,    0 },	//32
  {  3,    3,   3,   2,   2,   2,   2,   1,   1,    0 },	//33
  {  3,    3,   3,   3,   2,   2,   2,   2,   1,    0 },	//34
  {  3,    3,   3,   3,   2,   2,   2,   2,   1,    0 },	//35
  {  4,    3,   3,   3,   3,   2,   2,   2,   2,    0 },	//36
  {  4,    3,   3,   3,   3,   2,   2,   2,   2,    0 },	//37
  {  4,    4,   3,   3,   3,   3,   2,   2,   2,    0 },	//38
  {  4,    4,   3,   3,   3,   3,   2,   2,   2,    0 },	//39
  {  4,    4,   4,   3,   3,   3,   3,   2,   2,    0 },	//40
  {  4,    4,   4,   3,   3,   3,   3,   2,   2,    0 },	//41
  {  4,    4,   4,   4,   3,   3,   3,   3,   2,    0 },	//42
  {  4,    4,   4,   4,   3,   3,   3,   3,   2,    0 },	//43
  {  5,    4,   4,   4,   4,   3,   3,   3,   3,    0 },	//44
  {  5,    4,   4,   4,   4,   3,   3,   3,   3,    0 },	//45
  {  5,    5,   4,   4,   4,   4,   3,   3,   3,    0 },	//46
  {  5,    5,   4,   4,   4,   4,   3,   3,   3,    0 },	//47
  {  5,    5,   5,   4,   4,   4,   4,   3,   3,    0 },	//48
  {  5,    5,   5,   4,   4,   4,   4,   3,   3,    0 },	//49
  {  5,    5,   5,   5,   4,   4,   4,   4,   3,    0 }	//50
};

/** Strength attribute affects.
 * The fields are hit mod, damage mod, weight carried mod, and weight wielded
 * mod. */
cpp_extern const struct str_app_type str_app[] = {
  {-5, -5, 3, 1},	/* str = 0 */
  {-5, -5, 10, 1}, //1
  {-4, -4, 20, 2}, //2
  {-4, -4, 30, 3}, //3
  {-3, -3, 40, 4}, //4
  {-3, -3, 55, 5}, //5	/* str = 5 */
  {-2, -2, 80, 6}, //6
  {-2, -2, 90, 7}, //7
  {-1, -1, 100, 8}, //8
  {-1, -1, 100, 9}, //9
  {0, 0, 115, 10}, //10	/* str = 10 */
  {0, 0, 115, 11}, //11
  {1, 1, 140, 12}, //12
  {1, 1, 140, 13}, //13
  {2, 2, 170, 14}, //14
  {2, 2, 200, 15}, //15	/* str = 15 */
  {3, 3, 230, 16}, //16
  {3, 3, 300, 18}, //17
  {4, 4, 300, 20}, //18	/* str = 18 */
  {4, 4, 400, 22}, //19
  {5, 5, 400, 25}, //20	/* str = 20 */
  {5, 5, 520, 28}, //21
  {6, 6, 520, 32}, //22
  {6, 6, 700, 36}, //23
  {7, 7, 700, 40}, //24
  {7, 7, 920, 44}, //25	/* str = 25 */
  {8, 8, 920, 48}, //26
  {8, 8, 1200, 52}, //27
  {9, 9, 1200, 56}, //28
  {9, 9, 1500, 60}, //29
  {10, 10, 1500, 64}, //30
  {10, 10, 1850, 68}, //31
  {11, 11, 1850, 72}, //32
  {11, 11, 2250, 76}, //33
  {12, 12, 2250, 80}, //34
  {12, 12, 2700, 84}, //35
  {13, 13, 2700, 88}, //36
  {13, 13, 3200, 92}, //37
  {14, 14, 3200, 96}, //38
  {14, 14, 3750, 100}, //39
  {15, 15, 4300, 104}, //40
  {15, 15, 4900, 108}, //41
  {16, 16, 5400, 112}, //42
  {16, 16, 6050, 116}, //43
  {17, 17, 6700, 120}, //44
  {17, 17, 7400, 124}, //45
  {18, 18, 8100, 128}, //46
  {18, 18, 8850, 132}, //47
  {19, 19, 9600, 136}, //48
  {19, 19, 9700, 140}, //49
  {20, 20, 9800, 140}, //50
  {20, 20, 9999, 140}, //51
};

/** Dexterity skill modifiers for thieves.
 * The fields are for pick pockets, pick locks, find traps, sneak and hide. */
cpp_extern const struct dex_skill_type dex_app_skill[] = {
  {-99, -99, -90, -99, -60},	/* dex = 0 */
  {-90, -90, -60, -90, -50},	/* dex = 1 */
  {-80, -80, -40, -80, -45},
  {-70, -70, -30, -70, -40},
  {-60, -60, -30, -60, -35},
  {-50, -50, -20, -50, -30},	/* dex = 5 */
  {-40, -40, -20, -40, -25},
  {-30, -30, -15, -30, -20},
  {-20, -20, -15, -20, -15},
  {-15, -10, -10, -20, -10},
  {-10, -5, -10, -15, -5},	/* dex = 10 */
  {-5, 0, -5, -10, 0},
  {0, 0, 0, -5, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},		/* dex = 15 */
  {0, 5, 0, 0, 0},
  {5, 10, 0, 5, 5},
  {10, 15, 5, 10, 10},		/* dex = 18 */
  {15, 20, 10, 15, 15},
  {15, 20, 10, 15, 15},		/* dex = 20 */
  {20, 25, 10, 15, 20},
  {20, 25, 15, 20, 20},
  {25, 25, 15, 20, 20},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},		/* dex = 25 */
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}, //30
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}, //35
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}, //40
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}, //45
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25},
  {25, 30, 15, 25, 25}, //50
  {25, 30, 15, 25, 25}
};

/** Dexterity attribute affects.
 * The fields are reaction, missile attacks, and defensive (armor class). */
cpp_extern const struct dex_app_type dex_app[] = {
  {-5, -5, -5},		/* dex = 0 */
  {-5, -5, -5},		/* dex = 1 */
  {-4, -4, -4},		//2
  {-4, -4, -4},		//3
  {-3, -3, -3},		//4
  {-3, -3, -3},		/* dex = 5 */
  {-2, -2, -2},		//6
  {-2, -2, -2},		//7
  {-1, -1, -1},		//8
  {-1, -1, -1},		//9
  {0, 0, 0},		/* dex = 10 */
  {0, 0, 0},		//11
  {1, 1, 1},		//12
  {1, 1, 1},		//13
  {2, 2, 2},		//14
  {2, 2, 2},		/* dex = 15 */
  {3, 3, 3},		//16
  {3, 3, 3},		//17
  {4, 4, 4},		/* dex = 18 */
  {4, 4, 4},		//19
  {5, 5, 5},		/* dex = 20 */
  {5, 5, 5},		//21
  {6, 6, 6},		//22
  {6, 6, 6},		//23
  {7, 7, 7},		//24
  {7, 7, 7},		/* dex = 25 */
  {8, 8, 8},		//26
  {8, 8, 8},		//27
  {9, 9, 9},		//28
  {9, 9, 9},		//29
  {10, 10, 10},		//30
  {10, 10, 10},		//31
  {11, 11, 11},		//32
  {11, 11, 11},		//33
  {12, 12, 12},		//34
  {12, 12, 12},		//35
  {13, 13, 13},		//36
  {13, 13, 13},		//37
  {14, 14, 14},		//38
  {14, 14, 14},		//39
  {15, 15, 15},		//40
  {15, 15, 15},		//41
  {16, 16, 16},		//42
  {16, 16, 16},		//43
  {17, 17, 17},		//44
  {17, 17, 17},		//45
  {18, 18, 18},		//46
  {18, 18, 18},		//47
  {19, 19, 19},		//48
  {19, 19, 19},		//49
  {20, 20, 20},		//50
  {20, 20, 20}		//51
};

/** Constitution attribute affects.
 * The field referenced is for hitpoint bonus. */
cpp_extern const struct con_app_type con_app[] = {
  {-5},		/* con = 0 */
  {-5},		/* con = 1 */
  {-4},		// 2
  {-4},		//3
  {-3},		//4
  {-3},		/* con = 5 */
  {-2},		//6
  {-2},		//7
  {-1},		//8
  {-1},		//9
  {0},		/* con = 10 */
  {0},		//11
  {1},		//12
  {1},		//13
  {2},		//14
  {2},		/* con = 15 */
  {3},		//16
  {3},		//17
  {4},		/* con = 18 */
  {4},		//19
  {5},		/* con = 20 */
  {5},		//21
  {6},		//22
  {6},		//23
  {7},		//24
  {7},		/* con = 25 */
  {8},		/* con = 26 */
  {8},		/* con = 27 */
  {9},		//28
  {9},		//29
  {10},		//30
  {10},		//31
  {11},		//32
  {11},		//33
  {12},		//34
  {12},		//35
  {13},		//36
  {13},		//37
  {14},		//38
  {14},		//39
  {15},		//40
  {15},		//41
  {16},		//42
  {16},		//43
  {17},		//44
  {17},		//45
  {18},		//46
  {18},		//47
  {19},		//48
  {19},		//49
  {20},		//50
  {20}		/* con = 51 */
};

/** Intelligence attribute affects. */
cpp_extern const struct int_app_type int_app[] = {
  {-5},		//0
  {-5},		//1
  {-4},		// 2
  {-4},		//3
  {-3},		//4
  {-3},		//5
  {-2},		//6
  {-2},		//7
  {-1},		//8
  {-1},		//9
  {0},		//10
  {0},		//11
  {1},		//12
  {1},		//13
  {2},		//14
  {2},		//15
  {3},		//16
  {3},		//17
  {4},		//18
  {4},		//19
  {5},		//20
  {5},		//21
  {6},		//22
  {6},		//23
  {7},		//24
  {7},		//25
  {8},		//26
  {8},		//27
  {9},		//28
  {9},		//29
  {10},		//30
  {10},		//31
  {11},		//32
  {11},		//33
  {12},		//34
  {12},		//35
  {13},		//36
  {13},		//37
  {14},		//38
  {14},		//39
  {15},		//40
  {15},		//41
  {16},		//42
  {16},		//43
  {17},		//44
  {17},		//45
  {18},		//46
  {18},		//47
  {19},		//48
  {19},		//49
  {20},		//50
  {20}		/* 51 */
};

/** Wisdom attribute affects. */
cpp_extern const struct wis_app_type wis_app[] = {
  {-5},		/*  0 */
  {-5},		/*  1 */
  {-4},		// 2
  {-4},		//3
  {-3},		//4
  {-3},		/* 5 */
  {-2},		//6
  {-2},		//7
  {-1},		//8
  {-1},		//9
  {0},		/*  10 */
  {0},		//11
  {1},		//12
  {1},		//13
  {2},		//14
  {2},		/* 15 */
  {3},		//16
  {3},		//17
  {4},		/* 18 */
  {4},		//19
  {5},		/* 20 */
  {5},		//21
  {6},		//22
  {6},		//23
  {7},		//24
  {7},		/* 25 */
  {8},		/* 26 */
  {8},		/* 27 */
  {9},		//28
  {9},		//29
  {10},		//30
  {10},		//31
  {11},		//32
  {11},		//33
  {12},		//34
  {12},		//35
  {13},		//36
  {13},		//37
  {14},		//38
  {14},		//39
  {15},		//40
  {15},		//41
  {16},		//42
  {16},		//43
  {17},		//44
  {17},		//45
  {18},		//46
  {18},		//47
  {19},		//48
  {19},		//49
  {20},		//50
  {20}		/* 51 */
};

/** Charisma attribute affects. */
cpp_extern const struct cha_app_type cha_app[] = {
  {-5},		/* 0 */
  {-5},		/* 1 */
  {-4},		// 2
  {-4},		//3
  {-3},		//4
  {-3},		/* 5 */
  {-2},		//6
  {-2},		//7
  {-1},		//8
  {-1},		//9
  {0},		/* 10 */
  {0},		//11
  {1},		//12
  {1},		//13
  {2},		//14
  {2},		/*  15 */
  {3},		//16
  {3},		//17
  {4},		/* 18 */
  {4},		//19
  {5},		/* 20 */
  {5},		//21
  {6},		//22
  {6},		//23
  {7},		//24
  {7},		/* 25 */
  {8},		/* 26 */
  {8},		/* 27 */
  {9},		//28
  {9},		//29
  {10},		//30
  {10},		//31
  {11},		//32
  {11},		//33
  {12},		//34
  {12},		//35
  {13},		//36
  {13},		//37
  {14},		//38
  {14},		//39
  {15},		//40
  {15},		//41
  {16},		//42
  {16},		//43
  {17},		//44
  {17},		//45
  {18},		//46
  {18},		//47
  {19},		//48
  {19},		//49
  {20},		//50
  {20}		/* 51 */
};

/* lore values */
int lore_app[] = {
	0,		//0
	50,		//1
	100,
	150,
	200,
	300,		//5
	500,
	1000,
	2500,
	3750,
	4800,		//10
	6500,
	13000,
	17000,
	20000,
	30000,		//15
	40000,
	60000,
	80000,
	100000,
	150000,		//20
	200000,
	250000,
	300000,
	400000,
	500000,		//25
	600000,
	700000,
	800000,
	900000,
	1000000,	//30
	1200000,
	1400000,
	1600000,
	1800000,
	2000000,	//35
	2200000,
	2400000,
	2600000,
	2800000,
	3000000,	//40
	3200000,
	3400000,
	3600000,
	3800000,
	4000000,	//45
	4000000,	//46
	4000000,	//47
	4000000,	//48
	4000000,	//49
	4000000,	//50
	4000000,	//51
	4000000,	//52
	4000000,	//53
	4000000,	//54
	4000000,	//55
	4000000,	//56
	4000000,	//57
	4000000,	//58
	4000000,	//59
	4000000,	//60
	4000000,	//61
	4000000,	//62
	4000000,	//63
	4000000,	//64
	4000000,	//65
	4000000,	//66
	4000000,	//67
	4000000,	//68
	4000000,	//69
	4000000		//70
};



/** Define a set of opposite directions from the cardinal directions. */
int rev_dir[] =
{
  SOUTH,
  WEST,
  NORTH,
  EAST,
  DOWN,
  UP,
  SOUTHEAST,
  SOUTHWEST,
  NORTHWEST,
  NORTHEAST
};

/** How much movement is lost moving through a particular sector type. */
/* make sure it matches SECT_ */
int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  7,	/* Mountains  */
  4,	/* Swimming   */
  3,	/* Unswimable */
  1,	/* Flying     */
  5,   /* Underwater */
  1,	// zone entrance
  1,	// road north-south
  1,	// road east-west
  1,	// road intersection
  3,	// Desert
  1,	// Ocean
  5,	// Marshland
  10	// High Mountain
};

/** The names of the days of the mud week. Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the \tDMoon\tn",
  "the Day of the \tMBull\tn",
  "the Day of the \tDDeception\tn",
  "the Day of \tbThunder\tn",
  "the Day of \tWFreedom\tn",
  "the Day of the \tCGreat Powers\tn",
  "the Day of the \tYSun\tn"
};

/** The names of the mud months. Not used in sprinttype(). */
const char *month_name[] = {
  "Month of \tWWinter\tn",		/* 0 */
  "Month of the \tWWinter \tcWolf\tn",
  "Month of the \tWFrost \tYGiant\tn",
  "Month of the \tCOld \tDForces\tn",
  "Month of the \twGrand \tGStruggle\tn",
  "Month of the \tYSpr\tGing\tn",
  "Month of \tGN\tYa\tGt\tYu\tGr\tYe\tn",
  "Month of \tmFutility\tn",
  "Month of the \tRDragon\tn",
  "Month of the \tYSun\tn",
  "Month of the \tRHe\tYat\tn",
  "Month of the \tRBat\tDtle\tn",
  "Month of the \tDDark \twSha\tDdes\tn",
  "Month of the \tDS\tch\tDa\tcd\tDo\tcw\tDs\tn",
  "Month of the \tCLong \tDShadows\tn",
  "Month of the \tWAncient \tDDarkness\tn",
  "Month of the \trGreat \tREvil\tn"
};

/** Names for mob trigger types. */
const char *trig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};

/** Names for object trigger types. */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED1",
  "UNUSED2",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED3",
  "Remove",
  "UNUSED4",
  "Load",
  "UNUSED5",
  "Cast",
  "Leave",
  "UNUSED6",
  "Consume",
  "Time",
  "\n"
};

/** Names for world (room) trigger types. */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED1",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED2",
  "UNUSED3",
  "UNUSED4",
  "UNUSED5",
  "UNUSED6",
  "UNUSED7",
  "UNUSED8",
  "Cast",
  "Leave",
  "Door",
  "Login",
  "Time",
  "\n"
};

/** The names of the different channels that history is stored for.
 * @todo Only referenced by do_history at the moment. Should be moved local
 * to that function. */
const char *history_types[] = {
  "all",
  "say",
  "gossip/chat",
  "wiznet",
  "tell",
  "shout",
  "grats",
  "holler",
  "auction",
  "clantalk",
  "\n"
};

/** Flag names for Ideas, Bugs and Typos (defined in ibt.h) */
const char *ibt_bits[] = {
  "Resolved",
  "Important",
  "InProgress",
  "\n"
};

/** ability defines corresponding names **/
const char *ability_names[] = {
  "RESERVED",
  "Tumble",
  "Hide",
  "Sneak",
  "Spot",
  "Listen",
  "Treat Injury",
  "Taunt",
  "Concentration",
  "Spellcraft",
  "Appraise",
  "Discipline",
  "Parry",
  "Lore",
  "Mount",
  "Riding",
  "Taming",
  "Pick Locks",
  "Steal"
};




/* --- End of constants arrays. --- */

/* Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared. */
  /** Number of defined room bit descriptions. */
  size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
  /** Number of defined action bit descriptions. */
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	/** Number of defined affected bit descriptions. */
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	/** Number of defined extra bit descriptions. */
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	/** Number of defined wear bit descriptions. */
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;

