/*
 * do_brew - Potion brewing system for LuminariMUD
 * 
 * This function allows characters to brew potions using:
 * - Elemental motes (based on spell school)
 * - Gold (based on spell level) 
 * - Must have spell prepared or be able to cast it
 * - Alchemists can brew any spell available to any class up to their level/3
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "constants.h"
#include "act.h"
#include "class.h"
#include "spell_prep.h"
#include "crafting_new.h"
#include "domains_schools.h"
#include "helpers.h"
#include "mud_event.h"
#include "crafting_new.h"
#include "interpreter.h"
#include "brew.h"

/* External function declarations */
extern int find_skill_num(char *name);
extern int spell_school(int spellnum);
extern void save_char(struct char_data *ch, int load_room);

/* Map CRAFT_SKILL_* constants to ABILITY_* constants */
int craft_skill_to_ability(int craft_skill)
{
  switch (craft_skill) {
    case CRAFT_SKILL_BREWING:
      return ABILITY_CRAFT_ALCHEMY;
    case CRAFT_SKILL_WEAPONSMITH:
      return ABILITY_CRAFT_WEAPONSMITHING;
    case CRAFT_SKILL_ARMORSMITH:
      return ABILITY_CRAFT_ARMORSMITHING;
    case CRAFT_SKILL_JEWELER:
      return ABILITY_CRAFT_JEWELCRAFTING;
    case CRAFT_SKILL_CARPENTER:
      return ABILITY_CRAFT_WOODWORKING;
    case CRAFT_SKILL_TAILOR:
      return ABILITY_CRAFT_TAILORING;
    case CRAFT_SKILL_TINKER:
      return ABILITY_CRAFT_METALWORKING; /* Closest match */
    default:
      return ABILITY_CRAFT_ALCHEMY; /* Default fallback */
  }
}

/* Forward declarations */
struct obj_data *create_potion(int spell_num, struct char_data *ch);
struct obj_data *create_multi_spell_potion(int *spell_nums, int num_spells, struct char_data *ch);

