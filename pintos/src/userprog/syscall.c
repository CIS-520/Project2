#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include <user/syscall.h>
static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


struct file_info{

	struct file *file; 
	int fd; 
	char *filename; 
	struct list_elem elem; 
};

struct waiting_thread_info{
	struct thread *thread; 
	pid_t pid; 
	struct list_elem elem; 
};


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

	return 0; 
	

}

int 
syscall_create (struct intr_frame *f) {

	const char * filename = (char *)*(uint32_t * )(f-> esp +4); 
	printf("Value of filename %s", filename); 
	

	unsigned initial_size =10;  
		//*(unsigned*) (f-> esp + 8); 
	printf("Value of size %d", initial_size); 

	int success = 	filesys_create( filename, initial_size); 
	return success;


	
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
syscall_close(struct intr_frame *f)
{

	//grab fd from the stack 
	int fd = *(int*) (f->esp+4);

	//get the current thread so i can loop through its files
	struct thread *current = thread_current(); 
	
	//grab the current threads file list
	struct list_elem *next, *e = list_begin(&current -> list_of_files); 

	//loop through the thread list and remove correct file_info struct
	while (e != list_end(&current-> list_of_files))
	{
		next = list_next(e); 

		//get my file_info from the list_elem
		struct file_info *f_info = list_entry(e, struct file_info, elem); 

		//check if the fd's are the same
		if (fd == f_info -> fd)
		{
			file_close(f_info-> file); 
			list_remove(&f_info->elem); 
			free(f_info); 

		}
		e = next; 
	}

	//change this return statement
	return 0; 
}

int 
syscall_open(struct intr_frame *f)
{

	const char * filename = (char *)*(uint32_t * )(f-> esp +4); 
	printf("Value of filename %s", filename); 

	//make the structure to store file_info
	struct file_info *fp_info = malloc(sizeof(struct file_info)); 

	struct thread *current = thread_current(); 
	struct file *fp = filesys_open(filename); 
	fp_info -> file = fp;
	fp_info -> fd = current-> fd; 
	fp_info -> filename = filename; 

	//add file_info to threads list of files 
	current->fd++; 
	list_push_back( &current-> list_of_files, &fp_info-> elem); 

	//return file_descriptor
	return fp_info -> fd; 
	
}

void
syscall_exit(struct intr_frame *f)
{
	//0 indicates success 
	//non zero indicates failures
	
	//print the process name and exit code
	struct thread *current = thread_current(); 
	tid_t pid; 
	char *pname = current->name; //i could use thread_name() but it calls thread_current()
	//more efficient 

	//notify waiting threads to wake up
	struct list_elem *next, *e = list_begin(&current -> waiting_threads); 

	//loop through the thread list and remove correct file_info struct
	while (e != list_end(&current-> waiting_threads))
	{
		next = list_next(e); 

		//get my file_info from the list_elem
		struct waiting_list_info *thread_info = list_entry(e, struct waiting_list_info, elem); 

		//check if the fd's are the same
		if (pid == thread_info -> pid)
		{
			thread_unblock(thread_info->thread); 
		}
		e = next; 
	}
	
	int status = *(int*) (f->esp+4);
	//printf ("%s: exit(%d)\n", pname, pid);
	process_exit(); 
	
	
	//i don't know the logic to find out if the process exited ok 
	status = 0; 
}



int
syscall_filesize (struct intr_frame *f)
{

	int fd = *(int*) (f->esp+4);
	
	//https://stackoverflow.com/questions/6537436/how-do-you-get-file-size-by-fd
	//struct file *fp = fdopen(fd,"w"); 
	//fsize is measured in bytes
	//return file_length(fp); 
	return 0; 

}

int
syscall_read( struct intr_frame *f)
{
	int fd = *(int*) (f->esp+4);
	printf("Value of SD %d", fd);
	char *buffer = (char *)*(uint32_t *) (f->esp+8);
	printf("value of buffer %s", buffer); 
	unsigned size = *(unsigned* ) (f->esp+12);
	printf("value of size %d", size); 

	//i am assumming that we want to read from the current position in the file
	//function below returns number of bytes read
	//https://stackoverflow.com/questions/1941464/how-to-get-a-file-pointer-from-a-file-descriptor
	//struct file *fp = fdopen(fd, "w");
	//i am assumming that file_read returns -1 if error
	//return file_read(fp, buffer, size); 
	return 0; 
	

}

int
syscall_wait( struct intr_frame *f)
{

	pid_t pid = *(int*) (f->esp+4);
	//before I block the thread, tell waiting_on_thread
	
	//is pid alive?
	
	struct thread *current = thread_current(); 

	//make waiting_thread_info struct
	struct waiting_thread_info *thread_info = malloc(sizeof(struct waiting_thread_info)); 
	thread_info -> thread = current; 
	thread_info -> pid = current -> tid; 

	//add the thread 
	list_push_back( &current-> waiting_threads, &thread_info-> elem); 
	thread_block(); 
	
	//return status that pid returns; 
	return 0; 

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
	  case SYS_CLOSE : 
		 syscall_close(f); 
		 break;
	  case SYS_EXIT : 
		 syscall_exit(f); 
		 break;
	  case SYS_FILESIZE:
		 syscall_filesize(f); 
		 break;
	  case SYS_READ:
		 syscall_read(f); 
		 break;
	  case SYS_WAIT:
		 syscall_wait(f); 
		 break;
	  default:
		  printf("system call! %d\n", syscall_number);
  }
  thread_exit ();
}
