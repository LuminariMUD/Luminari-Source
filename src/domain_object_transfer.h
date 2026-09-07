#ifndef DOMAIN_OBJECT_TRANSFER_H
#define DOMAIN_OBJECT_TRANSFER_H

#include "structs.h"

struct domain_transfer_context
{
  struct domain_entity_handle actor;
  enum domain_transfer_cause cause;
  struct domain_transfer_context *previous;
};

/* Stack capture around a compound object operation, before mutation.
 * Finish at every exit. Nested operations on the same object fold into the
 * final outer outcome, including extraction. Handles survive holder disposal. */
struct domain_object_transfer_operation
{
  struct domain_object_moved event;
  struct domain_object_transfer_operation *previous;
  bool active;
  bool extracted;
};

void domain_transfer_context_begin(struct domain_transfer_context *context, struct char_data *actor,
                                   enum domain_transfer_cause cause);
void domain_transfer_context_finish(struct domain_transfer_context *context);
struct domain_object_holder domain_object_holder(const struct obj_data *object);
void domain_object_transfer_begin(struct domain_object_transfer_operation *operation,
                                  struct obj_data *object, struct char_data *actor,
                                  enum domain_transfer_cause cause);
void domain_object_transfer_finish(struct domain_object_transfer_operation *operation);
void domain_object_detaching(struct obj_data *object);
void domain_object_placed(struct obj_data *object);
void domain_object_disposed(struct obj_data *object);

#endif /* DOMAIN_OBJECT_TRANSFER_H */
