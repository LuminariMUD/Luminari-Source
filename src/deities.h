
/**
 * @file deities.h
 * @brief Deity System Header - Campaign-specific deity definitions and structures
 * 
 * This file contains all deity-related definitions for LuminariMUD's deity worship system.
 * The deities available depend on the campaign setting:
 * - CAMPAIGN_FR: Forgotten Realms deities (Faerun)
 * - CAMPAIGN_DL: DragonLance deities (Krynn)
 * - Default: Uses Forgotten Realms deities for generic D&D setting
 * 
 * The deity system affects:
 * - Cleric/Paladin domain selection
 * - Alignment restrictions
 * - Favored weapons
 * - Role-playing elements
 * 
 * @author LuminariMUD Development Team
 * @date Created: Unknown
 * @date Modified: August 2025
 */

/* Universal deity constant - represents no deity worship */
#define DEITY_NONE			0

#if defined(CAMPAIGN_FR)
/* ============================================================================
 * FORGOTTEN REALMS DEITIES
 * Campaign setting: Faerun
 * These deities are used when CAMPAIGN_FR is defined
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * Faerunian Pantheon (Human and general deities)
 * The main pantheon worshipped by humans and many other races in Faerun
 * ---------------------------------------------------------------------------- */

#define DEITY_AKADI 			1  /* Goddess of Air, elemental air */
#define DEITY_AURIL			2  /* Goddess of Winter, frost */
#define DEITY_AZUTH			3  /* God of Wizards, magic */
#define DEITY_BANE			4  /* God of Tyranny, hatred */
#define DEITY_BESHABA			5  /* Goddess of Misfortune */
#define DEITY_CHAUNTEA			6  /* Goddess of Agriculture */
#define DEITY_CYRIC			7  /* God of Murder, lies */
#define DEITY_DENEIR			8
#define DEITY_ELDATH			9
#define DEITY_FINDER_WYVERNSPUR 	10
#define DEITY_GARAGOS			11
#define DEITY_GARGAUTH			12
#define DEITY_GOND			13
#define DEITY_GRUMBAR			14
#define DEITY_GWAERON_WINDSTROM		15
#define DEITY_HELM			16
#define DEITY_HOAR			17
#define DEITY_ILMATER			18
#define DEITY_ISTISHIA			19
#define DEITY_JERGAL			20
#define DEITY_KELEMVOR			21
#define DEITY_KOSSUTH			22
#define DEITY_LATHANDER			23
#define DEITY_LLIIRA			24
#define DEITY_LOVIATAR			25
#define DEITY_LURUE			26
#define DEITY_MALAR			27
#define DEITY_MASK			28
#define DEITY_MIELIKKI			29
#define DEITY_MILIL			30
#define DEITY_MYSTRA			31
#define DEITY_NOBANION			32
#define DEITY_OGHMA			33
#define DEITY_RED_KNIGHT		34
#define DEITY_SAVRAS			35
#define DEITY_SELUNE			36
#define DEITY_SHAR			37
#define DEITY_SHARESS			38
#define DEITY_SHAUNDAKUL		39
#define DEITY_SHIALLIA			40
#define DEITY_SIAMORPHE			41
#define DEITY_SILVANUS			42
#define DEITY_SUNE			43
#define DEITY_TALONA			44
#define DEITY_TALOS			45
#define DEITY_TEMPUS			46
#define DEITY_TIAMAT			47
#define DEITY_TORM			48
#define DEITY_TYMORA			49
#define DEITY_TYR			50
#define DEITY_UBTAO			51
#define DEITY_ULUTIU			52
#define DEITY_UMBERLEE			53
#define DEITY_UTHGAR			54
#define DEITY_VALKUR			55
#define DEITY_VELSHAROON		56
#define DEITY_WAUKEEN			57

/* ----------------------------------------------------------------------------
 * Mulhorandi Pantheon
 * Egyptian-themed deities worshipped primarily in Mulhorand region
 * ---------------------------------------------------------------------------- */

#define DEITY_ANHUR			58
#define DEITY_GEB			59
#define DEITY_HATHOR			60
#define DEITY_HORUS_RE			61
#define DEITY_ISIS			62
#define DEITY_NEPHTHYS			63
#define DEITY_OSIRIS			64
#define DEITY_SEBEK			65
#define DEITY_SET			66
#define DEITY_THOTH			67

