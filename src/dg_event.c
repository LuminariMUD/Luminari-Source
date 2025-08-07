/**
* @file dg_event.c                          LuminariMUD
* This file contains a simplified event system to allow trigedit 
* to use the "wait" command, causing a delay in the middle of a script.
* This system could easily be expanded by coders who wish to implement
* an event driven mud.
* 
* Part of the core tbaMUD source code distribution, which is a derivative
* of, and continuation of, CircleMUD.
* 
* This source code, which was not part of the CircleMUD legacy code,
* was created by the following people:                                      
* $Author: Mark A. Heilpern/egreen/Welcor $                              
* $Date: 2004/10/11 12:07:00$                                            
* $Revision: 1.0.14 $                                                    
*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "dg_event.h"
#include "constants.h"
#include "comm.h" /* For access to the game pulse */
#include "mud_event.h"

/***************************************************************************
 * Begin mud specific event queue functions
 **************************************************************************/
/* file scope variables */
/** The mud specific queue of events. */
static struct dg_queue *event_q = NULL;
/** Flag to track if we're currently processing events (prevents dangerous operations) */
static int processing_events = 0;

/** Initializes the main event queue event_q.
 * @post The main event queue, event_q, has been created and initialized.
 */
void event_init(void)
{
  event_q = queue_init();
}

/** Creates a new event 'object' that is then enqueued to the global event_q.
 * @post If the newly created event is valid, it is always added to event_q.
 * @param func The function to be called when this event fires. This function
 * will be passed event_obj when it fires. The function must match the form
 * described by EVENTFUNC. 
 * @param event_obj An optional 'something' to be passed to func when this
 * event fires. It is func's job to cast event_obj. If event_obj is not needed,
 * pass in NULL.
 * @param when Number of pulses between firing(s) of this event.
 * @retval event * Returns a pointer to the newly created event.
 * */
struct event *event_create(EVENTFUNC(*func), void *event_obj, long when)
{
  struct event *new_event = NULL;

  /* Safety check: ensure event_q is initialized */
  if (!event_q)
  {
    log("SYSERR: event_create called before event_init()");
    return NULL;
  }

  if (when < 1) /* make sure its in the future */
    when = 1;

  CREATE(new_event, struct event, 1);
  new_event->func = func;
  new_event->event_obj = event_obj;
  new_event->q_el = queue_enq(event_q, new_event, when + pulse);
  new_event->isMudEvent = FALSE;

  return new_event;
}

/** Removes an event from event_q and frees the event. 
 * @param event Pointer to the event to be dequeued and removed. 
 */
void event_cancel(struct event *event)
{
  if (!event)
  {
    log("SYSERR:  Attempted to cancel a NULL event");
    return;
  }

  /* CRITICAL FIX: Double-free prevention
   * 
   * PROBLEM EXPLAINED FOR BEGINNERS:
   * When event_process() runs an event, it sets q_el to NULL (line ~142) 
   * BEFORE calling the event's function. This tells us the event is 
   * currently being processed.
   * 
   * If an event tries to cancel ITSELF while running (calls event_cancel 
   * on itself), we must NOT free it here because event_process() will 
   * free it when the event function returns (line ~153).
   * 
   * Freeing the same memory twice (double-free) causes crashes and 
   * memory corruption!
   * 
   * SOLUTION: 
   * If q_el is NULL, the event is being processed right now.
   * We only clean up the event_obj but do NOT free the event structure.
   * event_process() will handle freeing it when done.
   */
  if (!event->q_el)
  {
    /* Event is currently being processed - DO NOT free the event structure! */
    log("WARNING: Attempted to cancel an event during its execution.");
    
    /* IMPORTANT: We only handle mud events here. For non-mud events,
     * the event function itself is responsible for freeing event_obj.
     * We just need to prevent event_process() from double-freeing mud events. */
    if (event->isMudEvent && event->event_obj)
    {
      /* For mud events, we free the data and set to NULL to prevent 
       * event_process() from trying to free it again */
      cleanup_event_obj(event);
      event->event_obj = NULL;
    }
    /* For non-mud events, do NOT touch event_obj - the event function handles it */
    
    return; /* DO NOT free the event structure - event_process() will do it */
  }
  
  /* Event is in the queue and not currently running - safe to fully cancel */
  queue_deq(event_q, event->q_el);

  if (event->event_obj)
    cleanup_event_obj(event);

  free(event);
}

