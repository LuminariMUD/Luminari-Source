/* Four arms (FEAT_FOUR_ARMS, issue #168, described in
 * docs/systems/GAME_MECHANICS_SYSTEMS.md): the second weapon pair and the
 * doubled limb slots, exercised through the production equip paths.
 * Capability sources, anatomy gate, hand budget, placement, lower armor
 * consumers and the restore cases; loss handling, deferral across provider
 * cycles, order-independent restoration; second-pair combat routing
 * (THIRD/FOURTH attacks). */
#include "CuTest.h"
#include <string.h>
#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/race.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/combat/fight.h"
#include "../../src/core/constants.h"
#include "../../src/dgscript/dg_scripts.h"
#include "../../src/events/actionqueues.h"
#include "../../src/events/mud_event.h"
#include "../../src/net/protocol.h"

struct four_arm_fixture
{
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;
  struct account_data account;
  struct room_data room;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
};

static void begin_four_arm_fixture(struct four_arm_fixture *fixture)
{
  if (race_list[RACE_WEMIC].type == NULL)
    assign_races();
  if (feat_list[FEAT_FOUR_ARMS].name == NULL ||
      !strcmp(feat_list[FEAT_FOUR_ARMS].name, "Unused Feat"))
    assign_feats();
  if (!IS_SET(weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags, WEAPON_FLAG_RANGED))
    load_weapons();
  if (armor_list[SPEC_ARMOR_TYPE_FULL_PLATE].armorCheck == 0)
    load_armor();

  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->room.number = 169920;
  fixture->room.people = &fixture->ch;
  world = &fixture->room;
  top_of_world = 0;
  fixture->ch.player_specials = &fixture->specials;
  fixture->ch.desc = &fixture->descriptor;
  fixture->descriptor.character = &fixture->ch;
  fixture->descriptor.account = &fixture->account;
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
  fixture->descriptor.pProtocol = ProtocolCreate();
  STATE(&fixture->descriptor) = CON_PLAYING;
  IN_ROOM(&fixture->ch) = 0;
  GET_REAL_RACE(&fixture->ch) = RACE_HUMAN;
  GET_REAL_SIZE(&fixture->ch) = SIZE_MEDIUM;
  fixture->ch.points.size = SIZE_MEDIUM;
  GET_LEVEL(&fixture->ch) = 10;
  GET_POS(&fixture->ch) = POS_STANDING;
  fixture->ch.player.name = CuMutableString("four arm tester");
}

static void end_four_arm_fixture(struct four_arm_fixture *fixture)
{
  int pos;

  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(&fixture->ch, pos))
      unequip_char(&fixture->ch, pos);
  while (fixture->ch.carrying)
    obj_from_char(fixture->ch.carrying);
  if (fixture->descriptor.pProtocol != NULL)
    ProtocolDestroy(fixture->descriptor.pProtocol);
  if (fixture->descriptor.large_outbuf != NULL)
  {
    free(fixture->descriptor.large_outbuf->text);
    free(fixture->descriptor.large_outbuf);
  }
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
}

static void reset_output(struct four_arm_fixture *fixture)
{
  fixture->descriptor.small_outbuf[0] = '\0';
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
  fixture->descriptor.bufptr = 0;
}

static void init_weapon(struct obj_data *obj, const char *name, int weapon_type_value, int size)
{
  clear_object(obj);
  obj->name = CuMutableString(name);
  obj->short_description = CuMutableString(name);
  obj->description = CuMutableString(name);
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  GET_OBJ_VAL(obj, 0) = weapon_type_value;
  GET_OBJ_SIZE(obj) = size;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);
}

static void init_armor(struct obj_data *obj, const char *name, int wear_flag, int ac,
                       int armor_family)
{
  clear_object(obj);
  obj->name = CuMutableString(name);
  obj->short_description = CuMutableString(name);
  obj->description = CuMutableString(name);
  GET_OBJ_TYPE(obj) = ITEM_ARMOR;
  GET_OBJ_SIZE(obj) = SIZE_MEDIUM;
  GET_OBJ_VAL(obj, 0) = ac;
  GET_OBJ_VAL(obj, 1) = armor_family;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), wear_flag);
}

static void grant_feat_on_object(struct obj_data *obj, int feat)
{
  obj->affected[0].location = APPLY_FEAT;
  obj->affected[0].modifier = feat;
}

/* wear through the command path: the object starts in inventory */
static void wear_from_inventory(struct four_arm_fixture *fixture, struct obj_data *obj, int where)
{
  obj_to_char(obj, &fixture->ch);
  reset_output(fixture);
  perform_wear(&fixture->ch, obj, where);
}

static int worn_position(struct char_data *ch, struct obj_data *obj)
{
  int pos;

  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(ch, pos) == obj)
      return pos;
  return -1;
}

/* Grant sources: the character's own feat, an APPLY_FEAT item in an ordinary
 * slot, and mob feats for NPCs.  An item in a four-arm slot never sustains
 * the arms it needs. */
void TestFourArmsPredicateSources(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, bracer;
  struct char_data mob;

  begin_four_arm_fixture(&fixture);
  CuAssertTrue(tc, !has_four_arms(NULL));
  CuAssertTrue(tc, !has_four_arms(&fixture.ch));

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertTrue(tc, has_four_arms(&fixture.ch));
  CuAssertIntEquals(tc, 1, HAS_FEAT(&fixture.ch, FEAT_FOUR_ARMS));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  CuAssertTrue(tc, !has_four_arms(&fixture.ch));

  /* an ordinary-slot item provides the arms */
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  CuAssertPtrEquals(tc, &ring, GET_EQ(&fixture.ch, WEAR_FINGER_R));
  CuAssertTrue(tc, has_four_arms(&fixture.ch));

  /* a second provider in a four-arm slot is not counted */
  init_armor(&bracer, "a four-armed bracer", ITEM_WEAR_WRIST, 0, 0);
  grant_feat_on_object(&bracer, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &bracer, WEAR_WRIST_R2);
  CuAssertPtrEquals(tc, &bracer, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  CuAssertTrue(tc, !has_four_arms(&fixture.ch));
  /* the ring was the only provider: the lower-wrist bracer is displaced */
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &fixture.ch, bracer.carried_by);

  /* NPCs use their mob feats */
  memset(&mob, 0, sizeof(mob));
  SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
  CuAssertTrue(tc, IS_NPC(&mob));
  CuAssertTrue(tc, !has_four_arms(&mob));
  MOB_HAS_FEAT(&mob, FEAT_FOUR_ARMS) = 1;
  CuAssertTrue(tc, has_four_arms(&mob));
  CuAssertTrue(tc, character_can_use_wear_slot(&mob, WEAR_WIELD_3));

  end_four_arm_fixture(&fixture);
}

/* Every four-arm slot is closed without the feat, on the command path and at
 * the shared equip boundary, and each doubled slot follows its base slot's
 * anatomy rules. */
