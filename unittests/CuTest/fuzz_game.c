/* libFuzzer entry point for the production-linked fuzz targets.
 *
 * Build: ./configure CC=clang CFLAGS='-O1 -g -fsanitize=fuzzer-no-link,address,undefined
 *        -fno-omit-frame-pointer' LDFLAGS='-fsanitize=address,undefined' && make luminari_fuzz
 * Run:   LUMINARI_FUZZ_TARGET=<name> ./luminari_fuzz [libFuzzer options] <corpus dir>...
 * The target names come from unittests/CuTest/test_fuzz_targets.c; the driver
 * scripts/ci/run_fuzz_targets.sh runs every one with its seeds and dictionary. */
#include "fuzz_targets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *logfile;

int LLVMFuzzerInitialize(int *argc, char ***argv);
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static const struct fuzz_target *fuzz_selected_target;

static void fuzz_usage(void)
{
  const struct fuzz_target *targets;
  size_t count;
  size_t index;

  targets = fuzz_target_table(&count);
  fprintf(stderr, "Set LUMINARI_FUZZ_TARGET to one of:\n");
  for (index = 0; index < count; index++)
    fprintf(stderr, "  %-11s %s\n", targets[index].name, targets[index].description);
}

int LLVMFuzzerInitialize(int *argc, char ***argv)
{
  const char *name;

  (void)argc;
  (void)argv;
  name = getenv("LUMINARI_FUZZ_TARGET");
  fuzz_selected_target = fuzz_target_find(name);
  if (fuzz_selected_target == NULL)
  {
    fuzz_usage();
    exit(2);
  }
  /* Parser diagnostics are production log lines; they would otherwise flood
   * the fuzzer's own output at every rejected input. */
  logfile = fopen("/dev/null", "w");
  fuzz_selected_target->setup();
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
  return fuzz_selected_target->run(data, size);
}