/* The memory freeing routine tied into the mud event system.
 * 
 * IMPORTANT FOR BEGINNERS:
 * This function cleans up the data associated with an event when the event
 * is being removed from the system. There are two types of events:
 * 1. Mud Events: Complex events with structured data (freed via free_mud_event)
 * 2. Simple Events: Basic events with malloc'd data (freed via free)
 * 
 * CRITICAL DESIGN NOTE:
 * For non-mud events, we assume event_obj was dynamically allocated (malloc'd).
 * If event_obj points to static or stack memory, calling free() will crash!
 * Currently, ALL non-mud events in the codebase use malloc'd memory, so this
 * is safe. If this changes in the future, we'd need a new flag or callback
 * to handle different memory ownership models.
 */
void cleanup_event_obj(struct event *event)
{
  struct mud_event_data *mud_event = NULL;

  /* Safety check - don't try to free NULL pointers */
  if (!event || !event->event_obj)
    return;

  if (event->isMudEvent)
  {
    /* Mud events have their own cleanup function that knows
     * how to properly free all the complex data structures */
    mud_event = (struct mud_event_data *)event->event_obj;
    free_mud_event(mud_event);
  }
  else
  {
    /* ASSUMPTION: Non-mud events always have malloc'd event_obj.
     * This is currently true for all uses in the codebase.
     * If this assumption changes, we'd need additional flags to
     * track memory ownership (e.g., event->owns_memory flag). */
    free(event->event_obj);
  }
}

/** Process any events whose time has come. Should be called from, and at, every
 * pulse of heartbeat. Re-enqueues multi-use events.
 * 
 * BEGINNERS NOTE: This function runs every game pulse (1/10th second) and
 * executes any events that are scheduled to run now. Events can reschedule
 * themselves by returning a positive value (the delay until next run).
 */
void event_process(void)
{
  struct event *the_event = NULL;
  long new_time = 0;

  /* Safety check: ensure event_q is initialized */
  if (!event_q)
  {
    log("SYSERR: event_process called before event_init()");
    return;  /* No need to clear processing_events - it was never set */
  }
  
  /* Set flag to indicate we're processing events.
   * This prevents dangerous operations like queue_free() during processing. */
  processing_events = 1;

  while ((long)pulse >= queue_key(event_q))
  {

    if (!(the_event = (struct event *)queue_head(event_q)))
    {
      log("SYSERR: Attempt to get a NULL event");
      processing_events = 0;  /* CRITICAL: Must clear flag before returning! */
      return;
    }

    /* Set the_event->q_el to NULL so that any functions called beneath 
     * event_process can tell if they're being called beneath the actual
     * event function. 
     * 
     * IMPORTANT FOR BEGINNERS:
     * Setting q_el to NULL serves as a flag that this event is currently
     * being processed. If event_cancel() is called on this event while
     * it's running, it will see q_el is NULL and know NOT to free the
     * event structure (to prevent double-free). */
    the_event->q_el = NULL;

    /* call event func, reenqueue event if retval > 0 */
    if ((new_time = (the_event->func)(the_event->event_obj)) > 0)
      the_event->q_el = queue_enq(event_q, the_event, new_time + pulse);
    else
    {
      /* CLEANUP NOTE FOR BEGINNERS:
       * If the event canceled itself during execution, event_cancel() will
       * have set event_obj to NULL to prevent double-free. We check for NULL
       * before trying to free mud events. */
      if (the_event->isMudEvent && the_event->event_obj != NULL)
        free_mud_event((struct mud_event_data *)the_event->event_obj);

      /* It is assumed that the_event will already have freed ->event_obj. */
      free(the_event);
    }
  }
  
  /* Clear the processing flag - safe to do bulk operations again */
  processing_events = 0;
}

/** Returns the time remaining before the event as how many pulses from now. 
 * @param event Check this event for it's scheduled activation time. 
 * @retval long Number of pulses before this event will fire. */
long event_time(struct event *event)
{
  long when = 0;

  when = queue_elmt_key(event->q_el);

  return (when - pulse);
}

/** Frees all events from event_q. 
 * WARNING: This function should NEVER be called while event_process() is running!
 * Doing so would cause double-free crashes and memory corruption.
 * 
 * BEGINNERS NOTE: This function is typically only called during shutdown or
 * when completely resetting the event system. During normal gameplay, use
 * event_cancel() to remove individual events safely.
 */
