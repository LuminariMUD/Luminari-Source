/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/  Luminari Rank System                                                           
/  Created By: Slug on fredMUD, adapted to tba3.53 by Jamdog                                                           
\                    ported to Luminari by Zusuk             
/  Date: November 1996, January 20th 2007, January 29 2018                                                          
\       Command is in act.informative.c                                                   
/  Header: act.h                                                                                                                                                                                     
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "limits.h"
#include "screen.h"
#include "act.h"

/* extern vars */
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct player_index_element *player_table;

/* extern functions */

#ifdef NEEDS_STRDUP
char *strdup(char *source);
#endif

#define MAX_RANKED 180
#define GET_KEY(cc, tk) ((*((tk)->function))(cc))

typedef char *ranktype;

struct key_data
{
  char *keystring;
  char *outstring;
  ranktype (*function)(struct char_data *ch);
  struct key_data *next;
};

struct rank_data
{
  struct char_data *ch;
  char key[80];
};

ranktype rank_clanbucks(struct char_data *ch);
ranktype rank_gauntlet(struct char_data *ch);
ranktype rank_hp(struct char_data *ch);
ranktype rank_psp(struct char_data *ch);
ranktype rank_moves(struct char_data *ch);
ranktype rank_curhp(struct char_data *ch);
ranktype rank_power(struct char_data *ch);
ranktype rank_str(struct char_data *ch);
ranktype rank_int(struct char_data *ch);
ranktype rank_wis(struct char_data *ch);
ranktype rank_dex(struct char_data *ch);
ranktype rank_con(struct char_data *ch);
ranktype rank_cha(struct char_data *ch);
ranktype rank_fitness(struct char_data *ch);
ranktype rank_hitroll(struct char_data *ch);
ranktype rank_damroll(struct char_data *ch);
ranktype rank_armor(struct char_data *ch);
ranktype rank_sp(struct char_data *ch);
ranktype rank_xp(struct char_data *ch);
ranktype rank_height(struct char_data *ch);
ranktype rank_weight(struct char_data *ch);
ranktype rank_fatness(struct char_data *ch);
ranktype rank_coolness(struct char_data *ch);
ranktype rank_gold(struct char_data *ch);
ranktype rank_bank(struct char_data *ch);
ranktype rank_age(struct char_data *ch);
ranktype rank_kills(struct char_data *ch);
ranktype rank_deaths(struct char_data *ch);
ranktype rank_highest_xp(struct char_data *ch);
ranktype rank_kd(struct char_data *ch);
ranktype rank_played(struct char_data *ch);
//ranktype rank_birth(struct char_data *ch);
ranktype rank_remorts(struct char_data *ch);
ranktype rank_blabber(struct char_data *ch);

int rank_compare_top(const void *n1, const void *n2);
int rank_compare_bot(const void *n1, const void *n2);
int char_compare(const void *n1, const void *n2);

void eat_spaces(char **source);

void init_keys(void);

static void add_key(const char *key, const char *out, ranktype (*f)(struct char_data *ch));
struct key_data *search_key(char *key);

struct key_data *key_list = NULL;
struct key_data *key_list_tail = NULL;
struct key_data *the_key = NULL;

int maxkeylength = 5; /* used for formatting in rankhelp */
int maxoutlength = 5;

int keys_inited = FALSE;
int KEY_PRINTING = FALSE; /* used by rank functions sometimes */

char kbuf[80]; /* used by rank functions */
char *kbp;

struct rank_data tt[MAX_RANKED + 1];

