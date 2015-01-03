#ifndef WRAPPER_H
#define WRAPPER_H
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#define PID_LIST_SIZE 50
int global_pid_list[PID_LIST_SIZE];
int pid_list_count;
char ** string_to_command(char *command);
int execute_using_pipe(char **command, int sleeptime);
int exec_pipe(char *command, int sleeptime);
int exec_pipe_raw(char **command, int sleeptime);
#endif
