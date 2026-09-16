/* Production-linked tests for paid craft trainers (issue 196,
 * docs/ongoing-projects/craft-trainers.md): craft and harvest ranks, the contract record, the Craft
 * Trainer procedure, and the account-menu entry lock, settlement, and recall. The player-file
 * crafting records for supply contracts and golem projects are tested here too. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/race.h"
#include "../../src/character/talents.h"
#include "../../src/craft/brew.h"
#include "../../src/craft/craft_training.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/database/mysql.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/events/activity_manager.h"
#include "../../src/events/domain_event_types.h"
#include "../../src/events/domain_event_world.h"
#include "../../src/events/domain_events.h"
#include "../../src/events/mud_event.h"
#include "../../src/net/protocol.h"
#include "../../src/spec/spec_registry.h"

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
  if (get_filename(filename, sizeof(filename), CRASH_FILE, files->name))
    unlink(filename);
  unlink("plrfiles/index");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  rmdir("plrobjs/U-Z");
  rmdir("plrobjs");
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
  if (file != NULL)
  {
    fprintf(file,
            "Name: %s\nId  : 4301\nLevl: 7\nTlpt: 2\nAblt:\n%d 1\n%d 4\n0 0\nAbXP:\n%d 6500\n"
            "%d 10\n0 0\n",
            files.name, ABILITY_CRAFT_METALWORKING, ABILITY_HARVEST_GATHERING,
            ABILITY_CRAFT_METALWORKING, ABILITY_HARVEST_GATHERING);
    fclose(file);
  }

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

/** Count the CrTr lines in a saved player file and keep the last one. */
static int craft_training_saved_lines(const char *name, char *last, size_t size)
{
  char filename[MAX_FILEPATH];
  char line[MAX_INPUT_LENGTH];
  FILE *file;
  int count = 0;

  *last = '\0';
  if (!get_filename(filename, sizeof(filename), PLR_FILE, name))
    return -1;
  file = fopen(filename, "r");
  if (file == NULL)
    return -1;
  while (fgets(line, sizeof(line), file) != NULL)
    if (!strncmp(line, "CrTr:", 5))
    {
      snprintf(last, size, "%s", line);
      count++;
    }
  fclose(file);
  return count;
}

void Test_craft_training_contract_survives_save_and_load(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  struct char_data *cleared = new_char();
  char contract_line[MAX_INPUT_LENGTH], expected[MAX_INPUT_LENGTH], after_clear[MAX_INPUT_LENGTH];
  int saved, result, contract_lines, ability, experience, clear_saved, cleared_lines, clear_result;
  int cleared_ability;
  long end;

  craft_player_files_enter(tc, &files, "crrec", 4303);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4303;
  GET_LEVEL(ch) = 10;
  GET_CRAFT(ch).training_ability = ABILITY_HARVEST_FORESTRY;
  GET_CRAFT(ch).training_exp = craft_training_grant(4);
  GET_CRAFT(ch).training_end = (time_t)1800000000L;
  snprintf(expected, sizeof(expected), "CrTr: %d 2500 1800000000\n", ABILITY_HARVEST_FORESTRY);

  saved = save_char_checked(ch, 0);
  contract_lines = craft_training_saved_lines(files.name, contract_line, sizeof(contract_line));
  result = load_char(files.name, loaded);
  ability = GET_CRAFT(loaded).training_ability;
  experience = GET_CRAFT(loaded).training_exp;
  end = (long)GET_CRAFT(loaded).training_end;

  /* Clearing the record removes the line: settlement and recall rely on it. */
  GET_PFILEPOS(loaded) = 0;
  GET_CRAFT(loaded).training_ability = 0;
  clear_saved = save_char_checked(loaded, 0);
  cleared_lines = craft_training_saved_lines(files.name, after_clear, sizeof(after_clear));
  clear_result = load_char(files.name, cleared);
  cleared_ability = GET_CRAFT(cleared).training_ability;

  free_char(ch);
  free_char(loaded);
  free_char(cleared);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 1, contract_lines);
  CuAssertStrEquals(tc, expected, contract_line);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, ABILITY_HARVEST_FORESTRY, ability);
  CuAssertIntEquals(tc, 2500, experience);
  CuAssertTrue(tc, end == 1800000000L);
  CuAssertTrue(tc, clear_saved);
  CuAssertIntEquals(tc, 0, cleared_lines);
  CuAssertIntEquals(tc, 0, clear_result);
  CuAssertIntEquals(tc, 0, cleared_ability);
}

/** Count the lines in a saved player file that start with a tag, keeping the last one. */
static int craft_saved_tag_lines(const char *name, const char *tag, char *last, size_t size)
{
  char filename[MAX_FILEPATH];
  char line[MAX_INPUT_LENGTH];
  FILE *file;
  int count = 0;

  *last = '\0';
  if (!get_filename(filename, sizeof(filename), PLR_FILE, name))
    return -1;
  file = fopen(filename, "r");
  if (file == NULL)
    return -1;
  while (fgets(line, sizeof(line), file) != NULL)
    if (!strncmp(line, tag, strlen(tag)))
    {
      snprintf(last, size, "%s", line);
      count++;
    }
  fclose(file);
  return count;
}

