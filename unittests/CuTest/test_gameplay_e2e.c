#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/actionqueues.h"
#include "../../src/craft.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/fight.h"
#include "../../src/genwld.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/mob/mob_utils.h"
#include "../../src/missions.h"
#include "../../src/movement/movement.h"
#include "../../src/perks.h"
#include "../../src/protocol.h"
#include "../../src/spells.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct gameplay_fixture
{
  struct room_data rooms[2];
  struct room_direction_data exits[2];
  struct zone_data zones[1];
  struct index_data mobile_index[1];
  struct char_data actor;
  struct char_data victim;

  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
};

struct set_class_field_case
{
  const char *field;
  int class_num;
};

static const struct set_class_field_case set_class_field_cases[] = {
    {"wizard", CLASS_WIZARD},
    {"cleric", CLASS_CLERIC},
    {"rogue", CLASS_ROGUE},
    {"warrior", CLASS_WARRIOR},
    {"monk", CLASS_MONK},
    {"druid", CLASS_DRUID},
    {"berserker", CLASS_BERSERKER},
    {"sorcerer", CLASS_SORCERER},
    {"paladin", CLASS_PALADIN},
    {"ranger", CLASS_RANGER},
    {"bard", CLASS_BARD},
    {"weaponmaster", CLASS_WEAPON_MASTER},
    {"arcanearcher", CLASS_ARCANE_ARCHER},
    {"stalwartdefender", CLASS_STALWART_DEFENDER},
    {"shifter", CLASS_SHIFTER},
    {"duelist", CLASS_DUELIST},
    {"mystictheurge", CLASS_MYSTIC_THEURGE},
    {"alchemist", CLASS_ALCHEMIST},
    {"arcaneshadow", CLASS_ARCANE_SHADOW},
    {"sacredfist", CLASS_SACRED_FIST},
    {"eldritchknight", CLASS_ELDRITCH_KNIGHT},
    {"psionicist", CLASS_PSIONICIST},
    {"spellsword", CLASS_SPELLSWORD},
    {"shadowdancer", CLASS_SHADOW_DANCER},
    {"blackguard", CLASS_BLACKGUARD},
    {"assassin", CLASS_ASSASSIN},
    {"inquisitor", CLASS_INQUISITOR},
    {"summoner", CLASS_SUMMONER},
    {"warlock", CLASS_WARLOCK},
    {"necromancer", CLASS_NECROMANCER},
    {"knightoftheluminousthread", CLASS_KNIGHT_OF_SOLAMNIA},
    {"knightoftheshatteredmirror", CLASS_KNIGHT_OF_THE_THORN},
    {"knightofthepalethrone", CLASS_KNIGHT_OF_THE_SKULL},
    {"knightofthehowlingmoon", CLASS_KNIGHT_OF_THE_LILY},
    {"dragonrider", CLASS_DRAGONRIDER},
    {"artificer", CLASS_ARTIFICER},
};

static const char *test_source_root(void)
{
  const char *root;

  root = getenv("LUMINARI_TEST_ROOT");
  return root != NULL && *root != '\0' ? root : ".";
}

static bool rewrite_psychic_sundering_as_legacy(const char *filename)
{
  FILE *input;
  FILE *output;
  char line[MAX_STRING_LENGTH];
  char temp_filename[MAX_FILEPATH + 16];
  char *affect_fields;
  int affect_id;
  bool in_affects;
  bool saw_affect_version;
  bool saw_psychic_sundering;
  bool write_ok;

  if (snprintf(temp_filename, sizeof(temp_filename), "%s.legacy", filename) >=
      (int)sizeof(temp_filename))
    return false;

  input = fopen(filename, "r");
  if (input == NULL)
    return false;

  output = fopen(temp_filename, "w");
  if (output == NULL)
  {
    fclose(input);
    return false;
  }

  in_affects = false;
  saw_affect_version = false;
  saw_psychic_sundering = false;
  write_ok = true;

  while (fgets(line, sizeof(line), input) != NULL)
  {
    if (strncmp(line, "Affs:", 5) == 0)
    {
      if (fputs("Affs: 0\n", output) == EOF)
      {
        write_ok = false;
        break;
      }
      in_affects = true;
      saw_affect_version = true;
      continue;
    }

    if (in_affects && sscanf(line, "%d", &affect_id) == 1)
    {
      if (affect_id == AFFECT_PSIONICIST_PSYCHIC_SUNDERING)
      {
        affect_fields = strchr(line, ' ');
        if (affect_fields == NULL ||
            fprintf(output, "%d%s", PERK_PSIONICIST_PSYCHIC_SUNDERING, affect_fields) < 0)
        {
          write_ok = false;
          break;
        }
        saw_psychic_sundering = true;
        continue;
      }
      if (affect_id == 0)
        in_affects = false;
    }

    if (fputs(line, output) == EOF)
    {
      write_ok = false;
      break;
    }
  }

  if (ferror(input) || fflush(output) != 0)
    write_ok = false;
  if (fclose(input) != 0)
    write_ok = false;
  if (fclose(output) != 0)
    write_ok = false;

  if (write_ok && saw_affect_version && saw_psychic_sundering &&
      rename(temp_filename, filename) == 0)
    return true;

  unlink(temp_filename);
  return false;
}

static bool remove_boarding_ability_version(const char *filename)
{
  FILE *input;
  FILE *output;
  char line[MAX_STRING_LENGTH];
  char temp_filename[MAX_FILEPATH + 16];
  bool saw_version;
  bool write_ok;

  if (snprintf(temp_filename, sizeof(temp_filename), "%s.legacy", filename) >=
      (int)sizeof(temp_filename))
    return false;

  input = fopen(filename, "r");
  if (input == NULL)
    return false;

  output = fopen(temp_filename, "w");
  if (output == NULL)
  {
    fclose(input);
    return false;
  }

  saw_version = false;
  write_ok = true;
  while (fgets(line, sizeof(line), input) != NULL)
  {
    if (strncmp(line, "BrdV:", 5) == 0)
    {
      saw_version = true;
      continue;
    }
    if (fputs(line, output) == EOF)
    {
      write_ok = false;
      break;
    }
  }

  if (ferror(input) || fflush(output) != 0)
    write_ok = false;
  if (fclose(input) != 0)
    write_ok = false;
  if (fclose(output) != 0)
    write_ok = false;

  if (write_ok && saw_version && rename(temp_filename, filename) == 0)
    return true;

  unlink(temp_filename);
  return false;
}

