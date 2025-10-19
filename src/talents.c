/***********************************************************************
** TALENTS.C                                                          **
** Implementation of Crafting & Harvesting Talent System              **
***********************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "modify.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "talents.h"
#include "helpers.h" /* for two_arguments prototype */

struct talent_info talent_list[TALENT_MAX];

/* Category names for display */
const char *talent_category_names[NUM_TALENT_CATEGORIES] = {
  "General",
  "Woodworking",
  "Tailoring",
  "Alchemy",
  "Armorsmithing",
  "Weaponsmithing",
  "Bowmaking",
  "Jewelcrafting",
  "Leatherworking",
  "Trapmaking",
  "Poisonmaking",
  "Metalworking",
  "Fishing",
  "Cooking",
  "Mining",
  "Hunting",
  "Forestry",
  "Gathering"
};

/* Structure for sorting talents */
struct talent_sort_entry {
  int talent_id;
  const char *name;
};

/* Comparison function for qsort - alphabetical by name */
static int compare_talents_alpha(const void *a, const void *b) {
  const struct talent_sort_entry *ta = (const struct talent_sort_entry *)a;
  const struct talent_sort_entry *tb = (const struct talent_sort_entry *)b;
  return strcasecmp(ta->name, tb->name);
}

/* Helper to create a sorted array of talent IDs (alphabetically by name)
 * Returns number of talents in the array.
 * filter_available: if TRUE, only include talents player can afford
 * filter_in_game: if TRUE, only include in_game talents
 * include_disabled: if TRUE, include disabled talents (for admin view)
 */
static int get_sorted_talents(struct char_data *ch, struct talent_sort_entry *sorted,
                               bool filter_available, bool filter_in_game, bool include_disabled) {
  int i, count = 0;
  
  for (i = 1; i < TALENT_MAX; i++) {
    /* Skip if not in game and we're filtering for in_game only */
    if (filter_in_game && !talent_list[i].in_game) continue;
    
    /* Skip if disabled and we're not including disabled */
    if (!include_disabled && !talent_list[i].in_game) continue;
    
    /* Skip if filtering for available and player can't afford it */
    if (filter_available && !can_learn_talent(ch, i)) continue;
    
    sorted[count].talent_id = i;
    sorted[count].name = talent_list[i].name;
    count++;
  }
  
  /* Sort alphabetically */
  qsort(sorted, count, sizeof(struct talent_sort_entry), compare_talents_alpha);
  
  return count;
}

/* Helper to initialize a talent definition */
static void talento(int talent, const char *name, int base_point_cost, int base_gold_cost, int max_ranks,
                    const char *short_desc, const char *long_desc, bool in_game, int category) {
  if (talent < 0 || talent >= TALENT_MAX) return;
  talent_list[talent].name = name;
  talent_list[talent].base_point_cost = base_point_cost;
  talent_list[talent].base_gold_cost = base_gold_cost;
  talent_list[talent].max_ranks = MAX(1, max_ranks);
  talent_list[talent].short_desc = short_desc;
  talent_list[talent].long_desc = long_desc;
  talent_list[talent].in_game = in_game;
  talent_list[talent].category = category;
}

