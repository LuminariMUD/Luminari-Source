/*
 * Premade build code for LuminariMUD by Gicker aka Steve Squires
 */

void give_premade_skill(struct char_data *ch, bool verbose, int skill, int amount);
void increase_skills(struct char_data *ch, int chclass, bool verbose, int level);
void give_premade_feat(struct char_data *ch, bool verbose, int feat, int subfeat);
void set_premade_stats(struct char_data *ch, int chclass, int level);
void levelup_warrior(struct char_data *ch, int level, bool verbose);
void levelup_rogue(struct char_data *ch, int level, bool verbose);
void setup_premade_levelup(struct char_data *ch, int chclass);
void advance_premade_build(struct char_data *ch);

#define GET_PREMADE_BUILD_CLASS(ch) 	((ch)->player_specials->saved.premade_build)
