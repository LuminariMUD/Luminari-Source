/*
 * clan_transactions.c - Transaction support for atomic clan operations
 * 
 * This file implements a transaction system for clan operations to ensure
 * atomicity. If any part of a complex operation fails, all changes can be
 * rolled back to maintain data consistency.
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "clan.h"

/* Transaction types */
#define TRANS_TREASURY_CHANGE   0
#define TRANS_MEMBER_ADD        1
#define TRANS_MEMBER_REMOVE     2
#define TRANS_RANK_CHANGE       3
#define TRANS_WAR_DECLARE       4
#define TRANS_WAR_END          5
#define TRANS_ALLIANCE_FORM     6
#define TRANS_ALLIANCE_BREAK    7
#define TRANS_ZONE_CLAIM        8
#define TRANS_ZONE_UNCLAIM      9
#define TRANS_STAT_UPDATE       10
#define NUM_TRANS_TYPES         11

/* Transaction states */
#define TRANS_STATE_PENDING     0
#define TRANS_STATE_COMMITTED   1
#define TRANS_STATE_ROLLED_BACK 2

/* Maximum operations per transaction */
#define MAX_TRANS_OPERATIONS    50

/* Structure for a single transaction operation */
struct trans_operation {
    int type;                   /* Type of operation */
    clan_rnum clan;            /* Clan affected */
    long player_id;            /* Player affected (if applicable) */
    int old_value;             /* Previous value (for rollback) */
    int new_value;             /* New value */
    long old_long_value;       /* For treasury changes */
    long new_long_value;       /* For treasury changes */
    char *old_string;          /* For string changes */
    char *new_string;          /* For string changes */
    void *extra_data;          /* Additional data if needed */
};

/* Structure for a transaction */
struct clan_transaction {
    int id;                                     /* Transaction ID */
    int state;                                  /* Current state */
    time_t start_time;                         /* When transaction started */
    struct char_data *initiator;               /* Who started it */
    struct trans_operation operations[MAX_TRANS_OPERATIONS];
    int num_operations;                        /* Number of operations */
    char description[256];                     /* Transaction description */
    struct clan_transaction *next;             /* Linked list */
};

/* Global transaction list */
static struct clan_transaction *active_transactions = NULL;
static int next_trans_id = 1;

/* Forward declarations */
static struct clan_transaction *create_transaction(struct char_data *ch, const char *desc);
static void free_transaction(struct clan_transaction *trans);
static bool execute_operation(struct trans_operation *op);
static bool rollback_operation(struct trans_operation *op);
static void log_transaction(struct clan_transaction *trans, const char *action);
bool rollback_clan_transaction(struct clan_transaction *trans);
void remove_from_transaction_list(struct clan_transaction *trans);
void update_player_index_clan(long player_id, clan_vnum clan, int rank);

/* Create a new transaction */
struct clan_transaction *begin_clan_transaction(struct char_data *ch, const char *description)
{
    struct clan_transaction *trans;
    
    if (!ch || !description) {
        log("SYSERR: begin_clan_transaction called with NULL parameters");
        return NULL;
    }
    
    trans = create_transaction(ch, description);
    if (!trans) {
        send_to_char(ch, "Failed to create transaction.\r\n");
        return NULL;
    }
    
    /* Add to active list */
    trans->next = active_transactions;
    active_transactions = trans;
    
    log_transaction(trans, "BEGIN");
    
    return trans;
}

/* Add an operation to a transaction */
bool add_clan_trans_operation(struct clan_transaction *trans, int type, 
                             clan_rnum clan, long player_id,
                             int old_val, int new_val)
{
    struct trans_operation *op;
    
    if (!trans) {
        log("SYSERR: add_clan_trans_operation called with NULL transaction");
        return FALSE;
    }
    
    if (trans->state != TRANS_STATE_PENDING) {
        log("SYSERR: Attempt to add operation to non-pending transaction %d", trans->id);
        return FALSE;
    }
    
    if (trans->num_operations >= MAX_TRANS_OPERATIONS) {
        log("SYSERR: Transaction %d has reached maximum operations", trans->id);
        return FALSE;
    }
    
    op = &trans->operations[trans->num_operations];
    op->type = type;
    op->clan = clan;
    op->player_id = player_id;
    op->old_value = old_val;
    op->new_value = new_val;
    op->old_long_value = 0;
    op->new_long_value = 0;
    op->old_string = NULL;
    op->new_string = NULL;
    op->extra_data = NULL;
    
    trans->num_operations++;
    
    return TRUE;
}

