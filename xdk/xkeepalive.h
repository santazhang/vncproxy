#ifndef XKEEP_ALIVE_H_
#define XKEEP_ALIVE_H_

/**
  @author
    Santa Zhang

  @file
    xkeepalive.h

  @brief
    Provide functions that will run forever.
*/

/**
  @brief
    Type of entrance functions which will be running forever.

  @param argc
    Number of arguments, should be directly given from main().
  @param argv
    Array of arguments, should be directly given from main().

  @return
    Return what a main() function might return.
*/
typedef int (*keep_alive_func)(int argc, char* argv[]);

/**
  @brief
    Start a function, and always keep it up and running.

  @param func
    The function to be running.
  @param argc
    Number of arguments, should be directly given from main().
  @param argv
    Array of arguments, should be directly given from main().
    

  @return
    Returns what func might return.
*/
int xkeep_alive(keep_alive_func func, int argc, char* argv[]);

#endif  // XKEEP_ALIVE_H_

