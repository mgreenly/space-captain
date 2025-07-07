#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>

// Log levels - increasing in severity
// 0: NONE  - No logging is performed
// 1: FATAL - Critical failures resulting in immediate termination
// 2: ERROR - Severe issues preventing correct processing
// 3: WARN  - Non-critical issues indicating potential problems
// 4: INFO  - Normal operational status messages (default)
// 5: DEBUG - Detailed diagnostics for debugging sessions
#ifndef LOG_LEVEL
#define LOG_LEVEL 4 // INFO is the default log level
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

#if LOG_LEVEL >= 1
#define log_fatal(fmt, ...) log_base("FATAL", KRED, stderr, fmt, ##__VA_ARGS__)
#else
#define log_fatal(fmt, ...)
#endif

#if LOG_LEVEL >= 2
#define log_error(fmt, ...) log_base("ERROR", KRED, stderr, fmt, ##__VA_ARGS__)
#else
#define log_error(fmt, ...)
#endif

#if LOG_LEVEL >= 3
#define log_warn(fmt, ...) log_base("WARN ", KYEL, stdout, fmt, ##__VA_ARGS__)
#else
#define log_warn(fmt, ...)
#endif

#if LOG_LEVEL >= 4
#define log_info(fmt, ...) log_base("INFO ", KGRN, stdout, fmt, ##__VA_ARGS__)
#else
#define log_info(fmt, ...)
#endif

#if LOG_LEVEL >= 5
#define log_debug(fmt, ...) log_base("DEBUG", KBLU, stdout, fmt, ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

#endif // LOG_H
