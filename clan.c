/****************************************************************************
 *  Realms of Luminari
 *  File:     clan.c
 *  Usage:    most of the clan code, the editing/save/load are in clan_edit
 *  Header:   clan.h
 *  Authors:  Jamdog (ported to Luminari by Bakarus and Zusuk)
 ****************************************************************************/

#ifndef __CLAN_C__
#define __CLAN_C__

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"
#include "screen.h"
#include "improved-edit.h"
#include "spells.h" /* find skill, etc */
#include "clan.h"
#include "mudlim.h"

/* Global Variables used by clans */
struct clan_data *clan_list = NULL;
struct claim_data *claim_list = NULL;
int num_of_clans = 0;

/* Clan privileges.  Each of these is assigned a minimum rank */
const char * const clan_priv_names[] = {
    "Award",      /**< 'clan award' command    */
    "Claim",      /**< 'clan claim' command    */
    "Balance",    /**< 'clans bank balance     */
    "Demote",     /**< 'clan demote' command   */
    "Deposit",    /**< 'clan deposit' command  */
    "Edit",       /**< 'clan edit' command     */
    "Enrol",      /**< 'clan enrol' command    */
    "Expel",      /**< 'clan expel' command    */
    "Owner",      /**< 'clan owner' command    */
    "Promote",    /**< 'clan promote' command  */
    "Where",      /**< 'clan where' command    */
    "Withdraw",   /**< 'clan withdraw' command */
    "Set-Ally",   /**< Set clan Allied-With clan (in OLC)     */
    "Set-Appfee", /**< Set clan Application fee (in OLC)      */
    "Set-Applvl", /**< Set clan Application level (in OLC)    */
    "Set-Desc",   /**< Set clan Description (in OLC)          */
    "Set-Tax",    /**< Set clan Tax Rate (in OLC)             */
    "Set-Ranks",  /**< Set clan ranks and rank names (in OLC) */
    "Set-Title",  /**< Set clan title (name) (in OLC)         */
    "Set-War",    /**< Set clan At-War-With clan (in OLC)     */
    "Set-Privs",  /**< Set clan privilege ranks (in OLC)      */
    "\n"};

const struct clan_cmds clan_commands[] = {
    {"apply", CP_ALL, LVL_GRSTAFF, do_clanapply,
     "%sclan apply <clan>         %s- %sRequest to join a clan%s\r\n"},
    {"award", CP_AWARD, LVL_IMPL, do_clanaward,
     "%sclan award <player> <num> %s- %sAward CP to player (costs 10 "
     "coins per CP from clan bank)%s\r\n"},
    {"claim", CP_CLAIM, LVL_GRSTAFF, do_clanclaim,
     "%sclan claim                %s- %sTry to claim your current "
     "zone for your clan%s\r\n"},
    {"balance", CP_BALANCE, LVL_IMPL, do_clanbalance,
     "%sclan balance              %s- %sChecks the balance of the "
     "clan bank%s\r\n"},
    {"create", CP_NONE, LVL_IMPL, do_clancreate,
     "%sclan create <plr> <name>  %s- %sCreate a new clan, and set "
     "a leader \tw(IMP-Only!)%s\r\n"},
    {"demote", CP_DEMOTE, LVL_GRSTAFF, do_clandemote,
     "%sclan demote <player>      %s- %sReduce a clan member's clan "
     "rank%s\r\n"},
    {"deposit", CP_DEPOSIT, LVL_GRSTAFF, do_clandeposit,
     "%sclan deposit <amount>     %s- %sDeposit gold into clan "
     "bank%s\r\n"},
    {"destroy", CP_NONE, LVL_IMPL, do_clandestroy,
     "%sclan destroy <clan>       %s- %sRemove a clan from the clan list"
     " \tw(IMP-Only!)%s\r\n"},
    {"edit", CP_CLANEDIT, LVL_GRSTAFF, do_clanedit,
     "%sclan edit                 %s- %sEdit clan settings in "
     "an OLC%s\r\n"},
    {"enrol", CP_ENROL, LVL_GRSTAFF, do_clanenrol,
     "%sclan enrol [player]       %s- %sList requests or enrol a "
     "pending player%s\r\n"},
    {"expel", CP_EXPEL, LVL_GRSTAFF, do_clanexpel,
     "%sclan expel <player>       %s- %sKick a player out of the "
     "clan%s\r\n"},
    {"info", CP_ALL, LVL_IMMORT, do_claninfo,
     "%sclan info [clan]          %s- %sInfo about all clans, or a "
     "single clan%s\r\n"},
    {"list", CP_ALL, LVL_IMMORT, do_clanlist,
     "%sclan list                 %s- %sList all members of your clan "
     "(same as clan who)%s\r\n"},
    {"owner", CP_OWNER, LVL_GRSTAFF, do_clanowner,
     "%sclan owner <player>       %s- %sChange ownership of the "
     "clan%s\r\n"},
    {"promote", CP_PROMOTE, LVL_GRSTAFF, do_clanpromote,
     "%sclan promote <player>     %s- %sIncrease a clan member's clan"
     " rank%s\r\n"},
    {"status", CP_ALL, LVL_IMMORT, do_clanstatus,
     "%sclan status               %s- %sShows your current clan "
     "status%s\r\n"},
    {"where", CP_WHERE, LVL_IMMORT, do_clanwhere,
     "%sclan where                %s- %sShows the location of online "
     "clan members%s\r\n"},
    {"who", CP_ALL, LVL_IMMORT, do_clanlist,
     "%sclan who                  %s- %sList all members of your clan "
     "(same as clan list)%s\r\n"},
    {"withdraw", CP_WITHDRAW, LVL_GRSTAFF, do_clanwithdraw,
     "%sclan withdraw <amount>    %s- %sWithdraw gold from clan "
     "bank%s\r\n"},
    {"unclaim", CP_NONE, LVL_IMPL, do_clanunclaim,
     "%sclan unclaim <zone>       %s- %sRemove all claims over a "
     "specified zone \tw(IMP-Only)%s\r\n"},
    {"\n", 0, 0, 0, "\n"}};

/****************************************************************************
 Start basic clan utility functions
 ***************************************************************************/

/*
int find_clan_by_id(int idnum) {
  int i;

  for (i = 0; i < num_of_clans; i++)
    if (idnum == clan_list[i].id)
      return i;
  return -1;

}
*/

clan_rnum real_clan(clan_vnum c)
{
  int i;
  for (i = 0; i < num_of_clans; i++)
    if (clan_list[i].vnum == c)
      return i;

  return NO_CLAN;
}

clan_rnum get_clan_by_name(const char *c_n)
{
  int i, v;
  //  char buf[MAX_STRING_LENGTH];

  /* Look for exact match */
  for (i = 0; i < num_of_clans; i++)
  {
    if (is_abbrev(c_n, clan_list[i].clan_name))
      return i;
  }

  /* Strip color codes and look again */
  //  for (i=0; i<num_of_clans; i++) {
  //    snprintf(buf, sizeof(buf), "%s", clan_list[i].clan_name);
  //    if (is_abbrev(c_n, buf))
  //      return i;
  //  }

  /* Still not found, so let's look for the VNUM */
  if ((v = atoi(c_n)) > 0)
  {
    for (i = 0; i < num_of_clans; i++)
    {
      if (clan_list[i].vnum == v)
        return i;
    }
  }

  /* Give up! */
  return NO_CLAN;
}

int count_clan_members(clan_rnum c)
{
  int i, count = 0;

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].clan == clan_list[c].vnum)
      count++;
  }
  return count;
}

int count_clan_power(clan_rnum c)
{
  int i, count = 0;

  /* Add together the levels of all clan members */
  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].clan == clan_list[c].vnum)
      count += player_table[i].level;
  }
  return count;
}

/* Return the vnum of the clan in the list with the highest VNUM */
clan_vnum highest_clan_vnum(void)
{
  int i;
  clan_vnum hi = 0;

  for (i = 0; i < num_of_clans; i++)
  {
    if (clan_list[i].vnum > hi)
      hi = clan_list[i].vnum;
  }
  return (hi);
}

/* Add a new clan to the clan list - use current string pointers */
bool add_clan(struct clan_data *this_clan)
{
  struct clan_data *temp_list;
  int i = 0;

  /* Validate the clan data and VNUM */
  if (!this_clan)
    return FALSE;
  if (this_clan->vnum == NO_CLAN)
    return FALSE;

  /* Does clan already exist? */
  if (real_clan(this_clan->vnum) != NO_CLAN)
    return FALSE;

  /* Create a new list */
  CREATE(temp_list, struct clan_data, num_of_clans + 1);

  /* Copy the old list data */
  if (clan_list)
  {
    for (i = 0; i < num_of_clans; i++)
    {
      copy_clan_data(&(temp_list[i]), &(clan_list[i]));
    }
  }

  /* Add the new one */
  copy_clan_data(&(temp_list[i]), this_clan);

  /* Free the old list and set the temp as the new one */
  free(clan_list);
  clan_list = temp_list;
  num_of_clans++;

  return TRUE;
}

/* Totally remove a clan from the clan list -does NOT disband the clan first */
bool remove_clan(clan_vnum c_v)
{
  struct clan_data *temp_list = NULL;
  clan_rnum c_n;
  int i = 0;
  bool found = FALSE;

  /* Validate the clan data and VNUM */
  if (!clan_list || num_of_clans == 0)
    return FALSE;

  /* Does clan exist? */
  if ((c_n = real_clan(c_v)) == NO_CLAN)
    return FALSE;

  if (num_of_clans == 1)
  {
    free_clan_list();
  }
  else
  {
    /* Create a new list */
    CREATE(temp_list, struct clan_data, num_of_clans - 1);

    /* Copy the old list data */
    if (clan_list)
    {
      for (i = 0; i < num_of_clans; i++)
      {
        if (!(found))
        {
          if (i == c_n)
          {
            found = TRUE; /* Skip this one and set found flag */
          }
          else
          {
            copy_clan_data(&(temp_list[i]), &(clan_list[i]));
          }
        }
        else
        {
          copy_clan_data(&(temp_list[i - 1]), &(clan_list[i]));
        }
      }
    }

    /* Free only the string data in the old list used by the removed clan */
    if (clan_list[c_n].clan_name)
      free(clan_list[c_n].clan_name);
    for (i = 0; i < MAX_CLANRANKS; i++)
      if (clan_list[c_n].rank_name[i])
        free(clan_list[c_n].rank_name[i]);

    /* Free the old list and set the temp as the new one */
    free(clan_list);
    clan_list = temp_list;
    num_of_clans--;
  }

  return TRUE;
}

/* Free a single clan - should NOT be used for clans in the clan list */
void free_clan(struct clan_data *c)
{
  int i;
  if (c->clan_name)
    free(c->clan_name);
  for (i = 0; i < MAX_CLANRANKS; i++)
    if (c->rank_name[i])
      free(c->rank_name[i]);
  free(c);
}

/* Remove ALL clans from the MUD, freeing all memory used by the clan list */
void free_clan_list(void)
{
  struct clan_data *c;
  int i, j;
  /* Free all re-usable memory used in clan structs */
  for (i = 0; i < num_of_clans; i++)
  {
    c = &(clan_list[i]); /* Just makes this easier to read! */
    if (c->clan_name)
      free(c->clan_name);
    for (j = 0; j < MAX_CLANRANKS; j++)
      if (c->rank_name[j])
        free(c->rank_name[j]);
  }

  /* Then free the whole list */
  free(clan_list);
  clan_list = NULL;
  num_of_clans = 0;
}

/* set_clan - sets a player's clan (but not clan rank) */
bool set_clan(struct char_data *ch, clan_vnum c_v)
{
  clan_rnum c_n;
  int p_i;

  if (!ch)
    return FALSE;
  if ((c_n = real_clan(c_v)) == NO_CLAN)
    return FALSE;

  GET_CLAN(ch) = clan_list[c_n].vnum;
  save_char(ch, 0);
  if ((p_i = get_ptable_by_name(GET_NAME(ch))) < 0)
  {
    log("SYSERR: Unable to get player_table index for %s in set_clan",
        GET_NAME(ch));
    return FALSE;
  }
  else
  {
    player_table[p_i].clan = clan_list[c_n].vnum;
    save_player_index();
  }
  return TRUE;
}

/* Is 'ch' allowed to edit the specified clan (by vnum) */
bool can_edit_clan(struct char_data *ch, clan_vnum c)
{
  clan_rnum cn;

  /* Validate data */
  if (!ch)
    return FALSE;
  if (IS_NPC(ch))
    return FALSE;
  if ((cn = real_clan(c)) == NO_CLAN)
    return FALSE;

  /* IMPLementors can ALWAYS edit any clan */
  if (GET_LEVEL(ch) == LVL_IMPL)
    return TRUE;

  /* Non-imps can ONLY edit their own clan */
  if (GET_CLAN(ch) != c)
    return FALSE;

  /* Clan leader can always edit their clan */
  if (is_a_clan_leader(ch))
    return TRUE;

  /* Only fully-enrolled clan members have permissions */
  if (GET_CLANRANK(ch) == NO_CLANRANK)
    return FALSE;

  /* Check the clan permissions */
  if (GET_CLANRANK(ch) >= CLAN_PRIV(cn, CP_CLANEDIT))
    return FALSE;

  /* All checks passed! */
  return TRUE;
}

/* Copies a clan's information.  NOTE: Copies string POINTERS, not strings */
void copy_clan_data(struct clan_data *to_clan, struct clan_data *from_clan)
{
  int i;

  if (!to_clan || !from_clan)
    return;

  to_clan->vnum = from_clan->vnum;
  to_clan->clan_name = from_clan->clan_name;     /* Yes, String pointer! */
  to_clan->description = from_clan->description; /* Yes, String pointer! */
  to_clan->leader = from_clan->leader;
  to_clan->ranks = from_clan->ranks;
  to_clan->applev = from_clan->applev;
  to_clan->appfee = from_clan->appfee;
  to_clan->taxrate = from_clan->taxrate;
  to_clan->at_war = from_clan->at_war;
  to_clan->allied = from_clan->allied;
  to_clan->treasure = from_clan->treasure;
  to_clan->pk_win = from_clan->pk_win;
  to_clan->pk_lose = from_clan->pk_lose;
  to_clan->hall = from_clan->hall;
  to_clan->abrev = from_clan->abrev; /* Yes, String pointer! */

  for (i = 0; i < MAX_CLANRANKS; i++)
    /* Yes, String pointers! */
    to_clan->rank_name[i] = from_clan->rank_name[i];

  for (i = 0; i < NUM_CLAN_PRIVS; i++)
    to_clan->privilege[i] = from_clan->privilege[i];
}

