/* Production-linked tests for paid craft trainers (issue 196,
 * docs/world_game-data/CRAFTING_SYSTEM_NOTES.md): craft and harvest ranks, the contract record,
 * the Craft Trainer procedure, and the account-menu entry lock, settlement, and recall. The
 * player-file crafting records for supply contracts and golem projects are tested here too. */

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
#include "../../src/craft/craft.h"
#include "../../src/craft/craft_training.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/wilderness/resource_system.h"
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
#include <dirent.h>
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

  craft_actor_init(actor);
  SET_ABILITY(ch, ABILITY_CRAFT_ALCHEMY, 2);
  GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_ALCHEMY) = craft_skill_level_exp(NULL, 3) - 1;
  actor->specials.saved.talent_ranks[TALENT_INSIGHTFUL_ALCHEMY] = 5;

  /* One spell of circle 20 at skill 0 against DC 1000: the roll cannot succeed. */
  *seed = craft_brewing_seed(ch, critical);
  circle_srandom(*seed);
  test_brew_resolve(ch, 1, 20, 0, 1000, false, 0, 0, 0);
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

/* A player file that ends inside the class feat point list made the loader spin forever,
 * because get_line() leaves its last line in place at the end of the file; an out-of-range
 * class index was also written past the array. */
void Test_load_char_stops_at_a_truncated_class_feat_list(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  char filename[MAX_FILEPATH];
  FILE *file;
  int result, class_feats, epic_feats;

  craft_player_files_enter(tc, &files, "crcfp", 4303);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  if (file != NULL)
  {
    fprintf(file, "Name: %s\nId  : 4303\nLevl: 7\nEcfp:\n%d 2\n%d 9\n0\nCfpt:\n%d 3\n", files.name,
            CLASS_WIZARD, NUM_CLASSES + 5, CLASS_WIZARD);
    fclose(file);
  }

  result = load_char(files.name, loaded);
  class_feats = (int)GET_CLASS_FEATS(loaded, CLASS_WIZARD);
  epic_feats = (int)GET_EPIC_CLASS_FEATS(loaded, CLASS_WIZARD);
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 3, class_feats);
  CuAssertIntEquals(tc, 2, epic_feats);
}

/* A saved device line longer than its field is cut to fit; get_line() used to write it straight
 * into the field and on into the fields after it. */
void Test_load_char_cuts_device_text_to_its_fields(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  struct player_invention *device;
  char filename[MAX_FILEPATH];
  char keywords[101];
  char long_description[301];
  char short_description[MAX_INVENTION_SHORTDESC];
  FILE *file;
  size_t keywords_length, long_length;
  int result, num_spells, reliability, first_effect;

  memset(keywords, 'k', sizeof(keywords) - 1);
  keywords[sizeof(keywords) - 1] = '\0';
  memset(long_description, 'l', sizeof(long_description) - 1);
  long_description[sizeof(long_description) - 1] = '\0';
  craft_player_files_enter(tc, &files, "crdvt", 4304);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  if (file != NULL)
  {
    fprintf(file,
            "Name: %s\nId  : 4304\nLevl: 7\nDvis:\n1\n0\n%s\na brass device\n%s\n2 30 90 1 0\n"
            "7\n0\n0\n0\n~\n",
            files.name, keywords, long_description);
    fclose(file);
  }

  result = load_char(files.name, loaded);
  device = &loaded->player_specials->saved.inventions[0];
  keywords_length = strlen(device->keywords);
  long_length = strnlen(device->long_description, sizeof(device->long_description));
  strlcpy(short_description, device->short_description, sizeof(short_description));
  num_spells = device->num_spells;
  reliability = device->reliability;
  first_effect = device->spell_effects[0];
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, MAX_INVENTION_KEYWORDS - 1, (int)keywords_length);
  CuAssertStrEquals(tc, "a brass device", short_description);
  CuAssertIntEquals(tc, MAX_INVENTION_LONGDESC - 1, (int)long_length);
  CuAssertIntEquals(tc, 2, num_spells);
  CuAssertIntEquals(tc, 90, reliability);
  CuAssertIntEquals(tc, 7, first_effect);
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
  CuAssertIntEquals(tc, 400, craft_training_fee(0));
  CuAssertIntEquals(tc, 40000, craft_training_fee(9));
  CuAssertIntEquals(tc, 160000, craft_training_fee(19));
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
  CuAssertIntEquals(tc, 2296000, total);
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
  CuAssertStrEquals(tc, "training, 6h 0m left", status);
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
  GET_GOLD(player) = 40000;

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
  CuAssertPtrNotNull(tc, strstr(listing, "10000"));
  CuAssertPtrNotNull(tc, strstr(quote, "costs 10000 gold coins"));
  CuAssertPtrNotNull(tc, strstr(quote, "followers are dismissed"));
  CuAssertPtrNotNull(tc, strstr(quote, "apprentice alchemy confirm"));
  CuAssertIntEquals(tc, 40000, gold);
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

  GET_GOLD(fixture.player) = 9999;
  craft_trainer_command(&fixture, "apprentice alchemy confirm", poor, sizeof(poor));
  GET_GOLD(fixture.player) = 40000;

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
  CuAssertPtrNotNull(tc, strstr(poor, "costs 10000 gold coins, and you carry 9999"));
  CuAssertPtrNotNull(tc, strstr(fighting, "fighting for your life"));
  CuAssertTrue(tc, activity_started);
  CuAssertPtrNotNull(tc, strstr(busy, "cannot leave to train while testing an activity"));
  CuAssertPtrNotNull(tc, strstr(asleep, "is unable to talk to you"));
  CuAssertIntEquals(tc, 40000, gold);
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
  CuAssertIntEquals(tc, 30000, gold);
  CuAssertTrue(tc, marked);
  CuAssertIntEquals(tc, CON_MENU, state);
  CuAssertIntEquals(tc, 1, saved_lines);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, ABILITY_CRAFT_ALCHEMY, ability);
  CuAssertIntEquals(tc, 2500, experience);
  CuAssertTrue(tc,
               end >= before + CRAFT_TRAINING_DURATION && end <= after + CRAFT_TRAINING_DURATION);
  CuAssertIntEquals(tc, CRAFT_TRAINER_TEST_ROOM, (int)load_room);
  CuAssertIntEquals(tc, 30000, loaded_gold);
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
  CuAssertPtrNotNull(tc, strstr(seen, "training, 6h 0m left"));
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
  GET_GOLD(ch) = 30000;
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
  /* NOLINTNEXTLINE(clang-analyzer-security.ArrayBound) -- fread() returns at most its count */
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
  CuAssertIntEquals(tc, 30000, recalled.gold);
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