void TestFourArmsSlotsNeedTheFeat(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sleeves;
  const int slots[] = {WEAR_WIELD_3, WEAR_WIELD_4,  WEAR_WIELD_2H_2, WEAR_ARMS_2,
                       WEAR_HANDS_2, WEAR_WRIST_R2, WEAR_WRIST_L2};
  size_t i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < sizeof(slots) / sizeof(slots[0]); i++)
  {
    CuAssertTrue(tc, is_four_arm_wear_slot(slots[i]));
    CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, slots[i]));
    CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, slots[i]));
  }
  CuAssertTrue(tc, !is_four_arm_wear_slot(WEAR_TAIL));
  CuAssertTrue(tc, !is_four_arm_wear_slot(WEAR_WIELD_2H));

  init_armor(&sleeves, "lower sleeves", ITEM_WEAR_ARMS, 3, SPEC_ARMOR_TYPE_LEATHER_ARMS);
  equip_char(&fixture.ch, &sleeves, WEAR_ARMS_2);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_ARMS_2));
  CuAssertPtrEquals(tc, &fixture.ch, sleeves.carried_by);
  obj_from_char(&sleeves);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  for (i = 0; i < sizeof(slots) / sizeof(slots[0]); i++)
    CuAssertTrue(tc, character_can_use_wear_slot(&fixture.ch, slots[i]));

  /* trelux anatomy has no hand slots: the doubled slot inherits that */
  GET_REAL_RACE(&fixture.ch) = RACE_TRELUX;
  CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, WEAR_HANDS));
  CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, WEAR_HANDS_2));
  CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, WEAR_WIELD_3));
  GET_REAL_RACE(&fixture.ch) = RACE_HUMAN;
  CuAssertTrue(tc, character_wear_slot_restriction(&fixture.ch, WEAR_HANDS_2) == NULL);

  end_four_arm_fixture(&fixture);
}

/* Two arms: the third one-hander is refused.  Four arms: four one-handers
 * fill both pairs in order, the fifth is refused, and removal holes refill. */
void TestFourArmsOneHandersFillBothPairs(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data swords[5];
  int i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 5; i++)
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);

  wear_from_inventory(&fixture, &swords[0], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &swords[1], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &swords[1], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));
  wear_from_inventory(&fixture, &swords[2], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &swords[2]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  obj_from_char(&swords[2]);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertIntEquals(tc, 2, hands_available(&fixture.ch));
  wear_from_inventory(&fixture, &swords[2], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &swords[3], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &swords[2], GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &swords[3], GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));
  CuAssertIntEquals(tc, 4, hands_used(&fixture.ch));

  wear_from_inventory(&fixture, &swords[4], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &swords[4]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));

  /* a hole in the first pair is filled before the second pair */
  CuAssertPtrEquals(tc, &swords[1], unequip_char(&fixture.ch, WEAR_WIELD_OFFHAND));
  wear_from_inventory(&fixture, &swords[4], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &swords[4], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));

  end_four_arm_fixture(&fixture);
}

/* Two-handers: one per pair, never beside a one-hander of the same pair, and
 * a one-hander skips a pair whose two-hand position is taken. */
