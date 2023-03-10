/**************************************************************************
 *  File: players.c                                    Part of LuminariMUD *
 *  Usage: Player loading/saving and utility routines.                     *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "handler.h"
#include "pfdefaults.h"
#include "dg_scripts.h"
#include "comm.h"
#include "interpreter.h"
#include "mysql.h"
#include "genolc.h"     /* for strip_cr */
#include "config.h"     /* for pclean_criteria[] */
#include "dg_scripts.h" /* To enable saving of player variables to disk */
#include "quest.h"
#include "spells.h"
#include "clan.h"
#include "mud_event.h"
#include "craft.h" // crafting (auto craft quest inits)
#include "spell_prep.h"
#include "alchemy.h"
#include "templates.h"
#include "premadebuilds.h"
#include "missions.h"

#define LOAD_HIT 0
#define LOAD_PSP 1
#define LOAD_MOVE 2
#define LOAD_STRENGTH 3

#define PT_PNAME(i) (player_table[(i)].name)
#define PT_IDNUM(i) (player_table[(i)].id)
#define PT_LEVEL(i) (player_table[(i)].level)
#define PT_FLAGS(i) (player_table[(i)].flags)
#define PT_LLAST(i) (player_table[(i)].last)
#define PT_PCLAN(i) (player_table[(i)].clan)

/* 'global' vars defined here and used externally */
/** @deprecated Since this file really is basically a functional extension
 * of the database handling in db.c, until the day that the mud is broken
 * down to be less monolithic, I don't see why the following should be defined
 * anywhere but there.
struct player_index_element *player_table = NULL;
int top_of_p_table = 0;
int top_of_p_file = 0;
long top_idnum = 0;
 */

/* local functions */
static void load_dr(FILE *fl, struct char_data *ch);
static void load_events(FILE *fl, struct char_data *ch);
static void load_affects(FILE *fl, struct char_data *ch);
static void load_skills(FILE *fl, struct char_data *ch);
static void load_feats(FILE *fl, struct char_data *ch);
static void load_class_feat_points(FILE *fl, struct char_data *ch);
static void load_epic_class_feat_points(FILE *fl, struct char_data *ch);
static void load_skill_focus(FILE *fl, struct char_data *ch);
static void load_abilities(FILE *fl, struct char_data *ch);
static void load_favored_enemy(FILE *fl, struct char_data *ch);
static void load_spec_abil(FILE *fl, struct char_data *ch);
static void load_warding(FILE *fl, struct char_data *ch);
static void load_class_level(FILE *fl, struct char_data *ch);
static void load_coord_location(FILE *fl, struct char_data *ch);
static void load_praying(FILE *fl, struct char_data *ch);
static void load_praying_metamagic(FILE *fl, struct char_data *ch);
static void load_prayed(FILE *fl, struct char_data *ch);
static void load_prayed_metamagic(FILE *fl, struct char_data *ch);
static void load_praytimes(FILE *fl, struct char_data *ch);
static void load_quests(FILE *fl, struct char_data *ch);
static void load_HMVS(struct char_data *ch, const char *line, int mode);
static void write_aliases_ascii(FILE *file, struct char_data *ch);
static void read_aliases_ascii(FILE *file, struct char_data *ch, int count);
static void load_bombs(FILE *fl, struct char_data *ch);
static void load_judgements(FILE *fl, struct char_data *ch);
static void load_potions(FILE *fl, struct char_data *ch);
static void load_scrolls(FILE *fl, struct char_data *ch);
static void load_wands(FILE *fl, struct char_data *ch);
static void load_staves(FILE *fl, struct char_data *ch);
static void load_discoveries(FILE *fl, struct char_data *ch);
void save_char_pets(struct char_data *ch);
static void load_mercies(FILE *fl, struct char_data *ch);
static void load_cruelties(FILE *fl, struct char_data *ch);
static void load_buffs(FILE *fl, struct char_data *ch);
static void load_languages(FILE *fl, struct char_data *ch);

    // external functions
    void autoroll_mob(struct char_data *mob, bool realmode, bool summoned);

/* New version to build player index for ASCII Player Files. Generate index
 * table for the player file. */
void build_player_index(void)
{
  int rec_count = 0, i, nr;
  FILE *plr_index;
  char index_name[40], line[MEDIUM_STRING] = {'\0'}, bits[64];
  char arg2[80];

  snprintf(index_name, sizeof(index_name), "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(plr_index = fopen(index_name, "r")))
  {
    top_of_p_table = -1;
    log("No player index file!  First new char will be IMP!");
    return;
  }

  while (get_line(plr_index, line))
    if (*line != '~')
      rec_count++;
  rewind(plr_index);

  if (rec_count == 0)
  {
    player_table = NULL;
    top_of_p_table = -1;
    return;
  }

  CREATE(player_table, struct player_index_element, rec_count);

  /* zusuk was here - trying to init the player index, probably not smart/necessary  */
  /*
  for (i = 0; i < rec_count; i++) {
    player_table[i].name = "NoName";
    player_table[i].id = 0;
    player_table[i].level = 1;
    player_table[i].flags = 0;
    player_table[i].last = 0;
    player_table[i].clan = NO_CLAN;
  }
   */
  /**/

  for (i = 0; i < rec_count; i++)
  {
    get_line(plr_index, line);
    if ((nr = sscanf(line, "%ld %s %d %s %ld %d", &player_table[i].id, arg2,
                     &player_table[i].level, bits, (long *)&player_table[i].last, &player_table[i].clan)) != 6)
    {
      if ((nr = sscanf(line, "%ld %s %d %s %ld", &player_table[i].id, arg2,
                       &player_table[i].level, bits, (long *)&player_table[i].last)) != 5)
      {
        log("SYSERR: Invalid line in player index (%s)", line);
        continue;
      }
      player_table[i].clan = NO_CLAN;
    }
    CREATE(player_table[i].name, char, strlen(arg2) + 1);
    strcpy(player_table[i].name, arg2);
    player_table[i].flags = asciiflag_conv(bits);
    top_idnum = MAX(top_idnum, player_table[i].id);
  }

  fclose(plr_index);
  top_of_p_file = top_of_p_table = i - 1;
}

/* Create a new entry in the in-memory index table for the player file. If the
 * name already exists, by overwriting a deleted character, then we re-use the
 * old position. */
int create_entry(char *name)
{
  int i, pos;

  if (top_of_p_table == -1)
  { /* no table */
    pos = top_of_p_table = 0;
    CREATE(player_table, struct player_index_element, 1);
  }
  else if ((pos = get_ptable_by_name(name)) == -1)
  { /* new name */
    i = ++top_of_p_table + 1;

    RECREATE(player_table, struct player_index_element, i);
    pos = top_of_p_table;
  }

  CREATE(player_table[pos].name, char, strlen(name) + 1);

  /* copy lowercase equivalent of name to table field */
  for (i = 0; (player_table[pos].name[i] = LOWER(name[i])); i++)
    /* Nothing */;

  /* clear the bitflag and clan in case we have garbage data */
  player_table[pos].flags = 0;
  player_table[pos].clan = NO_CLAN;

  return (pos);
}

/* Remove an entry from the in-memory player index table.               *
 * Requires the 'pos' value returned by the get_ptable_by_name function */
void remove_player_from_index(int pos)
{
  int i;

  if (pos < 0 || pos > top_of_p_table)
    return;

  /* We only need to free the name string */
  free(PT_PNAME(pos));

  /* Move every other item in the list down the index */
  for (i = pos + 1; i <= top_of_p_table; i++)
  {
    PT_PNAME(i - 1) = PT_PNAME(i);
    PT_IDNUM(i - 1) = PT_IDNUM(i);
    PT_LEVEL(i - 1) = PT_LEVEL(i);
    PT_FLAGS(i - 1) = PT_FLAGS(i);
    PT_LLAST(i - 1) = PT_LLAST(i);
    PT_PCLAN(i - 1) = PT_PCLAN(i);
  }
  PT_PNAME(top_of_p_table) = NULL;

  /* Reduce the index table counter */
  top_of_p_table--;

  /* And reduce the size of the table */
  if (top_of_p_table >= 0)
    RECREATE(player_table, struct player_index_element, (top_of_p_table + 1));
  else
  {
    free(player_table);
    player_table = NULL;
  }
}

/* This function necessary to save a separate ASCII player index */
void save_player_index(void)
{
  int i = 0;
  char index_name[50] = {'\0'}, bits[64] = {'\0'};
  FILE *index_file;

  snprintf(index_name, sizeof(index_name), "%s%s", LIB_PLRFILES, INDEX_FILE);
  if (!(index_file = fopen(index_name, "w")))
  {
    log("SYSERR: Could not write player index file");
    return;
  }

  for (i = 0; i <= top_of_p_table; i++)
    if (*player_table[i].name)
    {
      sprintascii(bits, player_table[i].flags);
      if (player_table[i].clan == NO_CLAN)
      {
        fprintf(index_file, "%ld %s %d %s %ld\n", player_table[i].id, player_table[i].name,
                player_table[i].level, *bits ? bits : "0",
                (long)player_table[i].last);
      }
      else
      {
        fprintf(index_file, "%ld %s %d %s %ld %d\n", player_table[i].id,
                player_table[i].name, player_table[i].level, *bits ? bits : "0",
                (long)player_table[i].last, player_table[i].clan);
      }
    }
  fprintf(index_file, "~\n");

  fclose(index_file);
}

void free_player_index(void)
{
  int tp;

  if (!player_table)
    return;

  for (tp = 0; tp <= top_of_p_table; tp++)
    if (player_table[tp].name)
      free(player_table[tp].name);

  free(player_table);
  player_table = NULL;
  top_of_p_table = 0;
}

long get_ptable_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (i);

  return (-1);
}

long get_id_by_name(const char *name)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (!str_cmp(player_table[i].name, name))
      return (player_table[i].id);

  return (-1);
}

char *get_name_by_id(long id)
{
  int i;

  for (i = 0; i <= top_of_p_table; i++)
    if (player_table[i].id == id)
      return (player_table[i].name);

  return (NULL);
}

/* Stuff related to the save/load player system. */

/* New load_char reads ASCII Player Files. Load a char, TRUE if loaded, FALSE
 * if not. */
