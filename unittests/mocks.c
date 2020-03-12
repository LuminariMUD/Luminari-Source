/* LuminariMUD */

#include "../structs.h"

struct char_data;

struct player_special_data dummy_mob; /* dummy spec area for mobs	*/
void add_history(struct char_data *ch, char *str, int type) {}
void basic_mud_log(const char *x, ...) {}
struct config_data config_info; /* Game configuration list.	 */
struct room_data room = {0};
struct room_data* world = &room;
void speech_mtrigger(struct char_data *actor, char *str) {}
void speech_wtrigger(struct char_data *actor, char *str) {}
void send_to_group(struct char_data *ch, struct group_data *group, const char * msg, ...) {}
long last_webster_teller = -1L;
struct char_data *get_player_vis(struct char_data *ch, char *name, int *number, int inroom) { return NULL; }
struct char_data *character_list = NULL; /* global linked list of chars	*/
struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, int *number, struct obj_data *list) { return NULL; }
void quest_ask(struct char_data *ch, struct char_data *victim, char *keyword) {}
int get_feat_value(struct char_data *ch, int featnum) { return 0; }
