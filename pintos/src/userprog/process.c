#include "userprog/process.h"
#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "userprog/syscall.h"
static thread_func start_process NO_RETURN;

char global_filename [14];

static bool load (const char *cmdline, void (**eip) (void), void **esp, char** save_ptr);


/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */

 // char *token = strtok_r(file_name, " ", file_name); 
  //strcpy might not be the most secure method
  //i am unsure what is the secure method
  //i am worried about some serious buffer overflow
//char *program_name;
  //strcpy(destination, source)
  //first token is the program name

  //which exec do we call ?

  //other tokens are arguments
//  while(token = strtok_r(file_name, " " , filename)
 // {
  
  //}
  
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;
  char * next_token_ptr;
  char *token = strtok_r(file_name, " ", &next_token_ptr); 
 // printf("this is the token %s", token);
  

  char   program_name[14];
  strlcpy(program_name,token,sizeof(program_name));

  strlcpy(global_filename,program_name,sizeof(global_filename));
  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
//  printf("file name is: %s\n", program_name);
  success = load (program_name, &if_.eip, &if_.esp, &next_token_ptr); // LETS GIVE IT ONLY THE PROPER FILE NAME, THEN RUN A TEST IF IT STILL WORKS 
  //allocaet new page. make sure accessible from userprog. copy arguments to page. 

  /* If load failed, quit. */
  palloc_free_page (file_name);
  if (!success) 
    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
  timer_sleep(1000);
  printf("%s: exit(%d)\n",&global_filename,  0);
  //return -1;
  /*
  struct thread * cur_thread = thread_current();

  cur_thread->waiting_for_child = true;
  sema_init(&cur_thread->wait_child_sema);
  sema_down(&cur_thread->wait_child_sema);
  cur_thread->waiting_for_child = false;

  printf("Child exitd with status<%d>!\n", cur_thread->child_exit_status);
  return -1;
  */


// this is code from ryantimewilson
//
// this code that isn't mine isn't working I am trying to figure out why my program is getting stck
/*
	struct child_process* cp = get_child_process(child_tid);
	if (!cp)
	{
		return -1;
	}
	if(cp->wait)
	{
		return -1;
	}
	cp -> wait = true;
	while (!cp->exit)
	{
		barrier();
	}
	int status = cp-> status;
	remove_child_process(cp);
	return status;
*/
	return 0;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

#define WORD_SIZE 4
#define DEFAULT_ARGV 8
static bool setup_stack (void **esp,const char* file_name,
					 char** save_ptr);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp, char** save_ptr) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;
  //file_name = "echo"; // WE NEED TO CHANGE THIS, BECAUSE CURRENTLY IT IS HARD CODED TO KNOW THAT THE FILE WE ARE RUNNING IS ECHO

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  
  //printf("this is the file name %s \n", file_name);// debuggingggggggg delete later
	
  /* Open executable file. */
  file = filesys_open (file_name);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                               - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }
      //parse arguments here   -start of MG code
/*	   uint8_t *kpage;
	  bool success = false;

	  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
	  if (kpage != NULL) 
	    {
	      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
	      if (success)
		      //i don't know if this will work
		      //512 comes from 128 max arguments * 4 bytes each
			*esp = PHYS_BASE - 512;
	      else
		palloc_free_page (kpage);
	    }

	 void **argument_list_start; 
	 *argument_list_start = PHYS_BASE; 
	 void **arg_pointer; 
	 arg_pointer = argument_list_start; 

	  //put argc and argv on the stack 
	  //esp is a double pointer
	  //argv first on stack
	  //then argc on stack 
	  *esp = *esp - 4; 
	  **esp = argv;
	  *esp = *esp - 4; 
	  **esp = argc;
	  //esp now points to argc

	  //parse the arguments and put in page memory in array
	  char *token; 
	  while (token = strtok_r(file_name, " ", &next_token_ptr))
	  {
		// file name DOES NOT HAVE EVERYTHING.. .AND WE NEED TO CHANGE TOKEN
		//move before i put anything on
		*arg_pointer = *arg_pointer- 4; 
		**arg_pointer = token; 
	  }

	 

     // end of MG code
*/
    
  /* Set up stack. */
  if (!setup_stack (esp,file_name, save_ptr))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;
  
  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        {
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}



