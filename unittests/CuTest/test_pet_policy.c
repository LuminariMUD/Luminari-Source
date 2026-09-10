#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/backgrounds.h"
#include "../../src/net/protocol.h"

void Test_pet_admission_uses_existing_categories_without_materialization(CuTest *tc)
{
  struct char_data owner, pets[3], prototypes[3];
  struct player_special_data specials = {0};
  struct follow_type links[3] = {0};
  struct index_data indexes[3] = {0};
  struct char_data *saved_proto = mob_proto;
  struct index_data *saved_index = mob_index;
  struct room_data *saved_world = world;
  struct char_data *saved_characters = character_list;
  mob_rnum saved_top = top_of_mobt;
  mob_vnum vnums[3] = {MOB_DJINNI_KIND, MOB_DIRE_BADGER, RETAINER_MOB_VNUM};
  mob_vnum swap;
  int i, j;
  bool mixed, duplicate, summoner, general, mercenary, golem, invalid, unchanged;

  /* real_mobile() searches a sorted prototype index. No rooms are installed. */
  for (i = 0; i < 3; i++)
    for (j = i + 1; j < 3; j++)
      if (vnums[i] > vnums[j])
      {
        swap = vnums[i];
        vnums[i] = vnums[j];
        vnums[j] = swap;
      }
  clear_char(&owner);
  owner.player_specials = &specials;
  for (i = 0; i < 3; i++)
  {
    clear_char(&prototypes[i]);
    clear_char(&pets[i]);
    indexes[i].vnum = vnums[i];
    indexes[i].number = 7;
    SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_ISNPC);
    SET_BIT_AR(MOB_FLAGS(&pets[i]), MOB_ISNPC);
    SET_BIT_AR(AFF_FLAGS(&pets[i]), AFF_CHARM);
    GET_MOB_RNUM(&prototypes[i]) = i;
    GET_MOB_RNUM(&pets[i]) = i;
    pets[i].master = &owner;
    links[i].follower = &pets[i];
  }
  mob_proto = prototypes;
  mob_index = indexes;
  top_of_mobt = 2;
  world = NULL;

  owner.followers = &links[real_mobile(MOB_DJINNI_KIND)];
  mixed = can_add_follower(&owner, MOB_DIRE_BADGER) && can_add_follower(&owner, RETAINER_MOB_VNUM);
  duplicate = !can_add_follower(&owner, MOB_DJINNI_KIND);
  owner.followers = &links[real_mobile(MOB_DIRE_BADGER)];
  mixed = mixed && can_add_follower(&owner, MOB_DJINNI_KIND) &&
          can_add_follower(&owner, RETAINER_MOB_VNUM);
  duplicate = duplicate && !can_add_follower(&owner, MOB_DIRE_BADGER);
  CLASS_LEVEL((&owner), CLASS_SUMMONER) = 1;
  summoner = can_add_follower(&owner, MOB_DIRE_BADGER);
  owner.followers->next = &links[real_mobile(MOB_DJINNI_KIND)];
  summoner = summoner && can_add_follower(&owner, MOB_DIRE_BADGER);
  GET_MOB_RNUM(&pets[real_mobile(MOB_DJINNI_KIND)]) = real_mobile(MOB_DIRE_BADGER);
  summoner = summoner && !can_add_follower(&owner, MOB_DIRE_BADGER);

  i = real_mobile(RETAINER_MOB_VNUM);
  owner.followers = &links[i];
  general = !can_add_follower(&owner, RETAINER_MOB_VNUM);
  SET_BIT_AR(MOB_FLAGS(&pets[i]), MOB_MERCENARY);
  general = general && can_add_follower(&owner, RETAINER_MOB_VNUM);
  SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_MERCENARY);
  mercenary = !can_add_follower(&owner, RETAINER_MOB_VNUM);
  REMOVE_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_MERCENARY);
  SET_BIT_AR(MOB_FLAGS(&prototypes[i]), MOB_GOLEM);
  golem = can_add_follower(&owner, RETAINER_MOB_VNUM);
  REMOVE_BIT_AR(MOB_FLAGS(&pets[i]), MOB_MERCENARY);
  SET_BIT_AR(MOB_FLAGS(&pets[i]), MOB_GOLEM);
  golem = golem && !can_add_follower(&owner, RETAINER_MOB_VNUM);
  REMOVE_BIT_AR(AFF_FLAGS(&pets[i]), AFF_CHARM);
  general = general && can_add_follower(&owner, RETAINER_MOB_VNUM);
  links[i].follower = NULL;
  invalid = can_add_follower(&owner, RETAINER_MOB_VNUM) &&
            !can_add_follower(NULL, RETAINER_MOB_VNUM) && !can_add_follower(&owner, NOBODY);
  mob_proto = NULL;
  invalid = invalid && !can_add_follower(&owner, RETAINER_MOB_VNUM);
  mob_proto = prototypes;
  mob_index = NULL;
  invalid = invalid && !can_add_follower(&owner, RETAINER_MOB_VNUM);
  mob_index = indexes;
  top_of_mobt = NOBODY;
  invalid = invalid && !can_add_follower(&owner, RETAINER_MOB_VNUM);
  unchanged = character_list == saved_characters && world == NULL;
  for (i = 0; i < 3; i++)
    unchanged = unchanged && indexes[i].number == 7;

  mob_proto = saved_proto;
  mob_index = saved_index;
  top_of_mobt = saved_top;
  world = saved_world;
  CuAssertTrue(tc, mixed);
  CuAssertTrue(tc, duplicate);
  CuAssertTrue(tc, summoner);
  CuAssertTrue(tc, general);
  CuAssertTrue(tc, mercenary);
  CuAssertTrue(tc, golem);
  CuAssertTrue(tc, invalid);
  CuAssertTrue(tc, unchanged);
}

