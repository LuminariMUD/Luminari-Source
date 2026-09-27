/* Production-linked tests for the artificer class and its Weird Science devices (GitLab work
 * item 4, docs/ongoing-projects/ARTIFICER_DEVICE_FIXES.md). */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/premadebuilds.h"
#include "../../src/character/race.h"
#include "../../src/core/comm.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/craft/brew.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/events/actions.h"
#include "../../src/events/mud_event.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/** One artificer standing alone in one room, with its output captured. */
struct artificer_fixture
{
  struct room_data room;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  room_rnum saved_top_of_world;
};

static void load_spell_tables(void)
{
  if (class_list[CLASS_WIZARD].name == NULL)
    load_class_list();
  if (spell_info[SPELL_MAGIC_MISSILE].name == NULL ||
      spell_info[SPELL_MAGIC_MISSILE].name == unused_spellname)
    mag_assign_spells();
  if (spell_info[SPELL_MAGIC_MISSILE].min_level[CLASS_WIZARD] >= LVL_IMMORT)
    init_spell_levels();
}

static void begin_artificer(struct artificer_fixture *f, int level)
{
  struct char_data *ch = &f->ch;

  load_spell_tables();
  event_init();
  memset(f, 0, sizeof(*f));
  f->saved_world = world;
  f->saved_top_of_world = top_of_world;
  f->saved_character_list = character_list;
  world = &f->room;
  top_of_world = 0;

  clear_char(ch);
  ch->player_specials = &f->specials;
  ch->player.name = CuMutableString("Zzartificer");
  ch->desc = &f->descriptor;
  IN_ROOM(ch) = 0;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = GET_MAX_HIT(ch) = 1000;
  GET_CLASS(ch) = CLASS_ARTIFICER;
  GET_LEVEL(ch) = level;
  CLASS_LEVEL(ch, CLASS_ARTIFICER) = level;
  SET_FEAT(ch, FEAT_WEIRD_SCIENCE, 1);
  f->room.people = ch;
  character_list = ch;

  f->descriptor.character = ch;
  f->descriptor.output = f->descriptor.small_outbuf;
  f->descriptor.bufspace = SMALL_BUFSIZE - 1;
  f->descriptor.pProtocol = ProtocolCreate();
  STATE(&f->descriptor) = CON_PLAYING;
}

static void end_artificer(struct artificer_fixture *f)
{
  while (f->ch.affected != NULL)
    affect_remove_no_total(&f->ch, f->ch.affected);
  clear_char_event_list(&f->ch);
  event_free_all();
  (void)event_test_select_backend(EVENT_BACKEND_UNINITIALIZED);
  f->ch.desc = NULL;
  ProtocolDestroy(f->descriptor.pProtocol);
  if (f->descriptor.large_outbuf != NULL)
  {
    free(f->descriptor.large_outbuf->text);
    free(f->descriptor.large_outbuf);
  }
  world = f->saved_world;
  top_of_world = f->saved_top_of_world;
  character_list = f->saved_character_list;
}

static void clear_output(struct artificer_fixture *f)
{
  if (f->descriptor.large_outbuf != NULL)
  {
    free(f->descriptor.large_outbuf->text);
    free(f->descriptor.large_outbuf);
    f->descriptor.large_outbuf = NULL;
  }
  f->descriptor.output = f->descriptor.small_outbuf;
  f->descriptor.output[0] = '\0';
  f->descriptor.bufptr = 0;
  f->descriptor.bufspace = SMALL_BUFSIZE - 1;
}

/* Give the artificer a device holding the given spells at their lower list level. */
static struct player_invention *give_device(struct artificer_fixture *f, const int *spells,
                                            int count)
{
  struct player_invention *inv;
  int i, wizard, cleric;

  inv = &f->specials.saved.inventions[f->specials.saved.num_inventions++];
  strlcpy(inv->short_description, "a test device", sizeof(inv->short_description));
  for (i = 0; i < count; i++)
  {
    wizard = spell_info[spells[i]].min_level[CLASS_WIZARD];
    cleric = spell_info[spells[i]].min_level[CLASS_CLERIC];
    inv->spell_effects[i] = spells[i];
    inv->spell_levels[i] = MIN(wizard, cleric);
  }
  inv->num_spells = count;
  return inv;
}