/* primary function for display, in act.informative.c */
void do_slug_rank(struct char_data *ch, const char *arg)
{
  int i, j, k;
  int rk;
  int toprank;
  char buffer[MAX_STRING_LENGTH];
  char keybuf[MAX_STRING_LENGTH], nbuf[MAX_STRING_LENGTH];
  struct char_data *c;
  struct descriptor_data *d;
  struct key_data *tk;

  char *nbufp;

  if (!keys_inited)
  {
    init_keys();
    keys_inited = TRUE;
    log("Setting up Rank keys");
  }

  if (IS_NPC(ch))
    return;

  while (arg[0] == ' ')
    arg++;

  half_chop_c(arg, keybuf, sizeof(keybuf), nbuf, sizeof(nbuf));
  nbufp = nbuf;

  if (!strlen(keybuf))
    strlcpy(keybuf, "help", sizeof(keybuf));

  the_key = search_key(keybuf);

  if (the_key == NULL)
  {
    send_to_char(ch, "Please specify a valid key. Type \"rank help\" for instructions.\n\r");
    return;
  }
  else if (!strcmp(the_key->keystring, "help"))
  {
    send_to_char(ch, "Usage: %srank <key>%s\r\n", QYEL, QNRM);
    send_to_char(ch, "Valid keys:\n\r");
    j = 80 / (maxkeylength + 1);
    i = 0;
    buffer[0] = '\0';
    for (tk = key_list; tk; tk = tk->next)
    {
      snprintf(kbuf, sizeof(kbuf), "%*s ", maxkeylength, tk->keystring);
      strlcat(buffer, kbuf, sizeof(buffer));
      if (++i % j == 0)
        strlcat(buffer, "\n\r", sizeof(buffer));
    }
    strlcat(buffer, "\n\r", sizeof(buffer));
    strlcat(buffer, "Usage: rank <key> [+-][max]\n\r", sizeof(buffer));
    send_to_char(ch, buffer);
  }
  else
  {
    k = 0;
    for (d = descriptor_list; d; d = d->next)
    {
      if (!d->connected)
      {
        if (d->original)
          c = d->original;
        else
          c = d->character;
        if ((CAN_SEE(ch, c) /*&& !PRF_FLAGGED(c, PRF_NORANK)*/) ||
            (GET_LEVEL(ch) == LVL_IMPL))
        {
          if (k)
          {
            if (k < MAX_RANKED)
            {
              tt[k].ch = c;
              k++;
            }
            else
            { /*list is full */
              tt[k].ch = c;
            }
          }
          else
          { /* first player in list */
            tt[k].ch = c;
            k++;
          }
        }
      }
    }
    toprank = TRUE;
    if (!strcmp(the_key->keystring, "deaths"))
      toprank = FALSE;
    if (*nbufp == '-')
    {
      toprank = FALSE;
      nbufp++;
    }
    else if (*nbufp == '+')
    {
      toprank = TRUE;
      nbufp++;
    }
    rk = atoi(nbufp);
    if (rk == 0)
      rk = 20;
    if (k < rk)
      rk = k;
    for (i = 0; i < k; i++)
      strcpy(tt[i].key, GET_KEY(tt[i].ch, the_key));

    /* sort */
    if (toprank)
      qsort(tt, k, sizeof(struct rank_data), rank_compare_top);
    else
      qsort(tt, k, sizeof(struct rank_data), rank_compare_bot);

    /*print out */
    KEY_PRINTING = TRUE;
    send_to_char(ch, "%s %d players ranked by %s.\n\r", toprank ? "Top" : "Bottom", rk, the_key->outstring);
    for (i = 0; i < rk; i++)
    {
      strlcpy(kbuf, GET_KEY(tt[i].ch, the_key), sizeof(kbuf));
      for (kbp = kbuf; *kbp == ' '; kbp++)
        ;
      send_to_char(ch, "#%2d: %15s %*s\n\r", i + 1, GET_NAME(tt[i].ch),
                   count_color_chars(kbp) + 18, (GET_LEVEL(ch) >= LVL_IMMORT) ? kbp : "");
    }
    send_to_char(ch, "%s\n\r", CONFIG_OK);
  }
}

