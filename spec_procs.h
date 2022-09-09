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

int spell_sort_info[TOP_SKILL_DEFINE];
int sorted_spells[TOP_SKILL_DEFINE];
int sorted_skills[TOP_SKILL_DEFINE];

extern int prisoner_heads;

/*****************************************************************************
 * Begin Functions and defines for zone_procs.c
 ****************************************************************************/
void assign_kings_castle(void);
int do_npc_rescue(struct char_data *ch, struct char_data *friend);
void prisoner_on_death(struct char_data *ch);

/*****************************************************************************
 * Begin Functions and defines for spec_assign.c
 ****************************************************************************/
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
const char *get_spec_func_name(SPECIAL_DECL(*func));

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
int compute_ability_full(struct char_data *ch, int abilityNum, bool recursive);
void weapons_spells(const char *to_ch, const char *to_vict, const char *to_room,
                    struct char_data *ch, struct char_data *vict,
                    struct obj_data *obj, int spl);

/****************************************************************************/

/* Special functions */
/** !!MAKE SURE TO ADD TO: spec_func_list!!!  **/

/* a-c */
SPECIAL_DECL(abyss_randomizer);
SPECIAL_DECL(abyssal_vortex);
SPECIAL_DECL(acidstaff);
SPECIAL_DECL(acidsword);
SPECIAL_DECL(agrachdyrr);
SPECIAL_DECL(air_sphere);
SPECIAL_DECL(alandor_ferry);
SPECIAL_DECL(angel_leggings);
SPECIAL_DECL(dragon_robes);
SPECIAL_DECL(bandit_guard);
SPECIAL_DECL(bank);
SPECIAL_DECL(banshee);
SPECIAL_DECL(battlemaze_guard);
SPECIAL_DECL(beltush);
SPECIAL_DECL(bloodaxe);
SPECIAL_DECL(bought_pet);
SPECIAL_DECL(bolthammer);
SPECIAL_DECL(bonedancer);
SPECIAL_DECL(stability_boots);
SPECIAL_DECL(cf_alathar);
SPECIAL_DECL(cf_trainingmaster);
SPECIAL_DECL(chan);
SPECIAL_DECL(ches);
SPECIAL_DECL(chionthar_ferry);
SPECIAL_DECL(cityguard);
SPECIAL_DECL(clang_bracer);
SPECIAL_DECL(courage);
SPECIAL_DECL(crafting_kit);
SPECIAL_DECL(crafting_quest);
SPECIAL_DECL(cryogenicist);
SPECIAL_DECL(cube_slider);

/* d-f */
SPECIAL_DECL(disruption_mace);
SPECIAL_DECL(dog);
SPECIAL_DECL(dorfaxe);
SPECIAL_DECL(dracolich);
SPECIAL_DECL(dragonbone_hammer);
SPECIAL_DECL(drow_scimitar);
SPECIAL_DECL(duergar_guard);
SPECIAL_DECL(dump);
SPECIAL_DECL(ethereal_pet);
SPECIAL_DECL(etherealness);
SPECIAL_DECL(feybranche);
SPECIAL_DECL(fake_twilight);
SPECIAL_DECL(fido);
SPECIAL_DECL(flaming_scimitar);
SPECIAL_DECL(flamingwhip);
SPECIAL_DECL(floating_teleport);
SPECIAL_DECL(fog_dagger);
SPECIAL_DECL(forest_idol);
SPECIAL_DECL(fp_invoker);
SPECIAL_DECL(frostbite);
SPECIAL_DECL(frosty_scimitar);
SPECIAL_DECL(fzoul);

/* g-i */
SPECIAL_DECL(gatehouse_guard);
SPECIAL_DECL(speed_gaunts);
SPECIAL_DECL(gen_board);
SPECIAL_DECL(giantslayer);
SPECIAL_DECL(greatsword);
SPECIAL_DECL(gromph);
SPECIAL_DECL(guild);
SPECIAL_DECL(guild_golem);
SPECIAL_DECL(guild_guard);
SPECIAL_DECL(halberd);
SPECIAL_DECL(harpell);
SPECIAL_DECL(haste_bracers);
SPECIAL_DECL(hellfire);
SPECIAL_DECL(helmblade);
SPECIAL_DECL(hive_death);
SPECIAL_DECL(hound);
SPECIAL_DECL(illithid_gguard);
SPECIAL_DECL(imix);