static void run_device(struct artificer_fixture *f, const char *argument)
{
  clear_output(f);
  do_device(&f->ch, argument, 0, 0);
}

/* The reported bug: a level 20 artificer could not add a 3rd- or 4th-circle spell to a device,
 * because device add compared a class spell level against the circle cap. */
void Test_artificer_device_add_takes_third_and_fourth_circle_spells(CuTest *tc)
{
  struct artificer_fixture f;
  const int missile[] = {SPELL_MAGIC_MISSILE};
  struct player_invention *inv;
  int bolt_level, storm_level, num_spells;

  begin_artificer(&f, 20);
  CuAssertIntEquals(tc, 5, spell_info[SPELL_LIGHTNING_BOLT].min_level[CLASS_WIZARD]);
  CuAssertIntEquals(tc, 7, spell_info[SPELL_ICE_STORM].min_level[CLASS_WIZARD]);
  inv = give_device(&f, missile, 1);
  run_device(&f, "add 1 lightning bolt");
  run_device(&f, "add 1 ice storm");
  num_spells = inv->num_spells;
  bolt_level = inv->spell_levels[1];
  storm_level = inv->spell_levels[2];
  end_artificer(&f);

  CuAssertIntEquals(tc, 3, num_spells);
  CuAssertIntEquals(tc, 5, bolt_level);
  CuAssertIntEquals(tc, 7, storm_level);
}

/* device add keeps the circle cap and, like device create, the per-circle slot budget. */
void Test_artificer_device_add_keeps_circle_cap_and_budget(CuTest *tc)
{
  struct artificer_fixture f;
  const int missile[] = {SPELL_MAGIC_MISSILE};
  const int missile_bolt[] = {SPELL_MAGIC_MISSILE, SPELL_LIGHTNING_BOLT};
  const int hands[] = {SPELL_BURNING_HANDS};
  struct player_invention *first, *second;
  int low_level_spells, budget_spells;
  bool refused_budget;

  begin_artificer(&f, 3);
  first = give_device(&f, missile, 1);
  run_device(&f, "add 1 lightning bolt");
  low_level_spells = first->num_spells;
  end_artificer(&f);

  /* Level 5 has one 3rd-circle slot, which the first device's lightning bolt takes. */
  begin_artificer(&f, 5);
  give_device(&f, missile_bolt, 2);
  second = give_device(&f, hands, 1);
  run_device(&f, "add 2 fireball");
  budget_spells = second->num_spells;
  refused_budget = strstr(f.descriptor.output, "no free circle slot") != NULL;
  end_artificer(&f);

  CuAssertIntEquals(tc, 1, low_level_spells);
  CuAssertIntEquals(tc, 1, budget_spells);
  CuAssertTrue(tc, refused_budget);
}

/* Circle slots used to go to spells greedily in typed order, so with one 1st-circle slot left
 * "strength" (cleric 1, wizard 3) took it and "mage armor" (wizard 1) failed. Build time now
 * follows the assigned levels: 30 seconds each for 3 + 1. */
void Test_artificer_device_create_fits_spells_in_any_order(CuTest *tc)
{
  struct artificer_fixture f;
  const int shield[] = {SPELL_SHIELD};
  struct mud_event_data *creation;
  char expected[64], variables[MAX_STRING_LENGTH] = {'\0'};
  long remaining = 0;

  begin_artificer(&f, 3);
  CuAssertIntEquals(tc, 1, spell_info[SPELL_STRENGTH].min_level[CLASS_CLERIC]);
  CuAssertIntEquals(tc, 3, spell_info[SPELL_STRENGTH].min_level[CLASS_WIZARD]);
  give_device(&f, shield, 1);
  run_device(&f, "create strength \"mage armor\"");
  creation = char_has_mud_event(&f.ch, eDEVICE_CREATION);
  if (creation != NULL)
  {
    strlcpy(variables, creation->sVariables, sizeof(variables));
    remaining = mud_event_remaining(creation);
  }
  end_artificer(&f);

  snprintf(expected, sizeof(expected), "%d:3,%d:1|2|", SPELL_STRENGTH, SPELL_MAGE_ARMOR);
  CuAssertPtrNotNull(tc, creation);
  CuAssertTrue(tc, strncmp(variables, expected, strlen(expected)) == 0);
  CuAssertIntEquals(tc, (int)(120 * PASSES_PER_SEC), (int)remaining);
}