void event_free_all(void)
{
  /* CRITICAL SAFETY CHECK:
   * We must ensure event_process() is not currently running.
   * If it is, we risk freeing events that are being processed,
   * causing crashes when event_process() tries to access them.
   */
  if (processing_events) {
    log("SYSERR: event_free_all() called while events are being processed! Aborting to prevent crash.");
    return;
  }
  
  queue_free(event_q);
}

/** Boolean function to tell whether an event is queued or not. Does this by
 * checking if event->q_el points to anything but null.
 * @retval int 1 if the event has been queued, 0 if the event has not been
 * queued. */
int event_is_queued(struct event *event)
{
  if (!event)
    return 0;

  if (event->q_el)
    return 1;
  else
    return 0;
}
/***************************************************************************
 * End mud specific event queue functions
 **************************************************************************/

/***************************************************************************
 * Begin generic (abstract) priority queue functions
 **************************************************************************/
/** Create a new, empty, priority queue and return it.
 * @retval dg_queue * Pointer to the newly created queue structure. */
struct dg_queue *queue_init(void)
{
  struct dg_queue *q = NULL;
  int i;

  CREATE(q, struct dg_queue, 1);
  
  /* Initialize all head and tail pointers to NULL to prevent valgrind warnings */
  for (i = 0; i < NUM_EVENT_QUEUES; i++)
  {
    q->head[i] = NULL;
    q->tail[i] = NULL;
  }

  return q;
}

/** Add some 'data' to a priority queue. 
 * @pre The paremeter q must have been previously created by queue_init.
 * @post A new q_element is created to hold the data parameter.
 * @param q The existing dg_queue to add an element to. 
 * @param data The data to be associated with, and theoretically used, when
 * the element comes up in q. data is wrapped in a new q_element.
 * @param key Indicates where this event should be located in the queue, and
 * when the element should be activated.
 * @retval q_element * Pointer to the created q_element that contains
 * the data. */
struct q_element *queue_enq(struct dg_queue *q, void *data, long key)
{
  struct q_element *qe = NULL, *i = NULL;
  int bucket = 0;

  /* Safety check for NULL queue */
  if (!q)
  {
    log("SYSERR: queue_enq called with NULL queue");
    return NULL;
  }

  CREATE(qe, struct q_element, 1);
  qe->data = data;
  qe->key = key;
  qe->prev = NULL;  /* Explicitly initialize to prevent valgrind warnings */
  qe->next = NULL;  /* Explicitly initialize to prevent valgrind warnings */

  bucket = key % NUM_EVENT_QUEUES; /* which queue does this go in */

  if (!q->head[bucket])
  { /* queue is empty */
    q->head[bucket] = qe;
    q->tail[bucket] = qe;
  }

  else
  {
    for (i = q->tail[bucket]; i; i = i->prev)
    {

      if (i->key < key)
      { /* found insertion point */
        if (i == q->tail[bucket])
          q->tail[bucket] = qe;
        else
        {
          qe->next = i->next;
          i->next->prev = qe;
        }

        qe->prev = i;
        i->next = qe;
        break;
      }
    }

    if (i == NULL)
    { /* insertion point is front of list */
      qe->next = q->head[bucket];
      q->head[bucket] = qe;
      qe->next->prev = qe;
    }
  }

  return qe;
}

/** Remove queue element qe from the priority queue q.
 * @pre qe->data has been dealt with in some way.
 * @post qe has been freed. 
 * @param q Pointer to the queue containing qe.
 * @param qe Pointer to the q_element to remove from q.
 */
void queue_deq(struct dg_queue *q, struct q_element *qe)
{
  int i = 0;

  /* CRITICAL SAFETY CHECK:
   * Replace assert with proper NULL check for production safety.
   * Assert only works in debug builds - in production (with NDEBUG),
   * the assert disappears and we'd crash on NULL pointer access!
   * 
   * BEGINNERS NOTE: 
   * An 'assert' is a debug-only check that disappears in release builds.
   * We need real error checking that works in all builds.
   */
  if (!qe)
  {
    log("SYSERR: queue_deq called with NULL q_element");
    return;
  }

  /* Safety check for NULL queue */
  if (!q)
  {
    log("SYSERR: queue_deq called with NULL queue");
    return;
  }

  i = qe->key % NUM_EVENT_QUEUES;

  if (qe->prev == NULL)
    q->head[i] = qe->next;
  else
    qe->prev->next = qe->next;

  if (qe->next == NULL)
    q->tail[i] = qe->prev;
  else
    qe->next->prev = qe->prev;

  free(qe);
}

