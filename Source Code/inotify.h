#ifndef INOTIFY_H
#define INOITFY_H
#include<pthread.h>
#include<fcntl.h>
#include<sys/mman.h>
#define STACKSIZE 50
#define MAX_EVENT_STACK_SIZE 50
struct dirinfo {
	char path[513];
	struct dirinfo *children[50];
	int child_count;
	int wd;
	int parent_wd;
};

struct fdinfo {
	int fd;
	int wdlist[10];
	int count;
};

struct subprocess {						//Struct for holding info about the rsync process running.
	struct subprocess* next;
	struct subprocess* prev;
	struct inotify_event* event;
	int pid;
	char *command;
	char *path;
	char *start_time;
	int retries;
};

struct process_list {						//Linked list head.
	struct subprocess* starting;
	struct subprocess* last;
};

struct process_list pid_list;					//Global Linked list head.
struct dirinfo* global_wd_list[1025];                          //The global list for wd's and the directory address
struct fdinfo * global_fd_list[10];                            //The global list of fd's
int fd_count;							//No. of fd's in existence.
FILE *lgfile;                                                     //File pointer for the lgfilefile.
void *mmap_file_pointer;						//Pointer to mmap memory
struct flock file_lock;						//File lock structure
int mmap_fd;							//File descriptor for the mmap file.
struct inotify_event* event_stack[MAX_EVENT_STACK_SIZE];                         //Maintain the stack of inotify events.
int event_occurred_count; 					//Maintain the count - Makes things easier
int subprocess_count;
pthread_mutex_t inotify_event_lock;
pthread_t inotify_thread;
char address[50];						//Ip address and username
char dest_folder[50];
char home_folder[50];

struct dirinfo * dir_init();                                  //Initialize a dirinfo structure
int find_child_in_dir(int parent_wd,char *path);              //finding the position in the children list of a child when it's path is available.
int  checkif_dir(char *path);                                //check if a given path is a directory or not
int checkif_regfile(char *path);
int test_if_file_exists(char *path);
int test_if_dir_exists(char *path);
void remove_watch(int fd,int wd,int no_error_message);        //Remove watches from a folder recursively (Child subdirectories also removed.)
void remove_all(int fd);                                      //Remove all watches in an fd.
int add_watch_rec(int fd,char *path,int parent_wd);           //Add watch to a folder and its subdirectories recursively.
int fd_update(char *path,int fd);                              //Updates the fd with a new child watch or creates a new fd.
void reading(int fd);                               //Reading events (Time specified) 
void read_event(struct inotify_event* event,int fd);
int add_newdir(int fd,int parent_wd,char *path);              //Event: New directory addition - This function modifies the adjancency list properly.
int remove_dir(int fd,int parent_wd,char *path);              //Event: Directory removed - This function modifies the adjancecy list properly.
int put_event_in_stack(struct inotify_event* event);
void destroy_all();                                           //Remove all watches
void sigint_handler(int signum);                       //Standard SIGINT handler
void timeout(int signum);
#endif
