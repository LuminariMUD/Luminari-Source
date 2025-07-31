/**
 * @file actionqueues.c
 *
 * Author:  Ornir
 *
 * Action Queue system for Luminari MUD
 *
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "lists.h"
#include "actionqueues.h"
#include "mud_event.h"
#include "actions.h"

/* Initialize the queue, must be performed on any new queues. */
struct queue_type *create_queue()
{
  struct queue_type *queue = NULL;

  /* Allocate memory. */
  CREATE(queue, struct queue_type, 1);

  /* Initialize. */
  queue->first = NULL;
  queue->last = NULL;
  queue->size = 0;

  return queue;
};

struct queue_type *create_action_queue()
{
  return create_queue();
}

struct queue_type *create_attack_queue()
{
  return create_queue();
}

/* Empty the queue  and release the memory.  The queue pointer is
 * invalid after this operation. */
void free_action_queue(struct queue_type *queue)
{
  clear_action_queue(queue);
  free(queue);
};

void free_attack_queue(struct queue_type *queue)
{
  clear_attack_queue(queue);
  free(queue);
}

/* Empty the queue. */
void clear_action_queue(struct queue_type *queue)
{
  struct action_data *action = NULL;

  /* Check for NULL queue*/
  if (queue == NULL)
    return;
  else
  {
    /* Dequeue all the actions from the queue. */
    while (queue->size > 0)
    {
      action = dequeue_action(queue);

      /* Free the memory. */
      free(action->argument);
      free(action);
    }
  }
  /* Send a custom MSDP event so clients can manage queue displays. */
};

void clear_attack_queue(struct queue_type *queue)
{
  struct attack_action_data *attack = NULL;

  /* Check for NULL queue*/
  if (queue == NULL)
    return;
  else
  {
    /* Dequeue all the actions from the queue. */
    while (queue->size > 0)
    {
      attack = dequeue_attack(queue);

      /* Free the memory. */
      free(attack->argument);
      free(attack);
    }
  }
  /* Send a custom MSDP event so clients can manage queue displays. */
};

void enqueue(struct queue_type *queue, void *data)
{
  struct queue_element_type *el = NULL;

  CREATE(el, struct queue_element_type, 1);

  el->data = data;
  el->next = NULL;

  if (queue->first == NULL)
  {
    queue->first = el;
  }
  else
  {
    queue->last->next = el;
  }
  queue->last = el;
  queue->size += 1;
}

/* Add an action to the queue. */
void enqueue_action(struct queue_type *queue, struct action_data *action)
{
  enqueue(queue, (void *)action);

  /* Send a custom MSDP event so clients can manage queue displays */
};

/* Add an attack to the queue. */
void enqueue_attack(struct queue_type *queue, struct attack_action_data *attack)
{
  enqueue(queue, (void *)attack);
  /* Send a custom MSDP event so clients can manage queue displays */
};

void *dequeue(struct queue_type *queue)
{
  void *data;
  struct queue_element_type *el;

  if ((queue == NULL) || (queue->first == NULL))
    return NULL;
  else
  {
    el = queue->first;
    data = el->data;

    queue->first = el->next;

    if (queue->first == NULL)
      queue->last = NULL;

    free(el); /* Note: Queue element is freed, not the data. */

    queue->size -= 1;

    return data;
  }
}

/* Remove and return an action from the queue. */
struct action_data *dequeue_action(struct queue_type *queue)
{
  struct action_data *action = NULL;

  action = (struct action_data *)dequeue(queue);

  if (action != NULL)
  {
    /* Send a custom MSDP event so clients can manage queue displays */
  }

  return action;
}

/* Remove and return an attack from the queue. */
struct attack_action_data *dequeue_attack(struct queue_type *queue)
{
  struct attack_action_data *attack = NULL;

  attack = (struct attack_action_data *)dequeue(queue);

  if (attack != NULL)
  {
    /* Send a custom MSDP event so clients can manage queue displays */
  }

  return attack;
}

void *peek(struct queue_type *queue)
{
  if (queue == NULL)
    return NULL;
  else if (queue->first == NULL)
    return NULL;
  else
    return queue->first->data;
}

/* Return a pointer to the first action on the queue.  DO NOT DELETE IT.
 * This function just gives you a 'peek' and does not dequeue the action. */
struct action_data *peek_action(struct queue_type *queue)
{
  return (struct action_data *)peek(queue);
};

struct attack_action_data *peek_attack(struct queue_type *queue)
{
  return (struct attack_action_data *)peek(queue);
};

/* Check to see if the next action on the queue owned by ch can be executed.
 * If so, dequeue and execute the action. */
void execute_next_action(struct char_data *ch)
{
  struct action_data *action = NULL;

  if ((ch == NULL))
    return;

  action = peek_action(GET_QUEUE(ch));

  if (action == NULL) /* No action. */
    return;

  if (IS_SET(action->actions_required, ACTION_STANDARD) && !is_action_available(ch, atSTANDARD, FALSE))
    return;

  if (IS_SET(action->actions_required, ACTION_MOVE) && !is_action_available(ch, atMOVE, FALSE))
    return;

  action = dequeue_action(GET_QUEUE(ch));
  command_interpreter(ch, action->argument);
};

/* Check if there are pending actions on the queue. */
int pending_actions(struct char_data *ch)
{
  if (ch == NULL)
    return 0;

  return GET_QUEUE(ch)->size;
};

/* Check if there are pending attacks on the queue. */
int pending_attacks(struct char_data *ch)
{
  if (ch == NULL)
    return 0;

  return GET_ATTACK_QUEUE(ch)->size;
};

/* The action queue, being an integral part of gameplay,
 * requires some management by the players from time to
 * time, especially during combat when strategies may
 * need to change moment by moment.
 * Using this command, the player may manipulate their
 * action queue in various ways. */
ACMD(do_queue)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct queue_type *queue;
  struct queue_element_type *el;

  int i = 1;

  if (subcmd == SCMD_ACTION_QUEUE)
    queue = GET_QUEUE(ch);
  else
    queue = GET_ATTACK_QUEUE(ch);

  if (!*argument)
  {
    /* No arguments - List the currently queued actions. */
    send_to_char(ch, "%s:\r\n", (subcmd == SCMD_ACTION_QUEUE ? "Action Queue" : "Attack Queue"));

    if (queue != NULL)
    {
      for (el = queue->first; el != NULL; el = el->next)
      {
        if (subcmd == SCMD_ACTION_QUEUE)
        {
          send_to_char(ch, " %i) %s\r\n", i++, ((struct action_data *)el->data)->argument);
        }
        else
        {
          send_to_char(ch, " %i) %s%s\r\n", i++,
                       complete_cmd_info[((struct attack_action_data *)el->data)->command].command,
                       ((struct attack_action_data *)el->data)->argument);
        }
      }
    }

    return;
  }

  one_argument(argument, arg, sizeof(arg));

  if (is_abbrev(arg, "clear"))
  {

    /* Argument: clear - Clear the queue. */
    if (subcmd == SCMD_ACTION_QUEUE)
      clear_action_queue(GET_QUEUE(ch));
    else if (subcmd == SCMD_ATTACK_QUEUE)
      clear_attack_queue(GET_ATTACK_QUEUE(ch));

    send_to_char(ch, "%s queue cleared.\r\n", (subcmd == SCMD_ACTION_QUEUE ? "Action" : "Attack"));
  }
  else
  {
    send_to_char(ch, "What do you want to do to your queue? ('queue clear' to clear your queue)\r\n");
  }
}
