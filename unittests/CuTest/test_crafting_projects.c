/* Production-linked tests for the materials-and-motes crafting system (crafting_system 2): its boot
 * tables, the equipment project a player builds with the craft command, supply orders, golem
 * construction, material storage, and reforging with the reforge command or a crafting kit. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/act/act.h"
#include "../../src/character/feats.h"
#include "../../src/character/talents.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/config/vnums.h"
#include "../../src/core/comm.h"
#include "../../src/core/constants.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/craft/craft.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/craft/crafting_recipes.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/events/activity_manager.h"
#include "../../src/events/domain_event_runtime.h"
#include "../../src/events/domain_event_world.h"
#include "../../src/events/mud_event.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/treasure.h"

#include <string.h>

/** A connected player in a smithy with a forge and a weaponsmith's hammer, plus the weapon
 * prototype that project objects are built from and the prototype for material bundles. */
struct craft_project_fixture
{
  struct room_data room;
  struct zone_data zone;
  struct index_data object_index[2];
  struct obj_data object_proto[2];
  struct obj_data forge;
  struct obj_data hammer;
  struct char_data quartermaster;
  struct char_data ch;
  struct player_special_data specials;
  struct descriptor_data descriptor;

  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  struct index_data *saved_obj_index;
  struct obj_data *saved_obj_proto;
  obj_rnum saved_top_of_objt;
  struct obj_data *saved_object_list;
};

static void craft_project_reset_output(struct craft_project_fixture *f)
{
  if (f->descriptor.large_outbuf != NULL)
  {
    free(f->descriptor.large_outbuf->text);
    free(f->descriptor.large_outbuf);
    f->descriptor.large_outbuf = NULL;
  }
  f->descriptor.small_outbuf[0] = '\0';
  f->descriptor.output = f->descriptor.small_outbuf;
  f->descriptor.bufptr = 0;
  f->descriptor.bufspace = SMALL_BUFSIZE - 1;
}

static bool craft_project_output_has(struct craft_project_fixture *f, const char *text)
{
  return strstr(f->descriptor.output, text) != NULL;
}

static void craft_project_begin(struct craft_project_fixture *f)
{
  struct char_data *ch = &f->ch;
  int weapon, bundle;

  if (crafting_recipes[CRAFT_RECIPE_WEAPON_LONG_SWORD].object_type != ITEM_WEAPON)
    populate_crafting_recipes();
  if (!IS_SET(weapon_list[WEAPON_TYPE_LONG_BOW].weaponFlags, WEAPON_FLAG_RANGED))
    load_weapons();

  memset(f, 0, sizeof(*f));
  f->saved_world = world;
  f->saved_top_of_world = top_of_world;
  f->saved_zone_table = zone_table;
  f->saved_top_of_zone_table = top_of_zone_table;
  f->saved_obj_index = obj_index;
  f->saved_obj_proto = obj_proto;
  f->saved_top_of_objt = top_of_objt;
  f->saved_object_list = object_list;

  f->room.number = 1;
  f->room.name = CuMutableString("A smithy");
  f->room.description = CuMutableString("A forge glows here.\r\n");
  f->room.people = &f->ch;
  f->room.contents = &f->forge;
  world = &f->room;
  top_of_world = 0;

  f->zone.number = 0;
  f->zone.bot = 0;
  f->zone.top = MAX(WEAPON_PROTO, ITEM_PROTOTYPE);
  zone_table = &f->zone;
  top_of_zone_table = 0;

  clear_object(&f->forge);
  f->forge.name = CuMutableString("forge");
  SET_OBJ_FLAG(&f->forge, ITEM_CRAFTING_FORGE);
  IN_ROOM(&f->forge) = 0;
  clear_object(&f->hammer);

  /* Instances share the prototype strings, which free_obj() leaves alone. The index is sorted
   * by vnum for real_object(). */
  weapon = WEAPON_PROTO < ITEM_PROTOTYPE ? 0 : 1;
  bundle = 1 - weapon;
  clear_object(&f->object_proto[weapon]);
  f->object_proto[weapon].item_number = weapon;
  GET_OBJ_TYPE(&f->object_proto[weapon]) = ITEM_WEAPON;
  f->object_proto[weapon].name = CuMutableString("weapon prototype");
  f->object_proto[weapon].short_description = CuMutableString("a weapon prototype");
  f->object_proto[weapon].description = CuMutableString("A weapon prototype lies here.");
  f->object_index[weapon].vnum = WEAPON_PROTO;
  clear_object(&f->object_proto[bundle]);
  f->object_proto[bundle].item_number = bundle;
  f->object_proto[bundle].name = CuMutableString("item prototype");
  f->object_proto[bundle].short_description = CuMutableString("an item prototype");
  f->object_proto[bundle].description = CuMutableString("An item prototype lies here.");
  f->object_index[bundle].vnum = ITEM_PROTOTYPE;
  obj_proto = f->object_proto;
  obj_index = f->object_index;
  top_of_objt = 1;
  object_list = NULL;

  clear_char(ch);
  ch->player_specials = &f->specials;
  ch->player.name = CuMutableString("Crafter");
  GET_LEVEL(ch) = 30;
  IN_ROOM(ch) = 0;
  GET_CRAFT(ch).craft_variant = -1;
  GET_EQ(ch, WEAR_CRAFT_WEAPON_HAMMER) = &f->hammer;
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 20);

  f->descriptor.output = f->descriptor.small_outbuf;
  f->descriptor.bufspace = SMALL_BUFSIZE - 1;
  f->descriptor.character = &f->ch;
  f->descriptor.connected = CON_PLAYING;
  f->descriptor.pProtocol = ProtocolCreate();
  f->ch.desc = &f->descriptor;
}

/** Put a quartermaster beside the player, for commands that need one. */
static void craft_project_add_quartermaster(struct craft_project_fixture *f)
{
  clear_char(&f->quartermaster);
  SET_BIT_AR(MOB_FLAGS(&f->quartermaster), MOB_ISNPC);
  SET_BIT_AR(MOB_FLAGS(&f->quartermaster), MOB_QUARTERMASTER);
  f->quartermaster.player.short_descr = CuMutableString("the quartermaster");
  IN_ROOM(&f->quartermaster) = 0;
  f->ch.next_in_room = &f->quartermaster;
}

static void craft_project_end(struct craft_project_fixture *f)
{
  struct char_data *ch = &f->ch;

  while (ch->carrying != NULL)
    extract_obj(ch->carrying);
  GET_EQ(ch, WEAR_CRAFT_WEAPON_HAMMER) = NULL;
  free(GET_CRAFT(ch).keywords);
  free(GET_CRAFT(ch).short_description);
  free(GET_CRAFT(ch).room_description);
  free(GET_CRAFT(ch).ex_description);
  craft_project_reset_output(f);
  ProtocolDestroy(f->descriptor.pProtocol);
  ch->desc = NULL;
  ch->next_in_room = NULL;
  domain_event_world_forget_character(ch);

  world = f->saved_world;
  top_of_world = f->saved_top_of_world;
  zone_table = f->saved_zone_table;
  top_of_zone_table = f->saved_top_of_zone_table;
  obj_index = f->saved_obj_index;
  obj_proto = f->saved_obj_proto;
  top_of_objt = f->saved_top_of_objt;
  object_list = f->saved_object_list;
}

/** Everything a steel long sword needs except motes: type, variant, descriptions and materials. */
static void craft_project_ready_long_sword(struct char_data *ch)
{
  GET_CRAFT(ch).crafting_item_type = CRAFT_TYPE_WEAPON;
  GET_CRAFT(ch).crafting_specific = WEAPON_TYPE_LONG_SWORD;
  GET_CRAFT(ch).crafting_recipe = CRAFT_RECIPE_WEAPON_LONG_SWORD;
  GET_CRAFT(ch).craft_variant = 0;
  GET_CRAFT(ch).keywords = strdup("steel long sword");
  GET_CRAFT(ch).short_description = strdup("a steel long sword");
  GET_CRAFT(ch).room_description = strdup("A steel long sword lies here.");
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] = 6;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HIDES][0] = CRAFT_MAT_LOW_GRADE_HIDE;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HIDES][1] = 1;
}

/** A held supply order for three long swords (the long sword recipe's first variant). */
static void craft_project_hold_supply_order(struct char_data *ch)
{
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_SUPPLYORDER;
  GET_CRAFT(ch).crafting_recipe = CRAFT_RECIPE_WEAPON_LONG_SWORD;
  GET_CRAFT(ch).crafting_item_type = ITEM_WEAPON;
  GET_CRAFT(ch).crafting_specific = WEAPON_TYPE_LONG_SWORD;
  GET_CRAFT(ch).craft_variant = 0;
  GET_CRAFT(ch).supply_num_required = 3;
  GET_CRAFT(ch).skill_type = CRAFT_SKILL_WEAPONSMITH;
}

/** A reforgeable, free steel dagger built from the fixture's weapon prototype. */
static struct obj_data *craft_project_reforgeable_dagger(void)
{
  struct obj_data *obj = read_object(WEAPON_PROTO, VIRTUAL);

  if (obj == NULL)
    return NULL;
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  set_weapon_object(obj, WEAPON_TYPE_DAGGER);
  GET_OBJ_MATERIAL(obj) = MATERIAL_STEEL;
  GET_OBJ_COST(obj) = 0;
  SET_OBJ_FLAG(obj, ITEM_REFORGEABLE);
  return obj;
}

static int craft_project_live_objects(void)
{
  struct obj_data *obj;
  int count = 0;

  for (obj = object_list; obj != NULL; obj = obj->next)
    count++;
  return count;
}

/** A fresh game-scheduler event backend at a known pulse, for activities that complete later. */
static void craft_project_start_events(void)
{
  event_free_all();
  event_test_select_backend(EVENT_BACKEND_GAME_SCHEDULER);
  pulse = 200U;
  event_init();
}

/** Advance the scheduler by whole seconds. */
static void craft_project_advance_seconds(int seconds)
{
  int i;

  for (i = 0; i < seconds; i++)
  {
    pulse += PASSES_PER_SEC;
    event_test_advance();
  }
}

void Test_craft_materials_sort_and_list_every_material_once(CuTest *tc)
{
  struct craft_project_fixture f;
  int seen[NUM_CRAFT_MATS] = {0};
  int i, material, previous = 0;
  bool ordered = true, complete = true, listed;
  char first[64];

  sort_materials();
  for (i = 1; i < NUM_CRAFT_MATS; i++)
  {
    material = materials_sort_info[i];
    if (material <= CRAFT_MAT_NONE || material >= NUM_CRAFT_MATS)
    {
      complete = false;
      continue;
    }
    seen[material]++;
    if (previous != 0 && strcmp(crafting_materials[previous], crafting_materials[material]) > 0)
      ordered = false;
    previous = material;
  }
  for (i = 1; i < NUM_CRAFT_MATS; i++)
    if (seen[i] != 1)
      complete = false;

  /* The alphabetically first material is listed too. */
  craft_project_begin(&f);
  material = materials_sort_info[1];
  snprintf(first, sizeof(first), "%s",
           material > 0 && material < NUM_CRAFT_MATS ? crafting_materials[material] : "(none)");
  do_list_craft_materials(&f.ch, "", 0, 0);
  listed = craft_project_output_has(&f, first);
  craft_project_end(&f);

  CuAssertTrue(tc, complete);
  CuAssertTrue(tc, ordered);
  CuAssertTrue(tc, listed);
}