/* Mud event for brewing completion */
EVENTFUNC(event_brewing)
{
  struct char_data *ch = NULL;
  struct mud_event_data *pMudEvent = NULL;
  char *data_str = NULL;
  int spell_nums[3] = {-1, -1, -1};
  int num_spells = 0, highest_circle = 0, brewing_skill = 0;
  int total_motes_by_type[NUM_CRAFT_MOTES] = {0};
  int total_gold = 0;
  int dc = 0, d20_roll = 0, roll = 0;
  bool critical_success = FALSE;
  int i, cycle;
  
  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  
  if (pMudEvent->pStruct == NULL)
    return 0;
    
  ch = (struct char_data *)pMudEvent->pStruct;
  
  /* Parse data from the sVariables string */
  if (pMudEvent->sVariables) {
    data_str = strdup(pMudEvent->sVariables);
    if (data_str) {
      char *token;
      int token_count = 0;
      
      /* Parse: spell1,spell2,spell3,num_spells,highest_circle,brewing_skill,dc,total_motes_type0,total_motes_type1,...,total_gold,cost_multiplier */
      token = strtok(data_str, ",");
      while (token && token_count < (6 + NUM_CRAFT_MOTES + 2)) {
        switch (token_count) {
          case 0: spell_nums[0] = atoi(token); break;
          case 1: spell_nums[1] = atoi(token); break;
          case 2: spell_nums[2] = atoi(token); break;
          case 3: num_spells = atoi(token); break;
          case 4: highest_circle = atoi(token); break;
          case 5: brewing_skill = atoi(token); break;
          case 6: dc = atoi(token); break;
          default:
            if (token_count >= 7 && token_count < (7 + NUM_CRAFT_MOTES)) {
              total_motes_by_type[token_count - 7] = atoi(token);
            } else if (token_count == (7 + NUM_CRAFT_MOTES)) {
              total_gold = atoi(token);
            }
            /* Note: cost_multiplier is parsed but not used in event since costs are pre-calculated */
            break;
        }
        token = strtok(NULL, ",");
        token_count++;
      }
      free(data_str);
    }
  }
  
  if (!ch || num_spells == 0)
    return 0;
    
  /* NOW perform the skill check at the end of brewing */
  d20_roll = d20(ch);
  roll = d20_roll + brewing_skill;
  
  /* Display skill check details */
  send_to_char(ch, "Alchemy skill check: d20(%d) + skill(%d) = %d vs DC %d\r\n", 
               d20_roll, brewing_skill, roll, dc);
  
  /* Check for critical failure (natural 1) */
  if (d20_roll == 1) {
    send_to_char(ch, "Critical failure! You botch the alchemy process completely!\r\n");
    send_to_char(ch, "You lose half your materials in the disastrous attempt:\r\n");
    
    /* Show and consume half the materials on critical failure */
    for (i = 0; i < NUM_CRAFT_MOTES; i++) {
      if (total_motes_by_type[i] > 0) {
        int lost = total_motes_by_type[i] / 2;
        if (lost > 0) {
          send_to_char(ch, "  %d %s%s lost\r\n", lost, crafting_motes[i], lost > 1 ? "s" : "");
          GET_CRAFT_MOTES(ch, i) -= lost;
        }
      }
    }
    int gold_lost = total_gold / 2;
    if (gold_lost > 0) {
      send_to_char(ch, "  %d gold lost\r\n", gold_lost);
      GET_GOLD(ch) -= gold_lost;
    }
    
    /* Give minimal alchemy experience for critical failure */
    if (brewing_skill < LVL_STAFF) {
      int exp_gain = highest_circle * 5 + (num_spells - 1) * 2;
      GET_CRAFT_SKILL_EXP(ch, craft_skill_to_ability(CRAFT_SKILL_BREWING)) += exp_gain;
      send_to_char(ch, "You gain %d alchemy experience from the failed attempt.\r\n", exp_gain);
    }
    
    save_char(ch, 0);
    return 0;
  }
  
  /* Check for regular failure */
  if (roll < dc) {
    send_to_char(ch, "Your alchemy attempt fails!\r\n");
    send_to_char(ch, "You lose some materials in the failed attempt:\r\n");
    
    int gold_lost = total_gold / 4;
    if (gold_lost > 0) {
      send_to_char(ch, "  %d gold lost\r\n", gold_lost);
      GET_GOLD(ch) -= gold_lost;
    }
    
    /* Give some alchemy experience for regular failure */
    if (brewing_skill < LVL_STAFF) {
      int exp_gain = MAX(5, highest_circle * 3 / 4);
      GET_CRAFT_SKILL_EXP(ch, craft_skill_to_ability(CRAFT_SKILL_BREWING)) += exp_gain;
      send_to_char(ch, "You gain %d alchemy experience from the failed attempt.\r\n", exp_gain);
    }
    
    save_char(ch, 0);
    return 0;
  }
  
  /* Check for critical success (natural 20) */
  critical_success = (d20_roll == 20);
  if (critical_success) {
    send_to_char(ch, "Critical success! Your alchemy technique is flawless!\r\n");
    send_to_char(ch, "You create two potions instead of one!\r\n");
  } else {
    send_to_char(ch, "Success! Your alchemy attempt succeeds.\r\n");
  }
  
  /* Success! Consume all materials */
  send_to_char(ch, "Materials consumed for your alchemy success:\r\n");
  for (i = 0; i < NUM_CRAFT_MOTES; i++) {
    if (total_motes_by_type[i] > 0) {
      send_to_char(ch, "  %d %s%s used\r\n", total_motes_by_type[i], crafting_motes[i], total_motes_by_type[i] > 1 ? "s" : "");
      GET_CRAFT_MOTES(ch, i) -= total_motes_by_type[i];
    }
  }
  if (total_gold > 0) {
    send_to_char(ch, "  %d gold used\r\n", total_gold);
    GET_GOLD(ch) -= total_gold;
  }
  
  /* Consume spell slots for spontaneous casters */
  send_to_char(ch, "Spell components consumed:\r\n");
  for (i = 0; i < num_spells; i++) {
    if (spell_nums[i] > 0) {
      int casting_class = spell_prep_gen_extract(ch, spell_nums[i], 0);
      if (casting_class != CLASS_UNDEFINED) {
        send_to_char(ch, "  %s spell slot used\r\n", spell_info[spell_nums[i]].name);
      } else {
        /* This shouldn't happen if we did proper validation, but just in case */
        send_to_char(ch, "  %s (no spell slot consumed - not prepared/known)\r\n", 
                     spell_info[spell_nums[i]].name);
      }
    }
  }
    
  /* Success messages */
  if (num_spells == 1) {
    send_to_char(ch, "\tGYou finish brewing %s potion%s of %s!\tn\r\n", 
                 critical_success ? "two exceptional" : "a", 
                 critical_success ? "s" : "",
                 spell_info[spell_nums[0]].name);
  } else {
    send_to_char(ch, "\tGYou finish brewing %s complex %d-spell potion%s!\tn\r\n", 
                 critical_success ? "two exceptional" : "a",
                 num_spells,
                 critical_success ? "s" : "");
    send_to_char(ch, "The potion%s contain%s:\r\n", 
                 critical_success ? "s" : "",
                 critical_success ? "" : "s");
    for (i = 0; i < num_spells; i++) {
      if (spell_nums[i] > 0) {
        send_to_char(ch, "  - %s\r\n", spell_info[spell_nums[i]].name);
      }
    }
  }
  
  act("$n finishes creating a magical potion.", TRUE, ch, 0, 0, TO_ROOM);
  
  /* Create potion objects or add to stored consumables based on preference */
  int total_potions_created = 0;
  int creation_cycles = critical_success ? 2 : 1;
  
  /* Check if player is using stored consumables system */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_USE_STORED_CONSUMABLES)) {
    /* Add to stored consumables */
    for (cycle = 0; cycle < creation_cycles; cycle++) {
      for (i = 0; i < num_spells; i++) {
        if (spell_nums[i] > 0) {
          STORED_POTIONS(ch, spell_nums[i])++;
          total_potions_created++;
        }
      }
    }
    
    if (total_potions_created > 0) {
      send_to_char(ch, "\tCYou successfully added %d potion%s to your stored consumables!\tn\r\n", 
                   total_potions_created, total_potions_created > 1 ? "s" : "");
      if (num_spells == 1) {
        send_to_char(ch, "Stored: %d potion%s of %s\r\n", 
                     creation_cycles, creation_cycles > 1 ? "s" : "",
                     spell_info[spell_nums[0]].name);
      } else {
        send_to_char(ch, "Stored potions for each spell:\r\n");
        for (i = 0; i < num_spells; i++) {
          if (spell_nums[i] > 0) {
            send_to_char(ch, "  %d potion%s of %s\r\n", 
                         creation_cycles, creation_cycles > 1 ? "s" : "",
                         spell_info[spell_nums[i]].name);
          }
        }
      }
    }
  } else {
    /* Create physical potion objects */
    for (cycle = 0; cycle < creation_cycles; cycle++) {
      struct obj_data *potion = create_multi_spell_potion(spell_nums, num_spells, ch);
      if (potion) {
        obj_to_char(potion, ch);
        total_potions_created++;
      } else {
        send_to_char(ch, "\tRSomething went wrong creating the potion!\tn\r\n");
      }
    }
    
    if (total_potions_created > 0) {
      send_to_char(ch, "\tCYou successfully created %d physical potion%s!\tn\r\n", 
                   total_potions_created, total_potions_created > 1 ? "s" : "");
    }
  }
  
  /* Give additional alchemy experience for completion */
  if (brewing_skill < LVL_STAFF) {
    int completion_exp = highest_circle * 20 + (num_spells - 1) * 10;
    if (critical_success) {
      completion_exp = (completion_exp * 3) / 2;  /* 50% bonus experience for critical success */
      send_to_char(ch, "You gain %d alchemy experience for your masterful technique!\r\n", completion_exp);
    } else {
      send_to_char(ch, "You gain %d alchemy experience for completing the process.\r\n", completion_exp);
    }
    GET_CRAFT_SKILL_EXP(ch, craft_skill_to_ability(CRAFT_SKILL_BREWING)) += completion_exp;
  }
  
  /* Save character */
  save_char(ch, 0);
  
  return 0;
}