static void initialize_test_npc(struct char_data *ch, const char *name, room_rnum room)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = (char *)name;
  GET_LEVEL(ch) = 10;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;
  GET_MAX_HIT(ch) = 100;
  GET_MOVE(ch) = 100;
  GET_MAX_MOVE(ch) = 100;
  IN_ROOM(ch) = room;
}

static void begin_gameplay_fixture(struct gameplay_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  fixture->rooms[0].number = 100;
  fixture->rooms[0].zone = 0;
  fixture->rooms[0].sector_type = SECT_INSIDE;
  fixture->rooms[0].name = "End-to-end origin";
  fixture->rooms[0].description = "A production-linked test room.\r\n";
  fixture->rooms[1].number = 101;
  fixture->rooms[1].zone = 0;
  fixture->rooms[1].sector_type = SECT_INSIDE;
  fixture->rooms[1].name = "End-to-end destination";
  fixture->rooms[1].description = "A second production-linked test room.\r\n";
  fixture->rooms[0].trail_tracks = calloc(1, sizeof(*fixture->rooms[0].trail_tracks));
  fixture->rooms[1].trail_tracks = calloc(1, sizeof(*fixture->rooms[1].trail_tracks));

  fixture->exits[0].key = NOTHING;
  fixture->exits[0].to_room = 1;
  fixture->exits[1].key = NOTHING;
  fixture->exits[1].to_room = 0;
  fixture->rooms[0].dir_option[NORTH] = &fixture->exits[0];
  fixture->rooms[1].dir_option[SOUTH] = &fixture->exits[1];

  fixture->zones[0].number = 0;
  fixture->zones[0].bot = 100;
  fixture->zones[0].top = 101;
  fixture->zones[0].min_level = -1;
  fixture->zones[0].max_level = LVL_IMPL;

  fixture->mobile_index[0].vnum = 1;

  world = fixture->rooms;
  top_of_world = 1;
  zone_table = fixture->zones;
  top_of_zone_table = 0;
  mob_index = fixture->mobile_index;
  top_of_mobt = 0;

  initialize_test_npc(&fixture->actor, "fixture actor", 0);
  initialize_test_npc(&fixture->victim, "fixture victim", 0);
  fixture->rooms[0].people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->victim;
}

static void end_gameplay_fixture(struct gameplay_fixture *fixture)
{
  FIGHTING(&fixture->actor) = NULL;
  FIGHTING(&fixture->victim) = NULL;
  fixture->actor.last_attacker = NULL;
  fixture->victim.last_attacker = NULL;
  fixture->actor.next_in_room = NULL;
  fixture->victim.next_in_room = NULL;
  fixture->rooms[0].people = NULL;
  fixture->rooms[1].people = NULL;
  while (fixture->actor.affected != NULL)
    affect_remove_no_total(&fixture->actor, fixture->actor.affected);
  while (fixture->victim.affected != NULL)
    affect_remove_no_total(&fixture->victim, fixture->victim.affected);
  clear_repulsion_lists(&fixture->actor);
  clear_repulsion_lists(&fixture->victim);
  if (fixture->rooms[0].trail_tracks != NULL)
    free_trail_data_list(fixture->rooms[0].trail_tracks);
  if (fixture->rooms[1].trail_tracks != NULL)
    free_trail_data_list(fixture->rooms[1].trail_tracks);

  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

void Test_gameplay_e2e_harvest_uses_wilderness_only_as_fallback(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct descriptor_data descriptor;
  bool fallback_used;
  bool legacy_error_preserved;
  bool legacy_error_suppressed;

  begin_gameplay_fixture(&fixture);
  memset(&descriptor, 0, sizeof(descriptor));
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &fixture.actor;
  descriptor.pProtocol = ProtocolCreate();
  fixture.actor.desc = &descriptor;

  if (descriptor.pProtocol == NULL)
  {
    fixture.actor.desc = NULL;
    end_gameplay_fixture(&fixture);
    CuFail(tc, "could not initialize the harvest fallback fixture");
    return;
  }

  do_harvest(&fixture.actor, "not-a-resource", 0, 0);
  legacy_error_preserved =
      strstr(descriptor.output, "That doesn't seem to be present in this room.") != NULL;

  descriptor.small_outbuf[0] = '\0';
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  SET_BIT_AR(fixture.zones[0].zone_flags, ZONE_WILDERNESS);

  do_harvest(&fixture.actor, "not-a-resource", 0, 0);
  fallback_used = strstr(descriptor.output, "Invalid resource type.") != NULL;
  legacy_error_suppressed =
      strstr(descriptor.output, "That doesn't seem to be present in this room.") == NULL;

  fixture.actor.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, legacy_error_preserved);
  CuAssertTrue(tc, fallback_used);
  CuAssertTrue(tc, legacy_error_suppressed);
}

