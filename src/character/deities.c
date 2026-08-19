/**
 * @file deities.c
 * @brief Deity System Implementation - Manages deity worship and divine mechanics
 *
 * This file implements the deity worship system for LuminariMUD. It handles:
 * - Initialization of Luminari deity data
 * - Domain grants for divine casters (clerics, paladins)
 * - Alignment and ethos restrictions
 * - Deity-specific roleplay elements
 *
 * Two deity systems are supported:
 * 1. Old system: Basic info with domains and favored weapons
 * 2. New system: Extended info with symbols, worshipper names, etc.
 *
 * @author LuminariMUD Development Team
 * @date Created: Unknown
 * @date Modified: August 2025
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "magic/spells.h"
#include "interpreter.h"
#include "db.h"
#include "deities.h"

/* Global deity information array - indexed by DEITY_* constants */
struct deity_info deity_list[NUM_DEITIES];

/* Convenience macros for boolean values in deity data */
#define Y TRUE
#define N FALSE

/**
 * @brief Initialize all deity slots with default values
 *
 * This function sets all deity entries to safe default values before
 * the actual deity data is loaded. This ensures that any unassigned
 * deity slots have valid data rather than garbage values.
 *
 * Called by: assign_deities() before loading Luminari deities
 */
void init_deities(void)
{
  int i = 0, j = 0;

  /* Loop through all deity slots and set defaults */
  for (i = 0; i < NUM_DEITIES; i++)
  {
    /* Basic information defaults */
    deity_list[i].name = "None";
    deity_list[i].ethos = ETHOS_NEUTRAL;
    deity_list[i].alignment = ALIGNMENT_NEUTRAL;

    /* Clear all domain slots (6 is the fixed array size in struct deity_info) */
    for (j = 0; j < 6; j++)
      deity_list[i].domains[j] = DOMAIN_UNDEFINED;

    /* Combat and pantheon defaults */
    deity_list[i].favored_weapon = WEAPON_TYPE_UNARMED;
    deity_list[i].pantheon = DEITY_PANTHEON_NONE;

    /* Descriptive text defaults */
    deity_list[i].portfolio = "Nothing";
    deity_list[i].alias = "None";
    deity_list[i].symbol = "None";
    deity_list[i].worshipper_alignments = "None";
    deity_list[i].follower_names = "None";
    deity_list[i].description = "You do not worship a deity at all for reasons of your own.";

    /* Mark as using old system by default */
    deity_list[i].new_deity_system = false;
  }
}

/**
 * @brief Add a deity using the old deity system (with domains)
 *
 * This function populates a deity entry with the traditional deity data
 * including clerical domains and favored weapons. Used primarily for
 * game mechanics affecting clerics and paladins.
 *
 * @param deity         DEITY_* constant identifying the deity slot
 * @param name          Deity's name as displayed to players
 * @param ethos         ETHOS_LAWFUL, ETHOS_NEUTRAL, or ETHOS_CHAOTIC
 * @param alignment     ALIGNMENT_GOOD, ALIGNMENT_NEUTRAL, or ALIGNMENT_EVIL
 * @param d1-d6         Domain constants (DOMAIN_*) granted to clerics
 * @param weapon        WEAPON_TYPE_* constant for favored weapon
 * @param pantheon      DEITY_PANTHEON_* constant for racial/cultural group
 * @param portfolio     Brief description of deity's areas of influence
 * @param description   Full description shown to players
 */
void add_deity(int deity, const char *name, int ethos, int alignment, int d1, int d2, int d3,
               int d4, int d5, int d6, int weapon, int pantheon, const char *portfolio,
               const char *description)
{
  /* Bounds check - ensure deity index is valid to prevent buffer overflow */
  if (deity < 0 || deity >= NUM_DEITIES)
  {
    log("SYSERR: add_deity() called with invalid deity index %d (max: %d)", deity, NUM_DEITIES - 1);
    return;
  }

  /* Set basic deity information */
  deity_list[deity].name = name;
  deity_list[deity].ethos = ethos;
  deity_list[deity].alignment = alignment;

  /* Assign clerical domains (up to 6) */
  deity_list[deity].domains[0] = d1;
  deity_list[deity].domains[1] = d2;
  deity_list[deity].domains[2] = d3;
  deity_list[deity].domains[3] = d4;
  deity_list[deity].domains[4] = d5;
  deity_list[deity].domains[5] = d6;

  /* Set combat and organizational info */
  deity_list[deity].favored_weapon = weapon;
  deity_list[deity].pantheon = pantheon;

  /* Set descriptive text */
  deity_list[deity].portfolio = portfolio;
  deity_list[deity].description = description;

  /* Note: new_deity_system remains false (set in init_deities) */
}