/** Removes and returns the data of the first element of the priority queue q. 
 * @pre pulse must be defined. This is a multi-headed queue, the current
 * head is determined by the current pulse.
 * @post the q->head is dequeued. 
 * @param q The queue to return the head of. 
 * @retval void * NULL if there is not a currently available head, pointer
 * to any data object associated with the queue element. */
void *queue_head(struct dg_queue *q)
{
  void *dg_data = NULL;
  int i = 0;

  /* Safety check for NULL queue */
  if (!q)
    return NULL;

  i = pulse % NUM_EVENT_QUEUES;

  if (!q->head[i])
    return NULL;

  dg_data = q->head[i]->data;
  queue_deq(q, q->head[i]);
  return dg_data;
}

/** Returns the key of the head element of the priority queue.
 * @pre pulse must be defined. This is a multi-headed queue, the current
 * head is determined by the current pulse. 
 * @param q Queue to check for.
 * @retval long Return the key element of the head q_element. If no head
 * q_element is available, return LONG_MAX. */
long queue_key(struct dg_queue *q)
{
  int i = 0;

  /* Safety check for NULL queue */
  if (!q)
    return LONG_MAX;

  i = pulse % NUM_EVENT_QUEUES;

  if (q->head[i])
    return q->head[i]->key;
  else
    return LONG_MAX;
}

/** Returns the key of queue element qe.
 * @param qe Pointer to the keyed q_element. 
 * @retval long Key of qe, or LONG_MAX if qe is NULL.
 * 
 * BEGINNERS NOTE: The 'key' represents when this event should fire,
 * measured in game pulses. Lower keys fire sooner.
 */
long queue_elmt_key(struct q_element *qe)
{
  /* Safety check to prevent NULL pointer dereference */
  if (!qe) {
    log("WARNING: queue_elmt_key called with NULL q_element");
    return LONG_MAX;  /* Return max value to indicate error */
  }
  return qe->key;
}

/** Free q and all contents.
 * @pre Function requires definition of struct event.
 * @post All items associated with q, including non-abstract data, are freed.
 * @param q The priority queue to free.
 * 
 * CRITICAL WARNING FOR BEGINNERS:
 * This function frees ALL events in ALL queue buckets. It should NEVER be
 * called while event_process() is running, as that would cause double-free
 * crashes when event_process() tries to access already-freed memory.
 * 
 * This is typically only called during shutdown or complete system reset.
 * For removing individual events during gameplay, use event_cancel() instead.
 */
void queue_free(struct dg_queue *q)
{
  int i = 0;
  struct q_element *qe = NULL, *next_qe = NULL;
  struct event *event = NULL;
  
  /* Safety check for NULL queue */
  if (!q) {
    log("WARNING: queue_free called with NULL queue");
    return;
  }
  
  /* CRITICAL: Check if we're processing events right now */
  if (processing_events) {
    log("SYSERR: queue_free() called while event_process() is active! This would cause crashes!");
    log("SYSERR: Stack trace or debugging needed - this should never happen!");
    /* We could abort here but that might lose player data. Log and hope for the best. */
    return;
  }

  /* IMPORTANT: We iterate through all queue buckets (0 to NUM_EVENT_QUEUES-1)
   * Events are distributed across buckets based on their scheduled time
   * to improve performance (reduces search time for insertion). */
  for (i = 0; i < NUM_EVENT_QUEUES; i++)
  {
    /* Process each event in this bucket's linked list */
    for (qe = q->head[i]; qe; qe = next_qe)
    {
      /* Save the next pointer BEFORE freeing current element
       * (once freed, qe->next would be invalid memory!) */
      next_qe = qe->next;
      
      /* Extract the event from this queue element */
      if ((event = (struct event *)qe->data) != NULL)
      {
        /* DOUBLE-FREE PREVENTION CHECK:
         * If q_el is NULL, this event might be currently processing.
         * However, since queue_free() should NEVER be called during
         * event_process(), we log an error if we detect this situation. */
        if (!event->q_el) {
          log("SYSERR: queue_free() found event with NULL q_el - possible concurrent processing!");
          /* Continue anyway as we're likely shutting down */
        }
        
        /* Free any associated data with this event */
        if (event->event_obj)
          cleanup_event_obj(event);

        /* Free the event structure itself */
        free(event);
      }
      /* Free the queue element that held this event */
      free(qe);
    }
  }

  /* Finally, free the queue structure itself */
  free(q);
}