int load_char(const char *name, struct char_data *ch)
{
  int id, i, j;
  FILE *fl;
  char filename[40];
  char buf[128], buf2[128], line[MAX_INPUT_LENGTH + 1], tag[6];
  char f1[128], f2[128], f3[128], f4[128];
  trig_data *t = NULL;
  trig_rnum t_rnum = NOTHING;

  if ((id = get_ptable_by_name(name)) < 0)
    return (-1);
  else
  {
    if (!get_filename(filename, sizeof(filename), PLR_FILE, player_table[id].name))
      return (-1);
    if (!(fl = fopen(filename, "r")))
    {
      mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Couldn't open player file %s", filename);
      return (-1);
    }

    /* Character initializations. Necessary to keep some things straight. */
    ch->affected = NULL;
    for (i = 0; i < MAX_CLASSES; i++)
    {
      CLASS_LEVEL(ch, i) = 0;
      GET_SPEC_ABIL(ch, i) = 0;
    }
    for (i = 0; i < MAX_ENEMIES; i++)
      GET_FAVORED_ENEMY(ch, i) = 0;
    for (i = 0; i < MAX_WARDING; i++)
      GET_WARDING(ch, i) = 0;
    for (i = 1; i < MAX_SKILLS; i++)
      GET_SKILL(ch, i) = 0;
    for (i = 1; i <= MAX_ABILITIES; i++)
      GET_ABILITY(ch, i) = 0;
    for (i = 0; i < NUM_FEATS; i++)
      SET_FEAT(ch, i, 0);
    for (i = 0; i < (END_GENERAL_ABILITIES + 1); i++)
      for (j = 0; j < NUM_SKFEATS; j++)
        ch->player_specials->saved.skill_focus[i][j] = 0;
    for (i = 0; i < NUM_CFEATS; i++)
      for (j = 0; j < FT_ARRAY_MAX; j++)
        ch->char_specials.saved.combat_feats[i][j] = 0;

    for (i = 0; i < NUM_SFEATS; i++)
      ch->char_specials.saved.school_feats[i] = 0;

    for (i = 0; i < NUM_CLASSES; i++)
    {
      GET_CLASS_FEATS(ch, i) = 0;
      GET_EPIC_CLASS_FEATS(ch, i) = 0;
    }
    GET_FEAT_POINTS(ch) = 0;
    GET_EPIC_FEAT_POINTS(ch) = 0;
    destroy_spell_prep_queue(ch);
    destroy_innate_magic_queue(ch);
    destroy_spell_collection(ch);
    destroy_known_spells(ch);
    GET_REAL_SIZE(ch) = PFDEF_SIZE;
    IS_MORPHED(ch) = PFDEF_MORPHED;
    GET_SEX(ch) = PFDEF_SEX;
    GET_CLASS(ch) = PFDEF_CLASS;
    GET_LEVEL(ch) = PFDEF_LEVEL;
    GET_HEIGHT(ch) = PFDEF_HEIGHT;
    GET_WEIGHT(ch) = PFDEF_WEIGHT;
    GET_ALIGNMENT(ch) = PFDEF_ALIGNMENT;
    for (i = 0; i < NUM_OF_SAVING_THROWS; i++)
      GET_REAL_SAVE(ch, i) = PFDEF_SAVETHROW;
    for (i = 0; i < NUM_DAM_TYPES; i++)
      GET_REAL_RESISTANCES(ch, i) = PFDEF_RESISTANCES;
    GET_LOADROOM(ch) = PFDEF_LOADROOM;
    GET_INVIS_LEV(ch) = PFDEF_INVISLEV;
    GET_FREEZE_LEV(ch) = PFDEF_FREEZELEV;
    GET_WIMP_LEV(ch) = PFDEF_WIMPLEV;
    GET_COND(ch, HUNGER) = PFDEF_HUNGER;
    GET_COND(ch, THIRST) = PFDEF_THIRST;
    GET_COND(ch, DRUNK) = PFDEF_DRUNK;
    GET_BAD_PWS(ch) = PFDEF_BADPWS;
    GET_PRACTICES(ch) = PFDEF_PRACTICES;
    GET_TRAINS(ch) = PFDEF_TRAINS;
    GET_BOOSTS(ch) = PFDEF_BOOSTS;
    GET_SPECIALTY_SCHOOL(ch) = PFDEF_SPECIALTY_SCHOOL;
    GET_PREFERRED_ARCANE(ch) = PFDEF_PREFERRED_ARCANE;
    GET_PREFERRED_DIVINE(ch) = PFDEF_PREFERRED_DIVINE;
    GET_1ST_RESTRICTED_SCHOOL(ch) = PFDEF_RESTRICTED_SCHOOL_1;
    GET_2ND_RESTRICTED_SCHOOL(ch) = PFDEF_RESTRICTED_SCHOOL_2;
    GET_1ST_DOMAIN(ch) = PFDEF_DOMAIN_1;
    GET_2ND_DOMAIN(ch) = PFDEF_DOMAIN_2;
    GET_GOLD(ch) = PFDEF_GOLD;
    GET_BANK_GOLD(ch) = PFDEF_BANK;
    GET_EXP(ch) = PFDEF_EXP;
    GET_REAL_HITROLL(ch) = PFDEF_HITROLL;
    GET_REAL_DAMROLL(ch) = PFDEF_DAMROLL;
    GET_REAL_AC(ch) = PFDEF_AC;
    ch->real_abils.str_add = PFDEF_STRADD;
    GET_REAL_STR(ch) = PFDEF_STR;
    GET_REAL_CON(ch) = PFDEF_CON;
    GET_REAL_DEX(ch) = PFDEF_DEX;
    GET_REAL_INT(ch) = PFDEF_INT;
    GET_REAL_WIS(ch) = PFDEF_WIS;
    GET_REAL_CHA(ch) = PFDEF_CHA;
    GET_DR_MOD(ch) = 0;
    GET_HIT(ch) = PFDEF_HIT;
    GET_REAL_MAX_HIT(ch) = PFDEF_MAXHIT;
    GET_PSP(ch) = PFDEF_PSP;
    GET_REAL_MAX_PSP(ch) = PFDEF_MAXPSP;
    GET_MOVE(ch) = PFDEF_MOVE;
    GET_REAL_SPELL_RES(ch) = PFDEF_SPELL_RES;
    GET_REAL_MAX_MOVE(ch) = PFDEF_MAXMOVE;
    GET_OLC_ZONE(ch) = PFDEF_OLC;
    GET_PAGE_LENGTH(ch) = PFDEF_PAGELENGTH;
    GET_SCREEN_WIDTH(ch) = PFDEF_SCREENWIDTH;
    GET_ALIASES(ch) = NULL;
    SITTING(ch) = NULL;
    NEXT_SITTING(ch) = NULL;
    GET_QUESTPOINTS(ch) = PFDEF_QUESTPOINTS;

    for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    { /* loop through all the character's quest slots */
      GET_QUEST_COUNTER(ch, i) = PFDEF_QUESTCOUNT;
      GET_QUEST(ch, i) = PFDEF_CURRQUEST;
    }

    GET_HP_REGEN(ch) = 0;
    GET_MV_REGEN(ch) = 0;
    GET_PSP_REGEN(ch) = 0;
    GET_ENCUMBRANCE_MOD(ch) = 0;
    GET_FAST_HEALING_MOD(ch) = 0;

    GET_ELDRITCH_ESSENCE(ch) = 0;
    GET_ELDRITCH_SHAPE(ch) = 0;

    GET_NUM_QUESTS(ch) = PFDEF_COMPQUESTS;
    GET_LAST_MOTD(ch) = PFDEF_LASTMOTD;
    GET_LAST_NEWS(ch) = PFDEF_LASTNEWS;
    GET_REAL_RACE(ch) = PFDEF_RACE;
    GET_AUTOCQUEST_VNUM(ch) = PFDEF_AUTOCQUEST_VNUM;
    GET_AUTOCQUEST_MAKENUM(ch) = PFDEF_AUTOCQUEST_MAKENUM;
    GET_AUTOCQUEST_QP(ch) = PFDEF_AUTOCQUEST_QP;
    GET_AUTOCQUEST_EXP(ch) = PFDEF_AUTOCQUEST_EXP;
    GET_AUTOCQUEST_GOLD(ch) = PFDEF_AUTOCQUEST_GOLD;
    GET_AUTOCQUEST_DESC(ch) = NULL;
    GET_AUTOCQUEST_MATERIAL(ch) = PFDEF_AUTOCQUEST_MATERIAL;
    GET_CURRENT_MISSION(ch) = 0;
    GET_MISSION_CREDITS(ch) = 0;
    GET_MISSION_STANDING(ch) = 0;
    GET_MISSION_FACTION(ch) = 0;
    GET_MISSION_REP(ch) = 0;
    GET_MISSION_EXP(ch) = 0;
    GET_MISSION_COOLDOWN(ch) = 0;
    GET_MISSION_DIFFICULTY(ch) = 0;
    GET_MISSION_NPC_NAME_NUM(ch) = 0;
    GET_FACTION_STANDING(ch, FACTION_ADVENTURERS) = 0;
    GET_SALVATION_ROOM(ch) = NOWHERE;
    GET_SALVATION_NAME(ch) = NULL;
    GUARDING(ch) = NULL;
    GET_TOTAL_AOO(ch) = 0;
    GET_ACCOUNT_NAME(ch) = NULL;
    LEVELUP(ch) = NULL;
    CNDNSD(ch) = NULL;
    GET_DR(ch) = NULL;
    GET_WALKTO_LOC(ch) = 0;
    GET_TEMPLATE(ch) = PFDEF_TEMPLATE;
    GET_PREMADE_BUILD_CLASS(ch) = PFDEF_PREMADE_BUILD;
    init_spell_prep_queue(ch);
    init_innate_magic_queue(ch);
    init_collection_queue(ch);
    init_known_spells(ch);
    GET_DIPTIMER(ch) = PFDEF_DIPTIMER;
    GET_CLAN(ch) = PFDEF_CLAN;
    GET_CLANRANK(ch) = PFDEF_CLANRANK;
    GET_CLANPOINTS(ch) = PFDEF_CLANPOINTS;
    GET_DISGUISE_RACE(ch) = PFDEF_RACE;
    GET_DISGUISE_STR(ch) = 0;
    GET_DISGUISE_CON(ch) = 0;
    GET_DISGUISE_DEX(ch) = 0;
    GET_DISGUISE_AC(ch) = 0;
    EFREETI_MAGIC_USES(ch) = 0;
    EFREETI_MAGIC_TIMER(ch) = 0;
    LAUGHING_TOUCH_USES(ch) = 0;
    LAUGHING_TOUCH_TIMER(ch) = 0;
    FLEETING_GLANCE_USES(ch) = 0;
    FLEETING_GLANCE_TIMER(ch) = 0;
    FEY_SHADOW_WALK_USES(ch) = 0;
    FEY_SHADOW_WALK_TIMER(ch) = 0;
    DRAGON_MAGIC_USES(ch) = 0;
    DRAGON_MAGIC_TIMER(ch) = 0;
    IS_CASTING(ch) = 0;
    CASTING_TIME(ch) = 0;
    CASTING_TCH(ch) = NULL;
    CASTING_TOBJ(ch) = NULL;
    CASTING_SPELLNUM(ch) = 0;
    CASTING_METAMAGIC(ch) = 0;
    CASTING_CLASS(ch) = 0;
    for (i = 0; i < MAX_BUFFS; i++)
    {
      GET_BUFF(ch, i, 0) = 0;
      GET_BUFF(ch, i, 1) = 0;
    }
    GET_BUFF_TIMER(ch) = 0;
    GET_CURRENT_BUFF_SLOT(ch) = 0;
    IS_BUFFING(ch) = false;
    for (i = 0; i < MAX_PERFORMANCE_VARS; i++)
      GET_PERFORMANCE_VAR(ch, i) = 0;
    GET_PERFORMING(ch) = -1; /* 0 is an actual performance */
    PIXIE_DUST_USES(ch) = 0;
    PIXIE_DUST_TIMER(ch) = 0;
    GRAVE_TOUCH_USES(ch) = PFDEF_GRAVE_TOUCH_USES;
    GRAVE_TOUCH_TIMER(ch) = PFDEF_GRAVE_TOUCH_TIMER;
    GRASP_OF_THE_DEAD_USES(ch) = PFDEF_GRASP_OF_THE_DEAD_USES;
    GRASP_OF_THE_DEAD_TIMER(ch) = PFDEF_GRASP_OF_THE_DEAD_TIMER;
    INCORPOREAL_FORM_USES(ch) = PFDEF_INCORPOREAL_FORM_USES;
    INCORPOREAL_FORM_TIMER(ch) = PFDEF_INCORPOREAL_FORM_TIMER;
    HAS_SET_STATS_STUDY(ch) = PFDEF_HAS_SET_STATS_STUDY;
    GET_BLOODLINE_SUBTYPE(ch) = PFDEF_SORC_BLOODLINE_SUBTYPE;
    NEW_ARCANA_SLOT(ch, 0) = NEW_ARCANA_SLOT(ch, 1) = NEW_ARCANA_SLOT(ch, 2) = NEW_ARCANA_SLOT(ch, 3) = 0;
    GET_DRAGONBORN_ANCESTRY(ch) = 0;
    HIGH_ELF_CANTRIP(ch) = 0;
    for (i = 0; i < AF_ARRAY_MAX; i++)
      AFF_FLAGS(ch)[i] = PFDEF_AFFFLAGS;
    for (i = 0; i < PM_ARRAY_MAX; i++)
      PLR_FLAGS(ch)[i] = PFDEF_PLRFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
      PRF_FLAGS(ch)[i] = PFDEF_PREFFLAGS;
    for (i = 0; i < PR_ARRAY_MAX; i++)
    {
      EVOLUTIONS(ch)[i] = 0;
      KNOWN_EVOLUTIONS(ch)[i] = 0;
    }
    for (i = 0; i < MAX_BOMBS_ALLOWED; i++)
      GET_BOMB(ch, i) = 0;
    for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
      KNOWS_DISCOVERY(ch, i) = 0;
    GET_GRAND_DISCOVERY(ch) = 0;
    for (i = 0; i < NUM_PALADIN_MERCIES; i++)
      KNOWS_MERCY(ch, i) = 0;
    for (i = 0; i < STAFF_RAN_EVENTS_VAR; i++)
      STAFFRAN_PVAR(ch, i) = PFDEF_STAFFRAN_EVENT_VAR;
    GET_PSIONIC_ENERGY_TYPE(ch) = PFDEF_PSIONIC_ENERGY_TYPE;
    GET_DEITY(ch) = 0;
    for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
      KNOWS_CRUELTY(ch, i) = 0;
    ch->player_specials->saved.fiendish_boons = 0;
    ch->player_specials->saved.channel_energy_type = 0;

    for (i = 0; i < NUM_LANGUAGES; i++)
      ch->player_specials->saved.languages_known[i] = 0;
    SPEAKING(ch) = LANG_COMMON;
    GET_REGION(ch) = REGION_NONE;

    GET_PC_DESCRIPTOR_1(ch) = 0;
    GET_PC_ADJECTIVE_1(ch) = 0;
    GET_PC_DESCRIPTOR_2(ch) = 0;
    GET_PC_ADJECTIVE_2(ch) = 0;

    ch->sticky_bomb[0] = 0;
    ch->sticky_bomb[1] = 0;
    ch->sticky_bomb[2] = 0;
    ch->mission_owner = 0;
    ch->dead = 0;
    ch->confuser_idnum = 0;
    ch->preserve_organs_procced = 0;
    ch->mute_equip_messages = 0;

    GET_HOLY_WEAPON_TYPE(ch) = PFDEF_HOLY_WEAPON_TYPE;
    for (i = 0; i < MAX_SPELLS; i++)
    {
      STORED_POTIONS(ch, i) = STORED_SCROLLS(ch, i) = STORED_WANDS(ch, i) = STORED_STAVES(ch, i) = 0;
    }

    /* finished inits, start loading from file */

    while (get_line(fl, line))
    {
      tag_argument(line, tag);

      switch (*tag)
      {
      case 'A':
        if (!strcmp(tag, "Ablt"))
          load_abilities(fl, ch);
        else if (!strcmp(tag, "Ac  "))
          GET_REAL_AC(ch) = atoi(line);
        else if (!strcmp(tag, "Acct"))
        {
          GET_ACCOUNT_NAME(ch) = strdup(line);
          if (ch->desc && ch->desc->account == NULL)
          {
            CREATE(ch->desc->account, struct account_data, 1);
            for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
              ch->desc->account->character_names[i] = NULL;

            load_account(GET_ACCOUNT_NAME(ch), ch->desc->account);
          }
        }
        else if (!strcmp(tag, "Act "))
        {
          if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
          {
            PLR_FLAGS(ch)
            [0] = asciiflag_conv(f1);
            PLR_FLAGS(ch)
            [1] = asciiflag_conv(f2);
            PLR_FLAGS(ch)
            [2] = asciiflag_conv(f3);
            PLR_FLAGS(ch)
            [3] = asciiflag_conv(f4);
          }
          else
            PLR_FLAGS(ch)
          [0] = asciiflag_conv(line);
        }
        else if (!strcmp(tag, "Aff "))
        {
          if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
          {
            AFF_FLAGS(ch)
            [0] = asciiflag_conv(f1);
            AFF_FLAGS(ch)
            [1] = asciiflag_conv(f2);
            AFF_FLAGS(ch)
            [2] = asciiflag_conv(f3);
            AFF_FLAGS(ch)
            [3] = asciiflag_conv(f4);
          }
          else
            AFF_FLAGS(ch)
          [0] = asciiflag_conv(line);
        }
        if (!strcmp(tag, "Affs"))
          load_affects(fl, ch);
        else if (!strcmp(tag, "Alin"))
          GET_ALIGNMENT(ch) = atoi(line);
        else if (!strcmp(tag, "Alis"))
          read_aliases_ascii(fl, ch, atoi(line));
        break;

      case 'B':
        if (!strcmp(tag, "Badp"))
          GET_BAD_PWS(ch) = atoi(line);
        else if (!strcmp(tag, "Bane"))
          GET_BANE_TARGET_TYPE(ch) = atoi(line);
        else if (!strcmp(tag, "BGrd"))
          ch->player.background = fread_string(fl, buf2);
        else if (!strcmp(tag, "Bomb"))
          load_bombs(fl, ch);
        else if (!strcmp(tag, "Bost"))
          GET_BOOSTS(ch) = atoi(line);
        else if (!strcmp(tag, "Bank"))
          GET_BANK_GOLD(ch) = atoi(line);
        else if (!strcmp(tag, "Brth"))
          ch->player.time.birth = atol(line);
        else if (!strcmp(tag, "Buff"))
          load_buffs(fl, ch);
        break;

      case 'C':
        if (!strcmp(tag, "CbFt"))
        {
          sscanf(line, "%d %s %s %s %s", &i, f1, f2, f3, f4);
          if (i < 0 || i >= NUM_CFEATS)
          {
            log("load_char: %s combat feat record out of range: %s", GET_NAME(ch), line);
            break;
          }
          ch->char_specials.saved.combat_feats[i][0] = asciiflag_conv(f1);
          ch->char_specials.saved.combat_feats[i][1] = asciiflag_conv(f2);
          ch->char_specials.saved.combat_feats[i][2] = asciiflag_conv(f3);
          ch->char_specials.saved.combat_feats[i][3] = asciiflag_conv(f4);
        }
        else if (!strcmp(tag, "Cfpt"))
          load_class_feat_points(fl, ch);
        else if (!strcmp(tag, "Cha "))
          GET_REAL_CHA(ch) = atoi(line);
        else if (!strcmp(tag, "Clas"))
          GET_CLASS(ch) = atoi(line);
        else if (!strcmp(tag, "ClkT"))
          GET_SETCLOAK_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "Coll"))
          load_spell_collection(fl, ch);
        else if (!strcmp(tag, "Con "))
          GET_REAL_CON(ch) = atoi(line);
        else if (!strcmp(tag, "CLoc"))
          load_coord_location(fl, ch);
        else if (!strcmp(tag, "CLvl"))
          load_class_level(fl, ch);
        else if (!strcmp(tag, "Cln "))
          GET_CLAN(ch) = atoi(line);
        else if (!strcmp(tag, "Clrk"))
          GET_CLANRANK(ch) = atoi(line);
        else if (!strcmp(tag, "Clty"))
          load_cruelties(fl, ch);
        else if (!strcmp(tag, "CPts"))
          GET_CLANPOINTS(ch) = atoi(line);
        else if (!strcmp(tag, "Cvnm"))
          GET_AUTOCQUEST_VNUM(ch) = atoi(line);
        else if (!strcmp(tag, "Cmnm"))
          GET_AUTOCQUEST_MAKENUM(ch) = atoi(line);
        else if (!strcmp(tag, "Cqps"))
          GET_AUTOCQUEST_QP(ch) = atoi(line);
        else if (!strcmp(tag, "Cexp"))
          GET_AUTOCQUEST_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "Cgld"))
          GET_AUTOCQUEST_GOLD(ch) = atoi(line);
        else if (!strcmp(tag, "Cdsc"))
          GET_AUTOCQUEST_DESC(ch) = strdup(line);
        else if (!strcmp(tag, "Cmat"))
          GET_AUTOCQUEST_MATERIAL(ch) = atoi(line);
        else if (!strcmp(tag, "ChEn"))
          ch->player_specials->saved.channel_energy_type = atoi(line);

        break;

      case 'D':
        if (!strcmp(tag, "DmgR"))
          load_dr(fl, ch);
        else if (!strcmp(tag, "Desc"))
          ch->player.description = fread_string(fl, buf2);
        else if (!strcmp(tag, "DrgB"))
          GET_DRAGONBORN_ANCESTRY(ch) = atoi(line);
        else if (!strcmp(tag, "DAd1"))
          GET_PC_ADJECTIVE_1(ch) = atoi(line);
        else if (!strcmp(tag, "DAd2"))
          GET_PC_ADJECTIVE_2(ch) = atoi(line);
        else if (!strcmp(tag, "DDs1"))
          GET_PC_DESCRIPTOR_1(ch) = atoi(line);
        else if (!strcmp(tag, "DDs2"))
          GET_PC_DESCRIPTOR_2(ch) = atoi(line);
        else if (!strcmp(tag, "Dex "))
          GET_REAL_DEX(ch) = atoi(line);
        else if (!strcmp(tag, "DRMd"))
          GET_DR_MOD(ch) = atoi(line);
        else if (!strcmp(tag, "Drnk"))
          GET_COND(ch, DRUNK) = atoi(line);
        else if (!strcmp(tag, "Drol"))
          GET_REAL_DAMROLL(ch) = atoi(line);
        else if (!strcmp(tag, "Disc"))
          load_discoveries(fl, ch);
        else if (!strcmp(tag, "DipT"))
          GET_DIPTIMER(ch) = atoi(line);
        else if (!strcmp(tag, "DRac"))
          GET_DISGUISE_RACE(ch) = atoi(line);
        else if (!strcmp(tag, "DDex"))
          GET_DISGUISE_DEX(ch) = atoi(line);
        else if (!strcmp(tag, "DStr"))
          GET_DISGUISE_STR(ch) = atoi(line);
        else if (!strcmp(tag, "DCon"))
          GET_DISGUISE_CON(ch) = atoi(line);
        else if (!strcmp(tag, "DAC "))
          GET_DISGUISE_AC(ch) = atoi(line);
        else if (!strcmp(tag, "Dom1"))
          GET_1ST_DOMAIN(ch) = atoi(line);
        else if (!strcmp(tag, "Dom2"))
          GET_2ND_DOMAIN(ch) = atoi(line);
        else if (!strcmp(tag, "DrMU"))
          DRAGON_MAGIC_USES(ch) = atoi(line);
        else if (!strcmp(tag, "DrMT"))
          DRAGON_MAGIC_TIMER(ch) = atoi(line);
        break;

      case 'E':
        if (!strcmp(tag, "Exp "))
          GET_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "Evnt"))
          load_events(fl, ch);
        else if (!strcmp(tag, "Evol"))
        {
          if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
          {
            EVOLUTIONS(ch)[0] = asciiflag_conv(f1);
            EVOLUTIONS(ch)[1] = asciiflag_conv(f2);
            EVOLUTIONS(ch)[2] = asciiflag_conv(f3);
            EVOLUTIONS(ch)[3] = asciiflag_conv(f4);
          }
          else
            EVOLUTIONS(ch)[0] = asciiflag_conv(line);
        }
        else if (!strcmp(tag, "Ecfp"))
          load_epic_class_feat_points(fl, ch);
        else if (!strcmp(tag, "Efpt"))
          GET_EPIC_FEAT_POINTS(ch) = atoi(line);
        else if (!strcmp(tag, "EfMU"))
          EFREETI_MAGIC_USES(ch) = atoi(line);
        else if (!strcmp(tag, "EfMT"))
          EFREETI_MAGIC_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "EldE"))
          GET_ELDRITCH_ESSENCE(ch) = atoi(line);
        else if (!strcmp(tag, "EldS"))
          GET_ELDRITCH_SHAPE(ch) = atoi(line);
        else if (!strcmp(tag, "EncM"))
          GET_ENCUMBRANCE_MOD(ch) = atoi(line);
        break;

      case 'F':
        if (!strcmp(tag, "Frez"))
          GET_FREEZE_LEV(ch) = atoi(line);
        else if (!strcmp(tag, "FaEn"))
          load_favored_enemy(fl, ch);
        else if (!strcmp(tag, "FaAd"))
          GET_FACTION_STANDING(ch, FACTION_ADVENTURERS) = atol(line);
        else if (!strcmp(tag, "Feat"))
          load_feats(fl, ch);
        else if (!strcmp(tag, "FLGT"))
          FLEETING_GLANCE_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "FdBn"))
          ch->player_specials->saved.fiendish_boons = atoi(line);
        else if (!strcmp(tag, "FLGU"))
          FLEETING_GLANCE_USES(ch) = atoi(line);
        else if (!strcmp(tag, "Ftpt"))
          GET_FEAT_POINTS(ch) = atoi(line);
        else if (!strcmp(tag, "FSWT"))
          FEY_SHADOW_WALK_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "FSWU"))
          FEY_SHADOW_WALK_USES(ch) = atoi(line);
        else if (!strcmp(tag, "FstH"))
          GET_FAST_HEALING_MOD(ch) = atoi(line);

        break;

      case 'G':
        if (!strcmp(tag, "Gold"))
          GET_GOLD(ch) = atoi(line);
        if (!strcmp(tag, "God "))
          GET_DEITY(ch) = atoi(line);
        else if (!strcmp(tag, "GMCP") && ch->desc)
          ch->desc->pProtocol->bGMCP = atoi(line);
        else if (!strcmp(tag, "GrDs"))
          GET_GRAND_DISCOVERY(ch) = atoi(line);
        else if (!strcmp(tag, "GTCT"))
          GRAVE_TOUCH_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "GTCU"))
          GRAVE_TOUCH_USES(ch) = atoi(line);
        else if (!strcmp(tag, "GODT"))
          GRASP_OF_THE_DEAD_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "GODU"))
          GRASP_OF_THE_DEAD_USES(ch) = atoi(line);
        break;

      case 'H':
        if (!strcmp(tag, "Hit "))
          load_HMVS(ch, line, LOAD_HIT);
        else if (!strcmp(tag, "Hite"))
          GET_HEIGHT(ch) = atoi(line);
        else if (!strcmp(tag, "HECn"))
          HIGH_ELF_CANTRIP(ch) = atoi(line);
        else if (!strcmp(tag, "HlyW"))
          GET_HOLY_WEAPON_TYPE(ch) = atoi(line);
        else if (!strcmp(tag, "Home"))
          GET_REGION(ch) = atoi(line);
        else if (!strcmp(tag, "Host"))
        {
          if (GET_HOST(ch))
            free(GET_HOST(ch));
          GET_HOST(ch) = strdup(line);
        }
        else if (!strcmp(tag, "HPRg"))
          GET_HP_REGEN(ch) = atoi(line);
        else if (!strcmp(tag, "Hrol"))
          GET_REAL_HITROLL(ch) = atoi(line);
        else if (!strcmp(tag, "Hung"))
          GET_COND(ch, HUNGER) = atoi(line);
        break;

      case 'I':
        if (!strcmp(tag, "Id  "))
          GET_IDNUM(ch) = atol(line);
        else if (!strcmp(tag, "InMa"))
          load_innate_magic_queue(fl, ch);
        else if (!strcmp(tag, "Int "))
          GET_REAL_INT(ch) = atoi(line);
        else if (!strcmp(tag, "Invs"))
          GET_INVIS_LEV(ch) = atoi(line);
        else if (!strcmp(tag, "InFT"))
          INCORPOREAL_FORM_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "InFU"))
          INCORPOREAL_FORM_USES(ch) = atoi(line);
        break;

      case 'J':
        if (!strcmp(tag, "Judg"))
          load_judgements(fl, ch);
        break;

      case 'K':
        if (!strcmp(tag, "KnSp"))
          load_known_spells(fl, ch);
        else if (!strcmp(tag, "KEvo"))
        {
          if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
          {
            KNOWN_EVOLUTIONS(ch)[0] = asciiflag_conv(f1);
            KNOWN_EVOLUTIONS(ch)[1] = asciiflag_conv(f2);
            KNOWN_EVOLUTIONS(ch)[2] = asciiflag_conv(f3);
            KNOWN_EVOLUTIONS(ch)[3] = asciiflag_conv(f4);
          }
          else
            KNOWN_EVOLUTIONS(ch)[0] = asciiflag_conv(line);
        }
        break;

      case 'L':
        if (!strcmp(tag, "Last"))
          ch->player.time.logon = atol(line);
        else if (!strcmp(tag, "Lang"))
          load_languages(fl, ch);
        else if (!strcmp(tag, "Lern"))
          GET_PRACTICES(ch) = atoi(line);
        else if (!strcmp(tag, "Levl"))
          GET_LEVEL(ch) = atoi(line);
        else if (!strcmp(tag, "Lmot"))
          GET_LAST_MOTD(ch) = atoi(line);
        else if (!strcmp(tag, "Lnew"))
          GET_LAST_NEWS(ch) = atoi(line);
        else if (!strcmp(tag, "LTCT"))
          LAUGHING_TOUCH_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "LTCU"))
          LAUGHING_TOUCH_USES(ch) = atoi(line);
        break;

      case 'M':
        if (!strcmp(tag, "Move"))
          load_HMVS(ch, line, LOAD_MOVE);
        else if (!strcmp(tag, "Mrph"))
          IS_MORPHED(ch) = atol(line);
        else if (!strcmp(tag, "Mrcy"))
          load_mercies(fl, ch);
        // Faction mission system
        else if (!strcmp(tag, "MiCu"))
          GET_CURRENT_MISSION(ch) = atoi(line);
        else if (!strcmp(tag, "MiCr"))
          GET_MISSION_CREDITS(ch) = atol(line);
        else if (!strcmp(tag, "MiCd"))
          GET_MISSION_COOLDOWN(ch) = atoi(line);
        else if (!strcmp(tag, "MiSt"))
          GET_MISSION_STANDING(ch) = atoi(line);
        else if (!strcmp(tag, "MiFa"))
          GET_MISSION_FACTION(ch) = atoi(line);
        else if (!strcmp(tag, "MiRe"))
          GET_MISSION_REP(ch) = atoi(line);
        else if (!strcmp(tag, "MiXp"))
          GET_MISSION_EXP(ch) = atol(line);
        else if (!strcmp(tag, "MiDf"))
          GET_MISSION_DIFFICULTY(ch) = atoi(line);
        else if (!strcmp(tag, "MiRN"))
          GET_MISSION_NPC_NAME_NUM(ch) = atoi(line);
        else if (!strcmp(tag, "MVRg"))
          GET_MV_REGEN(ch) = atoi(line);
        break;

      case 'N':
        if (!strcmp(tag, "Name"))
          GET_PC_NAME(ch) = strdup(line);
        else if (!strcmp(tag, "NAr0"))
          NEW_ARCANA_SLOT(ch, 0) = atoi(line);
        else if (!strcmp(tag, "NAr1"))
          NEW_ARCANA_SLOT(ch, 1) = atoi(line);
        else if (!strcmp(tag, "NAr2"))
          NEW_ARCANA_SLOT(ch, 2) = atoi(line);
        else if (!strcmp(tag, "NAr3"))
          NEW_ARCANA_SLOT(ch, 3) = atoi(line);
        break;

      case 'O':
        if (!strcmp(tag, "Olc "))
          GET_OLC_ZONE(ch) = atoi(line);
        break;

      case 'P':
        if (!strcmp(tag, "Page"))
          GET_PAGE_LENGTH(ch) = atoi(line);
        else if (!strcmp(tag, "Pass"))
          strcpy(GET_PASSWD(ch), line);
        else if (!strcmp(tag, "Potn"))
          load_potions(fl, ch);
        else if (!strcmp(tag, "Plyd"))
          ch->player.time.played = atoi(line);
        else if (!strcmp(tag, "PreB"))
          GET_PREMADE_BUILD_CLASS(ch) = atoi(line);
        else if (!strcmp(tag, "Pryg"))
          load_praying(fl, ch);
        else if (!strcmp(tag, "Prgm"))
          load_praying_metamagic(fl, ch);
        else if (!strcmp(tag, "Pryd"))
          load_prayed(fl, ch);
        else if (!strcmp(tag, "Prdm"))
          load_prayed_metamagic(fl, ch);
        else if (!strcmp(tag, "Pryt"))
          load_praytimes(fl, ch);
        else if (!strcmp(tag, "PfIn"))
          POOFIN(ch) = strdup(line);
        else if (!strcmp(tag, "PfOt"))
          POOFOUT(ch) = strdup(line);
        else if (!strcmp(tag, "Pref"))
        {
          if (sscanf(line, "%s %s %s %s", f1, f2, f3, f4) == 4)
          {
            PRF_FLAGS(ch)
            [0] = asciiflag_conv(f1);
            PRF_FLAGS(ch)
            [1] = asciiflag_conv(f2);
            PRF_FLAGS(ch)
            [2] = asciiflag_conv(f3);
            PRF_FLAGS(ch)
            [3] = asciiflag_conv(f4);
          }
          else
            PRF_FLAGS(ch)
          [0] = asciiflag_conv(f1);
        }
        else if (!strcmp(tag, "PrQu"))
          load_spell_prep_queue(fl, ch);
        else if (!strcmp(tag, "PCAr"))
          GET_PREFERRED_ARCANE(ch) = atoi(line);
        else if (!strcmp(tag, "PCDi"))
          GET_PREFERRED_DIVINE(ch) = atoi(line);
        else if (!strcmp(tag, "PSP "))
          load_HMVS(ch, line, LOAD_PSP);
        else if (!strcmp(tag, "PSRg"))
          GET_PSP_REGEN(ch) = atoi(line);
        else if (!strcmp(tag, "PsET"))
          GET_PSIONIC_ENERGY_TYPE(ch) = atoi(line);
        else if (!strcmp(tag, "PxDU"))
          PIXIE_DUST_USES(ch) = atoi(line);
        else if (!strcmp(tag, "PxDT"))
          PIXIE_DUST_TIMER(ch) = atoi(line);
        break;

      case 'Q':
        if (!strcmp(tag, "Qstp"))
          GET_QUESTPOINTS(ch) = atoi(line);
        else if (!strcmp(tag, "Qpnt"))
          GET_QUESTPOINTS(ch) = atoi(line); /* Backward compatibility */
        else if (!strcmp(tag, "Qcur"))
          GET_QUEST(ch, 0) = atoi(line);
        else if (!strcmp(tag, "Qcu1"))
          GET_QUEST(ch, 1) = atoi(line);
        else if (!strcmp(tag, "Qcu2"))
          GET_QUEST(ch, 2) = atoi(line);
        else if (!strcmp(tag, "Qcnt"))
          GET_QUEST_COUNTER(ch, 0) = atoi(line);
        else if (!strcmp(tag, "Qcn1"))
          GET_QUEST_COUNTER(ch, 1) = atoi(line);
        else if (!strcmp(tag, "Qcn2"))
          GET_QUEST_COUNTER(ch, 2) = atoi(line);
        else if (!strcmp(tag, "Qest"))
          load_quests(fl, ch);
        break;

      case 'R':
        if (!strcmp(tag, "Race"))
          GET_REAL_RACE(ch) = atoi(line);
        if (!strcmp(tag, "RacR"))
          ch->player_specials->saved.new_race_stats = atoi(line);
        else if (!strcmp(tag, "Room"))
          GET_LOADROOM(ch) = atoi(line);
        else if (!strcmp(tag, "Res1"))
          GET_REAL_RESISTANCES(ch, 1) = atoi(line);
        else if (!strcmp(tag, "Res2"))
          GET_REAL_RESISTANCES(ch, 2) = atoi(line);
        else if (!strcmp(tag, "Res3"))
          GET_REAL_RESISTANCES(ch, 3) = atoi(line);
        else if (!strcmp(tag, "Res4"))
          GET_REAL_RESISTANCES(ch, 4) = atoi(line);
        else if (!strcmp(tag, "Res5"))
          GET_REAL_RESISTANCES(ch, 5) = atoi(line);
        else if (!strcmp(tag, "Res6"))
          GET_REAL_RESISTANCES(ch, 6) = atoi(line);
        else if (!strcmp(tag, "Res7"))
          GET_REAL_RESISTANCES(ch, 7) = atoi(line);
        else if (!strcmp(tag, "Res8"))
          GET_REAL_RESISTANCES(ch, 8) = atoi(line);
        else if (!strcmp(tag, "Res9"))
          GET_REAL_RESISTANCES(ch, 9) = atoi(line);
        else if (!strcmp(tag, "ResA"))
          GET_REAL_RESISTANCES(ch, 10) = atoi(line);
        else if (!strcmp(tag, "ResB"))
          GET_REAL_RESISTANCES(ch, 11) = atoi(line);
        else if (!strcmp(tag, "ResC"))
          GET_REAL_RESISTANCES(ch, 12) = atoi(line);
        else if (!strcmp(tag, "ResD"))
          GET_REAL_RESISTANCES(ch, 13) = atoi(line);
        else if (!strcmp(tag, "ResE"))
          GET_REAL_RESISTANCES(ch, 14) = atoi(line);
        else if (!strcmp(tag, "ResF"))
          GET_REAL_RESISTANCES(ch, 15) = atoi(line);
        else if (!strcmp(tag, "ResG"))
          GET_REAL_RESISTANCES(ch, 16) = atoi(line);
        else if (!strcmp(tag, "ResH"))
          GET_REAL_RESISTANCES(ch, 17) = atoi(line);
        else if (!strcmp(tag, "ResI"))
          GET_REAL_RESISTANCES(ch, 18) = atoi(line);
        else if (!strcmp(tag, "ResJ"))
          GET_REAL_RESISTANCES(ch, 19) = atoi(line);
        else if (!strcmp(tag, "ResK"))
          GET_REAL_RESISTANCES(ch, 20) = atoi(line);
        else if (!strcmp(tag, "RSc1"))
          GET_1ST_RESTRICTED_SCHOOL(ch) = atoi(line);
        else if (!strcmp(tag, "RSc2"))
          GET_2ND_RESTRICTED_SCHOOL(ch) = atoi(line);
        break;

      case 'S':
        if (!strcmp(tag, "Sex "))
          GET_SEX(ch) = atoi(line);
        else if (!strcmp(tag, "SBld"))
          GET_BLOODLINE_SUBTYPE(ch) = atoi(line);
        else if (!strcmp(tag, "SclF"))
        {
          sscanf(line, "%d %s", &i, f1);
          if (i < 0 || i >= NUM_SFEATS)
          {
            log("load_char: %s school feat record out of range: %s", GET_NAME(ch), line);
            break;
          }
          ch->char_specials.saved.school_feats[i] = asciiflag_conv(f1);
        }
        else if (!strcmp(tag, "Scrl"))
          load_scrolls(fl, ch);
        else if (!strcmp(tag, "ScrW"))
          GET_SCREEN_WIDTH(ch) = atoi(line);
        else if (!strcmp(tag, "Skil"))
          load_skills(fl, ch);
        else if (!strcmp(tag, "SklF"))
          load_skill_focus(fl, ch);
        else if (!strcmp(tag, "SpAb"))
          load_spec_abil(fl, ch);
        else if (!strcmp(tag, "Spek"))
          SPEAKING(ch) = atoi(line);
        else if (!strcmp(tag, "SpRs"))
          GET_REAL_SPELL_RES(ch) = atoi(line);
        else if (!strcmp(tag, "Size"))
          GET_REAL_SIZE(ch) = atoi(line);
        else if (!strcmp(tag, "Stav"))
          load_staves(fl, ch);
        else if (!strcmp(tag, "Slyr"))
          GET_SLAYER_JUDGEMENT(ch) = atoi(line);
        else if (!strcmp(tag, "SySt"))
          HAS_SET_STATS_STUDY(ch) = atoi(line);
        else if (!strcmp(tag, "Str "))
          load_HMVS(ch, line, LOAD_STRENGTH);
        else if (!strcmp(tag, "SSch"))
          GET_SPECIALTY_SCHOOL(ch) = atoi(line);
        break;

      case 'T':
        if (!strcmp(tag, "Tmpl"))
          GET_TEMPLATE(ch) = atoi(line);
        else if (!strcmp(tag, "Thir"))
          GET_COND(ch, THIRST) = atoi(line);
        else if (!strcmp(tag, "Thr1"))
          GET_REAL_SAVE(ch, 0) = atoi(line);
        else if (!strcmp(tag, "Thr2"))
          GET_REAL_SAVE(ch, 1) = atoi(line);
        else if (!strcmp(tag, "Thr3"))
          GET_REAL_SAVE(ch, 2) = atoi(line);
        else if (!strcmp(tag, "Thr4"))
          GET_REAL_SAVE(ch, 3) = atoi(line);
        else if (!strcmp(tag, "Thr5"))
          GET_REAL_SAVE(ch, 4) = atoi(line);
        else if (!strcmp(tag, "Titl"))
          GET_TITLE(ch) = strdup(line);
        else if (!strcmp(tag, "Trig") && CONFIG_SCRIPT_PLAYERS)
        {
          if ((t_rnum = real_trigger(atoi(line))) != NOTHING)
          {
            t = read_trigger(t_rnum);
            if (!SCRIPT(ch))
              CREATE(SCRIPT(ch), struct script_data, 1);
            add_trigger(SCRIPT(ch), t, -1);
          }
        }
        else if (!strcmp(tag, "Trns"))
          GET_TRAINS(ch) = atoi(line);
        else if (!strcmp(tag, "Todo"))
        {
          CREATE(GET_TODO(ch), struct txt_block, 1);
          struct txt_block *tmp = GET_TODO(ch);

          get_line(fl, line);
          while (*line != '~')
          {
            tmp->text = strdup(line);
            get_line(fl, line);

            if (*line != '~')
            {
              CREATE(tmp->next, struct txt_block, 1);
              tmp = tmp->next;
            }
          }
        }
        break;

      case 'U':
        if (!strcmp(tag, "UTF8") && ch->desc)
          ch->desc->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt = atoi(line);
        break;

      case 'V':
        if (!strcmp(tag, "Vars"))
          read_saved_vars_ascii(fl, ch, atoi(line));
        break;

      case 'W':
        if (!strcmp(tag, "Wate"))
          GET_WEIGHT(ch) = atoi(line);
        else if (!strcmp(tag, "Wand"))
          load_wands(fl, ch);
        else if (!strcmp(tag, "Wimp"))
          GET_WIMP_LEV(ch) = atoi(line);
        else if (!strcmp(tag, "Ward"))
          load_warding(fl, ch);
        else if (!strcmp(tag, "Wis "))
          GET_REAL_WIS(ch) = atoi(line);
        break;

      case 'X':
        if (!strcmp(tag, "XTrm") && ch->desc)
          ch->desc->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt = atoi(line);
        break;

      default:
        snprintf(buf, sizeof(buf), "SYSERR: Unknown tag %s in pfile %s", tag, name);
      }
    }
  }

  resetCastingData(ch);
  CLOUDKILL(ch) = 0;  // make sure init cloudkill burst
  DOOM(ch) = 0;       // make sure init creeping doom
  TENACIOUS_PLAGUE(ch) = 0;
  INCENDIARY(ch) = 0; // make sure init incendiary burst

  affect_total(ch);

  /* initialization for imms */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
  {
    for (i = 1; i < MAX_SKILLS; i++)
      GET_SKILL(ch, i) = 100;
    for (i = 1; i <= MAX_ABILITIES; i++)
      GET_ABILITY(ch, i) = 40;
    GET_COND(ch, HUNGER) = -1;
    GET_COND(ch, THIRST) = -1;
    GET_COND(ch, DRUNK) = -1;
  }
  fclose(fl);
  return (id);
}

