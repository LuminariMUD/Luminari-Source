/* Production-linked regressions for the reward API in src/rewards.c and the staff
 * award command, which lists the same award types. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/constants.h"
#include "../../src/act.h"
#include "../../src/handler.h"
#include "../../src/net/protocol.h"
#include "../../src/rewards.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static char reward_staff_name[] = "rewardstaff";
static char reward_target_name[] = "rewardtarget";
static char reward_npc_name[] = "rewardnpc";
static char reward_bidder_name[] = "rewardbidder";
static char reward_room_name[] = "reward room";

struct reward_actor
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
};

static void reward_reset_output(struct descriptor_data *descriptor)
{
  descriptor->small_outbuf[0] = '\0';
  descriptor->output = descriptor->small_outbuf;
  descriptor->bufptr = 0;
  descriptor->bufspace = SMALL_BUFSIZE - 1;
}

/** Build a level 10 warrior that is never saved; a connected actor also gets a
 * playing descriptor and an account. */
static int reward_actor_init(struct reward_actor *actor, char *name, int connected)
{
  memset(actor, 0, sizeof(*actor));
  actor->ch.player_specials = &actor->specials;
  actor->ch.player.name = name;
  GET_PFILEPOS(&actor->ch) = -1;
  IN_ROOM(&actor->ch) = NOWHERE;
  GET_LEVEL(&actor->ch) = 10;
  GET_CLASS(&actor->ch) = CLASS_WARRIOR;

  if (!connected)
    return TRUE;

  actor->ch.desc = &actor->descriptor;
  actor->descriptor.character = &actor->ch;
  actor->descriptor.account = &actor->account;
  STATE(&actor->descriptor) = CON_PLAYING;
  reward_reset_output(&actor->descriptor);
  actor->descriptor.pProtocol = ProtocolCreate();
  return actor->descriptor.pProtocol != NULL;
}

static void reward_actor_release(struct reward_actor *actor)
{
  if (actor->descriptor.pProtocol != NULL)
    ProtocolDestroy(actor->descriptor.pProtocol);
  actor->descriptor.pProtocol = NULL;
  actor->ch.desc = NULL;
}

/** Read the balance an award type changes, independently of src/rewards.c. */
static long reward_balance(struct char_data *ch, int type)
{
  switch (type)
  {
  case AWARD_EXPERIENCE:
    return GET_EXP(ch);
  case AWARD_QUEST_POINTS:
    return GET_QUESTPOINTS(ch);
  case AWARD_ACCOUNT_EXPERIENCE:
    return (ch->desc && ch->desc->account) ? ch->desc->account->experience : -1;
  case AWARD_GOLD:
    return GET_GOLD(ch);
  case AWARD_BANK_GOLD:
    return GET_BANK_GOLD(ch);
  case AWARD_SKILL_POINTS:
    return GET_TRAINS(ch);
  case AWARD_FEATS:
    return GET_FEAT_POINTS(ch);
  case AWARD_CLASS_FEATS:
    return GET_CLASS_FEATS(ch, GET_CLASS(ch));
  case AWARD_EPIC_FEATS:
    return GET_EPIC_FEAT_POINTS(ch);
  case AWARD_EPIC_CLASS_FEATS:
    return GET_EPIC_CLASS_FEATS(ch, GET_CLASS(ch));
  case AWARD_ABILITY_BOOSTS:
    return GET_BOOSTS(ch);
  default:
    return -1;
  }
}

/** A lit room that replaces the world while commands pass between actors. */
struct reward_room
{
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

/** Stand the actors in one room, where they can see each other and what lies there. */
static void reward_room_enter(struct reward_room *place, struct reward_actor **actors, int count)
{
  int i;

  memset(place, 0, sizeof(*place));
  place->saved_world = world;
  place->saved_top_of_world = top_of_world;
  place->room.number = 100;
  place->room.sector_type = SECT_INSIDE;
  place->room.name = reward_room_name;
  world = &place->room;
  top_of_world = 0;

  for (i = count - 1; i >= 0; i--)
  {
    IN_ROOM(&actors[i]->ch) = 0;
    GET_POS(&actors[i]->ch) = POS_STANDING;
    SET_BIT_AR(PRF_FLAGS(&actors[i]->ch), PRF_HOLYLIGHT);
    actors[i]->ch.next_in_room = place->room.people;
    place->room.people = &actors[i]->ch;
  }
}

/** Extract whatever the actors carry or left in the room, then restore the world. */
static void reward_room_leave(struct reward_room *place)
{
  struct char_data *ch;
  struct char_data *next_ch;

  for (ch = place->room.people; ch != NULL; ch = next_ch)
  {
    next_ch = ch->next_in_room;
    while (ch->carrying != NULL)
      extract_obj(ch->carrying);
    ch->next_in_room = NULL;
    IN_ROOM(ch) = NOWHERE;
  }
  place->room.people = NULL;
  while (place->room.contents != NULL)
    extract_obj(place->room.contents);
  world = place->saved_world;
  top_of_world = place->saved_top_of_world;
}

/** Coins a character carries as piles rather than in the purse. */
static int reward_pile_value(struct char_data *ch)
{
  struct obj_data *obj;
  int total = 0;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_MONEY)
      total += GET_OBJ_VAL(obj, 0);
  return total;
}