void Test_craft_project_cannot_change_while_its_work_runs(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct primary_activity_snapshot snapshot;
  enum domain_event_status runtime;
  int motes, element;
  bool started, refused, kept, shown, stopped, refunded;

  craft_project_begin(&f);
  event_free_all();
  event_init();
  runtime = domain_event_runtime_init();
  craft_project_ready_long_sword(ch);
  GET_CRAFT(ch).enhancement = 1;
  motes = craft_motes_required(0, 0, 0, 1);
  element = get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, WEAPON_TYPE_LONG_SWORD);
  GET_CRAFT(ch).enhancement_motes_required = motes;

  newcraft_create(ch, "start");
  started = primary_activity_snapshot(ch, &snapshot) && snapshot.type == PRIMARY_ACTIVITY_CRAFT;
  craft_project_reset_output(&f);
  newcraft_create(ch, "reset motes");
  refused = craft_project_output_has(&f, "cannot change your project");
  kept = GET_CRAFT(ch).enhancement_motes_required == motes && GET_CRAFT_MOTES(ch, element) == 0;
  craft_project_reset_output(&f);
  newcraft_create(ch, "show");
  shown = craft_project_output_has(&f, "Current Craft Project");

  /* Once the work stops, the same command refunds the motes. */
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_COMMAND, false);
  stopped = !primary_activity_snapshot(ch, &snapshot);
  newcraft_create(ch, "reset motes");
  refunded = GET_CRAFT(ch).enhancement_motes_required == 0 && GET_CRAFT_MOTES(ch, element) == motes;

  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, started);
  CuAssertTrue(tc, refused);
  CuAssertTrue(tc, kept);
  CuAssertTrue(tc, shown);
  CuAssertTrue(tc, stopped);
  CuAssertTrue(tc, refunded);
}

void Test_craft_completion_makes_nothing_from_an_unpaid_project(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool made, reported, kept;
  int objects;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  /* The enhancement is set but its motes are gone. */
  GET_CRAFT(ch).enhancement = 1;

  craft_create_complete(ch);
  made = ch->carrying != NULL;
  reported = craft_project_output_has(&f, "no longer complete");
  kept = GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 6;
  objects = craft_project_live_objects();
  craft_project_end(&f);

  CuAssertTrue(tc, !made);
  CuAssertTrue(tc, reported);
  CuAssertTrue(tc, kept);
  CuAssertIntEquals(tc, 0, objects);
}

void Test_craft_show_and_failed_completion_release_their_objects(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int after_show, live_after_show, after_failure, live_after_failure;
  bool shown, failed;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);

  show_current_craft(ch);
  shown = craft_project_output_has(&f, "Project DC");
  after_show = craft_project_live_objects();
  live_after_show = f.object_index[0].number + f.object_index[1].number;

  /* A DC no roll reaches fails the check at completion. */
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 0);
  GET_CRAFT(ch).level_adjust = -10;
  craft_project_reset_output(&f);
  craft_create_complete(ch);
  failed = craft_project_output_has(&f, "don't have the skill") && ch->carrying == NULL;
  after_failure = craft_project_live_objects();
  live_after_failure = f.object_index[0].number + f.object_index[1].number;
  craft_project_end(&f);

  CuAssertTrue(tc, shown);
  CuAssertIntEquals(tc, 0, after_show);
  CuAssertIntEquals(tc, 0, live_after_show);
  CuAssertTrue(tc, failed);
  CuAssertIntEquals(tc, 0, after_failure);
  CuAssertIntEquals(tc, 0, live_after_failure);
}

void Test_craft_released_materials_forfeit_efficient_savings(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool metal_cleared, hide_kept, refunded, all_cleared;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HARD_METALS][1] = 3;
  GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HIDES][0] = CRAFT_MAT_LOW_GRADE_HIDE;
  GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HIDES][1] = 1;

  newcraft_create(ch, "materials remove steel");
  metal_cleared = GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HARD_METALS][1] == 0;
  hide_kept = GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HIDES][1] == 1;
  refunded = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) == 6;
  newcraft_create(ch, "reset");
  all_cleared = GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HIDES][1] == 0;
  craft_project_end(&f);

  CuAssertTrue(tc, metal_cleared);
  CuAssertTrue(tc, hide_kept);
  CuAssertTrue(tc, refunded);
  CuAssertTrue(tc, all_cleared);
}

void Test_craft_efficient_talent_uses_the_recipe_skill_from_the_start(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool saved;

  craft_project_begin(&f);
  GET_CRAFT(ch).crafting_item_type = CRAFT_TYPE_WEAPON;
  GET_CRAFT(ch).crafting_specific = WEAPON_TYPE_LONG_SWORD;
  GET_CRAFT(ch).crafting_recipe = CRAFT_RECIPE_WEAPON_LONG_SWORD;
  GET_CRAFT(ch).craft_variant = 0;
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = 6;
  /* 34 ranks is a 102% chance, and no show has recorded a skill yet. */
  f.specials.saved.talent_ranks[TALENT_EFFICIENT_WEAPONSMITHING] = 34;

  newcraft_create(ch, "materials add steel");
  saved = GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HARD_METALS][0] == CRAFT_MAT_STEEL &&
          GET_CRAFT(ch).efficient_saved_materials[CRAFT_GROUP_HARD_METALS][1] == 3;
  craft_project_end(&f);

  CuAssertTrue(tc, saved);
}

void Test_craft_slot_mote_refund_leaves_the_enhancement_motes(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int enhancement_motes, slot_motes, enhancement_element, slot_element;
  bool slot_refunded, enhancement_kept;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  GET_CRAFT(ch).enhancement = 1;
  enhancement_motes = craft_motes_required(0, 0, 0, 1);
  enhancement_element = get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, WEAPON_TYPE_LONG_SWORD);
  GET_CRAFT(ch).enhancement_motes_required = enhancement_motes;
  GET_CRAFT(ch).affected[0].location = APPLY_STR;
  GET_CRAFT(ch).affected[0].modifier = 1;
  GET_CRAFT(ch).affected[0].bonus_type = BONUS_TYPE_ENHANCEMENT;
  slot_motes = craft_motes_required(APPLY_STR, 1, BONUS_TYPE_ENHANCEMENT, 0);
  slot_element = crafting_mote_by_bonus_location(APPLY_STR, 0, BONUS_TYPE_ENHANCEMENT);
  GET_CRAFT(ch).motes_required[0] = slot_motes;

  newcraft_create(ch, "motes remove 1");
  slot_refunded =
      GET_CRAFT(ch).motes_required[0] == 0 && GET_CRAFT_MOTES(ch, slot_element) == slot_motes;
  enhancement_kept = GET_CRAFT(ch).enhancement_motes_required == enhancement_motes &&
                     GET_CRAFT_MOTES(ch, enhancement_element) == 0;
  craft_project_end(&f);

  CuAssertTrue(tc, slot_element != enhancement_element);
  CuAssertTrue(tc, slot_refunded);
  CuAssertTrue(tc, enhancement_kept);
}

void Test_craft_motes_refuse_a_bonus_slot_past_the_sixth(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int mote;
  bool remove_refused, add_refused, motes_kept = true, project_kept;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  /* Two funded bonus slots and an enhancement, the fields that follow the sixth slot. */
  GET_CRAFT(ch).motes_required[0] = 3;
  GET_CRAFT(ch).motes_required[1] = 3;
  GET_CRAFT(ch).enhancement = 8;
  for (mote = 0; mote < NUM_CRAFT_MOTES; mote++)
    GET_CRAFT_MOTES(ch, mote) = 100;

  newcraft_create(ch, "motes remove 7");
  remove_refused = craft_project_output_has(&f, "between 1 and 6");
  craft_project_reset_output(&f);
  newcraft_create(ch, "motes add 7");
  add_refused = craft_project_output_has(&f, "between 1 and 6");
  for (mote = 0; mote < NUM_CRAFT_MOTES; mote++)
    if (GET_CRAFT_MOTES(ch, mote) != 100)
      motes_kept = false;
  project_kept = GET_CRAFT(ch).enhancement == 8 && GET_CRAFT(ch).motes_required[0] == 3 &&
                 GET_CRAFT(ch).motes_required[1] == 3;
  craft_project_end(&f);

  CuAssertTrue(tc, remove_refused);
  CuAssertTrue(tc, add_refused);
  CuAssertTrue(tc, motes_kept);
  CuAssertTrue(tc, project_kept);
}

void Test_craft_start_and_check_report_skill_and_station_first(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct primary_activity_snapshot snapshot;
  bool refused, idle, station_reported;
  int objects;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 0);
  GET_CRAFT(ch).level_adjust = -10;

  newcraft_create(ch, "start");
  refused = craft_project_output_has(&f, "don't have the skill to craft");
  idle = GET_CRAFT(ch).craft_duration == 0 && !primary_activity_snapshot(ch, &snapshot);
  objects = craft_project_live_objects();

  f.room.contents = NULL;
  craft_project_reset_output(&f);
  newcraft_create(ch, "check");
  station_reported = craft_project_output_has(&f, "in a room with a forge") &&
                     craft_project_output_has(&f, "not yet ready");
  craft_project_end(&f);

  CuAssertTrue(tc, refused);
  CuAssertTrue(tc, idle);
  CuAssertIntEquals(tc, 0, objects);
  CuAssertTrue(tc, station_reported);
}

void Test_craft_bonuses_accept_the_first_variant(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool set;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);

  newcraft_create(ch, "bonuses 1 strength enhancement 1");
  set = GET_CRAFT(ch).affected[0].location == APPLY_STR && GET_CRAFT(ch).affected[0].modifier == 1;
  craft_project_end(&f);

  CuAssertTrue(tc, set);
}

void Test_craft_types_offer_only_what_recipes_can_build(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int golem_type, bow;
  bool woodworking, weaponsmithing_without_hammer;

  craft_project_begin(&f);
  newcraft_create(ch, "itemtype golem");
  golem_type = GET_CRAFT(ch).crafting_item_type;
  newcraft_create(ch, "itemtype weapon");
  newcraft_create(ch, "specifictype composite long bow");
  bow = GET_CRAFT(ch).crafting_specific;

  /* Carpentry has no tool slot, so it needs no worn tool. */
  GET_EQ(ch, WEAR_CRAFT_WEAPON_HAMMER) = NULL;
  woodworking = is_wearing_tool_for_crafting_ability(ch, ABILITY_CRAFT_WOODWORKING);
  weaponsmithing_without_hammer =
      is_wearing_tool_for_crafting_ability(ch, ABILITY_CRAFT_WEAPONSMITHING);
  craft_project_end(&f);

  CuAssertIntEquals(tc, CRAFT_TYPE_NONE, golem_type);
  CuAssertIntEquals(tc, WEAPON_TYPE_COMPOSITE_LONGBOW_5, bow);
  CuAssertTrue(tc, woodworking);
  CuAssertTrue(tc, !weaponsmithing_without_hammer);
}

