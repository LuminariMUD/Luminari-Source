/* Four arms (FEAT_FOUR_ARMS, issue #168) and the arm count that generalized
 * it (described in docs/systems/GAME_MECHANICS_SYSTEMS.md): the second weapon
 * pair and the doubled limb slots, exercised through the production equip
 * paths.
 * Capability sources, anatomy gate, hand budget, placement, lower armor
 * consumers and the restore cases; loss handling, deferral across provider
 * cycles, order-independent restoration; second-pair combat routing
 * (THIRD/FOURTH attacks). */
#include "CuTest.h"
#include <string.h>
#include <sys/stat.h>
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
#include "../../src/craft/alchemy.h"
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
  sbyte saved_arm_adjust[NUM_EXTENDED_RACES];
};

static void save_arm_adjusts(sbyte *saved)
{
  int race;

  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    saved[race] = race_list[race].arm_adjust;
}

static void restore_arm_adjusts(const sbyte *saved)
{
  int race;

  for (race = 0; race < NUM_EXTENDED_RACES; race++)
    race_list[race].arm_adjust = saved[race];
}

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
  save_arm_adjusts(fixture->saved_arm_adjust);
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
  restore_arm_adjusts(fixture->saved_arm_adjust);
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
  CuAssertIntEquals(tc, 0, arm_count(NULL));
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));

  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
  CuAssertIntEquals(tc, 1, HAS_FEAT(&fixture.ch, FEAT_FOUR_ARMS));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));

  /* an ordinary-slot item provides the arms */
  init_armor(&ring, "a four-armed ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  CuAssertPtrEquals(tc, &ring, GET_EQ(&fixture.ch, WEAR_FINGER_R));
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));

  /* a second provider in a four-arm slot is not counted */
  init_armor(&bracer, "a four-armed bracer", ITEM_WEAR_WRIST, 0, 0);
  grant_feat_on_object(&bracer, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &bracer, WEAR_WRIST_R2);
  CuAssertPtrEquals(tc, &bracer, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  /* the ring was the only provider: the lower-wrist bracer is displaced */
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &fixture.ch, bracer.carried_by);

  /* NPCs use their mob feats */
  memset(&mob, 0, sizeof(mob));
  SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
  CuAssertTrue(tc, IS_NPC(&mob));
  CuAssertIntEquals(tc, 2, arm_count(&mob));
  MOB_HAS_FEAT(&mob, FEAT_FOUR_ARMS) = 1;
  CuAssertIntEquals(tc, 4, arm_count(&mob));
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
    CuAssertTrue(tc, wear_slot_arms_needed(slots[i]) > 2);
    CuAssertPtrNotNull(tc, character_wear_slot_restriction(&fixture.ch, slots[i]));
    CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, slots[i]));
  }
  CuAssertTrue(tc, wear_slot_arms_needed(WEAR_TAIL) <= 2);
  CuAssertTrue(tc, wear_slot_arms_needed(WEAR_WIELD_2H) <= 2);

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
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
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
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
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
  limb_reconcile(&fixture.ch);
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

  limb_defer_begin(&fixture.ch);
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  limb_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));

  /* nested deferral; the real loss lands when the outermost one ends */
  limb_defer_begin(&fixture.ch);
  limb_defer_begin(&fixture.ch);
  CuAssertPtrEquals(tc, &ring, unequip_char(&fixture.ch, WEAR_FINGER_R));
  limb_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  limb_defer_end(&fixture.ch);
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
  trig_index = (struct index_data **)calloc(1, sizeof(*trig_index));
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
  free((void *)trig_index);
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

  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
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
  CuAssertIntEquals(tc, 0, GET_EQ(&fixture.ch, WEAR_WIELD_4)->limb_restore_slot);
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
    CuAssertIntEquals(tc, 0, loaded->limb_restore_slot);
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
  sbyte saved_arm_adjust[NUM_EXTENDED_RACES];
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
  save_arm_adjusts(fixture->saved_arm_adjust);
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
  restore_arm_adjusts(fixture->saved_arm_adjust);
}

/* best of several rounds: a natural 1 misses even at +100 to hit */
static int damage_in_phase(struct four_arm_combat_fixture *fixture, int phase)
{
  int best = 0, round, dealt;

  for (round = 0; round < 6; round++)
  {
    /* a round of its own, as perform_violence() starts one at phase 1 */
    fixture->actor.char_specials.attack_round.drawn = FALSE;
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
  CuAssertIntEquals(tc, 4, arm_count(&fixture.actor));
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
  CuAssertIntEquals(tc, 0, sword.limb_restore_slot);
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
  CuAssertIntEquals(tc, 0, loaded->limb_restore_slot);
  obj_from_bag(&fixture.ch, loaded, 2);
  extract_obj(loaded);
  free(fixture.ch.bags);
  fixture.ch.bags = NULL;

  end_four_arm_fixture(&fixture);
}

/* ---- Arm count (docs/systems/GAME_MECHANICS_SYSTEMS.md, racial innate feats) ---- */

/* the fixture PC is human: its count is 2 plus the human arm adjustment */
static void set_human_arms(int count)
{
  race_list[RACE_HUMAN].arm_adjust = (sbyte)(count - 2);
}

static void set_vestigial_arm(struct four_arm_fixture *fixture, bool known)
{
  fixture->specials.saved.discoveries[ALC_DISC_VESTIGIAL_ARM] = known ? 1 : 0;
}

static void grant_feat_in_affect(struct obj_data *obj, int index, int feat)
{
  obj->affected[index].location = APPLY_FEAT;
  obj->affected[index].modifier = feat;
}

/* move whatever is worn or carried back out of the character, leaving the
 * stack objects free for the next case */
static void strip_character(struct char_data *ch)
{
  int pos;

  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(ch, pos))
      unequip_char(ch, pos);
  while (ch->carrying)
    obj_from_char(ch->carrying);
}

/* Grant sources: feats, race adjustment, the wild-shape feat switch, and
 * items worn in positions the intrinsic arms open. */
void TestArmCountSources(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring[2], amulet, bracer, lower_bracer;
  struct char_data mob;

  begin_four_arm_fixture(&fixture);
  CuAssertIntEquals(tc, 0, arm_count(NULL));
  CuAssertIntEquals(tc, 0, hands_have(NULL));
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));

  /* the character's own feats: Four Arms +2, Extra Arms +1 per rank */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 1);
  CuAssertIntEquals(tc, 3, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 2);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 5);
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, -1);
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 0);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
  /* both feats describe six arms, never the same two extra limbs twice */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 2);
  CuAssertIntEquals(tc, 6, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 0);
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);

  /* race adjustment, clamped at zero; an invalid race adds nothing */
  set_human_arms(1);
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  set_human_arms(0);
  CuAssertIntEquals(tc, 0, arm_count(&fixture.ch));
  race_list[RACE_HUMAN].arm_adjust = -5;
  CuAssertIntEquals(tc, 0, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertIntEquals(tc, 0, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);
  set_human_arms(2);
  GET_REAL_RACE(&fixture.ch) = NUM_EXTENDED_RACES;
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  GET_REAL_RACE(&fixture.ch) = -1;
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));

  /* a PC race's adjustment never reaches an NPC whose family shares its number */
  race_list[RACE_TYPE_HUMANOID].arm_adjust = -1;
  GET_REAL_RACE(&fixture.ch) = RACE_TYPE_HUMANOID;
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  GET_REAL_RACE(&fixture.ch) = RACE_HUMAN;
  memset(&mob, 0, sizeof(mob));
  SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
  GET_REAL_RACE(&mob) = RACE_TYPE_HUMANOID;
  CuAssertIntEquals(tc, 2, arm_count(&mob));
  MOB_HAS_FEAT(&mob, FEAT_FOUR_ARMS) = 1;
  CuAssertIntEquals(tc, 4, arm_count(&mob));
  MOB_HAS_FEAT(&mob, FEAT_EXTRA_ARMS) = 1;
  CuAssertIntEquals(tc, 5, arm_count(&mob));

  /* items: one Extra Arms rank per item even when the modifier repeats */
  init_armor(&ring[0], "an extra-arm ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_in_affect(&ring[0], 0, FEAT_EXTRA_ARMS);
  grant_feat_in_affect(&ring[0], 1, FEAT_EXTRA_ARMS);
  init_armor(&ring[1], "another extra-arm ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_in_affect(&ring[1], 0, FEAT_EXTRA_ARMS);
  equip_char(&fixture.ch, &ring[0], WEAR_FINGER_R);
  CuAssertIntEquals(tc, 3, arm_count(&fixture.ch));
  equip_char(&fixture.ch, &ring[1], WEAR_FINGER_L);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));

  /* NPCs and the wild-shaped feat source ignore item grants */
  GET_EQ(&mob, WEAR_FINGER_R) = &ring[1];
  CuAssertIntEquals(tc, 5, arm_count(&mob));
  GET_EQ(&mob, WEAR_FINGER_R) = NULL;

  /* an item bearing both feats; Four Arms counts once across every source */
  init_armor(&amulet, "a many-armed amulet", ITEM_WEAR_NECK, 0, 0);
  grant_feat_in_affect(&amulet, 0, FEAT_FOUR_ARMS);
  grant_feat_in_affect(&amulet, 1, FEAT_EXTRA_ARMS);
  equip_char(&fixture.ch, &amulet, WEAR_NECK_1);
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  init_armor(&bracer, "a four-armed bracer", ITEM_WEAR_WRIST, 0, 0);
  grant_feat_in_affect(&bracer, 0, FEAT_FOUR_ARMS);
  equip_char(&fixture.ch, &bracer, WEAR_WRIST_R);
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);

  /* a provider above the intrinsic arms is worn but never counted */
  init_armor(&lower_bracer, "a lower extra-arm bracer", ITEM_WEAR_WRIST, 0, 0);
  grant_feat_in_affect(&lower_bracer, 0, FEAT_EXTRA_ARMS);
  equip_char(&fixture.ch, &lower_bracer, WEAR_WRIST_R2);
  CuAssertPtrEquals(tc, &lower_bracer, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  /* with three intrinsic arms the same position counts */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 1);
  CuAssertIntEquals(tc, 9, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 0);
  CuAssertPtrEquals(tc, &lower_bracer, unequip_char(&fixture.ch, WEAR_WRIST_R2));

  /* a wild shape reads its disguise race and mob feats, not gear or the
   * character's own feats; an ordinary disguise changes nothing */
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 1);
  race_list[RACE_WEMIC].arm_adjust = 1;
  GET_DISGUISE_RACE(&fixture.ch) = RACE_WEMIC;
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_WILD_SHAPE);
  CuAssertIntEquals(tc, 3, arm_count(&fixture.ch));
  MOB_HAS_FEAT(&fixture.ch, FEAT_FOUR_ARMS) = 1;
  CuAssertIntEquals(tc, 5, arm_count(&fixture.ch));
  MOB_HAS_FEAT(&fixture.ch, FEAT_FOUR_ARMS) = 0;
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_WILD_SHAPE);
  GET_DISGUISE_RACE(&fixture.ch) = 0;
  CuAssertIntEquals(tc, 7, arm_count(&fixture.ch));
  SET_FEAT(&fixture.ch, FEAT_FOUR_ARMS, 0);

  /* the vestigial arm adds a hand, never an arm or a position */
  strip_character(&fixture.ch);
  set_human_arms(1);
  CuAssertIntEquals(tc, 1, hands_have(&fixture.ch));
  set_vestigial_arm(&fixture, true);
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  CuAssertIntEquals(tc, 2, hands_have(&fixture.ch));
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_HOLD_2H));
  set_vestigial_arm(&fixture, false);

  end_four_arm_fixture(&fixture);
}

