#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<pthread.h>
#include<sys/mman.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/inotify.h>
#include<string.h>
#include "inotify.h"							//Header file containing prototypes 
#include "wrapper.h"	
#define RSYNC_TIMEOUT_ERROR_CODE 30					//Rsync exit statuses...Can be used to decide whether syncing was successful.
#define RSYNC_SSH_ERROR_CODE 255
#define STATUS_FILESIZE (sizeof(char)*50*80)
struct reading_info {		//The reading_info struct and wait_for_events func is used in thread_making. Arbitrarily defined. No use as such
	int fd;
};

void *wait_for_events(void* info) {
	struct reading_info* inf=(struct reading_info*)info;
	reading(inf->fd);
}

char *get_time_string() {					//Get time at a particular instance...used for displaying time-started in dbstatus.
	time_t rawtime;
	struct tm *t;
	time(&rawtime);
	t=localtime(&rawtime);
	char *timing=(char *)malloc(sizeof(char)*9);
	if (t->tm_hour<10) {
		if (t->tm_min<10) {
			sprintf(timing,"0%d:0%d",t->tm_hour,t->tm_min);
		}
		else {
			sprintf(timing, "0%d:%d",t->tm_hour,t->tm_min);
		}
	}
	else {
		if (t->tm_min<10) {
			sprintf(timing,"%d:0%d",t->tm_hour,t->tm_min);
		}
		else {
			sprintf(timing, "%d:%d",t->tm_hour,t->tm_min);
		}
	}
	return timing;
}

void configure_status_file_and_mmap() {	//Opening file and creating a memory-mapped-file for easy and efficient file_writing/sharing data with the other python process - dbstatus
	char *file_name=(char *)malloc(sizeof(char)*(strlen(address)+20));
	sprintf(file_name,"%s_status_file.txt",address);
	mmap_fd=open(file_name,O_RDWR | O_CREAT | O_TRUNC,S_IRUSR | S_IWUSR);
	lseek (mmap_fd, STATUS_FILESIZE-1, SEEK_SET); 
	write (mmap_fd, "", 1); 
	lseek (mmap_fd, 0, SEEK_SET); 
	mmap_file_pointer=mmap(0,STATUS_FILESIZE, PROT_WRITE, MAP_SHARED, mmap_fd,0);
}

void clear_status_file() {				//File cleanup
	munmap(mmap_file_pointer,STATUS_FILESIZE);
	close(mmap_fd);
}

void lock_file() {					//File locking/unlocking to avoid race conditions.
	file_lock.l_type = F_WRLCK;
	fcntl(mmap_fd,F_SETLKW,&file_lock);
}

void unlock_file() {
	file_lock.l_type = F_UNLCK;
	fcntl(mmap_fd,F_SETLK,&file_lock);
}

void put_info_in_status_file(struct subprocess* process) {	//Write data to memory-mapped-file.
	char temp[100];
	sprintf(temp,"%s#%s#%d\n",process->path,process->start_time,process->retries);
	strcat((char *)mmap_file_pointer,temp);
}

int delete_list_element(struct subprocess* process) {		//Linked-list standard operation.
	if (process->prev==NULL && process->next==NULL) {
		pid_list.starting=NULL;
		pid_list.last=NULL;
	}
	else if (process->prev==NULL) {
		pid_list.starting=process->next;
		process->next->prev=NULL;
	}
	else if (process->next==NULL) {
		pid_list.last=process->prev;
		process->prev->next=NULL;
	}
	else {
		process->prev->next=process->next;
		process->next->prev=process->prev;
	}
	return 0;
}

int add_list_element(struct subprocess* process) {		//Linked list standard operation.
	if (pid_list.last==NULL) {
		pid_list.starting=process;
		pid_list.last=process;
	}
	else {
		pid_list.last->next=process;
		process->prev=pid_list.last;
		pid_list.last=process;
	}
	return 0;
}

