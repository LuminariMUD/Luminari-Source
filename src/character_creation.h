#ifndef CHARACTER_CREATION_H
#define CHARACTER_CREATION_H

struct char_data;
struct descriptor_data;
struct account_data;
struct player_removal_transaction;

enum character_creation_stage
{
  CHARACTER_CREATION_STAGE_NONE = 0,
  CHARACTER_CREATION_STAGE_PREFERENCES,
  CHARACTER_CREATION_STAGE_ROLEPLAY_DECISION,
  CHARACTER_CREATION_STAGE_ROLEPLAY_PROFILE,
  CHARACTER_CREATION_STAGE_MENU_PENDING,
  NUM_CHARACTER_CREATION_STAGES
};

enum character_creation_restart_result
{
  CHARACTER_CREATION_RESTART_OK = 0,
  CHARACTER_CREATION_RESTART_UNAVAILABLE,
  CHARACTER_CREATION_RESTART_ACCOUNT_FAILED,
  CHARACTER_CREATION_RESTART_PLAYER_FAILED,
  CHARACTER_CREATION_RESTART_ROLLBACK_FAILED
};

bool character_creation_stage_is_valid(int stage);
bool character_creation_is_active(const struct char_data *ch);
bool character_creation_set_stage_checked(struct char_data *ch,
                                          enum character_creation_stage stage);
bool character_creation_finish_checked(struct char_data *ch);

bool character_creation_can_back(const struct descriptor_data *d);
bool character_creation_back(struct descriptor_data *d);
bool character_creation_can_restart(const struct descriptor_data *d);
enum character_creation_restart_result character_creation_restart(struct descriptor_data *d);

/* Route a loaded, incomplete character back to its durable creation stage. */
bool character_creation_resume(struct descriptor_data *d);

#ifdef LUMINARI_CUTEST
typedef bool (*character_creation_save_callback)(struct char_data *ch, int mode);
void character_creation_set_save_callback_for_test(character_creation_save_callback callback);

struct character_creation_restart_test_hooks
{
  int (*lookup_player)(const char *name);
  bool (*begin_account_removal)(struct char_data *ch, struct account_data *account);
  void (*rollback_account_removal)(void);
  struct player_removal_transaction *(*prepare_player_removal)(int player_position);
  bool (*commit_account_removal)(struct account_data *account);
  bool (*rollback_player_removal)(struct player_removal_transaction *transaction);
  bool (*commit_player_removal)(struct player_removal_transaction *transaction);
};

void character_creation_set_restart_hooks_for_test(
    const struct character_creation_restart_test_hooks *hooks);
#endif

#endif /* CHARACTER_CREATION_H */
