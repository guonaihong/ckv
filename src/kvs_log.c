#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <math.h>
#include "kvs_log.h"

kvs_log_t *kvs_log_new(int level, char *prog, int flags) {
    kvs_log_t *log;
    log = (kvs_log_t *)malloc(sizeof(kvs_log_t));
    if (log == NULL)
        return log;

    if (flags == KVS_LOG_ATOMIC) {
        if (pthread_mutex_init(&log->lock, NULL) != 0) {
            free(log);
            return NULL;
        }
    }
    log->level = level;
    log->prog  = prog != NULL ? strdup(prog) : NULL;
    log->flags = flags;
    return log;
}

static int kvs_log_time_add(char *time, int time_size) {
    struct timeval tv;
    struct tm result;
    int    pos, millisec;

    if (gettimeofday(&tv, NULL) != 0) {
        return errno;
    }

    /* Round to nearest millisec */
    millisec = lrint(tv.tv_usec/1000.0);
    /* Allow for rounding up to nearest second */
    if (millisec >= 1000) {
        millisec -= 1000;
        tv.tv_sec++;
    }

    localtime_r(&tv.tv_sec, &result);

    /* YYYY-MM-DD hh:mm:ss */
    pos = strftime(time, time_size, "%Y-%m-%d %H:%M:%S", &result);
    snprintf(time + pos, time_size - pos, ".%03d", millisec);
    return 0;
}

static const char *kvs_log_level2str(int level) {
    switch(level) {
        case KVS_DEBUG:
            return "DEBUG";
        case KVS_INFO:
            return "INFO";
        case KVS_WARN:
            return "WARN";
        case KVS_ERROR:
            return "ERROR";
    }
    return "UNKNOWN";
}

int kvs_log(kvs_log_t *log, int level, const char *fmt, ...) {
    int n;

    char buffer[BUF_SIZE] = "";
    char title[64] = "";
    char *p = buffer;

    int buffer_size = BUF_SIZE;
    int title_size  = 0;
    va_list ap;

    if (level < log->level) {
        return -1;
    }

    kvs_log_time_add(title, sizeof(title));

    for (;;) {
        va_start(ap, fmt);

        if (log->prog) {
            title_size = snprintf(p, buffer_size, "[%s] [%s] [%-5s] ",
                         log->prog, title, kvs_log_level2str(level));
        } else {
            title_size = snprintf(p, buffer_size, "[%s] [%-5s] ", title,
                         kvs_log_level2str(level));
        }

        n = vsnprintf(p + title_size, buffer_size - title_size, fmt, ap);

        va_end(ap);

        if (n + title_size < buffer_size) {
            break;
        }
        buffer_size = n + title_size + 1;
        p = (char *)malloc(buffer_size);
    }

    if (log->flags == KVS_LOG_ATOMIC) {
        pthread_mutex_lock(&log->lock);
    }

    write(1, p, n + title_size);
    if (log->flags == KVS_LOG_ATOMIC) {
        pthread_mutex_unlock(&log->lock);
    }

    if (p != buffer) {
        free(p);
    }

    return 0;
}

void kvs_log_free(kvs_log_t *log) {
    if (log == NULL)
        return ;
    if (log->flags == KVS_LOG_ATOMIC) {
        pthread_mutex_destroy(&log->lock);
    }
    free(log->prog);
    free(log);
}
