/*
 * procmgr LD_PRELOADs liblibm.so, which hooks printf/puts/fwrite and friends
 * and swallows their output. Log with write(2) instead, to stderr and to
 * <program>.log in the current directory.
 */
#pragma once

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int nc4log_fd = -1;

static inline void nc4log_open(const char *path)
{
    nc4log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

__attribute__((format(printf, 1, 2)))
static inline void nc4log(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n > (int) sizeof(buf) - 2)
        n = sizeof(buf) - 2;
    if (n == 0 || buf[n - 1] != '\n')
        buf[n++] = '\n';
    if (write(STDERR_FILENO, buf, n) < 0) {
        /* nothing to do */
    }
    if (nc4log_fd >= 0 && write(nc4log_fd, buf, n) < 0) {
        /* nothing to do */
    }
}
