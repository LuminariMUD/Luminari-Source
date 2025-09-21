/**************************************************************************
 *  File: race.c                                               LuminariMUD *
 *  Usage: Source file for race-specific code.                             *
 *  Authors:  Nashak and Zusuk                                             *
 **************************************************************************/

/** Help buffer the global variable definitions */
#define __RACE_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "race.h"
#include "feats.h"
#include "class.h"
#include "backgrounds.h"
#include "treasure.h"
#include "spec_procs.h"

/* defines */
#define Y TRUE
#define N FALSE
/* racial classification for PC races */
#define IS_NORMAL 0
#define IS_ADVANCE 1
#define IS_EPIC_R 2

/* some pre setup here */

struct race_data race_list[NUM_EXTENDED_RACES];
int race_sort_info[NUM_EXTENDED_RACES+1];

/* Zusuk, 02/2016:  Start notes here!
 * RACE_ these are specific race defines, eventually should be a massive list
 *       of every race in our world (ex: iron golem)
 * SUBRACE_ these are subraces for NPC's, currently set to maximum 3, some
 *          mechanics such as resistances are built into these (ex: fire, goblinoid)
 * PC_SUBRACE_ these are subraces for PC's, only used for animal shapes spell
 *             currently, use to be part of the wildshape system (need to phase this out)
 * RACE_TYPE_ this is like the family the race belongs to, like an iron golem
 *            would be RACE_TYPE_CONSTRUCT
 */

/* start race code! */

/* this will set the appropriate gender for a given race */
void set_race_genders(int race, int neuter, int male, int female)
{
  race_list[race].genders[0] = neuter;
  race_list[race].genders[1] = male;
  race_list[race].genders[2] = female;
}

/* this will set the ability modifiers of the given race to whatever base
   stats are, to be used for both PC and wildshape forms */
const char *abil_mod_names[NUM_ABILITY_MODS + 1] = {
    /* an unfortunate necessity to make this constant array - we didn't make
     the modifiers same order as the structs.h version */
    "Str",
    "Con",
    "Int",
    "Wis",
    "Dex",
    "Cha",
    "\n"};
void set_race_abilities(int race, int str_mod, int con_mod, int int_mod,
                        int wis_mod, int dex_mod, int cha_mod)
{
  race_list[race].ability_mods[0] = str_mod;
  race_list[race].ability_mods[1] = con_mod;
  race_list[race].ability_mods[2] = int_mod;
  race_list[race].ability_mods[3] = wis_mod;
  race_list[race].ability_mods[4] = dex_mod;
  race_list[race].ability_mods[5] = cha_mod;
}
int get_race_stat(int race, int stat)
{
  if (stat < 0 || stat > 5)
    return 0;

  return (race_list[race].ability_mods[stat]);
}

/* appropriate alignments for given race */
void set_race_alignments(int race, int lg, int ng, int cg, int ln, int tn, int cn,
                         int le, int ne, int ce)
{
  race_list[race].alignments[0] = lg;
  race_list[race].alignments[1] = ng;
  race_list[race].alignments[2] = cg;
  race_list[race].alignments[3] = ln;
  race_list[race].alignments[4] = tn;
  race_list[race].alignments[5] = cn;
  race_list[race].alignments[6] = le;
  race_list[race].alignments[7] = ne;
  race_list[race].alignments[8] = ce;
}

/* set the attack types this race will use when not wielding */
void set_race_attack_types(int race, int hit, int sting, int whip, int slash,
                           int bite, int bludgeon, int crush, int pound, int claw, int maul,
                           int thrash, int pierce, int blast, int punch, int stab, int slice,
                           int thrust, int hack, int rake, int peck, int smash, int trample,
                           int charge, int gore)
{
  race_list[race].attack_types[0] = hit;
  race_list[race].attack_types[1] = sting;
  race_list[race].attack_types[2] = whip;
  race_list[race].attack_types[3] = slash;
  race_list[race].attack_types[4] = bite;
  race_list[race].attack_types[5] = bludgeon;
  race_list[race].attack_types[6] = crush;
  race_list[race].attack_types[7] = pound;
  race_list[race].attack_types[8] = claw;
  race_list[race].attack_types[9] = maul;
  race_list[race].attack_types[10] = thrash;
  race_list[race].attack_types[11] = pierce;
  race_list[race].attack_types[12] = blast;
  race_list[race].attack_types[13] = punch;
  race_list[race].attack_types[14] = stab;
  race_list[race].attack_types[15] = slice;
  race_list[race].attack_types[16] = thrust;
  race_list[race].attack_types[17] = hack;
  race_list[race].attack_types[18] = rake;
  race_list[race].attack_types[19] = peck;
  race_list[race].attack_types[20] = smash;
  race_list[race].attack_types[21] = trample;
  race_list[race].attack_types[22] = charge;
  race_list[race].attack_types[23] = gore;
}

/* function to initialize the whole race list to empty values */
void initialize_races(void)
{
  int i = 0;

  for (i = 0; i < NUM_EXTENDED_RACES; i++)
  {
    /* displaying the race */
    race_list[i].name = "unknown";
    race_list[i].type = NULL;
    race_list[i].type_color = NULL;
    race_list[i].abbrev = NULL;
    race_list[i].abbrev_color = NULL;

    /* displaying more race details (extension) */
    race_list[i].descrip = NULL;
    race_list[i].morph_to_char = NULL;
    race_list[i].morph_to_room = NULL;

    /* the rest of the values */
    race_list[i].family = RACE_TYPE_UNDEFINED;
    race_list[i].size = SIZE_MEDIUM;
    race_list[i].is_pc = FALSE;
    race_list[i].level_adjustment = 0;
    race_list[i].unlock_cost = 0;
    race_list[i].epic_adv = IS_NORMAL;

    /* handle outside add_race() */
    set_race_genders(i, N, N, N);
    set_race_abilities(i, 0, 0, 0, 0, 0, 0);
    set_race_alignments(i, N, N, N, N, N, N, N, N, N);
    set_race_attack_types(i, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
                          N, N, N, N, N, N, N);

    /* any linked lists to initailze? */
  }
}

/* papa assign-function for adding races to the race list */
static void add_race(int race,
                     const char *name, const char *type, const char *type_color, const char *abbrev, const char *abbrev_color,
                     ubyte family, byte size, sbyte is_pc, ubyte level_adjustment, int unlock_cost,
                     byte epic_adv)
{

  /* displaying the race */
  race_list[race].name = strdup(name);                 /* lower case no-space */
  race_list[race].type = strdup(type);                 /* capitalized space, no color */
  race_list[race].type_color = strdup(type_color);     /* capitalized space, color */
  race_list[race].abbrev = strdup(abbrev);             /* 4 letter abbrev, no color */
  race_list[race].abbrev_color = strdup(abbrev_color); /* 4 letter abbrev, color */

  /* assigning values */
  race_list[race].family = family;
  race_list[race].size = size;
  race_list[race].is_pc = is_pc;
  race_list[race].level_adjustment = level_adjustment;
  race_list[race].unlock_cost = unlock_cost;
  race_list[race].epic_adv = epic_adv;
}

/* extension of details added to race */
static void set_race_details(int race,
                             const char *descrip, const char *morph_to_char, const char *morph_to_room)
{

  race_list[race].descrip = strdup(descrip); /* Description of race */
  /* message to send to room if transforming to this particular race */
  race_list[race].morph_to_char = strdup(morph_to_char);
  race_list[race].morph_to_room = strdup(morph_to_room);
}

/*
// fun idea based on favored class system, not currently utilized in our game
void favored_class_female(int race, int favored_class) {
  race_list[race].favored_class[2] = favored_class;
}
*/

/* our little mini struct series for assigning feats to a race  */
/* create/allocate memory for the racefeatassign struct */
struct race_feat_assign *create_feat_assign_races(int feat_num, int level_received,
                                                  bool stacks)
{
  struct race_feat_assign *feat_assign = NULL;

  CREATE(feat_assign, struct race_feat_assign, 1);
  feat_assign->feat_num = feat_num;
  feat_assign->level_received = level_received;
  feat_assign->stacks = stacks;

  return feat_assign;
}
/* actual function called to perform the feat assignment */
void feat_race_assignment(int race_num, int feat_num, int level_received,
                          bool stacks)
{
  struct race_feat_assign *feat_assign = NULL;

  feat_assign = create_feat_assign_races(feat_num, level_received, stacks);

  /*   Link it up. */
  feat_assign->next = race_list[race_num].featassign_list;
  race_list[race_num].featassign_list = feat_assign;
}

/* our little mini struct series for assigning affects to a race  */
/* create/allocate memory for the struct */
struct affect_assign *create_affect_assign(int affect_num, int level_received)
{
  struct affect_assign *aff_assign = NULL;

  CREATE(aff_assign, struct affect_assign, 1);
  aff_assign->affect_num = affect_num;
  aff_assign->level_received = level_received;

  return aff_assign;
}
/* actual function called to perform the affect assignment */
void affect_assignment(int race_num, int affect_num, int level_received)
{
  struct affect_assign *aff_assign = NULL;

  aff_assign = create_affect_assign(affect_num, level_received);

  /*   Link it up. */
  aff_assign->next = race_list[race_num].affassign_list;
  race_list[race_num].affassign_list = aff_assign;
}

/* determines if ch qualifies for a race */
bool race_is_available(struct char_data *ch, int race_num)
{

  // dumb-dumb check
  if (race_num < 0 || race_num >= NUM_EXTENDED_RACES)
    return FALSE;

  // is this race pc (playable) race?
  if (!race_list[race_num].is_pc)
    return FALSE;

  // locked class that has been unlocked yet?
  if (!has_unlocked_race(ch, race_num))
    return FALSE;

  // made it!
  return TRUE;
}

/*****************************/
/*****************************/

/* this will be a general list of all pc races */
void display_pc_races(struct char_data *ch)
{
  struct descriptor_data *d = ch->desc;
  int counter, columns = 0;

  write_to_output(d, "\r\n");

  for (counter = 0; counter < NUM_EXTENDED_RACES; counter++)
  {
    if (race_list[counter].is_pc)
    {
      write_to_output(d, "%s%-20.20s %s",
                      race_is_available(ch, counter) ? " " : "*",
                      race_list[counter].type,
                      !(++columns % 3) ? "\r\n" : "");
    }
  }

  write_to_output(d, "\r\n\r\n");
  write_to_output(d, "* - not unlocked 'accexp' for details\r\n");
  write_to_output(d, "\r\n");
}

/* display a specific races details */
bool display_race_info(struct char_data *ch, const char *racename)
{
  int race = -1, stat_mod = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  static int line_length = 80, i = 0;
  size_t len = 0;
  bool found = FALSE;

  skip_spaces_c(&racename);
  race = parse_race_long(racename);

  if (race == -1 || race >= NUM_EXTENDED_RACES)
  {
    /* Not found - Maybe put in a soundex list here? */
    return FALSE;
  }

  /* We found the race, and the race number is stored in 'race'. */
  /* Display the race info, formatted. */
  send_to_char(ch, "\tC\r\n");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tcRace Name       : \tn%s\r\n", race_list[race].type_color);
  send_to_char(ch, "\tcNormal/Adv/Epic?: \tn%s\r\n", (race_list[race].epic_adv == IS_EPIC_R) ? "Epic Race" : (race_list[race].epic_adv == IS_ADVANCE) ? "Advance Race"
                                                                                                                                                      : "Normal Race");
  send_to_char(ch, "\tcUnlock Cost     : \tn%d Account XP\r\n", race_list[race].unlock_cost);
  send_to_char(ch, "\tcPlayable Race?  : \tn%s\r\n", race_list[race].is_pc ? "\tnYes\tn" : "\trNo, ask staff\tn");

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* build buffer for ability modifiers */
  for (i = 0; i < NUM_ABILITY_MODS; i++)
  {
    stat_mod = race_list[race].ability_mods[i];
    if (stat_mod != 0)
    {
      found = TRUE;
      len += snprintf(buf + len, sizeof(buf) - len,
                      "%s %s%d ",
                      abil_mod_names[i], (stat_mod > 0) ? "+" : "", stat_mod);
    }
  }

  send_to_char(ch, "\tcRace Size       : \tn%s\r\n", sizes[race_list[race].size]);
  send_to_char(ch, "\tcAbility Modifier: \tn%s\r\n", found ? buf : "None");

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  snprintf(buf, sizeof(buf), "\tcDescription : \tn%s\r\n", race_list[race].descrip);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tYType: \tRrace feats %s\tY for this race's feat info.\tn\r\n",
               race_list[race].type);

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn");

  return TRUE;
}

/* function to view a list of feats race is granted */
bool view_race_feats(struct char_data *ch, const char *racename)
{
  int race = RACE_UNDEFINED;
  struct race_feat_assign *feat_assign = NULL;

  skip_spaces_c(&racename);
  race = parse_race_long(racename);

  if (race == RACE_UNDEFINED)
  {
    return FALSE;
  }

  /* level feats */
  if (race_list[race].featassign_list != NULL)
  {
    /*  This race has feat assignment! Traverse the list and list. */
    for (feat_assign = race_list[race].featassign_list; feat_assign != NULL;
         feat_assign = feat_assign->next)
    {
      if (feat_assign->level_received > 0) /* -1 is just race feat assign */
        send_to_char(ch, "Level: %-2d, Stacks: %-3s, Feat: %s\r\n",
                     feat_assign->level_received,
                     feat_assign->stacks ? "Yes" : "No",
                     feat_list[feat_assign->feat_num].name);
    }
  }
  send_to_char(ch, "\r\n");

  send_to_char(ch, "\tYType: \tRfeat info <feat name>\tY for detailed info about a feat.\r\n");

  return TRUE;
}

/**************************************/

/* entry point for race command - getting race info */
ACMDU(do_race)
{
  char arg[80];
  char racename[80];

  half_chop(argument, arg, racename);

  /* no argument, or general list of races */
  if (is_abbrev(arg, "list") || !*arg)
  {
    display_pc_races(ch);

    /* race info - specific info on given race */
  }
  else if (is_abbrev(arg, "info"))
  {

    if (!strcmp(racename, ""))
    {
      send_to_char(ch, "\r\nYou must provide the name of a race.\r\n");
    }
    else if (!display_race_info(ch, racename))
    {
      send_to_char(ch, "Could not find that race.\r\n");
    }

    /* race feat - list of free feats for given race */
  }
  else if (is_abbrev(arg, "feats"))
  {

    if (!strcmp(racename, ""))
    {
      send_to_char(ch, "\r\nYou must provide the name of a race.\r\n");
    }
    else if (!view_race_feats(ch, racename))
    {
      send_to_char(ch, "Could not find that race.\r\n");
    }
  }

  send_to_char(ch, "\tDUsage: race <list|info|feats> <race name>\tn\r\n");
}

/*****************************/
/*****************************/

