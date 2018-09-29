#ifndef UTILS_HEADER
#define UTILS_HEADER

#include <stdio.h>
#include <time.h>

/*
#define TIME(code) \
{ \
	clockid_t clock_id; \
	struct timespec timespec; \
	clock_id = CLOCK_MONOTONIC; \
	clock_gettime(clock_id, &timespec); \
	long long start_sec = timespec.tv_sec; \
	long start_nsec = timespec.tv_nsec; \
	code; \
	clock_gettime(clock_id, &timespec); \
	long long end_sec = timespec.tv_sec; \
	long end_nsec = timespec.tv_nsec; \
	double elapsed = (end_sec - start_sec) + (1.0e-9 * (end_nsec - start_nsec)); \
	printf("\nTime elapsed (sec): %g\n", elapsed); \
} \
*/

void report_error(int, char *fmt, ...);

#endif
