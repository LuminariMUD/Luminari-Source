/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Crafts System
/  Created By: Created by Vatiken, (Joseph Arnusch)
\              installed by Ornir
/
\  Created: June 21st, 2012
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "core/structs.h"
#include "core/utils.h"

struct craft_data
{
  int craft_id;
  char *craft_name;
  int craft_flags;
  obj_vnum craft_object_vnum;
  int craft_timer;

  char *craft_msg_self;
  char *craft_msg_room;

  /* The craft or harvest ability (34 to 51) the recipe uses, -1 for none, or
   * CRAFT_SKILL_UNSUPPORTED for a legacy record whose skill has no ability; such a record
   * stays editable but cannot execute, and craft_skill_legacy keeps its raw id for write-back. */
  int craft_skill;
  int craft_skill_level; /* rank required (legacy levels convert on load, rounded up) */
  int craft_skill_legacy;

  struct list_data *requirements;
};

#define CRAFT_SKILL_UNSUPPORTED (-2)

struct requirement_data
{
  obj_vnum req_vnum;
  int req_amount;
  int req_flags;
};

#define CRAFT_ID(craft) (craft->craft_id)
#define CRAFT_NAME(craft) (craft->craft_name ? craft->craft_name : "<No Name>")
#define CRAFT_FLAGS(craft) (craft->craft_flags)
#define CRAFT_OBJVNUM(craft) (craft->craft_object_vnum)
#define CRAFT_TIMER(craft) (craft->craft_timer)
#define CRAFT_SKILL(craft) (craft->craft_skill)
#define CRAFT_SKILL_LEVEL(craft) (craft->craft_skill_level)
#define CRAFT_SKILL_LEGACY(craft) (craft->craft_skill_legacy)

#define CRAFT_MSG_SELF(craft) (craft->craft_msg_self)
#define CRAFT_MSG_ROOM(craft) (craft->craft_msg_room)

#define CRAFT_RECIPE (1 << 0)
#define NUM_CRAFT_FLAGS 1

#define REQ_FLAG_IN_ROOM (1 << 0)
#define REQ_FLAG_NO_REMOVE (1 << 1)
#define REQ_SAVE_ON_FAIL (1 << 2)

#define NUM_REQ_FLAGS 3

/* CRAFTEDIT STATES */
#define CRAFTEDIT_MENU 0
#define CRAFTEDIT_NAME 1
#define CRAFTEDIT_SAVE 2
#define CRAFTEDIT_TIMER 3
#define CRAFTEDIT_VNUM 4
#define CRAFTEDIT_REQUIREMENTS 5
#define CRAFTEDIT_REQ_NEW_VNUM 6
#define CRAFTEDIT_REQ_NEW_AMOUNT 7
#define CRAFTEDIT_REQ_DELETE 8
#define CRAFTEDIT_REQ_FLAGS 9
#define CRAFTEDIT_CRAFT_FLAGS 10
#define CRAFTEDIT_SKILL 11
#define CRAFTEDIT_SKILL_LEVEL 12
#define CRAFTEDIT_DELETE 13
#define CRAFTEDIT_MSG_SELF 14
#define CRAFTEDIT_MSG_ROOM 15

struct craft_data *create_craft(void);
void free_craft(struct craft_data *craft);
bool craft_skill_id_is_valid(int skill);
void load_crafts(void);
void list_all_crafts(struct char_data *ch);
void list_available_crafts(struct char_data *ch);
void show_craft(struct char_data *ch, struct craft_data *craft, int mode);
struct craft_data *get_craft_from_arg(char *arg);
struct craft_data *get_craft_from_id(int id);

ACMD_DECL(do_craft);
ACMD_DECL(do_craft_with_kits);

/* Craftedit */
extern struct list_data *global_craft_list;

#ifdef LUMINARI_CUTEST
void test_load_crafts_from(FILE *fp);
void test_save_crafts_to(FILE *fp);
void test_clear_crafts(void);
int test_missing_craft_requirements(struct char_data *ch, struct craft_data *craft);
void test_remove_components(struct char_data *ch, struct craft_data *craft, bool success);
bool test_character_meets_craft_skill(struct char_data *ch, struct craft_data *craft);
#endif
