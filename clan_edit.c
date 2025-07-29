/****************************************************************************
 *  Realms of Luminari
 *  File:     clan_edit.c
 *  Usage:    clan-related olc features, and saving/loading of clans
 *  Header:   clan.h
 *  Authors:  Jamdog (ported to Luminari by Bakarus and Zusuk)
 ****************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "comm.h"        /* descriptor_list etc */
#include "interpreter.h" /* one_argument() etc */
#include "modify.h"      /* string_write etc */
#include "ibt.h"
#include "clan.h"

/* Static internal (only used in clan_edit.c) functions */
static void clanedit_setup(struct descriptor_data *d);
static void clanedit_save(struct descriptor_data *d);
static void clanedit_disp_menu(struct descriptor_data *d);
static void clanedit_ranks_menu(struct descriptor_data *d);
static void clanedit_priv_menu(struct descriptor_data *d);
static void clanedit_clans_menu(struct descriptor_data *d, int player_clan);
static void get_priv_string(struct descriptor_data *d, char *t, int p);

/*============================================*/
/*======        Saving of Clans       ========*/
/*============================================*/

/* Write clans to the lib/etc/clans file */
void save_clans(void)
{
  FILE *fl;
  int i, j, x;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  if (!(fl = fopen(CLAN_FILE, "w")))
  {
    mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Unable to open clan file");
    return;
  }
  fprintf(fl, "* Clans File\n");
  fprintf(fl, "* Number of clans: %d\n", num_of_clans);
  for (i = 0; i < num_of_clans; i++)
  {
    fprintf(fl, "#%d\n", clan_list[i].vnum);
    fprintf(fl, "Name: %s\n", clan_list[i].clan_name);
    fprintf(fl, "Init: %s\n", clan_list[i].abrev);

    if (clan_list[i].description && *clan_list[i].description)
    {
      strlcpy(buf, clan_list[i].description, sizeof(buf));
      strip_cr(buf);
      fprintf(fl, "Desc:\n%s~\n", buf);
    }

    /* Save the ID of the current clan leader */
    fprintf(fl, "Lder: %ld\n", clan_list[i].leader);

    if (clan_list[i].applev != 0)
      fprintf(fl, "AppL: %d\n", clan_list[i].applev);
    if (clan_list[i].appfee != 0)
      fprintf(fl, "AppF: %d\n", clan_list[i].appfee);
    if (clan_list[i].taxrate != 0)
      fprintf(fl, "Tax : %d\n", clan_list[i].taxrate);
    if (clan_list[i].hall != 0)
      fprintf(fl, "Hall: %d\n", clan_list[i].hall);
    if (clan_list[i].treasure != 0)
      fprintf(fl, "Bank: %ld\n", clan_list[i].treasure);
    fprintf(fl, "Ally:");
    for (x = 0; x < MAX_CLANS; x++)
    {
      fprintf(fl, " %d", clan_list[i].allies[x]);
    }
    fprintf(fl, "\n");
    
    fprintf(fl, "War :");
    for (x = 0; x < MAX_CLANS; x++)
    {
      fprintf(fl, " %d", clan_list[i].at_war[x]);
    }
    fprintf(fl, "\n");
    if (clan_list[i].war_timer != 0)
      fprintf(fl, "WarT: %d\n", clan_list[i].war_timer);
    if (clan_list[i].pk_win != 0)
      fprintf(fl, "PWin: %d\n", clan_list[i].pk_win);
    if (clan_list[i].pk_lose != 0)
      fprintf(fl, "PLos: %d\n", clan_list[i].pk_lose);
    if (clan_list[i].raided != 0)
      fprintf(fl, "Raid: %d\n", clan_list[i].raided);
    if (clan_list[i].last_activity != 0)
      fprintf(fl, "LAct: %ld\n", (long)clan_list[i].last_activity);
    if (clan_list[i].max_members != 50)  /* Only save if not default */
      fprintf(fl, "MaxM: %d\n", clan_list[i].max_members);

    /* Save the rank names */
    if (clan_list[i].ranks > 0)
    {
      fprintf(fl, "Rank:\n");
      for (j = 0; j < clan_list[i].ranks; j++)
      {
        fprintf(fl, "%s\n", clan_list[i].rank_name[j]);
      }
      fprintf(fl, "~\n");
    }

    /* Save the Privilege Levels */
    fprintf(fl, "Priv:\n");
    for (j = 0; j < NUM_CLAN_PRIVS; j++)
    {
      fprintf(fl, "%d %d\n", j, clan_list[i].privilege[j]);
    }
    fprintf(fl, "~\n");
  }
  fprintf(fl, "$\n");
  fclose(fl);
  
  /* Clear all modified flags after successful save */
  for (i = 0; i < num_of_clans; i++)
  {
    clan_list[i].modified = FALSE;
  }
}