void TestFourArmsTwoHandersUseWholePairs(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data big[3], sword[2];
  int i;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  for (i = 0; i < 3; i++)
    init_weapon(&big[i], "a test greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  for (i = 0; i < 2; i++)
    init_weapon(&sword[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  CuAssertIntEquals(tc, 2, hands_needed(&fixture.ch, &big[0]));

  wear_from_inventory(&fixture, &big[0], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &big[1], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &big[0], GET_EQ(&fixture.ch, WEAR_WIELD_2H));
  CuAssertPtrEquals(tc, &big[1], GET_EQ(&fixture.ch, WEAR_WIELD_2H_2));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));
  wear_from_inventory(&fixture, &big[2], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big[2]));
  obj_from_char(&big[2]);

  /* free the second pair: one-handers skip the first pair's two-hander */
  CuAssertPtrEquals(tc, &big[1], unequip_char(&fixture.ch, WEAR_WIELD_2H_2));
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &sword[1], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &sword[0], GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &sword[1], GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_1));

  /* one one-hander in each pair: no pair is free for a two-hander even
   * though two hands are free */
  CuAssertPtrEquals(tc, &big[0], unequip_char(&fixture.ch, WEAR_WIELD_2H));
  CuAssertPtrEquals(tc, &sword[1], unequip_char(&fixture.ch, WEAR_WIELD_4));
  wear_from_inventory(&fixture, &sword[1], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &sword[1], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertIntEquals(tc, 2, hands_available(&fixture.ch));
  wear_from_inventory(&fixture, &big[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big[0]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "pair of hands"));

  /* two-armed characters keep the old first-pair behaviour untouched */
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  CuAssertPtrEquals(tc, &sword[0], unequip_char(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &sword[1], unequip_char(&fixture.ch, WEAR_WIELD_1));
  wear_from_inventory(&fixture, &big[0], WEAR_WIELD_1);
  CuAssertPtrEquals(tc, &big[0], GET_EQ(&fixture.ch, WEAR_WIELD_2H));

  end_four_arm_fixture(&fixture);
}

/* The second pair is melee only: launchers are refused by the command path
 * and moved to inventory by equip_char(), and every wield slot still counts
 * for the one-launcher, no-mixing policy. */
void TestFourArmsSecondPairRejectsLaunchers(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data bow, sword[2];
  int i;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  init_weapon(&bow, "a test bow", WEAPON_TYPE_LONG_BOW, SIZE_MEDIUM);
  for (i = 0; i < 2; i++)
    init_weapon(&sword[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);

  CuAssertTrue(tc, second_pair_rejects_object(&bow, WEAR_WIELD_3));
  CuAssertTrue(tc, second_pair_rejects_object(&bow, WEAR_WIELD_2H_2));
  CuAssertTrue(tc, !second_pair_rejects_object(&bow, WEAR_WIELD_1));
  CuAssertTrue(tc, !second_pair_rejects_object(&sword[0], WEAR_WIELD_3));

  equip_char(&fixture.ch, &bow, WEAR_WIELD_3);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &fixture.ch, bow.carried_by);
  obj_from_char(&bow);

  /* melee in the first pair, then a launcher can only reach the second */
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &sword[1], WEAR_WIELD_1);
  CuAssertIntEquals(tc, ITEM_WEAPON, is_wielding_type(&fixture.ch));
  wear_from_inventory(&fixture, &bow, WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bow));
  obj_from_char(&bow);

  /* a weapon only in the second pair is still "wielding" for the policy */
  CuAssertPtrEquals(tc, &sword[0], unequip_char(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &sword[1], unequip_char(&fixture.ch, WEAR_WIELD_OFFHAND));
  equip_char(&fixture.ch, &sword[0], WEAR_WIELD_4);
  CuAssertPtrEquals(tc, &sword[0], GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertIntEquals(tc, ITEM_WEAPON, is_wielding_type(&fixture.ch));
  CuAssertIntEquals(tc, 1, hands_used(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* Sleeves, gloves and wrists overflow to the lower limbs with four arms and
 * nowhere without them; lower sleeves join the armor consumers. */
void TestFourArmsLowerLimbSlotsAndArmorConsumers(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sleeves[2], gloves[2], wrists[5], suit;
  int i, base_penalty;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 2; i++)
  {
    init_armor(&sleeves[i], "test sleeves", ITEM_WEAR_ARMS, 4, SPEC_ARMOR_TYPE_FULL_PLATE_ARMS);
    init_armor(&gloves[i], "test gloves", ITEM_WEAR_HANDS, 0, 0);
  }
  for (i = 0; i < 5; i++)
    init_armor(&wrists[i], "a test bracer", ITEM_WEAR_WRIST, 0, 0);

  /* two arms: the second set has nowhere to go */
  wear_from_inventory(&fixture, &sleeves[0], WEAR_ARMS);
  wear_from_inventory(&fixture, &sleeves[1], WEAR_ARMS);
  CuAssertPtrEquals(tc, &sleeves[0], GET_EQ(&fixture.ch, WEAR_ARMS));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sleeves[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "already wearing"));
  obj_from_char(&sleeves[1]);
  CuAssertIntEquals(tc, 4, fixture.ch.points.armor);
  base_penalty = compute_gear_armor_penalty(&fixture.ch);
  CuAssertTrue(tc, base_penalty != 0);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  wear_from_inventory(&fixture, &sleeves[1], WEAR_ARMS);
  CuAssertPtrEquals(tc, &sleeves[1], GET_EQ(&fixture.ch, WEAR_ARMS_2));
  CuAssertIntEquals(tc, 8, fixture.ch.points.armor);
  /* two identical heavy pieces average to the same penalty, one piece more */
  CuAssertIntEquals(tc, base_penalty, compute_gear_armor_penalty(&fixture.ch));
  CuAssertTrue(tc, compute_gear_spell_failure(&fixture.ch) > 0);
  CuAssertTrue(tc, !is_proficient_with_sleeves(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_ARMOR_PROFICIENCY_HEAVY, 1);
  CuAssertTrue(tc, is_proficient_with_sleeves(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_ARMOR_PROFICIENCY_HEAVY, 0);

  wear_from_inventory(&fixture, &gloves[0], WEAR_HANDS);
  wear_from_inventory(&fixture, &gloves[1], WEAR_HANDS);
  CuAssertPtrEquals(tc, &gloves[0], GET_EQ(&fixture.ch, WEAR_HANDS));
  CuAssertPtrEquals(tc, &gloves[1], GET_EQ(&fixture.ch, WEAR_HANDS_2));

  for (i = 0; i < 5; i++)
    wear_from_inventory(&fixture, &wrists[i], WEAR_WRIST_R);
  CuAssertPtrEquals(tc, &wrists[0], GET_EQ(&fixture.ch, WEAR_WRIST_R));
  CuAssertPtrEquals(tc, &wrists[1], GET_EQ(&fixture.ch, WEAR_WRIST_L));
  CuAssertPtrEquals(tc, &wrists[2], GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &wrists[3], GET_EQ(&fixture.ch, WEAR_WRIST_L2));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &wrists[4]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "four of your wrists"));
  obj_from_char(&wrists[4]);

  /* whole-body armor conflicts with the lower sleeves in both orders */
  init_armor(&suit, "a whole-body suit", ITEM_WEAR_BODY, 5, SPEC_ARMOR_TYPE_LEATHER);
  SET_BIT_AR(GET_OBJ_EXTRA(&suit), ITEM_ROL_WHOLE_BODY);
  CuAssertPtrEquals(tc, &sleeves[0], unequip_char(&fixture.ch, WEAR_ARMS));
  CuAssertTrue(tc, rol_object_wear_conflicts(&fixture.ch, &suit, WEAR_BODY));
  CuAssertPtrEquals(tc, &sleeves[1], unequip_char(&fixture.ch, WEAR_ARMS_2));
  CuAssertTrue(tc, !rol_object_wear_conflicts(&fixture.ch, &suit, WEAR_BODY));
  equip_char(&fixture.ch, &suit, WEAR_BODY);
  CuAssertTrue(tc, rol_object_wear_conflicts(&fixture.ch, &sleeves[1], WEAR_ARMS_2));

  end_four_arm_fixture(&fixture);
}

/* Saved positions 44..50 restore with the feat and fall back to inventory
 * without it; the old tail position is unchanged. */
void TestFourArmsAutoEquipRestoresSlots(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword, bracer;

  begin_four_arm_fixture(&fixture);
  init_weapon(&sword, "a saved sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_armor(&bracer, "a saved bracer", ITEM_WEAR_WRIST, 0, 0);

  test_auto_equip_loaded_object(&fixture.ch, &sword, WEAR_WIELD_3 + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &fixture.ch, sword.carried_by);
  obj_from_char(&sword);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  test_auto_equip_loaded_object(&fixture.ch, &sword, WEAR_WIELD_3 + 1);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertIntEquals(tc, WEAR_WIELD_3, sword.worn_on);
  test_auto_equip_loaded_object(&fixture.ch, &bracer, WEAR_WRIST_L2 + 1);
  CuAssertPtrEquals(tc, &bracer, GET_EQ(&fixture.ch, WEAR_WRIST_L2));

  /* the wrong wear flag still falls back to inventory */
  CuAssertPtrEquals(tc, &bracer, unequip_char(&fixture.ch, WEAR_WRIST_L2));
  test_auto_equip_loaded_object(&fixture.ch, &bracer, WEAR_HANDS_2 + 1);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HANDS_2));
  CuAssertPtrEquals(tc, &fixture.ch, bracer.carried_by);

  end_four_arm_fixture(&fixture);
}

/* Every position is listed exactly once in the equipment order and every
 * new slot has a display label. */
void TestFourArmsSlotTablesAreComplete(CuTest *tc)
{
  int seen[NUM_WEARS] = {0};
  int i;

  CuAssertIntEquals(tc, 51, NUM_WEARS);
  CuAssertIntEquals(tc, WEAR_TAIL + 1, WEAR_WIELD_3);
  CuAssertIntEquals(tc, NUM_WEARS - 1, WEAR_WRIST_L2);
  for (i = 0; i < NUM_WEARS; i++)
  {
    CuAssertTrue(tc, eq_ordering_1[i] >= 0 && eq_ordering_1[i] < NUM_WEARS);
    seen[eq_ordering_1[i]]++;
  }
  for (i = 0; i < NUM_WEARS; i++)
  {
    CuAssertIntEquals(tc, 1, seen[i]);
    CuAssertPtrNotNull(tc, wear_where[i]);
    CuAssertTrue(tc, strlen(wear_where[i]) > 0);
    CuAssertTrue(tc, strcmp(equipment_types[i], "\n") != 0);
  }
  CuAssertStrEquals(tc, "\n", equipment_types[NUM_WEARS]);
  CuAssertIntEquals(tc, WEAR_ARMS, four_arm_slot_base(WEAR_ARMS_2));
  CuAssertIntEquals(tc, WEAR_WIELD_2H, four_arm_slot_base(WEAR_WIELD_2H_2));
  CuAssertIntEquals(tc, WEAR_BODY, four_arm_slot_base(WEAR_BODY));
}

static void init_held(struct obj_data *obj, const char *name)
{
  clear_object(obj);
  obj->name = CuMutableString(name);
  obj->short_description = CuMutableString(name);
  obj->description = CuMutableString(name);
  GET_OBJ_TYPE(obj) = ITEM_OTHER;
  GET_OBJ_SIZE(obj) = SIZE_MEDIUM;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
}

static int count_carried(struct char_data *ch)
{
  struct obj_data *obj;
  int count = 0;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    count++;
  return count;
}

/* Removing the last provider closes the seven slots: their gear moves to
 * inventory (not the room), the first pair stays, repeated checks are stable. */
void TestFourArmsLossClosesExtraSlots(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, swords[4], sleeves[2], wrists[4];
  const int extra[] = {WEAR_WIELD_3, WEAR_WIELD_4, WEAR_ARMS_2, WEAR_WRIST_R2, WEAR_WRIST_L2};
  size_t i;

  begin_four_arm_fixture(&fixture);
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  wear_from_inventory(&fixture, &ring, WEAR_FINGER_R);
  CuAssertTrue(tc, has_four_arms(&fixture.ch));
  for (i = 0; i < 4; i++)
  {
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
    wear_from_inventory(&fixture, &swords[i], WEAR_WIELD_1);
    init_armor(&wrists[i], "a test bracer", ITEM_WEAR_WRIST, 0, 0);
    wear_from_inventory(&fixture, &wrists[i], WEAR_WRIST_R);
  }
  for (i = 0; i < 2; i++)
  {
    init_armor(&sleeves[i], "test sleeves", ITEM_WEAR_ARMS, 2, SPEC_ARMOR_TYPE_LEATHER_ARMS);
    wear_from_inventory(&fixture, &sleeves[i], WEAR_ARMS);
  }
  CuAssertPtrEquals(tc, &swords[3], GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertPtrEquals(tc, &sleeves[1], GET_EQ(&fixture.ch, WEAR_ARMS_2));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  CuAssertIntEquals(tc, 4, fixture.ch.points.armor);

  reset_output(&fixture);
  perform_remove(&fixture.ch, WEAR_FINGER_R, FALSE);
  CuAssertTrue(tc, !has_four_arms(&fixture.ch));
  for (i = 0; i < sizeof(extra) / sizeof(extra[0]); i++)
    CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, extra[i]));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &swords[1], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, &sleeves[0], GET_EQ(&fixture.ch, WEAR_ARMS));
  CuAssertPtrEquals(tc, &wrists[1], GET_EQ(&fixture.ch, WEAR_WRIST_L));
  /* ring plus five displaced items, every one carried, none in the room */
  CuAssertIntEquals(tc, 6, count_carried(&fixture.ch));
  CuAssertPtrEquals(tc, &fixture.ch, swords[2].carried_by);
  CuAssertPtrEquals(tc, &fixture.ch, sleeves[1].carried_by);
  CuAssertPtrEquals(tc, NULL, fixture.room.contents);
  CuAssertIntEquals(tc, 2, fixture.ch.points.armor);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "no longer keep hold"));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));

  /* stable under repeated recomputation */
  affect_total(&fixture.ch);
  four_arms_reconcile(&fixture.ch);
  CuAssertIntEquals(tc, 6, count_carried(&fixture.ch));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));

  end_four_arm_fixture(&fixture);
}

