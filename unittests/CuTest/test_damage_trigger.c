#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/constants.h"
#include "../../src/db.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/dgscript/dg_olc.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/combat/fight.h"
#include "../../src/handler.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/net/protocol.h"
#include "../../src/olc/genolc.h"
#include "../../src/olc/oasis.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DAMAGE_TRIGGER_TEST_CAPACITY 16

struct damage_trigger_fixture
{
  struct room_data room;
  struct zone_data zone;
  struct index_data mobile_index;
  struct char_data mobile_prototype;
  struct char_data actor;
  struct char_data victim;
  struct player_special_data player_specials;
  struct room_data *saved_world;
  struct zone_data *saved_zone_table;
  struct index_data *saved_mob_index;
  struct char_data *saved_mob_proto;
  struct index_data **saved_trig_index;
  struct trig_data *saved_trigger_list;
  struct char_data *saved_combat_list;
  room_rnum saved_top_of_world;
  zone_rnum saved_top_of_zone_table;
  mob_rnum saved_top_of_mobt;
  trig_rnum saved_top_of_trigt;
};

static void damage_trigger_initialize_npc(struct char_data *ch, const char *name)
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
  IN_ROOM(ch) = 0;
}

static bool damage_trigger_fixture_begin(struct damage_trigger_fixture *fixture)
{
  memset(fixture, 0, sizeof(*fixture));

  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_mob_proto = mob_proto;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->saved_trig_index = trig_index;
  fixture->saved_top_of_trigt = top_of_trigt;
  fixture->saved_trigger_list = trigger_list;
  fixture->saved_combat_list = combat_list;

  fixture->room.number = 100;
  fixture->room.zone = 0;
  fixture->room.sector_type = SECT_INSIDE;
  fixture->room.name = "Damage trigger test room";
  fixture->room.description = "A production-linked damage trigger test room.\r\n";
  fixture->zone.number = 0;
  fixture->zone.bot = 100;
  fixture->zone.top = 199;
  fixture->zone.min_level = -1;
  fixture->zone.max_level = LVL_IMPL;
  fixture->mobile_index.vnum = 9100;

  world = &fixture->room;
  top_of_world = 0;
  zone_table = &fixture->zone;
  top_of_zone_table = 0;
  mob_index = &fixture->mobile_index;
  top_of_mobt = 0;
  damage_trigger_initialize_npc(&fixture->mobile_prototype, "damage trigger dynamic mob");
  fixture->mobile_prototype.player.name = "damage trigger dynamic";
  fixture->mobile_prototype.nr = 0;
  GET_PSP(&fixture->mobile_prototype) = 100;
  mob_proto = &fixture->mobile_prototype;
  trig_index = calloc(DAMAGE_TRIGGER_TEST_CAPACITY, sizeof(*trig_index));
  top_of_trigt = 0;

  damage_trigger_initialize_npc(&fixture->actor, "damage trigger actor");
  damage_trigger_initialize_npc(&fixture->victim, "damage trigger victim");
  fixture->room.people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->victim;

  return trig_index != NULL;
}

static void damage_trigger_free_prototypes(void)
{
  struct cmdlist_element *command;
  struct cmdlist_element *next_command;
  struct index_data *index;
  int i;

  for (i = 0; i < top_of_trigt; i++)
  {
    index = trig_index[i];
    if (index == NULL)
      continue;

    command = ((struct trig_data *)index->proto)->cmdlist;
    free_trigger((struct trig_data *)index->proto);
    while (command != NULL)
    {
      next_command = command->next;
      free(command->cmd);
      free(command);
      command = next_command;
    }
    free(index);
  }
}

static void damage_trigger_fixture_end(struct damage_trigger_fixture *fixture)
{
  clear_char_event_list(&fixture->actor);
  clear_char_event_list(&fixture->victim);
  FIGHTING(&fixture->actor) = NULL;
  FIGHTING(&fixture->victim) = NULL;
  fixture->actor.next_fighting = NULL;
  fixture->victim.next_fighting = NULL;
  combat_list = fixture->saved_combat_list;
  fixture->actor.last_attacker = NULL;
  fixture->victim.last_attacker = NULL;
  fixture->actor.next_in_room = NULL;
  fixture->victim.next_in_room = NULL;
  fixture->room.people = NULL;

  if (SCRIPT(&fixture->victim) != NULL)
    extract_script(&SCRIPT(&fixture->victim));
  if (SCRIPT(&fixture->actor) != NULL)
    extract_script(&SCRIPT(&fixture->actor));
  if (GET_ID(&fixture->actor) != 0)
    remove_from_lookup_table(GET_ID(&fixture->actor));
  if (GET_ID(&fixture->victim) != 0)
    remove_from_lookup_table(GET_ID(&fixture->victim));

  damage_trigger_free_prototypes();
  free(trig_index);

  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  mob_proto = fixture->saved_mob_proto;
  top_of_mobt = fixture->saved_top_of_mobt;
  trig_index = fixture->saved_trig_index;
  top_of_trigt = fixture->saved_top_of_trigt;
  trigger_list = fixture->saved_trigger_list;
}