/* here is the actual race list */
void assign_races(void)
{
  /* initialization */
  initialize_races();

  /* begin listing */

  /************/
  /* Humanoid */
  /************/

  /******/
  /* PC */
  /******/

#if defined(CAMPAIGN_DL)

/****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_HUMAN, "human", "Human", "\tRHuman\tn", "Humn", "\tRHumn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_HUMAN,
                   /*descrip*/
                   "Humans were among the first races created by "
                  "the gods. They represent the Neutral portion of "
                  "the triangle, and thus they were gifted with the "
                  "freedom to choose their own ethical and moral "
                  "paths. Due to their short lifespans, humans are "
                  "viewed by longer-lived races as ambitious and "
                  "impatient, restless and dissatisfied with their lot "
                  "in life. Humans live throughout Ansalon, with "
                  "cultures so diverse that the differences between "
                  "individual humans are as great as differences "
                  "between elves and dwarves. A race of extremes, "
                  "humankind keeps the great pendulum of history "
                  "constantly swaying between good and evil, law "
                  "and chaos. "
                  "Although each human culture differs from "
                  "every other, a basic distinction can be made "
                  "between so-called \"civilized\" human societies and "
                  "the primitive, nomadic tribes. Both cultures "
                  "believe their way of life to be superior to the "
                  "other. City dwellers think of the nomads as ignorant savages, while the tribesfolk look upon city "
                  "folk as soft and misguided ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Human.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Human.");
  set_race_genders(DL_RACE_HUMAN, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_HUMAN, 0, 0, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_HUMAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_HUMAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_HUMAN, FEAT_QUICK_TO_MASTER, 1, N);
  feat_race_assignment(DL_RACE_HUMAN, FEAT_SKILLED, 1, N);
  race_list[DL_RACE_HUMAN].racial_language = SKILL_LANG_COMMON;



  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_SILVANESTI_ELF, "silvanesti elf", "Silvanesti Elf", "\tGSilvanesti Elf\tn", "SvEl", "\tGSvEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_SILVANESTI_ELF,
                   // description
                   "Cool, aloof, and seemingly untouchable, the Silvanesti"
                    "elves represent all that is best and worst"
                    "in the elven people. Their haunting beauty is"
                    "marred by their cold and aloof natures. They"
                    "consider themselves better than all other people"
                    "on Ansalon, including their own kin, the Qualinesti"
                    "and Kagonesti. Being proud and arrogant, the Silvanesti"
                    "have little use for the members of any other"
                    "race, including other elves. Silvanesti are extremely"
                    "prejudiced against the cultures of “inferior”"
                    "people, and are intolerant of other customs"
                    "and beliefs. Silvanesti dislike change. Their society"
                    "has endured for more than 3,000 years, and"
                    "has changed very little in that time. When change"
                    "does occur, it is usually forced onto them. Slow to"
                    "trust and quick to blame, very few Silvanesti form"
                    "lasting friendships with non-Silvanesti.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Silvanesti Elf",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Silvanesti Elf");
  set_race_genders(DL_RACE_SILVANESTI_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_SILVANESTI_ELF, 0, 0, 1, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_SILVANESTI_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_SILVANESTI_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_HIGH_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_HIGH_ELF_CANTRIP, 1, N);
  feat_race_assignment(DL_RACE_SILVANESTI_ELF, FEAT_HIGH_ELF_LINGUIST, 1, N);
  race_list[DL_RACE_SILVANESTI_ELF].racial_language = SKILL_LANG_ELVEN;

  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_KAGONESTI_ELF, "kagonesti elf", "Kagonesti Elf", "\tBKagonesti Elf\tn", "KgEl", "\tBKgEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_KAGONESTI_ELF,
                   // description
                   "At home in the forests, the Kagonesti, or wild "
                    "elves, believe every creature and object, from "
                    "insects and birds to rivers and clouds, possesses a "
                    "spirit. They honor these spirits and know that, in "
                    "return, the spirits honor them. The Kagonesti believe that a happy "
                    "life can only truly be achieved by harmoniously "
                    "existing with nature. Passionate and proud, they "
                    "want only to be left to themselves. Due to the "
                    "expansion of human nations and mistreatment by "
                    "their own elven cousins, the Kagonesti have been "
                    "dragged from their forest homes and forced to live "
                    "in a world they do not like or understand. Unlike "
                    "their more placid kin, Kagonesti can be hottempered "
                    "and fierce when driven to extremes. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Kagonesti Elf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Kagonesti Elf.");
  set_race_genders(DL_RACE_KAGONESTI_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_KAGONESTI_ELF, 1, 0, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_KAGONESTI_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_KAGONESTI_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_WOOD_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_WOOD_ELF_FLEETNESS, 1, N);
  feat_race_assignment(DL_RACE_KAGONESTI_ELF, FEAT_WOOD_ELF_MASK_OF_THE_WILD, 1, N);
  race_list[DL_RACE_KAGONESTI_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_QUALINESTI_ELF, "qualinesti elf", "Qualinesti Elf", "\tYQualinesti Elf\tn", "QlEl", "\tYQlEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_QUALINESTI_ELF,
                   // description
                   "Of all the elven nations, the Qualinesti elves have "
                    "the most interaction with the other races of "
                    "Krynn. Though some Qualinesti prefer to remain "
                    "in their forest homes, others can be found exploring "
                    "the continent as merchants, priests, wizards, "
                    "and travelers. Because of their relatively long "
                    "life spans, Qualinesti accept the past without "
                    "regret and look forward to the future. They "
                    "patiently pursue their goals and have an optimistic "
                    "view of life. While they grieve for what has "
                    "been lost to them through the years, the Qualinesti "
                    "do not allow themselves to dwell on negative "
                    "emotions, preferring instead to look forward "
                    "to the next new day, the next new challenge. "
                    "The Qualinesti elves take pride in their abilities, "
                    "tending to look with disdain upon the "
                    "“crude” work by the obviously inferior races. "
                    "Although Qualinesti are more tolerant and outgoing "
                    "than the Silvanesti and relate well with "
                    "other races, the Qualinesti still consider themselves "
                    "the chosen of the gods. They are opposed "
                    "to interracial marriages and, although they may "
                    "offer sanctuary to half-elves, the half-elves are "
                    "never fully accepted by the Qualinesti. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Qualinesti Elf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Qualinesti Elf.");
  set_race_genders(DL_RACE_QUALINESTI_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_QUALINESTI_ELF, 0, 0, 0, 1, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_QUALINESTI_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_QUALINESTI_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_MOON_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_MOON_ELF_BATHED_IN_MOONLIGHT, 1, N);
  feat_race_assignment(DL_RACE_QUALINESTI_ELF, FEAT_MOON_ELF_LUNAR_MAGIC, 1, N);
  race_list[DL_RACE_QUALINESTI_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_HALF_ELF, "half elf", "Half Elf", "\tMHalf Elf\tn", "HElf", "\tMHElf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_HALF_ELF,
                   // description
                   "Since the time of Kith-Kanan, when elves first "
                   "began to interact extensively with human races, "
                   "elves and humans have fallen in love and married. "
                   "After the Cataclysm, human bandits and mercenaries "
                   "raided Qualinesti borders, looting the elven "
                   "lands, killing elven men and raping elven women. "
                   "Half-breed children are the result of both unions. "
                   "Whether born of love or hate, the mixed blood of "
                   "the half-elves forever brands them as outcasts "
                   "from both elven and human society. Half-elves inherit the best qualities "
                   "of both their parents. They have the love of "
                   "beauty and reverence for nature of the elves and "
                   "the ambition and drive of humans. Due, perhaps, "
                   "to the prejudice they face from both societies, "
                   "half-elves tend to be introverts and loners. "
                   "Scorned and belittled, some half-elves are insecure "
                   "and rebellious, lashing out at those who hate "
                   "them for what they are. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Elven.");
  set_race_genders(DL_RACE_HALF_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_HALF_ELF, 0, 0, 0, 0, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_HALF_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_HALF_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_HALF_BLOOD, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_ADAPTABILITY, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(DL_RACE_HALF_ELF, FEAT_HALF_ELF_RACIAL_ADJUSTMENT, 1, N);
  race_list[DL_RACE_HALF_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_KENDER, "kender", "Kender", "\tCKender\tn", "Kend", "\tCKend\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_KENDER,
                   // description
                   "To the other races, kender are the child-race of Krynn. The diminutive kender"
                   "have short attention spans, intense curiosity, and a fearlessness that serves"
                   "them well in battle, but often lands them (and those traveling with them) in"
                   "danger. Kender live a carefree existence where every new day is a day of"
                   "wonderful secrets just waiting to be discovered. Their most defining character"
                   "traits are their insatiable curiosity and their utter fearlessness, which makes"
                   "for a frightening combination. All dark caves need exploring, all locked doors"
                   "need opening, and all chests hide something interesting. Young kender around"
                   "the age of 20 or so are afflicted with \" wanderlust\", an intense desire to"
                   "depart their homeland and set out on a journey of discovery. Almost all kender"
                   "encountered outside the kender homelands are on wanderlust. Kender are"
                   "tantalized by the prospect of the new and exciting, and only the most extreme"
                   "circumstances force them to place their own selfpreservation above this"
                   "pursuit. Even the threat of imminent demise does not deter kender, for death is"
                   "the start of the next truly big adventure. The unquenchable curiosity of kender"
                   "drives them to investigate everything - including other people’s personal"
                   "possessions. Kender appropriate absolutely anything that catches their eye."
                   "Physical boundaries or notions of privacy are both alien concepts to them,"
                   "while the monetary value of an object means nothing to them. They are as likely"
                   "to be more captivated by the feather of goat-sucker bird as by a sapphire."
                   "Kender are never happier than when their hands are in the pockets, pouches, or"
                   "backpacks of those around them. Kender do not consider such appropriation to be"
                   "thievery as others understand it (kender are as contemptuous of thieves as the"
                   "next person). Kender term this \"handling\" or \"borrowing\" because they firmly"
                   "intend to return what they pilfer to the proper owner. It’s just that with so"
                   "many exciting and wonderful things going on in their lives, they forget to give"
                   "things back. Kender are at best bemused and at worst outraged at being accused"
                   "of theft or pick-pocketing. Kender always give perfectly reasonable"
                   "explanations for just about every accusation leveled at them. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Kender.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Kender.");
  set_race_genders(DL_RACE_KENDER, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_KENDER, -2, 0, 0, -2, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_KENDER, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_KENDER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_KENDER, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_WEAPON_PROFICIENCY_KENDER, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_KENDER_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_KENDER_SKILL_MOD, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_KENDER_BORROWING, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_KENDER_TAUNT, 1, N);
  feat_race_assignment(DL_RACE_KENDER, FEAT_KENDER_FEARLESSNESS, 1, N);
  race_list[DL_RACE_KENDER].racial_language = SKILL_LANG_COMMON;
  /* affect assignment */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_MOUNTAIN_DWARF, "mountain dwarf", "Mountain Dwarf", "\tJMountain Dwarf\tn", "MtDw", "\tJMtDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_MOUNTAIN_DWARF,
                   // desc
                   "The dwarves of the mountain kingdoms existed apart from much of Ansalon "
                  "throughout their history. Since their contact with the outside world often "
                  "turns out badly, the self-sufficient dwarves are quick to shut their gates and "
                  "seal off their halls to preserve the way of life that has sustained them since "
                  "the Age of Dreams. Mountain dwarves come from one of the following clans: Hylar "
                  "(\"Highest\"): This is the oldest of the dwarf clans, often considered the most "
                  "noble. Their halls within the mountain kingdoms are the best appointed and "
                  "always magnificent. Daewar (\"Dearest\"): Another highly respected clan, the "
                  "Daewar produce many important warriors and leaders. They are known for their "
                  "excellent fighting prowess and often work in conjunction with leaders of the "
                  "Hylar clan. Klar: The Klar were a clan of hill dwarves who were trapped inside "
                  "Thorbardin during the Cataclysm and not allowed to leave the mountain kingdom "
                  "when the dwarven kingdom was sealed from the inside. As a clan, they are known "
                  "for wild-looking eyes and wiry beards, though in truth their reputation as "
                  "madmen is largely undeserved. The mountain dwarves subjugated them as suspected "
                  "Neidar sympathizers during the Dwarfgate War (another unfairly leveled charge) "
                  "and since that time they have survived as a servitor clan. They are known as "
                  "fierce combatants and loyal friends. Though their position of servitude may be "
                  "unjust, many unflaggingly support their Hylar masters.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Mountain Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Mountain Dwarf.");
  set_race_genders(DL_RACE_MOUNTAIN_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_MOUNTAIN_DWARF, 2, 2, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_MOUNTAIN_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_MOUNTAIN_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_SHIELD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_SHIELD_DWARF_ARMOR_TRAINING, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_ARMOR_PROFICIENCY_LIGHT, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(DL_RACE_MOUNTAIN_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);
  race_list[DL_RACE_MOUNTAIN_DWARF].racial_language = SKILL_LANG_DWARVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_HILL_DWARF, "hill dwarf", "Hill Dwarf", "\tLHill Dwarf\tn", "HlDw", "\tLHlDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_HILL_DWARF,
                   // desc
                   "Hill dwarves have left their underground halls to practice their skills in the "
                  "greater world. Hill dwarves share the traits of their mountain dwarf cousins, "
                  "but are a bit more accepting of other races and cultures. All hill dwarves are "
                  "of the Neidar (\"Nearest\") clan. A longstanding, bitter feud exists between hill "
                  "dwarves and mountain dwarves, dating back to the Cataclysm. The hill dwarves "
                  "accuse the mountain dwarves of having shut the doors of Thorbardin on them when "
                  "the Neidar sought refuge following the Cataclysm. In their defense, the "
                  "mountain dwarves claim that they had resources enough to feed only their own "
                  "people and that, if they allowed the hill dwarves into the mountain, they all "
                  "might have starved.  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Hill Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Hill Dwarf.");
  set_race_genders(DL_RACE_HILL_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_HILL_DWARF, 0, 2, 0, 1, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_HILL_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_HILL_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_GOLD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_GOLD_DWARF_TOUGHNESS, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(DL_RACE_HILL_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);
  race_list[DL_RACE_HILL_DWARF].racial_language = SKILL_LANG_DWARVEN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_GULLY_DWARF, "gully dwarf", "Gully Dwarf", "\tLGully Dwarf\tn", "GuDw", "\tLGuDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_GULLY_DWARF,
                   // desc
                   "The Aghar ('Anguished'), or gully dwarves as most "
                  "races call them, are a misbegotten race of tough "
                  "survivors. Though gully dwarves themselves have "
                  "an extensive oral tradition (they love telling stories), "
                  "no two gully dwarf clans ever agree on the "
                  "exact details of their origins or history. The commonly "
                  "accepted tale of how gully dwarves came "
                  "to be is found within the annals of Astinus’s "
                  "Iconochronos. According to the Iconochronos, gully "
                  "dwarves are the result of breeding between "
                  "gnomes and dwarves in the years following the "
                  "transformation of the gnomes by the Graygem of "
                  "Gargath. The gnome-dwarf half-breeds appeared "
                  "to inherit the worst qualities of both races. The "
                  "unfortunate half-breeds were driven out of their "
                  "clans. Humans later christened them 'gully "
                  "dwarves,' reflecting their lowly status and poor "
                  "living conditions. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Hill Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Hill Dwarf.");
  set_race_genders(DL_RACE_GULLY_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_GULLY_DWARF, 0, 4, -4, 0, 4, -4);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_GULLY_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_GULLY_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_SURVIVAL_INSTINCT, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_PITIABLE, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_COWARDLY, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_GRUBBY, 1, N);
  feat_race_assignment(DL_RACE_GULLY_DWARF, FEAT_GULLY_DWARF_RACIAL_ADJUSTMENT, 1, N);
  race_list[DL_RACE_GULLY_DWARF].racial_language = SKILL_LANG_GULLYTALK;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_MINOTAUR, "minotaur", "Minotaur", "\tyMinotaur\tn", "Mntr", "\tyMntr\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_MINOTAUR,
                   // desc
                   "At home both on land and at sea, minotaurs live in an honor-based society where "
                   "strength determines power in both the gladiatorial arenas and in daily life. "
                   "Minotaurs believe in the superiority of their race above all "
                   "others. They believe their destiny is to rule the world. From youth, minotaurs "
                   "are trained in combat and warfare and instilled with a strict code of honor. "
                   "The militaristic society of minotaurs gives them a rigid view of the world, "
                   "clearly delineated in black and white. Minotaurs value strength, cunning, and "
                   "intelligence. The ultimate test of all three virtues is conducted in the Great "
                   "Circus, an annual contest held in a gladiatorial arena. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Minotaur.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Minotaur.");
  set_race_abilities(DL_RACE_MINOTAUR, 4, 0, -2, 0, -2, 0);         /* str con int wis dex cha */
  set_race_alignments(DL_RACE_MINOTAUR, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_MINOTAUR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_MINOTAUR_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_MINOTAUR_TOUGH_HIDE, 1, N);
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_MINOTAUR_INTIMIDATING, 1, N);
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_MINOTAUR_SEAFARING, 1, N);
  feat_race_assignment(DL_RACE_MINOTAUR, FEAT_MINOTAUR_GORE, 1, N);
  race_list[DL_RACE_MINOTAUR].racial_language = SKILL_LANG_MINOTAUR;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_GNOME, "gnome", "Gnome", "\tDGnome\tn", "Gnom", "\tDGnom\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_GNOME,
                   // Description
                   "Gnomes are the tinkers and inventors of Krynn. Fast thinking and fast speaking, "
                    "their minds are forever fixed on cogs, gears, wheels, bells, whistles, and "
                    "steam-powered engines. Despite the dangers inherent in their work, gnomes "
                    "(sometimes called \"tinker\" gnomes) adore technology and continue throughout the "
                    "ages to pursue and perfect their inventions. Personality: Inventive, skillful, "
                    "and enthusiastic, gnomes are devoted to making life easier through technology, "
                    "though their complex inventions usually have the exact opposite effect. Science "
                    "is a gnome's life, so much so that every gnome chooses a special Life Quest "
                    "upon reaching adulthood. More important than family ties, the Life Quest "
                    "defines the gnome. The Life Quest is always related to furthering knowledge or "
                    "developing technology. The goal is specific and usually out of reach. It is not "
                    "uncommon for Life Quests to be handed down from one generation to the next "
                    "multiple times before it is achieved. Successful completion of a Life Quest "
                    "ensures the gnome, and any forebear working on the same quest, a place in the "
                    "afterlife with Reorx. Only one gnome was ever able to complete three separate "
                    "Life Quests in his own lifetime, and he was deemed a mad gnome and cast out of "
                    "Mount Nevermind for making everyone else look bad. The gnome dedication to "
                    "knowledge and invention leaves them sadly lacking in the social graces, at "
                    "least when it concerns other races. Gnomes are always eager to discuss projects "
                    "and compare notes, and in their hurry to explain what they mean, they often "
                    "forget to be polite. Gnomes do care for other people's feelings, but they're "
                    "typically focused on another matter entirely by the time it occurs to them that "
                    "they were rude. The worse thing in the world (at least in the minds of other "
                    "races) is a gnome apology. Believing that action speaks louder than words, a "
                    "gnome making an apology will build an invention specifically for the injured "
                    "party. All too often, this invention ends  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Gnome.");
  set_race_genders(DL_RACE_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_GNOME, 0, 2, 2, -2, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_GNOME, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_GNOME,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_GNOME, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_RESISTANCE_TO_ILLUSIONS, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_ILLUSION_AFFINITY, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_TINKER_FOCUS, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_ROCK_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_ARTIFICERS_LORE, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_TINKER, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_GNOMISH_TINKERING, 1, N);
  feat_race_assignment(DL_RACE_GNOME, FEAT_BRILLIANCE_AND_BLUNDER, 1, N);
  race_list[DL_RACE_GNOME].racial_language = SKILL_LANG_GNOME;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_GOBLIN, "goblin", "Goblin", "\tgGoblin\tn", "Gobn", "\tgGobn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_GOBLIN,
                   // Description
                   "The Goblins of Ansalon are little, thin humanoids, standing no more than three  "
                    "and half feet in height. Their skin tones vary greatly. They have flat faces and "
                    "dark, stringy hair. They have dull red or yellow eyes. They are very quick and  "
                    "have sharp teeth. Goblins dress in leathers made from animal hide or scavenged  "
                    "clothing, and speak the Goblin Language. "
                    "Goblins respect and cherish those who have power and are strong. They are  "
                    "ambitious, seeking power for themselves but most never attain it. When in large  "
                    "groups they tend to fall prey to mob mentality and in some cases, operate like a "
                    "wolf pack. Goblins back a strong leader and will follow their lead. A lone  "
                    "goblin may appear weak and vulnerable but typically they are confident and sure  "
                    "of themselves. "
                    "Goblins live in tribes when their larger cousins, Bugbears and Hobgoblins, are  "
                    "not dominating them. Leaders, called Rukras, lead either through trickery or  "
                    "strength. Though nasty and callous, they have the best interests of their people "
                    "in their hearts. "
                    "Goblins are simple beings but are led by their cultural imperatives to fight,  "
                    "kill and do what the larger, meaner goblins tell them to do. They are numerous  "
                    "and are spread across the interior of Ansalon, from the far north to the colds  "
                    "of Icereach. They continually struggle against the other races to survive. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Gnome.");
  set_race_genders(DL_RACE_GOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_GOBLIN, -2, 2, 0, 0, 4, -2);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_GOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_GOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_GOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_NIMBLE_ESCAPE, 1, N);
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_FAST_MOVEMENT, 1, N);
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(DL_RACE_GOBLIN, FEAT_FURY_OF_THE_SMALL, 1, N);
  race_list[DL_RACE_GOBLIN].racial_language = SKILL_LANG_GOBLIN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_HOBGOBLIN, "hobgoblin", "Hobgoblin", "\tmHobgoblin\tn", "HobG", "\tmHobG\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_HOBGOBLIN,
                   // Description
                   "The Hobgoblins of Krynn are usually over six feet in height, can have a ruddy  "
                    "yellow, tan, dark red, or red-orange skin tone, brown-gray hair that covers  "
                    "their bodies, large pointed ears, and flattened, vaguely batlike faces. They  "
                    "closely resemble their smaller cousins the Goblins but are bigger and uglier,  "
                    "and are much smarter. They are faster than a Human, and have a much stronger  "
                    "endurance. Hobgoblins bore easily and will pick fights with their inferiors, but "
                    "will protect party members that are weaker than their opponents. "
                    "Most hobgoblins thrive on war, terror, and an impulse to oppose all other races. "
                    "There are some though that are understanding of civilization, and want to bring  "
                    "this to their goblin kin. These hobgoblins are called donek, or renegades in the "
                    "goblin language. The most famous donek is Lord Toede. "
                    "Hobgoblins live in semi-nomadic auls, or tribes, and are led by a murza. The  "
                    "murza will have a troop of assassins, shamans, bodyguards, and always a rival  "
                    "who wants to kill the current murza. Hobgoblins almost always defer to a  "
                    "bugbear, but will dominate any goblins in their group even going to live with  "
                    "larger goblin tribes to serve as leaders. The auls are usually dedicated to  "
                    "conquest and warfare. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Hobgoblin.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Hobgoblin.");
  set_race_genders(DL_RACE_HOBGOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(DL_RACE_HOBGOBLIN, 0, 2, 0, 0, 1, 0);           /* str con int wis dex cha */
  set_race_alignments(DL_RACE_HOBGOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_HOBGOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_HOBGOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_HOBGOBLIN, FEAT_HOBGOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(DL_RACE_HOBGOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(DL_RACE_HOBGOBLIN, FEAT_AUTHORITATIVE, 1, N);
  feat_race_assignment(DL_RACE_HOBGOBLIN, FEAT_FORTUNE_OF_THE_MANY, 1, N);
  race_list[DL_RACE_HOBGOBLIN].racial_language = SKILL_LANG_GOBLIN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_BAAZ_DRACONIAN, "baaz draconian", "Baaz Draconian", "\tWBaaz Draconian\tn", "Baaz", "\tWBaaz\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(DL_RACE_BAAZ_DRACONIAN,
                   // desc
                   "Baaz Draconians are the smallest of their brethren, made from Brass Dragon "
                  "eggs, and are commonly used as ground troops. They are usually of chaotic "
                  "alignment and as a result are interested in getting what is best for them "
                  "individually. Baaz wear disguises often, concealing their wings and scales "
                  "underneath large hoods and masks when traveling through non-draconian lands. " 
                  "When a Baaz is killed, its body turns to stone and releases a puff of gas "
                  "that paralyzes all nearby enemies.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Baaz Draconian.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Baaz Draconian.");
  set_race_abilities(DL_RACE_BAAZ_DRACONIAN, 2, 1, 0, 0, 0, 0);         /* str con int wis dex cha */
  set_race_alignments(DL_RACE_BAAZ_DRACONIAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_BAAZ_DRACONIAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_BAAZ_DEATH_THROES, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_DRACONIAN_CONTROLLED_FALL, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_DRACONIC_DEVOTION, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_DRACONIAN_GALLOP, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_DRACONIAN_DISEASE_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_BAAZ_DRACONIAN_SCALES, 1, N);
  feat_race_assignment(DL_RACE_BAAZ_DRACONIAN, FEAT_DRACONIAN_BITE, 1, N);
  race_list[DL_RACE_BAAZ_DRACONIAN].racial_language = SKILL_LANG_DRACONIC;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_KAPAK_DRACONIAN, "kapak draconian", "Kapak Draconian", "\tWKapak Draconian\tn", "Kapk", "\tWKapk\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 5000, IS_ADVANCE);
  set_race_details(DL_RACE_KAPAK_DRACONIAN,
                   // desc
                   "Kapak draconians are known for stealth, vicious"
                  "cunning, and for licking their blades with their"
                  "venom-soaked tongues before battle. While not"
                  "known for original thinking, kapaks exhibit cruel"
                  "creativity in carrying out assigned missions of"
                  "espionage and murder. Kapaks like structure in their"
                  "lives, and the military lifestyle suits them. Their"
                  "natural talents for stealth and butchery belie their"
                  "need for order. Many become assassins, because"
                  "they are adept at handling dangerous and constantly"
                  "changing situations. Female kapaks also"
                  "like structure, but they tend to be more nurturing,"
                  "using their inborn healing abilities to aid"
                  "other draconians. Kapaks are larger"
                  "and more draconic than baaz, with elongated"
                  "reptilian snouts, sharp-toothed maws, and"
                  "horned heads. They possess two large glands in"
                  "their mouths that produce either poison (males)"
                  "or a magical healing saliva (females). They have"
                  "scaly, green-tinged coppery hide, and sport a"
                  "pair of wings that extend 6 feet to each side"
                  "when outstretched.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Kapak Draconian.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Kapak Draconian.");
  set_race_abilities(DL_RACE_KAPAK_DRACONIAN, 0, 2, 0, 0, 4, 0);         /* str con int wis dex cha */
  set_race_alignments(DL_RACE_KAPAK_DRACONIAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_KAPAK_DRACONIAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_KAPAK_DEATH_THROES, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIAN_CONTROLLED_FALL, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIC_DEVOTION, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIAN_GALLOP, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIAN_DISEASE_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_KAPAK_DRACONIAN_SCALES, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIAN_BITE, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_KAPAK_SALIVA, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_SNEAK_ATTACK, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_APPLY_POISON, 1, N);
  feat_race_assignment(DL_RACE_KAPAK_DRACONIAN, FEAT_DRACONIAN_SPELL_RESISTANCE, 1, N);

  race_list[DL_RACE_KAPAK_DRACONIAN].racial_language = SKILL_LANG_DRACONIC;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(DL_RACE_BOZAK_DRACONIAN, "bozak draconian", "Bozak Draconian", "\tWBozak Draconian\tn", "Bozk", "\tWBozk\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 5000, IS_ADVANCE);
  set_race_details(DL_RACE_BOZAK_DRACONIAN,
                   // desc
                   "Possessed of magical talent and strong wills, bozaks are natural leaders -"
                    "often willing to give their lives for a cause they deem worthy, but intelligent"
                    "enough to fight to survive and win. Many bozaks have strong religious"
                    "tendencies, even though they are inherently talented in arcane magic. Bozaks"
                    "have sharp, tactically sound minds, and honing skills that bestow high"
                    "survivability. Most bozaks instinctively take charge of a situation and are"
                    "suited to military command, and in this they often excel. Bozaks associate all"
                    "magic with the gods and quite often have a reverence attached to both their"
                    "innate and learned magical talents, though if they feel betrayed by divine"
                    "forces they may hold a grudge that will last a lifetime. Bozaks are tall, with"
                    "bronze-hued scales-as tiny as fish scales on the draconian's face, growing to"
                    "the size of a bronze piece elsewhere on its body. Bozaks sport a pair of"
                    "curved, ram-like horns on top of their heads. They have small, dragon-like"
                    "wings they can use to glide.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Bozak Draconian.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Bozak Draconian.");
  set_race_abilities(DL_RACE_BOZAK_DRACONIAN, 2, 0, 2, 0, 0, 2);         /* str con int wis dex cha */
  set_race_alignments(DL_RACE_BOZAK_DRACONIAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(DL_RACE_BOZAK_DRACONIAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_BOZAK_DEATH_THROES, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIAN_CONTROLLED_FALL, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIC_DEVOTION, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIAN_GALLOP, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIAN_DISEASE_IMMUNITY, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_BOZAK_DRACONIAN_SCALES, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIAN_BITE, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_DRACONIAN_SPELL_RESISTANCE, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_BOZAK_SPELLCASTING, 1, N);
  feat_race_assignment(DL_RACE_BOZAK_DRACONIAN, FEAT_BOZAK_LIGHTNING_DISCHARGE, 1, N);

  race_list[DL_RACE_BOZAK_DRACONIAN].racial_language = SKILL_LANG_DRACONIC;

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LICH, "lich", "Lich", "\tLLich\tn", "Lich", "\tLLich\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 60000, IS_EPIC_R);
  set_race_details(RACE_LICH,
                   /*descrip*/ "Few creatures are more feared than the lich. The pinnacle of necromantic art, who "
                               "has chosen to shed his life as a method to cheat death by becoming undead. While many who reach "
                               "such heights of power stop at nothing to achieve immortality, the idea of becoming a lich is "
                               "abhorrent to most creatures. The process involves the extraction of ones life-force and its "
                               "imprisonment in a specially prepared phylactery.  One gives up life, but in trapping "
                               "life he also traps his death, and as long as his phylactery remains intact he can continue on in "
                               "his research and work without fear of the passage of time."
                               "\r\n\r\n"
                               "The quest to become a lich is a lengthy one. While construction of the magical phylactery to "
                               "contain ones soul is a critical component, a prospective lich must also learn the "
                               "secrets of transferring his soul into the receptacle and of preparing his body for the "
                               "transformation into undeath, neither of which are simple tasks. Further complicating the ritual "
                               "is the fact that no two bodies or souls are exactly alike, a ritual that works for one spellcaster "
                               "might simply kill another or drive him insane. "
                               "\r\n\r\n"
                               "Please note that a Lich has all the advantages/disadvantages of being Undead.\r\n  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Lich.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Lich.");
  set_race_genders(RACE_LICH, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_LICH, 0, 2, 6, 2, 2, 6);           /* str con int wis dex cha */
  set_race_alignments(RACE_LICH, N, N, N, N, N, N, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_LICH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, Y, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_LICH, FEAT_UNARMED_STRIKE, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_IMPROVED_UNARMED_STRIKE, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_HARDY, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_SPELL_RESIST, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_DAM_RESIST, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_TOUCH, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_REJUV, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_FEAR, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_ELECTRIC_IMMUNITY, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_COLD_IMMUNITY, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_VAMPIRE, "vampire", "Vampire", "\tLVampire\tn", "Vamp", "\tLVamp\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 60000, IS_EPIC_R);
  set_race_details(RACE_VAMPIRE,
                   /*descrip*/ "Vampires are one of the most fearsome of the Undead creatures in Krynn. With unnatural strength, "
                               " agility and cunning, they can easily overpower most other creatures with their physical "
                               "prowess alone. But the vampire is much more deadly than just his claws and wits. Vampires have "
                               "a number of supernatural abilities that inspire dread in his foes.  Gaining sustenance from "
                               "the blood of the living, vampires can heal quickly from almost any wound. For the victims "
                               "of their feeding, they may raise again as vampiric spawn... an undead creature under the vampire's "
                               "control with many vampiric abilities of their own. They may also call animal minions to aid them "
                               "in battle, from wolves, to swarms of rats and vampire bats as well. They can dominate intelligent "
                               "foes with a simple gaze, and they may drain the energy of living beings with an unarmed attack. "
                               "They can also assume the form of a wolf or a giant bat, as well as assume a gasoeus form at will, "
                               "and have the ability to scale sheer surfaces as easily as a spider may."
                               "\r\n\r\n"
                               "But a vampire is not without its weaknesses. Exposed to sunlight, they will quickly be reduced "
                               "to ash, and moving water is worse, able to kill a vampire submerged in running water in less than a minute."
                               "\r\n\r\n"
                               "Being a vampire is a state most would consider a curse, however there are legends of those who "
                               "sought out the 'gift' of vampirism, with some few who actually obtained it. To this day however, "
                               "such secrets have been lost to the ages. However these are the days of great heroes and villains, "
                               "and such days often bring to light secrets of the past. Perhaps one day soon the legends may become "
                               "reality."
                               "\r\n\r\n"
                               "Please note that a Vampire has all the advantages/disadvantages of being Undead.\r\n  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Vampire.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Vampire.");
  set_race_genders(RACE_VAMPIRE, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_VAMPIRE, 6, 4, 2, 2, 4, 4);           /* str con int wis dex cha */
  set_race_alignments(RACE_VAMPIRE, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_VAMPIRE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, Y, N, N, N, Y, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, Y, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_VAMPIRE, FEAT_ALERTNESS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_COMBAT_REFLEXES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_DODGE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_IMPROVED_INITIATIVE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_LIGHTNING_REFLEXES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_TOUGHNESS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_NATURAL_ARMOR, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_DAMAGE_REDUCTION, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ENERGY_RESISTANCE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_FAST_HEALING, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_WEAKNESSES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_BLOOD_DRAIN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CREATE_SPAWN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_DOMINATE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ENERGY_DRAIN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CHANGE_SHAPE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_GASEOUS_FORM, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_SPIDER_CLIMB, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_SKILL_BONUSES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ABILITY_SCORE_BOOSTS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_BONUS_FEATS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_HARDY, 1, N);

#else

#ifdef CAMPAIGN_FR
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HUMAN, "human", "Human", "\tRHuman\tn", "Humn", "\tRHumn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HUMAN,
                   /*descrip*/
                   "Humans dwell in every corner of Tori! and encompass a full range of cultures and ethnicities. Along the Sword "
                   "Coast and across the North, humans are the most pervasive of the races and in many places the most dominant. "
                   "Their cultural and societal makeup runs the gamut, from the cosmopolitan folk who reside in great cities "
                   "such as Baldur's Gate and Waterdeep to the barbarians who rage throughout the Savage Frontier. "
                   "Humans are famous for their adaptability. No other race lives in so many diverse lands or environments, "
                   "from lush jungles to burning deserts, from the eternal cold of the Great Glacier to the fertile shores along "
                   "rivers and seas. Humans find ways to survive and to thrive almost anywhere. In locations where elves "
                   "and dwarves have withdrawn, humans often move in and build anew a longside or on top of an earlier "
                   "community.\r\n\r\n"
                   "It follows, then, that the most common feature of humans is their lack of commonality. This diversity has "
                   "enabled human civilizations to grow faster than those of other races, making humans one of the dominant races "
                   "in much of the world today. It has also led to conflicts between communities of humans because of their cultural "
                   "and political differences. If not for their penchant for infighting, humans would be even more populous and "
                   "predominant than they already are.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Human.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Human.");
  set_race_genders(RACE_HUMAN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HUMAN, 0, 0, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HUMAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HUMAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HUMAN, FEAT_QUICK_TO_MASTER, 1, N);
  feat_race_assignment(RACE_HUMAN, FEAT_SKILLED, 1, N);
  race_list[RACE_HUMAN].racial_language = SKILL_LANG_COMMON;
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /**TEST**/ affect_assignment(RACE_HUMAN, AFF_DETECT_ALIGN, 1);
  /****************************************************************************/
  
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HIGH_ELF, "high elf", "High Elf", "\tGHigh Elf\tn", "HiEl", "\tGHiEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HIGH_ELF,
                   // description
                   "High elves, also known as eladrin, were graceful warriors and wizards who  "
                   "originated from the realm of Faerie, also known as the Feywild. They "
                   "lived in the forests of the world. They were magical in nature and shared an "
                   "interest in the arcane arts. From an early age they also learned to defend "
                   "themselves, particularly with swords. "
                   "High elves were graceful, intelligent beings, with a greater capacity for "
                   "intelligence than most humanoid races while also possessing an agility  "
                   "comparable with their elven kin. High elves were also unusually strong- "
                   "willed and had a natural resistance to the effects of enchantment spells. "
                   "High elves also had no need for sleep in the same way most humanoids did, "
                   "instead going into a trance. While in a trance, high elves remained fully "
                   "aware of their immediate surroundings. Furthermore, high elves needed only "
                   "rest for four hours to get the same effect that most other humanoids got from "
                   "six hours of sleep. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes High Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes High Elven.");
  set_race_genders(RACE_HIGH_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HIGH_ELF, 0, 0, 1, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HIGH_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HIGH_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HIGH_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_CANTRIP, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_LINGUIST, 1, N);
  race_list[RACE_HIGH_ELF].racial_language = SKILL_LANG_ELVEN;

  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_WOOD_ELF, "wood elf", "Wood Elf", "\tBWood Elf\tn", "WdEl", "\tBWdEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_WOOD_ELF,
                   // description
                   "Wood elves, also known as copper elves, or Or-tel-quessir were the most "
                   "populous of the elven races. Wood elves saw themselves as guardians of the "
                   "Tel-quessir forest homes that were largely abandoned after the Crown Wars and "
                   "before the Retreat, but unlike most elves they did not view themselves as a  "
                   "people apart from the rest of Faerun. "
                   "Wood elves were easily identifiable by their coppery skin and green, brown, or "
                   "hazel eyes. Wood elven hair was usually black or brown, although hues such as "
                   "blond or copper red were also found. Wood elves tended to dress in simple  "
                   "clothes, similar to those of the moon elves but with fewer bold colors and a "
                   "greater number of earth tones that blended into their natural surroundings. "
                   "Accustomed to a harsh, naturalistic lifestyle, wood elves loved to wear  "
                   "leather armor, even when they were not under immediate threat. Wood elves "
                   "were roughly identical to other elves in height and build, with males larger "
                   "than females. "
                   "Wood elves were often stronger than other Tel-quessir, including other elves, "
                   "but were frequently less cerebral than moon and sun elves, who put a greater "
                   "value on education. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Wood Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Wood Elven.");
  set_race_genders(RACE_WOOD_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_WOOD_ELF, 1, 0, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_WOOD_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_WOOD_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_WOOD_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_FLEETNESS, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_MASK_OF_THE_WILD, 1, N);
  race_list[RACE_WOOD_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_MOON_ELF, "moon elf", "Moon Elf", "\tYMoon Elf\tn", "MnEl", "\tYMnEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_MOON_ELF,
                   // description
                   "Moon elves, also known as Teu-tel-quessir in the elven language or as silver "
                   "elves, were the most common of all the elven subraces. More tolerant of humans "
                   "than other elves, moon elves were the ancestors of most half-elves. They were "
                   "considered high elves and among the eladrin. "
                   "Like all elves, the Teu-tel-quessir were tall, close to humans in height, but "
                   "more slender and beautiful. Moon elf skin was pale, often with an icy blue hue. "
                   "Moon elf hair was commonly black, blue, or silvery white, although human-like "
                   "colors were heard of as well, though very rare. Moon elf eyes, like those of "
                   "other elves, were very commonly green, although some were blue as well. "
                   "Of all the elven races, moon elves were the most impulsive, with a strong "
                   "distaste for complacency or isolation. Moon elves longed to be on the road, "
                   "traveling and exploring the untamed wilderness that lay between cities and "
                   "nations. This extroverted quality was part of the reason why moon elves got "
                   "along uncommonly well with other races and had come to the conclusion that "
                   "the N-Tel-Quess were not necessarily as foolhardy or unworthy as their brethren "
                   "might think. Moon elves, rather than feeling that interaction outside of their "
                   "race diminished or weakened them, believed that interacting with other races, "
                   "humans in particular, was the best way to spread the values of the Tel-quessir "
                   "races, thereby strengthening their culture. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Moon Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Moon Elven.");
  set_race_genders(RACE_MOON_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_MOON_ELF, 0, 0, 0, 1, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_MOON_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_MOON_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_MOON_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_BATHED_IN_MOONLIGHT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_LUNAR_MAGIC, 1, N);
  race_list[RACE_MOON_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_ELF, "half elf", "Half Elf", "\tMHalf Elf\tn", "HElf", "\tMHElf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_ELF,
                   // description
                   "Half-elves, as usually defined, were humanoids born through the union of an elf "
                   "and a human. Whether a half-elf was raised by their human parent or their elven "
                   "parent, they often felt isolated and alone. Because they took around twenty "
                   "years to reach adulthood, they matured quickly when raised by elves (who "
                   "think they look like humans), making them feel like an outsider in either "
                   "place. "
                   "Most half-elves were descended from moon elves. Pairings of elves and other "
                   "races also existed, though they were rare. "
                   "Half elves stood roughly around 5 feet 5 inches to six feet two inches (1.65 - "
                   "1.88 meters), making them only slightly shorter overall than humans, and "
                   "weighed in at 130 - 190 lbs. (59 - 86 kg), making them heavier than elves but "
                   "still considerably lighter than humans. Like humans, half-elves had a wide "
                   "variety of complexions, some of which were inherited from the elven half of "
                   "their heritage, such as a tendency for metallic-hued skin and inhuman hair "
                   "colors. "
                   "Unlike true Tel'Quessir, however, male half-elves were capable of growing "
                   "facial hair and often did so to distinguish themselves, in part, from their "
                   "elven parents. Half-elven ears were about the size of human ones, but like "
                   "elves, they were pointed on the ends. Half-elves were also notably more "
                   "durable and passionate than either elves or humans, a unique result of the "
                   "two races' blending.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Elven.");
  set_race_genders(RACE_HALF_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_ELF, 0, 0, 0, 0, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_HALF_BLOOD, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_ADAPTABILITY, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_HALF_ELF_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_HALF_ELF].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */

  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_DROW, "half drow", "Half Drow", "\tCHalf Drow\tn", "HDrw", "\tCHDrw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_DROW,
                   // description
                   "A half-drow was the offspring of one human parent and one drow parent. A half-"
                   "drow generally had dusky skin, silver or white hair, and human eye colors. They "
                   "could see around 60 feet (18 m) with darkvision, but otherwise had no other "
                   "known drow traits or abilities. "
                   "Half-drow were most commonly encountered in Dambrath and in the Underdark. "
                   "House Ousstyl, in particular, was known for having mated with humans. "
                   "Half-drow were often conceived when a male drow mated with one of his human "
                   "female slaves or when outcast drow mated with surface-dwelling species. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Drow.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Drow.");
  set_race_genders(RACE_HALF_DROW, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_DROW, 0, 0, 2, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_DROW, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_DROW,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_DROW, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_WEAPON_PROFICIENCY_DROW, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_BLOOD, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_DROW_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  race_list[RACE_HALF_DROW].racial_language = SKILL_LANG_UNDERCOMMON;
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DRAGONBORN, "dragonborn", "Dragonborn", "\tWDragonborn\tn", "DrgB", "\tWDrgB\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_DRAGONBORN,
                   /*descrip*/
                   "Dragonborn (also known as Strixiki in Draconic; or Vayemniri, \"Ash-Marked  "
                   "Ones\", in Tymantheran draconic) were a race of draconic creatures native to "
                   "Abeir, Toril's long-sundered twin. During the Spellplague, dragonborn were  "
                   "transplanted from Abeir to Toril, the majority of them living in the continent "
                   "of Laerakond in the 15th century DR. In Faerun, most dragonborn dwelt in the "
                   "militaristic nation of Tymanther. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Dragonborn.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Dragonborn.");
  set_race_genders(RACE_DRAGONBORN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DRAGONBORN, 2, 0, 0, 0, 0, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_DRAGONBORN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DRAGONBORN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_BREATH, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_RESISTANCE, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_FURY, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_DRAGONBORN].racial_language = SKILL_LANG_DRACONIC;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TIEFLING, "tielfing", "Tiefling", "\tATiefling\tn", "Tief", "\tATief\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_TIEFLING,
                   /*descrip*/
                   "Tieflings were human-based planetouched, native outsiders that were infused "
                   "with the touch of the fiendish planes, most often through descent from fiends-"
                   "demons, Yugoloths, devils, evil deities, and others who had bred with humans. "
                   "Tieflings were known for their cunning and personal allure, which made them  "
                   "excellent deceivers as well as inspiring leaders when prejudices were laid  "
                   "aside. Although their evil ancestors could be many generations removed, the "
                   "taint lingered. Unlike half-fiends, tieflings were not predisposed to evil "
                   "alignments and varied in alignment nearly as widely as full humans, though "
                   "tieflings were certainly devious. Tieflings tended to have an unsettling air "
                   "about them, and most people were uncomfortable around them, whether they were "
                   "aware of the tiefling's unsavory ancestry or not. While some looked like normal "
                   "humans, most retained physical characteristics derived from their ancestor, "
                   "with the most common such features being horns, prehensile tails, and pointed "
                   "teeth. Some tieflings also had eyes that were solid orbs of black, red, white, "
                   "silver, or gold, while others had eyes more similar to those of humans. Other, "
                   "more unusual characteristics included a sulfurous odor, cloven feet, or a "
                   "general aura of discomfort they left on others. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Tiefling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Tiefling.");
  set_race_genders(RACE_TIEFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_TIEFLING, 0, 0, 1, 0, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_TIEFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_TIEFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_TIEFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_HELLISH_RESISTANCE, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_BLOODHUNT, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_MAGIC, 1, N);
  race_list[RACE_TIEFLING].racial_language = SKILL_LANG_ABYSSAL;

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_AASIMAR, "aasimar", "Aasimar", "\tWAasimar\tn", "Asmr", "\tWAsmr\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_AASIMAR,
                   /*descrip*/
                   "Aasimar were human-based planetouched, native outsiders that had in their blood "
                   "some good, otherworldly characteristics. They were often, but not always, descended "
                   "from celestials and other creatures of pure good alignment, but while predisposed to "
                   "good alignments, aasimar were by no means always good. Aasimar bore the mark of their "
                   "celestial touch through many different physical features that often varied from "
                   "individual to individual. Most commonly, aasimar were very similar to humans, like "
                   "tieflings and other planetouched. Nearly all aasimar were uncommonly beautiful and "
                   "still, and they were often significantly taller than humans as well.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes an Aasimar.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes an Aasimar.");
  set_race_genders(RACE_AASIMAR, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_AASIMAR, 0, 0, 0, 1, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_AASIMAR, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_AASIMAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_AASIMAR, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_ASTRAL_MAJESTY, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_CELESTIAL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_HEALING_HANDS, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_LIGHT_BEARER, 1, N);
  race_list[RACE_AASIMAR].racial_language = SKILL_LANG_CELESTIAL;

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TABAXI, "tabaxi", "Tabaxi", "\tyTabaxi\tn", "Tbxi", "\tyTbxi\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_TABAXI,
                   /*descrip*/
                   "Tabaxi are taller than most humans at six to seven feet. Their bodies are slender "
                   "and covered in spotted or striped fur. Like most felines, Tabaxi had long tails and "
                   "retractable claws. Tabaxi fur color ranged from light yellow to brownish red. Tabaxi "
                   "eyes are slit-pupilled and usually green or yellow. Tabaxi are competent swimmers and "
                   "climbers as well as speedy runners. They had a good sense of balance and an acute sense "
                   "of smell. Depending on their region and fur coloration, tabaxi are known by different "
                   "names. Tabaxi with solid spots are sometimes called leopard men and tabaxi with rosette "
                   "spots are called jaguar men. The way the tabaxi pronounced their own name also varied; "
                   "the 'leopard men' pronounced it ta-BÆK-see, and the jaguar men tah-BAHSH-ee. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Tabaxi.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Tabaxi.");
  set_race_genders(RACE_TABAXI, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_TABAXI, 1, 0, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_TABAXI, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_TABAXI,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_TABAXI, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_CATS_CLAWS, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_CATS_TALENT, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_FELINE_AGILITY, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_SHIELD_DWARF, "shield dwarf", "Shield Dwarf", "\tJShield Dwarf\tn", "ShDw", "\tJShDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_SHIELD_DWARF,
                   // desc
                   "Shield dwarves, also known as mountain dwarves, are among the most common of "
                   "the dwarven peoples. Once the rulers of mighty kingdoms across Faerun, the "
                   "shield dwarves have since fallen by the wayside after centuries of warfare "
                   "with their goblinoid enemies. Since then, shield dwarves have been less "
                   "commonly seen throughout Faerun, though during the Era of Upheaval the subrace, "
                   "spurred on by the Thunder Blessing, began to retake an important role in local "
                   "politics. Shield dwarves are a cynical and gruff people, but they are not, "
                   "despite a reputation to the contrary, fatalistic, still possessing some hope "
                   "for the future. Typically, shield dwarves take time to trust and even longer "
                   "to forgive but the dwindling of their race has led many to be more open to "
                   "other ways of thinking. Traditionally, clan allegiance and caste placement "
                   "meant everything among the shield dwarves but as their civilization has "
                   "declined so has the importance of these identity constructs. Although  "
                   "bloodline is still a mark of pride for a dwarf from a particularly strong "
                   "clan, personal accomplishments have come to mean more practically than the old "
                   "ways, which seem increasingly irrelevant. Among the Hidden, traditions remain "
                   "strong, but there is an increasing number of shield dwarves willing to leave "
                   "the mountains for a life as adventurers or craftsmen among humans. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Shield Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Shield Dwarf.");
  set_race_genders(RACE_SHIELD_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_SHIELD_DWARF, 1, 2, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_SHIELD_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_SHIELD_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_SHIELD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_SHIELD_DWARF_ARMOR_TRAINING, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ARMOR_PROFICIENCY_LIGHT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);
  race_list[RACE_SHIELD_DWARF].racial_language = SKILL_LANG_DWARVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOLD_DWARF, "gold dwarf", "Gold Dwarf", "\tLGold Dwarf\tn", "GdDw", "\tLGdDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOLD_DWARF,
                   // desc
                   "Gold dwarves, also known as hill dwarves, are the aloof, confident and "
                   "sometimes proud subrace of dwarves that predominantly come from the Great "
                   "Rift. They are known to be particularly stalwart warriors and shrewd traders. "
                   "Gold dwarves are often trained specifically to battle the horrendous "
                   "aberrations that are known to come from the Underdark. Gold dwarves are stout, "
                   "tough individuals like their shield dwarven brethren but are less off-putting "
                   "and gruff in nature. Conversely, gold dwarves are often less agile than other "
                   "dwarves. The average gold dwarf is about four feet tall (1.2 meters) and as "
                   "heavy as a full-grown human, making them somewhat squatter than the more common "
                   "shield dwarves. Gold dwarves are also distinguishable by their light brown or "
                   "tanned skin, significantly darker than that of most dwarves, and their brown or "
                   "hazel eyes. Gold dwarves have black, gray, or brown hair, which fade to light "
                   "gray over time. Gold dwarf males and some females can grow beards, which are "
                   "carefully groomed and grown to great lengths. Humans who wander into the gold "
                   "dwarven strongholds may be surprised to find a people far more confident and "
                   "secure in their future than most dwarves. Whereas the shield dwarves suffered "
                   "serious setbacks during their history, the gold dwarves have stood firm against "
                   "the challenges thrown against them and so have few doubts about their place in "
                   "the world. As a result, gold dwarves can come off as haughty and almost  "
                   "eladrin-like in their pride,believing themselves culturally superior to "
                   "all other races and lacking the fatalistic pessimism of their shield dwarven "
                   "cousins. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Gold Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Gold Dwarf.");
  set_race_genders(RACE_GOLD_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOLD_DWARF, 0, 2, 0, 1, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOLD_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOLD_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_GOLD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_GOLD_DWARF_TOUGHNESS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);
  race_list[RACE_GOLD_DWARF].racial_language = SKILL_LANG_DWARVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LIGHTFOOT_HALFLING, "lightfoot halfling", "Lightfoot Halfling", "\tPLightfoot Halfling\tn", "LtHf", "\tPLtHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_LIGHTFOOT_HALFLING,
                   // description
                   "Lightfoot halflings were the most common type of halflings seen in the world, "
                   "in large part due to their famous wanderlust, which set them apart from the "
                   "relatively sedentary ghostwise and strongheart halflings. Lightfoots were most "
                   "comfortable living alongside other cultures, even adopting their cultural "
                   "practices, right down to their deities. A typical lightfoot halfling stood "
                   "around three feet tall and weighed around 35 to 40 pounds. Their skin colors "
                   "ranged from light pink to slightly reddish or bronze, and their hair color was "
                   "typically auburn, brown or black. Males usually wore their hair short on the "
                   "sides, often with a mullet or bowl cut. Facial hair among males was rare except "
                   "for extremely old halflings. Females rarely allowed their hair to grow beyond "
                   "shoulder length. When not adventuring or entertaining others, halflings "
                   "preferred simple, well-made clothes that were comfortable to wear yet looked "
                   "attractive. Lightfoots were, by nature, wanderers, and so could rarely be "
                   "found tied down to one place for very long. Typically, lightfoot halfling clans "
                   "moved from one area to another rapidly, staying in one place rarely more than a "
                   "year or two. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Lightfoot Halfling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Lightfoot Halfling.");
  set_race_genders(RACE_LIGHTFOOT_HALFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_LIGHTFOOT_HALFLING, 0, 0, 0, 0, 2, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_LIGHTFOOT_HALFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_LIGHTFOOT_HALFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_SHADOW_HOPPER, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_LUCKY, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_LIGHTFOOT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_NATURALLY_STEALTHY, 1, N);
  race_list[RACE_LIGHTFOOT_HALFLING].racial_language = SKILL_LANG_HALFLING;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_STOUT_HALFLING, "strongheart halfling", "Strongheart Halfling", "\tTStrongheart Halfling\tn", "StHf", "\tTStHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_STOUT_HALFLING,
                   // description
                   "Creatures of the earth who love a warm hearth and "
                   "pleasant company, s trongheart halflings are folks of few "
                   "enemies and many friends. Stronghearts a re sometimes "
                   "referred to fondly by members of other races as \"the "
                   "good folk,\" for little upsets stronghearts or corrupts "
                   "their spirit. To many of them, the greatest fear is to live "
                   "in a world of poor company and mean intent, where one "
                   "lacks freedom and the comfort of friendship. "
                   "When strongheart halflings settle into a place, they "
                   "intend to stay. It's not unusual for a dynasty of stronghearts "
                   "to live in the same place for a few centuries. "
                   "Strongheart halflings don't develop these homes in "
                   "seclusion. On the contrary, they do their best to fit into "
                   "the local community and become an essential part of "
                   "it. Their viewpoint stresses cooperation above all other "
                   "traits, and the ability to work well with others is the "
                   "most valued behavior in their lands. "
                   "Pushed from their nests, strongheart haflings typically "
                   "try to have as many comforts of home with them as "
                   "possible. Non-stronghearts with a more practical bent "
                   "can find strongheart travel habits maddening, but their "
                   "lightfoot cousins typically enjoy the novelty of it- so long "
                   "as the lightfoots don't have to carry any of the baggage. "
                   "While often stereotyped as fat and lazy due to their "
                   "homebound mindset and obsession with fine food, "
                   "strongheart halfings are typically quite industrious. "
                   "Nimble hands, their patient mindset, and their emphasis "
                   "on quality makes them excellent weavers, potters, wood "
                   "carvers, basket makers, painters, and farmers. "
                   "Strongheart halflings "
                   "are shorter on average than their lightfoot kin, and tend "
                   "to have rounder faces. They have the skin tones and hair "
                   "colors of humans, with most having brown hair. Unlike "
                   "their lightfoot cousins, strongheart halflings often have "
                   "blond or black hair and blue or green eyes. Ma les don't "
                   "grow beards or mustaches, but both males and females "
                   "can grow sideburns down to mid-cheek, and both genders "
                   "plait them into long braids. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Stout Halfling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Stout Halfling.");
  set_race_genders(RACE_STOUT_HALFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_STOUT_HALFLING, 0, 1, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_STOUT_HALFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_STOUT_HALFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_SHADOW_HOPPER, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_LUCKY, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_STOUT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_STOUT_RESILIENCE, 1, N);
  race_list[RACE_STOUT_HALFLING].racial_language = SKILL_LANG_HALFLING;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOBLIN, "goblin", "Goblin", "\tgGoblin\tn", "Gobn", "\tgGobn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOBLIN,
                   // Description
                   "The Goblins of Ansalon are little, thin humanoids, standing no more than three  "
                    "and half feet in height. Their skin tones vary greatly. They have flat faces and "
                    "dark, stringy hair. They have dull red or yellow eyes. They are very quick and  "
                    "have sharp teeth. Goblins dress in leathers made from animal hide or scavenged  "
                    "clothing, and speak the Goblin Language. "
                    "Goblins respect and cherish those who have power and are strong. They are  "
                    "ambitious, seeking power for themselves but most never attain it. When in large  "
                    "groups they tend to fall prey to mob mentality and in some cases, operate like a "
                    "wolf pack. Goblins back a strong leader and will follow their lead. A lone  "
                    "goblin may appear weak and vulnerable but typically they are confident and sure  "
                    "of themselves. "
                    "Goblins live in tribes when their larger cousins, Bugbears and Hobgoblins, are  "
                    "not dominating them. Leaders, called Rukras, lead either through trickery or  "
                    "strength. Though nasty and callous, they have the best interests of their people "
                    "in their hearts. "
                    "Goblins are simple beings but are led by their cultural imperatives to fight,  "
                    "kill and do what the larger, meaner goblins tell them to do. They are numerous  "
                    "and are spread across the interior of Ansalon, from the far north to the colds  "
                    "of Icereach. They continually struggle against the other races to survive. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Gnome.");
  set_race_genders(RACE_GOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOBLIN, -2, 2, 0, 0, 4, -2);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_GOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_NIMBLE_ESCAPE, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_FAST_MOVEMENT, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_FURY_OF_THE_SMALL, 1, N);
  race_list[RACE_GOBLIN].racial_language = SKILL_LANG_GOBLIN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HOBGOBLIN, "hobgoblin", "Hobgoblin", "\tmHobgoblin\tn", "HobG", "\tmHobG\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HOBGOBLIN,
                   // Description
                   "The Hobgoblins of Krynn are usually over six feet in height, can have a ruddy  "
                    "yellow, tan, dark red, or red-orange skin tone, brown-gray hair that covers  "
                    "their bodies, large pointed ears, and flattened, vaguely batlike faces. They  "
                    "closely resemble their smaller cousins the Goblins but are bigger and uglier,  "
                    "and are much smarter. They are faster than a Human, and have a much stronger  "
                    "endurance. Hobgoblins bore easily and will pick fights with their inferiors, but "
                    "will protect party members that are weaker than their opponents. "
                    "Most hobgoblins thrive on war, terror, and an impulse to oppose all other races. "
                    "There are some though that are understanding of civilization, and want to bring  "
                    "this to their goblin kin. These hobgoblins are called donek, or renegades in the "
                    "goblin language. The most famous donek is Lord Toede. "
                    "Hobgoblins live in semi-nomadic auls, or tribes, and are led by a murza. The  "
                    "murza will have a troop of assassins, shamans, bodyguards, and always a rival  "
                    "who wants to kill the current murza. Hobgoblins almost always defer to a  "
                    "bugbear, but will dominate any goblins in their group even going to live with  "
                    "larger goblin tribes to serve as leaders. The auls are usually dedicated to  "
                    "conquest and warfare. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Hobgoblin.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Hobgoblin.");
  set_race_genders(RACE_HOBGOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HOBGOBLIN, 0, 2, 0, 0, 1, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HOBGOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HOBGOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_HOBGOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_AUTHORITATIVE, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_FORTUNE_OF_THE_MANY, 1, N);
  race_list[RACE_HOBGOBLIN].racial_language = SKILL_LANG_GOBLIN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_FOREST_GNOME, "forest gnome", "Forest Gnome", "\tVForest Gnome\tn", "FrGn", "\tVFrGn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_FOREST_GNOME,
                   // Description
                   "Forest gnomes are among the least commonly seen gnomes on Toril, far shier than "
                   "even their deep gnome cousins. Small and reclusive, forest gnomes are so "
                   "unknown to most non-gnomes that they have repeatedly been \"discovered\" by "
                   "wandering outsiders who happen into their villages. Timid to an extreme, "
                   "forest gnomes almost never leave their hidden homes. Compared with other "
                   "gnomes, forest gnomes are even more diminutive than is typical of the stunted "
                   "race, rarely growing taller than 2½ feet in height or weighing in over 30 lbs. "
                   "Typically, males are slightly larger than females, at the most by four inches "
                   "or five pounds. Unlike other gnomes, forest gnomes generally grow their hair "
                   "long and free, feeling neither the need nor desire to shave or trim their hair "
                   "substantially, though males often do take careful care of their beards, "
                   "trimming them to a fine point or curling them into hornlike spikes. Forest "
                   "gnome skin is an earthy color and looks, in many ways, like wood, although it "
                   "is not particularly tough. Forest gnome hair is brown or black, though it grays "
                   "with age, sometimes to a pure white. Like other gnomes, forest gnomes generally "
                   "live for centuries, although their life expectancy is a bit longer than is the "
                   "case for either rock or deep gnomes; 400 is the average life expectancy of a "
                   "forest gnome. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Forest Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Forest Gnome.");
  set_race_genders(RACE_FOREST_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_FOREST_GNOME, 0, 0, 2, 0, 1, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_FOREST_GNOME, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_FOREST_GNOME,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_RESISTANCE_TO_ILLUSIONS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_ILLUSION_AFFINITY, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_TINKER_FOCUS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_FOREST_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_SPEAK_WITH_BEASTS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_NATURAL_ILLUSIONIST, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_GNOMISH_TINKERING, 1, N);
  race_list[RACE_FOREST_GNOME].racial_language = SKILL_LANG_GNOMISH;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_ROCK_GNOME, "rock gnome", "Rock Gnome", "\tDRock Gnome\tn", "RkGn", "\tDRkGn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_ROCK_GNOME,
                   // Description
                   "Rock gnomes were a curious, childlike race very much unlike their cousins, the "
                   "svirfneblin (or deep gnomes) and the forest gnomes. When most people thought of "
                   "gnomes, they were thinking of the rock gnome. Rock gnomes embodied the "
                   "characteristics of their creator and patron deity, Garl Glittergold, choosing "
                   "to spend their long lives by filling each day with as much fun and enjoyment as "
                   "possible. Rock gnomes were typically between 3 to 3½ ft (0.91‒1.1 m) tall and "
                   "weighed anywhere from 40 to 45 lb (18‒20 kg). They possessed a natural brownish "
                   "tint to their skin; the presence or absence of light had little effect upon it. "
                   "Young rock gnomes possessed any of a large number of hair colors that faded to "
                   "gray or white upon reaching adulthood. Male gnomes typically kept beards "
                   "groomed in a neat manner. Rock gnomes possessed many of the traits other races, "
                   "particularly humans, attributed to children. Most rock gnomes enjoyed life to "
                   "their very fullest; asking questions endlessly, playing pranks on friends and "
                   "strangers, and finding new and interesting hobbies were just a few of the "
                   "countless chores that rock gnomes burdened each day with. Much like a child, a "
                   "rock gnome possessed very little tolerance for long term mental focus unless "
                   "the task at hand was of notable interest.  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Rock Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Rock Gnome.");
  set_race_genders(RACE_ROCK_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_ROCK_GNOME, 0, 1, 2, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_ROCK_GNOME, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_ROCK_GNOME,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_RESISTANCE_TO_ILLUSIONS, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_ILLUSION_AFFINITY, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_TINKER_FOCUS, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_ROCK_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_ARTIFICERS_LORE, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_TINKER, 1, N);
  race_list[RACE_ROCK_GNOME].racial_language = SKILL_LANG_GNOMISH;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_ORC, "half orc", "Half Orc", "\tcHalf Orc\tn", "HOrc", "\tcHOrc\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_ORC,
                   // Description
                   "Half-orcs were humanoids born of both human and orc ancestry by a multitude of "
                   "means. Having the combined physical power of their orcish ancestors with the  "
                   "agility of their humans ones, half-orcs were formidable individuals. Though  "
                   "they were often shunned in both human and orcish society for different reasons, "
                   "half-orcs have proven themselves from time to time as worthy heroes and  "
                   "dangerous villains. Their existence implying an interesting back story that "
                   "most would not like to dwell on.  "
                   "Half-orcs were, on average, somewhere from 5'9\" – 6'4\" (1.75 – 1.93 meters) in "
                   "height and usually weigh around 155 – 225 pounds (70 – 102 kg) making them a  "
                   "little taller and stronger than humans on average. Most half-orcs had grayish  "
                   "skin, jutting jaws, prominent teeth, a sloping forehead, and coarse body hair, "
                   "which caused them to stand out from their human brethren, though their canines "
                   "were noticeably smaller than a full-blooded orc's tusks. "
                   "Like other half-breeds, half-orcs combined the natures of both their lineages "
                   "into a unique whole, a trait which extended into their mentality as well as  "
                   "their physical qualities. Like humans, half-orcs were quick to action,  "
                   "tenacious and bold, and possessed an adaptability that was unusual among most "
                   "races. This was useful to the race given that they were considered outsiders "
                   "just about everywhere; they had the ability to thrive in unwelcome or unusual "
                   "locations, which was a necessity for a half-orc's welfare. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Orcish.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Orcish.");
  set_race_genders(RACE_HALF_ORC, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_ORC, 2, 1, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_ORC, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_ORC,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_ORC, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_MENACING, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_RELENTLESS_ENDURANCE, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_SAVAGE_ATTACKS, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_HALF_ORC_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_HALF_ORC].racial_language = SKILL_LANG_ORCISH;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_SHADE, "shade", "Shade", "\tDShade\tn", "Shad", "\tDShad\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_SHADE,
                   // Description
                   "Ambitious, ruthless, and paranoid, shades are humans who trade part of their souls "
                   "for a sliver of the Shadowfell's dark essence through a ritual known as the Trail of "
                   "Five Darknesses. Even more so than the shadowborn (natives of the shadowfell descended "
                   "of common races) shades are gloom incarnate. No matter what nations or land one was "
                   "first born into, each shade undergoes a dark rebirth that transforms him or her into a "
                   "creature of stealth and secrecy who is caught between life and death. In exchange for "
                   "the twilight powers granted to shades, the Shadowfell taints their souls with dark "
                   "thoughts and a darker disposition.\r\n\r\n"
                   "For the most part, shades resemble a twisted form of their former stature and shape. "
                   "Since they were all humans prior, they all retain a humanoid shape. But their form is "
                   "changed to that of a slender shadow of their former selves. Their eyes become accustomed "
                   "to the dark and take on colors of grey, white, or even darker hues like black or purple. "
                   "Their hair becomes jet-black and their skin turns pale.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes that of a Shade.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes that of a Shade.");
  set_race_genders(RACE_SHADE, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_SHADE, 0, 0, 0, 0, 2, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_SHADE, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_SHADE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_SHADE, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_ONE_WITH_SHADOW, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_SHADOWFELL_MIND, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_PRACTICED_SNEAK, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_SHADE_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_SHADE].racial_language = SKILL_LANG_COMMON;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOLIATH, "goliath", "Goliath", "\tGGoliath\tn", "Glth", "\tGGlth\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOLIATH,
                   // Description
                   "At the highest mountain peaks - far above the slopes where trees grow and where the air "
                   "is thin and the frigid winds howl - dwell the reclusive goliaths. Few folk can claim to "
                   "have seen a goliath, and fewer still can claim friendship with them. Goliaths wander a "
                   "bleak realm of rock, wind, and cold. Their bodies look as if they are carved from mountain "
                   "stone and give them great physical power. Their spirits take after the wandering wind, making "
                   "them nomads who wander from peak to peak. Their hearts are infused with the cold regard of "
                   "their frigid realm, leaving each goliath with the responsibility to earn a place in the tribe or die trying."
                   "For goliaths, competition exists only when it is supported by a level playing field. Competition "
                   "measures talent, dedication, and effort. Those factors determine survival in their home territory, "
                   "not reliance on magic items, money, or other elements that can tip the balance one way or the other. "
                   "Goliaths happily rely on such benefits, but they are careful to remember that such an advantage can "
                   "always be lost. A goliath who relies too much on them can grow complacent, a recipe for disaster in the mountains."
                   "This trait manifests most strongly when goliaths interact with other folk. The relationship between peasants and "
                   "nobles puzzles goliaths. If a king lacks the intelligence or leadership to lead, then clearly the most talented "
                   "person in the kingdom should take his place. Goliaths rarely keep such opinions to themselves, and mock folk who "
                   "rely on society's structures or rules to maintain power.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes that of a Goliath.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes that of a Goliath.");
  set_race_genders(RACE_GOLIATH, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOLIATH, 2, 1, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOLIATH, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOLIATH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOLIATH, FEAT_NATURAL_ATHLETE, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_MOUNTAIN_BORN, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_POWERFUL_BUILD, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_STONES_ENDURANCE, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_GOLIATH_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_GOLIATH].racial_language = SKILL_LANG_GIANT;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DROW_ELF, "drow elf", "Drow Elf", "\trDrow Elf\tn", "Drow", "\trDrow\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 1, 5000, IS_NORMAL);
  set_race_details(RACE_DROW_ELF,
                   // description
                   "Drow (pronounced: /draʊ/ drow), also known as dark elves, deep elves, night  "
                   "elves, or sometimes \"The Ones Who Went Below\" on the surface, were a dark-"
                   "skinned sub-race of elves that predominantly lived in the Underdark. They "
                   "were hated and feared due to their cruelty, though some non-evil and an  "
                   "even smaller number of good drow existed. "
                   "In many ways, the drow resembled eladrin and elves. Their bodies were wiry "
                   "and athletic, while their faces were chiseled and attractive. "
                   "Drow, by reputation, were almost entirely evil. The teachings of Lolth  "
                   "represented the standard moral code for most of the race. They were overall "
                   "decadent but managed to hide it under a veneer of sophistication. "
                   "Drow were arrogant, ambitious, sadistic, treacherous, and hedonistic. From "
                   "birth, the drow were taught they were superior to other races, and that  "
                   "they should crush those beneath them. "
                   "Unlike inherently evil creatures like orcs, the evil of the drow wasn't of  "
                   "inherent nature: They enforced the Way of Lolth, leading to a race of  "
                   "emotionally stunted people, with a tenuous grasp on sanity and scarred  "
                   "mentalities, among which relatively undamaged minds were considered abnormal. "
                   "However, as mentioned above, the drow had no innate drive towards evil and "
                   "their morality was colored by the society they lived in. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Drow.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Drow.");
  set_race_genders(RACE_DROW_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DROW_ELF, 0, 0, 2, 0, 2, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_DROW_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DROW_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DROW_ELF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_DROW_SPELL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_WEAPON_PROFICIENCY_DROW, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_DROW_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_DROW_INNATE_MAGIC, 1, N);
  feat_race_assignment(RACE_DROW_ELF, FEAT_LIGHT_BLINDNESS, 1, N);
  race_list[RACE_DROW_ELF].racial_language = SKILL_LANG_UNDERCOMMON;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  add_race(RACE_DUERGAR, "duergar", "Duergar", "\t[F333]Duergar\tn", "Drgr", "\t[F333]Drgr\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 5000, IS_ADVANCE);
  set_race_details(RACE_DUERGAR,
                   /*descrip*/ "Duergar dwell in subterranean caverns far from the touch of light. They detest all races "
                               "living beneath the sun, but that hatred pales beside their loathing of their surface-dwarf "
                               "cousins. Dwarves and Duergar once were one race, but the dwarves left the deeps for their "
                               "mountain strongholds. Duergar still consider themselves the only true Dwarves, and the "
                               "rightful heirs of all beneath the world’s surface. In appearance, Duergar resemble gray-"
                               "skinned Dwarves, bearded but bald, with cold, lightless eyes. They favor taking captives "
                               "in battle over wanton slaughter, save for surface dwarves, who are slain without hesitation. "
                               "Duergar view life as ceaseless toil ended only by death. Though few can be described as "
                               "anything other than vile and cruel, Duergar still value honor and rarely break their word.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Duergar.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Duergar.");
  set_race_genders(RACE_DUERGAR, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DUERGAR, 2, 4, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_DUERGAR, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DUERGAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DUERGAR, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_LIGHT_BLINDNESS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_DUERGAR_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_DUERGAR_MAGIC, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_PARALYSIS_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_PHANTASM_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_STRONG_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_ENLARGE, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_STRENGTH, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_INVIS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_SPOT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_LISTEN, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_MOVE_SILENT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
// End Faerun races, start luminarimud races
// DL RACES
// #elif defined(CAMPGIN_DL)

#else
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HUMAN, "human", "Human", "\tBHuman\tn", "Humn", "\tBHumn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HUMAN,
                   /*descrip*/ "Humans possess exceptional drive and a great capacity to endure "
                               "and expand, and as such are currently the dominant race in the world. Their "
                               "empires and nations are vast, sprawling things, and the citizens of these "
                               "societies carve names for themselves with the strength of their sword arms "
                               "and the power of their spells. Humanity is best characterized by its "
                               "tumultuousness and diversity, and human cultures run the gamut from savage "
                               "but honorable tribes to decadent, devil-worshiping noble families in the most "
                               "cosmopolitan cities. Humans' curiosity and ambition often triumph over their "
                               "predilection for a sedentary lifestyle, and many leave their homes to explore "
                               "the innumerable forgotten corners of the world or lead mighty armies to conquer "
                               "their neighbors, simply because they can.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Human.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Human.");
  set_race_genders(RACE_HUMAN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HUMAN, 0, 0, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HUMAN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HUMAN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HUMAN, FEAT_QUICK_TO_MASTER, 1, N);
  feat_race_assignment(RACE_HUMAN, FEAT_SKILLED, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /**TEST**/ affect_assignment(RACE_HUMAN, AFF_DETECT_ALIGN, 1);
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_MOON_ELF, "moon elf", "Moon Elf", "\tYMoon Elf\tn", "MnEl", "\tYMnEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_ELF,
                   /*descrip*/ "The long-lived elves are children of the natural world, similar "
                               "in many superficial ways to fey creatures, though with key differences. "
                               "While fey are truly linked to the flora and fauna of their homes, existing "
                               "as the nearly immortal voices and guardians of the wilderness, elves are "
                               "instead mortals who are in tune with the natural world around them. Elves "
                               "seek to live in balance with the wild and understand it better than most "
                               "other mortals. Some of this understanding is mystical, but an equal part "
                               "comes from the elves' long lifespans, which in turn gives them long-ranging "
                               "outlooks. Elves can expect to remain active in the same locale for centuries. "
                               "By necessity, they must learn to maintain sustainable lifestyles, and this "
                               "is most easily done when they work with nature, rather than attempting to "
                               "bend it to their will. However, their links to nature are not entirely driven "
                               "by pragmatism. Elves' bodies slowly change over time, taking on a physical "
                               "representation of their mental and spiritual states, and those who dwell in "
                               "a region for a long period of time find themselves physically adapting to "
                               "match their surroundings, most noticeably taking on coloration that reflects "
                               "the local environment."
                               "\r\n\r\n"
                               "Moon elves are those most commonly found living among or near other races. "
                               "They have the ability to cast minor illusion and moonbeam spells innately "
                               "as well as move with great stealth in night time.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Elven.");
  set_race_genders(RACE_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_ELF, 0, 0, 0, 1, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_BATHED_IN_MOONLIGHT, 1, N);
  feat_race_assignment(RACE_MOON_ELF, FEAT_MOON_ELF_LUNAR_MAGIC, 1, N);

  add_race(RACE_HIGH_ELF, "high elf", "High Elf", "\tGHigh Elf\tn", "HiEl", "\tGHiEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HIGH_ELF,
                   // description
                   "High elves, also known as eladrin, were graceful warriors and wizards who  "
                   "originated from the realm of Faerie, also known as the Feywild. They "
                   "lived in the forests of the world. They were magical in nature and shared an "
                   "interest in the arcane arts. From an early age they also learned to defend "
                   "themselves, particularly with swords. "
                   "High elves were graceful, intelligent beings, with a greater capacity for "
                   "intelligence than most humanoid races while also possessing an agility  "
                   "comparable with their elven kin. High elves were also unusually strong- "
                   "willed and had a natural resistance to the effects of enchantment spells. "
                   "High elves also had no need for sleep in the same way most humanoids did, "
                   "instead going into a trance. While in a trance, high elves remained fully "
                   "aware of their immediate surroundings. Furthermore, high elves needed only "
                   "rest for four hours to get the same effect that most other humanoids got from "
                   "six hours of sleep. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes High Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes High Elven.");
  set_race_genders(RACE_HIGH_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HIGH_ELF, 0, 0, 1, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HIGH_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HIGH_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HIGH_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_CANTRIP, 1, N);
  feat_race_assignment(RACE_HIGH_ELF, FEAT_HIGH_ELF_LINGUIST, 1, N);
  race_list[RACE_HIGH_ELF].racial_language = SKILL_LANG_ELVEN;

  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_WOOD_ELF, "wild elf", "Wild Elf", "\tBWild Elf\tn", "WdEl", "\tBWdEl\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_WOOD_ELF,
                   // description
                   "The long-lived elves are children of the natural world, similar "
                   "in many superficial ways to fey creatures, though with key differences. "
                   "While fey are truly linked to the flora and fauna of their homes, existing "
                   "as the nearly immortal voices and guardians of the wilderness, elves are "
                   "instead mortals who are in tune with the natural world around them. Elves "
                   "seek to live in balance with the wild and understand it better than most "
                   "other mortals. Some of this understanding is mystical, but an equal part "
                   "comes from the elves' long lifespans, which in turn gives them long-ranging "
                   "outlooks. Elves can expect to remain active in the same locale for centuries. "
                   "By necessity, they must learn to maintain sustainable lifestyles, and this "
                   "is most easily done when they work with nature, rather than attempting to "
                   "bend it to their will. However, their links to nature are not entirely driven "
                   "by pragmatism. Elves' bodies slowly change over time, taking on a physical "
                   "representation of their mental and spiritual states, and those who dwell in "
                   "a region for a long period of time find themselves physically adapting to "
                   "match their surroundings, most noticeably taking on coloration that reflects "
                   "the local environment.\r\n\r\n"
                   "Wild elves differ from normal elves in that they live in amore tribal society, "
                   "with hunters and gatherers, often nomadic or occupying entire forests that they "
                   "are very protective of.  As such they are even more at home in the wild than "
                   "other elves, and have learned to move quickly and quietly.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Wild Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Wild Elven.");
  set_race_genders(RACE_WOOD_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_WOOD_ELF, 1, 0, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_WOOD_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_WOOD_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_WOOD_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_FLEETNESS, 1, N);
  feat_race_assignment(RACE_WOOD_ELF, FEAT_WOOD_ELF_MASK_OF_THE_WILD, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_ELF, "half elf", "Half Elf", "\twHalf \tYElf\tn", "HElf", "\twH\tYElf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_ELF,
                   /*descrip*/ "Elves have long drawn the covetous gazes of other races. Their "
                               "generous lifespans, magical affinity, and inherent grace each contribute "
                               "to the admiration or bitter envy of their neighbors. Of all their traits, "
                               "however, none so entrance their human associates as their beauty. Since "
                               "the two races first came into contact with each other, humans have held "
                               "up elves as models of physical perfection, seeing in these fair folk idealized "
                               "versions of themselves. For their part, many elves find humans attractive "
                               "despite their comparatively barbaric ways, and are drawn to the passion "
                               "and impetuosity with which members of the younger race play out their brief "
                               "lives. Sometimes this mutual infatuation leads to romantic relationships. "
                               "Though usually short-lived, even by human standards, such trysts may lead "
                               "to the birth of half-elves, a race descended from two cultures yet inheritor "
                               "of neither. Half-elves can breed with one another, but even these “pureblood” "
                               "half-elves tend to be viewed as bastards by humans and elves alike. Caught "
                               "between destiny and derision, half-elves often view themselves as the middle "
                               "children of the world.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Elven.");
  set_race_genders(RACE_HALF_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_ELF, 0, 0, 0, 0, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_ELF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_ELF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_ELF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_WEAPON_PROFICIENCY_ELF, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_HALF_BLOOD, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_ADAPTABILITY, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_HALF_ELF_RACIAL_ADJUSTMENT, 1, N);

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_DROW, "half drow", "Half Drow", "\tCHalf Drow\tn", "HDrw", "\tCHDrw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_DROW,
                   // description
                   "A half-drow was the offspring of one human parent and one drow parent. A half-"
                   "drow generally had dusky skin, silver or white hair, and human eye colors. They "
                   "could see around 60 feet (18 m) with darkvision, but otherwise had no other "
                   "known drow traits or abilities. "
                   "Half-drow were most commonly encountered in Dambrath and in the Underdark. "
                   "House Ousstyl, in particular, was known for having mated with humans. "
                   "Half-drow were often conceived when a male drow mated with one of his human "
                   "female slaves or when outcast drow mated with surface-dwelling species. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Drow.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Drow.");
  set_race_genders(RACE_HALF_DROW, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_DROW, 0, 0, 2, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_DROW, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_DROW,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_DROW, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_WEAPON_PROFICIENCY_DROW, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_BLOOD, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_DROW_SPELL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_HALF_DROW, FEAT_HALF_DROW_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  race_list[RACE_HALF_DROW].racial_language = SKILL_LANG_UNDERCOMMON;
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DRAGONBORN, "dragonborn", "Dragonborn", "\tWDragonborn\tn", "DrgB", "\tWDrgB\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_DRAGONBORN,
                   /*descrip*/
                   "Dragonborn (also known as Strixiki in Draconic; or Vayemniri, \"Ash-Marked  "
                   "Ones\", in Tymantheran draconic) were a race of draconic creatures native to "
                   "Abeir, Toril's long-sundered twin. During the Spellplague, dragonborn were  "
                   "transplanted from Abeir to Toril, the majority of them living in the continent "
                   "of Laerakond in the 15th century DR. In Faerun, most dragonborn dwelt in the "
                   "militaristic nation of Tymanther. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Dragonborn.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Dragonborn.");
  set_race_genders(RACE_DRAGONBORN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DRAGONBORN, 2, 0, 0, 0, 0, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_DRAGONBORN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DRAGONBORN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_BREATH, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_RESISTANCE, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_FURY, 1, N);
  feat_race_assignment(RACE_DRAGONBORN, FEAT_DRAGONBORN_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_DRAGONBORN].racial_language = SKILL_LANG_DRACONIC;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TIEFLING, "tielfing", "Tiefling", "\tATiefling\tn", "Tief", "\tATief\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_TIEFLING,
                   /*descrip*/
                   "Tieflings were human-based planetouched, native outsiders that were infused "
                   "with the touch of the fiendish planes, most often through descent from fiends-"
                   "demons, Yugoloths, devils, evil deities, and others who had bred with humans. "
                   "Tieflings were known for their cunning and personal allure, which made them  "
                   "excellent deceivers as well as inspiring leaders when prejudices were laid  "
                   "aside. Although their evil ancestors could be many generations removed, the "
                   "taint lingered. Unlike half-fiends, tieflings were not predisposed to evil "
                   "alignments and varied in alignment nearly as widely as full humans, though "
                   "tieflings were certainly devious. Tieflings tended to have an unsettling air "
                   "about them, and most people were uncomfortable around them, whether they were "
                   "aware of the tiefling's unsavory ancestry or not. While some looked like normal "
                   "humans, most retained physical characteristics derived from their ancestor, "
                   "with the most common such features being horns, prehensile tails, and pointed "
                   "teeth. Some tieflings also had eyes that were solid orbs of black, red, white, "
                   "silver, or gold, while others had eyes more similar to those of humans. Other, "
                   "more unusual characteristics included a sulfurous odor, cloven feet, or a "
                   "general aura of discomfort they left on others. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Tiefling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Tiefling.");
  set_race_genders(RACE_TIEFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_TIEFLING, 0, 0, 1, 0, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_TIEFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_TIEFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_TIEFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_HELLISH_RESISTANCE, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_BLOODHUNT, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_TIEFLING, FEAT_TIEFLING_MAGIC, 1, N);
  race_list[RACE_TIEFLING].racial_language = SKILL_LANG_ABYSSAL;
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_AASIMAR, "aasimar", "Aasimar", "\tWAasimar\tn", "Asmr", "\tWAsmr\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_AASIMAR,
                   /*descrip*/
                   "Aasimar were human-based planetouched, native outsiders that had in their blood "
                   "some good, otherworldly characteristics. They were often, but not always, descended "
                   "from celestials and other creatures of pure good alignment, but while predisposed to "
                   "good alignments, aasimar were by no means always good. Aasimar bore the mark of their "
                   "celestial touch through many different physical features that often varied from "
                   "individual to individual. Most commonly, aasimar were very similar to humans, like "
                   "tieflings and other planetouched. Nearly all aasimar were uncommonly beautiful and "
                   "still, and they were often significantly taller than humans as well.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes an Aasimar.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes an Aasimar.");
  set_race_genders(RACE_AASIMAR, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_AASIMAR, 0, 0, 0, 1, 0, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_AASIMAR, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_AASIMAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_AASIMAR, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_ASTRAL_MAJESTY, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_CELESTIAL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_HEALING_HANDS, 1, N);
  feat_race_assignment(RACE_AASIMAR, FEAT_AASIMAR_LIGHT_BEARER, 1, N);
  race_list[RACE_AASIMAR].racial_language = SKILL_LANG_CELESTIAL;

  /****************************************************************************/
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TABAXI, "tabaxi", "Tabaxi", "\tyTabaxi\tn", "Tbxi", "\tyTbxi\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_TABAXI,
                   /*descrip*/
                   "Tabaxi are taller than most humans at six to seven feet. Their bodies are slender "
                   "and covered in spotted or striped fur. Like most felines, Tabaxi had long tails and "
                   "retractable claws. Tabaxi fur color ranged from light yellow to brownish red. Tabaxi "
                   "eyes are slit-pupilled and usually green or yellow. Tabaxi are competent swimmers and "
                   "climbers as well as speedy runners. They had a good sense of balance and an acute sense "
                   "of smell. Depending on their region and fur coloration, tabaxi are known by different "
                   "names. Tabaxi with solid spots are sometimes called leopard men and tabaxi with rosette "
                   "spots are called jaguar men. The way the tabaxi pronounced their own name also varied; "
                   "the 'leopard men' pronounced it ta-BÆK-see, and the jaguar men tah-BAHSH-ee. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Tabaxi.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Tabaxi.");
  set_race_genders(RACE_TABAXI, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_TABAXI, 1, 0, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_TABAXI, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_TABAXI,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_TABAXI, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_CATS_CLAWS, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_CATS_TALENT, 1, N);
  feat_race_assignment(RACE_TABAXI, FEAT_TABAXI_FELINE_AGILITY, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_SHIELD_DWARF, "mountain dwarf", "Mountain Dwarf", "\tJMountain Dwarf\tn", "MtDw", "\tJMtDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_DWARF,
                   /*descrip*/ "Dwarves are a stoic but stern race, ensconced in cities carved "
                               "from the hearts of mountains and fiercely determined to repel the depredations "
                               "of savage races like orcs and goblins. More than any other race, dwarves "
                               "have acquired a reputation as dour and humorless artisans of the earth. "
                               "It could be said that their history shapes the dark disposition of many "
                               "dwarves, for they reside in high mountains and dangerous realms below the "
                               "earth, constantly at war with giants, goblins, and other such horrors."
                               "In addition Dwarves gain proficiency with Dwarven War Axes."
                               "\r\n\r\n"
                               "Mountain dwarves are generally from non-equitorial homelands, and are proficient "
                               "in all forms of light and medium armors, as well as being stout and hardy, able "
                               "to carry heavy loads for long distances with ease.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Dwarven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Dwarven.");
  set_race_genders(RACE_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DWARF, 2, 2, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DWARF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_SHIELD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_SHIELD_DWARF_ARMOR_TRAINING, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ARMOR_PROFICIENCY_LIGHT, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ARMOR_PROFICIENCY_MEDIUM, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(RACE_SHIELD_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOLD_DWARF, "gold dwarf", "Gold Dwarf", "\tLGold Dwarf\tn", "GdDw", "\tLGdDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOLD_DWARF,
                   // desc
                   "Gold dwarves, also known as hill dwarves, are the aloof, confident and "
                   "sometimes proud subrace of dwarves that predominantly come from the Great "
                   "Rift. They are known to be particularly stalwart warriors and shrewd traders. "
                   "Gold dwarves are often trained specifically to battle the horrendous "
                   "aberrations that are known to come from the Underdark. Gold dwarves are stout, "
                   "tough individuals like their shield dwarven brethren but are less off-putting "
                   "and gruff in nature. Conversely, gold dwarves are often less agile than other "
                   "dwarves. The average gold dwarf is about four feet tall (1.2 meters) and as "
                   "heavy as a full-grown human, making them somewhat squatter than the more common "
                   "shield dwarves. Gold dwarves are also distinguishable by their light brown or "
                   "tanned skin, significantly darker than that of most dwarves, and their brown or "
                   "hazel eyes. Gold dwarves have black, gray, or brown hair, which fade to light "
                   "gray over time. Gold dwarf males and some females can grow beards, which are "
                   "carefully groomed and grown to great lengths. Humans who wander into the gold "
                   "dwarven strongholds may be surprised to find a people far more confident and "
                   "secure in their future than most dwarves. Whereas the shield dwarves suffered "
                   "serious setbacks during their history, the gold dwarves have stood firm against "
                   "the challenges thrown against them and so have few doubts about their place in "
                   "the world. As a result, gold dwarves can come off as haughty and almost  "
                   "eladrin-like in their pride,believing themselves culturally superior to "
                   "all other races and lacking the fatalistic pessimism of their shield dwarven "
                   "cousins. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Gold Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Gold Dwarf.");
  set_race_genders(RACE_GOLD_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOLD_DWARF, 0, 2, 0, 1, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOLD_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOLD_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_GOLD_DWARF_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_GOLD_DWARF_TOUGHNESS, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_ENCUMBERED_RESILIENCE, 1, N);
  feat_race_assignment(RACE_GOLD_DWARF, FEAT_DWARVEN_WEAPON_PROFICIENCY, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LIGHTFOOT_HALFLING, "lightfoot halfling", "Lightfoot Halfling", "\tPLightfoot Halfling\tn", "LtHf", "\tPLtHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALFLING,
                   /*descrip*/ "Optimistic and cheerful by nature, blessed with uncanny luck, "
                               "and driven by a powerful wanderlust, halflings make up for their short "
                               "stature with an abundance of bravado and curiosity. At once excitable and "
                               "easy-going, halflings like to keep an even temper and a steady eye on opportunity, "
                               "and are not as prone to violent or emotional outbursts as some of the more "
                               "volatile races. Even in the jaws of catastrophe, halflings almost never "
                               "lose their sense of humor. Their ability to find humor in the absurd, no "
                               "matter how dire the situation, often allows halflings to distance themselves "
                               "ever so slightly from the dangers that surround them. This sense of detachment "
                               "can also help shield them from terrors that might immobilize their allies."
                               "\r\n\r\n"
                               "Lightfoot halflings are more lithe than their stout cousins, and have a natural "
                               "talent with stealth, even in the midst of battle.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Halfling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Halfling.");
  set_race_genders(RACE_HALFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALFLING, 0, 0, 0, 0, 2, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_SHADOW_HOPPER, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_LUCKY, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_LIGHTFOOT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LIGHTFOOT_HALFLING, FEAT_NATURALLY_STEALTHY, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_ROCK_GNOME, "rock gnome", "Rock Gnome", "\tDRock Gnome\tn", "RkGn", "\tDRkGn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GNOME,
                   /*descrip*/ "Gnomes are distant relatives of the fey, and their history tells "
                               "of a time when they lived in the fey's mysterious realm, a place where colors "
                               "are brighter, the wildlands wilder, and emotions more primal. Unknown forces "
                               "drove the ancient gnomes from that realm long ago, forcing them to seek "
                               "refuge in this world; despite this, the gnomes have never completely abandoned "
                               "their fey roots or adapted to mortal culture. Though gnomes are no longer "
                               "truly fey, their fey heritage can be seen in their innate magic powers, "
                               "their oft-capricious natures, and their outlooks on life and the world."
                               "\r\n\r\n"
                               "Rock gnomes are those who live in and delve deep into the mountains, searching "
                               "for gems and gold. They often use their talent in concert with mountain "
                               "dwarves, becoming incredible inventors and tinkers.  This gives them the "
                               "ability to improve armor and weapons temporarily.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Gnomish.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Gnomish.");
  set_race_genders(RACE_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GNOME, 0, 1, 2, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_GNOME, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GNOME,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GNOME, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_RESISTANCE_TO_ILLUSIONS, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_ILLUSION_AFFINITY, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_TINKER_FOCUS, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GNOME, FEAT_GNOMISH_TINKERING, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_ROCK_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_ARTIFICERS_LORE, 1, N);
  feat_race_assignment(RACE_ROCK_GNOME, FEAT_TINKER, 1, N);

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_STOUT_HALFLING, "stout halfling", "Stout Halfling", "\tTStout Halfling\tn", "StHf", "\tTStHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_STOUT_HALFLING,
                   // description
                   "Creatures of the earth who love a warm hearth and "
                   "pleasant company, s trongheart halflings are folks of few "
                   "enemies and many friends. Stouts are sometimes "
                   "referred to fondly by members of other races as \"the "
                   "good folk,\" for little upsets stouts or corrupts "
                   "their spirit. To many of them, the greatest fear is to live "
                   "in a world of poor company and mean intent, where one "
                   "lacks freedom and the comfort of friendship. "
                   "When stout halflings settle into a place, they "
                   "intend to stay. It's not unusual for a dynasty of stouts "
                   "to live in the same place for a few centuries. "
                   "Strongheart halflings don't develop these homes in "
                   "seclusion. On the contrary, they do their best to fit into "
                   "the local community and become an essential part of "
                   "it. Their viewpoint stresses cooperation above all other "
                   "traits, and the ability to work well with others is the "
                   "most valued behavior in their lands. "
                   "Pushed from their nests, stout haflings typically "
                   "try to have as many comforts of home with them as "
                   "possible. Non-stouts with a more practical bent "
                   "can find stout travel habits maddening, but their "
                   "lightfoot cousins typically enjoy the novelty of it- so long "
                   "as the lightfoots don't have to carry any of the baggage. "
                   "While often stereotyped as fat and lazy due to their "
                   "homebound mindset and obsession with fine food, "
                   "stout halfings are typically quite industrious. "
                   "Nimble hands, their patient mindset, and their emphasis "
                   "on quality makes them excellent weavers, potters, wood "
                   "carvers, basket makers, painters, and farmers. "
                   "Strongheart halflings "
                   "are shorter on average than their lightfoot kin, and tend "
                   "to have rounder faces. They have the skin tones and hair "
                   "colors of humans, with most having brown hair. Unlike "
                   "their lightfoot cousins, stout halflings often have "
                   "blond or black hair and blue or green eyes. Ma les don't "
                   "grow beards or mustaches, but both males and females "
                   "can grow sideburns down to mid-cheek, and both genders "
                   "plait them into long braids. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Stout Halfling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Stout Halfling.");
  set_race_genders(RACE_STOUT_HALFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_STOUT_HALFLING, 0, 1, 0, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_STOUT_HALFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_STOUT_HALFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_SHADOW_HOPPER, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_LUCKY, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_STOUT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_STOUT_HALFLING, FEAT_STOUT_RESILIENCE, 1, N);
  race_list[RACE_STOUT_HALFLING].racial_language = SKILL_LANG_HALFLING;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_FOREST_GNOME, "forest gnome", "Forest Gnome", "\tVForest Gnome\tn", "FrGn", "\tVFrGn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_FOREST_GNOME,
                   // Description
                   "Forest gnomes are among the least commonly seen gnomes on Toril, far shier than "
                   "even their deep gnome cousins. Small and reclusive, forest gnomes are so "
                   "unknown to most non-gnomes that they have repeatedly been \"discovered\" by "
                   "wandering outsiders who happen into their villages. Timid to an extreme, "
                   "forest gnomes almost never leave their hidden homes. Compared with other "
                   "gnomes, forest gnomes are even more diminutive than is typical of the stunted "
                   "race, rarely growing taller than 2½ feet in height or weighing in over 30 lbs. "
                   "Typically, males are slightly larger than females, at the most by four inches "
                   "or five pounds. Unlike other gnomes, forest gnomes generally grow their hair "
                   "long and free, feeling neither the need nor desire to shave or trim their hair "
                   "substantially, though males often do take careful care of their beards, "
                   "trimming them to a fine point or curling them into hornlike spikes. Forest "
                   "gnome skin is an earthy color and looks, in many ways, like wood, although it "
                   "is not particularly tough. Forest gnome hair is brown or black, though it grays "
                   "with age, sometimes to a pure white. Like other gnomes, forest gnomes generally "
                   "live for centuries, although their life expectancy is a bit longer than is the "
                   "case for either rock or deep gnomes; 400 is the average life expectancy of a "
                   "forest gnome. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Forest Gnome.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Forest Gnome.");
  set_race_genders(RACE_FOREST_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_FOREST_GNOME, 0, 0, 2, 0, 1, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_FOREST_GNOME, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_FOREST_GNOME,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_RESISTANCE_TO_ILLUSIONS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_ILLUSION_AFFINITY, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_TINKER_FOCUS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_FOREST_GNOME_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_SPEAK_WITH_BEASTS, 1, N);
  feat_race_assignment(RACE_FOREST_GNOME, FEAT_NATURAL_ILLUSIONIST, 1, N);
  race_list[RACE_FOREST_GNOME].racial_language = SKILL_LANG_GNOMISH;

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_ORC, "halforc", "HalfOrc", "\twHalf \tROrc\tn", "HOrc", "\twH\tROrc\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HALF_ORC,
                   /*descrip*/ "As seen by civilized races, half-orcs are monstrosities, the result "
                               "of perversion and violence—whether or not this is actually true. Half-orcs "
                               "are rarely the result of loving unions, and as such are usually forced to "
                               "grow up hard and fast, constantly fighting for protection or to make names "
                               "for themselves. Half-orcs as a whole resent this treatment, and rather than "
                               "play the part of the victim, they tend to lash out, unknowingly confirming "
                               "the biases of those around them. A few feared, distrusted, and spat-upon "
                               "half-orcs manage to surprise their detractors with great deeds and unexpected "
                               "wisdom—though sometimes it's easier just to crack a few skulls. Some half-orcs "
                               "spend their entire lives proving to full-blooded orcs that they are just as "
                               "fierce. Others opt for trying to blend into human society, constantly demonstrating "
                               "that they aren't monsters. Their need to always prove themselves worthy "
                               "encourages half-orcs to strive for power and greatness within the society "
                               "around them.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Orcish.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Orcish.");
  set_race_genders(RACE_HALF_ORC, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_ORC, 2, 1, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_ORC, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_ORC,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_ORC, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_HALF_ORC_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_MENACING, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_RELENTLESS_ENDURANCE, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_SAVAGE_ATTACKS, 1, N);
  feat_race_assignment(RACE_HALF_ORC, FEAT_HALF_ORC_RACIAL_ADJUSTMENT, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /**********/
  /*Advanced*/
  /**********/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_TROLL, "halftroll", "HalfTroll", "\trHalf Troll\tn", "HTrl", "\trHTrl\tn",
           /* race-family,     size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_LARGE, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(RACE_HALF_TROLL,
                   /*descrip*/ "Half-Trolls are large, green, lanky, powerful, agile and hardy.  They tend "
                               "to have warty thick skin, black eyes and mottled black or brown hair "
                               "on their head.  Half-Trolls are extremely destructive in nature, often "
                               "searching or planning to do acts of destruction against weaker races. "
                               "Half-Trolls tend to inhabit swamps and lakes, and tend to band together in "
                               "war clans.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Half-Troll.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Half-Troll.");
  set_race_genders(RACE_HALF_TROLL, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALF_TROLL, 2, 4, -2, -2, 2, -2);        /* str con int wis dex cha */
  set_race_alignments(RACE_HALF_TROLL, N, N, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALF_TROLL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALF_TROLL, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_TROLL_REGENERATION, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_WEAKNESS_TO_FIRE, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_WEAKNESS_TO_ACID, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_STRONG_AGAINST_POISON, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_STRONG_AGAINST_DISEASE, 1, N);
  feat_race_assignment(RACE_HALF_TROLL, FEAT_HALF_TROLL_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_ARCANA_GOLEM, "arcanagolem", "ArcanaGolem", "\tRArcana \tcGolem\tn", "ArGo", "\tRAr\tcGo\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(RACE_ARCANA_GOLEM,
                   /*descrip*/ "Arcana Golems are mortal spellcasters (typically, but not always human) "
                               "whose devotion to the magical arts has allowed them to fuse with a "
                               "golem -- but rather than being the mindless, magically incompetent beings "
                               "golems are, the process inverts the golem's intelligence and magical "
                               "aptitude. This rebirth leaves them indistinguishable on the outside from "
                               "humans except for a vastly improved magical aptitude. "
                               "\r\n"
                               "On the inside, Arcana Golems resemble the outside of Crystal Dwarves; "
                               "their organs have been replaced with metallic and/or crystal elements "
                               "that function as well as humanoid organs save for being a little frailer. "
                               "While Arcana Golems do not need to consume outside materials -- the ambient "
                               "magical energy in the air is more than enough fuel -- many like to "
                               "do so anyway. Arcana Golems do need to sleep a lot even for a mortal race; "
                               "they need an average of 12 hours of quality sleep per 24 hours to function "
                               "at full cylinders and like to sleep for several days at a time. Arcana "
                               "Golems reproduce as humans regardless of their original race. This can "
                               "cause animosity from the community of the rare dwarven or elven Arcana Golem."
                               "\r\n"
                               "The signature abilities of the Arcana Golem, aside from a natural talent "
                               "for spellcasting and crafting, is their Spell Battle. Spell Battle "
                               "allows the Arcana Golem to convert magical energy and accuracy into combat "
                               "prowess.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Arcana Golem.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Arcana Golem.");
  set_race_genders(RACE_ARCANA_GOLEM, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_ARCANA_GOLEM, 0, 0, 3, 3, 0, 3);           /* str con int wis dex cha */
  set_race_alignments(RACE_ARCANA_GOLEM, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_ARCANA_GOLEM,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_SPELLBATTLE, 1, N);
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_SPELL_VULNERABILITY, 1, N);
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_ENCHANTMENT_VULNERABILITY, 1, N);
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_PHYSICAL_VULNERABILITY, 1, N);
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_MAGICAL_HERITAGE, 1, N);
  feat_race_assignment(RACE_ARCANA_GOLEM, FEAT_ARCANA_GOLEM_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DROW, "drow", "Drow", "\tmDrow\tn", "Drow", "\tmDrow\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(RACE_DROW,
                   /*descrip*/ "Cruel and cunning, Drow, also known as dark elves, were cursed into their present appearance "
                               "by the Arcanite and Prisoner's magic, were led down the path to evil and corruption. "
                               "The drow have black skin that resembles polished obsidian and stark white or "
                               "pale yellow hair. They commonly have red eyes. Drow are the same size as elves but a "
                               "bit thinner."
                               "\r\n"
                               "Descending into the Underworld, they formed cities shaped from the rock of cyclopean caverns. "
                               "They developed a theocratic and matriarchal society based on power and deceit. "
                               "Females generally hold all positions of power and responsibility in the government, the military, and the home. "
                               "In such a society males are often trained as warriors to become soldiers, guards, and "
                               "servants of females. Those males showing aptitude with magic are trained as Wizards "
                               "instead. While they are not born evil, malignancy is deep-rooted in their culture and "
                               "society, and nonconformists rarely survive for long.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Drow.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Drow.");
  set_race_genders(RACE_DROW, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DROW, 0, 0, 4, 2, 2, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_DROW, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DROW,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DROW, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_WEAPON_PROFICIENCY_DROW, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_DROW_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_DROW_SPELL_RESISTANCE, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_SLA_FAERIE_FIRE, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_SLA_LEVITATE, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_SLA_DARKNESS, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_LIGHT_BLINDNESS, 1, N);
  feat_race_assignment(RACE_DROW, FEAT_DROW_INNATE_MAGIC, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DUERGAR, "duergar", "Duergar", "\t[F333]Duergar\tn", "Drgr", "\t[F333]Drgr\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(RACE_DUERGAR,
                   /*descrip*/ "Duergar dwell in subterranean caverns far from the touch of light. They detest all races "
                               "living beneath the sun, but that hatred pales beside their loathing of their surface-dwarf "
                               "cousins. Dwarves and Duergar once were one race, but the dwarves left the deeps for their "
                               "mountain strongholds. Duergar still consider themselves the only true Dwarves, and the "
                               "rightful heirs of all beneath the world’s surface. In appearance, Duergar resemble gray-"
                               "skinned Dwarves, bearded but bald, with cold, lightless eyes. They favor taking captives "
                               "in battle over wanton slaughter, save for surface dwarves, who are slain without hesitation. "
                               "Duergar view life as ceaseless toil ended only by death. Though few can be described as "
                               "anything other than vile and cruel, Duergar still value honor and rarely break their word.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Duergar.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Duergar.");
  set_race_genders(RACE_DUERGAR, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DUERGAR, 2, 4, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_DUERGAR, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_DUERGAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_DUERGAR, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_LIGHT_BLINDNESS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_DUERGAR_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_DUERGAR_MAGIC, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_PARALYSIS_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_PHANTASM_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_STRONG_SPELL_HARDINESS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_ENLARGE, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_STRENGTH, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_SLA_INVIS, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_SPOT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_LISTEN, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_AFFINITY_MOVE_SILENT, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_STABILITY, 1, N);
  feat_race_assignment(RACE_DUERGAR, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /******/
  /*Epic*/
  /******/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_CRYSTAL_DWARF, "crystaldwarf", "CrystalDwarf", "\tCCrystal \tgDwarf\tn", "CDwf", "\tCC\tgDwf\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 10, 30000, IS_EPIC_R);
  set_race_details(RACE_CRYSTAL_DWARF,
                   /*descrip*/ "Crystal Dwarves are dwarves whose committment to earth have way "
                               "exceeded even the norms of dwarves.  A cross of divine power and "
                               "magic enhance these dwarves connection to the earth to levels that "
                               "are near earth elementals.  Their bodies take crystal-like texture "
                               "and their skin takes on the sharp angles of crystals as well.  Often "
                               "their pupils become diamond shaped and share the reflective property "
                               "of diamonds as well.  In addition Crystal Dwarves have the ability "
                               "to transform parts of their body to fully crystal-like weight and "
                               "texture - which can be extremely effective in offensive and defensive "
                               "maneuvers in combat.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Crystal-Dwarf.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Crystal-Dwarf.");
  set_race_genders(RACE_CRYSTAL_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_CRYSTAL_DWARF, 2, 4, 0, 4, 2, 2);           /* str con int wis dex cha */
  set_race_alignments(RACE_CRYSTAL_DWARF, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_CRYSTAL_DWARF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_CRYSTAL_BODY, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_CRYSTAL_FIST, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_HARDY, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_CRYSTAL_SKIN, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_POISON_RESIST, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_CRYSTAL_DWARF_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TRELUX, "trelux", "Trelux", "\tGTre\tYlux\tn", "Trlx", "\tGTr\tYlx\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 10, 30000, IS_EPIC_R);
  set_race_details(RACE_TRELUX,
                   /*descrip*/ "Trelux have a small head with a fused thorax and abdomen. Trelux also have eight "
                               "powerful yet rough and jagged legs for leaping and stamina. Their bodies are beetle- "
                               "like with a hard shell protecting them. This exoskeleton covers their entire body  "
                               "providing ample protection. Their bodies shell contains wings underneath it which  "
                               "can be spread whenever flight is needed. Trelux also have two powerful claws in  "
                               "front of them to cut victims. Both pincers are equipped with a poisonous spur. "
                               "\r\n"
                               "Facts:  \r\n"
                               "*Trelux do NOT like having their antenna touched.  \r\n"
                               "*All Trelux usually have a black thorax/abdomen but a multitude of colors when it "
                               "comes to other parts of their body. \r\n"
                               "*Trelux also can have stripes, spots, or blotches on their body. \r\n"
                               "*Female Trelux usually have brighter colors than males. \r\n"
                               "*Trelux almost always have black eyes but rarely have yellow eyes.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Trelux.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Trelux.");
  set_race_genders(RACE_TRELUX, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_TRELUX, 4, 4, 0, 0, 4, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_TRELUX, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_TRELUX,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, Y,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, Y, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_TRELUX, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_HARDY, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_VULNERABLE_TO_COLD, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_TRELUX_EXOSKELETON, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_LEAP, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_WINGS, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_TRELUX_EQ, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_TRELUX_PINCERS, 1, N);
  feat_race_assignment(RACE_TRELUX, FEAT_INSECTBEING, 1, N);

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_SHADE, "shade", "Shade", "\tDShade\tn", "Shad", "\tDShad\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_SHADE,
                   // Description
                   "Ambitious, ruthless, and paranoid, shades are humans who trade part of their souls "
                   "for a sliver of the Shadowfell's dark essence through a ritual known as the Trail of "
                   "Five Darknesses. Even more so than the shadowborn (natives of the shadowfell descended "
                   "of common races) shades are gloom incarnate. No matter what nations or land one was "
                   "first born into, each shade undergoes a dark rebirth that transforms him or her into a "
                   "creature of stealth and secrecy who is caught between life and death. In exchange for "
                   "the twilight powers granted to shades, the Shadowfell taints their souls with dark "
                   "thoughts and a darker disposition.\r\n\r\n"
                   "For the most part, shades resemble a twisted form of their former stature and shape. "
                   "Since they were all humans prior, they all retain a humanoid shape. But their form is "
                   "changed to that of a slender shadow of their former selves. Their eyes become accustomed "
                   "to the dark and take on colors of grey, white, or even darker hues like black or purple. "
                   "Their hair becomes jet-black and their skin turns pale.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes that of a Shade.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes that of a Shade.");
  set_race_genders(RACE_SHADE, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_SHADE, 0, 0, 0, 0, 2, 1);           /* str con int wis dex cha */
  set_race_alignments(RACE_SHADE, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_SHADE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_SHADE, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_ONE_WITH_SHADOW, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_SHADOWFELL_MIND, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_PRACTICED_SNEAK, 1, N);
  feat_race_assignment(RACE_SHADE, FEAT_SHADE_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_SHADE].racial_language = SKILL_LANG_COMMON;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOLIATH, "goliath", "Goliath", "\tGGoliath\tn", "Glth", "\tGGlth\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOLIATH,
                   // Description
                   "At the highest mountain peaks - far above the slopes where trees grow and where the air "
                   "is thin and the frigid winds howl - dwell the reclusive goliaths. Few folk can claim to "
                   "have seen a goliath, and fewer still can claim friendship with them. Goliaths wander a "
                   "bleak realm of rock, wind, and cold. Their bodies look as if they are carved from mountain "
                   "stone and give them great physical power. Their spirits take after the wandering wind, making "
                   "them nomads who wander from peak to peak. Their hearts are infused with the cold regard of "
                   "their frigid realm, leaving each goliath with the responsibility to earn a place in the tribe or die trying."
                   "For goliaths, competition exists only when it is supported by a level playing field. Competition "
                   "measures talent, dedication, and effort. Those factors determine survival in their home territory, "
                   "not reliance on magic items, money, or other elements that can tip the balance one way or the other. "
                   "Goliaths happily rely on such benefits, but they are careful to remember that such an advantage can "
                   "always be lost. A goliath who relies too much on them can grow complacent, a recipe for disaster in the mountains."
                   "This trait manifests most strongly when goliaths interact with other folk. The relationship between peasants and "
                   "nobles puzzles goliaths. If a king lacks the intelligence or leadership to lead, then clearly the most talented "
                   "person in the kingdom should take his place. Goliaths rarely keep such opinions to themselves, and mock folk who "
                   "rely on society's structures or rules to maintain power.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes that of a Goliath.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes that of a Goliath.");
  set_race_genders(RACE_GOLIATH, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOLIATH, 2, 1, 0, 0, 0, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOLIATH, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOLIATH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOLIATH, FEAT_NATURAL_ATHLETE, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_MOUNTAIN_BORN, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_POWERFUL_BUILD, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_STONES_ENDURANCE, 1, N);
  feat_race_assignment(RACE_GOLIATH, FEAT_GOLIATH_RACIAL_ADJUSTMENT, 1, N);
  race_list[RACE_GOLIATH].racial_language = SKILL_LANG_GIANT;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GOBLIN, "goblin", "Goblin", "\tgGoblin\tn", "Gobn", "\tgGobn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_GOBLIN,
                   // Description
                   "The Goblins of Ansalon are little, thin humanoids, standing no more than three  "
                    "and half feet in height. Their skin tones vary greatly. They have flat faces and "
                    "dark, stringy hair. They have dull red or yellow eyes. They are very quick and  "
                    "have sharp teeth. Goblins dress in leathers made from animal hide or scavenged  "
                    "clothing, and speak the Goblin Language. "
                    "Goblins respect and cherish those who have power and are strong. They are  "
                    "ambitious, seeking power for themselves but most never attain it. When in large  "
                    "groups they tend to fall prey to mob mentality and in some cases, operate like a "
                    "wolf pack. Goblins back a strong leader and will follow their lead. A lone  "
                    "goblin may appear weak and vulnerable but typically they are confident and sure  "
                    "of themselves. "
                    "Goblins live in tribes when their larger cousins, Bugbears and Hobgoblins, are  "
                    "not dominating them. Leaders, called Rukras, lead either through trickery or  "
                    "strength. Though nasty and callous, they have the best interests of their people "
                    "in their hearts. "
                    "Goblins are simple beings but are led by their cultural imperatives to fight,  "
                    "kill and do what the larger, meaner goblins tell them to do. They are numerous  "
                    "and are spread across the interior of Ansalon, from the far north to the colds  "
                    "of Icereach. They continually struggle against the other races to survive. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Goblin.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Goblin.");
  set_race_genders(RACE_GOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GOBLIN, -2, 2, 0, 0, 4, -2);           /* str con int wis dex cha */
  set_race_alignments(RACE_GOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_GOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_GOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_GOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_NIMBLE_ESCAPE, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_FAST_MOVEMENT, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_GOBLIN, FEAT_FURY_OF_THE_SMALL, 1, N);
  race_list[RACE_GOBLIN].racial_language = SKILL_LANG_GOBLIN;

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HOBGOBLIN, "hobgoblin", "Hobgoblin", "\tmHobgoblin\tn", "HobG", "\tmHobG\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(RACE_HOBGOBLIN,
                   // Description
                   "The Hobgoblins of Krynn are usually over six feet in height, can have a ruddy  "
                    "yellow, tan, dark red, or red-orange skin tone, brown-gray hair that covers  "
                    "their bodies, large pointed ears, and flattened, vaguely batlike faces. They  "
                    "closely resemble their smaller cousins the Goblins but are bigger and uglier,  "
                    "and are much smarter. They are faster than a Human, and have a much stronger  "
                    "endurance. Hobgoblins bore easily and will pick fights with their inferiors, but "
                    "will protect party members that are weaker than their opponents. "
                    "Most hobgoblins thrive on war, terror, and an impulse to oppose all other races. "
                    "There are some though that are understanding of civilization, and want to bring  "
                    "this to their goblin kin. These hobgoblins are called donek, or renegades in the "
                    "goblin language. The most famous donek is Lord Toede. "
                    "Hobgoblins live in semi-nomadic auls, or tribes, and are led by a murza. The  "
                    "murza will have a troop of assassins, shamans, bodyguards, and always a rival  "
                    "who wants to kill the current murza. Hobgoblins almost always defer to a  "
                    "bugbear, but will dominate any goblins in their group even going to live with  "
                    "larger goblin tribes to serve as leaders. The auls are usually dedicated to  "
                    "conquest and warfare. ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Hobgoblin.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Hobgoblin.");
  set_race_genders(RACE_HOBGOBLIN, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HOBGOBLIN, 0, 2, 0, 0, 1, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_HOBGOBLIN, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HOBGOBLIN,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_HOBGOBLIN_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_AUTHORITATIVE, 1, N);
  feat_race_assignment(RACE_HOBGOBLIN, FEAT_FORTUNE_OF_THE_MANY, 1, N);
  race_list[RACE_HOBGOBLIN].racial_language = SKILL_LANG_GOBLIN;

  // end luminari race info
