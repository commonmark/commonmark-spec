#ifndef CMARK_BENCH_H
#define CMARK_BENCH_H

#include <stdio.h>
#include <time.h>

#ifdef TIMER
float _cmark_start_time;
float _cmark_end_time;
float _cmark_save_time;

#define start_timer() \
	_cmark_save_time = _cmark_start_time; \
	_cmark_start_time = (float)clock()/CLOCKS_PER_SEC

#define end_timer(M) \
	_cmark_end_time = (float)clock()/CLOCKS_PER_SEC; \
	fprintf(stderr, "[TIME] (%s:%d) %4.f ns " M "\n", __FILE__, \
		__LINE__, (_cmark_end_time - _cmark_start_time) * 1000000); \
	_cmark_start_time = _cmark_save_time;

#else
#define start_timer()
#define end_timer(M)
#endif

#endif
