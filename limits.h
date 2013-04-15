/****************************************************************************
 *  Realms of Luminari
 *  File:     limits.h
 *  Authors:  Nashak and Zusuk
 *  Created:  April 15, 2013
 ****************************************************************************/

#ifndef LIMITS_H
#define	LIMITS_H

#ifdef	__cplusplus
extern "C" {
#endif

  
/* limits.c functions */
void pulse_luminari();
bool death_check(struct char_data *ch);
int graf(int grafage, int p0, int p1, int p2, int p3, int p4, int p5, int p6);
void regen_update(struct char_data *ch);
int mana_gain(struct char_data *ch);
int hit_gain(struct char_data *ch);
int move_gain(struct char_data *ch);
void set_title(struct char_data *ch, char *title);
void run_autowiz(void);
int gain_exp(struct char_data *ch, int gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
void check_idling(struct char_data *ch);
void point_update(void);
int increase_gold(struct char_data *ch, int amt);
int decrease_gold(struct char_data *ch, int deduction);
int increase_bank(struct char_data *ch, int amt);
int decrease_bank(struct char_data *ch, int deduction);
void increase_anger(struct char_data *ch, float amount);


#ifdef	__cplusplus
}
#endif

#endif	/* LIMITS_H */