/* Four arms can hold two weapons and two held items in old positions alone;
 * losing the feat trims held items first and keeps both weapons. */
void TestFourArmsLossTrimsOldPositionsToCapacity(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data swords[2], held[2];
  int i;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  for (i = 0; i < 2; i++)
  {
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
    wear_from_inventory(&fixture, &swords[i], WEAR_WIELD_1);
    init_held(&held[i], "a test orb");
    wear_from_inventory(&fixture, &held[i], WEAR_HOLD_1);
  }
  CuAssertPtrEquals(tc, &held[0], GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, &held[1], GET_EQ(&fixture.ch, WEAR_HOLD_2));
  CuAssertIntEquals(tc, 4, hands_used(&fixture.ch));

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  reset_output(&fixture);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &swords[1], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_2));
  CuAssertIntEquals(tc, 2, count_carried(&fixture.ch));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* A provider cycle inside a deferral (save_char's unequip/re-equip) keeps
 * item-supported gear; the loss is acted on only when the deferral ends. */
void TestFourArmsDeferralSpansProviderCycle(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, sword;

  begin_four_arm_fixture(&fixture);
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  init_weapon(&sword, "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));

  four_arms_defer_begin(&fixture.ch);
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  CuAssertTrue(tc, !has_four_arms(&fixture.ch));
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  four_arms_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));

  /* nested deferral; the real loss lands when the outermost one ends */
  four_arms_defer_begin(&fixture.ch);
  four_arms_defer_begin(&fixture.ch);
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  four_arms_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  four_arms_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &fixture.ch, sword.carried_by);

  /* an affect batch also holds the change until it closes */
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  obj_from_char(&sword);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  affect_batch_begin(&fixture.ch);
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  affect_batch_end(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));

  end_four_arm_fixture(&fixture);
}

/* A remove trigger's veto cannot keep gear in a slot the body no longer has. */
void TestFourArmsLossIgnoresRemoveTriggerVeto(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword;
  struct index_data **saved_trig_index = trig_index;
  struct trig_data *saved_trigger_list = trigger_list;
  struct index_data *prototype_index;
  struct cmdlist_element *commands, *next_command;
  int saved_top_of_trigt = top_of_trigt;
  FILE *trigger_file;

  begin_four_arm_fixture(&fixture);
  trig_index = calloc(1, sizeof(*trig_index));
  top_of_trigt = 0;
  trigger_file = tmpfile();
  CuAssertPtrNotNull(tc, trig_index);
  CuAssertPtrNotNull(tc, trigger_file);
  fprintf(trigger_file, "Four arms remove veto~\n");
  fprintf(trigger_file, "%d %d 100\n", OBJ_TRIGGER, OTRIG_REMOVE);
  fprintf(trigger_file, "~\n");
  fprintf(trigger_file, "return 0\n");
  fprintf(trigger_file, "~\n");
  CuAssertTrue(tc, rewind_stream(trigger_file));
  parse_trigger(trigger_file, 9001);
  fclose(trigger_file);
  CuAssertIntEquals(tc, 1, top_of_trigt);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  init_weapon(&sword, "a clingy sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  sword.script = calloc(1, sizeof(*sword.script));
  add_trigger(sword.script, read_trigger(0), -1);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));

  /* the ordinary command honours the veto */
  perform_remove(&fixture.ch, WEAR_WIELD_3, FALSE);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));

  /* losing the arms does not */
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &fixture.ch, sword.carried_by);

  obj_from_char(&sword);
  extract_script(&sword.script);
  prototype_index = trig_index[0];
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
  free(trig_index);
  trig_index = saved_trig_index;
  top_of_trigt = saved_top_of_trigt;
  trigger_list = saved_trigger_list;
  end_four_arm_fixture(&fixture);
}