void Test_craft_specific_types_offer_the_recipes_that_build_them(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  const char *text;
  int recipe, misfiled = 0, blowgun, hand_crossbow, hand_recipe, helms = 0;
  bool heavy_lists_hand;

  craft_project_begin(&f);
  if (armor_list[SPEC_ARMOR_TYPE_CHAINMAIL_HEAD].name == NULL)
    load_armor();
  /* A weapon or armor recipe is listed under the type it builds. */
  for (recipe = CRAFT_RECIPE_NONE + 1; recipe < NUM_CRAFTING_RECIPES; recipe++)
    if ((crafting_recipes[recipe].object_type == ITEM_WEAPON ||
         crafting_recipes[recipe].object_type == ITEM_ARMOR) &&
        crafting_recipes[recipe].practical_type != crafting_recipes[recipe].object_subtype)
      misfiled++;

  /* No recipe builds a blowgun. */
  newcraft_create(ch, "itemtype weapon");
  newcraft_create(ch, "specifictype blowgun");
  blowgun = GET_CRAFT(ch).crafting_specific;

  newcraft_create(ch, "reset");
  newcraft_create(ch, "itemtype weapon");
  newcraft_create(ch, "specifictype heavy crossbow");
  craft_project_reset_output(&f);
  newcraft_create(ch, "variant");
  heavy_lists_hand = craft_project_output_has(&f, "hand crossbow");

  newcraft_create(ch, "reset");
  newcraft_create(ch, "itemtype weapon");
  newcraft_create(ch, "specifictype hand crossbow");
  hand_crossbow = GET_CRAFT(ch).crafting_specific;
  newcraft_create(ch, "variant hand crossbow");
  hand_recipe = GET_CRAFT(ch).crafting_recipe;

  /* The second chainmail helm type has no recipe, so only the light chainmail helm and one
   * chainmail helm are listed. */
  newcraft_create(ch, "reset");
  newcraft_create(ch, "itemtype armor");
  craft_project_reset_output(&f);
  newcraft_create(ch, "specifictype");
  for (text = strstr(f.descriptor.output, "chainmail helm"); text != NULL;
       text = strstr(text + 1, "chainmail helm"))
    helms++;
  craft_project_end(&f);

  CuAssertIntEquals(tc, 0, misfiled);
  CuAssertIntEquals(tc, 0, blowgun);
  CuAssertTrue(tc, !heavy_lists_hand);
  CuAssertIntEquals(tc, WEAPON_TYPE_HAND_CROSSBOW, hand_crossbow);
  CuAssertIntEquals(tc, CRAFT_RECIPE_WEAPON_HAND_CROSSBOW, hand_recipe);
  CuAssertIntEquals(tc, 2, helms);
}

void Test_craft_busy_message_names_golem_work(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool named;

  craft_project_begin(&f);
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_GOLEM;
  GET_CRAFT(ch).craft_duration = 5;
  do_newcraft(ch, "show", 0, SCMD_NEWCRAFT_CREATE);
  named = craft_project_output_has(&f, "project of type: golem.");
  GET_CRAFT(ch).crafting_method = 0;
  GET_CRAFT(ch).craft_duration = 0;
  craft_project_end(&f);

  CuAssertTrue(tc, named);
}

void Test_craft_resize_reset_clears_the_new_size(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool cleared;

  craft_project_begin(&f);
  GET_CRAFT(ch).new_size = SIZE_LARGE;
  GET_CRAFT(ch).resize_mat_type = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).resize_mat_num = 2;
  reset_current_craft(ch, CuMutableString("resize"), FALSE, TRUE);
  cleared = GET_CRAFT(ch).new_size == 0 && GET_CRAFT(ch).resize_mat_type == 0 &&
            GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) == 2;
  craft_project_end(&f);

  CuAssertTrue(tc, cleared);
}

void Test_supply_order_commands_leave_a_craft_project_alone(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool abandon_refused, remove_refused, select_refused, request_refused, intact;

  craft_project_begin(&f);
  craft_project_add_quartermaster(&f);
  craft_project_ready_long_sword(ch);

  newcraft_supplyorder(ch, "abandon");
  abandon_refused = craft_project_output_has(&f, "don't have a supply order to abandon");
  craft_project_reset_output(&f);
  newcraft_supplyorder(ch, "material remove");
  remove_refused = craft_project_output_has(&f, "don't have a supply order");
  craft_project_reset_output(&f);
  newcraft_supplyorder(ch, "select 1");
  select_refused = craft_project_output_has(&f, "craft reset");
  craft_project_reset_output(&f);
  newcraft_supplyorder(ch, "request");
  request_refused = craft_project_output_has(&f, "craft reset");
  intact = GET_CRAFT(ch).crafting_item_type == CRAFT_TYPE_WEAPON &&
           GET_CRAFT(ch).crafting_method == 0 &&
           GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 6 &&
           GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) == 0;
  craft_project_end(&f);

  CuAssertTrue(tc, abandon_refused);
  CuAssertTrue(tc, remove_refused);
  CuAssertTrue(tc, select_refused);
  CuAssertTrue(tc, request_refused);
  CuAssertTrue(tc, intact);
}

void Test_craft_commands_leave_a_supply_order_alone(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct primary_activity_snapshot snapshot;
  enum domain_event_status runtime;
  int mote;
  bool show_refused, check_refused, reset_refused, score_shown, golem_set, golem_refused, held;

  craft_project_begin(&f);
  event_free_all();
  event_init();
  runtime = domain_event_runtime_init();
  craft_project_hold_supply_order(ch);
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] = 6;

  newcraft_create(ch, "show");
  show_refused = craft_project_output_has(&f, "working on a supply order") &&
                 !craft_project_output_has(&f, "Current Craft Project");
  craft_project_reset_output(&f);
  newcraft_create(ch, "check");
  check_refused = craft_project_output_has(&f, "working on a supply order") &&
                  !craft_project_output_has(&f, "ready to begin");
  craft_project_reset_output(&f);
  newcraft_create(ch, "reset");
  reset_refused = craft_project_output_has(&f, "working on a supply order");

  /* Skill scores and the golem project do not use the project record. */
  craft_project_reset_output(&f);
  newcraft_create(ch, "score");
  score_shown = craft_project_output_has(&f, "SKILL") &&
                !craft_project_output_has(&f, "working on a supply order");
  newcraft_create(ch, "golem type wood");
  golem_set = GET_CRAFT(ch).golem_type == GOLEM_TYPE_WOOD;

  /* Starting golem work would record its method over the order's. */
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 1);
  GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) = 100;
  GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE) = 100;
  for (mote = 1; mote < NUM_CRAFT_MOTES; mote++)
    GET_CRAFT_MOTES(ch, mote) = 100;
  craft_project_reset_output(&f);
  newcraft_create(ch, "golem start");
  golem_refused = craft_project_output_has(&f, "working on a supply order") &&
                  !primary_activity_snapshot(ch, &snapshot);
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_COMMAND, false);
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 0);
  held = player_has_supply_order(ch) && GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 6;

  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, show_refused);
  CuAssertTrue(tc, check_refused);
  CuAssertTrue(tc, reset_refused);
  CuAssertTrue(tc, score_shown);
  CuAssertTrue(tc, golem_set);
  CuAssertTrue(tc, golem_refused);
  CuAssertTrue(tc, held);
}

void Test_supply_order_abandon_refunds_materials_and_clears_progress(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool refunded, cleared;

  craft_project_begin(&f);
  craft_project_hold_supply_order(ch);
  GET_NSUPPLY_NUM_MADE(ch) = 2;
  GET_CRAFT(ch).supply_contract_type = SUPPLY_CONTRACT_QUALITY;
  GET_CRAFT(ch).supply_quality_tier_requirement = QUALITY_TIER_SUPERIOR;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] = 6;

  newcraft_supplyorder(ch, "abandon");
  refunded = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) == 6 &&
             GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 0;
  cleared = !player_has_supply_order(ch) && GET_NSUPPLY_NUM_MADE(ch) == 0 &&
            GET_CRAFT(ch).supply_contract_type == 0 &&
            GET_CRAFT(ch).supply_quality_tier_requirement == QUALITY_TIER_STANDARD &&
            craft_project_output_has(&f, "abandoned your supply order");
  craft_project_end(&f);

  CuAssertTrue(tc, refunded);
  CuAssertTrue(tc, cleared);
}

void Test_supply_order_requests_start_fresh_with_orderable_recipes(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int i, requested = 0, fresh = 0, orderable = 0;

  craft_project_begin(&f);
  craft_project_add_quartermaster(&f);
  for (i = 0; i < 40; i++)
  {
    GET_NSUPPLY_NUM_MADE(ch) = 5;
    newcraft_supplyorder(ch, "request");
    if (player_has_supply_order(ch))
      requested++;
    if (GET_NSUPPLY_NUM_MADE(ch) == 0)
      fresh++;
    if (GET_CRAFT(ch).craft_variant >= 0 &&
        crafting_recipes[GET_CRAFT(ch).crafting_recipe].object_type != ITEM_INSTRUMENT)
      orderable++;
    reset_supply_order(ch);
    craft_project_reset_output(&f);
  }
  craft_project_end(&f);

  CuAssertIntEquals(tc, 40, requested);
  CuAssertIntEquals(tc, 40, fresh);
  CuAssertIntEquals(tc, 40, orderable);
}

void Test_supply_order_completion_needs_the_exact_quantity(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool refused, unpaid;

  craft_project_begin(&f);
  craft_project_add_quartermaster(&f);
  craft_project_hold_supply_order(ch);
  GET_NSUPPLY_NUM_MADE(ch) = 5;

  newcraft_supplyorder(ch, "complete");
  refused = craft_project_output_has(&f, "does not match") && player_has_supply_order(ch);
  unpaid = GET_GOLD(ch) == 0 && GET_ARTISAN_EXP(ch) == 0;
  craft_project_end(&f);

  CuAssertTrue(tc, refused);
  CuAssertTrue(tc, unpaid);
}

void Test_supply_order_consumes_every_group_or_none(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool consumed, untouched;

  craft_project_begin(&f);
  craft_project_hold_supply_order(ch);
  /* Enough hard metal, but no hide. */
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] = 6;

  consumed = consume_supply_order_materials(ch);
  untouched = GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] == CRAFT_MAT_STEEL &&
              GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 6;
  craft_project_end(&f);

  CuAssertTrue(tc, !consumed);
  CuAssertTrue(tc, untouched);
}

void Test_supply_order_quality_contract_needs_higher_grade_materials(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool tin_refused, iron_added;

  craft_project_begin(&f);
  craft_project_hold_supply_order(ch);
  GET_CRAFT(ch).supply_contract_type = SUPPLY_CONTRACT_QUALITY;
  GET_CRAFT(ch).supply_quality_tier_requirement = QUALITY_TIER_SUPERIOR;
  GET_CRAFT_MAT(ch, CRAFT_MAT_TIN) = 6;
  GET_CRAFT_MAT(ch, CRAFT_MAT_IRON) = 6;

  newcraft_supplyorder(ch, "material add tin");
  tin_refused = craft_project_output_has(&f, "higher grade") &&
                GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 0 &&
                GET_CRAFT_MAT(ch, CRAFT_MAT_TIN) == 6;
  newcraft_supplyorder(ch, "material add iron");
  iron_added = GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] == CRAFT_MAT_IRON &&
               GET_CRAFT_MAT(ch, CRAFT_MAT_IRON) == 0;
  craft_project_end(&f);

  CuAssertTrue(tc, tin_refused);
  CuAssertTrue(tc, iron_added);
}

