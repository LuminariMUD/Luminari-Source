#ifndef _DG_EVENT_INTERNAL_H_
#define _DG_EVENT_INTERNAL_H_

/* Private physical-rollback queue surface. */
#include "dg_event.h"
#include "dg_event_rollback.h"
#include "event_runtime.h"

#define NUM_EVENT_QUEUES 10
#define MAX_EVENTS 262144

struct event;

typedef void (*event_cleanup_func)(struct event *event);

struct event
{
  EVENTFUNC(*func);
  void *event_obj;
  struct q_element *q_el;
  event_cleanup_func cleanup;
  event_handle_cleanup_func handle_cleanup;
  bool cleanup_on_completion;
  event_handle_t handle;
  int profile_index;
  struct event_runtime_handle scheduler_handle;
  enum event_backend_kind backend;
  bool dispatching;
  bool cancel_requested;
  bool callback_terminal;
  struct game_event_owner owner;
  uint64_t debug_id;
  bool debug_registered;
  struct event *debug_previous;
  struct event *debug_next;
};

struct dg_queue
{
  struct q_element *head[NUM_EVENT_QUEUES];
  struct q_element *tail[NUM_EVENT_QUEUES];
};

struct q_element
{
  void *data;
  long key;
  struct q_element *prev;
  struct q_element *next;
};

struct event *event_create_named(EVENTFUNC(*func), void *event_obj, long when,
                                 const char *profile_name);
struct event *event_create_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                              const char *profile_name, event_cleanup_func cleanup);
struct event *event_create_owned_named(EVENTFUNC(*func), void *event_obj, long when,
                                       const char *profile_name, struct game_event_owner owner);
struct event *event_create_owned_named_with_cleanup(EVENTFUNC(*func), void *event_obj, long when,
                                                    const char *profile_name,
                                                    event_cleanup_func cleanup,
                                                    struct game_event_owner owner);

#define event_create(func, event_obj, when) event_create_named((func), (event_obj), (when), #func)
#define event_create_with_cleanup(func, event_obj, when, cleanup)                                  \
  event_create_named_with_cleanup((func), (event_obj), (when), #func, (cleanup))

void event_cancel(struct event *event);
long event_time(struct event *event);
void cleanup_event_obj(struct event *event);
int event_is_queued(struct event *event);

struct dg_queue *queue_init(void);
struct q_element *queue_enq(struct dg_queue *q, void *data, long key);
void queue_deq(struct dg_queue *q, struct q_element *qe);
void *queue_head(struct dg_queue *q);
long queue_key(struct dg_queue *q);
long queue_elmt_key(struct q_element *qe);
void queue_free(struct dg_queue *q);

#endif /* _DG_EVENT_INTERNAL_H_ */