static bool damage_trigger_attach(struct char_data *owner, const char *name, const char *flags,
                                  int chance, const char *body)
{
  struct trig_data *trigger;
  FILE *trigger_file;
  trig_rnum rnum;

  if (top_of_trigt >= DAMAGE_TRIGGER_TEST_CAPACITY)
    return false;

  trigger_file = tmpfile();
  if (trigger_file == NULL)
    return false;

  fprintf(trigger_file, "%s~\n", name);
  fprintf(trigger_file, "0 %s %d\n", flags, chance);
  fprintf(trigger_file, "~\n%s\n~\n", body);
  rewind(trigger_file);
  rnum = top_of_trigt;
  parse_trigger(trigger_file, 9100 + rnum);
  fclose(trigger_file);

  if (top_of_trigt != (int)rnum + 1 || trig_index[rnum] == NULL)
    return false;

  if (SCRIPT(owner) == NULL)
    SCRIPT(owner) = calloc(1, sizeof(*SCRIPT(owner)));
  if (SCRIPT(owner) == NULL)
    return false;

  trigger = read_trigger(rnum);
  if (trigger == NULL)
    return false;
  add_trigger(SCRIPT(owner), trigger, -1);
  return true;
}

static bool damage_trigger_fixture_add(struct damage_trigger_fixture *fixture, const char *name,
                                       const char *flags, int chance, const char *body)
{
  return damage_trigger_attach(&fixture->victim, name, flags, chance, body);
}

static struct char_data *damage_trigger_create_dynamic_npc(const char *name)
{
  struct char_data *ch;

  (void)name;
  ch = read_mobile(0, REAL);
  if (ch == NULL)
    return NULL;

  char_to_room(ch, 0);
  return ch;
}

static void damage_trigger_extract_dynamic_pair(struct char_data *actor, struct char_data *victim)
{
  if (!DEAD(actor))
    extract_char(actor);
  if (!DEAD(victim))
    extract_char(victim);
  extract_pending_chars();
}

static const char *damage_trigger_global(struct char_data *owner, const char *name)
{
  struct trig_var_data *variable;

  if (owner == NULL || SCRIPT(owner) == NULL)
    return NULL;

  for (variable = SCRIPT(owner)->global_vars; variable != NULL; variable = variable->next)
    if (strcmp(variable->name, name) == 0)
      return variable->value;

  return NULL;
}

