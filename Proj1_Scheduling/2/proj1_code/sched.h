#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PROC 10
#define TQ 2

void signal_handler(int signo);
void running_tq();
void do_child(int signo);
void check_waiting();
void check_running();
void parent_job(int signo);