struct pet_policy_fixture
{
  struct char_data owner, pets[8], prototypes[4];
  struct player_special_data specials;
  struct follow_type links[8];
  struct index_data indexes[4];
  struct char_data *saved_proto;
  struct index_data *saved_index;
  struct room_data *saved_world;
  mob_rnum saved_top;
};

static void begin_pet_policy_fixture(struct pet_policy_fixture *fixture, int count)
{
  mob_vnum vnums[4] = {MOB_DJINNI_KIND, MOB_DIRE_WOLF, MOB_MUMMY, RETAINER_MOB_VNUM};
  mob_vnum swap;
  int i, j;

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_proto = mob_proto;
  fixture->saved_index = mob_index;
  fixture->saved_world = world;
  fixture->saved_top = top_of_mobt;
  for (i = 0; i < 4; i++)
    for (j = i + 1; j < 4; j++)
      if (vnums[i] > vnums[j])
      {
        swap = vnums[i];
        vnums[i] = vnums[j];
        vnums[j] = swap;
      }
  clear_char(&fixture->owner);
  fixture->owner.player_specials = &fixture->specials;
  GET_CHA(&fixture->owner) = 10;
  for (i = 0; i < 4; i++)
  {
    clear_char(&fixture->prototypes[i]);
    SET_BIT_AR(MOB_FLAGS(&fixture->prototypes[i]), MOB_ISNPC);
    GET_MOB_RNUM(&fixture->prototypes[i]) = i;
    fixture->indexes[i].vnum = vnums[i];
  }
  mob_proto = fixture->prototypes;
  mob_index = fixture->indexes;
  top_of_mobt = 3;
  world = NULL;
  for (i = 0; i < 8; i++)
  {
    clear_char(&fixture->pets[i]);
    SET_BIT_AR(MOB_FLAGS(&fixture->pets[i]), MOB_ISNPC);
    SET_BIT_AR(AFF_FLAGS(&fixture->pets[i]), AFF_CHARM);
    GET_MOB_RNUM(&fixture->pets[i]) = real_mobile(RETAINER_MOB_VNUM);
    fixture->pets[i].master = &fixture->owner;
    fixture->pets[i].player.short_descr = "companion";
    fixture->links[i].follower = &fixture->pets[i];
    if (i + 1 < count)
      fixture->links[i].next = &fixture->links[i + 1];
  }
  fixture->owner.followers = count > 0 ? fixture->links : NULL;
}

static void end_pet_policy_fixture(struct pet_policy_fixture *fixture)
{
  mob_proto = fixture->saved_proto;
  mob_index = fixture->saved_index;
  top_of_mobt = fixture->saved_top;
  world = fixture->saved_world;
}

void Test_pet_policy_general_admission_matches_charisma_and_status(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  bool room_for_third, full, reduced, unrelated;

  begin_pet_policy_fixture(&fixture, 2);
  GET_CHA(&fixture.owner) = 14;
  room_for_third = can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
                   check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 1;
  fixture.links[1].next = &fixture.links[2];
  full = !can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
         check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 0;
  GET_CHA(&fixture.owner) = 4;
  reduced = !can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
            check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 0;
  fixture.links[0].follower = NULL;
  fixture.pets[1].master = &fixture.pets[0];
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.pets[2]), AFF_CHARM);
  unrelated = check_npc_followers(&fixture.owner, NPC_MODE_COUNT, 0) == 0 &&
              check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 1 &&
              can_add_follower(&fixture.owner, RETAINER_MOB_VNUM);
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, room_for_third);
  CuAssertTrue(tc, full);
  CuAssertTrue(tc, reduced);
  CuAssertTrue(tc, unrelated);
}

