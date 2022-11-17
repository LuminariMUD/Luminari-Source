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

/* defines */
#define Y TRUE
#define N FALSE
/* racial classification for PC races */
#define IS_NORMAL 0
#define IS_ADVANCE 1
#define IS_EPIC_R 2

/* some pre setup here */
struct race_data race_list[NUM_EXTENDED_RACES];

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
    race_list[i].name = NULL;
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
  race_list[RACE_HUMAN].racial_language = LANG_COMMON;
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
  race_list[RACE_HIGH_ELF].racial_language = LANG_ELVISH;

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
  race_list[RACE_WOOD_ELF].racial_language = LANG_ELVISH;
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
  race_list[RACE_MOON_ELF].racial_language = LANG_ELVISH;
  /* affect assignment */
  /*                  race-num  affect            lvl */

  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALF_ELF, "halfelf", "HalfElf", "\tMHalf Elf\tn", "HElf", "\tMHElf\tn",
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
  race_list[RACE_HALF_ELF].racial_language = LANG_ELVISH;
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
  race_list[RACE_HALF_DROW].racial_language = LANG_UNDERCOMMON;
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
  race_list[RACE_DRAGONBORN].racial_language = LANG_DRACONIC;
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
  race_list[RACE_TIEFLING].racial_language = LANG_ABYSSAL;

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
  race_list[RACE_AASIMAR].racial_language = LANG_CELESTIAL;

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
  race_list[RACE_SHIELD_DWARF].racial_language = LANG_DWARVISH;
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
  race_list[RACE_GOLD_DWARF].racial_language = LANG_DWARVISH;
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
  race_list[RACE_LIGHTFOOT_HALFLING].racial_language = LANG_HALFLING;
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
  race_list[RACE_STOUT_HALFLING].racial_language = LANG_HALFLING;
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
  race_list[RACE_FOREST_GNOME].racial_language = LANG_GNOMISH;
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
  race_list[RACE_ROCK_GNOME].racial_language = LANG_GNOMISH;
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
  race_list[RACE_HALF_ORC].racial_language = LANG_ORC;
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
  race_list[RACE_SHADE].racial_language = LANG_COMMON;
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
  race_list[RACE_GOLIATH].racial_language = LANG_GIANT;
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
  race_list[RACE_DROW_ELF].racial_language = LANG_UNDERCOMMON;
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
  race_list[RACE_HIGH_ELF].racial_language = LANG_ELVISH;

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
  add_race(RACE_HALF_ELF, "halfelf", "HalfElf", "\twHalf \tYElf\tn", "HElf", "\twH\tYElf\tn",
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
  race_list[RACE_DRAGONBORN].racial_language = LANG_DRACONIC;
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
  race_list[RACE_TIEFLING].racial_language = LANG_ABYSSAL;
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
  race_list[RACE_AASIMAR].racial_language = LANG_CELESTIAL;

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
  race_list[RACE_STOUT_HALFLING].racial_language = LANG_HALFLING;
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
  race_list[RACE_FOREST_GNOME].racial_language = LANG_GNOMISH;

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
  race_list[RACE_SHADE].racial_language = LANG_COMMON;
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
  race_list[RACE_GOLIATH].racial_language = LANG_GIANT;
  /* affect assignment */
  /*                  race-num  affect            lvl */

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
  race_list[RACE_FAE].racial_language = LANG_ELVEN;
  /* affect assignment */
  /*                  race-num  affect            lvl */

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

int get_region_language(int region)
{
  switch (region)
  {
  case REGION_AGLAROND:
  case REGION_WESTGATE:
    return LANG_AGLARONDAN;

  case REGION_AMN:
    return LANG_THORASS;

  case REGION_ANAUROCH_DESERT:
    return LANG_NETHERESE;

  case REGION_CALIMSHAN:
  case REGION_TETHYR:
    return LANG_ALZHEDO;

  case REGION_CHESSENTA:
  case REGION_THAY:
  case REGION_MULHORAND:
    return LANG_MULAN;

  case REGION_CHULT:
    return LANG_CHULTAN;

  case REGION_CORMYR:
  case REGION_LURUAR:
  case REGION_SEMBIA:
  case REGION_THE_DALELANDS:
    return LANG_CHONDATHAN;

  case REGION_DAMBRATH:
    return LANG_DAMBRATHAN;

  case REGION_ELFHARROW:
  case REGION_EVERMEET:
    return LANG_ELVISH;

  case REGION_HALRUAA:
    return LANG_HALRUAAN;

  case REGION_ICEWIND_DALE:
  case REGION_THE_SWORD_COAST:
  case REGION_THE_SAVAGE_FRONTIER:
    return LANG_ILLUSKAN;

  case REGION_IMPILTUR:
  case REGION_THESK:
  case REGION_THE_COLD_LANDS:
  case REGION_THE_MOONSEA:
    return LANG_DAMARAN;

  case REGION_KARA_TUR:
    return LANG_SHOU;

  case REGION_LUIREN:
    return LANG_HALFLING;

  case REGION_RASHEMEN:
    return LANG_RASHEMI;

  case REGION_THE_UNDERDARK:
    return LANG_UNDERCOMMON;

  case REGION_THE_HORDELANDS:
    return LANG_GURAN;

  case REGION_THE_LAKE_OF_STEAM:
  case REGION_TURMISH:
  case REGION_UNTHER:
    return LANG_SHAARAN;

  case REGION_TYMANTHER:
    return LANG_DRACONIC;

  case REGION_ZAKHARA:
    return LANG_MIDANI;

  default:
    return LANG_COMMON;
  }
  return LANG_COMMON;
}

bool is_furry(int race)
{
  return (race == RACE_TABAXI);
}

bool has_horns(int race)
{
  return (race == RACE_TIEFLING);
}

bool has_scales(int race)
{
  return (race == RACE_DRAGONBORN);
}

bool race_has_no_hair(int race)
{
  return (race == RACE_DRAGONBORN);
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
