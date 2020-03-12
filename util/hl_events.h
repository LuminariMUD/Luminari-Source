/****************************************************************************
 *  Realms of Luminari
 *  File:     hl_events.h
 *  Usage:    alternate event system
 *  Authors:  Eric Green (ejg3@cornell.edu)
 *            Ported to Luminari by Zusuk
 *  Created:  April 13, 2013
 ****************************************************************************/

#ifndef HL_EVENTS_H
#define HL_EVENTS_H

#ifdef __cplusplus
extern "C"
{
#endif

#define EVENTFUNC(name) long(name)(struct event * evet, void *event_obj)

  struct event
  {
    EVENTFUNC(*func);
    void *event_obj;
    struct q_element *q_el;
  };

  /* function protos need by other modules */
  /* events */
  void event_init(void);
  struct event *event_create(EVENTFUNC(*func), void *event_obj, long when);
  void event_cancel(struct event *event);
  void event_process(void);
  long event_time(struct event *event);
  /* queue */
  void event_free_all(void);
  struct queue *queue_init(void);
  struct q_element *queue_enq(struct queue *q, void *data, long key);
  void queue_deq(struct queue *q, struct q_element *qe);
  void *queue_head(struct queue *q);
  long queue_key(struct queue *q);
  long queue_elmt_key(struct q_element *qe);
  void queue_free(struct queue *q);

#ifdef __cplusplus
}
#endif

#endif /* HL_EVENTS_H */
