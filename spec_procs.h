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
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(wizard);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);
SPECIAL(crafting_kit);
SPECIAL(crafting_quest);
SPECIAL(wall);
SPECIAL(hound);
SPECIAL(abyss_randomizer);
SPECIAL(cf_trainingmaster);
SPECIAL(cf_alathar);
SPECIAL(jot_invasion_loader);
SPECIAL(wizard_library);
SPECIAL(air_sphere);
SPECIAL(chionthar_ferry);
SPECIAL(spikeshield);
SPECIAL(hive_death);
SPECIAL(courage);
SPECIAL(trade_master);
SPECIAL(trade_bandit);
SPECIAL(trade_object);
SPECIAL(forest_idol);
SPECIAL(cube_slider);
SPECIAL(guild_golem);
SPECIAL(battlemaze_guard);
SPECIAL(fzoul);
SPECIAL(bandit_guard);
SPECIAL(shar_statue);
SPECIAL(shar_heart);
SPECIAL(acidsword);
SPECIAL(snakewhip);
SPECIAL(witherdirk);
SPECIAL(cf_alathar);
SPECIAL(cf_trainingmaster);
SPECIAL(spiderdagger);
SPECIAL(menzo_chokers);
SPECIAL(planetar);
SPECIAL(ymir);
SPECIAL(thrym);
SPECIAL(gatehouse_guard);
SPECIAL(jot_invasion_loader);
SPECIAL(abyss_randomizer);
SPECIAL(practice_dummy);
SPECIAL(tia_moonblade);
SPECIAL(drow_scimitar);
SPECIAL(tia_rapier);
SPECIAL(ttf_monstrosity);
SPECIAL(ttf_abomination);
SPECIAL(ttf_rotbringer);
SPECIAL(ttf_patrol);
SPECIAL(abyssal_vortex);
SPECIAL(kt_twister);
SPECIAL(kt_shadowmaker);
SPECIAL(kt_kenjin);
SPECIAL(dog);
SPECIAL(ogremoch);
SPECIAL(md_carpet);
SPECIAL(duergar_guard);
SPECIAL(tormblade);
SPECIAL(air_sphere);
SPECIAL(chan);
SPECIAL(yan);
SPECIAL(halberd);
SPECIAL(rughnark);
SPECIAL(magma);
SPECIAL(ethereal_pet);
SPECIAL(bought_pet);
SPECIAL(storage_chest);
SPECIAL(ches);
SPECIAL(spikeshield);
SPECIAL(clang_bracer);
SPECIAL(secomber_guard);
SPECIAL(shades);
SPECIAL(illithid_gguard);
SPECIAL(hellfire);
SPECIAL(quicksand);
SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(guild);
SPECIAL(fido);
SPECIAL(tiamat);
SPECIAL(dracolich);
SPECIAL(imix);
SPECIAL(practice_dummy);
SPECIAL(olhydra);
SPECIAL(fp_invoker);
SPECIAL(gromph);
SPECIAL(beltush);
SPECIAL(willowisp);
SPECIAL(mereshaman);
SPECIAL(wallach);
SPECIAL(banshee);
SPECIAL(chionthar_ferry);
SPECIAL(alandor_ferry);
SPECIAL(harpell);
SPECIAL(shobalar);
SPECIAL(agrachdyrr);
SPECIAL(feybranche);
SPECIAL(skeleton_zombie);
SPECIAL(wraith);
SPECIAL(vampire);
SPECIAL(bonedancer);
SPECIAL(totemanimal);
SPECIAL(solid_elemental);
SPECIAL(wraith_elemental);
SPECIAL(janitor);
SPECIAL(bank);
SPECIAL(gen_board);
SPECIAL(lichdrain);
SPECIAL(phantom);
SPECIAL(planewalker);
SPECIAL(naga);
SPECIAL(naga_golem);
SPECIAL(mercenary);
SPECIAL(shadowdragon);
SPECIAL(warbow);
SPECIAL(malevolence);
SPECIAL(xvim_artifact);
SPECIAL(xvim_normal);
SPECIAL(helmblade);
SPECIAL(flamingwhip);
SPECIAL(dorfaxe);
SPECIAL(sarn);
SPECIAL(viperdagger);
SPECIAL(acidstaff);
SPECIAL(prismorb);
SPECIAL(whisperwind);
SPECIAL(bloodaxe);
SPECIAL(vengeance);
SPECIAL(skullsmasher);
SPECIAL(bolthammer);
SPECIAL(air_sphere);
SPECIAL(sparksword);
SPECIAL(tyrantseye);
SPECIAL(flaming_scimitar);
SPECIAL(frosty_scimitar);
SPECIAL(mithril_rapier);
SPECIAL(nutty_bracer);
SPECIAL(neverwinter_valve_control);
SPECIAL(neverwinter_button_control);
SPECIAL(floating_teleport);
SPECIAL(purity);
SPECIAL(etherealness);
SPECIAL(greatsword);
SPECIAL(fog_dagger);
SPECIAL(dragonbone_hammer);
SPECIAL(treantshield);
SPECIAL(mistweave);
SPECIAL(frostbite);
SPECIAL(ymir_cloak);
SPECIAL(vaprak_claws);
SPECIAL(twilight);
SPECIAL(fake_twilight);
SPECIAL(giantslayer);
SPECIAL(valkyrie_sword);
SPECIAL(planetar_sword);
SPECIAL(haste_bracers);
SPECIAL(disruption_mace);
SPECIAL(angel_leggings);
SPECIAL(monk_glove);
SPECIAL(monk_glove_cold);
SPECIAL(player_owned_shops);

/** !!MAKE SURE TO ADD TO: spec_func_list!!!  **/

#endif /* _SPEC_PROCS_H_ */
