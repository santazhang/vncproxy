#ifndef XLOG_H_
#define XLOG_H_

/**
  @author
    Santa Zhang

  @file
    xlog.h

  @brief
    Provide logging utility.
*/


/**
  @brief
    Write a log to file descriptor.

  @param logger
    The file descriptor where logs will be written to.
  @param fmt
    The formatting string.
*/
#define xflogf(logger, fmt, ...) fprintf(logger, fmt, __VA_ARGS__)

#endif  // XLOG_H_

