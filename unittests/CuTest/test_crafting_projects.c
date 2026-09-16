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
  ubyte saved_crafting_system;
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
  f->saved_crafting_system = CONFIG_CRAFTING_SYSTEM;
  CONFIG_CRAFTING_SYSTEM = CRAFTING_SYSTEM_MOTES;

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
  CONFIG_CRAFTING_SYSTEM = f->saved_crafting_system;
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
  bool show_refused, reset_refused, held;

  craft_project_begin(&f);
  craft_project_hold_supply_order(ch);
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][0] = CRAFT_MAT_STEEL;
  GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] = 6;

  newcraft_create(ch, "show");
  show_refused = craft_project_output_has(&f, "working on a supply order") &&
                 !craft_project_output_has(&f, "Current Craft Project");
  craft_project_reset_output(&f);
  newcraft_create(ch, "reset");
  reset_refused = craft_project_output_has(&f, "working on a supply order");
  held = player_has_supply_order(ch) && GET_CRAFT(ch).materials[CRAFT_GROUP_HARD_METALS][1] == 6;
  craft_project_end(&f);

  CuAssertTrue(tc, show_refused);
  CuAssertTrue(tc, reset_refused);
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
  /* Requests draw random recipes until one matches; an unseeded generator repeats one value. */
  circle_srandom(1);
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
  circle_srandom((unsigned long)time(NULL));
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
  bool stale_refused;

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
  craft_project_end(&f);

  CuAssertIntEquals(tc, 200, offers);
  CuAssertIntEquals(tc, offers, orderable);
  CuAssertTrue(tc, stale_refused);
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
  event_free_all();
  event_init();
  runtime = domain_event_runtime_init();
  weapon = craft_project_reforgeable_dagger();
  if (weapon != NULL)
  {
    obj_to_char(weapon, ch);
    do_reforge_new(ch, "weapon khop", 0, 0);
    abbreviated = GET_OBJ_VAL(weapon, 0);
    clear_char_event_list(ch);
    GET_CRAFTING_OBJ(ch) = NULL;
    do_reforge_new(ch, "weapon composite long bow (2)", 0, 0);
    numbered = GET_OBJ_VAL(weapon, 0);
    clear_char_event_list(ch);
    GET_CRAFTING_OBJ(ch) = NULL;
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
  event_free_all();
  event_init();
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
    reforged = GET_OBJ_VAL(first, 0);
    clear_char_event_list(ch);
    GET_CRAFTING_OBJ(ch) = NULL;
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