/* ---- Legacy skill conversion (crafting consolidation, Phase 2) ---- */

void Test_craft_legacy_conversion_boundaries(CuTest *tc)
{
  struct craft_actor actor;
  int second = -9;

  craft_actor_init(&actor);
  CuAssertIntEquals(tc, 0, craft_legacy_rank_for_skill(0));
  CuAssertIntEquals(tc, 0, craft_legacy_rank_for_skill(1));
  CuAssertIntEquals(tc, 0, craft_legacy_rank_for_skill(4));
  CuAssertIntEquals(tc, 1, craft_legacy_rank_for_skill(5));
  CuAssertIntEquals(tc, 2, craft_legacy_rank_for_skill(6));
  CuAssertIntEquals(tc, 10, craft_legacy_rank_for_skill(48));
  CuAssertIntEquals(tc, 18, craft_legacy_rank_for_skill(87));
  CuAssertIntEquals(tc, 20, craft_legacy_rank_for_skill(98));
  CuAssertIntEquals(tc, 20, craft_legacy_rank_for_skill(99));
  CuAssertIntEquals(tc, 20, craft_legacy_rank_for_skill(100));
  CuAssertIntEquals(tc, ABILITY_CRAFT_WEAPONSMITHING, craft_legacy_ability_for_skill(477, NULL));
  CuAssertIntEquals(tc, ABILITY_CRAFT_WEAPONSMITHING, craft_legacy_ability_for_skill(2077, NULL));
  CuAssertIntEquals(tc, ABILITY_CRAFT_TAILORING, craft_legacy_ability_for_skill(474, &second));
  CuAssertIntEquals(tc, ABILITY_HARVEST_GATHERING, second);
  CuAssertIntEquals(tc, -1, craft_legacy_ability_for_skill(2080, &second));
  CuAssertIntEquals(tc, -1, second);
  CuAssertIntEquals(tc, -1, craft_legacy_ability_for_skill(2085, NULL));
  CuAssertIntEquals(tc, -1, craft_legacy_ability_for_skill(1, NULL));
  /* A fresh character keeps the seed's starter access; ranks scale by the divisor. */
  CuAssertIntEquals(tc, 4, craft_legacy_skill_equivalent(&actor.ch, ABILITY_HARVEST_MINING));
  SET_ABILITY(&actor.ch, ABILITY_HARVEST_MINING, 10);
  CuAssertIntEquals(tc, 50, craft_legacy_skill_equivalent(&actor.ch, ABILITY_HARVEST_MINING));
  CuAssertIntEquals(tc, 4, craft_legacy_skill_equivalent(&actor.ch, 47));
}

/** Write a minimal pre-migration player file with the given legacy skill lines and extras. */
static void craft_write_legacy_pfile(CuTest *tc, struct craft_player_files *files, long id,
                                     const char *skills, const char *extra)
{
  char filename[MAX_FILEPATH];
  FILE *file;

  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files->name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  if (file != NULL)
  {
    fprintf(file, "Name: %s\nId  : %ld\nLevl: 7\nSkil:\n%s0 0\n%s", files->name, id, skills, extra);
    fclose(file);
  }
}

/** A veteran converts once: gates they met before they still meet, higher saved ranks and
 * spent talents survive, knitting fills two abilities and pays once, fast crafter pays
 * compensation, slot 47 gains nothing, and a reload of the published file pays nothing more. */
void Test_craft_legacy_skills_convert_once_on_load(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  struct char_data *again = new_char();
  char skills[256], extra[256], marker_line[MAX_INPUT_LENGTH];
  int result, mining, mining_exp, alchemy, tailoring, gathering, points, marker, unsaved;
  int alchemy_equivalent, mining_equivalent, slot47, slot47_exp, training, talent_rank, saved;
  int marker_lines, again_result, again_points, again_alchemy, again_unsaved, again_marker;

  craft_player_files_enter(tc, &files, "crmig", 4310);
  snprintf(skills, sizeof(skills), "%d 48\n%d 87\n%d 30\n%d 66\n%d 4\n", CRAFT_LEGACY_ID_MINING,
           CRAFT_LEGACY_ID_CHEMISTRY, CRAFT_LEGACY_ID_KNITTING, CRAFT_LEGACY_ID_FAST_CRAFTER,
           CRAFT_LEGACY_ID_ARMOR_SMITHING);
  /* Mining already holds a higher saved rank; three points are unspent; one rapid talent is
   * spent; a paid training contract is pending. */
  snprintf(extra, sizeof(extra), "Ablt:\n%d 12\n0 0\nTlpt: 3\nTlrk:", ABILITY_HARVEST_MINING);
  craft_write_legacy_pfile(tc, &files, 4310, skills, extra);
  {
    char filename[MAX_FILEPATH];
    FILE *file;
    int i;

    get_filename(filename, sizeof(filename), PLR_FILE, files.name);
    file = fopen(filename, "a");
    CuAssertPtrNotNull(tc, file);
    if (file != NULL)
    {
      for (i = 0; i < TALENT_MAX; i++)
        fprintf(file, " %d", i == TALENT_RAPID_MINING ? 1 : 0);
      fprintf(file, "\nCrTr: %d 2500 1800000000\n", ABILITY_HARVEST_FORESTRY);
      fclose(file);
    }
  }

  result = load_char(files.name, loaded);
  mining = GET_ABILITY(loaded, ABILITY_HARVEST_MINING);
  mining_exp = GET_CRAFT_SKILL_EXP(loaded, ABILITY_HARVEST_MINING);
  alchemy = GET_ABILITY(loaded, ABILITY_CRAFT_ALCHEMY);
  tailoring = GET_ABILITY(loaded, ABILITY_CRAFT_TAILORING);
  gathering = GET_ABILITY(loaded, ABILITY_HARVEST_GATHERING);
  points = GET_TALENT_POINTS(loaded);
  marker = GET_CRAFT_MIGRATION(loaded);
  unsaved = loaded->player_specials->craft_migration_unsaved;
  alchemy_equivalent = craft_legacy_skill_equivalent(loaded, ABILITY_CRAFT_ALCHEMY);
  mining_equivalent = craft_legacy_skill_equivalent(loaded, ABILITY_HARVEST_MINING);
  slot47 = GET_ABILITY(loaded, 47);
  slot47_exp = GET_CRAFT_SKILL_EXP(loaded, 47);
  training = GET_CRAFT(loaded).training_ability;
  talent_rank = get_talent_rank(loaded, TALENT_RAPID_MINING);

  /* Publish, then reload the published file: the marker is there and nothing is paid twice. */
  GET_PFILEPOS(loaded) = 0;
  saved = save_char_checked(loaded, 0);
  marker_lines = craft_saved_tag_lines(files.name, "CrMg:", marker_line, sizeof(marker_line));
  again_result = load_char(files.name, again);
  again_points = GET_TALENT_POINTS(again);
  again_alchemy = GET_ABILITY(again, ABILITY_CRAFT_ALCHEMY);
  again_unsaved = again->player_specials->craft_migration_unsaved;
  again_marker = GET_CRAFT_MIGRATION(again);

  free_char(loaded);
  free_char(again);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 12, mining);
  CuAssertTrue(tc, mining_exp >= craft_skill_level_exp(NULL, 12));
  CuAssertIntEquals(tc, 18, alchemy);
  CuAssertIntEquals(tc, 6, tailoring);
  CuAssertIntEquals(tc, 6, gathering);
  /* 3 unspent + 18 alchemy + 6 knitting (once) + 66 / 5 fast crafter; mining granted nothing. */
  CuAssertIntEquals(tc, 3 + 18 + 6 + 13, points);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, marker);
  CuAssertIntEquals(tc, 1, unsaved);
  CuAssertTrue(tc, alchemy_equivalent >= 87);
  CuAssertTrue(tc, mining_equivalent >= 48);
  CuAssertIntEquals(tc, 0, slot47);
  CuAssertIntEquals(tc, 0, slot47_exp);
  CuAssertIntEquals(tc, ABILITY_HARVEST_FORESTRY, training);
  CuAssertIntEquals(tc, 1, talent_rank);
  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 1, marker_lines);
  snprintf(skills, sizeof(skills), "CrMg: %d\n", CRAFT_MIGRATION_CURRENT);
  CuAssertStrEquals(tc, skills, marker_line);
  CuAssertIntEquals(tc, 0, again_result);
  CuAssertIntEquals(tc, 3 + 18 + 6 + 13, again_points);
  CuAssertIntEquals(tc, 18, again_alchemy);
  CuAssertIntEquals(tc, 0, again_unsaved);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, again_marker);
}

