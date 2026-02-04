#include "../include/clog.h"

#include <string.h>
#include <time.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#define getpid() _getpid()
#else
#include <unistd.h>
#endif

/* ===================== */
/* Internal global state */
/* ===================== */

static atomic_bool clog_is_initialized = false;
static clog_level_t clog_min_level = CLOG_TRACE;
static clog_color_mode_t clog_color_mode = CLOG_COLOR_AUTO;
static FILE *clog_output = NULL;
static bool clog_show_timestamp = true;
static bool clog_show_location = true;

/* Mutex */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static atomic_flag clog_mutex = ATOMIC_FLAG_INIT;
#define LOCK()   while (atomic_flag_test_and_set(&clog_mutex)) {}
#define UNLOCK() atomic_flag_clear(&clog_mutex)
#else
#define LOCK()
#define UNLOCK()
#endif

/* ===================== */
/* Internal helpers      */
/* ===================== */

static void clog_cleanup_lamari(void) {
  atomic_store(&clog_is_initialized, false);
}
static void clog_init(void) {
  bool expected = false;
  if (atomic_compare_exchange_strong(&clog_is_initialized, &expected, true)) {
    atexit(clog_cleanup_lamari);
  }
}


static const char *clog_level_string(clog_level_t level) {
  switch (level) {
  case CLOG_TRACE: return "TRACE";
  case CLOG_DEBUG: return "DEBUG";
  case CLOG_INFO:  return "INFO";
  case CLOG_WARN:  return "WARN";
  case CLOG_ERROR: return "ERROR";
  case CLOG_FATAL: return "FATAL";
  default:         return "UNKN";
  }
}

static void clog_format_time(char *buf, size_t sz) {
  time_t t = time(NULL);
  struct tm tm;
#ifdef _WIN32
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tm);
}

/* ===================== */
/* Public API            */
/* ===================== */

void clog_set_level(clog_level_t level) {
  clog_min_level = level;
}

void clog_set_color_mode(clog_color_mode_t mode) {
  clog_color_mode = mode;
}

void clog_set_output(FILE *fp) {
  clog_output = fp;
}

void clog_set_show_timestamp(bool show) {
  clog_show_timestamp = show;
}

void clog_set_show_location(bool show) {
  clog_show_location = show;
}

void clog_log(clog_level_t level,
              const char *file,
              int line,
              const char *func,
              const char *format,
              ...) {
  if (level < clog_min_level)
    return;

  if (!atomic_load(&clog_is_initialized))
    clog_init();

  LOCK();

  FILE *out = clog_output ? clog_output : stdout;

  if (clog_show_timestamp) {
    char tbuf[32];
    clog_format_time(tbuf, sizeof(tbuf));
    fprintf(out, "%s ", tbuf);
  }

  fprintf(out, "[%s] ", clog_level_string(level));

  va_list args;
  va_start(args, format);
  vfprintf(out, format, args);
  va_end(args);

  if (clog_show_location && file && func) {
    fprintf(out, " (%s:%d %s)", file, line, func);
  }

  fputc('\n', out);
  fflush(out);

  UNLOCK();
}
