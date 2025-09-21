/*
 * metamagic_science.c
 * Implementation of Metamagic Science feats for LuminariMUD
 * 
 * FEAT_METAMAGIC_SCIENCE allows applying metamagic to spell trigger items (wands/staves)
 * FEAT_IMPROVED_METAMAGIC_SCIENCE allows applying metamagic to spell completion items (scrolls/potions)
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
#include "fight.h"
#include "spell_prep.h"
#include "metamagic_science.h"

/* Parse metamagic arguments for consumable items
 * Returns parsed metamagic flags, or -1 on error */
int parse_metamagic_for_consumables(struct char_data *ch, char **argument, int spell_num, int item_type)
{
  char *temp_arg = NULL;
  char arg_copy[MAX_INPUT_LENGTH];
  int metamagic = 0;
  bool has_metamagic_science = FALSE;
  bool has_improved_metamagic_science = FALSE;
  
  if (!ch || !argument || !*argument)
    return 0;
    
  /* Check if player has the required feats */
  has_metamagic_science = HAS_FEAT(ch, FEAT_METAMAGIC_SCIENCE);
  has_improved_metamagic_science = HAS_FEAT(ch, FEAT_IMPROVED_METAMAGIC_SCIENCE);
  
  /* Only allow metamagic if they have the appropriate feat */
  if (item_type == ITEM_WAND || item_type == ITEM_STAFF) {
    if (!has_metamagic_science) {
      return 0; /* No metamagic allowed without the feat */
    }
  } else if (item_type == ITEM_SCROLL || item_type == ITEM_POTION) {
    if (!has_improved_metamagic_science) {
      return 0; /* No metamagic allowed without the feat */
    }
  } else {
    return 0; /* Unknown item type */
  }
  
  /* Make a copy of the argument to parse */
  strncpy(arg_copy, *argument, sizeof(arg_copy) - 1);
  arg_copy[sizeof(arg_copy) - 1] = '\0';
  
  /* Parse metamagic arguments before the spell name */
  temp_arg = strtok(arg_copy, " ");
  
  while (temp_arg && temp_arg[0] != '\'') {
    /* Check if this looks like a metamagic argument */
    if (is_abbrev(temp_arg, "quickened")) {
      /* Quicken cannot be used with consumables since they already use swift action */
      send_to_char(ch, "Consumable items already use a swift action and cannot be quickened!\r\n");
      return -1;
    }
    else if (is_abbrev(temp_arg, "maximized")) {
      if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL)) {
        SET_BIT(metamagic, METAMAGIC_MAXIMIZE);
      } else {
        send_to_char(ch, "You don't know how to maximize spells!\r\n");
        return -1;
      }
    }
    else if (is_abbrev(temp_arg, "empowered")) {
      if (HAS_FEAT(ch, FEAT_EMPOWER_SPELL)) {
        if (spell_num > 0 && !can_spell_be_empowered(spell_num)) {
          send_to_char(ch, "This spell cannot be empowered.\r\n");
          return -1;
        }
        SET_BIT(metamagic, METAMAGIC_EMPOWER);
      } else {
        send_to_char(ch, "You don't know how to empower spells!\r\n");
        return -1;
      }
    }
    else if (is_abbrev(temp_arg, "extended")) {
      if (HAS_FEAT(ch, FEAT_EXTEND_SPELL)) {
        if (spell_num > 0 && !can_spell_be_extended(spell_num)) {
          send_to_char(ch, "This spell cannot be extended.\r\n");
          return -1;
        }
        SET_BIT(metamagic, METAMAGIC_EXTEND);
      } else {
        send_to_char(ch, "You don't know how to extend spells!\r\n");
        return -1;
      }
    }
    else if (is_abbrev(temp_arg, "silent")) {
      if (HAS_FEAT(ch, FEAT_SILENT_SPELL)) {
        SET_BIT(metamagic, METAMAGIC_SILENT);
      } else {
        send_to_char(ch, "You don't know how to cast spells silently!\r\n");
        return -1;
      }
    }
    else if (is_abbrev(temp_arg, "still")) {
      if (HAS_FEAT(ch, FEAT_STILL_SPELL)) {
        SET_BIT(metamagic, METAMAGIC_STILL);
      } else {
        send_to_char(ch, "You don't know how to cast spells while bound!\r\n");
        return -1;
      }
    }
    else {
      /* Not a metamagic argument, must be part of spell name or target */
      break;
    }
    
    temp_arg = strtok(NULL, " ");
  }
  
  /* Update the argument pointer to skip the metamagic parts we consumed */
  if (metamagic > 0) {
    char *search_pos = *argument;
    int metamagic_words = 0, i;
    
    /* Count how many words we consumed for metamagic */
    if (IS_SET(metamagic, METAMAGIC_MAXIMIZE)) metamagic_words++;
    if (IS_SET(metamagic, METAMAGIC_EMPOWER)) metamagic_words++;
    if (IS_SET(metamagic, METAMAGIC_EXTEND)) metamagic_words++;
    if (IS_SET(metamagic, METAMAGIC_SILENT)) metamagic_words++;
    if (IS_SET(metamagic, METAMAGIC_STILL)) metamagic_words++;
    
    /* Skip past the metamagic words */
    for (i = 0; i < metamagic_words && *search_pos; i++) {
      /* Skip current word */
      while (*search_pos && !isspace(*search_pos)) search_pos++;
      /* Skip spaces */
      while (*search_pos && isspace(*search_pos)) search_pos++;
    }
    
    *argument = search_pos;
  }
  
  return metamagic;
}

