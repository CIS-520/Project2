#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include <stdlib.h>
#include "filesys/filesys.h"
#include "threads/synch.h"
struct lock *file_lock;

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  printf ("system call!\n");
  thread_exit ();
}

void exit (int status)
{
    //store current thread wanting to terminate
    struct thread *t = thread_current();

    //if thread exists
    if(thread_alive(t->parent))   
    {
      //set child status to parameter
       t->cp->status = status;
    }
    sprintf("%s: exit(%d)\n",t->name, status);

    //process exit
    thread_exit();
}

pid_t exec (const char *cmd_line)
{
    pid_t curr_pid = process_execute(cmd_line);

    //get child process
    struct child_process* cp = get_child_process(curr_pid);

    //if program cannot load
    if(cp->load == LOAD_FAIL)
    {
        return ERROR; 
    }
    
    //endure child process successfully
    //loaded its executable

}

int wait (pid_t pid)
{
    //  make sure process wait in process.c is 
    //  fully implemented
    return process_wait(pid);
}

bool create (const char *file, unsigned initial_size)
{
    lock_acquire(&file_lock);
    bool result = filesys_create(file, initial_size);
    lock_release(&file_lock);
    return result;
}

bool remove (const char *file)
{
    lock_acquire(&file_lock);
    bool result = filesys_remove(file);
    lock_release(&file_lock);
    return result;
}

int open (const char *file)
{
    lock_acquire(&file_lock);
    struct file *filename  = filesys_open(file);
    if(!filename)
    {
        //filename does not exist
        //or memory allocation fails
        lock_release(&file_lock);
        return ERROR;
    }
    lock_release(&file_lock);
    return result;

}

int filesize (int fd)
{
    lock_acquire(&file_lock);
    struct file *fil = process_get_file(fd);
    if (!fil)
    {
        lock_release(&file_lock);
        return ERROR;
    }
    int size = file_length(fil);
    lock_release(&file_lock);
    return size;
}

//reads size bytes from the file open as fd into buffer
int read (int fd, void *buffer, unsigned size)
{
    // think about read tests failing, (reading the buffer)
    lock_acquire(&file_lock);  
    struct file *fil = process_get_file(fd);
    if (!fil)
    {
        lock_release(&file_lock);
        return -1;
    }
    int result = file_read(fil, buffer, size);
    lock_release(&file_lock);
    return result;
}

int write (int fd, const void *buffer, unsigned size)
{
    
    lock_acquire(&file_lock);  
    struct file *fil = process_get_file(fd);
    if (!fil)
    {
        lock_release(&file_lock);
        return -1;
    }
    int result = file_write(fil, buffer, size);
    lock_release(&file_lock);
    return result;
}

void seek (int fd, unsigned position)
{
    
    lock_acquire(&file_lock);  
    struct file *fil= process_get_file(fd);
    if (!fil)
    {
        lock_release(&file_lock);
        return -1;
    }
    file_seek(fil, position);
    lock_release(&file_lock);
}

unsigned tell (int fd)
{
 
    lock_acquire(&file_lock);  
    struct file *fil = process_get_file(fd);
    if (!fil)
    {
        lock_release(&file_lock);
        return -1;
    }
    off_t number = file_seek(fil);
    lock_release(&file_lock);
    return number;
}

/*void close (int fd)
{

    lock_acquire(&file_lock);
    file_close(file);
    if(!filename)
    {
        lock_release(&file_lock);
    }
    lock_release(&file_lock);
}
*/








// the following code is pulled directly from https://github.com/ryantimwilson/Pintos-Project-2/blob/master/src/userprog/syscall.c


struct child_process* add_child_process (int pid)
{
	  struct child_process* cp = malloc(sizeof(struct child_process));
	    cp->pid = pid;
	      cp->load = NOT_LOADED;
	        cp->wait = false;
		  cp->exit = false;
		    lock_init(&cp->wait_lock);
		      list_push_back(&thread_current()->child_list,
				               &cp->elem);
		        return cp;
}

struct child_process* get_child_process (int pid)
{
	  struct thread *t = thread_current();
	    struct list_elem *e;

	      for (e = list_begin (&t->child_list); e != list_end (&t->child_list);
			             e = list_next (e))
		              {
				                struct child_process *cp = list_entry (e, struct child_process, elem);
						          if (pid == cp->pid)
								          {
										            return cp;
											            }
							          }
	        return NULL;
}

void remove_child_process (struct child_process *cp)
{
	  list_remove(&cp->elem);
	    free(cp);
}

void remove_child_processes (void)
{
	  struct thread *t = thread_current();
	    struct list_elem *next, *e = list_begin(&t->child_list);

	      while (e != list_end (&t->child_list))
		          {
				        next = list_next(e);
					      struct child_process *cp = list_entry (e, struct child_process,
							                               elem);
					            list_remove(&cp->elem);
						          free(cp);
							        e = next;
								    }
}