/* Save a single clan to a temporary file, then merge with existing clan file */
void save_single_clan(clan_rnum c)
{
  FILE *fl, *new_fl;
  char tmpname[256], backup[256];
  char line[MAX_INPUT_LENGTH + 1];  /* tag[6] unused in this function */
  int j, x, gl, current_clan = -1;
  bool clan_written = FALSE;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  
  if (c < 0 || c >= num_of_clans)
  {
    log("SYSERR: save_single_clan called with invalid clan rnum %d", c);
    return;
  }
  
  if (!clan_list[c].modified)
    return; /* No need to save if not modified */
  
  /* Create temporary file name */
  snprintf(tmpname, sizeof(tmpname), "%s.tmp", CLAN_FILE);
  snprintf(backup, sizeof(backup), "%s.bak", CLAN_FILE);
  
  /* Open original file for reading */
  if (!(fl = fopen(CLAN_FILE, "r")))
  {
    /* If original doesn't exist, do a full save */
    save_clans();
    return;
  }
  
  /* Open temporary file for writing */
  if (!(new_fl = fopen(tmpname, "w")))
  {
    fclose(fl);
    log("SYSERR: Unable to open temporary clan file for writing");
    return;
  }
  
  /* Copy header */
  fprintf(new_fl, "* Clans File\n");
  fprintf(new_fl, "* Number of clans: %d\n", num_of_clans);
  
  /* Read and copy clans, replacing the modified one */
  while ((gl = get_line(fl, line)) && *line != '$')
  {
    /* Skip comment lines */
    if (*line == '*')
      continue;
      
    if (*line == '#')
    {
      current_clan = atoi(line + 1);
      
      /* Check if this is the clan we're updating */
      if (current_clan == clan_list[c].vnum)
      {
        /* Write the updated clan data */
        fprintf(new_fl, "#%d\n", clan_list[c].vnum);
        fprintf(new_fl, "Name: %s\n", clan_list[c].clan_name);
        fprintf(new_fl, "Init: %s\n", clan_list[c].abrev);

        if (clan_list[c].description && *clan_list[c].description)
        {
          strlcpy(buf, clan_list[c].description, sizeof(buf));
          strip_cr(buf);
          fprintf(new_fl, "Desc:\n%s~\n", buf);
        }

        fprintf(new_fl, "Lder: %ld\n", clan_list[c].leader);

        if (clan_list[c].applev != 0)
          fprintf(new_fl, "AppL: %d\n", clan_list[c].applev);
        if (clan_list[c].appfee != 0)
          fprintf(new_fl, "AppF: %d\n", clan_list[c].appfee);
        if (clan_list[c].taxrate != 0)
          fprintf(new_fl, "Tax : %d\n", clan_list[c].taxrate);
        if (clan_list[c].hall != 0)
          fprintf(new_fl, "Hall: %d\n", clan_list[c].hall);
        if (clan_list[c].treasure != 0)
          fprintf(new_fl, "Bank: %ld\n", clan_list[c].treasure);
          
        fprintf(new_fl, "Ally:");
        for (x = 0; x < MAX_CLANS; x++)
          fprintf(new_fl, " %d", clan_list[c].allies[x]);
        fprintf(new_fl, "\n");
        
        fprintf(new_fl, "War :");
        for (x = 0; x < MAX_CLANS; x++)
          fprintf(new_fl, " %d", clan_list[c].at_war[x]);
        fprintf(new_fl, "\n");
        
        if (clan_list[c].war_timer != 0)
          fprintf(new_fl, "WarT: %d\n", clan_list[c].war_timer);
        if (clan_list[c].pk_win != 0)
          fprintf(new_fl, "PWin: %d\n", clan_list[c].pk_win);
        if (clan_list[c].pk_lose != 0)
          fprintf(new_fl, "PLos: %d\n", clan_list[c].pk_lose);
        if (clan_list[c].raided != 0)
          fprintf(new_fl, "Raid: %d\n", clan_list[c].raided);
        if (clan_list[c].last_activity != 0)
          fprintf(new_fl, "LAct: %ld\n", (long)clan_list[c].last_activity);
        if (clan_list[c].max_members != 50)
          fprintf(new_fl, "MaxM: %d\n", clan_list[c].max_members);

        /* Save the rank names */
        if (clan_list[c].ranks > 0)
        {
          fprintf(new_fl, "Rank:\n");
          for (j = 0; j < clan_list[c].ranks; j++)
            fprintf(new_fl, "%s\n", clan_list[c].rank_name[j]);
          fprintf(new_fl, "~\n");
        }

        /* Save the Privilege Levels */
        fprintf(new_fl, "Priv:\n");
        for (j = 0; j < NUM_CLAN_PRIVS; j++)
          fprintf(new_fl, "%d %d\n", j, clan_list[c].privilege[j]);
        fprintf(new_fl, "~\n");
        
        clan_written = TRUE;
        
        /* Skip the rest of this clan in the original file */
        while ((gl = get_line(fl, line)) && *line != '#' && *line != '$')
        {
          /* Skip until we find the next clan or end of file */
        }
        
        /* If we found another clan, we need to process it */
        if (*line == '#')
        {
          fprintf(new_fl, "%s\n", line);
          current_clan = atoi(line + 1);
        }
        else if (*line == '$')
        {
          /* End of file marker */
          break;
        }
      }
      else
      {
        /* Copy this clan as-is */
        fprintf(new_fl, "%s\n", line);
      }
    }
    else
    {
      /* Copy other lines as-is */
      fprintf(new_fl, "%s\n", line);
    }
  }
  
  fprintf(new_fl, "$\n");
  
  fclose(fl);
  fclose(new_fl);
  
  /* Now replace the original file with the new one */
  /* First, backup the original */
  remove(backup);
  rename(CLAN_FILE, backup);
  
  /* Then rename the temporary file */
  if (rename(tmpname, CLAN_FILE) != 0)
  {
    log("SYSERR: Couldn't rename temporary clan file, restoring backup");
    rename(backup, CLAN_FILE);
    remove(tmpname);
    return;
  }
  
  /* Success - remove backup and clear modified flag */
  remove(backup);
  clan_list[c].modified = FALSE;
}

/* Mark a clan as needing to be saved */
void mark_clan_modified(clan_rnum c)
{
  if (c >= 0 && c < num_of_clans)
  {
    clan_list[c].modified = TRUE;
  }
}