/* Every position at counts 0 through 6: the table decides, anatomy still
 * applies, and arms past four open nothing more. */
void TestArmCountPositionsAtEachCount(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct char_data mob;
  const struct
  {
    int pos;
    int arms;
  } table[] = {
      {WEAR_WIELD_1, 1},    {WEAR_HOLD_1, 1},        {WEAR_HOLD_2, 1},     {WEAR_SHIELD, 1},
      {WEAR_HANDS, 1},      {WEAR_ARMS, 1},          {WEAR_WRIST_R, 1},    {WEAR_FINGER_R, 1},
      {WEAR_FINGER_L, 1},   {WEAR_WIELD_OFFHAND, 2}, {WEAR_WIELD_2H, 2},   {WEAR_HOLD_2H, 2},
      {WEAR_WRIST_L, 2},    {WEAR_WIELD_3, 3},       {WEAR_HANDS_2, 3},    {WEAR_ARMS_2, 3},
      {WEAR_WRIST_R2, 3},   {WEAR_WIELD_4, 4},       {WEAR_WIELD_2H_2, 4}, {WEAR_WRIST_L2, 4},
      {WEAR_BODY, 0},       {WEAR_NECK_1, 0},        {WEAR_LIGHT, 0},      {WEAR_SHEATH, 0},
      {WEAR_INSTRUMENT, 0}, {WEAR_CRAFT_AXE, 0},     {WEAR_TAIL, 0},       {-1, 0},
      {NUM_WEARS, 0}};
  int count, pos, needed;
  size_t i;

  for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    CuAssertIntEquals(tc, table[i].arms, wear_slot_arms_needed(table[i].pos));

  begin_four_arm_fixture(&fixture);
  for (count = 0; count <= 6; count++)
  {
    set_human_arms(count);
    CuAssertIntEquals(tc, count, arm_count(&fixture.ch));
    for (pos = 0; pos < NUM_WEARS; pos++)
    {
      needed = wear_slot_arms_needed(pos);
      CuAssertIntEquals(tc, pos != WEAR_TAIL && needed <= count,
                        character_can_use_wear_slot(&fixture.ch, pos));
      if (pos != WEAR_TAIL && needed > count)
        CuAssertStrEquals(tc, "You do not have enough arms to use that equipment slot.",
                          character_wear_slot_restriction(&fixture.ch, pos));
    }
  }

  /* an ordinary count keeps race anatomy: trelux has no hand positions, and
   * its doubled positions inherit that even with enough arms */
  GET_REAL_RACE(&fixture.ch) = RACE_TRELUX;
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_HANDS));
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_WIELD_1));
  CuAssertTrue(tc, character_can_use_wear_slot(&fixture.ch, WEAR_BODY));
  race_list[RACE_TRELUX].arm_adjust = 2;
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_HANDS_2));
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.ch, WEAR_WIELD_3));
  CuAssertTrue(tc, character_can_use_wear_slot(&fixture.ch, WEAR_WRIST_L2));
  GET_REAL_RACE(&fixture.ch) = RACE_HUMAN;

  /* NPCs are checked too, at their fixed two arms */
  set_human_arms(0);
  memset(&mob, 0, sizeof(mob));
  SET_BIT_AR(MOB_FLAGS(&mob), MOB_ISNPC);
  GET_REAL_RACE(&mob) = RACE_HUMAN;
  CuAssertTrue(tc, character_can_use_wear_slot(&mob, WEAR_WIELD_OFFHAND));
  CuAssertTrue(tc, !character_can_use_wear_slot(&mob, WEAR_WIELD_3));

  end_four_arm_fixture(&fixture);
}

/* Zero and one arm through the wear command: refusals name the arms or the
 * hands, full positions report themselves, and the vestigial arm holds a
 * second item without opening an offhand or two-hand position. */
void TestArmCountWearAtZeroAndOneArm(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword[2], big, bow, shield, orb[2], gloves[2], sleeves, bracer[2], ring[2], vest;
  int i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 2; i++)
  {
    init_weapon(&sword[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
    init_held(&orb[i], "a test orb");
    init_armor(&gloves[i], "test gloves", ITEM_WEAR_HANDS, 0, 0);
    init_armor(&bracer[i], "a test bracer", ITEM_WEAR_WRIST, 0, 0);
    init_armor(&ring[i], "a test ring", ITEM_WEAR_FINGER, 0, 0);
  }
  init_weapon(&big, "a test greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_weapon(&bow, "a test bow", WEAPON_TYPE_LONG_BOW, SIZE_MEDIUM);
  init_armor(&shield, "a test shield", ITEM_WEAR_SHIELD, 0, 0);
  init_armor(&sleeves, "test sleeves", ITEM_WEAR_ARMS, 0, 0);
  init_armor(&vest, "a test vest", ITEM_WEAR_BODY, 0, 0);

  /* zero arms: no weapon, ring or glove; body armor still fits */
  set_human_arms(0);
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[0]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "enough arms"));
  wear_from_inventory(&fixture, &ring[0], WEAR_FINGER_R);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &ring[0]));
  wear_from_inventory(&fixture, &gloves[0], WEAR_HANDS);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &gloves[0]));
  wear_from_inventory(&fixture, &vest, WEAR_BODY);
  CuAssertIntEquals(tc, WEAR_BODY, worn_position(&fixture.ch, &vest));
  /* a vestigial arm is a hand without a position */
  set_vestigial_arm(&fixture, true);
  CuAssertIntEquals(tc, 1, hands_have(&fixture.ch));
  wear_from_inventory(&fixture, &orb[0], WEAR_HOLD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &orb[0]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "enough arms"));
  strip_character(&fixture.ch);
  set_vestigial_arm(&fixture, false);

  /* one arm: one hand for a weapon, a shield or a held item */
  set_human_arms(1);
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_1, worn_position(&fixture.ch, &sword[0]));
  wear_from_inventory(&fixture, &sword[1], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  wear_from_inventory(&fixture, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &shield));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  wear_from_inventory(&fixture, &orb[0], WEAR_HOLD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &orb[0]));
  /* both rings, one set of gloves and sleeves, one wrist */
  wear_from_inventory(&fixture, &ring[0], WEAR_FINGER_R);
  wear_from_inventory(&fixture, &ring[1], WEAR_FINGER_R);
  CuAssertIntEquals(tc, WEAR_FINGER_R, worn_position(&fixture.ch, &ring[0]));
  CuAssertIntEquals(tc, WEAR_FINGER_L, worn_position(&fixture.ch, &ring[1]));
  wear_from_inventory(&fixture, &gloves[0], WEAR_HANDS);
  wear_from_inventory(&fixture, &gloves[1], WEAR_HANDS);
  CuAssertIntEquals(tc, WEAR_HANDS, worn_position(&fixture.ch, &gloves[0]));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &gloves[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "on your hands"));
  wear_from_inventory(&fixture, &sleeves, WEAR_ARMS);
  CuAssertIntEquals(tc, WEAR_ARMS, worn_position(&fixture.ch, &sleeves));
  wear_from_inventory(&fixture, &bracer[0], WEAR_WRIST_R);
  wear_from_inventory(&fixture, &bracer[1], WEAR_WRIST_R);
  CuAssertIntEquals(tc, WEAR_WRIST_R, worn_position(&fixture.ch, &bracer[0]));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bracer[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "around your wrist."));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "NEVER"));
  /* the shield replaces the weapon */
  perform_remove(&fixture.ch, WEAR_WIELD_1, FALSE);
  obj_from_char(&shield);
  wear_from_inventory(&fixture, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, WEAR_SHIELD, worn_position(&fixture.ch, &shield));
  perform_remove(&fixture.ch, WEAR_SHIELD, FALSE);
  /* no two-hand position, and a bow's second hand does not exist */
  wear_from_inventory(&fixture, &big, WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big));
  wear_from_inventory(&fixture, &bow, WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bow));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  obj_from_char(&big);
  obj_from_char(&bow);

  /* one arm plus the vestigial arm: two one-hand items, never an offhand
   * weapon or a two-hand position */
  set_vestigial_arm(&fixture, true);
  obj_from_char(&sword[0]);
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_1, worn_position(&fixture.ch, &sword[0]));
  obj_from_char(&sword[1]);
  wear_from_inventory(&fixture, &sword[1], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Your hands are full."));
  obj_from_char(&orb[0]);
  wear_from_inventory(&fixture, &orb[0], WEAR_HOLD_1);
  CuAssertIntEquals(tc, WEAR_HOLD_1, worn_position(&fixture.ch, &orb[0]));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));
  perform_remove(&fixture.ch, WEAR_HOLD_1, FALSE);
  obj_from_char(&shield);
  wear_from_inventory(&fixture, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, WEAR_SHIELD, worn_position(&fixture.ch, &shield));
  perform_remove(&fixture.ch, WEAR_SHIELD, FALSE);
  perform_remove(&fixture.ch, WEAR_WIELD_1, FALSE);
  wear_from_inventory(&fixture, &big, WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "enough arms"));
  obj_from_char(&orb[0]);
  wear_from_inventory(&fixture, &orb[0], WEAR_HOLD_1);
  wear_from_inventory(&fixture, &orb[1], WEAR_HOLD_1);
  CuAssertIntEquals(tc, WEAR_HOLD_1, worn_position(&fixture.ch, &orb[0]));
  CuAssertIntEquals(tc, WEAR_HOLD_2, worn_position(&fixture.ch, &orb[1]));
  strip_character(&fixture.ch);
  set_vestigial_arm(&fixture, false);

  end_four_arm_fixture(&fixture);
}