static int damage_trigger_run_body(const char *body, int pending_damage)
{
  struct damage_trigger_fixture fixture;
  int result;

  if (!damage_trigger_fixture_begin(&fixture))
    return SCRIPT_ERROR_CODE;
  if (!damage_trigger_fixture_add(&fixture, "Damage result test", "u", 100, body))
  {
    damage_trigger_fixture_end(&fixture);
    return SCRIPT_ERROR_CODE;
  }

  result = damage_mtrigger(&fixture.actor, &fixture.victim, pending_damage, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  damage_trigger_fixture_end(&fixture);
  return result;
}

static void damage_trigger_reset_output(struct descriptor_data *descriptor)
{
  if (descriptor->output != NULL)
    comm_test_retain_unsent_output(descriptor, descriptor->output, descriptor->bufptr);
  descriptor->output = descriptor->small_outbuf;
  descriptor->small_outbuf[0] = '\0';
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

static bool damage_trigger_file_contains(const char *relative_path, const char *needle)
{
  const char *root;
  char line[MAX_STRING_LENGTH];
  char path[PATH_MAX];
  FILE *file;

  root = getenv("LUMINARI_TEST_ROOT");
  if (root == NULL || *root == '\0')
    root = ".";
  if (snprintf(path, sizeof(path), "%s/%s", root, relative_path) >= (int)sizeof(path))
    return false;
  file = fopen(path, "r");
  if (file == NULL)
    return false;

  while (fgets(line, sizeof(line), file) != NULL)
  {
    if (strstr(line, needle) != NULL)
    {
      fclose(file);
      return true;
    }
  }
  fclose(file);
  return false;
}

void Test_damage_trigger_olc_uses_attachment_specific_type_counts(CuTest *tc)
{
  struct descriptor_data descriptor;
  struct oasis_olc_data olc;
  struct trig_data trigger;
  struct char_data builder;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&olc, 0, sizeof(olc));
  memset(&trigger, 0, sizeof(trigger));
  damage_trigger_initialize_npc(&builder, "damage trigger builder");
  descriptor.character = &builder;
  descriptor.olc = &olc;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  builder.desc = &descriptor;
  olc.trig = &trigger;
  olc.mode = TRIGEDIT_TYPES;
  trigger.attach_type = MOB_TRIGGER;
  damage_trigger_reset_output(&descriptor);

  trigedit_parse(&descriptor, "21");
  CuAssertTrue(tc, IS_SET(GET_TRIG_TYPE(&trigger), MTRIG_DAMAGE));
  trigedit_parse(&descriptor, "21");
  CuAssertTrue(tc, !IS_SET(GET_TRIG_TYPE(&trigger), MTRIG_DAMAGE));

  trigger.attach_type = OBJ_TRIGGER;
  damage_trigger_reset_output(&descriptor);
  trigedit_parse(&descriptor, "21");
  CuAssertTrue(tc, !IS_SET(GET_TRIG_TYPE(&trigger), MTRIG_DAMAGE));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "20)"));
  CuAssertTrue(tc, strstr(descriptor.output, "21)") == NULL);

  trigger.attach_type = WLD_TRIGGER;
  damage_trigger_reset_output(&descriptor);
  trigedit_parse(&descriptor, "21");
  CuAssertTrue(tc, !IS_SET(GET_TRIG_TYPE(&trigger), MTRIG_DAMAGE));
  CuAssertPtrNotNull(tc, strstr(descriptor.output, "20)"));
  CuAssertTrue(tc, strstr(descriptor.output, "21)") == NULL);
  damage_trigger_reset_output(&descriptor);
  ProtocolDestroy(descriptor.pProtocol);
}

void Test_damage_trigger_olc_flag_serializes_and_reloads_as_u(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  struct descriptor_data descriptor;
  struct oasis_olc_data olc;
  struct trig_data trigger;
  struct char_data builder;
  char flags[MAX_INPUT_LENGTH] = {'\0'};
  bool started;
  bool added;

  memset(&descriptor, 0, sizeof(descriptor));
  memset(&olc, 0, sizeof(olc));
  memset(&trigger, 0, sizeof(trigger));
  damage_trigger_initialize_npc(&builder, "damage trigger builder");
  descriptor.character = &builder;
  descriptor.olc = &olc;
  descriptor.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, descriptor.pProtocol);
  if (descriptor.pProtocol == NULL)
    return;
  builder.desc = &descriptor;
  olc.trig = &trigger;
  olc.mode = TRIGEDIT_TYPES;
  trigger.attach_type = MOB_TRIGGER;
  damage_trigger_reset_output(&descriptor);

  trigedit_parse(&descriptor, "21");
  sprintascii(flags, GET_TRIG_TYPE(&trigger));
  started = damage_trigger_fixture_begin(&fixture);
  added = started &&
          damage_trigger_fixture_add(&fixture, "Damage round trip", flags, 100, "return %damage%");

  CuAssertStrEquals(tc, "u", flags);
  CuAssertTrue(tc, added);
  if (added)
    CuAssertTrue(tc, IS_SET(GET_TRIG_TYPE(TRIGGERS(SCRIPT(&fixture.victim))), MTRIG_DAMAGE));
  if (started)
    damage_trigger_fixture_end(&fixture);
  ProtocolDestroy(descriptor.pProtocol);
}

void Test_damage_trigger_chance_and_list_order_are_deterministic(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Never fires", "u", 0, "return 3"));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "First match", "u", 100, "return 7"));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Later match", "u", 100, "return 9"));

  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);

  CuAssertIntEquals(tc, 7, result);
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_default_and_return_contract(CuTest *tc)
{
  CuAssertIntEquals(tc, 17, damage_trigger_run_body("* logging only", 17));
  CuAssertIntEquals(tc, 17, damage_trigger_run_body("halt", 17));
  CuAssertIntEquals(tc, -1, damage_trigger_run_body("return -1", 17));
  CuAssertIntEquals(tc, 0, damage_trigger_run_body("return 0", 17));
  CuAssertIntEquals(tc, 1, damage_trigger_run_body("return 1", 17));
  CuAssertIntEquals(tc, 23, damage_trigger_run_body("return 23", 17));
  CuAssertIntEquals(tc, 5000, damage_trigger_run_body("return 5000", 17));
  CuAssertIntEquals(tc, -1, damage_trigger_run_body("return -2", 17));
  CuAssertIntEquals(tc, 17, test_damage_mtrigger_result(17, SCRIPT_ERROR_CODE, false));
}

