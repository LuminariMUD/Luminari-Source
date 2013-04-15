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
#include "craft.h"

/** Current tbaMUD version.
 * @todo defined with _TBAMUD so we don't have multiple constants to change.
 * @todo cpp_extern isn't needed here (or anywhere) as the extern reserved word
 * works correctly with C compilers (at least in my Experience)
 * Jeremy Osborne 1/28/2008 */
cpp_extern const char *tbamud_version = "LuminariMUD 1.37 (tbaMUD 3.64)";

/* strings corresponding to ordinals/bitvectors in structs.h */
/* (Note: strings for class definitions in class.c instead of here) */


/* ranged weapon types (bows, etc) */
const char *ranged_weapons[NUM_RANGED_WEAPONS + 1] = {
  "bow",
  "crossbow",
  "\n"  
};

/* ranged weapon missiles (arrows, etc) */
const char *ranged_missiles[NUM_RANGED_MISSILES + 1] = {
  "arrow",
  "bolt",
  "\n"    
};

/* note - in utils.c there are two functions for alignment as well 
 * char *get_align_by_num(int align)
 * char *get_align_by_num_cnd(int align)
*/
const char *alignment_names[NUM_ALIGNMENTS + 1] = {
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
const char *admin_level_names[LVL_IMPL - LVL_IMMORT + 2] = {
  "\tB[  \tC Staff   \tB ]\tn", // LVL_IMMORTAL
  "\tB[\tCSenior Staff\tB]\tn", // LVL_STAFF
  "\tB[\tCWorld Forger\tB]\tn", // LVL_GRSTAFF
  "\tB[   \tCForger\tB   ]\tn", // LVL_IMPL
  "\n",
};

const char *craft_type[NUM_CRAFT + 1] = {
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
  
const char *size_names[NUM_SIZES + 1] = {
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


const char *damtype_display[NUM_DAM_TYPES + 1] = {
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
const char *dirs[NUM_OF_DIRS + 1] =
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

const char *autoexits[NUM_OF_DIRS + 1] =
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
const char *room_bits[NUM_ROOM_FLAGS + 1] = {
  "Dark",
  "Death-Trap",
  "No-Mob",
  "Indoors",
  "Peaceful",
  "Soundproof",  //5
  "No-Track",
  "No-Magic",
  "Tunnel",
  "Private",
  "Staff-Room",  //10
  "House",
  "House-Crash",
  "Atrium",
  "OLC",
  "*",  /* The BFS Mark. (15) */
  "Worldmap",
  "Regenerating",
  "Fly-Needed",
  "No-Recall",
  "Singlefile",  //20
  "No-Teleport",
  "Magical-Darkness",
  "Magical-Light",
  "No-Summon",
  "No-Heal",  //25
  "No-Fly",
  "Fogged",
  "Airy",
  "\n"
};


// room affect effect
const char *room_affections[NUM_RAFF + 1] = {
  "Fogged",  
  "Darkened",
  "Lighted",
  "Stench",
  "Billowing",
  "Anti-Magic Field",
  "Acid-Fogged",
  "Blade-Barriered",
  "Spike Growth",
  "Spike Stone",
  "Holy",
  "Unholy",
  "\n"
};


/** Room flag descriptions. (ZONE_x)
 * @pre Must be in the same order as the defines in structs.h.
 * Must end array with a single newline. */
const char *zone_bits[NUM_ZONE_FLAGS + 1] = {
  "Closed",
  "No-Immortal",
  "Quest",
  "Grid",
  "No-Build",
  "No-Teleport",
  "Worldmap",
  "Not-Claimable",
  "Astral-Plane",
  "Ethereal-Plane",
  "Elemental-Plane",
  "\n"
};

/** Exit bits for doors.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *exit_bits[NUM_EXIT_BITS + 1] = {
  "Door",
  "Closed",
  "Locked",
  "Pickproof",
  "Hidden",
  "\n"
};

/** Description of the room sector type.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *sector_types[NUM_ROOM_SECTORS + 1] = {
  "Inside",  //0
  "City",
  "Field",
  "Forest",
  "Hills",
  "Low Mountains",  //5
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "Zone Entrance",  //10
  "Road North-South",
  "Road East-West",
  "Road Intersection",
  "Desert",
  "Ocean",  //15
  "Marshland",
  "High Mountains",
  "Outer Planes",
  "Underdark - Wild",
  "Underdark - City",  //20
  "Underdark - Inside",
  "Underdark - Water (Swim)",
  "Underdark - Water (No Swim)",
  "Underdark - In Flight",
  "Lava",  //25
  "\n"
};

/** How much movement is lost moving through a particular sector type. */
/* make sure it matches SECT_ */
int movement_loss[NUM_ROOM_SECTORS] =
{
  1,	/* Inside     */  // 0
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  7,	/* Mountains  */  //5
  4,	/* Swimming   */
  3,	/* Unswimable */
  1,	/* Flying     */
  5,   /* Underwater */
  1,	// zone entrance  10
  1,	// road north-south
  1,	// road east-west
  1,	// road intersection
  3,	// Desert
  1,	// Ocean  15
  5,	// Marshland
  10,	// High Mountain
  6,	// planes
  3,	// ud wild
  1,	// ud city  20
  1,	// ud inside
  4,	// ud swim
  3,	// ud noswim
  1,	// ud flying
  6,	// lava  25
};

/** PC and NPC sex.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *genders[NUM_GENDERS + 1] =
{
  "neutral",
  "male",
  "female",
  "\n"
};

/** Character positions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *position_types[NUM_POSITIONS + 1] = {
  "Dead",  //0
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Reclining",  //5
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};

/** Player flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *player_bits[NUM_PLR_BITS + 1] = {
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
const char *action_bits[NUM_MOB_FLAGS + 1] = {
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
  "Guard",
  "Citizen",
  "Hunter",
  "Listen",
  "Lit-Up",  //35
  "Planar-Ally",
  "No-Steal",
  "\n"
};

/** PC Preference flags.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *preference_bits[NUM_PRF_FLAGS + 1] = {
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
const char *affected_bits[NUM_AFF_FLAGS + 1] =
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
  "Infra-Vision",
  "Poison",
  "Protect-Evil",
  "Protect-Good",
  "Sleep",  //15
  "Not-Trackable",
  "Flying",
  "Underwater-Breathing",
  "Sneaking",
  "Hiding",  //20
  "Vampiric-Curse",
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
  "Ultra-Vision",
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
  "Regenerating",
  "Diseased",
  "T-Transform",  //50
  "Globe-of-Invulnerability",
  "Listen-Mode",
  "Displaced",
  "Spell-Mantled",
  "Confused",  //55
  "Refuged",
  "Spell-Turning",
  "Mind-Blanked",
  "Shadow-Shielded",
  "Time-Stopped",  //60
  "Bravery",
  "Free-Movement",
  "Faerie-Fired",
  "Battletide",
  "Spell-Resistant", //65
  "Dimensional-Locked",
  "Death-Ward",
  "Mode-Spellbattle",
  "Vampiric-Touch",
  "Blackmantled",  //70
  "Danger-Sense",
  "SafeFall",
  "Tower-of-Iron-Will",
  "Inertial-Barrier",
  "Not-teleportable",  //75
  "DamageBoost",  // AFF_MAX_DAMAGE
  "Incorporeal",
  "Caged",
  "Mage-Flamed",
  "Dark-Vision",  //80
  "Body-Weaponry",
  "Farsee",
  "\n"
};


/** Connection type descriptions.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *connected_types[NUM_CON_STATES + 1] = {
  "Playing",		//0
  "Disconnecting",
  "Get Name",
  "Confirm Name",
  "Get Password",
  "Get New PW",		//5
  "Confirm New PW",
  "Select Sex",
  "Select Class",
  "Reading MOTD",
  "Main Menu",		//10
  "Get Descrip",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",	//15
  "Self-Delete 2",
  "Disconnecting",
  "Object Edit",
  "Room Edit",
  "Zone Edit",		//20
  "Mobile Edit",
  "Shop Edit",
  "Text Edit",
  "Config Edit",
  "Social Edit",	//25
  "Trigger Edit",
  "Help Edit",
  "Quest Edit",
  "Preference Edit",
  "IBT Edit",		//30
  "Protocol Detection",
  "Select Race",
  "Clan Edit",
  "Message Edit",
  "Spells Known Edit",	//35
  "Class Help",	//36
  "Alignment Selection",	//37
  "Race Help", // 38
  "HL Quest Edit",
  "\n"  /* make sure this matches NUM_CON_STATES */
};

/** Describes the position in the equipment listing.
 * @pre Must be in the same order as the defines.
 * Not used in sprinttype() so no \n. */
const char *wear_where[NUM_WEARS] = {
  "\tY{Used As Light}\tn      ",  //0
  "\tY{Worn On Finger}\tn     ",
  "\tY{Worn On Finger}\tn     ",
  "\tY{Worn Around Neck}\tn   ",
  "\tY{Worn Around Neck}\tn   ",
  "\tY{Worn On Body}\tn       ",  //5
  "\tY{Worn On Head}\tn       ",
  "\tY{Worn On Legs}\tn       ",
  "\tY{Worn On Feet}\tn       ",
  "\tY{Worn On Hands}\tn      ",
  "\tY{Worn On Arms}\tn       ",  //10
  "\tY{Worn As Shield}\tn     ",
  "\tY{Worn About Body}\tn    ",
  "\tY{Worn About Waist}\tn   ",
  "\tY{Worn Around Wrist}\tn  ",
  "\tY{Worn Around Wrist}\tn  ",  //15
  "\tY{Wielded}\tn            ",
  "\tY{Held}\tn               ",
  "\tY{Wielded Offhand}\tn    ",
  "\tY{Held Offhand}\tn       ",
  "\tY{Wielded Twohanded}\tn  ",  //20
  "\tY{Held Twohanded}\tn     ",
  "\tY{Worn On Face}\tn       ",
  /* todo */
  "\tY{Worn As Quiver}\tn     ",
  "\tY{Worn In Ear}\tn        ",
  "\tY{Worn In Ear}\tn        ",  //25
  "\tY{Worn On Eyes}\tn       ",
  "\tY{Worn As Badge}\tn      ",
};

/* Describes where an item can be worn.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *equipment_types[NUM_WEARS + 1] = {
  "Used as light",  //0
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",  //5
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",  //10
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",  //15
  "Wielded",
  "Held",
  "Wielded offhand",
  "Held offhand",
  "Wielded twohands",  //20
  "Held twohands",
  "Worn on face",
  /* todo */
  "Worn as quiver",
  "Worn in right ear",
  "Worn in left ear",  //25
  "Worn on eyes",
  "Worn as badge",
  "\n"
};


// labels for item proficiency category
const char *item_profs[NUM_ITEM_PROFS + 1] = {
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

const char *material_name[NUM_MATERIALS + 1] = {
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
  "alchemical silver",
  "cold iron",
  "darkwood",
  "\n"
};


/** Describes the type of object.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *item_types[NUM_ITEM_TYPES + 1] = {
  "UNDEFINED",  //0
  "Light",
  "Scroll",
  "Wand",
  "Staff",
  "Weapon",  //5
  "Furniture",
  "Ranged-Weapon",
  "Treasure",
  "Armor",
  "Potion",  //10
  "Wearable",
  "OTHER",
  "Trash",
  "Ammo",
  "Container",  //15
  "Note",
  "Liquid-Cont",
  "Key",
  "Food",
  "Money",  //20
  "Pen",
  "Boat",
  "Fountain",
  "Clan-Armor",
  "Crystal",  //25
  "Essence",
  "Crafting Material",
  "Spellbook",
  "Portal",
  "Plant",  //30
  /* todo list */
  "Trap",
  "Teleport",
  "Poison",
  "Summon",
  "Switch",  //35
  "Quiver",
  "Pick",
  "Instrument",
  "Disguise",
  "Wall",  //40
  "Bowl",
  "Ingredient",
  "Blocker",
  "Wagon",
  "Resources",  //45  
  "\n"
};

/** Describes the wear flags set on an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *wear_bits[NUM_ITEM_WEARS + 1] = {
  "(Takeable)",  //0
  "Finger",
  "Neck",
  "Body",
  "Head",
  "Legs",  //5
  "Feet",
  "Hands",
  "Arms",
  "Shield",
  "About-Body",  //10
  "Waist",
  "Wrist",
  "Wield",
  "Hold",
  "Face",  //15
  /* todo */
  "Quiver",
  "Ears",
  "Eyes",
  "Badge",
  "\n"
};

/** Describes the extra flags applied to an item.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *extra_bits[NUM_ITEM_FLAGS + 1] = {
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
  "Anti-Bard",
  "Anti-Arcana-Golem",
  /* todo */
  "Floating",
  "Hidden",
  "Magical-Light",  //40
  "No-Locate",
  "No-Burn",
  "Transient",
  "Auto-Proc",
  "\n"
};