/* Map spell schools to elemental mote types for brewing */
int get_mote_type_for_school(int school) {
  switch (school) {
    case ABJURATION:    return CRAFTING_MOTE_LIGHT;     /* Protection/warding */
    case CONJURATION:   return CRAFTING_MOTE_EARTH;     /* Creation/summoning */
    case DIVINATION:    return CRAFTING_MOTE_AIR;       /* Knowledge/detection */
    case ENCHANTMENT:   return CRAFTING_MOTE_LIGHTNING; /* Mind affecting */
    case EVOCATION:     return CRAFTING_MOTE_FIRE;      /* Energy/force */
    case ILLUSION:      return CRAFTING_MOTE_ICE;       /* Deception/shadow */
    case NECROMANCY:    return CRAFTING_MOTE_DARK;      /* Death/negative energy */
    case TRANSMUTATION: return CRAFTING_MOTE_WATER;     /* Change/alteration */
    default:            return CRAFTING_MOTE_EARTH;     /* Default fallback */
  }
}

/* Calculate mote cost based on spell circle/level */
int get_motes_required_for_spell(int spell_circle) {
  switch (spell_circle) {
    case 0:  return 1;   /* Cantrips/orisons */
    case 1:  return 1;   /* 1st level */
    case 2:  return 1;   /* 2nd level */
    case 3:  return 2;   /* 3rd level */
    case 4:  return 2;   /* 4th level */
    case 5:  return 3;   /* 5th level */
    case 6:  return 3;   /* 6th level */
    case 7:  return 4;   /* 7th level */
    case 8:  return 4;   /* 8th level */
    case 9:  return 5;   /* 9th level */
    case 10: return 10;  /* Epic spells */
    default: return 1;
  }
}

/* Calculate gold cost based on spell circle */
int get_gold_cost_for_spell(int spell_circle) {
  switch (spell_circle) {
    case 0:  return 25;     /* Cantrips/orisons */
    case 1:  return 50;     /* 1st level */
    case 2:  return 100;    /* 2nd level */
    case 3:  return 200;    /* 3rd level */
    case 4:  return 400;    /* 4th level */
    case 5:  return 600;    /* 5th level */
    case 6:  return 900;    /* 6th level */
    case 7:  return 1200;   /* 7th level */
    case 8:  return 1600;   /* 8th level */
    case 9:  return 2000;   /* 9th level */
    case 10: return 5000;   /* Epic spells */
    default: return 25;
  }
}

/* Check if character can cast the spell (non-alchemist) */
int can_cast_spell(struct char_data *ch, int spellnum) {
  int class, min_level;
  
  /* Check if character can cast the spell */
  for (class = 0; class < NUM_CLASSES; class++) {
    if (!IS_SPELLCASTER_CLASS(class)) {
      continue;
    }
    
    min_level = spell_info[spellnum].min_level[class];
    if (min_level > 0 && min_level < LVL_STAFF) {
      if (CLASS_LEVEL(ch, class) >= min_level) {
        /* For spontaneous casters like sorcerers, check if they know the spell */
        if (class == CLASS_SORCERER || class == CLASS_BARD) {
          if (is_a_known_spell(ch, class, spellnum)) {
            return TRUE;
          }
        } else {
          /* For prepared casters, check if spell is prepared */
          if (is_spell_in_collection(ch, class, spellnum, METAMAGIC_NONE)) {
            return TRUE;
          }
        }
      }
    }
  }
  
  return FALSE;
}