static void extract_everything(struct char_data *ch)
{
  int pos;

  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(ch, pos))
      extract_obj(GET_EQ(ch, pos));
  while (ch->carrying)
    extract_obj(ch->carrying);
}

/* Flat-file round trip: a provider recorded after the gear that depends on it
 * still restores that gear into its saved slot, with contents; without any
 * provider the gear stays in inventory with its contents. */
void TestFourArmsRestoreIsOrderIndependent(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, sword, pouch, coin, bracer;
  struct obj_data *loaded;
  obj_save_data *records;
  FILE *file;

  begin_four_arm_fixture(&fixture);
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  init_weapon(&sword, "a saved sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_held(&pouch, "a wielded pouch");
  GET_OBJ_TYPE(&pouch) = ITEM_CONTAINER;
  GET_OBJ_VAL(&pouch, 0) = 50;
  SET_BIT_AR(GET_OBJ_WEAR(&pouch), ITEM_WEAR_WIELD);
  init_held(&coin, "a saved coin");
  init_armor(&bracer, "a saved bracer", ITEM_WEAR_WRIST, 0, 0);

  /* dependents first, provider last; the pouch's content precedes it */
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, test_objsave_save_obj_record(&sword, &fixture.ch, file, WEAR_WIELD_3 + 1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&coin, &fixture.ch, file, -1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&pouch, &fixture.ch, file, WEAR_WIELD_4 + 1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&bracer, &fixture.ch, file, WEAR_WRIST_R2 + 1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&ring, &fixture.ch, file, WEAR_FINGER_R + 1));
  fputs("$~\n", file);
  CuAssertTrue(tc, rewind_stream(file));
  records = objsave_parse_objects(file);
  fclose(file);
  CuAssertPtrNotNull(tc, records);
  CuAssertIntEquals(tc, 5, test_restore_loaded_objects(&fixture.ch, records));

  CuAssertTrue(tc, has_four_arms(&fixture.ch));
  CuAssertPtrNotNull(tc, GET_EQ(&fixture.ch, WEAR_FINGER_R));
  loaded = GET_EQ(&fixture.ch, WEAR_WIELD_3);
  CuAssertPtrNotNull(tc, loaded);
  CuAssertStrEquals(tc, "a saved sword", loaded->short_description);
  loaded = GET_EQ(&fixture.ch, WEAR_WIELD_4);
  CuAssertPtrNotNull(tc, loaded);
  CuAssertStrEquals(tc, "a wielded pouch", loaded->short_description);
  CuAssertPtrNotNull(tc, loaded->contains);
  CuAssertStrEquals(tc, "a saved coin", loaded->contains->short_description);
  CuAssertPtrNotNull(tc, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  CuAssertIntEquals(tc, 0, GET_EQ(&fixture.ch, WEAR_WIELD_4)->four_arms_restore_slot);
  extract_everything(&fixture.ch);

  /* no provider at all: the gear waits in inventory, contents intact */
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, test_objsave_save_obj_record(&coin, &fixture.ch, file, -1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&pouch, &fixture.ch, file, WEAR_WIELD_4 + 1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&sword, &fixture.ch, file, WEAR_WIELD_3 + 1));
  fputs("$~\n", file);
  CuAssertTrue(tc, rewind_stream(file));
  records = objsave_parse_objects(file);
  fclose(file);
  CuAssertPtrNotNull(tc, records);
  CuAssertIntEquals(tc, 3, test_restore_loaded_objects(&fixture.ch, records));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertIntEquals(tc, 2, count_carried(&fixture.ch));
  for (loaded = fixture.ch.carrying; loaded != NULL; loaded = loaded->next_content)
  {
    CuAssertIntEquals(tc, 0, loaded->four_arms_restore_slot);
    if (GET_OBJ_TYPE(loaded) == ITEM_CONTAINER)
      CuAssertPtrNotNull(tc, loaded->contains);
  }
  extract_everything(&fixture.ch);

  /* the tail position 44 (Loc 44) is untouched by the appended slots */
  CuAssertIntEquals(tc, 44, WEAR_TAIL + 1);
  CuAssertIntEquals(tc, 45, WEAR_WIELD_3 + 1);
  CuAssertIntEquals(tc, 51, WEAR_WRIST_L2 + 1);

  end_four_arm_fixture(&fixture);
}

/* ---- Step 3: combat routing ---- */

#define RETURN_NUM_ATTACKS 1
#define DISPLAY_ROUTINE_POTENTIAL 2
#define NORMAL_ATTACK_ROUTINE 0
#define PHASE_0 0
#define PHASE_1 1
#define PHASE_2 2
#define PHASE_3 3

static void set_weapon_dice(struct obj_data *obj, int num, int size)
{
  GET_OBJ_VAL(obj, 1) = num;
  GET_OBJ_VAL(obj, 2) = size;
}

/* THIRD reads the third hand, then the lower two-hander; FOURTH reads the
 * fourth hand, or the lower double weapon's other end. */