/**
 * @brief Add a deity using the new extended deity system
 *
 * This function populates a deity entry with extended roleplay information
 * including holy symbols, worshipper restrictions, and follower titles.
 * This system provides richer deity information for roleplay purposes.
 *
 * @param deity                    DEITY_* constant identifying the deity slot
 * @param name                     Deity's name as displayed to players
 * @param ethos                    ETHOS_LAWFUL, ETHOS_NEUTRAL, or ETHOS_CHAOTIC
 * @param alignment                ALIGNMENT_GOOD, ALIGNMENT_NEUTRAL, or ALIGNMENT_EVIL
 * @param pantheon                 DEITY_PANTHEON_* constant for racial/cultural group
 * @param alias                    Alternative names or titles for the deity
 * @param portfolio                Areas of influence and divine responsibilities
 * @param symbol                   Description of the deity's holy symbol
 * @param worshipper_alignments    Which alignments can worship this deity
 * @param follower_names           What the deity's worshippers are called
 * @param description              Full description and lore about the deity
 */
void add_deity_new(int deity, const char *name, int ethos, int alignment, int pantheon,
                   const char *alias, const char *portfolio, const char *symbol,
                   const char *worshipper_alignments, const char *follower_names,
                   const char *description)
{
  /* Bounds check - ensure deity index is valid to prevent buffer overflow */
  if (deity < 0 || deity >= NUM_DEITIES)
  {
    log("SYSERR: add_deity_new() called with invalid deity index %d (max: %d)", deity,
        NUM_DEITIES - 1);
    return;
  }

  /* Set basic deity information */
  deity_list[deity].name = name;
  deity_list[deity].ethos = ethos;
  deity_list[deity].alignment = alignment;
  deity_list[deity].pantheon = pantheon;

  /* Set extended roleplay information */
  deity_list[deity].alias = alias;
  deity_list[deity].portfolio = portfolio;
  deity_list[deity].symbol = symbol;
  deity_list[deity].worshipper_alignments = worshipper_alignments;
  deity_list[deity].follower_names = follower_names;
  deity_list[deity].description = description;

  /* Mark as using the new extended deity system */
  deity_list[deity].new_deity_system = true;
}

/**
 * @brief Master function to load all Luminari deity data
 *
 * This is the main initialization function for the deity system. It:
 * 1. Calls init_deities() to set safe defaults for all slots
 * 2. Loads deity data using add_deity() or add_deity_new()
 * 3. Is called once at boot time from the main initialization sequence
 *
 * Note: The 80-character ruler below helps format deity descriptions
 */