//this will be a function that will push the arguments on the stack ( it was given by chandra )

uint32_t * push_arg(uint32_t * stack_ptr, uint32_t arg)
{
    stack_ptr--;
    *stack_ptr = arg;

    return stack_ptr;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, const char * file_name, char **save_ptr) 
{
  uint8_t *kpage;
  bool success = false;
	
  
  //this will be 'chandra's implementation of argument passing doen inside of setup stack, this will make it so that we don't have to re-copy code. 

  
  uint32_t * stack_ptr;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE - 12;
      else
        palloc_free_page (kpage);
    }
  
  char *token;
  char **argv;
  int i, argc = 0, argv_size = DEFAULT_ARGV;
 
  uint8_t * newpage = palloc_get_page (PAL_USER | PAL_ZERO);
  uint8_t * newpage_ptr = (uint8_t *) 0x08048000 - PGSIZE;
  if (newpage != NULL) 
    {
      success = install_page (newpage_ptr, newpage, true);
      if (!success)
        palloc_free_page (newpage);
    }

 
 strlcpy(newpage_ptr, file_name, strlen(file_name)+1);
 strlcpy(newpage_ptr+strlen(file_name), "       ", 5);
 strlcpy(newpage_ptr+strlen(file_name)+1, *save_ptr,  strlen(*save_ptr)+1);
//  stack_ptr = esp;

*esp = *esp - 128;
argv = (char **)*esp;

char * next_token;

argv[0] = strtok_r(newpage_ptr, " ", &next_token);

while(++argc) {
	if( (argv[argc] = strtok_r(NULL, " ", &next_token) ) == NULL)
		break;

}

/*
  for (token = (char *) file_name; token != NULL;
		         token = strtok_r (NULL, " ", save_ptr))
  {
//	  //printf("this is current token: %s \n", token);
	 *esp -= strlen(token) + 1;
	// printf("esp is <%x>!\n", (int)*esp);
	 argv[argc] = *esp;
	// printf("this is what is in argv at current count %s \n", argv[argc]);
	//argc++;
	//printf("current value of argc is %d \n", argc);
	memcpy(*esp, token, strlen(token) + 1);
//	printf("argv[%d] is <%s>!\n", argc, argv[argc]);
	argc++;
  }

  //stack_ptr = push_arg(stack_ptr, 0x00ff0000); //pointer to argv
  //stack_ptr = push_arg(stack_ptr, 0x5a5a5a5a); //pointer to argc
  //stack_ptr = push_arg(stack_ptr, 0x00223344); //name of the program
 // *esp = stack_ptr; // not sure if I still need this probably
 //argv[argc] = 0xDEADBEEF;
 */ 
 *(uint32_t *)(*esp-4) = (uint32_t)argv;
 *(uint32_t *)(*esp-8) = argc;
 *(uint32_t *)(*esp-12) = 0xDEADBEEF;
 *esp = *esp - 12;
//int j;

// this code is not mine,,,,

// Align to word size (4 bytes)
 /*  i = (size_t) *esp % WORD_SIZE;
   if (i)
  {
    *esp -= i;
    memcpy(*esp, &argv[argc], i);
   }*/
// Push argv[i] for all i
//for (i = argc; i >= 0; i--)
 //{
  // *esp -= sizeof(char *);
 //printf("this is the current argument %s number: %d \n", argv[i], i);
  // memcpy(*esp, &argv[i], sizeof(char *));
//}
  // Push argv
 /* token = *esp;
  *esp -= sizeof(char **);
  memcpy(*esp, &token, sizeof(char **));
  // Push argc
  *esp -= sizeof(int);
   memcpy(*esp, &argc, sizeof(int));
 // Push fake return addr
  *esp -= sizeof(void *);
    memcpy(*esp, &argv[argc], sizeof(void *));
   // printf("this is the final argc value %d \n", argc);
  // Free argv
  free(argv);

   // printf("this is the final argv value at 0 %d \n", &argv[0]);
*/
//  ASSERT(*esp == (PHYS_BASE-12));
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}