void Test_supply_order_offers_always_have_a_variant(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int id, slot, offers = 0, orderable = 0;
  bool stale_refused, stale_replaced;

  craft_project_begin(&f);
  for (id = 1; id <= 40; id++)
  {
    GET_IDNUM(ch) = id;
    memset(GET_CRAFT(ch).supply_slot_active, 0, sizeof(GET_CRAFT(ch).supply_slot_active));
    memset(GET_CRAFT(ch).supply_slot_cooldowns, 0, sizeof(GET_CRAFT(ch).supply_slot_cooldowns));
    refresh_supply_slots(ch);
    for (slot = 0; slot < 5; slot++)
    {
      struct supply_contract *offer = &GET_CRAFT(ch).supply_slots[slot];

      if (!GET_CRAFT(ch).supply_slot_active[slot])
        continue;
      offers++;
      if (offer->variant >= 0 && crafting_recipes[offer->recipe].object_type != ITEM_INSTRUMENT)
        orderable++;
      free(offer->description);
      free(offer->requirements);
      offer->description = offer->requirements = NULL;
    }
  }

  /* An offer saved by an older version can still name an instrument without a variant. */
  memset(GET_CRAFT(ch).supply_slot_active, 0, sizeof(GET_CRAFT(ch).supply_slot_active));
  GET_CRAFT(ch).supply_slots_last_refresh = time(NULL);
  GET_CRAFT(ch).supply_slot_active[0] = TRUE;
  memset(&GET_CRAFT(ch).supply_slots[0], 0, sizeof(GET_CRAFT(ch).supply_slots[0]));
  GET_CRAFT(ch).supply_slots[0].recipe = CRAFT_RECIPE_INSTRUMENT_LYRE;
  GET_CRAFT(ch).supply_slots[0].variant = -1;
  GET_CRAFT(ch).supply_slots[0].quantity = 3;
  craft_project_add_quartermaster(&f);
  newcraft_supplyorder(ch, "select 1");
  stale_refused = craft_project_output_has(&f, "not available") && !player_has_supply_order(ch);

  /* The next refresh replaces that offer, so the same number selects a new one. */
  GET_CRAFT(ch).supply_slots_last_refresh = time(NULL) - 3600;
  newcraft_supplyorder(ch, "select 1");
  stale_replaced = player_has_supply_order(ch) && GET_CRAFT(ch).craft_variant >= 0 &&
                   GET_CRAFT(ch).crafting_recipe != CRAFT_RECIPE_INSTRUMENT_LYRE;
  reset_supply_order(ch);
  cleanup_supply_slots(ch);
  craft_project_end(&f);

  CuAssertIntEquals(tc, 200, offers);
  CuAssertIntEquals(tc, offers, orderable);
  CuAssertTrue(tc, stale_refused);
  CuAssertTrue(tc, stale_replaced);
}

void Test_supply_order_mobile_runs_every_supplyorder_subcommand(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  bool created_commands = false, handled = false, materials_reached = false, cancel_reached = false,
       info_reached = false;
  int supplyorder_cmd;

  craft_project_begin(&f);
  craft_project_add_quartermaster(&f);
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_commands = true;
  }
  supplyorder_cmd = find_command("supplyorder");
  if (supplyorder_cmd >= 0)
  {
    handled = new_supply_orders(ch, &f.quartermaster, supplyorder_cmd, "materials") != 0;
    materials_reached = craft_project_output_has(&f, "don't have a supply order.");
    craft_project_reset_output(&f);
    (void)new_supply_orders(ch, &f.quartermaster, supplyorder_cmd, "cancel");
    cancel_reached = craft_project_output_has(&f, "don't have a supply order to abandon");
    craft_project_reset_output(&f);
    (void)new_supply_orders(ch, &f.quartermaster, supplyorder_cmd, "info");
    info_reached = craft_project_output_has(&f, "don't have a supply order in progress");
  }
  if (created_commands)
    free_command_list();
  craft_project_end(&f);

  CuAssertTrue(tc, supplyorder_cmd >= 0);
  CuAssertTrue(tc, handled);
  CuAssertTrue(tc, materials_reached);
  CuAssertTrue(tc, cancel_reached);
  CuAssertTrue(tc, info_reached);
}

void Test_golem_construction_keeps_the_chosen_wood(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  enum domain_event_status runtime;
  int mote;
  bool started, kept;

  craft_project_begin(&f);
  event_free_all();
  event_init();
  runtime = domain_event_runtime_init();
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 1);
  /* Maple is the only wood on hand; ash is the requirement table's placeholder. */
  GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) = 100;
  GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE) = 100;
  for (mote = 1; mote < NUM_CRAFT_MOTES; mote++)
    GET_CRAFT_MOTES(ch, mote) = 100;
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_WOOD;
  GET_CRAFT(ch).golem_size = GOLEM_SIZE_SMALL;

  started = begin_golem_craft(ch);
  kept = GET_CRAFT(ch).golem_materials[0][0] == CRAFT_MAT_MAPLE_WOOD;
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_COMMAND, false);
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 0);

  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, started);
  CuAssertTrue(tc, kept);
}

void Test_golem_work_that_ends_leaves_supply_orders_available(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct index_data mob_index_entry;
  struct index_data *saved_mob_index = mob_index;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  enum domain_event_status runtime;
  int mote;
  bool started, cancel_cleared, requested_after_cancel, finish_cleared, requested_after_finish;

  craft_project_begin(&f);
  craft_project_add_quartermaster(&f);
  event_free_all();
  event_init();
  runtime = domain_event_runtime_init();
  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 1);
  GET_CRAFT_MAT(ch, CRAFT_MAT_MAPLE_WOOD) = 100;
  GET_CRAFT_MAT(ch, CRAFT_MAT_BRONZE) = 100;
  for (mote = 1; mote < NUM_CRAFT_MOTES; mote++)
    GET_CRAFT_MOTES(ch, mote) = 100;
  GET_CRAFT(ch).golem_type = GOLEM_TYPE_WOOD;
  GET_CRAFT(ch).golem_size = GOLEM_SIZE_SMALL;

  /* Cancelled work keeps the golem project for another start. */
  started = begin_golem_craft(ch);
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_COMMAND, false);
  cancel_cleared =
      GET_CRAFT(ch).crafting_method == 0 && GET_CRAFT(ch).golem_type == GOLEM_TYPE_WOOD;
  newcraft_supplyorder(ch, "request");
  requested_after_cancel = player_has_supply_order(ch);
  reset_supply_order(ch);

  /* Finished work: with no golem prototype loaded, completion ends the project unbuilt. */
  memset(&mob_index_entry, 0, sizeof(mob_index_entry));
  mob_index_entry.vnum = 1;
  mob_index = &mob_index_entry;
  top_of_mobt = 0;
  GET_CRAFT(ch).crafting_method = SCMD_NEWCRAFT_GOLEM;
  craft_golem_complete(ch);
  finish_cleared = GET_CRAFT(ch).crafting_method == 0;
  craft_project_reset_output(&f);
  newcraft_supplyorder(ch, "request");
  requested_after_finish = player_has_supply_order(ch);
  reset_supply_order(ch);
  mob_index = saved_mob_index;
  top_of_mobt = saved_top_of_mobt;

  SET_FEAT(ch, FEAT_CONSTRUCT_WOOD_GOLEM, 0);
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, started);
  CuAssertTrue(tc, cancel_cleared);
  CuAssertTrue(tc, requested_after_cancel);
  CuAssertTrue(tc, finish_cleared);
  CuAssertTrue(tc, requested_after_finish);
}

void Test_craft_material_bundles_keep_their_hide_grade(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int bundle_value, high_after_unstore, high_after_store, low_after_store;

  craft_project_begin(&f);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  GET_CRAFT_MAT(ch, CRAFT_MAT_HIGH_GRADE_HIDE) = 8;

  do_list_craft_materials(ch, "unstore 5 high grade hide", 0, 0);
  bundle_value = ch->carrying != NULL ? GET_OBJ_VAL(ch->carrying, 1) : -1;
  high_after_unstore = GET_CRAFT_MAT(ch, CRAFT_MAT_HIGH_GRADE_HIDE);
  do_list_craft_materials(ch, "store bundle", 0, 0);
  high_after_store = GET_CRAFT_MAT(ch, CRAFT_MAT_HIGH_GRADE_HIDE);
  low_after_store = GET_CRAFT_MAT(ch, CRAFT_MAT_LOW_GRADE_HIDE);
  craft_project_end(&f);

  CuAssertIntEquals(tc, CRAFT_MAT_HIGH_GRADE_HIDE, bundle_value);
  CuAssertIntEquals(tc, 3, high_after_unstore);
  CuAssertIntEquals(tc, 8, high_after_store);
  CuAssertIntEquals(tc, 0, low_after_store);
}

void Test_reforge_matches_abbreviations_and_numbered_bows(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *weapon;
  enum domain_event_status runtime;
  int abbreviated = -1, numbered = -1;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  weapon = craft_project_reforgeable_dagger();
  if (weapon != NULL)
  {
    obj_to_char(weapon, ch);
    /* The reforge resolves when its activity completes, not at the command. */
    do_reforge_new(ch, "weapon khop", 0, 0);
    abbreviated = GET_OBJ_VAL(weapon, 0) == WEAPON_TYPE_DAGGER ? -2 : -3;
    craft_project_advance_seconds(8);
    abbreviated = abbreviated == -2 ? GET_OBJ_VAL(weapon, 0) : abbreviated;
    do_reforge_new(ch, "khopesh composite long bow (2)", 0, 0);
    craft_project_advance_seconds(8);
    numbered = GET_OBJ_VAL(weapon, 0);
  }
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertIntEquals(tc, WEAPON_TYPE_KHOPESH, abbreviated);
  CuAssertIntEquals(tc, WEAPON_TYPE_COMPOSITE_LONGBOW_2, numbered);
}

