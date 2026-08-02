#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "account.h"
#include "roleplay.h"
#include "premadebuilds.h"
#include "character_creation.h"

#ifdef LUMINARI_CUTEST
static character_creation_save_callback character_creation_test_save_callback;
static struct character_creation_restart_test_hooks character_creation_restart_hooks;

void character_creation_set_save_callback_for_test(character_creation_save_callback callback)
{
  character_creation_test_save_callback = callback;
}

void character_creation_set_restart_hooks_for_test(
    const struct character_creation_restart_test_hooks *hooks)
{
  memset(&character_creation_restart_hooks, 0, sizeof(character_creation_restart_hooks));
  if (hooks != NULL)
    character_creation_restart_hooks = *hooks;
}
#endif

static bool character_creation_save(struct char_data *ch)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_test_save_callback != NULL)
    return character_creation_test_save_callback(ch, 0);
#endif

  return save_char_checked(ch, 0);
}

static int character_creation_lookup_player(const char *name)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.lookup_player != NULL)
    return character_creation_restart_hooks.lookup_player(name);
#endif
  return (int)get_ptable_by_name(name);
}

static bool character_creation_begin_account_removal(struct char_data *ch,
                                                     struct account_data *account)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.begin_account_removal != NULL)
    return character_creation_restart_hooks.begin_account_removal(ch, account);
#endif
  return begin_account_character_removal(ch, account);
}

static void character_creation_rollback_account_removal(void)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.rollback_account_removal != NULL)
  {
    character_creation_restart_hooks.rollback_account_removal();
    return;
  }
#endif
  rollback_account_character_removal();
}

static struct player_removal_transaction *
character_creation_prepare_player_removal(int player_position)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.prepare_player_removal != NULL)
    return character_creation_restart_hooks.prepare_player_removal(player_position);
#endif
  return prepare_player_removal_checked(player_position);
}

static bool character_creation_commit_account_removal(struct account_data *account)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.commit_account_removal != NULL)
    return character_creation_restart_hooks.commit_account_removal(account);
#endif
  return commit_account_character_removal(account);
}

static bool
character_creation_rollback_player_removal(struct player_removal_transaction *transaction)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.rollback_player_removal != NULL)
    return character_creation_restart_hooks.rollback_player_removal(transaction);
#endif
  return rollback_player_removal_checked(transaction);
}

static bool character_creation_commit_player_removal(struct player_removal_transaction *transaction)
{
#ifdef LUMINARI_CUTEST
  if (character_creation_restart_hooks.commit_player_removal != NULL)
    return character_creation_restart_hooks.commit_player_removal(transaction);
#endif
  return commit_player_removal_checked(transaction);
}

static bool is_core_creation_state(int state)
{
  switch (state)
  {
  case CON_GET_NAME:
  case CON_NAME_CNFRM:
  case CON_QSEX:
  case CON_QRACE:
  case CON_QRACE_HELP:
  case CON_QCLASS:
  case CON_QCLASS_HELP:
  case CON_CONFIRM_PREMADE:
  case CON_QALIGN:
    return TRUE;
  default:
    return FALSE;
  }
}

bool character_creation_stage_is_valid(int stage)
{
  return stage >= CHARACTER_CREATION_STAGE_NONE && stage < NUM_CHARACTER_CREATION_STAGES;
}

bool character_creation_is_active(const struct char_data *ch)
{
  return ch != NULL && !IS_NPC(ch) && GET_LEVEL(ch) == 0 &&
         CREATION_STAGE(ch) > CHARACTER_CREATION_STAGE_NONE &&
         CREATION_STAGE(ch) < NUM_CHARACTER_CREATION_STAGES;
}

bool character_creation_set_stage_checked(struct char_data *ch, enum character_creation_stage stage)
{
  int previous_stage = CHARACTER_CREATION_STAGE_NONE;

  if (ch == NULL || IS_NPC(ch) || !character_creation_stage_is_valid(stage))
    return FALSE;

  previous_stage = CREATION_STAGE(ch);
  CREATION_STAGE(ch) = stage;
  if (!character_creation_save(ch))
  {
    CREATION_STAGE(ch) = previous_stage;
    return FALSE;
  }
  return TRUE;
}

bool character_creation_finish_checked(struct char_data *ch)
{
  if (ch == NULL || IS_NPC(ch))
    return FALSE;
  if (!character_creation_is_active(ch))
    return TRUE;
  return character_creation_set_stage_checked(ch, CHARACTER_CREATION_STAGE_NONE);
}

bool character_creation_can_back(const struct descriptor_data *d)
{
  if (d == NULL || d->character == NULL || character_creation_is_active(d->character))
    return FALSE;

  return STATE(d) == CON_QRACE || STATE(d) == CON_QCLASS || STATE(d) == CON_CONFIRM_PREMADE ||
         STATE(d) == CON_QALIGN;
}

