/* Production-linked regressions for craft and harvest ranks, the experience path that paid craft
 * trainers share (issue 196, docs/ongoing-projects/craft-trainers.md). */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/core/db.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/race.h"
#include "../../src/character/talents.h"
#include "../../src/craft/brew.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/events/mud_event.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/** A player character that is never saved. */
struct craft_actor
{
  struct char_data ch;
  struct player_special_data specials;
};

static void craft_actor_init(struct craft_actor *actor)
{
  memset(actor, 0, sizeof(*actor));
  actor->ch.player_specials = &actor->specials;
  GET_PFILEPOS(&actor->ch) = -1;
  IN_ROOM(&actor->ch) = NOWHERE;
  GET_LEVEL(&actor->ch) = 10;
}

/** An isolated player directory and index, so real saves never touch lib/plrfiles. */
struct craft_player_files
{
  char temporary_directory[64];
  char directory[PATH_MAX];
  char name[32];
  struct player_index_element index[1];
  struct player_index_element *saved_table;
  int saved_top;
};

/* Player names start with Z so every file lands in plrfiles/U-Z. */
static void craft_player_files_enter(CuTest *tc, struct craft_player_files *files, const char *tag,
                                     long id)
{
  memset(files, 0, sizeof(*files));
  snprintf(files->temporary_directory, sizeof(files->temporary_directory),
           "/tmp/luminari-craft-training-XXXXXX");
  snprintf(files->name, sizeof(files->name), "Zz%s%ld", tag, (long)getpid());
  files->index[0].name = files->name;
  files->index[0].id = id;
  files->index[0].level = 10;
  files->saved_table = player_table;
  files->saved_top = top_of_p_table;
  player_table = files->index;
  top_of_p_table = 0;
  CuAssertPtrNotNull(tc, getcwd(files->directory, sizeof(files->directory)));
  CuAssertPtrNotNull(tc, mkdtemp(files->temporary_directory));
  CuAssertIntEquals(tc, 0, chdir(files->temporary_directory));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/U-Z", 0700));
}

/** Remove the player file and index, restore the player table, and return to the original
 * directory. */
static int craft_player_files_leave(struct craft_player_files *files)
{
  char filename[MAX_FILEPATH];
  int result;

  if (get_filename(filename, sizeof(filename), PLR_FILE, files->name))
    unlink(filename);
  unlink("plrfiles/index");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  player_table = files->saved_table;
  top_of_p_table = files->saved_top;
  result = chdir(files->directory);
  if (result == 0)
    rmdir(files->temporary_directory);
  return result;
}

void Test_craft_rank_for_exp_counts_cumulative_thresholds(CuTest *tc)
{
  CuAssertIntEquals(tc, 0, craft_skill_rank_for_exp(NULL, 999));
  CuAssertIntEquals(tc, 1, craft_skill_rank_for_exp(NULL, 1000));
  CuAssertIntEquals(tc, 1, craft_skill_rank_for_exp(NULL, 2999));
  CuAssertIntEquals(tc, 2, craft_skill_rank_for_exp(NULL, 3000));
  CuAssertIntEquals(tc, UCHAR_MAX, craft_skill_rank_for_exp(NULL, INT_MAX));
}

void Test_craft_gain_crossing_three_thresholds_raises_every_rank(CuTest *tc)
{
  struct craft_actor actor;
  struct char_data *ch = &actor.ch;

  craft_actor_init(&actor);
  gain_craft_exp(ch, craft_skill_level_exp(NULL, 3), ABILITY_CRAFT_WOODWORKING, FALSE);

  CuAssertIntEquals(tc, 3, GET_ABILITY(ch, ABILITY_CRAFT_WOODWORKING));
  CuAssertIntEquals(tc, 3, GET_TALENT_POINTS(ch));
  CuAssertIntEquals(tc, 6000, GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_WOODWORKING));
}