void Test_craft_supply_contract_terms_survive_save_and_load(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  char line[MAX_INPUT_LENGTH], cleared_line[MAX_INPUT_LENGTH], expected[64];
  int saved, lines, result, contract_type, quality_tier, cleared_saved, cleared_lines;

  craft_player_files_enter(tc, &files, "crcon", 4305);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4305;
  GET_LEVEL(ch) = 10;
  GET_CRAFT(ch).supply_contract_type = SUPPLY_CONTRACT_PRESTIGE;
  GET_CRAFT(ch).supply_quality_tier_requirement = QUALITY_TIER_EXCEPTIONAL;
  snprintf(expected, sizeof(expected), "CrCT: %d %d\n", SUPPLY_CONTRACT_PRESTIGE,
           QUALITY_TIER_EXCEPTIONAL);

  saved = save_char_checked(ch, 0);
  lines = craft_saved_tag_lines(files.name, "CrCT:", line, sizeof(line));
  result = load_char(files.name, loaded);
  contract_type = GET_CRAFT(loaded).supply_contract_type;
  quality_tier = GET_CRAFT(loaded).supply_quality_tier_requirement;

  /* An order without contract terms writes no line. */
  GET_CRAFT(ch).supply_contract_type = 0;
  GET_CRAFT(ch).supply_quality_tier_requirement = QUALITY_TIER_STANDARD;
  cleared_saved = save_char_checked(ch, 0);
  cleared_lines = craft_saved_tag_lines(files.name, "CrCT:", cleared_line, sizeof(cleared_line));

  free_char(ch);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 1, lines);
  CuAssertStrEquals(tc, expected, line);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, SUPPLY_CONTRACT_PRESTIGE, contract_type);
  CuAssertIntEquals(tc, QUALITY_TIER_EXCEPTIONAL, quality_tier);
  CuAssertTrue(tc, cleared_saved);
  CuAssertIntEquals(tc, 0, cleared_lines);
}

void Test_craft_golem_project_survives_save_and_load(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  char line[MAX_INPUT_LENGTH], cleared_line[MAX_INPUT_LENGTH], expected[64];
  int saved, lines, result, golem_type, golem_size, wood, cleared_saved, cleared_lines;

  craft_player_files_enter(tc, &files, "crgol", 4306);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4306;
  GET_LEVEL(ch) = 10;
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_WOOD;
  GET_CRAFT(ch).golem_size = GOLEM_SIZE_LARGE;
  GET_CRAFT(ch).golem_materials[0][0] = CRAFT_MAT_MAPLE_WOOD;
  snprintf(expected, sizeof(expected), "CrGo: %d %d %d\n", GOLEM_TYPE_WOOD, GOLEM_SIZE_LARGE,
           CRAFT_MAT_MAPLE_WOOD);

  saved = save_char_checked(ch, 0);
  lines = craft_saved_tag_lines(files.name, "CrGo:", line, sizeof(line));
  result = load_char(files.name, loaded);
  golem_type = GET_CRAFT(loaded).golem_type;
  golem_size = GET_CRAFT(loaded).golem_size;
  wood = GET_CRAFT(loaded).golem_materials[0][0];

  /* No golem project, no line. */
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_NONE;
  cleared_saved = save_char_checked(ch, 0);
  cleared_lines = craft_saved_tag_lines(files.name, "CrGo:", cleared_line, sizeof(cleared_line));

  free_char(ch);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 1, lines);
  CuAssertStrEquals(tc, expected, line);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, GOLEM_TYPE_WOOD, golem_type);
  CuAssertIntEquals(tc, GOLEM_SIZE_LARGE, golem_size);
  CuAssertIntEquals(tc, CRAFT_MAT_MAPLE_WOOD, wood);
  CuAssertTrue(tc, cleared_saved);
  CuAssertIntEquals(tc, 0, cleared_lines);
}

void Test_craft_training_ignores_malformed_contracts(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  char filename[MAX_FILEPATH];
  FILE *file;
  int result, ability, experience;
  time_t end;

  craft_player_files_enter(tc, &files, "crbad", 4304);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  /* A general skill, a slot past the harvest skills, no experience, no end time, a missing
   * field, trailing text, and text. */
  if (file != NULL)
  {
    fprintf(file,
            "Name: %s\nId  : 4304\nLevl: 7\nCrTr: %d 500 1800000000\nCrTr: %d 500 1800000000\n"
            "CrTr: %d 0 1800000000\nCrTr: %d 500 0\nCrTr: %d 500\nCrTr: %d 500 1800000000 x\n"
            "CrTr: alchemy\n",
            files.name, ABILITY_PERCEPTION, END_HARVEST_ABILITIES + 1, ABILITY_CRAFT_ALCHEMY,
            ABILITY_CRAFT_ALCHEMY, ABILITY_CRAFT_ALCHEMY, ABILITY_CRAFT_ALCHEMY);
    fclose(file);
  }

  result = load_char(files.name, loaded);
  ability = GET_CRAFT(loaded).training_ability;
  experience = GET_CRAFT(loaded).training_exp;
  end = GET_CRAFT(loaded).training_end;
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 0, ability);
  CuAssertIntEquals(tc, 0, experience);
  CuAssertTrue(tc, end == 0);
}

/* With every talent at its highest rank, a grant from just below the next threshold raises the
 * skill exactly one rank, for every trainable track and every rank below the ceiling. */
void Test_craft_training_grant_never_crosses_two_ranks(CuTest *tc)
{
  struct craft_actor actor;
  struct char_data *ch = &actor.ch;
  int ability, rank, talent, tracks = 0, failures = 0;

  if (talent_list[TALENT_INSIGHTFUL_GATHERING].name == NULL)
    init_talents();

  for (ability = START_CRAFT_ABILITIES; ability <= END_HARVEST_ABILITIES; ability++)
  {
    if (!craft_training_track_eligible(ability))
      continue;
    tracks++;
    for (rank = 0; rank < CRAFT_TRAINING_RANK_CEILING; rank++)
    {
      craft_actor_init(&actor);
      for (talent = 1; talent < TALENT_MAX; talent++)
        actor.specials.saved.talent_ranks[talent] = (ubyte)talent_max_ranks(talent);
      SET_ABILITY(ch, ability, rank);
      GET_CRAFT_SKILL_EXP(ch, ability) = craft_skill_level_exp(NULL, rank + 1) - 1;
      gain_craft_exp(ch, craft_training_grant(rank), ability, FALSE);
      if (GET_ABILITY(ch, ability) != rank + 1 || GET_TALENT_POINTS(ch) != 1)
        failures++;
    }
  }

  CuAssertIntEquals(tc, 12, tracks);
  CuAssertIntEquals(tc, 0, failures);
}

