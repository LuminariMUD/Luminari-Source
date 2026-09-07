/* Tests for the feats converted from Realms of Luminari player skills:
 * shadow, calm, establish camp, garrote and accompany. */

#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/actionqueues.h"
#include "../../src/activity_manager.h"
#include "../../src/actions.h"
#include "../../src/act.h"
#include "../../src/bardic_performance.h"
#include "../../src/character/abilities.h"
#include "../../src/constants.h"
#include "../../src/character/feats.h"
#include "../../src/combat/fight.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/domain_event_types.h"
#include "../../src/domain_event_world.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"
#include "../../src/rol_feats.h"

#include <string.h>

struct rol_feat_fixture
{
  struct room_data rooms[2];
  struct char_data lead;
  struct char_data second;
  struct group_data group;
  struct player_special_data lead_specials;
  struct player_special_data second_specials;
  struct descriptor_data lead_descriptor;
  struct descriptor_data second_descriptor;
  struct index_data mob_index_entry;
  struct room_data *saved_world;
  struct char_data *saved_character_list;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
};

typedef void (*rol_command_handler)(struct char_data *ch, const char *argument, int cmd,
                                    int subcmd);
typedef int (*rol_command_check)(struct char_data *ch, const char *argument, bool show_error);

static void setup_test_char(struct char_data *ch, struct player_special_data *specials,
                            struct descriptor_data *descriptor, const char *name)
{
  clear_char(ch);
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
  ch->player_specials = specials;
  ch->player.name = (char *)name;
  ch->desc = descriptor;
  IN_ROOM(ch) = 0;
  GET_LEVEL(ch) = 10;
  GET_REAL_SIZE(ch) = SIZE_MEDIUM;
  ch->points.size = SIZE_MEDIUM;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100;

  memset(descriptor, 0, sizeof(*descriptor));
  descriptor->character = ch;
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
  descriptor->pProtocol = ProtocolCreate();
  STATE(descriptor) = CON_PLAYING;
}

static void begin_rol_feat_fixture(struct rol_feat_fixture *fixture)
{
  if (skill_info[SKILL_SONG_OF_HEALING].name == NULL)
    mag_assign_spells();

  event_init();
  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_character_list = character_list;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  setup_test_char(&fixture->lead, &fixture->lead_specials, &fixture->lead_descriptor,
                  "rol feat lead");
  setup_test_char(&fixture->second, &fixture->second_specials, &fixture->second_descriptor,
                  "rol feat second");

  fixture->lead.next_in_room = &fixture->second;
  fixture->lead.next = &fixture->second;
  fixture->group.leader = &fixture->lead;
  fixture->lead.group = &fixture->group;
  fixture->second.group = &fixture->group;

  fixture->rooms[0].number = 169900;
  fixture->rooms[0].people = &fixture->lead;
  fixture->rooms[1].number = 169901;
  world = fixture->rooms;
  top_of_world = 1;
  character_list = &fixture->lead;
  fixture->mob_index_entry.vnum = DG_CASTER_PROXY;
  mob_index = &fixture->mob_index_entry;
  top_of_mobt = 0;
}

static void end_test_char(struct char_data *ch, struct descriptor_data *descriptor)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);
  clear_char_event_list(ch);
  free_attack_queue(GET_ATTACK_QUEUE(ch));
  GET_ATTACK_QUEUE(ch) = NULL;
  ch->desc = NULL;
  if (descriptor->pProtocol != NULL)
    ProtocolDestroy(descriptor->pProtocol);
}