/** Find a seed whose next d20 for ch is, or is not, a natural 1. event_brewing() rolls first. */
static unsigned long craft_brewing_seed(struct char_data *ch, bool critical)
{
  unsigned long seed;

  for (seed = 1; seed < 10000; seed++)
  {
    circle_srandom(seed);
    if ((d20(ch) == 1) == critical)
      return seed;
  }
  return 0;
}

/** Run one brewing completion that cannot meet its DC, from one point below alchemy rank 3 with
 * the insightful alchemy talent at +25%. */
static void craft_brew_failure(struct craft_actor *actor, bool critical, unsigned long *seed)
{
  struct char_data *ch = &actor->ch;
  struct mud_event_data event;
  char payload[128];
  int length, i;

  craft_actor_init(actor);
  SET_ABILITY(ch, ABILITY_CRAFT_ALCHEMY, 2);
  GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_ALCHEMY) = craft_skill_level_exp(NULL, 3) - 1;
  actor->specials.saved.talent_ranks[TALENT_INSIGHTFUL_ALCHEMY] = 5;

  /* spell1,spell2,spell3,num_spells,highest_circle,brewing_skill,dc,motes...,gold,multiplier */
  length = snprintf(payload, sizeof(payload), "1,-1,-1,1,20,0,1000");
  for (i = 0; i < NUM_CRAFT_MOTES; i++)
    length += snprintf(payload + length, sizeof(payload) - (size_t)length, ",0");
  snprintf(payload + length, sizeof(payload) - (size_t)length, ",0,1");

  memset(&event, 0, sizeof(event));
  event.pStruct = ch;
  event.sVariables = payload;
  *seed = craft_brewing_seed(ch, critical);
  circle_srandom(*seed);
  event_brewing(&event);
  circle_srandom((unsigned long)time(NULL));
}

void Test_craft_brewing_failures_raise_alchemy_with_insight(CuTest *tc)
{
  struct craft_actor critical, regular;
  struct char_data *critical_ch = &critical.ch, *regular_ch = &regular.ch;
  unsigned long critical_seed, regular_seed;
  int start = craft_skill_level_exp(NULL, 3) - 1;

  craft_brew_failure(&critical, TRUE, &critical_seed);
  craft_brew_failure(&regular, FALSE, &regular_seed);

  CuAssertTrue(tc, critical_seed != 0);
  CuAssertTrue(tc, regular_seed != 0);
  /* A natural 1 grants 20 x 5 = 100, plus 25. */
  CuAssertIntEquals(tc, start + 125, GET_CRAFT_SKILL_EXP(critical_ch, ABILITY_CRAFT_ALCHEMY));
  CuAssertIntEquals(tc, 3, GET_ABILITY(critical_ch, ABILITY_CRAFT_ALCHEMY));
  CuAssertIntEquals(tc, 1, GET_TALENT_POINTS(critical_ch));
  /* Any other miss grants 20 x 3 / 4 = 15, plus 3. */
  CuAssertIntEquals(tc, start + 18, GET_CRAFT_SKILL_EXP(regular_ch, ABILITY_CRAFT_ALCHEMY));
  CuAssertIntEquals(tc, 3, GET_ABILITY(regular_ch, ABILITY_CRAFT_ALCHEMY));
  CuAssertIntEquals(tc, 1, GET_TALENT_POINTS(regular_ch));
}

