/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Account System, Inspired by D20mud's Account System
/  Created By: zusuk
\
/
\         todo: move header stuff into account.h
/         Created on January 24, 2018, 3:37 PM
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#ifndef ACCOUNT_H
#define ACCOUNT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************/
/* defines */
/*******************************************************/
/* external functions */
void perform_do_account(struct char_data *ch, struct char_data *vict);
int change_account_xp(struct char_data *ch, int change_val);
void load_account_characters(struct account_data *account);
bool link_character_to_account_checked(struct char_data *ch, struct account_data *account);
bool save_account_checked(struct account_data *account);

/*
 * Hold the account-side character unlink inside a database transaction while
 * the corresponding player files and index are staged for checked removal.
 */
bool begin_account_character_removal(struct char_data *ch, struct account_data *account);
bool commit_account_character_removal(struct account_data *account);
void rollback_account_character_removal(void);

/*******************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ACCOUNT_H */
