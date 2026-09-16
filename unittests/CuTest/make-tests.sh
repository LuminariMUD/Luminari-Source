#!/usr/bin/env bash

# Auto generate single AllTests file for CuTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.
# Author: Asim Jalis
# Date: 01/08/2003
#
# With --prototypes, prints a header declaring every Test function instead.
# CuTest.h includes it in the root suite so test files compiled with
# -Wmissing-prototypes see a previous declaration of each test.

PROTOTYPES=0
if test "$1" = "--prototypes"; then
  PROTOTYPES=1
  shift
fi
if test $# -eq 0; then FILES=*.c; else FILES=$*; fi

if test $PROTOTYPES -eq 1; then
  echo '/* This is auto-generated code by make-tests.sh --prototypes. */'
  echo '#ifndef CUTEST_TEST_PROTOTYPES_H'
  echo '#define CUTEST_TEST_PROTOTYPES_H'
  echo
  cat $FILES | grep '^void Test' |
    sed -e 's/(.*$//' \
      -e 's/$/(CuTest *tc);/'
  echo
  echo '#endif /* CUTEST_TEST_PROTOTYPES_H */'
  exit 0
fi

echo '

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The runner declares the tests itself; see make-tests.sh --prototypes. */
#define CUTEST_RUNNER
#include "CuTest.h"

extern FILE *logfile;

/* Filtering is confined to this generated runner; nested suites still run fully. */
#define ADD_MATCHING_TEST(suite, test) \
    do { \
        registered++; \
        if (filter == NULL || strstr(#test, filter) != NULL) \
            SUITE_ADD_TEST(suite, test); \
    } while (0)
'

# The production-linked suite seeds every random source its tests reach. Quoted
# here-documents keep C character literals intact.
cat <<'EOF'

#ifdef LUMINARI_CUTEST
#include <errno.h>
#include <stdint.h>

void circle_srandom(unsigned long initial_seed);

/* Without LUMINARI_TEST_SEED every run uses this seed, so reruns match. */
static unsigned long cutest_run_seed = 1UL;

/* Before each test, seed the game generator (src/core/random.c) and the C
 * library generator from the run seed and the test name.  A test therefore
 * draws the same values in the full suite and in a filtered replay. */
static void cutest_seed_test(CuTest *tc)
{
    uint64_t hash = UINT64_C(14695981039346656037);
    uint64_t run_seed = (uint64_t)cutest_run_seed;
    unsigned long test_seed;
    const char *name;
    int shift;

    for (shift = 0; shift < 64; shift += 8)
    {
        hash ^= (run_seed >> shift) & UINT64_C(0xff);
        hash *= UINT64_C(1099511628211);
    }
    for (name = tc->name; *name != '\0'; name++)
    {
        hash ^= (uint64_t)(unsigned char)*name;
        hash *= UINT64_C(1099511628211);
    }
    /* The game generator needs a seed in 1..2147483646. */
    test_seed = (unsigned long)(1U + hash % UINT64_C(2147483646));
    circle_srandom(test_seed);
    srand((unsigned int)test_seed);
}

static int cutest_read_seed(void)
{
    const char *text = getenv("LUMINARI_TEST_SEED");
    char *end;
    unsigned long value;

    if (text == NULL || *text == '\0')
        return 1;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (*text < '0' || *text > '9' || errno != 0 || *end != '\0')
    {
        fprintf(stderr, "LUMINARI_TEST_SEED must be a decimal integer, not \"%s\"\n", text);
        return 0;
    }
    cutest_run_seed = value;
    return 1;
}
#endif

EOF

cat $FILES | grep '^void Test' |
  sed -e 's/(.*$//' \
    -e 's/$/(CuTest*);/' \
    -e 's/^/extern /'

echo \
  '

static int RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();
    int fail_count;
    const char *filter = getenv("CUTEST_FILTER");
    int registered = 0;

#ifdef LUMINARI_CUTEST
    if (!cutest_read_seed())
    {
        CuStringDelete(output);
        CuSuiteDelete(suite);
        return 1;
    }
    CuTestSetUp = cutest_seed_test;
#endif
'
cat $FILES | grep '^void Test' |
  sed -e 's/^void //' \
    -e 's/(.*$//' \
    -e 's/^/    ADD_MATCHING_TEST(suite, /' \
    -e 's/$/);/'

echo \
  '
    if (suite->count == 0)
    {
        fprintf(stderr, "No tests matched CUTEST_FILTER=%s\n", filter ? filter : "");
        CuStringDelete(output);
        CuSuiteDelete(suite);
        return 1;
    }
    if (filter != NULL && *filter != '\0')
        printf("CUTEST_FILTER=%s: %d of %d tests selected\n", filter, suite->count, registered);
#ifdef LUMINARI_CUTEST
    printf("LUMINARI_TEST_SEED=%lu\n", cutest_run_seed);
#endif
    /* Forked test children must not inherit and repeat buffered lines. */
    fflush(stdout);
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    fail_count = suite->failCount;
#ifdef LUMINARI_CUTEST
    if (fail_count > 0)
        printf("Replay a failure with LUMINARI_TEST_SEED=%lu CUTEST_FILTER=<test> ./cutest\n",
               cutest_run_seed);
#endif
    CuStringDelete(output);
    CuSuiteDelete(suite);
    return fail_count;
}

int main(void)
{
    logfile = stderr;
    return RunAllTests();
}
'