void Test_damage_trigger_wait_result_is_synchronous(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  struct trig_data *trigger;
  unsigned long saved_pulse;
  long wait_pulses;
  int hit_points_after_damage;
  int result;

  event_init();
  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(
      tc, damage_trigger_fixture_add(&fixture, "Wait before return", "u", 100, "wait 1\nreturn 3"));
  trigger = TRIGGERS(SCRIPT(&fixture.victim));
  result = damage(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);
  hit_points_after_damage = GET_HIT(&fixture.victim);
  CuAssertIntEquals(tc, 17, result);
  CuAssertIntEquals(tc, 83, hit_points_after_damage);
  CuAssertPtrNotNull(tc, GET_TRIG_WAIT(trigger));

  saved_pulse = pulse;
  wait_pulses = event_time(GET_TRIG_WAIT(trigger));
  pulse += wait_pulses;
  event_process();
  CuAssertIntEquals(tc, hit_points_after_damage, GET_HIT(&fixture.victim));
  CuAssertTrue(tc, GET_TRIG_WAIT(trigger) == NULL);
  pulse = saved_pulse;
  damage_trigger_fixture_end(&fixture);

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Return before wait", "u", 100,
                                              "return 5\nwait 1\nreturn 2"));
  trigger = TRIGGERS(SCRIPT(&fixture.victim));
  result = damage(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);
  hit_points_after_damage = GET_HIT(&fixture.victim);
  CuAssertIntEquals(tc, 5, result);
  CuAssertIntEquals(tc, 95, hit_points_after_damage);

  saved_pulse = pulse;
  wait_pulses = event_time(GET_TRIG_WAIT(trigger));
  pulse += wait_pulses;
  event_process();
  CuAssertIntEquals(tc, hit_points_after_damage, GET_HIT(&fixture.victim));
  CuAssertTrue(tc, GET_TRIG_WAIT(trigger) == NULL);
  pulse = saved_pulse;
  damage_trigger_fixture_end(&fixture);
  event_free_all();
}