void Test_pet_policy_source_flags_preserve_bonds_and_epic_allowances(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  bool independent, necromancer, exact_flags, invalid;
  mob_rnum mummy;

  begin_pet_policy_fixture(&fixture, 5);
  GET_MOB_RNUM(&fixture.pets[0]) = real_mobile(MOB_DIRE_WOLF);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[0]), MOB_C_ANIMAL);
  mummy = real_mobile(MOB_MUMMY);
  GET_MOB_RNUM(&fixture.pets[1]) = mummy;
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[1]), MOB_MUMMY_DUST);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[1]), MOB_ANIMATED_DEAD);
  GET_MOB_RNUM(&fixture.pets[2]) = mummy;
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[2]), MOB_ANIMATED_DEAD);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[3]), MOB_MERCENARY);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[4]), MOB_GOLEM);
  independent = can_add_follower(&fixture.owner, MOB_DIRE_WOLF) &&
                can_add_follower(&fixture.owner, MOB_DJINNI_KIND) &&
                can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
                !can_add_follower_by_flag(&fixture.owner, MOB_C_ANIMAL) &&
                !can_add_follower_by_flag(&fixture.owner, MOB_MUMMY_DUST) &&
                !can_add_follower_by_flag(&fixture.owner, MOB_MERCENARY) &&
                !can_add_follower_by_flag(&fixture.owner, MOB_GOLEM) &&
                !can_add_follower_by_flag(&fixture.owner, MOB_ANIMATED_DEAD) &&
                check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 1;
  CLASS_LEVEL((&fixture.owner), CLASS_NECROMANCER) = 1;
  SET_BIT_AR(MOB_FLAGS(&fixture.prototypes[mummy]), MOB_ANIMATED_DEAD);
  necromancer = can_add_follower_by_flag(&fixture.owner, MOB_ANIMATED_DEAD) &&
                can_add_follower(&fixture.owner, MOB_MUMMY);
  exact_flags = check_npc_followers(&fixture.owner, NPC_MODE_FLAG, MOB_ANIMATED_DEAD) == 2 &&
                check_npc_followers(&fixture.owner, NPC_MODE_SPECIFIC, MOB_MUMMY) == 2;
  invalid = !can_add_follower_by_flag(&fixture.owner, -1) &&
            !can_add_follower_by_flag(&fixture.owner, NUM_MOB_FLAGS) &&
            check_npc_followers(NULL, NPC_MODE_SPARE, 0) == 0;
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, independent);
  CuAssertTrue(tc, necromancer);
  CuAssertTrue(tc, exact_flags);
  CuAssertTrue(tc, invalid);
}

void Test_pet_policy_second_summon_requires_a_spare_general_slot(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  bool shared_full, extra_room, summon_full, first_dedicated;

  begin_pet_policy_fixture(&fixture, 2);
  CLASS_LEVEL((&fixture.owner), CLASS_SUMMONER) = 1;
  GET_MOB_RNUM(&fixture.pets[0]) = real_mobile(MOB_DIRE_WOLF);
  shared_full = !can_add_follower(&fixture.owner, MOB_DIRE_WOLF) &&
                check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 0;
  GET_CHA(&fixture.owner) = 14;
  extra_room = can_add_follower(&fixture.owner, MOB_DIRE_WOLF) &&
               check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 2;
  fixture.links[1].next = &fixture.links[2];
  GET_MOB_RNUM(&fixture.pets[2]) = real_mobile(MOB_DIRE_WOLF);
  summon_full = !can_add_follower(&fixture.owner, MOB_DIRE_WOLF) &&
                check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 1;
  fixture.owner.followers = &fixture.links[1];
  fixture.links[1].next = NULL;
  GET_CHA(&fixture.owner) = 10;
  first_dedicated = can_add_follower(&fixture.owner, MOB_DIRE_WOLF) &&
                    check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 0;
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, shared_full);
  CuAssertTrue(tc, extra_room);
  CuAssertTrue(tc, summon_full);
  CuAssertTrue(tc, first_dedicated);
}

