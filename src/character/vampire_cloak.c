/**************************************************************************
 *  File: character/vampire_cloak.c                    Part of LuminariMUD *
 *  Usage: Vampire cloak customization special procedure.                  *
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
#include "spec_procs.h"
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
#include "character/vampire_cloak.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"

SPECIAL(vampire_cloak)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;

  log("SYSERR: Typed Vampire Cloak adapter invoked outside a special-procedure gateway.");
  return FALSE;
}

int vampire_cloak_typed(struct spec_event_context *context)
{
  struct char_data *ch;
  struct obj_data *obj;
  const char *argument;
  char arg[200];
  char desc[255];
  char long_description[255];
  char old_description[255];
  int choice;
  int cmd;
  int count;
  int i;
  int result;

  ch = context->actor;
  obj = (struct obj_data *)context->owner;
  argument = context->argument;
  cmd = context->command;
  choice = 0;
  count = 0;
  result = 0;

  if (context->event == SPEC_EVENT_ITEM_IDENTIFY)
  {
    send_to_char(ch, "This vampire cloak can be customized using the 'setcloak' command.\r\n");
    send_to_char(ch, "Type 'setcloak' while wearing the cloak to see available options.\r\n");
    return TRUE;
  }
  if (context->event != SPEC_EVENT_COMMAND)
    return FALSE;

  if (!CMD_IS("setcloak"))
    return FALSE;

  if (IS_NPC(ch))
    return FALSE;

  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID ||
      obj->worn_on != WEAR_ABOUT || GET_OBJ_VNUM(obj) != VAMPIRE_CLOAK_OBJ_VNUM)
  {
    send_to_char(ch, "You must be wearing your vampire cloak to set its abilities.\r\n");
    return 1;
  }

  if (!IS_VAMPIRE(ch))
  {
    send_to_char(ch, "Only vampires can benefit from a vampire cloak.\r\n");
    return 1;
  }

  if (!*argument)
  {
    send_to_char(ch, "You must specify one of the following:\r\n");
    for (i = 0; i < NUM_APPLIES; i++)
    {
      if (valid_vampire_cloak_apply(i))
      {
        count++;
        if ((count % 4) == 3 || (count % 4) == 0)
          send_to_char(ch, "\tC");
        else
          send_to_char(ch, "\tc");
        send_to_char(ch, "%2d) \tn+%d to %-16s %s", count,
                     get_vampire_cloak_bonus(GET_LEVEL(ch), i), apply_types_lowercase(i),
                     get_vampire_cloak_bonus(GET_LEVEL(ch), i) >= 10 ? "" : " ");
        if ((count % 2) == 0)
          send_to_char(ch, "\r\n");
      }
    }
    if ((count % 2) == 1)
      send_to_char(ch, "\r\n");

    send_to_char(ch, "\r\n");
    send_to_char(ch, "Please enter 'setcloak (number)' where number is the number of the bonus "
                     "type you'd like above.\r\n");
    send_to_char(ch, "\r\n");
    send_to_char(
        ch, "Or, you can type 'setcloak description (new description)' to restring the cloak.\r\n");
    send_to_char(ch, "\r\n");
    return 1;
  }

  half_chop_c(argument, arg, sizeof(arg), desc, sizeof(desc));

  if (is_abbrev(arg, "description"))
  {
    if (!*desc)
    {
      send_to_char(ch, "You need to provide a new description for the cloak.  Use: setcloak "
                       "description (new description)\r\n");
      return 1;
    }

    if (strlen(desc) > 80)
    {
      send_to_char(ch, "That description is too long.\r\n");
    }

    snprintf(old_description, sizeof(old_description), "%s", obj->short_description);
    parse_at(desc);
    obj->short_description = strdup(desc);
    send_to_char(ch, "You have renamed '%s' to '%s'.\r\n", old_description, desc);
    strip_colors(desc);
    obj->name = strdup(desc);
    snprintf(long_description, sizeof(long_description), "%s is here.", CAP(desc));
    obj->description = strdup(long_description);
    return 1;
  }

  choice = atoi(argument);

  if (choice <= APPLY_NONE || choice >= NUM_APPLIES)
  {
    send_to_char(ch, "That is not a valid bonus type.  Please type 'setcloak' by itself to see a "
                     "list of options.\r\n");
    return 1;
  }

  for (i = 0; i < NUM_APPLIES; i++)
  {
    if (valid_vampire_cloak_apply(i))
    {
      count++;
      if (choice == count)
      {
        result = i;
        break;
      }
    }
  }

  if (!valid_vampire_cloak_apply(result))
  {
    send_to_char(ch, "That is not a valid bonus type.  Please type 'setcloak' by itself to see a "
                     "list of options.\r\n");
    return 1;
  }

  if (GET_SETCLOAK_TIMER(ch) > 0)
  {
    send_to_char(ch,
                 "You still have %d rounds to wait before you can change or set your vampire cloak "
                 "bonuses.\r\n",
                 GET_SETCLOAK_TIMER(ch));
    return 1;
  }

  /* Clear existing bonuses. */
  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
  {
    obj->affected[i].location = 0;
    obj->affected[i].modifier = 0;
    obj->affected[i].bonus_type = 0;
    obj->affected[i].specific = 0;
  }

  /* Add the selected bonus. */
  obj->affected[0].location = result;
  obj->affected[0].modifier = get_vampire_cloak_bonus(GET_LEVEL(ch), result);
  obj->affected[0].bonus_type = BONUS_TYPE_RACIAL;

  send_to_char(ch, "\tcYour vampire cloak now offers +%d to your %s!\r\n\tn",
               obj->affected[0].modifier, apply_types_lowercase(result));

  /* This intentionally produces only levels 0, 15, and 30. */
  GET_OBJ_LEVEL(obj) = (GET_LEVEL(ch) / 15) * 15;

  /* Make the customized cloak vampire-only. */
  REMOVE_OBJ_FLAG(obj, ITEM_ANTI_VAMPIRE);
  SET_OBJ_FLAG(obj, ITEM_VAMPIRE_ONLY);

  GET_SETCLOAK_TIMER(ch) = 100;

  return 1;
}