void Test_craft_training_fee_rises_with_rank(CuTest *tc)
{
  int rank, total = 0;

  CuAssertIntEquals(tc, 500, craft_training_grant(0));
  CuAssertIntEquals(tc, 5000, craft_training_grant(9));
  CuAssertIntEquals(tc, 10000, craft_training_grant(19));
  CuAssertIntEquals(tc, 100, craft_training_fee(0));
  CuAssertIntEquals(tc, 10000, craft_training_fee(9));
  CuAssertIntEquals(tc, 40000, craft_training_fee(19));
  for (rank = 1; rank < CRAFT_TRAINING_RANK_CEILING; rank++)
  {
    /* Both the fee and the gold paid per experience point rise. */
    CuAssertTrue(tc, craft_training_fee(rank) > craft_training_fee(rank - 1));
    CuAssertTrue(tc, (long)craft_training_fee(rank) * craft_training_grant(rank - 1) >
                         (long)craft_training_fee(rank - 1) * craft_training_grant(rank));
  }
  /* Two contracts per rank take a skill from 0 to the ceiling without any bonus. */
  for (rank = 0; rank < CRAFT_TRAINING_RANK_CEILING; rank++)
    total += 2 * craft_training_fee(rank);
  CuAssertIntEquals(tc, 574000, total);
}

void Test_craft_training_status_counts_down_to_finished(CuTest *tc)
{
  struct craft_actor actor;
  struct char_data *ch = &actor.ch;
  char status[64];
  time_t now = (time_t)1800000000L;

  craft_actor_init(&actor);
  CuAssertTrue(tc, !craft_training_status(ch, now, status, sizeof(status)));
  CuAssertStrEquals(tc, "", status);

  GET_CRAFT(ch).training_ability = ABILITY_CRAFT_TAILORING;
  GET_CRAFT(ch).training_exp = 500;
  GET_CRAFT(ch).training_end = now + CRAFT_TRAINING_DURATION;
  CuAssertTrue(tc, craft_training_status(ch, now, status, sizeof(status)));
  CuAssertStrEquals(tc, "training, 24h 0m left", status);
  GET_CRAFT(ch).training_end = now + 13L * 3600 + 19L * 60 + 1;
  craft_training_status(ch, now, status, sizeof(status));
  CuAssertStrEquals(tc, "training, 13h 20m left", status);
  GET_CRAFT(ch).training_end = now + 59;
  craft_training_status(ch, now, status, sizeof(status));
  CuAssertStrEquals(tc, "training, 1m left", status);
  GET_CRAFT(ch).training_end = now;
  craft_training_status(ch, now, status, sizeof(status));
  CuAssertStrEquals(tc, "training finished", status);
}

#define CRAFT_TRAINER_TEST_ROOM 373

static char craft_trainer_room_name[] = "A crafting hall";
static char craft_trainer_room_description[] = "Workbenches line the walls.\r\n";
static char craft_trainer_keywords[] = "artisan master";
static char craft_trainer_short[] = "the master artisan";

/** A lit room in zone 3 holding a Craft Trainer and one connected player, who owns an isolated
 * player file. The real command table dispatches the player's commands. */
struct craft_trainer_fixture
{
  struct craft_player_files files;
  struct room_data room;
  struct zone_data zone;
  struct index_data mobile_index;
  struct char_data trainer;
  struct descriptor_data descriptor;
  struct char_data *player;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  struct char_data *saved_character_list;
  bool created_commands;
};

static void craft_trainer_reset_output(struct descriptor_data *descriptor)
{
  if (descriptor->large_outbuf != NULL)
  {
    free(descriptor->large_outbuf->text);
    free(descriptor->large_outbuf);
    descriptor->large_outbuf = NULL;
  }
  descriptor->small_outbuf[0] = '\0';
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

static void craft_trainer_begin(CuTest *tc, struct craft_trainer_fixture *fixture, const char *tag,
                                long id)
{
  struct char_data *player;
  int i;

  memset(fixture, 0, sizeof(*fixture));
  craft_player_files_enter(tc, &fixture->files, tag, id);
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_character_list = character_list;

  fixture->room.number = CRAFT_TRAINER_TEST_ROOM;
  fixture->room.zone = 0;
  fixture->room.sector_type = SECT_INSIDE;
  fixture->room.name = craft_trainer_room_name;
  fixture->room.description = craft_trainer_room_description;
  fixture->zone.number = 3;
  fixture->zone.bot = 300;
  fixture->zone.top = 399;
  fixture->zone.min_level = -1;
  fixture->zone.max_level = LVL_IMPL;
  fixture->mobile_index.vnum = CRAFT_TRAINER_TEST_ROOM;
  fixture->mobile_index.func = find_spec_func_by_name("Craft Trainer");
  world = &fixture->room;
  top_of_world = 0;
  zone_table = &fixture->zone;
  top_of_zone_table = 0;
  mob_index = &fixture->mobile_index;
  top_of_mobt = 0;

  clear_char(&fixture->trainer);
  fixture->trainer.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture->trainer), MOB_ISNPC);
  fixture->trainer.nr = 0;
  fixture->trainer.player.name = craft_trainer_keywords;
  fixture->trainer.player.short_descr = craft_trainer_short;
  GET_LEVEL(&fixture->trainer) = 30;
  IN_ROOM(&fixture->trainer) = 0;

  player = new_char();
  fixture->player = player;
  player->player.name = strdup(fixture->files.name);
  GET_PFILEPOS(player) = 0;
  GET_IDNUM(player) = id;
  GET_LEVEL(player) = 10;
  GET_CLASS(player) = CLASS_WARRIOR;
  IN_ROOM(player) = 0;
  GET_POS(player) = POS_STANDING;
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
  {
    GET_QUEST(player, i) = NOTHING;
    GET_QUEST_TIME(player, i) = -1;
  }
  SET_ABILITY(player, ABILITY_CRAFT_ALCHEMY, 4);
  GET_CRAFT_SKILL_EXP(player, ABILITY_CRAFT_ALCHEMY) = craft_skill_level_exp(NULL, 4);
  GET_GOLD(player) = 10000;

  fixture->descriptor.character = player;
  player->desc = &fixture->descriptor;
  STATE(&fixture->descriptor) = CON_PLAYING;
  craft_trainer_reset_output(&fixture->descriptor);
  fixture->descriptor.pProtocol = ProtocolCreate();

  fixture->room.people = player;
  player->next_in_room = &fixture->trainer;
  character_list = player;
  player->next = &fixture->trainer;

  if (complete_cmd_info == NULL)
  {
    create_command_list();
    fixture->created_commands = true;
  }
}

