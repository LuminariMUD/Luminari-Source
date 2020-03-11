/**
* @file fight.h
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
#define MODE_ARMOR_CLASS_NORMAL                   0
#define MODE_ARMOR_CLASS_COMBAT_MANEUVER_DEFENSE  1
#define MODE_ARMOR_CLASS_PENALTIES                2
#define MODE_ARMOR_CLASS_DISPLAY                  3

#define SKILL_MESSAGE_MISS_FAIL        0
#define SKILL_MESSAGE_MISS_GENERIC     1
#define SKILL_MESSAGE_MISS_SHIELDBLOCK 2
#define SKILL_MESSAGE_MISS_PARRY       3
#define SKILL_MESSAGE_MISS_GLANCE      4
#define SKILL_MESSAGE_DEATH_BLOW       5
#define SKILL_MESSAGE_GENERIC_HIT      6

/* Attacktypes with grammar */
struct attack_hit_type {
   const char *singular;
   const char *plural;
};

/* Functions available in fight.c */
bool is_flanked(struct char_data *attacker, struct char_data *ch);
bool has_dex_bonus_to_ac(struct char_data *attacker, struct char_data *ch);
int damage_shield_check(struct char_data *ch, struct char_data *victim,
                        int attack_type, int dam);
void idle_weapon_spells(struct char_data *ch);
int compute_damtype_reduction(struct char_data *ch, int dam_type);
int compute_energy_absorb(struct char_data *ch, int dam_type);
void perform_flee(struct char_data *ch);
void appear(struct char_data *ch, bool forced);
void check_killer(struct char_data *ch, struct char_data *vict);
int perform_attacks(struct char_data *ch, int mode, int phase);
int combat_maneuver_check(struct char_data *ch, struct char_data *vict,
        int combat_maneuver_type, int attacker_bonus);
int compute_armor_class(struct char_data *attacker, struct char_data *ch, int is_touch, int mode);
int compute_damage_reduction(struct char_data *ch, int dam_type);
int compute_concealment(struct char_data *ch);
int compute_damage_bonus(struct char_data *ch, struct char_data *victim,
	struct obj_data *wielded, int attktype, int mod, int mode, int attack_type);
int compute_cmb(struct char_data *ch, int combat_maneuver_type);
int compute_cmd(struct char_data *vict, int combat_maneuver_type);
int damage(struct char_data *ch, struct char_data *victim,
	int dam, int attacktype, int dam_type, int dualwield);
void death_cry(struct char_data *ch);
void die(struct char_data * ch, struct char_data * killer);
void free_messages(void);
int dam_killed_vict(struct char_data *ch, struct char_data *victim);
void update_pos(struct char_data *victim);
int attack_roll(struct char_data *ch, struct char_data *victim, int attack_type, int is_touch, int attack_number);
int attack_of_opportunity(struct char_data *ch, struct char_data *victim, int penalty);
void attacks_of_opportunity(struct char_data *victim, int penalty);
int compute_attack_bonus(struct char_data *ch, struct char_data *victim, int attack_type);
int hit(struct char_data *ch, struct char_data *victim,
	int type, int dam_type, int penalty, int dualwield);
void load_messages(void);
void perform_violence(struct char_data *ch, int phase);
void raw_kill(struct char_data * ch, struct char_data * killer);
bool set_fighting(struct char_data *ch, struct char_data *victim);
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
          int attacktype, int dualwield);
void  stop_fighting(struct char_data *ch);
bool is_tanking(struct char_data *ch);
void compute_barehand_dam_dice(struct char_data *ch, int *diceOne, int *diceTwo);
int compute_hit_damage(struct char_data *ch, struct char_data *victim,
        int w_type, int diceroll, int mode, bool is_critical, int attack_type);


/* Global variables */
#ifndef __FIGHT_C__
extern struct attack_hit_type attack_hit_text[];
extern struct char_data *combat_list;
#endif /* __FIGHT_C__ */

#endif /* _FIGHT_H_*/