void Test_pet_policy_display_handles_missing_rooms_and_reports_real_capacity(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  struct descriptor_data descriptor = {0};
  bool displayed;

  begin_pet_policy_fixture(&fixture, 2);
  fixture.links[1].follower = NULL;
  GET_CHA(&fixture.owner) = 14;
  descriptor.output = descriptor.small_outbuf;
  descriptor.bufspace = SMALL_BUFSIZE - 1;
  descriptor.character = &fixture.owner;
  descriptor.pProtocol = ProtocolCreate();
  if (descriptor.pProtocol == NULL)
  {
    end_pet_policy_fixture(&fixture);
    CuFail(tc, "could not initialize the pet status fixture");
    return;
  }
  fixture.owner.desc = &descriptor;
  displayed = check_npc_followers(&fixture.owner, NPC_MODE_DISPLAY, 0) == 1 &&
              strstr(descriptor.output, "Away") != NULL &&
              strstr(descriptor.output, "General slots: 1/3 used, 2 available") != NULL &&
              can_add_follower(&fixture.owner, RETAINER_MOB_VNUM);
  fixture.owner.desc = NULL;
  ProtocolDestroy(descriptor.pProtocol);
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, displayed);
}

void Test_pet_policy_existing_mobile_uses_live_source_flags(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  bool live_source, duplicate, invalid;

  begin_pet_policy_fixture(&fixture, 1);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[1]), MOB_GOLEM);
  live_source = !can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
                can_add_follower_mobile(&fixture.owner, &fixture.pets[1]);
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[0]), MOB_GOLEM);
  duplicate = !can_add_follower_mobile(&fixture.owner, &fixture.pets[1]) &&
              can_add_follower(&fixture.owner, RETAINER_MOB_VNUM);
  invalid = !can_add_follower_mobile(&fixture.owner, NULL) &&
            !can_add_follower_mobile(NULL, &fixture.pets[1]) &&
            !can_add_follower_mobile(&fixture.owner, &fixture.owner);
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, live_source);
  CuAssertTrue(tc, duplicate);
  CuAssertTrue(tc, invalid);
}

void Test_pet_policy_admits_whole_authored_batches_and_rejects_repeat_casts(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  bool whole, repeat, independent, bounded;

  begin_pet_policy_fixture(&fixture, 0);
  whole = can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_ELEMENTAL_SWARM, 8) &&
          can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_SHAMBLER, 6);
  bounded = !can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_ELEMENTAL_SWARM, 9) &&
            !can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_SHAMBLER, 7) &&
            !can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_SHAMBLER, 0);
  fixture.owner.followers = &fixture.links[0];
  fixture.pets[0].pet_source_spell = SPELL_ELEMENTAL_SWARM;
  repeat = !can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_ELEMENTAL_SWARM, 8);
  independent = can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_SHAMBLER, 6) &&
                check_npc_followers(&fixture.owner, NPC_MODE_SPARE, 0) == 1;
  fixture.pets[0].pet_source_spell = SPELL_SHAMBLER;
  repeat = repeat && !can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_SHAMBLER, 6);
  independent = independent &&
                can_add_summoned_followers(&fixture.owner, MOB_DIRE_WOLF, SPELL_ELEMENTAL_SWARM, 8);
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, whole);
  CuAssertTrue(tc, bounded);
  CuAssertTrue(tc, repeat);
  CuAssertTrue(tc, independent);
}

void Test_pet_policy_elite_undead_cost_two_control_points(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  mob_rnum mummy, lesser;
  bool lesser_allowed, elite_denied, two_elites, mixed_full, source_cost;

  begin_pet_policy_fixture(&fixture, 1);
  mummy = real_mobile(MOB_MUMMY);
  lesser = real_mobile(RETAINER_MOB_VNUM);
  SET_BIT_AR(MOB_FLAGS(&fixture.prototypes[mummy]), MOB_ANIMATED_DEAD);
  SET_BIT_AR(MOB_FLAGS(&fixture.prototypes[lesser]), MOB_ANIMATED_DEAD);
  GET_MOB_RNUM(&fixture.pets[0]) = lesser;
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[0]), MOB_ANIMATED_DEAD);
  lesser_allowed = can_add_follower(&fixture.owner, RETAINER_MOB_VNUM);
  elite_denied = !can_add_follower(&fixture.owner, MOB_MUMMY) &&
                 !can_add_summoned_followers(&fixture.owner, MOB_MUMMY, SPELL_ANIMATE_DEAD, 1);
  CLASS_LEVEL((&fixture.owner), CLASS_NECROMANCER) = 1;
  GET_MOB_RNUM(&fixture.pets[0]) = mummy;
  two_elites = can_add_follower(&fixture.owner, MOB_MUMMY);
  fixture.links[0].next = &fixture.links[1];
  GET_MOB_RNUM(&fixture.pets[1]) = mummy;
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[1]), MOB_ANIMATED_DEAD);
  two_elites = two_elites && !can_add_follower(&fixture.owner, MOB_MUMMY) &&
               !can_add_follower(&fixture.owner, RETAINER_MOB_VNUM);
  GET_MOB_RNUM(&fixture.pets[1]) = lesser;
  mixed_full = can_add_follower(&fixture.owner, RETAINER_MOB_VNUM) &&
               !can_add_follower(&fixture.owner, MOB_MUMMY);
  /* Source attribution also charges elite cost when prototype flags are absent. */
  REMOVE_BIT_AR(MOB_FLAGS(&fixture.pets[0]), MOB_ANIMATED_DEAD);
  fixture.pets[0].pet_source_spell = SPELL_ANIMATE_DEAD;
  source_cost = !can_add_summoned_followers(&fixture.owner, MOB_MUMMY, SPELL_ANIMATE_DEAD, 1);
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, lesser_allowed);
  CuAssertTrue(tc, elite_denied);
  CuAssertTrue(tc, two_elites);
  CuAssertTrue(tc, mixed_full);
  CuAssertTrue(tc, source_cost);
}