/** Run one command and keep what the player saw. */
static void craft_trainer_command(struct craft_trainer_fixture *fixture, const char *command,
                                  char *seen, size_t size)
{
  char line[MAX_INPUT_LENGTH];

  craft_trainer_reset_output(&fixture->descriptor);
  snprintf(line, sizeof(line), "%s", command);
  command_interpreter(fixture->player, line);
  snprintf(seen, size, "%s", fixture->descriptor.output);
}

static int craft_trainer_end(struct craft_trainer_fixture *fixture)
{
  fixture->room.people = NULL;
  fixture->trainer.next_in_room = NULL;
  fixture->trainer.next = NULL;
  if (fixture->player != NULL)
  {
    fixture->player->next_in_room = NULL;
    fixture->player->next = NULL;
    fixture->player->desc = NULL;
    free_char(fixture->player);
  }
  craft_trainer_reset_output(&fixture->descriptor);
  ProtocolDestroy(fixture->descriptor.pProtocol);
  if (fixture->created_commands)
    free_command_list();
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
  character_list = fixture->saved_character_list;
  return craft_player_files_leave(&fixture->files);
}

void Test_craft_trainer_lists_and_quotes_without_changes(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  char listing[MAX_STRING_LENGTH], quote[MAX_STRING_LENGTH], away[MAX_STRING_LENGTH];
  char saved_line[MAX_INPUT_LENGTH];
  int gold, ability, saved_lines, extracted;

  craft_trainer_begin(tc, &fixture, "crlist", 4305);
  craft_trainer_command(&fixture, "apprentice", listing, sizeof(listing));
  craft_trainer_command(&fixture, "appr alch", quote, sizeof(quote));
  gold = GET_GOLD(fixture.player);
  ability = GET_CRAFT(fixture.player).training_ability;
  extracted = PLR_FLAGGED(fixture.player, PLR_NOTDEADYET);
  saved_lines = craft_training_saved_lines(fixture.files.name, saved_line, sizeof(saved_line));
  /* Away from a trainer the command is not available. */
  mob_index[0].func = NULL;
  craft_trainer_command(&fixture, "apprentice", away, sizeof(away));
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertPtrNotNull(tc, strstr(listing, "alchemy"));
  CuAssertPtrNotNull(tc, strstr(listing, "mining"));
  CuAssertPtrEquals(tc, NULL, strstr(listing, "bowmaking"));
  CuAssertPtrNotNull(tc, strstr(listing, "2500"));
  CuAssertPtrNotNull(tc, strstr(quote, "costs 2500 gold coins"));
  CuAssertPtrNotNull(tc, strstr(quote, "followers are dismissed"));
  CuAssertPtrNotNull(tc, strstr(quote, "apprentice alchemy confirm"));
  CuAssertIntEquals(tc, 10000, gold);
  CuAssertIntEquals(tc, 0, ability);
  CuAssertTrue(tc, !extracted);
  CuAssertIntEquals(tc, -1, saved_lines);
  CuAssertPtrNotNull(tc, strstr(away, "Sorry, but you cannot do that here!"));
}

static void craft_trainer_activity_ended(struct char_data *actor,
                                         enum primary_activity_end_reason reason, void *context)
{
  (void)actor;
  (void)reason;
  (void)context;
}

/* The apprentice command conflicts with no activity capability, so only the trainer's own check
 * stands between a running activity and the contract. */
static bool craft_trainer_refuses_during_activity(CuTest *tc, struct craft_trainer_fixture *fixture,
                                                  char *seen, size_t size)
{
  struct primary_activity_definition definition;
  struct domain_event_bus *bus;
  enum domain_event_status status;
  unsigned long saved_pulse = pulse;
  bool started;

  primary_activity_manager_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 4000U;
  event_init();
  bus = domain_event_bus_create(NULL, &status);
  CuAssertPtrNotNull(tc, bus);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, primary_activity_manager_init(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));

  memset(&definition, 0, sizeof(definition));
  definition.type = PRIMARY_ACTIVITY_TEST;
  definition.display_name = "testing an activity";
  definition.capabilities = PRIMARY_ACTIVITY_CAP_MOVEMENT;
  definition.progress_model = PRIMARY_ACTIVITY_PROGRESS_PROGRESSIVE;
  definition.progress_owner = PRIMARY_ACTIVITY_PROGRESS_CHARACTER;
  definition.total_steps = 2U;
  definition.step_interval = 10L;
  definition.movement_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.damage_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.combat_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.target_loss_response = PRIMARY_ACTIVITY_RESPONSE_CANCEL;
  definition.command_response = PRIMARY_ACTIVITY_RESPONSE_REJECT;
  definition.ended = craft_trainer_activity_ended;
  started = primary_activity_start(fixture->player,
                                   domain_event_character_handle(&fixture->trainer), &definition);
  craft_trainer_command(fixture, "apprentice alchemy confirm", seen, size);
  (void)primary_activity_cancel(fixture->player, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
  primary_activity_manager_shutdown();
  domain_event_bus_destroy(bus);
  event_free_all();
  pulse = saved_pulse;
  return started;
}

