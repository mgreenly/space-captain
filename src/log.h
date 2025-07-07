#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>

// Define log level constants
#define LOG_LEVEL_NONE  0
#define LOG_LEVEL_FATAL 1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_INFO  4
#define LOG_LEVEL_DEBUG 5

// If LOG_LEVEL is not already defined by a .c file, set a default.
// This must come AFTER the level constants are defined.
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#define LOG_TIME_FORMAT      "%Y-%m-%d %H:%M:%S"
#define LOG_TIME_BUFFER_SIZE 20

// ANSI color codes
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"

static inline void log_timestamp(FILE *stream) {
  char time_buf[LOG_TIME_BUFFER_SIZE];
  time_t now   = time(NULL);
  struct tm *t = localtime(&now);
  strftime(time_buf, sizeof(time_buf), LOG_TIME_FORMAT, t);
  fprintf(stream, "%s ", time_buf);
}

#define log_base(level, color, stream, fmt, ...)                                                   \
  do {                                                                                             \
    log_timestamp(stream);                                                                         \
    fprintf(stream, "%s" level ": " KNRM fmt "\n", color, ##__VA_ARGS__);                          \
  } while (0)

#if LOG_LEVEL >= LOG_LEVEL_FATAL
#define log_fatal(fmt, ...) log_base("FATAL", KRED, stderr, fmt, ##__VA_ARGS__)
#else
#define log_fatal(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define log_error(fmt, ...) log_base("ERROR", KRED, stderr, fmt, ##__VA_ARGS__)
#else
#define log_error(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARN
#define log_warn(fmt, ...) log_base("WARN ", KYEL, stdout, fmt, ##__VA_ARGS__)
#else
#define log_warn(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define log_info(fmt, ...) log_base("INFO ", KGRN, stdout, fmt, ##__VA_ARGS__)
#else
#define log_info(fmt, ...)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define log_debug(fmt, ...) log_base("DEBUG", KBLU, stdout, fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

#endif // LOG_H