/* put the add_keys in the order you want them displayed in help */
void init_keys(void)
{
  add_key("hp", "hit points", rank_hp);
  add_key("psp", "psp points", rank_psp);
  add_key("moves", "move points", rank_moves);
  add_key("curhp", "hit points", rank_curhp);
  add_key("power", "the sum of hp,psp,moves", rank_power);
  add_key("strength", "strength", rank_str);
  add_key("intel", "intelligence", rank_int);
  add_key("wisdom", "wisdom", rank_wis);
  add_key("dexterity", "dexterity", rank_dex);
  add_key("charisma", "charisma", rank_cha);
  add_key("const", "constitution", rank_con);
  add_key("fitness", "sum of stats", rank_fitness);
  add_key("hitroll", "hit bonus", rank_hitroll);
  add_key("damroll", "damage bonus", rank_damroll);
  add_key("armor", "armor", rank_armor);
  add_key("sp", "spellpower", rank_sp);
  add_key("xp", "experience points", rank_xp);
  add_key("height", "height", rank_height);
  add_key("weight", "weight", rank_weight);
  add_key("fatness", "body mass index", rank_fatness);
  add_key("coolness", "coolness", rank_coolness);
  add_key("gold", "gold carried", rank_gold);
  add_key("bank", "bank balance", rank_bank);
  add_key("age", "age", rank_age);
  //add_key("kills", "kills", rank_kills);
  //add_key("deaths", "deaths", rank_deaths);
  //add_key("kd", "kills to deaths ratio", rank_kd);
  add_key("played", "time played", rank_played);
  //add_key("birth", "time since creation", rank_birth);
  //add_key("remorts", "number of remorts", rank_remorts);
  //add_key("blabber", "# of gossips", rank_blabber);
  //add_key("clanbucks", "clan taxes earned", rank_clanbucks);

  add_key("help", "help", NULL);
} /* end init_keys */

struct key_data *search_key(char *key)
{

  struct key_data *tk;

  if (strlen(key))
    for (tk = key_list; tk; tk = tk->next)
      if (!strncmp(tk->keystring, key, strlen(key)))
        return tk;

  return NULL;

} /* end search_key */

static void add_key(const char *key, const char *out, ranktype (*f)(struct char_data *ch))
{

  struct key_data *tmp_key;

  CREATE(tmp_key, struct key_data, 1);
  tmp_key->keystring = strdup(key);
  tmp_key->outstring = strdup(out);
  tmp_key->function = f;
  tmp_key->next = NULL;

  maxkeylength = MAX(maxkeylength, strlen(key));
  maxoutlength = MAX(maxoutlength, strlen(out));

  /* append to list */
  if (!key_list)
    key_list = key_list_tail = tmp_key;
  else
  {
    key_list_tail->next = tmp_key;
    key_list_tail = tmp_key;
  }

} /* end add_key */

void eat_spaces(char **source)
{
  while (**source == ' ')
    (*source)++;
} /* end eat_spaces */

char k1[80];
char k2[80];

int rank_compare_top(const void *n1, const void *n2)
{
  strlcpy(k1, (*((struct rank_data *)n1)).key, sizeof(k1));
  strlcpy(k2, (*((struct rank_data *)n2)).key, sizeof(k2));
  return (strcmp(k2, k1));
}

int char_compare(const void *n1, const void *n2)
{
  return ((*((char *)n1)) - (*((char *)n2)));
}

int rank_compare_bot(const void *n1, const void *n2)
{

  strlcpy(k1, (*((struct rank_data *)n1)).key, sizeof(k1));
  strlcpy(k2, (*((struct rank_data *)n2)).key, sizeof(k2));

  return (strcmp(k1, k2));
}

/* RANK FUNCTIONS */

// added --mystic-- 5Jan06

/*
ranktype rank_clanbucks(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%27ld", GET_CLANBUCKS(ch));
  return (kbuf);
}*/
/* end rank_clanbucks */

ranktype rank_hp(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_MAX_HIT(ch));
  return (kbuf);
} /* end rank_hp */

ranktype rank_psp(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_MAX_PSP(ch));
  return (kbuf);
} /* end rank_psp */

ranktype rank_moves(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_MAX_MOVE(ch));
  return (kbuf);
} /* end rank_move */

ranktype rank_curhp(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_HIT(ch));
  return (kbuf);
} /* end rank_hp */

ranktype rank_power(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", (GET_MAX_MOVE(ch) + GET_MAX_PSP(ch) + GET_MAX_HIT(ch)));
  return (kbuf);
} /*end rank_power */

ranktype rank_str(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_STR(ch));
  return (kbuf);
} /* end rank_str */