/* Three arms add the third hand, lower gloves and sleeves and a third wrist;
 * each weapon pair is exclusive; direct lower requests pass the same checks;
 * six arms hold four weapons, a held item and a shield in exactly six hands. */
void TestArmCountPlacementAtThreeAndSixArms(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword[4], big[2], bow, bracer[5], gloves[2], sleeves[2], orb[2], shield;
  int i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 4; i++)
    init_weapon(&sword[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  for (i = 0; i < 5; i++)
    init_armor(&bracer[i], "a test bracer", ITEM_WEAR_WRIST, 0, 0);
  for (i = 0; i < 2; i++)
  {
    init_weapon(&big[i], "a test greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
    init_armor(&gloves[i], "test gloves", ITEM_WEAR_HANDS, 0, 0);
    init_armor(&sleeves[i], "test sleeves", ITEM_WEAR_ARMS, 0, 0);
    init_held(&orb[i], "a test orb");
  }
  init_weapon(&bow, "a test bow", WEAPON_TYPE_LONG_BOW, SIZE_MEDIUM);
  init_armor(&shield, "a test shield", ITEM_WEAR_SHIELD, 0, 0);

  set_human_arms(3);
  for (i = 0; i < 4; i++)
    wear_from_inventory(&fixture, &sword[i], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_1, worn_position(&fixture.ch, &sword[0]));
  CuAssertIntEquals(tc, WEAR_WIELD_OFFHAND, worn_position(&fixture.ch, &sword[1]));
  CuAssertIntEquals(tc, WEAR_WIELD_3, worn_position(&fixture.ch, &sword[2]));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[3]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  for (i = 0; i < 4; i++)
    wear_from_inventory(&fixture, &bracer[i], WEAR_WRIST_R);
  CuAssertIntEquals(tc, WEAR_WRIST_R2, worn_position(&fixture.ch, &bracer[2]));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bracer[3]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "all three of your wrists"));
  for (i = 0; i < 2; i++)
  {
    wear_from_inventory(&fixture, &gloves[i], WEAR_HANDS);
    wear_from_inventory(&fixture, &sleeves[i], WEAR_ARMS);
  }
  CuAssertIntEquals(tc, WEAR_HANDS_2, worn_position(&fixture.ch, &gloves[1]));
  CuAssertIntEquals(tc, WEAR_ARMS_2, worn_position(&fixture.ch, &sleeves[1]));
  obj_from_char(&sword[3]);
  wear_from_inventory(&fixture, &sword[3], WEAR_WIELD_4);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[3]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "enough arms"));
  strip_character(&fixture.ch);

  /* pair exclusivity: a two-hander never joins a one-hander's pair */
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  wear_from_inventory(&fixture, &big[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big[0]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "pair of hands"));
  perform_remove(&fixture.ch, WEAR_WIELD_1, FALSE);
  obj_from_char(&big[0]);
  wear_from_inventory(&fixture, &big[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_2H, worn_position(&fixture.ch, &big[0]));
  obj_from_char(&sword[0]);
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_3, worn_position(&fixture.ch, &sword[0]));
  /* and equip_char() enforces the same rule */
  equip_char(&fixture.ch, &sword[1], WEAR_WIELD_OFFHAND);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, &fixture.ch, sword[1].carried_by);
  /* a two-hander sent to a lower hand wants the lower pair, closed at three */
  strip_character(&fixture.ch);
  wear_from_inventory(&fixture, &big[1], WEAR_WIELD_3);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &big[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "enough arms"));
  strip_character(&fixture.ch);

  /* four arms: the same request takes the lower two-hand position; a
   * launcher never reaches the lower pair from the command either */
  set_human_arms(4);
  wear_from_inventory(&fixture, &big[1], WEAR_WIELD_3);
  CuAssertIntEquals(tc, WEAR_WIELD_2H_2, worn_position(&fixture.ch, &big[1]));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  wear_from_inventory(&fixture, &bow, WEAR_WIELD_3);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bow));
  wear_from_inventory(&fixture, &sword[0], WEAR_WIELD_4);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &sword[0]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "pair of hands"));
  strip_character(&fixture.ch);

  /* six arms: the existing positions hold six hands and no more */
  set_human_arms(6);
  for (i = 0; i < 4; i++)
    wear_from_inventory(&fixture, &sword[i], WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_4, worn_position(&fixture.ch, &sword[3]));
  wear_from_inventory(&fixture, &orb[0], WEAR_HOLD_1);
  wear_from_inventory(&fixture, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, WEAR_HOLD_1, worn_position(&fixture.ch, &orb[0]));
  CuAssertIntEquals(tc, WEAR_SHIELD, worn_position(&fixture.ch, &shield));
  CuAssertIntEquals(tc, 6, hands_used(&fixture.ch));
  CuAssertIntEquals(tc, 0, hands_available(&fixture.ch));
  wear_from_inventory(&fixture, &orb[1], WEAR_HOLD_1);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &orb[1]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "extra hand"));
  for (i = 0; i < 5; i++)
    wear_from_inventory(&fixture, &bracer[i], WEAR_WRIST_R);
  CuAssertIntEquals(tc, WEAR_WRIST_L2, worn_position(&fixture.ch, &bracer[3]));
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &bracer[4]));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "all four of your wrists"));
  strip_character(&fixture.ch);

  end_four_arm_fixture(&fixture);
}

static void equip_list(struct char_data *ch, struct obj_data *objs, const int *positions, int count)
{
  int i;

  for (i = 0; i < count; i++)
    equip_char(ch, &objs[i], positions[i]);
}

/* Losses one step at a time close the positions the count no longer opens,
 * keep the primary weapon while it is legal, and move gear to inventory. */
void TestArmCountLossClosesPositionsStepByStep(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data swords[4], bracers[4], gloves[2];
  const int sword_pos[] = {WEAR_WIELD_1, WEAR_WIELD_OFFHAND, WEAR_WIELD_3, WEAR_WIELD_4};
  const int bracer_pos[] = {WEAR_WRIST_R, WEAR_WRIST_L, WEAR_WRIST_R2, WEAR_WRIST_L2};
  const int glove_pos[] = {WEAR_HANDS, WEAR_HANDS_2};
  int i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 4; i++)
  {
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
    init_armor(&bracers[i], "a test bracer", ITEM_WEAR_WRIST, 0, 0);
  }
  for (i = 0; i < 2; i++)
    init_armor(&gloves[i], "test gloves", ITEM_WEAR_HANDS, 0, 0);

  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 2);
  equip_list(&fixture.ch, swords, sword_pos, 4);
  equip_list(&fixture.ch, bracers, bracer_pos, 4);
  equip_list(&fixture.ch, gloves, glove_pos, 2);
  CuAssertIntEquals(tc, 4, hands_used(&fixture.ch));

  /* 4 -> 3: the fourth hand and fourth wrist */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 1);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_L2));
  CuAssertPtrEquals(tc, &swords[2], GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &bracers[2], GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, &gloves[1], GET_EQ(&fixture.ch, WEAR_HANDS_2));
  CuAssertIntEquals(tc, 2, count_carried(&fixture.ch));

  /* 3 -> 2: every lower position */
  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 0);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_R2));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HANDS_2));
  CuAssertPtrEquals(tc, &swords[1], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertIntEquals(tc, 5, count_carried(&fixture.ch));

  /* 2 -> 1: the offhand weapon and left wrist; the primary stays */
  set_human_arms(1);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_L));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &bracers[0], GET_EQ(&fixture.ch, WEAR_WRIST_R));
  CuAssertPtrEquals(tc, &gloves[0], GET_EQ(&fixture.ch, WEAR_HANDS));
  CuAssertIntEquals(tc, 1, hands_used(&fixture.ch));
  CuAssertIntEquals(tc, 7, count_carried(&fixture.ch));

  /* 1 -> 0: nothing hand-borne remains, all of it carried, none dropped */
  set_human_arms(0);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_R));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HANDS));
  CuAssertIntEquals(tc, 10, count_carried(&fixture.ch));
  CuAssertPtrEquals(tc, NULL, fixture.room.contents);

  /* stable under repeated recomputation */
  affect_total(&fixture.ch);
  limb_reconcile(&fixture.ch);
  CuAssertIntEquals(tc, 10, count_carried(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* Capacity losses with no closed position still trim the hands: six to five
 * arms, and a lost vestigial arm. */
void TestArmCountLossTrimsHands(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data swords[4], orb, shield;
  const int sword_pos[] = {WEAR_WIELD_1, WEAR_WIELD_OFFHAND, WEAR_WIELD_3, WEAR_WIELD_4};
  int i;

  begin_four_arm_fixture(&fixture);
  for (i = 0; i < 4; i++)
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_held(&orb, "a test orb");
  init_armor(&shield, "a test shield", ITEM_WEAR_SHIELD, 0, 0);

  set_human_arms(6);
  equip_list(&fixture.ch, swords, sword_pos, 4);
  equip_char(&fixture.ch, &orb, WEAR_HOLD_1);
  equip_char(&fixture.ch, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, 6, hands_used(&fixture.ch));
  set_human_arms(5);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, &shield, GET_EQ(&fixture.ch, WEAR_SHIELD));
  CuAssertPtrEquals(tc, &swords[3], GET_EQ(&fixture.ch, WEAR_WIELD_4));
  CuAssertIntEquals(tc, 5, hands_used(&fixture.ch));
  CuAssertPtrEquals(tc, &fixture.ch, orb.carried_by);
  strip_character(&fixture.ch);

  /* one arm and the vestigial arm: the held item goes, the weapon stays */
  set_human_arms(1);
  set_vestigial_arm(&fixture, true);
  equip_char(&fixture.ch, &swords[0], WEAR_WIELD_1);
  equip_char(&fixture.ch, &orb, WEAR_HOLD_1);
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  set_vestigial_arm(&fixture, false);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertIntEquals(tc, 1, count_carried(&fixture.ch));
  strip_character(&fixture.ch);

  /* a closed position also triggers the trim when the hands end no lower
   * than the last completed check: two hands before and after a deferral
   * that grew to four arms and fell to one plus the vestigial arm */
  set_human_arms(2);
  affect_total(&fixture.ch);
  limb_defer_begin(&fixture.ch);
  set_human_arms(4);
  equip_char(&fixture.ch, &swords[0], WEAR_WIELD_1);
  equip_char(&fixture.ch, &swords[2], WEAR_WIELD_3);
  equip_char(&fixture.ch, &orb, WEAR_HOLD_1);
  equip_char(&fixture.ch, &shield, WEAR_SHIELD);
  CuAssertIntEquals(tc, 4, hands_used(&fixture.ch));
  set_human_arms(1);
  set_vestigial_arm(&fixture, true);
  limb_defer_end(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, &shield, GET_EQ(&fixture.ch, WEAR_SHIELD));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  set_vestigial_arm(&fixture, false);
  strip_character(&fixture.ch);

  end_four_arm_fixture(&fixture);
}

/* Cascading loss: removing a ring provider shrinks the hands, trimming the
 * held Extra Arms provider, which then closes the third hand already passed. */
void TestArmCountLossCascadesThroughHeldProvider(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, orb, swords[3];
  const int sword_pos[] = {WEAR_WIELD_1, WEAR_WIELD_OFFHAND, WEAR_WIELD_3};
  int i;

  begin_four_arm_fixture(&fixture);
  init_armor(&ring, "an extra-arm ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_EXTRA_ARMS);
  init_held(&orb, "an extra-arm orb");
  grant_feat_on_object(&orb, FEAT_EXTRA_ARMS);
  for (i = 0; i < 3; i++)
    init_weapon(&swords[i], "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);

  equip_char(&fixture.ch, &ring, WEAR_FINGER_R);
  equip_char(&fixture.ch, &orb, WEAR_HOLD_1);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.ch));
  equip_list(&fixture.ch, swords, sword_pos, 3);
  CuAssertIntEquals(tc, 4, hands_used(&fixture.ch));

  reset_output(&fixture);
  perform_remove(&fixture.ch, WEAR_FINGER_R, FALSE);
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_3));
  CuAssertPtrEquals(tc, &swords[0], GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &swords[1], GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertIntEquals(tc, 3, count_carried(&fixture.ch));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));

  end_four_arm_fixture(&fixture);
}

