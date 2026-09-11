/*
 * Hot-parser microbenchmark for the telnet/MSDP/GMCP protocol layer.
 *
 * Reuses the synthetic fuzz harness (its descriptor setup, stubs, and the
 * chunked input/output driver) so the benchmark exercises exactly the paths
 * the parser tests and fuzzer cover.  Build it with the flags under
 * evaluation (see PROTOCOL_BENCH_CFLAGS in the Makefile) and compare the
 * median nanoseconds per iteration between profiles.  The live game-loop
 * benchmark remains the event-core performance gate; this target isolates
 * parser cost from world, database, and scheduler effects.
 */

#include "fuzz_protocol_parser.c"

#include <errno.h>
#include <stdlib.h>
#include <time.h>

#define BENCH_MAX_INPUTS 32
#define BENCH_MAX_INPUT_SIZE 16384
#define BENCH_DEFAULT_ITERATIONS 2000
#define BENCH_ROUNDS 5

struct bench_input
{
  const char *name;
  uint8_t data[BENCH_MAX_INPUT_SIZE];
  size_t size;
};

static struct bench_input bench_inputs[BENCH_MAX_INPUTS];
static size_t bench_input_count;

static int load_input(const char *path)
{
  FILE *file;
  struct bench_input *input;

  if (bench_input_count >= BENCH_MAX_INPUTS)
    return -1;

  file = fopen(path, "rb");
  if (file == NULL)
    return -1;

  input = &bench_inputs[bench_input_count];
  input->name = path;
  input->size = fread(input->data, 1, sizeof(input->data), file);
  fclose(file);

  if (input->size == 0)
    return -1;

  bench_input_count++;
  return 0;
}

static long long elapsed_ns(const struct timespec *start, const struct timespec *end)
{
  return (long long)(end->tv_sec - start->tv_sec) * 1000000000LL +
         (long long)(end->tv_nsec - start->tv_nsec);
}

static int compare_ns(const void *left, const void *right)
{
  long long a = *(const long long *)left;
  long long b = *(const long long *)right;

  return (a > b) - (a < b);
}

/* Drive every loaded input through each harness mode (whole, split, small
 * chunks, and output parsing) once per iteration. */
static void run_iteration(void)
{
  uint8_t buffer[BENCH_MAX_INPUT_SIZE + 1];
  size_t index;
  unsigned mode;

  for (index = 0; index < bench_input_count; index++)
  {
    for (mode = 0; mode < 4; mode++)
    {
      buffer[0] = (uint8_t)mode;
      memcpy(buffer + 1, bench_inputs[index].data, bench_inputs[index].size);
      LLVMFuzzerTestOneInput(buffer, bench_inputs[index].size + 1);
    }
  }
}

int main(int argc, char **argv)
{
  long long rounds[BENCH_ROUNDS];
  struct timespec start;
  struct timespec end;
  long iterations = BENCH_DEFAULT_ITERATIONS;
  const char *iterations_env;
  char *end_ptr;
  long iteration;
  int round;
  int arg;

  if (argc < 2)
  {
    fprintf(stderr, "usage: %s INPUT_FILE...\n", argv[0]);
    return 2;
  }

  iterations_env = getenv("PROTOCOL_BENCH_ITERATIONS");
  if (iterations_env != NULL && *iterations_env != '\0')
  {
    errno = 0;
    iterations = strtol(iterations_env, &end_ptr, 10);
    if (errno != 0 || *end_ptr != '\0' || iterations <= 0)
    {
      fprintf(stderr, "invalid PROTOCOL_BENCH_ITERATIONS: %s\n", iterations_env);
      return 2;
    }
  }

  for (arg = 1; arg < argc; arg++)
  {
    if (load_input(argv[arg]) != 0)
    {
      fprintf(stderr, "cannot load benchmark input: %s\n", argv[arg]);
      return 2;
    }
  }

  /* Warm up allocator and caches before timing. */
  for (iteration = 0; iteration < iterations / 10 + 1; iteration++)
    run_iteration();

  for (round = 0; round < BENCH_ROUNDS; round++)
  {
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (iteration = 0; iteration < iterations; iteration++)
      run_iteration();
    clock_gettime(CLOCK_MONOTONIC, &end);
    rounds[round] = elapsed_ns(&start, &end) / iterations;
  }

  qsort(rounds, BENCH_ROUNDS, sizeof(rounds[0]), compare_ns);
  printf("protocol-parser-bench inputs=%zu iterations=%ld rounds=%d "
         "median_ns_per_iteration=%lld min_ns_per_iteration=%lld max_ns_per_iteration=%lld\n",
         bench_input_count, iterations, BENCH_ROUNDS, rounds[BENCH_ROUNDS / 2], rounds[0],
         rounds[BENCH_ROUNDS - 1]);
  return 0;
}