/* Bounded restore: a staged roster is admitted first-fit in the caller's
 * order, counted alongside live followers, and every rejection carries the
 * category and usage that denied it. */
void Test_pet_policy_staged_selection_is_deterministic_and_explains_denials(CuTest *tc)
{
  struct pet_policy_fixture fixture;
  struct char_data *staged[5];
  bool admitted[5];
  char reasons[5][64];
  int selected, i;
  bool first_fit, reasons_named, live_counted, reordered, invalid;

  /* One live general follower already uses the single Charisma slot. */
  begin_pet_policy_fixture(&fixture, 1);
  for (i = 0; i < 5; i++)
    staged[i] = &fixture.pets[i + 1];
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[2]), MOB_EIDOLON);
  GET_MOB_RNUM(&fixture.pets[3]) = real_mobile(MOB_DJINNI_KIND);
  fixture.pets[3].pet_source_spell = SPELL_DJINNI_KIND;
  SET_BIT_AR(MOB_FLAGS(&fixture.pets[4]), MOB_EIDOLON);
  memset(reasons, 0, sizeof(reasons));
  selected = select_restorable_followers(&fixture.owner, staged, 5, admitted, reasons[0],
                                         sizeof(reasons[0]));
  first_fit =
      selected == 2 && !admitted[0] && admitted[1] && admitted[2] && !admitted[3] && !admitted[4];
  reasons_named = strstr(reasons[0], "General") != NULL && strstr(reasons[0], "1/1") != NULL &&
                  strstr(reasons[3], "Eidolon") != NULL && strstr(reasons[3], "1/1") != NULL &&
                  strstr(reasons[4], "General") != NULL && reasons[1][0] == '\0' &&
                  reasons[2][0] == '\0';

  /* Without the live follower the first staged general pet takes the slot. */
  fixture.owner.followers = NULL;
  selected = select_restorable_followers(&fixture.owner, staged, 5, admitted, NULL, 0);
  live_counted =
      selected == 3 && admitted[0] && admitted[1] && admitted[2] && !admitted[3] && !admitted[4];

  /* Order is the priority: the same roster reversed admits the other eidolon. */
  staged[0] = &fixture.pets[5];
  staged[1] = &fixture.pets[4];
  staged[2] = &fixture.pets[3];
  staged[3] = &fixture.pets[2];
  staged[4] = &fixture.pets[1];
  selected = select_restorable_followers(&fixture.owner, staged, 5, admitted, NULL, 0);
  reordered =
      selected == 3 && admitted[0] && admitted[1] && admitted[2] && !admitted[3] && !admitted[4];

  staged[0] = NULL;
  staged[1] = &fixture.owner;
  selected = select_restorable_followers(&fixture.owner, staged, 2, admitted, reasons[0],
                                         sizeof(reasons[0]));
  invalid = selected == 0 && !admitted[0] && !admitted[1] && reasons[1][0] != '\0' &&
            select_restorable_followers(NULL, staged, 2, admitted, NULL, 0) == 0 &&
            select_restorable_followers(&fixture.owner, staged, 0, admitted, NULL, 0) == 0;
  end_pet_policy_fixture(&fixture);
  CuAssertTrue(tc, first_fit);
  CuAssertTrue(tc, reasons_named);
  CuAssertTrue(tc, live_counted);
  CuAssertTrue(tc, reordered);
  CuAssertTrue(tc, invalid);
}
