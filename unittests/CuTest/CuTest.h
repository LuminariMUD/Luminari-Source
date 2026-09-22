#ifndef CU_TEST_H
#define CU_TEST_H

#include <setjmp.h>
#include <stdarg.h>

#define CUTEST_VERSION "CuTest 1.5"

/* CuString */

char *CuStrAlloc(int size);
char *CuStrCopy(const char *old);
/* A writable copy of text for fixtures that store it in a char * field.  Copies
 * live in a fixed arena for the whole run and are never freed, like the string
 * literals they replace. */
char *CuMutableString(const char *text);

#define CU_ALLOC(TYPE) ((TYPE *)malloc(sizeof(TYPE)))

#define HUGE_STRING_LEN 8192
#define STRING_MAX 256
#define STRING_INC 256

typedef struct
{
  int length;
  int size;
  char *buffer;
} CuString;

void CuStringInit(CuString *str);
CuString *CuStringNew(void);
void CuStringRead(CuString *str, const char *path);
void CuStringAppend(CuString *str, const char *text);
void CuStringAppendChar(CuString *str, char ch);
void CuStringAppendFormat(CuString *str, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
void CuStringInsert(CuString *str, const char *text, int pos);
void CuStringResize(CuString *str, int newSize);
void CuStringDelete(CuString *str);

/* CuTest */

typedef struct CuTest CuTest;

typedef void (*TestFunction)(CuTest *);

struct CuTest
{
  char *name;
  TestFunction function;
  int failed;
  int ran;
  double elapsed_seconds;
  const char *message;
  jmp_buf *jumpBuf;
};

void CuTestInit(CuTest *t, const char *name, TestFunction function);
CuTest *CuTestNew(const char *name, TestFunction function);
void CuTestRun(CuTest *tc);
void CuTestDelete(CuTest *t);

/* When set, CuTestRun calls this before each test. The production-linked
 * runner uses it to reseed the random generators (see make-tests.sh). */
extern void (*CuTestSetUp)(CuTest *tc);

/* End a forked test child without running the parent's exit handlers. A
 * coverage build first records what the child executed. */
void CuTestChildExit(int status) __attribute__((noreturn));

/* A failed assertion jumps out of the test (or aborts when no test runner
 * is active), so the code after an assert never runs with the failed
 * condition. The attribute lets compilers and analyzers see that. */
#define CU_NORETURN __attribute__((noreturn))

/* Internal versions of assert functions -- use the public versions */
void CuFail_Line(CuTest *tc, const char *file, int line, const char *message2,
                 const char *message) CU_NORETURN;
void CuAssert_Line(CuTest *tc, const char *file, int line, const char *message, int condition);
void CuAssertStrEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               const char *expected, const char *actual);
void CuAssertIntEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               int expected, int actual);
void CuAssertDblEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               double expected, double actual, double delta);
void CuAssertPtrEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               const void *expected, const void *actual);

/* public assert functions */

#define CuFail(tc, ms) CuFail_Line((tc), __FILE__, __LINE__, NULL, (ms))
/* These call CuFail_Line() directly so the analyzer sees a failed check end the test. */
#define CuAssert(tc, ms, cond)                                                                     \
  ((cond) ? (void)0 : CuFail_Line((tc), __FILE__, __LINE__, NULL, (ms)))
#define CuAssertTrue(tc, cond)                                                                     \
  ((cond) ? (void)0 : CuFail_Line((tc), __FILE__, __LINE__, NULL, "assert failed"))

#define CuAssertStrEquals(tc, ex, ac)                                                              \
  CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertStrEquals_Msg(tc, ms, ex, ac)                                                      \
  CuAssertStrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
#define CuAssertIntEquals(tc, ex, ac)                                                              \
  CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertIntEquals_Msg(tc, ms, ex, ac)                                                      \
  CuAssertIntEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))
#define CuAssertDblEquals(tc, ex, ac, dl)                                                          \
  CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac), (dl))
#define CuAssertDblEquals_Msg(tc, ms, ex, ac, dl)                                                  \
  CuAssertDblEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac), (dl))
#define CuAssertPtrEquals(tc, ex, ac)                                                              \
  CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, NULL, (ex), (ac))
#define CuAssertPtrEquals_Msg(tc, ms, ex, ac)                                                      \
  CuAssertPtrEquals_LineMsg((tc), __FILE__, __LINE__, (ms), (ex), (ac))

#define CuAssertPtrNotNull(tc, p)                                                                  \
  (((p) != NULL) ? (void)0 : CuFail_Line((tc), __FILE__, __LINE__, NULL, "null pointer unexpected"))
#define CuAssertPtrNotNullMsg(tc, msg, p)                                                          \
  (((p) != NULL) ? (void)0 : CuFail_Line((tc), __FILE__, __LINE__, NULL, (msg)))

/* CuSuite */

#define MAX_TEST_CASES 2048

#define SUITE_ADD_TEST(SUITE, TEST) CuSuiteAdd(SUITE, CuTestNew(#TEST, TEST))

typedef struct
{
  int count;
  CuTest *list[MAX_TEST_CASES];
  int failCount;

} CuSuite;


void CuSuiteInit(CuSuite *testSuite);
CuSuite *CuSuiteNew(void);
void CuSuiteDelete(CuSuite *testSuite);
void CuSuiteAdd(CuSuite *testSuite, CuTest *testCase);
void CuSuiteAddSuite(CuSuite *testSuite, CuSuite *testSuite2);
void CuSuiteRun(CuSuite *testSuite);
void CuSuiteSummary(CuSuite *testSuite, CuString *summary);
void CuSuiteDetails(CuSuite *testSuite, CuString *details);

/* The root suite's build generates a prototype for every Test function
 * (make-tests.sh --prototypes) so test files satisfy -Wmissing-prototypes.
 * The generated runner declares the tests itself. */
#if defined(LUMINARI_CUTEST) && !defined(CUTEST_RUNNER)
#include "test_prototypes.h"
#endif

#endif /* CU_TEST_H */