static void end_rol_feat_fixture(struct rol_feat_fixture *fixture)
{
  int room_index;

  end_test_char(&fixture->lead, &fixture->lead_descriptor);
  end_test_char(&fixture->second, &fixture->second_descriptor);
  for (room_index = 0; room_index < 2; room_index++)
    while (fixture->rooms[room_index].affected_head != NULL)
      rem_room_aff(fixture->rooms[room_index].affected_head);
  event_free_all();
  (void)event_test_select_backend(EVENT_BACKEND_UNINITIALIZED);
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  character_list = fixture->saved_character_list;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

static bool rol_feat_file_contains(const char *relative_path, const char *needle)
{
  const char *root = NULL;
  char line[MAX_STRING_LENGTH];
  char path[PATH_MAX];
  FILE *file = NULL;

  root = getenv("LUMINARI_TEST_ROOT");
  if (root == NULL || *root == '\0')
    root = ".";
  if (snprintf(path, sizeof(path), "%s/%s", root, relative_path) >= (int)sizeof(path))
    return FALSE;

  file = fopen(path, "r");
  if (file == NULL)
    return FALSE;

  while (fgets(line, sizeof(line), file) != NULL)
  {
    if (strstr(line, needle) != NULL)
    {
      fclose(file);
      return TRUE;
    }
  }

  fclose(file);
  return FALSE;
}

/* A room camp speeds recovery for everyone settled there, but nowhere else. */
void TestCampRecoveryRequiresRestingInCamp(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  GET_POS(&fixture.lead) = POS_RESTING;
  CuAssertIntEquals(tc, 0, camp_recovery_bonus(&fixture.lead, 40));

  test_camp_create_site(&fixture.lead);
  CuAssertTrue(tc, ROOM_AFFECTED(0, RAFF_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.lead, 40));

  GET_POS(&fixture.second) = POS_RESTING;
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.second, 40));

  GET_POS(&fixture.lead) = POS_SLEEPING;
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.lead, 40));

  IN_ROOM(&fixture.lead) = 1;
  CuAssertIntEquals(tc, 0, camp_recovery_bonus(&fixture.lead, 40));

  IN_ROOM(&fixture.lead) = 0;
  CuAssertIntEquals(tc, 20, camp_recovery_bonus(&fixture.lead, 40));

  GET_POS(&fixture.lead) = POS_STANDING;
  CuAssertIntEquals(tc, 0, camp_recovery_bonus(&fixture.lead, 40));

  end_rol_feat_fixture(&fixture);
}

void TestCampRoomAffectExpiresAndClearsTheSite(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);
  test_camp_create_site(&fixture.lead);
  CuAssertTrue(tc, fixture.rooms[0].affected_head != NULL);
  CuAssertIntEquals(tc, SKILL_CAMP, fixture.rooms[0].affected_head->spell);

  fixture.rooms[0].affected_head->timer = 1;
  CuAssertIntEquals(tc, 1, (int)affect_update_room_one(&fixture.rooms[0]));
  CuAssertTrue(tc, !ROOM_AFFECTED(0, RAFF_CAMP));
  CuAssertTrue(tc, fixture.rooms[0].affected_head == NULL);

  end_rol_feat_fixture(&fixture);
}

void TestCampRejectsInvalidTerrainWithConsistentMessage(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);
  fixture.rooms[0].sector_type = SECT_WATER_SWIM;
  SET_FEAT(&fixture.lead, FEAT_ESTABLISH_CAMP, 1);

  do_camp(&fixture.lead, "", 0, 0);

  CuAssertTrue(tc, strstr(fixture.lead_descriptor.output, "You can't camp here!") != NULL);
  CuAssertTrue(tc, !ROOM_AFFECTED(0, RAFF_CAMP));
  end_rol_feat_fixture(&fixture);
}

