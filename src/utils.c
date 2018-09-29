#include <stdio.h>
#include <stdarg.h>

void report_error(int line, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[ERROR]: ");
	if(line != -1)
		fprintf(stderr, "Line %d: ", line);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
