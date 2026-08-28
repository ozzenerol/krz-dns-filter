#define _POSIX_C_SOURCE 200809L

#include "../include/log.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static FILE *g_log_stream = NULL;
static log_level_t g_log_level = LOG_INFO;
static int g_log_initialized = 0;

static const char *level_name(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "?????";
    }
}

static void log_init_defaults_locked(void) {
    if (!g_log_initialized) {
        g_log_stream = stderr;
        g_log_level = LOG_INFO;
        g_log_initialized = 1;
    }
}

void log_init(FILE *stream, log_level_t min_level) {
    pthread_mutex_lock(&g_log_mutex);
    g_log_stream = stream ? stream : stderr;
    g_log_level = min_level;
    g_log_initialized = 1;
    pthread_mutex_unlock(&g_log_mutex);
}

void log_set_level(log_level_t min_level) {
    pthread_mutex_lock(&g_log_mutex);
    log_init_defaults_locked();
    g_log_level = min_level;
    pthread_mutex_unlock(&g_log_mutex);
}

void log_set_stream(FILE *stream) {
    pthread_mutex_lock(&g_log_mutex);
    log_init_defaults_locked();
    g_log_stream = stream ? stream : stderr;
    pthread_mutex_unlock(&g_log_mutex);
}

void log_write(log_level_t level, const char *file, int line,
                const char *fmt, ...) {
    pthread_mutex_lock(&g_log_mutex);

    log_init_defaults_locked();

    if (level < g_log_level) {
        pthread_mutex_unlock(&g_log_mutex);
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);

    char timebuf[32];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);

    /* Trim the path down to the filename to keep lines short. */
    const char *base = strrchr(file, '/');
    base = base ? base + 1 : file;

    fprintf(g_log_stream, "%s.%03ld [%-5s] %s:%d: ",
            timebuf, ts.tv_nsec / 1000000, level_name(level), base, line);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_stream, fmt, args);
    va_end(args);

    fputc('\n', g_log_stream);
    fflush(g_log_stream);

    pthread_mutex_unlock(&g_log_mutex);

    if (level == LOG_FATAL) {
        abort();
    }
}
