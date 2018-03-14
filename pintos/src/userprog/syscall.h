#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "threads/synch.h"
void syscall_init (void);

//https://github.com/ryantimwilson/Pintos-Project-2/blob/master/src/userprog/syscall.h
/*
struct child_process{
	int pid; 
	int load; 
	bool wait; 
	bool exit; 
	int status; 
	struct lock wait_lock; 
	struct list_elem elem; 
};
*/
#endif /* userprog/syscall.h */