void Test_gameplay_e2e_set_supports_every_playable_class(CuTest *tc)
{
  struct char_data *staff;
  struct char_data *victim;
  size_t class_field_count;
  size_t i;
  int j;
  int result;

  class_field_count = sizeof(set_class_field_cases) / sizeof(set_class_field_cases[0]);
  CuAssertIntEquals(tc, CLASS_PLACEHOLDER_1, (int)class_field_count);

  staff = new_char();
  victim = new_char();
  CuAssertPtrNotNull(tc, staff);
  CuAssertPtrNotNull(tc, victim);

  staff->player.name = strdup("set class staff");
  victim->player.name = strdup("set class victim");
  GET_LEVEL(staff) = LVL_IMPL;
  GET_LEVEL(victim) = 1;
  GET_REAL_RACE(victim) = RACE_HUMAN;
  GET_REAL_STR(victim) = 10;
  GET_REAL_DEX(victim) = 10;
  GET_REAL_CON(victim) = 10;
  GET_REAL_INT(victim) = 10;
  GET_REAL_WIS(victim) = 10;
  GET_REAL_CHA(victim) = 10;

  for (i = 0; i < class_field_count; i++)
  {
    CuAssertIntEquals(tc, (int)i, set_class_field_cases[i].class_num);
    for (j = 0; j < NUM_CLASSES; j++)
      CLASS_LEVEL(victim, j) = 0;

    result = perform_set_class_level_for_test(staff, victim, set_class_field_cases[i].field, 7);
    CuAssertIntEquals(tc, 1, result);
    CuAssertIntEquals(tc, 7, CLASS_LEVEL(victim, set_class_field_cases[i].class_num));

    for (j = 0; j < NUM_CLASSES; j++)
    {
      if (j != set_class_field_cases[i].class_num)
        CuAssertIntEquals(tc, 0, CLASS_LEVEL(victim, j));
    }
  }

  free_char(staff);
  free_char(victim);
}

void Test_gameplay_e2e_combat_applies_real_damage(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int damage_result;
  int remaining_hit_points;

  begin_gameplay_fixture(&fixture);
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;

  damage_result = damage(&fixture.actor, &fixture.victim, 12, TYPE_HIT, DAM_BLUDGEON, FALSE);
  remaining_hit_points = GET_HIT(&fixture.victim);

  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, damage_result > 0);
  CuAssertTrue(tc, remaining_hit_points < 100);
  CuAssertTrue(tc, remaining_hit_points > 0);
}

void Test_gameplay_e2e_staff_all_feats_melee_rotation_executes(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data *staff;
  int expected_attacks;
  int attempted_attacks;
  int remaining_hit_points;
  int i;

  begin_gameplay_fixture(&fixture);
  staff = new_char();
  staff->player.name = strdup("staff rotation fixture");
  GET_QUEUE(staff) = create_action_queue();
  GET_ATTACK_QUEUE(staff) = create_attack_queue();
  GET_IDNUM(staff) = 4243;
  GET_LEVEL(staff) = LVL_IMPL;
  GET_CLASS(staff) = CLASS_WARRIOR;
  GET_REAL_RACE(staff) = RACE_HUMAN;
  GET_POS(staff) = POS_FIGHTING;
  GET_HIT(staff) = 100000;
  GET_MAX_HIT(staff) = 100000;
  GET_MOVE(staff) = 100000;
  GET_MAX_MOVE(staff) = 100000;
  GET_REAL_STR(staff) = 39;
  staff->aff_abils.str = 39;
  GET_REAL_DEX(staff) = 25;
  staff->aff_abils.dex = 25;
  GET_REAL_CON(staff) = 25;
  staff->aff_abils.con = 25;
  GET_REAL_INT(staff) = 25;
  GET_INT(staff) = 25;
  GET_REAL_WIS(staff) = 25;
  GET_WIS(staff) = 25;
  GET_REAL_CHA(staff) = 25;
  GET_CHA(staff) = 25;
  IN_ROOM(staff) = 0;

  for (i = 0; i < MAX_CLASSES; i++)
    CLASS_LEVEL(staff, i) = 30;
  for (i = 1; i < FEAT_LAST_FEAT; i++)
    SET_FEAT(staff, i, 1);

  GET_HIT(&fixture.victim) = 100000;
  GET_MAX_HIT(&fixture.victim) = 100000;
  GET_LEVEL(&fixture.victim) = LVL_IMPL;
  GET_REAL_RACE(&fixture.victim) = RACE_TYPE_UNDEAD;
  fixture.rooms[0].people = staff;
  staff->next_in_room = &fixture.victim;
  fixture.victim.next_in_room = NULL;
  FIGHTING(staff) = &fixture.victim;
  FIGHTING(&fixture.victim) = staff;

#define RETURN_NUM_ATTACKS 1
  expected_attacks = perform_attacks(staff, RETURN_NUM_ATTACKS, 0);
#undef RETURN_NUM_ATTACKS
  SET_BIT_AR(PRF_FLAGS(staff), PRF_CONDENSED);
  init_condensed_combat_data(staff);
#define NORMAL_ATTACK_ROUTINE 0
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 1);
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 2);
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 3);
#undef NORMAL_ATTACK_ROUTINE
  attempted_attacks = CNDNSD(staff)->num_times_attacking;
  remaining_hit_points = GET_HIT(&fixture.victim);

  FIGHTING(staff) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  staff->next_in_room = NULL;
  fixture.rooms[0].people = &fixture.actor;
  fixture.actor.next_in_room = &fixture.victim;
  fixture.victim.next_in_room = NULL;
  free_char(staff);
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, expected_attacks > 0);
  CuAssertIntEquals(tc, expected_attacks, attempted_attacks);
  CuAssertTrue(tc, remaining_hit_points < 100000);
}