void TestCampCommandUsesManagedActivityBeforeApplyingExistingBenefits(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  struct domain_event_bus *bus;
  struct primary_activity_snapshot snapshot;
  enum domain_event_status status;
  unsigned long saved_pulse = pulse;

  begin_rol_feat_fixture(&fixture);
  fixture.rooms[0].sector_type = SECT_FOREST;
  fixture.group.members = create_list();
  add_to_list(&fixture.lead, fixture.group.members);
  add_to_list(&fixture.second, fixture.group.members);
  SET_FEAT(&fixture.lead, FEAT_ESTABLISH_CAMP, 1);
  SET_ABILITY(&fixture.lead, ABILITY_SURVIVAL, 40);

  primary_activity_manager_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  bus = domain_event_bus_create(NULL, &status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  primary_activity_test_select_camp(true);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, primary_activity_manager_init(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));

  do_camp(&fixture.lead, "", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.lead, &snapshot));
  CuAssertIntEquals(tc, PRIMARY_ACTIVITY_CAMP, snapshot.type);
  CuAssertIntEquals(tc, 0, (int)snapshot.completed_steps);
  CuAssertTrue(tc, !ROOM_AFFECTED(0, RAFF_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.second, SKILL_CAMP));

  pulse += 2 RL_SEC;
  event_test_advance();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.lead, &snapshot));
  CuAssertIntEquals(tc, 1, (int)snapshot.completed_steps);
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));

  pulse += 2 RL_SEC;
  event_test_advance();
  CuAssertTrue(tc, primary_activity_snapshot(&fixture.lead, &snapshot));
  CuAssertIntEquals(tc, 2, (int)snapshot.completed_steps);
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));

  pulse += 2 RL_SEC;
  event_test_advance();
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.lead, &snapshot));
  CuAssertTrue(tc, ROOM_AFFECTED(0, RAFF_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.second, SKILL_CAMP));
  CuAssertIntEquals(tc, fixture.rooms[0].number, GET_LOADROOM(&fixture.lead));
  CuAssertIntEquals(tc, fixture.rooms[0].number, GET_LOADROOM(&fixture.second));

  primary_activity_manager_shutdown();
  domain_event_bus_destroy(bus);
  event_free_all();
  fixture.lead.group = NULL;
  fixture.second.group = NULL;
  free_list(fixture.group.members);
  fixture.group.members = NULL;
  pulse = saved_pulse;
  end_rol_feat_fixture(&fixture);
}

void TestCampCommandLegacySelectorPreservesImmediateRollback(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  struct domain_event_bus *bus;
  struct primary_activity_snapshot snapshot;
  enum domain_event_status status;

  begin_rol_feat_fixture(&fixture);
  fixture.rooms[0].sector_type = SECT_FOREST;
  fixture.group.members = create_list();
  add_to_list(&fixture.lead, fixture.group.members);
  add_to_list(&fixture.second, fixture.group.members);
  SET_FEAT(&fixture.lead, FEAT_ESTABLISH_CAMP, 1);
  SET_ABILITY(&fixture.lead, ABILITY_SURVIVAL, 40);

  primary_activity_manager_shutdown();
  event_free_all();
  CuAssertIntEquals(tc, 1, event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER));
  event_init();
  bus = domain_event_bus_create(NULL, &status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, status);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_register_foundation_types(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_world_register_resolvers(bus));
  primary_activity_test_select_camp(false);
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, primary_activity_manager_init(bus));
  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, domain_event_seal(bus));

  do_camp(&fixture.lead, "", 0, 0);
  CuAssertTrue(tc, !primary_activity_snapshot(&fixture.lead, &snapshot));
  CuAssertTrue(tc, ROOM_AFFECTED(0, RAFF_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.lead, SKILL_CAMP));
  CuAssertTrue(tc, !affected_by_spell(&fixture.second, SKILL_CAMP));

  primary_activity_test_select_camp(true);
  primary_activity_manager_shutdown();
  domain_event_bus_destroy(bus);
  event_free_all();
  fixture.lead.group = NULL;
  fixture.second.group = NULL;
  free_list(fixture.group.members);
  fixture.group.members = NULL;
  end_rol_feat_fixture(&fixture);
}

