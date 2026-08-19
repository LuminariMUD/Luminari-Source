
/**
 * @file deities.h
 * @brief Luminari deity definitions and structures
 *
 * This file contains all deity-related definitions for LuminariMUD's deity worship system.
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
#define DEITY_NONE 0

/* ============================================================================
 * LUMINARI ORIGINAL DEITIES
 * These are unique deities created specifically for LuminariMUD
 * ============================================================================ */

/* ----------------------------------------------------------------------------
 * The Luminari Pantheon
 * Original deities created for the world of Luminari
 * ---------------------------------------------------------------------------- */

/* The Creator and Primary Deities */
#define DEITY_LUMINARI 1 /* The Creator, God of Light and Magic */
#define DEITY_MORTIS 2   /* God of Death and the Afterlife */
#define DEITY_VITALIA 3  /* Goddess of Life and Nature */
#define DEITY_CHRONOS 4  /* God of Time and Fate */

/* The Elemental Lords */
#define DEITY_PYRONIS 5  /* Lord of Fire and Forge */
#define DEITY_AQUARIA 6  /* Lady of Water and Seas */
#define DEITY_TERRANUS 7 /* Lord of Earth and Mountains */
#define DEITY_AERION 8   /* Lord of Air and Storms */

/* The Aspect Deities */
#define DEITY_BELLUM 9     /* God of War and Conflict */
#define DEITY_SAPIENS 10   /* God of Knowledge and Wisdom */
#define DEITY_FORTUNA 11   /* Goddess of Luck and Chance */
#define DEITY_UMBRA 12     /* God of Shadows and Thieves */
#define DEITY_CONCORDIA 13 /* Goddess of Peace and Harmony */
#define DEITY_DECEPTOR 14  /* God of Lies and Deception */
#define DEITY_JUSTICIA 15  /* Goddess of Justice and Law */

/* The Racial Patrons */
#define DEITY_STONEFATHER 16  /* Dwarven Patron */
#define DEITY_MOONWHISPER 17  /* Elven Patron */
#define DEITY_HEARTHKEEPER 18 /* Halfling Patron */
#define DEITY_GEARMASTER 19   /* Gnome Patron */
#define DEITY_BLOODFANG 20    /* Orcish Patron */

/* Additional Core Deities */
#define DEITY_ZORREN 21 /* Lord of the Wild Hunt */

/* Total number of Luminari deities */
#define NUM_DEITIES 22 /* Includes DEITY_NONE (0) plus 21 deities */

/* Pantheon definitions for Luminari */
#define DEITY_PANTHEON_NONE 0              /* No specific pantheon */
#define DEITY_PANTHEON_ALL 1               /* Available to all */
#define DEITY_PANTHEON_LUMINARI_CORE 2     /* Core Luminari deities */
#define DEITY_PANTHEON_LUMINARI_DWARVEN 3  /* Dwarven Hearth and Forge */
#define DEITY_PANTHEON_LUMINARI_ELVEN 4    /* Elven Courts of Bough and Star */
#define DEITY_PANTHEON_LUMINARI_HALFLING 5 /* Halfling Hearth-Tide */
#define DEITY_PANTHEON_LUMINARI_ORCISH 6   /* Orcish Ash-Legion */
#define DEITY_PANTHEON_LUMINARI_SEAFOLK 7  /* Seafolk and Sky */
#define DEITY_PANTHEON_LUMINARI_DARK 8     /* Under-Shadow Dark Courts */
#define DEITY_PANTHEON_LUMINARI_PRIMARCH 9 /* Elemental Primarchs */

/* Legacy pantheon aliases for compatibility */
#define DEITY_PANTHEON_CREATOR DEITY_PANTHEON_LUMINARI_CORE
#define DEITY_PANTHEON_ELEMENTAL DEITY_PANTHEON_LUMINARI_PRIMARCH
#define DEITY_PANTHEON_ASPECT DEITY_PANTHEON_LUMINARI_CORE
#define DEITY_PANTHEON_RACIAL DEITY_PANTHEON_ALL

#define NUM_PANTHEONS 10 /* Total number of Luminari pantheons */

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
  const char *name; /* Deity's primary name */
  int ethos;        /* Lawful/Neutral/Chaotic */
  int alignment;    /* Good/Neutral/Evil */

  /* Old deity system fields (for clerics/paladins) */
  ubyte domains[6];     /* Divine domains granted to clerics */
  ubyte favored_weapon; /* Weapon type favored by deity */

  /* Pantheon and descriptive information */
  sbyte pantheon;          /* Which pantheon deity belongs to */
  const char *portfolio;   /* Areas of influence */
  const char *description; /* Full description of deity */

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
 * Sets all deity entries to safe defaults before loading Luminari data.
 * Called automatically by assign_deities() at boot time.
 */
void init_deities(void);

/**
 * @brief Load all Luminari deity data
 *
 * Master initialization function called once during boot from db.c.
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
void add_deity(int deity, const char *name, int ethos, int alignment, int d1, int d2, int d3,
               int d4, int d5, int d6, int weapon, int pantheon, const char *portfolio,
               const char *description);

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