/** An already-versioned file converts nothing again, and a fresh character (every legacy slot
 * at the seed of 4) gains no ranks or points but is marked converted. */
void Test_craft_migration_skips_versioned_and_fresh_files(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *versioned = new_char();
  struct char_data *fresh = new_char();
  char skills[512], extra[64];
  int i, offset = 0, versioned_result, versioned_mining, versioned_points, versioned_unsaved;
  int fresh_result, fresh_ranks = 0, fresh_points, fresh_marker, fresh_unsaved;

  craft_player_files_enter(tc, &files, "crver", 4311);
  snprintf(skills, sizeof(skills), "%d 99\n", CRAFT_LEGACY_ID_MINING);
  snprintf(extra, sizeof(extra), "CrMg: %d\n", CRAFT_MIGRATION_CURRENT);
  craft_write_legacy_pfile(tc, &files, 4311, skills, extra);
  versioned_result = load_char(files.name, versioned);
  versioned_mining = GET_ABILITY(versioned, ABILITY_HARVEST_MINING);
  versioned_points = GET_TALENT_POINTS(versioned);
  versioned_unsaved = versioned->player_specials->craft_migration_unsaved;

  for (i = CRAFT_LEGACY_ID_FIRST; i <= CRAFT_LEGACY_ID_LAST; i++)
    offset += snprintf(skills + offset, sizeof(skills) - offset, "%d 4\n", i);
  craft_write_legacy_pfile(tc, &files, 4311, skills, "");
  fresh_result = load_char(files.name, fresh);
  for (i = START_CRAFT_ABILITIES; i <= END_HARVEST_ABILITIES; i++)
    fresh_ranks += GET_ABILITY(fresh, i) + GET_CRAFT_SKILL_EXP(fresh, i);
  fresh_points = GET_TALENT_POINTS(fresh);
  fresh_marker = GET_CRAFT_MIGRATION(fresh);
  fresh_unsaved = fresh->player_specials->craft_migration_unsaved;

  free_char(versioned);
  free_char(fresh);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, versioned_result);
  CuAssertIntEquals(tc, 0, versioned_mining);
  CuAssertIntEquals(tc, 0, versioned_points);
  CuAssertIntEquals(tc, 0, versioned_unsaved);
  CuAssertIntEquals(tc, 0, fresh_result);
  CuAssertIntEquals(tc, 0, fresh_ranks);
  CuAssertIntEquals(tc, 0, fresh_points);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, fresh_marker);
  CuAssertIntEquals(tc, 1, fresh_unsaved);
}

/** Load a file whose first ten legacy slots hold value, except one slot, and report the mining
 * and alchemy ranks and the talent points the conversion paid. */
static void craft_load_seeded_pfile(CuTest *tc, struct craft_player_files *files, long birth,
                                    int value, int practised, int practised_value, int *mining,
                                    int *alchemy, int *points)
{
  struct char_data *loaded = new_char();
  char skills[512], extra[64];
  int i, offset = 0;

  for (i = CRAFT_LEGACY_ID_FIRST; i <= CRAFT_LEGACY_ID_FAST_CRAFTER; i++)
    offset += snprintf(skills + offset, sizeof(skills) - offset, "%d %d\n", i,
                       i == practised ? practised_value : value);
  snprintf(extra, sizeof(extra), "Brth: %ld\n", birth);
  craft_write_legacy_pfile(tc, files, 4316, skills, extra);
  CuAssertIntEquals(tc, 0, load_char(files->name, loaded));
  *mining = GET_ABILITY(loaded, ABILITY_HARVEST_MINING);
  *alchemy = GET_ABILITY(loaded, ABILITY_CRAFT_ALCHEMY);
  *points = GET_TALENT_POINTS(loaded);
  free_char(loaded);
}

/** Characters created before April 2015 started with 20 in each slot (5 before March 2013), and
 * an immortal grant set 100: every rank keeps the gate its value met, but only progress above
 * the seed pays talent points. */