/* Skills share the spell number space; they used to pass as 1st-circle device spells that did
 * nothing. A fourth spell without Brilliance and Blunder used to be dropped silently. */
void Test_artificer_device_create_refuses_skills_and_extra_spells(CuTest *tc)
{
  struct artificer_fixture f;
  bool skill_started, extra_started, extra_told;

  begin_artificer(&f, 20);
  run_device(&f, "create kick");
  skill_started = char_has_mud_event(&f.ch, eDEVICE_CREATION) != NULL;
  run_device(&f, "create shield \"mage armor\" strength haste");
  extra_started = char_has_mud_event(&f.ch, eDEVICE_CREATION) != NULL;
  extra_told = strstr(f.descriptor.output, "at most 3 spells") != NULL;
  end_artificer(&f);

  CuAssertTrue(tc, !skill_started);
  CuAssertTrue(tc, !extra_started);
  CuAssertTrue(tc, extra_told);
}

/* weird_science_table ends at level 20; only staff can push the class past it. */
void Test_artificer_device_create_past_level_twenty_uses_the_last_row(CuTest *tc)
{
  struct artificer_fixture f;
  bool started;

  begin_artificer(&f, 25);
  run_device(&f, "create \"ice storm\"");
  started = char_has_mud_event(&f.ch, eDEVICE_CREATION) != NULL;
  end_artificer(&f);

  CuAssertTrue(tc, started);
}

/* A full artificer is told it has no free slot instead of being counted one per device. */
void Test_artificer_device_create_reports_free_slots(CuTest *tc)
{
  struct artificer_fixture f;
  const int shield[] = {SPELL_SHIELD};
  bool full, empty_told;

  begin_artificer(&f, 1);
  run_device(&f, "create");
  empty_told = strstr(f.descriptor.output, "Free spell slots: 1") != NULL;
  give_device(&f, shield, 1);
  run_device(&f, "create");
  full = strstr(f.descriptor.output, "no free device slots") != NULL;
  end_artificer(&f);

  CuAssertTrue(tc, empty_told);
  CuAssertTrue(tc, full);
}

/* A broken device and a failed Use Magic Device check both take the standard action, so the
 * player cannot retry at once. */
void Test_artificer_failed_device_use_takes_the_standard_action(CuTest *tc)
{
  struct artificer_fixture f;
  const int shield[] = {SPELL_SHIELD};
  struct player_invention *inv;
  bool broken_waits, failed_waits;
  int penalty;

  begin_artificer(&f, 1);
  inv = give_device(&f, shield, 1);
  inv->broken = TRUE;
  run_device(&f, "use 1");
  broken_waits = !is_action_available(&f.ch, atSTANDARD, FALSE);
  end_artificer(&f);

  begin_artificer(&f, 1);
  inv = give_device(&f, shield, 1);
  inv->uses = 5;
  inv->dc_penalty = 200;
  run_device(&f, "use 1");
  failed_waits = !is_action_available(&f.ch, atSTANDARD, FALSE);
  penalty = inv->dc_penalty;
  end_artificer(&f);

  CuAssertTrue(tc, broken_waits);
  CuAssertTrue(tc, failed_waits);
  CuAssertIntEquals(tc, 204, penalty);
}

/* Brilliance and Blunder: an exploding device deals 1d6 per circle of its spells. A cleric-only
 * spell used to count as 11 circles, and a blast that left the maker below 0 hit points did not
 * change the maker's position. */
