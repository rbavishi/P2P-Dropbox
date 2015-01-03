#include<stdio.h>
#include<string.h>
#include<sys/inotify.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<signal.h>
#include<pthread.h>
#include "inotify.h"
#define SIZE (sizeof(struct inotify_event))
#define BUFLEN (1024*(SIZE+16))
//struct dirinfo* global_wd_list[1025];                          //The global list for wd's and the directory address
//struct fdinfo * global_fd_list[10];                            //The global list of fd's
//int fd_count;                                                //No. of fd's in existence.
//FILE *lgfile;                                                     //File pointer for the lgfilefile.
//struct inotify_event* event_stack[50];                         //Maintain the stack of inotify events.
//int event_occurred_count;                                       //Maintain the count - Makes things easier
void timeout (int signum) {
	extern FILE *lgfile;
	printf("TIMED OUT\n");
	pthread_cancel(inotify_thread);
	destroy_all();
	fclose(lgfile);
	exit(EXIT_SUCCESS);
}

void destroy_all()
{
	int i=0;
	extern struct fdinfo *global_fd_list[10];
	extern int fd_count;
	while (i<fd_count)
	{
		remove_all(i);
		i++;
	}
}

void sigint_handler(int signum)	//SIGINT signal handler
{
	destroy_all();
	extern FILE *lgfile;
	fclose(lgfile);
	signal(signum,SIG_DFL);
	raise(signum);
}

int checkif_dir(char *path)
{
	struct stat s;
	if (stat(path,&s)!=-1 && S_ISDIR(s.st_mode)){
		return 1;
	}
	return 0;
}

int checkif_regfile(char *path)
{
	struct stat s;
	if (stat(path,&s)!=-1 && S_ISREG(s.st_mode)){
		return 1;
	}
	return 0;
}

int test_if_file_exists(char *path) {
	char com[strlen(path)+7];
	sprintf(com,"test -f \"%s\"",path);
	return system(com);
}

int test_if_dir_exists(char *path) {
	char com[strlen(path)+7];
	sprintf(com,"test -d \"%s\"",path);
	return system(com);
}
struct dirinfo* dir_init()
{
	struct dirinfo * dir = (struct dirinfo*)malloc(sizeof(struct dirinfo));
 	dir->child_count=0;
	dir->wd=0;
}

int find_child_in_dir(int parent_wd,char* path)
{
	extern struct dirinfo* global_wd_list[1025];
	struct dirinfo *temp=global_wd_list[parent_wd];
	int i;
	for (i=0;i<temp->child_count;i++)
	{
		if (strcmp(temp->children[i]->path,path)==0)
		{
			return i;
		}
	}
	return -1;
}

int add_watch_rec(int fd,char *path,int parent_wd) 	//Add watch to folders recursively
{
	extern struct dirinfo* global_wd_list[1025];
	FILE *fp;
	extern FILE *lgfile;
	int wd,i;
	char placeholder[513]="find \"";                        //will be used as a command for popen as well as for fgets while reading the paths.
	struct dirinfo* temp=dir_init();
	strcpy(temp->path,path);
	wd=inotify_add_watch(fd,path,IN_MOVE|IN_CREATE|IN_DELETE|IN_DELETE_SELF|IN_MODIFY);
	temp->wd=wd;
	temp->parent_wd=parent_wd;
	i=wd;
	if (wd<0)
	{
		fprintf(lgfile,"Addwatch failure for %s.\n",path);
		return -1;
	}
	fprintf(lgfile,"Watching %s with wd = %d.\n",path,wd);
	global_wd_list[wd]=temp;
	strcat(placeholder,path);
	strcat(placeholder, "\" -mindepth 1 -maxdepth 1 -type d");
	fp=popen(placeholder,"r");
	if (fp==NULL)
	{
		fprintf(lgfile,"Error reading file.\n");
		raise(SIGINT);
	}
	while (fgets(placeholder,513,fp)!=NULL)
	{
		placeholder[strlen(placeholder)-1]='\0';
		wd=add_watch_rec(fd,placeholder,i);
		if (wd<0)
		{
			fprintf(lgfile,"Addwatch failure for %s.\n",placeholder);
			continue;
		}
		temp->children[temp->child_count]=global_wd_list[wd];
		temp->child_count+=1;
	}
	return i;
}