void Test_crafting_kit_reforge_needs_exactly_one_item(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *kit, *first, *second;
  enum domain_event_status runtime;
  char argument[MAX_INPUT_LENGTH];
  bool created_commands = false, refused = false;
  int reforge_cmd, reforged = -1;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  if (complete_cmd_info == NULL)
  {
    create_command_list();
    created_commands = true;
  }
  reforge_cmd = find_command("reforge");
  kit = read_object(WEAPON_PROTO, VIRTUAL);
  first = craft_project_reforgeable_dagger();
  second = craft_project_reforgeable_dagger();
  if (kit != NULL && first != NULL && second != NULL)
  {
    GET_OBJ_TYPE(kit) = ITEM_CONTAINER;
    obj_to_char(kit, ch);
    obj_to_obj(first, kit);
    obj_to_obj(second, kit);
    snprintf(argument, sizeof(argument), "khop");
    crafting_kit(ch, kit, reforge_cmd, argument);
    refused = craft_project_output_has(&f, "Only one item should be inside the kit") &&
              GET_OBJ_VAL(first, 0) == WEAPON_TYPE_DAGGER &&
              GET_OBJ_VAL(second, 0) == WEAPON_TYPE_DAGGER;

    /* With one item left, the abbreviation selects the khopesh. */
    obj_from_obj(second);
    extract_obj(second);
    snprintf(argument, sizeof(argument), "khop");
    crafting_kit(ch, kit, reforge_cmd, argument);
    reforged = GET_OBJ_VAL(first, 0) == WEAPON_TYPE_DAGGER ? -2 : -3;
    craft_project_advance_seconds(8);
    reforged = reforged == -2 ? GET_OBJ_VAL(first, 0) : reforged;
  }
  if (created_commands)
    free_command_list();
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, reforge_cmd >= 0);
  CuAssertTrue(tc, refused);
  CuAssertIntEquals(tc, WEAPON_TYPE_KHOPESH, reforged);
}

/* ---- Material identity and checked balance credits (crafting consolidation, Phase 1) ---- */

#include "../../src/craft/brew.h"
#include "../../src/craft/crafts.h"
#include "../../src/obj/objsave.h"

#include <limits.h>

/** A test prototype: its vnum, object material, the balance it stores as (CRAFT_MAT_NONE:
 * refused or object-only), and a keyword. */
struct craft_test_prototype
{
  obj_vnum vnum;
  int object_material;
  int balance;
  const char *keyword;
};

/** Legacy node and shop prototypes and the balance each stores as (CRAFT_MAT_NONE: refused). */
static const struct craft_test_prototype craft_legacy_prototypes[] = {
    {WOOD_MATERIAL, MATERIAL_WOOD, CRAFT_MAT_ASH_WOOD, "planks"},
    {ALDERWOOD_MATERIAL, MATERIAL_WOOD, CRAFT_MAT_ASH_WOOD, "alderwood"},
    {YEW_MATERIAL, MATERIAL_WOOD, CRAFT_MAT_MAPLE_WOOD, "yew"},
    {OAK_MATERIAL, MATERIAL_WOOD, CRAFT_MAT_MAHAGONY_WOOD, "oak"},
    {DARKWOOD_MATERIAL, MATERIAL_DARKWOOD, CRAFT_MAT_IRONWOOD, "darkwood"},
    {LEATHER_MQ_MATERIAL, MATERIAL_LEATHER, CRAFT_MAT_MEDIUM_GRADE_HIDE, "studded"},
    {LEATHER_HQ_MATERIAL, MATERIAL_LEATHER, CRAFT_MAT_HIGH_GRADE_HIDE, "cured"},
    {VELVET_MATERIAL, MATERIAL_VELVET, CRAFT_MAT_COTTON, "velvet"},
    {BURLAP_MATERIAL, MATERIAL_BURLAP, CRAFT_MAT_HEMP, "burlap"},
    {FOS_BIRD_MATERIAL, MATERIAL_STONE, CRAFT_MAT_NONE, "birdegg"},
    {FOS_WYVERN_MATERIAL, MATERIAL_STONE, CRAFT_MAT_NONE, "wyvernegg"},
    {FOS_DRAGON_MATERIAL, MATERIAL_STONE, CRAFT_MAT_NONE, "dragonegg"},
    {FOS_LIZARD_MATERIAL, MATERIAL_STONE, CRAFT_MAT_NONE, "lizardegg"},
};
#define NUM_CRAFT_LEGACY_PROTOTYPES                                                                \
  ((int)(sizeof(craft_legacy_prototypes) / sizeof(craft_legacy_prototypes[0])))
#define MAX_CRAFT_TEST_PROTOTYPES 24

/** A larger, sorted object index holding the fixture's two prototypes plus the given material
 * prototypes, so read_object(), real_object(), and GET_OBJ_VNUM() see the real vnums. */
static struct index_data craft_legacy_index[MAX_CRAFT_TEST_PROTOTYPES + 2];
static struct obj_data craft_legacy_proto[MAX_CRAFT_TEST_PROTOTYPES + 2];

static void craft_project_use_prototypes(struct craft_project_fixture *f,
                                         const struct craft_test_prototype *rows, int rows_count)
{
  int i, j, count = 0;
  struct index_data index_swap;
  struct obj_data proto_swap;

  memset(craft_legacy_index, 0, sizeof(craft_legacy_index));
  memset(craft_legacy_proto, 0, sizeof(craft_legacy_proto));
  for (i = 0; i < 2; i++)
  {
    craft_legacy_index[count] = f->object_index[i];
    craft_legacy_proto[count] = f->object_proto[i];
    count++;
  }
  for (i = 0; i < rows_count && i < MAX_CRAFT_TEST_PROTOTYPES; i++)
  {
    clear_object(&craft_legacy_proto[count]);
    GET_OBJ_TYPE(&craft_legacy_proto[count]) = ITEM_MATERIAL;
    GET_OBJ_MATERIAL(&craft_legacy_proto[count]) = rows[i].object_material;
    GET_OBJ_VAL(&craft_legacy_proto[count], 0) = 2;
    SET_BIT_AR(GET_OBJ_WEAR(&craft_legacy_proto[count]), ITEM_WEAR_TAKE);
    craft_legacy_proto[count].name = CuMutableString(rows[i].keyword);
    craft_legacy_proto[count].short_description = CuMutableString("a legacy material");
    craft_legacy_proto[count].description = CuMutableString("A legacy material lies here.");
    craft_legacy_index[count].vnum = rows[i].vnum;
    count++;
  }
  /* Insertion sort by vnum, keeping index and prototype rows together. */
  for (i = 1; i < count; i++)
  {
    for (j = i; j > 0 && craft_legacy_index[j - 1].vnum > craft_legacy_index[j].vnum; j--)
    {
      index_swap = craft_legacy_index[j];
      craft_legacy_index[j] = craft_legacy_index[j - 1];
      craft_legacy_index[j - 1] = index_swap;
      proto_swap = craft_legacy_proto[j];
      craft_legacy_proto[j] = craft_legacy_proto[j - 1];
      craft_legacy_proto[j - 1] = proto_swap;
    }
  }
  for (i = 0; i < count; i++)
    craft_legacy_proto[i].item_number = i;
  obj_index = craft_legacy_index;
  obj_proto = craft_legacy_proto;
  top_of_objt = count - 1;
}

static void craft_project_use_legacy_prototypes(struct craft_project_fixture *f)
{
  craft_project_use_prototypes(f, craft_legacy_prototypes, NUM_CRAFT_LEGACY_PROTOTYPES);
}

void Test_craft_prototype_table_names_every_legacy_material(CuTest *tc)
{
  int i, material;

  for (i = 0; i < NUM_CRAFT_LEGACY_PROTOTYPES; i++)
  {
    material = -1;
    CuAssertTrue(tc, craft_material_for_prototype(craft_legacy_prototypes[i].vnum, &material));
    CuAssertIntEquals(tc, craft_legacy_prototypes[i].balance, material);
  }
  CuAssertTrue(tc, !craft_material_for_prototype(ONYX_MATERIAL, &material));
  CuAssertTrue(tc, !craft_material_for_prototype(NOTHING, &material));
  CuAssertIntEquals(tc, 4, material_grade(CRAFT_MAT_COTTON));
  CuAssertIntEquals(tc, 4, material_grade(CRAFT_MAT_SILK));
  CuAssertIntEquals(tc, 5, material_grade(CRAFT_MAT_SATIN));
  CuAssertIntEquals(tc, CRAFT_MAT_ASH_WOOD, obj_material_to_craft_material(MATERIAL_WOOD));
  CuAssertIntEquals(tc, CRAFT_MAT_HEMP, obj_material_to_craft_material(MATERIAL_BURLAP));
}

/** Every Decision 2 row stores into its named balance, and a fossil egg is refused without
 * extraction (it must not store as stone). */
void Test_craft_store_credits_legacy_prototypes_and_refuses_eggs(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *obj;
  char command[64];
  int i, before, stored[NUM_CRAFT_LEGACY_PROTOTYPES], kept[NUM_CRAFT_LEGACY_PROTOTYPES];
  int stone_after;

  craft_project_begin(&f);
  craft_project_use_legacy_prototypes(&f);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  for (i = 0; i < NUM_CRAFT_LEGACY_PROTOTYPES; i++)
  {
    obj = read_object(craft_legacy_prototypes[i].vnum, VIRTUAL);
    CuAssertPtrNotNull(tc, obj);
    obj_to_char(obj, ch);
    before = craft_legacy_prototypes[i].balance != CRAFT_MAT_NONE
                 ? GET_CRAFT_MAT(ch, craft_legacy_prototypes[i].balance)
                 : 0;
    snprintf(command, sizeof(command), "store %s", craft_legacy_prototypes[i].keyword);
    do_list_craft_materials(ch, command, 0, 0);
    stored[i] = craft_legacy_prototypes[i].balance != CRAFT_MAT_NONE
                    ? GET_CRAFT_MAT(ch, craft_legacy_prototypes[i].balance) - before
                    : 0;
    kept[i] = ch->carrying == obj;
    if (kept[i])
      extract_obj(obj);
  }
  stone_after = GET_CRAFT_MAT(ch, CRAFT_MAT_STONE);
  craft_project_end(&f);

  for (i = 0; i < NUM_CRAFT_LEGACY_PROTOTYPES; i++)
  {
    if (craft_legacy_prototypes[i].balance != CRAFT_MAT_NONE)
    {
      CuAssertIntEquals(tc, 2, stored[i]);
      CuAssertIntEquals(tc, 0, kept[i]);
    }
    else
    {
      CuAssertIntEquals(tc, 1, kept[i]);
    }
  }
  CuAssertIntEquals(tc, 0, stone_after);
}

/** A generic high-hide bundle keeps its grade, and an unstored bundle written to and read
 * back from an object file still stores as the same material and quantity. */
void Test_craft_unstored_bundle_keeps_its_identity_through_an_object_file(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *bundle, *loaded = NULL;
  obj_save_data *records, *record;
  FILE *file;
  int loaded_value = -1, loaded_quantity = -1, high_after_store = -1, low_after_store = -1;
  int wrote = 0;

  craft_project_begin(&f);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  GET_CRAFT_MAT(ch, CRAFT_MAT_HIGH_GRADE_HIDE) = 8;
  do_list_craft_materials(ch, "unstore 5 high grade hide", 0, 0);
  bundle = ch->carrying;
  CuAssertPtrNotNull(tc, bundle);

  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  wrote = objsave_save_obj_record(bundle, ch, file, 0);
  extract_obj(bundle);
  rewind(file);
  records = objsave_parse_objects(file);
  fclose(file);
  for (record = records; record != NULL; record = record->next)
  {
    if (record->obj != NULL && loaded == NULL)
      loaded = record->obj;
    else if (record->obj != NULL)
      extract_obj(record->obj);
  }
  while (records != NULL)
  {
    record = records->next;
    free(records);
    records = record;
  }
  if (loaded != NULL)
  {
    loaded_value = GET_OBJ_VAL(loaded, 1);
    loaded_quantity = GET_OBJ_VAL(loaded, 0);
    obj_to_char(loaded, ch);
    do_list_craft_materials(ch, "store bundle", 0, 0);
    high_after_store = GET_CRAFT_MAT(ch, CRAFT_MAT_HIGH_GRADE_HIDE);
    low_after_store = GET_CRAFT_MAT(ch, CRAFT_MAT_LOW_GRADE_HIDE);
  }
  craft_project_end(&f);

  CuAssertIntEquals(tc, 1, wrote);
  CuAssertPtrNotNull(tc, loaded);
  CuAssertIntEquals(tc, CRAFT_MAT_HIGH_GRADE_HIDE, loaded_value);
  CuAssertIntEquals(tc, 5, loaded_quantity);
  CuAssertIntEquals(tc, 8, high_after_store);
  CuAssertIntEquals(tc, 0, low_after_store);
}