void Test_craft_trainer_refusals_leave_gold_and_contract_alone(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  char ineligible[MAX_STRING_LENGTH], ceiling[MAX_STRING_LENGTH], poor[MAX_STRING_LENGTH];
  char fighting[MAX_STRING_LENGTH], busy[MAX_STRING_LENGTH], asleep[MAX_STRING_LENGTH];
  int gold, ability, extracted;
  bool activity_started;

  craft_trainer_begin(tc, &fixture, "crnope", 4306);
  craft_trainer_command(&fixture, "apprentice bowmaking confirm", ineligible, sizeof(ineligible));

  SET_ABILITY(fixture.player, ABILITY_HARVEST_MINING, CRAFT_TRAINING_RANK_CEILING);
  craft_trainer_command(&fixture, "apprentice mining confirm", ceiling, sizeof(ceiling));

  GET_GOLD(fixture.player) = 2499;
  craft_trainer_command(&fixture, "apprentice alchemy confirm", poor, sizeof(poor));
  GET_GOLD(fixture.player) = 10000;

  FIGHTING(fixture.player) = &fixture.trainer;
  craft_trainer_command(&fixture, "apprentice alchemy confirm", fighting, sizeof(fighting));
  FIGHTING(fixture.player) = NULL;

  activity_started = craft_trainer_refuses_during_activity(tc, &fixture, busy, sizeof(busy));

  GET_POS(&fixture.trainer) = POS_SLEEPING;
  craft_trainer_command(&fixture, "apprentice alchemy confirm", asleep, sizeof(asleep));

  gold = GET_GOLD(fixture.player);
  ability = GET_CRAFT(fixture.player).training_ability;
  extracted = PLR_FLAGGED(fixture.player, PLR_NOTDEADYET);
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertPtrNotNull(tc, strstr(ineligible, "bowmaking skill cannot be trained here"));
  CuAssertPtrNotNull(tc, strstr(ceiling, "only below rank 20"));
  CuAssertPtrNotNull(tc, strstr(poor, "costs 2500 gold coins, and you carry 2499"));
  CuAssertPtrNotNull(tc, strstr(fighting, "fighting for your life"));
  CuAssertTrue(tc, activity_started);
  CuAssertPtrNotNull(tc, strstr(busy, "cannot leave to train while testing an activity"));
  CuAssertPtrNotNull(tc, strstr(asleep, "is unable to talk to you"));
  CuAssertIntEquals(tc, 10000, gold);
  CuAssertIntEquals(tc, 0, ability);
  CuAssertTrue(tc, !extracted);
}

void Test_craft_trainer_confirm_takes_fee_and_leaves_play(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  struct char_data *loaded = new_char();
  char seen[MAX_STRING_LENGTH], saved_line[MAX_INPUT_LENGTH];
  time_t before, after;
  int gold, marked, state, saved_lines, result, ability, experience, loaded_gold;
  room_vnum load_room;
  time_t end;

  craft_trainer_begin(tc, &fixture, "crgo", 4307);
  before = time(0);
  craft_trainer_command(&fixture, "apprentice alchemy confirm", seen, sizeof(seen));
  after = time(0);
  gold = GET_GOLD(fixture.player);
  marked = PLR_FLAGGED(fixture.player, PLR_NOTDEADYET);
  /* The game loop finishes the extraction later in the same pass. */
  extract_pending_chars();
  state = STATE(&fixture.descriptor);
  saved_lines = craft_training_saved_lines(fixture.files.name, saved_line, sizeof(saved_line));
  result = load_char(fixture.files.name, loaded);
  ability = GET_CRAFT(loaded).training_ability;
  experience = GET_CRAFT(loaded).training_exp;
  end = GET_CRAFT(loaded).training_end;
  load_room = GET_LOADROOM(loaded);
  loaded_gold = GET_GOLD(loaded);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertPtrNotNull(tc, strstr(seen, "leads you away to train"));
  CuAssertIntEquals(tc, 7500, gold);
  CuAssertTrue(tc, marked);
  CuAssertIntEquals(tc, CON_MENU, state);
  CuAssertIntEquals(tc, 1, saved_lines);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, ABILITY_CRAFT_ALCHEMY, ability);
  CuAssertIntEquals(tc, 2500, experience);
  CuAssertTrue(tc,
               end >= before + CRAFT_TRAINING_DURATION && end <= after + CRAFT_TRAINING_DURATION);
  CuAssertIntEquals(tc, CRAFT_TRAINER_TEST_ROOM, (int)load_room);
  CuAssertIntEquals(tc, 7500, loaded_gold);
}

static char craft_chisel_keywords[] = "chisel fine";
static char craft_chisel_short[] = "a fine chisel";
static char craft_chisel_long[] = "A fine chisel lies here.";

