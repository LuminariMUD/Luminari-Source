/**************************************************************************
 *  File: spec/spec_assign_rooms.c                    Part of LuminariMUD *
 *  Usage: Compiled room special-procedure assignment inventory.           *
 *                                                                         *
 *  All rights reserved. See license for complete information.             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"

#include "craft/craft.h"
#include "magic/spellbook_scroll.h"
#include "obj/treasure.h"
#include "obj/vendor.h"
#include "spec_assign.h"
#include "spec_assign_internal.h"
#include "spec_rooms.h"
#include "spec_zone_abyssal_vortex.h"
#include "spec_zone_hive_of_passion.h"
#include "spec_zone_kenjin_tower.h"
#include "spec_zone_quicksand.h"
#include "vessels/vessels_legacy.h"

#define SPEC_ASSIGN_STRINGIFY_INNER(value) #value
#define SPEC_ASSIGN_STRINGIFY(value) SPEC_ASSIGN_STRINGIFY_INNER(value)
#define SPEC_ASSIGN_LOCATION "src/spec/spec_assign_rooms.c:" SPEC_ASSIGN_STRINGIFY(__LINE__)

#define ASSIGNROOM(room, handler)                                                                  \
  spec_assign_room((room), (handler), #handler, SPEC_ASSIGN_LOCATION)

/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;


  /* bazaar - spend quest points on magic gear */
  ASSIGNROOM(103006, bazaar);

  /* crafting quest (autocraft) */
  ASSIGNROOM(370, crafting_quest);

  /* wizard library - research wizard spells for spellbook */
  ASSIGNROOM(5905, wizard_library);   /* wizard training mansion */
  ASSIGNROOM(103047, wizard_library); /* Ashenport Mage's Guild */

  /* buy pets */
  ASSIGNROOM(3031, pet_shops);
  ASSIGNROOM(10738, pet_shops);
  ASSIGNROOM(23281, pet_shops);
  ASSIGNROOM(25722, pet_shops);
  ASSIGNROOM(27155, pet_shops);
  ASSIGNROOM(27616, pet_shops);
  ASSIGNROOM(31523, pet_shops);
  /* this doesn't seem to be at all valid */
  // ASSIGNROOM(103031, pet_shops);
  ASSIGNROOM(145287, pet_shops); /* mosswood petshop */

  /* abyssal vortex */
  ASSIGNROOM(139200, abyssal_vortex);
  ASSIGNROOM(139201, abyssal_vortex);
  ASSIGNROOM(139202, abyssal_vortex);
  ASSIGNROOM(139203, abyssal_vortex);
  ASSIGNROOM(139204, abyssal_vortex);
  ASSIGNROOM(139205, abyssal_vortex);
  ASSIGNROOM(139206, abyssal_vortex);
  ASSIGNROOM(139207, abyssal_vortex);
  ASSIGNROOM(139208, abyssal_vortex);
  ASSIGNROOM(139209, abyssal_vortex);
  ASSIGNROOM(139210, abyssal_vortex);
  ASSIGNROOM(139211, abyssal_vortex);
  ASSIGNROOM(139212, abyssal_vortex);
  ASSIGNROOM(139213, abyssal_vortex);
  ASSIGNROOM(139214, abyssal_vortex);
  ASSIGNROOM(139215, abyssal_vortex);
  ASSIGNROOM(139216, abyssal_vortex);
  ASSIGNROOM(139217, abyssal_vortex);
  ASSIGNROOM(139218, abyssal_vortex);
  ASSIGNROOM(139219, abyssal_vortex);
  ASSIGNROOM(139210, abyssal_vortex);
  ASSIGNROOM(139221, abyssal_vortex);
  ASSIGNROOM(139222, abyssal_vortex);
  ASSIGNROOM(139223, abyssal_vortex);
  ASSIGNROOM(139224, abyssal_vortex);
  ASSIGNROOM(139225, abyssal_vortex);
  ASSIGNROOM(139226, abyssal_vortex);
  ASSIGNROOM(139227, abyssal_vortex);
  ASSIGNROOM(139228, abyssal_vortex);
  ASSIGNROOM(139229, abyssal_vortex);
  ASSIGNROOM(139230, abyssal_vortex);
  ASSIGNROOM(139231, abyssal_vortex);
  ASSIGNROOM(139232, abyssal_vortex);
  ASSIGNROOM(139233, abyssal_vortex);
  ASSIGNROOM(139234, abyssal_vortex);
  ASSIGNROOM(139235, abyssal_vortex);
  ASSIGNROOM(139236, abyssal_vortex);
  ASSIGNROOM(139237, abyssal_vortex);
  ASSIGNROOM(139238, abyssal_vortex);
  ASSIGNROOM(139239, abyssal_vortex);
  ASSIGNROOM(139240, abyssal_vortex);
  ASSIGNROOM(139241, abyssal_vortex);
  ASSIGNROOM(139242, abyssal_vortex);
  ASSIGNROOM(139243, abyssal_vortex);
  ASSIGNROOM(139244, abyssal_vortex);
  ASSIGNROOM(139245, abyssal_vortex);
  ASSIGNROOM(139246, abyssal_vortex);
  ASSIGNROOM(139247, abyssal_vortex);
  ASSIGNROOM(139248, abyssal_vortex);
  ASSIGNROOM(139249, abyssal_vortex);
  ASSIGNROOM(139250, abyssal_vortex);

  /* hive death */
  ASSIGNROOM(139300, hive_death);

  /* kt twister */
  ASSIGNROOM(132902, kt_twister);
  ASSIGNROOM(132903, kt_twister);
  ASSIGNROOM(132904, kt_twister);
  ASSIGNROOM(132905, kt_twister);

  /* kt shadowmaker */
  // ASSIGNROOM( 32921, kt_shadowmaker);

  /* quicksand */
  ASSIGNROOM(126771, quicksand);
  ASSIGNROOM(126776, quicksand);
  ASSIGNROOM(126752, quicksand);
  ASSIGNROOM(126710, quicksand);
  ASSIGNROOM(126716, quicksand);
  ASSIGNROOM(126731, quicksand);
  ASSIGNROOM(126870, quicksand);
  ASSIGNROOM(126871, quicksand);
  ASSIGNROOM(126887, quicksand);
  ASSIGNROOM(126831, quicksand);
  ASSIGNROOM(126840, quicksand);
  ASSIGNROOM(126848, quicksand);
  ASSIGNROOM(126788, quicksand);
  ASSIGNROOM(126793, quicksand);
  ASSIGNROOM(126800, quicksand);

  /* greyhawk ships */
  ASSIGNROOM(1403, greyhawk_ship_commands);
  ASSIGNROOM(70003, greyhawk_ship_commands);


  /* death traps are dumps, i.e. will destroy all gear that hits the ground */
  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
      {
        spec_assign_room(world[i].number, dump, "dump", SPEC_ASSIGN_LOCATION);
      }
}

/* eof */
