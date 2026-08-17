/**************************************************************************
 *  File: quest/quest_services.c                       Part of LuminariMUD *
 *  Usage: Quest reward service special procedures.                        *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "magic/spells.h"
#include "constants.h"
#include "act.h"
#include "quest_services.h"
#include "character/class.h"
#include "combat/fight.h"
#include "modify.h"
#include "obj/house.h"
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

SPECIAL(replace_quest_item)
{
  if (!CMD_IS("replace"))
    return 0;

  char arg1[200];
  struct obj_data *obj = NULL;
  int i = 0, counter = 0;
  qst_rnum rnum = NOTHING;

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "Quests that you have completed with replaceable rewards:\r\n"
                     "VNum  Description                                          Item Reward\r\n"
                     "----- ---------------------------------------------------- -----------\r\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    {
      if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
      {
        if (IS_SET(QST_FLAGS(rnum), AQ_REPLACE_OBJ_REWARD) && QST_OBJ(rnum) > 0 &&
            QST_OBJ(rnum) < 65535)
        {
          obj = read_object_reason(QST_OBJ(rnum), VIRTUAL, PERF_ENTITY_QUEST);
          if (obj)
          {
            send_to_char(ch, "\tg%5d\tn) \tc%-52.52s\tn \ty%s\tn\r\n", QST_NUM(rnum),
                         QST_DESC(rnum), obj->short_description);
          }
        }
      }
    }
    if (!counter)
    {
      send_to_char(ch, "You don't have any replaceable quest rewards.\r\n");
    }
    else
    {
      send_to_char(
          ch, "Type 'replace (quest vnum) to replace the quest item for the specified quest.\r\n");
    }
  }
  else
  {
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
    {
      if ((rnum = real_quest(ch->player_specials->saved.completed_quests[i])) != NOTHING)
      {
        if (IS_SET(QST_FLAGS(rnum), AQ_REPLACE_OBJ_REWARD) && QST_OBJ(rnum) > 0 &&
            QST_OBJ(rnum) < 65535)
        {
          obj = read_object_reason(QST_OBJ(rnum), VIRTUAL, PERF_ENTITY_QUEST);
          if (obj)
          {
            if (atoidx(arg1) == QST_NUM(rnum))
            {
              counter++;
              obj_to_char(obj, ch);
              send_to_char(ch, "You have had your quest item '%s' replaced.\r\n",
                           obj->short_description);
              return 1;
            }
          }
        }
      }
    }
    if (!counter)
    {
      send_to_char(ch, "There either is no replaceable quest object for that quest vnum, or you "
                       "haven't completed that quest yet.\r\n");
    }
  }
  return 1;
}