void Test_craft_store_refuses_overflow_and_keeps_the_bundle(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *bundle;
  int balance_after, kept, refused;

  craft_project_begin(&f);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = 5;
  do_list_craft_materials(ch, "unstore 5 steel", 0, 0);
  bundle = ch->carrying;
  CuAssertPtrNotNull(tc, bundle);
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = INT_MAX - 1;
  craft_project_reset_output(&f);
  do_list_craft_materials(ch, "store bundle", 0, 0);
  balance_after = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL);
  kept = ch->carrying == bundle;
  refused = craft_project_output_has(&f, "storage space");
  craft_project_end(&f);

  CuAssertIntEquals(tc, INT_MAX - 1, balance_after);
  CuAssertIntEquals(tc, 1, kept);
  CuAssertIntEquals(tc, 1, refused);
  CuAssertTrue(tc, !craft_balance_can_add(NULL, CRAFT_MAT_STEEL, 1));
  CuAssertTrue(tc, !craft_mote_can_add(NULL, CRAFTING_MOTE_AIR, 1));
}

/** A project refund the balance cannot accept leaves the allocation on the project. */
void Test_craft_capped_refund_keeps_its_allocation(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int steel_after, hide_after, steel_allocated, hide_allocated, enhancement_after, motes_after;

  craft_project_begin(&f);
  craft_project_ready_long_sword(ch);
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = INT_MAX - 2;
  GET_CRAFT_MAT(ch, CRAFT_MAT_LOW_GRADE_HIDE) = 0;
  GET_CRAFT(ch).enhancement = 1;
  GET_CRAFT(ch).enhancement_motes_required = 4;
  GET_CRAFT_MOTES(ch, get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, WEAPON_TYPE_LONG_SWORD)) =
      INT_MAX;
  reset_craft_materials(ch, FALSE, TRUE);
  reset_current_craft(ch, NULL, FALSE, TRUE);
  steel_after = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL);
  hide_after = GET_CRAFT_MAT(ch, CRAFT_MAT_LOW_GRADE_HIDE);
  steel_allocated = GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1];
  hide_allocated = GET_CRAFT(ch).materials[CRAFT_GROUP_HIDES][1];
  enhancement_after = GET_CRAFT(ch).enhancement_motes_required;
  motes_after =
      GET_CRAFT_MOTES(ch, get_enhancement_mote_type(ch, CRAFT_TYPE_WEAPON, WEAPON_TYPE_LONG_SWORD));
  craft_project_end(&f);

  CuAssertIntEquals(tc, INT_MAX - 2, steel_after);
  CuAssertIntEquals(tc, 6, steel_allocated);
  CuAssertIntEquals(tc, 1, hide_after);
  CuAssertIntEquals(tc, 0, hide_allocated);
  CuAssertIntEquals(tc, 4, enhancement_after);
  CuAssertIntEquals(tc, INT_MAX, motes_after);
}

/** Salvage preflights its material result: a full balance refuses the whole salvage, leaving
 * the item, the gold, and the balance untouched. The material roll is random, so repeat until a
 * material result is rolled; a miss must still salvage normally. */
void Test_salvage_refuses_a_full_balance_before_extracting(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *obj;
  int attempt, refusals = 0, successes = 0, bad_refusal = 0, bad_success = 0, gold_before;

  craft_project_begin(&f);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  SET_FEAT(ch, FEAT_SALVAGE, 1);
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = INT_MAX;
  for (attempt = 0; attempt < 400 && refusals == 0; attempt++)
  {
    obj = craft_project_reforgeable_dagger();
    CuAssertPtrNotNull(tc, obj);
    GET_OBJ_COST(obj) = 100;
    GET_OBJ_LEVEL(obj) = 30;
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
    obj_to_char(obj, ch);
    gold_before = GET_GOLD(ch);
    craft_project_reset_output(&f);
    do_salvage(ch, "weapon", 0, 0);
    if (ch->carrying == obj)
    {
      refusals++;
      if (GET_GOLD(ch) != gold_before || !craft_project_output_has(&f, "cannot hold"))
        bad_refusal++;
      extract_obj(obj);
    }
    else
    {
      successes++;
      if (GET_GOLD(ch) != gold_before + 15)
        bad_success++;
    }
    if (GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) != INT_MAX)
      bad_success++;
  }
  craft_project_end(&f);

  CuAssertIntEquals(tc, 1, refusals);
  CuAssertIntEquals(tc, 0, bad_refusal);
  CuAssertIntEquals(tc, 0, bad_success);
  CuAssertTrue(tc, successes >= 0);
}

/* ---- Catalog on abilities and balances (crafting consolidation, Phase 2) ---- */

/** Old and new catalog records load onto abilities, round-trip through the writer in the new
 * form without converting twice, and an unmapped legacy skill keeps its raw record but can never
 * execute. */
void Test_catalog_records_convert_once_and_round_trip(CuTest *tc)
{
  static const char *legacy_file = "NEW\nName: Sword of Aegon\nId  : 11371\nFlag: 0\n"
                                   "Vnum: 3299\nTime: 60\nSkil: 477 90\nMslf: You craft $p.\n"
                                   "Mroo: $n crafts $p.\nReq : 3193 10 0\nEnd :\n"
                                   "NEW\nName: Iron Cap\nId  : 11372\nFlag: 0\nVnum: 3299\n"
                                   "Time: 10\nSkil: 2076 70\nMslf: You craft $p.\n"
                                   "Mroo: $n crafts $p.\nEnd :\n"
                                   "NEW\nName: Quick Stitch\nId  : 11373\nFlag: 0\nVnum: 3299\n"
                                   "Time: 10\nSkil: 480 50\nMslf: You craft $p.\n"
                                   "Mroo: $n crafts $p.\nEnd :\n"
                                   "NEW\nName: Plain Hat\nId  : 11374\nFlag: 0\nVnum: 3299\n"
                                   "Time: 10\nSkil: -1 0\nMslf: You craft $p.\n"
                                   "Mroo: $n crafts $p.\nEnd :\n"
                                   "NEW\nName: Silver Ring\nId  : 11375\nFlag: 0\nVnum: 3299\n"
                                   "Time: 10\nAbil: 40 5\nMslf: You craft $p.\n"
                                   "Mroo: $n crafts $p.\nEnd :\n$\n";
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct craft_data *sword, *cap, *stitch, *hat, *ring;
  FILE *file;
  char saved[4096];
  size_t saved_length;
  int sword_skill = -9, sword_rank = -9, cap_skill = -9, cap_rank = -9, stitch_skill = -9;
  int stitch_legacy = -9, stitch_rank = -9, hat_skill = -9, ring_skill = -9, ring_rank = -9;
  int reloaded_sword_rank = -9, reloaded_stitch_skill = -9, reloaded_stitch_legacy = -9;
  int meets_at_17 = -1, meets_at_18 = -1, stitch_meets = -1;

  craft_project_begin(&f);
  test_clear_crafts();
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  fputs(legacy_file, file);
  rewind(file);
  test_load_crafts_from(file);
  fclose(file);

  sword = get_craft_from_id(11371);
  cap = get_craft_from_id(11372);
  stitch = get_craft_from_id(11373);
  hat = get_craft_from_id(11374);
  ring = get_craft_from_id(11375);
  if (sword && cap && stitch && hat && ring)
  {
    sword_skill = CRAFT_SKILL(sword);
    sword_rank = CRAFT_SKILL_LEVEL(sword);
    cap_skill = CRAFT_SKILL(cap);
    cap_rank = CRAFT_SKILL_LEVEL(cap);
    stitch_skill = CRAFT_SKILL(stitch);
    stitch_legacy = CRAFT_SKILL_LEGACY(stitch);
    stitch_rank = CRAFT_SKILL_LEVEL(stitch);
    hat_skill = CRAFT_SKILL(hat);
    ring_skill = CRAFT_SKILL(ring);
    ring_rank = CRAFT_SKILL_LEVEL(ring);
    SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 17);
    meets_at_17 = test_character_meets_craft_skill(ch, sword);
    SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 18);
    meets_at_18 = test_character_meets_craft_skill(ch, sword);
    SET_ABILITY(ch, ABILITY_CRAFT_TAILORING, 40);
    stitch_meets = test_character_meets_craft_skill(ch, stitch);
  }

  /* Write the catalog back and load that text again: no second conversion. */
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  test_save_crafts_to(file);
  rewind(file);
  saved_length = fread(saved, 1, sizeof(saved) - 1, file);
  saved[saved_length] = '\0';
  rewind(file);
  test_clear_crafts();
  test_load_crafts_from(file);
  fclose(file);
  sword = get_craft_from_id(11371);
  stitch = get_craft_from_id(11373);
  if (sword && stitch)
  {
    reloaded_sword_rank = CRAFT_SKILL_LEVEL(sword);
    reloaded_stitch_skill = CRAFT_SKILL(stitch);
    reloaded_stitch_legacy = CRAFT_SKILL_LEGACY(stitch);
  }
  test_clear_crafts();
  craft_project_end(&f);

  CuAssertIntEquals(tc, ABILITY_CRAFT_WEAPONSMITHING, sword_skill);
  CuAssertIntEquals(tc, 18, sword_rank);
  CuAssertIntEquals(tc, ABILITY_CRAFT_ARMORSMITHING, cap_skill);
  CuAssertIntEquals(tc, 14, cap_rank);
  CuAssertIntEquals(tc, CRAFT_SKILL_UNSUPPORTED, stitch_skill);
  CuAssertIntEquals(tc, 480, stitch_legacy);
  CuAssertIntEquals(tc, 50, stitch_rank);
  CuAssertIntEquals(tc, -1, hat_skill);
  CuAssertIntEquals(tc, ABILITY_CRAFT_JEWELCRAFTING, ring_skill);
  CuAssertIntEquals(tc, 5, ring_rank);
  CuAssertIntEquals(tc, 0, meets_at_17);
  CuAssertIntEquals(tc, 1, meets_at_18);
  CuAssertIntEquals(tc, 0, stitch_meets);
  CuAssertTrue(tc, strstr(saved, "Abil: 38 18\n") != NULL);
  CuAssertTrue(tc, strstr(saved, "Abil: 37 14\n") != NULL);
  CuAssertTrue(tc, strstr(saved, "Skil: 480 50\n") != NULL);
  CuAssertTrue(tc, strstr(saved, "Abil: -1 0\n") != NULL);
  CuAssertTrue(tc, strstr(saved, "Abil: 40 5\n") != NULL);
  CuAssertIntEquals(tc, 18, reloaded_sword_rank);
  CuAssertIntEquals(tc, CRAFT_SKILL_UNSUPPORTED, reloaded_stitch_skill);
  CuAssertIntEquals(tc, 480, reloaded_stitch_legacy);
}