/* Every award type credits, debits, floors at zero, and sets through the central path,
 * reporting the change it applied.  The account fixture has no name, so its persistence
 * attempt logs an incomplete-account SYSERR. */
void Test_rewards_every_award_type_applies_exact_changes(CuTest *tc)
{
  struct reward_actor actor;
  int type;

  for (type = 0; type < NUM_AWARD_TYPES; type++)
  {
    CuAssertTrue(tc, reward_actor_init(&actor, reward_target_name, TRUE));

    CuAssertIntEquals(tc, 7, (int)award_points(&actor.ch, type, 7));
    CuAssertIntEquals(tc, 7, (int)reward_balance(&actor.ch, type));
    CuAssertIntEquals(tc, -3, (int)award_points(&actor.ch, type, -3));
    CuAssertIntEquals(tc, 4, (int)reward_balance(&actor.ch, type));
    CuAssertIntEquals(tc, -4, (int)award_points(&actor.ch, type, -10));
    CuAssertIntEquals(tc, 0, (int)reward_balance(&actor.ch, type));
    CuAssertIntEquals(tc, 0, (int)award_points(&actor.ch, type, -1));
    CuAssertIntEquals(tc, 12, (int)award_set_points(&actor.ch, type, 12));
    CuAssertIntEquals(tc, 12, (int)reward_balance(&actor.ch, type));
    CuAssertIntEquals(tc, -12, (int)award_set_points(&actor.ch, type, -5));
    CuAssertIntEquals(tc, 0, (int)reward_balance(&actor.ch, type));

    reward_actor_release(&actor);
  }
}

/* Balances stop at their limits without wrapping, and a balance loaded above a limit is
 * never lowered by a credit. */
