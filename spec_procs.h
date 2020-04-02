/**
 * @file spec_procs.h                                   LuminariMUD
 * Header file for special procedure modules. This file groups a lot of the
 * legacy special procedures found in spec_procs.c and zone_procs.c.
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

#include "spells.h"

int spell_sort_info[MAX_SKILLS + 1];
int sorted_spells[MAX_SKILLS + 1];
int sorted_skills[MAX_SKILLS + 1];

/*****************************************************************************
 * Begin Functions and defines for zone_procs.c
 ****************************************************************************/
void assign_kings_castle(void);
int do_npc_rescue(struct char_data *ch, struct char_data *friend);

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c
 ****************************************************************************/
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
const char *get_spec_func_name(SPECIAL(*func));

/*****************************************************************************
 * Begin Functions and defines for spec_procs.c
 ****************************************************************************/
#define ABILITY_TYPE_ALL 0
#define ABILITY_TYPE_GENERAL 1
#define ABILITY_TYPE_CRAFT 2
#define ABILITY_TYPE_KNOWLEDGE 3
/* Utility functions */
void sort_spells(void);
void list_crafting_skills(struct char_data *ch);
void list_skills(struct char_data *ch);
void list_spells(struct char_data *ch, int mode, int class, int circle);
void list_abilities(struct char_data *ch, int ability_type);
bool is_wearing(struct char_data *ch, obj_vnum vnum);
int compute_ability(struct char_data *ch, int abilityNum);
void weapons_spells(char *to_ch, char *to_vict, char *to_room,
                    struct char_data *ch, struct char_data *vict,
                    struct obj_data *obj, int spl);

/****************************************************************************/

/* Special functions */
/** !!MAKE SURE TO ADD TO: spec_func_list!!!  **/

/* a-c */
SPECIAL(abyss_randomizer);
SPECIAL(abyssal_vortex);
SPECIAL(acidstaff);
SPECIAL(acidsword);
SPECIAL(agrachdyrr);
SPECIAL(air_sphere);
SPECIAL(alandor_ferry);
SPECIAL(angel_leggings);
SPECIAL(bandit_guard);
SPECIAL(bank);
SPECIAL(banshee);
SPECIAL(battlemaze_guard);
SPECIAL(beltush);
SPECIAL(bloodaxe);
SPECIAL(bought_pet);
SPECIAL(bolthammer);
SPECIAL(bonedancer);
SPECIAL(cf_alathar);
SPECIAL(cf_trainingmaster);
SPECIAL(chan);
SPECIAL(ches);
SPECIAL(chionthar_ferry);
SPECIAL(cityguard);
SPECIAL(clang_bracer);
SPECIAL(courage);
SPECIAL(crafting_kit);
SPECIAL(crafting_quest);
SPECIAL(cryogenicist);
SPECIAL(cube_slider);

/* d-f */
SPECIAL(disruption_mace);
SPECIAL(dog);
SPECIAL(dorfaxe);
SPECIAL(dracolich);
SPECIAL(dragonbone_hammer);
SPECIAL(drow_scimitar);
SPECIAL(duergar_guard);
SPECIAL(dump);
SPECIAL(ethereal_pet);
SPECIAL(etherealness);
SPECIAL(feybranche);
SPECIAL(fake_twilight);
SPECIAL(fido);
SPECIAL(flaming_scimitar);
SPECIAL(flamingwhip);
SPECIAL(floating_teleport);
SPECIAL(fog_dagger);
SPECIAL(forest_idol);
SPECIAL(fp_invoker);
SPECIAL(frostbite);
SPECIAL(frosty_scimitar);
SPECIAL(fzoul);

/* g-i */
SPECIAL(gatehouse_guard);
SPECIAL(gen_board);
SPECIAL(giantslayer);
SPECIAL(greatsword);
SPECIAL(gromph);
SPECIAL(guild);
SPECIAL(guild_golem);
SPECIAL(guild_guard);
SPECIAL(halberd);
SPECIAL(harpell);
SPECIAL(haste_bracers);
SPECIAL(hellfire);
SPECIAL(helmblade);
SPECIAL(hive_death);
SPECIAL(hound);
SPECIAL(illithid_gguard);
SPECIAL(imix);

/* j-l */
SPECIAL(janitor);
SPECIAL(jot_invasion_loader);
SPECIAL(kt_kenjin);
SPECIAL(kt_shadowmaker);
SPECIAL(kt_twister);
SPECIAL(lichdrain);

/* m-o */
SPECIAL(magma);
SPECIAL(malevolence);
SPECIAL(mayor);
SPECIAL(md_carpet);
SPECIAL(menzo_chokers);
SPECIAL(mercenary);
SPECIAL(mereshaman);
SPECIAL(mistweave);
SPECIAL(mithril_rapier);
SPECIAL(monk_glove);
SPECIAL(monk_glove_cold);
SPECIAL(naga);
SPECIAL(naga_golem);
SPECIAL(neverwinter_button_control);
SPECIAL(neverwinter_valve_control);
SPECIAL(nutty_bracer);
SPECIAL(ogremoch);
SPECIAL(olhydra);

/* p-r */
SPECIAL(pet_shops);
SPECIAL(phantom);
SPECIAL(planetar);
SPECIAL(planetar_sword);
SPECIAL(planewalker);
SPECIAL(player_owned_shops);
SPECIAL(postmaster);
SPECIAL(practice_dummy);
SPECIAL(prismorb);
SPECIAL(puff);
SPECIAL(purity);
SPECIAL(questmaster);
SPECIAL(quicksand);
SPECIAL(receptionist);
SPECIAL(rughnark);

/* s-u */
SPECIAL(sarn);
SPECIAL(shades);
SPECIAL(shadowdragon);
SPECIAL(shar_heart);
SPECIAL(shar_statue);
SPECIAL(shobalar);
SPECIAL(shop_keeper);
SPECIAL(secomber_guard);
SPECIAL(skeleton_zombie);
SPECIAL(skullsmasher);
SPECIAL(snake);
SPECIAL(snakewhip);
SPECIAL(solid_elemental);
SPECIAL(sparksword);
SPECIAL(spiderdagger);
SPECIAL(spikeshield);
SPECIAL(storage_chest);
SPECIAL(thief);
SPECIAL(thrym);
SPECIAL(tia_moonblade);
SPECIAL(tia_rapier);
SPECIAL(tiamat);
SPECIAL(tormblade);
SPECIAL(totemanimal);
SPECIAL(trade_bandit);
SPECIAL(trade_master);
SPECIAL(trade_object);
SPECIAL(treantshield);
SPECIAL(ttf_abomination);
SPECIAL(ttf_monstrosity);
SPECIAL(ttf_patrol);
SPECIAL(ttf_rotbringer);
SPECIAL(twilight);
SPECIAL(tyrantseye);

/* v-z */
SPECIAL(valkyrie_sword);
SPECIAL(vampire);
SPECIAL(vaprak_claws);
SPECIAL(vengeance);
SPECIAL(viperdagger);
SPECIAL(wall);
SPECIAL(wallach);
SPECIAL(warbow);
SPECIAL(whisperwind);
SPECIAL(willowisp);
SPECIAL(witherdirk);
SPECIAL(wizard);
SPECIAL(wizard_library);
SPECIAL(wraith);
SPECIAL(wraith_elemental);
SPECIAL(xvim_artifact);
SPECIAL(xvim_normal);
SPECIAL(yan);
SPECIAL(ymir);
SPECIAL(ymir_cloak);

/** !!MAKE SURE TO ADD TO: spec_func_list!!!  **/

#endif /* _SPEC_PROCS_H_ */
