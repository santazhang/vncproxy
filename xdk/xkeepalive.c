#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <string.h>

#include "xkeepalive.h"
#include "xutils.h"
#include "xdef.h"

/**
  @brief
    Information for keep alive function.
*/
typedef struct {
  int target_pid; ///< @brief Process id of target process, which we want to keep running.
  keep_alive_func target_func;  ///< @brief Target process's entrance function.
  int target_argc;  ///< @brief Parameter for target process's entrance function.
  char** target_argv;  ///< @brief Parameter for target process's entrance function.
  int expected_monitors;  ///< @brief Number of expected running monitors.
  int current_monitors; ///< @brief Current number of running monitors.
  int* monitor_pids;  ///< @brief List of running monitor's pid.
  int check_interval; ///< @brief The check interval.

  int msg_key_base;  ///< @brief Message key for IPC through message pipes.
  int msg_type; ///< @brief Message type for IPC through message pipes.
} keep_alive_info;

/**
  @brief
    Private IPC message structure.
*/
typedef struct {
  long mtype; ///< @brief Message type, act like an envelope around the message.
  long pid; ///< @brief The receiver pid.
  char cmd[32]; ///< @brief The message command, could be "monitor_die", "new_monitor", "change_target"
  long value; ///< @brief Optional value of the command.
} my_msg;

// forward declaration
static int monitor(keep_alive_info* info);

static int my_fork() {
  int ret = fork();
  if (ret != 0) {
    // make sure parent & child does not have same rand value
    srand(ret);
  } else {
    srand(time(NULL));
  }
  return ret;
}

static void monitor_server_real(keep_alive_info* info) {
  // please note the message key: info->msg_key_base + getpid(), this prevents multiple processes using the same message
  int msgid = msgget(info->msg_key_base + getpid(), IPC_CREAT | IPC_EXCL | 0666);
  my_msg msgbuf;
  printf("[xkeepalive] monitor %d started message server\n", getpid());
  for (;;) {
    msgrcv(msgid, &msgbuf, sizeof(my_msg), info->msg_type, 0);
    printf("[xkeepalive] monitor %d's message server got message '%s' with value %ld\n", getpid(), msgbuf.cmd, msgbuf.value);

    if (xcstr_startwith_cstr(msgbuf.cmd, "new_monitor")) {
      int i;
      xbool already_have = XFALSE;
      for (i = 0; i < info->current_monitors; i++) {
        if (info->monitor_pids[i] == msgbuf.value) {
          already_have = XTRUE;
          break;
        }
      }
      if (already_have == XFALSE) {
        info->monitor_pids[info->current_monitors] = msgbuf.value;
        info->current_monitors++;
      }
    } else if (xcstr_startwith_cstr(msgbuf.cmd, "monitor_die")) {
      int i, j;
      for (i = 0; i < info->current_monitors; i++) {
        if (info->monitor_pids[i] == msgbuf.value) {
          for (j = i; j < info->current_monitors - 1; j++) {
            info->monitor_pids[j] = info->monitor_pids[j + 1];
          }
          info->current_monitors--;
        }
      }
    } else if (xcstr_startwith_cstr(msgbuf.cmd, "change_target")) {
      info->target_pid = msgbuf.value;
    } else {
      printf("[xkeepalive] don't know how to deal with '%s' command, ignore it\n", msgbuf.cmd);
    }
  }
}

static void* monitor_server_wrapper(void* args) {
  monitor_server_real((keep_alive_info *) args);
  return NULL; 
}

static void run_monitor_server(keep_alive_info* info) {
  pthread_t tid;
  pthread_create(&tid, NULL, monitor_server_wrapper, (void *) info);
}

static void monitors_broadcast(keep_alive_info* info, char* cmd, long value) {
  int i;
  my_msg msgbuf;
  for (i = 0; i < info->current_monitors; i++) {
    // note the message key: info->msg_key_base + info->monitor_pids[i]
    int msgid = msgget(info->msg_key_base + info->monitor_pids[i], 0666);
    msgbuf.mtype = info->msg_type;
    msgbuf.pid = getpid();
    strcpy(msgbuf.cmd, cmd);
    msgbuf.value = value;
    printf("[xkeepalive] monitor %d is sending '%s' with value %ld to monitor %d\n", getpid(), msgbuf.cmd, msgbuf.value, info->monitor_pids[i]);
    msgsnd(msgid, &msgbuf, sizeof(my_msg) - sizeof(long), 0);
  }
}