#endif
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LICH, "lich", "Lich", "\tLLich\tn", "Lich", "\tLLich\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
#ifdef CAMPAIGN_FR
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 60000, IS_EPIC_R);
#else
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 999999999, IS_EPIC_R);
#endif
  set_race_details(RACE_LICH,
                   /*descrip*/ "Few creatures are more feared than the lich. The pinnacle of necromantic art, who "
                               "has chosen to shed his life as a method to cheat death by becoming undead. While many who reach "
                               "such heights of power stop at nothing to achieve immortality, the idea of becoming a lich is "
                               "abhorrent to most creatures. The process involves the extraction of ones life-force and its "
                               "imprisonment in a specially prepared phylactery.  One gives up life, but in trapping "
                               "life he also traps his death, and as long as his phylactery remains intact he can continue on in "
                               "his research and work without fear of the passage of time."
                               "\r\n\r\n"
                               "The quest to become a lich is a lengthy one. While construction of the magical phylactery to "
                               "contain ones soul is a critical component, a prospective lich must also learn the "
                               "secrets of transferring his soul into the receptacle and of preparing his body for the "
                               "transformation into undeath, neither of which are simple tasks. Further complicating the ritual "
                               "is the fact that no two bodies or souls are exactly alike, a ritual that works for one spellcaster "
                               "might simply kill another or drive him insane. "
                               "\r\n\r\n"
                               "Please note that a Lich will be the same size class they were before the transformation.\r\n  "
                               "Please note that becoming a lich requires level 30 and will reset your exp to 0.\r\n  "
                               "Please note that a Lich has all the advantages/disadvantages of being Undead.\r\n  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Lich.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Lich.");
  set_race_genders(RACE_LICH, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_LICH, 0, 2, 6, 2, 2, 6);           /* str con int wis dex cha */
  set_race_alignments(RACE_LICH, N, N, N, N, N, N, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_LICH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, Y, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_LICH, FEAT_UNARMED_STRIKE, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_IMPROVED_UNARMED_STRIKE, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_LICH, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_HARDY, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_SPELL_RESIST, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_DAM_RESIST, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_TOUCH, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_REJUV, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_LICH_FEAR, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_ELECTRIC_IMMUNITY, 1, N);
  feat_race_assignment(RACE_LICH, FEAT_COLD_IMMUNITY, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_VAMPIRE, "vampire", "Vampire", "\tLVampire\tn", "Vamp", "\tLVamp\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
#ifdef CAMPAIGN_FR
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 60000, IS_EPIC_R);
#else
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 999999999, IS_EPIC_R);
#endif
  set_race_details(RACE_VAMPIRE,
                   /*descrip*/ "Vampires are one of the most fearsome of the Undead creatures in Lumia. With unnatural strength, "
                               " agility and cunning, they can easily overpower most other creatures with their physical "
                               "prowess alone. But the vampire is much more deadly than just his claws and wits. Vampires have "
                               "a number of supernatural abilities that inspire dread in his foes.  Gaining sustenance from "
                               "the blood of the living, vampires can heal quickly from almost any wound. For the victims "
                               "of their feeding, they may raise again as vampiric spawn... an undead creature under the vampire's "
                               "control with many vampiric abilities of their own. They may also call animal minions to aid them "
                               "in battle, from wolves, to swarms of rats and vampire bats as well. They can dominate intelligent "
                               "foes with a simple gaze, and they may drain the energy of living beings with an unarmed attack. "
                               "They can also assume the form of a wolf or a giant bat, as well as assume a gasoeus form at will, "
                               "and have the ability to scale sheer surfaces as easily as a spider may."
                               "\r\n\r\n"
                               "But a vampire is not without its weaknesses. Exposed to sunlight, they will quickly be reduced "
                               "to ash, and moving water is worse, able to kill a vampire submerged in running water in less than a minute."
                               "\r\n\r\n"
                               "Being a vampire is a state most would consider a curse, however there are legends of those who "
                               "sought out the 'gift' of vampirism, with some few who actually obtained it. To this day however, "
                               "such secrets have been lost to the ages. However these are the days of great heroes and villains, "
                               "and such days often bring to light secrets of the past. Perhaps one day soon the legends may become "
                               "reality."
                               "\r\n\r\n"
                               "Please note that a Vampire will be the same size class they were before the transformation.\r\n  "
                               "Please note that becoming a Vampire requires level 30 and will reset your exp to 0.\r\n  "
                               "Please note that a Vampire has all the advantages/disadvantages of being Undead.\r\n  ",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Vampire.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Vampire.");
  set_race_genders(RACE_VAMPIRE, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_VAMPIRE, 6, 4, 2, 2, 4, 4);           /* str con int wis dex cha */
  set_race_alignments(RACE_VAMPIRE, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_VAMPIRE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, Y, N, N, N, Y, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, Y, N, Y, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_VAMPIRE, FEAT_ALERTNESS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_COMBAT_REFLEXES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_DODGE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_IMPROVED_INITIATIVE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_LIGHTNING_REFLEXES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_TOUGHNESS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_NATURAL_ARMOR, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_DAMAGE_REDUCTION, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ENERGY_RESISTANCE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_FAST_HEALING, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_WEAKNESSES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_BLOOD_DRAIN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CREATE_SPAWN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_DOMINATE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ENERGY_DRAIN, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_CHANGE_SHAPE, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_GASEOUS_FORM, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_SPIDER_CLIMB, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_SKILL_BONUSES, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_ABILITY_SCORE_BOOSTS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VAMPIRE_BONUS_FEATS, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_VAMPIRE, FEAT_HARDY, 1, N);

  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/


  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_FAE, "fae", "Fae", "\tMFae \tn", "Fae ", "\tMFae \tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
#ifdef CAMPAIGN_FR
           RACE_TYPE_HUMANOID, SIZE_TINY, TRUE, 10, 35000, IS_EPIC_R);
