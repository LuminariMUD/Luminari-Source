/* Four arms (FEAT_FOUR_ARMS): the second weapon pair and the doubled limb
 * slots, exercised through the production equip paths.  Step 1 of
 * docs/ongoing-projects/THRI_KREEN_FOUR_ARMS.md: capability sources, anatomy
 * gate, hand budget, placement, lower armor consumers and the restore cases. */
#include "CuTest.h"
#include <string.h>
#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/act.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/magic/spells.h"
#include "../../src/character/class.h"
#include "../../src/character/feats.h"
#include "../../src/character/race.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/combat/fight.h"
#include "../../src/constants.h"
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
  fixture->ch.player.name = (char *)"four arm tester";
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

static void init_weapon(struct obj_data *obj, const char *name, int weapon_type, int size)
{
  clear_object(obj);
  obj->name = (char *)name;
  obj->short_description = (char *)name;
  obj->description = (char *)name;
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  GET_OBJ_VAL(obj, 0) = weapon_type;
  GET_OBJ_SIZE(obj) = size;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);
}

static void init_armor(struct obj_data *obj, const char *name, int wear_flag, int ac,
                       int armor_family)
{
  clear_object(obj);
  obj->name = (char *)name;
  obj->short_description = (char *)name;
  obj->description = (char *)name;
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
  CuAssertPtrEquals(tc, &bracer, unequip_char(&fixture.ch, WEAR_WRIST_R2));

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