/* Check if alchemist can brew a spell */
bool alchemist_can_brew_spell(struct char_data *ch, int spellnum) {
  int class, min_level, max_alchemist_level;
  
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) == 0) {
    return FALSE;
  }
  
  max_alchemist_level = CLASS_LEVEL(ch, CLASS_ALCHEMIST) / 3;
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 30) {
    max_alchemist_level = 10; /* Can brew epic spells at level 30 */
  }
  
  /* Check if any class can cast this spell and if alchemist level is sufficient */
  for (class = 0; class < NUM_CLASSES; class++) {
    if (!IS_SPELLCASTER_CLASS(class)) {
      continue;
    }
    
    min_level = spell_info[spellnum].min_level[class];
    if (min_level > 0 && min_level < LVL_STAFF) {
      int spell_circle = (min_level + 1) / 2;
      if (spell_circle <= max_alchemist_level) {
        return TRUE;
      }
    }
  }
  
  return FALSE;
}

/* Get the minimum spell circle across all classes that can cast it */
int get_minimum_spell_circle(int spellnum) {
  int class, min_circle = 10, min_level;
  
  for (class = 0; class < NUM_CLASSES; class++) {
    if (!IS_SPELLCASTER_CLASS(class)) {
      continue;
    }
    
    min_level = spell_info[spellnum].min_level[class];
    if (min_level > 0 && min_level < LVL_STAFF) {
      int spell_circle = (min_level + 1) / 2;
      if (spell_circle < min_circle) {
        min_circle = spell_circle;
      }
    }
  }
  
  return (min_circle == 10) ? 1 : min_circle; /* Default to circle 1 if not found */
}

/* Function to find a spell by full name (case-insensitive, partial match) */
int find_spell_by_name(char *name)
{
  int i, j;
  char temp_name[MAX_INPUT_LENGTH];
  char spell_name[MAX_INPUT_LENGTH];
  
  if (!name || !*name)
    return -1;
    
  /* Convert input to lowercase for comparison */
  strcpy(temp_name, name);
  for (i = 0; temp_name[i]; i++)
    temp_name[i] = tolower(temp_name[i]);
  
  /* Check for exact matches first */
  for (i = 1; i <= TOP_SPELL_DEFINE; i++)
  {
    if (!spell_info[i].name || !*spell_info[i].name)
      continue;
      
    strcpy(spell_name, spell_info[i].name);
    for (j = 0; spell_name[j]; j++)
      spell_name[j] = tolower(spell_name[j]);
      
    if (!strcmp(temp_name, spell_name))
      return i;
  }
  
  /* Check for partial matches if no exact match found */
  for (i = 1; i <= TOP_SPELL_DEFINE; i++)
  {
    if (!spell_info[i].name || !*spell_info[i].name)
      continue;
      
    strcpy(spell_name, spell_info[i].name);
    for (j = 0; spell_name[j]; j++)
      spell_name[j] = tolower(spell_name[j]);
      
    if (is_abbrev(temp_name, spell_name))
      return i;
  }
  
  return -1;
}

/* Check if character can brew the spell */
bool can_brew_spell(struct char_data *ch, int spell_num)
{
  int spell_level = 0;
  
  if (!ch || spell_num < 1 || spell_num > TOP_SPELL_DEFINE)
    return FALSE;
    
  /* Check if it's a valid spell */
  if (!spell_info[spell_num].name || !*spell_info[spell_num].name)
    return FALSE;
    
  /* Get spell level - this would need to be implemented based on your spell system */
  spell_level = spell_info[spell_num].min_level[CLASS_WIZARD]; /* Default to wizard level */
  
  /* Check if it's an offensive spell (not allowed for brewing) */
  if (IS_SET(spell_info[spell_num].routines, MAG_DAMAGE))
  {
    send_to_char(ch, "You cannot brew offensive spells into potions.\r\n");
    return FALSE;
  }
  
  /* Check if character is an alchemist or can cast the spell */
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0)
  {
    /* Alchemist can brew any spell within their level limits */
    int max_spell_level = CLASS_LEVEL(ch, CLASS_ALCHEMIST) / 3;
    if (max_spell_level < 1) max_spell_level = 1;
    if (spell_level > max_spell_level)
    {
      send_to_char(ch, "That spell is too high level for you to brew.\r\n");
      return FALSE;
    }
  }
  else
  {
    /* Non-alchemists must be able to cast the spell */
    bool can_cast = FALSE;
    int class_num;
    
    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
    {
      if (CLASS_LEVEL(ch, class_num) > 0 && 
          spell_info[spell_num].min_level[class_num] <= CLASS_LEVEL(ch, class_num))
      {
        can_cast = TRUE;
        break;
      }
    }
    
    if (!can_cast)
    {
      send_to_char(ch, "You don't know how to cast that spell.\r\n");
      return FALSE;
    }
  }
  
  return TRUE;
}