void Test_rewards_balances_stop_at_their_limits(CuTest *tc)
{
  struct reward_actor actor;
  struct char_data *ch;
  int saw_gold_limit;
  int saw_bank_limit;

  CuAssertTrue(tc, reward_actor_init(&actor, reward_target_name, TRUE));
  ch = &actor.ch;

  GET_GOLD(ch) = MAX_GOLD - 10;
  CuAssertIntEquals(tc, 10, award_gold(&actor.ch, 1000));
  CuAssertIntEquals(tc, MAX_GOLD, GET_GOLD(&actor.ch));
  saw_gold_limit = strstr(actor.descriptor.output, "You have reached the maximum gold!") != NULL;
  CuAssertIntEquals(tc, -MAX_GOLD, award_gold(&actor.ch, INT_MIN));
  CuAssertIntEquals(tc, 0, GET_GOLD(&actor.ch));
  CuAssertTrue(tc, award_set_points(&actor.ch, AWARD_GOLD, 5000000000L) == MAX_GOLD);
  CuAssertIntEquals(tc, MAX_GOLD, GET_GOLD(&actor.ch));

  reward_reset_output(&actor.descriptor);
  GET_BANK_GOLD(&actor.ch) = MAX_BANK;
  CuAssertIntEquals(tc, 0, award_bank_gold(&actor.ch, 1));
  CuAssertIntEquals(tc, MAX_BANK, GET_BANK_GOLD(&actor.ch));
  saw_bank_limit =
      strstr(actor.descriptor.output, "You have reached the maximum bank balance!") != NULL;

  GET_QUESTPOINTS(&actor.ch) = MAX_QUEST_POINTS + 50;
  CuAssertIntEquals(tc, 0, award_quest_points(&actor.ch, 10));
  CuAssertIntEquals(tc, MAX_QUEST_POINTS + 50, GET_QUESTPOINTS(&actor.ch));
  CuAssertIntEquals(tc, -60, award_quest_points(&actor.ch, -60));
  CuAssertIntEquals(tc, MAX_QUEST_POINTS - 10, GET_QUESTPOINTS(&actor.ch));

  actor.account.experience = MAX_ACCOUNT_EXPERIENCE - 1;
  CuAssertIntEquals(tc, 1, award_account_experience(&actor.ch, 5));
  CuAssertIntEquals(tc, MAX_ACCOUNT_EXPERIENCE, actor.account.experience);

  GET_FEAT_POINTS(ch) = 120;
  CuAssertIntEquals(tc, SCHAR_MAX - 120, (int)award_points(&actor.ch, AWARD_FEATS, 1000));
  CuAssertIntEquals(tc, SCHAR_MAX, GET_FEAT_POINTS(ch));
  GET_EPIC_CLASS_FEATS(ch, CLASS_WARRIOR) = SCHAR_MAX;
  CuAssertIntEquals(tc, 0, (int)award_points(&actor.ch, AWARD_EPIC_CLASS_FEATS, 1));
  GET_BOOSTS(&actor.ch) = 250;
  CuAssertIntEquals(tc, 5, (int)award_points(&actor.ch, AWARD_ABILITY_BOOSTS, 300));
  CuAssertIntEquals(tc, UCHAR_MAX, GET_BOOSTS(&actor.ch));

  GET_EXP(&actor.ch) = LONG_MAX - 2;
  CuAssertIntEquals(tc, 2, (int)award_points(&actor.ch, AWARD_EXPERIENCE, 100));
  CuAssertTrue(tc, GET_EXP(&actor.ch) == LONG_MAX);

  reward_actor_release(&actor);

  CuAssertTrue(tc, saw_gold_limit);
  CuAssertTrue(tc, saw_bank_limit);
}

/* NULL characters, unknown types, NPC player balances, and missing accounts are
 * rejected without changing anything. */
void Test_rewards_reject_null_npc_and_unknown_targets(CuTest *tc)
{
  struct reward_actor actor;
  int type;
  int expected;

  CuAssertIntEquals(tc, 0, (int)award_points(NULL, AWARD_GOLD, 10));
  CuAssertIntEquals(tc, 0, (int)award_set_points(NULL, AWARD_GOLD, 10));
  CuAssertIntEquals(tc, 0, award_experience(NULL, 10, AWARD_EXP_MODE_QUEST));
  CuAssertIntEquals(tc, 0, award_experience_uncapped(NULL, 10, FALSE));
  CuAssertIntEquals(tc, 0, award_gold(NULL, 10));
  CuAssertIntEquals(tc, 0, award_bank_gold(NULL, 10));
  CuAssertIntEquals(tc, 0, award_quest_points(NULL, 10));
  CuAssertIntEquals(tc, 0, award_account_experience(NULL, 10));

  CuAssertTrue(tc, reward_actor_init(&actor, reward_target_name, FALSE));
  CuAssertIntEquals(tc, 0, (int)award_points(&actor.ch, -1, 10));
  CuAssertIntEquals(tc, 0, (int)award_points(&actor.ch, NUM_AWARD_TYPES, 10));
  CuAssertIntEquals(tc, 0, (int)award_set_points(&actor.ch, NUM_AWARD_TYPES, 10));
  CuAssertIntEquals(tc, 0, award_account_experience(&actor.ch, 5));
  GET_CLASS(&actor.ch) = NUM_CLASSES;
  CuAssertIntEquals(tc, 0, (int)award_points(&actor.ch, AWARD_CLASS_FEATS, 1));
  reward_actor_release(&actor);

  CuAssertTrue(tc, reward_actor_init(&actor, reward_npc_name, FALSE));
  SET_BIT_AR(MOB_FLAGS(&actor.ch), MOB_ISNPC);
  for (type = 0; type < NUM_AWARD_TYPES; type++)
  {
    expected = (type == AWARD_EXPERIENCE || type == AWARD_GOLD) ? 4 : 0;
    CuAssertIntEquals(tc, expected, (int)award_points(&actor.ch, type, 4));
  }
  CuAssertIntEquals(tc, 0, GET_QUESTPOINTS(&actor.ch));
  CuAssertIntEquals(tc, 50, award_experience(&actor.ch, 100, AWARD_EXP_MODE_SOLO));
  CuAssertTrue(tc, GET_EXP(&actor.ch) == 54);
  reward_actor_release(&actor);
}

