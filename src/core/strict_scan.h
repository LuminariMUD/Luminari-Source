/**
 * @file strict_scan.h
 * sscanf and fscanf with numbers that cannot overflow.
 *
 * strict_sscanf() and strict_fscanf() read the same formats as sscanf() and fscanf() and
 * return the same results, except that a number outside the range of its target type is a
 * matching failure: the scan stops there instead of storing an undefined value. They cover
 * the directives the code base uses (see strict_scan.c); the format attribute gives them the
 * compiler's scanf argument checks.
 */
#ifndef STRICT_SCAN_H
#define STRICT_SCAN_H

#include <stdio.h>

int strict_sscanf(const char *input, const char *format, ...) __attribute__((format(scanf, 2, 3)));
int strict_fscanf(FILE *stream, const char *format, ...) __attribute__((format(scanf, 2, 3)));

#endif /* STRICT_SCAN_H */