void Test_artificer_device_explosion_counts_device_circles(CuTest *tc)
{
  struct artificer_fixture f;
  const int cure[] = {SPELL_CURE_LIGHT};
  struct player_invention *inv;
  int attempt, lost = 0, position;

  begin_artificer(&f, 1);
  CuAssertIntEquals(tc, LVL_IMMORT, spell_info[SPELL_CURE_LIGHT].min_level[CLASS_WIZARD]);
  SET_FEAT(&f.ch, FEAT_BRILLIANCE_AND_BLUNDER, 1);
  inv = give_device(&f, cure, 1);
  for (attempt = 0; attempt < 200 && !inv->broken; attempt++)
  {
    inv->uses = 5;
    inv->dc_penalty = 200;
    GET_HIT(&f.ch) = 1000;
    run_device(&f, "use 1");
  }
  lost = 1000 - GET_HIT(&f.ch);

  inv->broken = FALSE;
  for (attempt = 0; attempt < 200 && !inv->broken; attempt++)
  {
    inv->uses = 5;
    inv->dc_penalty = 200;
    GET_HIT(&f.ch) = 1;
    run_device(&f, "use 1");
  }
  position = (unsigned char)GET_POS(&f.ch);
  end_artificer(&f);

  CuAssertTrue(tc, lost >= 1 && lost <= 6);
  CuAssertTrue(tc, position < POS_STANDING);
}

/* A spell name longer than the word buffer in find_skill_num() used to overrun it. */
void Test_artificer_device_create_survives_an_overlong_spell_word(CuTest *tc)
{
  struct artificer_fixture f;
  char argument[MAX_INPUT_LENGTH];
  char name[401];
  bool started;

  memset(name, 'm', sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';
  CuAssertIntEquals(tc, -1, find_skill_num(name));
  begin_artificer(&f, 20);
  snprintf(argument, sizeof(argument), "create %s", name);
  run_device(&f, argument);
  started = char_has_mud_event(&f.ch, eDEVICE_CREATION) != NULL;
  end_artificer(&f);

  CuAssertTrue(tc, !started);
}

/* device create cancel and device repair cancel were refused by the wait device work puts on
 * every other command, so the work could not be abandoned. Other device commands still wait. */
void Test_artificer_device_work_can_be_cancelled(CuTest *tc)
{
  struct artificer_fixture f;
  bool created_commands = complete_cmd_info == NULL;
  char create_cancel[] = "device create cancel";
  char repair_cancel[] = "device repair cancel";
  char list_command[] = "device list";
  bool list_waited, creating, repairing;

  begin_artificer(&f, 1);
  if (created_commands)
    create_command_list();
  attach_mud_event(new_mud_event(eDEVICE_CREATION, &f.ch, "1:1|1|24|0"),
                   (long)600 * PASSES_PER_SEC);
  clear_output(&f);
  command_interpreter(&f.ch, list_command);
  list_waited = strstr(f.descriptor.output, "too busy devising") != NULL;
  command_interpreter(&f.ch, create_cancel);
  creating = char_has_mud_event(&f.ch, eDEVICE_CREATION) != NULL;
  attach_mud_event(new_mud_event(eDEVICE_REPAIR, &f.ch, "0"), (long)600 * PASSES_PER_SEC);
  command_interpreter(&f.ch, repair_cancel);
  repairing = char_has_mud_event(&f.ch, eDEVICE_REPAIR) != NULL;
  if (created_commands)
    free_command_list();
  end_artificer(&f);

  CuAssertTrue(tc, list_waited);
  CuAssertTrue(tc, !creating);
  CuAssertTrue(tc, !repairing);
}

/* Metamagic Science on a wand spends one extra charge per level the metamagic adds; it used to
 * spend one charge whatever was applied. */
void Test_metamagic_science_wand_spends_a_charge_per_added_level(CuTest *tc)
{
  struct artificer_fixture f;
  struct obj_data wand;
  char argument[64];
  int refused_charges, spent_charges;
  bool armored;

  begin_artificer(&f, 11);
  SET_FEAT(&f.ch, FEAT_METAMAGIC_SCIENCE, 1);
  SET_FEAT(&f.ch, FEAT_EXTEND_SPELL, 1);
  memset(&wand, 0, sizeof(wand));
  wand.name = CuMutableString("wand");
  wand.short_description = CuMutableString("a test wand");
  GET_OBJ_TYPE(&wand) = ITEM_WAND;
  GET_OBJ_VAL(&wand, 0) = 5;
  GET_OBJ_VAL(&wand, 1) = 2;
  GET_OBJ_VAL(&wand, 2) = 1;
  GET_OBJ_VAL(&wand, 3) = SPELL_MAGE_ARMOR;
  strlcpy(argument, "extended zzartificer", sizeof(argument));
  mag_objectmagic(&f.ch, &wand, argument);
  refused_charges = GET_OBJ_VAL(&wand, 2);
  GET_OBJ_VAL(&wand, 2) = 2;
  strlcpy(argument, "extended zzartificer", sizeof(argument));
  mag_objectmagic(&f.ch, &wand, argument);
  spent_charges = GET_OBJ_VAL(&wand, 2);
  armored = affected_by_spell(&f.ch, SPELL_MAGE_ARMOR);
  end_artificer(&f);

  CuAssertIntEquals(tc, 1, refused_charges);
  CuAssertIntEquals(tc, 0, spent_charges);
  CuAssertTrue(tc, armored);
}

/* Improved Metamagic Science: skill_check() returns 0 on a failed check, which the code read as
 * a pass, and stored potions built the DC from an unset spell level (99). The DC is now
 * 20 + 3 x (circle + metamagic levels): 26 for an extended mage armor. */
void Test_improved_metamagic_science_potion_check_can_fail_and_pass(CuTest *tc)
{
  struct artificer_fixture f;
  struct char_data *ch = &f.ch;
  int failed_left, passed_left;

  begin_artificer(&f, 11);
  SET_FEAT(ch, FEAT_IMPROVED_METAMAGIC_SCIENCE, 1);
  SET_FEAT(ch, FEAT_EXTEND_SPELL, 1);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_USE_STORED_CONSUMABLES);
  STORED_POTIONS(ch, SPELL_MAGE_ARMOR) = 2;
  do_use_consumable(ch, "extended mage armor", 0, SCMD_QUAFF);
  failed_left = STORED_POTIONS(ch, SPELL_MAGE_ARMOR);
  SET_ABILITY(ch, ABILITY_USE_MAGIC_DEVICE, 40);
  do_use_consumable(ch, "extended mage armor", 0, SCMD_QUAFF);
  passed_left = STORED_POTIONS(ch, SPELL_MAGE_ARMOR);
  end_artificer(&f);

  CuAssertIntEquals(tc, 2, failed_left);
  CuAssertIntEquals(tc, 1, passed_left);
}

