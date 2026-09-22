#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include "CuTest.h"

/*-------------------------------------------------------------------------*
 * CuStr
 *-------------------------------------------------------------------------*/

char *CuStrAlloc(int size)
{
  char *newStr = (char *)malloc(sizeof(char) * (size));
  return newStr;
}

char *CuStrCopy(const char *old)
{
  int len = (int)strlen(old);
  char *newStr = CuStrAlloc(len + 1);
  memcpy(newStr, old, (size_t)len + 1);
  return newStr;
}

#define CU_MUTABLE_ARENA_SIZE (8 * 1024 * 1024)

char *CuMutableString(const char *text)
{
  static char arena[CU_MUTABLE_ARENA_SIZE];
  static size_t used;
  size_t length = strlen(text) + 1;
  char *copy;

  if (length > sizeof(arena) - used)
  {
    fprintf(stderr, "CuMutableString: fixture string arena exhausted\n");
    abort();
  }
  copy = arena + used;
  memcpy(copy, text, length);
  used += length;
  return copy;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

void CuStringInit(CuString *str)
{
  str->length = 0;
  str->size = STRING_MAX;
  str->buffer = (char *)malloc(sizeof(char) * str->size);
  str->buffer[0] = '\0';
}

CuString *CuStringNew(void)
{
  CuString *str = (CuString *)malloc(sizeof(CuString));
  str->length = 0;
  str->size = STRING_MAX;
  str->buffer = (char *)malloc(sizeof(char) * str->size);
  str->buffer[0] = '\0';
  return str;
}

void CuStringDelete(CuString *str)
{
  if (!str)
    return;
  free(str->buffer);
  free(str);
}

void CuStringResize(CuString *str, int newSize)
{
  char *buffer = (char *)realloc(str->buffer, sizeof(char) * newSize);

  if (buffer == NULL)
  {
    fprintf(stderr, "CuStringResize: out of memory\n");
    abort();
  }
  str->buffer = buffer;
  str->size = newSize;
}

void CuStringAppend(CuString *str, const char *text)
{
  int length;

  if (text == NULL)
  {
    text = "NULL";
  }

  length = (int)strlen(text);
  if (str->length + length + 1 >= str->size)
    CuStringResize(str, str->length + length + 1 + STRING_INC);
  memcpy(str->buffer + str->length, text, (size_t)length + 1);
  str->length += length;
}

void CuStringAppendChar(CuString *str, char ch)
{
  char text[2];
  text[0] = ch;
  text[1] = '\0';
  CuStringAppend(str, text);
}

void CuStringAppendFormat(CuString *str, const char *format, ...)
{
  va_list argp;
  char buf[HUGE_STRING_LEN];
  va_start(argp, format);
  vsnprintf(buf, sizeof(buf), format, argp);
  va_end(argp);
  CuStringAppend(str, buf);
}

void CuStringInsert(CuString *str, const char *text, int pos)
{
  int length = (int)strlen(text);
  if (pos > str->length)
    pos = str->length;
  if (str->length + length + 1 >= str->size)
    CuStringResize(str, str->length + length + 1 + STRING_INC);
  memmove(str->buffer + pos + length, str->buffer + pos, (str->length - pos) + 1);
  str->length += length;
  memcpy(str->buffer + pos, text, length);
}

/*-------------------------------------------------------------------------*
 * CuTest
 *-------------------------------------------------------------------------*/

void CuTestInit(CuTest *t, const char *name, TestFunction function)
{
  t->name = CuStrCopy(name);
  t->failed = 0;
  t->ran = 0;
  t->elapsed_seconds = 0.0;
  t->message = NULL;
  t->function = function;
  t->jumpBuf = NULL;
}

CuTest *CuTestNew(const char *name, TestFunction function)
{
  CuTest *tc = CU_ALLOC(CuTest);
  CuTestInit(tc, name, function);
  return tc;
}

void CuTestDelete(CuTest *t)
{
  if (!t)
    return;
  free(t->name);
  free(t);
}

void (*CuTestSetUp)(CuTest *tc) = NULL;

/* libgcov's writer, present only in a coverage build. */
extern void __gcov_dump(void) __attribute__((weak));

void CuTestChildExit(int status)
{
  if (__gcov_dump != NULL)
    __gcov_dump();
  _exit(status);
}

void CuTestRun(CuTest *tc)
{
  jmp_buf buf;
  struct timespec start = {0}, end = {0};

  if (CuTestSetUp != NULL)
    CuTestSetUp(tc);
  clock_gettime(CLOCK_MONOTONIC, &start);
  tc->jumpBuf = &buf;
  if (setjmp(buf) == 0)
  {
    tc->ran = 1;
    (tc->function)(tc);
  }
  tc->jumpBuf = 0;
  clock_gettime(CLOCK_MONOTONIC, &end);
  tc->elapsed_seconds =
      (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1000000000.0;
}

static void CuFailInternal(CuTest *tc, const char *file, int line, CuString *string) CU_NORETURN;

static void CuFailInternal(CuTest *tc, const char *file, int line, CuString *string)
{
  char buf[HUGE_STRING_LEN];

  snprintf(buf, sizeof(buf), "%s:%d: ", file, line);
  CuStringInsert(string, buf, 0);

  tc->failed = 1;
  tc->message = string->buffer;
  if (tc->jumpBuf != 0)
    longjmp(*(tc->jumpBuf), 0);
  /* No runner to jump back to: continuing would run the test with the failed condition. */
  fprintf(stderr, "%s\n", string->buffer);
  abort();
}

void CuFail_Line(CuTest *tc, const char *file, int line, const char *message2, const char *message)
{
  CuString string;

  CuStringInit(&string);
  if (message2 != NULL)
  {
    CuStringAppend(&string, message2);
    CuStringAppend(&string, ": ");
  }
  CuStringAppend(&string, message);
  CuFailInternal(tc, file, line, &string);
}

void CuAssert_Line(CuTest *tc, const char *file, int line, const char *message, int condition)
{
  if (condition)
    return;
  CuFail_Line(tc, file, line, NULL, message);
}

void CuAssertStrEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               const char *expected, const char *actual)
{
  CuString string;
  if ((expected == NULL && actual == NULL) ||
      (expected != NULL && actual != NULL && strcmp(expected, actual) == 0))
  {
    return;
  }

  CuStringInit(&string);
  if (message != NULL)
  {
    CuStringAppend(&string, message);
    CuStringAppend(&string, ": ");
  }
  CuStringAppend(&string, "expected <");
  CuStringAppend(&string, expected);
  CuStringAppend(&string, "> but was <");
  CuStringAppend(&string, actual);
  CuStringAppend(&string, ">");
  CuFailInternal(tc, file, line, &string);
}

void CuAssertIntEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               int expected, int actual)
{
  char buf[STRING_MAX];
  if (expected == actual)
    return;
  snprintf(buf, sizeof(buf), "expected <%d> but was <%d>", expected, actual);
  CuFail_Line(tc, file, line, message, buf);
}

void CuAssertDblEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               double expected, double actual, double delta)
{
  char buf[STRING_MAX];
  if (fabs(expected - actual) <= delta)
    return;
  snprintf(buf, sizeof(buf), "expected <%f> but was <%f>", expected, actual);

  CuFail_Line(tc, file, line, message, buf);
}

void CuAssertPtrEquals_LineMsg(CuTest *tc, const char *file, int line, const char *message,
                               const void *expected, const void *actual)
{
  char buf[STRING_MAX];
  if (expected == actual)
    return;
  snprintf(buf, sizeof(buf), "expected pointer <0x%p> but was <0x%p>", expected, actual);
  CuFail_Line(tc, file, line, message, buf);
}


/*-------------------------------------------------------------------------*
 * CuSuite
 *-------------------------------------------------------------------------*/

void CuSuiteInit(CuSuite *testSuite)
{
  testSuite->count = 0;
  testSuite->failCount = 0;
  memset((void *)testSuite->list, 0, sizeof(testSuite->list));
}

CuSuite *CuSuiteNew(void)
{
  CuSuite *testSuite = CU_ALLOC(CuSuite);
  CuSuiteInit(testSuite);
  return testSuite;
}

void CuSuiteDelete(CuSuite *testSuite)
{
  unsigned int n;
  for (n = 0; n < MAX_TEST_CASES; n++)
  {
    if (testSuite->list[n])
    {
      CuTestDelete(testSuite->list[n]);
    }
  }
  free(testSuite);
}

void CuSuiteAdd(CuSuite *testSuite, CuTest *testCase)
{
  assert(testSuite->count < MAX_TEST_CASES);
  testSuite->list[testSuite->count] = testCase;
  testSuite->count++;
}

void CuSuiteAddSuite(CuSuite *testSuite, CuSuite *testSuite2)
{
  int i;
  for (i = 0; i < testSuite2->count; ++i)
  {
    CuTest *testCase = testSuite2->list[i];
    CuSuiteAdd(testSuite, testCase);
  }
}

void CuSuiteRun(CuSuite *testSuite)
{
  int i;
  for (i = 0; i < testSuite->count; ++i)
  {
    CuTest *testCase = testSuite->list[i];
    CuTestRun(testCase);
    if (testCase->failed)
    {
      testSuite->failCount += 1;
    }
  }
}

void CuSuiteSummary(CuSuite *testSuite, CuString *summary)
{
  int i;
  for (i = 0; i < testSuite->count; ++i)
  {
    CuTest *testCase = testSuite->list[i];
    CuStringAppend(summary, testCase->failed ? "F" : ".");
  }
  CuStringAppend(summary, "\n\n");
}

void CuSuiteDetails(CuSuite *testSuite, CuString *details)
{
  int i;
  int failCount = 0;

  if (testSuite->failCount == 0)
  {
    int passCount = testSuite->count - testSuite->failCount;
    const char *testWord = passCount == 1 ? "test" : "tests";
    CuStringAppendFormat(details, "OK (%d %s)\n", passCount, testWord);
  }
  else
  {
    if (testSuite->failCount == 1)
      CuStringAppend(details, "There was 1 failure:\n");
    else
      CuStringAppendFormat(details, "There were %d failures:\n", testSuite->failCount);

    for (i = 0; i < testSuite->count; ++i)
    {
      CuTest *testCase = testSuite->list[i];
      if (testCase->failed)
      {
        failCount++;
        CuStringAppendFormat(details, "%d) %s: %s\n", failCount, testCase->name, testCase->message);
      }
    }
    CuStringAppend(details, "\n!!!FAILURES!!!\n");

    CuStringAppendFormat(details, "Runs: %d ", testSuite->count);
    CuStringAppendFormat(details, "Passes: %d ", testSuite->count - testSuite->failCount);
    CuStringAppendFormat(details, "Fails: %d\n", testSuite->failCount);
  }

  /* Report wall time, including forked boot tests and waits, without a flaky gate. */
  for (i = 0; i < testSuite->count; ++i)
  {
    CuTest *testCase = testSuite->list[i];
    if (testCase->elapsed_seconds > 1.0)
      CuStringAppendFormat(details, "Slow test: %s (%.3f s)\n", testCase->name,
                           testCase->elapsed_seconds);
  }
}