/* ----------------------------------------------------------------------------
 * Drow Pantheon
 * Dark elf deities, primarily evil-aligned except Eilistraee
 * ---------------------------------------------------------------------------- */

#define DEITY_EILISTRAEE		68
#define DEITY_GHAUNADAUR		69
#define DEITY_KIARANSALEE		70
#define DEITY_LOLTH			71
#define DEITY_SELVETARM			72
#define DEITY_VHAERAUN			73

/* ----------------------------------------------------------------------------
 * Dwarven Pantheon (Morndinsamman)
 * The dwarven deities led by Moradin the Soul Forger
 * ---------------------------------------------------------------------------- */

#define DEITY_ABBATHOR			74
#define DEITY_BERRONAR_TRUESILVER	75
#define DEITY_CLANGEDDIN_SILVERBEARD	76
#define DEITY_DEEP_DUERRA		77
#define DEITY_DUGMAREN_BRIGHTMANTLE	78
#define DEITY_DUMATHOIN			79
#define DEITY_GORM_GULTHYN		80
#define DEITY_HAELA_BRIGHTAXE		81
#define DEITY_LADUGUER			82
#define DEITY_MARTHAMMOR_DUIN		83
#define DEITY_MORADIN			84
#define DEITY_SHARINDLAR		85
#define DEITY_THARD_HARR		86
#define DEITY_VERGADAIN			87

/* ----------------------------------------------------------------------------
 * Elven Pantheon (Seldarine)
 * The elven deities led by Corellon Larethian
 * ---------------------------------------------------------------------------- */

#define DEITY_AERDRIE_FAENYA		88
#define DEITY_ANGHARRADH		89
#define DEITY_CORELLON_LARETHIAN	90
#define DEITY_DEEP_SASHELAS		91
#define DEITY_EREVAN_ILESERE		92
#define DEITY_FENMAREL_MESTARINE	93
#define DEITY_HANALI_CELANIL		94
#define DEITY_LABELAS_ENORETH		95
#define DEITY_RILLIFANE_RALLATHIL	96
#define DEITY_SEHANINE_MOONBOW		97
#define DEITY_SHEVARASH			98
#define DEITY_SOLONOR_THELANDIRA	99

/* ----------------------------------------------------------------------------
 * Gnome Pantheon (Lords of the Golden Hills)
 * The gnome deities led by Garl Glittergold
 * ---------------------------------------------------------------------------- */

#define DEITY_BAERVAN_WILDWANDERER	100
#define DEITY_BARAVAR_CLOAKSHADOW	101
#define DEITY_CALLARDURAN_SMOOTHHANDS	102
#define DEITY_FLANDAL_STEELSKIN		103
#define DEITY_GAERDAL_IRONHAND		104
#define DEITY_GARL_GLITTERGOLD		105
#define DEITY_SEGOJAN_EARTHCALLER	106
#define DEITY_URDLEN			107

/* ----------------------------------------------------------------------------
 * Halfling Pantheon
 * The halfling deities led by Yondalla
 * ---------------------------------------------------------------------------- */

#define DEITY_ARVOREEN			108
#define DEITY_BRANDOBARIS		109
#define DEITY_CYRROLLALEE		110
#define DEITY_SHEELA_PERYROYL		111
#define DEITY_UROGALAN			112
#define DEITY_YONDALLA			113

/* ----------------------------------------------------------------------------
 * Orc Pantheon
 * The orcish deities led by Gruumsh One-Eye
 * ---------------------------------------------------------------------------- */

#define DEITY_BAHGTRU			114
#define DEITY_GRUUMSH			115
#define DEITY_ILNEVAL			116
#define DEITY_LUTHIC			117
#define DEITY_SHARGAAS			118
#define DEITY_YURTRUS			119


#define NUM_DEITIES			120 /* Total count of FR deities (array size) */