/** Connect to the explicitly configured test database, as test_database_persistence.c does. */
static MYSQL *craft_training_open_test_database(void)
{
  const char *host = getenv("LUMINARI_TEST_MYSQL_HOST");
  const char *user = getenv("LUMINARI_TEST_MYSQL_USER");
  const char *password = getenv("LUMINARI_TEST_MYSQL_PASSWORD");
  const char *database = getenv("LUMINARI_TEST_MYSQL_DATABASE");
  const char *port_text = getenv("LUMINARI_TEST_MYSQL_PORT");
  MYSQL *connection;

  if (host == NULL || user == NULL || password == NULL || database == NULL)
    return NULL;
  connection = mysql_init(NULL);
  if (connection == NULL)
    return NULL;
  if (mysql_real_connect(connection, host, user, password, database,
                         port_text != NULL ? (unsigned int)strtoul(port_text, NULL, 10) : 3306U,
                         NULL, 0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }
  return connection;
}

/** Start a contract while carrying a chisel, with rent free or not, and count the chisel's rows
 * in a shadowed object store. */
static void craft_trainer_belongings(CuTest *tc, bool free_rent, int *rows, bool *left_behind)
{
  struct craft_trainer_fixture fixture;
  struct obj_data prototype;
  struct index_data object_index;
  struct obj_data *saved_proto = obj_proto;
  struct index_data *saved_index = obj_index;
  obj_rnum saved_top_of_objt = top_of_objt;
  MYSQL *saved_conn = conn;
  MYSQL *connection;
  MYSQL_RES *result;
  MYSQL_ROW row;
  bool saved_available = mysql_available;
  int saved_free_rent = CONFIG_FREE_RENT;
  char seen[MAX_STRING_LENGTH], query[256];
  bool tables;

  connection = craft_training_open_test_database();
  CuAssertPtrNotNull(tc, connection);
  tables = mysql_query(connection, "CREATE TEMPORARY TABLE player_save_objs (idnum INT "
                                   "AUTO_INCREMENT PRIMARY KEY, name VARCHAR(100), "
                                   "serialized_obj TEXT)") == 0;

  craft_trainer_begin(tc, &fixture, free_rent ? "crfree" : "crrent", 4308);
  CuAssertIntEquals(tc, 0, mkdir("plrobjs", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrobjs/U-Z", 0700));
  clear_object(&prototype);
  prototype.item_number = 0;
  prototype.name = craft_chisel_keywords;
  prototype.short_description = craft_chisel_short;
  prototype.description = craft_chisel_long;
  GET_OBJ_TYPE(&prototype) = ITEM_OTHER;
  memset(&object_index, 0, sizeof(object_index));
  object_index.vnum = CRAFT_TRAINER_TEST_ROOM;
  obj_proto = &prototype;
  obj_index = &object_index;
  top_of_objt = 0;
  obj_to_char(read_object(0, REAL), fixture.player);

  conn = connection;
  mysql_available = true;
  CONFIG_FREE_RENT = free_rent;
  craft_trainer_command(&fixture, "apprentice alchemy confirm", seen, sizeof(seen));
  extract_pending_chars();
  CONFIG_FREE_RENT = saved_free_rent;
  mysql_available = saved_available;
  conn = saved_conn;

  *rows = -1;
  snprintf(query, sizeof(query), "SELECT COUNT(*) FROM player_save_objs WHERE name = '%s'",
           fixture.files.name);
  result = tables && mysql_query(connection, query) == 0 ? mysql_store_result(connection) : NULL;
  if (result != NULL)
  {
    row = mysql_fetch_row(result);
    if (row != NULL && row[0] != NULL)
      *rows = (int)strtol(row[0], NULL, 10);
    mysql_free_result(result);
  }
  *left_behind = fixture.room.contents != NULL || fixture.player->carrying != NULL;
  while (fixture.room.contents != NULL)
    extract_obj(fixture.room.contents);
  while (fixture.player->carrying != NULL)
    extract_obj(fixture.player->carrying);
  mysql_close(connection);
  obj_proto = saved_proto;
  obj_index = saved_index;
  top_of_objt = saved_top_of_objt;
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));
}

/* With free rent the quit saves belongings and without it the trainer does: either way they
 * reach the object store exactly once and nothing is left on the floor. */
void Test_craft_trainer_saves_belongings_once(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  int free_rows, rent_rows;
  bool free_left, rent_left;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;

  craft_trainer_belongings(tc, TRUE, &free_rows, &free_left);
  craft_trainer_belongings(tc, FALSE, &rent_rows, &rent_left);

  CuAssertIntEquals(tc, 1, free_rows);
  CuAssertTrue(tc, !free_left);
  CuAssertIntEquals(tc, 1, rent_rows);
  CuAssertTrue(tc, !rent_left);
}

void Test_craft_training_main_menu_refuses_entry_while_away(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  char seen[MAX_STRING_LENGTH], line[] = "1";
  struct char_data *listed;
  int state;
  bool in_play = false;

  craft_trainer_begin(tc, &fixture, "crlock", 4309);
  craft_trainer_command(&fixture, "apprentice alchemy confirm", seen, sizeof(seen));
  extract_pending_chars();
  /* The descriptor now sits at the main menu with the character that just left. */
  craft_trainer_reset_output(&fixture.descriptor);
  nanny(&fixture.descriptor, line);
  snprintf(seen, sizeof(seen), "%s", fixture.descriptor.output);
  state = STATE(&fixture.descriptor);
  for (listed = character_list; listed != NULL; listed = listed->next)
    in_play = in_play || listed == fixture.player;
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertIntEquals(tc, CON_MENU, state);
  CuAssertTrue(tc, !in_play);
  CuAssertPtrNotNull(tc, strstr(seen, "cannot enter the game from here"));
  CuAssertPtrNotNull(tc, strstr(seen, "training, 24h 0m left"));
}

void Test_craft_training_copyover_drops_a_character_leaving_to_train(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  char seen[MAX_STRING_LENGTH];
  bool before, leaving;

  craft_trainer_begin(tc, &fixture, "crcopy", 4310);
  before = copyover_restores_descriptor(&fixture.descriptor);
  craft_trainer_command(&fixture, "apprentice alchemy confirm", seen, sizeof(seen));
  /* A copyover later in the same pass runs before the pending extraction. */
  leaving = copyover_restores_descriptor(&fixture.descriptor);
  extract_pending_chars();
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertTrue(tc, before);
  CuAssertTrue(tc, !leaving);
}

/** A connection at the account menu whose account lists one isolated player file. */
struct craft_account_fixture
{
  struct craft_player_files files;
  struct descriptor_data descriptor;
  struct account_data account;
  long id;
};

static void craft_account_begin(CuTest *tc, struct craft_account_fixture *fixture, const char *tag,
                                long id)
{
  memset(fixture, 0, sizeof(*fixture));
  craft_player_files_enter(tc, &fixture->files, tag, id);
  fixture->id = id;
  fixture->account.character_names[0] = fixture->files.name;
  fixture->descriptor.account = &fixture->account;
  STATE(&fixture->descriptor) = CON_ACCOUNT_MENU;
  craft_trainer_reset_output(&fixture->descriptor);
  fixture->descriptor.pProtocol = ProtocolCreate();
}

/** Save the account's character at alchemy rank 4, one point short of rank 5, with 7,500 gold,
 * the insightful alchemy talent at rank insight, and a rank-4 contract ending at end. */
static bool craft_account_save_contract(struct craft_account_fixture *fixture, time_t end,
                                        int insight)
{
  struct char_data *ch = new_char();
  bool saved;

  ch->player.name = strdup(fixture->files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = fixture->id;
  GET_LEVEL(ch) = 10;
  GET_GOLD(ch) = 7500;
  SET_ABILITY(ch, ABILITY_CRAFT_ALCHEMY, 4);
  GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_ALCHEMY) = craft_skill_level_exp(NULL, 5) - 1;
  ch->player_specials->saved.talent_ranks[TALENT_INSIGHTFUL_ALCHEMY] = (ubyte)insight;
  GET_CRAFT(ch).training_ability = ABILITY_CRAFT_ALCHEMY;
  GET_CRAFT(ch).training_exp = craft_training_grant(4);
  GET_CRAFT(ch).training_end = end;
  saved = save_char_checked(ch, 0);
  free_char(ch);
  return saved;
}

/** Type one line at the connection's current menu and keep what it printed. */
static void craft_account_input(struct craft_account_fixture *fixture, const char *input,
                                char *seen, size_t size)
{
  char line[MAX_INPUT_LENGTH];

  craft_trainer_reset_output(&fixture->descriptor);
  snprintf(line, sizeof(line), "%s", input);
  nanny(&fixture->descriptor, line);
  snprintf(seen, size, "%s", fixture->descriptor.output);
}

/** What the account's player file holds now. */
struct craft_account_record
{
  int loaded;
  int ability;
  int experience;
  int rank;
  int talent_points;
  int gold;
  int contract_lines;
};

static void craft_account_read(struct craft_account_fixture *fixture,
                               struct craft_account_record *record)
{
  struct char_data *ch = new_char();
  char line[MAX_INPUT_LENGTH];

  record->loaded = load_char(fixture->files.name, ch);
  record->ability = GET_CRAFT(ch).training_ability;
  record->experience = GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_ALCHEMY);
  record->rank = GET_ABILITY(ch, ABILITY_CRAFT_ALCHEMY);
  record->talent_points = GET_TALENT_POINTS(ch);
  record->gold = GET_GOLD(ch);
  record->contract_lines = craft_training_saved_lines(fixture->files.name, line, sizeof(line));
  free_char(ch);
}