/* A mixed first pair allowed at two arms with the vestigial arm loses its
 * two-hander when a third arm makes the pairs exclusive; an unchanged
 * two-armed body is never audited for it. */
void TestArmCountThirdArmSplitsMixedFirstPair(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword, big;

  begin_four_arm_fixture(&fixture);
  init_weapon(&sword, "a test sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&big, "a test greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  set_vestigial_arm(&fixture, true);
  wear_from_inventory(&fixture, &sword, WEAR_WIELD_1);
  wear_from_inventory(&fixture, &big, WEAR_WIELD_1);
  CuAssertIntEquals(tc, WEAR_WIELD_1, worn_position(&fixture.ch, &sword));
  CuAssertIntEquals(tc, WEAR_WIELD_2H, worn_position(&fixture.ch, &big));
  affect_total(&fixture.ch);
  CuAssertIntEquals(tc, WEAR_WIELD_2H, worn_position(&fixture.ch, &big));

  SET_FEAT(&fixture.ch, FEAT_EXTRA_ARMS, 1);
  affect_total(&fixture.ch);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WIELD_2H));
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &fixture.ch, big.carried_by);
  set_vestigial_arm(&fixture, false);

  end_four_arm_fixture(&fixture);
}

/* write the records in the given order, then restore them onto the fixture */
static int restore_records(CuTest *tc, struct four_arm_fixture *fixture, struct obj_data **objs,
                           const int *locations, int count)
{
  obj_save_data *records;
  FILE *file = tmpfile();
  int i;

  CuAssertPtrNotNull(tc, file);
  if (file == NULL)
    return 0;
  for (i = 0; i < count; i++)
    CuAssertTrue(tc, test_objsave_save_obj_record(objs[i], &fixture->ch, file, locations[i]));
  fputs("$~\n", file);
  CuAssertTrue(tc, rewind_stream(file));
  records = objsave_parse_objects(file);
  fclose(file);
  CuAssertPtrNotNull(tc, records);
  return test_restore_loaded_objects(&fixture->ch, records);
}

static const char *worn_name(struct char_data *ch, int pos)
{
  return GET_EQ(ch, pos) ? GET_EQ(ch, pos)->short_description : "";
}

static bool restore_markers_clear(struct char_data *ch)
{
  struct obj_data *obj;
  int pos;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    if (obj->limb_restore_slot != 0)
      return false;
  for (pos = 0; pos < NUM_WEARS; pos++)
    if (GET_EQ(ch, pos) && GET_EQ(ch, pos)->limb_restore_slot != 0)
      return false;
  return true;
}

/* Flat-file restore with the Extra Arms provider recorded after the gear it
 * supports, on intrinsically one- and zero-armed bodies; missing, rejected and
 * self-supporting providers; and an over-budget loadout in ordinary positions. */