void Test_craft_legacy_seed_pays_no_talent_points(CuTest *tc)
{
  struct craft_player_files files;
  int mining[4], alchemy[4], points[4];

  craft_player_files_enter(tc, &files, "crseed", 4316);
  /* June 2013, the 20 seed: mining practised to 31 pays its three ranks above rank 4. */
  craft_load_seeded_pfile(tc, &files, 1370044800L, 20, CRAFT_LEGACY_ID_MINING, 31, &mining[0],
                          &alchemy[0], &points[0]);
  /* June 2012, the 5 seed: chemistry practised to 12 pays two ranks above rank 1. */
  craft_load_seeded_pfile(tc, &files, 1339000000L, 5, CRAFT_LEGACY_ID_CHEMISTRY, 12, &mining[1],
                          &alchemy[1], &points[1]);
  /* Inside the 2013 window but a slot below 20: still the 5 seed. */
  craft_load_seeded_pfile(tc, &files, 1363600000L, 5, CRAFT_LEGACY_ID_CHEMISTRY, 12, &mining[2],
                          &alchemy[2], &points[2]);
  /* 2020, the 4 seed, once promoted: slots at 100 pay nothing; mining earned to 48 pays 10. */
  craft_load_seeded_pfile(tc, &files, 1600000000L, 100, CRAFT_LEGACY_ID_MINING, 48, &mining[3],
                          &alchemy[3], &points[3]);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 7, mining[0]);
  CuAssertIntEquals(tc, 4, alchemy[0]);
  CuAssertIntEquals(tc, 3, points[0]);
  CuAssertIntEquals(tc, 1, mining[1]);
  CuAssertIntEquals(tc, 3, alchemy[1]);
  CuAssertIntEquals(tc, 2, points[1]);
  CuAssertIntEquals(tc, 1, mining[2]);
  CuAssertIntEquals(tc, 3, alchemy[2]);
  CuAssertIntEquals(tc, 2, points[2]);
  CuAssertIntEquals(tc, 10, mining[3]);
  CuAssertIntEquals(tc, 20, alchemy[3]);
  CuAssertIntEquals(tc, 10, points[3]);
}

/** The marker lives outside the project state: a project reset and a respec leave it alone. */
void Test_craft_migration_marker_survives_reset_and_respec(CuTest *tc)
{
  struct char_data *ch = new_char();
  int after_reset, after_respec;

  if (class_list[CLASS_WARRIOR].name == NULL)
    load_class_list();
  if (feat_list[FEAT_ANIMATE_DEAD].name == NULL)
    assign_feats();
  if (race_list[RACE_HUMAN].name == NULL)
    assign_races();
  ch->player.name = strdup("Zzcraftmarker");
  GET_PFILEPOS(ch) = -1;
  IN_ROOM(ch) = NOWHERE;
  GET_CLASS(ch) = CLASS_WARRIOR;
  GET_REAL_RACE(ch) = RACE_HUMAN;
  GET_LEVEL(ch) = 10;
  CLASS_LEVEL(ch, CLASS_WARRIOR) = 10;
  GET_CRAFT_MIGRATION(ch) = CRAFT_MIGRATION_SKILLS;

  reset_current_craft(ch, NULL, FALSE, FALSE);
  after_reset = GET_CRAFT_MIGRATION(ch);
  do_start(ch);
  after_respec = GET_CRAFT_MIGRATION(ch);
  free_char(ch);

  CuAssertIntEquals(tc, CRAFT_MIGRATION_SKILLS, after_reset);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_SKILLS, after_respec);
}

/** The player writer publishes by replacement: when the temporary file cannot be created, the
 * save reports failure and the previous file is untouched. */
void Test_craft_save_failure_leaves_the_previous_file(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  char before[MAX_INPUT_LENGTH], after[MAX_INPUT_LENGTH];
  int first, second = -1, before_lines, after_lines, leftovers = 0;
  struct stat directory;

  craft_player_files_enter(tc, &files, "crsave", 4312);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4312;
  GET_LEVEL(ch) = 10;
  GET_TALENT_POINTS(ch) = 7;
  first = save_char_checked(ch, 0);
  before_lines = craft_saved_tag_lines(files.name, "Tlpt:", before, sizeof(before));

  if (geteuid() != 0 && stat("plrfiles/U-Z", &directory) == 0 &&
      chmod("plrfiles/U-Z", S_IRUSR | S_IXUSR) == 0)
  {
    GET_TALENT_POINTS(ch) = 9;
    second = save_char_checked(ch, 0);
    chmod("plrfiles/U-Z", directory.st_mode & 07777);
  }
  after_lines = craft_saved_tag_lines(files.name, "Tlpt:", after, sizeof(after));
  {
    /* No temporary file may be left behind. */
    DIR *dir = opendir("plrfiles/U-Z");
    struct dirent *entry;

    if (dir != NULL)
    {
      while ((entry = readdir(dir)) != NULL)
        if (strstr(entry->d_name, ".save-tmp.") != NULL)
          leftovers++;
      closedir(dir);
    }
  }
  free_char(ch);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, first);
  CuAssertIntEquals(tc, 1, before_lines);
  CuAssertStrEquals(tc, "Tlpt: 7\n", before);
  if (second != -1)
  {
    CuAssertIntEquals(tc, 0, second);
    CuAssertIntEquals(tc, 1, after_lines);
    CuAssertStrEquals(tc, "Tlpt: 7\n", after);
  }
  CuAssertIntEquals(tc, 0, leftovers);
}

/** A player file whose mode cannot be carried over is still published: the save reports success
 * and its contents replace the previous file's. */
void Test_craft_save_publishes_when_the_mode_cannot_carry_over(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *ch = new_char();
  char after[MAX_INPUT_LENGTH];
  int first, second, after_lines;

  craft_player_files_enter(tc, &files, "crmode", 4313);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4313;
  GET_LEVEL(ch) = 10;
  GET_TALENT_POINTS(ch) = 7;
  first = save_char_checked(ch, 0);
  GET_TALENT_POINTS(ch) = 9;
  save_char_fail_fchmod_for_test(true);
  second = save_char_checked(ch, 0);
  save_char_fail_fchmod_for_test(false);
  after_lines = craft_saved_tag_lines(files.name, "Tlpt:", after, sizeof(after));
  free_char(ch);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, first);
  CuAssertTrue(tc, second);
  CuAssertIntEquals(tc, 1, after_lines);
  CuAssertStrEquals(tc, "Tlpt: 9\n", after);
}

/* ---- Brew and legacy order settlement (crafting consolidation, Phase 4) ---- */

/** A brew resolves once at completion: a success spends its motes and gold and stores the
 * potion; a resolution whose inputs no longer suffice spends nothing. */