/* A death loss returns, and tells the character, the experience actually taken, which is
 * what resurrection restores from the corpse. */
void Test_rewards_experience_loss_returns_the_amount_taken(CuTest *tc)
{
  struct reward_actor actor;
  int saved_max_exp_loss;
  int lost;
  int restored;
  int reported_loss;
  long after_loss;

  CuAssertTrue(tc, reward_actor_init(&actor, reward_target_name, TRUE));
  saved_max_exp_loss = CONFIG_MAX_EXP_LOSS;
  CONFIG_MAX_EXP_LOSS = 500000;
  GET_EXP(&actor.ch) = 1000;

  lost = award_experience(&actor.ch, -5000, AWARD_EXP_MODE_DEATH);
  after_loss = GET_EXP(&actor.ch);
  restored = award_experience_uncapped(&actor.ch, -lost, TRUE);
  reported_loss = strstr(actor.descriptor.output, "You lose 1000 experience points!\r\n") != NULL;

  CONFIG_MAX_EXP_LOSS = saved_max_exp_loss;
  reward_actor_release(&actor);

  CuAssertIntEquals(tc, -1000, lost);
  CuAssertTrue(tc, reported_loss);
  CuAssertTrue(tc, after_loss == 0);
  CuAssertIntEquals(tc, 1000, restored);
  CuAssertTrue(tc, GET_EXP(&actor.ch) == 1000);
}

/* The staff award command changes every award type on its connected target through the
 * central path, credits account experience to the target rather than the staff member,
 * reports the applied amount at a limit, and refuses targets that are not playing. */
void Test_rewards_staff_award_uses_the_central_path_for_every_type(CuTest *tc)
{
  struct room_data rooms[1];
  struct room_data *saved_world = world;
  room_rnum saved_top_of_world = top_of_world;
  struct reward_actor staff;
  struct reward_actor target;
  struct char_data *victim;
  char command[MAX_INPUT_LENGTH];
  int type;
  int awarded_every_type = TRUE;
  int reported_every_type = TRUE;
  int staff_account_experience;
  int target_account_experience;
  int feats_after_limit;
  int reported_limit;
  int gold_after_large_award;
  int reported_applied_gold;
  int quest_points_after_menu_award;
  int refused_menu_target;
  int quest_points_after_linkdead_award;
  int refused_linkdead_target;

  memset(rooms, 0, sizeof(rooms));
  rooms[0].number = 100;
  rooms[0].sector_type = SECT_INSIDE;
  rooms[0].name = reward_staff_name;
  world = rooms;
  top_of_world = 0;

  CuAssertTrue(tc, reward_actor_init(&staff, reward_staff_name, TRUE));
  CuAssertTrue(tc, reward_actor_init(&target, reward_target_name, TRUE));
  GET_LEVEL(&staff.ch) = LVL_IMPL;
  SET_BIT_AR(PRF_FLAGS(&staff.ch), PRF_HOLYLIGHT);
  IN_ROOM(&staff.ch) = 0;
  IN_ROOM(&target.ch) = 0;
  rooms[0].people = &target.ch;
  target.ch.next_in_room = &staff.ch;
  victim = &target.ch;

  for (type = 0; type < NUM_AWARD_TYPES; type++)
  {
    snprintf(command, sizeof(command), "%s %s 6", reward_target_name, award_types[type]);
    reward_reset_output(&staff.descriptor);
    reward_reset_output(&target.descriptor);
    do_award(&staff.ch, command, 0, 0);
    if (reward_balance(&target.ch, type) != 6)
      awarded_every_type = FALSE;
    if (strstr(staff.descriptor.output, "You have increased") == NULL)
      reported_every_type = FALSE;
  }
  staff_account_experience = staff.account.experience;
  target_account_experience = target.account.experience;

  GET_FEAT_POINTS(victim) = SCHAR_MAX;
  snprintf(command, sizeof(command), "%s feats 5", reward_target_name);
  reward_reset_output(&staff.descriptor);
  do_award(&staff.ch, command, 0, 0);
  feats_after_limit = GET_FEAT_POINTS(victim);
  reported_limit = strstr(staff.descriptor.output, "is already at its maximum") != NULL;

  snprintf(command, sizeof(command), "%s gold 9999999999", reward_target_name);
  reward_reset_output(&staff.descriptor);
  reward_reset_output(&target.descriptor);
  do_award(&staff.ch, command, 0, 0);
  gold_after_large_award = GET_GOLD(&target.ch);
  snprintf(command, sizeof(command), "by %d.", MAX_GOLD - 6);
  reported_applied_gold = strstr(staff.descriptor.output, command) != NULL;

  STATE(&target.descriptor) = CON_MENU;
  snprintf(command, sizeof(command), "%s quest-points 5", reward_target_name);
  reward_reset_output(&staff.descriptor);
  do_award(&staff.ch, command, 0, 0);
  quest_points_after_menu_award = GET_QUESTPOINTS(&target.ch);
  refused_menu_target =
      strstr(staff.descriptor.output, "You can only award online players") != NULL;

  STATE(&target.descriptor) = CON_PLAYING;
  target.ch.desc = NULL;
  reward_reset_output(&staff.descriptor);
  do_award(&staff.ch, command, 0, 0);
  quest_points_after_linkdead_award = GET_QUESTPOINTS(&target.ch);
  refused_linkdead_target =
      strstr(staff.descriptor.output, "You can only award online players") != NULL;

  world = saved_world;
  top_of_world = saved_top_of_world;
  reward_actor_release(&staff);
  reward_actor_release(&target);

  CuAssertTrue(tc, awarded_every_type);
  CuAssertTrue(tc, reported_every_type);
  CuAssertIntEquals(tc, 0, staff_account_experience);
  CuAssertIntEquals(tc, 6, target_account_experience);
  CuAssertIntEquals(tc, SCHAR_MAX, feats_after_limit);
  CuAssertTrue(tc, reported_limit);
  CuAssertIntEquals(tc, MAX_GOLD, gold_after_large_award);
  CuAssertTrue(tc, reported_applied_gold);
  CuAssertIntEquals(tc, 6, quest_points_after_menu_award);
  CuAssertTrue(tc, refused_menu_target);
  CuAssertIntEquals(tc, 6, quest_points_after_linkdead_award);
  CuAssertTrue(tc, refused_linkdead_target);
}