void TestArmCountRestoreFromRecords(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data ring, amulet, bracer, pouch, coin, sword, gloves, orb[2], bad_ring, self_bracer;
  struct obj_data *objs[6];
  int locations[6];

  begin_four_arm_fixture(&fixture);
  init_armor(&ring, "an extra-arm ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&ring, FEAT_EXTRA_ARMS);
  init_armor(&amulet, "an extra-arm amulet", ITEM_WEAR_NECK, 0, 0);
  grant_feat_on_object(&amulet, FEAT_EXTRA_ARMS);
  init_armor(&bracer, "a left bracer", ITEM_WEAR_WRIST, 0, 0);
  init_held(&pouch, "an offhand pouch");
  GET_OBJ_TYPE(&pouch) = ITEM_CONTAINER;
  GET_OBJ_VAL(&pouch, 0) = 50;
  SET_BIT_AR(GET_OBJ_WEAR(&pouch), ITEM_WEAR_WIELD);
  init_held(&coin, "a pouch coin");
  init_weapon(&sword, "a primary sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_armor(&gloves, "restored gloves", ITEM_WEAR_HANDS, 0, 0);
  init_held(&orb[0], "a first orb");
  init_held(&orb[1], "a second orb");
  /* a provider with the wrong wear flag for its saved position */
  init_armor(&bad_ring, "a misfiled ring", ITEM_WEAR_FINGER, 0, 0);
  grant_feat_on_object(&bad_ring, FEAT_EXTRA_ARMS);
  init_armor(&self_bracer, "a self-supporting bracer", ITEM_WEAR_WRIST, 0, 0);
  grant_feat_on_object(&self_bracer, FEAT_EXTRA_ARMS);

  /* one arm: dependents first, the ring provider last */
  set_human_arms(1);
  objs[0] = &bracer, locations[0] = WEAR_WRIST_L + 1;
  objs[1] = &coin, locations[1] = -1;
  objs[2] = &pouch, locations[2] = WEAR_WIELD_OFFHAND + 1;
  objs[3] = &sword, locations[3] = WEAR_WIELD_1 + 1;
  objs[4] = &ring, locations[4] = WEAR_FINGER_R + 1;
  CuAssertIntEquals(tc, 5, restore_records(tc, &fixture, objs, locations, 5));
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  CuAssertStrEquals(tc, "a left bracer", worn_name(&fixture.ch, WEAR_WRIST_L));
  CuAssertStrEquals(tc, "an offhand pouch", worn_name(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrNotNull(tc, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND)->contains);
  CuAssertStrEquals(tc, "a primary sword", worn_name(&fixture.ch, WEAR_WIELD_1));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  CuAssertTrue(tc, restore_markers_clear(&fixture.ch));
  extract_everything(&fixture.ch);

  /* zero arms: a neck provider opens one arm for the gear recorded before it */
  set_human_arms(0);
  objs[0] = &gloves, locations[0] = WEAR_HANDS + 1;
  objs[1] = &sword, locations[1] = WEAR_WIELD_1 + 1;
  objs[2] = &amulet, locations[2] = WEAR_NECK_1 + 1;
  CuAssertIntEquals(tc, 3, restore_records(tc, &fixture, objs, locations, 3));
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  CuAssertStrEquals(tc, "restored gloves", worn_name(&fixture.ch, WEAR_HANDS));
  CuAssertStrEquals(tc, "a primary sword", worn_name(&fixture.ch, WEAR_WIELD_1));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  extract_everything(&fixture.ch);

  /* one arm, no provider: the dependent waits in inventory, marker cleared */
  set_human_arms(1);
  objs[0] = &bracer, locations[0] = WEAR_WRIST_L + 1;
  objs[1] = &sword, locations[1] = WEAR_WIELD_1 + 1;
  CuAssertIntEquals(tc, 2, restore_records(tc, &fixture, objs, locations, 2));
  CuAssertStrEquals(tc, "a primary sword", worn_name(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_L));
  CuAssertIntEquals(tc, 1, count_carried(&fixture.ch));
  CuAssertTrue(tc, restore_markers_clear(&fixture.ch));
  extract_everything(&fixture.ch);

  /* a provider in the position it would open never sustains itself */
  objs[0] = &self_bracer, locations[0] = WEAR_WRIST_L + 1;
  CuAssertIntEquals(tc, 1, restore_records(tc, &fixture, objs, locations, 1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_WRIST_L));
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  CuAssertIntEquals(tc, 1, count_carried(&fixture.ch));
  CuAssertTrue(tc, restore_markers_clear(&fixture.ch));
  extract_everything(&fixture.ch);

  /* a provider refused for its wear flag leaves its dependents carried */
  objs[0] = &pouch, locations[0] = WEAR_WIELD_OFFHAND + 1;
  objs[1] = &bad_ring, locations[1] = WEAR_HANDS + 1;
  CuAssertIntEquals(tc, 2, restore_records(tc, &fixture, objs, locations, 2));
  CuAssertIntEquals(tc, 1, arm_count(&fixture.ch));
  CuAssertIntEquals(tc, 2, count_carried(&fixture.ch));
  CuAssertTrue(tc, restore_markers_clear(&fixture.ch));
  extract_everything(&fixture.ch);

  /* a fresh two-armed restore over the hand budget in ordinary positions */
  set_human_arms(2);
  objs[0] = &sword, locations[0] = WEAR_WIELD_1 + 1;
  objs[1] = &orb[0], locations[1] = WEAR_HOLD_1 + 1;
  objs[2] = &orb[1], locations[2] = WEAR_HOLD_2 + 1;
  CuAssertIntEquals(tc, 3, restore_records(tc, &fixture, objs, locations, 3));
  CuAssertStrEquals(tc, "a primary sword", worn_name(&fixture.ch, WEAR_WIELD_1));
  CuAssertStrEquals(tc, "a first orb", worn_name(&fixture.ch, WEAR_HOLD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_HOLD_2));
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  CuAssertIntEquals(tc, 1, count_carried(&fixture.ch));
  extract_everything(&fixture.ch);

  end_four_arm_fixture(&fixture);
}

/* Repeated real saves: save_char() strips and re-equips everything, and gear
 * that its provider's position supports returns to the same slots silently,
 * even when the dependent's position comes first in the wear table. */
void TestArmCountRepeatedSavesKeepGear(CuTest *tc)
{
  char temporary_directory[] = "/tmp/luminari-arm-count-XXXXXX";
  struct player_index_element index[1] = {0};
  struct player_index_element *saved_table = player_table;
  int saved_top = top_of_p_table;
  struct four_arm_fixture fixture;
  struct obj_data provider, bracer, offhand;
  char directory[PATH_MAX], name[32], filename[MAX_FILEPATH];
  char *fixture_name;
  bool first_saved, second_saved, quiet;
  int restored;

  begin_four_arm_fixture(&fixture);
  snprintf(name, sizeof(name), "Zzarm%ld", (long)getpid());
  fixture_name = fixture.ch.player.name;
  fixture.ch.player.name = name;
  index[0].name = name;
  index[0].id = 4248;
  player_table = index;
  top_of_p_table = 0;
  GET_PFILEPOS(&fixture.ch) = 0;

  set_human_arms(1);
  init_weapon(&provider, "an extra-arm sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  grant_feat_on_object(&provider, FEAT_EXTRA_ARMS);
  init_armor(&bracer, "a left bracer", ITEM_WEAR_WRIST, 0, 0);
  init_weapon(&offhand, "an offhand sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  equip_char(&fixture.ch, &provider, WEAR_WIELD_1);
  equip_char(&fixture.ch, &bracer, WEAR_WRIST_L);
  equip_char(&fixture.ch, &offhand, WEAR_WIELD_OFFHAND);
  CuAssertTrue(tc, WEAR_WRIST_L < WEAR_WIELD_1);
  CuAssertIntEquals(tc, 2, arm_count(&fixture.ch));
  CuAssertPtrEquals(tc, &bracer, GET_EQ(&fixture.ch, WEAR_WRIST_L));

  CuAssertPtrNotNull(tc, getcwd(directory, sizeof(directory)));
  CuAssertPtrNotNull(tc, mkdtemp(temporary_directory));
  CuAssertIntEquals(tc, 0, chdir(temporary_directory));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles", 0700));
  CuAssertIntEquals(tc, 0, mkdir("plrfiles/U-Z", 0700));
  reset_output(&fixture);
  first_saved = save_char_checked(&fixture.ch, 0);
  second_saved = save_char_checked(&fixture.ch, 0);
  quiet = fixture.descriptor.output[0] == '\0';
  if (get_filename(filename, sizeof(filename), PLR_FILE, name))
    unlink(filename);
  unlink("plrfiles/index");
  rmdir("plrfiles/U-Z");
  rmdir("plrfiles");
  restored = chdir(directory);
  rmdir(temporary_directory);
  player_table = saved_table;
  top_of_p_table = saved_top;
  fixture.ch.player.name = fixture_name;

  CuAssertIntEquals(tc, 0, restored);
  CuAssertTrue(tc, first_saved);
  CuAssertTrue(tc, second_saved);
  CuAssertTrue(tc, quiet);
  CuAssertPtrEquals(tc, &provider, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &bracer, GET_EQ(&fixture.ch, WEAR_WRIST_L));
  CuAssertPtrEquals(tc, &offhand, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  CuAssertIntEquals(tc, 0, fixture.ch.limb_defer);

  end_four_arm_fixture(&fixture);
}

/* Item bonuses of a one-hander double only with an actual spare hand: not on
 * a one-armed body, not when every hand is already in use, and a third-hand
 * weapon only with a hand left once the primary takes its own. */
void TestArmCountOneHanderDoublingNeedsSpareHand(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data sword, big, third, bow;
  int base_str, base_dex;

  begin_four_arm_fixture(&fixture);
  fixture.ch.real_abils.str = 10;
  fixture.ch.aff_abils.str = 10;
  fixture.ch.real_abils.dex = 10;
  fixture.ch.aff_abils.dex = 10;
  init_weapon(&sword, "a mighty sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  sword.affected[0].location = APPLY_STR;
  sword.affected[0].modifier = 2;
  sword.affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  init_weapon(&big, "a test greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_weapon(&third, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  third.affected[0].location = APPLY_DEX;
  third.affected[0].modifier = 2;
  third.affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  init_weapon(&bow, "a test bow", WEAPON_TYPE_LONG_BOW, SIZE_MEDIUM);
  affect_total(&fixture.ch);
  base_str = GET_STR(&fixture.ch);
  base_dex = GET_DEX(&fixture.ch);

  /* two arms, lone sword: the spare hand doubles it */
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&sword, &fixture.ch));
  CuAssertIntEquals(tc, base_str + 4, GET_STR(&fixture.ch));
  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_1));

  /* one arm: no spare hand, single bonus */
  set_human_arms(1);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  CuAssertTrue(tc, !is_weapon_wielded_two_handed(&sword, &fixture.ch));
  CuAssertIntEquals(tc, base_str + 2, GET_STR(&fixture.ch));
  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_1));

  /* three hands all in use (a mixed first pair beside a vestigial arm) */
  set_human_arms(2);
  set_vestigial_arm(&fixture, true);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  equip_char(&fixture.ch, &big, WEAR_WIELD_2H);
  CuAssertIntEquals(tc, 3, hands_used(&fixture.ch));
  CuAssertTrue(tc, !is_weapon_wielded_two_handed(&sword, &fixture.ch));
  CuAssertIntEquals(tc, base_str + 2, GET_STR(&fixture.ch));
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&big, &fixture.ch));
  CuAssertPtrEquals(tc, &big, unequip_char(&fixture.ch, WEAR_WIELD_2H));
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&sword, &fixture.ch));
  set_vestigial_arm(&fixture, false);
  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_1));

  /* three arms with a third-hand weapon: the primary takes the one spare
   * hand, so only its bonus doubles */
  set_human_arms(3);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  equip_char(&fixture.ch, &third, WEAR_WIELD_3);
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&sword, &fixture.ch));
  CuAssertTrue(tc, !is_weapon_wielded_two_handed(&third, &fixture.ch));
  CuAssertIntEquals(tc, base_str + 4, GET_STR(&fixture.ch));
  CuAssertIntEquals(tc, base_dex + 2, GET_DEX(&fixture.ch));
  strip_character(&fixture.ch);

  /* four arms: a spare hand for each, both double */
  set_human_arms(4);
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  equip_char(&fixture.ch, &third, WEAR_WIELD_3);
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&sword, &fixture.ch));
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&third, &fixture.ch));
  CuAssertIntEquals(tc, base_str + 4, GET_STR(&fixture.ch));
  CuAssertIntEquals(tc, base_dex + 4, GET_DEX(&fixture.ch));
  strip_character(&fixture.ch);

  /* a launcher already holds its two hands */
  set_human_arms(2);
  equip_char(&fixture.ch, &bow, WEAR_WIELD_1);
  CuAssertIntEquals(tc, 2, hands_used(&fixture.ch));
  CuAssertTrue(tc, is_weapon_wielded_two_handed(&bow, &fixture.ch));

  end_four_arm_fixture(&fixture);
}

struct sheath_case
{
  struct obj_data sheath;
  struct obj_data *primary;
  struct obj_data *secondary;
};

static void fill_sheath(struct four_arm_fixture *fixture, struct sheath_case *sheath,
                        struct obj_data *primary, struct obj_data *secondary)
{
  if (GET_EQ(&fixture->ch, WEAR_SHEATH) == NULL)
  {
    init_armor(&sheath->sheath, "a test sheath", ITEM_WEAR_SHEATH, 0, 0);
    equip_char(&fixture->ch, &sheath->sheath, WEAR_SHEATH);
  }
  sheath->sheath.sheath_primary = primary;
  sheath->sheath.sheath_secondary = secondary;
  reset_output(fixture);
  do_unsheath(&fixture->ch, "", 0, 0);
}

/* every object is in exactly one place: sheathed, worn or carried */
static bool sheath_item_accounted(struct char_data *ch, struct sheath_case *sheath,
                                  struct obj_data *obj)
{
  int places = 0;

  if (sheath->sheath.sheath_primary == obj || sheath->sheath.sheath_secondary == obj)
    places++;
  if (worn_position(ch, obj) >= 0)
    places++;
  if (obj->carried_by == ch)
    places++;
  return places == 1;
}

/* Unsheathing draws each item only into an open, empty first-pair position
 * with the hands to spare; what cannot be drawn stays sheathed for a later
 * unsheath, and only drawn items are reported. */