/* Read clans from the lib/etc/clans file */
void load_clans(void)
{
  FILE *fl;
  int j, gl = 0, priv, lev;
  char tag[6], line[MAX_INPUT_LENGTH + 1], buf[MAX_STRING_LENGTH] = {'\0'};
  struct clan_data c;

  c.vnum = 0;
  free_clan_list();
  init_clan_hash();  /* Initialize hash table for fast lookups */
  clear_clan_vals(&c);

  if (!(fl = fopen(CLAN_FILE, "r")))
  {
    log("   Clan file does not exist. Will create a new one");
    save_clans();
    return;
  }
  else
  {
    while ((gl = get_line(fl, line)) && *line != '$')
    {
      while (*line == '*')
        gl = get_line(fl, line);
      if (*line == '#')
      {
        if (c.vnum)
        {
          add_clan(&c);
          clear_clan_vals(&c);
        }
        if (num_of_clans >= MAX_CLANS)
        {
          log("SYSERR: Too many clans found in clans file (Max: %d)",
              MAX_CLANS);
          return;
        }
        c.vnum = atoi(line + 1);
        gl = 0;
      }
      if (gl)
      {
        tag_argument(line, tag);

        switch (*tag)
        {
        case 'A':
          if (!strcmp(tag, "AppL"))
            c.applev = atoi(line);
          else if (!strcmp(tag, "AppF"))
            c.appfee = atoi(line);
          else if (!strcmp(tag, "Ally"))
          {
            if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                &c.allies[0], &c.allies[1], &c.allies[2], &c.allies[3], &c.allies[4], 
                &c.allies[5], &c.allies[6], &c.allies[7], &c.allies[8], &c.allies[9], 
                &c.allies[10], &c.allies[11], &c.allies[12], &c.allies[13], &c.allies[14], 
                &c.allies[15], &c.allies[16], &c.allies[17], &c.allies[18], &c.allies[19], 
                &c.allies[20], &c.allies[21], &c.allies[22], &c.allies[23], &c.allies[24]) != 25)
            {
              log("SYSERR: Unknown Ally tag format in clan file %s",CLAN_FILE);  
            }
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'B':
          if (!strcmp(tag, "Bank"))
            c.treasure = atol(line);
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'D':
          if (!strcmp(tag, "Desc"))
          {
            if (c.description)
            {
              free(c.description);
            }
            c.description = fread_string(fl, buf);
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'I':
          if (!strcmp(tag, "Init"))
          {
            if (c.abrev)
            {
              free(c.abrev);
            }
            c.abrev = strdup(line);
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'H':
          if (!strcmp(tag, "Hall"))
            c.hall = atoi(line);
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'L':
          if (!strcmp(tag, "Lder"))
            c.leader = atol(line);
          else if (!strcmp(tag, "LAct"))
            c.last_activity = (time_t)atol(line);
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'M':
          if (!strcmp(tag, "MaxM"))
            c.max_members = atoi(line);
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'N':
          if (!strcmp(tag, "Name"))
          {
            if (c.clan_name)
            {
              free(c.clan_name);
            }
            c.clan_name = strdup(line);
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'P':
          if (!strcmp(tag, "PLos"))
            c.pk_lose = atoi(line);
          else if (!strcmp(tag, "PWin"))
            c.pk_win = atoi(line);
          else if (!strcmp(tag, "Priv"))
          {
            j = 0;
            get_line(fl, line);
            while (*line != '~')
            {
              if (j >= NUM_CLAN_PRIVS)
              {
                log("SYSERR: Too many privs in clan file (clan ID: %d, "
                    "rank line %d)",
                    c.vnum, j + 1);
              }
              else
              {
                sscanf(line, "%d %d", &priv, &lev);
                if (priv >= 21 || priv < 0)
                {
                  log("SYSERR: Invalid priv in clan file (clan ID: %d, "
                      "rank line %d, val=%d)",
                      c.vnum, j + 1, priv);
                }
                else
                {
                  c.privilege[priv] = lev;
                }
              }
              get_line(fl, line);
              j++;
            }
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'R':
          if (!strcmp(tag, "Raid"))
            c.raided = atoi(line);
          else if (!strcmp(tag, "Rank"))
          {
            j = 0;
            get_line(fl, line);
            while (*line != '~')
            {
              if (j >= 20)
              {
                log("SYSERR: Too many ranks in clan file (clan ID: %d,"
                    " rank line %d)",
                    c.vnum, j + 1);
              }
              else
              {
                if (c.rank_name[j])
                  free(c.rank_name[j]);
                c.rank_name[j] = strdup(line);
                j++;
              }
              get_line(fl, line);
            }
            c.ranks = j;
          }
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        case 'T':
          if (!strcmp(tag, "Tax "))
            c.taxrate = atoi(line);
          break;

        case 'W':
          if (!strcmp(tag, "War "))
          {
            if (sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
                &c.at_war[0], &c.at_war[1], &c.at_war[2], &c.at_war[3], &c.at_war[4], 
                &c.at_war[5], &c.at_war[6], &c.at_war[7], &c.at_war[8], &c.at_war[9], 
                &c.at_war[10], &c.at_war[11], &c.at_war[12], &c.at_war[13], &c.at_war[14], 
                &c.at_war[15], &c.at_war[16], &c.at_war[17], &c.at_war[18], &c.at_war[19], 
                &c.at_war[20], &c.at_war[21], &c.at_war[22], &c.at_war[23], &c.at_war[24]) != 25)
            {
              log("SYSERR: Unknown War tag format in clan file %s",CLAN_FILE);  
            }
          }
          else if (!strcmp(tag, "WarT"))
            c.war_timer = atoi(line);
          else
            log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;

        default:
          log("SYSERR: Unknown tag %s in clan file %s", tag, CLAN_FILE);
          break;
        } /* end switch tag */
      }   /* end if (gl) */
    }     /* end while get_line */
    /* if there is a clan pending, add it */
    if (c.vnum)
      add_clan(&c);
    fclose(fl);
  } /* end else */
}

bool save_claims(void)
{
  FILE *fl;
  int i;
  struct claim_data *this_claim;

  if (!(fl = fopen(CLAIMS_FILE, "w")))
  {
    mudlog(CMP, LVL_IMPL, TRUE, "SYSERR: Unable to open claims file");
    return FALSE;
  }
  fprintf(fl, "* Clan Zone Claims File\n");
  fprintf(fl, "* Number of clans: %d\n", num_of_clans);
  for (this_claim = claim_list; this_claim; this_claim = this_claim->next)
  {
    fprintf(fl, "#%d\n", this_claim->zn);

    if (this_claim->clan != NO_CLAN)
      fprintf(fl, "Clan: %d\n", this_claim->clan);
    if (this_claim->claimant != 0)
      fprintf(fl, "Clmt: %ld\n", this_claim->claimant);

    fprintf(fl, "Popu:\n");
    for (i = 0; i < MAX_CLANS; i++)
    {
      if (this_claim->popularity[i] > 0)
        fprintf(fl, "%d %f\n", i, this_claim->popularity[i]);
    }
    fprintf(fl, "~\n");
  }
  fprintf(fl, "$\n");
  fclose(fl);
  return TRUE;
}

void load_claims(void)
{
  FILE *fl;
  int i, j, gl = 0, cn;
  float pop;
  char tag[6], line[MAX_INPUT_LENGTH + 1];
  struct claim_data c, *newc = NULL;

  c.zn = NOWHERE;
  c.clan = NO_CLAN;
  c.claimant = 0;
  for (i = 0; i < MAX_CLANS; i++)
    c.popularity[i] = 0;
  free_claim_list();

  if (!(fl = fopen(CLAIMS_FILE, "r")))
  {
    log("   Clan file does not exist. Will create a new one");
    save_clans();
    return;
  }
  else
  {
    while ((gl = get_line(fl, line)) && *line != '$')
    {
      while (*line == '*')
        gl = get_line(fl, line);
      if (*line == '#')
      {
        if (c.zn != NOWHERE)
        {
          if ((newc = add_claim(c.zn, c.clan, c.claimant)) != NULL)
          {
            for (i = 0; i < MAX_CLANS; i++)
            {
              newc->popularity[i] = c.popularity[i];
              c.popularity[i] = 0;
            }
            c.clan = NO_CLAN;
            c.claimant = 0;
          }
        }
        c.zn = atoi(line + 1);
        gl = 0;
      }
      if (gl)
      {
        tag_argument(line, tag);

        switch (*tag)
        {
        case 'C':
          if (!strcmp(tag, "Clan"))
            c.clan = atoi(line);
          else if (!strcmp(tag, "Clmt"))
            c.claimant = atol(line);
          else
            log("SYSERR: Unknown tag %s in claims file %s",
                tag, CLAIMS_FILE);
          break;

        case 'P':
          if (!strcmp(tag, "Popu"))
          {
            j = 0;
            get_line(fl, line);
            while (*line != '~')
            {
              if (j >= MAX_CLANS)
              {
                log("SYSERR: Too many popularity values in claims file "
                    "(zone ID: %d, popularity line %d)",
                    c.zn, j + 1);
              }
              else
              {
                sscanf(line, "%d %f", &cn, &pop);
                if (pop >= 100.0 || pop < 0.0)
                {
                  log("SYSERR: Invalid popularity value in claims file "
                      "(zone ID: %d, popularity line %d, clan=%d, "
                      "val=%f)",
                      c.zn, j + 1, cn, pop);
                }
                else
                {
                  c.popularity[cn] = pop;
                }
              }
              get_line(fl, line);
              j++;
            }
          }
          else
            log("SYSERR: Unknown tag %s in claims file %s", tag,
                CLAIMS_FILE);
          break;

        default:
          log("SYSERR: Unknown tag %s in claims file %s", tag, CLAIMS_FILE);
          break;
        } /* end switch tag */
      }   /* end if (gl) */
    }     /* end while get_line */
    /* if there is a claim pending, add it */
    if (c.zn != NOWHERE)
    {
      if ((newc = add_claim(c.zn, c.clan, c.claimant)) != NULL)
      {
        for (i = 0; i < MAX_CLANS; i++)
        {
          newc->popularity[i] = c.popularity[i];
          c.popularity[i] = 0;
        }
      }
    }
    fclose(fl);
  } /* end else */
}

/*============================================*/
/*======       Clan Editting (olc)    ========*/
/*============================================*/

/* The OLC for clan leaders or members with permissions and Imps only */
ACMD(do_clanedit)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int c_id = 0;
  clan_rnum cr;
  struct descriptor_data *d;

  one_argument(argument, arg, sizeof(arg));

  /***************************************************************************/
  /** Work out which clan is being edited.                                  **/
  /***************************************************************************/
  if ((!IS_IN_CLAN(ch)) || GET_CLANRANK(ch) == NO_CLANRANK)
  {
    /* No clan set, only Imps can edit, and they MUST specify a clan ID */
    if (GET_LEVEL(ch) < LVL_IMPL)
    {
      send_to_char(ch, "You aren't in a clan, so you can't edit one!\r\n");
      return;
    }
    if (!*arg)
    {
      send_to_char(ch, "Usage: %sclan edit <clan ID>%s!\r\n", QYEL, QNRM);
      return;
    }
    c_id = atoi(arg);
  }
  else
  {
    if (GET_LEVEL(ch) == LVL_IMPL)
    {
      if (*arg)
      {
        c_id = atoi(arg);
      }
      else
      {
        c_id = GET_CLAN(ch);
      }
    }
    else
    {
      c_id = GET_CLAN(ch);
    }
  }

  if ((cr = real_clan(c_id)) == NO_CLAN)
  {
    send_to_char(ch, "Invalid clan ID!\r\n");
    return;
  }

  if (!check_clanpriv(ch, CP_CLANEDIT))
  {
    send_to_char(ch, "You don't have sufficient access to edit a clan!\r\n");
    return;
  }

  /***************************************************************************/
  /** Check that the clan isn't already being edited.                       **/
  /***************************************************************************/
  for (d = descriptor_list; d; d = d->next)
  {
    if (STATE(d) == CON_CLANEDIT)
    {
      if (d->olc && OLC_NUM(d) == c_id)
      {
        send_to_char(ch, "The clan is currently being edited by %s.\r\n",
                     PERS(d->character, ch));
        return;
      }
    }
  }

  /***************************************************************************/
  /** Point d to the editor's descriptor.                                   **/
  /***************************************************************************/
  d = ch->desc;

  /***************************************************************************/
  /** Give the descriptor an OLC structure.                                 **/
  /***************************************************************************/
  if (d->olc)
  {
    mudlog(BRF, LVL_IMMORT, TRUE,
           "SYSERR: do_clanedit: Player already had olc structure.");
    free(d->olc);
  }

  CREATE(d->olc, struct oasis_olc_data, 1);

  /***************************************************************************/
  /** Check player has edit permissions for this clan, or is an IMPL        **/
  /***************************************************************************/
  if (!can_edit_clan(ch, c_id))
  {
    send_to_char(ch, "You do not have permission to edit this clan.\r\n");

    /*************************************************************************/
    /** Free the OLC structure.                                             **/
    /*************************************************************************/
    free(d->olc);
    d->olc = NULL;
    return;
  }

  OLC_NUM(d) = c_id;

  /* Only existing clan can be edited, so no setup_new/setup_existing */
  clanedit_setup(d);

  STATE(d) = CON_CLANEDIT;

  act("$n starts using Clan edit.", TRUE, d->character, 0, 0, TO_ROOM);
  SET_BIT_AR(PLR_FLAGS(ch), PLR_WRITING);

  mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
         "OLC: %s starts editing clan %d (%s)",
         GET_NAME(ch), OLC_NUM(d), CLAN_NAME(cr));
}