/* Maybe should be free_clan(), but this doesn't re-arrange the clan_list */
void free_single_clan_data(struct clan_data *c)
{
  int i;

  /* Free allocated string data */
  if ((c)->clan_name)
    free((c)->clan_name);

  if ((c)->abrev)
    free((c)->abrev);

  if ((c)->description)
    free((c)->description);

  for (i = 0; i < MAX_CLANRANKS; i++)
    if ((c)->rank_name[i])
      free((c)->rank_name[i]);

  free(c);
}

/* Automatically appoint a new clan leader from the highest ranking officers */
bool auto_appoint_new_clan_leader(clan_rnum c_n)
{
  struct char_data *new_l, *v;
  long new_l_id = NOBODY;
  int i, rk_num, max_rk;
  bool loaded = FALSE;

  /* Verify c_n */
  if (c_n >= num_of_clans)
    return FALSE;

  /* Set the max rank as lowest possible rank */
  max_rk = clan_list[c_n].ranks;

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].clan == clan_list[c_n].vnum)
    {
      if ((v = is_playing(player_table[i].name)) != NULL)
      {
        /* If they are playing, no need to load the pfile to get data */
        rk_num = GET_CLANRANK(v) - 1;

        if (rk_num < 0)
          rk_num = (clan_list[c_n].ranks);
        if (rk_num < max_rk)
        {
          max_rk = rk_num;
          new_l_id = player_table[i].id;
        }
      }
      else
      {
        /* They are NOT playing, we need to load this pfile to find the rank */
        v = new_char();

        if (load_char(player_table[i].name, v) < 0)
        {
          free_char(v);
          continue;
        }

        rk_num = GET_CLANRANK(v) - 1;

        if (rk_num < 0)
          rk_num = (clan_list[c_n].ranks);
        if (rk_num < max_rk)
        {
          max_rk = rk_num;
          new_l_id = player_table[i].id;
        }

        free_char(v);
      }
    }
  }
  /* Check that we now know the ID of the new leader */
  if (new_l_id == NOBODY)
    return (FALSE);

  /* Load the new leader's pfile again if required */
  if ((new_l = is_playing(player_table[new_l_id].name)) == NULL)
  {
    new_l = new_char();

    if (load_char(player_table[new_l_id].name, new_l) < 0)
    {
      free_char(new_l);
      log("SYSERR: Unable to load pfile (%s) to set as leader of '%s' clan.",
          player_table[new_l_id].name, clan_list[c_n].clan_name);
      return (FALSE);
    }
    loaded = TRUE;
  }

  GET_CLANRANK(new_l) = 1;                  /* Set to top rank */
  clan_list[c_n].leader = GET_IDNUM(new_l); /* Set clan leader */
  save_clans();

  log("(CLAN) %s auto-appointed as new clan leader for '%s'", GET_NAME(new_l),
      clan_list[c_n].clan_name);

  if (loaded)
  {
    save_char(new_l, 0);
    free_char(new_l);
  }
  else
  {
    send_to_char(new_l, "You have been promoted to Clan Leader!\r\n");
    send_to_char(new_l, "'%s' is now YOUR clan!\r\n",
                 clan_list[c_n].clan_name);
  }
  return (TRUE);
}

/* Returns TRUE is the player is the leader of any clan */
bool is_a_clan_leader(struct char_data *ch)
{
  int i;
  for (i = 0; i < num_of_clans; i++)
  {
    if (clan_list[i].leader == GET_IDNUM(ch))
    {
      return TRUE;
    }
  }
  return FALSE;
}

/* Sets all a clan's values to 0 or NULL.
 * NOTE: strings should be free'd BEFORE calling */
void clear_clan_vals(struct clan_data *cl)
{
  int i;

  cl->applev = 0;
  cl->appfee = 0;
  cl->taxrate = 0;
  cl->hall = 0;
  cl->treasure = 0;
  cl->at_war = 0;
  cl->war_timer = 0;
  cl->allied = 0;
  cl->pk_win = 0;
  cl->pk_lose = 0;
  cl->clan_name = NULL;
  cl->description = NULL;
  cl->abrev = NULL;

  for (i = 0; i < MAX_CLANRANKS; i++)
    cl->rank_name[i] = NULL;

  for (i = 0; i < NUM_CLAN_PRIVS; i++)
    cl->privilege[i] = 0;
}

/****************************************************************************
 End utility functions, Start ACMDC()
 ***************************************************************************/

/* entry point for clans */
ACMD(do_clan)
{
  clan_rnum clan;
  int rank, i;
  char clan_cmd[MAX_INPUT_LENGTH];
  const char *args;

  clan = real_clan(GET_CLAN(ch));
  rank = GET_CLANRANK(ch);

  if (!argument || !*argument)
  {
    send_to_char(ch, "*Note, a staff member needs to set up new clans.\r\n");
    if ((clan == NO_CLAN) && (GET_LEVEL(ch) < LVL_IMMORT))
    {
      send_to_char(ch, "You are not a member of any clan!\r\n");
      send_to_char(ch, "Use %sclan apply <clan>%s to join one.\r\n",
                   QCYN, QNRM);
    }
    else if ((rank == NO_CLANRANK) && (GET_LEVEL(ch) < LVL_IMMORT))
    {
      send_to_char(ch, "You are awaiting approval for a clan!\r\n");
      send_to_char(ch, "Use %sclan apply <clan>%s to change your choice.\r\n",
                   QCYN, QNRM);
    }

    send_to_char(ch, "Available clan commands:\r\n");

    for (i = 0; *clan_commands[i].command && *clan_commands[i].command != '\n';
         i++)
    {
      if ((CC_PRIV(i) == CP_NONE) && (GET_LEVEL(ch) < CC_ILEV(i)))
        continue;

      if ((CC_PRIV(i) != CP_ALL) && (clan == NO_CLAN) &&
          (GET_LEVEL(ch) < CC_ILEV(i)))
        continue;

      /* Flagged accessible for all players      */
      if ((CC_PRIV(i) == CP_ALL) ||
          /* Meets minimum imm level for the command */
          (GET_LEVEL(ch) >= CC_ILEV(i)) ||
          /* Meets minimum rank for this clan priv   */
          (clan != NO_CLAN && rank >= CLAN_PRIV(clan, CC_PRIV(i))))
        send_to_char(ch, CC_TEXT(i), QCYN, QNRM, QYEL, QNRM);
    }
  }
  else
  {
    args = one_argument_c(argument, clan_cmd, sizeof(clan_cmd));

    for (i = 0; *CC_CMD(i) && *CC_CMD(i) != '\n'; i++)
    {
      if (is_abbrev(clan_cmd, CC_CMD(i)))
      {
        if ((CC_PRIV(i) == CP_NONE) && (CC_ILEV(i) != LVL_IMPL) &&
            (GET_LEVEL(ch) < CC_ILEV(i)))
        {
          continue;
        }
        else if ((CC_PRIV(i) == CP_NONE) && (CC_ILEV(i) == LVL_IMPL) &&
                 (GET_LEVEL(ch) < CC_ILEV(i)))
        {
          continue;
        }
        else if ((CC_PRIV(i) != CP_ALL) && (clan == NO_CLAN) &&
                 (GET_LEVEL(ch) < CC_ILEV(i)))
        {
          continue;
          /* Flagged accessible for all players      */
        }
        else if ((CC_PRIV(i) == CP_ALL) ||
                 /* Meets minimum imm level for the command */
                 (GET_LEVEL(ch) >= CC_ILEV(i)) ||
                 /* Meets minimum rank for this clan priv   */
                 (clan != NO_CLAN && rank >= CLAN_PRIV(clan, CC_PRIV(i))))
        {
          ((CC_FUNC(i))(ch, args, cmd, subcmd));
          return;
        }
      }
    }
    send_to_char(ch, "Unknown clan command.\r\n");
  }
}

/* clan apply command */
ACMD(do_clanapply)
{
  clan_rnum c_n;

  if (!argument || !(*argument))
  {
    send_to_char(ch, "Usage:  %sclan apply <clan>%s\r\n", QYEL, QNRM);
    return;
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "Gods cannot apply for any clan.\r\n");
    return;
  }

  if (GET_CLANRANK(ch) > NO_CLANRANK)
  {
    send_to_char(ch, "You already belong to a clan!\r\n");
    return;
  }

  if ((c_n = get_clan_by_name(argument)) == NO_CLAN)
  {
    send_to_char(ch, "Unknown clan.\r\n");
    send_to_char(ch, "Usage:  %sclan apply <clan>%s  (see 'clan info' "
                     "list)\r\n",
                 QYEL, QNRM);
    return;
  }

  if (GET_LEVEL(ch) < clan_list[c_n].applev)
  {
    send_to_char(ch, "You are not mighty enough to apply to this clan.\r\n");
    return;
  }

  if (GET_GOLD(ch) < clan_list[c_n].appfee)
  {
    send_to_char(ch, "You cannot afford the application fee!\r\n");
    return;
  }

  /* Application is allowed - take application fee */
  decrease_gold(ch, clan_list[c_n].appfee);
  clan_list[c_n].treasure += clan_list[c_n].appfee;
  save_clans();

  /* Set player as in the clan with no clan rank (pending) */
  set_clan(ch, clan_list[c_n].vnum);
  GET_CLANRANK(ch) = NO_CLANRANK;

  send_to_char(ch, "You've applied to clan '%s%s'\r\n",
               clan_list[c_n].clan_name, QNRM);
  return;
}