void TestArmCountUnsheathChecksEachItem(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct sheath_case sheath;
  struct obj_data sword, off, shield, big, huge, orb;

  begin_four_arm_fixture(&fixture);
  memset(&sheath, 0, sizeof(sheath));
  init_weapon(&sword, "a sheathed sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&off, "a sheathed dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  init_armor(&shield, "a slung shield", ITEM_WEAR_SHIELD, 0, 0);
  init_weapon(&big, "a sheathed greatsword", WEAPON_TYPE_GREAT_SWORD, SIZE_LARGE);
  init_weapon(&huge, "an enormous blade", WEAPON_TYPE_GREAT_SWORD, SIZE_HUGE);

  /* two arms: both draw */
  fill_sheath(&fixture, &sheath, &sword, &off);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &off, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, NULL, sheath.sheath.sheath_primary);
  CuAssertPtrEquals(tc, NULL, sheath.sheath.sheath_secondary);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "You unsheath a sheathed sword and"));
  unequip_char(&fixture.ch, WEAR_WIELD_1);
  unequip_char(&fixture.ch, WEAR_WIELD_OFFHAND);

  /* one arm: the primary draws, the secondary shield has no hand and stays */
  set_human_arms(1);
  fill_sheath(&fixture, &sheath, &sword, &shield);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.ch, WEAR_SHIELD));
  CuAssertPtrEquals(tc, NULL, sheath.sheath.sheath_primary);
  CuAssertPtrEquals(tc, &shield, sheath.sheath.sheath_secondary);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "You unsheath a sheathed sword."));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "cannot draw a slung shield"));
  CuAssertTrue(tc, sheath_item_accounted(&fixture.ch, &sheath, &shield));
  unequip_char(&fixture.ch, WEAR_WIELD_1);

  /* one arm and the vestigial arm: the shield fits, the offhand weapon never */
  set_vestigial_arm(&fixture, true);
  fill_sheath(&fixture, &sheath, &sword, &shield);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &shield, GET_EQ(&fixture.ch, WEAR_SHIELD));
  unequip_char(&fixture.ch, WEAR_WIELD_1);
  unequip_char(&fixture.ch, WEAR_SHIELD);
  fill_sheath(&fixture, &sheath, &sword, &off);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &off, sheath.sheath.sheath_secondary);
  CuAssertTrue(tc, sheath_item_accounted(&fixture.ch, &sheath, &off));
  unequip_char(&fixture.ch, WEAR_WIELD_1);

  /* secondary only: a two-hand primary has no two-hand position at one arm */
  fill_sheath(&fixture, &sheath, &big, &shield);
  CuAssertPtrEquals(tc, &big, sheath.sheath.sheath_primary);
  CuAssertPtrEquals(tc, &shield, GET_EQ(&fixture.ch, WEAR_SHIELD));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "You unsheath a slung shield."));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "You unsheath a sheathed great"));
  CuAssertTrue(tc, sheath_item_accounted(&fixture.ch, &sheath, &big));
  unequip_char(&fixture.ch, WEAR_SHIELD);
  set_vestigial_arm(&fixture, false);

  /* zero arms: nothing draws, nothing is lost or duplicated */
  set_human_arms(0);
  fill_sheath(&fixture, &sheath, &sword, &off);
  CuAssertPtrEquals(tc, &sword, sheath.sheath.sheath_primary);
  CuAssertPtrEquals(tc, &off, sheath.sheath.sheath_secondary);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "You unsheath"));
  CuAssertTrue(tc, sheath_item_accounted(&fixture.ch, &sheath, &sword));
  CuAssertTrue(tc, sheath_item_accounted(&fixture.ch, &sheath, &off));

  /* two arms and a held orb: the sword draws, the dagger has no hand; once
   * the orb is put away, unsheath again draws the dagger beside the sword */
  set_human_arms(2);
  init_held(&orb, "a glowing orb");
  equip_char(&fixture.ch, &orb, WEAR_HOLD_1);
  fill_sheath(&fixture, &sheath, &sword, &off);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &off, sheath.sheath.sheath_secondary);
  CuAssertPtrEquals(tc, &orb, unequip_char(&fixture.ch, WEAR_HOLD_1));
  reset_output(&fixture);
  do_unsheath(&fixture.ch, "", 0, 0);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &off, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  CuAssertPtrEquals(tc, NULL, sheath.sheath.sheath_secondary);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "You unsheath a sheathed dagger."));
  unequip_char(&fixture.ch, WEAR_WIELD_1);
  unequip_char(&fixture.ch, WEAR_WIELD_OFFHAND);

  /* two arms: an oversized primary stays; a two-hand secondary cannot be an
   * offhand weapon */
  set_human_arms(2);
  fill_sheath(&fixture, &sheath, &huge, &off);
  CuAssertPtrEquals(tc, &huge, sheath.sheath.sheath_primary);
  CuAssertPtrEquals(tc, &off, GET_EQ(&fixture.ch, WEAR_WIELD_OFFHAND));
  unequip_char(&fixture.ch, WEAR_WIELD_OFFHAND);
  fill_sheath(&fixture, &sheath, &sword, &big);
  CuAssertPtrEquals(tc, &sword, GET_EQ(&fixture.ch, WEAR_WIELD_1));
  CuAssertPtrEquals(tc, &big, sheath.sheath.sheath_secondary);
  CuAssertIntEquals(tc, 0, count_carried(&fixture.ch));
  unequip_char(&fixture.ch, WEAR_WIELD_1);
  sheath.sheath.sheath_primary = NULL;
  sheath.sheath.sheath_secondary = NULL;

  end_four_arm_fixture(&fixture);
}

#define RETURN_NUM_ATTACKS 1
#define DISPLAY_ROUTINE_POTENTIAL 2
#define NORMAL_ATTACK_ROUTINE 0
#define PHASE_0 0
#define PHASE_1 1
#define PHASE_2 2
#define PHASE_3 3

/* ---- Arm count combat ---- */

/* Three arms swing the third hand but never a fourth; empty non-monk lower
 * hands swing nothing; five arms attack exactly like four. */
void TestArmCountAttackRoutinesFollowTheCount(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct obj_data first, third, fourth;
  int four_arms;

  begin_combat_fixture(&fixture);
  GET_CLASS(&fixture.actor) = CLASS_ROGUE; /* trained: every mirror roll is 100 percent */
  init_weapon(&first, "a first dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  set_weapon_dice(&first, 1, 1);
  init_weapon(&third, "a third maul", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&third, 50, 1);
  init_weapon(&fourth, "a fourth maul", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&fourth, 50, 1);

  MOB_HAS_FEAT(&fixture.actor, FEAT_EXTRA_ARMS) = 1;
  CuAssertIntEquals(tc, 3, arm_count(&fixture.actor));
  equip_char(&fixture.actor, &first, WEAR_WIELD_1);
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);
  CuAssertPtrEquals(tc, NULL, GET_EQ(&fixture.actor, WEAR_WIELD_4));
  obj_from_char(&fourth);
  CuAssertIntEquals(tc, 2, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_2) >= 40);
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_3) < 40);
  /* the fourth hand stays shut to a lower double weapon at three arms too */
  CuAssertTrue(tc, !character_can_use_wear_slot(&fixture.actor, WEAR_WIELD_2H_2));

  /* Extra Arms no longer swings on its own: empty lower hands add nothing */
  CuAssertPtrEquals(tc, &third, unequip_char(&fixture.actor, WEAR_WIELD_3));
  MOB_HAS_FEAT(&fixture.actor, FEAT_EXTRA_ARMS) = 2;
  CuAssertIntEquals(tc, 4, arm_count(&fixture.actor));
  CuAssertIntEquals(tc, 1, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_2) < 40);

  /* five arms: the same candidates as four for the same equipment */
  MOB_HAS_FEAT(&fixture.actor, FEAT_EXTRA_ARMS) = 0;
  MOB_HAS_FEAT(&fixture.actor, FEAT_FOUR_ARMS) = 1;
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);
  four_arms = perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0);
  CuAssertIntEquals(tc, 3, four_arms);
  MOB_HAS_FEAT(&fixture.actor, FEAT_EXTRA_ARMS) = 1;
  CuAssertIntEquals(tc, 5, arm_count(&fixture.actor));
  CuAssertIntEquals(tc, four_arms, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertTrue(tc, damage_in_phase(&fixture, PHASE_3) >= 40);

  end_combat_fixture(&fixture);
}

/* A failed mirror roll never moves a later candidate: at 75 percent the third
 * hand keeps phase 2 and the fourth keeps phase 3, hit or miss. */
void TestArmCountFailedMirrorRollsKeepPhases(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct obj_data first, third, fourth;
  int round, dealt, third_hits = 0, third_misses = 0, fourth_hits = 0, stray = 0;

  begin_combat_fixture(&fixture);
  GET_CLASS(&fixture.actor) = CLASS_WARRIOR; /* NPC two-weapon training only: 75 percent */
  MOB_HAS_FEAT(&fixture.actor, FEAT_FOUR_ARMS) = 1;
  init_weapon(&first, "a first dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  set_weapon_dice(&first, 1, 1);
  init_weapon(&third, "a third maul", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&third, 60, 1);
  init_weapon(&fourth, "a fourth club", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  set_weapon_dice(&fourth, 25, 1);
  equip_char(&fixture.actor, &first, WEAR_WIELD_1);
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);
  /* floor(0.75 + 0.75) */
  CuAssertIntEquals(tc, 2, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));

  for (round = 0; round < 40; round++)
  {
    fixture.actor.char_specials.attack_round.drawn = FALSE; /* phases 2 and 3 of a new round */
    GET_HITROLL(&fixture.actor) = 100;
    GET_HIT(&fixture.victim) = 100000;
    GET_POS(&fixture.victim) = POS_STANDING;
    perform_attacks(&fixture.actor, NORMAL_ATTACK_ROUTINE, PHASE_2);
    dealt = 100000 - GET_HIT(&fixture.victim);
    if (dealt >= 50)
      third_hits++;
    else if (dealt == 0)
      third_misses++;
    else
      stray++;
    GET_HIT(&fixture.victim) = 100000;
    perform_attacks(&fixture.actor, NORMAL_ATTACK_ROUTINE, PHASE_3);
    dealt = 100000 - GET_HIT(&fixture.victim);
    if (dealt >= 50)
      stray++;
    else if (dealt >= 10)
      fourth_hits++;
  }
  CuAssertTrue(tc, third_hits > 0);
  CuAssertTrue(tc, third_misses > 0);
  CuAssertTrue(tc, fourth_hits > 0);
  CuAssertIntEquals(tc, 0, stray);

  end_combat_fixture(&fixture);
}

static void make_pc_monk_actor(struct four_arm_combat_fixture *fixture,
                               struct player_special_data *specials, int monk_class)
{
  REMOVE_BIT_AR(MOB_FLAGS(&fixture->actor), MOB_ISNPC);
  fixture->actor.player_specials = specials;
  fixture->actor.player.title = CuMutableString("");
  GET_REAL_RACE(&fixture->actor) = RACE_HUMAN;
  GET_LEVEL(&fixture->actor) = 5;
  GET_CLASS(&fixture->actor) = monk_class;
  CLASS_LEVEL((&fixture->actor), monk_class) = 5;
  fixture->actor.real_abils.str = fixture->actor.aff_abils.str = 16;
  fixture->actor.real_abils.dex = fixture->actor.aff_abils.dex = 16;
  fixture->actor.real_abils.con = fixture->actor.aff_abils.con = 16;
  fixture->actor.real_abils.wis = fixture->actor.aff_abils.wis = 16;
  SET_FEAT(&fixture->actor, FEAT_TWO_WEAPON_FIGHTING, 1);
  SET_FEAT(&fixture->actor, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, 1);
}

/* Real rounds on a PC monk with three arms and nothing in hand: the empty
 * third position strikes in its own phase; the same body as a warrior does
 * not. */