/****************************************************************************
 End of Claim Popularity code - Start of Clan Edit OLC code
 ***************************************************************************/
static void clanedit_setup(struct descriptor_data *d)
{
  struct clan_data *nw_cln;
  clan_rnum c;

  c = real_clan(OLC_NUM(d));

  CREATE(nw_cln, struct clan_data, 1);

  clear_clan_vals(nw_cln);
  duplicate_clan_data(nw_cln, &(clan_list[c]));

  OLC_CLAN(d) = nw_cln;
  OLC_CLAN(d)->vnum = OLC_NUM(d);
  OLC_VAL(d) = 0;

  /* Show the main clan edit menu                              */
  clanedit_disp_menu(d);
}

static void clanedit_save(struct descriptor_data *d)
{
  clan_rnum cr;

  if ((cr = real_clan(OLC_CLAN(d)->vnum)) == NO_CLAN)
  {
    log("SYSERR: clanedit_save: Invalid clan vnum (%d) in OLC struct",
        OLC_CLAN(d)->vnum);
    return;
  }

  /* Overwrite old clan's data */
  duplicate_clan_data(&(clan_list[cr]), OLC_CLAN(d));

  mark_clan_modified(cr);
  save_single_clan(cr);
}

static void get_priv_string(struct descriptor_data *d, char *t, int p)
{
  if (OLC_CLAN(d)->privilege[p] > 0)
    sprintf(t, "%d", OLC_CLAN(d)->privilege[p]);
  else
    sprintf(t, "Leader Only");
}