#else
           RACE_TYPE_HUMANOID, SIZE_TINY, TRUE, 10, 50000, IS_EPIC_R);
#endif
  set_race_details(RACE_FAE,
                   // Description
                   "Fae are relatively reclusive. They would rather spend their time frolicking in woodland glades than "
                   "cavorting with other races. They are the consummate trickster, often devising elaborate ruses to lead "
                   "strangers away from their glades. They do love visitors though, even if it is just to have a target for "
                   "their tricks. They also have a love of stories and magic... Bards are therefore almost always welcome in "
                   "a glade. Monks are greatly cherished as visitors as well, due to their ingrained resilience to faerie glamor. "
                   "Competitions are held to see who can trick the monk, with the winner crowned prince of glamor for the day. It "
                   "should be noted though, then monks are generally not harmed in order to encourage their return.\r\n\r\n"
                   "They also posses an utterly alien sense of morals. They would completely erase a mortals memory, or put one to "
                   "sleep for a year without thought to the consequences. Their chaotic behavior often stems from this lack of "
                   "concern for consequences as Fae tend to understand the term in a different light than mortals. Likewise, harm "
                   "to a mortal is often disregarded in the same manner. The saying \"It's all fun and games until someone losses an "
                   "arm... then its just hilarious \" is very applicable here.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes that of a Fae.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes that of a Fae.");
  set_race_genders(RACE_FAE, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_FAE, -4, 0, 0, 0, 10, 6);         /* str con int wis dex cha */
  set_race_alignments(RACE_FAE, N, Y, Y, N, Y, Y, N, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_FAE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_FAE, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_DODGE, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_FAE_RACIAL_ADJUSTMENT, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_FAE_MAGIC, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_FAE_RESISTANCE, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_FAE_SENSES, 1, N);
  feat_race_assignment(RACE_FAE, FEAT_FAE_FLIGHT, 1, N);
  race_list[RACE_FAE].racial_language = SKILL_LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