/* award_capacity() is the largest credit a balance accepts in full, so a transfer can be
 * refused before anything is taken. */
void Test_rewards_capacity_is_the_credit_a_balance_accepts(CuTest *tc)
{
  struct reward_actor actor;
  struct char_data *ch;

  CuAssertIntEquals(tc, 0, (int)award_capacity(NULL, AWARD_GOLD));

  CuAssertTrue(tc, reward_actor_init(&actor, reward_target_name, TRUE));
  ch = &actor.ch;

  CuAssertTrue(tc, award_capacity(ch, AWARD_GOLD) == MAX_GOLD);
  GET_GOLD(ch) = MAX_GOLD - 25;
  CuAssertIntEquals(tc, 25, (int)award_capacity(ch, AWARD_GOLD));
  CuAssertIntEquals(tc, 25, award_gold(ch, 25));
  CuAssertIntEquals(tc, 0, (int)award_capacity(ch, AWARD_GOLD));
  GET_GOLD(ch) = -40;
  CuAssertTrue(tc, award_capacity(ch, AWARD_GOLD) == MAX_GOLD);
  CuAssertIntEquals(tc, MAX_GOLD, award_gold(ch, MAX_GOLD));
  CuAssertIntEquals(tc, MAX_GOLD - 40, GET_GOLD(ch));

  GET_QUESTPOINTS(ch) = MAX_QUEST_POINTS + 1;
  CuAssertIntEquals(tc, 0, (int)award_capacity(ch, AWARD_QUEST_POINTS));
  actor.account.experience = MAX_ACCOUNT_EXPERIENCE - 7;
  CuAssertIntEquals(tc, 7, (int)award_capacity(ch, AWARD_ACCOUNT_EXPERIENCE));
  GET_BOOSTS(ch) = 250;
  CuAssertIntEquals(tc, 5, (int)award_capacity(ch, AWARD_ABILITY_BOOSTS));
  CuAssertIntEquals(tc, 0, (int)award_capacity(ch, NUM_AWARD_TYPES));
  ch->desc = NULL;
  CuAssertIntEquals(tc, 0, (int)award_capacity(ch, AWARD_ACCOUNT_EXPERIENCE));
  reward_actor_release(&actor);

  CuAssertTrue(tc, reward_actor_init(&actor, reward_npc_name, FALSE));
  SET_BIT_AR(MOB_FLAGS(&actor.ch), MOB_ISNPC);
  CuAssertIntEquals(tc, 0, (int)award_capacity(&actor.ch, AWARD_BANK_GOLD));
  CuAssertTrue(tc, award_capacity(&actor.ch, AWARD_GOLD) == MAX_GOLD);
  reward_actor_release(&actor);
}

