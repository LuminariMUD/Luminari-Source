#ifndef LUMINARI_PERSISTENCE_H
#define LUMINARI_PERSISTENCE_H

/* Result from one bounded persistence-scheduler step. */
enum persistence_step_result
{
  PERSISTENCE_STEP_FAILURE = -1,
  PERSISTENCE_STEP_IDLE = 0,
  PERSISTENCE_STEP_PROGRESS = 1,
  PERSISTENCE_STEP_COMPLETE = 2
};

#endif /* LUMINARI_PERSISTENCE_H */
