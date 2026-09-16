/* Production-linked tests for the materials-and-motes crafting system (crafting_system 2): its boot
 * tables and the equipment project a player builds with the craft command. */

#include "CuTest.h"

#include "conf.h"
#include "../../src/core/sysdep.h"
#include "../../src/core/structs.h"
#include "../../src/core/utils.h"
#include "../../src/character/feats.h"
#include "../../src/character/talents.h"
#include "../../src/combat/assign_wpn_armor.h"
#include "../../src/config/vnums.h"
#include "../../src/core/comm.h"
#include "../../src/core/constants.h"
#include "../../src/core/db.h"
#include "../../src/core/handler.h"
#include "../../src/core/interpreter.h"
#include "../../src/craft/crafting_new.h"
#include "../../src/craft/crafting_recipes.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/events/activity_manager.h"
#include "../../src/events/domain_event_runtime.h"
#include "../../src/events/domain_event_world.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"

#include <string.h>

/** A connected player in a smithy with a forge and a weaponsmith's hammer, plus the weapon
 * prototype that project objects are built from. */
struct craft_project_fixture
{
  struct room_data room;
  struct zone_data zone;
  struct index_data object_index;
  struct obj_data weapon_proto;
  struct obj_data forge;
  struct obj_data hammer;
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
  f->zone.top = WEAPON_PROTO;
  zone_table = &f->zone;
  top_of_zone_table = 0;

  clear_object(&f->forge);
  f->forge.name = CuMutableString("forge");
  SET_OBJ_FLAG(&f->forge, ITEM_CRAFTING_FORGE);
  IN_ROOM(&f->forge) = 0;
  clear_object(&f->hammer);

  /* Instances share the prototype strings, which free_obj() leaves alone. */
  clear_object(&f->weapon_proto);
  f->weapon_proto.item_number = 0;
  GET_OBJ_TYPE(&f->weapon_proto) = ITEM_WEAPON;
  f->weapon_proto.name = CuMutableString("weapon prototype");
  f->weapon_proto.short_description = CuMutableString("a weapon prototype");
  f->weapon_proto.description = CuMutableString("A weapon prototype lies here.");
  f->object_index.vnum = WEAPON_PROTO;
  obj_proto = &f->weapon_proto;
  obj_index = &f->object_index;
  top_of_objt = 0;
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