/* Elbow Grease and Jack of All Trades reached compute_ability() only, while craft, harvest and
 * golem checks read raw ranks. Rank bookkeeping still reads the raw ranks. */
void Test_artificer_craft_rolls_add_elbow_grease_and_jack_of_all_trades(CuTest *tc)
{
  struct artificer_fixture f;
  int ranks, craft_roll, arcana_roll;

  begin_artificer(&f, 10);
  SET_FEAT(&f.ch, FEAT_ELBOW_GREASE, 1);
  SET_FEAT(&f.ch, FEAT_JACK_OF_ALL_TRADES, 1);
  SET_ABILITY(&f.ch, ABILITY_CRAFT_ALCHEMY, 7);
  SET_ABILITY(&f.ch, ABILITY_ARCANA, 4);
  ranks = get_craft_skill_value(&f.ch, ABILITY_CRAFT_ALCHEMY);
  craft_roll = get_craft_roll_value(&f.ch, ABILITY_CRAFT_ALCHEMY);
  arcana_roll = get_craft_roll_value(&f.ch, ABILITY_ARCANA);
  end_artificer(&f);

  CuAssertIntEquals(tc, 7, ranks);
  CuAssertIntEquals(tc, 7 + 6 + 3, craft_roll);
  CuAssertIntEquals(tc, 4 + 3, arcana_roll);
}

/* The premade artificer bought Craft Magical Arms and Armor and Craft Wonderous Item, which the
 * class grants free at 5 and 4, and a human bought Craft Wonderous Item twice. */