void Test_gameplay_e2e_repulsion_tracks_and_allows_melee_attackers(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool attacker_tracked;
  int blocked_hit_points;
  int remaining_hit_points;

  begin_gameplay_fixture(&fixture);
  GET_ATTACK_QUEUE(&fixture.actor) = create_attack_queue();
  GET_HIT(&fixture.victim) = 100000;
  GET_MAX_HIT(&fixture.victim) = 100000;
  GET_POS(&fixture.victim) = POS_SLEEPING;
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_REPULSION);

  hit(&fixture.actor, &fixture.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);

  CuAssertPtrNotNull(tc, fixture.victim.char_specials.repulse_blacklist);
  CuAssertPtrNotNull(tc, fixture.victim.char_specials.repulse_whitelist);
  attacker_tracked =
      find_in_list(&fixture.actor, fixture.victim.char_specials.repulse_blacklist) != NULL ||
      find_in_list(&fixture.actor, fixture.victim.char_specials.repulse_whitelist) != NULL;

  if (find_in_list(&fixture.actor, fixture.victim.char_specials.repulse_blacklist) != NULL)
    remove_from_list(&fixture.actor, fixture.victim.char_specials.repulse_blacklist);
  if (find_in_list(&fixture.actor, fixture.victim.char_specials.repulse_whitelist) != NULL)
    remove_from_list(&fixture.actor, fixture.victim.char_specials.repulse_whitelist);
  add_to_list(&fixture.actor, fixture.victim.char_specials.repulse_blacklist);

  GET_HIT(&fixture.victim) = 100000;
  GET_POS(&fixture.victim) = POS_SLEEPING;
  hit(&fixture.actor, &fixture.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
  blocked_hit_points = GET_HIT(&fixture.victim);

  remove_from_list(&fixture.actor, fixture.victim.char_specials.repulse_blacklist);
  add_to_list(&fixture.actor, fixture.victim.char_specials.repulse_whitelist);

  GET_HIT(&fixture.victim) = 100000;
  GET_POS(&fixture.victim) = POS_SLEEPING;
  hit(&fixture.actor, &fixture.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
  remaining_hit_points = GET_HIT(&fixture.victim);

  clear_repulsion_lists(&fixture.victim);
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_REPULSION);
  free_attack_queue(GET_ATTACK_QUEUE(&fixture.actor));
  GET_ATTACK_QUEUE(&fixture.actor) = NULL;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, attacker_tracked);
  CuAssertIntEquals(tc, 100000, blocked_hit_points);
  CuAssertTrue(tc, remaining_hit_points < 100000);
}

void Test_gameplay_e2e_repulsion_initializes_and_cleans_target_state(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool target_affected;
  bool caster_lists_untouched;
  bool target_lists_initialized;
  bool target_state_cleared;

  begin_gameplay_fixture(&fixture);
  mag_affects(10, &fixture.actor, &fixture.victim, NULL, SPELL_REPULSION, SAVING_WILL, CAST_SPELL,
              0);

  target_affected = AFF_FLAGGED(&fixture.victim, AFF_REPULSION);
  caster_lists_untouched = fixture.actor.char_specials.repulse_blacklist == NULL &&
                           fixture.actor.char_specials.repulse_whitelist == NULL;
  target_lists_initialized = fixture.victim.char_specials.repulse_blacklist != NULL &&
                             fixture.victim.char_specials.repulse_whitelist != NULL;

  affect_remove(&fixture.victim, fixture.victim.affected);
  target_state_cleared = !AFF_FLAGGED(&fixture.victim, AFF_REPULSION) &&
                         fixture.victim.char_specials.repulse_blacklist == NULL &&
                         fixture.victim.char_specials.repulse_whitelist == NULL;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, target_affected);
  CuAssertTrue(tc, caster_lists_untouched);
  CuAssertTrue(tc, target_lists_initialized);
  CuAssertTrue(tc, target_state_cleared);
}

void Test_gameplay_e2e_casting_dispatches_magic_missile(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int cast_result;
  int remaining_hit_points;

  begin_gameplay_fixture(&fixture);
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_UNLIMITED_SPELL_SLOTS);
  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();

  cast_result =
      call_magic(&fixture.actor, &fixture.victim, NULL, SPELL_MAGIC_MISSILE, 0, 10, CAST_INNATE);
  remaining_hit_points = GET_HIT(&fixture.victim);

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, cast_result);
  CuAssertTrue(tc, remaining_hit_points < 100);
  CuAssertTrue(tc, remaining_hit_points > 0);
}

void Test_gameplay_e2e_movement_changes_room(CuTest *tc)
{
  struct gameplay_fixture fixture;
  int move_result;
  room_rnum destination;

  begin_gameplay_fixture(&fixture);

  move_result = perform_move(&fixture.actor, NORTH, FALSE);
  destination = IN_ROOM(&fixture.actor);

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, move_result);
  CuAssertIntEquals(tc, 1, destination);
}

void Test_gameplay_e2e_movement_trail_statistics_follow_live_world(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data player_specials;
  size_t initial_trails;
  size_t final_trails;
  int move_result;

  begin_gameplay_fixture(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &player_specials;
  fixture.actor.player.name = "fixture player";

  initial_trails = count_live_movement_trails();
  move_result = perform_move(&fixture.actor, NORTH, FALSE);
  final_trails = count_live_movement_trails();

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 0, (int)initial_trails);
  CuAssertIntEquals(tc, 1, move_result);
  CuAssertIntEquals(tc, 1, (int)final_trails);
}

void Test_gameplay_e2e_npc_movement_does_not_retain_trails(CuTest *tc)
{
  struct gameplay_fixture fixture;
  size_t initial_trails;
  size_t final_trails;
  int move_result;

  begin_gameplay_fixture(&fixture);

  initial_trails = count_live_movement_trails();
  move_result = perform_move(&fixture.actor, NORTH, FALSE);
  final_trails = count_live_movement_trails();

  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 0, (int)initial_trails);
  CuAssertIntEquals(tc, 1, move_result);
  CuAssertIntEquals(tc, 0, (int)final_trails);
}

void Test_gameplay_e2e_movement_trails_refresh_and_remain_bounded(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct trail_data *trail;
  char name[32];
  size_t trail_count;
  int i;

  begin_gameplay_fixture(&fixture);

  movement_trail_record(fixture.rooms[0].trail_tracks, "repeat walker", "human", DIR_NONE, NORTH,
                        100);
  movement_trail_record(fixture.rooms[0].trail_tracks, "repeat walker", "human", DIR_NONE, NORTH,
                        200);
  trail_count = count_live_movement_trails();
  CuAssertIntEquals(tc, 1, (int)trail_count);
  CuAssertIntEquals(tc, 200, (int)fixture.rooms[0].trail_tracks->head->age);

  for (i = 0; i < TRAIL_MAX_PER_ROOM + 5; i++)
  {
    snprintf(name, sizeof(name), "walker %d", i);
    movement_trail_record(fixture.rooms[0].trail_tracks, name, "human", DIR_NONE, NORTH, 300 + i);
  }
  trail_count = count_live_movement_trails();
  trail = fixture.rooms[0].trail_tracks->head;

  CuAssertIntEquals(tc, TRAIL_MAX_PER_ROOM, (int)trail_count);
  CuAssertTrue(tc, trail != NULL);
  CuAssertIntEquals(tc, 300 + TRAIL_MAX_PER_ROOM + 4, (int)trail->age);
  CuAssertTrue(tc, fixture.rooms[0].trail_tracks->tail != NULL);

  end_gameplay_fixture(&fixture);
}