/*-------------------------------------------------------------------*/

/*. Display main menu . */

static void clanedit_disp_menu(struct descriptor_data *d)
{
  int x, xcount = 0;
  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "-- Clan ID     : %s[%s%d%s]%s\r\n",
                  cyn, yel, OLC_NUM(d), cyn, nrm);

  if (CHK_CP(CP_TITLE))
  {
    write_to_output(d, "%s1%s) Clan Title  : %s\r\n",
                    cyn, nrm, OLC_CLAN(d)->clan_name);
  }

  if (CHK_CP(CP_TITLE))
  {
    write_to_output(d, "%s2%s) Abbreviation: %s\r\n",
                    cyn, nrm, OLC_CLAN(d)->abrev ? OLC_CLAN(d)->abrev : "<Not Set>");
  }

  if (CHK_CP(CP_DESC))
  {
    write_to_output(d, "%s3%s) Description : \r\n%s%s\r\n",
                    cyn, nrm, yel, OLC_CLAN(d)->description ? OLC_CLAN(d)->description : "<Not Set!>");
  }

  if (CHK_CP(CP_APPLEV))
  {
    write_to_output(d, "%s4%s) App. Level  : %s%d\r\n",
                    cyn, nrm, yel, OLC_CLAN(d)->applev);
  }

  if (CHK_CP(CP_APPFEE))
  {
    write_to_output(d, "%s5%s) App. Fee    : %s%d\r\n",
                    cyn, nrm, yel, OLC_CLAN(d)->appfee);
  }

  if (CHK_CP(CP_TAXRATE))
  {
    write_to_output(d, "%s6%s) Tax Rate %%  : %s%d\r\n",
                    cyn, nrm, yel, OLC_CLAN(d)->taxrate);
  }

  if (CHK_CP(CP_ATWAR))
  {
    xcount = 0;
    write_to_output(d, "%s7%s) At War      : %s", cyn, nrm, cyn);
      for (x = 0; x < MAX_CLANS; x++)
      {
        if (OLC_CLAN(d)->at_war[x] == FALSE)
          continue;
        if (xcount > 0)
          write_to_output(d, ", ");
        write_to_output(d, "%s", clan_list[x].clan_name ? clan_list[x].clan_name : "<Unnamed>");
        xcount++;
      }
      if (xcount == 0)
        write_to_output(d, "<None!>");
      write_to_output(d, "%s\r\n", nrm);
  }

  if (CHK_CP(CP_ALLIED))
  {
      xcount = 0;
      write_to_output(d, "%s8%s) Allies      : %s", cyn, nrm, cyn);
      for (x = 0; x < MAX_CLANS; x++)
      {
        if (OLC_CLAN(d)->allies[x] == FALSE)
          continue;
        if (xcount > 0)
          write_to_output(d, ", ");
        write_to_output(d, "%s", clan_list[x].clan_name ? clan_list[x].clan_name : "<Unnamed>");
        xcount++;
      }
      if (xcount == 0)
        write_to_output(d, "<None!>");
      write_to_output(d, "%s\r\n", nrm);
  }

  if (CHK_CP(CP_RANKS))
  {
    write_to_output(d, "%s9%s) Set Ranks and Titles...\r\n",
                    cyn, nrm);
  }

  if (CHK_CP(CP_SETPRIVS))
  {
    write_to_output(d, "%sA%s) Set Clan Privilege Levels...\r\n",
                    cyn, nrm);
  }

  write_to_output(d, "%sQ%s) Quit\r\n",
                  cyn, nrm);

  write_to_output(d, "Enter choice : ");

  OLC_MODE(d) = CLANEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*. Display ranks menu . */

static void clanedit_ranks_menu(struct descriptor_data *d)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "%sEditing ranks%s\r\n",
                  cyn, nrm);

  write_to_output(d, "%s1%s) Set Number of Ranks : %s%d\r\n",
                  cyn, nrm, yel, OLC_CLAN(d)->ranks);

  for (i = 0; i < OLC_CLAN(d)->ranks; i++)
  {
    write_to_output(d, "%s%c%s) Rank %-2d : %s%s\r\n",
                    cyn, 'A' + i, nrm, (i + 1), yel,
                    (OLC_CLAN(d)->rank_name[i]) ? OLC_CLAN(d)->rank_name[i] : "<Not Set!>");
  }

  write_to_output(d, "%sQ%s) Quit\r\n",
                  cyn, nrm);

  write_to_output(d, "Enter choice : ");

  OLC_MODE(d) = CLANEDIT_RANK_MENU;
}

/*-------------------------------------------------------------------*/

/*. Display privileges menu . */

