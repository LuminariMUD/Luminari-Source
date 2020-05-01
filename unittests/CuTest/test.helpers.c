#include "CuTest.h"

#include "../../helpers.h"

#include <stdio.h>


void Test_one_argument(CuTest *tc)
{
	// NULL argument
	{
		char *inp = NULL;
		char outp[8] = "nada";

		char *exp_res = NULL;
		const char * const exp_outp = "";
		char * res = one_argument(inp, outp);

		CuAssertPtrEquals(tc, exp_res, res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Simple cases
	{
		char inp[] = "HeLLo WOrld bongo";
		char outp[8]= "nada";

		char *exp_res = inp + 5;
		const char * const exp_outp = "hello";
		char *res = one_argument(inp, outp);

		CuAssertPtrEquals(tc, exp_res, res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	{
		char inp[] = "  taCos";
		char outp[8] = "nada";

		char *exp_res = inp + 7;
		const char * const exp_outp = "tacos";
		char * res = one_argument(inp, outp);

		CuAssertPtrEquals(tc, exp_res, res);
		CuAssertStrEquals(tc, exp_outp, outp);	
	}

	// Preceding spaces are trimmed
	{
		char inp[] = "     HeLLo world";
		char outp[8] = "nada";

		char *exp_res = inp + 10;
		const char * const exp_outp = "hello";
		char *res = one_argument(inp, outp);

		CuAssertPtrEquals(tc, exp_res, res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Leading fill words are skipped
	{
		char inp[] = "    TO    At oN  ThE  With FROM In   Hello World";
		char outp[8] = "nada";

		char *exp_res = inp + 42;
		const char * const exp_outp = "hello";
		char *res = one_argument(inp, outp);

		CuAssertPtrEquals(tc, exp_res, res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}
}

void Test_one_argument_c(CuTest *tc)
{
	// NULL argument
	{
		const char *inp = NULL;
		char outp[8] = "nada";

		const char *exp_res = NULL;
		const char * const exp_outp = "";
		const char * res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Simple cases
	{
		const char inp[] = "HeLLo WOrld bongo";
		char outp[8]= "nada";

		const char *exp_res = inp + 5;
		const char * const exp_outp = "hello";
		const char *res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	{
		const char inp[] = "  taCos";
		char outp[8] = "nada";

		const char *exp_res = inp + 7;
		const char * const exp_outp = "tacos";
		const char * res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);	
	}

	// Preceding spaces are trimmed
	{
		const char inp[] = "     HeLLo world";
		char outp[8] = "nada";

		const char *exp_res = inp + 10;
		const char * const exp_outp = "hello";
		const char *res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Leading fill words are skipped
	{
		const char inp[] = "    TO    At oN  ThE  With FROM In   Hello World";
		char outp[8] = "nada";

		const char *exp_res = inp + 42;
		const char * const exp_outp = "hello";
		const char *res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Basic truncation
	{
		const char inp[] = "HeLLoWOrld bongo";
		char outp[8]= "nada";

		const char *exp_res = inp + 10;
		const char * const exp_outp = "hellowo";
		const char *res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}

	// Truncated leading words won't be skipped
	{
		const char inp[] = "    TO    At oN  ThE  With FROM In   Hello World";
		char outp[3] = "na";

		const char *exp_res = inp + 20;
		const char * const exp_outp = "th";
		const char *res = one_argument_c(inp, outp, sizeof(outp));

		CuAssertTrue(tc, exp_res == res);
		CuAssertStrEquals(tc, exp_outp, outp);
	}
}
