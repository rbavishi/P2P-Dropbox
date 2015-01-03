#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<errno.h>
#define PID_LIST_SIZE 50
int global_pid_list[PID_LIST_SIZE];
int pid_list_count=0;                             //To store the number of child pids created.

char ** string_to_command(char *command) {         //Break down command into pieces for execlp
	int len=strlen(command),i=0,index=0,prev_starting_index=0;
	char **c=malloc(sizeof(char *)*13);
	while (i<len) {
		if (command[i]=='\"') {
			int j=++i;
			while (command[i]!='\"') {
				i++;
			}
			c[index]=(char *)malloc(sizeof(char)*(i-j)+1);
			command[i]='\0';
			strcpy(c[index],&command[j]);
			//printf("%s\n",c[index]);
			index+=1;
			prev_starting_index=i+1;
			i+=2;
		}
		else if (command[i]=='\'') {
			int j=++i;
			while (command[i]!='\'') {
				i++;
			}
			c[index]=(char *)malloc(sizeof(char)*(i-j)+1);
			command[i]='\0';
			strcpy(c[index],&command[j]);
			//printf("%s\n",c[index]);
			index+=1;
			prev_starting_index=i+1;
			i+=2;
		}
		else if (command[i]==' ') {
			command[i]='\0';
			c[index]=(char *)malloc(sizeof(char)*strlen(&command[prev_starting_index])+1);
			strcpy(c[index],&command[prev_starting_index]);
			//printf("%s\n",c[index]);
			index+=1;
			prev_starting_index=i+1;
			i+=1;
		}
		else {i+=1;}
	}
	if (command[prev_starting_index]!='\0') {
		c[index]=(char *)malloc(sizeof(char)*strlen(&command[prev_starting_index])+1);
		strcpy(c[index],&command[prev_starting_index]);
		index+=1;
	}
	while (index!=13){
		c[index]=NULL;
		index+=1;
	}
	return c;
}

int execute_using_pipe(char **command,int sleeptime) {	//Creating our own version of Popen to do non-blocking polling.
	extern int global_pid_list[PID_LIST_SIZE];
	extern int pid_list_count;
	int fd[2];
	if (pipe(fd)==-1) {
		if (errno==EMFILE) {
			printf("PIPE CREATE FAILURE: No free descriptors.\n");
		}
		else if (errno==EFAULT) {
			printf("PIPE CREATE FAILURE: fd array not valid.\n");
		}
		return -1;
	}
	pid_t childpid;
	if ((childpid=fork())==-1) {
		if (errno==EAGAIN) {
			printf("Resource limit exceeded.\n");
		}
		if (errno=ENOMEM) {
			printf("Memory tight.\n");
		}
		return -1;
	}
	if (childpid==0) {    //Child process - duplicating onto input stream and executing command.
		dup2(0,fd[0]);
		sleep(sleeptime);
		execlp(command[0],command[0],command[1],command[2],command[3],command[4],command[5],command[6],command[7],command[8],command[9],command[10],command[11],command[12]);  //For now, ability to call command with 13 subparts
		printf("Error executing command.\n");
		exit(EXIT_FAILURE);
		//Child process is killed after execution of execlp, so no need to worry about a return value.
	}

	else {  //Parent process
		close(fd[1]);
		global_pid_list[pid_list_count]=childpid;
		pid_list_count+=1;
		return childpid;
	}
}
int exec_pipe(char *command,int sleeptime) {
	//printf("%s\n",command);
	return execute_using_pipe(string_to_command(command),sleeptime);
}

int exec_pipe_raw(char **command,int sleeptime) {
	return execute_using_pipe(command,sleeptime);
}