void TestArmCountMonkEmptyThirdHandStrikes(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct player_special_data specials;
  int phase1, phase2, phase3;

  begin_combat_fixture(&fixture);
  memset(&specials, 0, sizeof(specials));
  make_pc_monk_actor(&fixture, &specials, CLASS_MONK);
  SET_FEAT(&fixture.actor, FEAT_EXTRA_ARMS, 1);
  affect_total(&fixture.actor);
  CuAssertIntEquals(tc, 3, arm_count(&fixture.actor));
  CuAssertTrue(tc, monk_gear_ok(&fixture.actor));
  CuAssertIntEquals(tc, 2, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));

  phase1 = damage_in_phase(&fixture, PHASE_1);
  phase2 = damage_in_phase(&fixture, PHASE_2);
  phase3 = damage_in_phase(&fixture, PHASE_3);
  CuAssertTrue(tc, phase1 > 0);
  CuAssertTrue(tc, phase2 > 0);
  CuAssertIntEquals(tc, 0, phase3);

  /* the same body without monk levels: no unarmed third hand */
  CLASS_LEVEL((&fixture.actor), CLASS_MONK) = 0;
  GET_CLASS(&fixture.actor) = CLASS_WARRIOR;
  CLASS_LEVEL((&fixture.actor), CLASS_WARRIOR) = 5;
  CuAssertIntEquals(tc, 1, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));
  CuAssertIntEquals(tc, 0, damage_in_phase(&fixture, PHASE_2));

  fixture.actor.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  end_combat_fixture(&fixture);
}

/* one real round, phases 1..3, with an optional change after phase 1 */
static void run_real_round(struct four_arm_combat_fixture *fixture,
                           void (*between_phases)(struct four_arm_combat_fixture *))
{
  int phase;

  test_reset_second_pair_swings();
  for (phase = PHASE_1; phase <= PHASE_3; phase++)
  {
    GET_HITROLL(&fixture->actor) = 100;
    GET_HIT(&fixture->victim) = 100000;
    GET_POS(&fixture->victim) = POS_STANDING;
    perform_violence(&fixture->actor, phase);
    if (phase == PHASE_1 && between_phases != NULL)
      between_phases(fixture);
  }
}

/* one real round; returns how many second-pair ordinals did not swing exactly
 * once: expected ordinals are first + 1 ..first + candidates */
static int second_pair_round_misfits(struct four_arm_combat_fixture *fixture, int candidates,
                                     void (*between_phases)(struct four_arm_combat_fixture *))
{
  const struct attack_round_plan *plan = &fixture->actor.char_specials.attack_round;
  int ordinal, want, misfits = 0;

  run_real_round(fixture, between_phases);
  if (candidates < 0)
    candidates = plan->air_embodiment ? 4 : 3;
  for (ordinal = 0; ordinal < TEST_SECOND_PAIR_ORDINALS; ordinal++)
  {
    want = ordinal > plan->second_pair_first_ordinal &&
           ordinal <= plan->second_pair_first_ordinal + candidates;
    if (test_get_second_pair_swings(ordinal) != want)
      misfits++;
  }
  return misfits;
}

static struct obj_data round_plan_offhand;

static void wield_round_plan_offhand(struct four_arm_combat_fixture *fixture)
{
  equip_char(&fixture->actor, &round_plan_offhand, WEAR_WIELD_OFFHAND);
}

/* A round draws its extra-attack procs once: phases 2 and 3 replay phase 1's
 * Air Embodiment roll and its second-pair numbering, so every lower-hand
 * candidate swings exactly once per round, and the next round draws again. */
void TestArmCountRoundPlanKeepsSecondPairPhases(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct player_special_data specials;
  struct obj_data first, third, fourth;
  int round, misfits = 0, air_rounds = 0;

  begin_combat_fixture(&fixture);
  memset(&specials, 0, sizeof(specials));
  /* trained PC fighter: every mirror roll is 100 percent, and a dual second
   * pair adds the improved fourth-hand candidate: three candidates, four with
   * the Air Embodiment attack */
  make_pc_monk_actor(&fixture, &specials, CLASS_WARRIOR);
  SET_FEAT(&fixture.actor, FEAT_FOUR_ARMS, 1);
  affect_total(&fixture.actor);
  CuAssertIntEquals(tc, 4, arm_count(&fixture.actor));
  init_weapon(&first, "a first dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  init_weapon(&third, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&fourth, "a fourth sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&round_plan_offhand, "an offhand dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  equip_char(&fixture.actor, &first, WEAR_WIELD_1);
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);
  CuAssertIntEquals(tc, 4, perform_attacks(&fixture.actor, RETURN_NUM_ATTACKS, PHASE_0));

  GET_ELEMENTAL_EMBODIMENT_TIMER(&fixture.actor) = 10;
  GET_ELEMENTAL_EMBODIMENT_TYPE(&fixture.actor) = 3;
  for (round = 0; round < 200; round++)
  {
    misfits += second_pair_round_misfits(&fixture, -1, NULL);
    if (fixture.actor.char_specials.attack_round.air_embodiment)
      air_rounds++;
  }
  CuAssertIntEquals(tc, 0, misfits);
  /* each round drew anew: some rounds got the extra attack, most did not */
  CuAssertTrue(tc, air_rounds > 0 && air_rounds < 200);

  /* an offhand weapon wielded after phase 1 lengthens the ordinary attacks,
   * but the round keeps phase 1's second-pair numbering */
  GET_ELEMENTAL_EMBODIMENT_TIMER(&fixture.actor) = 0;
  CuAssertIntEquals(tc, 0, second_pair_round_misfits(&fixture, 3, wield_round_plan_offhand));
  CuAssertIntEquals(tc, 1, fixture.actor.char_specials.attack_round.second_pair_first_ordinal);
  /* the next round numbers after the longer first pair */
  CuAssertIntEquals(tc, 0, second_pair_round_misfits(&fixture, 3, NULL));
  CuAssertIntEquals(tc, 3, fixture.actor.char_specials.attack_round.second_pair_first_ordinal);

  fixture.actor.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  end_combat_fixture(&fixture);
}

static void gain_haste(struct four_arm_combat_fixture *fixture)
{
  SET_BIT_AR(AFF_FLAGS(&fixture->actor), AFF_HASTE);
}

static void lose_haste(struct four_arm_combat_fixture *fixture)
{
  REMOVE_BIT_AR(AFF_FLAGS(&fixture->actor), AFF_HASTE);
}

static void drop_fourth_weapon(struct four_arm_combat_fixture *fixture)
{
  unequip_char(&fixture->actor, WEAR_WIELD_4);
}

/* swings of each second-pair candidate in the last round: third, fourth,
 * haste and improved fourth-hand */
static bool second_pair_round_swings(int third, int fourth, int haste, int improved)
{
  return test_count_second_pair_label_swings("Third hand") == third &&
         test_count_second_pair_label_swings("Fourth hand") == fourth &&
         test_count_second_pair_label_swings("Third hand (Haste)") == haste &&
         test_count_second_pair_label_swings("Fourth hand (Improved 2 Weapon Fighting)") ==
             improved;
}

/* The round lists its second-pair candidates once: haste gained after phase
 * 1 adds no candidate and moves none (the improved fourth-hand swing stays
 * once a round), and a candidate whose haste or weapon is gone by its phase
 * keeps its ordinal but does not swing. */
