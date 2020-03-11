#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* http://www.jera.com/techinfo/jtns/jtn002.html */
#define mu_assert(message, test) do { if (!(test)) {puts(message); exit(1);}} while (0)

void mu_equals(const char* expect, const char* actual)
{
  if (strcmp(expect, actual))
  {
    printf("Expected: %s -- got %s\n", expect, actual);
    exit(1);
  }
}
