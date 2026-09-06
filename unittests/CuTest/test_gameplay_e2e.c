#include "CuTest.h"

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
#include "../../src/magic/buff_sequence.h"
#include "../../src/magic/spell_prep.h"
#include "../../src/domain_event_runtime.h"
#include "../../src/domain_object_transfer.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_event_world.h"
#include "../../src/event_runtime.h"
#include "../../src/bardic_performance.h"
#include "../../src/craft/craft.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/db.h"
#include "../../src/comm.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/combat/fight.h"
#include "../../src/olc/genwld.h"
#include "../../src/olc/genobj.h"
#include "../../src/vessels/transport.h"
#include "../../src/vessels/transport_jobs.h"
#include "../../src/quest/staff_events.h"
#include "../../src/quest/staff_event_agenda.h"
#include "../../src/vessels/routing.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/mob/mob_utils.h"
#include "../../src/quest/missions.h"
#include "../../src/quest/quest.h"
#include "../../src/movement/movement.h"
#include "../../src/movement/door_state.h"
#include "../../src/character/perks.h"
#include "../../src/net/protocol.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/spec/spec_binding.h"
#include "../../src/mud_event.h"
#include "../../src/mudlim.h"
#include "../../src/dgscript/dg_event.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void verify_gameplay_charge_load(CuTest *tc, unsigned int format, int elapsed, int charisma,
                                        int interval, const char *expected)
{
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
  CuAssertIntEquals(tc, 0, chdir("lib"));
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
  CuAssertIntEquals(tc, 0, chdir(directory));
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

void Test_gameplay_save_captures_charge_cadence_before_unequipping(CuTest *tc)
{
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
  CuAssertIntEquals(tc, 0, chdir("lib"));
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
  CuAssertIntEquals(tc, 0, chdir(directory));
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

void Test_gameplay_transport_loads_a_versioned_stable_destination(CuTest *tc)
{
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
  CuAssertIntEquals(tc, 0, chdir("lib"));
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
  directory_restored = chdir(directory);
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
  int i;

  begin_gameplay_fixture(&f);
  character_list = NULL;
  staffevent_data.event_num = UNDEFINED_EVENT;
  staffevent_data.ticks_left = staffevent_data.delay = 0;
  event_free_all();
  pulse = 10 * PASSES_PER_SEC;
  event_init();
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_runtime_init());
  CuAssertTrue(tc, set_event_state(THE_PRISONER_EVENT, 2));
  active = is_event_active() && get_event_time_remaining() == 2 &&
           staff_event_agenda_seconds() == 2 * SECS_PER_MUD_HOUR - 10;
  if (mode == 1)
  {
    CuAssertIntEquals(tc, EVENT_ERROR_NO_ACTIVE_EVENT, end_staff_event(JACKALOPE_HUNT));
    CuAssertTrue(tc, is_event_active());
    CuAssertIntEquals(tc, EVENT_SUCCESS, end_staff_event(THE_PRISONER_EVENT));
    CuAssertIntEquals(tc, EVENT_ERROR_DELAY_ACTIVE, start_staff_event(JACKALOPE_HUNT));
    CuAssertTrue(tc, set_event_delay(0));
    CuAssertTrue(tc, set_event_state(THE_PRISONER_EVENT, 3));
  }
  for (i = 0; i < (2 * SECS_PER_MUD_HOUR - 10) * PASSES_PER_SEC; i++)
  {
    pulse++;
    event_test_advance();
  }
  expired = mode == 1 ? is_event_active() && get_event_time_remaining() == 1 : !is_event_active();
  if (mode == 1)
    end_staff_event(THE_PRISONER_EVENT);
  delayed = get_event_delay() == STAFF_EVENT_DELAY_CNST;
  for (i = 0; i < STAFF_EVENT_DELAY_CNST * SECS_PER_MUD_HOUR * PASSES_PER_SEC; i++)
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

static void verify_tactical_clock_persistence(CuTest *tc, int format, bool bleeding)
{
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
  CuAssertIntEquals(tc, 0, chdir("lib"));
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
  CuAssertIntEquals(tc, 0, chdir(directory));
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