/* Calculate additional charges needed for metamagic on wands/staves */
int calculate_metamagic_charge_cost(int metamagic, int base_spell_level)
{
  int additional_charges = 0;
  
  /* Base consumable usage is always 1 charge, metamagic adds to this */
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE)) {
    additional_charges += 3;  /* Maximize adds 3 charges */
  }
  if (IS_SET(metamagic, METAMAGIC_EMPOWER)) {
    additional_charges += 2;  /* Empower adds 2 charges */
  }
  if (IS_SET(metamagic, METAMAGIC_EXTEND)) {
    additional_charges += 1;  /* Extend adds 1 charge */
  }
  if (IS_SET(metamagic, METAMAGIC_SILENT)) {
    additional_charges += 1;  /* Silent adds 1 charge */
  }
  if (IS_SET(metamagic, METAMAGIC_STILL)) {
    additional_charges += 1;  /* Still adds 1 charge */
  }
  /* Note: Quicken is not allowed for consumables since they already use swift action */
  
  return additional_charges;
}

/* Calculate Use Magic Device DC for metamagic scrolls and potions */
int calculate_metamagic_scroll_dc(int base_spell_level, int metamagic)
{
  int modified_level = base_spell_level + calculate_metamagic_charge_cost(metamagic, base_spell_level);
  return 20 + (3 * modified_level);
}

/* Get metamagic description string for display */
void get_metamagic_description(int metamagic, char *buf, size_t buf_size)
{
  char temp[MAX_INPUT_LENGTH];
  bool first = TRUE;
  
  buf[0] = '\0';
  
  if (IS_SET(metamagic, METAMAGIC_QUICKEN)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "quickened");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
  if (IS_SET(metamagic, METAMAGIC_MAXIMIZE)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "maximized");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
  if (IS_SET(metamagic, METAMAGIC_EMPOWER)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "empowered");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
  if (IS_SET(metamagic, METAMAGIC_EXTEND)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "extended");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
  if (IS_SET(metamagic, METAMAGIC_SILENT)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "silent");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
  if (IS_SET(metamagic, METAMAGIC_STILL)) {
    snprintf(temp, sizeof(temp), "%s%s", first ? "" : ", ", "still");
    strncat(buf, temp, buf_size - strlen(buf) - 1);
    first = FALSE;
  }
}