void Test_artificer_premade_build_skips_feats_the_class_grants(CuTest *tc)
{
  struct artificer_fixture f;
  struct char_data *ch = &f.ch;
  int level, craft_feats;
  bool aptitude, initiative, empower;

  if (race_list[RACE_HUMAN].name == NULL)
    assign_races();
  if (feat_list[FEAT_MAGICAL_APTITUDE].name == NULL)
    assign_feats();
  begin_artificer(&f, 1);
  GET_REAL_RACE(ch) = RACE_HUMAN;
  GET_PREMADE_BUILD_CLASS(ch) = CLASS_ARTIFICER;
  for (level = 1; level <= 3; level++)
  {
    GET_LEVEL(ch) = level;
    CLASS_LEVEL(ch, CLASS_ARTIFICER) = level;
    advance_premade_build(ch);
    clear_output(&f);
  }
  craft_feats = HAS_REAL_FEAT(ch, FEAT_CRAFT_WONDEROUS_ITEM) +
                HAS_REAL_FEAT(ch, FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR);
  aptitude = HAS_REAL_FEAT(ch, FEAT_MAGICAL_APTITUDE) == 1;
  initiative = HAS_REAL_FEAT(ch, FEAT_IMPROVED_INITIATIVE) == 1;
  empower = HAS_REAL_FEAT(ch, FEAT_EMPOWER_SPELL) == 1;
  end_artificer(&f);

  CuAssertIntEquals(tc, 0, craft_feats);
  CuAssertTrue(tc, aptitude);
  CuAssertTrue(tc, initiative);
  CuAssertTrue(tc, empower);
}

/* Construct Iron Golem was granted at artificer 30 on a class that stops at 20. Every artificer
 * grant must be reachable; the golems come at 10, 15 and 20. */
void Test_artificer_class_feats_are_reachable(CuTest *tc)
{
  struct class_feat_assign *assign;
  int stone = 0, iron = 0, unreachable = 0;

  if (class_list[CLASS_ARTIFICER].name == NULL)
    load_class_list();
  for (assign = class_list[CLASS_ARTIFICER].featassign_list; assign; assign = assign->next)
  {
    if (assign->level_received > class_list[CLASS_ARTIFICER].max_level)
      unreachable = assign->feat_num;
    if (assign->feat_num == FEAT_CONSTRUCT_STONE_GOLEM)
      stone = assign->level_received;
    if (assign->feat_num == FEAT_CONSTRUCT_IRON_GOLEM)
      iron = assign->level_received;
  }

  CuAssertIntEquals(tc, 0, unreachable);
  CuAssertIntEquals(tc, 15, stone);
  CuAssertIntEquals(tc, 20, iron);
}

/* Brilliance and Blunder belonged to the retired Dragonlance gnome and no race granted it; gnomes
 * now get it beside Gnomish Tinkering. */
void Test_gnomes_get_brilliance_and_blunder(CuTest *tc)
{
  struct race_feat_assign *assign;
  int level = 0;

  if (race_list[RACE_GNOME].name == NULL)
    assign_races();
  for (assign = race_list[RACE_GNOME].featassign_list; assign; assign = assign->next)
    if (assign->feat_num == FEAT_BRILLIANCE_AND_BLUNDER)
      level = assign->level_received;

  CuAssertIntEquals(tc, 1, level);
}

/* Artificer Item Creation was checked nowhere. An artificer with it brews any wizard or cleric
 * spell of a circle it can put in a device, and no higher. */
