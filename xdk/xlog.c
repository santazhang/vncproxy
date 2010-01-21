#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "xlog.h"
#include "xoption.h"
#include "xutils.h"

static xbool xlog_initialized = XFALSE;

static int log_level = 3;  // default, [info]
static const char* log_dir = "log";
static long log_max_size = 10 * 1024 * 1024;  // 10M
static int log_history = 5;
static xbool log_screenonly = XFALSE;
static FILE* log_fp = NULL;

static pthread_mutex_t logging_lock = PTHREAD_MUTEX_INITIALIZER;

xsuccess xlog_init(int argc, char* argv[]) {
  xsuccess ret = XSUCCESS;
  xoption xopt = xoption_new();
  xoption_parse(xopt, argc, argv);

  if (xoption_has(xopt, "log-level")) {
//    printf("log-level given: %s\n", xoption_get(xopt, "log-level"));
    log_level = atoi(xoption_get(xopt, "log-level"));
  }

  if (xoption_has(xopt, "log-dir")) {
//    printf("log-dir given: %s\n", xoption_get(xopt, "log-dir"));
    log_dir = xoption_get(xopt, "log-dir");
    
  }

  if (xoption_has(xopt, "log-maxsize")) {
//    printf("log-maxsize given: %s\n", xoption_get(xopt, "log-maxsize"));
    log_max_size = xfilesystem_parse_filesize(xoption_get(xopt, "log-maxsize"));
    if (log_max_size < 10 * 1024) {
      log_max_size = 10 * 1024; // lower limit is 10K
    }
  }

  if (xoption_has(xopt, "log-history")) {
//    printf("log-history given: %s\n", xoption_get(xopt, "log-history"));
    log_history = atoi(xoption_get(xopt, "log-history"));
    if (log_history < 0) {
      log_history = 0;
    }
    if (log_history > 100) {
      log_history = 100;
    }
    // hard limit is [0, 100]
  }

  if (xoption_has(xopt, "log-screenonly")) {
//    printf("log-screenonly given\n");
    log_screenonly = XTRUE;
  }

  xoption_delete(xopt);
  if (ret == XSUCCESS) {
    xlog_initialized = XTRUE;
  }
  return ret;
}

static void do_log(const char* cstr) {
  printf("%s", cstr);
  if (log_screenonly != XTRUE) {
    fprintf(log_fp, "%s", cstr);
  }
}

void xlog_start_record(int level) {
  // prevent 2 thread from logging concurrently, which will mess up the log file
  pthread_mutex_lock(&logging_lock);
  if (xlog_initialized == XFALSE) {
    // must print something on screen
    fprintf(stderr, "[error] cannot do log: logging system not initialized!\n");
  } else if (level <= log_level) {
    char timestr_buf[32];
    struct tm* tm_struct;
    time_t tm_val = time(NULL);
    tm_struct = localtime(&tm_val);
    strftime(timestr_buf, sizeof(timestr_buf), "[%b %d %H:%M:%S] ", tm_struct);
    do_log(timestr_buf);
    // print time
    // log format: Time: [level] blah\n
    // make sure no 2 logs got written in one line
    switch (level) {
    case XLOG_FATAL:
      do_log("[fatal] ");
      break;
    case XLOG_ERROR:
      do_log("[error] ");
      break;
    case XLOG_WARNING:
      do_log("[warning] ");
      break;
    case XLOG_INFO:
      do_log("[info] ");
      break;
    case XLOG_DEBUG:
      do_log("[debug] ");
      break;
    case XLOG_LEVEL5:
      do_log("[level5] ");
      break;
    case XLOG_LEVEL6:
      do_log("[level6] ");
      break;
    case XLOG_LEVEL7:
      do_log("[level7] ");
      break;
    }
  } else {
    fprintf(stderr, "[error] no such logging level: %d\n", level);
  }
}


void xlog_end_record(const char* fmt) {
  // if no new line in fmt, append a new line
  if (fmt[strlen(fmt) - 1] != '\n') {
    do_log("\n");
  }
  pthread_mutex_unlock(&logging_lock);
}

FILE* xlog_get_log_fp() {
  return log_fp;
}

xbool xlog_is_screen_only() {
  return log_screenonly;
}