void assign_deities(void)
{
  /* Initialize all deity slots with safe defaults */
  init_deities();

  /* 80-character ruler for formatting deity descriptions - keep descriptions within this width
 * -----------------------------------------------------------------------------
 * Use this as a guide when writing deity descriptions to ensure proper formatting
 * in the game's text display. Each line should not exceed the ruler's width.
 */

  /* ============================================================================
 * LUMINARI ORIGINAL DEITY LOADING
 * ============================================================================ */

  /* Special entry for characters who worship no deity */
  add_deity(DEITY_NONE, "None", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            DOMAIN_UNDEFINED, WEAPON_TYPE_UNARMED, DEITY_PANTHEON_ALL, "The Faithless",
            "Those who choose to worship no deity at all are known as the faithless.\r\n"
            "In the world of Luminari, their souls wander the void between planes after\r\n"
            "death, never finding rest or peace.\r\n");

  /* ========== THE CREATOR AND PRIMARY DEITIES ========== */

  /* Aethyra - Lady of the Loom (replaces Luminari as central deity) */
  add_deity(
      DEITY_LUMINARI, "Aethyra", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_MAGIC, DOMAIN_KNOWLEDGE,
      DOMAIN_RUNE, DOMAIN_PROTECTION, DOMAIN_SPELL, DOMAIN_LAW, WEAPON_TYPE_QUARTERSTAFF,
      DEITY_PANTHEON_LUMINARI_CORE, "Magic, Oaths, Hidden Patterns, Memory, The Weave",
      "Aethyra is the Lady of the Loom, guardian of the metaphysical weave that binds\r\n"
      "all reality. She oversees magic, oaths, and the hidden patterns that connect all\r\n"
      "things. Every vow spoken strengthens the Loom, while broken oaths fray its threads.\r\n"
      "Her followers are mages, oathkeepers, archivists, and those who preserve memory\r\n"
      "and knowledge. She teaches that magic without compassion is mere destruction, and\r\n"
      "that unremembered truths become cages for future generations. Her priests guard\r\n"
      "dangerous secrets while sharing beneficial knowledge freely. Her holy symbol is a\r\n"
      "silver spindle wreathed in seven motes of different colors, representing the\r\n"
      "threads of fate and magic she weaves.\r\n");

  /* Nethris - The Gravewarden (replaces Mortis) */
  add_deity(
      DEITY_MORTIS, "Nethris", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_DEATH, DOMAIN_FATE,
      DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_LAW, DOMAIN_UNDEFINED, WEAPON_TYPE_SCYTHE,
      DEITY_PANTHEON_LUMINARI_CORE, "Death, Fate, Proper Rites, Thresholds of Endings",
      "Nethris is the Gravewarden, the impartial guardian of death's threshold who ensures\r\n"
      "every soul reaches its proper destination. Neither cruel nor merciful, he maintains\r\n"
      "the boundary between life and death with unwavering dedication. His domain encompasses\r\n"
      "not just death, but all endings - the close of chapters, the fall of empires, the\r\n"
      "final words of stories. He teaches that every life is a ledger that must be settled\r\n"
      "cleanly, and that the dead are owed silence and safe passage. His priests oversee\r\n"
      "funerals, combat undead abominations, preserve last wills, and conduct the Crossing\r\n"
      "Vigils at dusk. They know that fate is a road, not a jailer, and help the dying find\r\n"
      "peace in their final moments. His holy symbol is a key bound to a raven-feather cord,\r\n"
      "representing the unlocking of the final door.\r\n");

  /* Seraphine - The Dawnstar (replaces Vitalia) */
  add_deity(DEITY_VITALIA, "Seraphine", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_SUN, DOMAIN_RENEWAL,
            DOMAIN_HEALING, DOMAIN_GOOD, DOMAIN_PLANT, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR,
            DEITY_PANTHEON_LUMINARI_CORE,
            "Sun, Renewal, Harvest, Redemption, Agriculture, Second Chances",
            "Seraphine the Dawnstar brings light, renewal, and redemption to all who seek it.\r\n"
            "She is the golden promise of each new dawn, teaching that no soul is beyond\r\n"
            "redemption and that every sunrise offers both a debt and a chance to repay it.\r\n"
            "Her domain encompasses the sun's life-giving warmth, the harvest's abundance, and\r\n"
            "the power of second chances. She shows particular mercy to those who abandon evil\r\n"
            "for honest work, believing that labor cleanses the spirit. Her followers include\r\n"
            "farmers, healers, reformers, and former criminals seeking redemption. Her priests\r\n"
            "tend fields, heal the sick, conduct Dawn Pardons where communities forgive past\r\n"
            "wrongs, and celebrate the Harvest Home. She teaches to share abundance freely,\r\n"
            "for hoarded wealth becomes corruption. Her holy symbol is an eight-rayed aureole\r\n"
            "within a wheat circlet, often rendered in gold and amber.\r\n");

  /* Kaelthir - Starwarden (replaces Chronos) */
  add_deity(DEITY_CHRONOS, "Kaelthir", ETHOS_NEUTRAL, ALIGNMENT_NEUTRAL, DOMAIN_KNOWLEDGE,
            DOMAIN_TIME, DOMAIN_RUNE, DOMAIN_FATE, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_DAGGER, DEITY_PANTHEON_LUMINARI_ELVEN,
            "Ancient Knowledge, Time, Prophecy, The Old Names",
            "Kaelthir the Starwarden is the keeper of ancient knowledge and guardian of time's\r\n"
            "flow. An elven deity of immense age and wisdom, he reads the patterns of stars to\r\n"
            "divine the future while preserving the truths of the past. He maintains the old\r\n"
            "names - the true names of things that hold power - and teaches that to forget\r\n"
            "history is to be doomed to repeat it. His followers are historians, prophets,\r\n"
            "astronomers, and keepers of lore. His priests maintain ancient libraries, chart\r\n"
            "the heavens, and warn of coming dangers without seeking to rule through their\r\n"
            "knowledge. They believe in reading the sky humbly and preserving wisdom for future\r\n"
            "generations. His holy symbol is a seven-pointed star-map, each point marking a\r\n"
            "moment of cosmic significance.\r\n");

  /* ========== THE ELEMENTAL LORDS ========== */

  /* Pyrion - The First Flame (Elemental Primarch) */
  add_deity(DEITY_PYRONIS, "Pyrion", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_FIRE,
            DOMAIN_DESTRUCTION, DOMAIN_RENEWAL, DOMAIN_CRAFT, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_SCIMITAR, DEITY_PANTHEON_LUMINARI_PRIMARCH,
            "Primal Fire, Creation Through Destruction, Forge-Heat, Passion",
            "Pyrion is the First Flame, the primal fire that existed before the world was\r\n"
            "shaped. He represents fire in its purest form - both creative and destructive,\r\n"
            "passionate and consuming. He teaches that destruction clears the way for new\r\n"
            "creation, that the hottest flames forge the strongest steel, and that passion\r\n"
            "without purpose is mere destruction. His followers include elemental fire mages,\r\n"
            "smiths who work with magical metals, and passionate artists who burn with creative\r\n"
            "fire. His priests tend eternal flames, oversee ritual burnings that clear old\r\n"
            "growth for new, and teach that the forge requires patience despite its heat. His\r\n"
            "holy symbol is a spiral of white flame consuming itself, representing the eternal\r\n"
            "cycle of destruction and rebirth.\r\n");

  /* Vaelith - Whispering Tide */
  add_deity(DEITY_AQUARIA, "Vaelith", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_OCEAN, DOMAIN_STORM,
            DOMAIN_TRAVEL, DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_TRIDENT, DEITY_PANTHEON_LUMINARI_CORE,
            "Sea, Storms, Currents, Horizons, The Deep",
            "Vaelith of the Whispering Tide rules the seas and all their moods - from gentle\r\n"
            "harbor waves to ship-crushing storms. She speaks in the language of currents and\r\n"
            "tides, guiding those who know how to listen while drowning those who show\r\n"
            "disrespect. Her domain includes not just the ocean's surface but the mysterious\r\n"
            "deeps where light never reaches. She teaches her followers to respect the deep\r\n"
            "and bargain with the shore, to share water and spare the hungry, and that storms\r\n"
            "test both hull and heart alike. Sailors cast the Third Knot before voyages for\r\n"
            "her blessing, and coastal communities hold Spiral-Wake memorials after storms to\r\n"
            "honor those the sea has claimed. Her holy symbol is three wave-crests forming a\r\n"
            "spiral shell, representing the eternal dance of the tides.\r\n");

  /* Orith - The Stonefather */
  add_deity(
      DEITY_TERRANUS, "Orith", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_EARTH, DOMAIN_PROTECTION,
      DOMAIN_STRENGTH, DOMAIN_CAVERN, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_GREAT_CLUB,
      DEITY_PANTHEON_LUMINARI_CORE, "Earth, Mountains, Patience, Endurance, Shelter",
      "Orith the Stonefather is the mountain itself given thought and purpose. He embodies\r\n"
      "the patience of stone, the strength of bedrock, and the shelter of the cave. He\r\n"
      "teaches that true strength comes from deep roots and solid foundations, that time\r\n"
      "is an ally to those who can wait, and that the mountain must shelter the valley\r\n"
      "from avalanches it might begin. His followers include miners, mountaineers,\r\n"
      "stalwart defenders, and those who build to last. His priests maintain mountain\r\n"
      "passes, consecrate new halls with the Stonebinding ritual, and conduct Echofast\r\n"
      "ceremonies at solstices within cavern temples. They move slowly but surely,\r\n"
      "knowing that haste often leads to collapse. His holy symbol is a mountain rune\r\n"
      "carved in granite, unchanging and eternal.\r\n");

  /* Aerion - Swiftwind (Elven deity of Air) */
  add_deity(
      DEITY_AERION, "Aerion", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_AIR, DOMAIN_TRAVEL,
      DOMAIN_PROTECTION, DOMAIN_LUCK, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SHORT_BOW,
      DEITY_PANTHEON_LUMINARI_ELVEN, "Air, Migratory Paths, Messengers, True Tidings",
      "Aerion Swiftwind is the elven lord of air and patron of all who carry messages\r\n"
      "across the world. He guides migratory birds, speeds the feet of messengers, and\r\n"
      "ensures that important tidings arrive when needed. He embodies the freedom of\r\n"
      "the wind but also its responsibility - for a lie carried on the wind becomes a\r\n"
      "storm that devastates all in its path. His followers include messengers, scouts,\r\n"
      "travelers, and those who value truth in communication. His priests maintain\r\n"
      "messenger routes, train carrier birds, and perform divinations using wind patterns.\r\n"
      "They believe that bearing tidings true is a sacred duty. His holy symbol is a\r\n"
      "feathered spiral, representing the paths of wind and wing.\r\n");

  /* ========== THE ASPECT DEITIES ========== */

  /* Kordran - Hammer of Dawn (replaces Bellum) */
  add_deity(DEITY_BELLUM, "Kordran", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_WAR, DOMAIN_PROTECTION,
            DOMAIN_STRENGTH, DOMAIN_LAW, DOMAIN_GOOD, DOMAIN_UNDEFINED, WEAPON_TYPE_WARHAMMER,
            DEITY_PANTHEON_LUMINARI_CORE,
            "War, Duty, Resolve, Banners, Righteous Battle, Military Honor",
            "Kordran, the Hammer of Dawn, stands as the divine champion of righteous warfare\r\n"
            "and protector of the innocent. He values duty, honor, and the courage to hold\r\n"
            "the line when all seems lost. Unlike deities of conquest, Kordran teaches that\r\n"
            "true victory comes not from domination but from protecting those who cannot\r\n"
            "protect themselves. His followers include knights, city guards, paladins, and\r\n"
            "military commanders who fight with honor. His priests perform the Shieldbinding\r\n"
            "ritual at dawn before campaigns, honor fallen standards on the Day of Banners,\r\n"
            "and keep vigil through the longest night. They believe that retreat is acceptable\r\n"
            "only to save innocent lives, and that victory without honor wounds the soul. His\r\n"
            "holy symbol is a sunlit hammer over a squared shield with a rising sun.\r\n");

  /* Thalos - The Scales Unblinking (replaces Sapiens) */
  add_deity(
      DEITY_SAPIENS, "Thalos", ETHOS_LAWFUL, ALIGNMENT_NEUTRAL, DOMAIN_LAW, DOMAIN_KNOWLEDGE,
      DOMAIN_PROTECTION, DOMAIN_PLANNING, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
      WEAPON_TYPE_LONG_SWORD, DEITY_PANTHEON_LUMINARI_CORE,
      "Judgment, Law, Scholarship, Cities, Records",
      "Thalos of the Scales Unblinking is the divine arbiter of law and keeper of all\r\n"
      "records. He oversees judgment, maintains civic order, and ensures that law serves\r\n"
      "justice rather than tyranny. His domain encompasses written law, scholarly pursuit,\r\n"
      "and the growth of civilization through order. He teaches that records bind both\r\n"
      "the living and the dead, that mercy without measure seeds future crimes, and that\r\n"
      "truth emerges through reasoned debate. His followers include magistrates, scholars,\r\n"
      "lorekeepers, and diplomats. His priests oversee trials, maintain legal archives,\r\n"
      "witness Charter Oaths when founding councils, and conduct the Census of Grey Ink\r\n"
      "every tenth year. His holy symbol is a blindfolded scale with a rune on each pan,\r\n"
      "representing impartial judgment through knowledge.\r\n");

  /* Lumerion - The Lantern-Bearer (replaces Fortuna) */
  add_deity(DEITY_FORTUNA, "Lumerion", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_LUCK, DOMAIN_TRAVEL,
            DOMAIN_PROTECTION, DOMAIN_GOOD, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_SHORT_SWORD, DEITY_PANTHEON_LUMINARI_CORE,
            "Luck, Travel, Wayfinding, Small Chances, Crossroads",
            "Lumerion the Lantern-Bearer lights the way for travelers and brings luck to those\r\n"
            "who share the road. He is the patron of wanderers, messengers, and all who trust\r\n"
            "in fortune's favor. His luck is not the cold chance of dice, but the warm fortune\r\n"
            "that comes to those who help others along their journey. He teaches that luck is\r\n"
            "a lantern best carried for others, that maps should be shared freely, and that\r\n"
            "loose coins find empty hands. His followers light lanterns at crossroads during\r\n"
            "Lanternnight, keeping vigil until dawn to guide lost travelers. His priests bless\r\n"
            "journeys with coin-tosses, maintain wayside shrines, and ensure that no traveler\r\n"
            "goes without aid. His holy symbol is a hanging lantern with a smiling wick-flame,\r\n"
            "representing the light that guides and the joy of the journey.\r\n");

  /* Nyxara - Of the Veil (replaces Umbra) */
  add_deity(
      DEITY_UMBRA, "Nyxara", ETHOS_NEUTRAL, ALIGNMENT_EVIL, DOMAIN_DARKNESS, DOMAIN_KNOWLEDGE,
      DOMAIN_TRICKERY, DOMAIN_CAVERN, DOMAIN_PORTAL, DOMAIN_UNDEFINED, WEAPON_TYPE_DAGGER,
      DEITY_PANTHEON_LUMINARI_CORE, "Night, Secrets, Thresholds, Lost Names, Hidden Knowledge",
      "Nyxara of the Veil is the keeper of secrets and guardian of thresholds. She rules\r\n"
      "the night not as mere darkness, but as the time when hidden truths emerge and\r\n"
      "secret doors open. Sister-shadow to Seraphine, they were once one deity split\r\n"
      "during the First Dawn. She teaches that secrets are coins that spend themselves\r\n"
      "once, that in darkness one finds their true shape, and that doors are promises\r\n"
      "about who may enter. Her followers include spies, secret-keepers, the dispossessed,\r\n"
      "and those who guard dangerous knowledge. Her priests swear the Black Oath on\r\n"
      "starless nights, perform Last-Lamp rites when swearing silence, and keep Threshold\r\n"
      "Vigils at each solstice. Her holy symbol is a half-mask of polished obsidian with\r\n"
      "silver tears, representing the hidden face and the price of secrets.\r\n");

  /* Myrr - The Quiet Brook (replaces Concordia) */
  add_deity(
      DEITY_CONCORDIA, "Myrr", ETHOS_NEUTRAL, ALIGNMENT_GOOD, DOMAIN_HEALING, DOMAIN_WATER,
      DOMAIN_PROTECTION, DOMAIN_FAMILY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
      WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_LUMINARI_CORE,
      "Peace, Healing, Sanctuaries, Small Kindnesses, Wells",
      "Myrr of the Quiet Brook is the gentle deity of healing waters and peaceful places.\r\n"
      "She brings healing not through grand miracles but through small, steady kindnesses\r\n"
      "that accumulate like drops filling a basin. She maintains sanctuaries where no\r\n"
      "violence may enter, tends to all who thirst, and teaches that a life saved is a\r\n"
      "promise to the Loom. Her followers include healers, pacifists, and wayhouse-keepers\r\n"
      "who offer shelter to all. Her priests perform Waterblessings on new wells, keep\r\n"
      "their doors open during winter's first frost for any who need warmth, and believe\r\n"
      "that feuds should end with bread and stories, not blood. They never poison wells,\r\n"
      "for water is sacred to all life. Her holy symbol is a cupped hand brimming with\r\n"
      "water, representing both the giving and receiving of mercy.\r\n");

  /* Vespera - Of the Many Masks (replaces Deceptor) */
  add_deity(
      DEITY_DECEPTOR, "Vespera", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_TRICKERY, DOMAIN_CHARM,
      DOMAIN_LUCK, DOMAIN_PORTAL, DOMAIN_ILLUSION, DOMAIN_UNDEFINED, WEAPON_TYPE_RAPIER,
      DEITY_PANTHEON_LUMINARI_CORE,
      "Trickery, Performance, Subterfuge, Reinvention, Revolution Through Mockery",
      "Vespera of the Many Masks is the divine trickster who teaches that identity is\r\n"
      "fluid and power is best undermined through laughter. Unlike mere deceivers, she\r\n"
      "uses tricks to expose tyrants, performances to spread truth, and mockery to topple\r\n"
      "the mighty. She believes that truth is a stage direction while meaning is the play,\r\n"
      "that every face is a mask so one should choose wisely, and that the greatest trick\r\n"
      "is making power laugh at itself. Her followers include thespians, spies, confidence\r\n"
      "artists, revolutionaries, and satirists. Her priests oversee the Masquerade of Low\r\n"
      "Kings each midwinter where commoners mock nobility, perform the Rite of the Second\r\n"
      "Name for those seeking rebirth, and stage monthly Theatre of Truth performances.\r\n"
      "Her holy symbol is a fan of three masks - laughing, weeping, and serene.\r\n");

  /* Calystral - The Flameheart (replaces Justicia with passion/art) */
  add_deity(
      DEITY_JUSTICIA, "Calystral", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_CHARM, DOMAIN_GOOD,
      DOMAIN_FIRE, DOMAIN_MOBILITY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_WHIP,
      DEITY_PANTHEON_LUMINARI_CORE, "Love, Art, Passion, Creative Fire, Beauty",
      "Calystral the Flameheart is the goddess of passionate love and creative fire. She\r\n"
      "inspires artists to create boldly, lovers to love honestly, and all to forgive\r\n"
      "fiercely. She teaches that beauty is a duty when cruelty is easy, that one should\r\n"
      "burn for what they would save, and that creation without passion is mere craft.\r\n"
      "Her domain encompasses romantic love, artistic inspiration, the fire of creativity,\r\n"
      "and the courage to express one's true feelings. Her followers include artists,\r\n"
      "lovers, celebrants, and patron-muses who support creative works. Her priests\r\n"
      "oversee the Night of a Thousand Lamps in summer where lovers declare themselves,\r\n"
      "witness Vows of Twinned Breath for unions, and maintain galleries where art can\r\n"
      "flourish. Her holy symbol is a heart-flame within a circlet of quills, representing\r\n"
      "passion expressed through art.\r\n");

  /* ========== THE RACIAL PATRON DEITIES ========== */

  /* Borhild - Emberforge (Dwarven Forge Deity) */
  add_deity(DEITY_STONEFATHER, "Borhild", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_CRAFT, DOMAIN_METAL,
            DOMAIN_FIRE, DOMAIN_PROTECTION, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_WARHAMMER, DEITY_PANTHEON_LUMINARI_DWARVEN,
            "Forgecraft, Innovation, Consecrated Labor, Dwarven Craft",
            "Borhild Emberforge is the divine smith of the dwarven pantheon, patron of all who\r\n"
            "work metal with skill and purpose. She teaches that work honors ancestors, that\r\n"
            "strength must be tempered with purpose, and that masterpieces are vows made in\r\n"
            "steel. Her forge burns with eternal flame, and from it come the greatest works\r\n"
            "of dwarven craft - weapons that never dull, armor that never breaks, and tools\r\n"
            "that build wonders. Her followers are smiths, artificers, and innovators who push\r\n"
            "the boundaries of their craft. Her priests maintain the great forges, teach\r\n"
            "apprentices the sacred techniques, and oversee the creation of items worthy of\r\n"
            "legend. Her holy symbol is an anvil marked with an ember-sigil, representing the\r\n"
            "marriage of force and fire that creates true art.\r\n");

  /* Selithiel - Moonbough (Elven Moon Deity) */
  add_deity(DEITY_MOONWHISPER, "Selithiel", ETHOS_CHAOTIC, ALIGNMENT_GOOD, DOMAIN_MOON,
            DOMAIN_ILLUSION, DOMAIN_TRAVEL, DOMAIN_GOOD, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_LUMINARI_ELVEN,
            "Moon, Dreams, Passage Between Worlds, Elven Mysticism",
            "Selithiel Moonbough is the elven goddess of moon and dreams, who guides souls\r\n"
            "through the veil between waking and sleeping, between the mortal world and the\r\n"
            "feywild. She teaches that dreams remember what the waking mind forgets, that\r\n"
            "moonlight reveals truths hidden by day, and that one should walk softly between\r\n"
            "worlds. Her silver light has guided elven travelers for millennia, and her dreams\r\n"
            "bring visions of possible futures. Her followers include mystics, dream-walkers,\r\n"
            "travelers, and those who seek wisdom in the moon's phases. Her priests conduct\r\n"
            "rituals under the full moon, interpret prophetic dreams, and maintain the ancient\r\n"
            "moon-paths that connect elven realms. Her holy symbol is a crescent circlet of\r\n"
            "silver, representing the moon's journey through darkness.\r\n");

  /* Pella - Of the Warm Hearth (Halfling Home Deity) */
  add_deity(DEITY_HEARTHKEEPER, "Pella", ETHOS_LAWFUL, ALIGNMENT_GOOD, DOMAIN_FAMILY,
            DOMAIN_PROTECTION, DOMAIN_TRAVEL, DOMAIN_GOOD, DOMAIN_COMMUNITY, DOMAIN_UNDEFINED,
            WEAPON_TYPE_QUARTERSTAFF, DEITY_PANTHEON_LUMINARI_HALFLING,
            "Home, Hospitality, Safe Roads, Halfling Communities",
            "Pella of the Warm Hearth is the halfling goddess who ensures that every door\r\n"
            "opens to welcome and every road leads safely home. She values hospitality above\r\n"
            "wealth, community above glory, and a good meal above conquest. She teaches that\r\n"
            "one should leave a light in the window for travelers, that no guest should go\r\n"
            "hungry, and that the best adventures are the ones that bring you back to your\r\n"
            "own hearth. Her followers are primarily halflings who maintain inns, protect\r\n"
            "traveling merchants, and ensure their communities thrive. Her priests bless new\r\n"
            "homes, oversee harvest festivals, mediate disputes with good food and better\r\n"
            "stories, and maintain the roads between halfling settlements. Her holy symbol is\r\n"
            "a kettle-and-key, representing both hospitality and security.\r\n");

  /* Gearmaster - Gnome Patron */
  add_deity(DEITY_GEARMASTER, "Gearmaster", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_CHAOS,
            DOMAIN_ARTIFICE, DOMAIN_KNOWLEDGE, DOMAIN_TRICKERY, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED,
            WEAPON_TYPE_LIGHT_HAMMER, DEITY_PANTHEON_RACIAL, "Gnomes, Invention, Humor, Gems",
            "The Gearmaster sparked the gnomes to life with curiosity and cleverness. He\r\n"
            "values innovation, humor, and the joy of discovery. His followers are primarily\r\n"
            "gnomes who express their faith through invention and pranks. His priests maintain\r\n"
            "workshops, teach the young, and ensure that life never becomes too serious. They\r\n"
            "believe that laughter and learning go hand in hand. His symbol is a complex gear\r\n"
            "mechanism with a gem at its center.\r\n");

  /* Ghorak - The Ash-Eyed (Orcish Conquest Deity) */
  add_deity(
      DEITY_BLOODFANG, "Ghorak", ETHOS_CHAOTIC, ALIGNMENT_EVIL, DOMAIN_STRENGTH, DOMAIN_WAR,
      DOMAIN_DESTRUCTION, DOMAIN_HATRED, DOMAIN_UNDEFINED, DOMAIN_UNDEFINED, WEAPON_TYPE_SPEAR,
      DEITY_PANTHEON_LUMINARI_ORCISH, "Conquest, Strength, Survival, Taking the Strong Ground",
      "Ghorak the Ash-Eyed is the orcish god of conquest and survival through strength.\r\n"
      "His eyes burn with the ash of burned cities, seeing weakness to be crushed and\r\n"
      "strength to be claimed. He teaches that one must take the strong ground, crush\r\n"
      "weak chains that bind, and honor only the unbroken. Unlike mindless destruction,\r\n"
      "Ghorak values strategic conquest - taking what makes one stronger and burning\r\n"
      "what would make one weak. His followers are warriors, raiders, and survivors who\r\n"
      "understand that mercy is a luxury the weak cannot afford. His priests lead war\r\n"
      "bands, oversee trials of strength, mark warriors with ash before battle, and\r\n"
      "ensure that orcish strength is never diluted by softness. His holy symbol is a\r\n"
      "charred eye sigil, representing the vision that sees through weakness to strength.\r\n");

  /* ========== ADDITIONAL CORE DEITIES ========== */

  /* Zorren - Lord of the Wild Hunt */
  add_deity(DEITY_ZORREN, "Zorren", ETHOS_CHAOTIC, ALIGNMENT_NEUTRAL, DOMAIN_ANIMAL, DOMAIN_MOON,
            DOMAIN_STRENGTH, DOMAIN_TRAVEL, DOMAIN_LIBERATION, DOMAIN_UNDEFINED,
            WEAPON_TYPE_COMPOSITE_LONGBOW, DEITY_PANTHEON_LUMINARI_CORE,
            "Beasts, Moonlit Pursuit, Primal Triumph, Freedom, The Hunter's Bond",
            "Zorren, Lord of the Wild Hunt, leads the eternal chase beneath the moon. He is\r\n"
            "the primal spirit of the hunt - both predator and protector, savage and noble.\r\n"
            "He teaches that one should hunt to live rather than glut, that the fallen deserve\r\n"
            "both a song and their bones returned to earth, and that chains penning the wind\r\n"
            "must be broken. His wisdom states that the pack survives where the lone wolf\r\n"
            "perishes, and that respect for prey honors the hunter. His followers include\r\n"
            "rangers, druids of the chase, beast-tamers, and free companies who refuse to be\r\n"
            "bound. His priests conduct the Moonfeast at autumn's first full moon, perform\r\n"
            "Last-Track rites for the year's final hunt, share Blood Bonds with their pack,\r\n"
            "and run Wild Nights under each full moon. His holy symbol is a white antler\r\n"
            "circled by a thin crescent, representing the wild's nobility under moon's light.\r\n");

} /* End of assign_deities() */
