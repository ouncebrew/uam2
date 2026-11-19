#include "uam2.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void uam2_logf(struct uam2_log* log, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);
	if (len <= 0)
		return;

	size_t need = log->len + len + 1;
	if (need > log->cap)
	{
		size_t cap = log->cap ? log->cap : 256;
		while (cap < need)
			cap *= 2;
		char* buf = realloc(log->buf, cap);
		if (!buf)
			return;
		log->buf = buf;
		log->cap = cap;
	}

	va_start(args, fmt);
	vsnprintf(log->buf + log->len, log->cap - log->len, fmt, args);
	va_end(args);
	log->len += len;
}
