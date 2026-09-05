/**
 * @file fight.h                                 LuminariMUD
 * Fighting and violence functions and variables.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 *
 */
#ifndef _FIGHT_H_
#define _FIGHT_H_

/* Structures and defines */
#define MODE_ARMOR_CLASS_NORMAL 0
#define MODE_ARMOR_CLASS_COMBAT_MANEUVER_DEFENSE 1
#define MODE_ARMOR_CLASS_PENALTIES 2
#define MODE_ARMOR_CLASS_DISPLAY 3
#define MODE_ARMOR_CLASS_TOUCH 4

#define MODE_NORMAL_HIT 0      // Normal damage calculating in hit()
#define MODE_DISPLAY_PRIMARY 2 // Display damage info primary
#define MODE_DISPLAY_OFFHAND 3 // Display damage info offhand
#define MODE_DISPLAY_RANGED 4  // Display damage info ranged

#define SKILL_MESSAGE_MISS_FAIL 0
#define SKILL_MESSAGE_MISS_GENERIC 1
#define SKILL_MESSAGE_MISS_SHIELDBLOCK 2
#define SKILL_MESSAGE_MISS_PARRY 3
#define SKILL_MESSAGE_MISS_GLANCE 4
#define SKILL_MESSAGE_DEATH_BLOW 5
#define SKILL_MESSAGE_GENERIC_HIT 6

/* Attacktypes with grammar */
struct attack_hit_type
{
  const char *singular;
  const char *plural;
};

/* Commit already-resolved damage and publish its actual HP loss. This does not
 * perform resistance, combat admission, position updates or death resolution.
 * Use INT_MIN for no gameplay floor; callers retain their existing death policy. */
void combat_apply_raw_damage(struct char_data *victim, struct char_data *source, int amount,
                             int damage_type, int minimum_hit);

/* Functions available in fight.c */
void init_condensed_combat_data(struct char_data *ch);
int valid_fight_cond(struct char_data *ch, bool strict);
int apply_damage_reduction(struct char_data *ch, struct char_data *victim, struct obj_data *wielded,
                           int dam, bool display);
bool is_flanked(struct char_data *attacker, struct char_data *ch);
bool has_dex_bonus_to_ac(struct char_data *attacker, struct char_data *ch);
int damage_shield_check(struct char_data *ch, struct char_data *victim, int attack_type, int dam,
                        int dam_type);
void idle_weapon_spells(struct char_data *ch);
int compute_damtype_reduction(struct char_data *ch, int dam_type, struct char_data *attacker,
                              int w_type);
int compute_energy_absorb(struct char_data *ch, int dam_type);
void perform_flee(struct char_data *ch);
void appear(struct char_data *ch, bool forced);
void check_killer(struct char_data *ch, struct char_data *vict);
int perform_attacks(struct char_data *ch, int mode, int phase);
int combat_maneuver_check(struct char_data *ch, struct char_data *vict, int combat_maneuver_type,
                          int attacker_bonus);
int compute_armor_class(struct char_data *attacker, struct char_data *ch, int is_touch, int mode);
int compute_damage_reduction(struct char_data *ch, int dam_type);
int compute_concealment(struct char_data *ch, struct char_data *attacker);
bool ok_damage_handling(int attacktype);
int compute_damage_bonus(struct char_data *ch, struct char_data *victim, struct obj_data *wielded,
                         int attktype, int mod, int mode, int attack_type);
int compute_cmb(struct char_data *ch, int combat_maneuver_type);
int compute_cmd(struct char_data *vict, int combat_maneuver_type);
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int dam_type,
           int attack_type);
void death_cry(struct char_data *ch);
void die(struct char_data *ch, struct char_data *killer);
void free_messages(void);
int dam_killed_vict(struct char_data *ch, struct char_data *victim);
void update_pos(struct char_data *victim);
int attack_roll(struct char_data *ch, struct char_data *victim, int attack_type, int is_touch,
                int attack_number);
int attack_roll_with_critical(struct char_data *ch, struct char_data *victim, int attack_type,
                              int is_touch, int attack_number, int threat_range);
int attack_of_opportunity(struct char_data *ch, struct char_data *victim, int penalty);
void attacks_of_opportunity(struct char_data *victim, int penalty);
int compute_attack_bonus(struct char_data *ch, struct char_data *victim, int attack_type);
int hit(struct char_data *ch, struct char_data *victim, int type, int dam_type, int penalty,
        int attack_type);