/* Pantheon identifiers - used to group deities by racial/cultural groups */
#define DEITY_PANTHEON_NONE		0  /* No specific pantheon */
#define DEITY_PANTHEON_ALL		1  /* Available to all races */
#define DEITY_PANTHEON_FAERUNIAN	2  /* Human/general Faerunian */
#define DEITY_PANTHEON_FR_DWARVEN	3  /* Dwarven pantheon */
#define DEITY_PANTHEON_FR_DROW	        4  /* Drow/dark elf pantheon */
#define DEITY_PANTHEON_FR_ELVEN	        5  /* Surface elf pantheon */
#define DEITY_PANTHEON_FR_GNOME	        6  /* Gnome pantheon */
#define DEITY_PANTHEON_FR_HALFLING      7  /* Halfling pantheon */
#define DEITY_PANTHEON_FR_ORC 	        8  /* Orc pantheon */

#define NUM_PANTHEONS 9  /* Total number of pantheons */

/* Deity list filters - used for displaying subsets of deities to players */
#define DEITY_LIST_ALL 1        /* Show all deities */
#define DEITY_LIST_GOOD 2       /* Good-aligned deities only */
#define DEITY_LIST_NEUTRAL 3    /* Neutral-aligned deities */
#define DEITY_LIST_EVIL 4       /* Evil-aligned deities */
#define DEITY_LIST_LAWFUL 5     /* Lawful ethos deities */
#define DEITY_LIST_CHAOTIC 6    /* Chaotic ethos deities */
#define DEITY_LIST_FAERUN 7     /* Faerunian pantheon */
#define DEITY_LIST_DWARVEN 8    /* Dwarven deities */
#define DEITY_LIST_DROW 9       /* Drow deities */
#define DEITY_LIST_ELVEN 10     /* Elven deities */
#define DEITY_LIST_GNOME 11     /* Gnome deities */
#define DEITY_LIST_HALFLING 12  /* Halfling deities */
#define DEITY_LIST_ORC 13       /* Orc deities */

#elif defined(CAMPAIGN_DL)
/* ============================================================================
 * DRAGONLANCE DEITIES 
 * Campaign setting: Krynn
 * These deities are used when CAMPAIGN_DL is defined
 * Organized by alignment: Good (Light), Neutral (Balance), Evil (Darkness)
 * ============================================================================ */

/* Gods of Light (Good) */
#define DEITY_PALADINE 1        /* Platinum Dragon, leader of good */
#define DEITY_MISHAKAL 2        /* Goddess of Healing */
#define DEITY_KIRI_JOLITH 3     /* God of Honor and War */
#define DEITY_HABBAKUK 4        /* God of Animals and Sea */
#define DEITY_MAJERE 5          /* God of Discipline */
#define DEITY_BRANCHALA 6       /* God of Music */
#define DEITY_SOLINARI 7        /* God of Good Magic (White Robes) */

/* Gods of Balance (Neutral) */
#define DEITY_GILEAN 8          /* God of Knowledge */
#define DEITY_REORX 9           /* God of Forge */
#define DEITY_ZIVILYN 10        /* God of Wisdom */
#define DEITY_CHISLEV 11        /* Goddess of Nature */
#define DEITY_SIRRION 12        /* God of Fire and Creativity */
#define DEITY_SHINARE 13        /* Goddess of Wealth and Commerce */
#define DEITY_LUNITARI 14       /* Goddess of Neutral Magic (Red Robes) */

/* Gods of Darkness (Evil) */
#define DEITY_TAKHISIS 15       /* Queen of Darkness, leader of evil */
#define DEITY_MORGION 16        /* God of Disease and Decay */
#define DEITY_CHEMOSH 17        /* God of Undeath */
#define DEITY_SARGONNAS 18      /* God of Vengeance and Fire */
#define DEITY_ZEBOIM 19         /* Goddess of Sea and Storms */
#define DEITY_HIDDUKEL 20       /* God of Lies and Greed */
#define DEITY_NUITARI 21        /* God of Evil Magic (Black Robes) */

#define NUM_DEITIES 22          /* Total DragonLance deities */

#define DEITY_PANTHEON_NONE		0
#define DEITY_PANTHEON_ALL		1
#define NUM_PANTHEONS 2

#define DEITY_LIST_ALL 1
#define DEITY_LIST_GOOD 2
#define DEITY_LIST_NEUTRAL 3
#define DEITY_LIST_EVIL 4
#define DEITY_LIST_LAWFUL 5
#define DEITY_LIST_CHAOTIC 6

