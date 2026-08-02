/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\
/  Luminari Mail System
/  Created By: Gicker
\
/
\         todo:
/
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

/* adjusted to return number of NEW mail and added 'silent' mode -zusuk */
int new_mail_alert(struct char_data *ch, bool silent);
bool new_mail_send_system(const char *receiver, const char *subject, const char *message);

ACMD_DECL(do_new_mail);

/*eof*/