void Test_craft_respec_keeps_craft_and_harvest_ranks(CuTest *tc)
{
  struct char_data *ch = new_char();
  int perception, weaponsmithing, weaponsmithing_exp, mining, mining_exp, talent_points;

  if (class_list[CLASS_WARRIOR].name == NULL)
    load_class_list();
  if (feat_list[FEAT_ANIMATE_DEAD].name == NULL)
    assign_feats();
  if (race_list[RACE_HUMAN].name == NULL)
    assign_races();

  ch->player.name = strdup("Zzcraftrespec");
  GET_PFILEPOS(ch) = -1;
  IN_ROOM(ch) = NOWHERE;
  GET_CLASS(ch) = CLASS_WARRIOR;
  GET_REAL_RACE(ch) = RACE_HUMAN;
  GET_LEVEL(ch) = 10;
  CLASS_LEVEL(ch, CLASS_WARRIOR) = 10;
  SET_ABILITY(ch, ABILITY_PERCEPTION, 5);
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 4);
  GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_WEAPONSMITHING) = 12000;
  SET_ABILITY(ch, ABILITY_HARVEST_MINING, 2);
  GET_CRAFT_SKILL_EXP(ch, ABILITY_HARVEST_MINING) = 3500;
  GET_TALENT_POINTS(ch) = 6;

  do_start(ch);

  perception = GET_ABILITY(ch, ABILITY_PERCEPTION);
  weaponsmithing = GET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING);
  weaponsmithing_exp = GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_WEAPONSMITHING);
  mining = GET_ABILITY(ch, ABILITY_HARVEST_MINING);
  mining_exp = GET_CRAFT_SKILL_EXP(ch, ABILITY_HARVEST_MINING);
  talent_points = GET_TALENT_POINTS(ch);
  free_char(ch);

  CuAssertIntEquals(tc, 0, perception);
  CuAssertIntEquals(tc, 4, weaponsmithing);
  CuAssertIntEquals(tc, 12000, weaponsmithing_exp);
  CuAssertIntEquals(tc, 2, mining);
  CuAssertIntEquals(tc, 3500, mining_exp);
  CuAssertIntEquals(tc, 6, talent_points);
}

void Test_craft_load_restores_rank_its_experience_earned(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  char filename[MAX_FILEPATH];
  FILE *file;
  int result, rank, gathering, talent_points;

  craft_player_files_enter(tc, &files, "crrank", 4301);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  /* Metalworking lags two ranks behind 6,500 experience; gathering is ahead of its experience. */
  fprintf(file,
          "Name: %s\nId  : 4301\nLevl: 7\nTlpt: 2\nAblt:\n%d 1\n%d 4\n0 0\nAbXP:\n%d 6500\n"
          "%d 10\n0 0\n",
          files.name, ABILITY_CRAFT_METALWORKING, ABILITY_HARVEST_GATHERING,
          ABILITY_CRAFT_METALWORKING, ABILITY_HARVEST_GATHERING);
  fclose(file);

  result = load_char(files.name, loaded);
  rank = GET_ABILITY(loaded, ABILITY_CRAFT_METALWORKING);
  gathering = GET_ABILITY(loaded, ABILITY_HARVEST_GATHERING);
  talent_points = GET_TALENT_POINTS(loaded);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 3, rank);
  CuAssertIntEquals(tc, 4, gathering);
  CuAssertIntEquals(tc, 2, talent_points);
}

/* Harvest talents have ids above 63, which the rank storage once could not hold. */
void Test_craft_harvest_talent_ranks_apply_and_persist(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  int learned, gained, saved, result, rank, mote_rank;

  if (talent_list[TALENT_INSIGHTFUL_GATHERING].name == NULL)
    init_talents();

  craft_player_files_enter(tc, &files, "crtal", 4302);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4302;
  GET_LEVEL(ch) = 10;
  GET_GOLD(ch) = 100000;
  GET_TALENT_POINTS(ch) = 20;
  learned = learn_talent(ch, TALENT_INSIGHTFUL_GATHERING);
  ch->player_specials->saved.talent_ranks[TALENT_WATER_MOTE_SYNERGY] = 1;
  gain_craft_exp(ch, 100, ABILITY_HARVEST_GATHERING, FALSE);
  gained = GET_CRAFT_SKILL_EXP(ch, ABILITY_HARVEST_GATHERING);
  saved = save_char_checked(ch, 0);
  result = load_char(files.name, loaded);
  rank = get_talent_rank(loaded, TALENT_INSIGHTFUL_GATHERING);
  mote_rank = get_talent_rank(loaded, TALENT_WATER_MOTE_SYNERGY);
  free_char(ch);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, learned);
  CuAssertIntEquals(tc, 105, gained);
  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 1, rank);
  CuAssertIntEquals(tc, 1, mote_rank);
}