/* Add a treasury operation */
bool add_clan_trans_treasury(struct clan_transaction *trans, clan_rnum clan,
                            long old_amount, long new_amount)
{
    struct trans_operation *op;
    
    if (!trans || clan < 0 || clan >= num_of_clans) {
        log("SYSERR: add_clan_trans_treasury called with invalid parameters");
        return FALSE;
    }
    
    if (trans->num_operations >= MAX_TRANS_OPERATIONS) {
        return FALSE;
    }
    
    op = &trans->operations[trans->num_operations];
    op->type = TRANS_TREASURY_CHANGE;
    op->clan = clan;
    op->player_id = 0;
    op->old_value = 0;
    op->new_value = 0;
    op->old_long_value = old_amount;
    op->new_long_value = new_amount;
    op->old_string = NULL;
    op->new_string = NULL;
    op->extra_data = NULL;
    
    trans->num_operations++;
    
    return TRUE;
}

/* Commit a transaction */
bool commit_clan_transaction(struct clan_transaction *trans)
{
    int i;
    bool success = TRUE;
    
    if (!trans) {
        log("SYSERR: commit_clan_transaction called with NULL transaction");
        return FALSE;
    }
    
    if (trans->state != TRANS_STATE_PENDING) {
        log("SYSERR: Attempt to commit non-pending transaction %d", trans->id);
        return FALSE;
    }
    
    /* Execute all operations */
    for (i = 0; i < trans->num_operations && success; i++) {
        if (!execute_operation(&trans->operations[i])) {
            success = FALSE;
            break;
        }
    }
    
    if (success) {
        trans->state = TRANS_STATE_COMMITTED;
        log_transaction(trans, "COMMIT");
        
        /* Save affected clans */
        for (i = 0; i < trans->num_operations; i++) {
            if (trans->operations[i].clan >= 0 && 
                trans->operations[i].clan < num_of_clans) {
                mark_clan_modified(trans->operations[i].clan);
                save_single_clan(trans->operations[i].clan);
            }
        }
    } else {
        /* Rollback on failure */
        rollback_clan_transaction(trans);
    }
    
    /* Remove from active list and free */
    remove_from_transaction_list(trans);
    free_transaction(trans);
    
    return success;
}

/* Rollback a transaction */
bool rollback_clan_transaction(struct clan_transaction *trans)
{
    int i;
    
    if (!trans) {
        log("SYSERR: rollback_clan_transaction called with NULL transaction");
        return FALSE;
    }
    
    if (trans->state == TRANS_STATE_COMMITTED) {
        log("SYSERR: Cannot rollback committed transaction %d", trans->id);
        return FALSE;
    }
    
    /* Rollback operations in reverse order */
    for (i = trans->num_operations - 1; i >= 0; i--) {
        rollback_operation(&trans->operations[i]);
    }
    
    trans->state = TRANS_STATE_ROLLED_BACK;
    log_transaction(trans, "ROLLBACK");
    
    /* Save affected clans */
    for (i = 0; i < trans->num_operations; i++) {
        if (trans->operations[i].clan >= 0 && 
            trans->operations[i].clan < num_of_clans) {
            mark_clan_modified(trans->operations[i].clan);
            save_single_clan(trans->operations[i].clan);
        }
    }
    
    return TRUE;
}