void Test_gameplay_e2e_command_dispatch_reaches_movement(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool created_command_list;
  room_rnum destination;
  char command[] = "north";

  begin_gameplay_fixture(&fixture);
  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }

  command_interpreter(&fixture.actor, command);
  destination = IN_ROOM(&fixture.actor);

  if (created_command_list)
    free_command_list();
  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 1, destination);
}

void Test_gameplay_e2e_winters_war_march_recovery_covers_failed_save_slow(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct affected_type forced_failure;
  struct affected_type *effect;
  struct char_data *bard;
  struct char_data *saved_character_list;
  int recovery_duration;
  int slow_duration;
  int update;
  bool recovery_never_preceded_slow;
  bool recovery_removed;
  bool slow_removed;

  begin_gameplay_fixture(&fixture);
  bard = new_char();
  bard->player.name = strdup("winter war march test bard");
  GET_CLASS(bard) = CLASS_BARD;
  CLASS_LEVEL(bard, CLASS_BARD) = 20;
  GET_LEVEL(bard) = 20;
  GET_POS(bard) = POS_STANDING;
  GET_HIT(bard) = 100;
  GET_MAX_HIT(bard) = 100;
  GET_MOVE(bard) = 100;
  GET_MAX_MOVE(bard) = 100;
  IN_ROOM(bard) = 0;
  IS_PERFORMING(bard) = 1;
  GET_ATTACK_QUEUE(bard) = create_attack_queue();
  add_char_perk(bard, PERK_BARD_WINTERS_WAR_MARCH, CLASS_BARD);

  fixture.rooms[0].people = bard;
  bard->next_in_room = &fixture.victim;
  fixture.actor.next_in_room = NULL;
  fixture.victim.next_in_room = NULL;
  GET_HIT(&fixture.victim) = 100000;
  GET_MAX_HIT(&fixture.victim) = 100000;
  GET_POS(&fixture.victim) = POS_SLEEPING;
  FIGHTING(bard) = &fixture.victim;
  FIGHTING(&fixture.victim) = bard;

  new_affect(&forced_failure);
  forced_failure.spell = AFFECT_WIZARD_IRRESISTIBLE_MAGIC;
  forced_failure.duration = 1;
  affect_to_char(&fixture.victim, &forced_failure);

  hit(bard, &fixture.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);

  recovery_duration = -1;
  slow_duration = -1;
  for (effect = fixture.victim.affected; effect; effect = effect->next)
  {
    if (effect->spell == AFFECT_BARD_WINTERS_WAR_MARCH)
      slow_duration = effect->duration;
    else if (effect->spell == AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY)
      recovery_duration = effect->duration;
  }

  saved_character_list = character_list;
  fixture.victim.next = NULL;
  character_list = &fixture.victim;
  recovery_never_preceded_slow = true;
  for (update = 0; update < 4; update++)
  {
    affect_update();
    if (affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH) &&
        !affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY))
      recovery_never_preceded_slow = false;
  }
  recovery_removed = !affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY);
  slow_removed = !affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH);
  character_list = saved_character_list;

  FIGHTING(bard) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  bard->next_in_room = NULL;
  IN_ROOM(bard) = NOWHERE;
  fixture.rooms[0].people = &fixture.actor;
  fixture.actor.next_in_room = &fixture.victim;
  free_attack_queue(GET_ATTACK_QUEUE(bard));
  GET_ATTACK_QUEUE(bard) = NULL;
  free_char(bard);
  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, 3, slow_duration);
  CuAssertIntEquals(tc, slow_duration, recovery_duration);
  CuAssertTrue(tc, recovery_never_preceded_slow);
  CuAssertTrue(tc, recovery_removed);
  CuAssertTrue(tc, slow_removed);
}

