#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
//#include </usr/include/unistd.h>
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

int
syscall_write (struct intr_frame *f){
int fd;

	fd = *(int*) (f->esp+4);
	printf("Value of SD %d", fd);
	char *buffer = (char *)*(uint32_t *) (f->esp+8);
	printf("value of buffer %s", buffer); 
	unsigned size = *(unsigned* ) (f->esp+12);
	printf("value of size %d", size); 

	
	printf("value of buffer"); 
	int i; 
	for (int i= 0; i < size; i++)
	{
		printf("%c", buffer[i]);
	}
	

}



static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_number; 
  syscall_number =*(int *) f->esp;

  switch(syscall_number)
  {
	  case SYS_WRITE :
		 syscall_write(f);
		 break;
	  default:
		  printf("system call! %d\n", syscall_number);
  }
  thread_exit ();
}