/** Describes the apply types.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *apply_types[NUM_APPLIES + 1] = {
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
  "AC (obsolete)",
  "Hitroll",
  "Damroll",
  "Save-Fortitude",
  "Save-Reflex",
  "Save-Will",
  "Save-Poison",
  "Save-Death",
  "Spell-Resist",
  "Size",
  "AC (converted)",
  "Resist-Fire",
  "Resist-Cold",
  "Resist-Air",
  "Resist-Earth",
  "Resist-Acid",
  "Resist-Holy",
  "Resist-Electric",
  "Resist-Unholy",
  "Resist-Slice",
  "Resist-Puncture",
  "Resist-Force",
  "Resist-Sound",
  "Resist-Poison",
  "Resist-Disease",
  "Resist-Negative",
  "Resist-Illusion",
  "Resist-Mental",
  "Resist-Light",
  "Resist-Energy",
  "Resist-Water",
  "\n"
};

/** Describes the closure mechanism for a container.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *container_bits[NUM_CONT_FLAGS + 1] = {
  "Closeable",
  "Pick-Proof",
  "Closed",
  "Locked",
  "\n",
};

/** Describes the liquid description.
 * @pre Must be in the same order as the defines.
 * Must end array with a single newline. */
const char *drinks[NUM_LIQ_TYPES + 1] =
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
const char *drinknames[NUM_LIQ_TYPES + 1] =
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
const char *color_liquid[NUM_LIQ_TYPES + 1] =
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
	4200000,	//46
	4400000,	//47
	4600000,	//48
	4800000,	//49
	5000000,	//50
	5200000,	//51
	5400000,	//52
	5600000,	//53
	5800000,	//54
	6000000,	//55
	6200000,	//56
	6400000,	//57
	6600000,	//58
	6800000,	//59
	7000000,	//60
	7200000,	//61
	7400000,	//62
	7600000,	//63
	7800000,	//64
	8000000,	//65
	8200000,	//66
	8400000,	//67
	8600000,	//68
	8800000,	//69
	9000000,  //70
	9200000,  //71
	9400000,  //72
	9600000,  //73
	9800000,  //74
	9900000,  //75
	9999999,  //76
	9999999,  //77
	9999999,  //78
	9999999,  //79
	9999999,  //80
	9999999,  //81
	9999999,  //82
	9999999,  //83
	9999999,  //84
	9999999,  //85
	9999999,  //86
	9999999,  //87
	9999999,  //88
	9999999,  //89
	9999999   //90
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
  "<under construction>",
  "<under construction>",
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