/* Write the vital data of a player to the player file. */

/* This is the ASCII Player Files save routine. */
void save_char(struct char_data *ch, int mode)
{
  FILE *fl;
  char filename[40] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'},
       bits[127] = {'\0'}, bits2[127] = {'\0'},
       bits3[127] = {'\0'}, bits4[127] = {'\0'};
  int i = 0, j = 0, id = 0, save_index = FALSE;
  struct affected_type *aff = NULL;
  struct affected_type tmp_aff[MAX_AFFECT] = {
      {0}};
  struct damage_reduction_type *tmp_dr = NULL, *cur_dr = NULL;
  struct obj_data *char_eq[NUM_WEARS] = {NULL};
  trig_data *t = NULL;
  struct mud_event_data *pMudEvent = NULL;

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;

  /* If ch->desc is not null, then update session data before saving. */
  if (ch->desc)
  {
    if (ch->desc->host && *ch->desc->host)
    {
      if (!GET_HOST(ch))
        GET_HOST(ch) = strdup(ch->desc->host);
      else if (GET_HOST(ch) && strcmp(GET_HOST(ch), ch->desc->host))
      {
        free(GET_HOST(ch));
        GET_HOST(ch) = strdup(ch->desc->host);
      }
    }

    /* Only update the time.played and time.logon if the character is playing. */
    if (STATE(ch->desc) == CON_PLAYING)
    {
      ch->player.time.played += time(0) - ch->player.time.logon;
      ch->player.time.logon = time(0);
    }
  }

  /* any problems with file handling? */
  if (!get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch)))
    return;
  if (!(fl = fopen(filename, "w")))
  {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Couldn't open player file %s for write", filename);
    return;
  }

  /* Unaffect everything a character can be affected by. */
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (GET_EQ(ch, i))
    {
      char_eq[i] = unequip_char(ch, i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      remove_otrigger(char_eq[i], ch);
#endif
    }
    else
      char_eq[i] = NULL;
  }

  for (aff = ch->affected, i = 0; i < MAX_AFFECT; i++)
  {
    if (aff)
    {
      tmp_aff[i] = *aff;
      for (j = 0; j < AF_ARRAY_MAX; j++)
        tmp_aff[i].bitvector[j] = aff->bitvector[j];
      tmp_aff[i].next = 0;
      aff = aff->next;
    }
    else
    {
      new_affect(&(tmp_aff[i]));
      tmp_aff[i].next = 0;
    }
  }

  /* Save off the dr since that is attached to affects (i.e. stoneskin will
   * create a dr structure that is loosely coupled to the affect for the spell.
   * If the spell affect is removed, however, the stoneskin dr is dropped.)
   * This only counts for dr where spell is != 0. */
  if (ch && GET_DR(ch) != NULL)
  {
    for (cur_dr = GET_DR(ch); cur_dr != NULL; cur_dr = cur_dr->next)
    {
      if (cur_dr->spell != 0)
      {

        struct damage_reduction_type *tmp;

        CREATE(tmp, struct damage_reduction_type, 1);
        *tmp = *cur_dr;
        tmp->next = tmp_dr;
        tmp_dr = tmp;
      }
    }
  }

  /* Remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in. */

  while (ch->affected)
    affect_remove(ch, ch->affected);

  if ((i >= MAX_AFFECT) && aff && aff->next)
    log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");

  ch->aff_abils = ch->real_abils;
  reset_char_points(ch);

  /* Make sure size doesn't go over/under caps */

  /* end char_to_store code */

  if (GET_NAME(ch))
    fprintf(fl, "Name: %s\n", GET_NAME(ch));
  if (GET_PASSWD(ch))
    fprintf(fl, "Pass: %s\n", GET_PASSWD(ch));
  if (ch->desc && ch->desc->account && ch->desc->account->name)
  {
    fprintf(fl, "Acct: %s\n", ch->desc->account->name);
    //    fprintf(fl, "ActN: %s\n", ch->desc->account->name);
  }

  if (GET_TITLE(ch))
    fprintf(fl, "Titl: %s\n", GET_TITLE(ch));

  /*save todo lists*/
  struct txt_block *tmp;
  if ((tmp = GET_TODO(ch)))
  {
    fprintf(fl, "Todo:\n");
    while (tmp)
    {
      if (tmp->text)
        fprintf(fl, "%s\n", tmp->text);
      tmp = tmp->next;
    }
    fprintf(fl, "~\n");
  }

  if (ch->player.description && *ch->player.description)
  {
    strlcpy(buf, ch->player.description, sizeof(buf));
    strip_cr(buf);
    fprintf(fl, "Desc:\n%s~\n", buf);
  }
  if (ch->player.background && *ch->player.background)
  {
    strlcpy(buf, ch->player.background, sizeof(buf));
    strip_cr(buf);
    fprintf(fl, "BGrd:\n%s~\n", buf);
  }
  if (POOFIN(ch))
    fprintf(fl, "PfIn: %s\n", POOFIN(ch));
  if (POOFOUT(ch))
    fprintf(fl, "PfOt: %s\n", POOFOUT(ch));
  if (GET_SEX(ch) != PFDEF_SEX)
    fprintf(fl, "Sex : %d\n", GET_SEX(ch));
  if (GET_BLOODLINE_SUBTYPE(ch) != PFDEF_SORC_BLOODLINE_SUBTYPE)
    fprintf(fl, "SBld: %d\n", GET_BLOODLINE_SUBTYPE(ch));
  if (GET_CLASS(ch) != PFDEF_CLASS)
    fprintf(fl, "Clas: %d\n", GET_CLASS(ch));
  if (GET_REAL_RACE(ch) != PFDEF_RACE)
    fprintf(fl, "Race: %d\n", GET_REAL_RACE(ch));
  if (GET_REAL_SIZE(ch) != PFDEF_SIZE)
    fprintf(fl, "Size: %d\n", GET_REAL_SIZE(ch));
  if (HAS_SET_STATS_STUDY(ch) != PFDEF_HAS_SET_STATS_STUDY)
    fprintf(fl, "SySt: %d\n", HAS_SET_STATS_STUDY(ch));
  if (GET_LEVEL(ch) != PFDEF_LEVEL)
    fprintf(fl, "Levl: %d\n", GET_LEVEL(ch));
  if (GET_DISGUISE_RACE(ch))
    fprintf(fl, "DRac: %d\n", GET_DISGUISE_RACE(ch));
  if (GET_DISGUISE_STR(ch))
    fprintf(fl, "DStr: %d\n", GET_DISGUISE_STR(ch));
  if (GET_DISGUISE_DEX(ch))
    fprintf(fl, "DDex: %d\n", GET_DISGUISE_DEX(ch));
  if (GET_DISGUISE_CON(ch))
    fprintf(fl, "DCon: %d\n", GET_DISGUISE_CON(ch));
  if (GET_DISGUISE_AC(ch))
    fprintf(fl, "DAC: %d\n", GET_DISGUISE_AC(ch));
  if (NEW_ARCANA_SLOT(ch, 0))
    fprintf(fl, "NAr0: %d\n", NEW_ARCANA_SLOT(ch, 0));
  if (NEW_ARCANA_SLOT(ch, 1))
    fprintf(fl, "NAr1: %d\n", NEW_ARCANA_SLOT(ch, 1));
  if (NEW_ARCANA_SLOT(ch, 2))
    fprintf(fl, "NAr2: %d\n", NEW_ARCANA_SLOT(ch, 2));
  if (NEW_ARCANA_SLOT(ch, 3))
    fprintf(fl, "NAr3: %d\n", NEW_ARCANA_SLOT(ch, 3));
  fprintf(fl, "Id  : %ld\n", GET_IDNUM(ch));
  fprintf(fl, "Brth: %ld\n", (long)ch->player.time.birth);
  fprintf(fl, "Plyd: %d\n", ch->player.time.played);
  fprintf(fl, "Last: %ld\n", (long)ch->player.time.logon);

  if (GET_LAST_MOTD(ch) != PFDEF_LASTMOTD)
    fprintf(fl, "Lmot: %d\n", (int)GET_LAST_MOTD(ch));
  if (GET_LAST_NEWS(ch) != PFDEF_LASTNEWS)
    fprintf(fl, "Lnew: %d\n", (int)GET_LAST_NEWS(ch));

  fprintf(fl, "DrgB: %d\n", GET_DRAGONBORN_ANCESTRY(ch));

  fprintf(fl, "Spek: %d\n", SPEAKING(ch));
  fprintf(fl, "Home: %d\n", GET_REGION(ch));
  fprintf(fl, "DAd1: %d\n", GET_PC_ADJECTIVE_1(ch));
  fprintf(fl, "DAd2: %d\n", GET_PC_ADJECTIVE_2(ch));
  fprintf(fl, "DDs1: %d\n", GET_PC_DESCRIPTOR_1(ch));
  fprintf(fl, "DDs2: %d\n", GET_PC_DESCRIPTOR_2(ch));

  if (ch->player_specials->saved.new_race_stats)
    fprintf(fl, "RacR: %d\n", ch->player_specials->saved.new_race_stats);

  if (GET_HOST(ch))
    fprintf(fl, "Host: %s\n", GET_HOST(ch));
  if (GET_HEIGHT(ch) != PFDEF_HEIGHT)
    fprintf(fl, "Hite: %d\n", GET_HEIGHT(ch));
  if (HIGH_ELF_CANTRIP(ch))
    fprintf(fl, "HECn: %d\n", HIGH_ELF_CANTRIP(ch));
  if (GET_HOLY_WEAPON_TYPE(ch) != PFDEF_HOLY_WEAPON_TYPE)
    fprintf(fl, "HlyW: %d\n", GET_HOLY_WEAPON_TYPE(ch));
  if (GET_WEIGHT(ch) != PFDEF_WEIGHT)
    fprintf(fl, "Wate: %d\n", GET_WEIGHT(ch));
  if (GET_ALIGNMENT(ch) != PFDEF_ALIGNMENT)
    fprintf(fl, "Alin: %d\n", GET_ALIGNMENT(ch));
  if (GET_TEMPLATE(ch) != PFDEF_TEMPLATE)
    fprintf(fl, "Tmpl: %d\n", GET_TEMPLATE(ch));
  // Faction mission system
  fprintf(fl, "MiCu: %d\n", GET_CURRENT_MISSION(ch));
  fprintf(fl, "MiCr: %ld\n", GET_MISSION_CREDITS(ch));
  fprintf(fl, "MiCd: %d\n", GET_MISSION_COOLDOWN(ch));
  fprintf(fl, "MiSt: %ld\n", GET_MISSION_STANDING(ch));
  fprintf(fl, "MiFa: %d\n", GET_MISSION_FACTION(ch));
  fprintf(fl, "MiRe: %ld\n", GET_MISSION_REP(ch));
  fprintf(fl, "MiXp: %ld\n", GET_MISSION_EXP(ch));
  fprintf(fl, "MiDf: %d\n", GET_MISSION_DIFFICULTY(ch));
  fprintf(fl, "MiRN: %d\n", GET_MISSION_NPC_NAME_NUM(ch));

  sprintascii(bits, PLR_FLAGS(ch)[0]);
  sprintascii(bits2, PLR_FLAGS(ch)[1]);
  sprintascii(bits3, PLR_FLAGS(ch)[2]);
  sprintascii(bits4, PLR_FLAGS(ch)[3]);
  fprintf(fl, "Act : %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, AFF_FLAGS(ch)[0]);
  sprintascii(bits2, AFF_FLAGS(ch)[1]);
  sprintascii(bits3, AFF_FLAGS(ch)[2]);
  sprintascii(bits4, AFF_FLAGS(ch)[3]);
  fprintf(fl, "Aff : %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, EVOLUTIONS(ch)[0]);
  sprintascii(bits2, EVOLUTIONS(ch)[1]);
  sprintascii(bits3, EVOLUTIONS(ch)[2]);
  sprintascii(bits4, EVOLUTIONS(ch)[3]);
  fprintf(fl, "Evol: %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, KNOWN_EVOLUTIONS(ch)[0]);
  sprintascii(bits2, KNOWN_EVOLUTIONS(ch)[1]);
  sprintascii(bits3, KNOWN_EVOLUTIONS(ch)[2]);
  sprintascii(bits4, KNOWN_EVOLUTIONS(ch)[3]);
  fprintf(fl, "KEvo: %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, PRF_FLAGS(ch)[0]);
  sprintascii(bits2, PRF_FLAGS(ch)[1]);
  sprintascii(bits3, PRF_FLAGS(ch)[2]);
  sprintascii(bits4, PRF_FLAGS(ch)[3]);
  fprintf(fl, "Pref: %s %s %s %s\n", bits, bits2, bits3, bits4);

  if (GET_SAVE(ch, 0) != PFDEF_SAVETHROW)
    fprintf(fl, "Thr1: %d\n", GET_SAVE(ch, 0));
  if (GET_SAVE(ch, 1) != PFDEF_SAVETHROW)
    fprintf(fl, "Thr2: %d\n", GET_SAVE(ch, 1));
  if (GET_SAVE(ch, 2) != PFDEF_SAVETHROW)
    fprintf(fl, "Thr3: %d\n", GET_SAVE(ch, 2));
  if (GET_SAVE(ch, 3) != PFDEF_SAVETHROW)
    fprintf(fl, "Thr4: %d\n", GET_SAVE(ch, 3));
  if (GET_SAVE(ch, 4) != PFDEF_SAVETHROW)
    fprintf(fl, "Thr5: %d\n", GET_SAVE(ch, 4));

  if (GET_RESISTANCES(ch, 1) != PFDEF_RESISTANCES)
    fprintf(fl, "Res1: %d\n", GET_RESISTANCES(ch, 1));
  if (GET_RESISTANCES(ch, 2) != PFDEF_RESISTANCES)
    fprintf(fl, "Res2: %d\n", GET_RESISTANCES(ch, 2));
  if (GET_RESISTANCES(ch, 3) != PFDEF_RESISTANCES)
    fprintf(fl, "Res3: %d\n", GET_RESISTANCES(ch, 3));
  if (GET_RESISTANCES(ch, 4) != PFDEF_RESISTANCES)
    fprintf(fl, "Res4: %d\n", GET_RESISTANCES(ch, 4));
  if (GET_RESISTANCES(ch, 5) != PFDEF_RESISTANCES)
    fprintf(fl, "Res5: %d\n", GET_RESISTANCES(ch, 5));
  if (GET_RESISTANCES(ch, 6) != PFDEF_RESISTANCES)
    fprintf(fl, "Res6: %d\n", GET_RESISTANCES(ch, 6));
  if (GET_RESISTANCES(ch, 7) != PFDEF_RESISTANCES)
    fprintf(fl, "Res7: %d\n", GET_RESISTANCES(ch, 7));
  if (GET_RESISTANCES(ch, 8) != PFDEF_RESISTANCES)
    fprintf(fl, "Res8: %d\n", GET_RESISTANCES(ch, 8));
  if (GET_RESISTANCES(ch, 9) != PFDEF_RESISTANCES)
    fprintf(fl, "Res9: %d\n", GET_RESISTANCES(ch, 9));
  if (GET_RESISTANCES(ch, 10) != PFDEF_RESISTANCES)
    fprintf(fl, "ResA: %d\n", GET_RESISTANCES(ch, 10));
  if (GET_RESISTANCES(ch, 11) != PFDEF_RESISTANCES)
    fprintf(fl, "ResB: %d\n", GET_RESISTANCES(ch, 11));
  if (GET_RESISTANCES(ch, 12) != PFDEF_RESISTANCES)
    fprintf(fl, "ResC: %d\n", GET_RESISTANCES(ch, 12));
  if (GET_RESISTANCES(ch, 13) != PFDEF_RESISTANCES)
    fprintf(fl, "ResD: %d\n", GET_RESISTANCES(ch, 13));
  if (GET_RESISTANCES(ch, 14) != PFDEF_RESISTANCES)
    fprintf(fl, "ResE: %d\n", GET_RESISTANCES(ch, 14));
  if (GET_RESISTANCES(ch, 15) != PFDEF_RESISTANCES)
    fprintf(fl, "ResF: %d\n", GET_RESISTANCES(ch, 15));
  if (GET_RESISTANCES(ch, 16) != PFDEF_RESISTANCES)
    fprintf(fl, "ResG: %d\n", GET_RESISTANCES(ch, 16));
  if (GET_RESISTANCES(ch, 17) != PFDEF_RESISTANCES)
    fprintf(fl, "ResH: %d\n", GET_RESISTANCES(ch, 17));
  if (GET_RESISTANCES(ch, 18) != PFDEF_RESISTANCES)
    fprintf(fl, "ResI: %d\n", GET_RESISTANCES(ch, 18));
  if (GET_RESISTANCES(ch, 19) != PFDEF_RESISTANCES)
    fprintf(fl, "ResJ: %d\n", GET_RESISTANCES(ch, 19));
  if (GET_RESISTANCES(ch, 20) != PFDEF_RESISTANCES)
    fprintf(fl, "ResK: %d\n", GET_RESISTANCES(ch, 20));

  if (GET_WIMP_LEV(ch) != PFDEF_WIMPLEV)
    fprintf(fl, "Wimp: %d\n", GET_WIMP_LEV(ch));
  if (GET_FREEZE_LEV(ch) != PFDEF_FREEZELEV)
    fprintf(fl, "Frez: %d\n", GET_FREEZE_LEV(ch));
  if (GET_INVIS_LEV(ch) != PFDEF_INVISLEV)
    fprintf(fl, "Invs: %d\n", GET_INVIS_LEV(ch));
  if (GET_LOADROOM(ch) != PFDEF_LOADROOM)
    fprintf(fl, "Room: %d\n", GET_LOADROOM(ch));
  if (ch->player_specials->saved.fiendish_boons != 0)
    fprintf(fl, "FdBn: %d\n", ch->player_specials->saved.fiendish_boons);
  if (ch->player_specials->saved.channel_energy_type != 0)
    fprintf(fl, "ChEn: %d\n", ch->player_specials->saved.channel_energy_type);

  fprintf(fl, "FaAd: %ld\n", GET_FACTION_STANDING(ch, FACTION_ADVENTURERS));

  if (GET_BAD_PWS(ch) != PFDEF_BADPWS)
    fprintf(fl, "Badp: %d\n", GET_BAD_PWS(ch));
  if (GET_PRACTICES(ch) != PFDEF_PRACTICES)
    fprintf(fl, "Lern: %d\n", GET_PRACTICES(ch));
  if (GET_TRAINS(ch) != PFDEF_TRAINS)
    fprintf(fl, "Trns: %d\n", GET_TRAINS(ch));
  if (GET_BOOSTS(ch) != PFDEF_BOOSTS)
    fprintf(fl, "Bost: %d\n", GET_BOOSTS(ch));

  if (GET_1ST_DOMAIN(ch) != PFDEF_DOMAIN_1)
    fprintf(fl, "Dom1: %d\n", GET_1ST_DOMAIN(ch));
  if (GET_2ND_DOMAIN(ch) != PFDEF_DOMAIN_2)
    fprintf(fl, "Dom2: %d\n", GET_2ND_DOMAIN(ch));
  if (GET_SPECIALTY_SCHOOL(ch) != PFDEF_SPECIALTY_SCHOOL)
    fprintf(fl, "SSch: %d\n", GET_SPECIALTY_SCHOOL(ch));
  if (GET_1ST_RESTRICTED_SCHOOL(ch) != PFDEF_RESTRICTED_SCHOOL_1)
    fprintf(fl, "RSc1: %d\n", GET_1ST_RESTRICTED_SCHOOL(ch));
  if (GET_2ND_RESTRICTED_SCHOOL(ch) != PFDEF_RESTRICTED_SCHOOL_2)
    fprintf(fl, "RSc2: %d\n", GET_2ND_RESTRICTED_SCHOOL(ch));

  if (GET_PREFERRED_ARCANE(ch) != PFDEF_PREFERRED_ARCANE)
    fprintf(fl, "PCAr: %d\n", GET_PREFERRED_ARCANE(ch));
  if (GET_PREFERRED_DIVINE(ch) != PFDEF_PREFERRED_DIVINE)
    fprintf(fl, "PCDi: %d\n", GET_PREFERRED_DIVINE(ch));

  if (GET_FEAT_POINTS(ch) != 0)
    fprintf(fl, "Ftpt: %d\n", GET_FEAT_POINTS(ch));

  fprintf(fl, "Cfpt:\n");
  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_CLASS_FEATS(ch, i) != 0)
      fprintf(fl, "%d %d\n", i, GET_CLASS_FEATS(ch, i));
  fprintf(fl, "0\n");

  if (GET_EPIC_FEAT_POINTS(ch) != 0)
    fprintf(fl, "Efpt: %d\n", GET_EPIC_FEAT_POINTS(ch));

  fprintf(fl, "Ecfp:\n");
  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_EPIC_CLASS_FEATS(ch, i) != 0)
      fprintf(fl, "%d %d\n", i, GET_EPIC_CLASS_FEATS(ch, i));
  fprintf(fl, "0\n");

  if (GET_COND(ch, HUNGER) != PFDEF_HUNGER && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "Hung: %d\n", GET_COND(ch, HUNGER));
  if (GET_COND(ch, THIRST) != PFDEF_THIRST && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "Thir: %d\n", GET_COND(ch, THIRST));
  if (GET_COND(ch, DRUNK) != PFDEF_DRUNK && GET_LEVEL(ch) < LVL_IMMORT)
    fprintf(fl, "Drnk: %d\n", GET_COND(ch, DRUNK));

  if (GET_HIT(ch) != PFDEF_HIT || GET_MAX_HIT(ch) != PFDEF_MAXHIT)
    fprintf(fl, "Hit : %d/%d\n", GET_HIT(ch), GET_MAX_HIT(ch));
  if (GET_PSP(ch) != PFDEF_PSP || GET_MAX_PSP(ch) != PFDEF_MAXPSP)
    fprintf(fl, "PSP : %d/%d\n", GET_PSP(ch), GET_MAX_PSP(ch));
  if (GET_MOVE(ch) != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    fprintf(fl, "Move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));

  if (GET_HP_REGEN(ch) != PFDEF_HP_REGEN)
    fprintf(fl, "HPRg : %d\n", GET_HP_REGEN(ch));
  if (GET_MV_REGEN(ch) != PFDEF_MV_REGEN)
    fprintf(fl, "MVRg : %d\n", GET_MV_REGEN(ch));
  if (GET_PSP_REGEN(ch) != PFDEF_PSP_REGEN)
    fprintf(fl, "PSRg : %d\n", GET_PSP_REGEN(ch));

  if (GET_SETCLOAK_TIMER(ch) != PFDEF_SETCLOAK_TIMER)
    fprintf(fl, "ClkT : %d\n", GET_SETCLOAK_TIMER(ch));

  if (GET_STR(ch) != PFDEF_STR || GET_ADD(ch) != PFDEF_STRADD)
    fprintf(fl, "Str : %d/%d\n", GET_STR(ch), GET_ADD(ch));

  if (GET_INT(ch) != PFDEF_INT)
    fprintf(fl, "Int : %d\n", GET_INT(ch));
  if (GET_WIS(ch) != PFDEF_WIS)
    fprintf(fl, "Wis : %d\n", GET_WIS(ch));
  if (GET_DEX(ch) != PFDEF_DEX)
    fprintf(fl, "Dex : %d\n", GET_DEX(ch));
  if (GET_CON(ch) != PFDEF_CON)
    fprintf(fl, "Con : %d\n", GET_CON(ch));
  if (GET_CHA(ch) != PFDEF_CHA)
    fprintf(fl, "Cha : %d\n", GET_CHA(ch));

  if (GET_AC(ch) != PFDEF_AC)
    fprintf(fl, "Ac  : %d\n", GET_AC(ch));
  if (GET_GOLD(ch) != PFDEF_GOLD)
    fprintf(fl, "Gold: %d\n", GET_GOLD(ch));
  if (GET_BANK_GOLD(ch) != PFDEF_BANK)
    fprintf(fl, "Bank: %d\n", GET_BANK_GOLD(ch));
  if (GET_EXP(ch) != PFDEF_EXP)
    fprintf(fl, "Exp : %d\n", GET_EXP(ch));
  if (GET_HITROLL(ch) != PFDEF_HITROLL)
    fprintf(fl, "Hrol: %d\n", GET_HITROLL(ch));
  if (GET_DAMROLL(ch) != PFDEF_DAMROLL)
    fprintf(fl, "Drol: %d\n", GET_DAMROLL(ch));
  if (GET_SPELL_RES(ch) != PFDEF_SPELL_RES)
    fprintf(fl, "SpRs: %d\n", GET_SPELL_RES(ch));
  if (IS_MORPHED(ch) != PFDEF_MORPHED)
    fprintf(fl, "Mrph: %d\n", IS_MORPHED(ch));
  fprintf(fl, "God : %d\n", GET_DEITY(ch));

  if (GET_AUTOCQUEST_VNUM(ch) != PFDEF_AUTOCQUEST_VNUM)
    fprintf(fl, "Cvnm: %d\n", GET_AUTOCQUEST_VNUM(ch));
  if (GET_AUTOCQUEST_MAKENUM(ch) != PFDEF_AUTOCQUEST_MAKENUM)
    fprintf(fl, "Cmnm: %d\n", GET_AUTOCQUEST_MAKENUM(ch));
  if (GET_AUTOCQUEST_QP(ch) != PFDEF_AUTOCQUEST_QP)
    fprintf(fl, "Cqps: %d\n", GET_AUTOCQUEST_QP(ch));
  if (GET_AUTOCQUEST_EXP(ch) != PFDEF_AUTOCQUEST_EXP)
    fprintf(fl, "Cexp: %d\n", GET_AUTOCQUEST_EXP(ch));
  if (GET_AUTOCQUEST_GOLD(ch) != PFDEF_AUTOCQUEST_GOLD)
    fprintf(fl, "Cgld: %d\n", GET_AUTOCQUEST_GOLD(ch));
  if (GET_AUTOCQUEST_DESC(ch) != PFDEF_AUTOCQUEST_DESC)
    fprintf(fl, "Cdsc: %s\n", GET_AUTOCQUEST_DESC(ch));
  if (GET_AUTOCQUEST_MATERIAL(ch) != PFDEF_AUTOCQUEST_MATERIAL)
    fprintf(fl, "Cmat: %d\n", GET_AUTOCQUEST_MATERIAL(ch));

  if (EFREETI_MAGIC_USES(ch) != PFDEF_EFREETI_MAGIC_USES)
    fprintf(fl, "EfMU: %d\n", EFREETI_MAGIC_USES(ch));
  if (EFREETI_MAGIC_TIMER(ch) != PFDEF_EFREETI_MAGIC_TIMER)
    fprintf(fl, "EfMT: %d\n", EFREETI_MAGIC_TIMER(ch));
  if (GET_ENCUMBRANCE_MOD(ch) != 0)
    fprintf(fl, "EncM: %d\n", GET_ENCUMBRANCE_MOD(ch));
  if (GET_ELDRITCH_ESSENCE(ch) != 0)
    fprintf(fl, "EldE: %d\n", GET_ELDRITCH_ESSENCE(ch));
  if (GET_ELDRITCH_SHAPE(ch) != 0)
    fprintf(fl, "EldS: %d\n", GET_ELDRITCH_SHAPE(ch));
  if (GET_DR_MOD(ch) > 0)
    fprintf(fl, "DRMd: %d\n", GET_DR_MOD(ch));

  if (DRAGON_MAGIC_USES(ch) != PFDEF_DRAGON_MAGIC_USES)
    fprintf(fl, "DrMU: %d\n", DRAGON_MAGIC_USES(ch));
  if (DRAGON_MAGIC_TIMER(ch) != PFDEF_DRAGON_MAGIC_TIMER)
    fprintf(fl, "DrMT: %d\n", DRAGON_MAGIC_TIMER(ch));
  if (PIXIE_DUST_USES(ch) != PFDEF_PIXIE_DUST_USES)
    fprintf(fl, "PxDU: %d\n", PIXIE_DUST_USES(ch));
  if (PIXIE_DUST_TIMER(ch) != PFDEF_PIXIE_DUST_TIMER)
    fprintf(fl, "PxDT: %d\n", PIXIE_DUST_TIMER(ch));

  if (LAUGHING_TOUCH_USES(ch) != PFDEF_LAUGHING_TOUCH_USES)
    fprintf(fl, "LTCU: %d\n", LAUGHING_TOUCH_USES(ch));
  if (LAUGHING_TOUCH_TIMER(ch) != PFDEF_LAUGHING_TOUCH_TIMER)
    fprintf(fl, "LTCT: %d\n", LAUGHING_TOUCH_TIMER(ch));

  if (FLEETING_GLANCE_USES(ch) != PFDEF_FLEETING_GLANCE_USES)
    fprintf(fl, "FLGU: %d\n", FLEETING_GLANCE_USES(ch));
  if (FLEETING_GLANCE_TIMER(ch) != PFDEF_FLEETING_GLANCE_TIMER)
    fprintf(fl, "FLGT: %d\n", FLEETING_GLANCE_TIMER(ch));

  if (FEY_SHADOW_WALK_USES(ch) != PFDEF_FEY_SHADOW_WALK_USES)
    fprintf(fl, "FSWU: %d\n", FEY_SHADOW_WALK_USES(ch));
  if (FEY_SHADOW_WALK_TIMER(ch) != PFDEF_FEY_SHADOW_WALK_TIMER)
    fprintf(fl, "FSWT: %d\n", FEY_SHADOW_WALK_TIMER(ch));

  if (GET_FAST_HEALING_MOD(ch) != 0)
    fprintf(fl, "FstH: %d\n", GET_FAST_HEALING_MOD(ch));
  if (GRAVE_TOUCH_USES(ch) != PFDEF_GRAVE_TOUCH_USES)
    fprintf(fl, "GTCU: %d\n", GRAVE_TOUCH_USES(ch));
  if (GRAVE_TOUCH_TIMER(ch) != PFDEF_GRAVE_TOUCH_TIMER)
    fprintf(fl, "GTCT: %d\n", GRAVE_TOUCH_TIMER(ch));

  if (GRASP_OF_THE_DEAD_USES(ch) != PFDEF_GRASP_OF_THE_DEAD_USES)
    fprintf(fl, "GODU: %d\n", GRASP_OF_THE_DEAD_USES(ch));
  if (GRASP_OF_THE_DEAD_TIMER(ch) != PFDEF_GRASP_OF_THE_DEAD_TIMER)
    fprintf(fl, "GODT: %d\n", GRASP_OF_THE_DEAD_TIMER(ch));

  if (INCORPOREAL_FORM_USES(ch) != PFDEF_INCORPOREAL_FORM_USES)
    fprintf(fl, "InFU: %d\n", INCORPOREAL_FORM_USES(ch));
  if (INCORPOREAL_FORM_TIMER(ch) != PFDEF_INCORPOREAL_FORM_TIMER)
    fprintf(fl, "InFT: %d\n", INCORPOREAL_FORM_TIMER(ch));

  if (GET_PSIONIC_ENERGY_TYPE(ch) != PFDEF_PSIONIC_ENERGY_TYPE)
    fprintf(fl, "PsET: %d\n", GET_PSIONIC_ENERGY_TYPE(ch));

  if (GET_OLC_ZONE(ch) != PFDEF_OLC)
    fprintf(fl, "Olc : %d\n", GET_OLC_ZONE(ch));
  if (GET_PAGE_LENGTH(ch) != PFDEF_PAGELENGTH)
    fprintf(fl, "Page: %d\n", GET_PAGE_LENGTH(ch));
  if (GET_SCREEN_WIDTH(ch) != PFDEF_SCREENWIDTH)
    fprintf(fl, "ScrW: %d\n", GET_SCREEN_WIDTH(ch));
  if (GET_QUESTPOINTS(ch) != PFDEF_QUESTPOINTS)
    fprintf(fl, "Qstp: %d\n", GET_QUESTPOINTS(ch));
  if (GET_QUEST_COUNTER(ch, 0) != PFDEF_QUESTCOUNT)
    fprintf(fl, "Qcnt: %d\n", GET_QUEST_COUNTER(ch, 0));
  if (GET_QUEST_COUNTER(ch, 1) != PFDEF_QUESTCOUNT)
    fprintf(fl, "Qcn1: %d\n", GET_QUEST_COUNTER(ch, 1));
  if (GET_QUEST_COUNTER(ch, 2) != PFDEF_QUESTCOUNT)
    fprintf(fl, "Qcn2: %d\n", GET_QUEST_COUNTER(ch, 2));
  if (GET_NUM_QUESTS(ch) != PFDEF_COMPQUESTS)
  {
    fprintf(fl, "Qest:\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
      fprintf(fl, "%d\n", ch->player_specials->saved.completed_quests[i]);
    fprintf(fl, "%d\n", NOTHING);
  }
  if (GET_QUEST(ch, 0) != PFDEF_CURRQUEST)
    fprintf(fl, "Qcur: %d\n", GET_QUEST(ch, 0));
  if (GET_QUEST(ch, 1) != PFDEF_CURRQUEST)
    fprintf(fl, "Qcu1: %d\n", GET_QUEST(ch, 1));
  if (GET_QUEST(ch, 2) != PFDEF_CURRQUEST)
    fprintf(fl, "Qcu2: %d\n", GET_QUEST(ch, 2));
  if (GET_DIPTIMER(ch) != PFDEF_DIPTIMER)
    fprintf(fl, "DipT: %d\n", GET_DIPTIMER(ch));
  if (GET_CLAN(ch) != PFDEF_CLAN)
    fprintf(fl, "Cln : %d\n", GET_CLAN(ch));
  if (GET_CLANRANK(ch) != PFDEF_CLANRANK)
    fprintf(fl, "Clrk: %d\n", GET_CLANRANK(ch));
  if (GET_CLANPOINTS(ch) != PFDEF_CLANPOINTS)
    fprintf(fl, "CPts: %d\n", GET_CLANPOINTS(ch));
  if (GET_SLAYER_JUDGEMENT(ch) != 0)
    fprintf(fl, "Slyr: %d\n", GET_SLAYER_JUDGEMENT(ch));
  if (GET_BANE_TARGET_TYPE(ch) != 0)
    fprintf(fl, "Bane: %d\n", GET_BANE_TARGET_TYPE(ch));
  if (SCRIPT(ch))
  {
    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      fprintf(fl, "Trig: %d\n", GET_TRIG_VNUM(t));
  }

  if (ch->desc)
  {
    fprintf(fl, "GMCP: %d\n", ch->desc->pProtocol->bGMCP);
    fprintf(fl, "XTrm: %d\n", ch->desc->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt);
    fprintf(fl, "UTF8: %d\n", ch->desc->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt);
  }

  if (GET_PREMADE_BUILD_CLASS(ch) != PFDEF_PREMADE_BUILD)
    fprintf(fl, "PreB: %d\n", GET_PREMADE_BUILD_CLASS(ch));

  /* Save skills */
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    fprintf(fl, "Skil:\n");
    for (i = 1; i < MAX_SKILLS; i++)
    {
      if (GET_SKILL(ch, i))
        fprintf(fl, "%d %d\n", i, GET_SKILL(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  /* Save abilities */
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    fprintf(fl, "Ablt:\n");
    for (i = 1; i <= MAX_ABILITIES; i++)
    {
      if (GET_ABILITY(ch, i))
        fprintf(fl, "%d %d\n", i, GET_ABILITY(ch, i));
    }
    fprintf(fl, "0 0\n");
  }

  // Save Buffs
  fprintf(fl, "Buff:\n");
  for (i = 0; i < MAX_BUFFS; i++)
    fprintf(fl, "%d %d %d\n", i, GET_BUFF(ch, i, 0), GET_BUFF(ch, i, 1));
  fprintf(fl, "-1 -1 -1\n");

  /* Save Bombs */
  fprintf(fl, "Bomb:\n");
  for (i = 0; i < MAX_BOMBS_ALLOWED; i++)
    fprintf(fl, "%d\n", GET_BOMB(ch, i));
  fprintf(fl, "-1\n");

  // Save consumables: potions, scrolls, wands and staves
  fprintf(fl, "Potn:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_POTIONS(ch, i) > 0)
      fprintf(fl, "%d %d\n", i, STORED_POTIONS(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Scrl:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_SCROLLS(ch, i) > 0)
      fprintf(fl, "%d %d\n", i, STORED_SCROLLS(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Stav:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_STAVES(ch, i) > 0)
      fprintf(fl, "%d %d\n", i, STORED_STAVES(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Wand:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_WANDS(ch, i) > 0)
      fprintf(fl, "%d %d\n", i, STORED_WANDS(ch, i));
  fprintf(fl, "-1\n");
  // End save consumables

  fprintf(fl, "Disc:\n");
  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
    fprintf(fl, "%d\n", KNOWS_DISCOVERY(ch, i));
  fprintf(fl, "-1\n");
  fprintf(fl, "GrDs: %d\n", GET_GRAND_DISCOVERY(ch));

  fprintf(fl, "Mrcy:\n");
  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    fprintf(fl, "%d\n", KNOWS_MERCY(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Clty:\n");
  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    fprintf(fl, "%d\n", KNOWS_CRUELTY(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Judg:\n");
  for (i = 0; i < NUM_INQ_JUDGEMENTS; i++)
    fprintf(fl, "%d\n", IS_JUDGEMENT_ACTIVE(ch, i));
  fprintf(fl, "-1\n");

  fprintf(fl, "Lang:\n");
  for (i = 0; i < NUM_LANGUAGES; i++)
    fprintf(fl, "%d\n", ch->player_specials->saved.languages_known[i]);
  fprintf(fl, "-1\n");

  /* Save Combat Feats */
  for (i = 0; i < NUM_CFEATS; i++)
  {
    sprintascii(bits, ch->char_specials.saved.combat_feats[i][0]);
    sprintascii(bits2, ch->char_specials.saved.combat_feats[i][1]);
    sprintascii(bits3, ch->char_specials.saved.combat_feats[i][2]);
    sprintascii(bits4, ch->char_specials.saved.combat_feats[i][3]);
    fprintf(fl, "CbFt: %d %s %s %s %s\n", i, bits, bits2, bits3, bits4);
  }

  /* Save School Feats */
  for (i = 0; i < NUM_SFEATS; i++)
  {
    sprintascii(bits, ch->char_specials.saved.school_feats[i]);
    fprintf(fl, "SclF: %d %s\n", i, bits);
  }

  /* Save Skill Foci */
  fprintf(fl, "SklF:\n");
  for (i = 0; i < MAX_ABILITIES; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_SKFEATS; j++)
    {
      fprintf(fl, "%d ", ch->player_specials->saved.skill_focus[i][j]);
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "-1 -1 -1\n");

  /* Save feats */
  fprintf(fl, "Feat:\n");
  for (i = 1; i < NUM_FEATS; i++)
  {
    if (HAS_REAL_FEAT(ch, i))
      fprintf(fl, "%d %d\n", i, HAS_REAL_FEAT(ch, i));
  }
  fprintf(fl, "0 0\n");

  /* spell prep system */
  save_spell_prep_queue(fl, ch);
  save_innate_magic_queue(fl, ch);
  save_spell_collection(fl, ch);
  save_known_spells(fl, ch);
  /* end spell prep system */

  // Save memorizing list of prayers, prayed list and times
  /* Note: added metamagic to pfile.  19.01.2015 Ornir */
  fprintf(fl, "Pryg:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      if (PREPARATION_QUEUE(ch, i, j).spell < MAX_SPELLS)
        fprintf(fl, "%d ", PREPARATION_QUEUE(ch, i, j).spell);
      else
        fprintf(fl, "0 ");
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "Prgm:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      if (PREPARATION_QUEUE(ch, i, j).spell < MAX_SPELLS)
        fprintf(fl, "%d ", PREPARATION_QUEUE(ch, i, j).metamagic);
      else
        fprintf(fl, "0 ");
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "-1 -1\n");
  fprintf(fl, "Pryd:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      fprintf(fl, "%d ", PREPARED_SPELLS(ch, i, j).spell);
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "-1 -1\n");

  fprintf(fl, "Pryt:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      fprintf(fl, "%d ", PREP_TIME(ch, i, j));
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "-1 -1\n");

  fprintf(fl, "Prdm:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    fprintf(fl, "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      fprintf(fl, "%d ", PREPARED_SPELLS(ch, i, j).metamagic);
    }
    fprintf(fl, "\n");
  }
  fprintf(fl, "-1 -1\n");

  // class levels
  fprintf(fl, "CLvl:\n");
  for (i = 0; i < MAX_CLASSES; i++)
  {
    fprintf(fl, "%d %d\n", i, CLASS_LEVEL(ch, i));
  }
  fprintf(fl, "-1 -1\n");

  // coordinate location
  fprintf(fl, "CLoc:\n");
  fprintf(fl, "%d %d\n", ch->coords[0], ch->coords[1]);

  // warding, etc..
  fprintf(fl, "Ward:\n");
  for (i = 0; i < MAX_WARDING; i++)
  {
    fprintf(fl, "%d %d\n", i, GET_WARDING(ch, i));
  }
  fprintf(fl, "-1 -1\n");

  // spec abilities
  fprintf(fl, "SpAb:\n");
  for (i = 0; i < MAX_CLASSES; i++)
  {
    fprintf(fl, "%d %d\n", i, GET_SPEC_ABIL(ch, i));
  }
  fprintf(fl, "-1 -1\n");

  // favored enemies (rangers)
  fprintf(fl, "FaEn:\n");
  for (i = 0; i < MAX_ENEMIES; i++)
  {
    fprintf(fl, "%d %d\n", i, GET_FAVORED_ENEMY(ch, i));
  }
  fprintf(fl, "-1 -1\n");

  /* save_char(x, 1) will skip this block (i.e. not saving events)
     this is necessary due to clearing events that occurs immediately
     before extract_char_final() in extract_char() -Zusuk */
  if (mode != 1)
  {
    /* Save events */
    /* Not going to save every event */
    fprintf(fl, "Evnt:\n");
    /* Order:  Event-ID   Duration */
    /* eSTRUGGLE - don't need to save this */
    if ((pMudEvent = char_has_mud_event(ch, eINVISIBLE_ROGUE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eVANISHED)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eVANISH)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTAUNT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTAUNTED)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINTIMIDATED)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINTIMIDATE_COOLDOWN)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eRAGE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSACRED_FLAMES)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINNER_FIRE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eMUTAGEN)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCRIPPLING_CRITICAL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDEFENSIVE_STANCE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINSECTBEING)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCRYSTALFIST)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCRYSTALBODY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_STRENGTH)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_ENLARGE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_INVIS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_LEVITATE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_DARKNESS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_FAERIE_FIRE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eLAYONHANDS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTOUCHOFCORRUPTION)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eJUDGEMENT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTRUEJUDGEMENT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCHILDRENOFTHENIGHT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eVAMPIREENERGYDRAIN)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eVAMPIREBLOODDRAIN)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eBANE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eMASTERMIND)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDANCINGWEAPON)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSPIRITUALWEAPON)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCHANNELENERGY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eEMPTYBODY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eWHOLENESSOFBODY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eRENEWEDDEFENSE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eRENEWEDVIGOR)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTREATINJURY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eMUMMYDUST)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRAGONKNIGHT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eGREATERRUIN)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eHELLBALL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eEPICMAGEARMOR)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eEPICWARDING)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDEATHARROW)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eQUIVERINGPALM)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eANIMATEDEAD)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSTUNNINGFIST)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSURPRISE_ACCURACY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCOME_AND_GET_ME)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, ePOWERFUL_BLOW)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eD_ROLL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eLAST_WORD)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, ePURIFY)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eC_ANIMAL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eC_FAMILIAR)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eC_MOUNT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSUMMONSHADOW)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTURN_UNDEAD)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSPELLBATTLE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eQUEST_COMPLETE)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRACBREATH)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRACCLAWS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRAGBREATH)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCATSCLAWS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eARCANEADEPT)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCHANNELSPELL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, ePSIONICFOCUS)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDOUBLEMANIFEST)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWCALL)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWJUMP)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWILLUSION)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWPOWER)))
      fprintf(fl, "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));

    fprintf(fl, "-1 -1\n");
  }

  /* Save affects */
  if (tmp_aff[0].spell > 0)
  {
    fprintf(fl, "Affs:\n");
    for (i = 0; i < MAX_AFFECT; i++)
    {
      aff = &tmp_aff[i];
      if (aff->spell)
        fprintf(fl,
                "%d %d %d %d %d %d %d %d %d %d\n",
                aff->spell,
                aff->duration,
                aff->modifier,
                aff->location,
                aff->bitvector[0],
                aff->bitvector[1],
                aff->bitvector[2],
                aff->bitvector[3],
                aff->bonus_type,
                aff->specific);
    }
    fprintf(fl, "0 0 0 0 0 0 0 0 0 0\n");
  }

  /* Save Damage Reduction */
  if ((tmp_dr != NULL) || (GET_DR(ch) != NULL))
  {
    struct damage_reduction_type *dr;
    int k = 0, x = 0;
    int max_loops = 100; /* zusuk put this here to limit the loop, we were having issues with pfiles 10/27/22 */
    int snum[100];
    bool found = false;

    fprintf(fl, "DmgR:\n");

    /* DR from affects...*/
    for (dr = tmp_dr; dr != NULL && 0 <= max_loops--; dr = dr->next)
    {
      // dupe check -- only want one DR entry per spell/ability/power
      found = false;
      for (x = 0; x < 100; x++)
      {
        if (snum[x] == dr->spell)
        {
          found = true;
          break;
        }
        else if (snum[x] == 0)
        {
          snum[x] = dr->spell;
          break;
        }
      }
      if (found || x == 100)
        continue;

      fprintf(fl, "1 %d %d %d %d\n", dr->amount, dr->max_damage, dr->spell, dr->feat);
      for (k = 0; k < MAX_DR_BYPASS; k++)
      {
        fprintf(fl, "%d %d\n", dr->bypass_cat[k], dr->bypass_val[k]);
      }
    }

    /* reset our counter */
    max_loops = 100;

    /* Permanent DR. */
    for (dr = GET_DR(ch); dr != NULL && 0 <= max_loops--; dr = dr->next)
    {
      // dupe check -- only want one DR entry per spell/ability/power
      found = false;
      for (x = 0; x < 100; x++)
      {
        if (snum[x] == dr->spell)
        {
          found = true;
          break;
        }
        else if (snum[x] == 0)
        {
          snum[x] = dr->spell;
          break;
        }
      }
      if (found || x == 100)
        continue;
      fprintf(fl, "1 %d %d %d %d\n", dr->amount, dr->max_damage, dr->spell, dr->feat);
      for (k = 0; k < MAX_DR_BYPASS; k++)
      {
        fprintf(fl, "%d %d\n", dr->bypass_cat[k], dr->bypass_val[k]);
      }
    }

    /* done close off */
    fprintf(fl, "0 0 0 0 0\n");
  }
  /* end DR saving */

  write_aliases_ascii(fl, ch);
  save_char_vars_ascii(fl, ch);

  /* Save account data
     Trying this before file gets closed, before use to be
     at very end of this function 11/28/2017 -Zusuk*/
  if (ch->desc && ch->desc->account)
  {
    for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
    {
      if (ch->desc->account->character_names[i] != NULL &&
          !strcmp(ch->desc->account->character_names[i], GET_NAME(ch)))
        break;
      if (ch->desc->account->character_names[i] == NULL)
        break;
    }

    if (i != MAX_CHARS_PER_ACCOUNT && !IS_SET_AR(PLR_FLAGS(ch), PLR_DELETED))
      ch->desc->account->character_names[i] = strdup(GET_NAME(ch));
    save_account(ch->desc->account);
  }

  /* FILE CLOSED!!! */
  fclose(fl);

  /* add affects, dr, etc back in */

  /* More char_to_store code to add spell and eq affections back in. */
  for (i = 0; i < MAX_AFFECT; i++)
  {
    if (tmp_aff[i].spell)
      affect_to_char(ch, &tmp_aff[i]);
  }

  /* Reapply dr.*/
  if (tmp_dr != NULL)
  {
    GET_DR(ch) = tmp_dr;
  }

  // This will prevent things like item special abilities that send messages to the character
  // when they equip it, since equipment is removed then re-equipped when saving character
  ch->mute_equip_messages = TRUE;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (char_eq[i])