int execute_and_create_process(struct subprocess* process, struct inotify_event* event, char *command) { //Execute command using wrapper function.
	process->start_time=(char*)malloc(sizeof(char)*9);
	strcpy(process->start_time,get_time_string());
	int pid=exec_pipe(command,0);
	if (pid==-1) {
		return -1;
	}
	process->pid=pid;
	process->retries=0;
	process->command=(char *)malloc(sizeof(char)*strlen(command));
	process->event=event;
	process->prev=NULL;
	process->next=NULL;
	strcpy(process->command,command);
	process->path=(char *)malloc(sizeof(char)*(strlen(global_wd_list[event->wd]->path)+strlen(event->name)+2));
	sprintf(process->path,"%s/%s",global_wd_list[event->wd]->path,event->name);
	add_list_element(process);
	return pid;
}

int kill_and_replace_process(struct subprocess* to_be_killed, struct subprocess* replacement, struct inotify_event* event, char*command) {
	kill(to_be_killed->pid,SIGINT);
	delete_list_element(to_be_killed);
	return execute_and_create_process(replacement,event,command);
}

int push_event_into_subprocess_list(struct inotify_event* event,char *command) {	//Push events into the subprocess stack, with linked_list ops.
	struct subprocess* process=(struct subprocess*)malloc(sizeof(struct subprocess));
	if (pid_list.starting==NULL) {
		subprocess_count+=1;
		return execute_and_create_process(process,event,command);
	}
	else {
		struct subprocess * current=pid_list.starting;
		while (current->next!=NULL) {
			if (current->event->wd==event->wd && current->event->name==event->name) {
				return kill_and_replace_process(current,process,event,command);
			}
			current=current->next;
		}
		if (current->event->wd==event->wd && current->event->name==event->name) {	
			return kill_and_replace_process(current,process,event,command);
		}
		subprocess_count+=1;
		return execute_and_create_process(process,event,command);
	}
}

int poll_pids() {			//Poll all the alive/exited pids with WNOHANG - returns immediately without waiting for child process to exit.
	if (pid_list.starting==NULL) {
		printf("No processes.\n");
		return 0;
	}
	else {
		configure_status_file_and_mmap();
		int sleeptime=5,i;
		struct subprocess* current=pid_list.starting;
		int poll_status,status;
		lock_file();
		sprintf((char *)mmap_file_pointer,"");
		while (current->next!=NULL) {
			poll_status=waitpid(current->pid,&status,WNOHANG);
			if (poll_status==current->pid) {
				int exit_code=WEXITSTATUS(status);
				if (exit_code==0) {
					subprocess_count-=1;
					delete_list_element(current);
				}
				else if (exit_code==30 || exit_code==255 || exit_code==12) {
					if (exit_code==12) {sleeptime=1;}
					current->retries+=1;
					sleeptime=5;
					for (i=0;i<current->retries && sleeptime<320;i++) {sleeptime*=2;}
					strcpy(current->start_time,get_time_string());
					current->pid=exec_pipe(current->command,sleeptime);
				}
				else {
					subprocess_count-=1;
					delete_list_element(current);
				}
			}
			else if (poll_status==0) {
				put_info_in_status_file(current);
			}
			current=current->next;
		}
		poll_status=waitpid(current->pid,&status,WNOHANG);
		if (poll_status==current->pid) {
			int exit_code=WEXITSTATUS(status);
			if (exit_code==0) {
				subprocess_count-=1;
				delete_list_element(current);
			}
			else if (exit_code==30 || exit_code==255 || exit_code==12) {
				if (exit_code==12) {sleeptime=1;}
				current->retries+=1;
				sleeptime=5;
				for (i=0;i<current->retries && sleeptime<320;i++) {sleeptime*=2;}
				strcpy(current->start_time,get_time_string());
				printf("%s\n",current->command);
				current->pid=exec_pipe(current->command,sleeptime);
			}
			else {
				subprocess_count-=1;
				delete_list_element(current);
			}
		}
		else if (poll_status==0) {
			put_info_in_status_file(current);
		}
		unlock_file();
		clear_status_file();
		return 0;
	}
}

