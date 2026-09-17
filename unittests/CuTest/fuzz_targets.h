/* Production-linked fuzz targets for the runtime trust boundaries.
 *
 * Each target feeds one input through the parser the server itself links, so
 * there is no mirror to drift. The table is compiled into the production-linked
 * CuTest suite (which replays the seed and regression inputs deterministically)
 * and into the libFuzzer entry point in fuzz_game.c. */
#ifndef LUMINARI_FUZZ_TARGETS_H
#define LUMINARI_FUZZ_TARGETS_H

#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>

/* The production loader terminates the process on a rejected file. The libFuzzer
 * entry point turns that exit into a return; the CuTest replay accepts status 1. */
#define FUZZ_TARGET_MAY_EXIT 1u

struct fuzz_target
{
  const char *name;
  const char *description;
  void (*setup)(void);
  int (*run)(const uint8_t *data, size_t size);
  unsigned int flags;
};

const struct fuzz_target *fuzz_target_table(size_t *count);
const struct fuzz_target *fuzz_target_find(const char *name);

/* Set while a FUZZ_TARGET_MAY_EXIT target runs production code; an interposed
 * exit() longjmps back instead of ending the process. */
extern jmp_buf fuzz_exit_jump;
extern volatile sig_atomic_t fuzz_exit_active;
extern volatile sig_atomic_t fuzz_exit_status;

/* Shared with test_i3_client_production.c. */
void i3_test_setup(void);
void i3_test_cleanup(void);

#endif /* LUMINARI_FUZZ_TARGETS_H */
