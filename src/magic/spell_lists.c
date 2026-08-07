/**************************************************************************
 *  File: magic/spell_lists.c                         Part of LuminariMUD *
 *  Usage: Spell sorting and player-facing spell lists.                    *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/


#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "magic/spell_lists.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "clan.h"
#include "mudlim.h"
#include "graph.h"
#include "dgscript/dg_scripts.h"
#include "mud_event.h"
#include "actions.h"
#include "combat/assign_wpn_armor.h"
#include "magic/domains_schools.h"
#include "character/feats.h"
#include "magic/spell_prep.h"
#include "obj/item.h"
#include "craft/alchemy.h"
#include "obj/treasure.h"
#include "mob/mob_utils.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "quest/quest.h"
#include "character/backgrounds.h"
#include "character/perks.h"

static int compare_spells(const void *x, const void *y);

int spell_sort_info[TOP_SKILL_DEFINE];


static int compare_spells(const void *x, const void *y)
{
  int a = *(const int *)x, b = *(const int *)y;

  if (a < 1 || b < 1)
    return 0;

  if (a >= TOP_SPELLS_POWERS_SKILLS_BOMBS || b >= TOP_SPELLS_POWERS_SKILLS_BOMBS)
    return 0;

  /* Handle null or empty spell names */
  if (!spell_info[a].name || !spell_info[b].name)
  {
    if (!spell_info[a].name && !spell_info[b].name)
      return 0;
    if (!spell_info[a].name)
      return 1; /* Move empty entries to the end */
    return -1;
  }

  return strcmp(spell_info[a].name, spell_info[b].name);
}

/* this will create a full list, added two more lists
   to separate the skills/spells */
void sort_spells(void)
{
  int a;

  /* full list */

  /* initialize array, avoiding reserved. */
  for (a = 0; a <= TOP_SPELLS_POWERS_SKILLS_BOMBS; a++)
  {
    spell_sort_info[a] = a;
  }

  qsort(&spell_sort_info[0], TOP_SPELLS_POWERS_SKILLS_BOMBS, sizeof(int), compare_spells);
}


// returns true if you have all the requisites for the skill
// false if you don't

