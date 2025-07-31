

void choose_random_roleplay_goal(struct char_data *ch);
void choose_random_roleplay_personality(struct char_data *ch, int background);
void choose_random_roleplay_ideals(struct char_data *ch, int background);
void choose_random_roleplay_bonds(struct char_data *ch, int background);
void choose_random_roleplay_flaws(struct char_data *ch, int background);

void show_character_goal_idea_menu(struct char_data *ch);
void show_character_goal_edit(struct descriptor_data *d);
void show_character_personality_edit(struct descriptor_data *d);
void show_character_ideals_edit(struct descriptor_data *d);
void show_character_flaws_edit(struct descriptor_data *d);
void show_character_personality_idea_menu(struct char_data *ch);
void show_character_ideals_idea_menu(struct char_data *ch);
void show_character_bonds_idea_menu(struct char_data *ch);
void show_character_flaws_idea_menu(struct char_data *ch);

void display_age_menu(struct descriptor_data *d);
void display_faction_menu(struct descriptor_data *d);
void display_hometown_menu(struct descriptor_data *d);
void display_deity_menu(struct descriptor_data *d);
void display_deity_info(struct descriptor_data *d);
void display_rp_decide_menu(struct descriptor_data *d);

void HandleStateCharacterHometownParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateCharacterAgeParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateCharacterFactionParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateCharacterDeityParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateCharacterDeityConfirmParseMenuChoice(struct descriptor_data *d, char *arg);
void HandleStateCharacterRPDecideParseMenuChoice(struct descriptor_data *d, char *arg);

ACMD_DECL(do_goals);
ACMD_DECL(do_rpsheet);
ACMD_DECL(do_showrpinfo);

#define CHARACTER_AGE_ADULT 0
#define CHARACTER_AGE_YOUNG 1
#define CHARACTER_AGE_MIDDLE_AGED 2
#define CHARACTER_AGE_OLD 3
#define CHARACTER_AGE_VENERABLE 4

#define NUM_CHARACTER_AGES 5