/* Check if character has required materials */
bool has_brew_materials(struct char_data *ch, int spell_num)
{
  int spell_circle = get_minimum_spell_circle(spell_num);
  int school = spell_info[spell_num].schoolOfMagic;
  int mote_type = get_mote_type_for_school(school);
  int motes_required = get_motes_required_for_spell(spell_circle);
  int gold_required = get_gold_cost_for_spell(spell_circle);
  
  /* Alchemists pay more if they can't cast the spell themselves */
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0) {
    if (!can_cast_spell(ch, spell_num)) {
      /* Can't cast the spell - pay 2.5x cost */
      motes_required = (motes_required * 5) / 2;  /* 2.5x cost */
      gold_required = (gold_required * 5) / 2;    /* 2.5x cost */
    }
    /* If they can cast it, they pay normal cost */
  }
  
  /* Check if character has enough gold */
  if (GET_GOLD(ch) < gold_required) {
    send_to_char(ch, "You need %d gold coins to brew this potion.\r\n", gold_required);
    return FALSE;
  }
  
  /* Check if character has enough motes */
  if (GET_CRAFT_MOTES(ch, mote_type) < motes_required) {
    char mote_name[64];

    /* Get the proper mote name based on type */
    if (mote_type >= 0 && mote_type < NUM_CRAFT_MOTES) {
      snprintf(mote_name, sizeof(mote_name), "%s", crafting_motes[mote_type]);
    }
    else
    {
      send_to_char(ch, "Invalid mote type.\r\n");
      return FALSE;
    }
    
    send_to_char(ch, "You need %d %s%s to brew this potion (you have %d).\r\n", 
                 motes_required, mote_name, motes_required > 1 ? "s" : "",  GET_CRAFT_MOTES(ch, mote_type));
    return FALSE;
  }
  
  return TRUE;
}

/* Consume materials for brewing */
void consume_brew_materials(struct char_data *ch, int spell_num)
{
  int spell_circle = get_minimum_spell_circle(spell_num);
  int school = spell_info[spell_num].schoolOfMagic;
  int mote_type = get_mote_type_for_school(school);
  int motes_required = get_motes_required_for_spell(spell_circle);
  int gold_required = get_gold_cost_for_spell(spell_circle);
  
  /* Alchemists pay more if they can't cast the spell themselves */
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0) {
    if (!can_cast_spell(ch, spell_num)) {
      /* Can't cast the spell - pay 2.5x cost */
      motes_required = (motes_required * 5) / 2;  /* 2.5x cost */
      gold_required = (gold_required * 5) / 2;    /* 2.5x cost */
    }
    /* If they can cast it, they pay normal cost */
  }
  
  /* Consume gold */
  GET_GOLD(ch) -= gold_required;
  
  /* Consume motes */
  int current_motes = GET_CRAFT_MOTES(ch, mote_type);
  GET_CRAFT_MOTES(ch, mote_type) = current_motes - motes_required;
  
  /* Save character after consuming materials */
  save_char(ch, 0);
}

/* Create a potion object for the given spell */
struct obj_data *create_potion(int spell_num, struct char_data *ch)
{
  struct obj_data *potion = NULL;
  char short_desc[128], long_desc[256], name[128];
  int spell_circle = get_minimum_spell_circle(spell_num);
  int caster_level = 1;
  int class_num;
  
  /* Calculate the brewer's caster level for this spell */
  for (class_num = 0; class_num < NUM_CLASSES; class_num++) {
    if (!IS_SPELLCASTER_CLASS(class_num)) {
      continue;
    }
    
    int min_level = spell_info[spell_num].min_level[class_num];
    if (min_level > 0 && min_level < LVL_STAFF && CLASS_LEVEL(ch, class_num) >= min_level) {
      int class_caster_level = CLASS_LEVEL(ch, class_num);
      if (class_caster_level > caster_level) {
        caster_level = class_caster_level;
      }
    }
  }
  
  /* For alchemists, use their alchemist level as caster level */
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0) {
    caster_level = CLASS_LEVEL(ch, CLASS_ALCHEMIST);
  }
  
  /* Create a new object */
  potion = create_obj();
  if (!potion)
    return NULL;
    
  /* Set object type and flags */
  GET_OBJ_TYPE(potion) = ITEM_POTION;
  SET_BIT_AR(GET_OBJ_WEAR(potion), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(potion), ITEM_WEAR_HOLD);
  
  /* Set spell values */
  GET_OBJ_VAL(potion, 0) = caster_level;     /* Caster level */
  GET_OBJ_VAL(potion, 1) = spell_num;        /* Spell number */
  GET_OBJ_VAL(potion, 2) = -1;               /* No second spell */
  GET_OBJ_VAL(potion, 3) = -1;               /* No third spell */
  
  /* Set basic properties */
  GET_OBJ_WEIGHT(potion) = 1;
  GET_OBJ_COST(potion) = get_gold_cost_for_spell(spell_circle);
  GET_OBJ_RENT(potion) = GET_OBJ_COST(potion) / 10;
  
  /* Create descriptions */
  snprintf(name, sizeof(name), "potion %s", spell_info[spell_num].name);
  potion->name = strdup(name);
  
  snprintf(short_desc, sizeof(short_desc), "a potion of %s", spell_info[spell_num].name);
  potion->short_description = strdup(short_desc);
  
  snprintf(long_desc, sizeof(long_desc), "A small vial containing a magical potion of %s lies here.", 
           spell_info[spell_num].name);
  potion->description = strdup(long_desc);
  
  return potion;
}