/* Calm's limited uses are wired to the generic daily-use event machinery. */
void TestCalmUsesDailyCooldownRegistry(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  struct mud_event_data *event = NULL;
  int uses = 0;

  begin_rol_feat_fixture(&fixture);
  if (feat_list[FEAT_CALM].name == NULL)
    assign_feats();

  GET_CHA(&fixture.lead) = 16;
  uses = get_daily_uses(&fixture.lead, FEAT_CALM);

  CuAssertIntEquals(tc, eROL_CALM, feat_list[FEAT_CALM].event);
  CuAssertTrue(tc, uses > 1);
  CuAssertIntEquals(tc, 1, start_daily_use_cooldown(&fixture.lead, FEAT_CALM));
  CuAssertIntEquals(tc, uses - 1, daily_uses_remaining(&fixture.lead, FEAT_CALM));

  event = char_has_mud_event(&fixture.lead, eROL_CALM);
  CuAssertTrue(tc, event != NULL);
  CuAssertStrEquals(tc, "uses:1", event->sVariables);

  end_rol_feat_fixture(&fixture);
}

/* Only an able accompanist with the feat lifts a lead performance. */
void TestAccompanimentBonusRequiresAnAbleAccompanist(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  int bonus;

  begin_rol_feat_fixture(&fixture);

  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  ACCOMPANYING(&fixture.second) = &fixture.lead;
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  fixture.second.group = NULL;
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  fixture.second.group = &fixture.group;
  bonus = accompaniment_bonus(&fixture.lead);
  CuAssertTrue(tc, bonus >= 1);
  CuAssertTrue(tc, bonus <= 12);

  /* an accompanist leading a song of their own is not backing anyone */
  IS_PERFORMING(&fixture.second) = TRUE;
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));
  IS_PERFORMING(&fixture.second) = FALSE;

  /* a silenced accompanist contributes nothing */
  SET_BIT_AR(AFF_FLAGS(&fixture.second), AFF_SILENCED);
  CuAssertIntEquals(tc, 0, accompaniment_bonus(&fixture.lead));

  end_rol_feat_fixture(&fixture);
}

