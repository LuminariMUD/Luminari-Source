 /**
* @file actions.h
* Header file for the Luminari Action System, based roughly on D20 actions.
*/
#ifndef _ACTIONS_H_
#define _ACTIONS_H_

/* Defines for attack actions */
#define AA_TRIP              0
#define AA_CHARGE            1
#define AA_SMITE_EVIL        2
#define AA_STUNNINGFIST      3
#define AA_HEADBUTT          4
#define AA_KICK              5
#define AA_SHIELDPUNCH       6
#define AA_QUIVERINGPALM     7
#define AA_SUPRISE_ACCURACY  8
#define AA_POWERFUL_BLOW     9
#define AA_COME_AND_GET_ME   10
#define AA_DISARM            11
#define AA_SMITE_GOOD        12
#define AA_SMITE_DESTRUCTION 13
#define AA_SEEKER_ARROW      14
#define AA_DEATH_ARROW       15
#define AA_ARROW_SWARM       16
#define AA_FAERIE_FIRE       17
#define AA_FEINT             18
/**/
#define NUM_ATTACK_ACTIONS   19
/**************************/

#define USE_STANDARD_ACTION(ch)   start_action_cooldown(ch, atSTANDARD, 6 RL_SEC)
#define USE_MOVE_ACTION(ch)       (is_action_available(ch, atMOVE, FALSE) ? start_action_cooldown(ch, atMOVE, 6 RL_SEC) : start_action_cooldown(ch, atSTANDARD, 6 RL_SEC))
#define USE_SWIFT_ACTION(c) start_action_cooldown(ch, atSWIFT, 6 RL_SEC)
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

// Send action data to MSDP.
void update_msdp_actions(struct char_data * ch); 
#endif /* _ACTIONS_H_ */
