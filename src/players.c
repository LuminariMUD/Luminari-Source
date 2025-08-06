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
#include "evolutions.h"
#include "class.h"
#include "oasis.h"
#include "crafting_new.h"
#include <sys/time.h>

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
static void load_evolutions(FILE *fl, struct char_data *ch);
static void load_known_evolutions(FILE *fl, struct char_data *ch);
static void load_class_feat_points(FILE *fl, struct char_data *ch);
static void load_epic_class_feat_points(FILE *fl, struct char_data *ch);
static void load_skill_focus(FILE *fl, struct char_data *ch);
static void load_abilities(FILE *fl, struct char_data *ch);
static void load_ability_exp(FILE *fl, struct char_data *ch);
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
static void load_failed_dialogue_quests(FILE *fl, struct char_data *ch);
static void load_HMVS(struct char_data *ch, const char *line, int mode);
static void write_aliases_ascii(FILE *file, struct char_data *ch);
static void read_aliases_ascii(FILE *file, struct char_data *ch, int count);
static void load_bombs(FILE *fl, struct char_data *ch);
static void load_craft_mats_onhand(FILE *fl, struct char_data *ch);
static void load_craft_motes_onhand(FILE *fl, struct char_data *ch);
static void load_judgements(FILE *fl, struct char_data *ch);
static void load_potions(FILE *fl, struct char_data *ch);
static void load_scrolls(FILE *fl, struct char_data *ch);
static void load_wands(FILE *fl, struct char_data *ch);
static void load_staves(FILE *fl, struct char_data *ch);
static void load_discoveries(FILE *fl, struct char_data *ch);
void load_temp_evolutions(FILE *fl, struct char_data *ch);
void save_char_pets(struct char_data *ch);
static void load_mercies(FILE *fl, struct char_data *ch);
static void load_cruelties(FILE *fl, struct char_data *ch);
static void load_buffs(FILE *fl, struct char_data *ch);
static void load_languages(FILE *fl, struct char_data *ch);
static void load_craft_affects(FILE *fl, struct char_data *ch);
static void load_craft_materials(FILE *fl, struct char_data *ch);
static void load_craft_motes(FILE *fl, struct char_data *ch);


// external functions
void autoroll_mob(struct char_data *mob, bool realmode, bool summoned);
void pet_save_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum);
void pet_load_objs(struct char_data *ch, struct char_data *owner, long int pet_idnum);

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

  /* Initialize all fields to prevent uninitialized value access */
  for (i = 0; i < rec_count; i++) {
    player_table[i].name = NULL;
    player_table[i].id = 0;
    player_table[i].level = 0;
    player_table[i].flags = 0;
    player_table[i].last = 0;
    player_table[i].clan = NO_CLAN;
  }

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
  
  /* Initialize all fields to prevent uninitialized value access */
  player_table[pos].id = 0;
  player_table[pos].level = 0;
  player_table[pos].last = 0;

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
    if (player_table[i].name && *player_table[i].name)
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
    {
      GET_ABILITY(ch, i) = 0;
      GET_CRAFT_SKILL_EXP(ch, i) = 0;
    }
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

    ch->char_specials.post_combat_exp = ch->char_specials.post_combat_gold = ch->char_specials.post_combat_account_exp = 0;

    BLASTING(ch) = FALSE;

    for (i = 0; i < NUM_CLASSES; i++)
    {
      GET_CLASS_FEATS(ch, i) = 0;
      GET_EPIC_CLASS_FEATS(ch, i) = 0;
    }
    GET_HOMETOWN(ch) = 0;
    GET_FEAT_POINTS(ch) = 0;
    GET_EPIC_FEAT_POINTS(ch) = 0;
    destroy_spell_prep_queue(ch);
    destroy_innate_magic_queue(ch);
    destroy_spell_collection(ch);
    destroy_known_spells(ch);
    GET_CH_AGE(ch) = 0;
    ch->player_specials->saved.character_age_saved = false;
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
    GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) = 0;
    GET_DRAGON_BOND_TYPE(ch) = 0;
    GET_DRAGON_RIDER_DRAGON_TYPE(ch) = 0;
    GET_FORAGE_COOLDOWN(ch) = 0;
    GET_RETAINER_COOLDOWN(ch) = 0;
    GET_SCROUNGE_COOLDOWN(ch) = 0;

    for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    { /* loop through all the character's quest slots */
      GET_QUEST_COUNTER(ch, i) = PFDEF_QUESTCOUNT;
      GET_QUEST(ch, i) = PFDEF_CURRQUEST;
    }

    GET_IMM_TITLE(ch) = NULL;
    ch->player.goals = NULL;
    ch->player.personality = NULL;
    ch->player.ideals = NULL;
    ch->player.bonds = NULL;
    ch->player.flaws = NULL;
    GET_HP_REGEN(ch) = 0;
    GET_MV_REGEN(ch) = 0;
    GET_PSP_REGEN(ch) = 0;
    GET_ENCUMBRANCE_MOD(ch) = 0;
    GET_FAST_HEALING_MOD(ch) = 0;
    GET_ELDRITCH_ESSENCE(ch) = -1;
    GET_ELDRITCH_SHAPE(ch) = -1;
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
    GET_CURRENT_MISSION_ROOM(ch) = 0;
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
    GET_BACKGROUND(ch) = 0;
    GET_PREMADE_BUILD_CLASS(ch) = PFDEF_PREMADE_BUILD;
    init_spell_prep_queue(ch);
    init_innate_magic_queue(ch);
    init_collection_queue(ch);
    init_known_spells(ch);
    reset_current_craft(ch, NULL, false, false);
    for (i = 0; i < NUM_CRAFT_MOTES; i++)
      GET_CRAFT_MOTES(ch, i) = 0;
    for (i = 0; i < NUM_CRAFT_MATS; i++)
      GET_CRAFT_MAT(ch, i) = 0;
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
    GET_KAPAK_SALIVA_HEALING_COOLDOWN(ch) = 0;
    for (i = 0; i < MAX_BUFFS; i++)
    {
      GET_BUFF(ch, i, 0) = 0;
      GET_BUFF(ch, i, 1) = 0;
    }
    GET_BUFF_TIMER(ch) = 0;
    GET_LAST_ROOM(ch) = 0;
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
    for (i = 0; i < NUM_EVOLUTIONS; i++)
    {
      HAS_REAL_EVOLUTION(ch, i) = 0;
      HAS_TEMP_EVOLUTION(ch, i) = 0;
      KNOWS_EVOLUTION(ch, i) = 0;
    }
    GET_EIDOLON_BASE_FORM(ch) = 0;
    CALL_EIDOLON_COOLDOWN(ch) = 0;
    MERGE_FORMS_TIMER(ch) = 0;
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

    NECROMANCER_CAST_TYPE(ch) = 0;

    GET_PC_DESCRIPTOR_1(ch) = 0;
    GET_PC_ADJECTIVE_1(ch) = 0;
    GET_PC_DESCRIPTOR_2(ch) = 0;
    GET_PC_ADJECTIVE_2(ch) = 0;

    GET_EIDOLON_LONG_DESCRIPTION(ch) = NULL;
    GET_EIDOLON_SHORT_DESCRIPTION(ch) = NULL;

    VITAL_STRIKING(ch) = FALSE;

    for (i = 0; i < MAX_BAGS; i++)
    {
      GET_BAG_NAME(ch, i) = NULL;
    }

    for (i = 0; i < 100; i++)
    {
      ch->player_specials->saved.failed_dialogue_quests[i] = 0;
    }

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
        if (!strcmp(tag, "AbXP"))
          load_ability_exp(fl, ch);
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
        else if (!strcmp(tag, "Age "))
          GET_CH_AGE(ch) = atoi(line);
        else if (!strcmp(tag, "AgeS"))
          (ch)->player_specials->saved.character_age_saved = atoi(line);
        else if (!strcmp(tag, "Alis"))
          read_aliases_ascii(fl, ch, atoi(line));
        break;

      case 'B':
        if (!strcmp(tag, "Badp"))
          GET_BAD_PWS(ch) = atoi(line);
        else if (!strcmp(tag, "BGnd"))
          GET_BACKGROUND(ch) = atoi(line);
        else if (!strcmp(tag, "Bond"))
          ch->player.bonds = fread_string(fl, buf2);
        else if (!strcmp(tag, "Bag1"))
          GET_BAG_NAME(ch, 1) = strdup(line);
        else if (!strcmp(tag, "Blst"))
          BLASTING(ch) = atoi(line);
        else if (!strcmp(tag, "Bag2"))
          GET_BAG_NAME(ch, 2) = strdup(line);
        else if (!strcmp(tag, "Bag3"))
          GET_BAG_NAME(ch, 3) = strdup(line);
        else if (!strcmp(tag, "Bag4"))
          GET_BAG_NAME(ch, 4) = strdup(line);
        else if (!strcmp(tag, "Bag5"))
          GET_BAG_NAME(ch, 5) = strdup(line);
        else if (!strcmp(tag, "Bag6"))
          GET_BAG_NAME(ch, 6) = strdup(line);
        else if (!strcmp(tag, "Bag7"))
          GET_BAG_NAME(ch, 7) = strdup(line);
        else if (!strcmp(tag, "Bag8"))
          GET_BAG_NAME(ch, 8) = strdup(line);
        else if (!strcmp(tag, "Bag9"))
          GET_BAG_NAME(ch, 9) = strdup(line);
        else if (!strcmp(tag, "Bag0"))
          GET_BAG_NAME(ch, 10) = strdup(line);
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
        else if (!strcmp(tag, "CfMt"))
          load_craft_mats_onhand(fl, ch);
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
        else if (!strcmp(tag, "CrAf"))
          load_craft_affects(fl, ch);
        else if (!strcmp(tag, "CrMo"))
          load_craft_motes(fl, ch);
        else if (!strcmp(tag, "CrMa"))
          load_craft_materials(fl, ch);
        else if (!strcmp(tag, "CrMe"))
          GET_CRAFT(ch).crafting_method = atoi(line);
        else if (!strcmp(tag, "CrIT"))
          GET_CRAFT(ch).crafting_item_type = atoi(line);
        else if (!strcmp(tag, "CrSp"))
          GET_CRAFT(ch).crafting_specific = atoi(line);
        else if (!strcmp(tag, "CrSk"))
          GET_CRAFT(ch).skill_type = atoi(line);
        else if (!strcmp(tag, "CrRe"))
          GET_CRAFT(ch).crafting_recipe = atoi(line);
        else if (!strcmp(tag, "CrVt"))
          GET_CRAFT(ch).craft_variant = atoi(line);
        else if (!strcmp(tag, "CrMe"))
          GET_CRAFT(ch).crafting_method = atoi(line);
        else if (!strcmp(tag, "CrEn"))
          GET_CRAFT(ch).enhancement = atoi(line);
        else if (!strcmp(tag, "CrEM"))
          GET_CRAFT(ch).enhancement_motes_required = atoi(line);
        else if (!strcmp(tag, "CrRl"))
          GET_CRAFT(ch).skill_roll = atoi(line);
        else if (!strcmp(tag, "CrDC"))
          GET_CRAFT(ch).dc = atoi(line);
        else if (!strcmp(tag, "CrDu"))
          GET_CRAFT(ch).craft_duration = atoi(line);
        else if (!strcmp(tag, "CrKy"))
        {
          if (GET_CRAFT(ch).keywords)
            free(GET_CRAFT(ch).keywords);
          GET_CRAFT(ch).keywords = strdup(line);
        }
        else if (!strcmp(tag, "CrSD"))
        {
          if (GET_CRAFT(ch).short_description)
            free(GET_CRAFT(ch).short_description);
          GET_CRAFT(ch).short_description = strdup(line);
        }
        else if (!strcmp(tag, "CrRD"))
        {
          if (GET_CRAFT(ch).room_description)
            free(GET_CRAFT(ch).room_description);
          GET_CRAFT(ch).room_description = strdup(line);
        }
        else if (!strcmp(tag, "CrEx"))
        {
          if (GET_CRAFT(ch).ex_description)
            free(GET_CRAFT(ch).ex_description);
          GET_CRAFT(ch).ex_description = strdup(line);
        }
        else if (!strcmp(tag, "CrOL"))
          GET_CRAFT(ch).obj_level = atoi(line);
        else if (!strcmp(tag, "CrLA"))
          GET_CRAFT(ch).level_adjust = atoi(line);
        else if (!strcmp(tag, "CrSN"))
          GET_CRAFT(ch).supply_num_required = atoi(line);
        else if (!strcmp(tag, "CrSR"))
          GET_CRAFT(ch).survey_rooms = atoi(line);
        else if (!strcmp(tag, "CrIy"))
          GET_CRAFT(ch).instrument_type = atoi(line);
        else if (!strcmp(tag, "CrIQ"))
          GET_CRAFT(ch).instrument_quality = atoi(line);
        else if (!strcmp(tag, "CrIE"))
          GET_CRAFT(ch).instrument_effectiveness = atoi(line);
        else if (!strcmp(tag, "CrIB"))
          GET_CRAFT(ch).instrument_breakability = atoi(line);
        else if (!strcmp(tag, "CrI1"))
          GET_CRAFT(ch).instrument_motes[1] = atoi(line);
        else if (!strcmp(tag, "CrI2"))
          GET_CRAFT(ch).instrument_motes[2] = atoi(line);
        else if (!strcmp(tag, "CrI3"))
          GET_CRAFT(ch).instrument_motes[3] = atoi(line);
        
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
        else if (!strcmp(tag, "DrBT"))
          GET_DRAGON_BOND_TYPE(ch) = atoi(line);
        else if (!strcmp(tag, "DrDT"))
          GET_DRAGON_RIDER_DRAGON_TYPE(ch) = atoi(line);
        break;

      case 'E':
        if (!strcmp(tag, "Exp "))
          GET_EXP(ch) = atoi(line);
        else if (!strcmp(tag, "Evnt"))
          load_events(fl, ch);
        else if (!strcmp(tag, "Evol"))
          load_evolutions(fl, ch);
        else if (!strcmp(tag, "Ecfp"))
          load_epic_class_feat_points(fl, ch);
        else if (!strcmp(tag, "Efpt"))
          GET_EPIC_FEAT_POINTS(ch) = atoi(line);
        else if (!strcmp(tag, "EidB"))
          GET_EIDOLON_BASE_FORM(ch) = atoi(line);
        else if (!strcmp(tag, "EidC"))
          CALL_EIDOLON_COOLDOWN(ch) = atoi(line);
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
        if (!strcmp(tag, "FBAB"))
          FIXED_BAB(ch) = atoi(line);
        else if (!strcmp(tag, "FaEn"))
          load_favored_enemy(fl, ch);
        else if (!strcmp(tag, "FaAd"))
          GET_FACTION_STANDING(ch, FACTION_ADVENTURERS) = atol(line);
        else if (!strcmp(tag, "Feat"))
          load_feats(fl, ch);
        else if (!strcmp(tag, "FrgC"))
          GET_FORAGE_COOLDOWN(ch) = atoi(line);
        else if (!strcmp(tag, "FLGT"))
          FLEETING_GLANCE_TIMER(ch) = atoi(line);
        else if (!strcmp(tag, "Flaw"))
          ch->player.flaws = fread_string(fl, buf2);
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
        else if (!strcmp(tag, "FttD"))
          GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) = atoi(line);
        else if (!strcmp(tag, "FDQs"))
          load_failed_dialogue_quests(fl, ch);
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
        else if (!strcmp(tag, "Goal"))
          ch->player.goals = fread_string(fl, buf2);
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
        else if (!strcmp(tag, "HomT"))
          GET_HOMETOWN(ch) = atoi(line);
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
        else if (!strcmp(tag, "Idel"))
          ch->player.ideals = fread_string(fl, buf2);
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
        else if (!strcmp(tag, "ITtl"))
          GET_IMM_TITLE(ch) = strdup(line);
        break;

      case 'J':
        if (!strcmp(tag, "Judg"))
          load_judgements(fl, ch);
        break;

      case 'K':
        if (!strcmp(tag, "KnSp"))
          load_known_spells(fl, ch);
        else if (!strcmp(tag, "KpkS"))
          GET_KAPAK_SALIVA_HEALING_COOLDOWN(ch) = atoi(line);
        else if (!strcmp(tag, "KEvo"))
          load_known_evolutions(fl, ch);
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
        else if (!strcmp(tag, "LstR"))
          GET_LAST_ROOM(ch) = atoi(line);
        break;

      case 'M':
        if (!strcmp(tag, "Move"))
          load_HMVS(ch, line, LOAD_MOVE);
        else if (!strcmp(tag, "Mote"))
          load_craft_motes_onhand(fl, ch);
        else if (!strcmp(tag, "Mrph"))
          IS_MORPHED(ch) = atol(line);
        else if (!strcmp(tag, "MFrm"))
          MERGE_FORMS_TIMER(ch) = atoi(line);
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
        else if (!strcmp(tag, "MiRm"))
          GET_CURRENT_MISSION_ROOM(ch) = atoi(line);
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
        else if (!strcmp(tag, "NecC"))
          NECROMANCER_CAST_TYPE(ch) = atoi(line);
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
        else if (!strcmp(tag, "Pers"))
          ch->player.personality = fread_string(fl, buf2);
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
        else if (!strcmp(tag, "RetC"))
          GET_RETAINER_COOLDOWN(ch) = atoi(line);
        else if (!strcmp(tag, "RM00"))
          GET_CRAFT(ch).refining_materials[0][0] = atoi(line);
        else if (!strcmp(tag, "RM01"))
          GET_CRAFT(ch).refining_materials[0][1] = atoi(line);
        else if (!strcmp(tag, "RM10"))
          GET_CRAFT(ch).refining_materials[1][0] = atoi(line);
        else if (!strcmp(tag, "RM11"))
          GET_CRAFT(ch).refining_materials[1][1] = atoi(line);
        else if (!strcmp(tag, "RM20"))
          GET_CRAFT(ch).refining_materials[2][0] = atoi(line);
        else if (!strcmp(tag, "RM21"))
          GET_CRAFT(ch).refining_materials[2][1] = atoi(line);
        else if (!strcmp(tag, "RRs0"))
          GET_CRAFT(ch).refining_result[0] = atoi(line);
        else if (!strcmp(tag, "RRs1"))
          GET_CRAFT(ch).refining_result[1] = atoi(line);
        else if (!strcmp(tag, "RSSz"))
          GET_CRAFT(ch).new_size = atoi(line);
        else if (!strcmp(tag, "RSMT"))
          GET_CRAFT(ch).resize_mat_type = atoi(line);
        else if (!strcmp(tag, "RSMN"))
          GET_CRAFT(ch).resize_mat_num = atoi(line);
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
        else if (!strcmp(tag, "Scrg"))
          GET_SCROUNGE_COOLDOWN(ch) = atoi(line);
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
        else if (!strcmp(tag, "SpNM"))
          GET_NSUPPLY_NUM_MADE(ch) = atoi(line);
        else if (!strcmp(tag, "SpCd"))
          GET_NSUPPLY_COOLDOWN(ch) = atoi(line);
        break;

      case 'T':
        if (!strcmp(tag, "Tmpl"))
          GET_TEMPLATE(ch) = atoi(line);
        else if (!strcmp(tag, "TEvo"))
          load_temp_evolutions(fl, ch);
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
        else if (!strcmp(tag, "VitS"))
          VITAL_STRIKING(ch) = atoi(line);
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

  if (GET_CRAFT(ch).new_size)
  {
    GET_CRAFT_MAT(ch, GET_CRAFT(ch).resize_mat_type) += GET_CRAFT(ch).resize_mat_num;
    GET_CRAFT(ch).new_size = GET_CRAFT(ch).resize_mat_type = GET_CRAFT(ch).resize_mat_num = GET_CRAFT(ch).crafting_method = GET_CRAFT(ch).craft_duration = 0;
  }

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

