#ifndef XLOG_H_
#define XLOG_H_

#include <stdio.h>
#include <stdarg.h>

#include "xdef.h"

/**
  @author
    Santa Zhang

  @file
    xlog.h

  @brief
    Provide logging utility.
    
    The logging system uses the following parameters:<br>
    --log-level=2<br>
    Loging levels are: 0 (fatal), 1 (error), 2 (warning), 3 (info), 4 (debug), 5~7(user defined).<br>

    --log-dir=.<br>
    Where all the logs and log history will be saved.<br>

    --log-maxsize=10M<br>
    Max file size of the log, if it is bigger than this, log file is moved into history.<br>

    --log-history=5<br>
    Max number of 'old' logs to be kept.

    --log-screenonly<br>
    Only write to screen. This is by default disabled.
*/

/**
  @brief
    Logging level of fatal errors. The system will typically halt on fatal errors.
*/
#define XLOG_FATAL    0

/**
  @brief
    Logging level of errors. The system will run incorrectly on these errors.
*/
#define XLOG_ERROR    1

/**
  @brief
    Logging level of warnings.
*/
#define XLOG_WARNING  2

/**
  @brief
    Logging level of informations.
*/
#define XLOG_INFO     3

/**
  @brief
    Logging level of debug messages.
*/
#define XLOG_DEBUG    4

/**
  @brief
    Logging level 5. User defined.
*/
#define XLOG_LEVEL5   5

/**
  @brief
    Logging level 6. User defined.
*/
#define XLOG_LEVEL6   6

/**
  @brief
    Logging level 7. User defined.
*/
#define XLOG_LEVEL7   7

/**
  @brief
    Initialize logging system, parse command line args concerning the log system.

  @param argc
    The number of command line args. This is provided to main().
  @param argv
    The array of command line args. This is provided to main().

  @return
    Whether the logging system was correctly started.
*/
xsuccess xlog_init(int argc, char* argv[]);


/**
  @brief
    Write logs of a certain logging level.

  @param level
    The logging level, could be 0 (fatal), 1 (error), 2 (warning), 3 (info), 4 (debug), 5~7 (user defined).
  @param fmt
    The formatting string.

  @return
    Whether the log was correctly written.
*/
#define xlog(level, fmt, ...) { \
  if (level <= XLOG_LEVEL7) { \
    xlog_start_record(level); \
    printf(fmt, ## __VA_ARGS__); \
    if (xlog_is_screen_only() == XFALSE) {  \
      fprintf(xlog_get_log_fp(), fmt, ## __VA_ARGS__); \
    } \
    xlog_end_record(fmt); \
  } else {  \
    fprintf(stderr, "[error] no such log level: %d\n", level);  \
  } \
};

/**
  @brief
    Start writing a log record.

  @param level
    The logging level, could be 0 (fatal), 1 (error), 2 (warning), 3 (info), 4 (debug), 5~7 (user defined).

  @return
    Whether the log was correctly written.
*/
void xlog_start_record(int level);

/**
  @brief
    Stop writing a log record.

  @param fmt
    The formatting string.

  @return
    Whether the log was correctly written.
*/
void xlog_end_record(const char* fmt);

/**
  @brief
    Get the file pointer of log file.

  @return
    The file pointer.
*/
FILE* xlog_get_log_fp();

/**
  @brief
    Check if the logging system only writes to stdout.

  @return
    XTRUE if logs are written to stdout only.
*/
xbool xlog_is_screen_only();

#endif  // XLOG_H_