#endif

  /**********/
  /* Animal */
  /**********/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_EAGLE, "eagle", "Eagle", "Eagle", "Eagl", "Eagl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_EAGLE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, Y, Y, N, N, N, N);

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_BAT, "bat", "Bat", "Bat", "Bat", "Bat",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_DIMINUTIVE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_BAT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DINOSAUR, "dinosaur", "Dinosaur", "Dinosaur", "Dino", "Dino",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DINOSAUR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, Y, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_ELEPHANT, "elephant", "Elephant", "Elephant", "Elep", "Elep",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_ELEPHANT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, Y, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DIRE_ELEPHANT, "dire elephant", "Dire Elephant", "Dire Elephant", "DElp", "DElp",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DIRE_ELEPHANT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, Y, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_ROC, "roc", "Roc", "Roc", "Roc ", "Roc ",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_ROC,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, Y, Y, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DIRE_ROC, "dire roc", "Dire Roc", "Dire Roc", "DRoc", "DRoc",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DIRE_ROC,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, Y, Y, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_PURPLE_WORM, "purple worm", "Purple Worm", "Purple Worm", "PWrm", "PWrm",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_PURPLE_WORM,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, Y);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_CRIMSON_WORM, "crimson worm", "Crimson Worm", "Crimson Worm", "CWrm", "CWrm",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_CRIMSON_WORM,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, Y);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LEOPARD, "leopard", "Leopard", "Leopard", "Leop", "Leop",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LEOPARD,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LION, "lion", "Lion", "Lion", "Lion", "Lion",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LION,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_TIGER, "tiger", "Tiger", "Tiger", "Tigr", "Tigr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_TIGER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_BLACK_BEAR, "black bear", "Black Bear", "Black Bear", "BlBr", "BlBr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_BLACK_BEAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_BROWN_BEAR, "brown bear", "Brown Bear", "Brown Bear", "BrBr", "BrBr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_BROWN_BEAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_POLAR_BEAR, "polar bear", "Polar Bear", "Polar Bear", "PlBr", "PlBr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_POLAR_BEAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_RHINOCEROS, "rhinoceros", "Rhinoceros", "Rhinoceros", "Rino", "Rino",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_RHINOCEROS,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, Y, Y);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_BOAR, "boar", "Boar", "Boar", "Boar", "Boar",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_BOAR,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, Y, Y);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_APE, "ape", "Ape", "Ape", "Ape", "Ape",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_APE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                        simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_RAT, "rat", "Rat", "Rat", "Rat", "Rat",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_TINY, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_RAT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_WOLF, "wolf", "Wolf", "Wolf", "Wolf", "Wolf",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_WOLF,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HORSE, "horse", "Horse", "Horse", "Hors", "Hors",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HORSE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, Y, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_CONSTRICTOR_SNAKE, "constrictor snake", "Constrictor Snake", "Constrictor Snake", "CSnk", "CSnk",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_CONSTRICTOR_SNAKE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GIANT_CONSTRICTOR_SNAKE, "giant constrictor snake", "Giant Constrictor Snake",
           "Giant Constrictor Snake", "GCSk", "GCSk",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GIANT_CONSTRICTOR_SNAKE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_DIRE_CONSTRICTOR_SNAKE, "dire constrictor snake", "Dire Constrictor Snake",
           "Dire Constrictor Snake", "DCSk", "DCSk",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DIRE_CONSTRICTOR_SNAKE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MEDIUM_VIPER, "medium viper", "Medium Viper", "Medium Viper", "MVip", "MVip",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MEDIUM_VIPER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_LARGE_VIPER, "large viper", "Large Viper", "Large Viper", "LVip", "LVip",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LARGE_VIPER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HUGE_VIPER, "huge viper", "Huge Viper", "Huge Viper", "HVip", "HVip",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HUGE_VIPER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_DIRE_VIPER, "dire viper", "Dire Viper", "Dire Viper", "DVip", "DVip",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DIRE_VIPER,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_WOLVERINE, "wolverine", "Wolverine", "Wolverine", "Wlvr", "Wlvr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_WOLVERINE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_CROCODILE, "crocodile", "Crocodile", "Crocodile", "Croc", "Croc",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_CROCODILE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GIANT_CROCODILE, "giant crocodile", "Giant Crocodile", "Giant Crocodile", "GCrc", "GCrc",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GIANT_CROCODILE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_DIRE_CROCODILE, "dire crocodile", "Dire Crocodile", "Dire Crocodile", "DCrc", "DCrc",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_DIRE_CROCODILE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_CHEETAH, "cheetah", "Cheetah", "Cheetah", "Chet", "Chet",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ANIMAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_CHEETAH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /*********/
  /* Plant */
  /*********/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MANDRAGORA, "mandragora", "Mandragora", "Mandragora", "Mand", "Mand",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MANDRAGORA,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MYCANOID, "mycanoid", "Mycanoid", "Mycanoid", "Mycd", "Mycd",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MYCANOID,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SHAMBLING_MOUND, "shambling mound", "Shambling Mound", "Shambling Mound", "Shmb", "Shmb",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_SHAMBLING_MOUND,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_TREANT, "treant", "Treant", "Treant", "Trnt", "Trnt",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_TREANT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GREATER_TREANT, "greater treant", "Greater Treant", "Greater Treant", "GTrt", "GTrt",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GREATER_TREANT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_ELDER_TREANT, "elder treant", "Elder Treant", "Elder Treant", "ETrt", "ETrt",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_ELDER_TREANT,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/

  /*************/
  /* Elemental */
  /*************/

  /* FIRE */
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SMALL_FIRE_ELEMENTAL, "small fire elemental", "Small Fire Elemental", "Small Fire Elemental", "SFEl", "SFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_SMALL_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MEDIUM_FIRE_ELEMENTAL, "medium fire elemental", "Medium Fire Elemental", "Medium Fire Elemental", "MFEl", "MFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MEDIUM_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_LARGE_FIRE_ELEMENTAL, "large fire elemental", "Large Fire Elemental", "Large Fire Elemental", "LFEl", "LFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LARGE_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HUGE_FIRE_ELEMENTAL, "huge fire elemental", "Huge Fire Elemental", "Huge Fire Elemental", "HFEl", "HFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HUGE_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GARGANTUAN_FIRE_ELEMENTAL, "greater fire elemental", "Greater Fire Elemental", "Greater Fire Elemental", "GFEl", "GFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GARGANTUAN_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_COLOSSAL_FIRE_ELEMENTAL, "elder fire elemental", "Elder Fire Elemental", "Elder Fire Elemental", "EFEl", "EFEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_COLOSSAL_FIRE_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, Y, N, N, N, N, N, N);
  /****************************************************************************/
  /* Earth */
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SMALL_EARTH_ELEMENTAL, "small earth elemental", "Small Earth Elemental", "Small Earth Elemental", "SEEl", "SEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_SMALL_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MEDIUM_EARTH_ELEMENTAL, "medium earth elemental", "Medium Earth Elemental", "Medium Earth Elemental", "MEEl", "MEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MEDIUM_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_LARGE_EARTH_ELEMENTAL, "large earth elemental", "Large Earth Elemental", "Large Earth Elemental", "LEEl", "LEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LARGE_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HUGE_EARTH_ELEMENTAL, "huge earth elemental", "Huge Earth Elemental", "Huge Earth Elemental", "HEEl", "HEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HUGE_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GARGANTUAN_EARTH_ELEMENTAL, "greater earth elemental", "Greater Earth Elemental", "Greater Earth Elemental", "GEEl", "GEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GARGANTUAN_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_COLOSSAL_EARTH_ELEMENTAL, "elder earth elemental", "Elder Earth Elemental", "Elder Earth Elemental", "EEEl", "EEEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_COLOSSAL_EARTH_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  /****************************************************************************/
  /* Air */
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SMALL_AIR_ELEMENTAL, "small air elemental", "Small Air Elemental", "Small Air Elemental", "SAEl", "SAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_SMALL_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MEDIUM_AIR_ELEMENTAL, "medium air elemental", "Medium Air Elemental", "Medium Air Elemental", "MAEl", "MAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MEDIUM_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_LARGE_AIR_ELEMENTAL, "large air elemental", "Large Air Elemental", "Large Air Elemental", "LAEl", "LAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LARGE_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HUGE_AIR_ELEMENTAL, "huge air elemental", "Huge Air Elemental", "Huge Air Elemental", "HAEl", "HAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HUGE_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GARGANTUAN_AIR_ELEMENTAL, "greater air elemental", "Greater Air Elemental", "Greater Air Elemental", "GAEl", "GAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GARGANTUAN_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_COLOSSAL_AIR_ELEMENTAL, "elder air elemental", "Elder Air Elemental", "Elder Air Elemental", "EAEl", "EAEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_COLOSSAL_AIR_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, Y, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /* Water */
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SMALL_WATER_ELEMENTAL, "small water elemental", "Small Water Elemental", "Small Water Elemental", "SWEl", "SWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_SMALL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_SMALL_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_MEDIUM_WATER_ELEMENTAL, "medium water elemental", "Medium Water Elemental", "Medium Water Elemental", "MWEl", "MWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_MEDIUM_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_LARGE_WATER_ELEMENTAL, "large water elemental", "Large Water Elemental", "Large Water Elemental", "LWEl", "LWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_LARGE_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_HUGE_WATER_ELEMENTAL, "huge water elemental", "Huge Water Elemental", "Huge Water Elemental", "HWEl", "HWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_HUGE_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GARGANTUAN_WATER_ELEMENTAL, "greater water elemental", "Greater Water Elemental", "Greater Water Elemental", "GWEl", "GWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_GARGANTUAN, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GARGANTUAN_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_COLOSSAL_WATER_ELEMENTAL, "elder water elemental", "Elder Water Elemental", "Elder Water Elemental", "EWEl", "EWEl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_ELEMENTAL, SIZE_COLOSSAL, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_COLOSSAL_WATER_ELEMENTAL,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, Y, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, Y, N, N, N, N, N, N, N);
  /****************************************************************************/

  /* monstrous humanoid */
  /*
  add_race(RACE_HALF_TROLL, "half troll", "HalfTroll", "Half Troll", RACE_TYPE_MONSTROUS_HUMANOID, N, Y, Y, 2, 4, 0, 0, 0, -2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, TRUE, CLASS_WARRIOR, SKILL_LANG_GOBLIN, 0);
  */

  /* giant */
  /*
  add_race(RACE_HALF_OGRE, "half ogre", "HlfOgre", "Half Ogre", RACE_TYPE_GIANT, N, Y, Y, 6, 4, -2, 0, 2, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_LARGE, FALSE, CLASS_BERSERKER, SKILL_LANG_GIANT, 2);
    */

  /* undead */
  /*
  add_race(RACE_SKELETON, "skeleton", "Skeletn", "Skeleton", RACE_TYPE_UNDEAD, Y, N, N, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_ZOMBIE, "zombie", "Zombie", "Zombie", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHOUL, "ghoul", "Ghoul", "Ghoul", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_GHAST, "ghast", "Ghast", "Ghast", RACE_TYPE_UNDEAD, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MUMMY, "mummy", "Mummy", "Mummy", RACE_TYPE_UNDEAD, N, Y, Y, 14, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  add_race(RACE_MOHRG, "mohrg", "Mohrg", "Mohrg", RACE_TYPE_UNDEAD, N, Y, Y, 11, 0, 0, 0, 9, 0,
          N, N, N, N, Y, N, N, N, N, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
    */

  /* ooze */

  /* magical beast */
  /*
  add_race(RACE_BLINK_DOG, "blink dog", "BlinkDog", "Blink Dog", "BlDg", "BlDg",
           // race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic?
           RACE_TYPE_MAGICAL_BEAST, SIZE_MEDIUM, FALSE, 0, 0, IS_NORMAL);

  set_race_attack_types(RACE_BLINK_DOG,
                      // hit sting whip slash bite bludgeon crush pound claw maul thrash pierce
                      N, N, N, N, Y, N, N, N, Y, N, N, N,
                      // blast punch stab slice thrust hack rake peck smash trample charge gore
                      N, N, N, N, N, N, N, N, N, N, N, N);
  */
  add_race(RACE_MANTICORE, "manticore", "Mnticore", "Manticore", "Mntc", "Mntc",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_MAGICAL_BEAST, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);

  set_race_attack_types(RACE_MANTICORE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);

  /* fey */

  add_race(RACE_PIXIE, "pixie", "Pixie", "Pixie", "Pixi", "Pixi",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_FEY, SIZE_TINY, FALSE, 0, 0, IS_NORMAL);

  set_race_attack_types(RACE_PIXIE,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, Y, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        Y, N, N, N, N, N, N, N, N, N, N, N);

  /* construct */

  add_race(RACE_IRON_GOLEM, "iron golem", "IronGolem", "Iron Golem", "IrGl", "IrGl",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_CONSTRUCT, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_IRON_GOLEM,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);

  /* outsiders */

  add_race(RACE_EFREETI, "efreeti", "Efreeti", "Efreeti", "Efrt", "Efrt",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_OUTSIDER, SIZE_LARGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_EFREETI,
                        // hit sting whip slash bite bludgeon crush pound claw maul thrash pierce
                        N, N, N, Y, N, N, N, N, N, N, Y, N,
                        // blast p slice thrust hack rake peck smash trample charge gore
                        N, N, N, N, N, N, N, N, N, N, N, N);

  /*
  add_race(RACE_AEON_THELETOS, "aeon theletos", "AeonThel", "Theletos Aeon", RACE_TYPE_OUTSIDER, N, Y, Y, 0, 0, 0, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_WARRIOR, SKILL_LANG_COMMON, 0);
  */

  /* dragon */
  /*
  add_race(RACE_DRAGON_CLOUD, "dragon cloud", "DrgCloud", "Cloud Dragon", RACE_TYPE_DRAGON, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_HUGE, FALSE, CLASS_WARRIOR, SKILL_LANG_DRACONIC, 0);
    */

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_WHITE_DRAGON, "white dragon", "WhtDragn", "White Dragon", "WhDr", "WhDr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_DRAGON, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_WHITE_DRAGON,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_BLACK_DRAGON, "black dragon", "BlkDragn", "Black Dragon", "BlDr", "BlDr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_DRAGON, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_BLACK_DRAGON,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_GREEN_DRAGON, "green dragon", "GrnDragn", "green Dragon", "GrDr", "GrDr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_DRAGON, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_GREEN_DRAGON,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_BLUE_DRAGON, "blue dragon", "BluDragn", "Blue Dragon", "BlDr", "BlDr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_DRAGON, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_WHITE_DRAGON,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_RED_DRAGON, "red dragon", "RedDragn", "Red Dragon", "RdDr", "RdDr",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_DRAGON, SIZE_HUGE, FALSE, 0, 0, IS_NORMAL);
  set_race_attack_types(RACE_RED_DRAGON,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, Y, N, N, N, Y, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, N, N, N, N);
  /****************************************************************************/

  /* aberration */
  /*
  add_race(RACE_TRELUX, "trelux", "Trelux", "Trelux", RACE_TYPE_ABERRATION, N, Y, Y, 0, 0, 0, 0, 0, 0,
          N, N, N, N, Y, N, Y, Y, Y, SIZE_SMALL, TRUE, CLASS_WARRIOR, SKILL_LANG_ABERRATION, 0);
    */

  /* end listing */
}

