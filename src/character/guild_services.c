/**************************************************************************
 *  File: character/guild_services.c                   Part of LuminariMUD *
 *  Usage: Class guild training and entrance services.                     *
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
#include "guild_services.h"
#include "character/abilities.h"
#include "character/skill_lists.h"
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

SPECIAL(guild)
{
  int skill_num, percent;
  char arg[MAX_STRING_LENGTH] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};
  char *ability_name = NULL;

  if (IS_NPC(ch) || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return (FALSE);

  skip_spaces(&argument);

  // Practice code
  if (CMD_IS("practice"))
  {
    list_crafting_skills(ch);
    return (TRUE);

    /***************************************/
    /* everything below this is deprecated */
    /***************************************/

    if (!*argument)
    {
      list_skills(ch);
      return (TRUE);
    }
    if (GET_PRACTICES(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to practice now.\r\n");
      return (TRUE);
    }

    skill_num = find_skill_num(argument);

    if (skill_num < 1 || GET_LEVEL(ch) < spell_info[skill_num].min_level[(int)GET_CLASS(ch)])
    {
      send_to_char(ch, "You do not know of that skill.\r\n");
      return (TRUE);
    }

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
      send_to_char(ch, "You are already learned in that area.\r\n");
      return (TRUE);
    }
     */

    if (skill_num > SPELL_RESERVED_DBC && skill_num < MAX_SPELLS)
    {
      send_to_char(ch, "You can't practice spells.\r\n");
      return (TRUE);
    }

    if (!meet_skill_reqs(ch, skill_num))
    {
      send_to_char(ch, "You haven't met the pre-requisites for that skill.\r\n");
      return (TRUE);
    }

    /* added with addition of crafting system so you can't use your
     'practice points' for training your crafting skills which have
     a much lower base value than 75 */

    if (GET_SKILL(ch, skill_num))
    {
      send_to_char(ch, "You already have this skill trained.\r\n");
      return TRUE;
    }

    send_to_char(ch, "You practice '%s' with your trainer...\r\n", spell_info[skill_num].name);
    GET_PRACTICES(ch)
    --;

    percent = GET_SKILL(ch, skill_num);
    percent += int_app[GET_INT(ch)].learn;

    SET_SKILL(ch, skill_num, percent);

    /*
    if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
      send_to_char(ch, "You are now \tGlearned\tn in '%s.'\r\n",
            spell_info[skill_num].name);
     */

    // for further expansion - zusuk
    process_skill(ch, skill_num);

    return (TRUE);
  }
  else if (CMD_IS("train"))
  {
    // training code

    if (!*argument)
    {
      list_abilities(ch, ABILITY_TYPE_GENERAL);
      return (TRUE);
    }

    if (GET_TRAINS(ch) <= 0)
    {
      send_to_char(ch, "You do not seem to be able to train now.\r\n");
      return (TRUE);
    }

    /* Parse argument and check for 'knowledge' or 'craft' as a first arg- */
    ability_name = one_argument_u(argument, arg);
    skip_spaces(&ability_name);

    if (is_abbrev(arg, "craft"))
    {
      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_CRAFT);
        return (TRUE);
      }
      /* Crafting skill */
      snprintf(buf, sizeof(buf), "Craft (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else if (is_abbrev(arg, "knowledge"))
    {
      if (!strcmp(ability_name, ""))
      {
        list_abilities(ch, ABILITY_TYPE_KNOWLEDGE);
        return (TRUE);
      }
      /* Knowledge skill */
      snprintf(buf, sizeof(buf), "Knowledge (%s", ability_name);
      skill_num = find_ability_num(buf);
    }
    else
    {
      skill_num = find_ability_num(argument);
    }

    if (skill_num < 1)
    {
      send_to_char(ch, "You do not know of that ability.\r\n");
      return (TRUE);
    }

    // ability not available to this class
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 0)
    {
      send_to_char(ch, "This ability is not available to your class...\r\n");
      return (TRUE);
    }

    // cross-class ability
    if (GET_TRAINS(ch) < 2 && modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(
          ch, "(Cross-Class) You don't have enough training sessions to train that ability...\r\n");
      return (TRUE);
    }
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    // class ability
    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 2)
    {
      send_to_char(ch, "You are already trained in that area.\r\n");
      return (TRUE);
    }

    send_to_char(ch, "You train for a while...\r\n");
    GET_TRAINS(ch)
    --;
    if (modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
    {
      GET_TRAINS(ch)
      --;
      send_to_char(ch, "You used two training sessions to train a cross-class ability...\r\n");
    }
    GET_ABILITY(ch, skill_num)
    ++;

    if (GET_ABILITY(ch, skill_num) >= (GET_LEVEL(ch) + 3))
      send_to_char(ch, "You are now trained in that area.\r\n");
    if (GET_ABILITY(ch, skill_num) >= ((int)((GET_LEVEL(ch) + 3) / 2)) &&
        modify_class_ability(ch, skill_num, GET_CLASS(ch)) == 1)
      send_to_char(ch, "You are already trained in that area.\r\n");

    return (TRUE);
  }
  else if (CMD_IS("boosts"))
  {
    if (!argument || !*argument)
      send_to_char(ch,
                   "\tCStat boost sessions remaining: %d\tn\r\n"
                   "\tcStats:\tn\r\n"
                   "Strength\r\n"
                   "Constitution\r\n"
                   "Dexterity\r\n"
                   "Intelligence\r\n"
                   "Wisdom\r\n"
                   "Charisma\r\n"
                   "\r\n",
                   GET_BOOSTS(ch));
    else if (!GET_BOOSTS(ch))
      send_to_char(ch, "You have no ability training sessions.\r\n");
    else if (!strncasecmp("strength", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour strength increases!\tn\r\n");
      GET_REAL_STR(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("constitution", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour constitution increases!\tn\r\n");
      GET_REAL_CON(ch) += 1;
      /* Give them retroactive hit points for constitution */
      if (!(GET_REAL_CON(ch) % 2))
      {
        GET_REAL_MAX_HIT(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYou gain %d hitpoints!\tn\r\n", GET_LEVEL(ch));
      }
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("dexterity", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour dexterity increases!\tn\r\n");
      GET_REAL_DEX(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("intelligence", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "%s", "\tMYour intelligence increases!\tn\r\n");
      GET_REAL_INT(ch) += 1;
      GET_BOOSTS(ch) -= 1;
      /* Give them retroactive trains */
      if (!(GET_REAL_INT(ch) % 2))
      {
        GET_TRAINS(ch) += GET_LEVEL(ch);
        send_to_char(ch, "\tMYour intelligence increases!\tn\r\n");
        send_to_char(ch, "\tMYou gain %d trains!\tn\r\n", GET_LEVEL(ch));
      }
    }
    else if (!strncasecmp("wisdom", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "\tMYour wisdom increases!\tn\r\n");
      GET_REAL_WIS(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else if (!strncasecmp("charisma", argument, strlen(argument)))
    {
      send_to_char(ch, "%s", CONFIG_OK);
      send_to_char(ch, "\tMYour charisma increases!\tn\r\n");
      GET_REAL_CHA(ch) += 1;
      GET_BOOSTS(ch) -= 1;
    }
    else
      send_to_char(ch,
                   "\tCStat boost sessions remaining: %d\tn\r\n"
                   "\tcStats:\tn\r\n"
                   "Strength\r\n"
                   "Constitution\r\n"
                   "Dexterity\r\n"
                   "Intelligence\r\n"
                   "Wisdom\r\n"
                   "Charisma\r\n"
                   "\r\n",
                   GET_BOOSTS(ch));
    affect_total(ch);
    send_to_char(ch, "\tDType 'feats' to see your feats\tn\r\n");
    send_to_char(ch, "\tDType 'train' to see your abilities\tn\r\n");
    send_to_char(ch, "\tDType 'boost' to adjust your stats\tn\r\n");
    send_to_char(ch, "\tDType 'craft' to see your crafting proficiency\tn\r\n");
    send_to_char(ch, "\tDType 'spells <classname>' to see your currently known spells\tn\r\n");
    return (TRUE);
  }

  // should not be able to get here
  log("Reached the unreachable in SPECIAL(guild) in character/guild_services.c");
  return (FALSE);
}

SPECIAL(guild_guard)
{
  int i, direction;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return (FALSE);

  /* find out what direction they are trying to go */
  for (direction = 0; direction < NUM_OF_DIRS; direction++)
    if (!strcmp(cmd_info[cmd].command, dirs[direction]))
      for (direction = 0; direction < DIR_COUNT; direction++)
        if (!strcmp(cmd_info[cmd].command, dirs[direction]) ||
            !strcmp(cmd_info[cmd].command, autoexits[direction]))
          break;

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++)
  {
    /* Wrong guild. */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room)
      continue;

    /* Wrong direction. */
    if (direction != guild_info[i].direction)
      continue;

    /* Allow the people of the guild through. */
    /* Can't use GET_CLASS anymore, need CLASS_LEVEL(ch, i)!!  - 04/08/2013 Ornir */
    if (!IS_NPC(ch) && (CLASS_LEVEL(ch, guild_info[i].pc_class) > 0))
      continue;

    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return (TRUE);
  }
  return (FALSE);
}
