#ifndef XG_QUERY_TRACE_H
#define XG_QUERY_TRACE_H
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* XG_TRACE_QUERIES=1: observe returned guest query values, never request extra
 * results, wait for the GPU, consume GL errors or change a query. At most 180
 * one-second summaries. Counts are API reads, not distinct tests or pixels.
 * xg_log is supplied by the host. No graphics API calls or dependency here. */
static inline void xg_query_trace(int begin, unsigned id, unsigned name, unsigned value)
{
	static int enabled = -1, rows;
	static time_t next;
	static unsigned begins, ready, pending, zero, one, other, target;
	time_t now;
	if (enabled < 0) {
		const char *option = getenv("XG_TRACE_QUERIES");
		enabled = option && !strcmp(option, "1");
	}
	if (!enabled || rows >= 180) return;
	if (begin) { begins++; target = name; }
	else if (name == 0x8867 /* GL_QUERY_RESULT_AVAILABLE */) {
		if (value) ready++; else pending++;
	}
	else if (name == 0x8866 /* GL_QUERY_RESULT */) {
		if (!value) zero++; else if (value == 1) one++; else other++;
	}
	else return;
	now = time(NULL);
	if (!next) next = now + 1;
	if (now < next) return;
	xg_log("query reads: time %lld last id %u target 0x%x begins %u ready %u pending %u result-zero %u result-one %u result-other %u",
		(long long)now, id, target, begins, ready, pending, zero, one, other);
	begins = ready = pending = zero = one = other = 0;
	next = now + 1;
	rows++;
}
#endif