#ifndef NO_EXTRANEOUS_TRIGGERS
      if (wear_otrigger(char_eq[i], ch, i))
#endif
        equip_char(ch, char_eq[i], i);
#ifndef NO_EXTRANEOUS_TRIGGERS
      else
        obj_to_char(char_eq[i], ch);
#endif
  }

  ch->mute_equip_messages = FALSE;

  /* end char_to_store code */

  if ((id = get_ptable_by_name(GET_NAME(ch))) < 0)
    return;

  /* update the player in the player index */
  if (player_table[id].level != GET_LEVEL(ch))
  {
    save_index = TRUE;
    player_table[id].level = GET_LEVEL(ch);
  }
  if (player_table[id].last != ch->player.time.logon)
  {
    save_index = TRUE;
    player_table[id].last = ch->player.time.logon;
  }
  i = player_table[id].flags;
  if (PLR_FLAGGED(ch, PLR_DELETED))
    SET_BIT(player_table[id].flags, PINDEX_DELETED);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_DELETED);
  if (PLR_FLAGGED(ch, PLR_NODELETE) || PLR_FLAGGED(ch, PLR_CRYO))
    SET_BIT(player_table[id].flags, PINDEX_NODELETE);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NODELETE);

  if (PLR_FLAGGED(ch, PLR_FROZEN) || PLR_FLAGGED(ch, PLR_NOWIZLIST))
    SET_BIT(player_table[id].flags, PINDEX_NOWIZLIST);
  else
    REMOVE_BIT(player_table[id].flags, PINDEX_NOWIZLIST);

  if (player_table[id].flags != i || save_index)
    save_player_index();
}

