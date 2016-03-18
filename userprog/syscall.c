#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* These header files were added in */
#include "filesys/filesys.h"
#include "filesys/files.h"
#include "lib/kernel/list.h"
/* End of header files added in */

/* These Global variables were added in */
#define MAX_FILE_SIZE 204800
#define MAX_FILE_NAME_LENGTH 64



/* End of Global variables added in */

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

//TO DO: syscalls: create, remove, open, filesize

//NOTE: Global Variables "MAX_FILE_SIZE" and "MAX_FILE_NAME_LENGTH" must be defined
bool
create (const char *file, unsigned initial_size)
{	
	//Case 1: "initial_size" is lower than the limit
	if (initial_size <= MAX_FILE_SIZE){
		//Must check if file name length is lower than limit too
		if (strlen(file) > MAX_FILE_NAME_LENGTH){
			printf("Cannot create file because file name is too long");
			}
		else if (strlen(file) == 0){
			printf("Cannot create file because there is no file name");
			}
		//file name length and initial size are lower than the limits...
		//...then it's ok to create the file then
		else{
			return filesys_create(file, initial_size);
			//NOTE: filesys_create is a built in function located in "filesys.c"
		}
	}
	//Case 2: "initial_size" is larger than the limit
	else{
		printf("File size is too large!");
	}

return false;
}

bool
remove (const char *file)
{
return true;
}

int
open (const char *file)
{
return true;
}

int
filesize (int fd)
{
return true;
}
