static int fork_new_monitor(keep_alive_info* info) {
  int pid = my_fork();
  if (pid != 0) {
    // parent process, increase counter, return pid
    monitors_broadcast(info, "new_monitor", pid);
    info->monitor_pids[info->current_monitors] = pid;
    info->current_monitors++;
    return pid;
  } else {
    // child process, start monitor
    info->monitor_pids[info->current_monitors] = getpid();
    info->current_monitors++;
    monitor(info);
    exit(0);
  }
}

static void check_alive_monitors(keep_alive_info* info) {
  int i;
  for (i = 0; i < info->current_monitors; i++) {
    // TODO pthread locking here?

    // just checking if alive, no signal sent
    if (kill(info->monitor_pids[i], 0) < 0) {
      printf("[xkeepalive] oops, monitor %d is dead!\n", info->monitor_pids[i]);
      monitors_broadcast(info, "monitor_die", info->monitor_pids[i]);
    }
  }
}

static void check_is_target_alive(keep_alive_info* info) {
  // just checking if alive, no signal sent
  if (kill(info->target_pid, 0) < 0) {
    int new_target_pid;
    printf("[xkeepalive] oops, target process %d is dead!\n", info->target_pid);
    
    new_target_pid = my_fork();
    if (new_target_pid != 0) {
      // parent process, still work as monitor
      printf("[xkeepalive] recover by reforking worker process, with pid %d\n", new_target_pid);
      monitors_broadcast(info, "change_target", new_target_pid);
    } else {
      // child process, recover the original work
      info->target_func(info->target_argc, info->target_argv);
      exit(0);
    }
  }
}

static int monitor(keep_alive_info* info) {
  const int sleep_min = 100;  // minimum sleep interval
  int sleep_rand_range = 100; // default sleep range is in 100 ~ 200 (= 100 + 100)
  if (info->check_interval - sleep_min > sleep_rand_range) {
    sleep_rand_range = info->check_interval - sleep_min;  // could change sleep range to a larger value
  }

  if (info->expected_monitors > 4) {
    info->expected_monitors = 4;  // allow maximum 4 monitors at the same time
  }

  run_monitor_server(info); // run monitor server, in a separate thread

  for (;;) {

    // sleep for a random range of seconds, optimistically prevent 2 process working at the same time.
    // could use locking instead
    int sleep_time = sleep_min + rand() % sleep_rand_range;
    xsleep_msec(sleep_time);  // sleep, in milliseconds
    printf("[xkeepalive] monitor process %d woke up from %d ms sleep interval\n", getpid(), sleep_time);

    // check failed monitors
    // if failure happened, broadcast failed monitors to running monitors
    printf("[xkeepalive] checking alive monitors\n");
    check_alive_monitors(info);

    // check if there is too few monitors
    while (info->current_monitors < info->expected_monitors) {
      int pid = fork_new_monitor(info);
      printf("[xkeepalive] forked new monitor with pid %d\n", pid);
    }

    // check if target failed
    // if failure happened, fork new child process, and broad cast to all monitors
    printf("[xkeepalive] checking alive working process\n");
    check_is_target_alive(info);
  }
  return 0;
}

int xkeep_alive(keep_alive_func func, int argc, char* argv[]) {
  int pid;

  // prevent zombie processes
  signal(SIGCHLD, SIG_IGN); 

  pid = my_fork();
  if (pid != 0) {
    // parent process, do real job
    return func(argc, argv);
  } else {
    // child process, do monitoring
    keep_alive_info info;
    info.target_pid = getppid();  // get parent process's pid
    info.target_func = func;
    info.target_argc = argc;
    info.target_argv = argv;
    info.expected_monitors = 2;
    info.current_monitors = 1;
    info.monitor_pids = (int *) malloc(8 * sizeof(int));  // do not use xmalloc here
    info.monitor_pids[0] = getpid();
    info.check_interval = 10000;
    info.msg_key_base = 10000 + rand() % 54000;
    info.msg_type = 10000 + rand() % 54000;
    monitor(&info);
    exit(0);
  }
}