/* clan award command */
ACMD(do_clanaward)
{
  char plr[MAX_INPUT_LENGTH], ncp[MAX_INPUT_LENGTH];
  struct char_data *l;
  long num_cp;
  clan_rnum c_r;

  /* These checks are already made, but duplicated here just in case */
  if ((!IS_IN_CLAN(ch)) || (GET_CLANRANK(ch) == NO_CLANRANK))
  {
    send_to_char(ch, "You aren't even in a clan!\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_AWARD))
  {
    send_to_char(ch, "You don't have sufficient access to award"
                     " clan points!\r\n");
    return;
  }

  two_arguments_c(argument, plr, sizeof(plr), ncp, sizeof(ncp));

  if (!*plr || !*ncp)
  {
    send_to_char(ch, "Usage:   %sclan award <player> <num>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "Example: clan award Vash 10\r\n");
    send_to_char(ch, "Note:    Deducts 10 DD per clanpoint awarded from the"
                     " clan's bank.\r\n");
    return;
  }
  if ((l = get_player_vis(ch, plr, NULL, FIND_CHAR_WORLD)) == NULL)
  {
    send_to_char(ch, "Usage:   %sclan award <player> <num>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "Example: clan award Vash 10\r\n");
    send_to_char(ch, "Note:    Deducts 10 DD per clanpoint awarded from the"
                     " clan's bank.\r\n");
    send_to_char(ch, "         Player must be online!\r\n");
    return;
  }

  if ((c_r = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "Sorry, unable to award clanpoints at this time!\r\n");
    log("SYSERR: %s has invalid clan ID (%d) in do_clanaward", GET_NAME(ch),
        GET_CLAN(ch));
    return;
  }

  if ((num_cp = atol(ncp)) < 1)
  {
    send_to_char(ch, "Invalid number of clanpoints!\r\n");
    return;
  }

  if ((num_cp * 10) > CLAN_BANK(c_r))
  {
    send_to_char(ch, "Your clan bank doesn't have enough Double Dollars!\r\n");
    return;
  }

  GET_CLANPOINTS(l) += num_cp;
  CLAN_BANK(c_r) -= (num_cp * 10);

  send_to_char(l, "%sCLAN: You have been awarded %s%ld%s clan points!%s\r\n",
               CCMAG(l, C_NRM), CBYEL(l, C_NRM), num_cp, CCMAG(l, C_NRM), CCNRM(l, C_NRM));
  send_to_char(ch, "%s has been awarded %s%ld%s clan points!\r\n",
               GET_NAME(l), QBYEL, num_cp, QNRM);
  send_to_char(ch, "%s%s%s Double Dollars has been taken from the "
                   "clan bank!\r\n",
               QBYEL, add_commas(num_cp * 10), QNRM);
}

/* clan claim command */
ACMD(do_clanclaim)
{
  zone_vnum zv;
  zone_rnum zr;

  zr = GET_ROOM_ZONE(IN_ROOM(ch));
  zv = zone_table[(zr)].number; /* Players current zone vnum */

  if (GET_CLAN(ch) == 0 || GET_CLAN(ch) == NO_CLAN)
  {
    send_to_char(ch, "Only clan members can claim zones for their clan.\r\n");
    return;
  }

  if (GET_CLANRANK(ch) == NO_CLANRANK)
  {
    send_to_char(ch, "You can't claim zones until your clan application "
                     "is approved.\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_CLAIM))
  {
    send_to_char(ch, "You don't have sufficient access to claim zones!\r\n");
    return;
  }

  if (get_owning_clan(zv) == GET_CLAN(ch))
  {
    if (get_claimant_id(zv) == GET_IDNUM(ch))
      send_to_char(ch, "You have already claimed this area for "
                       "your clan!\r\n");
    else
      send_to_char(ch, "This area already belongs to your clan!\r\n");
    return;
  }

  if (ZONE_FLAGGED(zr, ZONE_NOCLAIM))
  {
    send_to_char(ch, "This area cannot be claimed!\r\n");
    return;
  }

  if (get_popularity(zv, GET_CLAN(ch)) < CONFIG_MIN_POP_TO_CLAIM)
  {
    send_to_char(ch, "The locals reject your claim.  Seems your clan isn't"
                     " popular here.\r\n");
    return;
  }

  log("%s claiming zone %d: Current Clan=%d, New Clan=%d",
      GET_NAME(ch), zv, get_owning_clan(zv), GET_CLAN(ch));

  if (add_claim_by_char(ch, zv) != NULL)
  {
    send_to_char(ch, "The locals rejoice and welcome your clans claim.\r\n");
    mudlog(NRM, LVL_IMMORT, TRUE, "(CLAN) %s has claimed %s [%d] for %s [%d]",
           GET_NAME(ch), zone_table[real_zone(zv)].name, zv,
           clan_list[real_clan(GET_CLAN(ch))].clan_name, GET_CLAN(ch));
    game_info("\tC%s has been claimed by %s!\r\n",
              zone_table[real_zone(zv)].name,
              clan_list[real_clan(GET_CLAN(ch))].clan_name);

    save_claims();
  }
  else
  {
    send_to_char(ch, "Sorry, your claim failed.\r\n");
    mudlog(NRM, LVL_IMMORT, TRUE, "(CLAN) %s has FAILED to claim %s [%d]"
                                  " for %s [%d]",
           GET_NAME(ch), zone_table[real_zone(zv)].name, zv,
           clan_list[real_clan(GET_CLAN(ch))].clan_name, GET_CLAN(ch));
  }
}

/* clan create (IMP) command */
ACMD(do_clancreate)
{
  char c_n[MAX_INPUT_LENGTH], c_l[MAX_INPUT_LENGTH];
  struct char_data *l;
  struct clan_data new_clan;
  int i, v;

  half_chop_c(argument, c_l, sizeof(c_l), c_n, sizeof(c_n));

  if (!*c_l || !*c_n)
  {
    send_to_char(ch, "Usage:   %sclan create <clan leader> <clan name>%s\r\n",
                 QYEL, QNRM);
    send_to_char(ch, "Example: clan create Jamdog Jamdog's Army\r\n");
    return;
  }
  if ((l = get_player_vis(ch, c_l, NULL, FIND_CHAR_WORLD)) == NULL)
  {
    send_to_char(ch, "Clan leader '%s' not found!  The player MUST be "
                     "online!\r\n",
                 c_l);
    send_to_char(ch, "Usage:   %sclan create <clan leader> <clan name>%s\r\n",
                 QYEL, QNRM);
    return;
  }
  if (GET_LEVEL(l) >= LVL_IMMORT)
  {
    send_to_char(ch, "Immortals cannot be clan members!\r\n");
    return;
  }

  /* Truncate the clan name if too long */
  if (strlen(c_n) > MAX_CLAN_NAME)
    c_n[MAX_CLAN_NAME] = '\x0';

  v = highest_clan_vnum() + 1;
  new_clan.vnum = v;
  new_clan.clan_name = strdup(c_n);
  new_clan.leader = GET_IDNUM(l);

  new_clan.ranks = 6;
  for (i = 0; i < MAX_CLANRANKS; i++)
    new_clan.rank_name[i] = NULL;
  new_clan.rank_name[5] = strdup("Recruit");
  new_clan.rank_name[4] = strdup("Member");
  new_clan.rank_name[3] = strdup("Lord");
  new_clan.rank_name[2] = strdup("Baron");
  new_clan.rank_name[1] = strdup("Count");
  new_clan.rank_name[0] = strdup("Duke");

  /* All default values for the new clan */
  new_clan.applev = 5;
  new_clan.appfee = 0;
  new_clan.taxrate = 0;
  new_clan.pk_win = 0;
  new_clan.pk_lose = 0;
  new_clan.hall = 0;
  new_clan.treasure = 0;
  new_clan.at_war = NO_CLAN;
  new_clan.allied = NO_CLAN;
  new_clan.description = NULL;
  new_clan.abrev = NULL;

  for (i = 0; i < NUM_CLAN_PRIVS; i++)
    new_clan.privilege[i] = 1; /* All privs set to top rank */

  /* Special case privs */
  /* Leader-Only: Award clanpoints     */
  new_clan.privilege[CP_AWARD] = RANK_LEADERONLY;
  /* Leader-Only: Change Owner         */
  new_clan.privilege[CP_OWNER] = RANK_LEADERONLY;
  /* Leader-Only: Edit clan using OLC  */
  new_clan.privilege[CP_CLANEDIT] = RANK_LEADERONLY;
  /* Leader-Only: Set ranks and titles */
  new_clan.privilege[CP_RANKS] = RANK_LEADERONLY;
  /* Leader-Only: Set clan name        */
  new_clan.privilege[CP_TITLE] = RANK_LEADERONLY;
  /* Leader-Only: Set clan description */
  new_clan.privilege[CP_DESC] = RANK_LEADERONLY;
  /* Leader-Only: Set clan privs       */
  new_clan.privilege[CP_SETPRIVS] = RANK_LEADERONLY;
  /* Member (Rank 5): Claim zones      */
  new_clan.privilege[CP_CLAIM] = RANK_LEADERONLY;

  send_to_char(ch, "Adding clan '%s' (Leader: %s) at VNUM %d\r\n",
               c_n, GET_NAME(l), v);
  if (add_clan(&new_clan))
  {
    send_to_char(ch, "Clan added successfully.\r\n");
    save_clans();
    /* Now we need to set the clan data in the leaders pindex record */
    GET_CLANRANK(l) = 1;
    if (set_clan(l, v))
    {
      send_to_char(l, "Your new clan, '\ty%s\tn' has been created.\r\n",
                   c_n);
      send_to_char(l, "Type %sclan%s for a list of available commands.\r\n",
                   QBWHT, QNRM);
      send_to_char(ch, "Leader set successfully - leader has been "
                       "notified.\r\n");
    }
  }
  else
  {
    send_to_char(ch, "Clan adding failed!\r\n");
  }
}

/* clan demote command - demote a clan member, even if they are offline */
ACMD(do_clandemote)
{
  struct char_data *vict;
  char buf[MAX_INPUT_LENGTH];
  bool immcom = FALSE;
  clan_rnum c_n;
  int j, rk_num, p_pos = 0;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    immcom = TRUE;

  one_argument_c(argument, buf, sizeof(buf));

  if (!immcom && (c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "You aren't in a clan!\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_DEMOTE))
  {
    send_to_char(ch, "You don't have sufficient access to demote another "
                     "clan member!\r\n");
    return;
  }

  /* Go through the full player list to find clanmembers */
  for (j = 0; j <= top_of_p_table; j++)
  {
    if (!str_cmp(player_table[j].name, buf))
    {
      if (!immcom && (player_table[j].clan != GET_CLAN(ch)))
      {
        send_to_char(ch, "That player isn't in your clan!\r\n");
        return;
      }
      /* If currently playing */
      if ((vict = is_playing(player_table[j].name)) != NULL)
      {
        if (vict == ch)
        {
          send_to_char(ch, "You can't demote yourself!\r\n");
          return;
        }

        c_n = real_clan(GET_CLAN(vict));
        rk_num = GET_CLANRANK(vict) - 1;

        if (c_n == NO_CLAN)
        {
          log("SYSERR: %s has an invalid clan! Fixing!\r\n", GET_NAME(vict));
          GET_CLAN(vict) = player_table[j].clan;
          c_n = real_clan(GET_CLAN(vict));
          if (c_n == NO_CLAN)
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data."
                             " Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
        {
          log("SYSERR: %s has an invalid clan rank! Fixing!\r\n",
              GET_NAME(vict));
          GET_CLANRANK(vict) = clan_list[c_n].ranks; /* Set to lowest rank */
          rk_num = GET_CLANRANK(vict) - 1;
          if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data."
                             " Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if (!immcom && GET_CLANRANK(vict) <= GET_CLANRANK(ch))
        {
          send_to_char(ch, "You cannot demote anyone who is above your own"
                           " rank!\r\n");
          return;
        }
        if (GET_CLANRANK(vict) >= clan_list[c_n].ranks)
        {
          send_to_char(ch, "%s is already at the lowest rank!\r\n",
                       GET_NAME(vict));
          return;
        }
        GET_CLANRANK(vict) = GET_CLANRANK(vict) + 1;
        save_char(vict, 0);
        send_to_char(vict, "You have been demoted to %s within your clan!\r\n",
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
        send_to_char(ch, "%s has been demoted to %s!\r\n", GET_NAME(vict),
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
      }
      else /* Not currently playing */
      {
        vict = new_char();

        if ((p_pos = load_char(player_table[j].name, vict)) < 0)
        {
          free_char(vict);
          return;
        }

        c_n = real_clan(GET_CLAN(vict));
        rk_num = GET_CLANRANK(vict) - 1;

        if (c_n == NO_CLAN)
        {
          log("SYSERR: %s has an invalid clan! Fixing!\r\n", GET_NAME(vict));
          GET_CLAN(vict) = player_table[j].clan;
          c_n = real_clan(GET_CLAN(vict));
          if (c_n == NO_CLAN)
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data. "
                             "Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
        {
          log("SYSERR: %s has an invalid clan rank! Fixing!\r\n",
              GET_NAME(vict));
          GET_CLANRANK(vict) = clan_list[c_n].ranks; /* Set to lowest rank */
          rk_num = GET_CLANRANK(vict) - 1;
          if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data. "
                             "Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if (!immcom && GET_CLANRANK(vict) <= GET_CLANRANK(ch))
        {
          send_to_char(ch, "You cannot demote anyone who is above your "
                           "own rank!\r\n");
          free_char(vict);
          return;
        }
        if (GET_CLANRANK(vict) >= clan_list[c_n].ranks)
        {
          send_to_char(ch, "%s is already at the lowest rank!\r\n",
                       GET_NAME(vict));
          free_char(vict);
          return;
        }
        GET_CLANRANK(vict) = GET_CLANRANK(vict) + 1;
        GET_PFILEPOS(vict) = p_pos;
        save_char(vict, 0);

        send_to_char(ch, "%s has been demoted to %s!\r\n", GET_NAME(vict),
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
        send_to_char(ch, "Saved to file.\r\n");
        free_char(vict);
      } /* End else (not playing) */
    }   /* End if name found (in player_table) */
  }
  /* End for loop (through player_table) */
}

/* clan balance command - balance of gold in clan's bank */
ACMD(do_clanbalance)
{
  char buf[MAX_INPUT_LENGTH];
  const char *buf2;
  bool immcom = FALSE;
  clan_rnum c_n = NO_CLAN;
  int amt;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    immcom = TRUE;

  buf2 = one_argument_c(argument, buf, sizeof(buf));

  if (immcom)
  {
    if (!buf2 || !*buf2)
    {
      if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
      {
        send_to_char(ch, "You aren't a member of a clan!\r\n");
        return;
      }
    }
    else
    {
      if ((c_n = get_clan_by_name(buf2)) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan!\r\n");
        send_to_char(ch, "Usage: clan deposit <amount> [clan]\r\n");
        return;
      }
    }
  }
  else
  {
    if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
    {
      send_to_char(ch, "You aren't a member of a clan!\r\n");
      return;
    }
  }

  if (!check_clanpriv(ch, CP_BALANCE))
  {
    send_to_char(ch, "You don't have sufficient access to see the balance "
                     "of the clan bank!\r\n");
    return;
  }

  amt = clan_list[(c_n)].treasure;
  send_to_char(ch, "The Clan bank balance is:\tW %d\tn\r\n", amt);
}

/* clan deposit command - deposit gold into the clan's bank */
ACMD(do_clandeposit)
{
  char buf[MAX_INPUT_LENGTH];
  const char *buf2;
  bool immcom = FALSE;
  clan_rnum c_n = NO_CLAN;
  int amt;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    immcom = TRUE;

  buf2 = one_argument_c(argument, buf, sizeof(buf));

  amt = atoi(buf);
  if (amt == 0)
  {
    if (immcom)
      send_to_char(ch, "Usage: clan deposit <amount> [clan]\r\n");
    else
      send_to_char(ch, "Usage: clan deposit <amount>\r\n");
    return;
  }

  if (immcom)
  {
    if (!buf2 || !*buf2)
    {
      if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
      {
        send_to_char(ch, "You aren't a member of a clan!\r\n");
        return;
      }
    }
    else
    {
      if ((c_n = get_clan_by_name(buf2)) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan!\r\n");
        send_to_char(ch, "Usage: clan deposit <amount> [clan]\r\n");
        return;
      }
    }
  }
  else
  {
    if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
    {
      send_to_char(ch, "You aren't a member of a clan!\r\n");
      return;
    }
  }

  if (!check_clanpriv(ch, CP_DEPOSIT))
  {
    send_to_char(ch, "You don't have sufficient access to deposit money "
                     "into the clan bank!\r\n");
    return;
  }

  if (amt < 0)
  {
    send_to_char(ch, "You can't deposit negative coins!"
                     "  Use clan withdraw!\r\n");
    return;
  }

  if (GET_GOLD(ch) < amt)
  {
    send_to_char(ch, "You don't have that many coins!\r\n");
    return;
  }

  if ((clan_list[(c_n)].treasure + amt) > MAX_BANK)
  {
    amt = MAX_BANK - clan_list[(c_n)].treasure;
    if (amt > 0)
    {
      send_to_char(ch, "The clan's bank account is almost FULL! You fill it"
                       " to maximum.\r\n");
    }
    else
    {
      send_to_char(ch, "The clan's bank account is FULL! You can't deposit"
                       " any more.\r\n");
      return;
    }
  }

  clan_list[(c_n)].treasure += amt;
  decrease_gold(ch, amt);

  save_char(ch, 0);
  save_clans();

  send_to_char(ch, "You have deposited %d coins into the clan bank.\r\n", amt);
}

/* clan destroy (IMP) command */
ACMD(do_clandestroy)
{
  struct char_data *vict;
  bool immcom = FALSE;
  clan_vnum c_v;
  clan_rnum c_n;
  char c_name[MAX_INPUT_LENGTH], c_ldr[MAX_NAME_LENGTH],
      buf[MAX_INPUT_LENGTH];
  const char *buf2;
  int j, p_pos = 0;
  long c_lid;

  buf2 = one_argument_c(argument, buf, sizeof(buf));

  if (GET_LEVEL(ch) == LVL_IMPL)
    immcom = TRUE;

  if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    if (immcom)
    {
      if ((c_n = real_clan(atoi(buf2))) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan VNUM specified.\r\n");
        send_to_char(ch, "The clan you entered %d, doesn't match your"
                         " clan %d.\r\n",
                     real_clan(atoi(buf2)),
                     real_clan(GET_CLAN(ch)));
        return;
      }
    }
    else
    {
      send_to_char(ch, "You aren't in a clan!\r\n");
      return;
    }
  }

  if ((GET_IDNUM(ch) != clan_list[c_n].leader) &&
      (GET_LEVEL(ch) < LVL_IMPL))
  {
    send_to_char(ch, "Only the clan leader or an implementor can "
                     "disband a clan!\r\n");
    return;
  }

  c_lid = -1;

  /* Go through the full player list to find clanmembers */
  for (j = 0; j <= top_of_p_table; j++)
  {
    /* If this is the leader, grab their list position */
    if (player_table[j].id == clan_list[c_n].leader)
      c_lid = j;

    /* Are they in the clan? */
    if (player_table[j].clan == clan_list[c_n].vnum)
    {
      /* If currently playing */
      if ((vict = is_playing(player_table[j].name)) != NULL)
      {
        GET_CLAN(vict) = 0;
        GET_CLANRANK(vict) = 0;
        save_char(vict, 0);
        if (vict != ch)
        {
          send_to_char(vict, "Your clan has been disbanded.  "
                             "You are no longer in a clan!\r\n");
        }
      }
      else /* Not currently playing */
      {
        vict = new_char();

        if ((p_pos = load_char(player_table[j].name, vict)) < 0)
        {
          free_char(vict);
          return;
        }
        GET_CLAN(vict) = 0;
        GET_CLANRANK(vict) = 0;
        GET_PFILEPOS(vict) = p_pos;
        save_char(vict, 0);

        free_char(vict);
      } /* End else (not playing) */
      player_table[j].clan = 0;
    } /* End if   (in the clan) */
  }   /* End for loop (through player_table) */

  /* Now we re-arrange the clans to remove this one */
  c_v = clan_list[c_n].vnum;
  snprintf(c_name, sizeof(c_name), "%s", clan_list[c_n].clan_name);

  if (c_lid >= 0 && c_lid < top_of_p_table)
    snprintf(c_ldr, sizeof(c_ldr), "%s", player_table[c_lid].name);
  else
    snprintf(c_ldr, sizeof(c_ldr), "<Unknown!>");

  if (remove_clan(c_v))
  {
    send_to_char(ch, "Clan disbanded and destroyed!\r\n");
    log("(CLAN) Clan '%s' (leader=%s) destroyed by %s.", c_name, CAP(c_ldr),
        GET_NAME(ch));
  }
  else
  {
    send_to_char(ch, "Unable to destroy clan!\r\n");
  }
}

ACMD(do_clanenrol)
{
  char arg[MAX_INPUT_LENGTH];
  clan_rnum c_n = NO_CLAN;
  struct char_data *v = NULL;
  bool immcom = FALSE;
  int count = 0, i;

  if (GET_LEVEL(ch) == LVL_IMPL)
    immcom = TRUE;

  one_argument_c(argument, arg, sizeof(arg));

  if (!immcom && (c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "You aren't a member of any clan.\r\n");
    return;
  }
  else if (immcom)
  {
    if (*arg && is_number(arg))
    {
      if ((c_n = real_clan(atoi(arg))) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan ID\r\n");
        return;
      }
    }
    else
    {
      if (!immcom && (c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
      {
        send_to_char(ch, "You aren't a member of any clan.\r\n");
        return;
      }
    }
  }

  if (!immcom && (GET_CLANRANK(ch) < 1))
  {
    send_to_char(ch, "YOU aren't enrolled into your clan yet.\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_ENROL))
  {
    send_to_char(ch, "You don't have sufficient access to enrol "
                     "applicants!\r\n");
    return;
  }

  if (!immcom && !*arg)
  {
    for (i = 0; i <= top_of_p_table; i++)
    {
      if (player_table[i].clan == clan_list[c_n].vnum)
      {
        if ((v = is_playing(player_table[i].name)) != NULL)
        {
          if (GET_CLANRANK(v) == NO_CLANRANK)
          {
            if (count == 0)
            {
              send_to_char(ch, "%sApplicants to the '%s%s' clan:%s\r\n",
                           QCYN, clan_list[c_n].clan_name, QCYN, QNRM);
            }
            send_to_char(ch, "%s%c%-15s  %s(Level %d)%s\r\n", QYEL,
                         UPPER(*player_table[i].name), player_table[i].name + 1,
                         QCYN, player_table[i].level, QNRM);
            count++;
          }
        }
      }
    }
    if (count)
    {
      send_to_char(ch, "%sThere are %d online player%s awaiting "
                       "enrolment.\r\n",
                   QNRM, count, (count == 1) ? "" : "s");
      send_to_char(ch, "Usage: %sclan enrol <player>%s\r\n", QYEL, QNRM);
    }
    return;
  }

  if (!(v = get_player_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "Er, Who ??  You can only enrol someone if they are"
                     " online!\r\n");
    return;
  }

  if ((c_n = real_clan(GET_CLAN(v))) == NO_CLAN)
  {
    send_to_char(ch, "%s has not yet applied to join any clan.\r\n",
                 GET_NAME(v));
    return;
  }

  if (!immcom && (c_n != real_clan(GET_CLAN(ch))))
  {
    send_to_char(ch, "%s has not applied to join YOUR clan.\r\n",
                 GET_NAME(v));
    return;
  }

  if (GET_CLANRANK(v) != NO_CLANRANK)
  {
    send_to_char(ch, "%s is already enrolled.\r\n", GET_NAME(v));
    return;
  }

  GET_CLANRANK(v) = (clan_list[(c_n)].ranks); /* Set to lowest rank    */
  set_clan(v, clan_list[(c_n)].vnum);         /* Set the clan and save */

  send_to_char(ch, "%s is now a member of %s%s.\r\n", GET_NAME(v),
               clan_list[(c_n)].clan_name, QNRM);
  send_to_char(v, "You have been enrolled into %s%s.\r\n",
               clan_list[(c_n)].clan_name, CCNRM(v, C_NRM));
}

ACMD(do_clanexpel) /* Expel a member */
{
  char arg[MAX_INPUT_LENGTH];
  clan_rnum c_n = NO_CLAN;
  struct char_data *v = NULL;
  long v_id;
  int v_pos;
  bool immcom = FALSE;

  if (GET_LEVEL(ch) == LVL_IMPL)
    immcom = TRUE;

  one_argument_c(argument, arg, sizeof(arg));

  if (!immcom && (c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "You aren't a member of any clan.\r\n");
    return;
  }

  if (!immcom && (GET_CLANRANK(ch) < 1))
  {
    send_to_char(ch, "YOU aren't enrolled into your clan yet.\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_EXPEL))
  {
    send_to_char(ch, "You don't have sufficient access to expel clan"
                     " members!\r\n");
    return;
  }

  if (!*arg)
  {
    send_to_char(ch, "Usage: %sclan expel <player>%s\r\n", QYEL, QNRM);
    return;
  }

  if ((v_id = get_ptable_by_name(arg)) < 0)
  {
    send_to_char(ch, "Er, Who ??\r\n");
    return;
  }

  if (!immcom && (player_table[(v_id)].clan != clan_list[(c_n)].vnum))
  {
    send_to_char(ch, "They aren't in YOUR clan!\r\n");
    return;
  }

  c_n = real_clan(player_table[(v_id)].clan);

  if ((v = is_playing(player_table[(v_id)].name)) != NULL)
  {

    GET_CLAN(v) = 0;
    GET_CLANRANK(v) = NO_CLANRANK;
    player_table[v_id].clan = 0;

    send_to_char(v, "You have been expelled from %s%s!\r\n",
                 clan_list[(c_n)].clan_name, CCNRM(v, C_NRM));
    send_to_char(ch, "You have expelled %s from %s%s!\r\n",
                 GET_NAME(v), clan_list[(c_n)].clan_name, CCNRM(v, C_NRM));
    log("(CLAN) %s has expelled %s from %s!\r\n", GET_NAME(ch), GET_NAME(v),
        clan_list[(c_n)].clan_name);
    save_char(v, 0);
    save_player_index();
  }
  else
  {
    v = new_char();

    if ((v_pos = load_char(player_table[v_id].name, v)) < 0)
    {
      free_char(v);
      log("SYSERR: Unable to load char %s in do_clanexpel",
          player_table[v_id].name);
      return;
    }
    GET_CLAN(v) = 0;
    GET_CLANRANK(v) = NO_CLANRANK;
    player_table[v_id].clan = 0;
    GET_PFILEPOS(v) = v_pos;

    send_to_char(v, "You have been expelled from %s%s!\r\n",
                 clan_list[(c_n)].clan_name, CCNRM(v, C_NRM));
    send_to_char(ch, "You have expelled %s from %s%s!\r\n", GET_NAME(v),
                 clan_list[(c_n)].clan_name, CCNRM(v, C_NRM));
    send_to_char(ch, "Saved to file.\r\n");
    log("(CLAN) %s has expelled %s from %s!\r\n", GET_NAME(ch), GET_NAME(v),
        clan_list[(c_n)].clan_name);

    save_char(v, 0);
    save_player_index();

    free_char(v);
  }
}

ACMD(do_claninfo) /* Information about clans */
{
  int i, j, count = 0, mems, pow;
  char pr[4];

  if (!argument || !*argument)
  {
    for (i = 0; i < num_of_clans; i++)
    {
      if (++count == 1)
        send_to_char(ch, "Clan ID  Clan Name                       "
                         "Members  Power\r\n");
      mems = count_clan_members(i);
      pow = count_clan_power(i);
      send_to_char(ch, "[%5d]  %-*s%s  %7d  %5d\r\n", clan_list[i].vnum,
                   30 + count_color_chars(clan_list[i].clan_name),
                   clan_list[i].clan_name, QNRM, mems, pow);
    }
    if (count == 0)
    {
      send_to_char(ch, "There are currently no clans in the realm of "
                       "Luminari.\r\n");
    }
    else
    {
      send_to_char(ch, "There %s %d clan%s in realm of Luminari.\r\n",
                   (count == 1) ? "is" : "are", count, (count == 1) ? "" : "s");
    }
    return;
  }
  else
  { /* There is an arg - check it */
    if ((i = get_clan_by_name(argument)) == NO_CLAN)
    {
      send_to_char(ch, "There is no such clan!\r\n");
    }
    else
    {
      /* Verify that this clan has a valid leader */
      if (get_name_by_id(clan_list[i].leader) == NULL)
      {
        /* Somehow, we have no leader for this clan! */
        if (auto_appoint_new_clan_leader(i) == FALSE)
        {
          send_to_char(ch, "That clan has no members!\r\n");
          return;
        }
      }
      send_to_char(ch, "Clan Information for %s%s\r\n", clan_list[i].clan_name,
                   QNRM);
      send_to_char(ch, "Abbrev : %s%s%s\r\n", QYEL, clan_list[i].abrev, QNRM);
      send_to_char(ch, "Leader : %s%c%s%s\r\n", QCYN,
                   UPPER(*get_name_by_id(clan_list[i].leader)),
                   get_name_by_id(clan_list[i].leader) + 1, QNRM);
      send_to_char(ch, "Members: %s%d%s\r\n", QCYN, count_clan_members(i),
                   QNRM);
      send_to_char(ch, "Power  : %s%d%s\r\n", QCYN, count_clan_power(i), QNRM);
      if ((GET_IDNUM(ch) == clan_list[i].leader) ||
          (GET_LEVEL(ch) == LVL_IMPL))
      {
        send_to_char(ch, "Bank   : %s%ld coins%s\r\n", QYEL,
                     clan_list[i].treasure, QNRM);
      }
      send_to_char(ch, "App Lev: %s%d%s\r\n", QYEL, clan_list[i].applev, QNRM);
      send_to_char(ch, "App Fee: %s%d%s\r\n", QYEL, clan_list[i].appfee, QNRM);
      send_to_char(ch, "At War : %s%s  \r\n",
                   real_clan(clan_list[i].at_war) == NO_CLAN ? "<None!>" : clan_list[(real_clan(clan_list[i].at_war))].clan_name, QNRM);
      send_to_char(ch, "Allied : %s%s  \r\n",
                   real_clan(clan_list[i].allied) == NO_CLAN ? "<None!>" : clan_list[(real_clan(clan_list[i].allied))].clan_name, QNRM);
      send_to_char(ch, "PK Won : %s%d%s\r\n", QYEL, clan_list[i].pk_win, QNRM);
      send_to_char(ch, "PK Lost: %s%d%s\r\n", QYEL,
                   clan_list[i].pk_lose, QNRM);
      send_to_char(ch, "Ranks  : %s%d%s\r\n", QYEL, clan_list[i].ranks, QNRM);
      send_to_char(ch, "Titles : ");
      if (clan_list[i].ranks > 0)
      {
        send_to_char(ch, "%s[%s%-2d%s] %s%s", QCYN, QYEL, 1, QCYN,
                     clan_list[i].rank_name[0], QNRM);
        for (j = 1; j < clan_list[i].ranks; j++)
        {
          send_to_char(ch, "\r\n         %s[%s%-2d%s] %s%s", QCYN, QYEL, j + 1,
                       QCYN, clan_list[i].rank_name[j], QNRM);
        }
      }
      else
      {
        send_to_char(ch, "%s<None Set!>%s", QCYN, QNRM);
      }

      // added to show clan description in the clan info display
      if (clan_list[i].description == NULL)
      {
        send_to_char(ch, "\r\n\nDescription : Not set.\r\n");
      }
      else
        send_to_char(ch, "\r\n\nDescription : %s\r", clan_list[i].description);

      /* Only clan leader and imps can see the privs on this screen */
      if ((GET_IDNUM(ch) == clan_list[i].leader) ||
          (GET_LEVEL(ch) == LVL_IMPL))
      {
        send_to_char(ch, "\r\n%sClan Privs%s\r\n", QBCYN, QNRM);
        count = 0;
        for (j = 0; j < NUM_CLAN_PRIVS; j++)
        {
          if (clan_list[i].privilege[j] > 0)
            snprintf(pr, sizeof(pr), "%2d", clan_list[i].privilege[j]);
          else
            snprintf(pr, sizeof(pr), "LO");
          send_to_char(ch, "%12s: %s[%s%2s%s]%s%s", clan_priv_names[j], QCYN,
                       QYEL, pr, QCYN, QNRM, !(++count % 4) ? "\r\n" : "");
        }
        send_to_char(ch, "\r\n%sClan Zone Popularity%s\r\n", QBCYN, QNRM);
        show_clan_popularities(ch, clan_list[i].vnum);
      }
      send_to_char(ch, "\r\n");
      show_clan_claims(ch, clan_list[i].vnum);
    }
  }
}

ACMD(do_clanlist) /* List of clan members */
{
  int i, rk_num;
  clan_rnum c;
  clan_vnum vc;
  struct char_data *v;
  char rk_name[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  if (!GET_CLAN(ch) && (GET_LEVEL(ch) < LVL_IMMORT))
  {
    send_to_char(ch, "You are not in a clan!");
    return;
  }
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    one_argument_c(argument, arg, sizeof(arg));
    if (!*arg)
    {
      if ((c = real_clan(GET_CLAN(ch))) == NO_CLAN)
      {
        send_to_char(ch, "Your clan is invalid - see the syslog!\r\n");
        log("SYSERR: Player %s has clan VNUM %d, but clan isn't found in "
            "clan_list",
            GET_NAME(ch), GET_CLAN(ch));
        return;
      }
    }
    else
    {
      vc = atoi(arg);
      if ((c = real_clan(vc)) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan ID!\r\n");
        return;
      }
    }
  }
  else
  {
    if ((c = real_clan(GET_CLAN(ch))) == NO_CLAN)
    {
      send_to_char(ch, "Your clan is invalid - tell an Imm!\r\n");
      log("SYSERR: Player %s has clan VNUM %d, but clan isn't found in "
          "clan_list",
          GET_NAME(ch), GET_CLAN(ch));
      return;
    }
  }

  send_to_char(ch, "%sMembers of the '%s%s' clan:%s\r\n", QCYN,
               clan_list[c].clan_name, QCYN, QNRM);

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].clan == clan_list[c].vnum)
    {
      if ((v = is_playing(player_table[i].name)) != NULL)
      {
        rk_num = GET_CLANRANK(v) - 1;

        if (rk_num >= 0)
          snprintf(rk_name, sizeof(rk_name), "%s", clan_list[c].rank_name[rk_num]);
        else
          snprintf(rk_name, sizeof(rk_name), "%sINVALID!", QBRED);

        /* Show leader in white */
        if (player_table[i].id == clan_list[c].leader)
          send_to_char(ch, "%s(Online)%s   %c%-15s  %s%s (Leader)%s\r\n",
                       QBGRN, QBWHT, UPPER(*player_table[i].name),
                       player_table[i].name + 1, rk_name, QBWHT, QNRM);
        else if (GET_CLANRANK(v) == NO_CLANRANK)
          send_to_char(ch, "%s(Online)%s   %c%-15s  %s(Applicant)%s\r\n",
                       QBGRN, QNRM, UPPER(*player_table[i].name),
                       player_table[i].name + 1, QBRED, QNRM);
        else
          send_to_char(ch, "%s(Online)%s   %c%-15s  %s%s\r\n",
                       QBGRN, QNRM, UPPER(*player_table[i].name),
                       player_table[i].name + 1, rk_name, QNRM);
      }
      else
      {
        v = new_char();

        if (load_char(player_table[i].name, v) < 0)
        {
          free_char(v);
          continue;
        }

        rk_num = GET_CLANRANK(v) - 1;

        if (rk_num >= 0)
          snprintf(rk_name, sizeof(rk_name), "%s", clan_list[c].rank_name[rk_num]);
        else
          snprintf(rk_name, sizeof(rk_name), "%sINVALID!", QBRED);

        /* Show leader in white */
        if (player_table[i].id == clan_list[c].leader)
          send_to_char(ch, "%s(Offline)%s  %c%-15s  %s%s (Leader)%s\r\n",
                       QBRED, QBWHT, UPPER(*player_table[i].name),
                       player_table[i].name + 1, rk_name, QBWHT, QNRM);
        else if (GET_CLANRANK(v) == NO_CLANRANK)
          send_to_char(ch, "%s(Offline)%s  %c%-15s  %s(Applicant)%s\r\n",
                       QBRED, QNRM, UPPER(*player_table[i].name),
                       player_table[i].name + 1, QBRED, QNRM);
        else
          send_to_char(ch, "%s(Offline)%s  %c%-15s  %s%s\r\n",
                       QBRED, QNRM, UPPER(*player_table[i].name),
                       player_table[i].name + 1, rk_name, QNRM);

        free_char(v);
      }
    }
  }
  return;
}

ACMD(do_clanowner)
{
  clan_rnum c_n;
  struct char_data *new_l;
  char buf[MAX_STRING_LENGTH];
  const char *buf2;
  bool immcom = FALSE;

  buf2 = one_argument_c(argument, buf, sizeof(buf));

  if (GET_LEVEL(ch) == LVL_IMPL)
    immcom = TRUE;

  if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    if (immcom)
    {
      if ((c_n = real_clan(atoi(buf2))) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan VNUM specified.\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "You aren't in a clan!\r\n");
      return;
    }
  }

  if ((GET_IDNUM(ch) != clan_list[c_n].leader) &&
      (GET_LEVEL(ch) < LVL_IMPL))
  {
    send_to_char(ch, "Only the clan leader or an implementor can change"
                     " clan ownership!\r\n");
    return;
  }

  if ((new_l = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)) == NULL)
  {
    send_to_char(ch, "Player not found! The new owner must be online.\r\n");
    return;
  }
  if ((GET_LEVEL(ch) < LVL_IMPL) && (GET_LEVEL(new_l) >= LVL_IMMORT))
  {
    send_to_char(ch, "The new owner cannot be an immortal!\r\n");
    return;
  }
  if (real_clan(GET_CLAN(new_l)) != c_n)
  {
    send_to_char(ch, "The new owner must be a member of the clan!\r\n");
    return;
  }
  GET_CLANRANK(ch) = 1;    /* Set to top rank */
  GET_CLANRANK(new_l) = 1; /* Set to top rank */
  clan_list[c_n].leader = GET_IDNUM(new_l);
  save_clans();

  send_to_char(ch, "Ownership changed to %s.\r\n", GET_NAME(new_l));
  send_to_char(new_l, "You have been promoted to Clan Leader!\r\n");
  send_to_char(new_l, "'%s' is now YOUR clan!\r\n", clan_list[c_n].clan_name);
}

ACMD(do_clanpromote)
{
  struct char_data *vict;
  char buf[MAX_INPUT_LENGTH];
  bool immcom = FALSE;
  clan_rnum c_n;
  int j, rk_num, p_pos = 0;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    immcom = TRUE;

  one_argument_c(argument, buf, sizeof(buf));

  if (!immcom && (c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "You aren't in a clan!\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_PROMOTE))
  {
    send_to_char(ch, "You don't have sufficient access to promote"
                     " clan members!\r\n");
    return;
  }

  /* Go through the full player list to find clanmembers */
  for (j = 0; j <= top_of_p_table; j++)
  {
    if (!str_cmp(player_table[j].name, buf))
    {
      if (!immcom && (player_table[j].clan != GET_CLAN(ch)))
      {
        send_to_char(ch, "That player isn't in your clan!\r\n");
        return;
      }
      /* If currently playing */
      if ((vict = is_playing(player_table[j].name)) != NULL)
      {
        if (vict == ch)
        {
          send_to_char(ch, "You can't promote yourself!\r\n");
          return;
        }

        c_n = real_clan(GET_CLAN(vict));
        rk_num = GET_CLANRANK(vict) - 1;

        if (c_n == NO_CLAN)
        {
          log("SYSERR: %s has an invalid clan! Fixing!\r\n", GET_NAME(vict));
          GET_CLAN(vict) = player_table[j].clan;
          c_n = real_clan(GET_CLAN(vict));
          if (c_n == NO_CLAN)
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data."
                             " Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
        {
          log("SYSERR: %s has an invalid clan rank! Fixing!\r\n",
              GET_NAME(vict));
          GET_CLANRANK(vict) = clan_list[c_n].ranks; /* Set to lowest rank */
          rk_num = GET_CLANRANK(vict) - 1;
          if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data."
                             " Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if (!immcom && GET_CLANRANK(vict) <= GET_CLANRANK(ch))
        {
          send_to_char(ch, "You cannot promote %s above your own rank!\r\n",
                       GET_NAME(vict));
          return;
        }
        if (GET_CLANRANK(vict) <= 1)
        {
          send_to_char(ch, "%s is already at the highest rank!\r\n",
                       GET_NAME(vict));
          return;
        }
        GET_CLANRANK(vict) = rk_num;
        save_char(vict, 0);
        send_to_char(vict, "You have been promoted to %s within your"
                           " clan!\r\n",
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
        send_to_char(ch, "%s has been promoted to %s!\r\n", GET_NAME(vict),
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
      }
      else /* Not currently playing */
      {
        vict = new_char();

        if ((p_pos = load_char(player_table[j].name, vict)) < 0)
        {
          free_char(vict);
          return;
        }

        c_n = real_clan(GET_CLAN(vict));
        rk_num = GET_CLANRANK(vict) - 1;

        if (c_n == NO_CLAN)
        {
          log("SYSERR: %s has an invalid clan! Fixing!\r\n", GET_NAME(vict));
          GET_CLAN(vict) = player_table[j].clan;
          c_n = real_clan(GET_CLAN(vict));
          if (c_n == NO_CLAN)
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data. "
                             "Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
        {
          log("SYSERR: %s has an invalid clan rank! Fixing!\r\n",
              GET_NAME(vict));
          GET_CLANRANK(vict) = clan_list[c_n].ranks; /* Set to lowest rank */
          rk_num = GET_CLANRANK(vict) - 1;
          if ((rk_num < 0) || (rk_num >= clan_list[c_n].ranks))
          {
            log("SYSERR: Unable to Fix - aborting!\r\n");
            send_to_char(ch, "Sorry, %s seems to have invalid clan data. "
                             "Aborting!\r\n",
                         GET_NAME(vict));
            return;
          }
        }

        if (!immcom && GET_CLANRANK(vict) <= GET_CLANRANK(ch))
        {
          send_to_char(ch, "You cannot promote %s above your own rank!\r\n",
                       GET_NAME(vict));
          free_char(vict);
          return;
        }
        if (GET_CLANRANK(vict) <= 1)
        {
          send_to_char(ch, "%s is already at the highest rank!\r\n",
                       GET_NAME(vict));
          free_char(vict);
          return;
        }
        GET_CLANRANK(vict) = rk_num;
        GET_PFILEPOS(vict) = p_pos;
        save_char(vict, 0);

        send_to_char(ch, "%s has been promoted to %s!\r\n", GET_NAME(vict),
                     clan_list[c_n].rank_name[(GET_CLANRANK(vict) - 1)]);
        send_to_char(ch, "Saved to file.\r\n");
        free_char(vict);
      } /* End else (not playing) */
    }   /* End if name found (in player_table) */
  }     /* End for loop (through player_table) */
}

ACMD(do_clanstatus)
{
  clan_rnum c_n;
  int c_r;

  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    send_to_char(ch, "You are immortal and cannot join any clan!\r\n");
    return;
  }

  c_n = real_clan(GET_CLAN(ch));
  c_r = GET_CLANRANK(ch);

  if (c_r == NO_CLANRANK)
  {
    if (c_n != NO_CLAN)
    {
      send_to_char(ch, "You have applied to %s%s\r\n", clan_list[c_n].clan_name,
                   QNRM);
    }
    else
    {
      send_to_char(ch, "You do not belong to a clan!\r\n");
    }
    return;
  }

  if (c_n == NO_CLAN)
  {
    send_to_char(ch, "You do not belong to a clan!\r\n");
  }
  else
  {
    send_to_char(ch, "You are %s (Rank %d) of %s\r\n",
                 clan_list[c_n].rank_name[(c_r - 1)], c_r,
                 clan_list[c_n].clan_name);

    if (clan_list[c_n].leader == GET_IDNUM(ch))
      send_to_char(ch, "You are the leader of this clan!\r\n");
  }

  return;
}

ACMD(do_clanwhere)
{
  clan_rnum c;
  room_rnum r;
  zone_rnum z;
  int count = 0, c_r, i;
  struct char_data *vict;

  if (!GET_CLAN(ch))
  {
    send_to_char(ch, "You are not in a clan!");
    return;
  }

  if ((c = real_clan(GET_CLAN(ch))) == NO_CLAN)
  {
    send_to_char(ch, "Your clan is invalid - tell an Imm!\r\n");
    log("SYSERR: Player %s has clan VNUM %d, but clan isn't found in"
        " clan_list",
        GET_NAME(ch), GET_CLAN(ch));
    return;
  }

  if (!check_clanpriv(ch, CP_WHERE))
  {
    send_to_char(ch, "You don't have sufficient access to view member "
                     "locations!\r\n");
    return;
  }

  send_to_char(ch, "%sLocations for members of the '%s%s' clan:%s\r\n",
               QCYN, clan_list[c].clan_name, QCYN, QNRM);

  for (i = 0; i <= top_of_p_table; i++)
  {
    if (player_table[i].clan == clan_list[c].vnum)
    {
      if ((vict = is_playing(player_table[i].name)) != NULL)
      {
        r = IN_ROOM(vict);
        z = world[r].zone;
        c_r = GET_CLANRANK(vict);
        if (count++ == 0)
          send_to_char(ch, "%sName             Rank          Room         "
                           "              Zone%s\r\n",
                       QCYN, QNRM);
        if (GET_IDNUM(vict) == clan_list[c].leader)
          send_to_char(ch, "%-15s  %sClan Leader!%s  %-25s%s  %-20s%s\r\n",
                       GET_NAME(vict), QBWHT, QNRM, world[r].name, QNRM,
                       zone_table[z].name, QNRM);
        else if (c_r == NO_CLANRANK)
          send_to_char(ch, "%-15s  %s(Applicant) %s  %-25s%s  %-20s%s\r\n",
                       GET_NAME(vict), QBRED, QNRM, world[r].name, QNRM,
                       zone_table[z].name, QNRM);
        else
          send_to_char(ch, "%-15s  %-12s%s  %-25s%s  %-20s%s\r\n",
                       GET_NAME(vict), clan_list[c].rank_name[(c_r - 1)],
                       QNRM, world[r].name, QNRM, zone_table[z].name, QNRM);
      }
    }
  }
}

ACMD(do_clanwithdraw)
{
  char buf[MAX_INPUT_LENGTH];
  const char *buf2;
  bool immcom = FALSE;
  clan_rnum c_n = NO_CLAN;
  int amt;

  if (GET_LEVEL(ch) > LVL_IMMORT)
    immcom = TRUE;

  buf2 = one_argument_c(argument, buf, sizeof(buf));

  amt = atoi(buf);
  if (amt == 0)
  {
    if (immcom)
      send_to_char(ch, "Usage: clan withdraw <amount> [clan]\r\n");
    else
      send_to_char(ch, "Usage: clan withdraw <amount>\r\n");
    return;
  }

  if (immcom)
  {
    if (!buf2 || !*buf2)
    {
      if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
      {
        send_to_char(ch, "You aren't a member of a clan!\r\n");
        return;
      }
    }
    else
    {
      if ((c_n = get_clan_by_name(buf2)) == NO_CLAN)
      {
        send_to_char(ch, "Invalid clan!\r\n");
        send_to_char(ch, "Usage: clan withdraw <amount> [clan]\r\n");
        return;
      }
    }
  }
  else
  {
    if ((c_n = real_clan(GET_CLAN(ch))) == NO_CLAN)
    {
      send_to_char(ch, "You aren't a member of a clan!\r\n");
      return;
    }
  }

  if (!check_clanpriv(ch, CP_WITHDRAW))
  {
    send_to_char(ch, "You don't have sufficient access to withdraw from the"
                     " clan bank!\r\n");
    return;
  }

  if (amt < 0)
  {
    send_to_char(ch, "You can't withdraw negative coins!  "
                     "Use clan deposit!\r\n");
    return;
  }

  if (GET_GOLD(ch) > MAX_GOLD)
  {
    send_to_char(ch, "You can't hold that many coins!\r\n");
    return;
  }

  if ((clan_list[(c_n)].treasure - amt) < 0)
  {
    amt = clan_list[(c_n)].treasure;
    if (amt > 0)
    {
      send_to_char(ch, "The clan's bank account is almost EMPTY! You take"
                       " what remains.\r\n");
    }
    else
    {
      send_to_char(ch, "The clan's bank account is EMPTY! You can't withdraw"
                       " any more.\r\n");
      return;
    }
  }

  clan_list[(c_n)].treasure -= amt;
  increase_gold(ch, amt);

  save_char(ch, 0);
  save_clans();

  send_to_char(ch, "You have withdrawn %d coins from the clan bank.\r\n", amt);
}

ACMD(do_clanunclaim)
{
  int z, zr;
  bool found = FALSE;
  struct claim_data *this_claim;

  if (!argument || !*argument)
  {
    send_to_char(ch, "Usage: %sclan unclaim <zone vnum>%s\r\n", QYEL, QNRM);
    return;
  }

  if (GET_LEVEL(ch) < LVL_IMPL)
  {
    send_to_char(ch, "Implementors ONLY!\r\n");
    return;
  }

  z = atoi(argument);

  if ((zr = real_zone(z)) == NOWHERE)
  {
    send_to_char(ch, "Invalid clan vnum\r\n");
    return;
  }

  if (get_owning_clan(z) == NO_CLAN)
  {
    send_to_char(ch, "%s hasn't been claimed\r\n",
                 zone_table[real_zone(z)].name);
    return;
  }
  for (this_claim = claim_list; this_claim && found == FALSE;
       this_claim = this_claim->next)
  {
    if (this_claim->zn == z)
    {
      found = TRUE;
      /* For total removal, including popularity, use remove_claim_from_list */
      CLAIM_CLAN(this_claim) = NO_CLAN;
      CLAIM_CLAIMANT(this_claim) = 0L; /* same as (long int) 0 */
      send_to_char(ch, "Claim removed from %s\r\n",
                   zone_table[real_zone(z)].name);
    }
  }
}

/************************************************************************
 End of Clan commands - Start of Zone Claim Code
 ***********************************************************************/

struct claim_data *new_claim(void)
{
  struct claim_data *new_data;
  int i;

  CREATE(new_data, struct claim_data, 1);

  new_data->zn = NOWHERE;
  new_data->clan = NO_CLAN;
  new_data->claimant = 0;
  new_data->next = NULL;
  for (i = 0; i < MAX_CLANS; i++)
    new_data->popularity[i] = 0.0;

  return (new_data);
}

struct claim_data *get_claim_by_zone(zone_vnum z_num)
{
  struct claim_data *this_claim;

  if (!claim_list)
    return NULL;

  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (this_claim->zn == z_num)
      return this_claim;
  }
  return NULL;
}

void remove_claim_from_list(struct claim_data *rem_claim)
{
  struct claim_data *this_claim = NULL;

  if (!claim_list)
    return;

  /* Check the main claim list */
  for (this_claim = claim_list; this_claim && this_claim != rem_claim;
       this_claim = this_claim->next)
    ;
  if (this_claim == NULL)
    return;

  /* We found the claim in the list, remove it */
  if (this_claim == claim_list)
  {
    /* 1st item - move the main claim_list pointer */
    claim_list = claim_list->next;
    free_claim(this_claim);
  }
  else
  {
    for (this_claim = claim_list; this_claim && this_claim->next != rem_claim;
         this_claim = this_claim->next)
      ;
    this_claim->next = rem_claim->next;
    free_claim(rem_claim);
  }
}

void free_claim(struct claim_data *this_claim)
{
  if (this_claim)
  {
    if (get_claim_by_zone(this_claim->zn) != NULL)
      remove_claim_from_list(this_claim);
    free(this_claim);
  }
}

void free_claim_list(void)
{
  struct claim_data *this_claim;

  for (this_claim = claim_list; this_claim; this_claim = claim_list)
  {
    claim_list = this_claim->next;
    free_claim(this_claim);
  }
  claim_list = NULL;
}

struct claim_data *add_claim(zone_vnum z, clan_vnum c, long p_id)
{
  struct claim_data *this_claim = NULL, *i = NULL, *j = NULL;
  int k;

  if (claim_list == NULL)
  {
    claim_list = new_claim();
    this_claim = claim_list;
  }
  else
  {
    /* Sort numerically at add-time */
    for (i = claim_list; i && (CLAIM_ZONE(i) < z); i = i->next)
      j = i;

    if (i && z == CLAIM_ZONE(i))
    {
      /* Claim data already exists for this zone - don't add, just set new vars */
      CLAIM_CLAN(i) = c;
      CLAIM_CLAIMANT(i) = p_id;
      return i;
    }

    /* Doesn't exist, make a new one */
    this_claim = new_claim();
    if (j)
    {
      /* Insert at the correct point in the list */
      this_claim->next = j->next;
      j->next = this_claim;
    }
    else
    {
      /* Insert at the start of the list */
      this_claim->next = claim_list;
      claim_list = this_claim;
    }
  }
  CLAIM_ZONE(this_claim) = z;
  CLAIM_CLAN(this_claim) = c;
  CLAIM_CLAIMANT(this_claim) = p_id;
  for (k = 0; k < MAX_CLANS; k++)
    this_claim->popularity[k] = 0;

  return (this_claim);
}

struct claim_data *add_no_claim(zone_vnum z)
{
  return (add_claim(z, NO_CLAN, 0));
}

struct claim_data *add_claim_by_char(struct char_data *ch, zone_vnum z)
{
  if (!ch)
    return NULL;
  if (IS_NPC(ch))
    return NULL;

  return (add_claim(z, GET_CLAN(ch), GET_IDNUM(ch)));
}

void show_claims(struct char_data *ch, char *arg)
{
  clan_vnum c;
  clan_rnum i;
  int cc = 0;

  if (arg && *arg)
    c = atoi(arg);
  else
    c = NO_CLAN;

  if (c == NO_CLAN)
  {
    send_to_char(ch, "Listing all zone claims, by clan:\r\n");
    // Show all clans
    for (i = 0; i < num_of_clans; i++)
    {
      show_clan_claims(ch, clan_list[i].vnum);
      cc++;
    }
    send_to_char(ch, "%d clan%s listed\r\n", cc, (cc == 1) ? "" : "s");
  }
  else if (c == 0)
  {
    send_to_char(ch, "Usage: %sshow claims [clan ID]%s\r\n", QYEL, QNRM);
  }
  else
  {
    if (real_clan(c) == NO_CLAN)
    {
      send_to_char(ch, "Unrecognised clan ID, type %sclan info%s for "
                       "a list.\r\n",
                   QYEL, QNRM);
    }
    else
    {
      // Show just one clan
      show_clan_claims(ch, c);
    }
  }
}

void show_clan_claims(struct char_data *ch, clan_vnum c)
{
  struct claim_data *this_claim;
  int count = 0;
  zone_rnum zn;
  clan_rnum cn;

  if (!ch)
    return;
  if ((cn = real_clan(c)) == NO_CLAN)
    return;

  send_to_char(ch, "Zones owned by %s%s\r\n", clan_list[cn].clan_name, QNRM);

  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (CLAIM_CLAN(this_claim) == c)
    {
      count++;
      zn = real_zone(CLAIM_ZONE(this_claim));
      if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
      {
        send_to_char(ch, "%s[%s%3d%s]%s %-30s%s Claimed by %s\r\n", QCYN, QYEL,
                     zone_table[zn].number, QCYN, QNRM, zone_table[zn].name, QNRM,
                     get_name_by_id(CLAIM_CLAIMANT(this_claim)));
      }
      else
      {
        send_to_char(ch, "%-30s%s Claimed by %s\r\n", zone_table[zn].name,
                     QNRM, get_name_by_id(CLAIM_CLAIMANT(this_claim)));
      }
    }
  }
  if (count == 0)
    send_to_char(ch, " - %sNone!%s - \r\n", QYEL, QNRM);
}

clan_vnum get_owning_clan(zone_vnum z)
{
  struct claim_data *this_claim;
  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (CLAIM_ZONE(this_claim) == z)
    {
      return (CLAIM_CLAN(this_claim));
    }
  }
  return NO_CLAN;
}

long get_claimant_id(zone_vnum z)
{
  struct claim_data *this_claim;
  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (CLAIM_ZONE(this_claim) == z)
    {
      return (CLAIM_CLAIMANT(this_claim));
    }
  }
  return 0L;
}

/*************************************************************************
 End of Zone Claim code - Start of Claim Popularity code
 ************************************************************************/
float get_popularity(zone_vnum zn, clan_vnum cn)
{
  struct claim_data *this_claim = NULL, *found_claim = NULL;
  clan_rnum c_r;

  if (real_zone(zn) == NOWHERE)
  {
    log("(CLAIMS) Invalid zone vnum %d passed to get_popularity.", zn);
    return (0.0);
  }

  for (this_claim = claim_list; this_claim && !found_claim;
       this_claim = this_claim->next)
  {
    if (this_claim->zn == zn)
    {
      found_claim = this_claim;
    }
  }
  /* Doesn't exist for this zone?  Must be zero */
  if (!found_claim)
    return (0.0);

  if ((c_r = real_clan(cn)) == NO_CLAN)
  {
    log("(CLAIMS) get_popularity failed for clan %d, due to clan not found!",
        cn);
    return (0.0);
  }

  return (found_claim->popularity[c_r]);
}

void increase_popularity(zone_vnum zn, clan_vnum cn, float amt)
{
  int i, i_rand, j, count = 0;
  float tot_vals = 0.0, share_vals, new_val = 0.0, val_diff, vals[MAX_CLANS];
  struct claim_data *this_claim = NULL, *found_claim = NULL;
  clan_rnum c_r;

  if (real_zone(zn) == NOWHERE)
  {
    log("(CLAIMS) Invalid zone vnum %d passed to increase_popularity.", zn);
    return;
  }

  for (this_claim = claim_list; this_claim && !found_claim;
       this_claim = this_claim->next)
  {
    if (this_claim->zn == zn)
    {
      found_claim = this_claim;
    }
  }
  /* Doesn't exist for this zone?  Create the data */
  if (!found_claim)
    found_claim = add_no_claim(zn);

  if (!found_claim)
  {
    log("(CLAIMS) Increase popularity failed for zone %d, due to zone "
        "not found!",
        zn);
    return;
  }

  if ((c_r = real_clan(cn)) == NO_CLAN)
  {
    log("(CLAIMS) Increase popularity failed for clan %d, due to clan "
        "not found!",
        cn);
    return;
  }

  /* 100% is the max */
  if ((found_claim->popularity[c_r]) >= MAX_POPULARITY)
    return;

  for (i = 0; i < MAX_CLANS; i++)
  {
    if ((found_claim->popularity[i] > 0) && (i != c_r))
      tot_vals += found_claim->popularity[i];
  }

  found_claim->popularity[c_r] =
      FLOATMIN(FLOATMAX(found_claim->popularity[c_r] + amt, 0.0),
               MAX_POPULARITY);

  /* Check this doesn't exceed the max allowed */
  if (found_claim->popularity[c_r] + tot_vals > MAX_POPULARITY)
  {
    /* it does, so share out the points */
    share_vals = MAX_POPULARITY - found_claim->popularity[c_r];
    for (i = 0; i < MAX_CLANS; i++)
    {
      if ((found_claim->popularity[i] > 0) && (i != c_r))
      {
        vals[i] = (found_claim->popularity[i] * share_vals) / tot_vals;
      }
      else
      {
        vals[i] = 0;
      }
    }
    /* Add up 'vals' */
    for (j = 0; j < MAX_CLANS; j++)
      new_val += vals[j];

    /* Any spare 'points', add then to random popularity values, but 
     * do not exceed the original value - give up after 50 passes*/
    if ((val_diff = MAX_POPULARITY -
                    (found_claim->popularity[c_r] + new_val)) > 0)
    {
      while (val_diff > 0 && count < 50)
      {
        i_rand = rand_number(1, MAX_CLANS);
        if (i_rand != c_r && vals[i_rand] > 0 && vals[i_rand] < found_claim->popularity[i_rand])
        {
          vals[i_rand] += 0.1;
          val_diff -= 0.1;
        }
        count++;
      } /* end while */
    }   /* end if (spare points) */
    /* Copy it back */
    for (i = 0; i < MAX_CLANS; i++)
    {
      if (i != c_r)
      {
        found_claim->popularity[i] = vals[i];
      }
    } /* end for */
  }   /* end if (exceed max allowed) */

  save_claims();
}

void show_zone_popularities(struct char_data *ch,
                            struct claim_data *this_claim)
{
  zone_rnum z_r;
  int i, j, numbars;
  float tot = 0.0;
  char bar[14];

  if (!this_claim)
    return;

  if ((z_r = real_zone(this_claim->zn)) != NOWHERE)
  {
    send_to_char(ch, "%sPopularity for %s%s%s\r\n", QBWHT, QNRM,
                 zone_table[z_r].name, QNRM);
    for (i = 0; i < MAX_CLANS; i++)
    {
      if (this_claim->popularity[i] > 0.0)
      {
        numbars = ((int)(this_claim->popularity[i])) / 5;
        for (j = 0; j < 12; j++)
        {
          if (j < numbars)
            bar[j] = '*';
          else
            bar[j] = ' ';
        }
        bar[12] = '\0';
        send_to_char(ch, "%-25s%s: %3.1f%% %s[%s%s%s]%s\r\n",
                     clan_list[i].clan_name, QNRM, this_claim->popularity[i], QCYN,
                     QYEL, bar, QCYN, QNRM);
        tot += this_claim->popularity[i];
      }
    }
  }
  else
  {
    log("Zone %d returned rnum %d in show_zone_popularities",
        this_claim->zn, z_r);
  }
}

void show_clan_popularities(struct char_data *ch, clan_vnum c_v)
{
  zone_rnum z_r;
  clan_rnum c_r;
  struct claim_data *this_claim;
  int j, cz = 0, numbars;
  char bar[14];

  if ((c_r = real_clan(c_v)) == NO_CLAN)
    return;

  send_to_char(ch, "%sPopularity for %s%s%s\r\n", QBWHT, QNRM,
               clan_list[c_r].clan_name, QNRM);
  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if ((z_r = real_zone(this_claim->zn)) != NOWHERE)
    {
      if (this_claim->popularity[c_r] > 0.0)
      {
        numbars = ((int)(this_claim->popularity[c_r])) / 5;
        for (j = 0; j < 12; j++)
        {
          if (j < numbars)
            bar[j] = '*';
          else
            bar[j] = ' ';
        }
        bar[12] = '\0';
        if (GET_LEVEL(ch) >= LVL_IMMORT && PRF_FLAGGED(ch, PRF_SHOWVNUMS))
        {
          send_to_char(ch, "%s[%s%3d%s]%s %-30s%s: %3.2f%% %s[%s%s%s]%s\r\n",
                       QCYN, QYEL, this_claim->zn, QCYN, QNRM, zone_table[z_r].name,
                       QNRM, this_claim->popularity[c_r], QCYN, QYEL, bar,
                       QCYN, QNRM);
        }
        else
        {
          send_to_char(ch, "%-30s%s: %3.2f%% %s[%s%s%s]%s\r\n",
                       zone_table[z_r].name, QNRM, this_claim->popularity[c_r],
                       QCYN, QYEL, bar, QCYN, QNRM);
        }
        cz++;
      }
    }
  }
  if (cz == 0)
  {
    send_to_char(ch, "This clan has no popularity in any zone yet.\r\n");
  }
  else
  {
    send_to_char(ch, "This clan has gained some popularity in %d zone%s.\r\n",
                 cz, (cz == 1) ? "" : "s");
  }
}

void show_popularity(struct char_data *ch, char *arg)
{
  zone_vnum c;
  int cz = 0;
  struct claim_data *this_claim;

  if (arg && *arg)
    c = atoi(arg);
  else
    c = NOWHERE;

  if (c == NOWHERE)
  {
    send_to_char(ch, "Listing all zone claim popularity, by zone:\r\n");
    // Show all claim popularities
    for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
    {
      log("this_claim->zn=%d", this_claim->zn);
      show_zone_popularities(ch, this_claim);
      cz++;
    }
    send_to_char(ch, "%d zone%s listed\r\n", cz, (cz == 1) ? "" : "s");
  }
  else if (c == 0 && *arg != '0')
  {
    send_to_char(ch, "Usage: %sshow popularity [zone vnum]%s\r\n",
                 QYEL, QNRM);
  }
  else
  {
    if (real_zone(c) == NOWHERE)
    {
      send_to_char(ch, "Unrecognised zone, type %sshow zones%s for a"
                       " list.\r\n",
                   QYEL, QNRM);
    }
    else
    {
      // Show just one clan
      for (this_claim = claim_list; this_claim;
           this_claim = this_claim->next)
      {
        if (this_claim->zn == c)
          show_zone_popularities(ch, this_claim);
      }
    }
  }
}

/* Should be called each tick, by the heartbeat function in comm.c */
void check_diplomacy(void)
{
  struct descriptor_data *d;

  /* Reduce all diplomacy timers where necessary */
  for (d = descriptor_list; d; d = d->next)
    if (d && IS_PLAYING(d))
      if (d->character && !IS_NPC(d->character))
        if (GET_DIPTIMER(d->character))
          GET_DIPTIMER(d->character)
          --;
}

/***********************************************************************
 End of claim popularity code - Start of Clanset Imm command code
 **********************************************************************/
/* Same as defines in act.wizard.c for the 'set' command */
#define MISC 0
#define BINARY 1
#define NUMBER 2

ACMD(do_clanset)
{
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH];
  char val_arg[MAX_INPUT_LENGTH], rankname[MAX_INPUT_LENGTH],
      rankbuf[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char spellname[MAX_INPUT_LENGTH], spellbuf[MAX_INPUT_LENGTH];
  int value = 0, rankid, i, l;
  int clannum = -1; /* The 'real' number of the clan */
  int spellid, spellnum;

  const struct clanset_struct
  {
    const char *cmd;
    sh_int level;
    char type;
  } fields[] = {
      {"name", LVL_STAFF, MISC},         /* 0  */
      {"numranks", LVL_STAFF, NUMBER},   /* 1  */
      {"rankname", LVL_STAFF, MISC},     /* 2  */
      {"treasure", LVL_GRSTAFF, NUMBER}, /* 3  */
      {"clanhall", LVL_IMPL, NUMBER},    /* 4  */
      {"applev", LVL_STAFF, NUMBER},     /* 5  */
      {"appfee", LVL_STAFF, NUMBER},     /* 6  */
      {"tax", LVL_STAFF, NUMBER},        /* 7  */
      {"skills", LVL_STAFF, NUMBER},     /* 8  */
      {"desc", LVL_STAFF, MISC},         /* 9  */
      {"atwar", LVL_STAFF, NUMBER},      /* 10 */
      {"allied", LVL_STAFF, NUMBER},     /* 11 */
      {"wartimer", LVL_GRSTAFF, NUMBER}, /* 12 */
      {"pkwin", LVL_IMPL, NUMBER},       /* 13 */
      {"pklose", LVL_IMPL, NUMBER},      /* 14 */
      {"raided", LVL_IMPL, NUMBER},      /* 15 */
      {"abbreviation", LVL_STAFF, MISC}, /* 16  */
      {"\n", 0, MISC}};

  /* Get the clan ID */
  half_chop_c(argument, name, sizeof(name), buf, sizeof(buf));

  if (!strcmp(name, "save"))
  {
    /* save all the clan info back into the clan files */
    save_clans();
    save_claims();
    send_to_char(ch, "Clan and claims information saved!\r\n");
    return;
  }

  /* Get the 'field' name */
  half_chop(buf, field, buf);

  strlcpy(val_arg, buf, sizeof(val_arg));

  if (!*name || !*field)
  {
    send_to_char(ch, "%sUsage: %sclanset <clan id> <field> <value>\r\n",
                 QBGRN, QBYEL);
    send_to_char(ch, "       clanset <clan id> %srankname%s <rank id> "
                     "<value>\r\n",
                 QBRED, QBYEL);
    send_to_char(ch, "       clanset save\r\n\r\n");
    send_to_char(ch, "%sValid fields:-\r\n", QNRM);
    send_to_char(ch, "%s       name         numranks   %srankname   "
                     "%streasure   clanhall\r\n",
                 QBYEL, QBRED, QBYEL);
    send_to_char(ch, "       applev       appfee     desc       tax"
                     "        raided\r\n");
    send_to_char(ch, "       atwar        allied     wartimer   pkwin"
                     "      pklose\r\n");
    send_to_char(ch, "       abbreviation\r\n");
    send_to_char(ch, "%sFields shown in %sred%s use a special format shown"
                     " above\r\n",
                 QNRM, QBRED, QNRM);
    return;
  }

  value = atoi(name);

  /* Check the clan exists */
  if ((clannum = real_clan(value)) == NO_CLAN)
  {
    send_to_char(ch, "Invalid clan ID - this clan does not exist!\r\n");
    return;
  }

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  /* Was the end of the list reached? */
  if (*(fields[l].cmd) == '\n')
  {
  }

  if (GET_LEVEL(ch) < fields[l].level)
  {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }

  if (fields[l].type == NUMBER)
  {
    value = atoi(val_arg);
  }

  mudlog(CMP, LVL_IMPL, TRUE, "CLANSET: clan=%d, field=%d (%s), "
                              "val_arg=%s (by %s)",
         clannum, l, fields[l].cmd, val_arg,
         GET_NAME(ch));

  strlcpy(buf, "Okay.", sizeof(buf)); /* can't use OK macro here 'cause of \r\n */

  switch (l)
  {
  case 0: /* clanset <clannum> name <val_arg>*/
    if (strlen(val_arg) >= MAX_CLAN_NAME)
    {
      send_to_char(ch, "Clan name too long! (%d characters max)",
                   MAX_CLAN_NAME);
      return;
    }
    if (CLAN_NAME(clannum))
      free(CLAN_NAME(clannum));
    CLAN_NAME(clannum) = strdup(val_arg);
    snprintf(buf, sizeof(buf), "Clan ID %d: Name is now: %s%s", clan_list[clannum].vnum,
            CLAN_NAME(clannum), QNRM);
    break;

  case 1: /* clanset <clannum> numranks <value>*/
    if ((value < 1) || (value > MAX_CLANRANKS))
    {
      send_to_char(ch, "You MUST have between 1 and %d ranks in a clan!",
                   MAX_CLANRANKS);
      return;
    }
    clan_list[clannum].ranks = value;
    /* Erase all the old rank names that are now outside the range (if any) */
    for (i = value; i < MAX_CLANRANKS; i++)
    {
      if (clan_list[clannum].rank_name[i])
      {
        free(clan_list[clannum].rank_name[i]);
        clan_list[clannum].rank_name[i] = NULL;
      }
    }
    snprintf(buf, sizeof(buf), "Clan ID %d: Number of ranks set to %d%s",
            clan_list[clannum].vnum, clan_list[clannum].ranks, QNRM);
    break;

    /* clanset <clannum> rankname <rank id> <val_arg>*/
  case 2:
    /* Val arg would be the rank ID, and the title - seperate them */
    half_chop(val_arg, rankbuf, rankname);
    rankid = atoi(rankbuf);
    if ((rankid < 1) || (rankid > clan_list[clannum].ranks))
    {
      send_to_char(ch, "Invalid rank ID number - Try again!");
      return;
    }
    if (strlen(rankname) > MAX_CLAN_NAME)
    {
      send_to_char(ch, "Clan rankname too long! (%d characters max)",
                   MAX_CLAN_NAME);
      return;
    }
    if (clan_list[clannum].rank_name[rankid - 1])
      free(clan_list[clannum].rank_name[rankid - 1]);
    clan_list[clannum].rank_name[rankid - 1] = strdup(rankname);

    snprintf(buf, sizeof(buf), "Clan ID %d: Rank %d changed to \"%s%s\"",
            clan_list[clannum].vnum, rankid, rankname, QNRM);
    break;
  case 3: /* clanset clannum treasure <value>*/
    if ((value < 0) || (value > MAX_GOLD))
    {
      send_to_char(ch, "Clan treasure cannot be greater than %s\r\n",
                   add_commas(MAX_GOLD));
      return;
    }
    clan_list[clannum].treasure = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: Treasure (clan bank) set to %s\r\n",
            clan_list[clannum].vnum, add_commas(value));
    break;
  case 4: /* clanset clannum clannhall <value>*/
    if ((value < 0) || (value > 655))
    {
      send_to_char(ch, "Invalid Zone for clanhall (0-655)\r\n");
      return;
    }
    if (real_zone(value) == NOWHERE)
    {
      send_to_char(ch, "That zone does not exist!\r\n");
      return;
    }
    clan_list[clannum].hall = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: Clanhall zone set to %d\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 5: /* clanset clannum applev <value>*/
    if ((value < 1) || (value >= LVL_IMMORT))
    {
      send_to_char(ch, "Invalid clan application level (0-%d)",
                   (LVL_IMMORT - 1));
      return;
    }
    clan_list[clannum].applev = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: Application Level set to %d\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 6: /* clanset clannum appfee <value>*/
    if ((value < 1) || (value > MAX_GOLD))
    {
      send_to_char(ch, "Invalid clan application fee (0 - %s)",
                   add_commas(MAX_GOLD));
      return;
    }
    clan_list[clannum].appfee = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: Application Fee set to %s\r\n",
            clan_list[clannum].vnum, add_commas(value));
    break;
  case 7: /* clanset clannum tax <value>*/
    if ((value < 0) || (value > 40))
    {
      send_to_char(ch, "Invalid clan tax (0%% - 40%%)");
      return;
    }
    clan_list[clannum].taxrate = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: Tax set to %d%%\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 8: /* clanset clannum skills <id> <value>*/
    /* Val arg would be the spell ID, and the spell name - seperate them */
    half_chop(val_arg, spellbuf, spellname);
    spellid = atoi(spellbuf);
    if ((spellid < 1) || (spellid > 5))
    {
      send_to_char(ch, "Invalid clan skill ID number - Try again! (1 to 5)");
      return;
    }
    spellnum = find_skill_num(spellname);
    if (spellnum == -1)
    {
      spellnum = atoi(spellname);
      if (spellnum <= 0)
      {
        send_to_char(ch, "Invalid skill name or skill number");
        return;
      }
    }
    clan_list[clannum].spells[spellid - 1] = spellnum;
    snprintf(buf, sizeof(buf), "Clan ID %d: Skill %d set to %s\r\n", clan_list[clannum].vnum,
            spellid, skill_name(spellnum));
    break;
  case 9: /* clanset clannum desc <val_arg>*/
    if (strlen(val_arg) >= MAX_CLAN_DESC)
    {
      send_to_char(ch, "Clan description too long! (%d characters max)",
                   MAX_CLAN_DESC);
      return;
    }
    if (clan_list[clannum].description)
      free(clan_list[clannum].description);
    clan_list[clannum].description = strdup(val_arg);
    snprintf(buf, sizeof(buf), "Clan ID %d: Clan plan is now: \r\n%s\tn",
            clan_list[clannum].vnum, clan_list[clannum].description);
    break;
  case 10: /* clanset clannum atwar <clan id>*/
    if ((value < 0) || (value > num_of_clans))
    {
      send_to_char(ch, "Invalid at war clan ID (1-%d, or 0=off)",
                   num_of_clans);
      return;
    }
    if (value == 0)
    {
      clan_list[clannum].at_war = NO_CLAN;
      send_to_char(ch, "%s%s is no longer at war.  Peace declared!\r\n",
                   clan_list[clannum].clan_name, QNRM);
    }
    else
    {
      clan_list[clannum].at_war = value;
      send_to_char(ch, "%s%s is now at war with %s%s",
                   clan_list[clannum].clan_name, QNRM,
                   clan_list[real_clan(clan_list[clannum].at_war)].clan_name, QNRM);

      snprintf(buf, sizeof(buf), "Clan ID %d: Enemy Clan set to %s\r\n",
              clan_list[clannum].vnum,
              clan_list[real_clan(clan_list[clannum].at_war)].clan_name);
    }
    break;
  case 11: /* clanset clannum allied <clan id>*/
    if ((value < 0) || (value > num_of_clans))
    {
      send_to_char(ch, "Invalid alliance clan ID (1-%d, or 0=off)",
                   num_of_clans);
      return;
    }
    if (value == 0)
    {
      clan_list[clannum].allied = NO_CLAN;
      send_to_char(ch, "%s%s is no longer allied with any other clan.  "
                       "Alliance broken!\r\n",
                   clan_list[clannum].clan_name, QNRM);
    }
    else
    {
      clan_list[clannum].allied = value;
      send_to_char(ch, "%s%s is now allied with %s%s",
                   clan_list[clannum].clan_name, QNRM,
                   clan_list[real_clan(clan_list[clannum].allied)].clan_name, QNRM);
      snprintf(buf, sizeof(buf), "Clan ID %d: Ally Clan set to %s\r\n",
              clan_list[clannum].vnum,
              clan_list[real_clan(clan_list[clannum].allied)].clan_name);
    }
    break;
  case 12: /* clanset clannum wartimer <value>*/
    if ((value < 0) || (value > 1000))
    {
      send_to_char(ch, "Invalid war timer (0-1000)");
      return;
    }
    clan_list[clannum].war_timer = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: War Timer set to %d ticks.\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 13: /* clanset clannum pkwin <value>*/
    if ((value < 0) || (value > 30000))
    {
      send_to_char(ch, "Invalid number of battles won (0-30000)");
      return;
    }
    clan_list[clannum].pk_win = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: PK Wins set to %d.\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 14: /* clanset clannum pklose <value>*/
    if ((value < 0) || (value > 30000))
    {
      send_to_char(ch, "Invalid number of battles lost(1-30000)");
      return;
    }
    clan_list[clannum].pk_lose = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: PK Losses set to %d.\r\n",
            clan_list[clannum].vnum, value);
    break;
  case 15: /* clanset clannum pklose <value>*/
    if ((value < 0) || (value > 30000))
    {
      send_to_char(ch, "Invalid number of raids(1-30000)");
      return;
    }
    clan_list[clannum].raided = value;
    snprintf(buf, sizeof(buf), "Clan ID %d: PK Raided set to %d.\r\n",
            clan_list[clannum].vnum, value);
    break;

  case 16: /* clanset <clannum> abbrev <val_arg>*/
    if (strlen(val_arg) >= MAX_CLAN_ABREV)
    {
      send_to_char(ch, "Clan abbreviation too long! (%d characters max)",
                   MAX_CLAN_ABREV);
      return;
    }
    if (CLAN_ABREV(clannum))
      free(clan_list[clannum].abrev);
    clan_list[clannum].abrev = strdup(val_arg);
    snprintf(buf, sizeof(buf), "Clan ID %d: ABREV is now: %s%s", clan_list[clannum].vnum,
            CLAN_ABREV(clannum), QNRM);
    break;

  default:
    snprintf(buf, sizeof(buf), "Can't clanset that!");
    break;
  }

  send_to_char(ch, "%s", CAP(buf));
  log("(CLAN) Clanset by %s: %s", GET_NAME(ch), buf);
}

/***************************************************************************
 End of Clanset Imm command code - Start of clantalk communications channel
 **************************************************************************/
ACMD(do_clantalk)
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH],
      arg[MAX_INPUT_LENGTH];
  const char *arg2;
  const char *msg = NULL;
  clan_vnum c_id;
  struct descriptor_data *i;
  bool leader = FALSE, imm = FALSE;
  int c_arg;

  skip_spaces_c(&argument);

  if (GET_LEVEL(ch) > LVL_IMMORT)
    imm = TRUE;

  if (!imm && (GET_CLAN(ch) == 0 || GET_CLAN(ch) == NO_CLAN))
  {
    send_to_char(ch, "You can't use the clan talk channel if your aren't"
                     " in a clan!\r\n");
    return;
  }

  if (!imm && GET_IDNUM(ch) == clan_list[real_clan(GET_CLAN(ch))].leader)
    leader = TRUE;

  if (!imm && !leader && GET_CLANRANK(ch) == NO_CLANRANK)
  {
    send_to_char(ch, "You must be fully enrolled in your clan to"
                     " do that!\r\n");
    return;
  }

  if (imm)
  {
    arg2 = one_argument_c(argument, arg, sizeof(arg));
    if ((c_arg = atoi(arg)) > 0)
    {
      if (real_clan(c_arg) != NO_CLAN)
      {
        c_id = (clan_vnum)c_arg;
        argument = arg2;        /* Skip the arg */
        skip_spaces_c(&argument); /* Strip spaces. */
      }
      else
      {
        send_to_char(ch, "Invalid clan ID.\r\n");
        return;
      }
    }
    else if (IS_IN_CLAN(ch))
    {
      c_id = GET_CLAN(ch);
    }
    else
    {
      send_to_char(ch, "IMM Clantalk: If you aren't in a clan, then you "
                       "must use a clan ID.\r\n");
      send_to_char(ch, "Usage: \tyclantalk <clan vnum> <message>\tn\r\n");
      return;
    }
  }
  else
  {
    c_id = GET_CLAN(ch);
  }

  if (!*argument)
  {
    send_to_char(ch, "What do you want to tell your clan?\r\n");
    return;
  }
  /* Removed the +1 from argument...No idea why they did that. -Ornir */
  snprintf(buf, sizeof(buf), "%s[Clan] \"%s\"%s", QBCYN, (argument), QNRM);
  msg = act(buf, TRUE, ch, 0, ch, TO_VICT);
  add_history(ch, msg, HIST_CLANTALK);

  for (i = descriptor_list; i; i = i->next)
  {
    if (STATE(i) != CON_PLAYING)
      continue;

    if (!i->character)
      continue;

    imm = FALSE;
    if (GET_LEVEL(i->character) >= LVL_STAFF)
      if (!PRF_FLAGGED(i->character, PRF_NOCLANTALK))
        imm = TRUE;

    /* Must be in the same clan (obviously) */
    if (GET_CLAN(i->character) != c_id && !imm)
      continue;

    /* Is this the clan leader? */
    leader = FALSE;
    if (imm)
      leader = TRUE;
    else if (GET_LEVEL(ch) >= LVL_STAFF)
      leader = FALSE;
    else if (GET_CLAN(ch))
      if (GET_IDNUM(i->character) == clan_list[real_clan(GET_CLAN(ch))].leader)
        leader = TRUE;

    /* Only fully enrolled clan members receive clantalk */
    if (!leader && GET_CLANRANK(i->character) == NO_CLANRANK)
      continue;

    //    /* Sleeping clan members miss the message! */
    //    if (!imm && !leader && GET_POS(i->character) == POS_SLEEPING)
    //      continue;

    /* Skip the sender (already sent message to them) */
    if ((i->character) == ch)
      continue;

    /* Again, removed the +1 from argument. */
    snprintf(buf, sizeof(buf), "[Clantalk] %s$n \"%s\"%s",
             CCMAG(i->character, C_NRM),
             (argument), CCNRM(i->character, C_NRM));

    if (imm)
    {
      snprintf(buf2, sizeof(buf2), "[%sClan %d - %s %s] %s",
               CBWHT(i->character, C_NRM), c_id, clan_list[c_id - 1].abrev,
               CBWHT(i->character, C_NRM), buf);
      msg = act(buf2, TRUE, ch, 0, i->character, TO_VICT);
      add_history(i->character, msg, HIST_CLANTALK);
    }
    else
    {
      msg = act(buf, TRUE, ch, 0, i->character, TO_VICT);
      add_history(i->character, msg, HIST_CLANTALK);
    }
  }
}

/* Note - returns clan VNUM or NO_CLAN */
clan_vnum zone_is_clanhall(zone_vnum z)
{
  int i;

  for (i = 0; i < num_of_clans; i++)
  {
    if (clan_list[i].hall == z)
      return (clan_list[i].vnum);
  }
  return NO_CLAN;
}

/* Note - returns zone VNUM or NOWHERE */
zone_vnum get_clanhall_by_char(struct char_data *ch)
{
  clan_vnum c;

  if ((c = GET_CLAN(ch)) == NO_CLAN)
    return NOWHERE;
  if (real_clan(c) == NO_CLAN)
    return NOWHERE;

  return (clan_list[(real_clan(c))].hall);
}

int get_clan_taxrate(struct char_data *ch)
{
  zone_vnum zn;
  clan_vnum cn, shop_cn;
  struct claim_data *this_claim;

  cn = GET_CLAN(ch);
  /* The zone that ch is in */
  zn = zone_table[(world[IN_ROOM(ch)].zone)].number;

  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (CLAIM_ZONE(this_claim) == zn)
    {
      if ((shop_cn = CLAIM_CLAN(this_claim)) == cn)
      {
        return 0;
      }
      else
      {
        if (real_clan(shop_cn) == NO_CLAN)
          return 0;
        return (clan_list[(real_clan(shop_cn))].taxrate);
      }
    }
  }
  return 0;
}

clan_vnum get_owning_clan_by_char(struct char_data *ch)
{
  zone_vnum zn;
  struct claim_data *this_claim;

  /* The zone that ch is in */
  zn = zone_table[(world[IN_ROOM(ch)].zone)].number;

  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    if (CLAIM_ZONE(this_claim) == zn)
    {
      return (CLAIM_CLAN(this_claim));
    }
  }
  return NO_CLAN;
}

void do_clan_tax_losses(struct char_data *ch, int amount)
{
  int tax, loss;
  clan_rnum cn;
  zone_vnum zn;

  if (IS_IN_CLAN(ch))
  {
    if ((tax = get_clan_taxrate(ch)) > 0)
    {
      loss = (amount * tax) / 100;
      if (loss > 0)
      {
        /* The zone that ch is in */
        zn = zone_table[(world[IN_ROOM(ch)].zone)].number;
        cn = real_clan(get_owning_clan(zn));
        decrease_gold(ch, loss);
        send_to_char(ch, "This area is owned by %s.\r\n", CLAN_NAME(cn));
        send_to_char(ch, "You lose %d coins to clan taxes.\r\n", loss);
      }
    }
  }
}

bool check_clanpriv(struct char_data *ch, int p)
{
  clan_rnum cr;
  int rk;

  if (!ch || IS_NPC(ch))
    return FALSE;
  if (GET_LEVEL(ch) == LVL_IMPL)
    return TRUE;
  if (!IS_IN_CLAN(ch))
    return FALSE;
  if ((cr = real_clan(GET_CLAN(ch))) == NO_CLAN)
    return FALSE;
  if (CLAN_LEADER(cr) == GET_IDNUM(ch))
    return TRUE;
  if ((rk = GET_CLANRANK(ch)) == NO_CLANRANK)
    return FALSE;
  if (rk <= CLAN_PRIV(cr, p))
    return TRUE;
  return FALSE;
}

#endif