/* Separate a 4-character id tag from the data it precedes */
void tag_argument(char *argument, char *tag)
{
  char *tmp = argument, *ttag = tag, *wrt = argument;
  int i;

  for (i = 0; i < 4; i++)
    *(ttag++) = *(tmp++);
  *ttag = '\0';

  while (*tmp == ':' || *tmp == ' ')
    tmp++;

  while (*tmp)
    *(wrt++) = *(tmp++);
  *wrt = '\0';
}

/* Stuff related to the player file cleanup system. */

/* remove_player() removes all files associated with a player who is self-deleted,
 * deleted by an immortal, or deleted by the auto-wipe system (if enabled). */
void remove_player(int pfilepos)
{
  char filename[MAX_STRING_LENGTH] = {'\0'};
  int i;

  if (!*player_table[pfilepos].name)
    return;

  /* Unlink all player-owned files */
  for (i = 0; i < MAX_FILES; i++)
  {
    if (get_filename(filename, sizeof(filename), i, player_table[pfilepos].name))
      unlink(filename);
  }

  log("PCLEAN: %s Lev: %d Last: %s",
      player_table[pfilepos].name, player_table[pfilepos].level,
      asctime(localtime(&player_table[pfilepos].last)));
  player_table[pfilepos].name[0] = '\0';

  /* Update index table. */
  remove_player_from_index(pfilepos);

  save_player_index();
}

