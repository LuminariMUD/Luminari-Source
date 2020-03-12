/* @file actionqueues.h
 * Header file for the Action Queue system for Luminari MUD
 */

#ifndef _ACTIONQUEUES_H_
#define _ACTIONQUEUES_H_

#define MAX_QUEUE_SIZE 10

#define SCMD_ACTION_QUEUE 0
#define SCMD_ATTACK_QUEUE 1

/* AQ specific commands */
ACMD(do_queue);

struct action_data
{
    char *argument;
    int actions_required;
};

struct attack_action_data
{
    int command;     /* Command that generated the action. */
    int attack_type; /* Attack type - Subcmd of the command. */
    char *argument;  /* Argument for the attack */
};

struct queue_element_type
{
    //  	struct action_data   * action;
    void *data;
    struct queue_element_type *next;
};

struct queue_type
{
    struct queue_element_type *first;
    struct queue_element_type *last;
    int size;
};

/* Action queue */
struct queue_type *create_action_queue();
void free_action_queue(struct queue_type *queue);
void clear_action_queue(struct queue_type *queue);
void enqueue_action(struct queue_type *queue, struct action_data *action);
struct action_data *dequeue_action(struct queue_type *queue);
struct action_data *peek_action(struct queue_type *queue);

void execute_next_action(struct char_data *ch);
int pending_actions(struct char_data *ch);

/*  Attack queue */
struct queue_type *create_attack_queue();
void free_attack_queue(struct queue_type *queue);
void clear_attack_queue(struct queue_type *queue);
void enqueue_attack(struct queue_type *queue, struct attack_action_data *attack);
struct attack_action_data *dequeue_attack(struct queue_type *queue);
struct attack_action_data *peek_attack(struct queue_type *queue);

int pending_attacks(struct char_data *ch);

#endif /* _ACTIONQUEUES_H_ */