/* Helper function for save_char to optimize string operations */
static inline void buffer_write_string_field(char *buffer, size_t *buffer_used, size_t buffer_size, 
                                            const char *field_name, const char *field_value, 
                                            char *temp_buf, size_t temp_buf_size) {
  if (field_value && *field_value) {
    char stripped[MAX_STRING_LENGTH];
    strlcpy(stripped, field_value, sizeof(stripped));
    strip_cr(stripped);
    
    int len = snprintf(temp_buf, temp_buf_size, "%s:\n%s~\n", field_name, stripped);
    if (len > 0 && *buffer_used + len < buffer_size) {
      memcpy(buffer + *buffer_used, temp_buf, len);
      *buffer_used += len;
    }
  }
}

/* This is the ASCII Player Files save routine. */
void save_char(struct char_data *ch, int mode)
{
  FILE *fl;
  char filename[40] = {'\0'},
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
  
  /* PERFORMANCE OPTIMIZATION: Buffered I/O system */
  char *write_buffer = NULL;
  size_t buffer_size = 65536; /* 64KB initial buffer */
  size_t buffer_used = 0;
  char temp_buf[2048]; /* Temporary buffer for sprintf operations */
  
  /* Performance timing */
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  if (IS_NPC(ch) || GET_PFILEPOS(ch) < 0)
    return;
  
  /* Allocate write buffer for performance */
  CREATE(write_buffer, char, buffer_size);
  if (!write_buffer) {
    log("SYSERR: save_char: Could not allocate write buffer for %s", GET_NAME(ch));
    return;
  }
  
  /* Helper macro for buffered writes */
  #define BUFFER_WRITE(...) do { \
    int len = snprintf(temp_buf, sizeof(temp_buf), __VA_ARGS__); \
    if (len > 0) { \
      if (buffer_used + len >= buffer_size) { \
        /* Expand buffer if needed */ \
        size_t new_size = buffer_size * 2; \
        char *new_buffer = realloc(write_buffer, new_size); \
        if (!new_buffer) { \
          log("SYSERR: save_char: Buffer realloc failed"); \
          free(write_buffer); \
          return; \
        } \
        write_buffer = new_buffer; \
        buffer_size = new_size; \
      } \
      memcpy(write_buffer + buffer_used, temp_buf, len); \
      buffer_used += len; \
    } \
  } while(0)

  /* If ch->desc is not null, then update session data before saving. */
  if (ch->desc)
  {
    if (*ch->desc->host)
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
  if (!get_filename(filename, sizeof(filename), PLR_FILE, GET_NAME(ch))) {
    free(write_buffer);
    return;
  }
  if (!(fl = fopen(filename, "w")))
  {
    mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Couldn't open player file %s for write", filename);
    free(write_buffer);
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
    BUFFER_WRITE("Name: %s\n", GET_NAME(ch));
  if (GET_PASSWD(ch))
    BUFFER_WRITE("Pass: %s\n", GET_PASSWD(ch));
  if (ch->desc && ch->desc->account && ch->desc->account->name)
  {
    BUFFER_WRITE("Acct: %s\n", ch->desc->account->name);
    //    BUFFER_WRITE( "ActN: %s\n", ch->desc->account->name);
  }

  if (GET_TITLE(ch))
    BUFFER_WRITE("Titl: %s\n", GET_TITLE(ch));
  if (GET_IMM_TITLE(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    BUFFER_WRITE("ITtl: %s\n", GET_IMM_TITLE(ch));

  /*save todo lists*/
  struct txt_block *tmp;
  if ((tmp = GET_TODO(ch)))
  {
    BUFFER_WRITE("Todo:\n");
    while (tmp)
    {
      if (tmp->text)
        BUFFER_WRITE("%s\n", tmp->text);
      tmp = tmp->next;
    }
    BUFFER_WRITE("~\n");
  }

  /* Optimize string field writes */
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Desc", ch->player.description, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "BGrd", ch->player.background, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Goal", ch->player.goals, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Pers", ch->player.personality, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Idel", ch->player.ideals, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Bond", ch->player.bonds, temp_buf, sizeof(temp_buf));
  buffer_write_string_field(write_buffer, &buffer_used, buffer_size, "Flaw", ch->player.flaws, temp_buf, sizeof(temp_buf));
  if (BLASTING(ch))
    BUFFER_WRITE( "Blst: 1\n");
  if (POOFIN(ch))
    BUFFER_WRITE( "PfIn: %s\n", POOFIN(ch));
  if (POOFOUT(ch))
    BUFFER_WRITE( "PfOt: %s\n", POOFOUT(ch));
  if (GET_SEX(ch) != PFDEF_SEX)
    BUFFER_WRITE( "Sex : %d\n", GET_SEX(ch));
  if (GET_BLOODLINE_SUBTYPE(ch) != PFDEF_SORC_BLOODLINE_SUBTYPE)
    BUFFER_WRITE( "SBld: %d\n", GET_BLOODLINE_SUBTYPE(ch));
  if (GET_CLASS(ch) != PFDEF_CLASS)
    BUFFER_WRITE( "Clas: %d\n", GET_CLASS(ch));
  if (GET_REAL_RACE(ch) != PFDEF_RACE)
    BUFFER_WRITE( "Race: %d\n", GET_REAL_RACE(ch));
  if (GET_REAL_SIZE(ch) != PFDEF_SIZE)
    BUFFER_WRITE( "Size: %d\n", GET_REAL_SIZE(ch));
  if (HAS_SET_STATS_STUDY(ch) != PFDEF_HAS_SET_STATS_STUDY)
    BUFFER_WRITE( "SySt: %d\n", HAS_SET_STATS_STUDY(ch));
  if (GET_LEVEL(ch) != PFDEF_LEVEL)
    BUFFER_WRITE( "Levl: %d\n", GET_LEVEL(ch));
  if (GET_DISGUISE_RACE(ch))
    BUFFER_WRITE( "DRac: %d\n", GET_DISGUISE_RACE(ch));
  if (GET_DISGUISE_STR(ch))
    BUFFER_WRITE( "DStr: %d\n", GET_DISGUISE_STR(ch));
  if (GET_DISGUISE_DEX(ch))
    BUFFER_WRITE( "DDex: %d\n", GET_DISGUISE_DEX(ch));
  if (GET_DISGUISE_CON(ch))
    BUFFER_WRITE( "DCon: %d\n", GET_DISGUISE_CON(ch));
  if (GET_DISGUISE_AC(ch))
    BUFFER_WRITE( "DAC: %d\n", GET_DISGUISE_AC(ch));
  if (NEW_ARCANA_SLOT(ch, 0))
    BUFFER_WRITE( "NAr0: %d\n", NEW_ARCANA_SLOT(ch, 0));
  if (NEW_ARCANA_SLOT(ch, 1))
    BUFFER_WRITE( "NAr1: %d\n", NEW_ARCANA_SLOT(ch, 1));
  if (NEW_ARCANA_SLOT(ch, 2))
    BUFFER_WRITE( "NAr2: %d\n", NEW_ARCANA_SLOT(ch, 2));
  if (NEW_ARCANA_SLOT(ch, 3))
    BUFFER_WRITE( "NAr3: %d\n", NEW_ARCANA_SLOT(ch, 3));
  if (NECROMANCER_CAST_TYPE(ch))
    BUFFER_WRITE( "NecC: %d\n", NECROMANCER_CAST_TYPE(ch));
  BUFFER_WRITE( "Id  : %ld\n", GET_IDNUM(ch));
  BUFFER_WRITE( "Brth: %ld\n", (long)ch->player.time.birth);
  BUFFER_WRITE( "Plyd: %d\n", ch->player.time.played);
  BUFFER_WRITE( "Last: %ld\n", (long)ch->player.time.logon);
  BUFFER_WRITE( "LstR: %d\n", GET_LAST_ROOM(ch));

  if (GET_LAST_MOTD(ch) != PFDEF_LASTMOTD)
    BUFFER_WRITE( "Lmot: %d\n", (int)GET_LAST_MOTD(ch));
  if (GET_LAST_NEWS(ch) != PFDEF_LASTNEWS)
    BUFFER_WRITE( "Lnew: %d\n", (int)GET_LAST_NEWS(ch));

  BUFFER_WRITE( "DrgB: %d\n", GET_DRAGONBORN_ANCESTRY(ch));

  BUFFER_WRITE( "Spek: %d\n", SPEAKING(ch));
  BUFFER_WRITE( "Home: %d\n", GET_REGION(ch));
  BUFFER_WRITE( "HomT: %d\n", GET_HOMETOWN(ch));
  BUFFER_WRITE( "DAd1: %d\n", GET_PC_ADJECTIVE_1(ch));
  BUFFER_WRITE( "DAd2: %d\n", GET_PC_ADJECTIVE_2(ch));
  BUFFER_WRITE( "DDs1: %d\n", GET_PC_DESCRIPTOR_1(ch));
  BUFFER_WRITE( "DDs2: %d\n", GET_PC_DESCRIPTOR_2(ch));

  if (ch->player_specials->saved.new_race_stats)
    BUFFER_WRITE( "RacR: %d\n", ch->player_specials->saved.new_race_stats);

  if (GET_HOST(ch))
    BUFFER_WRITE( "Host: %s\n", GET_HOST(ch));
  if (GET_HEIGHT(ch) != PFDEF_HEIGHT)
    BUFFER_WRITE( "Hite: %d\n", GET_HEIGHT(ch));
  if (HIGH_ELF_CANTRIP(ch))
    BUFFER_WRITE( "HECn: %d\n", HIGH_ELF_CANTRIP(ch));
  if (GET_HOLY_WEAPON_TYPE(ch) != PFDEF_HOLY_WEAPON_TYPE)
    BUFFER_WRITE( "HlyW: %d\n", GET_HOLY_WEAPON_TYPE(ch));
  if (GET_WEIGHT(ch) != PFDEF_WEIGHT)
    BUFFER_WRITE( "Wate: %d\n", GET_WEIGHT(ch));
  if (GET_ALIGNMENT(ch) != PFDEF_ALIGNMENT)
    BUFFER_WRITE( "Alin: %d\n", GET_ALIGNMENT(ch));
  if (GET_CH_AGE(ch) != 0)
    BUFFER_WRITE( "Age : %d\n", GET_CH_AGE(ch));
  if ((ch)->player_specials->saved.character_age_saved != 0)
    BUFFER_WRITE( "AgeS: %d\n", (ch)->player_specials->saved.character_age_saved);
  if (GET_TEMPLATE(ch) != PFDEF_TEMPLATE)
    BUFFER_WRITE( "Tmpl: %d\n", GET_TEMPLATE(ch));
  // Faction mission system
  BUFFER_WRITE( "MiCu: %d\n", GET_CURRENT_MISSION(ch));
  BUFFER_WRITE( "MiCr: %ld\n", GET_MISSION_CREDITS(ch));
  BUFFER_WRITE( "MiCd: %d\n", GET_MISSION_COOLDOWN(ch));
  BUFFER_WRITE( "MiSt: %ld\n", GET_MISSION_STANDING(ch));
  BUFFER_WRITE( "MiFa: %d\n", GET_MISSION_FACTION(ch));
  BUFFER_WRITE( "MiRe: %ld\n", GET_MISSION_REP(ch));
  BUFFER_WRITE( "MiXp: %ld\n", GET_MISSION_EXP(ch));
  BUFFER_WRITE( "MiDf: %d\n", GET_MISSION_DIFFICULTY(ch));
  BUFFER_WRITE( "MiRN: %d\n", GET_MISSION_NPC_NAME_NUM(ch));
  BUFFER_WRITE( "MiRm: %d\n", GET_CURRENT_MISSION_ROOM(ch));

  if (VITAL_STRIKING(ch))
    BUFFER_WRITE( "VitS: %d\n", VITAL_STRIKING(ch));

  sprintascii(bits, PLR_FLAGS(ch)[0]);
  sprintascii(bits2, PLR_FLAGS(ch)[1]);
  sprintascii(bits3, PLR_FLAGS(ch)[2]);
  sprintascii(bits4, PLR_FLAGS(ch)[3]);
  BUFFER_WRITE( "Act : %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, AFF_FLAGS(ch)[0]);
  sprintascii(bits2, AFF_FLAGS(ch)[1]);
  sprintascii(bits3, AFF_FLAGS(ch)[2]);
  sprintascii(bits4, AFF_FLAGS(ch)[3]);
  BUFFER_WRITE( "Aff : %s %s %s %s\n", bits, bits2, bits3, bits4);

  sprintascii(bits, PRF_FLAGS(ch)[0]);
  sprintascii(bits2, PRF_FLAGS(ch)[1]);
  sprintascii(bits3, PRF_FLAGS(ch)[2]);
  sprintascii(bits4, PRF_FLAGS(ch)[3]);
  BUFFER_WRITE( "Pref: %s %s %s %s\n", bits, bits2, bits3, bits4);

  if (GET_SAVE(ch, 0) != PFDEF_SAVETHROW)
    BUFFER_WRITE( "Thr1: %d\n", GET_SAVE(ch, 0));
  if (GET_SAVE(ch, 1) != PFDEF_SAVETHROW)
    BUFFER_WRITE( "Thr2: %d\n", GET_SAVE(ch, 1));
  if (GET_SAVE(ch, 2) != PFDEF_SAVETHROW)
    BUFFER_WRITE( "Thr3: %d\n", GET_SAVE(ch, 2));
  if (GET_SAVE(ch, 3) != PFDEF_SAVETHROW)
    BUFFER_WRITE( "Thr4: %d\n", GET_SAVE(ch, 3));
  if (GET_SAVE(ch, 4) != PFDEF_SAVETHROW)
    BUFFER_WRITE( "Thr5: %d\n", GET_SAVE(ch, 4));

  if (GET_RESISTANCES(ch, 1) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res1: %d\n", GET_RESISTANCES(ch, 1));
  if (GET_RESISTANCES(ch, 2) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res2: %d\n", GET_RESISTANCES(ch, 2));
  if (GET_RESISTANCES(ch, 3) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res3: %d\n", GET_RESISTANCES(ch, 3));
  if (GET_RESISTANCES(ch, 4) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res4: %d\n", GET_RESISTANCES(ch, 4));
  if (GET_RESISTANCES(ch, 5) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res5: %d\n", GET_RESISTANCES(ch, 5));
  if (GET_RESISTANCES(ch, 6) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res6: %d\n", GET_RESISTANCES(ch, 6));
  if (GET_RESISTANCES(ch, 7) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res7: %d\n", GET_RESISTANCES(ch, 7));
  if (GET_RESISTANCES(ch, 8) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res8: %d\n", GET_RESISTANCES(ch, 8));
  if (GET_RESISTANCES(ch, 9) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "Res9: %d\n", GET_RESISTANCES(ch, 9));
  if (GET_RESISTANCES(ch, 10) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResA: %d\n", GET_RESISTANCES(ch, 10));
  if (GET_RESISTANCES(ch, 11) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResB: %d\n", GET_RESISTANCES(ch, 11));
  if (GET_RESISTANCES(ch, 12) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResC: %d\n", GET_RESISTANCES(ch, 12));
  if (GET_RESISTANCES(ch, 13) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResD: %d\n", GET_RESISTANCES(ch, 13));
  if (GET_RESISTANCES(ch, 14) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResE: %d\n", GET_RESISTANCES(ch, 14));
  if (GET_RESISTANCES(ch, 15) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResF: %d\n", GET_RESISTANCES(ch, 15));
  if (GET_RESISTANCES(ch, 16) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResG: %d\n", GET_RESISTANCES(ch, 16));
  if (GET_RESISTANCES(ch, 17) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResH: %d\n", GET_RESISTANCES(ch, 17));
  if (GET_RESISTANCES(ch, 18) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResI: %d\n", GET_RESISTANCES(ch, 18));
  if (GET_RESISTANCES(ch, 19) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResJ: %d\n", GET_RESISTANCES(ch, 19));
  if (GET_RESISTANCES(ch, 20) != PFDEF_RESISTANCES)
    BUFFER_WRITE( "ResK: %d\n", GET_RESISTANCES(ch, 20));

  if (GET_WIMP_LEV(ch) != PFDEF_WIMPLEV)
    BUFFER_WRITE( "Wimp: %d\n", GET_WIMP_LEV(ch));
  if (GET_FREEZE_LEV(ch) != PFDEF_FREEZELEV)
    BUFFER_WRITE( "Frez: %d\n", GET_FREEZE_LEV(ch));
  if (GET_INVIS_LEV(ch) != PFDEF_INVISLEV)
    BUFFER_WRITE( "Invs: %d\n", GET_INVIS_LEV(ch));
  if (GET_LOADROOM(ch) != PFDEF_LOADROOM)
    BUFFER_WRITE( "Room: %d\n", GET_LOADROOM(ch));
  if (ch->player_specials->saved.fiendish_boons != 0)
    BUFFER_WRITE( "FdBn: %d\n", ch->player_specials->saved.fiendish_boons);
  if (ch->player_specials->saved.channel_energy_type != 0)
    BUFFER_WRITE( "ChEn: %d\n", ch->player_specials->saved.channel_energy_type);

  if (FIXED_BAB(ch) != 0)
    BUFFER_WRITE( "FBAB: %d\n", FIXED_BAB(ch));

  BUFFER_WRITE( "FaAd: %ld\n", GET_FACTION_STANDING(ch, FACTION_ADVENTURERS));

  if (GET_BAD_PWS(ch) != PFDEF_BADPWS)
    BUFFER_WRITE( "Badp: %d\n", GET_BAD_PWS(ch));
  if (GET_BACKGROUND(ch) != 0)
    BUFFER_WRITE( "BGnd: %d\n", GET_BACKGROUND(ch));
  if (GET_PRACTICES(ch) != PFDEF_PRACTICES)
    BUFFER_WRITE( "Lern: %d\n", GET_PRACTICES(ch));
  if (GET_TRAINS(ch) != PFDEF_TRAINS)
    BUFFER_WRITE( "Trns: %d\n", GET_TRAINS(ch));
  if (GET_BOOSTS(ch) != PFDEF_BOOSTS)
    BUFFER_WRITE( "Bost: %d\n", GET_BOOSTS(ch));

  if (GET_1ST_DOMAIN(ch) != PFDEF_DOMAIN_1)
    BUFFER_WRITE( "Dom1: %d\n", GET_1ST_DOMAIN(ch));
  if (GET_2ND_DOMAIN(ch) != PFDEF_DOMAIN_2)
    BUFFER_WRITE( "Dom2: %d\n", GET_2ND_DOMAIN(ch));
  if (GET_SPECIALTY_SCHOOL(ch) != PFDEF_SPECIALTY_SCHOOL)
    BUFFER_WRITE( "SSch: %d\n", GET_SPECIALTY_SCHOOL(ch));
  if (GET_1ST_RESTRICTED_SCHOOL(ch) != PFDEF_RESTRICTED_SCHOOL_1)
    BUFFER_WRITE( "RSc1: %d\n", GET_1ST_RESTRICTED_SCHOOL(ch));
  if (GET_2ND_RESTRICTED_SCHOOL(ch) != PFDEF_RESTRICTED_SCHOOL_2)
    BUFFER_WRITE( "RSc2: %d\n", GET_2ND_RESTRICTED_SCHOOL(ch));

  if (GET_PREFERRED_ARCANE(ch) != PFDEF_PREFERRED_ARCANE)
    BUFFER_WRITE( "PCAr: %d\n", GET_PREFERRED_ARCANE(ch));
  if (GET_PREFERRED_DIVINE(ch) != PFDEF_PREFERRED_DIVINE)
    BUFFER_WRITE( "PCDi: %d\n", GET_PREFERRED_DIVINE(ch));

  if (GET_FEAT_POINTS(ch) != 0)
    BUFFER_WRITE( "Ftpt: %d\n", GET_FEAT_POINTS(ch));

  BUFFER_WRITE( "Cfpt:\n");
  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_CLASS_FEATS(ch, i) != 0)
      BUFFER_WRITE( "%d %d\n", i, GET_CLASS_FEATS(ch, i));
  BUFFER_WRITE( "0\n");

  if (GET_EPIC_FEAT_POINTS(ch) != 0)
    BUFFER_WRITE( "Efpt: %d\n", GET_EPIC_FEAT_POINTS(ch));

  BUFFER_WRITE( "Ecfp:\n");
  for (i = 0; i < NUM_CLASSES; i++)
    if (GET_EPIC_CLASS_FEATS(ch, i) != 0)
      BUFFER_WRITE( "%d %d\n", i, GET_EPIC_CLASS_FEATS(ch, i));
  BUFFER_WRITE( "0\n");

  if (GET_COND(ch, HUNGER) != PFDEF_HUNGER && GET_LEVEL(ch) < LVL_IMMORT)
    BUFFER_WRITE( "Hung: %d\n", GET_COND(ch, HUNGER));
  if (GET_COND(ch, THIRST) != PFDEF_THIRST && GET_LEVEL(ch) < LVL_IMMORT)
    BUFFER_WRITE( "Thir: %d\n", GET_COND(ch, THIRST));
  if (GET_COND(ch, DRUNK) != PFDEF_DRUNK && GET_LEVEL(ch) < LVL_IMMORT)
    BUFFER_WRITE( "Drnk: %d\n", GET_COND(ch, DRUNK));

  if (GET_HIT(ch) != PFDEF_HIT || GET_MAX_HIT(ch) != PFDEF_MAXHIT)
    BUFFER_WRITE( "Hit : %d/%d\n", GET_HIT(ch), GET_MAX_HIT(ch));
  if (GET_PSP(ch) != PFDEF_PSP || GET_MAX_PSP(ch) != PFDEF_MAXPSP)
    BUFFER_WRITE( "PSP : %d/%d\n", GET_PSP(ch), GET_MAX_PSP(ch));
  if (GET_MOVE(ch) != PFDEF_MOVE || GET_MAX_MOVE(ch) != PFDEF_MAXMOVE)
    BUFFER_WRITE( "Move: %d/%d\n", GET_MOVE(ch), GET_MAX_MOVE(ch));

  if (GET_HP_REGEN(ch) != PFDEF_HP_REGEN)
    BUFFER_WRITE( "HPRg : %d\n", GET_HP_REGEN(ch));
  if (GET_MV_REGEN(ch) != PFDEF_MV_REGEN)
    BUFFER_WRITE( "MVRg : %d\n", GET_MV_REGEN(ch));
  if (GET_PSP_REGEN(ch) != PFDEF_PSP_REGEN)
    BUFFER_WRITE( "PSRg : %d\n", GET_PSP_REGEN(ch));

  if (GET_SETCLOAK_TIMER(ch) != PFDEF_SETCLOAK_TIMER)
    BUFFER_WRITE( "ClkT : %d\n", GET_SETCLOAK_TIMER(ch));

  if (GET_STR(ch) != PFDEF_STR || GET_ADD(ch) != PFDEF_STRADD)
    BUFFER_WRITE( "Str : %d/%d\n", GET_STR(ch), GET_ADD(ch));

  if (GET_INT(ch) != PFDEF_INT)
    BUFFER_WRITE( "Int : %d\n", GET_INT(ch));
  if (GET_WIS(ch) != PFDEF_WIS)
    BUFFER_WRITE( "Wis : %d\n", GET_WIS(ch));
  if (GET_DEX(ch) != PFDEF_DEX)
    BUFFER_WRITE( "Dex : %d\n", GET_DEX(ch));
  if (GET_CON(ch) != PFDEF_CON)
    BUFFER_WRITE( "Con : %d\n", GET_CON(ch));
  if (GET_CHA(ch) != PFDEF_CHA)
    BUFFER_WRITE( "Cha : %d\n", GET_CHA(ch));

  if (GET_AC(ch) != PFDEF_AC)
    BUFFER_WRITE( "Ac  : %d\n", GET_AC(ch));
  if (GET_GOLD(ch) != PFDEF_GOLD)
    BUFFER_WRITE( "Gold: %d\n", GET_GOLD(ch));
  if (GET_BANK_GOLD(ch) != PFDEF_BANK)
    BUFFER_WRITE( "Bank: %d\n", GET_BANK_GOLD(ch));
  if (GET_EXP(ch) != PFDEF_EXP)
    BUFFER_WRITE( "Exp : %d\n", GET_EXP(ch));
  if (GET_HITROLL(ch) != PFDEF_HITROLL)
    BUFFER_WRITE( "Hrol: %d\n", GET_HITROLL(ch));
  if (GET_DAMROLL(ch) != PFDEF_DAMROLL)
    BUFFER_WRITE( "Drol: %d\n", GET_DAMROLL(ch));
  if (GET_SPELL_RES(ch) != PFDEF_SPELL_RES)
    BUFFER_WRITE( "SpRs: %d\n", GET_SPELL_RES(ch));
  if (IS_MORPHED(ch) != PFDEF_MORPHED)
    BUFFER_WRITE( "Mrph: %d\n", IS_MORPHED(ch));
  if (MERGE_FORMS_TIMER(ch) != 0)
    BUFFER_WRITE( "MFrm: %d\n", MERGE_FORMS_TIMER(ch));
  if (GET_EIDOLON_BASE_FORM(ch) != 0)
    BUFFER_WRITE( "EidB: %d\n", GET_EIDOLON_BASE_FORM(ch));
  if (CALL_EIDOLON_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "EidC: %d\n", CALL_EIDOLON_COOLDOWN(ch));
  if (GET_FORAGE_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "FrgC: %d\n", GET_FORAGE_COOLDOWN(ch));
  if (GET_SCROUNGE_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "Scrg: %d\n", GET_SCROUNGE_COOLDOWN(ch));
  if (GET_RETAINER_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "RetC: %d\n", GET_RETAINER_COOLDOWN(ch));
  BUFFER_WRITE( "God : %d\n", GET_DEITY(ch));
  if (GET_AUTOCQUEST_VNUM(ch) != PFDEF_AUTOCQUEST_VNUM)
    BUFFER_WRITE( "Cvnm: %d\n", GET_AUTOCQUEST_VNUM(ch));
  if (GET_AUTOCQUEST_MAKENUM(ch) != PFDEF_AUTOCQUEST_MAKENUM)
    BUFFER_WRITE( "Cmnm: %d\n", GET_AUTOCQUEST_MAKENUM(ch));
  if (GET_AUTOCQUEST_QP(ch) != PFDEF_AUTOCQUEST_QP)
    BUFFER_WRITE( "Cqps: %d\n", GET_AUTOCQUEST_QP(ch));
  if (GET_AUTOCQUEST_EXP(ch) != PFDEF_AUTOCQUEST_EXP)
    BUFFER_WRITE( "Cexp: %d\n", GET_AUTOCQUEST_EXP(ch));
  if (GET_AUTOCQUEST_GOLD(ch) != PFDEF_AUTOCQUEST_GOLD)
    BUFFER_WRITE( "Cgld: %d\n", GET_AUTOCQUEST_GOLD(ch));
  if (GET_AUTOCQUEST_DESC(ch) != PFDEF_AUTOCQUEST_DESC)
    BUFFER_WRITE( "Cdsc: %s\n", GET_AUTOCQUEST_DESC(ch));
  if (GET_AUTOCQUEST_MATERIAL(ch) != PFDEF_AUTOCQUEST_MATERIAL)
    BUFFER_WRITE( "Cmat: %d\n", GET_AUTOCQUEST_MATERIAL(ch));

  if (EFREETI_MAGIC_USES(ch) != PFDEF_EFREETI_MAGIC_USES)
    BUFFER_WRITE( "EfMU: %d\n", EFREETI_MAGIC_USES(ch));
  if (EFREETI_MAGIC_TIMER(ch) != PFDEF_EFREETI_MAGIC_TIMER)
    BUFFER_WRITE( "EfMT: %d\n", EFREETI_MAGIC_TIMER(ch));
  if (GET_ENCUMBRANCE_MOD(ch) != 0)
    BUFFER_WRITE( "EncM: %d\n", GET_ENCUMBRANCE_MOD(ch));
  BUFFER_WRITE( "EldE: %d\n", GET_ELDRITCH_ESSENCE(ch));
  BUFFER_WRITE( "EldS: %d\n", GET_ELDRITCH_SHAPE(ch));
  if (GET_DR_MOD(ch) > 0)
    BUFFER_WRITE( "DRMd: %d\n", GET_DR_MOD(ch));

  if (DRAGON_MAGIC_USES(ch) != PFDEF_DRAGON_MAGIC_USES)
    BUFFER_WRITE( "DrMU: %d\n", DRAGON_MAGIC_USES(ch));
  if (DRAGON_MAGIC_TIMER(ch) != PFDEF_DRAGON_MAGIC_TIMER)
    BUFFER_WRITE( "DrMT: %d\n", DRAGON_MAGIC_TIMER(ch));
  if (PIXIE_DUST_USES(ch) != PFDEF_PIXIE_DUST_USES)
    BUFFER_WRITE( "PxDU: %d\n", PIXIE_DUST_USES(ch));
  if (PIXIE_DUST_TIMER(ch) != PFDEF_PIXIE_DUST_TIMER)
    BUFFER_WRITE( "PxDT: %d\n", PIXIE_DUST_TIMER(ch));

  if (LAUGHING_TOUCH_USES(ch) != PFDEF_LAUGHING_TOUCH_USES)
    BUFFER_WRITE( "LTCU: %d\n", LAUGHING_TOUCH_USES(ch));
  if (LAUGHING_TOUCH_TIMER(ch) != PFDEF_LAUGHING_TOUCH_TIMER)
    BUFFER_WRITE( "LTCT: %d\n", LAUGHING_TOUCH_TIMER(ch));

  if (FLEETING_GLANCE_USES(ch) != PFDEF_FLEETING_GLANCE_USES)
    BUFFER_WRITE( "FLGU: %d\n", FLEETING_GLANCE_USES(ch));
  if (FLEETING_GLANCE_TIMER(ch) != PFDEF_FLEETING_GLANCE_TIMER)
    BUFFER_WRITE( "FLGT: %d\n", FLEETING_GLANCE_TIMER(ch));

  if (FEY_SHADOW_WALK_USES(ch) != PFDEF_FEY_SHADOW_WALK_USES)
    BUFFER_WRITE( "FSWU: %d\n", FEY_SHADOW_WALK_USES(ch));
  if (FEY_SHADOW_WALK_TIMER(ch) != PFDEF_FEY_SHADOW_WALK_TIMER)
    BUFFER_WRITE( "FSWT: %d\n", FEY_SHADOW_WALK_TIMER(ch));

  if (GET_FAST_HEALING_MOD(ch) != 0)
    BUFFER_WRITE( "FstH: %d\n", GET_FAST_HEALING_MOD(ch));
  if (GRAVE_TOUCH_USES(ch) != PFDEF_GRAVE_TOUCH_USES)
    BUFFER_WRITE( "GTCU: %d\n", GRAVE_TOUCH_USES(ch));
  if (GRAVE_TOUCH_TIMER(ch) != PFDEF_GRAVE_TOUCH_TIMER)
    BUFFER_WRITE( "GTCT: %d\n", GRAVE_TOUCH_TIMER(ch));
  if (GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "FttD: %d\n", GET_FIGHT_TO_THE_DEATH_COOLDOWN(ch));
  if (GET_DRAGON_BOND_TYPE(ch) != 0)
    BUFFER_WRITE( "DrBT: %d\n", GET_DRAGON_BOND_TYPE(ch));
  if (GET_DRAGON_RIDER_DRAGON_TYPE(ch) != 0)
    BUFFER_WRITE( "DrDT: %d\n", GET_DRAGON_RIDER_DRAGON_TYPE(ch));

  if (GRASP_OF_THE_DEAD_USES(ch) != PFDEF_GRASP_OF_THE_DEAD_USES)
    BUFFER_WRITE( "GODU: %d\n", GRASP_OF_THE_DEAD_USES(ch));
  if (GRASP_OF_THE_DEAD_TIMER(ch) != PFDEF_GRASP_OF_THE_DEAD_TIMER)
    BUFFER_WRITE( "GODT: %d\n", GRASP_OF_THE_DEAD_TIMER(ch));

  if (INCORPOREAL_FORM_USES(ch) != PFDEF_INCORPOREAL_FORM_USES)
    BUFFER_WRITE( "InFU: %d\n", INCORPOREAL_FORM_USES(ch));
  if (INCORPOREAL_FORM_TIMER(ch) != PFDEF_INCORPOREAL_FORM_TIMER)
    BUFFER_WRITE( "InFT: %d\n", INCORPOREAL_FORM_TIMER(ch));

  if (GET_PSIONIC_ENERGY_TYPE(ch) != PFDEF_PSIONIC_ENERGY_TYPE)
    BUFFER_WRITE( "PsET: %d\n", GET_PSIONIC_ENERGY_TYPE(ch));

  if (GET_OLC_ZONE(ch) != PFDEF_OLC)
    BUFFER_WRITE( "Olc : %d\n", GET_OLC_ZONE(ch));
  if (GET_PAGE_LENGTH(ch) != PFDEF_PAGELENGTH)
    BUFFER_WRITE( "Page: %d\n", GET_PAGE_LENGTH(ch));
  if (GET_SCREEN_WIDTH(ch) != PFDEF_SCREENWIDTH)
    BUFFER_WRITE( "ScrW: %d\n", GET_SCREEN_WIDTH(ch));
  if (GET_QUESTPOINTS(ch) != PFDEF_QUESTPOINTS)
    BUFFER_WRITE( "Qstp: %d\n", GET_QUESTPOINTS(ch));
  if (GET_QUEST_COUNTER(ch, 0) != PFDEF_QUESTCOUNT)
    BUFFER_WRITE( "Qcnt: %d\n", GET_QUEST_COUNTER(ch, 0));
  if (GET_QUEST_COUNTER(ch, 1) != PFDEF_QUESTCOUNT)
    BUFFER_WRITE( "Qcn1: %d\n", GET_QUEST_COUNTER(ch, 1));
  if (GET_QUEST_COUNTER(ch, 2) != PFDEF_QUESTCOUNT)
    BUFFER_WRITE( "Qcn2: %d\n", GET_QUEST_COUNTER(ch, 2));
  if (GET_NUM_QUESTS(ch) != PFDEF_COMPQUESTS)
  {
    BUFFER_WRITE( "Qest:\n");
    for (i = 0; i < GET_NUM_QUESTS(ch); i++)
      BUFFER_WRITE( "%d\n", ch->player_specials->saved.completed_quests[i]);
    BUFFER_WRITE( "%d\n", NOTHING);
  }

  if (GET_NSUPPLY_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "SpCd: %d\n", GET_NSUPPLY_COOLDOWN(ch));
  if (GET_NSUPPLY_NUM_MADE(ch) != 0)
    BUFFER_WRITE( "SpNM: %d\n", GET_NSUPPLY_NUM_MADE(ch));

  if (GET_QUEST(ch, 0) != PFDEF_CURRQUEST)
    BUFFER_WRITE( "Qcur: %d\n", GET_QUEST(ch, 0));
  if (GET_QUEST(ch, 1) != PFDEF_CURRQUEST)
    BUFFER_WRITE( "Qcu1: %d\n", GET_QUEST(ch, 1));
  if (GET_QUEST(ch, 2) != PFDEF_CURRQUEST)
    BUFFER_WRITE( "Qcu2: %d\n", GET_QUEST(ch, 2));
  if (GET_DIPTIMER(ch) != PFDEF_DIPTIMER)
    BUFFER_WRITE( "DipT: %d\n", GET_DIPTIMER(ch));
  if (GET_CLAN(ch) != PFDEF_CLAN)
    BUFFER_WRITE( "Cln : %d\n", GET_CLAN(ch));
  if (GET_CLANRANK(ch) != PFDEF_CLANRANK)
    BUFFER_WRITE( "Clrk: %d\n", GET_CLANRANK(ch));
  if (GET_CLANPOINTS(ch) != PFDEF_CLANPOINTS)
    BUFFER_WRITE( "CPts: %d\n", GET_CLANPOINTS(ch));
  if (GET_SLAYER_JUDGEMENT(ch) != 0)
    BUFFER_WRITE( "Slyr: %d\n", GET_SLAYER_JUDGEMENT(ch));
  if (GET_BANE_TARGET_TYPE(ch) != 0)
    BUFFER_WRITE( "Bane: %d\n", GET_BANE_TARGET_TYPE(ch));
  if (GET_KAPAK_SALIVA_HEALING_COOLDOWN(ch) != 0)
    BUFFER_WRITE( "KpkS: %d\n", GET_KAPAK_SALIVA_HEALING_COOLDOWN(ch));
  if (SCRIPT(ch))
  {
    for (t = TRIGGERS(SCRIPT(ch)); t; t = t->next)
      BUFFER_WRITE( "Trig: %d\n", GET_TRIG_VNUM(t));
  }

  if (ch->desc)
  {
    BUFFER_WRITE( "GMCP: %d\n", ch->desc->pProtocol->bGMCP);
    BUFFER_WRITE( "XTrm: %d\n", ch->desc->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt);
    BUFFER_WRITE( "UTF8: %d\n", ch->desc->pProtocol->pVariables[eMSDP_UTF_8]->ValueInt);
  }

  if (GET_PREMADE_BUILD_CLASS(ch) != PFDEF_PREMADE_BUILD)
    BUFFER_WRITE( "PreB: %d\n", GET_PREMADE_BUILD_CLASS(ch));

  /* Save skills */
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    BUFFER_WRITE( "Skil:\n");
    for (i = 1; i < MAX_SKILLS; i++)
    {
      if (GET_SKILL(ch, i))
        BUFFER_WRITE( "%d %d\n", i, GET_SKILL(ch, i));
    }
    BUFFER_WRITE( "0 0\n");
  }

  /* Save abilities */
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    BUFFER_WRITE( "Ablt:\n");
    for (i = 1; i <= MAX_ABILITIES; i++)
    {
      if (GET_ABILITY(ch, i))
        BUFFER_WRITE( "%d %d\n", i, GET_ABILITY(ch, i));
    }
    BUFFER_WRITE( "0 0\n");
  }
  if (GET_LEVEL(ch) < LVL_IMMORT)
  {
    BUFFER_WRITE( "AbXP:\n");
    for (i = 1; i <= MAX_ABILITIES; i++)
    {
      if (GET_CRAFT_SKILL_EXP(ch, i))
        BUFFER_WRITE( "%d %d\n", i, GET_CRAFT_SKILL_EXP(ch, i));
    }
    BUFFER_WRITE( "0 0\n");
  }

  // Save Buffs
  BUFFER_WRITE( "Buff:\n");
  for (i = 0; i < MAX_BUFFS; i++)
    BUFFER_WRITE( "%d %d %d\n", i, GET_BUFF(ch, i, 0), GET_BUFF(ch, i, 1));
  BUFFER_WRITE( "-1 -1 -1\n");

  // Save Bags
  if (GET_BAG_NAME(ch, 1))
    BUFFER_WRITE( "Bag1: %s\n", GET_BAG_NAME(ch, 1));
  if (GET_BAG_NAME(ch, 2))
    BUFFER_WRITE( "Bag2: %s\n", GET_BAG_NAME(ch, 2));
  if (GET_BAG_NAME(ch, 3))
    BUFFER_WRITE( "Bag3: %s\n", GET_BAG_NAME(ch, 3));
  if (GET_BAG_NAME(ch, 4))
    BUFFER_WRITE( "Bag4: %s\n", GET_BAG_NAME(ch, 4));
  if (GET_BAG_NAME(ch, 5))
    BUFFER_WRITE( "Bag5: %s\n", GET_BAG_NAME(ch, 5));
  if (GET_BAG_NAME(ch, 6))
    BUFFER_WRITE( "Bag6: %s\n", GET_BAG_NAME(ch, 6));
  if (GET_BAG_NAME(ch, 7))
    BUFFER_WRITE( "Bag7: %s\n", GET_BAG_NAME(ch, 7));
  if (GET_BAG_NAME(ch, 8))
    BUFFER_WRITE( "Bag8: %s\n", GET_BAG_NAME(ch, 8));
  if (GET_BAG_NAME(ch, 9))
    BUFFER_WRITE( "Bag9: %s\n", GET_BAG_NAME(ch, 9));
  if (GET_BAG_NAME(ch, 10))
    BUFFER_WRITE( "Bag0: %s\n", GET_BAG_NAME(ch, 10));

  /* Save Bombs */
  BUFFER_WRITE( "Bomb:\n");
  for (i = 0; i < MAX_BOMBS_ALLOWED; i++)
    BUFFER_WRITE( "%d\n", GET_BOMB(ch, i));
  BUFFER_WRITE( "-1\n");

  // Save Craft mats onhand
  BUFFER_WRITE( "CfMt:\n");
  for (i = 0; i < NUM_CRAFT_MATS; i++)
    BUFFER_WRITE( "%d\n", GET_CRAFT_MAT(ch, i));
  BUFFER_WRITE( "-1\n");

  // Save Craft motes onhand
  BUFFER_WRITE( "Mote:\n");
  for (i = 0; i < NUM_CRAFT_MOTES; i++)
    BUFFER_WRITE( "%d\n", GET_CRAFT_MOTES(ch, i));
  BUFFER_WRITE( "-1\n");

  // Save Craft Affects
  BUFFER_WRITE( "CrAf:\n");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    BUFFER_WRITE( "%d %d %d %d %d\n", i, GET_CRAFT(ch).affected[i].location, GET_CRAFT(ch).affected[i].modifier, GET_CRAFT(ch).affected[i].bonus_type, GET_CRAFT(ch).affected[i].specific);
  BUFFER_WRITE( "-1\n");

  // Save Craft Motes
  BUFFER_WRITE( "CrMo:\n");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    BUFFER_WRITE( "%d %d\n", i, GET_CRAFT(ch).motes_required[i]);
  BUFFER_WRITE( "-1\n");

  // Save Craft Materials
  BUFFER_WRITE( "CrMa:\n");
  for (i = 0; i < NUM_CRAFT_GROUPS; i++)
    BUFFER_WRITE( "%d %d %d\n", i, GET_CRAFT(ch).materials[i][0], GET_CRAFT(ch).materials[i][1]);
  BUFFER_WRITE( "-1\n");

  // misc craft things
  BUFFER_WRITE( "CrMe: %d\n", GET_CRAFT(ch).crafting_method);
  BUFFER_WRITE( "CrIT: %d\n", GET_CRAFT(ch).crafting_item_type);
  BUFFER_WRITE( "CrSp: %d\n", GET_CRAFT(ch).crafting_specific);
  BUFFER_WRITE( "CrSk: %d\n", GET_CRAFT(ch).skill_type);
  BUFFER_WRITE( "CrRe: %d\n", GET_CRAFT(ch).crafting_recipe);
  BUFFER_WRITE( "CrVt: %d\n", GET_CRAFT(ch).craft_variant);
  BUFFER_WRITE( "CrMe: %d\n", GET_CRAFT(ch).crafting_method);
  BUFFER_WRITE( "CrEn: %d\n", GET_CRAFT(ch).enhancement);
  BUFFER_WRITE( "CrEM: %d\n", GET_CRAFT(ch).enhancement_motes_required);
  BUFFER_WRITE( "CrRl: %d\n", GET_CRAFT(ch).skill_roll);
  BUFFER_WRITE( "CrDC: %d\n", GET_CRAFT(ch).dc);
  BUFFER_WRITE( "CrDu: %d\n", GET_CRAFT(ch).craft_duration);
  BUFFER_WRITE( "CrKy: %s\n", GET_CRAFT(ch).keywords);
  BUFFER_WRITE( "CrSD: %s\n", GET_CRAFT(ch).short_description);
  BUFFER_WRITE( "CrRD: %s\n", GET_CRAFT(ch).room_description);
  BUFFER_WRITE( "CrEx: %s\n", GET_CRAFT(ch).ex_description);

  // refining stuff
  BUFFER_WRITE( "RM00: %d\n", GET_CRAFT(ch).refining_materials[0][0]);
  BUFFER_WRITE( "RM01: %d\n", GET_CRAFT(ch).refining_materials[0][1]);
  BUFFER_WRITE( "RM10: %d\n", GET_CRAFT(ch).refining_materials[1][0]);
  BUFFER_WRITE( "RM11: %d\n", GET_CRAFT(ch).refining_materials[1][1]);
  BUFFER_WRITE( "RM20: %d\n", GET_CRAFT(ch).refining_materials[2][0]);
  BUFFER_WRITE( "RM21: %d\n", GET_CRAFT(ch).refining_materials[2][1]);
  BUFFER_WRITE( "RRs0: %d\n", GET_CRAFT(ch).refining_result[0]);
  BUFFER_WRITE( "RRs1: %d\n", GET_CRAFT(ch).refining_result[1]);

  // resizing stuff
  BUFFER_WRITE( "RSSz: %d\n", GET_CRAFT(ch).new_size);
  BUFFER_WRITE( "RSMT: %d\n", GET_CRAFT(ch).resize_mat_type);
  BUFFER_WRITE( "RSMN: %d\n", GET_CRAFT(ch).resize_mat_num);

  BUFFER_WRITE( "CrOL: %d\n", GET_CRAFT(ch).obj_level);
  BUFFER_WRITE( "CrLA: %d\n", GET_CRAFT(ch).level_adjust);

  BUFFER_WRITE( "CrSN: %d\n", GET_CRAFT(ch).supply_num_required);
  BUFFER_WRITE( "CrSR: %d\n", GET_CRAFT(ch).survey_rooms);
  BUFFER_WRITE( "CrIy: %d\n", GET_CRAFT(ch).instrument_type);
  BUFFER_WRITE( "CrIQ: %d\n", GET_CRAFT(ch).instrument_quality);
  BUFFER_WRITE( "CrIE: %d\n", GET_CRAFT(ch).instrument_effectiveness);
  BUFFER_WRITE( "CrIB: %d\n", GET_CRAFT(ch).instrument_breakability);
  BUFFER_WRITE( "CrI1: %d\n", GET_CRAFT(ch).instrument_motes[1]);
  BUFFER_WRITE( "CrI2: %d\n", GET_CRAFT(ch).instrument_motes[2]);
  BUFFER_WRITE( "CrI3: %d\n", GET_CRAFT(ch).instrument_motes[3]);


  // Save consumables: potions, scrolls, wands and staves
  BUFFER_WRITE( "Potn:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_POTIONS(ch, i) > 0)
      BUFFER_WRITE( "%d %d\n", i, STORED_POTIONS(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Scrl:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_SCROLLS(ch, i) > 0)
      BUFFER_WRITE( "%d %d\n", i, STORED_SCROLLS(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Stav:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_STAVES(ch, i) > 0)
      BUFFER_WRITE( "%d %d\n", i, STORED_STAVES(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Wand:\n");
  for (i = 0; i < MAX_SPELLS; i++)
    if (STORED_WANDS(ch, i) > 0)
      BUFFER_WRITE( "%d %d\n", i, STORED_WANDS(ch, i));
  BUFFER_WRITE( "-1\n");
  // End save consumables

  BUFFER_WRITE( "Disc:\n");
  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
    BUFFER_WRITE( "%d\n", KNOWS_DISCOVERY(ch, i));
  BUFFER_WRITE( "-1\n");
  BUFFER_WRITE( "GrDs: %d\n", GET_GRAND_DISCOVERY(ch));

  BUFFER_WRITE( "Mrcy:\n");
  for (i = 0; i < NUM_PALADIN_MERCIES; i++)
    BUFFER_WRITE( "%d\n", KNOWS_MERCY(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "FDQs:\n");
  for (i = 0; i < 100; i++)
    if (ch->player_specials->saved.failed_dialogue_quests[i] > 0)
      BUFFER_WRITE( "%d\n", ch->player_specials->saved.failed_dialogue_quests[i]);
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Clty:\n");
  for (i = 0; i < NUM_BLACKGUARD_CRUELTIES; i++)
    BUFFER_WRITE( "%d\n", KNOWS_CRUELTY(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Judg:\n");
  for (i = 0; i < NUM_INQ_JUDGEMENTS; i++)
    BUFFER_WRITE( "%d\n", IS_JUDGEMENT_ACTIVE(ch, i));
  BUFFER_WRITE( "-1\n");

  BUFFER_WRITE( "Lang:\n");
  for (i = 0; i < NUM_LANGUAGES; i++)
    BUFFER_WRITE( "%d\n", ch->player_specials->saved.languages_known[i]);
  BUFFER_WRITE( "-1\n");

  /* Save Combat Feats */
  for (i = 0; i < NUM_CFEATS; i++)
  {
    sprintascii(bits, ch->char_specials.saved.combat_feats[i][0]);
    sprintascii(bits2, ch->char_specials.saved.combat_feats[i][1]);
    sprintascii(bits3, ch->char_specials.saved.combat_feats[i][2]);
    sprintascii(bits4, ch->char_specials.saved.combat_feats[i][3]);
    BUFFER_WRITE( "CbFt: %d %s %s %s %s\n", i, bits, bits2, bits3, bits4);
  }

  /* Save School Feats */
  for (i = 0; i < NUM_SFEATS; i++)
  {
    sprintascii(bits, ch->char_specials.saved.school_feats[i]);
    BUFFER_WRITE( "SclF: %d %s\n", i, bits);
  }

  /* Save Skill Foci */
  BUFFER_WRITE( "SklF:\n");
  for (i = 0; i < MAX_ABILITIES; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_SKFEATS; j++)
    {
      BUFFER_WRITE( "%d ", ch->player_specials->saved.skill_focus[i][j]);
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "-1 -1 -1\n");

  /* Save feats */
  BUFFER_WRITE( "Feat:\n");
  for (i = 1; i < NUM_FEATS; i++)
  {
    if (HAS_REAL_FEAT(ch, i))
      BUFFER_WRITE( "%d %d\n", i, HAS_REAL_FEAT(ch, i));
  }
  BUFFER_WRITE( "0 0\n");

  /* Save evolutions */
  BUFFER_WRITE( "Evol:\n");
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (HAS_REAL_EVOLUTION(ch, i))
      BUFFER_WRITE( "%d %d\n", i, HAS_REAL_EVOLUTION(ch, i));
  }
  BUFFER_WRITE( "0 0\n");

  /* Save temp evolutions */
  BUFFER_WRITE( "TEvo:\n");
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (HAS_TEMP_EVOLUTION(ch, i))
      BUFFER_WRITE( "%d %d\n", i, HAS_TEMP_EVOLUTION(ch, i));
  }
  BUFFER_WRITE( "0 0\n");

  /* Save known evolutions */
  BUFFER_WRITE( "KEvo:\n");
  for (i = 1; i < NUM_EVOLUTIONS; i++)
  {
    if (KNOWS_EVOLUTION(ch, i))
      BUFFER_WRITE( "%d %d\n", i, KNOWS_EVOLUTION(ch, i));
  }
  BUFFER_WRITE( "0 0\n");

  /* spell prep system */
  save_spell_prep_queue(fl, ch);
  save_innate_magic_queue(fl, ch);
  save_spell_collection(fl, ch);
  save_known_spells(fl, ch);
  /* end spell prep system */

  // Save memorizing list of prayers, prayed list and times
  /* Note: added metamagic to pfile.  19.01.2015 Ornir */
  BUFFER_WRITE( "Pryg:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      if (PREPARATION_QUEUE(ch, i, j).spell < MAX_SPELLS)
        BUFFER_WRITE( "%d ", PREPARATION_QUEUE(ch, i, j).spell);
      else
        BUFFER_WRITE( "0 ");
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "Prgm:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      if (PREPARATION_QUEUE(ch, i, j).spell < MAX_SPELLS)
        BUFFER_WRITE( "%d ", PREPARATION_QUEUE(ch, i, j).metamagic);
      else
        BUFFER_WRITE( "0 ");
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "-1 -1\n");
  BUFFER_WRITE( "Pryd:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      BUFFER_WRITE( "%d ", PREPARED_SPELLS(ch, i, j).spell);
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "-1 -1\n");

  BUFFER_WRITE( "Pryt:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      BUFFER_WRITE( "%d ", PREP_TIME(ch, i, j));
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "-1 -1\n");

  BUFFER_WRITE( "Prdm:\n");
  for (i = 0; i < MAX_MEM; i++)
  {
    BUFFER_WRITE( "%d ", i);
    for (j = 0; j < NUM_CASTERS; j++)
    {
      BUFFER_WRITE( "%d ", PREPARED_SPELLS(ch, i, j).metamagic);
    }
    BUFFER_WRITE( "\n");
  }
  BUFFER_WRITE( "-1 -1\n");

  // class levels
  BUFFER_WRITE( "CLvl:\n");
  for (i = 0; i < MAX_CLASSES; i++)
  {
    BUFFER_WRITE( "%d %d\n", i, CLASS_LEVEL(ch, i));
  }
  BUFFER_WRITE( "-1 -1\n");

  // coordinate location
  BUFFER_WRITE( "CLoc:\n");
  BUFFER_WRITE( "%d %d\n", ch->coords[0], ch->coords[1]);

  // warding, etc..
  BUFFER_WRITE( "Ward:\n");
  for (i = 0; i < MAX_WARDING; i++)
  {
    BUFFER_WRITE( "%d %d\n", i, GET_WARDING(ch, i));
  }
  BUFFER_WRITE( "-1 -1\n");

  // spec abilities
  BUFFER_WRITE( "SpAb:\n");
  for (i = 0; i < MAX_CLASSES; i++)
  {
    BUFFER_WRITE( "%d %d\n", i, GET_SPEC_ABIL(ch, i));
  }
  BUFFER_WRITE( "-1 -1\n");

  // favored enemies (rangers)
  BUFFER_WRITE( "FaEn:\n");
  for (i = 0; i < MAX_ENEMIES; i++)
  {
    BUFFER_WRITE( "%d %d\n", i, GET_FAVORED_ENEMY(ch, i));
  }
  BUFFER_WRITE( "-1 -1\n");

  /* save_char(x, 1) will skip this block (i.e. not saving events)
     this is necessary due to clearing events that occurs immediately
     before extract_char_final() in extract_char() -Zusuk */
  if (mode != 1)
  {
    /* Save events */
    /* Not going to save every event */
    BUFFER_WRITE( "Evnt:\n");
    /* Order:  Event-ID   Duration */
    /* eSTRUGGLE - don't need to save this */
    if ((pMudEvent = char_has_mud_event(ch, eINVISIBLE_ROGUE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_INVISIBLE_ROGUE) - daily_uses_remaining(ch, FEAT_INVISIBLE_ROGUE));
    if ((pMudEvent = char_has_mud_event(ch, eVANISHED)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_VANISH) - daily_uses_remaining(ch, FEAT_VANISH));
    if ((pMudEvent = char_has_mud_event(ch, eVANISH)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_VANISH) - daily_uses_remaining(ch, FEAT_VANISH));
    if ((pMudEvent = char_has_mud_event(ch, eTAUNT)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTAUNTED)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINTIMIDATED)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eINTIMIDATE_COOLDOWN)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eRAGE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RAGE) - daily_uses_remaining(ch, FEAT_RAGE));
    if ((pMudEvent = char_has_mud_event(ch, eSACRED_FLAMES)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SACRED_FLAMES) - daily_uses_remaining(ch, FEAT_SACRED_FLAMES));
    if ((pMudEvent = char_has_mud_event(ch, eINNER_FIRE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_INNER_FIRE) - daily_uses_remaining(ch, FEAT_INNER_FIRE));
    if ((pMudEvent = char_has_mud_event(ch, eMUTAGEN)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_MUTAGEN) - daily_uses_remaining(ch, FEAT_MUTAGEN));
    if ((pMudEvent = char_has_mud_event(ch, eCRIPPLING_CRITICAL)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CRIPPLING_CRITICAL) - daily_uses_remaining(ch, FEAT_CRIPPLING_CRITICAL));
    if ((pMudEvent = char_has_mud_event(ch, eDEFENSIVE_STANCE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DEFENSIVE_STANCE) - daily_uses_remaining(ch, FEAT_DEFENSIVE_STANCE));
    if ((pMudEvent = char_has_mud_event(ch, eINSECTBEING)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_INSECTBEING) - daily_uses_remaining(ch, FEAT_INSECTBEING));
    if ((pMudEvent = char_has_mud_event(ch, eCRYSTALFIST)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CRYSTAL_FIST) - daily_uses_remaining(ch, FEAT_CRYSTAL_FIST));
    if ((pMudEvent = char_has_mud_event(ch, eCRYSTALBODY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CRYSTAL_BODY) - daily_uses_remaining(ch, FEAT_CRYSTAL_BODY));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_STRENGTH)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_STRENGTH) - daily_uses_remaining(ch, FEAT_SLA_STRENGTH));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_ENLARGE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_ENLARGE) - daily_uses_remaining(ch, FEAT_SLA_ENLARGE));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_INVIS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_INVIS) - daily_uses_remaining(ch, FEAT_SLA_INVIS));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_LEVITATE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_LEVITATE) - daily_uses_remaining(ch, FEAT_SLA_LEVITATE));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_DARKNESS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_DARKNESS) - daily_uses_remaining(ch, FEAT_SLA_DARKNESS));
    if ((pMudEvent = char_has_mud_event(ch, eSLA_FAERIE_FIRE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SLA_FAERIE_FIRE) - daily_uses_remaining(ch, FEAT_SLA_FAERIE_FIRE));
    if ((pMudEvent = char_has_mud_event(ch, eLAYONHANDS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_LAYHANDS) - daily_uses_remaining(ch, FEAT_LAYHANDS));
    if ((pMudEvent = char_has_mud_event(ch, eTOUCHOFCORRUPTION)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_TOUCH_OF_CORRUPTION) - daily_uses_remaining(ch, FEAT_TOUCH_OF_CORRUPTION));
    if ((pMudEvent = char_has_mud_event(ch, eJUDGEMENT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_JUDGEMENT) - daily_uses_remaining(ch, FEAT_JUDGEMENT));
    if ((pMudEvent = char_has_mud_event(ch, eTRUEJUDGEMENT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_TRUE_JUDGEMENT) - daily_uses_remaining(ch, FEAT_TRUE_JUDGEMENT));
    if ((pMudEvent = char_has_mud_event(ch, eCHILDRENOFTHENIGHT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT) - daily_uses_remaining(ch, FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT));
    if ((pMudEvent = char_has_mud_event(ch, eVAMPIREENERGYDRAIN)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_VAMPIRE_ENERGY_DRAIN) - daily_uses_remaining(ch, FEAT_VAMPIRE_ENERGY_DRAIN));
    if ((pMudEvent = char_has_mud_event(ch, eVAMPIREBLOODDRAIN)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_VAMPIRE_BLOOD_DRAIN) - daily_uses_remaining(ch, FEAT_VAMPIRE_BLOOD_DRAIN));
    if ((pMudEvent = char_has_mud_event(ch, eBANE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_BANE) - daily_uses_remaining(ch, FEAT_BANE));
    if ((pMudEvent = char_has_mud_event(ch, eMASTERMIND)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_MASTER_OF_THE_MIND) - daily_uses_remaining(ch, FEAT_MASTER_OF_THE_MIND));
    if ((pMudEvent = char_has_mud_event(ch, eDANCINGWEAPON)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eSPIRITUALWEAPON)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eCHANNELENERGY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CHANNEL_ENERGY) - daily_uses_remaining(ch, FEAT_CHANNEL_ENERGY));
    if ((pMudEvent = char_has_mud_event(ch, eEMPTYBODY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_EMPTY_BODY) - daily_uses_remaining(ch, FEAT_EMPTY_BODY));
    if ((pMudEvent = char_has_mud_event(ch, eWHOLENESSOFBODY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_WHOLENESS_OF_BODY) - daily_uses_remaining(ch, FEAT_WHOLENESS_OF_BODY));
    if ((pMudEvent = char_has_mud_event(ch, eRENEWEDDEFENSE)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RENEWED_DEFENSE) - daily_uses_remaining(ch, FEAT_RENEWED_DEFENSE));
    if ((pMudEvent = char_has_mud_event(ch, eRENEWEDVIGOR)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RP_RENEWED_VIGOR) - daily_uses_remaining(ch, FEAT_RP_RENEWED_VIGOR));
    if ((pMudEvent = char_has_mud_event(ch, eTREATINJURY)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eMUMMYDUST)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRAGONKNIGHT)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eGREATERRUIN)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eHELLBALL)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eEPICMAGEARMOR)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eEPICWARDING)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDEATHARROW)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eQUIVERINGPALM)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eANIMATEDEAD)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_ANIMATE_DEAD) - daily_uses_remaining(ch, FEAT_ANIMATE_DEAD));
    if ((pMudEvent = char_has_mud_event(ch, eSTUNNINGFIST)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_STUNNING_FIST) - daily_uses_remaining(ch, FEAT_STUNNING_FIST));
    if ((pMudEvent = char_has_mud_event(ch, eSURPRISE_ACCURACY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RP_SURPRISE_ACCURACY) - daily_uses_remaining(ch, FEAT_RP_SURPRISE_ACCURACY));
    if ((pMudEvent = char_has_mud_event(ch, eCOME_AND_GET_ME)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RP_COME_AND_GET_ME) - daily_uses_remaining(ch, FEAT_RP_COME_AND_GET_ME));
    if ((pMudEvent = char_has_mud_event(ch, ePOWERFUL_BLOW)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RP_POWERFUL_BLOW) - daily_uses_remaining(ch, FEAT_RP_POWERFUL_BLOW));
    if ((pMudEvent = char_has_mud_event(ch, eD_ROLL)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DEFENSIVE_ROLL) - daily_uses_remaining(ch, FEAT_DEFENSIVE_ROLL));
    if ((pMudEvent = char_has_mud_event(ch, eLAST_WORD)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_LAST_WORD) - daily_uses_remaining(ch, FEAT_LAST_WORD));
    if ((pMudEvent = char_has_mud_event(ch, ePURIFY)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_REMOVE_DISEASE) - daily_uses_remaining(ch, FEAT_REMOVE_DISEASE));
    if ((pMudEvent = char_has_mud_event(ch, eC_ANIMAL)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_ANIMAL_COMPANION) - daily_uses_remaining(ch, FEAT_ANIMAL_COMPANION));
    if ((pMudEvent = char_has_mud_event(ch, eC_DRAGONMOUNT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRAGON_BOND) - daily_uses_remaining(ch, FEAT_DRAGON_BOND));
    if ((pMudEvent = char_has_mud_event(ch, eC_EIDOLON)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_EIDOLON) - daily_uses_remaining(ch, FEAT_EIDOLON));
    if ((pMudEvent = char_has_mud_event(ch, eC_FAMILIAR)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SUMMON_FAMILIAR) - daily_uses_remaining(ch, FEAT_SUMMON_FAMILIAR));
    if ((pMudEvent = char_has_mud_event(ch, eC_MOUNT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CALL_MOUNT) - daily_uses_remaining(ch, FEAT_CALL_MOUNT));
    if ((pMudEvent = char_has_mud_event(ch, eSUMMONSHADOW)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SUMMON_SHADOW) - daily_uses_remaining(ch, FEAT_SUMMON_SHADOW));
    if ((pMudEvent = char_has_mud_event(ch, eTURN_UNDEAD)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_TURN_UNDEAD) - daily_uses_remaining(ch, FEAT_TURN_UNDEAD));
    if ((pMudEvent = char_has_mud_event(ch, eSPELLBATTLE)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    // if ((pMudEvent = char_has_mud_event(ch, eQUEST_COMPLETE)))
    //   BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eDRACBREATH)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRACONIC_HERITAGE_BREATHWEAPON) - daily_uses_remaining(ch, FEAT_DRACONIC_HERITAGE_BREATHWEAPON));
    if ((pMudEvent = char_has_mud_event(ch, eDRACCLAWS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRACONIC_HERITAGE_CLAWS) - daily_uses_remaining(ch, FEAT_DRACONIC_HERITAGE_CLAWS));
    if ((pMudEvent = char_has_mud_event(ch, eDRAGBREATH)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRAGONBORN_BREATH) - daily_uses_remaining(ch, FEAT_DRAGONBORN_BREATH));
    if ((pMudEvent = char_has_mud_event(ch, eCATSCLAWS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_TABAXI_CATS_CLAWS) - daily_uses_remaining(ch, FEAT_TABAXI_CATS_CLAWS));
    if ((pMudEvent = char_has_mud_event(ch, eARCANEADEPT)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_METAMAGIC_ADEPT) - daily_uses_remaining(ch, FEAT_METAMAGIC_ADEPT));
    if ((pMudEvent = char_has_mud_event(ch, eCHANNELSPELL)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CHANNEL_SPELL) - daily_uses_remaining(ch, FEAT_CHANNEL_SPELL));
    if ((pMudEvent = char_has_mud_event(ch, ePSIONICFOCUS)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_PSIONIC_FOCUS) - daily_uses_remaining(ch, FEAT_PSIONIC_FOCUS));
    if ((pMudEvent = char_has_mud_event(ch, eDOUBLEMANIFEST)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DOUBLE_MANIFEST) - daily_uses_remaining(ch, FEAT_DOUBLE_MANIFEST));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWCALL)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SHADOW_CALL) - daily_uses_remaining(ch, FEAT_SHADOW_CALL));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWJUMP)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SHADOW_JUMP) - daily_uses_remaining(ch, FEAT_SHADOW_JUMP));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWILLUSION)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SHADOW_ILLUSION) - daily_uses_remaining(ch, FEAT_SHADOW_ILLUSION));
    if ((pMudEvent = char_has_mud_event(ch, eSHADOWPOWER)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SHADOW_POWER) - daily_uses_remaining(ch, FEAT_SHADOW_POWER));
    if ((pMudEvent = char_has_mud_event(ch, eEVOBREATH)))
      BUFFER_WRITE( "%d %ld\n", pMudEvent->iId, event_time(pMudEvent->pEvent));
    if ((pMudEvent = char_has_mud_event(ch, eTOUCHOFUNDEATH)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_TOUCH_OF_UNDEATH) - daily_uses_remaining(ch, FEAT_TOUCH_OF_UNDEATH));
    if ((pMudEvent = char_has_mud_event(ch, eSTRENGTHOFHONOR)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_STRENGTH_OF_HONOR) - daily_uses_remaining(ch, FEAT_STRENGTH_OF_HONOR));
    if ((pMudEvent = char_has_mud_event(ch, eCROWNOFKNIGHTHOOD)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_CROWN_OF_KNIGHTHOOD) - daily_uses_remaining(ch, FEAT_CROWN_OF_KNIGHTHOOD));
    if ((pMudEvent = char_has_mud_event(ch, eSOULOFKNIGHTHOOD)))
      BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SOUL_OF_KNIGHTHOOD) - daily_uses_remaining(ch, FEAT_SOUL_OF_KNIGHTHOOD));
    if ((pMudEvent = char_has_mud_event(ch, eINSPIRECOURAGE)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_INSPIRE_COURAGE) - daily_uses_remaining(ch, FEAT_INSPIRE_COURAGE));
    if ((pMudEvent = char_has_mud_event(ch, eWISDOMOFTHEMEASURE)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_WISDOM_OF_THE_MEASURE) - daily_uses_remaining(ch, FEAT_WISDOM_OF_THE_MEASURE));
    if ((pMudEvent = char_has_mud_event(ch, eFINALSTAND)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_FINAL_STAND) - daily_uses_remaining(ch, FEAT_FINAL_STAND));
    if ((pMudEvent = char_has_mud_event(ch, eKNIGHTHOODSFLOWER)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_KNIGHTHOODS_FLOWER) - daily_uses_remaining(ch, FEAT_KNIGHTHOODS_FLOWER));
    if ((pMudEvent = char_has_mud_event(ch, eRALLYINGCRY)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_RALLYING_CRY) - daily_uses_remaining(ch, FEAT_RALLYING_CRY));
    if ((pMudEvent = char_has_mud_event(ch, eCOSMICUNDERSTANDING)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_COSMIC_UNDERSTANDING) - daily_uses_remaining(ch, FEAT_COSMIC_UNDERSTANDING));
    if ((pMudEvent = char_has_mud_event(ch, eDRAGOONPOINTS)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRAGOON_POINTS) - daily_uses_remaining(ch, FEAT_DRAGOON_POINTS));
    if ((pMudEvent = char_has_mud_event(ch, eC_DRAGONMOUNT)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DRAGON_BOND) - daily_uses_remaining(ch, FEAT_DRAGON_BOND));
    if ((pMudEvent = char_has_mud_event(ch, eSMITE_EVIL)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SMITE_EVIL) - daily_uses_remaining(ch, FEAT_SMITE_EVIL));
    if ((pMudEvent = char_has_mud_event(ch, eSMITE_GOOD)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_SMITE_GOOD) - daily_uses_remaining(ch, FEAT_SMITE_GOOD));
    if ((pMudEvent = char_has_mud_event(ch, eSMITE_DESTRUCTION)))
        BUFFER_WRITE( "%d %ld %d\n", pMudEvent->iId, event_time(pMudEvent->pEvent), get_daily_uses(ch, FEAT_DESTRUCTIVE_SMITE) - daily_uses_remaining(ch, FEAT_DESTRUCTIVE_SMITE));

    BUFFER_WRITE( "-1 -1\n");
  }

  /* Save affects */
  if (tmp_aff[0].spell > 0)
  {
    BUFFER_WRITE( "Affs:\n");
    for (i = 0; i < MAX_AFFECT; i++)
    {
      aff = &tmp_aff[i];
      if (aff->spell)
        BUFFER_WRITE(
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
    BUFFER_WRITE( "0 0 0 0 0 0 0 0 0 0\n");
  }

  /* Save Damage Reduction */
  if ((tmp_dr != NULL) || (GET_DR(ch) != NULL))
  {
    struct damage_reduction_type *dr;
    int k = 0, x = 0;
    int max_loops = 100; /* zusuk put this here to limit the loop, we were having issues with pfiles 10/27/22 */
    int snum[100] = {0};  /* Initialize array to prevent uninitialized value access */
    bool found = false;

    BUFFER_WRITE( "DmgR:\n");

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

      BUFFER_WRITE( "1 %d %d %d %d\n", dr->amount, dr->max_damage, dr->spell, dr->feat);
      for (k = 0; k < MAX_DR_BYPASS; k++)
      {
        BUFFER_WRITE( "%d %d\n", dr->bypass_cat[k], dr->bypass_val[k]);
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
      BUFFER_WRITE( "1 %d %d %d %d\n", dr->amount, dr->max_damage, dr->spell, dr->feat);
      for (k = 0; k < MAX_DR_BYPASS; k++)
      {
        BUFFER_WRITE( "%d %d\n", dr->bypass_cat[k], dr->bypass_val[k]);
      }
    }

    /* done close off */
    BUFFER_WRITE( "0 0 0 0 0\n");
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
    {
      /* Free existing string to prevent memory leak */
      if (ch->desc->account->character_names[i] != NULL)
        free(ch->desc->account->character_names[i]);
      ch->desc->account->character_names[i] = strdup(GET_NAME(ch));
    }
    save_account(ch->desc->account);
  }

  /* Write buffer to file and close */
  if (buffer_used > 0) {
    if (fwrite(write_buffer, 1, buffer_used, fl) != buffer_used) {
      log("SYSERR: save_char: Failed to write buffer for %s", GET_NAME(ch));
    }
  }
  
  /* FILE CLOSED!!! */
  fclose(fl);
  
  /* Free the write buffer */
  free(write_buffer);
  #undef BUFFER_WRITE

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
  
  /* Log performance metrics */
  gettimeofday(&end_time, NULL);
  long elapsed_usec = (end_time.tv_sec - start_time.tv_sec) * 1000000 + 
                      (end_time.tv_usec - start_time.tv_usec);
  long elapsed_ms = elapsed_usec / 1000;
  
  /* Log if save took more than 260ms */
  if (elapsed_ms > 260) {
    log("PERF: save_char(%s) took %ldms (buffer: %zu bytes)", 
        GET_NAME(ch), elapsed_ms, buffer_used);
  }
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

static void load_craft_affects(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d %d %d", &num, &num2, &num3, &num4, &num5);
    if (num != -1)
    {
      GET_CRAFT(ch).affected[num].location = num2;
      GET_CRAFT(ch).affected[num].modifier = num3;
      GET_CRAFT(ch).affected[num].bonus_type = num4;
      GET_CRAFT(ch).affected[num].specific = num5;
    }
  } while (num != -1);
}

static void load_craft_materials(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0, num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d %d", &num, &num2, &num3);
    if (num != -1)
    {
      GET_CRAFT(ch).materials[num][0] = num2;
      GET_CRAFT(ch).materials[num][1] = num3;
    }
  } while (num != -1);
}

static void load_craft_motes_onhand(FILE *fl, struct char_data *ch)
{
  int num = 0;
  char line[MAX_INPUT_LENGTH + 1];

  int i = 0;

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
      GET_CRAFT_MOTES(ch, i) = num;
    i++;
  } while (num != -1);
}

static void load_craft_motes(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != -1)
    {
      GET_CRAFT(ch).motes_required[num] = num2;
    }
  } while (num != -1);
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
      else if (n_vars == 7)
      { /* New 128-bit version */
        af.bitvector[0] = num5;
        af.bitvector[1] = num6;
        af.bitvector[2] = num7;
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

static void load_failed_dialogue_quests(FILE *fl, struct char_data *ch)
{
  int num = 0, i = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
    {
      ch->player_specials->saved.failed_dialogue_quests[i] = num;
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

static void load_craft_mats_onhand(FILE *fl, struct char_data *ch)
{
  int num = 0;
  char line[MAX_INPUT_LENGTH + 1];

  int i = 0;

  do
  {
    get_line(fl, line);
    sscanf(line, "%d", &num);
    if (num != -1)
      GET_CRAFT_MAT(ch, i) = num;
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

static void load_ability_exp(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      GET_CRAFT_SKILL_EXP(ch, num) = num2;
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

void load_evolutions(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      HAS_REAL_EVOLUTION(ch, num) = num2;
  } while (num != 0);
}

void load_temp_evolutions(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      HAS_TEMP_EVOLUTION(ch, num) = num2;
  } while (num != 0);
}

void load_known_evolutions(FILE *fl, struct char_data *ch)
{
  int num = 0, num2 = 0;
  char line[MAX_INPUT_LENGTH + 1];

  do
  {
    get_line(fl, line);
    sscanf(line, "%d %d", &num, &num2);
    if (num != 0)
      KNOWS_EVOLUTION(ch, num) = num2;
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
  int num3 = 0;
  char line[MAX_INPUT_LENGTH + 1];
  char uses[SMALL_STRING];

  do
  {
    get_line(fl, line);
    if (sscanf(line, "%d %ld %d", &num, &num2, &num3) == 2)
    {
      if (num != -1)
        attach_mud_event(new_mud_event(num, ch, NULL), num2);
    }
    else
    {
      if (num != -1)
      {
        snprintf(uses, sizeof(uses), "uses:%d", num3);
        attach_mud_event(new_mud_event(num, ch, uses), num2);
      }
    }
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
  struct char_data *ch = NULL;
  char buf[2048]; /* For MySQL insert. */
  char char_info[1000];
  char classes_list[MAX_INPUT_LENGTH] = {'\0'};
  size_t len = 0;
  int class_len = 0;

  for (d = descriptor_list; d; d = d->next)
  {

    if (!d || !d->character)
      continue;

    ch = d->character;

    len = 0;

    if (GET_LEVEL(ch) < LVL_IMMORT)
    {
      int inc, classCount = 0;
      len += snprintf(classes_list + len, sizeof(classes_list) - len, "[%2d %4s ", GET_LEVEL(ch), RACE_ABBR_REAL(ch));
      for (inc = 0; inc < MAX_CLASSES; inc++)
      {
        if (CLASS_LEVEL(ch, inc))
        {
          if (classCount)
            len += snprintf(classes_list + len, sizeof(classes_list) - len, "|");
          len += snprintf(classes_list + len, sizeof(classes_list) - len, "%s", CLSLIST_ABBRV(inc));
          classCount++;
        }
      }
      class_len = strlen(classes_list) - count_color_chars(classes_list);
      while (class_len < 11)
      {
        len += snprintf(classes_list + len, sizeof(classes_list) - len, " ");
        class_len++;
      }
      snprintf(char_info, sizeof(char_info), "%s]", classes_list);
    }
    else
    {
      snprintf(char_info, sizeof(char_info), "[%2d %s] ", GET_LEVEL(ch), GET_IMM_TITLE(ch));
    }

    char *escaped_char_info = mysql_escape_string_alloc(conn, char_info);
    char *escaped_name_update = mysql_escape_string_alloc(conn, GET_NAME(d->character));
    if (!escaped_char_info || !escaped_name_update) {
      log("SYSERR: Failed to escape strings in save_char mysql update");
      if (escaped_char_info) free(escaped_char_info);
      if (escaped_name_update) free(escaped_name_update);
    } else {
      snprintf(buf, sizeof(buf), "UPDATE player_data SET last_online = NOW(), character_info='%s' WHERE name = '%s';", escaped_char_info, escaped_name_update);
      free(escaped_char_info);
      free(escaped_name_update);
      if (mysql_query(conn, buf))
      {
        /* Try without character_info column for compatibility */
        char *escaped_name_update2 = mysql_escape_string_alloc(conn, GET_NAME(d->character));
        if (!escaped_name_update2) {
          log("SYSERR: Failed to escape character name in save_char mysql fallback");
        } else {
          snprintf(buf, sizeof(buf), "UPDATE player_data SET last_online = NOW() WHERE name = '%s';", escaped_name_update2);
          free(escaped_name_update2);
          if (mysql_query(conn, buf))
          {
            log("SYSERR: Unable to UPDATE last_online for %s on PLAYER_DATA: %s", GET_NAME(d->character), mysql_error(conn));
          }
        }
      }
    }
  }
}

bool valid_pet_name(char *name)
{
  if (strstr(name, ";") || strstr(name, "\""))
    return false;
  return true;
}

void save_char_pets(struct char_data *ch)
{

  if (!ch || !ch->desc || IS_NPC(ch))
    return;

  struct follow_type *f = NULL;
  struct char_data *tch = NULL;
  char query[MAX_STRING_LENGTH] = {'\0'};
  char query2[MAX_STRING_LENGTH] = {'\0'};
  char query3[MAX_STRING_LENGTH] = {'\0'};
  char finalQuery[MAX_STRING_LENGTH] = {'\0'};
  char chname[MAX_STRING_LENGTH] = {'\0'};
  char *end = NULL, *end2 = NULL;

  snprintf(chname, sizeof(chname), "%s", GET_NAME(ch));

  mysql_ping(conn);

  char del_buf[2048];
  /* Delete existing save data.  In the future may just flag these for deletion. */
  snprintf(del_buf, sizeof(del_buf), "delete from pet_save_objs where owner_name = '%s';", GET_NAME(ch));
  if (mysql_query(conn, del_buf))
  {
    /* Table might not exist, continue anyway */
    log("INFO: pet_save_objs table might not exist: %s", mysql_error(conn));
  }

  end = stpcpy(query, "DELETE FROM pet_data WHERE owner_name=");
  *end++ = '\'';
  end += mysql_real_escape_string(conn, end, chname, strlen(chname));
  *end++ = '\'';
  *end++ = '\0';

  if (mysql_query(conn, query))
  {
    log("SYSERR: Unable to DELETE from pet_data: %s", mysql_error(conn));
  }

  for (f = ch->followers; f; f = f->next)
  {
    tch = f->follower;
    if (!IS_NPC(tch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    snprintf(query2, sizeof(query2), "INSERT INTO pet_data (pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, hp, max_hp, str, con, dex, ac, wis, cha) VALUES(NULL,");

    end2 = stpcpy(query2, "INSERT INTO pet_data (pet_data_id, owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, level, hp, max_hp, str, con, dex, ac, wis, cha) VALUES(NULL,");
    *end2++ = '\'';
    end2 += mysql_real_escape_string(conn, end2, GET_NAME(ch), strlen(GET_NAME(ch)));
    *end2++ = '\'';
    *end2++ = ',';
    
    if (valid_pet_name(tch->player.name))
    {
      *end2++ = '\'';
      end2 += mysql_real_escape_string(conn, end2, GET_NAME(tch), strlen(GET_NAME(tch)));
      *end2++ = '\'';
    }
    else
    {
      *end2++ = '\'';
      *end2++ = '\'';
    }
    *end2++ = ',';
    if (valid_pet_name(tch->player.short_descr))
    {
      *end2++ = '\'';
      end2 += mysql_real_escape_string(conn, end2, tch->player.short_descr, strlen(tch->player.short_descr));
      *end2++ = '\'';
    }
    else
    {
      *end2++ = '\'';
      *end2++ = '\'';
    }
    *end2++ = ',';
    if (valid_pet_name(tch->player.long_descr))
    {
      *end2++ = '\'';
      end2 += mysql_real_escape_string(conn, end2, tch->player.long_descr, strlen(tch->player.long_descr));
      *end2++ = '\'';
    }
    else
    {
      *end2++ = '\'';
      *end2++ = '\'';
    }
    *end2++ = ',';
    if (valid_pet_name(tch->player.description))
    {
      *end2++ = '\'';
      end2 += mysql_real_escape_string(conn, end2, tch->player.description, strlen(tch->player.description));
      *end2++ = '\'';
    }
    else
    {
      *end2++ = '\'';
      *end2++ = '\'';
    }
    *end2++ = ',';
    
    *end2++ = '\0';

    snprintf(query3, sizeof(query3), "'%d','%d','%d','%d','%d','%d','%d','%d','%d','%d')",
             GET_MOB_VNUM(tch), GET_LEVEL(tch), GET_HIT(tch), GET_REAL_MAX_HIT(tch),
             GET_REAL_STR(tch), GET_REAL_CON(tch), GET_REAL_DEX(tch), GET_REAL_AC(tch),
             GET_REAL_WIS(tch), GET_REAL_CHA(tch));
    snprintf(finalQuery, sizeof(finalQuery), "%s%s", query2, query3);
    if (mysql_query(conn, finalQuery))
    {
      log("SYSERR: Unable to INSERT INTO pet_data: %s", mysql_error(conn));
      log("QUERY: %s", finalQuery);
      return;
    }
    long int insert_id = 0;
    insert_id = mysql_insert_id(conn);

    if (insert_id > 0)
    {
      pet_save_objs(tch, ch, insert_id);
    }

  }
}

void load_char_pets(struct char_data *ch)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[200];
  char buf[MAX_EXTRA_DESC];
  char desc1[MAX_STRING_LENGTH] = {'\0'}; char desc2[MAX_STRING_LENGTH] = {'\0'};
  char desc3[MAX_STRING_LENGTH] = {'\0'}; char desc4[MAX_STRING_LENGTH] = {'\0'};
  long int pet_idnum = 0;
  struct char_data *mob = NULL;

  if (!ch)
    return;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  mysql_ping(conn);

  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in load_pet_data");
    return;
  }
  snprintf(query, sizeof(query), "SELECT vnum, level, hp, max_hp, str, con, dex, ac, intel, wis, cha, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, pet_data_id FROM pet_data WHERE owner_name='%s'", escaped_name);
  free(escaped_name);

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
    if (strlen(row[11]) > 0)
    {
      snprintf(desc1, sizeof(desc1), "%s", row[11]);
      mob->player.name = strdup(desc1);
    }
    if (strlen(row[12]) > 0)
    {
      snprintf(desc2, sizeof(desc2), "%s", row[12]);
      mob->player.short_descr = strdup(desc2);
    }
    if (strlen(row[13]) > 0)
    {
      snprintf(desc3, sizeof(desc3), "%s", row[13]);
      mob->player.long_descr = strdup(desc3);
    }
    if (strlen(row[14]) > 0)
    {
      snprintf(desc4, sizeof(desc4), "%s", row[14]);
      mob->player.description = strdup(desc4);
    }
    if (atol(row[15]) > 0)
    {
      pet_idnum = atol(row[15]);
    }
    if (MOB_FLAGGED(mob, MOB_EIDOLON))
    {
      set_eidolon_descs(ch);
      assign_eidolon_evolutions(ch, mob, false);
      if (GET_EIDOLON_SHORT_DESCRIPTION(ch) && GET_EIDOLON_LONG_DESCRIPTION(ch))
      {snprintf(buf, sizeof(buf), "%s eidolon", GET_EIDOLON_SHORT_DESCRIPTION(ch));
        mob->player.name = strdup(buf);
        mob->player.short_descr = strdup(GET_EIDOLON_SHORT_DESCRIPTION(ch));
        mob->player.long_descr = strdup(GET_EIDOLON_LONG_DESCRIPTION(ch));
        
        snprintf(buf, sizeof(buf), "%s\n", GET_EIDOLON_LONG_DESCRIPTION(ch));
        mob->player.description = strdup(buf);
      }
    }
    GET_REAL_STR(mob) = MIN(100, atoi(row[4]));
    GET_REAL_CON(mob) = MIN(100, atoi(row[5]));
    GET_REAL_DEX(mob) = MIN(100, atoi(row[6]));
    GET_REAL_INT(mob) = MIN(100, atoi(row[8]));
    GET_REAL_WIS(mob) = MIN(100, atoi(row[9]));
    GET_REAL_CHA(mob) = MIN(100, atoi(row[10]));
    GET_REAL_AC(mob) = MIN(100, atoi(row[7]));
    GET_REAL_MAX_HIT(mob) = atoi(row[3]);
    GET_HIT(mob) = atoi(row[2]);
    affect_total(mob);
    load_mtrigger(mob);
    add_follower(mob, ch);
    pet_load_objs(mob, ch, pet_idnum);
    if (!GROUP(mob) && GROUP(ch) && GROUP_LEADER(GROUP(ch)) == ch)
      join_group(mob, GROUP(ch));
    act("$N appears beside you.", true, ch, 0, mob, TO_CHAR);
    act("$N appears beside $n.", true, ch, 0, mob, TO_ROOM);
  }

  mysql_free_result(result);
}

void save_eidolon_descs(struct char_data *ch)
{
  char query[1000];
  char *end2 = NULL;

  if (!GET_EIDOLON_SHORT_DESCRIPTION(ch) || !GET_EIDOLON_LONG_DESCRIPTION(ch))
    return;

  char *escaped_name_del = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name_del) {
    log("SYSERR: Failed to escape player name in save_eidolon_data delete");
    return;
  }
  snprintf(query, sizeof(query), "DELETE FROM player_eidolons WHERE owner='%s'", escaped_name_del);
  free(escaped_name_del);

  if (mysql_query(conn, query))
  {
    log("SYSERR: 1 Unable to DELETE from player_eidolons: %s", mysql_error(conn));
  }

  snprintf(query, sizeof(query), "INSERT INTO player_eidolons (idnum,owner,short_desc,long_desc) VALUES(NULL,");
  end2 = stpcpy(query, "INSERT INTO player_eidolons (idnum,owner,short_desc,long_desc) VALUES(NULL,");

  *end2++ = '\'';
  end2 += mysql_real_escape_string(conn, end2, GET_NAME(ch), strlen(GET_NAME(ch)));
  *end2++ = '\'';
  *end2++ = ',';

  if (valid_pet_name(GET_EIDOLON_SHORT_DESCRIPTION(ch)))
  {
    *end2++ = '\'';
    end2 += mysql_real_escape_string(conn, end2, GET_EIDOLON_SHORT_DESCRIPTION(ch), strlen(GET_EIDOLON_SHORT_DESCRIPTION(ch)));
    *end2++ = '\'';
  }
  else
  {
    *end2++ = '\'';
    *end2++ = '\'';
  }
  *end2++ = ',';
  if (valid_pet_name(GET_EIDOLON_LONG_DESCRIPTION(ch)))
  {
    *end2++ = '\'';
    end2 += mysql_real_escape_string(conn, end2, GET_EIDOLON_LONG_DESCRIPTION(ch), strlen(GET_EIDOLON_LONG_DESCRIPTION(ch)));
    *end2++ = '\'';
  }
  else
  {
    *end2++ = '\'';
    *end2++ = '\'';
  }
  *end2++ = ')';
  *end2++ = '\0';

  if (mysql_query(conn, query))
  {
    log("SYSERR: 1 Unable to INSERT into player_eidolons: %s", mysql_error(conn));
  }

}

void set_eidolon_descs(struct char_data *ch)
{
  if (!ch) return;

  MYSQL_RES *result;
  MYSQL_ROW row;
  char query[200];

  char *escaped_name = mysql_escape_string_alloc(conn, GET_NAME(ch));
  if (!escaped_name) {
    log("SYSERR: Failed to escape player name in load_eidolon_data");
    return;
  }
  snprintf(query, sizeof(query), "SELECT * FROM player_eidolons WHERE owner='%s'", escaped_name);
  free(escaped_name);

  if (mysql_query(conn, query))
  {
    log("SYSERR: 1 Unable to SELECT from player_eidolons: %s", mysql_error(conn));
  }

  if (!(result = mysql_store_result(conn)))
  {
    log("SYSERR: 2 Unable to SELECT from player_eidolons: %s", mysql_error(conn));
    return;
  }

  if ((row = mysql_fetch_row(result)))
  {
    GET_EIDOLON_SHORT_DESCRIPTION(ch) = strdup(row[2]);
    GET_EIDOLON_LONG_DESCRIPTION(ch) = strdup(row[3]);
  }

  mysql_free_result(result);
}