static int craft_account_end(struct craft_account_fixture *fixture)
{
  if (fixture->descriptor.character != NULL)
  {
    fixture->descriptor.character->desc = NULL;
    free_char(fixture->descriptor.character);
    fixture->descriptor.character = NULL;
  }
  craft_trainer_reset_output(&fixture->descriptor);
  ProtocolDestroy(fixture->descriptor.pProtocol);
  return craft_player_files_leave(&fixture->files);
}

/** The whole player file, to prove a refusal wrote nothing. */
static bool craft_account_file_text(struct craft_account_fixture *fixture, char *text, size_t size)
{
  char filename[MAX_FILEPATH];
  FILE *file;
  size_t length;

  if (!get_filename(filename, sizeof(filename), PLR_FILE, fixture->files.name))
    return false;
  file = fopen(filename, "r");
  if (file == NULL)
    return false;
  length = fread(text, 1, size - 1, file);
  text[length] = '\0';
  fclose(file);
  return length > 0;
}

void Test_craft_training_selection_waits_for_the_contract_to_end(CuTest *tc)
{
  struct craft_account_fixture fixture;
  struct craft_account_record record;
  char seen[MAX_STRING_LENGTH], before[MAX_STRING_LENGTH * 4], after[MAX_STRING_LENGTH * 4];
  bool saved, read_before, read_after;
  int state;

  craft_account_begin(tc, &fixture, "crwait", 4311);
  saved = craft_account_save_contract(&fixture, time(0) + 3600, 0);
  read_before = craft_account_file_text(&fixture, before, sizeof(before));
  craft_account_input(&fixture, "1", seen, sizeof(seen));
  state = STATE(&fixture.descriptor);
  read_after = craft_account_file_text(&fixture, after, sizeof(after));
  craft_account_read(&fixture, &record);
  CuAssertIntEquals(tc, 0, craft_account_end(&fixture));

  CuAssertTrue(tc, saved && read_before && read_after);
  CuAssertIntEquals(tc, CON_ACCOUNT_MENU, state);
  CuAssertPtrNotNull(tc, strstr(seen, "is away learning alchemy (training, 1h 0m left)"));
  CuAssertPtrNotNull(tc, strstr(seen, "recall 1 confirm"));
  CuAssertStrEquals(tc, before, after);
  CuAssertIntEquals(tc, ABILITY_CRAFT_ALCHEMY, record.ability);
  CuAssertIntEquals(tc, craft_skill_level_exp(NULL, 5) - 1, record.experience);
}