void init_talents(void) {
  memset(talent_list, 0, sizeof(talent_list));
  talento(TALENT_NONE, "(none)", 0, 0, 1, "", "", FALSE, TALENT_CAT_GENERAL);
  talento(TALENT_SCAVENGER, "scavenger", 10, 25000, 1,
          "This talent grants access to the scavenger command.",
          "This talent grants access to the scavenger command.", TRUE, TALENT_CAT_GENERAL);
  
  /* Mote Synergy Talents */
  talento(TALENT_AIR_MOTE_SYNERGY, "air mote synergy", 2, 5000, 5,
          "10% chance per rank for extra air mote when harvesting",
          "Each rank gives a 10% chance to gain an extra air mote whenever you would gain an air mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_DARK_MOTE_SYNERGY, "dark mote synergy", 2, 5000, 5,
          "10% chance per rank for extra dark mote when harvesting",
          "Each rank gives a 10% chance to gain an extra dark mote whenever you would gain a dark mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_EARTH_MOTE_SYNERGY, "earth mote synergy", 2, 5000, 5,
          "10% chance per rank for extra earth mote when harvesting",
          "Each rank gives a 10% chance to gain an extra earth mote whenever you would gain an earth mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_FIRE_MOTE_SYNERGY, "fire mote synergy", 2, 5000, 5,
          "10% chance per rank for extra fire mote when harvesting",
          "Each rank gives a 10% chance to gain an extra fire mote whenever you would gain a fire mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_ICE_MOTE_SYNERGY, "ice mote synergy", 2, 5000, 5,
          "10% chance per rank for extra ice mote when harvesting",
          "Each rank gives a 10% chance to gain an extra ice mote whenever you would gain an ice mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_LIGHT_MOTE_SYNERGY, "light mote synergy", 2, 5000, 5,
          "10% chance per rank for extra light mote when harvesting",
          "Each rank gives a 10% chance to gain an extra light mote whenever you would gain a light mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_LIGHTNING_MOTE_SYNERGY, "lightning mote synergy", 2, 5000, 5,
          "10% chance per rank for extra lightning mote when harvesting",
          "Each rank gives a 10% chance to gain an extra lightning mote whenever you would gain a lightning mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  talento(TALENT_WATER_MOTE_SYNERGY, "water mote synergy", 2, 5000, 5,
          "10% chance per rank for extra water mote when harvesting",
          "Each rank gives a 10% chance to gain an extra water mote whenever you would gain a water mote from harvesting.", TRUE, TALENT_CAT_GENERAL);
  
  /* Skill-specific talents generated from CSV */
  /* Woodworking talents */
  talento(TALENT_PROFICIENT_WOODWORKING, "proficient woodworking", 1, 1000, 5,
          "+1 to skill checks for the woodworking skill",
          "Each rank gives +1 to skill checks made with the woodworking skill.", TRUE, TALENT_CAT_WOODWORKING);
  talento(TALENT_RAPID_WOODWORKING, "rapid woodworking", 1, 1000, 10,
          "-2 seconds on woodworking skill task completion per rank",
          "Reduces completion time for tasks performed with the woodworking skill by 2 seconds per rank.", TRUE, TALENT_CAT_WOODWORKING);
  talento(TALENT_WOODWORKING_EXPERTISE, "woodworking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the woodworking skill.", TRUE, TALENT_CAT_WOODWORKING);
  talento(TALENT_EFFICIENT_WOODWORKING, "efficient woodworking", 1, 2500, 10,
          "chance to use less materials on woodworking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the woodworking skill.", TRUE, TALENT_CAT_WOODWORKING);
  talento(TALENT_INSIGHTFUL_WOODWORKING, "insightful woodworking", 2, 5000, 5,
          "Chance for extra experience on success for woodworking skill tasks",
          "Chance for extra experience on success for woodworking skill tasks", TRUE, TALENT_CAT_WOODWORKING);
  
  /* Tailoring talents */
  talento(TALENT_PROFICIENT_TAILORING, "proficient tailoring", 1, 1000, 5,
          "+1 to skill checks for the tailoring skill",
          "Each rank gives +1 to skill checks made with the tailoring skill.", TRUE, TALENT_CAT_TAILORING);
  talento(TALENT_RAPID_TAILORING, "rapid tailoring", 1, 1000, 10,
          "-2 seconds on tailoring skill task completion per rank",
          "Reduces completion time for tasks performed with the tailoring skill by 2 seconds per rank.", TRUE, TALENT_CAT_TAILORING);
  talento(TALENT_TAILORING_EXPERTISE, "tailoring expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the tailoring skill.", TRUE, TALENT_CAT_TAILORING);
  talento(TALENT_EFFICIENT_TAILORING, "efficient tailoring", 1, 2500, 10,
          "chance to use less materials on tailoring skill tasks",
          "Each rank gives a 3% chance to save half materials when using the tailoring skill.", TRUE, TALENT_CAT_TAILORING);
  talento(TALENT_INSIGHTFUL_TAILORING, "insightful tailoring", 2, 5000, 5,
          "Chance for extra experience on success for tailoring skill tasks",
          "Chance for extra experience on success for tailoring skill tasks", TRUE, TALENT_CAT_TAILORING);
  
  /* Alchemy talents */
  talento(TALENT_PROFICIENT_ALCHEMY, "proficient alchemy", 1, 1000, 5,
          "+1 to skill checks for the alchemy skill",
          "Each rank gives +1 to skill checks made with the alchemy skill.", TRUE, TALENT_CAT_ALCHEMY);
  talento(TALENT_RAPID_ALCHEMY, "rapid alchemy", 1, 1000, 10,
          "-2 seconds on alchemy skill task completion per rank",
          "Reduces completion time for tasks performed with the alchemy skill by 2 seconds per rank.", TRUE, TALENT_CAT_ALCHEMY);
  talento(TALENT_ALCHEMY_EXPERTISE, "alchemy expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the alchemy skill.", TRUE, TALENT_CAT_ALCHEMY);
  talento(TALENT_EFFICIENT_ALCHEMY, "efficient alchemy", 1, 2500, 10,
          "chance to use less materials on alchemy skill tasks",
          "Each rank gives a 3% chance to save half materials when using the alchemy skill.", TRUE, TALENT_CAT_ALCHEMY);
  talento(TALENT_INSIGHTFUL_ALCHEMY, "insightful alchemy", 2, 5000, 5,
          "Chance for extra experience on success for alchemy skill tasks",
          "Chance for extra experience on success for alchemy skill tasks", TRUE, TALENT_CAT_ALCHEMY);
  
  /* Armorsmithing talents */
  talento(TALENT_PROFICIENT_ARMORSMITHING, "proficient armorsmithing", 1, 1000, 5,
          "+1 to skill checks for the armorsmithing skill",
          "Each rank gives +1 to skill checks made with the armorsmithing skill.", TRUE, TALENT_CAT_ARMORSMITHING);
  talento(TALENT_RAPID_ARMORSMITHING, "rapid armorsmithing", 1, 1000, 10,
          "-2 seconds on armorsmithing skill task completion per rank",
          "Reduces completion time for tasks performed with the armorsmithing skill by 2 seconds per rank.", TRUE, TALENT_CAT_ARMORSMITHING);
  talento(TALENT_ARMORSMITHING_EXPERTISE, "armorsmithing expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the armorsmithing skill.", TRUE, TALENT_CAT_ARMORSMITHING);
  talento(TALENT_EFFICIENT_ARMORSMITHING, "efficient armorsmithing", 1, 2500, 10,
          "chance to use less materials on armorsmithing skill tasks",
          "Each rank gives a 3% chance to save half materials when using the armorsmithing skill.", TRUE, TALENT_CAT_ARMORSMITHING);
  talento(TALENT_INSIGHTFUL_ARMORSMITHING, "insightful armorsmithing", 2, 5000, 5,
          "Chance for extra experience on success for armorsmithing skill tasks",
          "Chance for extra experience on success for armorsmithing skill tasks", TRUE, TALENT_CAT_ARMORSMITHING);
  
  /* Weaponsmithing talents */
  talento(TALENT_PROFICIENT_WEAPONSMITHING, "proficient weaponsmithing", 1, 1000, 5,
          "+1 to skill checks for the weaponsmithing skill",
          "Each rank gives +1 to skill checks made with the weaponsmithing skill.", TRUE, TALENT_CAT_WEAPONSMITHING);
  talento(TALENT_RAPID_WEAPONSMITHING, "rapid weaponsmithing", 1, 1000, 10,
          "-2 seconds on weaponsmithing skill task completion per rank",
          "Reduces completion time for tasks performed with the weaponsmithing skill by 2 seconds per rank.", TRUE, TALENT_CAT_WEAPONSMITHING);
  talento(TALENT_WEAPONSMITHING_EXPERTISE, "weaponsmithing expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the weaponsmithing skill.", TRUE, TALENT_CAT_WEAPONSMITHING);
  talento(TALENT_EFFICIENT_WEAPONSMITHING, "efficient weaponsmithing", 1, 2500, 10,
          "chance to use less materials on weaponsmithing skill tasks",
          "Each rank gives a 3% chance to save half materials when using the weaponsmithing skill.", TRUE, TALENT_CAT_WEAPONSMITHING);
  talento(TALENT_INSIGHTFUL_WEAPONSMITHING, "insightful weaponsmithing", 2, 5000, 5,
          "Chance for extra experience on success for weaponsmithing skill tasks",
          "Chance for extra experience on success for weaponsmithing skill tasks", TRUE, TALENT_CAT_WEAPONSMITHING);
  
  /* Bowmaking talents */
  talento(TALENT_PROFICIENT_BOWMAKING, "proficient bowmaking", 1, 1000, 5,
          "+1 to skill checks for the bowmaking skill",
          "Each rank gives +1 to skill checks made with the bowmaking skill.", FALSE, TALENT_CAT_BOWMAKING);
  talento(TALENT_RAPID_BOWMAKING, "rapid bowmaking", 1, 1000, 10,
          "-2 seconds on bowmaking skill task completion per rank",
          "Reduces completion time for tasks performed with the bowmaking skill by 2 seconds per rank.", FALSE, TALENT_CAT_BOWMAKING);
  talento(TALENT_BOWMAKING_EXPERTISE, "bowmaking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the bowmaking skill.", FALSE, TALENT_CAT_BOWMAKING);
  talento(TALENT_EFFICIENT_BOWMAKING, "efficient bowmaking", 1, 2500, 10,
          "chance to use less materials on bowmaking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the bowmaking skill.", FALSE, TALENT_CAT_BOWMAKING);
  talento(TALENT_INSIGHTFUL_BOWMAKING, "insightful bowmaking", 2, 5000, 5,
          "Chance for extra experience on success for bowmaking skill tasks",
          "Chance for extra experience on success for bowmaking skill tasks", FALSE, TALENT_CAT_BOWMAKING);
  
  /* Jewelcrafting talents */
  talento(TALENT_PROFICIENT_JEWELCRAFTING, "proficient jewelcrafting", 1, 1000, 5,
          "+1 to skill checks for the jewelcrafting skill",
          "Each rank gives +1 to skill checks made with the jewelcrafting skill.", TRUE, TALENT_CAT_JEWELCRAFTING);
  talento(TALENT_RAPID_JEWELCRAFTING, "rapid jewelcrafting", 1, 1000, 10,
          "-2 seconds on jewelcrafting skill task completion per rank",
          "Reduces completion time for tasks performed with the jewelcrafting skill by 2 seconds per rank.", TRUE, TALENT_CAT_JEWELCRAFTING);
  talento(TALENT_JEWELCRAFTING_EXPERTISE, "jewelcrafting expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the jewelcrafting skill.", TRUE, TALENT_CAT_JEWELCRAFTING);
  talento(TALENT_EFFICIENT_JEWELCRAFTING, "efficient jewelcrafting", 1, 2500, 10,
          "chance to use less materials on jewelcrafting skill tasks",
          "Each rank gives a 3% chance to save half materials when using the jewelcrafting skill.", TRUE, TALENT_CAT_JEWELCRAFTING);
  talento(TALENT_INSIGHTFUL_JEWELCRAFTING, "insightful jewelcrafting", 2, 5000, 5,
          "Chance for extra experience on success for jewelcrafting skill tasks",
          "Chance for extra experience on success for jewelcrafting skill tasks", TRUE, TALENT_CAT_JEWELCRAFTING);
  
  /* Leatherworking talents */
  talento(TALENT_PROFICIENT_LEATHERWORKING, "proficient leatherworking", 1, 1000, 5,
          "+1 to skill checks for the leatherworking skill",
          "Each rank gives +1 to skill checks made with the leatherworking skill.", TRUE, TALENT_CAT_LEATHERWORKING);
  talento(TALENT_RAPID_LEATHERWORKING, "rapid leatherworking", 1, 1000, 10,
          "-2 seconds on leatherworking skill task completion per rank",
          "Reduces completion time for tasks performed with the leatherworking skill by 2 seconds per rank.", TRUE, TALENT_CAT_LEATHERWORKING);
  talento(TALENT_LEATHERWORKING_EXPERTISE, "leatherworking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the leatherworking skill.", TRUE, TALENT_CAT_LEATHERWORKING);
  talento(TALENT_EFFICIENT_LEATHERWORKING, "efficient leatherworking", 1, 2500, 10,
          "chance to use less materials on leatherworking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the leatherworking skill.", TRUE, TALENT_CAT_LEATHERWORKING);
  talento(TALENT_INSIGHTFUL_LEATHERWORKING, "insightful leatherworking", 2, 5000, 5,
          "Chance for extra experience on success for leatherworking skill tasks",
          "Chance for extra experience on success for leatherworking skill tasks", TRUE, TALENT_CAT_LEATHERWORKING);
  
  /* Trapmaking talents */
  talento(TALENT_PROFICIENT_TRAPMAKING, "proficient trapmaking", 1, 1000, 5,
          "+1 to skill checks for the trapmaking skill",
          "Each rank gives +1 to skill checks made with the trapmaking skill.", FALSE, TALENT_CAT_TRAPMAKING);
  talento(TALENT_RAPID_TRAPMAKING, "rapid trapmaking", 1, 1000, 10,
          "-2 seconds on trapmaking skill task completion per rank",
          "Reduces completion time for tasks performed with the trapmaking skill by 2 seconds per rank.", FALSE, TALENT_CAT_TRAPMAKING);
  talento(TALENT_TRAPMAKING_EXPERTISE, "trapmaking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the trapmaking skill.", FALSE, TALENT_CAT_TRAPMAKING);
  talento(TALENT_EFFICIENT_TRAPMAKING, "efficient trapmaking", 1, 2500, 10,
          "chance to use less materials on trapmaking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the trapmaking skill.", FALSE, TALENT_CAT_TRAPMAKING);
  talento(TALENT_INSIGHTFUL_TRAPMAKING, "insightful trapmaking", 2, 5000, 5,
          "Chance for extra experience on success for trapmaking skill tasks",
          "Chance for extra experience on success for trapmaking skill tasks", FALSE, TALENT_CAT_TRAPMAKING);
  
  /* Poisonmaking talents */
  talento(TALENT_PROFICIENT_POISONMAKING, "proficient poisonmaking", 1, 1000, 5,
          "+1 to skill checks for the poisonmaking skill",
          "Each rank gives +1 to skill checks made with the poisonmaking skill.", FALSE, TALENT_CAT_POISONMAKING);
  talento(TALENT_RAPID_POISONMAKING, "rapid poisonmaking", 1, 1000, 10,
          "-2 seconds on poisonmaking skill task completion per rank",
          "Reduces completion time for tasks performed with the poisonmaking skill by 2 seconds per rank.", FALSE, TALENT_CAT_POISONMAKING);
  talento(TALENT_POISONMAKING_EXPERTISE, "poisonmaking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the poisonmaking skill.", FALSE, TALENT_CAT_POISONMAKING);
  talento(TALENT_EFFICIENT_POISONMAKING, "efficient poisonmaking", 1, 2500, 10,
          "chance to use less materials on poisonmaking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the poisonmaking skill.", FALSE, TALENT_CAT_POISONMAKING);
  talento(TALENT_INSIGHTFUL_POISONMAKING, "insightful poisonmaking", 2, 5000, 5,
          "Chance for extra experience on success for poisonmaking skill tasks",
          "Chance for extra experience on success for poisonmaking skill tasks", FALSE, TALENT_CAT_POISONMAKING);
  
  /* Metalworking talents */
  talento(TALENT_PROFICIENT_METALWORKING, "proficient metalworking", 1, 1000, 5,
          "+1 to skill checks for the metalworking skill",
          "Each rank gives +1 to skill checks made with the metalworking skill.", TRUE, TALENT_CAT_METALWORKING);
  talento(TALENT_RAPID_METALWORKING, "rapid metalworking", 1, 1000, 10,
          "-2 seconds on metalworking skill task completion per rank",
          "Reduces completion time for tasks performed with the metalworking skill by 2 seconds per rank.", TRUE, TALENT_CAT_METALWORKING);
  talento(TALENT_METALWORKING_EXPERTISE, "metalworking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the metalworking skill.", TRUE, TALENT_CAT_METALWORKING);
  talento(TALENT_EFFICIENT_METALWORKING, "efficient metalworking", 1, 2500, 10,
          "chance to use less materials on metalworking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the metalworking skill.", TRUE, TALENT_CAT_METALWORKING);
  talento(TALENT_INSIGHTFUL_METALWORKING, "insightful metalworking", 2, 5000, 5,
          "Chance for extra experience on success for metalworking skill tasks",
          "Chance for extra experience on success for metalworking skill tasks", TRUE, TALENT_CAT_METALWORKING);
  
  /* Fishing talents */
  talento(TALENT_PROFICIENT_FISHING, "proficient fishing", 1, 1000, 5,
          "+1 to skill checks for the fishing skill",
          "Each rank gives +1 to skill checks made with the fishing skill.", FALSE, TALENT_CAT_FISHING);
  talento(TALENT_RAPID_FISHING, "rapid fishing", 1, 1000, 10,
          "-2 seconds on fishing skill task completion per rank",
          "Reduces completion time for tasks performed with the fishing skill by 2 seconds per rank.", FALSE, TALENT_CAT_FISHING);
  talento(TALENT_FISHING_EXPERTISE, "fishing expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the fishing skill.", FALSE, TALENT_CAT_FISHING);
  talento(TALENT_EFFICIENT_FISHING, "efficient fishing", 1, 2500, 10,
          "chance to use less materials on fishing skill tasks",
          "Each rank gives a 3% chance to save half materials when using the fishing skill.", FALSE, TALENT_CAT_FISHING);
  talento(TALENT_INSIGHTFUL_FISHING, "insightful fishing", 2, 5000, 5,
          "Chance for extra experience on success for fishing skill tasks",
          "Chance for extra experience on success for fishing skill tasks", FALSE, TALENT_CAT_FISHING);
  
  /* Cooking talents */
  talento(TALENT_PROFICIENT_COOKING, "proficient cooking", 1, 1000, 5,
          "+1 to skill checks for the cooking skill",
          "Each rank gives +1 to skill checks made with the cooking skill.", FALSE, TALENT_CAT_COOKING);
  talento(TALENT_RAPID_COOKING, "rapid cooking", 1, 1000, 10,
          "-2 seconds on cooking skill task completion per rank",
          "Reduces completion time for tasks performed with the cooking skill by 2 seconds per rank.", FALSE, TALENT_CAT_COOKING);
  talento(TALENT_COOKING_EXPERTISE, "cooking expertise", 2, 2500, 5,
          "+1% chance to critical success when crafting",
          "+1% chance to critical success when crafting something with the cooking skill.", FALSE, TALENT_CAT_COOKING);
  talento(TALENT_EFFICIENT_COOKING, "efficient cooking", 1, 2500, 10,
          "chance to use less materials on cooking skill tasks",
          "Each rank gives a 3% chance to save half materials when using the cooking skill.", FALSE, TALENT_CAT_COOKING);
  talento(TALENT_INSIGHTFUL_COOKING, "insightful cooking", 2, 5000, 5,
          "Chance for extra experience on success for cooking skill tasks",
          "Chance for extra experience on success for cooking skill tasks", FALSE, TALENT_CAT_COOKING);
  
  /* Mining talents (harvesting) */
  talento(TALENT_PROFICIENT_MINING, "proficient mining", 1, 1000, 5,
          "+1 to skill checks for the mining skill",
          "Each rank gives +1 to skill checks made with the mining skill.", TRUE, TALENT_CAT_MINING);
  talento(TALENT_RAPID_MINING, "rapid mining", 1, 1000, 5,
          "-1 second on mining skill task completion per rank",
          "Reduces completion time for tasks performed with the mining skill by 1 second per rank.", TRUE, TALENT_CAT_MINING);
  talento(TALENT_MINING_EXPERTISE, "mining expertise", 2, 2500, 5,
          "On critical success, chance to gain extra materials",
          "On critical success when mining there's a 10% chance per rank to gain extra materials.", TRUE, TALENT_CAT_MINING);
  talento(TALENT_EFFICIENT_MINING, "efficient mining", 1, 2500, 10,
          "chance to use less materials on mining skill tasks",
          "Each rank gives a 3% chance to gain 2 extra units when harvesting with the mining skill.", TRUE, TALENT_CAT_MINING);
  talento(TALENT_INSIGHTFUL_MINING, "insightful mining", 2, 5000, 5,
          "Chance for extra experience on success for mining skill tasks",
          "Chance for extra experience on success for mining skill tasks", TRUE, TALENT_CAT_MINING);
  
  /* Hunting talents (harvesting) */
  talento(TALENT_PROFICIENT_HUNTING, "proficient hunting", 1, 1000, 5,
          "+1 to skill checks for the hunting skill",
          "Each rank gives +1 to skill checks made with the hunting skill.", TRUE, TALENT_CAT_HUNTING);
  talento(TALENT_RAPID_HUNTING, "rapid hunting", 1, 1000, 5,
          "-1 second on hunting skill task completion per rank",
          "Reduces completion time for tasks performed with the hunting skill by 1 second per rank.", TRUE, TALENT_CAT_HUNTING);
  talento(TALENT_HUNTING_EXPERTISE, "hunting expertise", 2, 2500, 5,
          "On critical success, chance to gain extra materials",
          "On critical success when hunting there's a 10% chance per rank to gain extra materials.", TRUE, TALENT_CAT_HUNTING);
  talento(TALENT_EFFICIENT_HUNTING, "efficient hunting", 1, 2500, 10,
          "chance to use less materials on hunting skill tasks",
          "Each rank gives a 3% chance to gain 2 extra units when harvesting with the hunting skill.", TRUE, TALENT_CAT_HUNTING);
  talento(TALENT_INSIGHTFUL_HUNTING, "insightful hunting", 2, 5000, 5,
          "Chance for extra experience on success for hunting skill tasks",
          "Chance for extra experience on success for hunting skill tasks", TRUE, TALENT_CAT_HUNTING);
  
  /* Forestry talents (harvesting) */
  talento(TALENT_PROFICIENT_FORESTRY, "proficient forestry", 1, 1000, 5,
          "+1 to skill checks for the forestry skill",
          "Each rank gives +1 to skill checks made with the forestry skill.", TRUE, TALENT_CAT_FORESTRY);
  talento(TALENT_RAPID_FORESTRY, "rapid forestry", 1, 1000, 5,
          "-1 second on forestry skill task completion per rank",
          "Reduces completion time for tasks performed with the forestry skill by 1 second per rank.", TRUE, TALENT_CAT_FORESTRY);
  talento(TALENT_FORESTRY_EXPERTISE, "forestry expertise", 2, 2500, 5,
          "On critical success, chance to gain extra materials",
          "On critical success when harvesting wood there's a 10% chance per rank to gain extra materials.", TRUE, TALENT_CAT_FORESTRY);
  talento(TALENT_EFFICIENT_FORESTRY, "efficient forestry", 1, 2500, 10,
          "chance to use less materials on forestry skill tasks",
          "Each rank gives a 3% chance to gain 2 extra units when harvesting with the forestry skill.", TRUE, TALENT_CAT_FORESTRY);
  talento(TALENT_INSIGHTFUL_FORESTRY, "insightful forestry", 2, 5000, 5,
          "Chance for extra experience on success for forestry skill tasks",
          "Chance for extra experience on success for forestry skill tasks", TRUE, TALENT_CAT_FORESTRY);
  
  /* Gathering talents (harvesting) */
  talento(TALENT_PROFICIENT_GATHERING, "proficient gathering", 1, 1000, 5,
          "+1 to skill checks for the gathering skill",
          "Each rank gives +1 to skill checks made with the gathering skill.", TRUE, TALENT_CAT_GATHERING);
  talento(TALENT_RAPID_GATHERING, "rapid gathering", 1, 1000, 5,
          "-1 second on gathering skill task completion per rank",
          "Reduces completion time for tasks performed with the gathering skill by 1 second per rank.", TRUE, TALENT_CAT_GATHERING);
  talento(TALENT_GATHERING_EXPERTISE, "gathering expertise", 2, 2500, 5,
          "On critical success, chance to gain extra materials",
          "On critical success when gathering there's a 10% chance per rank to gain extra materials.", TRUE, TALENT_CAT_GATHERING);
  talento(TALENT_EFFICIENT_GATHERING, "efficient gathering", 1, 2500, 10,
          "chance to use less materials on gathering skill tasks",
          "Each rank gives a 3% chance to gain 2 extra units when harvesting with the gathering skill.", TRUE, TALENT_CAT_GATHERING);
  talento(TALENT_INSIGHTFUL_GATHERING, "insightful gathering", 2, 5000, 5,
          "Chance for extra experience on success for gathering skill tasks",
          "Chance for extra experience on success for gathering skill tasks", TRUE, TALENT_CAT_GATHERING);
}