/** A catalog recipe's storable material requirements are met from and spent against the shared
 * balances (aggregated over duplicate rows), exact gems stay objects, save-on-fail protects the
 * debit, and a missing ingredient consumes nothing. */
void Test_catalog_material_requirements_spend_shared_balances(CuTest *tc)
{
  static const struct craft_test_prototype rows[] = {
      {ADAMANTINE_MATERIAL, MATERIAL_ADAMANTINE, CRAFT_MAT_ADAMANTINE, "adamantine"},
      {ONYX_MATERIAL, MATERIAL_ONYX, CRAFT_MAT_NONE, "onyx"},
  };
  static const char *catalog = "NEW\nName: Adamantine Blade\nId  : 11380\nFlag: 0\nVnum: 3299\n"
                               "Time: 10\nAbil: 38 1\nMslf: You craft $p.\nMroo: $n crafts $p.\n"
                               "Req : 3193 6 0\nReq : 3193 4 4\nReq : 3187 2 0\nEnd :\n$\n";
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct craft_data *blade;
  struct obj_data *onyx_one, *onyx_two;
  FILE *file;
  int missing_short = -9, missing_with_balance = -9, missing_no_gems = -9, missing_ready = -9;
  int balance_after_refusal = -9, balance_after_fail = -9, balance_after_success = -9;
  int gems_after_success = -9, refused = 0;

  craft_project_begin(&f);
  craft_project_use_prototypes(&f, rows, 2);
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 20);
  test_clear_crafts();
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  fputs(catalog, file);
  rewind(file);
  test_load_crafts_from(file);
  fclose(file);
  blade = get_craft_from_id(11380);
  CuAssertPtrNotNull(tc, blade);

  /* Nine adamantine in the balance and no gems: the balance row is short and the gems missing. */
  GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE) = 9;
  missing_short = test_missing_craft_requirements(ch, blade);
  GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE) = 10;
  missing_no_gems = test_missing_craft_requirements(ch, blade);
  onyx_one = read_object(ONYX_MATERIAL, VIRTUAL);
  onyx_two = read_object(ONYX_MATERIAL, VIRTUAL);
  CuAssertPtrNotNull(tc, onyx_one);
  CuAssertPtrNotNull(tc, onyx_two);
  obj_to_char(onyx_one, ch);
  obj_to_char(onyx_two, ch);
  missing_ready = test_missing_craft_requirements(ch, blade);
  GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE) = 9;
  missing_with_balance = test_missing_craft_requirements(ch, blade);

  /* The command refuses a short recipe before anything is spent. */
  craft_project_reset_output(&f);
  do_craft_with_kits(ch, "Adamantine Blade", 0, 0);
  refused = craft_project_output_has(&f, "still missing");
  balance_after_refusal = GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE);

  /* A failure keeps the save-on-fail four units and spends the other six. */
  GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE) = 10;
  test_remove_components(ch, blade, FALSE);
  balance_after_fail = GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE);

  /* Success spends all ten and both gems. */
  GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE) = 10;
  if (ch->carrying != onyx_one && ch->carrying != onyx_two)
  {
    onyx_one = read_object(ONYX_MATERIAL, VIRTUAL);
    onyx_two = read_object(ONYX_MATERIAL, VIRTUAL);
    obj_to_char(onyx_one, ch);
    obj_to_char(onyx_two, ch);
  }
  test_remove_components(ch, blade, TRUE);
  balance_after_success = GET_CRAFT_MAT(ch, CRAFT_MAT_ADAMANTINE);
  gems_after_success = ch->carrying == NULL ? 0 : 1;
  test_clear_crafts();
  craft_project_end(&f);

  CuAssertIntEquals(tc, 2, missing_short);
  CuAssertIntEquals(tc, 1, missing_no_gems);
  CuAssertIntEquals(tc, 0, missing_ready);
  CuAssertIntEquals(tc, 1, missing_with_balance);
  CuAssertIntEquals(tc, 1, refused);
  CuAssertIntEquals(tc, 9, balance_after_refusal);
  CuAssertIntEquals(tc, 4, balance_after_fail);
  CuAssertIntEquals(tc, 0, balance_after_success);
  CuAssertIntEquals(tc, 0, gems_after_success);
}

/** craft, craftscore, and the editor's own score reach the same ranks; skill-less kit work
 * keeps its base timer. */
void Test_craft_commands_show_one_rank_space(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  int score_shows = 0, craft_score_shows = 0, menu_lists_catalog = 0;

  craft_project_begin(&f);
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 20);
  craft_project_reset_output(&f);
  do_craft_score(ch, "", 0, 0);
  score_shows =
      craft_project_output_has(&f, "weaponsmithing") && craft_project_output_has(&f, " 20 ");
  craft_project_reset_output(&f);
  do_craft(ch, "score", 0, 0);
  craft_score_shows =
      craft_project_output_has(&f, "weaponsmithing") && craft_project_output_has(&f, " 20 ");
  craft_project_reset_output(&f);
  do_craft(ch, "", 0, 0);
  menu_lists_catalog = craft_project_output_has(&f, "craft catalog");
  craft_project_end(&f);

  CuAssertIntEquals(tc, 1, score_shows);
  CuAssertIntEquals(tc, 1, craft_score_shows);
  CuAssertIntEquals(tc, 1, menu_lists_catalog);
  CuAssertIntEquals(tc, 30, craft_legacy_kit_seconds(ch, -1, 5));
  CuAssertIntEquals(tc, 6, craft_legacy_kit_seconds(ch, ABILITY_CRAFT_ALCHEMY, 1));
}

/* ---- Kit operations on the shared lifecycle (crafting consolidation, Phase 4) ---- */

/** An empty container carried by the crafter, used as the crafting kit. */
static struct obj_data *craft_project_kit(struct char_data *ch)
{
  struct obj_data *kit = read_object(WEAPON_PROTO, VIRTUAL);

  if (kit == NULL)
    return NULL;
  GET_OBJ_TYPE(kit) = ITEM_CONTAINER;
  obj_to_char(kit, ch);
  return kit;
}

/** A body-armor mold of the given material and level, weighing enough for five units. */
static struct obj_data *craft_project_mold(int material, int level)
{
  struct obj_data *mold = read_object(WEAPON_PROTO, VIRTUAL);

  if (mold == NULL)
    return NULL;
  GET_OBJ_TYPE(mold) = ITEM_ARMOR;
  SET_BIT_AR(GET_OBJ_EXTRA(mold), ITEM_MOLD);
  SET_BIT_AR(GET_OBJ_WEAR(mold), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_WEAR(mold), ITEM_WEAR_BODY);
  GET_OBJ_MATERIAL(mold) = material;
  GET_OBJ_LEVEL(mold) = level;
  GET_OBJ_WEIGHT(mold) = 100;
  GET_OBJ_COST(mold) = 10;
  return mold;
}

/** A mold becomes equipment at completion, through the kit or 'craft mold': crystal affects
 * and enhancement, the essence's masterwork, the feat bonus, the level, the strings, the
 * balance debit, and the gold all land once; nothing moves at admission. */
void Test_kit_create_makes_equipment_from_a_mold_and_balances(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *kit, *mold, *crystal, *essence, *wood_mold;
  enum domain_event_status runtime;
  int create_cmd, objects_before, objects_after;
  bool created_commands = false;
  int mold_at_admission = -1, steel_at_admission = -1, gold_at_admission = -1;
  int mold_after = -1, level_after = -1, material_after = -1, str_after = -1, bonus_after = -1;
  int enhancement_after = -1, masterwork_after = -1, steel_after = -1, gold_after = -1;
  int exp_after = -1, in_inventory = 0, named = 0, wood_material = -1, ash_after = -1;
  int wood_in_inventory = 0;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  if (complete_cmd_info == NULL)
  {
    created_commands = true;
    create_command_list();
  }
  create_cmd = find_command("create");
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  GET_LEVEL(ch) = LVL_IMMORT; /* the staff override makes the masterwork roll certain */
  SET_ABILITY(ch, ABILITY_CRAFT_ARMORSMITHING, 20);
  SET_FEAT(ch, FEAT_MASTERWORK_CRAFTING, 1);
  GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL) = 10;
  GET_GOLD(ch) = 1000;

  kit = craft_project_kit(ch);
  mold = craft_project_mold(MATERIAL_STEEL, 3);
  crystal = read_object(WEAPON_PROTO, VIRTUAL);
  essence = read_object(WEAPON_PROTO, VIRTUAL);
  CuAssertPtrNotNull(tc, kit);
  CuAssertPtrNotNull(tc, mold);
  CuAssertPtrNotNull(tc, crystal);
  CuAssertPtrNotNull(tc, essence);
  GET_OBJ_TYPE(crystal) = ITEM_CRYSTAL;
  GET_OBJ_LEVEL(crystal) = 4;
  crystal->affected[0].location = APPLY_STR;
  crystal->affected[0].modifier = 1;
  GET_OBJ_TYPE(essence) = ITEM_ESSENCE;
  GET_OBJ_LEVEL(essence) = 5;
  obj_to_obj(mold, kit);
  obj_to_obj(crystal, kit);
  obj_to_obj(essence, kit);
  objects_before = craft_project_live_objects();

  crafting_kit(ch, kit, create_cmd, "a steel breastplate");
  mold_at_admission = OBJ_FLAGGED(mold, ITEM_MOLD) && mold->in_obj == kit;
  steel_at_admission = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL);
  gold_at_admission = GET_GOLD(ch);
  craft_project_advance_seconds(70);
  mold_after = OBJ_FLAGGED(mold, ITEM_MOLD);
  in_inventory = mold->carried_by == ch;
  level_after = GET_OBJ_LEVEL(mold);
  material_after = GET_OBJ_MATERIAL(mold);
  str_after = mold->affected[0].location == APPLY_STR ? mold->affected[0].modifier : -1;
  bonus_after = mold->affected[0].bonus_type;
  enhancement_after = GET_OBJ_VAL(mold, 4);
  masterwork_after =
      mold->affected[3].location != 0 && mold->affected[3].bonus_type == BONUS_TYPE_INHERENT;
  steel_after = GET_CRAFT_MAT(ch, CRAFT_MAT_STEEL);
  gold_after = GET_GOLD(ch);
  exp_after = GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_ARMORSMITHING);
  named = mold->short_description && !strcmp(mold->short_description, "a steel breastplate");
  objects_after = craft_project_live_objects();

  /* The editor's entry point runs the same operation on a wood mold from wilderness balances;
   * it finds the kit by its special-procedure binding. */
  obj_index[GET_OBJ_RNUM(kit)].func = crafting_kit;
  wood_mold = craft_project_mold(MATERIAL_WOOD, 1);
  CuAssertPtrNotNull(tc, wood_mold);
  obj_to_obj(wood_mold, kit);
  GET_CRAFT_MAT(ch, CRAFT_MAT_ASH_WOOD) = 5;
  do_craft(ch, "mold an ash wood cuirass", 0, 0);
  craft_project_advance_seconds(70);
  wood_material = GET_OBJ_MATERIAL(wood_mold);
  ash_after = GET_CRAFT_MAT(ch, CRAFT_MAT_ASH_WOOD);
  wood_in_inventory = wood_mold->carried_by == ch && !OBJ_FLAGGED(wood_mold, ITEM_MOLD);

  obj_index[GET_OBJ_RNUM(kit)].func = NULL;
  if (created_commands)
    free_command_list();
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, create_cmd >= 0);
  CuAssertIntEquals(tc, 1, mold_at_admission);
  CuAssertIntEquals(tc, 10, steel_at_admission);
  CuAssertIntEquals(tc, 1000, gold_at_admission);
  CuAssertIntEquals(tc, 0, mold_after);
  CuAssertIntEquals(tc, 1, in_inventory);
  CuAssertIntEquals(tc, 4, level_after);
  CuAssertIntEquals(tc, MATERIAL_STEEL, material_after);
  CuAssertIntEquals(tc, 1, str_after);
  CuAssertIntEquals(tc, BONUS_TYPE_ENHANCEMENT, bonus_after);
  CuAssertIntEquals(tc, 1, enhancement_after);
  CuAssertIntEquals(tc, 1, masterwork_after);
  CuAssertIntEquals(tc, 5, steel_after);
  CuAssertIntEquals(tc, 1000 - 4 * 4 * 100 / 3, gold_after);
  CuAssertTrue(tc, exp_after > 0);
  CuAssertIntEquals(tc, 1, named);
  /* The crystal and the essence were consumed. */
  CuAssertIntEquals(tc, objects_before - 2, objects_after);
  CuAssertIntEquals(tc, MATERIAL_ASH, wood_material);
  CuAssertIntEquals(tc, 0, ash_after);
  CuAssertIntEquals(tc, 1, wood_in_inventory);
}