// interpret race for interpreter.c and act.wizard.c etc
// notice, epic races are not manually or in-game settable at this stage
int parse_race(char arg)
{
  arg = LOWER(arg);

  switch (arg)
  {
  case 'a':
    return RACE_HUMAN;
  case 'b':
    return RACE_ELF;
  case 'c':
    return RACE_DWARF;
  case 'd':
    return RACE_HALF_TROLL;
  case 'f':
    return RACE_HALFLING;
  case 'g':
    return RACE_H_ELF;
  case 'h':
    return RACE_H_ORC;
  case 'i':
    return RACE_GNOME;
  case 'j':
    return RACE_ARCANA_GOLEM;
  case 'k':
    return RACE_DROW;
  case 'l':
    return RACE_DUERGAR;
  default:
    return RACE_UNDEFINED;
  }
}

/* accept short descrip, return race */
int parse_race_long(const char *arg_in)
{
  size_t arg_sz = strlen(arg_in) + 1;
  char arg_buf[arg_sz];
  strlcpy(arg_buf, arg_in, arg_sz);
  char *arg = arg_buf;

  int l = 0; /* string length */

  for (l = 0; *(arg + l); l++) /* convert to lower case */
    *(arg + l) = LOWER(*(arg + l));
#if defined(CAMPAIGN_DL)

  if (is_abbrev(arg, "human")) return  DL_RACE_HUMAN;
  if (is_abbrev(arg, "qualinesti elf")) return  DL_RACE_QUALINESTI_ELF;
  if (is_abbrev(arg, "qualinesti-elf")) return  DL_RACE_QUALINESTI_ELF;
  if (is_abbrev(arg, "qualinestielf")) return  DL_RACE_QUALINESTI_ELF;
  if (is_abbrev(arg, "silvanesti elf")) return  DL_RACE_SILVANESTI_ELF;
  if (is_abbrev(arg, "silvanesti-elf")) return  DL_RACE_SILVANESTI_ELF;
  if (is_abbrev(arg, "silvanestielf")) return  DL_RACE_SILVANESTI_ELF;
  if (is_abbrev(arg, "kagonesti elf")) return  DL_RACE_KAGONESTI_ELF;
  if (is_abbrev(arg, "kagonesti-elf")) return  DL_RACE_KAGONESTI_ELF;
  if (is_abbrev(arg, "kagonestielf")) return  DL_RACE_KAGONESTI_ELF;
  // if (is_abbrev(arg, "dargonesti elf")) return  DL_RACE_DARGONESTI_ELF;
  // if (is_abbrev(arg, "dargonesti-elf")) return  DL_RACE_DARGONESTI_ELF;
  // if (is_abbrev(arg, "dargonestielf")) return  DL_RACE_DARGONESTI_ELF;
  if (is_abbrev(arg, "mountain dwarf")) return  DL_RACE_MOUNTAIN_DWARF;
  if (is_abbrev(arg, "mountain-dwarf")) return  DL_RACE_MOUNTAIN_DWARF;
  if (is_abbrev(arg, "mountaindwarf")) return  DL_RACE_MOUNTAIN_DWARF;
  if (is_abbrev(arg, "hill dwarf")) return  DL_RACE_HILL_DWARF;
  if (is_abbrev(arg, "hill-dwarf")) return  DL_RACE_HILL_DWARF;
  if (is_abbrev(arg, "hilldwarf")) return  DL_RACE_HILL_DWARF;
  if (is_abbrev(arg, "gully dwarf")) return  DL_RACE_GULLY_DWARF;
  if (is_abbrev(arg, "gully-dwarf")) return  DL_RACE_GULLY_DWARF;
  if (is_abbrev(arg, "gullydwarf")) return  DL_RACE_GULLY_DWARF;
  if (is_abbrev(arg, "minotaur")) return  DL_RACE_MINOTAUR;
  if (is_abbrev(arg, "kender")) return  DL_RACE_KENDER;
  if (is_abbrev(arg, "gnome")) return  DL_RACE_GNOME;
  if (is_abbrev(arg, "half elf")) return  DL_RACE_HALF_ELF;
  if (is_abbrev(arg, "half-elf")) return  DL_RACE_HALF_ELF;
  if (is_abbrev(arg, "halfelf")) return  DL_RACE_HALF_ELF;
  if (is_abbrev(arg, "baaz draconian")) return  DL_RACE_BAAZ_DRACONIAN;
  if (is_abbrev(arg, "baaz-draconian")) return  DL_RACE_BAAZ_DRACONIAN;
  if (is_abbrev(arg, "baazdraconian")) return  DL_RACE_BAAZ_DRACONIAN;
  if (is_abbrev(arg, "goblin")) return  DL_RACE_GOBLIN;
  if (is_abbrev(arg, "hobgoblin")) return  DL_RACE_HOBGOBLIN;
  if (is_abbrev(arg, "kapak draconian")) return  DL_RACE_KAPAK_DRACONIAN;
  if (is_abbrev(arg, "kapak-draconian")) return  DL_RACE_KAPAK_DRACONIAN;
  if (is_abbrev(arg, "kapakdraconian")) return  DL_RACE_KAPAK_DRACONIAN;
  if (is_abbrev(arg, "bozak draconian")) return  DL_RACE_BOZAK_DRACONIAN;
  if (is_abbrev(arg, "bozak-draconian")) return  DL_RACE_BOZAK_DRACONIAN;
  if (is_abbrev(arg, "bozakdraconian")) return  DL_RACE_BOZAK_DRACONIAN;
  // if (is_abbrev(arg, "sivak draconian")) return  DL_RACE_SIVAK_DRACONIAN;
  // if (is_abbrev(arg, "sivak-draconian")) return  DL_RACE_SIVAK_DRACONIAN;
  // if (is_abbrev(arg, "sivakdraconian")) return  DL_RACE_SIVAK_DRACONIAN;
  // if (is_abbrev(arg, "aurak draconian")) return  DL_RACE_AURAK_DRACONIAN;
  // if (is_abbrev(arg, "aurak-draconian")) return  DL_RACE_AURAK_DRACONIAN;
  // if (is_abbrev(arg, "aurakdraconian")) return  DL_RACE_AURAK_DRACONIAN;
  // if (is_abbrev(arg, "irda")) return  DL_RACE_IRDA;
  // if (is_abbrev(arg, "ogre")) return  DL_RACE_OGRE;
  if (is_abbrev(arg, "lich")) return RACE_LICH;
  if (is_abbrev(arg, "vampire")) return RACE_VAMPIRE;

#else

  if (is_abbrev(arg, "human"))
    return RACE_HUMAN;
  if (is_abbrev(arg, "moon-elf"))
    return RACE_ELF;
  if (is_abbrev(arg, "moonelf"))
    return RACE_ELF;
  if (is_abbrev(arg, "moon elf"))
    return RACE_ELF;
  if (is_abbrev(arg, "high-elf"))
    return RACE_HIGH_ELF;
  if (is_abbrev(arg, "highelf"))
    return RACE_HIGH_ELF;
  if (is_abbrev(arg, "high elf"))
    return RACE_HIGH_ELF;
  if (is_abbrev(arg, "darkelf"))
    return RACE_DROW;
  if (is_abbrev(arg, "dark-elf"))
    return RACE_DROW;
  if (is_abbrev(arg, "dark elf"))
    return RACE_DROW;
  if (is_abbrev(arg, "drowelf"))
    return RACE_DROW;
  if (is_abbrev(arg, "drow-elf"))
    return RACE_DROW;
  if (is_abbrev(arg, "drow elf"))
    return RACE_DROW;
  if (is_abbrev(arg, "mountain-dwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "mountaindwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "mountain dwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "shield-dwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "shielddwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "shield dwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "gold dwarf"))
    return RACE_GOLD_DWARF;
  if (is_abbrev(arg, "gold-dwarf"))
    return RACE_GOLD_DWARF;
  if (is_abbrev(arg, "golddwarf"))
    return RACE_GOLD_DWARF;
  if (is_abbrev(arg, "duergar"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "duergardwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "duergar-dwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "graydwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "darkdwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "gray-dwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "dark-dwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "gray dwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "dark dwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "lightfoot-halfling"))
    return RACE_HALFLING;
  if (is_abbrev(arg, "lightfoothalfling"))
    return RACE_HALFLING;
  if (is_abbrev(arg, "lightfoot halfling"))
    return RACE_HALFLING;
  if (is_abbrev(arg, "halfelf"))
    return RACE_H_ELF;
  if (is_abbrev(arg, "half-elf"))
    return RACE_H_ELF;
  if (is_abbrev(arg, "half elf"))
    return RACE_H_ELF;
  if (is_abbrev(arg, "halforc"))
    return RACE_H_ORC;
  if (is_abbrev(arg, "half-orc"))
    return RACE_H_ORC;
  if (is_abbrev(arg, "half orc"))
    return RACE_H_ORC;
  if (is_abbrev(arg, "rock-gnome"))
    return RACE_GNOME;
  if (is_abbrev(arg, "rockgnome"))
    return RACE_GNOME;
  if (is_abbrev(arg, "rock gnome"))
    return RACE_GNOME;
#ifndef CAMPAIGN_FR
  if (is_abbrev(arg, "half-troll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halftroll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "half troll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "arcanagolem"))
    return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "arcana-golem"))
    return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "arcana golem"))
    return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "trelux"))
    return RACE_TRELUX;
  if (is_abbrev(arg, "crystaldwarf"))
    return RACE_CRYSTAL_DWARF;
  if (is_abbrev(arg, "crystal-dwarf"))
    return RACE_CRYSTAL_DWARF;
  if (is_abbrev(arg, "crystal dwarf"))
    return RACE_CRYSTAL_DWARF;
#endif
  if (is_abbrev(arg, "wood-elf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "wild-elf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "woodelf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "wildelf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "wild elf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "wood elf"))
    return RACE_WOOD_ELF;
  if (is_abbrev(arg, "dragonborn"))
    return RACE_DRAGONBORN;
  if (is_abbrev(arg, "dragon-born"))
    return RACE_DRAGONBORN;
  if (is_abbrev(arg, "dragon born"))
    return RACE_DRAGONBORN;
  if (is_abbrev(arg, "halfdrow"))
    return RACE_HALF_DROW;
  if (is_abbrev(arg, "half-drow"))
    return RACE_HALF_DROW;
  if (is_abbrev(arg, "half drow"))
    return RACE_HALF_DROW;
  if (is_abbrev(arg, "tiefling"))
    return RACE_TIEFLING;
  if (is_abbrev(arg, "teifling"))
    return RACE_TIEFLING;
  if (is_abbrev(arg, "forestgnome"))
    return RACE_FOREST_GNOME;
  if (is_abbrev(arg, "forest-gnome"))
    return RACE_FOREST_GNOME;
  if (is_abbrev(arg, "forest gnome"))
    return RACE_FOREST_GNOME;
  if (is_abbrev(arg, "stouthalfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "stout-halfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "stout halfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "stronghearthalfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "strongheart-halfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "strongheart halfling"))
    return RACE_STOUT_HALFLING;
  if (is_abbrev(arg, "aasimar"))
    return RACE_AASIMAR;
  if (is_abbrev(arg, "tabaxi"))
    return RACE_TABAXI;
  if (is_abbrev(arg, "shade"))
    return RACE_SHADE;
  if (is_abbrev(arg, "goliath"))
    return RACE_GOLIATH;
  if (is_abbrev(arg, "lich"))
    return RACE_LICH;
  if (is_abbrev(arg, "vampire"))
    return RACE_VAMPIRE;
  if (is_abbrev(arg, "fae"))
    return RACE_FAE;
  if (is_abbrev(arg, "goblin"))
    return RACE_GOBLIN;
  if (is_abbrev(arg, "hobgoblin"))
    return RACE_HOBGOBLIN;
#endif

  return RACE_UNDEFINED;
}

// returns the proper integer for the race, given a character
bitvector_t find_race_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; rpos < strlen(arg); rpos++)
    ret |= (1 << parse_race(arg[rpos]));

  return (ret);
}

/* Invalid wear flags */
int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if ((OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_HALF_TROLL) && IS_HALF_TROLL(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_HALFLING) && IS_HALFLING(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ELF) && IS_H_ELF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_H_ORC) && IS_H_ORC(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_CRYSTAL_DWARF) && IS_CRYSTAL_DWARF(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_TRELUX) && IS_TRELUX(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_LICH) && IS_LICH(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_VAMPIRE) && IS_VAMPIRE(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_VAMPIRE_ONLY) && !IS_VAMPIRE(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANA_GOLEM) && IS_ARCANA_GOLEM(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DROW) && IS_DROW(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DUERGAR) && IS_DUERGAR(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch)))
    return 1;
  else
    return 0;
}

sbyte has_racial_abils_unchosen(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  switch (GET_RACE(ch))
  {
  case RACE_HIGH_ELF:
    if (HIGH_ELF_CANTRIP(ch) == 0)
      return true;
    break;
  case DL_RACE_SILVANESTI_ELF:
    if (HIGH_ELF_CANTRIP(ch) == 0)
      return true;
    break;
  case RACE_DRAGONBORN:
    if (!GET_DRAGONBORN_ANCESTRY(ch))
      return true;
    break;
  }
  return false;
}

int get_random_basic_pc_race(void)
{

  int num = dice(1, NUM_RACES + 1);
  num--;

  while (race_list[num].epic_adv != IS_NORMAL)
  {
    num = dice(1, NUM_RACES + 1);
    num--;
  }

  return num;
}

/* can a class be this race because of potential alignment issues? (character creation) */
int valid_class_race_alignment(int class, int race)
{
  int i = 0;

  for (i = 0; i < NUM_ALIGNMENTS; i++)
  {
    if (valid_align_by_class(i, class) &&
        valid_align_by_race(i, race))
      return 1;
  }

  /*nothing!*/
  return 0;
}

/* returns 1 for valid alignment, returns 0 for problem with alignment */
int valid_align_by_race(int alignment, int race)
{
  return (race_list[race].alignments[alignment]);
}