/* j-l */
SPECIAL_DECL(janitor);
SPECIAL_DECL(jot_invasion_loader);
SPECIAL_DECL(kt_kenjin);
SPECIAL_DECL(kt_shadowmaker);
SPECIAL_DECL(kt_twister);
SPECIAL_DECL(lichdrain);

/* m-o */
SPECIAL_DECL(magi_staff);
SPECIAL_DECL(magma);
SPECIAL_DECL(malevolence);
SPECIAL_DECL(mayor);
SPECIAL_DECL(md_carpet);
SPECIAL_DECL(menzo_chokers);
SPECIAL_DECL(mercenary);
SPECIAL_DECL(mereshaman);
SPECIAL_DECL(mistweave);
SPECIAL_DECL(mithril_rapier);
SPECIAL_DECL(monk_glove);
SPECIAL_DECL(monk_glove_cold);
SPECIAL_DECL(naga);
SPECIAL_DECL(naga_golem);
SPECIAL_DECL(neverwinter_button_control);
SPECIAL_DECL(neverwinter_valve_control);
SPECIAL_DECL(nutty_bracer);
SPECIAL_DECL(ogremoch);
SPECIAL_DECL(olhydra);

/* p-r */
SPECIAL_DECL(pet_shops);
SPECIAL_DECL(phantom);
SPECIAL_DECL(planetar);
SPECIAL_DECL(planetar_sword);
SPECIAL_DECL(planewalker);
SPECIAL_DECL(player_owned_shops);
SPECIAL_DECL(postmaster);
SPECIAL_DECL(practice_dummy);
SPECIAL_DECL(prismorb);
SPECIAL_DECL(puff);
SPECIAL_DECL(purity);
SPECIAL_DECL(questmaster);
SPECIAL_DECL(quicksand);
SPECIAL_DECL(receptionist);
SPECIAL_DECL(rughnark);

/* s-u */
SPECIAL_DECL(sarn);
SPECIAL_DECL(shades);
SPECIAL_DECL(shadowdragon);
SPECIAL_DECL(shar_heart);
SPECIAL_DECL(shar_statue);
SPECIAL_DECL(rune_scimitar);
SPECIAL_DECL(shobalar);
SPECIAL_DECL(shop_keeper);
SPECIAL_DECL(secomber_guard);
SPECIAL_DECL(skeleton_zombie);
SPECIAL_DECL(skullsmasher);
SPECIAL_DECL(snake);
SPECIAL_DECL(snakewhip);
SPECIAL_DECL(solid_elemental);
SPECIAL_DECL(sparksword);
SPECIAL_DECL(spiderdagger);
SPECIAL_DECL(spikeshield);
SPECIAL_DECL(star_circlet);
SPECIAL_DECL(storage_chest);
SPECIAL_DECL(thief);
SPECIAL_DECL(thrym);
SPECIAL_DECL(tia_moonblade);
SPECIAL_DECL(tia_rapier);
SPECIAL_DECL(the_prisoner);
SPECIAL_DECL(tormblade);
SPECIAL_DECL(totemanimal);
SPECIAL_DECL(trade_bandit);
SPECIAL_DECL(trade_master);
SPECIAL_DECL(trade_object);
SPECIAL_DECL(treantshield);
SPECIAL_DECL(ttf_abomination);
SPECIAL_DECL(ttf_monstrosity);
SPECIAL_DECL(ttf_patrol);
SPECIAL_DECL(ttf_rotbringer);
SPECIAL_DECL(twilight);
SPECIAL_DECL(tyrantseye);

/* v-z */
SPECIAL_DECL(valkyrie_sword);
SPECIAL_DECL(vampire);
SPECIAL_DECL(vaprak_claws);
SPECIAL_DECL(vengeance);
SPECIAL_DECL(viperdagger);
SPECIAL_DECL(wall);
SPECIAL_DECL(wallach);
SPECIAL_DECL(warbow);
SPECIAL_DECL(whisperwind);
SPECIAL_DECL(willowisp);
SPECIAL_DECL(witherdirk);
SPECIAL_DECL(wizard);
SPECIAL_DECL(wizard_library);
SPECIAL_DECL(wraith);
SPECIAL_DECL(wraith_elemental);
SPECIAL_DECL(xvim_artifact);
SPECIAL_DECL(xvim_normal);
SPECIAL_DECL(yan);
SPECIAL_DECL(ymir);
SPECIAL_DECL(ymir_cloak);

/** !!MAKE SURE TO ADD TO: spec_func_list!!!  **/

#endif /* _SPEC_PROCS_H_ */
