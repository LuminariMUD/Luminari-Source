/*
 * clan_transactions.h - Header for clan transaction system
 */

#ifndef _CLAN_TRANSACTIONS_H_
#define _CLAN_TRANSACTIONS_H_

/* Forward declaration */
struct clan_transaction;

/* Function prototypes */
struct clan_transaction *begin_clan_transaction(struct char_data *ch, const char *description);
bool add_clan_trans_operation(struct clan_transaction *trans, int type, 
                             clan_rnum clan, long player_id,
                             int old_val, int new_val);
bool add_clan_trans_treasury(struct clan_transaction *trans, clan_rnum clan,
                            long old_amount, long new_amount);
bool commit_clan_transaction(struct clan_transaction *trans);
bool rollback_clan_transaction(struct clan_transaction *trans);
void cleanup_clan_transactions(void);
void update_player_index_clan(long player_id, clan_vnum clan, int rank);
void remove_from_transaction_list(struct clan_transaction *trans);

/* Transaction types - used in add_clan_trans_operation */
#define CLAN_TRANS_TREASURY     0
#define CLAN_TRANS_MEMBER_ADD   1
#define CLAN_TRANS_MEMBER_REM   2
#define CLAN_TRANS_RANK_CHANGE  3
#define CLAN_TRANS_WAR_DECLARE  4
#define CLAN_TRANS_WAR_END      5
#define CLAN_TRANS_ALLY_FORM    6
#define CLAN_TRANS_ALLY_BREAK   7
#define CLAN_TRANS_ZONE_CLAIM   8
#define CLAN_TRANS_ZONE_UNCLAIM 9
#define CLAN_TRANS_STAT_UPDATE  10

#endif /* _CLAN_TRANSACTIONS_H_ */