/* A faltering lead hands the song to an accompanist instead of losing it. */
void TestAccompanyTakeoverPassesTheSongOn(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  SET_FEAT(&fixture.second, FEAT_BARDIC_MUSIC, 1);
  SET_FEAT(&fixture.second, FEAT_SONG_OF_HEALING, 1);
  ACCOMPANYING(&fixture.second) = &fixture.lead;

  CuAssertTrue(tc, accompany_takeover(&fixture.lead, 0));
  CuAssertIntEquals(tc, 0, GET_PERFORMING(&fixture.second));
  CuAssertTrue(tc, IS_PERFORMING(&fixture.second));
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Without the song's own feat there is nobody to hand the song to. */
void TestAccompanyTakeoverNeedsTheSongFeat(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_FEAT(&fixture.second, FEAT_ACCOMPANY, 1);
  SET_FEAT(&fixture.second, FEAT_BARDIC_MUSIC, 1);
  ACCOMPANYING(&fixture.second) = &fixture.lead;

  CuAssertTrue(tc, !accompany_takeover(&fixture.lead, 0));
  CuAssertTrue(tc, !IS_PERFORMING(&fixture.second));
  CuAssertPtrEquals(tc, &fixture.lead, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Garrote accepts any equipment layout that leaves at least one hand free. */
void TestGarroteRequiresAFreeHand(CuTest *tc)
{
  struct rol_feat_fixture fixture;
  struct obj_data held_one;
  struct obj_data held_two;

  begin_rol_feat_fixture(&fixture);
  memset(&held_one, 0, sizeof(held_one));
  memset(&held_two, 0, sizeof(held_two));

  SET_FEAT(&fixture.lead, FEAT_GARROTE, 1);
  SET_BIT_AR(AFF_FLAGS(&fixture.lead), AFF_HIDE);
  SET_BIT_AR(AFF_FLAGS(&fixture.lead), AFF_SNEAK);
  CuAssertIntEquals(tc, CAN_CMD, can_garrote(&fixture.lead, "", FALSE));

  GET_EQ(&fixture.lead, WEAR_HOLD_1) = &held_one;
  CuAssertIntEquals(tc, CAN_CMD, can_garrote(&fixture.lead, "", FALSE));

  GET_EQ(&fixture.lead, WEAR_HOLD_2) = &held_two;
  CuAssertTrue(tc, can_garrote(&fixture.lead, "", FALSE) != CAN_CMD);

  GET_EQ(&fixture.lead, WEAR_HOLD_1) = NULL;
  GET_EQ(&fixture.lead, WEAR_HOLD_2) = NULL;
  end_rol_feat_fixture(&fixture);
}

/* A tail cannot be kept up by someone who has stopped moving silently. */
void TestShadowTailBreaksWithoutSneak(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SHADOWING(&fixture.second) = &fixture.lead;
  shadowers_follow(&fixture.lead, 0, NORTH);

  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));
  CuAssertIntEquals(tc, 0, IN_ROOM(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Moving away independently ends the tail instead of leaving a stale link. */
void TestShadowTailBreaksWhenTailMovesAway(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SHADOWING(&fixture.second) = &fixture.lead;
  shadow_movement_complete(&fixture.second);
  CuAssertPtrEquals(tc, &fixture.lead, SHADOWING(&fixture.second));

  IN_ROOM(&fixture.second) = 1;
  shadow_movement_complete(&fixture.second);
  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Nor by someone who has been drawn into a fight. */
void TestShadowTailBreaksWhenFighting(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SET_BIT_AR(AFF_FLAGS(&fixture.second), AFF_SNEAK);
  FIGHTING(&fixture.second) = &fixture.lead;
  SHADOWING(&fixture.second) = &fixture.lead;

  shadowers_follow(&fixture.lead, 0, NORTH);

  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));

  FIGHTING(&fixture.second) = NULL;
  end_rol_feat_fixture(&fixture);
}

/* Tails and accompaniments are dropped in both directions when a link dies. */
void TestShadowAndAccompanyLinksAreClearedBothWays(CuTest *tc)
{
  struct rol_feat_fixture fixture;

  begin_rol_feat_fixture(&fixture);

  SHADOWING(&fixture.second) = &fixture.lead;
  SHADOWING(&fixture.lead) = &fixture.second;
  clear_shadow_links(&fixture.lead);
  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.lead));
  CuAssertPtrEquals(tc, NULL, SHADOWING(&fixture.second));

  ACCOMPANYING(&fixture.second) = &fixture.lead;
  ACCOMPANYING(&fixture.lead) = &fixture.second;
  clear_accompany_links(&fixture.lead);
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.lead));
  CuAssertPtrEquals(tc, NULL, ACCOMPANYING(&fixture.second));

  end_rol_feat_fixture(&fixture);
}