bool character_creation_back(struct descriptor_data *d)
{
  struct char_data *ch = NULL;

  if (!character_creation_can_back(d))
    return FALSE;

  ch = d->character;
  switch (STATE(d))
  {
  case CON_QRACE:
    GET_REAL_RACE(ch) = RACE_UNDEFINED;
    GET_CLASS(ch) = CLASS_UNDEFINED;
    GET_PREMADE_BUILD_CLASS(ch) = CLASS_UNDEFINED;
    GET_ALIGNMENT(ch) = 0;
    STATE(d) = CON_QSEX;
    write_to_output(d, "\r\nReturned to identity selection. What is your sex (M/F)? ");
    break;
  case CON_QCLASS:
    GET_CLASS(ch) = CLASS_UNDEFINED;
    GET_PREMADE_BUILD_CLASS(ch) = CLASS_UNDEFINED;
    GET_ALIGNMENT(ch) = 0;
    STATE(d) = CON_QRACE;
    write_to_output(d, "\r\nReturned to ancestry selection. Choose a race: ");
    break;
  case CON_CONFIRM_PREMADE:
    GET_PREMADE_BUILD_CLASS(ch) = CLASS_UNDEFINED;
    GET_ALIGNMENT(ch) = 0;
    STATE(d) = CON_QCLASS;
    write_to_output(d, "\r\nReturned to class selection. Choose a class: ");
    break;
  case CON_QALIGN:
    GET_ALIGNMENT(ch) = 0;
    STATE(d) = CON_CONFIRM_PREMADE;
    write_to_output(d, "\r\nReturned to build selection. Enter either 'premade' or 'custom': ");
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

bool character_creation_can_restart(const struct descriptor_data *d)
{
  if (d == NULL || d->character == NULL || IS_NPC(d->character) || d->account == NULL)
    return FALSE;

  if (is_core_creation_state(STATE(d)) && !character_creation_is_active(d->character))
    return TRUE;

  return character_creation_is_active(d->character) && GET_LEVEL(d->character) == 0;
}

static void return_to_account_lobby(struct descriptor_data *d)
{
  roleplay_pending_clear(d);
  d->forced_short_desc_setup = FALSE;
  if (d->character != NULL)
  {
    free_char(d->character);
    d->character = NULL;
  }
  STATE(d) = CON_ACCOUNT_MENU;
  show_account_menu(d);
}

enum character_creation_restart_result character_creation_restart(struct descriptor_data *d)
{
  struct player_removal_transaction *player_transaction = NULL;
  int player_position = -1;
  bool player_rollback_ok = TRUE;

  if (!character_creation_can_restart(d))
    return CHARACTER_CREATION_RESTART_UNAVAILABLE;

  if (!character_creation_is_active(d->character))
  {
    return_to_account_lobby(d);
    return CHARACTER_CREATION_RESTART_OK;
  }

  player_position = character_creation_lookup_player(GET_NAME(d->character));
  if (player_position < 0)
    return CHARACTER_CREATION_RESTART_PLAYER_FAILED;

  if (!character_creation_begin_account_removal(d->character, d->account))
    return CHARACTER_CREATION_RESTART_ACCOUNT_FAILED;

  player_transaction = character_creation_prepare_player_removal(player_position);
  if (player_transaction == NULL)
  {
    character_creation_rollback_account_removal();
    return CHARACTER_CREATION_RESTART_PLAYER_FAILED;
  }

  if (!character_creation_commit_account_removal(d->account))
  {
    player_rollback_ok = character_creation_rollback_player_removal(player_transaction);
    return player_rollback_ok ? CHARACTER_CREATION_RESTART_ACCOUNT_FAILED
                              : CHARACTER_CREATION_RESTART_ROLLBACK_FAILED;
  }

  /*
   * At this point no live player path, index entry, or account link remains.
   * A failed unlink can leave only an explicitly named staging file, which is
   * logged for operators and cannot be loaded as a character.
   */
  (void)character_creation_commit_player_removal(player_transaction);
  return_to_account_lobby(d);
  return CHARACTER_CREATION_RESTART_OK;
}

bool character_creation_resume(struct descriptor_data *d)
{
  if (d == NULL || d->character == NULL || !character_creation_is_active(d->character))
    return FALSE;

  switch (CREATION_STAGE(d->character))
  {
  case CHARACTER_CREATION_STAGE_PREFERENCES:
    write_to_output(d, "\r\nResume character creation: use the recommended preference settings? "
                       "(yes/no) ");
    STATE(d) = CON_SETPREFS;
    return TRUE;
  case CHARACTER_CREATION_STAGE_ROLEPLAY_DECISION:
    display_rp_decide_menu(d);
    STATE(d) = CON_CHAR_RP_DECIDE;
    return TRUE;
  case CHARACTER_CREATION_STAGE_ROLEPLAY_PROFILE:
    show_character_rp_menu(d);
    STATE(d) = CON_CHAR_RP_MENU;
    return TRUE;
  case CHARACTER_CREATION_STAGE_MENU_PENDING:
    write_to_output(d, "\r\n%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;
    return TRUE;
  default:
    return FALSE;
  }
}
