/* Tiny test macros. Each failed check prints file, line and both values. */
#ifndef PH_CHECK_H
#define PH_CHECK_H

#include <stdio.h>
#include <string.h>

extern int t_checks, t_failed;

#define CHECK(c) do { \
	t_checks++; \
	if (!(c)) { t_failed++; fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); } \
} while (0)

#define CHECK_EQ(a, b) do { \
	long long ck_a_ = (long long)(a), ck_b_ = (long long)(b); \
	t_checks++; \
	if (ck_a_ != ck_b_) { t_failed++; fprintf(stderr, "%s:%d: %s == %s failed: %lld vs %lld\n", \
		__FILE__, __LINE__, #a, #b, ck_a_, ck_b_); } \
} while (0)

#define CHECK_STR(a, b) do { \
	const char *ck_a_ = (a), *ck_b_ = (b); \
	t_checks++; \
	if (!ck_a_ || !ck_b_ || strcmp(ck_a_, ck_b_) != 0) { t_failed++; \
		fprintf(stderr, "%s:%d: %s == %s failed: \"%s\" vs \"%s\"\n", __FILE__, __LINE__, #a, #b, \
			ck_a_ ? ck_a_ : "(null)", ck_b_ ? ck_b_ : "(null)"); } \
} while (0)

#endif