/* give, split, and cexchange refuse, before taking anything, a credit the receiving balance
 * cannot hold, and move the exact amount when it fits. */
void Test_rewards_transfer_commands_refuse_credits_beyond_the_limit(CuTest *tc)
{
  struct reward_actor giver;
  struct reward_actor target;
  struct reward_actor *actors[2];
  struct reward_room place;
  struct group_data group;
  char give_command[] = "100 coins rewardtarget";
  char split_command[] = "100";
  char gold_exchange[] = "gold 10";
  char accexp_exchange[] = "accexp 10";
  int give_refused, give_moved, give_reported;
  int split_refused, split_moved, split_reported;
  int points_refused, points_exchanged, points_reported;
  int accexp_refused, accexp_reported;

  CuAssertTrue(tc, reward_actor_init(&giver, reward_staff_name, TRUE));
  CuAssertTrue(tc, reward_actor_init(&target, reward_target_name, TRUE));
  actors[0] = &giver;
  actors[1] = &target;
  reward_room_enter(&place, actors, 2);

  GET_GOLD(&giver.ch) = 1000;
  GET_GOLD(&target.ch) = MAX_GOLD - 10;
  do_give(&giver.ch, give_command, 0, 0);
  give_refused = GET_GOLD(&giver.ch) == 1000 && GET_GOLD(&target.ch) == MAX_GOLD - 10;
  give_reported = strstr(giver.descriptor.output, "cannot carry that many more coins") != NULL;
  GET_GOLD(&target.ch) = 0;
  do_give(&giver.ch, give_command, 0, 0);
  give_moved = GET_GOLD(&giver.ch) == 900 && GET_GOLD(&target.ch) == 100;

  memset(&group, 0, sizeof(group));
  group.members = create_list();
  add_to_list(&giver.ch, group.members);
  add_to_list(&target.ch, group.members);
  group.leader = &giver.ch;
  giver.ch.group = &group;
  target.ch.group = &group;
  GET_GOLD(&giver.ch) = 1000;
  GET_GOLD(&target.ch) = MAX_GOLD - 10;
  reward_reset_output(&giver.descriptor);
  do_split(&giver.ch, split_command, 0, 0);
  split_refused = GET_GOLD(&giver.ch) == 1000 && GET_GOLD(&target.ch) == MAX_GOLD - 10;
  split_reported = strstr(giver.descriptor.output, "cannot carry 50 more coins") != NULL;
  GET_GOLD(&target.ch) = 0;
  reward_reset_output(&giver.descriptor);
  reward_reset_output(&target.descriptor);
  do_split(&giver.ch, split_command, 0, 0);
  split_moved = GET_GOLD(&giver.ch) == 950 && GET_GOLD(&target.ch) == 50;
  giver.ch.group = NULL;
  target.ch.group = NULL;
  remove_from_list(&giver.ch, group.members);
  remove_from_list(&target.ch, group.members);
  free_list(group.members);

  GET_GOLD(&giver.ch) = 1000000;
  GET_QUESTPOINTS(&giver.ch) = MAX_QUEST_POINTS;
  reward_reset_output(&giver.descriptor);
  do_cexchange(&giver.ch, gold_exchange, 0, 0);
  points_refused = GET_GOLD(&giver.ch) == 1000000 && GET_QUESTPOINTS(&giver.ch) == MAX_QUEST_POINTS;
  points_reported = strstr(giver.descriptor.output, "Quest points cap at") != NULL;
  GET_QUESTPOINTS(&giver.ch) = MAX_QUEST_POINTS - 10;
  do_cexchange(&giver.ch, gold_exchange, 0, 0);
  points_exchanged =
      GET_GOLD(&giver.ch) == 1000000 - 3000 && GET_QUESTPOINTS(&giver.ch) == MAX_QUEST_POINTS;

  GET_GOLD(&giver.ch) = MAX_GOLD;
  giver.account.experience = 1000;
  reward_reset_output(&giver.descriptor);
  do_cexchange(&giver.ch, accexp_exchange, 0, 0);
  accexp_refused = giver.account.experience == 1000 && GET_GOLD(&giver.ch) == MAX_GOLD;
  accexp_reported = strstr(giver.descriptor.output, "cannot carry that much more gold") != NULL;

  reward_room_leave(&place);
  reward_actor_release(&giver);
  reward_actor_release(&target);

  CuAssertTrue(tc, give_refused);
  CuAssertTrue(tc, give_reported);
  CuAssertTrue(tc, give_moved);
  CuAssertTrue(tc, split_refused);
  CuAssertTrue(tc, split_reported);
  CuAssertTrue(tc, split_moved);
  CuAssertTrue(tc, points_refused);
  CuAssertTrue(tc, points_reported);
  CuAssertTrue(tc, points_exchanged);
  CuAssertTrue(tc, accexp_refused);
  CuAssertTrue(tc, accexp_reported);
}