void Test_gameplay_e2e_player_file_round_trip(CuTest *tc)
{
  struct player_index_element fixture_index[1];
  struct player_index_element *saved_player_table;
  struct char_data *source;
  struct char_data *loaded;
  struct char_data *legacy_loaded;
  struct affected_type af;
  int saved_top_of_p_table;
  int load_result;
  int loaded_level;
  int loaded_gold;
  int loaded_boarding;
  int legacy_loaded_boarding;
  int restore_result;
  long loaded_faction_one;
  long loaded_faction_two;
  long loaded_faction_three;
  unsigned long long loaded_merchant_consequence;
  bool loaded_name_matches;
  bool loaded_ambush_preserved;
  bool loaded_supremacy_migrated;
  bool loaded_stones_endurance_preserved;
  bool changed_directory;
  bool filename_ready;
  bool legacy_file_ready;
  char original_directory[PATH_MAX];
  char lib_directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char player_name[32];

  memset(fixture_index, 0, sizeof(fixture_index));
  memset(filename, 0, sizeof(filename));
  source = new_char();
  loaded = new_char();
  legacy_loaded = new_char();
  snprintf(player_name, sizeof(player_name), "Zzct%ld", (long)getpid());

  fixture_index[0].name = player_name;
  fixture_index[0].id = 4242;
  fixture_index[0].level = 7;
  fixture_index[0].last = 0;

  saved_player_table = player_table;
  saved_top_of_p_table = top_of_p_table;
  player_table = fixture_index;
  top_of_p_table = 0;

  source->player.name = strdup(player_name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4242;
  GET_LEVEL(source) = 7;
  GET_GOLD(source) = 12345;
  SET_ABILITY(source, ABILITY_BOARDING, 9);
  GET_FACTION_STANDING(source, 1) = 111;
  GET_FACTION_STANDING(source, 2) = -222;
  GET_FACTION_STANDING(source, 3) = 333;
  GET_VESSEL_MERCHANT_CONSEQUENCE(source) = 987654321ULL;
  source->player.time.logon = 0;

  new_affect(&af);
  af.spell = PERK_INQUISITOR_SUPREMACY;
  af.duration = -1;
  af.location = APPLY_WIS;
  af.modifier = 2;
  af.bonus_type = BONUS_TYPE_UNIVERSAL;
  affect_to_char(source, &af);

  new_affect(&af);
  af.spell = ABILITY_AFFECT_STONES_ENDURANCE;
  af.duration = 5;
  affect_to_char(source, &af);

  new_affect(&af);
  af.spell = AFFECT_INQUISITOR_AMBUSH_USED;
  af.duration = 7;
  affect_to_char(source, &af);

  changed_directory = false;
  filename_ready = false;
  load_result = -1;
  loaded_level = -1;
  loaded_gold = -1;
  loaded_boarding = -1;
  legacy_loaded_boarding = -1;
  loaded_faction_one = 0;
  loaded_faction_two = 0;
  loaded_faction_three = 0;
  loaded_merchant_consequence = 0;
  loaded_name_matches = false;
  loaded_ambush_preserved = false;
  loaded_supremacy_migrated = false;
  loaded_stones_endurance_preserved = false;
  legacy_file_ready = false;
  restore_result = 0;

  if (getcwd(original_directory, sizeof(original_directory)) != NULL &&
      snprintf(lib_directory, sizeof(lib_directory), "%s/lib", test_source_root()) <
          (int)sizeof(lib_directory) &&
      chdir(lib_directory) == 0)
  {
    changed_directory = true;
    filename_ready = get_filename(filename, sizeof(filename), PLR_FILE, player_name);
    if (filename_ready)
    {
      save_char(source, TRUE);
      load_result = load_char(player_name, loaded);
      if (load_result >= 0)
      {
        loaded_level = GET_LEVEL(loaded);
        loaded_gold = GET_GOLD(loaded);
        loaded_boarding = GET_ABILITY(loaded, ABILITY_BOARDING);
        loaded_faction_one = GET_FACTION_STANDING(loaded, 1);
        loaded_faction_two = GET_FACTION_STANDING(loaded, 2);
        loaded_faction_three = GET_FACTION_STANDING(loaded, 3);
        loaded_merchant_consequence = GET_VESSEL_MERCHANT_CONSEQUENCE(loaded);
        loaded_name_matches =
            GET_NAME(loaded) != NULL && strcmp(GET_NAME(loaded), player_name) == 0;
        loaded_ambush_preserved = affected_by_spell(loaded, AFFECT_INQUISITOR_AMBUSH_USED) &&
                                  !affected_by_spell(loaded, AFFECT_PSIONICIST_PSYCHIC_SUNDERING);
        loaded_supremacy_migrated = affected_by_spell(loaded, AFFECT_INQUISITOR_SUPREMACY) &&
                                    !affected_by_spell(loaded, PERK_INQUISITOR_SUPREMACY);
        loaded_stones_endurance_preserved =
            affected_by_spell(loaded, ABILITY_AFFECT_STONES_ENDURANCE) &&
            !affected_by_spell(loaded, AFFECT_ALCHEMIST_DISCOVERY_EXTRACTION);
      }
      legacy_file_ready = remove_boarding_ability_version(filename);
      if (legacy_file_ready && load_char(player_name, legacy_loaded) >= 0)
        legacy_loaded_boarding = GET_ABILITY(legacy_loaded, ABILITY_BOARDING);
      unlink(filename);
    }
  }

  if (changed_directory)
    restore_result = chdir(original_directory);

  free_char(loaded);
  free_char(legacy_loaded);
  free_char(source);
  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertTrue(tc, changed_directory);
  CuAssertIntEquals(tc, 0, restore_result);
  CuAssertTrue(tc, filename_ready);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertTrue(tc, loaded_name_matches);
  CuAssertIntEquals(tc, 7, loaded_level);
  CuAssertIntEquals(tc, 12345, loaded_gold);
  CuAssertIntEquals(tc, 9, loaded_boarding);
  CuAssertTrue(tc, legacy_file_ready);
  CuAssertIntEquals(tc, 0, legacy_loaded_boarding);
  CuAssertTrue(tc, loaded_faction_one == 111);
  CuAssertTrue(tc, loaded_faction_two == -222);
  CuAssertTrue(tc, loaded_faction_three == 333);
  CuAssertTrue(tc, loaded_merchant_consequence == 987654321ULL);
  CuAssertTrue(tc, loaded_ambush_preserved);
  CuAssertTrue(tc, loaded_supremacy_migrated);
  CuAssertTrue(tc, loaded_stones_endurance_preserved);
}

void Test_gameplay_e2e_late_psychic_sundering_migrates_from_legacy_affects(CuTest *tc)
{
  struct player_index_element fixture_index[1];
  struct player_index_element *saved_player_table;
  struct affected_type af;
  struct affected_type *loaded_affect;
  struct char_data *loaded;
  struct char_data *source;
  int load_result;
  int loaded_duration;
  int loaded_reduction;
  int restore_result;
  int saved_top_of_p_table;
  bool changed_directory;
  bool filename_ready;
  bool legacy_file_ready;
  bool migrated;
  bool save_result;
  char filename[MAX_FILEPATH];
  char lib_directory[PATH_MAX];
  char original_directory[PATH_MAX];
  char player_name[32];

  memset(fixture_index, 0, sizeof(fixture_index));
  memset(filename, 0, sizeof(filename));
  source = new_char();
  loaded = new_char();
  snprintf(player_name, sizeof(player_name), "Zzps%ld", (long)getpid());

  fixture_index[0].name = player_name;
  fixture_index[0].id = 4243;
  fixture_index[0].level = 7;
  fixture_index[0].last = 0;

  saved_player_table = player_table;
  saved_top_of_p_table = top_of_p_table;
  player_table = fixture_index;
  top_of_p_table = 0;

  source->player.name = strdup(player_name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4243;
  GET_LEVEL(source) = 7;
  source->player.time.logon = 0;

  new_affect(&af);
  af.spell = AFFECT_PSIONICIST_PSYCHIC_SUNDERING;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.duration = 5;
  affect_to_char(source, &af);

  changed_directory = false;
  filename_ready = false;
  legacy_file_ready = false;
  save_result = false;
  load_result = -1;
  loaded_duration = -1;
  loaded_reduction = 0;
  migrated = false;
  restore_result = 0;

  if (getcwd(original_directory, sizeof(original_directory)) != NULL &&
      snprintf(lib_directory, sizeof(lib_directory), "%s/lib", test_source_root()) <
          (int)sizeof(lib_directory) &&
      chdir(lib_directory) == 0)
  {
    changed_directory = true;
    filename_ready = get_filename(filename, sizeof(filename), PLR_FILE, player_name);
    if (filename_ready)
    {
      save_result = save_char_checked(source, TRUE);
      if (save_result)
        legacy_file_ready = rewrite_psychic_sundering_as_legacy(filename);
      if (legacy_file_ready)
      {
        load_result = load_char(player_name, loaded);
        if (load_result >= 0)
        {
          migrated = affected_by_spell(loaded, AFFECT_PSIONICIST_PSYCHIC_SUNDERING) &&
                     !affected_by_spell(loaded, AFFECT_INQUISITOR_AMBUSH_USED);
          for (loaded_affect = loaded->affected; loaded_affect; loaded_affect = loaded_affect->next)
          {
            if (loaded_affect->spell == AFFECT_PSIONICIST_PSYCHIC_SUNDERING)
            {
              loaded_duration = loaded_affect->duration;
              break;
            }
          }
          loaded_reduction =
              compute_damtype_reduction(loaded, DAM_RESERVED_DBC, NULL, TYPE_UNDEFINED);
        }
      }
      unlink(filename);
    }
  }

  if (changed_directory)
    restore_result = chdir(original_directory);

  free_char(loaded);
  free_char(source);
  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertTrue(tc, changed_directory);
  CuAssertIntEquals(tc, 0, restore_result);
  CuAssertTrue(tc, filename_ready);
  CuAssertTrue(tc, save_result);
  CuAssertTrue(tc, legacy_file_ready);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertTrue(tc, migrated);
  CuAssertIntEquals(tc, 5, loaded_duration);
  CuAssertIntEquals(tc, -10, loaded_reduction);
}

void Test_gameplay_e2e_dg_trigger_parse_and_execute(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct index_data *prototype_index;
  struct cmdlist_element *commands;
  struct cmdlist_element *next_command;
  struct trig_var_data *variable;
  int saved_top_of_trigt;
  int trigger_result;
  bool parsed;
  bool result_matches;
  FILE *trigger_file;

  begin_gameplay_fixture(&fixture);
  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  saved_trigger_list = trigger_list;
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = tmpfile();
  parsed = false;
  result_matches = false;
  trigger_result = 0;

  if (trig_index != NULL && trigger_file != NULL)
  {
    fprintf(trigger_file, "End-to-end command trigger~\n");
    fprintf(trigger_file, "2 c 100\n");
    fprintf(trigger_file, "probe~\n");
    fprintf(trigger_file, "set result 42\n");
    fprintf(trigger_file, "global result\n");
    fprintf(trigger_file, "~\n");
    rewind(trigger_file);

    parse_trigger(trigger_file, 9000);
    parsed = top_of_trigt == 1 && trig_index[0] != NULL;
    if (parsed)
    {
      fixture.rooms[0].script = calloc(1, sizeof(*fixture.rooms[0].script));
      if (fixture.rooms[0].script != NULL)
      {
        add_trigger(fixture.rooms[0].script, read_trigger(0), -1);
        trigger_result = command_wtrigger(&fixture.actor, "probe", "");
        for (variable = fixture.rooms[0].script->global_vars; variable != NULL;
             variable = variable->next)
        {
          if (strcmp(variable->name, "result") == 0 && strcmp(variable->value, "42") == 0)
          {
            result_matches = true;
            break;
          }
        }
        extract_script(&fixture.rooms[0], WLD_TRIGGER);
      }
    }
  }

  if (trigger_file != NULL)
    fclose(trigger_file);

  prototype_index = parsed ? trig_index[0] : NULL;
  if (prototype_index != NULL)
  {
    commands = ((struct trig_data *)prototype_index->proto)->cmdlist;
    free_trigger((struct trig_data *)prototype_index->proto);
    while (commands != NULL)
    {
      next_command = commands->next;
      free(commands->cmd);
      free(commands);
      commands = next_command;
    }
    free(prototype_index);
  }
  free(trig_index);
  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
  trigger_list = saved_trigger_list;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, parsed);
  CuAssertIntEquals(tc, 1, trigger_result);
  CuAssertTrue(tc, result_matches);
}

void Test_gameplay_e2e_mob_path_handles_already_at_destination(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool moved_on_path;

  begin_gameplay_fixture(&fixture);
  PATH_SIZE(&fixture.actor) = 1;
  PATH_INDEX(&fixture.actor) = 0;
  PATH_DELAY(&fixture.actor) = 0;
  PATH_RESET(&fixture.actor) = 0;
  GET_PATH(&fixture.actor, 0) = fixture.rooms[0].number;

  moved_on_path = move_on_path(&fixture.actor);

  CuAssertTrue(tc, moved_on_path);
  CuAssertIntEquals(tc, 1, PATH_INDEX(&fixture.actor));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_e2e_damage_trigger_overrides_damage(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct index_data *prototype_index;
  struct cmdlist_element *commands;
  struct cmdlist_element *next_command;
  struct trig_var_data *variable;
  int saved_top_of_trigt;
  int damage_result;
  int remaining_hit_points;
  bool parsed;
  bool damage_variable_matches;
  FILE *trigger_file;

  begin_gameplay_fixture(&fixture);
  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  saved_trigger_list = trigger_list;
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = tmpfile();
  parsed = false;
  damage_variable_matches = false;
  damage_result = -1;
  remaining_hit_points = GET_HIT(&fixture.victim);

  if (trig_index != NULL && trigger_file != NULL)
  {
    fprintf(trigger_file, "End-to-end damage trigger~\n");
    fprintf(trigger_file, "0 u 100\n");
    fprintf(trigger_file, "~\n");
    fprintf(trigger_file, "set seen_damage %%damage%%\n");
    fprintf(trigger_file, "global seen_damage\n");
    fprintf(trigger_file, "return 5\n");
    fprintf(trigger_file, "~\n");
    rewind(trigger_file);

    parse_trigger(trigger_file, 9001);
    parsed = top_of_trigt == 1 && trig_index[0] != NULL;
    if (parsed)
    {
      fixture.victim.script = calloc(1, sizeof(*fixture.victim.script));
      if (fixture.victim.script != NULL)
      {
        add_trigger(fixture.victim.script, read_trigger(0), -1);
        FIGHTING(&fixture.actor) = &fixture.victim;
        FIGHTING(&fixture.victim) = &fixture.actor;
        damage_result = damage(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON, FALSE);
        remaining_hit_points = GET_HIT(&fixture.victim);
        for (variable = fixture.victim.script->global_vars; variable != NULL;
             variable = variable->next)
        {
          if (strcmp(variable->name, "seen_damage") == 0 && strcmp(variable->value, "17") == 0)
          {
            damage_variable_matches = true;
            break;
          }
        }
        extract_script(&fixture.victim, MOB_TRIGGER);
      }
    }
  }

  if (GET_ID(&fixture.actor) != 0)
    remove_from_lookup_table(GET_ID(&fixture.actor));
  if (GET_ID(&fixture.victim) != 0)
    remove_from_lookup_table(GET_ID(&fixture.victim));
  if (trigger_file != NULL)
    fclose(trigger_file);

  prototype_index = parsed ? trig_index[0] : NULL;
  if (prototype_index != NULL)
  {
    commands = ((struct trig_data *)prototype_index->proto)->cmdlist;
    free_trigger((struct trig_data *)prototype_index->proto);
    while (commands != NULL)
    {
      next_command = commands->next;
      free(commands->cmd);
      free(commands);
      commands = next_command;
    }
    free(prototype_index);
  }
  free(trig_index);
  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
  trigger_list = saved_trigger_list;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, parsed);
  CuAssertIntEquals(tc, 5, damage_result);
  CuAssertIntEquals(tc, 95, remaining_hit_points);
  CuAssertTrue(tc, damage_variable_matches);
}

void Test_gameplay_e2e_actual_minimal_world_parse(CuTest *tc)
{
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct room_data *parsed_world;
  struct zone_data *parsed_zone;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  int room_count;
  int i;
  bool file_opened;
  bool rooms_match;
  bool exits_match;
  char world_path[PATH_MAX];
  FILE *world_file;

  saved_world = world;
  saved_top_of_world = top_of_world;
  saved_zone_table = zone_table;
  saved_top_of_zone_table = top_of_zone_table;
  parsed_world = calloc(4, sizeof(*parsed_world));
  parsed_zone = calloc(1, sizeof(*parsed_zone));
  world_file = NULL;
  file_opened = false;
  rooms_match = false;
  exits_match = false;
  room_count = 0;

  if (parsed_world != NULL && parsed_zone != NULL &&
      snprintf(world_path, sizeof(world_path), "%s/lib/world/minimal/0.wld", test_source_root()) <
          (int)sizeof(world_path))
  {
    parsed_zone[0].number = 0;
    parsed_zone[0].bot = 0;
    parsed_zone[0].top = 3099;
    world = parsed_world;
    top_of_world = 0;
    zone_table = parsed_zone;
    top_of_zone_table = 0;
    world_file = fopen(world_path, "r");
    file_opened = world_file != NULL;
    if (file_opened)
    {
      discrete_load(world_file, DB_BOOT_WLD, world_path);
      fclose(world_file);
      world_file = NULL;
      renum_world();
      room_count = top_of_world + 1;
      rooms_match = top_of_world == 3 && world[0].number == 0 && world[1].number == 3000 &&
                    world[2].number == 3001 && world[3].number == 3002 &&
                    strcmp(world[1].name, "Arrival Platform") == 0 &&
                    strstr(world[2].description, "Starlight spills") != NULL;
      exits_match = world[1].dir_option[EAST] != NULL && world[1].dir_option[EAST]->to_room == 2 &&
                    world[2].dir_option[WEST] != NULL && world[2].dir_option[WEST]->to_room == 1 &&
                    world[2].dir_option[EAST] != NULL && world[2].dir_option[EAST]->to_room == 3;
    }
  }

  if (world_file != NULL)
    fclose(world_file);
  if (parsed_world != NULL)
  {
    for (i = 0; i < room_count; i++)
    {
      if (parsed_world[i].script != NULL)
        extract_script(&parsed_world[i], WLD_TRIGGER);
      free_proto_script(&parsed_world[i], WLD_TRIGGER);
      free_room_strings(&parsed_world[i]);
      if (parsed_world[i].trail_tracks != NULL)
        free_trail_data_list(parsed_world[i].trail_tracks);
    }
  }
  free(parsed_world);
  free(parsed_zone);
  world = saved_world;
  top_of_world = saved_top_of_world;
  zone_table = saved_zone_table;
  top_of_zone_table = saved_top_of_zone_table;

  CuAssertTrue(tc, file_opened);
  CuAssertIntEquals(tc, 4, room_count);
  CuAssertTrue(tc, rooms_match);
  CuAssertTrue(tc, exits_match);
}
