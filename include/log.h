#ifndef KRZ_LOG_H
#define KRZ_LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL
} log_level_t;

/* Initialize the logger. Safe to call multiple times; only the first
 * call takes effect. If never called, the logger lazily initializes
 * itself writing to stderr at LOG_INFO. Pass NULL to keep writing to
 * stderr. */
void log_init(FILE *stream, log_level_t min_level);

/* Change the minimum level and output stream at runtime. Thread-safe. */
void log_set_level(log_level_t min_level);
void log_set_stream(FILE *stream);

/* Core logging call. Use the LOG_* macros below instead of calling
 * this directly. Thread-safe: a single mutex serializes writes so
 * lines from concurrent threads are never interleaved. */
void log_write(log_level_t level, const char *file, int line,
                const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 4, 5)))
#endif
    ;

#define LOG_DEBUG(...) log_write(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_write(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_write(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_write(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_write(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* KRZ_LOG_H */