void Test_craft_brew_resolution_spends_once_or_nothing(CuTest *tc)
{
  struct craft_actor actor;
  struct char_data *ch = &actor.ch;
  int water_after_success, gold_after_success, potions_after_success;
  int water_after_refusal, gold_after_refusal;

  craft_actor_init(&actor);
  SET_ABILITY(ch, ABILITY_CRAFT_ALCHEMY, 40);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_USE_STORED_CONSUMABLES);
  GET_CRAFT_MOTES(ch, CRAFTING_MOTE_WATER) = 10;
  GET_GOLD(ch) = 100;
  /* Skill 40 + d20 always beats DC 5 unless the die shows a 1; a natural 1 is a critical
   * failure that spends half, so pick a seed whose first d20 is not 1. */
  circle_srandom(7);
  while (d20(ch) == 1)
    ;
  test_brew_resolve(ch, 1, 1, 40, 5, false, CRAFTING_MOTE_WATER, 4, 20);
  water_after_success = GET_CRAFT_MOTES(ch, CRAFTING_MOTE_WATER);
  gold_after_success = GET_GOLD(ch);
  potions_after_success = STORED_POTIONS(ch, 1);

  /* Motes fell below the requirement before resolution: nothing is spent. */
  GET_CRAFT_MOTES(ch, CRAFTING_MOTE_WATER) = 2;
  test_brew_resolve(ch, 1, 1, 40, 5, false, CRAFTING_MOTE_WATER, 4, 20);
  water_after_refusal = GET_CRAFT_MOTES(ch, CRAFTING_MOTE_WATER);
  gold_after_refusal = GET_GOLD(ch);

  CuAssertTrue(tc, water_after_success == 6 || water_after_success == 8);
  CuAssertTrue(tc, gold_after_success == 80 || gold_after_success == 90);
  CuAssertTrue(tc, potions_after_success >= 1 || water_after_success == 8);
  CuAssertIntEquals(tc, 2, water_after_refusal);
  CuAssertIntEquals(tc, gold_after_success, gold_after_refusal);
}

/** CrMg stage 2 settles a room-370 order once: an unfinished order refunds the units its
 * completed installments consumed, a completed one pays its saved rewards, and a settlement
 * the destination cannot take keeps its record; supply and training contracts are untouched. */
void Test_craft_legacy_supply_orders_settle_once(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *partial = new_char(), *again = new_char(), *done = new_char();
  struct char_data *capped = new_char(), *odd = new_char();
  char extra[512];
  int partial_result, partial_steel, partial_vnum, partial_marker, partial_training;
  int partial_contract, again_steel, again_marker, done_gold, done_qp, done_vnum, done_marker;
  int capped_vnum, capped_marker, capped_gold, odd_vnum, odd_marker;
  const char *partial_note;
  bool note_mentions_units;

  craft_player_files_enter(tc, &files, "crord", 4313);

  /* Three of five installments remain: two were made, so six units of steel come back. */
  snprintf(extra, sizeof(extra),
           "Cvnm: 30084\nCmnm: 3\nCqps: 1\nCexp: 200\nCgld: 100\nCdsc: a sword\nCmat: %d\n"
           "CrCT: 1 0\nCrTr: %d 2500 1800000000\nCrMg: 1\n",
           MATERIAL_STEEL, ABILITY_HARVEST_FORESTRY);
  craft_write_legacy_pfile(tc, &files, 4313, "", extra);
  partial_result = load_char(files.name, partial);
  partial_steel = GET_CRAFT_MAT(partial, CRAFT_MAT_STEEL);
  partial_vnum = (int)GET_AUTOCQUEST_VNUM(partial);
  partial_marker = GET_CRAFT_MIGRATION(partial);
  partial_training = GET_CRAFT(partial).training_ability;
  partial_contract = GET_CRAFT(partial).supply_contract_type;
  partial_note = partial->player_specials->craft_settlement_note;
  note_mentions_units = partial_note != NULL && strstr(partial_note, "6 units of steel") != NULL;
  GET_PFILEPOS(partial) = 0;
  CuAssertTrue(tc, save_char_checked(partial, 0));
  CuAssertIntEquals(tc, 0, load_char(files.name, again));
  again_steel = GET_CRAFT_MAT(again, CRAFT_MAT_STEEL);
  again_marker = GET_CRAFT_MIGRATION(again);

  /* Completed and unclaimed: the saved rewards, once. */
  snprintf(extra, sizeof(extra),
           "Gold: 5\nCvnm: 30084\nCmnm: 0\nCqps: 1\nCexp: 200\nCgld: 100\nCdsc: a shield\n"
           "Cmat: %d\n",
           MATERIAL_WOOD);
  craft_write_legacy_pfile(tc, &files, 4313, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, done));
  done_gold = GET_GOLD(done);
  done_qp = GET_QUESTPOINTS(done);
  done_vnum = (int)GET_AUTOCQUEST_VNUM(done);
  done_marker = GET_CRAFT_MIGRATION(done);

  /* A reward past the gold capacity keeps its record and does not advance the stage. */
  snprintf(extra, sizeof(extra),
           "Gold: %d\nCvnm: 30084\nCmnm: 0\nCqps: 0\nCexp: 0\nCgld: 100\nCdsc: a cape\nCmat: %d\n",
           MAX_GOLD - 10, MATERIAL_HEMP);
  craft_write_legacy_pfile(tc, &files, 4313, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, capped));
  capped_vnum = (int)GET_AUTOCQUEST_VNUM(capped);
  capped_marker = GET_CRAFT_MIGRATION(capped);
  capped_gold = GET_GOLD(capped);

  /* An unmappable material keeps its record too. */
  snprintf(extra, sizeof(extra), "Cvnm: 30084\nCmnm: 2\nCdsc: a lens\nCmat: %d\n", MATERIAL_GLASS);
  craft_write_legacy_pfile(tc, &files, 4313, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, odd));
  odd_vnum = (int)GET_AUTOCQUEST_VNUM(odd);
  odd_marker = GET_CRAFT_MIGRATION(odd);

  free_char(partial);
  free_char(again);
  free_char(done);
  free_char(capped);
  free_char(odd);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, partial_result);
  CuAssertIntEquals(tc, 6, partial_steel);
  CuAssertIntEquals(tc, 0, partial_vnum);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, partial_marker);
  CuAssertIntEquals(tc, ABILITY_HARVEST_FORESTRY, partial_training);
  CuAssertIntEquals(tc, 1, partial_contract);
  CuAssertTrue(tc, note_mentions_units);
  CuAssertIntEquals(tc, 6, again_steel);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, again_marker);
  CuAssertIntEquals(tc, 105, done_gold);
  CuAssertIntEquals(tc, 1, done_qp);
  CuAssertIntEquals(tc, 0, done_vnum);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, done_marker);
  CuAssertIntEquals(tc, 30084, capped_vnum);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_SKILLS, capped_marker);
  CuAssertIntEquals(tc, MAX_GOLD - 10, capped_gold);
  CuAssertIntEquals(tc, 30084, odd_vnum);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_SKILLS, odd_marker);
}

/* ---- Old wilderness holdings (crafting consolidation, Phase 5) ---- */