#else
/* ============================================================================
 * LUMINARI ORIGINAL DEITIES
 * Used when no specific campaign is defined
 * These are unique deities created specifically for LuminariMUD
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * The Luminari Pantheon
 * Original deities created for the world of Luminari
 * ---------------------------------------------------------------------------- */

/* The Creator and Primary Deities */
#define DEITY_LUMINARI          1  /* The Creator, God of Light and Magic */
#define DEITY_MORTIS            2  /* God of Death and the Afterlife */
#define DEITY_VITALIA           3  /* Goddess of Life and Nature */
#define DEITY_CHRONOS           4  /* God of Time and Fate */

/* The Elemental Lords */
#define DEITY_PYRONIS           5  /* Lord of Fire and Forge */
#define DEITY_AQUARIA           6  /* Lady of Water and Seas */
#define DEITY_TERRANUS          7  /* Lord of Earth and Mountains */
#define DEITY_AERION            8  /* Lord of Air and Storms */

/* The Aspect Deities */
#define DEITY_BELLUM            9  /* God of War and Conflict */
#define DEITY_SAPIENS          10  /* God of Knowledge and Wisdom */
#define DEITY_FORTUNA          11  /* Goddess of Luck and Chance */
#define DEITY_UMBRA            12  /* God of Shadows and Thieves */
#define DEITY_CONCORDIA        13  /* Goddess of Peace and Harmony */
#define DEITY_DECEPTOR         14  /* God of Lies and Deception */
#define DEITY_JUSTICIA         15  /* Goddess of Justice and Law */

/* The Racial Patrons */
#define DEITY_STONEFATHER      16  /* Dwarven Patron */
#define DEITY_MOONWHISPER      17  /* Elven Patron */
#define DEITY_HEARTHKEEPER     18  /* Halfling Patron */
#define DEITY_GEARMASTER       19  /* Gnome Patron */
#define DEITY_BLOODFANG        20  /* Orcish Patron */

/* Additional Core Deities */
#define DEITY_ZORREN           21  /* Lord of the Wild Hunt */

/* Total number of Luminari deities */
#define NUM_DEITIES            22  /* Includes DEITY_NONE (0) plus 21 deities */

/* Pantheon definitions for Luminari */
#define DEITY_PANTHEON_NONE         0  /* No specific pantheon */
#define DEITY_PANTHEON_ALL          1  /* Available to all */
#define DEITY_PANTHEON_LUMINARI_CORE      2  /* Core Luminari deities */
#define DEITY_PANTHEON_LUMINARI_DWARVEN   3  /* Dwarven Hearth and Forge */
#define DEITY_PANTHEON_LUMINARI_ELVEN     4  /* Elven Courts of Bough and Star */
#define DEITY_PANTHEON_LUMINARI_HALFLING  5  /* Halfling Hearth-Tide */
#define DEITY_PANTHEON_LUMINARI_ORCISH    6  /* Orcish Ash-Legion */
#define DEITY_PANTHEON_LUMINARI_SEAFOLK   7  /* Seafolk and Sky */
#define DEITY_PANTHEON_LUMINARI_DARK      8  /* Under-Shadow Dark Courts */
#define DEITY_PANTHEON_LUMINARI_PRIMARCH  9  /* Elemental Primarchs */

/* Legacy pantheon aliases for compatibility */
#define DEITY_PANTHEON_CREATOR      DEITY_PANTHEON_LUMINARI_CORE
#define DEITY_PANTHEON_ELEMENTAL    DEITY_PANTHEON_LUMINARI_PRIMARCH
#define DEITY_PANTHEON_ASPECT       DEITY_PANTHEON_LUMINARI_CORE
#define DEITY_PANTHEON_RACIAL       DEITY_PANTHEON_ALL

#define NUM_PANTHEONS 10  /* Total number of Luminari pantheons */

/* Deity list filters for Luminari */
#define DEITY_LIST_ALL 1
#define DEITY_LIST_GOOD 2
#define DEITY_LIST_NEUTRAL 3
#define DEITY_LIST_EVIL 4
#define DEITY_LIST_LAWFUL 5
#define DEITY_LIST_CHAOTIC 6
#define DEITY_LIST_CREATOR 7
#define DEITY_LIST_ELEMENTAL 8
#define DEITY_LIST_ASPECT 9
#define DEITY_LIST_RACIAL 10