/* Create a multi-spell potion */
struct obj_data *create_multi_spell_potion(int *spell_nums, int num_spells, struct char_data *ch)
{
  struct obj_data *potion = NULL;
  char short_desc[256], long_desc[512], name[256];
  int highest_circle = 0;
  int caster_level = 1;
  int class_num, i;
  
  if (num_spells <= 0 || num_spells > 3) {
    return NULL;
  }
  
  /* Calculate the highest spell circle and brewer's caster level */
  for (i = 0; i < num_spells; i++) {
    if (spell_nums[i] > 0) {
      int spell_circle = get_minimum_spell_circle(spell_nums[i]);
      if (spell_circle > highest_circle) {
        highest_circle = spell_circle;
      }
    }
  }
  
  /* Calculate the brewer's caster level */
  for (class_num = 0; class_num < NUM_CLASSES; class_num++) {
    if (!IS_SPELLCASTER_CLASS(class_num)) {
      continue;
    }
    
    int class_caster_level = CLASS_LEVEL(ch, class_num);
    if (class_caster_level > caster_level) {
      caster_level = class_caster_level;
    }
  }
  
  /* For alchemists, use their alchemist level as caster level */
  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0) {
    caster_level = CLASS_LEVEL(ch, CLASS_ALCHEMIST);
  }
  
  /* Create a new object */
  potion = create_obj();
  if (!potion)
    return NULL;
    
  /* Set object type and flags */
  GET_OBJ_TYPE(potion) = ITEM_POTION;
  SET_BIT_AR(GET_OBJ_WEAR(potion), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(potion), ITEM_WEAR_HOLD);
  
  /* Set spell values */
  GET_OBJ_VAL(potion, 0) = caster_level;     /* Caster level */
  GET_OBJ_VAL(potion, 1) = (num_spells >= 1 && spell_nums[0] > 0) ? spell_nums[0] : -1;
  GET_OBJ_VAL(potion, 2) = (num_spells >= 2 && spell_nums[1] > 0) ? spell_nums[1] : -1;
  GET_OBJ_VAL(potion, 3) = (num_spells >= 3 && spell_nums[2] > 0) ? spell_nums[2] : -1;
  
  /* Set basic properties - higher cost for multi-spell potions */
  GET_OBJ_WEIGHT(potion) = 1;
  GET_OBJ_COST(potion) = get_gold_cost_for_spell(highest_circle) * num_spells;
  GET_OBJ_RENT(potion) = GET_OBJ_COST(potion) / 10;
  
  /* Create descriptions */
  if (num_spells == 1) {
    snprintf(name, sizeof(name), "potion %s", spell_info[spell_nums[0]].name);
    snprintf(short_desc, sizeof(short_desc), "a potion of %s", spell_info[spell_nums[0]].name);
    snprintf(long_desc, sizeof(long_desc), "A small vial containing a magical potion of %s lies here.", 
             spell_info[spell_nums[0]].name);
  } else {
    snprintf(name, sizeof(name), "potion complex multi-spell");
    snprintf(short_desc, sizeof(short_desc), "a complex %d-spell potion", num_spells);
    snprintf(long_desc, sizeof(long_desc), "A small vial containing a complex magical potion with %d spells lies here.", 
             num_spells);
  }
  
  potion->name = strdup(name);
  potion->short_description = strdup(short_desc);
  potion->description = strdup(long_desc);
  
  return potion;
}

