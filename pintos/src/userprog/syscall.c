#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
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

int 
syscall_create (struct intr_frame *f) {

	const char * filename = (char *)*(uint32_t * )(f-> esp +4); 
	printf("Value of filename %s", filename); 
	unsigned initial_size = *(unsigned*) (f-> esp + 8); 
	printf("Value of size %d", initial_size); 

	filesys_create( filename, initial_size); 


	
}

bool
syscall_remove (struct intr_frame *f) 
{
	const char * filename = (char *)*(uint32_t * )(f-> esp +4); 
	printf("Value of filename %s", filename); 

	//instructions say filesys_remove includes UNIX-like semantics
	//(removing open files for ex)
	return filesys_remove(filename); 

}

int 
syscall_open(struct intr_frame *f)
{
	const char * filename = (char *)*(uint32_t * )(f-> esp +4); 
	printf("Value of filename %s", filename); 

	FILE *fp = filesys_open(filename); 
	//https://stackoverflow.com/questions/3167298/how-can-i-convert-a-file-pointer-file-fp-to-a-file-descriptor-int-fd/19970205
	//apparently this may cause unexpected output with buffer problems
	int fd = fileno(fp); 
	return fd; 
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_number; 
  syscall_number =*(int *) f->esp;
  printf("this is the system call number right now %d \n", syscall_number);
  switch(syscall_number)
  {
	  case SYS_WRITE :
		 syscall_write(f);
		 break;
	  case SYS_CREATE : 
		 syscall_create(f); 
		 break;
	  case SYS_REMOVE : 
		 syscall_remove(f); 
		 break;
	  case SYS_OPEN : 
		 syscall_open(f); 
		 break;
	  default:
		  printf("system call! %d\n", syscall_number);
  }
  thread_exit ();
}
