#ifndef MUD_EVENT_CALLBACK_H
#define MUD_EVENT_CALLBACK_H

/** Signature used by table-driven MUD event behaviors. */
#define MUD_EVENT_CALLBACK(name) long(name)(void *event_obj __attribute__((unused)))

typedef long (*mud_event_callback_func)(void *event_obj);

#endif /* MUD_EVENT_CALLBACK_H */