void remove_watch(int fd, int wd, int no_error_message)		//Remove watches recursively starting with the given wd.
{
	extern struct dirinfo * global_wd_list[1025];
	if (global_wd_list[wd]->child_count!=0)
	{
		int i;
		for (i=0;i<global_wd_list[wd]->child_count;i++)
		{
			remove_watch(fd,global_wd_list[wd]->children[i]->wd,no_error_message);
		}
	}
	if (inotify_rm_watch(fd,wd)==0)
	{	
		fprintf(lgfile,"Watch removed for wd=%d\n",wd);
	}
	else
	{
		if (no_error_message==0)
		{
			fprintf(lgfile,"Watch removal failure for wd=%d\n",wd);
		}
	}
}

void remove_all(int i)					//Remove all watches linked to a particular fd.
{
	extern struct fdinfo *global_fd_list[10];
	int j;
	extern FILE *lgfile;
	fprintf(lgfile,"\nRemoving.\n");
	for (j=0;j<global_fd_list[i]->count;j++)
	{
		remove_watch(global_fd_list[i]->fd,global_fd_list[i]->wdlist[j],0);
	}
	fclose(lgfile);
	lgfile=fopen("logfile.txt","a");
}

int fd_update(char *path,int fd)			//Create fd and add watch to the path.
{
	extern struct fdinfo* global_fd_list[10];
	extern int fd_count;
	extern FILE *lgfile;
	if (fd!=-1)
	{
		int wd=add_watch_rec(fd,path,-1);
		if (wd<0)
		{
			fprintf(lgfile,"Addwatch failure for %s\n",path);
			return -1;
		}
		global_fd_list[fd]->wdlist[global_fd_list[fd]->count]=wd;
		global_fd_list[fd]->count+=1;
		return fd;
	}
	else
	{
		fd=inotify_init();
		if (fd<0)
		{
			fprintf(lgfile,"inotify_init() failure.\n");
			raise(SIGINT);
		}
		struct fdinfo * fdd = (struct fdinfo*)malloc(sizeof(struct fdinfo));
		fdd->count=1;
		fdd->fd=fd;
		int wd=add_watch_rec(fd,path,-1);
		if (wd<0)
		{
			fprintf(lgfile,"Addwatch failure for %s\n",path);
			return -1;
		}
		fdd->wdlist[0]=wd;
		global_fd_list[fd_count]=fdd;
		fd_count+=1;
		return fd;
	}
	fclose(lgfile);
	lgfile=fopen("logfile.txt","a");
}

int add_newdir(int fd,int parent_wd,char *path)				//Add new dir when inotify-event of a directory creation/move is detected.
{
	extern struct dirinfo* global_wd_list[1025];
	extern FILE *lgfile;
	int wd= add_watch_rec(fd,path,parent_wd);
	global_wd_list[parent_wd]->children[global_wd_list[parent_wd]->child_count]=global_wd_list[wd];
	global_wd_list[parent_wd]->child_count+=1;
	fclose(lgfile);
	lgfile=fopen("logfile.txt","a");
	return wd;
}

void update_parent_directory(parent_wd,i) {
	extern struct dirinfo* global_wd_list[1025];
	if (i==global_wd_list[parent_wd]->child_count-1) {
		global_wd_list[parent_wd]->children[i]=NULL;
		global_wd_list[parent_wd]->child_count-=1;
	}
	else {
		global_wd_list[parent_wd]->children[i]=global_wd_list[parent_wd]->children[global_wd_list[parent_wd]->child_count-1];
		global_wd_list[parent_wd]->child_count-=1;
	}
}


int remove_dir(int fd,int parent_wd,char*path)		//Remove directory when an inotify-event of dir-moved-out/dir-deletion is detected.
{
	extern struct dirinfo* global_wd_list[1025];
	extern FILE *lgfile;
	int i=find_child_in_dir(parent_wd,path);
 	if (i==-1) {
		return -1;
	}
	int wd=global_wd_list[parent_wd]->children[i]->wd;
	remove_watch(fd,wd,0);
	update_parent_directory(parent_wd,i);
	fclose(lgfile);
	lgfile=fopen("logfile.txt","a");
	return i;
}