void clean_pfiles(void)
{
  int i, ci;

  for (i = 0; i <= top_of_p_table; i++)
  {
    /* We only want to go further if the player isn't protected from deletion
     * and hasn't already been deleted. */
    if (!IS_SET(player_table[i].flags, PINDEX_NODELETE) &&
        *player_table[i].name)
    {
      /* If the player is already flagged for deletion, then go ahead and get
       * rid of him. */
      if (IS_SET(player_table[i].flags, PINDEX_DELETED))
      {
        remove_player(i);
      }
      else
      {
        /* Check to see if the player has overstayed his welcome based on level. */
        for (ci = 0; pclean_criteria[ci].level > -1; ci++)
        {
          if (player_table[i].level <= pclean_criteria[ci].level &&
              ((time(0) - player_table[i].last) >
               (pclean_criteria[ci].days * SECS_PER_REAL_DAY)))
          {
            remove_player(i);
            break;
          }
        }
        /* If we got this far and the players hasn't been kicked out, then he
         * can stay a little while longer. */
      }
    }
  }
  /* After everything is done, we should rebuild player_index and remove the
   * entries of the players that were just deleted. */
}

/* Load Damage Reduction - load_dr */
static void load_dr(FILE *f1, struct char_data *ch)
{
  struct damage_reduction_type *dr;
  int i, num, num2, num3, num4, num5, n_vars;
  char line[MAX_INPUT_LENGTH + 1];
  int max_loops = 200; /* zusuk put this here to limit the loop, we were having issues with pfiles 10/27/22 */

  do
  {
    get_line(f1, line);
    n_vars = sscanf(line, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
    if (num > 0)
    {
      /* Set the DR data.*/
      CREATE(dr, struct damage_reduction_type, 1);

      if (n_vars == 5)
      {
        dr->amount = num2;
        dr->max_damage = num3;
        dr->spell = num4;
        dr->feat = num5;

        for (i = 0; i < MAX_DR_BYPASS; i++)
        {
          get_line(f1, line);
          n_vars = sscanf(line, "%d %d", &num2, &num3);
          if (n_vars == 2)
          {
            dr->bypass_cat[i] = num2;
            dr->bypass_val[i] = num3;
          }
          else
          {
            log("SYSERR: Invalid dr bypass in pfile (%s), expecting 2 values", GET_NAME(ch));
          }
        }
        dr->next = GET_DR(ch);
        GET_DR(ch) = dr;
      }
      else
      {
        log("SYSERR: Invalid dr in pfile (%s), expecting 5 values", GET_NAME(ch));
      }
    }
  } while (num != 0 && 0 <= max_loops--);
}

/* load_affects function now handles both 32-bit and
   128-bit affect bitvectors for backward compatibility */
static void load_affects(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0, num7 = 0,
      num8 = 0, num9 = 0, i, n_vars, num10, num11, num12, num13, num14, num15;
  char line[MAX_INPUT_LENGTH + 1];
  struct affected_type af;

  i = 0;
  do
  {
    new_affect(&af);
    get_line(fl, line);
    n_vars = sscanf(line, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &num,
                    &num2, &num3, &num4, &num5, &num6, &num7, &num8, &num9, &num10, &num11,
                    &num12, &num13, &num14, &num15);
    if (num > 0)
    {
      af.spell = num;
      af.duration = num2;
      af.modifier = num3;
      af.location = num4;
      if (n_vars == 10)
      { /* Version with bonus type! */
        af.bitvector[0] = num5;
        af.bitvector[1] = num6;
        af.bitvector[2] = num7;
        af.bitvector[3] = num8;
        af.bonus_type = num9;
        af.specific = num10;
      }
      else if (n_vars == 9)
      { /* Version with bonus type! */
        af.bitvector[0] = num5;
        af.bitvector[1] = num6;
        af.bitvector[2] = num7;
        af.bitvector[3] = num8;
        af.bonus_type = num9;
      }
      else if (n_vars == 8)
      { /* New 128-bit version */
        af.bitvector[0] = num5;
        af.bitvector[1] = num6;
        af.bitvector[2] = num7;
        af.bitvector[3] = num8;
      }
      else if (n_vars == 5)
      {                                        /* Old 32-bit conversion version */
        if (num5 > 0 && num5 <= NUM_AFF_FLAGS) /* Ignore invalid values */
          SET_BIT_AR(af.bitvector, num5);
      }
      else
      {
        log("SYSERR: Invalid affects in pfile (%s), expecting 5, 8, 9 values, got %d", GET_NAME(ch), n_vars);
      }
      affect_to_char(ch, &af);
      i++;
    }
  } while (num != 0);
}

/* praytimes loading isn't a loop, so has to be manually changed if you
   change NUM_CASTERS! */
static void load_praytimes(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0,
      num7 = 0, num8 = 0;
  int counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    num2 = 0;
    num3 = 0;
    num4 = 0;
    num5 = 0;
    num6 = 0;
    num7 = 0;
    num8 = 0;
    get_line(fl, line);

    sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5,
           &num6, &num7, &num8);
    if (num != -1)
    {
      PREP_TIME(ch, num, 0) = num2;
      PREP_TIME(ch, num, 1) = num3;
      PREP_TIME(ch, num, 2) = num4;
      PREP_TIME(ch, num, 3) = num5;
      PREP_TIME(ch, num, 4) = num6;
      PREP_TIME(ch, num, 5) = num7;
      PREP_TIME(ch, num, 6) = num8;
    }
    counter++;
  } while (counter < MAX_MEM && num != -1);
}