void TestArmCountRoundPlanKeepsSecondPairCandidates(CuTest *tc)
{
  struct four_arm_combat_fixture fixture;
  struct player_special_data specials;
  struct obj_data first, third, fourth;

  begin_combat_fixture(&fixture);
  memset(&specials, 0, sizeof(specials));
  /* trained PC fighter, level 5: every mirror roll is 100 percent */
  make_pc_monk_actor(&fixture, &specials, CLASS_WARRIOR);
  SET_FEAT(&fixture.actor, FEAT_FOUR_ARMS, 1);
  affect_total(&fixture.actor);
  init_weapon(&first, "a first dagger", WEAPON_TYPE_DAGGER, SIZE_MEDIUM);
  init_weapon(&third, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_weapon(&fourth, "a fourth sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  equip_char(&fixture.actor, &first, WEAR_WIELD_1);
  equip_char(&fixture.actor, &third, WEAR_WIELD_3);
  equip_char(&fixture.actor, &fourth, WEAR_WIELD_4);

  /* third = 2, fourth = 3, improved fourth = 4 (phase 1); haste arrives
   * after phase 1 */
  run_real_round(&fixture, gain_haste);
  CuAssertTrue(tc, AFF_FLAGGED(&fixture.actor, AFF_HASTE));
  CuAssertTrue(tc, second_pair_round_swings(1, 1, 0, 1));

  /* hasted from the start: the haste swing is listed and swings once */
  run_real_round(&fixture, NULL);
  CuAssertIntEquals(tc, 2, fixture.actor.char_specials.attack_round.second_pair_first_ordinal);
  CuAssertTrue(tc, second_pair_round_swings(1, 1, 1, 1));

  /* haste ends after phase 1: its swing (5, phase 2) is gone, the rest stay */
  run_real_round(&fixture, lose_haste);
  CuAssertTrue(tc, second_pair_round_swings(1, 1, 0, 1));

  /* the fourth weapon leaves after phase 1: the improved swing already came
   * in phase 1, the fourth-hand swing (3, phase 3) does not come */
  run_real_round(&fixture, drop_fourth_weapon);
  CuAssertTrue(tc, second_pair_round_swings(1, 0, 0, 1));

  fixture.actor.player_specials = &dummy_mob;
  SET_BIT_AR(MOB_FLAGS(&fixture.actor), MOB_ISNPC);
  end_combat_fixture(&fixture);
}

/* the damage dice printed on the display row that starts with label */
static bool display_row_has(struct four_arm_fixture *fixture, const char *label, const char *weapon,
                            const char *dice)
{
  const char *row = strstr(fixture->descriptor.output, label);
  const char *found_weapon, *found_dice;

  if (row == NULL)
    return false;
  found_weapon = strstr(row, weapon);
  found_dice = strstr(row, "Damage Dice: ");
  return found_weapon != NULL && found_dice != NULL && found_weapon < found_dice &&
         strncmp(found_dice + strlen("Damage Dice: "), dice, strlen(dice)) == 0;
}

static void show_routine(struct four_arm_fixture *fixture)
{
  reset_output(fixture);
  perform_attacks(&fixture->ch, DISPLAY_ROUTINE_POTENTIAL, PHASE_0);
}

static void make_fixture_monk(struct four_arm_fixture *fixture, int monk_class)
{
  GET_LEVEL(&fixture->ch) = 5;
  GET_CLASS(&fixture->ch) = monk_class;
  CLASS_LEVEL((&fixture->ch), monk_class) = 5;
}

/* Monk third hand: an empty third position offers unarmed candidates with
 * monk dice for monks and sacred fists at three arms and up, mirrors flurry,
 * needs monk gear, and never gives an empty fourth hand a swing. Zero and one
 * arm keep the two-armed unarmed opportunities. */
void TestArmCountMonkThirdHandCandidates(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data kama, sword, plate;
  int two_arms, flurry_two_arms;

  begin_four_arm_fixture(&fixture);
  make_fixture_monk(&fixture, CLASS_MONK);
  init_weapon(&kama, "a monk kama", WEAPON_TYPE_KAMA, SIZE_MEDIUM);
  set_weapon_dice(&kama, 9, 9);
  init_weapon(&sword, "a plain sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);
  init_armor(&plate, "a plate cuirass", ITEM_WEAR_BODY, 8, SPEC_ARMOR_TYPE_FULL_PLATE);
  CuAssertIntEquals(tc, WEAPON_FAMILY_MONK, weapon_list[WEAPON_TYPE_KAMA].weaponFamily);
  CuAssertTrue(tc, monk_gear_ok(&fixture.ch));

  two_arms = perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0);
  set_human_arms(1);
  CuAssertIntEquals(tc, two_arms, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  set_human_arms(0);
  CuAssertIntEquals(tc, two_arms, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  show_routine(&fixture);
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "Mainhand, Attack Bonus"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));
  set_human_arms(2);
  show_routine(&fixture);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));

  /* three arms: one unarmed third-hand candidate with monk dice at 50 percent */
  set_human_arms(3);
  CuAssertIntEquals(tc, two_arms, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  show_routine(&fixture);
  CuAssertTrue(tc, display_row_has(&fixture, "Third hand, Attack Bonus", "Bare-hands", "1D8"));
  CuAssertPtrNotNull(tc, strstr(fixture.descriptor.output, "(50% chance)"));
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Fourth hand"));
  SET_FEAT(&fixture.ch, FEAT_TWO_WEAPON_FIGHTING, 1);
  SET_FEAT(&fixture.ch, FEAT_IMPROVED_TWO_WEAPON_FIGHTING, 1);
  CuAssertIntEquals(tc, two_arms + 1, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));

  /* flurry is mirrored as a third-hand bonus candidate */
  SET_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_FLURRY_OF_BLOWS);
  set_human_arms(2);
  flurry_two_arms = perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0);
  set_human_arms(3);
  CuAssertIntEquals(tc, flurry_two_arms + 2,
                    perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  show_routine(&fixture);
  CuAssertTrue(tc, display_row_has(&fixture, "Third hand Bonus", "Bare-hands", "1D8"));
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.ch), AFF_FLURRY_OF_BLOWS);

  /* four and five arms: the same candidates, no empty fourth-hand swing */
  set_human_arms(4);
  CuAssertIntEquals(tc, two_arms + 1, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  set_human_arms(5);
  CuAssertIntEquals(tc, two_arms + 1, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  show_routine(&fixture);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Fourth hand"));

  /* an upper monk weapon keeps the unarmed third hand, with unarmed dice,
   * although the character is no longer bare-handed */
  set_human_arms(4);
  equip_char(&fixture.ch, &kama, WEAR_WIELD_1);
  CuAssertTrue(tc, !is_bare_handed(&fixture.ch));
  CuAssertTrue(tc, monk_gear_ok(&fixture.ch));
  show_routine(&fixture);
  CuAssertTrue(tc, display_row_has(&fixture, "Third hand, Attack Bonus", "Bare-hands", "1D8"));
  CuAssertPtrEquals(tc, &kama, unequip_char(&fixture.ch, WEAR_WIELD_1));

  /* a fourth-hand monk weapon beside an empty third: both hands attack */
  equip_char(&fixture.ch, &kama, WEAR_WIELD_4);
  show_routine(&fixture);
  CuAssertTrue(tc, display_row_has(&fixture, "Third hand, Attack Bonus", "Bare-hands", "1D8"));
  CuAssertTrue(tc, display_row_has(&fixture, "Fourth hand, Attack Bonus", "a monk kama", "9D9"));
  CuAssertPtrEquals(tc, &kama, unequip_char(&fixture.ch, WEAR_WIELD_4));

  /* a non-monk weapon or armor breaks monk gear: no unarmed third hand */
  equip_char(&fixture.ch, &sword, WEAR_WIELD_1);
  show_routine(&fixture);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));
  CuAssertPtrEquals(tc, &sword, unequip_char(&fixture.ch, WEAR_WIELD_1));
  equip_char(&fixture.ch, &plate, WEAR_BODY);
  CuAssertTrue(tc, !monk_gear_ok(&fixture.ch));
  show_routine(&fixture);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));
  CuAssertPtrEquals(tc, &plate, unequip_char(&fixture.ch, WEAR_BODY));

  /* sacred fists are monks here too */
  CLASS_LEVEL((&fixture.ch), CLASS_MONK) = 0;
  make_fixture_monk(&fixture, CLASS_SACRED_FIST);
  show_routine(&fixture);
  CuAssertTrue(tc, display_row_has(&fixture, "Third hand, Attack Bonus", "Bare-hands", "1D8"));

  /* anyone else leaves empty lower hands idle */
  CLASS_LEVEL((&fixture.ch), CLASS_SACRED_FIST) = 0;
  make_fixture_monk(&fixture, CLASS_WARRIOR);
  CuAssertIntEquals(tc, two_arms, perform_attacks(&fixture.ch, RETURN_NUM_ATTACKS, PHASE_0));
  show_routine(&fixture);
  CuAssertPtrEquals(tc, NULL, strstr(fixture.descriptor.output, "Third hand"));

  end_four_arm_fixture(&fixture);
}

/* Monk glove bonuses stay whole-character bare-hand bonuses: the unarmed
 * third hand gains them only while nothing is wielded. */
void TestArmCountMonkGlovesNeedBareHands(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data kama, gloves;
  int without_gloves, with_gloves;

  begin_four_arm_fixture(&fixture);
  make_fixture_monk(&fixture, CLASS_MONK);
  set_human_arms(3);
  init_weapon(&kama, "a monk kama", WEAPON_TYPE_KAMA, SIZE_MEDIUM);
  init_armor(&gloves, "monk gloves", ITEM_WEAR_HANDS, 3, 0);

  without_gloves = compute_damage_bonus(&fixture.ch, &fixture.ch, NULL, TYPE_HIT, 0,
                                        MODE_NORMAL_HIT, ATTACK_TYPE_THIRD);
  equip_char(&fixture.ch, &gloves, WEAR_HANDS);
  with_gloves = compute_damage_bonus(&fixture.ch, &fixture.ch, NULL, TYPE_HIT, 0, MODE_NORMAL_HIT,
                                     ATTACK_TYPE_THIRD);
  CuAssertIntEquals(tc, without_gloves + 3, with_gloves);
  CuAssertPtrEquals(tc, &gloves, unequip_char(&fixture.ch, WEAR_HANDS));

  equip_char(&fixture.ch, &kama, WEAR_WIELD_1);
  without_gloves = compute_damage_bonus(&fixture.ch, &fixture.ch, NULL, TYPE_HIT, 0,
                                        MODE_NORMAL_HIT, ATTACK_TYPE_THIRD);
  equip_char(&fixture.ch, &gloves, WEAR_HANDS);
  with_gloves = compute_damage_bonus(&fixture.ch, &fixture.ch, NULL, TYPE_HIT, 0, MODE_NORMAL_HIT,
                                     ATTACK_TYPE_THIRD);
  CuAssertIntEquals(tc, without_gloves, with_gloves);

  /* zero arms: no glove position at all */
  strip_character(&fixture.ch);
  set_human_arms(0);
  wear_from_inventory(&fixture, &gloves, WEAR_HANDS);
  CuAssertIntEquals(tc, -1, worn_position(&fixture.ch, &gloves));

  end_four_arm_fixture(&fixture);
}

/* whether the third hand's strength line names a free lower hand */
static bool third_hand_has_support(struct four_arm_fixture *fixture)
{
  reset_output(fixture);
  compute_damage_bonus(&fixture->ch, &fixture->ch, get_wielded(&fixture->ch, ATTACK_TYPE_THIRD),
                       TYPE_UNDEFINED, 0, MODE_DISPLAY_PRIMARY, ATTACK_TYPE_THIRD);
  return strstr(fixture->descriptor.output, "free lower hand") != NULL;
}

/* An unarmed strike fills a hand no equipped position counts.  Three empty
 * arms: the primary's strike and support hand and the third strike use all
 * three, so the unarmed third hand keeps plain strength; a fourth arm is its
 * support hand.  A first-pair one-hander leaves three arms no support hand
 * either. */
void TestArmCountUnarmedThirdHandNeedsARealSpareHand(CuTest *tc)
{
  struct four_arm_fixture fixture;
  struct obj_data kama, sword;

  begin_four_arm_fixture(&fixture);
  make_fixture_monk(&fixture, CLASS_MONK);
  init_weapon(&kama, "a monk kama", WEAPON_TYPE_KAMA, SIZE_MEDIUM);
  init_weapon(&sword, "a third sword", WEAPON_TYPE_LONG_SWORD, SIZE_MEDIUM);

  set_human_arms(3);
  CuAssertTrue(tc, !third_hand_has_support(&fixture));
  set_human_arms(4);
  CuAssertTrue(tc, third_hand_has_support(&fixture));

  set_human_arms(3);
  equip_char(&fixture.ch, &kama, WEAR_WIELD_1);
  CuAssertTrue(tc, !third_hand_has_support(&fixture));
  set_human_arms(4);
  CuAssertTrue(tc, third_hand_has_support(&fixture));

  /* an armed third hand keeps the old rule: beside the first pair's
   * one-hander, four arms leave it a support hand and three do not */
  equip_char(&fixture.ch, &sword, WEAR_WIELD_3);
  CuAssertTrue(tc, third_hand_has_support(&fixture));
  set_human_arms(3);
  CuAssertTrue(tc, !third_hand_has_support(&fixture));

  end_four_arm_fixture(&fixture);
}

#undef RETURN_NUM_ATTACKS
#undef DISPLAY_ROUTINE_POTENTIAL
#undef NORMAL_ATTACK_ROUTINE
#undef PHASE_0
#undef PHASE_1
#undef PHASE_2
#undef PHASE_3