static void clanedit_priv_menu(struct descriptor_data *d)
{
  char buf1[MAX_INPUT_LENGTH] = {'\0'}, buf2[MAX_INPUT_LENGTH] = {'\0'};

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "%sEditing clan privileges%s\r\n",
                  cyn, nrm);

  write_to_output(d, "Command Privs                   Editor Privs\r\n");

  get_priv_string(d, buf1, CP_WHERE);
  get_priv_string(d, buf2, CP_TITLE);

  write_to_output(d, "%s1%s) Where   : %s[%s%11s%s]      "
                     "A%s) Set Title    : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_CLAIM);
  get_priv_string(d, buf1, CP_BALANCE);
  get_priv_string(d, buf2, CP_DESC);

  write_to_output(d, "%s2%s) Claim   : %s[%s%11s%s]      "
                     "B%s) Set Desc     : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_ENROL);
  get_priv_string(d, buf2, CP_APPFEE);

  write_to_output(d, "%s3%s) Enrol   : %s[%s%11s%s]      "
                     "C%s) Set App Fee  : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_PROMOTE);
  get_priv_string(d, buf2, CP_APPLEV);

  write_to_output(d, "%s4%s) Promote : %s[%s%11s%s]      "
                     "D%s) Set App Level: %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_DEMOTE);
  get_priv_string(d, buf2, CP_TAXRATE);

  write_to_output(d, "%s5%s) Demote  : %s[%s%11s%s]      "
                     "E%s) Set Tax Rate : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_EXPEL);
  get_priv_string(d, buf2, CP_ALLIED);

  write_to_output(d, "%s6%s) Expel   : %s[%s%11s%s]      "
                     "F%s) Set Ally     : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_DEPOSIT);
  get_priv_string(d, buf2, CP_ATWAR);

  write_to_output(d, "%s7%s) Deposit : %s[%s%11s%s]      "
                     "G%s) Set Enemy    : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_WITHDRAW);
  get_priv_string(d, buf2, CP_RANKS);

  write_to_output(d, "%s8%s) Withdraw: %s[%s%11s%s]      "
                     "H%s) Set Ranks    : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_OWNER);
  get_priv_string(d, buf2, CP_SETPRIVS);

  write_to_output(d, "%s9%s) Owner   : %s[%s%11s%s]      "
                     "I%s) Set Privs    : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn, nrm, cyn, yel, buf2, cyn);

  get_priv_string(d, buf1, CP_CLANEDIT);

  write_to_output(d, "%s0%s) Edit    : %s[%s%11s%s]\r\n",
                  cyn, nrm, cyn, yel, buf1, cyn);

  write_to_output(d, "%sQ%s) Quit to main menu\r\n",
                  cyn, nrm);

  write_to_output(d, "Enter choice : ");

  OLC_MODE(d) = CLANEDIT_PRIV_MENU;
}

/*-------------------------------------------------------------------*/

/*. Display privileges menu . */

static void clanedit_clans_menu(struct descriptor_data *d, int player_clan)
{
  int i;

  get_char_colors(d->character);
  clear_screen(d);

  write_to_output(d, "%sSelect a clan%s\r\n",
                  cyn, nrm);
  for (i = 0; i < num_of_clans; i++)
  {
    write_to_output(d, "%s%c%s) %s%s%s\r\n",
                    cyn, 'A' + i, nrm, CLAN_NAME(i),
                     OLC_CLAN(d)->allies[i] == TRUE ? " (Allied)" : (OLC_CLAN(d)->at_war[i] == TRUE ? " (At War)" : ""),
                    nrm);
  }
}

/*-------------------------------------------------------------------*/

/* main clanedit parser function... interpreter throws all input to here. */
void clanedit_parse(struct descriptor_data *d, char *arg)
{
  int i, number = atoi(arg), x = 0, pclan = 0;
  char *oldtext = NULL;

  switch (OLC_MODE(d))
  {

  case CLANEDIT_CONFIRM_SAVESTRING:
    switch (*arg)
    {
    case 'y':
    case 'Y':
      clanedit_save(d);
      write_to_output(d, "Clan saved.\r\n");
      mudlog(CMP, MAX(LVL_BUILDER, GET_INVIS_LEV(d->character)), TRUE,
             "OLC: %s edits clan %d.", GET_NAME(d->character), OLC_NUM(d));
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case 'n':
    case 'N':
      cleanup_olc(d, CLEANUP_ALL);
      break;
    case 'a': /* abort quit */
    case 'A':
      clanedit_disp_menu(d);
      break;
    default:
      write_to_output(d, "Invalid choice! (Y)es/(N)o/(A)bort\r\n");
      write_to_output(d, "Do you wish to save your changes? : \r\n");
      break;
    }
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_MAIN_MENU:
    switch (*arg)
    {
    case 'q':
    case 'Q':
      if (OLC_VAL(d))
      { /* Something has been modified. */
        write_to_output(d, "Do you wish to save your changes? : ");
        OLC_MODE(d) = CLANEDIT_CONFIRM_SAVESTRING;
      }
      else
        cleanup_olc(d, CLEANUP_ALL);
      return;

    case '1':
      if (CHK_CP(CP_TITLE))
      {
        write_to_output(d, "Enter new clan name : ");
        OLC_MODE(d) = CLANEDIT_NAME;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '2':
      if (CHK_CP(CP_TITLE))
      {
        write_to_output(d, "Enter new clan name abbreviation : ");
        OLC_MODE(d) = CLANEDIT_ABBREV;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '3':
      if (CHK_CP(CP_DESC))
      {
        OLC_MODE(d) = CLANEDIT_DESC;
        send_editor_help(d);
        write_to_output(d, "Enter clan description:\r\n\r\n");

        if (OLC_CLAN(d)->description)
        {
          write_to_output(d, "%s", OLC_CLAN(d)->description);
          oldtext = strdup(OLC_CLAN(d)->description);
        }
        string_write(d, &OLC_CLAN(d)->description, MAX_CLAN_DESC, 0, oldtext);
        OLC_VAL(d) = 1;

        return;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '4':
      if (CHK_CP(CP_APPLEV))
      {
        write_to_output(d, "Enter minimum level for applicants (0-%d) : ",
                        LVL_IMMORT - 1);
        OLC_MODE(d) = CLANEDIT_APPLEV;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '5':
      if (CHK_CP(CP_APPFEE))
      {
        write_to_output(d, "Enter fee paid by applicants : ");
        OLC_MODE(d) = CLANEDIT_APPFEE;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '6':
      if (CHK_CP(CP_TAXRATE))
      {
        write_to_output(d, "Enter tax percentage paid by non-clan members"
                           " (0-40) : ");
        OLC_MODE(d) = CLANEDIT_TAXRATE;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '7':
      if (CHK_CP(CP_ATWAR))
      {
        for (x = 0; x < num_of_clans; x++)
          if (clan_list[x].vnum == OLC_CLAN(d)->vnum)
            pclan = x;
        clanedit_clans_menu(d, pclan);
        write_to_output(d, "Enter Enemy Clan Choice (z=Nobody), (0=Abort) : ");
        OLC_MODE(d) = CLANEDIT_ATWAR;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '8':
      if (CHK_CP(CP_ALLIED))
      {
        for (x = 0; x < num_of_clans; x++)
          if (clan_list[x].vnum == OLC_CLAN(d)->vnum)
            pclan = x;
        clanedit_clans_menu(d, pclan);
        write_to_output(d, "Enter Ally Clan Choice, (z=Nobody), (0=Abort) : ");
        OLC_MODE(d) = CLANEDIT_ALLIED;
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case '9':
      if (CHK_CP(CP_RANKS))
      {
        clanedit_ranks_menu(d);
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    case 'a':
    case 'A':
      if (CHK_CP(CP_SETPRIVS))
      {
        clanedit_priv_menu(d);
      }
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      }
      break;

    default:

      clanedit_disp_menu(d);
      break;
    }

    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_PRIV_MENU:
    switch (*arg)
    {

    case '1':
      if (CHK_CP(CP_WHERE))
      {
        write_to_output(d, "Enter the min. rank that can use clan where "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_WHERE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '2':
      if (CHK_CP(CP_CLAIM))
      {
        write_to_output(d, "Enter the min. rank that can use clan claim "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_CLAIM;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '3':
      if (CHK_CP(CP_BALANCE))
      {
        write_to_output(d, "Enter the min. rank that can use clan balance "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_BALANCE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '4':
      if (CHK_CP(CP_ENROL))
      {
        write_to_output(d, "Enter the min. rank that can use clan enrol "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_ENROL;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '5':
      if (CHK_CP(CP_PROMOTE))
      {
        write_to_output(d, "Enter the min. rank that can use clan promote "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_PROMOTE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '6':
      if (CHK_CP(CP_DEMOTE))
      {
        write_to_output(d, "Enter the min. rank that can use clan demote "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_DEMOTE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '7':
      if (CHK_CP(CP_EXPEL))
      {
        write_to_output(d, "Enter the min. rank that can use clan expel "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_EXPEL;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '8':
      if (CHK_CP(CP_DEPOSIT))
      {
        write_to_output(d, "Enter the min. rank that can use clan deposit "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_DEPOSIT;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '9':
      if (CHK_CP(CP_WITHDRAW))
      {
        write_to_output(d, "Enter the min. rank that can use clan withdraw "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_WITHDRAW;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'j':
    case 'J':
      if (CHK_CP(CP_OWNER))
      {
        write_to_output(d, "Enter the min. rank that can use clan owner "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_OWNER;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case '0':
      if (CHK_CP(CP_CLANEDIT))
      {
        write_to_output(d, "Enter the min. rank that can use clan where "
                           "(0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_CLANEDIT;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'a':
    case 'A':
      if (CHK_CP(CP_TITLE))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan "
                           "name (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_TITLE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'b':
    case 'B':
      if (CHK_CP(CP_DESC))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan "
                           "description (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_TITLE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'c':
    case 'C':
      if (CHK_CP(CP_APPLEV))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan "
                           "application level (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_APPLEV;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'd':
    case 'D':
      if (CHK_CP(CP_APPFEE))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan "
                           "application fee (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_APPFEE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'e':
    case 'E':
      if (CHK_CP(CP_TAXRATE))
      {
        write_to_output(d, "Enter the mimimum rank that can set the tax "
                           "rate (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_TAXRATE;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'f':
    case 'F':
      if (CHK_CP(CP_ALLIED))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan's "
                           "ally (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_ALLIED;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'g':
    case 'G':
      if (CHK_CP(CP_ATWAR))
      {
        write_to_output(d, "Enter the mimimum rank that can set the clan's "
                           "enemy (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_ATWAR;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'h':
    case 'H':
      if (CHK_CP(CP_RANKS))
      {
        write_to_output(d, "Enter the mimimum rank that can set ranks and "
                           "titles (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_RANKS;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'i':
    case 'I':
      if (CHK_CP(CP_SETPRIVS))
      {
        write_to_output(d, "Enter the mimimum rank that can edit privilege "
                           "levels (0-%d) : ",
                        OLC_CLAN(d)->ranks);
        OLC_MODE(d) = CLANEDIT_CP_SETPRIVS;
      }
      else
      {
        write_to_output(d, "%sYou don't have sufficient clan access.%s\r\n"
                           "Enter Choice : ",
                        CBRED(d->character, C_NRM),
                        CCNRM(d->character, C_NRM));
      }
      return;

    case 'q':
    case 'Q':
      clanedit_disp_menu(d);
      return;

    default:
      clanedit_priv_menu(d);
      break;
    }
    return; /* end of CRAFTEDIT_PRIV_MENU */
    /*-------------------------------------------------------------------*/
  case CLANEDIT_RANK_MENU:
    if (*arg == '1')
    {
      write_to_output(d, "Enter number of ranks (1-%d) : ", MAX_CLANRANKS);
      OLC_MODE(d) = CLANEDIT_NUM_RANKS;
    }
    else if (*arg == 'q' || *arg == 'Q')
    {
      clanedit_disp_menu(d);
    }
    else
    {
      /* Grab any input between a and z */
      if (*arg >= 'a' && *arg <= 'z')
        number = *arg - 'a';
      else if (*arg >= 'A' && *arg <= 'Z')
        number = *arg - 'A';
      else
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        return;
      }
      /* Check it's in the valid rank range */
      if (number < 0 || number >= OLC_CLAN(d)->ranks)
      {
        write_to_output(d, "%sInvalid Choice!%s\r\nEnter Choice : ",
                        CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
        return;
      }
      /* Zone number isn't used in clanedit, so we'll use it as a placeholder */
      write_to_output(d, "Enter a new title for rank %d : ", (number + 1));
      OLC_ZNUM(d) = number;
      OLC_MODE(d) = CLANEDIT_RANK_NAME;
    }
    return; /* end of CRAFTEDIT_RANK_MENU */
    /*-------------------------------------------------------------------*/
  case CLANEDIT_NAME:
    if (OLC_CLAN(d)->clan_name)
      free(OLC_CLAN(d)->clan_name);
    if (*arg)
      OLC_CLAN(d)->clan_name = strdup(arg);
    else
      OLC_CLAN(d)->clan_name = NULL;
    OLC_VAL(d) = 1;
    clanedit_disp_menu(d);
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_ABBREV:
    if (OLC_CLAN(d)->abrev)
      free(OLC_CLAN(d)->abrev);
    if (*arg)
      OLC_CLAN(d)->abrev = strdup(arg);
    else
      OLC_CLAN(d)->abrev = NULL;
    OLC_VAL(d) = 1;
    clanedit_disp_menu(d);
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_DESC:
    /*
     * We should never get here.
     */
    clanedit_save(d);
    write_to_output(d, "Clan saved.\r\n");
    cleanup_olc(d, CLEANUP_ALL);
    mudlog(BRF, LVL_BUILDER, TRUE, "SYSERR: OLC: clanedit_parse(): Reached"
                                   "CLANEDIT_DESC case!");
    write_to_output(d, "Oops (still some work to do here -Zusuk)...\r\n");

    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_APPLEV:
    OLC_CLAN(d)->applev = LIMIT(number, 0, (LVL_IMMORT - 1));
    OLC_VAL(d) = 1;
    clanedit_disp_menu(d);
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_APPFEE:
    OLC_CLAN(d)->appfee = LIMIT(number, 0, 1000000);
    OLC_VAL(d) = 1;
    clanedit_disp_menu(d);
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_TAXRATE:
    OLC_CLAN(d)->taxrate = LIMIT(number, 0, 40);
    OLC_VAL(d) = 1;
    clanedit_disp_menu(d);
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_ATWAR:
    if (*arg == '0')
    { /* Abort option */
      clanedit_disp_menu(d);
      return;
    }
    if (*arg == 'z')
    { /* war with no one option */
      for (x = 0; x < MAX_CLANS; x++)
        OLC_CLAN(d)->at_war[x] = NO_CLAN;
      OLC_VAL(d) = 1;
      clanedit_disp_menu(d);
      return;
    }

    if (*arg >= 'a' && *arg <= 'z')
      number = *arg - 'a';
    else if (*arg >= 'A' && *arg <= 'Z')
      number = *arg - 'A';
    else
    {
      write_to_output(d, "%sInvalid Choice!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    if (number < 0 || number >= num_of_clans)
    {
      write_to_output(d, "%sInvalid Choice!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    if (clan_list[number].vnum == OLC_NUM(d))
    {
      write_to_output(d, "%sDon't be ridiculous!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      return;
    }
    if (are_clans_allied(real_clan(OLC_CLAN(d)->vnum), number))
    {
      write_to_output(d, "%sThe clan can't be at war with an ally!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    OLC_CLAN(d)->at_war[number] = !OLC_CLAN(d)->at_war[number];
    write_to_output(d, "Your at-war status with %s has changed.\r\n", clan_list[number].clan_name);
    clanedit_disp_menu(d);
    OLC_VAL(d) = 1;
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_ALLIED:
    if (*arg == '0')
    { /* Abort option */
      clanedit_disp_menu(d);
      return;
    }
    if (*arg == 'z')
    { /* war with no one option */
      for (x = 0; x < MAX_CLANS; x++)
        OLC_CLAN(d)->allies[x] = NO_CLAN;
      OLC_VAL(d) = 1;
      clanedit_disp_menu(d);
      return;
    }

    if (*arg >= 'a' && *arg <= 'z')
      number = *arg - 'a';
    else if (*arg >= 'A' && *arg <= 'Z')
      number = *arg - 'A';
    else
    {
      write_to_output(d, "%sInvalid Choice!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    if (number < 0 || number >= num_of_clans)
    {
      write_to_output(d, "%sInvalid Choice!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    if (clan_list[number].vnum == OLC_NUM(d))
    {
      write_to_output(d, "%sDon't be ridiculous!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      return;
    }
    if (are_clans_at_war(real_clan(OLC_CLAN(d)->vnum), number))
    {
      write_to_output(d, "%sThe clan can't be allied with an enemy!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_disp_menu(d);
      return;
    }
    OLC_CLAN(d)->allies[number] = !OLC_CLAN(d)->allies[number];
    write_to_output(d, "Your alliance with %s has changed.\r\n", clan_list[number].clan_name);
    clanedit_disp_menu(d);
    OLC_VAL(d) = 1;
    return;

    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_CLAIM:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_CLAIM] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_BALANCE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_BALANCE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_DEMOTE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_DEMOTE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_DEPOSIT:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_DEPOSIT] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_CLANEDIT:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_CLANEDIT] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_ENROL:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_ENROL] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_EXPEL:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_EXPEL] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_OWNER:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_OWNER] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_PROMOTE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_PROMOTE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_WHERE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_WHERE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_WITHDRAW:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_WITHDRAW] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_ALLIED:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_ALLIED] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_APPFEE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_APPFEE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_APPLEV:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_APPLEV] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_DESC:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_DESC] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_TAXRATE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_TAXRATE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_RANKS:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_RANKS] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_TITLE:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_TITLE] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_ATWAR:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_ATWAR] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_CP_SETPRIVS:
    if (number < 0 || number > OLC_CLAN(d)->ranks)
    {
      write_to_output(d, "%sInvalid Rank!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      clanedit_priv_menu(d);
      return;
    }
    OLC_CLAN(d)->privilege[CP_SETPRIVS] = number;
    OLC_VAL(d) = 1;
    clanedit_priv_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_NUM_RANKS:
    if (number < 1 || number > MAX_CLANRANKS)
    {
      write_to_output(d, "%sInvalid number of ranks!%s\r\n",
                      CBRED(d->character, C_NRM), CCNRM(d->character, C_NRM));
      write_to_output(d, "Enter number of ranks! (1-%d) : ",
                      MAX_CLANRANKS);
      return;
    }
    OLC_CLAN(d)->ranks = number;
    /* Erase all the old rank names that are now outside the range (if any) */
    for (i = number; i < MAX_CLANRANKS; i++)
    {
      if (OLC_CLAN(d)->rank_name[i])
      {
        free(OLC_CLAN(d)->rank_name[i]);
        OLC_CLAN(d)->rank_name[i] = NULL;
      }
    }
    clanedit_ranks_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case CLANEDIT_RANK_NAME:
    /* We use the OLC_ZNUM as a placeholder for the rank number -get it back */
    number = OLC_ZNUM(d);

    /* Free the old rank name */
    if (OLC_CLAN(d)->rank_name[number])
    {
      free(OLC_CLAN(d)->rank_name[number]);
      OLC_CLAN(d)->rank_name[number] = NULL;
    }

    if (arg && *arg)
    { /* No arg, remove title! */
      OLC_CLAN(d)->rank_name[number] = strdup(arg);
    }
    clanedit_ranks_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  default:
    mudlog(BRF, LVL_BUILDER, TRUE,
           "SYSERR: OLC: Reached default case in clanedit_parse()!");
    write_to_output(d, "Oops...\r\n");
    clanedit_disp_menu(d);
    break;
  }
}

void clanedit_string_cleanup(struct descriptor_data *d, int terminator)
{
  switch (OLC_MODE(d))
  {

  case CLANEDIT_DESC:
  default:
    clanedit_disp_menu(d);
    break;
  }
}

/* Copies a clan's information.
 * NOTE: Allocates memory for DUPLICATED strings */
void duplicate_clan_data(struct clan_data *to_clan,
                         struct clan_data *from_clan)
{
  int i;

  /* Ensure no allocated memory is wasted or lost */
  if (to_clan->clan_name)
    free(to_clan->clan_name);

  if (to_clan->abrev)
    free(to_clan->abrev);

  if (to_clan->description)
    free(to_clan->description);

  for (i = 0; i < MAX_CLANRANKS; i++)
    if (to_clan->rank_name[i])
      free(to_clan->rank_name[i]);

  /* Now copy across the data */
  copy_clan_data(to_clan, from_clan);

  /* And re-duplicate the strings */
  if (from_clan->clan_name)
    to_clan->clan_name = strdup(from_clan->clan_name);

  if (from_clan->abrev)
    to_clan->abrev = strdup(from_clan->abrev);

  if (from_clan->description)
    to_clan->description = strdup(from_clan->description);

  for (i = 0; i < MAX_CLANRANKS; i++)
    if (from_clan->rank_name[i])
      to_clan->rank_name[i] = strdup(from_clan->rank_name[i]);
}