/** Redesc changes nothing at admission or on cancellation, and applies its description, its
 * gold, and its delivery exactly once at completion. */
void Test_kit_utility_changes_nothing_until_completion(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *kit, *item;
  enum domain_event_status runtime;
  struct primary_activity_snapshot snapshot;
  int redesc_cmd;
  bool created_commands = false;
  int started = 0, unchanged_at_admission = 0, unchanged_after_cancel = 0, still_in_kit = 0;
  int described = 0, gold_after = -1, delivered = 0, exp_after = -1;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  if (complete_cmd_info == NULL)
  {
    created_commands = true;
    create_command_list();
  }
  redesc_cmd = find_command("redesc");
  GET_GOLD(ch) = 100;
  kit = craft_project_kit(ch);
  item = craft_project_reforgeable_dagger();
  CuAssertPtrNotNull(tc, kit);
  CuAssertPtrNotNull(tc, item);
  GET_OBJ_LEVEL(item) = 1;
  obj_to_obj(item, kit);

  crafting_kit(ch, kit, redesc_cmd, "A plain steel blade, well kept.");
  started = primary_activity_snapshot(ch, &snapshot);
  unchanged_at_admission =
      item->ex_description == NULL && GET_GOLD(ch) == 100 && item->in_obj == kit;
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
  craft_project_advance_seconds(40);
  unchanged_after_cancel = item->ex_description == NULL && GET_GOLD(ch) == 100;
  still_in_kit = item->in_obj == kit;

  crafting_kit(ch, kit, redesc_cmd, "A plain steel blade, well kept.");
  craft_project_advance_seconds(40);
  described = item->ex_description != NULL && item->ex_description->description != NULL &&
              strstr(item->ex_description->description, "well kept") != NULL;
  gold_after = GET_GOLD(ch);
  delivered = item->carried_by == ch;
  exp_after = GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_WEAPONSMITHING);

  if (created_commands)
    free_command_list();
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, redesc_cmd >= 0);
  CuAssertIntEquals(tc, 1, started);
  CuAssertIntEquals(tc, 1, unchanged_at_admission);
  CuAssertIntEquals(tc, 1, unchanged_after_cancel);
  CuAssertIntEquals(tc, 1, still_in_kit);
  CuAssertIntEquals(tc, 1, described);
  CuAssertIntEquals(tc, 90, gold_after);
  CuAssertIntEquals(tc, 1, delivered);
  /* Renaming is skill-less: no craft experience. */
  CuAssertIntEquals(tc, 0, exp_after);
}

/** Both reforge front ends run one validation: without the forge neither admits, and while
 * one crafting activity runs no other crafting, catalog, or brew work is admitted. */
void Test_reforge_contexts_share_validation_and_refuse_overlap(CuTest *tc)
{
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct obj_data *kit, *first, *second;
  enum domain_event_status runtime;
  struct primary_activity_snapshot snapshot;
  int reforge_cmd;
  bool created_commands = false;
  int standalone_refused = 0, kit_refused = 0, standalone_started = 0, kit_overlap_refused = 0;
  int catalog_overlap_refused = 0, brew_overlap_refused = 0;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  if (complete_cmd_info == NULL)
  {
    created_commands = true;
    create_command_list();
  }
  reforge_cmd = find_command("reforge");
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  kit = craft_project_kit(ch);
  first = craft_project_reforgeable_dagger();
  second = craft_project_reforgeable_dagger();
  CuAssertPtrNotNull(tc, kit);
  CuAssertPtrNotNull(tc, first);
  CuAssertPtrNotNull(tc, second);
  obj_to_char(first, ch);
  obj_to_obj(second, kit);

  /* No forge in the room: the same station rule refuses both. */
  f.room.contents = NULL;
  craft_project_reset_output(&f);
  do_reforge_new(ch, "weapon khop", 0, 0);
  standalone_refused =
      craft_project_output_has(&f, "You need") && !primary_activity_snapshot(ch, &snapshot);
  craft_project_reset_output(&f);
  crafting_kit(ch, kit, reforge_cmd, "khop");
  kit_refused =
      craft_project_output_has(&f, "You need") && !primary_activity_snapshot(ch, &snapshot);
  f.room.contents = &f.forge;

  do_reforge_new(ch, "weapon khop", 0, 0);
  standalone_started = primary_activity_snapshot(ch, &snapshot);
  craft_project_reset_output(&f);
  crafting_kit(ch, kit, reforge_cmd, "khop");
  kit_overlap_refused = craft_project_output_has(&f, "already doing something");
  craft_project_reset_output(&f);
  do_craft_with_kits(ch, "nothing", 0, 0);
  catalog_overlap_refused = craft_project_output_has(&f, "already busy") ||
                            craft_project_output_has(&f, "What are you trying to craft");
  craft_project_reset_output(&f);
  do_brew(ch, "bless", 0, 0);
  brew_overlap_refused = craft_project_output_has(&f, "already busy");
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);

  if (created_commands)
    free_command_list();
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertIntEquals(tc, 1, standalone_refused);
  CuAssertIntEquals(tc, 1, kit_refused);
  CuAssertIntEquals(tc, 1, standalone_started);
  CuAssertIntEquals(tc, 1, kit_overlap_refused);
  CuAssertIntEquals(tc, 1, catalog_overlap_refused);
  CuAssertIntEquals(tc, 1, brew_overlap_refused);
}

/** A catalog recipe runs on the activity manager: success creates the item and pays craft
 * experience once; a struggle retries in the same activity; a failure ends it with nothing. */
void Test_catalog_craft_resolves_success_retry_and_failure(CuTest *tc)
{
  static const char *catalog = "NEW\nName: Plain Dagger\nId  : 11390\nFlag: 0\nVnum: 3299\n"
                               "Time: 10\nAbil: 38 4\nMslf: You craft $p.\nMroo: $n crafts $p.\n"
                               "End :\n$\n";
  struct craft_project_fixture f;
  struct char_data *ch = &f.ch;
  struct primary_activity_snapshot snapshot;
  enum domain_event_status runtime;
  FILE *file;
  unsigned long seed, retry_seed = 0, failure_seed = 0;
  int r, objects_before, success_objects = -1, success_exp = -1, retry_active = -1;
  int retry_objects = -1, failure_active = -1, failure_objects = -1;

  craft_project_begin(&f);
  craft_project_start_events();
  runtime = domain_event_runtime_init();
  SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
  test_clear_crafts();
  file = tmpfile();
  CuAssertPtrNotNull(tc, file);
  fputs(catalog, file);
  rewind(file);
  test_load_crafts_from(file);
  fclose(file);
  objects_before = craft_project_live_objects();

  /* Rank 40 reads as 200 on the legacy scale: it beats any roll up to 40. */
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 40);
  do_craft_with_kits(ch, "Plain Dagger", 0, 0);
  CuAssertTrue(tc, primary_activity_snapshot(ch, &snapshot));
  craft_project_advance_seconds(12);
  success_objects = craft_project_live_objects() - objects_before;
  success_exp = GET_CRAFT_SKILL_EXP(ch, ABILITY_CRAFT_WEAPONSMITHING);

  /* Rank 4 (the threshold) reads as 20 against a roll of 0 to 40: 20 to 39 struggles and
   * retries, 40 fails outright. */
  SET_ABILITY(ch, ABILITY_CRAFT_WEAPONSMITHING, 4);
  for (seed = 1; seed < 200000 && (!retry_seed || !failure_seed); seed++)
  {
    circle_srandom(seed);
    r = rand_number(0, 40);
    if (r >= 20 && r <= 39 && !retry_seed)
      retry_seed = seed;
    if (r == 40 && !failure_seed)
      failure_seed = seed;
  }
  objects_before = craft_project_live_objects();
  do_craft_with_kits(ch, "Plain Dagger", 0, 0);
  circle_srandom(retry_seed);
  craft_project_advance_seconds(11);
  retry_active = primary_activity_snapshot(ch, &snapshot);
  retry_objects = craft_project_live_objects() - objects_before;
  primary_activity_cancel(ch, PRIMARY_ACTIVITY_END_PLAYER_CANCELLED, false);
  do_craft_with_kits(ch, "Plain Dagger", 0, 0);
  circle_srandom(failure_seed);
  craft_project_advance_seconds(11);
  failure_active = primary_activity_snapshot(ch, &snapshot);
  failure_objects = craft_project_live_objects() - objects_before;

  test_clear_crafts();
  domain_event_runtime_shutdown();
  event_free_all();
  craft_project_end(&f);

  CuAssertIntEquals(tc, DOMAIN_EVENT_OK, runtime);
  CuAssertTrue(tc, retry_seed && failure_seed);
  CuAssertIntEquals(tc, 1, success_objects);
  CuAssertTrue(tc, success_exp > 0);
  CuAssertIntEquals(tc, 1, retry_active);
  CuAssertIntEquals(tc, 0, retry_objects);
  CuAssertIntEquals(tc, 0, failure_active);
  CuAssertIntEquals(tc, 0, failure_objects);
}
