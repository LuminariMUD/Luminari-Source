#ifndef _D20SW_MISSIONS_HPP_
#define _D20SW_MISSIONS_HPP_

// defines
#define MISSION_LVL             0
#define MISSION_REBELS          1
#define MISSION_EMPIRE          2
#define MISSION_HUTTS           3
#define MISSION_FREELANCERS     4
#define MISSION_PLANET          5
#define MISSION_ROOM_VNUM       6
#define MISSION_ZONE_NAME       7

#define MISSION_STANDING	1
#define MISSION_REP		2
#define MISSION_CREDITS		3
#define MISSION_EXP		4

#define MISSION_DIFF_NORMAL		0
#define MISSION_DIFF_TOUGH		1
#define MISSION_DIFF_CHALLENGING	2
#define MISSION_DIFF_ARDUOUS		3
#define MISSION_DIFF_SEVERE		4

#define GET_CURRENT_MISSION(ch)			(ch->player_specials->saved.current_mission)
#define GET_MISSION_CREDITS(ch)			(ch->player_specials->saved.mission_credits)
#define GET_MISSION_STANDING(ch)		(ch->player_specials->saved.mission_standing)
#define GET_MISSION_FACTION(ch)			(ch->player_specials->saved.mission_faction)
#define GET_MISSION_REP(ch)			(ch->player_specials->saved.mission_reputation)
#define GET_MISSION_EXP(ch)			(ch->player_specials->saved.mission_experience)
#define GET_MISSION_DIFFICULTY(ch)		(ch->player_specials->saved.mission_difficulty)
#define GET_MISSION_NPC_NAME_NUM(ch)		(ch->player_specials->saved.mission_rand_name)
#define GET_MISSION_DECLINE(ch)                 (ch->player_specials->saved.mission_decline)
#define GET_MISSION_COMPLETE(ch)                (ch->player_specials->saved.mission_complete)
#define GET_FACTION_STANDING(ch, i)             (ch->player_specials->saved.faction_standing[i])

#define MISSION_MOB_DFLT_VNUM 60000

// variables
extern const char * const mission_details[][8];
extern const char * const mission_targets[5];
extern const char * const mission_difficulty[5];

// functions
int mission_details_to_faction(int faction);
ACMD_DECL(do_missions);
long get_mission_reward(struct char_data *ch, int reward_type);
void clear_mission_mobs(struct char_data *ch);
void create_mission_mobs(struct char_data *ch);
bool are_mission_mobs_loaded(struct char_data *ch);
void apply_mission_rewards(struct char_data *ch);
void clear_mission(struct char_data *ch);
bool is_mission_mob(struct char_data *ch, struct char_data *mob);
void create_mission_on_entry(struct char_data *ch);

#endif