void Test_craft_training_selection_grants_a_finished_contract_once(CuTest *tc)
{
  struct craft_account_fixture fixture;
  struct craft_account_record first, second;
  char seen[MAX_STRING_LENGTH], again[MAX_STRING_LENGTH];
  int first_state, second_state;
  bool saved;

  craft_account_begin(tc, &fixture, "crdone", 4312);
  /* Insight at rank 2 adds 10%: 2,500 + 250 from 14,999 crosses into rank 5 only. */
  saved = craft_account_save_contract(&fixture, time(0) - 1, 2);
  craft_account_input(&fixture, "1", seen, sizeof(seen));
  first_state = STATE(&fixture.descriptor);
  craft_account_read(&fixture, &first);
  /* Back at the account menu, the next selection loads the file again and finds no contract. */
  STATE(&fixture.descriptor) = CON_ACCOUNT_MENU;
  craft_account_input(&fixture, "1", again, sizeof(again));
  second_state = STATE(&fixture.descriptor);
  craft_account_read(&fixture, &second);
  CuAssertIntEquals(tc, 0, craft_account_end(&fixture));

  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, CON_RMOTD, first_state);
  CuAssertPtrNotNull(tc, strstr(seen, "returns from training in alchemy"));
  CuAssertIntEquals(tc, 0, first.loaded);
  CuAssertIntEquals(tc, 0, first.ability);
  CuAssertIntEquals(tc, 0, first.contract_lines);
  CuAssertIntEquals(tc, 14999 + 2750, first.experience);
  CuAssertIntEquals(tc, 5, first.rank);
  CuAssertIntEquals(tc, 1, first.talent_points);
  CuAssertIntEquals(tc, CON_RMOTD, second_state);
  CuAssertPtrEquals(tc, NULL, strstr(again, "returns from training"));
  CuAssertIntEquals(tc, first.experience, second.experience);
  CuAssertIntEquals(tc, 5, second.rank);
  CuAssertIntEquals(tc, 1, second.talent_points);
}

void Test_craft_training_recall_ends_contract_without_refund(CuTest *tc)
{
  struct craft_account_fixture fixture;
  struct craft_account_record quoted, recalled;
  char quote[MAX_STRING_LENGTH], done[MAX_STRING_LENGTH], enter[MAX_STRING_LENGTH];
  int state;
  bool saved;

  craft_account_begin(tc, &fixture, "crcall", 4313);
  saved = craft_account_save_contract(&fixture, time(0) + 3600, 0);
  craft_account_input(&fixture, "recall 1", quote, sizeof(quote));
  craft_account_read(&fixture, &quoted);
  craft_account_input(&fixture, "recall 1 confirm", done, sizeof(done));
  craft_account_read(&fixture, &recalled);
  craft_account_input(&fixture, "1", enter, sizeof(enter));
  state = STATE(&fixture.descriptor);
  CuAssertIntEquals(tc, 0, craft_account_end(&fixture));

  CuAssertTrue(tc, saved);
  CuAssertPtrNotNull(tc, strstr(quote, "forfeits the fee and the 2500 experience"));
  CuAssertIntEquals(tc, ABILITY_CRAFT_ALCHEMY, quoted.ability);
  CuAssertPtrNotNull(tc, strstr(done, "returns from training early"));
  CuAssertIntEquals(tc, 0, recalled.ability);
  CuAssertIntEquals(tc, 0, recalled.contract_lines);
  CuAssertIntEquals(tc, 7500, recalled.gold);
  CuAssertIntEquals(tc, craft_skill_level_exp(NULL, 5) - 1, recalled.experience);
  CuAssertIntEquals(tc, CON_RMOTD, state);
}

/* show_account_menu() looks each character up in player_data, so the row needs the database. */
void Test_craft_training_account_menu_shows_time_left(CuTest *tc)
{
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  struct craft_account_fixture fixture;
  MYSQL *saved_conn = conn;
  MYSQL *connection;
  bool saved_available = mysql_available;
  char running[MAX_STRING_LENGTH * 2], finished[MAX_STRING_LENGTH * 2], query[256];
  bool prepared, saved_running, saved_finished;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;
  if (race_list[RACE_HUMAN].name == NULL)
    assign_races();
  connection = craft_training_open_test_database();
  CuAssertPtrNotNull(tc, connection);

  craft_account_begin(tc, &fixture, "crrow", 4314);
  snprintf(query, sizeof(query), "INSERT INTO player_data (name) VALUES ('%s')",
           fixture.files.name);
  prepared =
      mysql_query(connection, "CREATE TEMPORARY TABLE player_data (name VARCHAR(100))") == 0 &&
      mysql_query(connection, query) == 0;
  conn = connection;
  mysql_available = true;
  /* 13h 19m 30s rounds up to 13h 20m for the next half minute. */
  saved_running = craft_account_save_contract(&fixture, time(0) + 13L * 3600 + 19L * 60 + 30, 0);
  craft_trainer_reset_output(&fixture.descriptor);
  show_account_menu(&fixture.descriptor);
  snprintf(running, sizeof(running), "%s", fixture.descriptor.output);
  saved_finished = craft_account_save_contract(&fixture, time(0) - 60, 0);
  craft_trainer_reset_output(&fixture.descriptor);
  show_account_menu(&fixture.descriptor);
  snprintf(finished, sizeof(finished), "%s", fixture.descriptor.output);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);
  CuAssertIntEquals(tc, 0, craft_account_end(&fixture));

  CuAssertTrue(tc, prepared && saved_running && saved_finished);
  CuAssertPtrNotNull(tc, strstr(running, fixture.files.name));
  CuAssertPtrNotNull(tc, strstr(running, "training, 13h 20m left"));
  CuAssertPtrNotNull(tc, strstr(finished, "training finished"));
}