#if defined(CAMPAIGN_DL)
const char *get_region_info(int region)
{
  switch (region)
  {
    case REGION_ABANASINIA: return "Abanasinia found itself near the shores of the New Sea, created after the Fiery Mountain struck Krynn. Perhaps the most relaxed region of Ansalon, the plains are home to numerous tribes of plains barbarians, as well as the cities of Solace, Haven, Gateway and at least one hill dwarf community. Whilst the region is best known for it's wide and verdant plains, it also has a number of city states dotted throughout. Those from Abanasinia are called Abanasinian.";
    case REGION_BALIFOR: return "When the Cataclysm struck Balifor, much of the southern regions of the nation crumbled and sank into the ocean, killing many thousands. The kender and other beings more inland were able to escape death. Since the Cataclysm, the land turned into a hot desert wasteland, and most kender decided to pack up and founded Kendermore to the east instead. Humans dominate the land there now, with most living in cities or as nomads in the desert. In the winter of 348 AC - 349 AC, the Black Dragonarmy invaded Balifor, quickly seizing and taking control of the lands. ";
    case REGION_BLODE: return "Following the Cataclysm, the ogres of Blode moved down and started claiming land around the Khalkist Mountains. They established a principality to the west of the Khalkist Mountains, naming it Blodehelm (which translates into Common as 'Home of Blode'). The rich and fertile plains initially became the agricultural center of the region, however the arrival of human warlords from the Plains of Dust, forced a bloody conflict between the warlords and the ogres. Ultimately the warlords and the ogres drew up a pact, in which the warlords claimed Blodehelm for themselves and the ogres remained in their mountainous home of Blode. During the War of the Lance, Blode sided with the Nerakans and the Queen of Darkness in fighting Paladine's forces of good. They were not hoping to get right into the fighting right away, but were hoping they could take over Silvanesti due to the elves being an age-old enemy. They also provided a lot of food and supplies from Blodehelm to keep the Dragonarmies marching.";
    case REGION_BLOOD_SEA_ISLES: return "Blood Sea Isles are located off the northeastern coast of Ansalon. They were formed from the lands of the Empire of Istar, including Falthana, Gather, Midrath, and Seldjuk. Almost everyone that lives in the Isles is a mariner of some sort, from the smallest fisherman to the mightiest admirals of the Minotaur Empire. The people make a living ranging from fishing and hauling cargo to being a privateer and making war. Also known as the Blood Sea Isles, these islands are the home of sea barbarians and minotaur. Legend states that when the fiery mountain struck Krynn, these islands survived because of their innocence of the guilt of Istar. More likely, however, is the fact that these islands were originally part of the coastal islands bordering on the Courrain Ocean, and survived due to their altitude. Saifhum is the home of many sea barbarians, and not much else. The rocky highlands do not sport much in the form of anything living. The only real evidence of civilization is the capital, Sea Reach, which lies on the southwest shore of the island. Saifhum is the western most island of the four. Karthay is the largest island is also furthest north, and is shaped somewhat like a crescent moon. The dry plains in the eastern and western portions of the island are interrupted only by high peaks, known as the Worldscap Mountains. The only evidence of civilization is Winston's Tower-a ruined lighthouse. Mithas lies directly to the south of Karthay, and is one of the minotaur isles. Along the southern shore of the island are four volcanic peaks, a sharp contrast to the rest of the island's grassy plains. The capital of Lacynos (Nethosak) lies on the western shores, and is sheltered from the ravages of the Blood Sea by the Horned Bay. Though it does not have the volcanic problem of its northern neighbor, Kothas doesn't have the vegetation to make up for it. Minotaur rule this island, along with human pirates who live in the capital of Kalpethis. Kothas lies to the south of Mithas, and Kalpethis lies on the southwestern shores of the island.";
    case REGION_ENSTAR: return "Enstar is a human island located south of Southern Ergoth, west of Qualinesti, and northwest of Nostar. Its major geographical features are the Sirrion Sea on its western and southern border, with the Straits of Algoni to the east of it. The island is mostly flat grassland.";
    case REGION_ESTWILDE: return "Estwilde is a mostly-human tribal nation that covers a long and narrow region stretching from Kalaman Bay in the absolute northern reaches of Ansalon, and then spreading downwards and ending at the New Sea in the central part of the continent of Ansalon. The region is bordered by Lemish, Throt and Nightlund on its western border, Taman Busuk on its eastern side, and Nordmaar on its northeast border. The humans of Estwilde are split between the natives who hail from the Lor-Tai tribe, the cannibals of the Lahutian tribe, the wild mountain barbarians of Estwilde, and the civilised folk of the South Shore. Most outsiders deal with the mountain barbarians, who offer themselves as foot soldiers and mercenaries to those with enough money to hire their services. In the early years following the Cataclysm, Estwilde was often where skirmishes and border fights were played out between the forces of Solamnia and those of Taman Busuk. During the War of the Lance, the Dragonarmies invaded the Estwilde region from Taman Busuk, quickly conquering the realm and turning it into an ally to the dark armies. The invasion really didn't affect the people of the land much, and the Dragonarmies used the land to build up and train their forces, while launching attacks upon Solamnia.";
    case REGION_GOODLUND: return "Goodlund is located east of Balifor, south of the Minotaur Empire, and southeast of Khur. The geographical features of the lands are the Blood Sea of Istar, which borders the northern coast and the Southern Courrain Ocean on the south and east. The forests of Wendle Woods and Beast's Run are located on the west and southern portion of the land, while the rivers Lifesbreath and Heartsblood lead out into The Maw. Furthermore, several prominent geographical features are located along the coastline, including the Writhing Wreck, Habbakuk's Necklace, the Restless Waters, Thunderhead, Boiler's Bay, Churning Reach, Mistlestraits, Land's End, and the Sombre Coast. During the War of the Lance, the White Dragonarmy invaded Goodlund in the winter of 348 AC - 349 AC. The dragonarmy's intention was to isolate the kender of Kendermore, while the evil creatures of the Laughing Lands have either allied with the White Wing or were impressed into the army's service. The Dairly Plains are the only area of Goodlund that withstood the dragonarmy's advance and continued its resistance throughout the war.";
    case REGION_HYLO: return "Hylo is a kender nation that is located north of Northern Ergoth and Sikk'et Hul with the Straits of Algoni to the east. Before the Cataclysm, Hylo was located near the Empire of Ergoth's provinces of the Eastern Hundred and the Mountain Hundred. The kender from Hylo are called Hyloian. The forests of the land are temperate year round, very little snow falls in the winter months except for a few flurries. In order to become the leader of Hylo for a year, you must win some sort of contest on Election Day, which could involve spitting seeds, who can swim the fastest, or even stand on your head the longest. While the War of the Lance raged across Ansalon, the kender of Hylo were completely unaware that a war was even going on. Some tales were brought to the kender about a war, but most thought it was just a good story. The goblins of Sikk'et Hul pledged an alliance with both Hylo and Northern Ergoth during this time.";
    case REGION_KAYOLIN: return "Kayolin, sometimes spelled Kaolyn, is a dwarven kingdom that is set in the peaks of the Garnet Mountains and is bordered by Solamnia on its northern, southern and western sides, and Lemish to the east. A small number of towns and fortifications are readily visible in the mountain range, but the large cities of Kayolin are all based underground beneath the mountains. Kayolin is populated by the Hylar, Daewar, Neidar and Aghar dwarves, who trade and deal openly with humans and merchants of other races. The realm of Kayolin is a peaceful one that does not have the clan fighting that is contained in the other kingdoms of the dwarves, and is ruled by a single nominated governor. The Cataclysm severed the close ties that Kayolin had with Thorbardin, with the New Sea creating a gap between the two dwarven kingdoms. The smaller kingdom learnt to survive on its own and boosted their production of metals and manufactured goods, becoming the primary supplier of these components for northern Ansalon. The dwarves of Kayolin then formed an alliance with the Knights of Solamnia in keeping the forces of Lemish at bay, and forced strengthened the relationship with their human neighbours in Solamnia.";
    case REGION_KHUR: return "Khur is a Human nomadic nation that is located southeast of Taman Busuk, northeast of Blöde, west of the Ogrelands, north of Silvanesti, and east of Thoradin. It is a mountainous and desert region with scattered oases & shrubs that are usually controlled by one of the Khur tribes there. Geographical features include the Khalkist Mountains in the west and north, the Burning Lands in southern Khur, and the Khurman Sea on the southeastern border. The nation is led by the Khan of Khur and inhabited by the seven tribes which form the Nomads of Khur. The people of Khur love their horses, and are known for breeding the best in all of Ansalon. Races are held between the tribes to prove which breeds the best and wagers are placed on the outcomes. Khur came into being after the Cataclysm struck Krynn. A leader by the name of Keja united all the tribes together into one nation. He had seven sons who, after his death, vied for power and ended up splitting the nation into seven different tribes. Over time this area has bred hardy warriors from the desert. During the War of the Lance, the Green Dragonarmy was invited into Khur for their war on Ansalon. They began seizing the cities and water sources until a Khurish warrior named Salah-Khan became the next head of the Green Dragonarmy. After the War of the Lance, the Green Dragonarmy was no longer in Khur.";
    case REGION_LEMISH: return "Lemish is a human land that is bordered by Solamnia in the west and north, Throt in the east, and New Sea in the south. Its major geographical features are the Southern Dargaard Mountains on its eastern border, Godsfell Woods on the northeast corner, the Darkwoods are a large forest that take up most of the land, and the Garnet Mountains on its southwestern border with Solamnia. Seventy-five percent of the land is covered in thick forests. The Great Seven Gods Highway brought a lot of trade through Lemish prior to the Cataclysm. It entered into Lemish from the south and went east to Sanction. At least one Grand Master even hailed from this Solamnic province in 1634 PC by the name of Gregori uth Telan. The nation was ruled by the Dictator of Lemish. Following the Cataclysm Lemish was completely cut off from Ergoth, and built port cities to capitalize on the sudden appearance of the Newsea at their southern border. The Lemish people continued to blame the Solamnics to the north, while they just tried to survive. Solamnia has tried to annex Lemish multiple times after the Cataclysm, yet have continually been driven back by the Lemish natives. At one point the Lemish goaded the Nerakans and the Solamnics into a war that lasted fifty years. These became known as the Nerakan Wars. During the War of the Lance, Lemish allied with the forces of the Dark Queen in order to be able to attack Solamnia. Their forces led the attacks against Garnet, Caergoth, and Solanthus, but were repelled from invading Kayolin.";
    case REGION_NIGHTLUND: return "Nightlund is a Human land that shares its western border with Solamnia. Nightlund is north of both Lemish and Throt, west of Estwilde, and southwest of Nordmaar. Major geographical features include the Dargaard Mountains on the eastern border with Estwilde, and the Vingaard River which runs the length of Nightlund's western border and north to Kalaman. There is a large forested land of cypress trees west of the Dargaard Mountains called \"The Grove\". Heavily wooded and mountainous, the land was never suitable for farming. Even before the Cataclysm, it was only sparsely populated. In the time following the Cataclysm, Knightlund was renamed to Nightlund for the perpetual twilight that shrouds the land, even during the daytime. The Cataclysm was particularly devastating to this region of Ansalon. The mountains were split by earthquakes, the river overflowed and shifted course, destroying most of the settlements. It was known as a place of disease and plagues, killing the inhabitants of the land. The roads and bridges were similarly destroyed and eventually reclaimed by nature. By the time of the War of the Lance, only outlaws choose to live here.";
    case REGION_NORDMAAR: return "Nordmaar is a human nomadic nation that is located northeast of Estwilde, north of Taman Busuk, in northeastern Ansalon. The north and east of the realm is bordered by the Northern Courrain Ocean with the Last Coast being on the northeastern side, whilst the Turbidus Ocean lies on the western side of Nordmaar. A few of the major geographical features are The Great Moors, the Southern Wastes, the Sahket Jungle, the Emerald Peaks, The Horseman and the Fountain of Renewal. The northern part of the land is tropical with many rainforests and jungles, but as you go farther south towards Taman Busuk, it is more arid. The humans of the realm are split between various tribes and are further factioned between the Nordmen of the cities, who all offer fealty to a single king, who resides in North Keep, and the Horselords, who loyally serve the Khan of the Southern Wastes. Prior to 348 AC Nordmaar had only fought one known war, the War of the Sky, against the Knights of Solamnia in 202 AC. After the war, Nordmaar and Solamnia forged an alliance that remains to this day. During the War of the Lance, the Red Dragonarmy invaded, taking over all of Nordmaar. The Nordmen were quickly subjugated by their enemy, however the Horselords maintained a steady fight, which resulted in large losses for the nomadic humans. ";
    case REGION_NORTHERN_ERGOTH: return "Northern Ergoth is an island located west of Ansalon and north of Southern Ergoth that was created when the Cataclysm tore Ergoth in two. Northern Ergoth shares its northern border with the kender nation of Hylo and its southern border with the goblin province of Sikk'et Hul. The major geographical features of the island are the Sirrion Sea that surrounds the island, the Sentinel Mountains that serve as a border between Northern Ergoth and Hylo, and Raekel's Pit located in the southern crook of the Sentinel Mountains. The lands of Northern Ergoth are mostly grasslands and coastal plains with the province of Sikk'et Hul a desert of hills and scrubland. The people of the land are usually dark-skinned seagoing people, with large fishing vessels. Rice paddies are common on the island. The island's armies are organized into legions. During the War of the Lance the dragonarmies made attempts to invade Northern Ergoth but were driven back from the island.";
    case REGION_NOSTAR: return "Nostar is a large island located west of Ansalon, 40 miles southeast of Enstar, and over 80 miles south of Southern Ergoth. Nostar is roughly 60 miles east to west and 40 miles north to south. It is an island of large grasslands, forests, and rolling hills, mostly inhabited by goblins, hobgoblins, and bugbears in the interior, with a few humans and elves living close to the coastline. The geographical features of this island are a rock formation known as the Three Brothers, located on the eastern part of the island, and an unnamed egg-shaped lake in the center of the island. Nostar was also known for a type of black wine that was produced on the island.";
    case REGION_QUALINESTI: return "Qualinesti, one of the prominent elven nations on Ansalon, was a realm of great beauty and grace. Nestled deep within the towering forests of the continent of Ansalon, Qualinesti was home to the Qualinesti Elves, a subrace of the elven people known for their wisdom, refinement, and dedication to preserving the natural world. The nation was ruled by the Speaker of the Suns, who was both a political leader and a spiritual guide for the elves. The capital city, Qualinost, was a breathtaking sight, featuring intricately designed treetop cities and bridges, emphasizing the elves' close connection to the forest. The elven architecture was a testament to their harmony with nature, as they sought to coexist with the land rather than dominate it. During the War of the Lance, Qualinesti found itself in a perilous situation as it faced the threat of the dragonarmies of Takhisis. Despite their commitment to neutrality and peace, the Qualinesti Elves were forced to defend their homeland against the advancing armies. The war had a profound impact on Qualinesti, testing the elves' strength, determination, and their relationship with their neighboring elven nation, Silvanesti. Qualinesti's resilience and the sacrifices made by its people during this tumultuous time became emblematic of the elven spirit and their enduring connection to the land they cherished.";
    case REGION_SANCRIST_ISLE: return "The Isle of Sancrist is the westernmost landmass of all of Ansalon. It is located west of Northern Ergoth and north and northwest of Cristyne in the Sirrion Sea. The isle is considered to have two sections: Gunthar and Sancrist. The Sancrist area has many mountain ranges, including the Gargath Mountains, the Majestic Mountains, the Numbered Mountains, the Skyfisher Range, the Sun Range, and the Caves of Pyrothraxus. The Gunthar section of Sancrist Isle has the Whitestone River which leads from the Whitestone Glade to Thalan Bay. It is where the Whitestone Council was held that united the forces of good against the evil Dragonarmies, and it also is home to Mount Nevermind, the peculiar mountain-city of most gnomes who call Krynn home.";
    case REGION_SCHALLSEA: return "Schallsea is an island east of Abanasinia. It is, for the most part, populated by humans. It is located in Newsea, separated from Abanasinia in by the Straits of Schallsea. Schallsea is about 200 miles long and 80 miles wide. Schallsea has a large croup of hills called the Barren Hills in the center, dividing it roughly in half. Schallsea also has a number of streams. Schallsea did not exist prior to the Cataclysm, it was formed when Newsea flooded the lands in the middle of Ansalon. The Blue Dragonarmies fought the Que-Nal, a group of people on the island, on Schallsea.";
    case REGION_SILVANESTI: return "The original elven kingdom in Ansalon, from where the Kagonesti and Qualinesti elves probably came. Holding capital at Silvanost, this kingdom is famous for its marble buildings, garden-like forests and the towers of Eru at the mouths of the Thon-Thalas. The Cataclysm ended Istar's dominance over the world. After the disaster, Silvanesti sent out scouts to report the damage done to their nation. The effects of the Cataclysm had been catastrophic. Blaming the disaster upon the humans, the elves remained in their kingdom and continued to remain isolated from the world. In 349 AC, Silvanesti was drawn into the War of the Lance. First negotiating a peace with the great Dragonarmies, the elves were soon betrayed. The dark forces turned south and into the woodland border. Knowing his nation's destruction was imminent, the elven King Lorac used a dragon orb that he had rescued from Istar prior to the empire's destruction. While the population of Silvanesti fled south and west out of the nation, the king tried to drive the invading armies and dragons out of the land. Unable to control the power of the Dragonorb, the artifact instead took control of King Lorac and threw the nation of Silvanesti into a nightmare state. The forest and everyone in it became twisted into an evil mockery of what they once were. Seeing the elven nation warping before their very eyes, the Dragonarmies quickly retreated out of the nation desperately trying to reach safety. For thirty long years, the refugees of Silvanesti would remain without a homeland. While the majority of the population lived at Silvamori on Southern Ergoth, the rest worked hard to try and destroy the nightmare that plagued their lands.";
    case REGION_SOLAMNIA: return "Solamnia is a human nation in northwestern Ansalon. Solamnia has just about every geographical feature: fertile plains, three mountain ranges (the Vingaard Mountains, Dargaard Mountains and the Garnet Mountains), the longest river, the Vingaard River, and the Northern Wastes. Not a perfectly flat land, has ridges, gullies, dry creek beds, small stands of trees, mostly a grassy steppe land. People from Solamnia are referred to as being Solamnic or Solamnian and they speak Solamnic or some of the Knights speak Old Solamnic. Slavery is outlawed in Solamnia, and if you are caught having slaves, the punishment is very severe. Solamnia's chief exports are grain and cattle. The provinces that make up Solamnia are Coastlund in the west, Palanthas east of that, Hinterlund on the border with Nightlund, Plains of Solamnia also on the border with Nightlund, Elkholm and Heartlund south of Plains of Solamnia, and Southlund southwest of Heartlund. In Southlund, in the area are Caergoth, the area is known for very violent thunderstorms. From Coastlund in the west, to Hinterlund in the east, live a variety of people—mostly humans, but small bands of ogres and draconians are far too common. Originally located in the center of the Ansalonian continent, Solamnia is now very much a sea-going state because the Cataclysm dumped three oceans around its borders. In the years following the Cataclysm, the commoners of Solamnia blame the Knights of Solamnia for the Cataclysm. Some Knights go into hiding while others throw down their sword and armor to leave the Knighthood forever. These are the dark ages for Solamnia and the Knighthood. In the year 334 AC, peasants led by Ragnell revolted against the nation, leading to the destruction of some major castles and death to a few prominent Solamnic families. In the year 351 AC, Dragonarmies invade eastern Solamnia, quickly overrunning the Knights of Solamnia there and pushing them all the way to the High Clerist's Tower by 352 AC. Solamnia is also betrayed and Dragonarmies are allowed to invade from the south, but thanks to Solamnia's allies of the Kayolin dwarves, the dwarves are able to hold the invaders until the Knights could regroup and hold the Dragonarmies in the south in a stalemate.";
    case REGION_SOUTHERN_ERGOTH: return "Southern Ergoth is an island located west of Ansalon and south of Northern Ergoth that was created when the Cataclysm tore Ergoth in two. Southern Ergoth stretches about 250 miles from north to south. The major geographical features of Southern Ergoth are the Sirrion Sea that surrounds the island, the Straits of Algoni that separate it from Ansalon, Morgash Lake which is just north of the capital city of Daltigoth, the Last Gaard Mountains running down the center of Southern Ergoth with Foghaven Vale in the mountains, Harkun Bay on the southern coastline, Plains of Tothen located in the southwestern coastline, Plains of Kri on the southern coastline, and the River Ergot located in the northeast. In the years following the Cataclysm, Southern Ergoth has had a close working relationship with Northern Ergoth, trying to restore the Empire. Southern Ergoth has been ruled from Daltigoth by regents appointed by the Redic Dynasty. The forests of Southern Ergoth became the homeland for the Kagonesti Elves. During the War of the Lance, the elves of Qualinesti and Silvanesti moved their people to Southern Ergoth to escape from the invading Dragonarmies. They enslaved the Kagonesti to build them cities while in exile. ";
    case REGION_TAMAN_BUSUK: return "Taman Busuk is the melting pot for all the 'evil' races of Ansalon. This region contains three of the most important dark cities: Sanction, Neraka, and Gargath. Taman Busuk borders the Estwilde and Kern in the north and Zhakar in the south. For the most part, Taman Busuk is a mountainous region, with the exception of the wastelands located in the south. The Khalkists are broken in few places, most notable Godshome and Gargath. As a state, Taman Busuk is very weak, but the cities within it are much more important.";
    case REGION_TARSIS: return "The small nation of Tarsis is centred around the great trade city of the same name. In the Pre-Cataclius Ansalon, Tarsis was a respected Lordcity of the realm, which was located in the forested region to the east of Kharolis and the west of the Silvanesti nation. The minor town and settlements around Tarsis all benefited with the large trade that the lordcity did with the other nations. However the Cataclysm resulted in the Lordcity of Tarsis becoming landlocked in the newly risen Plains of Dust. The nation once known as Tarsis ceased to exist at this point, and the city of Tarsis became little more than a dusty husk of it's former self that was considered part of this arid realm.";
    case REGION_TEYR: return "Teyr is a draconian nation located south of Nordmaar, north of Busuk Taman, and east of Estwilde. It is a mountainous region that has heavy forested lands, and some northern grasslands. The Astivar Mountains are in Teyr, while the Woods of Lahue are west of it. The Great Moors are to the east of it. Mount Brego is located west of the city of Teyr, in the Astivar Mountains, with the Peak of Destiny as the southernmost peak.";
    case REGION_THORBARDIN: return "Thorbardin is a dwarven nation located in southwestern Ansalon and is bordered by the nations of Qualinesti to the north, Plains of Dust to the east, and Kharolis to the south. The major geographical features of Thorbardin is the Kharolis Mountains which is most of the nation, the Plains of Dergoth which is a barren wasteland, the Urkhan Sea which is located inside of Thorbardin, and the Valley of Thanes. The Dwarven realm of Thorbardin not only refers to the kingdom under Cloudseeker Mountain, but also much of the land surrounding the Kharolis Mountains. During the War of the Lance in 351 AC, Thorbardin is besieged by Highlord Verminaard and his Red Dragonarmy at the northern gate. They are never able to gain access to Thorbardin. But a small band called the Companions leading the Refugees of Pax Tharkas are allowed entry into the kingdom for a time. During that time, the Companions find the Hammer of Kharas that had been thought lost since the Dwarfgate War many years ago. After finding it, they give it to Glade Hornfel Kytil who becomes the first King of Thorbardin since the Dwarfgate War.";
  }
  return "Undefined";
}
#else
const char *get_region_info(int region)
{
  switch (region)
  {
  case REGION_AMN:
    return "A nation led by the representatives of five noble families, Amn is a place where the wealthy rule, openly and without pretense. Shrewd traders and ruthless in business, Amnians believe that the end of a successful transaction is justified by any means, ethical or otherwise. Although the nation is richer by far than even the northern metropolises of Baldur's Gate and Waterdeep, its influence is curtailed by the unwillingness of its rulers to work together in the nation's best interest. The members of the Council of Five are fairly unified and tight-fisted in their control of Amn, but their ability to affect events outside their own borders is limited because they can't agree enough on major matters of foreign policy. The oligarchs utterly control their nation, but beyond the areas that each rules, their families and businesses compete with one another and with the locals of far-flung places.\r\n\r\n"
           "The use of arcane magic is illegal in Amn, meaning that the only authorized spellcasters in the nation are wielders of divine magic who enjoy the support and patronage of a temple, and users of arcane magic who have been given special dispensation by one of the oligarchs. So pervasive is the sway of Amn's oligarchy that few crimes merit physical punishment but those that involve the use of arcane magic or an offense against one of the council's merchant houses. Other infractions are forgiven after the miscreant makes payment of an appropriate fine.";
  case REGION_CALIMSHAN:
    return "This southern land has long been the battleground for warring genies. After years of struggling beneath their genasi masters, human slaves arose to follow a Chosen of Ilmater, at first using nonviolent resistance, and then erupting in full rebellion following his disappearance. They overthrew the genie lords of Calimport and Memnon, casting the remaining genies out of the cities and back to their elemental homes or into the depths of the deserts.\r\n\r\n"
           "Much of Calimshan is a chaotic place dominated by wealth, political influence, and personal power. Many pray for the return of the Chosen and the completion of his work. Others are learning to live together without genie masters, and to grudgingly accept the remaining genasi among them.";
  case REGION_CHULT:
    return "The vast, choking jungles of Chult hide what many believe to be great mineral wealth, including large gemstones and veins of ore. Poisonous flora and fauna riddle the jungles, but some still brave the dangers to seek their fortunes. Some of the exotic plants that grow only in Chult fetch high prices in mainland markets. Ruined Mezro stands across the sea from Calimshan, waiting for explorers and its displaced people to cleanse the city of its undead inhabitants and uncover the treasures that lie hidden there.\r\n\r\n"
           "Eastward along the Chultan peninsula lie the remains of Thindol and Samarach. Despite the apparent fall of both civilizations, Thindol remains infested with yuan-ti, while the illusions cloaking Samarach's mountain passes conceal the activities in that nation.";
  case REGION_DAMBRATH:
    return "Situated on a warm plain on the shore of the Great Sea, Dambrath is ruled by nomadic clans of human horse riders who revere Silvanus, Malar, and occasionally Selune. Given the Dambrathans' history of domination by the Crinti, a ruling caste of half-drow, it is no surprise that they reserve their greatest hatred for the drow.\r\n\r\n"
           "The clans meet twice a year at a sacred site known as the Hills of the Kings, where dozens of totem sculptures are preserved. At these gatherings, each clan updates its totem with an account of its exploits over the previous seasons. Many Dambrathans seek out lycanthropy as a means of showing reverence for their favored deity and honoring their heritage.";
  case REGION_ELFHARROW:
    return "A blasted near-desert north and east of the North Wall mountains bordering Halruaa, Elfharrow isn't a name bestowed by its residents, but rather the sobriquet that travelers use for this violent region. The tribes of xenophobic elves that claim this area don't hesitate to discourage uninvited guests by any means necessary. A simple group of pilgrims might be scared off with some arrows, while a band of hunters or explorers is likely to be killed outright.\r\n\r\n"
           "Food is sparse in this region, with the forests long since vanished, and as a result the elves of Elfharrow fiercely protect the herds of animals they have cultivated. The elves have no interest in looting the cities of fallen Lapaliiya, but neither are they willing to allow 'adventurers' free access to those lands through their territory.";
  case REGION_HALRUAA:
    return "Once believed destroyed in the conflagration of the Spellplague, Halruaa has largely been restored to the insular, magic-mighty nation it once was. Because of the foresight of their divinations, Halruaan wizards were able to use the raging blue fire that followed Mystra's death to propel their nation safely into the realm of Toril's twin, Abeir (displacing part of that world into the Plane of Shadow).\r\n\r\n"
           "Now that the events of those times have mostly been undone, the famed Halruaan skyships and waterborne vessels have spread out from their home once again, seeking to establish trading routes and political connections, as well as to learn what has changed of the world in their century of absence.";
  case REGION_THE_LAKE_OF_STEAM:
    return "Far to the south and east of the Sword Coast, the Lake of Steam is more accurately an inland sea, its waters tainted by volcanism and undrinkable. Around its perimeter is a conglomeration of city-states and minor baronies typified by the shifting domains known as the Border Kingdoms. Here, along the southern shore of the lake, explorers and fortune seekers squander their amassed wealth building castles, founding communities, and drawing loyal vassals to them - only to have all those good works disappear within a generation or two. In some cases, one of these realms is fortunate to be saved from its inevitable decline by another group of successful adventurers, who inject enough wealth and wisdom to keep the enterprise going a few more decades.";
  case REGION_LUIREN:
    return "Long the homeland of halflings and thought to be the place where their race had its genesis, Luiren was lost during the Spellplague to a great inundation of the sea. In the century since that great disaster, the waters receded, and now stories told by travelers from the south tell of halfling communities that survived as island redoubts.";
  case REGION_TETHYR:
    return "Tethyr is a feudal realm ruled by Queen Anais from its capital of Darromar. The queen commands her dukes, who in turn receive homage from the counts and countesses of the realm, appoint sheriffs over their counties, and generally maintain order. The farmlands of Tethyr are abundant, and its markets flow freely with trade from the Western Heartlands.\r\n\r\n"
           "Tethyr has seen more than its share of noble intrigue and royal murder, and adventurers who are native to Tethyr or merely passing through that land are often drawn into such plots, either as unwitting accomplices or as easy scapegoats.";
  case REGION_AGLAROND:
    return "The great peninsula of Aglarond juts out into the Inner Sea, and that body of water and the forests of the Yuirwood define much of the nation's character. A realm of humans living in harmony with their elf and half-elf neighbors, Aglarond has been a foe of Thay for centuries, in part due to the temperament of its former ruler, the Simbul. The nation is now ruled by a Simbarch Council, which has backed away from open hostilities with Thay. With the restoration of the Weave, the ongoing changes to the political landscape, and calls for elven independence within the nation, it is unclear what sort of place Aglarond will be in a generation's time, except that its potential for great change will be realized.";
  case REGION_CHESSENTA:
    return "A collection of city-states bound by common culture and mutual defense, Chessenta isn't truly a nation. Each city boasts its own heroes, worships its own gladiatorial champions, and spends as much time insulting and competing with the other cities as it does on any other activity. The city of Luthcheq is dominated by worship of the bizarre deity known as Entropy, while Erebos is ruled by the latest incarnation of the red dragon known as Tchazzar the Undying. Heptios contains the largest library in Chessenta, a center of learning where all nobles aspire to send their children for tutoring. That city is looked on with disdain by the people of Akanax, whose militant contempt for the 'fat philosophers' of Heptios is widely known. Toreus welcomes all visitors, even those from lands that are despised or mistrusted, and foreign coin can buy nearly anything there. The floating city of Airspur still flies somehow, its earthmotes unaffected by the fall of its fellows when the Sundering came to a close.";
  case REGION_CORMYR:
    return "For most folk in central Faerun, the notion of a human kingdom is inextricably linked to Cormyr. A strong realm bolstered by its loyal army (the Purple Dragons), a cadre of magical defenders and investigators (the War Wizards), and numerous wealthy and influential nobles, Cormyr is recovering from its war with Sembia and Netheril - a conflict that cost the nation much, but left the kingdom standing, and which, in the end, Netheril didn't survive. The pride of that victory remains strong in Cormyr's collective consciousness, even as Queen Raedra draws back from plans to permanently welcome into the realm towns that lie beyond Cormyr's traditional borders.\r\n"
           "Cormyreans are justly proud of their homeland, and go to great lengths to guard it and its honor. Still, there is no shortage of danger in the Forest Kingdom, whether from scheming, treacherous nobles, monsters out of the Hullack Forest or the Stonelands, or some ancient, hidden magic. Cormyr is many things, but dull isn't one of them.";
  case REGION_THE_COLD_LANDS:
    return "The nations of Damara, Narfell, Sossal, and Vaasa, known collectively to most Faerunians as the Cold Lands, rest near the Great Glacier in the cold, dry environs of the northeast. Few outside the region have much interest in what goes on here, except for those in the immediately surrounding lands, who fear a resurgence of the ancient evils of the region - though they aren't fearful enough to do more than send an adventuring party or two into the area to investigate.\r\n\r\n"
           "In Damara, the usurper King Yarin Frostmantle sits on the throne of the Dragonbane dynasty, while his people complain about his tyranny and the growing threat from demons across the country. In Narfell, skilled riders and archers hunt, raid, and are gradually reclaiming their heritage as a great nation of mages who treated with devils. The Warlock Knights of Vaasa threaten to break the bounds of their nation and invade Damara, the Moonsea, or both, while some of its members suspiciously eye the ominously silent Castle Perilous, perhaps planning another excursion to the place. The tiny nation of Sossal trades with its neighbors, but shares little of itself with the wider world.";
  case REGION_THE_DALELANDS:
    return "The humans who call the Dalelands home want nothing more than lives untroubled by the concerns of larger nations. They take great pride in their peaceful coexistence with the elves of Cormanthor, and in their ability to remain largely self-sufficient and autonomous even when their homeland was used as a battlefield by Cormyr, Netheril, Sembia, and Myth Drannor in the recent conflicts. Featherdale and Tasseldale have reasserted their independence since the end of the war, and rejoined Archendale, Battledale, Daggerdale, Deepingdale, Harrowdale, Mistledale, Scardale, and Shadowdale on the Dales Council. The High Dale did the same shortly afterward.\r\n\r\n"
           "Dalesfolk are mistrustful of anyone unwilling to sacrifice for the common good, but those who put in good work - whether in defense or labor - are accepted as equals, entitled to share in the rewards from their toil.";
  case REGION_THE_HORDELANDS:
    return "Formerly known as the Endless Wastes, this land has gained a new name among Faerunians, styled after the vast Tuigan horde that roared out of the east and rode against Faerun more than a century ago. After these tribesfolk were defeated, some of the fierce, mounted warriors who survived the conflict gathered to form the small nation of Yaïmunnahar. Some others cling to the old ways, mastering the sword and the bow and riding across the steppes on their short-legged horses. Brave merchants still traverse the Golden Way to and from Kara-Tur, but those who return from such a voyage are fewer than they once were.";
  case REGION_IMPILTUR:
    return "With the rising of the waters of the Sea of Fallen Stars, some of Impiltur's wealth and influence is returning, leading to whispers among the populace that a lost king of the line of old will rise up to lift Impiltur out of its woes and back to the great nation it once was.\r\n\r\n"
           "Impiltur is a nation of humans with pockets of dwarves and halflings among its populace. Where once a long royal line sat its throne and ruled over a unified kingdom, now a Grand Council sits around a table and struggles to combat the presence of demons, and demon worship, within the nation's borders.";
  case REGION_THE_MOONSEA:
    return "The shores of the Moonsea have long been home to cities that rise swiftly, relying on vigorous trade and gathering powerful mercenaries to their banners, only to overextend themselves and fall - sometimes crumbling over time, and sometimes dropping like stones from the sky.\r\n\r\n"
           "Now that Netheril and Myth Drannor have fallen, those two great powers can no longer exert their influence over the Moonsea, allowing the city of Hillsfar to spread its wings and eye southward expansion, and Mulmaster to once again further the worship of Bane. Phlan, Teshwave, Thentia, and Voonlar - all Moonsea cities where greater powers jockeyed for influence - now work to find their own identities before an unchecked or malevolent realm swallows them, one by one.\r\n\r\n"
           "This region is also home to the ruins of the Citadel of the Raven and Zhentil Keep, former strongholds of the Zhentarim, which the Black Network shows occasional interest in restoring.";
  case REGION_MULHORAND:
    return "Since the Chosen of the gods began to appear in the last few years, Mulhorand has become a land transformed. Its deities manifested fully in the forms of some of their descendants, and swiftly rallied the Mulan to overthrow the Imaskari. Aided by the mighty wizard Nezram, known as the World-Walker, the Mulhorandi overthrew the rulers of High Imaskar, who fled into the Plains of Purple Dust or to extraplanar safeholds.\r\n\r\n"
           "When the upheaval ended and the Chosen began to disappear, the gods of Mulhorand remained to rule their people, focusing their attention on defending their restored homeland to keep the war in Unther and Tymanther from spilling over its borders. For the first time in centuries, the people in Mulhorand are free, with the gods declaring that slavery shall no longer be practiced among the Mulan since their return.";
  case REGION_RASHEMEN:
    return "A harsh, cold land filled with hardy folk, Rashemen is a fiercely traditional nation. It is ruled by its Iron Lord, Mangan Uruk, who speaks for the power behind the throne: the Wychlaran, the society of masked witches that determine Rashemen's course. These witches wield great powers tied to the land and its magic and guard against evil fey and vengeful spirits. A small number of male spellcasters, known as the Old Ones, create magic items and weave arcane rituals for the witches. Rashemi witches revere the Three, a triumvirate of goddesses they call Bhalla (the Den Mother), Khelliara (the Forest Maiden), and the Hidden One. Over the centuries, scholars in other lands have speculated that these deities might be faces of Chauntea, Mielikki, and Mystra, respectively.\r\n\r\n"
           "The nation's warriors are a fierce, stoic lot, famed for their strength, endurance, and stubbornness in battle. Rashemen is a long-standing enemy of Thay, and has often thwarted that nation's ambitions to rule Faerun. Little pleases a Rashemi warrior more than the chance to strike down a Red Wizard in battle.";
  case REGION_SEMBIA:
    return "Following a period of subjugation at the hands of Netheril, Sembia is already on its way to becoming the economic power it was in prior years. Although relations are cool with the Dales and Cormyr following the most recent war, Sembian merchants are quick to dismiss previous conflicts as the work of the Netherese, and remind their former trading partners of the long and mutually profitable relationships they previously enjoyed. To prove its good intentions, Sembia has 'allowed' Featherdale and Tasseldale to regain their independence, even though Sembian investors had owned much of Featherdale for nearly seventy years when the war came to an end.\r\n\r\n"
           "Before Netheril claimed Sembia as a vassal state, mercenary work and adventuring were popular livelihoods among Sembians who didn't have local families to feed. Those endeavors are even more popular now among veterans of the war, who are better trained than their predecessors were. A few of Sembia's less scrupulous former soldiers have taken to banditry, which offers other Sembians more opportunities for guard work.";
  case REGION_THAY:
    return "For centuries one of the greatest concentrations of magical might in Faerun, Thay is ruled by the ancient lich, Szass Tam, and the nation's Council of Zulkirs in a ruthless magocracy. The council's will is enacted by regional tharchions and bureaucrats, leaving the ruling Red Wizards to focus on magical study and more important arcane matters.\r\n\r\n"
           "For a time, living mages couldn't hope to advance to prominence in Thay: Szass Tam promoted undeath as a means of existence with boundless possibilities, and held back those who didn't agree with this philosophy. The recent battles with the demon Eltab, however, have prompted Szass Tam to loosen this stricture - the living now have hope of ascending within the Red Wizards, even if that hope is merely to advance to a high station within the cadre of Tam's servants.";
  case REGION_THESK:
    return "Reminders of the century-old war with the Tuigan horde remain throughout Thesk, in the many and varied features of its present-day inhabitants, particularly the half-orc descendants of the mercenaries who fought in that great conflict.\r\n\r\n"
           "Thesk is known to many as the Gateway to the East because it is the western terminus of the Golden Way, which runs through the Hordelands and into Kara-Tur. Because their city is a crossroads of sorts between Faerun and the east, it should come as no surprise that Theskians don't judge outsiders quickly, and don't bristle at visitors who demonstrate strange quirks in speech or behavior. The people of Thesk trade readily with any folk, even nearby orcs and goblins that are willing to treat with them peacefully. They aren't fools, however, and have no patience for violent or raiding humanoids of all sorts.";
  case REGION_TURMISH:
    return "On the southern shore of the Sea of Fallen Stars, Turmish is a nation of mercantile cities ruled by its Assembly of Stars, representatives of each of its cities in a parliamentary democracy. After being much diminished by the devastation wrought in this area a century ago, Turmish is currently enjoying a revival of its fortunes, as the rising of the waters of the Inner Sea has returned some of the trade that was lost in the cataclysm. Turmish is the birthplace of the Emerald Enclave, which has proudly taken credit for the rebirth of Turmishan agriculture, the cessation of the great rains that plagued the region a few years ago, and the restoration of the god Lathander.";
  case REGION_TYMANTHER:
    return "In decades past, the land of the dragonborn claimed as its territory part of what had been the vanished nation of Unther. Then Unther suddenly returned to Faerun a few years ago and promptly went to war against Tymanther. The realm has since been reduced to small tracts mainly along the coast of the Alamber Sea and Ash Lake. The dragonborn that have withdrawn to those areas have lost none of their military tradition, and their ability to hold this smaller amount of territory makes it unlikely that Unther will push farther any time soon - particularly since the Untherite navy has been unable to overcome the great beast that guards the harbor of Djerad Kethendi and the nearby waters of the Alamber.\r\n\r\n"
           "Some of Tymanther's dragonborn have spread across Faerun and gained reputations as competent, highly sought-after mercenaries.";
  case REGION_UNTHER:
    return "Trapped in another world, the people of Unther had succumbed to domination by others. Then among them arose one who called himself Gilgeam, and he reminded them of their former greatness. Under the leadership of this reincarnated god, the people of Unther rose up as an army to face their masters. On the eve of a great battle, the people of Unther were miraculously returned to their home, and Gilgeam wasted no time in leading them against the dragonborn occupying their ancestral lands. The Untherites have retaken much of the land they formerly held, while seeking to wipe out the 'godless lizards' they blame for their time of oppression in Abeir.\r\n\r\n"
           "Gilgeam wants nothing short of a complete return to Unther's former glory. This achievement will require utterly destroying Tymanther, of course, and eventual war with Mulhorand to reclaim lands lost centuries ago, but as every Untherite knows, the great God-King is patient, for he is eternal.";
  case REGION_WESTGATE:
    return "The dismal city of Westgate isn't a romantic place, but someone seeking employment for shady work, or looking to hire someone for the same, will find few places better suited in all of Faerun.\r\n\r\n"
           "Westgate is considered by some Faerunians as a harbinger of the eventual fate of places like Amn and Sembia, where coin rules over all other considerations. As in many such places, one's moral outlook is less important in Westgate than one's attitude toward bribery. The city's proximity to Cormyr makes it a breeding ground for that nation's enemies, including the Fire Knives, a guild of thieves and assassins that the naive pretend doesn't exist.";
  case REGION_KARA_TUR:
    return "Far to the east, past the wastes of the Hordelands, lie the empires of Shou Lung, Kozakura, Wa, and the other lands of the vast continent of Kara-Tur. To most people of Faerun, Kara-Tur is like another world, and the tales told by travelers from its nations seem to confirm it. The gods that humans worship in Faerun are unknown there, as are common peoples such as gnomes and orcs. Other dragons, neither chromatic nor metallic, dwell in its lands and fly its skies. And its mages practice forms of magic mysterious even to archwizards of Faerun.\r\n\r\n"
           "Stories of Kara-Tur tell of gold and jade in great abundance, rich spices, silks, and other goods rare or unknown in western lands - alongside tales of shapechanging spirit-people, horned giants, and nightmare monsters absent in Faerun.";
  case REGION_ZAKHARA:
    return "Far to the south of Faerun, beyond Calimshan and even the jungles of Chult, are the Lands of Fate. Surrounded by waters thick with pirates and corsairs, Zakhara is a place less hospitable than most, but still braved by travelers who hope to profit from its exotic goods and strange magics. Like Kara-Tur, Zakhara seems a world away to Faerunians. It is thought of as a vast desert, sprinkled with glittering cities like scattered gems. Romantic tales abound of scimitar-wielding rogues riding flying carpets and of genies bound in service to humans. Their mages, called sha'ir, practice their magic with the aid of genies and, it is said, might carry the lineage of these elemental beings in their blood.\r\n";
  case REGION_ICEWIND_DALE:
    return "Icewind Dale is an arctic tundra located in the Frozenfar region of the North, known for being the northernmost explored region in all of Faerun. It earned its name from the harsh winds and icy storms that destroyed buildings and scoured the landscape.\r\n\r\n"
           "The dale is a harsh, near-uninhabitable land that regularly plunges below freezing temperatures, and receives little sunlight, particularly during the severe winter months. It is home to only the most hardened of frontiersmen, pioneers, and barbarians. Beyond the sporadic dots of civilization dwelled terrifying beasts and deadly monsters of the North.";
  case REGION_THE_SWORD_COAST:
    return "The Sword Coast is a region on the northwestern coast of Faerun. While it comprises a rough landscape of rugged hills, precarious mountain ranges and dense forests, it is home to several like-minded towns and cities. The most prominent of these cities banded together with nearby allies to form the Lords' Alliance, which unites much of the region.\r\n\r\n"
           "This vast region of the North stretches all the way from the Spine of the World in the north, south to the great metropolis of Waterdeep. It is bordered on the west by the coastland of the Sea of Swords and along the east by the Long Road.";
  case REGION_LURUAR:
    return "Luruar, also commonly known as the Silver Marches, is a confederation of cities in the north of Faerun, under the leadership of Alustriel Silverhand, former ruler of Silverymoon. It consists of Silverymoon, Citadel Adbar, Deadsnows, Jalanthar, Quaervarr, Citadel Felbarr, Everlund, Mithral Hall, and Sundabar, and its goal is to protect the North against the growing horde of orcs in the mountains. The nation is also a member of the Lords' Alliance.\r\n"
           "It was bordered by the Anauroch desert to the east, the High Forest to the south, the Savage Frontier to the west and the Spine of the World mountain range to the north. It also shares a border with the kingdom of Many-Arrows.";
  case REGION_EVERMEET:
    return "The island nation Evermeet, occasionally known as the Green Isle, is the last true kingdom of, and the final destination for all non-drow Tel'Quessir ('elves') on Faerun. Very few non-elves have ever been permitted to visit.\r\n"
           "Much of Evermeet's architecture is created by magical means, most notably by the use of the spell construction.\r\n\r\n"
           "The north of the island is made up of rugged terrain with steep headlands and covered in dark pine forest.\r\n\r\n"
           "The eastern shore is heavily forested with oaks and evergreen right up to the calm, deep blue waters of the sea.";
  case REGION_THE_SAVAGE_FRONTIER:
    return "The Savage Frontier is the region of northwest Faerun north of the Delimbiyr River, excluding the Sword Coast North, the High Forest and the nation of Luruar, also known as the Silver Marches. In contrast to the civilized cities found to the south and west, the Savage Frontier comprises rural farmsteads and rough settlements of miners and loggers.\r\n\r\n"
           "It is a temperate land, with rugged landscape that is rich with natural resources.";
  case REGION_ANAUROCH_DESERT:
    return "Anauroch, or The Great Sand Sea, is a magical desert in northern Faerun. It holds the remnants of the once-powerful Netherese Empire, their flying enclaves having crashed to the ground when their greatest mage Karsus, in a desperate bid to end the war against the phaerimm, challenged the goddess Mystryl for her divine mantle, causing the Weave to falter and all magic to fail. For generations since, Anauroch, the greatest desert in Faerun, encroached relentlessly on border nations, burying them beneath the sands.\r\n\r\n"
           "The returned masters of Anauroch, the ancient Netherese wizards of Thultanthar, warped and twisted by their long exile in the Plane of Shadow, were determined to retake what they considered their birthright, longing to restore the barren wasteland that was Anauroch to the once-fertile land of Netheril. The Shadovar had a regimented society, ruled by Telamont Tanthul and his Princes of Shade, all working in unison, toughened by centuries of hardship in the Plane of Shadow, to accomplish their common goal. They all but ignored the Bedine and Zhentarim, considering them beneath their notice.";
  case REGION_THE_UNDERDARK:
    return "The Underdark is the vast network of underground caverns and tunnels underneath the surface of Toril. It is home to a host of evil beings driven deep into the caverns at the end of the age of demons.\r\n\r\n"
           "The Underdark is not one giant cavern under Faerun, but rather, many huge networks of caverns and caves. As a result, it is not always possible to travel from one end of the Underdark to the other. The Underdark is divided into several domains that were similar to continents of the world above. While it is possible to travel from one place to another within a domain, separate domains tend to have very few passages linking them. The major domains of the Underdark are the Buried Realms, the Darklands, the Deep Wastes, the Earthroot, the Glimmersea, Great Bhaerynden, the Northdark, and Old Shanatar.";
  }
  return "Undefined";
}
#endif
#if defined(CAMPAIGN_DL)