/* prayed loading isn't a loop, so has to be manually changed if you
   change NUM_CASTERS! */
static void load_prayed_metamagic(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0,
      num7 = 0, num8 = 0;
  int counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    num2 = 0;
    num3 = 0;
    num4 = 0;
    num5 = 0;
    num6 = 0;
    num7 = 0;
    num8 = 0;
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5,
           &num6, &num7, &num8);
    if (num != -1)
    {
      PREPARED_SPELLS(ch, num, 0).metamagic = num2;
      PREPARED_SPELLS(ch, num, 1).metamagic = num3;
      PREPARED_SPELLS(ch, num, 2).metamagic = num4;
      PREPARED_SPELLS(ch, num, 3).metamagic = num5;
      PREPARED_SPELLS(ch, num, 4).metamagic = num6;
      PREPARED_SPELLS(ch, num, 5).metamagic = num7;
      PREPARED_SPELLS(ch, num, 6).metamagic = num8;
    }
    counter++;
  } while (counter < MAX_MEM && num != -1);
}

/* prayed loading isn't a loop, so has to be manually changed if you
   change NUM_CASTERS! */
static void load_prayed(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0,
      num7 = 0, num8 = 0;
  int counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    num2 = 0;
    num3 = 0;
    num4 = 0;
    num5 = 0;
    num6 = 0;
    num7 = 0;
    num8 = 0;
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5,
           &num6, &num7, &num8);
    if (num != -1)
    {
      PREPARED_SPELLS(ch, num, 0).spell = num2;
      PREPARED_SPELLS(ch, num, 1).spell = num3;
      PREPARED_SPELLS(ch, num, 2).spell = num4;
      PREPARED_SPELLS(ch, num, 3).spell = num5;
      PREPARED_SPELLS(ch, num, 4).spell = num6;
      PREPARED_SPELLS(ch, num, 5).spell = num7;
      PREPARED_SPELLS(ch, num, 6).spell = num8;
    }
    counter++;
  } while (counter < MAX_MEM && num != -1);
}

