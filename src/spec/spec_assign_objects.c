/**************************************************************************
 *  File: spec/spec_assign_objects.c                  Part of LuminariMUD *
 *  Usage: Compiled object special-procedure assignment inventory.         *
 *                                                                         *
 *  All rights reserved. See license for complete information.             *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"

#include "character/vampire_cloak.h"
#include "comms/boards.h"
#include "craft/craft.h"
#include "magic/spells.h"
#include "obj/vendor.h"
#include "spec_assign.h"
#include "spec_assign_internal.h"
#include "spec_assign_table.h"
#include "spec_objects.h"
#include "spec_zone_fire_giant.h"
#include "spec_zone_jot.h"
#include "spec_zone_neverwinter.h"
#include "spec_zone_prisoner.h"
#include "vessels/vessels_legacy.h"

#define SPEC_ASSIGN_STRINGIFY_INNER(value) #value
#define SPEC_ASSIGN_STRINGIFY(value) SPEC_ASSIGN_STRINGIFY_INNER(value)
#define SPEC_ASSIGN_LOCATION "src/spec/spec_assign_objects.c:" SPEC_ASSIGN_STRINGIFY(__LINE__)

#define ASSIGNOBJ(obj, handler) spec_assign_object((obj), (handler), #handler, SPEC_ASSIGN_LOCATION)

#if !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
static const struct spec_obj_assignment luminari_object_assignments[] = {
    {NOOB_CRAFTING_KIT, "Crafting Kit"},
    {VAMPIRE_CLOAK_OBJ_VNUM, "Vampire Cloak"},
};

#define LUMINARI_OBJECT_ASSIGNMENT_COUNT                                                           \
  (sizeof(luminari_object_assignments) / sizeof(luminari_object_assignments[0]))

/** Apply one validated object table through the shared assignment path. */
static void apply_object_assignments(const struct spec_obj_assignment *rows, size_t count,
                                     const char *source_location)
{
  const struct spec_definition *definition;
  char error[256];
  size_t index;

  for (index = 0; index < count; index++)
  {
    definition = spec_assign_table_resolve(rows[index].definition_name, SPEC_OWNER_OBJECT, error,
                                           sizeof(error));
    if (definition == NULL)
    {
      log("SYSERR: Skipping declarative object assignment for #%d: %s", rows[index].vnum, error);
      continue;
    }
    spec_assign_object(rows[index].vnum, spec_definition_callback(definition),
                       definition->canonical_name, source_location);
  }
}
#endif

/**
 * Validate every declarative table against the registry.
 *
 * A bad row is a programmer error, not content, so it fails boot rather than
 * silently binding nothing. This matches the registry's own pre-world-load
 * contract and runs alongside it.
 */
void spec_assign_table_boot_validate(void)
{
  char error[512];

#if !defined(CAMPAIGN_FR) && !defined(CAMPAIGN_DL)
  if (!spec_assign_table_validate_objects(luminari_object_assignments,
                                          LUMINARI_OBJECT_ASSIGNMENT_COUNT, error, sizeof(error)))
  {
    log("SYSERR: Invalid declarative object assignment table: %s", error);
    exit(1);
  }
#else
  (void)error;
#endif
}