#endif

/**
 * @struct deity_info
 * @brief Complete information structure for a deity
 * 
 * This structure holds all the data associated with a deity in the game.
 * It supports both the old deity system (with domains and favored weapons)
 * and the new deity system (with additional roleplay information).
 */
struct deity_info
{
    /* Basic deity information */
    const char *name;                  /* Deity's primary name */
    int ethos;                         /* Lawful/Neutral/Chaotic */
    int alignment;                     /* Good/Neutral/Evil */
    
    /* Old deity system fields (for clerics/paladins) */
    ubyte domains[6];                  /* Divine domains granted to clerics */
    ubyte favored_weapon;              /* Weapon type favored by deity */
    
    /* Pantheon and descriptive information */
    sbyte pantheon;                    /* Which pantheon deity belongs to */
    const char *portfolio;             /* Areas of influence */
    const char *description;           /* Full description of deity */
    
    /* New deity system fields (additional roleplay info) */
    const char *alias;                 /* Alternative names/titles */
    const char *symbol;                /* Holy symbol description */
    const char *worshipper_alignments; /* Allowed alignments for worshippers */
    const char *follower_names;        /* What followers are called */
    bool new_deity_system;             /* TRUE if using new system fields */
};

extern struct deity_info deity_list[NUM_DEITIES];

/* Function prototypes for deity system management */

/**
 * @brief Initialize all deity slots with safe default values
 * 
 * Sets all deity entries to safe defaults before loading campaign-specific data.
 * Called automatically by assign_deities() at boot time.
 */
void init_deities(void);

/**
 * @brief Load all campaign-specific deity data
 * 
 * Master initialization function that loads deity data based on campaign setting.
 * Called once during boot sequence from db.c initialization.
 * Automatically calls init_deities() first to set defaults.
 */
void assign_deities(void);

/**
 * @brief Add a deity using the old system (with domains and favored weapon)
 * 
 * Used for deities that grant clerical domains and have favored weapons.
 * This is the traditional deity system used for game mechanics.
 * 
 * @param deity         DEITY_* constant identifying the deity slot
 * @param name          Deity's display name
 * @param ethos         ETHOS_LAWFUL, ETHOS_NEUTRAL, or ETHOS_CHAOTIC
 * @param alignment     ALIGNMENT_GOOD, ALIGNMENT_NEUTRAL, or ALIGNMENT_EVIL
 * @param d1-d6         Domain constants (DOMAIN_*) granted to clerics
 * @param weapon        WEAPON_TYPE_* constant for favored weapon
 * @param pantheon      DEITY_PANTHEON_* constant for racial/cultural group
 * @param portfolio     Brief description of areas of influence
 * @param description   Full description shown to players
 */
void add_deity(int deity, const char *name, int ethos, int alignment,
               int d1, int d2, int d3, int d4, int d5, int d6,
               int weapon, int pantheon, 
               const char *portfolio, const char *description);

/**
 * @brief Add a deity using the new extended system (roleplay-focused)
 * 
 * Used for deities with detailed roleplay information but no domain mechanics.
 * Provides richer deity information for immersive gameplay.
 * 
 * @param deity                    DEITY_* constant identifying the deity slot
 * @param name                     Deity's display name
 * @param ethos                    ETHOS_LAWFUL, ETHOS_NEUTRAL, or ETHOS_CHAOTIC
 * @param alignment                ALIGNMENT_GOOD, ALIGNMENT_NEUTRAL, or ALIGNMENT_EVIL
 * @param pantheon                 DEITY_PANTHEON_* constant for racial/cultural group
 * @param alias                    Alternative names or titles
 * @param portfolio                Areas of influence and responsibilities
 * @param symbol                   Description of holy symbol
 * @param worshipper_alignments    Which alignments can worship this deity
 * @param follower_names           What the deity's worshippers are called
 * @param description              Full description and lore
 */
void add_deity_new(int deity, const char *name, int ethos, int alignment, int pantheon,
                   const char *alias, const char *portfolio, const char *symbol,
                   const char *worshipper_alignments, const char *follower_names,
                   const char *description);