int get_region_language(int region)
{
  switch (region)
  {
    case REGION_NONE: return LANG_COMMON;
    case REGION_ABANASINIA: return LANG_ABANASINIAN;
    case REGION_BALIFOR: return LANG_KHUR;
    case REGION_BLODE: return LANG_OGRE;
    case REGION_BLOOD_SEA_ISLES: return LANG_KALINESE;
    case REGION_ENSTAR: return LANG_ERGOT;
    case REGION_ESTWILDE: return LANG_SOLAMNIC;
    case REGION_GOODLUND: return LANG_KENDER;
    case REGION_HYLO: return LANG_KENDER;
    case REGION_KAYOLIN: return LANG_DWARVEN;
    case REGION_KHUR : return LANG_KHUR;
    case REGION_LEMISH : return LANG_SOLAMNIC;
    case REGION_NIGHTLUND : return LANG_SOLAMNIC;
    case REGION_NORDMAAR : return LANG_NORDMAARIAN;
    case REGION_NORTHERN_ERGOTH : return LANG_ERGOT;
    case REGION_NOSTAR : return LANG_ERGOT;
    case REGION_QUALINESTI : return LANG_ELVEN;
    case REGION_SANCRIST_ISLE : return LANG_SOLAMNIC;
    case REGION_SCHALLSEA : return LANG_ABANASINIAN;
    case REGION_SILVANESTI : return LANG_ELVEN;
    case REGION_SOLAMNIA : return LANG_SOLAMNIC;
    case REGION_SOUTHERN_ERGOTH : return LANG_ERGOT;
    case REGION_TAMAN_BUSUK : return LANG_NERAKESE;
    case REGION_TARSIS : return LANG_KHAROLIAN;
    case REGION_TEYR : return LANG_DRACONIC;
    case REGION_THORBARDIN: return LANG_DWARVEN;
  }
  return LANG_COMMON;
}
#else

int get_region_language(int region)
{
  switch (region)
  {
  case REGION_AGLAROND:
  case REGION_WESTGATE:
    return SKILL_LANG_AGLARONDAN;

  case REGION_AMN:
    return SKILL_LANG_THORASS;

  case REGION_ANAUROCH_DESERT:
    return SKILL_LANG_NETHERESE;

  case REGION_CALIMSHAN:
  case REGION_TETHYR:
    return SKILL_LANG_ALZHEDO;

  case REGION_CHESSENTA:
  case REGION_THAY:
  case REGION_MULHORAND:
    return SKILL_LANG_MULAN;

  case REGION_CHULT:
    return SKILL_LANG_CHULTAN;

  case REGION_CORMYR:
  case REGION_LURUAR:
  case REGION_SEMBIA:
  case REGION_THE_DALELANDS:
    return SKILL_LANG_CHONDATHAN;

  case REGION_DAMBRATH:
    return SKILL_LANG_DAMBRATHAN;

  case REGION_ELFHARROW:
  case REGION_EVERMEET:
    return SKILL_LANG_ELVEN;

  case REGION_HALRUAA:
    return SKILL_LANG_HALRUAAN;

  case REGION_ICEWIND_DALE:
  case REGION_THE_SWORD_COAST:
  case REGION_THE_SAVAGE_FRONTIER:
    return SKILL_LANG_ILLUSKAN;

  case REGION_IMPILTUR:
  case REGION_THESK:
  case REGION_THE_COLD_LANDS:
  case REGION_THE_MOONSEA:
    return SKILL_LANG_DAMARAN;

  case REGION_KARA_TUR:
    return SKILL_LANG_SHOU;

  case REGION_LUIREN:
    return SKILL_LANG_HALFLING;

  case REGION_RASHEMEN:
    return SKILL_LANG_RASHEMI;

  case REGION_THE_UNDERDARK:
    return SKILL_LANG_UNDERCOMMON;

  case REGION_THE_HORDELANDS:
    return SKILL_LANG_GURAN;

  case REGION_THE_LAKE_OF_STEAM:
  case REGION_TURMISH:
  case REGION_UNTHER:
    return SKILL_LANG_SHAARAN;

  case REGION_TYMANTHER:
    return SKILL_LANG_DRACONIC;

  case REGION_ZAKHARA:
    return SKILL_LANG_MIDANI;

  default:
    return SKILL_LANG_COMMON;
  }
  return SKILL_LANG_COMMON;
}
#endif

bool is_furry(int race)
{
  switch (race)
  {
    case RACE_TABAXI:
    case DL_RACE_MINOTAUR:
      return true;
  }
  return false;
}

bool has_horns(int race)
{
  switch (race)
  {
    case RACE_TIEFLING:
    case DL_RACE_MINOTAUR:
      return true;
  }
  return false;
}

bool has_scales(int race)
{
  switch (race)
  {
    case RACE_DRAGONBORN:
    case DL_RACE_AURAK_DRACONIAN:
    case DL_RACE_BAAZ_DRACONIAN:
    case DL_RACE_BOZAK_DRACONIAN:
    case DL_RACE_KAPAK_DRACONIAN:
    case DL_RACE_SIVAK_DRACONIAN:
      return true;
  }
  return false;
}

bool race_has_no_hair(int race)
{
  switch (race)
  {
    case RACE_DRAGONBORN:
    case DL_RACE_AURAK_DRACONIAN:
    case DL_RACE_BAAZ_DRACONIAN:
    case DL_RACE_BOZAK_DRACONIAN:
    case DL_RACE_KAPAK_DRACONIAN:
    case DL_RACE_SIVAK_DRACONIAN:
      return true;
  }
  return false;
}

int compare_races(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(race_list[a].name, race_list[b].name);
}

/* sort feats called at boot up */
void sort_races(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 0; a < NUM_EXTENDED_RACES; a++)
    race_sort_info[a] = a;

  qsort(&race_sort_info[0], NUM_EXTENDED_RACES, sizeof(int), compare_races);

}

ACMD(do_listraces)
{
  int i, sortpos;

  send_to_char(ch, "\r\n");
  draw_line(ch, 80, '-', '-');
  text_line(ch, "Races of Krynn", 80, '-', '-');
  draw_line(ch, 80, '-', '-');
  for (sortpos = 0; sortpos < NUM_EXTENDED_RACES; sortpos++)
  {
    i = race_sort_info[sortpos];
    if (race_list[i].is_pc)
    {
      send_to_char(ch, " %4.4s - %s\r\n", race_list[i].abbrev, race_list[i].type);
    }
  }
  send_to_char(ch, "\r\n");
  draw_line(ch, 80, '-', '-');
}

// awards a random food item.
// result is the result of a skill check vs. a dc
// type determines the output message given
// 1 is forage, 2 is scrounge
void award_random_food_item(struct char_data *ch, int result, int type)
{

  bool food = false;
  char ripeness[50], food_desc[150];
  struct obj_data *obj;
  int modifier = 2, bonus = 0;

  result /= 10;

  switch (result)
  {
    case 0: modifier = 2; snprintf(ripeness, sizeof(ripeness), "rather unripe"); break;
    case 1: modifier = 3; snprintf(ripeness, sizeof(ripeness), "barely ripe"); break;
    case 2: modifier = 4; snprintf(ripeness, sizeof(ripeness), "nicely ripened"); break;
    case 3: modifier = 5; snprintf(ripeness, sizeof(ripeness), "very well ripened"); break;
    default: modifier = 6; snprintf(ripeness, sizeof(ripeness), "perfectly ripened"); break;
  }

  switch (rand_number(0, 18))
  {
    case 0: bonus = APPLY_AC_NEW; break;
    case 1: bonus = APPLY_STR; break;
    case 2: bonus = APPLY_DEX; break;
    case 3: bonus = APPLY_CON; break;
    case 4: bonus = APPLY_INT; break;
    case 5: bonus = APPLY_WIS; break;
    case 6: bonus = APPLY_CHA; break;
    case 7: bonus = APPLY_DAMROLL; modifier /= 2; break;
    case 8: bonus = APPLY_HITROLL; modifier /= 2; break;
    case 9: bonus = APPLY_ENCUMBRANCE; break;
    case 10: bonus = APPLY_HIT; modifier *= 10; break;
    case 11: bonus = APPLY_MOVE; modifier *= 100; break;
    case 12: bonus = APPLY_HP_REGEN; break;
    case 13: bonus = APPLY_MV_REGEN; break;
    case 14: bonus = APPLY_PSP; modifier *= 5; break;
    case 15: bonus = APPLY_PSP_REGEN; break;
    case 16: bonus = APPLY_SAVING_FORT; break;
    case 17: bonus = APPLY_SAVING_REFL; break;
    case 18: bonus = APPLY_SAVING_WILL; break;
  }

  obj = read_object(FORAGE_FOOD_ITEM_VNUM, VIRTUAL);

  if (!obj)
  {
    send_to_char(ch, "The forage food item prototype was not found. Please inform a staff with code ERRFOR00%d.\r\n", type);
    return;
  }

  GET_FORAGE_COOLDOWN(ch) = 100;
  
  food = apply_type_food_or_drink[bonus];
  
  if (!food)
    GET_OBJ_TYPE(obj) = ITEM_DRINK;

  obj->affected[0].location = bonus;
  obj->affected[0].modifier = modifier;
  obj->affected[0].bonus_type = (food) ? BONUS_TYPE_FOOD : BONUS_TYPE_DRINK;
  
  snprintf(food_desc, sizeof(food_desc), "some %s %s", ripeness, apply_type_food_names[bonus]);
  obj->name = strdup(food_desc);
  obj->short_description = strdup(food_desc);
  snprintf(food_desc, sizeof(food_desc), "Some %s %s lie here.", ripeness, apply_type_food_names[bonus]);
  obj->description = strdup(food_desc);

  obj_to_char(obj, ch);

  if (type == 1)
  {
    act("You forage for food and find $p!", TRUE, ch, obj, 0, TO_CHAR);
    act("$n forages for food and finds $p!", TRUE, ch, obj, 0, TO_ROOM);
  }
  else if (type == 2)
  {
    act("You scrounge for supplies and find $p!", TRUE, ch, obj, 0, TO_CHAR);
    act("$n scrounges for supplies and finds $p!", TRUE, ch, obj, 0, TO_ROOM);
  }
}

ACMD(do_scrounge)
{

  int skill, dc, result, roll, scrounge, grade, amount;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot scrounge.\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_SURVIVAL_INSTINCT))
  {
    send_to_char(ch, "You don't know how to scrounge.\r\n");
    return;
  }

  if (GET_SCROUNGE_COOLDOWN(ch) > 0)
  {
    send_to_char(ch, "You've recently scrounged for supplies and will have to wait.\r\n");
    return;
  }

  skill = compute_ability(ch, ABILITY_NATURE);
  dc = 15;
  roll = d20(ch);
  result = roll + skill - dc;

  send_to_char(ch, "You attempt to scrounge for supplies... Roll %d + %d (nature skill) for total %d vs. dc %d\r\n",
                  roll, skill, roll + skill, dc);

  if (result < 0)
  {
    send_to_char(ch, "You fail to find anything of use.\r\n");
    GET_SCROUNGE_COOLDOWN(ch) = 100;
    return;
  }

  grade = result / 10;

  grade = MAX(GRADE_TYPICAL, grade);
  grade = MIN(GRADE_SUPERIOR, grade);

  scrounge = dice(1, 20);

  act("You scrounge the area for useful supplies.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n scrounges the area for useful supplies.", TRUE, ch, 0, 0, TO_ROOM);

  ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_SCROUNGE;
  switch (scrounge)
  {
    case 1: // weapon
      award_magic_weapon(ch, grade);
      break;
    case 2: // armor
      switch (dice(1, 5))
      {
        case 1: award_magic_armor(ch, grade, ITEM_WEAR_BODY); break;
        case 2: award_magic_armor(ch, grade, ITEM_WEAR_ARMS); break;
        case 3: award_magic_armor(ch, grade, ITEM_WEAR_LEGS); break;
        case 4: award_magic_armor(ch, grade, ITEM_WEAR_HEAD); break;
        case 5: award_magic_armor(ch, grade, ITEM_WEAR_SHIELD); break;
      }
      break;
    case 3: // misc
      award_misc_magic_item(ch, dice(1, 9), grade);
      break;
    case 4: // consumable
    case 5: // consumable
    case 6: // consumable
      award_expendable_item(ch, grade, dice(1, 2));
      break;
    case 7: // food/drink
    case 8: // food/drink
    case 9: // food/drink
      award_random_food_item(ch, result, 2);
      break;
    case 10: // gold
    case 11: // gold
    case 12: // gold
      amount = award_random_money(ch, result);
      send_to_char(ch, "You find a pouch containing %d gold coins.\r\n", amount);
      break;
    default: // nada
      send_to_char(ch, "Your scrounging uncovered nothing of use.\r\n");
      break;
  }
  ch->char_specials.which_treasure_message = 0;  
}

/*
int get_size(struct char_data *ch) {
  int racenum;

  if (ch == NULL)
    return SIZE_MEDIUM;

  racenum = GET_RACE(ch);

  if (racenum < 0 || racenum >= NUM_EXTENDED_RACES)
    return SIZE_MEDIUM;

  return (GET_SIZE(ch) = ((affected_by_spell(ch, SPELL_ENLARGE_PERSON) ? 1 : 0) + race_list[racenum].size));
}
 */

/* clear up local defines */
#undef Y
#undef N
#undef IS_NORMAL
#undef IS_ADVANCE
#undef IS_EPIC_R

/*EOF*/