/* Coins a purse cannot hold keep their value as a pile in the inventory: coins picked up, and
 * the auction refund and payout that would pass the limit. */
void Test_rewards_coins_beyond_the_limit_stay_as_a_pile(CuTest *tc)
{
  struct reward_actor seller;
  struct reward_actor outbid;
  struct reward_actor winner;
  struct reward_actor *actors[3];
  struct reward_room place;
  struct obj_data *coins;
  struct obj_data *relic;
  char get_command[] = "coins";
  char auction_command[] = "relic 100";
  char first_bid[] = "100";
  char winning_bid[] = "200";
  int round;
  int pickup_kept_pile, pickup_credited;
  int refund_kept_pile, payout_kept_pile, relic_delivered;

  CuAssertTrue(tc, reward_actor_init(&seller, reward_staff_name, FALSE));
  CuAssertTrue(tc, reward_actor_init(&outbid, reward_target_name, FALSE));
  CuAssertTrue(tc, reward_actor_init(&winner, reward_bidder_name, FALSE));
  actors[0] = &seller;
  actors[1] = &outbid;
  actors[2] = &winner;
  reward_room_enter(&place, actors, 3);

  GET_GOLD(&seller.ch) = MAX_GOLD - 30;
  coins = create_money(100);
  obj_to_room(coins, 0);
  do_get(&seller.ch, get_command, 0, 0);
  pickup_kept_pile = GET_GOLD(&seller.ch) == MAX_GOLD - 30 && coins->carried_by == &seller.ch &&
                     GET_OBJ_VAL(coins, 0) == 100;
  obj_from_char(coins);
  obj_to_room(coins, 0);
  GET_GOLD(&seller.ch) = 0;
  do_get(&seller.ch, get_command, 0, 0);
  pickup_credited =
      GET_GOLD(&seller.ch) == 100 && seller.ch.carrying == NULL && place.room.contents == NULL;

  relic = create_obj();
  relic->name = strdup("relic");
  relic->short_description = strdup("a relic");
  GET_OBJ_TYPE(relic) = ITEM_TREASURE;
  GET_OBJ_COST(relic) = 100;
  obj_to_char(relic, &seller.ch);
  GET_GOLD(&outbid.ch) = 1000;
  GET_GOLD(&winner.ch) = 1000;
  do_auction(&seller.ch, auction_command, 0, 0);
  do_bid(&outbid.ch, first_bid, 0, 0);
  GET_GOLD(&outbid.ch) = MAX_GOLD - 20;
  do_bid(&winner.ch, winning_bid, 0, 0);
  refund_kept_pile = GET_GOLD(&outbid.ch) == MAX_GOLD && reward_pile_value(&outbid.ch) == 80;
  GET_GOLD(&seller.ch) = MAX_GOLD - 70;
  for (round = 0; round < 4; round++)
    check_auction();
  payout_kept_pile = GET_GOLD(&seller.ch) == MAX_GOLD && reward_pile_value(&seller.ch) == 130;
  relic_delivered = relic->carried_by == &winner.ch && GET_GOLD(&winner.ch) == 800;

  reward_room_leave(&place);
  reward_actor_release(&seller);
  reward_actor_release(&outbid);
  reward_actor_release(&winner);

  CuAssertTrue(tc, pickup_kept_pile);
  CuAssertTrue(tc, pickup_credited);
  CuAssertTrue(tc, refund_kept_pile);
  CuAssertTrue(tc, payout_kept_pile);
  CuAssertTrue(tc, relic_delivered);
}