/** CrMg stage 3 moves old wilderness holdings through the frozen pre-merge mapping once:
 * every material category and quality tier, the cold-iron and adamantine exceptions, mote
 * records at quantity times quality, duplicate destinations aggregated; invalid records or
 * overflow keep every holding and the stage unadvanced; files at markers 0, 1, and 2 all reach
 * stage 3 exactly once, and a pre-merge file still converts its legacy skills. */
void Test_craft_wilderness_holdings_convert_once(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *holder = new_char(), *again = new_char(), *invalid = new_char();
  struct char_data *full = new_char(), *unmarked = new_char();
  char extra[1024];
  int result, marker, hemp, flax, tin, cold_iron, adamantine, ironwood, dragonscale, water;
  int water_records, again_marker, again_hemp, again_records, invalid_marker, invalid_records;
  int invalid_hemp, full_marker, full_records, unmarked_marker, unmarked_mining, unmarked_hemp;

  craft_player_files_enter(tc, &files, "crwild", 4314);

  /* Vegetation poor and common (hemp, flax; hemp twice), minerals poor (tin), cold iron at
   * rare, adamantine at legendary, ironwood at legendary, dragonscale at legendary, and
   * spring water at uncommon (three motes per unit). */
  snprintf(extra, sizeof(extra),
           "CrMg: 2\nWMat: 8\nMat : %d 0 1 2\nMat : %d 0 2 3\nMat : %d 0 1 4\nMat : %d %d 4 1\n"
           "Mat : %d %d 5 1\nMat : %d %d 5 2\nMat : %d %d 5 1\nMat : %d %d 3 2\nMat : %d 0 1 5\n",
           RESOURCE_VEGETATION, RESOURCE_VEGETATION, RESOURCE_MINERALS, RESOURCE_MINERALS,
           ORE_COLD_IRON, RESOURCE_MINERALS, ORE_ADAMANTINE, RESOURCE_WOOD, WOOD_IRONWOOD,
           RESOURCE_GAME, GAME_LEATHER, RESOURCE_WATER, WATER_SPRING, RESOURCE_VEGETATION);
  craft_write_legacy_pfile(tc, &files, 4314, "", extra);
  result = load_char(files.name, holder);
  marker = GET_CRAFT_MIGRATION(holder);
  hemp = GET_CRAFT_MAT(holder, CRAFT_MAT_HEMP);
  flax = GET_CRAFT_MAT(holder, CRAFT_MAT_FLAX);
  tin = GET_CRAFT_MAT(holder, CRAFT_MAT_TIN);
  cold_iron = GET_CRAFT_MAT(holder, CRAFT_MAT_COLD_IRON);
  adamantine = GET_CRAFT_MAT(holder, CRAFT_MAT_ADAMANTINE);
  ironwood = GET_CRAFT_MAT(holder, CRAFT_MAT_IRONWOOD);
  dragonscale = GET_CRAFT_MAT(holder, CRAFT_MAT_DRAGONSCALE);
  water = GET_CRAFT_MOTES(holder, CRAFTING_MOTE_WATER);
  water_records = holder->player_specials->saved.stored_material_count;
  GET_PFILEPOS(holder) = 0;
  CuAssertTrue(tc, save_char_checked(holder, 0));
  CuAssertIntEquals(tc, 0, load_char(files.name, again));
  again_marker = GET_CRAFT_MIGRATION(again);
  again_hemp = GET_CRAFT_MAT(again, CRAFT_MAT_HEMP);
  again_records = again->player_specials->saved.stored_material_count;

  /* An invalid record keeps every holding and the stage stays at 2. */
  snprintf(extra, sizeof(extra), "CrMg: 2\nWMat: 2\nMat : %d 0 1 2\nMat : 99 0 1 2\n",
           RESOURCE_VEGETATION);
  craft_write_legacy_pfile(tc, &files, 4314, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, invalid));
  invalid_marker = GET_CRAFT_MIGRATION(invalid);
  invalid_records = invalid->player_specials->saved.stored_material_count;
  invalid_hemp = GET_CRAFT_MAT(invalid, CRAFT_MAT_HEMP);

  /* A destination that cannot take the holdings keeps them too (the balance block is one
   * value per material in id order, ended by -1). */
  {
    int offset, material;

    offset = snprintf(extra, sizeof(extra), "CrMg: 2\nWMat: 1\nMat : %d 0 1 2\nCfMt:\n",
                      RESOURCE_VEGETATION);
    for (material = 0; material < NUM_CRAFT_MATS; material++)
      offset += snprintf(extra + offset, sizeof(extra) - (size_t)offset, "%d\n",
                         material == CRAFT_MAT_HEMP ? INT_MAX : 0);
    snprintf(extra + offset, sizeof(extra) - (size_t)offset, "-1\n");
  }
  craft_write_legacy_pfile(tc, &files, 4314, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, full));
  full_marker = GET_CRAFT_MIGRATION(full);
  full_records = full->player_specials->saved.stored_material_count;

  /* A pre-merge file (no marker, legacy skills, holdings) runs every stage in order. */
  snprintf(extra, sizeof(extra), "WMat: 1\nMat : %d 0 1 2\n", RESOURCE_VEGETATION);
  snprintf(extra + strlen(extra), sizeof(extra) - strlen(extra), "%s", "");
  {
    char skills[64];

    snprintf(skills, sizeof(skills), "%d 48\n", CRAFT_LEGACY_ID_MINING);
    craft_write_legacy_pfile(tc, &files, 4314, skills, extra);
  }
  CuAssertIntEquals(tc, 0, load_char(files.name, unmarked));
  unmarked_marker = GET_CRAFT_MIGRATION(unmarked);
  unmarked_mining = GET_ABILITY(unmarked, ABILITY_HARVEST_MINING);
  unmarked_hemp = GET_CRAFT_MAT(unmarked, CRAFT_MAT_HEMP);

  free_char(holder);
  free_char(again);
  free_char(invalid);
  free_char(full);
  free_char(unmarked);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_HOLDINGS, marker);
  CuAssertIntEquals(tc, 7, hemp);
  CuAssertIntEquals(tc, 3, flax);
  CuAssertIntEquals(tc, 4, tin);
  CuAssertIntEquals(tc, 1, cold_iron);
  CuAssertIntEquals(tc, 1, adamantine);
  CuAssertIntEquals(tc, 2, ironwood);
  CuAssertIntEquals(tc, 1, dragonscale);
  CuAssertIntEquals(tc, 6, water);
  CuAssertIntEquals(tc, 0, water_records);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_HOLDINGS, again_marker);
  CuAssertIntEquals(tc, 7, again_hemp);
  CuAssertIntEquals(tc, 0, again_records);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_ORDERS, invalid_marker);
  CuAssertIntEquals(tc, 2, invalid_records);
  CuAssertIntEquals(tc, 0, invalid_hemp);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_ORDERS, full_marker);
  CuAssertIntEquals(tc, 1, full_records);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_HOLDINGS, unmarked_marker);
  CuAssertIntEquals(tc, 10, unmarked_mining);
  CuAssertIntEquals(tc, 2, unmarked_hemp);
}

