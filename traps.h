/*
 * File:   traps.h
 * Author: Zusuk
 *
 * Created on 2 נובמבר 2014, 22:39
 */

#ifndef TRAPS_H
#define TRAPS_H

/* TRAPS [ in structs.h now ] */
/* trap types */
/*
#define TRAP_TYPE_LEAVE_ROOM         0
#define TRAP_TYPE_OPEN_DOOR          1
#define TRAP_TYPE_UNLOCK_DOOR        2
#define TRAP_TYPE_OPEN_CONTAINER     3
#define TRAP_TYPE_UNLOCK_CONTAINER   4
#define TRAP_TYPE_GET_OBJECT         5
#define TRAP_TYPE_ENTER_ROOM         6
*/
/**/
/*
#define MAX_TRAP_TYPES               7
*/
/******************************************/
/* trap effects
   if the effect is < 1000, its just suppose to cast a spell */
/*
#define TRAP_EFFECT_FIRST_VALUE         1000
*/
/**/
/*
#define TRAP_EFFECT_WALL_OF_FLAMES      1000
#define TRAP_EFFECT_LIGHTNING_STRIKE    1001
#define TRAP_EFFECT_IMPALING_SPIKE      1002
#define TRAP_EFFECT_DARK_GLYPH          1003
#define TRAP_EFFECT_SPIKE_PIT           1004
#define TRAP_EFFECT_DAMAGE_DART         1005
#define TRAP_EFFECT_POISON_GAS          1006
#define TRAP_EFFECT_DISPEL_MAGIC        1007
#define TRAP_EFFECT_DARK_WARRIOR_AMBUSH 1008
#define TRAP_EFFECT_BOULDER_DROP        1009
#define TRAP_EFFECT_WALL_SMASH          1010
#define TRAP_EFFECT_SPIDER_HORDE        1011
#define TRAP_EFFECT_DAMAGE_GAS          1012
#define TRAP_EFFECT_FREEZING_CONDITIONS 1013
#define TRAP_EFFECT_SKELETAL_HANDS      1014
#define TRAP_EFFECT_SPIDER_WEBS         1015
*/
/**/
/*
#define TOP_TRAP_EFFECTS                1016
#define MAX_TRAP_EFFECTS                (TOP_TRAP_EFFECTS - TRAP_EFFECT_FIRST_VALUE)
*/
/******************************************/
/*end traps*/

/* fuctions defined in traps.c */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir);
void set_off_trap(struct char_data *ch, struct obj_data *trap);
bool is_trap_detected(struct obj_data *trap);
void set_trap_detected(struct obj_data *trap);
int perform_detecttrap(struct char_data *ch, bool silent);

/* ACMD */
ACMD_DECL(do_disabletrap);
ACMD_DECL(do_detecttrap);

/* special defines */
#define TRAP_DARK_WARRIOR_MOBILE 135600
#define TRAP_SPIDER_MOBILE 180437

#endif /* TRAPS_H */
