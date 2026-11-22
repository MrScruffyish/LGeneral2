#ifndef LGTRACE_H
#define LGTRACE_H

/*
 * Opt-in tracing for the LGeneral build. Included with -include and enabled
 * with -DLG_TRACE by "make TRACE=1". Output goes to /tmp/human.log unless
 * LG_TRACE_FILE points somewhere else.
 */
#ifdef LG_TRACE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

inline void lgtrace_header(FILE *fp)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        fputs("[no-clock] ", fp);
        return;
    }
    fprintf(fp, "[%llu.%09ld] ",
            static_cast<unsigned long long>(ts.tv_sec), ts.tv_nsec);
}

inline void lgtrace(const char *fmt, ...)
{
    static FILE *lgtrace_fp = NULL;
    va_list ap;

    if (lgtrace_fp == NULL) {
        const char *path = getenv("LG_TRACE_FILE");
        if (path == NULL || *path == '\0')
            path = "/tmp/human.log";
        lgtrace_fp = fopen(path, "a");
        if (lgtrace_fp == NULL)
            return;
        setvbuf(lgtrace_fp, NULL, _IOLBF, 0);
        fputc('\n', lgtrace_fp);
        lgtrace_header(lgtrace_fp);
        fputs("=== trace session ===\n", lgtrace_fp);
    }
    va_start(ap, fmt);
    lgtrace_header(lgtrace_fp);
    vfprintf(lgtrace_fp, fmt, ap);
    va_end(ap);
}

#else

#define lgtrace(...) ((void)0)

#endif /* LG_TRACE */
#endif /* LGTRACE_H */