/** Entry publishes a migration that ran at load and delivers its settlement note once. When the
 * player file cannot be replaced, the flag stays set for the next save while the note is still
 * spent; characters without player specials are left alone. */
void Test_craft_migration_publishes_at_entry(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *first = new_char(), *again = new_char(), *blocked = new_char();
  char extra[256], filename[MAX_FILEPATH];
  int first_unsaved, first_marker, again_marker, again_unsaved, blocked_unsaved;
  bool first_note, first_published, first_note_spent, again_note, blocked_published;
  bool blocked_note_spent, npc_published;

  craft_player_files_enter(tc, &files, "crpub", 4314);
  snprintf(extra, sizeof(extra),
           "Cvnm: 30084\nCmnm: 3\nCqps: 1\nCexp: 200\nCgld: 100\nCdsc: a sword\nCmat: %d\n"
           "CrMg: 1\n",
           MATERIAL_STEEL);
  craft_write_legacy_pfile(tc, &files, 4314, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, first));
  first_unsaved = first->player_specials->craft_migration_unsaved;
  first_marker = GET_CRAFT_MIGRATION(first);
  first_note = first->player_specials->craft_settlement_note != NULL;
  GET_PFILEPOS(first) = 0;
  first_published = craft_publish_migration_on_entry(first);
  first_note_spent = first->player_specials->craft_settlement_note == NULL;
  CuAssertIntEquals(tc, 0, load_char(files.name, again));
  again_marker = GET_CRAFT_MIGRATION(again);
  again_unsaved = again->player_specials->craft_migration_unsaved;
  again_note = again->player_specials->craft_settlement_note != NULL;

  /* The player file replaced by a directory: the temporary file is discarded, nothing moves. */
  craft_write_legacy_pfile(tc, &files, 4314, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, blocked));
  GET_PFILEPOS(blocked) = 0;
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  CuAssertIntEquals(tc, 0, unlink(filename));
  CuAssertIntEquals(tc, 0, mkdir(filename, 0700));
  blocked_published = craft_publish_migration_on_entry(blocked);
  blocked_unsaved = blocked->player_specials->craft_migration_unsaved;
  blocked_note_spent = blocked->player_specials->craft_settlement_note == NULL;
  CuAssertIntEquals(tc, 0, rmdir(filename));

  SET_BIT_AR(MOB_FLAGS(first), MOB_ISNPC);
  npc_published = craft_publish_migration_on_entry(first) && craft_publish_migration_on_entry(NULL);
  REMOVE_BIT_AR(MOB_FLAGS(first), MOB_ISNPC);

  free_char(first);
  free_char(again);
  free_char(blocked);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertTrue(tc, first_unsaved);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, first_marker);
  CuAssertTrue(tc, first_note);
  CuAssertTrue(tc, first_published);
  CuAssertTrue(tc, first_note_spent);
  CuAssertIntEquals(tc, CRAFT_MIGRATION_CURRENT, again_marker);
  CuAssertTrue(tc, !again_unsaved);
  CuAssertTrue(tc, !again_note);
  CuAssertTrue(tc, !blocked_published);
  CuAssertTrue(tc, blocked_unsaved);
  CuAssertTrue(tc, blocked_note_spent);
  CuAssertTrue(tc, npc_published);
}

/** A resize interrupted by logout refunds its material at load; a refused credit keeps the
 * allocation for recovery in game. */
void Test_craft_resize_interrupted_by_logout_refunds_at_load(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char(), *full = new_char();
  char extra[512];
  int loaded_steel, loaded_size, full_size, full_num, full_steel, offset, material;

  craft_player_files_enter(tc, &files, "crrsz", 4315);
  snprintf(extra, sizeof(extra), "CrMg: %d\nRSSz: 2\nRSMT: %d\nRSMN: 3\n", CRAFT_MIGRATION_CURRENT,
           CRAFT_MAT_STEEL);
  craft_write_legacy_pfile(tc, &files, 4315, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, loaded));
  loaded_steel = GET_CRAFT_MAT(loaded, CRAFT_MAT_STEEL);
  loaded_size = GET_CRAFT(loaded).new_size;

  offset = snprintf(extra, sizeof(extra), "CrMg: %d\nRSSz: 2\nRSMT: %d\nRSMN: 3\nCfMt:\n",
                    CRAFT_MIGRATION_CURRENT, CRAFT_MAT_STEEL);
  for (material = 0; material < NUM_CRAFT_MATS; material++)
    offset += snprintf(extra + offset, sizeof(extra) - (size_t)offset, "%d\n",
                       material == CRAFT_MAT_STEEL ? INT_MAX : 0);
  snprintf(extra + offset, sizeof(extra) - (size_t)offset, "-1\n");
  craft_write_legacy_pfile(tc, &files, 4315, "", extra);
  CuAssertIntEquals(tc, 0, load_char(files.name, full));
  full_size = GET_CRAFT(full).new_size;
  full_num = GET_CRAFT(full).resize_mat_num;
  full_steel = GET_CRAFT_MAT(full, CRAFT_MAT_STEEL);

  free_char(loaded);
  free_char(full);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 3, loaded_steel);
  CuAssertIntEquals(tc, 0, loaded_size);
  CuAssertIntEquals(tc, 2, full_size);
  CuAssertIntEquals(tc, 3, full_num);
  CuAssertIntEquals(tc, INT_MAX, full_steel);
}

/** Device work still blocks ordinary commands while the crafting blockers are gone. */
void Test_device_events_block_ordinary_commands(CuTest *tc)
{
  struct craft_trainer_fixture fixture;
  char seen[MAX_STRING_LENGTH];
  bool creation_blocks, repair_blocks, score_allowed;

  craft_trainer_begin(tc, &fixture, "crdev", 4317);
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  attach_mud_event(new_mud_event(eDEVICE_CREATION, fixture.player, NULL), 300L * PASSES_PER_SEC);
  craft_trainer_command(&fixture, "inventory", seen, sizeof(seen));
  creation_blocks = strstr(seen, "devising your creation") != NULL;
  event_cancel_specific(fixture.player, eDEVICE_CREATION);
  attach_mud_event(new_mud_event(eDEVICE_REPAIR, fixture.player, NULL), 300L * PASSES_PER_SEC);
  craft_trainer_command(&fixture, "inventory", seen, sizeof(seen));
  repair_blocks = strstr(seen, "repairing your device") != NULL;
  craft_trainer_command(&fixture, "score", seen, sizeof(seen));
  score_allowed = strstr(seen, "repairing your device") == NULL;
  event_cancel_specific(fixture.player, eDEVICE_REPAIR);
  event_free_all();
  CuAssertIntEquals(tc, 0, craft_trainer_end(&fixture));

  CuAssertTrue(tc, creation_blocks);
  CuAssertTrue(tc, repair_blocks);
  CuAssertTrue(tc, score_allowed);
}