/* Execute a single operation */
static bool execute_operation(struct trans_operation *op)
{
    struct char_data *victim;
    
    if (!op || op->clan < 0 || op->clan >= num_of_clans) {
        return FALSE;
    }
    
    switch (op->type) {
        case TRANS_TREASURY_CHANGE:
            clan_list[op->clan].treasure = op->new_long_value;
            return TRUE;
            
        case TRANS_MEMBER_ADD:
            /* Find the player */
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLAN(victim) = clan_list[op->clan].vnum;
                    GET_CLANRANK(victim) = op->new_value;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            /* Player not online, update player index */
            update_player_index_clan(op->player_id, clan_list[op->clan].vnum, op->new_value);
            return TRUE;
            
        case TRANS_MEMBER_REMOVE:
            /* Find the player */
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLAN(victim) = NO_CLAN;
                    GET_CLANRANK(victim) = NO_CLANRANK;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            /* Player not online, update player index */
            update_player_index_clan(op->player_id, NO_CLAN, NO_CLANRANK);
            return TRUE;
            
        case TRANS_RANK_CHANGE:
            /* Find the player */
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLANRANK(victim) = op->new_value;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            /* Player not online, update player index */
            update_player_index_clan(op->player_id, clan_list[op->clan].vnum, op->new_value);
            return TRUE;
            
        case TRANS_WAR_DECLARE:
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].at_war[op->new_value] = TRUE;
                clan_list[op->new_value].at_war[op->clan] = TRUE;
                clan_list[op->clan].war_timer = DEFAULT_WAR_DURATION;
                clan_list[op->new_value].war_timer = DEFAULT_WAR_DURATION;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_WAR_END:
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].at_war[op->new_value] = FALSE;
                clan_list[op->new_value].at_war[op->clan] = FALSE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_ALLIANCE_FORM:
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].allies[op->new_value] = TRUE;
                clan_list[op->new_value].allies[op->clan] = TRUE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_ALLIANCE_BREAK:
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].allies[op->new_value] = FALSE;
                clan_list[op->new_value].allies[op->clan] = FALSE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_STAT_UPDATE:
            /* Handle various stat updates based on old_value as sub-type */
            switch (op->old_value) {
                case 0: /* total_deposits */
                    clan_list[op->clan].total_deposits = op->new_long_value;
                    break;
                case 1: /* total_withdrawals */
                    clan_list[op->clan].total_withdrawals = op->new_long_value;
                    break;
                case 2: /* total_members_joined */
                    clan_list[op->clan].total_members_joined = op->new_value;
                    break;
                case 3: /* total_members_left */
                    clan_list[op->clan].total_members_left = op->new_value;
                    break;
            }
            return TRUE;
            
        default:
            log("SYSERR: Unknown transaction operation type %d", op->type);
            return FALSE;
    }
}

/* Rollback a single operation */
static bool rollback_operation(struct trans_operation *op)
{
    struct char_data *victim;
    
    if (!op || op->clan < 0 || op->clan >= num_of_clans) {
        return FALSE;
    }
    
    switch (op->type) {
        case TRANS_TREASURY_CHANGE:
            clan_list[op->clan].treasure = op->old_long_value;
            return TRUE;
            
        case TRANS_MEMBER_ADD:
            /* Reverse of add - remove the member */
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLAN(victim) = NO_CLAN;
                    GET_CLANRANK(victim) = NO_CLANRANK;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            update_player_index_clan(op->player_id, NO_CLAN, NO_CLANRANK);
            return TRUE;
            
        case TRANS_MEMBER_REMOVE:
            /* Reverse of remove - add the member back */
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLAN(victim) = clan_list[op->clan].vnum;
                    GET_CLANRANK(victim) = op->old_value;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            update_player_index_clan(op->player_id, clan_list[op->clan].vnum, op->old_value);
            return TRUE;
            
        case TRANS_RANK_CHANGE:
            for (victim = character_list; victim; victim = victim->next) {
                if (!IS_NPC(victim) && GET_IDNUM(victim) == op->player_id) {
                    GET_CLANRANK(victim) = op->old_value;
                    save_char(victim, 0);
                    return TRUE;
                }
            }
            update_player_index_clan(op->player_id, clan_list[op->clan].vnum, op->old_value);
            return TRUE;
            
        case TRANS_WAR_DECLARE:
            /* Reverse - end the war */
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].at_war[op->new_value] = FALSE;
                clan_list[op->new_value].at_war[op->clan] = FALSE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_WAR_END:
            /* Reverse - declare war again (unusual but maintains consistency) */
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].at_war[op->new_value] = TRUE;
                clan_list[op->new_value].at_war[op->clan] = TRUE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_ALLIANCE_FORM:
            /* Reverse - break the alliance */
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].allies[op->new_value] = FALSE;
                clan_list[op->new_value].allies[op->clan] = FALSE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_ALLIANCE_BREAK:
            /* Reverse - form the alliance again */
            if (op->new_value >= 0 && op->new_value < num_of_clans) {
                clan_list[op->clan].allies[op->new_value] = TRUE;
                clan_list[op->new_value].allies[op->clan] = TRUE;
                return TRUE;
            }
            return FALSE;
            
        case TRANS_STAT_UPDATE:
            /* Restore old stat values */
            switch (op->old_value) {
                case 0: /* total_deposits */
                    clan_list[op->clan].total_deposits = op->old_long_value;
                    break;
                case 1: /* total_withdrawals */
                    clan_list[op->clan].total_withdrawals = op->old_long_value;
                    break;
                case 2: /* total_members_joined */
                    clan_list[op->clan].total_members_joined = op->old_value;
                    break;
                case 3: /* total_members_left */
                    clan_list[op->clan].total_members_left = op->old_value;
                    break;
            }
            return TRUE;
            
        default:
            log("SYSERR: Unknown transaction operation type %d in rollback", op->type);
            return FALSE;
    }
}

