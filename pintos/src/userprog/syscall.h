#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H


/// this code was added from https://github.com/ryantimwilson/Pintos-Project-2/blob/master/src/userprog/syscall.h#L4
#include "threads/synch.h"

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

struct child_process {
	  int pid;
	    int load;
	      bool wait;
	        bool exit;
		  int status;
		    struct lock wait_lock;
		      struct list_elem elem;
};

struct child_process* add_child_process (int pid);
struct child_process* get_child_process (int pid);
void remove_child_process (struct child_process *cp);
void remove_child_processes (void);

void process_close_file (int fd);



// end of adding stuff from website.....

void syscall_init (void);

#endif /* userprog/syscall.h */