int read_event_shell(struct inotify_event* event,int fd) { 		//Read inotify and events and form rsync commands accordingly
	char *temp=(char *)malloc(sizeof(char)*(strlen(global_wd_list[event->wd]->path)+strlen(event->name)+2));
	sprintf(temp,"%s/%s",global_wd_list[event->wd]->path,event->name);
	char *dest=(char *)malloc(sizeof(char)*(strlen(temp)+strlen(dest_folder)-strlen(home_folder)+strlen(address)+2));
	sprintf(dest,"%s:%s%s",address,dest_folder,&temp[strlen(home_folder)]);
	char *destfold=(char *)malloc(sizeof(char)*(strlen(dest)));
	sprintf(destfold,"%s:%s%s",address,dest_folder,&global_wd_list[event->wd]->path[strlen(home_folder)]);
	char command[513];
	if (event->mask &IN_MOVED_TO) {
		if (checkif_dir(temp)) {
			sprintf(command,"rsync -azse ssh \"%s/\" \"%s\"",temp,dest);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"Directory %s moved to %s.\n",event->name,global_wd_list[event->wd]->path);
		        int status=add_newdir(fd,event->wd,temp);
			if (status==-1) {
				fprintf(lgfile,"Addwatch failure for new directory.\n");
			}
		}
		else {
			if (test_if_file_exists(temp)!=0) {return 0;}
			sprintf(command,"rsync -azse ssh \"%s\" \"%s\"",temp,dest);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"File %s moved to %s.\n",event->name,global_wd_list[event->wd]->path);
		}

	}
	else if (event->mask &IN_MOVED_FROM) {
		int status=remove_dir(fd,event->wd,temp);
		if (status==-1) {
			sprintf(command,"rsync -azsre ssh --delete --force \'--include=%s\' \'--exclude=*\' \"%s/\" \"%s\"",event->name,global_wd_list[event->wd]->path,destfold);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"File %s moved from %s.\n",event->name,global_wd_list[event->wd]->path);
		}
		else {
			sprintf(command,"rsync -azsre ssh --delete --force \'--include=%s**\' \'--exclude=*\' \"%s/\" \"%s\"",event->name,global_wd_list[event->wd]->path,destfold);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"Directory %s moved from %s.\n",event->name,global_wd_list[event->wd]->path);
		}
	}
	else if (event->mask &IN_MODIFY) {
		if (!(checkif_dir(temp))) {
			if (test_if_file_exists(temp)!=0) {return 0;}
			sprintf(command,"rsync -azse ssh \"%s\" \"%s\"",temp,dest);
			push_event_into_subprocess_list(event,command);
		        fprintf(lgfile,"File %s was modified.\n",event->name);
		}
	}
	else if (event->mask &IN_CREATE) {
		if (checkif_dir(temp)) {
			sprintf(command,"rsync -azse ssh \"%s/\" \"%s\"",temp,dest);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"Directory %s created in %s.\n",event->name,global_wd_list[event->wd]->path);
			int status=add_newdir(fd,event->wd,temp);
			if (status==-1)
			{
				fprintf(lgfile,"Addwatch failure for new directory.\n");
			}
		}
		else {
			if (test_if_file_exists(temp)!=0) {return 0;}
			sprintf(command,"rsync -azse ssh \"%s\" \"%s\"",temp,dest);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"File %s created in %s.\n",event->name,global_wd_list[event->wd]->path);
		}
	}
	else if (event->mask &IN_DELETE) {
		int status=find_child_in_dir(event->wd,temp);
		if (status==-1) {
			sprintf(command,"rsync -azsre ssh --delete --force \'--include=%s\' \'--exclude=*\' \"%s/\" \"%s\"",event->name,global_wd_list[event->wd]->path,destfold);
			push_event_into_subprocess_list(event,command);
			fprintf(lgfile,"File %s deleted from %s.\n",event->name,global_wd_list[event->wd]->path);
		}
		else {
			sprintf(command,"rsync -azsre ssh --delete --force \'--include=%s**\' \'--exclude=*\' \"%s/\" \"%s\"",event->name,global_wd_list[event->wd]->path,destfold);
			push_event_into_subprocess_list(event,command);
			update_parent_directory(event->wd,status);
		}
	}
	else if (event->mask &IN_DELETE_SELF) {
		fprintf(lgfile,"Directory %s deleted.\n",global_wd_list[event->wd]->path);
		fprintf(lgfile,"Watch removed for wd = %d\n",event->wd);
		if (global_wd_list[event->wd]->parent_wd==-1) {
			int f=0;
			for (f=0;f<fd_count;f++) {
				if (global_fd_list[f]->fd==fd) {
					break;
				}
			}
			if (global_fd_list[f]->wdlist[global_fd_list[f]->count-1]==event->wd) {
				global_fd_list[f]->count-=1;
			}
			else {
				int i;
				for (i=0;i<global_fd_list[f]->count;i++) {
					if (global_fd_list[f]->wdlist[i]==event->wd)
					{break;}
				}
				global_fd_list[f]->wdlist[i]=global_fd_list[f]->wdlist[global_fd_list[f]->count-1];
				global_fd_list[f]->count-=1;
			}
		}
	}
	fclose(lgfile);
	lgfile=fopen("logfile.txt","a");
	return 0;
}

	
int main(int argc,char *argv[]) {
	fd_count=0;
	lgfile=fopen("logfile.txt","w");

	/* Locking File Config */
	file_lock.l_type   = F_WRLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
	file_lock.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
	file_lock.l_start  = 0;        /* Offset from l_whence         */
	file_lock.l_len    = 0;        /* length, 0 = to EOF           */
	file_lock.l_pid    = getpid(); /* our PID */
	/*Locking File Config */

	strcpy(home_folder,argv[2]);
	strcpy(address,argv[3]);
	strcpy(dest_folder,argv[4]);
	int fd=fd_update(home_folder,-1);

	/* Initalizing linked-list head */
	pid_list.starting=NULL;
	pid_list.last=NULL;
	/*Initializing linked list head */

	/* Setting signal. */
	signal(SIGINT,sigint_handler);
	signal(SIGALRM,timeout);
	/* Setting signal. */

	struct reading_info input;
	input.fd=fd;
	alarm(atoi(argv[1]));
	event_occurred_count=0;
	subprocess_count=0;
	/*Creating thread for inotify. */

	int retvalue;
	if ((retvalue=pthread_create(&inotify_thread,NULL,&wait_for_events,&input))) {
		printf("Thread creation failure. Code: %d\n",retvalue);
	}
	/*Initializing thread-lock variable*/
	if ((pthread_mutex_init(&inotify_event_lock,NULL))) {
		printf("Lock initialization error. Fatal.\n:");
		exit(EXIT_FAILURE);
	}
	/*Initializing thread-lock variable*/
	int status1,i;
	char com[80];
	if (test_if_dir_exists(home_folder)!=0) {
		printf("Making dir...\n");
		sprintf(com,"mkdir %s",home_folder);
		exec_pipe(com,0);
		waitpid(-1,&status1,0);
	}

	/* An initial rsync to sync the two folders */
	sprintf(com,"rsync -azsre ssh --delete \"%s/\" \"%s:%s\"",home_folder,address,dest_folder);
	int exit_status=system(com);
	if (exit_status!=0) {
		printf("Connection does not exist. Please check whether the remote host is alive or not.\n");
		exit(EXIT_FAILURE);
	}
	/* An initial rsync to sync the two folders */

	while(1) {
		if (event_occurred_count!=0 && subprocess_count<MAX_EVENT_STACK_SIZE) {	 
			pthread_mutex_lock(&inotify_event_lock);
			int i=0;
			while (i<event_occurred_count) {
				read_event_shell(event_stack[i],fd);
				event_stack[i]=NULL;
				i+=1;
			}
			event_occurred_count=0;
			pthread_mutex_unlock(&inotify_event_lock);
		}
		for (i=0;i<2;i++) {
			poll_pids();
			sleep(1);
		}
	}
	destroy_all();
	fclose(lgfile);
	return 0;
}