/* Main brew command - Enhanced to support multiple spells */
ACMD(do_brew)
{
  char spell_names[3][MAX_INPUT_LENGTH];
  char data_string[1024]; /* Increased size for longer data string */
  int spell_nums[3] = {-1, -1, -1};
  int spell_levels[3] = {0, 0, 0};
  int spell_circles[3], mote_types[3];
  int motes_required[3], gold_required[3];
  int total_motes_by_type[NUM_CRAFT_MOTES] = {0};
  int total_gold = 0, brew_time = 0;
  int num_spells = 0, highest_circle = 0, i;
  float cost_multiplier = 1.0;
  struct mud_event_data *pMudEvent = NULL;
  const char *arg_ptr;
  int brewing_skill, dc;
  
  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot brew potions.\r\n");
    return;
  }
  
  /* Check if already brewing */
  if (char_has_mud_event(ch, eBREWING))
  {
    send_to_char(ch, "You are already brewing a potion.\r\n");
    return;
  }
  
  /* Parse arguments for up to 3 spells */
  arg_ptr = argument;
  
  /* Skip leading whitespace */
  while (*arg_ptr && isspace(*arg_ptr)) arg_ptr++;
  
  if (!*arg_ptr)
  {
    send_to_char(ch, "Brew which spell(s) into a potion?\r\n");
    send_to_char(ch, "Usage: brew <spell1> [spell2] [spell3]\r\n");
    send_to_char(ch, "\r\nPotion creation requires:\r\n");
    send_to_char(ch, "- Elemental motes (type depends on spell school)\r\n");
    send_to_char(ch, "- Gold (amount depends on spell level)\r\n");
    send_to_char(ch, "- Ability to cast the spell OR be an alchemist\r\n");
    send_to_char(ch, "- Alchemy skill check (DC based on highest spell level)\r\n");
    send_to_char(ch, "\r\nMultiple spells on one potion:\r\n");
    send_to_char(ch, "- 2 spells: 1.5x total cost\r\n");
    send_to_char(ch, "- 3 spells: 2.0x total cost\r\n");
    send_to_char(ch, "\r\nExample: brew 'cure light wounds' 'bless' 'shield'\r\n");
    return;
  }
  
  /* Parse spell names - handle quoted and unquoted spells */
  for (i = 0; i < 3 && *arg_ptr; i++) {
    /* Skip leading whitespace */
    while (*arg_ptr && isspace(*arg_ptr)) arg_ptr++;
    if (!*arg_ptr) break;
    
    if (*arg_ptr == '\'' || *arg_ptr == '\"') {
      /* Quoted spell name */
      char quote = *arg_ptr++;
      const char *start = arg_ptr;
      const char *end = strchr(start, quote);
      if (!end) {
        send_to_char(ch, "Unterminated quote in spell name.\r\n");
        return;
      }
      size_t len = end - start;
      if (len >= MAX_INPUT_LENGTH) len = MAX_INPUT_LENGTH - 1;
      strncpy(spell_names[i], start, len);
      spell_names[i][len] = '\0';
      arg_ptr = end + 1;
    } else {
      /* Unquoted spell name - read until next space or end */
      const char *start = arg_ptr;
      while (*arg_ptr && !isspace(*arg_ptr)) arg_ptr++;
      if (arg_ptr > start) {
        size_t len = arg_ptr - start;
        if (len >= MAX_INPUT_LENGTH) len = MAX_INPUT_LENGTH - 1;
        strncpy(spell_names[i], start, len);
        spell_names[i][len] = '\0';
      } else {
        break;
      }
    }
    num_spells++;
  }
  
  if (num_spells == 0) {
    send_to_char(ch, "You must specify at least one spell to brew.\r\n");
    return;
  }
  
  /* Find and validate each spell */
  for (i = 0; i < num_spells; i++) {
    spell_nums[i] = find_spell_by_name(spell_names[i]);
    if (spell_nums[i] < 1) {
      send_to_char(ch, "There is no such spell '%s'.\r\n", spell_names[i]);
      return;
    }
    
    /* Check if character can brew this spell */
    if (!can_brew_spell(ch, spell_nums[i])) {
      return; /* Error message already sent by can_brew_spell */
    }
    
    /* Calculate individual spell costs */
    spell_circles[i] = get_minimum_spell_circle(spell_nums[i]);
    spell_levels[i] = spell_info[spell_nums[i]].min_level[CLASS_WIZARD];
    if (spell_levels[i] < 1) spell_levels[i] = 1;
    
    mote_types[i] = get_mote_type_for_school(spell_info[spell_nums[i]].schoolOfMagic);
    motes_required[i] = get_motes_required_for_spell(spell_circles[i]);
    gold_required[i] = get_gold_cost_for_spell(spell_circles[i]);
    
    /* Track highest circle for DC calculation */
    if (spell_circles[i] > highest_circle) {
      highest_circle = spell_circles[i];
    }
    
    /* Apply alchemist cost modifiers */
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) > 0 && !can_cast_spell(ch, spell_nums[i])) {
      motes_required[i] = (motes_required[i] * 5) / 2;  /* 2.5x cost */
      gold_required[i] = (gold_required[i] * 5) / 2;    /* 2.5x cost */
    }
  }
  
  /* Apply cost multipliers for multiple spells */
  if (num_spells == 2) {
    cost_multiplier = 1.5;
  } else if (num_spells == 3) {
    cost_multiplier = 2.0;
  }
  
  /* Calculate total costs and apply multiplier */
  for (i = 0; i < num_spells; i++) {
    motes_required[i] = (int)(motes_required[i] * cost_multiplier);
    gold_required[i] = (int)(gold_required[i] * cost_multiplier);
    total_motes_by_type[mote_types[i]] += motes_required[i];
    total_gold += gold_required[i];
  }
  
  /* Check if character has spells prepared/known and slots available */
  send_to_char(ch, "Checking spell availability...\r\n");
  for (i = 0; i < num_spells; i++) {
    if (spell_nums[i] > 0) {
      if (spell_prep_gen_check(ch, spell_nums[i], 0) == CLASS_UNDEFINED) {
        send_to_char(ch, "You don't have '%s' prepared or you lack the spell slots to cast it.\r\n", 
                     spell_info[spell_nums[i]].name);
        return;
      } else {
        send_to_char(ch, "--%s is available\r\n", spell_info[spell_nums[i]].name);
      }
    }
  }
  
  /* Check if character has required materials (but don't consume yet) */
  for (i = 0; i < NUM_CRAFT_MOTES; i++) {
    if (total_motes_by_type[i] > 0 && GET_CRAFT_MOTES(ch, i) < total_motes_by_type[i]) {
      send_to_char(ch, "You need %d %s%s but only have %d.\r\n", 
                   total_motes_by_type[i], crafting_motes[i], 
                   total_motes_by_type[i] > 1 ? "s" : "",
                   GET_CRAFT_MOTES(ch, i));
      return;
    }
  }
  
  if (GET_GOLD(ch) < total_gold) {
    send_to_char(ch, "You need %d gold coins but only have %d.\r\n", total_gold, GET_GOLD(ch));
    return;
  }
  
  /* Calculate alchemy skill check DC (for later use in event) */
  dc = 10 + (highest_circle * 2) + (num_spells - 1) * 3;  /* +3 DC per additional spell */
  brewing_skill = get_craft_skill_value(ch, craft_skill_to_ability(CRAFT_SKILL_BREWING));
  
  /* Materials are available - start alchemy process without consuming them yet */
  send_to_char(ch, "You have the required materials. Starting alchemy process...\r\n");
  send_to_char(ch, "Required: %d total gold", total_gold);
  for (i = 0; i < NUM_CRAFT_MOTES; i++) {
    if (total_motes_by_type[i] > 0) {
      send_to_char(ch, ", %d %s%s", total_motes_by_type[i], crafting_motes[i], 
                   total_motes_by_type[i] > 1 ? "s" : "");
    }
  }
  send_to_char(ch, "\r\n");
  send_to_char(ch, "Alchemy skill check will be: skill(%d) + d20 vs DC %d\r\n", brewing_skill, dc);
  
  /* Calculate brewing time (5 seconds per spell level, +50% for multiple spells) */
  brew_time = 0;
  for (i = 0; i < num_spells; i++) {
    brew_time += spell_levels[i] * 5;
  }
  if (num_spells > 1) {
    brew_time = (int)(brew_time * 1.5);  /* 50% longer for multi-spell potions */
  }
  
  /* Start brewing process */
  if (num_spells == 1) {
    send_to_char(ch, "You begin brewing a potion of %s. This will take %d seconds.\r\n", 
                 spell_info[spell_nums[0]].name, brew_time);
  } else {
    send_to_char(ch, "You begin brewing a complex %d-spell potion. This will take %d seconds.\r\n", 
                 num_spells, brew_time);
    send_to_char(ch, "Spells: ");
    for (i = 0; i < num_spells; i++) {
      send_to_char(ch, "%s%s", spell_info[spell_nums[i]].name, 
                   (i < num_spells - 1) ? ", " : "\r\n");
    }
  }
  
  send_to_char(ch, "Cost multiplier: %.1fx for %d spell%s\r\n", 
               cost_multiplier, num_spells, num_spells > 1 ? "s" : "");
  
  act("$n begins creating a magical potion.", TRUE, ch, 0, 0, TO_ROOM);
  
  /* Create the mud event with data string containing all spell info, costs, and DC */
  /* Format: spell1,spell2,spell3,num_spells,highest_circle,brewing_skill,dc,mote_type0,mote_type1,...,total_gold,cost_multiplier */
  char mote_data[512] = {0};
  char temp[32];
  for (i = 0; i < NUM_CRAFT_MOTES; i++) {
    snprintf(temp, sizeof(temp), "%d,", total_motes_by_type[i]);
    strncat(mote_data, temp, sizeof(mote_data) - strlen(mote_data) - 1);
  }
  
  snprintf(data_string, sizeof(data_string), "%d,%d,%d,%d,%d,%d,%d,%s%d,%.2f", 
           spell_nums[0], spell_nums[1], spell_nums[2], 
           num_spells, highest_circle, brewing_skill, dc,
           mote_data, total_gold, cost_multiplier);
  
  pMudEvent = new_mud_event(eBREWING, (void *)ch, data_string);
  if (pMudEvent == NULL) {
    send_to_char(ch, "Error: Could not create alchemy event.\r\n");
    return;
  }
  
  /* Attach the event with proper timing */
  attach_mud_event(pMudEvent, brew_time * PASSES_PER_SEC);
  
  /* Save character */
  save_char(ch, 0);
}
/*
 * do_brew - Enhanced Potion brewing system for LuminariMUD
 * 
 * This function allows characters to brew potions using:
 * - Elemental motes (based on spell school)
 * - Gold (based on spell level) 
 * - Must have spell prepared or be able to cast it
 * - Alchemists can brew any spell available to any class up to their level/3
 * - Support for multiple spells (up to 3) on one potion with scaling costs
 * - Alchemy skill checks with experience gain
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "class.h"
#include "spell_prep.h"
#include "crafting_new.h"
#include "domains_schools.h"
#include "helpers.h"

/* External function declarations */
extern int find_skill_num(char *name);
extern int spell_school(int spellnum);
extern void save_char(struct char_data *ch, int load_room);