/* Every numeric player-file tag that load_char() reads with a parse helper, in file order.
 * Aliases use their own multi-line format and are written separately. */
static const char *const numeric_pfile_tags[] = {
    "AExp", "Alin", "Age ", "AgeS", "Badp", "BGnd", "Blst", "Bane", "Bost", "Bank", "ClkT", "Con ",
    "Cln ", "Clrk", "CPts", "ChEn", "CrMe", "DvCD", "Dex ", "DRMd", "Drnk", "Drol", "DipT", "DRac",
    "DDex", "DStr", "DCon", "DAC ", "Dom1", "Dom2", "DrMU", "DrMT", "DrBT", "DrDT", "Exp ", "Efpt",
    "EidB", "EidC", "EfMU", "EfMT", "EncM", "Frez", "FBAB", "FrgC", "FLGT", "FdBn", "FLGU", "Ftpt",
    "FSWT", "FSWU", "FstH", "FttD", "GTCT", "GTCU", "GODT", "GODU", "Hite", "HECn", "HPRg", "Hrol",
    "Hung", "InqR", "Int ", "Invs", "InFT", "InFU", "KpkS", "Lern", "Lmot", "Lnew", "LTCT", "LTCU",
    "Mrph", "MFrm", "MVRg", "MBSp", "MBSU", "MBSR", "NAr0", "NAr1", "NAr2", "NAr3", "NecC", "PSRg",
    "PxDU", "PxDT", "PvPT", "DvRc", "Qstp", "Qpnt", "Qcnt", "Qcn1", "Qcn2", "QSvy", "RacR", "Res1",
    "Res2", "Res3", "Res4", "Res5", "Res6", "Res7", "Res8", "Res9", "ResA", "ResB", "ResC", "ResD",
    "ResE", "ResF", "ResG", "ResH", "ResI", "ResJ", "ResK", "RSc1", "RSc2", "RetC", "BDsU", "BDsT",
    "BSlU", "BSlT", "Sex ", "SBld", "Scrg", "SpWC", "IrMC", "QkCs", "SpRc", "SpRs", "Slyr", "SySt",
    "SSch", "SpNM", "SpCd", "SuLR", "SuNR", "Tmpl", "Thir", "Thr1", "Thr2", "Thr3", "Thr4", "Thr5",
    "Trns", "VitS", "Wate", "Wimp", "Wis ",
};

/* The value written for each tag: small enough for the byte-sized fields. */
static int numeric_pfile_value(size_t index)
{
  return 1 + (int)(index % 90);
}

static int numeric_pfile_tag_value(const char *tag)
{
  size_t i;

  for (i = 0; i < sizeof(numeric_pfile_tags) / sizeof(numeric_pfile_tags[0]); i++)
    if (strcmp(numeric_pfile_tags[i], tag) == 0)
      return numeric_pfile_value(i);
  return -1;
}

void Test_load_char_reads_every_numeric_tag(CuTest *tc)
{
  struct craft_player_files files;
  struct char_data *loaded = new_char();
  struct alias_data *alias;
  char filename[MAX_FILEPATH];
  FILE *file;
  size_t i;
  int result, alignment, bank, con, dex, intel, wis, exp, height, weight, wimp, sex, trains;
  int questpoints, save_fort, save_will, practices, alias_type, invis;
  sbyte hunger;
  char alias_name[64] = "", alias_replacement[64] = "";

  craft_player_files_enter(tc, &files, "crtags", 4303);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  fprintf(file, "Name: %s\nId  : 4303\nLevl: 7\n", files.name);
  for (i = 0; i < sizeof(numeric_pfile_tags) / sizeof(numeric_pfile_tags[0]); i++)
    fprintf(file, "%s: %d\n", numeric_pfile_tags[i], numeric_pfile_value(i));
  fprintf(file, "Alis: 1\n gt\n group tell\n1\n");
  fclose(file);

  result = load_char(files.name, loaded);
  alignment = GET_ALIGNMENT(loaded);
  bank = GET_BANK_GOLD(loaded);
  con = GET_REAL_CON(loaded);
  dex = GET_REAL_DEX(loaded);
  intel = GET_REAL_INT(loaded);
  wis = GET_REAL_WIS(loaded);
  exp = (int)GET_EXP(loaded);
  height = GET_HEIGHT(loaded);
  weight = GET_WEIGHT(loaded);
  wimp = GET_WIMP_LEV(loaded);
  sex = GET_SEX(loaded);
  trains = GET_TRAINS(loaded);
  questpoints = GET_QUESTPOINTS(loaded);
  save_fort = GET_REAL_SAVE(loaded, 0);
  save_will = GET_REAL_SAVE(loaded, 2);
  hunger = GET_COND(loaded, HUNGER);
  practices = GET_PRACTICES(loaded);
  invis = GET_INVIS_LEV(loaded);
  alias = GET_ALIASES(loaded);
  alias_type = alias != NULL ? alias->type : -1;
  if (alias != NULL)
  {
    strlcpy(alias_name, alias->alias, sizeof(alias_name));
    strlcpy(alias_replacement, alias->replacement, sizeof(alias_replacement));
  }
  free_char(loaded);
  CuAssertIntEquals(tc, 0, craft_player_files_leave(&files));

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Alin"), alignment);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Bank"), bank);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Con "), con);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Dex "), dex);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Int "), intel);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Wis "), wis);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Exp "), exp);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Hite"), height);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Wate"), weight);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Wimp"), wimp);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Sex "), sex);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Trns"), trains);
  /* Qpnt is the older spelling of Qstp and is written after it. */
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Qpnt"), questpoints);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Thr1"), save_fort);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Thr3"), save_will);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Hung"), hunger);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Lern"), practices);
  CuAssertIntEquals(tc, numeric_pfile_tag_value("Invs"), invis);
  CuAssertIntEquals(tc, 1, alias_type);
  CuAssertStrEquals(tc, "gt", alias_name);
  CuAssertStrEquals(tc, " group tell", alias_replacement);
}
