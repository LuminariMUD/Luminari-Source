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
    race_list[i].unlock_cost = 99999;
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
  send_to_char(ch, strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

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
ACMD(do_race)
{
  char arg[80];
  char arg2[80];
  const char *racename;

  /*  Have to process arguments like this
   *  because of the syntax - race info <racename> */
  racename = one_argument(argument, arg, sizeof(arg));
  one_argument(racename, arg2, sizeof(arg2));

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
  add_race(RACE_ELF, "elf", "Elf", "\tYElf\tn", "Elf ", "\tYElf \tn",
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
                               "the local environment.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Elven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Elven.");
  set_race_genders(RACE_ELF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_ELF, 0, -2, 0, 0, 2, 0);          /* str con int wis dex cha */
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
  set_race_abilities(RACE_HALF_ELF, 0, 0, 0, 0, 0, 0);           /* str con int wis dex cha */
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
  feat_race_assignment(RACE_HALF_ELF, FEAT_KEEN_SENSES, 1, N);
  feat_race_assignment(RACE_HALF_ELF, FEAT_RESISTANCE_TO_ENCHANTMENTS, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_DWARF, "dwarf", "Dwarf", "\tgDwarf\tn", "Dwrf", "\tgDwrf\tn",
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
                               "In addition Dwarves gain proficiency with Dwarven War Axes.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Dwarven.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Dwarven.");
  set_race_genders(RACE_DWARF, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_DWARF, 0, 2, 0, 0, 0, -2);          /* str con int wis dex cha */
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
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HALFLING, "halfling", "Halfling", "\tcHalfling\tn", "Hflg", "\tcHflg\tn",
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
                               "can also help shield them from terrors that might immobilize their allies.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Halfling.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Halfling.");
  set_race_genders(RACE_HALFLING, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_HALFLING, -2, 0, 0, 0, 2, 0);          /* str con int wis dex cha */
  set_race_alignments(RACE_HALFLING, Y, Y, Y, Y, Y, Y, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_HALFLING,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, Y, N, N, N, N, N, N, N, N, N, N);
  /* feat assignment */
  /*                   race-num    feat                  lvl stack */
  feat_race_assignment(RACE_HALFLING, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_HALFLING, FEAT_SHADOW_HOPPER, 1, N);
  feat_race_assignment(RACE_HALFLING, FEAT_LUCKY, 1, N);
  feat_race_assignment(RACE_HALFLING, FEAT_COMBAT_TRAINING_VS_GIANTS, 1, N);
  feat_race_assignment(RACE_HALFLING, FEAT_HALFLING_RACIAL_ADJUSTMENT, 1, N);
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/
  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_GNOME, "gnome", "Gnome", "\tMGnome\tn", "Gnme", "\tMGnme\tn",
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
                               "their oft-capricious natures, and their outlooks on life and the world.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes Gnomish.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes Gnomish.");
  set_race_genders(RACE_GNOME, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_GNOME, -2, 2, 0, 0, 0, 0);          /* str con int wis dex cha */
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
  set_race_abilities(RACE_HALF_ORC, 2, 0, -2, 0, 0, -2);         /* str con int wis dex cha */
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
  set_race_abilities(RACE_HALF_TROLL, 2, 2, -4, -4, 2, -4);        /* str con int wis dex cha */
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
  set_race_abilities(RACE_ARCANA_GOLEM, -2, -2, 2, 2, 0, 2);         /* str con int wis dex cha */
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
  set_race_abilities(RACE_DROW, 0, -2, 2, 2, 2, 2);          /* str con int wis dex cha */
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
  set_race_abilities(RACE_DUERGAR, 0, 4, 0, 0, 0, -2);          /* str con int wis dex cha */
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
  set_race_abilities(RACE_CRYSTAL_DWARF, 2, 4, 0, 2, 0, 2);           /* str con int wis dex cha */
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
  set_race_abilities(RACE_TRELUX, 2, 4, 0, 0, 4, 0);           /* str con int wis dex cha */
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
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LICH, "lich", "Lich", "\tLLich\tn", "Lich", "\tLLich\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 999999999, IS_EPIC_R);
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
                               "Please note that a Lich has all the advantages/disadvantages of being Undead.",
                   /*morph to-char*/ "Your body twists and contorts painfully until your form becomes a Lich.",
                   /*morph to-room*/ "$n's body twists and contorts painfully until $s form becomes a Lich.");
  set_race_genders(RACE_LICH, N, Y, Y);                      /* n m f */
  set_race_abilities(RACE_LICH, 0, 2, 6, 0, 2, 0);           /* str con int wis dex cha */
  set_race_alignments(RACE_LICH, N, N, N, N, N, N, Y, Y, Y); /* law-good -> cha-evil */
  set_race_attack_types(RACE_LICH,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        Y, N, N, N, N, N, N, N, N, N, Y, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
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

  /*
  add_race(RACE_ROCK_GNOME, "rock gnome", "RkGnome", "Rock Gnome", RACE_TYPE_HUMANOID, N, Y, Y, -2, 0, 2, 0, 0, 0,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_WIZARD, SKILL_LANG_GNOME, 0);
  add_race(RACE_DEEP_GNOME, "svirfneblin", "Svfnbln", "Svirfneblin", RACE_TYPE_HUMANOID, N, Y, Y, -2, 0, 0, 2, 2, -4,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_SMALL, FALSE, CLASS_ROGUE, SKILL_LANG_GNOME, 3);
  add_race(RACE_ORC, "orc", "Orc", "Orc", RACE_TYPE_HUMANOID, N, Y, Y, 2, 2, -2, -2, 0, -2,
          Y, Y, Y, Y, Y, Y, Y, Y, Y, SIZE_MEDIUM, FALSE, CLASS_BERSERKER, SKILL_LANG_ORCISH, 0);
  */

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
  if (is_abbrev(arg, "elf"))
    return RACE_ELF;
  if (is_abbrev(arg, "darkelf"))
    return RACE_DROW;
  if (is_abbrev(arg, "dark-elf"))
    return RACE_DROW;
  if (is_abbrev(arg, "drow"))
    return RACE_DROW;
  if (is_abbrev(arg, "dwarf"))
    return RACE_DWARF;
  if (is_abbrev(arg, "duergar"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "graydwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "darkdwarf"))
    return RACE_DUERGAR;
  if (is_abbrev(arg, "half-troll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halftroll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halfling"))
    return RACE_HALFLING;
  if (is_abbrev(arg, "halfelf"))
    return RACE_H_ELF;
  if (is_abbrev(arg, "half-elf"))
    return RACE_H_ELF;
  if (is_abbrev(arg, "halforc"))
    return RACE_H_ORC;
  if (is_abbrev(arg, "half-orc"))
    return RACE_H_ORC;
  if (is_abbrev(arg, "gnome"))
    return RACE_GNOME;
  if (is_abbrev(arg, "arcanagolem"))
    return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "arcana-golem"))
    return RACE_ARCANA_GOLEM;
  if (is_abbrev(arg, "trelux"))
    return RACE_TRELUX;
  if (is_abbrev(arg, "lich"))
    return RACE_LICH;
  if (is_abbrev(arg, "crystaldwarf"))
    return RACE_CRYSTAL_DWARF;
  if (is_abbrev(arg, "crystal-dwarf"))
    return RACE_CRYSTAL_DWARF;

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
      (OBJ_FLAGGED(obj, ITEM_ANTI_ARCANA_GOLEM) && IS_ARCANA_GOLEM(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DROW) && IS_DROW(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DUERGAR) && IS_DUERGAR(ch)) ||
      (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch)))
    return 1;
  else
    return 0;
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