void Test_damage_trigger_scope_and_zero_damage_contract(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Scope test", "u", 100,
                                              "set fired 1\nglobal fired\nreturn 5"));

  result = damage_mtrigger(&fixture.actor, &fixture.victim, 0, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 0, result);
  CuAssertTrue(tc, damage_trigger_global(&fixture.victim, "fired") == NULL);

  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 5, result);
  CuAssertStrEquals(tc, "1", damage_trigger_global(&fixture.victim, "fired"));

  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);
  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 17, result);
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_CHARM);

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_ISNPC);
  fixture.victim.player_specials = &fixture.player_specials;
  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 17, result);
  SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_ISNPC);
  fixture.victim.player_specials = &dummy_mob;

  result = damage_mtrigger(&fixture.victim, &fixture.victim, 17, TYPE_SUFFERING, DAM_POISON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, 5, result);
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_exposes_stable_combat_metadata(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  const char *actor_id;
  const char *victim_id;
  const char *self_id;
  char expected[MAX_INPUT_LENGTH];
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Metadata test", "u", 100,
                                              "set seen_actor %actor.id%\n"
                                              "global seen_actor\n"
                                              "set seen_victim %victim.id%\n"
                                              "global seen_victim\n"
                                              "set seen_self %self.id%\n"
                                              "global seen_self\n"
                                              "set seen_damage %damage%\n"
                                              "global seen_damage\n"
                                              "set seen_legacy %attacktype%\n"
                                              "global seen_legacy\n"
                                              "set seen_attack_id %attackid%\n"
                                              "global seen_attack_id\n"
                                              "set seen_attack_name %attackname%\n"
                                              "global seen_attack_name\n"
                                              "set seen_damage_type %damagetype%\n"
                                              "global seen_damage_type\n"
                                              "set seen_damage_type_name %damagetypename%\n"
                                              "global seen_damage_type_name\n"
                                              "set seen_mode_id %attackmodeid%\n"
                                              "global seen_mode_id\n"
                                              "set seen_mode %attackmode%\n"
                                              "global seen_mode"));

  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_SLASH, DAM_FIRE,
                           ATTACK_TYPE_OFFHAND);
  actor_id = damage_trigger_global(&fixture.victim, "seen_actor");
  victim_id = damage_trigger_global(&fixture.victim, "seen_victim");
  self_id = damage_trigger_global(&fixture.victim, "seen_self");
  CuAssertIntEquals(tc, 17, result);
  CuAssertPtrNotNull(tc, actor_id);
  CuAssertPtrNotNull(tc, victim_id);
  CuAssertPtrNotNull(tc, self_id);
  CuAssertTrue(tc, strcmp(actor_id, victim_id) != 0);
  CuAssertStrEquals(tc, victim_id, self_id);
  CuAssertStrEquals(tc, "17", damage_trigger_global(&fixture.victim, "seen_damage"));
  CuAssertStrEquals(tc, "UNDEFINED", damage_trigger_global(&fixture.victim, "seen_legacy"));
  snprintf(expected, sizeof(expected), "%d", TYPE_SLASH);
  CuAssertStrEquals(tc, expected, damage_trigger_global(&fixture.victim, "seen_attack_id"));
  CuAssertStrEquals(tc, attack_hit_types[TYPE_SLASH - TYPE_HIT],
                    damage_trigger_global(&fixture.victim, "seen_attack_name"));
  snprintf(expected, sizeof(expected), "%d", DAM_FIRE);
  CuAssertStrEquals(tc, expected, damage_trigger_global(&fixture.victim, "seen_damage_type"));
  CuAssertStrEquals(tc, damtypes[DAM_FIRE],
                    damage_trigger_global(&fixture.victim, "seen_damage_type_name"));
  snprintf(expected, sizeof(expected), "%d", ATTACK_TYPE_OFFHAND);
  CuAssertStrEquals(tc, expected, damage_trigger_global(&fixture.victim, "seen_mode_id"));
  CuAssertStrEquals(tc, "Offhand", damage_trigger_global(&fixture.victim, "seen_mode"));

  damage_mtrigger(&fixture.actor, &fixture.victim, 17, SPELL_MAGIC_MISSILE, DAM_FORCE,
                  ATTACK_TYPE_RANGED);
  CuAssertStrEquals(tc, skill_name(SPELL_MAGIC_MISSILE),
                    damage_trigger_global(&fixture.victim, "seen_attack_name"));
  damage_mtrigger(&fixture.actor, &fixture.victim, 17, SKILL_BASH, DAM_FORCE, ATTACK_TYPE_PRIMARY);
  CuAssertStrEquals(tc, skill_name(SKILL_BASH),
                    damage_trigger_global(&fixture.victim, "seen_attack_name"));
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_detach_cancels_outer_damage(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Detach owner", "u", 100,
                                              "detach all %self.id%\nreturn 5"));
  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);

  CuAssertIntEquals(tc, -1, result);
  CuAssertTrue(tc, SCRIPT(&fixture.victim) == NULL);
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_owner_and_actor_purge_cancel_outer_damage(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  struct char_data *saved_character_list;
  struct char_data *actor;
  struct char_data *victim;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  saved_character_list = character_list;
  fixture.room.people = NULL;
  fixture.actor.next_in_room = NULL;
  actor = damage_trigger_create_dynamic_npc("damage purge actor");
  victim = damage_trigger_create_dynamic_npc("damage purge victim");
  CuAssertPtrNotNull(tc, actor);
  CuAssertPtrNotNull(tc, victim);
  CuAssertTrue(tc,
               damage_trigger_attach(victim, "Purge owner", "u", 100, "mpurge %self%\nreturn 5"));

  result = damage_mtrigger(actor, victim, 17, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, -1, result);
  CuAssertTrue(tc, DEAD(victim));
  damage_trigger_extract_dynamic_pair(actor, victim);
  CuAssertPtrEquals(tc, saved_character_list, character_list);
  damage_trigger_fixture_end(&fixture);

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  saved_character_list = character_list;
  fixture.room.people = NULL;
  fixture.actor.next_in_room = NULL;
  actor = damage_trigger_create_dynamic_npc("damage purge actor");
  victim = damage_trigger_create_dynamic_npc("damage purge victim");
  CuAssertPtrNotNull(tc, actor);
  CuAssertPtrNotNull(tc, victim);
  CuAssertTrue(tc,
               damage_trigger_attach(victim, "Purge actor", "u", 100, "mpurge %actor%\nreturn 5"));

  result = damage_mtrigger(actor, victim, 17, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, -1, result);
  CuAssertTrue(tc, DEAD(actor));
  damage_trigger_extract_dynamic_pair(actor, victim);
  CuAssertPtrEquals(tc, saved_character_list, character_list);
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_owner_and_actor_death_cancel_outer_damage(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Kill owner", "u", 100,
                                              "mdamage %self% 200\nreturn 5"));
  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, -1, result);
  CuAssertIntEquals(tc, POS_DEAD, GET_POS(&fixture.victim));
  damage_trigger_fixture_end(&fixture);

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Kill actor", "u", 100,
                                              "mdamage %actor% 200\nreturn 5"));
  result = damage_mtrigger(&fixture.actor, &fixture.victim, 17, TYPE_HIT, DAM_BLUDGEON,
                           ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, -1, result);
  CuAssertIntEquals(tc, POS_DEAD, GET_POS(&fixture.actor));
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_post_hook_mitigation_and_cap_still_apply(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(
                       &fixture, "Post mitigation", "u", 100,
                       "set seen_damage %damage%\nglobal seen_damage\nreturn %damage%"));
  GET_RESISTANCES(&fixture.victim, DAM_FIRE) = 50;
  result = damage(&fixture.actor, &fixture.victim, 20, TYPE_HIT, DAM_FIRE, ATTACK_TYPE_PRIMARY);
  CuAssertStrEquals(tc, "20", damage_trigger_global(&fixture.victim, "seen_damage"));
  CuAssertIntEquals(tc, 10, result);
  CuAssertIntEquals(tc, 90, GET_HIT(&fixture.victim));
  damage_trigger_fixture_end(&fixture);

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Damage cap", "u", 100, "return 5000"));
  GET_HIT(&fixture.victim) = 5000;
  GET_MAX_HIT(&fixture.victim) = 5000;
  result = damage(&fixture.actor, &fixture.victim, 20, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, DAMAGE_CAP, result);
  CuAssertIntEquals(tc, 5000 - DAMAGE_CAP, GET_HIT(&fixture.victim));
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_pre_hook_rejections_do_not_run_script(CuTest *tc)
{
  struct damage_trigger_fixture fixture;
  int result;

  CuAssertTrue(tc, damage_trigger_fixture_begin(&fixture));
  CuAssertTrue(tc, damage_trigger_fixture_add(&fixture, "Protected mob", "u", 100,
                                              "set fired 1\nglobal fired\nreturn 5"));
  SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_NOKILL);
  result = damage(&fixture.actor, &fixture.victim, 20, TYPE_HIT, DAM_BLUDGEON, ATTACK_TYPE_PRIMARY);

  CuAssertIntEquals(tc, 0, result);
  CuAssertTrue(tc, damage_trigger_global(&fixture.victim, "fired") == NULL);
  CuAssertIntEquals(tc, 100, GET_HIT(&fixture.victim));
  damage_trigger_fixture_end(&fixture);
}

