#ifndef PLAYER_RENAME_H
#define PLAYER_RENAME_H

#include <stddef.h>

struct char_data;

enum player_rename_status
{
  PLAYER_RENAME_OK = 0,
  PLAYER_RENAME_INVALID_NAME,
  PLAYER_RENAME_NAME_EXISTS,
  PLAYER_RENAME_PLAYER_NOT_FOUND,
  PLAYER_RENAME_DATABASE_UNAVAILABLE,
  PLAYER_RENAME_DATABASE_ERROR,
  PLAYER_RENAME_FILE_COLLISION,
  PLAYER_RENAME_FILE_ERROR,
  PLAYER_RENAME_SAVE_ERROR,
  PLAYER_RENAME_POSTCONDITION_FAILED
};

struct player_rename_report
{
  long player_id;
  int account_id;
  int account_linked;
  unsigned int database_rows_changed;
  unsigned int object_rows_changed;
  unsigned int files_moved;
  unsigned int introduction_files_changed;
  enum player_rename_status status;
  int rollback_succeeded;
  char old_name[MAX_NAME_LENGTH + 1];
  char new_name[MAX_NAME_LENGTH + 1];
  char failure_stage[80];
};

enum player_rename_status rename_player_everywhere(struct char_data *actor,
                                                   struct char_data *victim,
                                                   const char *requested_name,
                                                   struct player_rename_report *report);
const char *player_rename_status_string(enum player_rename_status status);

#ifdef LUMINARI_CUTEST
enum player_rename_test_failure_point
{
  PLAYER_RENAME_TEST_FAIL_NONE = 0,
  PLAYER_RENAME_TEST_FAIL_AUXILIARY_MOVE,
  PLAYER_RENAME_TEST_FAIL_DATABASE_UPDATE,
  PLAYER_RENAME_TEST_FAIL_PLAYER_FILE_WRITE,
  PLAYER_RENAME_TEST_FAIL_INTRODUCTION_WRITE,
  PLAYER_RENAME_TEST_FAIL_INDEX_WRITE,
  PLAYER_RENAME_TEST_FAIL_POSTCONDITION,
  PLAYER_RENAME_TEST_FAIL_COMMIT
};

void player_rename_set_failure_for_test(enum player_rename_test_failure_point point,
                                        unsigned int occurrence);
enum player_rename_status player_rename_validate_name_for_test(const char *old_name,
                                                               const char *requested_name,
                                                               char *display_name,
                                                               size_t display_size,
                                                               char *index_name, size_t index_size);
enum player_rename_status player_rename_memory_preflight_for_test(struct char_data *victim,
                                                                  const char *old_name,
                                                                  const char *new_name);
int player_rename_rewrite_file_for_test(const char *path, const char *old_name,
                                        const char *new_name, const char *account_name,
                                        int rewrite_identity, unsigned int *intro_changes);
int player_rename_file_identity_matches_for_test(const char *path, const char *name,
                                                 long player_id);
void player_rename_replace_generated_name_for_test(char **field, const char *old_name,
                                                   const char *new_name);
#endif

#endif /* PLAYER_RENAME_H */