/* assign special procedures to objects */
void assign_objects(void)
{
#ifdef CAMPAIGN_FR

  ASSIGNOBJ(115, bank);

  ASSIGNOBJ(3118, crafting_kit);
  ASSIGNOBJ(3322, crafting_kit);

  ASSIGNOBJ(VAMPIRE_CLOAK_OBJ_VNUM, vampire_cloak);
#elif defined(CAMPAIGN_DL)
  // general
  ASSIGNOBJ(VAMPIRE_CLOAK_OBJ_VNUM, vampire_cloak);

#else
  apply_object_assignments(luminari_object_assignments, LUMINARI_OBJECT_ASSIGNMENT_COUNT,
                           SPEC_ASSIGN_LOCATION);

  ASSIGNOBJ(1226, gen_board);   /* builder's board */
  ASSIGNOBJ(1227, gen_board);   /* staff board */
  ASSIGNOBJ(1228, gen_board);   /* advertising board */
  ASSIGNOBJ(3096, gen_board);   /* social board */
  ASSIGNOBJ(3097, gen_board);   /* freeze board */
  ASSIGNOBJ(3098, gen_board);   /* immortal board */
  ASSIGNOBJ(3099, gen_board);   /* mortal board */
  ASSIGNOBJ(100400, gen_board); /* quest board */
  ASSIGNOBJ(103093, gen_board); /* ashenport market board */
  ASSIGNOBJ(103094, gen_board); /* forger board */
  ASSIGNOBJ(103095, gen_board); /* areas board */
  ASSIGNOBJ(103096, gen_board); /* social board */
  ASSIGNOBJ(103097, gen_board); /* freeze board */
  ASSIGNOBJ(103098, gen_board); /* immortal board */
  ASSIGNOBJ(103099, gen_board); /* mortal board */

  ASSIGNOBJ(115, bank);
  ASSIGNOBJ(334, bank);  /* atm */
  ASSIGNOBJ(336, bank);  /* cashcard */
  ASSIGNOBJ(3034, bank); /* atm */
  ASSIGNOBJ(3036, bank); /* cashcard */
  ASSIGNOBJ(3907, bank);
  ASSIGNOBJ(10640, bank);
  ASSIGNOBJ(10751, bank);
  ASSIGNOBJ(25758, bank);
  ASSIGNOBJ(102541, bank);
  ASSIGNOBJ(103122, bank);
  /* homeland - need to be converted to objects */
  /*
    ASSIGNOBJ(105039, bank);
    ASSIGNOBJ(105807, bank);
    ASSIGNOBJ(108181, bank);
    ASSIGNOBJ(108198, bank);
    ASSIGNOBJ(110421, bank);
    ASSIGNOBJ(111374, bank);
    ASSIGNOBJ(113010, bank);
    ASSIGNOBJ(119102, bank);
    ASSIGNOBJ(121475, bank);
    ASSIGNOBJ(121825, bank);
    ASSIGNOBJ(122685, bank);
    ASSIGNOBJ(125088, bank);
    ASSIGNOBJ(125984, bank);
    ASSIGNOBJ(126906, bank);
    ASSIGNOBJ(127618, bank);
    ASSIGNOBJ(135051, bank);
    ASSIGNOBJ(138809, bank);
   */

  ASSIGNOBJ(128106, ches); // weapon

  ASSIGNOBJ(128150, spikeshield); // shield

  ASSIGNOBJ(224, monk_glove); /*electric damage*/
  ASSIGNOBJ(9215, monk_glove_cold);

  /* fire giant zone */
  ASSIGNOBJ(34549, flamekissed_instrument);

  /* the prisoner */
  ASSIGNOBJ(132125, tia_rapier);
  // ASSIGNOBJ(132109, magi_staff);
  ASSIGNOBJ(132104, star_circlet);
  ASSIGNOBJ(132101, malevolence);
  ASSIGNOBJ(132128, speed_gaunts);
  ASSIGNOBJ(132126, rune_scimitar);
  ASSIGNOBJ(132300, celestial_sword);
  ASSIGNOBJ(132133, stability_boots);
  ASSIGNOBJ(132118, ancient_moonblade);

  ASSIGNOBJ(132115, warbow);

  /* not yet defined? */
  // ASSIGNOBJ(133103, mithril_rapier);
  // ASSIGNOBJ(141800, treantshield);

  ASSIGNOBJ(136100, air_sphere); // weapon (lightning)

  /* JOTUNHEIM EQ */
  ASSIGNOBJ(2096012, mistweave);
  ASSIGNOBJ(2096000, frostbite);
  ASSIGNOBJ(2096059, ymir_cloak);
  ASSIGNOBJ(2096062, vaprak_claws);
  ASSIGNOBJ(2096056, valkyrie_sword);
  ASSIGNOBJ(2096081, twilight);
  ASSIGNOBJ(2096090, fake_twilight);
  ASSIGNOBJ(2096066, giantslayer);
  ASSIGNOBJ(2096073, planetar_sword);
  ASSIGNOBJ(2096087, skullsmasher);

  /* more homeland, unsorted */
  ASSIGNOBJ(100400, gen_board); /* quest board */

  ASSIGNOBJ(123419, neverwinter_button_control);
  ASSIGNOBJ(123418, neverwinter_valve_control);

  ASSIGNOBJ(113803, nutty_bracer);

  /* Moving Portals */
  ASSIGNOBJ(106019, floating_teleport);

  ASSIGNOBJ(110015, floating_teleport);

  ASSIGNOBJ(112500, floating_teleport);

  ASSIGNOBJ(126703, floating_teleport);
  ASSIGNOBJ(126712, floating_teleport);
  ASSIGNOBJ(126713, floating_teleport);
  ASSIGNOBJ(126714, floating_teleport);
  ASSIGNOBJ(126715, floating_teleport);

  ASSIGNOBJ(129015, floating_teleport);

  ASSIGNOBJ(129500, floating_teleport);
  ASSIGNOBJ(129501, floating_teleport);
  ASSIGNOBJ(129502, floating_teleport);

  ASSIGNOBJ(136400, floating_teleport);

  ASSIGNOBJ(139200, floating_teleport);
  ASSIGNOBJ(139201, floating_teleport);
  ASSIGNOBJ(139202, floating_teleport);
  // ASSIGNOBJ(139203, floating_teleport); // obj #139203 doesn't exist

  /* ferry, ferry-like */
  ASSIGNOBJ(104072, chionthar_ferry);
  ASSIGNOBJ(126429, alandor_ferry);
  // ASSIGNOBJ(120010, md_carpet); // obj #120010 doesn't exist

  /* purchased pet objects */
  ASSIGNOBJ(118190, bought_pet);
  ASSIGNOBJ(103670, bought_pet);
  ASSIGNOBJ(103671, bought_pet);
  ASSIGNOBJ(103672, bought_pet);
  ASSIGNOBJ(103673, bought_pet);
  ASSIGNOBJ(103674, bought_pet);

  /* not yet defined
  ASSIGNOBJ(101290, storage_chest);
  ASSIGNOBJ(101291, storage_chest);
   */

  /* not yet defined? */
  // ASSIGNOBJ(100600, forest_idol);
  // ASSIGNOBJ(100601, forest_idol);
  // ASSIGNOBJ(100602, forest_idol);
  // ASSIGNOBJ(100603, forest_idol);
  // ASSIGNOBJ(100604, forest_idol);
  // ASSIGNOBJ(100605, forest_idol);

  /* Weapon Procs */
  ASSIGNOBJ(141914, witherdirk);
  ASSIGNOBJ(135511, snakewhip);
  ASSIGNOBJ(135500, snakewhip);
  ASSIGNOBJ(135534, snakewhip);
  ASSIGNOBJ(135199, acidsword);
  ASSIGNOBJ(100510, halberd);
  // ASSIGNOBJ(100513, halberd); // obj #100513 doesn't exist

  ASSIGNOBJ(114838, rughnark);
  ASSIGNOBJ(139900, magma);
  ASSIGNOBJ(110601, bolthammer);
  // ASSIGNOBJ(111507, prismorb); // obj #111507 doesn't exist
  ASSIGNOBJ(129602, flamingwhip);
  ASSIGNOBJ(126315, dorfaxe);
  ASSIGNOBJ(121207, helmblade);
  ASSIGNOBJ(117014, bloodaxe);
  ASSIGNOBJ(100501, xvim_artifact);
  ASSIGNOBJ(100502, xvim_normal);

  ASSIGNOBJ(109802, whisperwind);
  ASSIGNOBJ(127224, sparksword);
  ASSIGNOBJ(100581, tyrantseye);
  ASSIGNOBJ(113898, flaming_scimitar);
  ASSIGNOBJ(113897, frosty_scimitar);
  ASSIGNOBJ(129011, purity);
  ASSIGNOBJ(117024, etherealness);
  ASSIGNOBJ(129001, greatsword);
  ASSIGNOBJ(125519, sarn);
  ASSIGNOBJ(115003, fog_dagger);
  ASSIGNOBJ(115007, dragonbone_hammer);
  ASSIGNOBJ(126704, viperdagger);
  ASSIGNOBJ(126717, acidstaff);
  ASSIGNOBJ(132102, hellfire);
  ASSIGNOBJ(110017, vengeance);
  ASSIGNOBJ(101199, vengeance);
  ASSIGNOBJ(101849, skullsmasher);
  ASSIGNOBJ(101850, skullsmasher);
  ASSIGNOBJ(139250, courage);
  ASSIGNOBJ(139251, courage);
  ASSIGNOBJ(121456, clang_bracer);
  ASSIGNOBJ(128150, spikeshield);
  ASSIGNOBJ(128106, ches);
  ASSIGNOBJ(100596, tormblade);
  // ASSIGNOBJ(100599, tormblade); // obj #100599 doesn't exist

  ASSIGNOBJ(138447, disruption_mace);
  ASSIGNOBJ(138415, haste_bracers);
  ASSIGNOBJ(135626, menzo_chokers);
  ASSIGNOBJ(135627, menzo_chokers);
  ASSIGNOBJ(106021, angel_leggings);
  ASSIGNOBJ(135535, spiderdagger);

  /* clouds realm */
  ASSIGNOBJ(144669, dragon_robes);

#endif
}

/* eof */
