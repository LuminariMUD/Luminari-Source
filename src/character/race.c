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
#include "magic/spells.h"
#include "interpreter.h"
#include "constants.h"
#include "act.h"
#include "handler.h"
#include "comm.h"
#include "race.h"
#include "abilities.h"
#include "feats.h"
#include "class.h"
#include "backgrounds.h"
#include "character_creation_content.h"
#include "obj/treasure.h"

/* defines */
#define Y TRUE
#define N FALSE
/* racial classification for PC races */
#define IS_NORMAL 0
#define IS_ADVANCE 1
#define IS_EPIC_R 2

/* some pre setup here */

struct race_data race_list[NUM_EXTENDED_RACES];
int race_sort_info[NUM_EXTENDED_RACES + 1];

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
    "Str", "Con", "Int", "Wis", "Dex", "Cha", "\n"};
void set_race_abilities(int race, int str_mod, int con_mod, int int_mod, int wis_mod, int dex_mod,
                        int cha_mod)
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

int race_starting_hp_bonus(int race_num)
{
  switch (race_num)
  {
  case RACE_CRYSTAL_DWARF:
  case RACE_TRELUX:
  case RACE_LICH:
  case RACE_VAMPIRE:
  case RACE_HALF_ILLITHID:
  case RACE_MYCONID:
    return 10;
  default:
    return 0;
  }
}

int race_hp_bonus_per_level(int race_num)
{
  switch (race_num)
  {
  case RACE_CRYSTAL_DWARF:
  case RACE_TRELUX:
  case RACE_LICH:
  case RACE_VAMPIRE:
  case RACE_HALF_ILLITHID:
  case RACE_MYCONID:
    return 4;
  case RACE_HALF_OGRE:
    return 2;
  case RACE_WEMIC:
    return 1;
  default:
    return 0;
  }
}

/* appropriate alignments for given race */
void set_race_alignments(int race, int lg, int ng, int cg, int ln, int tn, int cn, int le, int ne,
                         int ce)
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
void set_race_attack_types(int race, int hit, int sting, int whip, int slash, int bite,
                           int bludgeon, int crush, int pound, int claw, int maul, int thrash,
                           int pierce, int blast, int punch, int stab, int slice, int thrust,
                           int hack, int rake, int peck, int smash, int trample, int charge,
                           int gore)
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
    set_race_attack_types(i, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
                          N);

    /* any linked lists to initailze? */
  }
}

/* papa assign-function for adding races to the race list */
static void add_race(int race, const char *name, const char *type, const char *type_color,
                     const char *abbrev, const char *abbrev_color, ubyte family, byte size,
                     sbyte is_pc, ubyte level_adjustment, int unlock_cost, byte epic_adv)
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
static void set_race_details(int race, const char *descrip, const char *morph_to_char,
                             const char *morph_to_room)
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
struct race_feat_assign *create_feat_assign_races(int feat_num, int level_received, bool stacks)
{
  struct race_feat_assign *feat_assign = NULL;

  CREATE(feat_assign, struct race_feat_assign, 1);
  feat_assign->feat_num = feat_num;
  feat_assign->level_received = level_received;
  feat_assign->stacks = stacks;

  return feat_assign;
}
/* actual function called to perform the feat assignment */
void feat_race_assignment(int race_num, int feat_num, int level_received, bool stacks)
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

/* Creation policy is deliberately independent of numeric density. */
bool race_is_creation_eligible(int race_num)
{
  if (race_num < 0 || race_num >= NUM_EXTENDED_RACES)
    return FALSE;

  if (!race_list[race_num].is_pc)
    return FALSE;

  /* These races are acquired only by their existing conversion paths. */
  if (race_num == RACE_LICH || race_num == RACE_VAMPIRE)
    return FALSE;

  return TRUE;
}