void TestFourArmsGetWieldedRoutesSecondPair(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword, axe, big, staff;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  init_weapon(&sword, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&axe, "a fourth axe", WEAPON_TYPE_HAND_AXE, SIZE_MEDIUM);
  init_weapon(&big, "a lower greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_weapon(&staff, "a lower double axe", WEAPON_TYPE_DOUBLE_AXE, SIZE_LARGE);
  CuAssertTrue(tc, IS_SET(weapon_list[WEAPON_TYPE_DOUBLE_AXE].weaponFlags, WEAPON_FLAG_DOUBLE));

  CuAssertPtrEquals(tc, NULL, test_get_wielded(&fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&fixture.ch, ATTACK_TYPE_FOURTH));
  CuAssertTrue(tc, !is_dual_wielding_second_pair(&fixture.ch));

  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  equip_char(&fixture.ch, &axe, WEAR_WIELD_4);
  CuAssertPtrEquals(tc, &sword, test_get_wielded(&fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertPtrEquals(tc, &axe, test_get_wielded(&fixture.ch, ATTACK_TYPE_FOURTH));
  CuAssertTrue(tc, is_dual_wielding_second_pair(&fixture.ch));
  /* the first pair is untouched by the second */
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&fixture.ch, ATTACK_TYPE_PRIMARY));
  CuAssertTrue(tc, !is_dual_wielding(&fixture.ch));
  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &axe, unequip_char(&fixture.ch, WEAR_WIELD_4));

  equip_char(&fixture.ch, &big, WEAR_WIELD_2H_2);
  CuAssertPtrEquals(tc, &big, test_get_wielded(&fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertPtrEquals(tc, NULL, test_get_wielded(&fixture.ch, ATTACK_TYPE_FOURTH));
  CuAssertTrue(tc, !is_dual_wielding_second_pair(&fixture.ch));
  CuAssertPtrEquals(tc, &big, unequip_char(&fixture.ch, WEAR_WIELD_2H_2));

  equip_char(&fixture.ch, &staff, WEAR_WIELD_2H_2);
  CuAssertTrue(tc, is_using_double_weapon_at(&fixture.ch, WEAR_WIELD_2H_2));
  CuAssertTrue(tc, !is_using_double_weapon(&fixture.ch));
  CuAssertPtrEquals(tc, &staff, test_get_wielded(&fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertPtrEquals(tc, &staff, test_get_wielded(&fixture.ch, ATTACK_TYPE_FOURTH));
  CuAssertTrue(tc, is_dual_wielding_second_pair(&fixture.ch));

  CuAssertIntEquals(tc, WEAR_WIELD_2H_2, attack_pair_two_hand_slot(ATTACK_TYPE_THIRD));
  CuAssertIntEquals(tc, WEAR_WIELD_2H, attack_pair_two_hand_slot(ATTACK_TYPE_OFFHAND));
  CuAssertTrue(tc, attack_is_offhand_role(ATTACK_TYPE_FOURTH));
  CuAssertTrue(tc, !attack_is_offhand_role(ATTACK_TYPE_THIRD));

  end_four_arm_fixture(&fixture);
}

/* Strength, two-hand and spare-hand damage rules and the two-weapon attack
 * penalties come from the attacking weapon's own pair. */
void TestFourArmsSecondPairBonusesReadOwnPair(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data first, big, third, fourth;
  int str_bonus, base_hit;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  fixture.ch.real_abils.str = 18;
  fixture.ch.aff_abils.str = 18;
  str_bonus = GET_STR_BONUS(&fixture.ch);
  CuAssertTrue(tc, str_bonus > 0);
  init_weapon(&first, "a first sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&big, "an upper greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_weapon(&third, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&fourth, "a fourth sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);

  /* a lone third-hand weapon with three spare hands: primary-first allocation
   * gives the first pair one spare hand and the third hand the next */
  equip_char(&fixture.ch, &third, WEAR_WIELD_3);
  CuAssertIntEquals(tc, str_bonus + 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &third, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_THIRD));

  /* an upper two-hander does not rewrite or upgrade the third hand's swing */
  equip_char(&fixture.ch, &big, WEAR_WIELD_2H);
  CuAssertIntEquals(tc, 1, hands_available(&fixture.ch));
  CuAssertIntEquals(tc, str_bonus + 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &third, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_THIRD));
  CuAssertIntEquals(tc, str_bonus * 3 / 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &big, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_PRIMARY));
  CuAssertPtrEquals(tc, &big, unequip_char(&fixture.ch, WEAR_WIELD_2H));

  /* a full second pair: the fourth hand uses offhand strength, and both lower
   * swings carry the two-weapon penalty while the first pair does not */
  equip_char(&fixture.ch, &first, WEAR_WIELD_1);
  equip_char(&fixture.ch, &fourth, WEAR_WIELD_4);
  CuAssertIntEquals(tc, str_bonus / 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &fourth, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_FOURTH));
  CuAssertIntEquals(tc, str_bonus,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &third, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_THIRD));
  base_hit = compute_attack_bonus(&fixture.ch, &fixture.ch, ATTACK_TYPE_PRIMARY);
  CuAssertIntEquals(tc, -6, second_pair_dual_wielding_penalty(&fixture.ch, FALSE));
  CuAssertIntEquals(tc, -10, second_pair_dual_wielding_penalty(&fixture.ch, TRUE));
  CuAssertTrue(tc, !is_dual_wielding(&fixture.ch));
  CuAssertIntEquals(tc, base_hit - 6,
                    compute_attack_bonus(&fixture.ch, &fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertIntEquals(tc, base_hit - 10,
                    compute_attack_bonus(&fixture.ch, &fixture.ch, ATTACK_TYPE_FOURTH));

  /* the lower two-hander gets the two-hand strength rule for its own swing */
  CuAssertPtrEquals(tc, &third, unequip_char(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &fourth, unequip_char(&fixture.ch, WEAR_WIELD_4));
  equip_char(&fixture.ch, &big, WEAR_WIELD_2H_2);
  CuAssertPtrEquals(tc, &big, test_get_wielded(&fixture.ch, ATTACK_TYPE_THIRD));
  CuAssertIntEquals(tc, str_bonus * 3 / 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &big, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_THIRD));
  /* while the first pair's one-hander keeps its own rule */
  CuAssertIntEquals(tc, str_bonus + 2,
                    compute_damage_bonus(&fixture.ch, &fixture.ch, &first, TYPE_HIT, 0,
                                         MODE_NORMAL_HIT, ATTACK_TYPE_PRIMARY));

  end_four_arm_fixture(&fixture);
}

/* Count mode adds the floor of the summed mirror chances, never rolling;
 * display mode prints the second-pair rows with their weapons and chance. */
void TestFourArmsAttackRoutineCountsAndDisplays(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data first, third, fourth;
  int base;

  begin_four_arm_fixture(&fixture);
  init_weapon(&first, "a first sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&third, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&fourth, "a fourth axe", WEAPON_TYPE_HAND_AXE, SIZE_MEDIUM);
  equip_char(&fixture.ch, &first, WEAR_WIELD_1);
  base = perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0);
  CuAssertTrue(tc, base >= 1);

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  /* no lower weapons: nothing changes */
  CuAssertIntEquals(tc, base, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* one third-hand weapon at 50 percent: floor(0.5) adds nothing */
  equip_char(&fixture.ch, &third, WEAR_WIELD_3);
  CuAssertIntEquals(tc, base, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* third and fourth at 50 percent each: one expected attack */
  equip_char(&fixture.ch, &fourth, WEAR_WIELD_4);
  CuAssertIntEquals(tc, base + 1, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* two-weapon training: 75 percent each, floor(1.5) */
  SET_FEAT(&fixture.ch, FEAT_TWO_WEAPON_FIGHTING, 1);
  CuAssertIntEquals(tc, base + 1, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* improved training: 100 percent, plus the trained extra fourth-hand swing;
   * the first pair gains nothing because it is not dual wielding */
  SET_FEAT(&fixture.ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, 1);
  CuAssertIntEquals(tc, base + 3, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertIntEquals(tc, base + 3, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_2));

  /* vital strike suppresses the whole second-pair routine */
  VITAL_STRIKING(&fixture.ch) = TRUE;
  CuAssertIntEquals(tc, base, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  VITAL_STRIKING(&fixture.ch) = FALSE;

  /* display: rows for the lower hands, with their own weapons, no rolls */
  reset_output(&fixture);
  perform_attacks(&fixture.ch, DISPLAY_ROUTINE_POTENTIAL, PHASE_0);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Third hand, Attack Bonus"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Fourth hand, Attack Bonus"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "(100% chance)"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "a third sword"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "a fourth axe"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Improved 2 Weapon Fighting"));
  CuAssertPtrEquals(tc, NULL, FIGHTING(&fixture.ch));

  /* a weapon only in the fourth hand never produces an empty third swing */
  CuAssertPtrEquals(tc, &third, unequip_char(&fixture.ch, WEAR_WIELD_3));
  CuAssertIntEquals(tc, base + 2, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  reset_output(&fixture);
  perform_attacks(&fixture.ch, DISPLAY_ROUTINE_POTENTIAL, PHASE_0);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Fourth hand"));

  end_four_arm_fixture(&fixture);
}

struct four_arm_combat_fixture
{
  struct room_data room;
  struct index_data mobile_index[1];
  struct char_data actor;
  struct char_data victim;
  struct room_data *saved_world;
  struct index_data *saved_mob_index;
  room_rnum saved_top_of_world;
  mob_rnum saved_top_of_mobt;
};

static void init_combat_npc(struct char_data *ch, const char *name)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = CuMutableString(name);
  ch->player.name = CuMutableString(name);
  GET_LEVEL(ch) = 1;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 100000;
  GET_MAX_HIT(ch) = 100000;
  GET_REAL_SIZE(ch) = SIZE_MEDIUM;
  ch->points.size = SIZE_MEDIUM;
  IN_ROOM(ch) = 0;
}

static void begin_combat_fixture(struct four_arm_combat_fixture *fixture)
{
  if (!IS_SET(weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags, WEAPON_FLAG_RANGED))
    load_weapons();
  memset(fixture, 0, sizeof(*fixture));
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;
  fixture->room.number = 169930;
  fixture->room.light = 1;
  fixture->mobile_index[0].vnum = 1;
  world = &fixture->room;
  top_of_world = 0;
  mob_index = fixture->mobile_index;
  top_of_mobt = 0;
  init_combat_npc(&fixture->actor, "four arm attacker");
  init_combat_npc(&fixture->victim, "four arm target");
  GET_ATTACK_QUEUE(&fixture->actor) = create_attack_queue();
  GET_ATTACK_QUEUE(&fixture->victim) = create_attack_queue();
  fixture->room.people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->victim;
  FIGHTING(&fixture->actor) = &fixture->victim;
  FIGHTING(&fixture->victim) = &fixture->actor;
}

static void end_combat_fixture(struct four_arm_combat_fixture *fixture)
{
  int pos;

  FIGHTING(&fixture->actor) = NULL;
  FIGHTING(&fixture->victim) = NULL;
  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(&fixture->actor, pos))
      unequip_char(&fixture->actor, pos);
  while (fixture->actor.carrying)
    obj_from_char(fixture->actor.carrying);
  while (fixture->actor.affected != NULL)
    affect_remove_no_total(&fixture->actor, fixture->actor.affected);
  while (fixture->victim.affected != NULL)
    affect_remove_no_total(&fixture->victim, fixture->victim.affected);
  clear_char_event_list(&fixture->actor);
  clear_char_event_list(&fixture->victim);
  free_attack_queue(GET_ATTACK_QUEUE(&fixture->actor));
  free_attack_queue(GET_ATTACK_QUEUE(&fixture->victim));
  GET_ATTACK_QUEUE(&fixture->actor) = NULL;
  GET_ATTACK_QUEUE(&fixture->victim) = NULL;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;
}

/* best of several rounds: a natural 1 misses even at +100 to hit */
static int damage_in_phase(struct four_arm_combat_fixture *fixture, int phase)
{
  int best = 0, round, dealt;

  for (round = 0; round < 6; round++)
  {
    /* equipment changes recompute affects and reset the hit roll */
    GET_HITROLL(&fixture->actor) = 100;
    GET_HIT(&fixture->victim) = 100000;
    GET_POS(&fixture->victim) = POS_STANDING;
    perform_attacks(&fixture->actor, NORMAL_ATTACK_ROUTINE, phase);
    dealt = 100000 - GET_HIT(&fixture->victim);
    if (dealt > best)
      best = dealt;
  }
  return best;
}

/* Real rounds: each lower-hand swing lands with its own weapon dice in its
 * own phase, ordinals continue after the ordinary attacks, and phases 1..3
 * together deliver the same swings as the whole round. */
void TestFourArmsSecondPairAttacksLandWithOwnWeapons(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct obj_data first, third, fourth;
  int phase1, phase2, phase3, whole;

  begin_combat_fixture(&fixture);
  /* an NPC rogue: two-weapon and improved training both count, 100 percent */
  GET_CLASS(&fixture.actor) = CLASS_ROGUE;
  MOB_HAS_FEAT(&fixture.actor, FEAT_FOUR_ARMS) = 1;
  CuAssertTrue(tc, has_four_arms(&fixture.actor));
  init_weapon(&first, "a first dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  set_weapon_dice(&first, 1, 1);
  init_weapon(&third, "a third maul", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&third, 50, 1);
  init_weapon(&fourth, "a fourth maul", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&fourth, 50, 1);
  equip_char(&fixture.actor, &first, WEAR_WIELD_1);
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);
  GET_HITROLL(&fixture.actor) = 100;
  CuAssertIntEquals(tc, 3, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));

  /* ordinal 1: first pair (phase 1); ordinal 2: third hand (phase 2);
   * ordinal 3: fourth hand (phase 3) */
  phase1 = damage_in_phase(&fixture, PHASE_1);
  phase2 = damage_in_phase(&fixture, PHASE_2);
  phase3 = damage_in_phase(&fixture, PHASE_3);
  whole = damage_in_phase(&fixture, PHASE_0);
  /* 50d1 less the level-one mob's strength penalty: well above the dagger */
  CuAssertTrue(tc, phase1 > 0 && phase1 < 40);
  CuAssertTrue(tc, phase2 >= 40);
  CuAssertTrue(tc, phase3 >= 40);
  CuAssertTrue(tc, whole >= 80 + phase1);

  /* the fourth hand alone: no empty third-hand swing, the fourth still lands */
  CuAssertPtrEquals(tc, &third, unequip_char(&fixture.actor, WEAR_WIELD_3));
  CuAssertIntEquals(tc, 2, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_2) >= 40);
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_3) < 40);

  /* without the arms the lower weapons never swing */
  MOB_HAS_FEAT(&fixture.actor, FEAT_FOUR_ARMS) = 0;
  affect_total(&fixture.actor);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.actor, WEAR_WIELD_4));
  CuAssertIntEquals(tc, 1, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_0) < 40);

  end_combat_fixture(&fixture);
}

