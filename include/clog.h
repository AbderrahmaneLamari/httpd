#ifndef CLOG_H
#define CLOG_H

#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Log Levels */
typedef enum {
  CLOG_TRACE = 0,
  CLOG_DEBUG,
  CLOG_INFO,
  CLOG_WARN,
  CLOG_ERROR,
  CLOG_FATAL
} clog_level_t;

/* Color modes */
typedef enum {
  CLOG_COLOR_AUTO = 0,
  CLOG_COLOR_NEVER,
  CLOG_COLOR_ALWAYS,
  CLOG_COLOR_ANSI,
  CLOG_COLOR_WIN32
} clog_color_mode_t;

/* Configuration API */
void clog_set_level(clog_level_t level);
void clog_set_color_mode(clog_color_mode_t mode);
void clog_set_output(FILE *fp);
void clog_set_show_timestamp(bool show);
void clog_set_show_location(bool show);

/* Logging API */
void clog_log(clog_level_t level,
              const char *file,
              int line,
              const char *func,
              const char *format,
              ...);

/* Convenience macros */
#define TRACE(...) clog_log(CLOG_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUG(...) clog_log(CLOG_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define INFO(...)  clog_log(CLOG_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define WARN(...)  clog_log(CLOG_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define ERROR(...) clog_log(CLOG_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define FATAL(...) clog_log(CLOG_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* CLOG_H */
