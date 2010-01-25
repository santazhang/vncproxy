#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "xlog.h"
#include "xoption.h"
#include "xutils.h"
#include "xsys.h"

static int log_level = 3;  // default, [info]
static const char* log_dir = ".";
static const char* log_name = NULL;  // filename for the logs
static long log_max_size = 10 * 1024 * 1024;  // 10M
static int log_history = 5;
static xbool log_screenonly = XFALSE;
static FILE* log_fp = NULL;

static pthread_mutex_t logging_lock = PTHREAD_MUTEX_INITIALIZER;

static void get_log_name(const char* argv0) {
  int i = 0;
  char sep = xsys_fs_sep_char;
  for (i = strlen(argv0) - 1; i >= 0 && argv0[i] != sep; i--) {
  }
  if (i == 0) {
    log_name = argv0;
  } else {
    // got sep char
    log_name = argv0 + i + 1;
  }
}

xsuccess xlog_init(int argc, char* argv[]) {
  xsuccess ret = XSUCCESS;
  xoption xopt = xoption_new();
  xoption_parse(xopt, argc, argv);

  if (xoption_has(xopt, "log-level")) {
    log_level = atoi(xoption_get(xopt, "log-level"));
  }

  if (xoption_has(xopt, "log-dir")) {
    log_dir = xoption_get(xopt, "log-dir");
  }

  if (xoption_has(xopt, "log-maxsize")) {
    log_max_size = xfilesystem_parse_filesize(xoption_get(xopt, "log-maxsize"));
    if (log_max_size < 10 * 1024) {
      log_max_size = 10 * 1024; // lower limit is 10K
    }
  }

  if (xoption_has(xopt, "log-history")) {
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
    log_screenonly = XTRUE;
  }

  get_log_name((char *) xvec_get(xoption_get_array(xopt, NULL), 0));

  xoption_delete(xopt);
  return ret;
}

static void do_log(const char* cstr) {
  printf("%s", cstr);
  if (log_screenonly != XTRUE && log_fp != NULL) {
    fprintf(log_fp, "%s", cstr);
  }
}

static void move_current_log_into_history() {
  xstr current_log_fn = xstr_new();
  int i;
  xjoin_path_cstr(current_log_fn, log_dir, log_name);
  xstr_append_cstr(current_log_fn, ".log");

  for (i = 1; i <= log_history; i++) {
    xstr detect_log_fn = xstr_new();
    xstr_printf(detect_log_fn, "%s.%d", xstr_get_cstr(current_log_fn), i);
    if (xfilesystem_exists(xstr_get_cstr(detect_log_fn)) == XFALSE) {
      break;
    }
    xstr_delete(detect_log_fn);
  }

  if (i <= log_history) {
    xstr history_log_fn = xstr_new();
    xstr_printf(history_log_fn, "%s.%d", xstr_get_cstr(current_log_fn), i);
    rename(xstr_get_cstr(current_log_fn), xstr_get_cstr(history_log_fn));
    xstr_delete(history_log_fn);
  } else {
    xstr history_log_fn = xstr_new();
    xstr_printf(history_log_fn, "%s.%d", xstr_get_cstr(current_log_fn), 1);
    remove(xstr_get_cstr(history_log_fn));
    for (i = 2; i <= log_history; i++) {
      xstr from_fn = xstr_new();
      xstr to_fn = xstr_new();
      xstr_printf(from_fn, "%s.%d", xstr_get_cstr(current_log_fn), i);
      xstr_printf(to_fn, "%s.%d", xstr_get_cstr(current_log_fn), i - 1);
      rename(xstr_get_cstr(from_fn), xstr_get_cstr(to_fn));
      xstr_delete(from_fn);
      xstr_delete(to_fn);
    }
    xstr_printf(history_log_fn, "%s.%d", xstr_get_cstr(current_log_fn), log_history);
    rename(xstr_get_cstr(current_log_fn), xstr_get_cstr(history_log_fn));
    xstr_delete(history_log_fn);
  }

  xstr_delete(current_log_fn);
}

static void prepare_fp() {
  if (log_fp != NULL) {
    if (ftello(log_fp) >= log_max_size) {
      fclose(log_fp);
      move_current_log_into_history();
      log_fp = NULL;
      prepare_fp(); // retry, again
    }
  } else if (log_name != NULL) {
    // only prepare fp if log_name is given, that means the logging system is initialized
    xstr log_fn = xstr_new();
    xjoin_path_cstr(log_fn, log_dir, log_name);
    xstr_append_cstr(log_fn, ".log");
    log_fp = fopen(xstr_get_cstr(log_fn), "a");
    xstr_delete(log_fn);
  }
}

void xlog_start_record(int level) {
  // prevent 2 thread from logging concurrently, which will mess up the log file
  pthread_mutex_lock(&logging_lock);
  prepare_fp();
  if (level <= log_level) {
    char timestr_buf[32];
    struct tm* tm_struct;
    time_t tm_val = time(NULL);
    tm_struct = localtime(&tm_val);
    strftime(timestr_buf, sizeof(timestr_buf), "[%Y.%m.%d %H:%M:%S] ", tm_struct);
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
  }
}


void xlog_end_record(const char* fmt) {
  // if no new line in fmt, append a new line
  if (fmt[strlen(fmt) - 1] != '\n') {
    do_log("\n");
  }
  // prevent 2 thread from logging concurrently, which will mess up the log file
  pthread_mutex_unlock(&logging_lock);
}

FILE* xlog_get_log_fp() {
  return log_fp;
}

xbool xlog_is_screen_only() {
  return log_screenonly;
}