void Test_artificer_item_creation_brews_device_spells(CuTest *tc)
{
  struct artificer_fixture f;
  bool without_feat, armor, stoneskin, skill, armor_refused, stoneskin_refused;
  char armor_command[MAX_INPUT_LENGTH], stoneskin_command[MAX_INPUT_LENGTH];

  begin_artificer(&f, 5);
  snprintf(armor_command, sizeof(armor_command), "'%s'", spell_info[SPELL_MAGE_ARMOR].name);
  snprintf(stoneskin_command, sizeof(stoneskin_command), "'%s'", spell_info[SPELL_STONESKIN].name);
  without_feat = artificer_can_emulate_spell(&f.ch, SPELL_MAGE_ARMOR);
  SET_FEAT(&f.ch, FEAT_ARTIFICER_ITEM_CREATION, 1);
  armor = artificer_can_emulate_spell(&f.ch, SPELL_MAGE_ARMOR);
  stoneskin = artificer_can_emulate_spell(&f.ch, SPELL_STONESKIN);
  skill = artificer_can_emulate_spell(&f.ch, SKILL_KICK);
  clear_output(&f);
  do_brew(&f.ch, armor_command, 0, 0);
  armor_refused = strstr(f.descriptor.output, "know how to cast") != NULL;
  clear_output(&f);
  do_brew(&f.ch, stoneskin_command, 0, 0);
  stoneskin_refused = strstr(f.descriptor.output, "know how to cast") != NULL;
  end_artificer(&f);

  CuAssertTrue(tc, !without_feat);
  CuAssertTrue(tc, armor);
  CuAssertTrue(tc, !stoneskin);
  CuAssertTrue(tc, !skill);
  CuAssertTrue(tc, !armor_refused);
  CuAssertTrue(tc, stoneskin_refused);
}

/** An isolated player directory, so saves never touch lib/plrfiles. */
struct device_player_files
{
  char temporary_directory[64];
  char directory[PATH_MAX];
  char name[32];
  struct player_index_element index[1];
  struct player_index_element *saved_table;
  int saved_top;
};

static void device_player_files_enter(CuTest *tc, struct device_player_files *files, long id)
{
  memset(files, 0, sizeof(*files));
  snprintf(files->temporary_directory, sizeof(files->temporary_directory),
           "/tmp/luminari-artificer-XXXXXX");
  snprintf(files->name, sizeof(files->name), "Zzdevice%ld", (long)getpid());
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

static int device_player_files_leave(struct device_player_files *files)
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

/* A broken device used to come back working and fully charged after a relog: neither the broken
 * flag nor the DC penalty was saved. Files saved before them load both as 0. */
void Test_artificer_device_save_keeps_broken_and_penalty(CuTest *tc)
{
  struct device_player_files files;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  struct char_data *old_format = new_char();
  struct player_invention *device;
  char filename[MAX_FILEPATH];
  FILE *file;
  int saved, result, old_result, penalty, old_penalty, uses;
  bool broken, old_broken;

  device_player_files_enter(tc, &files, 4401);
  ch->player.name = strdup(files.name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4401;
  GET_LEVEL(ch) = 10;
  ch->player_specials->saved.num_inventions = 1;
  device = &ch->player_specials->saved.inventions[0];
  strlcpy(device->keywords, "shield device", sizeof(device->keywords));
  strlcpy(device->short_description, "a shield device", sizeof(device->short_description));
  strlcpy(device->long_description, "A shield device is here.", sizeof(device->long_description));
  device->num_spells = 1;
  device->spell_effects[0] = SPELL_SHIELD;
  device->spell_levels[0] = 1;
  device->dc_penalty = 12;
  device->broken = TRUE;
  saved = save_char_checked(ch, 0);
  result = load_char(files.name, loaded);
  penalty = loaded->player_specials->saved.inventions[0].dc_penalty;
  broken = loaded->player_specials->saved.inventions[0].broken;
  uses = loaded->player_specials->saved.inventions[0].uses;

  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, files.name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  if (file != NULL)
  {
    fprintf(file,
            "Name: %s\nId  : 4401\nLevl: 7\nDvis:\n1\n0\n kit\n a kit\n A kit lies here.\n"
            "1 24 0 2 0\n%d\n-1\n-1\n-1\n-1\n~\n",
            files.name, SPELL_SHIELD);
    fclose(file);
  }
  old_result = load_char(files.name, old_format);
  old_penalty = old_format->player_specials->saved.inventions[0].dc_penalty;
  old_broken = old_format->player_specials->saved.inventions[0].broken;
  free_char(ch);
  free_char(loaded);
  free_char(old_format);
  CuAssertIntEquals(tc, 0, device_player_files_leave(&files));

  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 12, penalty);
  CuAssertTrue(tc, broken);
  CuAssertIntEquals(tc, 0, uses);
  CuAssertIntEquals(tc, 0, old_result);
  CuAssertIntEquals(tc, 0, old_penalty);
  CuAssertTrue(tc, !old_broken);
}