/* Player data integration helpers */
int get_talent_rank(struct char_data *ch, int talent) {
  if (!ch || IS_NPC(ch)) return 0;
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  return GET_TALENT_RANK(ch, talent);
}

int has_talent(struct char_data *ch, int talent) {
  return get_talent_rank(ch, talent) > 0;
}

static int current_rank(struct char_data *ch, int talent) {
  return get_talent_rank(ch, talent);
}

int talent_max_ranks(int talent) {
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  return MAX(1, talent_list[talent].max_ranks);
}

/* Cost progression: costs multiply by rank number.
 * Rank 1: base*1, Rank 2: base*2, Rank 3: base*3, Rank 4: base*4, etc. */
int talent_next_point_cost(struct char_data *ch, int talent) {
  if (!ch || talent <= 0 || talent >= TALENT_MAX) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  int base = talent_list[talent].base_point_cost;
  return base * (rank + 1); /* base * next_rank */
}

int talent_next_gold_cost(struct char_data *ch, int talent) {
  if (!ch || talent <= 0 || talent >= TALENT_MAX) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  int base = talent_list[talent].base_gold_cost;
  return base * (rank + 1); /* base * next_rank */
}

int can_learn_talent(struct char_data *ch, int talent) {
  if (!ch || IS_NPC(ch)) return 0;
  if (talent <= 0 || talent >= TALENT_MAX) return 0;
  if (!talent_list[talent].in_game) return 0;
  int rank = current_rank(ch, talent);
  if (rank >= talent_max_ranks(talent)) return 0;
  int p_cost = talent_next_point_cost(ch, talent);
  int g_cost = talent_next_gold_cost(ch, talent);
  if (p_cost <= 0) return 0;
  if (GET_TALENT_POINTS(ch) < p_cost) return 0;
  if (g_cost > 0 && GET_GOLD(ch) < g_cost) return 0;
  return 1;
}