void Test_damage_trigger_builder_documentation_sources_are_complete(CuTest *tc)
{
  CuAssertTrue(tc, damage_trigger_file_contains("lib/text/help/help.hlp",
                                                "MOB-DAMAGE-TRIGGER MTRIG-DAMAGE "
                                                "TRIGEDIT-MOB-DAMAGE"));
  CuAssertTrue(tc, damage_trigger_file_contains("lib/text/help/help.hlp", "21) Damage"));
  CuAssertTrue(tc, damage_trigger_file_contains("lib/text/help/help.hlp", "%attackmode%"));
  CuAssertTrue(tc, damage_trigger_file_contains("sql/components/help_dg_damage_trigger.sql",
                                                "No explicit return preserves"));
  CuAssertTrue(tc, damage_trigger_file_contains("sql/components/help_dg_damage_trigger.sql",
                                                "MTRIG-DAMAGE"));
  CuAssertTrue(tc, damage_trigger_file_contains("sql/components/verify_help_dg_damage_trigger.sql",
                                                "dg_damage_content"));
  CuAssertTrue(tc, damage_trigger_file_contains("docs/web/dg-scripts/trigger-types.html",
                                                "Damage is synchronous and pre-mitigation"));
  CuAssertTrue(tc, damage_trigger_file_contains("lib/world/minimal/0.trg", "0 u 100"));
  CuAssertTrue(tc, damage_trigger_file_contains("lib/world/minimal/0.mob", "T 1"));
}