bool race_is_selectable_for_creation(struct char_data *ch, int race_num)
{
  if (!race_is_creation_eligible(race_num))
    return FALSE;

  return !is_locked_race(race_num) || has_unlocked_race(ch, race_num);
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
      write_to_output(d, "%s%-20.20s %s", race_is_available(ch, counter) ? " " : "*",
                      race_list[counter].type, !(++columns % 3) ? "\r\n" : "");
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
  send_to_char(ch, "\tcNormal/Adv/Epic?: \tn%s\r\n",
               (race_list[race].epic_adv == IS_EPIC_R)    ? "Epic Race"
               : (race_list[race].epic_adv == IS_ADVANCE) ? "Advance Race"
                                                          : "Normal Race");
  send_to_char(ch, "\tcUnlock Cost     : \tn%d Account XP\r\n", race_list[race].unlock_cost);
  send_to_char(ch, "\tcPlayable Race?  : \tn%s\r\n",
               race_list[race].is_pc ? "\tnYes\tn" : "\trNo, ask staff\tn");

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* build buffer for ability modifiers */
  for (i = 0; i < NUM_ABILITY_MODS; i++)
  {
    stat_mod = race_list[race].ability_mods[i];
    if (stat_mod != 0)
    {
      found = TRUE;
      len = snprintf_append(buf, sizeof(buf), len, "%s %s%d ", abil_mod_names[i],
                            (stat_mod > 0) ? "+" : "", stat_mod);
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
        send_to_char(ch, "Level: %-2d, Stacks: %-3s, Feat: %s\r\n", feat_assign->level_received,
                     feat_assign->stacks ? "Yes" : "No", feat_list[feat_assign->feat_num].name);
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


  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_HUMAN, "human", "Human", "\tBHuman\tn", "Humn", "\tBHumn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(
      RACE_HUMAN,
      /*descrip*/
      "Humans possess exceptional drive and a great capacity to endure "
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
  set_race_details(
      RACE_ELF,
      /*descrip*/
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
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes High Elven.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes High Elven.");
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
  set_race_details(
      RACE_WOOD_ELF,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes Wild Elven.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes Wild Elven.");
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
  set_race_details(
      RACE_HALF_ELF,
      /*descrip*/
      "Elves have long drawn the covetous gazes of other races. Their "
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
      "of neither. Half-elves can breed with one another, but even these \"pureblood\" "
      "half-elves tend to be viewed as bastards by humans and elves alike. Caught "
      "between destiny and derision, half-elves often view themselves as the middle "
      "children of the world.",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes Half-Elven.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes Half-Elven.");
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
  set_race_details(
      RACE_HALF_DROW,
      // description
      "A half-drow was the offspring of one human parent and one drow parent. A half-"
      "drow generally had dusky skin, silver or white hair, and human eye colors. They "
      "could see around 60 feet (18 m) with darkvision, but otherwise had no other "
      "known drow traits or abilities. "
      "Half-drow were most commonly encountered in Dambrath and in the Underdark. "
      "House Ousstyl, in particular, was known for having mated with humans. "
      "Half-drow were often conceived when a male drow mated with one of his human "
      "female slaves or when outcast drow mated with surface-dwelling species. ",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes Half-Drow.",
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
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes Dragonborn.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes Dragonborn.");
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
  set_race_details(
      RACE_TIEFLING,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Tiefling.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes a Tiefling.");
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
  set_race_details(
      RACE_AASIMAR,
      /*descrip*/
      "Aasimar were human-based planetouched, native outsiders that had in their blood "
      "some good, otherworldly characteristics. They were often, but not always, descended "
      "from celestials and other creatures of pure good alignment, but while predisposed to "
      "good alignments, aasimar were by no means always good. Aasimar bore the mark of their "
      "celestial touch through many different physical features that often varied from "
      "individual to individual. Most commonly, aasimar were very similar to humans, like "
      "tieflings and other planetouched. Nearly all aasimar were uncommonly beautiful and "
      "still, and they were often significantly taller than humans as well.",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes an Aasimar.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes an Aasimar.");
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
  set_race_details(
      RACE_TABAXI,
      /*descrip*/
      "Tabaxi are taller than most humans at six to seven feet. Their bodies are slender "
      "and covered in spotted or striped fur. Like most felines, Tabaxi had long tails and "
      "retractable claws. Tabaxi fur color ranged from light yellow to brownish red. Tabaxi "
      "eyes are slit-pupilled and usually green or yellow. Tabaxi are competent swimmers and "
      "climbers as well as speedy runners. They had a good sense of balance and an acute sense "
      "of smell. Depending on their region and fur coloration, tabaxi are known by different "
      "names. Tabaxi with solid spots are sometimes called leopard men and tabaxi with rosette "
      "spots are called jaguar men. The way the tabaxi pronounced their own name also varied; "
      "the 'leopard men' pronounced it ta-BAEK-see, and the jaguar men tah-BAHSH-ee. ",
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
  add_race(RACE_SHIELD_DWARF, "mountain dwarf", "Mountain Dwarf", "\tJMountain Dwarf\tn", "MtDw",
           "\tJMtDw\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 0, 0, IS_NORMAL);
  set_race_details(
      RACE_DWARF,
      /*descrip*/
      "Dwarves are a stoic but stern race, ensconced in cities carved "
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
  set_race_details(
      RACE_GOLD_DWARF,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Gold Dwarf.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes a Gold Dwarf.");
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
  add_race(RACE_LIGHTFOOT_HALFLING, "lightfoot halfling", "Lightfoot Halfling",
           "\tPLightfoot Halfling\tn", "LtHf", "\tPLtHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(
      RACE_HALFLING,
      /*descrip*/
      "Optimistic and cheerful by nature, blessed with uncanny luck, "
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
  set_race_details(
      RACE_GNOME,
      /*descrip*/
      "Gnomes are distant relatives of the fey, and their history tells "
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
  add_race(RACE_STOUT_HALFLING, "stout halfling", "Stout Halfling", "\tTStout Halfling\tn", "StHf",
           "\tTStHf\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(
      RACE_STOUT_HALFLING,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Stout Halfling.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes a Stout Halfling.");
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
  add_race(RACE_FOREST_GNOME, "forest gnome", "Forest Gnome", "\tVForest Gnome\tn", "FrGn",
           "\tVFrGn\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_SMALL, TRUE, 0, 0, IS_NORMAL);
  set_race_details(
      RACE_FOREST_GNOME,
      // Description
      "Forest gnomes are among the least commonly seen gnomes on Toril, far shier than "
      "even their deep gnome cousins. Small and reclusive, forest gnomes are so "
      "unknown to most non-gnomes that they have repeatedly been \"discovered\" by "
      "wandering outsiders who happen into their villages. Timid to an extreme, "
      "forest gnomes almost never leave their hidden homes. Compared with other "
      "gnomes, forest gnomes are even more diminutive than is typical of the stunted "
      "race, rarely growing taller than 21/2 feet in height or weighing in over 30 lbs. "
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Forest Gnome.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes a Forest Gnome.");
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
  set_race_details(
      RACE_HALF_ORC,
      /*descrip*/
      "As seen by civilized races, half-orcs are monstrosities, the result "
      "of perversion and violence-whether or not this is actually true. Half-orcs "
      "are rarely the result of loving unions, and as such are usually forced to "
      "grow up hard and fast, constantly fighting for protection or to make names "
      "for themselves. Half-orcs as a whole resent this treatment, and rather than "
      "play the part of the victim, they tend to lash out, unknowingly confirming "
      "the biases of those around them. A few feared, distrusted, and spat-upon "
      "half-orcs manage to surprise their detractors with great deeds and unexpected "
      "wisdom-though sometimes it's easier just to crack a few skulls. Some half-orcs "
      "spend their entire lives proving to full-blooded orcs that they are just as "
      "fierce. Others opt for trying to blend into human society, constantly demonstrating "
      "that they aren't monsters. Their need to always prove themselves worthy "
      "encourages half-orcs to strive for power and greatness within the society "
      "around them.",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes Half-Orcish.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes Half-Orcish.");
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
                   /*descrip*/
                   "Half-Trolls are large, green, lanky, powerful, agile and hardy.  They tend "
                   "to have warty thick skin, black eyes and mottled black or brown hair "
                   "on their head.  Half-Trolls are extremely destructive in nature, often "
                   "searching or planning to do acts of destruction against weaker races. "
                   "Half-Trolls tend to inhabit swamps and lakes, and tend to band together in "
                   "war clans.",
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes Half-Troll.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes Half-Troll.");
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
  add_race(RACE_ARCANA_GOLEM, "arcanagolem", "ArcanaGolem", "\tRArcana \tcGolem\tn", "ArGo",
           "\tRAr\tcGo\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(RACE_ARCANA_GOLEM,
                   /*descrip*/
                   "Arcana Golems are mortal spellcasters (typically, but not always human) "
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
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes Arcana Golem.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes Arcana Golem.");
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
  set_race_details(
      RACE_DROW,
      /*descrip*/
      "Cruel and cunning, Drow, also known as dark elves, were cursed into their present "
      "appearance "
      "by the Arcanite and Prisoner's magic, were led down the path to evil and corruption. "
      "The drow have black skin that resembles polished obsidian and stark white or "
      "pale yellow hair. They commonly have red eyes. Drow are the same size as elves but a "
      "bit thinner."
      "\r\n"
      "Descending into the Underworld, they formed cities shaped from the rock of cyclopean "
      "caverns. "
      "They developed a theocratic and matriarchal society based on power and deceit. "
      "Females generally hold all positions of power and responsibility in the government, the "
      "military, and the home. "
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
  set_race_details(
      RACE_DUERGAR,
      /*descrip*/
      "Duergar dwell in subterranean caverns far from the touch of light. They detest all races "
      "living beneath the sun, but that hatred pales beside their loathing of their surface-dwarf "
      "cousins. Dwarves and Duergar once were one race, but the dwarves left the deeps for their "
      "mountain strongholds. Duergar still consider themselves the only true Dwarves, and the "
      "rightful heirs of all beneath the world's surface. In appearance, Duergar resemble gray-"
      "skinned Dwarves, bearded but bald, with cold, lightless eyes. They favor taking captives "
      "in battle over wanton slaughter, save for surface dwarves, who are slain without "
      "hesitation. "
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
  add_race(RACE_CRYSTAL_DWARF, "crystaldwarf", "CrystalDwarf", "\tCCrystal \tgDwarf\tn", "CDwf",
           "\tCC\tgDwf\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_HUMANOID, SIZE_MEDIUM, TRUE, 10, 30000, IS_EPIC_R);
  set_race_details(RACE_CRYSTAL_DWARF,
                   /*descrip*/
                   "Crystal Dwarves are dwarves whose committment to earth have way "
                   "exceeded even the norms of dwarves.  A cross of divine power and "
                   "magic enhance these dwarves connection to the earth to levels that "
                   "are near earth elementals.  Their bodies take crystal-like texture "
                   "and their skin takes on the sharp angles of crystals as well.  Often "
                   "their pupils become diamond shaped and share the reflective property "
                   "of diamonds as well.  In addition Crystal Dwarves have the ability "
                   "to transform parts of their body to fully crystal-like weight and "
                   "texture - which can be extremely effective in offensive and defensive "
                   "maneuvers in combat.",
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes Crystal-Dwarf.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes Crystal-Dwarf.");
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
  set_race_details(
      RACE_TRELUX,
      /*descrip*/
      "Trelux have a small head with a fused thorax and abdomen. Trelux also have eight "
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
  set_race_details(
      RACE_SHADE,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes that of a Shade.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes that of a Shade.");
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
  set_race_details(
      RACE_GOLIATH,
      // Description
      "At the highest mountain peaks - far above the slopes where trees grow and where the air "
      "is thin and the frigid winds howl - dwell the reclusive goliaths. Few folk can claim to "
      "have seen a goliath, and fewer still can claim friendship with them. Goliaths wander a "
      "bleak realm of rock, wind, and cold. Their bodies look as if they are carved from mountain "
      "stone and give them great physical power. Their spirits take after the wandering wind, "
      "making "
      "them nomads who wander from peak to peak. Their hearts are infused with the cold regard of "
      "their frigid realm, leaving each goliath with the responsibility to earn a place in the "
      "tribe or die trying."
      "For goliaths, competition exists only when it is supported by a level playing field. "
      "Competition "
      "measures talent, dedication, and effort. Those factors determine survival in their home "
      "territory, "
      "not reliance on magic items, money, or other elements that can tip the balance one way or "
      "the other. "
      "Goliaths happily rely on such benefits, but they are careful to remember that such an "
      "advantage can "
      "always be lost. A goliath who relies too much on them can grow complacent, a recipe for "
      "disaster in the mountains."
      "This trait manifests most strongly when goliaths interact with other folk. The relationship "
      "between peasants and "
      "nobles puzzles goliaths. If a king lacks the intelligence or leadership to lead, then "
      "clearly the most talented "
      "person in the kingdom should take his place. Goliaths rarely keep such opinions to "
      "themselves, and mock folk who "
      "rely on society's structures or rules to maintain power.",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes that of a Goliath.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes that of a Goliath.");
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
  set_race_details(
      RACE_GOBLIN,
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
  set_race_abilities(RACE_GOBLIN, -2, 2, 0, 0, 4, -2);         /* str con int wis dex cha */
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
  set_race_details(
      RACE_HOBGOBLIN,
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
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Hobgoblin.",
      /*morph to-room*/
      "$n's body twists and contorts painfully until $s form becomes a Hobgoblin.");
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
  /* affect assignment */
  /*                  race-num  affect            lvl */
  /****************************************************************************/

  /****************************************************************************/
  add_race(RACE_WEMIC, "wemic", "Wemic", "\tYWemic\tn", "Wemc", "\tYWemc\tn",
           RACE_TYPE_MONSTROUS_HUMANOID, SIZE_LARGE, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(
      RACE_WEMIC,
      "Wemics are leonine centaur-folk: proud hunters whose humanoid torsos rise from powerful "
      "lion bodies. Their strength, speed, keen senses, and wilderness lore make them fearsome "
      "front-line combatants and scouts. This race is the Luminari equivalent of the tribal "
      "Barbarian people found in Realms of Luminari.",
      "Your body broadens and a leonine lower body forms until you become a Wemic.",
      "$n's body broadens and a leonine lower body forms until $e becomes a Wemic.");
  set_race_genders(RACE_WEMIC, N, Y, Y);
  set_race_abilities(RACE_WEMIC, 8, 4, -2, 2, 2, -2);
  set_race_alignments(RACE_WEMIC, Y, Y, Y, Y, Y, Y, Y, Y, Y);
  set_race_attack_types(RACE_WEMIC, N, N, N, N, Y, N, N, N, Y, N, N, N, N, N, N, N, N, N, N, N, N,
                        Y, N, N);
  feat_race_assignment(RACE_WEMIC, FEAT_INFRAVISION, 1, N);
  feat_race_assignment(RACE_WEMIC, FEAT_NATURAL_ATHLETE, 1, N);
  feat_race_assignment(RACE_WEMIC, FEAT_POWERFUL_BUILD, 1, N);
  feat_race_assignment(RACE_WEMIC, FEAT_CLAWS_AND_BITE, 1, N);
  feat_race_assignment(RACE_WEMIC, FEAT_SURVIVAL_INSTINCT, 1, N);
  feat_race_assignment(RACE_WEMIC, FEAT_HARDY, 1, N);
  race_list[RACE_WEMIC].racial_language = SKILL_LANG_COMMON;

  /****************************************************************************/
  add_race(RACE_HALF_OGRE, "half ogre", "Half-Ogre", "\trHalf-Ogre\tn", "HOgr", "\trHOgr\tn",
           RACE_TYPE_GIANT, SIZE_LARGE, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(
      RACE_HALF_OGRE,
      "Half-ogres combine a giant's raw power and reach with the adaptability of their smaller "
      "kin. They are imposing, durable, and often underestimated as tacticians. This race is the "
      "Luminari equivalent of the Ogre player race found in Realms of Luminari.",
      "Your frame swells with giant blood until you become a Half-Ogre.",
      "$n's frame swells with giant blood until $e becomes a Half-Ogre.");
  set_race_genders(RACE_HALF_OGRE, N, Y, Y);
  set_race_abilities(RACE_HALF_OGRE, 6, 2, -2, 0, -2, -2);
  set_race_alignments(RACE_HALF_OGRE, Y, Y, Y, Y, Y, Y, Y, Y, Y);
  set_race_attack_types(RACE_HALF_OGRE, Y, N, N, N, N, N, N, Y, N, N, N, N, N, Y, N, N, N, N, N, N,
                        Y, N, N, N);
  feat_race_assignment(RACE_HALF_OGRE, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_OGRE, FEAT_POWERFUL_BUILD, 1, N);
  feat_race_assignment(RACE_HALF_OGRE, FEAT_STRONG_AGAINST_POISON, 1, N);
  feat_race_assignment(RACE_HALF_OGRE, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_HALF_OGRE, FEAT_ARMOR_SKIN, 1, Y);
  race_list[RACE_HALF_OGRE].racial_language = SKILL_LANG_GIANT;

  /****************************************************************************/
  add_race(RACE_HALF_ILLITHID, "half illithid", "Half-Illithid", "\tMHalf-Illithid\tn", "HIll",
           "\tMHIll\tn", RACE_TYPE_ABERRATION, SIZE_MEDIUM, TRUE, 10, 30000, IS_EPIC_R);
  set_race_details(
      RACE_HALF_ILLITHID,
      "Half-illithids bear the transformed mind and unsettling features of the illithid while "
      "retaining a mortal ancestry. Their formidable intellect, will, psionic instinct, and "
      "levitation make them an epic race. This race is the Luminari equivalent of the Illithid "
      "player race found in Realms of Luminari.",
      "Your thoughts expand as illithid traits reshape your face and mind.",
      "$n shudders as illithid traits reshape $s face and mind.");
  set_race_genders(RACE_HALF_ILLITHID, N, Y, Y);
  set_race_abilities(RACE_HALF_ILLITHID, 0, 0, 4, 4, 0, 4);
  set_race_alignments(RACE_HALF_ILLITHID, Y, Y, Y, Y, Y, Y, Y, Y, Y);
  set_race_attack_types(RACE_HALF_ILLITHID, Y, N, N, N, N, N, N, N, N, N, Y, N, N, Y, N, N, N, N, N,
                        N, N, N, N, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_QUICK_MIND, 1, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_SLA_LEVITATE, 1, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_HALF_ILLITHID, FEAT_HARDY, 1, N);
  race_list[RACE_HALF_ILLITHID].racial_language = SKILL_LANG_ABERRATION;

  /****************************************************************************/
  add_race(RACE_YUAN_TI, "yuan-ti", "Yuan-Ti", "\tgYuan-Ti\tn", "Yuan", "\tgYuan\tn",
           RACE_TYPE_MONSTROUS_HUMANOID, SIZE_MEDIUM, TRUE, 2, 1000, IS_ADVANCE);
  set_race_details(
      RACE_YUAN_TI,
      "Yuan-ti are serpentfolk whose controlled minds, scaled bodies, venom, and innate magic "
      "make them dangerous adversaries. Pureblooded yuan-ti can pass among humanoids, but their "
      "reptilian heritage is never entirely hidden. This race directly preserves the Yuan-Ti "
      "archetype found in Realms of Luminari.",
      "Scales ripple across your skin as your form becomes Yuan-Ti.",
      "Scales ripple across $n's skin as $s form becomes Yuan-Ti.");
  set_race_genders(RACE_YUAN_TI, N, Y, Y);
  set_race_abilities(RACE_YUAN_TI, 0, 0, 2, 0, 2, 2);
  set_race_alignments(RACE_YUAN_TI, N, N, N, Y, Y, Y, Y, Y, Y);
  set_race_attack_types(RACE_YUAN_TI, N, N, N, N, Y, N, N, N, N, N, Y, N, N, N, N, N, N, N, N, N, N,
                        N, N, N);
  feat_race_assignment(RACE_YUAN_TI, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_YUAN_TI, FEAT_POISON_BITE, 1, N);
  feat_race_assignment(RACE_YUAN_TI, FEAT_POISON_IMMUNITY, 1, N);
  feat_race_assignment(RACE_YUAN_TI, FEAT_STUBBORN_MIND, 1, N);
  feat_race_assignment(RACE_YUAN_TI, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_YUAN_TI, FEAT_ARMOR_SKIN, 1, Y);
  race_list[RACE_YUAN_TI].racial_language = SKILL_LANG_DRACONIC;

  /****************************************************************************/
  /*            simple-name, no-color-name, color-name, abbrev, color-abbrev*/
  add_race(RACE_LICH, "lich", "Lich", "\tLLich\tn", "Lich", "\tLLich\tn",
           /* race-family,     size-class,  Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 999999999, IS_EPIC_R);
  set_race_details(
      RACE_LICH,
      /*descrip*/
      "Few creatures are more feared than the lich. The pinnacle of necromantic art, who "
      "has chosen to shed his life as a method to cheat death by becoming undead. While many who "
      "reach "
      "such heights of power stop at nothing to achieve immortality, the idea of becoming a lich "
      "is "
      "abhorrent to most creatures. The process involves the extraction of ones life-force and its "
      "imprisonment in a specially prepared phylactery.  One gives up life, but in trapping "
      "life he also traps his death, and as long as his phylactery remains intact he can continue "
      "on in "
      "his research and work without fear of the passage of time."
      "\r\n\r\n"
      "The quest to become a lich is a lengthy one. While construction of the magical phylactery "
      "to "
      "contain ones soul is a critical component, a prospective lich must also learn the "
      "secrets of transferring his soul into the receptacle and of preparing his body for the "
      "transformation into undeath, neither of which are simple tasks. Further complicating the "
      "ritual "
      "is the fact that no two bodies or souls are exactly alike, a ritual that works for one "
      "spellcaster "
      "might simply kill another or drive him insane. "
      "\r\n\r\n"
      "Please note that a Lich will be the same size class they were before the "
      "transformation.\r\n  "
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
           RACE_TYPE_UNDEAD, SIZE_MEDIUM, TRUE, 10, 999999999, IS_EPIC_R);
  set_race_details(
      RACE_VAMPIRE,
      /*descrip*/
      "Vampires are one of the most fearsome of the Undead creatures in Lumia. With unnatural "
      "strength, "
      " agility and cunning, they can easily overpower most other creatures with their physical "
      "prowess alone. But the vampire is much more deadly than just his claws and wits. Vampires "
      "have "
      "a number of supernatural abilities that inspire dread in his foes.  Gaining sustenance from "
      "the blood of the living, vampires can heal quickly from almost any wound. For the victims "
      "of their feeding, they may raise again as vampiric spawn... an undead creature under the "
      "vampire's "
      "control with many vampiric abilities of their own. They may also call animal minions to aid "
      "them "
      "in battle, from wolves, to swarms of rats and vampire bats as well. They can dominate "
      "intelligent "
      "foes with a simple gaze, and they may drain the energy of living beings with an unarmed "
      "attack. "
      "They can also assume the form of a wolf or a giant bat, as well as assume a gasoeus form at "
      "will, "
      "and have the ability to scale sheer surfaces as easily as a spider may."
      "\r\n\r\n"
      "But a vampire is not without its weaknesses. Exposed to sunlight, they will quickly be "
      "reduced "
      "to ash, and moving water is worse, able to kill a vampire submerged in running water in "
      "less than a minute."
      "\r\n\r\n"
      "Being a vampire is a state most would consider a curse, however there are legends of those "
      "who "
      "sought out the 'gift' of vampirism, with some few who actually obtained it. To this day "
      "however, "
      "such secrets have been lost to the ages. However these are the days of great heroes and "
      "villains, "
      "and such days often bring to light secrets of the past. Perhaps one day soon the legends "
      "may become "
      "reality."
      "\r\n\r\n"
      "Please note that a Vampire will be the same size class they were before the "
      "transformation.\r\n  "
      "Please note that becoming a Vampire requires level 30 and will reset your exp to 0.\r\n  "
      "Please note that a Vampire has all the advantages/disadvantages of being Undead.\r\n  ",
      /*morph to-char*/
      "Your body twists and contorts painfully until your form becomes a Vampire.",
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
           RACE_TYPE_HUMANOID, SIZE_TINY, TRUE, 10, 50000, IS_EPIC_R);
  set_race_details(RACE_FAE,
                   // Description
                   "Fae are relatively reclusive. They would rather spend their time frolicking in "
                   "woodland glades than "
                   "cavorting with other races. They are the consummate trickster, often devising "
                   "elaborate ruses to lead "
                   "strangers away from their glades. They do love visitors though, even if it is "
                   "just to have a target for "
                   "their tricks. They also have a love of stories and magic... Bards are "
                   "therefore almost always welcome in "
                   "a glade. Monks are greatly cherished as visitors as well, due to their "
                   "ingrained resilience to faerie glamor. "
                   "Competitions are held to see who can trick the monk, with the winner crowned "
                   "prince of glamor for the day. It "
                   "should be noted though, then monks are generally not harmed in order to "
                   "encourage their return.\r\n\r\n"
                   "They also posses an utterly alien sense of morals. They would completely erase "
                   "a mortals memory, or put one to "
                   "sleep for a year without thought to the consequences. Their chaotic behavior "
                   "often stems from this lack of "
                   "concern for consequences as Fae tend to understand the term in a different "
                   "light than mortals. Likewise, harm "
                   "to a mortal is often disregarded in the same manner. The saying \"It's all fun "
                   "and games until someone losses an "
                   "arm... then its just hilarious \" is very applicable here.",
                   /*morph to-char*/
                   "Your body twists and contorts painfully until your form becomes that of a Fae.",
                   /*morph to-room*/
                   "$n's body twists and contorts painfully until $s form becomes that of a Fae.");
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
  add_race(RACE_CONSTRICTOR_SNAKE, "constrictor snake", "Constrictor Snake", "Constrictor Snake",
           "CSnk", "CSnk",
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
  add_race(RACE_GIANT_CROCODILE, "giant crocodile", "Giant Crocodile", "Giant Crocodile", "GCrc",
           "GCrc",
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
  add_race(RACE_DIRE_CROCODILE, "dire crocodile", "Dire Crocodile", "Dire Crocodile", "DCrc",
           "DCrc",
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
  add_race(RACE_MYCONID, "myconid", "Myconid", "\tgMyconid\tn", "Myco", "\tgMyco\tn",
           /* race-family, size-class, Is PC?, Lvl-Adj, Unlock, Epic? */
           RACE_TYPE_PLANT, SIZE_LARGE, TRUE, 10, 30000, IS_EPIC_R);
  set_race_details(
      RACE_MYCONID,
      "Myconids are communal fungal beings whose alien biology resists poison, paralysis, and "
      "sleep. Powerful myconid adventurers combine immense physical resilience with a patient, "
      "collective outlook. This epic race completes the unfinished Myconid player concept from "
      "Realms of Luminari.",
      "Fungal tissue spreads across your growing frame until you become a Myconid.",
      "Fungal tissue spreads across $n's growing frame until $e becomes a Myconid.");
  set_race_genders(RACE_MYCONID, Y, Y, Y);
  set_race_abilities(RACE_MYCONID, 8, 6, -2, -2, -4, -4);
  set_race_alignments(RACE_MYCONID, Y, Y, Y, Y, Y, Y, Y, Y, Y);
  set_race_attack_types(RACE_MYCONID,
                        /* hit sting whip slash bite bludgeon crush pound claw maul thrash pierce */
                        N, N, N, N, N, Y, Y, Y, N, N, N, N,
                        /* blast punch stab slice thrust hack rake peck smash trample charge gore */
                        N, N, N, N, N, N, N, N, Y, N, N, N);
  feat_race_assignment(RACE_MYCONID, FEAT_ULTRAVISION, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_VITAL, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_HARDY, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_POISON_IMMUNITY, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_SLEEP_ENCHANTMENT_IMMUNITY, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_PARALYSIS_IMMUNITY, 1, N);
  feat_race_assignment(RACE_MYCONID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_MYCONID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_MYCONID, FEAT_ARMOR_SKIN, 1, Y);
  feat_race_assignment(RACE_MYCONID, FEAT_ARMOR_SKIN, 1, Y);
  race_list[RACE_MYCONID].racial_language = SKILL_LANG_UNDERCOMMON;
  /****************************************************************************/

  /****************************************************************************/
  /*                  simple-name, no-color-name, color-name, abbrev (4), color-abbrev (4) */
  add_race(RACE_SHAMBLING_MOUND, "shambling mound", "Shambling Mound", "Shambling Mound", "Shmb",
           "Shmb",
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
  add_race(RACE_GREATER_TREANT, "greater treant", "Greater Treant", "Greater Treant", "GTrt",
           "GTrt",
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
  add_race(RACE_SMALL_FIRE_ELEMENTAL, "small fire elemental", "Small Fire Elemental",
           "Small Fire Elemental", "SFEl", "SFEl",
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
  add_race(RACE_MEDIUM_FIRE_ELEMENTAL, "medium fire elemental", "Medium Fire Elemental",
           "Medium Fire Elemental", "MFEl", "MFEl",
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
  add_race(RACE_LARGE_FIRE_ELEMENTAL, "large fire elemental", "Large Fire Elemental",
           "Large Fire Elemental", "LFEl", "LFEl",
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
  add_race(RACE_HUGE_FIRE_ELEMENTAL, "huge fire elemental", "Huge Fire Elemental",
           "Huge Fire Elemental", "HFEl", "HFEl",
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
  add_race(RACE_GARGANTUAN_FIRE_ELEMENTAL, "greater fire elemental", "Greater Fire Elemental",
           "Greater Fire Elemental", "GFEl", "GFEl",
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
  add_race(RACE_COLOSSAL_FIRE_ELEMENTAL, "elder fire elemental", "Elder Fire Elemental",
           "Elder Fire Elemental", "EFEl", "EFEl",
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
  add_race(RACE_SMALL_EARTH_ELEMENTAL, "small earth elemental", "Small Earth Elemental",
           "Small Earth Elemental", "SEEl", "SEEl",
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
  add_race(RACE_MEDIUM_EARTH_ELEMENTAL, "medium earth elemental", "Medium Earth Elemental",
           "Medium Earth Elemental", "MEEl", "MEEl",
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
  add_race(RACE_LARGE_EARTH_ELEMENTAL, "large earth elemental", "Large Earth Elemental",
           "Large Earth Elemental", "LEEl", "LEEl",
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
  add_race(RACE_HUGE_EARTH_ELEMENTAL, "huge earth elemental", "Huge Earth Elemental",
           "Huge Earth Elemental", "HEEl", "HEEl",
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
  add_race(RACE_GARGANTUAN_EARTH_ELEMENTAL, "greater earth elemental", "Greater Earth Elemental",
           "Greater Earth Elemental", "GEEl", "GEEl",
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
  add_race(RACE_COLOSSAL_EARTH_ELEMENTAL, "elder earth elemental", "Elder Earth Elemental",
           "Elder Earth Elemental", "EEEl", "EEEl",
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
  add_race(RACE_SMALL_AIR_ELEMENTAL, "small air elemental", "Small Air Elemental",
           "Small Air Elemental", "SAEl", "SAEl",
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
  add_race(RACE_MEDIUM_AIR_ELEMENTAL, "medium air elemental", "Medium Air Elemental",
           "Medium Air Elemental", "MAEl", "MAEl",
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
  add_race(RACE_LARGE_AIR_ELEMENTAL, "large air elemental", "Large Air Elemental",
           "Large Air Elemental", "LAEl", "LAEl",
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
  add_race(RACE_HUGE_AIR_ELEMENTAL, "huge air elemental", "Huge Air Elemental",
           "Huge Air Elemental", "HAEl", "HAEl",
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
  add_race(RACE_GARGANTUAN_AIR_ELEMENTAL, "greater air elemental", "Greater Air Elemental",
           "Greater Air Elemental", "GAEl", "GAEl",
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
  add_race(RACE_COLOSSAL_AIR_ELEMENTAL, "elder air elemental", "Elder Air Elemental",
           "Elder Air Elemental", "EAEl", "EAEl",
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
  add_race(RACE_SMALL_WATER_ELEMENTAL, "small water elemental", "Small Water Elemental",
           "Small Water Elemental", "SWEl", "SWEl",
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
  add_race(RACE_MEDIUM_WATER_ELEMENTAL, "medium water elemental", "Medium Water Elemental",
           "Medium Water Elemental", "MWEl", "MWEl",
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
  add_race(RACE_LARGE_WATER_ELEMENTAL, "large water elemental", "Large Water Elemental",
           "Large Water Elemental", "LWEl", "LWEl",
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
  add_race(RACE_HUGE_WATER_ELEMENTAL, "huge water elemental", "Huge Water Elemental",
           "Huge Water Elemental", "HWEl", "HWEl",
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
  add_race(RACE_GARGANTUAN_WATER_ELEMENTAL, "greater water elemental", "Greater Water Elemental",
           "Greater Water Elemental", "GWEl", "GWEl",
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
  add_race(RACE_COLOSSAL_WATER_ELEMENTAL, "elder water elemental", "Elder Water Elemental",
           "Elder Water Elemental", "EWEl", "EWEl",
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
  char arg_buf[MAX_INPUT_LENGTH];
  char *arg = arg_buf;

  int l = 0; /* string length */

  if (arg_in == NULL)
    return RACE_UNDEFINED;

  strlcpy(arg_buf, arg_in, sizeof(arg_buf));

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
  if (is_abbrev(arg, "half-troll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "halftroll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "half troll"))
    return RACE_HALF_TROLL;
  if (is_abbrev(arg, "half-ogre"))
    return RACE_HALF_OGRE;
  if (is_abbrev(arg, "halfogre"))
    return RACE_HALF_OGRE;
  if (is_abbrev(arg, "half ogre"))
    return RACE_HALF_OGRE;
  if (is_abbrev(arg, "ogre"))
    return RACE_HALF_OGRE;
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
  if (is_abbrev(arg, "wemic"))
    return RACE_WEMIC;
  if (is_abbrev(arg, "barbarian"))
    return RACE_WEMIC;
  if (is_abbrev(arg, "half-illithid"))
    return RACE_HALF_ILLITHID;
  if (is_abbrev(arg, "halfillithid"))
    return RACE_HALF_ILLITHID;
  if (is_abbrev(arg, "half illithid"))
    return RACE_HALF_ILLITHID;
  if (is_abbrev(arg, "illithid"))
    return RACE_HALF_ILLITHID;
  if (is_abbrev(arg, "yuan-ti"))
    return RACE_YUAN_TI;
  if (is_abbrev(arg, "yuanti"))
    return RACE_YUAN_TI;
  if (is_abbrev(arg, "yuan ti"))
    return RACE_YUAN_TI;
  if (is_abbrev(arg, "myconid"))
    return RACE_MYCONID;
  if (is_abbrev(arg, "mycanoid"))
    return RACE_MYCONID;
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

  return RACE_UNDEFINED;
}

// returns the proper integer for the race, given a character
bitvector_t find_race_bitvector(const char *arg)
{
  size_t rpos, ret = 0;

  for (rpos = 0; (size_t)rpos < strlen(arg); rpos++)
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
  case LEGACY_RACE_SILVANESTI_ELF:
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
  int candidate = RACE_HUMAN;
  int count = 0;
  int race = 0;

  for (race = 0; race < NUM_EXTENDED_RACES; race++)
  {
    if (!race_is_creation_eligible(race) || race_list[race].epic_adv != IS_NORMAL)
      continue;

    count++;
    if (dice(1, count) == 1)
      candidate = race;
  }

  return candidate;
}

/* can a class be this race because of potential alignment issues? (character creation) */
int valid_class_race_alignment(int class, int race)
{
  int i = 0;

  for (i = 0; i < NUM_ALIGNMENTS; i++)
  {
    if (valid_align_by_class(i, class) && valid_align_by_race(i, race))
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
  const struct character_creation_homeland *homeland =
      character_creation_homeland_for_region(region);

  if (homeland == NULL)
    return "That homeland is not available.";

  return homeland->description;
}
int get_region_language(int region)
{
  const struct character_creation_homeland *homeland =
      character_creation_homeland_for_region(region);

  return homeland != NULL ? homeland->language : LANG_COMMON;
}

const char *get_region_language_name(int region)
{
  int language = get_region_language(region);

  if (language < LANG_COMMON || language >= NUM_LANGUAGES)
    return "Unknown";

  return languages[language];
}

bool is_furry(int race)
{
  switch (race)
  {
  case RACE_TABAXI:
  case RACE_WEMIC:
  case LEGACY_RACE_MINOTAUR:
    return true;
  }
  return false;
}

bool has_horns(int race)
{
  switch (race)
  {
  case RACE_TIEFLING:
  case LEGACY_RACE_MINOTAUR:
    return true;
  }
  return false;
}

bool has_scales(int race)
{
  switch (race)
  {
  case RACE_DRAGONBORN:
  case RACE_YUAN_TI:
  case LEGACY_RACE_AURAK_DRACONIAN:
  case LEGACY_RACE_BAAZ_DRACONIAN:
  case LEGACY_RACE_BOZAK_DRACONIAN:
  case LEGACY_RACE_KAPAK_DRACONIAN:
  case LEGACY_RACE_SIVAK_DRACONIAN:
    return true;
  }
  return false;
}

bool race_has_no_hair(int race)
{
  switch (race)
  {
  case RACE_DRAGONBORN:
  case RACE_HALF_ILLITHID:
  case RACE_YUAN_TI:
  case RACE_MYCONID:
  case LEGACY_RACE_AURAK_DRACONIAN:
  case LEGACY_RACE_BAAZ_DRACONIAN:
  case LEGACY_RACE_BOZAK_DRACONIAN:
  case LEGACY_RACE_KAPAK_DRACONIAN:
  case LEGACY_RACE_SIVAK_DRACONIAN:
    return true;
  }
  return false;
}

int compare_races(const void *x, const void *y)
{
  int a = *(const int *)x, b = *(const int *)y;

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
  text_line(ch, "Races of Luminari", 80, '-', '-');
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
  case 0:
    modifier = 2;
    snprintf(ripeness, sizeof(ripeness), "rather unripe");
    break;
  case 1:
    modifier = 3;
    snprintf(ripeness, sizeof(ripeness), "barely ripe");
    break;
  case 2:
    modifier = 4;
    snprintf(ripeness, sizeof(ripeness), "nicely ripened");
    break;
  case 3:
    modifier = 5;
    snprintf(ripeness, sizeof(ripeness), "very well ripened");
    break;
  default:
    modifier = 6;
    snprintf(ripeness, sizeof(ripeness), "perfectly ripened");
    break;
  }

  switch (rand_number(0, 18))
  {
  case 0:
    bonus = APPLY_AC_NEW;
    break;
  case 1:
    bonus = APPLY_STR;
    break;
  case 2:
    bonus = APPLY_DEX;
    break;
  case 3:
    bonus = APPLY_CON;
    break;
  case 4:
    bonus = APPLY_INT;
    break;
  case 5:
    bonus = APPLY_WIS;
    break;
  case 6:
    bonus = APPLY_CHA;
    break;
  case 7:
    bonus = APPLY_DAMROLL;
    modifier /= 2;
    break;
  case 8:
    bonus = APPLY_HITROLL;
    modifier /= 2;
    break;
  case 9:
    bonus = APPLY_ENCUMBRANCE;
    break;
  case 10:
    bonus = APPLY_HIT;
    modifier *= 10;
    break;
  case 11:
    bonus = APPLY_MOVE;
    modifier *= 100;
    break;
  case 12:
    bonus = APPLY_HP_REGEN;
    break;
  case 13:
    bonus = APPLY_MV_REGEN;
    break;
  case 14:
    bonus = APPLY_PSP;
    modifier *= 5;
    break;
  case 15:
    bonus = APPLY_PSP_REGEN;
    break;
  case 16:
    bonus = APPLY_SAVING_FORT;
    break;
  case 17:
    bonus = APPLY_SAVING_REFL;
    break;
  case 18:
    bonus = APPLY_SAVING_WILL;
    break;
  }

  obj = read_object(FORAGE_FOOD_ITEM_VNUM, VIRTUAL);

  if (!obj)
  {
    send_to_char(ch,
                 "The forage food item prototype was not found. Please inform a staff with code "
                 "ERRFOR00%d.\r\n",
                 type);
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
  snprintf(food_desc, sizeof(food_desc), "Some %s %s lie here.", ripeness,
           apply_type_food_names[bonus]);
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

  send_to_char(ch,
               "You attempt to scrounge for supplies... Roll %d + %d (nature skill) for total %d "
               "vs. dc %d\r\n",
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
    case 1:
      award_magic_armor(ch, grade, ITEM_WEAR_BODY);
      break;
    case 2:
      award_magic_armor(ch, grade, ITEM_WEAR_ARMS);
      break;
    case 3:
      award_magic_armor(ch, grade, ITEM_WEAR_LEGS);
      break;
    case 4:
      award_magic_armor(ch, grade, ITEM_WEAR_HEAD);
      break;
    case 5:
      award_magic_armor(ch, grade, ITEM_WEAR_SHIELD);
      break;
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