bool combat_readied_attack_allowed(struct char_data *ch, struct char_data *victim);
int combat_readied_attack(struct char_data *ch, struct char_data *victim);
void load_messages(void);
void perform_violence(struct char_data *ch, int phase);
bool combat_run_compatibility_phase(struct char_data *ch, unsigned int phase);
bool combat_run_semantic_round(struct char_data *ch, bool was_hit);
void raw_kill(struct char_data *ch, struct char_data *killer);
bool set_fighting(struct char_data *ch, struct char_data *victim);
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype,
                  int attack_type);
void stop_fighting(struct char_data *ch);
bool is_tanking(struct char_data *ch);
void compute_barehand_dam_dice(struct char_data *ch, int *diceOne, int *diceTwo);
bool activate_rol_delayed_hunter(struct char_data *victim, int damage);
int compute_hit_damage(struct char_data *ch, struct char_data *victim, int w_type, int diceroll,
                       int mode, bool is_critical, int attack_type, int dam_type);
struct obj_data *make_a_corpse_4_npcs(struct char_data *ch);
int handle_warding(struct char_data *ch, struct char_data *victim, int dam);
void weapon_poison(struct char_data *ch, struct char_data *victim, struct obj_data *wielded,
                   struct obj_data *missile);
int compute_attack_bonus_full(struct char_data *ch, struct char_data *victim, int attack_type,
                              bool display);
int determine_threat_range(struct char_data *ch, struct obj_data *wielded, struct char_data *victim,
                           int attack_type);
int dual_wielding_penalty(struct char_data *ch, bool offhand);
int is_dual_wielding(struct char_data *ch);
int get_initiative_modifier(struct char_data *ch);
int get_monk_stunning_fist_dc(struct char_data *ch);

#ifdef LUMINARI_CUTEST
int test_award_kill_experience(struct char_data *ch, int exp, int mode);
int test_cap_combat_damage(struct char_data *ch, int dam, int w_type);
int test_damage_handling(struct char_data *ch, struct char_data *victim, int dam, int attacktype,
                         int dam_type);
void test_apply_bard_commanding_cadence(struct char_data *ch, struct char_data *victim,
                                        int can_hit);
int test_apply_bard_frostbite_rider(struct char_data *ch, struct char_data *victim,
                                    int weapon_damage, int can_hit, int attack_type);
int test_handle_successful_artifact_attack(struct char_data *ch, struct char_data *victim,
                                           struct obj_data *wielded, int dam, int is_critical,
                                           int dam_type);
int test_compute_projectile_attack_bonus(struct char_data *ch, struct char_data *victim,
                                         struct obj_data *wielded, struct obj_data *projectile,
                                         int attack_type);
int test_compute_projectile_damage_bonus(struct char_data *ch, struct char_data *victim,
                                         struct obj_data *wielded, struct obj_data *projectile,
                                         int attack_type);
bool test_projectile_attack_context_was_invalidated(struct char_data *ch, struct char_data *victim,
                                                    int attack_type, struct obj_data *projectile);
bool test_can_process_projectile_weapon_abilities(struct char_data *ch, struct obj_data *wielded,
                                                  int attack_type);
void test_finish_thrown_projectile_attack(struct char_data *ch);
void test_apply_bard_warbeat_allies(struct char_data *ch);
void test_reset_bard_warbeat_observations(void);
int test_get_bard_warbeat_opening_attacks(void);
struct obj_data *test_get_wielded(struct char_data *ch, int attack_type);
struct char_data *test_find_divine_sacrifice_defender(struct char_data *victim);
void test_apply_group_sacred_vengeance(struct char_data *victim);
bool test_life_shield_can_reflect(struct char_data *attacker, struct char_data *victim, int damage,
                                  int source);
struct affected_type *test_find_spell_affect(struct char_data *ch, int spell);
bool test_attack_number_runs_in_phase(int attack_number, int phase);
#endif

/* Global variables */
#ifndef __FIGHT_C__
extern struct attack_hit_type attack_hit_text[];
#endif /* __FIGHT_C__ */

#endif /* _FIGHT_H_*/
