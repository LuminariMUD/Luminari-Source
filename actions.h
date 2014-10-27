/**
* @file actions.h
* Header file for the Luminari Action System, based roughly on D20 actions.
*/
#ifndef _ACTIONS_H_
#define _ACTIONS_H_

/* Defines for attack actions */
#define AA_TRIP            0
#define AA_CHARGE          1
#define AA_SMITE           2
#define AA_STUNNINGFIST    3
#define AA_HEADBUTT        4
#define AA_KICK            5
#define AA_SHIELDPUNCH     6
#define AA_QUIVERINGPALM   7
/**/
#define NUM_ATTACK_ACTIONS 8
/**************************/

#define USE_STANDARD_ACTION(ch)   start_action_cooldown(ch, atSTANDARD, 6 RL_SEC)
#define USE_MOVE_ACTION(ch)       (is_action_available(ch, atMOVE, FALSE) ? start_action_cooldown(ch, atMOVE, 6 RL_SEC) : start_action_cooldown(ch, atSTANDARD, 6 RL_SEC))
#define USE_FULL_ROUND_ACTION(ch) USE_STANDARD_ACTION(ch); USE_MOVE_ACTION(ch)

extern void  (*attack_actions[NUM_ATTACK_ACTIONS])(struct char_data *ch,
                                            char *argument,
                                            int cmd,
                                            int subcmd);
/* Prototype for the event function. */
EVENTFUNC(event_action_recover);

/* - Cooldown utilities --------------- */
bool is_action_available(struct char_data * ch, action_type act_type, bool msg_to_char);
void start_cmd_cooldown(struct char_data *ch, int cmd);
//void start_skill_cooldown(struct char_data *ch, int skill, int weapon);
void start_action_cooldown(struct char_data * ch, action_type act_type, int duration);

#endif /* _ACTIONS_H_ */