ranktype rank_int(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_INT(ch));
  return (kbuf);
} /* end rank_int */

ranktype rank_wis(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_WIS(ch));
  return (kbuf);
} /* end rank_wis */

ranktype rank_dex(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_DEX(ch));
  return (kbuf);
} /* end rank_dex */

ranktype rank_con(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_CON(ch));
  return (kbuf);
} /* end rank_con */

ranktype rank_cha(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_CHA(ch));
  return (kbuf);
} /* end rank_cha */

ranktype rank_fitness(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_STR(ch) + GET_INT(ch) + GET_WIS(ch) + GET_DEX(ch) + GET_CON(ch));
  return (kbuf);
} /* end rank_fitness */

ranktype rank_hitroll(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_HITROLL(ch));
  return (kbuf);
} /* end rank_hitroll */

ranktype rank_damroll(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_DAMROLL(ch));
  return (kbuf);
} /* end rank_damroll */

ranktype rank_armor(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_AC(ch));
  return (kbuf);
} /* end rank_armor */

ranktype rank_sp(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", CASTER_LEVEL(ch));
  return (kbuf);
} /* end rank_sp */

ranktype rank_xp(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_EXP(ch));
  return (kbuf);
} /* end rank_xp */

ranktype rank_height(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_HEIGHT(ch));
  return (kbuf);
} /* end rank_height */

ranktype rank_weight(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", GET_WEIGHT(ch));
  return (kbuf);
} /* end rank_weight */

ranktype rank_fatness(struct char_data *ch)
{
  float bmi;
  bmi = ((float)GET_WEIGHT(ch) * 10000.0) /
        (2.2 * (float)GET_HEIGHT(ch) * (float)GET_HEIGHT(ch));
  snprintf(kbuf, sizeof(kbuf), "%20.2f", bmi);
  return (kbuf);
} /* end rank_fatness */

ranktype rank_coolness(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%20d", (GET_LEVEL(ch) == LVL_IMPL) ? 100 : ((GET_GOLD(ch) + GET_EXP(ch) % 100) % 100));
  return (kbuf);
} /* end rank_coolness */

ranktype rank_gold(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_GOLD(ch));
  return (kbuf);
} /* end rank_gold */

ranktype rank_bank(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%27d", GET_BANK_GOLD(ch));
  return (kbuf);
} /* end rank_bank */

ranktype rank_age(struct char_data *ch)
{
  snprintf(kbuf, sizeof(kbuf), "%3d", GET_AGE(ch));
  return (kbuf);
} /* end rank_age */

/*
ranktype rank_kills(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%20ld", GET_KILLS(ch));
  return (kbuf);
}*/
/* end rank_kills */

/*
ranktype rank_deaths(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%20ld", GET_DEATHS(ch));
  return (kbuf);
}*/
/* end rank_deaths */

/*
ranktype rank_kd(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%20.2f", GET_DEATHS(ch) ? (float) GET_KILLS(ch) / GET_DEATHS(ch) : (float) GET_KILLS(ch));
  return (kbuf);
} */
/* end rank_kd */

ranktype rank_played(struct char_data *ch)
{
  struct time_info_data playing_time;
  //struct time_info_data real_time_passed(time_t t2, time_t t1);
  playing_time = *real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
  snprintf(kbuf, sizeof(kbuf), "%3d days, %2d hours.", playing_time.day, playing_time.hours);
  return (kbuf);
} /* end rank_days */

/*
ranktype rank_birth(struct char_data *ch) {
  struct time_info_data *playing_time;
  playing_time = age(ch);
  snprintf(kbuf, sizeof(kbuf), "%3d days, %2d hours.", playing_time->day, playing_time->hours);
  return (kbuf);
} */
/* end rank_days */

/*
ranktype rank_remorts(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%20.2d", GET_REMORT_NUMBER(ch));
  return (kbuf);
}*/
/* end rank_remorts */

/*
ranktype rank_blabber(struct char_data *ch) {
  snprintf(kbuf, sizeof(kbuf), "%20.2d", GET_GOSSIPS(ch));
  return (kbuf);
}*/
/* end rank_blabber */
