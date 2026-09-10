#include "CuTest.h"
#include "test_spec_fixtures.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/actionqueues.h"
#include "../../src/actions.h"
#include "../../src/ready_action.h"
#include "../../src/tactical_effects.h"
#include "../../src/combat/combat_encounters.h"
#include "../../src/activity_manager.h"
#include "../../src/active_world.h"
#include "../../src/character_periodic.h"
#include "../../src/affected_owners.h"
#include "../../src/magic/buff_sequence.h"
#include "../../src/magic/spell_prep.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/domain_object_transfer.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_runtime.h"
#include "../../src/point_update_periodic.h"
#include "../../src/bardic_performance.h"
#include "../../src/craft/craft.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/db.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/combat/fight.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/olc/genwld.h"
#include "../../src/olc/oasis.h"
#include "../../src/olc/prefedit.h"
#include "../../src/olc/genobj.h"
#include "../../src/vessels/transport.h"
#include "../../src/vessels/transport_jobs.h"
#include "../../src/quest/staff_events.h"
#include "../../src/quest/staff_event_agenda.h"
#include "../../src/vessels/routing.h"
#include "../../src/handler.h"
#include "../../src/obj/vendor.h"
#include "../../src/obj/shop.h"
#include "../../src/interpreter.h"
#include "../../src/mob/mob_utils.h"
#include "../../src/mob/mob_known_spells.h"
#include "../../src/mob/phenomenon_response.h"
#include "../../src/quest/missions.h"
#include "../../src/quest/quest.h"
#include "../../src/movement/movement.h"
#include "../../src/movement/door_state.h"
#include "../../src/wilderness/kdtree.h"
#include "../../src/wilderness/spatial_core.h"
#include "../../src/wilderness/wilderness.h"
#include "../../src/character/perks.h"
#include "../../src/net/protocol.h"
#include "../../src/magic/spells.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/character/class.h"
#include "../../src/character/premadebuilds.h"
#include "../../src/character/feats.h"
#include "../../src/character/evolutions.h"
#include "../../src/character/backgrounds.h"
#include "../../src/combat/spec_abilities.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/spec/spec_dispatch.h"
#include "../../src/spec/spec_objects.h"
#include "../../src/spec/spec_mobile_archetypes.h"
#include "../../src/spec/spec_mobiles.h"
#include "../../src/mud_event.h"
#include "../../src/mudlim.h"
#include "../../src/mysql.h"
#include "../../src/dgscript/dg_event.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Real player saves also write the index: keep every persistence fixture isolated. */
static void enter_player_fixture(CuTest *tc, char *temporary_directory)
{
  CuAssertPtrNotNull(tc, mkdtemp(temporary_directory));
  CuAssertIntEquals(tc, 0, chdir(temporary_directory));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/U-Z", 0700));
}

/** Remove the synthetic player index and return to the original working directory. */
static int leave_player_fixture(const char *directory, const char *temporary_directory)
{
  int result;

  unlink("plrfiles/index");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  result = chdir(directory);
  if (result == 0)
    rmdir(temporary_directory);
  return result;
}

/** Load charge timing formats from isolated player files and check recovered cadence. */
static void verify_gameplay_charge_load(CuTest *tc, unsigned int format, int elapsed, int charisma,
                                        int interval, const char *expected)
{
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *loaded = new_char();
  struct mud_event_data *event;
  char directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char name[32];
  FILE *file;
  int result;
  bool restored = false;
  unsigned long saved_pulse = pulse;

  snprintf(name, sizeof(name), "Zzev%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4245;
  player_table = index;
  top_of_p_table = 0;
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  fprintf(file, "Name: %s\nId  : 4245\nLevl: 7\nEvn2: %u\n%d 2 4245 1 %lld 3", name, format,
          eCHANNELENERGY, (long long)time(NULL) - elapsed);
  if (format == MUD_EVENT_DURABLE_FORMAT_VERSION)
    fprintf(file, " %d", interval);
  fprintf(file, "\n-1\nCha : %d\n", charisma);
  fclose(file);
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  result = load_char(name, loaded);
  event = char_has_mud_event(loaded, eCHANNELENERGY);
  if (event != NULL)
    restored = event->sVariables != NULL && !strcmp(event->sVariables, expected);
  unlink(filename);
  CuAssertIntEquals(tc, 0, leave_player_fixture(directory, temporary_directory));
  free_char(loaded);
  event_free_all();
  pulse = saved_pulse;
  player_table = saved_table;
  top_of_p_table = saved_top;
  CuAssertIntEquals(tc, 0, result);
  CuAssertTrue(tc, restored);
}

void Test_gameplay_load_recovers_charges_after_effective_stats(CuTest *tc)
{
  verify_gameplay_charge_load(tc, 1U, 2, 18, 0, "uses:2");
}

void Test_gameplay_load_recovers_charges_at_saved_equipped_cadence(CuTest *tc)
{
  verify_gameplay_charge_load(tc, MUD_EVENT_DURABLE_FORMAT_VERSION, SECS_PER_MUD_DAY / 8 + 2, 10,
                              (SECS_PER_MUD_DAY / 8) * PASSES_PER_SEC, "uses:1");
}

/** Preserve charge cadence across real saves without writing the development player index. */
void Test_gameplay_save_captures_charge_cadence_before_unequipping(CuTest *tc)
{
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *ch = new_char();
  struct obj_data *item;
  char directory[PATH_MAX];
  char filename[MAX_FILEPATH];
  char name[32];
  char line[MAX_INPUT_LENGTH];
  FILE *file;
  long long owner, remaining, epoch, cadence;
  long long saved_cadence = -1;
  unsigned int schema;
  int type, uses;
  int equipped_charisma;
  bool saved;
  unsigned long saved_pulse = pulse;

  snprintf(name, sizeof(name), "Zzcd%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4246;
  index[0].level = 7;
  player_table = index;
  top_of_p_table = 0;
  ch->player.name = strdup(name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4246;
  GET_LEVEL(ch) = 7;
  GET_REAL_CHA(ch) = 10;
  affect_total(ch);
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  item = create_obj();
  item->affected[0].location = APPLY_CHA;
  item->affected[0].modifier = 10;
  equip_char(ch, item, WEAR_NECK_1);
  equipped_charisma = GET_CHA(ch);
  attach_mud_event(new_mud_event(eCHANNELENERGY, ch, "uses:3"), PASSES_PER_SEC);

  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  saved = save_char_checked(ch, 0);
  file = fopen(filename, "r");
  if (file != NULL)
  {
    while (fgets(line, sizeof(line), file) != NULL)
      if (sscanf(line, "%d %u %lld %lld %lld %d %lld", &type, &schema, &owner, &remaining, &epoch,
                 &uses, &cadence) == 7 &&
          type == eCHANNELENERGY && owner == 4246)
        saved_cadence = cadence;
    fclose(file);
  }
  unlink(filename);
  CuAssertIntEquals(tc, 0, leave_player_fixture(directory, temporary_directory));
  unequip_char(ch, WEAR_NECK_1);
  extract_obj(item);
  free_char(ch);
  event_free_all();
  pulse = saved_pulse;
  player_table = saved_table;
  top_of_p_table = saved_top;
  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 20, equipped_charisma);
  CuAssertTrue(tc, saved_cadence == (SECS_PER_MUD_DAY / 8) * PASSES_PER_SEC);
}

/* Copyover uses this same mode-zero pfile save and native load path. */
void Test_gameplay_pet_cooldowns_survive_character_save_and_load(CuTest *tc)
{
  const event_id types[] = {eMUMMYDUST,  eDRAGONKNIGHT, eC_ANIMAL,     eC_DRAGONMOUNT,
                            eC_FAMILIAR, eC_MOUNT,      eSUMMONSHADOW, eC_EIDOLON};
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *ch = new_char();
  struct char_data *loaded = new_char();
  struct mud_event_data *event;
  const struct mud_event_persistence_policy *policy;
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  char directory[PATH_MAX], filename[MAX_FILEPATH], name[32];
  unsigned long saved_pulse = pulse;
  size_t i;
  long remaining;
  bool saved, retained = true;
  int result;

  snprintf(name, sizeof(name), "Zzpetcd%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4247;
  index[0].level = 7;
  player_table = index;
  top_of_p_table = 0;
  ch->player.name = strdup(name);
  GET_PFILEPOS(ch) = 0;
  GET_IDNUM(ch) = 4247;
  GET_LEVEL(ch) = 7;
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  for (i = 0; i < sizeof(types) / sizeof(types[0]); i++)
  {
    policy = mud_event_persistence_policy(types[i]);
    attach_mud_event(
        new_mud_event(types[i], ch,
                      policy->payload_policy == MUD_EVENT_PAYLOAD_USES ? "uses:1" : NULL),
        300 * PASSES_PER_SEC);
  }
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  saved = save_char_checked(ch, 0);
  free_char(ch);
  event_free_all();
  event_init();
  result = load_char(name, loaded);
  for (i = 0; i < sizeof(types) / sizeof(types[0]); i++)
  {
    event = char_has_mud_event(loaded, types[i]);
    if (event == NULL)
    {
      retained = false;
      continue;
    }
    remaining = mud_event_remaining(event);
    retained = retained && remaining > 0 && remaining <= 300 * PASSES_PER_SEC;
    policy = mud_event_persistence_policy(types[i]);
    if (policy->payload_policy == MUD_EVENT_PAYLOAD_USES)
      retained = retained && event->sVariables != NULL && !strcmp(event->sVariables, "uses:1");
  }
  unlink(filename);
  CuAssertIntEquals(tc, 0, leave_player_fixture(directory, temporary_directory));
  free_char(loaded);
  event_free_all();
  pulse = saved_pulse;
  player_table = saved_table;
  top_of_p_table = saved_top;
  CuAssertTrue(tc, saved);
  CuAssertIntEquals(tc, 0, result);
  CuAssertTrue(tc, retained);
}

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

static bool player_file_has_cooldown_checkpoint(const char *filename)
{
  FILE *file;
  char line[MAX_INPUT_LENGTH];
  long long checkpoint;
  bool found;

  file = fopen(filename, "r");
  if (file == NULL)
    return false;
  found = false;
  while (fgets(line, sizeof(line), file) != NULL)
  {
    if (sscanf(line, "CkAt: %lld", &checkpoint) == 1 && checkpoint > 0)
    {
      found = true;
      break;
    }
  }
  if (fclose(file) != 0)
    return false;
  return found;
}

static bool rewrite_player_cooldown_checkpoint(const char *filename, int64_t checkpoint)
{
  FILE *input;
  FILE *output;
  char line[MAX_STRING_LENGTH];
  char temp_filename[MAX_FILEPATH + 16];
  bool saw_checkpoint;
  bool write_ok;

  if (snprintf(temp_filename, sizeof(temp_filename), "%s.cooldown", filename) >=
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

  saw_checkpoint = false;
  write_ok = true;
  while (fgets(line, sizeof(line), input) != NULL)
  {
    if (strncmp(line, "CkAt:", 5) == 0)
    {
      if (fprintf(output, "CkAt: %" PRId64 "\n", checkpoint) < 0)
      {
        write_ok = false;
        break;
      }
      saw_checkpoint = true;
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
  if (write_ok && saw_checkpoint && rename(temp_filename, filename) == 0)
    return true;

  unlink(temp_filename);
  return false;
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
  movement_trail_registry_shutdown();

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
  movement_trail_registry_shutdown();

  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

void Test_gameplay_e2e_npc_audience_requires_player_in_same_room(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool empty_of_players;
  bool player_detected;
  bool invalid_room_rejected;

  begin_gameplay_fixture(&fixture);
  empty_of_players = !npc_room_has_player(&fixture.actor);

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_ISNPC);
  player_detected = npc_room_has_player(&fixture.actor);

  IN_ROOM(&fixture.actor) = NOWHERE;
  invalid_room_rejected = !npc_room_has_player(&fixture.actor);
  IN_ROOM(&fixture.actor) = 0;
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, empty_of_players);
  CuAssertTrue(tc, player_detected);
  CuAssertTrue(tc, invalid_room_rejected);
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

void Test_gameplay_e2e_class_list_hides_disabled_classes(CuTest *tc)
{
  struct class_table saved_class_list[NUM_CLASSES];
  struct char_data character;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct account_data account;
  bool saw_wizard;
  bool saw_placeholder_1;
  bool saw_placeholder_2;
  int i;

  memcpy(saved_class_list, class_list, sizeof(saved_class_list));
  for (i = 0; i < NUM_CLASSES; i++)
  {
    memset(&class_list[i], 0, sizeof(class_list[i]));
    class_list[i].name = "disabled class";
    class_list[i].max_level = 20;
  }
  class_list[CLASS_WIZARD].name = "wizard";
  class_list[CLASS_WIZARD].in_game = true;
  class_list[CLASS_PLACEHOLDER_1].name = "placeholder 1";
  class_list[CLASS_PLACEHOLDER_2].name = "placeholder 2";

  memset(&character, 0, sizeof(character));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&account, 0, sizeof(account));
  character.player_specials = &player_specials;
  character.desc = &descriptor;
  descriptor.character = &character;
  descriptor.account = &account;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    memcpy(class_list, saved_class_list, sizeof(saved_class_list));
    CuFail(tc, "could not initialize the class list descriptor");
    return;
  }

  do_class(&character, "list", 0, 0);

  saw_wizard = strstr(descriptor.output, "wizard") != NULL;
  saw_placeholder_1 = strstr(descriptor.output, "placeholder 1") != NULL;
  saw_placeholder_2 = strstr(descriptor.output, "placeholder 2") != NULL;

  ProtocolDestroy(descriptor.pProtocol);
  memcpy(class_list, saved_class_list, sizeof(saved_class_list));

  CuAssertTrue(tc, saw_wizard);
  CuAssertTrue(tc, !saw_placeholder_1);
  CuAssertTrue(tc, !saw_placeholder_2);
}

void Test_gameplay_e2e_accexp_class_hides_disabled_classes(CuTest *tc)
{
  struct class_table saved_class_list[NUM_CLASSES];
  struct char_data character;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct account_data account;
  bool saw_cleric;
  bool saw_placeholder_1;
  bool saw_placeholder_2;
  int i;

  memcpy(saved_class_list, class_list, sizeof(saved_class_list));
  for (i = 0; i < NUM_CLASSES; i++)
  {
    memset(&class_list[i], 0, sizeof(class_list[i]));
    class_list[i].name = "disabled class";
    class_list[i].max_level = 20;
  }
  class_list[CLASS_CLERIC].name = "cleric";
  class_list[CLASS_CLERIC].locked_class = true;
  class_list[CLASS_CLERIC].in_game = true;
  class_list[CLASS_CLERIC].unlock_cost = 200;
  class_list[CLASS_PLACEHOLDER_1].name = "placeholder 1";
  class_list[CLASS_PLACEHOLDER_1].locked_class = true;
  class_list[CLASS_PLACEHOLDER_2].name = "placeholder 2";
  class_list[CLASS_PLACEHOLDER_2].locked_class = true;

  memset(&character, 0, sizeof(character));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&account, 0, sizeof(account));
  for (i = 0; i < MAX_UNLOCKED_CLASSES; i++)
    account.classes[i] = -1;
  character.player_specials = &player_specials;
  character.desc = &descriptor;
  descriptor.character = &character;
  descriptor.account = &account;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();

  if (descriptor.pProtocol == NULL)
  {
    memcpy(class_list, saved_class_list, sizeof(saved_class_list));
    CuFail(tc, "could not initialize the account experience descriptor");
    return;
  }

  do_accexp(&character, "class", 0, 0);

  saw_cleric = strstr(descriptor.output, "cleric (200 account experience)") != NULL;
  saw_placeholder_1 = strstr(descriptor.output, "placeholder 1") != NULL;
  saw_placeholder_2 = strstr(descriptor.output, "placeholder 2") != NULL;

  ProtocolDestroy(descriptor.pProtocol);
  memcpy(class_list, saved_class_list, sizeof(saved_class_list));

  CuAssertTrue(tc, saw_cleric);
  CuAssertTrue(tc, !saw_placeholder_1);
  CuAssertTrue(tc, !saw_placeholder_2);
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
  int compatibility_attempts;
  int semantic_attempts;
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
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 0);
  semantic_attempts = CNDNSD(staff)->num_times_attacking;
  init_condensed_combat_data(staff);
  GET_HIT(&fixture.victim) = 100000;
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 1);
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 2);
  perform_attacks(staff, NORMAL_ATTACK_ROUTINE, 3);
#undef NORMAL_ATTACK_ROUTINE
  compatibility_attempts = CNDNSD(staff)->num_times_attacking;
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
  CuAssertIntEquals(tc, expected_attacks, semantic_attempts);
  CuAssertIntEquals(tc, expected_attacks, compatibility_attempts);
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

void Test_gameplay_pet_wait_holds_position_until_explicit_recall(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct follow_type link = {0};
  struct char_data *saved_characters = character_list;
  bool waited, automatic, recalled, followed;

  begin_gameplay_fixture(&fixture);
  fixture.actor.player.name = (char *)"owner";
  fixture.victim.player.name = (char *)"companion";
  fixture.victim.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  link.follower = &fixture.victim;
  fixture.actor.followers = &link;
  character_list = &fixture.victim;

  do_pets(&fixture.actor, "companion wait", 0, 0);
  waited = perform_move(&fixture.actor, NORTH, FALSE) == 1 && IN_ROOM(&fixture.victim) == 0;
  automatic = !char_pets_to_char_loc(&fixture.actor, false) && IN_ROOM(&fixture.victim) == 0;
  recalled = char_pets_to_char_loc(&fixture.actor, true) && IN_ROOM(&fixture.victim) == 1;
  do_pets(&fixture.actor, "followers passive", 0, 0);
  followed = perform_move(&fixture.actor, SOUTH, FALSE) == 1 && IN_ROOM(&fixture.victim) == 0 &&
             !pet_assists_automatically(&fixture.victim, &fixture.actor);

  fixture.actor.followers = NULL;
  fixture.victim.master = NULL;
  character_list = saved_characters;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, waited);
  CuAssertTrue(tc, automatic);
  CuAssertTrue(tc, recalled);
  CuAssertTrue(tc, followed);
}

void Test_gameplay_dg_single_target_teleport_moves_the_targets_pets(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data pet;
  struct char_data *saved_characters = character_list;
  bool moved;

  begin_gameplay_fixture(&fixture);
  initialize_test_npc(&pet, "target companion", 0);
  fixture.actor.player.name = (char *)"teleporter";
  fixture.victim.player.name = (char *)"target";
  pet.player.name = (char *)"companion";
  pet.master = &fixture.victim;
  SET_BIT_AR(AFF_FLAGS(&pet), AFF_CHARM);
  fixture.victim.next_in_room = &pet;
  fixture.actor.next = &fixture.victim;
  fixture.victim.next = &pet;
  character_list = &fixture.actor;

  do_mteleport(&fixture.actor, "target 101", 0, 0);
  moved = IN_ROOM(&fixture.victim) == 1 && IN_ROOM(&pet) == 1 && IN_ROOM(&fixture.actor) == 0;

  pet.master = NULL;
  fixture.actor.next = NULL;
  fixture.victim.next = NULL;
  character_list = saved_characters;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  domain_event_world_forget_character(&pet);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, moved);
}

void Test_gameplay_pet_ids_select_identical_names_without_bypassing_range_or_ownership(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data other;
  bool selected, denied;
  char target[96];
  const char *invalid[] = {"#",   "#0",      "#-2",
                           "#+2", "#22tail", "#99999999999999999999999999999999999999"};
  size_t i;

  begin_gameplay_fixture(&fixture);
  initialize_test_npc(&other, "other companion", 0);
  other.player.name = fixture.victim.player.name = (char *)"companion";
  fixture.victim.next_in_room = &other;
  fixture.victim.master = other.master = &fixture.actor;
  fixture.victim.pet_data_id = 21;
  other.pet_data_id = 22;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  SET_BIT_AR(AFF_FLAGS(&other), AFF_CHARM);
  do_pets(&fixture.actor, "#22 wait", 0, 0);
  selected =
      other.pet_behavior == PET_BEHAVIOR_WAIT && fixture.victim.pet_behavior == PET_BEHAVIOR_FOLLOW;
  snprintf(target, sizeof(target), "#21");
  selected = selected && get_pet_command_target(&fixture.actor, target) == &fixture.victim;
  denied = true;
  for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
  {
    snprintf(target, sizeof(target), "%s", invalid[i]);
    denied = denied && get_pet_command_target(&fixture.actor, target) == NULL;
  }
  snprintf(target, sizeof(target), "#22");
  other.master = &fixture.victim;
  denied = denied && get_pet_command_target(&fixture.actor, target) == NULL;
  other.master = &fixture.actor;
  IN_ROOM(&other) = 1;
  denied = denied && get_pet_command_target(&fixture.actor, target) == NULL;
  IN_ROOM(&other) = 0;
  SET_BIT_AR(MOB_FLAGS(&other), MOB_NOTDEADYET);
  denied = denied && get_pet_command_target(&fixture.actor, target) == NULL;
  REMOVE_BIT_AR(MOB_FLAGS(&other), MOB_NOTDEADYET);
  REMOVE_BIT_AR(AFF_FLAGS(&other), AFF_CHARM);
  do_pets(&fixture.actor, "#22 guard", 0, 0);
  denied = denied && other.pet_behavior == PET_BEHAVIOR_WAIT;
  fixture.victim.master = other.master = NULL;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  domain_event_world_forget_character(&other);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, selected);
  CuAssertTrue(tc, denied);
}

void Test_gameplay_pet_behavior_selection_preserves_ownership_and_control(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data other;
  bool numbered, group, denied, assist;

  begin_gameplay_fixture(&fixture);
  initialize_test_npc(&other, "other companion", 0);
  other.player.name = (char *)"companion";
  fixture.victim.player.name = (char *)"companion";
  fixture.victim.next_in_room = &other;
  fixture.victim.master = &fixture.actor;
  other.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  SET_BIT_AR(AFF_FLAGS(&other), AFF_CHARM);

  do_pets(&fixture.actor, "2.companion wait", 0, 0);
  numbered =
      other.pet_behavior == PET_BEHAVIOR_WAIT && fixture.victim.pet_behavior == PET_BEHAVIOR_FOLLOW;
  do_pets(&fixture.actor, "followers assist", 0, 0);
  group = other.pet_behavior == PET_BEHAVIOR_ASSIST &&
          fixture.victim.pet_behavior == PET_BEHAVIOR_ASSIST;
  assist = pet_assists_automatically(&other, &fixture.actor) &&
           !pet_assists_automatically(&other, &fixture.victim);
  other.master = &fixture.victim;
  do_pets(&fixture.actor, "2.companion guard", 0, 0);
  denied = other.pet_behavior == PET_BEHAVIOR_ASSIST;
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_CHARM);
  do_pets(&fixture.actor, "followers passive", 0, 0);
  denied = denied && fixture.victim.pet_behavior == PET_BEHAVIOR_ASSIST;

  fixture.victim.master = NULL;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  domain_event_world_forget_character(&other);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, numbered);
  CuAssertTrue(tc, group);
  CuAssertTrue(tc, denied);
  CuAssertTrue(tc, assist);
}

void Test_gameplay_pet_guard_obeys_rescue_preference_and_owner_boundary(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data pet;
  struct player_special_data specials = {0};
  unsigned long rescue_seed;
  bool allowed, disabled, wrong_owner, passive, pending, elemental_policy;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  initialize_test_npc(&pet, "guard", 0);
  pet.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&pet), AFF_CHARM);
  pet.pet_behavior = PET_BEHAVIOR_GUARD;
  allowed = pet_guards_owner(&pet, &fixture.actor, &fixture.victim);
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_NO_CHARMIE_RESCUE);
  disabled = !pet_guards_owner(&pet, &fixture.actor, &fixture.victim);
  REMOVE_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_NO_CHARMIE_RESCUE);
  wrong_owner = !pet_guards_owner(&pet, &fixture.victim, &fixture.actor);
  pet.pet_behavior = PET_BEHAVIOR_PASSIVE;
  passive = !npc_rescue(&pet) && !pet_guards_owner(&pet, &fixture.actor, &fixture.victim);
  FIGHTING(&fixture.victim) = &fixture.actor;
  for (rescue_seed = 1; rescue_seed < 100; rescue_seed++)
  {
    circle_srandom(rescue_seed);
    if (rand_number(0, 1) == 0)
      break;
  }
  circle_srandom(rescue_seed);
  elemental_policy = !solid_elemental(&pet, NULL, 0, "");
  pet.pet_behavior = PET_BEHAVIOR_WAIT;
  circle_srandom(rescue_seed);
  elemental_policy = elemental_policy && !wraith_elemental(&pet, NULL, 0, "");
  FIGHTING(&fixture.victim) = NULL;
  circle_srandom((unsigned long)time(NULL));
  pet.pet_behavior = PET_BEHAVIOR_GUARD;
  SET_BIT_AR(MOB_FLAGS(&pet), MOB_NOTDEADYET);
  pending = !pet_guards_owner(&pet, &fixture.actor, &fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, allowed);
  CuAssertTrue(tc, disabled);
  CuAssertTrue(tc, wrong_owner);
  CuAssertTrue(tc, passive);
  CuAssertTrue(tc, elemental_policy);
  CuAssertTrue(tc, pending);
}

static void verify_golem_completion_resources(CuTest *tc, int mode)
{
  struct gameplay_fixture fixture;
  struct char_data *ch = &fixture.actor;
  struct player_special_data specials = {0};
  struct follow_type existing = {0};
  struct char_data prototype, *golem;
  struct char_data *saved_prototypes = mob_proto;
  struct char_data *saved_characters = character_list;
  int i, wood, bronze, mote;
  bool created, reset;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"constructor";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_WOOD;
  GET_CRAFT(ch).golem_size = GOLEM_SIZE_SMALL;
  GET_CRAFT(ch).golem_materials[0][0] = CRAFT_MAT_MAPLE_WOOD;
  GET_CRAFT(ch).dc = mode == 1 || mode == 5 || mode == 6 || mode == 8 ? 10000 : -10000;
  GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) = 100;
  GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE) = 100;
  for (i = 1; i < NUM_CRAFT_MOTES; i++)
    GET_CRAFT_MOTES(ch, i) = 100;
  if (mode == 3)
    GET_CRAFT_MOTES(ch, 1) = 0;
  initialize_test_npc(&prototype, "wood golem", NOWHERE);
  GET_REAL_RACE(&prototype) = mode == 7 || mode == 8 ? RACE_TYPE_ANIMAL : RACE_TYPE_CONSTRUCT;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"golem";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  fixture.mobile_index[0].vnum = PET_GOLEM_WOOD_SMALL;
  mob_proto = mode == 2 || mode == 5 ? NULL : &prototype;
  if (mode == 6)
    fixture.mobile_index[0].vnum = PET_GOLEM_WOOD_SMALL + GOLEM_SIZE_LARGE;
  /* A golem has its own allowance even when the general slot is occupied. */
  existing.follower = &fixture.victim;
  fixture.actor.followers = &existing;
  fixture.victim.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  GET_MOB_RNUM(&fixture.victim) = 0;
  if (mode == 4)
    SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_GOLEM);

  craft_golem_complete(&fixture.actor);
  golem = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  if (golem == &fixture.victim)
    golem = NULL;
  created = golem != NULL && MOB_FLAGGED(golem, MOB_GOLEM) && IS_PET(golem) &&
            golem->char_specials.is_charmie && GET_REAL_RACE(golem) == RACE_TYPE_CONSTRUCT &&
            IN_ROOM(golem) == 0;
  wood = GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD);
  bronze = GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE);
  mote = GET_CRAFT_MOTES(ch, 1);
  reset = GET_CRAFT(ch).golem_type == GOLEM_TYPE_NONE;
  if (golem != NULL)
  {
    extract_char(golem);
    extract_pending_chars();
  }
  character_list = saved_characters;
  fixture.actor.followers = NULL;
  fixture.victim.master = NULL;
  mob_proto = saved_prototypes;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);

  CuAssertIntEquals(tc, mode == 0, created);
  CuAssertIntEquals(tc, mode <= 1 ? 50 : 100, wood);
  CuAssertIntEquals(tc, mode <= 1 ? 90 : 100, bronze);
  CuAssertTrue(tc, mode == 3 ? mote == 0 : mode <= 1 ? mote < 100 : mote == 100);
  CuAssertTrue(tc, reset);
}

void Test_gameplay_golem_completion_consumes_selected_resources_after_creation(CuTest *tc)
{
  verify_golem_completion_resources(tc, 0);
}

void Test_gameplay_failed_golem_ritual_consumes_selected_wood(CuTest *tc)
{
  verify_golem_completion_resources(tc, 1);
}

void Test_gameplay_missing_golem_prototype_retains_resources(CuTest *tc)
{
  verify_golem_completion_resources(tc, 2);
  verify_golem_completion_resources(tc, 5);
  verify_golem_completion_resources(tc, 6);
  verify_golem_completion_resources(tc, 7);
  verify_golem_completion_resources(tc, 8);
}

void Test_gameplay_golem_completion_rechecks_motes_before_consumption(CuTest *tc)
{
  verify_golem_completion_resources(tc, 3);
}

void Test_gameplay_existing_golem_denies_completion_without_spending(CuTest *tc)
{
  verify_golem_completion_resources(tc, 4);
}

void Test_gameplay_craft_c_abbreviates_check_not_create(CuTest *tc)
{
  struct char_data crafter;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};

  clear_char(&crafter);
  crafter.player_specials = &specials;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &crafter;
  descriptor.pProtocol = ProtocolCreate();
  crafter.desc = &descriptor;
  newcraft_create(&crafter, "c");
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "This craft is not yet ready to begin"));
  CuAssertPtrEquals(tc, NULL, strstr(descriptor.output, "See HELP CRAFTING"));
  ProtocolDestroy(descriptor.pProtocol);
}

static bool verify_authored_constructs(const char *sandbox, char *error, size_t error_size)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data *ch = &fixture.actor, *pet;
  struct obj_data *corpse, *loot;
  struct domain_entity_handle corpse_handle;
  struct follow_type occupied = {0};
  struct descriptor_data feedback = {0};
  FILE *file;
  char path[PATH_MAX], line[256];
  int i, type, size, vnum, mode, needed, material;
  bool bone_refund_reported;

  (void)sandbox;
  snprintf(path, sizeof(path), "%s/data/pet-constructs/196.mob", test_source_root());
  file = fopen(path, "r");
  if (file == NULL)
    return false;
  begin_gameplay_fixture(&fixture);
  mob_proto = calloc(19, sizeof(*mob_proto));
  mob_index = calloc(19, sizeof(*mob_index));
  if (mob_proto == NULL || mob_index == NULL)
    return false;
  for (i = 0; i < 13; i++)
  {
    if (!get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 ||
        vnum != PET_GOLEM_WOOD_SMALL + i)
      return false;
    parse_mobile(file, vnum);
  }
  fclose(file);
  snprintf(path, sizeof(path), "%s/data/pet-planar-allies/197.mob", test_source_root());
  file = fopen(path, "r");
  if (file == NULL)
    return false;
  for (i = 0; i < 3; i++)
  {
    if (!get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 ||
        vnum != PET_CELESTIAL_GUARDIAN + i)
      return false;
    parse_mobile(file, vnum);
  }
  fclose(file);
  if (GET_MOB_VNUM(&mob_proto[15]) != PET_XVIM_NIGHTMARE ||
      !isname("nightmare", mob_proto[15].player.name) ||
      GET_REAL_RACE(&mob_proto[15]) != RACE_TYPE_OUTSIDER || GET_SIZE(&mob_proto[15]) != SIZE_LARGE)
    return false;
  snprintf(path, sizeof(path), "%s/data/pet-undead/198.mob", test_source_root());
  file = fopen(path, "r");
  if (file == NULL)
    return false;
  for (i = 0; i < 2; i++)
  {
    if (!get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 || vnum != PET_SKELETAL_MAGE + i)
      return false;
    parse_mobile(file, vnum);
  }
  fclose(file);
  snprintf(path, sizeof(path), "%s/data/pet-illusions/199.mob", test_source_root());
  file = fopen(path, "r");
  if (file == NULL || !get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 ||
      vnum != PET_MISLEAD_DECOY)
    return false;
  parse_mobile(file, vnum);
  fclose(file);
  REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &specials;
  ch->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(ch) = -1;
  for (type = GOLEM_TYPE_WOOD; type <= GOLEM_TYPE_IRON; type++)
  {
    for (size = GOLEM_SIZE_SMALL; size <= GOLEM_SIZE_HUGE; size++)
    {
      GET_CRAFT(ch).golem_type = type;
      GET_CRAFT(ch).golem_size = size;
      GET_CRAFT(ch).golem_materials[0][0] = CRAFT_MAT_MAPLE_WOOD;
      GET_CRAFT(ch).dc = -10000;
      for (i = 1; i < NUM_CRAFT_MATS; i++)
        GET_CRAFT_MAT(ch, i) = 10000;
      for (i = 1; i < NUM_CRAFT_MOTES; i++)
        GET_CRAFT_MOTES(ch, i) = 10000;
      craft_golem_complete(ch);
      pet = ch->followers != NULL ? ch->followers->follower : NULL;
      if (pet == NULL || !IS_PET(pet) || GET_REAL_RACE(pet) != RACE_TYPE_CONSTRUCT ||
          GET_SIZE(pet) != SIZE_SMALL + size || GET_GOLD(pet) != 0 ||
          get_golem_type_from_vnum(GET_MOB_VNUM(pet)) != type ||
          get_golem_size_from_vnum(GET_MOB_VNUM(pet)) != size ||
          GET_CRAFT_MAT(ch, type == GOLEM_TYPE_WOOD    ? CRAFT_MAT_MAPLE_WOOD
                            : type == GOLEM_TYPE_STONE ? CRAFT_MAT_STONE
                                                       : CRAFT_MAT_IRON) >= 10000)
      {
        snprintf(error, error_size, "authored golem acquisition failed for type %d size %d", type,
                 size);
        return false;
      }
      if (GET_DR_MOD(pet) != (type == GOLEM_TYPE_WOOD ? 0 : type == GOLEM_TYPE_STONE ? 5 : 8))
        return false;
      extract_char(pet);
      extract_pending_chars();
    }
  }
  snprintf(error, error_size, "corpse construct acquisition/resource/lifecycle contract failed");
  event_free_all();
  event_init();
  CONFIG_CRAFTING_SYSTEM = CRAFTING_SYSTEM_MOTES;
  corpse = create_obj();
  corpse->name = strdup("corpse");
  corpse->short_description = strdup("a corpse");
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_VAL(corpse, 3) = 1;
  obj_to_room(corpse, 0);
  corpse_handle = domain_event_object_handle(corpse);
  loot = create_obj();
  obj_to_obj(loot, corpse);
  occupied.follower = &fixture.victim;
  SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_GOLEM);
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  fixture.victim.master = ch;
  for (mode = 0; mode < 7; mode++)
  {
    SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, mode == 0 ? 0 : 1);
    GET_OBJ_VAL(corpse, 4) = mode == 1 ? 123 : 0;
    CORPSE_ANIMATION_BLOCKED(corpse) = mode == 2;
    ch->followers = mode == 3 ? &occupied : NULL;
    top_of_mobt = mode == 4 ? 11 : 12;
    GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) = mode == 5 ? 0 : 100;
    for (i = 1; i < NUM_CRAFT_MOTES; i++)
      GET_CRAFT_MOTES(ch, i) = mode == 6 ? 0 : 100;
    newcraft_create(ch, "golem animate corpse");
    if (ch->followers != (mode == 3 ? &occupied : NULL) ||
        domain_event_world_resolve_object(corpse_handle) != corpse || corpse->contains != loot ||
        GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) != (mode == 5 ? 0 : 100) ||
        !is_action_available(ch, atSTANDARD, false))
      return false;
  }
  ch->followers = NULL;
  fixture.victim.master = NULL;
  top_of_mobt = 12;
  GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) = 100;
  for (i = 1; i < NUM_CRAFT_MOTES; i++)
    GET_CRAFT_MOTES(ch, i) = 100;
  SET_ABILITY(ch, ABILITY_ARCANA, 0);
  newcraft_create(ch, "create golem animate corpse");
  if (ch->followers != NULL || GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) != 60 ||
      GET_CRAFT_MOTES(ch, 1) != 94 || corpse->contains != loot ||
      is_action_available(ch, atSTANDARD, false))
    return false;
  clear_char_event_list(ch);
  SET_ABILITY(ch, ABILITY_ARCANA, 100);
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 0);
  SET_FEAT(ch, FEAT_SUMMON_GREATER_UNDEAD, 1);
  GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) = 100;
  for (i = 1; i < NUM_CRAFT_MOTES; i++)
    GET_CRAFT_MOTES(ch, i) = 100;
  newcraft_create(ch, "golem animate corpse");
  pet = ch->followers != NULL ? ch->followers->follower : NULL;
  if (pet == NULL || !IS_PET(pet) || GET_MOB_VNUM(pet) != PET_GOLEM_BONE ||
      GET_REAL_RACE(pet) != RACE_TYPE_CONSTRUCT || GET_DR_MOD(pet) != 3 || pet->carrying != loot ||
      domain_event_world_resolve_object(corpse_handle) != NULL ||
      GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) != 60 || GET_CRAFT_MOTES(ch, 1) != 94)
    return false;
  GET_HIT(pet)--;
  if (!can_repair_golem(ch, pet, &needed, &material) || needed != 4 || material != CRAFT_MAT_BONE)
    return false;
  do_destroygolem(ch, "bone", 0, 0);
  if (MOB_FLAGGED(pet, MOB_NOTDEADYET) || pet->carrying != loot ||
      GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) != 60)
    return false;
  obj_from_char(loot);
  obj_to_room(loot, 0);
  feedback.output = feedback.small_outbuf;
  feedback.bufspace = SMALL_BUFSIZE - 1;
  feedback.character = ch;
  feedback.pProtocol = ProtocolCreate();
  ch->desc = &feedback;
  do_destroygolem(ch, "bone", 0, 0);
  bone_refund_reported = strstr(feedback.output, "20 bone") != NULL;
  ch->desc = NULL;
  ProtocolDestroy(feedback.pProtocol);
  do_destroygolem(ch, "bone", 0, 0);
  if (!bone_refund_reported || GET_CRAFT_MAT(ch, CRAFT_MAT_BONE) != 80 ||
      GET_CRAFT_MOTES(ch, 1) != 94)
    return false;
  extract_pending_chars();
  top_of_mobt = 14;
  CONFIG_SUMMON_LEVEL_11_20_HP = 100;
  for (i = 0; i < 2; i++)
  {
    set_pet_summon_choice(ch, SPELL_PLANAR_ALLY, i == 0 ? "guardian" : "healer");
    mag_summons(15, ch, NULL, SPELL_PLANAR_ALLY, 0, CAST_SPELL);
    pet = ch->followers != NULL ? ch->followers->follower : NULL;
    if (pet == NULL || !IS_PET(pet) || !MOB_FLAGGED(pet, MOB_PLANAR_ALLY) ||
        GET_MOB_VNUM(pet) != (mob_vnum)(PET_CELESTIAL_GUARDIAN + i) ||
        GET_REAL_RACE(pet) != RACE_TYPE_OUTSIDER || GET_GOLD(pet) != 0 ||
        (i == 0 && GET_DR_MOD(pet) != 5) ||
        (i == 1 && (!MOB_KNOWS_SPELL(pet, SPELL_CURE_CRITIC) ||
                    !MOB_KNOWS_SPELL(pet, SPELL_REMOVE_PARALYSIS))))
    {
      snprintf(error, error_size, "authored planar ally %d failed acquisition or role checks", i);
      return false;
    }
    mag_summons(15, ch, NULL, SPELL_PLANAR_ALLY, 0, CAST_SPELL);
    if (ch->followers->follower != pet || ch->followers->next != NULL)
      return false;
    if (cast_spell(ch, NULL, NULL, SPELL_PLANAR_ALLY, 0) != 0)
      return false;
    if (i == 1)
    {
      int cast_index;
      struct descriptor_data feedback = {0};

      ch->player.name = (char *)"petcaller";
      feedback.output = feedback.small_outbuf;
      feedback.bufspace = SMALL_BUFSIZE - 1;
      feedback.character = ch;
      feedback.pProtocol = ProtocolCreate();
      ch->desc = &feedback;
      GET_MAX_HIT(ch) = GET_REAL_MAX_HIT(ch) = 200;
      if (complete_cmd_info == NULL)
        create_command_list();
      if (spell_info[SPELL_CURE_CRITIC].name == NULL ||
          spell_info[SPELL_CURE_CRITIC].name == unused_spellname)
        mag_assign_spells();
      if (!npc_can_cast(pet, SPELL_CURE_CRITIC) || npc_can_cast(pet, SPELL_MAGIC_MISSILE))
        return false;
      clear_char_event_list(ch);
      do_order(ch, "healer cast 'cure critic' absent", 0, 0);
      if (pet->mob_specials.known_spell_slots[SPELL_CURE_CRITIC] != 2 ||
          !is_action_available(pet, atSTANDARD, false) ||
          strstr(feedback.output, "find the target") == NULL)
        return false;
      for (cast_index = 0; cast_index < 3; cast_index++)
      {
        clear_char_event_list(ch);
        clear_char_event_list(pet);
        GET_HIT(ch) = 20;
        do_order(ch, "healer cast 'cure critic' petcaller", 0, 0);
        if ((cast_index < 2 && GET_HIT(ch) <= 20) || (cast_index == 2 && GET_HIT(ch) != 20) ||
            pet->mob_specials.known_spell_slots[SPELL_CURE_CRITIC] != MAX(0, 1 - cast_index))
        {
          snprintf(error, error_size, "ordered healer cast %d failed: hp=%d slots=%d: %.300s",
                   cast_index, GET_HIT(ch), pet->mob_specials.known_spell_slots[SPELL_CURE_CRITIC],
                   feedback.output);
          return false;
        }
        if (cast_index < 2 && is_action_available(pet, atSTANDARD, false))
          return false;
      }
      if (npc_can_cast(pet, SPELL_CURE_CRITIC) ||
          strstr(feedback.output, "Your pet cannot cast that spell") == NULL)
        return false;
      clear_char_event_list(ch);
      ch->desc = NULL;
      ProtocolDestroy(feedback.pProtocol);
    }
    extract_char(pet);
    extract_pending_chars();
  }
  top_of_mobt = 17;
  SET_FEAT(ch, FEAT_ANIMATE_DEAD, 3);
  GET_LEVEL(ch) = 9;
  do_animatedead(ch, "mage", 0, 0);
  do_animatedead(ch, "lich", 0, 0);
  if (ch->followers != NULL || daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) != 3)
    return false;
  GET_LEVEL(ch) = 31;
  top_of_mobt = 15;
  do_animatedead(ch, "mage", 0, 0);
  do_animatedead(ch, "lich", 0, 0);
  if (ch->followers != NULL || daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) != 3)
    return false;
  top_of_mobt = 17;
  for (i = 0; i < 2; i++)
  {
    do_animatedead(ch, i == 0 ? "mage" : "lich", 0, 0);
    pet = ch->followers != NULL ? ch->followers->follower : NULL;
    if (pet == NULL || !IS_PET(pet) || !MOB_FLAGGED(pet, MOB_ANIMATED_DEAD) ||
        GET_MOB_VNUM(pet) != (mob_vnum)(PET_SKELETAL_MAGE + i) ||
        GET_REAL_RACE(pet) != RACE_TYPE_UNDEAD || IS_INCORPOREAL(pet) ||
        !MOB_KNOWS_SPELL(pet, SPELL_MAGIC_MISSILE) ||
        !MOB_KNOWS_SPELL(pet, i == 0 ? SPELL_RAY_OF_ENFEEBLEMENT : SPELL_HASTE) ||
        daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) != 2 - i)
    {
      snprintf(error, error_size, "undead spellcaster %d acquisition or role failure", i);
      return false;
    }
    /* A lich cannot fit beside a lesser caster; two liches cannot fit either. */
    do_animatedead(ch, "lich", 0, 0);
    if (ch->followers->follower != pet || ch->followers->next != NULL ||
        daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) != 2 - i)
      return false;
    consume_known_spell_slot(pet, SPELL_MAGIC_MISSILE);
    consume_known_spell_slot(pet, SPELL_MAGIC_MISSILE);
    if (has_known_spell_slot(pet, SPELL_MAGIC_MISSILE))
      return false;
    extract_char(pet);
    extract_pending_chars();
    clear_char_event_list(ch);
    /* Restore only the consumed daily-use count after clearing fixture action events. */
    SET_FEAT(ch, FEAT_ANIMATE_DEAD, 2 - i);
  }
  top_of_mobt = 18;
  mag_summons(15, ch, NULL, SPELL_MISLEAD, 0, CAST_SPELL);
  pet = ch->followers != NULL ? ch->followers->follower : NULL;
  if (pet == NULL || !IS_PET(pet) || !is_illusory_pet(pet) || !IS_INCORPOREAL(pet) ||
      pet_order_check(ch, pet) || pet->pet_behavior != PET_BEHAVIOR_ASSIST ||
      char_has_mud_event(pet, ePURGEMOB) == NULL)
  {
    snprintf(error, error_size, "mislead decoy ownership, orderability, or expiry setup failed");
    return false;
  }
  mag_summons(15, ch, NULL, SPELL_MISLEAD, 0, CAST_SPELL);
  if (ch->followers->follower != pet || ch->followers->next != NULL ||
      cast_spell(ch, ch, NULL, SPELL_MISLEAD, 0) != 0)
    return false;
  pulse += 120 * PASSES_PER_SEC;
  event_test_advance();
  extract_pending_chars();
  if (ch->followers != NULL)
  {
    snprintf(error, error_size, "mislead decoy did not expire");
    return false;
  }
  return true;
}

void Test_gameplay_authored_construct_prototypes_load_and_complete_all_recipes(CuTest *tc)
{
  char error[512] = {0};
  const int legacy[] = {GOLEM_WOOD_SMALL, GOLEM_STONE_SMALL, GOLEM_IRON_SMALL};
  int type, size;
  struct class_spell_assign *assignment;
  int cleric_level = 0, summoner_level = 0;

  if (class_list[CLASS_CLERIC].name == NULL)
    load_class_list();
  for (assignment = class_list[CLASS_CLERIC].spellassign_list; assignment != NULL;
       assignment = assignment->next)
    if (assignment->spell_num == SPELL_PLANAR_ALLY)
      cleric_level = assignment->level;
  for (assignment = class_list[CLASS_SUMMONER].spellassign_list; assignment != NULL;
       assignment = assignment->next)
    if (assignment->spell_num == SPELL_PLANAR_ALLY)
      summoner_level = assignment->level;
  CuAssertIntEquals(tc, 11, cleric_level);
  CuAssertIntEquals(tc, 16, summoner_level);
  CuAssertIntEquals(tc, MOB_PLANAR_ALLY, summoned_follower_flag(SPELL_PLANAR_ALLY));
  for (type = 0; type < 3; type++)
    for (size = 0; size < 4; size++)
    {
      CuAssertIntEquals(tc, type + 1, get_golem_type_from_vnum(legacy[type] + size));
      CuAssertIntEquals(tc, size, get_golem_size_from_vnum(legacy[type] + size));
    }
  CuAssert(tc, error, spec_test_run_isolated(verify_authored_constructs, error, sizeof(error)));
}

void Test_gameplay_golem_minor_repairs_cost_materials_and_pending_destruction_cannot_pay(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data *ch = &fixture.actor, *golem = &fixture.victim;
  int needed = 0, material = 0, unit_cost, saved_system;
  bool empty_denied, small_repair, next_increment, pending_denied, no_reward;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &specials;
  golem->master = ch;
  SET_BIT_AR(AFF_FLAGS(golem), AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(golem), MOB_GOLEM);
  GET_MOB_RNUM(golem) = 0;
  fixture.mobile_index[0].vnum = PET_GOLEM_WOOD_SMALL;
  GET_REAL_MAX_HIT(golem) = 100;
  GET_HIT(golem) = GET_MAX_HIT(golem) - 1;
  unit_cost = get_golem_repair_material_cost(GOLEM_TYPE_WOOD, GOLEM_SIZE_SMALL);
  empty_denied = !can_repair_golem(ch, golem, &needed, &material);
  GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) = 100;
  small_repair = can_repair_golem(ch, golem, &needed, &material) && needed == unit_cost &&
                 material == CRAFT_MAT_MAPLE_WOOD;
  GET_HIT(golem) = GET_MAX_HIT(golem) * 89 / 100;
  next_increment = can_repair_golem(ch, golem, &needed, &material) && needed == 2 * unit_cost;
  SET_BIT_AR(MOB_FLAGS(golem), MOB_NOTDEADYET);
  pending_denied = !can_repair_golem(ch, golem, &needed, &material);
  saved_system = CONFIG_CRAFTING_SYSTEM;
  CONFIG_CRAFTING_SYSTEM = CRAFTING_SYSTEM_MOTES;
  do_destroygolem(ch, "victim", 0, 0);
  do_destroygolem(ch, "victim", 0, 0);
  no_reward =
      GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) == 100 && GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE) == 0;
  CONFIG_CRAFTING_SYSTEM = saved_system;
  REMOVE_BIT_AR(MOB_FLAGS(golem), MOB_NOTDEADYET);
  golem->master = NULL;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, empty_denied);
  CuAssertTrue(tc, small_repair);
  CuAssertTrue(tc, next_increment);
  CuAssertTrue(tc, pending_denied);
  CuAssertTrue(tc, no_reward);
}

void Test_gameplay_bonded_recall_preserves_existing_pet_state(CuTest *tc)
{
  const int roles[] = {MOB_C_ANIMAL, MOB_C_FAMILIAR, MOB_C_MOUNT,
                       MOB_C_DRAGON, MOB_SHADOW,     MOB_EIDOLON};
  const int classes[] = {CLASS_DRUID,       CLASS_WIZARD,        CLASS_PALADIN,
                         CLASS_DRAGONRIDER, CLASS_SHADOW_DANCER, CLASS_SUMMONER};
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct follow_type link = {0};
  struct char_data rider;
  struct obj_data *gear;
  struct char_data *pet;
  bool preserved = true, loyal = true, controlled = true, denied_respec, wrong_owner;
  size_t i;

  begin_gameplay_fixture(&f);
  pet = &f.victim;
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  GET_LEVEL(&f.actor) = 10;
  GET_PFILEPOS(&f.actor) = -1;
  GET_PREMADE_BUILD_CLASS(&f.actor) = CLASS_UNDEFINED;
  initialize_test_npc(&rider, "left behind rider", 1);
  GET_MOB_RNUM(pet) = 0;
  pet->master = &f.actor;
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  link.follower = pet;
  f.actor.followers = &link;
  GET_LEVEL(pet) = 7;
  GET_HIT(pet) = 17;
  GET_MOVE(pet) = 19;
  GET_PSP(pet) = 11;
  GET_REAL_MAX_HIT(pet) = 100;
  pet->pet_data_id = 42;
  gear = create_obj();
  obj_to_char(gear, pet);
  for (i = 0; i < sizeof(roles) / sizeof(roles[0]); i++)
  {
    SET_BIT_AR(MOB_FLAGS(pet), roles[i]);
    GET_CLASS(&f.actor) = classes[i];
    GET_LEVEL(&f.actor) = 11;
    CLASS_LEVEL((&f.actor), classes[i]) = 6;
    advance_level(&f.actor, classes[i]);
    char_from_room(pet);
    char_to_room(pet, 1);
    mount_char(&rider, pet);
    SET_FEAT(&f.actor, FEAT_BOON_COMPANION, 1);
    perform_call(&f.actor, roles[i], 30);
    perform_call(&f.actor, roles[i], 30);
    do_unfollow(pet, "", 0, 0);
    do_follow(pet, "self", 0, 0);
    loyal = loyal && pet->master == &f.actor && f.actor.followers == &link &&
            AFF_FLAGGED(pet, AFF_CHARM);
    controlled = controlled && pet_order_check(&f.actor, pet) && !pet_order_check(&rider, pet);
    REMOVE_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    controlled = controlled && !pet_order_check(&f.actor, pet);
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    preserved = preserved && IN_ROOM(pet) == 0 && pet->master == &f.actor &&
                f.actor.followers == &link && link.next == NULL && GET_LEVEL(pet) == 7 &&
                GET_HIT(pet) == 17 && GET_MOVE(pet) == 19 && GET_PSP(pet) == 11 &&
                GET_REAL_MAX_HIT(pet) == 100 && pet->pet_data_id == 42 && gear->carried_by == pet &&
                RIDING(&rider) == NULL && RIDDEN_BY(pet) == NULL;
    REMOVE_BIT_AR(MOB_FLAGS(pet), roles[i]);
    CLASS_LEVEL((&f.actor), classes[i]) = 0;
  }
  f.actor.desc = &descriptor;
  descriptor.character = &f.actor;
  do_respec(&f.actor, "wizard", 0, 0);
  denied_respec = GET_LEVEL(&f.actor) == 11 && f.actor.followers == &link &&
                  pet->master == &f.actor && GET_HIT(pet) == 17 && gear->carried_by == pet;
  f.actor.desc = NULL;
  SET_BIT_AR(MOB_FLAGS(pet), MOB_C_FAMILIAR);
  char_from_room(pet);
  char_to_room(pet, 1);
  pet->master = &rider;
  perform_call(&f.actor, MOB_C_FAMILIAR, 30);
  wrong_owner = IN_ROOM(pet) == 1 && pet->master == &rider;
  f.actor.followers = NULL;
  pet->master = NULL;
  extract_obj(gear);
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(pet);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, preserved);
  CuAssertTrue(tc, loyal);
  CuAssertTrue(tc, controlled);
  CuAssertTrue(tc, denied_respec);
  CuAssertTrue(tc, wrong_owner);
}

void Test_gameplay_pet_damage_feedback_honors_owner_preferences_and_presence(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct char_data owner;
  struct char_data *pet;
  bool correct = true;
  int illusion, mode;

  begin_gameplay_fixture(&f);
  initialize_test_npc(&owner, "pet owner", NOWHERE);
  REMOVE_BIT_AR(MOB_FLAGS(&owner), MOB_ISNPC);
  owner.player_specials = &specials;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &owner;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  owner.desc = &descriptor;
  char_to_room(&owner, 0);
  pet = &f.actor;
  pet->master = &owner;
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  for (illusion = 0; illusion < 2; illusion++)
  {
    pet->pet_source_spell = illusion ? SPELL_MISLEAD : 0;
    for (mode = 0; mode < 4; mode++)
    {
      char_from_room(&owner);
      char_to_room(&owner, mode == 2 ? 1 : 0);
      GET_POS(&owner) = mode == 3 ? POS_SLEEPING : POS_STANDING;
      if (mode == 0)
        REMOVE_BIT_AR(PRF_FLAGS(&owner), PRF_CHARMIE_COMBATROLL);
      else
        SET_BIT_AR(PRF_FLAGS(&owner), PRF_CHARMIE_COMBATROLL);
      descriptor.output[0] = '\0';
      descriptor.bufptr = 0;
      descriptor.bufspace = SMALL_BUFSIZE - 1;
      GET_HIT(&f.victim) = GET_MAX_HIT(&f.victim) = 1000;
      damage(pet, &f.victim, 7, TYPE_HIT, DAM_FORCE, FALSE);
      correct = correct && GET_HIT(&f.victim) == 993 &&
                ((strstr(descriptor.output, "[7]") != NULL) == (mode == 1));
    }
  }
  pet->master = NULL;
  char_from_room(&owner);
  owner.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  clear_char_event_list(&f.actor);
  clear_char_event_list(&f.victim);
  domain_event_world_forget_character(&owner);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, correct);
}

void Test_gameplay_natural_pet_attacks_bypass_only_eligible_damage_reduction(CuTest *tc)
{
  struct gameplay_fixture f;
  struct damage_reduction_type magic = {0}, material = {0}, physical = {0};
  struct char_data *pet, *target;
  struct obj_data *weapon;
  int ordinary, enchanted, evolved, mixed_first, mixed_last, unavoidable;
  int matching_physical, mismatched_physical, saved_damage_types;

  begin_gameplay_fixture(&f);
  pet = &f.actor;
  target = &f.victim;
  magic.amount = 8;
  magic.max_damage = -1;
  magic.bypass_cat[0] = DR_BYPASS_CAT_MAGIC;
  magic.bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
  material.amount = 5;
  material.max_damage = -1;
  material.bypass_cat[0] = DR_BYPASS_CAT_MATERIAL;
  material.bypass_val[0] = MATERIAL_SILVER;
  material.bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
  GET_DR(target) = &magic;
  ordinary = apply_damage_reduction(pet, target, NULL, 20, false);
  SET_BIT_AR(AFF2_FLAGS(pet), AFF2_MAGIC_ATTACKS);
  enchanted = apply_damage_reduction(pet, target, NULL, 20, false);
  REMOVE_BIT_AR(AFF2_FLAGS(pet), AFF2_MAGIC_ATTACKS);
  HAS_REAL_EVOLUTION(pet, EVOLUTION_MAGIC_ATTACKS) = 1;
  evolved = apply_damage_reduction(pet, target, NULL, 20, false);
  magic.next = &material;
  mixed_first = apply_damage_reduction(pet, target, NULL, 20, false);
  magic.next = NULL;
  material.next = &magic;
  GET_DR(target) = &material;
  mixed_last = apply_damage_reduction(pet, target, NULL, 20, false);
  material.bypass_cat[0] = DR_BYPASS_CAT_NONE;
  unavoidable = apply_damage_reduction(pet, target, NULL, 20, false);
  weapon = create_obj();
  GET_OBJ_TYPE(weapon) = ITEM_WEAPON;
  GET_OBJ_VAL(weapon, 0) = 0;
  saved_damage_types = weapon_list[0].damageTypes;
  weapon_list[0].damageTypes = DAMAGE_TYPE_BLUDGEONING;
  physical.amount = 6;
  physical.max_damage = -1;
  physical.bypass_cat[0] = DR_BYPASS_CAT_DAMTYPE;
  physical.bypass_cat[1] = DR_BYPASS_CAT_UNUSED;
  physical.bypass_val[0] = DR_DAMTYPE_BLUDGEONING;
  GET_DR(target) = &physical;
  matching_physical = apply_damage_reduction(pet, target, weapon, 20, false);
  physical.bypass_val[0] = DR_DAMTYPE_SLASHING;
  mismatched_physical = apply_damage_reduction(pet, target, weapon, 20, false);
  weapon_list[0].damageTypes = saved_damage_types;
  extract_obj(weapon);
  GET_DR(target) = NULL;
  end_gameplay_fixture(&f);
  CuAssertIntEquals(tc, 12, ordinary);
  CuAssertIntEquals(tc, 20, enchanted);
  CuAssertIntEquals(tc, 20, evolved);
  CuAssertIntEquals(tc, 15, mixed_first);
  CuAssertIntEquals(tc, 15, mixed_last);
  CuAssertIntEquals(tc, 15, unavoidable);
  CuAssertIntEquals(tc, 20, matching_physical);
  CuAssertIntEquals(tc, 14, mismatched_physical);
}

static void interrupt_juggernaut_arrival(const struct domain_event_context *context, void *data)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *pet = domain_event_world_resolve_character(event->character);
  bool *interrupted = data;

  if (pet != NULL && pet->pet_source_spell == PSIONIC_ECTOPLASMIC_SHAMBLER)
  {
    *interrupted = true;
    extract_char(pet);
  }
}

void Test_gameplay_juggernaut_failed_publication_retains_daily_use(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet, *owner;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct domain_event_subscription_config observer = {0};
  struct domain_event_subscription_handle subscription;
  bool interrupted = false, retained, spent, denied;
  int boosted[4] = {0}, ordinary[4] = {0};

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  owner = &f.actor;
  REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
  owner->player_specials = &specials;
  owner->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(owner) = -1;
  CLASS_LEVEL(owner, CLASS_PSIONICIST) = 20;
  add_char_perk(owner, PERK_PSIONICIST_ASTRAL_JUGGERNAUT, CLASS_PSIONICIST);
  add_char_perk(owner, PERK_PSIONICIST_HARDENED_CONSTRUCTS_I, CLASS_PSIONICIST);
  add_char_perk(owner, PERK_PSIONICIST_HARDENED_CONSTRUCTS_II, CLASS_PSIONICIST);
  f.mobile_index[0].vnum = MOB_ECTOPLASMIC_SHAMBLER;
  initialize_test_npc(&prototype, "shambler", NOWHERE);
  prototype.player.name = (char *)"shambler";
  GET_MOB_RNUM(&prototype) = 0;
  GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  mob_proto = &prototype;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  observer.type = DOMAIN_EVENT_CHARACTER_MOVED;
  observer.topic.role = DOMAIN_EVENT_TOPIC_DESTINATION;
  observer.topic.entity = domain_event_room_handle(0);
  observer.owner = domain_event_character_handle(owner);
  observer.identity = "test.juggernaut.interrupted-arrival";
  observer.handler = interrupt_juggernaut_arrival;
  observer.handler_context = &interrupted;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &observer, &subscription));
  circle_srandom(421);
  mag_summons(20, owner, NULL, PSIONIC_ECTOPLASMIC_SHAMBLER, 0, CAST_SPELL);
  extract_pending_chars();
  retained = interrupted && owner->followers == NULL && can_use_astral_juggernaut(owner);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_unsubscribe(domain_event_runtime_bus(), subscription));
  circle_srandom(421);
  mag_summons(20, owner, NULL, PSIONIC_ECTOPLASMIC_SHAMBLER, 0, CAST_SPELL);
  pet = owner->followers != NULL ? owner->followers->follower : NULL;
  spent = pet != NULL && GET_SIZE(pet) == SIZE_LARGE &&
          char_has_mud_event(owner, eASTRAL_JUGGERNAUT_USED) != NULL;
  circle_srandom(421);
  mag_summons(20, owner, NULL, PSIONIC_ECTOPLASMIC_SHAMBLER, 0, CAST_SPELL);
  denied = pet != NULL && owner->followers->follower == pet && owner->followers->next == NULL;
  if (pet != NULL)
  {
    boosted[0] = GET_HITROLL(pet);
    boosted[1] = GET_DAMROLL(pet);
    boosted[2] = GET_AC(pet);
    boosted[3] = GET_MAX_HIT(pet);
    CuAssertIntEquals(tc, 20, GET_HIT(pet) - GET_MAX_HIT(pet));
    CuAssertTrue(tc, AFF2_FLAGGED(pet, AFF2_MAGIC_ATTACKS));
    CuAssertPtrNotNull(tc, GET_DR(pet));
    CuAssertIntEquals(tc, 2, GET_DR(pet)->amount);
    GET_HIT(pet) = 1;
    affect_total(pet);
    affect_total(pet);
    CuAssertIntEquals(tc, boosted[0], GET_HITROLL(pet));
    CuAssertIntEquals(tc, boosted[1], GET_DAMROLL(pet));
    CuAssertIntEquals(tc, boosted[2], GET_AC(pet));
    CuAssertIntEquals(tc, boosted[3], GET_MAX_HIT(pet));
    CuAssertIntEquals(tc, SIZE_LARGE, GET_SIZE(pet));
    CuAssertIntEquals(tc, 1, GET_HIT(pet));
    extract_char(pet);
  }
  extract_pending_chars();
  circle_srandom(421);
  mag_summons(20, owner, NULL, PSIONIC_ECTOPLASMIC_SHAMBLER, 0, CAST_SPELL);
  pet = owner->followers != NULL ? owner->followers->follower : NULL;
  CuAssertPtrNotNull(tc, pet);
  ordinary[0] = GET_HITROLL(pet);
  ordinary[1] = GET_DAMROLL(pet);
  ordinary[2] = GET_AC(pet);
  ordinary[3] = GET_MAX_HIT(pet);
  CuAssertIntEquals(tc, 20, GET_HIT(pet) - GET_MAX_HIT(pet));
  extract_char(pet);
  extract_pending_chars();
  remove_all_perks(owner);
  circle_srandom(421);
  mag_summons(20, owner, NULL, PSIONIC_ECTOPLASMIC_SHAMBLER, 0, CAST_SPELL);
  pet = owner->followers != NULL ? owner->followers->follower : NULL;
  CuAssertPtrNotNull(tc, pet);
  CuAssertIntEquals(tc, GET_AC(pet) + 30, ordinary[2]);
  CuAssertIntEquals(tc, GET_MAX_HIT(pet), GET_HIT(pet));
  extract_char(pet);
  extract_pending_chars();
  clear_char_event_list(owner);
  if (owner->events != NULL)
    free_list(owner->events);
  remove_all_perks(owner);
  domain_event_world_forget_character(owner);
  domain_event_runtime_shutdown();
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, spent);
  CuAssertTrue(tc, denied);
  CuAssertIntEquals(tc, ordinary[0] + 4, boosted[0]);
  CuAssertIntEquals(tc, ordinary[1] + 4, boosted[1]);
  CuAssertIntEquals(tc, ordinary[2] + 40, boosted[2]);
  CuAssertIntEquals(tc, ordinary[3] + 40, boosted[3]);
}

static void verify_nature_summon_scaling(CuTest *tc, int mob_level, int scaling)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  int base[3], improved[3], recalculated[3];
  int mode, i;
  int saved_scaling[] = {CONFIG_SUMMON_LEVEL_1_10_HIT_DAM,  CONFIG_SUMMON_LEVEL_11_20_HIT_DAM,
                         CONFIG_SUMMON_LEVEL_21_30_HIT_DAM, CONFIG_SUMMON_LEVEL_1_10_AC,
                         CONFIG_SUMMON_LEVEL_11_20_AC,      CONFIG_SUMMON_LEVEL_21_30_AC};

  begin_gameplay_fixture(&f);
  CONFIG_SUMMON_LEVEL_1_10_HIT_DAM = CONFIG_SUMMON_LEVEL_11_20_HIT_DAM =
      CONFIG_SUMMON_LEVEL_21_30_HIT_DAM = scaling;
  CONFIG_SUMMON_LEVEL_1_10_AC = CONFIG_SUMMON_LEVEL_11_20_AC = CONFIG_SUMMON_LEVEL_21_30_AC =
      scaling;
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&f.actor) = -1;
  f.mobile_index[0].vnum = MOB_DIRE_BADGER;
  initialize_test_npc(&prototype, "badger", NOWHERE);
  prototype.player.name = (char *)"badger";
  GET_LEVEL(&prototype) = mob_level;
  GET_REAL_HITROLL(&prototype) = GET_HITROLL(&prototype) = 4;
  GET_REAL_DAMROLL(&prototype) = GET_DAMROLL(&prototype) = 6;
  GET_REAL_AC(&prototype) = prototype.points.armor = 100;
  GET_MOB_RNUM(&prototype) = 0;
  GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  mob_proto = &prototype;
  for (mode = 0; mode < 2; mode++)
  {
    if (mode == 1)
      add_char_perk(&f.actor, PERK_RANGER_SPELL_FOCUS_CONJURATION_I, CLASS_RANGER);
    circle_srandom(421);
    mag_summons(10, &f.actor, NULL, SPELL_SUMMON_NATURES_ALLY_1, 0, CAST_SPELL);
    pet = f.actor.followers != NULL ? f.actor.followers->follower : NULL;
    CuAssertPtrNotNull(tc, pet);
    if (mode == 0)
    {
      affect_total(pet);
      base[0] = GET_HITROLL(pet);
      base[1] = GET_DAMROLL(pet);
      base[2] = GET_REAL_AC(pet);
    }
    else
    {
      improved[0] = GET_HITROLL(pet);
      improved[1] = GET_DAMROLL(pet);
      improved[2] = GET_REAL_AC(pet);
      GET_HIT(pet) = 1;
      affect_total(pet);
      affect_total(pet);
      recalculated[0] = GET_HITROLL(pet);
      recalculated[1] = GET_DAMROLL(pet);
      recalculated[2] = GET_REAL_AC(pet);
      CuAssertIntEquals(tc, 1, GET_HIT(pet));
    }
    extract_char(pet);
    extract_pending_chars();
    clear_char_event_list(&f.actor);
  }
  remove_all_perks(&f.actor);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&f);
  CONFIG_SUMMON_LEVEL_1_10_HIT_DAM = saved_scaling[0];
  CONFIG_SUMMON_LEVEL_11_20_HIT_DAM = saved_scaling[1];
  CONFIG_SUMMON_LEVEL_21_30_HIT_DAM = saved_scaling[2];
  CONFIG_SUMMON_LEVEL_1_10_AC = saved_scaling[3];
  CONFIG_SUMMON_LEVEL_11_20_AC = saved_scaling[4];
  CONFIG_SUMMON_LEVEL_21_30_AC = saved_scaling[5];
  for (i = 0; i < 3; i++)
  {
    CuAssertIntEquals(tc, base[i] + scaling / 100, improved[i]);
    CuAssertIntEquals(tc, improved[i], recalculated[i]);
  }
}

void Test_gameplay_nature_summon_combat_bonuses_survive_recalculation_without_stacking(CuTest *tc)
{
  int level;

  for (level = 5; level <= 25; level += 10)
  {
    verify_nature_summon_scaling(tc, level, 100);
    verify_nature_summon_scaling(tc, level, 200);
  }
}

void Test_gameplay_alpha_bond_saves_survive_recalculation_without_stacking(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  int base[3], improved[3], recalculated[3], recalled[3];
  const int saves[] = {SAVING_FORT, SAVING_REFL, SAVING_WILL};
  int mode, i;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&f.actor) = -1;
  GET_ANIMAL_COMPANION(&f.actor) = MOB_DIRE_BADGER;
  f.mobile_index[0].vnum = MOB_DIRE_BADGER;
  initialize_test_npc(&prototype, "badger", NOWHERE);
  prototype.player.name = (char *)"badger";
  GET_MOB_RNUM(&prototype) = 0;
  GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  mob_proto = &prototype;
  for (mode = 0; mode < 2; mode++)
  {
    if (mode == 1)
      add_char_perk(&f.actor, PERK_RANGER_ALPHA_BOND, CLASS_RANGER);
    circle_srandom(421);
    perform_call(&f.actor, MOB_C_ANIMAL, 10);
    pet = f.actor.followers != NULL ? f.actor.followers->follower : NULL;
    CuAssertPtrNotNull(tc, pet);
    for (i = 0; i < 3; i++)
      if (mode == 0)
        base[i] = GET_SAVE(pet, saves[i]);
      else
        improved[i] = GET_SAVE(pet, saves[i]);
    if (mode == 1)
    {
      affect_total(pet);
      affect_total(pet);
      for (i = 0; i < 3; i++)
        recalculated[i] = GET_SAVE(pet, saves[i]);
      char_from_room(pet);
      char_to_room(pet, 1);
      perform_call(&f.actor, MOB_C_ANIMAL, 20);
      for (i = 0; i < 3; i++)
        recalled[i] = GET_SAVE(pet, saves[i]);
    }
    extract_char(pet);
    extract_pending_chars();
    clear_char_event_list(&f.actor);
  }
  remove_all_perks(&f.actor);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&f);
  for (i = 0; i < 3; i++)
  {
    CuAssertIntEquals(tc, base[i] + 3, improved[i]);
    CuAssertIntEquals(tc, improved[i], recalculated[i]);
    CuAssertIntEquals(tc, improved[i], recalled[i]);
  }
}

static bool verify_authored_lycanthropes(const char *sandbox, char *error, size_t error_size)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data *owner = &fixture.actor, *pet;
  struct obj_data *wand;
  FILE *file;
  char path[PATH_MAX], line[256];
  int i, vnum;

  (void)sandbox;
  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  mob_proto = calloc(2, sizeof(*mob_proto));
  mob_index = calloc(2, sizeof(*mob_index));
  if (mob_proto == NULL || mob_index == NULL)
    return false;
  snprintf(path, sizeof(path), "%s/data/pet-lycanthropes/195.mob", test_source_root());
  file = fopen(path, "r");
  if (file == NULL)
    return false;
  for (i = 0; i < 2; i++)
  {
    if (!get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 || vnum != PET_LYCAN_WEREWOLF + i)
      return false;
    parse_mobile(file, vnum);
    if (!MOB_FLAGGED(&mob_proto[i], MOB_ROL_LYCANTHROPE_SUMMON))
      return false;
  }
  fclose(file);
  top_of_mobt = 1;
  REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
  owner->player_specials = &specials;
  owner->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(owner) = -1;
  GET_LEVEL(owner) = 20;
  SET_SKILL(owner, ABILITY_USE_MAGIC_DEVICE, 100);
  if (spell_info[SPELL_CALL_LYCANTHROPE].name == NULL ||
      spell_info[SPELL_CALL_LYCANTHROPE].name == unused_spellname)
    mag_assign_spells();
  obj_proto = calloc(1, sizeof(*obj_proto));
  obj_index = calloc(1, sizeof(*obj_index));
  if (obj_proto == NULL || obj_index == NULL)
    return false;
  snprintf(path, sizeof(path), "%s/data/pet-lycanthropes/195.obj", test_source_root());
  file = fopen(path, "r");
  if (file == NULL || !get_line(file, line) || sscanf(line, "#%d", &vnum) != 1 ||
      vnum != PET_LYCAN_CALL_WAND)
    return false;
  parse_object(file, vnum);
  fclose(file);
  top_of_objt = 0;
  wand = read_object(PET_LYCAN_CALL_WAND, VIRTUAL);
  if (wand == NULL || GET_OBJ_TYPE(wand) != ITEM_WAND ||
      GET_OBJ_VAL(wand, 3) != SPELL_CALL_LYCANTHROPE || GET_OBJ_VAL(wand, 2) != 1)
    return false;
  equip_char(owner, wand, WEAR_HOLD_1);
  for (i = 0; i < 2; i++)
    REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[i]), MOB_ROL_LYCANTHROPE_SUMMON);
  do_use(owner, "mooncall self", 0, SCMD_USE);
  if (GET_OBJ_VAL(wand, 2) != 1 || owner->followers != NULL)
    return false;
  for (i = 0; i < 2; i++)
  {
    REMOVE_BIT_AR(MOB_FLAGS(&mob_proto[1 - i]), MOB_ROL_LYCANTHROPE_SUMMON);
    SET_BIT_AR(MOB_FLAGS(&mob_proto[i]), MOB_ROL_LYCANTHROPE_SUMMON);
    GET_OBJ_VAL(wand, 2) = 1;
    do_use(owner, "mooncall self", 0, SCMD_USE);
    pet = owner->followers != NULL ? owner->followers->follower : NULL;
    if (GET_OBJ_VAL(wand, 2) != 0 || pet == NULL || !IS_PET(pet) ||
        GET_MOB_VNUM(pet) != (mob_vnum)(PET_LYCAN_WEREWOLF + i) ||
        GET_RACE(pet) != RACE_TYPE_LYCANTHROPE || !pet_order_check(owner, pet) ||
        pet->pet_source_spell != SPELL_CALL_LYCANTHROPE || GET_GOLD(pet) != 0 ||
        char_has_mud_event(pet, eROL_CALL_LYCANTHROPE_CHARM) == NULL)
    {
      snprintf(error, error_size, "authored lycanthrope %d acquisition failed", i);
      return false;
    }
    GET_OBJ_VAL(wand, 2) = 1;
    do_use(owner, "mooncall self", 0, SCMD_USE);
    if (GET_OBJ_VAL(wand, 2) != 1 || owner->followers->follower != pet ||
        owner->followers->next != NULL)
      return false;
    pulse += 30 * PASSES_PER_SEC;
    event_test_advance();
    extract_pending_chars();
    if (owner->followers != NULL)
      return false;
  }
  return true;
}

void Test_gameplay_authored_lycanthropes_acquire_and_expire(CuTest *tc)
{
  char error[512] = {0};

  CuAssert(tc, error, spec_test_run_isolated(verify_authored_lycanthropes, error, sizeof(error)));
}

void Test_gameplay_lycanthrope_admission_expiry_and_control_break(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data prototype, *pet, *owner;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct domain_entity_handle pet_handle;
  unsigned long saved_pulse = pulse;
  bool missing, charmed, owned, denied, resolved;
  int mode;
  unsigned long seed;

  for (mode = 0; mode < 4; mode++)
  {
    begin_gameplay_fixture(&fixture);
    event_free_all();
    event_init();
    owner = &fixture.actor;
    initialize_test_npc(&prototype, "summoned lycanthrope", NOWHERE);
    SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
    GET_MOB_RNUM(&prototype) = 0;
    GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = 100;
    mob_proto = &prototype;
    spell_call_lycanthrope(20, owner, NULL, NULL, 0);
    missing = owner->followers == NULL && !can_call_lycanthrope(owner) &&
              cast_spell(owner, owner, NULL, SPELL_CALL_LYCANTHROPE, 0) == 0 &&
              !IS_CASTING(owner) && owner->primary_activity == NULL;
    SET_BIT_AR(MOB_FLAGS(&prototype), MOB_ROL_LYCANTHROPE_SUMMON);
    SET_BIT_AR(AFF_FLAGS(owner), AFF_CHARM);
    spell_call_lycanthrope(20, owner, NULL, NULL, 0);
    charmed = owner->followers == NULL && !can_call_lycanthrope(owner) &&
              cast_spell(owner, owner, NULL, SPELL_CALL_LYCANTHROPE, 0) == 0 &&
              !IS_CASTING(owner) && owner->primary_activity == NULL;
    REMOVE_BIT_AR(AFF_FLAGS(owner), AFF_CHARM);
    spell_call_lycanthrope(20, owner, NULL, NULL, 0);
    pet = owner->followers != NULL ? owner->followers->follower : NULL;
    owned = pet != NULL && IS_PET(pet) && pet->char_specials.is_charmie &&
            pet->pet_source_spell == SPELL_CALL_LYCANTHROPE &&
            char_has_mud_event(pet, eROL_CALL_LYCANTHROPE_CHARM) != NULL;
    spell_call_lycanthrope(20, owner, NULL, NULL, 0);
    denied = pet != NULL && owner->followers->follower == pet && owner->followers->next == NULL &&
             cast_spell(owner, owner, NULL, SPELL_CALL_LYCANTHROPE, 0) == 0 && !IS_CASTING(owner) &&
             owner->primary_activity == NULL;
    resolved = false;
    if (pet != NULL)
    {
      pet_handle = domain_event_character_handle(pet);
      if (mode != 0)
      {
        if (mode == 1)
        {
          char_from_room(pet);
          char_to_room(pet, 1);
        }
        FIGHTING(pet) = &fixture.victim;
        GET_CHA(owner) = mode == 3 ? 30 : 1;
        GET_HIT(owner) = GET_MAX_HIT(owner) = GET_REAL_MAX_HIT(owner) = 10000;
        for (seed = 1; seed < 1000; seed++)
        {
          circle_srandom(seed);
          if (rand_number(1, 20) < 20)
            break;
        }
        circle_srandom(seed);
      }
      pulse += 30 * PASSES_PER_SEC;
      event_test_advance();
      extract_pending_chars();
      pet = domain_event_world_resolve_character(pet_handle);
      if (mode == 0)
        resolved = pet == NULL && owner->followers == NULL;
      else if (mode == 1)
        resolved = pet != NULL && pet->master == NULL && !AFF_FLAGGED(pet, AFF_CHARM) &&
                   owner->followers == NULL && FIGHTING(pet) == NULL &&
                   pet->char_specials.is_charmie && IN_ROOM(pet) == 1;
      else if (mode == 2)
        resolved = pet != NULL && pet->master == NULL && !AFF_FLAGGED(pet, AFF_CHARM) &&
                   owner->followers == NULL && FIGHTING(pet) == owner &&
                   char_has_mud_event(pet, eROL_CALL_LYCANTHROPE_CHARM) == NULL;
      else
        resolved = pet != NULL && pet->master == owner && AFF_FLAGGED(pet, AFF_CHARM) &&
                   FIGHTING(pet) == &fixture.victim &&
                   char_has_mud_event(pet, eROL_CALL_LYCANTHROPE_CHARM) != NULL;
      if (pet != NULL)
        extract_char(pet);
      extract_pending_chars();
    }
    domain_event_world_forget_character(owner);
    event_free_all();
    mob_proto = saved_proto;
    character_list = saved_characters;
    pulse = saved_pulse;
    end_gameplay_fixture(&fixture);
    CuAssertTrue(tc, missing);
    CuAssertTrue(tc, charmed);
    CuAssertTrue(tc, owned);
    CuAssertTrue(tc, denied);
    CuAssertTrue(tc, resolved);
  }
}

void Test_gameplay_mercenary_recruited_hit_points_survive_recalculation(CuTest *tc)
{
  const int classes[] = {CLASS_WARRIOR, CLASS_ROGUE, CLASS_WIZARD};
  const int expected[] = {90, 60, 40};
  struct gameplay_fixture fixture;
  struct char_data *pet;
  int i, pass;

  for (i = 0; i < 3; i++)
  {
    begin_gameplay_fixture(&fixture);
    pet = &fixture.actor;
    pet->master = &fixture.victim;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    GET_CLASS(pet) = classes[i];
    GET_REAL_CON(pet) = pet->aff_abils.con = 10;
    GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) = 100;
    GET_HIT(pet) = 17;
    CuAssertIntEquals(tc, TRUE, mercenary(pet, pet, 0, ""));
    for (pass = 0; pass < 2; pass++)
    {
      affect_total(pet);
      CuAssertIntEquals(tc, expected[i], GET_REAL_MAX_HIT(pet));
      CuAssertIntEquals(tc, expected[i], GET_MAX_HIT(pet));
      CuAssertIntEquals(tc, 17, GET_HIT(pet));
      CuAssertIntEquals(tc, FALSE, mercenary(pet, pet, 0, ""));
    }
    pet->master = NULL;
    end_gameplay_fixture(&fixture);
  }
}

void Test_gameplay_eidolon_appearance_protects_defender_from_poison_and_disease(CuTest *tc)
{
  const int appearances[] = {EVOLUTION_UNDEAD_APPEARANCE, EVOLUTION_CELESTIAL_APPEARANCE};
  const int levels[] = {1, 6, 7, 11, 12, 20};
  struct gameplay_fixture fixture;
  struct char_data *attacker, *pet;
  int appearance, index, baseline, expected;

  for (appearance = 0; appearance < 2; appearance++)
  {
    for (index = 0; index < 6; index++)
    {
      begin_gameplay_fixture(&fixture);
      attacker = &fixture.actor;
      pet = &fixture.victim;
      GET_REAL_RACE(pet) = RACE_TYPE_OUTSIDER;
      GET_LEVEL(pet) = levels[index];
      baseline = get_poison_save_mod(attacker, pet);
      HAS_REAL_EVOLUTION(pet, appearances[appearance]) = 1;
      expected = levels[index] >= 12 ? 100 : levels[index] >= 7 ? 4 : 2;
      CuAssertIntEquals(tc, baseline + expected, get_poison_save_mod(attacker, pet));
      CuAssertIntEquals(tc, levels[index] < 12, can_poison(pet));
      CuAssertIntEquals(tc, levels[index] < 12, can_disease(pet));
      HAS_REAL_EVOLUTION(pet, appearances[appearance]) = 0;
      HAS_REAL_EVOLUTION(attacker, appearances[appearance]) = 1;
      CuAssertIntEquals(tc, baseline, get_poison_save_mod(attacker, pet));
      CuAssertTrue(tc, can_poison(pet));
      CuAssertTrue(tc, can_disease(pet));
      GET_REAL_RACE(pet) = RACE_TYPE_UNDEAD;
      HAS_REAL_EVOLUTION(pet, appearances[appearance]) = 1;
      CuAssertTrue(tc, !can_poison(pet));
      CuAssertTrue(tc, !can_disease(pet));
      end_gameplay_fixture(&fixture);
    }
  }
}

void Test_gameplay_eidolon_progression_bonuses_survive_recalculation(CuTest *tc)
{
  const int forms[] = {EIDOLON_BASE_FORM_AVIAN, EIDOLON_BASE_FORM_BIPED,
                       EIDOLON_BASE_FORM_QUADRUPED, EIDOLON_BASE_FORM_SERPENTINE,
                       EIDOLON_BASE_FORM_TAURIC};
  const int strength[] = {4, 6, 6, 4, 4};
  const int dexterity[] = {6, 4, 6, 6, 2};
  const int constitution[] = {4, 4, 4, 4, 6};
  struct gameplay_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *owner, *pet;
  int selection, bonus, pass, form;

  for (selection = 0; selection < 20; selection++)
  {
    form = selection / 4;
    begin_gameplay_fixture(&fixture);
    memset(&player_specials, 0, sizeof(player_specials));
    owner = &fixture.actor;
    pet = &fixture.victim;
    REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
    owner->player_specials = &player_specials;
    GET_EIDOLON_BASE_FORM(owner) = forms[form];
    SET_FEAT(owner, FEAT_GRAND_EIDOLON, (selection & 1) != 0);
    SET_FEAT(owner, FEAT_EPIC_EIDOLON, (selection & 2) != 0);
    bonus = ((selection & 1) ? 2 : 0) + ((selection & 2) ? 4 : 0);
    GET_REAL_INT(pet) = GET_REAL_WIS(pet) = GET_REAL_CHA(pet) = 10;
    GET_REAL_STR(pet) = GET_REAL_DEX(pet) = GET_REAL_CON(pet) = 10;
    GET_HIT(pet) = 17;
    GET_MOVE(pet) = 19;

    assign_eidolon_evolutions(owner, pet, false);
    for (pass = 0; pass < 2; pass++)
    {
      affect_total(pet);
      CuAssertIntEquals(tc, 10 + bonus, GET_INT(pet));
      CuAssertIntEquals(tc, 10 + bonus, GET_WIS(pet));
      CuAssertIntEquals(tc, 10 + bonus, GET_CHA(pet));
      CuAssertIntEquals(tc, 10 + strength[form] + bonus, GET_STR(pet));
      CuAssertIntEquals(tc, 10 + dexterity[form] + bonus, GET_DEX(pet));
      CuAssertIntEquals(tc, 10 + constitution[form] + bonus, GET_CON(pet));
      CuAssertIntEquals(tc, form != 2, HAS_REAL_FEAT(pet, FEAT_IRON_WILL));
      CuAssertIntEquals(tc, form == 1 || form == 2 || form == 4,
                        HAS_REAL_FEAT(pet, FEAT_GREAT_FORTITUDE));
      CuAssertIntEquals(tc, form != 1 && form != 4, HAS_REAL_FEAT(pet, FEAT_LIGHTNING_REFLEXES));
      CuAssertIntEquals(tc, 17, GET_HIT(pet));
      CuAssertIntEquals(tc, 19, GET_MOVE(pet));
    }
    end_gameplay_fixture(&fixture);
  }
}

void Test_gameplay_necromancer_calls_and_recalls_undead_cohort(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *owner, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  MYSQL *saved_connection = conn;
  bool denied, acquired, recalled = false;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  conn = NULL;
  owner = &fixture.actor;
  REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
  owner->player_specials = &specials;
  owner->player.name = (char *)"cohortcaller";
  owner->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(owner) = -1;
  GET_CLASS(owner) = CLASS_NECROMANCER;
  GET_LEVEL(owner) = CLASS_LEVEL(owner, CLASS_NECROMANCER) = 1;
  do_call(owner, "cohort", 0, 0);
  denied = owner->followers == NULL && char_has_mud_event(owner, eC_EIDOLON) == NULL;
  if (class_list[CLASS_NECROMANCER].name == NULL)
    load_class_list();
  process_class_level_feats(owner, CLASS_NECROMANCER);
  GET_LEVEL(owner) = CLASS_LEVEL(owner, CLASS_NECROMANCER) = 12;
  KNOWS_EVOLUTION(owner, EVOLUTION_UNDEAD_APPEARANCE) = 1;
  GET_EIDOLON_BASE_FORM(owner) = EIDOLON_BASE_FORM_BIPED;
  initialize_test_npc(&prototype, "cohort", NOWHERE);
  prototype.player.name = (char *)"cohort";
  GET_MOB_RNUM(&prototype) = 0;
  GET_REAL_RACE(&prototype) = RACE_TYPE_OUTSIDER;
  GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
  fixture.mobile_index[0].vnum = MOB_NUM_EIDOLON;
  mob_proto = &prototype;
  do_call(owner, "cohort", 0, 0);
  pet = owner->followers != NULL ? owner->followers->follower : NULL;
  acquired = pet != NULL && IS_PET(pet) && MOB_FLAGGED(pet, MOB_EIDOLON) && GET_LEVEL(pet) == 12 &&
             HAS_EVOLUTION(pet, EVOLUTION_UNDEAD_APPEARANCE) && !can_poison(pet) &&
             !can_disease(pet) && char_has_mud_event(owner, eC_EIDOLON) != NULL;
  if (pet != NULL)
  {
    GET_HIT(pet) = 17;
    GET_MOVE(pet) = 19;
    char_from_room(pet);
    char_to_room(pet, 1);
    do_call(owner, "cohort", 0, 0);
    do_call(owner, "cohort", 0, 0);
    recalled = owner->followers->follower == pet && owner->followers->next == NULL &&
               IN_ROOM(pet) == IN_ROOM(owner) && GET_HIT(pet) == 17 && GET_MOVE(pet) == 19;
    extract_char(pet);
    extract_pending_chars();
  }
  clear_char_event_list(owner);
  if (owner->events != NULL)
    free_list(owner->events);
  owner->events = NULL;
  domain_event_world_forget_character(owner);
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  conn = saved_connection;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, denied);
  CuAssertTrue(tc, acquired);
  CuAssertTrue(tc, recalled);
}

void Test_gameplay_dragonrider_calls_every_authored_mount_and_recalls_without_refresh(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials;
  struct char_data prototype, *owner, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  MYSQL *saved_connection = conn;
  int type;
  bool denied = true, acquired = true, recalled = true;

  if (class_list[CLASS_DRAGONRIDER].name == NULL)
    load_class_list();
  for (type = 1; type < NUM_DRAGON_TYPES; type++)
  {
    memset(&specials, 0, sizeof(specials));
    begin_gameplay_fixture(&fixture);
    event_free_all();
    event_init();
    conn = NULL;
    owner = &fixture.actor;
    REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
    owner->player_specials = &specials;
    owner->player.name = (char *)"dragoncaller";
    owner->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
    GET_PFILEPOS(owner) = -1;
    GET_CLASS(owner) = CLASS_DRAGONRIDER;
    GET_LEVEL(owner) = CLASS_LEVEL(owner, CLASS_DRAGONRIDER) = 1;
    process_class_level_feats(owner, CLASS_DRAGONRIDER);
    GET_LEVEL(owner) = 10;
    GET_DRAGON_RIDER_DRAGON_TYPE(owner) = type;
    fixture.mobile_index[0].vnum = PET_DRAGON_MOUNT_FIRST + type - 1;
    mob_proto = NULL;
    do_call(owner, "dragon", 0, 0);
    denied =
        denied && owner->followers == NULL && char_has_mud_event(owner, eC_DRAGONMOUNT) == NULL;
    initialize_test_npc(&prototype, "dragon", NOWHERE);
    prototype.player.name = (char *)"dragon";
    GET_MOB_RNUM(&prototype) = 0;
    GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
    mob_proto = &prototype;
    do_call(owner, "dragon", 0, 0);
    pet = owner->followers != NULL ? owner->followers->follower : NULL;
    acquired = acquired && pet != NULL && IS_PET(pet) && MOB_FLAGGED(pet, MOB_C_DRAGON) &&
               GET_LEVEL(pet) == 10 &&
               GET_MOB_VNUM(pet) == (mob_vnum)(PET_DRAGON_MOUNT_FIRST + type - 1) &&
               char_has_mud_event(owner, eC_DRAGONMOUNT) != NULL;
    if (pet != NULL)
    {
      GET_HIT(pet) = 17;
      GET_MOVE(pet) = 19;
      GET_PSP(pet) = 11;
      GET_LEVEL(owner) = 20;
      char_from_room(pet);
      char_to_room(pet, 1);
      do_call(owner, "dragon", 0, 0);
      do_call(owner, "dragon", 0, 0);
      recalled = recalled && owner->followers->follower == pet && owner->followers->next == NULL &&
                 IN_ROOM(pet) == IN_ROOM(owner) && GET_LEVEL(pet) == 10 && GET_HIT(pet) == 17 &&
                 GET_MOVE(pet) == 19 && GET_PSP(pet) == 11;
      extract_char(pet);
      extract_pending_chars();
    }
    clear_char_event_list(owner);
    if (owner->events != NULL)
      free_list(owner->events);
    owner->events = NULL;
    domain_event_world_forget_character(owner);
    event_free_all();
    mob_proto = saved_proto;
    character_list = saved_characters;
    conn = saved_connection;
    end_gameplay_fixture(&fixture);
  }
  CuAssertTrue(tc, denied);
  CuAssertTrue(tc, acquired);
  CuAssertTrue(tc, recalled);
}

void Test_gameplay_companion_creation_sets_bond_and_preserves_failed_call_cooldown(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet;
  struct char_data *saved_prototypes = mob_proto;
  struct char_data *saved_characters = character_list;
  bool failed, bonded, recalled, blocked_corpse = false;
  struct obj_data *corpse;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"caller";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_FAMILIAR(&fixture.actor) = MOB_DIRE_BADGER;
  fixture.mobile_index[0].vnum = MOB_DIRE_BADGER;
  mob_proto = NULL;
  perform_call(&fixture.actor, MOB_C_FAMILIAR, 10);
  failed =
      fixture.actor.followers == NULL && char_has_mud_event(&fixture.actor, eC_FAMILIAR) == NULL;
  initialize_test_npc(&prototype, "familiar", NOWHERE);
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"familiar";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  perform_call(&fixture.actor, MOB_C_FAMILIAR, 10);
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  bonded = pet != NULL && MOB_FLAGGED(pet, MOB_C_FAMILIAR) && IS_PET(pet) &&
           char_has_mud_event(&fixture.actor, eC_FAMILIAR) != NULL;
  recalled = false;
  if (pet != NULL)
  {
    char_from_room(pet);
    char_to_room(pet, 1);
    perform_call(&fixture.actor, MOB_C_FAMILIAR, 10);
    recalled = IN_ROOM(pet) == 0 && fixture.actor.followers->next == NULL;
    /* Failed saving must not allow a released called pet to supply another summon. */
    stop_follower(pet);
    raw_kill(pet, &fixture.victim);
    extract_pending_chars();
    corpse = fixture.rooms[0].contents;
    blocked_corpse = corpse != NULL && IS_CORPSE(corpse) && !corpse_can_be_animated(corpse);
    if (corpse != NULL)
      extract_obj(corpse);
  }
  clear_char_event_list(&fixture.actor);
  if (fixture.actor.events != NULL)
    free_list(fixture.actor.events);
  fixture.actor.events = NULL;
  event_free_all();
  character_list = saved_characters;
  mob_proto = saved_prototypes;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, failed);
  CuAssertTrue(tc, bonded);
  CuAssertTrue(tc, recalled);
  CuAssertTrue(tc, blocked_corpse);
}

static void verify_item_pet_acquisition(CuTest *tc, bool horn)
{
  struct gameplay_fixture fixture;
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct obj_data *item;
  struct obj_special_ability ability = {0};
  int kind = horn ? ITEM_SPECAB_HORN_OF_SUMMONING : ITEM_SPECAB_ITEM_SUMMON;
  int failed_uses, acquired_uses, repeated_uses;
  bool acquired;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  initialize_special_abilities();
  item = create_obj();
  ability.value[0] = RETAINER_MOB_VNUM;
  fixture.mobile_index[0].vnum = RETAINER_MOB_VNUM;
  mob_proto = NULL;
  if (horn)
    item_specab_horn_of_summoning(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  else
    item_specab_item_summon(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  failed_uses = daily_item_specab_uses_remaining(item, kind);

  initialize_test_npc(&prototype, "summoned follower", NOWHERE);
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"follower";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  if (horn)
    item_specab_horn_of_summoning(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  else
    item_specab_item_summon(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  acquired = pet != NULL && IS_PET(pet) && IN_ROOM(pet) == 0;
  acquired_uses = daily_item_specab_uses_remaining(item, kind);
  if (horn)
    item_specab_horn_of_summoning(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  else
    item_specab_item_summon(&ability, item, &fixture.actor, NULL, ACTMTD_USE);
  repeated_uses = daily_item_specab_uses_remaining(item, kind);
  if (pet != NULL)
  {
    extract_char(pet);
    extract_pending_chars();
  }
  extract_obj(item);
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, acquired);
  CuAssertIntEquals(tc, 2, failed_uses);
  CuAssertIntEquals(tc, 1, acquired_uses);
  CuAssertIntEquals(tc, 1, repeated_uses);
}

void Test_gameplay_summoning_horn_accepts_room_zero_and_charges_only_success(CuTest *tc)
{
  verify_item_pet_acquisition(tc, true);
}

void Test_gameplay_summoning_item_accepts_room_zero_and_charges_only_success(CuTest *tc)
{
  verify_item_pet_acquisition(tc, false);
}

void Test_gameplay_retainer_call_preserves_cooldown_and_rejects_remote_duplicate(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet, *ch = &fixture.actor;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  bool failed, acquired, duplicate;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &specials;
  ch->player.name = (char *)"squire";
  ch->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(ch) = -1;
  GET_CHA(ch) = 18;
  SET_FEAT(ch, FEAT_BG_SQUIRE, 1);
  fixture.mobile_index[0].vnum = RETAINER_MOB_VNUM;
  mob_proto = NULL;
  do_retainer(ch, "call", 0, 0);
  failed = ch->followers == NULL && GET_RETAINER_COOLDOWN(ch) == 0;
  initialize_test_npc(&prototype, "retainer", NOWHERE);
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"retainer";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  do_retainer(ch, "call", 0, 0);
  pet = ch->followers != NULL ? ch->followers->follower : NULL;
  acquired = pet != NULL && IS_PET(pet) && GET_RETAINER_COOLDOWN(ch) == 100;
  duplicate = false;
  if (pet != NULL)
  {
    char_from_room(pet);
    char_to_room(pet, 1);
    GET_RETAINER_COOLDOWN(ch) = 0;
    do_retainer(ch, "call", 0, 0);
    duplicate = ch->followers->follower == pet && ch->followers->next == NULL &&
                GET_RETAINER_COOLDOWN(ch) == 0;
    extract_char(pet);
    extract_pending_chars();
  }
  mob_proto = saved_proto;
  character_list = saved_characters;
  domain_event_world_forget_character(ch);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, failed);
  CuAssertTrue(tc, acquired);
  CuAssertTrue(tc, duplicate);
}

void Test_gameplay_innate_animation_sets_source_flag_and_retains_failed_use(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet, *ch = &fixture.actor;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct follow_type lesser = {0};
  bool failed, partial_capacity, acquired, duplicate, rejected_choices, lesser_choice;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &specials;
  ch->player.name = (char *)"animator";
  ch->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(ch) = -1;
  GET_LEVEL(ch) = 31;
  SET_FEAT(ch, FEAT_ANIMATE_DEAD, 2);
  fixture.mobile_index[0].vnum = MOB_MUMMY;
  mob_proto = NULL;
  do_animatedead(ch, "", 0, 0);
  failed = ch->followers == NULL && char_has_mud_event(ch, eANIMATEDEAD) == NULL;
  initialize_test_npc(&prototype, "animated mummy", NOWHERE);
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"mummy";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  /* A mummy needs two points; the one remaining point must not spend a daily use. */
  SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_ANIMATED_DEAD);
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  fixture.victim.master = ch;
  lesser.follower = &fixture.victim;
  ch->followers = &lesser;
  do_animatedead(ch, "", 0, 0);
  partial_capacity = ch->followers == &lesser && daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) == 2;
  ch->followers = NULL;
  fixture.victim.master = NULL;
  do_animatedead(ch, "", 0, 0);
  pet = ch->followers != NULL ? ch->followers->follower : NULL;
  acquired = pet != NULL && MOB_FLAGGED(pet, MOB_ANIMATED_DEAD) &&
             daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) == 1;
  do_animatedead(ch, "", 0, 0);
  duplicate = pet != NULL && ch->followers->next == NULL &&
              daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) == 1;
  if (pet != NULL)
  {
    extract_char(pet);
    extract_pending_chars();
  }
  do_animatedead(ch, "dragon", 0, 0);
  do_animatedead(ch, "zombie extra", 0, 0);
  do_animatedead(ch, "ghoul", 0, 0); /* Missing selected prototype. */
  GET_LEVEL(ch) = 9;
  do_animatedead(ch, "mummy", 0, 0);
  rejected_choices = ch->followers == NULL && daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) == 1;
  GET_LEVEL(ch) = 31;
  fixture.mobile_index[0].vnum = MOB_GHOUL;
  ch->followers = &lesser;
  fixture.victim.master = ch;
  do_animatedead(ch, "ghoul", 0, 0);
  pet = ch->followers != &lesser ? ch->followers->follower : NULL;
  lesser_choice = pet != NULL && GET_MOB_VNUM(pet) == MOB_GHOUL && IS_PET(pet) &&
                  pet->char_specials.is_charmie && daily_uses_remaining(ch, FEAT_ANIMATE_DEAD) == 0;
  if (pet != NULL)
  {
    extract_char(pet);
    extract_pending_chars();
  }
  ch->followers = NULL;
  fixture.victim.master = NULL;
  clear_char_event_list(ch);
  if (ch->events != NULL)
    free_list(ch->events);
  ch->events = NULL;
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  domain_event_world_forget_character(ch);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, failed);
  CuAssertTrue(tc, partial_capacity);
  CuAssertTrue(tc, acquired);
  CuAssertTrue(tc, duplicate);
  CuAssertTrue(tc, rejected_choices);
  CuAssertTrue(tc, lesser_choice);
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
  const struct trail_data_list *trails;
  struct trail_data *trail;
  char name[32];
  size_t trail_count;
  int i;

  begin_gameplay_fixture(&fixture);

  movement_trail_record_at_room(&fixture.rooms[0], "repeat walker", "human", DIR_NONE, NORTH, 100);
  movement_trail_record_at_room(&fixture.rooms[0], "repeat walker", "human", DIR_NONE, NORTH, 200);
  trails = movement_trails_at_room(&fixture.rooms[0]);
  trail_count = count_live_movement_trails();
  CuAssertIntEquals(tc, 1, (int)trail_count);
  CuAssertPtrNotNull(tc, trails);
  CuAssertIntEquals(tc, 200, (int)trails->head->age);

  for (i = 0; i < TRAIL_MAX_PER_ROOM + 5; i++)
  {
    snprintf(name, sizeof(name), "walker %d", i);
    movement_trail_record_at_room(&fixture.rooms[0], name, "human", DIR_NONE, NORTH, 300 + i);
  }
  trails = movement_trails_at_room(&fixture.rooms[0]);
  trail_count = count_live_movement_trails();
  trail = trails->head;

  CuAssertIntEquals(tc, TRAIL_MAX_PER_ROOM, (int)trail_count);
  CuAssertTrue(tc, trail != NULL);
  CuAssertIntEquals(tc, 300 + TRAIL_MAX_PER_ROOM + 4, (int)trail->age);
  CuAssertTrue(tc, trails->tail != NULL);

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

static struct char_data *pet_order_test_owner;
static struct char_data *pet_order_test_other;
static struct char_data *pet_order_test_last;
static int pet_order_test_mode;
static int pet_order_test_dispatches;

/* Exercise callbacks through the real command table and interpreter. */
ACMD(pet_order_test_command)
{
  pet_order_test_last = ch;
  pet_order_test_dispatches++;
  if (pet_order_test_dispatches != 1)
    return;
  if (pet_order_test_mode == 1)
  {
    extract_char(pet_order_test_other);
    extract_pending_chars();
    pet_order_test_other = NULL;
  }
  else if (pet_order_test_mode == 2)
  {
    char_from_room(pet_order_test_owner);
    char_to_room(pet_order_test_owner, 1);
  }
  else if (pet_order_test_mode == 3)
    REMOVE_BIT_AR(AFF_FLAGS(pet_order_test_other), AFF_CHARM);
}

static void verify_pet_group_orders(CuTest *tc, int mode, int expected)
{
  struct gameplay_fixture fixture;
  struct char_data *saved_characters = character_list;
  struct char_data *saved_prototypes = mob_proto;
  struct char_data prototype;
  struct command_info saved_command;
  bool created_command_list, charged_once, selection = true;
  int command, dispatched, repeated;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  created_command_list = complete_cmd_info == NULL;
  if (created_command_list)
    create_command_list();
  command = find_command("say");
  saved_command = complete_cmd_info[command];
  complete_cmd_info[command].command_pointer = pet_order_test_command;

  fixture.actor.player.name = (char *)"owner";
  fixture.victim.player.name = (char *)"companion";
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  fixture.victim.master = &fixture.actor;
  pet_order_test_owner = &fixture.actor;
  initialize_test_npc(&prototype, "another companion", NOWHERE);
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  prototype.player.name = (char *)"companion";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = 100; /* Native prototype HP upper bound. */
  GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  pet_order_test_other = read_mobile(0, REAL);
  char_to_room(pet_order_test_other, 0);
  add_follower(pet_order_test_other, &fixture.actor);
  SET_BIT_AR(AFF_FLAGS(pet_order_test_other), AFF_CHARM);
  /* The first pet's command changes a later candidate. */
  fixture.rooms[0].people = &fixture.actor;
  fixture.actor.next_in_room = &fixture.victim;
  fixture.victim.next_in_room = pet_order_test_other;
  pet_order_test_other->next_in_room = NULL;
  pet_order_test_dispatches = 0;
  pet_order_test_mode = mode;

  do_order(&fixture.actor, "followers say ready", 0, 0);
  dispatched = pet_order_test_dispatches;
  charged_once = !is_action_available(&fixture.actor, atSWIFT, FALSE) &&
                 complete_cmd_info[find_command("order")].actions_required == ACTION_SWIFT;
  do_order(&fixture.actor, "followers say again", 0, 0);
  repeated = pet_order_test_dispatches;

  if (mode == 4 || mode == 5)
  {
    pet_order_test_other->pet_data_id = 902;
    clear_char_event_list(&fixture.actor);
    do_order(&fixture.actor, mode == 5 ? "#902 say selected" : "2.companion say selected", 0, 0);
    selection =
        pet_order_test_dispatches == expected + 1 && pet_order_test_last == pet_order_test_other;
    clear_char_event_list(&fixture.actor);
    REMOVE_BIT_AR(AFF_FLAGS(pet_order_test_other), AFF_CHARM);
    do_order(&fixture.actor, mode == 5 ? "#902 say denied" : "2.companion say denied", 0, 0);
    selection = selection && pet_order_test_dispatches == expected + 1 &&
                is_action_available(&fixture.actor, atSWIFT, FALSE);
  }

  if (pet_order_test_other != NULL)
  {
    extract_char(pet_order_test_other);
    extract_pending_chars();
  }
  character_list = saved_characters;
  pet_order_test_other = NULL;
  pet_order_test_last = NULL;
  pet_order_test_owner = NULL;
  fixture.victim.master = NULL;
  clear_char_event_list(&fixture.actor);
  if (fixture.actor.events != NULL)
  {
    free_list(fixture.actor.events);
    fixture.actor.events = NULL;
  }
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  complete_cmd_info[command] = saved_command;
  if (created_command_list)
    free_command_list();
  event_free_all();
  end_gameplay_fixture(&fixture);
  mob_proto = saved_prototypes;

  CuAssertIntEquals(tc, expected, dispatched);
  CuAssertIntEquals(tc, expected, repeated);
  CuAssertTrue(tc, charged_once);
  CuAssertTrue(tc, selection);
}

void Test_gameplay_pet_group_order_uses_one_owner_action(CuTest *tc)
{
  verify_pet_group_orders(tc, 0, 2);
}

void Test_gameplay_pet_shop_unavailable_stock_preserves_payment(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype;
  struct char_data *saved_proto = mob_proto;
  bool created_commands, handled, invalid_index;
  int remaining_gold;

  begin_gameplay_fixture(&fixture);
  created_commands = complete_cmd_info == NULL;
  if (created_commands)
    create_command_list();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  GET_GOLD(&fixture.actor) = 10000;
  GET_CHA(&fixture.actor) = 10;
  fixture.actor.next_in_room = NULL;
  fixture.rooms[1].people = &fixture.victim;
  IN_ROOM(&fixture.victim) = 1;
  fixture.victim.player.name = "puppy";
  GET_MOB_RNUM(&fixture.victim) = NOBODY;
  initialize_test_npc(&prototype, "puppy", NOWHERE);
  GET_MOB_RNUM(&prototype) = 0;
  mob_proto = &prototype;
  handled = pet_shops(&fixture.actor, NULL, find_command("buy"), "puppy");
  remaining_gold = GET_GOLD(&fixture.actor);
  invalid_index = read_mobile(NOBODY, REAL) == NULL && read_mobile((mob_vnum)-2, REAL) == NULL &&
                  fixture.mobile_index[0].number == 0;
  if (created_commands)
    free_command_list();
  end_gameplay_fixture(&fixture);
  mob_proto = saved_proto;
  CuAssertTrue(tc, handled);
  CuAssertTrue(tc, invalid_index);
  CuAssertIntEquals(tc, 10000, remaining_gold);
}

void Test_gameplay_pet_token_admission_checks_carrier(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype;
  struct char_data *saved_proto = mob_proto;
  struct obj_data token = {0};
  struct index_data object_index = {0};
  struct index_data *saved_obj_index = obj_index;
  obj_rnum saved_top = top_of_objt;
  struct follow_type link = {0};
  bool retained;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  GET_CHA(&fixture.actor) = 10;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  fixture.victim.master = &fixture.actor;
  link.follower = &fixture.victim;
  fixture.actor.followers = &link;
  initialize_test_npc(&prototype, "puppy", NOWHERE);
  GET_MOB_RNUM(&prototype) = 0;
  mob_proto = &prototype;
  object_index.vnum = fixture.mobile_index[0].vnum;
  obj_index = &object_index;
  top_of_objt = 0;
  GET_OBJ_RNUM(&token) = 0;
  token.carried_by = &fixture.actor;
  /* A callback actor can have capacity while the actual token carrier does not. */
  retained = !bought_pet(&fixture.victim, &token, 0, "") && token.carried_by == &fixture.actor &&
             fixture.mobile_index[0].number == 0;
  fixture.actor.followers = NULL;
  fixture.victim.master = NULL;
  obj_index = saved_obj_index;
  top_of_objt = saved_top;
  mob_proto = saved_proto;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, retained);
}

static void interrupt_purchased_pet_arrival(const struct domain_event_context *context, void *data)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *pet = domain_event_world_resolve_character(event->character);
  bool *controlled_arrival = data;

  if (pet != NULL && IS_NPC(pet))
  {
    *controlled_arrival = IS_PET(pet) && pet->char_specials.is_charmie;
    extract_char(pet);
  }
}

static void verify_pet_shop_payment_and_recovery(CuTest *tc, int currency, bool producing)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct account_data account = {0};
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct index_data object_index = {0}, *saved_obj_index = obj_index;
  obj_rnum saved_top = top_of_objt;
  struct obj_data *token, *stock_token;
  struct obj_data object_prototype = {0}, *saved_object_proto = obj_proto;
  struct domain_entity_handle token_handle;
  struct domain_event_subscription_config observer = {0};
  struct domain_event_subscription_handle subscription;
  bool purchased, owned, retained, controlled_arrival = false;
  struct shop_data shop = {0}, *saved_shops = shop_index;
  int saved_top_shop = top_shop;
  obj_vnum products[] = {NOTHING, NOTHING};
  room_vnum shop_rooms[] = {100, NOWHERE};
  struct follow_type occupied = {0};
  char buy_argument[] = "token";
  int buy_command, paid_balance;
  bool missing_denied, capacity_denied, stock_preserved;

  begin_gameplay_fixture(&fixture);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  descriptor.character = &fixture.actor;
  descriptor.account = &account;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  fixture.actor.desc = &descriptor;
  account.experience = currency == ITEM_ACCOUNT_EXP ? 10000 : 0;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_CHA(&fixture.actor) = 10;
  GET_REAL_STR(&fixture.actor) = fixture.actor.aff_abils.str = 18;
  GET_REAL_DEX(&fixture.actor) = fixture.actor.aff_abils.dex = 18;
  fixture.actor.player.name = (char *)"petbuyer";
  GET_GOLD(&fixture.actor) = currency == 0 ? 10000 : 0;
  GET_QUESTPOINTS(&fixture.actor) = currency == ITEM_QUEST ? 10000 : 0;
  initialize_test_npc(&prototype, "purchased companion", NOWHERE);
  GET_MOB_RNUM(&prototype) = 0;
  GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = GET_PSP(&prototype) = 100;
  mob_proto = &prototype;
  object_index.vnum = fixture.mobile_index[0].vnum;
  object_index.number = 1;
  object_index.func = bought_pet;
  obj_proto = &object_prototype;
  obj_index = &object_index;
  top_of_objt = 0;
  token = create_obj();
  GET_OBJ_RNUM(token) = 0;
  token->name = strdup("token");
  token->short_description = strdup("a companion token");
  GET_OBJ_TYPE(token) = ITEM_PET;
  GET_OBJ_COST(token) = 1000;
  if (currency == ITEM_QUEST)
    SET_BIT_AR(GET_OBJ_EXTRA(token), ITEM_QUEST);
  else if (currency == ITEM_ACCOUNT_EXP)
    SET_BIT_AR(GET_OBJ_EXTRA(token), ITEM_ACCOUNT_EXP);
  clear_object(&object_prototype);
  GET_OBJ_RNUM(&object_prototype) = 0;
  object_prototype.name = token->name;
  object_prototype.short_description = token->short_description;
  object_prototype.obj_flags = token->obj_flags;
  if (producing)
    products[0] = 0;
  stock_token = token;
  obj_to_char(token, &fixture.victim);
  GET_MOB_RNUM(&fixture.victim) = 0;
  shop.keeper = 0;
  shop.in_room = shop_rooms;
  shop.producing = products;
  shop.profit_buy = 1.0;
  shop.close1 = 24;
  shop.lastsort = 1;
  shop_index = &shop;
  top_shop = 0;
  if (complete_cmd_info == NULL)
    create_command_list();
  buy_command = find_command("buy");
  mob_proto = NULL;
  shop_keeper(&fixture.actor, &fixture.victim, buy_command, buy_argument);
  missing_denied = GET_GOLD(&fixture.actor) == (currency == 0 ? 10000 : 0) &&
                   GET_QUESTPOINTS(&fixture.actor) == (currency == ITEM_QUEST ? 10000 : 0) &&
                   account.experience == (currency == ITEM_ACCOUNT_EXP ? 10000 : 0) &&
                   token->carried_by == &fixture.victim;
  mob_proto = &prototype;
  fixture.victim.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  occupied.follower = &fixture.victim;
  fixture.actor.followers = &occupied;
  shop_keeper(&fixture.actor, &fixture.victim, buy_command, buy_argument);
  capacity_denied = GET_GOLD(&fixture.actor) == (currency == 0 ? 10000 : 0) &&
                    GET_QUESTPOINTS(&fixture.actor) == (currency == ITEM_QUEST ? 10000 : 0) &&
                    account.experience == (currency == ITEM_ACCOUNT_EXP ? 10000 : 0) &&
                    token->carried_by == &fixture.victim;
  fixture.actor.followers = NULL;
  fixture.victim.master = NULL;
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  token_handle = domain_event_object_handle(token);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  observer.type = DOMAIN_EVENT_CHARACTER_MOVED;
  observer.topic.role = DOMAIN_EVENT_TOPIC_DESTINATION;
  observer.topic.entity = domain_event_room_handle(0);
  observer.owner = domain_event_character_handle(&fixture.actor);
  observer.identity = "test.pet-purchase.interrupted-arrival";
  observer.handler = interrupt_purchased_pet_arrival;
  observer.handler_context = &controlled_arrival;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &observer, &subscription));
  shop_keeper(&fixture.actor, &fixture.victim, buy_command, buy_argument);
  if (producing)
  {
    token = fixture.actor.carrying;
    token_handle = domain_event_object_handle(token);
  }
  paid_balance = currency == ITEM_ACCOUNT_EXP ? account.experience
                 : currency == ITEM_QUEST     ? GET_QUESTPOINTS(&fixture.actor)
                                              : GET_GOLD(&fixture.actor);
  retained = paid_balance > 0 && paid_balance < 10000;
  extract_pending_chars();
  retained = retained && controlled_arrival && fixture.actor.followers == NULL &&
             domain_event_world_resolve_object(token_handle) == token && token != NULL &&
             token->carried_by == &fixture.actor;
  stock_preserved = !producing || (token != stock_token && fixture.victim.carrying == stock_token &&
                                   stock_token->carried_by == &fixture.victim &&
                                   stock_token->next_content == NULL && object_index.number == 2);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_unsubscribe(domain_event_runtime_bus(), subscription));
  spec_gateway_object_automatic_activity(token);
  purchased = fixture.actor.followers != NULL &&
              GET_GOLD(&fixture.actor) == (currency == 0 ? paid_balance : 0) &&
              GET_QUESTPOINTS(&fixture.actor) == (currency == ITEM_QUEST ? paid_balance : 0) &&
              account.experience == (currency == ITEM_ACCOUNT_EXP ? paid_balance : 0);
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  owned = pet != NULL && pet->master == &fixture.actor && IS_PET(pet) &&
          pet->char_specials.is_charmie && IN_ROOM(pet) == IN_ROOM(&fixture.actor) &&
          domain_event_world_resolve_object(token_handle) == NULL;
  stock_preserved = stock_preserved && object_index.number == (producing ? 1 : 0) &&
                    (producing || fixture.victim.carrying == NULL);
  if (producing)
    extract_obj(stock_token);
  free(object_prototype.name);
  free(object_prototype.short_description);
  if (pet != NULL)
    extract_char(pet);
  extract_pending_chars();
  domain_event_world_forget_character(&fixture.actor);
  domain_event_runtime_shutdown();
  event_free_all();
  obj_index = saved_obj_index;
  obj_proto = saved_object_proto;
  top_of_objt = saved_top;
  mob_proto = saved_proto;
  character_list = saved_characters;
  shop_index = saved_shops;
  top_shop = saved_top_shop;
  ProtocolDestroy(descriptor.pProtocol);
  fixture.actor.desc = NULL;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, stock_preserved);
  CuAssertTrue(tc, purchased);
  CuAssertTrue(tc, owned);
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, missing_denied);
  CuAssertTrue(tc, capacity_denied);
}

void Test_gameplay_pet_purchase_commits_token_and_establishes_summon_ownership(CuTest *tc)
{
  verify_pet_shop_payment_and_recovery(tc, 0, false);
  verify_pet_shop_payment_and_recovery(tc, 0, true);
  verify_pet_shop_payment_and_recovery(tc, ITEM_QUEST, false);
  verify_pet_shop_payment_and_recovery(tc, ITEM_QUEST, true);
  verify_pet_shop_payment_and_recovery(tc, ITEM_ACCOUNT_EXP, false);
  verify_pet_shop_payment_and_recovery(tc, ITEM_ACCOUNT_EXP, true);
}

static void verify_native_summon_batch(CuTest *tc, int spell, int expected, bool selected)
{
  struct gameplay_fixture fixture;
  struct descriptor_data feedback = {0};
  struct char_data prototypes[4], *pet;
  struct index_data indexes[4] = {0};
  struct char_data *saved_proto = mob_proto;
  struct char_data *saved_characters = character_list;
  unsigned long seed;
  int first_count, repeated_count, i, rolled;
  bool staged_cleaned, correct_form = true, full_denied, missing_denied;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  for (i = 0; i < 4; i++)
  {
    initialize_test_npc(&prototypes[i], "summoned creature", NOWHERE);
    prototypes[i].player.name = "summoned creature";
    SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_CUSTOM_MOB_STATS);
    GET_MOB_RNUM(&prototypes[i]) = i;
    GET_PSP(&prototypes[i]) = GET_REAL_MAX_HIT(&prototypes[i]) = GET_REAL_MAX_MOVE(&prototypes[i]) =
        100;
    /* Native spell prototypes: swarm elementals 9412-9415, shambler 9499. */
    indexes[i].vnum = spell == SPELL_ELEMENTAL_SWARM ? PET_SWARM_AIR + i
                      : spell == SPELL_SHAMBLER      ? PET_SHAMBLER + i
                                                     : MOB_FIRE_ELEMENTAL + i;
  }
  mob_proto = prototypes;
  mob_index = indexes;
  top_of_mobt = 3;
  if (selected)
    set_pet_summon_choice(&fixture.actor, spell, "water");
  /* Find a seed yielding the authored maximum after the fail-message roll. */
  for (seed = 1; seed < 1000; seed++)
  {
    circle_srandom(seed);
    (void)rand_number(2, 6);
    if (spell == SPELL_ELEMENTAL_SWARM && !selected)
      (void)rand_number(0, 3);
    rolled = spell == SPELL_ELEMENTAL_SWARM ? dice(2, 4)
             : spell == SPELL_SHAMBLER      ? dice(1, 4) + 2
                                            : rand_number(0, 101) >= 10;
    if (rolled == expected)
      break;
  }
  circle_srandom(seed);
  mag_summons(20, &fixture.actor, NULL, spell, 0, 0);
  first_count = check_npc_followers(&fixture.actor, NPC_MODE_COUNT, 0);
  feedback.output = feedback.small_outbuf;
  feedback.bufspace = SMALL_BUFSIZE - 1;
  feedback.character = &fixture.actor;
  feedback.pProtocol = ProtocolCreate();
  fixture.actor.desc = &feedback;
  full_denied = cast_spell(&fixture.actor, NULL, NULL, spell, 0) == 0 &&
                fixture.actor.primary_activity == NULL &&
                strstr(feedback.output, "control allowance is full") != NULL;
  mag_summons(20, &fixture.actor, NULL, spell, 0, 0);
  repeated_count = check_npc_followers(&fixture.actor, NPC_MODE_COUNT, 0);
  while (fixture.actor.followers != NULL)
  {
    if (selected)
      correct_form = correct_form &&
                     GET_MOB_VNUM(fixture.actor.followers->follower) ==
                         (spell == SPELL_ELEMENTAL_SWARM ? PET_SWARM_WATER : MOB_WATER_ELEMENTAL);
    extract_char(fixture.actor.followers->follower);
    extract_pending_chars();
  }
  top_of_mobt = -1;
  feedback.small_outbuf[0] = '\0';
  feedback.bufptr = 0;
  feedback.bufspace = SMALL_BUFSIZE - 1;
  missing_denied = cast_spell(&fixture.actor, NULL, NULL, spell, 0) == 0 &&
                   fixture.actor.primary_activity == NULL &&
                   strstr(feedback.output, "chosen summon is unavailable") != NULL;
  top_of_mobt = 3;
  fixture.actor.desc = NULL;
  ProtocolDestroy(feedback.pProtocol);
  pet = read_mobile(0, REAL);
  staged_cleaned = pet != NULL && IN_ROOM(pet) == NOWHERE && attach_follower(pet, &fixture.actor);
  if (pet != NULL)
  {
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
    extract_char(pet);
    extract_pending_chars();
  }
  staged_cleaned =
      staged_cleaned && fixture.actor.followers == NULL && character_list == saved_characters;
  for (i = 0; i < 4; i++)
    staged_cleaned = staged_cleaned && indexes[i].number == 0;
  domain_event_world_forget_character(&fixture.actor);
  event_free_all();
  end_gameplay_fixture(&fixture);
  mob_proto = saved_proto;
  CuAssertIntEquals(tc, expected, first_count);
  CuAssertIntEquals(tc, expected, repeated_count);
  CuAssertTrue(tc, staged_cleaned);
  CuAssertTrue(tc, correct_form);
  CuAssertTrue(tc, full_denied);
  CuAssertTrue(tc, missing_denied);
}

void Test_gameplay_shambler_cast_preserves_full_batch_and_staged_cleanup(CuTest *tc)
{
  verify_native_summon_batch(tc, SPELL_SHAMBLER, 6, false);
}

void Test_gameplay_elemental_swarm_preserves_full_batch_and_staged_cleanup(CuTest *tc)
{
  verify_native_summon_batch(tc, SPELL_ELEMENTAL_SWARM, 8, false);
  verify_native_summon_batch(tc, SPELL_ELEMENTAL_SWARM, 8, true);
}

void Test_gameplay_geniekind_denials_preserve_existing_benefits(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct affected_type previous;
  struct char_data prototype;
  struct char_data *saved_proto = mob_proto;
  struct follow_type link = {0};
  bool missing_retained, full_retained, missing_cast_denied, full_cast_denied;

  begin_gameplay_fixture(&f);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.desc = &descriptor;
  descriptor.character = &f.actor;
  initialize_test_npc(&prototype, "genie", NOWHERE);
  mob_proto = &prototype;
  f.mobile_index[0].vnum = MOB_DJINNI_KIND;
  new_affect(&previous);
  previous.spell = SPELL_DJINNI_KIND;
  previous.location = APPLY_SPECIAL;
  previous.duration = 17;
  affect_to_char(&f.actor, &previous);
  set_pet_summon_choice(&f.actor, SPELL_GENIEKIND, "marid");
  missing_cast_denied = cast_spell(&f.actor, NULL, NULL, SPELL_GENIEKIND, 0) == 0;
  spell_geniekind(20, &f.actor, NULL, NULL, CAST_SPELL);
  missing_retained = affected_by_spell(&f.actor, SPELL_DJINNI_KIND) &&
                     !affected_by_spell(&f.actor, SPELL_MARID_KIND) &&
                     f.actor.affected->duration == 17 && f.actor.followers == NULL;
  set_pet_summon_choice(&f.actor, SPELL_GENIEKIND, "djinni");
  link.follower = &f.victim;
  f.actor.followers = &link;
  f.victim.master = &f.actor;
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_GENIEKIND);
  full_cast_denied = cast_spell(&f.actor, NULL, NULL, SPELL_GENIEKIND, 0) == 0;
  spell_geniekind(20, &f.actor, NULL, NULL, CAST_SPELL);
  full_retained = f.actor.affected->duration == 17 && f.actor.followers == &link;
  f.actor.followers = NULL;
  f.victim.master = NULL;
  affect_from_char(&f.actor, SPELL_DJINNI_KIND);
  f.actor.desc = NULL;
  domain_event_world_forget_character(&f.actor);
  end_gameplay_fixture(&f);
  mob_proto = saved_proto;
  CuAssertTrue(tc, missing_cast_denied);
  CuAssertTrue(tc, missing_retained);
  CuAssertTrue(tc, full_cast_denied);
  CuAssertTrue(tc, full_retained);
}

void Test_gameplay_elemental_choices_are_owned_by_each_cast_and_preserve_limits(CuTest *tc)
{
  struct char_data first = {0}, second = {0};
  struct player_special_data specials = {0};
  const int spells[] = {SPELL_SUMMON_CREATURE_7,     SPELL_SUMMON_CREATURE_8,
                        SPELL_SUMMON_CREATURE_9,     SPELL_SUMMON_NATURES_ALLY_7,
                        SPELL_SUMMON_NATURES_ALLY_8, SPELL_SUMMON_NATURES_ALLY_9};
  size_t i;

  first.player_specials = &specials;
  for (i = 0; i < sizeof(spells) / sizeof(spells[0]); i++)
  {
    CuAssertTrue(tc, set_pet_summon_choice(&first, spells[i], "air"));
    CuAssertTrue(tc, set_pet_summon_choice(&second, spells[i], "earth"));
    CuAssertIntEquals(tc, MOB_AIR_ELEMENTAL, pet_summon_choice_mob(&first, spells[i]));
    CuAssertIntEquals(tc, MOB_EARTH_ELEMENTAL, pet_summon_choice_mob(&second, spells[i]));
    verify_native_summon_batch(tc, spells[i], 1, true);
  }
  CuAssertTrue(tc, set_pet_summon_choice(&first, SPELL_GENIEKIND, "marid"));
  CuAssertTrue(tc, set_pet_summon_choice(&second, SPELL_GENIEKIND, "efreeti"));
  CuAssertIntEquals(tc, MOB_MARID_KIND, pet_summon_choice_mob(&first, SPELL_GENIEKIND));
  CuAssertIntEquals(tc, MOB_EFREETI_KIND, pet_summon_choice_mob(&second, SPELL_GENIEKIND));
  CuAssertTrue(tc, !set_pet_summon_choice(&first, SPELL_GENIEKIND, "not-a-genie"));
  CuAssertIntEquals(tc, NOBODY, pet_summon_choice_mob(&first, SPELL_GENIEKIND));
  set_pet_summon_choice(&first, SPELL_GENIEKIND, "djinni");
  resetCastingData(&first);
  CuAssertIntEquals(tc, NOBODY, pet_summon_choice_mob(&first, SPELL_GENIEKIND));
  CuAssertIntEquals(tc, NOBODY, pet_summon_choice_mob(&second, SPELL_ELEMENTAL_SWARM));
}

void Test_gameplay_dismiss_refuses_gear_and_preserves_pets_after_save_failure(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct obj_data gear = {0};
  struct follow_type link = {0};
  bool gear_retained, failed_save_retained;

  begin_gameplay_fixture(&fixture);
  fixture.victim.player.name = "companion";
  fixture.victim.master = &fixture.actor;
  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  fixture.victim.carrying = &gear;
  gear.carried_by = &fixture.victim;
  link.follower = &fixture.victim;
  fixture.actor.followers = &link;
  do_dismiss(&fixture.actor, "companion", 0, 0);
  gear_retained = !MOB_FLAGGED(&fixture.victim, MOB_NOTDEADYET) && fixture.victim.carrying == &gear;
  fixture.victim.carrying = NULL;
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = "owner";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  fixture.victim.pet_data_id = 901;
  do_dismiss(&fixture.actor, "#901", 0, 0);
  failed_save_retained = !MOB_FLAGGED(&fixture.victim, MOB_NOTDEADYET) &&
                         AFF_FLAGGED(&fixture.victim, AFF_CHARM) &&
                         fixture.victim.master == &fixture.actor;
  fixture.actor.followers = NULL;
  fixture.victim.master = NULL;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, gear_retained);
  CuAssertTrue(tc, failed_save_retained);
}

static void verify_expiring_pet_assets(CuTest *tc, int mode)
{
  struct gameplay_fixture fixture;
  struct char_data prototype;
  struct char_data *charmie;
  struct char_data *saved_prototypes = mob_proto;
  struct char_data *saved_characters = character_list;
  struct obj_data *carried;
  struct obj_data *equipped, *nested, *object;
  struct domain_entity_handle pet_handle;
  unsigned long saved_pulse = pulse;
  int dropped_count = 0;
  bool assets_dropped;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  initialize_test_npc(&prototype, "dismissed charmie", NOWHERE);
  prototype.player.name = (char *)"charmie";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = 100;
  GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  charmie = read_mobile(0, REAL);
  CuAssertPtrNotNull(tc, charmie);
  char_to_room(charmie, 0);
  pet_handle = domain_event_character_handle(charmie);
  charmie->char_specials.is_charmie = true;
  carried = create_obj();
  equipped = create_obj();
  GET_OBJ_TYPE(carried) = ITEM_CONTAINER;
  nested = create_obj();
  obj_to_obj(nested, carried);
  obj_to_char(carried, charmie);
  equip_char(charmie, equipped, WEAR_NECK_1);

  if (mode == 0)
    extract_char(charmie);
  else
  {
    SET_BIT_AR(MOB_FLAGS(charmie), MOB_C_ANIMAL);
    SET_BIT_AR(AFF_FLAGS(charmie), AFF_CHARM);
    CuAssertTrue(tc, attach_follower(charmie, &fixture.actor));
    char_from_room(&fixture.actor);
    char_to_room(&fixture.actor, 1);
    if (mode == 1)
      attach_mud_event(new_mud_event(ePURGEMOB, charmie, NULL), 12 * PASSES_PER_SEC);
    else
      stop_follower(charmie);
    pulse += 12 * PASSES_PER_SEC;
    event_test_advance();
  }
  extract_pending_chars();
  assets_dropped = IN_ROOM(carried) == 0 && carried->carried_by == NULL && IN_ROOM(equipped) == 0 &&
                   equipped->worn_by == NULL && nested->in_obj == carried &&
                   carried->contains == nested && fixture.actor.followers == NULL &&
                   domain_event_world_resolve_character(pet_handle) == NULL;
  for (object = fixture.rooms[0].contents; object != NULL; object = object->next_content)
    dropped_count++;

  extract_obj(carried);
  extract_obj(equipped);
  domain_event_world_forget_character(&fixture.actor);
  event_free_all();
  pulse = saved_pulse;
  character_list = saved_characters;
  mob_proto = saved_prototypes;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, assets_dropped);
  CuAssertIntEquals(tc, 2, dropped_count);
}

void Test_gameplay_extracting_a_present_charmie_drops_its_assets(CuTest *tc)
{
  int mode;

  for (mode = 0; mode < 3; mode++)
    verify_expiring_pet_assets(tc, mode);
}

void Test_gameplay_pet_group_order_survives_target_extraction(CuTest *tc)
{
  verify_pet_group_orders(tc, 1, 1);
}

void Test_gameplay_pet_group_order_rechecks_owner_location(CuTest *tc)
{
  verify_pet_group_orders(tc, 2, 1);
}

void Test_gameplay_pet_group_order_rechecks_control(CuTest *tc)
{
  verify_pet_group_orders(tc, 3, 1);
}

void Test_gameplay_pet_id_orders_require_a_loyal_target(CuTest *tc)
{
  verify_pet_group_orders(tc, 5, 2);
}

void Test_gameplay_pet_numbered_orders_require_a_loyal_target(CuTest *tc)
{
  verify_pet_group_orders(tc, 4, 2);
}

void Test_gameplay_e2e_cexchange_preserves_hidden_sneaking(CuTest *tc)
{
  struct gameplay_fixture fixture;
  bool created_command_list;
  bool remained_hidden;
  bool remained_sneaking;
  char command[] = "cexchange";

  begin_gameplay_fixture(&fixture);
  created_command_list = false;
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_command_list = true;
  }
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_HIDE);
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_SNEAK);

  command_interpreter(&fixture.actor, command);
  remained_hidden = AFF_FLAGGED(&fixture.actor, AFF_HIDE);
  remained_sneaking = AFF_FLAGGED(&fixture.actor, AFF_SNEAK);

  if (created_command_list)
    free_command_list();
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, remained_hidden);
  CuAssertTrue(tc, remained_sneaking);
}

void Test_gameplay_e2e_winters_war_march_failed_save_slow_expires(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct affected_type forced_failure;
  struct affected_type *effect;
  struct char_data *bard;
  struct char_data *saved_character_list;
  int slow_duration;
  int update;
  bool immunity_absent;
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
  affect_to_char(bard, &forced_failure);

  test_apply_bard_winters_war_march_verse(bard);

  slow_duration = -1;
  for (effect = fixture.victim.affected; effect; effect = effect->next)
  {
    if (effect->spell == AFFECT_BARD_WINTERS_WAR_MARCH)
      slow_duration = effect->duration;
  }
  immunity_absent = !affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH_IMMUNITY);

  saved_character_list = character_list;
  fixture.victim.next = NULL;
  character_list = &fixture.victim;
  affected_registry_attach(&fixture.victim);
  for (update = 0; update < 4; update++)
    affect_update_character_one(&fixture.victim);
  slow_removed = !affected_by_spell(&fixture.victim, AFFECT_BARD_WINTERS_WAR_MARCH);
  affected_registry_detach(&fixture.victim);
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
  CuAssertTrue(tc, immunity_absent);
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
  int loaded_mission_cooldown;
  int loaded_race;
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
  bool cooldown_checkpoint_written;
  bool cooldown_checkpoint_backdated;
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
  GET_REAL_RACE(source) = RACE_YUAN_TI;
  GET_GOLD(source) = 12345;
  GET_MISSION_COOLDOWN(source) = 10;
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
  loaded_mission_cooldown = -1;
  loaded_race = RACE_UNDEFINED;
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
  cooldown_checkpoint_written = false;
  cooldown_checkpoint_backdated = false;
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
      cooldown_checkpoint_written = player_file_has_cooldown_checkpoint(filename);
      cooldown_checkpoint_backdated =
          rewrite_player_cooldown_checkpoint(filename, (int64_t)time(NULL) - 120);
      if (cooldown_checkpoint_backdated)
        load_result = load_char(player_name, loaded);
      if (load_result >= 0)
      {
        loaded_level = GET_LEVEL(loaded);
        loaded_gold = GET_GOLD(loaded);
        loaded_mission_cooldown = GET_MISSION_COOLDOWN(loaded);
        loaded_race = GET_REAL_RACE(loaded);
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
  CuAssertTrue(tc, cooldown_checkpoint_written);
  CuAssertTrue(tc, cooldown_checkpoint_backdated);
  CuAssertTrue(tc, load_result >= 0);
  CuAssertTrue(tc, loaded_name_matches);
  CuAssertIntEquals(tc, 7, loaded_level);
  CuAssertIntEquals(tc, 12345, loaded_gold);
  CuAssertIntEquals(tc, 0, loaded_mission_cooldown);
  CuAssertIntEquals(tc, RACE_YUAN_TI, loaded_race);
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
        extract_script(&fixture.rooms[0].script);
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
  int saved_top_of_trigt;
  int damage_result;
  int remaining_hit_points;
  bool parsed;
  bool attachment_found;
  char line[MAX_INPUT_LENGTH];
  char mobile_path[PATH_MAX];
  char trigger_path[PATH_MAX];
  FILE *mobile_file;
  FILE *trigger_file;

  begin_gameplay_fixture(&fixture);
  saved_trig_index = trig_index;
  saved_top_of_trigt = top_of_trigt;
  saved_trigger_list = trigger_list;
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = NULL;
  mobile_file = NULL;
  parsed = false;
  attachment_found = false;
  damage_result = -1;
  remaining_hit_points = GET_HIT(&fixture.victim);

  if (trig_index != NULL &&
      snprintf(trigger_path, sizeof(trigger_path), "%s/lib/world/minimal/0.trg",
               test_source_root()) < (int)sizeof(trigger_path) &&
      snprintf(mobile_path, sizeof(mobile_path), "%s/lib/world/minimal/0.mob", test_source_root()) <
          (int)sizeof(mobile_path))
  {
    trigger_file = fopen(trigger_path, "r");
    mobile_file = fopen(mobile_path, "r");
    if (trigger_file != NULL && get_line(trigger_file, line) && strcmp(line, "#1") == 0)
    {
      parse_trigger(trigger_file, 1);
      parsed = top_of_trigt == 1 && trig_index[0] != NULL &&
               IS_SET(GET_TRIG_TYPE((struct trig_data *)trig_index[0]->proto), MTRIG_DAMAGE);
    }
    while (mobile_file != NULL && get_line(mobile_file, line))
      if (strcmp(line, "T 1") == 0)
        attachment_found = true;

    if (parsed)
    {
      fixture.victim.script = calloc(1, sizeof(*fixture.victim.script));
      if (fixture.victim.script != NULL)
      {
        add_trigger(fixture.victim.script, read_trigger(0), -1);
        FIGHTING(&fixture.actor) = &fixture.victim;
        FIGHTING(&fixture.victim) = &fixture.actor;
        damage_result = damage(&fixture.actor, &fixture.victim, 40, TYPE_HIT, DAM_BLUDGEON, FALSE);
        remaining_hit_points = GET_HIT(&fixture.victim);
        extract_script(&fixture.victim.script);
      }
    }
  }

  if (GET_ID(&fixture.actor) != 0)
    remove_from_lookup_table(GET_ID(&fixture.actor));
  if (GET_ID(&fixture.victim) != 0)
    remove_from_lookup_table(GET_ID(&fixture.victim));
  if (trigger_file != NULL)
    fclose(trigger_file);
  if (mobile_file != NULL)
    fclose(mobile_file);

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
  CuAssertTrue(tc, attachment_found);
  CuAssertIntEquals(tc, 25, damage_result);
  CuAssertIntEquals(tc, 75, remaining_hit_points);
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
        extract_script(&parsed_world[i].script);
      free_proto_script(&parsed_world[i].proto_script);
      spec_binding_free(&parsed_world[i].spec_binding);
      free_room_strings(&parsed_world[i]);
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

struct ready_combat_trace
{
  int hits;
  int lost;
};

static void ready_combat_damage(const struct domain_event_context *context, void *data)
{
  struct ready_combat_trace *trace = data;
  const struct domain_character_damaged *damage = context->payload;

  if (damage->amount > 0)
  {
    trace->hits++;
    trace->lost += damage->amount;
  }
}

static void verify_readied_cast_outcome(CuTest *tc, int outcome)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data *saved_characters = character_list;
  struct spell_info_type saved_spell = spell_info[SPELL_CURE_LIGHT];
  int saved_mode = CONFIG_SPELLCASTING_TIME_MODE;
  unsigned long saved_pulse = pulse;
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct ready_combat_trace trace = {0};
  struct attack_action_data *queued;
  struct ready_action_latency latency;
  unsigned int tick;
  struct primary_activity_snapshot cast;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "watcher";
  f.victim.player.name = "caster";
  f.actor.next = &f.victim;
  character_list = &f.actor;
  f.rooms[0].light = 1;
  GET_HITROLL(&f.actor) = outcome == 2 ? -100 : 100;
  GET_DAMROLL(&f.actor) = outcome == 1 ? 20 : 100;
  if (outcome == 1)
    GET_LEVEL(&f.victim) = 100;
  GET_CLASS(&f.actor) = CLASS_WARRIOR;
  CLASS_LEVEL((&f.actor), CLASS_WARRIOR) = 10;
  GET_CLASS(&f.victim) = CLASS_CLERIC;
  GET_HIT(&f.victim) = GET_MAX_HIT(&f.victim) = 100000;
  GET_ATTACK_QUEUE(&f.actor) = create_attack_queue();
  CONFIG_SPELLCASTING_TIME_MODE = 1;
  memset(&spell_info[SPELL_CURE_LIGHT], 0, sizeof(spell_info[SPELL_CURE_LIGHT]));
  spell_info[SPELL_CURE_LIGHT].name = "cure light";
  spell_info[SPELL_CURE_LIGHT].min_position = POS_FIGHTING;
  spell_info[SPELL_CURE_LIGHT].targets = TAR_CHAR_ROOM;
  spell_info[SPELL_CURE_LIGHT].routines = MAG_POINTS;
  spell_info[SPELL_CURE_LIGHT].time = 1;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  config.type = DOMAIN_EVENT_CHARACTER_DAMAGED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = domain_event_character_handle(&f.victim);
  config.owner = domain_event_character_handle(&f.actor);
  config.identity = "test.ready.damage";
  config.handler = ready_combat_damage;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  if (outcome == 6)
  {
    f.exits[0].exit_info = EX_ISDOOR | EX_CLOSED;
    do_ready(&f.actor, "attack caster on door open north", 0, 0);
  }
  else
    do_ready(&f.actor, outcome == 5 ? "attack caster on entry" : "attack caster on casting", 0, 0);
  CuAssertPtrNotNull(tc, f.actor.ready_action);
  if (outcome == 5)
    domain_event_runtime_character_moved(&f.victim, 1, 0, SOUTH);
  CuAssertIntEquals(tc, 1, cast_spell(&f.victim, &f.victim, NULL, SPELL_CURE_LIGHT, 0));
  CuAssertTrue(tc, IS_CASTING(&f.victim));
  if (outcome == 6)
    door_state_update(0, NORTH, EX_CLOSED, 0, false, DOMAIN_DOOR_GAMEPLAY);
  if (outcome == 4)
    SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_BLIND);
  queued = calloc(1U, sizeof(*queued));
  queued->attack_type = AA_KICK;
  queued->argument = strdup("caster");
  enqueue_attack(GET_ATTACK_QUEUE(&f.actor), queued);
  circle_srandom(1234);
  pulse += outcome == 3 ? 2 * PASSES_PER_SEC : 1;
  event_test_advance();
  CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
  CuAssertIntEquals(tc, outcome == 2 || outcome == 4 ? 0 : 1, trace.hits);
  CuAssertIntEquals(tc, 1, pending_attacks(&f.actor));
  if (outcome == 0 || outcome == 3 || outcome == 5 || outcome == 6)
  {
    CuAssertTrue(tc, trace.lost >= 80);
    CuAssertTrue(tc, !IS_CASTING(&f.victim));
  }
  else
    CuAssertTrue(tc, IS_CASTING(&f.victim));
  CuAssertTrue(tc, !is_action_available(&f.actor, atSTANDARD, false));
  ready_action_latency_read(&latency);
  CuAssertTrue(tc, latency.callbacks == 1U);
  stop_fighting(&f.actor);
  stop_fighting(&f.victim);
  for (tick = 0; tick < 3 * PASSES_PER_SEC; tick++)
  {
    pulse++;
    event_test_advance();
  }
  CuAssertTrue(tc, !primary_activity_snapshot(&f.victim, &cast));
  CuAssertTrue(tc, !IS_CASTING(&f.victim));
  domain_event_runtime_shutdown();
  event_free_all();
  free_attack_queue(GET_ATTACK_QUEUE(&f.actor));
  GET_ATTACK_QUEUE(&f.actor) = NULL;
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  CONFIG_SPELLCASTING_TIME_MODE = saved_mode;
  spell_info[SPELL_CURE_LIGHT] = saved_spell;
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
}

void Test_gameplay_readied_strike_damage_interrupts_real_timed_cast(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 0);
}

void Test_gameplay_readied_strike_successful_concentration_preserves_cast(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 1);
}

void Test_gameplay_readied_strike_miss_preserves_cast(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 2);
}

void Test_gameplay_readied_strike_precedes_cast_when_both_deadlines_are_overdue(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 3);
}

void Test_gameplay_readied_strike_rechecks_visibility_at_execution(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 4);
}

void Test_gameplay_readied_entry_strike_uses_single_reserved_attack(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 5);
}

void Test_gameplay_readied_door_strike_uses_single_reserved_attack(CuTest *tc)
{
  verify_readied_cast_outcome(tc, 6);
}

struct gameplay_move_trace
{
  int count;
  struct domain_character_moved event;
};

static void gameplay_capture_move(const struct domain_event_context *context, void *data)
{
  struct gameplay_move_trace *trace = data;

  trace->count++;
  trace->event = *(const struct domain_character_moved *)context->payload;
}

void Test_gameplay_movement_fact_waits_for_entry_script_acceptance(CuTest *tc)
{
  struct gameplay_fixture f;
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct cmdlist_element command = {0};
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct gameplay_move_trace trace = {0};
  int rejected, accepted, rejected_count, accepted_count, direction, cause;
  room_rnum rejected_room, accepted_room;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER);
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  config.type = DOMAIN_EVENT_CHARACTER_MOVED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = domain_event_character_handle(&f.actor);
  config.owner = domain_event_character_handle(&f.victim);
  config.identity = "test.actual-move";
  config.handler = gameplay_capture_move;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  script.types = WTRIG_ENTER;
  script.trig_list = &trigger;
  trigger.trigger_type = WTRIG_ENTER;
  trigger.narg = 100;
  trigger.name = (char *)"entry veto";
  trigger.nr = NOTHING;
  trigger.cmdlist = &command;
  command.cmd = (char *)"return 0";
  SCRIPT(&f.rooms[1]) = &script;

  rejected = perform_move(&f.actor, NORTH, FALSE);
  rejected_room = IN_ROOM(&f.actor);
  rejected_count = trace.count;
  SCRIPT(&f.rooms[1]) = NULL;
  free_varlist(trigger.var_list);
  accepted = perform_move(&f.actor, NORTH, FALSE);
  accepted_room = IN_ROOM(&f.actor);
  accepted_count = trace.count;
  direction = trace.event.direction;
  cause = trace.event.cause;

  domain_event_runtime_shutdown();
  event_free_all();
  domain_event_world_shutdown();
  end_gameplay_fixture(&f);
  CuAssertIntEquals(tc, 0, rejected);
  CuAssertIntEquals(tc, 0, rejected_room);
  CuAssertIntEquals(tc, 0, rejected_count);
  CuAssertIntEquals(tc, 1, accepted);
  CuAssertIntEquals(tc, 1, accepted_room);
  CuAssertIntEquals(tc, 1, accepted_count);
  CuAssertIntEquals(tc, NORTH, direction);
  CuAssertIntEquals(tc, DOMAIN_RELOCATION_WALK, cause);
}

struct gameplay_transfer_trace
{
  int count;
  struct domain_object_moved events[16];
};

static void gameplay_capture_transfer(const struct domain_event_context *context, void *data)
{
  struct gameplay_transfer_trace *trace = data;

  if (trace->count < 16)
    trace->events[trace->count] = *(const struct domain_object_moved *)context->payload;
  trace->count++;
}

void Test_gameplay_object_transfer_has_one_complete_holder_fact(CuTest *tc)
{
  struct gameplay_fixture f;
  struct obj_data *item, *container;
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct gameplay_transfer_trace trace = {0};
  struct domain_object_transfer_operation operation;
  bool gave, repeated;
  int count;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER);
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  item = create_obj();
  container = create_obj();
  item->name = strdup("parcel");
  item->short_description = strdup("a parcel");
  obj_to_room(item, 0);
  config.type = DOMAIN_EVENT_OBJECT_MOVED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = domain_event_object_handle(item);
  config.owner = domain_event_character_handle(&f.actor);
  config.identity = "test.transfer";
  config.handler = gameplay_capture_transfer;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  obj_from_room(item);
  CuAssertIntEquals(tc, 0, trace.count);
  obj_to_char(item, &f.actor);
  gave = perform_give(&f.actor, &f.victim, item);
  repeated = perform_give(&f.actor, &f.victim, item);
  obj_from_char(item);
  obj_to_obj(item, container);
  obj_from_obj(item);
  obj_to_room(item, 1);
  /* A rollback of provisional mutations has no committed transfer. */
  domain_object_transfer_begin(&operation, item, &f.actor, DOMAIN_TRANSFER_COMMAND);
  obj_from_room(item);
  obj_to_char(item, &f.actor);
  obj_from_char(item);
  obj_to_room(item, 1);
  domain_object_transfer_finish(&operation);
  domain_object_transfer_finish(&operation);
  count = trace.count;
  extract_obj(item);
  extract_obj(container);
  domain_event_runtime_shutdown();
  event_free_all();
  domain_event_world_shutdown();
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, gave);
  CuAssertTrue(tc, !repeated);
  CuAssertIntEquals(tc, 4, count);
  CuAssertIntEquals(tc, 5, trace.count);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_ROOM, trace.events[0].source.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_INVENTORY, trace.events[0].destination.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_INVENTORY, trace.events[1].source.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_INVENTORY, trace.events[1].destination.kind);
  CuAssertIntEquals(tc, DOMAIN_TRANSFER_COMMAND, trace.events[1].cause);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_CONTAINER, trace.events[2].destination.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_CONTAINER, trace.events[3].source.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_ROOM, trace.events[3].destination.kind);
  CuAssertIntEquals(tc, DOMAIN_TRANSFER_EXTRACT, trace.events[4].cause);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_NONE, trace.events[4].destination.kind);
  CuAssertTrue(tc, trace.events[0].transfer_id < trace.events[1].transfer_id);
}

void Test_gameplay_nested_transfer_and_scoped_extraction(CuTest *tc)
{
  struct gameplay_fixture f;
  struct obj_data *item;
  struct bag_data bags = {0};
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct gameplay_transfer_trace trace = {0};
  struct domain_object_transfer_operation outer, inner;
  struct domain_entity_handle item_handle;
  int before_disposal;

  begin_gameplay_fixture(&f);
  f.actor.bags = &bags;
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  item = create_obj();
  obj_to_room(item, 0);
  item_handle = domain_event_object_handle(item);
  config.type = DOMAIN_EVENT_OBJECT_MOVED;
  config.topic.role = DOMAIN_EVENT_TOPIC_SUBJECT;
  config.topic.entity = item_handle;
  config.owner = domain_event_character_handle(&f.victim);
  config.identity = "test.nested-transfer";
  config.handler = gameplay_capture_transfer;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  domain_object_transfer_begin(&outer, item, &f.actor, DOMAIN_TRANSFER_COMMAND);
  obj_from_room(item);
  obj_to_char(item, &f.actor);
  domain_object_transfer_begin(&inner, item, &f.victim, DOMAIN_TRANSFER_SCRIPT);
  obj_from_char(item);
  obj_to_bag(&f.actor, item, 2);
  obj_to_bag(&f.actor, item, 2);
  domain_object_transfer_finish(&inner);
  CuAssertIntEquals(tc, 0, trace.count);
  domain_object_transfer_finish(&outer);
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertIntEquals(tc, 1, IS_CARRYING_N(&f.actor));
  CuAssertTrue(tc, item->next_content == NULL);
  obj_from_bag(&f.actor, item, 2);
  obj_to_room(item, 1);
  domain_object_transfer_begin(&outer, item, &f.actor, DOMAIN_TRANSFER_COMMAND);
  extract_obj(item);
  before_disposal = trace.count;
  domain_object_transfer_finish(&outer);
  domain_object_transfer_finish(&outer);
  CuAssertPtrEquals(tc, NULL, domain_event_world_resolve_object(item_handle));
  domain_event_runtime_shutdown();
  event_free_all();
  end_gameplay_fixture(&f);

  CuAssertIntEquals(tc, DOMAIN_HOLDER_ROOM, trace.events[0].source.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_BAG, trace.events[0].destination.kind);
  CuAssertIntEquals(tc, 2, trace.events[0].destination.slot);
  CuAssertIntEquals(tc, DOMAIN_TRANSFER_SCRIPT, trace.events[0].cause);
  CuAssertIntEquals(tc, 2, before_disposal);
  CuAssertIntEquals(tc, 3, trace.count);
  CuAssertIntEquals(tc, DOMAIN_TRANSFER_EXTRACT, trace.events[2].cause);
}

void Test_gameplay_quest_delivery_consumes_one_committed_item_once(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct aq_data quest = {0}, *saved_quests = aquest_table;
  qst_rnum saved_count = total_quests;
  struct index_data prototype = {0}, *saved_index = obj_index;
  obj_rnum saved_top = top_of_objt;
  struct obj_data object_prototype = {0}, *saved_proto = obj_proto;
  struct obj_data *item;
  struct domain_entity_handle item_handle;
  int i, remaining;
  bool gave, pending;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "delivery fixture";
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST(&f.actor, i) = NOTHING;
  aquest_table = &quest;
  total_quests = 1;
  quest.vnum = 700;
  quest.type = AQ_OBJ_RETURN;
  quest.target = 900;
  quest.value[5] = 1;
  quest.value[6] = 1;
  prototype.vnum = 900;
  prototype.number = 1;
  obj_proto = &object_prototype;
  obj_index = &prototype;
  top_of_objt = 0;
  f.victim.nr = 0;
  GET_QUEST(&f.actor, 0) = 700;
  GET_QUEST_COUNTER(&f.actor, 0) = 1;
  item = create_obj();
  GET_OBJ_RNUM(item) = 0;
  item->name = strdup("parcel");
  item->short_description = strdup("a quest parcel");
  obj_to_char(item, &f.actor);
  item_handle = domain_event_object_handle(item);
  gave = perform_give(&f.actor, &f.victim, item);
  remaining = GET_QUEST_COUNTER(&f.actor, 0);
  pending = char_has_mud_event(&f.actor, eQUEST_COMPLETE) != NULL;
  CuAssertPtrEquals(tc, NULL, domain_event_world_resolve_object(item_handle));
  domain_event_runtime_shutdown();
  event_free_all();
  aquest_table = saved_quests;
  total_quests = saved_count;
  obj_index = saved_index;
  obj_proto = saved_proto;
  top_of_objt = saved_top;
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, gave);
  CuAssertIntEquals(tc, 0, remaining);
  CuAssertTrue(tc, pending);
}

void Test_gameplay_quest_resolution_skill_and_witness_use_committed_facts(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct aq_data quests[4] = {0}, *saved_quests = aquest_table;
  qst_rnum saved_count = total_quests;
  struct domain_world_phenomenon phenomenon = {0};
  struct char_data *saved_character_list = character_list;
  struct kdtree *saved_wilderness_rooms = kd_wilderness_rooms;
  bool saved_spatial_system_enabled = spatial_system_enabled;
  room_rnum indexed_room = 0;
  double indexed_location[2] = {0.0, 0.0};
  int i;

  begin_gameplay_fixture(&f);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "objective fixture";
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &f.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  f.actor.desc = &descriptor;
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST(&f.actor, i) = NOTHING;
  aquest_table = quests;
  total_quests = 4;
  quests[0].vnum = 710;
  quests[0].type = AQ_MOB_RESOLVE;
  quests[0].target = 1;
  quests[0].value[6] = 1;
  quests[1].vnum = 711;
  quests[1].type = AQ_SKILL_SUCCESS;
  quests[1].target = ABILITY_PERCEPTION;
  quests[1].value[6] = 1;
  quests[2].vnum = 712;
  quests[2].type = AQ_WITNESS_PHENOMENON;
  quests[2].target = DOMAIN_PHENOMENON_FIRE;
  quests[2].value[6] = 1;
  quests[3].vnum = 713;
  quests[3].type = AQ_DIALOGUE;
  quests[3].target = 1;
  quests[3].value[6] = 1;
  f.victim.nr = 0;
  for (i = 0; i < 3; i++)
  {
    GET_QUEST(&f.actor, i) = quests[i].vnum;
    GET_QUEST_COUNTER(&f.actor, i) = 1;
  }
  f.actor.next = &f.victim;
  f.victim.next = NULL;
  character_list = &f.actor;

  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_resolved(
                        &f.actor, &f.victim, DOMAIN_CHARACTER_RESOLUTION_RESCUED, SKILL_RESCUE));
  CuAssertIntEquals(tc, 0, GET_QUEST_COUNTER(&f.actor, 0));
  GET_QUEST(&f.actor, 0) = quests[3].vnum;
  GET_QUEST_COUNTER(&f.actor, 0) = 1;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_character_resolved(&f.actor, &f.victim,
                                                            DOMAIN_CHARACTER_RESOLUTION_NEGOTIATED,
                                                            ABILITY_DIPLOMACY));
  CuAssertIntEquals(tc, 0, GET_QUEST_COUNTER(&f.actor, 0));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_skill_resolved(&f.actor, domain_event_room_handle(0),
                                                        ABILITY_PERCEPTION, 1, 0, 20, false));
  CuAssertIntEquals(tc, 1, GET_QUEST_COUNTER(&f.actor, 1));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_skill_resolved(&f.actor, domain_event_room_handle(0),
                                                        ABILITY_PERCEPTION, 20, 5, 20, true));
  CuAssertIntEquals(tc, 0, GET_QUEST_COUNTER(&f.actor, 1));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_runtime_skill_resolved(&f.actor, domain_event_room_handle(0),
                                                        ABILITY_PERCEPTION, 20, 5, 20, true));
  CuAssertIntEquals(tc, 0, GET_QUEST_COUNTER(&f.actor, 1));

  phenomenon.phenomenon_id = 714U;
  phenomenon.source = domain_event_character_handle(&f.victim);
  phenomenon.source_room = domain_event_room_handle(0);
  phenomenon.kind = DOMAIN_PHENOMENON_FIRE;
  phenomenon.intensity = 1.0f;
  phenomenon.channels = DOMAIN_WORLD_PHENOMENON_VISUAL;
  phenomenon.propagation = DOMAIN_WORLD_PROPAGATE_ROOMS;
  phenomenon.visual_range = 0;
  phenomenon.minimum_range = 0;
  phenomenon.visual_description = "A controlled flame flares nearby.";
  SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_BLIND);
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(), DOMAIN_EVENT_WORLD_PHENOMENON, &phenomenon));
  CuAssertIntEquals(tc, 1, GET_QUEST_COUNTER(&f.actor, 2));
  REMOVE_BIT_AR(AFF_FLAGS(&f.actor), AFF_BLIND);
  f.rooms[0].wilderness_coordinates_set = true;
  f.rooms[0].coords[X_COORD] = 0;
  f.rooms[0].coords[Y_COORD] = 0;
  kd_wilderness_rooms = kd_create(2);
  CuAssertPtrNotNull(tc, kd_wilderness_rooms);
  CuAssertIntEquals(tc, 0, kd_insert(kd_wilderness_rooms, indexed_location, &indexed_room));
  spatial_system_enabled = true;
  phenomenon.propagation = DOMAIN_WORLD_PROPAGATE_COORDINATES;
  phenomenon.source_x = 0;
  phenomenon.source_y = 0;
  phenomenon.source_z = 0;
  phenomenon.phenomenon_id++;
  CuAssertIntEquals(
      tc, DOMAIN_EVENT_OK,
      DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(), DOMAIN_EVENT_WORLD_PHENOMENON, &phenomenon));
  CuAssertIntEquals(tc, 0, GET_QUEST_COUNTER(&f.actor, 2));

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  kd_free(kd_wilderness_rooms);
  kd_wilderness_rooms = saved_wilderness_rooms;
  spatial_system_enabled = saved_spatial_system_enabled;
  aquest_table = saved_quests;
  total_quests = saved_count;
  character_list = saved_character_list;
  f.actor.next = NULL;
  f.actor.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  end_gameplay_fixture(&f);
}

void Test_gameplay_supply_refresh_is_lazy_and_preserves_existing_offers(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials = {0};
  struct supply_contract *offers;
  time_t now = time(NULL), refreshed;
  int i, count;
  bool offline_due, fresh_due, unchanged = true;

  clear_char(&ch);
  ch.player_specials = &specials;
  GET_CRAFT((&ch)).supply_slots_last_refresh = now - 3601;
  for (i = 0; i < 5; i++)
  {
    GET_CRAFT((&ch)).supply_slot_active[i] = true;
    GET_CRAFT((&ch)).supply_slots[i].quantity = 10 + i;
  }
  /* No descriptor and no global pulse are required to refresh aged offers. */
  offline_due = should_refresh_supply_slots(&ch);
  offers = generate_available_contracts(&ch, &count);
  refreshed = GET_CRAFT((&ch)).supply_slots_last_refresh;
  fresh_due = should_refresh_supply_slots(&ch);
  if (offers == NULL || count != 5)
    unchanged = false;
  else
    for (i = 0; i < count; i++)
      if (offers[i].quantity != 10 + i)
        unchanged = false;
  free_contract_list(offers, count);

  CuAssertTrue(tc, offline_due);
  CuAssertTrue(tc, !fresh_due);
  CuAssertTrue(tc, refreshed >= now);
  CuAssertTrue(tc, unchanged);
  CuAssertTrue(tc, GET_CRAFT((&ch)).supply_slots_next_refresh == refreshed + 3600);
}

static void verify_owned_craft_lifecycle(CuTest *tc, bool move_instead)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct primary_activity_snapshot snapshot;
  unsigned long saved_pulse = pulse;
  bool admitted, active_after, completed;
  int remaining, paused_remaining, i;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "craft fixture";
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST(&f.actor, i) = NOTHING;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &f.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  f.actor.desc = &descriptor;
  /* This descriptor is deliberately absent from descriptor_list. */
  GET_CRAFT((&f.actor)).crafting_method = SCMD_NEWCRAFT_SURVEY;
  GET_CRAFT((&f.actor)).craft_duration = 3;
  resume_craft_activity(&f.actor);
  admitted =
      primary_activity_snapshot(&f.actor, &snapshot) && snapshot.type == PRIMARY_ACTIVITY_CRAFT;
  pulse += PASSES_PER_SEC;
  event_test_advance();
  remaining = GET_CRAFT((&f.actor)).craft_duration;
  if (move_instead)
  {
    char_from_room(&f.actor);
    char_to_room_cause(&f.actor, 1, NULL, DOMAIN_RELOCATION_SCRIPT, -1);
  }
  else
    f.actor.desc = NULL;
  pulse += PASSES_PER_SEC;
  event_test_advance();
  active_after = primary_activity_snapshot(&f.actor, &snapshot);
  paused_remaining = GET_CRAFT((&f.actor)).craft_duration;
  f.actor.desc = &descriptor;
  if (!move_instead)
  {
    /* Offline time does not advance CrDu, and login reconstructs an owned timer. */
    pulse += 20 * PASSES_PER_SEC;
    event_test_advance();
    resume_craft_activity(&f.actor);
    pulse += PASSES_PER_SEC;
    event_test_advance();
    pulse += PASSES_PER_SEC;
    event_test_advance();
  }
  completed = specials.surveyed_room && GET_CRAFT((&f.actor)).craft_duration == 0 &&
              !primary_activity_snapshot(&f.actor, &snapshot);
  domain_event_runtime_shutdown();
  event_free_all();
  ProtocolDestroy(descriptor.pProtocol);
  f.actor.desc = NULL;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, admitted);
  CuAssertIntEquals(tc, 2, remaining);
  CuAssertTrue(tc, !active_after);
  CuAssertIntEquals(tc, move_instead ? 0 : 2, paused_remaining);
  CuAssertIntEquals(tc, !move_instead, completed);
}

void Test_gameplay_owned_craft_pauses_offline_and_resumes_without_descriptor_scan(CuTest *tc)
{
  verify_owned_craft_lifecycle(tc, false);
}

void Test_gameplay_owned_craft_cancels_on_committed_relocation(CuTest *tc)
{
  verify_owned_craft_lifecycle(tc, true);
}

void Test_gameplay_object_editor_copy_preserves_live_transfer_state(CuTest *tc)
{
  struct obj_data source, live;

  clear_object(&source);
  clear_object(&live);
  live.transfer_pending = true;
  live.transfer_source.kind = DOMAIN_HOLDER_ROOM;
  live.transfer_bag.kind = DOMAIN_HOLDER_BAG;
  live.transfer_bag.slot = 2;
  source.transfer_disposed = true;
  source.transfer_extracting = true;
  source.transfer_bag.kind = DOMAIN_HOLDER_BAG;
  source.transfer_bag.slot = 9;
  copy_object(&live, &source);

  CuAssertTrue(tc, live.transfer_pending);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_ROOM, live.transfer_source.kind);
  CuAssertIntEquals(tc, DOMAIN_HOLDER_BAG, live.transfer_bag.kind);
  CuAssertIntEquals(tc, 2, live.transfer_bag.slot);
  CuAssertTrue(tc, !live.transfer_disposed && !live.transfer_extracting);
}

static void verify_native_transport(CuTest *tc, int mode)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct primary_activity_definition activity = {0};
  unsigned long saved_pulse = pulse;
  game_event_type_id_t type;
  size_t live = 99;
  bool admitted, duplicate_rejected, primary_allowed, paused = true;
  int i, remaining, destination;

  begin_gameplay_fixture(&f);
  f.rooms[1].number = 66700;
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "transport fixture";
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST(&f.actor, i) = NOTHING;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &f.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  f.actor.desc = &descriptor;
  admitted = transport_job_start(&f.actor, 1, 0, 3, TRAVEL_CARRIAGE, 0);
  duplicate_rejected = !transport_job_start(&f.actor, 1, 0, 100, TRAVEL_CARRIAGE, 0);
  char_from_room(&f.actor);
  char_to_room_cause(&f.actor, 1, &f.actor, DOMAIN_RELOCATION_TRANSPORT, -1);
  activity.type = PRIMARY_ACTIVITY_TEST;
  activity.display_name = "passenger activity";
  activity.total_steps = 100;
  activity.step_interval = 100 * PASSES_PER_SEC;
  primary_allowed = primary_activity_start(&f.actor, domain_event_room_handle(1), &activity);
  pulse += PASSES_PER_SEC;
  event_test_advance();
  remaining = transport_remaining_seconds(&f.actor);
  if (mode == 1)
  {
    transport_job_cancel(&f.actor, true);
    f.actor.desc = NULL;
    pulse += 20 * PASSES_PER_SEC;
    event_test_advance();
    paused = transport_remaining_seconds(&f.actor) == 2 && IN_ROOM(&f.actor) == 1;
    f.actor.desc = &descriptor;
    transport_job_resume(&f.actor);
  }
  else if (mode == 2)
  {
    char_from_room(&f.actor);
    char_to_room_cause(&f.actor, 0, NULL, DOMAIN_RELOCATION_SCRIPT, -1);
  }
  else if (mode == 3)
    f.rooms[0].event_owner_generation++;
  pulse += 2 * PASSES_PER_SEC;
  event_test_advance();
  destination = IN_ROOM(&f.actor);
  event_runtime_find_type("transport.arrival", &type);
  event_runtime_type_live_count(type, &live);
  domain_event_runtime_shutdown();
  event_free_all();
  ProtocolDestroy(descriptor.pProtocol);
  f.actor.desc = NULL;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, admitted && duplicate_rejected);
  CuAssertTrue(tc, primary_allowed);
  CuAssertIntEquals(tc, 2, remaining);
  CuAssertTrue(tc, paused);
  CuAssertIntEquals(tc, mode == 3 ? 1 : 0, destination);
  CuAssertIntEquals(tc, 0, (int)live);
  CuAssertPtrEquals(tc, NULL, specials.transport_job);
  if (mode != 3)
    CuAssertIntEquals(tc, 0, specials.travel_type);
}

void Test_gameplay_transport_is_an_owned_deadline_not_a_primary_activity(CuTest *tc)
{
  verify_native_transport(tc, 0);
}

void Test_gameplay_transport_pauses_offline_and_reconstructs_its_deadline(CuTest *tc)
{
  verify_native_transport(tc, 1);
}

void Test_gameplay_transport_cancels_after_scripted_relocation(CuTest *tc)
{
  verify_native_transport(tc, 2);
}

void Test_gameplay_transport_rejects_a_recycled_destination(CuTest *tc)
{
  verify_native_transport(tc, 3);
}

/** Round-trip stable transport destinations using an isolated player and index fixture. */
void Test_gameplay_transport_loads_a_versioned_stable_destination(CuTest *tc)
{
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *loaded = new_char();
  char directory[PATH_MAX], filename[MAX_FILEPATH], name[32];
  FILE *file;
  int result, destination, remaining, type, locale, directory_restored;
  bool unscheduled, saved = false;
  char line[256];

  snprintf(name, sizeof(name), "Zztr%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4247;
  player_table = index;
  top_of_p_table = 0;
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, file);
  fprintf(file, "Name: %s\nId  : 4247\nLevl: 7\nTrv1: 103000 17 1 0\n", name);
  fclose(file);
  result = load_char(name, loaded);
  destination = loaded->player_specials->destination;
  remaining = transport_remaining_seconds(loaded);
  type = loaded->player_specials->travel_type;
  locale = loaded->player_specials->travel_locale;
  unscheduled = loaded->player_specials->transport_job == NULL;
  save_char(loaded, 0);
  file = fopen(filename, "r");
  if (file != NULL)
  {
    while (fgets(line, sizeof(line), file) != NULL)
      if (strcmp(line, "Trv1: 103000 17 1 0\n") == 0)
        saved = true;
    fclose(file);
  }
  unlink(filename);
  directory_restored = leave_player_fixture(directory, temporary_directory);
  free_char(loaded);
  player_table = saved_table;
  top_of_p_table = saved_top;

  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, 103000, destination);
  CuAssertIntEquals(tc, 17, remaining);
  CuAssertIntEquals(tc, TRAVEL_CARRIAGE, type);
  CuAssertIntEquals(tc, 0, locale);
  CuAssertTrue(tc, unscheduled && saved);
  CuAssertIntEquals(tc, 0, directory_restored);
}

void Test_gameplay_transport_group_admission_precedes_fare_and_departure(CuTest *tc)
{
  struct gameplay_fixture f;
  struct room_data rooms[3] = {0};
  struct player_special_data actor_specials = {0}, companion_specials = {0};
  struct descriptor_data descriptors[2] = {0};
  struct group_data group = {0};
  struct follow_type follower = {0};
  struct char_data *passenger;
  game_event_type_id_t type;
  size_t live = 0;
  bool rolled_back;
  int i, j, fare, actor_room, companion_room, remaining_gold;

  begin_gameplay_fixture(&f);
  rooms[0].number = 66700;
  rooms[0].zone = 0;
  rooms[0].sector_type = SECT_INSIDE;
  rooms[0].name = strdup("Private transit");
  rooms[0].description = strdup("A private transport fixture.\r\n");
  rooms[1] = f.rooms[0];
  rooms[1].number = 103000;
  rooms[2] = f.rooms[1];
  rooms[2].number = 145387;
  world = rooms;
  top_of_world = 2;
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  for (i = 0; i < 2; i++)
  {
    passenger = i == 0 ? &f.actor : &f.victim;
    REMOVE_BIT_AR(MOB_FLAGS(passenger), MOB_ISNPC);
    passenger->player_specials = i == 0 ? &actor_specials : &companion_specials;
    passenger->player.name = i == 0 ? "transport leader" : "transport companion";
    passenger->player.title = "";
    IN_ROOM(passenger) = 1;
    for (j = 0; j < MAX_CURRENT_QUESTS; j++)
      GET_QUEST(passenger, j) = NOTHING;
    descriptors[i].output = descriptors[i].small_outbuf;
    descriptors[i].bufspace = SMALL_BUFSIZE - 1;
    descriptors[i].character = passenger;
    descriptors[i].pProtocol = ProtocolCreate();
    descriptors[i].connected = CON_PLAYING;
    passenger->desc = &descriptors[i];
    passenger->group = &group;
  }
  follower.follower = &f.victim;
  f.actor.followers = &follower;
  f.victim.master = &f.actor;
  GET_GOLD(&f.actor) = 100;
  fare = get_carriage_locale_cost(1);
  /* A busy leader makes the second admission fail after the follower was admitted. */
  transport_job_start(&f.actor, 0, 2, 50, TRAVEL_CARRIAGE, 1);
  do_carriage(&f.actor, "mosswood village", 0, 0);
  rolled_back = companion_specials.transport_job == NULL && actor_specials.transport_job != NULL &&
                GET_GOLD(&f.actor) == 100 && IN_ROOM(&f.actor) == 1 && IN_ROOM(&f.victim) == 1;
  transport_job_cancel(&f.actor, false);
  do_carriage(&f.actor, "mosswood village", 0, 0);
  remaining_gold = GET_GOLD(&f.actor);
  actor_room = IN_ROOM(&f.actor);
  companion_room = IN_ROOM(&f.victim);
  event_runtime_find_type("transport.arrival", &type);
  event_runtime_type_live_count(type, &live);
  domain_event_runtime_shutdown();
  event_free_all();
  for (i = 0; i < 2; i++)
    ProtocolDestroy(descriptors[i].pProtocol);
  f.actor.desc = f.victim.desc = NULL;
  f.actor.group = f.victim.group = NULL;
  f.actor.followers = NULL;
  f.victim.master = NULL;
  free(rooms[0].name);
  free(rooms[0].description);
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, rolled_back);
  CuAssertIntEquals(tc, 100 - fare, remaining_gold);
  CuAssertIntEquals(tc, 0, actor_room);
  CuAssertIntEquals(tc, 0, companion_room);
  CuAssertIntEquals(tc, 2, (int)live);
}

static void verify_buff_sequence_lifecycle(CuTest *tc, int mode)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct primary_activity_definition activity = {0};
  game_event_type_id_t type;
  unsigned long saved_pulse = pulse;
  size_t pending = 0;
  bool admitted, stopped, retained;
  int i;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "buff fixture";
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST((&f.actor), i) = NOTHING;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &f.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  f.actor.desc = &descriptor;
  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  GET_BUFF((&f.actor), MAX_BUFFS - 1, 0) = SPELL_ARMOR;
  if (mode == 1)
    GET_BUFF_TARGET((&f.actor)) = &f.victim;
  if (mode == 2)
  {
    activity.type = PRIMARY_ACTIVITY_TEST;
    activity.display_name = "existing work";
    activity.total_steps = 100;
    activity.step_interval = PASSES_PER_SEC;
    primary_activity_start(&f.actor, domain_event_character_handle(&f.actor), &activity);
  }
  admitted = buff_sequence_start(&f.actor);
  if (mode == 1)
  {
    char_from_room(&f.victim);
    char_to_room_cause(&f.victim, 1, &f.actor, DOMAIN_RELOCATION_SCRIPT, -1);
  }
  if (mode == 3)
    f.actor.desc = NULL;
  /* No descriptor_list membership: the native deadline alone drives continuation. */
  for (i = 0; i < 10 * PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  stopped = !IS_BUFFING((&f.actor)) && specials.buff_sequence == NULL;
  retained = GET_BUFF((&f.actor), MAX_BUFFS - 1, 0) == SPELL_ARMOR;
  event_runtime_find_type("buff.sequence.next-cast", &type);
  event_runtime_type_live_count(type, &pending);
  domain_event_runtime_shutdown();
  event_free_all();
  ProtocolDestroy(descriptor.pProtocol);
  f.actor.desc = NULL;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);

  CuAssertIntEquals(tc, mode != 2, admitted);
  CuAssertTrue(tc, stopped && retained);
  CuAssertIntEquals(tc, 0, (int)pending);
}

void Test_gameplay_buff_sequence_handles_sparse_final_slot_without_polling(CuTest *tc)
{
  verify_buff_sequence_lifecycle(tc, 0);
}

void Test_gameplay_buff_sequence_stops_on_selected_target_relocation(CuTest *tc)
{
  verify_buff_sequence_lifecycle(tc, 1);
}

void Test_gameplay_buff_sequence_rejects_admission_during_primary_work(CuTest *tc)
{
  verify_buff_sequence_lifecycle(tc, 2);
}

void Test_gameplay_buff_sequence_stops_offline_but_keeps_saved_list(CuTest *tc)
{
  verify_buff_sequence_lifecycle(tc, 3);
}

static void verify_buff_sequence_casting(CuTest *tc, int mode)
{
  struct gameplay_fixture f;
  struct char_data *actor;
  struct char_data decoy;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct primary_activity_snapshot snapshot;
  unsigned long saved_pulse = pulse;
  int saved_mode = CONFIG_SPELLCASTING_TIME_MODE;
  int saved_divine_prep = CONFIG_DIVINE_PREP_TIME;
  int saved_min_level = spell_info[SPELL_CURE_LIGHT].min_level[CLASS_CLERIC];
  int i, hit_points;
  bool admitted, casting, pending_spell, stopped;
  bool interrupt = mode == 1;
  size_t waiting_events = 99;
  game_event_type_id_t type;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  actor = &f.actor;
  REMOVE_BIT_AR(MOB_FLAGS(actor), MOB_ISNPC);
  actor->player_specials = &specials;
  actor->player.name = "buffcaster";
  actor->player.title = "";
  CLASS_LEVEL(actor, CLASS_CLERIC) = 10;
  actor->real_abils.wis = actor->aff_abils.wis = 18;
  GET_SKILL(actor, SPELL_CURE_LIGHT) = 99;
  GET_HIT(actor) = 10;
  GET_MAX_HIT(actor) = 100;
  if (mode == 2)
  {
    initialize_test_npc(&decoy, "a guard", 0);
    decoy.player.name = "guard";
    decoy.next_in_room = &f.victim;
    actor->next_in_room = &decoy;
    f.victim.player.name = "guard";
    f.victim.player.short_descr = "a guard";
    GET_BUFF_TARGET(actor) = &f.victim;
    GET_HIT(&f.victim) = 10;
    GET_MAX_HIT(&f.victim) = 100;
  }
  CONFIG_SPELLCASTING_TIME_MODE = 1;
  CONFIG_DIVINE_PREP_TIME = 1;
  for (i = 0; i < MAX_CURRENT_QUESTS; i++)
    GET_QUEST(actor, i) = NOTHING;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  actor->desc = &descriptor;
  if (spell_info[SPELL_ARMOR].name == NULL || spell_info[SPELL_ARMOR].name == unused_spellname)
    mag_assign_spells();
  saved_min_level = spell_info[SPELL_CURE_LIGHT].min_level[CLASS_CLERIC];
  spell_info[SPELL_CURE_LIGHT].min_level[CLASS_CLERIC] = 1;
  collection_add(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  collection_add(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  GET_BUFF(actor, 0, 0) = SPELL_CURE_LIGHT;
  GET_BUFF(actor, 1, 0) = SPELL_CURE_LIGHT;
  admitted = buff_sequence_start(actor);
  for (i = 0; i < PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  casting =
      primary_activity_snapshot(actor, &snapshot) && snapshot.type == PRIMARY_ACTIVITY_CASTING;
  event_runtime_find_type("buff.sequence.next-cast", &type);
  event_runtime_type_live_count(type, &waiting_events);
  if (interrupt)
    primary_activity_cancel(actor, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
  for (i = 0; i < 30 * PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  hit_points = mode == 2 ? GET_HIT(&f.victim) : GET_HIT(actor);
  pending_spell = is_spell_in_collection(actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0);
  stopped = !IS_BUFFING(actor) && actor->player_specials->buff_sequence == NULL;
  domain_event_runtime_shutdown();
  event_free_all();
  clear_collection_by_class(actor, CLASS_CLERIC);
  clear_prep_queue_by_class(actor, CLASS_CLERIC);
  ProtocolDestroy(descriptor.pProtocol);
  actor->desc = NULL;
  actor->next_in_room = &f.victim;
  CONFIG_SPELLCASTING_TIME_MODE = saved_mode;
  CONFIG_DIVINE_PREP_TIME = saved_divine_prep;
  spell_info[SPELL_CURE_LIGHT].min_level[CLASS_CLERIC] = saved_min_level;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, admitted && casting);
  CuAssertIntEquals(tc, 0, (int)waiting_events);
  CuAssertTrue(tc, stopped);
  CuAssertIntEquals(tc, interrupt, pending_spell);
  if (interrupt)
    CuAssertIntEquals(tc, 10, hit_points);
  else
    CuAssertTrue(tc, hit_points > 10);
}

void Test_gameplay_buff_sequence_waits_for_casting_and_spends_each_preparation_once(CuTest *tc)
{
  verify_buff_sequence_casting(tc, 0);
}

void Test_gameplay_buff_sequence_interrupted_cast_preserves_later_preparation(CuTest *tc)
{
  verify_buff_sequence_casting(tc, 1);
}

void Test_gameplay_buff_sequence_resolves_selected_target_among_identical_names(CuTest *tc)
{
  verify_buff_sequence_casting(tc, 2);
}

static void verify_staff_agenda_lifecycle(CuTest *tc, int mode)
{
  struct gameplay_fixture f;
  struct staffevent_struct saved_staff = staffevent_data;
  struct char_data *saved_characters = character_list;
  unsigned long saved_pulse = pulse;
  game_event_type_id_t type;
  size_t live = 99;
  bool active, expired, delayed, cleared;
  int duration_ticks;
  int i;

  begin_gameplay_fixture(&f);
  character_list = NULL;
  staffevent_data.event_num = UNDEFINED_EVENT;
  staffevent_data.ticks_left = staffevent_data.delay = 0;
  event_free_all();
  pulse = 10 * PASSES_PER_SEC;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  duration_ticks = (2 * SECS_PER_MUD_HOUR - 10) * PASSES_PER_SEC;
  CuAssertTrue(tc, set_event_state(THE_PRISONER_EVENT, duration_ticks));
  active = is_event_active() && get_event_time_remaining() == duration_ticks &&
           staff_event_agenda_seconds() == 2 * SECS_PER_MUD_HOUR - 10;
  if (mode == 1)
  {
    CuAssertIntEquals(tc, EVENT_ERROR_NO_ACTIVE_EVENT, end_staff_event(JACKALOPE_HUNT));
    CuAssertTrue(tc, is_event_active());
    CuAssertIntEquals(tc, EVENT_SUCCESS, end_staff_event(THE_PRISONER_EVENT));
    CuAssertIntEquals(tc, EVENT_ERROR_DELAY_ACTIVE, start_staff_event(JACKALOPE_HUNT));
    CuAssertTrue(tc, set_event_delay(0));
    CuAssertTrue(tc,
                 set_event_state(THE_PRISONER_EVENT, duration_ticks + STAFF_EVENT_MUD_HOUR_TICKS));
  }
  for (i = 0; i < (2 * SECS_PER_MUD_HOUR - 10) * PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  expired = mode == 1
                ? is_event_active() && get_event_time_remaining() == STAFF_EVENT_MUD_HOUR_TICKS
                : !is_event_active();
  if (mode == 1)
    end_staff_event(THE_PRISONER_EVENT);
  delayed = get_event_delay() == STAFF_EVENT_DELAY_CNST;
  for (i = 0; i < STAFF_EVENT_DELAY_CNST; i++)
  {
    pulse++;
    event_test_advance();
  }
  cleared = get_event_delay() == 0;
  event_runtime_find_type("staff-event.prisoner-presence", &type);
  event_runtime_type_live_count(type, &live);
  domain_event_runtime_shutdown();
  event_free_all();
  staffevent_data = saved_staff;
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);

  CuAssertTrue(tc, active && expired && delayed && cleared);
  CuAssertIntEquals(tc, 0, (int)live);
}

void Test_gameplay_staff_agenda_preserves_hour_phase_and_expires_before_maintenance(CuTest *tc)
{
  verify_staff_agenda_lifecycle(tc, 0);
}

void Test_gameplay_staff_agenda_old_expiry_cannot_end_replacement(CuTest *tc)
{
  verify_staff_agenda_lifecycle(tc, 1);
}

void Test_gameplay_staff_agenda_rejects_start_before_announcement_when_unavailable(CuTest *tc)
{
  struct gameplay_fixture f;
  struct staffevent_struct saved_staff = staffevent_data;
  struct descriptor_data descriptor = {0};
  struct descriptor_data *saved_descriptors = descriptor_list;
  struct char_data *saved_characters = character_list;
  event_result_t result;
  bool silent, inactive;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  staffevent_data.event_num = UNDEFINED_EVENT;
  staffevent_data.ticks_left = staffevent_data.delay = 0;
  character_list = NULL;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &f.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  descriptor_list = &descriptor;
  f.actor.desc = &descriptor;
  staff_event_agenda_shutdown();
  result = start_staff_event(JACKALOPE_HUNT);
  silent = descriptor.small_outbuf[0] == '\0';
  inactive = !is_event_active() && character_list == NULL;
  domain_event_runtime_shutdown();
  event_free_all();
  f.actor.desc = NULL;
  descriptor_list = saved_descriptors;
  character_list = saved_characters;
  staffevent_data = saved_staff;
  ProtocolDestroy(descriptor.pProtocol);
  end_gameplay_fixture(&f);
  CuAssertIntEquals(tc, EVENT_ERROR_SCHEDULER, result);
  CuAssertTrue(tc, silent && inactive);
}

void Test_gameplay_staff_agenda_shutdown_discards_active_event_and_boot_delay_expires(CuTest *tc)
{
  struct gameplay_fixture f;
  struct staffevent_struct saved_staff = staffevent_data;
  unsigned long saved_pulse = pulse;
  bool admitted, forgotten, delayed, cleared;
  int i;

  begin_gameplay_fixture(&f);
  event_free_all();
  pulse = 0;
  event_init();
  staffevent_data.event_num = UNDEFINED_EVENT;
  staffevent_data.ticks_left = 0;
  staffevent_data.delay = 3;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  delayed = get_event_delay() == 3;
  for (i = 0; i < 3 * SECS_PER_MUD_HOUR * PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  cleared = get_event_delay() == 0;
  admitted = set_event_state(THE_PRISONER_EVENT, 10);
  domain_event_runtime_shutdown();
  forgotten = !is_event_active();
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  forgotten = forgotten && !is_event_active() && get_event_time_remaining() == 0;
  domain_event_runtime_shutdown();
  event_free_all();
  staffevent_data = saved_staff;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, delayed && cleared && admitted && forgotten);
}


static void capture_terminal_cast(const struct domain_event_context *context, void *data)
{
  const struct domain_activity_transitioned *event = context->payload;

  if (event->activity_type == PRIMARY_ACTIVITY_CASTING &&
      (event->current_state == PRIMARY_ACTIVITY_STATE_CANCELLED ||
       event->current_state == PRIMARY_ACTIVITY_STATE_COMPLETED))
    *(struct domain_activity_transitioned *)data = *event;
}

/* Production command, cast admission, resource debit and native reaction dispatch. */
static void verify_counterspell_reaction(CuTest *tc, int scenario)
{
  struct gameplay_fixture f;
  struct char_data competitor;
  struct player_special_data competitor_specials = {0};
  bool competing = scenario == 11 || scenario == 12;
  struct player_special_data specials = {0};
  struct player_special_data caster_specials = {0};
  struct char_data *saved_characters = character_list;
  struct spell_info_type saved_spell = spell_info[SPELL_CURE_LIGHT];
  int saved_mode = CONFIG_SPELLCASTING_TIME_MODE;
  int saved_prep = CONFIG_DIVINE_PREP_TIME;
  int saved_pk = CONFIG_PK_ALLOWED;
  unsigned long saved_pulse = pulse;
  struct primary_activity_snapshot original;
  struct domain_activity_transitioned terminal = {0};
  struct domain_event_subscription_config observer = {0};
  struct domain_event_subscription_handle subscription;
  struct primary_activity_snapshot replacement;
  struct domain_event_bus_stats before_admission;
  struct domain_event_bus_stats after_admission;
  bool retained;
  int metamagic = METAMAGIC_NONE;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "watcher";
  f.victim.player.name = "caster";
  f.actor.next = &f.victim;
  character_list = &f.actor;
  f.rooms[0].light = 1;
  GET_CLASS(&f.actor) = CLASS_CLERIC;
  CLASS_LEVEL((&f.actor), CLASS_CLERIC) = 10;
  GET_ABILITY(&f.actor, ABILITY_SPELLCRAFT) = 100;
  f.actor.real_abils.wis = f.actor.aff_abils.wis = 18;
  GET_CLASS(&f.victim) = CLASS_CLERIC;
  GET_HIT(&f.victim) = 10;
  GET_MAX_HIT(&f.victim) = 100;
  CONFIG_SPELLCASTING_TIME_MODE = 1;
  CONFIG_DIVINE_PREP_TIME = 1;
  memset(&spell_info[SPELL_CURE_LIGHT], 0, sizeof(spell_info[SPELL_CURE_LIGHT]));
  spell_info[SPELL_CURE_LIGHT].name = "cure light";
  spell_info[SPELL_CURE_LIGHT].min_position = POS_FIGHTING;
  spell_info[SPELL_CURE_LIGHT].min_level[CLASS_CLERIC] = 1;
  spell_info[SPELL_CURE_LIGHT].targets = TAR_CHAR_ROOM;
  spell_info[SPELL_CURE_LIGHT].routines = MAG_POINTS;
  spell_info[SPELL_CURE_LIGHT].time = scenario == 6 ? 0 : 1;
  if (scenario == 6)
  {
    REMOVE_BIT_AR(MOB_FLAGS(&f.victim), MOB_ISNPC);
    f.victim.player_specials = &caster_specials;
    CLASS_LEVEL((&f.victim), CLASS_CLERIC) = 10;
    f.victim.real_abils.wis = f.victim.aff_abils.wis = 18;
    GET_SKILL(&f.victim, SPELL_CURE_LIGHT) = 99;
    CONFIG_PK_ALLOWED = TRUE;
    SET_BIT_AR(PRF_FLAGS(&f.actor), PRF_PVP);
    SET_BIT_AR(PRF_FLAGS(&f.victim), PRF_PVP);
    collection_add(&f.victim, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  }
  if (scenario != 4)
    collection_add(&f.actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  if (competing)
  {
    initialize_test_npc(&competitor, "the other counterer", 0);
    REMOVE_BIT_AR(MOB_FLAGS(&competitor), MOB_ISNPC);
    competitor.player_specials = &competitor_specials;
    competitor.player.name = "counterer";
    GET_CLASS(&competitor) = CLASS_CLERIC;
    CLASS_LEVEL((&competitor), CLASS_CLERIC) = 10;
    GET_ABILITY(&competitor, ABILITY_SPELLCRAFT) = 100;
    competitor.real_abils.wis = competitor.aff_abils.wis = 18;
    f.victim.next = &competitor;
    f.victim.next_in_room = &competitor;
    collection_add(&competitor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0, 0, 0);
  }
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  observer.type = DOMAIN_EVENT_ACTIVITY_TRANSITIONED;
  observer.topic = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT,
                                               domain_event_character_handle(&f.victim)};
  observer.owner = domain_event_character_handle(&f.actor);
  observer.identity = "test.counterspell.terminal";
  observer.handler = capture_terminal_cast;
  observer.handler_context = &terminal;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &observer, &subscription));
  if (scenario == 7 || scenario == 8)
    SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_DEAF);
  if (scenario == 5)
    metamagic = METAMAGIC_SILENT | METAMAGIC_STILL;
  else if (scenario == 7 || scenario == 9 || scenario == 10)
    metamagic = METAMAGIC_STILL;
  else if (scenario == 8)
    metamagic = METAMAGIC_SILENT;
  if (scenario == 13)
  {
    domain_event_bus_get_stats(domain_event_runtime_bus(), &before_admission);
    ready_action_runtime_shutdown();
  }
  if (scenario == 12)
    do_ready(&competitor, "counterspell caster on casting", 0, 0);
  do_ready(&f.actor, "counterspell caster on casting", 0, 0);
  if (scenario == 11)
    do_ready(&competitor, "counterspell caster on casting", 0, 0);
  if (competing)
  {
    CuAssertPtrNotNull(tc, competitor.ready_action);
    CuAssertTrue(tc, !is_action_available(&competitor, atSTANDARD, false));
  }
  if (scenario == 13)
  {
    domain_event_bus_get_stats(domain_event_runtime_bus(), &after_admission);
    CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
    CuAssertTrue(tc, is_action_available(&f.actor, atSTANDARD, false));
    CuAssertTrue(tc, is_spell_in_collection(&f.actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0));
    CuAssertPtrEquals(tc, NULL, SPELL_PREP_QUEUE(&f.actor, CLASS_CLERIC));
    CuAssertTrue(tc, before_admission.live_subscription_count ==
                         after_admission.live_subscription_count);
    goto counterspell_cleanup;
  }
  CuAssertPtrNotNull(tc, f.actor.ready_action);
  CuAssertTrue(tc, !is_action_available(&f.actor, atSTANDARD, false));
  if (scenario == 14)
    ready_action_runtime_shutdown();
  CuAssertIntEquals(tc, 1, cast_spell(&f.victim, &f.victim, NULL, SPELL_CURE_LIGHT, metamagic));
  if (scenario != 6)
    CuAssertTrue(tc, primary_activity_snapshot(&f.victim, &original));
  else
    CuAssertTrue(tc, !primary_activity_snapshot(&f.victim, &original));
  if (scenario == 1)
    SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_BLIND);
  if (scenario == 9)
    SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_DEAF);
  if (scenario == 2)
  {
    CuAssertTrue(tc, primary_activity_cancel_id(&f.victim, original.id,
                                                PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false));
    CuAssertIntEquals(tc, 1, cast_spell(&f.victim, &f.victim, NULL, SPELL_CURE_LIGHT, 0));
    CuAssertTrue(tc, primary_activity_snapshot(&f.victim, &replacement));
    CuAssertTrue(tc, replacement.id != original.id);
    CuAssertTrue(tc, !primary_activity_cancel_id(&f.victim, original.id,
                                                 PRIMARY_ACTIVITY_END_COUNTERED, false));
  }
  if (scenario == 3 || scenario == 12)
    pulse += (CASTING_TIME(&f.victim) + 1U) * PASSES_PER_SEC;
  else
    pulse++;
  event_test_advance();
  retained = is_spell_in_collection(&f.actor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0);
  if (scenario == 0 || scenario == 3 || scenario == 8 || scenario == 10 || competing)
  {
    CuAssertTrue(tc, !IS_CASTING(&f.victim));
    CuAssertTrue(tc, terminal.activity_id == original.id);
    CuAssertIntEquals(tc, PRIMARY_ACTIVITY_STATE_CANCELLED, terminal.current_state);
    CuAssertIntEquals(tc, PRIMARY_ACTIVITY_END_COUNTERED, terminal.end_reason);
    if (competing)
    {
      CuAssertIntEquals(
          tc, 1, retained + is_spell_in_collection(&competitor, CLASS_CLERIC, SPELL_CURE_LIGHT, 0));
      CuAssertIntEquals(tc, 1,
                        (SPELL_PREP_QUEUE(&f.actor, CLASS_CLERIC) != NULL) +
                            (SPELL_PREP_QUEUE(&competitor, CLASS_CLERIC) != NULL));
      CuAssertPtrEquals(tc, NULL, competitor.ready_action);
      CuAssertTrue(tc, !is_action_available(&competitor, atSTANDARD, false));
    }
    else
    {
      CuAssertTrue(tc, !retained);
      CuAssertPtrNotNull(tc, SPELL_PREP_QUEUE(&f.actor, CLASS_CLERIC));
    }
  }
  else
  {
    CuAssertTrue(tc, IS_CASTING(&f.victim) == (scenario != 6));
    CuAssertTrue(tc, retained == (scenario != 4));
    CuAssertPtrEquals(tc, NULL, SPELL_PREP_QUEUE(&f.actor, CLASS_CLERIC));
  }
  if (scenario == 5 || scenario == 6 || scenario == 7)
  {
    CuAssertPtrNotNull(tc, f.actor.ready_action);
    ready_action_cancel(&f.actor, false);
  }
  else
    CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
  CuAssertTrue(tc, !is_action_available(&f.actor, atSTANDARD, false));
counterspell_cleanup:
  domain_event_runtime_shutdown();
  event_free_all();
  if (scenario == 6)
  {
    clear_collection_by_class(&f.victim, CLASS_CLERIC);
    clear_prep_queue_by_class(&f.victim, CLASS_CLERIC);
  }
  if (competing)
  {
    clear_collection_by_class(&competitor, CLASS_CLERIC);
    clear_prep_queue_by_class(&competitor, CLASS_CLERIC);
    if (competitor.events != NULL)
      free_list(competitor.events);
    f.victim.next = NULL;
  }
  clear_collection_by_class(&f.actor, CLASS_CLERIC);
  clear_prep_queue_by_class(&f.actor, CLASS_CLERIC);
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  CONFIG_SPELLCASTING_TIME_MODE = saved_mode;
  CONFIG_DIVINE_PREP_TIME = saved_prep;
  CONFIG_PK_ALLOWED = saved_pk;
  spell_info[SPELL_CURE_LIGHT] = saved_spell;
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
}

void Test_gameplay_counterspell_consumes_one_preparation_and_cancels_real_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 0);
}

void Test_gameplay_counterspell_rechecks_visibility_without_spending_spell(CuTest *tc)
{
  verify_counterspell_reaction(tc, 1);
}

void Test_gameplay_counterspell_never_cancels_a_replacement_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 2);
}

void Test_gameplay_counterspell_precedes_later_overdue_cast_completion(CuTest *tc)
{
  verify_counterspell_reaction(tc, 3);
}

void Test_gameplay_counterspell_requires_matching_resource(CuTest *tc)
{
  verify_counterspell_reaction(tc, 4);
}


void Test_gameplay_counterspell_cannot_observe_silent_still_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 5);
}

void Test_gameplay_counterspell_does_not_delay_or_react_to_instant_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 6);
}


struct committed_attack_trace
{
  unsigned int count;
  uint64_t first_attempt_id;
  struct domain_attack_committed last;
  struct char_data *forget_attacker;
};

static void capture_committed_attack(const struct domain_event_context *context, void *data)
{
  struct committed_attack_trace *trace = data;

  if (trace->count == 0U)
    trace->first_attempt_id =
        ((const struct domain_attack_committed *)context->payload)->attempt_id;
  trace->count++;
  trace->last = *(const struct domain_attack_committed *)context->payload;
  if (trace->forget_attacker != NULL)
    domain_event_world_forget_character(trace->forget_attacker);
}

static struct obj_data *attack_test_object(const char *name, int type, int subtype)
{
  struct obj_data *object = create_obj();

  object->name = strdup(name);
  object->short_description = strdup(name);
  object->description = strdup(name);
  GET_OBJ_TYPE(object) = type;
  GET_OBJ_VAL(object, 0) = subtype;
  GET_OBJ_BOUND_ID(object) = NOBODY;
  return object;
}

static void verify_committed_attack_boundary(CuTest *tc, int scenario)
{
  struct gameplay_fixture f;
  struct script_data script = {0};
  struct trig_data trigger = {0};
  struct cmdlist_element command = {0};
  struct player_special_data defender_specials = {0};
  struct obj_data *weapon = NULL;
  struct domain_entity_handle weapon_handle = {0};
  struct domain_entity_handle pouch_handle = {0};
  struct domain_entity_handle projectile_handle = {0};
  struct domain_event_subscription_handle nested_subscription;
  struct char_data *saved_characters = character_list;
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct committed_attack_trace trace = {0};
  struct domain_entity_handle attacker;
  struct domain_entity_handle defender;
  int result;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  GET_ATTACK_QUEUE(&f.actor) = create_attack_queue();
  GET_ATTACK_QUEUE(&f.victim) = create_attack_queue();
  f.actor.next = &f.victim;
  character_list = &f.actor;
  f.rooms[0].light = 1;
  f.actor.player.name = "attacker";
  f.victim.player.name = "target";
  GET_HIT(&f.victim) = GET_MAX_HIT(&f.victim) = 100000;
  if (scenario == 2)
    SET_BIT_AR(ROOM_FLAGS(0), ROOM_PEACEFUL);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  attacker = domain_event_character_handle(&f.actor);
  defender = domain_event_character_handle(&f.victim);
  config.type = DOMAIN_EVENT_ATTACK_COMMITTED;
  config.topic = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, defender};
  config.owner = defender;
  config.identity = "test.attack.committed";
  config.handler = capture_committed_attack;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  if (scenario == 4)
    trace.forget_attacker = &f.actor;
  if (scenario == 9 || scenario == 10)
  {
    struct obj_data *pouch;
    struct obj_data *projectile;

    load_weapons();
    weapon = attack_test_object("bow", ITEM_WEAPON, WEAPON_TYPE_LONG_BOW);
    pouch = attack_test_object("pouch", ITEM_AMMO_POUCH, 10);
    projectile = attack_test_object("arrow", ITEM_MISSILE, AMMO_TYPE_ARROW);
    equip_char(&f.actor, weapon, WEAR_WIELD_1);
    equip_char(&f.actor, pouch, WEAR_AMMO_POUCH);
    obj_to_obj(projectile, pouch);
    weapon_handle = domain_event_object_handle(weapon);
    pouch_handle = domain_event_object_handle(pouch);
    projectile_handle = domain_event_object_handle(projectile);
    if (scenario == 9)
    {
      IN_ROOM(&f.victim) = 1;
      f.actor.next_in_room = NULL;
      f.rooms[1].people = &f.victim;
    }
  }
  if ((scenario >= 5 && scenario <= 7) || scenario == 10)
  {
    script.types = MTRIG_FIGHT;
    script.trig_list = &trigger;
    trigger.trigger_type = MTRIG_FIGHT;
    trigger.narg = 100;
    trigger.name = (char *)"attack entry mutation";
    trigger.nr = NOTHING;
    trigger.cmdlist = &command;
    command.cmd = scenario == 5    ? (char *)"mteleport target 101"
                  : scenario == 6  ? (char *)"mgoto 101"
                  : scenario == 10 ? (char *)"mjunk all.pouch"
                                   : (char *)"mjunk all.sword";
    SCRIPT(&f.actor) = &script;
    FIGHTING(&f.actor) = &f.victim;
    if (scenario == 7)
    {
      load_weapons();
      weapon = attack_test_object("sword", ITEM_WEAPON, WEAPON_TYPE_LONG_SWORD);
      equip_char(&f.actor, weapon, WEAR_WIELD_1);
      weapon_handle = domain_event_object_handle(weapon);
    }
  }
  if (scenario == 8)
  {
    REMOVE_BIT_AR(MOB_FLAGS(&f.victim), MOB_ISNPC);
    f.victim.player_specials = &defender_specials;
    CLASS_LEVEL((&f.victim), CLASS_WARRIOR) = 10;
    GET_ABILITY(&f.victim, ABILITY_TOTAL_DEFENSE) = 100;
    GET_HITROLL(&f.victim) = 100;
    GET_HIT(&f.actor) = GET_MAX_HIT(&f.actor) = 100000;
    TOTAL_DEFENSE(&f.victim) = 1;
    SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_TOTAL_DEFENSE);
    config.topic = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, attacker};
    config.identity = "test.attack.riposte";
    CuAssertIntEquals(
        tc, DOMAIN_EVENT_OK,
        domain_event_subscribe(domain_event_runtime_bus(), &config, &nested_subscription));
  }
  /* Equipment setup recalculates hitroll; select the outcome afterward. */
  GET_HITROLL(&f.actor) = scenario == 0 || scenario == 8 || scenario == 9 ? -100 : 100;
  circle_srandom(1234);
  result = hit(&f.actor, &f.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0,
               scenario == 3 || scenario == 9 || scenario == 10 ? ATTACK_TYPE_RANGED
                                                                : ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(
      tc,
      scenario == 2 || scenario == 3 || (scenario >= 5 && scenario <= 7) || scenario == 10 ? 0
      : scenario == 8                                                                      ? 2
                                                                                           : 1,
      trace.count);
  if (trace.count != 0U)
  {
    CuAssertTrue(tc, trace.last.attempt_id != 0U);
    CuAssertTrue(
        tc, domain_entity_handle_equal(scenario == 8 ? defender : attacker, trace.last.attacker));
    CuAssertTrue(
        tc, domain_entity_handle_equal(scenario == 8 ? attacker : defender, trace.last.defender));
    if (scenario == 8)
      CuAssertTrue(tc, trace.last.attempt_id > trace.first_attempt_id);
    CuAssertTrue(tc,
                 domain_entity_handle_equal(domain_event_room_handle(0), trace.last.origin_room));
  }
  if (scenario != 1)
  {
    CuAssertIntEquals(tc, 0, result);
    CuAssertIntEquals(tc, 100000, GET_HIT(&f.victim));
  }
  else
    CuAssertTrue(tc, result > 0 && GET_HIT(&f.victim) < 100000);
  if (scenario == 5)
    CuAssertIntEquals(tc, 1, IN_ROOM(&f.victim));
  if (scenario == 6)
    CuAssertIntEquals(tc, 1, IN_ROOM(&f.actor));
  if (scenario == 7)
  {
    CuAssertPtrEquals(tc, NULL, GET_EQ(&f.actor, WEAR_WIELD_1));
    CuAssertPtrEquals(
        tc, NULL,
        domain_event_resolve(domain_event_runtime_bus(), weapon_handle, DOMAIN_ENTITY_OBJECT));
  }
  if (scenario == 9 || scenario == 10)
  {
    struct obj_data *live_projectile =
        domain_event_resolve(domain_event_runtime_bus(), projectile_handle, DOMAIN_ENTITY_OBJECT);
    struct obj_data *live_pouch =
        domain_event_resolve(domain_event_runtime_bus(), pouch_handle, DOMAIN_ENTITY_OBJECT);
    if (scenario == 9)
    {
      CuAssertTrue(tc, live_projectile == NULL || IN_ROOM(live_projectile) == 1);
      CuAssertPtrNotNull(tc, live_pouch);
      CuAssertPtrEquals(tc, NULL, live_pouch->contains);
    }
    else
    {
      CuAssertPtrEquals(tc, NULL, live_projectile);
      CuAssertPtrEquals(tc, NULL, live_pouch);
      CuAssertPtrEquals(tc, NULL, GET_EQ(&f.actor, WEAR_AMMO_POUCH));
    }
    if (live_projectile != NULL)
      extract_obj(live_projectile);
    if (live_pouch != NULL)
      extract_obj(live_pouch);
    extract_obj(weapon);
  }
  SCRIPT(&f.actor) = NULL;
  free_varlist(trigger.var_list);
  domain_event_runtime_shutdown();
  event_free_all();
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  free_attack_queue(GET_ATTACK_QUEUE(&f.actor));
  free_attack_queue(GET_ATTACK_QUEUE(&f.victim));
  GET_ATTACK_QUEUE(&f.actor) = GET_ATTACK_QUEUE(&f.victim) = NULL;
  character_list = saved_characters;
  end_gameplay_fixture(&f);
}

void Test_gameplay_committed_attack_includes_a_real_miss(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 0);
}

void Test_gameplay_committed_attack_precedes_a_real_hit(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 1);
}

void Test_gameplay_peaceful_rejection_does_not_commit_attack(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 2);
}

void Test_gameplay_missing_projectile_does_not_commit_attack(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 3);
}

void Test_gameplay_committed_attack_observer_retirement_aborts_borrowed_context(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 4);
}


static void verify_ally_readiness(CuTest *tc, int scenario)
{
  struct gameplay_fixture f;
  struct char_data foe;
  struct player_special_data specials = {0};
  struct char_data *saved_characters = character_list;
  unsigned long saved_pulse = pulse;
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct committed_attack_trace retaliation = {0};
  struct attack_action_data *queued;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  event_init();
  initialize_test_npc(&foe, "the attacker", 0);
  foe.player.name = "foe";
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.player.name = "protector";
  f.victim.player.name = "ally";
  f.victim.master = scenario == 6 ? NULL : &f.actor;
  f.actor.next = &f.victim;
  f.victim.next = &foe;
  character_list = &f.actor;
  f.victim.next_in_room = &foe;
  f.rooms[0].light = 1;
  GET_CLASS(&f.actor) = CLASS_WARRIOR;
  CLASS_LEVEL((&f.actor), CLASS_WARRIOR) = 10;
  GET_HITROLL(&f.actor) = 100;
  GET_DAMROLL(&f.actor) = 20;
  GET_HITROLL(&foe) = scenario == 1 ? 100 : -100;
  GET_HIT(&foe) = GET_MAX_HIT(&foe) = 100000;
  GET_HIT(&f.victim) = GET_MAX_HIT(&f.victim) = 100000;
  GET_ATTACK_QUEUE(&f.actor) = create_attack_queue();
  GET_ATTACK_QUEUE(&f.victim) = create_attack_queue();
  GET_ATTACK_QUEUE(&foe) = create_attack_queue();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  config.type = DOMAIN_EVENT_ATTACK_COMMITTED;
  config.topic =
      (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, domain_event_character_handle(&foe)};
  config.owner = domain_event_character_handle(&f.actor);
  config.identity = "test.ally.retaliation";
  config.handler = capture_committed_attack;
  config.handler_context = &retaliation;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  do_ready(&f.actor, "attack on ally ally attacked", 0, 0);
  if (scenario == 6)
  {
    CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
    CuAssertTrue(tc, is_action_available(&f.actor, atSTANDARD, false));
  }
  else
  {
    CuAssertPtrNotNull(tc, f.actor.ready_action);
    CuAssertTrue(tc, !is_action_available(&f.actor, atSTANDARD, false));
    if (scenario == 5)
      f.victim.master = NULL;
    if (scenario == 7)
      SET_BIT_AR(AFF_FLAGS(&f.actor), AFF_BLIND);
    if (scenario == 10)
    {
      domain_event_runtime_character_died(&f.victim, &foe);
      CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
    }
    circle_srandom(1234);
    (void)hit(&foe, &f.victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
    CuAssertIntEquals(tc, 100000, GET_HIT(&foe));
    if (scenario == 1)
      CuAssertTrue(tc, GET_HIT(&f.victim) < 100000);
    else
      CuAssertIntEquals(tc, 100000, GET_HIT(&f.victim));
    if (scenario == 2)
      domain_event_runtime_character_moved(&foe, 0, 1, NORTH);
    if (scenario == 8)
      domain_event_runtime_character_died(&foe, NULL);
    if (scenario == 9)
      domain_event_runtime_character_extracted(&foe, 0U);
    if (scenario == 3)
    {
      domain_event_runtime_character_died(&f.victim, &foe);
      domain_event_world_forget_character(&f.victim);
      CuAssertPtrNotNull(tc, f.actor.ready_action);
    }
    if (scenario == 4)
    {
      domain_event_runtime_attack_committed(&foe, &f.victim, ATTACK_TYPE_PRIMARY);
      domain_event_runtime_attack_committed(&f.actor, &f.victim, ATTACK_TYPE_PRIMARY);
    }
    queued = calloc(1U, sizeof(*queued));
    queued->attack_type = AA_KICK;
    queued->argument = strdup("foe");
    enqueue_attack(GET_ATTACK_QUEUE(&f.actor), queued);
    circle_srandom(1234);
    pulse++;
    event_test_advance();
    CuAssertIntEquals(tc, scenario == 2 || scenario == 5 || scenario == 7 || scenario >= 8 ? 0 : 1,
                      retaliation.count);
    CuAssertIntEquals(tc, 1, pending_attacks(&f.actor));
    if (retaliation.count != 0U)
    {
      CuAssertTrue(tc, GET_HIT(&foe) < 100000);
      CuAssertPtrEquals(tc, NULL, f.actor.ready_action);
    }
    else
      CuAssertIntEquals(tc, 100000, GET_HIT(&foe));
  }
  ready_action_cancel(&f.actor, false);
  domain_event_runtime_shutdown();
  event_free_all();
  free_attack_queue(GET_ATTACK_QUEUE(&f.actor));
  free_attack_queue(GET_ATTACK_QUEUE(&f.victim));
  free_attack_queue(GET_ATTACK_QUEUE(&foe));
  GET_ATTACK_QUEUE(&f.actor) = GET_ATTACK_QUEUE(&f.victim) = GET_ATTACK_QUEUE(&foe) = NULL;
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  if (foe.events != NULL)
    free_list(foe.events);
  f.victim.master = NULL;
  f.victim.next = NULL;
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
}

void Test_gameplay_ally_readiness_reacts_to_miss_with_one_reserved_strike(CuTest *tc)
{
  verify_ally_readiness(tc, 0);
}

void Test_gameplay_ally_readiness_reacts_after_the_triggering_damage(CuTest *tc)
{
  verify_ally_readiness(tc, 1);
}

void Test_gameplay_ally_readiness_cancels_when_bound_attacker_leaves(CuTest *tc)
{
  verify_ally_readiness(tc, 2);
}

void Test_gameplay_ally_readiness_survives_ally_death_after_claim(CuTest *tc)
{
  verify_ally_readiness(tc, 3);
}

void Test_gameplay_ally_readiness_claims_once_without_dispatching_queued_attack(CuTest *tc)
{
  verify_ally_readiness(tc, 4);
}

void Test_gameplay_ally_readiness_rechecks_relationship(CuTest *tc)
{
  verify_ally_readiness(tc, 5);
}

void Test_gameplay_ally_readiness_rejects_nonally_before_action_cost(CuTest *tc)
{
  verify_ally_readiness(tc, 6);
}

void Test_gameplay_ally_readiness_requires_visible_attempt(CuTest *tc)
{
  verify_ally_readiness(tc, 7);
}


void Test_gameplay_ally_readiness_cancels_on_bound_attacker_death(CuTest *tc)
{
  verify_ally_readiness(tc, 8);
}

void Test_gameplay_ally_readiness_cancels_on_bound_attacker_extraction(CuTest *tc)
{
  verify_ally_readiness(tc, 9);
}

void Test_gameplay_ally_readiness_cancels_on_ally_death_before_claim(CuTest *tc)
{
  verify_ally_readiness(tc, 10);
}


void Test_gameplay_counterspell_deaf_observer_cannot_identify_verbal_only_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 7);
}

void Test_gameplay_counterspell_deaf_observer_can_identify_visible_gestures(CuTest *tc)
{
  verify_counterspell_reaction(tc, 8);
}

void Test_gameplay_counterspell_rechecks_hearing_before_resource_debit(CuTest *tc)
{
  verify_counterspell_reaction(tc, 9);
}

void Test_gameplay_counterspell_hearing_observer_can_identify_verbal_only_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 10);
}


void Test_gameplay_competing_counterspells_share_deadline_and_spend_one_resource(CuTest *tc)
{
  verify_counterspell_reaction(tc, 11);
}

void Test_gameplay_competing_counterspells_reverse_admission_with_overdue_cast(CuTest *tc)
{
  verify_counterspell_reaction(tc, 12);
}


void Test_gameplay_counterspell_failed_native_admission_preserves_action_and_resource(CuTest *tc)
{
  verify_counterspell_reaction(tc, 13);
}

void Test_gameplay_counterspell_failed_trigger_preserves_resource_without_action_refund(CuTest *tc)
{
  verify_counterspell_reaction(tc, 14);
}


void Test_gameplay_dg_fight_target_teleport_aborts_before_attack_commitment(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 5);
}

void Test_gameplay_dg_fight_attacker_teleport_aborts_before_attack_commitment(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 6);
}

void Test_gameplay_dg_fight_weapon_extraction_aborts_before_attack_commitment(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 7);
}

void Test_gameplay_nested_riposte_has_distinct_committed_attack_identity(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 8);
}


void Test_gameplay_ranged_miss_commits_once_and_releases_projectile(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 9);
}

void Test_gameplay_dg_fight_ammunition_extraction_aborts_before_attack_commitment(CuTest *tc)
{
  verify_committed_attack_boundary(tc, 10);
}

struct defense_turn_trace
{
  struct char_data *subject;
  bool expired_before_action;
};

static bool observe_defense_turn(struct char_data *ch, unsigned int phase, void *context)
{
  struct defense_turn_trace *trace = context;

  (void)phase;
  if (ch == trace->subject)
    trace->expired_before_action = !has_defensive_casting_active(ch);
  return true;
}

static void verify_tactical_defense_clock(CuTest *tc, int scenario)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data *saved_characters = character_list;
  struct defense_turn_trace trace = {0};
  unsigned long saved_pulse = pulse;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  pulse = 24000U;
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.next = &f.victim;
  character_list = &f.actor;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &f.actor;
  combat_encounter_test_set_phase_callback(observe_defense_turn, &trace);
  if (scenario == 1 || scenario == 2 || scenario == 6 || scenario == 7)
  {
    FIGHTING(&f.actor) = &f.victim;
    FIGHTING(&f.victim) = &f.actor;
    CuAssertTrue(tc, combat_encounter_join(&f.actor, &f.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&f.victim, &f.actor, 1));
  }
  CuAssertTrue(tc, tactical_defense_start(&f.actor));
  CuAssertIntEquals(tc, 4, get_defensive_casting_ac_bonus(&f.actor));
  proc_d20_round_one(&f.actor);
  CuAssertTrue(tc, has_defensive_casting_active(&f.actor));
  pulse += 3 RL_SEC;
  CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
  if (scenario == 0)
  {
    tactical_defense_pause(&f.actor);
    pulse += 40 RL_SEC;
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
    tactical_defense_resume(&f.actor);
  }
  else if (scenario == 2 || scenario == 6)
  {
    combat_encounter_leave(&f.actor, COMBAT_ENCOUNTER_DEPARTURE_MOVED);
    combat_encounter_leave(&f.victim, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
    FIGHTING(&f.actor) = FIGHTING(&f.victim) = NULL;
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
    CuAssertIntEquals(tc, 0, f.actor.defensive_casting_turn);
    if (scenario == 6)
    {
      FIGHTING(&f.actor) = &f.victim;
      FIGHTING(&f.victim) = &f.actor;
      CuAssertTrue(tc, combat_encounter_join(&f.actor, &f.victim, 1));
      CuAssertTrue(tc, combat_encounter_join(&f.victim, &f.actor, 1));
      CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
    }
  }
  else if (scenario == 4)
  {
    CuAssertTrue(tc, tactical_defense_start(&f.actor));
    pulse += 3 RL_SEC;
    event_test_advance();
    CuAssertTrue(tc, has_defensive_casting_active(&f.actor));
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
  }
  else if (scenario == 5)
  {
    FIGHTING(&f.actor) = &f.victim;
    FIGHTING(&f.victim) = &f.actor;
    CuAssertTrue(tc, combat_encounter_join(&f.actor, &f.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&f.victim, &f.actor, 1));
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
  }
  else if (scenario == 7)
  {
    combat_encounter_runtime_shutdown();
    pulse += 40 RL_SEC;
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_defense_remaining(&f.actor));
    tactical_defense_resume(&f.actor);
  }
  pulse += (3 RL_SEC) - 1;
  event_test_advance();
  CuAssertTrue(tc, has_defensive_casting_active(&f.actor));
  pulse++;
  event_test_advance();
  CuAssertTrue(tc, !has_defensive_casting_active(&f.actor));
  if (scenario == 1)
    CuAssertTrue(tc, trace.expired_before_action);
  CuAssertIntEquals(tc, 0, tactical_defense_remaining(&f.actor));
  tactical_defense_pause(&f.actor);
  domain_event_runtime_shutdown();
  event_free_all();
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
}

void Test_gameplay_defensive_casting_preserves_paused_native_interval(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 0);
}

void Test_gameplay_defensive_casting_expires_before_semantic_action(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 1);
}

void Test_gameplay_defensive_casting_combat_departure_preserves_residual_interval(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 2);
}

/** Round-trip tactical clocks in isolated player files for active, expired, and legacy cases. */
static void verify_tactical_clock_persistence(CuTest *tc, int format, bool bleeding)
{
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *source = new_char();
  struct char_data *loaded = new_char();
  char directory[PATH_MAX], filename[MAX_FILEPATH], name[32];
  FILE *file;
  int result, remaining, timer;
  int source_remaining;
  struct affected_type af;

  snprintf(name, sizeof(name), "Zzdf%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4250;
  index[0].level = 7;
  player_table = index;
  top_of_p_table = 0;
  source->player.name = strdup(name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4250;
  GET_LEVEL(source) = 7;
  if (bleeding)
  {
    new_affect(&af);
    af.spell = ABILITY_BLEEDING_CRITICAL;
    af.duration = 2;
    af.modifier = 5;
    SET_BIT_AR(af.bitvector, AFF_BLEED);
    affect_to_char(source, &af);
    source->bleeding_critical_pulses = 17;
  }
  else
  {
    GET_DEFENSIVE_CASTING_TIMER(source) = 1;
    source->player_specials->saved.defensive_casting_pulses = 17;
  }
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  if (format == 1)
    CuAssertTrue(tc, save_char_checked(source, 0));
  else
  {
    file = fopen(filename, "w");
    CuAssertPtrNotNull(tc, file);
    fprintf(file, "Name: %s\nId  : 4250\nLevl: 7\nPDCt: %s\n", name, format == 0 ? "1" : "1 0");
    fclose(file);
  }
  result = load_char(name, loaded);
  remaining = bleeding ? tactical_bleeding_remaining(loaded) : tactical_defense_remaining(loaded);
  source_remaining =
      bleeding ? tactical_bleeding_remaining(source) : tactical_defense_remaining(source);
  timer = bleeding ? (loaded->affected != NULL ? loaded->affected->duration : 0)
                   : GET_DEFENSIVE_CASTING_TIMER(loaded);
  unlink(filename);
  CuAssertIntEquals(tc, 0, leave_player_fixture(directory, temporary_directory));
  free_char(source);
  free_char(loaded);
  player_table = saved_table;
  top_of_p_table = saved_top;
  CuAssertIntEquals(tc, 0, result);
  CuAssertIntEquals(tc, format == 0 ? 6 RL_SEC : format == 1 ? 17 : 0, remaining);
  CuAssertIntEquals(tc, bleeding ? 2 : format == 2 ? 0 : 1, timer);
  if (format == 1)
    CuAssertIntEquals(tc, 17, source_remaining);
}

void Test_gameplay_defensive_casting_loads_legacy_round_timer(CuTest *tc)
{
  verify_tactical_clock_persistence(tc, 0, false);
}

void Test_gameplay_defensive_casting_round_trips_residual_pulses(CuTest *tc)
{
  verify_tactical_clock_persistence(tc, 1, false);
}

void Test_gameplay_defensive_casting_does_not_restore_expired_saved_interval(CuTest *tc)
{
  verify_tactical_clock_persistence(tc, 2, false);
}

void Test_gameplay_defensive_casting_live_character_without_descriptor_keeps_expiring(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 3);
}

void Test_gameplay_defensive_casting_refresh_replaces_original_expiry(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 4);
}

void Test_gameplay_defensive_casting_combat_entry_does_not_extend_elapsed_interval(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 5);
}

void Test_gameplay_defensive_casting_combat_reentry_does_not_restart_interval(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 6);
}

void Test_gameplay_defensive_casting_shutdown_captures_semantic_residual(CuTest *tc)
{
  verify_tactical_defense_clock(tc, 7);
}

struct bleeding_clock_trace
{
  struct char_data *subject;
  unsigned int count;
  unsigned int count_before_action;
  int last_amount;
  int mutation;
  bool leave_on_action;
};

static void observe_bleeding_damage(const struct domain_event_context *context, void *data)
{
  struct bleeding_clock_trace *trace = data;
  const struct domain_character_damaged *event = context->payload;
  struct affected_type replacement;

  if (event->damage_type != DAM_BLEEDING)
    return;
  trace->count++;
  trace->last_amount = event->amount;
  if (trace->count != 1U || trace->mutation == 0)
    return;
  affect_from_char(trace->subject, ABILITY_BLEEDING_CRITICAL);
  if (trace->mutation == 2)
  {
    new_affect(&replacement);
    replacement.spell = ABILITY_BLEEDING_CRITICAL;
    replacement.duration = 1;
    replacement.modifier = 7;
    SET_BIT_AR(replacement.bitvector, AFF_BLEED);
    affect_to_char(trace->subject, &replacement);
  }
}

static bool observe_bleeding_turn(struct char_data *ch, unsigned int phase, void *data)
{
  struct bleeding_clock_trace *trace = data;

  (void)phase;
  if (ch == trace->subject)
  {
    trace->count_before_action = trace->count;
    if (trace->leave_on_action)
    {
      struct char_data *opponent = FIGHTING(ch);

      trace->leave_on_action = false;
      combat_encounter_leave(ch, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
      combat_encounter_leave(opponent, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
      FIGHTING(ch) = FIGHTING(opponent) = NULL;
    }
  }
  return true;
}

static void verify_bleeding_clock(CuTest *tc, int scenario)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data *saved_characters = character_list;
  struct bleeding_clock_trace trace = {0};
  struct domain_event_subscription_config config = {0};
  struct domain_event_subscription_handle subscription;
  struct affected_type af;
  unsigned long saved_pulse = pulse;

  begin_gameplay_fixture(&f);
  domain_event_runtime_shutdown();
  event_free_all();
  pulse = 28000U;
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.next = &f.victim;
  character_list = &f.actor;
  GET_HIT(&f.actor) = GET_MAX_HIT(&f.actor) = 100000;
  affected_registry_attach(&f.actor);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &f.actor;
  trace.leave_on_action = scenario == 9;
  trace.mutation = scenario == 3 ? 1 : scenario == 4 ? 2 : 0;
  config.type = DOMAIN_EVENT_CHARACTER_DAMAGED;
  config.owner = domain_event_character_handle(&f.actor);
  config.topic = (struct domain_event_topic){DOMAIN_EVENT_TOPIC_SUBJECT, config.owner};
  config.identity = "test.bleeding.clock";
  config.handler = observe_bleeding_damage;
  config.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &config, &subscription));
  combat_encounter_test_set_phase_callback(observe_bleeding_turn, &trace);
  if (scenario == 1 || scenario == 2 || scenario == 8 || scenario == 9)
  {
    FIGHTING(&f.actor) = &f.victim;
    FIGHTING(&f.victim) = &f.actor;
    CuAssertTrue(tc, combat_encounter_join(&f.actor, &f.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&f.victim, &f.actor, 1));
  }
  new_affect(&af);
  af.spell = ABILITY_BLEEDING_CRITICAL;
  af.duration = 2;
  af.modifier = 5;
  SET_BIT_AR(af.bitvector, AFF_BLEED);
  if (scenario == 8)
    f.actor.bleeding_critical_pulses = 6 RL_SEC;
  affect_to_char(&f.actor, &af);
  affect_update_character_one(&f.actor);
  update_damage_and_effects_over_time_one(&f.actor);
  CuAssertIntEquals(tc, 0, trace.count);
  CuAssertIntEquals(tc, 2, f.actor.affected->duration);
  if (scenario == 7)
  {
    FIGHTING(&f.actor) = &f.victim;
    FIGHTING(&f.victim) = &f.actor;
    CuAssertTrue(tc, combat_encounter_join(&f.actor, &f.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&f.victim, &f.actor, 1));
  }
  pulse += 3 RL_SEC;
  if (scenario == 6)
  {
    af.duration = 1;
    af.modifier = 7;
    affect_join(&f.actor, &af, false, false, true, false);
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_bleeding_remaining(&f.actor));
  }
  if (scenario == 2)
  {
    combat_encounter_leave(&f.actor, COMBAT_ENCOUNTER_DEPARTURE_MOVED);
    combat_encounter_leave(&f.victim, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
    FIGHTING(&f.actor) = FIGHTING(&f.victim) = NULL;
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_bleeding_remaining(&f.actor));
  }
  if (scenario == 5)
  {
    affected_registry_detach(&f.actor);
    pulse += 40 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 0, trace.count);
    CuAssertIntEquals(tc, 3 RL_SEC, tactical_bleeding_remaining(&f.actor));
    affected_registry_attach(&f.actor);
  }
  pulse += 3 RL_SEC;
  event_test_advance();
  if (scenario == 9)
  {
    CuAssertIntEquals(tc, 0, trace.count);
    CuAssertIntEquals(tc, 1, tactical_bleeding_remaining(&f.actor));
    pulse++;
    event_test_advance();
  }
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertIntEquals(tc, scenario == 6 ? 12 : 5, trace.last_amount);
  if (scenario == 1)
    CuAssertIntEquals(tc, 0, trace.count_before_action);
  pulse += 6 RL_SEC;
  event_test_advance();
  CuAssertIntEquals(tc, scenario == 3 || scenario == 6 ? 1 : 2, trace.count);
  if (scenario == 4)
    CuAssertIntEquals(tc, 7, trace.last_amount);
  if (scenario == 1)
    CuAssertIntEquals(tc, 1, trace.count_before_action);
  CuAssertTrue(tc, !affected_by_spell(&f.actor, ABILITY_BLEEDING_CRITICAL));
  affected_registry_detach(&f.actor);
  domain_event_runtime_shutdown();
  event_free_all();
  if (f.actor.events != NULL)
    free_list(f.actor.events);
  if (f.victim.events != NULL)
    free_list(f.victim.events);
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&f);
}

void Test_gameplay_bleeding_critical_owns_native_damage_and_duration(CuTest *tc)
{
  verify_bleeding_clock(tc, 0);
}

void Test_gameplay_bleeding_critical_runs_after_subject_actions(CuTest *tc)
{
  verify_bleeding_clock(tc, 1);
}

void Test_gameplay_bleeding_critical_preserves_combat_departure_interval(CuTest *tc)
{
  verify_bleeding_clock(tc, 2);
}

void Test_gameplay_bleeding_critical_cure_during_damage_stops_next_tick(CuTest *tc)
{
  verify_bleeding_clock(tc, 3);
}

void Test_gameplay_bleeding_critical_replacement_during_damage_keeps_new_clock(CuTest *tc)
{
  verify_bleeding_clock(tc, 4);
}

void Test_gameplay_bleeding_critical_removal_preserves_residual_interval(CuTest *tc)
{
  verify_bleeding_clock(tc, 5);
}

void Test_gameplay_bleeding_critical_save_preserves_live_and_loaded_residual(CuTest *tc)
{
  verify_tactical_clock_persistence(tc, 1, true);
}

void Test_gameplay_bleeding_critical_stacking_preserves_the_pending_tick(CuTest *tc)
{
  verify_bleeding_clock(tc, 6);
}

struct hazard_exposure_trace
{
  struct char_data *subject;
  unsigned int count;
  uint64_t source_identity;
};

static bool observe_hazard_exposure(struct raff_node *source, struct char_data *subject,
                                    void *context)
{
  struct hazard_exposure_trace *trace = context;

  if (subject != trace->subject)
    return false;
  trace->count++;
  trace->source_identity = source->source_identity;
  return true;
}

static bool skip_hazard_test_actions(struct char_data *subject, unsigned int phase, void *context)
{
  (void)subject;
  (void)phase;
  (void)context;
  return true;
}

static struct raff_node *create_test_billowing_source(CuTest *tc, struct gameplay_fixture *fixture)
{
  struct raff_node *source;

  CREATE(source, struct raff_node, 1);
  source->room = 1;
  source->timer = 15;
  source->affection = RAFF_BILLOWING;
  source->spell = SPELL_BILLOWING_CLOUD;
  CuAssertTrue(tc, tactical_room_hazard_prepare_source(source, 10));
  source->next = raff_list;
  raff_list = source;
  affected_room_owner_add(source);
  SET_BIT(fixture->rooms[1].room_affections, RAFF_BILLOWING);
  tactical_room_hazard_source_created(source);
  return source;
}

static void verify_billowing_cloud_exposure(CuTest *tc, int scenario)
{
  struct gameplay_fixture fixture;
  struct char_data *saved_characters = character_list;
  struct raff_node *saved_raff_list = raff_list;
  struct raff_node *source;
  struct raff_node *second_source = NULL;
  struct hazard_exposure_trace trace = {0};
  uint64_t rejected_before;
  unsigned long saved_pulse = pulse;

  begin_gameplay_fixture(&fixture);
  domain_event_runtime_shutdown();
  event_free_all();
  raff_list = NULL;
  pulse = 32000U;
  fixture.actor.next = &fixture.victim;
  character_list = &fixture.actor;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &fixture.actor;
  tactical_effects_set_hazard_test_callback(observe_hazard_exposure, &trace);
  rejected_before = tactical_room_hazard_exposure_rejections();
  if (scenario == 3)
    tactical_effects_set_hazard_exposure_limit_for_test(0U);
  source = create_test_billowing_source(tc, &fixture);
  if (scenario == 4)
    source->timer = 1;
  if (scenario == 5)
    second_source = create_test_billowing_source(tc, &fixture);
  if (scenario == 6)
    GET_LEVEL(&fixture.actor) = 13;

  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 1, &fixture.victim, DOMAIN_RELOCATION_FORCED, NORTH);
  if (scenario == 3 || scenario == 6)
  {
    CuAssertIntEquals(tc, 0, trace.count);
    CuAssertIntEquals(tc, 0, tactical_room_hazard_exposures());
    CuAssertTrue(tc, tactical_room_hazard_exposure_rejections() ==
                         rejected_before + (scenario == 3 ? 1U : 0U));
  }
  else
  {
    CuAssertIntEquals(tc, scenario == 5 ? 2 : 1, trace.count);
    CuAssertTrue(tc, trace.source_identity == source->source_identity);
    CuAssertIntEquals(tc, scenario == 5 ? 2 : 1, tactical_room_hazard_exposures());
  }

  if (scenario == 0)
  {
    char_from_room(&fixture.actor);
    char_to_room_cause(&fixture.actor, 0, &fixture.victim, DOMAIN_RELOCATION_FORCED, SOUTH);
    char_from_room(&fixture.actor);
    char_to_room_cause(&fixture.actor, 1, &fixture.victim, DOMAIN_RELOCATION_FORCED, NORTH);
    CuAssertIntEquals(tc, 1, trace.count);
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 2, trace.count);
    CuAssertTrue(tc, !is_action_available(&fixture.actor, atMOVE, false));
  }
  else if (scenario == 1)
  {
    char_from_room(&fixture.victim);
    char_to_room_cause(&fixture.victim, 1, NULL, DOMAIN_RELOCATION_WALK, NORTH);
    FIGHTING(&fixture.actor) = &fixture.victim;
    FIGHTING(&fixture.victim) = &fixture.actor;
    combat_encounter_test_set_phase_callback(skip_hazard_test_actions, NULL);
    CuAssertTrue(tc, combat_encounter_join(&fixture.actor, &fixture.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&fixture.victim, &fixture.actor, 1));
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 2, trace.count);
    CuAssertTrue(tc, !is_action_available(&fixture.actor, atMOVE, false));
    combat_encounter_leave(&fixture.actor, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
    combat_encounter_leave(&fixture.victim, COMBAT_ENCOUNTER_DEPARTURE_STOPPED);
    FIGHTING(&fixture.actor) = FIGHTING(&fixture.victim) = NULL;
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 3, trace.count);
  }
  else if (scenario == 2)
  {
    rem_room_aff(source);
    source = NULL;
    CuAssertIntEquals(tc, 0, tactical_room_hazard_exposures());
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 1, trace.count);
  }
  else if (scenario == 4)
  {
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 1, trace.count);
    CuAssertPtrEquals(tc, NULL, raff_list);
    CuAssertIntEquals(tc, 0, tactical_room_hazard_exposures());
    source = NULL;
  }
  else if (scenario == 5)
  {
    CuAssertTrue(tc, source->source_identity != second_source->source_identity);
    char_from_room(&fixture.actor);
    char_to_room_cause(&fixture.actor, 0, NULL, DOMAIN_RELOCATION_WALK, SOUTH);
    char_from_room(&fixture.actor);
    char_to_room_cause(&fixture.actor, 1, NULL, DOMAIN_RELOCATION_WALK, NORTH);
    CuAssertIntEquals(tc, 2, trace.count);
  }
  else if (scenario == 7)
  {
    char_from_room(&fixture.victim);
    char_to_room_cause(&fixture.victim, 1, NULL, DOMAIN_RELOCATION_WALK, NORTH);
    FIGHTING(&fixture.actor) = &fixture.victim;
    FIGHTING(&fixture.victim) = &fixture.actor;
    CuAssertTrue(tc, combat_encounter_join(&fixture.actor, &fixture.victim, 1));
    CuAssertTrue(tc, combat_encounter_join(&fixture.victim, &fixture.actor, 1));
    CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
    CuAssertIntEquals(tc, 0, tactical_room_hazard_exposures());
    event_free_all();
    event_init();
    CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
    CuAssertIntEquals(tc, 2, tactical_room_hazard_exposures());
    pulse += 6 RL_SEC;
    event_test_advance();
    CuAssertIntEquals(tc, 2, trace.count);
    FIGHTING(&fixture.actor) = FIGHTING(&fixture.victim) = NULL;
  }

  if (second_source != NULL)
    rem_room_aff(second_source);
  if (source != NULL)
    rem_room_aff(source);
  tactical_effects_set_hazard_test_callback(NULL, NULL);
  tactical_effects_set_hazard_exposure_limit_for_test(1024U);
  domain_event_runtime_shutdown();
  event_free_all();
  if (fixture.actor.events != NULL)
    free_list(fixture.actor.events);
  if (fixture.victim.events != NULL)
    free_list(fixture.victim.events);
  character_list = saved_characters;
  raff_list = saved_raff_list;
  pulse = saved_pulse;
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_billowing_cloud_deduplicates_forced_reentry_and_native_tick(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 0);
}

void Test_gameplay_billowing_cloud_moves_native_exposure_to_subject_turn_end(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 1);
}

void Test_gameplay_billowing_cloud_vanished_source_cancels_future_exposure(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 2);
}

void Test_gameplay_billowing_cloud_rejects_untracked_exposure_at_capacity(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 3);
}

void Test_gameplay_billowing_cloud_expiry_wins_at_shared_deadline(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 4);
}

void Test_gameplay_billowing_cloud_distinct_sources_keep_distinct_budgets(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 5);
}

void Test_gameplay_billowing_cloud_preserves_level_thirteen_immunity(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 6);
}

void Test_gameplay_billowing_cloud_rebuilds_after_event_runtime_restart(CuTest *tc)
{
  verify_billowing_cloud_exposure(tc, 7);
}

struct wall_crossing_trace
{
  struct char_data *subject;
  struct obj_data *extract_after_first;
  struct domain_entity_handle sources[4];
  int count;
};

static bool observe_wall_crossing(struct domain_entity_handle source, struct char_data *subject,
                                  void *context)
{
  struct wall_crossing_trace *trace = context;

  if (subject != trace->subject)
    return false;
  if (trace->count < 4)
    trace->sources[trace->count] = source;
  trace->count++;
  if (trace->count == 1 && trace->extract_after_first != NULL)
  {
    extract_obj(trace->extract_after_first);
    trace->extract_after_first = NULL;
  }
  return true;
}

static struct obj_data *create_test_wall(room_rnum room, int dir, int type)
{
  struct obj_data *wall = create_obj();

  GET_OBJ_TYPE(wall) = ITEM_WALL;
  GET_OBJ_VAL(wall, WALL_TYPE) = type;
  GET_OBJ_VAL(wall, WALL_DIR) = dir;
  GET_OBJ_VAL(wall, WALL_LEVEL) = 10;
  GET_OBJ_VAL(wall, WALL_IDNUM) = 999999;
  wall->name = strdup("test wall");
  wall->short_description = strdup("a test wall");
  wall->description = strdup("A test wall crosses the way.");
  obj_to_room(wall, room);
  return wall;
}

void Test_gameplay_wall_crossing_uses_one_committed_fact_per_source(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct wall_crossing_trace trace = {0};
  struct obj_data *origin_wall;
  struct obj_data *destination_wall;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &fixture.actor;
  wall_crossing_set_test_callback(observe_wall_crossing, &trace);
  origin_wall = create_test_wall(0, NORTH, WALL_TYPE_FIRE);
  destination_wall = create_test_wall(1, SOUTH, WALL_TYPE_THORNS);

  CuAssertIntEquals(tc, 1, perform_move(&fixture.actor, NORTH, FALSE));
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 2, trace.count);
  CuAssertTrue(
      tc, domain_entity_handle_equal(domain_event_object_handle(origin_wall), trace.sources[0]));
  CuAssertTrue(tc, domain_entity_handle_equal(domain_event_object_handle(destination_wall),
                                              trace.sources[1]));

  wall_crossing_set_test_callback(NULL, NULL);
  extract_obj(origin_wall);
  extract_obj(destination_wall);
  domain_event_runtime_shutdown();
  event_free_all();
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_blocking_destination_wall_vetoes_before_relocation(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct wall_crossing_trace trace = {0};
  struct obj_data *wall;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &fixture.actor;
  wall_crossing_set_test_callback(observe_wall_crossing, &trace);
  wall = create_test_wall(1, SOUTH, WALL_TYPE_FORCE);

  CuAssertIntEquals(tc, 0, perform_move(&fixture.actor, NORTH, FALSE));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 0, trace.count);

  wall_crossing_set_test_callback(NULL, NULL);
  extract_obj(wall);
  domain_event_runtime_shutdown();
  event_free_all();
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_forced_wall_crossing_honors_vanished_sources(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct wall_crossing_trace trace = {0};
  struct obj_data *wall;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &fixture.actor;
  wall_crossing_set_test_callback(observe_wall_crossing, &trace);
  wall = create_test_wall(0, NORTH, WALL_TYPE_FORCE);
  extract_obj(wall);

  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 1, &fixture.victim, DOMAIN_RELOCATION_FORCED, NORTH);
  CuAssertIntEquals(tc, 0, trace.count);
  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 0, &fixture.victim, DOMAIN_RELOCATION_FORCED, SOUTH);
  wall = create_test_wall(0, NORTH, WALL_TYPE_FORCE);

  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 1, &fixture.victim, DOMAIN_RELOCATION_FORCED, NORTH);
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertTrue(tc, domain_entity_handle_equal(domain_event_object_handle(wall), trace.sources[0]));

  wall_crossing_set_test_callback(NULL, NULL);
  extract_obj(wall);
  domain_event_runtime_shutdown();
  event_free_all();
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_wall_crossing_does_not_dereference_an_extracted_successor(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct wall_crossing_trace trace = {0};
  struct obj_data *first_wall;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  trace.subject = &fixture.actor;
  trace.extract_after_first = create_test_wall(0, NORTH, WALL_TYPE_THORNS);
  first_wall = create_test_wall(0, NORTH, WALL_TYPE_FIRE);
  wall_crossing_set_test_callback(observe_wall_crossing, &trace);

  CuAssertIntEquals(tc, 1, perform_move(&fixture.actor, NORTH, FALSE));
  CuAssertIntEquals(tc, 1, trace.count);
  CuAssertTrue(
      tc, domain_entity_handle_equal(domain_event_object_handle(first_wall), trace.sources[0]));

  wall_crossing_set_test_callback(NULL, NULL);
  extract_obj(first_wall);
  domain_event_runtime_shutdown();
  event_free_all();
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_bleeding_critical_native_tick_and_combat_turn_share_one_interval(CuTest *tc)
{
  verify_bleeding_clock(tc, 7);
}

void Test_gameplay_bleeding_critical_combat_turn_before_native_tick_shares_interval(CuTest *tc)
{
  verify_bleeding_clock(tc, 8);
}

void Test_gameplay_bleeding_critical_leaving_during_action_preserves_due_end_tick(CuTest *tc)
{
  verify_bleeding_clock(tc, 9);
}

struct phenomenon_response_trace
{
  int started;
  int cleared;
};

static void capture_phenomenon_response(struct char_data *observer, bool responding, void *data)
{
  struct phenomenon_response_trace *trace = data;

  (void)observer;
  if (responding)
    trace->started++;
  else
    trace->cleared++;
}

void Test_gameplay_npc_phenomenon_interest_replaces_expires_and_investigates(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data *saved_characters = character_list;
  struct domain_phenomenon_perceived perceived = {0};
  struct phenomenon_response_trace trace = {0};
  struct event_runtime_handle first_interest;
  unsigned long saved_pulse = pulse;
  unsigned long count;

  begin_gameplay_fixture(&fixture);
  char_from_room(&fixture.victim);
  char_to_room(&fixture.victim, 1);
  fixture.actor.next = &fixture.victim;
  character_list = &fixture.actor;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_GUARD);
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_SENTINEL);

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(false);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 100U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  phenomenon_response_set_test_callback(capture_phenomenon_response, &trace);

  perceived.phenomenon_id = 7001U;
  perceived.observer = domain_event_character_handle(&fixture.actor);
  perceived.phenomenon_source = domain_event_character_handle(&fixture.victim);
  perceived.source_room = domain_event_room_handle(1);
  perceived.kind = DOMAIN_PHENOMENON_MAGIC_IMPACT;
  perceived.senses = DOMAIN_WORLD_PHENOMENON_AUDIBLE;
  perceived.distance = 1U;
  perceived.intensity = 1.0f;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(),
                                         DOMAIN_EVENT_PHENOMENON_PERCEIVED, &perceived));
  first_interest = fixture.actor.phenomenon_interest_event;
  CuAssertTrue(tc, !event_runtime_handle_is_none(first_interest));
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 1, trace.started);
  CuAssertTrue(tc, AFF_FLAGGED(&fixture.actor, AFF_TOTAL_DEFENSE));

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(),
                                         DOMAIN_EVENT_PHENOMENON_PERCEIVED, &perceived));
  CuAssertTrue(
      tc, event_runtime_handles_equal(first_interest, fixture.actor.phenomenon_interest_event));
  perceived.phenomenon_id++;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(),
                                         DOMAIN_EVENT_PHENOMENON_PERCEIVED, &perceived));
  CuAssertIntEquals(tc, 1, trace.cleared);
  CuAssertTrue(tc, !AFF_FLAGGED(&fixture.actor, AFF_TOTAL_DEFENSE));
  CuAssertTrue(
      tc, !event_runtime_handles_equal(first_interest, fixture.actor.phenomenon_interest_event));
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 2, trace.started);
  CuAssertTrue(tc, AFF_FLAGGED(&fixture.actor, AFF_TOTAL_DEFENSE));
  for (count = 0U; count < 30U * PASSES_PER_SEC; count++)
  {
    pulse++;
    event_test_advance();
  }
  CuAssertIntEquals(tc, 2, trace.cleared);
  CuAssertTrue(tc, event_runtime_handle_is_none(fixture.actor.phenomenon_interest_event));
  CuAssertTrue(tc, !AFF_FLAGGED(&fixture.actor, AFF_TOTAL_DEFENSE));

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_GUARD);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_SENTINEL);
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_LISTEN);
  perceived.phenomenon_id++;
  perceived.kind = DOMAIN_PHENOMENON_ALARM;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    DOMAIN_EVENT_PUBLISH(domain_event_runtime_bus(),
                                         DOMAIN_EVENT_PHENOMENON_PERCEIVED, &perceived));
  pulse++;
  event_test_advance();
  CuAssertIntEquals(tc, 1, IN_ROOM(&fixture.actor));
  CuAssertIntEquals(tc, 3, trace.started);

  phenomenon_response_set_test_callback(NULL, NULL);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&fixture);
}

void Test_gameplay_search_commits_after_owned_work_and_cancels_on_movement(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};
  struct primary_activity_snapshot snapshot;
  struct char_data *saved_characters = character_list;
  unsigned long saved_pulse = pulse;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  fixture.actor.player.name = "search fixture";
  GET_LEVEL(&fixture.actor) = LVL_IMPL;
  GET_ABILITY(&fixture.actor, ABILITY_PERCEPTION) = 100;
  fixture.rooms[0].light = 1;
  SET_BIT(fixture.exits[0].exit_info, EX_HIDDEN | EX_HIDDEN_EASY);
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &fixture.actor;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.connected = CON_PLAYING;
  fixture.actor.desc = &descriptor;
  fixture.actor.next = &fixture.victim;
  character_list = &fixture.actor;

  event_free_all();
  active_world_reset_for_test();
  active_world_select_for_test(false);
  character_periodic_reset_for_test();
  character_periodic_select_for_test(false);
  point_update_periodic_reset_for_test();
  point_update_periodic_select_for_test(false);
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  pulse = 200U;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());

  do_search(&fixture.actor, "", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_SEARCH, snapshot.type);
  CuAssertTrue(tc, EXIT_FLAGGED(&fixture.exits[0], EX_HIDDEN));
  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 1, NULL, DOMAIN_RELOCATION_WALK, NORTH);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, EXIT_FLAGGED(&fixture.exits[0], EX_HIDDEN));

  char_from_room(&fixture.actor);
  char_to_room_cause(&fixture.actor, 0, NULL, DOMAIN_RELOCATION_WALK, SOUTH);
  do_search(&fixture.actor, "", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.actor, &snapshot));
  pulse += PULSE_VIOLENCE;
  event_test_advance();
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.actor, &snapshot));
  CuAssertTrue(tc, !EXIT_FLAGGED(&fixture.exits[0], EX_HIDDEN));

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_shutdown());
  event_free_all();
  active_world_reset_for_test();
  character_periodic_reset_for_test();
  point_update_periodic_reset_for_test();
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  ProtocolDestroy(descriptor.pProtocol);
  fixture.actor.desc = NULL;
  fixture.actor.next = NULL;
  character_list = saved_characters;
  pulse = saved_pulse;
  end_gameplay_fixture(&fixture);
}

/* Keeper storage round trip: store a live pet, keep it out of ordinary active
 * snapshots, then reclaim the same pet with its saved identity intact. */
static MYSQL *open_keeper_test_database(void)
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
                         port_text ? (unsigned int)strtoul(port_text, NULL, 10) : 3306, NULL,
                         0) == NULL)
  {
    mysql_close(connection);
    return NULL;
  }
  return connection;
}

static bool create_keeper_temporary_schema(MYSQL *connection)
{
  const char *queries[] = {
      "CREATE TEMPORARY TABLE pet_data ("
      "pet_data_id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, "
      "owner_name VARCHAR(50) NOT NULL, pet_name VARCHAR(50), pet_sdesc VARCHAR(255), "
      "pet_ldesc TEXT, pet_ddesc TEXT, vnum INT NOT NULL, level INT NOT NULL, "
      "hp INT NOT NULL, max_hp INT NOT NULL, str INT NOT NULL, con INT NOT NULL, "
      "dex INT NOT NULL, ac INT NOT NULL, intel INT NOT NULL, wis INT NOT NULL, "
      "cha INT NOT NULL, runtime_state LONGTEXT, owner_id INT UNSIGNED NOT NULL DEFAULT 0, "
      "owner_created BIGINT NOT NULL DEFAULT 0, pet_state TINYINT NOT NULL DEFAULT 0"
      ") ENGINE=InnoDB",
      "CREATE TEMPORARY TABLE pet_save_objs ("
      "idnum INT UNSIGNED AUTO_INCREMENT PRIMARY KEY, pet_idnum BIGINT NOT NULL, "
      "owner_name VARCHAR(50) NOT NULL, serialized_obj TEXT NOT NULL, "
      "creation_date TIMESTAMP DEFAULT CURRENT_TIMESTAMP) ENGINE=InnoDB",
      NULL};
  int index;

  for (index = 0; queries[index] != NULL; index++)
    if (mysql_query(connection, queries[index]))
      return false;
  return true;
}

static int keeper_query_int(MYSQL *connection, const char *query)
{
  MYSQL_RES *result;
  MYSQL_ROW row;
  int value = -1;

  if (mysql_query(connection, query))
    return -1;
  result = mysql_store_result(connection);
  if (result == NULL)
    return -1;
  row = mysql_fetch_row(result);
  if (row && row[0])
    value = atoi(row[0]);
  mysql_free_result(result);
  return value;
}

void Test_copyover_pet_preflight_retains_linkdead_pets_on_failure_and_retries(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data prototype, connected, linkdead, menu;
  struct player_special_data connected_specials = {0}, linkdead_specials = {0}, menu_specials = {0};
  struct descriptor_data descriptor = {0};
  struct char_data *pet;
  struct obj_data *item;
  struct char_data *saved_prototypes = mob_proto;
  struct char_data *saved_characters = character_list;
  MYSQL *saved_conn = conn;
  bool saved_available = mysql_available;
  MYSQL *connection;
  const char *enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  bool initial, failed, retained, retried, restore_blocked;
  long int pet_id;

  if (enabled == NULL || strcmp(enabled, "1") != 0)
    return;
  connection = open_keeper_test_database();
  CuAssertPtrNotNull(tc, connection);
  CuAssertTrue(tc, create_keeper_temporary_schema(connection));
  conn = connection;
  mysql_available = true;
  begin_gameplay_fixture(&fixture);
  initialize_test_npc(&prototype, "a copyover companion", NOWHERE);
  prototype.player.name = (char *)"companion";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  clear_char(&connected);
  clear_char(&linkdead);
  clear_char(&menu);
  menu.player_specials = &menu_specials;
  connected.player_specials = &connected_specials;
  linkdead.player_specials = &linkdead_specials;
  connected.player.name = (char *)"CopyoverConnected";
  linkdead.player.name = (char *)"CopyoverLinkdead";
  GET_IDNUM(&connected) = 5301;
  GET_IDNUM(&linkdead) = 5302;
  connected.player.time.birth = linkdead.player.time.birth = (time_t)1234;
  connected.pet_roster_load_state = linkdead.pet_roster_load_state = PET_ROSTER_LOADED;
  connected.desc = &descriptor;
  descriptor.character = &connected;
  STATE(&descriptor) = CON_PLAYING;
  char_to_room(&connected, 0);
  char_to_room(&linkdead, 0);
  /* A roomless, unloaded menu character must not prevent copyover. */
  connected.next = &linkdead;
  linkdead.next = &menu;
  character_list = &connected;
  pet = read_mobile(0, REAL);
  char_to_room(pet, 0);
  add_follower(pet, &linkdead);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  GET_HIT(pet) = 71;
  item = create_obj();
  GET_OBJ_RNUM(item) = NOTHING;
  item->name = strdup("keepsake");
  item->short_description = strdup("a keepsake");
  item->description = strdup("A keepsake lies here.");
  obj_to_char(item, pet);

  initial = save_player_pets();
  pet_id = pet->pet_data_id;
  GET_HIT(pet) = 63;
  /* The unchanged connected owner's cache is hit; the changed linkdead
   * owner's SQL transaction fails. Its previous complete snapshot survives. */
  mysql_test_fail_nth_query(1);
  failed = !save_player_pets();
  mysql_test_clear_query_failure();
  retained = pet_id > 0 && pet->pet_data_id == pet_id && pet->master == &linkdead &&
             pet->carrying == item && IN_ROOM(pet) == 0 && !MOB_FLAGGED(pet, MOB_NOTDEADYET) &&
             connected.desc == &descriptor && STATE(&descriptor) == CON_PLAYING &&
             keeper_query_int(connection, "SELECT hp FROM pet_data") == 71 &&
             keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 1;
  retried = save_player_pets() && pet->pet_data_id == pet_id &&
            keeper_query_int(connection, "SELECT hp FROM pet_data") == 63 &&
            keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 1;
  linkdead.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  restore_blocked = !save_player_pets() && pet->master == &linkdead && pet->carrying == item;
  linkdead.pet_roster_load_state = PET_ROSTER_LOADED;

  extract_obj(item);
  extract_char(pet);
  extract_pending_chars();
  char_from_room(&connected);
  char_from_room(&linkdead);
  domain_event_world_forget_character(&connected);
  domain_event_world_forget_character(&linkdead);
  character_list = saved_characters;
  mob_proto = saved_prototypes;
  end_gameplay_fixture(&fixture);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);
  CuAssertTrue(tc, initial);
  CuAssertTrue(tc, failed);
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, retried);
  CuAssertTrue(tc, restore_blocked);
}

struct keeper_publication_trace
{
  long int pet_id;
  int arrivals;
  bool published_with_gear;
  bool reject_arrival;
};

static void keeper_observe_publication(const struct domain_event_context *context, void *data)
{
  struct keeper_publication_trace *trace = data;
  const struct domain_character_moved *event = context->payload;
  struct char_data *pet = domain_event_world_resolve_character(event->character);

  if (!pet || pet->pet_data_id != trace->pet_id)
    return;
  trace->arrivals++;
  trace->published_with_gear = pet->carrying != NULL && GET_EQ(pet, WEAR_NECK_1) != NULL;
  if (trace->reject_arrival)
    extract_char(pet);
}

static void verify_named_pet_keeper_round_trip(CuTest *tc, bool eidolon)
{
  struct gameplay_fixture fixture;
  struct char_data prototype;
  struct char_data owner;
  struct char_data keeper;
  struct player_special_data owner_specials = {0};
  struct descriptor_data descriptor = {0};
  struct account_data account = {0};
  struct char_data *pet;
  struct char_data *reclaimed;
  struct char_data *saved_prototypes;
  struct char_data *saved_characters;
  struct index_data object_index = {0};
  struct obj_data object_prototype;
  struct index_data *saved_obj_index;
  struct obj_data *saved_obj_proto;
  struct obj_data *item;
  struct obj_data *keepsake;
  struct obj_data *necklace;
  struct affected_type charm_affect;
  obj_rnum saved_top_objt;
  const char *enabled;
  const char *reason;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool schema_created;
  bool gear_left_with_pet;
  bool retry_kept_one_copy;
  bool listed_id_matches;
  bool stored;
  bool owns_event_bus = domain_event_runtime_bus() == NULL;
  bool activation_failures_retained = true;
  bool publication_failure_retained;
  int failure_query, baseline_mobiles;
  struct keeper_publication_trace trace = {0};
  struct domain_event_subscription_config observer = {0};
  struct domain_event_subscription_handle subscription;
  bool named, name_denials, failed_name_retained;
  bool stable_failure_message;
  bool created_command_list;
  const char *invalid_names[] = {"",          "Ab",        "A-name-that-is-far-too-long",
                                 "two names", "Bad;name",  "Bad\nname",
                                 "Bad\tname", "Bad$Name",  "-Name",
                                 "Name-",     "followers", "restore",
                                 "self",      "all",       "The",
                                 "from",      "with",      "room",
                                 "someone"};
  size_t name_index;
  char name_command[96], respec_error[256], dismissal_error[512] = {0};
  bool snapshot_kept_storage, respec_preserved_storage;
  int owner_class = eidolon ? CLASS_SUMMONER : CLASS_WIZARD;
  bool reclaimed_identity;
  bool repeat_denied;
  bool foreign_owner_denied;
  bool dismissed_and_reacquired = !eidolon;
  long int pet_id;
  int stable_command, stored_count;

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }
  connection = open_keeper_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  saved_characters = character_list;
  conn = connection;
  mysql_available = true;
  schema_created = create_keeper_temporary_schema(connection);
  if (eidolon)
  {
    schema_created =
        schema_created &&
        mysql_query(connection,
                    "CREATE TEMPORARY TABLE player_eidolons (idnum INT, "
                    "owner VARCHAR(50), short_desc VARCHAR(120), long_desc VARCHAR(120))") == 0 &&
        mysql_query(connection,
                    "INSERT INTO player_eidolons VALUES "
                    "(1, 'KeeperOwner', 'an unnamed eidolon', 'An unnamed eidolon waits.')") == 0;
  }

  begin_gameplay_fixture(&fixture);
  if (owns_event_bus)
  {
    event_free_all();
    event_init();
    CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  }
  saved_prototypes = mob_proto;
  initialize_test_npc(&prototype, "a stabled companion", NOWHERE);
  prototype.player.name = (char *)"companion";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = 100;
  GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  if (eidolon)
  {
    SET_BIT_AR(MOB_FLAGS(&prototype), MOB_EIDOLON);
    fixture.mobile_index[0].vnum = MOB_NUM_EIDOLON;
  }
  mob_proto = &prototype;

  clear_char(&owner);
  owner.player_specials = &owner_specials;
  owner.player.name = (char *)"KeeperOwner";
  GET_LEVEL(&owner) = 20;
  GET_PFILEPOS(&owner) = -1;
  GET_REAL_RACE(&owner) = RACE_HUMAN;
  GET_CLASS(&owner) = owner_class;
  CLASS_LEVEL((&owner), owner_class) = 20;
  GET_EXP(&owner) = 12345;
  if (class_list[owner_class].name == NULL)
    load_class_list();
  GET_POS(&owner) = POS_STANDING;
  GET_IDNUM(&owner) = eidolon ? 5002 : 5001;
  owner.player.time.birth = (time_t)1234;
  owner.pet_roster_load_state = PET_ROSTER_LOADED;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &owner;
  descriptor.account = &account;
  descriptor.connected = CON_PLAYING;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  owner.desc = &descriptor;
  char_to_room(&owner, 0);
  initialize_test_npc(&keeper, "stable keeper", 0);
  created_command_list = complete_cmd_info == NULL;
  if (created_command_list)
    create_command_list();
  stable_command = find_command("stable");

  saved_obj_index = obj_index;
  saved_obj_proto = obj_proto;
  saved_top_objt = top_of_objt;
  clear_object(&object_prototype);
  object_prototype.name = (char *)"token";
  object_prototype.short_description = (char *)"a keeper token";
  object_prototype.description = (char *)"A keeper token lies here.";
  object_index.vnum = 900;
  object_index.number = 0;
  obj_proto = &object_prototype;
  obj_index = &object_index;
  top_of_objt = 0;

  pet = read_mobile(0, REAL);
  char_to_room(pet, 0);
  add_follower(pet, &owner);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
  pet->player.short_descr = strdup("the stabled companion");
  GET_LEVEL(pet) = 12;
  new_affect(&charm_affect);
  charm_affect.spell = SPELL_CHARM_MONSTER;
  charm_affect.duration = 12;
  SET_BIT_AR(charm_affect.bitvector, AFF_CHARM);
  affect_to_char(pet, &charm_affect);
  item = create_obj();
  /* A prototype-less item serializes in full, as restrung pet gear does. */
  GET_OBJ_RNUM(item) = NOTHING;
  item->name = strdup("token");
  item->short_description = strdup("a keeper token");
  item->description = strdup("A keeper token lies here.");
  GET_OBJ_TYPE(item) = ITEM_CONTAINER;
  GET_OBJ_VAL(item, 0) = 100;
  GET_OBJ_WEIGHT(item) = 3;
  GET_OBJ_BOUND_ID(item) = 424242;
  keepsake = create_obj();
  GET_OBJ_RNUM(keepsake) = NOTHING;
  GET_OBJ_WEIGHT(keepsake) = 7;
  keepsake->name = strdup("keepsake");
  keepsake->short_description = strdup("a keeper keepsake");
  keepsake->description = strdup("A keeper keepsake lies here.");
  obj_to_obj(keepsake, item);
  obj_to_char(item, pet);
  necklace = create_obj();
  GET_OBJ_RNUM(necklace) = NOTHING;
  GET_OBJ_TYPE(necklace) = ITEM_WORN;
  SET_BIT_AR(GET_OBJ_WEAR(necklace), ITEM_WEAR_NECK);
  necklace->name = strdup("necklace");
  necklace->short_description = strdup("a keeper necklace");
  necklace->description = strdup("A keeper necklace lies here.");
  equip_char(pet, necklace, WEAR_NECK_1);

  GET_REAL_MAX_HIT(pet) = GET_MAX_HIT(pet) = 100;
  GET_HIT(pet) = 70;
  GET_MOVE(pet) = 19;
  GET_PSP(pet) = 11;
  do_pets(&owner, "companion name Alder", 0, 0);
  named = !strcmp(GET_NAME(pet), "Alder") && !strcmp(pet->player.name, "Alder companion");
  snprintf(name_command, sizeof(name_command), "#%ld name O'Rowan", pet->pet_data_id);
  do_pets(&owner, name_command, 0, 0);
  named = named && !strcmp(GET_NAME(pet), "O'Rowan") &&
          !strcmp(pet->player.name, "O'Rowan companion") &&
          !strcmp(pet->player.long_descr, "O'Rowan is here.\r\n");
  name_denials = true;
  for (name_index = 0; name_index < sizeof(invalid_names) / sizeof(invalid_names[0]); name_index++)
    name_denials = !pet_set_custom_name(&owner, pet, invalid_names[name_index], &reason) &&
                   reason != NULL && name_denials;
  pet->master = NULL;
  name_denials = !pet_set_custom_name(&owner, pet, "Wrong", &reason) && name_denials;
  pet->master = &owner;
  mysql_test_fail_nth_query(1);
  failed_name_retained = !pet_set_custom_name(&owner, pet, "Unsaved", &reason);
  mysql_test_clear_query_failure();
  failed_name_retained = failed_name_retained && !strcmp(GET_NAME(pet), "O'Rowan") &&
                         !strcmp(pet->player.name, "O'Rowan companion") &&
                         !strcmp(pet->player.long_descr, "O'Rowan is here.\r\n");

  owner.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  pet_keeper(&owner, &keeper, stable_command, "store companion");
  stable_failure_message = strstr(descriptor.output, "stays at your side") != NULL &&
                           strstr(descriptor.output, "is led away to the stables") == NULL;
  owner.pet_roster_load_state = PET_ROSTER_LOADED;
  descriptor.output[0] = '\0';
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;

  stored = schema_created && pet_store_pet(&owner, pet);
  pet_id = pet->pet_data_id;
  /* A stored pet leaves play as part of the commit. */
  extract_pending_chars();
  stored_count = pet_stored_count(&owner);
  /* Stored gear travels with the pet; extraction must not drop a second copy. */
  gear_left_with_pet = stored && world[0].contents == NULL &&
                       keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 3;

  /* An ordinary active snapshot must not remove or duplicate stored rows. */
  snapshot_kept_storage =
      stored && save_char_pets(&owner) &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1") == 1 &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 0") == 0;

  /* Respec changes the owner while the stored pet keeps its own state. */
  do_respec(&owner, eidolon ? "summoner" : "wizard", 0, 0);
  respec_preserved_storage =
      GET_LEVEL(&owner) == 1 && GET_CLASS(&owner) == owner_class &&
      CLASS_LEVEL((&owner), owner_class) == 1 && GET_EXP(&owner) == 12345 &&
      HAS_FEAT(&owner, eidolon ? FEAT_EIDOLON : FEAT_SUMMON_FAMILIAR) && owner.followers == NULL &&
      pet_stored_id_at(&owner, 1) == pet_id &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1") == 1 &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 3;

  snprintf(respec_error, sizeof(respec_error),
           "respec class=%d level=%d ranks=%d xp=%ld feat=%d followers=%d stored=%ld: %.100s",
           GET_CLASS(&owner), GET_LEVEL(&owner), CLASS_LEVEL((&owner), owner_class),
           GET_EXP(&owner), HAS_FEAT(&owner, eidolon ? FEAT_EIDOLON : FEAT_SUMMON_FAMILIAR),
           owner.followers != NULL, pet_stored_id_at(&owner, 1), descriptor.output);

  /* The listed position resolves to the same stable ID a player would quote. */
  listed_id_matches = pet_stored_id_at(&owner, 1) == pet_id && pet_stored_id_at(&owner, 2) == 0;
  trace.pet_id = pet_id;
  observer.type = DOMAIN_EVENT_CHARACTER_MOVED;
  observer.topic.role = DOMAIN_EVENT_TOPIC_DESTINATION;
  observer.topic.entity = domain_event_room_handle(0);
  observer.owner = domain_event_character_handle(&owner);
  observer.identity = "test.keeper.committed-arrival";
  observer.handler = keeper_observe_publication;
  observer.handler_context = &trace;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_subscribe(domain_event_runtime_bus(), &observer, &subscription));
  baseline_mobiles = mob_index[0].number;
  /* The last two queries activate the row and commit. Eidolons read owner
   * descriptions once as well. Fail each after the inventory has decoded. */
  for (failure_query = eidolon ? 5 : 4; failure_query <= (eidolon ? 6 : 5); failure_query++)
  {
    mysql_test_fail_nth_query(failure_query);
    reclaimed = pet_retrieve_stored(&owner, pet_id, &reason);
    mysql_test_clear_query_failure();
    activation_failures_retained = !reclaimed && reason != NULL && activation_failures_retained;
    extract_pending_chars();
    activation_failures_retained =
        activation_failures_retained && owner.followers == NULL && world[0].contents == NULL &&
        trace.arrivals == 0 && mob_index[0].number == baseline_mobiles &&
        keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1") == 1 &&
        keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 3;
  }
  trace.reject_arrival = true;
  reason = NULL;
  reclaimed = pet_retrieve_stored(&owner, pet_id, &reason);
  extract_pending_chars();
  publication_failure_retained =
      reclaimed == NULL && reason != NULL && owner.followers == NULL && world[0].contents == NULL &&
      mob_index[0].number == baseline_mobiles &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1") == 1 &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 0") == 0 &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_save_objs") == 3 &&
      save_char_pets(&owner) &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1") == 1;
  trace.reject_arrival = false;
  trace.arrivals = 0;
  reason = NULL;
  reclaimed = stored ? pet_retrieve_stored(&owner, pet_id, &reason) : NULL;
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                    domain_event_unsubscribe(domain_event_runtime_bus(), subscription));
  reclaimed_identity =
      reclaimed != NULL && reclaimed->master == &owner && reclaimed->pet_data_id == pet_id &&
      GET_LEVEL(reclaimed) == 12 && GET_HIT(reclaimed) == 70 && GET_MOVE(reclaimed) == 19 &&
      GET_PSP(reclaimed) == 11 && reclaimed->player.short_descr != NULL &&
      !strcmp(reclaimed->player.short_descr, "O'Rowan") &&
      !strcmp(reclaimed->player.name, "O'Rowan companion") &&
      !strcmp(reclaimed->player.long_descr, "O'Rowan is here.\r\n") &&
      keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 0") == 1 &&
      reclaimed->carrying != NULL && reclaimed->carrying->name != NULL &&
      !strcmp(reclaimed->carrying->name, "token") && GET_OBJ_WEIGHT(reclaimed->carrying) == 10 &&
      GET_OBJ_BOUND_ID(reclaimed->carrying) == 424242 && reclaimed->carrying->contains != NULL &&
      reclaimed->carrying->contains->name != NULL &&
      !strcmp(reclaimed->carrying->contains->name, "keepsake") && reclaimed->affected != NULL &&
      reclaimed->affected->duration == 12 && GET_EQ(reclaimed, WEAR_NECK_1) != NULL &&
      !strcmp(GET_EQ(reclaimed, WEAR_NECK_1)->name, "necklace");

  /* A retry after a failed restore must not publish a live pet a second time. */
  owner.pet_roster_load_state = PET_ROSTER_UNLOADED;
  load_char_pets(&owner);
  retry_kept_one_copy = owner.followers != NULL && owner.followers->next == NULL &&
                        owner.pet_roster_load_state == PET_ROSTER_LOADED;
  owner.pet_roster_load_state = PET_ROSTER_LOADED;

  /* The same row cannot be reclaimed twice, and another owner cannot claim it. */
  reason = NULL;
  repeat_denied = pet_retrieve_stored(&owner, pet_id, &reason) == NULL && reason != NULL;
  mysql_query(connection, "UPDATE pet_data SET pet_state = 1");
  owner.player.time.birth = (time_t)9999;
  reason = NULL;
  foreign_owner_denied = pet_retrieve_stored(&owner, pet_id, &reason) == NULL && reason != NULL;

  if (reclaimed != NULL)
  {
    while (reclaimed->carrying)
      extract_obj(reclaimed->carrying);
    if (GET_EQ(reclaimed, WEAR_NECK_1))
      extract_obj(unequip_char(reclaimed, WEAR_NECK_1));
    if (eidolon)
    {
      owner.player.time.birth = (time_t)1234;
      mysql_query(connection, "UPDATE pet_data SET pet_state = 0");
      do_dismiss(&owner, "companion", 0, 0);
      extract_pending_chars();
      dismissed_and_reacquired = owner.followers == NULL &&
                                 keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data") == 0;
      do_call(&owner, "eidolon", 0, 0);
      reclaimed = owner.followers != NULL ? owner.followers->follower : NULL;
      dismissed_and_reacquired =
          dismissed_and_reacquired && reclaimed != NULL && reclaimed->master == &owner &&
          IS_PET(reclaimed) && MOB_FLAGGED(reclaimed, MOB_EIDOLON) && GET_LEVEL(reclaimed) == 1 &&
          reclaimed->pet_data_id > 0 && reclaimed->pet_data_id != pet_id &&
          char_has_mud_event(&owner, eC_EIDOLON) != NULL &&
          keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 0") == 1;
      do_call(&owner, "eidolon", 0, 0);
      dismissed_and_reacquired = dismissed_and_reacquired && owner.followers != NULL &&
                                 owner.followers->follower == reclaimed &&
                                 owner.followers->next == NULL;
      snprintf(dismissal_error, sizeof(dismissal_error),
               "fresh eidolon: present=%d level=%d id=%ld old=%ld cooldown=%d: %.300s",
               reclaimed != NULL, reclaimed ? GET_LEVEL(reclaimed) : -1,
               reclaimed ? reclaimed->pet_data_id : 0, pet_id,
               char_has_mud_event(&owner, eC_EIDOLON) != NULL, descriptor.output);
    }
    if (reclaimed != NULL)
      extract_char(reclaimed);
    extract_pending_chars();
  }
  clear_char_event_list(&owner);
  if (owner.events != NULL)
  {
    free_list(owner.events);
    owner.events = NULL;
  }
  while (owner.affected != NULL)
    affect_remove_no_total(&owner, owner.affected);
  remove_all_perks(&owner);
  free(GET_TITLE(&owner));
  free(GET_IMM_TITLE(&owner));
  domain_event_world_forget_character(&owner);
  owner.desc = NULL;
  if (owns_event_bus)
  {
    domain_event_runtime_shutdown();
    event_free_all();
  }
  mob_proto = saved_prototypes;
  obj_index = saved_obj_index;
  obj_proto = saved_obj_proto;
  top_of_objt = saved_top_objt;
  character_list = saved_characters;
  if (created_command_list)
    free_command_list();
  end_gameplay_fixture(&fixture);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  free(GET_EIDOLON_SHORT_DESCRIPTION((&owner)));
  free(GET_EIDOLON_LONG_DESCRIPTION((&owner)));

  CuAssert(tc, dismissal_error, dismissed_and_reacquired);
  CuAssert(tc, respec_error, respec_preserved_storage);
  CuAssertTrue(tc, activation_failures_retained);
  CuAssertTrue(tc, publication_failure_retained);
  CuAssertIntEquals(tc, 1, trace.arrivals);
  CuAssertTrue(tc, trace.published_with_gear);
  CuAssertTrue(tc, named);
  CuAssertTrue(tc, name_denials);
  CuAssertTrue(tc, failed_name_retained);
  CuAssertTrue(tc, stable_failure_message);
  CuAssertTrue(tc, schema_created);
  CuAssertTrue(tc, stored);
  CuAssertTrue(tc, gear_left_with_pet);
  CuAssertIntEquals(tc, 1, stored_count);
  CuAssertTrue(tc, snapshot_kept_storage);
  CuAssertTrue(tc, reclaimed_identity);
  CuAssertTrue(tc, listed_id_matches);
  CuAssertTrue(tc, retry_kept_one_copy);
  CuAssertTrue(tc, repeat_denied);
  CuAssertTrue(tc, foreign_owner_denied);
}

void Test_pet_keeper_stores_and_reclaims_the_same_pet(CuTest *tc)
{
  verify_named_pet_keeper_round_trip(tc, false);
}

void Test_named_eidolon_keeper_restore_preserves_saved_identity(CuTest *tc)
{
  verify_named_pet_keeper_round_trip(tc, true);
}

/* Owner death ends following, but eligible pets stay owned and reclaimable. */
void Test_owner_death_stores_surviving_pets_within_capacity(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data prototype;
  struct char_data owner;
  struct player_special_data owner_specials = {0};
  struct char_data *first;
  struct char_data *second;
  struct char_data *overflow;
  struct char_data *saved_prototypes;
  struct char_data *saved_characters;
  const char *enabled;
  MYSQL *connection;
  MYSQL *saved_conn;
  bool saved_available;
  bool schema_created;
  bool overflow_retained;
  int stored;
  int stored_rows;
  int overflow_stored;
  int index;
  const char *filler_insert =
      "INSERT INTO pet_data (owner_name, pet_name, pet_sdesc, pet_ldesc, pet_ddesc, vnum, "
      "level, hp, max_hp, str, con, dex, ac, intel, wis, cha, owner_id, owner_created, "
      "pet_state) VALUES ('DyingOwner', 'filler', 'filler', 'filler', 'filler', 1, 1, 1, 1, "
      "1, 1, 1, 1, 1, 1, 1, 6001, 4321, 1)";

  enabled = getenv("LUMINARI_TEST_MYSQL_ENABLE");
  if (enabled == NULL || strcmp(enabled, "1") != 0)
  {
    CuAssertTrue(tc, 1);
    return;
  }
  connection = open_keeper_test_database();
  if (connection == NULL)
  {
    CuFail(tc, "could not connect to the explicitly configured test database");
    return;
  }

  saved_conn = conn;
  saved_available = mysql_available;
  saved_characters = character_list;
  conn = connection;
  mysql_available = true;
  schema_created = create_keeper_temporary_schema(connection);

  begin_gameplay_fixture(&fixture);
  saved_prototypes = mob_proto;
  initialize_test_npc(&prototype, "a surviving companion", NOWHERE);
  prototype.player.name = (char *)"companion";
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = 100;
  GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;

  clear_char(&owner);
  owner.player_specials = &owner_specials;
  owner.player.name = (char *)"DyingOwner";
  GET_LEVEL(&owner) = 20;
  GET_POS(&owner) = POS_STANDING;
  GET_IDNUM(&owner) = 6001;
  owner.player.time.birth = (time_t)4321;
  owner.pet_roster_load_state = PET_ROSTER_LOADED;
  char_to_room(&owner, 0);

  first = read_mobile(0, REAL);
  char_to_room(first, 0);
  add_follower(first, &owner);
  SET_BIT_AR(AFF_FLAGS(first), AFF_CHARM);
  second = read_mobile(0, REAL);
  char_to_room(second, 0);
  add_follower(second, &owner);
  SET_BIT_AR(AFF_FLAGS(second), AFF_CHARM);

  stored = schema_created ? pet_store_surviving_followers(&owner) : -1;
  extract_pending_chars();
  stored_rows = keeper_query_int(connection, "SELECT COUNT(*) FROM pet_data WHERE pet_state = 1");

  /* A full keeper releases the remaining pets exactly as before. */
  for (index = stored_rows; index >= 0 && index < PET_KEEPER_CAPACITY; index++)
  {
    if (mysql_query(connection, filler_insert))
      break;
  }
  overflow = read_mobile(0, REAL);
  char_to_room(overflow, 0);
  add_follower(overflow, &owner);
  SET_BIT_AR(AFF_FLAGS(overflow), AFF_CHARM);
  overflow_stored = pet_store_surviving_followers(&owner);
  overflow_retained = overflow_stored == 0 && !MOB_FLAGGED(overflow, MOB_NOTDEADYET);
  extract_char(overflow);
  extract_pending_chars();

  mob_proto = saved_prototypes;
  character_list = saved_characters;
  end_gameplay_fixture(&fixture);
  conn = saved_conn;
  mysql_available = saved_available;
  mysql_close(connection);

  CuAssertTrue(tc, schema_created);
  CuAssertIntEquals(tc, 2, stored);
  CuAssertIntEquals(tc, 2, stored_rows);
  CuAssertTrue(tc, overflow_retained);
}

/* The unseen servant is a utility conjuration: it handles items for its caster
 * without fighting, and it does nothing at all without the spell. */
void Test_unseen_servant_handles_items_only_while_conjured(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct affected_type servant_affect;
  struct obj_data *crate;
  struct obj_data *chest;
  bool denied_without_servant;
  bool fetched_past_item_count;
  bool denied_in_combat;
  bool stowed_in_container;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  fixture.actor.player.name = (char *)"servant caster";
  GET_LEVEL(&fixture.actor) = 10;

  crate = create_obj();
  GET_OBJ_RNUM(crate) = NOTHING;
  crate->name = strdup("crate");
  crate->short_description = strdup("a small crate");
  crate->description = strdup("A small crate rests here.");
  SET_BIT_AR(GET_OBJ_WEAR(crate), ITEM_WEAR_TAKE);
  GET_OBJ_WEIGHT(crate) = 5;
  obj_to_room(crate, 0);

  do_servant(&fixture.actor, "get crate", 0, 0);
  denied_without_servant = world[0].contents == crate;

  new_affect(&servant_affect);
  servant_affect.spell = SPELL_UNSEEN_SERVANT;
  servant_affect.location = APPLY_SPECIAL;
  servant_affect.modifier = 100;
  servant_affect.duration = 10;
  affect_to_char(&fixture.actor, &servant_affect);

  /* A full pair of hands does not stop the servant from fetching. */
  IS_CARRYING_N(&fixture.actor) = CAN_CARRY_N(&fixture.actor);
  do_servant(&fixture.actor, "get crate", 0, 0);
  fetched_past_item_count = fixture.actor.carrying == crate && world[0].contents == NULL;

  chest = create_obj();
  GET_OBJ_RNUM(chest) = NOTHING;
  chest->name = strdup("chest");
  chest->short_description = strdup("a stout chest");
  chest->description = strdup("A stout chest rests here.");
  GET_OBJ_TYPE(chest) = ITEM_CONTAINER;
  GET_OBJ_VAL(chest, 0) = 100;
  SET_BIT_AR(GET_OBJ_WEAR(chest), ITEM_WEAR_TAKE);
  obj_to_char(chest, &fixture.actor);

  FIGHTING(&fixture.actor) = &fixture.victim;
  do_servant(&fixture.actor, "put crate chest", 0, 0);
  denied_in_combat = chest->contains == NULL;
  FIGHTING(&fixture.actor) = NULL;

  do_servant(&fixture.actor, "put crate chest", 0, 0);
  stowed_in_container = chest->contains == crate;

  while (fixture.actor.carrying != NULL)
    extract_obj(fixture.actor.carrying);
  while (world[0].contents != NULL)
    extract_obj(world[0].contents);
  while (fixture.actor.affected != NULL)
    affect_remove(&fixture.actor, fixture.actor.affected);
  end_gameplay_fixture(&fixture);

  CuAssertTrue(tc, denied_without_servant);
  CuAssertTrue(tc, fetched_past_item_count);
  CuAssertTrue(tc, denied_in_combat);
  CuAssertTrue(tc, stowed_in_container);
}

/** Round-trip output choices, persist muted defaults, and retain choices on failed saves. */
void Test_gameplay_output_preferences_persist_and_failed_changes_roll_back(CuTest *tc)
{
  char temporary_directory[] = "/tmp/luminari-player-fixture-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct char_data *source = new_char();
  struct char_data *loaded = new_char();
  struct descriptor_data descriptor = {0};
  char directory[PATH_MAX], filename[MAX_FILEPATH], name[32];
  char failure_directory[] = "/tmp/luminari-output-save-XXXXXX";
  char on[] = "on", off[] = "off";
  int result, defaults_result, legacy_result;
  FILE *legacy_file;
  bool restored_reader, restored_sound, retained_map, failure_restored, legacy_defaults;
  bool defaults_muted, defaults_idempotent, defaults_saved;

  snprintf(name, sizeof(name), "Zzaccess%ld", (long)getpid());
  index[0].name = name;
  index[0].id = 4251;
  index[0].level = 7;
  player_table = index;
  top_of_p_table = 0;
  source->player.name = strdup(name);
  GET_PFILEPOS(source) = 0;
  GET_IDNUM(source) = 4251;
  GET_LEVEL(source) = 7;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  descriptor.character = source;
  STATE(&descriptor) = CON_PLAYING;
  source->desc = &descriptor;
  SET_BIT_AR(PRF_FLAGS(source), PRF_AUTOMAP);
  SET_BIT_AR(PRF_FLAGS(source), PRF_DISPHP);
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  enter_player_fixture(tc, temporary_directory);
  CuAssertTrue(tc, get_filename(filename, sizeof(filename), PLR_FILE, name));
  do_screenreader(source, on, 0, 0);
  do_sound(source, on, 0, 0);
  result = load_char(name, loaded);
  restored_reader = PRF_FLAGGED(loaded, PRF_SCREEN_READER);
  restored_sound = PRF_FLAGGED(loaded, PRF_SOUND);
  retained_map = PRF_FLAGGED(loaded, PRF_AUTOMAP) && PRF_FLAGGED(loaded, PRF_DISPHP);
  free_char(loaded);
  loaded = new_char();

  /* Restoring defaults revokes consent, even with MSP already negotiated. */
  descriptor.pProtocol->bMSP = true;
  do_oasis_prefedit(source, "", 0, 0);
  prefedit_parse(&descriptor, "d");
  defaults_muted = !IS_SET_AR(OLC_PREFS(&descriptor)->pref_flags, PRF_SOUND);
  prefedit_parse(&descriptor, "d");
  defaults_idempotent = !IS_SET_AR(OLC_PREFS(&descriptor)->pref_flags, PRF_SOUND);
  prefedit_parse(&descriptor, "q");
  prefedit_parse(&descriptor, "y");
  defaults_result = load_char(name, loaded);
  defaults_saved = descriptor.olc == NULL && !PRF_FLAGGED(loaded, PRF_SOUND) &&
                   PRF_FLAGGED(loaded, PRF_SCREEN_READER) && descriptor.pProtocol->bMSP &&
                   !SoundEnabled(&descriptor) &&
                   !strcmp(ProtocolOutput(&descriptor, "\t!SOUND(luminari-test.wav)", NULL), "");
  free_char(loaded);
  loaded = new_char();

  legacy_file = fopen(filename, "w");
  CuAssertPtrNotNull(tc, legacy_file);
  fprintf(legacy_file, "Name: %s\nId  : 4251\nLevl: 7\n", name);
  fclose(legacy_file);
  legacy_result = load_char(name, loaded);
  legacy_defaults = !PRF_FLAGGED(loaded, PRF_SCREEN_READER) && !PRF_FLAGGED(loaded, PRF_SOUND);
  unlink(filename);
  CuAssertIntEquals(tc, 0, leave_player_fixture(directory, temporary_directory));

  /* Exercise the actual checked-save failure without touching another player. */
  SET_BIT_AR(PRF_FLAGS(source), PRF_SOUND);
  CuAssertPtrNotNull(tc, mkdtemp(failure_directory));
  CuAssertIntEquals(tc, 0, chdir(failure_directory));
  do_screenreader(source, off, 0, 0);
  do_sound(source, off, 0, 0);
  failure_restored = PRF_FLAGGED(source, PRF_SCREEN_READER) && PRF_FLAGGED(source, PRF_SOUND) &&
                     strstr(descriptor.output, "previous setting remains") != NULL;
  CuAssertIntEquals(tc, 0, chdir(directory));
  rmdir(failure_directory);
  source->desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  free_char(source);
  free_char(loaded);
  player_table = saved_table;
  top_of_p_table = saved_top;
  CuAssertIntEquals(tc, 0, result);
  CuAssertTrue(tc, restored_reader);
  CuAssertTrue(tc, restored_sound);
  CuAssertTrue(tc, retained_map);
  CuAssertTrue(tc, defaults_muted);
  CuAssertTrue(tc, defaults_idempotent);
  CuAssertIntEquals(tc, 0, defaults_result);
  CuAssertTrue(tc, defaults_saved);
  CuAssertTrue(tc, failure_restored);
  CuAssertIntEquals(tc, 0, legacy_result);
  CuAssertTrue(tc, legacy_defaults);
}

/* Both full-prompt aliases restore the fields cleared by their off aliases. */
void Test_gameplay_prompt_all_restores_gold_and_time_after_none(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  const char *enable[] = {"all", "on"};
  const char *disable[] = {"none", "off"};
  bool restored = true, cleared = true;
  size_t i;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  for (i = 0; i < sizeof(enable) / sizeof(enable[0]); i++)
  {
    do_display(&fixture.actor, disable[i], 0, 0);
    cleared = cleared && is_prompt_empty(&fixture.actor) &&
              !PRF_FLAGGED(&fixture.actor, PRF_DISPGOLD) &&
              !PRF_FLAGGED(&fixture.actor, PRF_DISPTIME);
    do_display(&fixture.actor, enable[i], 0, 0);
    restored = restored && !is_prompt_empty(&fixture.actor) &&
               PRF_FLAGGED(&fixture.actor, PRF_DISPGOLD) &&
               PRF_FLAGGED(&fixture.actor, PRF_DISPTIME);
  }
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, cleared);
  CuAssertTrue(tc, restored);
}

/** Suppress gameplay prompts while preserving telnet delimiters and pager/editor instructions. */
void Test_gameplay_screen_reader_hides_actual_prompts_but_keeps_input_instructions(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct descriptor_data descriptor = {0};
  struct player_special_data specials = {0};
  char *editor_text = NULL;
  const char telnet_go_ahead[] = {(char)255, (char)249, '\0'};
  const char *prompt;
  bool normal_visible, idle_hidden, combat_hidden, pager_visible, editor_visible;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  fixture.actor.player.name = (char *)"Accessibility fixture";
  fixture.actor.desc = &descriptor;
  descriptor.character = &fixture.actor;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_DISPHP);
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AFK);
  prompt = comm_make_prompt_for_test(&descriptor);
  normal_visible = strstr(prompt, "100") != NULL && strstr(prompt, "AFK") != NULL;
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_SCREEN_READER);
  idle_hidden = !strcmp(comm_make_prompt_for_test(&descriptor), telnet_go_ahead);
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  combat_hidden = !strcmp(comm_make_prompt_for_test(&descriptor), telnet_go_ahead);
  descriptor.showstr_count = 2;
  pager_visible = strstr(comm_make_prompt_for_test(&descriptor), "Return to continue") != NULL;
  descriptor.showstr_count = 0;
  descriptor.str = &editor_text;
  editor_visible = strstr(comm_make_prompt_for_test(&descriptor), "]") != NULL;
  descriptor.str = NULL;
  fixture.actor.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, normal_visible);
  CuAssertTrue(tc, idle_hidden);
  CuAssertTrue(tc, combat_hidden);
  CuAssertTrue(tc, pager_visible);
  CuAssertTrue(tc, editor_visible);
}

/** Compare reader-mode room output with mapless output using the real room renderer. */
static void verify_screen_reader_room_text(CuTest *tc, bool wilderness)
{
  struct gameplay_fixture fixture;
  struct descriptor_data descriptor = {0};
  struct player_special_data specials = {0};
  char *without_map;
  bool same_text, useful_text;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  fixture.actor.player.name = (char *)"Accessibility fixture";
  fixture.actor.desc = &descriptor;
  fixture.rooms[0].light = 1;
  fixture.rooms[0].people = NULL;
  if (wilderness)
  {
    SET_BIT_AR(ZONE_FLAGS(0), ZONE_WILDERNESS);
    fixture.rooms[0].sector_type = SECT_FIELD;
  }
  descriptor.character = &fixture.actor;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;
  look_at_room(&fixture.actor, TRUE);
  without_map = strdup(descriptor.output);
  useful_text = wilderness ? strlen(without_map) > strlen(fixture.rooms[0].name) + 20
                           : strstr(without_map, "A production-linked test room.") != NULL;
  descriptor.output[0] = '\0';
  descriptor.bufptr = 0;
  descriptor.bufspace = descriptor.large_outbuf ? LARGE_BUFSIZE - 1 : SMALL_BUFSIZE - 1;
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AUTOMAP);
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_SCREEN_READER);
  look_at_room(&fixture.actor, TRUE);
  same_text = !strcmp(without_map, descriptor.output);
  free(without_map);
  fixture.actor.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, useful_text);
  CuAssertTrue(tc, same_text);
}

/** Keep ordinary room descriptions identical to explicit mapless output in reader mode. */
void Test_gameplay_screen_reader_room_output_matches_mapless_description(CuTest *tc)
{
  verify_screen_reader_room_text(tc, false);
}

/** Keep wilderness descriptions identical to explicit mapless output in reader mode. */
void Test_gameplay_screen_reader_wilderness_output_matches_mapless_description(CuTest *tc)
{
  verify_screen_reader_room_text(tc, true);
}

/** Edit consent independently of MSP and restore live preferences after a failed save. */
void Test_gameplay_prefedit_sound_keeps_capability_and_rolls_back_failed_save(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct descriptor_data descriptor = {0};
  struct player_special_data specials = {0};
  char directory[PATH_MAX];
  char failure_directory[] = "/tmp/luminari-prefedit-save-XXXXXX";
  bool copied, capability_retained, rolled_back, no_success;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player_specials = &specials;
  fixture.actor.player.name = (char *)"Accessibility fixture";
  fixture.actor.desc = &descriptor;
  descriptor.character = &fixture.actor;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.pProtocol = ProtocolCreate();
  STATE(&descriptor) = CON_PLAYING;
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_SCREEN_READER);
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_SOUND);
  do_oasis_prefedit(&fixture.actor, "", 0, 0);
  copied = IS_SET_AR(OLC_PREFS(&descriptor)->pref_flags, PRF_SCREEN_READER) &&
           IS_SET_AR(OLC_PREFS(&descriptor)->pref_flags, PRF_SOUND);
  OLC_MODE(&descriptor) = PREFEDIT_TOGGLE_MENU;
  prefedit_parse(&descriptor, "r");
  capability_retained =
      !descriptor.pProtocol->bMSP && !IS_SET_AR(OLC_PREFS(&descriptor)->pref_flags, PRF_SOUND);
  OLC_MODE(&descriptor) = PREFEDIT_CONFIRM_SAVE;
  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  CuAssertPtrNotNull(tc, mkdtemp(failure_directory));
  CuAssertIntEquals(tc, 0, chdir(failure_directory));
  prefedit_parse(&descriptor, "y");
  CuAssertIntEquals(tc, 0, chdir(directory));
  rmdir(failure_directory);
  rolled_back = PRF_FLAGGED(&fixture.actor, PRF_SCREEN_READER) &&
                PRF_FLAGGED(&fixture.actor, PRF_SOUND) && descriptor.olc != NULL &&
                OLC_MODE(&descriptor) == PREFEDIT_CONFIRM_SAVE;
  no_success = strstr(descriptor.output, "Preferences saved.") == NULL;
  prefedit_parse(&descriptor, "n");
  fixture.actor.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  if (descriptor.large_outbuf != NULL)
  {
    free(descriptor.large_outbuf->text);
    free(descriptor.large_outbuf);
  }
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, copied);
  CuAssertTrue(tc, capability_retained);
  CuAssertTrue(tc, rolled_back);
  CuAssertTrue(tc, no_success);
}

static void verify_artifact_pet_acquisition(CuTest *tc, int mode, int artifact)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct obj_data sword = {0};
  struct index_data sword_index = {0};
  struct index_data *saved_obj_index = obj_index;
  struct follow_type existing = {0};
  bool created_commands, handled, created, repeat_denied;
  int owner_hp, cooldown;
  mob_vnum pet_vnum = artifact == 0 ? PET_XVIM_NIGHTMARE : artifact == 1 ? 101225 : 132131;
  obj_vnum sword_vnum = artifact == 0 ? 100501 : artifact == 1 ? 109802 : 132118;
  const char *word = artifact == 0 ? "nightmare" : "wind";
  SPECIAL_DECL(*summon) = artifact == 0   ? xvim_artifact
                          : artifact == 1 ? whisperwind
                                          : ancient_moonblade;

  begin_gameplay_fixture(&fixture);
  created_commands = complete_cmd_info == NULL;
  if (created_commands)
    create_command_list();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"nightmareowner";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_LEVEL(&fixture.actor) = 10;
  GET_CHA(&fixture.actor) = 10;
  GET_HIT(&fixture.actor) = 17;
  sword_index.vnum = sword_vnum;
  obj_index = &sword_index;
  GET_OBJ_RNUM(&sword) = 0;
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &sword;
  initialize_test_npc(&prototype, "nightmare", NOWHERE);
  prototype.player.name = (char *)"nightmare";
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  fixture.mobile_index[0].vnum = mode == 1 ? 1 : pet_vnum;
  if (mode == 2)
  {
    existing.follower = &fixture.victim;
    fixture.actor.followers = &existing;
    fixture.victim.master = &fixture.actor;
    GET_MOB_RNUM(&fixture.victim) = 0;
    SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  }
  handled = summon(&fixture.actor, &sword, find_command("whisper"), word);
  owner_hp = GET_HIT(&fixture.actor);
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  if (pet == &fixture.victim)
    pet = NULL;
  created = pet != NULL && IS_PET(pet) && IN_ROOM(pet) == 0 && GET_HIT(pet) == GET_MAX_HIT(pet) &&
            (artifact == 0 ? GET_HIT(pet) >= 110 && GET_HIT(pet) <= 160
                           : GET_HIT(pet) == GET_MAX_HIT(&fixture.actor));
  cooldown = GET_OBJ_SPECTIMER(&sword, 1);
  summon(&fixture.actor, &sword, find_command("whisper"), word);
  repeat_denied = fixture.mobile_index[0].number == (mode == 0 ? 1 : 0);
  if (pet != NULL)
    extract_char(pet);
  extract_pending_chars();
  fixture.actor.followers = NULL;
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  character_list = saved_characters;
  mob_proto = saved_proto;
  point_update_object_forget(&sword);
  obj_index = saved_obj_index;
  if (created_commands)
    free_command_list();
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, handled);
  CuAssertIntEquals(tc, 17, owner_hp);
  CuAssertIntEquals(tc, mode == 0, created);
  CuAssertTrue(tc, repeat_denied);
  CuAssertIntEquals(tc, mode == 0 && artifact != 0 ? 72 : 0, cooldown);
}

void Test_gameplay_xvim_nightmare_preserves_owner_hp_and_enforces_admission(CuTest *tc)
{
  verify_artifact_pet_acquisition(tc, 0, 0);
  verify_artifact_pet_acquisition(tc, 1, 0);
  verify_artifact_pet_acquisition(tc, 2, 0);
}

void Test_gameplay_moonblade_pets_commit_cooldown_only_after_admission(CuTest *tc)
{
  int artifact, mode;

  for (artifact = 1; artifact <= 2; artifact++)
    for (mode = 0; mode <= 2; mode++)
      verify_artifact_pet_acquisition(tc, mode, artifact);
}

static void interrupt_quest_follower_arrival(const struct domain_event_context *context, void *data)
{
  const struct domain_character_moved *event = context->payload;
  struct char_data *pet = domain_event_world_resolve_character(event->character);
  bool *interrupted = data;

  if (pet != NULL && GET_MOB_VNUM(pet) == RETAINER_MOB_VNUM)
  {
    *interrupted = true;
    extract_char(pet);
  }
}

static void verify_quest_pet_reward_admission(CuTest *tc, int mode)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct aq_data quest = {0}, *saved_quests = aquest_table;
  qst_rnum saved_count = total_quests;
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct follow_type existing = {0};
  struct descriptor_data descriptor = {0};
  struct domain_event_subscription_config observer = {0};
  struct domain_event_subscription_handle subscription;
  bool retained, completed, duplicate_denied, waited_message;
  bool interrupted = false, subscription_active = false;
  bool owns_event_bus = false;
  int gold, points, saved_move_gain;
  int class_num = mode == 5 ? CLASS_WARRIOR : CLASS_WIZARD;

  begin_gameplay_fixture(&fixture);
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"questpetowner";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_CHA(&fixture.actor) = 10;
  GET_LEVEL(&fixture.actor) = 30;
  GET_EXP(&fixture.actor) = 1;
  saved_move_gain = class_list[class_num].move_gain;
  class_list[class_num].move_gain = 1;
  GET_GOLD(&fixture.actor) = 0;
  aquest_table = &quest;
  total_quests = 1;
  quest.vnum = 700;
  quest.done = (char *)"Your service is complete.";
  quest.follower_reward = RETAINER_MOB_VNUM;
  quest.obj_reward = NOTHING;
  quest.race_reward = mode == 4 ? RACE_LICH : mode == 5 ? RACE_VAMPIRE : RACE_UNDEFINED;
  quest.next_quest = NOTHING;
  quest.gold_reward = 100;
  quest.value[0] = 10;
  GET_QUEST(&fixture.actor, 0) = quest.vnum;
  GET_QUEST_COUNTER(&fixture.actor, 0) = 0;
  initialize_test_npc(&prototype, "quest follower", NOWHERE);
  prototype.player.name = (char *)"follower";
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = 100;
  GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  fixture.mobile_index[0].vnum = mode == 2 ? 1 : RETAINER_MOB_VNUM;
  if (mode == 1)
  {
    existing.follower = &fixture.victim;
    fixture.actor.followers = &existing;
    fixture.victim.master = &fixture.actor;
    SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  }
  if (mode == 3)
    IN_ROOM(&fixture.actor) = NOWHERE;
  if (mode == 6)
  {
    descriptor.output = descriptor.small_outbuf;
    descriptor.bufspace = SMALL_BUFSIZE - 1;
    descriptor.character = &fixture.actor;
    descriptor.pProtocol = ProtocolCreate();
    fixture.actor.desc = &descriptor;
    if (domain_event_runtime_bus() == NULL)
    {
      event_free_all();
      event_init();
      owns_event_bus = true;
      CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
    }
    observer.type = DOMAIN_EVENT_CHARACTER_MOVED;
    observer.topic.role = DOMAIN_EVENT_TOPIC_DESTINATION;
    observer.topic.entity = domain_event_room_handle(0);
    observer.owner = domain_event_character_handle(&fixture.actor);
    observer.identity = "test.quest-follower.interrupted-arrival";
    observer.handler = interrupt_quest_follower_arrival;
    observer.handler_context = &interrupted;
    subscription_active = domain_event_subscribe(domain_event_runtime_bus(), &observer,
                                                 &subscription) == DOMAIN_EVENT_OK;
    CuAssertTrue(tc, subscription_active);
  }

  complete_quest(&fixture.actor, 0);
  if (mode == 6)
    extract_pending_chars();
  waited_message =
      mode != 6 || (interrupted && strstr(descriptor.output, "quest reward must wait") != NULL);
  retained = mode == 0 || (mode >= 4 && mode <= 5) ||
             (GET_QUEST(&fixture.actor, 0) == (int)quest.vnum &&
              GET_NUM_QUESTS(&fixture.actor) == 0 && GET_GOLD(&fixture.actor) == 0 &&
              GET_QUESTPOINTS(&fixture.actor) == 0 && fixture.mobile_index[0].number == 0);
  if (subscription_active)
  {
    CuAssertIntEquals(tc, DOMAIN_EVENT_OK,
                      domain_event_unsubscribe(domain_event_runtime_bus(), subscription));
    subscription_active = false;
  }
  if ((mode > 0 && mode < 4) || mode == 6)
  {
    fixture.actor.followers = NULL;
    fixture.victim.master = NULL;
    fixture.mobile_index[0].vnum = RETAINER_MOB_VNUM;
    IN_ROOM(&fixture.actor) = 0;
    complete_quest(&fixture.actor, 0);
  }
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  completed = pet != NULL && IS_PET(pet) && IN_ROOM(pet) == 0 &&
              GET_QUEST(&fixture.actor, 0) == (int)NOTHING &&
              is_complete(&fixture.actor, quest.vnum) && GET_NUM_QUESTS(&fixture.actor) == 1;
  if (mode >= 4 && mode <= 5)
    completed = completed && GET_REAL_RACE(&fixture.actor) == quest.race_reward &&
                CLASS_LEVEL((&fixture.actor), class_num) == 1;
  gold = GET_GOLD(&fixture.actor);
  points = GET_QUESTPOINTS(&fixture.actor);
  complete_quest(&fixture.actor, 0);
  duplicate_denied = fixture.mobile_index[0].number == 1 && GET_GOLD(&fixture.actor) == gold &&
                     GET_QUESTPOINTS(&fixture.actor) == points;
  if (pet != NULL)
    extract_char(pet);
  extract_pending_chars();
  fixture.actor.followers = NULL;
  free(specials.saved.completed_quests);
  fixture.actor.desc = NULL;
  if (descriptor.pProtocol)
    ProtocolDestroy(descriptor.pProtocol);
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  if (owns_event_bus)
  {
    domain_event_runtime_shutdown();
    event_free_all();
  }
  character_list = saved_characters;
  mob_proto = saved_proto;
  aquest_table = saved_quests;
  total_quests = saved_count;
  end_gameplay_fixture(&fixture);
  class_list[class_num].move_gain = saved_move_gain;
  if (mode >= 4 && mode <= 5)
    free(GET_TITLE(&fixture.actor));
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, waited_message);
  CuAssertTrue(tc, completed);
  CuAssertTrue(tc, gold >= 100 && points >= 10);
  CuAssertTrue(tc, duplicate_denied);
}

void Test_gameplay_quest_pet_rewards_wait_for_admission_and_retry_once(CuTest *tc)
{
  int mode;

  for (mode = 0; mode <= 6; mode++)
    verify_quest_pet_reward_admission(tc, mode);
}

void Test_gameplay_incorporeal_pets_reject_physical_objects(CuTest *tc)
{
  struct gameplay_fixture f;
  struct obj_data *item, *bag;
  bool denied_give, denied_get, denied_container, denied_wear, physical_give;
  char get_item[] = "parcel", get_bag_item[] = "parcel bag";

  begin_gameplay_fixture(&f);
  f.victim.master = &f.actor;
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_IMMATERIAL);
  item = create_obj();
  item->name = strdup("parcel");
  item->short_description = strdup("a parcel");
  GET_OBJ_TYPE(item) = ITEM_ARMOR;
  SET_BIT_AR(GET_OBJ_WEAR(item), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(item), ITEM_WEAR_HEAD);
  obj_to_char(item, &f.actor);
  denied_give = !perform_give(&f.actor, &f.victim, item) && item->carried_by == &f.actor;
  obj_from_char(item);
  obj_to_room(item, 0);
  do_get(&f.victim, get_item, 0, 0);
  denied_get = item->in_room == 0;
  obj_from_room(item);
  bag = create_obj();
  bag->name = strdup("bag");
  bag->short_description = strdup("a bag");
  GET_OBJ_TYPE(bag) = ITEM_CONTAINER;
  obj_to_char(bag, &f.victim);
  obj_to_obj(item, bag);
  do_get(&f.victim, get_bag_item, 0, 0);
  denied_container = item->in_obj == bag;
  obj_from_obj(item);
  obj_to_char(item, &f.victim);
  perform_wear(&f.victim, item, WEAR_HEAD);
  denied_wear = item->carried_by == &f.victim && GET_EQ(&f.victim, WEAR_HEAD) == NULL;
  obj_from_char(item);
  obj_to_char(item, &f.actor);
  REMOVE_BIT_AR(AFF_FLAGS(&f.victim), AFF_IMMATERIAL);
  physical_give = perform_give(&f.actor, &f.victim, item) && item->carried_by == &f.victim;
  extract_obj(item);
  extract_obj(bag);
  f.victim.master = NULL;
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(&f.victim);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, denied_give);
  CuAssertTrue(tc, denied_get);
  CuAssertTrue(tc, denied_container);
  CuAssertTrue(tc, denied_wear);
  CuAssertTrue(tc, physical_give);
}

void Test_gameplay_dragon_rider_recognizes_only_its_controlled_bonded_mount(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data other;
  bool bonded, unrelated, separated, unlinked, uncontrolled, wrong_owner, pending;
  int rider_base, mount_base, rider_bonus, mount_bonus;

  begin_gameplay_fixture(&f);
  initialize_test_npc(&other, "other owner", 0);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  GET_MOB_RNUM(&f.victim) = 0;
  SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_C_DRAGON);
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  f.victim.master = &f.actor;
  mount_char(&f.actor, &f.victim);
  bonded = is_dragon_rider_mount(&f.victim) && is_riding_dragon_mount(&f.actor);
  GET_DAMROLL(&f.actor) = GET_DAMROLL(&f.victim) = 10;
  rider_base = compute_damage_bonus(&f.actor, NULL, NULL, TYPE_HIT, 0, 0, ATTACK_TYPE_PRIMARY);
  mount_base = compute_damage_bonus(&f.victim, NULL, NULL, TYPE_HIT, 0, 0, ATTACK_TYPE_PRIMARY);
  SET_FEAT(&f.actor, FEAT_UNITED_WE_STAND, 1);
  rider_bonus = compute_damage_bonus(&f.actor, NULL, NULL, TYPE_HIT, 0, 0, ATTACK_TYPE_PRIMARY);
  mount_bonus = compute_damage_bonus(&f.victim, NULL, NULL, TYPE_HIT, 0, 0, ATTACK_TYPE_PRIMARY);

  REMOVE_BIT_AR(MOB_FLAGS(&f.victim), MOB_C_DRAGON);
  unrelated = !is_dragon_rider_mount(&f.victim) && !is_riding_dragon_mount(&f.actor);
  SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_C_DRAGON);
  IN_ROOM(&f.victim) = 1;
  separated = !is_riding_dragon_mount(&f.actor);
  IN_ROOM(&f.victim) = 0;
  RIDDEN_BY(&f.victim) = NULL;
  unlinked = !is_riding_dragon_mount(&f.actor);
  RIDDEN_BY(&f.victim) = &f.actor;
  REMOVE_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  uncontrolled = !is_riding_dragon_mount(&f.actor);
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  f.victim.master = &other;
  wrong_owner = !is_riding_dragon_mount(&f.actor);
  f.victim.master = &f.actor;
  SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_NOTDEADYET);
  pending = !is_riding_dragon_mount(&f.actor);
  REMOVE_BIT_AR(MOB_FLAGS(&f.victim), MOB_NOTDEADYET);
  dismount_char(&f.actor);
  f.victim.master = NULL;
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, bonded);
  CuAssertIntEquals(tc, 4, rider_bonus - rider_base);
  CuAssertIntEquals(tc, 4, mount_bonus - mount_base);
  CuAssertTrue(tc, unrelated);
  CuAssertTrue(tc, separated);
  CuAssertTrue(tc, unlinked);
  CuAssertTrue(tc, uncontrolled);
  CuAssertTrue(tc, wrong_owner);
  CuAssertTrue(tc, pending);
}

static void verify_mounted_charge_damage(CuTest *tc, bool lance, bool spirited)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data mount;
  struct obj_data *weapon = NULL;
  int ordinary, mounted, on_foot;
  bool stale_cleared;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  initialize_test_npc(&mount, "charge mount", 0);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.victim.player.name = "victim";
  GET_REAL_DAMROLL(&f.actor) = GET_DAMROLL(&f.actor) = 20;
  SET_FEAT(&f.actor, FEAT_SPIRITED_CHARGE, spirited);
  if (lance)
  {
    if (weapon_list[WEAPON_TYPE_LANCE].name == NULL)
      load_weapons();
    weapon = create_obj();
    GET_OBJ_TYPE(weapon) = ITEM_WEAPON;
    GET_OBJ_VAL(weapon, 0) = WEAPON_TYPE_LANCE;
    equip_char(&f.actor, weapon, WEAR_WIELD_1);
  }
  FIGHTING(&f.actor) = &f.victim;
  mount_char(&f.actor, &mount);
  circle_srandom(421);
  ordinary = compute_hit_damage(&f.actor, &f.victim, TYPE_HIT, 10, MODE_NORMAL_HIT, false,
                                ATTACK_TYPE_PRIMARY, DAM_BLUDGEON);
  do_charge(&f.actor, "victim", 0, 0);
  circle_srandom(421);
  mounted = compute_hit_damage(&f.actor, &f.victim, TYPE_HIT, 10, MODE_NORMAL_HIT, false,
                               ATTACK_TYPE_PRIMARY, DAM_BLUDGEON);
  dismount_char(&f.actor);
  circle_srandom(421);
  on_foot = compute_hit_damage(&f.actor, &f.victim, TYPE_HIT, 10, MODE_NORMAL_HIT, false,
                               ATTACK_TYPE_PRIMARY, DAM_BLUDGEON);
  mount_char(&f.actor, &mount);
  IN_ROOM(&mount) = 1;
  affect_from_char(&f.actor, SKILL_CHARGE);
  do_charge(&f.actor, "victim", 0, 0);
  stale_cleared = RIDING(&f.actor) == NULL && RIDDEN_BY(&mount) == NULL;
  if (weapon != NULL)
    extract_obj(weapon);
  clear_char_event_list(&f.actor);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(&f.victim);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, ordinary > 0);
  CuAssertIntEquals(tc, ordinary * (lance && spirited ? 3 : 2), mounted);
  CuAssertIntEquals(tc, ordinary, on_foot);
  CuAssertTrue(tc, stale_cleared);
}

void Test_gameplay_spirited_charge_requires_a_current_mount(CuTest *tc)
{
  verify_mounted_charge_damage(tc, false, true);
  verify_mounted_charge_damage(tc, true, false);
  verify_mounted_charge_damage(tc, true, true);
}

void Test_gameplay_mounted_combat_negates_one_hit_only_while_riding(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data mount;
  bool blocked, spent, dismounted;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  initialize_test_npc(&mount, "combat mount", 0);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  GET_HIT(&f.actor) = GET_MAX_HIT(&f.actor) = GET_REAL_MAX_HIT(&f.actor) = 100000;
  GET_HITROLL(&f.victim) = 20;
  GET_DAMROLL(&f.victim) = 20;
  FIGHTING(&f.actor) = &f.victim;
  FIGHTING(&f.victim) = &f.actor;
  SET_ABILITY(&f.actor, ABILITY_RIDE, 100);
  SET_FEAT(&f.actor, FEAT_MOUNTED_COMBAT, 1);
  mount_char(&f.actor, &mount);
  MOUNTED_BLOCKS_LEFT(&f.actor) = 1;
  circle_srandom(421);
  combat_readied_attack(&f.victim, &f.actor);
  blocked = GET_HIT(&f.actor) == 100000 && MOUNTED_BLOCKS_LEFT(&f.actor) == 0;
  circle_srandom(421);
  combat_readied_attack(&f.victim, &f.actor);
  spent = GET_HIT(&f.actor) < 100000 && MOUNTED_BLOCKS_LEFT(&f.actor) == 0;
  GET_HIT(&f.actor) = 100000;
  MOUNTED_BLOCKS_LEFT(&f.actor) = 1;
  dismount_char(&f.actor);
  circle_srandom(421);
  combat_readied_attack(&f.victim, &f.actor);
  dismounted = GET_HIT(&f.actor) < 100000 && MOUNTED_BLOCKS_LEFT(&f.actor) == 1;
  clear_char_event_list(&f.actor);
  clear_char_event_list(&f.victim);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(&f.victim);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, blocked);
  CuAssertTrue(tc, spent);
  CuAssertTrue(tc, dismounted);
}

void Test_gameplay_mounted_combat_blocks_reset_without_accumulating(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data mount;
  int ordinary, legendary, repeated, missing_base, repeated_missing_base, separated;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  initialize_test_npc(&mount, "combat mount", 0);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  GET_ATTACK_QUEUE(&f.actor) = create_attack_queue();
  GET_HIT(&f.actor) = GET_MAX_HIT(&f.actor) = 100000;
  GET_HIT(&f.victim) = GET_MAX_HIT(&f.victim) = 100000;
  FIGHTING(&f.actor) = &f.victim;
  mount_char(&f.actor, &mount);
  SET_FEAT(&f.actor, FEAT_MOUNTED_COMBAT, 1);
  MOUNTED_BLOCKS_LEFT(&f.actor) = 9;
  perform_violence(&f.actor, 1);
  ordinary = MOUNTED_BLOCKS_LEFT(&f.actor);
  SET_FEAT(&f.actor, FEAT_LEGENDARY_RIDER, 1);
  perform_violence(&f.actor, 1);
  legendary = MOUNTED_BLOCKS_LEFT(&f.actor);
  perform_violence(&f.actor, 1);
  repeated = MOUNTED_BLOCKS_LEFT(&f.actor);
  SET_FEAT(&f.actor, FEAT_MOUNTED_COMBAT, 0);
  perform_violence(&f.actor, 1);
  missing_base = MOUNTED_BLOCKS_LEFT(&f.actor);
  perform_violence(&f.actor, 1);
  repeated_missing_base = MOUNTED_BLOCKS_LEFT(&f.actor);
  IN_ROOM(&mount) = 1;
  perform_violence(&f.actor, 1);
  separated = MOUNTED_BLOCKS_LEFT(&f.actor);
  dismount_char(&f.actor);
  free_attack_queue(GET_ATTACK_QUEUE(&f.actor));
  GET_ATTACK_QUEUE(&f.actor) = NULL;
  clear_char_event_list(&f.actor);
  clear_char_event_list(&f.victim);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(&f.victim);
  end_gameplay_fixture(&f);
  CuAssertIntEquals(tc, 1, ordinary);
  CuAssertIntEquals(tc, 2, legendary);
  CuAssertIntEquals(tc, 2, repeated);
  CuAssertIntEquals(tc, 1, missing_base);
  CuAssertIntEquals(tc, 1, repeated_missing_base);
  CuAssertIntEquals(tc, 0, separated);
}

void Test_gameplay_mounted_travel_and_recall_clear_only_separated_riding(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  room_rnum saved_start = r_mortal_start_room;
  bool traveled, returned, recalled, retained_owner, stale_cleared;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  SET_FEAT(&f.actor, FEAT_MOUNTED_COMBAT, 1);
  mount_char(&f.actor, &f.victim);
  traveled = perform_move(&f.actor, NORTH, false) == 1 && IN_ROOM(&f.actor) == 1 &&
             IN_ROOM(&f.victim) == 1 && RIDING(&f.actor) == &f.victim &&
             RIDDEN_BY(&f.victim) == &f.actor;
  returned = perform_move(&f.actor, SOUTH, false) == 1 && IN_ROOM(&f.actor) == 0 &&
             IN_ROOM(&f.victim) == 0 && RIDING(&f.actor) == &f.victim;
  f.victim.master = &f.actor;
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
  r_mortal_start_room = 1;
  spell_recall(10, &f.actor, &f.actor, NULL, CAST_SPELL);
  recalled = IN_ROOM(&f.actor) == 1 && IN_ROOM(&f.victim) == 0 && RIDING(&f.actor) == NULL &&
             RIDDEN_BY(&f.victim) == NULL;
  retained_owner = f.victim.master == &f.actor && AFF_FLAGGED(&f.victim, AFF_CHARM);
  /* A stale link from any other separation must not affect the next movement. */
  mount_char(&f.actor, &f.victim);
  stale_cleared = perform_move(&f.actor, SOUTH, false) == 1 && RIDING(&f.actor) == NULL &&
                  RIDDEN_BY(&f.victim) == NULL;
  r_mortal_start_room = saved_start;
  f.victim.master = NULL;
  dismount_char(&f.actor);
  clear_char_event_list(&f.actor);
  clear_char_event_list(&f.victim);
  event_free_all();
  domain_event_world_forget_character(&f.actor);
  domain_event_world_forget_character(&f.victim);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, traveled);
  CuAssertTrue(tc, returned);
  CuAssertTrue(tc, recalled);
  CuAssertTrue(tc, retained_owner);
  CuAssertTrue(tc, stale_cleared);
}

void Test_gameplay_mount_cleanup_preserves_unrelated_reciprocal_links(CuTest *tc)
{
  struct gameplay_fixture f;
  struct char_data other;
  bool valid, separated, stale_rider, stale_mount;

  begin_gameplay_fixture(&f);
  initialize_test_npc(&other, "other rider", 0);
  mount_char(&f.actor, &f.victim);
  mount_cleanup(&f.actor);
  valid = RIDING(&f.actor) == &f.victim && RIDDEN_BY(&f.victim) == &f.actor &&
          f.actor.master == NULL && f.victim.master == NULL;
  IN_ROOM(&f.victim) = 1;
  mount_cleanup(&f.actor);
  separated = RIDING(&f.actor) == NULL && RIDDEN_BY(&f.victim) == NULL;
  IN_ROOM(&f.victim) = 0;
  mount_char(&other, &f.victim);
  RIDING(&f.actor) = &f.victim;
  mount_cleanup(&f.actor);
  stale_rider =
      RIDING(&f.actor) == NULL && RIDING(&other) == &f.victim && RIDDEN_BY(&f.victim) == &other;
  dismount_char(&other);
  mount_char(&f.actor, &other);
  RIDDEN_BY(&f.victim) = &f.actor;
  mount_cleanup(&f.victim);
  stale_mount =
      RIDDEN_BY(&f.victim) == NULL && RIDING(&f.actor) == &other && RIDDEN_BY(&other) == &f.actor;
  dismount_char(&f.actor);
  dismount_char(NULL);
  mount_cleanup(NULL);
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, valid);
  CuAssertTrue(tc, separated);
  CuAssertTrue(tc, stale_rider);
  CuAssertTrue(tc, stale_mount);
}

void Test_gameplay_mount_restrictions_and_riding_do_not_grant_ownership(CuTest *tc)
{
  struct gameplay_fixture f;
  struct player_special_data specials = {0};
  struct char_data owner;
  bool denied = true, mounted, dismounted;
  int mode;

  begin_gameplay_fixture(&f);
  event_free_all();
  event_init();
  initialize_test_npc(&owner, "horse owner", 0);
  REMOVE_BIT_AR(MOB_FLAGS(&f.actor), MOB_ISNPC);
  f.actor.player_specials = &specials;
  f.actor.points.size = GET_REAL_SIZE(&f.actor) = SIZE_MEDIUM;
  SET_ABILITY(&f.actor, ABILITY_RIDE, 100);
  f.victim.points.size = GET_REAL_SIZE(&f.victim) = SIZE_LARGE;
  GET_LEVEL(&f.victim) = 1;
  f.victim.player.name = (char *)"horse";
  SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_MOUNTABLE);
  SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_TAMED);
  for (mode = 0; mode < 5; mode++)
  {
    if (mode == 0)
      SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_IMMATERIAL);
    if (mode == 1)
    {
      f.victim.master = &owner;
      SET_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
    }
    if (mode == 2)
      GET_POS(&f.victim) = POS_SLEEPING;
    if (mode == 3)
      SET_BIT_AR(MOB_FLAGS(&f.victim), MOB_NOTDEADYET);
    if (mode == 4)
      f.victim.points.size = GET_REAL_SIZE(&f.victim) = SIZE_MEDIUM;
    do_mount(&f.actor, "horse", 0, 0);
    denied = denied && RIDING(&f.actor) == NULL && RIDDEN_BY(&f.victim) == NULL &&
             is_action_available(&f.actor, atMOVE, false);
    REMOVE_BIT_AR(AFF_FLAGS(&f.victim), AFF_IMMATERIAL);
    REMOVE_BIT_AR(AFF_FLAGS(&f.victim), AFF_CHARM);
    REMOVE_BIT_AR(MOB_FLAGS(&f.victim), MOB_NOTDEADYET);
    f.victim.master = NULL;
    GET_POS(&f.victim) = POS_STANDING;
    f.victim.points.size = GET_REAL_SIZE(&f.victim) = SIZE_LARGE;
  }
  do_mount(&f.actor, "horse", 0, 0);
  mounted = RIDING(&f.actor) == &f.victim && RIDDEN_BY(&f.victim) == &f.actor &&
            f.victim.master == NULL && !pet_order_check(&f.actor, &f.victim);
  do_dismount(&f.actor, "", 0, 0);
  dismounted = RIDING(&f.actor) == NULL && RIDDEN_BY(&f.victim) == NULL;
  clear_char_event_list(&f.actor);
  event_free_all();
  end_gameplay_fixture(&f);
  CuAssertTrue(tc, denied);
  CuAssertTrue(tc, mounted);
  CuAssertTrue(tc, dismounted);
}

static void verify_corpse_animation_eligibility(CuTest *tc, int mode, bool charged, int *stats)
{
  struct gameplay_fixture fixture;
  struct player_special_data player_specials;
  struct char_data *owner;
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct obj_data *corpse, *loot;
  struct domain_entity_handle corpse_handle;
  unsigned long seed;
  bool created, retained, loot_retained, charge_retained, immaterial = FALSE;
  int spellnum = mode >= 5 ? SPELL_GREATER_ANIMATION : SPELL_ANIMATE_DEAD;
  int level = mode >= 5 ? 15 + (mode - 5) * 5 : 1;

  begin_gameplay_fixture(&fixture);
  memset(&player_specials, 0, sizeof(player_specials));
  owner = &fixture.actor;
  REMOVE_BIT_AR(MOB_FLAGS(owner), MOB_ISNPC);
  owner->player_specials = &player_specials;
  GET_PFILEPOS(owner) = -1;
  owner->pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  owner->char_specials.deathless_touch = charged;
  event_free_all();
  event_init();
  initialize_test_npc(&prototype, "animated zombie", NOWHERE);
  prototype.player.name = (char *)"zombie";
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = 100;
  fixture.mobile_index[0].vnum = animated_dead_summon_mob(spellnum, level);
  mob_proto = &prototype;
  corpse = create_obj();
  corpse->name = strdup(mode == 2 ? "pcorpse victim" : "corpse");
  corpse->short_description = strdup("a corpse");
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_VAL(corpse, 3) = 1;
  GET_OBJ_VAL(corpse, 4) = mode == 1 ? 123 : 0;
  CORPSE_ANIMATION_BLOCKED(corpse) = mode == 3;
  if (mode == 4)
    GET_OBJ_TYPE(corpse) = ITEM_TRASH;
  obj_to_room(corpse, 0);
  loot = create_obj();
  obj_to_obj(loot, corpse);
  corpse_handle = domain_event_object_handle(corpse);
  for (seed = 1; seed < 1000; seed++)
  {
    circle_srandom(seed);
    (void)rand_number(2, 6);
    if (mode >= 5)
      (void)summon_spell_mob_level(spellnum, level);
    if (rand_number(0, 101) >= 10)
      break;
  }
  circle_srandom(seed);
  mag_summons(level, &fixture.actor, corpse, spellnum, 0, CAST_SPELL);
  pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
  created = pet != NULL && IS_PET(pet) && MOB_FLAGGED(pet, MOB_ANIMATED_DEAD);
  charge_retained = owner->char_specials.deathless_touch;
  if (pet != NULL && stats != NULL)
  {
    affect_total(pet);
    affect_total(pet);
    stats[0] = GET_STR(pet);
    stats[1] = GET_DEX(pet);
    stats[2] = GET_CON(pet);
    stats[3] = GET_REAL_AC(pet);
    stats[4] = GET_MAX_HIT(pet);
    stats[5] = GET_LEVEL(pet);
  }
  corpse = domain_event_world_resolve_object(corpse_handle);
  retained = corpse != NULL;
  if (pet != NULL)
    immaterial = IS_INCORPOREAL(pet);
  if (mode >= 5 && mode <= 7)
    loot_retained = loot->in_room == 0 && loot->carried_by == NULL;
  else
    loot_retained = mode == 0 || mode == 8 ? loot->carried_by == pet : loot->in_obj == corpse;
  if (corpse != NULL)
    extract_obj(corpse);
  if (pet != NULL)
    extract_char(pet);
  extract_pending_chars();
  if (mode == 0 || mode >= 5)
    extract_obj(loot);
  domain_event_world_forget_character(&fixture.actor);
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&fixture);
  CuAssertIntEquals(tc, mode == 0 || mode >= 5, created);
  CuAssertIntEquals(tc, mode > 0 && mode < 5, retained);
  CuAssertIntEquals(tc, mode >= 5 && mode <= 7, immaterial);
  CuAssertTrue(tc, loot_retained);
  CuAssertIntEquals(tc, charged && mode > 0 && mode < 5, charge_retained);
}

void Test_gameplay_animation_rejects_player_and_blocked_corpses_without_taking_loot(CuTest *tc)
{
  int mode, i;
  int ordinary[6] = {0}, empowered[6] = {0};

  for (mode = 0; mode <= 8; mode++)
    verify_corpse_animation_eligibility(tc, mode, true, NULL);
  for (mode = 0; mode <= 5; mode += 5)
  {
    verify_corpse_animation_eligibility(tc, mode, false, ordinary);
    verify_corpse_animation_eligibility(tc, mode, true, empowered);
    for (i = 0; i < 3; i++)
      CuAssertIntEquals(tc, ordinary[i] + 2, empowered[i]);
    CuAssertIntEquals(tc, ordinary[3] + 20, empowered[3]);
    CuAssertIntEquals(tc, ordinary[4] + ordinary[5], empowered[4]);
    CuAssertIntEquals(tc, ordinary[5], empowered[5]);
  }
}

void Test_gameplay_pet_death_preserves_corpse_loot_but_blocks_reanimation(CuTest *tc)
{
  struct gameplay_fixture fixture;
  struct char_data prototype, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct obj_data *loot, *corpse;
  bool blocked, retained, dismounted;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  initialize_test_npc(&prototype, "summoned beast", NOWHERE);
  prototype.player.name = (char *)"beast";
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  GET_MOB_RNUM(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = 100;
  mob_proto = &prototype;
  pet = read_mobile(0, REAL);
  CuAssertTrue(tc, place_pet_follower(&fixture.actor, pet));
  loot = create_obj();
  obj_to_char(loot, pet);
  /* A control break must not make a summoned pet eligible for raising. */
  stop_follower(pet);
  mount_char(&fixture.actor, pet);
  raw_kill(pet, &fixture.victim);
  extract_pending_chars();
  dismounted = RIDING(&fixture.actor) == NULL;
  corpse = fixture.rooms[0].contents;
  blocked = corpse != NULL && IS_CORPSE(corpse) && !corpse_can_be_animated(corpse);
  retained = corpse != NULL && loot->in_obj == corpse;
  if (corpse != NULL)
    extract_obj(corpse);
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  event_free_all();
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&fixture);
  CuAssertTrue(tc, blocked);
  CuAssertTrue(tc, retained);
  CuAssertTrue(tc, dismounted);
}

static void verify_auto_raise_admission(CuTest *tc, int mode)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototypes[2], *pet;
  struct index_data indexes[2] = {0};
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct spell_info_type saved_spell = spell_info[SPELL_ANIMATE_DEAD];
  struct follow_type full_roster[2] = {0};
  struct char_data existing[2];
  struct obj_data *corpse;
  struct domain_entity_handle corpse_handle;
  unsigned long seed;
  bool attempted, created, retained, charged;
  int i;

  begin_gameplay_fixture(&fixture);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"autoraiseowner";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  CLASS_LEVEL((&fixture.actor), CLASS_NECROMANCER) = 2;
  SET_FEAT(&fixture.actor, FEAT_SUMMON_UNDEAD, mode == 2 ? 0 : 1);
  if (mode != 1)
    SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AUTORAISE);
  GET_LEVEL(&fixture.actor) = 2;
  for (i = 0; i < 2; i++)
  {
    initialize_test_npc(&prototypes[i], "animated undead", NOWHERE);
    prototypes[i].player.name = (char *)"undead";
    SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_CUSTOM_MOB_STATS);
    GET_MOB_RNUM(&prototypes[i]) = i;
    GET_PSP(&prototypes[i]) = GET_REAL_MAX_HIT(&prototypes[i]) = GET_REAL_MAX_MOVE(&prototypes[i]) =
        100;
    indexes[i].vnum = i == 0 ? (mode == 8 ? 1 : MOB_ZOMBIE) : MOB_MUMMY;
  }
  mob_proto = prototypes;
  mob_index = indexes;
  top_of_mobt = 1;
  memset(&spell_info[SPELL_ANIMATE_DEAD], 0, sizeof(spell_info[SPELL_ANIMATE_DEAD]));
  spell_info[SPELL_ANIMATE_DEAD].name = "animate dead";
  spell_info[SPELL_ANIMATE_DEAD].routines = MAG_SUMMONS;
  spell_info[SPELL_ANIMATE_DEAD].schoolOfMagic = NECROMANCY;
  corpse = create_obj();
  corpse->name = strdup("corpse");
  corpse->short_description = strdup("a corpse");
  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_VAL(corpse, 3) = 1;
  GET_OBJ_VAL(corpse, 4) = mode == 3 ? 123 : 0;
  CORPSE_ANIMATION_BLOCKED(corpse) = mode == 4;
  obj_to_room(corpse, 0);
  corpse_handle = domain_event_object_handle(corpse);
  if (mode == 5)
  {
    /* Two elite undead use all four Necromancer control points. */
    for (i = 0; i < 2; i++)
    {
      initialize_test_npc(&existing[i], "mummy", 0);
      GET_MOB_RNUM(&existing[i]) = 1;
      SET_BIT_AR(MOB_FLAGS(&existing[i]), MOB_ANIMATED_DEAD);
      SET_BIT_AR(AFF_FLAGS(&existing[i]), AFF_CHARM);
      existing[i].master = &fixture.actor;
      full_roster[i].follower = &existing[i];
      full_roster[i].next = i == 0 ? &full_roster[1] : NULL;
    }
    fixture.actor.followers = &full_roster[0];
  }
  if (mode == 6)
    USE_SWIFT_ACTION(&fixture.actor);
  for (seed = 1; seed < 1000; seed++)
  {
    circle_srandom(seed);
    (void)rand_number(2, 6);
    if ((rand_number(0, 101) < 10) == (mode == 7))
      break;
  }
  circle_srandom(seed);
  attempted = try_auto_raise_corpse(&fixture.actor, corpse);
  charged = !is_action_available(&fixture.actor, atSWIFT, FALSE);
  pet = fixture.actor.followers != NULL && fixture.actor.followers != &full_roster[0]
            ? fixture.actor.followers->follower
            : NULL;
  created = pet != NULL && IS_PET(pet);
  corpse = domain_event_world_resolve_object(corpse_handle);
  retained = corpse != NULL;
  if (corpse != NULL)
    extract_obj(corpse);
  if (pet != NULL)
    extract_char(pet);
  extract_pending_chars();
  fixture.actor.followers = NULL;
  clear_char_event_list(&fixture.actor);
  if (fixture.actor.events != NULL)
    free_list(fixture.actor.events);
  fixture.actor.events = NULL;
  domain_event_world_forget_character(&fixture.actor);
  event_free_all();
  spell_info[SPELL_ANIMATE_DEAD] = saved_spell;
  mob_proto = saved_proto;
  character_list = saved_characters;
  end_gameplay_fixture(&fixture);
  CuAssertIntEquals(tc, mode == 0 || mode == 7, attempted);
  CuAssertIntEquals(tc, mode == 0, created);
  CuAssertIntEquals(tc, mode != 0, retained);
  CuAssertIntEquals(tc, mode == 0 || mode == 6 || mode == 7, charged);
}

void Test_gameplay_auto_raise_requires_opt_in_eligible_corpse_and_control_capacity(CuTest *tc)
{
  int mode;

  for (mode = 0; mode <= 8; mode++)
    verify_auto_raise_admission(tc, mode);
}

static void verify_auto_raise_direct_kill(CuTest *tc, int mode)
{
  struct gameplay_fixture fixture;
  struct player_special_data specials = {0};
  struct char_data prototype, *victim, *pet;
  struct char_data *saved_proto = mob_proto, *saved_characters = character_list;
  struct spell_info_type saved_spell = spell_info[SPELL_ANIMATE_DEAD];
  struct obj_data *older, *obj;
  struct domain_entity_handle older_handle;
  bool charged, older_retained, deathless_raised = true, illusion_credit = true;
  int saved_max_gain = CONFIG_MAX_EXP_GAIN;
  int saved_exp_multiplier = CONFIG_EXPERIENCE_MULTIPLIER;
  char credit_error[160];

  begin_gameplay_fixture(&fixture);
  snprintf(credit_error, sizeof(credit_error), "illusion credit mode %d", mode);
  event_free_all();
  event_init();
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  fixture.actor.player.name = (char *)"killraiser";
  fixture.actor.player_specials = &specials;
  fixture.actor.pet_roster_load_state = PET_ROSTER_LOAD_FAILED;
  GET_PFILEPOS(&fixture.actor) = -1;
  GET_LEVEL(&fixture.actor) = 2;
  CLASS_LEVEL((&fixture.actor), CLASS_NECROMANCER) = 2;
  SET_FEAT(&fixture.actor, FEAT_SUMMON_UNDEAD, 1);
  SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AUTORAISE);
  initialize_test_npc(&prototype, "a beast", NOWHERE);
  prototype.player.name = (char *)"beast";
  SET_BIT_AR(MOB_FLAGS(&prototype), MOB_CUSTOM_MOB_STATS);
  GET_MOB_RNUM(&prototype) = 0;
  GET_LEVEL(&prototype) = 1;
  GET_EXP(&prototype) = 0;
  GET_PSP(&prototype) = GET_REAL_MAX_HIT(&prototype) = GET_REAL_MAX_MOVE(&prototype) = 100;
  fixture.mobile_index[0].vnum = MOB_ZOMBIE;
  mob_proto = &prototype;
  memset(&spell_info[SPELL_ANIMATE_DEAD], 0, sizeof(spell_info[SPELL_ANIMATE_DEAD]));
  spell_info[SPELL_ANIMATE_DEAD].name = "animate dead";
  spell_info[SPELL_ANIMATE_DEAD].routines = MAG_SUMMONS;
  spell_info[SPELL_ANIMATE_DEAD].schoolOfMagic = NECROMANCY;
  older = create_obj();
  older->name = strdup("corpse");
  older->short_description = strdup("an older corpse");
  GET_OBJ_TYPE(older) = ITEM_CONTAINER;
  GET_OBJ_VAL(older, 3) = 1;
  obj_to_room(older, 0);
  older_handle = domain_event_object_handle(older);
  victim = read_mobile(0, REAL);
  char_to_room(victim, 0);
  if (mode == 1)
    GET_REAL_RACE(victim) = RACE_TYPE_UNDEAD;
  if (mode == 2)
    victim->pet_source_spell = SPELL_SUMMON_CREATURE_1;
  if (mode == 3)
  {
    fixture.victim.master = &fixture.actor;
    SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  }
  if (mode == 4)
    SET_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AUTOSAC);
  if (mode >= 6)
  {
    CONFIG_MAX_EXP_GAIN = 1000;
    CONFIG_EXPERIENCE_MULTIPLIER = 100;
    GET_CLASS(&fixture.actor) = CLASS_NECROMANCER;
    GET_EXP(&fixture.actor) = 0;
    GET_EXP(victim) = 3000;
    fixture.victim.master = &fixture.actor;
    fixture.victim.pet_source_spell = SPELL_MISLEAD;
    SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
    if (mode == 7)
    {
      char_from_room(&fixture.actor);
      char_to_room(&fixture.actor, 1);
    }
    if (mode == 8)
      REMOVE_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
    illusion_credit = !pet_order_check(&fixture.actor, &fixture.victim);
  }
  circle_srandom(1);
  if (mode == 5)
  {
    REMOVE_BIT_AR(PRF_FLAGS(&fixture.actor), PRF_AUTORAISE);
    damage(&fixture.actor, victim, 10000, ABILITY_DEATHLESS_TOUCH, DAM_FORCE, FALSE);
  }
  else if (mode >= 6)
  {
    damage(&fixture.victim, victim, 10000, TYPE_HIT, DAM_FORCE, FALSE);
    snprintf(credit_error, sizeof(credit_error),
             "illusion mode %d: owner xp %ld, target hp %d, dead %d", mode, GET_EXP(&fixture.actor),
             GET_HIT(victim), !!MOB_FLAGGED(victim, MOB_NOTDEADYET));
    illusion_credit = illusion_credit && ((GET_EXP(&fixture.actor) > 0) == (mode == 6)) &&
                      MOB_FLAGGED(victim, MOB_NOTDEADYET);
  }
  else
    dam_killed_vict(mode == 3 ? &fixture.victim : &fixture.actor, victim);
  extract_pending_chars();
  if (mode == 5)
  {
    deathless_raised = fixture.actor.char_specials.deathless_touch;
    for (obj = fixture.rooms[0].contents; obj != NULL; obj = obj->next_content)
      if (obj != older && corpse_can_be_animated(obj))
        break;
    if (obj == NULL)
      deathless_raised = false;
    else
    {
      circle_srandom(1);
      mag_summons(1, &fixture.actor, obj, SPELL_ANIMATE_DEAD, 0, CAST_SPELL);
      pet = fixture.actor.followers != NULL ? fixture.actor.followers->follower : NULL;
      deathless_raised = deathless_raised && pet != NULL && IS_PET(pet) &&
                         MOB_FLAGGED(pet, MOB_ANIMATED_DEAD) &&
                         !fixture.actor.char_specials.deathless_touch;
    }
  }
  charged = !is_action_available(&fixture.actor, atSWIFT, FALSE);
  older_retained = domain_event_world_resolve_object(older_handle) != NULL;
  while (fixture.actor.followers != NULL)
  {
    pet = fixture.actor.followers->follower;
    extract_char(pet);
    extract_pending_chars();
  }
  while ((obj = fixture.rooms[0].contents) != NULL)
    extract_obj(obj);
  clear_char_event_list(&fixture.actor);
  if (fixture.actor.events != NULL)
    free_list(fixture.actor.events);
  fixture.actor.events = NULL;
  domain_event_world_forget_character(&fixture.actor);
  domain_event_world_forget_character(&fixture.victim);
  event_free_all();
  spell_info[SPELL_ANIMATE_DEAD] = saved_spell;
  mob_proto = saved_proto;
  character_list = saved_characters;
  CONFIG_MAX_EXP_GAIN = saved_max_gain;
  CONFIG_EXPERIENCE_MULTIPLIER = saved_exp_multiplier;
  end_gameplay_fixture(&fixture);
  CuAssertIntEquals(tc, mode == 0, charged);
  CuAssertTrue(tc, older_retained);
  CuAssertTrue(tc, deathless_raised);
  CuAssert(tc, credit_error, illusion_credit);
}

void Test_gameplay_auto_raise_uses_only_the_direct_kills_new_corpse(CuTest *tc)
{
  int mode;

  for (mode = 0; mode <= 8; mode++)
    verify_auto_raise_direct_kill(tc, mode);
}

void Test_gameplay_autoraise_toggle_requires_the_native_class_ability(CuTest *tc)
{
  struct char_data owner;
  struct player_special_data specials = {0};
  struct descriptor_data descriptor = {0};

  clear_char(&owner);
  owner.player_specials = &specials;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &owner;
  descriptor.pProtocol = ProtocolCreate();
  owner.desc = &descriptor;
  do_gen_tog(&owner, "", 0, SCMD_AUTORAISE);
  CuAssertTrue(tc, !PRF_FLAGGED(&owner, PRF_AUTORAISE));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "Summon Undead ability"));
  SET_FEAT(&owner, FEAT_SUMMON_UNDEAD, 1);
  do_gen_tog(&owner, "", 0, SCMD_AUTORAISE);
  CuAssertTrue(tc, PRF_FLAGGED(&owner, PRF_AUTORAISE));
  descriptor.output[0] = '\0';
  descriptor.bufptr = 0;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  SET_FEAT(&owner, FEAT_SUMMON_UNDEAD, 0);
  do_gen_tog(&owner, "", 0, SCMD_AUTORAISE);
  CuAssertTrue(tc, !PRF_FLAGGED(&owner, PRF_AUTORAISE));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "Summon Undead ability"));
  ProtocolDestroy(descriptor.pProtocol);
}
