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
    /*******************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ACCOUNT_H */
