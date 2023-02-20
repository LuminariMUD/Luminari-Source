

// defines

#define TEMPLATE_NONE			0
#define TEMPLATE_CHAMPION		1

#define NUM_TEMPLATES			2

#define LEVELINFO_SYNTAX                                                                                                  \
  "@nPlease use one of the following options:\r\n"                                                                        \
  "-- levelinfo\r\n"                                                                                                      \
  "----> This lists all of the levels you've done so far including the level id needed for other levelinfo commands.\r\n" \
  "-- levelinfo (level_id)\r\n"                                                                                           \
  "----> This will display levelup info for the level_id specified.\r\n"                                                  \
  "-- levelinfo search (feat|skill|ability) (keyword(s))\r\n"                                                             \
  "----> This will allow you to search your levels for the feat, skill, or ability score mentioned.\r\n"

// externals
void finalize_study(struct descriptor_data *d);

// internals
void gain_template_level(struct char_data *ch, int t_type, int level);
void set_template(struct char_data *ch, int template_type);
void show_level_history(struct char_data *ch, int level);
long get_level_id_by_level_num(int level_num, char * chname);
void show_levelinfo_for_specific_level(struct char_data *ch, long level_id, char * chname);
void display_levelinfo_feats(char_data *ch, int feat_num, int subfeat);
void display_levelinfo_ability_scores(char_data *ch, int ability_score);
void erase_levelup_info(struct char_data *ch);
void levelinfo_search(struct char_data *ch, int type, char *searchString);

SPECIAL_DECL(select_templates);
ACMD_DECL(do_templates);
ACMD_DECL(do_levelinfo);

// constants
extern const char * const template_types[NUM_TEMPLATES];
extern const char * const template_types_capped[NUM_TEMPLATES];
extern const char * const template_db_names[NUM_TEMPLATES];
extern const char * const levelup_ability_scores[6];

// macros
#define GET_TEMPLATE(ch) (ch->player_specials->saved.template)