#undef RETURN_NUM_ATTACKS
#undef DISPLAY_ROUTINE_POTENTIAL
#undef NORMAL_ATTACK_ROUTINE
#undef PHASE_0
#undef PHASE_1
#undef PHASE_2
#undef PHASE_3

/* ---- Step 4 support: displays, typed bonuses, downgrade fallback ---- */

/* The equipment command labels every lower-arm slot, and four bracers with
 * the same typed bonus do not stack four times. */
void TestFourArmsEquipmentDisplayAndTypedBonuses(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword, big, sleeves, gloves, wrists[4];
  int base_dex, i;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  fixture.ch.real_abils.dex = 10;
  fixture.ch.aff_abils.dex = 10;
  base_dex = GET_DEX(&fixture.ch);
  init_weapon(&sword, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&big, "a lower greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_armor(&sleeves, "lower sleeves", ITEM_WEAR_ARMS, 1, SPEC_ARMOR_TYPE_LEATHER_ARMS);
  init_armor(&gloves, "lower gloves", ITEM_WEAR_HANDS, 0, 0);
  for (i = 0; i < 4; i++)
  {
    init_armor(&wrists[i], "a nimble bracer", ITEM_WEAR_WRIST, 0, 0);
    wrists[i].affected[0].location = APPLY_DEX;
    wrists[i].affected[0].modifier = 2;
    wrists[i].affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  }

  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  equip_char(&fixture.ch, &sleeves, WEAR_ARMS_2);
  equip_char(&fixture.ch, &gloves, WEAR_HANDS_2);
  equip_char(&fixture.ch, &wrists[0], WEAR_WRIST_R);
  equip_char(&fixture.ch, &wrists[1], WEAR_WRIST_L);
  equip_char(&fixture.ch, &wrists[2], WEAR_WRIST_R2);
  equip_char(&fixture.ch, &wrists[3], WEAR_WRIST_L2);
  CuAssertIntEquals(tc, base_dex + 2, GET_DEX(&fixture.ch));

  reset_output(&fixture);
  do_equipment(&fixture.ch, "", 0, 0);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Wielded Third"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Worn On Lower Arms"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Worn On Lower Hands"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "a third sword"));

  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_3));
  equip_char(&fixture.ch, &big, WEAR_WIELD_2H_2);
  reset_output(&fixture);
  do_equipment(&fixture.ch, "", 0, 0);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Wielded Twohanded 2"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "a lower greatsword"));

  /* removing one bracer keeps the single typed bonus */
  CuAssertPtrEquals(tc, &wrists[3], unequip_char(&fixture.ch, WEAR_WRIST_L2));
  CuAssertIntEquals(tc, base_dex + 2, GET_DEX(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* Downgrade fallback: a saved position beyond the wear table goes to
 * inventory on the player path, while the strict pet parser refuses it. */
void TestFourArmsUnknownSavedSlotFallsBackToInventory(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword;

  begin_four_arm_fixture(&fixture);
  init_weapon(&sword, "a sword from the future", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  test_auto_equip_loaded_object(&fixture.ch, &sword, NUM_WEARS + 1);
  CuAssertPtrEquals(tc, &fixture.ch, sword.carried_by);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword));
  CuAssertIntEquals(tc, 0, sword.four_arms_restore_slot);
  end_four_arm_fixture(&fixture);
}

/* Lower sleeves join the armor enhancement average in both numerator and
 * denominator: an equal lower piece leaves the averaged bonus unchanged. */
void TestFourArmsLowerSleevesAverageIntoArmorEnhancement(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data upper, lower;
  int one_piece, both_pieces;

  begin_four_arm_fixture(&fixture);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  init_armor(&upper, "upper sleeves", ITEM_WEAR_ARMS, 0, SPEC_ARMOR_TYPE_LEATHER_ARMS);
  init_armor(&lower, "lower sleeves", ITEM_WEAR_ARMS, 0, SPEC_ARMOR_TYPE_LEATHER_ARMS);
  GET_OBJ_VAL(&upper, 4) = 4;
  GET_OBJ_VAL(&lower, 4) = 4;

  equip_char(&fixture.ch, &upper, WEAR_ARMS);
  one_piece = compute_armor_class(&fixture.ch, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  equip_char(&fixture.ch, &lower, WEAR_ARMS_2);
  both_pieces = compute_armor_class(&fixture.ch, &fixture.ch, FALSE, MODE_ARMOR_CLASS_NORMAL);
  CuAssertIntEquals(tc, one_piece, both_pieces);
  CuAssertPtrEquals(tc, &lower, unequip_char(&fixture.ch, WEAR_ARMS_2));
  one_piece = compute_gear_enhancement_bonus(&fixture.ch);
  equip_char(&fixture.ch, &lower, WEAR_ARMS_2);
  CuAssertIntEquals(tc, one_piece, compute_gear_enhancement_bonus(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* A deferred four-arm item saved with a bag sort still retries from
 * inventory; one whose provider never arrives ends up in its bag. */
void TestFourArmsDeferredRestoreHonorsBagSort(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, sword;
  struct obj_data *loaded;
  obj_save_data *records;
  FILE *file;

  begin_four_arm_fixture(&fixture);
  CREATE(fixture.ch.bags, struct bag_data, 1); /* players own bag storage */
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  init_weapon(&sword, "a sorted sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  GET_OBJ_SORT(&sword) = 2;

  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, test_objsave_save_obj_record(&sword, &fixture.ch, file, WEAR_WIELD_3 + 1));
  CuAssertTrue(tc, test_objsave_save_obj_record(&ring, &fixture.ch, file, WEAR_FINGER_R + 1));
  fputs("$~\n", file);
  CuAssertTrue(tc, rewind_stream(file));
  records = objsave_parse_objects(file);
  fclose(file);
  CuAssertPtrNotNull(tc, records);
  GET_OBJ_SORT(records->obj) = 2; /* prototype-less test records keep no sort */
  CuAssertIntEquals(tc, 2, test_restore_loaded_objects(&fixture.ch, records));
  loaded = GET_EQ(&fixture.ch, WEAR_WIELD_3);
  CuAssertPtrNotNull(tc, loaded);
  CuAssertStrEquals(tc, "a sorted sword", loaded->short_description);
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  extract_everything(&fixture.ch);

  /* no provider: the sword lands in bag 2, not loose in inventory */
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  CuAssertTrue(tc, test_objsave_save_obj_record(&sword, &fixture.ch, file, WEAR_WIELD_3 + 1));
  fputs("$~\n", file);
  CuAssertTrue(tc, rewind_stream(file));
  records = objsave_parse_objects(file);
  fclose(file);
  CuAssertPtrNotNull(tc, records);
  GET_OBJ_SORT(records->obj) = 2;
  CuAssertIntEquals(tc, 1, test_restore_loaded_objects(&fixture.ch, records));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  CuAssertPtrNotNull(tc, fixture.ch.bags);
  CuAssertPtrNotNull(tc, fixture.ch.bags->bag2);
  loaded = fixture.ch.bags->bag2;
  CuAssertStrEquals(tc, "a sorted sword", loaded->short_description);
  CuAssertIntEquals(tc, 0, loaded->four_arms_restore_slot);
  obj_from_bag(&fixture.ch, loaded, 2);
  extract_obj(loaded);
  free(fixture.ch.bags);
  fixture.ch.bags = NULL;

  end_four_arm_fixture(&fixture);
}