/* praying loading isn't a loop, so has to be manually changed if you
   change NUM_CASTERS! */
static void load_praying(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0,
      num7 = 0, num8 = 0;
  char line[MAX_INPUT_LENGTH + 1];
  int counter = 0;

  do
  {
    num2 = 0;
    num3 = 0;
    num4 = 0;
    num5 = 0;
    num6 = 0;
    num7 = 0;
    num8 = 0;
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5,
           &num6, &num7, &num8);
    if (num != -1)
    {
      if (num2 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 0).spell = num2;
      if (num3 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 1).spell = num3;
      if (num4 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 2).spell = num4;
      if (num5 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 3).spell = num5;
      if (num6 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 4).spell = num6;
      if (num7 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 5).spell = num7;
      if (num8 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 6).spell = num8;
    }
    counter++;
  } while (num != -1 && counter < MAX_MEM);
}

/* praying loading isn't a loop, so has to be manually changed if you
   change NUM_CASTERS! */
static void load_praying_metamagic(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0, num6 = 0,
      num7 = 0, num8 = 0;
  char line[MAX_INPUT_LENGTH + 1];
  int counter = 0;

  do
  {
    num2 = 0;
    num3 = 0;
    num4 = 0;
    num5 = 0;
    num6 = 0;
    num7 = 0;
    num8 = 0;
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d %d %d %d", &num, &num2, &num3, &num4, &num5,
           &num6, &num7, &num8);
    if (num != -1)
    {
      if (num2 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 0).metamagic = num2;
      if (num3 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 1).metamagic = num3;
      if (num4 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 2).metamagic = num4;
      if (num5 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 3).metamagic = num5;
      if (num6 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 4).metamagic = num6;
      if (num7 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 5).metamagic = num7;
      if (num8 < MAX_SPELLS)
        PREPARATION_QUEUE(ch, num, 6).metamagic = num8;
    }
    counter++;
  } while (num != -1 && counter < MAX_MEM);
}

static void load_class_level(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != -1)
      CLASS_LEVEL(ch, num) = num2;
  } while (num != -1);
}

static void load_coord_location(FILE *fl, struct char_data *ch)
{
  char line[MAX_INPUT_LENGTH + 1];

  get_line(fl, line);
  sscanf(line, "%d %d", ch->coords, ch->coords + 1);
}

static void load_warding(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != -1)
      GET_WARDING(ch, num) = num2;
  } while (num != -1);
}

static void load_spec_abil(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != -1)
      GET_SPEC_ABIL(ch, num) = num2;
  } while (num != -1);
}

static void load_mercies(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      KNOWS_MERCY(ch, i) = num;
      i++;
    }
  } while (num != -1);
}

static void load_cruelties(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      KNOWS_CRUELTY(ch, i) = num;
      i++;
    }
  } while (num != -1);
}

static void load_languages(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      ch->player_specials->saved.languages_known[i] = num;
      i++;
    }
  } while (num != -1);
}

static void load_discoveries(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      KNOWS_DISCOVERY(ch, i) = num;
      i++;
    }
  } while (num != -1);
}

static void load_judgements(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      IS_JUDGEMENT_ACTIVE(ch, i) = num;
      i++;
    }
  } while (num != -1);
}

static void load_bombs(FILE *fl, struct char_data *ch)
{
  int num = 0;
  char line[MAX_INPUT_LENGTH + 1];

  int i = 0;

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
      GET_BOMB(ch, i) = num;
    i++;
  } while (num != -1);
}

static void load_favored_enemy(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != -1)
      GET_FAVORED_ENEMY(ch, num) = num2;
  } while (num != -1);
}

static void load_potions(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      STORED_POTIONS(ch, num) = num2;
  } while (num != -1);
}

static void load_buffs(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d", &num, &num2, &num3);
    GET_BUFF(ch, num, 0) = num2;
    GET_BUFF(ch, num, 1) = num3;
  } while (num != -1);
}

static void load_scrolls(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      STORED_SCROLLS(ch, num) = num2;
  } while (num != -1);
}

static void load_wands(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      STORED_WANDS(ch, num) = num2;
  } while (num != -1);
}

static void load_staves(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      STORED_STAVES(ch, num) = num2;
  } while (num != -1);
}

static void load_abilities(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      GET_ABILITY(ch, num) = num2;
  } while (num != 0);
}

static void load_skills(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, counter = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
    {

      /* this is a hack since we moved the skill numbering */
      if (num < START_SKILLS)
        num += 1600;

      if (counter >= (MAX_SKILLS + 1))
        ;
      else
      {
        GET_SKILL(ch, num) = num2;
      }
      /* end hack */

      counter++;
    }
  } while (num != 0 && counter < MAX_SKILLS);
}

void load_feats(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      SET_FEAT(ch, num, num2);
  } while (num != 0);
}

void load_class_feat_points(FILE *fl, struct char_data *ch)
{

  int cls = 0, pts = 0, num_fields = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);

    if ((num_fields = sscanf(line, "%d %d", &cls, &pts)) == 1)
      return;
    GET_CLASS_FEATS(ch, cls) = pts;
  } while (1);
}

void load_epic_class_feat_points(FILE *fl, struct char_data *ch)
{

  int cls = 0, pts = 0, num_fields = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);

    if ((num_fields = sscanf(line, "%d %d", &cls, &pts)) == 1)
      return;
    GET_EPIC_CLASS_FEATS(ch, cls) = pts;
  } while (1);
}

/* if NUM_SKFEATS changes, this must be modified manually */
void load_skill_focus(FILE *fl, struct char_data *ch)
{
  int skfeat = 0, skill = 0, skfeat_epic = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d", &skill, &skfeat, &skfeat_epic);
    if (skill != -1)
    {
      ch->player_specials->saved.skill_focus[skill][0] = skfeat;
      ch->player_specials->saved.skill_focus[skill][1] = skfeat_epic;
    }
  } while (skill != -1);
}

static void load_events(FILE *fl, struct char_data *ch)
{
  int num = 0;
  long num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %ld", &num, &num2);
    if (num != -1)
      attach_mud_event(new_mud_event(num, ch, NULL), num2);
  } while (num != -1);
}

void load_quests(FILE *fl, struct char_data *ch)
{
  int num = NOTHING;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != NOTHING)
      add_completed_quest(ch, num);
  } while (num != NOTHING);
}

static void load_HMVS(struct char_data *ch, const char *line, int mode)
{
  int num = 0, num2 = 0;

  sscanf(line, "%d/%d", &num, &num2);

  switch (mode)
  {
  case LOAD_HIT:
    GET_HIT(ch) = num;
    GET_REAL_MAX_HIT(ch) = num2;
    break;

  case LOAD_PSP:
    GET_PSP(ch) = num;
    GET_REAL_MAX_PSP(ch) = num2;
    break;

  case LOAD_MOVE:
    GET_MOVE(ch) = num;
    GET_REAL_MAX_MOVE(ch) = num2;
    break;

  case LOAD_STRENGTH:
    GET_REAL_STR(ch) = num;
    ch->real_abils.str_add = num2;
    break;
  }
}

static void write_aliases_ascii(FILE *file, struct char_data *ch)
{
  struct alias_data *temp;
  int count = 0;

  if (GET_ALIASES(ch) == NULL)
    return;

  for (temp = GET_ALIASES(ch); temp; temp = temp->next)
    count++;

  fprintf(file, "Alis: %d\n", count);

  for (temp = GET_ALIASES(ch); temp; temp = temp->next)
    fprintf(file, " %s\n" /* Alias: prepend a space in order to avoid issues with aliases beginning
                           * with * (get_line treats lines beginning with * as comments and ignores them */
                  "%s\n"  /* Replacement: always prepended with a space in memory anyway */
                  "%d\n", /* Type */
            temp->alias,
            temp->replacement,
            temp->type);
}

static void read_aliases_ascii(FILE *file, struct char_data *ch, int count)
{
  int i;

  if (count == 0)
  {
    GET_ALIASES(ch) = NULL;
    return; /* No aliases in the list. */
  }

  /* This code goes both ways for the old format (where alias and replacement start at the
   * first character on the line) and the new (where they are prepended by a space in order
   * to avoid the possibility of a * at the start of the line */
  for (i = 0; i < count; i++)
  {
    char abuf[MAX_INPUT_LENGTH + 1], rbuf[MAX_INPUT_LENGTH + 1], tbuf[MAX_INPUT_LENGTH] = {'\0'};

    /* Read the aliased command. */
    get_line(file, abuf);

    /* Read the replacement. This needs to have a space prepended before placing in
     * the in-memory struct. The space may be there already, but we can't be certain! */
    rbuf[0] = ' ';
    get_line(file, rbuf + 1);

    /* read the type */
    get_line(file, tbuf);

    if (abuf[0] && rbuf[1] && *tbuf)
    {
      struct alias_data *temp;
      CREATE(temp, struct alias_data, 1);
      temp->alias = strdup(abuf[0] == ' ' ? abuf + 1 : abuf);
      temp->replacement = strdup(rbuf[1] == ' ' ? rbuf + 1 : rbuf);
      temp->type = atoi(tbuf);
      temp->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = temp;
    }
  }
}

void update_player_last_on(void)
{

  struct descriptor_data *d = NULL;
  char buf[2048]; /* For MySQL insert. */

  for (d = descriptor_list; d; d = d->next)
  {

    if (!d || !d->character)
      continue;

    snprintf(buf, sizeof(buf), "UPDATE player_data SET last_online = NOW() "
                               "WHERE name = '%s';",
             GET_NAME(d->character));
    if (mysql_query(conn, buf))
    {
      log("SYSERR: Unable to UPDATE last_online for %s on PLAYER_DATA: %s", GET_NAME(d->character), mysql_error(conn));
    }
  }
}

void save_char_pets(struct char_data *ch)
{

  if (!ch)
    return;

  struct follow_type *f = NULL;
  struct char_data *tch = NULL;
  char query[MEDIUM_STRING] = {'\0'};
  char query2[MEDIUM_STRING] = {'\0'};
  char query3[MEDIUM_STRING] = {'\0'};
  char finalQuery[MEDIUM_STRING * 2];
  char *end = NULL, *end2 = NULL;

  mysql_ping(conn);

  end = stpcpy(query, "DELETE FROM pet_data WHERE owner_name=");
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, GET_NAME(ch), strlen(GET_NAME(ch)));
  *end++ = '\'';
  *end++ = '\0';

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to DELETE from pet_data: %s", mysql_error(conn));
  }

  if (!ch || !ch->desc || IS_NPC(ch))
    return;

  for (f = ch->followers; f; f = f->next)
  {
    tch = f->follower;
    if (!IS_NPC(tch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    snprintf(query2, sizeof(query2), "INSERT INTO pet_data (pet_data_id, owner_name, vnum, level, hp, max_hp, str, con, dex, ac) VALUES(NULL,");

    end2 = stpcpy(query2, "INSERT INTO pet_data (pet_data_id, owner_name, vnum, level, hp, max_hp, str, con, dex, ac) VALUES(NULL,");
    *end2++ = '\'';
    end2 += mysql_real_escape_string(conn, end2, GET_NAME(ch), strlen(GET_NAME(ch)));
    *end2++ = '\'';
    *end2++ = '\0';

    snprintf(query3, sizeof(query3), ",'%d','%d','%d','%d','%d','%d','%d','%d')",
             GET_MOB_VNUM(tch), GET_LEVEL(tch), GET_HIT(tch), GET_REAL_MAX_HIT(tch),
             GET_REAL_STR(tch), GET_REAL_CON(tch), GET_REAL_DEX(tch), GET_REAL_AC(tch));
    snprintf(finalQuery, sizeof(finalQuery), "%s%s", query2, query3);
    if (mysql_query(conn, finalQuery))
    {
      log("SYSERR: Unable to INSERT INTO pet_data: %s", mysql_error(conn));
      return;
    }
  }
}

void load_char_pets(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[200];
  struct char_data *mob = NULL;

  if (!ch)
    return;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  mysql_ping(conn);

  snprintf(query, sizeof(query), "SELECT vnum, level, hp, max_hp, str, con, dex, ac FROM pet_data WHERE owner_name='%s'", GET_NAME(ch));

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to SELECT from pet_data: %s", mysql_error(conn));
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: Unable to SELECT from pet_data: %s", mysql_error(conn));
    return;
  }

  while ((row = mysql_fetch_row(result)))
  {
    mob = read_mobile(atoi(row[0]), VIRTUAL);
    if (!mob)
      continue;
    if (isSummonMob(atoi(row[0])))
    {
      if (GET_LEVEL(mob) <= 10)
      {
        GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
        GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
        mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
        mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_1_10_HIT_DAM / 100;
      }
      else if (GET_LEVEL(mob) <= 20)
      {
        GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
        GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
        mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
        mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_11_20_HIT_DAM / 100;
      }
      else
      {
        GET_HITROLL(mob) = GET_HITROLL(mob) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
        GET_DAMROLL(mob) = GET_DAMROLL(mob) * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
        mob->mob_specials.damnodice = mob->mob_specials.damnodice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
        mob->mob_specials.damsizedice = mob->mob_specials.damsizedice * CONFIG_SUMMON_LEVEL_21_30_HIT_DAM / 100;
      }
    }
    log("Pet for %s: %s, loaded.", GET_NAME(ch), GET_NAME(mob));
    if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
    {
      X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
      Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
    }
    char_to_room(mob, IN_ROOM(ch));
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
    GET_LEVEL(mob) = atoi(row[1]);
    autoroll_mob(mob, TRUE, TRUE);
    if (GET_MOB_VNUM(mob) == 10) // MOB_CLONE define from magic.c
    {
      mob->player.name = strdup(GET_NAME(ch));
      mob->player.short_descr = strdup(GET_NAME(ch));
    }
    GET_REAL_STR(mob) = atoi(row[4]);
    GET_REAL_CON(mob) = atoi(row[5]);
    GET_REAL_DEX(mob) = atoi(row[6]);
    GET_REAL_AC(mob) = atoi(row[7]);
    GET_REAL_MAX_HIT(mob) = atoi(row[3]);
    GET_HIT(mob) = atoi(row[2]);
    load_mtrigger(mob);
    add_follower(mob, ch);
    if (GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));
    act("$N appears beside you.", true, ch, 0, mob, TO_CHAR);
    act("$N appears beside $n.", true, ch, 0, mob, TO_ROOM);
  }

  mysql_free_result(result);
}