void list_spells(struct char_data *ch, int mode, int class, int circle)
{
  int i = 0, slot = 0, sinfo = 0;
  int bottom = 0, top = 0;
  size_t len = 0;
  int nlen = 0;
  char buf2[MAX_STRING_LENGTH] = {'\0'};
  char cname[100];
  const char *overflow = "\r\n**OVERFLOW**\r\n";

  if (!ch)
    return;

  int domain_1 = GET_1ST_DOMAIN(ch);
  int domain_2 = GET_2ND_DOMAIN(ch);
  bool is_psionic = (class == CLASS_PSIONICIST);
  bool is_warlock = (class == CLASS_WARLOCK);

  // default class case
  if (class == -1)
  {
    class = GET_CLASS(ch);
    if (!CLASS_LEVEL(ch, class))
      send_to_char(ch, "You don't have any levels in your current class.\r\n");
  }

  snprintf(cname, sizeof(cname), "%s", class_list[class].name);

  if (mode == 0)
  {
    len = snprintf(buf2, sizeof(buf2), "\tCKnown %s %s List\tn\r\n%s", CAP(cname),
                   (is_psionic || is_warlock) ? "Power" : "Spell",
                   (is_psionic && CLASS_LEVEL(ch, CLASS_PSIONICIST) == 1)
                       ? "\tYNOTE:\tnThere is a known bug where new psionicists will show all "
                         "powers instead of\r\n"
                         "only the ones they know. To correct this, please quit, then press '0' to "
                         "return to\r\n"
                         "the account menu, and login again.\r\n"
                       : "");

    for (slot = get_class_highest_circle(ch, class); slot >= 0; slot--)
    {
      if ((circle != -1) && circle != slot)
        continue;

      char header_buf[80];
      if (slot == 0)
        snprintf(header_buf, sizeof(header_buf), "\r\n\tCCantrips\tn\r\n");
      else if (is_psionic || is_warlock)
        snprintf(header_buf, sizeof(header_buf), "\r\n\tCPower Circle Level %d\tn\r\n", slot);
      else
        snprintf(header_buf, sizeof(header_buf), "\r\n\tCSpell Circle Level %d\tn\r\n", slot);

      bool header_added = FALSE;
      int col = 0;

      bottom = 1;
      top = TOP_SPELLS_POWERS_SKILLS_BOMBS;
      for (; bottom < top; bottom++)
      {
        i = spell_sort_info[bottom];
        if (do_not_list_spell(i))
          continue;
        sinfo = spell_info[i].min_level[class];

        bool auto_cantrip_known = spell_is_cantrip(i) && sinfo == 0 && CLASS_LEVEL(ch, class) > 0;

        if (class == CLASS_SORCERER &&
            (is_a_known_spell(ch, CLASS_SORCERER, i) || (auto_cantrip_known && slot == 0)) &&
            compute_spells_circle(ch, CLASS_SORCERER, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_BARD &&
                 (is_a_known_spell(ch, CLASS_BARD, i) || (auto_cantrip_known && slot == 0)) &&
                 compute_spells_circle(ch, CLASS_BARD, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_SUMMONER &&
                 (is_a_known_spell(ch, CLASS_SUMMONER, i) || (auto_cantrip_known && slot == 0)) &&
                 compute_spells_circle(ch, CLASS_SUMMONER, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_INQUISITOR &&
                 (is_a_known_spell(ch, CLASS_INQUISITOR, i) || (auto_cantrip_known && slot == 0)) &&
                 compute_spells_circle(ch, CLASS_INQUISITOR, i, 0, GET_1ST_DOMAIN(ch)) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_WARLOCK &&
                 (is_a_known_spell(ch, CLASS_WARLOCK, i) || (auto_cantrip_known && slot == 0)) &&
                 warlock_spell_type(i) == WARLOCK_POWER_SPELL &&
                 compute_spells_circle(ch, CLASS_WARLOCK, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_PSIONICIST &&
                 (is_a_known_spell(ch, CLASS_PSIONICIST, i) || (auto_cantrip_known && slot == 0)) &&
                 compute_spells_circle(ch, CLASS_PSIONICIST, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class == CLASS_WIZARD && spellbook_ok(ch, i, class, FALSE) &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >= sinfo &&
                 compute_spells_circle(ch, class, i, 0, DOMAIN_UNDEFINED) == slot &&
                 ((slot == 0 && sinfo == 0) || GET_SKILL(ch, i)))
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_WIZARD &&
                 class != CLASS_INQUISITOR && class != CLASS_PSIONICIST && class != CLASS_WARLOCK &&
                 class != CLASS_SUMMONER &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >=
                     MIN_SPELL_LVL(i, class, domain_1) &&
                 compute_spells_circle(ch, class, i, 0, domain_1) == slot &&
                 ((slot == 0 && sinfo == 0) || GET_SKILL(ch, i)))
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
        else if (class != CLASS_SORCERER && class != CLASS_BARD && class != CLASS_WIZARD &&
                 class != CLASS_INQUISITOR && class != CLASS_PSIONICIST && class != CLASS_WARLOCK &&
                 class != CLASS_SUMMONER &&
                 (BONUS_CASTER_LEVEL(ch, class) + CLASS_LEVEL(ch, class)) >=
                     MIN_SPELL_LVL(i, class, domain_2) &&
                 compute_spells_circle(ch, class, i, 0, domain_2) == slot &&
                 ((slot == 0 && sinfo == 0) || GET_SKILL(ch, i)))
        {
          if (!header_added)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%s", header_buf);
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            header_added = TRUE;
            col = 0;
          }
          if (CONFIG_SPELLCASTING_TIME_MODE == 0)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s  ", spell_info[i].name);
          }
          else
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-20s %2dbst  ", spell_info[i].name,
                            spell_info[i].time);
          }
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          {
            break;
          }
          len += nlen;
          col++;
          if (col == 3)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            {
              break;
            }
            len += nlen;
            col = 0;
          }
        }
      }

      if (header_added && col != 0)
      {
        nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
        if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
        {
          break;
        }
        len += nlen;
      }
    }
  }
  else
  {
    len = snprintf(buf2, sizeof(buf2), "\tCFull %s List\tn\r\n", is_psionic ? "Power" : "Spell");

    if (class == CLASS_PALADIN || class == CLASS_RANGER)
      slot = 4;
    if (class == CLASS_ALCHEMIST)
      slot = 6;
    else
      slot = 9;

    for (; slot > 0; slot--)
    {
      if ((circle != -1) && circle != slot)
        continue;
      nlen =
          snprintf(buf2 + len, sizeof(buf2) - len, "\r\n\tC%s Circle Level %d\tn\r\n",
                   is_psionic ? "Power" : (class == CLASS_ALCHEMIST ? "Extract" : "Spell"), slot);
      if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
        break;
      len += nlen;

      int col = 0;
      bottom = 1;
      top = TOP_SPELLS_POWERS_SKILLS_BOMBS;
      for (; bottom < top; bottom++)
      {
        i = spell_sort_info[bottom];
        sinfo = spell_info[i].min_level[class];
        if (do_not_list_spell(i))
          continue;

        /* SPELL PREPARATION HOOK (spellCircle) */
        if (compute_spells_circle(ch, class, i, 0, DOMAIN_UNDEFINED) == slot)
        {
          nlen = snprintf(buf2 + len, sizeof(buf2) - len, "%-30s %-15s  ", spell_info[i].name,
                          school_names_specific[spell_info[i].schoolOfMagic]);
          if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
            break;
          len += nlen;
          col++;
          if (col == 2)
          {
            nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
            if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
              break;
            len += nlen;
            col = 0;
          }
        }
      }
      if (col != 0)
      {
        nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\n");
        if (nlen < 0 || (size_t)nlen >= sizeof(buf2) - len)
          break;
        len += nlen;
      }
    }
  }
  if (len >= sizeof(buf2))
    strcpy(buf2 + sizeof(buf2) - strlen(overflow) - 1, overflow); /* strcpy: OK */

  /* Append acronym legend for bst only in seconds-based mode */
  if (CONFIG_SPELLCASTING_TIME_MODE != 0)
  {
    nlen = snprintf(buf2 + len, sizeof(buf2) - len, "\r\nbst: base spellcasting time\r\n");
    if (len + nlen < sizeof(buf2) && nlen > 0)
      len += nlen;
  }

  page_string(ch->desc, buf2, TRUE);
}
