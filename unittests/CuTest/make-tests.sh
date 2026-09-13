#!/usr/bin/env bash

# Auto generate single AllTests file for CuTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.
# Author: Asim Jalis
# Date: 01/08/2003

if test $# -eq 0 ; then FILES=*.c ; else FILES=$* ; fi

echo '

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

cat $FILES | grep '^void Test' |
    sed -e 's/(.*$//' \
        -e 's/$/(CuTest*);/' \
        -e 's/^/extern /'

echo \
'

int RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();
    int fail_count;
    const char *filter = getenv("CUTEST_FILTER");
    int registered = 0;

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
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    fail_count = suite->failCount;
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