int put_event_in_stack(struct inotify_event* event) {	//Put event in the event_stack with optimization (Deleting non-useful events)
	char *temp=(char *)malloc(sizeof(char)*(strlen(&global_wd_list[event->wd]->path[0])+strlen(event->name)+1)+1);
	sprintf(temp,"%s/%s",&global_wd_list[event->wd]->path[0],event->name);
	int status_dir=find_child_in_dir(event->wd,temp);
	if (event->mask &IN_CREATE && test_if_file_exists(temp)!=0 && status_dir==-1) {return 0;}
	if (event->mask &IN_MODIFY) {
		if (test_if_file_exists(temp)!=0) {return 0;}
		int i=0;
		for (i=0;i<event_occurred_count;i++) {
			if (event_stack[i]==NULL) {
				continue;
			}	
			if (event_stack[i]->wd==event->wd && event_stack[i]->name==event->name && event_stack[i]->mask &IN_MODIFY) {
				event_stack[i]=event;
				return 0;
			}
		}
		event_stack[i]=event;
		event_occurred_count+=1;
		return 0;
	}
	else if (event->mask &IN_DELETE) {
		if (status_dir==-1) {
			int i=0,status=0;
			for (i=0;i<event_occurred_count;i++) {
				if (event_stack[i]==NULL) {
					continue;
				}
				if (event_stack[i]->wd==event->wd && event_stack[i]->name==event->name) {
					event_stack[i]=NULL;
					if (event_stack[i]->mask &IN_CREATE) {
						status=1;
					}
				}
			}
			if (status==0) {
				event_stack[i]=event;
				event_occurred_count+=1;
			}
			return 0;
		}
		else {
			int wd_dir=global_wd_list[event->wd]->children[status_dir]->wd;
			int i=0,status=0;
			for (i=0;i<event_occurred_count;i++) {
				if (event_stack[i]==NULL) {
					continue;
				}
				if (event_stack[i]->wd==event->wd && event_stack[i]->name==event->name) {
					event_stack[i]=NULL;
					if (event_stack[i]->mask &IN_CREATE) {
						status=1;
					}
				}
				if (event_stack[i]->wd==wd_dir) {
					event_stack[i]=NULL;
				}
			}
			if (status==0) {
				event_stack[i]=event;
				event_occurred_count+=1;
			}
			return 0;
		}
	}
	else {
		event_stack[event_occurred_count]=event;
		event_occurred_count+=1;
	}
}

void reading(int fd) {			//Reading inotify-events...Blocked until an event occurs.
	int length;
	char *p;
	char buffer[BUFLEN];
	int sleeptime=2;
	while (1)
	{
		length=read(fd,buffer,BUFLEN);
		if (length==0)
		{
			fprintf(lgfile,"read() returned 0!.\n");
			raise(SIGINT);
		}
		if (length<0)
		{
			fprintf(lgfile,"read() failed.\n");
			raise(SIGINT);
		}
		for (p=buffer;p<buffer+length;)
		{
			struct inotify_event* event=(struct inotify_event*)malloc(sizeof(struct inotify_event)+((struct inotify_event*)p)->len);
			memcpy(event,((struct inotify_event*)p),sizeof(struct inotify_event)+((struct inotify_event*)p)->len);
			strcpy(event->name,((struct inotify_event*)p)->name);
			while (event_occurred_count>=MAX_EVENT_STACK_SIZE-1) {		//Waiting until the event stack is free
				sleep(sleeptime);
				if (sleeptime<300) {sleeptime+=5;}
			}
			/* Locking thread-resource and put event in the stack */
			pthread_mutex_lock(&inotify_event_lock);
			put_event_in_stack(event);
			pthread_mutex_unlock(&inotify_event_lock);
			/* Unlocking thread-resource for modification. */
			p+=SIZE+event->len;
		}
	}
}













	