int learn_talent(struct char_data *ch, int talent) {
  if (!can_learn_talent(ch, talent)) return 0;
  int p_cost = talent_next_point_cost(ch, talent);
  int g_cost = talent_next_gold_cost(ch, talent);
  /* Deduct costs */
  if (p_cost > 0) GET_TALENT_POINTS(ch) -= p_cost;
  if (g_cost > 0) {
    if (GET_GOLD(ch) < g_cost) return 0; /* double-check */
    GET_GOLD(ch) -= g_cost;
  }
  /* Increase rank */
  int rank = current_rank(ch, talent);
  (ch)->player_specials->saved.talent_ranks[talent] = MIN(rank + 1, talent_max_ranks(talent));
  if (rank == 0)
    send_to_char(ch, "\tGYou learn the talent: %s (rank %d/%d)!\tn\r\n", talent_list[talent].name, rank+1, talent_max_ranks(talent));
  else
    send_to_char(ch, "\tGYou improve %s to rank %d/%d!\tn\r\n", talent_list[talent].name, rank+1, talent_max_ranks(talent));
  send_to_char(ch, "Cost: %d talent point%s, %d gold\r\n", p_cost, p_cost==1?"":"s", g_cost);
  return 1;
}

void list_talents(struct char_data *ch) {
  struct talent_sort_entry sorted[TALENT_MAX];
  int count, i, idx;
  char left_col[MAX_STRING_LENGTH], right_col[MAX_STRING_LENGTH];
  int col_toggle = 0;
  
  send_to_char(ch, "\r\n\tCCrafting Talents (Ranked System - Alphabetical)\tn\r\n");
  send_to_char(ch, "==============================================================================\r\n");
  send_to_char(ch, "You have \tW%d\tn unspent talent point%s.  Gold: \tY%d\tn\r\n\r\n", 
               GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  
  /* Get sorted list of in_game talents */
  count = get_sorted_talents(ch, sorted, FALSE, TRUE, FALSE);
  
  left_col[0] = '\0';
  right_col[0] = '\0';
  
  for (idx = 0; idx < count; idx++) {
    i = sorted[idx].talent_id;
    int rank = get_talent_rank(ch, i);
    int maxr = talent_max_ranks(i);
    int p_cost = talent_next_point_cost(ch, i);
    int g_cost = talent_next_gold_cost(ch, i);
    char line[256];
    
    if (rank >= maxr)
      snprintf(line, sizeof(line), "\tW%2d\tn) %-24s %d/%-2d \tR[MAX]\tn", 
               i, talent_list[i].name, rank, maxr);
    else if (rank > 0)
      snprintf(line, sizeof(line), "\tW%2d\tn) %-24s %d/%-2d %2dpt/%4dgp", 
               i, talent_list[i].name, rank, maxr, p_cost, g_cost);
    else
      snprintf(line, sizeof(line), "\tW%2d\tn) %-24s \tD%d/%-2d\tn %2dpt/%4dgp", 
               i, talent_list[i].name, rank, maxr, p_cost, g_cost);
    
    if (col_toggle == 0) {
      snprintf(left_col, sizeof(left_col), "%s", line);
      col_toggle = 1;
    } else {
      snprintf(right_col, sizeof(right_col), "%s", line);
      send_to_char(ch, "%-39s %s\r\n", left_col, right_col);
      col_toggle = 0;
      left_col[0] = '\0';
      right_col[0] = '\0';
    }
  }
  
  /* Print any remaining left column entry */
  if (col_toggle == 1) {
    send_to_char(ch, "%s\r\n", left_col);
  }
  
  send_to_char(ch, "\r\nUse '\tCtalents learn <number>\tn' to learn or rank up a talent.\r\n");
  send_to_char(ch, "Use '\tCtalents info <number>\tn' to see details about a talent.\r\n");
  send_to_char(ch, "Use '\tCtalents available\tn' to see only talents you can afford.\r\n");
  send_to_char(ch, "Use '\tCtalents all\tn' to see ALL talents (including disabled).\r\n");
}

void gain_talent_point(struct char_data *ch, int amount) {
  if (!ch || IS_NPC(ch) || amount <= 0) return;
  GET_TALENT_POINTS(ch) += amount;
  send_to_char(ch, "\tGYou gain %d crafting talent point%s!\tn\r\n", amount, amount==1?"":"s");
}

/* Helper: list only talents the character can currently afford */
void list_available_talents(struct char_data *ch) {
  struct talent_sort_entry sorted[TALENT_MAX];
  int count, idx, i;
  char left_col[MAX_STRING_LENGTH], right_col[MAX_STRING_LENGTH];
  int col_toggle = 0;
  
  send_to_char(ch, "\r\n\tCAvailable Talents (You Can Afford - Alphabetical)\tn\r\n");
  send_to_char(ch, "==============================================================================\r\n");
  send_to_char(ch, "You have \tW%d\tn unspent talent point%s.  Gold: \tY%d\tn\r\n\r\n", 
               GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  
  /* Get sorted list of talents player can afford */
  count = get_sorted_talents(ch, sorted, TRUE, TRUE, FALSE);
  
  left_col[0] = '\0';
  right_col[0] = '\0';
  
  for (idx = 0; idx < count; idx++) {
    i = sorted[idx].talent_id;
    int rank = get_talent_rank(ch, i);
    int maxr = talent_max_ranks(i);
    int p_cost = talent_next_point_cost(ch, i);
    int g_cost = talent_next_gold_cost(ch, i);
    char line[256];
    
    snprintf(line, sizeof(line), "\tG%2d\tn) %-24s %d/%-2d %2dpt/%4dgp", 
             i, talent_list[i].name, rank, maxr, p_cost, g_cost);
    
    if (col_toggle == 0) {
      snprintf(left_col, sizeof(left_col), "%s", line);
      col_toggle = 1;
    } else {
      snprintf(right_col, sizeof(right_col), "%s", line);
      send_to_char(ch, "%-39s %s\r\n", left_col, right_col);
      col_toggle = 0;
      left_col[0] = '\0';
      right_col[0] = '\0';
    }
  }
  
  /* Print any remaining left column entry */
  if (col_toggle == 1) {
    send_to_char(ch, "%s\r\n", left_col);
  }
  
  if (count == 0)
    send_to_char(ch, "You cannot afford any talents right now.\r\n");
  else
    send_to_char(ch, "\r\nUse '\tCtalents learn <number>\tn' to learn a talent.\r\n");
}

/* Helper: list ALL talents including disabled ones (for admin review) */
void list_all_talents(struct char_data *ch) {
  struct talent_sort_entry sorted[TALENT_MAX];
  int count, idx, i;
  char left_col[MAX_STRING_LENGTH], right_col[MAX_STRING_LENGTH];
  int col_toggle = 0;
  
  send_to_char(ch, "\r\n\tCAll Talents (Alphabetical)\tn\r\n");
  send_to_char(ch, "==============================================================================\r\n");
  send_to_char(ch, "You have \tW%d\tn unspent talent point%s.  Gold: \tY%d\tn\r\n\r\n", 
               GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  
  /* Get sorted list of in-game talents (exclude disabled) */
  count = get_sorted_talents(ch, sorted, FALSE, TRUE, FALSE);
  
  left_col[0] = '\0';
  right_col[0] = '\0';
  
  for (idx = 0; idx < count; idx++) {
    i = sorted[idx].talent_id;
    int rank = get_talent_rank(ch, i);
    int maxr = talent_max_ranks(i);
    int p_cost = talent_next_point_cost(ch, i);
    int g_cost = talent_next_gold_cost(ch, i);
    char line[256];
    
    if (rank >= maxr)
      snprintf(line, sizeof(line), "\tW%2d\tn) %-24s %d/%-2d \tR[MAX]\tn", 
               i, talent_list[i].name, rank, maxr);
    else
      snprintf(line, sizeof(line), "\tW%2d\tn) %-24s %d/%-2d %2dpt/%4dgp", 
               i, talent_list[i].name, rank, maxr, p_cost, g_cost);
    
    if (col_toggle == 0) {
      snprintf(left_col, sizeof(left_col), "%s", line);
      col_toggle = 1;
    } else {
      snprintf(right_col, sizeof(right_col), "%s", line);
      send_to_char(ch, "%-39s %s\r\n", left_col, right_col);
      col_toggle = 0;
      left_col[0] = '\0';
      right_col[0] = '\0';
    }
  }
  
  /* Print any remaining left column entry */
  if (col_toggle == 1) {
    send_to_char(ch, "%s\r\n", left_col);
  }
  
}/* Helper: show detailed info about a specific talent */
void show_talent_info(struct char_data *ch, int talent) {
  if (talent <= 0 || talent >= TALENT_MAX) {
    send_to_char(ch, "Invalid talent number.\r\n");
    return;
  }
  
  int rank = get_talent_rank(ch, talent);
  int maxr = talent_max_ranks(talent);
  int p_cost = talent_next_point_cost(ch, talent);
  int g_cost = talent_next_gold_cost(ch, talent);
  
  send_to_char(ch, "\r\n\tC=== %s ===\tn\r\n", talent_list[talent].name);
  send_to_char(ch, "Status: %s\r\n", talent_list[talent].in_game ? "\tG[ACTIVE]\tn" : "\tD[DISABLED]\tn");
  send_to_char(ch, "Your Rank: \tW%d/%d\tn\r\n", rank, maxr);
  
  if (rank >= maxr) {
    send_to_char(ch, "This talent is \tRMAXED OUT\tn.\r\n");
  } else {
    send_to_char(ch, "Next Rank Cost: \tY%d talent point%s\tn and \tY%d gold\tn\r\n", 
                 p_cost, p_cost==1?"":"s", g_cost);
    send_to_char(ch, "Can Learn: %s\r\n", can_learn_talent(ch, talent) ? "\tGYES\tn" : "\tRNO\tn");
  }
  
  /* Show long description only */
  if (talent_list[talent].long_desc && *talent_list[talent].long_desc) {
    send_to_char(ch, "\r\nDescription:\r\n%s\r\n", talent_list[talent].long_desc);
  }
  
  /* Show cost progression for all ranks - one per line */
  send_to_char(ch, "\r\n\tCCost Progression (Rank: TP/GP):\tn\r\n");
  int r, base_pt = talent_list[talent].base_point_cost, base_gp = talent_list[talent].base_gold_cost;
  
  for (r = 1; r <= maxr; r++) {
    int rank_pt = base_pt * r;
    int rank_gp = base_gp * r;
    
    if (r <= rank)
      send_to_char(ch, "  \tGRank %2d: %4d TP / %d GP\tn\r\n", r, rank_pt, rank_gp);
    else if (r == rank + 1)
      send_to_char(ch, "  \tYRank %2d: %4d TP / %d GP\tn\r\n", r, rank_pt, rank_gp);
    else
      send_to_char(ch, "  Rank %2d: %4d TP / %d GP\r\n", r, rank_pt, rank_gp);
  }
  send_to_char(ch, "\r\n");
}

/* Helper: list available talent categories with talent counts */
void list_talent_categories(struct char_data *ch) {
  int cat, i, count;
  int cat_counts[NUM_TALENT_CATEGORIES];
  
  /* Count talents in each category (only in-game talents) */
  for (cat = 0; cat < NUM_TALENT_CATEGORIES; cat++) {
    cat_counts[cat] = 0;
  }
  
  for (i = 1; i < TALENT_MAX; i++) {
    if (talent_list[i].in_game) {
      cat_counts[talent_list[i].category]++;
    }
  }
  
  send_to_char(ch, "\r\n\tCTalent Categories\tn\r\n");
  send_to_char(ch, "==============================================================================\r\n");
  send_to_char(ch, "You have \tW%d\tn unspent talent point%s.  Gold: \tY%d\tn\r\n\r\n", 
               GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  
  for (cat = 0; cat < NUM_TALENT_CATEGORIES; cat++) {
    count = cat_counts[cat];
    if (count > 0) {
      send_to_char(ch, "  \tC%-20s\tn - %d talent%s\r\n", 
                   talent_category_names[cat], count, count==1?"":"s");
    }
  }
  
  send_to_char(ch, "\r\nUse '\tCtalents <category>\tn' to view talents in a specific category.\r\n");
  send_to_char(ch, "Example: '\tCtalents woodworking\tn' or '\tCtalents alchemy\tn'\r\n");
}

/* Helper: list talents in a specific category */
void list_talents_by_category(struct char_data *ch, int category) {
  struct talent_sort_entry sorted[TALENT_MAX];
  int count, i, idx;
  
  if (category < 0 || category >= NUM_TALENT_CATEGORIES) {
    send_to_char(ch, "Invalid category.\r\n");
    return;
  }
  
  /* Get sorted list of in_game talents, filtered by category */
  count = 0;
  for (i = 1; i < TALENT_MAX; i++) {
    if (talent_list[i].in_game && talent_list[i].category == category) {
      sorted[count].talent_id = i;
      sorted[count].name = talent_list[i].name;
      count++;
    }
  }
  
  /* Sort alphabetically */
  qsort(sorted, count, sizeof(struct talent_sort_entry), compare_talents_alpha);
  
  send_to_char(ch, "\r\n\tC%s Talents (Alphabetical)\tn\r\n", talent_category_names[category]);
  send_to_char(ch, "==============================================================================\r\n");
  send_to_char(ch, "You have \tW%d\tn unspent talent point%s.  Gold: \tY%d\tn\r\n\r\n", 
               GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
  
  if (count == 0) {
    send_to_char(ch, "No talents available in this category.\r\n");
    return;
  }
  
  /* Single column display */
  for (idx = 0; idx < count; idx++) {
    i = sorted[idx].talent_id;
    int rank = get_talent_rank(ch, i);
    int maxr = talent_max_ranks(i);
    int p_cost = talent_next_point_cost(ch, i);
    int g_cost = talent_next_gold_cost(ch, i);
    
    if (rank >= maxr)
      send_to_char(ch, "\tW%2d\tn) %-32s %d/%-2d \tR[MAX]\tn\r\n", 
               i, talent_list[i].name, rank, maxr);
    else if (rank > 0)
      send_to_char(ch, "\tW%2d\tn) %-32s %d/%-2d  Next: %2dpt/%5dgp\r\n", 
               i, talent_list[i].name, rank, maxr, p_cost, g_cost);
    else
      send_to_char(ch, "\tW%2d\tn) %-32s \tD%d/%-2d\tn  Cost: %2dpt/%5dgp\r\n", 
               i, talent_list[i].name, rank, maxr, p_cost, g_cost);
  }
  
  send_to_char(ch, "\r\nUse '\tCtalents learn <number>\tn' to learn or rank up a talent.\r\n");
  send_to_char(ch, "Use '\tCtalents info <number|name>\tn' to see details about a talent.\r\n");
  send_to_char(ch, "Use '\tCtalents\tn' to see all categories.\r\n");
}

/* Commands */
ACMD(do_talents) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH];
  int cat;
  
  if (IS_NPC(ch)) {
    send_to_char(ch, "NPCs cannot use talents.\r\n");
    return;
  }
  
  /* Copy argument to preserve spaces in talent names */
  strcpy(buf, argument);
  half_chop(buf, arg1, arg2);
  
  if (!*arg1) {
    list_talent_categories(ch);
    return;
  }
  
  if (is_abbrev(arg1, "available")) {
    list_available_talents(ch);
    return;
  }
  
  if (is_abbrev(arg1, "all")) {
    list_all_talents(ch);
    return;
  }
  
  if (is_abbrev(arg1, "info")) {
    if (!*arg2) {
      send_to_char(ch, "Specify a talent number or name to view details.\r\n");
      return;
    }
    
    /* Try to parse as number first */
    int num = atoi(arg2);
    
    /* If not a valid number or zero, search by name */
    if (num <= 0 || num >= TALENT_MAX) {
      int found = -1;
      int i;
      size_t arg_len = strlen(arg2);
      
      /* First try exact case-insensitive match */
      for (i = 1; i < TALENT_MAX; i++) {
        if (strcasecmp(arg2, talent_list[i].name) == 0) {
          found = i;
          break;
        }
      }
      
      /* If no exact match, try prefix match (case-insensitive) */
      if (found == -1) {
        for (i = 1; i < TALENT_MAX; i++) {
          if (strncasecmp(arg2, talent_list[i].name, arg_len) == 0) {
            if (found == -1) {
              found = i;
            } else {
              /* Multiple matches - list them all */
              send_to_char(ch, "Multiple talents match that name. Please be more specific:\r\n");
              send_to_char(ch, "  %s\r\n", talent_list[found].name);
              send_to_char(ch, "  %s\r\n", talent_list[i].name);
              /* Continue to show all matches */
              for (i++; i < TALENT_MAX; i++) {
                if (strncasecmp(arg2, talent_list[i].name, arg_len) == 0) {
                  send_to_char(ch, "  %s\r\n", talent_list[i].name);
                }
              }
              return;
            }
          }
        }
      }
      
      if (found == -1) {
        send_to_char(ch, "No talent found matching '%s'.\r\n", arg2);
        return;
      }
      
      num = found;
    }
    
    show_talent_info(ch, num);
    return;
  }
  
  if (is_abbrev(arg1, "learn")) {
    if (!*arg2) {
      send_to_char(ch, "Specify a talent number to learn.\r\n");
      send_to_char(ch, "Usage: talents learn <number>\r\n");
      return;
    }
    int num = atoi(arg2);
    if (num <= 0 || num >= TALENT_MAX) {
      send_to_char(ch, "Invalid talent number. Use 'talents' to see the list.\r\n");
      return;
    }
    
    if (!talent_list[num].in_game) {
      send_to_char(ch, "That talent is not available.\r\n");
      return;
    }
    
    int rank = get_talent_rank(ch, num);
    int maxr = talent_max_ranks(num);
    int p_cost = talent_next_point_cost(ch, num);
    int g_cost = talent_next_gold_cost(ch, num);
    
    if (!can_learn_talent(ch, num)) {
      if (rank >= maxr)
        send_to_char(ch, "That talent is already at maximum rank (%d/%d).\r\n", rank, maxr);
      else if (GET_TALENT_POINTS(ch) < p_cost)
        send_to_char(ch, "You need \tR%d talent point%s\tn (you have %d) to learn the next rank.\r\n", 
                     p_cost, p_cost==1?"":"s", GET_TALENT_POINTS(ch));
      else if (g_cost > 0 && GET_GOLD(ch) < g_cost)
        send_to_char(ch, "You need \tY%d gold\tn (you have %d) to learn the next rank.\r\n", 
                     g_cost, GET_GOLD(ch));
      else
        send_to_char(ch, "You cannot learn that talent right now.\r\n");
      return;
    }
    
    if (learn_talent(ch, num)) {
      send_to_char(ch, "Remaining: %d talent point%s, %d gold\r\n", 
                   GET_TALENT_POINTS(ch), GET_TALENT_POINTS(ch)==1?"":"s", GET_GOLD(ch));
      return;
    }
    
    send_to_char(ch, "Learning failed.\r\n");
    return;
  }
  
  /* Check if arg1 matches a category name */
  for (cat = 0; cat < NUM_TALENT_CATEGORIES; cat++) {
    if (is_abbrev(arg1, talent_category_names[cat])) {
      list_talents_by_category(ch, cat);
      return;
    }
  }
  
  send_to_char(ch, "Unknown option or category.\r\n");
  send_to_char(ch, "Usage:\r\n");
  send_to_char(ch, "  talents                 - List talent categories\r\n");
  send_to_char(ch, "  talents <category>      - List talents in a category\r\n");
  send_to_char(ch, "  talents available       - List only talents you can afford\r\n");
  send_to_char(ch, "  talents all             - List all talents\r\n");
  send_to_char(ch, "  talents info <num>      - Show detailed info about a talent\r\n");
  send_to_char(ch, "  talents learn <num>     - Learn or rank up a talent\r\n");
}

/* Admin command to set talent points or ranks */
ACMD(do_talento) {
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH], arg3[MAX_INPUT_LENGTH];
  int talent_num, value;
  
  three_arguments(argument, name, sizeof(name), arg2, sizeof(arg2), arg3, sizeof(arg3));
  
  if (!*name || !*arg2) {
    send_to_char(ch, "Usage: talento <player> <talent_num|'points'> [value]\r\n");
    send_to_char(ch, "  talento <player> points <amount>  - Set talent points\r\n");
    send_to_char(ch, "  talento <player> <num> <rank>     - Set talent rank\r\n");
    send_to_char(ch, "  talento <player> <num>             - Show talent info for player\r\n");
    return;
  }
  
  if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "No such player.\r\n");
    return;
  }
  
  if (IS_NPC(vict)) {
    send_to_char(ch, "NPCs cannot have talents.\r\n");
    return;
  }
  
  if (is_abbrev(arg2, "points")) {
    if (!*arg3) {
      send_to_char(ch, "%s currently has %d talent point%s.\r\n", 
                   GET_NAME(vict), GET_TALENT_POINTS(vict), 
                   GET_TALENT_POINTS(vict)==1?"":"s");
      return;
    }
    value = atoi(arg3);
    GET_TALENT_POINTS(vict) = MAX(0, value);
    send_to_char(ch, "%s now has %d talent point%s.\r\n", 
                 GET_NAME(vict), GET_TALENT_POINTS(vict), 
                 GET_TALENT_POINTS(vict)==1?"":"s");
    send_to_char(vict, "Your talent points have been set to %d by an immortal.\r\n", 
                 GET_TALENT_POINTS(vict));
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, 
           "(GC) %s set %s's talent points to %d", 
           GET_NAME(ch), GET_NAME(vict), GET_TALENT_POINTS(vict));
    return;
  }
  
  talent_num = atoi(arg2);
  if (talent_num <= 0 || talent_num >= TALENT_MAX) {
    send_to_char(ch, "Invalid talent number (1-%d).\r\n", TALENT_MAX - 1);
    return;
  }
  
  if (!*arg3) {
    int rank = get_talent_rank(vict, talent_num);
    int maxr = talent_max_ranks(talent_num);
    send_to_char(ch, "%s's talent '%s': Rank %d/%d\r\n", 
                 GET_NAME(vict), talent_list[talent_num].name, rank, maxr);
    return;
  }
  
  value = atoi(arg3);
  if (value < 0 || value > 255) {
    send_to_char(ch, "Rank must be between 0 and 255.\r\n");
    return;
  }
  
  if (talent_num > 0 && talent_num < 64)
    vict->player_specials->saved.talent_ranks[talent_num] = (ubyte)value;
  
  send_to_char(ch, "%s's talent '%s' set to rank %d.\r\n", 
               GET_NAME(vict), talent_list[talent_num].name, value);
  send_to_char(vict, "Your talent '%s' has been set to rank %d by an immortal.\r\n", 
               talent_list[talent_num].name, value);
  mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, 
         "(GC) %s set %s's talent '%s' to rank %d", 
         GET_NAME(ch), GET_NAME(vict), talent_list[talent_num].name, value);
}
