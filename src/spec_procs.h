/**
 * @file spec_procs.h                                   LuminariMUD
 * Header file for special procedure modules. This file groups a lot of the
 * legacy special procedures found in spec_procs.c and their feature owners.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 *
 */
#ifndef _SPEC_PROCS_H_
#define _SPEC_PROCS_H_

#include "character/abilities.h"
#include "character/guild_services.h"
#include "character/skill_lists.h"
#include "clan_services.h"
#include "magic/spellbook_scroll.h"
#include "magic/spell_lists.h"
#include "magic/spells.h"
#include "obj/vendor.h"
#include "spec/spec_mobile_archetypes.h"
#include "spec/spec_mobiles.h"
#include "spec/spec_rooms.h"
#include "spec/spec_zone_abyss.h"
#include "spec/spec_zone_abyssal_vortex.h"
#include "spec/spec_zone_agrach_dyrr.h"
#include "spec/spec_zone_banshee.h"
#include "spec/spec_zone_celestial_leviathan.h"
#include "spec/spec_zone_crimson_flame.h"
#include "spec/spec_zone_earth_plane.h"
#include "spec/spec_zone_feybranche.h"
#include "spec/spec_zone_fire_giant.h"
#include "spec/spec_zone_hive_of_passion.h"
#include "spec/spec_zone_jot.h"
#include "spec/spec_zone_kenjin_tower.h"
#include "spec/spec_zone_kings_castle.h"
#include "spec/spec_zone_mad_drow.h"
#include "spec/spec_zone_prisoner.h"
#include "spec/spec_zone_quicksand.h"
#include "spec/spec_zone_shadow_dragon.h"
#include "spec/spec_zone_shobalar.h"
#include "spec/spec_zone_ttf.h"
#include "vessels/vessels_moving_rooms.h"

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c
 ****************************************************************************/
void spec_assign_table_boot_validate(void);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
const char *get_spec_func_name(SPECIAL_DECL(*func));
int get_spec_func_count(void);
const char *get_spec_func_name_by_index(int idx);
SPECIAL_DECL(*get_spec_func_by_index(int idx));
SPECIAL_DECL(*find_spec_func_by_name(const char *name));

/*****************************************************************************
 * Compatibility functions still implemented by special-procedure modules
 ****************************************************************************/
void weapons_spells(const char *to_ch, const char *to_vict, const char *to_room,
                    struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spl);

/****************************************************************************/

/* Special functions */
/** Procedures exposed through world persistence or OLC also need registry metadata. **/

/* a-c */
SPECIAL_DECL(acidstaff);
SPECIAL_DECL(acidsword);
SPECIAL_DECL(air_sphere);
SPECIAL_DECL(alandor_ferry);
SPECIAL_DECL(angel_leggings);
SPECIAL_DECL(dragon_robes);
SPECIAL_DECL(bandit_guard);
SPECIAL_DECL(battlemaze_guard);
SPECIAL_DECL(beltush);
SPECIAL_DECL(bloodaxe);
SPECIAL_DECL(bolthammer);
SPECIAL_DECL(stability_boots);
SPECIAL_DECL(chan);
SPECIAL_DECL(ches);
SPECIAL_DECL(chionthar_ferry);
SPECIAL_DECL(clang_bracer);
SPECIAL_DECL(courage);
SPECIAL_DECL(crafting_kit);
SPECIAL_DECL(crafting_quest);
SPECIAL_DECL(cryogenicist);

/* d-f */
SPECIAL_DECL(disruption_mace);
SPECIAL_DECL(dorfaxe);
SPECIAL_DECL(dragonbone_hammer);
SPECIAL_DECL(drow_scimitar);
SPECIAL_DECL(duergar_guard);
SPECIAL_DECL(etherealness);
SPECIAL_DECL(flaming_scimitar);
SPECIAL_DECL(flamingwhip);
SPECIAL_DECL(floating_teleport);
SPECIAL_DECL(fog_dagger);
SPECIAL_DECL(forest_idol);
SPECIAL_DECL(fp_invoker);
SPECIAL_DECL(frosty_scimitar);
SPECIAL_DECL(fzoul);

/* g-i */
SPECIAL_DECL(speed_gaunts);
SPECIAL_DECL(gen_board);
SPECIAL_DECL(greatsword);
SPECIAL_DECL(gromph);
SPECIAL_DECL(guild_golem);
SPECIAL_DECL(halberd);
SPECIAL_DECL(harpell);
SPECIAL_DECL(haste_bracers);
SPECIAL_DECL(hellfire);
SPECIAL_DECL(helmblade);
SPECIAL_DECL(illithid_gguard);
SPECIAL_DECL(imix);

/* j-l */
SPECIAL_DECL(kt_shadowmaker);

/* m-o */
SPECIAL_DECL(magi_staff);
SPECIAL_DECL(magma);
SPECIAL_DECL(malevolence);
SPECIAL_DECL(md_carpet);
SPECIAL_DECL(menzo_chokers);
SPECIAL_DECL(mereshaman);
SPECIAL_DECL(mithril_rapier);
SPECIAL_DECL(monk_glove);
SPECIAL_DECL(monk_glove_cold);
SPECIAL_DECL(ancient_moonblade);
SPECIAL_DECL(naga);
SPECIAL_DECL(naga_golem);
SPECIAL_DECL(neverwinter_button_control);
SPECIAL_DECL(neverwinter_valve_control);
SPECIAL_DECL(nutty_bracer);
SPECIAL_DECL(olhydra);

/* p-r */
SPECIAL_DECL(player_owned_shops);
SPECIAL_DECL(postmaster);
SPECIAL_DECL(prismorb);
SPECIAL_DECL(purity);
SPECIAL_DECL(questmaster);
SPECIAL_DECL(receptionist);
SPECIAL_DECL(rughnark);

/* s-u */
SPECIAL_DECL(sarn);
SPECIAL_DECL(shar_heart);
SPECIAL_DECL(shar_statue);
SPECIAL_DECL(rune_scimitar);
SPECIAL_DECL(shop_keeper);
SPECIAL_DECL(secomber_guard);
SPECIAL_DECL(skullsmasher);
SPECIAL_DECL(snakewhip);
SPECIAL_DECL(sparksword);
SPECIAL_DECL(spiderdagger);
SPECIAL_DECL(spikeshield);
SPECIAL_DECL(star_circlet);
SPECIAL_DECL(storage_chest);
SPECIAL_DECL(celestial_sword);
SPECIAL_DECL(tormblade);
SPECIAL_DECL(trade_bandit);
SPECIAL_DECL(trade_master);
SPECIAL_DECL(trade_object);
SPECIAL_DECL(treantshield);
SPECIAL_DECL(tyrantseye);

/* v-z */
SPECIAL_DECL(vengeance);
SPECIAL_DECL(viperdagger);
SPECIAL_DECL(wallach);
SPECIAL_DECL(warbow);
SPECIAL_DECL(whisperwind);
SPECIAL_DECL(willowisp);
SPECIAL_DECL(witherdirk);
SPECIAL_DECL(xvim_artifact);
SPECIAL_DECL(xvim_normal);
SPECIAL_DECL(yan);

/* Vessel/Ship Special Procedures */
SPECIAL_DECL(greyhawk_ship_object);
SPECIAL_DECL(greyhawk_ship_commands);

/** Procedures exposed through world persistence or OLC also need registry metadata. **/

#endif /* _SPEC_PROCS_H_ */
