

#ifndef LUMINARI_ROLEPLAY_H
#define LUMINARI_ROLEPLAY_H

struct char_data;
struct descriptor_data;

/*
 * Stable identities for the seven free-form role-play fields.  Both Telnet
 * editor setup and structured onboarding resolve through this table so a web
 * field can never be committed into a different character slot.
 */
enum roleplay_text_field
{
  ROLEPLAY_TEXT_FIELD_INVALID = -1,
  ROLEPLAY_TEXT_FIELD_LONG_DESCRIPTION = 0,
  ROLEPLAY_TEXT_FIELD_BACKGROUND_STORY,
  ROLEPLAY_TEXT_FIELD_GOALS,
  ROLEPLAY_TEXT_FIELD_PERSONALITY,
  ROLEPLAY_TEXT_FIELD_IDEALS,
  ROLEPLAY_TEXT_FIELD_BONDS,
  ROLEPLAY_TEXT_FIELD_FLAWS,
  ROLEPLAY_TEXT_FIELD_COUNT
};

enum roleplay_text_commit_result
{
  ROLEPLAY_TEXT_COMMIT_OK = 0,
  ROLEPLAY_TEXT_COMMIT_INVALID_FIELD,
  ROLEPLAY_TEXT_COMMIT_TOO_LARGE,
  ROLEPLAY_TEXT_COMMIT_INVALID_CONTENT,
  ROLEPLAY_TEXT_COMMIT_NO_MEMORY,
  ROLEPLAY_TEXT_COMMIT_SAVE_FAILED
};

enum roleplay_commit_result
{
  ROLEPLAY_COMMIT_OK = 0,
  ROLEPLAY_COMMIT_INVALID_SELECTION,
  ROLEPLAY_COMMIT_LOCKED,
  ROLEPLAY_COMMIT_SAVE_FAILED,
  ROLEPLAY_COMMIT_INDEX_SAVE_FAILED
};

enum roleplay_text_field roleplay_text_field_from_state(int state);
enum roleplay_text_field roleplay_text_field_from_id(const char *field_id);
const char *roleplay_text_field_id(enum roleplay_text_field field);
size_t roleplay_text_field_max_bytes(enum roleplay_text_field field);
char **roleplay_text_field_slot(struct char_data *ch, enum roleplay_text_field field);
const char *roleplay_text_field_value(const struct char_data *ch, enum roleplay_text_field field);

/*
 * Allocate the same normalized representation accepted by the checked commit
 * path without mutating a character. The caller owns `*normalized` and must
 * wipe it before freeing it. Empty content succeeds with a NULL result.
 */
enum roleplay_text_commit_result
roleplay_text_normalize_copy(enum roleplay_text_field field, const unsigned char *content,
                             size_t content_bytes, char **normalized, size_t *normalized_bytes);

/*
 * Atomically normalize, apply, and durably save one field.  On every failure
 * the original character-owned pointer is restored before this returns.
 */
enum roleplay_text_commit_result roleplay_text_commit_checked(struct char_data *ch,
                                                              enum roleplay_text_field field,
                                                              const unsigned char *content,
                                                              size_t content_bytes);

/* Descriptor-local candidates and checked commits shared by Telnet and web. */
void roleplay_pending_clear(struct descriptor_data *d);
void roleplay_pending_clear_examples(struct descriptor_data *d);
enum roleplay_commit_result roleplay_commit_short_description(struct descriptor_data *d);
enum roleplay_commit_result roleplay_commit_background(struct descriptor_data *d);
enum roleplay_commit_result roleplay_commit_age(struct descriptor_data *d, int age);
enum roleplay_commit_result roleplay_commit_region(struct descriptor_data *d);
enum roleplay_commit_result roleplay_commit_faction(struct descriptor_data *d, int selection);
enum roleplay_commit_result roleplay_commit_hometown(struct descriptor_data *d, int hometown);
enum roleplay_commit_result roleplay_commit_deity(struct descriptor_data *d);

/* Stable protocol identities and media keys keyed by persistent source IDs. */
const char *roleplay_age_stable_id(int age);
const char *roleplay_age_media_key(int age);
void roleplay_region_stable_id(int region, char *buf, size_t buf_size);
const char *roleplay_region_media_key(int region);
void roleplay_faction_stable_id(int faction_vnum, char *buf, size_t buf_size);
const char *roleplay_faction_media_key(int faction_vnum);
bool roleplay_hometown_is_selectable(int hometown);
void roleplay_hometown_stable_id(int hometown, char *buf, size_t buf_size);
const char *roleplay_hometown_media_key(int hometown);
void roleplay_deity_stable_id(int deity, char *buf, size_t buf_size);
const char *roleplay_deity_media_key(int deity);

#ifdef LUMINARI_CUTEST
typedef bool (*roleplay_text_save_callback)(struct char_data *ch, int mode);
typedef bool (*roleplay_index_save_callback)(void);
void roleplay_text_set_save_callback_for_test(roleplay_text_save_callback callback);
void roleplay_index_set_save_callback_for_test(roleplay_index_save_callback callback);
#endif

void choose_random_roleplay_goal(struct char_data *ch);
void choose_random_roleplay_personality(struct char_data *ch, int background);
void choose_random_roleplay_ideals(struct char_data *ch, int background);
void choose_random_roleplay_bonds(struct char_data *ch, int background);
void choose_random_roleplay_flaws(struct char_data *ch, int background);

void show_character_goal_idea_menu(struct char_data *ch);
void show_character_goal_edit(struct descriptor_data *d);
void show_character_personality_edit(struct descriptor_data *d);
void show_character_ideals_edit(struct descriptor_data *d);
void show_character_bonds_edit(struct descriptor_data *d);
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

#endif /* LUMINARI_ROLEPLAY_H */