/* Every converted skill is reachable as a command and as a feat. */
void TestConvertedRolSkillsAreRegistered(CuTest *tc)
{
  static const struct
  {
    const char *name;
    rol_command_handler handler;
    rol_command_check check;
    int actions;
  } commands[] = {{"shadow", do_shadow, can_shadow, ACTION_MOVE},
                  {"calm", do_calm, can_calm, ACTION_STANDARD},
                  {"camp", do_camp, can_camp, ACTION_STANDARD | ACTION_MOVE},
                  {"garrote", do_garrote, can_garrote, ACTION_STANDARD | ACTION_MOVE},
                  {"accompany", do_accompany, can_accompany, ACTION_MOVE}};
  static const char *feats[] = {"shadow", "calm", "establish camp", "garrote", "accompany"};
  size_t i;
  int j;
  bool found;

  for (i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
  {
    found = FALSE;
    for (j = 0; *cmd_info[j].command != '\n'; j++)
    {
      if (str_cmp(cmd_info[j].command, commands[i].name) == 0)
      {
        found = TRUE;
        CuAssertTrue(tc, cmd_info[j].command_pointer == commands[i].handler);
        CuAssertTrue(tc, cmd_info[j].command_check_pointer == commands[i].check);
        CuAssertIntEquals(tc, commands[i].actions, cmd_info[j].actions_required);
        break;
      }
    }
    CuAssertTrue(tc, found);
  }

  for (i = 0; i < sizeof(feats) / sizeof(feats[0]); i++)
    CuAssertTrue(tc, find_feat_num(feats[i]) > 0);
}

/* Both maintained help sources and their verifier cover every converted feat. */
void TestConvertedRolFeatHelpSourcesAreComplete(CuTest *tc)
{
  static const char *flat_keywords[] = {"ACCOMPANY", "CALM PACIFY", "CAMP ESTABLISH-CAMP",
                                        "GARROTE STRANGLE", "SHADOW TAIL"};
  static const char *database_tags[] = {"VALUES ('ACCOMPANY'", "VALUES ('CALM'",
                                        "VALUES ('CAMP'",      "VALUES ('ACTIVITY'",
                                        "VALUES ('GARROTE'",   "VALUES ('SHADOW'"};
  static const char *prerequisite_text[] = {"5 ranks of perform",
                                            "free at level 2",
                                            "charisma 19",
                                            "3 ranks of nature",
                                            "14 ranks of stealth and BAB 8",
                                            "21 ranks of stealth"};
  size_t i = 0;

  for (i = 0; i < sizeof(flat_keywords) / sizeof(flat_keywords[0]); i++)
    CuAssertTrue(tc, rol_feat_file_contains("lib/text/help/help.hlp", flat_keywords[i]));

  for (i = 0; i < sizeof(database_tags) / sizeof(database_tags[0]); i++)
    CuAssertTrue(
        tc, rol_feat_file_contains("sql/components/help_rol_feat_entries.sql", database_tags[i]));

  for (i = 0; i < sizeof(prerequisite_text) / sizeof(prerequisite_text[0]); i++)
  {
    CuAssertTrue(tc, rol_feat_file_contains("lib/text/help/help.hlp", prerequisite_text[i]));
    CuAssertTrue(tc, rol_feat_file_contains("sql/components/help_rol_feat_entries.sql",
                                            prerequisite_text[i]));
  }

  CuAssertTrue(tc, rol_feat_file_contains("sql/components/verify_help_rol_feat_entries.sql",
                                          "rol_feat_content"));
  CuAssertTrue(tc, rol_feat_file_contains("sql/components/verify_help_rol_feat_entries.sql",
                                          "rol_feat_command_keyword_owners"));
  CuAssertTrue(tc, rol_feat_file_contains("sql/components/help_rol_feat_entries.sql",
                                          "bards gain it for free at level 2"));
  CuAssertTrue(tc, rol_feat_file_contains("sql/components/verify_help_rol_feat_entries.sql",
                                          "'ACTIVITY', 'PRIMARY-ACTIVITY'"));
  CuAssertTrue(tc, rol_feat_file_contains("sql/components/help_rol_feat_entries.sql",
                                          "UPPER(keyword) = 'ACTIVITY'"));
}

static int rol_feat_prerequisite_count(int feat_num)
{
  struct feat_prerequisite *prereq = NULL;
  int count = 0;

  for (prereq = feat_list[feat_num].prerequisite_list; prereq != NULL; prereq = prereq->next)
    count++;

  return count;
}

static bool rol_feat_has_prerequisite(int feat_num, int prerequisite_type, int value0, int value1)
{
  struct feat_prerequisite *prereq = NULL;

  for (prereq = feat_list[feat_num].prerequisite_list; prereq != NULL; prereq = prereq->next)
  {
    if (prereq->prerequisite_type == prerequisite_type && prereq->values[0] == value0 &&
        prereq->values[1] == value1)
      return TRUE;
  }

  return FALSE;
}

/* Converted feat selection gates stay aligned with their intended advancement bands. */
void TestConvertedRolFeatPrerequisitesAreExact(CuTest *tc)
{
  if (feat_list[FEAT_SHADOW].name == NULL)
    assign_feats();

  CuAssertIntEquals(tc, 1, rol_feat_prerequisite_count(FEAT_SHADOW));
  CuAssertTrue(tc,
               rol_feat_has_prerequisite(FEAT_SHADOW, FEAT_PREREQ_ABILITY, ABILITY_STEALTH, 21));

  CuAssertIntEquals(tc, 1, rol_feat_prerequisite_count(FEAT_CALM));
  CuAssertTrue(tc, rol_feat_has_prerequisite(FEAT_CALM, FEAT_PREREQ_ATTRIBUTE, AB_CHA, 19));

  CuAssertIntEquals(tc, 1, rol_feat_prerequisite_count(FEAT_ESTABLISH_CAMP));
  CuAssertTrue(
      tc, rol_feat_has_prerequisite(FEAT_ESTABLISH_CAMP, FEAT_PREREQ_ABILITY, ABILITY_SURVIVAL, 3));

  CuAssertIntEquals(tc, 2, rol_feat_prerequisite_count(FEAT_GARROTE));
  CuAssertTrue(tc,
               rol_feat_has_prerequisite(FEAT_GARROTE, FEAT_PREREQ_ABILITY, ABILITY_STEALTH, 14));
  CuAssertTrue(tc, rol_feat_has_prerequisite(FEAT_GARROTE, FEAT_PREREQ_BAB, 8, 0));

  CuAssertIntEquals(tc, 1, rol_feat_prerequisite_count(FEAT_ACCOMPANY));
  CuAssertTrue(tc,
               rol_feat_has_prerequisite(FEAT_ACCOMPANY, FEAT_PREREQ_ABILITY, ABILITY_PERFORM, 5));
}

/* All five feats remain learnable; only bards receive Accompany for free. */
void TestConvertedRolFeatsAreNormallyLearnableWithExactClassGrants(CuTest *tc)
{
  static const int feats[] = {FEAT_SHADOW, FEAT_CALM, FEAT_ESTABLISH_CAMP, FEAT_GARROTE,
                              FEAT_ACCOMPANY};
  struct class_feat_assign *assignment;
  int accompany_assignments = 0;
  size_t i;
  int class_num;

  if (feat_list[FEAT_SHADOW].name == NULL)
    assign_feats();
  if (class_list[CLASS_WIZARD].name == NULL)
    load_class_list();

  for (i = 0; i < sizeof(feats) / sizeof(feats[0]); i++)
  {
    CuAssertTrue(tc, feat_list[feats[i]].can_learn);
    CuAssertTrue(tc, feat_list[feats[i]].feat_type > FEAT_TYPE_NONE);
    CuAssertTrue(tc, feat_list[feats[i]].feat_type < NUM_LEARNABLE_FEAT_TYPES);

    for (class_num = 0; class_num < NUM_CLASSES; class_num++)
    {
      for (assignment = class_list[class_num].featassign_list; assignment != NULL;
           assignment = assignment->next)
      {
        if (assignment->feat_num != feats[i])
          continue;

        CuAssertIntEquals(tc, FEAT_ACCOMPANY, feats[i]);
        CuAssertIntEquals(tc, CLASS_BARD, class_num);
        CuAssertTrue(tc, assignment->is_classfeat);
        CuAssertIntEquals(tc, 2, assignment->level_received);
        CuAssertTrue(tc, !assignment->stacks);
        accompany_assignments++;
      }
    }
  }

  CuAssertIntEquals(tc, 1, accompany_assignments);
}