/* Create a new transaction structure */
static struct clan_transaction *create_transaction(struct char_data *ch, const char *desc)
{
    struct clan_transaction *trans;
    
    CREATE(trans, struct clan_transaction, 1);
    trans->id = next_trans_id++;
    trans->state = TRANS_STATE_PENDING;
    trans->start_time = time(0);
    trans->initiator = ch;
    trans->num_operations = 0;
    trans->next = NULL;
    
    strncpy(trans->description, desc, sizeof(trans->description) - 1);
    trans->description[sizeof(trans->description) - 1] = '\0';
    
    return trans;
}

/* Free a transaction structure */
static void free_transaction(struct clan_transaction *trans)
{
    int i;
    
    if (!trans)
        return;
    
    /* Free any allocated strings in operations */
    for (i = 0; i < trans->num_operations; i++) {
        if (trans->operations[i].old_string)
            free(trans->operations[i].old_string);
        if (trans->operations[i].new_string)
            free(trans->operations[i].new_string);
        if (trans->operations[i].extra_data)
            free(trans->operations[i].extra_data);
    }
    
    free(trans);
}

/* Remove transaction from active list */
void remove_from_transaction_list(struct clan_transaction *trans)
{
    struct clan_transaction *temp, *prev = NULL;
    
    for (temp = active_transactions; temp; prev = temp, temp = temp->next) {
        if (temp == trans) {
            if (prev)
                prev->next = temp->next;
            else
                active_transactions = temp->next;
            break;
        }
    }
}

/* Log transaction activity */
static void log_transaction(struct clan_transaction *trans, const char *action)
{
    if (!trans || !action)
        return;
    
    mudlog(NRM, LVL_STAFF, TRUE, "CLAN TRANS: %s transaction %d (%s) by %s",
           action, trans->id, trans->description,
           trans->initiator ? GET_NAME(trans->initiator) : "SYSTEM");
}

/* Cleanup expired transactions (called periodically) */
void cleanup_clan_transactions(void)
{
    struct clan_transaction *trans, *next_trans;
    time_t current_time = time(0);
    
    for (trans = active_transactions; trans; trans = next_trans) {
        next_trans = trans->next;
        
        /* Rollback and remove transactions older than 5 minutes */
        if ((current_time - trans->start_time) > 300) {
            log("CLAN TRANS: Auto-rollback expired transaction %d", trans->id);
            rollback_clan_transaction(trans);
            remove_from_transaction_list(trans);
            free_transaction(trans);
        }
    }
}

/* Update player index clan data (for offline players) */
void update_player_index_clan(long player_id, clan_vnum clan, int rank)
{
    int i;
    
    /* Find player in index and update */
    for (i = 0; i <= top_of_p_table; i++) {
        if (player_table[i].id == player_id) {
            player_table[i].clan = clan;
            /* Can't update rank without loading player file */
            /* TODO: Consider adding clanrank to player_index_element */
            save_player_index();
            return;
        }
    }
    
    log("SYSERR: Player ID %ld not found in player index for clan update", player_id);
}