#include "CuTest.h"

// #include "../../"
#include "../../bool.h"
#include "../../utils.h"
#include "../../structs.h"
#include "../../interpreter.h"

#include <stdio.h>
#include <stddef.h>


void Test_three_arguments_u(CuTest *tc)
{
    // Simple cases
    {
        char inp[] = "HeLLo WOrld bongo turtles";
        char outp1[8] = "nada";
        char outp2[8] = "nada";
        char outp3[8] = "nada";

        char *exp_res = inp + 17;
        const char * const exp_outp1 = "hello";
        const char * const exp_outp2 = "world";
        const char * const exp_outp3 = "bongo";
        char *res = three_arguments_u(inp, outp1, outp2, outp3);

        CuAssertPtrEquals(tc, exp_res, res);
        CuAssertStrEquals(tc, exp_outp1, outp1);
        CuAssertStrEquals(tc, exp_outp2, outp2);
        CuAssertStrEquals(tc, exp_outp3, outp3);
    }
}

void Test_three_arguments(CuTest *tc)
{
    // Simple cases
    {
        const char * const inp = "HeLLo WOrld bongo turtles";
        char outp1[8] = "nada";
        char outp2[8] = "nada";
        char outp3[8] = "nada";

        const char *exp_res = inp + 17;
        const char * const exp_outp1 = "hello";
        const char * const exp_outp2 = "world";
        const char * const exp_outp3 = "bongo";
        const char *res = three_arguments(inp, outp1, sizeof(outp1), outp2, sizeof(outp2), outp3, sizeof(outp3));

        CuAssertTrue(tc, exp_res == res);
        CuAssertStrEquals(tc, exp_outp1, outp1);
        CuAssertStrEquals(tc, exp_outp2, outp2);
        CuAssertStrEquals(tc, exp_outp3, outp3);
    }
}